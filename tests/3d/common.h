#ifndef GRATE_TESTS_3D_COMMON_H
#define GRATE_TESTS_3D_COMMON_H 1

#include <X11/Xlib.h>
#include <EGL/egl.h>

struct window {
	struct {
		Display *display;
		Window window;
	} x;

	struct {
		EGLDisplay display;
		EGLContext context;
		EGLSurface surface;
	} egl;

	unsigned int width;
	unsigned int height;
};

struct window *window_create(unsigned int x, unsigned int y,
			     unsigned int width, unsigned int height);
void window_close(struct window *window);
void window_show(struct window *window);
void window_event_loop(struct window *window);

#endif
