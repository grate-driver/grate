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

#include "vpe_vliw.h"

static char swizzle(unsigned swzl)
{
	switch (swzl) {
	case SWIZZLE_X:
		return 'x';
	case SWIZZLE_Y:
		return 'y';
	case SWIZZLE_Z:
		return 'z';
	case SWIZZLE_W:
		return 'w';
	default:
		break;
	}

	return '!';
}

static const char * vector_opcode(unsigned opcode)
{
	switch (opcode) {
	case VECTOR_OPCODE_NOP:
		return "NOP";
	case VECTOR_OPCODE_MOV:
		return "MOV";
	case VECTOR_OPCODE_MUL:
		return "MUL";
	case VECTOR_OPCODE_ADD:
		return "ADD";
	case VECTOR_OPCODE_MAD:
		return "MAD";
	case VECTOR_OPCODE_DP3:
		return "DP3";
	case VECTOR_OPCODE_DPH:
		return "DPH";
	case VECTOR_OPCODE_DP4:
		return "DP4";
	case VECTOR_OPCODE_DST:
		return "DST";
	case VECTOR_OPCODE_MIN:
		return "MIN";
	case VECTOR_OPCODE_MAX:
		return "MAX";
	case VECTOR_OPCODE_SLT:
		return "SLT";
	case VECTOR_OPCODE_SGE:
		return "SGE";
	case VECTOR_OPCODE_ARL:
		return "ARL";
	case VECTOR_OPCODE_FRC:
		return "FRC";
	case VECTOR_OPCODE_FLR:
		return "FLR";
	case VECTOR_OPCODE_SEQ:
		return "SEQ";
	case VECTOR_OPCODE_SGT:
		return "SGT";
	case VECTOR_OPCODE_SLE:
		return "SLE";
	case VECTOR_OPCODE_SNE:
		return "SNE";
	case VECTOR_OPCODE_STR:
		return "STR";
	case VECTOR_OPCODE_SSG:
		return "SSG";
	case VECTOR_OPCODE_ARR:
		return "ARR";
	case VECTOR_OPCODE_ARA:
		return "ARA";
	case VECTOR_OPCODE_TXL:
		return "TXL";
	case VECTOR_OPCODE_PUSHA:
		return "PUSHA";
	case VECTOR_OPCODE_POPA:
		return "POPA";
	case 28:
		return "OPCODE-28";
	case 29:
		return "OPCODE-29";
	case 30:
		return "OPCODE-30";
	case 31:
		return "OPCODE-31";
	default:
		break;
	}

	return "INVALID!";
}

static const char * scalar_opcode(unsigned opcode)
{
	switch (opcode) {
	case SCALAR_OPCODE_NOP:
		return "NOP";
	case SCALAR_OPCODE_MOV:
		return "MOV";
	case SCALAR_OPCODE_RCP:
		return "RCP";
	case SCALAR_OPCODE_RCC:
		return "RCC";
	case SCALAR_OPCODE_RSQ:
		return "RSQ";
	case SCALAR_OPCODE_EXP:
		return "EXP";
	case SCALAR_OPCODE_LOG:
		return "LOG";
	case SCALAR_OPCODE_LIT:
		return "LIT";
	case SCALAR_OPCODE_BRA:
		return "BRA";
	case SCALAR_OPCODE_CAL:
		return "CAL";
	case SCALAR_OPCODE_RET:
		return "RET";
	case SCALAR_OPCODE_LG2:
		return "LG2";
	case SCALAR_OPCODE_EX2:
		return "EX2";
	case SCALAR_OPCODE_SIN:
		return "SIN";
	case SCALAR_OPCODE_COS:
		return "COS";
	case SCALAR_OPCODE_PUSHA:
		return "PUSHA";
	case SCALAR_OPCODE_POPA:
		return "POPA";
	case 17:
		return "OPCODE-17";
	case 18:
		return "OPCODE-18";
	case 21:
		return "OPCODE-21";
	case 22:
		return "OPCODE-22";
	case 23:
		return "OPCODE-23";
	case 24:
		return "OPCODE-24";
	case 25:
		return "OPCODE-25";
	case 26:
		return "OPCODE-26";
	case 27:
		return "OPCODE-27";
	case 28:
		return "OPCODE-28";
	case 29:
		return "OPCODE-29";
	case 30:
		return "OPCODE-30";
	case 31:
		return "OPCODE-31";
	default:
		break;
	}

	return "INVALID!";
}

static char address_register(int index)
{
	switch (index) {
	case 0: return 'x';
	case 1: return 'y';
	case 2: return 'z';
	case 3: return 'w';
	default: break;
	}

	return '!';
}

#define A	0
#define B	1
#define C	2

static const char * r(int reg, const vpe_instr128 *ins)
{
	static char ret[3][32];
	char *buf = ret[reg];
	int index;
	int absolute_value;
	int type;
	int negate;
	int swizzle_x;
	int swizzle_y;
	int swizzle_z;
	int swizzle_w;

	switch (reg) {
	case A:
		index = ins->rA_index;
		absolute_value = ins->rA_absolute_value;
		type = ins->rA_type;
		negate = ins->rA_negate;
		swizzle_x = ins->rA_swizzle_x;
		swizzle_y = ins->rA_swizzle_y;
		swizzle_z = ins->rA_swizzle_z;
		swizzle_w = ins->rA_swizzle_w;
		break;
	case B:
		index = ins->rB_index;
		absolute_value = ins->rB_absolute_value;
		type = ins->rB_type;
		negate = ins->rB_negate;
		swizzle_x = ins->rB_swizzle_x;
		swizzle_y = ins->rB_swizzle_y;
		swizzle_z = ins->rB_swizzle_z;
		swizzle_w = ins->rB_swizzle_w;
		break;
	case C:
		index = ins->rC_index;
		absolute_value = ins->rC_absolute_value;
		type = ins->rC_type;
		negate = ins->rC_negate;
		swizzle_x = ins->rC_swizzle_x;
		swizzle_y = ins->rC_swizzle_y;
		swizzle_z = ins->rC_swizzle_z;
		swizzle_w = ins->rC_swizzle_w;
		break;
	default:
		return "!";
	}

	memset(ret[reg], 0, 32);

	if (negate)
		buf += sprintf(buf, "-");

	if (absolute_value)
		buf += sprintf(buf, "abs(");

	switch (type) {
	case REG_TYPE_UNDEFINED:
		buf += sprintf(buf, "u");
		break;
	case REG_TYPE_TEMPORARY:
		buf += sprintf(buf, "r");
		break;
	case REG_TYPE_ATTRIBUTE:
		buf += sprintf(buf, "a[");

		if (ins->attribute_relative_addressing_enable) {
			buf += sprintf(buf, "A0.%c + ",
				address_register(ins->address_register_select));
		}
		break;
	case REG_TYPE_UNIFORM:
		buf += sprintf(buf, "c[");

		if (ins->constant_relative_addressing_enable) {
			buf += sprintf(buf, "A0.%c + ",
				address_register(ins->address_register_select));
		}
		break;
	default:
		return "!";
	}

	switch (type) {
	case REG_TYPE_TEMPORARY:
		buf += sprintf(buf, "%d", index);
		break;
	case REG_TYPE_ATTRIBUTE:
		buf += sprintf(buf, "%d]", ins->attribute_fetch_index);
		break;
	case REG_TYPE_UNIFORM:
		buf += sprintf(buf, "%d]", ins->uniform_fetch_index);
		break;
	default:
		break;
	}

	buf += sprintf(buf, ".%c%c%c%c",
		       swizzle(swizzle_x),
		       swizzle(swizzle_y),
		       swizzle(swizzle_z),
		       swizzle(swizzle_w));

	if (absolute_value)
		buf += sprintf(buf, ")");

	return ret[reg];
}

static const char * vec_rD(const vpe_instr128 *ins)
{
	static char ret[9];
	char *buf = ret;

	buf += sprintf(buf, "r%d.", ins->vector_rD_index);

	buf[0] = ins->vector_op_write_x_enable ? 'x' : '*';
	buf[1] = ins->vector_op_write_y_enable ? 'y' : '*';
	buf[2] = ins->vector_op_write_z_enable ? 'z' : '*';
	buf[3] = ins->vector_op_write_w_enable ? 'w' : '*';
	buf[4] = '\0';

	return ret;
}

static const char * sca_rD(const vpe_instr128 *ins)
{
	static char ret[9];
	char *buf = ret;

	buf += sprintf(buf, "r%d.", ins->scalar_rD_index);

	buf[0] = ins->scalar_op_write_x_enable ? 'x' : '*';
	buf[1] = ins->scalar_op_write_y_enable ? 'y' : '*';
	buf[2] = ins->scalar_op_write_z_enable ? 'z' : '*';
	buf[3] = ins->scalar_op_write_w_enable ? 'w' : '*';
	buf[4] = '\0';

	return ret;
}

const char * vpe_vliw_disassemble(const vpe_instr128 *ins)
{
	static char ret[256];
	char *buf = ret;

	memset(ret, 0, sizeof(ret));

	buf += sprintf(buf, ins->end_of_program ? "EXEC_END" : "EXEC");

	buf += sprintf(buf, "(export[");

	if (ins->export_relative_addressing_enable) {
		buf += sprintf(buf, "A0.%c + ",
				address_register(ins->address_register_select));
	}

	buf += sprintf(buf, "%d]=%s)",
		       ins->export_write_index,
		       ins->export_vector_write_enable ? "vector" : "scalar");

	if (ins->saturate_result)
		buf += sprintf(buf, "(saturate)");

	buf += sprintf(buf, "(cr=%d)", ins->condition_register_index);

	if (ins->condition_set)
		buf += sprintf(buf, "(cs)");

	if (ins->condition_check)
		buf += sprintf(buf, "(cc)");

	if (ins->condition_flags_write_enable)
		buf += sprintf(buf, "(cwr)");

	if (ins->predicate_lt)
		buf += sprintf(buf, "(lt)");

	if (ins->predicate_eq)
		buf += sprintf(buf, "(eq)");

	if (ins->predicate_gt)
		buf += sprintf(buf, "(gt)");

	buf += sprintf(buf, "(p.%c%c%c%c)",
			swizzle(ins->predicate_swizzle_x),
			swizzle(ins->predicate_swizzle_y),
			swizzle(ins->predicate_swizzle_z),
			swizzle(ins->predicate_swizzle_w));

	buf += sprintf(buf, "\n");
	buf += sprintf(buf, "\t");

	buf += sprintf(buf, "%sv ", vector_opcode(ins->vector_opcode));

	switch (ins->vector_opcode) {
	case VECTOR_OPCODE_NOP:
	case VECTOR_OPCODE_PUSHA:
	case VECTOR_OPCODE_POPA:
		buf += sprintf(buf, "\n");
		break;
	case VECTOR_OPCODE_ADD:
		buf += sprintf(buf, "%s, %s, %s\n", vec_rD(ins),
			       r(A, ins), r(C, ins));
		break;
	case VECTOR_OPCODE_MUL:
	case VECTOR_OPCODE_DP3:
	case VECTOR_OPCODE_DPH:
	case VECTOR_OPCODE_DP4:
	case VECTOR_OPCODE_DST:
	case VECTOR_OPCODE_MIN:
	case VECTOR_OPCODE_MAX:
	case VECTOR_OPCODE_SLT:
	case VECTOR_OPCODE_SGE:
	case VECTOR_OPCODE_SEQ:
	case VECTOR_OPCODE_SGT:
	case VECTOR_OPCODE_SLE:
	case VECTOR_OPCODE_SNE:
		buf += sprintf(buf, "%s, %s, %s\n", vec_rD(ins),
			       r(A, ins), r(B, ins));
		break;
	case VECTOR_OPCODE_STR:
	case VECTOR_OPCODE_ARA:
		buf += sprintf(buf, "%s\n", vec_rD(ins));
		break;
	case VECTOR_OPCODE_MOV:
	case VECTOR_OPCODE_ARL:
	case VECTOR_OPCODE_FRC:
	case VECTOR_OPCODE_FLR:
	case VECTOR_OPCODE_SSG:
	case VECTOR_OPCODE_ARR:
	case VECTOR_OPCODE_TXL:
		buf += sprintf(buf, "%s, %s\n", vec_rD(ins), r(A, ins));
		break;
	case VECTOR_OPCODE_MAD:
	default:
		buf += sprintf(buf, "%s, %s, %s, %s\n", vec_rD(ins),
			       r(A, ins), r(B, ins), r(C, ins));
		break;
	}

	buf += sprintf(buf, "\t");

	buf += sprintf(buf, "%ss ", scalar_opcode(ins->scalar_opcode));

	switch (ins->scalar_opcode) {
	case SCALAR_OPCODE_NOP:
	case SCALAR_OPCODE_PUSHA:
	case SCALAR_OPCODE_POPA:
	case SCALAR_OPCODE_RET:
		buf += sprintf(buf, "\n");
		break;
	case SCALAR_OPCODE_BRA:
	case SCALAR_OPCODE_CAL:
		buf += sprintf(buf, "%d\n", ins->iaddr);
		break;
	default:
		buf += sprintf(buf, "%s, %s\n", sca_rD(ins), r(C, ins));
		break;
	}

	buf += sprintf(buf, ";");

	return ret;
}
