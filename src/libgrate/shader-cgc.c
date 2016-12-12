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

#include <locale.h>
#include <stdio.h>
#include <string.h>

#include "libgrate-private.h"
#include "host1x.h"
#include "libcgc.h"
#include "grate.h"
#include "asm.h"

void grate_shader_emit(struct host1x_pushbuf *pb, struct grate_shader *shader)
{
	unsigned int i;

	for (i = 0; i < shader->num_words; i++)
		host1x_pushbuf_push(pb, shader->words[i]);
}

struct grate_shader *grate_shader_new(struct grate *grate,
				      enum grate_shader_type type,
				      const char *lines[],
				      unsigned int count)
{
	size_t length = 0, offset = 0, len;
	enum cgc_shader_type shader_type;
	struct cgc_fragment_shader *fs;
	struct cgc_vertex_shader *vs;
	struct grate_shader *shader;
	struct cgc_header *header;
	struct cgc_shader *cgc;
	unsigned int i;
	size_t size;
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

	switch (cgc->type) {
	case CGC_SHADER_VERTEX:
		vs = cgc->stream;
		fprintf(stdout, "DEBUG: vertex shader: %u words\n", vs->unknownec / 4);
		shader->words = cgc->stream + vs->unknowne8 * 4;
		shader->num_words = vs->unknownec / 4;
		break;

	case CGC_SHADER_FRAGMENT:
		header = cgc->binary;

		fs = cgc->binary + header->binary_offset;
		size = header->binary_size - sizeof(*fs);

		fprintf(stdout, "DEBUG: fragment shader: %zu words\n", size / 4);
		shader->num_words = size / 4;
		shader->words = fs->words;
		break;

	default:
		shader->num_words = 0;
		shader->words = NULL;
		break;
	}

	free(code);

	return shader;
}

static void grate_free_asm_shader(struct grate_shader *shader)
{
	free(shader->words);
	free(shader->cgc->symbols);
	free(shader->cgc);
}

void grate_shader_free(struct grate_shader *shader)
{
	if (shader) {
		switch (shader->cgc->type) {
		case CGC_SHADER_VERTEX:
		case CGC_SHADER_FRAGMENT:
			cgc_shader_free(shader->cgc);
			break;

		default:
			grate_free_asm_shader(shader);
			break;
		}
	}

	free(shader);
}

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

static void grate_program_add_attribute(struct grate_program *program,
					struct cgc_symbol *symbol)
{
	struct grate_attribute *attribute;
	size_t size;

	size = (program->num_attributes + 1) * sizeof(*attribute);

	attribute = realloc(program->attributes, size);
	if (!attribute) {
		fprintf(stderr, "ERROR: failed to allocate attribute\n");
		return;
	}

	program->attributes = attribute;

	attribute = &program->attributes[program->num_attributes];
	attribute->position = symbol->location;
	attribute->name = symbol->name;

	program->num_attributes++;
}

static void grate_program_add_uniform(struct grate_program *program,
				      struct cgc_symbol *symbol)
{
	struct grate_uniform *uniform;
	size_t size;

	size = (program->num_uniforms + 1) * sizeof(*uniform);

	uniform = realloc(program->uniforms, size);
	if (!uniform) {
		fprintf(stderr, "ERROR: failed to allocate uniform\n");
		return;
	}

	program->uniforms = uniform;

	uniform = &program->uniforms[program->num_uniforms];
	uniform->position = symbol->location;
	uniform->name = symbol->name;

	program->num_uniforms++;
}

void grate_program_link(struct grate_program *program)
{
	struct cgc_shader *shader;
	unsigned int i;

	shader = program->vs->cgc;

	for (i = 0; i < shader->num_symbols; i++) {
		struct cgc_symbol *symbol = &shader->symbols[i];
		uint32_t attr_mask = (1 << symbol->location);

		if (symbol->location == -1)
			continue;

		switch (symbol->kind) {
		case GLSL_KIND_ATTRIBUTE:
			printf("attribute %s @%u", symbol->name,
			       symbol->location);

			if (symbol->input) {
				grate_program_add_attribute(program, symbol);
				program->attributes_mask |= attr_mask << 16;
			} else {
				program->attributes_mask |= attr_mask;
			}

			break;

		case GLSL_KIND_UNIFORM:
			printf("uniform %s @%u", symbol->name,
			       symbol->location);

			grate_program_add_uniform(program, symbol);
			break;

		case GLSL_KIND_CONSTANT:
			printf("constant %s @%u", symbol->name,
			       symbol->location);

			memcpy(&program->uniform[symbol->location * 4],
			       symbol->vector, 16);

			break;

		default:
			break;
		}

		if (symbol->input)
			printf(" (input)");
		else
			printf(" (output)");

		if (!symbol->used)
			printf(" (unused)");

		printf("\n");
	}

	shader = program->fs->cgc;

	for (i = 0; i < shader->num_symbols; i++) {
		struct cgc_symbol *symbol = &shader->symbols[i];

		if (symbol->location == -1)
			continue;

		switch (symbol->kind) {
		case GLSL_KIND_ATTRIBUTE:
			printf("attribute %s @%u", symbol->name,
			       symbol->location);
			break;

		case GLSL_KIND_UNIFORM:
			printf("uniform %s @%u", symbol->name,
			       symbol->location);
			break;

		case GLSL_KIND_CONSTANT:
			printf("constant %s @%u", symbol->name,
			       symbol->location);
			break;

		default:
			break;
		}

		if (symbol->input)
			printf(" (input)");
		else
			printf(" (output)");

		if (!symbol->used)
			printf(" (unused)");

		printf("\n");
	}
}

struct grate_shader *grate_shader_parse_asm(const char *asm_txt)
{
	struct grate_shader *shader;
	struct cgc_symbol *symbols;
	struct cgc_shader *cgc;
	static char *locale;
	uint32_t tmp;
	int err;
	int i;

	locale = strdup( setlocale(LC_ALL, NULL) );
	if (!locale)
		return NULL;

	/* atof() delimiter is locale-dependent! */
	setlocale(LC_ALL, "C");

	reset_asm_parser_state();
	yy_scan_string(asm_txt);
	err = yyparse();

	/* restore locale */
	setlocale(LC_ALL, locale);
	free(locale);

	if (err != 0)
		return NULL;

	if (asm_vs_instructions_nb == 0) {
		fprintf(stderr, "ERROR: no instructions generated");
		return NULL;
	}

	shader = calloc(1, sizeof(*shader));
	if (!shader)
		return NULL;

	shader->num_words = 2 + 4 * asm_vs_instructions_nb;
	shader->words = malloc(shader->num_words * 4);
	if (!shader->words) {
		free(shader);
		return NULL;
	}

	asm_vs_instructions[asm_vs_instructions_nb - 1].end_of_program = 1;

	shader->words[0] = HOST1X_OPCODE_IMM(0x205, 0x00);
	shader->words[1] = HOST1X_OPCODE_NONINCR(0x206,
						 asm_vs_instructions_nb * 4);
	memcpy(shader->words + 2, asm_vs_instructions,
	       asm_vs_instructions_nb * 4 * 4);

	for (i = 2; i < shader->num_words; i += 4) {
		tmp = shader->words[i + 3];
		shader->words[i + 3] = shader->words[i];
		shader->words[i] = tmp;

		tmp = shader->words[i + 2];
		shader->words[i + 2] = shader->words[i + 1];
		shader->words[i + 1] = tmp;
	}

	cgc = calloc(1, sizeof(*cgc));
	if (!shader) {
		free(shader);
		return NULL;
	}

	shader->cgc = cgc;

	for (i = 0; i < 16; i++) {
		if (!asm_vs_attributes[i].used)
			continue;

		symbols = realloc(cgc->symbols,
			(cgc->num_symbols + 1) * sizeof(struct cgc_symbol));

		if (!symbols) {
			free(cgc->symbols);
			free(shader->cgc);
			free(shader);
			return NULL;
		}

		cgc->symbols = symbols;

		cgc->symbols[cgc->num_symbols].location = i;
		cgc->symbols[cgc->num_symbols].kind = GLSL_KIND_ATTRIBUTE;
		cgc->symbols[cgc->num_symbols].type = GLSL_TYPE_VEC4;
		cgc->symbols[cgc->num_symbols].name = asm_vs_attributes[i].name;
		cgc->symbols[cgc->num_symbols].input = true;
		cgc->symbols[cgc->num_symbols].used = true;

		cgc->num_symbols++;
	}

	for (i = 0; i < 16; i++) {
		if (!asm_vs_exports[i].used)
			continue;

		symbols = realloc(cgc->symbols,
			(cgc->num_symbols + 1) * sizeof(struct cgc_symbol));

		if (!symbols) {
			free(cgc->symbols);
			free(shader->cgc);
			free(shader);
			return NULL;
		}

		cgc->symbols = symbols;

		cgc->symbols[cgc->num_symbols].location = i;
		cgc->symbols[cgc->num_symbols].kind = GLSL_KIND_ATTRIBUTE;
		cgc->symbols[cgc->num_symbols].type = GLSL_TYPE_UNKNOWN;
		cgc->symbols[cgc->num_symbols].name = asm_vs_exports[i].name;
		cgc->symbols[cgc->num_symbols].input = false;
		cgc->symbols[cgc->num_symbols].used = true;

		cgc->num_symbols++;
	}

	for (i = 0; i < 256; i++) {
		if (!asm_vs_constants[i].used)
			continue;

		symbols = realloc(cgc->symbols,
			(cgc->num_symbols + 1) * sizeof(struct cgc_symbol));

		if (!symbols) {
			free(cgc->symbols);
			free(shader->cgc);
			free(shader);
			return NULL;
		}

		cgc->symbols = symbols;

		cgc->symbols[cgc->num_symbols].location = i;
		cgc->symbols[cgc->num_symbols].kind = GLSL_KIND_CONSTANT;
		cgc->symbols[cgc->num_symbols].type = GLSL_TYPE_VEC4;
		cgc->symbols[cgc->num_symbols].name = "asm constant";
		cgc->symbols[cgc->num_symbols].input = true;
		cgc->symbols[cgc->num_symbols].used = true;
		cgc->symbols[cgc->num_symbols].vector[0] =
					asm_vs_constants[i].vector.x.value;
		cgc->symbols[cgc->num_symbols].vector[1] =
					asm_vs_constants[i].vector.y.value;
		cgc->symbols[cgc->num_symbols].vector[2] =
					asm_vs_constants[i].vector.z.value;
		cgc->symbols[cgc->num_symbols].vector[3] =
					asm_vs_constants[i].vector.w.value;
		cgc->num_symbols++;
	}

	return shader;
}

const char *grate_shader_disasm_vs(struct grate_shader *shader)
{
	char *disassembly = malloc(1);
	const char *instr_txt;
	char *tmp;
	vpe_instr128 instr;
	int i, sz;

	if (!disassembly)
		return NULL;

	disassembly[0] = '\0';

	for (i = 2, sz = 1; i < shader->num_words - 3; i += 4) {
		instr.part0 = *(shader->words + i + 3);
		instr.part1 = *(shader->words + i + 2);
		instr.part2 = *(shader->words + i + 1);
		instr.part3 = *(shader->words + i + 0);

		instr_txt = vpe_vliw_disassemble(&instr);

		sz += strlen(instr_txt) + 1;
		tmp = realloc(disassembly, sz);

		if (!tmp) {
			free(disassembly);
			return NULL;
		}

		disassembly = tmp;
		strcat(disassembly, "\n");
		strcat(disassembly, instr_txt);
	}

	return disassembly;
}
