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
	"uniform float z;\n",
	"\n",
	"void main()\n",
	"{\n",
	"    gl_Position = position + vec4(1.0, 0.5, 0.25, z);\n",
	"}"
};

static const GLchar *fragment_shader[] = {
	"precision mediump float;\n",
	"\n",
	"void main()\n",
	"{\n",
	"    gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n",
	"}"
};

void pbuffer_draw(struct pbuffer *pbuffer)
{
	static const GLfloat vertices[] = {
		 0.0f,  0.5f, 0.0f, 1.0f,
		-0.5f, -0.5f, 0.0f, 1.0f,
		 0.5f, -0.5f, 0.0f, 1.0f,
	};
	static const GLushort indices[] = {
		0, 1, 2,
	};
	GLuint vs, fs, program;
	GLint position, z;

	printf("=== compiling shaders\n");
	vs = glsl_shader_load(GL_VERTEX_SHADER, vertex_shader,
			      ARRAY_SIZE(vertex_shader));
	fs = glsl_shader_load(GL_FRAGMENT_SHADER, fragment_shader,
			      ARRAY_SIZE(fragment_shader));
	glFlush();
	printf("=== linking program\n");
	program = glsl_program_create(vs, fs);
	glsl_program_link(program);
	glFlush();
	printf("=== using program\n");
	glUseProgram(program);
	glFlush();

	position = glGetAttribLocation(program, "position");
	z = glGetUniformLocation(program, "z");

	printf("=== clearing buffer\n");
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();

	printf("=== setting vertex attribute\n");
	glVertexAttribPointer(position, 4, GL_FLOAT, GL_FALSE,
			      4 * sizeof(GLfloat), vertices);
	glFlush();
	printf("=== enabling vertex attribute\n");
	glEnableVertexAttribArray(position);
	glFlush();

	glUniform1f(z, 0.125f);

	printf("=== drawing triangles\n");
	glDrawElements(GL_TRIANGLES, ARRAY_SIZE(indices), GL_UNSIGNED_SHORT,
		       indices);
	glFlush();
	printf("=== done\n");
}

int main(int argc, char *argv[])
{
	struct pbuffer *pbuffer;

	pbuffer = pbuffer_create(32, 32);
	if (!pbuffer) {
		fprintf(stderr, "pbuffer_create() failed\n");
		return 1;
	}

	pbuffer_draw(pbuffer);
	pbuffer_save(pbuffer, "test.png");
	pbuffer_free(pbuffer);

	return 0;
}
