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

#include "config.h"
#include "x11-display.h"

#ifdef HAVE_XCB

#include <errno.h>
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>

#define WIN_WIDTH	720
#define WIN_HEIGHT	576

struct xcb_stuff {
	struct host1x_pixelbuffer *pixbuf;
	struct host1x *host1x;
	xcb_connection_t *disp;
	xcb_gcontext_t gc;
	xcb_window_t win;
	xcb_image_t *img;
};

static int x11_overlay_create(struct host1x_display *display,
			      struct host1x_overlay **overlayp)
{
	return -1;
}

static void * mmap_fb(struct host1x_pixelbuffer *pixbuf)
{
	struct host1x_bo *bo = pixbuf->bo;
	void *mmap;
	int err;

	err = HOST1X_BO_MMAP(bo, &mmap);
	if (err < 0)
		return NULL;

	err = HOST1X_BO_INVALIDATE(bo, bo->offset, bo->size);
	if (err < 0)
		return NULL;

	return mmap + bo->offset;
}

static void *fbdata(struct xcb_stuff *stuff, struct host1x_framebuffer *fb)
{
	struct host1x_pixelbuffer *pixbuf = fb->pixbuf;
	int err;

	if (!stuff->pixbuf)
		stuff->pixbuf = host1x_pixelbuffer_create(
						stuff->host1x,
						WIN_WIDTH,
						WIN_HEIGHT,
						WIN_WIDTH * 4,
						PIX_BUF_FMT_RGBA8888,
						PIX_BUF_LAYOUT_LINEAR);
	if (!stuff->pixbuf)
		return NULL;

	err = host1x_gr2d_blit(stuff->host1x->gr2d,
			       pixbuf, stuff->pixbuf,
			       0, 0, 0, 0,
			       WIN_WIDTH, -WIN_HEIGHT);
	if (err < 0)
		return NULL;

	return mmap_fb(stuff->pixbuf);
}

static int x11_display_set(struct host1x_display *displayp,
			   struct host1x_framebuffer *fb, bool vsync)
{
	struct xcb_stuff *stuff = displayp->priv;

	stuff->img->data = fbdata(stuff, fb);
	if (!stuff->img->data)
		return -1;

	xcb_image_put(stuff->disp, stuff->win, stuff->gc, stuff->img, 0, 0, 0);
	xcb_flush(stuff->disp);

	return 0;
}

int x11_display_create(struct host1x *host1x, struct host1x_display *base)
{
	struct xcb_stuff *stuff;
	xcb_void_cookie_t cookie;
	xcb_screen_t *screen;

	stuff = calloc(1, sizeof(*stuff));
	if (!stuff)
		return -ENOMEM;

	stuff->host1x = host1x;

	stuff->img = xcb_image_create(WIN_WIDTH, WIN_HEIGHT,
				      XCB_IMAGE_FORMAT_Z_PIXMAP,
				      8, 24, 32, 0,
				      XCB_IMAGE_ORDER_LSB_FIRST,
				      XCB_IMAGE_ORDER_LSB_FIRST,
				      0, 0, NULL);
	if (!stuff->img) {
		host1x_error("Failed to create xcb_image\n");
		free(stuff);
		return -ENOMEM;
	}

	stuff->disp = xcb_connect(NULL,NULL);
	if (xcb_connection_has_error(stuff->disp)) {
		host1x_error("Cannot open display\n");
		free(stuff);
		return -ENODEV;
	}

	screen = xcb_setup_roots_iterator( xcb_get_setup(stuff->disp) ).data;

	stuff->win = xcb_generate_id(stuff->disp);
	cookie = xcb_create_window_checked(stuff->disp, screen->root_depth,
					   stuff->win, screen->root, 0, 0,
					   WIN_WIDTH, WIN_HEIGHT, 1,
					   XCB_WINDOW_CLASS_INPUT_OUTPUT,
					   screen->root_visual, 0, NULL);
	if (xcb_request_check(stuff->disp, cookie)) {
		host1x_error("Cannot create window\n");
		xcb_disconnect(stuff->disp);
		free(stuff);
		return -ENODEV;
	}

	cookie = xcb_map_window_checked(stuff->disp, stuff->win);
	if (xcb_request_check(stuff->disp, cookie)) {
		host1x_error("Cannot map window\n");
		xcb_disconnect(stuff->disp);
		free(stuff);
		return -ENODEV;
	}

	stuff->gc = xcb_generate_id(stuff->disp);
	xcb_create_gc(stuff->disp, stuff->gc, stuff->win, 0, NULL);

	base->width = WIN_WIDTH;
	base->height = WIN_HEIGHT;
	base->create_overlay = x11_overlay_create;
	base->set = x11_display_set;
	base->priv = stuff;

	return 0;
}
#else
int x11_display_create(struct host1x *host1x, struct host1x_display *base)
{
	return -1;
}
#endif
