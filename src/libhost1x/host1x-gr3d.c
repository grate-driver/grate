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

#include <fcntl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include "host1x.h"
#include "nvhost.h"

#define HOST1X_GR3D_TEST 0

#define HOST1X_GR3D_SCISSOR_HORIZONTAL	0x350
#define HOST1X_GR3D_SCISSOR_VERTICAL	0x351

#define CLK_RST_CONTROLLER_RST_DEV_L_SET_0	0x300
#define CLK_RST_CONTROLLER_RST_DEV_L_CLR_0	0x304

#define CAR_3D	(1 << 24)

static int host1x_gr3d_test(struct host1x_gr3d *gr3d)
{
	struct host1x_syncpt *syncpt = &gr3d->client->syncpts[0];
	struct host1x_pushbuf *pb;
	struct host1x_job *job;
	uint32_t fence;
	int err = 0;

	job = HOST1X_JOB_CREATE(syncpt->id, 1);
	if (!job)
		return -ENOMEM;

	pb = HOST1X_JOB_APPEND(job, gr3d->commands, 0);
	if (!pb) {
		host1x_job_free(job);
		return -ENOMEM;
	}

	host1x_pushbuf_push(pb, HOST1X_OPCODE_SETCL(0x000, 0x060, 0x00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x000, 0x0001));
	host1x_pushbuf_push(pb, 0x000001 << 8 | syncpt->id);

	err = HOST1X_CLIENT_SUBMIT(gr3d->client, job);
	if (err < 0) {
		host1x_job_free(job);
		return err;
	}

	host1x_job_free(job);

	err = HOST1X_CLIENT_FLUSH(gr3d->client, &fence);
	if (err < 0)
		return err;

	err = HOST1X_CLIENT_WAIT(gr3d->client, fence, ~0u);
	if (err < 0)
		return err;

	return 0;
}

static int map_mem(void **mem_virt, off_t phys_address, int size)
{
	off_t PageOffset, PageAddress;
	int PagesSize;
	int mem_dev;

	mem_dev = open("/dev/mem", O_RDWR | O_SYNC);
	if (mem_dev < 0)
		return mem_dev;

	PageOffset  = phys_address % getpagesize();
	PageAddress = phys_address - PageOffset;
	PagesSize   = (((size - 1) / getpagesize()) + 1) * getpagesize();

	*mem_virt = mmap(NULL, (size_t)PagesSize, PROT_READ | PROT_WRITE,
			 MAP_SHARED, mem_dev, PageAddress);

	if (*mem_virt == MAP_FAILED)
		return -1;

	*mem_virt += PageOffset;

	return 0;
}

static void reg_write(void *mem_virt, uint32_t offset, uint32_t value)
{
	*(volatile uint32_t*)(mem_virt + offset) = value;
}

static void host1x_gr3d_reset_hw(void)
{
	void *CAR_io_mem_virt;
	int err;

	err = map_mem(&CAR_io_mem_virt, 0x60006000, 0x1000);
	if (err < 0) {
		host1x_error("host1x_gr3d_reset_hw() failed\n");
		return;
	}

	reg_write(CAR_io_mem_virt,
		  CLK_RST_CONTROLLER_RST_DEV_L_SET_0, CAR_3D);

	usleep(1000);

	reg_write(CAR_io_mem_virt,
		  CLK_RST_CONTROLLER_RST_DEV_L_CLR_0, CAR_3D);
}

static int host1x_gr3d_reset(struct host1x_gr3d *gr3d)
{
	struct host1x_syncpt *syncpt = &gr3d->client->syncpts[0];
	struct host1x_pushbuf *pb;
	struct host1x_job *job;
	unsigned int i;
	uint32_t fence;
	int err;

	host1x_gr3d_reset_hw();

	job = HOST1X_JOB_CREATE(syncpt->id, 1);
	if (!job)
		return -ENOMEM;

	pb = HOST1X_JOB_APPEND(job, gr3d->commands, 0);
	if (!pb)
		return -ENOMEM;

	/* Tegra30 specific stuff */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x750, 0x0010));
	for (i = 0; i < 16; i++)
		host1x_pushbuf_push(pb, 0x00000000);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x907, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x908, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x909, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x90a, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x90b, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xb00, 0x0003));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xb01, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xb04, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xb06, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xb07, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xb08, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xb09, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xb0a, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xb0b, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xb0c, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xb0d, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xb0e, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xb0f, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xb10, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xb11, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xb12, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xb14, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe40, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe41, 0x0000));

	/* Common stuff */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x00d, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x00e, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x00f, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x010, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x011, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x012, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x013, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x014, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x015, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x120, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x122, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x124, 0x0007));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x125, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x126, 0x0000));

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x200, 0x0005));
	host1x_pushbuf_push(pb, 0x00000011);
	host1x_pushbuf_push(pb, 0x0000ffff);
	host1x_pushbuf_push(pb, 0x00ff0000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x209, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x20a, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x20c, 0x0003));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x300, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x301, 0x0000));

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

	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x352, 0x001b));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x41800000);
	host1x_pushbuf_push(pb, 0x41800000);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x354, 0x0009));
	host1x_pushbuf_push(pb, 0x3efffff0);
	host1x_pushbuf_push(pb, 0x3efffff0);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x358, 0x0003));
	host1x_pushbuf_push(pb, 0x3f800000);
	host1x_pushbuf_push(pb, 0x3f800000);
	host1x_pushbuf_push(pb, 0x3f800000);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x363, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x364, 0x0000));

	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x400, 0x07ff));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x401, 0x07ff));

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

	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x500, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x501, 0x0007));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x502, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x503, 0x0000));

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x520, 0x0020));
	for (i = 0; i < 32; i++)
		host1x_pushbuf_push(pb, 0x00000000);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x540, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x542, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x543, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x544, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x545, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x546, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x60e, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x702, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x740, 0x0001));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x741, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x742, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x902, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x903, 0x0000));

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xa00, 0x000d));
	host1x_pushbuf_push(pb, 0x00000e00);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, 0x00000020);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x00000100);
	host1x_pushbuf_push(pb, 0x0f0f0f0f);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x00000000);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe20, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe21, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe22, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe25, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe26, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe27, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe28, 0x0000));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe29, 0x0000));

	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x00, 0x01));
	host1x_pushbuf_push(pb, 0x000001 << 8 | syncpt->id);

	err = HOST1X_CLIENT_SUBMIT(gr3d->client, job);
	if (err < 0)
		return err;

	err = HOST1X_CLIENT_FLUSH(gr3d->client, &fence);
	if (err < 0)
		return err;

	err = HOST1X_CLIENT_WAIT(gr3d->client, fence, ~0u);
	if (err < 0)
		return err;

	return 0;
}

int host1x_gr3d_init(struct host1x *host1x, struct host1x_gr3d *gr3d)
{
	int err;

	gr3d->commands = host1x_bo_create(host1x, 32 * 4096,
					  NVHOST_BO_FLAG_COMMAND_BUFFER);
	if (!gr3d->commands)
		return -ENOMEM;

	err = HOST1X_BO_MMAP(gr3d->commands, NULL);
	if (err < 0) {
		host1x_bo_free(gr3d->commands);
		return err;
	}

	gr3d->attributes = host1x_bo_create(host1x, 12 * 4096,
					    NVHOST_BO_FLAG_ATTRIBUTES);
	if (!gr3d->attributes) {
		host1x_bo_free(gr3d->commands);
		return -ENOMEM;
	}

	err = HOST1X_BO_MMAP(gr3d->attributes, NULL);
	if (err < 0) {
		host1x_bo_free(gr3d->attributes);
		host1x_bo_free(gr3d->commands);
		return err;
	}

	if (HOST1X_GR3D_TEST) {
		err = host1x_gr3d_test(gr3d);
		if (err < 0) {
			host1x_error("host1x_gr3d_test() failed: %d\n", err);
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
			 struct host1x_pixelbuffer *pixbuf)
{
	struct host1x_syncpt *syncpt = &gr3d->client->syncpts[0];
	float *attr = gr3d->attributes->ptr;
	struct host1x_pushbuf *pb;
	unsigned int depth = 32;
	struct host1x_job *job;
	uint32_t format;
	uint16_t *indices;
	uint32_t fence;
	int err, i;

	/* XXX: count syncpoint increments in command stream */
	job = HOST1X_JOB_CREATE(syncpt->id, 9);
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

	err = HOST1X_BO_FLUSH(gr3d->attributes, gr3d->attributes->offset, 112);
	if (err < 0) {
		host1x_job_free(job);
		return err;
	}

	/*
	  Command Buffer:
	    mem: e462a7c0, offset: 3b4c, words: 103
	    commands: 103
	*/

	pb = HOST1X_JOB_APPEND(job, gr3d->commands, 0);
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

	host1x_pushbuf_push_float(pb, (float)pixbuf->width * 8.0f);
	host1x_pushbuf_push_float(pb, (float)pixbuf->height * 8.0f);
	host1x_pushbuf_push_float(pb, (float)pixbuf->width * 8.0f);
	host1x_pushbuf_push_float(pb, (float)pixbuf->height * 8.0f);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x358, 0x03));
	host1x_pushbuf_push(pb, 0x4376f000);
	host1x_pushbuf_push(pb, 0x4376f000);
	host1x_pushbuf_push(pb, 0x40dfae14);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x343, 0x01));
	host1x_pushbuf_push(pb, 0xb8e00000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x350, 0x02));
	host1x_pushbuf_push(pb, pixbuf->width  & 0xffff);
	host1x_pushbuf_push(pb, pixbuf->height & 0xffff);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xe11, 0x01));

	if (depth == 16)
		format = HOST1X_GR3D_FORMAT_RGB565;
	else
		format = HOST1X_GR3D_FORMAT_RGBA8888;

	host1x_pushbuf_push(pb, 0x04000000 | (pixbuf->pitch << 8) | format << 2 | 0x1);

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
	    mem: e462a7c0, offset: 3d28, words: 67
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
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe21, 0x0140));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xe01, 0x01));
	/* relocate color render target */
	HOST1X_PUSHBUF_RELOCATE(pb, pixbuf->bo, 0, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
	/* vertex position attribute */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x100, 0x01));
	HOST1X_PUSHBUF_RELOCATE(pb, gr3d->attributes, 0x30, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
	/* vertex color attribute */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x102, 0x01));
	HOST1X_PUSHBUF_RELOCATE(pb, gr3d->attributes, 0, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
	/* primitive indices */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x121, 0x03));
	HOST1X_PUSHBUF_RELOCATE(pb, gr3d->attributes, 0x60, 0);
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

	err = HOST1X_CLIENT_SUBMIT(gr3d->client, job);
	if (err < 0) {
		host1x_job_free(job);
		return err;
	}

	host1x_job_free(job);

	err = HOST1X_CLIENT_FLUSH(gr3d->client, &fence);
	if (err < 0)
		return err;

	err = HOST1X_CLIENT_WAIT(gr3d->client, fence, ~0u);
	if (err < 0)
		return err;

	return 0;
}
