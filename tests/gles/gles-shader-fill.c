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

static const GLchar *vertex_shader[] = {
	"attribute vec4 position;\n",
	//"attribute vec2 texcoord;\n",
	//"varying vec2 uv;\n",
	"\n",
	"void main()\n",
	"{\n",
	"    gl_Position = position;\n",
	//"    uv = texcoord;\n",
	"}"
};

static const GLchar *fragment_shader[] = {
	"void main()\n",
	"{\n",
	"    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n",
	"}"
};

void window_draw(struct window *window)
{
	static const GLfloat vertices[] = {
		-1.0f,  1.0f,
		 1.0f,  1.0f,
		-1.0f, -1.0f,
		 1.0f, -1.0f,
	};
	static const GLfloat uv[] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
	};
	static const GLushort indices[] = {
		0, 1, 2,
		2, 3, 1,
	};
	GLint position, texcoord;
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

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE,
			      2 * sizeof(GLfloat), vertices);
	glEnableVertexAttribArray(position);

	glVertexAttribPointer(texcoord, 2, GL_FLOAT, GL_FALSE,
			      2 * sizeof(GLfloat), uv);
	glEnableVertexAttribArray(texcoord);

	glDrawElements(GL_TRIANGLES, ARRAY_SIZE(indices), GL_UNSIGNED_SHORT,
		       indices);

	eglSwapBuffers(window->egl.display, window->egl.surface);
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
	window_draw(window);

	sleep(1);

	window_close(window);
	return 0;
}
