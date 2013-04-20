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

#include "libgrate-private.h"
#include "host1x.h"

struct grate_display {
	struct host1x_display *base;
};

struct grate_overlay {
	struct host1x_overlay *base;
};

struct grate_display *grate_display_open(struct grate *grate)
{
	struct grate_display *display;

	display = calloc(1, sizeof(*display));
	if (!display)
		return NULL;

	display->base = host1x_get_display(grate->host1x);

	return display;
}

void grate_display_close(struct grate_display *display)
{
	free(display);
}

void grate_display_get_resolution(struct grate_display *display,
				  unsigned int *width, unsigned int *height)
{
	host1x_display_get_resolution(display->base, width, height);
}

void grate_display_show(struct grate_display *display,
			struct grate_framebuffer *fb, bool vsync)
{
	grate_framebuffer_swap(fb);

	host1x_display_set(display->base, fb->front, vsync);
}

struct grate_overlay *grate_overlay_create(struct grate_display *display)
{
	struct grate_overlay *overlay;
	int err;

	overlay = calloc(1, sizeof(*overlay));
	if (!overlay)
		return NULL;

	err = host1x_overlay_create(&overlay->base, display->base);
	if (err < 0) {
		grate_error("host1x_overlay_create() failed: %d\n", err);
		free(overlay);
		return NULL;
	}

	return overlay;
}

void grate_overlay_free(struct grate_overlay *overlay)
{
	if (overlay)
		host1x_overlay_close(overlay->base);

	free(overlay);
}

void grate_overlay_show(struct grate_overlay *overlay,
			struct grate_framebuffer *fb, unsigned int x,
			unsigned int y, unsigned int width,
			unsigned int height, bool vsync)
{
	int err;

	grate_framebuffer_swap(fb);

	err = host1x_overlay_set(overlay->base, fb->front, x, y, width,
				 height, vsync);
	if (err < 0)
		grate_error("host1x_overlay_set() failed: %d\n", err);
}
