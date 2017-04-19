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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

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
	GLenum func = GL_NEVER;
	GLuint mask = 0;
	GLint ref = 0;
	GLenum fail = GL_ZERO;
	GLenum zfail = GL_ZERO;
	GLenum zpass = GL_ZERO;
	GLenum funcface = GL_FRONT_AND_BACK;
	GLenum opface = GL_FRONT_AND_BACK;
	int stencil_enb = 0;
	static const struct option long_opts[] = {
		{ "StencilFunc", 1, NULL, 'u' },
		{ "StencilRef", 1, NULL, 'r' },
		{ "StencilMask", 1, NULL, 'm' },
		{ "StencilOpFail", 1, NULL, 'f' },
		{ "StencilOpZfail", 1, NULL, 'z' },
		{ "StencilOpZpass", 1, NULL, 'p' },
		{ "StencilEnabled", 0, NULL, 's' },
		{ "StencilFuncFace", 1, NULL, 'n' },
		{ "StencilOpFace", 1, NULL, 'o' },
	};
	static const char opts[] = "u:r:m:f:z:p:sn:o:";
	int opt;

	if (chdir( dirname(argv[0]) ) == -1)
		fprintf(stderr, "chdir failed\n");

	if (chdir("../../") == -1)
		fprintf(stderr, "chdir failed\n");

	while ((opt = getopt_long(argc, argv, opts, long_opts, NULL)) != -1) {
		switch (opt) {
		case 'u':
			if (strcmp(optarg, "GL_NEVER") == 0)	{ func = GL_NEVER; break; }
			if (strcmp(optarg, "GL_LESS") == 0)	{ func = GL_LESS; break; }
			if (strcmp(optarg, "GL_LEQUAL") == 0)	{ func = GL_LEQUAL; break; }
			if (strcmp(optarg, "GL_GREATER") == 0)	{ func = GL_GREATER; break; }
			if (strcmp(optarg, "GL_GEQUAL") == 0)	{ func = GL_GEQUAL; break; }
			if (strcmp(optarg, "GL_EQUAL") == 0)	{ func = GL_EQUAL; break; }
			if (strcmp(optarg, "GL_NOTEQUAL") == 0)	{ func = GL_NOTEQUAL; break; }
			if (strcmp(optarg, "GL_ALWAYS") == 0)	{ func = GL_ALWAYS; break; }
			break;

		case 'r':
			ref = atoi(optarg);
			break;

		case 'm':
			mask = strtoul(optarg, NULL, 10);
			break;

		case 'f':
			if (strcmp(optarg, "GL_KEEP") == 0)	{ fail = GL_KEEP; break; }
			if (strcmp(optarg, "GL_ZERO") == 0)	{ fail = GL_ZERO; break; }
			if (strcmp(optarg, "GL_REPLACE") == 0)	{ fail = GL_REPLACE; break; }
			if (strcmp(optarg, "GL_INVERT") == 0)	{ fail = GL_INVERT; break; }
			if (strcmp(optarg, "GL_INCR") == 0)	{ fail = GL_INCR; break; }
			if (strcmp(optarg, "GL_DECR") == 0)	{ fail = GL_DECR; break; }
			if (strcmp(optarg, "GL_INCR_WRAP") == 0){ fail = GL_INCR_WRAP; break; }
			if (strcmp(optarg, "GL_DECR_WRAP") == 0){ fail = GL_DECR_WRAP; break; }
			break;

		case 'z':
			if (strcmp(optarg, "GL_KEEP") == 0)	{ zfail = GL_KEEP; break; }
			if (strcmp(optarg, "GL_ZERO") == 0)	{ zfail = GL_ZERO; break; }
			if (strcmp(optarg, "GL_REPLACE") == 0)	{ zfail = GL_REPLACE; break; }
			if (strcmp(optarg, "GL_INVERT") == 0)	{ zfail = GL_INVERT; break; }
			if (strcmp(optarg, "GL_INCR") == 0)	{ zfail = GL_INCR; break; }
			if (strcmp(optarg, "GL_DECR") == 0)	{ zfail = GL_DECR; break; }
			if (strcmp(optarg, "GL_INCR_WRAP") == 0){ zfail = GL_INCR_WRAP; break; }
			if (strcmp(optarg, "GL_DECR_WRAP") == 0){ zfail = GL_DECR_WRAP; break; }
			break;

		case 'p':
			if (strcmp(optarg, "GL_KEEP") == 0)	{ zpass = GL_KEEP; break; }
			if (strcmp(optarg, "GL_ZERO") == 0)	{ zpass = GL_ZERO; break; }
			if (strcmp(optarg, "GL_REPLACE") == 0)	{ zpass = GL_REPLACE; break; }
			if (strcmp(optarg, "GL_INVERT") == 0)	{ zpass = GL_INVERT; break; }
			if (strcmp(optarg, "GL_INCR") == 0)	{ zpass = GL_INCR; break; }
			if (strcmp(optarg, "GL_DECR") == 0)	{ zpass = GL_DECR; break; }
			if (strcmp(optarg, "GL_INCR_WRAP") == 0){ zpass = GL_INCR_WRAP; break; }
			if (strcmp(optarg, "GL_DECR_WRAP") == 0){ zpass = GL_DECR_WRAP; break; }
			break;

		case 's':
			stencil_enb = 1;
			break;

		case 'n':
			if (strcmp(optarg, "GL_FRONT_AND_BACK") == 0)	{ funcface = GL_FRONT_AND_BACK; break; }
			if (strcmp(optarg, "GL_FRONT") == 0)	{ funcface = GL_FRONT; break; }
			if (strcmp(optarg, "GL_BACK") == 0)	{ funcface = GL_BACK; break; }
			break;

		case 'o':
			if (strcmp(optarg, "GL_FRONT_AND_BACK") == 0)	{ opface = GL_FRONT_AND_BACK; break; }
			if (strcmp(optarg, "GL_FRONT") == 0)	{ opface = GL_FRONT; break; }
			if (strcmp(optarg, "GL_BACK") == 0)	{ opface = GL_BACK; break; }
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

	printf("stencil_enb = %d\n", stencil_enb);

	switch (func) {
	case GL_NEVER: printf("func = GL_NEVER\n"); break;
	case GL_LESS: printf("func = GL_LESS\n"); break;
	case GL_LEQUAL: printf("func = GL_LEQUAL\n"); break;
	case GL_GREATER: printf("func = GL_GREATER\n"); break;
	case GL_GEQUAL: printf("func = GL_GEQUAL\n"); break;
	case GL_EQUAL: printf("func = GL_EQUAL\n"); break;
	case GL_NOTEQUAL: printf("func = GL_NOTEQUAL\n"); break;
	case GL_ALWAYS: printf("func = GL_ALWAYS\n"); break;
	}

	printf("ref %d mask %u\n", ref, mask);

	switch (fail) {
	case GL_KEEP: printf("fail = GL_KEEP\n"); break;
	case GL_ZERO: printf("fail = GL_ZERO\n"); break;
	case GL_REPLACE: printf("fail = GL_REPLACE\n"); break;
	case GL_INVERT: printf("fail = GL_INVERT\n"); break;
	case GL_INCR: printf("fail = GL_INCR\n"); break;
	case GL_DECR: printf("fail = GL_DECR\n"); break;
	case GL_INCR_WRAP: printf("fail = GL_INCR_WRAP\n"); break;
	case GL_DECR_WRAP: printf("fail = GL_DECR_WRAP\n"); break;
	}

	switch (zfail) {
	case GL_KEEP: printf("zfail = GL_KEEP\n"); break;
	case GL_ZERO: printf("zfail = GL_ZERO\n"); break;
	case GL_REPLACE: printf("zfail = GL_REPLACE\n"); break;
	case GL_INVERT: printf("zfail = GL_INVERT\n"); break;
	case GL_INCR: printf("zfail = GL_INCR\n"); break;
	case GL_DECR: printf("zfail = GL_DECR\n"); break;
	case GL_INCR_WRAP: printf("zfail = GL_INCR_WRAP\n"); break;
	case GL_DECR_WRAP: printf("zfail = GL_DECR_WRAP\n"); break;
	}

	switch (zpass) {
	case GL_KEEP: printf("zpass = GL_KEEP\n"); break;
	case GL_ZERO: printf("zpass = GL_ZERO\n"); break;
	case GL_REPLACE: printf("zpass = GL_REPLACE\n"); break;
	case GL_INVERT: printf("zpass = GL_INVERT\n"); break;
	case GL_INCR: printf("zpass = GL_INCR\n"); break;
	case GL_DECR: printf("zpass = GL_DECR\n"); break;
	case GL_INCR_WRAP: printf("zpass = GL_INCR_WRAP\n"); break;
	case GL_DECR_WRAP: printf("zpass = GL_DECR_WRAP\n"); break;
	}

	switch (funcface) {
	case GL_FRONT_AND_BACK: printf("funcface = GL_FRONT_AND_BACK\n"); break;
	case GL_FRONT: printf("funcface = GL_FRONT\n"); break;
	case GL_BACK: printf("funcface = GL_BACK\n"); break;
	}

	switch (opface) {
	case GL_FRONT_AND_BACK: printf("opface = GL_FRONT_AND_BACK\n"); break;
	case GL_FRONT: printf("opface = GL_FRONT\n"); break;
	case GL_BACK: printf("opface = GL_BACK\n"); break;
	}

	window_setup(window);

	if (stencil_enb)
		glEnable(GL_STENCIL_TEST);

	glStencilFuncSeparate(funcface, func, ref, mask);
	glStencilOpSeparate(opface, fail, zfail, zpass);

	window_show(window);
	window_draw(window);
	window_close(window);

	return 0;
}
