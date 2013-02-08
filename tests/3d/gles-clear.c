#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <GLES2/gl2.h>

#include "common.h"

void window_draw(struct window *window)
{
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

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
