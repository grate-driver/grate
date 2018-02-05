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
#include "host1x.h"
#include "syscall.h"

struct host1x_file {
	struct list_head handles;
	struct file file;
};

struct host1x_handle {
	int id;
	size_t size;
	char *mapped;

	struct list_head list;
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
};

static struct host1x_handle *host1x_handle_new(unsigned long id, size_t size)
{
	struct host1x_handle *handle;

	handle = calloc(1, sizeof(*handle));
	if (!handle)
		return NULL;

	INIT_LIST_HEAD(&handle->list);
	handle->size = size;
	handle->id = id;

	return handle;
}

static void host1x_handle_free(struct host1x_handle *handle)
{
	if (handle->mapped)
		munmap_orig(handle->mapped, handle->size);

	list_del(&handle->list);
	free(handle);
}

static struct host1x_handle *host1x_file_lookup_handle(struct host1x_file *host1x,
						       unsigned long id)
{
	struct host1x_handle *handle;

	list_for_each_entry(handle, &host1x->handles, list)
		if (handle->id == id)
			return handle;

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
	printf("  SYNCPT increment:\n");
	printf("    id: %u\n", args->id);
}

static void host1x_file_enter_ioctl_syncpt_wait(struct host1x_file *host1x,
						struct drm_tegra_syncpt_wait *args)
{
	printf("  SYNCPT wait:\n");
	printf("    id: %u\n", args->id);
	printf("    thresh: %u\n", args->thresh);
	printf("    timeout: %u\n", args->timeout);
}

static void host1x_file_enter_ioctl_submit(struct host1x_file *host1x,
					   struct drm_tegra_submit *args)
{
	struct drm_tegra_cmdbuf *cmdbuf = (void *)(intptr_t)args->cmdbufs;
	struct host1x_handle *handle;
	unsigned int i;

	printf("  SUBMIT:\n");
	printf("    context: %llu\n", args->context);
	printf("    num_syncpts: %u\n", args->num_syncpts);
	printf("    num_cmdbufs: %u\n", args->num_cmdbufs);
	printf("    num_relocs: %u\n", args->num_relocs);
	printf("    num_waitchks: %u\n", args->num_waitchks);
	printf("    waitchk_mask: %x\n", args->waitchk_mask);
	printf("    timeout: %u\n", args->timeout);
	printf("    syncpts: 0x%08llx\n", args->syncpts);
	printf("    cmdbufs: 0x%08llx\n", args->cmdbufs);
	printf("    relocs: 0x%08llx\n", args->relocs);
	printf("    waitchks: 0x%08llx\n", args->waitchks);

	for (i = 0; i < args->num_cmdbufs; i++, cmdbuf++) {
		uint32_t *commands;

		handle = host1x_file_lookup_handle(host1x, cmdbuf->handle);
		if (!handle) {
			fprintf(stderr, "invalid handle: %u\n", cmdbuf->handle);
			continue;
		}

		if (!handle->mapped) {
			fprintf(stderr, "cmdbuf %u not mapped\n", i);
			continue;
		}

		commands = (uint32_t *)(handle->mapped + cmdbuf->offset);
		cdma_dump_commands(commands, cmdbuf->words);
	}
}

static void host1x_file_enter_ioctl_gem_set_tiling(struct host1x_file *host1x,
						   struct drm_tegra_gem_set_tiling *args)
{
	struct host1x_handle *handle;

	printf("  Set GEM tiling:\n");
	printf("    handle: %u\n", args->handle);
	printf("    mode: %u\n", args->mode);
	printf("    value: %u\n", args->value);

	handle = host1x_file_lookup_handle(host1x, args->handle);
	if (!handle) {
		fprintf(stderr, "invalid handle: %x\n", args->handle);
		return;
	}
}

static void host1x_file_enter_ioctl_gem_set_flags(struct host1x_file *host1x,
						  struct drm_tegra_gem_set_flags *args)
{
	struct host1x_handle *handle;

	printf("  Set GEM flags:\n");
	printf("    handle: %u\n", args->handle);
	printf("    flags: %u\n", args->flags);

	handle = host1x_file_lookup_handle(host1x, args->handle);
	if (!handle) {
		fprintf(stderr, "invalid handle: %x\n", args->handle);
		return;
	}
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

	default:
		break;
	}

	return 0;
}

static void host1x_file_leave_ioctl_gem_create(struct host1x_file *host1x,
					       struct drm_tegra_gem_create *args)
{
	struct host1x_handle *handle;
	char buf[256] = { };

	host1x_gem_flags_to_string(buf, args->flags);

	printf("  GEM created:\n");
	printf("    handle: %u\n", args->handle);
	printf("    size: %llx\n", args->size);
	printf("    flags: %x (%s )\n", args->flags, buf);

	handle = host1x_handle_new(args->handle, args->size);
	if (!handle) {
		fprintf(stderr, "failed to create handle\n");
		return;
	}

	list_add_tail(&handle->list, &host1x->handles);
}

static void host1x_file_leave_ioctl_gem_mmap(struct host1x_file *host1x,
					     struct drm_tegra_gem_mmap *args)
{
	struct host1x_handle *handle;

	printf("  GEM mapped:\n");
	printf("    handle: %u\n", args->handle);
	printf("    offset: %llx\n", args->offset);

	handle = host1x_file_lookup_handle(host1x, args->handle);
	if (!handle) {
		fprintf(stderr, "invalid handle: %x\n", args->handle);
		return;
	}

	if (!handle->mapped) {
		handle->mapped = mmap_orig(NULL, handle->size, PROT_READ,
					   MAP_SHARED, host1x->file.fd,
					   args->offset);
		assert(handle->mapped != MAP_FAILED);
	}
}

static void host1x_file_leave_ioctl_syncpt_read(struct host1x_file *host1x,
						struct drm_tegra_syncpt_read *args)
{
	printf("  SYNCPT read:\n");
	printf("    id: %u\n", args->id);
	printf("    value: %u\n", args->value);
}

static void host1x_file_leave_ioctl_syncpt_wait(struct host1x_file *host1x,
						struct drm_tegra_syncpt_wait *args)
{
	printf("  SYNCPT wait:\n");
	printf("    id: %u\n", args->id);
	printf("    thresh: %u\n", args->thresh);
	printf("    timeout: %u\n", args->timeout);
	printf("    value: %u\n", args->value);
}

static void host1x_file_leave_ioctl_submit(struct host1x_file *host1x,
					   struct drm_tegra_submit *args)
{
	printf("  SUBMIT:\n");
	printf("    context: %llu\n", args->context);
	printf("    num_syncpts: %u\n", args->num_syncpts);
	printf("    num_cmdbufs: %u\n", args->num_cmdbufs);
	printf("    num_relocs: %u\n", args->num_relocs);
	printf("    num_waitchks: %u\n", args->num_waitchks);
	printf("    waitchk_mask: %x\n", args->waitchk_mask);
	printf("    timeout: %u\n", args->timeout);
	printf("    syncpts: 0x%08llx\n", args->syncpts);
	printf("    cmdbufs: 0x%08llx\n", args->cmdbufs);
	printf("    relocs: 0x%08llx\n", args->relocs);
	printf("    waitchks: 0x%08llx\n", args->waitchks);
	printf("    fence: %u\n", args->fence);
}

static void host1x_file_leave_ioctl_open_channel(struct host1x_file *host1x,
						 struct drm_tegra_open_channel *args)
{
	printf("  Channel opened:\n");
	printf("    client: 0x%X\n", args->client);
	printf("    context: %llu\n", args->context);
}

static void host1x_file_leave_ioctl_close_channel(struct host1x_file *host1x,
						  struct drm_tegra_close_channel *args)
{
	printf("  Channel closed:\n");
	printf("    context: %llu\n", args->context);
}

static void host1x_file_leave_ioctl_get_syncpt(struct host1x_file *host1x,
					       struct drm_tegra_get_syncpt *args)
{
	printf("  Got SYNCPT:\n");
	printf("    context: %llu\n", args->context);
	printf("    index: %u\n", args->index);
	printf("    id: %u\n", args->id);
}

static void host1x_file_leave_ioctl_get_syncpt_base(struct host1x_file *host1x,
						    struct drm_tegra_get_syncpt_base *args)
{
	printf("  Got SYNCPT base:\n");
	printf("    context: %llu\n", args->context);
	printf("    syncpt: %u\n", args->syncpt);
	printf("    id: %u\n", args->id);
}

static void host1x_file_enter_ioctl_gem_get_tiling(struct host1x_file *host1x,
						   struct drm_tegra_gem_get_tiling *args)
{
	struct host1x_handle *handle;

	printf("  Set GEM tiling:\n");
	printf("    handle: %u\n", args->handle);
	printf("    mode: %u\n", args->mode);
	printf("    value: %u\n", args->value);

	handle = host1x_file_lookup_handle(host1x, args->handle);
	if (!handle) {
		fprintf(stderr, "invalid handle: %x\n", args->handle);
		return;
	}
}

static void host1x_file_enter_ioctl_gem_get_flags(struct host1x_file *host1x,
						  struct drm_tegra_gem_get_flags *args)
{
	struct host1x_handle *handle;

	printf("  Set GEM flags:\n");
	printf("    handle: %u\n", args->handle);
	printf("    flags: %u\n", args->flags);

	handle = host1x_file_lookup_handle(host1x, args->handle);
	if (!handle) {
		fprintf(stderr, "invalid handle: %x\n", args->handle);
		return;
	}
}

static void host1x_file_leave_ioctl_create_dumb(struct host1x_file *host1x,
						struct drm_mode_create_dumb *args)
{
	struct host1x_handle *handle;
	char buf[256] = { };

	host1x_gem_flags_to_string(buf, args->flags);

	printf("  Dumb GEM created:\n");
	printf("    height: %u\n", args->height);
	printf("    width: %u\n", args->width);
	printf("    bpp: %u\n", args->bpp);
	printf("    flags: %x\n", args->flags);
	printf("    handle: %u\n", args->handle);
	printf("    pitch: %u\n", args->pitch);
	printf("    size: %llx\n", args->size);

	handle = host1x_handle_new(args->handle, args->size);
	if (!handle) {
		fprintf(stderr, "failed to create handle\n");
		return;
	}

	list_add_tail(&handle->list, &host1x->handles);
}

static void host1x_file_leave_ioctl_mmap_dumb(struct host1x_file *host1x,
					      struct drm_mode_map_dumb *args)
{
	struct host1x_handle *handle;

	printf("  Dumb GEM mapped:\n");
	printf("    handle: %u\n", args->handle);
	printf("    offset: %llx\n", args->offset);

	handle = host1x_file_lookup_handle(host1x, args->handle);
	if (!handle) {
		fprintf(stderr, "invalid handle: %x\n", args->handle);
		return;
	}

	if (!handle->mapped) {
		handle->mapped = mmap_orig(NULL, handle->size, PROT_READ,
					   MAP_SHARED, host1x->file.fd,
					   args->offset);
		assert(handle->mapped != MAP_FAILED);
	}
}

static void host1x_file_leave_ioctl_destroy_dumb(struct host1x_file *host1x,
						 struct drm_mode_destroy_dumb *args)
{
	struct host1x_handle *handle;

	printf("  GEM destroyed:\n");
	printf("    handle: %u\n", args->handle);

	handle = host1x_file_lookup_handle(host1x, args->handle);
	if (!handle) {
		fprintf(stderr, "invalid handle: %x\n", args->handle);
		return;
	}

	host1x_handle_free(handle);
}

static void host1x_file_leave_ioctl_gem_close(struct host1x_file *host1x,
					      struct drm_gem_close *args)
{
	struct host1x_handle *handle;

	printf("  GEM closed:\n");
	printf("    handle: %u\n", args->handle);

	handle = host1x_file_lookup_handle(host1x, args->handle);
	if (!handle) {
		fprintf(stderr, "invalid handle: %x\n", args->handle);
		return;
	}

	host1x_handle_free(handle);
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

	default:
		break;
	}

	return 0;
}

static void host1x_file_release(struct file *file)
{
	struct host1x_file *host1x = to_host1x_file(file);
	struct host1x_handle *handle, *tmp;

	list_for_each_entry_safe(handle, tmp, &host1x->handles, list)
		host1x_handle_free(handle);

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

	INIT_LIST_HEAD(&host1x->handles);

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
