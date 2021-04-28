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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include <libdrm/drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "host1x-private.h"
#include "tegra_drm.h"
#include "x11-display.h"

#ifndef DRM_PLANE_TYPE_OVERLAY
#define DRM_PLANE_TYPE_OVERLAY			0
#endif
#ifndef DRM_PLANE_TYPE_PRIMARY
#define DRM_PLANE_TYPE_PRIMARY			1
#endif
#ifndef DRM_CLIENT_CAP_UNIVERSAL_PLANES
#define DRM_CLIENT_CAP_UNIVERSAL_PLANES		2
#endif
#ifndef DRM_CLIENT_CAP_ATOMIC
#define DRM_CLIENT_CAP_ATOMIC			3
#endif

struct drm;

struct drm_bo {
	struct host1x_bo base;
	struct drm *drm;

	struct {
		uint32_t mapping_id;
		uint32_t channel_ctx;
	} mappings[2];
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
	uint32_t plane;
	uint32_t crtc;
	int reflected;
	bool upside_down;
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
	int reflected;
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
	bool new_uapi;
};

static struct drm *to_drm(struct host1x *host1x)
{
	return container_of(host1x, struct drm, base);
}

static int drm_plane_type(struct drm *drm, drmModePlane *p)
{
	drmModeObjectPropertiesPtr props;
	drmModePropertyPtr prop;
	int type = -EINVAL;
	unsigned int i;

	props = drmModeObjectGetProperties(drm->fd, p->plane_id,
					   DRM_MODE_OBJECT_PLANE);
	if (!props)
		return -ENODEV;

	for (i = 0; i < props->count_props && type == -EINVAL; i++) {
		prop = drmModeGetProperty(drm->fd, props->props[i]);
		if (prop) {
			if (!strcmp(prop->name, "type"))
				type = props->prop_values[i];

			drmModeFreeProperty(prop);
		}
	}

	drmModeFreeObjectProperties(props);

	return type;
}

static int drm_display_find_plane(struct drm_display *display,
				  uint32_t *plane, uint32_t type)
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
			continue;
		}

		if ((p->possible_crtcs & (1u << display->pipe)) &&
		    (drm_plane_type(drm, p) == type))
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

static int drm_overlay_reflect(struct drm_display *display, uint32_t plane_id,
			       bool reflect_y)
{
#ifdef DRM_MODE_REFLECT_Y
	struct drm *drm = display->drm;
	drmModeObjectPropertiesPtr properties;
	drmModePropertyPtr property;
	drmModeAtomicReqPtr req;
	unsigned int rotate_display;
	unsigned int reflect_flag;
	unsigned int i;
	int ret;

	req = drmModeAtomicAlloc();
	if (!req) {
		host1x_error("drmModeAtomicAlloc() failed\n");
		return -ENOMEM;
	}

	properties = drmModeObjectGetProperties(drm->fd, plane_id,
						DRM_MODE_OBJECT_PLANE);
	if (!properties) {
		host1x_error("drmModeObjectGetProperties() failed\n");
		ret = -EINVAL;
		goto atomic_free;
	}

	rotate_display = drm->base.options->rotate_display;

	if (rotate_display != 0 && rotate_display != 180) {
		host1x_error("unsupported display rotation %u, only 0 and 180 are supported\n",
			     rotate_display);

		rotate_display = 0;
	}

	if (display->upside_down)
		rotate_display = (rotate_display == 0) ? 180 : 0;

	for (i = 0, ret = -100; i < properties->count_props; i++) {
		property = drmModeGetProperty(drm->fd, properties->props[i]);
		if (!property)
			continue;

		if (reflect_y)
			reflect_flag = DRM_MODE_REFLECT_Y;
		else
			reflect_flag = 0;

		if (!strcmp(property->name, "rotation")) {
			if (rotate_display == 180)
				reflect_flag |= DRM_MODE_ROTATE_180;
			else
				reflect_flag |= DRM_MODE_ROTATE_0;

			ret = drmModeAtomicAddProperty(req, plane_id,
						       property->prop_id,
						       reflect_flag);
			if (ret < 0)
				host1x_error("drmModeAtomicAddProperty() failed: %d\n",
					     ret);
		}

		free(property);
	}

	free(properties);

	if (ret >= 0) {
		ret = drmModeAtomicCommit(drm->fd, req, 0, NULL);
		if (ret < 0)
			host1x_error("drmModeAtomicCommit() failed: %d\n", ret);
	}

atomic_free:
	drmModeAtomicFree(req);

	if (ret == -100)
		host1x_error("couldn't get DRM plane \"rotation\" property\n");

	return ret;
#else
	return 0;
#endif
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
			   unsigned int height, bool vsync, bool reflect_y)
{
	struct drm_overlay *plane = to_drm_overlay(overlay);
	struct drm_display *display = plane->display;
	struct drm *drm = display->drm;
	int err;

	if (plane->reflected != reflect_y) {
		drm_overlay_reflect(display, plane->plane, reflect_y);
		plane->reflected = reflect_y;
	}

	if (vsync) {
		drmVBlank vblank = {
			.request = {
				.type = DRM_VBLANK_RELATIVE,
				.sequence = 1,
			},
		};

		vblank.request.type |=
				display->pipe << DRM_VBLANK_HIGH_CRTC_SHIFT;

		err = drmWaitVBlank(drm->fd, &vblank);
		if (err < 0) {
			host1x_error("drmWaitVBlank() failed: %m\n");
			return -errno;
		}
	}

	if (display->upside_down) {
		x = display->mode.hdisplay - width - x;
		y = display->mode.vdisplay - height - y;
	}

	err = drmModeSetPlane(drm->fd, plane->plane, display->crtc,
			      fb->handle, 0, x, y, width, height, 0, 0,
			      fb->pixbuf->width << 16,
			      fb->pixbuf->height << 16);
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

	err = drm_display_find_plane(drm, &plane, DRM_PLANE_TYPE_OVERLAY);
	if (err < 0)
		return err;

	overlay = calloc(1, sizeof(*overlay));
	if (!overlay)
		return -ENOMEM;

	overlay->base.close = drm_overlay_close;
	overlay->base.set = drm_overlay_set;

	overlay->display = drm;
	overlay->plane = plane;
	overlay->reflected = -1;

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
			   struct host1x_framebuffer *fb,
			   bool vsync, bool reflect_y)
{
	struct drm_display *drm = to_drm_display(display);
	int err;

	if (drm->reflected != reflect_y) {
		drm_overlay_reflect(drm, drm->plane, reflect_y);
		drm->reflected = reflect_y;
	}

	if (vsync) {
		struct timeval timeout;
		fd_set fds;

		err = drmModePageFlip(drm->drm->fd, drm->crtc, fb->handle,
				      DRM_MODE_PAGE_FLIP_EVENT, drm);
		if (err < 0) {
			err = drmModeSetCrtc(drm->drm->fd, drm->crtc,
					     fb->handle, 0, 0, &drm->connector,
					     1, &drm->mode);
		}

		if (err < 0) {
			host1x_error("drmModePageFlip() failed: %m\n");
			return -errno;
		}

		memset(&timeout, 0, sizeof(timeout));
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		FD_ZERO(&fds);
		FD_SET((unsigned)drm->drm->fd, &fds);

		err = select(drm->drm->fd + 1, &fds, NULL, NULL, &timeout);
		if (err <= 0) {
		}

		if (FD_ISSET((unsigned)drm->drm->fd, &fds)) {
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

static int drm_display_setup(struct drm_display *display, int display_id)
{
	struct drm *drm = display->drm;
	int ret = -ENODEV;
	drmModeRes *res;
	uint32_t i, k;

	res = drmModeGetResources(drm->fd);
	if (!res)
		return -ENODEV;

retry_connector:
	for (i = 0; i < res->count_connectors; i++) {
		drmModePropertyPtr property;
		drmModeConnector *connector;
		drmModeEncoder *encoder;

		connector = drmModeGetConnector(drm->fd, res->connectors[i]);
		if (!connector)
			continue;

		if (connector->connection != DRM_MODE_CONNECTED ||
		    (display_id > -1 && i != display_id))
		{
			drmModeFreeConnector(connector);

			if (i == display_id) {
				host1x_info("selected display is unconnected, skipping\n");
				display_id = -1;
				goto retry_connector;
			}
			continue;
		}

		encoder = drmModeGetEncoder(drm->fd, connector->encoder_id);
		if (!encoder) {
			drmModeFreeConnector(connector);
			continue;
		}

		for (k = 0; k < connector->count_props; k++) {
			property = drmModeGetProperty(drm->fd, connector->props[k]);
			if (!property)
				continue;

			if (!strcmp(property->name, "panel orientation")) {
				unsigned int orientation_id = connector->prop_values[k];
				const char *orientation_name = property->enums[orientation_id].name;

				if (!strcmp(orientation_name, "Upside Down"))
					display->upside_down = true;
			}

			free(property);
		}

		display->connector = res->connectors[i];
		display->mode = connector->modes[0];
		display->crtc = encoder->crtc_id;
		display->reflected = -1;

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

	if (ret == 0)
		ret = drm_display_find_plane(display, &display->plane,
					     DRM_PLANE_TYPE_PRIMARY);
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
	if (err < 0)
		goto try_x11;

#ifdef HAVE_LIBDRM_CLIENT_CAP
	err = drmSetClientCap(drm->fd, DRM_CLIENT_CAP_ATOMIC, 1);
	if (err)
		host1x_error("drmSetClientCap(ATOMIC) failed: %d\n", err);

	err = drmSetClientCap(drm->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	if (err)
		host1x_error("drmSetClientCap(UNIVERSAL_PLANES) failed: %d\n",
			     err);
#endif

	err = drm_display_setup(display, drm->base.options->display_id);
	if (err < 0)
		goto try_x11;

	display->base.width = display->mode.hdisplay;
	display->base.height = display->mode.vdisplay;
	display->base.create_overlay = drm_overlay_create;
	display->base.set = drm_display_set;

	*displayp = display;

	return 0;
try_x11:
	err = x11_display_create(&drm->base, &display->base, drm->fd);
	if (err < 0) {
		free(display);
		return err;
	}

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
	struct host1x_bo *orig;
	void *ptr;
	int err;

	orig = bo->wrapped ?: bo;

	if (orig->ptr) {
		bo->ptr = orig->ptr;
		return 0;
	}

	memset(&args, 0, sizeof(args));
	args.handle = bo->handle;

	err = ioctl(drm->drm->fd, DRM_IOCTL_TEGRA_GEM_MMAP, &args);
	if (err < 0)
		return -errno;

	ptr = mmap(NULL, orig->size, PROT_READ | PROT_WRITE, MAP_SHARED,
		   drm->drm->fd, (__off_t)args.offset);
	if (ptr == MAP_FAILED)
		return -errno;

	orig->ptr = ptr;
	bo->ptr = ptr;

	return 0;
}

static int drm_bo_invalidate(struct host1x_bo *bo, unsigned long offset,
			     size_t length)
{
	return 0;
}

static int drm_bo_flush(struct host1x_bo *bo, unsigned long offset,
			size_t length)
{
	return 0;
}

static void drm_bo_free(struct host1x_bo *bo)
{
	struct drm_bo *drm_bo = to_drm_bo(bo);
	struct drm_gem_close args;
	int err;

	if (bo->wrapped)
		return free(drm_bo);

	if (drm_bo->drm->new_uapi) {
		struct drm_tegra_channel_unmap unmap_args;
		unsigned int i;

		for (i = 0; i < 2; i++) {
			if (drm_bo->mappings[i].mapping_id == 0)
				break;

			memset(&unmap_args, 0, sizeof(unmap_args));
			unmap_args.channel_ctx =
				drm_bo->mappings[i].channel_ctx;
			unmap_args.mapping_id = drm_bo->mappings[i].mapping_id;

			ioctl(drm_bo->drm->fd, DRM_IOCTL_TEGRA_CHANNEL_UNMAP,
			      &unmap_args);
		}
	}

	memset(&args, 0, sizeof(args));
	args.handle = bo->handle;

	err = ioctl(drm_bo->drm->fd, DRM_IOCTL_GEM_CLOSE, &args);
	if (err < 0)
		host1x_error("failed to delete buffer object: %m\n");

	free(drm_bo);
}

static struct host1x_bo *drm_bo_clone(struct host1x_bo *bo)
{
	struct drm_bo *dbo = to_drm_bo(bo);
	struct drm_bo *clone = malloc(sizeof(*dbo));

	if (!clone)
		return NULL;

	memcpy(clone, dbo, sizeof(*dbo));

	return &clone->base;
}

static int drm_bo_export(struct host1x_bo *bo, uint32_t *handle)
{
	struct drm_bo *dbo = to_drm_bo(bo);
	struct drm_gem_flink args;
	int err;

	memset(&args, 0, sizeof(args));
	args.handle = bo->handle;

	err = drmIoctl(dbo->drm->fd, DRM_IOCTL_GEM_FLINK, &args);
	if (err < 0)
		return -errno;

	*handle = args.name;

	return 0;
}

static struct host1x_bo *drm_bo_create(struct host1x *host1x,
				       struct host1x_bo_priv *priv,
				       size_t size, unsigned long flags)
{
	struct drm_tegra_gem_create args;
	struct drm *drm = to_drm(host1x);
	struct drm_bo *bo;
	int err;

	bo = calloc(1, sizeof(*bo));
	if (!bo)
		return NULL;

	bo->drm = drm;
	bo->base.priv = priv;

	memset(&args, 0, sizeof(args));
	args.size = size;

	if (!drm->new_uapi) {
		if (flags & HOST1X_BO_CREATE_FLAG_BOTTOM_UP)
			args.flags |= DRM_TEGRA_GEM_CREATE_BOTTOM_UP;

		if (flags & HOST1X_BO_CREATE_FLAG_TILED)
			args.flags |= DRM_TEGRA_GEM_CREATE_TILED;
	}

	err = ioctl(drm->fd, DRM_IOCTL_TEGRA_GEM_CREATE, &args);
	if (err < 0) {
		free(bo);
		return NULL;
	}

	bo->base.handle = args.handle;

	bo->base.priv->mmap = drm_bo_mmap;
	bo->base.priv->invalidate = drm_bo_invalidate;
	bo->base.priv->flush = drm_bo_flush;
	bo->base.priv->free = drm_bo_free;
	bo->base.priv->clone = drm_bo_clone;
	bo->base.priv->export = drm_bo_export;

	return &bo->base;
}

static struct host1x_bo *drm_bo_import(struct host1x *host1x,
				       struct host1x_bo_priv *priv,
				       uint32_t handle)
{
	struct drm_gem_open args;
	struct drm *drm = to_drm(host1x);
	struct drm_bo *bo;
	int err;

	bo = calloc(1, sizeof(*bo));
	if (!bo)
		return NULL;

	bo->drm = drm;
	bo->base.priv = priv;

	memset(&args, 0, sizeof(args));
	args.name = handle;

	err = ioctl(drm->fd, DRM_IOCTL_GEM_OPEN, &args);
	if (err < 0) {
		free(bo);
		return NULL;
	}

	bo->base.handle = args.handle;

	bo->base.priv->mmap = drm_bo_mmap;
	bo->base.priv->invalidate = drm_bo_invalidate;
	bo->base.priv->flush = drm_bo_flush;
	bo->base.priv->free = drm_bo_free;
	bo->base.priv->clone = drm_bo_clone;
	bo->base.priv->export = drm_bo_export;

	return &bo->base;
}

static int drm_bo_map(struct host1x_bo *host1x_bo, struct drm_channel *channel,
		      uint32_t *mapping_id)
{
	struct drm_tegra_channel_map args;
	struct drm_bo *bo = to_drm_bo(host1x_bo);
	int i, free_i = -1, err;

	if (!bo->drm->new_uapi)
		return 0;

	for (i = 0; i < 2; i++) {
		if (bo->mappings[i].channel_ctx == channel->context) {
			*mapping_id = bo->mappings[i].mapping_id;
			return 0;
		}

		if (bo->mappings[i].mapping_id == 0)
			free_i = i;
	}

	if (free_i == -1)
		return -EBUSY;

	memset(&args, 0, sizeof(args));
	args.channel_ctx = channel->context;
	args.handle = bo->base.handle;

	err = ioctl(channel->drm->fd, DRM_IOCTL_TEGRA_CHANNEL_MAP, &args);
	if (err < 0) {
		host1x_error("ioctl(DRM_IOCTL_TEGRA_CHANNEL_MAP) failed: %d\n",
			     errno);
		return -errno;
	}

	bo->mappings[free_i].channel_ctx = channel->context;
	bo->mappings[free_i].mapping_id = args.mapping_id;

	*mapping_id = args.mapping_id;

	return 0;
}

static int drm_framebuffer_init(struct host1x *host1x,
				struct host1x_framebuffer *fb)
{
	uint32_t handles[4], pitches[4], offsets[4], format;
#ifdef DRM_FORMAT_MOD_NVIDIA_TEGRA_TILED
	uint64_t modifiers[4];
#endif
	struct host1x_pixelbuffer *pixbuf = fb->pixbuf;
	struct drm *drm = to_drm(host1x);
	int err = -1;

	/* XXX: support other formats */
	switch (pixbuf->format)
	{
	case PIX_BUF_FMT_RGB565:
		format = DRM_FORMAT_RGB565;
		break;
	case PIX_BUF_FMT_RGBA8888:
		format = DRM_FORMAT_XBGR8888;
		break;
	case PIX_BUF_FMT_BGRA8888:
		format = DRM_FORMAT_XRGB8888;
		break;
	default:
		host1x_error("Unsupported framebuffer format\n");
		return -EINVAL;
	}

	memset(handles, 0, sizeof(handles));
	memset(pitches, 0, sizeof(pitches));
	memset(offsets, 0, sizeof(offsets));

	handles[0] = pixbuf->bo->handle;
	pitches[0] = pixbuf->pitch;
	offsets[0] = pixbuf->bo->offset;

#ifdef DRM_FORMAT_MOD_NVIDIA_TEGRA_TILED
	memset(modifiers, 0, sizeof(modifiers));

	if (pixbuf->layout == PIX_BUF_LAYOUT_TILED_16x16)
		modifiers[0] = DRM_FORMAT_MOD_NVIDIA_TEGRA_TILED;
	else
		modifiers[0] = DRM_FORMAT_MOD_LINEAR;

	err = drmModeAddFB2WithModifiers(drm->fd, pixbuf->width, pixbuf->height,
					 format, handles, pitches, offsets,
					 modifiers, &fb->handle,
					 DRM_MODE_FB_MODIFIERS);
	if (!err)
		return 0;
#endif
	err = drmModeAddFB2(drm->fd, pixbuf->width, pixbuf->height, format,
			    handles, pitches, offsets, &fb->handle, 0);
	if (err < 0)
		return -errno;

	return 0;
}

static int drm_channel_open(struct drm *drm, uint32_t class, uint64_t *channel)
{
	int err;

	if (drm->new_uapi) {
		struct drm_tegra_channel_open args;

		memset(&args, 0, sizeof(args));
		args.host1x_class = class;

		err = ioctl(drm->fd, DRM_IOCTL_TEGRA_CHANNEL_OPEN, &args);
		if (err < 0)
			return -errno;

		*channel = args.channel_ctx;
	} else {
		struct drm_tegra_open_channel args;

		memset(&args, 0, sizeof(args));
		args.client = class;

		err = ioctl(drm->fd, DRM_IOCTL_TEGRA_OPEN_CHANNEL, &args);
		if (err < 0)
			return -errno;

		*channel = args.context;
	}

	return 0;
}

static int drm_channel_submit2(struct host1x_client *client,
			       struct host1x_job *job)
{
	struct drm_channel *channel = to_drm_channel(client);
	struct drm_tegra_channel_submit args;
	struct drm_tegra_submit_buf *bufs, *next_buf;
	struct drm_tegra_submit_cmd *cmds;
	uint32_t next_offset = 0, first_offset;
	unsigned int i, num_relocs = 0;
	int err;

	memset(&args, 0, sizeof(args));
	args.channel_ctx = channel->context;
	args.syncpt_incr.id = job->syncpt;
	args.syncpt_incr.num_incrs = job->syncpt_incrs;

	cmds = calloc(job->num_pushbufs, sizeof(*bufs));
	if (!cmds)
		return -ENOMEM;

	args.cmds_ptr = (__u64)(unsigned long)cmds;
	args.num_cmds = job->num_pushbufs;

	for (i = 0; i < job->num_pushbufs; i++) {
		struct host1x_pushbuf *pushbuf = &job->pushbufs[i];
		struct drm_tegra_submit_cmd *cmd = &cmds[i];

		if (pushbuf->offset != next_offset) {
			free(cmds);
			return -EINVAL;
		}

		if (next_offset == 0) {
			args.gather_data_ptr = (__u64)(unsigned long)(pushbuf->bo->ptr + pushbuf->offset);
			args.gather_data_words += pushbuf->length;

			first_offset = pushbuf->offset;
		}

		cmd->type = DRM_TEGRA_SUBMIT_CMD_GATHER_UPTR;
		cmd->gather_uptr.words = pushbuf->length;

		next_offset += pushbuf->length;
		num_relocs += pushbuf->num_relocs;
	}

	bufs = calloc(num_relocs, sizeof(*bufs));
	if (!bufs) {
		free(cmds);
		return -ENOMEM;
	}

	args.bufs_ptr = (__u64)(unsigned long)bufs;
	args.num_bufs = num_relocs;

	next_buf = bufs;

	for (i = 0; i < job->num_pushbufs; i++) {
		struct host1x_pushbuf *pushbuf = &job->pushbufs[i];
		unsigned int j;

		for (j = 0; j < pushbuf->num_relocs; j++) {
			struct host1x_pushbuf_reloc *r = &pushbuf->relocs[j];

			err = drm_bo_map(r->target_bo, channel,
					 &next_buf->mapping_id);
			if (err < 0) {
				free(bufs);
				free(cmds);
				return err;
			}

			next_buf->reloc.gather_offset_words = r->source_offset/4 - first_offset;
			next_buf->reloc.target_offset = r->target_offset;
			next_buf->reloc.shift = r->shift;

			next_buf++;
		}
	}

	err = ioctl(channel->drm->fd, DRM_IOCTL_TEGRA_CHANNEL_SUBMIT, &args);
	if (err < 0) {
		host1x_error("ioctl(DRM_IOCTL_TEGRA_CHANNEL_SUBMIT) failed: %d\n",
			     errno);
		err = -errno;
	} else {
		channel->fence = args.syncpt_incr.fence_value;
		err = 0;
	}

	free(bufs);
	free(cmds);

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
	if (!relocs) {
		free(cmdbufs);
		return -ENOMEM;
	}

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
		host1x_error("ioctl(DRM_IOCTL_TEGRA_SUBMIT) failed: %d\n",
			     errno);
		err = -errno;
	} else {
		channel->fence = args.fence;
		err = 0;
	}

	free(relocs);
	free(cmdbufs);

	return err;
}

static int drm_channel_flush(struct host1x_client *client, uint32_t *fence)
{
	struct drm_channel *channel = to_drm_channel(client);

	*fence = channel->fence;

	return 0;
}

static uint64_t gettime_ns(void)
{
    struct timespec current;
    clock_gettime(CLOCK_MONOTONIC, &current);
    return (uint64_t)current.tv_sec * 1000000000ull + current.tv_nsec;
}

static int drm_channel_wait(struct host1x_client *client, uint32_t fence,
			    uint32_t timeout)
{
	struct drm_channel *channel = to_drm_channel(client);
	struct host1x_syncpt *syncpt;
	int err;

	syncpt = &channel->client.syncpts[0];

	if (channel->drm->new_uapi) {
		struct drm_tegra_syncpoint_wait args;

		memset(&args, 0, sizeof(args));
		args.id = syncpt->id;
		args.threshold = fence;
		args.timeout_ns = gettime_ns() + timeout * 1000000ull;

		err = ioctl(channel->drm->fd, DRM_IOCTL_TEGRA_SYNCPOINT_WAIT, &args);
		if (err < 0) {
			host1x_error("ioctl(DRM_IOCTL_TEGRA_SYNCPOINT_WAIT) failed: %d\n",
				errno);
			return -errno;
		}
	} else {
		struct drm_tegra_syncpt_wait args;

		memset(&args, 0, sizeof(args));
		args.id = syncpt->id;
		args.thresh = fence;
		args.timeout = timeout;

		err = ioctl(channel->drm->fd, DRM_IOCTL_TEGRA_SYNCPT_WAIT, &args);
		if (err < 0) {
			host1x_error("ioctl(DRM_IOCTL_TEGRA_SYNCPT_WAIT) failed: %d\n",
				errno);
			return -errno;
		}
	}

	return 0;
}

static int drm_channel_allocate_syncpt(struct drm* drm, struct drm_channel* channel, unsigned int i)
{
	struct host1x_syncpt *syncpt = &channel->client.syncpts[i];
	int err;

	if (drm->new_uapi) {
		struct drm_tegra_syncpoint_allocate args;

		memset(&args, 0, sizeof(args));

		err = ioctl(drm->fd, DRM_IOCTL_TEGRA_SYNCPOINT_ALLOCATE, &args);
		if (err < 0)
			return -errno;

		syncpt->id = args.id;
		syncpt->value = 0;
	} else {
		struct drm_tegra_get_syncpt args;

		memset(&args, 0, sizeof(args));
		args.context = channel->context;
		args.index = i;

		err = ioctl(drm->fd, DRM_IOCTL_TEGRA_GET_SYNCPT, &args);
		if (err < 0)
			return -errno;

		syncpt->id = args.id;
		syncpt->value = 0;
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
		err = drm_channel_allocate_syncpt(drm, channel, i);
		if (err < 0)
			channel->client.syncpts[i].id = 0;
	}

	if (drm->new_uapi)
		channel->client.submit = drm_channel_submit2;
	else
		channel->client.submit = drm_channel_submit;

	channel->client.flush = drm_channel_flush;
	channel->client.wait = drm_channel_wait;

	return 0;
}

static void drm_channel_exit(struct drm_channel *channel)
{
	int err;

	if (channel->drm->new_uapi) {
		struct drm_tegra_channel_close args;

		memset(&args, 0, sizeof(args));
		args.channel_ctx = channel->context;

		err =  ioctl(channel->drm->fd, DRM_IOCTL_TEGRA_CHANNEL_CLOSE, &args);
		if (err < 0)
			host1x_error("ioctl(DRM_IOCTL_TEGRA_CHANNEL_CLOSE) failed: %d\n",
				     -errno);
	} else {
		struct drm_tegra_close_channel args;

		memset(&args, 0, sizeof(args));
		args.context = channel->context;

		err = ioctl(channel->drm->fd, DRM_IOCTL_TEGRA_CLOSE_CHANNEL, &args);
		if (err < 0)
			host1x_error("ioctl(DRM_IOCTL_TEGRA_CLOSE_CHANNEL) failed: %d\n",
				-errno);
	}

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

static enum tegra_soc_id host1x_drm_get_soc_id(void)
{
	const char *path = "/sys/devices/soc0/soc_id";
	FILE *file;

	file = fopen(path, "r");
	if (file) {
		unsigned int id = 0;

		if (fscanf(file, "%d", &id) != 1)
			host1x_error("fscanf failed for %s\n", path);
		fclose(file);

		switch (id) {
		case 0x20:
			return TEGRA20_SOC;
		case 0x30:
			return TEGRA30_SOC;
		case 0x35:
			return TEGRA114_SOC;
		}
	} else {
		host1x_error("failed to open %s\n", path);
	}

	return TEGRA_UNKOWN_SOC;
}

struct host1x *host1x_drm_open(struct host1x_options *options)
{
	struct drm *drm;
	int fd = options->fd;
	drmVersionPtr ver;
	int err;

	if (fd < 0) {
		fd = open("/dev/dri/card0", O_RDWR);
		if (fd < 0)
			return NULL;
	}

	ver = drmGetVersion(fd);
	if (!ver) {
		close(fd);
		return NULL;
	}

	drm = calloc(1, sizeof(*drm));
	if (!drm) {
		drmFreeVersion(ver);
		close(fd);
		return NULL;
	}

	if (ver->version_major == 1 &&
	    !getenv("GRATE_FORCE_OLD_UAPI"))
		drm->new_uapi = true;

	drmFreeVersion(ver);

	if (drm->new_uapi)
		printf("Using new UAPI.\n");
	else
		printf("Using old UAPI.\n");

	drm->fd = fd;

	drm->base.bo_create = drm_bo_create;
	drm->base.framebuffer_init = drm_framebuffer_init;
	drm->base.close = drm_close;
	drm->base.bo_import = drm_bo_import;
	drm->base.options = options;

	err = drm_gr2d_create(&drm->gr2d, drm);
	if (err < 0) {
		host1x_error("drm_gr2d_create() failed: %d\n", err);
		free(drm);
		close(fd);
		return NULL;
	}

	err = drm_gr3d_create(&drm->gr3d, drm);
	if (err < 0) {
		host1x_error("drm_gr3d_create() failed: %d\n", err);
		free(drm);
		close(fd);
		return NULL;
	}

	drm->base.gr2d = &drm->gr2d->base;
	drm->base.gr3d = &drm->gr3d->base;

	options->chip_info.soc_id = host1x_drm_get_soc_id();

	return &drm->base;
}

void host1x_drm_display_init(struct host1x *host1x)
{
	struct drm *drm = to_drm(host1x);
	int err;

	err = drm_display_create(&drm->display, drm);
	if (err < 0) {
		host1x_error("drm_display_create() failed: %d\n", err);
	} else {
		drm->base.display = &drm->display->base;
	}
}
