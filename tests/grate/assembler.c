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
	char *path;
	uint32_t expected_result;
} tests[] = {
	{ "asm/vs_mov.txt", 0xFF01FF00 },
	{ "asm/vs_constant.txt", 0x664D331A },
	{ "asm/vs_constant_relative_addressing.txt", 0x664D331A },
	{ "asm/vs_attribute_relative_addressing.txt", 0xFF01FF00 },
	{ "asm/vs_export_relative_addressing.txt", 0x6601FF1A },
	{ "asm/vs_branching.txt", 0xFF01FF00 },
	{ "asm/vs_function.txt", 0xFF01FF00 },
	{ "asm/vs_stack.txt", 0xFF01FF00 },
	{ "asm/vs_predicate.txt", 0x667F007F },
};

static const char *fragment_shader[] = {
	"precision mediump float;\n",
	"varying vec4 vcolor;\n",
	"\n",
	"void main()\n",
	"{\n",
	"    gl_FragColor = vcolor;\n",
	"}"
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

int main(int argc, char *argv[])
{
	struct grate_program *program;
	struct grate_framebuffer *fb;
	struct grate_shader *vs, *fs;
	struct grate_options options;
	unsigned long offset = 0;
	struct grate *grate;
	struct grate_bo *bo;
	struct stat sb;
	void *vertex_shader;
	void *buffer;
	uint32_t *fb_data;
	uint32_t result;
	char *path;
	int location;
	int i, j;
	int fd;

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

		fd = open(tests[i].path, O_RDONLY);
		if (fd == -1) {
			fprintf(stderr, "failed to open %s: %s\n",
				tests[i].path, strerror(errno));
			return 1;
		}

		if (fstat(fd, &sb) == -1) {
			fprintf(stderr, "failed to get stat %s: %s\n",
				tests[i].path, strerror(errno));
			return 1;
		}

		vertex_shader = mmap(NULL, sb.st_size, PROT_READ,
				     MAP_PRIVATE, fd, 0);

		if (vertex_shader == MAP_FAILED) {
			fprintf(stderr, "failed to mmap %s: %s\n",
				tests[i].path, strerror(errno));
			return 1;
		}

		vs = grate_shader_parse_asm(vertex_shader);
		if (!vs) {
			fprintf(stderr, "%s assembler parse failed\n",
				tests[i].path);
			return 1;
		}

		fs = grate_shader_new(grate, GRATE_SHADER_FRAGMENT,
				      fragment_shader, ARRAY_SIZE(fragment_shader));
		if (!fs) {
			fprintf(stderr, "fragment shader compilation failed\n");
			return 1;
		}

		program = grate_program_new(grate, vs, fs);
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

			fprintf(stderr, "\nDisassembly:\n%s\n",
				grate_shader_disasm_vs(vs) ?: "");

			fprintf(stderr, "\ntest %s failed: expected 0x%08X, got 0x%08X\n",
				tests[i].path, tests[i].expected_result, result);

			return 1;
		}

		grate_program_free(program);

		if (munmap(vertex_shader, sb.st_size) == -1) {
			fprintf(stderr, "failed to munmap %s: %s\n",
				tests[i].path, strerror(errno));
		}

		close(fd);
	}

	printf("All tests passed\n");

	grate_exit(grate);
	return 0;
}
