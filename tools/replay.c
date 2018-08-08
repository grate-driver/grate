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

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>

#ifdef ENABLE_ZLIB
#include <zlib.h>
#endif

#ifdef ENABLE_LZ4
#include <lz4.h>
#endif

#include <libdrm/drm_fourcc.h>

#include "grate.h"
#include "host1x-private.h"
#include "list.h"
#include "record_replay.h"
#include "tegra_drm.h"

struct rep_ctx {
	struct list_head node;
	unsigned int id;
};

struct rep_bo {
	struct list_head node;
	struct host1x_bo *bo;
	unsigned int refcnt;
	unsigned int ctx_id;
	unsigned int id;
	uint32_t flags;
	char *map;
};

struct rep_framebuffer {
	struct list_head node;
	struct host1x_framebuffer *hfb;
	unsigned int bo_id;
	unsigned int ctx_id;
	unsigned format;
	unsigned width;
	unsigned height;
	unsigned pitch;
	bool reflect_y;
};

struct rep_job_ctx {
	struct list_head node;
	unsigned int id;
	bool gr2d;
};

static LIST_HEAD(ctx_list);
static LIST_HEAD(bo_list);
static LIST_HEAD(fb_list);
static LIST_HEAD(job_ctx_list);

static FILE *recfile;

static struct host1x *host1x;
static struct host1x_display *display;
static struct host1x_overlay *overlay;
static struct host1x_gr2d *gr2d;
static struct host1x_gr3d *gr3d;

static enum record_compression compression;
static struct rep_framebuffer *displayed_fb;

static const char *str_actions[] = {
	[REC_START] = "REC_START",
	[REC_INFO] = "REC_INFO",
	[REC_CTX_CREATE] = "REC_CTX_CREATE",
	[REC_CTX_DESTROY] = "REC_CTX_DESTROY",
	[REC_BO_CREATE] = "REC_BO_CREATE",
	[REC_BO_DESTROY] = "REC_BO_DESTROY",
	[REC_BO_LOAD_DATA] = "REC_BO_LOAD_DATA",
	[REC_BO_SET_FLAGS] = "REC_BO_SET_FLAGS",
	[REC_ADD_FRAMEBUFFER] = "REC_ADD_FRAMEBUFFER",
	[REC_DEL_FRAMEBUFFER] = "REC_DEL_FRAMEBUFFER",
	[REC_DISP_FRAMEBUFFER] = "REC_DISP_FRAMEBUFFER",
	[REC_JOB_CTX_CREATE] = "REC_JOB_CTX_CREATE",
	[REC_JOB_CTX_DESTROY] = "REC_JOB_CTX_DESTROY",
	[REC_JOB_SUBMIT] = "REC_JOB_SUBMIT",
};

static void create_context(unsigned int id)
{
	struct rep_ctx *ctx = calloc(1, sizeof(*ctx));
	assert(ctx != NULL);

	ctx->id = id;

	INIT_LIST_HEAD(&ctx->node);
	list_add_tail(&ctx->node, &ctx_list);
}

static struct rep_ctx *lookup_context(unsigned int id)
{
	struct rep_ctx *ctx;

	list_for_each_entry(ctx, &ctx_list, node)
		if (ctx->id == id)
			return ctx;

	return NULL;
}

static void destroy_context(unsigned int id)
{
	struct rep_ctx *ctx = lookup_context(id);
	assert(ctx != NULL);

	list_del(&ctx->node);
	free(ctx);
}

static void create_bo(unsigned int id, unsigned int ctx_id,
		      unsigned int size, uint32_t flags)
{
	struct rep_ctx *ctx;
	struct rep_bo *rbo;

	ctx = lookup_context(ctx_id);
	assert(ctx != NULL);

	rbo = calloc(1, sizeof(*rbo));
	assert(rbo != NULL);

	if (flags & DRM_TEGRA_GEM_CREATE_TILED)
		rbo->flags |= HOST1X_BO_CREATE_FLAG_TILED;

	if (flags & DRM_TEGRA_GEM_CREATE_BOTTOM_UP)
		rbo->flags |= HOST1X_BO_CREATE_FLAG_BOTTOM_UP;

	rbo->id = id;
	rbo->bo = HOST1X_BO_CREATE(host1x, size, rbo->flags);
	assert(rbo->bo != NULL);

	HOST1X_BO_MMAP(rbo->bo, (void *)&rbo->map);
	assert(rbo->map != NULL);

	INIT_LIST_HEAD(&rbo->node);
	list_add_tail(&rbo->node, &bo_list);
}

static struct rep_bo *lookup_bo(unsigned int id, unsigned int ctx_id)
{
	struct rep_bo *rbo;

	list_for_each_entry(rbo, &bo_list, node)
		if (rbo->id == id)
			return rbo;

	return NULL;
}

static void destroy_bo(unsigned int id, unsigned int ctx_id)
{
	struct rep_bo *rbo = lookup_bo(id, ctx_id);
	assert(rbo != NULL);

	list_del(&rbo->node);
	free(rbo);
}

#ifdef ENABLE_ZLIB
static void decompress_data_zlib(void *in, void *out,
				 size_t in_size, size_t out_size)
{
	z_stream strm;
	void *buf;
	int ret;

	/* utilizing CPU cache is way much faster than uncached DRAM */
	buf = alloca(out_size);

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = in_size;
	strm.next_in = in;
	strm.avail_out = out_size;
	strm.next_out = buf;

	ret = inflateInit(&strm);
	assert(ret == Z_OK);

	ret = inflate(&strm, Z_FINISH);
	assert(ret == Z_STREAM_END);

	ret = inflateEnd(&strm);
	assert(ret == Z_OK);

	memcpy(out, buf, out_size);
}
#endif

#ifdef ENABLE_LZ4
static void decompress_data_lz4(void *in, void *out,
				size_t in_size, size_t out_size)
{
	void *buf;
	int ret;

	/* utilizing CPU cache is way much faster than uncached DRAM */
	buf = alloca(out_size + 512);

	ret = LZ4_decompress_safe(in, buf, in_size, out_size + 512);
	assert(ret >= 0);

	memcpy(out, buf, out_size);
}
#endif

static int decompress_data(void *in, void *out,
			    size_t in_size, size_t out_size)
{
#ifdef ENABLE_ZLIB
	if (compression == REC_ZLIB) {
		decompress_data_zlib(in, out, in_size, out_size);
		return 1;
	}
#endif

#ifdef ENABLE_LZ4
	if (compression == REC_LZ4) {
		decompress_data_lz4(in, out, in_size, out_size);
		return 1;
	}
#endif

	return 0;
}

static int load_bo(unsigned int id, unsigned int ctx_id,
		   unsigned int page, unsigned int size)
{
	struct rep_bo *rbo = lookup_bo(id, ctx_id);
	uint8_t compressed[size];
	void *dest;
	int ret;

	assert(rbo != NULL);

	dest = rbo->map + page * 4096;

	if (size) {
		ret = fread(compressed, size, 1, recfile);
		if (ret == 1)
			ret = decompress_data(compressed, dest, size, 4096);
	} else {
		ret = fread(dest, 4096, 1, recfile);
	}

	return ret;
}

static void set_bo_flags(unsigned int id, unsigned int ctx_id, uint32_t flags)
{
	struct rep_bo *rbo = lookup_bo(id, ctx_id);
	assert(rbo != NULL);

	/* TODO: implement */
}

static void create_framebuffer(unsigned int bo_id, unsigned int ctx_id,
			       unsigned int width, unsigned int height,
			       unsigned int pitch, unsigned int format,
			       uint32_t flags)
{
	struct rep_bo *rbo = lookup_bo(bo_id, ctx_id);
	struct host1x_framebuffer *hfb;
	struct host1x_pixelbuffer *pb;
	struct rep_framebuffer *rfb;
	int err;

	assert(rbo != NULL);

	hfb = calloc(1, sizeof(*hfb));
	assert(hfb != NULL);

	pb = calloc(1, sizeof(*pb));
	assert(pb != NULL);

	rfb = calloc(1, sizeof(*rfb));
	assert(rfb != NULL);

	switch (format) {
	case DRM_FORMAT_XBGR8888:
		format = PIX_BUF_FMT_RGBA8888;
		break;

	case DRM_FORMAT_XRGB8888:
		format = PIX_BUF_FMT_RGBA8888;
		break;

	case DRM_FORMAT_RGB565:
		format = PIX_BUF_FMT_RGB565;
		break;

	default:
		fprintf(stderr, "Invalid FB format %u, can't continue\n",
			format);
		abort();
	}

	pb->bo = rbo->bo;
	pb->width = width;
	pb->height = height;
	pb->pitch = pitch;
	pb->format = format;
	pb->layout = PIX_BUF_LAYOUT_LINEAR;

	hfb->pixbuf = pb;
	hfb->flags = flags;

	if (rbo->flags & HOST1X_BO_CREATE_FLAG_TILED)
		pb->layout = PIX_BUF_LAYOUT_TILED_16x16;

	if (host1x->framebuffer_init) {
		err = host1x->framebuffer_init(host1x, hfb);
		if (err != 0) {
			fprintf(stderr, "FB init failed %d (%s), can't continue\n",
				err, strerror(-err));
			abort();
		}
	}

	rfb->hfb = hfb;
	rfb->bo_id = bo_id;
	rfb->ctx_id = ctx_id;
	rfb->format = format;
	rfb->width = width;
	rfb->height = height;
	rfb->pitch = pitch;
	rfb->reflect_y = !!(rbo->flags & HOST1X_BO_CREATE_FLAG_BOTTOM_UP);

	INIT_LIST_HEAD(&rfb->node);
	list_add_tail(&rfb->node, &fb_list);
}

static struct rep_framebuffer *lookup_framebuffer(unsigned int bo_id,
						  unsigned int ctx_id)
{
	struct rep_framebuffer *rfb;

	list_for_each_entry(rfb, &fb_list, node)
		if (rfb->bo_id == bo_id && rfb->ctx_id == ctx_id)
			return rfb;

	return NULL;
}

static void destroy_framebuffer(unsigned int bo_id, unsigned int ctx_id)
{
	struct rep_framebuffer *rfb = lookup_framebuffer(bo_id, ctx_id);
	assert(rfb != NULL);

	list_del(&rfb->node);
	host1x_framebuffer_free(rfb->hfb);
	free(rfb);

	if (displayed_fb == rfb)
		displayed_fb = NULL;
}

static void display_framebuffer(unsigned int bo_id, unsigned int ctx_id)
{
	struct rep_framebuffer *rfb = lookup_framebuffer(bo_id, ctx_id);
	assert(rfb != NULL);
	int err;

	printf("    displaying fb bo_id: %u\n", rfb->bo_id);

	if (displayed_fb == rfb)
		return;

	err = host1x_overlay_set(overlay, rfb->hfb, 0, 0,
				 rfb->width, rfb->height, false,
				 rfb->reflect_y);
	assert(err == 0);

	displayed_fb = rfb;
}

static void create_job_context(unsigned int id, bool gr2d)
{
	struct rep_job_ctx *job_ctx = calloc(1, sizeof(*job_ctx));
	assert(job_ctx != NULL);

	job_ctx->id = id;
	job_ctx->gr2d = gr2d;

	INIT_LIST_HEAD(&job_ctx->node);
	list_add_tail(&job_ctx->node, &job_ctx_list);
}

static struct rep_job_ctx *lookup_job_context(unsigned int id)
{
	struct rep_job_ctx *job_ctx;

	list_for_each_entry(job_ctx, &job_ctx_list, node)
		if (job_ctx->id == id)
			return job_ctx;

	return NULL;
}

static void destroy_job_context(unsigned int id)
{
	struct rep_job_ctx *job_ctx = lookup_job_context(id);
	assert(job_ctx != NULL);

	list_del(&job_ctx->node);
	free(job_ctx);
}

static int submit_job(unsigned int ctx_id,
		      unsigned int num_gathers,
		      unsigned int num_relocs,
		      unsigned int num_syncpt_incrs)
{
	struct rep_job_ctx *job_ctx;
	struct host1x_client *client;
	struct record_gather *gathers;
	struct host1x_syncpt *syncpt;
	struct rep_framebuffer *rfb;
	struct record_reloc *relocs;
	struct host1x_pushbuf *pb;
	struct host1x_job *job;
	struct rep_ctx *ctx;
	struct rep_bo *rbo;
	bool *handled_relocs;
	unsigned int size;
	unsigned int i, k;
	uint32_t fence;
	int ret;

	job_ctx = lookup_job_context(ctx_id);

	if (job_ctx->gr2d) {
		syncpt = &gr2d->client->syncpts[0];
		client = gr2d->client;

		printf("    gr2d\n");
	} else {
		syncpt = &gr3d->client->syncpts[0];
		client = gr3d->client;

		printf("    gr3d\n");
	}

	job = HOST1X_JOB_CREATE(syncpt->id, 1);
	if (!job)
		abort();

	if (num_gathers) {
		size = sizeof(*gathers) * num_gathers;
		gathers = alloca(size);
		ret = fread(gathers, size, 1, recfile);
		if (ret != 1)
			return ret;
	} else {
		gathers = NULL;
	}

	if (num_relocs) {
		size = sizeof(*relocs) * num_relocs;
		relocs = alloca(size);
		ret = fread(relocs, size, 1, recfile);
		if (ret != 1)
			return ret;
	} else {
		relocs = NULL;
	}

	handled_relocs = alloca(sizeof(bool) * num_relocs);
	rfb = NULL;

	for (i = 0; i < num_relocs; i++) {
		ctx = lookup_context(relocs[i].ctx_id);

		assert(ctx != NULL);
		assert(relocs[i].ctx_id == relocs[0].ctx_id);

		handled_relocs[i] = false;
	}

	if (num_relocs) {
		for (i = 0; i < num_gathers; i++) {
			printf("        gather %u bo_id: %u ctx_id: %u\n",
			       i, gathers[i].id, gathers[i].ctx_id);
			assert(gathers[i].ctx_id == relocs[0].ctx_id);
		}
	}

	for (i = 0; i < num_gathers; i++) {
		rbo = lookup_bo(gathers[i].id, gathers[i].ctx_id);
		assert(rbo != NULL);

		pb = HOST1X_JOB_APPEND(job, rbo->bo, 0);
		if (!pb)
			abort();

		for (k = 0; k < num_relocs; k++) {
			if (handled_relocs[k])
				continue;

			if (gathers[i].id != relocs[k].gather_id)
				continue;

			rbo = lookup_bo(relocs[k].id, relocs[k].ctx_id);
			assert(rbo != NULL);

			pb->ptr = pb->bo->ptr + relocs[k].patch_offset;

			ret = HOST1X_PUSHBUF_RELOCATE(pb, rbo->bo,
						      relocs[k].offset, 0);
			assert(ret == 0);

			handled_relocs[k] = true;
		}

		pb->offset = gathers[i].offset;
		pb->length = gathers[i].num_words;
	}

	for (i = 0; i < num_relocs; i++) {
		char *fbsrt = "";

		if (!rfb) {
			rfb = lookup_framebuffer(relocs[i].id, relocs[i].ctx_id);
			if (rfb)
				fbsrt = "(fb)";
		} else if (lookup_framebuffer(relocs[i].id, relocs[i].ctx_id)) {
			fbsrt = "(fb)";
		}

		printf("        reloc %u bo_id: %u ctx_id: %u offset: %u %s\n",
		       i, relocs[i].id, relocs[i].ctx_id, relocs[i].offset, fbsrt);
		assert(relocs[i].ctx_id == relocs[0].ctx_id);
		assert(handled_relocs[i]);
	}

	job->syncpt_incrs = num_syncpt_incrs;

	ret = HOST1X_CLIENT_SUBMIT(client, job);
	if (ret < 0)
		abort();

	host1x_job_free(job);

	ret = HOST1X_CLIENT_FLUSH(client, &fence);
	if (ret < 0)
		abort();

	ret = HOST1X_CLIENT_WAIT(client, fence, ~0u);
	if (ret < 0)
		abort();

	return 0;
}

static void handle_single_step(void)
{
	static bool ss = false;

	if (ss || grate_key_pressed2(NULL) == 32 /* Space */) {
		fprintf(stderr, "Press Space to single step or Enter to continue\n");

		while (1) {
			uint8_t key = grate_key_pressed2(NULL);

			if (key == 32 /* Space */) {
				ss = true;
				break;
			}

			if (key == 10 /* Enter */) {
				ss = false;
				break;
			}
		}
	} else {
		ss = false;
	}
}

int main(int argc, char *argv[])
{
	struct record_act r;
	unsigned int act_cnt = 0;
	char *path = NULL;
	int err = 1;
	int ret;
	int c;

	host1x = host1x_open(true, -1);
	if (!host1x) {
		fprintf(stderr, "host1x_open() failed\n");
		abort();
	}

	display = host1x_get_display(host1x);
	if (display) {
		ret = host1x_overlay_create(&overlay, display);
		if (ret < 0) {
			fprintf(stderr, "host1x_overlay_create() failed %d\n",
				ret);
			abort();
		}
	}

	gr2d = host1x_get_gr2d(host1x);
	if (!gr2d) {
		fprintf(stderr, "host1x_get_gr2d() failed\n");
		abort();
	}

	gr3d = host1x_get_gr3d(host1x);
	if (!gr3d) {
		fprintf(stderr, "host1x_get_gr3d() failed\n");
		abort();
	}

	do {
		struct option long_options[] =
		{
			{"recfile",	required_argument, NULL, 0},
			{ /* Sentinel */ }
		};
		int option_index = 0;

		c = getopt_long(argc, argv, "h", long_options, &option_index);

		switch (c) {
		case 0:
			switch (option_index) {
			case 0:
				path = optarg;
				break;

			default:
				return 0;
			}

		case -1:
			break;

		default:
			fprintf(stderr, "Invalid arguments\n\n");
			abort();
		}
	} while (c != -1);

	if (!path) {
		fprintf(stderr, "'--recfile path' is missing\n\n");
		abort();
	}

	recfile = fopen(path, "r");
	if (!recfile) {
		fprintf(stderr, "%s: Failed to open %s: %s\n",
			__func__, path, strerror(errno));
		abort();
	}

	do {
		ret = fread(&r.act, sizeof(r.act), 1, recfile);
		if (ret != 1) {
			if (feof(recfile))
				break;

			goto err_act;
		}

		if (act_cnt == 0)
			assert(r.act == REC_START);

		if (act_cnt == 1)
			assert(r.act == REC_INFO);

		if (r.act <= REC_JOB_SUBMIT)
			printf("replaying action %u: %s\n",
			       act_cnt, str_actions[r.act]);

		switch (r.act) {
		case REC_INFO:
			ret = fread(&r.data.record_info, sizeof(r.data.record_info), 1, recfile);
			if (ret != 1)
				goto err_act_data;

			printf("    drm: %u\n", r.data.record_info.drm);

			if (!r.data.record_info.drm)
				goto err_bad_info;

			compression = r.data.record_info.compression;

			printf("    compression: %u\n", compression);

			if (compression > REC_LZ4)
				goto err_bad_info;

			break;

		case REC_CTX_CREATE:
			ret = fread(&r.data, sizeof(r.data.ctx_create), 1, recfile);
			if (ret != 1)
				goto err_act_data;

			printf("    id: %u\n", r.data.ctx_create.id);

			create_context(r.data.ctx_create.id);

			break;

		case REC_CTX_DESTROY:
			ret = fread(&r.data, sizeof(r.data.ctx_destroy), 1, recfile);
			if (ret != 1)
				goto err_act_data;

			printf("    id: %u\n", r.data.ctx_destroy.id);

			destroy_context(r.data.ctx_destroy.id);

			break;

		case REC_BO_CREATE:
			ret = fread(&r.data, sizeof(r.data.bo_create), 1, recfile);
			if (ret != 1)
				goto err_act_data;

			printf("    bo_id: %u\n", r.data.bo_create.id);
			printf("    ctx_id: %u\n", r.data.bo_create.ctx_id);
			printf("    num_pages: %u\n", r.data.bo_create.num_pages);
			printf("    flags: 0x%08X\n", r.data.bo_create.flags);

			create_bo(r.data.bo_create.id, r.data.bo_create.ctx_id,
				  r.data.bo_create.num_pages * 4096,
				  r.data.bo_create.flags);

			break;

		case REC_BO_DESTROY:
			ret = fread(&r.data, sizeof(r.data.bo_destroy), 1, recfile);
			if (ret != 1)
				goto err_act_data;

			printf("    bo_id: %u\n", r.data.bo_destroy.id);
			printf("    ctx_id: %u\n", r.data.bo_destroy.ctx_id);

			destroy_bo(r.data.bo_destroy.id, r.data.bo_destroy.ctx_id);

			break;

		case REC_BO_LOAD_DATA:
			ret = fread(&r.data, sizeof(r.data.bo_load), 1, recfile);
			if (ret != 1)
				goto err_act_data;

			printf("    bo_id: %u\n", r.data.bo_load.id);
			printf("    ctx_id: %u\n", r.data.bo_load.ctx_id);
			printf("    page: %u\n", r.data.bo_load.page_id);

			ret = load_bo(r.data.bo_load.id, r.data.bo_load.ctx_id,
				      r.data.bo_load.page_id,
				      r.data.bo_load.data_size);
			if (ret != 1)
				goto err_act_data;

			break;

		case REC_BO_SET_FLAGS:
			ret = fread(&r.data, sizeof(r.data.bo_set_flags), 1, recfile);
			if (ret != 1)
				goto err_act_data;

			printf("    id: %u\n", r.data.bo_set_flags.id);
			printf("    ctx_id: %u\n", r.data.bo_set_flags.ctx_id);
			printf("    flags: 0x%08X\n", r.data.bo_set_flags.flags);

			set_bo_flags(r.data.bo_set_flags.id,
				     r.data.bo_set_flags.ctx_id,
				     r.data.bo_set_flags.flags);

			break;

		case REC_ADD_FRAMEBUFFER:
			ret = fread(&r.data, sizeof(r.data.add_framebuffer), 1, recfile);
			if (ret != 1)
				goto err_act_data;

			printf("    bo_id: %u\n", r.data.add_framebuffer.bo_id);
			printf("    ctx_id: %u\n", r.data.add_framebuffer.ctx_id);
			printf("    width: %u\n", r.data.add_framebuffer.width);
			printf("    height: %u\n", r.data.add_framebuffer.height);
			printf("    pitch: %u\n", r.data.add_framebuffer.pitch);
			printf("    format: %u\n", r.data.add_framebuffer.format);
			printf("    flags: %u\n", r.data.add_framebuffer.flags);

			create_framebuffer(r.data.add_framebuffer.bo_id,
					   r.data.add_framebuffer.ctx_id,
					   r.data.add_framebuffer.width,
					   r.data.add_framebuffer.height,
					   r.data.add_framebuffer.pitch,
					   r.data.add_framebuffer.format,
					   r.data.add_framebuffer.flags);

			break;

		case REC_DEL_FRAMEBUFFER:
			ret = fread(&r.data, sizeof(r.data.del_framebuffer), 1, recfile);
			if (ret != 1)
				goto err_act_data;

			printf("    bo_id: %u\n", r.data.del_framebuffer.bo_id);
			printf("    ctx_id: %u\n", r.data.del_framebuffer.ctx_id);

			destroy_framebuffer(r.data.del_framebuffer.bo_id,
					    r.data.del_framebuffer.ctx_id);

			break;

		case REC_DISP_FRAMEBUFFER:
			ret = fread(&r.data, sizeof(r.data.disp_framebuffer), 1, recfile);
			if (ret != 1)
				goto err_act_data;

			printf("    bo_id: %u\n", r.data.disp_framebuffer.bo_id);
			printf("    ctx_id: %u\n", r.data.disp_framebuffer.ctx_id);

			display_framebuffer(r.data.disp_framebuffer.bo_id,
					    r.data.disp_framebuffer.ctx_id);

			break;

		case REC_JOB_CTX_CREATE:
			ret = fread(&r.data, sizeof(r.data.job_ctx_create), 1, recfile);
			if (ret != 1)
				goto err_act_data;

			printf("    id: %u\n", r.data.job_ctx_create.id);
			printf("    gr2d: %u\n", r.data.job_ctx_create.gr2d);

			create_job_context(r.data.job_ctx_create.id,
					   r.data.job_ctx_create.gr2d);

			break;

		case REC_JOB_CTX_DESTROY:
			ret = fread(&r.data, sizeof(r.data.job_ctx_destroy), 1, recfile);
			if (ret != 1)
				goto err_act_data;

			printf("    id: %u\n", r.data.job_ctx_destroy.id);

			destroy_job_context(r.data.job_ctx_destroy.id);

			break;

		case REC_JOB_SUBMIT:
			ret = fread(&r.data, sizeof(r.data.job_submit), 1, recfile);
			if (ret != 1)
				goto err_act_data;

			printf("    job_ctx_id: %u\n", r.data.job_submit.job_ctx_id);
			printf("    num_gathers: %u\n", r.data.job_submit.num_gathers);
			printf("    num_relocs: %u\n", r.data.job_submit.num_relocs);
			printf("    num_syncpt_incrs: %u\n", r.data.job_submit.num_syncpt_incrs);

			ret = submit_job(r.data.job_submit.job_ctx_id,
					 r.data.job_submit.num_gathers,
					 r.data.job_submit.num_relocs,
					 r.data.job_submit.num_syncpt_incrs);
			if (ret != 0)
				goto err_act_data;

			handle_single_step();

			break;

		case REC_START:
			ret = fread(&r.data, sizeof(r.data.header), 1, recfile);
			if (ret != 1)
				goto err_act_data;

			if (strncmp(r.data.header.magic, REC_MAGIC,
				    strlen(REC_MAGIC) != 0))
				goto err_invalid_header;

			if (r.data.header.version != 4)
				goto err_invalid_version;

			break;

		default:
			goto err_invalid_action;
		}

		act_cnt++;
	} while (1);

	fprintf(stderr, "\n\nEnd of record file reached\n");

stop:
	fprintf(stderr, "Press Enter to exit\n");
	grate_wait_for_key(NULL);

exit:
	grate_exit(NULL);

	return err;

err_act_data:
	fprintf(stderr, "Failed to read %s data (%s), stopping\n",
		str_actions[r.act], strerror(errno));
	goto stop;

err_invalid_action:
	fprintf(stderr, "Invalid action %u, stopping\n", r.act);
	goto stop;

err_act:
	fprintf(stderr, "Failed to read action: %d (%s), stopping\n",
		ret, strerror(errno));
	goto stop;

err_invalid_version:
	fprintf(stderr, "Invalid record version, can't continue\n");
	goto exit;

err_bad_info:
	fprintf(stderr, "Invalid record info, can't continue\n");
	goto exit;

err_invalid_header:
	fprintf(stderr, "Invalid record file, can't continue\n");
	goto exit;
}
