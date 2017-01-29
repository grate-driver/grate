#ifndef GRATE_LIBGRATE_PRIVATE_H
#define GRATE_LIBGRATE_PRIVATE_H 1

#include "grate.h"
#include "libcgc.h"

struct host1x_pushbuf;

struct grate_region {
	unsigned long start;
	unsigned long end;

	struct grate_region *prev, *next;
};

struct grate_color {
	float r, g, b, a;
};

struct grate_framebuffer {
	struct host1x_framebuffer *front;
	struct host1x_framebuffer *back;
};

void grate_framebuffer_swap(struct grate_framebuffer *fb);

struct grate {
	struct grate_options *options;
	struct grate_display *display;
	struct grate_overlay *overlay;
	struct grate_framebuffer *fb;
	struct grate_color clear;
	struct host1x *host1x;
};

#define grate_error(fmt, args...) \
	fprintf(stderr, "ERROR: %s: " fmt, __func__, ##args)

#endif
