#include <stdio.h>

#include <GLES2/gl2.h>
#include <EGL/egl.h>

int main(int argc, char *argv[])
{
	EGLint major, minor;
	EGLDisplay display;
	EGLContext context;
	EGLSurface surface;
	EGLint num_config;
	EGLConfig config;
	GLint height;
	GLint width;

	static EGLint const config_attrs[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_DEPTH_SIZE, 8,
		EGL_NONE
	};

	static EGLint const context_attrs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	static EGLint const pbuffer_attrs[] = {
		EGL_WIDTH, 640,
		EGL_HEIGHT, 480,
		EGL_LARGEST_PBUFFER, EGL_TRUE,
		EGL_NONE
	};

	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (display == EGL_NO_DISPLAY) {
		fprintf(stderr, "no display found\n");
		return 1;
	}

	if (!eglInitialize(display, &major, &minor)) {
		fprintf(stderr, "failed to initialize EGL\n");
		return 1;
	}

	printf("EGL: %d.%d\n", major, minor);

	if (!eglChooseConfig(display, config_attrs, &config, 1, &num_config)) {
		fprintf(stderr, "failed to choose configuration\n");
		return 1;
	}

	printf("config:%p num_config:%d\n", config, num_config);

	context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attrs);
	if (context == EGL_NO_CONTEXT) {
		fprintf(stderr, "failed to create context\n");
		return 1;
	}

	surface = eglCreatePbufferSurface(display, config, pbuffer_attrs);
	if (surface == EGL_NO_SURFACE) {
		fprintf(stderr, "failed to create surface\n");
		return 1;
	}

	if (!eglQuerySurface(display, surface, EGL_WIDTH, &width)) {
		fprintf(stderr, "failed to query surface width\n");
		return 1;
	}

	if (!eglQuerySurface(display, surface, EGL_HEIGHT, &height)) {
		fprintf(stderr, "failed to query surface height\n");
		return 1;
	}

	printf("PBuffer: %dx%d\n", width, height);

	if (!eglMakeCurrent(display, surface, surface, context)) {
		fprintf(stderr, "failed to activate context\n");
		return 1;
	}

	printf("=== Calling glClearColor()\n");
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glFlush();
	printf("=== Calling glClear()\n");
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();

	printf("=== Calling eglSwapBuffers()\n");
	eglSwapBuffers(display, surface);
	eglDestroySurface(display, surface);
	eglTerminate(display);

	return 0;
}
