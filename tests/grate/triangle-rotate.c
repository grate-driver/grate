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

#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "grate.h"
#include "matrix.h"

static const GLchar *vertex_shader[] = {
	"attribute vec4 position;\n",
	"uniform mat4 modelview;\n",
	"attribute vec4 color;\n",
	"varying vec4 vcolor;\n",
	"\n",
	"void main()\n",
	"{\n",
	"    gl_Position = position * modelview;\n",
	"    vcolor = color;\n",
	"}"
};

static const GLchar *fragment_shader[] = {
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
	struct grate_profile *profile;
	struct grate_framebuffer *fb;
	struct grate_shader *vs, *fs;
	struct grate_options options;
	unsigned long offset = 0;
	GLfloat angle = 0.0f;
	struct grate *grate;
	struct grate_bo *bo;
	struct mat4 matrix;
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

	fb = grate_framebuffer_create(grate, options.width, options.height,
				      GRATE_RGBA8888, GRATE_DOUBLE_BUFFERED);
	if (!fb) {
		fprintf(stderr, "grate_framebuffer_create() failed\n");
		return 1;
	}

	grate_clear_color(grate, 0.0f, 0.0f, 0.0f, 1.0f);
	grate_bind_framebuffer(grate, fb);

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
	grate_attribute_pointer(grate, location, sizeof(float), 4, 3, bo,
				offset);
	offset += sizeof(vertices);

	location = grate_get_attribute_location(grate, "color");
	if (location < 0) {
		fprintf(stderr, "\"color\": attribute not found\n");
		return 1;
	}

	memcpy(buffer + offset, colors, sizeof(colors));
	grate_attribute_pointer(grate, location, sizeof(float), 4, 3, bo,
				offset);
	offset += sizeof(colors);

	memcpy(buffer + offset, indices, sizeof(indices));

	profile = grate_profile_start(grate);

	while (true) {
		grate_clear(grate);

		mat4_rotate_z(&matrix, angle);

		location = grate_get_uniform_location(grate, "modelview");

		grate_uniform(grate, location, 16, (float *)&matrix);

		grate_draw_elements(grate, GRATE_TRIANGLES, 2, 3, bo, offset);
		grate_flush(grate);
		grate_swap_buffers(grate);

		if (grate_key_pressed(grate))
			break;

		grate_profile_sample(profile);
		angle += 1.0f;
	}

	grate_profile_finish(profile);
	grate_profile_free(profile);

	grate_exit(grate);
	return 0;
}
