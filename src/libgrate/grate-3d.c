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

#include "grate.h"
#include "host1x.h"
#include "tgr_3d.xml.h"

#define TGR3D_VAL(reg_name, field_name, value) \
	(((value) << TGR3D_ ## reg_name ## _ ## field_name ## __SHIFT) & \
		     TGR3D_ ## reg_name ## _ ## field_name ## __MASK)

#define TGR3D_BOOL(reg_name, field_name, boolean) \
	((boolean) ? TGR3D_ ## reg_name ## _ ## field_name : 0)

static unsigned int count_pseq_instructions_nb(struct grate_shader *shader)
{
	unsigned int pseq_instructions_nb = 0;
	unsigned int i;

	for (i = 0; i < shader->num_words; i++) {
		uint32_t host1x_command = shader->words[i];
		unsigned int host1x_opcode = host1x_command >> 28;
		unsigned int offset = (host1x_command >> 16) & 0xfff;
		unsigned int count = host1x_command & 0xffff;
		unsigned int mask = count;

		switch (host1x_opcode) {
		case 0: /* SETCL */
			break;
		case 1: /* INCR */
			if (offset <= 0x541 && offset + count > 0x541)
				pseq_instructions_nb++;
			i += count;
			break;
		case 2: /* NONINCR */
			if (offset == 0x541)
				pseq_instructions_nb += count;
			i += count;
			break;
		case 3: /* MASK */
			for (count = 0; count < 16; count++) {
				if (mask & (1u << count)) {
					if (offset + count == 0x541)
						pseq_instructions_nb++;
					i++;
				}
			}
			break;
		case 4: /* IMM */
			if (offset == 0x541)
				pseq_instructions_nb++;
			break;
		case 5: /* EXTEND */
			break;
		default:
			fprintf(stderr,
				"ERROR: fragment shader host1x command "
					"stream is invalid\n");
			break;
		}
	}

	return pseq_instructions_nb;
}

unsigned linker_instructions_nb(struct grate_shader *linker)
{
	return (linker->num_words - 3) / 2;
}

void grate_3d_begin(struct host1x_pushbuf *pb)
{
	host1x_pushbuf_push(pb,
			    HOST1X_OPCODE_SETCL(0x0, HOST1X_CLASS_GR3D, 0x0));
}

void grate_3d_set_depth_range(struct host1x_pushbuf *pb,
			      float near, float far)
{
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_DEPTH_RANGE_NEAR, 2));
	host1x_pushbuf_push(pb, (uint32_t)(0xFFFFF * near));
	host1x_pushbuf_push(pb, (uint32_t)(0xFFFFF * far));
}

void grate_3d_set_dither(struct host1x_pushbuf *pb, uint32_t unk)
{
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(TGR3D_DITHER, unk));
}

void grate_3d_set_viewport(struct host1x_pushbuf *pb, float x, float y, float z,
			  float width, float height, float depth)
{
	host1x_pushbuf_push(pb,
			    HOST1X_OPCODE_INCR(TGR3D_VIEWPORT_X_BIAS, 6));

	/* X bias */
	host1x_pushbuf_push_float(pb, x * 16.0f + width * 8.0f);

	/* Y bias */
	host1x_pushbuf_push_float(pb, y * 16.0f + height * 8.0f);

	/* Z bias */
	host1x_pushbuf_push_float(pb, z - powf(2.0f, -21));

	/* X scale */
	host1x_pushbuf_push_float(pb, width * 8.0f);

	/* Y scale */
	host1x_pushbuf_push_float(pb, height * 8.0f);

	/* Z scale */
	host1x_pushbuf_push_float(pb, depth - powf(2.0f, -21));
}

void grate_3d_set_point_params(struct host1x_pushbuf *pb, uint32_t params)
{
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_POINT_PARAMS, 1));
	host1x_pushbuf_push(pb, params);
}

void grate_3d_set_point_size(struct host1x_pushbuf *pb, float size)
{
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_POINT_SIZE, 1));
	host1x_pushbuf_push_float(pb, size);
}

void grate_3d_set_line_params(struct host1x_pushbuf *pb, uint32_t params)
{
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_LINE_PARAMS, 1));
	host1x_pushbuf_push(pb, params);
}

void grate_3d_set_line_width(struct host1x_pushbuf *pb, float width)
{
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_HALF_LINE_WIDTH, 1));
	host1x_pushbuf_push_float(pb, width * 0.5f);
}

void grate_3d_set_guardband(struct host1x_pushbuf *pb,
			    float width, float height, float depth)
{
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_GUARDBAND_WIDTH, 3));

	/* Guardband X */
	host1x_pushbuf_push_float(pb,
				  (4096.0f - width / 2.0f) / (width / 2.0f));

	/* Guardband Y */
	host1x_pushbuf_push_float(pb,
				  (4096.0f - height / 2.0f) / (height / 2.0f));

	/* Guardband Z */
	host1x_pushbuf_push_float(pb, (4.0f - depth) / depth - powf(2.0f, -12));
}

void grate_3d_set_cull_face_and_linker_inst_nb(struct host1x_pushbuf *pb,
					       bool front_cw,
					       bool cull_ccw,
					       bool cull_cw,
					       unsigned linker_inst_nb,
					       unsigned unk)
{
	uint32_t value = 0;

	value |= TGR3D_BOOL(CULL_FACE_LINKER_SETUP, FRONT_CW, front_cw);
	value |= TGR3D_BOOL(CULL_FACE_LINKER_SETUP, CULL_CCW, cull_ccw);
	value |= TGR3D_BOOL(CULL_FACE_LINKER_SETUP, CULL_CW, cull_cw);
	value |= TGR3D_VAL(CULL_FACE_LINKER_SETUP, LINKER_INST_COUNT,
			   linker_inst_nb - 1);
	value |= TGR3D_VAL(CULL_FACE_LINKER_SETUP, UNK_18_31, unk);

	host1x_pushbuf_push(pb,
			HOST1X_OPCODE_INCR(TGR3D_CULL_FACE_LINKER_SETUP, 1));

	host1x_pushbuf_push(pb, value);
}

void grate_3d_set_scissor(struct host1x_pushbuf *pb,
			  unsigned x, unsigned width,
			  unsigned y, unsigned height)
{
	uint32_t value;

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_SCISSOR_HORIZ, 2));

	value  = TGR3D_VAL(SCISSOR_HORIZ, MIN, x);
	value |= TGR3D_VAL(SCISSOR_HORIZ, MAX, x + width);

	host1x_pushbuf_push(pb, value);

	value  = TGR3D_VAL(SCISSOR_VERT, MIN, y);
	value |= TGR3D_VAL(SCISSOR_VERT, MAX, y + height);

	host1x_pushbuf_push(pb, value);
}

void grate_3d_set_render_target_params(struct host1x_pushbuf *pb,
				       unsigned index,
				       bool enable_dither,
				       unsigned pixel_format,
				       unsigned pitch,
				       bool tiled)
{
	uint32_t value = 0;

	value |= TGR3D_BOOL(RT_PARAMS, DITHER_ENABLE, enable_dither);
	value |= TGR3D_VAL(RT_PARAMS, FORMAT, pixel_format);
	value |= TGR3D_VAL(RT_PARAMS, PITCH, pitch);
	value |= TGR3D_BOOL(RT_PARAMS, TILED, tiled);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_RT_PARAMS(index), 1));
	host1x_pushbuf_push(pb, value);
}

void grate_3d_set_render_targets_enable(struct host1x_pushbuf *pb,
					unsigned index)
{
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_RT_ENABLE, 1));
	host1x_pushbuf_push(pb, 1u << index);
}

void grate_3d_relocate_render_target(struct host1x_pushbuf *pb,
				     struct host1x_bo *target,
				     unsigned index)
{
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_RT_PTR(index), 1));
	host1x_pushbuf_relocate(pb, target, 0, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
}

void grate_3d_set_point_coord_range(struct host1x_pushbuf *pb,
				    float min_s, float max_s,
				    float min_t, float max_t)
{
	host1x_pushbuf_push(pb,
			HOST1X_OPCODE_INCR(TGR3D_POINT_COORD_RANGE_MAX_S, 4));
	host1x_pushbuf_push_float(pb, max_s);
	host1x_pushbuf_push_float(pb, max_t);
	host1x_pushbuf_push_float(pb, min_s);
	host1x_pushbuf_push_float(pb, min_t);
}

void grate_3d_set_alu_buffer_size(struct host1x_pushbuf *pb,
				  unsigned size, unsigned unk_pseq_cfg)
{
	uint32_t value = 0;

	value |= TGR3D_VAL(ALU_BUFFER_SIZE, SIZE, size - 1);
	value |= TGR3D_VAL(ALU_BUFFER_SIZE, SIZEx4,
			   (unk_pseq_cfg - 1) / (size * 4));

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_ALU_BUFFER_SIZE, 1));
	host1x_pushbuf_push(pb, value);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(0x501, 1));
	host1x_pushbuf_push(pb, (0x0032 << 16) | (unk_pseq_cfg << 4) | 0xF);
}

void grate_3d_set_used_tram_rows_nb(struct host1x_pushbuf *pb, unsigned nb)
{
	uint32_t value = 0;

	value |= TGR3D_VAL(TRAM_SETUP, USED_TRAM_ROWS_NB, nb);
	value |= TGR3D_VAL(TRAM_SETUP, DIV64, 64 / nb);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_TRAM_SETUP, 1));
	host1x_pushbuf_push(pb, value);
}

void grate_3d_set_pseq_dw_cfg(struct host1x_pushbuf *pb, unsigned exec_nb)
{
	/* Number of executed PSEQ instructions before DW */
	uint32_t value = TGR3D_VAL(PSEQ_DW_CFG, PSEQ_TO_DW_EXEC_NB, exec_nb);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_PSEQ_DW_CFG, 1));
	host1x_pushbuf_push(pb, value);
}

void grate_3d_upload_vp_consts(struct host1x_pushbuf *pb,
			       float *uniforms, unsigned nb)
{
	unsigned i;

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_VP_UPLOAD_INST_ID, 1));
	host1x_pushbuf_push(pb, 0);

	host1x_pushbuf_push(pb,
			HOST1X_OPCODE_NONINCR(TGR3D_VP_UPLOAD_CONST, nb));

	for (i = 0; i < nb; i++) {
		host1x_pushbuf_push_float(pb, uniforms[i]);
	}
}

void grate_3d_upload_fp_consts(struct host1x_pushbuf *pb,
			       uint32_t *uniforms, unsigned nb)
{
	unsigned i;

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_FP_CONST(0), nb));

	for (i = 0; i < nb; i++)
		host1x_pushbuf_push(pb, uniforms[i]);
}

void grate_3d_set_vp_attributes_in_out_mask(struct host1x_pushbuf *pb,
					    unsigned attributes_mask)
{
	host1x_pushbuf_push(pb,
			HOST1X_OPCODE_INCR(TGR3D_VP_ATTRIB_IN_OUT_SELECT, 1));
	host1x_pushbuf_push(pb, attributes_mask);
}

void grate_3d_set_polygon_offset(struct host1x_pushbuf *pb,
				 float units, float factor)
{
	host1x_pushbuf_push(pb,
			    HOST1X_OPCODE_INCR(TGR3D_POLYGON_OFFSET_UNITS, 2));
	host1x_pushbuf_push_float(pb, units);
	host1x_pushbuf_push_float(pb, factor);
}

void grate_3d_set_attribute_mode(struct host1x_pushbuf *pb, unsigned index,
				 unsigned type, unsigned size, unsigned stride)
{
	uint32_t value = 0;

	switch (type) {
	case ATTRIB_TYPE_UBYTE:
	case ATTRIB_TYPE_UBYTE_NORM:
	case ATTRIB_TYPE_SBYTE:
	case ATTRIB_TYPE_SBYTE_NORM:
		break;
	case ATTRIB_TYPE_USHORT:
	case ATTRIB_TYPE_USHORT_NORM:
	case ATTRIB_TYPE_SSHORT:
	case ATTRIB_TYPE_SSHORT_NORM:
	case ATTRIB_TYPE_FIXED16:
	case ATTRIB_TYPE_FLOAT16:
		stride *= 2;
		break;
	default:
		stride *= 4;
		break;
	}

	value |= TGR3D_VAL(ATTRIB_MODE, TYPE, type);
	value |= TGR3D_VAL(ATTRIB_MODE, SIZE, size);
	value |= TGR3D_VAL(ATTRIB_MODE, STRIDE, stride);

	host1x_pushbuf_push(pb,
			    HOST1X_OPCODE_INCR(TGR3D_ATTRIB_MODE(index), 1));
	host1x_pushbuf_push(pb, value);
}

void grate_3d_relocate_attribute_data(struct host1x_pushbuf *pb,
				      struct host1x_bo *data,
				      unsigned index, unsigned offset)
{
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_ATTRIB_PTR(index), 1));
	host1x_pushbuf_relocate(pb, data, offset, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
}

void grate_3d_relocate_primitive_indices(struct host1x_pushbuf *pb,
					 struct host1x_bo *indices,
					 unsigned offset)
{
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_INDEX_PTR, 1));
	host1x_pushbuf_relocate(pb, indices, offset, 0);
	host1x_pushbuf_push(pb, 0xdeadbeef);
}

void grate_3d_set_draw_params(struct host1x_pushbuf *pb,
			      unsigned index_mode,
			      unsigned provoking_vtx,
			      unsigned primitive_type,
			      unsigned first)
{
	uint32_t value = 0;

	value |= TGR3D_VAL(DRAW_PARAMS, INDEX_MODE, index_mode);
	value |= TGR3D_VAL(DRAW_PARAMS, PROVOKING_VERTEX, provoking_vtx);
	value |= TGR3D_VAL(DRAW_PARAMS, PRIMITIVE_TYPE, primitive_type);
	value |= TGR3D_VAL(DRAW_PARAMS, FIRST, first);
	value |= 0xC0000000;

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_DRAW_PARAMS, 1));
	host1x_pushbuf_push(pb, value);
}

void grate_3d_draw_primitive(struct host1x_pushbuf *pb,
			     unsigned index_count, unsigned offset)
{
	uint32_t value = 0;

	value |= TGR3D_VAL(DRAW_PRIMITIVES, INDEX_COUNT, index_count - 1);
	value |= TGR3D_VAL(DRAW_PRIMITIVES, OFFSET, offset);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_DRAW_PRIMITIVES, 1));
	host1x_pushbuf_push(pb, value);
}

void grate_3d_init(struct host1x_pushbuf *pb)
{
	/* reset upload counters ? */
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x503, 0x00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0x545, 0x00));
	host1x_pushbuf_push(pb, HOST1X_OPCODE_IMM(0xe22, 0x00));
	host1x_pushbuf_push(pb,
			    HOST1X_OPCODE_IMM(TGR3D_FP_UPLOAD_MFU_INST_ID, 0));
	host1x_pushbuf_push(pb,
			    HOST1X_OPCODE_IMM(TGR3D_FP_UPLOAD_ALU_INST_ID, 0));

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

void grate_3d_startup_pseq_engine(struct host1x_pushbuf *pb,
				  unsigned inst_count)
{
	host1x_pushbuf_push(pb, HOST1X_OPCODE_INCR(TGR3D_PSEQ_ENGINE_INST, 1));
	host1x_pushbuf_push(pb, 0x20006000 | inst_count);
}

void grate_draw_elements(struct grate *grate, enum grate_primitive type,
			 unsigned int size, unsigned int count,
			 struct grate_bo *bo, unsigned long offset)
{
	struct host1x_gr3d *gr3d = host1x_get_gr3d(grate->host1x);
	struct host1x_syncpt *syncpt = &gr3d->client->syncpts[0];
	struct grate_viewport *viewport = &grate->viewport;
	struct host1x_framebuffer *fb = grate->fb->back;
	struct grate_program *program = grate->program;
	uint32_t format = PIXEL_FORMAT_RGBA8888;
	uint32_t pitch = fb->width * 4;
	uint32_t fence;
	struct host1x_pushbuf *pb;
	struct host1x_job *job;
	unsigned depth = 32;
	unsigned dither = 0;
	unsigned tiled = 1;
	unsigned index_mode;
	unsigned draw_type;
	unsigned idx;
	int err;

	switch (type) {
	case GRATE_POINTS:
		draw_type = PRIMITIVE_TYPE_POINTS;
		break;
	case GRATE_LINES:
		draw_type = PRIMITIVE_TYPE_LINES;
		break;
	case GRATE_LINE_STRIP:
		draw_type = PRIMITIVE_TYPE_LINE_STRIP;
		break;
	case GRATE_LINE_LOOP:
		draw_type = PRIMITIVE_TYPE_LINE_LOOP;
		break;
	case GRATE_TRIANGLES:
		draw_type = PRIMITIVE_TYPE_TRIANGLES;
		break;
	case GRATE_TRIANGLE_STRIP:
		draw_type = PRIMITIVE_TYPE_TRIANGLE_STRIP;
		break;
	case GRATE_TRIANGLE_FAN:
		draw_type = PRIMITIVE_TYPE_TRIANGLE_FAN;
		break;
	default:
		fprintf(stderr, "ERROR: unsupported type: %d\n", type);
		return;
	}

	switch (size) {
	case 0:
		index_mode = INDEX_MODE_NONE;
		break;
	case 1:
		index_mode = INDEX_MODE_UINT8;
		break;
	case 2:
		index_mode = INDEX_MODE_UINT16;
		break;
	default:
		fprintf(stderr, "ERROR: unsupported size: %d\n", size);
		return;
	}

	/* invalidate memory for indices */
	err = host1x_bo_invalidate(bo->bo, offset, count * size);
	if (err < 0) {
		fprintf(stderr, "ERROR: failed to invalidate buffer\n");
		return;
	}

	/*
	 * build command stream
	 */

	job = host1x_job_create(syncpt->id, 1);
	if (!job)
		return;

	pb = host1x_job_append(job, gr3d->commands, 0);
	if (!pb) {
		host1x_job_free(job);
		return;
	}

	if (depth == 16) {
		format = PIXEL_FORMAT_RGB565;
		pitch /= 2;
	}

	grate_3d_begin(pb);
	grate_3d_init(pb);
	grate_3d_set_depth_range(pb, 0.0f, 1.0f);
	grate_3d_set_dither(pb, 0x779);
	grate_3d_set_point_params(pb, 0x1401);
	grate_3d_set_point_size(pb, 1.0f);
	grate_3d_set_line_params(pb, 0x2);
	grate_3d_set_line_width(pb, 1.0f);
	grate_3d_set_viewport(pb, viewport->x, viewport->y, 0.5f,
			      viewport->width, viewport->height, 0.5f);
	grate_3d_set_guardband(pb, viewport->width, viewport->height, 0.5f);
	grate_3d_set_cull_face_and_linker_inst_nb(pb, false, false, false,
			linker_instructions_nb(program->linker), 0x2E38);
	grate_3d_set_scissor(pb, 0, fb->width, 0, fb->height);
	grate_3d_set_render_target_params(pb, 1, dither, format, pitch, tiled);
	grate_3d_set_render_targets_enable(pb, 1);
	grate_3d_relocate_render_target(pb, fb->bo, 1);
	grate_3d_set_point_coord_range(pb, 0.0f, 1.0f, 0.0f, 1.0f);
	grate_3d_set_alu_buffer_size(pb, 1, 0x12C);
	grate_3d_set_used_tram_rows_nb(pb, 2);
	grate_3d_set_pseq_dw_cfg(pb, 1);
	grate_3d_upload_vp_consts(pb, program->uniform, 256 * 4);
	grate_3d_upload_fp_consts(pb, program->fs_uniform, 32);
	grate_3d_set_vp_attributes_in_out_mask(pb, program->attributes_mask);
	grate_3d_set_polygon_offset(pb, 0.0f, 0.0f);
	grate_3d_relocate_primitive_indices(pb, bo->bo, offset);
	grate_3d_set_draw_params(pb, index_mode,
				 PROVOKING_VERTEX_LAST, draw_type, 0);
	grate_3d_startup_pseq_engine(pb,
				     count_pseq_instructions_nb(program->fs));

	for (idx = 0; idx < GRATE_MAX_ATTRIBUTES; idx++) {
		struct grate_vertex_attribute *attr = &grate->attributes[idx];

		if (!attr->bo)
			continue;

		grate_3d_relocate_attribute_data(pb, attr->bo->bo,
						 idx, attr->offset);

		grate_3d_set_attribute_mode(pb, idx,
					    ATTRIB_TYPE_FLOAT32,
					    attr->size, attr->stride);
	}

	grate_shader_emit(pb, program->vs);
	grate_shader_emit(pb, program->fs);
	grate_shader_emit(pb, program->linker);

	grate_3d_draw_primitive(pb, count, 0);

	host1x_pushbuf_push(pb, HOST1X_OPCODE_NONINCR(0x00, 0x01));
	host1x_pushbuf_push(pb, 0x000001 << 8 | syncpt->id);

	err = host1x_client_submit(gr3d->client, job);
	if (err < 0) {
		host1x_job_free(job);
		return;
	}

	host1x_job_free(job);

	err = host1x_client_flush(gr3d->client, &fence);
	if (err < 0)
		return;

	err = host1x_client_wait(gr3d->client, fence, ~0u);
	if (err < 0)
		return;
}
