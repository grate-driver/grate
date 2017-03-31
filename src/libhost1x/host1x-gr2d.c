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

#define FLOAT_TO_FIXED_6_12(fp) \
	(((int32_t) (fp * 4096.0f + 0.5f)) & ((1 << 18) - 1))

#define FLOAT_TO_FIXED_0_8(fp) \
	(((int32_t) (fp * 256.0f + 0.5f)) & ((1 << 8) - 1))

static int host1x_gr2d_test(struct host1x_gr2d *gr2d)
{
	struct host1x_syncpt *syncpt = &gr2d->client->syncpts[0];
	struct host1x_pushbuf *pb;
	struct host1x_job *job;
	uint32_t fence;
	int err = 0;

	job = HOST1X_JOB_CREATE(syncpt->id, 1);
	if (!job)
		return -ENOMEM;

	pb = HOST1X_JOB_APPEND(job, gr2d->commands, 0);
	if (!pb) {
		host1x_job_free(job);
		return -ENOMEM;
	}

	host1x_pushbuf_push(pb, HOST1X_OPCODE_SETCL(0x000, 0x051, 0x00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x000, 0x0001));
	host1x_pushbuf_push(pb, 0x000001 << 8 | syncpt->id);

	err = HOST1X_CLIENT_SUBMIT(gr2d->client, job);
	if (err < 0) {
		host1x_job_free(job);
		return err;
	}

	host1x_job_free(job);

	err = HOST1X_CLIENT_FLUSH(gr2d->client, &fence);
	if (err < 0)
		return err;

	err = HOST1X_CLIENT_WAIT(gr2d->client, fence, ~0u);
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

	job = HOST1X_JOB_CREATE(syncpt->id, 1);
	if (!job)
		return -ENOMEM;

	pb = HOST1X_JOB_APPEND(job, gr2d->commands, 0);
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
	HOST1X_PUSHBUF_RELOCATE(pb, gr2d->scratch, 0, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
	host1x_pushbuf_push(pb, 0x00000020);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x031, 0x0005));
	HOST1X_PUSHBUF_RELOCATE(pb, gr2d->scratch, 0, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
	host1x_pushbuf_push(pb, 0x00000020);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x046, 0x000d));
	host1x_pushbuf_push(pb, 0x00000000);
	HOST1X_PUSHBUF_RELOCATE(pb, gr2d->scratch, 0, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
	HOST1X_PUSHBUF_RELOCATE(pb, gr2d->scratch, 0, 0);
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

	err = HOST1X_CLIENT_SUBMIT(gr2d->client, job);
	if (err < 0) {
		host1x_job_free(job);
		return err;
	}

	host1x_job_free(job);

	err = HOST1X_CLIENT_FLUSH(gr2d->client, &fence);
	if (err < 0)
		return err;

	err = HOST1X_CLIENT_WAIT(gr2d->client, fence, ~0u);
	if (err < 0)
		return err;

	return 0;
}

int host1x_gr2d_init(struct host1x *host1x, struct host1x_gr2d *gr2d)
{
	int err;

	gr2d->commands = HOST1X_BO_CREATE(host1x, 8 * 4096,
					  NVHOST_BO_FLAG_COMMAND_BUFFER);
	if (!gr2d->commands)
		return -ENOMEM;

	err = HOST1X_BO_MMAP(gr2d->commands, NULL);
	if (err < 0)
		return err;

	gr2d->scratch = HOST1X_BO_CREATE(host1x, 64, NVHOST_BO_FLAG_SCRATCH);
	if (!gr2d->scratch) {
		host1x_bo_free(gr2d->commands);
		return -ENOMEM;
	}

	if (HOST1X_GR2D_TEST) {
		err = host1x_gr2d_test(gr2d);
		if (err < 0) {
			host1x_error("host1x_gr2d_test() failed: %d\n", err);
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
		      uint32_t color)
{
	struct host1x_syncpt *syncpt = &gr2d->client->syncpts[0];
	struct host1x_pushbuf *pb;
	struct host1x_job *job;
	uint32_t fence;
	int err;

	job = HOST1X_JOB_CREATE(syncpt->id, 1);
	if (!job)
		return -ENOMEM;

	pb = HOST1X_JOB_APPEND(job, gr2d->commands, 0);
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
	HOST1X_PUSHBUF_RELOCATE(pb, pixbuf->bo, pixbuf->bo->offset, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
	host1x_pushbuf_push(pb, pixbuf->pitch);
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

	err = HOST1X_CLIENT_SUBMIT(gr2d->client, job);
	if (err < 0) {
		host1x_job_free(job);
		return err;
	}

	host1x_job_free(job);

	err = HOST1X_CLIENT_FLUSH(gr2d->client, &fence);
	if (err < 0)
		return err;

	err = HOST1X_CLIENT_WAIT(gr2d->client, fence, ~0u);
	if (err < 0)
		return err;

	return 0;
}

int host1x_gr2d_blit(struct host1x_gr2d *gr2d,
		     struct host1x_pixelbuffer *src,
		     struct host1x_pixelbuffer *dst,
		     unsigned int sx, unsigned int sy,
		     unsigned int dx, unsigned int dy,
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
		host1x_error("Unequal bytes size\n");
		return -EINVAL;
	}

	switch (src->layout) {
	case PIX_BUF_LAYOUT_TILED_16x16:
		src_tiled = 1;
	case PIX_BUF_LAYOUT_LINEAR:
		break;
	default:
		host1x_error("Invalid src layout %u\n", src->layout);
		return -EINVAL;
	}

	switch (dst->layout) {
	case PIX_BUF_LAYOUT_TILED_16x16:
		dst_tiled = 1;
	case PIX_BUF_LAYOUT_LINEAR:
		break;
	default:
		host1x_error("Invalid dst layout %u\n", dst->layout);
		return -EINVAL;
	}

	job = HOST1X_JOB_CREATE(syncpt->id, 1);
	if (!job)
		return -ENOMEM;

	pb = HOST1X_JOB_APPEND(job, gr2d->commands, 0);
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
	HOST1X_PUSHBUF_RELOCATE(pb, dst->bo, dst->bo->offset, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef); /* dstba */
	host1x_pushbuf_push(pb, dst->pitch); /* dstst */
	HOST1X_PUSHBUF_RELOCATE(pb, src->bo, src->bo->offset, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef); /* srcba */
	host1x_pushbuf_push(pb, src->pitch); /* srcst */
	host1x_pushbuf_push(pb, height << 16 | width); /* dstsize */
	host1x_pushbuf_push(pb, sy << 16 | sx); /* srcps */
	host1x_pushbuf_push(pb, dy << 16 | dx); /* dstps */

	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x000, 1));
	host1x_pushbuf_push(pb, 0x000001 << 8 | syncpt->id);

	err = HOST1X_CLIENT_SUBMIT(gr2d->client, job);
	if (err < 0) {
		host1x_job_free(job);
		return err;
	}

	host1x_job_free(job);

	err = HOST1X_CLIENT_FLUSH(gr2d->client, &fence);
	if (err < 0)
		return err;

	err = HOST1X_CLIENT_WAIT(gr2d->client, fence, ~0u);
	if (err < 0)
		return err;

	return 0;
}

static uint32_t sb_offset(struct host1x_pixelbuffer *pixbuf,
			  uint32_t xpos, uint32_t ypos)
{
	uint32_t offset;
	uint32_t bytes_per_pixel = PIX_BUF_FORMAT_BYTES(pixbuf->format);
	uint32_t pixels_per_line = pixbuf->pitch / bytes_per_pixel;
	uint32_t xb;

	if (pixbuf->layout == PIX_BUF_LAYOUT_LINEAR) {
		offset = ypos * pixbuf->pitch;
		offset += xpos * bytes_per_pixel;
	} else {
		xb = xpos * bytes_per_pixel;
		offset = 16 * pixels_per_line * (ypos / 16);
		offset += 256 * (xb / 16);
		offset += 16 * (ypos % 16);
		offset += xb % 16;
	}

	return offset;
}

int host1x_gr2d_surface_blit(struct host1x_gr2d *gr2d,
			     struct host1x_pixelbuffer *src,
			     struct host1x_pixelbuffer *dst,
			     unsigned int sx, unsigned int sy,
			     unsigned int src_width, unsigned int src_height,
			     unsigned int dx, unsigned int dy,
			     unsigned int dst_width, unsigned int dst_height)
{
	struct host1x_syncpt *syncpt = &gr2d->client->syncpts[0];
	struct host1x_pushbuf *pb;
	struct host1x_job *job;
	float inv_scale_x;
	float inv_scale_y;
	unsigned src_tiled = 0;
	unsigned dst_tiled = 0;
	unsigned src_fmt;
	unsigned dst_fmt;
	unsigned hftype;
	unsigned vftype;
	unsigned vfen;
	uint32_t fence;
	int err;

	switch (src->layout) {
	case PIX_BUF_LAYOUT_TILED_16x16:
		src_tiled = 1;
	case PIX_BUF_LAYOUT_LINEAR:
		break;
	default:
		host1x_error("Invalid src layout %u\n", src->layout);
		return -EINVAL;
	}

	switch (dst->layout) {
	case PIX_BUF_LAYOUT_TILED_16x16:
		dst_tiled = 1;
	case PIX_BUF_LAYOUT_LINEAR:
		break;
	default:
		host1x_error("Invalid dst layout %u\n", dst->layout);
		return -EINVAL;
	}

	switch (src->format) {
	case PIX_BUF_FMT_ARGB8888:
		src_fmt = 14;
		break;
	case PIX_BUF_FMT_ABGR8888:
		src_fmt = 15;
		break;
	default:
		host1x_error("Invalid src format %u\n", src->format);
		return -EINVAL;
	}

	switch (dst->format) {
	case PIX_BUF_FMT_ARGB8888:
		dst_fmt = 14;
		break;
	case PIX_BUF_FMT_ABGR8888:
		dst_fmt = 15;
		break;
	default:
		host1x_error("Invalid dst format %u\n", dst->format);
		return -EINVAL;
	}

	inv_scale_x = (src_width - 1) / (float)(dst_width - 1);
	inv_scale_y = (src_height - 1) / (float)(dst_height - 1);

	if (inv_scale_x == 1.0f)
		hftype = 7;
	else if (inv_scale_x < 1.0f)
		hftype = 0;
	else if (inv_scale_x < 1.3f)
		hftype = 1;
	else if (inv_scale_x < 2.0f)
		hftype = 3;
	else
		hftype = 6;

	if (inv_scale_y == 1.0f) {
		vftype = 0;
		vfen = 0;
	} else {
		vfen = 1;

		if (inv_scale_y < 1.0f)
			vftype = 0;
		else if (inv_scale_y < 1.3f)
			vftype = 1;
		else if (inv_scale_y < 2.0f)
			vftype = 2;
		else
			vftype = 3;
	}

	job = HOST1X_JOB_CREATE(syncpt->id, 1);
	if (!job)
		return -ENOMEM;

	pb = HOST1X_JOB_APPEND(job, gr2d->commands, 0);
	if (!pb) {
		host1x_job_free(job);
		return -ENOMEM;
	}

	host1x_pushbuf_push(pb, HOST1X_OPCODE_SETCL(0, 0x52, 0));

	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x009, 0xF09));
	host1x_pushbuf_push(pb, 0x00000038); /* trigger */
	host1x_pushbuf_push(pb, 0x00000001); /* cmdsel */
	host1x_pushbuf_push(pb, FLOAT_TO_FIXED_6_12(inv_scale_y)); /* vdda */
	host1x_pushbuf_push(pb, FLOAT_TO_FIXED_0_8(sy)); /* vddaini */
	host1x_pushbuf_push(pb, FLOAT_TO_FIXED_6_12(inv_scale_x)); /* hdda */
	host1x_pushbuf_push(pb, FLOAT_TO_FIXED_0_8(sx)); /* hddainils */

	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x1c, 0xF));
	host1x_pushbuf_push(pb, dst_fmt << 8 | src_fmt); /* sbformat */
	host1x_pushbuf_push(pb,
			    hftype << 20 |
			    vfen << 18 | vftype << 16); /* controlsb */
	host1x_pushbuf_push(pb, 0x00000000); /* controlsecond */
	/*
	 * [20:20] source color depth (0: mono, 1: same)
	 * [17:16] destination color depth (0: 8 bpp, 1: 16 bpp, 2: 32 bpp)
	 */
	host1x_pushbuf_push(pb, /* controlmain */
			1 << 28 | 1 << 27 |
			(PIX_BUF_FORMAT_BYTES(dst->format) >> 1) << 16);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x046, 0xD));
	/*
	 * [20:20] destination write tile mode (0: linear, 1: tiled)
	 * [ 0: 0] tile mode Y/RGB (0: linear, 1: tiled)
	 */
	host1x_pushbuf_push(pb, dst_tiled << 20 | src_tiled); /* tilemode */
	HOST1X_PUSHBUF_RELOCATE(pb, src->bo,
				src->bo->offset + sb_offset(src, sx, sy), 0);
	host1x_pushbuf_push(pb, 0xdeadbeef); /* srcba_sb_surfbase */
	HOST1X_PUSHBUF_RELOCATE(pb, dst->bo,
				dst->bo->offset + sb_offset(dst, dx, dy), 0);
	host1x_pushbuf_push(pb, 0xdeadbeef); /* dstba_sb_surfbase */

	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x02b, 0x3149));
	HOST1X_PUSHBUF_RELOCATE(pb, dst->bo, dst->bo->offset, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef); /* dstba */
	host1x_pushbuf_push(pb, dst->pitch); /* dstst */
	HOST1X_PUSHBUF_RELOCATE(pb, src->bo, src->bo->offset, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef); /* srcba */
	host1x_pushbuf_push(pb, src->pitch); /* srcst */
	host1x_pushbuf_push(pb,
			    (src_height - 1) << 16 |
			    (src_width - 1)); /* srcsize */
	host1x_pushbuf_push(pb,
			    (dst_height - 1) << 16 |
			    (dst_width - 1)); /* dstsize */

	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x000, 1));
	host1x_pushbuf_push(pb, 0x000001 << 8 | syncpt->id);

	err = HOST1X_CLIENT_SUBMIT(gr2d->client, job);
	if (err < 0) {
		host1x_job_free(job);
		return err;
	}

	host1x_job_free(job);

	err = HOST1X_CLIENT_FLUSH(gr2d->client, &fence);
	if (err < 0)
		return err;

	err = HOST1X_CLIENT_WAIT(gr2d->client, fence, ~0u);
	if (err < 0)
		return err;

	return 0;
}
