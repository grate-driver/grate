/*
 * Copyright (c) 2012, 2013 Erik Faye-Lund
 * Copyright (c) 2013 Avionic Design GmbH
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

#ifndef GRATE_GRATE_H
#define GRATE_GRATE_H 1

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "grate-3d-ctx.h"
#include "host1x.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define BIT(x) (1 << (x))

struct grate_framebuffer;
struct grate_display;
struct grate_overlay;
struct grate;

#define GRATE_SINGLE_BUFFERED (0 << 0)
#define GRATE_DOUBLE_BUFFERED (1 << 0)

struct grate_framebuffer *grate_framebuffer_create(struct grate *grate,
						   unsigned int width,
						   unsigned int height,
						   enum pixel_format format,
						   enum layout_format layout,
						   unsigned long flags);
void grate_framebuffer_free(struct grate_framebuffer *fb);
void grate_framebuffer_save(struct grate *grate, struct grate_framebuffer *fb,
			    const char *path);
void *grate_framebuffer_data(struct grate_framebuffer *fb, bool front);

struct host1x_bo *grate_bo_create_and_map(struct grate *grate,
					  unsigned long flags,
					  size_t size, void **map);
struct host1x_bo *grate_bo_create_from_data(struct grate *grate, size_t size,
					    unsigned long flags,
					    const void *data);

#define grate_create_attrib_bo_from_data(grate, data)			\
	grate_bo_create_from_data(grate, sizeof(data),			\
				  NVHOST_BO_FLAG_ATTRIBUTES, data)

struct grate_options {
	unsigned int x, y, width, height;
	bool fullscreen;
	bool nodisplay;
	bool vsync;
};

bool grate_parse_command_line(struct grate_options *options, int argc,
			      char *argv[]);
struct grate *grate_init(struct grate_options *options);
struct grate *grate_init2(struct grate_options *options, int fd);
void grate_exit(struct grate *grate);

void grate_clear_color(struct grate *grate, float red, float green, float blue,
		       float alpha);
void grate_clear(struct grate *grate);

void grate_bind_framebuffer(struct grate *grate, struct grate_framebuffer *fb);

struct host1x_pixelbuffer * grate_get_draw_pixbuf(struct grate_framebuffer *fb);

void grate_flush(struct grate *grate);
void grate_swap_buffers(struct grate *grate);
void grate_wait_for_key(struct grate *grate);
bool grate_key_pressed(struct grate *grate);
uint8_t grate_key_pressed2(struct grate *grate);

struct host1x *grate_get_host1x(struct grate *grate);

enum grate_shader_type {
	GRATE_SHADER_VERTEX,
	GRATE_SHADER_FRAGMENT,
};

struct grate_shader;

struct grate_shader *grate_shader_new(struct grate *grate,
				      enum grate_shader_type type,
				      const char *lines[],
				      unsigned int count);
void grate_shader_free(struct grate_shader *shader);
struct grate_shader *grate_shader_parse_vertex_asm(const char *);
const char *grate_shader_disasm_vs(struct grate_shader *shader);
struct grate_shader *grate_shader_parse_fragment_asm(const char *);
const char *grate_shader_disasm_fs(struct grate_shader *shader);
struct grate_shader *grate_shader_parse_linker_asm(const char *);
const char *grate_shader_disasm_linker(struct grate_shader *shader);
struct grate_shader *grate_shader_parse_vertex_asm_from_file(const char *path);
struct grate_shader *grate_shader_parse_fragment_asm_from_file(const char *path);
struct grate_shader *grate_shader_parse_linker_asm_from_file(const char *path);

struct grate_program;

struct grate_program *grate_program_new(struct grate *grate,
					struct grate_shader *vs,
					struct grate_shader *fs,
					struct grate_shader *linker);
void grate_program_free(struct grate_program *program);

int grate_get_attribute_location(struct grate_program *program,
				 const char *name);
int grate_get_vertex_uniform_location(struct grate_program *program,
				      const char *name);
int grate_get_fragment_uniform_location(struct grate_program *program,
					const char *name);

void grate_program_link(struct grate_program *program);
void grate_use_program(struct grate *grate, struct grate_program *program);

struct grate_profile;

struct grate_profile *grate_profile_start(struct grate *grate);
void grate_profile_free(struct grate_profile *profile);
void grate_profile_sample(struct grate_profile *profile);
void grate_profile_finish(struct grate_profile *profile);
float grate_profile_time_elapsed(struct grate_profile *profile);

struct grate_3d_ctx;
struct grate_texture;

void grate_3d_draw_elements(struct grate_3d_ctx *ctx,
			    unsigned primitive_type,
			    struct host1x_bo *indices_bo,
			    unsigned index_mode,
			    unsigned vtx_count);

enum grate_textute_wrap_mode {
	GRATE_TEXTURE_CLAMP_TO_EDGE,
	GRATE_TEXTURE_MIRRORED_REPEAT,
	GRATE_TEXTURE_REPEAT,
};

enum grate_textute_filter {
	GRATE_TEXTURE_NEAREST,
	GRATE_TEXTURE_LINEAR,
	GRATE_TEXTURE_NEAREST_MIPMAP_NEAREST,
	GRATE_TEXTURE_LINEAR_MIPMAP_NEAREST,
	GRATE_TEXTURE_NEAREST_MIPMAP_LINEAR,
	GRATE_TEXTURE_LINEAR_MIPMAP_LINEAR,
};

struct grate_texture *grate_create_texture(struct grate *grate,
					   unsigned width, unsigned height,
					   enum pixel_format format,
					   enum layout_format layout);
struct grate_texture *grate_create_texture2(struct grate *grate,
					    const char *path,
					    enum pixel_format format,
					    enum layout_format layout);
int grate_texture_load(struct grate *grate, struct grate_texture *tex,
		       const char *path);
struct host1x_pixelbuffer *grate_texture_pixbuf(struct grate_texture *tex);
void grate_texture_free(struct grate_texture *tex);
void grate_texture_set_max_lod(struct grate_texture *tex, unsigned max_lod);
void grate_texture_set_wrap_s(struct grate_texture *tex,
			      enum grate_textute_wrap_mode wrap_mode);
void grate_texture_set_wrap_t(struct grate_texture *tex,
			      enum grate_textute_wrap_mode wrap_mode);
void grate_texture_set_min_filter(struct grate_texture *tex,
				  enum grate_textute_filter filter);
void grate_texture_set_mag_filter(struct grate_texture *tex,
				  enum grate_textute_filter filter);
void grate_texture_clear(struct grate *grate, struct grate_texture *texture,
			 uint32_t color);
void grate_texture_clear2(struct grate *grate, struct grate_texture *texture,
			 uint32_t color, unsigned x, unsigned y,
			 unsigned width, unsigned height);
int grate_texture_generate_mipmap(struct grate *grate,
				  struct grate_texture *tex);
int grate_texture_load_miplevel(struct grate *grate,
				struct grate_texture *tex,
				unsigned level, const char *path);
int grate_texture_blit(struct grate *grate,
		       struct grate_texture *src_tex,
		       struct grate_texture *dst_tex,
		       unsigned sx, unsigned sy, unsigned sw, unsigned sh,
		       unsigned dx, unsigned dy, unsigned dw, unsigned dh);

struct grate_font;

struct grate_font *grate_create_font(struct grate *grate,
				     const char *font_path,
				     const char *config_path);
void grate_3d_printf(struct grate *grate,
		     const struct grate_3d_ctx *ctx,
		     struct grate_font *font,
		     float x, float y, float scale,
		     const char *fmt, ...);

#endif
