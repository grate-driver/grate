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
