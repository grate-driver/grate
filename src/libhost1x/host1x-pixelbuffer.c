/*
 * Copyright (c) 2012, 2013 Erik Faye-Lund
 * Copyright (c) 2013 Avionic Design GmbH
 * Copyright (c) 2013 Thierry Reding
 * Copyright (c) 2017 Dmitry Osipenko
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

#define PIXBUF_GUARD_PATTERN	0xF5132803

#include <string.h>
#include "host1x-private.h"

#define ALIGN(x,a)		__ALIGN_MASK(x,(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)	(((x)+(mask))&~(mask))

static bool pixbuf_guard_disabled;

struct host1x_pixelbuffer *host1x_pixelbuffer_create(
				struct host1x *host1x,
				unsigned width, unsigned height,
				unsigned pitch,
				enum pixel_format format,
				enum layout_format layout)
{
	struct host1x_pixelbuffer *pixbuf;
	unsigned long flags = NVHOST_BO_FLAG_FRAMEBUFFER;
	size_t bo_size;

	pixbuf = calloc(1, sizeof(*pixbuf));
	if (!pixbuf)
		return NULL;

	if (layout == PIX_BUF_LAYOUT_TILED_16x16)
		pitch = ALIGN(pitch, 256);

	pixbuf->pitch = pitch;
	pixbuf->width = width;
	pixbuf->height = height;
	pixbuf->format = format;
	pixbuf->layout = layout;

	if (layout == PIX_BUF_LAYOUT_TILED_16x16)
		height = ALIGN(height, 16);

	bo_size = pixbuf->pitch * height;

	if (!pixbuf_guard_disabled)
		bo_size += PIXBUF_GUARD_AREA_SIZE * 2;

	if (layout == PIX_BUF_LAYOUT_TILED_16x16)
		flags |= HOST1X_BO_CREATE_FLAG_TILED;

	flags |= HOST1X_BO_CREATE_FLAG_BOTTOM_UP;

	pixbuf->bo = HOST1X_BO_CREATE(host1x, bo_size, flags);
	if (!pixbuf->bo) {
		free(pixbuf);
		return NULL;
	}

	if (!pixbuf_guard_disabled)
		pixbuf->bo->offset += PIXBUF_GUARD_AREA_SIZE;

	host1x_pixelbuffer_setup_guard(pixbuf);

	return pixbuf;
}

void host1x_pixelbuffer_free(struct host1x_pixelbuffer *pixbuf)
{
	host1x_bo_free(pixbuf->bo);
	free(pixbuf);
}

int host1x_pixelbuffer_load_data(struct host1x *host1x,
				 struct host1x_pixelbuffer *pixbuf,
				 void *data,
				 unsigned data_pitch,
				 unsigned long data_size,
				 enum pixel_format data_format,
				 enum layout_format data_layout)
{
	struct host1x_pixelbuffer *tmp;
	bool blit = false;
	void *map;
	int err;

	if (pixbuf->format != data_format)
		return -1;

	if (pixbuf->layout != data_layout)
		blit = true;

	if (pixbuf->pitch != data_pitch)
		blit = true;

	if (blit) {
		tmp = host1x_pixelbuffer_create(host1x,
						pixbuf->width, pixbuf->height,
						data_pitch, data_format,
						data_layout);
		if (!tmp)
			return -1;
	} else {
		tmp = pixbuf;
	}

	err = HOST1X_BO_MMAP(tmp->bo, &map);
	if (err)
		return err;

	memcpy(map + tmp->bo->offset, data, data_size);

	HOST1X_BO_FLUSH(tmp->bo, tmp->bo->offset, data_size);

	if (blit) {
		err = host1x_gr2d_blit(host1x->gr2d, tmp, pixbuf,
				       0, 0, 0, 0,
				       pixbuf->width, pixbuf->height);
		host1x_pixelbuffer_free(tmp);
	}

	return err;
}

void host1x_pixelbuffer_setup_guard(struct host1x_pixelbuffer *pixbuf)
{
	volatile uint32_t *guard;
	unsigned i;

	if (pixbuf_guard_disabled)
		return;

	HOST1X_BO_MMAP(pixbuf->bo, (void**)&guard);

	for (i = 0; i < PIXBUF_GUARD_AREA_SIZE / 4; i++)
		guard[i] = PIXBUF_GUARD_PATTERN + i;

	HOST1X_BO_FLUSH(pixbuf->bo, 0, PIXBUF_GUARD_AREA_SIZE);

	guard = (void*)guard + pixbuf->bo->size - PIXBUF_GUARD_AREA_SIZE;

	for (i = 0; i < PIXBUF_GUARD_AREA_SIZE / 4; i++)
		guard[i] = PIXBUF_GUARD_PATTERN + i;

	HOST1X_BO_FLUSH(pixbuf->bo, pixbuf->bo->size - PIXBUF_GUARD_AREA_SIZE,
			PIXBUF_GUARD_AREA_SIZE);
}

void host1x_pixelbuffer_check_guard(struct host1x_pixelbuffer *pixbuf)
{
	struct host1x_bo *orig_bo;
	volatile uint32_t *guard;
	bool smashed = false;
	uint32_t value;
	unsigned i;

	if (pixbuf_guard_disabled)
		return;

	orig_bo = pixbuf->bo->wrapped ?: pixbuf->bo;

	HOST1X_BO_INVALIDATE(orig_bo, orig_bo->size - PIXBUF_GUARD_AREA_SIZE,
			     PIXBUF_GUARD_AREA_SIZE);
	HOST1X_BO_MMAP(orig_bo, (void**)&guard);

	for (i = 0; i < PIXBUF_GUARD_AREA_SIZE / 4; i++) {
		value = guard[i];

		if (value != PIXBUF_GUARD_PATTERN + i) {
			host1x_error("Back guard[%d of %d] smashed, "
				     "0x%08X != 0x%08X\n",
				     i, PIXBUF_GUARD_AREA_SIZE / 4 - 1,
				     value, PIXBUF_GUARD_PATTERN + i);
			smashed = true;
		}
	}

	if (smashed)
		goto smashed_abort;

	guard = (void*)guard + orig_bo->size - PIXBUF_GUARD_AREA_SIZE;

	for (i = 0; i < PIXBUF_GUARD_AREA_SIZE / 4; i++) {
		value = guard[i];

		if (value != PIXBUF_GUARD_PATTERN + i) {
			host1x_error("Front guard[%d of %d] smashed, "
				     "0x%08X != 0x%08X\n",
				     i, PIXBUF_GUARD_AREA_SIZE / 4 - 1,
				     value, PIXBUF_GUARD_PATTERN + i);
			smashed = true;
		}
	}

	if (smashed) {
smashed_abort:
		host1x_error("Pixbuf %p: width %u, height %u, "
			     "pitch %u, format %u\n",
			      pixbuf, pixbuf->width, pixbuf->height,
			      pixbuf->pitch, pixbuf->format);
		abort();
	}
}

void host1x_pixelbuffer_disable_bo_guard(void)
{
	pixbuf_guard_disabled = true;
}

bool host1x_pixelbuffer_bo_guard_disabled(void)
{
	return pixbuf_guard_disabled;
}
