#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GLES2/gl2.h>

#include "common.h"

struct window *window_create(unsigned int x, unsigned int y,
			     unsigned int width, unsigned int height)
{
	static const EGLint attribs[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
	static const EGLint attrs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	XSetWindowAttributes swa;
	struct window *window;
	EGLint major, minor;
	unsigned long mask;
	EGLint num_configs;
	EGLConfig config;
	EGLint version;
	Window root;

	window = calloc(1, sizeof(*window));
	if (!window)
		return NULL;

	window->width = width;
	window->height = height;

	window->x.display = XOpenDisplay(NULL);
	if (!window->x.display) {
		free(window);
		return NULL;
	}

	root = DefaultRootWindow(window->x.display);

	memset(&swa, 0, sizeof(swa));
	swa.event_mask = StructureNotifyMask | ExposureMask |
			 VisibilityChangeMask;
	mask = CWEventMask;

	window->x.window = XCreateWindow(window->x.display, root, 0, 0, width,
					 height, 0, CopyFromParent, InputOutput,
					 CopyFromParent, mask, &swa);
	if (!window->x.window) {
		XCloseDisplay(window->x.display);
		free(window);
		return NULL;
	}

	XMapWindow(window->x.display, window->x.window);

	window->egl.display = eglGetDisplay(window->x.display);
	if (window->egl.display == EGL_NO_DISPLAY) {
		XCloseDisplay(window->x.display);
		free(window);
		return NULL;
	}

	if (!eglInitialize(window->egl.display, &major, &minor)) {
		XCloseDisplay(window->x.display);
		free(window);
		return NULL;
	}

	printf("EGL: %d.%d\n", major, minor);

	if (!eglChooseConfig(window->egl.display, attribs, &config, 1,
			     &num_configs)) {
		XCloseDisplay(window->x.display);
		free(window);
		return NULL;
	}

	eglBindAPI(EGL_OPENGL_ES_API);

	window->egl.surface = eglCreateWindowSurface(window->egl.display,
						     config, window->x.window,
						     NULL);
	if (window->egl.surface == EGL_NO_SURFACE) {
		XCloseDisplay(window->x.display);
		free(window);
		return NULL;
	}

	window->egl.context = eglCreateContext(window->egl.display, config,
					       EGL_NO_CONTEXT, attrs);
	if (window->egl.context == EGL_NO_CONTEXT) {
		XCloseDisplay(window->x.display);
		free(window);
		return NULL;
	}

	eglQueryContext(window->egl.display, window->egl.context, EGL_CONTEXT_CLIENT_VERSION, &version);
	printf("OpenGL ES: %d\n", version);

	if (!eglMakeCurrent(window->egl.display, window->egl.surface,
			    window->egl.surface, window->egl.context)) {
		XCloseDisplay(window->x.display);
		free(window);
		return NULL;
	}

	return window;
}

void window_close(struct window *window)
{
	eglDestroySurface(window->egl.display, window->egl.surface);
	eglDestroyContext(window->egl.display, window->egl.context);
	XDestroyWindow(window->x.display, window->x.window);

	eglTerminate(window->egl.display);
	XCloseDisplay(window->x.display);
	free(window);
}

void window_show(struct window *window)
{
	int done = False;

	while (!done) {
		XEvent event;

		XNextEvent(window->x.display, &event);

		switch (event.type) {
		case ConfigureNotify:
			window->width = event.xconfigure.width;
			window->height = event.xconfigure.height;
			break;

		case Expose:
			done = True;
			break;

		default:
			break;
		}
	}

	glViewport(0, 0, window->width, window->height);
}

void window_event_loop(struct window *window)
{
	while (True) {
		XEvent event;

		XNextEvent(window->x.display, &event);

		switch (event.type) {
		case Expose:
			break;

		case ConfigureNotify:
			window->width = event.xconfigure.width;
			window->height = event.xconfigure.height;
			break;

		case KeyPress:
			break;

		default:
			break;
		}
	}
}
