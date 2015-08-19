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

#include "nvhost-gr3d.h"
#include "host1x.h"

#define HOST1X_GR3D_TEST 0

#define HOST1X_GR3D_SCISSOR_HORIZONTAL	0x350
#define HOST1X_GR3D_SCISSOR_VERTICAL	0x351

static int host1x_gr3d_test(struct host1x_gr3d *gr3d)
{
	struct host1x_syncpt *syncpt = &gr3d->client->syncpts[0];
	struct host1x_pushbuf *pb;
	struct host1x_job *job;
	uint32_t fence;
	int err = 0;

	job = host1x_job_create(syncpt->id, 1);
	if (!job)
		return -ENOMEM;

	pb = host1x_job_append(job, gr3d->commands, 0);
	if (!pb) {
		host1x_job_free(job);
		return -ENOMEM;
	}

	host1x_pushbuf_push(pb, HOST1X_OPCODE_SETCL(0x000, 0x060, 0x00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x000, 0x0001));
	host1x_pushbuf_push(pb, 0x000001 << 8 | syncpt->id);

	err = host1x_client_submit(gr3d->client, job);
	if (err < 0) {
		host1x_job_free(job);
		return err;
	}

	host1x_job_free(job);

	err = host1x_client_flush(gr3d->client, &fence);
	if (err < 0)
		return err;

	err = host1x_client_wait(gr3d->client, fence, -1);
	if (err < 0)
		return err;

	return 0;
}

static int host1x_gr3d_reset(struct host1x_gr3d *gr3d)
{
	struct host1x_syncpt *syncpt = &gr3d->client->syncpts[0];
	const unsigned int num_attributes = 16;
	struct host1x_pushbuf *pb;
	struct host1x_job *job;
	unsigned int i;
	uint32_t fence;
	int err;

	job = host1x_job_create(syncpt->id, 2);
	if (!job)
		return -ENOMEM;

	pb = host1x_job_append(job, gr3d->commands, 0);
	if (!pb)
		return -ENOMEM;

	/*
	  Command Buffer:
	    mem: e5059be0, offset: 0, words: 1705
	    commands: 1705
	*/

	host1x_pushbuf_push(pb, HOST1X_OPCODE_SETCL(0x000, 0x060, 0x00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xb00, 0x0003));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x001, 0x0001));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x002, 0x0001));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x00c, 0x0002));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x00e, 0x0002));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x010, 0x0002));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x012, 0x0002));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x014, 0x0002));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);

	/* reset attribute modes */
	for (i = 0; i < num_attributes; i++) {
		host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x101 + i * 2, 0x001));
		host1x_pushbuf_push(pb, 0x00000000);
	}

	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x120, 0x0005));
	host1x_pushbuf_push(pb, 0x00000001);
	host1x_pushbuf_push(pb, 0x00000000);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x124, 0x0003));
	host1x_pushbuf_push(pb, 0x00000007);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x200, 0x0005));
	host1x_pushbuf_push(pb, 0x00000011);
	host1x_pushbuf_push(pb, 0x0000ffff);
	host1x_pushbuf_push(pb, 0x00ff0000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);

	/* Vertex processor constants (256 vectors of 4 elements each) */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x207, 0x0001));
	host1x_pushbuf_push(pb, 0x00000000);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x208, 256 * 4));

	for (i = 0; i < 256; i++) {
		host1x_pushbuf_push(pb, 0x00000000);
		host1x_pushbuf_push(pb, 0x00000000);
		host1x_pushbuf_push(pb, 0x00000000);
		host1x_pushbuf_push(pb, 0x00000000);
	}

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x209, 0x0003));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000003);

	/* XXX */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x300, 0x0040));

	for (i = 0; i < 64; i++)
		host1x_pushbuf_push(pb, 0x00000000);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x343, 0x0019));
	host1x_pushbuf_push(pb, 0xb8e00000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000105);
	host1x_pushbuf_push(pb, 0x3f000000);
	host1x_pushbuf_push(pb, 0x3f800000);
	host1x_pushbuf_push(pb, 0x3f800000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x3f000000);
	host1x_pushbuf_push(pb, 0x3f800000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000205);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x363, 0x0002));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x400, 0x0002));
	host1x_pushbuf_push(pb, 0x000007ff);
	host1x_pushbuf_push(pb, 0x000007ff);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x402, 0x0012));
	host1x_pushbuf_push(pb, 0x00000040);
	host1x_pushbuf_push(pb, 0x00000310);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x000fffff);
	host1x_pushbuf_push(pb, 0x00000001);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x1fff1fff);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000006);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000008);
	host1x_pushbuf_push(pb, 0x00000048);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x500, 0x0004));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000007);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);

	/* XXX */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x520, 0x0020));

	for (i = 0; i < 32; i++)
		host1x_pushbuf_push(pb, 0x00000000);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x540, 0x0001));
	host1x_pushbuf_push(pb, 0x00000000);

	/* XXX */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x541, 0x0040));

	for (i = 0; i < 64; i++)
		host1x_pushbuf_push(pb, 0x00000000);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x542, 0x0005));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x600, 0x0001));
	host1x_pushbuf_push(pb, 0x00000000);

	/* XXX */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x602, 0x0010));

	for (i = 0; i < 16; i++)
		host1x_pushbuf_push(pb, 0x00000000);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x603, 0x0001));
	host1x_pushbuf_push(pb, 0x00000000);

	/* XXX */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x604, 0x0080));

	for (i = 0; i < 128; i++)
		host1x_pushbuf_push(pb, 0x00000000);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x608, 0x0004));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x60e, 0x0001));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x700, 0x0001));
	host1x_pushbuf_push(pb, 0x00000000);

	/* XXX */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x701, 0x0040));

	for (i = 0; i < 64; i++)
		host1x_pushbuf_push(pb, 0x00000000);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x702, 0x0001));
	host1x_pushbuf_push(pb, 0x00000000);

	/* reset texture parameters */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x720, 0x0020));

	for (i = 0; i < 16; i++) {
		host1x_pushbuf_push(pb, 0x00000000);
		host1x_pushbuf_push(pb, 0x00000000);
	}

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x740, 0x0003));
	host1x_pushbuf_push(pb, 0x00000001);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);

	/* XXX */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x750, 0x0010));

	for (i = 0; i < 16; i++)
		host1x_pushbuf_push(pb, 0x00000000);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x800, 0x0001));
	host1x_pushbuf_push(pb, 0x00000000);

	/* XXX */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x802, 0x0010));

	for (i = 0; i < 16; i++)
		host1x_pushbuf_push(pb, 0x00000000);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x803, 0x0001));
	host1x_pushbuf_push(pb, 0x00000000);

	/*
	  Command Buffer:
	    mem: e5059be0, offset: 1aa4, words: 2048
	    commands: 2048
	*/
	/* write 256 64-bit fragment shader instructions (NOP?) */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x804, 0x0200));

	for (i = 0; i < 256; i++) {
		host1x_pushbuf_push(pb, 0x00000000);
		host1x_pushbuf_push(pb, 0x00000000);
	}

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x805, 0x0001));
	host1x_pushbuf_push(pb, 0x00000000);

	/* XXX */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x806, 0x0040));

	for (i = 0; i < 64; i++)
		host1x_pushbuf_push(pb, 0x00000000);

	/* write 32 floating point constants */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x820, 0x0020));

	for (i = 0; i < 32; i++)
		host1x_pushbuf_push(pb, 0x00000000);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x900, 0x0001));
	host1x_pushbuf_push(pb, 0x00000000);

	/* XXX */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x901, 0x0040));

	for (i = 0; i < 64; i++)
		host1x_pushbuf_push(pb, 0x00000000);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x902, 0x0002));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x907, 0x0003));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x90a, 0x0002));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xa00, 0x000d));
	host1x_pushbuf_push(pb, 0x00000e00);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, 0x00000020);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, 0x00000100);
	host1x_pushbuf_push(pb, 0x0f0f0f0f);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xb01, 0x0001));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xb04, 0x0001));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xb06, 0x0002));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xb08, 0x0002));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xb0a, 0x0009));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000); /* 0xb12 -- why aren't 0xb13 written? */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xb14, 0x0001));
	host1x_pushbuf_push(pb, 0x00000000);

	/* XXX render target parameters? */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xe10, 0x0010));

	for (i = 0; i < 16; i++)
		host1x_pushbuf_push(pb, 0x00000000);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xe20, 0x0003));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xe25, 5));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xe2c, 1));
	host1x_pushbuf_push(pb, 0x00000000);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xe40, 0x0002));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x205, 0x0001));
	host1x_pushbuf_push(pb, 0x00000000);

	/* write 256 128-bit vertex shader instructions (NOP?) */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x206, 256 * 4));

	for (i = 0; i < 256; i++) {
		host1x_pushbuf_push(pb, 0x001f9c6c);
		host1x_pushbuf_push(pb, 0x0000000d);
		host1x_pushbuf_push(pb, 0x8106c083);
		host1x_pushbuf_push(pb, 0x60401ffd);
	}

	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xb00, 0x0001));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe41, 0x0001));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xb00, 0x0002));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe41, 0x0003));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xb00, 0x0003));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xe10, 0x0010));
	host1x_pushbuf_push(pb, 0x0c00002c);
	host1x_pushbuf_push(pb, 0x08000019);
	host1x_pushbuf_push(pb, 0x0c00000c);
	host1x_pushbuf_push(pb, 0x0c000000);
	host1x_pushbuf_push(pb, 0x08000050);
	host1x_pushbuf_push(pb, 0x08000019);
	host1x_pushbuf_push(pb, 0x08000019);
	host1x_pushbuf_push(pb, 0x08000019);
	host1x_pushbuf_push(pb, 0x08000019);
	host1x_pushbuf_push(pb, 0x08000019);
	host1x_pushbuf_push(pb, 0x08000019);
	host1x_pushbuf_push(pb, 0x08000019);
	host1x_pushbuf_push(pb, 0x08000019);
	host1x_pushbuf_push(pb, 0x08000019);
	host1x_pushbuf_push(pb, 0x08000019);
	host1x_pushbuf_push(pb, 0x08000019);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe26, 0x0924));

	/* write 16 vertex attribute specifiers */
	for (i = 0; i < 16; ++i) {
		host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x101 + i * 2, 0x0001));
		host1x_pushbuf_push(pb, 0x0000104d);
	}

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x343, 0x0001));
	host1x_pushbuf_push(pb, 0xb8e08000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x902, 0x0001));
	host1x_pushbuf_push(pb, 0x00000003);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x344, 0x0002));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);

	/* XXX scissors setup? */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x350, 0x0002));
	host1x_pushbuf_push(pb, 0x00001fff);
	host1x_pushbuf_push(pb, 0x00001fff);

	/* XXX viewport setup? */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x352, 0x001b));
	host1x_pushbuf_push(pb, 0x00000000); /* offset: 0x00000352 */
	host1x_pushbuf_push(pb, 0x00000000); /* offset: 0x00000353 */
	host1x_pushbuf_push(pb, 0x41800000); /* offset: 0x00000355 */
	host1x_pushbuf_push(pb, 0x41800000); /* offset: 0x00000356 */

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x404, 0x0002));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x000fffff);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x354, 0x0009));
	host1x_pushbuf_push(pb, 0x3efffff0); /* offset: 0x00000354 */
	host1x_pushbuf_push(pb, 0x3efffff0); /* offset: 0x00000357 */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x358, 0x0003));
	host1x_pushbuf_push(pb, 0x3f800000);
	host1x_pushbuf_push(pb, 0x3f800000);
	host1x_pushbuf_push(pb, 0x3f800000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x343, 0x0001));
	host1x_pushbuf_push(pb, 0xb8e08000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x300, 0x0002));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x000, 0x0001));
	host1x_pushbuf_push(pb, 0x000002 << 8 | syncpt->id);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe21, 0x0140));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x347, 0x0001));
	host1x_pushbuf_push(pb, 0x3f800000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x346, 0x0001));
	host1x_pushbuf_push(pb, 0x00000001);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x348, 0x0004));
	host1x_pushbuf_push(pb, 0x3f800000);
	host1x_pushbuf_push(pb, 0x3f800000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x34c, 0x0002));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x3f800000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x35b, 0x0001));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xa02, 0x0006));
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, 0x00000020);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa00, 0x0e00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa08, 0x0100));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x403, 0x0710));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x40c, 0x0006));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xa02, 0x0006));
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, 0x00000020);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa00, 0x0e00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa08, 0x0100));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x402, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x40e, 0x0030));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xa02, 0x0006));
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, 0x00000020);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa00, 0x0e00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa08, 0x0100));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x40c, 0x0006));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0xe28, 0x0003));
	host1x_pushbuf_push(pb, 0x00000049); /* offset: 0x00000e28 */
	host1x_pushbuf_push(pb, 0x00000049); /* offset: 0x00000e29 */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xa02, 0x0006));
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, 0x00000020);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa00, 0x0e00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa08, 0x0100));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x400, 0x0003));
	host1x_pushbuf_push(pb, 0x000002ff); /* offset: 0x00000400 */
	host1x_pushbuf_push(pb, 0x000002ff); /* offset: 0x00000401 */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xa02, 0x0006));
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, 0x00000020);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa00, 0x0e00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa08, 0x0100));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x402, 0x0040));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0xe28, 0x0003));
	host1x_pushbuf_push(pb, 0x0001fe49); /* offset: 0x00000e28 */
	host1x_pushbuf_push(pb, 0x0001fe49); /* offset: 0x00000e29 */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xa02, 0x0006));
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, 0x00000020);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa00, 0x0e00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa08, 0x0100));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x402, 0x0048));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x40c, 0x0006));

	/*
	  Command Buffer:
	    mem: e5059be0, offset: 3aa4, words: 42
	    commands: 42
	*/

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xa02, 0x0006));
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, 0x00000020);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa00, 0x0e00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa08, 0x0100));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xa02, 0x0006));
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, 0x00000020);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa00, 0x0e00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa08, 0x0100));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x740, 0x0011));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xe20, 0x0001));
	host1x_pushbuf_push(pb, 0x58000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x503, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x545, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x501, 0x0001));
	host1x_pushbuf_push(pb, 0x0000000f);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe22, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x603, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x803, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x520, 0x0001));
	host1x_pushbuf_push(pb, 0x20006001);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x546, 0x0001));
	host1x_pushbuf_push(pb, 0x00000040);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe25, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa0a, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x544, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe27, 0x0001));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x000, 0x0001));
	host1x_pushbuf_push(pb, 0x000001 << 8 | syncpt->id);

	err = host1x_client_submit(gr3d->client, job);
	if (err < 0)
		return err;

	err = host1x_client_flush(gr3d->client, &fence);
	if (err < 0)
		return err;

	err = host1x_client_wait(gr3d->client, fence, -1);
	if (err < 0)
		return err;

	return 0;
}

void host1x_gr3d_viewport(struct host1x_pushbuf *pb, float x, float y,
			  float width, float height)
{
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x352, 0x1b));

	/* X bias */
	host1x_pushbuf_push_float(pb, x * 16.0f + width * 8.0f);

	/* Y bias */
	host1x_pushbuf_push_float(pb, y * 16.0f + height * 8.0f);

	/* X scale */
	host1x_pushbuf_push_float(pb, width * 8.0f);

	/* Y scale */
	host1x_pushbuf_push_float(pb, height * 8.0f);
}

void host1x_gr3d_line_width(struct host1x_pushbuf *pb, float width)
{
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x34d, 1));
	host1x_pushbuf_push_float(pb, width * 0.5f);
}

int host1x_gr3d_init(struct host1x *host1x, struct host1x_gr3d *gr3d)
{
	int err;

	gr3d->commands = host1x_bo_create(host1x, 32 * 4096, 2);
	if (!gr3d->commands)
		return -ENOMEM;

	err = host1x_bo_mmap(gr3d->commands, NULL);
	if (err < 0) {
		host1x_bo_free(gr3d->commands);
		return err;
	}

	gr3d->attributes = host1x_bo_create(host1x, 12 * 4096, 4);
	if (!gr3d->attributes) {
		host1x_bo_free(gr3d->commands);
		return -ENOMEM;
	}

	err = host1x_bo_mmap(gr3d->attributes, NULL);
	if (err < 0) {
		host1x_bo_free(gr3d->attributes);
		host1x_bo_free(gr3d->commands);
		return err;
	}

	if (HOST1X_GR3D_TEST) {
		err = host1x_gr3d_test(gr3d);
		if (err < 0) {
			fprintf(stderr, "host1x_gr3d_test() failed: %d\n",
				err);
			host1x_bo_free(gr3d->attributes);
			host1x_bo_free(gr3d->commands);
			return err;
		}
	}

	err = host1x_gr3d_reset(gr3d);
	if (err < 0) {
		host1x_bo_free(gr3d->attributes);
		host1x_bo_free(gr3d->commands);
		return err;
	}

	return 0;
}

void host1x_gr3d_exit(struct host1x_gr3d *gr3d)
{
	host1x_bo_free(gr3d->attributes);
	host1x_bo_free(gr3d->commands);
}

int host1x_gr3d_triangle(struct host1x_gr3d *gr3d,
			 struct host1x_framebuffer *fb)
{
	struct host1x_syncpt *syncpt = &gr3d->client->syncpts[0];
	float *attr = gr3d->attributes->ptr;
	struct host1x_pushbuf *pb;
	unsigned int depth = 32;
	struct host1x_job *job;
	uint32_t format, pitch;
	uint16_t *indices;
	uint32_t fence;
	int err, i;

	/* XXX: count syncpoint increments in command stream */
	job = host1x_job_create(syncpt->id, 9);
	if (!job)
		return -ENOMEM;

	/* colors */
	/* red */
	*attr++ = 1.0f;
	*attr++ = 0.0f;
	*attr++ = 0.0f;
	*attr++ = 1.0f;

	/* green */
	*attr++ = 0.0f;
	*attr++ = 1.0f;
	*attr++ = 0.0f;
	*attr++ = 1.0f;

	/* blue */
	*attr++ = 0.0f;
	*attr++ = 0.0f;
	*attr++ = 1.0f;
	*attr++ = 1.0f;

	/* geometry */
	*attr++ =  0.0f;
	*attr++ =  0.5f;
	*attr++ =  0.0f;
	*attr++ =  1.0f;

	*attr++ = -0.5f;
	*attr++ = -0.5f;
	*attr++ =  0.0f;
	*attr++ =  1.0f;

	*attr++ =  0.5f;
	*attr++ = -0.5f;
	*attr++ =  0.0f;
	*attr++ =  1.0f;

	indices = gr3d->attributes->ptr +
		  host1x_bo_get_offset(gr3d->attributes, attr);

	/* indices */
	*indices++ = 0x0000;
	*indices++ = 0x0001;
	*indices++ = 0x0002;

	err = host1x_bo_invalidate(gr3d->attributes, 0, 112);
	if (err < 0) {
		host1x_job_free(job);
		return err;
	}

	/*
	  Command Buffer:
	    mem: e462a7c0, offset: 3b4c, words: 103
	    commands: 103
	*/

	pb = host1x_job_append(job, gr3d->commands, 0);
	if (!pb) {
		host1x_job_free(job);
		return -ENOMEM;
	}

	host1x_pushbuf_push(pb, HOST1X_OPCODE_SETCL(0x000, 0x060, 0x00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x404, 2));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x000fffff);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x354, 9));
	host1x_pushbuf_push(pb, 0x3efffff0);
	host1x_pushbuf_push(pb, 0x3efffff0);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x740, 0x035));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe26, 0x779));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x346, 0x2));
	host1x_pushbuf_push(pb, 0x00001401);
	host1x_pushbuf_push(pb, 0x3f800000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x34c, 0x2));
	host1x_pushbuf_push(pb, 0x00000002);
	host1x_pushbuf_push(pb, 0x3f000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x000, 0x1));
	host1x_pushbuf_push(pb, 0x000002 << 8 | syncpt->id);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x000, 0x1));
	host1x_pushbuf_push(pb, 0x000002 << 8 | syncpt->id);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x352, 0x1b));

	host1x_pushbuf_push_float(pb, fb->width * 8.0f);
	host1x_pushbuf_push_float(pb, fb->height * 8.0f);
	host1x_pushbuf_push_float(pb, fb->width * 8.0f);
	host1x_pushbuf_push_float(pb, fb->height * 8.0f);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x358, 0x03));
	host1x_pushbuf_push(pb, 0x4376f000);
	host1x_pushbuf_push(pb, 0x4376f000);
	host1x_pushbuf_push(pb, 0x40dfae14);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x343, 0x01));
	host1x_pushbuf_push(pb, 0xb8e00000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x350, 0x02));
	host1x_pushbuf_push(pb, fb->width  & 0xffff);
	host1x_pushbuf_push(pb, fb->height & 0xffff);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xe11, 0x01));

	if (depth == 16) {
		format = HOST1X_GR3D_FORMAT_RGB565;
		pitch = fb->width * 2;
	} else {
		format = HOST1X_GR3D_FORMAT_RGBA8888;
		pitch = fb->width * 4;
	}

	host1x_pushbuf_push(pb, 0x04000000 | (pitch << 8) | format << 2 | 0x1);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x903, 0x01));
	host1x_pushbuf_push(pb, 0x00000002);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xe15, 0x07));
	host1x_pushbuf_push(pb, 0x08000001);
	host1x_pushbuf_push(pb, 0x08000001);
	host1x_pushbuf_push(pb, 0x08000001);
	host1x_pushbuf_push(pb, 0x08000001);
	host1x_pushbuf_push(pb, 0x08000001);
	host1x_pushbuf_push(pb, 0x08000001);
	host1x_pushbuf_push(pb, 0x08000001);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xe10, 0x01));
	host1x_pushbuf_push(pb, 0x0c000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xe13, 0x01));
	host1x_pushbuf_push(pb, 0x0c000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xe12, 0x01));
	host1x_pushbuf_push(pb, 0x0c000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x348, 0x04));
	host1x_pushbuf_push(pb, 0x3f800000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x3f800000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe27, 0x01));

	for (i = 0; i < 4; ++i) {
		host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xa02, 0x06));
		host1x_pushbuf_push(pb, 0x000001ff);
		host1x_pushbuf_push(pb, 0x000001ff);
		host1x_pushbuf_push(pb, 0x000001ff);
		host1x_pushbuf_push(pb, 0x00000030);
		host1x_pushbuf_push(pb, 0x00000020);
		host1x_pushbuf_push(pb, 0x00000030);
		host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa00, 0xe00));
		host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa08, 0x100));
	}

	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x000, 0x01));
	host1x_pushbuf_push(pb, 0x000002 << 8 | syncpt->id);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x000, 0x01));
	host1x_pushbuf_push(pb, 0x000002 << 8 | syncpt->id);

	/*
	  Command Buffer:
	    mem: e462ae40, offset: 0, words: 10
	    commands: 10
	*/

	/*
	 * This seems to write the following vertex shader:
	 *
	 * attribute vec4 position;
	 * attribute vec4 color;
	 * varying vec4 vcolor;
	 *
	 * void main()
	 * {
	 *     gl_Position = position;
	 *     vcolor = color;
	 * }
	 *
	 * Register 0x205 contains an ID, while register 0x206 is a FIFO that
	 * is used to upload vertex program code.
	 */

	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x205, 0x00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x206, 0x08));
	host1x_pushbuf_push(pb, 0x401f9c6c);
	host1x_pushbuf_push(pb, 0x0040000d);
	host1x_pushbuf_push(pb, 0x8106c083);
	host1x_pushbuf_push(pb, 0x6041ff80);
	host1x_pushbuf_push(pb, 0x401f9c6c);
	host1x_pushbuf_push(pb, 0x0040010d);
	host1x_pushbuf_push(pb, 0x8106c083);
	host1x_pushbuf_push(pb, 0x6041ff9d);

	/*
	  Command Buffer:
	    mem: e462a7c0, offset: 3ce8, words: 16
	    commands: 16
	*/

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x343, 0x01));
	host1x_pushbuf_push(pb, 0xb8e00000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x300, 0x02));
	host1x_pushbuf_push(pb, 0x00000008);
	host1x_pushbuf_push(pb, 0x0000fecd);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xe20, 0x01));
	host1x_pushbuf_push(pb, 0x58000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x503, 0x00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x545, 0x00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe22, 0x00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x603, 0x00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x803, 0x00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x520, 0x01));
	host1x_pushbuf_push(pb, 0x20006001);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x546, 0x01));
	host1x_pushbuf_push(pb, 0x00000040);

	/*
	  Command Buffer:
	    mem: e462aec0, offset: 10, words: 26
	    commands: 26
	*/

	/*
	 * This writes the fragment shader:
	 *
	 * precision mediump float;
	 * varying vec4 vcolor;
	 *
	 * void main()
	 * {
	 *     gl_FragColor = vcolor;
	 * }
	 */

	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x541, 0x01));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x500, 0x01));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x601, 0x01));
	host1x_pushbuf_push(pb, 0x00000001);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x604, 0x02));
	host1x_pushbuf_push(pb, 0x104e51ba);
	host1x_pushbuf_push(pb, 0x00408102);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x701, 0x01));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x801, 0x01));
	host1x_pushbuf_push(pb, 0x00000001);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x804, 0x08));
	host1x_pushbuf_push(pb, 0x0001c0c0);
	host1x_pushbuf_push(pb, 0x3f41f200);
	host1x_pushbuf_push(pb, 0x0001a080);
	host1x_pushbuf_push(pb, 0x3f41f200);
	host1x_pushbuf_push(pb, 0x00014000);
	host1x_pushbuf_push(pb, 0x3f41f200);
	host1x_pushbuf_push(pb, 0x00012040);
	host1x_pushbuf_push(pb, 0x3f41f200);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x806, 0x01));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x901, 0x01));
	host1x_pushbuf_push(pb, 0x00028005);

	/*
	  Command Buffer:
	    mem: e462a7c0, offset: 3d28, words: 66
	    commands: 66
	*/

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xa02, 0x06));
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, 0x00000020);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa00, 0xe00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa08, 0x100));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x40c, 0x06));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x120, 0x01));
	host1x_pushbuf_push(pb, 0x00030081);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x344, 0x02));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x000, 0x01));
	host1x_pushbuf_push(pb, 0x000001 << 8 | syncpt->id);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa00, 0xe01));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x00, 0x01));
	host1x_pushbuf_push(pb, 0x000002 << 8 | syncpt->id);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xe01, 0x01));
	/* relocate color render target */
	host1x_pushbuf_relocate(pb, fb->bo, 0, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
	/* vertex position attribute */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x100, 0x01));
	host1x_pushbuf_relocate(pb, gr3d->attributes, 0x30, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
	/* vertex color attribute */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x102, 0x01));
	host1x_pushbuf_relocate(pb, gr3d->attributes, 0, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
	/* primitive indices */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x121, 0x03));
	host1x_pushbuf_relocate(pb, gr3d->attributes, 0x60, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
	host1x_pushbuf_push(pb, 0xec000000);
	host1x_pushbuf_push(pb, 0x00200000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe27, 0x02));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x000, 0x01));
	host1x_pushbuf_push(pb, 0x000002 << 8 | syncpt->id);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x000, 0x01));
	host1x_pushbuf_push(pb, 0x000001 << 8 | syncpt->id);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x00, 0x01));
	host1x_pushbuf_push(pb, 0x000001 << 8 | syncpt->id);

	err = host1x_client_submit(gr3d->client, job);
	if (err < 0) {
		host1x_job_free(job);
		return err;
	}

	host1x_job_free(job);

	err = host1x_client_flush(gr3d->client, &fence);
	if (err < 0)
		return err;

	err = host1x_client_wait(gr3d->client, fence, -1);
	if (err < 0)
		return err;

	return 0;
}
