/*
 * Copyright (c) GRATE-DRIVER project
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
#include "grate-3d.h"
#include "matrix.h"
#include "tgr_3d.xml.h"

#define ANIMATION_SPEED		80.0f

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

static const float sky_vertices[] = {
	-1.0f, -1.0f, 0.0f, 1.0f,
	 1.0f, -1.0f, 0.0f, 1.0f,
	 1.0f,  1.0f, 0.0f, 1.0f,
	-1.0f,  1.0f, 0.0f, 1.0f,
};

static const float sky_uv[] = {
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f,
};

static const unsigned short sky_indices[] = {
	0, 1, 2,
	0, 2, 3,
};

struct comression {
	const char *name;
	unsigned fmt;
};

static const struct comression compression_modes[] = {
	{ .name = "NONE", .fmt = PIX_BUF_FMT_RGBA8888 },
	{ .name = "ETC1", .fmt = PIX_BUF_FMT_ETC1 },
	{ .name = "DXT1", .fmt = PIX_BUF_FMT_DXT1 },
	{ .name = "DXT3", .fmt = PIX_BUF_FMT_DXT3 },
	{ .name = "DXT5", .fmt = PIX_BUF_FMT_DXT5 },
};

static struct mat4 projection, mvp;
static float x = 0.0f, y = 0.0f, z = 0.0f, font_scale;
static struct grate_profile *profile;
static struct grate_framebuffer *fb;
static struct grate_options options;
static struct grate *grate;
static struct grate_3d_ctx *ctx;
static struct grate_texture *depth_buffer;
static struct grate_font *font;

static struct grate_program *cube_program;
static struct grate_shader *cube_vs, *cube_fs, *cube_linker;
static struct host1x_bo *cube_vertices_bo, *cube_texcoord_bo;
static struct host1x_bo *cube_bo;
static int cube_vertices_loc, cube_texcoord_loc;

static struct grate_program *sky_program;
static struct grate_shader *sky_vs, *sky_fs, *sky_linker;
static struct host1x_bo *sky_vertices_bo, *sky_texcoord_bo;
static struct host1x_bo *sky_bo;
static int sky_vertices_loc, sky_texcoord_loc;

static struct grate_texture *sky_textures[ARRAY_SIZE(compression_modes)];
static struct grate_texture *cube_textures[ARRAY_SIZE(compression_modes)];
static struct grate_texture *galaxy_textures[ARRAY_SIZE(compression_modes)];

static struct host1x_pixelbuffer *fb_pixbuf;
static struct host1x_pixelbuffer *pixbuf;
static unsigned i, frames = 0;
static float aspect, elapsed = 0;
static float basetime = 0;
static int cube_mvp_loc;
static int sky_mvp_loc;
static int location;

static void draw_background(struct grate_texture *sky_texture,
			    struct grate_texture *galaxy_texture)
{
	mat4_identity(&mvp);

	/* Make background look nice on vertical display */
	if (options.width < options.height)
		mat4_rotate_z(&mvp, 90);

	grate_3d_ctx_bind_texture(ctx, 0, sky_texture);
	grate_3d_ctx_bind_texture(ctx, 1, galaxy_texture);

	grate_3d_ctx_bind_program(ctx, sky_program);

	location = grate_get_fragment_uniform_location(sky_program, "day");
	grate_3d_ctx_set_fragment_float_uniform(ctx, location,
				powf(sinf(elapsed * 0.1f), 6.0f));

	location = grate_get_fragment_uniform_location(sky_program, "night");
	grate_3d_ctx_set_fragment_float_uniform(ctx, location,
				powf(cosf(elapsed * 0.1f), 6.0f));

	grate_3d_ctx_set_vertex_mat4_uniform(ctx, sky_mvp_loc, &mvp);
	grate_3d_ctx_vertex_attrib_float_pointer(ctx, sky_vertices_loc,
							4, sky_vertices_bo);
	grate_3d_ctx_vertex_attrib_float_pointer(ctx, sky_texcoord_loc,
							2, sky_texcoord_bo);
	grate_3d_ctx_enable_vertex_attrib_array(ctx, sky_vertices_loc);
	grate_3d_ctx_enable_vertex_attrib_array(ctx, sky_texcoord_loc);

	grate_3d_draw_elements(ctx, TGR3D_PRIMITIVE_TYPE_TRIANGLES,
				sky_bo, TGR3D_INDEX_MODE_UINT16,
				ARRAY_SIZE(sky_indices));
	grate_flush(grate);

	grate_3d_ctx_disable_vertex_attrib_array(ctx, sky_vertices_loc);
	grate_3d_ctx_disable_vertex_attrib_array(ctx, sky_texcoord_loc);
}

static void draw_cube(struct grate_texture *cube_texture,
		      float x, float y, float z,
		      float rx, float ry, float rz)
{
	struct mat4 modelview, transform, rotate, result;

	/* set up rotation matrix */
	mat4_identity(&modelview);
	mat4_rotate_x(&transform, rx);
	mat4_multiply(&rotate, &modelview, &transform);
	mat4_rotate_y(&transform, ry);
	mat4_multiply(&modelview, &rotate, &transform);
	mat4_rotate_z(&transform, rz);
	mat4_multiply(&rotate, &modelview, &transform);

	mat4_identity(&modelview);
	mat4_multiply(&result, &modelview, &rotate);

	/* move and rotate relatively to the centered cube */
	if (x || y || z) {
		mat4_translate(&transform, x, y, z);
		mat4_multiply(&modelview, &transform, &result);
		mat4_multiply(&result, &rotate, &modelview);
	}

	mat4_translate(&transform, 0.0f, 0.0f, -2.0f);
	mat4_multiply(&modelview, &transform, &result);

	mat4_multiply(&mvp, &projection, &modelview);

	/* Draw cube */
	grate_3d_ctx_bind_texture(ctx, 0, cube_texture);
	grate_3d_ctx_bind_program(ctx, cube_program);
	grate_3d_ctx_set_vertex_mat4_uniform(ctx, cube_mvp_loc, &mvp);
	grate_3d_ctx_vertex_attrib_float_pointer(ctx, cube_vertices_loc,
							4, cube_vertices_bo);
	grate_3d_ctx_vertex_attrib_float_pointer(ctx, cube_texcoord_loc,
							2, cube_texcoord_bo);
	grate_3d_ctx_enable_vertex_attrib_array(ctx, cube_vertices_loc);
	grate_3d_ctx_enable_vertex_attrib_array(ctx, cube_texcoord_loc);

	grate_3d_draw_elements(ctx, TGR3D_PRIMITIVE_TYPE_TRIANGLES,
				cube_bo, TGR3D_INDEX_MODE_UINT16,
				ARRAY_SIZE(cube_indices));
	grate_flush(grate);
}

int main(int argc, char *argv[])
{
	struct grate_texture *cube_texture, *galaxy_texture, *sky_texture;

	if (access("tests/grate/asm/cube2_vs.txt", F_OK) == -1) {
		if (chdir( dirname(argv[0]) ) == -1)
			fprintf(stderr, "chdir failed\n");

		if (chdir("../../") == -1)
			fprintf(stderr, "chdir failed\n");
	}

	if (!grate_parse_command_line(&options, argc, argv))
		return 1;

	grate = grate_init(&options);
	if (!grate)
		return 1;

	aspect = options.width / (float)options.height;

	fb = grate_framebuffer_create(grate, options.width, options.height,
				      PIX_BUF_FMT_RGBA8888,
				      PIX_BUF_LAYOUT_TILED_16x16,
				      GRATE_DOUBLE_BUFFERED);
	if (!fb) {
		fprintf(stderr, "grate_framebuffer_create() failed\n");
		return 1;
	}

	fb_pixbuf = grate_get_draw_pixbuf(fb);

	grate_clear_color(grate, 0.1, 0.1, 0.1, 1.0f);
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

	cube_linker = grate_shader_parse_linker_asm_from_file(
				"tests/grate/asm/cube2_linker.txt");
	if (!cube_linker) {
		fprintf(stderr, "cube2_linker assembler parse failed\n");
		return 1;
	}

	cube_program = grate_program_new(grate, cube_vs, cube_fs, cube_linker);
	if (!cube_program) {
		fprintf(stderr, "grate_program_new(cube_program) failed\n");
		return 1;
	}

	grate_program_link(cube_program);

	cube_mvp_loc = grate_get_vertex_uniform_location(cube_program, "mvp");

	/* Prepare shaders */

	sky_vs = grate_shader_parse_vertex_asm_from_file(
				"tests/grate/asm/sky_vs.txt");
	if (!sky_vs) {
		fprintf(stderr, "sky_vs assembler parse failed\n");
		return 1;
	}

	sky_fs = grate_shader_parse_fragment_asm_from_file(
				"tests/grate/asm/sky_fs.txt");
	if (!sky_fs) {
		fprintf(stderr, "sky_fs assembler parse failed\n");
		return 1;
	}

	sky_linker = grate_shader_parse_linker_asm_from_file(
				"tests/grate/asm/sky_linker.txt");
	if (!sky_linker) {
		fprintf(stderr, "sky_linker assembler parse failed\n");
		return 1;
	}

	sky_program = grate_program_new(grate, sky_vs, sky_fs, sky_linker);
	if (!sky_program) {
		fprintf(stderr, "grate_program_new(sky_program) failed\n");
		return 1;
	}

	grate_program_link(sky_program);

	sky_mvp_loc = grate_get_vertex_uniform_location(sky_program, "mvp");

	/* Set up context */

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
	grate_3d_ctx_set_cull_face(ctx, GRATE_3D_CTX_CULL_FACE_BACK);
	grate_3d_ctx_set_scissor(ctx, 0, options.width, 0, options.height);
	grate_3d_ctx_set_point_coord_range(ctx, 0.0f, 1.0f, 0.0f, 1.0f);
	grate_3d_ctx_set_polygon_offset(ctx, 0.0f, 0.0f);
	grate_3d_ctx_set_provoking_vtx_last(ctx, true);

	/* Set up depth buffer */

	depth_buffer = grate_create_texture(grate,
					    options.width, options.height,
					    PIX_BUF_FMT_D16_LINEAR,
					    PIX_BUF_LAYOUT_TILED_16x16);

	pixbuf = grate_texture_pixbuf(depth_buffer);

	grate_3d_ctx_bind_depth_buffer(ctx, pixbuf);

	/* Set up cube attributes */

	cube_vertices_loc = grate_get_attribute_location(cube_program,
							 "position");
	cube_vertices_bo = grate_create_attrib_bo_from_data(grate,
							    cube_vertices);

	cube_texcoord_loc = grate_get_attribute_location(cube_program,
							 "texcoord");
	cube_texcoord_bo = grate_create_attrib_bo_from_data(grate, cube_uv);

	/* Set up grate attributes */

	sky_vertices_loc = grate_get_attribute_location(sky_program, "position");
	sky_vertices_bo = grate_create_attrib_bo_from_data(grate, sky_vertices);

	sky_texcoord_loc = grate_get_attribute_location(sky_program, "texcoord");
	sky_texcoord_bo = grate_create_attrib_bo_from_data(grate, sky_uv);

	/* Set up render target */

	grate_3d_ctx_enable_render_target(ctx, 1);

	/* Set up font */

	font = grate_create_font(grate, "data/font.png", "data/font.fnt");
	if (!font) {
		fprintf(stderr, "failed to load font\n");
		return 1;
	}

	font_scale = options.width / 500.0f / aspect;

	/* Set up textures */

	for (i = 0; i < ARRAY_SIZE(compression_modes); i++) {
		unsigned num_textures = 3;
		unsigned percent = 100 / ARRAY_SIZE(compression_modes) / num_textures;
		float txtpos = (float) strlen("LOADING 99%") * -14 / options.width *
				font_scale;

		/* Show progress */

		grate_clear(grate);
		pixbuf = grate_get_draw_pixbuf(fb);
		grate_3d_ctx_bind_render_target(ctx, 1, pixbuf);
		grate_3d_printf(grate, ctx, font, 1, txtpos, 0.0f, font_scale,
				"LOADING %u%%", percent * (i * num_textures + 0));
		grate_swap_buffers(grate);

		/* Load and compress cube texture */

		cube_texture = grate_create_texture(grate, 812, 812,
						    compression_modes[i].fmt,
						    PIX_BUF_LAYOUT_LINEAR);
		grate_texture_load(grate, cube_texture, "data/tegra.png");
		grate_texture_set_wrap_s(cube_texture, GRATE_TEXTURE_CLAMP_TO_EDGE);
		grate_texture_set_wrap_t(cube_texture, GRATE_TEXTURE_CLAMP_TO_EDGE);
		grate_texture_set_min_filter(cube_texture, GRATE_TEXTURE_NEAREST);
		grate_texture_set_mag_filter(cube_texture, GRATE_TEXTURE_NEAREST);

		cube_textures[i] = cube_texture;

		/* Show progress */

		grate_clear(grate);
		pixbuf = grate_get_draw_pixbuf(fb);
		grate_3d_ctx_bind_render_target(ctx, 1, pixbuf);
		grate_3d_printf(grate, ctx, font, 1, txtpos, 0.0f, font_scale,
				"LOADING %u%%", percent * (i * num_textures + 1));
		grate_swap_buffers(grate);

		/* Load and compress galaxy texture */

		galaxy_texture = grate_create_texture(grate, 2048, 2048,
						      compression_modes[i].fmt,
						      PIX_BUF_LAYOUT_LINEAR);
		grate_texture_load(grate, galaxy_texture, "data/galaxy-1489300667bhk.jpg");
		grate_texture_set_wrap_s(galaxy_texture, GRATE_TEXTURE_CLAMP_TO_EDGE);
		grate_texture_set_wrap_t(galaxy_texture, GRATE_TEXTURE_CLAMP_TO_EDGE);
		grate_texture_set_min_filter(galaxy_texture, GRATE_TEXTURE_NEAREST);
		grate_texture_set_mag_filter(galaxy_texture, GRATE_TEXTURE_NEAREST);

		galaxy_textures[i] = galaxy_texture;

		/* Show progress */

		grate_clear(grate);
		pixbuf = grate_get_draw_pixbuf(fb);
		grate_3d_ctx_bind_render_target(ctx, 1, pixbuf);
		grate_3d_printf(grate, ctx, font, 1, txtpos, 0.0f, font_scale,
				"LOADING %u%%", percent * (i * num_textures + 2));
		grate_swap_buffers(grate);

		/* Load and compress sky texture */

		sky_texture = grate_create_texture(grate, 2048, 2048,
						   compression_modes[i].fmt,
						   PIX_BUF_LAYOUT_LINEAR);
		grate_texture_load(grate, sky_texture, "data/blue-sky-and-white-clouds-1457775320B2G.jpg");
		grate_texture_set_wrap_s(sky_texture, GRATE_TEXTURE_CLAMP_TO_EDGE);
		grate_texture_set_wrap_t(sky_texture, GRATE_TEXTURE_CLAMP_TO_EDGE);
		grate_texture_set_min_filter(sky_texture, GRATE_TEXTURE_NEAREST);
		grate_texture_set_mag_filter(sky_texture, GRATE_TEXTURE_NEAREST);

		sky_textures[i] = sky_texture;
	}

	font_scale = options.width / 700.0f / aspect;

	/* Create indices BO */

	cube_bo = grate_create_attrib_bo_from_data(grate, cube_indices);
	sky_bo = grate_create_attrib_bo_from_data(grate, sky_indices);

	mat4_perspective(&projection, 60.0f, aspect, 1.0f, 1024.0f);

	profile = grate_profile_start(grate);

	while (true) {
		static unsigned prev_mode = 0;
		unsigned mode;

		mode = (int)elapsed / 7 % ARRAY_SIZE(compression_modes);

		/* Reset FPS stats on a mode switch */
		if (mode != prev_mode) {
			prev_mode = mode;
			basetime = elapsed;
			frames = 0;
		}

		sky_texture = sky_textures[mode];
		cube_texture = cube_textures[mode];
		galaxy_texture = galaxy_textures[mode];

		/* Set up render target */
		pixbuf = grate_get_draw_pixbuf(fb);
		grate_3d_ctx_bind_render_target(ctx, 1, pixbuf);

		/* Draw background, bypassing depth tests */
		grate_3d_ctx_perform_depth_test(ctx, false);
		grate_3d_ctx_perform_depth_write(ctx, false);

		draw_background(sky_texture, galaxy_texture);

		/* Clear depth buffer and enable depth test */
		grate_texture_clear(grate, depth_buffer, 0xFFFFFFFF);
		grate_3d_ctx_set_depth_func(ctx, GRATE_3D_CTX_DEPTH_FUNC_LEQUAL);
		grate_3d_ctx_perform_depth_test(ctx, true);
		grate_3d_ctx_perform_depth_write(ctx, true);

		draw_cube(cube_texture, 0.0f, 0.0f, 0.0f, x, y, z);

		grate_3d_printf(grate, ctx, font, 1, -0.85f, 0.85f, font_scale,
				"Texture compression: %s\n"
				"SKY texture size: %ux%u\n"
				"Galaxy texture size: %ux%u\n"
				"Cube texture size: %ux%u\n"
				"Framebuffer size: %ux%u\n"
				"FPS: %.2f (%s)\n",
				compression_modes[mode].name,
				sky_texture->pixbuf->width,
				sky_texture->pixbuf->height,
				galaxy_texture->pixbuf->width,
				galaxy_texture->pixbuf->height,
				cube_texture->pixbuf->width,
				cube_texture->pixbuf->height,
				fb_pixbuf->width, fb_pixbuf->height,
				frames / (elapsed - basetime),
				(options.vsync || !options.singlebuffered) ?
				"VSYNC limited" : "Unlimited");

		grate_swap_buffers(grate);

		if (grate_key_pressed(grate))
			break;

		grate_profile_sample(profile);

		elapsed = grate_profile_time_elapsed(profile);
		frames++;

		x = 0.3f * ANIMATION_SPEED * elapsed;
		y = 0.2f * ANIMATION_SPEED * elapsed;
		z = 0.4f * ANIMATION_SPEED * elapsed;
	}

	grate_profile_finish(profile);
	grate_profile_free(profile);

	grate_exit(grate);
	return 0;
}
