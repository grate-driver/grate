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

#include <string.h>

#include "libgrate-private.h"
#include "grate-3d.h"
#include "tgr_3d.xml.h"

static uint32_t float_to_fp20(float f)
{
	uint32_t sign, mantissa, exponent;
	union {
		uint32_t u;
		float f;
	} value;

	value.f = f;

	sign = (value.u >> 31) & 0x1;
	exponent = (value.u >> 23) & 0xff;
	mantissa = (value.u >>  0) & 0x7fffff;

	if (exponent == 0xff)
		exponent = 0x3f;
	else
		exponent = (exponent - 127 + 31) & 0x3f;

	return (sign << 19) | (exponent << 13) | (mantissa >> (23 - 13));
}

static uint32_t float_to_fx10(float f)
{
	uint32_t u = f * 256.0f;
	return u & 0x3ff;
}

struct grate_3d_ctx * grate_3d_alloc_ctx(struct grate *grate)
{
	struct grate_3d_ctx *ctx = calloc(1, sizeof(struct grate_3d_ctx));

	if (!ctx) {
		grate_error("Failed to allocate context\n");
		return NULL;
	}

	ctx->grate = grate;

	return ctx;
}

int grate_3d_ctx_vertex_attrib_pointer(struct grate_3d_ctx *ctx,
				       unsigned location, unsigned size,
				       unsigned type, unsigned stride,
				       struct host1x_bo *data_bo)
{
	struct grate_vtx_attribute *attr;

	switch (type) {
	case ATTRIB_TYPE_UBYTE:
	case ATTRIB_TYPE_UBYTE_NORM:
	case ATTRIB_TYPE_SBYTE:
	case ATTRIB_TYPE_SBYTE_NORM:
	case ATTRIB_TYPE_USHORT:
	case ATTRIB_TYPE_USHORT_NORM:
	case ATTRIB_TYPE_SSHORT:
	case ATTRIB_TYPE_SSHORT_NORM:
	case ATTRIB_TYPE_FIXED16:
	case ATTRIB_TYPE_FLOAT16:
	case ATTRIB_TYPE_UINT:
	case ATTRIB_TYPE_UINT_NORM:
	case ATTRIB_TYPE_SINT:
	case ATTRIB_TYPE_SINT_NORM:
	case ATTRIB_TYPE_FLOAT32:
		break;
	default:
		grate_error("Invalid type %u\n", type);
		return -1;
	}

	if (location >= 16) {
		grate_error("Invalid location %u\n", location);
		return -1;
	}

	if (!data_bo) {
		grate_error("Invalid data BO ptr\n");
		return -1;
	}

	attr = calloc(1, sizeof(*attr));
	if (!attr) {
		grate_error("Allocation failed\n");
		return -1;
	}

	attr->stride = stride;
	attr->type = type;
	attr->size = size;
	attr->bo = data_bo;

	ctx->vtx_attributes[location] = attr;

	return 0;
}

int grate_3d_ctx_enable_vertex_attrib_array(struct grate_3d_ctx *ctx,
					    unsigned location)
{
	if (location >= 16) {
		grate_error("Invalid location %u\n", location);
		return -1;
	}

	ctx->attributes_enable_mask |= 1u << location;

	return 0;
}

int grate_3d_ctx_disable_vertex_attrib_array(struct grate_3d_ctx *ctx,
					     unsigned target)
{
	if (target >= 16) {
		grate_error("Invalid target location %u\n", target);
		return -1;
	}

	ctx->attributes_enable_mask &= ~(1u << target);

	return 0;
}

int grate_3d_ctx_bind_render_target(struct grate_3d_ctx *ctx,
				    unsigned target,
				    struct host1x_pixelbuffer *pixbuf)
{
	switch (pixbuf->format) {
	case PIX_BUF_FMT_A8:
	case PIX_BUF_FMT_L8:
	case PIX_BUF_FMT_S8:
	case PIX_BUF_FMT_LA88:
	case PIX_BUF_FMT_RGB565:
	case PIX_BUF_FMT_RGBA5551:
	case PIX_BUF_FMT_RGBA4444:
	case PIX_BUF_FMT_D16_LINEAR:
	case PIX_BUF_FMT_D16_NONLINEAR:
	case PIX_BUF_FMT_RGBA8888:
	case PIX_BUF_FMT_RGBA_FP32:
		break;
	default:
		grate_error("Invalid format %u\n", pixbuf->format);
		return -1;
	}

	if (target >= 16) {
		grate_error("Invalid target location %u\n", target);
		return -1;
	}

	ctx->render_targets[target].pixbuf = pixbuf;

	return 0;
}

int grate_3d_ctx_set_render_target_dither(struct grate_3d_ctx *ctx,
					  unsigned target,
					  bool enable)
{
	if (target >= 16) {
		grate_error("Invalid target location %u\n", target);
		return -1;
	}

	ctx->render_targets[target].dither_enabled = enable;

	return 0;
}

int grate_3d_ctx_enable_render_target(struct grate_3d_ctx *ctx,
				      unsigned target)
{
	if (target >= 16) {
		grate_error("Invalid target location %u\n", target);
		return -1;
	}

	ctx->render_targets_enable_mask |= 1u << target;

	return 0;
}

int grate_3d_ctx_disable_render_target(struct grate_3d_ctx *ctx,
				       unsigned target)
{
	if (target >= 16) {
		grate_error("Invalid target location %u\n", target);
		return -1;
	}

	ctx->render_targets_enable_mask &= ~(1u << target);

	return 0;
}

int grate_3d_ctx_bind_program(struct grate_3d_ctx *ctx,
			      struct grate_program *program)
{
	if (!program) {
		grate_error("Bad program ptr\n");
		return -1;
	}

	ctx->program = program;

	memcpy(ctx->vs_uniforms, program->vs_constants,
	       sizeof(ctx->vs_uniforms));

	memcpy(ctx->fs_uniforms, program->fs_constants,
	       sizeof(ctx->fs_uniforms));

	return 0;
}

int grate_3d_ctx_set_vertex_uniform(struct grate_3d_ctx *ctx,
				    unsigned location, unsigned nb,
				    float *values)
{
	if (!ctx->program) {
		grate_error("No program bound\n");
		return -1;

	}

	if (location + nb > 256) {
		grate_error("Invalid location %u\n", location);
		return -1;
	}

	memcpy(&ctx->vs_uniforms[location * 4], values, nb * sizeof(float));

	return 0;
}

int grate_3d_ctx_set_fragment_uniform_fp20(struct grate_3d_ctx *ctx,
					   unsigned location, float value)
{
	if (!ctx->program) {
		grate_error("No program bound\n");
		return -1;

	}

	if (location >= 32) {
		grate_error("Invalid location %u\n", location);
		return -1;
	}

	ctx->fs_uniforms[location] = float_to_fp20(value);

	return 0;
}

int grate_3d_ctx_set_fragment_uniform_fx10_low(struct grate_3d_ctx *ctx,
					       unsigned location, float value)
{
	if (!ctx->program) {
		grate_error("No program bound\n");
		return -1;

	}

	if (location >= 32) {
		grate_error("Invalid location %u\n", location);
		return -1;
	}

	ctx->fs_uniforms[location] &= ~0x3ff;
	ctx->fs_uniforms[location] |= float_to_fx10(value);

	return 0;
}

int grate_3d_ctx_set_fragment_uniform_fx10_high(struct grate_3d_ctx *ctx,
						unsigned location, float value)
{
	if (!ctx->program) {
		grate_error("No program bound\n");
		return -1;

	}

	if (location >= 32) {
		grate_error("Invalid location %u\n", location);
		return -1;
	}

	ctx->fs_uniforms[location] &= 0x3ff;
	ctx->fs_uniforms[location] |= float_to_fx10(value) << 10;

	return 0;
}

void grate_3d_ctx_set_depth_range(struct grate_3d_ctx *ctx,
				  float near, float far)
{
	ctx->depth_range_near = near;
	ctx->depth_range_far = far;
}

void grate_3d_ctx_set_dither(struct grate_3d_ctx *ctx, uint32_t unk)
{
	ctx->dither_unk = unk;
}

void grate_3d_ctx_set_viewport_bias(struct grate_3d_ctx *ctx,
				    float x, float y, float z)
{
	ctx->viewport_x_bias = x;
	ctx->viewport_y_bias = y;
	ctx->viewport_z_bias = z;
}

void grate_3d_ctx_set_viewport_scale(struct grate_3d_ctx *ctx,
				     float width, float height, float depth)
{
	ctx->viewport_x_scale = width;
	ctx->viewport_y_scale = height;
	ctx->viewport_z_scale = depth;
}

void grate_3d_ctx_set_point_params(struct grate_3d_ctx *ctx, uint32_t params)
{
	ctx->point_params = params;
}

void grate_3d_ctx_set_point_size(struct grate_3d_ctx *ctx, float size)
{
	ctx->point_size = size;
}

void grate_3d_ctx_set_line_params(struct grate_3d_ctx *ctx, uint32_t params)
{
	ctx->line_params = params;
}

void grate_3d_ctx_set_line_width(struct grate_3d_ctx *ctx, float width)
{
	ctx->line_width = width;
}

void grate_3d_ctx_use_guardband(struct grate_3d_ctx *ctx, bool enabled)
{
	ctx->guarband_enabled = enabled;
}

void grate_3d_ctx_set_front_direction_is_cw(struct grate_3d_ctx *ctx,
					    bool front_cw)
{
	ctx->tri_face_front_cw = front_cw;
}

void grate_3d_ctx_set_cull_face(struct grate_3d_ctx *ctx,
                                enum grate_cull_face cull_face)
{
	ctx->cull_face = cull_face;
}

void grate_3d_ctx_set_scissor(struct grate_3d_ctx *ctx,
			      unsigned x, unsigned width,
			      unsigned y, unsigned height)
{
	ctx->scissor_x = x;
	ctx->scissor_y = y;
	ctx->scissor_width = width;
	ctx->scissor_heigth = height;
}

void grate_3d_ctx_set_point_coord_range(struct grate_3d_ctx *ctx,
					float min_s, float max_s,
					float min_t, float max_t)
{
	ctx->point_coord_range_min_s = min_s;
	ctx->point_coord_range_max_s = max_s;
	ctx->point_coord_range_min_t = min_t;
	ctx->point_coord_range_max_t = max_t;
}

void grate_3d_ctx_set_polygon_offset(struct grate_3d_ctx *ctx,
				     float units, float factor)
{
	ctx->polygon_offset_units = units;
	ctx->polygon_offset_factor = factor;
}

void grate_3d_ctx_set_provoking_vtx_last(struct grate_3d_ctx *ctx, bool last)
{
	ctx->provoking_vtx_last = last;
}

int grate_3d_ctx_bind_texture(struct grate_3d_ctx *ctx,
			      unsigned location,
			      struct grate_texture *tex)
{
	if (location >= 16) {
		grate_error("Invalid location %u\n", location);
		return -1;
	}

	ctx->textures[location] = tex;

	return 0;
}
