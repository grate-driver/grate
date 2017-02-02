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

#include <string.h>
#include "host1x-private.h"

#define ALIGN(x,a)		__ALIGN_MASK(x,(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)	(((x)+(mask))&~(mask))

struct host1x_pixelbuffer *host1x_pixelbuffer_create(
				struct host1x *host1x,
				unsigned width, unsigned height,
				unsigned pitch,
				enum pixel_format format,
				enum layout_format layout)
{
	struct host1x_pixelbuffer *pixbuf;
	unsigned long flags = 1;

	pixbuf = calloc(1, sizeof(*pixbuf));
	if (!pixbuf)
		return NULL;

	if (layout == PIX_BUF_LAYOUT_TILED_16x16)
		pitch = ALIGN(pitch, 16);

	pixbuf->pitch = pitch;
	pixbuf->width = width;
	pixbuf->height = height;
	pixbuf->format = format;
	pixbuf->layout = layout;

	if (layout == PIX_BUF_LAYOUT_TILED_16x16)
		flags |= HOST1X_BO_CREATE_FLAG_TILED;

	flags |= HOST1X_BO_CREATE_FLAG_BOTTOM_UP;

	pixbuf->bo = HOST1X_BO_CREATE(host1x, pixbuf->pitch * height, flags);
	if (!pixbuf->bo) {
		free(pixbuf);
		return NULL;
	}

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

	memcpy(map, data, data_size);

	HOST1X_BO_FLUSH(tmp->bo, tmp->bo->offset, data_size);

	if (blit) {
		err = host1x_gr2d_blit(host1x->gr2d, tmp, pixbuf,
				       0, 0, 0, 0,
				       pixbuf->width, pixbuf->height);
		host1x_pixelbuffer_free(tmp);
	}

	return err;
}
