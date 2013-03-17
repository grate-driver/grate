#ifndef GRATE_LIBGRATE_PRIVATE_H
#define GRATE_LIBGRATE_PRIVATE_H 1

#include "grate.h"

struct grate_color {
	float r, g, b, a;
};

struct grate {
	struct grate_program *program;
	struct grate_framebuffer *fb;
	struct grate_color clear;

	struct host1x *host1x;
};

#endif
