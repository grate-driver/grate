/*
 * Copyright (c) GRATE-DRIVER project
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "x11-display.h"

#ifdef HAVE_XCB

#include <xcb/dri2.h>
#include <xf86drm.h>

static int dri2_display_set(struct host1x_display *displayp,
			    struct host1x_framebuffer *fb,
			    bool vsync, bool reflect_y)
{
	struct xcb_stuff *stuff = displayp->priv;
	xcb_dri2_swap_buffers_cookie_t cookie;
	xcb_dri2_swap_buffers_reply_t *rep;

	host1x_gr2d_surface_blit(stuff->host1x->gr2d,
				 fb->pixbuf, stuff->pixbuf,
				 0, 0,
				 fb->pixbuf->width,
				 fb->pixbuf->height,
				 0, 0,
				 stuff->pixbuf->width,
				-stuff->pixbuf->height);

	cookie = xcb_dri2_swap_buffers(stuff->disp, stuff->win,
				       0, 0,  0, 0,  0, 0);
	rep = xcb_dri2_swap_buffers_reply(stuff->disp, cookie, NULL);

	if (!rep)
		host1x_error("xcb_dri2_swap_buffers_reply failed\n");
	free(rep);

	return 0;
}

void dri2_display_create(struct xcb_stuff *stuff, struct host1x_display *disp)
{
	struct host1x_bo *dri_bo;
	xcb_dri2_attachment_t attachment = XCB_DRI2_ATTACHMENT_BUFFER_BACK_LEFT;
	xcb_dri2_authenticate_cookie_t authenticate_cookie;
	xcb_dri2_authenticate_reply_t *reply;
	xcb_dri2_connect_cookie_t connect_cookie;
	xcb_dri2_get_buffers_cookie_t get_buffers_cookie;
	xcb_dri2_get_buffers_reply_t *get_buffers_reply;
	xcb_dri2_connect_reply_t *connect_reply;
	xcb_dri2_dri2_buffer_t *dri2_buffer;
	xcb_void_cookie_t cookie;
	xcb_generic_error_t *err;
	drm_magic_t magic;
	unsigned format;

	if (drmGetMagic(stuff->drm_fd, &magic)) {
		host1x_error("drmGetMagic failed\n");
		return;
	}

	authenticate_cookie = xcb_dri2_authenticate(stuff->disp, stuff->win, magic);
	reply = xcb_dri2_authenticate_reply(stuff->disp, authenticate_cookie, NULL);
	if (!reply) {
		host1x_error("xcb_dri2_authenticate failed\n");
		return;
	}
	free(reply);

	host1x_info("authenticated\n");

	connect_cookie = xcb_dri2_connect(stuff->disp, stuff->win,
					  XCB_DRI2_DRIVER_TYPE_DRI);
	connect_reply = xcb_dri2_connect_reply(stuff->disp, connect_cookie, NULL);
	if (!connect_reply) {
		host1x_error("xcb_dri2_connect failed\n");
		free(reply);
		return;
	}

	host1x_info("connected to %s\n", xcb_dri2_connect_driver_name(connect_reply));
	free(connect_reply);

	cookie = xcb_dri2_create_drawable_checked(stuff->disp, stuff->win);
	err = xcb_request_check(stuff->disp, cookie);
	free(err);
	if (err) {
		host1x_error("xcb_dri2_create_drawable failed\n");
		return;
	}

	cookie = xcb_dri2_swap_interval_checked(stuff->disp, stuff->win, 1);
	err = xcb_request_check(stuff->disp, cookie);
	free(err);
	if (err) {
		host1x_error("xcb_dri2_swap_interval failed\n");
		return;
	}

	get_buffers_cookie = xcb_dri2_get_buffers(stuff->disp, stuff->win, 1, 1,
						  &attachment);
	get_buffers_reply = xcb_dri2_get_buffers_reply(stuff->disp,
						       get_buffers_cookie, NULL);
	if (!get_buffers_reply) {
		host1x_error("xcb_dri2_get_buffers failed\n");
		free(get_buffers_reply);
		return;
	}

	dri2_buffer = xcb_dri2_get_buffers_buffers(get_buffers_reply);
	if (!dri2_buffer) {
		host1x_error("xcb_dri2_get_buffers_buffers failed\n");
		free(get_buffers_reply);
		return;
	}

	host1x_info("got buffer %ux%u:%u:%u\n",
		    get_buffers_reply->width, get_buffers_reply->height,
		    dri2_buffer[0].pitch, dri2_buffer[0].cpp);

	switch (dri2_buffer[0].cpp) {
	case 4:
		format = PIX_BUF_FMT_BGRA8888;
		break;

	default:
		host1x_error("unsupported CPP %u\n", dri2_buffer[0].cpp);
		free(get_buffers_reply);
		return;
	}

	dri_bo = HOST1X_BO_IMPORT(stuff->host1x, dri2_buffer[0].name);
	if (!dri_bo) {
		free(get_buffers_reply);
		return;
	}

	stuff->pixbuf = calloc(1, sizeof(*stuff->pixbuf));
	if (!stuff->pixbuf) {
		host1x_bo_free(dri_bo);
		free(get_buffers_reply);
		return;
	}

	stuff->pixbuf->pitch = dri2_buffer[0].pitch;
	stuff->pixbuf->width = get_buffers_reply->width;
	stuff->pixbuf->height = get_buffers_reply->height;
	stuff->pixbuf->format = format;
	stuff->pixbuf->layout = PIX_BUF_LAYOUT_LINEAR;
	stuff->pixbuf->bo = dri_bo;

	free(get_buffers_reply);

	disp->set = dri2_display_set;

	host1x_info("DRI2 initialized\n");

	return;
}
#endif
