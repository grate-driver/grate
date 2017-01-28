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

struct host1x_framebuffer *host1x_framebuffer_create(struct host1x *host1x,
						     unsigned int width,
						     unsigned int height,
						     enum pixel_format format,
						     unsigned long flags)
{
	struct host1x_framebuffer *fb;
	unsigned pitch;
	int err;

	switch ( PIX_BUF_FORMAT(format) ) {
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

	fb->pb = host1x_pixelbuffer_create(host1x,
					    width, height, pitch, format);
	if (!fb->pb) {
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
	host1x_pixelbuffer_free(fb->pb);
	free(fb);
}

int host1x_framebuffer_save(struct host1x_framebuffer *fb, const char *path)
{
	struct host1x_pixelbuffer *pb = fb->pb;
	png_structp png;
	png_bytep *rows;
	png_infop info;
	unsigned int i;
	void *buffer;
	FILE *fp;
	int err;

	if (PIX_BUF_FORMAT_BITS(pb->format) != 32) {
		fprintf(stderr, "ERROR: %u bits per pixel not supported\n",
			PIX_BUF_FORMAT_BITS(pb->format));
		return -EINVAL;
	}

	err = host1x_bo_mmap(pb->bo, NULL);
	if (err < 0)
		return -EFAULT;

	err = host1x_bo_invalidate(pb->bo, 0, pb->bo->size);
	if (err < 0)
		return -EFAULT;

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

	png_set_IHDR(png, info, pb->width, pb->height, 8, PNG_COLOR_TYPE_RGBA,
		     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
		     PNG_FILTER_TYPE_BASE);
	png_write_info(png, info);

	if (setjmp(png_jmpbuf(png))) {
		fprintf(stderr, "failed to write IHDR\n");
		return -EIO;
	}

	buffer = malloc(pb->pitch * pb->height);
	if (!buffer)
		return ENOMEM;

	detile(buffer, pb, 16, 16);

	rows = malloc(pb->height * sizeof(png_bytep));
	if (!rows) {
		fprintf(stderr, "out-of-memory\n");
		free(buffer);
		return -ENOMEM;
	}

	for (i = 0; i < pb->height; i++)
		rows[pb->height - i - 1] = buffer + i * pb->pitch;

	png_write_image(png, rows);

	free(rows);
	free(buffer);

	if (setjmp(png_jmpbuf(png)))
		return -EIO;

	png_write_end(png, NULL);

	fclose(fp);

	return 0;
}
