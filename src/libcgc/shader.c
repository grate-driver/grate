#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libcgc.h"
#include "host1x.h"
#include "cgdrv.h"

static const char swizzle[4] = { 'x', 'y', 'z', 'w' };

void *memdup(const void *ptr, size_t size)
{
	void *dup;

	dup = malloc(size);
	if (!dup)
		return NULL;

	memcpy(dup, ptr, size);

	return dup;
}

static void vertex_shader_disassemble(struct cgdrv_shader *shader, FILE *fp)
{
	struct cgdrv_shader_header *header = shader->binary;
	const uint32_t *ptr;
	unsigned int i, j;

	ptr = shader->binary + header->binary_offset;
	printf("  instructions:\n");

	for (i = 0; i < header->binary_size; i += 16) {
		uint8_t sx, sy, sz, sw, neg, op, constant, attribute;
		struct instruction *inst;
		uint32_t words[4], reg;

		for (j = 0; j < 4; j++)
			words[j] = *ptr++;

		printf("    ");

		for (j = 0; j < 4; j++)
			printf("%08x", words[j]);

		printf(" |");

		for (j = 0; j < 4; j++)
			printf(" %08x", words[j]);

		printf("\n");

		inst = instruction_create_from_words(words, 4);

		op = instruction_extract(inst, 86, 87);
		switch (op) {
		case 0x1:
			printf("      fetch\n");
			break;
		case 0x2:
			printf("      mul\n");
			break;
		case 0x3:
			printf("      add\n");
			break;
		default:
			printf("      unknown\n");
			break;
		}

		constant = instruction_extract(inst, 76, 83);
		attribute = instruction_extract(inst, 72, 75);

		neg = instruction_get_bit(inst, 71);
		sx = instruction_extract(inst, 69, 70);
		sy = instruction_extract(inst, 67, 68);
		sz = instruction_extract(inst, 65, 66);
		sw = instruction_extract(inst, 63, 64);

		printf("      %ssrc0.%c%c%c%c\n", neg ? "-" : "", swizzle[sx],
		       swizzle[sy], swizzle[sz], swizzle[sw]);

		if (instruction_get_bit(inst, 55))
			printf("        constant #%02x\n", constant);
		else
			printf("        attribute #%02x\n", attribute);

		neg = instruction_get_bit(inst, 54);
		sx = instruction_extract(inst, 52, 53);
		sy = instruction_extract(inst, 50, 51);
		sz = instruction_extract(inst, 48, 49);
		sw = instruction_extract(inst, 46, 47);

		printf("      %ssrc1.%c%c%c%c\n", neg ? "-" : "", swizzle[sx],
		       swizzle[sy], swizzle[sz], swizzle[sw]);

		reg = instruction_extract(inst, 2, 6);
		sx = instruction_get_bit(inst, 16);
		sy = instruction_get_bit(inst, 15);
		sz = instruction_get_bit(inst, 14);
		sw = instruction_get_bit(inst, 13);

		printf("      dst.%s%s%s%s = %x\n", sx ? "x" : "",
		       sy ? "y" : "", sz ? "z" : "", sw ? "w" : "", reg);

		if (instruction_get_bit(inst, 0))
			printf("    done\n");

		instruction_free(inst);
	}
}

static void fragment_shader_disassemble(struct cgdrv_shader *shader, FILE *fp)
{
}

static void shader_stream_dump(struct cgdrv_shader *shader, FILE *fp)
{
	struct cgdrv_shader_header *header;
	struct cgdrv_fragment_shader *fs;
	struct cgdrv_vertex_shader *vs;
	struct host1x_stream stream;
	size_t length;
	void *words;

	fprintf(fp, "dumping stream... %d\n", shader->type);

	switch (shader->type) {
	case CGDRV_SHADER_VERTEX:
		vs = shader->stream;
		words = shader->stream + vs->unknowne8 * 4;
		length = vs->unknownec;
		break;

	case CGDRV_SHADER_FRAGMENT:
		header = shader->binary;
		fs = shader->binary + header->binary_offset;
		length = header->binary_size - sizeof(*fs);
		words = fs->words;

		fprintf(fp, "signature: %.*s\n", 8, fs->signature);
		fprintf(fp, "unknown0: 0x%08x\n", fs->unknown0);
		fprintf(fp, "unknown1: 0x%08x\n", fs->unknown1);
		break;

	default:
		fprintf(fp, "unknown type: %d\n", shader->type);
		return;
	}

	fprintf(fp, "stream @%p, %zu bytes\n", words, length);
	host1x_stream_init(&stream, words, length);
	host1x_stream_dump(&stream, fp);
}

static void cgdrv_shader_disassemble(struct cgdrv_shader *shader, FILE *fp)
{
	struct cgdrv_shader_header *header = shader->binary;

	switch (header->type) {
	case 0x1b5d:
		vertex_shader_disassemble(shader, fp);
		shader_stream_dump(shader, fp);
		break;

	case 0x1b5e:
		fragment_shader_disassemble(shader, fp);
		shader_stream_dump(shader, fp);
		break;
	}
}

struct cgdrv_shader *cgdrv_compile(enum cgdrv_shader_type type,
				   const char *code, size_t size)
{
	struct cgdrv_shader *shader;
	const char *shader_type;
	char source[65536];
	struct CgDrv *cg;
	size_t i;
	int err;

	switch (type) {
	case CGDRV_SHADER_VERTEX:
		shader_type = "vertex";
		break;

	case CGDRV_SHADER_FRAGMENT:
		shader_type = "fragment";
		break;

	default:
		return NULL;
	}

	cg = CgDrv_Create();
	if (!cg)
		return NULL;

	memcpy(source, code, size);

	fprintf(stdout, "compiling %s shader (%zu bytes)...\n", shader_type,
		size);
	fputs("| ", stdout);

	for (i = 0; i < size; i++) {
		fputc(code[i], stdout);

		if (code[i] == '\n' && i != size - 1)
			fputs("| ", stdout);
	}

	fputs("\n", stdout);

	err = CgDrv_Compile(cg, 1, type, source, size, 0);
	if (err) {
		fprintf(stderr, "%s\n", cg->error);
		fprintf(stderr, "%s\n", cg->log);
		CgDrv_Delete(cg);
		return NULL;
	}

	printf("%s\n", cg->log);

	shader = calloc(1, sizeof(*shader));
	if (!shader) {
		CgDrv_Delete(cg);
		return NULL;
	}

	shader->type = type;

	shader->binary = memdup(cg->binary, cg->binary_size);
	shader->size = cg->binary_size;

	shader->stream = memdup(cg->stream, cg->length);
	shader->length = cg->length;

	CgDrv_Delete(cg);

	return shader;
}

void cgdrv_shader_free(struct cgdrv_shader *shader)
{
	if (shader) {
		free(shader->stream);
		free(shader->binary);
	}

	free(shader);
}

void cgdrv_shader_dump(struct cgdrv_shader *shader, FILE *fp)
{
	struct cgdrv_shader_header *header = shader->binary;
	unsigned int i, j;
	const char *type;

	fprintf(fp, "shader binary: %zu bytes\n", shader->size);

	for (i = 0; i < shader->size; i += 4) {
		uint32_t value = *(uint32_t *)(shader->binary + i);
		uint8_t *bytes = (uint8_t *)(shader->binary + i);

		fprintf(fp, "  %08x: %08x |", i, value);

		for (j = 0; j < 4; j++)
			fprintf(fp, " %02x", bytes[j]);

		fprintf(fp, " | ");

		for (j = 0; j < 4; j++) {
			if (isprint(bytes[j]))
				fprintf(fp, "%c", bytes[j]);
			else
				fprintf(fp, ".");
		}

		fprintf(fp, " |\n");
	}

	fprintf(fp, "shader stream: %zu bytes\n", shader->length);

	for (i = 0; i < shader->length; i += 4) {
		uint32_t value = *(uint32_t *)(shader->stream + i);
		uint8_t *bytes = (uint8_t *)(shader->stream + i);

		fprintf(fp, "  %08x: %08x |", i, value);

		for (j = 0; j < 4; j++)
			fprintf(fp, " %02x", bytes[j]);

		fprintf(fp, " | ");

		for (j = 0; j < 4; j++) {
			if (isprint(bytes[j]))
				fprintf(fp, "%c", bytes[j]);
			else
				fprintf(fp, ".");
		}

		fprintf(fp, " |\n");
	}

	if (header->type == 0x1b5d)
		type = "vertex";
	else if (header->type == 0x1b5e)
		type = "fragment";
	else
		type = "unknown";

	fprintf(fp, "/* %s shader */\n", type);
	fprintf(fp, "{\n");
	fprintf(fp, "\t.type = 0x%08x,\n", header->type);
	fprintf(fp, "\t.unknown00 = 0x%08x,\n", header->unknown00);
	fprintf(fp, "\t.size = 0x%08x,\n", header->size);
	fprintf(fp, "\t.num_symbols = %u,\n", header->num_symbols);
	fprintf(fp, "\t.bar_size = %u,\n", header->bar_size);
	fprintf(fp, "\t.bar_offset = 0x%08x,\n", header->bar_offset);
	fprintf(fp, "\t.binary_size = %u,\n", header->binary_size);
	fprintf(fp, "\t.binary_offset = 0x%08x,\n", header->binary_offset);
	fprintf(fp, "\t.unknown01 = 0x%08x,\n", header->unknown01);
	fprintf(fp, "\t.unknown02 = 0x%08x,\n", header->unknown02);
	fprintf(fp, "\t.unknown03 = 0x%08x,\n", header->unknown03);
	fprintf(fp, "\t.unknown04 = 0x%08x,\n", header->unknown04);
	fprintf(fp, "\t.symbols = {\n");

	for (i = 0; i < header->num_symbols; i++) {
		struct cgdrv_shader_symbol *symbol = &header->symbol[i];
		const char *name;

		name = shader->binary + symbol->name_offset;

		fprintf(fp, "\t\t[%u] = {\n", i);
		fprintf(fp, "\t\t\t.unknown00 = 0x%08x,\n", symbol->unknown00);
		fprintf(fp, "\t\t\t.unknown01 = 0x%08x,\n", symbol->unknown01);
		fprintf(fp, "\t\t\t.unknown02 = 0x%08x, /* type? */\n", symbol->unknown02);
		fprintf(fp, "\t\t\t.unknown03 = 0x%08x,\n", symbol->unknown03);
		fprintf(fp, "\t\t\t.name_offset = 0x%08x, /* \"%s\" */\n",
			symbol->name_offset, name);
		fprintf(fp, "\t\t\t.values_offset = 0x%08x,\n",
			symbol->values_offset);

		if (symbol->values_offset) {
			const uint32_t *values = shader->binary + symbol->values_offset;

			for (j = 0; j < 4; j++)
				fprintf(fp, "\t\t\t\t0x%08x\n", values[j]);
		}

		fprintf(fp, "\t\t\t.unknown06 = 0x%08x,\n", symbol->unknown06);
		fprintf(fp, "\t\t\t.alt_offset = 0x%08x,\n", symbol->alt_offset);
		fprintf(fp, "\t\t\t.unknown08 = 0x%08x,\n", symbol->unknown08);
		fprintf(fp, "\t\t\t.unknown09 = 0x%08x,\n", symbol->unknown09);
		fprintf(fp, "\t\t\t.unknown10 = 0x%08x,\n", symbol->unknown10);
		fprintf(fp, "\t\t\t.unknown11 = 0x%08x,\n", symbol->unknown11);
		fprintf(fp, "\t\t},\n");
	}

	fprintf(fp, "\t}\n");
	fprintf(fp, "}\n");

	cgdrv_shader_disassemble(shader, fp);
}
