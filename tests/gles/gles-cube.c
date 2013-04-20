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
	"attribute vec4 color;\n",
	"uniform mat4 mvp;\n",
	"\n",
	"varying vec4 vcolor;\n",
	"\n",
	"void main()\n",
	"{\n",
	"  gl_Position = position * mvp;\n",
	"  vcolor = color;\n",
	"}\n"
};

static const GLchar *fragment_shader[] = {
	"precision mediump float;\n",
	"varying vec4 vcolor;\n",
	"\n",
	"void main()\n",
	"{\n",
	"  gl_FragColor = vcolor;\n",
	"}\n"
};

static void window_draw(struct window *window)
{
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

	static const GLfloat colors[] = {
		/* front */
		1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f,
		/* back */
		1.0f, 1.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 0.0f, 1.0f,
		/* left */
		0.0f, 0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f,
		/* right */
		1.0f, 0.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 1.0f, 1.0f,
		/* top */
		0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		/* bottom */
		1.0f, 0.5f, 0.0f, 1.0f,
		1.0f, 0.5f, 0.0f, 1.0f,
		1.0f, 0.5f, 0.0f, 1.0f,
		1.0f, 0.5f, 0.0f, 1.0f,
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

	struct mat4 matrix, modelview, projection, transform, result;
	static GLfloat x = 0.0f, y = 0.0f, z = 0.0f;
	GLint position, color, mvp;
	GLuint vs, fs, program;
	GLfloat aspect;

	vs = glsl_shader_load(GL_VERTEX_SHADER, vertex_shader,
			      ARRAY_SIZE(vertex_shader));
	fs = glsl_shader_load(GL_FRAGMENT_SHADER, fragment_shader,
			      ARRAY_SIZE(fragment_shader));
	program = glsl_program_create(vs, fs);
	glsl_program_link(program);
	glUseProgram(program);

	position = glGetAttribLocation(program, "position");
	color = glGetAttribLocation(program, "color");
	mvp = glGetUniformLocation(program, "mvp");

	aspect = window->width / (GLfloat)window->height;

	mat4_perspective(&projection, 60.0f, aspect, 1.0f, 1024.0f);
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

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glVertexAttribPointer(position, 4, GL_FLOAT, GL_FALSE,
			      4 * sizeof(GLfloat), vertices);
	glEnableVertexAttribArray(position);

	glVertexAttribPointer(color, 4, GL_FLOAT, GL_FALSE,
			      4 * sizeof(GLfloat), colors);
	glEnableVertexAttribArray(color);

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

	window_show(window);

	while (true) {
		if (!window_event_loop(window))
			break;

		window_draw(window);
	}

	window_close(window);

	return 0;
}
