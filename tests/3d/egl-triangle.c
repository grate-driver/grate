#include <stdio.h>

#include <GLES2/gl2.h>
#include <EGL/egl.h>

int main(int argc, char *argv[])
{
	GLuint vertex, fragment;
	EGLint major, minor;
	EGLDisplay display;
	EGLContext context;
	EGLSurface surface;
	EGLint num_config;
	EGLConfig config;
	GLuint program;
	GLint height;
	GLint width;

	static const char *vertex_source =
		"attribute vec4 position;\n"
		"attribute vec4 color;\n"
		"\n"
		"varying vec4 vcolor;\n"
		"\n"
		"void main()\n"
		"{\n"
		"  gl_Position = position;\n"
		"  vcolor = color;\n"
		"}\n";

	static const char *fragment_source =
		"precision mediump float;\n"
		"varying vec4 vcolor;\n"
		"\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = vcolor;\n"
		"}\n";

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

	static GLfloat vertices[] = {
		 0.0f,  0.5f, 0.0f,
		-0.5f, -0.5f, 0.0f,
		 0.5f, -0.5f, 0.0f,
	};

	static GLfloat colors[] = {
		1.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f,
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

	glFlush();

	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vertex_source, NULL);
	glCompileShader(vertex);

	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fragment_source, NULL);
	glCompileShader(fragment);

	program = glCreateProgram();
	glAttachShader(program, vertex);
	glAttachShader(program, fragment);

	glBindAttribLocation(program, 0, "position");
	glBindAttribLocation(program, 1, "color");

	glLinkProgram(program);
	glUseProgram(program);

	glFlush();

	glClearColor(0.0, 0.0, 0.0, 1.0);
	glFlush();
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices);
	glFlush();
	glEnableVertexAttribArray(0);
	glFlush();

	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, colors);
	glFlush();
	glEnableVertexAttribArray(1);
	glFlush();

	glDrawArrays(GL_TRIANGLES, 0, 3);
	glFlush();

	eglSwapBuffers(display, surface);
	glFlush();

	eglDestroySurface(display, surface);
	glFlush();

	eglTerminate(display);
	return 0;
}
