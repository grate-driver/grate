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

#ifndef GRATE_HOST1X_H
#define GRATE_HOST1X_H 1

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef BIT
#define BIT(x) (1 << (x))
#endif

/* gr3d pixel formats */
#define HOST1X_GR3D_FORMAT_RGB565	0x6
#define HOST1X_GR3D_FORMAT_RGBA8888	0xd

enum host1x_format {
	HOST1X_FORMAT_RGB565 = 0x6,
	HOST1X_FORMAT_RGBA8888 = 0xd,
};

struct host1x_stream {
	const uint32_t *words;
	const uint32_t *ptr;
	const uint32_t *end;
};

void host1x_stream_init(struct host1x_stream *stream, const void *buffer,
			size_t size);
void host1x_stream_dump(struct host1x_stream *stream, FILE *fp);

struct host1x_framebuffer;
struct host1x_texture;
struct host1x_display;
struct host1x_overlay;
struct host1x_client;
struct host1x_gr2d;
struct host1x_gr3d;
struct host1x_bo;
struct host1x;

struct host1x *host1x_open(void);
void host1x_close(struct host1x *host1x);

struct host1x_display *host1x_get_display(struct host1x *host1x);
struct host1x_gr2d *host1x_get_gr2d(struct host1x *host1x);
struct host1x_gr3d *host1x_get_gr3d(struct host1x *host1x);

int host1x_display_get_resolution(struct host1x_display *display,
				  unsigned int *width, unsigned int *height);
int host1x_display_set(struct host1x_display *display,
		       struct host1x_framebuffer *fb, bool vsync);

int host1x_overlay_create(struct host1x_overlay **overlayp,
			  struct host1x_display *display);
int host1x_overlay_close(struct host1x_overlay *overlay);
int host1x_overlay_set(struct host1x_overlay *overlay,
		       struct host1x_framebuffer *fb, unsigned int x,
		       unsigned int y, unsigned int width,
		       unsigned int height, bool vsync);

struct host1x_bo *host1x_bo_create(struct host1x *host1x, size_t size,
				   unsigned long flags);
void host1x_bo_free(struct host1x_bo *bo);

int host1x_bo_invalidate(struct host1x_bo *bo, loff_t offset, size_t length);
int host1x_bo_mmap(struct host1x_bo *bo, void **ptr);

#define HOST1X_OPCODE_SETCL(offset, classid, mask) \
	((0x0 << 28) | (((offset) & 0xfff) << 16) | (((classid) & 0x3ff) << 6) | ((mask) & 0x3f))
#define HOST1X_OPCODE_INCR(offset, count) \
	((0x1 << 28) | (((offset) & 0xfff) << 16) | ((count) & 0xffff))
#define HOST1X_OPCODE_NONINCR(offset, count) \
	((0x2 << 28) | (((offset) & 0xfff) << 16) | ((count) & 0xffff))
#define HOST1X_OPCODE_MASK(offset, mask) \
	((0x3 << 28) | (((offset) & 0xfff) << 16) | ((mask) & 0xffff))
#define HOST1X_OPCODE_IMM(offset, data) \
	((0x4 << 28) | (((offset) & 0xfff) << 16) | ((data) & 0xffff))
#define HOST1X_OPCODE_EXTEND(subop, value) \
	((0xe << 28) | (((subop) & 0xf) << 24) | ((value) & 0xffffff))

struct host1x_pushbuf_reloc {
	unsigned long source_offset;
	unsigned long target_handle;
	unsigned long target_offset;
	unsigned long shift;
};

struct host1x_pushbuf {
	struct host1x_bo *bo;
	unsigned long offset;
	unsigned long length;

	struct host1x_pushbuf_reloc *relocs;
	unsigned long num_relocs;

	uint32_t *ptr;
};

struct host1x_pushbuf *host1x_pushbuf_create(struct host1x_bo *bo,
					     unsigned long offset);

struct host1x_job {
	uint32_t syncpt;
	uint32_t syncpt_incrs;

	struct host1x_pushbuf *pushbufs;
	unsigned int num_pushbufs;
};

struct host1x_job *host1x_job_create(uint32_t syncpt, uint32_t increments);
void host1x_job_free(struct host1x_job *job);
struct host1x_pushbuf *host1x_job_append(struct host1x_job *job,
					 struct host1x_bo *bo,
					 unsigned long offset);
int host1x_pushbuf_push(struct host1x_pushbuf *pb, uint32_t word);
int host1x_pushbuf_relocate(struct host1x_pushbuf *pb, struct host1x_bo *target,
			    unsigned long offset, unsigned long shift);

int host1x_client_submit(struct host1x_client *client, struct host1x_job *job);
int host1x_client_flush(struct host1x_client *client, uint32_t *fence);
int host1x_client_wait(struct host1x_client *client, uint32_t fence,
		       uint32_t timeout);

#define HOST1X_FRAMEBUFFER_DEPTH (1 << 0)

struct host1x_framebuffer *host1x_framebuffer_create(struct host1x *host1x,
						     unsigned short width,
						     unsigned short height,
						     unsigned short depth,
						     unsigned long flags);
void host1x_framebuffer_free(struct host1x_framebuffer *fb);
int host1x_framebuffer_save(struct host1x_framebuffer *fb, const char *path);

struct host1x_texture *host1x_texture_load(struct host1x *host1x,
					   const char *path);
void host1x_texture_free(struct host1x_texture *texture);

struct host1x_gr2d;
struct host1x_gr3d;

int host1x_gr2d_clear(struct host1x_gr2d *gr2d, struct host1x_framebuffer *fb,
		      float red, float green, float blue, float alpha);
int host1x_gr2d_blit(struct host1x_gr2d *gr2d, struct host1x_framebuffer *src,
		     struct host1x_framebuffer *dst, unsigned int sx,
		     unsigned int sy, unsigned int dx, unsigned int dy,
		     unsigned int width, unsigned int height);
void host1x_gr3d_viewport(struct host1x_pushbuf *pb, float x, float y,
			  float width, float height);
int host1x_gr3d_triangle(struct host1x_gr3d *gr3d,
			 struct host1x_framebuffer *fb);

#endif
