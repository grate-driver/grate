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

#include "host1x.h"
#include "host1x-private.h"

#define HOST1X_GR2D_TEST 0

static int host1x_gr2d_test(struct host1x_gr2d *gr2d)
{
	struct host1x_syncpt *syncpt = &gr2d->client->syncpts[0];
	struct host1x_pushbuf *pb;
	struct host1x_job *job;
	uint32_t fence;
	int err = 0;

	job = host1x_job_create(syncpt->id, 1);
	if (!job)
		return -ENOMEM;

	pb = host1x_job_append(job, gr2d->commands, 0);
	if (!pb) {
		host1x_job_free(job);
		return -ENOMEM;
	}

	host1x_pushbuf_push(pb, HOST1X_OPCODE_SETCL(0x000, 0x051, 0x00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x000, 0x0001));
	host1x_pushbuf_push(pb, 0x000001 << 8 | syncpt->id);

	err = host1x_client_submit(gr2d->client, job);
	if (err < 0) {
		host1x_job_free(job);
		return err;
	}

	host1x_job_free(job);

	err = host1x_client_flush(gr2d->client, &fence);
	if (err < 0)
		return err;

	err = host1x_client_wait(gr2d->client, fence, ~0u);
	if (err < 0)
		return err;

	return 0;
}

static int host1x_gr2d_reset(struct host1x_gr2d *gr2d)
{
	struct host1x_syncpt *syncpt = &gr2d->client->syncpts[0];
	struct host1x_pushbuf *pb;
	struct host1x_job *job;
	uint32_t fence;
	int err;

	job = host1x_job_create(syncpt->id, 1);
	if (!job)
		return -ENOMEM;

	pb = host1x_job_append(job, gr2d->commands, 0);
	if (!pb) {
		host1x_job_free(job);
		return -ENOMEM;
	}

	host1x_pushbuf_push(pb, HOST1X_OPCODE_SETCL(0x000, 0x051, 0x00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_EXTEND(0x00, 0x00000002));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x009, 0x0009));
	host1x_pushbuf_push(pb, 0x00000038);
	host1x_pushbuf_push(pb, 0x00000001);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x01c, 0x000b));
	host1x_pushbuf_push(pb, 0x00000808);
	host1x_pushbuf_push(pb, 0x00700000);
	host1x_pushbuf_push(pb, 0x18010000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x02b, 0x0009));
	host1x_pushbuf_relocate(pb, gr2d->scratch, 0, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
	host1x_pushbuf_push(pb, 0x00000020);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x031, 0x0005));
	host1x_pushbuf_relocate(pb, gr2d->scratch, 0, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
	host1x_pushbuf_push(pb, 0x00000020);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x046, 0x000d));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_relocate(pb, gr2d->scratch, 0, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
	host1x_pushbuf_relocate(pb, gr2d->scratch, 0, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x011, 0x0003));
	host1x_pushbuf_push(pb, 0x00001000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x013, 0x0003));
	host1x_pushbuf_push(pb, 0x00001000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x015, 0x0007));
	host1x_pushbuf_push(pb, 0x00080080);
	host1x_pushbuf_push(pb, 0x80000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x037, 0x0003));
	host1x_pushbuf_push(pb, 0x00000001);
	host1x_pushbuf_push(pb, 0x00000001);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_EXTEND(0x01, 0x00000002));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x000, 0x0001));
	host1x_pushbuf_push(pb, 0x000001 << 8 | syncpt->id);

	err = host1x_client_submit(gr2d->client, job);
	if (err < 0) {
		host1x_job_free(job);
		return err;
	}

	host1x_job_free(job);

	err = host1x_client_flush(gr2d->client, &fence);
	if (err < 0)
		return err;

	err = host1x_client_wait(gr2d->client, fence, ~0u);
	if (err < 0)
		return err;

	return 0;
}

int host1x_gr2d_init(struct host1x *host1x, struct host1x_gr2d *gr2d)
{
	int err;

	gr2d->commands = host1x_bo_create(host1x, 8 * 4096, 2);
	if (!gr2d->commands)
		return -ENOMEM;

	err = host1x_bo_mmap(gr2d->commands, NULL);
	if (err < 0)
		return err;

	gr2d->scratch = host1x_bo_create(host1x, 64, 3);
	if (!gr2d->scratch) {
		host1x_bo_free(gr2d->commands);
		return -ENOMEM;
	}

	if (HOST1X_GR2D_TEST) {
		err = host1x_gr2d_test(gr2d);
		if (err < 0) {
			fprintf(stderr, "host1x_gr2d_test() failed: %d\n",
				err);
			return err;
		}
	}

	err = host1x_gr2d_reset(gr2d);
	if (err < 0) {
		host1x_bo_free(gr2d->scratch);
		host1x_bo_free(gr2d->commands);
		return err;
	}

	return 0;
}

void host1x_gr2d_exit(struct host1x_gr2d *gr2d)
{
	host1x_bo_free(gr2d->commands);
	host1x_bo_free(gr2d->scratch);
}

int host1x_gr2d_clear(struct host1x_gr2d *gr2d,
		      struct host1x_pixelbuffer *pixbuf,
		      float red, float green, float blue, float alpha)
{
	struct host1x_syncpt *syncpt = &gr2d->client->syncpts[0];
	struct host1x_pushbuf *pb;
	struct host1x_job *job;
	uint32_t fence, color;
	uint32_t pitch;
	int err;

	if (PIX_BUF_FORMAT_BITS(pixbuf->format) == 16) {
		color = ((uint32_t)(red   * 31) << 11) |
			((uint32_t)(green * 63) <<  5) |
			((uint32_t)(blue  * 31) <<  0);
		pitch = pixbuf->width * 2;
	} else {
		color = ((uint32_t)(alpha * 255) << 24) |
			((uint32_t)(blue  * 255) << 16) |
			((uint32_t)(green * 255) <<  8) |
			((uint32_t)(red   * 255) <<  0);
		pitch = pixbuf->width * 4;
	}

	job = host1x_job_create(syncpt->id, 1);
	if (!job)
		return -ENOMEM;

	pb = host1x_job_append(job, gr2d->commands, 0);
	if (!pb) {
		host1x_job_free(job);
		return -ENOMEM;
	}

	host1x_pushbuf_push(pb, HOST1X_OPCODE_SETCL(0, 0x51, 0));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_EXTEND(0, 0x01));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x09, 9));
	host1x_pushbuf_push(pb, 0x0000003a);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x1e, 7));
	host1x_pushbuf_push(pb, 0x00000000);

	if (PIX_BUF_FORMAT_BITS(pixbuf->format) == 16)
		host1x_pushbuf_push(pb, 0x00010044); /* 16-bit depth */
	else
		host1x_pushbuf_push(pb, 0x00020044); /* 32-bit depth */

	host1x_pushbuf_push(pb, 0x000000cc);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x2b, 9));
	host1x_pushbuf_relocate(pb, pixbuf->bo, 0, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
	host1x_pushbuf_push(pb, pitch);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x35, 1));
	host1x_pushbuf_push(pb, color);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x46, 1));
	host1x_pushbuf_push(pb, 0x00100000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x38, 5));
	host1x_pushbuf_push(pb, pixbuf->height << 16 | pixbuf->width);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_EXTEND(1, 1));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x00, 1));
	host1x_pushbuf_push(pb, 0x000001 << 8 | syncpt->id);

	err = host1x_client_submit(gr2d->client, job);
	if (err < 0) {
		host1x_job_free(job);
		return err;
	}

	host1x_job_free(job);

	err = host1x_client_flush(gr2d->client, &fence);
	if (err < 0)
		return err;

	err = host1x_client_wait(gr2d->client, fence, ~0u);
	if (err < 0)
		return err;

	return 0;
}

int host1x_gr2d_blit(struct host1x_gr2d *gr2d, struct host1x_pixelbuffer *src,
		     struct host1x_pixelbuffer *dst, unsigned int sx,
		     unsigned int sy, unsigned int dx, unsigned int dy,
		     unsigned int width, unsigned int height)
{
	struct host1x_syncpt *syncpt = &gr2d->client->syncpts[0];
	struct host1x_pushbuf *pb;
	struct host1x_job *job;
	unsigned src_tiled = 0;
	unsigned dst_tiled = 0;
	uint32_t fence;
	int err;

	if (PIX_BUF_FORMAT_BYTES(src->format) !=
		PIX_BUF_FORMAT_BYTES(dst->format))
	{
		return -EINVAL;
	}

	switch (src->layout) {
	case PIX_BUF_LAYOUT_TILED_16x16:
		src_tiled = 1;
	case PIX_BUF_LAYOUT_LINEAR:
		break;
	default:
		return -EINVAL;
	}

	switch (dst->layout) {
	case PIX_BUF_LAYOUT_TILED_16x16:
		dst_tiled = 1;
	case PIX_BUF_LAYOUT_LINEAR:
		break;
	default:
		return -EINVAL;
	}

	job = host1x_job_create(syncpt->id, 1);
	if (!job)
		return -ENOMEM;

	pb = host1x_job_append(job, gr2d->commands, 0);
	if (!pb) {
		host1x_job_free(job);
		return -ENOMEM;
	}

	host1x_pushbuf_push(pb, HOST1X_OPCODE_SETCL(0, 0x51, 0));

	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x009, 0x9));
	host1x_pushbuf_push(pb, 0x0000003a); /* trigger */
	host1x_pushbuf_push(pb, 0x00000000); /* cmdsel */

	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x01e, 0x7));
	host1x_pushbuf_push(pb, 0x00000000); /* controlsecond */
	/*
	 * [20:20] source color depth (0: mono, 1: same)
	 * [17:16] destination color depth (0: 8 bpp, 1: 16 bpp, 2: 32 bpp)
	 */
	host1x_pushbuf_push(pb, /* controlmain */
			1 << 20 |
			(PIX_BUF_FORMAT_BYTES(dst->format) >> 1) << 16);
	host1x_pushbuf_push(pb, 0x000000cc); /* ropfade */

	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x046, 1));
	/*
	 * [20:20] destination write tile mode (0: linear, 1: tiled)
	 * [ 0: 0] tile mode Y/RGB (0: linear, 1: tiled)
	 */
	host1x_pushbuf_push(pb, dst_tiled << 20 | src_tiled); /* tilemode */

	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x02b, 0xe149));
	host1x_pushbuf_relocate(pb, dst->bo, 0, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef); /* dstba */
	host1x_pushbuf_push(pb, dst->pitch); /* dstst */
	host1x_pushbuf_relocate(pb, src->bo, 0, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef); /* srcba */
	host1x_pushbuf_push(pb, src->pitch); /* srcst */
	host1x_pushbuf_push(pb, height << 16 | width); /* dstsize */
	host1x_pushbuf_push(pb, sy << 16 | sx); /* srcps */
	host1x_pushbuf_push(pb, dy << 16 | dx); /* dstps */

	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x000, 1));
	host1x_pushbuf_push(pb, 0x000001 << 8 | syncpt->id);

	err = host1x_client_submit(gr2d->client, job);
	if (err < 0) {
		host1x_job_free(job);
		return err;
	}

	host1x_job_free(job);

	err = host1x_client_flush(gr2d->client, &fence);
	if (err < 0)
		return err;

	err = host1x_client_wait(gr2d->client, fence, ~0u);
	if (err < 0)
		return err;

	return 0;
}
