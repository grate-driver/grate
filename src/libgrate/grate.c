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

#include <stdlib.h>

#include "libgrate-private.h"

#include "host1x.h"

struct grate_framebuffer {
	struct host1x_framebuffer *base;
};

struct grate *grate_init(void)
{
	struct grate *grate;

	grate = calloc(1, sizeof(*grate));
	if (!grate)
		return NULL;

	grate->host1x = host1x_open();
	if (!grate->host1x) {
		free(grate);
		return NULL;
	}

	return grate;
}

void grate_exit(struct grate *grate)
{
	if (grate)
		host1x_close(grate->host1x);

	free(grate);
}

void grate_clear_color(struct grate *grate, float red, float green,
		       float blue, float alpha)
{
	grate->clear.r = red;
	grate->clear.g = green;
	grate->clear.b = blue;
	grate->clear.a = alpha;
}

void grate_clear(struct grate *grate)
{
	struct host1x_gr2d *gr2d = host1x_get_gr2d(grate->host1x);
	struct grate_color *clear = &grate->clear;
	int err;

	if (!grate->fb)
		return;

	err = host1x_gr2d_clear(gr2d, grate->fb->base, clear->r, clear->g,
				clear->b, clear->a);
	if (err < 0) {
	}
}

void grate_bind_framebuffer(struct grate *grate, struct grate_framebuffer *fb)
{
	grate->fb = fb;
}

void grate_attribute_pointer(struct grate *grate, const char *name,
			     unsigned int size, unsigned int stride,
			     unsigned int count, const void *buffer)
{
}

void grate_draw_elements(struct grate *grate, enum grate_primitive type,
			 unsigned int size, unsigned int count,
			 const void *indices)
{
}

void grate_flush(struct grate *grate)
{
}

struct grate_framebuffer *grate_framebuffer_new(struct grate *grate,
						unsigned int width,
						unsigned int height,
						enum grate_format format)
{
	struct grate_framebuffer *fb;
	unsigned int bpp = 32;

	if (format != GRATE_RGBA8888)
		return NULL;

	fb = calloc(1, sizeof(*fb));
	if (!fb)
		return NULL;

	fb->base = host1x_framebuffer_create(grate->host1x, width, height, bpp,
					     0);
	if (!fb->base) {
		free(fb);
		return NULL;
	}

	return fb;
}

void grate_framebuffer_free(struct grate_framebuffer *fb)
{
	if (fb)
		host1x_framebuffer_free(fb->base);

	free(fb);
}

void grate_framebuffer_save(struct grate_framebuffer *fb, const char *path)
{
	host1x_framebuffer_save(fb->base, path);
}

void grate_use_program(struct grate *grate, struct grate_program *program)
{
	grate->program = program;
}
