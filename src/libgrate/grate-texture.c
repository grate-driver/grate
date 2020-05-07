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

#include <assert.h>
#include <byteswap.h>

#include <IL/il.h>
#include <IL/ilu.h>

#include "etc1.h"
#include "grate.h"
#include "grate-3d.h"

#include "libgrate-private.h"

#define MAX(a, b)		(((a) > (b)) ? (a) : (b))
#define MIN(a, b)		(((a) < (b)) ? (a) : (b))

struct grate_texture *grate_create_texture(struct grate *grate,
					   unsigned width, unsigned height,
					   enum pixel_format format,
					   enum layout_format layout)
{
	struct grate_texture *tex;
	unsigned pitch;

	switch (format) {
	case PIX_BUF_FMT_S8:
	case PIX_BUF_FMT_RGBA8888:
	case PIX_BUF_FMT_D16_LINEAR:
	case PIX_BUF_FMT_D16_NONLINEAR:
	case PIX_BUF_FMT_DXT1:
	case PIX_BUF_FMT_DXT3:
	case PIX_BUF_FMT_DXT5:
	case PIX_BUF_FMT_ETC1:
		break;
	default:
		grate_error("Invalid format %u\n", format);
		return NULL;
	}

	switch (layout) {
	case PIX_BUF_LAYOUT_LINEAR:
	case PIX_BUF_LAYOUT_TILED_16x16:
		break;
	default:
		grate_error("Invalid layout %u\n", layout);
		return NULL;
	}

	tex = calloc(1, sizeof(*tex));
	if (!tex)
		return NULL;

	pitch = PIX_BUF_FORMAT_BYTES(format) * width;
	pitch = pitch / PIX_BUF_FORMAT_TEXEL_WIDTH(format);
	pitch = ALIGN(pitch, PIX_BUF_FORMAT_ALIGNMENT(format));

	tex->pixbuf = host1x_pixelbuffer_create(grate->host1x, width, height,
						pitch, format, layout);
	if (!tex->pixbuf) {
		free(tex);
		return NULL;
	}

	return tex;
}

static int grate_texture_load_internal(struct grate *grate,
				       struct grate_texture **tex,
				       const char *path, bool create,
				       enum pixel_format format,
				       enum layout_format layout)
{
	unsigned tex_pitch;
	unsigned tex_bpp;
	unsigned tw, bpp;
	void *tex_data;
	ILuint tex_size;
	ILuint ImageTex;
	ILenum il_fmt;
	unsigned i;
	int err;

	grate_info("loading \"%s\" pixbuf format 0x%08x layout %u\n",
		   path, format, layout);

	if (format == PIX_BUF_FMT_ETC1)
		il_fmt = IL_RGB;
	else
		il_fmt = IL_RGBA;

	ilInit();
	ilGenImages(1, &ImageTex);
	ilBindImage(ImageTex);
	ilLoadImage(path);
	ilConvertImage(il_fmt, IL_UNSIGNED_BYTE);

	err = ilGetError();
	if (err != IL_NO_ERROR) {
		grate_error("\"%s\" load failed 0x%04X\n", path, err);
		goto out;
	}

	if (create) {
		*tex = grate_create_texture(grate,
					    ilGetInteger(IL_IMAGE_WIDTH),
					    ilGetInteger(IL_IMAGE_HEIGHT),
					    format, layout);
		if (!(*tex)) {
			err = -12;
			goto out;
		}
	} else {
		iluScale((*tex)->pixbuf->width, (*tex)->pixbuf->height, 0);
	}

	/*
	 * ilOriginFunc() doesn't work properly in conjunction with
	 * ilCompressDXT(), image isn't rotated and colors are shifted.
	 *
	 * Note that iluRotate(180) only flips vertically! It's a DevIL's
	 * oddity that comes handy here.
	 */
	iluRotate(180.0f);

	err = ilGetError();
	if (err != IL_NO_ERROR) {
		grate_error("\"%s\" load failed 0x%04X\n", path, err);
		goto out;
	}

	tex_data  = ilGetData();
	tex_size  = ilGetInteger(IL_IMAGE_SIZE_OF_DATA);
	tex_bpp   = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
	tex_pitch = ilGetInteger(IL_IMAGE_WIDTH) * tex_bpp;

	if (PIX_BUF_FORMAT_COMPRESSED(format)) {
		etc1_byte *etc1_data;
		ILenum DXTCFormat;
		ILuint dxtSize;

		switch (format) {
		case PIX_BUF_FMT_DXT1:
			grate_info("compressing data to DXT1\n");
			DXTCFormat = IL_DXT1;
			break;
		case PIX_BUF_FMT_DXT3:
			grate_info("compressing data to DXT3\n");
			DXTCFormat = IL_DXT3;
			break;
		case PIX_BUF_FMT_DXT5:
			grate_info("compressing data to DXT5\n");
			DXTCFormat = IL_DXT5;
			break;
		case PIX_BUF_FMT_ETC1:
			grate_info("compressing data to ETC1\n");
			dxtSize = etc1_get_encoded_data_size(
						ilGetInteger(IL_IMAGE_WIDTH),
						ilGetInteger(IL_IMAGE_HEIGHT));

			etc1_data = malloc(dxtSize);
			if (!etc1_data) {
				err = -1;
				goto out;
			}

			err = etc1_encode_image(tex_data,
						ilGetInteger(IL_IMAGE_WIDTH),
						ilGetInteger(IL_IMAGE_HEIGHT),
						tex_bpp, tex_pitch, etc1_data);
			if (err)
				goto err_compression;

			/* Tegra's GR3D uses a different layout for ETC1 data */
			for (i = 0; i < dxtSize / 8; i++) {
				uint64_t *etc1_word64 = (uint64_t*) etc1_data;
				uint64_t a = bswap_32(etc1_word64[i] >> 32);
				uint64_t b = bswap_32(etc1_word64[i]);

				etc1_word64[i] = (b << 32) | a;
			}

			tex_data = etc1_data;

			goto done_compression;
		default:
			grate_error("\"%s\" unsupported compression format %u\n",
				    path, format);
			err = -1;
			goto out;
		}

		ilEnable(IL_SQUISH_COMPRESS);

		tex_data = ilCompressDXT(tex_data,
					 ilGetInteger(IL_IMAGE_WIDTH),
					 ilGetInteger(IL_IMAGE_HEIGHT), 1,
					 DXTCFormat, &dxtSize);
		err = ilGetError();
err_compression:
		if (err != IL_NO_ERROR) {
			grate_error("\"%s\" compression failed 0x%04X\n",
				    path, err);
			goto out;
		}
done_compression:
		grate_info("compressed \"%s\" size tex_size %u -> %u\n",
			   path, tex_size, dxtSize);

		tw        = PIX_BUF_FORMAT_TEXEL_WIDTH(format);
		bpp       = PIX_BUF_FORMAT_BYTES(format);
		tex_pitch = ALIGN(ilGetInteger(IL_IMAGE_WIDTH), tw) / tw * bpp;
		tex_size  = dxtSize;
	}

	err = host1x_pixelbuffer_load_data(grate->host1x, (*tex)->pixbuf,
					   tex_data, tex_pitch, tex_size,
					   format, PIX_BUF_LAYOUT_LINEAR);
out:
	ilDeleteImage(ImageTex);

	if (err)
		grate_error("failed to load \"%s\"\n", path);
	else
		grate_info("loaded \"%s\"\n", path);

	return err;
}

int grate_texture_load(struct grate *grate, struct grate_texture *tex,
		       const char *path)
{
	return grate_texture_load_internal(grate, &tex, path, false,
					   tex->pixbuf->format,
					   tex->pixbuf->layout);
}

struct grate_texture *grate_create_texture2(struct grate *grate,
					    const char *path,
					    enum pixel_format format,
					    enum layout_format layout)
{
	struct grate_texture *tex;
	int err;

	err = grate_texture_load_internal(grate, &tex, path, true,
					  format, layout);
	if (err)
		return NULL;

	return tex;
}

struct host1x_pixelbuffer *grate_texture_pixbuf(struct grate_texture *tex)
{
	return tex->pixbuf;
}

void grate_texture_free(struct grate_texture *tex)
{
	host1x_pixelbuffer_free(tex->pixbuf);
	free(tex);
}

void grate_texture_set_max_lod(struct grate_texture *tex, unsigned max_lod)
{
	tex->max_lod = max_lod;
}

void grate_texture_set_wrap_s(struct grate_texture *tex,
			      enum grate_textute_wrap_mode wrap_mode)
{
	switch (wrap_mode) {
	case GRATE_TEXTURE_CLAMP_TO_EDGE:
		tex->wrap_s_clamp_to_edge = true;
		tex->wrap_s_mirrored_repeat = false;
		break;
	case GRATE_TEXTURE_MIRRORED_REPEAT:
		tex->wrap_s_clamp_to_edge = false;
		tex->wrap_s_mirrored_repeat = true;
		break;
	case GRATE_TEXTURE_REPEAT:
		tex->wrap_s_clamp_to_edge = false;
		tex->wrap_s_mirrored_repeat = false;
		break;
	}
}

void grate_texture_set_wrap_t(struct grate_texture *tex,
			      enum grate_textute_wrap_mode wrap_mode)
{
	switch (wrap_mode) {
	case GRATE_TEXTURE_CLAMP_TO_EDGE:
		tex->wrap_t_clamp_to_edge = true;
		tex->wrap_t_mirrored_repeat = false;
		break;
	case GRATE_TEXTURE_MIRRORED_REPEAT:
		tex->wrap_t_clamp_to_edge = false;
		tex->wrap_t_mirrored_repeat = true;
		break;
	case GRATE_TEXTURE_REPEAT:
		tex->wrap_t_clamp_to_edge = false;
		tex->wrap_t_mirrored_repeat = false;
		break;
	}
}

void grate_texture_set_min_filter(struct grate_texture *tex,
				  enum grate_textute_filter filter)
{
	switch (filter) {
	case GRATE_TEXTURE_NEAREST:
		tex->min_filter_enabled = false;
		tex->mip_filter_enabled = false;
		tex->mipmap_enabled = false;
		break;
	case GRATE_TEXTURE_LINEAR:
		tex->min_filter_enabled = true;
		tex->mip_filter_enabled = false;
		tex->mipmap_enabled = false;
		break;
	case GRATE_TEXTURE_NEAREST_MIPMAP_NEAREST:
		tex->min_filter_enabled = false;
		tex->mip_filter_enabled = false;
		tex->mipmap_enabled = true;
		break;
	case GRATE_TEXTURE_LINEAR_MIPMAP_NEAREST:
		tex->min_filter_enabled = true;
		tex->mip_filter_enabled = false;
		tex->mipmap_enabled = true;
		break;
	case GRATE_TEXTURE_NEAREST_MIPMAP_LINEAR:
		tex->min_filter_enabled = false;
		tex->mip_filter_enabled = true;
		tex->mipmap_enabled = true;
		break;
	case GRATE_TEXTURE_LINEAR_MIPMAP_LINEAR:
		tex->min_filter_enabled = true;
		tex->mip_filter_enabled = true;
		tex->mipmap_enabled = true;
		break;
	}
}

void grate_texture_set_mag_filter(struct grate_texture *tex,
				  enum grate_textute_filter filter)
{
	switch (filter) {
	case GRATE_TEXTURE_NEAREST:
		tex->mag_filter_enabled = false;
		break;
	case GRATE_TEXTURE_LINEAR:
		tex->mag_filter_enabled = true;
		break;
	default:
		grate_error("Invalid filter: %d\n", filter);
	}
}

void grate_texture_clear(struct grate *grate, struct grate_texture *tex,
			 uint32_t color)
{
	struct host1x_gr2d *gr2d = host1x_get_gr2d(grate->host1x);
	int err;

	err = host1x_gr2d_clear(gr2d, tex->pixbuf, color);
	if (err < 0)
		grate_error("host1x_gr2d_clear() failed: %d\n", err);
}

void grate_texture_clear_rect(struct grate *grate, struct grate_texture *tex,
			      uint32_t color, unsigned x, unsigned y,
			      unsigned width, unsigned height)
{
	struct host1x_gr2d *gr2d = host1x_get_gr2d(grate->host1x);
	int err;

	err = host1x_gr2d_clear_rect(gr2d, tex->pixbuf, color,
				     x, y, width, height);
	if (err < 0)
		grate_error("host1x_gr2d_clear() failed: %d\n", err);
}

static int alloc_mipmap(struct grate *grate, struct grate_texture *tex)
{
	struct host1x_pixelbuffer *pixbuf = tex->pixbuf;
	struct host1x_bo *bo;
	unsigned log2_width, log2_height;
	unsigned lod, lod_levels, size;
	unsigned w, h, bpp;

	if (!tex->pixbuf)
		return -1;

	if (tex->mipmap_pixbuf)
		return 0;

	log2_width  = log2_size(pixbuf->width);
	log2_height = log2_size(pixbuf->height);

	lod_levels = MAX(log2_width, log2_height);

	grate_info("Texture size w: %u h: %u max LOD %u\n",
		   pixbuf->width, pixbuf->height, lod_levels);

	bpp = PIX_BUF_FORMAT_BYTES(pixbuf->format);

	for (size = 0, lod = 0; lod <= lod_levels; lod++) {
		w = MAX(1 << log2_width >> lod, 1);
		h = MAX(1 << log2_height >> lod, 1);

		grate_info("LOD %u w: %u h: %u\toffset 0x%08X\n",
			   lod, w, h, size);

		size += ALIGN(w * bpp, 16) * h;
	}

	if (!host1x_pixelbuffer_bo_guard_disabled())
		size += PIXBUF_GUARD_AREA_SIZE * 2;

	bo = HOST1X_BO_CREATE(grate->host1x, size, NVHOST_BO_FLAG_FRAMEBUFFER);
	if (!bo)
		return -1;

	if (!host1x_pixelbuffer_bo_guard_disabled())
		bo->offset += PIXBUF_GUARD_AREA_SIZE;

	tex->mipmap_pixbuf = calloc(1, sizeof(*tex->mipmap_pixbuf));
	if (!tex->mipmap_pixbuf)
		return -1;

	tex->mipmap_pixbuf->bo     = bo;
	tex->mipmap_pixbuf->width  = 1 << log2_width;
	tex->mipmap_pixbuf->height = 1 << log2_height;
	tex->mipmap_pixbuf->pitch  = tex->mipmap_pixbuf->width * bpp;
	tex->mipmap_pixbuf->format = pixbuf->format;
	tex->mipmap_pixbuf->layout = pixbuf->layout;

	tex->max_lod = lod_levels;

	host1x_pixelbuffer_setup_guard(tex->mipmap_pixbuf);

	return 0;
}

static void setup_lod_pixbuf(struct host1x_pixelbuffer *mipmap,
			     struct host1x_pixelbuffer *dst,
			     unsigned level)
{
	unsigned long offset = 0;
	unsigned long size = 0;
	unsigned w, h, bpp;
	unsigned i;

	bpp = PIX_BUF_FORMAT_BYTES(mipmap->format);

	for (i = 0; i <= level; i++) {
		w = MAX(mipmap->width >> i, 1);
		h = MAX(mipmap->height >> i, 1);
		size = ALIGN(w * bpp, 16) * h;
		offset += size;
	}

	dst->bo     = HOST1X_BO_WRAP(mipmap->bo, offset - size, size);
	dst->format = mipmap->format;
	dst->layout = mipmap->layout;
	dst->width  = MAX(mipmap->width >> level, 1);
	dst->height = MAX(mipmap->height >> level, 1);
	dst->pitch  = ALIGN(dst->width * bpp, 16);

	assert(dst->bo != NULL);
}

int grate_texture_generate_mipmap(struct grate *grate,
				  struct grate_texture *tex)
{
	struct host1x_gr2d *gr2d = host1x_get_gr2d(grate->host1x);
	struct host1x_pixelbuffer *pixbuf = tex->pixbuf;
	struct host1x_pixelbuffer src_pixbuf, dst_pixbuf;
	int lod;
	int err;

	err = alloc_mipmap(grate, tex);
	if (err)
		return err;

	/* Setup base level. */
	setup_lod_pixbuf(tex->mipmap_pixbuf, &dst_pixbuf, 0);

	/* XXX: GR2D can't handle all possible scale ratios? */
	err = host1x_gr2d_surface_blit(gr2d, pixbuf, &dst_pixbuf,
				       0, 0,
				       pixbuf->width,
				       pixbuf->height,
				       0, 0,
				       dst_pixbuf.width,
				       dst_pixbuf.height);
	host1x_bo_free(dst_pixbuf.bo);

	/* Setup mip levels. */
	for (lod = 1; err == 0 && lod <= tex->max_lod; lod++) {
		setup_lod_pixbuf(tex->mipmap_pixbuf, &src_pixbuf, lod - 1);
		setup_lod_pixbuf(tex->mipmap_pixbuf, &dst_pixbuf, lod);

		grate_info("LOD %u w: %u h: %u\toffset 0x%08lX -> 0x%08lX\n",
			   lod,
			   dst_pixbuf.width,
			   dst_pixbuf.height,
			   src_pixbuf.bo->offset,
			   dst_pixbuf.bo->offset);

		err = host1x_gr2d_surface_blit(gr2d, &src_pixbuf, &dst_pixbuf,
					       0, 0,
					       src_pixbuf.width,
					       src_pixbuf.height,
					       0, 0,
					       dst_pixbuf.width,
					       dst_pixbuf.height);
		host1x_bo_free(src_pixbuf.bo);
		host1x_bo_free(dst_pixbuf.bo);
	}

	if (err)
		grate_error("Mipmap generation failed\n");

	return err;
}

int grate_texture_load_miplevel(struct grate *grate,
				struct grate_texture *tex,
				unsigned level, const char *path)
{
	struct host1x_pixelbuffer dst_pixbuf;
	ILuint ImageTex;
	int err;

	err = alloc_mipmap(grate, tex);
	if (err)
		return err;

	setup_lod_pixbuf(tex->mipmap_pixbuf, &dst_pixbuf,
			 MIN(level, tex->max_lod));
	ilInit();
	ilGenImages(1, &ImageTex);
	ilBindImage(ImageTex);
	ilLoadImage(path);
	ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
	iluScale(dst_pixbuf.width, dst_pixbuf.height, 0);

	/*
	 * ilOriginFunc() doesn't work properly in conjunction with
	 * ilCompressDXT().
	 */
	iluRotate(180.0f);

	err = ilGetError();
	if (err != IL_NO_ERROR) {
		grate_error("\"%s\" load failed 0x%04X\n", path, err);
		goto out;
	}

	grate_info("Loading texture w: %u h: %u to LOD %u\toffset 0x%08lX\n",
		   dst_pixbuf.width, dst_pixbuf.height, level,
		   dst_pixbuf.bo->offset);

	err = host1x_pixelbuffer_load_data(grate->host1x, &dst_pixbuf,
					   ilGetData(),
					   ilGetInteger(IL_IMAGE_WIDTH) * 4,
					   ilGetInteger(IL_IMAGE_SIZE_OF_DATA),
					   dst_pixbuf.format,
					   dst_pixbuf.layout);
out:
	host1x_bo_free(dst_pixbuf.bo);
	ilDeleteImage(ImageTex);

	return err;
}

int grate_texture_blit(struct grate *grate,
		       struct grate_texture *src_tex,
		       struct grate_texture *dst_tex,
		       unsigned sx, unsigned sy, unsigned sw, unsigned sh,
		       unsigned dx, unsigned dy, unsigned dw, signed dh)
{
	struct host1x_gr2d *gr2d = host1x_get_gr2d(grate->host1x);
	struct host1x_pixelbuffer *src_pixbuf = src_tex->pixbuf;
	struct host1x_pixelbuffer *dst_pixbuf = dst_tex->pixbuf;
	int err;

	if (sw == dw && sh == dh)
		err = host1x_gr2d_blit(gr2d, src_pixbuf, dst_pixbuf,
				       sx, sy, dx, dy, dw, dh);
	else
		err = host1x_gr2d_surface_blit(gr2d, src_pixbuf, dst_pixbuf,
					       sx, sy, sw, sh, dx, dy, dw, dh);

	return err;
}
