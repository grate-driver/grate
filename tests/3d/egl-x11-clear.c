#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

struct display {
	Display *x11;
	EGLDisplay egl;
};

static struct display *display_open(void)
{
	struct display *display;
	EGLint major, minor;

	display = calloc(1, sizeof(*display));
	if (!display)
		return NULL;

	display->x11 = XOpenDisplay(NULL);
	if (!display->x11) {
		free(display);
		return NULL;
	}

	display->egl = eglGetDisplay(display->x11);
	if (display->egl == EGL_NO_DISPLAY) {
		XCloseDisplay(display->x11);
		free(display);
		return NULL;
	}

	if (!eglInitialize(display->egl, &major, &minor)) {
		XCloseDisplay(display->x11);
		free(display);
		return NULL;
	}

	printf("EGL: %d.%d\n", major, minor);

	return display;
}

static void display_close(struct display *display)
{
	if (!display)
		return;

	eglTerminate(display->egl);
	XCloseDisplay(display->x11);
	free(display);
}

struct window {
	struct display *display;
	Window x11;
	EGLContext context;
	EGLSurface surface;
	unsigned int width;
	unsigned int height;
};

static struct window *window_create(struct display *display, const char *name,
				    unsigned int x, unsigned int y,
				    unsigned int width, unsigned int height)
{
	static const EGLint attribs[] = {
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_DEPTH_SIZE, 1,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
	static const EGLint attrs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	XSetWindowAttributes attr;
	struct window *window;
	unsigned long mask;
	XVisualInfo visual;
	EGLint num_configs;
	XVisualInfo *info;
	XSizeHints hints;
	EGLConfig config;
	int num_visuals;
	EGLint version;
	Window root;
	int screen;
	EGLint vid;

	window = calloc(1, sizeof(*window));
	if (!window)
		return NULL;

	window->display = display;

	screen = DefaultScreen(display->x11);
	root = RootWindow(display->x11, screen);

	if (!eglChooseConfig(display->egl, attribs, &config, 1, &num_configs)) {
		free(window);
		return NULL;
	}

	if (!eglGetConfigAttrib(display->egl, config, EGL_NATIVE_VISUAL_ID, &vid)) {
		free(window);
		return NULL;
	}

	visual.visualid = vid;

	info = XGetVisualInfo(display->x11, VisualIDMask, &visual, &num_visuals);
	if (!info) {
		free(window);
		return NULL;
	}

	memset(&attr, 0, sizeof(attr));
	attr.background_pixel = 0;
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap(display->x11, root, info->visual, AllocNone);
	attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
	mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

	window->x11 = XCreateWindow(display->x11, root, 0, 0, width, height,
				    0, info->depth, InputOutput, info->visual,
				    mask, &attr);
	if (!window->x11) {
		free(window);
		return NULL;
	}

	memset(&hints, 0, sizeof(hints));
	hints.x = x;
	hints.y = y;
	hints.width = width;
	hints.height = height;
	hints.flags = USSize | USPosition;

	XSetNormalHints(display->x11, window->x11, &hints);
	XSetStandardProperties(display->x11, window->x11, name, name, None,
			       NULL, 0, &hints);

	eglBindAPI(EGL_OPENGL_ES_API);

	window->context = eglCreateContext(display->egl, config,
					   EGL_NO_CONTEXT, attrs);
	if (window->context == EGL_NO_CONTEXT) {
		free(window);
		return NULL;
	}

	eglQueryContext(display->egl, window->context, EGL_CONTEXT_CLIENT_VERSION, &version);
	printf("OpenGL ES: %d\n", version);

	window->surface = eglCreateWindowSurface(display->egl, config,
						 window->x11, NULL);
	if (window->surface == EGL_NO_SURFACE) {
		free(window);
		return NULL;
	}

	XFree(info);

	window->width = width;
	window->height = height;

	return window;
}

static void window_close(struct window *window)
{
	if (!window)
		return;

	eglDestroySurface(window->display->egl, window->surface);
	eglDestroyContext(window->display->egl, window->context);
	XDestroyWindow(window->display->x11, window->x11);

	free(window);
}

static void window_show(struct window *window)
{
	XMapWindow(window->display->x11, window->x11);

	if (!eglMakeCurrent(window->display->egl, window->surface, window->surface, window->context))
		fprintf(stderr, "eglMakeCurrent():\n");

	XFlush(window->display->x11);
}

static void draw(struct window *window)
{
	printf("=== calling glViewport()\n");
	glViewport(0, 0, window->width, window->height);
	glFlush();
	printf("=== calling glClearColor()\n");
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glFlush();
	printf("=== calling glClear()\n");
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();
	printf("=== calling eglSwapBuffers()\n");
	eglSwapBuffers(window->display->egl, window->surface);
	glFlush();
	printf("=== done\n");
}

static void event_loop(struct window *window)
{
	while (1) {
		bool redraw = false;
		char buffer[16];
		XEvent event;

		XNextEvent(window->display->x11, &event);

		switch (event.type) {
		case Expose:
			redraw = true;
			break;

		case ConfigureNotify:
			glViewport(0, 0, (GLint)event.xconfigure.width, (GLint)event.xconfigure.height);
			break;

		case KeyPress:
			XLookupString(&event.xkey, buffer, sizeof(buffer), NULL, NULL);
			if (buffer[0] == 27)
				return;
			break;

		default:
			break;
		}

		if (redraw)
			draw(window);
	}
}

int main(int argc, char *argv[])
{
	const unsigned int width = 640;
	const unsigned int height = 480;
	struct display *display;
	struct window *window;

	display = display_open();
	if (!display) {
		fprintf(stderr, "failed to open display\n");
		return 1;
	}

	window = window_create(display, argv[0], 0, 0, width, height);
	if (!window) {
		fprintf(stderr, "failed to create window\n");
		return 1;
	}

	window_show(window);

	event_loop(window);

	window_close(window);
	display_close(display);
	return 0;
}
