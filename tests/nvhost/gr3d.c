#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "gr3d.h"

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
	float *attr = gr3d->attributes->ptr;
	struct nvhost_pushbuf *pb;
	struct nvhost_job *job;
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

	pb = nvhost_job_append(job, gr3d->buffer, 0x3b4c);
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
	nvhost_pushbuf_push(pb, 0x43800000);
	nvhost_pushbuf_push(pb, 0x43800000);
	nvhost_pushbuf_push(pb, 0x43800000);
	nvhost_pushbuf_push(pb, 0x43800000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x358, 0x03));
	nvhost_pushbuf_push(pb, 0x4376f000);
	nvhost_pushbuf_push(pb, 0x4376f000);
	nvhost_pushbuf_push(pb, 0x40dfae14);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x343, 0x01));
	nvhost_pushbuf_push(pb, 0xb8e00000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x350, 0x02));
	nvhost_pushbuf_push(pb, 0x00000020);
	nvhost_pushbuf_push(pb, 0x00000020);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0xe11, 0x01));
	nvhost_pushbuf_push(pb, 0x04004019);
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
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x100, 0x01));
	/* vertex position attribute */
	nvhost_pushbuf_relocate(pb, gr3d->attributes, 0x30, 0);
	nvhost_pushbuf_push(pb, 0xdeadbeef);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x102, 0x01));
	/* vertex color attribute */
	nvhost_pushbuf_relocate(pb, gr3d->attributes, 0, 0);
	nvhost_pushbuf_push(pb, 0xdeadbeef);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_INCR(0x121, 0x03));
	/* primitive indices */
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
