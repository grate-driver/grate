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

#include "gr3d.h"

#define NVHOST_GR3D_FORMAT_RGB565	0x6
#define NVHOST_GR3D_FORMAT_RGBA8888	0xd

#define NVHOST_GR3D_SCISSOR_HORIZONTAL	0x350
#define NVHOST_GR3D_SCISSOR_VERTICAL	0x351

static int nvhost_gr3d_init(struct nvhost_gr3d *gr3d)
{
	const unsigned int num_attributes = 16;
	struct nvhost_pushbuf *pb;
	struct nvhost_job *job;
	unsigned int i;
	uint32_t fence;
	int err;

	job = nvhost_job_create(gr3d->client.syncpt, 2);
	if (!job)
		return -ENOMEM;

	pb = nvhost_job_append(job, gr3d->buffer, 0);
	if (!pb)
		return -ENOMEM;

	/*
	  Command Buffer:
	    mem: e5059be0, offset: 0, words: 1705
	    commands: 1705
	*/

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x000, 0x060, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xb00, 0x0003));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x001, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x002, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x00c, 0x0002));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x00e, 0x0002));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x010, 0x0002));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x012, 0x0002));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x014, 0x0002));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, 0x00000000);

	/* reset attribute pointers and modes */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x100, 0x0020));

	for (i = 0; i < num_attributes; i++) {
		nvhost_pushbuf_push(pb, 0x00000000);
		nvhost_pushbuf_push(pb, 0x00000000);
	}

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x120, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000001);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x121, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x122, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x124, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000007);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x125, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x126, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x200, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000011);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x201, 0x0001));
	nvhost_pushbuf_push(pb, 0x0000ffff);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x202, 0x0001));
	nvhost_pushbuf_push(pb, 0x00ff0000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x203, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x204, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x207, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);

	/* Vertex processor constants (256 vectors of 4 elements each) */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x208, 256 * 4));

	for (i = 0; i < 256; i++) {
		nvhost_pushbuf_push(pb, 0x00000000);
		nvhost_pushbuf_push(pb, 0x00000000);
		nvhost_pushbuf_push(pb, 0x00000000);
		nvhost_pushbuf_push(pb, 0x00000000);
	}

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x209, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x20a, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x20b, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000003);

	/* XXX */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x300, 0x0040));

	for (i = 0; i < 64; i++)
		nvhost_pushbuf_push(pb, 0x00000000);

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x343, 0x0001));
	nvhost_pushbuf_push(pb, 0xb8e00000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x344, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x345, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x346, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000105);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x347, 0x0001));
	nvhost_pushbuf_push(pb, 0x3f000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x348, 0x0001));
	nvhost_pushbuf_push(pb, 0x3f800000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x349, 0x0001));
	nvhost_pushbuf_push(pb, 0x3f800000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x34a, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x34b, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x34c, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x34d, 0x0001));
	nvhost_pushbuf_push(pb, 0x3f000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x34e, 0x0001));
	nvhost_pushbuf_push(pb, 0x3f800000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x34f, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x350, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x351, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x352, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x353, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x354, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x355, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x356, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x357, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x358, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x359, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x35a, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x35b, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000205);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x363, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x364, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x400, 0x0002));
	nvhost_pushbuf_push(pb, 0x000007ff);
	nvhost_pushbuf_push(pb, 0x000007ff);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x402, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000040);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x403, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000310);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x404, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x405, 0x0001));
	nvhost_pushbuf_push(pb, 0x000fffff);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x406, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000001);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x407, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x408, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x409, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x40a, 0x0001));
	nvhost_pushbuf_push(pb, 0x1fff1fff);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x40b, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x40c, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000006);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x40d, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x40e, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000008);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x40f, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000048);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x410, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x411, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x412, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x413, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x500, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x501, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000007);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x502, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x503, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);

	/* XXX */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x520, 0x0020));

	for (i = 0; i < 32; i++)
		nvhost_pushbuf_push(pb, 0x00000000);

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x540, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);

	/* XXX */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x541, 0x0040));

	for (i = 0; i < 64; i++)
		nvhost_pushbuf_push(pb, 0x00000000);

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x542, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x543, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x544, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x545, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x546, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x600, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);

	/* XXX */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x602, 0x0010));

	for (i = 0; i < 16; i++)
		nvhost_pushbuf_push(pb, 0x00000000);

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x603, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);

	/* XXX */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x604, 0x0080));

	for (i = 0; i < 128; i++)
		nvhost_pushbuf_push(pb, 0x00000000);

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x608, 0x0004));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x60e, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x700, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);

	/* XXX */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x701, 0x0040));

	for (i = 0; i < 64; i++)
		nvhost_pushbuf_push(pb, 0x00000000);

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x702, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);

	/* reset texture pointers */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x710, 0x0010));

	for (i = 0; i < 16; i++)
		nvhost_pushbuf_push(pb, 0x00000000);

	/* reset texture parameters */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x720, 0x0020));

	for (i = 0; i < 16; i++) {
		nvhost_pushbuf_push(pb, 0x00000000);
		nvhost_pushbuf_push(pb, 0x00000000);
	}

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x740, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000001);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x741, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x742, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);

	/* XXX */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x750, 0x0010));

	for (i = 0; i < 16; i++)
		nvhost_pushbuf_push(pb, 0x00000000);

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x800, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);

	/* XXX */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x802, 0x0010));

	for (i = 0; i < 16; i++)
		nvhost_pushbuf_push(pb, 0x00000000);

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x803, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);

	/*
	  Command Buffer:
	    mem: e5059be0, offset: 1aa4, words: 2048
	    commands: 2048
	*/
	/* write 256 64-bit fragment shader instructions (NOP?) */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x804, 0x0200));

	for (i = 0; i < 256; i++) {
		nvhost_pushbuf_push(pb, 0x00000000);
		nvhost_pushbuf_push(pb, 0x00000000);
	}

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x805, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);

	/* XXX */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x806, 0x0040));

	for (i = 0; i < 64; i++)
		nvhost_pushbuf_push(pb, 0x00000000);

	/* write 32 floating point constants */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x820, 0x0020));

	for (i = 0; i < 32; i++)
		nvhost_pushbuf_push(pb, 0x00000000);

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x900, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);

	/* XXX */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x901, 0x0040));

	for (i = 0; i < 64; i++)
		nvhost_pushbuf_push(pb, 0x00000000);

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x902, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x903, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x904, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x907, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x908, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x909, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x90a, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x90b, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa00, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000e00);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa01, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa02, 0x0001));
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa03, 0x0001));
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa04, 0x0001));
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa05, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa06, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000020);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa07, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa08, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000100);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa09, 0x0001));
	nvhost_pushbuf_push(pb, 0x0f0f0f0f);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa0a, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa0b, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa0c, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xb01, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xb04, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xb06, 0x0002));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xb08, 0x0002));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xb0a, 0x0002));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xb0c, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xb0d, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xb0e, 0x0002));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xb10, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xb11, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xb12, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xb14, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);

	/* XXX render target pointers? */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe00, 0x0010));

	for (i = 0; i < 16; i++)
		nvhost_pushbuf_push(pb, 0x00000000);

	/* XXX render target parameters? */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe10, 0x0010));

	for (i = 0; i < 16; i++)
		nvhost_pushbuf_push(pb, 0x00000000);

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe20, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe21, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe22, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe25, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe26, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe27, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe28, 0x0002));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe2a, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe2b, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);

	/* XXX */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe30, 0x0010));

	for (i = 0; i < 16; i++)
		nvhost_pushbuf_push(pb, 0x00000000);

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe40, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe41, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x205, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);

	/* write 256 128-bit vertex shader instructions (NOP?) */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x206, 256 * 4));

	for (i = 0; i < 256; i++) {
		nvhost_pushbuf_push(pb, 0x001f9c6c);
		nvhost_pushbuf_push(pb, 0x0000000d);
		nvhost_pushbuf_push(pb, 0x8106c083);
		nvhost_pushbuf_push(pb, 0x60401ffd);
	}

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xb00, 0x0001));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xe41, 0x0001));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xb00, 0x0002));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xe41, 0x0003));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xb00, 0x0003));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe10, 0x0001));
	nvhost_pushbuf_push(pb, 0x0c00002c);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe11, 0x0001));
	nvhost_pushbuf_push(pb, 0x08000019);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe12, 0x0001));
	nvhost_pushbuf_push(pb, 0x0c00000c);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe13, 0x0001));
	nvhost_pushbuf_push(pb, 0x0c000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe14, 0x0001));
	nvhost_pushbuf_push(pb, 0x08000050);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe15, 0x0001));
	nvhost_pushbuf_push(pb, 0x08000019);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe16, 0x0001));
	nvhost_pushbuf_push(pb, 0x08000019);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe17, 0x0001));
	nvhost_pushbuf_push(pb, 0x08000019);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe18, 0x0001));
	nvhost_pushbuf_push(pb, 0x08000019);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe19, 0x0001));
	nvhost_pushbuf_push(pb, 0x08000019);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe1a, 0x0001));
	nvhost_pushbuf_push(pb, 0x08000019);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe1b, 0x0001));
	nvhost_pushbuf_push(pb, 0x08000019);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe1c, 0x0001));
	nvhost_pushbuf_push(pb, 0x08000019);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe1d, 0x0001));
	nvhost_pushbuf_push(pb, 0x08000019);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe1e, 0x0001));
	nvhost_pushbuf_push(pb, 0x08000019);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe1f, 0x0001));
	nvhost_pushbuf_push(pb, 0x08000019);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xe26, 0x0924));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x101, 0x0001));
	nvhost_pushbuf_push(pb, 0x0000104d);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x103, 0x0001));
	nvhost_pushbuf_push(pb, 0x0000104d);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x105, 0x0001));
	nvhost_pushbuf_push(pb, 0x0000104d);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x107, 0x0001));
	nvhost_pushbuf_push(pb, 0x0000104d);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x109, 0x0001));
	nvhost_pushbuf_push(pb, 0x0000104d);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x10b, 0x0001));
	nvhost_pushbuf_push(pb, 0x0000104d);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x10d, 0x0001));
	nvhost_pushbuf_push(pb, 0x0000104d);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x10f, 0x0001));
	nvhost_pushbuf_push(pb, 0x0000104d);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x111, 0x0001));
	nvhost_pushbuf_push(pb, 0x0000104d);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x113, 0x0001));
	nvhost_pushbuf_push(pb, 0x0000104d);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x115, 0x0001));
	nvhost_pushbuf_push(pb, 0x0000104d);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x117, 0x0001));
	nvhost_pushbuf_push(pb, 0x0000104d);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x119, 0x0001));
	nvhost_pushbuf_push(pb, 0x0000104d);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x11b, 0x0001));
	nvhost_pushbuf_push(pb, 0x0000104d);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x11d, 0x0001));
	nvhost_pushbuf_push(pb, 0x0000104d);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x11f, 0x0001));
	nvhost_pushbuf_push(pb, 0x0000104d);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x343, 0x0001));
	nvhost_pushbuf_push(pb, 0xb8e08000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x902, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000003);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x344, 0x0002));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, 0x00000000);

	/* XXX scissors setup? */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x350, 0x0002));
	nvhost_pushbuf_push(pb, 0x00001fff);
	nvhost_pushbuf_push(pb, 0x00001fff);

	/* XXX viewport setup? */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_MASK(0x352, 0x001b));
	nvhost_pushbuf_push(pb, 0x00000000); /* offset: 0x00000352 */
	nvhost_pushbuf_push(pb, 0x00000000); /* offset: 0x00000353 */
	nvhost_pushbuf_push(pb, 0x41800000); /* offset: 0x00000355 */
	nvhost_pushbuf_push(pb, 0x41800000); /* offset: 0x00000356 */

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x404, 0x0002));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, 0x000fffff);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_MASK(0x354, 0x0009));
	nvhost_pushbuf_push(pb, 0x3efffff0); /* offset: 0x00000354 */
	nvhost_pushbuf_push(pb, 0x3efffff0); /* offset: 0x00000357 */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x358, 0x0003));
	nvhost_pushbuf_push(pb, 0x3f800000);
	nvhost_pushbuf_push(pb, 0x3f800000);
	nvhost_pushbuf_push(pb, 0x3f800000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x343, 0x0001));
	nvhost_pushbuf_push(pb, 0xb8e08000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x300, 0x0002));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x000, 0x060, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x000, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000216);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x000, 0x001, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x009, 0x0001));
	nvhost_pushbuf_push(pb, 0x16030001);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x000, 0x060, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xe21, 0x0140));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x347, 0x0001));
	nvhost_pushbuf_push(pb, 0x3f800000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x346, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000001);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x348, 0x0004));
	nvhost_pushbuf_push(pb, 0x3f800000);
	nvhost_pushbuf_push(pb, 0x3f800000);
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x34c, 0x0002));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, 0x3f800000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x35b, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa02, 0x0006));
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, 0x00000020);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa00, 0x0e00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa08, 0x0100));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0x403, 0x0710));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0x40c, 0x0006));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa02, 0x0006));
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, 0x00000020);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa00, 0x0e00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa08, 0x0100));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0x402, 0x0000));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0x40e, 0x0030));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa02, 0x0006));
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, 0x00000020);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa00, 0x0e00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa08, 0x0100));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0x40c, 0x0006));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_MASK(0xe28, 0x0003));
	nvhost_pushbuf_push(pb, 0x00000049); /* offset: 0x00000e28 */
	nvhost_pushbuf_push(pb, 0x00000049); /* offset: 0x00000e29 */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa02, 0x0006));
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, 0x00000020);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa00, 0x0e00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa08, 0x0100));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_MASK(0x400, 0x0003));
	nvhost_pushbuf_push(pb, 0x000002ff); /* offset: 0x00000400 */
	nvhost_pushbuf_push(pb, 0x000002ff); /* offset: 0x00000401 */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa02, 0x0006));
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, 0x00000020);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa00, 0x0e00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa08, 0x0100));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0x402, 0x0040));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_MASK(0xe28, 0x0003));
	nvhost_pushbuf_push(pb, 0x0001fe49); /* offset: 0x00000e28 */
	nvhost_pushbuf_push(pb, 0x0001fe49); /* offset: 0x00000e29 */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa02, 0x0006));
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, 0x00000020);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa00, 0x0e00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa08, 0x0100));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0x402, 0x0048));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0x40c, 0x0006));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x000, 0x001, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x00c, 0x0001));
	nvhost_pushbuf_push(pb, 0x03000001);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x000, 0x060, 0x00));

	/*
	  Command Buffer:
	    mem: e5059be0, offset: 3aa4, words: 42
	    commands: 42
	*/

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa02, 0x0006));
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, 0x00000020);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa00, 0x0e00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa08, 0x0100));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa02, 0x0006));
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, 0x00000020);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa00, 0x0e00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa08, 0x0100));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0x740, 0x0011));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe20, 0x0001));
	nvhost_pushbuf_push(pb, 0x58000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0x503, 0x0000));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0x545, 0x0000));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x501, 0x0001));
	nvhost_pushbuf_push(pb, 0x0000000f);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xe22, 0x0000));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0x603, 0x0000));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0x803, 0x0000));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x520, 0x0001));
	nvhost_pushbuf_push(pb, 0x20006001);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x546, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000040);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xe25, 0x0000));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa0a, 0x0000));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0x544, 0x0000));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xe27, 0x0001));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x000, 0x0001));
	nvhost_pushbuf_push(pb, 0x00000116);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x000, 0x001, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x00c, 0x0001));
	nvhost_pushbuf_push(pb, 0x03000001);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x000, 0x060, 0x00));

	err = nvhost_client_submit(&gr3d->client, job);
	if (err < 0)
		return err;

	err = nvhost_client_flush(&gr3d->client, &fence);
	if (err < 0)
		return err;

	printf("fence: %u\n", fence);

	err = nvhost_client_wait(&gr3d->client, fence, -1);
	if (err < 0)
		return err;

	return 0;
}

struct nvhost_gr3d *nvhost_gr3d_open(struct nvmap *nvmap,
				     struct nvhost_ctrl *ctrl)
{
	struct nvhost_gr3d *gr3d;
	int err, fd;

	gr3d = calloc(1, sizeof(*gr3d));
	if (!gr3d)
		return NULL;

	fd = open("/dev/nvhost-gr3d", O_RDWR);
	if (fd < 0) {
		free(gr3d);
		return NULL;
	}

	err = nvhost_client_init(&gr3d->client, nvmap, ctrl, 22, fd);
	if (err < 0) {
		close(fd);
		free(gr3d);
		return NULL;
	}

	gr3d->buffer = nvmap_handle_create(nvmap, 32 * 4096);
	if (!gr3d->buffer) {
		nvhost_client_exit(&gr3d->client);
		free(gr3d);
		return NULL;
	}

	err = nvmap_handle_alloc(nvmap, gr3d->buffer, 1 << 0, 0x0a000001, 0x20);
	if (err < 0) {
		nvmap_handle_free(nvmap, gr3d->buffer);
		nvhost_client_exit(&gr3d->client);
		free(gr3d);
		return NULL;
	}

	err = nvmap_handle_mmap(nvmap, gr3d->buffer);
	if (err < 0) {
		nvmap_handle_free(nvmap, gr3d->buffer);
		nvhost_client_exit(&gr3d->client);
		free(gr3d);
		return NULL;
	}

	gr3d->attributes = nvmap_handle_create(nvmap, 12 * 4096);
	if (!gr3d->attributes) {
		nvmap_handle_free(nvmap, gr3d->buffer);
		nvhost_client_exit(&gr3d->client);
		free(gr3d);
		return NULL;
	}

	err = nvmap_handle_alloc(nvmap, gr3d->attributes, 1 << 30, 0x3d000001,
				 0x4);
	if (err < 0) {
		nvmap_handle_free(nvmap, gr3d->attributes);
		nvmap_handle_free(nvmap, gr3d->buffer);
		nvhost_client_exit(&gr3d->client);
		free(gr3d);
		return NULL;
	}

	err = nvmap_handle_mmap(nvmap, gr3d->attributes);
	if (err < 0) {
		nvmap_handle_free(nvmap, gr3d->attributes);
		nvmap_handle_free(nvmap, gr3d->buffer);
		nvhost_client_exit(&gr3d->client);
		free(gr3d);
		return NULL;
	}

	err = nvhost_gr3d_init(gr3d);
	if (err < 0) {
		nvmap_handle_free(nvmap, gr3d->attributes);
		nvmap_handle_free(nvmap, gr3d->buffer);
		nvhost_client_exit(&gr3d->client);
		free(gr3d);
		return NULL;
	}

	return gr3d;
}

void nvhost_gr3d_close(struct nvhost_gr3d *gr3d)
{
	if (gr3d) {
		nvmap_handle_free(gr3d->client.nvmap, gr3d->attributes);
		nvmap_handle_free(gr3d->client.nvmap, gr3d->buffer);
		nvhost_client_exit(&gr3d->client);
	}

	free(gr3d);
}

int nvhost_gr3d_triangle(struct nvhost_gr3d *gr3d,
			 struct nvmap_framebuffer *fb)
{
	union {
		uint32_t u;
		float f;
	} value;
	float *attr = gr3d->attributes->ptr;
	struct nvhost_pushbuf *pb;
	unsigned int depth = 32;
	struct nvhost_job *job;
	uint32_t format, pitch;
	uint16_t *indices;
	uint32_t fence;
	int err;

	/* XXX: count syncpoint increments in command stream */
	job = nvhost_job_create(gr3d->client.syncpt, 9);
	if (!job)
		return -ENOMEM;

	/*
	 * XXX: Does the gr3d function properly if we submit everything in a
	 *      single pushbuffer instead of 5?
	 */
	/* XXX: wait check should be 1, mask 0x40000 */

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
		  nvmap_handle_get_offset(gr3d->attributes, attr);

	printf("indices @%lx\n", (unsigned long)indices - (unsigned long)gr3d->attributes->ptr);

	/* indices */
	*indices++ = 0x0000;
	*indices++ = 0x0001;
	*indices++ = 0x0002;

	err = nvmap_handle_writeback_invalidate(gr3d->client.nvmap,
						gr3d->attributes, 0,
						112);
	if (err < 0) {
	}

	/*
	  Command Buffer:
	    mem: e462a7c0, offset: 3b4c, words: 103
	    commands: 103
	*/

	pb = nvhost_job_append(job, gr3d->buffer, 0);
	if (!pb)
		return -ENOMEM;

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x404, 2));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, 0x000fffff);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_MASK(0x354, 9));
	nvhost_pushbuf_push(pb, 0x3efffff0);
	nvhost_pushbuf_push(pb, 0x3efffff0);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0x740, 0x035));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xe26, 0x779));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x346, 0x2));
	nvhost_pushbuf_push(pb, 0x00001401);
	nvhost_pushbuf_push(pb, 0x3f800000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x34c, 0x2));
	nvhost_pushbuf_push(pb, 0x00000002);
	nvhost_pushbuf_push(pb, 0x3f000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x000, 0x1));
	nvhost_pushbuf_push(pb, 0x00000216);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x000, 0x1));
	nvhost_pushbuf_push(pb, 0x00000216);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_MASK(0x352, 0x1b));

	value.f = fb->width * 8.0f;
	nvhost_pushbuf_push(pb, value.u);
	value.f = fb->height * 8.0f;
	nvhost_pushbuf_push(pb, value.u);

	value.f = fb->width * 8.0f;
	nvhost_pushbuf_push(pb, value.u);
	value.f = fb->height * 8.0f;
	nvhost_pushbuf_push(pb, value.u);

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x358, 0x03));
	nvhost_pushbuf_push(pb, 0x4376f000);
	nvhost_pushbuf_push(pb, 0x4376f000);
	nvhost_pushbuf_push(pb, 0x40dfae14);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x343, 0x01));
	nvhost_pushbuf_push(pb, 0xb8e00000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x350, 0x02));
	nvhost_pushbuf_push(pb, fb->width  & 0xffff);
	nvhost_pushbuf_push(pb, fb->height & 0xffff);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe11, 0x01));

	if (depth == 16) {
		format = NVHOST_GR3D_FORMAT_RGB565;
		pitch = fb->width * 2;
	} else {
		format = NVHOST_GR3D_FORMAT_RGBA8888;
		pitch = fb->width * 4;
	}

	nvhost_pushbuf_push(pb, 0x04000000 | (pitch << 8) | format << 2 | 0x1);

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x903, 0x01));
	nvhost_pushbuf_push(pb, 0x00000002);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe15, 0x01));
	nvhost_pushbuf_push(pb, 0x08000001);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe16, 0x01));
	nvhost_pushbuf_push(pb, 0x08000001);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe17, 0x01));
	nvhost_pushbuf_push(pb, 0x08000001);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe18, 0x01));
	nvhost_pushbuf_push(pb, 0x08000001);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe19, 0x01));
	nvhost_pushbuf_push(pb, 0x08000001);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe1a, 0x01));
	nvhost_pushbuf_push(pb, 0x08000001);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe1b, 0x01));
	nvhost_pushbuf_push(pb, 0x08000001);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe10, 0x01));
	nvhost_pushbuf_push(pb, 0x0c000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe13, 0x01));
	nvhost_pushbuf_push(pb, 0x0c000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe12, 0x01));
	nvhost_pushbuf_push(pb, 0x0c000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x348, 0x04));
	nvhost_pushbuf_push(pb, 0x3f800000);
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, 0x3f800000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xe27, 0x01));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa02, 0x06));
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, 0x00000020);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa00, 0xe00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa08, 0x100));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa02, 0x06));
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, 0x00000020);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa00, 0xe00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa08, 0x100));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0x403, 0x610));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa02, 0x6));
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, 0x00000020);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa00, 0xe00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa08, 0x100));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa02, 0x06));
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, 0x00000020);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa00, 0xe00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa08, 0x100));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x000, 0x01));
	nvhost_pushbuf_push(pb, 0x00000216);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x000, 0x01));
	nvhost_pushbuf_push(pb, 0x00000216);

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

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0x205, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x206, 0x08));
	nvhost_pushbuf_push(pb, 0x401f9c6c);
	nvhost_pushbuf_push(pb, 0x0040000d);
	nvhost_pushbuf_push(pb, 0x8106c083);
	nvhost_pushbuf_push(pb, 0x6041ff80);
	nvhost_pushbuf_push(pb, 0x401f9c6c);
	nvhost_pushbuf_push(pb, 0x0040010d);
	nvhost_pushbuf_push(pb, 0x8106c083);
	nvhost_pushbuf_push(pb, 0x6041ff9d);

	/*
	  Command Buffer:
	    mem: e462a7c0, offset: 3ce8, words: 16
	    commands: 16
	*/

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x343, 0x01));
	nvhost_pushbuf_push(pb, 0xb8e00000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x300, 0x02));
	nvhost_pushbuf_push(pb, 0x00000008);
	nvhost_pushbuf_push(pb, 0x0000fecd);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe20, 0x01));
	nvhost_pushbuf_push(pb, 0x58000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0x503, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0x545, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xe22, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0x603, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0x803, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x520, 0x01));
	nvhost_pushbuf_push(pb, 0x20006001);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x546, 0x01));
	nvhost_pushbuf_push(pb, 0x00000040);

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

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x541, 0x01));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x500, 0x01));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x601, 0x01));
	nvhost_pushbuf_push(pb, 0x00000001);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x604, 0x02));
	nvhost_pushbuf_push(pb, 0x104e51ba);
	nvhost_pushbuf_push(pb, 0x00408102);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x701, 0x01));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x801, 0x01));
	nvhost_pushbuf_push(pb, 0x00000001);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x804, 0x08));
	nvhost_pushbuf_push(pb, 0x0001c0c0);
	nvhost_pushbuf_push(pb, 0x3f41f200);
	nvhost_pushbuf_push(pb, 0x0001a080);
	nvhost_pushbuf_push(pb, 0x3f41f200);
	nvhost_pushbuf_push(pb, 0x00014000);
	nvhost_pushbuf_push(pb, 0x3f41f200);
	nvhost_pushbuf_push(pb, 0x00012040);
	nvhost_pushbuf_push(pb, 0x3f41f200);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x806, 0x01));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x901, 0x01));
	nvhost_pushbuf_push(pb, 0x00028005);

	/*
	  Command Buffer:
	    mem: e462a7c0, offset: 3d28, words: 66
	    commands: 66
	*/

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xa02, 0x06));
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x000001ff);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, 0x00000020);
	nvhost_pushbuf_push(pb, 0x00000030);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa00, 0xe00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa08, 0x100));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0x40c, 0x06));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x120, 0x01));
	nvhost_pushbuf_push(pb, 0x00030081);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x00, 0x01, 0x00));
	/* XXX: don't wait for syncpoint */
	/*
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x008, 0x01));
	nvhost_pushbuf_push(pb, 0x120000b1);
	*/
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x00, 0x60, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x344, 0x02));
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, 0x00000000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x00, 0x60, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x000, 0x01));
	nvhost_pushbuf_push(pb, 0x00000116);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x00, 0x01, 0x00));
	/* XXX: don't wait for syncpoint */
	/*
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x009, 0x01));
	nvhost_pushbuf_push(pb, 0x16030005);
	*/
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x00, 0x60, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xa00, 0xe01));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x00, 0x60, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x00, 0x01));
	nvhost_pushbuf_push(pb, 0x00000216);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x00, 0x01, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x009, 0x01));
	nvhost_pushbuf_push(pb, 0x16030006);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x00, 0x60, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe01, 0x01));
	/* relocate color render target */
	nvhost_pushbuf_relocate(pb, fb->handle, 0, 0);
	nvhost_pushbuf_push(pb, 0xdeadbeef);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe31, 0x01));
	nvhost_pushbuf_push(pb, 0x00000000);
	/* vertex position attribute */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x100, 0x01));
	nvhost_pushbuf_relocate(pb, gr3d->attributes, 0x30, 0);
	nvhost_pushbuf_push(pb, 0xdeadbeef);
	/* vertex color attribute */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x102, 0x01));
	nvhost_pushbuf_relocate(pb, gr3d->attributes, 0, 0);
	nvhost_pushbuf_push(pb, 0xdeadbeef);
	/* primitive indices */
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x121, 0x03));
	nvhost_pushbuf_relocate(pb, gr3d->attributes, 0x60, 0);
	nvhost_pushbuf_push(pb, 0xdeadbeef);
	nvhost_pushbuf_push(pb, 0xec000000);
	nvhost_pushbuf_push(pb, 0x00200000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_IMM(0xe27, 0x02));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x000, 0x01));
	nvhost_pushbuf_push(pb, 0x00000216);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x00, 0x60, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x000, 0x01));
	nvhost_pushbuf_push(pb, 0x00000116);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x00, 0x01, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x09, 0x01));
	nvhost_pushbuf_push(pb, 0x16030008);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x00, 0x60, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x00, 0x01, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x0c, 0x01));
	nvhost_pushbuf_push(pb, 0x03000008);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x00, 0x60, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x00, 0x01));
	nvhost_pushbuf_push(pb, 0x00000116);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x00, 0x01, 0x00));
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x0c, 0x01));
	nvhost_pushbuf_push(pb, 0x03000001);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_SETCL(0x00, 0x60, 0x00));

	err = nvhost_client_submit(&gr3d->client, job);
	if (err < 0) {
		return err;
	}

	err = nvhost_client_flush(&gr3d->client, &fence);
	if (err < 0) {
		return err;
	}

	printf("fence: %u\n", fence);

	err = nvhost_client_wait(&gr3d->client, fence, -1);
	if (err < 0) {
		return err;
	}

	return 0;
}
