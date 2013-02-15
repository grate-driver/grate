#ifndef GRATE_GR3D_H
#define GRATE_GR3D_H 1

#include "nvhost.h"

struct nvhost_gr3d {
	struct nvhost_client client;
	struct nvmap_handle *attributes;
	struct nvmap_handle *buffer;
	int fd;
};

struct nvhost_gr3d *nvhost_gr3d_open(struct nvmap *nvmap,
				     struct nvhost_ctrl *ctrl);
void nvhost_gr3d_close(struct nvhost_gr3d *gr3d);
int nvhost_gr3d_triangle(struct nvhost_gr3d *gr3d,
			 struct nvmap_framebuffer *fb);

#endif
