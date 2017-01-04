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

#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fragment_asm.h"

static unsigned imm_fp20[3];
static unsigned imm_fx10_low[3];
static unsigned imm_fx10_high[3];
static unsigned alu_imm_used;

static const char * reg_name(int reg, int id)
{
	static char ret[32];
	char *buf = ret;

	switch (reg) {
	case FRAG_ROW_REG_0 ... FRAG_ROW_REG_15:
		buf += sprintf(buf, "r%d", reg);
		break;
	case FRAG_GENERAL_PURPOSE_REG_0 ... FRAG_GENERAL_PURPOSE_REG_7:
		buf += sprintf(buf, "g%d", reg - FRAG_GENERAL_PURPOSE_REG_0);
		break;
	case FRAG_ALU_RESULT_REG_0 ... FRAG_ALU_RESULT_REG_3:
		buf += sprintf(buf, "alu%d", reg - FRAG_ALU_RESULT_REG_0);
		break;
	case FRAG_EMBEDDED_CONSTANT_0 ... FRAG_EMBEDDED_CONSTANT_2:
		buf += sprintf(buf, "imm%d", reg - FRAG_EMBEDDED_CONSTANT_0);
		break;
	case FRAG_LOWP_VEC2_0_1:
		if (id == -1) {
			buf += sprintf(buf, "lp");
		} else {
			buf += sprintf(buf, "#%d", id);
		}
		break;
	case FRAG_UNIFORM_REG_0 ... FRAG_UNIFORM_REG_31:
		buf += sprintf(buf, "u%d", reg - FRAG_UNIFORM_REG_0);
		break;
	case FRAG_CONDITION_REG_0 ... FRAG_CONDITION_REG_7:
		buf += sprintf(buf, "cr%d",
			       (reg - FRAG_CONDITION_REG_0) * 2 + id);
		break;
	case FRAGMENT_POS_X:
		buf += sprintf(buf, "posx");
		break;
	case FRAGMENT_POS_Y:
		buf += sprintf(buf, "posy");
		break;
	case FRAG_POLYGON_FACE:
		buf += sprintf(buf, "pface");
		break;
	default:
		buf += sprintf(buf, "invalid reg %d", reg);
		break;
	}

	return ret;
}

static const char * mfu_var(const mfu_instr *mfu, int reg)
{
	static char ret[4][32];
	char *buf = ret[reg];
	unsigned saturate;
	unsigned opcode;
	unsigned source;

	switch (reg) {
	case 0:
		saturate = mfu->var0_saturate;
		opcode = mfu->var0_opcode;
		source = mfu->var0_source;
		break;
	case 1:
		saturate = mfu->var1_saturate;
		opcode = mfu->var1_opcode;
		source = mfu->var1_source;
		break;
	case 2:
		saturate = mfu->var2_saturate;
		opcode = mfu->var2_opcode;
		source = mfu->var2_source;
		break;
	case 3:
		saturate = mfu->var3_saturate;
		opcode = mfu->var3_opcode;
		source = mfu->var3_source;
		break;
	default:
		return "invalid register";
	}

	if (opcode == MFU_VAR_NOP) {
		buf += sprintf(buf, "NOP");
	} else {
		if (saturate) {
			buf += sprintf(buf, "sat(");
		}

		switch (opcode) {
		case MFU_VAR_FP20:
			buf += sprintf(buf, "%s.fp20", reg_name(source, 0));
			break;
		case MFU_VAR_FX10:
			buf += sprintf(buf, "%s.fx10", reg_name(source, 0));
			break;
		default:
			buf += sprintf(buf, "invalid opcode");
			break;
		}

		if (saturate) {
			buf += sprintf(buf, ")");
		}
	}

	return ret[reg];
}

static const char * disassemble_mfu(const mfu_instr *mfu)
{
	static char ret[128];
	char *buf = ret;

	if (mfu->part0 != 0x00000000) {
		buf += sprintf(buf, "var ");

		if (mfu->unk_varying != 0) {
			buf += sprintf(buf, "unk(0x%llX) ",
				       (uint64_t)mfu->unk_varying);
		}

		buf += sprintf(buf, "%s, %s, %s, %s",
			       mfu_var(mfu, 0),
			       mfu_var(mfu, 1),
			       mfu_var(mfu, 2),
			       mfu_var(mfu, 3));

	} else if (mfu->opcode == MFU_NOP) {
		buf += sprintf(buf, "NOP");
	} else {
		switch (mfu->opcode) {
		case MFU_RCP:
			buf += sprintf(buf, "rcp");
			break;
		case MFU_RSQ:
			buf += sprintf(buf, "rsq");
			break;
		case MFU_LG2:
			buf += sprintf(buf, "lg2");
			break;
		case MFU_EX2:
			buf += sprintf(buf, "ex2");
			break;
		case MFU_SQRT:
			buf += sprintf(buf, "sqrt");
			break;
		case MFU_SIN:
			buf += sprintf(buf, "sin");
			break;
		case MFU_COS:
			buf += sprintf(buf, "cos");
			break;
		case MFU_FRC:
			buf += sprintf(buf, "frc");
			break;
		case MFU_PREEX2:
			buf += sprintf(buf, "preEx2");
			break;
		case MFU_PRESIN:
			buf += sprintf(buf, "preSin");
			break;
		case MFU_PRECOS:
			buf += sprintf(buf, "preCos");
			break;
		default:
			buf += sprintf(buf, "invalid opcode!");
			break;
		}

		buf += sprintf(buf, " v%d", mfu->reg);
	}

	return ret;
}

#define ALU_rA	0
#define ALU_rB	1
#define ALU_rC	2
#define ALU_rD	3

static const char * alu_r(const union fragment_alu_instruction *alu, int reg)
{
	static char ret[4][64];
	char *buf = ret[reg];
	unsigned reg_select;
	unsigned sub_reg_select_high;
	unsigned minus_one;
	unsigned fixed10;
	unsigned absolute_value;
	unsigned negate;
	unsigned scale_by_two;
	unsigned precision = 1;

	switch (reg) {
	case ALU_rA:
		reg_select 		= alu->rA_reg_select;
		sub_reg_select_high	= alu->rA_sub_reg_select_high;
		minus_one		= alu->rA_minus_one;
		fixed10			= alu->rA_fixed10;
		absolute_value		= alu->rA_absolute_value;
		negate			= alu->rA_negate;
		scale_by_two		= alu->rA_scale_by_two;
		break;
	case ALU_rB:
		reg_select 		= alu->rB_reg_select;
		sub_reg_select_high	= alu->rB_sub_reg_select_high;
		minus_one		= alu->rB_minus_one;
		fixed10			= alu->rB_fixed10;
		absolute_value		= alu->rB_absolute_value;
		negate			= alu->rB_negate;
		scale_by_two		= alu->rB_scale_by_two;
		break;
	case ALU_rC:
		reg_select 		= alu->rC_reg_select;
		sub_reg_select_high	= alu->rC_sub_reg_select_high;
		minus_one		= alu->rC_minus_one;
		fixed10			= alu->rC_fixed10;
		absolute_value		= alu->rC_absolute_value;
		negate			= alu->rC_negate;
		scale_by_two		= alu->rC_scale_by_two;
		break;
	case ALU_rD:
		reg_select 		= alu->rD_reg_select;
		sub_reg_select_high	= alu->rD_sub_reg_select_high;
		minus_one		= alu->rD_minus_one;
		fixed10			= alu->rD_fixed10;
		absolute_value		= alu->rD_absolute_value;
		negate			= 0;
		scale_by_two		= 0;

		if (!alu->rD_enable) {
			return "#1";
		}
		break;
	default:
		return "invalid operand";
	}

	if (negate) {
		buf += sprintf(buf, "-");
	}

	if (absolute_value) {
		buf += sprintf(buf, "abs(");
	}

	switch (reg) {
	case ALU_rD:
		buf += sprintf(buf, reg_select ? "rC" : "rB");
		break;
	default:
		buf += sprintf(buf, "%s",
			       reg_name(reg_select, sub_reg_select_high));

		switch (reg_select) {
		case FRAG_EMBEDDED_CONSTANT_0 ... FRAG_EMBEDDED_CONSTANT_2:
			if (fixed10) {
				if (sub_reg_select_high) {
					imm_fx10_high[reg_select - FRAG_EMBEDDED_CONSTANT_0] = 1;
				} else {
					imm_fx10_low[reg_select - FRAG_EMBEDDED_CONSTANT_0] = 1;
				}
			} else {
				imm_fp20[reg_select - FRAG_EMBEDDED_CONSTANT_0] = 1;
			}

			alu_imm_used = 1;
			break;
		case FRAG_CONDITION_REG_0 ... FRAG_CONDITION_REG_7:
		case FRAG_LOWP_VEC2_0_1:
		case FRAGMENT_POS_X:
		case FRAGMENT_POS_Y:
		case FRAG_POLYGON_FACE:
			precision = 0;
		}
		break;
	}

	if (precision && fixed10) {
		buf += sprintf(buf, sub_reg_select_high ? ".h" : ".l");
	}

	if (absolute_value) {
		buf += sprintf(buf, ")");
	}

	if (scale_by_two) {
		buf += sprintf(buf, "*2");
	}

	if (minus_one) {
		buf += sprintf(buf, "-1");
	}

	switch (reg) {
	case ALU_rA:
	case ALU_rB:
	case ALU_rC:
		buf += sprintf(buf, ",");
		break;
	default:
		break;
	}

	return ret[reg];
}

static const char * alu_dst(const union fragment_alu_instruction *alu)
{
	static char ret[32];
	char *buf = ret;
	int cr_adj;

	if (alu->dst_reg >= FRAG_CONDITION_REG_0 && alu->dst_reg <= FRAG_CONDITION_REG_7) {
		cr_adj = (alu->write_low_sub_reg || alu->write_low_sub_reg);

		buf += sprintf(buf, "%s", reg_name(alu->dst_reg, cr_adj));
	} else {
		buf += sprintf(buf, "%s.", reg_name(alu->dst_reg, -1));
		buf += sprintf(buf, alu->write_low_sub_reg ? "l" : "*");
		buf += sprintf(buf, alu->write_high_sub_reg ? "h" : "*");
	}

	buf += sprintf(buf, ",");

	return ret;
}

static const char * alu_opcode(const union fragment_alu_instruction *alu)
{
	static char ret[8];
	char *buf = ret;

	switch (alu->opcode) {
	case ALU_OPCODE_MAD:
		if (alu->addition_disable) {
			buf += sprintf(buf, "MUL");
		} else {
			buf += sprintf(buf, "MAD");
		}
		break;
	case ALU_OPCODE_MIN:
	case ALU_OPCODE_MAX:
		if (alu->opcode == ALU_OPCODE_MIN) {
			buf += sprintf(buf, "MIN");
		} else {
			buf += sprintf(buf, "MAX");
		}
		break;
	case ALU_OPCODE_CSEL:
		buf += sprintf(buf, "CSEL");
		break;
	}

	return ret;
}

static const char * disassemble_alu(const union fragment_alu_instruction *alu)
{
	static char ret[256];
	char *buf = ret;

	buf += sprintf(buf, "%-5s%-8s%-12s%-12s%-12s%-5s",
		       alu_opcode(alu),
		       alu_dst(alu),
		       alu_r(alu, ALU_rA),
		       alu_r(alu, ALU_rB),
		       alu_r(alu, ALU_rC),
		       alu_r(alu, ALU_rD));

	switch (alu->scale_result) {
	case 1:
		buf += sprintf(buf, "(x2)");
		break;
	case 2:
		buf += sprintf(buf, "(x4)");
		break;
	case 3:
		buf += sprintf(buf, "(/2)");
		break;
	}

	if (alu->saturate_result) {
		buf += sprintf(buf, "(sat)");
	}

	if (alu->accumulate_result_this) {
		buf += sprintf(buf, "(this)");
	}

	if (alu->accumulate_result_other) {
		buf += sprintf(buf, "(other)");
	}

	switch (alu->condition_code) {
	case ALU_CC_ZERO:
		buf += sprintf(buf, "(eq)");
		break;
	case ALU_CC_GREATER_THAN_ZERO:
		buf += sprintf(buf, "(gt)");
		break;
	case ALU_CC_ZERO_OR_GREATER:
		buf += sprintf(buf, "(ge)");
		break;
	}

	return ret;
}

static float fp20(uint32_t value)
{
	uint32_t sign, mantissa, exponent;
	union {
		uint32_t u;
		float f;
	} bits;

	sign = (value >> 19) & 0x1;
	exponent = (value >> 13) & 0x3f;
	mantissa = (value >>  0) & 0x1fff;

	if (exponent == 0x3f)
		exponent = 0xff;
	else
		exponent += 127 - 31;

	mantissa = mantissa << (23 - 13);

	bits.u = sign << 31 | (exponent << 23) | mantissa;

	return bits.f;
}

static float fx10(uint32_t value)
{
	if ((value >> 9) & 0x1)
		return -(((value - 1) ^ 0x3ff) / 256.0f);
	else
		return value / 256.0f;
}

static const char * disassemble_alu_imm(const alu_instr *alu__)
{
	alu_instr alu = *alu__;
	uint32_t swap;
	static char ret[128];
	char *buf = ret;
	int comma = 0;
	int i;

	swap = alu.part7;
	alu.part7 = alu.part6;
	alu.part6 = swap;

	for (i = 0; i < 3; i++) {
		if (imm_fp20[i]) {
			if (comma) {
				buf += sprintf(buf, ", ");
			} else {
				comma = 1;
			}

			buf += sprintf(buf, "imm%d = ", i);

			switch (i) {
			case 0:
				buf += sprintf(buf, "%f", fp20(alu.imm0.fp20));
				break;
			case 1:
				buf += sprintf(buf, "%f", fp20(alu.imm1.fp20));
				break;
			case 2:
				buf += sprintf(buf, "%f", fp20(alu.imm2.fp20));
				break;
			}
		}

		if (imm_fx10_low[i]) {
			if (comma) {
				buf += sprintf(buf, ", ");
			} else {
				comma = 1;
			}

			buf += sprintf(buf, "imm%d.l = ", i);

			switch (i) {
			case 0:
				buf += sprintf(buf, "%f", fx10(alu.imm0.fx10_low));
				break;
			case 1:
				buf += sprintf(buf, "%f", fx10(alu.imm1.fx10_low));
				break;
			case 2:
				buf += sprintf(buf, "%f", fx10(alu.imm2.fx10_low));
				break;
			}
		}

		if (imm_fx10_high[i]) {
			if (comma) {
				buf += sprintf(buf, ", ");
			} else {
				comma = 1;
			}

			buf += sprintf(buf, "imm%d.h = ", i);

			switch (i) {
			case 0:
				buf += sprintf(buf, "%f", fx10(alu.imm0.fx10_high));
				break;
			case 1:
				buf += sprintf(buf, "%f", fx10(alu.imm1.fx10_high));
				break;
			case 2:
				buf += sprintf(buf, "%f", fx10(alu.imm2.fx10_high));
				break;
			}
		}
	}

	return ret;
}

static const char * disassemble_alus(const alu_instr *alu)
{
	static char ret[1024];
	char *buf = ret;

	memset(imm_fp20, 0, sizeof(imm_fp20));
	memset(imm_fx10_low, 0, sizeof(imm_fx10_low));
	memset(imm_fx10_high, 0, sizeof(imm_fx10_high));
	alu_imm_used = 0;

	buf += sprintf(buf, "\tALU0:\t%s\n", disassemble_alu(&alu->a[0]));
	buf += sprintf(buf, "\tALU1:\t%s\n", disassemble_alu(&alu->a[1]));
	buf += sprintf(buf, "\tALU2:\t%s\n", disassemble_alu(&alu->a[2]));

	if (!alu_imm_used) {
		buf += sprintf(buf, "\tALU3:\t%s\n",
			       disassemble_alu(&alu->a[3]));
	} else {
		buf += sprintf(buf, "\tALU3:\t%s\n",
			       disassemble_alu_imm(alu));
	}

	return ret;
}

const char * fragment_pipeline_disassemble(
	const pseq_instr *pseq,
	const mfu_instr *mfu, unsigned mfu_nb,
	const tex_instr *tex,
	const alu_instr *alu, unsigned alu_nb,
	const dw_instr *dw)
{
	static char ret[2048];
	static char *locale;
	char *buf = ret;
	unsigned i;

	locale = strdup( setlocale(LC_ALL, NULL) );
	if (!locale)
		return "Out of memory";

	/* float decimal point is locale-dependent! */
	setlocale(LC_ALL, "C");

	buf += sprintf(buf, "EXEC\n");

	if (pseq->data != 0x00000000) {
		buf += sprintf(buf, "\tPSEQ:\t0x%08X\n", pseq->data);
	}

	for (i = 0; i < mfu_nb; i++) {
		buf += sprintf(buf, "\tMFU:\t%s\n", disassemble_mfu(mfu + i));
	}

	if (tex->data != 0x00000000) {
		buf += sprintf(buf, "\tTEX:\t0x%08X\n", tex->data);
	}

	for (i = 0; i < alu_nb; i++) {
		buf += sprintf(buf, "%s", disassemble_alus(alu + i));
	}

	if (dw->data != 0x00000000) {
		buf += sprintf(buf, "\tDW:\t0x%08X\n", dw->data);
	}

	buf += sprintf(buf, ";");

	/* restore locale */
	setlocale(LC_ALL, locale);
	free(locale);

	return ret;
}
