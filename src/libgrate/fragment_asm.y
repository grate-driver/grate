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

%{
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asm.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define PARSE_ERROR(txt)	\
	{			\
		yyerror(txt);	\
		YYABORT;	\
	}

extern int fragment_asmlex(void);
extern int fragment_asmlineno;

void __attribute__((weak)) yyerror(char *err)
{
	fprintf(stderr, "fs: line %d: %s\n", fragment_asmlineno, err);
}

pseq_instr	asm_pseq_instructions[64];
mfu_instr	asm_mfu_instructions[64];
tex_instr	asm_tex_instructions[64];
alu_instr	asm_alu_instructions[64];
dw_instr	asm_dw_instructions[64];

instr_sched	asm_mfu_sched[64];
instr_sched	asm_alu_sched[64];

uint32_t	asm_fs_constants[32];
asm_in_out	asm_fs_uniforms[32 * 2];

unsigned asm_fs_instructions_nb;
unsigned asm_mfu_instructions_nb;
unsigned asm_alu_instructions_nb;

unsigned asm_alu_buffer_size;
unsigned asm_pseq_to_dw_exec_nb;

int asm_discards_fragment;

static void reset_fragment_asm_parser_state(void)
{
	int i, k;
	memset(asm_pseq_instructions, 0, sizeof(asm_pseq_instructions));
	memset(asm_mfu_instructions, 0, sizeof(asm_mfu_instructions));
	memset(asm_tex_instructions, 0, sizeof(asm_tex_instructions));
	memset(asm_dw_instructions, 0, sizeof(asm_dw_instructions));
	memset(asm_mfu_sched, 0, sizeof(asm_mfu_sched));
	memset(asm_alu_sched, 0, sizeof(asm_alu_sched));
	memset(asm_fs_constants, 0, sizeof(asm_fs_constants));
	memset(asm_fs_uniforms, 0, sizeof(asm_fs_uniforms));

	for (i = 0; i < ARRAY_SIZE(asm_alu_instructions); i++) {
		for (k = 0; k < 4; k++) {
			// a NOP is an instruction that writes 0.0 to r31
			asm_alu_instructions[i].a[k].part0 = 0x3e41f200;
			asm_alu_instructions[i].a[k].part1 = 0x000fe7e8;
		}
	}

	asm_fs_instructions_nb = 0;
	asm_mfu_instructions_nb = 0;
	asm_alu_instructions_nb = 0;
	asm_alu_buffer_size = 1;
	asm_pseq_to_dw_exec_nb = 1;

	asm_discards_fragment = 0;

	fragment_asmlineno = 1;
}

static uint32_t float_to_fp20(float f)
{
	uint32_t sign, mantissa, exponent;
	union {
		uint32_t u;
		float f;
	} value;

	if (f == 0.0f)
		return 0;

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
%}

%token T_ASM
%token T_CONSTANTS
%token T_UNIFORMS
%token T_EXEC
%token T_REGISTER
%token T_UNDEFINED
%token T_NEG
%token T_ABS
%token T_SATURATE

%token <s> T_STRING
%token <u> T_NUMBER

%token T_PSEQ
%token T_MFU
%token T_TEX
%token T_DW

%token T_IMM;

%token T_SYNTAX_ERROR

%token T_ALU
%token <u> T_ALUX
%token T_ALU_COMPLEMENT

%token <aluX_instr> T_OPCODE_NOP

%token T_MFU_OPCODE_VAR
%token T_MFU_OPCODE_RCP
%token T_MFU_OPCODE_RSQ
%token T_MFU_OPCODE_LG2
%token T_MFU_OPCODE_EX2
%token T_MFU_OPCODE_SQRT
%token T_MFU_OPCODE_SIN
%token T_MFU_OPCODE_COS
%token T_MFU_OPCODE_FRC
%token T_MFU_OPCODE_PREEX2
%token T_MFU_OPCODE_PRESIN
%token T_MFU_OPCODE_PRECOS

%token T_MFU_FETCH_INTERPOLATE
%token T_MFU_SFU
%token T_MFU_MUL0
%token T_MFU_MUL1

%token <u> T_MFU_MUL_DST
%token T_MFU_MUL_DST_BARYCENTRIC

%token <u> T_MUL_SRC
%token T_MFU_MUL_SRC_SFU_RESULT
%token T_MFU_MUL_SRC_BARYCENTRIC_0
%token T_MFU_MUL_SRC_BARYCENTRIC_1

%token T_MFU_UNK

%token <u> T_TEX_SAMPLER_ID
%token T_TEX_OPCODE
%token T_TXB_OPCODE

%token T_ALU_rB
%token T_ALU_rC

%token T_ALU_LOWP
%token T_POSITION_X
%token T_POSITION_Y
%token T_POLIGON_FACE
%token T_KILL
%token <u> T_TRAM_ROW
%token <u> T_ROW_REGISTER
%token <u> T_GLOBAL_REGISTER
%token <u> T_ALU_RESULT_REGISTER
%token <u> T_IMMEDIATE
%token <u> T_CONST_0_1
%token <u> T_ALU_UNIFORM
%token <u> T_ALU_CONDITION_REGISTER

%token T_FX10
%token T_FP20

%token T_LOW
%token T_HIGH

%token T_ALU_OPCODE_MAD
%token T_ALU_OPCODE_MUL
%token T_ALU_OPCODE_MIN
%token T_ALU_OPCODE_MAX
%token T_ALU_OPCODE_CSEL

%token T_ALU_ACCUM_THIS
%token T_ALU_ACCUM_OTHER
%token T_ALU_CC_EQ
%token T_ALU_CC_GT
%token T_ALU_CC_GE
%token T_ALU_X2
%token T_ALU_X4
%token T_ALU_DIV2

%token T_DW_STORE
%token T_DW_STENCIL
%token <u> T_DW_RENDER_TARGET

%token <u> T_HEX
%token <f> T_FLOAT

%type <u> UNIFORM_TYPE

%type <u> FP20
%type <u> FX10

%type <u> MFU_VAR_PRECISION
%type <u> MFU_OPCODE
%type <mfu_var> MFU_VAR
%type <u> MFU_MUL_DST
%type <u> MFU_MUL_SRC

%type <aluX_instr> ALU_OPERATION
%type <aluX_instr> ALU_OPERATIONS
%type <alu_reg> ALU_SRC_REG_MODIFIED
%type <alu_reg> ALU_SRC
%type <alu_reg> ALU_SRC_FLOAT_TYPE
%type <alu_reg> ALU_SRC_D
%type <alu_reg> ALU_REG_ABS
%type <alu_reg> ALU_REG_x2
%type <alu_reg> ALU_REG
%type <alu_dst> ALU_DST
%type <alu_dst> ALU_DST_REG
%type <alu_dst> ALU_DST_LOW_HIGH
%type <alu_mod> ALU_MODIFIERS
%type <alu_mod> ALU_MODIFIER
%type <u> ALU_OPCODE

%token T_ALU_BUFFER_SIZE
%token T_PSEQ_DW_EXEC_NB

%union {
	char c;
	float f;
	uint64_t u;
	char s[256];

	union fragment_alu_instruction aluX_instr;

	struct {
		unsigned saturate;
		unsigned opcode;
		unsigned source;
	} mfu_var;

	struct alu_reg {
		unsigned disable;
		unsigned index;
		unsigned absolute;
		unsigned minus_one;
		unsigned scale_x2;
		unsigned negate;
		unsigned rD;

		struct lowp {
			unsigned enable;
			unsigned high;
		} lowp;
	} alu_reg;

	struct alu_dst {
		unsigned index;
		unsigned low;
		unsigned high;
	} alu_dst;

	struct alu_mod {
		unsigned accumulate_result_this;
		unsigned accumulate_result_other;
		unsigned saturate;
		unsigned scale;
		unsigned cc;
	} alu_mod;
}

%%

program: program sections
	|
	{
		reset_fragment_asm_parser_state();
	}
	;

sections:
	T_ASM INSTRUCTIONS
	|
	T_CONSTANTS CONSTANTS
	|
	T_UNIFORMS UNIFORMS
	|
	T_ALU_BUFFER_SIZE '=' T_NUMBER
	{
		if ($3 != 0 && $3 > 4) {
			PARSE_ERROR("Invalid ALU buffer size, should be 1-4");
		}

		asm_alu_buffer_size = $3;
	}
	|
	T_PSEQ_DW_EXEC_NB '=' T_NUMBER
	{
		asm_pseq_to_dw_exec_nb = $3;
	}
	;

UNIFORMS: UNIFORMS UNIFORM
	|
	;

UNIFORM:
	'[' T_NUMBER ']' UNIFORM_TYPE '=' T_STRING ';'
	{
		if ($2 > 31) {
			PARSE_ERROR("Invalid uniform index");
		}

		$2 <<= 1;

		switch ($4) {
		case FS_UNIFORM_FP20:
		case FS_UNIFORM_FX10_LOW:
			if (asm_fs_uniforms[$2].used) {
				PARSE_ERROR("Overriding uniform name");
			}

			strcpy(asm_fs_uniforms[$2].name, $6);
			asm_fs_uniforms[$2].type = $4;

			if ($4 != FS_UNIFORM_FP20) {
				break;
			}
		case FS_UNIFORM_FX10_HIGH:
			if (asm_fs_uniforms[$2 + 1].used) {
				PARSE_ERROR("Overriding uniform name");
			}

			strcpy(asm_fs_uniforms[$2 + 1].name, $6);
			asm_fs_uniforms[$2 + 1].type = $4;
			break;
		default:
			PARSE_ERROR("Shouldn't happen");
			break;
		}
	}
	;

UNIFORM_TYPE:
	'.' T_LOW
	{
		yyval.u = FS_UNIFORM_FX10_LOW;
	}
	|
	'.' T_HIGH
	{
		yyval.u = FS_UNIFORM_FX10_HIGH;
	}
	|
	{
		yyval.u = FS_UNIFORM_FP20;
	}
	;

CONSTANTS: CONSTANTS CONSTANT
	|
	;

CONSTANT:
	'[' T_NUMBER ']' '=' T_FLOAT ';'
	{
		if ($2 > 31) {
			PARSE_ERROR("Invalid constant index, max 31");
		}

		asm_fs_constants[$2] = float_to_fp20($5);
	}
	|
	'[' T_NUMBER ']' '=' T_HEX ';'
	{
		if ($2 > 31) {
			PARSE_ERROR("Invalid constant index, max 31");
		}

		asm_fs_constants[$2] = $5;
	}
	|
	'[' T_NUMBER ']' '.' T_LOW '=' T_FLOAT ';'
	{
		if ($2 > 31) {
			PARSE_ERROR("Invalid constant index, max 31");
		}

		asm_fs_constants[$2] = (asm_fs_constants[$2] & ~0x3ff) | float_to_fx10($7);
	}
	|
	'[' T_NUMBER ']' '.' T_LOW '=' T_HEX ';'
	{
		if ($2 > 31) {
			PARSE_ERROR("Invalid constant index, max 31");
		}

		asm_fs_constants[$2] = (asm_fs_constants[$2] & ~0x3ff) | ($7 & 0x3ff);
	}
	|
	'[' T_NUMBER ']' '.' T_HIGH '=' T_FLOAT ';'
	{
		if ($2 > 31) {
			PARSE_ERROR("Invalid constant index, max 31");
		}

		asm_fs_constants[$2] = (asm_fs_constants[$2] & 0x3ff) | float_to_fx10($7) << 10;
	}
	|
	'[' T_NUMBER ']' '.' T_HIGH '=' T_HEX ';'
	{
		if ($2 > 31) {
			PARSE_ERROR("Invalid constant index, max 31");
		}

		asm_fs_constants[$2] = (asm_fs_constants[$2] & 0x3ff) | ($7 & 0x3ff) << 10;
	}
	;

INSTRUCTIONS: INSTRUCTIONS EXEC_PIPELINE_BATCH
	|
	;

EXEC_PIPELINE_BATCH:
	T_EXEC
		PSEQ_INSTRUCTION
		MFU_INSTRUCTIONS
		TEX_INSTRUCTION
		ALU_INSTRUCTIONS
		ALU_COMPLEMENT
		DW_INSTRUCTION
	';'
	{
		if (++asm_fs_instructions_nb > 64) {
			PARSE_ERROR("Over 64 exec batches");
		}
	}
	;

PSEQ_INSTRUCTION:
	T_PSEQ T_HEX
	{
		asm_pseq_instructions[asm_fs_instructions_nb].data = $2;
	}
	|
	T_PSEQ T_OPCODE_NOP
	|
	;

MFU_INSTRUCTIONS: MFU_INSTRUCTIONS MFU_INSTRUCTION
	{
		unsigned instructions_nb;
		unsigned address = 0;

		instructions_nb = asm_mfu_sched[asm_fs_instructions_nb].instructions_nb + 1;

		if (instructions_nb > 3) {
			PARSE_ERROR("Over 3 MFU instructions per exec batch");
		}

		if (asm_mfu_instructions_nb > 0) {
			address = asm_mfu_instructions_nb - instructions_nb + 1;
		}

		if (address + instructions_nb > 64) {
			PARSE_ERROR("Over 64 MFU instructions in total");
		}

		asm_mfu_sched[asm_fs_instructions_nb].instructions_nb++;
		asm_mfu_sched[asm_fs_instructions_nb].address = address;
		asm_mfu_instructions_nb++;
	}
	|
	;

MFU_INSTRUCTION:
	T_MFU T_HEX ',' T_HEX
	{
		asm_mfu_instructions[asm_mfu_instructions_nb].part0 = $4;
		asm_mfu_instructions[asm_mfu_instructions_nb].part1 = $2;
	}
	|
	T_MFU T_OPCODE_NOP
	|
	T_MFU MFU_FUNC MFU_FUNC MFU_FUNC MFU_FUNC
	|
	T_MFU MFU_FUNC MFU_FUNC MFU_FUNC
	|
	T_MFU MFU_FUNC MFU_FUNC
	|
	T_MFU MFU_FUNC
	;

MFU_FUNC:
	T_MFU_MUL0 MFU_MUL_DST ',' MFU_MUL_SRC ',' MFU_MUL_SRC
	{
		asm_mfu_instructions[asm_mfu_instructions_nb].mul0_dst = $2;
		asm_mfu_instructions[asm_mfu_instructions_nb].mul0_src0 = $4;
		asm_mfu_instructions[asm_mfu_instructions_nb].mul0_src1 = $6;
	}
	|
	T_MFU_MUL1 MFU_MUL_DST ',' MFU_MUL_SRC ',' MFU_MUL_SRC
	{
		asm_mfu_instructions[asm_mfu_instructions_nb].mul1_dst = $2;
		asm_mfu_instructions[asm_mfu_instructions_nb].mul1_src0 = $4;
		asm_mfu_instructions[asm_mfu_instructions_nb].mul1_src1 = $6;
	}
	|
	T_MFU_FETCH_INTERPOLATE MFU_VAR ',' MFU_VAR ',' MFU_VAR ',' MFU_VAR
	{
		asm_mfu_instructions[asm_mfu_instructions_nb].var0_saturate = $2.saturate;
		asm_mfu_instructions[asm_mfu_instructions_nb].var0_opcode = $2.opcode;
		asm_mfu_instructions[asm_mfu_instructions_nb].var0_source = $2.source;

		asm_mfu_instructions[asm_mfu_instructions_nb].var1_saturate = $4.saturate;
		asm_mfu_instructions[asm_mfu_instructions_nb].var1_opcode = $4.opcode;
		asm_mfu_instructions[asm_mfu_instructions_nb].var1_source = $4.source;

		asm_mfu_instructions[asm_mfu_instructions_nb].var2_saturate = $6.saturate;
		asm_mfu_instructions[asm_mfu_instructions_nb].var2_opcode = $6.opcode;
		asm_mfu_instructions[asm_mfu_instructions_nb].var2_source = $6.source;

		asm_mfu_instructions[asm_mfu_instructions_nb].var3_saturate = $8.saturate;
		asm_mfu_instructions[asm_mfu_instructions_nb].var3_opcode = $8.opcode;
		asm_mfu_instructions[asm_mfu_instructions_nb].var3_source = $8.source;
	}
	|
	T_MFU_SFU MFU_OPCODE T_ROW_REGISTER
	{
		asm_mfu_instructions[asm_mfu_instructions_nb].opcode = $2;
		asm_mfu_instructions[asm_mfu_instructions_nb].reg = $3;
	}
	;

MFU_MUL_DST:
	T_MFU_MUL_DST
	|
	T_MFU_MUL_DST_BARYCENTRIC
	{
		yyval.u = MFU_MUL_DST_BARYCENTRIC_WEIGHT;
	}
	|
	T_ROW_REGISTER
	{
		if ($1 > 3) {
			PARSE_ERROR("Invalid row register number, r3 max");
		}

		yyval.u = MFU_MUL_DST_ROW_REG_0 + $1;
	}
	;

MFU_MUL_SRC:
	T_ROW_REGISTER
	{
		if ($1 > 3) {
			PARSE_ERROR("Invalid row register number, r3 max");
		}

		yyval.u = MFU_MUL_SRC_ROW_REG_0 + $1;
	}
	|
	T_MFU_MUL_SRC_SFU_RESULT
	{
		yyval.u = MFU_MUL_SRC_SFU_RESULT;
	}
	|
	T_MFU_MUL_SRC_BARYCENTRIC_0
	{
		yyval.u = MFU_MUL_SRC_BARYCENTRIC_COEF_0;
	}
	|
	T_MFU_MUL_SRC_BARYCENTRIC_1
	{
		yyval.u = MFU_MUL_SRC_BARYCENTRIC_COEF_1;
	}
	|
	T_CONST_0_1
	{
		if ($1 != 1) {
			PARSE_ERROR("Invalid constant, should be #1");
		}

		yyval.u = MFU_MUL_SRC_CONST_1;
	}
	|
	T_MUL_SRC
	;

MFU_OPCODE:
	T_OPCODE_NOP
	{
		yyval.u = MFU_NOP;
	}
	|
	T_MFU_OPCODE_RCP
	{
		yyval.u = MFU_RCP;
	}
	|
	T_MFU_OPCODE_RSQ
	{
		yyval.u = MFU_RSQ;
	}
	|
	T_MFU_OPCODE_LG2
	{
		yyval.u = MFU_LG2;
	}
	|
	T_MFU_OPCODE_EX2
	{
		yyval.u = MFU_EX2;
	}
	|
	T_MFU_OPCODE_SQRT
	{
		yyval.u = MFU_SQRT;
	}
	|
	T_MFU_OPCODE_SIN
	{
		yyval.u = MFU_SIN;
	}
	|
	T_MFU_OPCODE_COS
	{
		yyval.u = MFU_COS;
	}
	|
	T_MFU_OPCODE_FRC
	{
		yyval.u = MFU_FRC;
	}
	|
	T_MFU_OPCODE_PREEX2
	{
		yyval.u = MFU_PREEX2;
	}
	|
	T_MFU_OPCODE_PRESIN
	{
		yyval.u = MFU_PRESIN;
	}
	|
	T_MFU_OPCODE_PRECOS
	{
		yyval.u = MFU_PRECOS;
	}
	;

MFU_VAR:
	T_TRAM_ROW '.' MFU_VAR_PRECISION
	{
		if ($1 > 15) {
			PARSE_ERROR("Invalid TRAM row number, should be 0..15");
		}

		yyval.mfu_var.saturate = 0;
		yyval.mfu_var.source = $1;
		yyval.mfu_var.opcode = $3;
	}
	|
	T_SATURATE '(' T_TRAM_ROW '.' MFU_VAR_PRECISION ')'
	{
		if ($3 > 15) {
			PARSE_ERROR("Invalid TRAM row number, should be 0..15");
		}

		yyval.mfu_var.saturate = 1;
		yyval.mfu_var.source = $3;
		yyval.mfu_var.opcode = $5;
	}
	|
	T_OPCODE_NOP
	{
		memset(&yyval.mfu_var, 0, sizeof(yyval.mfu_var));
	}
	|
	T_SATURATE '(' T_OPCODE_NOP ')'
	{
		memset(&yyval.mfu_var, 0, sizeof(yyval.mfu_var));
		yyval.mfu_var.saturate = 1;
	}
	;

MFU_VAR_PRECISION:
	T_FP20
	{
		yyval.u = MFU_VAR_FP20;
	}
	|
	T_FX10
	{
		yyval.u = MFU_VAR_FX10;
	}
	;

TEX_INSTRUCTION:
	T_TEX T_TEX_OPCODE T_ROW_REGISTER ',' T_ROW_REGISTER ',' T_TEX_SAMPLER_ID ',' T_ROW_REGISTER ',' T_ROW_REGISTER ',' T_ROW_REGISTER
	{
		asm_tex_instructions[asm_fs_instructions_nb].enable = 1;

		if ($3 == 0 && $5 == 1) {
			asm_tex_instructions[asm_fs_instructions_nb].sample_dst_regs_select = 0;
		}
		else if ($3 == 2 && $5 == 3) {
			asm_tex_instructions[asm_fs_instructions_nb].sample_dst_regs_select = 1;
		}
		else {
			PARSE_ERROR("TEX destination registers should be either \"r0,r1\" or \"r2,r3\"");
		}

		if ($7 > 15){
			PARSE_ERROR("Invalid TEX sampler id, 15 is maximum");
		}

		asm_tex_instructions[asm_fs_instructions_nb].sampler_index = $7;

		if ($9 == 0 && $11 == 1 && $13 == 2) {
			asm_tex_instructions[asm_fs_instructions_nb].src_regs_select = TEX_SRC_R0_R1_R2_R3;
		}
		else if ($9 == 2 && $11 == 3 && $13 == 0) {
			asm_tex_instructions[asm_fs_instructions_nb].src_regs_select = TEX_SRC_R2_R3_R0_R1;
		}
		else {
			PARSE_ERROR("TEX source registers should be either \"r0,r1,r2\" or \"r2,r3,r0\"");
		}
	}
	|
	T_TEX T_TXB_OPCODE T_ROW_REGISTER ',' T_ROW_REGISTER ',' T_TEX_SAMPLER_ID ',' T_ROW_REGISTER ',' T_ROW_REGISTER ',' T_ROW_REGISTER ',' T_ROW_REGISTER
	{
		asm_tex_instructions[asm_fs_instructions_nb].enable = 1;

		if ($3 == 0 && $5 == 1) {
			asm_tex_instructions[asm_fs_instructions_nb].sample_dst_regs_select = 0;
		}
		else if ($3 == 2 && $5 == 3) {
			asm_tex_instructions[asm_fs_instructions_nb].sample_dst_regs_select = 1;
		}
		else {
			PARSE_ERROR("TEX destination registers should be either \"r0,r1\" or \"r2,r3\"");
		}

		if ($7 > 15){
			PARSE_ERROR("Invalid TEX sampler id, 15 is maximum");
		}

		asm_tex_instructions[asm_fs_instructions_nb].sampler_index = $7;
		asm_tex_instructions[asm_fs_instructions_nb].enable_bias = 1;

		if ($9 == 0 && $11 == 1 && $13 == 2 && $15 == 3) {
			asm_tex_instructions[asm_fs_instructions_nb].src_regs_select = TEX_SRC_R0_R1_R2_R3;
		}
		else if ($9 == 2 && $11 == 3 && $13 == 0 && $15 == 1) {
			asm_tex_instructions[asm_fs_instructions_nb].src_regs_select = TEX_SRC_R2_R3_R0_R1;
		}
		else {
			PARSE_ERROR("TXB source registers should be either \"r0,r1,r2,r3\" or \"r2,r3,r0,r1\"");
		}
	}
	|
	T_TEX T_HEX
	{
		asm_tex_instructions[asm_fs_instructions_nb].data = $2;
	}
	|
	T_TEX T_OPCODE_NOP
	|
	;

ALU_INSTRUCTIONS: ALU_INSTRUCTIONS ALU_INSTRUCTION
	|
	;

ALU_INSTRUCTION: T_ALU ALUX_INSTRUCTIONS
	{
		unsigned instructions_nb = asm_alu_sched[asm_fs_instructions_nb].instructions_nb + 1;
		unsigned address = 0;

		if (instructions_nb > 3) {
			PARSE_ERROR("Over 3 ALU instructions per exec batch");
		}

		if (asm_alu_instructions_nb > 0) {
			address = asm_alu_instructions_nb - instructions_nb + 1;
		}

		if (address + instructions_nb > 64) {
			PARSE_ERROR("Over 64 ALU instructions in total");
		}

		asm_alu_sched[asm_fs_instructions_nb].instructions_nb++;
		asm_alu_sched[asm_fs_instructions_nb].address = address;
		asm_alu_instructions_nb++;
	}
	;

ALUX_INSTRUCTIONS:
	ALUX_INSTRUCTION ALUX_INSTRUCTION ALUX_INSTRUCTION ALUX_INSTRUCTION
	|
	ALUX_INSTRUCTION ALUX_INSTRUCTION ALUX_INSTRUCTION
	|
	ALUX_INSTRUCTION ALUX_INSTRUCTION
	|
	ALUX_INSTRUCTION
	|
	;

ALUX_INSTRUCTION:
	T_ALUX ALU_OPERATION
	{
		asm_alu_instructions[asm_alu_instructions_nb].a[$1] = $2;
	}
	|
	T_ALUX ALU3_IMMEDIATES
	{
		uint32_t swap = asm_alu_instructions[asm_alu_instructions_nb].part7;

		if ($1 != 3) {
			PARSE_ERROR("ALU immediates can override ALU3 only");
		}

		asm_alu_instructions[asm_alu_instructions_nb].part7 =
			asm_alu_instructions[asm_alu_instructions_nb].part6;

		asm_alu_instructions[asm_alu_instructions_nb].part6 = swap;
	}
	;

ALU3_IMMEDIATES:
	ALU_IMMEDIATE
	|
	ALU_IMMEDIATE ',' ALU_IMMEDIATE
	|
	ALU_IMMEDIATE ',' ALU_IMMEDIATE ',' ALU_IMMEDIATE
	|
	ALU_IMMEDIATE ',' ALU_IMMEDIATE ',' ALU_IMMEDIATE ',' ALU_IMMEDIATE
	|
	ALU_IMMEDIATE ',' ALU_IMMEDIATE ',' ALU_IMMEDIATE ',' ALU_IMMEDIATE ',' ALU_IMMEDIATE
	|
	ALU_IMMEDIATE ',' ALU_IMMEDIATE ',' ALU_IMMEDIATE ',' ALU_IMMEDIATE ',' ALU_IMMEDIATE ',' ALU_IMMEDIATE
	;

ALU_IMMEDIATE:
	T_IMMEDIATE '=' FP20
	{
		switch ($1) {
		case 0:
			asm_alu_instructions[asm_alu_instructions_nb].imm0.fp20 = $3;
			break;
		case 1:
			asm_alu_instructions[asm_alu_instructions_nb].imm1.fp20 = $3;
			break;
		case 2:
			asm_alu_instructions[asm_alu_instructions_nb].imm2.fp20 = $3;
			break;
		default:
			PARSE_ERROR("Invalid immediate number, 2 maximum");
		}
	}
	|
	T_IMMEDIATE '.' T_LOW '=' FX10
	{
		switch ($1) {
		case 0:
			asm_alu_instructions[asm_alu_instructions_nb].imm0.fx10_low = $5;
			break;
		case 1:
			asm_alu_instructions[asm_alu_instructions_nb].imm1.fx10_low = $5;
			break;
		case 2:
			asm_alu_instructions[asm_alu_instructions_nb].imm2.fx10_low = $5;
			break;
		default:
			PARSE_ERROR("Invalid immediate number, 2 maximum");
		}
	}
	|
	T_IMMEDIATE '.' T_HIGH '=' FX10
	{
		switch ($1) {
		case 0:
			asm_alu_instructions[asm_alu_instructions_nb].imm0.fx10_high = $5;
			break;
		case 1:
			asm_alu_instructions[asm_alu_instructions_nb].imm1.fx10_high = $5;
			break;
		case 2:
			asm_alu_instructions[asm_alu_instructions_nb].imm2.fx10_high = $5;
			break;
		default:
			PARSE_ERROR("Invalid immediate number, 2 maximum");
		}
	}
	;

ALU_OPERATION:
	T_HEX ',' T_HEX
	{
		yyval.aluX_instr.part0 = $3;
		yyval.aluX_instr.part1 = $1;
	}
	|
	ALU_OPERATIONS ALU_MODIFIERS
	{
		yyval.aluX_instr = $1;

		yyval.aluX_instr.condition_code		= $2.cc;
		yyval.aluX_instr.accumulate_result_this	= $2.accumulate_result_this;
		yyval.aluX_instr.accumulate_result_other= $2.accumulate_result_other;
		yyval.aluX_instr.scale_result		= $2.scale;
		yyval.aluX_instr.saturate_result	= $2.saturate;
	}
	|
	T_OPCODE_NOP
	{
		memset(&yyval.aluX_instr, 0, sizeof(yyval.aluX_instr));

		yyval.aluX_instr.rC_fixed10		= 1;
		yyval.aluX_instr.rC_reg_select		= FRAGMENT_LOWP_VEC2_0_1;
		yyval.aluX_instr.rB_fixed10		= 1;
		yyval.aluX_instr.rB_reg_select		= FRAGMENT_LOWP_VEC2_0_1;
		yyval.aluX_instr.rA_fixed10		= 1;
		yyval.aluX_instr.rA_reg_select		= FRAGMENT_LOWP_VEC2_0_1;
		yyval.aluX_instr.dst_reg		= FRAGMENT_LOWP_VEC2_0_1;
	}
	;

ALU_MODIFIERS: ALU_MODIFIERS ALU_MODIFIER
	{
		yyval.alu_mod.accumulate_result_other |= $2.accumulate_result_other;
		yyval.alu_mod.accumulate_result_this |= $2.accumulate_result_this;
		yyval.alu_mod.saturate |= $2.saturate;
		yyval.alu_mod.scale |= $2.scale;
		yyval.alu_mod.cc |= $2.cc;
	}
	|
	{
		memset(&yyval.alu_mod, 0, sizeof(yyval.alu_mod));
	}
	;

ALU_MODIFIER:
	'(' T_ALU_ACCUM_THIS ')'
	{
		memset(&yyval.alu_mod, 0, sizeof(yyval.alu_mod));
		yyval.alu_mod.accumulate_result_this = 1;
	}
	|
	'(' T_ALU_ACCUM_OTHER ')'
	{
		memset(&yyval.alu_mod, 0, sizeof(yyval.alu_mod));
		yyval.alu_mod.accumulate_result_other = 1;
	}
	|
	'(' T_ALU_CC_EQ ')'
	{
		memset(&yyval.alu_mod, 0, sizeof(yyval.alu_mod));
		yyval.alu_mod.cc = ALU_CC_ZERO;
	}
	|
	'(' T_ALU_CC_GT ')'
	{
		memset(&yyval.alu_mod, 0, sizeof(yyval.alu_mod));
		yyval.alu_mod.cc = ALU_CC_GREATER_THAN_ZERO;
	}
	|
	'(' T_ALU_CC_GE ')'
	{
		memset(&yyval.alu_mod, 0, sizeof(yyval.alu_mod));
		yyval.alu_mod.cc = ALU_CC_ZERO_OR_GREATER;
	}
	|
	'(' T_ALU_X2 ')'
	{
		memset(&yyval.alu_mod, 0, sizeof(yyval.alu_mod));
		yyval.alu_mod.scale = ALU_SCALE_X2;
	}
	|
	'(' T_ALU_X4 ')'
	{
		memset(&yyval.alu_mod, 0, sizeof(yyval.alu_mod));
		yyval.alu_mod.scale = ALU_SCALE_X4;
	}
	|
	'(' T_ALU_DIV2 ')'
	{
		memset(&yyval.alu_mod, 0, sizeof(yyval.alu_mod));
		yyval.alu_mod.scale = ALU_SCALE_DIV2;
	}
	|
	'(' T_SATURATE ')'
	{
		memset(&yyval.alu_mod, 0, sizeof(yyval.alu_mod));
		yyval.alu_mod.saturate = 1;
	}
	;

ALU_OPERATIONS:
	ALU_OPCODE ALU_DST ',' ALU_SRC ',' ALU_SRC ',' ALU_SRC ALU_SRC_D
	{
		memset(&yyval.aluX_instr, 0, sizeof(yyval.aluX_instr));

		if ($4.rD || $6.rD || $8.rD || !$9.rD) {
			PARSE_ERROR("Invalid src register");
		}

		yyval.aluX_instr.rD_enable		= !$9.disable;
		yyval.aluX_instr.rD_absolute_value	= $9.absolute;
		yyval.aluX_instr.rD_fixed10		= $9.lowp.enable;
		yyval.aluX_instr.rD_minus_one		= $9.minus_one;
		yyval.aluX_instr.rD_sub_reg_select_high	= $9.lowp.high;
		yyval.aluX_instr.rD_reg_select		= $9.index;

		yyval.aluX_instr.rC_scale_by_two	= $8.scale_x2;
		yyval.aluX_instr.rC_negate		= $8.negate;
		yyval.aluX_instr.rC_absolute_value	= $8.absolute;
		yyval.aluX_instr.rC_fixed10		= $8.lowp.enable;
		yyval.aluX_instr.rC_minus_one		= $8.minus_one;
		yyval.aluX_instr.rC_sub_reg_select_high	= $8.lowp.high;
		yyval.aluX_instr.rC_reg_select		= $8.index;

		yyval.aluX_instr.rB_scale_by_two	= $6.scale_x2;
		yyval.aluX_instr.rB_negate		= $6.negate;
		yyval.aluX_instr.rB_absolute_value	= $6.absolute;
		yyval.aluX_instr.rB_fixed10		= $6.lowp.enable;
		yyval.aluX_instr.rB_minus_one		= $6.minus_one;
		yyval.aluX_instr.rB_sub_reg_select_high	= $6.lowp.high;
		yyval.aluX_instr.rB_reg_select		= $6.index;

		yyval.aluX_instr.rA_scale_by_two	= $4.scale_x2;
		yyval.aluX_instr.rA_negate		= $4.negate;
		yyval.aluX_instr.rA_absolute_value	= $4.absolute;
		yyval.aluX_instr.rA_fixed10		= $4.lowp.enable;
		yyval.aluX_instr.rA_minus_one		= $4.minus_one;
		yyval.aluX_instr.rA_sub_reg_select_high	= $4.lowp.high;
		yyval.aluX_instr.rA_reg_select		= $4.index;

		yyval.aluX_instr.write_low_sub_reg	= $2.low;
		yyval.aluX_instr.write_high_sub_reg	= $2.high;
		yyval.aluX_instr.dst_reg		= $2.index;

		yyval.aluX_instr.addition_disable	= !!($1 & 4);
		yyval.aluX_instr.opcode			= $1 & 3;
	}
	;

ALU_OPCODE:
	T_ALU_OPCODE_MAD
	{
		yyval.u = ALU_OPCODE_MAD;
	}
	|
	T_ALU_OPCODE_MUL
	{
		yyval.u = ALU_OPCODE_MAD | 4;
	}
	|
	T_ALU_OPCODE_MIN
	{
		yyval.u = ALU_OPCODE_MIN;
	}
	|
	T_ALU_OPCODE_MAX
	{
		yyval.u = ALU_OPCODE_MAX;
	}
	|
	T_ALU_OPCODE_CSEL
	{
		yyval.u = ALU_OPCODE_CSEL;
	}
	;

ALU_DST:
	ALU_DST_REG '.' ALU_DST_LOW_HIGH ALU_DST_LOW_HIGH
	{
		yyval.alu_dst.index	= $1.index;
		yyval.alu_dst.low	= $3.low || $4.low;
		yyval.alu_dst.high	= $3.high || $4.high;
	}
	|
	ALU_DST_REG '.' ALU_DST_LOW_HIGH
	{
		yyval.alu_dst.index	= $1.index;
		yyval.alu_dst.low	= $3.low;
		yyval.alu_dst.high	= $3.high;
	}
	|
	T_ALU_CONDITION_REGISTER
	{
		if ($1 > 15) {
			PARSE_ERROR("Invalid dst reg, CR should be 0..15");
		}

		yyval.alu_dst.index	= FRAGMENT_CONDITION_REG($1 >> 1);
		yyval.alu_dst.low	= $1 & 1;
		yyval.alu_dst.high	= $1 & 1;
	}
	|
	T_ALU_LOWP
	{
		yyval.alu_dst.index	= FRAGMENT_LOWP_VEC2_0_1;
		yyval.alu_dst.low	= 0;
		yyval.alu_dst.high	= 0;
	}
	|
	T_KILL
	{
		yyval.alu_dst.index	= FRAGMENT_KILL_REG;
		yyval.alu_dst.low	= 1;
		yyval.alu_dst.high	= 1;

		asm_discards_fragment = 1;
	}
	;

ALU_DST_REG:
	T_ROW_REGISTER
	{
		if ($1 > 15) {
			PARSE_ERROR("Invalid dst reg, row should be 0..15");
		}

		yyval.alu_dst.index = FRAGMENT_ROW_REG($1);
	}
	|
	T_GLOBAL_REGISTER
	{
		if ($1 > 7) {
			PARSE_ERROR("Invalid dst reg, global should be 0..7");
		}

		yyval.alu_dst.index = FRAGMENT_GENERAL_PURPOSE_REG($1);
	}
	|
	T_ALU_LOWP
	{
		yyval.alu_dst.index = FRAGMENT_LOWP_VEC2_0_1;
	}
	|
	T_ALU_UNIFORM
	{
		if ($1 > 7) {
			PARSE_ERROR("Invalid dst reg, uniform should be 0..7");
		}

		yyval.alu_dst.index = FRAGMENT_UNIFORM_REG($1);
	}
	;

ALU_DST_LOW_HIGH:
	T_LOW
	{
		yyval.alu_dst.low = 1;
		yyval.alu_dst.high = 0;
	}
	|
	T_HIGH
	{
		yyval.alu_dst.low = 0;
		yyval.alu_dst.high = 1;
	}
	|
	'*'
	{
		yyval.alu_dst.low = 0;
		yyval.alu_dst.high = 0;
	}
	;

ALU_SRC:
	'-' ALU_SRC_REG_MODIFIED
	{
		yyval.alu_reg = $2;
		yyval.alu_reg.negate = 1;
	}
	|
	ALU_SRC_REG_MODIFIED
	{
		yyval.alu_reg = $1;
		yyval.alu_reg.negate = 0;
	}
	;

ALU_SRC_REG_MODIFIED:
	ALU_REG_x2 '-' T_NUMBER
	{
		yyval.alu_reg = $1;

		if ($3 != 0) {
			if ($3 != 1) {
				PARSE_ERROR("Only decrement by 1 is possible");
			}

			yyval.alu_reg.minus_one = 1;
		} else {
			yyval.alu_reg.minus_one = 0;
		}
	}
	|
	ALU_REG_x2
	{
		yyval.alu_reg = $1;
		yyval.alu_reg.minus_one = 0;
	}
	;

ALU_REG_x2:
	ALU_REG_ABS '*' T_NUMBER
	{
		yyval.alu_reg = $1;

		if ($3 != 1) {
			if ($3 != 2) {
				PARSE_ERROR("Only register scale by x2 is possible");
			}

			yyval.alu_reg.scale_x2 = 1;
		}
	}
	|
	ALU_REG_ABS
	{
		yyval.alu_reg = $1;
		yyval.alu_reg.scale_x2 = 0;
	}
	;

ALU_REG_ABS:
	T_ABS '(' ALU_REG ')'
	{
		yyval.alu_reg = $3;
		yyval.alu_reg.absolute = 1;
	}
	|
	ALU_REG
	;

ALU_REG:
	T_ROW_REGISTER ALU_SRC_FLOAT_TYPE
	{
		if ($1 > 15) {
			PARSE_ERROR("Invalid row reg, should be 0..15");
		}

		memset(&yyval.alu_reg, 0, sizeof(yyval.alu_reg));
		yyval.alu_reg.index = FRAGMENT_ROW_REG($1);
		yyval.alu_reg.lowp.enable = $2.lowp.enable;
		yyval.alu_reg.lowp.high = $2.lowp.high;
		yyval.alu_reg.absolute = 0;
		yyval.alu_reg.minus_one = 0;
		yyval.alu_reg.rD = 0;
	}
	|
	T_GLOBAL_REGISTER ALU_SRC_FLOAT_TYPE
	{
		if ($1 > 7) {
			PARSE_ERROR("Invalid global reg, should be 0..7");
		}

		memset(&yyval.alu_reg, 0, sizeof(yyval.alu_reg));
		yyval.alu_reg.index = FRAGMENT_GENERAL_PURPOSE_REG($1);
		yyval.alu_reg.lowp.enable = $2.lowp.enable;
		yyval.alu_reg.lowp.high = $2.lowp.high;
		yyval.alu_reg.absolute = 0;
		yyval.alu_reg.minus_one = 0;
		yyval.alu_reg.rD = 0;
	}
	|
	T_ALU_RESULT_REGISTER ALU_SRC_FLOAT_TYPE
	{
		if ($1 > 3) {
			PARSE_ERROR("Invalid alu reg, should be 0..3");
		}

		memset(&yyval.alu_reg, 0, sizeof(yyval.alu_reg));
		yyval.alu_reg.index = FRAGMENT_ALU_RESULT_REG($1);
		yyval.alu_reg.lowp.enable = $2.lowp.enable;
		yyval.alu_reg.lowp.high = $2.lowp.high;
		yyval.alu_reg.absolute = 0;
		yyval.alu_reg.minus_one = 0;
		yyval.alu_reg.rD = 0;
	}
	|
	T_IMMEDIATE ALU_SRC_FLOAT_TYPE
	{
		if ($1 > 2) {
			PARSE_ERROR("Invalid immediate reg, should be 0..2");
		}

		memset(&yyval.alu_reg, 0, sizeof(yyval.alu_reg));
		yyval.alu_reg.index = FRAGMENT_EMBEDDED_CONSTANT($1);
		yyval.alu_reg.lowp.enable = $2.lowp.enable;
		yyval.alu_reg.lowp.high = $2.lowp.high;
		yyval.alu_reg.absolute = 0;
		yyval.alu_reg.minus_one = 0;
		yyval.alu_reg.rD = 0;
	}
	|
	T_ALU_UNIFORM  ALU_SRC_FLOAT_TYPE
	{
		if ($1 > 63) {
			PARSE_ERROR("Invalid uniform reg, should be 0..63");
		}

		memset(&yyval.alu_reg, 0, sizeof(yyval.alu_reg));
		yyval.alu_reg.index = FRAGMENT_UNIFORM_REG($1);
		yyval.alu_reg.lowp.enable = $2.lowp.enable;
		yyval.alu_reg.lowp.high = $2.lowp.high;
		yyval.alu_reg.absolute = 0;
		yyval.alu_reg.minus_one = 0;
		yyval.alu_reg.rD = 0;
	}
	|
	T_ALU_CONDITION_REGISTER
	{
		if ($1 > 15) {
			PARSE_ERROR("Invalid CR reg, should be 0..15");
		}

		memset(&yyval.alu_reg, 0, sizeof(yyval.alu_reg));
		yyval.alu_reg.index = FRAGMENT_CONDITION_REG($1 >> 1);
		yyval.alu_reg.lowp.enable = 1;
		yyval.alu_reg.lowp.high = $1 & 1;
		yyval.alu_reg.absolute = 0;
		yyval.alu_reg.minus_one = 0;
		yyval.alu_reg.rD = 0;
	}
	|
	T_CONST_0_1
	{
		memset(&yyval.alu_reg, 0, sizeof(yyval.alu_reg));
		yyval.alu_reg.index = FRAGMENT_LOWP_VEC2_0_1;
		yyval.alu_reg.absolute = 0;
		yyval.alu_reg.minus_one = 0;
		yyval.alu_reg.lowp.enable = 1;
		yyval.alu_reg.lowp.high = $1;
		yyval.alu_reg.rD = 0;
	}
	|
	T_POSITION_X
	{
		memset(&yyval.alu_reg, 0, sizeof(yyval.alu_reg));
		yyval.alu_reg.index = FRAGMENT_POS_X;
		yyval.alu_reg.absolute = 0;
		yyval.alu_reg.minus_one = 0;
		yyval.alu_reg.lowp.enable = 0;
		yyval.alu_reg.lowp.high = 0;
		yyval.alu_reg.rD = 0;
	}
	|
	T_POSITION_Y
	{
		memset(&yyval.alu_reg, 0, sizeof(yyval.alu_reg));
		yyval.alu_reg.index = FRAGMENT_POS_Y;
		yyval.alu_reg.absolute = 0;
		yyval.alu_reg.minus_one = 0;
		yyval.alu_reg.lowp.enable = 0;
		yyval.alu_reg.lowp.high = 0;
		yyval.alu_reg.rD = 0;
	}
	|
	T_POLIGON_FACE
	{
		memset(&yyval.alu_reg, 0, sizeof(yyval.alu_reg));
		yyval.alu_reg.index = FRAGMENT_POLYGON_FACE;
		yyval.alu_reg.absolute = 0;
		yyval.alu_reg.minus_one = 0;
		yyval.alu_reg.lowp.enable = 0;
		yyval.alu_reg.lowp.high = 0;
		yyval.alu_reg.rD = 0;
	}
	|
	T_ALU_rB  ALU_SRC_FLOAT_TYPE
	{
		memset(&yyval.alu_reg, 0, sizeof(yyval.alu_reg));
		yyval.alu_reg.index = 0;
		yyval.alu_reg.lowp.enable = $2.lowp.enable;
		yyval.alu_reg.lowp.high = $2.lowp.high;
		yyval.alu_reg.absolute = 0;
		yyval.alu_reg.minus_one = 0;
		yyval.alu_reg.rD = 1;
	}
	|
	T_ALU_rC ALU_SRC_FLOAT_TYPE
	{
		memset(&yyval.alu_reg, 0, sizeof(yyval.alu_reg));
		yyval.alu_reg.index = 1;
		yyval.alu_reg.lowp.enable = $2.lowp.enable;
		yyval.alu_reg.lowp.high = $2.lowp.high;
		yyval.alu_reg.absolute = 0;
		yyval.alu_reg.minus_one = 0;
		yyval.alu_reg.rD = 1;
	}
	;

ALU_SRC_D:
	',' ALU_SRC
	{
		yyval.alu_reg = $2;

		if (yyval.alu_reg.index == FRAGMENT_LOWP_VEC2_0_1) {
			yyval.alu_reg.rD = !!yyval.alu_reg.lowp.high;
			yyval.alu_reg.disable = yyval.alu_reg.rD;
		}
	}
	|
	{
		memset(&yyval.alu_reg, 0, sizeof(yyval.alu_reg));
		yyval.alu_reg.disable = 1;
		yyval.alu_reg.rD = 1;
	}
	;

ALU_SRC_FLOAT_TYPE:
	'.' T_LOW
	{
		yyval.alu_reg.lowp.enable = 1;
		yyval.alu_reg.lowp.high = 0;
	}
	|
	'.' T_HIGH
	{
		yyval.alu_reg.lowp.enable = 1;
		yyval.alu_reg.lowp.high = 1;
	}
	|
	{
		yyval.alu_reg.lowp.enable = 0;
		yyval.alu_reg.lowp.high = 0;
	}
	;

ALU_COMPLEMENT:
	T_ALU_COMPLEMENT T_HEX
	{
		asm_alu_instructions[asm_fs_instructions_nb].complement = $2;
	}
	|
	{
		asm_alu_instructions[asm_fs_instructions_nb].complement = 0;
	}
	;

DW_INSTRUCTION:
	T_DW T_HEX
	{
		asm_dw_instructions[asm_fs_instructions_nb].data = $2;
	}
	|
	T_DW T_DW_STORE T_DW_RENDER_TARGET ',' T_ROW_REGISTER ',' T_ROW_REGISTER
	{
		asm_dw_instructions[asm_fs_instructions_nb].enable = 1;

		if ($5 == 0 && $7 == 1) {
			asm_dw_instructions[asm_fs_instructions_nb].src_regs_select = 0;
		}
		else if ($5 == 2 && $7 == 3) {
			asm_dw_instructions[asm_fs_instructions_nb].src_regs_select = 1;
		}
		else {
			PARSE_ERROR("DW source registers should be either \"r0,r1\" or \"r2,r3\"");
		}

		if ($3 > 15) {
			PARSE_ERROR("Invalid DW render target, 15 is maximum");
		}

		asm_dw_instructions[asm_fs_instructions_nb].render_target_index = $3;
		asm_dw_instructions[asm_fs_instructions_nb].unk_16_31 = 2;
	}
	|
	T_DW T_DW_STORE T_DW_STENCIL
	{
		asm_dw_instructions[asm_fs_instructions_nb].enable = 1;
		asm_dw_instructions[asm_fs_instructions_nb].stencil_write = 1;
		asm_dw_instructions[asm_fs_instructions_nb].render_target_index = 2;
		asm_dw_instructions[asm_fs_instructions_nb].unk_16_31 = 2;
	}
	|
	T_DW T_OPCODE_NOP
	|
	;

FP20: T_FLOAT
	{
		yyval.u = float_to_fp20($1);
	}
	;

FX10: T_FLOAT
	{
		yyval.u = float_to_fx10($1);
	}
	;

%%
