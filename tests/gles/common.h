#ifndef GRATE_TESTS_3D_COMMON_H
#define GRATE_TESTS_3D_COMMON_H 1

#include <stdbool.h>

#include <X11/Xlib.h>
#include <EGL/egl.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

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
