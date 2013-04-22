#include <stdio.h>

#include <X11/Xlib.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

int main(int argc, char *argv[])
{
	EGLint major, minor;
	Display *display;
	EGLDisplay egl;
	GLint value;

	display = XOpenDisplay(NULL);
	if (!display) {
		fprintf(stderr, "failed to open display\n");
		return 1;
	}

	egl = eglGetDisplay(display);
	if (egl == EGL_NO_DISPLAY) {
		fprintf(stderr, "eglGetDisplay() failed\n");
		XCloseDisplay(display);
		return 1;
	}

	if (!eglInitialize(egl, &major, &minor)) {
		fprintf(stderr, "eglInitialize() failed\n");
		eglTerminate(egl);
		XCloseDisplay(display);
		return 1;
	}

	printf("EGL: %d.%d\n", major, minor);

	eglBindAPI(EGL_OPENGL_ES_API);

	glGetIntegerv(GL_DEPTH_BITS, &value);
	printf("GL_DEPTH_BITS: %d\n", value);

	eglTerminate(egl);
	XCloseDisplay(display);
	return 0;
}
