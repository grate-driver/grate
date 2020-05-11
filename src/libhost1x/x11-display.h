/*
 * Copyright (c) 2017 Dmitry Osipenko <digetx@gmail.com>
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

#ifndef GRATE_HOST1X_X11_DISPLAY_H
#define GRATE_HOST1X_X11_DISPLAY_H 1

#include "host1x-private.h"

#ifdef HAVE_XCB

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_image.h>

struct xcb_stuff {
	struct host1x_pixelbuffer *pixbuf;
	struct host1x *host1x;
	xcb_connection_t *disp;
	xcb_gcontext_t gc;
	xcb_window_t win;
	xcb_image_t *img;
	int drm_fd;
};

void dri2_display_create(struct xcb_stuff *stuff, struct host1x_display *disp);
#endif

int x11_display_create(struct host1x *host1x, struct host1x_display *base,
		       int drm_fd);

#endif
