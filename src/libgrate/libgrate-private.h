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

struct grate {
	struct grate_options *options;
	struct grate_display *display;
	struct grate_overlay *overlay;
	struct grate_framebuffer *fb;
	struct grate_color clear;
	struct host1x *host1x;
};

struct grate_display *grate_display_open(struct grate *grate);
void grate_display_close(struct grate_display *display);
void grate_display_get_resolution(struct grate_display *display,
				  unsigned int *width, unsigned int *height);
void grate_display_show(struct grate_display *display,
			struct grate_framebuffer *fb, bool vsync);

struct grate_overlay *grate_overlay_create(struct grate_display *display);
void grate_overlay_free(struct grate_overlay *overlay);
void grate_overlay_show(struct grate_overlay *overlay,
			struct grate_framebuffer *fb, unsigned int x,
			unsigned int y, unsigned int width,
			unsigned int height, bool vsync);

#define grate_error(fmt, args...) \
	fprintf(stderr, "ERROR: %s: " fmt, __func__, ##args)

#endif
