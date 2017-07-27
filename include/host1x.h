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

enum host1x_class {
	HOST1X_CLASS_GR2D = 0x51,
	HOST1X_CLASS_GR3D = 0x60,
};

/* gr3d pixel formats */
#define HOST1X_GR3D_FORMAT_RGB565	0x6
#define HOST1X_GR3D_FORMAT_RGBA8888	0xd

#define NVHOST_BO_FLAG_FRAMEBUFFER	1
#define NVHOST_BO_FLAG_COMMAND_BUFFER	2
#define NVHOST_BO_FLAG_SCRATCH		3
#define NVHOST_BO_FLAG_ATTRIBUTES	4

struct host1x_stream {
	const uint32_t *words;
	const uint32_t *ptr;
	const uint32_t *end;

	void (*write_word)(void *user, int classid, int offset, uint32_t value);
	int classid;
	void *user;
};

void host1x_stream_init(struct host1x_stream *stream, const void *buffer,
			size_t size);
void host1x_stream_interpret(struct host1x_stream *stream);

struct host1x_display;
struct host1x_overlay;
struct host1x_client;
struct host1x_gr2d;
struct host1x_gr3d;
struct host1x_bo_priv;
struct host1x;

struct host1x_bo {
	struct host1x_bo_priv *priv;
	struct host1x_bo *wrapped;
	uint32_t handle;
	unsigned long offset;
	size_t size;
	void *ptr;
};

#define PIX_BUF_FMT(id, bpp) \
	((id) << 8 | (bpp))

#define PIX_BUF_FORMAT_BITS(f) \
	((f) & 0xff)

#define PIX_BUF_FORMAT_BYTES(f) \
	(PIX_BUF_FORMAT_BITS(f) >> 3)

enum pixel_format {
    PIX_BUF_FMT_A8            = PIX_BUF_FMT(0, 8),
    PIX_BUF_FMT_L8            = PIX_BUF_FMT(1, 8),
    PIX_BUF_FMT_S8            = PIX_BUF_FMT(2, 8),
    PIX_BUF_FMT_LA88          = PIX_BUF_FMT(3, 16),
    PIX_BUF_FMT_RGB565        = PIX_BUF_FMT(4, 16),
    PIX_BUF_FMT_RGBA5551      = PIX_BUF_FMT(5, 16),
    PIX_BUF_FMT_RGBA4444      = PIX_BUF_FMT(6, 16),
    PIX_BUF_FMT_D16_LINEAR    = PIX_BUF_FMT(7, 16),
    PIX_BUF_FMT_D16_NONLINEAR = PIX_BUF_FMT(8, 16),
    PIX_BUF_FMT_RGBA8888      = PIX_BUF_FMT(9, 32),
    PIX_BUF_FMT_RGBA_FP32     = PIX_BUF_FMT(10, 32),
    PIX_BUF_FMT_ARGB8888      = PIX_BUF_FMT(11, 32),
    PIX_BUF_FMT_ABGR8888      = PIX_BUF_FMT(12, 32),
};

enum layout_format {
	PIX_BUF_LAYOUT_LINEAR,
	PIX_BUF_LAYOUT_TILED_16x16,
};

struct host1x_pixelbuffer {
	struct host1x_bo *bo;
	enum pixel_format format;
	enum layout_format layout;
	unsigned width;
	unsigned height;
	unsigned pitch;
};

#define PIXBUF_GUARD_AREA_SIZE	0x4000

struct host1x_pixelbuffer *host1x_pixelbuffer_create(
				struct host1x *host1x,
				unsigned width, unsigned height,
				unsigned pitch,
				enum pixel_format format,
				enum layout_format layout);
void host1x_pixelbuffer_free(struct host1x_pixelbuffer *pixbuf);
int host1x_pixelbuffer_load_data(struct host1x *host1x,
				 struct host1x_pixelbuffer *pixbuf,
				 void *data,
				 unsigned data_pitch,
				 unsigned long data_size,
				 enum pixel_format data_format,
				 enum layout_format data_layout);
void host1x_pixelbuffer_setup_guard(struct host1x_pixelbuffer *pixbuf);
void host1x_pixelbuffer_check_guard(struct host1x_pixelbuffer *pixbuf);
void host1x_pixelbuffer_disable_bo_guard(void);
bool host1x_pixelbuffer_bo_guard_disabled(void);

struct host1x *host1x_open(bool open_display, int fd);
void host1x_close(struct host1x *host1x);

struct host1x_display *host1x_get_display(struct host1x *host1x);
struct host1x_gr2d *host1x_get_gr2d(struct host1x *host1x);
struct host1x_gr3d *host1x_get_gr3d(struct host1x *host1x);

struct host1x_framebuffer {
	struct host1x_pixelbuffer *pixbuf;
	unsigned long flags;
	uint32_t handle;
};

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
struct host1x_bo *host1x_bo_wrap(struct host1x_bo *bo,
				 unsigned long offset, size_t size);
void host1x_bo_free(struct host1x_bo *bo);
int host1x_bo_invalidate(struct host1x_bo *bo, unsigned long offset,
			 size_t length);
int host1x_bo_flush(struct host1x_bo *bo, unsigned long offset,
		    size_t length);
int host1x_bo_mmap(struct host1x_bo *bo, void **ptr);

static inline struct host1x_bo *host1x_bo_create_helper(struct host1x *host1x,
						size_t size, int flags,
						const char *file, int line)
{
	struct host1x_bo *bo = host1x_bo_create(host1x, size, flags);
	if (!bo)
		fprintf(stderr,
			"ERROR: %s:%d: host1x_bo_create() failed\n",
			file, line);
	return bo;
}

static inline struct host1x_bo *host1x_bo_wrap_helper(struct host1x_bo *bo,
					unsigned long offset, size_t size,
					const char *file, int line)
{
	struct host1x_bo *wrap = host1x_bo_wrap(bo, offset, size);
	if (!wrap)
		fprintf(stderr,
			"ERROR: %s:%d: host1x_bo_wrap() failed\n",
			file, line);
	return wrap;
}

static inline int host1x_bo_invalidate_helper(struct host1x_bo *bo,
					unsigned long offset, size_t length,
					const char *file, int line)
{
	int err = host1x_bo_invalidate(bo, offset, length);
	if (err)
		fprintf(stderr,
			"ERROR: %s:%d: host1x_bo_invalidate() failed %d\n",
			file, line, err);
	return err;
}

static inline int host1x_bo_flush_helper(struct host1x_bo *bo,
					 unsigned long offset, size_t length,
					 const char *file, int line)
{
	int err = host1x_bo_flush(bo, offset, length);
	if (err)
		fprintf(stderr,
			"ERROR: %s:%d: host1x_bo_flush() failed %d\n",
			file, line, err);
	return err;
}

static inline int host1x_bo_mmap_helper(struct host1x_bo *bo, void **ptr,
					const char *file, int line)
{
	int err = host1x_bo_mmap(bo, ptr);
	if (err)
		fprintf(stderr,
			"ERROR: %s:%d: host1x_bo_mmap() failed %d\n",
			file, line, err);
	return err;
}

#define HOST1X_BO_CREATE(host1x, size, flags) \
	host1x_bo_create_helper(host1x, size, flags, __FILE__, __LINE__)

#define HOST1X_BO_WRAP(bo, offset, size) \
	host1x_bo_wrap_helper(bo, offset, size, __FILE__, __LINE__)

#define HOST1X_BO_INVALIDATE(bo, offset, length) \
	host1x_bo_invalidate_helper(bo, offset, length, __FILE__, __LINE__)

#define HOST1X_BO_FLUSH(bo, offset, length) \
	host1x_bo_flush_helper(bo, offset, length, __FILE__, __LINE__)

#define HOST1X_BO_MMAP(bo, ptr) \
	host1x_bo_mmap_helper(bo, ptr, __FILE__, __LINE__)

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
	((0xeu << 28) | (((subop) & 0xf) << 24) | ((value) & 0xffffff))

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

static inline int host1x_pushbuf_push_float(struct host1x_pushbuf *pb, float f)
{
	union {
		uint32_t u;
		float f;
	} value;
	value.f = f;
	return host1x_pushbuf_push(pb, value.u);
}

static inline struct host1x_job *host1x_job_create_helper(
					uint32_t syncpt, uint32_t increments,
					const char *file, int line)
{
	struct host1x_job *job = host1x_job_create(syncpt, increments);
	if (!job)
		fprintf(stderr,
			"ERROR: %s:%d: host1x_job_create() failed\n",
			file, line);
	return job;
}

static inline struct host1x_pushbuf *host1x_job_append_helper(
					struct host1x_job *job,
					struct host1x_bo *bo,
					unsigned long offset,
					const char *file, int line)
{
	struct host1x_pushbuf *pb = host1x_job_append(job, bo, offset);
	if (!pb)
		fprintf(stderr,
			"ERROR: %s:%d: host1x_job_append() failed\n",
			file, line);
	return pb;
}

static inline int host1x_pushbuf_relocate_helper(struct host1x_pushbuf *pb,
						   struct host1x_bo *target,
						   unsigned long offset,
						   unsigned long shift,
						   const char *file, int line)
{
	int err = host1x_pushbuf_relocate(pb, target, offset, shift);
	if (err)
		fprintf(stderr,
			"ERROR: %s:%d: host1x_pushbuf_relocate() failed %d\n",
			file, line, err);
	return err;
}

static inline int host1x_client_submit_helper(struct host1x_client *client,
					      struct host1x_job *job,
					      const char *file, int line)
{
	int err = host1x_client_submit(client, job);
	if (err)
		fprintf(stderr,
			"ERROR: %s:%d: host1x_client_submit() failed %d\n",
			file, line, err);
	return err;
}

static inline int host1x_client_flush_helper(struct host1x_client *client,
					     uint32_t *fence,
					     const char *file, int line)
{
	int err = host1x_client_flush(client, fence);
	if (err)
		fprintf(stderr,
			"ERROR: %s:%d: host1x_client_flush() failed %d\n",
			file, line, err);
	return err;
}

static inline int host1x_client_wait_helper(struct host1x_client *client,
					    uint32_t fence, uint32_t timeout,
					    const char *file, int line)
{
	int err = host1x_client_wait(client, fence, timeout);
	if (err)
		fprintf(stderr,
			"ERROR: %s:%d: host1x_client_wait() failed %d\n",
			file, line, err);
	return err;
}

#define HOST1X_JOB_CREATE(syncpt, increments) \
	host1x_job_create_helper(syncpt, increments, __FILE__, __LINE__)

#define HOST1X_JOB_APPEND(job, bo, offset) \
	host1x_job_append_helper(job, bo, offset, __FILE__, __LINE__)

#define HOST1X_PUSHBUF_RELOCATE(pb, target, offset, shift) \
	host1x_pushbuf_relocate_helper(pb, target, offset, shift, \
					__FILE__, __LINE__)

#define HOST1X_CLIENT_SUBMIT(client, job) \
	host1x_client_submit_helper(client, job, __FILE__, __LINE__)

#define HOST1X_CLIENT_FLUSH(client, fence) \
	host1x_client_flush_helper(client, fence, __FILE__, __LINE__)

#define HOST1X_CLIENT_WAIT(client, fence, timeout) \
	host1x_client_wait_helper(client, fence, timeout, __FILE__, __LINE__)

struct host1x_framebuffer *host1x_framebuffer_create(struct host1x *host1x,
						     unsigned int width,
						     unsigned int height,
						     enum pixel_format format,
						     enum layout_format layout,
						     unsigned long flags);
void host1x_framebuffer_free(struct host1x_framebuffer *fb);
int host1x_framebuffer_save(struct host1x *host1x,
			    struct host1x_framebuffer *fb,
			    const char *path);

struct host1x_gr2d;
struct host1x_gr3d;

int host1x_gr2d_clear(struct host1x_gr2d *gr2d,
		      struct host1x_pixelbuffer *pixbuf,
		      uint32_t color);
int host1x_gr2d_clear_rect(struct host1x_gr2d *gr2d,
			   struct host1x_pixelbuffer *pixbuf,
			   uint32_t color,
			   unsigned x, unsigned y,
			   unsigned width, unsigned height);
int host1x_gr2d_blit(struct host1x_gr2d *gr2d,
		     struct host1x_pixelbuffer *src,
		     struct host1x_pixelbuffer *dst,
		     unsigned int sx, unsigned int sy,
		     unsigned int dx, unsigned int dy,
		     unsigned int width, int height);
int host1x_gr2d_surface_blit(struct host1x_gr2d *gr2d,
			     struct host1x_pixelbuffer *src,
			     struct host1x_pixelbuffer *dst,
			     unsigned int sx, unsigned int sy,
			     unsigned int src_width, unsigned int src_height,
			     unsigned int dx, unsigned int dy,
			     unsigned int dst_width, int dst_height);
int host1x_gr3d_triangle(struct host1x_gr3d *gr3d,
			 struct host1x_pixelbuffer *pixbuf);

#endif
