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

#ifndef GRATE_HOST1X_PRIVATE_H
#define GRATE_HOST1X_PRIVATE_H 1

#include <stddef.h>
#include <stdint.h>

#include "host1x.h"

#define container_of(ptr, type, member) ({ \
		const typeof(((type *)0)->member) *__mptr = (ptr); \
		(type *)((char *)__mptr - offsetof(type, member)); \
	})

#define HOST1X_BO_CREATE_FLAG_TILED	(1 << 8)
#define HOST1X_BO_CREATE_FLAG_BOTTOM_UP	(1 << 9)
#define HOST1X_BO_CREATE_DRM_FLAGS_MASK 0x300

struct host1x_syncpt {
	uint32_t id;
	uint32_t value;
};

struct host1x_bo_priv {
	int (*mmap)(struct host1x_bo *bo);
	int (*invalidate)(struct host1x_bo *bo, unsigned long offset,
			  size_t length);
	int (*flush)(struct host1x_bo *bo, unsigned long offset, size_t length);
	void (*free)(struct host1x_bo *bo);
	struct host1x_bo* (*clone)(struct host1x_bo *bo);
};

static inline unsigned long host1x_bo_get_offset(struct host1x_bo *bo,
						 void *ptr)
{
	return (unsigned long)ptr - (unsigned long)bo->ptr;
}

struct host1x_display {
	unsigned int width;
	unsigned int height;
	void *priv;

	int (*create_overlay)(struct host1x_display *display,
			      struct host1x_overlay **overlayp);
	int (*set)(struct host1x_display *display,
		   struct host1x_framebuffer *fb, bool vsync);
};

struct host1x_overlay {
	int (*close)(struct host1x_overlay *overlay);
	int (*set)(struct host1x_overlay *overlay,
		   struct host1x_framebuffer *fb, unsigned int x,
		   unsigned int y, unsigned int width, unsigned int height,
		   bool vsync);
};

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
	struct host1x_bo *(*bo_create)(struct host1x *host1x,
				       struct host1x_bo_priv *priv,
				       size_t size, unsigned long flags);
	int (*framebuffer_init)(struct host1x *host1x,
				struct host1x_framebuffer *fb);
	void (*close)(struct host1x *host1x);

	struct host1x_display *display;
	struct host1x_gr2d *gr2d;
	struct host1x_gr3d *gr3d;
};

struct host1x *host1x_nvhost_open(void);
void host1x_nvhost_display_init(struct host1x *host1x);

struct host1x *host1x_drm_open(void);
void host1x_drm_display_init(struct host1x *host1x);

#define host1x_error(fmt, args...) \
	fprintf(stderr, "ERROR: %s: %d: " fmt, __func__, __LINE__, ##args)

#endif
