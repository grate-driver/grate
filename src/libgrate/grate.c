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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "../libhost1x/host1x-private.h"
#include "libgrate-private.h"

#include "grate.h"
#include "host1x.h"

struct grate_bo *grate_bo_create(struct grate *grate, size_t size,
				 unsigned long flags)
{
	struct grate_bo *bo;

	bo = calloc(1, sizeof(*bo));
	if (!bo)
		return NULL;

	bo->bo = host1x_bo_create(grate->host1x, size, 2);
	if (!bo->bo) {
		free(bo);
		return NULL;
	}

	bo->size = size;

	return bo;
}

void grate_bo_free(struct grate_bo *bo)
{
	host1x_bo_free(bo->bo);
	free(bo);
}

void *grate_bo_map(struct grate_bo *bo)
{
	void *ptr = NULL;
	int err;

	err = host1x_bo_mmap(bo->bo, &ptr);
	if (err < 0)
		return NULL;

	return ptr;
}

void grate_bo_unmap(struct grate_bo *bo, void *ptr)
{
}

bool grate_parse_command_line(struct grate_options *options, int argc,
			      char *argv[])
{
	static const struct option long_opts[] = {
		{ "fullscreen", 0, NULL, 'f' },
		{ "width", 1, NULL, 'w' },
		{ "height", 1, NULL, 'h' },
		{ "vsync", 0, NULL, 'v' },
	};
	static const char opts[] = "fw:h:v";
	int opt;

	options->fullscreen = false;
	options->vsync = false;
	options->x = 0;
	options->y = 0;
	options->width = 256;
	options->height = 256;

	while ((opt = getopt_long(argc, argv, opts, long_opts, NULL)) != -1) {
		switch (opt) {
		case 'f':
			options->fullscreen = true;
			break;

		case 'w':
			options->width = atoi(optarg);
			break;

		case 'h':
			options->height = atoi(optarg);
			break;

		case 'v':
			options->vsync = true;
			break;

		default:
			return false;
		}
	}

	return true;
}

struct grate *grate_init(struct grate_options *options)
{
	struct grate *grate;

	grate = calloc(1, sizeof(*grate));
	if (!grate)
		return NULL;

	grate->host1x = host1x_open();
	if (!grate->host1x) {
		free(grate);
		return NULL;
	}

	grate->options = options;

	grate->display = grate_display_open(grate);
	if (grate->display) {
		if (!grate->options->fullscreen)
			grate->overlay = grate_overlay_create(grate->display);

		if (!grate->overlay)
			grate_display_get_resolution(grate->display,
						     &grate->options->width,
						     &grate->options->height);
	}

	return grate;
}

void grate_exit(struct grate *grate)
{
	if (grate)
		host1x_close(grate->host1x);

	free(grate);
}

void grate_viewport(struct grate *grate, float x, float y, float width,
		    float height)
{
	grate->viewport.x = x;
	grate->viewport.y = y;
	grate->viewport.width = width;
	grate->viewport.height = height;
}

void grate_clear_color(struct grate *grate, float red, float green,
		       float blue, float alpha)
{
	grate->clear.r = red;
	grate->clear.g = green;
	grate->clear.b = blue;
	grate->clear.a = alpha;
}

void grate_clear(struct grate *grate)
{
	struct host1x_gr2d *gr2d = host1x_get_gr2d(grate->host1x);
	struct grate_color *clear = &grate->clear;
	int err;

	if (!grate->fb) {
		grate_error("no framebuffer bound to state\n");
		return;
	}

	err = host1x_gr2d_clear(gr2d, grate->fb->back, clear->r, clear->g,
				clear->b, clear->a);
	if (err < 0)
		grate_error("host1x_gr2d_clear() failed: %d\n", err);
}

void grate_bind_framebuffer(struct grate *grate, struct grate_framebuffer *fb)
{
	grate->fb = fb;
}

int grate_get_attribute_location(struct grate *grate, const char *name)
{
	struct grate_program *program = grate->program;
	unsigned int i;

	for (i = 0; i < GRATE_MAX_ATTRIBUTES; i++) {
		struct grate_attribute *attribute = &program->attributes[i];

		if (strcmp(name, attribute->name) == 0)
			return attribute->position;
	}

	return -1;
}

void grate_attribute_pointer(struct grate *grate, unsigned int location,
			     unsigned int size, unsigned int stride,
			     unsigned int count, struct grate_bo *bo,
			     unsigned long offset)
{
	struct grate_vertex_attribute *attribute;
	size_t length;
	int err;

	if (location > GRATE_MAX_ATTRIBUTES) {
		fprintf(stderr, "ERROR: invalid location: %u\n", location);
		return;
	}

	//fprintf(stdout, "DEBUG: using location %u\n", location);
	attribute = &grate->attributes[location];
	length = count * stride * size;

	attribute->offset = offset;
	attribute->stride = stride;
	attribute->count = count;
	attribute->size = size;
	attribute->bo = bo;

	err = host1x_bo_invalidate(bo->bo, offset, length);
	if (err < 0) {
		fprintf(stderr, "ERROR: failed to invalidate buffer\n");
		return;
	}
}

int grate_get_uniform_location(struct grate *grate, const char *name)
{
	struct grate_program *program = grate->program;
	unsigned int i;

	for (i = 0; i < program->num_uniforms; i++) {
		struct grate_uniform *uniform = &program->uniforms[i];

		if (strcmp(name, uniform->name) == 0)
			return uniform->position;
	}

	return -1;
}

void grate_uniform(struct grate *grate, unsigned int location,
		   unsigned int count, float *values)
{
	struct grate_program *program = grate->program;
	unsigned int i;

	//fprintf(stdout, "DEBUG: using location %u\n", location);

	for (i = 0; i < count; i++)
		program->uniform[location * 4 + i] = values[i];
}

enum host1x_gr3d_type {
	HOST1X_GR3D_UBYTE,
	HOST1X_GR3D_UBYTE_NORM,
	HOST1X_GR3D_SBYTE,
	HOST1X_GR3D_SBYTE_NORM,
	HOST1X_GR3D_USHORT,
	HOST1X_GR3D_USHORT_NORM,
	HOST1X_GR3D_SSHORT,
	HOST1X_GR3D_SSHORT_NORM,
	HOST1X_GR3D_FIXED = 0xc,
	HOST1X_GR3D_FLOAT,
};

enum host1x_gr3d_index {
	HOST1X_GR3D_INDEX_NONE,
	HOST1X_GR3D_INDEX_UINT8,
	HOST1X_GR3D_INDEX_UINT16,
};

enum host1x_gr3d_primitive {
	HOST1X_GR3D_POINTS,
	HOST1X_GR3D_LINES,
	HOST1X_GR3D_LINE_LOOP,
	HOST1X_GR3D_LINE_STRIP,
	HOST1X_GR3D_TRIANGLES,
	HOST1X_GR3D_TRIANGLE_STRIP,
	HOST1X_GR3D_TRIANGLE_FAN,
};

void grate_draw_elements(struct grate *grate, enum grate_primitive type,
			 unsigned int size, unsigned int count,
			 struct grate_bo *bo, unsigned long offset)
{
	struct host1x_gr3d *gr3d = host1x_get_gr3d(grate->host1x);
	struct host1x_syncpt *syncpt = &gr3d->client->syncpts[0];
	struct host1x_framebuffer *fb = grate->fb->back;
	struct grate_program *program = grate->program;
	struct grate_viewport *vp = &grate->viewport;
	unsigned long length = count * size;
	enum host1x_gr3d_primitive mode;
	uint32_t format, pitch, fence;
	enum host1x_gr3d_index index;
	unsigned int depth = 32, i;
	struct host1x_pushbuf *pb;
	struct host1x_job *job;
	int tiled = 1;
	int err;

	switch (type) {
	case GRATE_POINTS:         mode = HOST1X_GR3D_POINTS;         break;
	case GRATE_LINES:          mode = HOST1X_GR3D_LINES;          break;
	case GRATE_LINE_STRIP:     mode = HOST1X_GR3D_LINE_STRIP;     break;
	case GRATE_LINE_LOOP:      mode = HOST1X_GR3D_LINE_LOOP;      break;
	case GRATE_TRIANGLES:      mode = HOST1X_GR3D_TRIANGLES;      break;
	case GRATE_TRIANGLE_STRIP: mode = HOST1X_GR3D_TRIANGLE_STRIP; break;
	case GRATE_TRIANGLE_FAN:   mode = HOST1X_GR3D_TRIANGLE_FAN;   break;

	default:
		fprintf(stderr, "ERROR: unsupported type: %d\n", type);
		return;
	}

	switch (size) {
	case 2:
		index = HOST1X_GR3D_INDEX_UINT16;
		break;

	default:
		fprintf(stderr, "ERROR: unsupported size: %d\n", size);
		return;
	}

	/* invalidate memory for indices */
	err = host1x_bo_invalidate(bo->bo, offset, length);
	if (err < 0) {
		fprintf(stderr, "ERROR: failed to invalidate buffer\n");
		return;
	}

	/*
	 * build command stream
	 */

	job = host1x_job_create(syncpt->id, 1);
	if (!job)
		return;

	pb = host1x_job_append(job, gr3d->commands, 0);
	if (!pb) {
		host1x_job_free(job);
		return;
	}

	host1x_pushbuf_push(pb, HOST1X_OPCODE_SETCL(0x000, 0x060, 0x00));

	/* depth range */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x404, 2));
	host1x_pushbuf_push(pb, 0x00000000);
	host1x_pushbuf_push(pb, 0x000fffff);

	/* viewport z bias and scale */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_MASK(0x354, 9));
	/* 2^-21 = half an ULP in the z-buffer */
	host1x_pushbuf_push(pb, 0.5f - powf(2.0f, -21));
	host1x_pushbuf_push(pb, 0.5f - powf(2.0f, -21));

	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x740, 0x035));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe26, 0x779));

	/* point params and size */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x346, 2));
	host1x_pushbuf_push(pb, 0x00001401);
	host1x_pushbuf_push_float(pb, 1.0f);

	/* line params */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x34c, 1));
	host1x_pushbuf_push(pb, 0x00000002);

	host1x_gr3d_line_width(pb, 1.0f);

	host1x_gr3d_viewport(pb, vp->x, vp->y, vp->width, vp->height);

	/* guardband ? */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x358, 0x03));
	host1x_pushbuf_push(pb, 0x4376f000);
	host1x_pushbuf_push(pb, 0x4376f000);
	host1x_pushbuf_push(pb, 0x40dfae14);

	/* cull face */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x343, 0x01));
	host1x_pushbuf_push(pb, 0xb8e00000);

	/* scissor */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x350, 0x02));
	host1x_pushbuf_push(pb, fb->width  & 0xffff);
	host1x_pushbuf_push(pb, fb->height & 0xffff);

	/* rt 1 color params */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xe11, 0x01));

	if (depth == 16) {
		format = HOST1X_GR3D_FORMAT_RGB565;
		pitch = fb->width * 2;
	} else {
		format = HOST1X_GR3D_FORMAT_RGBA8888;
		pitch = fb->width * 4;
	}

	host1x_pushbuf_push(pb, (tiled << 26) | (pitch << 8) | format << 2 | 0x1);

	/* write masks */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x903, 0x01));
	host1x_pushbuf_push(pb, 0x00000002);

	/* rt 4..10  color params */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xe15, 0x07));
	host1x_pushbuf_push(pb, 0x08000001);
	host1x_pushbuf_push(pb, 0x08000001);
	host1x_pushbuf_push(pb, 0x08000001);
	host1x_pushbuf_push(pb, 0x08000001);
	host1x_pushbuf_push(pb, 0x08000001);
	host1x_pushbuf_push(pb, 0x08000001);
	host1x_pushbuf_push(pb, 0x08000001);

	/* rt 0 color params */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xe10, 0x01));
	host1x_pushbuf_push(pb, 0x0c000000);

	/* rt 3 color params */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xe13, 0x01));
	host1x_pushbuf_push(pb, 0x0c000000);

	/* rt 2 color params */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xe12, 0x01));
	host1x_pushbuf_push(pb, 0x0c000000);

	/* point coord range */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x348, 0x04));
	host1x_pushbuf_push_float(pb, 1.0f);
	host1x_pushbuf_push_float(pb, 0.0f);
	host1x_pushbuf_push_float(pb, 0.0f);
	host1x_pushbuf_push_float(pb, 1.0f);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe27, 0x01));


	for (i = 0; i < 4; i++) {
		host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xa02, 0x06));
		host1x_pushbuf_push(pb, 0x000001ff);
		host1x_pushbuf_push(pb, 0x000001ff);
		host1x_pushbuf_push(pb, 0x000001ff);
		host1x_pushbuf_push(pb, 0x00000030);
		host1x_pushbuf_push(pb, 0x00000020);
		host1x_pushbuf_push(pb, 0x00000030);
		host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa00, 0xe00));
		host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa08, 0x100));
	}

	grate_shader_emit(pb, grate->program->vs);

	/* cull face */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x343, 0x01));
	host1x_pushbuf_push(pb, 0xb8e00000);

	/* link "program" */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x300, 0x02));
	host1x_pushbuf_push(pb, 0x00000008);
	host1x_pushbuf_push(pb, 0x0000fecd);

	/* unknown */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xe20, 0x01));
	host1x_pushbuf_push(pb, 0x58000000);

	/* reset upload counters ? */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x503, 0x00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x545, 0x00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe22, 0x00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x603, 0x00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x803, 0x00));

	/* unknown */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x520, 0x01));
	host1x_pushbuf_push(pb, 0x20006001);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x546, 0x01));
	host1x_pushbuf_push(pb, 0x00000040);

	grate_shader_emit(pb, grate->program->fs);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xa02, 0x06));
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, 0x00000020);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa00, 0xe00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa08, 0x100));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x40c, 0x06));

	/* upload uniforms */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x207, 0x001));
	host1x_pushbuf_push(pb, 0x00000000);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x208, 256 * 4));

	for (i = 0; i < 256; i++) {
		uint32_t *ptr = (void *)&program->uniform[i * 4];

		host1x_pushbuf_push(pb, ptr[0]);
		host1x_pushbuf_push(pb, ptr[1]);
		host1x_pushbuf_push(pb, ptr[2]);
		host1x_pushbuf_push(pb, ptr[3]);
	}

	/* varying flags */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x120, 0x01));
	host1x_pushbuf_push(pb, 0x00030081);

	/* polygon offset */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x344, 0x02));
	host1x_pushbuf_push_float(pb, 0.0f);
	host1x_pushbuf_push_float(pb, 0.0f);

	/* needed for lines, it seems! */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa00, 0xe01));

	/* relocate color render target */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xe01, 0x01));
	host1x_pushbuf_relocate(pb, fb->bo, 0, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);

	for (i = 0; i < GRATE_MAX_ATTRIBUTES; i++) {
		unsigned int reg = 0x100 + (i << 1);
		struct grate_vertex_attribute *attr;
		uint32_t value, stride;

		attr = &grate->attributes[i];
		if (!attr->bo)
			continue;

		host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(reg, 0x02));
		host1x_pushbuf_relocate(pb, attr->bo->bo, attr->offset, 0);
		host1x_pushbuf_push(pb, 0xdeadbeef);

		stride = attr->stride * sizeof(float);

		value = stride << 8 | attr->size << 4 | HOST1X_GR3D_FLOAT;

		host1x_pushbuf_push(pb, value);
	}

	/* primitive indices */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x121, 0x03));
	host1x_pushbuf_relocate(pb, bo->bo, offset, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);

	/* do the actual draw */
	host1x_pushbuf_push(pb, 0xc8000000 | (index << 28) | (mode << 24));
	host1x_pushbuf_push(pb, (count - 1) << 20);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe27, 0x02));

	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x00, 0x01));
	host1x_pushbuf_push(pb, 0x000001 << 8 | syncpt->id);

	err = host1x_client_submit(gr3d->client, job);
	if (err < 0) {
		host1x_job_free(job);
		return;
	}

	host1x_job_free(job);

	err = host1x_client_flush(gr3d->client, &fence);
	if (err < 0)
		return;

	err = host1x_client_wait(gr3d->client, fence, -1);
	if (err < 0)
		return;
}

void grate_flush(struct grate *grate)
{
}

struct grate_framebuffer *grate_framebuffer_create(struct grate *grate,
						   unsigned int width,
						   unsigned int height,
						   enum grate_format format,
						   unsigned long flags)
{
	struct grate_framebuffer *fb;
	unsigned int bpp = 32;

	if (format != GRATE_RGBA8888)
		return NULL;

	fb = calloc(1, sizeof(*fb));
	if (!fb)
		return NULL;

	fb->front = host1x_framebuffer_create(grate->host1x, width, height,
					      bpp, 0);
	if (!fb->front) {
		free(fb);
		return NULL;
	}

	if (flags & GRATE_DOUBLE_BUFFERED) {
		fb->back = host1x_framebuffer_create(grate->host1x, width,
						     height, bpp, 0);
		if (!fb->back) {
			host1x_framebuffer_free(fb->front);
			free(fb);
			return NULL;
		}
	}

	return fb;
}

void grate_framebuffer_free(struct grate_framebuffer *fb)
{
	if (fb) {
		host1x_framebuffer_free(fb->front);
		host1x_framebuffer_free(fb->back);
	}

	free(fb);
}

void grate_framebuffer_swap(struct grate_framebuffer *fb)
{
	struct host1x_framebuffer *tmp = fb->front;

	fb->front = fb->back;
	fb->back = tmp;
}

void grate_framebuffer_save(struct grate_framebuffer *fb, const char *path)
{
	host1x_framebuffer_save(fb->back, path);
}

void grate_use_program(struct grate *grate, struct grate_program *program)
{
	grate->program = program;
}

void grate_swap_buffers(struct grate *grate)
{
	if (grate->display || grate->overlay) {
		struct grate_options *options = grate->options;

		if (grate->overlay)
			grate_overlay_show(grate->overlay, grate->fb, 0, 0,
					   options->width, options->height,
					   options->vsync);
		else
			grate_display_show(grate->display, grate->fb,
					   options->vsync);
	} else {
		grate_framebuffer_save(grate->fb, "test.png");
	}
}

void grate_wait_for_key(struct grate *grate)
{
	/*
	 * If on-screen display isn't supported, don't wait for key-press
	 * because the output is rendered into a PNG file.
	 */
	if (!grate->display && !grate->overlay)
		return;

	getchar();
}

bool grate_key_pressed(struct grate *grate)
{
	int err, max_fd = STDIN_FILENO;
	struct timeval timeout;
	fd_set fds;

	/*
	 * If on-screen display isn't supported, pretend that a key was
	 * pressed so that the main loop can be exited.
	 */
	if (!grate->display && !grate->overlay)
		return true;

	memset(&timeout, 0, sizeof(timeout));
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);

	err = select(max_fd + 1, &fds, NULL, NULL, &timeout);
	if (err <= 0) {
		if (err < 0) {
			grate_error("select() failed: %m\n");
			return false;
		}

		return false;
	}

	if (FD_ISSET(STDIN_FILENO, &fds))
		return true;

	return false;
}
