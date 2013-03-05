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
	unsigned int width = 32, height = 32;
	struct host1x_framebuffer *fb;
	struct host1x_gr2d *gr2d;
	struct host1x *host1x;
	int err;

	host1x = host1x_open();
	if (!host1x) {
		return 1;
	}

	gr2d = host1x_get_gr2d(host1x);
	if (!gr2d) {
		return 1;
	}

	fb = host1x_framebuffer_create(host1x, width, height, 32, 0);
	if (!fb) {
		return 1;
	}

	err = host1x_gr2d_clear(gr2d, fb, 1.0f, 0.0f, 1.0f, 1.0f);
	if (err < 0) {
		return 1;
	}

	host1x_framebuffer_save(fb, "test.png");
	host1x_framebuffer_free(fb);
	host1x_close(host1x);

	return 0;
}
