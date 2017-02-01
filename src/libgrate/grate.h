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

struct grate_display *grate_display_open(struct grate *grate);
void grate_display_close(struct grate_display *display);
void grate_display_get_resolution(struct grate_display *display,
				  unsigned int *width, unsigned int *height);
void grate_display_show(struct grate_display *display,
			struct grate_framebuffer *fb, bool vsync);

struct grate_overlay *grate_overlay_create(struct grate_display *display);
void grate_overlay_free(struct grate_overlay *overlay);
void grate_overlay_show(struct grate_overlay *overlay,
			struct grate_framebuffer *fb, unsigned int x,
			unsigned int y, unsigned int width,
			unsigned int height, bool vsync);

struct host1x_bo *grate_bo_create_from_data(struct grate *grate, size_t size,
					    unsigned long flags,
					    const void *data);

struct grate_options {
	unsigned int x, y, width, height;
	bool fullscreen;
	bool vsync;
};

bool grate_parse_command_line(struct grate_options *options, int argc,
			      char *argv[]);
struct grate *grate_init(struct grate_options *options);
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

struct grate_3d_ctx;
struct grate_texture;

struct grate_3d_ctx * grate_3d_alloc_ctx(struct grate *grate);
int grate_3d_ctx_vertex_attrib_pointer(struct grate_3d_ctx *ctx,
				       unsigned location, unsigned size,
				       unsigned type, unsigned stride,
				       struct host1x_bo *data_bo);
int grate_3d_ctx_enable_vertex_attrib_array(struct grate_3d_ctx *ctx,
					    unsigned target);
int grate_3d_ctx_disable_vertex_attrib_array(struct grate_3d_ctx *ctx,
					     unsigned target);
int grate_3d_ctx_bind_render_target(struct grate_3d_ctx *ctx,
				    unsigned target,
				    struct host1x_pixelbuffer *pb);
int grate_3d_ctx_set_render_target_dither(struct grate_3d_ctx *ctx,
					  unsigned target,
					  bool enable);
int grate_3d_ctx_enable_render_target(struct grate_3d_ctx *ctx,
				      unsigned target);
int grate_3d_ctx_disable_render_target(struct grate_3d_ctx *ctx,
				       unsigned target);
int grate_3d_ctx_bind_program(struct grate_3d_ctx *ctx,
			      struct grate_program *program);
int grate_3d_ctx_set_vertex_uniform(struct grate_3d_ctx *ctx,
				    unsigned location, unsigned nb,
				    float *values);
int grate_3d_ctx_set_fragment_uniform_fp20(struct grate_3d_ctx *ctx,
					   unsigned location, float value);
int grate_3d_ctx_set_fragment_uniform_fx10_low(struct grate_3d_ctx *ctx,
					       unsigned location, float value);
int grate_3d_ctx_set_fragment_uniform_fx10_high(struct grate_3d_ctx *ctx,
						unsigned location, float value);
void grate_3d_ctx_set_depth_range(struct grate_3d_ctx *ctx,
				  float near, float far);
void grate_3d_ctx_set_dither(struct grate_3d_ctx *ctx, uint32_t unk);
void grate_3d_ctx_set_viewport_bias(struct grate_3d_ctx *ctx,
				    float x, float y, float z);
void grate_3d_ctx_set_viewport_scale(struct grate_3d_ctx *ctx,
				     float width, float height, float depth);
void grate_3d_ctx_set_point_params(struct grate_3d_ctx *ctx, uint32_t params);
void grate_3d_ctx_set_point_size(struct grate_3d_ctx *ctx, float size);
void grate_3d_ctx_set_line_params(struct grate_3d_ctx *ctx, uint32_t params);
void grate_3d_ctx_set_line_width(struct grate_3d_ctx *ctx, float width);
void grate_3d_ctx_use_guardband(struct grate_3d_ctx *ctx, bool enabled);
void grate_3d_ctx_set_front_direction_is_cw(struct grate_3d_ctx *ctx,
					    bool front_cw);
void grate_3d_ctx_set_cull_ccw(struct grate_3d_ctx *ctx, bool cull_ccw);
void grate_3d_ctx_set_cull_cw(struct grate_3d_ctx *ctx, bool cull_cw);
void grate_3d_ctx_set_scissor(struct grate_3d_ctx *ctx,
			      unsigned x, unsigned width,
			      unsigned y, unsigned height);
void grate_3d_ctx_set_point_coord_range(struct grate_3d_ctx *ctx,
					float min_s, float max_s,
					float min_t, float max_t);
void grate_3d_ctx_set_polygon_offset(struct grate_3d_ctx *ctx,
				     float units, float factor);
void grate_3d_ctx_set_provoking_vtx_last(struct grate_3d_ctx *ctx, bool last);
int grate_3d_ctx_bind_texture(struct grate_3d_ctx *ctx,
			      unsigned location,
			      struct grate_texture *tex);

void grate_3d_draw_elements(struct grate_3d_ctx *ctx,
			    unsigned primitive_type,
			    struct host1x_bo *indices_bo,
			    unsigned index_mode,
			    unsigned vtx_count);

struct grate_texture *grate_create_texture(struct grate *grate,
					   unsigned width, unsigned height,
					   enum pixel_format format,
					   enum layout_format layout);
int grate_texture_load(struct grate *grate, struct grate_texture *tex,
		       const char *path);
struct host1x_pixelbuffer *grate_texture_pixbuf(struct grate_texture *tex);
void grate_texture_free(struct grate_texture *tex);
void grate_texture_set_max_lod(struct grate_texture *tex, unsigned max_lod);
void grate_texture_set_wrap_mode(struct grate_texture *tex, unsigned wrap_mode);
void grate_texture_set_mip_filter(struct grate_texture *tex, bool enable);
void grate_texture_set_mag_filter(struct grate_texture *tex, bool enable);
void grate_texture_set_min_filter(struct grate_texture *tex, bool enable);

#endif
