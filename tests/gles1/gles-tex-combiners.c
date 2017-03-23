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

#include <GLES/gl.h>

#include "common.h"

static const GLfloat vertices[] = {
	-0.5f, -0.5f,  0.0f, 1.0f,
	 0.5f, -0.5f,  0.0f, 1.0f,
	 0.0f,  0.5f,  0.0f, 1.0f,
};
/*
static const GLfloat texcoords[3][6] = {
	{
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
	}, {
		0.0f, 0.0f,
		2.0f, 0.0f,
		0.0f, 2.0f,
	},  {
		0.0f, 0.0f,
		3.0f, 0.0f,
		0.0f, 3.0f,
	}
};*/

static void window_setup(struct window *window)
{
/*	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL); */
	glDisable(GL_CULL_FACE);

	glVertexPointer(4, GL_FLOAT, 4 * sizeof(GLfloat), vertices);
	glEnableClientState(GL_VERTEX_ARRAY);

/*	glTexCoordPointer(2, GL_FLOAT, 2 * sizeof(GLfloat), texcoords[0]);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY); */
}

static void window_draw(struct window *window)
{
	glViewport(0, 0, window->width, window->height);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	eglSwapBuffers(window->egl.display, window->egl.surface);
}

int main(int argc, char *argv[])
{
	struct window *window;

	window = window_create(0, 0, 32, 32);
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
		break;
	}

	window_close(window);

	return 0;
}
