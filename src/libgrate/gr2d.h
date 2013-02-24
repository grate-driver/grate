#ifndef GRATE_GR2D_H
#define GRATE_GR2D_H 1

#include "nvhost.h"

struct nvhost_gr2d {
	struct nvhost_client client;
	struct nvmap_handle *buffer;
	struct nvmap_handle *scratch;
	int fd;
};

struct nvhost_gr2d *nvhost_gr2d_open(struct nvmap *nvmap,
				     struct nvhost_ctrl *ctrl);
void nvhost_gr2d_close(struct nvhost_gr2d *gr2d);
int nvhost_gr2d_clear(struct nvhost_gr2d *gr2d, struct nvmap_framebuffer *fb,
		      float red, float green, float blue, float alpha);

#endif
