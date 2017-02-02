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
#include <getopt.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "grate.h"
#include "tgr_3d.xml.h"

struct vs_asm_test {
	char *vs_path;
	char *fs_path;
	char *linker_path;
	uint32_t expected_result;
	bool has_expected;
	bool test_only;
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

static void * open_file(const char *path, const char *desc)
{
	struct stat sb;
	int fd;
	void *ret;

	fd = open(path, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "failed to open %s %s: %s\n",
			desc, path, strerror(errno));
		return NULL;
	}

	if (fstat(fd, &sb) == -1) {
		fprintf(stderr, "failed to get stat %s: %s\n",
			path, strerror(errno));
		return NULL;
	}

	ret = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (ret == MAP_FAILED) {
		fprintf(stderr, "failed to mmap %s: %s\n",
			path, strerror(errno));
		return NULL;
	}

	return ret;
}

static int parse_command_line(struct vs_asm_test *test, int argc, char *argv[])
{
	int c;

	memset(test, 0, sizeof(*test));

	do {
		struct option long_options[] =
		{
			{"expected",	required_argument, NULL, 0},
			{"vs",		required_argument, NULL, 0},
			{"fs",		required_argument, NULL, 0},
			{"lnk",		required_argument, NULL, 0},
			{"testonly",	no_argument, NULL, 0},
			{ /* Sentinel */ }
		};
		int option_index = 0;

		c = getopt_long(argc, argv, "h:", long_options, &option_index);

		switch (c) {
		case 0:
			switch (option_index) {
			case 0:
				errno = 0;
				sscanf(optarg, "0x%X", &test->expected_result);
				if (errno != 0) {
					fprintf(stderr, "failed to parse \"expected\" argument\n");
					return 0;
				}
				test->has_expected = true;
				break;
			case 1:
				test->vs_path = optarg;
				break;
			case 2:
				test->fs_path = optarg;
				break;
			case 3:
				test->linker_path = optarg;
				break;
			case 4:
				test->test_only = true;
				break;
			default:
				return 0;
			}
			break;
		case -1:
			break;
		default:
			fprintf(stderr, "Invalid arguments\n\n");
		case 'h':
			fprintf(stderr, "Valid arguments:\n");
			fprintf(stderr, "\t--vs path : vertex asm path\n");
			fprintf(stderr, "\t--fs path : fragment asm path\n");
			fprintf(stderr, "\t--lnk path : linker asm path\n");
			fprintf(stderr, "\t--expected 0x00000000 : perform the test\n");
			fprintf(stderr, "\t--testonly : don't show the rendered result\n");
			fprintf(stderr, "\t-h\n");
			return 0;
		}
	} while (c != -1);

	return 1;
}

static void dump_asm(struct grate_shader *vs,
		     struct grate_shader *fs,
		     struct grate_shader *linker)
{
	fprintf(stderr, "\nVertex disassembly:\n%s\n",
		grate_shader_disasm_vs(vs) ?: "");

	fprintf(stderr, "\nFragment disassembly:\n%s\n",
		grate_shader_disasm_fs(fs) ?: "");

	fprintf(stderr, "\nLinker disassembly:\n%s\n",
		grate_shader_disasm_linker(linker) ?: "");
}

int main(int argc, char *argv[])
{
	struct vs_asm_test test;
	struct grate_program *program;
	struct grate_framebuffer *fb;
	struct grate_shader *vs, *fs, *linker;
	struct grate_options options;
	struct grate *grate;
	struct grate_3d_ctx *ctx;
	struct host1x_pixelbuffer *pixbuf;
	struct host1x_bo *bo;
	void *vertex_shader;
	void *fragment_shader;
	void *linker_code;
	uint32_t *fb_data;
	uint32_t result;
	int ret = 0;
	int location;
	int i;

	if (!parse_command_line(&test, argc, argv))
		return 1;

	if (!grate_parse_command_line(&options, argc, argv))
		return 1;

	grate = grate_init(&options);
	if (!grate)
		return 1;

	fb = grate_framebuffer_create(grate, options.width, options.height,
				      PIX_BUF_FMT_RGBA8888,
				      PIX_BUF_LAYOUT_TILED_16x16,
				      GRATE_SINGLE_BUFFERED);
	if (!fb)
		return 1;

	fb_data = grate_framebuffer_data(fb, true);
	if (!fb_data)
		return 1;

	grate_clear_color(grate, 0.0f, 0.0f, 0.0f, 1.0f);
	grate_bind_framebuffer(grate, fb);
	grate_clear(grate);

	/* Prepare shaders */

	vertex_shader = open_file(test.vs_path, "vertex shader");
	if (vertex_shader == NULL)
		return 1;

	fragment_shader = open_file(test.fs_path, "fragment shader");
	if (fragment_shader == NULL)
		return 1;

	linker_code = open_file(test.linker_path, "shader linker");
	if (linker_code == NULL)
		return 1;

	vs = grate_shader_parse_vertex_asm(vertex_shader);
	if (!vs) {
		fprintf(stderr, "%s assembler parse failed\n",
			test.vs_path);
		return 1;
	}

	fs = grate_shader_parse_fragment_asm(fragment_shader);
	if (!fs) {
		fprintf(stderr, "%s assembler parse failed\n",
			test.fs_path);
		return 1;
	}

	linker = grate_shader_parse_linker_asm(linker_code);
	if (!linker) {
		fprintf(stderr, "%s assembler parse failed\n",
			test.linker_path);
		return 1;
	}

	program = grate_program_new(grate, vs, fs, linker);
	grate_program_link(program);

	/* Setup context */

	ctx = grate_3d_alloc_ctx(grate);

	grate_3d_ctx_bind_program(ctx, program);
	grate_3d_ctx_set_depth_range(ctx, 0.0f, 1.0f);
	grate_3d_ctx_set_dither(ctx, 0x779);
	grate_3d_ctx_set_point_params(ctx, 0x1401);
	grate_3d_ctx_set_point_size(ctx, 1.0f);
	grate_3d_ctx_set_line_params(ctx, 0x2);
	grate_3d_ctx_set_line_width(ctx, 1.0f);
	grate_3d_ctx_set_viewport_bias(ctx, 0.0f, 0.0f, 0.5f);
	grate_3d_ctx_set_viewport_scale(ctx, options.width, options.height, 0.5f);
	grate_3d_ctx_use_guardband(ctx, true);
	grate_3d_ctx_set_front_direction_is_cw(ctx, false);
	grate_3d_ctx_set_cull_ccw(ctx, false);
	grate_3d_ctx_set_cull_cw(ctx, false);
	grate_3d_ctx_set_scissor(ctx, 0, options.width, 0, options.height);
	grate_3d_ctx_set_point_coord_range(ctx, 0.0f, 1.0f, 0.0f, 1.0f);
	grate_3d_ctx_set_polygon_offset(ctx, 0.0f, 0.0f);
	grate_3d_ctx_set_provoking_vtx_last(ctx, true);

	/* Setup vertices attribute */

	location = grate_get_attribute_location(program, "position");
	bo = grate_bo_create_from_data(grate, sizeof(vertices), 4, vertices);
	grate_3d_ctx_vertex_attrib_pointer(ctx, location, 4,
					   ATTRIB_TYPE_FLOAT32,
				           4 * sizeof(float), bo);
	grate_3d_ctx_enable_vertex_attrib_array(ctx, location);

	/* Setup colors attribute */

	location = grate_get_attribute_location(program, "color");
	bo = grate_bo_create_from_data(grate, sizeof(colors), 4, colors);
	grate_3d_ctx_vertex_attrib_pointer(ctx, location, 4,
					   ATTRIB_TYPE_FLOAT32,
				           4 * sizeof(float), bo);
	grate_3d_ctx_enable_vertex_attrib_array(ctx, location);

	/* Setup render target */

	pixbuf = grate_get_draw_pixbuf(fb);
	grate_3d_ctx_bind_render_target(ctx, 1, pixbuf);
	grate_3d_ctx_enable_render_target(ctx, 1);

	/* Create indices BO */

	bo = grate_bo_create_from_data(grate, sizeof(indices), 4, indices);

	if (!test.test_only) {
		dump_asm(vs, fs, linker);
	}

	grate_3d_draw_elements(ctx, PRIMITIVE_TYPE_TRIANGLES,
			       bo, INDEX_MODE_UINT16,
			       ARRAY_SIZE(indices));
	grate_flush(grate);

	result = fb_data[0];

	if (test.has_expected && test.expected_result != result) {
		for (i = 0; i < options.width * options.height; i += 4)
			fprintf(stderr, "%d: 0x%08X 0x%08X 0x%08X 0x%08X\n", i,
				fb_data[i], fb_data[i + 1],
				fb_data[i + 2], fb_data[i + 3]);

		dump_asm(vs, fs, linker);

		fprintf(stderr, "\ntest %s; %s; %s; failed: expected 0x%08X, got 0x%08X\n",
			test.vs_path,
			test.fs_path,
			test.linker_path,
			test.expected_result, result);

		ret = 1;
	}

	if (!test.test_only) {
		grate_swap_buffers(grate);
		grate_wait_for_key(grate);
	}

	grate_exit(grate);
	return ret;
}
