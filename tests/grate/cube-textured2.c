/*
 * Copyright (c) 2012, 2013 Erik Faye-Lund
 * Copyright (c) 2013 Avionic Design GmbH
 * Copyright (c) 2013 Thierry Reding
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
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "grate.h"
#include "matrix.h"
#include "tgr_3d.xml.h"

#define ANIMATION_SPEED		60.0f

static const float cube_vertices[] = {
	/* front */
	-0.5f, -0.5f,  0.5f, 1.0f,
	 0.5f, -0.5f,  0.5f, 1.0f,
	 0.5f,  0.5f,  0.5f, 1.0f,
	-0.5f,  0.5f,  0.5f, 1.0f,
	/* back */
	-0.5f, -0.5f, -0.5f, 1.0f,
	 0.5f, -0.5f, -0.5f, 1.0f,
	 0.5f,  0.5f, -0.5f, 1.0f,
	-0.5f,  0.5f, -0.5f, 1.0f,
	/* left */
	-0.5f, -0.5f,  0.5f, 1.0f,
	-0.5f,  0.5f,  0.5f, 1.0f,
	-0.5f,  0.5f, -0.5f, 1.0f,
	-0.5f, -0.5f, -0.5f, 1.0f,
	/* right */
	 0.5f, -0.5f,  0.5f, 1.0f,
	 0.5f,  0.5f,  0.5f, 1.0f,
	 0.5f,  0.5f, -0.5f, 1.0f,
	 0.5f, -0.5f, -0.5f, 1.0f,
	/* top */
	-0.5f,  0.5f,  0.5f, 1.0f,
	 0.5f,  0.5f,  0.5f, 1.0f,
	 0.5f,  0.5f, -0.5f, 1.0f,
	-0.5f,  0.5f, -0.5f, 1.0f,
	/* bottom */
	-0.5f, -0.5f,  0.5f, 1.0f,
	 0.5f, -0.5f,  0.5f, 1.0f,
	 0.5f, -0.5f, -0.5f, 1.0f,
	-0.5f, -0.5f, -0.5f, 1.0f,
};

static const float cube_uv[] = {
	/* front */
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f,
	/* back */
	1.0f, 0.0f,
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	/* left */
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f,
	/* right */
	1.0f, 0.0f,
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	/* top */
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f,
	/* bottom */
	1.0f, 0.0f,
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
};

static const unsigned short cube_indices[] = {
	/* front */
	 0,  1,  2,
	 0,  2,  3,
	/* back */
	 6,  5,  4,
	 7,  6,  4,
	/* left */
	 8,  9, 10,
	 8, 10, 11,
	/* right */
	14, 13, 12,
	15, 14, 12,
	/* top */
	16, 17, 18,
	16, 18, 19,
	/* bottom */
	22, 21, 20,
	23, 22, 20,
};

static const float grate_vertices[] = {
	-1.0f, -1.0f, 0.0f, 1.0f,
	 1.0f, -1.0f, 0.0f, 1.0f,
	 1.0f,  1.0f, 0.0f, 1.0f,
	-1.0f,  1.0f, 0.0f, 1.0f,
};

static const float grate_uv[] = {
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f,
};

static const unsigned short grate_indices[] = {
	0, 1, 2,
	0, 2, 3,
};

int main(int argc, char *argv[])
{
	float x = 0.0f, y = 0.0f, z = 0.0f;
	struct grate_profile *profile;
	struct grate_framebuffer *fb;
	struct grate_options options;
	struct grate *grate;
	struct grate_3d_ctx *ctx;

	struct grate_program *cube_program;
	struct grate_texture *cube_texture;
	struct grate_shader *cube_vs, *cube_fs, *cube_linker;
	struct host1x_bo *cube_vertices_bo, *cube_texcoord_bo;
	struct host1x_bo *cube_bo;
	int cube_vertices_loc, cube_texcoord_loc;

	struct grate_program *grate_program;
	struct grate_texture *grate_texture;
	struct grate_shader *grate_fs;
	struct host1x_bo *grate_vertices_bo, *grate_texcoord_bo;
	struct host1x_bo *grate_bo;
	int grate_vertices_loc, grate_texcoord_loc;

	struct host1x_pixelbuffer *pixbuf;
	int cube_mvp_loc, grate_mvp_loc;
	float aspect, elapsed;

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
				      GRATE_DOUBLE_BUFFERED);
	if (!fb) {
		fprintf(stderr, "grate_framebuffer_create() failed\n");
		return 1;
	}

	aspect = options.width / (float)options.height;

	grate_clear_color(grate, 0.0, 0.0, 0.0, 1.0f);
	grate_bind_framebuffer(grate, fb);

	/* Prepare shaders */

	cube_vs = grate_shader_parse_vertex_asm_from_file(
				"tests/grate/asm/cube2_vs.txt");
	if (!cube_vs) {
		fprintf(stderr, "cube2_vs assembler parse failed\n");
		return 1;
	}

	cube_fs = grate_shader_parse_fragment_asm_from_file(
				"tests/grate/asm/cube2_fs.txt");
	if (!cube_fs) {
		fprintf(stderr, "cube2_fs assembler parse failed\n");
		return 1;
	}

	grate_fs = grate_shader_parse_fragment_asm_from_file(
				"tests/grate/asm/cube2_grate_fs.txt");
	if (!grate_fs) {
		fprintf(stderr, "cube2_grate_fs assembler parse failed\n");
		return 1;
	}

	cube_linker = grate_shader_parse_linker_asm_from_file(
				"tests/grate/asm/cube2_linker.txt");
	if (!cube_linker) {
		fprintf(stderr, "cube2_linker assembler parse failed\n");
		return 1;
	}

	cube_program = grate_program_new(grate, cube_vs, cube_fs, cube_linker);
	grate_program_link(cube_program);

	grate_program = grate_program_new(grate, cube_vs, grate_fs, cube_linker);
	grate_program_link(grate_program);

	cube_mvp_loc = grate_get_vertex_uniform_location(cube_program, "mvp");
	grate_mvp_loc = grate_get_vertex_uniform_location(grate_program, "mvp");

	/* Setup context */

	ctx = grate_3d_alloc_ctx(grate);

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
	grate_3d_ctx_set_cull_face(ctx, GRATE_CULL_FACE_BACK);
	grate_3d_ctx_set_scissor(ctx, 0, options.width, 0, options.height);
	grate_3d_ctx_set_point_coord_range(ctx, 0.0f, 1.0f, 0.0f, 1.0f);
	grate_3d_ctx_set_polygon_offset(ctx, 0.0f, 0.0f);
	grate_3d_ctx_set_provoking_vtx_last(ctx, true);

	/* Setup cube attributes */

	cube_vertices_loc = grate_get_attribute_location(cube_program,
							 "position");
	cube_vertices_bo = grate_bo_create_from_data(grate,
						     sizeof(cube_vertices),
						     NVHOST_BO_FLAG_ATTRIBUTES,
						     cube_vertices);

	cube_texcoord_loc = grate_get_attribute_location(cube_program,
							 "texcoord");
	cube_texcoord_bo = grate_bo_create_from_data(grate,
						     sizeof(cube_uv),
						     NVHOST_BO_FLAG_ATTRIBUTES,
						     cube_uv);

	/* Setup grate attributes */

	grate_vertices_loc = grate_get_attribute_location(grate_program,
							  "position");
	grate_vertices_bo = grate_bo_create_from_data(grate,
						      sizeof(grate_vertices),
						      NVHOST_BO_FLAG_ATTRIBUTES,
						      grate_vertices);

	grate_texcoord_loc = grate_get_attribute_location(grate_program,
							  "texcoord");
	grate_texcoord_bo = grate_bo_create_from_data(grate,
						      sizeof(grate_uv),
						      NVHOST_BO_FLAG_ATTRIBUTES,
						      grate_uv);

	/* Setup render target */

	grate_3d_ctx_enable_render_target(ctx, 1);

	/* Setup textures */

	cube_texture = grate_create_texture(grate, 300, 300,
					    PIX_BUF_FMT_RGBA8888,
					    PIX_BUF_LAYOUT_LINEAR);
	grate_texture_load(grate, cube_texture, "data/tegra.png");
	grate_texture_set_max_lod(cube_texture, 0);
	grate_texture_set_wrap_mode(cube_texture, 0);
	grate_texture_set_mip_filter(cube_texture, false);
	grate_texture_set_mag_filter(cube_texture, false);
	grate_texture_set_min_filter(cube_texture, false);

	grate_3d_ctx_bind_texture(ctx, 0, cube_texture);


	grate_texture = grate_create_texture(grate, 1024, 768,
					     PIX_BUF_FMT_RGBA8888,
					     PIX_BUF_LAYOUT_LINEAR);
	grate_texture_load(grate, grate_texture, "data/grate/grate.jpg");
	grate_texture_set_max_lod(grate_texture, 0);
	grate_texture_set_wrap_mode(grate_texture, 0);
	grate_texture_set_mip_filter(grate_texture, false);
	grate_texture_set_mag_filter(grate_texture, false);
	grate_texture_set_min_filter(grate_texture, false);

	grate_3d_ctx_bind_texture(ctx, 11, grate_texture);

	/* Create indices BO */

	cube_bo = grate_bo_create_from_data(grate, sizeof(cube_indices),
					    NVHOST_BO_FLAG_ATTRIBUTES,
					    cube_indices);

	grate_bo = grate_bo_create_from_data(grate, sizeof(grate_indices),
					     NVHOST_BO_FLAG_ATTRIBUTES,
					     grate_indices);

	profile = grate_profile_start(grate);

	while (true) {
		struct mat4 modelview, projection, transform, result, rotate;
		struct mat4 cube_mvp, grate_mvp;

		grate_clear(grate);

		/* Setup render target */
		pixbuf = grate_get_draw_pixbuf(fb);
		grate_3d_ctx_bind_render_target(ctx, 1, pixbuf);

		mat4_perspective(&projection, 60.0f, aspect, 1.0f, 1024.0f);

		/* Cube MVP */
		mat4_identity(&modelview);
		mat4_rotate_x(&transform, x);
		mat4_multiply(&rotate, &modelview, &transform);
		mat4_rotate_y(&transform, y);
		mat4_multiply(&modelview, &rotate, &transform);
		mat4_rotate_z(&transform, z);
		mat4_multiply(&rotate, &modelview, &transform);
		mat4_translate(&transform, 0.0f, 0.8f, -4.0f);
		mat4_multiply(&modelview, &transform, &rotate);
		mat4_multiply(&cube_mvp, &projection, &modelview);

		/* Draw cube */
		grate_3d_ctx_bind_program(ctx, cube_program);
		grate_3d_ctx_set_vertex_uniform(ctx, cube_mvp_loc, 16,
						(float *) &cube_mvp);
		grate_3d_ctx_vertex_attrib_pointer(
			ctx, cube_vertices_loc, 4, ATTRIB_TYPE_FLOAT32,
			4 * sizeof(float), cube_vertices_bo);
		grate_3d_ctx_vertex_attrib_pointer(
			ctx, cube_texcoord_loc, 2, ATTRIB_TYPE_FLOAT32,
			2 * sizeof(float), cube_texcoord_bo);
		grate_3d_ctx_enable_vertex_attrib_array(ctx, cube_vertices_loc);
		grate_3d_ctx_enable_vertex_attrib_array(ctx, cube_texcoord_loc);

		grate_3d_draw_elements(ctx, PRIMITIVE_TYPE_TRIANGLES,
				       cube_bo, INDEX_MODE_UINT16,
				       ARRAY_SIZE(cube_indices));
		grate_flush(grate);

		/* Draw couple more cubes */
		mat4_identity(&modelview);
		mat4_multiply(&result, &modelview, &rotate);
		mat4_translate(&transform, -2.5f, 0.0f, 0.0f);
		mat4_multiply(&modelview, &transform, &result);
		mat4_multiply(&result, &rotate, &modelview);
		mat4_translate(&transform, 0.0f, 0.8f, -4.0f);
		mat4_multiply(&modelview, &transform, &result);
		mat4_multiply(&cube_mvp, &projection, &modelview);

		grate_3d_ctx_set_vertex_uniform(ctx, cube_mvp_loc, 16,
						(float *) &cube_mvp);
		grate_3d_draw_elements(ctx, PRIMITIVE_TYPE_TRIANGLES,
				       cube_bo, INDEX_MODE_UINT16,
				       ARRAY_SIZE(cube_indices));
		grate_flush(grate);

		mat4_identity(&modelview);
		mat4_multiply(&result, &modelview, &rotate);
		mat4_translate(&transform, 2.5f, 0.0f, 0.0f);
		mat4_multiply(&modelview, &transform, &result);
		mat4_multiply(&result, &rotate, &modelview);
		mat4_translate(&transform, 0.0f, 0.8f, -4.0f);
		mat4_multiply(&modelview, &transform, &result);
		mat4_multiply(&cube_mvp, &projection, &modelview);

		grate_3d_ctx_set_vertex_uniform(ctx, cube_mvp_loc, 16,
						(float *) &cube_mvp);
		grate_3d_draw_elements(ctx, PRIMITIVE_TYPE_TRIANGLES,
				       cube_bo, INDEX_MODE_UINT16,
				       ARRAY_SIZE(cube_indices));
		grate_flush(grate);

		grate_3d_ctx_disable_vertex_attrib_array(ctx, cube_vertices_loc);
		grate_3d_ctx_disable_vertex_attrib_array(ctx, cube_texcoord_loc);

		/* Grate MVP */
		mat4_identity(&grate_mvp);

		/* Draw grate */
		grate_3d_ctx_bind_program(ctx, grate_program);
		grate_3d_ctx_set_vertex_uniform(ctx, grate_mvp_loc, 16,
						(float *) &grate_mvp);
		grate_3d_ctx_vertex_attrib_pointer(
			ctx, grate_vertices_loc, 4,  ATTRIB_TYPE_FLOAT32,
			4 * sizeof(float), grate_vertices_bo);
		grate_3d_ctx_vertex_attrib_pointer(
			ctx, grate_texcoord_loc, 2, ATTRIB_TYPE_FLOAT32,
			2 * sizeof(float), grate_texcoord_bo);
		grate_3d_ctx_enable_vertex_attrib_array(ctx, grate_vertices_loc);
		grate_3d_ctx_enable_vertex_attrib_array(ctx, grate_texcoord_loc);

		grate_3d_draw_elements(ctx, PRIMITIVE_TYPE_TRIANGLES,
				       grate_bo, INDEX_MODE_UINT16,
				       ARRAY_SIZE(grate_indices));
		grate_flush(grate);

		grate_3d_ctx_disable_vertex_attrib_array(ctx, grate_vertices_loc);
		grate_3d_ctx_disable_vertex_attrib_array(ctx, grate_texcoord_loc);

		grate_swap_buffers(grate);

		if (grate_key_pressed(grate))
			break;

		grate_profile_sample(profile);

		elapsed = grate_profile_time_elapsed(profile);

		x = 0.3f * ANIMATION_SPEED * elapsed;
		y = 0.2f * ANIMATION_SPEED * elapsed;
		z = 0.4f * ANIMATION_SPEED * elapsed;
	}

	grate_profile_finish(profile);
	grate_profile_free(profile);

	grate_exit(grate);
	return 0;
}
