#include "grate.h"

int main(int argc, char *argv[])
{
	struct grate_framebuffer *fb;
	unsigned int height = 32;
	unsigned int width = 32;
	struct grate *grate;

	grate = grate_init();
	if (!grate)
		return 1;

	fb = grate_framebuffer_new(grate, width, height, GRATE_RGBA8888);
	if (!fb)
		return 1;

	grate_clear_color(grate, 1.0f, 0.0f, 1.0f, 1.0f);
	grate_bind_framebuffer(grate, fb);
	grate_clear(grate);

	grate_framebuffer_save(fb, "test.png");

	grate_exit(grate);
	return 0;
}
