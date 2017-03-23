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
#include <stdio.h>
#include <unistd.h>

#include "gr2d.h"

static int nvhost_gr2d_init(struct nvhost_gr2d *gr2d)
{
	struct nvhost_pushbuf *pb;
	struct nvhost_job *job;
	uint32_t fence;
	int err;

	job = nvhost_job_create(gr2d->client.syncpt, 1);
	if (!job)
		return -ENOMEM;

	pb = nvhost_job_append(job, gr2d->buffer, 0);
	if (!pb) {
		nvhost_job_free(job);
		return -ENOMEM;
	}

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x000, 0x051, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x000, 0x052, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_EXTEND(0x00, 0x00000002));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_MASK(0x009, 0x0009));
	nvhost_pushbuf_push(pb, 0x00000038);
	nvhost_pushbuf_push(pb, 0x00000001);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_MASK(0x01c, 0x000b));
	nvhost_pushbuf_push(pb, 0x00000808);
	nvhost_pushbuf_push(pb, 0x00700000);
	nvhost_pushbuf_push(pb, 0x18010000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_MASK(0x02b, 0x0009));
	nvhost_pushbuf_relocate(pb, gr2d->scratch, 0, 0);
	nvhost_pushbuf_push(pb, 0xdeadbeef);
	nvhost_pushbuf_push(pb, 0x00000020);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_MASK(0x031, 0x0005));
	nvhost_pushbuf_relocate(pb, gr2d->scratch, 0, 0);
	nvhost_pushbuf_push(pb, 0xdeadbeef);
	nvhost_pushbuf_push(pb, 0x00000020);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_MASK(0x046, 0x000d));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_relocate(pb, gr2d->scratch, 0, 0);
	nvhost_pushbuf_push(pb, 0xdeadbeef);
	nvhost_pushbuf_relocate(pb, gr2d->scratch, 0, 0);
	nvhost_pushbuf_push(pb, 0xdeadbeef);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_MASK(0x011, 0x0003));
	nvhost_pushbuf_push(pb, 0x00001000);
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_MASK(0x013, 0x0003));
	nvhost_pushbuf_push(pb, 0x00001000);
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_MASK(0x015, 0x0007));
	nvhost_pushbuf_push(pb, 0x00080080);
	nvhost_pushbuf_push(pb, 0x80000000);
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_MASK(0x037, 0x0003));
	nvhost_pushbuf_push(pb, 0x00000001);
	nvhost_pushbuf_push(pb, 0x00000001);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_EXTEND(0x01, 0x00000002));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x000, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000112);

	err = nvhost_client_submit(&gr2d->client, job);
	if (err < 0) {
		nvhost_job_free(job);
		return err;
	}

	nvhost_job_free(job);

	err = nvhost_client_flush(&gr2d->client, &fence);
	if (err < 0) {
		return err;
	}

	printf("fence: %u\n", fence);

	err = nvhost_client_wait(&gr2d->client, fence, ~0u);
	if (err < 0) {
		return err;
	}

	return 0;
}

struct nvhost_gr2d *nvhost_gr2d_open(struct nvmap *nvmap,
				     struct nvhost_ctrl *ctrl)
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

	err = nvhost_client_init(&gr2d->client, nvmap, ctrl, 18, fd);
	if (err < 0) {
		free(gr2d);
		close(fd);
		return NULL;
	}

	gr2d->buffer = nvmap_handle_create(nvmap, 8 * 4096);
	if (!gr2d->buffer) {
		nvhost_client_exit(&gr2d->client);
		free(gr2d);
		return NULL;
	}

	err = nvmap_handle_alloc(nvmap, gr2d->buffer, 1 << 0,
	                         NVMAP_HANDLE_WRITE_COMBINE, 0x20);
	if (err < 0) {
		nvmap_handle_free(nvmap, gr2d->buffer);
		nvhost_client_exit(&gr2d->client);
		free(gr2d);
		return NULL;
	}

	err = nvmap_handle_mmap(nvmap, gr2d->buffer);
	if (err < 0) {
		nvmap_handle_free(nvmap, gr2d->buffer);
		nvhost_client_exit(&gr2d->client);
		free(gr2d);
		return NULL;
	}

	gr2d->scratch = nvmap_handle_create(nvmap, 64);
	if (!gr2d->scratch) {
		nvmap_handle_free(nvmap, gr2d->buffer);
		nvhost_client_exit(&gr2d->client);
		free(gr2d);
		return NULL;
	}

	err = nvmap_handle_alloc(nvmap, gr2d->scratch, 1 << 30,
	                         NVMAP_HANDLE_WRITE_COMBINE, 0x20);
	if (err < 0) {
		nvmap_handle_free(nvmap, gr2d->scratch);
		nvmap_handle_free(nvmap, gr2d->buffer);
		nvhost_client_exit(&gr2d->client);
		free(gr2d);
		return NULL;
	}

	err = nvhost_gr2d_init(gr2d);
	if (err < 0) {
		nvmap_handle_free(nvmap, gr2d->scratch);
		nvmap_handle_free(nvmap, gr2d->buffer);
		nvhost_client_exit(&gr2d->client);
		free(gr2d);
		return NULL;
	}

	return gr2d;
}

void nvhost_gr2d_close(struct nvhost_gr2d *gr2d)
{
	if (gr2d) {
		nvmap_handle_free(gr2d->client.nvmap, gr2d->buffer);
		nvhost_client_exit(&gr2d->client);
	}

	free(gr2d);
}

int nvhost_gr2d_clear(struct nvhost_gr2d *gr2d, struct nvmap_framebuffer *fb,
		      float red, float green, float blue, float alpha)
{
	struct nvhost_pushbuf *pb;
	struct nvhost_job *job;
	uint32_t fence, color;
	uint32_t pitch;
	int err;

	if (fb->depth == 16) {
		color = ((uint32_t)(red   * 31) << 11) |
			((uint32_t)(green * 63) <<  5) |
			((uint32_t)(blue  * 31) <<  0);
		pitch = fb->width * 2;
	} else {
		color = ((uint32_t)(alpha * 255) << 24) |
			((uint32_t)(blue  * 255) << 16) |
			((uint32_t)(green * 255) <<  8) |
			((uint32_t)(red   * 255) <<  0);
		pitch = fb->width * 4;
	}

	printf("color: %08x\n", color);

	job = nvhost_job_create(gr2d->client.syncpt, 1);
	if (!job)
		return -ENOMEM;

	pb = nvhost_job_append(job, gr2d->buffer, 0);
	if (!pb) {
		nvhost_job_free(job);
		return -ENOMEM;
	}

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0, 0x51, 0));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0, 0x51, 0));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_EXTEND(0, 0x01));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_MASK(0x09, 9));
	nvhost_pushbuf_push(pb, 0x0000003a);
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_MASK(0x1e, 7));
	nvhost_pushbuf_push(pb, 0x00000000);

	if (fb->depth == 16)
		nvhost_pushbuf_push(pb, 0x00010044); /* 16-bit depth */
	else
		nvhost_pushbuf_push(pb, 0x00020044); /* 32-bit depth */

	nvhost_pushbuf_push(pb, 0x000000cc);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_MASK(0x2b, 9));
	nvhost_pushbuf_relocate(pb, fb->handle, 0, 0);
	nvhost_pushbuf_push(pb, 0xdeadbeef);
	nvhost_pushbuf_push(pb, pitch);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x35, 1));
	nvhost_pushbuf_push(pb, color);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x46, 1));
	nvhost_pushbuf_push(pb, 0x00100000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_MASK(0x38, 5));
	nvhost_pushbuf_push(pb, fb->height << 16 | fb->width);
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_EXTEND(1, 1));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x00, 1));
	nvhost_pushbuf_push(pb, 0x00000112);

	err = nvhost_client_submit(&gr2d->client, job);
	if (err < 0) {
		nvhost_job_free(job);
		return err;
	}

	nvhost_job_free(job);

	err = nvhost_client_flush(&gr2d->client, &fence);
	if (err < 0) {
		return err;
	}

	printf("fence: %u\n", fence);

	err = nvhost_client_wait(&gr2d->client, fence, ~0u);
	if (err < 0) {
		return err;
	}

	return 0;
}
