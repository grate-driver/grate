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

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define BIT(x) (1 << (x))

struct grate_framebuffer;
struct grate_display;
struct grate_overlay;
struct grate_bo;
struct grate;

enum grate_type {
	GRATE_TYPE_FLOAT,
	GRATE_TYPE_USHORT,
};

enum grate_format {
	GRATE_RGBA8888,
};

#define GRATE_DOUBLE_BUFFERED (1 << 0)

struct grate_framebuffer *grate_framebuffer_create(struct grate *grate,
						   unsigned int width,
						   unsigned int height,
						   enum grate_format format,
						   unsigned long flags);
void grate_framebuffer_free(struct grate_framebuffer *fb);
void grate_framebuffer_save(struct grate_framebuffer *fb, const char *path);
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

struct grate_bo *grate_bo_create(struct grate *grate, size_t size,
				 unsigned long flags);
void grate_bo_free(struct grate_bo *bo);
void *grate_bo_map(struct grate_bo *bo);
void grate_bo_unmap(struct grate_bo *bo, void *ptr);

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

void grate_viewport(struct grate *grate, float x, float y, float width,
		    float height);

void grate_bind_framebuffer(struct grate *grate, struct grate_framebuffer *fb);

int grate_get_attribute_location(struct grate *grate, const char *name);
void grate_attribute_pointer(struct grate *grate, unsigned int location,
			     unsigned int size, unsigned int stride,
			     unsigned int count, struct grate_bo *bo,
			     unsigned long offset);

int grate_get_uniform_location(struct grate *grate, const char *name);
void grate_uniform(struct grate *grate, unsigned int location,
		   unsigned int count, float *values);

enum grate_primitive {
	GRATE_POINTS,
	GRATE_LINES,
	GRATE_LINE_LOOP,
	GRATE_LINE_STRIP,
	GRATE_TRIANGLES,
	GRATE_TRIANGLE_STRIP,
	GRATE_TRIANGLE_FAN
};

void grate_draw_elements(struct grate *grate, enum grate_primitive type,
			 unsigned int size, unsigned int count,
			 struct grate_bo *bo, unsigned long offset);

void grate_flush(struct grate *grate);
void grate_swap_buffers(struct grate *grate);
void grate_wait_for_key(struct grate *grate);
bool grate_key_pressed(struct grate *grate);

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

void grate_program_link(struct grate_program *program);
void grate_use_program(struct grate *grate, struct grate_program *program);

struct grate_profile;

struct grate_profile *grate_profile_start(struct grate *grate);
void grate_profile_free(struct grate_profile *profile);
void grate_profile_sample(struct grate_profile *profile);
void grate_profile_finish(struct grate_profile *profile);

#endif
