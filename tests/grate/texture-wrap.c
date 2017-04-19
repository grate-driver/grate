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

#include <libgen.h>
#include <string.h>
#include <unistd.h>

#include "grate.h"
#include "tgr_3d.xml.h"

static const float vertices[] = {
	-0.5f, -0.5f, 0.0f, 1.0f,
	 0.5f, -0.5f, 0.0f, 1.0f,
	 0.5f,  0.5f, 0.0f, 1.0f,
	-0.5f,  0.5f, 0.0f, 1.0f,
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

union offset_u {
	float f[2];

	struct {
		float x, y;
	};
};

int main(int argc, char *argv[])
{
	struct grate_program *program;
	struct grate_framebuffer *fb;
	struct grate_shader *vs, *fs, *linker;
	struct grate_options options;
	struct grate *grate;
	struct grate_3d_ctx *ctx;
	struct host1x_pixelbuffer *pixbuf;
	struct host1x_bo *bo;
	struct grate_texture *texture1, *texture2;
	union offset_u offset_uniform;
	int vtx_offset_loc, tex_offset_loc, tex_scale_loc;
	int location;

	if (chdir( dirname(argv[0]) ) == -1)
		fprintf(stderr, "chdir failed\n");

	if (chdir("../../") == -1)
		fprintf(stderr, "chdir failed\n");

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
				"tests/grate/asm/texture_wrap_vs.txt");
	if (!vs) {
		fprintf(stderr, "texture_wrap_vs assembler parse failed\n");
		return 1;
	}

	fs = grate_shader_parse_fragment_asm_from_file(
				"tests/grate/asm/texture_wrap_fs.txt");
	if (!fs) {
		fprintf(stderr, "texture_wrap_fs assembler parse failed\n");
		return 1;
	}

	linker = grate_shader_parse_linker_asm_from_file(
				"tests/grate/asm/texture_wrap_linker.txt");
	if (!linker) {
		fprintf(stderr, "texture_wrap_linker assembler parse failed\n");
		return 1;
	}

	program = grate_program_new(grate, vs, fs, linker);
	if (!program) {
		fprintf(stderr, "grate_program_new() failed\n");
		return 1;
	}

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
	grate_3d_ctx_set_cull_face(ctx, GRATE_3D_CTX_CULL_FACE_NONE);
	grate_3d_ctx_set_scissor(ctx, 0, options.width, 0, options.height);
	grate_3d_ctx_set_point_coord_range(ctx, 0.0f, 1.0f, 0.0f, 1.0f);
	grate_3d_ctx_set_polygon_offset(ctx, 0.0f, 0.0f);
	grate_3d_ctx_set_provoking_vtx_last(ctx, true);

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

	/* Setup textures */

	texture1 = grate_create_texture(grate, 512, 512,
					PIX_BUF_FMT_RGBA8888,
					PIX_BUF_LAYOUT_LINEAR);
	grate_texture_load(grate, texture1, "data/tegra.png");

	texture2 = grate_create_texture(grate, 128, 128,
					PIX_BUF_FMT_RGBA8888,
				 PIX_BUF_LAYOUT_LINEAR);
	grate_texture_load(grate, texture2, "data/checkerboard.png");

	/* Setup render target */

	pixbuf = grate_get_draw_pixbuf(fb);
	grate_3d_ctx_bind_render_target(ctx, 1, pixbuf);
	grate_3d_ctx_enable_render_target(ctx, 1);

	/* Create indices BO */

	bo = grate_create_attrib_bo_from_data(grate, indices);

	/* Get uniforms location */

	vtx_offset_loc = grate_get_vertex_uniform_location(program, "vtx_offset");
	tex_offset_loc = grate_get_fragment_uniform_location(program, "tex_offset");
	tex_scale_loc = grate_get_fragment_uniform_location(program, "tex_scale");

	/* Draw clamped to edge */

	offset_uniform.x = -0.5f;
	offset_uniform.y = -0.5f;

	grate_3d_ctx_set_vertex_uniform(ctx, vtx_offset_loc, 2, offset_uniform.f);
	grate_3d_ctx_set_fragment_float_uniform(ctx, tex_scale_loc, 2.0f);
	grate_3d_ctx_set_fragment_float_uniform(ctx, tex_offset_loc, -0.5f);

	grate_3d_ctx_bind_texture(ctx, 0, texture2);
	grate_texture_set_wrap_s(texture2, GRATE_TEXTURE_CLAMP_TO_EDGE);
	grate_texture_set_wrap_t(texture2, GRATE_TEXTURE_CLAMP_TO_EDGE);

	grate_3d_draw_elements(ctx, TGR3D_PRIMITIVE_TYPE_TRIANGLES,
			       bo, TGR3D_INDEX_MODE_UINT16,
			       ARRAY_SIZE(indices));
	grate_flush(grate);

	/* Draw repeated */

	offset_uniform.x =  0.5f;
	offset_uniform.y = -0.5f;

	grate_3d_ctx_set_vertex_uniform(ctx, vtx_offset_loc, 2, offset_uniform.f);
	grate_3d_ctx_set_fragment_float_uniform(ctx, tex_scale_loc, 3.0f);
	grate_3d_ctx_set_fragment_float_uniform(ctx, tex_offset_loc, 0.0f);

	grate_3d_ctx_bind_texture(ctx, 0, texture1);
	grate_texture_set_wrap_s(texture1, GRATE_TEXTURE_REPEAT);
	grate_texture_set_wrap_t(texture1, GRATE_TEXTURE_REPEAT);

	grate_3d_draw_elements(ctx, TGR3D_PRIMITIVE_TYPE_TRIANGLES,
			       bo, TGR3D_INDEX_MODE_UINT16,
			       ARRAY_SIZE(indices));
	grate_flush(grate);

	/* Draw mirror repeated */

	offset_uniform.x = 0.5f;
	offset_uniform.y = 0.5f;

	grate_3d_ctx_set_vertex_uniform(ctx, vtx_offset_loc, 2, offset_uniform.f);
	grate_3d_ctx_set_fragment_float_uniform(ctx, tex_scale_loc, 3.0f);
	grate_3d_ctx_set_fragment_float_uniform(ctx, tex_offset_loc, 0.0f);

	grate_3d_ctx_bind_texture(ctx, 0, texture1);
	grate_texture_set_wrap_s(texture1, GRATE_TEXTURE_MIRRORED_REPEAT);
	grate_texture_set_wrap_t(texture1, GRATE_TEXTURE_MIRRORED_REPEAT);

	grate_3d_draw_elements(ctx, TGR3D_PRIMITIVE_TYPE_TRIANGLES,
			       bo, TGR3D_INDEX_MODE_UINT16,
			       ARRAY_SIZE(indices));
	grate_flush(grate);

	/* Draw mixed */

	offset_uniform.x = -0.5f;
	offset_uniform.y =  0.5f;

	grate_3d_ctx_set_vertex_uniform(ctx, vtx_offset_loc, 2, offset_uniform.f);
	grate_3d_ctx_set_fragment_float_uniform(ctx, tex_scale_loc, 2.0f);
	grate_3d_ctx_set_fragment_float_uniform(ctx, tex_offset_loc, -0.5f);

	grate_3d_ctx_bind_texture(ctx, 0, texture2);
	grate_texture_set_wrap_s(texture2, GRATE_TEXTURE_MIRRORED_REPEAT);
	grate_texture_set_wrap_t(texture2, GRATE_TEXTURE_CLAMP_TO_EDGE);

	grate_3d_draw_elements(ctx, TGR3D_PRIMITIVE_TYPE_TRIANGLES,
			       bo, TGR3D_INDEX_MODE_UINT16,
			       ARRAY_SIZE(indices));
	grate_flush(grate);

	grate_swap_buffers(grate);
	grate_wait_for_key(grate);

	grate_exit(grate);
	return 0;
}
