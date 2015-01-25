/*
 * Copyright (c) 2012, 2013 Erik Faye-Lund
 * Copyright (c) 2013 Thierry Reding
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
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include <libdrm/drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "host1x-private.h"
#include "tegra_drm.h"

struct drm;

struct drm_bo {
	struct host1x_bo base;
	struct drm *drm;
};

static inline struct drm_bo *to_drm_bo(struct host1x_bo *bo)
{
	return container_of(bo, struct drm_bo, base);
}

struct drm_channel {
	struct host1x_client client;
	uint64_t context;
	struct drm *drm;
	uint32_t fence;
};

static inline struct drm_channel *to_drm_channel(struct host1x_client *client)
{
	return container_of(client, struct drm_channel, client);
}

struct drm_display {
	struct host1x_display base;
	struct drm *drm;
	drmModeModeInfo mode;
	uint32_t connector;
	unsigned int pipe;
	uint32_t crtc;
};

static inline struct drm_display *to_drm_display(struct host1x_display *display)
{
	return container_of(display, struct drm_display, base);
}

struct drm_overlay {
	struct host1x_overlay base;
	struct drm_display *display;
	uint32_t plane;

	unsigned int x;
	unsigned int y;
	unsigned int width;
	unsigned int height;
	uint32_t format;
};

static inline struct drm_overlay *to_drm_overlay(struct host1x_overlay *overlay)
{
	return container_of(overlay, struct drm_overlay, base);
}

struct drm_gr2d {
	struct drm_channel channel;
	struct host1x_gr2d base;
};

struct drm_gr3d {
	struct drm_channel channel;
	struct host1x_gr3d base;
};

struct drm {
	struct host1x base;

	struct drm_display *display;
	struct drm_gr2d *gr2d;
	struct drm_gr3d *gr3d;

	int fd;
};

static struct drm *to_drm(struct host1x *host1x)
{
	return container_of(host1x, struct drm, base);
}

static int drm_display_find_plane(struct drm_display *display, uint32_t *plane)
{
	struct drm *drm = display->drm;
	drmModePlaneRes *res;
	uint32_t id = 0, i;

	res = drmModeGetPlaneResources(drm->fd);
	if (!res)
		return -errno;

	for (i = 0; i < res->count_planes && !id; i++) {
		drmModePlane *p = drmModeGetPlane(drm->fd, res->planes[i]);
		if (!p) {
		}

		if (!p->crtc_id && (p->possible_crtcs & (1 << display->pipe)))
			id = p->plane_id;

		drmModeFreePlane(p);
	}

	drmModeFreePlaneResources(res);

	if (!id)
		return -ENODEV;

	if (plane)
		*plane = id;

	return 0;
}

static int drm_overlay_close(struct host1x_overlay *overlay)
{
	struct drm_overlay *plane = to_drm_overlay(overlay);
	struct drm_display *display = plane->display;
	struct drm *drm = display->drm;

	drmModeSetPlane(drm->fd, plane->plane, display->crtc, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0);

	free(plane);
	return 0;
}

static int drm_overlay_set(struct host1x_overlay *overlay,
			   struct host1x_framebuffer *fb, unsigned int x,
			   unsigned int y, unsigned int width,
			   unsigned int height, bool vsync)
{
	struct drm_overlay *plane = to_drm_overlay(overlay);
	struct drm_display *display = plane->display;
	struct drm *drm = display->drm;
	int err;

	if (vsync) {
		drmVBlank vblank = {
			.request = {
				.type = DRM_VBLANK_RELATIVE,
				.sequence = 1,
			},
		};

		err = drmWaitVBlank(drm->fd, &vblank);
		if (err < 0) {
			fprintf(stderr, "drmWaitVBlank() failed: %m\n");
			return -errno;
		}
	}

	err = drmModeSetPlane(drm->fd, plane->plane, display->crtc,
			      fb->handle, 0, x, y, width, height, 0, 0,
			      fb->width << 16, fb->height << 16);
	if (err < 0)
		return -errno;

	return 0;
}

static int drm_overlay_create(struct host1x_display *display,
			      struct host1x_overlay **overlayp)
{
	struct drm_display *drm = to_drm_display(display);
	struct drm_overlay *overlay;
	uint32_t plane = 0;
	int err;

	err = drm_display_find_plane(drm, &plane);
	if (err < 0)
		return err;

	overlay = calloc(1, sizeof(*overlay));
	if (!overlay)
		return -ENOMEM;

	overlay->base.close = drm_overlay_close;
	overlay->base.set = drm_overlay_set;

	overlay->display = drm;
	overlay->plane = plane;

	*overlayp = &overlay->base;

	return 0;
}

static void drm_display_on_page_flip(int fd, unsigned int frame,
				     unsigned int sec, unsigned int usec,
				     void *data)
{
}

static void drm_display_on_vblank(int fd, unsigned int frame,
				  unsigned int sec, unsigned int usec,
				  void *data)
{
}

static int drm_display_set(struct host1x_display *display,
			   struct host1x_framebuffer *fb, bool vsync)
{
	struct drm_display *drm = to_drm_display(display);
	int err;

	if (vsync) {
		struct timeval timeout;
		fd_set fds;

		err = drmModePageFlip(drm->drm->fd, drm->crtc, fb->handle,
				      DRM_MODE_PAGE_FLIP_EVENT, drm);
		if (err < 0) {
			fprintf(stderr, "drmModePageFlip() failed: %m\n");
			return -errno;
		}

		memset(&timeout, 0, sizeof(timeout));
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		FD_ZERO(&fds);
		FD_SET(drm->drm->fd, &fds);

		err = select(drm->drm->fd + 1, &fds, NULL, NULL, &timeout);
		if (err <= 0) {
		}

		if (FD_ISSET(drm->drm->fd, &fds)) {
			drmEventContext context;

			memset(&context, 0, sizeof(context));
			context.version = DRM_EVENT_CONTEXT_VERSION;
			context.page_flip_handler = drm_display_on_page_flip;
			context.vblank_handler = drm_display_on_vblank;

			drmHandleEvent(drm->drm->fd, &context);
		}
	} else {
		err = drmModeSetCrtc(drm->drm->fd, drm->crtc, fb->handle, 0,
				     0, &drm->connector, 1, &drm->mode);
		if (err < 0)
			return -errno;
	}

	return 0;
}

static int drm_display_setup(struct drm_display *display)
{
	struct drm *drm = display->drm;
	int ret = -ENODEV;
	drmModeRes *res;
	uint32_t i;

	res = drmModeGetResources(drm->fd);
	if (!res)
		return -ENODEV;

	for (i = 0; i < res->count_connectors; i++) {
		drmModeConnector *connector;
		drmModeEncoder *encoder;

		connector = drmModeGetConnector(drm->fd, res->connectors[i]);
		if (!connector)
			continue;

		if (connector->connection != DRM_MODE_CONNECTED) {
			drmModeFreeConnector(connector);
			continue;
		}

		encoder = drmModeGetEncoder(drm->fd, connector->encoder_id);
		if (!encoder) {
			drmModeFreeConnector(connector);
			continue;
		}

		display->connector = res->connectors[i];
		display->mode = connector->modes[0];
		display->crtc = encoder->crtc_id;

		drmModeFreeEncoder(encoder);
		drmModeFreeConnector(connector);
		ret = 0;
		break;
	}

	for (i = 0; i < res->count_crtcs; i++) {
		drmModeCrtc *crtc;

		crtc = drmModeGetCrtc(drm->fd, res->crtcs[i]);
		if (!crtc)
			continue;

		if (crtc->crtc_id == display->crtc) {
			drmModeFreeCrtc(crtc);
			display->pipe = i;
			break;
		}

		drmModeFreeCrtc(crtc);
	}

	drmModeFreeResources(res);
	return ret;
}

static int drm_display_create(struct drm_display **displayp, struct drm *drm)
{
	struct drm_display *display;
	int err;

	display = calloc(1, sizeof(*display));
	if (!display)
		return -ENOMEM;

	display->drm = drm;

	err = drmSetMaster(drm->fd);
	if (err < 0) {
		free(display);
		return -errno;
	}

	err = drm_display_setup(display);
	if (err < 0) {
		free(display);
		return err;
	}

	display->base.width = display->mode.hdisplay;
	display->base.height = display->mode.vdisplay;
	display->base.create_overlay = drm_overlay_create;
	display->base.set = drm_display_set;

	*displayp = display;

	return 0;
}

static int drm_display_close(struct drm_display *display)
{
	struct drm *drm;

	if (!display)
		return -EINVAL;

	drm = display->drm;

	drmDropMaster(drm->fd);
	free(display);

	return 0;
}

static int drm_bo_mmap(struct host1x_bo *bo)
{
	struct drm_bo *drm = to_drm_bo(bo);
	struct drm_tegra_gem_mmap args;
	void *ptr;
	int err;

	if (bo->ptr)
		return 0;

	memset(&args, 0, sizeof(args));
	args.handle = bo->handle;

	err = ioctl(drm->drm->fd, DRM_IOCTL_TEGRA_GEM_MMAP, &args);
	if (err < 0)
		return -errno;

	ptr = mmap(NULL, bo->size, PROT_READ | PROT_WRITE, MAP_SHARED,
		   drm->drm->fd, args.offset);
	if (ptr == MAP_FAILED)
		return -errno;

	bo->ptr = ptr;

	return 0;
}

static int drm_bo_invalidate(struct host1x_bo *bo, loff_t offset,
			     size_t length)
{
	return 0;
}

static int drm_bo_flush(struct host1x_bo *bo, loff_t offset, size_t length)
{
	return 0;
}

static void drm_bo_free(struct host1x_bo *bo)
{
	struct drm_bo *drm = to_drm_bo(bo);
	struct drm_gem_close args;
	int err;

	memset(&args, 0, sizeof(args));
	args.handle = bo->handle;

	err = ioctl(drm->drm->fd, DRM_IOCTL_GEM_CLOSE, &args);
	if (err < 0)
		fprintf(stderr, "failed to delete buffer object: %m\n");

	free(drm);
}

static struct host1x_bo *drm_bo_create(struct host1x *host1x, size_t size,
				       unsigned long flags)
{
	struct drm_tegra_gem_create args;
	struct drm *drm = to_drm(host1x);
	struct drm_bo *bo;
	int err;

	bo = calloc(1, sizeof(*bo));
	if (!bo)
		return NULL;

	bo->drm = drm;

	memset(&args, 0, sizeof(args));
	args.flags = DRM_TEGRA_GEM_CREATE_BOTTOM_UP |
		     DRM_TEGRA_GEM_CREATE_TILED;
	args.size = size;

	err = ioctl(drm->fd, DRM_IOCTL_TEGRA_GEM_CREATE, &args);
	if (err < 0) {
		free(bo);
		return NULL;
	}

	bo->base.handle = args.handle;
	bo->base.size = size;

	bo->base.mmap = drm_bo_mmap;
	bo->base.invalidate = drm_bo_invalidate;
	bo->base.flush = drm_bo_flush;
	bo->base.free = drm_bo_free;

	return &bo->base;
}

static int drm_framebuffer_init(struct host1x *host1x,
				struct host1x_framebuffer *fb)
{
	uint32_t handles[1], pitches[1], offsets[1], format;
	struct drm *drm = to_drm(host1x);
	int err;

	/* XXX: support other formats */
	if (fb->depth != 32) {
		fprintf(stderr, "ERROR: only 32-bit (XBGR8888) supported\n");
		return -EINVAL;
	}

	format = DRM_FORMAT_XBGR8888;
	handles[0] = fb->bo->handle;
	pitches[0] = fb->pitch;
	offsets[0] = 0;

	err = drmModeAddFB2(drm->fd, fb->width, fb->height, format,  handles,
			    pitches, offsets, &fb->handle, 0);
	if (err < 0)
		return -errno;

	return 0;
}

static int drm_channel_open(struct drm *drm, uint32_t class, uint64_t *channel)
{
	struct drm_tegra_open_channel args;
	int err;

	memset(&args, 0, sizeof(args));
	args.client = class;

	err = ioctl(drm->fd, DRM_IOCTL_TEGRA_OPEN_CHANNEL, &args);
	if (err < 0)
		return -errno;

	*channel = args.context;

	return 0;
}

static int drm_channel_submit(struct host1x_client *client,
			      struct host1x_job *job)
{
	struct drm_channel *channel = to_drm_channel(client);
	struct drm_tegra_reloc *relocs, *reloc;
	unsigned int i, j, num_relocs = 0;
	struct drm_tegra_cmdbuf *cmdbufs;
	struct drm_tegra_syncpt syncpt;
	struct drm_tegra_submit args;
	int err;

	memset(&syncpt, 0, sizeof(syncpt));
	syncpt.id = job->syncpt;
	syncpt.incrs = job->syncpt_incrs;

	cmdbufs = calloc(job->num_pushbufs, sizeof(*cmdbufs));
	if (!cmdbufs)
		return -ENOMEM;

	for (i = 0; i < job->num_pushbufs; i++) {
		struct host1x_pushbuf *pushbuf = &job->pushbufs[i];
		struct drm_tegra_cmdbuf *cmdbuf = &cmdbufs[i];

		cmdbuf->handle = pushbuf->bo->handle;
		cmdbuf->offset = pushbuf->offset;
		cmdbuf->words = pushbuf->length;

		num_relocs += pushbuf->num_relocs;
	}

	relocs = calloc(num_relocs, sizeof(*relocs));
	if (!relocs)
		return -ENOMEM;

	reloc = relocs;

	for (i = 0; i < job->num_pushbufs; i++) {
		struct host1x_pushbuf *pushbuf = &job->pushbufs[i];

		for (j = 0; j < pushbuf->num_relocs; j++) {
			struct host1x_pushbuf_reloc *r = &pushbuf->relocs[j];

			reloc->cmdbuf.handle = pushbuf->bo->handle;
			reloc->cmdbuf.offset = r->source_offset;
			reloc->target.handle = r->target_handle;
			reloc->target.offset = r->target_offset;
			reloc->shift = r->shift;

			reloc++;
		}
	}

	memset(&args, 0, sizeof(args));
	args.context = channel->context;
	args.num_syncpts = 1;
	args.num_cmdbufs = job->num_pushbufs;
	args.num_relocs = num_relocs;
	args.num_waitchks = 0;
	args.waitchk_mask = 0;
	args.timeout = 1000;

	args.syncpts = (unsigned long)&syncpt;
	args.cmdbufs = (unsigned long)cmdbufs;
	args.relocs = (unsigned long)relocs;
	args.waitchks = 0;

	err = ioctl(channel->drm->fd, DRM_IOCTL_TEGRA_SUBMIT, &args);
	if (err < 0) {
		fprintf(stderr, "ioctl(DRM_IOCTL_TEGRA_SUBMIT) failed: %d\n",
			errno);
		return -errno;
	}

	channel->fence = args.fence;

	return 0;
	return 0;
}

static int drm_channel_flush(struct host1x_client *client, uint32_t *fence)
{
	struct drm_channel *channel = to_drm_channel(client);

	*fence = channel->fence;

	return 0;
}

static int drm_channel_wait(struct host1x_client *client, uint32_t fence,
			    uint32_t timeout)
{
	struct drm_channel *channel = to_drm_channel(client);
	struct drm_tegra_syncpt_wait args;
	struct host1x_syncpt *syncpt;
	int err;

	syncpt = &channel->client.syncpts[0];

	memset(&args, 0, sizeof(args));
	args.id = syncpt->id;
	args.thresh = fence;
	args.timeout = timeout;

	err = ioctl(channel->drm->fd, DRM_IOCTL_TEGRA_SYNCPT_WAIT, &args);
	if (err < 0) {
		fprintf(stderr, "ioctl(DRM_IOCTL_TEGRA_SYNCPT_WAIT) failed: %d\n",
			errno);
		return -errno;
	}

	return 0;
}

static int drm_channel_init(struct drm *drm, struct drm_channel *channel,
			    uint32_t class, unsigned int num_syncpts)
{
	struct host1x_syncpt *syncpts;
	unsigned int i;
	int err;

	err = drm_channel_open(drm, class, &channel->context);
	if (err < 0)
		return err;

	channel->drm = drm;

	syncpts = calloc(num_syncpts, sizeof(*syncpts));
	if (!syncpts)
		return -ENOMEM;

	channel->client.num_syncpts = num_syncpts;
	channel->client.syncpts = syncpts;

	for (i = 0; i < num_syncpts; i++) {
		struct drm_tegra_get_syncpt args;

		memset(&args, 0, sizeof(args));
		args.context = channel->context;
		args.index = i;

		err = ioctl(drm->fd, DRM_IOCTL_TEGRA_GET_SYNCPT, &args);
		if (err < 0) {
			syncpts[i].id = 0;
			continue;
		}

		syncpts[i].id = args.id;
		syncpts[i].value = 0;
	}

	channel->client.submit = drm_channel_submit;
	channel->client.flush = drm_channel_flush;
	channel->client.wait = drm_channel_wait;

	return 0;
}

static void drm_channel_exit(struct drm_channel *channel)
{
	struct drm_tegra_close_channel args;
	int err;

	memset(&args, 0, sizeof(args));
	args.context = channel->context;

	err = ioctl(channel->drm->fd, DRM_IOCTL_TEGRA_CLOSE_CHANNEL, &args);
	if (err < 0)
		fprintf(stderr, "ioctl(DRM_IOCTL_TEGRA_CLOSE_CHANNEL) failed: %d\n",
			-errno);

	free(channel->client.syncpts);
}

static int drm_gr2d_create(struct drm_gr2d **gr2dp, struct drm *drm)
{
	struct drm_gr2d *gr2d;
	int err;

	gr2d = calloc(1, sizeof(*gr2d));
	if (!gr2d)
		return -ENOMEM;

	err = drm_channel_init(drm, &gr2d->channel, HOST1X_CLASS_GR2D, 1);
	if (err < 0) {
		free(gr2d);
		return err;
	}

	gr2d->base.client = &gr2d->channel.client;

	err = host1x_gr2d_init(&drm->base, &gr2d->base);
	if (err < 0) {
		free(gr2d);
		return err;
	}

	*gr2dp = gr2d;

	return 0;
}

static void drm_gr2d_close(struct drm_gr2d *gr2d)
{
	if (gr2d) {
		drm_channel_exit(&gr2d->channel);
		host1x_gr2d_exit(&gr2d->base);
	}

	free(gr2d);
}

static int drm_gr3d_create(struct drm_gr3d **gr3dp, struct drm *drm)
{
	struct drm_gr3d *gr3d;
	int err;

	gr3d = calloc(1, sizeof(*gr3d));
	if (!gr3d)
		return -ENOMEM;

	err = drm_channel_init(drm, &gr3d->channel, HOST1X_CLASS_GR3D, 1);
	if (err < 0) {
		free(gr3d);
		return err;
	}

	gr3d->base.client = &gr3d->channel.client;

	err = host1x_gr3d_init(&drm->base, &gr3d->base);
	if (err < 0) {
		free(gr3d);
		return err;
	}

	*gr3dp = gr3d;

	return 0;
}

static void drm_gr3d_close(struct drm_gr3d *gr3d)
{
	if (gr3d) {
		drm_channel_exit(&gr3d->channel);
		host1x_gr3d_exit(&gr3d->base);
	}

	free(gr3d);
}

static void drm_close(struct host1x *host1x)
{
	struct drm *drm = to_drm(host1x);

	drm_gr3d_close(drm->gr3d);
	drm_gr2d_close(drm->gr2d);
	drm_display_close(drm->display);

	close(drm->fd);
	free(drm);
}

struct host1x *host1x_drm_open(void)
{
	struct drm *drm;
	int fd, err;

	fd = open("/dev/dri/card0", O_RDWR);
	if (fd < 0)
		return NULL;

	drm = calloc(1, sizeof(*drm));
	if (!drm) {
		close(fd);
		return NULL;
	}

	drm->fd = fd;

	drm->base.bo_create = drm_bo_create;
	drm->base.framebuffer_init = drm_framebuffer_init;
	drm->base.close = drm_close;

	err = drm_display_create(&drm->display, drm);
	if (err < 0 && err != -ENODEV) {
		fprintf(stderr, "drm_display_create() failed: %d\n", err);
		free(drm);
		close(fd);
		return NULL;
	}

	err = drm_gr2d_create(&drm->gr2d, drm);
	if (err < 0) {
		fprintf(stderr, "drm_gr2d_create() failed: %d\n", err);
		free(drm);
		close(fd);
		return NULL;
	}

	err = drm_gr3d_create(&drm->gr3d, drm);
	if (err < 0) {
		fprintf(stderr, "drm_gr3d_create() failed: %d\n", err);
		free(drm);
		close(fd);
		return NULL;
	}

	drm->base.display = &drm->display->base;
	drm->base.gr2d = &drm->gr2d->base;
	drm->base.gr3d = &drm->gr3d->base;

	return &drm->base;
}
