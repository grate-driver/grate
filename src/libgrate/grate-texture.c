/*
 * Copyright (c) 2017 Dmitry Osipenko
 * Copyright (c) 2017 Erik Faye-Lund
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

#include <IL/il.h>
#include <IL/ilu.h>

#include "grate.h"
#include "grate-3d.h"

#include "libgrate-private.h"

#define ALIGN(x,a)		__ALIGN_MASK(x,(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)	(((x)+(mask))&~(mask))

struct grate_texture *grate_create_texture(struct grate *grate,
					   unsigned width, unsigned height,
					   enum pixel_format format,
					   enum layout_format layout)
{
	struct grate_texture *tex;
	unsigned pitch;

	switch (format) {
	case PIX_BUF_FMT_RGBA8888:
		break;
	default:
		grate_error("Invalid format %u\n", format);
		return NULL;
	}

	switch (layout) {
	case PIX_BUF_LAYOUT_LINEAR:
		break;
	default:
		grate_error("Invalid layout %u\n", layout);
		return NULL;
	}

	tex = calloc(1, sizeof(*tex));
	if (!tex)
		return NULL;

	/* Pitch needs to be aligned to 64 bytes */
	pitch = ALIGN(width * PIX_BUF_FORMAT_BYTES(format), 64);

	tex->pb = host1x_pixelbuffer_create(grate->host1x, width, height, pitch,
					    format, layout);
	if (!tex->pb) {
		free(tex);
		return NULL;
	}

	return tex;
}

int grate_texture_load(struct grate *grate, struct grate_texture *tex,
		       const char *path)
{
	ILuint ImageTex;
	int err;

	ilInit();
	ilEnable(IL_ORIGIN_SET);
	ilOriginFunc(IL_ORIGIN_LOWER_LEFT);
	ilGenImages(1, &ImageTex);
	ilBindImage(ImageTex);
	ilLoadImage(path);
	ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
	iluScale(tex->pb->width, tex->pb->height, 0);

	err = ilGetError();
	if (err != IL_NO_ERROR) {
		grate_error("\"%s\" load failed 0x%04X\n", path, err);
		goto out;
	}

	err = host1x_pixelbuffer_load_data(grate->host1x, tex->pb,
					   ilGetData(),
					   ilGetInteger(IL_IMAGE_WIDTH) * 4,
					   ilGetInteger(IL_IMAGE_SIZE_OF_DATA),
					   PIX_BUF_FMT_RGBA8888,
					   PIX_BUF_LAYOUT_LINEAR);
out:
	ilDeleteImage(ImageTex);

	return err;
}

struct host1x_pixelbuffer *grate_texture_pixbuf(struct grate_texture *tex)
{
	return tex->pb;
}

void grate_texture_free(struct grate_texture *tex)
{
	host1x_pixelbuffer_free(tex->pb);
	free(tex);
}

void grate_texture_set_max_lod(struct grate_texture *tex, unsigned max_lod)
{
	tex->max_lod = max_lod;
}

void grate_texture_set_wrap_mode(struct grate_texture *tex, unsigned wrap_mode)
{
	tex->wrap_mode = wrap_mode;
}

void grate_texture_set_mip_filter(struct grate_texture *tex, bool enable)
{
	tex->mip_filter = enable;
}

void grate_texture_set_mag_filter(struct grate_texture *tex, bool enable)
{
	tex->mag_filter = enable;
}

void grate_texture_set_min_filter(struct grate_texture *tex, bool enable)
{
	tex->min_filter = enable;
}
