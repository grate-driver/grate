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

#define _LARGEFILE64_SOURCE

#ifdef ENABLE_ZLIB
#include <zlib.h>
#endif

#ifdef ENABLE_LZ4
#include <lz4.h>
#endif

#include "recorder.h"

static struct recorder rec;
static enum record_compression compression = REC_UNCOMPRESSED;

static unsigned long calc_page_checksum(void *page_data, size_t size)
{
#ifdef ENABLE_ZLIB
	return adler32(0, page_data, size);
#endif
	return 0;
}

static void record_write_data(void *data, size_t size)
{
	int ret = fwrite(data, size, 1, rec.fout);
	if (ret != 1) {
		fprintf(stderr, "%s: size %zu: Failed: %d (%s)\n",
			__func__, size, ret, strerror(errno));
		abort();
	}
}

static void record_write_action(struct record_act *r)
{
	size_t size = sizeof(r->act);
	void *data = r;

	switch (r->act) {
	case REC_START:
		size += sizeof(r->data.header);
		break;

	case REC_INFO:
		size += sizeof(r->data.record_info);
		break;

	case REC_CTX_CREATE:
		size += sizeof(r->data.ctx_create);
		break;

	case REC_CTX_DESTROY:
		size += sizeof(r->data.ctx_destroy);
		break;

	case REC_BO_CREATE:
		size += sizeof(r->data.bo_create);
		break;

	case REC_BO_DESTROY:
		size += sizeof(r->data.bo_destroy);
		break;

	case REC_BO_LOAD_DATA:
		size += sizeof(r->data.bo_load);
		break;

	case REC_BO_SET_FLAGS:
		size += sizeof(r->data.bo_set_flags);
		break;

	case REC_ADD_FRAMEBUFFER:
		size += sizeof(r->data.add_framebuffer);
		break;

	case REC_DEL_FRAMEBUFFER:
		size += sizeof(r->data.del_framebuffer);
		break;

	case REC_DISP_FRAMEBUFFER:
		size += sizeof(r->data.disp_framebuffer);
		break;

	case REC_JOB_CTX_CREATE:
		size += sizeof(r->data.job_ctx_create);
		break;

	case REC_JOB_CTX_DESTROY:
		size += sizeof(r->data.job_ctx_destroy);
		break;

	case REC_JOB_SUBMIT:
		size += sizeof(r->data.job_submit);
		break;

	default:
		fprintf(stderr, "%s: ERROR: invalid action %u\n",
			__func__, r->act);
		abort();
	}

	record_write_data(data, size);
}

static bool record_start(void)
{
	struct record_act r;
	uint8_t buf[4096];
	char *path;

	path = getenv("LIBWRAP_RECORD_PATH");
	if (!path)
		return false;

	rec.fout = fopen(path, "w");
	if (!rec.fout) {
		fprintf(stderr, "%s: Failed to open %s: %s\n",
			__func__, path, strerror(errno));
		abort();
	}

	memset(buf, 0, 4096);
	rec.zeroed_page_chksum = calc_page_checksum(buf, 4096);

	r.act = REC_START;
	r.data.header.version = REC_VER;
	strncpy(r.data.header.magic, REC_MAGIC, sizeof(r.data.header.magic));

	record_write_action(&r);

	r.act = REC_INFO;
	r.data.record_info.drm = access("/dev/nvhost-ctrl", F_OK) == -1;

#ifdef ENABLE_ZLIB
	compression = REC_ZLIB;
#endif
#ifdef ENABLE_LZ4
	compression = REC_LZ4;
#endif
	r.data.record_info.compression = compression;

	record_write_action(&r);

	fprintf(stderr, "libwrap recording started on %s to %s\n",
		r.data.record_info.drm ? "DRM" : "NVHOST", path);

	return true;
}

bool recorder_enabled(void)
{
	if (rec.inited)
		return rec.enabled;

	rec.inited = true;
	rec.enabled = record_start();

	return rec.enabled;
}

struct rec_ctx *record_create_ctx(void)
{
	struct record_act r;
	struct rec_ctx *ctx;

	if (!recorder_enabled())
		return NULL;

	ctx = calloc(1, sizeof(*ctx));
	assert(ctx != NULL);

	ctx->id = rec.ctx_cnt++;

	r.act = REC_CTX_CREATE;
	r.data.ctx_create.id = ctx->id;

	record_write_action(&r);

	return ctx;
}

void record_destroy_ctx(struct rec_ctx *ctx)
{
	struct record_act r;

	if (!recorder_enabled())
		return;

	r.act = REC_CTX_DESTROY;
	r.data.ctx_destroy.id = ctx->id;

	free(ctx);

	record_write_action(&r);
}

struct bo_rec *record_create_bo(struct rec_ctx *ctx, size_t size,
				uint32_t flags)
{
	struct record_act r;
	struct bo_rec *bo;
	unsigned int i;

	if (!recorder_enabled())
		return NULL;

	bo = calloc(1, sizeof(*bo));
	assert(bo != NULL);

	bo->id = rec.bos_cnt++;
	bo->ctx = ctx;
	bo->num_pages = (size + 4095) / 4096;
	bo->page_meta = malloc(sizeof(*bo->page_meta) * bo->num_pages);
	bo->flags = flags;

	assert(bo->page_meta != NULL);

	for (i = 0; i < bo->num_pages; i++)
		bo->page_meta[i].chksum = rec.zeroed_page_chksum;

	r.act = REC_BO_CREATE;
	r.data.bo_create.id = bo->id;
	r.data.bo_create.ctx_id = bo->ctx->id;
	r.data.bo_create.num_pages = bo->num_pages;
	r.data.bo_create.flags = bo->flags;

	record_write_action(&r);

	return bo;
}

void record_set_bo_data(struct bo_rec *bo, void *data)
{
	if (!recorder_enabled())
		return;

	bo->page_data = data;
}

void record_destroy_bo(struct bo_rec *bo)
{
	struct record_act r;

	if (!recorder_enabled())
		return;

	r.act = REC_BO_DESTROY;
	r.data.bo_destroy.id = bo->id;
	r.data.bo_destroy.ctx_id = bo->ctx->id;

	free(bo);

	record_write_action(&r);
}

#ifdef ENABLE_ZLIB
static size_t compress_data_zlib(void *in, void *out,
				 size_t in_size, size_t out_size)
{
	z_stream strm;
	int ret;

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = in_size;
	strm.next_in = in;
	strm.avail_out = out_size;
	strm.next_out = out;

	ret = deflateInit(&strm, Z_BEST_SPEED);
	assert(ret == Z_OK);

	ret = deflate(&strm, Z_FINISH);
	if (ret != Z_STREAM_END) {
		fprintf(stderr, "%s: ERROR: deflate failed: %d\n",
			__func__, ret);
		abort();
	}

	ret = deflateEnd(&strm);
	assert(ret == Z_OK);

	if (strm.total_out >= in_size)
		return 0;

	return strm.total_out;
}
#endif

#ifdef ENABLE_LZ4
static size_t compress_data_lz4(void *in, void *out,
				size_t in_size, size_t out_size)
{
	return LZ4_compress_default(in, out, in_size, out_size);
}
#endif

static size_t compress_data(void *in, void *out,
			    size_t in_size, size_t out_size)
{
#ifdef ENABLE_LZ4
	if (compression == REC_LZ4)
		return compress_data_lz4(in, out, in_size, out_size);
#endif

#ifdef ENABLE_ZLIB
	if (compression == REC_ZLIB)
		return compress_data_zlib(in, out, in_size, out_size);
#endif

	return 0;
}

static bool check_and_load_page(struct bo_rec *bo, unsigned int page)
{
	struct record_act r;
	unsigned long chksum;
	uint16_t data_size;
	uint8_t compressed[3328];
	uint8_t buf[4096];
	void *data;

	memcpy(buf, bo->page_data[page].data, 4096);

	chksum = calc_page_checksum(buf, 4096);

#ifdef ENABLE_ZLIB
	if (chksum == bo->page_meta[page].chksum)
		return false;
#endif

	/* size 0 means go uncompressed */
	data_size = compress_data(buf, compressed, 4096, 3328);
	data = data_size ? compressed : buf;

	r.act = REC_BO_LOAD_DATA;
	r.data.bo_load.id = bo->id;
	r.data.bo_load.page_id = page;
	r.data.bo_load.ctx_id = bo->ctx->id;
	r.data.bo_load.data_size = data_size;

	record_write_action(&r);
	record_write_data(data, data_size ?: 4096);

	bo->page_meta[page].chksum = chksum;

	return true;
}

void record_capture_bo_data(struct bo_rec *bo, bool force)
{
	unsigned int hpage[3];
	unsigned int i;
	bool dirty = false;

	if (!recorder_enabled())
		return;

	if (!bo->page_data)
		return;

	if (!bo->captured)
		force = true;
	else if (!force && bo->is_framebuffer)
		return;

	/*
	 * Lame heuristics.. check first, middle and last pages for changes
	 * and if these pages aren't changed, assume BO isn't changed at all.
	 */
	hpage[0] = 0;
	hpage[1] = bo->num_pages / 2;
	hpage[2] = bo->num_pages - 1;

	if (!force && bo->num_pages > 4) {
		dirty |= check_and_load_page(bo, hpage[0]);
		dirty |= check_and_load_page(bo, hpage[1]);
		dirty |= check_and_load_page(bo, hpage[2]);
	} else {
		force = true;
	}

	if (force || dirty)
		for (i = 0; i < bo->num_pages; i++)
			if (force || (i != hpage[0] && i != hpage[1] && i != hpage[2]))
				check_and_load_page(bo, i);

	bo->captured = true;
}

void record_set_bo_flags(struct bo_rec *bo, uint32_t flags)
{
	struct record_act r;

	if (!recorder_enabled())
		return;

	bo->flags = flags;

	r.act = REC_BO_SET_FLAGS;
	r.data.bo_set_flags.id = bo->id;
	r.data.bo_set_flags.ctx_id = bo->ctx->id;
	r.data.bo_set_flags.flags = bo->flags;

	record_write_action(&r);
}

void record_add_framebuffer(struct bo_rec *bo, uint32_t flags)
{
	struct record_act r;

	if (!recorder_enabled())
		return;

	bo->is_framebuffer = true;

	r.act = REC_ADD_FRAMEBUFFER;
	r.data.add_framebuffer.bo_id = bo->id;
	r.data.add_framebuffer.ctx_id = bo->ctx->id;
	r.data.add_framebuffer.width = bo->width;
	r.data.add_framebuffer.height = bo->height;
	r.data.add_framebuffer.pitch = bo->pitch;
	r.data.add_framebuffer.format = bo->format;
	r.data.add_framebuffer.flags = flags;

	record_write_action(&r);
}

void record_del_framebuffer(struct bo_rec *bo)
{
	struct record_act r;

	if (!recorder_enabled())
		return;

	bo->is_framebuffer = false;
	bo->fb_id = 0;

	r.act = REC_DEL_FRAMEBUFFER;
	r.data.del_framebuffer.bo_id = bo->id;
	r.data.del_framebuffer.ctx_id = bo->ctx->id;

	record_write_action(&r);
}

void record_display_framebuffer(struct bo_rec *bo)
{
	struct record_act r;

	if (!recorder_enabled())
		return;

	r.act = REC_DISP_FRAMEBUFFER;
	r.data.disp_framebuffer.bo_id = bo->id;
	r.data.disp_framebuffer.ctx_id = bo->ctx->id;

	record_write_action(&r);
}

struct job_ctx_rec *record_job_ctx_create(bool gr2d)
{
	struct record_act r;
	struct job_ctx_rec *ctx;

	if (!recorder_enabled())
		return NULL;

	ctx = calloc(1, sizeof(*ctx));
	assert(ctx != NULL);

	ctx->id = rec.job_ctx_cnt++;
	ctx->gr2d = gr2d;

	r.act = REC_JOB_CTX_CREATE;
	r.data.job_ctx_create.id = ctx->id;
	r.data.job_ctx_create.gr2d = ctx->gr2d;

	record_write_action(&r);

	return ctx;
}

void record_job_ctx_destroy(struct job_ctx_rec *ctx)
{
	struct record_act r;

	if (!recorder_enabled())
		return;

	r.act = REC_JOB_CTX_DESTROY;
	r.data.job_ctx_destroy.id = ctx->id;

	free(ctx);

	record_write_action(&r);
}

void record_job_submit(struct job_rec *job)
{
	struct record_act r;

	if (!recorder_enabled())
		return;

	r.act = REC_JOB_SUBMIT;
	r.data.job_submit.job_ctx_id = job->ctx->id;
	r.data.job_submit.num_gathers = job->num_gathers;
	r.data.job_submit.num_relocs = job->num_relocs;
	r.data.job_submit.num_syncpt_incrs = job->num_syncpt_incrs;

	record_write_action(&r);

	if (job->num_gathers)
		record_write_data(job->gathers,
				sizeof(*job->gathers) * job->num_gathers);

	if (job->num_relocs)
		record_write_data(job->relocs,
				sizeof(*job->relocs) * job->num_relocs);
}
