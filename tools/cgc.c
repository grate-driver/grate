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

struct opts {
	enum cgc_shader_type type;
	bool version;
	bool help;
};

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
