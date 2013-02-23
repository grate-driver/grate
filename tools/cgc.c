#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BIT(x) (1 << (x))

struct cgdrv {
	unsigned int unknown00;
	const char *error;
	const char *log;
	size_t binary_size1;
	void *binary1;
	unsigned int unknown14;
	void *binary;
	size_t binary_size;
	unsigned int unknown20;
	unsigned int unknown24;
	unsigned int unknown28;
	unsigned int unknown2c;
	unsigned int unknown30;
	unsigned int unknown34;
	unsigned int unknown38;
	unsigned int unknown3c;
	unsigned int unknown40;
	unsigned int unknown44;
	unsigned int unknown48;
	unsigned int unknown4c;
};

struct cgdrv *CgDrv_Create(void);
void CgDrv_Delete(struct cgdrv *cgdrv);
void CgDrv_CleanUp(struct cgdrv *cgdrv);
int CgDrv_Compile(struct cgdrv *cgdrv, int unknown, int type,
		  const char *code, size_t length, int unknown2);

enum cgdrv_shader_type {
	CGDRV_SHADER_VERTEX = 1,
	CGDRV_SHADER_FRAGMENT = 2,
};

struct shader_binary {
	char signature[8];
	uint32_t unknown1;
	uint32_t unknown2;
	uint32_t words[0];
};

struct opts {
	enum cgdrv_shader_type type;
	bool version;
	bool help;
};

struct instruction {
	unsigned int length;
	uint32_t *bits;
};

struct instruction *instruction_create_from_words(uint32_t *words,
						  unsigned int count)
{
	struct instruction *inst;
	unsigned int i;

	inst = calloc(1, sizeof(*inst));
	if (!inst)
		return NULL;

	inst->length = count * 32;

	inst->bits = malloc(sizeof(*words) * count);
	if (!inst->bits) {
		free(inst);
		return NULL;
	}

	for (i = 0; i < count; i++)
		inst->bits[i] = words[count - i - 1];

	return inst;
}

void instruction_free(struct instruction *inst)
{
	if (inst)
		free(inst->bits);

	free(inst);
}

unsigned int instruction_get_bit(struct instruction *inst, unsigned int pos)
{
	unsigned int word = pos / 32;
	unsigned int bit = pos % 32;

	if (pos >= inst->length) {
		fprintf(stderr, "WARNING: bit out of range: %u\n", pos);
		return 0;
	}

	return (inst->bits[word] & (1 << bit)) != 0;
}

uint32_t instruction_extract(struct instruction *inst, unsigned int from,
			     unsigned int to)
{
	unsigned int bits = to - from, i;
	uint32_t value = 0;

	if (to < from || bits > 32) {
		fprintf(stderr, "WARNING: invalid bitfield: %u-%u\n", from,
			to);
		return 0;
	}

	for (i = from; i <= to; i++) {
		unsigned int word = i / 32;
		unsigned int bit = i % 32;

		if (inst->bits[word] & (1 << bit))
			value |= 1 << (i - from);
	}

	return value;
}

#if 0
enum {
	DUMP_PREFIX_NONE,
	DUMP_PREFIX_OFFSET,
	DUMP_PREFIX_ADDRESS,
};

static void print_hexdump(FILE *fp, int prefix_type, const char *prefix,
			  const void *buffer, size_t size, size_t columns,
			  bool ascii)
{
	const unsigned char *ptr = buffer;
	unsigned int i, j;

	for (j = 0; j < size; j += columns) {
		const char *space = "";

		if (prefix)
			fputs(prefix, fp);

		switch (prefix_type) {
		case DUMP_PREFIX_NONE:
		default:
			break;

		case DUMP_PREFIX_OFFSET:
			fprintf(fp, "%08x: ", j);
			break;

		case DUMP_PREFIX_ADDRESS:
			fprintf(fp, "%p: ", buffer + j);
			break;
		}

		for (i = 0; (i < columns) && (j + i < size); i++) {
			fprintf(fp, "%s%02x", space, ptr[j + i]);
			space = " ";
		}

		for (i = i; i < columns; i++)
			fprintf(fp, "   ");

		fputs(" | ", fp);

		if (ascii) {
			for (i = 0; (i < columns) && (j + i < size); i++) {
				if (isprint(ptr[j + i]))
					fprintf(fp, "%c", ptr[j + i]);
				else
					fprintf(fp, ".");
			}
		}

		fputs("\n", fp);
	}
}
#endif

static void usage(FILE *fp, const char *program)
{
	fprintf(fp, "usage: %s [options] FILE\n", program);
}

static int parse_command_line(struct opts *opts, int argc, char *argv[])
{
	static const struct option options[] = {
		{ "fragment", 0, NULL, 'F' },
		{ "help", 0, NULL, 'h' },
		{ "vertex", 0, NULL, 'v' },
		{ "version", 0, NULL, 'V' },
		{ NULL, 0, NULL, 0 }
	};
	int opt;

	memset(opts, 0, sizeof(*opts));
	opts->type = CGDRV_SHADER_FRAGMENT;

	while ((opt = getopt_long(argc, argv, "FhvV", options, NULL)) != -1) {
		switch (opt) {
		case 'F':
			opts->type = CGDRV_SHADER_FRAGMENT;
			break;

		case 'h':
			opts->help = true;
			break;

		case 'v':
			opts->type = CGDRV_SHADER_VERTEX;
			break;

		case 'V':
			opts->version = true;
			break;

		default:
			fprintf(stderr, "invalid option '%c'\n", opt);
			return -1;
		}
	}

	return optind;
}

struct stream {
	const uint32_t *words;
	const uint32_t *ptr;
	const uint32_t *end;
};

enum host1x_opcode {
	HOST1X_OPCODE_SETCL,
	HOST1X_OPCODE_INCR,
	HOST1X_OPCODE_NONINCR,
	HOST1X_OPCODE_MASK,
	HOST1X_OPCODE_IMM,
	HOST1X_OPCODE_RESTART,
	HOST1X_OPCODE_GATHER,
	HOST1X_OPCODE_EXTEND = 14,
	HOST1X_OPCODE_CHDONE = 15,
};

void dump_lut(struct stream *stream, unsigned int count)
{
	unsigned int i;

	for (i = 0; i < count; i += 2) {
		uint32_t lo = *stream->ptr++;
		uint32_t hi = *stream->ptr++;
		uint8_t opcode, reg;
		uint64_t data;

		data = ((uint64_t)hi << 32) | lo;
		printf("      %016llx\n", data);

		opcode = (data >> 22) & 0xf;
		reg = (data >> 26) & 0x1f;

		printf("        opcode:%x reg:%x\n", opcode, reg);
	}
}

static void dump_operand_base(uint16_t operand, int xreg)
{
	uint8_t x10 = (operand >>  3) & 0x01;
	uint8_t reg = (operand >>  5) & 0x3f;
	uint8_t uni = (operand >> 11) & 0x01;

	if (operand & ~7) {
		if (operand == 0x7c8) {
			printf("#0");
			return;
		} else if (operand == 0x7e8) {
			printf("#1");
			return;
		}
	}

	//printf("%3x %x %x", operand, x10, reg);
	printf("%3x:", operand);

	printf("%c%u%s", uni ? 'c' : 'r', reg >> 1, x10 ? " fx10" : "");
}

static void dump_operand(uint16_t operand, int xreg)
{
	uint8_t s2x, neg, abs;

	s2x = (operand >> 0) & 1;
	neg = (operand >> 1) & 1;
	abs = (operand >> 2) & 1;

	if (neg)
		printf("-");
	else
		printf(" ");

	if (abs)
		printf("abs(");

	dump_operand_base(operand, xreg);

	if (s2x)
		printf(" * #2");

	if (abs)
		printf(")");
}

static void dump_alu(struct stream *stream, unsigned int count)
{
	unsigned int i;

	for (i = 0; i < count; i += 2) {
		uint32_t lo = *stream->ptr++;
		uint32_t hi = *stream->ptr++;
		uint8_t opcode, xreg;
		uint64_t inst;

		inst = ((uint64_t)hi << 32) | lo;
		printf("      %016llx\n", inst);

		opcode = (inst >> 30) & 0x3;
		xreg = (inst >> 12) & 0x1;

		printf("        opcode: %x %x (", opcode, xreg);
		dump_operand((inst >>  0) & 0xfff, xreg);
		printf(", ");
		dump_operand((inst >> 51) & 0xfff, xreg);
		printf(", ");
		dump_operand((inst >> 38) & 0xfff, xreg);
		printf(")\n");

		if ((inst & 0x7f8) == 0x7e8)
			printf("        #1\n");

		if ((inst & 0x7f8) == 0x7c8)
			printf("        #0\n");
	}
}

void dump_stream(struct stream *stream)
{
	while (stream->ptr < stream->end) {
		uint32_t opcode = (*stream->ptr >> 28) & 0xf;
		uint16_t offset, count, mask, value, i;

		printf("    %08x: ", *stream->ptr);

		switch (opcode) {
		case HOST1X_OPCODE_SETCL:
			printf("HOST1X_OPCODE_SETCL\n");
			stream->ptr++;
			break;

		case HOST1X_OPCODE_INCR:
			offset = (*stream->ptr >> 16) & 0xfff;
			count = *stream->ptr & 0xffff;

			printf("HOST1X_OPCODE_INCR    0x%03x, 0x%04x\n",
			       offset, count);

			stream->ptr++;

			for (i = 0; i < count; i++)
				printf("      %08x\n", *stream->ptr++);

			break;

		case HOST1X_OPCODE_NONINCR:
			offset = (*stream->ptr >> 16) & 0xfff;
			count = *stream->ptr & 0xffff;

			printf("HOST1X_OPCODE_NONINCR 0x%03x, 0x%04x\n",
			       offset, count);

			stream->ptr++;

			switch (offset) {
			case 0x604: /* LUT */
				dump_lut(stream, count);
				break;

			case 0x804: /* ALU */
				dump_alu(stream, count);
				break;

			default:
				for (i = 0; i < count; i++)
					printf("      %08x\n", *stream->ptr++);

				break;
			}

			break;

		case HOST1X_OPCODE_MASK:
			offset = (*stream->ptr >> 16) & 0xfff;
			mask = *stream->ptr & 0xffff;

			printf("HOST1X_OPCODE_MASK    0x%03x 0x%04x\n",
			       offset, mask);

			stream->ptr++;

			for (i = 0; i < 16; i++)
				if (mask & BIT(i))
					printf("      %08x: %08x\n",
					       offset + i, *stream->ptr++);

			break;

		case HOST1X_OPCODE_IMM:
			offset = (*stream->ptr >> 16) & 0xfff;
			value = *stream->ptr & 0xffff;

			printf("HOST1X_OPCODE_IMM      0x%03x 0x%04x\n",
			       offset, value);

			stream->ptr++;
			break;

		case HOST1X_OPCODE_RESTART:
			printf("HOST1X_OPCODE_RESTART\n");
			break;

		case HOST1X_OPCODE_GATHER:
			printf("HOST1X_OPCODE_GATHER\n");
			break;

		case HOST1X_OPCODE_EXTEND:
			printf("HOST1X_OPCODE_EXTEND\n");
			break;

		case HOST1X_OPCODE_CHDONE:
			printf("HOST1X_OPCODE_CHDONE\n");
			break;

		default:
			stream->ptr++;
			printf("\n");
			break;
		}
	}
}

static void vertex_shader_disassemble(const void *code, size_t length)
{
	const char swizzle[4] = { 'x', 'y', 'z', 'w' };
	const uint32_t *ptr = code;
	unsigned int i, j;

	printf("  instructions:\n");

	for (i = 0; i < length; i += 16) {
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

struct symbol {
	uint32_t unknown00;
	uint32_t unknown01;
	uint32_t unknown02;
	uint32_t unknown03;
	uint32_t name_offset;
	uint32_t values_offset;
	uint32_t unknown06;
	uint32_t alt_offset;
	uint32_t unknown08;
	uint32_t unknown09;
	uint32_t unknown10;
	uint32_t unknown11;
};

struct cgbin {
	uint32_t type;
	uint32_t unknown00;
	uint32_t size;
	uint32_t num_foo;
	uint32_t bar_size;
	uint32_t bar_offset;
	uint32_t binary_size;
	uint32_t binary_offset;
	uint32_t unknown01;
	uint32_t unknown02;
	uint32_t unknown03;
	uint32_t unknown04;

	struct symbol symbol[0];
};

/**
 * This only works if the code is provided through static storage. Allocating
 * memory and passing the pointer into CgDrv_Compile() will segfault.
 */
int main(int argc, char *argv[])
{
	const struct cgbin *cgbin;
	const char *shader_type;
	size_t length, i, j;
	struct opts opts;
	struct cgdrv *cg;
	char code[65536];
	FILE *fp;
	int err;

	err = parse_command_line(&opts, argc, argv);
	if (err < 0) {
		return 1;
	}

	if (opts.help) {
		usage(stdout, argv[0]);
		return 0;
	}

	if (opts.version) {
		printf("cgc %s\n", PACKAGE_VERSION);
		return 0;
	}

	if (err < argc) {
		fp = fopen(argv[err], "r");
		if (!fp) {
			fprintf(stderr, "failed to open `%s': %m\n", argv[1]);
			return 1;
		}
	} else {
		printf("reading stdin\n");
		fp = stdin;
	}

	length = fread(code, 1, sizeof(code), fp);
	if (length == 0) {
	}

	code[length] = '\0';

	fclose(fp);

	cg = CgDrv_Create();
	if (!cg) {
		return 1;
	}

	printf("Compiling shader (%zu bytes)...\n", length);
	fputs("| ", stdout);

	for (i = 0; i < length; i++) {
		fputc(code[i], stdout);

		if (code[i] == '\n' && i != length - 1)
			fputs("| ", stdout);
	}

	err = CgDrv_Compile(cg, 1, opts.type, code, length, 0);
	printf("CgDrv_Compile(): %d\n", err);
	if (err) {
		fprintf(stderr, "%s\n", cg->error);
		fprintf(stderr, "%s\n", cg->log);
		CgDrv_Delete(cg);
		return 1;
	}

	printf("%s\n", cg->log);

#if 0
	printf("Binary: %zu bytes\n", cg->binary_size);
	print_hexdump(stdout, DUMP_PREFIX_OFFSET, NULL, cg->binary,
		      cg->binary_size, 16, true);
	printf("Binary: %zu bytes\n", cg->binary_size1);
	print_hexdump(stdout, DUMP_PREFIX_OFFSET, NULL, cg->binary1,
		      cg->binary_size1, 16, true);
#endif

	for (i = 0; i < cg->binary_size; i += 4) {
		uint32_t value = *(uint32_t *)(cg->binary + i);
		uint8_t *bytes = (uint8_t *)(cg->binary + i);

		printf("%08x: %08x |", i, value);

		for (j = 0; j < 4; j++)
			printf(" %02x", bytes[j]);

		printf(" | ");

		for (j = 0; j < 4; j++) {
			if (isprint(bytes[j]))
				printf("%c", bytes[j]);
			else
				printf(".");
		}

		printf(" |\n");
	}

	cgbin = cg->binary;

	if (cgbin->type == 0x1b5d)
		shader_type = "vertex";
	else if (cgbin->type == 0x1b5e)
		shader_type = "fragment";
	else
		shader_type = "unknown";

	printf("Header: %08x (%s), %08x\n", cgbin->type, shader_type,
	       cgbin->unknown00);
	printf("  size: %u, %x\n", cgbin->size, cgbin->size);
	printf("  foo: %u, %x\n", cgbin->num_foo, cgbin->num_foo);
	printf("  bar: %u bytes, %x\n", cgbin->bar_size, cgbin->bar_size);
	printf("    offset: %x\n", cgbin->bar_offset);
	printf("  binary: %u bytes, %x\n", cgbin->binary_size, cgbin->binary_size);
	printf("    offset: %x\n", cgbin->binary_offset);
	printf("  unknown01: %08x\n", cgbin->unknown01);
	printf("  unknown02: %08x\n", cgbin->unknown02);
	printf("  unknown03: %08x\n", cgbin->unknown03);
	printf("  unknown04: %08x\n", cgbin->unknown04);

	printf("  %u foo structures:\n", cgbin->num_foo);

	for (i = 0; i < cgbin->num_foo; i++) {
		unsigned long offset = (unsigned long)&cgbin->symbol[i] -
				       (unsigned long)cg->binary;

		printf("    %u: %08lx\n", i, offset);
		printf("      unknown00: %08x\n", cgbin->symbol[i].unknown00);
		printf("      unknown01: %08x\n", cgbin->symbol[i].unknown01);
		printf("      unknown02: %08x\n", cgbin->symbol[i].unknown02);
		printf("      unknown03: %08x\n", cgbin->symbol[i].unknown03);
		printf("      name_offset: %08x", cgbin->symbol[i].name_offset);
		if (cgbin->symbol[i].name_offset != 0) {
			uint32_t index = cgbin->symbol[i].name_offset;
			printf(" --> \"%s\"", (char *)cg->binary + index);
		}
		printf("\n");
		//printf("        %s\n", (char *)cg->binary + cgbin->symbol[i].name_offset);
		printf("      values_offset: %08x\n", cgbin->symbol[i].values_offset);

		if (cgbin->symbol[i].values_offset != 0) {
			uint32_t index = cgbin->symbol[i].values_offset / 4;
			uint32_t *binary = cg->binary;

			for (j = 0; j < 4; j++)
				printf("        %08x\n", binary[index + j]);
		}

		printf("      unknown06: %08x\n", cgbin->symbol[i].unknown06);
		printf("      alt_offset: %08x\n", cgbin->symbol[i].alt_offset);

		if (cgbin->symbol[i].alt_offset != 0)
			printf("        %s\n", (char *)cg->binary + cgbin->symbol[i].alt_offset);

		printf("      unknown08: %08x\n", cgbin->symbol[i].unknown08);
		printf("      unknown09: %08x\n", cgbin->symbol[i].unknown09);
		printf("      unknown10: %08x\n", cgbin->symbol[i].unknown10);
		printf("      unknown11: %08x\n", cgbin->symbol[i].unknown11);
	}

	if (1) {
		const uint32_t *binary = cg->binary;
		uint32_t binary_offset, binary_size;
		const struct shader_binary *sb;
		unsigned int num_words;
		struct stream stream;

		binary_size = binary[6];
		binary_offset = binary[7];

		printf("binary @0x%08x\n", binary_offset);

		if (opts.type == CGDRV_SHADER_FRAGMENT) {
			num_words = (binary_size - 16) / 4;

			sb = cg->binary + binary_offset;

			printf("  signature: %.*s\n", 8, sb->signature);
			printf("  unknown1: %08x\n", sb->unknown1);
			printf("  unknown2: %08x\n", sb->unknown2);
			printf("  words:\n");

			stream.words = stream.ptr = sb->words;
			stream.end = stream.ptr + num_words;
			dump_stream(&stream);
		} else {
			const void *binary = cg->binary + cgbin->binary_offset;

			vertex_shader_disassemble(binary, cgbin->binary_size);
		}
	}

#if 0
	for (i = 0; i < cg->binary_size1; i += 4) {
		uint32_t value = *(uint32_t *)(cg->binary1 + i);
		uint8_t *bytes = (uint8_t *)(cg->binary1 + i);

		printf("%08x: %08x |", i, value);

		for (j = 0; j < 4; j++)
			printf(" %02x", bytes[j]);

		printf(" | ");

		for (j = 0; j < 4; j++) {
			if (isprint(bytes[j]))
				printf("%c", bytes[j]);
			else
				printf(".");
		}

		printf(" |\n");
	}

	if (1) {
		const uint32_t *binary = (const uint32_t *)cg->binary1;
		unsigned int offset, size;

		//printf("offset: %x, %u\n", binary[58], binary[58]);
		//printf("size: %x, %u\n", binary[59], binary[59]);

		offset = binary[58] * 4;
		size = binary[59];

		printf("offset: %x, size: %u/%u\n", offset, size, size / 4);
	}
#endif

	CgDrv_CleanUp(cg);

	CgDrv_Delete(cg);
	return 0;
}
