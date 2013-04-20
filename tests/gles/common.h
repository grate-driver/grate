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

#ifndef GRATE_TESTS_3D_COMMON_H
#define GRATE_TESTS_3D_COMMON_H 1

#include <stdbool.h>

#include <X11/Xlib.h>
#include <EGL/egl.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

struct gles_options {
	unsigned int width;
	unsigned int height;
};

int gles_parse_command_line(struct gles_options *options, int argc,
			    char *argv[]);

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
bool window_event_loop(struct window *window);

struct pbuffer {
	EGLDisplay display;
	EGLContext context;
	EGLSurface surface;

	unsigned int width;
	unsigned int height;
};

struct pbuffer *pbuffer_create(unsigned int width, unsigned int height);
void pbuffer_free(struct pbuffer *pbuffer);
bool pbuffer_save(struct pbuffer *pbuffer, const char *filename);

GLuint glsl_shader_load(GLenum type, const GLchar *lines[], size_t count);
GLuint glsl_program_create(GLuint vertex, GLuint fragment);
void glsl_program_link(GLuint program);

struct framebuffer {
	unsigned int width;
	unsigned int height;
	GLuint texture;
	GLenum format;
	GLuint id;
};

struct framebuffer *framebuffer_create(unsigned int width, unsigned int height,
				       GLenum format);
struct framebuffer *display_create(unsigned int width, unsigned int height);
void framebuffer_free(struct framebuffer *framebuffer);
void framebuffer_bind(struct framebuffer *framebuffer);
bool framebuffer_save(struct framebuffer *framebuffer, const char *filename);

#endif
