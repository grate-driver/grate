#include "grate.h"

static const char *vertex_shader[] = {
	"attribute vec4 position;\n",
	"attribute vec4 color;\n",
	"varying vec4 vcolor;\n",
	"\n",
	"void main()\n",
	"{\n",
	"    gl_Position = position;\n",
	"    vcolor = color;\n",
	"}"
};

static const char *fragment_shader[] = {
	"precision mediump float;\n",
	"varying vec4 vcolor;\n",
	"\n",
	"void main()\n",
	"{\n",
	"    gl_FragColor = vcolor;\n",
	"}"
};

static const float vertices[] = {
	 0.0f,  0.5f, 0.0f, 1.0f,
	-0.5f, -0.5f, 0.0f, 1.0f,
	 0.5f, -0.5f, 0.0f, 1.0f,
};

static const float colors[] = {
	1.0f, 0.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f, 1.0f,
};

static const unsigned short indices[] = {
	0, 1, 2,
};

int main(int argc, char *argv[])
{
	struct grate_program *program;
	struct grate_framebuffer *fb;
	struct grate_shader *vs, *fs;
	unsigned int height = 32;
	unsigned int width = 32;
	struct grate *grate;

	grate = grate_init();
	if (!grate)
		return 1;

	fb = grate_framebuffer_new(grate, width, height, GRATE_RGBA8888);
	if (!fb)
		return 1;

	grate_clear_color(grate, 0.0f, 0.0f, 0.0f, 1.0f);
	grate_bind_framebuffer(grate, fb);
	grate_clear(grate);

	vs = grate_shader_new(grate, GRATE_SHADER_VERTEX, vertex_shader,
			      ARRAY_SIZE(vertex_shader));
	fs = grate_shader_new(grate, GRATE_SHADER_FRAGMENT, fragment_shader,
			      ARRAY_SIZE(fragment_shader));
	program = grate_program_new(grate, vs, fs);
	grate_program_link(program);

	grate_use_program(grate, program);

	grate_attribute_pointer(grate, "position", sizeof(float), 4, 3,
				vertices);
	grate_attribute_pointer(grate, "color", sizeof(float), 4, 3, colors);

	grate_draw_elements(grate, GRATE_TRIANGLES, 2, 3, indices);
	grate_flush(grate);

	grate_framebuffer_save(fb, "test.png");

	grate_exit(grate);
	return 0;
}
