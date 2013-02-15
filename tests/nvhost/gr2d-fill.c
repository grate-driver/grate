#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "nvhost.h"
#include "gr2d.h"

int main(int argc, char *argv[])
{
	struct nvmap_framebuffer *fb;
	unsigned int width, height;
	struct nvhost_gr2d *gr2d;
	struct nvhost_ctrl *ctrl;
	struct nvmap *nvmap;
	int err;

	width = height = 32;

	nvmap = nvmap_open();
	if (!nvmap) {
		return 1;
	}

	ctrl = nvhost_ctrl_open();
	if (!ctrl) {
		return 1;
	}

	fb = nvmap_framebuffer_create(nvmap, width, height, 32);
	if (!fb) {
		return 1;
	}

	gr2d = nvhost_gr2d_open(nvmap, ctrl);
	if (!gr2d) {
		return 1;
	}

	err = nvhost_gr2d_clear(gr2d, fb, 1.0f, 0.0f, 1.0f, 1.0f);
	if (err < 0) {
		return 1;
	}

	nvmap_framebuffer_save(fb, "test.png");
	nvmap_framebuffer_free(fb);
	nvhost_gr2d_close(gr2d);
	nvhost_ctrl_close(ctrl);
	nvmap_close(nvmap);

	return 0;
}
