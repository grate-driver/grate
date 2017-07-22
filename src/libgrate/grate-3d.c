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

#include <math.h>

#include "../libhost1x/host1x-private.h"
#include "libgrate-private.h"

#include "host1x.h"
#include "grate.h"
#include "grate-3d.h"
#include "tgr_3d.xml.h"

#define TGR3D_VAL(reg_name, field_name, value) \
	(((value) << TGR3D_ ## reg_name ## _ ## field_name ## __SHIFT) & \
		     TGR3D_ ## reg_name ## _ ## field_name ## __MASK)

#define TGR3D_BOOL(reg_name, field_name, boolean) \
	((boolean) ? TGR3D_ ## reg_name ## _ ## field_name : 0)

static void grate_shader_emit(struct host1x_pushbuf *pb,
			      struct grate_shader *shader)
{
	unsigned i;

	for (i = 0; i < shader->num_words; i++)
		host1x_pushbuf_push(pb, shader->words[i]);
}

static void grate_3d_begin(struct host1x_pushbuf *pb)
{
	host1x_pushbuf_push(pb,
			    HOST1X_OPCODE_SETCL(0x0, HOST1X_CLASS_GR3D, 0x0));
}

static void grate_3d_set_depth_range(struct host1x_pushbuf *pb,
				     struct grate_3d_ctx *ctx)
{
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_DEPTH_RANGE_NEAR, 2));
	host1x_pushbuf_push(pb, (uint32_t)(0xFFFFF * ctx->depth_range_near));
	host1x_pushbuf_push(pb, (uint32_t)(0xFFFFF * ctx->depth_range_far));
}

static void grate_3d_set_dither(struct host1x_pushbuf *pb,
				struct grate_3d_ctx *ctx)
{
	host1x_pushbuf_push(pb,
			    HOST1X_OPCODE_IMM(TGR3D_DITHER, ctx->dither_unk));
}

static void grate_3d_set_viewport_bias_scale(struct host1x_pushbuf *pb,
					     struct grate_3d_ctx *ctx)
{
	host1x_pushbuf_push(pb,
			    HOST1X_OPCODE_INCR(TGR3D_VIEWPORT_X_BIAS, 6));

	/* X bias */
	host1x_pushbuf_push_float(pb,
		ctx->viewport_x_bias * 16.0f + ctx->viewport_x_scale * 8.0f);

	/* Y bias */
	host1x_pushbuf_push_float(pb,
		ctx->viewport_y_bias * 16.0f + ctx->viewport_y_scale * 8.0f);

	/* Z bias */
	host1x_pushbuf_push_float(pb, ctx->viewport_z_bias - powf(2.0f, -21));

	/* X scale */
	host1x_pushbuf_push_float(pb, ctx->viewport_x_scale * 8.0f);

	/* Y scale */
	host1x_pushbuf_push_float(pb, ctx->viewport_y_scale * 8.0f);

	/* Z scale */
	host1x_pushbuf_push_float(pb, ctx->viewport_z_scale - powf(2.0f, -21));
}

static void grate_3d_set_point_params(struct host1x_pushbuf *pb,
				      struct grate_3d_ctx *ctx)
{
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_POINT_PARAMS, 1));
	host1x_pushbuf_push(pb, ctx->point_params);
}

static void grate_3d_set_point_size(struct host1x_pushbuf *pb,
				    struct grate_3d_ctx *ctx)
{
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_POINT_SIZE, 1));
	host1x_pushbuf_push_float(pb, ctx->point_size);
}

static void grate_3d_set_line_params(struct host1x_pushbuf *pb,
				     struct grate_3d_ctx *ctx)
{
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_LINE_PARAMS, 1));
	host1x_pushbuf_push(pb, ctx->line_params);
}

static void grate_3d_set_line_width(struct host1x_pushbuf *pb,
				    struct grate_3d_ctx *ctx)
{
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_HALF_LINE_WIDTH, 1));
	host1x_pushbuf_push_float(pb, ctx->line_width * 0.5f);
}

static void grate_3d_set_guardband(struct host1x_pushbuf *pb,
				   struct grate_3d_ctx *ctx)
{
	float x = ctx->viewport_x_scale;
	float y = ctx->viewport_y_scale;
	float z = ctx->viewport_z_scale;

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_GUARDBAND_WIDTH, 3));

	if (ctx->guarband_enabled) {
		host1x_pushbuf_push_float(pb, (4096.0f - x / 2.0f) / (x / 2.0f));
		host1x_pushbuf_push_float(pb, (4096.0f - y / 2.0f) / (y / 2.0f));
		host1x_pushbuf_push_float(pb, (4.0f - z) / z - powf(2.0f, -12));
	} else {
		host1x_pushbuf_push_float(pb, 1.0f);
		host1x_pushbuf_push_float(pb, 1.0f);
		host1x_pushbuf_push_float(pb, 1.0f);
	}
}

static void grate_3d_set_cull_face_and_linker_inst_nb(struct host1x_pushbuf *pb,
						      struct grate_3d_ctx *ctx)
{
	uint32_t unk = 0x2E38;
	uint32_t value = 0;

	value |= TGR3D_BOOL(CULL_FACE_LINKER_SETUP, FRONT_CW,
			    ctx->tri_face_front_cw);
	value |= TGR3D_VAL(CULL_FACE_LINKER_SETUP, CULL_FACE,
			   ctx->cull_face);

	value |= TGR3D_VAL(CULL_FACE_LINKER_SETUP, LINKER_INST_COUNT,
			   ctx->program->linker->linker_inst_nb - 1);
	value |= TGR3D_VAL(CULL_FACE_LINKER_SETUP, UNK_18_31, unk);

	host1x_pushbuf_push(pb,
			HOST1X_OPCODE_INCR(TGR3D_CULL_FACE_LINKER_SETUP, 1));

	host1x_pushbuf_push(pb, value);
}

static void grate_3d_set_scissor(struct host1x_pushbuf *pb,
				 struct grate_3d_ctx *ctx)
{
	uint32_t value;

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_SCISSOR_HORIZ, 2));

	value  = TGR3D_VAL(SCISSOR_HORIZ, MIN, ctx->scissor_x);
	value |= TGR3D_VAL(SCISSOR_HORIZ, MAX,
			   ctx->scissor_x + ctx->scissor_width);

	host1x_pushbuf_push(pb, value);

	value  = TGR3D_VAL(SCISSOR_VERT, MIN, ctx->scissor_y);
	value |= TGR3D_VAL(SCISSOR_VERT, MAX,
			   ctx->scissor_y + ctx->scissor_heigth);

	host1x_pushbuf_push(pb, value);
}

static int grate_3d_set_render_target_params(struct host1x_pushbuf *pb,
					     unsigned index,
					     bool depth_test,
					     bool stencil_test,
					     bool enable_dither,
					     struct host1x_pixelbuffer *pixbuf)
{
	unsigned pixel_format;
	uint32_t value = 0;

	switch (pixbuf->format) {
	case PIX_BUF_FMT_A8:
		pixel_format = TGR3D_PIXEL_FORMAT_A8;
		break;
	case PIX_BUF_FMT_L8:
		pixel_format = TGR3D_PIXEL_FORMAT_L8;
		break;
	case PIX_BUF_FMT_S8:
		pixel_format = TGR3D_PIXEL_FORMAT_S8;
		break;
	case PIX_BUF_FMT_LA88:
		pixel_format = TGR3D_PIXEL_FORMAT_LA88;
		break;
	case PIX_BUF_FMT_RGB565:
		pixel_format = TGR3D_PIXEL_FORMAT_RGB565;
		break;
	case PIX_BUF_FMT_RGBA5551:
		pixel_format = TGR3D_PIXEL_FORMAT_RGBA5551;
		break;
	case PIX_BUF_FMT_RGBA4444:
		pixel_format = TGR3D_PIXEL_FORMAT_RGBA4444;
		break;
	case PIX_BUF_FMT_D16_LINEAR:
		pixel_format = TGR3D_PIXEL_FORMAT_D16_LINEAR;
		break;
	case PIX_BUF_FMT_D16_NONLINEAR:
		pixel_format = TGR3D_PIXEL_FORMAT_D16_NONLINEAR;
		break;
	case PIX_BUF_FMT_RGBA8888:
		pixel_format = TGR3D_PIXEL_FORMAT_RGBA8888;
		break;
	case PIX_BUF_FMT_RGBA_FP32:
		pixel_format = TGR3D_PIXEL_FORMAT_RGBA_FP32;
		break;
	default:
		grate_error("Invalid format %u\n", pixbuf->format);
		return -1;
	}

	if (index == 0 && depth_test) {
		switch (pixel_format) {
		case TGR3D_PIXEL_FORMAT_D16_LINEAR:
		case TGR3D_PIXEL_FORMAT_D16_NONLINEAR:
			break;
		default:
			grate_error("Invalid depth buffer format %u\n",
				    pixbuf->format);
			return -1;
		}
	}

	if (index == 2 && stencil_test) {
		switch (pixel_format) {
		case TGR3D_PIXEL_FORMAT_S8:
			break;
		default:
			grate_error("Invalid stencil buffer format %u\n",
				    pixbuf->format);
			return -1;
		}
	}

	switch (pixbuf->layout) {
	case PIX_BUF_LAYOUT_LINEAR:
	case PIX_BUF_LAYOUT_TILED_16x16:
		break;
	default:
		grate_error("Invalid layout %u\n", pixbuf->layout);
		return -1;
	}

	value |= TGR3D_BOOL(RT_PARAMS, DITHER_ENABLE, enable_dither);
	value |= TGR3D_VAL(RT_PARAMS, FORMAT, pixel_format);
	value |= TGR3D_VAL(RT_PARAMS, PITCH, pixbuf->pitch);
	value |= TGR3D_BOOL(RT_PARAMS, TILED,
			    pixbuf->layout == PIX_BUF_LAYOUT_TILED_16x16);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_RT_PARAMS(index), 1));
	host1x_pushbuf_push(pb, value);

	return 0;
}

static void grate_3d_enable_render_targets(struct host1x_pushbuf *pb,
					   uint32_t mask)
{
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_RT_ENABLE, 1));
	host1x_pushbuf_push(pb, mask);
}

static void grate_3d_relocate_render_target(struct host1x_pushbuf *pb,
					    unsigned index,
					    struct host1x_bo *bo,
					    unsigned offset)
{
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_RT_PTR(index), 1));
	HOST1X_PUSHBUF_RELOCATE(pb, bo, offset, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
}

static void grate_3d_set_point_coord_range(struct host1x_pushbuf *pb,
					   struct grate_3d_ctx *ctx)
{
	host1x_pushbuf_push(pb,
			HOST1X_OPCODE_INCR(TGR3D_POINT_COORD_RANGE_MAX_S, 4));
	host1x_pushbuf_push_float(pb, ctx->point_coord_range_max_s);
	host1x_pushbuf_push_float(pb, ctx->point_coord_range_max_t);
	host1x_pushbuf_push_float(pb, ctx->point_coord_range_min_s);
	host1x_pushbuf_push_float(pb, ctx->point_coord_range_min_t);
}

static void grate_3d_set_alu_buffer_size(struct host1x_pushbuf *pb,
					 struct grate_3d_ctx *ctx)
{
	unsigned alu_buf_size = ctx->program->fs->alu_buf_size;
	unsigned unk_pseq_cfg = 0x12C;
	uint32_t value = 0;

	value |= TGR3D_VAL(ALU_BUFFER_SIZE, SIZE, alu_buf_size - 1);
	value |= TGR3D_VAL(ALU_BUFFER_SIZE, SIZEx4,
			   (unk_pseq_cfg - 1) / (alu_buf_size * 4));

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_ALU_BUFFER_SIZE, 1));
	host1x_pushbuf_push(pb, value);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x501, 1));
	host1x_pushbuf_push(pb, (0x0032 << 16) | (unk_pseq_cfg << 4) | 0xF);
}

static void grate_3d_set_pseq_dw_cfg(struct host1x_pushbuf *pb,
				     struct grate_3d_ctx *ctx)
{
	uint32_t value = TGR3D_VAL(FP_PSEQ_DW_CFG, PSEQ_TO_DW_EXEC_NB,
				   ctx->program->fs->pseq_to_dw_nb);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_FP_PSEQ_DW_CFG, 1));
	host1x_pushbuf_push(pb, value);
}

static void grate_3d_set_used_tram_rows_nb(struct host1x_pushbuf *pb,
					   struct grate_3d_ctx *ctx)
{
	uint32_t value = 0;

	value |= TGR3D_VAL(TRAM_SETUP, USED_TRAM_ROWS_NB,
			   ctx->program->linker->used_tram_rows_nb);
	value |= TGR3D_VAL(TRAM_SETUP, DIV64,
			   64 / ctx->program->linker->used_tram_rows_nb);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_TRAM_SETUP, 1));
	host1x_pushbuf_push(pb, value);
}

static void grate_3d_upload_vp_constants(struct host1x_pushbuf *pb,
					 struct grate_3d_ctx *ctx)
{
	unsigned i;

	host1x_pushbuf_push(pb,
			HOST1X_OPCODE_IMM(TGR3D_VP_UPLOAD_CONST_ID, 0));

	host1x_pushbuf_push(pb,
			HOST1X_OPCODE_NONINCR(TGR3D_VP_UPLOAD_CONST, 256 * 4));

	for (i = 0; i < 256 * 4; i++)
		host1x_pushbuf_push(pb, ctx->vs_uniforms[i]);
}

static void grate_3d_upload_fp_constants(struct host1x_pushbuf *pb,
					 struct grate_3d_ctx *ctx)
{
	unsigned i;

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_FP_CONST(0), 32));

	for (i = 0; i < 32; i++)
		host1x_pushbuf_push(pb, ctx->fs_uniforms[i]);
}

static void grate_3d_set_polygon_offset(struct host1x_pushbuf *pb,
					struct grate_3d_ctx *ctx)
{
	host1x_pushbuf_push(pb,
			    HOST1X_OPCODE_INCR(TGR3D_POLYGON_OFFSET_UNITS, 2));
	host1x_pushbuf_push_float(pb, ctx->polygon_offset_units);
	host1x_pushbuf_push_float(pb, ctx->polygon_offset_factor);
}

static void grate_3d_relocate_primitive_indices(struct host1x_pushbuf *pb,
						struct host1x_bo *indices,
						unsigned offset)
{
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_INDEX_PTR, 1));
	HOST1X_PUSHBUF_RELOCATE(pb, indices, offset, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
}

static void grate_3d_set_draw_params(struct host1x_pushbuf *pb,
				     struct grate_3d_ctx *ctx,
				     unsigned primitive_type,
				     unsigned index_mode)
{
	unsigned first_vtx = 0;
	uint32_t value = 0;

	value |= TGR3D_VAL(DRAW_PARAMS, INDEX_MODE, index_mode);
	value |= TGR3D_VAL(DRAW_PARAMS, PROVOKING_VERTEX,
			   !!ctx->provoking_vtx_last);
	value |= TGR3D_VAL(DRAW_PARAMS, PRIMITIVE_TYPE, primitive_type);
	value |= TGR3D_VAL(DRAW_PARAMS, FIRST, first_vtx);
	value |= 0xC0000000;

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_DRAW_PARAMS, 1));
	host1x_pushbuf_push(pb, value);
}

static void grate_3d_draw_primitives(struct host1x_pushbuf *pb,
				     unsigned index_count)
{
	unsigned first_index = 0;
	uint32_t value = 0;

	value |= TGR3D_VAL(DRAW_PRIMITIVES, INDEX_COUNT, index_count - 1);
	value |= TGR3D_VAL(DRAW_PRIMITIVES, OFFSET, first_index);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_DRAW_PRIMITIVES, 1));
	host1x_pushbuf_push(pb, value);
}

static void grate_3d_init(struct host1x_pushbuf *pb)
{
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x503, 0x00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x545, 0x00));

	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x740, 0x035));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe27, 0x01));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0xa02, 0x06));
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x000001ff);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, 0x00000020);
	host1x_pushbuf_push(pb, 0x00000030);
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa00, 0xe01));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xa08, 0x100));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x40c, 0x06));
}

static void grate_3d_reset_program(struct host1x_pushbuf *pb)
{
	/* reset upload counters */
	host1x_pushbuf_push(pb,
			    HOST1X_OPCODE_IMM(TGR3D_VP_UPLOAD_INST_ID, 0));
	host1x_pushbuf_push(pb,
			    HOST1X_OPCODE_IMM(TGR3D_FP_UPLOAD_INST_ID_COMMON, 0));
	host1x_pushbuf_push(pb,
			    HOST1X_OPCODE_IMM(TGR3D_FP_UPLOAD_MFU_INST_ID, 0));
	host1x_pushbuf_push(pb,
			    HOST1X_OPCODE_IMM(TGR3D_FP_UPLOAD_ALU_INST_ID, 0));
}


static void grate_3d_startup_pseq_engine(struct host1x_pushbuf *pb,
					 struct grate_3d_ctx *ctx)
{
	host1x_pushbuf_push(pb,
			    HOST1X_OPCODE_INCR(TGR3D_FP_PSEQ_ENGINE_INST, 1));
	host1x_pushbuf_push(pb, 0x20006000 | ctx->program->fs->pseq_inst_nb);
}

static void grate_3d_set_late_test(struct host1x_pushbuf *pb,
				   struct grate_3d_ctx *ctx)
{
	bool discard = ctx->program->fs->discards_fragment;
	uint32_t value = 0x48;

	if (discard && ctx->stencil_test)
		value = 0x60;

	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x40f, value));
}

static void grate_3d_set_depth_buffer(struct host1x_pushbuf *pb,
				      struct grate_3d_ctx *ctx)
{
	uint32_t value = 0;

	value |= TGR3D_VAL(DEPTH_TEST_PARAMS, FUNC, ctx->depth_func);
	value |= TGR3D_BOOL(DEPTH_TEST_PARAMS, DEPTH_TEST, ctx->depth_test);
	value |= TGR3D_BOOL(DEPTH_TEST_PARAMS, DEPTH_WRITE, ctx->depth_write);
	value |= 0x200;

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_DEPTH_TEST_PARAMS, 1));
	host1x_pushbuf_push(pb, value);
}

static void grate_3d_set_stencil_test(struct host1x_pushbuf *pb,
				      struct grate_3d_ctx *ctx)
{
	uint32_t value;

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_STENCIL_FRONT1, 3));

	value  = TGR3D_VAL(STENCIL_FRONT1, MASK, ctx->stencil_mask_front);
	value |= TGR3D_VAL(STENCIL_FRONT1, FUNC, ctx->stencil_func_front);
	host1x_pushbuf_push(pb, value);

	value  = TGR3D_VAL(STENCIL_BACK1, MASK, ctx->stencil_mask_back);
	value |= TGR3D_VAL(STENCIL_BACK1, FUNC, ctx->stencil_func_back);
	host1x_pushbuf_push(pb, value);

	value  = TGR3D_BOOL(STENCIL_PARAMS, STENCIL_TEST, ctx->stencil_test);
	value |= TGR3D_BOOL(STENCIL_PARAMS, STENCIL_WRITE_EARLY,
			    !ctx->program->fs->discards_fragment);
	value |= 0x8;
	host1x_pushbuf_push(pb, value);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_STENCIL_FRONT2, 2));

	value  = TGR3D_VAL(STENCIL_FRONT2, REF, ctx->stencil_ref_front);
	value |= TGR3D_VAL(STENCIL_FRONT2, OP_FAIL, ctx->stencil_fail_op_front);
	value |= TGR3D_VAL(STENCIL_FRONT2, OP_ZFAIL, ctx->stencil_zfail_op_front);
	value |= TGR3D_VAL(STENCIL_FRONT2, OP_ZPASS, ctx->stencil_zpass_op_front);
	value |= 0x1fe00;
	host1x_pushbuf_push(pb, value);

	value  = TGR3D_VAL(STENCIL_BACK2, REF, ctx->stencil_ref_back);
	value |= TGR3D_VAL(STENCIL_BACK2, OP_FAIL, ctx->stencil_fail_op_back);
	value |= TGR3D_VAL(STENCIL_BACK2, OP_ZFAIL, ctx->stencil_zfail_op_back);
	value |= TGR3D_VAL(STENCIL_BACK2, OP_ZPASS, ctx->stencil_zpass_op_back);
	value |= 0x1fe00;
	host1x_pushbuf_push(pb, value);
}

static void grate_3d_set_vp_attributes_in_out_mask(struct host1x_pushbuf *pb,
						   uint32_t in_mask,
						   uint32_t out_mask)
{
	host1x_pushbuf_push(pb,
			HOST1X_OPCODE_INCR(TGR3D_VP_ATTRIB_IN_OUT_SELECT, 1));
	host1x_pushbuf_push(pb, in_mask << 16 | out_mask);
}

static void grate_3d_set_attribute(struct host1x_pushbuf *pb,
				   unsigned index,
				   struct host1x_bo *bo,
				   unsigned offset, unsigned type,
				   unsigned size, unsigned stride)
{
	uint32_t value = 0;

	value |= TGR3D_VAL(ATTRIB_MODE, TYPE, type);
	value |= TGR3D_VAL(ATTRIB_MODE, SIZE, size);
	value |= TGR3D_VAL(ATTRIB_MODE, STRIDE, stride);

	host1x_pushbuf_push(pb,
			    HOST1X_OPCODE_INCR(TGR3D_ATTRIB_PTR(index), 2));

	HOST1X_PUSHBUF_RELOCATE(pb, bo, offset, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
	host1x_pushbuf_push(pb, value);
}

static void grate_3d_setup_attributes(struct host1x_pushbuf *pb,
				      struct grate_3d_ctx *ctx)
{
	uint32_t enable_mask = ctx->program->attributes_use_mask;
	uint32_t out_mask = enable_mask & 0xFFFF;
	uint32_t in_mask = enable_mask >> 16;
	unsigned i;

	in_mask &= ctx->attributes_enable_mask;

	for (i = 0; i < 16; i++) {
		struct grate_vtx_attribute *attr = ctx->vtx_attributes[i];

		if (!(in_mask & (1u << i)))
			continue;

		if (!attr) {
			in_mask &= ~(1u << i);
			continue;
		}

		grate_3d_set_attribute(pb, i,
				       attr->bo,
				       attr->bo->offset,
				       attr->type,
				       attr->size,
				       attr->stride);
	}

	grate_3d_set_vp_attributes_in_out_mask(pb, in_mask, out_mask);
}

static void grate_3d_setup_render_targets(struct host1x_pushbuf *pb,
					  struct grate_3d_ctx *ctx)
{
	uint32_t enable_mask = 0;
	unsigned i;

	for (i = 0; i < 16; i++) {
		struct grate_render_target *rt = &ctx->render_targets[i];

		/*
		 * Render target 0 is used for depth buffer inclusively,
		 * depth write is performed only if depth test is enabled.
		 */
		if (i == 0 && ctx->depth_test)
			goto pixbuf_check;

		if (i == 2 && ctx->stencil_test)
			goto pixbuf_check;

		if (!(ctx->render_targets_enable_mask & (1u << i)))
			continue;
pixbuf_check:
		if (!rt->pixbuf)
			continue;

		if (grate_3d_set_render_target_params(pb, i,
						      ctx->depth_test,
						      ctx->stencil_test,
						      rt->dither_enabled,
						      rt->pixbuf))
			continue;

		grate_3d_relocate_render_target(pb, i,
						rt->pixbuf->bo,
						rt->pixbuf->bo->offset);

		if (i == 0 && ctx->depth_test)
			enable_mask |= TGR3D_RT_ENABLE_DEPTH_BUFFER;

		enable_mask |= 1u << i;
	}

	grate_3d_enable_render_targets(pb, enable_mask);
}

static void grate_3d_disable_texture(struct host1x_pushbuf *pb, unsigned index)
{
	host1x_pushbuf_push(pb,
			    HOST1X_OPCODE_INCR(TGR3D_TEXTURE_DESC1(index), 2));
	host1x_pushbuf_push(pb, 0);
	host1x_pushbuf_push(pb, 0);
}

static void grate_3d_relocate_texture(struct host1x_pushbuf *pb,
				      unsigned index,
				      struct host1x_bo *bo,
				      unsigned offset)
{
	host1x_pushbuf_push(pb,
			    HOST1X_OPCODE_INCR(TGR3D_TEXTURE_POINTER(index), 1));
	HOST1X_PUSHBUF_RELOCATE(pb, bo, offset, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
}

static void grate_3d_set_texture_desc(struct host1x_pushbuf *pb,
				      unsigned index,
				      struct host1x_pixelbuffer *pixbuf,
				      unsigned max_lod,
				      bool wrap_t_clamp_to_edge,
				      bool wrap_s_clamp_to_edge,
				      bool wrap_t_mirrored_repeat,
				      bool wrap_s_mirrored_repeat,
				      bool mipmap_enabled,
				      bool min_filter_enabled,
				      bool mip_filter_enabled,
				      bool mag_filter_enabled)
{
	int log2_width = log2_size(pixbuf->width);
	int log2_height = log2_size(pixbuf->height);
	unsigned pixel_format;
	uint32_t value = 0;

	switch (pixbuf->format) {
	case PIX_BUF_FMT_A8:
		pixel_format = TGR3D_PIXEL_FORMAT_A8;
		break;
	case PIX_BUF_FMT_L8:
		pixel_format = TGR3D_PIXEL_FORMAT_L8;
		break;
	case PIX_BUF_FMT_S8:
		pixel_format = TGR3D_PIXEL_FORMAT_S8;
		break;
	case PIX_BUF_FMT_LA88:
		pixel_format = TGR3D_PIXEL_FORMAT_LA88;
		break;
	case PIX_BUF_FMT_RGB565:
		pixel_format = TGR3D_PIXEL_FORMAT_RGB565;
		break;
	case PIX_BUF_FMT_RGBA5551:
		pixel_format = TGR3D_PIXEL_FORMAT_RGBA5551;
		break;
	case PIX_BUF_FMT_RGBA4444:
		pixel_format = TGR3D_PIXEL_FORMAT_RGBA4444;
		break;
	case PIX_BUF_FMT_D16_LINEAR:
		pixel_format = TGR3D_PIXEL_FORMAT_D16_LINEAR;
		break;
	case PIX_BUF_FMT_D16_NONLINEAR:
		pixel_format = TGR3D_PIXEL_FORMAT_D16_NONLINEAR;
		break;
	case PIX_BUF_FMT_RGBA8888:
		pixel_format = TGR3D_PIXEL_FORMAT_RGBA8888;
		break;
	case PIX_BUF_FMT_RGBA_FP32:
		pixel_format = TGR3D_PIXEL_FORMAT_RGBA_FP32;
		break;
	default:
		grate_error("Invalid format %u\n", pixbuf->format);
		return;
	}

	switch (pixbuf->layout) {
	case PIX_BUF_LAYOUT_LINEAR:
		break;
	default:
		grate_error("Invalid layout %u\n", pixbuf->layout);
		return;
	}

	host1x_pushbuf_push(pb,
			    HOST1X_OPCODE_INCR(TGR3D_TEXTURE_DESC1(index), 2));

	value  = TGR3D_VAL(TEXTURE_DESC1, FORMAT, pixel_format);
	value |= TGR3D_BOOL(TEXTURE_DESC1, MINFILTER_LINEAR_WITHIN,
			    min_filter_enabled);
	value |= TGR3D_BOOL(TEXTURE_DESC1, MINFILTER_LINEAR_BETWEEN,
			    mip_filter_enabled);
	value |= TGR3D_BOOL(TEXTURE_DESC1, MAGFILTER_LINEAR,
			    mag_filter_enabled);
	value |= TGR3D_BOOL(TEXTURE_DESC1, WRAP_T_CLAMP_TO_EDGE,
			    wrap_t_clamp_to_edge);
	value |= TGR3D_BOOL(TEXTURE_DESC1, WRAP_S_CLAMP_TO_EDGE,
			    wrap_s_clamp_to_edge);
	value |= TGR3D_BOOL(TEXTURE_DESC1, WRAP_T_MIRRORED_REPEAT,
			    wrap_t_mirrored_repeat);
	value |= TGR3D_BOOL(TEXTURE_DESC1, WRAP_S_MIRRORED_REPEAT,
			    wrap_s_mirrored_repeat);
// 	value |= 0x50;

	host1x_pushbuf_push(pb, value);

	value = TGR3D_BOOL(TEXTURE_DESC2, MIPMAP_DISABLE, !mipmap_enabled);

	if (pixbuf->width == 1 << log2_width &&
	    pixbuf->height == 1 << log2_height) {
		value |= TGR3D_VAL(TEXTURE_DESC2, MAX_LOD, max_lod);
		value |= TGR3D_VAL(TEXTURE_DESC2, WIDTH_LOG2, log2_width);
		value |= TGR3D_VAL(TEXTURE_DESC2, HEIGHT_LOG2, log2_height);
	} else {
		value |= TGR3D_BOOL(TEXTURE_DESC2, NOT_POW2_DIMENSIONS, 1);
		value |= TGR3D_VAL(TEXTURE_DESC2, WIDTH, pixbuf->width);
		value |= TGR3D_VAL(TEXTURE_DESC2, HEIGHT, pixbuf->height);
	}

	host1x_pushbuf_push(pb, value);
}

static void grate_3d_setup_textures(struct host1x_pushbuf *pb,
				    struct grate_3d_ctx *ctx)
{
	unsigned i;

	for (i = 0; i < 16; i++) {
		struct grate_texture *tex = ctx->textures[i];
		struct host1x_pixelbuffer *pixbuf;

		if (!tex) {
			grate_3d_disable_texture(pb, i);
			continue;
		}

		if (tex->mipmap_enabled)
			pixbuf = tex->mipmap_pixbuf;
		else
			pixbuf = tex->pixbuf;

		if (!pixbuf) {
			grate_3d_disable_texture(pb, i);
			continue;
		}

		grate_3d_relocate_texture(pb, i,
					  pixbuf->bo,
					  pixbuf->bo->offset);

		grate_3d_set_texture_desc(pb, i,
					  pixbuf,
					  tex->max_lod,
					  tex->wrap_t_clamp_to_edge,
					  tex->wrap_s_clamp_to_edge,
					  tex->wrap_t_mirrored_repeat,
					  tex->wrap_s_mirrored_repeat,
					  tex->mipmap_enabled,
					  tex->min_filter_enabled,
					  tex->mip_filter_enabled,
					  tex->mag_filter_enabled);
	}
}

static void grate_3d_setup_indices(struct host1x_pushbuf *pb,
				   struct host1x_bo *bo,
				   unsigned index_mode)
{
	if (index_mode == TGR3D_INDEX_MODE_NONE)
		return;

	grate_3d_relocate_primitive_indices(pb, bo, bo->offset);
}

static void grate_3d_setup_context(struct host1x_pushbuf *pb,
				   struct grate_3d_ctx *ctx)
{
	grate_3d_begin(pb);
	grate_3d_init(pb);

	grate_3d_set_dither(pb, ctx);
	grate_3d_set_scissor(pb, ctx);
	grate_3d_set_guardband(pb, ctx);
	grate_3d_set_late_test(pb, ctx);
	grate_3d_set_point_size(pb, ctx);
	grate_3d_set_line_width(pb, ctx);
	grate_3d_set_line_params(pb, ctx);
	grate_3d_set_pseq_dw_cfg(pb, ctx);
	grate_3d_set_depth_range(pb, ctx);
	grate_3d_set_point_params(pb, ctx);
	grate_3d_set_depth_buffer(pb, ctx);
	grate_3d_set_stencil_test(pb, ctx);
	grate_3d_set_polygon_offset(pb, ctx);
	grate_3d_set_alu_buffer_size(pb, ctx);
	grate_3d_startup_pseq_engine(pb, ctx);
	grate_3d_set_point_coord_range(pb, ctx);
	grate_3d_set_used_tram_rows_nb(pb, ctx);
	grate_3d_set_viewport_bias_scale(pb, ctx);
	grate_3d_set_cull_face_and_linker_inst_nb(pb, ctx);

	grate_3d_upload_vp_constants(pb, ctx);
	grate_3d_upload_fp_constants(pb, ctx);

	grate_3d_setup_attributes(pb, ctx);
	grate_3d_setup_render_targets(pb, ctx);
	grate_3d_setup_textures(pb, ctx);

	grate_3d_reset_program(pb);
	grate_shader_emit(pb, ctx->program->vs);
	grate_shader_emit(pb, ctx->program->fs);
	grate_shader_emit(pb, ctx->program->linker);
}

static void grate_3d_check_render_targets_guard(struct grate_3d_ctx *ctx)
{
	unsigned i;

	for (i = 0; i < 16; i++) {
		struct grate_render_target *rt = &ctx->render_targets[i];

		/*
		* Render target 0 is used for depth buffer inclusively,
		* depth write is performed only if depth test is enabled.
		*/
		if (i == 0 && ctx->depth_test)
			goto pixbuf_check;

		if (i == 2 && ctx->stencil_test)
			goto pixbuf_check;

		if (!(ctx->render_targets_enable_mask & (1u << i)))
			continue;
pixbuf_check:
		if (!rt->pixbuf)
			continue;

		host1x_pixelbuffer_check_guard(rt->pixbuf);
	}
}

void grate_3d_draw_elements(struct grate_3d_ctx *ctx,
			    unsigned primitive_type,
			    struct host1x_bo *indices_bo,
			    unsigned index_mode,
			    unsigned vtx_count)
{
	struct grate *grate = ctx->grate;
	struct host1x_gr3d *gr3d = host1x_get_gr3d(grate->host1x);
	struct host1x_syncpt *syncpt = &gr3d->client->syncpts[0];
	struct host1x_pushbuf *pb;
	struct host1x_job *job;
	uint32_t fence;
	int err;

	if (!ctx->program) {
		grate_error("No program bound\n");
		return;
	}

	if (!ctx->program->vs || !ctx->program->fs || !ctx->program->linker) {
		grate_error("Program wasn't compiled\n");
		return;
	}

	switch (primitive_type) {
	case TGR3D_PRIMITIVE_TYPE_POINTS:
	case TGR3D_PRIMITIVE_TYPE_LINES:
	case TGR3D_PRIMITIVE_TYPE_LINE_STRIP:
	case TGR3D_PRIMITIVE_TYPE_LINE_LOOP:
	case TGR3D_PRIMITIVE_TYPE_TRIANGLES:
	case TGR3D_PRIMITIVE_TYPE_TRIANGLE_STRIP:
	case TGR3D_PRIMITIVE_TYPE_TRIANGLE_FAN:
		break;
	default:
		grate_error("Unsupported primitive type: %d\n", primitive_type);
		return;
	}

	switch (index_mode) {
	case TGR3D_INDEX_MODE_NONE:
	case TGR3D_INDEX_MODE_UINT8:
	case TGR3D_INDEX_MODE_UINT16:
		break;
	default:
		grate_error("Invalid index buffer mode: %u\n", index_mode);
		return;
	}

	job = HOST1X_JOB_CREATE(syncpt->id, 1);
	if (!job)
		return;

	pb = HOST1X_JOB_APPEND(job, gr3d->commands, 0);
	if (!pb) {
		host1x_job_free(job);
		return;
	}

	grate_3d_setup_context(pb, ctx);
	grate_3d_setup_indices(pb, indices_bo, index_mode);
	grate_3d_set_draw_params(pb, ctx, primitive_type, index_mode);
	grate_3d_draw_primitives(pb, vtx_count);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x00, 0x01));
	host1x_pushbuf_push(pb, 0x000001 << 8 | syncpt->id);

	err = HOST1X_CLIENT_SUBMIT(gr3d->client, job);
	if (err < 0) {
		host1x_job_free(job);
		return;
	}

	host1x_job_free(job);

	err = HOST1X_CLIENT_FLUSH(gr3d->client, &fence);
	if (err < 0)
		return;

	err = HOST1X_CLIENT_WAIT(gr3d->client, fence, ~0u);
	if (err < 0)
		return;

	grate_3d_check_render_targets_guard(ctx);
}
