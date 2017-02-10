/*
 * Copyright (c) 2012, 2013 Erik Faye-Lund
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
#include <stdio.h>
#include <unistd.h>

#include "nvhost.h"

struct nvhost_gr2d *nvhost_gr2d_open(struct nvhost *nvhost)
{
	struct nvhost_gr2d *gr2d;
	int err, fd;

	gr2d = calloc(1, sizeof(*gr2d));
	if (!gr2d)
		return NULL;

	fd = open("/dev/nvhost-gr2d", O_RDWR);
	if (fd < 0) {
		free(gr2d);
		return NULL;
	}

	err = nvhost_client_init(&gr2d->client, nvhost->nvmap, nvhost->ctrl,
				 fd);
	if (err < 0) {
		close(fd);
		free(gr2d);
		return NULL;
	}

	gr2d->base.client = &gr2d->client.base;

	err = host1x_gr2d_init(&nvhost->base, &gr2d->base);
	if (err < 0) {
		nvhost_client_exit(&gr2d->client);
		free(gr2d);
		return NULL;
	}

	return gr2d;
}

void nvhost_gr2d_close(struct nvhost_gr2d *gr2d)
{
	if (gr2d) {
		nvhost_client_exit(&gr2d->client);
		host1x_gr2d_exit(&gr2d->base);
	}

	free(gr2d);
}
