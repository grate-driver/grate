/*
 * Copyright (c) 2012, 2013 Erik Faye-Lund
 * Copyright (c) 2013 Avionic Design GmbH
 * Copyright (c) 2013 Thierry Reding
 * Copyright (c) 2017 Dmitry Osipenko
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

#include <getopt.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <GLES2/gl2.h>

#include "common.h"
#include "matrix.h"

static const GLchar *vertex_shader[] = {
	"attribute vec4 position;\n",
	"attribute vec2 texcoord;\n",
	"varying vec2 vtexcoord;\n",
	"uniform mat4 mvp;\n",
	"\n",
	"void main()\n",
	"{\n",
	"  gl_Position = position * mvp;\n",
	"  vtexcoord = texcoord;\n",
	"}\n"
};

static const GLchar *fragment_shader[] = {
	"precision mediump float;\n",
	"varying vec2 vtexcoord;\n",
	"uniform sampler2D tex;\n",
	"\n",
	"void main()\n",
	"{\n",
	"  gl_FragColor = texture2D(tex, vtexcoord);\n",
	"}\n"
};

static const GLfloat vertices[] = {
	-0.5f, -0.5f,  0.0f, 1.0f,
	 0.5f, -0.5f,  0.0f, 1.0f,
	 0.5f,  0.5f,  0.0f, 1.0f,
	-0.5f,  0.5f,  0.0f, 1.0f,
};

static const GLfloat uv[] = {
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f,
};

static const GLushort indices[] = {
	 0,  1,  2,
	 0,  2,  3,
};

struct gles_texture *texture;
char *img_path = "data/tegra.png";
unsigned img_width = 300;
unsigned img_height = 300;
static GLfloat x = 0.0f;
static GLfloat y = 0.0f;
static GLfloat z = 0.0f;
static GLint mvp;

static void window_setup(struct window *window)
{
	GLint position, texcoord, tex;
	GLuint vs, fs, program;

	vs = glsl_shader_load(GL_VERTEX_SHADER, vertex_shader,
			      ARRAY_SIZE(vertex_shader));
	fs = glsl_shader_load(GL_FRAGMENT_SHADER, fragment_shader,
			      ARRAY_SIZE(fragment_shader));
	program = glsl_program_create(vs, fs);
	glsl_program_link(program);
	glUseProgram(program);

	position = glGetAttribLocation(program, "position");
	texcoord = glGetAttribLocation(program, "texcoord");
	tex = glGetUniformLocation(program, "tex");
	mvp = glGetUniformLocation(program, "mvp");

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glVertexAttribPointer(position, 4, GL_FLOAT, GL_FALSE,
			      4 * sizeof(GLfloat), vertices);
	glEnableVertexAttribArray(position);

	glVertexAttribPointer(texcoord, 2, GL_FLOAT, GL_FALSE,
			      2 * sizeof(GLfloat), uv);
	glEnableVertexAttribArray(texcoord);

	texture = gles_texture_load2(img_path, img_width, img_height);
	if (!texture)
		return;

	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, texture->id);
	glUniform1i(tex, 0);
}

static void window_draw(struct window *window)
{
	struct mat4 matrix, modelview, projection, transform, result;
	GLfloat aspect;

	aspect = window->width / (GLfloat)window->height;

	mat4_perspective(&projection, 35.0f, aspect, 1.0f, 1024.0f);
	mat4_identity(&modelview);

	mat4_rotate_x(&transform, x);
	mat4_multiply(&result, &modelview, &transform);
	mat4_rotate_y(&transform, y);
	mat4_multiply(&modelview, &result, &transform);
	mat4_rotate_z(&transform, z);
	mat4_multiply(&result, &modelview, &transform);
	mat4_translate(&transform, 0.0f, 0.0f, -2.0f);
	mat4_multiply(&modelview, &transform, &result);

	mat4_multiply(&matrix, &projection, &modelview);
	glUniformMatrix4fv(mvp, 1, GL_FALSE, (GLfloat *)&matrix);

	glViewport(0, 0, window->width, window->height);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDrawElements(GL_TRIANGLES, ARRAY_SIZE(indices), GL_UNSIGNED_SHORT,
		       indices);
	glFlush();

	eglSwapBuffers(window->egl.display, window->egl.surface);

	x += 0.3f;
	y += 0.2f;
	z += 0.4f;
}

int main(int argc, char *argv[])
{
	struct window *window;
	GLuint minf = GL_NEAREST;
	GLuint magf = GL_NEAREST;
	GLuint wrap_s = GL_CLAMP_TO_EDGE;
	GLuint wrap_t = GL_CLAMP_TO_EDGE;
	static const struct option long_opts[] = {
		{ "min", 1, NULL, 'n' },
		{ "mag", 1, NULL, 'g' },
		{ "img", 1, NULL, 'i' },
		{ "width", 1, NULL, 'w' },
		{ "height", 1, NULL, 'h' },
		{ "wraps", 1, NULL, 's' },
		{ "wrapt", 1, NULL, 't' },
	};
	static const char opts[] = "n:g:i:w:h:s:t:";
	int opt;

	if (chdir( dirname(argv[0]) ) == -1)
		fprintf(stderr, "chdir failed\n");

	if (chdir("../../") == -1)
		fprintf(stderr, "chdir failed\n");

	while ((opt = getopt_long(argc, argv, opts, long_opts, NULL)) != -1) {
		switch (opt) {
		case 'n':
			if (strcmp(optarg, "GL_NEAREST_MIPMAP_NEAREST") == 0) {
				minf = GL_NEAREST_MIPMAP_NEAREST;
				break;
			}

			if (strcmp(optarg, "GL_LINEAR_MIPMAP_NEAREST") == 0) {
				minf = GL_LINEAR_MIPMAP_NEAREST;
				break;
			}

			if (strcmp(optarg, "GL_NEAREST_MIPMAP_LINEAR") == 0) {
				minf = GL_NEAREST_MIPMAP_LINEAR;
				break;
			}

			if (strcmp(optarg, "GL_LINEAR_MIPMAP_LINEAR") == 0) {
				minf = GL_LINEAR_MIPMAP_LINEAR;
				break;
			}

			if (strcmp(optarg, "GL_NEAREST") == 0) {
				minf = GL_NEAREST;
				break;
			}

			if (strcmp(optarg, "GL_LINEAR") == 0) {
				minf = GL_LINEAR;
				break;
			}
			break;

		case 'g':
			if (strcmp(optarg, "GL_NEAREST") == 0) {
				magf = GL_NEAREST;
				break;
			}

			if (strcmp(optarg, "GL_LINEAR") == 0) {
				magf = GL_LINEAR;
				break;
			}
			break;

		case 'i':
			img_path = strdup(optarg);
			break;

		case 'w':
			img_width = strtoul(optarg, NULL, 10);
			break;

		case 'h':
			img_height = strtoul(optarg, NULL, 10);
			break;

		case 's':
			if (strcmp(optarg, "GL_CLAMP_TO_EDGE") == 0) {
				wrap_s = GL_CLAMP_TO_EDGE;
				break;
			}

			if (strcmp(optarg, "GL_MIRRORED_REPEAT") == 0) {
				wrap_s = GL_MIRRORED_REPEAT;
				break;
			}

			if (strcmp(optarg, "GL_REPEAT") == 0) {
				wrap_s = GL_REPEAT;
				break;
			}
			break;

		case 't':
			if (strcmp(optarg, "GL_CLAMP_TO_EDGE") == 0) {
				wrap_t = GL_CLAMP_TO_EDGE;
				break;
			}

			if (strcmp(optarg, "GL_MIRRORED_REPEAT") == 0) {
				wrap_t = GL_MIRRORED_REPEAT;
				break;
			}

			if (strcmp(optarg, "GL_REPEAT") == 0) {
				wrap_t = GL_REPEAT;
				break;
			}
			break;

		default:
			fprintf(stderr, "Invalid cmdline argument\n");
			return 1;
		}
	}

	window = window_create(0, 0, 640, 480);
	if (!window) {
		fprintf(stderr, "window_create() failed\n");
		return 1;
	}

	window_setup(window);

	printf("img = %s width = %u height = %u\n",
	       img_path, img_width, img_height);

	switch (minf) {
	case GL_NEAREST:
		printf("min filter = GL_LINEAR\n");
		break;
	case GL_LINEAR:
		printf("min filter = GL_LINEAR\n");
		break;
	case GL_NEAREST_MIPMAP_NEAREST:
		printf("min filter = GL_NEAREST_MIPMAP_NEAREST\n");
		break;
	case GL_LINEAR_MIPMAP_NEAREST:
		printf("min filter = GL_LINEAR_MIPMAP_NEAREST\n");
		break;
	case GL_NEAREST_MIPMAP_LINEAR:
		printf("min filter = GL_NEAREST_MIPMAP_LINEAR\n");
		break;
	case GL_LINEAR_MIPMAP_LINEAR:
		printf("min filter = GL_LINEAR_MIPMAP_LINEAR\n");
		break;
	default:
		return -1;
	}

	switch (magf) {
	case GL_NEAREST:
		printf("mag filter = GL_NEAREST\n");
		break;
	case GL_LINEAR:
		printf("mag filter = GL_LINEAR\n");
		break;
	default:
		return -1;
	}

	switch (wrap_s) {
	case GL_CLAMP_TO_EDGE:
		printf("wrap_s = GL_CLAMP_TO_EDGE\n");
		break;
	case GL_MIRRORED_REPEAT:
		printf("wrap_s = GL_MIRRORED_REPEAT\n");
		break;
	case GL_REPEAT:
		printf("wrap_s = GL_REPEAT\n");
		break;
	default:
		return -1;
	}

	switch (wrap_t) {
	case GL_CLAMP_TO_EDGE:
		printf("wrap_t = GL_CLAMP_TO_EDGE\n");
		break;
	case GL_MIRRORED_REPEAT:
		printf("wrap_t = GL_MIRRORED_REPEAT\n");
		break;
	case GL_REPEAT:
		printf("wrap_t = GL_REPEAT\n");
		break;
	default:
		return -1;
	}

	switch (minf) {
	case GL_NEAREST_MIPMAP_NEAREST:
	case GL_LINEAR_MIPMAP_NEAREST:
	case GL_NEAREST_MIPMAP_LINEAR:
	case GL_LINEAR_MIPMAP_LINEAR:
		glGenerateMipmap(GL_TEXTURE_2D);
		break;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minf);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magf);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
	glFlush();

	window_show(window);

	while (true) {
		if (!window_event_loop(window))
			break;

		window_draw(window);
	}

	window_close(window);

	return 0;
}
