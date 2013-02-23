#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <GLES2/gl2.h>

#include "common.h"

static const GLchar *vertex_shader[] = {
	"attribute vec4 position;\n",
	"attribute vec4 color;\n",
	"varying vec4 vcolor;\n",
	"\n",
	"void main()\n",
	"{\n",
	"    gl_Position = position;\n",
	"    vcolor = color;\n",
	"}"
};

static const GLchar *fragment_shader[] = {
	"precision mediump float;\n",
	"varying vec4 vcolor;\n",
	"uniform float r;\n",
	"uniform float g;\n",
	"uniform float b;\n",
	"\n",
	"void main()\n",
	"{\n",
	"    gl_FragColor = vcolor + vec4(r, g, b, 0.5);\n",
	"}"
};

void pbuffer_draw(struct pbuffer *pbuffer)
{
	static const GLfloat vertices[] = {
		 0.0f,  0.5f, 0.0f, 1.0f,
		-0.5f, -0.5f, 0.0f, 1.0f,
		 0.5f, -0.5f, 0.0f, 1.0f,
	};
	static const GLfloat colors[] = {
		1.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f,
	};
	static const GLushort indices[] = {
		0, 1, 2,
	};
	GLint position, color, r, g, b;
	GLuint vs, fs, program;

	printf("=== compiling shaders\n");
	vs = glsl_shader_load(GL_VERTEX_SHADER, vertex_shader,
			      ARRAY_SIZE(vertex_shader));
	fs = glsl_shader_load(GL_FRAGMENT_SHADER, fragment_shader,
			      ARRAY_SIZE(fragment_shader));
	glFlush();
	printf("=== linking program\n");
	program = glsl_program_create(vs, fs);
	glsl_program_link(program);
	glFlush();
	printf("=== using program\n");
	glUseProgram(program);
	glFlush();

	position = glGetAttribLocation(program, "position");
	color = glGetAttribLocation(program, "color");
	r = glGetUniformLocation(program, "r");
	g = glGetUniformLocation(program, "g");
	b = glGetUniformLocation(program, "b");

	printf("=== clearing buffer\n");
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();

	printf("=== setting vertex attribute\n");
	glVertexAttribPointer(position, 4, GL_FLOAT, GL_FALSE,
			      4 * sizeof(GLfloat), vertices);
	glFlush();
	printf("=== enabling vertex attribute\n");
	glEnableVertexAttribArray(position);
	glFlush();

	printf("=== setting vertex attribute\n");
	glVertexAttribPointer(color, 4, GL_FLOAT, GL_FALSE,
			      4 * sizeof(GLfloat), colors);
	glFlush();
	printf("=== enabling vertex attribute\n");
	glEnableVertexAttribArray(color);
	glFlush();

	glUniform1f(r, 0.0f);
	glUniform1f(g, 0.5f);
	glUniform1f(b, 1.0f);

	printf("=== drawing triangles\n");
	glDrawElements(GL_TRIANGLES, ARRAY_SIZE(indices), GL_UNSIGNED_SHORT,
		       indices);
	glFlush();
	printf("=== done\n");
}

int main(int argc, char *argv[])
{
	struct pbuffer *pbuffer;

	pbuffer = pbuffer_create(32, 32);
	if (!pbuffer) {
		fprintf(stderr, "pbuffer_create() failed\n");
		return 1;
	}

	pbuffer_draw(pbuffer);
	pbuffer_save(pbuffer, "test.png");
	pbuffer_free(pbuffer);

	return 0;
}
