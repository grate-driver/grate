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

#ifndef GRATE_NVMAP_H
#define GRATE_NVMAP_H 1

#include <stdint.h>
#include <stdlib.h>

struct nvmap {
	int fd;
};

struct nvmap *nvmap_open(void);
void nvmap_close(struct nvmap *nvmap);

struct nvmap_handle {
	size_t size;
	uint32_t id;
	void *ptr;
};

static inline unsigned long
nvmap_handle_get_offset(struct nvmap_handle *handle, void *ptr)
{
	return (unsigned long)ptr - (unsigned long)handle->ptr;
}

#define NVMAP_HANDLE_UNREACHABLE     (0x0ul << 0)
#define NVMAP_HANDLE_WRITE_COMBINE   (0x1ul << 0)
#define NVMAP_HANDLE_INNER_CACHEABLE (0x2ul << 0)
#define NVMAP_HANDLE_CACHEABLE       (0x3ul << 0)
#define NVMAP_HANDLE_CACHE_FLAG      (0x3ul << 0)

#define NVMAP_HANDLE_SECURE          (0x1ul << 2)
#define NVMAP_HANDLE_KIND_SPECIFIED  (0x1ul << 3)
#define NVMAP_HANDLE_COMPR_SPECIFIED (0x1ul << 4)
#define NVMAP_HANDLE_ZEROED_PAGES    (0x1ul << 5)

struct nvmap_handle *nvmap_handle_create(struct nvmap *nvmap, size_t size);
void nvmap_handle_free(struct nvmap *nvmap, struct nvmap_handle *handle);
int nvmap_handle_alloc(struct nvmap *nvmap, struct nvmap_handle *handle,
		       unsigned long heap_mask, unsigned long flags,
		       unsigned long align);
int nvmap_handle_mmap(struct nvmap *nvmap, struct nvmap_handle *handle);
int nvmap_handle_invalidate(struct nvmap *nvmap, struct nvmap_handle *handle,
			    unsigned long offset, unsigned long length);
int nvmap_handle_writeback_invalidate(struct nvmap *nvmap,
				      struct nvmap_handle *handle,
				      unsigned long offset,
				      unsigned long length);

struct nvmap_framebuffer {
	unsigned short width;
	unsigned short height;
	unsigned short depth;
	unsigned short pitch;

	struct nvmap_handle *handle;
	struct nvmap *nvmap;
};

struct nvmap_framebuffer *nvmap_framebuffer_create(struct nvmap *nvmap,
						   unsigned short width,
						   unsigned short height,
						   unsigned short depth);
void nvmap_framebuffer_free(struct nvmap_framebuffer *fb);
int nvmap_framebuffer_save(struct nvmap_framebuffer *fb, const char *filename);

#endif
