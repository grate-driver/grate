#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

struct opts {
	enum cgdrv_shader_type type;
	bool version;
	bool help;
};

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

static void usage(FILE *fp, const char *program)
{
	fprintf(fp, "usage: %s [options] FILE\n", program);
}

static int parse_command_line(struct opts *opts, int argc, char *argv[])
{
	static const struct option options[] = {
		{ "help", 0, NULL, 'h' },
		{ "version", 0, NULL, 'V' },
		{ NULL, 0, NULL, 0 }
	};
	int opt;

	memset(opts, 0, sizeof(*opts));
	opts->type = CGDRV_SHADER_VERTEX;

	while ((opt = getopt_long(argc, argv, "hV", options, NULL)) != -1) {
		switch (opt) {
		case 'h':
			opts->help = true;
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
	struct opts opts;
	struct cgdrv *cg;
	char code[65536];
	size_t length, i;
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

	if (err <= argc) {
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

		if (code[i] == '\n')
			fputs("| ", stdout);
	}

	err = CgDrv_Compile(cg, 1, opts.type, code, length, 0);
	printf("CgDrv_Compile(): %d\n", err);
	if (err) {
		fprintf(stderr, "%s\n", cg->error);
		CgDrv_Delete(cg);
		return 1;
	}

	printf("%s\n", cg->log);

	printf("Binary: %zu bytes\n", cg->binary_size);
	print_hexdump(stdout, DUMP_PREFIX_OFFSET, NULL, cg->binary,
		      cg->binary_size, 16, true);
	printf("Binary: %zu bytes\n", cg->binary_size1);
	print_hexdump(stdout, DUMP_PREFIX_OFFSET, NULL, cg->binary1,
		      cg->binary_size1, 16, true);

	CgDrv_CleanUp(cg);

	CgDrv_Delete(cg);
	return 0;
}
