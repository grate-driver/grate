#ifndef GRATE_HOST1X_PRIVATE_H
#define GRATE_HOST1X_PRIVATE_H 1

#include <stddef.h>
#include <stdint.h>

#include "host1x.h"

#define container_of(ptr, type, member) ({ \
		const typeof(((type *)0)->member) *__mptr = (ptr); \
		(type *)((char *)__mptr - offsetof(type, member)); \
	})

struct host1x_framebuffer {
	unsigned short width;
	unsigned int pitch;
	unsigned short height;
	unsigned short depth;
	unsigned long flags;
	struct host1x_bo *bo;
};

struct host1x_syncpt {
	uint32_t id;
	uint32_t value;
};

struct host1x_bo {
	uint32_t handle;
	size_t size;
	void *ptr;

	int (*mmap)(struct host1x_bo *bo);
	int (*invalidate)(struct host1x_bo *bo, loff_t offset, size_t length);
	int (*flush)(struct host1x_bo *bo, loff_t offset, size_t length);
	void (*free)(struct host1x_bo *bo);
};

static inline unsigned long host1x_bo_get_offset(struct host1x_bo *bo,
						 void *ptr)
{
	return (unsigned long)ptr - (unsigned long)bo->ptr;
}

struct host1x_client {
	struct host1x_syncpt *syncpts;
	unsigned int num_syncpts;

	int (*submit)(struct host1x_client *client, struct host1x_job *job);
	int (*flush)(struct host1x_client *client, uint32_t *fence);
	int (*wait)(struct host1x_client *client, uint32_t fence,
		    uint32_t timeout);
};

struct host1x_gr2d {
	struct host1x_client *client;
	struct host1x_bo *commands;
	struct host1x_bo *scratch;
};

int host1x_gr2d_init(struct host1x *host1x, struct host1x_gr2d *gr2d);
void host1x_gr2d_exit(struct host1x_gr2d *gr2d);

struct host1x_gr3d {
	struct host1x_client *client;
	struct host1x_bo *commands;
	struct host1x_bo *attributes;
};

int host1x_gr3d_init(struct host1x *host1x, struct host1x_gr3d *gr3d);
void host1x_gr3d_exit(struct host1x_gr3d *gr3d);

struct host1x {
	struct host1x_bo *(*bo_create)(struct host1x *host1x, size_t size,
				       unsigned long flags);
	void (*close)(struct host1x *host1x);

	struct host1x_gr2d *gr2d;
	struct host1x_gr3d *gr3d;
};

struct host1x *host1x_nvhost_open(void);
struct host1x *host1x_drm_open(void);

#endif
