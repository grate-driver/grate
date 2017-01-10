/*
 * Copyright (c) 2016 Dmitry Osipenko <digetx@gmail.com>
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "linker_asm.h"

const char * tram_component(const link_instr *instr, unsigned id)
{
	static char ret[4][16];
	char *buf = ret[id];
	unsigned interpolation_disable;
	unsigned const_across_length;
	unsigned const_across_width;
	unsigned tram_dst_type;

	switch (id) {
	case 0:
		interpolation_disable = instr->interpolation_disable_x;
		const_across_length = instr->const_x_across_length;
		const_across_width = instr->const_x_across_width;
		tram_dst_type = instr->tram_dst_type_x;
		break;
	case 1:
		interpolation_disable = instr->interpolation_disable_y;
		const_across_length = instr->const_y_across_length;
		const_across_width = instr->const_y_across_width;
		tram_dst_type = instr->tram_dst_type_y;
		break;
	case 2:
		interpolation_disable = instr->interpolation_disable_z;
		const_across_length = instr->const_z_across_length;
		const_across_width = instr->const_z_across_width;
		tram_dst_type = instr->tram_dst_type_z;
		break;
	case 3:
		interpolation_disable = instr->interpolation_disable_w;
		const_across_length = instr->const_w_across_length;
		const_across_width = instr->const_w_across_width;
		tram_dst_type = instr->tram_dst_type_w;
		break;
	default:
		return "invalid id";
	}

	switch (tram_dst_type) {
	case TRAM_DST_NONE:
		buf += sprintf(buf, "NOP");
		break;
	case TRAM_DST_FX10_LOW:
		buf += sprintf(buf, "fx10.l");
		break;
	case TRAM_DST_FX10_HIGH:
		buf += sprintf(buf, "fx10.h");
		break;
	case TRAM_DST_FP20:
		buf += sprintf(buf, "fp20");
		break;
	default:
		buf += sprintf(buf, "Invalid type!");
		break;
	}

	if (interpolation_disable) {
		buf += sprintf(buf, "(dis)");
	}

	if (const_across_length) {
		buf += sprintf(buf, "(cl)");
	}

	if (const_across_width) {
		buf += sprintf(buf, "(cw)");
	}

	return ret[id];
}

static char export_swizzle(unsigned swzl)
{
	switch (swzl) {
	case LINK_SWIZZLE_X:
		return 'x';
	case LINK_SWIZZLE_Y:
		return 'y';
	case LINK_SWIZZLE_Z:
		return 'z';
	case LINK_SWIZZLE_W:
		return 'w';
	}

	return '!';
}

const char * linker_instruction_disassemble(const link_instr *instr)
{
	static char ret[128];
	char *buf = ret;

	buf += sprintf(buf, "LINK %s, %s, %s, %s, tram%d, export%d.%c%c%c%c",
		       tram_component(instr, 0),
		       tram_component(instr, 1),
		       tram_component(instr, 2),
		       tram_component(instr, 3),
		       instr->tram_row_index,
		       instr->vertex_export_index,
		       export_swizzle(instr->swizzle_x),
		       export_swizzle(instr->swizzle_y),
		       export_swizzle(instr->swizzle_z),
		       export_swizzle(instr->swizzle_w));

	return ret;
}
