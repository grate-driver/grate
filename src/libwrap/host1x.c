/*
 * Copyright (c) 2018 Dmitry Osipenko
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

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>

#include "cdma_parser.h"
#include "disasm.h"
#include "host1x.h"
#include "syscall.h"
#include "recorder.h"
#include "drm_fourcc.h"

enum host1x_class {
	HOST1X_CLASS_GR2D = 0x51,
	HOST1X_CLASS_GR3D = 0x60,
};

struct drm_context {
	uint32_t id;
	uint32_t client;

	struct list_head list;

	struct job_ctx_rec *rec_job_ctx;
};

struct host1x_file {
	struct list_head bos;
	struct list_head contexts;
	struct file file;

	struct rec_ctx *rec_ctx;
};

struct host1x_bo {
	int id;
	size_t size;
	char *mapped;

	struct list_head list;

	struct bo_rec *rec_bo;
};

static inline struct host1x_file *to_host1x_file(struct file *file)
{
	return container_of(file, struct host1x_file, file);
}

static const struct ioctl host1x_ioctls[] = {
	IOCTL(DRM_IOCTL_TEGRA_GEM_CREATE),
	IOCTL(DRM_IOCTL_TEGRA_GEM_MMAP),
	IOCTL(DRM_IOCTL_TEGRA_SYNCPT_READ),
	IOCTL(DRM_IOCTL_TEGRA_SYNCPT_INCR),
	IOCTL(DRM_IOCTL_TEGRA_SYNCPT_WAIT),
	IOCTL(DRM_IOCTL_TEGRA_OPEN_CHANNEL),
	IOCTL(DRM_IOCTL_TEGRA_CLOSE_CHANNEL),
	IOCTL(DRM_IOCTL_TEGRA_GET_SYNCPT),
	IOCTL(DRM_IOCTL_TEGRA_SUBMIT),
	IOCTL(DRM_IOCTL_TEGRA_GET_SYNCPT_BASE),
	IOCTL(DRM_IOCTL_TEGRA_GEM_SET_TILING),
	IOCTL(DRM_IOCTL_TEGRA_GEM_GET_TILING),
	IOCTL(DRM_IOCTL_TEGRA_GEM_SET_FLAGS),
	IOCTL(DRM_IOCTL_TEGRA_GEM_GET_FLAGS),
	IOCTL(DRM_IOCTL_MODE_CREATE_DUMB),
	IOCTL(DRM_IOCTL_MODE_MAP_DUMB),
	IOCTL(DRM_IOCTL_MODE_DESTROY_DUMB),
	IOCTL(DRM_IOCTL_GEM_CLOSE),
	IOCTL(DRM_IOCTL_MODE_ADDFB),
	IOCTL(DRM_IOCTL_MODE_ADDFB2),
	IOCTL(DRM_IOCTL_MODE_RMFB),
	IOCTL(DRM_IOCTL_MODE_SETPLANE),
	IOCTL(DRM_IOCTL_MODE_SETCRTC),
	IOCTL(DRM_IOCTL_MODE_PAGE_FLIP),
	IOCTL(DRM_IOCTL_GEM_OPEN),
};

static struct host1x_bo *host1x_bo_new(struct host1x_file *host1x,
				       unsigned long id, size_t size,
				       uint32_t flags)
{
	struct host1x_bo *bo;

	bo = calloc(1, sizeof(*bo));
	if (!bo)
		return NULL;

	INIT_LIST_HEAD(&bo->list);
	bo->size = size;
	bo->id = id;
	bo->rec_bo = record_create_bo(host1x->rec_ctx, size, flags);

	return bo;
}

static void host1x_bo_free(struct host1x_bo *bo)
{
	if (bo->mapped)
		munmap_orig(bo->mapped, bo->size);

	record_destroy_bo(bo->rec_bo);
	list_del(&bo->list);
	free(bo);
}

static struct host1x_bo *host1x_file_lookup_bo(struct host1x_file *host1x,
					       unsigned long id)
{
	struct host1x_bo *bo;

	list_for_each_entry(bo, &host1x->bos, list)
		if (bo->id == id)
			return bo;

	return NULL;
}

static struct drm_context *host1x_context_new(unsigned long id, uint32_t client)
{
	struct drm_context *ctx;

	ctx = calloc(1, sizeof(*ctx));
	if (!ctx)
		return NULL;

	INIT_LIST_HEAD(&ctx->list);
	ctx->client = client;
	ctx->id = id;
	ctx->rec_job_ctx = record_job_ctx_create(client == HOST1X_CLASS_GR2D);

	return ctx;
}

static void host1x_context_free(struct drm_context *ctx)
{
	record_job_ctx_destroy(ctx->rec_job_ctx);
	list_del(&ctx->list);
	free(ctx);
}

static struct drm_context *host1x_file_lookup_context(struct host1x_file *host1x,
						       unsigned long id)
{
	struct drm_context *ctx;

	list_for_each_entry(ctx, &host1x->contexts, list)
		if (ctx->id == id)
			return ctx;

	return NULL;
}

static void host1x_gem_flags_to_string(char *str, uint32_t flags)
{
	if (flags & DRM_TEGRA_GEM_CREATE_TILED)
		strcat(str, " tiled");

	if (flags & DRM_TEGRA_GEM_CREATE_BOTTOM_UP)
		strcat(str, " bottom-up");

	if (flags & ~(DRM_TEGRA_GEM_CREATE_TILED | DRM_TEGRA_GEM_CREATE_BOTTOM_UP))
		strcat(str, " garbage");
}

static void host1x_file_enter_ioctl_syncpt_incr(struct host1x_file *host1x,
						struct drm_tegra_syncpt_incr *args)
{
	PRINTF("  SYNCPT increment:\n");
	PRINTF("    id: %u\n", args->id);
}

static void host1x_file_enter_ioctl_syncpt_wait(struct host1x_file *host1x,
						struct drm_tegra_syncpt_wait *args)
{
	PRINTF("  SYNCPT wait:\n");
	PRINTF("    id: %u\n", args->id);
	PRINTF("    thresh: %u\n", args->thresh);
	PRINTF("    timeout: %u\n", args->timeout);
}

static void host1x_file_enter_ioctl_submit(struct host1x_file *host1x,
					   struct drm_tegra_submit *args)
{
	struct drm_tegra_cmdbuf *cmdbuf = (void *)(intptr_t)args->cmdbufs;
	struct drm_tegra_reloc *reloc = (void *)(intptr_t)args->relocs;
	struct drm_tegra_syncpt *syncpt = (void *)(intptr_t)args->syncpts;
	struct host1x_bo *bo;
	struct drm_context *ctx;
	struct disasm_state d;
	struct job_rec j;
	uint32_t classid;
	unsigned int *handles;
	unsigned int num_handles;
	unsigned int i, k;

	PRINTF("  SUBMIT:\n");
	PRINTF("    context: %llu\n", args->context);
	PRINTF("    num_syncpts: %u\n", args->num_syncpts);
	PRINTF("    num_cmdbufs: %u\n", args->num_cmdbufs);
	PRINTF("    num_relocs: %u\n", args->num_relocs);
	PRINTF("    num_waitchks: %u\n", args->num_waitchks);
	PRINTF("    waitchk_mask: %x\n", args->waitchk_mask);
	PRINTF("    timeout: %u\n", args->timeout);
	PRINTF("    syncpts: 0x%08llx\n", args->syncpts);
	PRINTF("    cmdbufs: 0x%08llx\n", args->cmdbufs);
	PRINTF("    relocs: 0x%08llx\n", args->relocs);
	PRINTF("    waitchks: 0x%08llx\n", args->waitchks);

	ctx = host1x_file_lookup_context(host1x, args->context);
	if (!ctx) {
		fprintf(stderr, "invalid context: %llu\n", args->context);
		abort();
	}

	classid = ctx->client;
	disasm_reset(&d);

	j.num_gathers = args->num_cmdbufs;
	j.num_relocs = args->num_relocs;
	j.num_syncpt_incrs = syncpt->incrs;
	j.gathers = alloca(sizeof(*j.gathers) * args->num_cmdbufs);
	j.relocs = alloca(sizeof(*j.relocs) * args->num_relocs);
	j.ctx = ctx->rec_job_ctx;

	for (i = 0; i < args->num_cmdbufs; i++, cmdbuf++) {
		uint32_t *commands;

		bo = host1x_file_lookup_bo(host1x, cmdbuf->handle);
		if (!bo) {
			fprintf(stderr, "invalid handle: %u\n", cmdbuf->handle);
			abort();
		}

		if (!bo->mapped) {
			fprintf(stderr, "cmdbuf %u not mapped\n", i);
			abort();
		}

		commands = (uint32_t *)(bo->mapped + cmdbuf->offset);
		cdma_parse_commands(commands, cmdbuf->words, libwrap_verbose,
				    &classid, &d, disasm_write_reg);

		if (!recorder_enabled())
			continue;

		j.gathers[i].id = bo->rec_bo->id;
		j.gathers[i].ctx_id = bo->rec_bo->ctx->id;
		j.gathers[i].offset = cmdbuf->offset;
		j.gathers[i].num_words = cmdbuf->words;

		record_capture_bo_data(bo->rec_bo, false);
	}

	if (!recorder_enabled())
		return;

	handles = alloca(sizeof(handles) * args->num_relocs);
	num_handles = 0;

	for (i = 0; i < args->num_relocs; i++)
		handles[i] = ~0u;

	for (i = 0; i < args->num_relocs; i++, reloc++) {
		bo = host1x_file_lookup_bo(host1x, reloc->cmdbuf.handle);
		if (!bo) {
			fprintf(stderr, "invalid gather handle: %u\n",
				reloc->target.handle);
			abort();
		}

		j.relocs[i].gather_id = bo->rec_bo->id;
		j.relocs[i].patch_offset = reloc->cmdbuf.offset;

		bo = host1x_file_lookup_bo(host1x, reloc->target.handle);
		if (!bo) {
			fprintf(stderr, "invalid handle: %u\n",
				reloc->target.handle);
			abort();
		}

		j.relocs[i].id = bo->rec_bo->id;
		j.relocs[i].ctx_id = bo->rec_bo->ctx->id;
		j.relocs[i].offset = reloc->target.offset;

		/* skip duplicated BO's */
		for (k = 0; k < num_handles; k++)
			if (handles[k] == reloc->target.handle)
				break;

		if (k < num_handles)
			continue;

		handles[num_handles++] = reloc->target.handle;

		if (!bo->mapped)
			continue;

		record_capture_bo_data(bo->rec_bo, false);
	}

	record_job_submit(&j);
}

static void host1x_file_enter_ioctl_gem_set_tiling(struct host1x_file *host1x,
						   struct drm_tegra_gem_set_tiling *args)
{
	struct host1x_bo *bo;

	PRINTF("  Set GEM tiling:\n");
	PRINTF("    handle: %u\n", args->handle);
	PRINTF("    mode: %u\n", args->mode);
	PRINTF("    value: %u\n", args->value);

	bo = host1x_file_lookup_bo(host1x, args->handle);
	if (!bo) {
		fprintf(stderr, "invalid handle: %x\n", args->handle);
		abort();
	}
}

static void host1x_file_enter_ioctl_gem_set_flags(struct host1x_file *host1x,
						  struct drm_tegra_gem_set_flags *args)
{
	struct host1x_bo *bo;

	PRINTF("  Set GEM flags:\n");
	PRINTF("    handle: %u\n", args->handle);
	PRINTF("    flags: %u\n", args->flags);

	bo = host1x_file_lookup_bo(host1x, args->handle);
	if (!bo) {
		fprintf(stderr, "invalid handle: %x\n", args->handle);
		abort();
	}

	if (!recorder_enabled())
		return;

	record_set_bo_flags(bo->rec_bo, args->flags);
}

static struct host1x_bo *host1x_file_lookup_fb(struct host1x_file *host1x,
					       unsigned long fb_id)
{
	struct host1x_bo *bo;

	if (!fb_id)
		return NULL;

	list_for_each_entry(bo, &host1x->bos, list)
		if (bo->rec_bo->fb_id == fb_id &&
		    bo->rec_bo->ctx->id == host1x->rec_ctx->id)
			return bo;

	return NULL;
}

static void host1x_file_enter_ioctl_mode_setplane(struct host1x_file *host1x,
						  struct drm_mode_set_plane *args)
{
	struct host1x_bo *bo;

	PRINTF("  Set plane:\n");
	PRINTF("    fb_id: %u\n", args->fb_id);

	if (!recorder_enabled())
		return;

	bo = host1x_file_lookup_fb(host1x, args->fb_id);
	if (bo)
		return record_display_framebuffer(bo->rec_bo);
}

static void host1x_file_enter_ioctl_mode_setcrtc(struct host1x_file *host1x,
						 struct drm_mode_crtc *args)
{
	struct host1x_bo *bo;

	PRINTF("  Set CRTC:\n");
	PRINTF("    fb_id: %u\n", args->fb_id);

	if (!recorder_enabled())
		return;

	bo = host1x_file_lookup_fb(host1x, args->fb_id);
	if (bo)
		return record_display_framebuffer(bo->rec_bo);
}

static void host1x_file_enter_ioctl_mode_page_flip(struct host1x_file *host1x,
						   struct drm_mode_crtc_page_flip *args)
{
	struct host1x_bo *bo;

	PRINTF("  Flip CRTC:\n");
	PRINTF("    fb_id: %u\n", args->fb_id);

	if (!recorder_enabled())
		return;

	bo = host1x_file_lookup_fb(host1x, args->fb_id);
	if (bo)
		return record_display_framebuffer(bo->rec_bo);
}

static void host1x_file_enter_ioctl_rmfb(struct host1x_file *host1x,
					 unsigned long *fb_id)
{
	struct host1x_bo *bo;

	PRINTF("  Remove FB:\n");
	PRINTF("    fb_id: %lu\n", *fb_id);

	if (!recorder_enabled())
		return;

	bo = host1x_file_lookup_fb(host1x, *fb_id);
	if (bo)
		return record_del_framebuffer(bo->rec_bo);
}

static int host1x_file_enter_ioctl(struct file *file, unsigned long request,
				   void *arg)
{
	struct host1x_file *host1x = to_host1x_file(file);

	switch (request) {
	case DRM_IOCTL_TEGRA_SYNCPT_INCR:
		host1x_file_enter_ioctl_syncpt_incr(host1x, arg);
		break;

	case DRM_IOCTL_TEGRA_SYNCPT_WAIT:
		host1x_file_enter_ioctl_syncpt_wait(host1x, arg);
		break;

	case DRM_IOCTL_TEGRA_SUBMIT:
		host1x_file_enter_ioctl_submit(host1x, arg);
		break;

	case DRM_IOCTL_TEGRA_GEM_SET_TILING:
		host1x_file_enter_ioctl_gem_set_tiling(host1x, arg);
		break;

	case DRM_IOCTL_TEGRA_GEM_SET_FLAGS:
		host1x_file_enter_ioctl_gem_set_flags(host1x, arg);
		break;

	case DRM_IOCTL_MODE_SETPLANE:
		host1x_file_enter_ioctl_mode_setplane(host1x, arg);
		break;

	case DRM_IOCTL_MODE_SETCRTC:
		host1x_file_enter_ioctl_mode_setcrtc(host1x, arg);
		break;

	case DRM_IOCTL_MODE_PAGE_FLIP:
		host1x_file_enter_ioctl_mode_page_flip(host1x, arg);
		break;

	case DRM_IOCTL_MODE_RMFB:
		host1x_file_enter_ioctl_rmfb(host1x, arg);
		break;

	default:
		break;
	}

	return 0;
}

static void host1x_file_leave_ioctl_gem_create(struct host1x_file *host1x,
					       struct drm_tegra_gem_create *args)
{
	struct host1x_bo *bo;
	char buf[256] = { };

	host1x_gem_flags_to_string(buf, args->flags);

	PRINTF("  GEM created:\n");
	PRINTF("    handle: %u\n", args->handle);
	PRINTF("    size: %llx\n", args->size);
	PRINTF("    flags: %x (%s )\n", args->flags, buf);

	bo = host1x_bo_new(host1x, args->handle, args->size, args->flags);
	if (!bo) {
		fprintf(stderr, "failed to create bo\n");
		abort();
	}

	list_add_tail(&bo->list, &host1x->bos);
}

static void host1x_file_leave_ioctl_gem_mmap(struct host1x_file *host1x,
					     struct drm_tegra_gem_mmap *args)
{
	struct host1x_bo *bo;

	PRINTF("  GEM mapped:\n");
	PRINTF("    handle: %u\n", args->handle);
	PRINTF("    offset: %llx\n", args->offset);

	bo = host1x_file_lookup_bo(host1x, args->handle);
	if (!bo) {
		fprintf(stderr, "invalid handle: %x\n", args->handle);
		abort();
	}

	if (!bo->mapped) {
		bo->mapped = mmap_orig(NULL, bo->size, PROT_READ,
					   MAP_SHARED, host1x->file.fd,
					   args->offset);
		assert(bo->mapped != MAP_FAILED);

		record_set_bo_data(bo->rec_bo, bo->mapped);
	}
}

static void host1x_file_leave_ioctl_syncpt_read(struct host1x_file *host1x,
						struct drm_tegra_syncpt_read *args)
{
	PRINTF("  SYNCPT read:\n");
	PRINTF("    id: %u\n", args->id);
	PRINTF("    value: %u\n", args->value);
}

static void host1x_file_leave_ioctl_syncpt_wait(struct host1x_file *host1x,
						struct drm_tegra_syncpt_wait *args)
{
	PRINTF("  SYNCPT wait:\n");
	PRINTF("    id: %u\n", args->id);
	PRINTF("    thresh: %u\n", args->thresh);
	PRINTF("    timeout: %u\n", args->timeout);
	PRINTF("    value: %u\n", args->value);
}

static void host1x_file_leave_ioctl_submit(struct host1x_file *host1x,
					   struct drm_tegra_submit *args)
{
	PRINTF("  SUBMIT:\n");
	PRINTF("    context: %llu\n", args->context);
	PRINTF("    num_syncpts: %u\n", args->num_syncpts);
	PRINTF("    num_cmdbufs: %u\n", args->num_cmdbufs);
	PRINTF("    num_relocs: %u\n", args->num_relocs);
	PRINTF("    num_waitchks: %u\n", args->num_waitchks);
	PRINTF("    waitchk_mask: %x\n", args->waitchk_mask);
	PRINTF("    timeout: %u\n", args->timeout);
	PRINTF("    syncpts: 0x%08llx\n", args->syncpts);
	PRINTF("    cmdbufs: 0x%08llx\n", args->cmdbufs);
	PRINTF("    relocs: 0x%08llx\n", args->relocs);
	PRINTF("    waitchks: 0x%08llx\n", args->waitchks);
	PRINTF("    fence: %u\n", args->fence);
}

static void host1x_file_leave_ioctl_open_channel(struct host1x_file *host1x,
						 struct drm_tegra_open_channel *args)
{
	struct drm_context *ctx;

	PRINTF("  Channel opened:\n");
	PRINTF("    client: 0x%X\n", args->client);
	PRINTF("    context: %llu\n", args->context);

	ctx = host1x_context_new(args->context, args->client);
	if (!ctx) {
		fprintf(stderr, "failed to create context\n");
		abort();
	}

	list_add_tail(&ctx->list, &host1x->contexts);
}

static void host1x_file_leave_ioctl_close_channel(struct host1x_file *host1x,
						  struct drm_tegra_close_channel *args)
{
	struct drm_context *ctx;

	PRINTF("  Channel closed:\n");
	PRINTF("    context: %llu\n", args->context);

	ctx = host1x_file_lookup_context(host1x, args->context);
	if (!ctx) {
		fprintf(stderr, "invalid context: %llu\n", args->context);
		abort();
	}

	host1x_context_free(ctx);
}

static void host1x_file_leave_ioctl_get_syncpt(struct host1x_file *host1x,
					       struct drm_tegra_get_syncpt *args)
{
	PRINTF("  Got SYNCPT:\n");
	PRINTF("    context: %llu\n", args->context);
	PRINTF("    index: %u\n", args->index);
	PRINTF("    id: %u\n", args->id);
}

static void host1x_file_leave_ioctl_get_syncpt_base(struct host1x_file *host1x,
						    struct drm_tegra_get_syncpt_base *args)
{
	PRINTF("  Got SYNCPT base:\n");
	PRINTF("    context: %llu\n", args->context);
	PRINTF("    syncpt: %u\n", args->syncpt);
	PRINTF("    id: %u\n", args->id);
}

static void host1x_file_enter_ioctl_gem_get_tiling(struct host1x_file *host1x,
						   struct drm_tegra_gem_get_tiling *args)
{
	struct host1x_bo *bo;

	PRINTF("  Set GEM tiling:\n");
	PRINTF("    handle: %u\n", args->handle);
	PRINTF("    mode: %u\n", args->mode);
	PRINTF("    value: %u\n", args->value);

	bo = host1x_file_lookup_bo(host1x, args->handle);
	if (!bo) {
		fprintf(stderr, "invalid handle: %x\n", args->handle);
		abort();
	}
}

static void host1x_file_enter_ioctl_gem_get_flags(struct host1x_file *host1x,
						  struct drm_tegra_gem_get_flags *args)
{
	struct host1x_bo *bo;

	PRINTF("  Set GEM flags:\n");
	PRINTF("    handle: %u\n", args->handle);
	PRINTF("    flags: %u\n", args->flags);

	bo = host1x_file_lookup_bo(host1x, args->handle);
	if (!bo) {
		fprintf(stderr, "invalid handle: %x\n", args->handle);
		abort();
	}
}

static void host1x_file_leave_ioctl_create_dumb(struct host1x_file *host1x,
						struct drm_mode_create_dumb *args)
{
	struct host1x_bo *bo;
	char buf[256] = { };

	host1x_gem_flags_to_string(buf, args->flags);

	PRINTF("  Dumb GEM created:\n");
	PRINTF("    height: %u\n", args->height);
	PRINTF("    width: %u\n", args->width);
	PRINTF("    bpp: %u\n", args->bpp);
	PRINTF("    flags: %x\n", args->flags);
	PRINTF("    handle: %u\n", args->handle);
	PRINTF("    pitch: %u\n", args->pitch);
	PRINTF("    size: %llx\n", args->size);

	bo = host1x_bo_new(host1x, args->handle, args->size, args->flags);
	if (!bo) {
		fprintf(stderr, "failed to create bo\n");
		abort();
	}

	list_add_tail(&bo->list, &host1x->bos);
}

static void host1x_file_leave_ioctl_mmap_dumb(struct host1x_file *host1x,
					      struct drm_mode_map_dumb *args)
{
	struct host1x_bo *bo;

	PRINTF("  Dumb GEM mapped:\n");
	PRINTF("    handle: %u\n", args->handle);
	PRINTF("    offset: %llx\n", args->offset);

	bo = host1x_file_lookup_bo(host1x, args->handle);
	if (!bo) {
		fprintf(stderr, "invalid handle: %x\n", args->handle);
		abort();
	}

	if (!bo->mapped) {
		bo->mapped = mmap_orig(NULL, bo->size, PROT_READ,
					   MAP_SHARED, host1x->file.fd,
					   args->offset);
		assert(bo->mapped != MAP_FAILED);

		record_set_bo_data(bo->rec_bo, bo->mapped);
	}
}

static void host1x_file_leave_ioctl_destroy_dumb(struct host1x_file *host1x,
						 struct drm_mode_destroy_dumb *args)
{
	struct host1x_bo *bo;

	PRINTF("  GEM destroyed:\n");
	PRINTF("    handle: %u\n", args->handle);

	bo = host1x_file_lookup_bo(host1x, args->handle);
	if (!bo) {
		fprintf(stderr, "invalid handle: %x\n", args->handle);
		abort();
	}

	host1x_bo_free(bo);
}

static void host1x_file_leave_ioctl_gem_close(struct host1x_file *host1x,
					      struct drm_gem_close *args)
{
	struct host1x_bo *bo;

	PRINTF("  GEM closed:\n");
	PRINTF("    handle: %u\n", args->handle);

	bo = host1x_file_lookup_bo(host1x, args->handle);
	if (!bo) {
		fprintf(stderr, "invalid handle: %x\n", args->handle);
		abort();
	}

	host1x_bo_free(bo);
}

static void host1x_file_leave_ioctl_addfb(struct host1x_file *host1x,
					  struct drm_mode_fb_cmd *args)
{
	struct host1x_bo *bo;
	uint32_t pixel_format;

	PRINTF("  FB added:\n");
	PRINTF("    fb_id: %u\n", args->fb_id);
	PRINTF("    width: %u\n", args->width);
	PRINTF("    height: %u\n", args->height);
	PRINTF("    bpp: %u\n", args->bpp);
	PRINTF("    depth: %u\n", args->depth);
	PRINTF("    handle: %u\n", args->handle);

	bo = host1x_file_lookup_bo(host1x, args->handle);
	if (!bo) {
		fprintf(stderr, "invalid handle: %x\n", args->handle);
		abort();
	}

	if (args->bpp == 32 && args->depth == 24)
		pixel_format = DRM_FORMAT_XRGB8888;
	else if (args->bpp == 16 && args->depth == 16)
		pixel_format = DRM_FORMAT_RGB565;
	else
		return;

	if (!recorder_enabled())
		return;

	bo->rec_bo->width = args->width;
	bo->rec_bo->height = args->height;
	bo->rec_bo->pitch = args->pitch;
	bo->rec_bo->format = pixel_format;
	bo->rec_bo->fb_id = args->fb_id;

	record_add_framebuffer(bo->rec_bo, 0);
}

static void host1x_file_leave_ioctl_addfb2(struct host1x_file *host1x,
					   struct drm_mode_fb_cmd2 *args)
{
	struct host1x_bo *bo;
	unsigned int i;

	PRINTF("  FB2 added:\n");
	PRINTF("    fb_id: %u\n", args->fb_id);
	PRINTF("    width: %u\n", args->width);
	PRINTF("    height: %u\n", args->height);
	PRINTF("    pixel_format: %u\n", args->pixel_format);
	PRINTF("    flags: %u\n", args->flags);

	for (i = 0; i < 4; i++) {
		PRINTF("    handles[%u]: %u\n", i, args->handles[i]);
		PRINTF("    pitches[%u]: %u\n", i, args->pitches[i]);
		PRINTF("    offsets[%u]: %u\n", i, args->offsets[i]);
		PRINTF("    modifier[%u]: %llu\n", i, args->modifier[i]);
	}

	bo = host1x_file_lookup_bo(host1x, args->handles[0]);
	if (!bo) {
		fprintf(stderr, "invalid handle: %x\n", args->handles[0]);
		abort();
	}

	if (args->pixel_format != DRM_FORMAT_XBGR8888 &&
	    args->pixel_format != DRM_FORMAT_XRGB8888 &&
	    args->pixel_format != DRM_FORMAT_RGB565)
		return;

	if (!recorder_enabled())
		return;

	bo->rec_bo->width = args->width;
	bo->rec_bo->height = args->height;
	bo->rec_bo->pitch = args->pitches[0];
	bo->rec_bo->format = args->pixel_format;
	bo->rec_bo->fb_id = args->fb_id;

	record_add_framebuffer(bo->rec_bo, args->flags);
}

static void host1x_file_leave_ioctl_gem_open(struct host1x_file *host1x,
					     struct drm_gem_open *args)
{
	struct host1x_bo *bo;

	PRINTF("  GEM imported:\n");
	PRINTF("    name: %u\n", args->name);
	PRINTF("    handle: %u\n", args->handle);
	PRINTF("    size: %llx\n", args->size);

	bo = host1x_bo_new(host1x, args->handle, args->size, 0);
	if (!bo) {
		fprintf(stderr, "failed to create bo\n");
		abort();
	}

	list_add_tail(&bo->list, &host1x->bos);
}

static int host1x_file_leave_ioctl(struct file *file, unsigned long request,
				   void *arg)
{
	struct host1x_file *host1x = to_host1x_file(file);

	switch (request) {
	case DRM_IOCTL_TEGRA_GEM_CREATE:
		host1x_file_leave_ioctl_gem_create(host1x, arg);
		break;

	case DRM_IOCTL_TEGRA_GEM_MMAP:
		host1x_file_leave_ioctl_gem_mmap(host1x, arg);
		break;

	case DRM_IOCTL_TEGRA_SYNCPT_READ:
		host1x_file_leave_ioctl_syncpt_read(host1x, arg);
		break;

	case DRM_IOCTL_TEGRA_SYNCPT_WAIT:
		host1x_file_leave_ioctl_syncpt_wait(host1x, arg);
		break;

	case DRM_IOCTL_TEGRA_OPEN_CHANNEL:
		host1x_file_leave_ioctl_open_channel(host1x, arg);
		break;

	case DRM_IOCTL_TEGRA_CLOSE_CHANNEL:
		host1x_file_leave_ioctl_close_channel(host1x, arg);
		break;

	case DRM_IOCTL_TEGRA_GET_SYNCPT:
		host1x_file_leave_ioctl_get_syncpt(host1x, arg);
		break;

	case DRM_IOCTL_TEGRA_SUBMIT:
		host1x_file_leave_ioctl_submit(host1x, arg);
		break;

	case DRM_IOCTL_TEGRA_GET_SYNCPT_BASE:
		host1x_file_leave_ioctl_get_syncpt_base(host1x, arg);
		break;

	case DRM_IOCTL_TEGRA_GEM_GET_TILING:
		host1x_file_enter_ioctl_gem_get_tiling(host1x, arg);
		break;

	case DRM_IOCTL_TEGRA_GEM_GET_FLAGS:
		host1x_file_enter_ioctl_gem_get_flags(host1x, arg);
		break;

	case DRM_IOCTL_MODE_CREATE_DUMB:
		host1x_file_leave_ioctl_create_dumb(host1x, arg);
		break;

	case DRM_IOCTL_MODE_MAP_DUMB:
		host1x_file_leave_ioctl_mmap_dumb(host1x, arg);
		break;

	case DRM_IOCTL_MODE_DESTROY_DUMB:
		host1x_file_leave_ioctl_destroy_dumb(host1x, arg);
		break;

	case DRM_IOCTL_GEM_CLOSE:
		host1x_file_leave_ioctl_gem_close(host1x, arg);
		break;

	case DRM_IOCTL_MODE_ADDFB:
		host1x_file_leave_ioctl_addfb(host1x, arg);
		break;

	case DRM_IOCTL_MODE_ADDFB2:
		host1x_file_leave_ioctl_addfb2(host1x, arg);
		break;

	case DRM_IOCTL_GEM_OPEN:
		host1x_file_leave_ioctl_gem_open(host1x, arg);
		break;

	default:
		break;
	}

	return 0;
}

static void host1x_file_release(struct file *file)
{
	struct host1x_file *host1x = to_host1x_file(file);
	struct host1x_bo *bo, *bo_tmp;
	struct drm_context *ctx, *ctx_tmp;

	list_for_each_entry_safe(bo, bo_tmp, &host1x->bos, list)
		host1x_bo_free(bo);

	list_for_each_entry_safe(ctx, ctx_tmp, &host1x->contexts, list)
		host1x_context_free(ctx);

	record_destroy_ctx(host1x->rec_ctx);

	free(file->path);
	free(host1x);
}

static const struct file_ops host1x_file_ops = {
	.enter_ioctl = host1x_file_enter_ioctl,
	.leave_ioctl = host1x_file_leave_ioctl,
	.release = host1x_file_release,
};

static struct file *host1x_file_new(const char *path, int fd)
{
	struct host1x_file *host1x;

	host1x = calloc(1, sizeof(*host1x));
	if (!host1x)
		return NULL;

	INIT_LIST_HEAD(&host1x->file.list);
	host1x->file.path = strdup(path);
	host1x->file.fd = fd;

	host1x->file.num_ioctls = ARRAY_SIZE(host1x_ioctls);
	host1x->file.ioctls = host1x_ioctls;
	host1x->file.ops = &host1x_file_ops;

	host1x->rec_ctx = record_create_ctx();

	INIT_LIST_HEAD(&host1x->bos);
	INIT_LIST_HEAD(&host1x->contexts);

	return &host1x->file;
}

static const struct file_table host1x_files[] = {
	{ "/dev/dri/card0", host1x_file_new },
	{ "/dev/dri/render128", host1x_file_new },
	{ "/dev/dri/by-path/platform-50000000.host1x-card", host1x_file_new },
	{ "/dev/dri/by-path/platform-50000000.host1x-render", host1x_file_new },
	{ "/sys/devices/soc0/50000000.host1x/drm/drm/card0", host1x_file_new },
	{ "/sys/devices/soc0/50000000.host1x/drm/drm/render128", host1x_file_new },
};

void host1x_register(void)
{
	file_table_register(host1x_files, ARRAY_SIZE(host1x_files));
}
