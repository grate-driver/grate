#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <GLES2/gl2.h>

#include "common.h"

#define USE_PBUFFER

#ifndef USE_PBUFFER
void window_draw(struct window *window)
{
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	eglSwapBuffers(window->egl.display, window->egl.surface);
}
#else
void pbuffer_draw(struct pbuffer *pbuffer)
{
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	eglSwapBuffers(window->egl.display, window->egl.surface);
}
#endif

int main(int argc, char *argv[])
{
#ifdef USE_PBUFFER
	struct framebuffer *framebuffer;
	struct window *window;

	window = window_create(0, 0, 640, 480);
	if (!window) {
		fprintf(stderr, "window_create() failed\n");
		return 1;
	}

	window_show(window);
	window_draw(window);

	framebuffer = framebuffer_create(640, 480, GL_RGBA);
	if (!framebuffer) {
		fprintf(stderr, "framebuffer_create() failed\n");
		window_close(window);
		return 1;
	}

	framebuffer_bind(framebuffer);
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	framebuffer_save(framebuffer, "test.png");
	framebuffer_free(framebuffer);

	sleep(1);

	window_close(window);
	return 0;
}
