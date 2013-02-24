#include <stdio.h>
#include <string.h>

#include "cgdrv.h"
#include "grate.h"

struct grate_shader {
	void *binary;
};

struct grate_shader *grate_shader_new(struct grate *grate,
				      enum grate_shader_type type,
				      const char *lines[],
				      unsigned int count)
{
	size_t length = 0, offset = 0, len;
	enum cgdrv_shader_type shader_type;
	struct grate_shader *shader;
	struct cgdrv_shader *cg;
	unsigned int i;
	char *code;

	switch (type) {
	case GRATE_SHADER_VERTEX:
		shader_type = CGDRV_SHADER_VERTEX;
		break;

	case GRATE_SHADER_FRAGMENT:
		shader_type = CGDRV_SHADER_FRAGMENT;
		break;

	default:
		return NULL;
	}

	for (i = 0; i < count; i++)
		length += strlen(lines[i]);

	code = malloc(length + 1);
	if (!code)
		return NULL;

	for (i = 0; i < count; i++) {
		len = strlen(lines[i]);
		strncpy(code + offset, lines[i], len);
		offset += len;
	}

	code[length] = '\0';

	cg = cgdrv_compile(shader_type, code, length);
	if (!cg) {
		free(code);
		return NULL;
	}

	shader = calloc(1, sizeof(*shader));
	if (!shader) {
		free(code);
		return NULL;
	}

	free(code);

	return shader;
}

void grate_shader_free(struct grate_shader *shader)
{
	free(shader);
}

struct grate_program {
	struct grate_shader *vs;
	struct grate_shader *fs;
};

struct grate_program *grate_program_new(struct grate *grate,
					struct grate_shader *vs,
					struct grate_shader *fs)
{
	struct grate_program *program;

	program = calloc(1, sizeof(*program));
	if (!program)
		return NULL;

	program->vs = vs;
	program->fs = fs;

	return program;
}

void grate_program_free(struct grate_program *program)
{
	if (program) {
		grate_shader_free(program->fs);
		grate_shader_free(program->vs);
	}

	free(program);
}
