#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libcgc.h"

#define BIT(x) (1 << (x))

#if 0
struct shader_binary {
	char signature[8];
	uint32_t unknown1;
	uint32_t unknown2;
	uint32_t words[0];
};
#endif

struct opts {
	enum cgc_shader_type type;
	bool version;
	bool help;
};

#if 0
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
#endif

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
	opts->type = CGC_SHADER_FRAGMENT;

	while ((opt = getopt_long(argc, argv, "FhvV", options, NULL)) != -1) {
		switch (opt) {
		case 'F':
			opts->type = CGC_SHADER_FRAGMENT;
			break;

		case 'h':
			opts->help = true;
			break;

		case 'v':
			opts->type = CGC_SHADER_VERTEX;
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

#if 0
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
#endif

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
	struct cgc_shader *shader;
	size_t length, i;
	struct opts opts;
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

	printf("Compiling shader (%zu bytes)...\n", length);
	fputs("| ", stdout);

	for (i = 0; i < length; i++) {
		fputc(code[i], stdout);

		if (code[i] == '\n' && i != length - 1)
			fputs("| ", stdout);
	}

	shader = cgc_compile(opts.type, code, length);
	if (shader) {
		cgc_shader_dump(shader, stdout);
		cgc_shader_free(shader);
	}

	return 0;
}
