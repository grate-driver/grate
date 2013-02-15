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
