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

#include "host1x-private.h"

#define ALIGN(x,a)		__ALIGN_MASK(x,(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)	(((x)+(mask))&~(mask))

struct host1x_pixelbuffer *host1x_pixelbuffer_create(
				struct host1x *host1x,
				unsigned width, unsigned height,
				unsigned pitch, enum pixel_format format)
{
	struct host1x_pixelbuffer *pb;
	unsigned long flags = 0;

	switch ( PIX_BUF_FORMAT(format) ) {
	case PIX_BUF_FMT_A8:
	case PIX_BUF_FMT_L8:
	case PIX_BUF_FMT_S8:
	case PIX_BUF_FMT_LA88:
	case PIX_BUF_FMT_RGB565:
	case PIX_BUF_FMT_RGBA5551:
	case PIX_BUF_FMT_RGBA4444:
	case PIX_BUF_FMT_D16_LINEAR:
	case PIX_BUF_FMT_D16_NONLINEAR:
	case PIX_BUF_FMT_RGBA8888:
	case PIX_BUF_FMT_RGBA_FP32:
		break;
	default:
		return NULL;
	}

	pb = calloc(1, sizeof(*pb));
	if (!pb)
		return NULL;

	if ( PIX_BUF_FORMAT_TILED(format) )
		pitch = ALIGN(pitch, 16);

	pb->pitch = pitch;
	pb->width = width;
	pb->height = height;
	pb->format = format;

	if ( PIX_BUF_FORMAT_TILED(format) )
		flags |= HOST1X_BO_CREATE_FLAG_TILED;

	flags |= HOST1X_BO_CREATE_FLAG_BOTTOM_UP;

	pb->bo = host1x_bo_create(host1x, pb->pitch * height, flags);
	if (!pb->bo) {
		free(pb);
		return NULL;
	}

	return pb;
}

void host1x_pixelbuffer_free(struct host1x_pixelbuffer *pb)
{
	host1x_bo_free(pb->bo);
	free(pb);
}
