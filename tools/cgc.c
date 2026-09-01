#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libcgc.h"

struct opts {
	enum cgc_shader_type type;
	bool help;
	unsigned flags;
};

static void usage(FILE *fp, const char *program)
{
	fprintf(fp, "usage: %s [options] FILE\n", program);
	fprintf(fp, "Options:\n");
	fprintf(fp, "  -h, --help           Show this help message\n");
	fprintf(fp, "  -F, --fragment       Compile fragment-shader\n");
	fprintf(fp, "  -V, --vertex         Compile vertex-shader\n");
	fprintf(fp, "  -v, --verbose        Verbose output\n");
	fprintf(fp, "  -b, --binary         Print binary\n");
	fprintf(fp, "  -s, --stream         Print stream\n");
	fprintf(fp, "  -H, --header         Print header\n");
	fprintf(fp, "  -a, --attribs        Print attribs\n");
	fprintf(fp, "  -u, --uniforms       Print uniforms\n");
	fprintf(fp, "  -c, --constants      Print constants\n");
	fprintf(fp, "  -r, --raw            Print raw instruction bits\n");
	fprintf(fp, "  -U, --unknown        Print unknown instuction bits\n");
	fprintf(fp, "  -g, --gpr            Track GPR reads/writes\n");
	fprintf(fp, "  -R, --regs           Print command stream register writes\n");
}

static int parse_command_line(struct opts *opts, int argc, char *argv[])
{
	static const struct option options[] = {
		{ "help", 0, NULL, 'h' },
		{ "fragment", 0, NULL, 'F' },
		{ "vertex", 0, NULL, 'V' },
		{ "verbose", 0, NULL, 'v' },
		{ "binary", 0, NULL, 'b' },
		{ "stream", 0, NULL, 's' },
		{ "header", 0, NULL, 'H' },
		{ "attribs", 0, NULL, 'a' },
		{ "uniforms", 0, NULL, 'u' },
		{ "constants", 0, NULL, 'c' },
		{ "raw", 0, NULL, 'r' },
		{ "unknown", 0, NULL, 'U' },
		{ "gpr", 0, NULL, 'g' },
		{ "regs", 0, NULL, 'R' },
		{ NULL, 0, NULL, 0 }
	};
	int opt;

	memset(opts, 0, sizeof(*opts));
	opts->type = CGC_SHADER_FRAGMENT;

	while ((opt = getopt_long(argc, argv, "FhvAbsHaucrUgR", options, NULL)) != -1) {
		switch (opt) {
		case 'h':
			opts->help = true;
			break;

		case 'F':
			opts->type = CGC_SHADER_FRAGMENT;
			break;

		case 'V':
			opts->type = CGC_SHADER_VERTEX;
			break;

		case 'v':
			opts->flags = ~0;
			break;

		case 'b':
			opts->flags |= DUMP_BIN;
			break;

		case 's':
			opts->flags |= DUMP_STR;
			break;

		case 'H':
			opts->flags |= DUMP_HDR;
			break;

		case 'a':
			opts->flags |= DUMP_ATT;
			break;

		case 'u':
			opts->flags |= DUMP_UNI;
			break;

		case 'c':
			opts->flags |= DUMP_CONST;
			break;

		case 'r':
			opts->flags |= DUMP_RAW;
			break;

		case 'U':
			opts->flags |= DUMP_UNK;
			break;

		case 'g':
			opts->flags |= DUMP_GPR;
			break;

		case 'R':
			opts->flags |= DUMP_REG;
			break;

		default:
			fprintf(stderr, "invalid option '%c'\n", opt);
			return -1;
		}
	}

	return optind;
}

/**
 * This only works if the code is provided through static storage. Allocating
 * memory and passing the pointer into CgDrv_Compile() will segfault.
 */
int main(int argc, char *argv[])
{
	struct cgc_shader *shader;
	size_t length;
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

	shader = cgc_compile(opts.type, code, length);
	if (shader) {
		cgc_shader_dump(shader, stdout, opts.flags);
		cgc_shader_free(shader);
	}

	return 0;
}
