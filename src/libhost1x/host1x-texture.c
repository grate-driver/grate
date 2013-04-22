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

#include <png.h>

#include "host1x-private.h"

struct host1x_texture {
	unsigned int width, height, pitch;
	enum host1x_format format;
	struct host1x_bo *bo;
	size_t size;
};

struct host1x_texture *host1x_texture_load(struct host1x *host1x,
					   const char *path)
{
	struct host1x_texture *texture;
	int depth, color, interlace;
	png_uint_32 width, height;
	png_structp png = NULL;
	png_infop info = NULL;
	png_size_t pitch;
	png_bytepp rows;
	png_uint_32 i;
	void *data;
	FILE *fp;
	int err;

	texture = calloc(1, sizeof(*texture));
	if (!texture)
		return NULL;

	fp = fopen(path, "rb");
	if (!fp)
		goto free;

	png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png)
		goto close;

	info = png_create_info_struct(png);
	if (!info)
		goto destroy;

	if (setjmp(png_jmpbuf(png)))
		goto destroy;

	png_init_io(png, fp);
	png_set_sig_bytes(png, 0);

	png_read_info(png, info);
	png_get_IHDR(png, info, &width, &height, &depth, &color, &interlace,
		     NULL, NULL);

	if (color == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png);

	if (color == PNG_COLOR_TYPE_GRAY ||
	    color == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png);

	if (depth == 16)
		png_set_strip_16(png);

	if (depth < 8)
		png_set_packing(png);

	if (color == PNG_COLOR_TYPE_RGB)
		png_set_filler(png, 0xff, PNG_FILLER_AFTER);

	texture->format = HOST1X_FORMAT_RGBA8888;
	png_read_update_info(png, info);

	pitch = png_get_rowbytes(png, info);
	printf("pitch: %u\n", pitch);

	texture->width = width;
	texture->pitch = pitch;
	texture->height = height;
	texture->size = pitch * height;

	texture->bo = host1x_bo_create(host1x, texture->size, 1);
	if (!texture->bo)
		goto destroy;

	err = host1x_bo_mmap(texture->bo, &data);
	if (err < 0) {
		host1x_bo_free(texture->bo);
		goto destroy;
	}

	rows = calloc(height, sizeof(png_bytep));
	if (!rows) {
		host1x_bo_free(texture->bo);
		goto destroy;
	}

	for (i = 0; i < height; i++) {
		unsigned int index = height - i - 1;
		unsigned long offset = i * pitch;

		rows[index] = data + offset;
	}

	png_read_image(png, rows);
	png_read_end(png, info);
	free(rows);

	png_destroy_read_struct(&png, &info, NULL);
	fclose(fp);

	return texture;

destroy:
	png_read_end(png, info);
	png_destroy_read_struct(&png, &info, NULL);
close:
	fclose(fp);
free:
	free(texture);
	return NULL;
}

void host1x_texture_free(struct host1x_texture *texture)
{
	if (texture)
		host1x_bo_free(texture->bo);

	free(texture);
}
