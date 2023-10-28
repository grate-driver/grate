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
	case FRAGMENT_ROW_REG_0 ... FRAGMENT_ROW_REG_15:
		sprintf(buf, "r%d", reg);
		break;
	case FRAGMENT_GENERAL_PURPOSE_REG_0 ... FRAGMENT_GENERAL_PURPOSE_REG_7:
		sprintf(buf, "g%d", reg - FRAGMENT_GENERAL_PURPOSE_REG_0);
		break;
	case FRAGMENT_ALU_RESULT_REG_0 ... FRAGMENT_ALU_RESULT_REG_3:
		sprintf(buf, "alu%d", reg - FRAGMENT_ALU_RESULT_REG_0);
		break;
	case FRAGMENT_EMBEDDED_CONSTANT_0 ... FRAGMENT_EMBEDDED_CONSTANT_2:
		sprintf(buf, "imm%d", reg - FRAGMENT_EMBEDDED_CONSTANT_0);
		break;
	case FRAGMENT_LOWP_VEC2_0_1:
		if (id == -1) {
			sprintf(buf, "lp");
		} else {
			sprintf(buf, "#%d", id);
		}
		break;
	case FRAGMENT_UNIFORM_REG_0 ... FRAGMENT_UNIFORM_REG_31:
		sprintf(buf, "u%d", reg - FRAGMENT_UNIFORM_REG_0);
		break;
	case FRAGMENT_CONDITION_REG_0 ... FRAGMENT_CONDITION_REG_7:
		sprintf(buf, "cr%d", (reg - FRAGMENT_CONDITION_REG_0) * 2 + id);
		break;
	case FRAGMENT_POS_X:
		sprintf(buf, "posx");
		break;
	case FRAGMENT_POS_Y:
		sprintf(buf, "posy");
		break;
	case FRAGMENT_POLYGON_FACE:
		sprintf(buf, "pface");
		break;
	case FRAGMENT_KILL_REG:
		sprintf(buf, "kill");
		break;
	default:
		sprintf(buf, "invalid reg %d", reg);
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

	if (saturate) {
		buf += sprintf(buf, "sat(");
	}

	switch (opcode) {
	case MFU_VAR_NOP:
		buf += sprintf(buf, "NOP");
		break;
	case MFU_VAR_FP20:
		buf += sprintf(buf, "t%d.fp20", source);
		break;
	case MFU_VAR_FX10:
		buf += sprintf(buf, "t%d.fx10", source);
		break;
	default:
		buf += sprintf(buf, "invalid opcode");
		break;
	}

	if (saturate) {
		sprintf(buf, ")");
	}

	return ret[reg];
}

static const char * mfu_mul_src(unsigned src, int mul_idx)
{
	static char ret[2][16];
	char *buf = ret[mul_idx];

	switch (src) {
	case MFU_MUL_SRC_ROW_REG_0:
		return "r0";
	case MFU_MUL_SRC_ROW_REG_1:
		return "r1";
	case MFU_MUL_SRC_ROW_REG_2:
		return "r2";
	case MFU_MUL_SRC_ROW_REG_3:
		return "r3";
	case MFU_MUL_SRC_SFU_RESULT:
		return "sfu";
	case MFU_MUL_SRC_BARYCENTRIC_COEF_0:
		return "bar0";
	case MFU_MUL_SRC_BARYCENTRIC_COEF_1:
		return "bar1";
	case MFU_MUL_SRC_CONST_1:
		return "#1";
	default:
		sprintf(buf, "src%d", src);
		break;
	}

	return ret[mul_idx];
}

static const char * mfu_mul_dst(unsigned dst)
{
	static char ret[16];
	char *buf = ret;

	switch (dst) {
	case MFU_MUL_DST_BARYCENTRIC_WEIGHT:
		return "bar";
	case MFU_MUL_DST_ROW_REG_0:
		return "r0";
	case MFU_MUL_DST_ROW_REG_1:
		return "r1";
	case MFU_MUL_DST_ROW_REG_2:
		return "r2";
	case MFU_MUL_DST_ROW_REG_3:
		return "r3";
	default:
		sprintf(buf, "dst%d", dst);
		break;
	}

	return ret;
}

static const char * mfu_mul(const mfu_instr *mfu, int mul_idx)
{
	static char ret[2][32];
	char *buf = ret[mul_idx];
	unsigned src0 = mul_idx ? mfu->mul1_src0 : mfu->mul0_src0;
	unsigned src1 = mul_idx ? mfu->mul1_src1 : mfu->mul0_src1;
	unsigned dst = mul_idx ? mfu->mul1_dst : mfu->mul0_dst;

	sprintf(buf, "%s, %s, %s",
		mfu_mul_dst(dst),
		mfu_mul_src(src0, 0),
		mfu_mul_src(src1, 1));

	return ret[mul_idx];
}

static const char * mfu_opcode(unsigned opcode)
{
	static char ret[16];
	char *buf = ret;

	switch (opcode) {
	case MFU_NOP:
		return "nop";
	case MFU_RCP:
		return "rcp";
	case MFU_RSQ:
		return "rsq";
	case MFU_LG2:
		return "lg2";
	case MFU_EX2:
		return "ex2";
	case MFU_SQRT:
		return "sqrt";
	case MFU_SIN:
		return "sin";
	case MFU_COS:
		return "cos";
	case MFU_FRC:
		return "frc";
	case MFU_PREEX2:
		return "preEx2";
	case MFU_PRESIN:
		return "preSin";
	case MFU_PRECOS:
		return "preCos";
	default:
		sprintf(buf, "op%d", opcode);
		break;
	}

	return ret;
}

static const char * disassemble_mfu(const mfu_instr *mfu)
{
	static char ret[128];
	char *buf = ret;

	buf += sprintf(buf, "sfu: %s r%d\n\t\t",
		       mfu_opcode(mfu->opcode), mfu->reg);

	buf += sprintf(buf, "mul0: %s\n\t\t", mfu_mul(mfu, 0));
	buf += sprintf(buf, "mul1: %s\n\t\t", mfu_mul(mfu, 1));

	sprintf(buf, "ipl: %s, %s, %s, %s",
		mfu_var(mfu, 0), mfu_var(mfu, 1),
		mfu_var(mfu, 2), mfu_var(mfu, 3));

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
		case FRAGMENT_EMBEDDED_CONSTANT_0 ... FRAGMENT_EMBEDDED_CONSTANT_2:
			if (fixed10) {
				if (sub_reg_select_high) {
					imm_fx10_high[reg_select - FRAGMENT_EMBEDDED_CONSTANT_0] = 1;
				} else {
					imm_fx10_low[reg_select - FRAGMENT_EMBEDDED_CONSTANT_0] = 1;
				}
			} else {
				imm_fp20[reg_select - FRAGMENT_EMBEDDED_CONSTANT_0] = 1;
			}

			alu_imm_used = 1;
			break;
		case FRAGMENT_CONDITION_REG_0 ... FRAGMENT_CONDITION_REG_7:
		case FRAGMENT_LOWP_VEC2_0_1:
		case FRAGMENT_POS_X:
		case FRAGMENT_POS_Y:
		case FRAGMENT_POLYGON_FACE:
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
		sprintf(buf, ",");
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
	int adj = -1;

	switch (alu->dst_reg) {
	case FRAGMENT_CONDITION_REG_0 ... FRAGMENT_CONDITION_REG_7:
		adj = (alu->write_low_sub_reg || alu->write_low_sub_reg);
	case FRAGMENT_KILL_REG:
		buf += sprintf(buf, "%s", reg_name(alu->dst_reg, adj));
		break;
	default:
		buf += sprintf(buf, "%s.", reg_name(alu->dst_reg, -1));
		buf += sprintf(buf, alu->write_low_sub_reg ? "l" : "*");
		buf += sprintf(buf, alu->write_high_sub_reg ? "h" : "*");
		break;
	}

	sprintf(buf, ",");

	return ret;
}

static const char * alu_opcode(const union fragment_alu_instruction *alu)
{
	switch (alu->opcode) {
	case ALU_OPCODE_MAD:
		if (alu->addition_disable) {
			return "MUL";
		} else {
			return "MAD";
		}
	case ALU_OPCODE_MIN:
		return "MIN";
	case ALU_OPCODE_MAX:
		return "MAX";
	case ALU_OPCODE_CSEL:
		return "CSEL";
	default:
		break;
	}

	return "ERR!";
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
		sprintf(buf, "(eq)");
		break;
	case ALU_CC_GREATER_THAN_ZERO:
		sprintf(buf, "(gt)");
		break;
	case ALU_CC_ZERO_OR_GREATER:
		sprintf(buf, "(ge)");
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

	buf += sprintf(buf, "\t\tALU0:\t%s\n", disassemble_alu(&alu->a[0]));
	buf += sprintf(buf, "\t\tALU1:\t%s\n", disassemble_alu(&alu->a[1]));
	buf += sprintf(buf, "\t\tALU2:\t%s\n", disassemble_alu(&alu->a[2]));

	if (!alu_imm_used) {
		sprintf(buf, "\t\tALU3:\t%s\n", disassemble_alu(&alu->a[3]));
	} else {
		sprintf(buf, "\t\tALU3:\t%s\n", disassemble_alu_imm(alu));
	}

	return ret;
}

static const char * disassemble_tex(const tex_instr *tex)
{
	static char ret[64];
	char *buf = ret;

	if (tex->enable_bias) {
		buf += sprintf(buf, "txb ");
	} else {
		buf += sprintf(buf, "tex ");
	}

	if (tex->sample_dst_regs_select) {
		buf += sprintf(buf, "r2, r3, ");
	} else {
		buf += sprintf(buf, "r0, r1, ");
	}

	buf += sprintf(buf, "tex%d, ", tex->sampler_index);

	if (tex->src_regs_select == TEX_SRC_R2_R3_R0_R1) {
		buf += sprintf(buf, "r2, r3, r0");
	} else {
		buf += sprintf(buf, "r0, r1, r2");
	}

	if (tex->enable_bias) {
		buf += sprintf(buf, tex->src_regs_select ? ", r1" : ", r3");
	}

	if (tex->unk_6_9 || tex->unk_11 || tex->unk_13_31) {
		sprintf(buf, " // 0x%08X", tex->data);
	}

	return ret;
}

static const char * disassemble_dw(const dw_instr *dw)
{
	static char ret[64];
	char *buf = ret;

	if (dw->enable) {
		buf += sprintf(buf, "store ");

		if (dw->stencil_write && dw->render_target_index == 2) {
			buf += sprintf(buf, "stencil");
		} else {
			buf += sprintf(buf, "rt%d, ", dw->render_target_index);

			if (dw->src_regs_select) {
				buf += sprintf(buf, "r2, r3");
			} else {
				buf += sprintf(buf, "r0, r1");
			}
		}
	} else {
		buf += sprintf(buf, "NOP");
	}

	if (dw->unk_1 || dw->unk_6_9 || dw->unk_11_14 || dw->unk_16_31 != 2) {
		sprintf(buf, " // 0x%08X", dw->data);
	}

	return ret;
}

static const char * disassemble_pseq(const pseq_instr *pseq)
{
	static char ret[64];
	char *buf = ret;

	buf += sprintf(buf, "fetch ");

	if (pseq->dst_regs_select) {
		buf += sprintf(buf, "r2, r3, ");
	} else {
		buf += sprintf(buf, "r0, r1, ");
	}

	buf += sprintf(buf, "rt%u", pseq->rt_select);

	if (pseq->unk_0 || pseq->unk_2 || pseq->unk_4_15 ||
	    pseq->unk_20_22 || pseq->unk_24_31) {
		sprintf(buf, " // 0x%08X", pseq->data);
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
	char *locale;
	char *buf = ret;
	unsigned i;

	locale = strdup( setlocale(LC_ALL, NULL) );
	if (!locale)
		return "Out of memory";

	/* float decimal point is locale-dependent! */
	setlocale(LC_ALL, "C");

	buf += sprintf(buf, "EXEC\n");

	if (pseq->data != 0x00000000) {
		buf += sprintf(buf, "\tPSEQ:\t%s\n", disassemble_pseq(pseq));
	}

	for (i = 0; i < mfu_nb; i++) {
		buf += sprintf(buf, "\tMFU:\t%s\n", disassemble_mfu(mfu + i));
	}

	if (tex->data != 0x00000000) {
		buf += sprintf(buf, "\tTEX:\t%s\n", disassemble_tex(tex));
	}

	for (i = 0; i < alu_nb; i++) {
		buf += sprintf(buf, "\tALU:\n%s", disassemble_alus(alu + i));
	}

	if (dw->data != 0x00000000) {
		buf += sprintf(buf, "\tDW:\t%s\n", disassemble_dw(dw));
	}

	sprintf(buf, ";");

	/* restore locale */
	setlocale(LC_ALL, locale);
	free(locale);

	return ret;
}
