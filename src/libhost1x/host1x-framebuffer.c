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
#include <stdlib.h>
#include <string.h>

#include <png.h>

#include "host1x-private.h"

#if 0
static void detile(void *target, struct host1x_pixelbuffer *pb,
		   unsigned int tx, unsigned int ty)
{
	const unsigned int nx = pb->pitch / tx, ny = pb->height / ty;
	const unsigned int size = tx * ty, pitch = tx * nx;
	const void *source = pb->bo->ptr;
	unsigned int i, j, k;

	for (j = 0; j < ny; j++) {
		for (i = 0; i < nx; i++) {
			unsigned int to = (j * nx * size) + (i * tx);
			unsigned int so = (j * nx + i) * size;

			for (k = 0; k < ty; k++) {
				memcpy(target + to + k * pitch,
				       source + so + k * tx,
				       tx);
			}
		}
	}
}
#endif

struct host1x_framebuffer *host1x_framebuffer_create(struct host1x *host1x,
						     unsigned int width,
						     unsigned int height,
						     enum pixel_format format,
						     enum layout_format layout,
						     unsigned long flags)
{
	struct host1x_framebuffer *fb;
	unsigned pitch;
	int err;

	switch (layout) {
	case PIX_BUF_LAYOUT_LINEAR:
	case PIX_BUF_LAYOUT_TILED_16x16:
		break;
	default:
		return NULL;
	}

	switch (format) {
	case PIX_BUF_FMT_RGB565:
		pitch = width * 2;
		break;
	case PIX_BUF_FMT_RGBA8888:
		pitch = width * 4;
		break;
	default:
		return NULL;
	}

	fb = calloc(1, sizeof(*fb));
	if (!fb)
		return NULL;

	fb->pixbuf = host1x_pixelbuffer_create(host1x,
					       width, height, pitch,
					       format, layout);
	if (!fb->pixbuf) {
		free(fb);
		return NULL;
	}

	fb->flags = flags;

	if (host1x->framebuffer_init) {
		err = host1x->framebuffer_init(host1x, fb);
		if (err < 0) {
			host1x_framebuffer_free(fb);
			fb = NULL;
		}
	}

	return fb;
}

void host1x_framebuffer_free(struct host1x_framebuffer *fb)
{
	host1x_pixelbuffer_free(fb->pixbuf);
	free(fb);
}

int host1x_framebuffer_save(struct host1x *host1x,
			    struct host1x_framebuffer *fb,
			    const char *path)
{
	struct host1x_pixelbuffer *tiled_pixbuf = fb->pixbuf;
	struct host1x_pixelbuffer *detiled_pixbuf;
	png_structp png;
	png_bytep *rows;
	png_infop info;
	unsigned int i;
	void *buffer;
	FILE *fp;
	int err;

	if (PIX_BUF_FORMAT_BITS(tiled_pixbuf->format) != 32) {
		fprintf(stderr, "ERROR: %u bits per pixel not supported\n",
			PIX_BUF_FORMAT_BITS(tiled_pixbuf->format));
		return -EINVAL;
	}

	fp = fopen(path, "wb");
	if (!fp) {
		fprintf(stderr, "failed to write `%s'\n", path);
		return -errno;
	}

	png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png)
		return -ENOMEM;

	info = png_create_info_struct(png);
	if (!info)
		return -ENOMEM;

	if (setjmp(png_jmpbuf(png)))
		return -EIO;

	png_init_io(png, fp);

	if (setjmp(png_jmpbuf(png)))
		return -EIO;

	png_set_IHDR(png, info, tiled_pixbuf->width, tiled_pixbuf->height,
		     8, PNG_COLOR_TYPE_RGBA,
		     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
		     PNG_FILTER_TYPE_BASE);
	png_write_info(png, info);

	if (setjmp(png_jmpbuf(png))) {
		fprintf(stderr, "failed to write IHDR\n");
		return -EIO;
	}

	if (tiled_pixbuf->layout == PIX_BUF_LAYOUT_TILED_16x16) {
		detiled_pixbuf = host1x_pixelbuffer_create(host1x,
							tiled_pixbuf->width,
							tiled_pixbuf->height,
							tiled_pixbuf->width * 4,
							tiled_pixbuf->format,
							PIX_BUF_LAYOUT_LINEAR);
		if (!detiled_pixbuf)
			return -ENOMEM;

		err = host1x_gr2d_blit(host1x->gr2d,
				       tiled_pixbuf,
				       detiled_pixbuf,
				       0, 0, 0, 0,
				       tiled_pixbuf->width,
				       tiled_pixbuf->height);
		if (err < 0)
			return -EFAULT;
	} else {
		detiled_pixbuf = tiled_pixbuf;
	}

	err = host1x_bo_mmap(detiled_pixbuf->bo, &buffer);
	if (err < 0)
		return -EFAULT;

	err = host1x_bo_invalidate(detiled_pixbuf->bo, 0,
				   detiled_pixbuf->bo->size);
	if (err < 0)
		return -EFAULT;

	rows = malloc(detiled_pixbuf->height * sizeof(png_bytep));
	if (!rows) {
		fprintf(stderr, "out-of-memory\n");
		return -ENOMEM;
	}

	for (i = 0; i < detiled_pixbuf->height; i++)
		rows[detiled_pixbuf->height - i - 1] =
					buffer + i * detiled_pixbuf->pitch;

	png_write_image(png, rows);

	if (setjmp(png_jmpbuf(png)))
		return -EIO;

	png_write_end(png, NULL);

	if (detiled_pixbuf != tiled_pixbuf)
		host1x_pixelbuffer_free(detiled_pixbuf);

	fclose(fp);

	return 0;
}
