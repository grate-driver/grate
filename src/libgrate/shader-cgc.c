#include <stdio.h>
#include <string.h>

#include "libgrate-private.h"
#include "libcgc.h"
#include "grate.h"

struct grate_shader {
	struct cgc_shader *cgc;
};

struct grate_shader *grate_shader_new(struct grate *grate,
				      enum grate_shader_type type,
				      const char *lines[],
				      unsigned int count)
{
	size_t length = 0, offset = 0, len;
	enum cgc_shader_type shader_type;
	struct grate_shader *shader;
	struct cgc_shader *cgc;
	unsigned int i;
	char *code;

	switch (type) {
	case GRATE_SHADER_VERTEX:
		shader_type = CGC_SHADER_VERTEX;
		break;

	case GRATE_SHADER_FRAGMENT:
		shader_type = CGC_SHADER_FRAGMENT;
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

	cgc = cgc_compile(shader_type, code, length);
	if (!cgc) {
		free(code);
		return NULL;
	}

	//cgc_shader_dump(cgc, stdout);

	shader = calloc(1, sizeof(*shader));
	if (!shader) {
		free(code);
		return NULL;
	}

	shader->cgc = cgc;

	free(code);

	return shader;
}

void grate_shader_free(struct grate_shader *shader)
{
	if (shader)
		cgc_shader_free(shader->cgc);

	free(shader);
}

struct grate_attribute {
	unsigned int position;
	const char *name;
};

struct grate_uniform {
	unsigned int position;
	const char *name;
};

struct grate_program {
	struct grate_shader *vs;
	struct grate_shader *fs;

	struct grate_attribute *attributes;
	unsigned int num_attributes;

	struct grate_uniform *uniforms;
	unsigned int num_uniforms;
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

void grate_program_link(struct grate_program *program)
{
	struct cgc_shader *shader;
	struct cgc_symbol *symbol;
	unsigned int i;

	shader = program->vs->cgc;

	for (i = 0; i < shader->num_symbols; i++) {
		struct cgc_symbol *symbol = &shader->symbols[i];

		switch (symbol->kind) {
		case GLSL_KIND_ATTRIBUTE:
			printf("attribute %s @%u\n", symbol->name,
			       symbol->location);
			break;

		case GLSL_KIND_UNIFORM:
			printf("uniform %s @%u\n", symbol->name,
			       symbol->location);
			break;

		case GLSL_KIND_CONSTANT:
			printf("constant %s @%u\n", symbol->name,
			       symbol->location);
			break;

		default:
			break;
		}
	}

	shader = program->fs->cgc;

	for (i = 0; i < shader->num_symbols; i++) {
		struct cgc_symbol *symbol = &shader->symbols[i];

		switch (symbol->kind) {
		case GLSL_KIND_ATTRIBUTE:
			printf("attribute %s @%u\n", symbol->name,
			       symbol->location);
			break;

		case GLSL_KIND_UNIFORM:
			printf("uniform %s @%u\n", symbol->name,
			       symbol->location);
			break;

		case GLSL_KIND_CONSTANT:
			printf("constant %s @%u\n", symbol->name,
			       symbol->location);
			break;

		default:
			break;
		}
	}
}

void grate_uniform(struct grate *grate, const char *name, unsigned int count,
		   float *values)
{
}
