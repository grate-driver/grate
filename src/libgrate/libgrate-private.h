#ifndef GRATE_LIBGRATE_PRIVATE_H
#define GRATE_LIBGRATE_PRIVATE_H 1

#include "grate.h"
#include "libcgc.h"

#define GRATE_MAX_ATTRIBUTES 16

struct host1x_pushbuf;

struct grate_region {
	unsigned long start;
	unsigned long end;

	struct grate_region *prev, *next;
};

struct grate_color {
	float r, g, b, a;
};

struct grate_bo {
	struct host1x_bo *bo;
	size_t size;
};

struct grate_vertex_attribute {
	struct grate_bo *bo;
	unsigned long offset;
	unsigned int stride;
	unsigned int count;
	unsigned int size;
};

struct grate_attribute {
	unsigned int position;
	const char *name;
};

struct grate_uniform {
	unsigned int position;
	const char *name;
};

struct grate_shader {
	struct cgc_shader *cgc;

	unsigned int num_words;
	uint32_t *words;
};

struct grate_program {
	struct grate_shader *vs;
	struct grate_shader *fs;
	struct grate_shader *linker;

	struct grate_attribute *attributes;
	unsigned int num_attributes;
	uint32_t attributes_mask;

	struct grate_uniform *uniforms;
	unsigned int num_uniforms;
	float uniform[256 * 4];

	uint32_t fs_uniform[32];
};

void grate_shader_emit(struct host1x_pushbuf *pb, struct grate_shader *shader);

struct grate_framebuffer {
	struct host1x_framebuffer *front;
	struct host1x_framebuffer *back;
};

void grate_framebuffer_swap(struct grate_framebuffer *fb);

struct grate_viewport {
	float x;
	float y;
	float width;
	float height;
};

struct grate {
	struct grate_options *options;
	struct grate_display *display;
	struct grate_overlay *overlay;

	struct grate_viewport viewport;
	struct grate_program *program;
	struct grate_framebuffer *fb;
	struct grate_color clear;

	struct grate_vertex_attribute attributes[GRATE_MAX_ATTRIBUTES];

	struct host1x *host1x;
};

#define grate_error(fmt, args...) \
	fprintf(stderr, "ERROR: " fmt, ##args)

#endif
