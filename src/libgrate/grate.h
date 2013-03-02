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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define BIT(x) (1 << (x))

struct grate_framebuffer;
struct grate;

enum grate_type {
	GRATE_TYPE_FLOAT,
	GRATE_TYPE_USHORT,
};

enum grate_format {
	GRATE_RGBA8888,
};

struct grate_framebuffer *grate_framebuffer_new(struct grate *grate,
						unsigned int width,
						unsigned int height,
						enum grate_format format);
void grate_framebuffer_free(struct grate_framebuffer *fb);
void grate_framebuffer_save(struct grate_framebuffer *fb, const char *path);

struct grate *grate_init(void);
void grate_exit(struct grate *grate);

void grate_clear_color(struct grate *grate, float red, float green, float blue,
		       float alpha);
void grate_clear(struct grate *grate);

void grate_bind_framebuffer(struct grate *grate, struct grate_framebuffer *fb);

void grate_attribute_pointer(struct grate *grate, const char *name,
			     unsigned int size, unsigned int stride,
			     unsigned int count, const void *buffer);

enum grate_primitive {
	GRATE_TRIANGLES,
};

void grate_draw_elements(struct grate *grate, enum grate_primitive type,
			 unsigned int size, unsigned int count,
			 const void *indices);

void grate_flush(struct grate *grate);

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

struct grate_program;

struct grate_program *grate_program_new(struct grate *grate,
					struct grate_shader *vs,
					struct grate_shader *fs);
void grate_program_free(struct grate_program *program);

void grate_program_link(struct grate_program *program);
void grate_use_program(struct grate *grate, struct grate_program *program);

void grate_uniform(struct grate *grate, const char *name, unsigned int count,
		   float *values);

#endif
