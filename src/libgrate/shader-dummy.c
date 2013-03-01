#include "grate.h"

struct grate_shader {
};

struct grate_shader *grate_shader_new(struct grate *grate,
				      enum grate_shader_type type,
				      const char *lines[],
				      unsigned int count)
{
	struct grate_shader *shader;

	shader = calloc(1, sizeof(*shader));
	if (!shader)
		return NULL;

	return shader;
}

void grate_shader_free(struct grate_shader *shader)
{
	free(shader);
}

struct grate_program {
};

struct grate_program *grate_program_new(struct grate *grate,
					struct grate_shader *vs,
					struct grate_shader *fs)
{
	struct grate_program *program;

	program = calloc(1, sizeof(*program));
	if (!program)
		return NULL;

	return program;
}

void grate_program_free(struct grate_program *program)
{
	free(program);
}

void grate_program_link(struct grate_program *program)
{
}

void grate_uniform(struct grate *grate, const char *name, unsigned int count,
		   float *values)
{
}
