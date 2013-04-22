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

#include <string.h>

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
	unsigned long offset = 0, flags;
	struct grate_program *program;
	struct grate_framebuffer *fb;
	struct grate_shader *vs, *fs;
	struct grate_options options;
	struct grate *grate;
	struct grate_bo *bo;
	void *buffer;
	int location;

	if (!grate_parse_command_line(&options, argc, argv))
		return 1;

	grate = grate_init(&options);
	if (!grate)
		return 1;

	bo = grate_bo_create(grate, 4096, 0);
	if (!bo) {
		grate_exit(grate);
		return 1;
	}

	buffer = grate_bo_map(bo);
	if (!buffer) {
		grate_bo_free(bo);
		grate_exit(grate);
		return 1;
	}

	flags = GRATE_FRAMEBUFFER_FRONT;

	fb = grate_framebuffer_create(grate, options.width, options.height,
				      GRATE_RGBA8888, flags);
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

	grate_viewport(grate, 0.0f, 0.0f, options.width, options.height);
	grate_use_program(grate, program);

	location = grate_get_attribute_location(grate, "position");
	if (location < 0) {
		fprintf(stderr, "\"position\": attribute not found\n");
		return 1;
	}

	memcpy(buffer + offset, vertices, sizeof(vertices));
	grate_attribute_pointer(grate, location, sizeof(float), 4, 4, bo,
				offset);
	offset += sizeof(vertices);

	location = grate_get_attribute_location(grate, "color");
	if (location < 0) {
		fprintf(stderr, "\"color\": attribute not found\n");
		return 1;
	}

	memcpy(buffer + offset, colors, sizeof(colors));
	grate_attribute_pointer(grate, location, sizeof(float), 4, 4, bo,
				offset);
	offset += sizeof(colors);

	memcpy(buffer + offset, indices, sizeof(indices));
	grate_draw_elements(grate, GRATE_TRIANGLES, 2, 6, bo, offset);
	grate_flush(grate);

	grate_swap_buffers(grate);
	grate_wait_for_key(grate);

	grate_exit(grate);
	return 0;
}
