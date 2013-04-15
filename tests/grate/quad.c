/*
 * Copyright (c) 2012, 2013 Erik Faye-Lund
 * Copyright (c) 2013 Avionic Design GmbH
 * Copyright (c) 2013 Thierry Reding
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

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
	-0.5f, -0.5f, 0.0f, 1.0f,
	 0.5f, -0.5f, 0.0f, 1.0f,
	 0.5f,  0.5f, 0.0f, 1.0f,
	-0.5f,  0.5f, 0.0f, 1.0f,
};

static const float colors[] = {
	1.0f, 0.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 0.0f, 1.0f,
};

static const unsigned short indices[] = {
	0, 1, 2,
	0, 2, 3,
};

int main(int argc, char *argv[])
{
	struct grate_program *program;
	struct grate_framebuffer *fb;
	struct grate_shader *vs, *fs;
	unsigned int height = 256;
	unsigned int width = 256;
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

	grate_attribute_pointer(grate, "position", sizeof(float), 4, 4,
				vertices);
	grate_attribute_pointer(grate, "color", sizeof(float), 4, 4, colors);

	grate_draw_elements(grate, GRATE_TRIANGLES, 2, 6, indices);
	grate_flush(grate);

	grate_framebuffer_save(fb, "test.png");

	grate_exit(grate);
	return 0;
}
