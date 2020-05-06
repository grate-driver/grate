/*
 * Copyright (c) 2017 Dmitry Osipenko <digetx@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <linux/fb.h>
#include <sys/ioctl.h>

#include "nvhost.h"
#include "tegra_dc_ext.h"

static inline struct nvhost_display *to_nvhost_display(
					struct host1x_display *display)
{
	return container_of(display, struct nvhost_display, base);
}

static inline struct nvhost_display *to_nvhost_display_overlay(
					struct host1x_overlay *overlay)
{
	return container_of(overlay, struct nvhost_display, overlay);
}

static int overlay_set(struct host1x_overlay *overlayp,
		       struct host1x_framebuffer *fb, unsigned int x,
		       unsigned int y, unsigned int width,
		       unsigned int height, bool vsync, bool reflect_y)
{
	struct nvhost_display *display = to_nvhost_display_overlay(overlayp);
	struct nvhost *nvhost = display->nvhost;
	struct tegra_dc_ext_flip flip;
	uint32_t vblank_cnt;
	unsigned pixformat;
	int err;

	switch (fb->pixbuf->format) {
	case PIX_BUF_FMT_RGB565:
		pixformat = TEGRA_DC_EXT_FMT_B5G6R5;
		break;
	case PIX_BUF_FMT_RGBA8888:
		pixformat = TEGRA_DC_EXT_FMT_R8G8B8A8;
		break;
	default:
		return -1;
	}

	memset(&flip, 0, sizeof(flip));

	flip.win[0].index = -1;
	flip.win[1].index = -1;
	flip.win[2].index = display->plane;
	flip.win[2].stride = fb->pixbuf->pitch;
	flip.win[2].offset = fb->pixbuf->bo->offset;
	flip.win[2].buff_id = fb->pixbuf->bo->handle;
	flip.win[2].pixformat = pixformat;
	flip.win[2].w = fb->pixbuf->width << 12;
	flip.win[2].h = fb->pixbuf->height << 12;
	flip.win[2].out_x = x;
	flip.win[2].out_y = y;
	flip.win[2].out_w = width;
	flip.win[2].out_h = height;
	flip.win[2].pre_syncpt_id = (uint32_t)-1;

	if (reflect_y)
		flip.win[2].flags = TEGRA_DC_EXT_FLIP_FLAG_INVERT_V;

	if (fb->pixbuf->layout == PIX_BUF_LAYOUT_TILED_16x16)
		flip.win[2].flags |= TEGRA_DC_EXT_FLIP_FLAG_TILED;

	if (vsync) {
		err = nvhost_ctrl_read_syncpt(nvhost->ctrl,
					      display->vblank_syncpt,
					      &vblank_cnt);
		if (err < 0)
			return err;

		flip.win[2].pre_syncpt_id = display->vblank_syncpt;
		flip.win[2].pre_syncpt_val = vblank_cnt + 1;
	}

	err = ioctl(display->fd, TEGRA_DC_EXT_FLIP, &flip);
	if (err < 0)
		return -errno;

	err = nvhost_ctrl_wait_syncpt(nvhost->ctrl,
				      flip.post_syncpt_id,
				      flip.post_syncpt_val,
				      1000);
	if (err < 0)
		return err;

	return 0;
}

static int overlay_close(struct host1x_overlay *overlayp)
{
	struct nvhost_display *display = to_nvhost_display_overlay(overlayp);
	int err;

	err = ioctl(display->fd, TEGRA_DC_EXT_PUT_WINDOW, display->plane);
	if (err < 0)
		return -errno;

	return 0;
}

static int overlay_create(struct host1x_display *displayp,
			  struct host1x_overlay **overlayp)
{
	struct nvhost_display *display = to_nvhost_display(displayp);
	int plane = TEGRA_DC_EXT_FLIP_N_WINDOWS;
	int err;

	while (--plane >= 0) {
		err = ioctl(display->fd, TEGRA_DC_EXT_GET_WINDOW, plane);
		if (err == 0)
			break;
	}

	if (err < 0)
		return -errno;

	display->overlay.close = overlay_close;
	display->overlay.set = overlay_set;
	display->plane = plane;

	*overlayp = &display->overlay;

	return 0;
}

static int display_set(struct host1x_display *displayp,
		       struct host1x_framebuffer *fb,
		       bool vsync, bool reflect_y)
{
	struct nvhost_display *display = to_nvhost_display(displayp);
	int plane;
	int err;

	for (plane = 0; plane < TEGRA_DC_EXT_FLIP_N_WINDOWS; plane++) {
		err = ioctl(display->fd, TEGRA_DC_EXT_GET_WINDOW, plane);
		if (err == 0)
			break;
	}

	if (err < 0)
		return -errno;

	display->plane = plane;

	return overlay_set(&display->overlay, fb, 0, 0,
			   displayp->width, displayp->height,
			   vsync, reflect_y);
}

struct nvhost_display * nvhost_display_create(struct nvhost *nvhost)
{
	struct tegra_dc_ext_control_output_properties output_properties;
	struct fb_var_screeninfo fb_info;
	struct nvhost_display *display;
	uint32_t num_outputs = 0;
	uint32_t syncpt;
	uint32_t output;
	int err;
	int fd;

	fd = open("/dev/tegra_dc_ctrl", O_RDWR);
	if (fd < 0)
		return NULL;

	err = ioctl(fd, TEGRA_DC_EXT_CONTROL_GET_NUM_OUTPUTS, &num_outputs);
	if (err < 0) {
		close(fd);
		return NULL;
	}

	for (output = 0; output < num_outputs; output++) {
		output_properties.handle = output;

		err = ioctl(fd, TEGRA_DC_EXT_CONTROL_GET_OUTPUT_PROPERTIES,
			    &output_properties);
		if (err < 0)
			continue;

		if (!output_properties.connected)
			continue;

		break;
	}

	close(fd);

	if (output == num_outputs)
		return NULL;

	switch (output) {
	case 0:
		fd = open("/dev/fb0", O_RDWR);
		break;
	case 1:
		fd = open("/dev/fb1", O_RDWR);
		break;
	default:
		return NULL;
	}

	if (fd < 0)
		return NULL;

	err = ioctl(fd, FBIOGET_VSCREENINFO, &fb_info);

	close(fd);

	if (err < 0)
		return NULL;

	switch (output) {
	case 0:
		fd = open("/dev/tegra_dc_0", O_RDWR);
		break;
	case 1:
		fd = open("/dev/tegra_dc_1", O_RDWR);
		break;
	default:
		return NULL;
	}

	if (fd < 0)
		return NULL;

	err = ioctl(fd, TEGRA_DC_EXT_SET_NVMAP_FD, nvhost->nvmap->fd);
	if (err < 0) {
		close(fd);
		return NULL;
	}

	err = ioctl(fd, TEGRA_DC_EXT_GET_VBLANK_SYNCPT, &syncpt);
	if (err < 0) {
		close(fd);
		return NULL;
	}

	display = calloc(1, sizeof(*display));
	if (!display) {
		close(fd);
		return NULL;
	}

	display->fd = fd;
	display->nvhost = nvhost;
	display->base.width = fb_info.xres;
	display->base.height = fb_info.yres;
	display->base.create_overlay = overlay_create;
	display->base.set = display_set;
	display->vblank_syncpt = syncpt;

	return display;
}

void nvhost_display_close(struct nvhost_display *display)
{
	struct tegra_dc_ext_flip flip;

	if (!display)
		return;

	memset(&flip, 0, sizeof(flip));

	flip.win[0].index = display->plane;
	flip.win[1].index = -1;
	flip.win[2].index = -1;

	ioctl(display->fd, TEGRA_DC_EXT_FLIP, &flip);

	close(display->fd);
	free(display);
}
