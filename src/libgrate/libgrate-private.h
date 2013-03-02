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

	struct nvhost_ctrl *ctrl;
	struct nvmap *nvmap;
	struct nvhost_gr2d *gr2d;
	struct nvhost_gr3d *gr3d;
};

#endif
