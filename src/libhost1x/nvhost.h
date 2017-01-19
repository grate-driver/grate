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

#ifndef GRATE_HOST1X_NVHOST_H
#define GRATE_HOST1X_NVHOST_H 1

#include "host1x.h"
#include "host1x-private.h"

#include "nvhost-nvmap.h"

struct nvhost_ctrl {
	int fd;
};

struct nvhost_ctrl *nvhost_ctrl_open(void);
void nvhost_ctrl_close(struct nvhost_ctrl *ctrl);

int nvhost_ctrl_read_syncpt(struct nvhost_ctrl *ctrl, uint32_t syncpt,
			    uint32_t *value);

int nvhost_ctrl_wait_syncpt(struct nvhost_ctrl *ctrl, uint32_t syncpt,
			    uint32_t thresh, uint32_t timeout);

struct nvhost_pushbuf_reloc {
	unsigned long source_offset;
	unsigned long target_handle;
	unsigned long target_offset;
	unsigned long shift;
};

struct nvhost_client {
	struct host1x_client base;
	struct nvhost_ctrl *ctrl;
	struct nvmap *nvmap;
	int fd;
};

static inline struct nvhost_client *to_nvhost_client(struct host1x_client *client)
{
	return container_of(client, struct nvhost_client, base);
}

int nvhost_client_init(struct nvhost_client *client, struct nvmap *nvmap,
		       struct nvhost_ctrl *ctrl, int fd);
void nvhost_client_exit(struct nvhost_client *client);

struct nvhost_bo {
	struct host1x_bo base;
	struct nvmap *nvmap;
	struct nvmap_handle *handle;
};

static inline struct nvhost_bo *to_nvhost_bo(struct host1x_bo *bo)
{
	return container_of(bo, struct nvhost_bo, base);
}

struct nvhost {
	struct host1x base;

	struct nvhost_ctrl *ctrl;
	struct nvhost_display *display;
	struct nvhost_gr2d *gr2d;
	struct nvhost_gr3d *gr3d;
	struct nvmap *nvmap;
};

static inline struct nvhost *to_nvhost(struct host1x *host1x)
{
	return container_of(host1x, struct nvhost, base);
}

#endif
