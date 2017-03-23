/*
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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include <png.h>

#include "nvmap.h"

struct nvmap_create_handle {
	union {
		uint32_t key;
		uint32_t id;
		uint32_t size;
	};
	uint32_t handle;
};

struct nvmap_alloc_handle {
	uint32_t handle;
	uint32_t heap_mask;
	uint32_t flags;
	uint32_t align;
};

struct nvmap_map_caller {
	uint32_t handle;
	uint32_t offset;
	uint32_t length;
	uint32_t flags;
	uint32_t addr;
};

struct nvmap_rw_handle {
	uint32_t addr;
	uint32_t handle;
	uint32_t offset;
	uint32_t elem_size;
	uint32_t hmem_stride;
	uint32_t user_stride;
	uint32_t count;
};

struct nvmap_handle_param {
	uint32_t handle;
	uint32_t param;
	uint32_t result;
};

struct nvmap_pin_handle {
	uint32_t handles;
	uint32_t addr;
	uint32_t count;
};

struct nvmap_cache_op {
	uint32_t addr;
	uint32_t handle;
	uint32_t length;
	uint32_t op;
};

#define NVMAP_IOCTL_MAGIC 'N'

#define NVMAP_IOCTL_CREATE _IOWR(NVMAP_IOCTL_MAGIC, 0, struct nvmap_create_handle)
#define NVMAP_IOCTL_CLAIM _IOWR(NVMAP_IOCTL_MAGIC, 1, struct nvmap_create_handle)
#define NVMAP_IOCTL_FROM_ID _IOWR(NVMAP_IOCTL_MAGIC, 2, struct nvmap_create_handle)
#define NVMAP_IOCTL_ALLOC _IOW(NVMAP_IOCTL_MAGIC, 3, struct nvmap_alloc_handle)
#define NVMAP_IOCTL_FREE _IO(NVMAP_IOCTL_MAGIC, 4)
#define NVMAP_IOCTL_MMAP _IOWR(NVMAP_IOCTL_MAGIC, 5, struct nvmap_map_caller)
#define NVMAP_IOCTL_WRITE _IOW(NVMAP_IOCTL_MAGIC, 6, struct nvmap_rw_handle)
#define NVMAP_IOCTL_READ _IOW(NVMAP_IOCTL_MAGIC, 7, struct nvmap_rw_handle)
#define NVMAP_IOCTL_PARAM _IOWR(NVMAP_IOCTL_MAGIC, 8, struct nvmap_handle_param)
#define NVMAP_IOCTL_PIN _IOWR(NVMAP_IOCTL_MAGIC, 10, struct nvmap_pin_handle)
#define NVMAP_IOCTL_UNPIN _IOW(NVMAP_IOCTL_MAGIC, 11, struct nvmap_pin_handle)
#define NVMAP_IOCTL_CACHE _IOW(NVMAP_IOCTL_MAGIC, 12, struct nvmap_cache_op)
#define NVMAP_IOCTL_GET_ID _IOWR(NVMAP_IOCTL_MAGIC, 13, struct nvmap_create_handle)

static void detile(void *target, struct nvmap_framebuffer *fb,
		   unsigned int tx, unsigned int ty)
{
	const unsigned int nx = fb->pitch / tx, ny = fb->height / ty;
	const unsigned int size = tx * ty, pitch = tx * nx;
	const void *source = fb->handle->ptr;
	unsigned int i, j, k;

	for (j = 0; j < ny; j++) {
		for (i = 0; i < nx; i++) {
			unsigned int to = (j * nx * size) + (i * tx);
			unsigned int so = (j * nx + i) * size;

			for (k = 0; k < ty; k++) {
				memcpy(target + to + k * pitch,
				       source + so + k * tx,
				       tx);
			}
		}
	}
}

struct nvmap *nvmap_open(void)
{
	struct nvmap *nvmap;

	nvmap = calloc(1, sizeof(*nvmap));
	if (!nvmap)
		return NULL;

	nvmap->fd = open("/dev/nvmap", O_RDWR);
	if (nvmap->fd < 0) {
		free(nvmap);
		return NULL;
	}

	return nvmap;
}

void nvmap_close(struct nvmap *nvmap)
{
	if (nvmap) {
		if (nvmap->fd >= 0)
			close(nvmap->fd);
	}

	free(nvmap);
}

struct nvmap_handle *nvmap_handle_create(struct nvmap *nvmap, size_t size)
{
	struct nvmap_create_handle args;
	struct nvmap_handle *handle;
	int err;

	handle = calloc(1, sizeof(*handle));
	if (!handle)
		return NULL;

	handle->size = size;

	memset(&args, 0, sizeof(args));
	args.size = size;

	err = ioctl(nvmap->fd, NVMAP_IOCTL_CREATE, &args);
	if (err < 0) {
		free(handle);
		return NULL;
	}

	handle->id = args.handle;

	return handle;
}

void nvmap_handle_free(struct nvmap *nvmap, struct nvmap_handle *handle)
{
	int err;

	err = ioctl(nvmap->fd, NVMAP_IOCTL_FREE, handle->id);
	if (err < 0) {
		fprintf(stderr, "failed to free nvmap handle\n");
	}

	free(handle);
}

int nvmap_handle_alloc(struct nvmap *nvmap, struct nvmap_handle *handle,
		       unsigned long heap_mask, unsigned long flags,
		       unsigned long align)
{
	struct nvmap_alloc_handle args;
	int err;

	memset(&args, 0, sizeof(args));
	args.handle = handle->id;
	args.heap_mask = heap_mask;
	args.flags = flags;
	args.align = align;

	err = ioctl(nvmap->fd, NVMAP_IOCTL_ALLOC, &args);
	if (err < 0) {
		return -errno;
	}

	return 0;
}

#define ROUNDUP(n, d) ((((n) + (d) - 1) / (d)) * (d))

int nvmap_handle_mmap(struct nvmap *nvmap, struct nvmap_handle *handle)
{
	uint32_t size = ROUNDUP(handle->size, 4096);
	struct nvmap_map_caller args;
	int err;

	if (handle->ptr)
		return 0;

	handle->ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,
			   nvmap->fd, 0);

	memset(&args, 0, sizeof(args));
	args.handle = handle->id;
	args.offset = 0;
	args.length = size;
	args.flags = 0;
	args.addr = (uintptr_t)handle->ptr;

	err = ioctl(nvmap->fd, NVMAP_IOCTL_MMAP, &args);
	if (err < 0) {
		munmap(handle->ptr, size);
		return -errno;
	}

	return 0;
}

int nvmap_handle_invalidate(struct nvmap *nvmap, struct nvmap_handle *handle,
			    unsigned long offset, unsigned long length)
{
	struct nvmap_cache_op args;
	int err;

	memset(&args, 0, sizeof(args));
	args.addr = (uintptr_t)handle->ptr + offset;
	args.handle = handle->id;
	args.length = length;
	args.op = 1;

	err = ioctl(nvmap->fd, NVMAP_IOCTL_CACHE, &args);
	if (err < 0)
		return -errno;

	return 0;
}

int nvmap_handle_writeback_invalidate(struct nvmap *nvmap,
				      struct nvmap_handle *handle,
				      unsigned long offset,
				      unsigned long length)
{
	struct nvmap_cache_op args;
	int err;

	memset(&args, 0, sizeof(args));
	args.addr = (uintptr_t)handle->ptr + offset;
	args.handle = handle->id;
	args.length = length;
	args.op = 2;

	err = ioctl(nvmap->fd, NVMAP_IOCTL_CACHE, &args);
	if (err < 0)
		return -errno;

	return 0;
}

struct nvmap_framebuffer *nvmap_framebuffer_create(struct nvmap *nvmap,
						   unsigned short width,
						   unsigned short height,
						   unsigned short depth)
{
	struct nvmap_framebuffer *fb;
	int err;

	fb = calloc(1, sizeof(*fb));
	if (!fb)
		return NULL;

	fb->pitch = width * (depth / 8);
	fb->width = width;
	fb->height = height;
	fb->depth = depth;
	fb->nvmap = nvmap;

	fb->handle = nvmap_handle_create(nvmap, fb->pitch * height);
	if (!fb->handle) {
		free(fb);
		return NULL;
	}

	err = nvmap_handle_alloc(nvmap, fb->handle, 1 << 0,
	                         NVMAP_HANDLE_WRITE_COMBINE, 0x100);
	if (err < 0) {
		free(fb);
		return NULL;
	}

	return fb;
}

void nvmap_framebuffer_free(struct nvmap_framebuffer *fb)
{
	nvmap_handle_free(fb->nvmap, fb->handle);
	free(fb);
}

int nvmap_framebuffer_save(struct nvmap_framebuffer *fb, const char *filename)
{
	png_structp png;
	png_bytep *rows;
	png_infop info;
	unsigned int i;
	size_t stride;
	void *buffer;
	FILE *fp;
	int err;

	if (fb->depth != 32) {
		fprintf(stderr, "ERROR: %u bits per pixel not supported\n",
			fb->depth);
		return -EINVAL;
	}

	err = nvmap_handle_mmap(fb->nvmap, fb->handle);
	if (err < 0)
		return -EFAULT;

	err = nvmap_handle_invalidate(fb->nvmap, fb->handle, 0,
				      fb->handle->size);
	if (err < 0)
		return -EFAULT;

	fp = fopen(filename, "wb");
	if (!fp) {
		fprintf(stderr, "failed to write `%s'\n", filename);
		return -errno;
	}

	png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png)
		return -ENOMEM;

	info = png_create_info_struct(png);
	if (!info)
		return -ENOMEM;

	if (setjmp(png_jmpbuf(png)))
		return -EIO;

	png_init_io(png, fp);

	if (setjmp(png_jmpbuf(png)))
		return -EIO;

	png_set_IHDR(png, info, fb->width, fb->height, 8, PNG_COLOR_TYPE_RGBA,
		     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
		     PNG_FILTER_TYPE_BASE);
	png_write_info(png, info);

	if (setjmp(png_jmpbuf(png))) {
		fprintf(stderr, "failed to write IHDR\n");
		return -EIO;
	}

	stride = fb->width * (fb->depth / 8);

	buffer = malloc(stride * fb->height);
	if (!buffer)
		return ENOMEM;

	detile(buffer, fb, 16, 16);

	rows = malloc(fb->height * sizeof(png_bytep));
	if (!rows) {
		fprintf(stderr, "out-of-memory\n");
		free(buffer);
		return -ENOMEM;
	}

	for (i = 0; i < fb->height; i++)
		rows[fb->height - i - 1] = buffer + i * stride;

	png_write_image(png, rows);

	free(rows);
	free(buffer);

	if (setjmp(png_jmpbuf(png)))
		return -EIO;

	png_write_end(png, NULL);

	fclose(fp);

	return 0;
}
