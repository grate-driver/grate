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

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GLES2/gl2.h>
#include <png.h>

#include "common.h"

#define PNG_COLOR_TYPE_INVALID 0xff

png_byte png_format(GLenum format)
{
	switch (format) {
	case GL_RGB:
		return PNG_COLOR_TYPE_RGB;
	
	case GL_RGBA:
		return PNG_COLOR_TYPE_RGBA;
	}

	return PNG_COLOR_TYPE_INVALID;
}

#define PNG_DEPTH_INVALID 0xff

png_byte png_depth(png_byte format)
{
	switch (format) {
	case PNG_COLOR_TYPE_RGB:
		return 24;

	case PNG_COLOR_TYPE_RGBA:
		return 32;
	}

	return PNG_DEPTH_INVALID;
}

int gles_parse_command_line(struct gles_options *opts, int argc, char *argv[])
{
	static const struct option options[] = {
		{ "size", 1, NULL, 's' },
		{ NULL, 0, NULL, 0 }
	};
	int opt, num;

	while ((opt = getopt_long(argc, argv, "s:", options, NULL)) != -1) {
		switch (opt) {
		case 's':
			num = sscanf(optarg, "%ux%u", &opts->width,
				     &opts->height);
			if (num != 2) {
				fprintf(stderr, "invalid size: %s\n", optarg);
				return -EINVAL;
			}

			break;
		}
	}

	return optind;
}

struct window *window_create(unsigned int x, unsigned int y,
			     unsigned int width, unsigned int height)
{
	static const EGLint attribs[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_DEPTH_SIZE, 8,
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
			 KeyPressMask | VisibilityChangeMask;
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

bool window_event_loop(struct window *window)
{
	if (XPending(window->x.display)) {
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
			return false;

		default:
			break;
		}
	}

	return true;
}

struct pbuffer *pbuffer_create(unsigned int width, unsigned int height)
{
	const EGLint config_attribs[] = {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
		EGL_DEPTH_SIZE, 8,
		EGL_NONE
	};
	const EGLint surface_attribs[] = {
		EGL_WIDTH, width,
		EGL_HEIGHT, height,
		EGL_NONE
	};
	const EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	struct pbuffer *pbuffer;
	EGLint major, minor;
	EGLint num_configs;
	EGLConfig config;
	EGLint version;

	pbuffer = calloc(1, sizeof(*pbuffer));
	if (!pbuffer)
		return NULL;

	pbuffer->width = width;
	pbuffer->height = height;

	pbuffer->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (pbuffer->display == EGL_NO_DISPLAY) {
		free(pbuffer);
		return NULL;
	}

	if (!eglInitialize(pbuffer->display, &major, &minor)) {
		free(pbuffer);
		return NULL;
	}

	printf("EGL: %d.%d\n", major, minor);

	if (!eglChooseConfig(pbuffer->display, config_attribs, &config, 1,
			     &num_configs)) {
		free(pbuffer);
		return NULL;
	}

	pbuffer->surface = eglCreatePbufferSurface(pbuffer->display, config,
						   surface_attribs);
	if (pbuffer->surface == EGL_NO_SURFACE) {
		free(pbuffer);
		return NULL;
	}

	pbuffer->context = eglCreateContext(pbuffer->display, config, NULL,
					    context_attribs);
	if (pbuffer->context == EGL_NO_CONTEXT) {
		free(pbuffer);
		return NULL;
	}

	eglQueryContext(pbuffer->display, pbuffer->context,
			EGL_CONTEXT_CLIENT_VERSION, &version);
	printf("OpenGL ES: %d\n", version);

	if (!eglMakeCurrent(pbuffer->display, pbuffer->surface,
			    pbuffer->surface, pbuffer->context)) {
		free(pbuffer);
		return NULL;
	}

	return pbuffer;
}

void pbuffer_free(struct pbuffer *pbuffer)
{
	eglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_NO_SURFACE,
		       EGL_NO_CONTEXT);
	eglDestroyContext(pbuffer->display, pbuffer->context);
	eglDestroySurface(pbuffer->display, pbuffer->surface);
	eglTerminate(pbuffer->display);
	free(pbuffer);
}

bool pbuffer_save(struct pbuffer *pbuffer, const char *filename)
{
	size_t stride, size;
	png_byte format;
	png_structp png;
	png_bytep *rows;
	png_byte depth;
	png_infop info;
	unsigned int i;
	void *data;
	FILE *fp;

	fp = fopen(filename, "wb");
	if (!fp) {
		fprintf(stderr, "failed to write `%s'\n", filename);
		return false;
	}

	png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png) {
		return false;
	}

	info = png_create_info_struct(png);
	if (!info) {
		return false;
	}

	if (setjmp(png_jmpbuf(png))) {
		return false;
	}

	png_init_io(png, fp);

	if (setjmp(png_jmpbuf(png))) {
		return false;
	}

	format = png_format(GL_RGBA);
	if (format == PNG_COLOR_TYPE_INVALID)
		return false;

	depth = png_depth(format);

	png_set_IHDR(png, info, pbuffer->width, pbuffer->height, 8, format,
		     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
		     PNG_FILTER_TYPE_BASE);
	png_write_info(png, info);

	if (setjmp(png_jmpbuf(png))) {
		fprintf(stderr, "failed to write IHDR\n");
		return false;
	}

	stride = pbuffer->width * (depth / 8);
	size = pbuffer->height * stride;

	data = malloc(size);
	if (!data) {
		return false;
	}

	glReadPixels(0, 0, pbuffer->width, pbuffer->height, GL_RGBA,
		     GL_UNSIGNED_BYTE, data);

	rows = malloc(pbuffer->height * sizeof(png_bytep));
	if (!rows) {
		fprintf(stderr, "out-of-memory\n");
		return false;
	}

	for (i = 0; i < pbuffer->height; i++)
		rows[pbuffer->height - i - 1] = data + i * stride;

	png_write_image(png, rows);

	free(rows);
	free(data);

	if (setjmp(png_jmpbuf(png))) {
		return false;
	}

	png_write_end(png, NULL);

	fclose(fp);
	return true;
}

GLuint glsl_shader_load(GLenum type, const GLchar *lines[], size_t count)
{
	GLuint shader;
	GLint status;

	shader = glCreateShader(type);
	if (!shader)
		return 0;

	glShaderSource(shader, count, lines, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status) {
		GLint size;

		fprintf(stderr, "failed to compile shader:\n");

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
		if (size > 0) {
			GLint length;
			GLchar *log;

			log = malloc(size);
			if (!log) {
				fprintf(stderr, "out-of-memory\n");
				goto delete;
			}

			glGetShaderInfoLog(shader, size, &length, log);
			fprintf(stderr, "%.*s\n", length, log);
			free(log);
		}

delete:
		glDeleteShader(shader);
		shader = 0;
	}

	return shader;
}

GLuint glsl_program_create(GLuint vertex, GLuint fragment)
{
	GLuint program;

	program = glCreateProgram();
	if (!program)
		return 0;

	glAttachShader(program, vertex);
	glAttachShader(program, fragment);

	return program;
}

void glsl_program_link(GLuint program)
{
	GLint status;

	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status) {
		GLint size;

		fprintf(stderr, "failed to link program:\n");

		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &size);
		if (size > 0) {
			GLint length;
			GLchar *log;

			log = malloc(size);
			if (!log) {
				fprintf(stderr, "out-of-memory\n");
				return;
			}

			glGetProgramInfoLog(program, size, &length, log);
			fprintf(stderr, "%.*s\n", length, log);
			free(log);
		}
	}
}

enum image_format {
	IMAGE_FORMAT_RGB888,
	IMAGE_FORMAT_RGBA8888,
};

struct image {
	unsigned int width, height;
	enum image_format format;
	size_t size;
	void *data;
};

struct image *image_load_png(const char *filename)
{
	int depth, color, interlace, transforms;
	png_uint_32 width, height;
	png_structp png = NULL;
	png_infop info = NULL;
	struct image *image;
	png_size_t pitch;
	png_bytepp rows;
	png_uint_32 i;
	FILE *fp;

	image = calloc(1, sizeof(*image));
	if (!image)
		return NULL;

	fp = fopen(filename, "rb");
	if (!fp)
		goto free;

	png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png)
		goto close;

	info = png_create_info_struct(png);
	if (!info)
		goto destroy;

	if (setjmp(png_jmpbuf(png)))
		goto destroy;

	png_init_io(png, fp);
	png_set_sig_bytes(png, 0);

	transforms = PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING |
		     PNG_TRANSFORM_EXPAND;
	png_read_png(png, info, transforms, NULL);

	png_get_IHDR(png, info, &width, &height, &depth, &color, &interlace,
		     NULL, NULL);

	pitch = png_get_rowbytes(png, info);
	rows = png_get_rows(png, info);

	if (depth != 8) {
		fprintf(stderr, "ERROR: only 8-bit components supported\n");
		goto destroy;
	}

	if (color & PNG_COLOR_MASK_COLOR) {
		if (color & PNG_COLOR_MASK_ALPHA)
			image->format = IMAGE_FORMAT_RGBA8888;
		else
			image->format = IMAGE_FORMAT_RGB888;
	}

	image->width = width;
	image->height = height;
	image->size = pitch * height;

	image->data = malloc(image->size);
	if (!image->data)
		goto destroy;

	for (i = 0; i < height; i++) {
		unsigned int index = height - i - 1;
		unsigned long offset = i * pitch;

		memcpy(image->data + offset, rows[index], pitch);
	}

	png_destroy_read_struct(&png, &info, NULL);
	fclose(fp);

	return image;

destroy:
	png_destroy_read_struct(&png, &info, NULL);
close:
	fclose(fp);
free:
	free(image);
	return NULL;
}

void image_free(struct image *image)
{
	if (image)
		free(image->data);

	free(image);
}

GLenum gles_texture_format(struct image *image)
{
	switch (image->format) {
	case IMAGE_FORMAT_RGB888:
		return GL_RGB;

	case IMAGE_FORMAT_RGBA8888:
		return GL_RGBA;
	}

	return GL_NONE;
}

struct texture {
	struct gles_texture base;
	struct image *image;
};

struct gles_texture *gles_texture_load(const char *filename)
{
	struct texture *texture;

	texture = calloc(1, sizeof(*texture));
	if (!texture)
		return NULL;

	texture->image = image_load_png(filename);
	if (!texture->image) {
		free(texture);
		return NULL;
	}

	texture->base.format = gles_texture_format(texture->image);

	glGenTextures(1, &texture->base.id);
	glBindTexture(GL_TEXTURE_2D, texture->base.id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, texture->base.format,
		     texture->image->width, texture->image->height, 0,
		     texture->base.format, GL_UNSIGNED_BYTE,
		     texture->image->data);

	return &texture->base;
}

void texture_free(struct texture *texture)
{
	if (texture) {
		glDeleteTextures(1, &texture->base.id);
		image_free(texture->image);
	}

	free(texture);
}
