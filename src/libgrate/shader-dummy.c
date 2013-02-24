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
