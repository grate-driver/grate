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

#include <string.h>

#include "host1x-private.h"
#include "nvhost.h"

static int nvhost_bo_mmap(struct host1x_bo *bo)
{
	struct nvhost_bo *nbo = to_nvhost_bo(bo);
	int err;

	err = nvmap_handle_mmap(nbo->nvmap, nbo->handle);
	if (err < 0)
		return err;

	bo->ptr = nbo->handle->ptr;

	return 0;
}

static int nvhost_bo_invalidate(struct host1x_bo *bo, unsigned long offset,
				size_t length)
{
	struct nvhost_bo *nbo = to_nvhost_bo(bo);

	return nvmap_handle_invalidate(nbo->nvmap, nbo->handle, offset,
				       length);
}

static int nvhost_bo_flush(struct host1x_bo *bo, unsigned long offset,
			   size_t length)
{
	struct nvhost_bo *nbo = to_nvhost_bo(bo);

	return nvmap_handle_writeback_invalidate(nbo->nvmap, nbo->handle,
						 offset, length);
}

static void nvhost_bo_free(struct host1x_bo *bo)
{
	struct nvhost_bo *nbo = to_nvhost_bo(bo);

	nvmap_handle_free(nbo->nvmap, nbo->handle);

	free(nbo);
}

static struct host1x_bo *nvhost_bo_create(struct host1x *host1x,
					  struct host1x_bo_priv *priv,
					  size_t size, unsigned long flags)
{
	struct nvhost *nvhost = to_nvhost(host1x);
	unsigned long heap_mask, align;
	struct nvhost_bo *bo;
	int err;

	bo = calloc(1, sizeof(*bo));
	if (!bo)
		return NULL;

	bo->nvmap = nvhost->nvmap;
	bo->base.priv = priv;

	bo->handle = nvmap_handle_create(nvhost->nvmap, size);
	if (!bo->handle) {
		free(bo);
		return NULL;
	}

	switch (flags & ~HOST1X_BO_CREATE_DRM_FLAGS_MASK) {
	case NVHOST_BO_FLAG_FRAMEBUFFER:
		heap_mask = NVMAP_HEAP_CARVEOUT_GENERIC;
		flags = NVMAP_HANDLE_WRITE_COMBINE;
		align = 0x100;
		break;

	case NVHOST_BO_FLAG_COMMAND_BUFFER:
		heap_mask = NVMAP_HEAP_CARVEOUT_GENERIC;
		flags = NVMAP_HANDLE_WRITE_COMBINE;
		align = 0x20;
		break;

	case NVHOST_BO_FLAG_SCRATCH:
		heap_mask = NVMAP_HEAP_IOVMM;
		flags = NVMAP_HANDLE_WRITE_COMBINE;
		align = 0x20;
		break;

	case NVHOST_BO_FLAG_ATTRIBUTES:
		heap_mask = NVMAP_HEAP_IOVMM;
		flags = NVMAP_HANDLE_WRITE_COMBINE;
		align = 0x4;
		break;

	default:
		heap_mask = NVMAP_HEAP_IOVMM;
		flags = NVMAP_HANDLE_WRITE_COMBINE;
		align = 0x4;
		break;
	}

	/* XXX what to use for flags and heap_mask? */
	err = nvmap_handle_alloc(nvhost->nvmap, bo->handle, heap_mask, flags,
				 align);
	if (err < 0) {
		nvmap_handle_free(nvhost->nvmap, bo->handle);
		free(bo);
		return NULL;
	}

	bo->base.handle = bo->handle->id;
	bo->base.size = bo->handle->size;

	bo->base.priv->mmap = nvhost_bo_mmap;
	bo->base.priv->invalidate = nvhost_bo_invalidate;
	bo->base.priv->flush = nvhost_bo_flush;
	bo->base.priv->free = nvhost_bo_free;

	return &bo->base;
}

static void host1x_nvhost_close(struct host1x *host1x)
{
	struct nvhost *nvhost = to_nvhost(host1x);

	nvhost_gr3d_close(nvhost->gr3d);
	nvhost_gr2d_close(nvhost->gr2d);
	nvhost_display_close(nvhost->display);
	nvhost_ctrl_close(nvhost->ctrl);
	nvmap_close(nvhost->nvmap);
}

static int nvhost_framebuffer_init(struct host1x *host1x,
				   struct host1x_framebuffer *fb)
{
	void *mmap;
	int err;

	err = HOST1X_BO_MMAP(fb->pixbuf->bo, &mmap);
	if (err < 0)
		return err;

	memset(mmap, 0, fb->pixbuf->bo->size);

	err = HOST1X_BO_FLUSH(fb->pixbuf->bo,
			      fb->pixbuf->bo->offset,
			      fb->pixbuf->bo->size);
	if (err < 0)
		return err;

	return 0;
}

struct host1x *host1x_nvhost_open(void)
{
	struct nvhost *nvhost;

	nvhost = calloc(1, sizeof(*nvhost));
	if (!nvhost)
		return NULL;

	nvhost->nvmap = nvmap_open();
	if (!nvhost->nvmap)
		return NULL;

	nvhost->ctrl = nvhost_ctrl_open();
	if (!nvhost->ctrl)
		return NULL;

	nvhost->base.bo_create = nvhost_bo_create;
	nvhost->base.close = host1x_nvhost_close;

	nvhost->gr2d = nvhost_gr2d_open(nvhost);
	if (!nvhost->gr2d)
		return NULL;

	nvhost->gr3d = nvhost_gr3d_open(nvhost);
	if (!nvhost->gr3d)
		return NULL;

	nvhost->display = nvhost_display_create(nvhost);
	if (!nvhost->display) {
		host1x_error("nvhost_display_create() failed\n");
	} else {
		nvhost->base.display = &nvhost->display->base;
	}

	nvhost->base.gr2d = &nvhost->gr2d->base;
	nvhost->base.gr3d = &nvhost->gr3d->base;
	nvhost->base.framebuffer_init = nvhost_framebuffer_init;

	return &nvhost->base;
}
