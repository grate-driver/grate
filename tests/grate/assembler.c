/*
 * Copyright (c) 2016 Dmitry Osipenko
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

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "grate.h"

static const struct vs_asm_test {
	char *vs_path;
	char *fs_path;
	char *linker_path;
	uint32_t expected_result;
} tests[] = {
	{
		.vs_path = "asm/vs_mov.txt",
		.fs_path = "asm/fs_vs_tests.txt",
		.linker_path = "asm/linker_vs_tests.txt",
		.expected_result = 0xFF01FF00,
	},
	{
		.vs_path = "asm/vs_constant.txt",
		.fs_path = "asm/fs_vs_tests.txt",
		.linker_path = "asm/linker_vs_tests.txt",
		.expected_result = 0x664D331A,
	},
	{
		.vs_path = "asm/vs_constant_relative_addressing.txt",
		.fs_path = "asm/fs_vs_tests.txt",
		.linker_path = "asm/linker_vs_tests.txt",
		.expected_result = 0x664D331A,
	},
	{
		.vs_path = "asm/vs_attribute_relative_addressing.txt",
		.fs_path = "asm/fs_vs_tests.txt",
		.linker_path = "asm/linker_vs_tests.txt",
		.expected_result = 0xFF01FF00,
	},
	{
		.vs_path = "asm/vs_export_relative_addressing.txt",
		.fs_path = "asm/fs_vs_tests.txt",
		.linker_path = "asm/linker_vs_tests.txt",
		.expected_result = 0x6601FF1A,
	},
	{
		.vs_path = "asm/vs_branching.txt",
		.fs_path = "asm/fs_vs_tests.txt",
		.linker_path = "asm/linker_vs_tests.txt",
		.expected_result = 0xFF01FF00,
	},
	{
		.vs_path = "asm/vs_function.txt",
		.fs_path = "asm/fs_vs_tests.txt",
		.linker_path = "asm/linker_vs_tests.txt",
		.expected_result = 0xFF01FF00,
	},
	{
		.vs_path = "asm/vs_stack.txt",
		.fs_path = "asm/fs_vs_tests.txt",
		.linker_path = "asm/linker_vs_tests.txt",
		.expected_result = 0xFF01FF00,
	},
	{
		.vs_path = "asm/vs_predicate.txt",
		.fs_path = "asm/fs_vs_tests.txt",
		.linker_path = "asm/linker_vs_tests.txt",
		.expected_result = 0x667F007F,
	},
};

static const float vertices[] = {
	-1.0f,  1.0f, 0.0f, 1.0f,
	-1.0f, -1.0f, 0.0f, 1.0f,
	 1.0f,  1.0f, 0.0f, 1.0f,
	 1.0f, -1.0f, 0.0f, 1.0f,
};

static const float colors[] = {
	1.0f, 0.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f, 1.0f,
	1.0f, 0.0f, 0.0f, 1.0f,
};

static const unsigned short indices[] = {
	0, 1, 2, 1, 2, 3,
};

static void * open_file(int *fd, struct stat *sb, const char *path)
{
	void *ret;

	*fd = open(path, O_RDONLY);
	if (*fd == -1) {
		fprintf(stderr, "failed to open %s: %s\n",
			path, strerror(errno));
		return NULL;
	}

	if (fstat(*fd, sb) == -1) {
		fprintf(stderr, "failed to get stat %s: %s\n",
			path, strerror(errno));
		return NULL;
	}

	ret = mmap(NULL, sb->st_size, PROT_READ, MAP_PRIVATE, *fd, 0);
	if (ret == MAP_FAILED) {
		fprintf(stderr, "failed to mmap %s: %s\n",
			path, strerror(errno));
		return NULL;
	}

	return ret;
}

int main(int argc, char *argv[])
{
	struct grate_program *program;
	struct grate_framebuffer *fb;
	struct grate_shader *vs, *fs, *linker;
	struct grate_options options;
	unsigned long offset = 0;
	struct grate *grate;
	struct grate_bo *bo;
	struct stat sb[3];
	void *vertex_shader;
	void *fragment_shader;
	void *linker_code;
	void *buffer;
	uint32_t *fb_data;
	uint32_t result;
	char *path;
	int location;
	int i, j;
	int fd[3];

	path = dirname(argv[0]);
	if (chdir(path) == -1)
		fprintf(stderr, "failed to chdir to %s\n", path);

	if (!grate_parse_command_line(&options, argc, argv))
		return 1;

	grate = grate_init(&options);
	if (!grate)
		return 1;

	bo = grate_bo_create(grate, 4096, 0);
	if (!bo)
		return 1;

	buffer = grate_bo_map(bo);
	if (!buffer)
		return 1;

	fb = grate_framebuffer_create(grate, options.width, options.height,
				      GRATE_RGBA8888, GRATE_DOUBLE_BUFFERED);
	if (!fb)
		return 1;

	fb_data = grate_framebuffer_data(fb, false);
	if (!fb_data)
		return 1;

	grate_clear_color(grate, 0.0f, 0.0f, 0.0f, 1.0f);
	grate_bind_framebuffer(grate, fb);

	for (i = 0; i < ARRAY_SIZE(tests); i++, offset = 0) {
		grate_clear(grate);

		vertex_shader = open_file(&fd[0], &sb[0], tests[i].vs_path);
		if (vertex_shader == NULL)
			return 1;

		fragment_shader = open_file(&fd[1], &sb[1], tests[i].fs_path);
		if (fragment_shader == NULL)
			return 1;

		linker_code = open_file(&fd[2], &sb[2], tests[i].linker_path);
		if (linker_code == NULL)
			return 1;

		vs = grate_shader_parse_vertex_asm(vertex_shader);
		if (!vs) {
			fprintf(stderr, "%s assembler parse failed\n",
				tests[i].vs_path);
			return 1;
		}

		fs = grate_shader_parse_fragment_asm(fragment_shader);
		if (!fs) {
			fprintf(stderr, "%s assembler parse failed\n",
				tests[i].fs_path);
			return 1;
		}

		linker = grate_shader_parse_linker_asm(linker_code);
		if (!linker) {
			fprintf(stderr, "%s assembler parse failed\n",
				tests[i].linker_path);
			return 1;
		}

		program = grate_program_new(grate, vs, fs, linker);
		grate_program_link(program);

		grate_viewport(grate, 0.0f, 0.0f, options.width, options.height);
		grate_use_program(grate, program);

		location = grate_get_attribute_location(grate, "position");
		if (location < 0) {
			fprintf(stderr, "\"position\": attribute not found\n");
			return 1;
		}

		memcpy(buffer + offset, vertices, sizeof(vertices));
		grate_attribute_pointer(grate, location, sizeof(float), 4, 4,
					bo, offset);
		offset += sizeof(vertices);

		location = grate_get_attribute_location(grate, "color");
		if (location < 0) {
			fprintf(stderr, "\"color\": attribute not found\n");
			return 1;
		}

		memcpy(buffer + offset, colors, sizeof(colors));
		grate_attribute_pointer(grate, location, sizeof(float), 4, 4,
					bo, offset);
		offset += sizeof(colors);

		memcpy(buffer + offset, indices, sizeof(indices));
		grate_draw_elements(grate, GRATE_TRIANGLES, 2, 6, bo, offset);
		grate_flush(grate);

		result = fb_data[0];

		if (tests[i].expected_result != result) {
			for (j = 0; j < options.width * options.height; j += 4)
				fprintf(stderr, "%d: 0x%08X 0x%08X 0x%08X 0x%08X\n", j,
					fb_data[j], fb_data[j + 1],
					fb_data[j + 2], fb_data[j + 3]);

			fprintf(stderr, "\nVertex disassembly:\n%s\n",
				grate_shader_disasm_vs(vs) ?: "");

			fprintf(stderr, "\nFragment disassembly:\n%s\n",
				grate_shader_disasm_fs(fs) ?: "");

			fprintf(stderr, "\nLinker disassembly:\n%s\n",
				grate_shader_disasm_linker(linker) ?: "");

			fprintf(stderr, "\ntest %s; %s; %s; failed: expected 0x%08X, got 0x%08X\n",
				tests[i].vs_path,
				tests[i].fs_path,
				tests[i].linker_path,
				tests[i].expected_result, result);

			return 1;
		}

		grate_program_free(program);

		if (munmap(vertex_shader, sb[0].st_size) == -1) {
			fprintf(stderr, "failed to munmap %s: %s\n",
				tests[i].vs_path, strerror(errno));
		}

		if (munmap(fragment_shader, sb[1].st_size) == -1) {
			fprintf(stderr, "failed to munmap %s: %s\n",
				tests[i].fs_path, strerror(errno));
		}

		if (munmap(linker_code, sb[2].st_size) == -1) {
			fprintf(stderr, "failed to munmap %s: %s\n",
				tests[i].linker_path, strerror(errno));
		}

		close(fd[0]);
		close(fd[1]);
		close(fd[2]);
	}

	printf("\nAll tests passed\n");

	grate_exit(grate);
	return 0;
}
