#include <stdlib.h>

#include "grate.h"

#include "gr2d.h"
#include "gr3d.h"

struct grate_framebuffer {
	struct nvmap_framebuffer *base;
};

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

struct grate *grate_init(void)
{
	struct grate *grate;

	grate = calloc(1, sizeof(*grate));
	if (!grate)
		return NULL;

	grate->nvmap = nvmap_open();
	if (!grate->nvmap) {
		free(grate);
		return NULL;
	}

	grate->ctrl = nvhost_ctrl_open();
	if (!grate->ctrl) {
		free(grate);
		return NULL;
	}

	grate->gr2d = nvhost_gr2d_open(grate->nvmap, grate->ctrl);
	if (!grate->gr2d) {
		free(grate);
		return NULL;
	}

	grate->gr3d = nvhost_gr3d_open(grate->nvmap, grate->ctrl);
	if (!grate->gr3d) {
		free(grate);
		return NULL;
	}

	return grate;
}

void grate_exit(struct grate *grate)
{
	if (grate) {
		nvhost_gr3d_close(grate->gr3d);
		nvhost_gr2d_close(grate->gr2d);
		nvhost_ctrl_close(grate->ctrl);
		nvmap_close(grate->nvmap);
	}

	free(grate);
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
	struct grate_color *clear = &grate->clear;
	int err;

	if (!grate->fb)
		return;

	err = nvhost_gr2d_clear(grate->gr2d, grate->fb->base, clear->r,
				clear->g, clear->b, clear->a);
	if (err < 0) {
	}
}

void grate_bind_framebuffer(struct grate *grate, struct grate_framebuffer *fb)
{
	grate->fb = fb;
}

void grate_attribute_pointer(struct grate *grate, const char *name,
			     unsigned int size, unsigned int stride,
			     unsigned int count, const void *buffer)
{
}

void grate_draw_elements(struct grate *grate, enum grate_primitive type,
			 unsigned int size, unsigned int count,
			 const void *indices)
{
}

void grate_flush(struct grate *grate)
{
}

struct grate_framebuffer *grate_framebuffer_new(struct grate *grate,
						unsigned int width,
						unsigned int height,
						enum grate_format format)
{
	struct grate_framebuffer *fb;
	unsigned int bpp = 32;

	if (format != GRATE_RGBA8888)
		return NULL;

	fb = calloc(1, sizeof(*fb));
	if (!fb)
		return NULL;

	fb->base = nvmap_framebuffer_create(grate->nvmap, width, height, bpp);
	if (!fb->base) {
		free(fb);
		return NULL;
	}

	return fb;
}

void grate_framebuffer_free(struct grate_framebuffer *fb)
{
	if (fb)
		nvmap_framebuffer_free(fb->base);

	free(fb);
}

void grate_framebuffer_save(struct grate_framebuffer *fb, const char *path)
{
	nvmap_framebuffer_save(fb->base, path);
}

void grate_use_program(struct grate *grate, struct grate_program *program)
{
	grate->program = program;
}
