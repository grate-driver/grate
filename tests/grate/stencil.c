/*
 * Copyright (c) 2017 Dmitry Osipenko
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

#include <assert.h>
#include <unistd.h>

#include "grate.h"
#include "tgr_3d.xml.h"

#define CHECK(idx_bo, chk_value)				\
	if (idx_bo) {						\
		grate_3d_draw_elements(ctx,			\
				TGR3D_PRIMITIVE_TYPE_TRIANGLES,	\
				idx_bo, TGR3D_INDEX_MODE_UINT16,\
				ARRAY_SIZE(indices));		\
		grate_flush(grate);				\
		grate_swap_buffers(grate);			\
	}							\
	HOST1X_BO_INVALIDATE(pixbuf->bo,			\
			     pixbuf->bo->offset,		\
			     pixbuf->bo->size);			\
	printf("stencil = 0x%02x %s\n", stencil_data[0],	\
		stencil_data[0] == chk_value ? "OK" : "FAIL");	\
	assert(stencil_data[0] == chk_value);

static const float vertices[] = {
	-1.0f, -1.0f, 0.0f, 1.0f,
	 1.0f, -1.0f, 0.0f, 1.0f,
	 1.0f,  1.0f, 0.0f, 1.0f,
	-1.0f,  1.0f, 0.0f, 1.0f,
};

static const float uv[] = {
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f,
};

static const unsigned short indices[] = {
	0, 1, 2,
	0, 2, 3,
};

static const unsigned short indices_inv[] = {
	2, 1, 0,
	3, 2, 0,
};

int main(int argc, char *argv[])
{
	struct grate_program *program, *program2;
	struct grate_framebuffer *fb;
	struct grate_shader *vs, *fs, *linker;
	struct grate_options options;
	struct grate *grate;
	struct grate_3d_ctx *ctx;
	struct host1x_pixelbuffer *pixbuf;
	struct host1x_bo *indices_bo, *indices_inv_bo;
	struct host1x_bo *bo;
	struct grate_texture *stencil_buffer;
	struct grate_texture *depth_buffer;
	struct grate_texture *text;
	struct grate_font *font;
	volatile uint8_t *stencil_data;
	int location;

	grate_init_data_path(argv[0]);

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

	grate_clear_color(grate, 0.0f, 0.0f, 0.0f, 1.0f);
	grate_bind_framebuffer(grate, fb);
	grate_clear(grate);

	/* Prepare shaders */

	vs = grate_shader_parse_vertex_asm_from_file(
				"tests/grate/asm/stencil_test_vs.txt");
	if (!vs) {
		fprintf(stderr, "stencil_test_vs assembler parse failed\n");
		return 1;
	}

	fs = grate_shader_parse_fragment_asm_from_file(
				"tests/grate/asm/stencil_test_fs.txt");
	if (!fs) {
		fprintf(stderr, "stencil_test_fs assembler parse failed\n");
		return 1;
	}

	linker = grate_shader_parse_linker_asm_from_file(
				"tests/grate/asm/stencil_test_linker.txt");
	if (!linker) {
		fprintf(stderr, "stencil_test_linker assembler parse failed\n");
		return 1;
	}

	program = grate_program_new(grate, vs, fs, linker);
	if (!program) {
		fprintf(stderr, "grate_program_new() failed\n");
		return 1;
	}

	grate_program_link(program);

	fs = grate_shader_parse_fragment_asm_from_file(
				"tests/grate/asm/stencil_test_fs2.txt");
	if (!fs) {
		fprintf(stderr, "stencil_test_fs2 assembler parse failed\n");
		return 1;
	}

	program2 = grate_program_new(grate, vs, fs, linker);
	if (!program2) {
		fprintf(stderr, "grate_program_new() failed\n");
		return 1;
	}

	grate_program_link(program2);

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
	grate_3d_ctx_set_cull_face(ctx, GRATE_3D_CTX_CULL_FACE_NONE);
	grate_3d_ctx_set_scissor(ctx, 0, options.width, 0, options.height);
	grate_3d_ctx_set_point_coord_range(ctx, 0.0f, 1.0f, 0.0f, 1.0f);
	grate_3d_ctx_set_polygon_offset(ctx, 0.0f, 0.0f);
	grate_3d_ctx_set_provoking_vtx_last(ctx, true);

	/* Load font */

	font = grate_create_font(grate, "data/font.png", "data/font.fnt");
	if (!font) {
		fprintf(stderr, "failed to load font\n");
		return 1;
	}

	/* Setup vertices attribute */

	location = grate_get_attribute_location(program, "position");
	bo = grate_create_attrib_bo_from_data(grate, vertices);
	grate_3d_ctx_vertex_attrib_float_pointer(ctx, location, 4, bo);
	grate_3d_ctx_enable_vertex_attrib_array(ctx, location);

	/* Setup texcoord attribute */

	location = grate_get_attribute_location(program, "texcoord");
	bo = grate_create_attrib_bo_from_data(grate, uv);
	grate_3d_ctx_vertex_attrib_float_pointer(ctx, location, 2, bo);
	grate_3d_ctx_enable_vertex_attrib_array(ctx, location);

	/* Setup depth buffer */

	depth_buffer = grate_create_texture(grate,
					    options.width, options.height,
					    PIX_BUF_FMT_D16_LINEAR,
					    PIX_BUF_LAYOUT_TILED_16x16);

	pixbuf = grate_texture_pixbuf(depth_buffer);

	grate_3d_ctx_set_depth_func(ctx, GRATE_3D_CTX_DEPTH_FUNC_ALWAYS);
	grate_3d_ctx_bind_depth_buffer(ctx, pixbuf);
	grate_3d_ctx_perform_depth_test(ctx, true);
	grate_3d_ctx_perform_depth_write(ctx, true);

	/* Setup stencil buffer */

	stencil_buffer = grate_create_texture(grate,
					      options.width, options.height,
					      PIX_BUF_FMT_S8,
					      PIX_BUF_LAYOUT_TILED_16x16);
	pixbuf = grate_texture_pixbuf(stencil_buffer);
	grate_3d_ctx_bind_stencil_buffer(ctx, pixbuf);

	HOST1X_BO_MMAP(pixbuf->bo, (void**)&stencil_data);

	/* Setup render target */

	pixbuf = grate_get_draw_pixbuf(fb);
	grate_3d_ctx_bind_render_target(ctx, 1, pixbuf);
	grate_3d_ctx_enable_render_target(ctx, 1);

	text = grate_create_texture(grate, options.width, options.height,
				    PIX_BUF_FMT_RGBA8888,
				    PIX_BUF_LAYOUT_LINEAR);

	/* Create indices BO */

	indices_bo     = grate_create_attrib_bo_from_data(grate, indices);
	indices_inv_bo = grate_create_attrib_bo_from_data(grate, indices_inv);

	/* Some basic stencil tests */

	pixbuf = grate_texture_pixbuf(stencil_buffer);
	grate_3d_ctx_perform_stencil_test(ctx, true);

	grate_texture_clear(grate, stencil_buffer, 0x77);
	CHECK(NULL, 0x77);

	grate_3d_ctx_set_stencil_func(ctx,
				      GRATE_3D_CTX_STENCIL_TEST_BOTH,
				      GRATE_3D_CTX_STENCIL_TEST_EQUAL,
				      0x78, 0xff);
	grate_3d_ctx_set_stencil_ops(ctx,
				     GRATE_3D_CTX_STENCIL_TEST_BOTH,
				     GRATE_3D_CTX_STENCIL_OP_ZERO,
				     GRATE_3D_CTX_STENCIL_OP_REPLACE,
				     GRATE_3D_CTX_STENCIL_OP_REPLACE);
	CHECK(indices_bo, 0x00);

	grate_3d_ctx_set_stencil_func(ctx,
				      GRATE_3D_CTX_STENCIL_TEST_BOTH,
				      GRATE_3D_CTX_STENCIL_TEST_ALWAYS,
				      0x55, 0xff);
	grate_3d_ctx_set_stencil_ops(ctx,
				     GRATE_3D_CTX_STENCIL_TEST_BOTH,
				     GRATE_3D_CTX_STENCIL_OP_ZERO,
				     GRATE_3D_CTX_STENCIL_OP_REPLACE,
				     GRATE_3D_CTX_STENCIL_OP_REPLACE);
	CHECK(indices_bo, 0x55);

	grate_3d_ctx_set_stencil_func(ctx,
				      GRATE_3D_CTX_STENCIL_TEST_BOTH,
				      GRATE_3D_CTX_STENCIL_TEST_NEVER,
				      0x11, 0xff);
	grate_3d_ctx_set_stencil_ops(ctx,
				     GRATE_3D_CTX_STENCIL_TEST_BOTH,
				     GRATE_3D_CTX_STENCIL_OP_REPLACE,
				     GRATE_3D_CTX_STENCIL_OP_ZERO,
				     GRATE_3D_CTX_STENCIL_OP_ZERO);
	CHECK(indices_bo, 0x11);

	grate_3d_ctx_set_stencil_func(ctx,
				      GRATE_3D_CTX_STENCIL_TEST_BOTH,
				      GRATE_3D_CTX_STENCIL_TEST_EQUAL,
				      0x33, 0x22);
	grate_3d_ctx_set_stencil_ops(ctx,
				     GRATE_3D_CTX_STENCIL_TEST_BOTH,
				     GRATE_3D_CTX_STENCIL_OP_INVERT,
				     GRATE_3D_CTX_STENCIL_OP_REPLACE,
				     GRATE_3D_CTX_STENCIL_OP_REPLACE);
	CHECK(indices_bo, 0xee);

	grate_texture_clear(grate, stencil_buffer, 0x02);
	grate_3d_ctx_set_depth_func(ctx, GRATE_3D_CTX_DEPTH_FUNC_NEVER);

	grate_3d_ctx_set_stencil_func(ctx,
				      GRATE_3D_CTX_STENCIL_TEST_BOTH,
				      GRATE_3D_CTX_STENCIL_TEST_ALWAYS,
				      0x00, 0xff);
	grate_3d_ctx_set_stencil_ops(ctx,
				     GRATE_3D_CTX_STENCIL_TEST_BOTH,
				     GRATE_3D_CTX_STENCIL_OP_ZERO,
				     GRATE_3D_CTX_STENCIL_OP_DECR,
				     GRATE_3D_CTX_STENCIL_OP_ZERO);
	CHECK(indices_bo, 0x01);

	grate_texture_clear(grate, stencil_buffer, 0x00);

	grate_3d_ctx_set_stencil_func(ctx,
				      GRATE_3D_CTX_STENCIL_TEST_BOTH,
				      GRATE_3D_CTX_STENCIL_TEST_ALWAYS,
				      0x00, 0xff);
	grate_3d_ctx_set_stencil_ops(ctx,
				     GRATE_3D_CTX_STENCIL_TEST_BOTH,
				     GRATE_3D_CTX_STENCIL_OP_ZERO,
				     GRATE_3D_CTX_STENCIL_OP_DECR,
				     GRATE_3D_CTX_STENCIL_OP_ZERO);
	CHECK(indices_bo, 0x00);

	grate_3d_ctx_set_stencil_func(ctx,
				      GRATE_3D_CTX_STENCIL_TEST_BOTH,
				      GRATE_3D_CTX_STENCIL_TEST_ALWAYS,
				      0x00, 0xff);
	grate_3d_ctx_set_stencil_ops(ctx,
				     GRATE_3D_CTX_STENCIL_TEST_BOTH,
				     GRATE_3D_CTX_STENCIL_OP_ZERO,
				     GRATE_3D_CTX_STENCIL_OP_DECR_WRAP,
				     GRATE_3D_CTX_STENCIL_OP_ZERO);
	CHECK(indices_bo, 0xff);

	grate_3d_ctx_set_depth_func(ctx, GRATE_3D_CTX_DEPTH_FUNC_ALWAYS);

	grate_3d_ctx_set_stencil_func(ctx,
				      GRATE_3D_CTX_STENCIL_TEST_BOTH,
				      GRATE_3D_CTX_STENCIL_TEST_ALWAYS,
				      0x00, 0xff);
	grate_3d_ctx_set_stencil_ops(ctx,
				     GRATE_3D_CTX_STENCIL_TEST_BOTH,
				     GRATE_3D_CTX_STENCIL_OP_ZERO,
				     GRATE_3D_CTX_STENCIL_OP_ZERO,
				     GRATE_3D_CTX_STENCIL_OP_INCR_WRAP);
	CHECK(indices_bo, 0x00);

	grate_3d_ctx_set_stencil_func(ctx,
				      GRATE_3D_CTX_STENCIL_TEST_FRONT,
				      GRATE_3D_CTX_STENCIL_TEST_GEQUAL,
				      0x01, 0xff);
	grate_3d_ctx_set_stencil_ops(ctx,
				     GRATE_3D_CTX_STENCIL_TEST_FRONT,
				     GRATE_3D_CTX_STENCIL_OP_ZERO,
				     GRATE_3D_CTX_STENCIL_OP_ZERO,
				     GRATE_3D_CTX_STENCIL_OP_INVERT);
	grate_3d_ctx_set_stencil_func(ctx,
				      GRATE_3D_CTX_STENCIL_TEST_BACK,
				      GRATE_3D_CTX_STENCIL_TEST_GREATER,
				      0x00, 0xff);
	grate_3d_ctx_set_stencil_ops(ctx,
				     GRATE_3D_CTX_STENCIL_TEST_BACK,
				     GRATE_3D_CTX_STENCIL_OP_ZERO,
				     GRATE_3D_CTX_STENCIL_OP_ZERO,
				     GRATE_3D_CTX_STENCIL_OP_ZERO);
	CHECK(indices_bo, 0xFF);

	grate_3d_ctx_set_stencil_func(ctx,
				      GRATE_3D_CTX_STENCIL_TEST_FRONT,
				      GRATE_3D_CTX_STENCIL_TEST_ALWAYS,
				      0x00, 0xff);
	grate_3d_ctx_set_stencil_ops(ctx,
				     GRATE_3D_CTX_STENCIL_TEST_FRONT,
				     GRATE_3D_CTX_STENCIL_OP_KEEP,
				     GRATE_3D_CTX_STENCIL_OP_KEEP,
				     GRATE_3D_CTX_STENCIL_OP_KEEP);
	grate_3d_ctx_set_stencil_func(ctx,
				      GRATE_3D_CTX_STENCIL_TEST_BACK,
				      GRATE_3D_CTX_STENCIL_TEST_LESS,
				      0x00, 0xff);
	grate_3d_ctx_set_stencil_ops(ctx,
				     GRATE_3D_CTX_STENCIL_TEST_BACK,
				     GRATE_3D_CTX_STENCIL_OP_ZERO,
				     GRATE_3D_CTX_STENCIL_OP_ZERO,
				     GRATE_3D_CTX_STENCIL_OP_INVERT);
	CHECK(indices_inv_bo, 0x00);

	/* Draw white-on-black text to the texture */

	grate_texture_clear(grate, text, 0x00000000);

	pixbuf = grate_texture_pixbuf(text);
	grate_3d_ctx_bind_render_target(ctx, 1, pixbuf);

	grate_3d_printf(grate, ctx, font, 1, -0.85f, -0.15f,
			options.width / 200.0f, "Stencil test works!");
	grate_flush(grate);

	/*
	 * Fill the stencil buffer with the text pattern.
	 * The stencil_test_fs2.txt discards black-colored texels.
	 */

	grate_3d_ctx_bind_program(ctx, program2);

	pixbuf = grate_get_draw_pixbuf(fb);
	grate_3d_ctx_bind_render_target(ctx, 1, pixbuf);

	grate_texture_clear(grate, stencil_buffer, 0x00);

	grate_3d_ctx_set_stencil_func(ctx,
				      GRATE_3D_CTX_STENCIL_TEST_BOTH,
				      GRATE_3D_CTX_STENCIL_TEST_NEVER,
				      0xAB, 0xA0);
	grate_3d_ctx_set_stencil_ops(ctx,
				     GRATE_3D_CTX_STENCIL_TEST_BOTH,
				     GRATE_3D_CTX_STENCIL_OP_REPLACE,
				     GRATE_3D_CTX_STENCIL_OP_ZERO,
				     GRATE_3D_CTX_STENCIL_OP_ZERO);

	grate_3d_ctx_bind_texture(ctx, 0, text);

	grate_3d_draw_elements(ctx, TGR3D_PRIMITIVE_TYPE_TRIANGLES,
			       indices_bo, TGR3D_INDEX_MODE_UINT16,
			       ARRAY_SIZE(indices));
	grate_flush(grate);

	/*
	 * Now draw a blue quad that fits whole screen and drawn only
	 * in the stencil-set area; i.e. blue-colored "Stencil test works!"
	 * must be displayed. The blue color comes from the stencil_test_fs.txt
	 * shader.
	 */
	grate_clear(grate);

	grate_3d_ctx_bind_program(ctx, program);

	grate_3d_ctx_set_stencil_func(ctx,
				      GRATE_3D_CTX_STENCIL_TEST_BOTH,
				      GRATE_3D_CTX_STENCIL_TEST_EQUAL,
				      0xAB, 0xff);

	grate_3d_draw_elements(ctx, TGR3D_PRIMITIVE_TYPE_TRIANGLES,
			       indices_bo, TGR3D_INDEX_MODE_UINT16,
			       ARRAY_SIZE(indices));
	grate_flush(grate);

	grate_swap_buffers(grate);
	grate_wait_for_key(grate);

	grate_exit(grate);
	return 0;
}
