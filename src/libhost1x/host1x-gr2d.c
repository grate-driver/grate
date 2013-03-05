#include <errno.h>

#include "host1x.h"
#include "host1x-private.h"

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
	host1x_pushbuf_push(pb, HOST1X_OPCODE_SETCL(0x000, 0x052, 0x00));
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
	host1x_pushbuf_push(pb, 0x00000112);

	err = host1x_client_submit(gr2d->client, job);
	if (err < 0) {
		host1x_job_free(job);
		return err;
	}

	host1x_job_free(job);

	err = host1x_client_flush(gr2d->client, &fence);
	if (err < 0) {
		return err;
	}

	//printf("fence: %u\n", fence);

	err = host1x_client_wait(gr2d->client, fence, -1);
	if (err < 0) {
		return err;
	}

	return 0;
}

int host1x_gr2d_init(struct host1x *host1x, struct host1x_gr2d *gr2d)
{
	int err;

	gr2d->commands = host1x_bo_create(host1x, 8 * 4096, 2);
	if (!gr2d->commands)
		return -ENOMEM;

	err = host1x_bo_mmap(gr2d->commands);
	if (err < 0)
		return err;

	gr2d->scratch = host1x_bo_create(host1x, 64, 3);
	if (!gr2d->scratch) {
		host1x_bo_free(gr2d->commands);
		return -ENOMEM;
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

int host1x_gr2d_clear(struct host1x_gr2d *gr2d, struct host1x_framebuffer *fb,
		      float red, float green, float blue, float alpha)
{
	struct host1x_syncpt *syncpt = &gr2d->client->syncpts[0];
	struct host1x_pushbuf *pb;
	struct host1x_job *job;
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

	job = host1x_job_create(syncpt->id, 1);
	if (!job)
		return -ENOMEM;

	pb = host1x_job_append(job, gr2d->commands, 0);
	if (!pb) {
		host1x_job_free(job);
		return -ENOMEM;
	}

	host1x_pushbuf_push(pb, HOST1X_OPCODE_SETCL(0, 0x51, 0));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_SETCL(0, 0x51, 0));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_EXTEND(0, 0x01));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x09, 9));
	host1x_pushbuf_push(pb, 0x0000003a);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x1e, 7));
	host1x_pushbuf_push(pb, 0x00000000);

	if (fb->depth == 16)
		host1x_pushbuf_push(pb, 0x00010044); /* 16-bit depth */
	else
		host1x_pushbuf_push(pb, 0x00020044); /* 32-bit depth */

	host1x_pushbuf_push(pb, 0x000000cc);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x2b, 9));
	host1x_pushbuf_relocate(pb, fb->bo, 0, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
	host1x_pushbuf_push(pb, pitch);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x35, 1));
	host1x_pushbuf_push(pb, color);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x46, 1));
	host1x_pushbuf_push(pb, 0x00100000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x38, 5));
	host1x_pushbuf_push(pb, fb->height << 16 | fb->width);
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_EXTEND(1, 1));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x00, 1));
	host1x_pushbuf_push(pb, 0x00000112);

	err = host1x_client_submit(gr2d->client, job);
	if (err < 0) {
		host1x_job_free(job);
		return err;
	}

	host1x_job_free(job);

	err = host1x_client_flush(gr2d->client, &fence);
	if (err < 0) {
		return err;
	}

	printf("fence: %u\n", fence);

	err = host1x_client_wait(gr2d->client, fence, -1);
	if (err < 0) {
		return err;
	}

	return 0;
}
