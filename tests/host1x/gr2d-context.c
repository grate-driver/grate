/*
 * Copyright (c) Dmitry Osipenko
 * Copyright (c) Erik Faye-Lund
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "host1x.h"
#include "host1x-private.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define CTX_NB 11

struct ctx2d {
	struct host1x *host1x;
	struct host1x_gr2d *gr2d;
};

static struct ctx2d *create_context()
{
	struct ctx2d *ctx = calloc(1, sizeof(struct ctx2d));

	ctx->host1x = host1x_open(true, -1, -1);
	if (!ctx->host1x) {
		fprintf(stderr, "host1x_open() failed\n");
		abort();
	}

	ctx->gr2d = host1x_get_gr2d(ctx->host1x);
	if (!ctx->gr2d) {
		fprintf(stderr, "host1x_get_gr2d() failed\n");
		abort();
	}

	return ctx;
}

static void prepare_context(struct ctx2d *ctx,
			    struct host1x_pixelbuffer *target,
			    unsigned int width, unsigned int height,
			    uint32_t color)
{
	struct host1x_syncpt *syncpt = &ctx->gr2d->client->syncpts[0];
	struct host1x_pixelbuffer *src;
	struct host1x_pushbuf *pb;
	struct host1x_job *job;
	struct host1x_bo *dst;
	uint32_t handle;
	uint32_t fence;
	int err;

	err = HOST1X_BO_EXPORT(target->bo, &handle);
	if (err < 0)
		abort();

	dst = HOST1X_BO_IMPORT(ctx->host1x, handle);
	if (!dst)
		abort();

	src = host1x_pixelbuffer_create(ctx->host1x,
					width, height, width * 4,
					PIX_BUF_FMT_RGBA8888,
					PIX_BUF_LAYOUT_LINEAR);
	if (!src)
		abort();

	err = host1x_gr2d_clear(ctx->gr2d, src, color);
	if (err < 0)
		abort();

	job = HOST1X_JOB_CREATE(syncpt->id, 1);
	if (!job)
		abort();

	pb = HOST1X_JOB_APPEND(job, ctx->gr2d->commands, 0);
	if (!pb)
		abort();

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
			(PIX_BUF_FORMAT_BYTES(target->format) >> 1) << 16 |
			0 << 14 | 0 << 10 | 0 << 9);
	host1x_pushbuf_push(pb, 0x000000cc); /* ropfade */

	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x046, 1));
	/*
	 * [20:20] destination write tile mode (0: linear, 1: tiled)
	 * [ 0: 0] tile mode Y/RGB (0: linear, 1: tiled)
	 */
	host1x_pushbuf_push(pb, 0 << 20 | 0); /* tilemode */

	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x02b, 0x6149));
	HOST1X_PUSHBUF_RELOCATE(pb, dst, target->bo->offset, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef); /* dstba */
	host1x_pushbuf_push(pb, target->pitch); /* dstst */
	HOST1X_PUSHBUF_RELOCATE(pb, src->bo, src->bo->offset, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef); /* srcba */
	host1x_pushbuf_push(pb, src->pitch); /* srcst */
	host1x_pushbuf_push(pb, height << 16 | width); /* dstsize */
	host1x_pushbuf_push(pb, 0 << 16 | 0); /* srcps */

	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x000, 1));
	host1x_pushbuf_push(pb, 0x000001 << 8 | syncpt->id);

	err = HOST1X_CLIENT_SUBMIT(ctx->gr2d->client, job);
	if (err < 0)
		abort();

	host1x_job_free(job);

	err = HOST1X_CLIENT_FLUSH(ctx->gr2d->client, &fence);
	if (err < 0)
		abort();
}

static void exec_context(struct ctx2d *ctx, unsigned int dx, unsigned int dy)
{
	struct host1x_syncpt *syncpt = &ctx->gr2d->client->syncpts[0];
	struct host1x_pushbuf *pb;
	struct host1x_job *job;
	uint32_t fence;
	int err;

	job = HOST1X_JOB_CREATE(syncpt->id, 1);
	if (!job)
		abort();

	pb = HOST1X_JOB_APPEND(job, ctx->gr2d->commands, 0);
	if (!pb)
		abort();

	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x03a, 1));
	host1x_pushbuf_push(pb, dy << 16 | dx); /* dstps */

	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x000, 1));
	host1x_pushbuf_push(pb, 0x000001 << 8 | syncpt->id);

	err = HOST1X_CLIENT_SUBMIT(ctx->gr2d->client, job);
	if (err < 0)
		abort();

	host1x_job_free(job);

	err = HOST1X_CLIENT_FLUSH(ctx->gr2d->client, &fence);
	if (err < 0)
		abort();

	err = HOST1X_CLIENT_WAIT(ctx->gr2d->client, fence, ~0u);
	if (err < 0)
		abort();
}

int main(int argc, char *argv[])
{
	struct host1x_display *display = NULL;
	struct host1x_overlay *overlay = NULL;
	struct host1x_framebuffer *fb;
	unsigned int width = 256;
	unsigned int height = 256;
	struct host1x *host1x;
	struct ctx2d *ctx[CTX_NB];
	unsigned char *map;
	unsigned x, y;
	int err;
	int i;

	host1x = host1x_open(true, -1, -1);
	if (!host1x) {
		fprintf(stderr, "host1x_open() failed\n");
		return 1;
	}

	display = host1x_get_display(host1x);
	if (display) {
		err = host1x_overlay_create(&overlay, display);
		if (err < 0) {
			fprintf(stderr, "overlay support missing\n");

			/*
			 * If overlay support is missing but we still have
			 * on-screen display support, make the framebuffer
			 * the same resolution as the display to make sure
			 * it can be properly displayed.
			 */

			err = host1x_display_get_resolution(display, &width,
							    &height);
			if (err < 0) {
				fprintf(stderr, "host1x_display_get_resolution() failed: %d\n", err);
				return 1;
			}
		}
	}

	fb = host1x_framebuffer_create(host1x, width, height,
				       PIX_BUF_FMT_RGBA8888,
				       PIX_BUF_LAYOUT_LINEAR, 0);
	if (!fb) {
		fprintf(stderr, "host1x_framebuffer_create() failed\n");
		return 1;
	}

	for (i = 0; i < CTX_NB; i++)
		ctx[i] = create_context();

	for (i = 0; i < CTX_NB; i++)
		prepare_context(ctx[i], fb->pixbuf,
				width - i * CTX_NB,
				height,
				( 63 / (i + 1)) << 0 |
				(127 / (i + 1)) << 8 |
				(255 / (i + 1)) << 16);

	for (i = 0; i < CTX_NB; i++)
		exec_context(ctx[i], i * CTX_NB, 0);

	if (display) {
		if (overlay) {
			err = host1x_overlay_set(overlay, fb, 0, 0,
						 width, height, false, true);
			if (err < 0)
				fprintf(stderr, "host1x_overlay_set() failed: %d\n", err);
			else
				sleep(5);

			host1x_overlay_close(overlay);
		} else {
			err = host1x_display_set(display, fb, false, true);
			if (err < 0)
				fprintf(stderr, "host1x_display_set() failed: %d\n", err);
			else
				sleep(5);
		}
	} else {
		host1x_framebuffer_save(host1x, fb, "test.png");
	}

	HOST1X_BO_MMAP(fb->pixbuf->bo, (void**) &map);
	map += fb->pixbuf->bo->offset;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			int idx = MIN(x / CTX_NB, CTX_NB - 1);
			uint32_t pixel = 0;
			uint32_t test = 0;

			pixel |= map[x * 4 + 0] << 24;
			pixel |= map[x * 4 + 1] << 16;
			pixel |= map[x * 4 + 2] << 8;
			pixel |= map[x * 4 + 3] << 0;

			test |= ( 63 / (idx + 1)) << 24;
			test |= (127 / (idx + 1)) << 16;
			test |= (255 / (idx + 1)) << 8;
			test |= (  0 / (idx + 1)) << 0;

			if (pixel != test) {
				fprintf(stderr, "test failed at (%u, %u) "
						"0x%08X != 0x%08X\n",
					x, y, pixel, test);
				abort();
			}
		}

		map += fb->pixbuf->pitch;
	}

	printf("test passed\n");

	host1x_framebuffer_free(fb);
	host1x_close(host1x);

	return 0;
}
