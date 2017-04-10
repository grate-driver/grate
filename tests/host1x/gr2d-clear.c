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

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "host1x.h"

int main(int argc, char *argv[])
{
	struct host1x_display *display = NULL;
	struct host1x_overlay *overlay = NULL;
	struct host1x_framebuffer *fb;
	unsigned int width = 256;
	unsigned int height = 256;
	struct host1x_gr2d *gr2d;
	struct host1x *host1x;
	int err;

	host1x = host1x_open(true, -1);
	if (!host1x) {
		fprintf(stderr, "host1x_open() failed\n");
		return 1;
	}

	display = host1x_get_display(host1x);
	if (display) {
		err = host1x_overlay_create(&overlay, display);
		if (err < 0) {
			fprintf(stderr, "overlay support missing\n");

			/*
			 * If overlay support is missing but we still have
			 * on-screen display support, make the framebuffer
			 * the same resolution as the display to make sure
			 * it can be properly displayed.
			 */

			err = host1x_display_get_resolution(display, &width,
							    &height);
			if (err < 0) {
				fprintf(stderr, "host1x_display_get_resolution() failed: %d\n", err);
				return 1;
			}
		}
	}

	gr2d = host1x_get_gr2d(host1x);
	if (!gr2d) {
		fprintf(stderr, "host1x_get_gr2d() failed\n");
		return 1;
	}

	fb = host1x_framebuffer_create(host1x, width, height,
				       PIX_BUF_FMT_RGBA8888,
				       PIX_BUF_LAYOUT_TILED_16x16, 0);
	if (!fb) {
		fprintf(stderr, "host1x_framebuffer_create() failed\n");
		return 1;
	}

	err = host1x_gr2d_clear(gr2d, fb->pixbuf, 0xFFFF00FF);
	if (err < 0) {
		fprintf(stderr, "host1x_gr2d_clear() failed: %d\n", err);
		return 1;
	}

	if (display) {
		if (overlay) {
			err = host1x_overlay_set(overlay, fb, 0, 0, width, height, false);
			if (err < 0)
				fprintf(stderr, "host1x_overlay_set() failed: %d\n", err);
			else
				sleep(5);

			host1x_overlay_close(overlay);
		} else {
			err = host1x_display_set(display, fb, false);
			if (err < 0)
				fprintf(stderr, "host1x_display_set() failed: %d\n", err);
			else
				sleep(5);
		}
	} else {
		host1x_framebuffer_save(host1x, fb, "test.png");
	}

	host1x_framebuffer_free(fb);
	host1x_close(host1x);

	return 0;
}
