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
	/* front */
	-0.5f, -0.5f,  0.5f, 1.0f,
	 0.5f, -0.5f,  0.5f, 1.0f,
	 0.5f,  0.5f,  0.5f, 1.0f,
	-0.5f,  0.5f,  0.5f, 1.0f,
	/* back */
	-0.5f, -0.5f, -0.5f, 1.0f,
	 0.5f, -0.5f, -0.5f, 1.0f,
	 0.5f,  0.5f, -0.5f, 1.0f,
	-0.5f,  0.5f, -0.5f, 1.0f,
	/* left */
	-0.5f, -0.5f,  0.5f, 1.0f,
	-0.5f,  0.5f,  0.5f, 1.0f,
	-0.5f,  0.5f, -0.5f, 1.0f,
	-0.5f, -0.5f, -0.5f, 1.0f,
	/* right */
	 0.5f, -0.5f,  0.5f, 1.0f,
	 0.5f,  0.5f,  0.5f, 1.0f,
	 0.5f,  0.5f, -0.5f, 1.0f,
	 0.5f, -0.5f, -0.5f, 1.0f,
	/* top */
	-0.5f,  0.5f,  0.5f, 1.0f,
	 0.5f,  0.5f,  0.5f, 1.0f,
	 0.5f,  0.5f, -0.5f, 1.0f,
	-0.5f,  0.5f, -0.5f, 1.0f,
	/* bottom */
	-0.5f, -0.5f,  0.5f, 1.0f,
	 0.5f, -0.5f,  0.5f, 1.0f,
	 0.5f, -0.5f, -0.5f, 1.0f,
	-0.5f, -0.5f, -0.5f, 1.0f,
};

static const GLfloat uv[] = {
	/* front */
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f,
	/* back */
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f,
	/* left */
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f,
	/* right */
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f,
	/* top */
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f,
	/* bottom */
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f,
};

static const GLushort indices[] = {
	/* front */
	 0,  1,  2,
	 0,  2,  3,
	/* back */
	 4,  5,  6,
	 4,  6,  7,
	/* left */
	 8,  9, 10,
	 8, 10, 11,
	/* right */
	12, 13, 14,
	12, 14, 15,
	/* top */
	16, 17, 18,
	16, 18, 19,
	/* bottom */
	20, 21, 22,
	20, 22, 23,
};

struct gles_texture *texture;
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

	texture = gles_texture_load("data/tegra.png");
	if (!texture)
		return;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture->id);
	glActiveTexture(GL_TEXTURE0 + 0);
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

	eglSwapBuffers(window->egl.display, window->egl.surface);

	x += 0.3f;
	y += 0.2f;
	z += 0.4f;
}

int main(int argc, char *argv[])
{
	struct window *window;

	window = window_create(0, 0, 640, 480);
	if (!window) {
		fprintf(stderr, "window_create() failed\n");
		return 1;
	}

	window_setup(window);
	window_show(window);

	while (true) {
		if (!window_event_loop(window))
			break;

		window_draw(window);
	}

	window_close(window);

	return 0;
}
