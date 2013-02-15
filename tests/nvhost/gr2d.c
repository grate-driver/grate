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

	err = nvhost_client_wait(&gr2d->client, fence, -1);
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

	err = nvmap_handle_alloc(nvmap, gr2d->buffer, 1 << 0, 0x0a000001, 0x20);
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

	err = nvmap_handle_alloc(nvmap, gr2d->scratch, 1 << 30, 1 << 0, 0x20);
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
	int err;

	if (fb->depth == 16) {
		color = ((uint32_t)(red   * 31) << 11) |
			((uint32_t)(green * 63) <<  5) |
			((uint32_t)(blue  * 31) <<  0);
	} else {
		color = ((uint32_t)(alpha * 255) << 24) |
			((uint32_t)(blue  * 255) << 16) |
			((uint32_t)(green * 255) <<  8) |
			((uint32_t)(red   * 255) <<  0);
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

	if (fb->depth == 16)
		nvhost_pushbuf_push(pb, 0x00000040); /* 64 byte stride */
	else
		nvhost_pushbuf_push(pb, 0x00000080); /* 128 byte stride */

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x35, 1));

#if 0
	if (fb->depth == 16) {
		//nvhost_pushbuf_push(pb, 0x0000f800); /* 16-bit red */
		nvhost_pushbuf_push(pb, 0x00000000); /* 16-bit black */
	} else {
		//nvhost_pushbuf_push(pb, 0xff0000ff); /* 32-bit red */
		nvhost_pushbuf_push(pb, 0xff000000); /* 32-bit black */
	}
#else
	nvhost_pushbuf_push(pb, color);
#endif

	nvhost_pushbuf_push(pb, NVHOST_OPCODE_NONINCR(0x46, 1));
	nvhost_pushbuf_push(pb, 0x00100000);
	nvhost_pushbuf_push(pb, NVHOST_OPCODE_MASK(0x38, 5));
	nvhost_pushbuf_push(pb, 0x00200020);
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

	err = nvhost_client_wait(&gr2d->client, fence, -1);
	if (err < 0) {
		return err;
	}

	return 0;
}
