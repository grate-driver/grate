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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "host1x.h"
#include "host1x-private.h"

struct host1x *host1x_open(void)
{
	struct host1x *host1x;

	printf("Looking for Tegra DRM interface...");
	fflush(stdout);

	host1x = host1x_drm_open();
	if (host1x) {
		printf("found\n");
		return host1x;
	}

	printf("not found\n");
	printf("Looking for L4T interface...");
	fflush(stdout);

	host1x = host1x_nvhost_open();
	if (host1x) {
		printf("found\n");
		return host1x;
	}

	printf("not found\n");
	return NULL;
}

void host1x_close(struct host1x *host1x)
{
	host1x->close(host1x);
}

struct host1x_display *host1x_get_display(struct host1x *host1x)
{
	return host1x->display;
}

struct host1x_gr2d *host1x_get_gr2d(struct host1x *host1x)
{
	return host1x->gr2d;
}

struct host1x_gr3d *host1x_get_gr3d(struct host1x *host1x)
{
	return host1x->gr3d;
}

int host1x_display_get_resolution(struct host1x_display *display,
				  unsigned int *width, unsigned int *height)
{
	*width = display->width;
	*height = display->height;

	return 0;
}

int host1x_display_set(struct host1x_display *display,
		       struct host1x_framebuffer *fb, bool vsync)
{
	return display->set(display, fb, vsync);
}

int host1x_overlay_create(struct host1x_overlay **overlayp,
			  struct host1x_display *display)
{
	return display->create_overlay(display, overlayp);
}

int host1x_overlay_close(struct host1x_overlay *overlay)
{
	return overlay->close(overlay);
}

int host1x_overlay_set(struct host1x_overlay *overlay,
		       struct host1x_framebuffer *fb, unsigned int x,
		       unsigned int y, unsigned int width,
		       unsigned int height, bool vsync)
{
	return overlay->set(overlay, fb, x, y, width, height, vsync);
}

struct host1x_bo *host1x_bo_create(struct host1x *host1x, size_t size,
				   unsigned long flags)
{
	struct host1x_bo_priv *priv;
	struct host1x_bo *bo;

	priv = calloc(1, sizeof(*priv));
	if (!priv)
		return NULL;

	bo = host1x->bo_create(host1x, priv, size, flags);
	if (!bo)
		free(priv);

	return bo;
}

void host1x_bo_free(struct host1x_bo *bo)
{
	struct host1x_bo_priv *priv = bo->priv;

	bo->priv->free(bo);
	free(priv);
}

int host1x_bo_mmap(struct host1x_bo *bo, void **ptr)
{
	int err;

	err = bo->priv->mmap(bo);
	if (err < 0)
		return err;

	if (ptr)
		*ptr = bo->ptr;

	return 0;
}

int host1x_bo_invalidate(struct host1x_bo *bo, unsigned long offset,
			 size_t length)
{
	if (bo->priv->invalidate)
		return bo->priv->invalidate(bo, offset, length);

	return 0;
}

int host1x_bo_flush(struct host1x_bo *bo, unsigned long offset,
		    size_t length)
{
	if (bo->priv->flush)
		return bo->priv->flush(bo, offset, length);

	return 0;
}

/*
 * Offset is given relatively to the wrapped BO and not the original,
 * to make nested wrapping work seamlessly. However the actual offset of
 * the wrapped BO is set relatively to the original BO.
 */
struct host1x_bo *host1x_bo_wrap(struct host1x_bo *bo,
				 unsigned long offset, size_t size)
{
	struct host1x_bo_priv *priv;
	struct host1x_bo *wrap;
	struct host1x_bo *orig;

	orig = bo->wrapped ?: bo;

	if (bo->wrapped) {
		if (bo->offset + bo->size + offset + size > orig->size)
			goto chk_err;
	} else {
		if (bo->offset + offset + size > orig->size)
			goto chk_err;
	}

	priv = calloc(1, sizeof(*priv));
	if (!priv)
		return NULL;

	wrap = bo->priv->clone(bo);
	if (wrap) {
		memcpy(priv, bo->priv, sizeof(*priv));
		wrap->offset += (bo->wrapped ? bo->size : 0) + offset;
		wrap->wrapped = orig;
		wrap->size = size;
		wrap->priv = priv;
		wrap->ptr = NULL;
	} else {
		free(priv);
	}

	return wrap;

chk_err:
	host1x_error("To wrap offset %lu size %zu orig size %zu; "
		     "Requested offset %lu size %zu\n",
		     bo->offset, bo->size, orig->size, offset, size);

	return NULL;
}

struct host1x_job *host1x_job_create(uint32_t syncpt, uint32_t increments)
{
	struct host1x_job *job;

	job = calloc(1, sizeof(*job));
	if (!job)
		return NULL;

	job->syncpt = syncpt;
	job->syncpt_incrs = increments;

	return job;
}

void host1x_job_free(struct host1x_job *job)
{
	unsigned int i;

	for (i = 0; i < job->num_pushbufs; i++) {
		struct host1x_pushbuf *pb = &job->pushbufs[i];
		free(pb->relocs);
	}

	free(job->pushbufs);
	free(job);
}

struct host1x_pushbuf *host1x_job_append(struct host1x_job *job,
					 struct host1x_bo *bo,
					 unsigned long offset)
{
	struct host1x_pushbuf *pb;
	size_t size;

	if (!bo->ptr)
		return NULL;

	size = (job->num_pushbufs + 1) * sizeof(*pb);

	pb = realloc(job->pushbufs, size);
	if (!pb)
		return NULL;

	job->pushbufs = pb;

	pb = &job->pushbufs[job->num_pushbufs++];
	memset(pb, 0, sizeof(*pb));

	pb->ptr = bo->ptr + offset;
	pb->offset = offset;
	pb->bo = bo;

	return pb;
}

int host1x_pushbuf_push(struct host1x_pushbuf *pb, uint32_t word)
{
	*pb->ptr++ = word;
	pb->length++;

	return 0;
}

int host1x_pushbuf_relocate(struct host1x_pushbuf *pb, struct host1x_bo *target,
			    unsigned long offset, unsigned long shift)
{
	struct host1x_pushbuf_reloc *reloc;
	size_t size;

	size = (pb->num_relocs + 1) * sizeof(*reloc);

	reloc = realloc(pb->relocs, size);
	if (!reloc)
		return -ENOMEM;

	pb->relocs = reloc;

	reloc = &pb->relocs[pb->num_relocs++];

	reloc->source_offset = host1x_bo_get_offset(pb->bo, pb->ptr);
	reloc->target_handle = target->handle;
	reloc->target_offset = offset;
	reloc->shift = shift;

	return 0;
}

int host1x_client_submit(struct host1x_client *client, struct host1x_job *job)
{
	return client->submit(client, job);
}

int host1x_client_flush(struct host1x_client *client, uint32_t *fence)
{
	return client->flush(client, fence);
}

int host1x_client_wait(struct host1x_client *client, uint32_t fence,
		       uint32_t timeout)
{
	return client->wait(client, fence, timeout);
}
