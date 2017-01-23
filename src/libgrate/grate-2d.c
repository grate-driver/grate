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

#include "../libhost1x/host1x-private.h"
#include "libgrate-private.h"

#include "grate.h"

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

	if (!grate->fb) {
		grate_error("no framebuffer bound to state\n");
		return;
	}

	err = host1x_gr2d_clear(gr2d, grate->fb->back, clear->r, clear->g,
				clear->b, clear->a);
	if (err < 0)
		grate_error("host1x_gr2d_clear() failed: %d\n", err);
}
