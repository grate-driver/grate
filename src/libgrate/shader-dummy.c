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

#include "libgrate-private.h"
#include "host1x.h"

struct grate_shader {
};

struct grate_shader *grate_shader_new(struct grate *grate,
				      enum grate_shader_type type,
				      const char *lines[],
				      unsigned int count)
{
	return NULL;
}

void grate_shader_free(struct grate_shader *shader)
{
	free(shader);
}

void grate_shader_emit(struct host1x_pushbuf *pb, struct grate_shader *shader)
{
}

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

struct grate_shader *grate_shader_parse_asm(const char *asm_txt)
{
	return NULL;
}

const char *grate_shader_disasm_vs(struct grate_shader *shader)
{
	return NULL;
}
