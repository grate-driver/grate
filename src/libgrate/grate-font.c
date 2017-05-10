/*
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

#define _GNU_SOURCE

#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "libgrate-private.h"
#include "grate-3d.h"
#include "tgr_3d.xml.h"

#define CHAR_SZ		(32 * 2 + 12)
#define MAX_CHARS	(4096 / CHAR_SZ)

struct character {
	unsigned code;
	unsigned pos_x;
	unsigned pos_y;
	unsigned width;
	unsigned height;
	int off_x;
	int off_y;
	unsigned orig_width;
	unsigned orig_height;
};

struct grate_font {
	struct grate_program *program;
	struct grate_texture *texture;
	struct character ch[128];
	struct host1x_bo *indices_bo;
	struct host1x_bo *vertices_bo;
	struct host1x_bo *uv_bo;
	float *vertices;
	float *uv;
};

static const char *vs_asm = "					\n\
.exports							\n\
	[0] = \"position\";					\n\
	[7] = \"texcoord\";					\n\
								\n\
.attributes							\n\
	[0] = \"position\";					\n\
	[1] = \"texcoord\";					\n\
								\n\
.asm								\n\
EXEC(export[0]=vector) MOVv r63.xy**, a[0].xyzw;		\n\
EXEC(export[7]=vector) MOVv r63.xy**, a[1].xyzw;		\n\
";

static const char *fs_asm = "					\n\
pseq_to_dw_exec_nb = 1						\n\
alu_buffer_size = 1						\n\
								\n\
.asm								\n\
EXEC								\n\
	MFU:	sfu: rcp r4					\n\
		mul0: bar, sfu, bar0				\n\
		mul1: bar, sfu, bar1				\n\
		ipl: t0.fp20, t0.fp20, NOP, NOP			\n\
								\n\
	TEX:	tex r0, r1, tex0, r0, r1, r2			\n\
								\n\
	ALU:	ALU0:	MAD kill, r1.h, #1, #0 (eq)		\n\
								\n\
	DW:	store rt1, r0, r1				\n\
;								\n\
";

static const char *ln_asm = "					\n\
LINK fp20, fp20, NOP, NOP, tram0.xyzw, export1			\n\
";

static int parse_config(struct character *chars, const char *config_path)
{
	struct character ch;
	FILE *fp;
	char *text = NULL;
	size_t sz;
	int nb = 0;
	int ret;

	fp = fopen(config_path, "r");
	if (!fp)
		return -1;

	while (getline(&text, &sz, fp) != -1) {
		ret = sscanf(text, "%u\t%u\t%u\t%u\t%u\t%d\t%d\t%u\t%u",
			     &ch.code, &ch.pos_x, &ch.pos_y, &ch.width,
			     &ch.height, &ch.off_x, &ch.off_y, &ch.orig_width,
			     &ch.orig_height);
		if (ret != 9)
			continue;

		if (ch.code > 127)
			continue;

		chars[ch.code] = ch; nb++;
	}

	free(text);
	fclose(fp);

	return nb == 0;
}

struct grate_font *grate_create_font(struct grate *grate,
				     const char *font_path,
				     const char *config_path)
{
	struct grate_font *font;
	struct grate_shader *vs, *fs, *linker;
	struct grate_program *program;
	struct grate_texture *texture;
	struct host1x_bo *bo;
	uint16_t *indices;
	void *map;
	int err;
	int i;

	vs = grate_shader_parse_vertex_asm(vs_asm);
	if (!vs) {
		grate_error("vs assembler parse failed\n");
		return NULL;
	}

	fs = grate_shader_parse_fragment_asm(fs_asm);
	if (!fs) {
		grate_error("fs assembler parse failed\n");
		return NULL;
	}

	linker = grate_shader_parse_linker_asm(ln_asm);
	if (!linker) {
		grate_error("linker assembler parse failed\n");
		return NULL;
	}

	program = grate_program_new(grate, vs, fs, linker);
	if (!program) {
		grate_error("grate_program_new() failed\n");
		return NULL;
	}

	grate_program_link(program);

	texture = grate_create_texture2(grate, font_path,
					PIX_BUF_FMT_RGBA8888,
					PIX_BUF_LAYOUT_LINEAR);
	if (!texture)
		return NULL;

	font= calloc(1, sizeof(*font));
	if (!font)
		return NULL;

	err = parse_config(font->ch, config_path);
	if (err) {
		grate_error("%s parse failed\n", config_path);
		return NULL;
	}

	bo = HOST1X_BO_CREATE(grate->host1x, MAX_CHARS * CHAR_SZ,
			      NVHOST_BO_FLAG_ATTRIBUTES);
	if (!bo)
		return NULL;

	err = HOST1X_BO_MMAP(bo, &map);
	if (err != 0)
		return NULL;

	font->vertices_bo = HOST1X_BO_WRAP(bo, 0, MAX_CHARS * 32);
	if (!font->vertices_bo)
		return NULL;

	font->uv_bo = HOST1X_BO_WRAP(font->vertices_bo, 0, MAX_CHARS * 32);
	if (!font->uv_bo)
		return NULL;

	font->indices_bo = HOST1X_BO_WRAP(font->uv_bo, 0, MAX_CHARS * 12);
	if (!font->indices_bo)
		return NULL;

	indices = map + MAX_CHARS * 64;

	for (i = 0; i < MAX_CHARS; i++) {
		indices[0 + i * 6] = 0 + i * 4;
		indices[1 + i * 6] = 1 + i * 4;
		indices[2 + i * 6] = 2 + i * 4;
		indices[3 + i * 6] = 0 + i * 4;
		indices[4 + i * 6] = 2 + i * 4;
		indices[5 + i * 6] = 3 + i * 4;
	}

	HOST1X_BO_FLUSH(font->indices_bo, font->indices_bo->offset,
			MAX_CHARS * sizeof(indices));

	font->vertices = map;
	font->uv = map + MAX_CHARS * 32;
	font->texture = texture;
	font->program = program;

	return font;
}

void grate_3d_printf(struct grate *grate,
		     const struct grate_3d_ctx *ctx,
		     struct grate_font *font,
		     unsigned render_target,
		     float x, float y, float scale,
		     const char *fmt, ...)
{
	struct host1x_pixelbuffer *fb_pixbuf;
	struct host1x_pixelbuffer *tex_pixbuf;
	struct grate_3d_ctx ctx_copy;
	struct character *ch;
	va_list ap;
	unsigned code, chars_nb, chars_nb_to_draw = 0, location, i;
	float left, right, top, bottom, tex_w, tex_h, fb_w, fb_h, orig_x = x;
	char *text = NULL;
	int offt, ret;
	bool skip;

	va_start(ap, fmt);
	ret = vasprintf(&text, fmt, ap);
	va_end(ap);

	if (ret < 0)
		goto out;

	chars_nb = strlen(text);
	if (!chars_nb)
		goto out;

	fb_pixbuf = ctx->render_targets[render_target].pixbuf;
	tex_pixbuf = font->texture->pixbuf;

	if (render_target > 15 || !fb_pixbuf) {
		grate_error("Invalid render target %u\n", render_target);
		goto out;
	}

	ctx_copy = *ctx;
	grate_3d_ctx_perform_depth_test(&ctx_copy, false);
	grate_3d_ctx_perform_depth_write(&ctx_copy, false);
	grate_3d_ctx_perform_stencil_test(&ctx_copy, false);
	grate_3d_ctx_set_cull_face(&ctx_copy, GRATE_3D_CTX_CULL_FACE_NONE);
	grate_3d_ctx_bind_program(&ctx_copy, font->program);

	for (i = 0; i < 16; i++) {
		grate_3d_ctx_disable_vertex_attrib_array(&ctx_copy, i);
		grate_3d_ctx_disable_render_target(&ctx_copy, i);
		grate_3d_ctx_bind_texture(&ctx_copy, i, NULL);
	}

	location = grate_get_attribute_location(font->program, "position");
	grate_3d_ctx_vertex_attrib_float_pointer(&ctx_copy, location,
						 2, font->vertices_bo);
	grate_3d_ctx_enable_vertex_attrib_array(&ctx_copy, location);

	location = grate_get_attribute_location(font->program, "texcoord");
	grate_3d_ctx_vertex_attrib_float_pointer(&ctx_copy, location,
						 2, font->uv_bo);
	grate_3d_ctx_enable_vertex_attrib_array(&ctx_copy, location);

	grate_3d_ctx_bind_texture(&ctx_copy, 0, font->texture);
	grate_texture_set_wrap_t(font->texture, GRATE_TEXTURE_MIRRORED_REPEAT);
	grate_texture_set_mag_filter(font->texture, GRATE_TEXTURE_LINEAR);

	grate_3d_ctx_bind_render_target(&ctx_copy, render_target, fb_pixbuf);
	grate_3d_ctx_enable_render_target(&ctx_copy, render_target);

	fb_w = fb_pixbuf->width;
	fb_h = fb_pixbuf->height;

	tex_w = tex_pixbuf->width;
	tex_h = tex_pixbuf->height;

	for (i = 0, skip = false; i < chars_nb; i++, skip = false) {
		code = text[i];
		ch = &font->ch[code];

		switch (code) {
		case '\n':
			ch = &font->ch[' '];
			y -= ch->orig_height / fb_h * scale;
			x  = orig_x;
			skip = true;
			break;
		case ' ':
			x += ch->orig_width / fb_w * scale;
			skip = true;
			break;
		default:
			if (code > 127)
				ch = &font->ch['?'];
			break;
		}

		if (!skip) {
			left   = ch->pos_x / tex_w;
			right  = ch->width / tex_w + left;
			top    = ch->pos_y / tex_h;
			bottom = ch->height / tex_h + top;

			font->uv[0 + chars_nb_to_draw * 8] = left;
			font->uv[1 + chars_nb_to_draw * 8] = 1.0f - top;
			font->uv[2 + chars_nb_to_draw * 8] = right;
			font->uv[3 + chars_nb_to_draw * 8] = 1.0f - top;
			font->uv[4 + chars_nb_to_draw * 8] = right;
			font->uv[5 + chars_nb_to_draw * 8] = 1.0f - bottom;
			font->uv[6 + chars_nb_to_draw * 8] = left;
			font->uv[7 + chars_nb_to_draw * 8] = 1.0f - bottom;

			offt = ch->orig_height - ch->off_y - ch->height;

			left   = x    +  ch->off_x / fb_w * scale;
			right  = left +  ch->width / fb_w * scale;
			top    = y    +       offt / fb_h * scale;
			bottom = top  + ch->height / fb_h * scale;

			font->vertices[0 + chars_nb_to_draw * 8] = left;
			font->vertices[1 + chars_nb_to_draw * 8] = bottom;
			font->vertices[2 + chars_nb_to_draw * 8] = right;
			font->vertices[3 + chars_nb_to_draw * 8] = bottom;
			font->vertices[4 + chars_nb_to_draw * 8] = right;
			font->vertices[5 + chars_nb_to_draw * 8] = top;
			font->vertices[6 + chars_nb_to_draw * 8] = left;
			font->vertices[7 + chars_nb_to_draw * 8] = top;

			x += ch->orig_width / fb_w * scale;
		}

		if (skip && i != chars_nb - 1)
			continue;

		if (!skip)
			chars_nb_to_draw++;

		if (chars_nb_to_draw == 0)
			continue;

		if (chars_nb_to_draw != MAX_CHARS && i != chars_nb - 1)
			continue;

		HOST1X_BO_FLUSH(font->uv_bo, font->uv_bo->offset,
				chars_nb_to_draw * 32);
		HOST1X_BO_FLUSH(font->vertices_bo, font->vertices_bo->offset,
				chars_nb_to_draw * 32);

		grate_3d_draw_elements(&ctx_copy,
				       TGR3D_PRIMITIVE_TYPE_TRIANGLES,
				       font->indices_bo,
				       TGR3D_INDEX_MODE_UINT16,
				       chars_nb_to_draw * 6);
		grate_flush(grate);

		chars_nb_to_draw = 0;
	}
out:
	free(text);
}
