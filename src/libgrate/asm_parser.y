/*
 * Copyright (c) 2016 Dmitry Osipenko <digetx@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, see <http://www.gnu.org/licenses/>.
 */

%{
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <asm.h>

extern int yylex(void);
extern int yylineno;
extern int yydebug;

void yyerror(char *err)
{
	fprintf(stderr, "line %d: %s\n", yylineno, err);
}

vpe_instr128 asm_vs_instructions[256];

asm_const asm_vs_constants[256];

asm_in_out asm_vs_attributes[16];

asm_in_out asm_vs_exports[16];

int asm_vs_instructions_nb;

struct parse_state {
	int rC_used;
	int uniform_used;
	int attribute_used;

	int opcode;

	int type;
	int negate;
	int absolute;
	int index;
	int swizzle_x;
	int swizzle_y;
	int swizzle_z;
	int swizzle_w;
	int wr_x;
	int wr_y;
	int wr_z;
	int wr_w;

	int rD;

	int address_register_select;
	int attribute_fetch_index;
	int uniform_fetch_index;

	int constant_relative_addressing_enable;
	int attribute_relative_addressing_enable;

	int export_write_index;
};

static struct parse_state pst;
static vpe_instr128 instr;

#define PARSE_ERROR(txt)	\
	{			\
		yyerror(txt);	\
		YYABORT;	\
	}

static int swizzle(int component)
{
	switch (component) {
	case 'x': return SWIZZLE_X;
	case 'y': return SWIZZLE_Y;
	case 'z': return SWIZZLE_Z;
	case 'w': return SWIZZLE_W;
	default: break;
	}

	return -1;
}

static void reset_instruction(void)
{
	memset(&pst, 0, sizeof(pst));
	memset(&instr, 0, sizeof(instr));

	instr.export_write_index = 31;

	instr.rA_swizzle_x = SWIZZLE_X;
	instr.rA_swizzle_y = SWIZZLE_Y;
	instr.rA_swizzle_z = SWIZZLE_Z;
	instr.rA_swizzle_w = SWIZZLE_W;

	instr.rB_swizzle_x = SWIZZLE_X;
	instr.rB_swizzle_y = SWIZZLE_Y;
	instr.rB_swizzle_z = SWIZZLE_Z;
	instr.rB_swizzle_w = SWIZZLE_W;

	instr.rC_swizzle_x = SWIZZLE_X;
	instr.rC_swizzle_y = SWIZZLE_Y;
	instr.rC_swizzle_z = SWIZZLE_Z;
	instr.rC_swizzle_w = SWIZZLE_W;

	instr.predicate_swizzle_x = SWIZZLE_X;
	instr.predicate_swizzle_y = SWIZZLE_Y;
	instr.predicate_swizzle_z = SWIZZLE_Z;
	instr.predicate_swizzle_w = SWIZZLE_W;
}

void reset_asm_parser_state(void)
{
	memset(&asm_vs_attributes, 0, sizeof(asm_vs_attributes));
	memset(&asm_vs_constants, 0, sizeof(asm_vs_constants));
	memset(&asm_vs_exports, 0, sizeof(asm_vs_exports));

	reset_instruction();

	asm_vs_instructions_nb = 0;

	yylineno = 1;
	yydebug = 0;
}
%}

%token T_VECTOR_OPCODE_NOP
%token T_VECTOR_OPCODE_MOV
%token T_VECTOR_OPCODE_MUL
%token T_VECTOR_OPCODE_ADD
%token T_VECTOR_OPCODE_MAD
%token T_VECTOR_OPCODE_DP3
%token T_VECTOR_OPCODE_DPH
%token T_VECTOR_OPCODE_DP4
%token T_VECTOR_OPCODE_DST
%token T_VECTOR_OPCODE_MIN
%token T_VECTOR_OPCODE_MAX
%token T_VECTOR_OPCODE_SLT
%token T_VECTOR_OPCODE_SGE
%token T_VECTOR_OPCODE_ARL
%token T_VECTOR_OPCODE_FRC
%token T_VECTOR_OPCODE_FLR
%token T_VECTOR_OPCODE_SEQ
%token T_VECTOR_OPCODE_SFL
%token T_VECTOR_OPCODE_SGT
%token T_VECTOR_OPCODE_SLE
%token T_VECTOR_OPCODE_SNE
%token T_VECTOR_OPCODE_STR
%token T_VECTOR_OPCODE_SSG
%token T_VECTOR_OPCODE_ARR
%token T_VECTOR_OPCODE_ARA
%token T_VECTOR_OPCODE_TXL
%token T_VECTOR_OPCODE_PUSHA
%token T_VECTOR_OPCODE_POPA

%token T_SCALAR_OPCODE_NOP
%token T_SCALAR_OPCODE_MOV
%token T_SCALAR_OPCODE_RCP
%token T_SCALAR_OPCODE_RCC
%token T_SCALAR_OPCODE_RSQ
%token T_SCALAR_OPCODE_EXP
%token T_SCALAR_OPCODE_LOG
%token T_SCALAR_OPCODE_LIT
%token T_SCALAR_OPCODE_BRA
%token T_SCALAR_OPCODE_CAL
%token T_SCALAR_OPCODE_RET
%token T_SCALAR_OPCODE_LG2
%token T_SCALAR_OPCODE_EX2
%token T_SCALAR_OPCODE_SIN
%token T_SCALAR_OPCODE_COS
%token T_SCALAR_OPCODE_PUSHA
%token T_SCALAR_OPCODE_POPA

%token T_ASM
%token T_EXPORTS
%token T_CONSTANTS
%token T_ATTRIBUTES
%token T_EXEC
%token T_EXEC_END
%token T_REGISTER
%token T_ATTRIBUTE
%token T_CONSTANT
%token T_UNDEFINED
%token T_NEG
%token T_ABS
%token T_SET_CONDITION
%token T_CHECK_CONDITION_LT
%token T_CHECK_CONDITION_EQ
%token T_CHECK_CONDITION_GT
%token T_CHECK_CONDITION_CHECK
%token T_CHECK_CONDITION_WR
%token T_VECTOR
%token T_SCALAR
%token T_EXPORT
%token T_PREDICATE
%token T_CONDITION_REGISTER
%token T_SATURATE

%token T_BIT120

%token T_SYNTAX_ERROR

%token <c> T_COMPONENT_X
%token <c> T_COMPONENT_Y
%token <c> T_COMPONENT_Z
%token <c> T_COMPONENT_W
%token <c> T_COMPONENT_DISABLED
%token <u> T_FLOAT
%token <u> T_HEX
%token <u> T_NUMBER
%token <c> T_ADDRESS_REG
%token <s> T_STRING

%type <c> COMPONENT
%type <c> DST_MASK_X
%type <c> DST_MASK_Y
%type <c> DST_MASK_Z
%type <c> DST_MASK_W
%type <c> ADDRESS_REG
%type <u> VALUE

%union {
	char c;
	float f;
	uint32_t u;
	char s[256];
}

%%

program: program sections
	|
	;

sections:
	T_ASM INSTRUCTIONS
	|
	T_CONSTANTS CONSTANTS
	|
	T_ATTRIBUTES ATTRIBUTES
	|
	T_EXPORTS EXPORTS
	;

EXPORTS: EXPORTS EXPORT
	|
	;

EXPORT:
	'[' T_NUMBER ']' '=' T_STRING ';'
	{
		if ($2 > 15) {
			PARSE_ERROR("Invalid export index");
		}

		if (asm_vs_exports[$2].used) {
			PARSE_ERROR("Overriding export name");
		}

		strcpy(asm_vs_exports[$2].name, $5);
		asm_vs_exports[$2].used = 1;
	}
	;

ATTRIBUTES: ATTRIBUTES ATTRIBUTE
	|
	;

ATTRIBUTE:
	'[' T_NUMBER ']' '=' T_STRING ';'
	{
		if ($2 > 15) {
			PARSE_ERROR("Invalid attribute index");
		}

		if (asm_vs_attributes[$2].used) {
			PARSE_ERROR("Overriding attribute name");
		}

		strcpy(asm_vs_attributes[$2].name, $5);
		asm_vs_attributes[$2].used = 1;
	}
	;

CONSTANTS: CONSTANTS CONSTANT
	|
	;

CONSTANT:
	'[' T_NUMBER ']' '.' COMPONENT '=' VALUE ';'
	{
		int overwrite = 0;

		if ($2 > 255) {
			PARSE_ERROR("Invalid constant index");
		}

		switch ($5) {
		case 'x':
			overwrite = asm_vs_constants[$2].vector.x.dirty;
			asm_vs_constants[$2].vector.x.value = $7;
			asm_vs_constants[$2].vector.x.dirty = 1;
			break;
		case 'y':
			overwrite = asm_vs_constants[$2].vector.y.dirty;
			asm_vs_constants[$2].vector.y.value = $7;
			asm_vs_constants[$2].vector.y.dirty = 1;
			break;
		case 'z':
			overwrite = asm_vs_constants[$2].vector.z.dirty;
			asm_vs_constants[$2].vector.z.value = $7;
			asm_vs_constants[$2].vector.z.dirty = 1;
			break;
		case 'w':
			overwrite = asm_vs_constants[$2].vector.w.dirty;
			asm_vs_constants[$2].vector.w.value = $7;
			asm_vs_constants[$2].vector.w.dirty = 1;
			break;
		default:
			PARSE_ERROR("Something gone wrong");
		}

		if (overwrite) {
			PARSE_ERROR("Overriding constant component");
		}

		asm_vs_constants[$2].used = 1;
	}
	;

VALUE:
	T_NUMBER
	|
	T_FLOAT
	|
	T_HEX
	;

INSTRUCTIONS: INSTRUCTIONS VLIW_INSTRUCTION
	|
	;

VLIW_INSTRUCTION: EXEC_PREAMBLE OPTIONS OPCODES ';'
	{
		if (asm_vs_instructions_nb == 256) {
			PARSE_ERROR("Over 256 vertex instructions");
		}

		asm_vs_instructions[asm_vs_instructions_nb++] = instr;

		reset_instruction();
	}
	;

EXEC_PREAMBLE:
	T_EXEC
	|
	T_EXEC_END
	{
		instr.end_of_program = 1;
	}
	;

OPTIONS:
	OPTIONS '(' OPTION ')'
	|
	;

OPTION:
	T_PREDICATE '.' COMPONENT COMPONENT COMPONENT COMPONENT
	{
		instr.predicate_swizzle_x = swizzle($3);
		instr.predicate_swizzle_y = swizzle($4);
		instr.predicate_swizzle_z = swizzle($5);
		instr.predicate_swizzle_w = swizzle($6);
	}
	|
	T_SET_CONDITION
	{
		instr.condition_set = 1;
	}
	|
	T_CHECK_CONDITION_EQ
	{
		instr.predicate_eq = 1;
	}
	|
	T_CHECK_CONDITION_LT
	{
		instr.predicate_lt = 1;
	}
	|
	T_CHECK_CONDITION_GT
	{
		instr.predicate_gt = 1;
	}
	|
	T_CHECK_CONDITION_CHECK
	{
		instr.condition_check = 1;
	}
	|
	T_CHECK_CONDITION_WR
	{
		instr.condition_flags_write_enable = 1;
	}
	|
	T_CONDITION_REGISTER '=' T_NUMBER
	{
		if ($3 > 1) {
			PARSE_ERROR("Invalid condition register index");
		}

		instr.condition_register_index = $3;
	}
	|
	T_EXPORT '[' EXPORT_REG ']' '=' EXPORT_SRC
	{
		if (pst.export_write_index > 15 && pst.export_write_index != 31) {
			PARSE_ERROR("Invalid export register index");
		}

		instr.export_write_index = pst.export_write_index;
	}
	|
	T_SATURATE
	{
		instr.saturate_result = 1;
	}
	|
	T_BIT120
	{
		instr.bit120 = 1;
	}
	;

EXPORT_REG:
	T_NUMBER
	{
		instr.export_relative_addressing_enable = 0;
		pst.export_write_index = $1;
	}
	|
	ADDRESS_REG
	{
		instr.export_relative_addressing_enable = 1;
		instr.address_register_select = $1;
		pst.export_write_index = 0;
	}
	|
	ADDRESS_REG '+' T_NUMBER
	{
		instr.export_relative_addressing_enable = 1;
		instr.address_register_select = $1;
		pst.export_write_index = $3;
	}
	;

EXPORT_SRC:
	T_VECTOR
	{
		instr.export_vector_write_enable = 1;
	}
	|
	T_SCALAR
	{
		instr.export_vector_write_enable = 0;
	}
	;

OPCODES:
	POST_VECTOR_OPCODE OPTIONAL_SCALAR_OPCODE
	|
	POST_SCALAR_OPCODE OPTIONAL_VECTOR_OPCODE
	;

OPTIONAL_SCALAR_OPCODE:
	POST_SCALAR_OPCODE
	|
	{
		pst.opcode = SCALAR_OPCODE_NOP;
		pst.rD = 63;
	}
	;

OPTIONAL_VECTOR_OPCODE:
	POST_VECTOR_OPCODE
	|
	{
		pst.opcode = VECTOR_OPCODE_NOP;
		pst.rD = 63;
	}
	;

POST_VECTOR_OPCODE:
	VECTOR_OPCODE
	{
		instr.vector_opcode = pst.opcode;
		instr.vector_op_write_x_enable = pst.wr_x;
		instr.vector_op_write_y_enable = pst.wr_y;
		instr.vector_op_write_z_enable = pst.wr_z;
		instr.vector_op_write_w_enable = pst.wr_w;
		instr.vector_rD_index = pst.rD;

		pst.wr_x = pst.wr_y = pst.wr_z = pst.wr_w = 0;
	}
	;

POST_SCALAR_OPCODE:
	SCALAR_OPCODE
	{
		instr.scalar_opcode = pst.opcode;
		instr.scalar_op_write_x_enable = pst.wr_x;
		instr.scalar_op_write_y_enable = pst.wr_y;
		instr.scalar_op_write_z_enable = pst.wr_z;
		instr.scalar_op_write_w_enable = pst.wr_w;
		instr.scalar_rD_index = pst.rD;

		pst.wr_x = pst.wr_y = pst.wr_z = pst.wr_w = 0;
	}
	;

VECTOR_OPCODE:
	T_VECTOR_OPCODE_NOP
	{
		pst.opcode = VECTOR_OPCODE_NOP;
		pst.rD = 63;
	}
	|
	T_VECTOR_OPCODE_MOV REGISTER_DST_MASKED ',' REGISTER_SRC_A
	{
		pst.opcode = VECTOR_OPCODE_MOV;
	}
	|
	T_VECTOR_OPCODE_MUL REGISTER_DST_MASKED ',' REGISTER_SRC_A ',' REGISTER_SRC_B
	{
		pst.opcode = VECTOR_OPCODE_MUL;
	}
	|
	T_VECTOR_OPCODE_ADD REGISTER_DST_MASKED ',' REGISTER_SRC_A ',' REGISTER_SRC_C
	{
		pst.opcode = VECTOR_OPCODE_ADD;
	}
	|
	T_VECTOR_OPCODE_MAD REGISTER_DST_MASKED ',' REGISTER_SRC_A ',' REGISTER_SRC_B ',' REGISTER_SRC_C
	{
		pst.opcode = VECTOR_OPCODE_MAD;
	}
	|
	T_VECTOR_OPCODE_DP3 REGISTER_DST_MASKED ',' REGISTER_SRC_A ',' REGISTER_SRC_B
	{
		pst.opcode = VECTOR_OPCODE_DP3;
	}
	|
	T_VECTOR_OPCODE_DPH REGISTER_DST_MASKED ',' REGISTER_SRC_A ',' REGISTER_SRC_B
	{
		pst.opcode = VECTOR_OPCODE_DPH;
	}
	|
	T_VECTOR_OPCODE_DP4 REGISTER_DST_MASKED ',' REGISTER_SRC_A ',' REGISTER_SRC_B
	{
		pst.opcode = VECTOR_OPCODE_DP4;
	}
	|
	T_VECTOR_OPCODE_DST REGISTER_DST_MASKED ',' REGISTER_SRC_A ',' REGISTER_SRC_B
	{
		pst.opcode = VECTOR_OPCODE_DST;
	}
	|
	T_VECTOR_OPCODE_MIN REGISTER_DST_MASKED ',' REGISTER_SRC_A ',' REGISTER_SRC_B
	{
		pst.opcode = VECTOR_OPCODE_MIN;
	}
	|
	T_VECTOR_OPCODE_MAX REGISTER_DST_MASKED ',' REGISTER_SRC_A ',' REGISTER_SRC_B
	{
		pst.opcode = VECTOR_OPCODE_MAX;
	}
	|
	T_VECTOR_OPCODE_SLT REGISTER_DST_MASKED ',' REGISTER_SRC_A ',' REGISTER_SRC_B
	{
		pst.opcode = VECTOR_OPCODE_SLT;
	}
	|
	T_VECTOR_OPCODE_SGE REGISTER_DST_MASKED ',' REGISTER_SRC_A ',' REGISTER_SRC_B
	{
		pst.opcode = VECTOR_OPCODE_SGE;
	}
	|
	T_VECTOR_OPCODE_ARL ADDRESS_DST_MASKED ',' REGISTER_SRC_A
	{
		pst.opcode = VECTOR_OPCODE_ARL;
	}
	|
	T_VECTOR_OPCODE_FRC REGISTER_DST_MASKED ',' REGISTER_SRC_A
	{
		pst.opcode = VECTOR_OPCODE_FRC;
	}
	|
	T_VECTOR_OPCODE_FLR REGISTER_DST_MASKED ',' REGISTER_SRC_A
	{
		pst.opcode = VECTOR_OPCODE_FLR;
	}
	|
	T_VECTOR_OPCODE_SEQ REGISTER_DST_MASKED ',' REGISTER_SRC_A ',' REGISTER_SRC_B
	{
		pst.opcode = VECTOR_OPCODE_SEQ;
	}
	|
	T_VECTOR_OPCODE_SFL REGISTER_DST_MASKED
	{
		pst.opcode = VECTOR_OPCODE_SFL;
	}
	|
	T_VECTOR_OPCODE_SGT REGISTER_DST_MASKED ',' REGISTER_SRC_A ',' REGISTER_SRC_B
	{
		pst.opcode = VECTOR_OPCODE_SGT;
	}
	|
	T_VECTOR_OPCODE_SLE REGISTER_DST_MASKED ',' REGISTER_SRC_A ',' REGISTER_SRC_B
	{
		pst.opcode = VECTOR_OPCODE_SLE;
	}
	|
	T_VECTOR_OPCODE_SNE REGISTER_DST_MASKED ',' REGISTER_SRC_A ',' REGISTER_SRC_B
	{
		pst.opcode = VECTOR_OPCODE_SNE;
	}
	|
	T_VECTOR_OPCODE_STR REGISTER_DST_MASKED
	{
		pst.opcode = VECTOR_OPCODE_STR;
	}
	|
	T_VECTOR_OPCODE_SSG REGISTER_DST_MASKED
	{
		pst.opcode = VECTOR_OPCODE_SSG;
	}
	|
	T_VECTOR_OPCODE_ARR ADDRESS_DST_MASKED ',' REGISTER_SRC_A
	{
		pst.opcode = VECTOR_OPCODE_ARR;
	}
	|
	T_VECTOR_OPCODE_ARA ADDRESS_DST_MASKED
	{
		pst.opcode = VECTOR_OPCODE_ARA;
	}
	|
	T_VECTOR_OPCODE_TXL REGISTER_DST_MASKED
	{
		pst.opcode = VECTOR_OPCODE_TXL;
	}
	|
	T_VECTOR_OPCODE_PUSHA
	{
		pst.opcode = VECTOR_OPCODE_PUSHA;
		pst.rD = 0;
	}
	|
	T_VECTOR_OPCODE_POPA
	{
		pst.opcode = VECTOR_OPCODE_POPA;
		pst.rD = 0;
	}
	;

SCALAR_OPCODE:
	T_SCALAR_OPCODE_NOP
	{
		pst.opcode = SCALAR_OPCODE_NOP;
		pst.rD = 63;
	}
	|
	T_SCALAR_OPCODE_MOV REGISTER_DST_MASKED ',' REGISTER_SRC_C
	{
		pst.opcode = SCALAR_OPCODE_MOV;
	}
	|
	T_SCALAR_OPCODE_RCP REGISTER_DST_MASKED ',' REGISTER_SRC_C
	{
		pst.opcode = SCALAR_OPCODE_RCP;
	}
	|
	T_SCALAR_OPCODE_RCC REGISTER_DST_MASKED ',' REGISTER_SRC_C
	{
		pst.opcode = SCALAR_OPCODE_RCC;
	}
	|
	T_SCALAR_OPCODE_RSQ REGISTER_DST_MASKED ',' REGISTER_SRC_C
	{
		pst.opcode = SCALAR_OPCODE_RSQ;
	}
	|
	T_SCALAR_OPCODE_EXP REGISTER_DST_MASKED ',' REGISTER_SRC_C
	{
		pst.opcode = SCALAR_OPCODE_EXP;
	}
	|
	T_SCALAR_OPCODE_LOG REGISTER_DST_MASKED ',' REGISTER_SRC_C
	{
		pst.opcode = SCALAR_OPCODE_LOG;
	}
	|
	T_SCALAR_OPCODE_LIT REGISTER_DST_MASKED ',' REGISTER_SRC_C
	{
		pst.opcode = SCALAR_OPCODE_LIT;
	}
	|
	T_SCALAR_OPCODE_BRA T_NUMBER
	{
		if ($2 > 255) {
			PARSE_ERROR("Invalid branching address, max 255");
		}

		pst.opcode = SCALAR_OPCODE_BRA;
		pst.rD = 63;
		instr.iaddr = $2;
	}
	|
	T_SCALAR_OPCODE_CAL T_NUMBER
	{
		if ($2 > 255) {
			PARSE_ERROR("Invalid callee address, max 255");
		}

		pst.opcode = SCALAR_OPCODE_CAL;
		pst.rD = 63;
		instr.iaddr = $2;
	}
	|
	T_SCALAR_OPCODE_RET
	{
		pst.opcode = SCALAR_OPCODE_RET;
	}
	|
	T_SCALAR_OPCODE_LG2 REGISTER_DST_MASKED ',' REGISTER_SRC_C
	{
		pst.opcode = SCALAR_OPCODE_LG2;
	}
	|
	T_SCALAR_OPCODE_EX2 REGISTER_DST_MASKED ',' REGISTER_SRC_C
	{
		pst.opcode = SCALAR_OPCODE_EX2;
	}
	|
	T_SCALAR_OPCODE_SIN REGISTER_DST_MASKED ',' REGISTER_SRC_C
	{
		pst.opcode = SCALAR_OPCODE_SIN;
	}
	|
	T_SCALAR_OPCODE_COS REGISTER_DST_MASKED ',' REGISTER_SRC_C
	{
		pst.opcode = SCALAR_OPCODE_COS;
	}
	|
	T_SCALAR_OPCODE_PUSHA
	{
		pst.opcode = SCALAR_OPCODE_PUSHA;
		pst.rD = 0;
	}
	|
	T_SCALAR_OPCODE_POPA
	{
		pst.opcode = SCALAR_OPCODE_POPA;
		pst.rD = 0;
	}
	;

REGISTER_DST_MASKED:
	T_REGISTER T_NUMBER '.' DST_MASK_X DST_MASK_Y DST_MASK_Z DST_MASK_W
	{
		pst.wr_x = !($4 == '*');
		pst.wr_y = !($5 == '*');
 		pst.wr_z = !($6 == '*');
		pst.wr_w = !($7 == '*');

		if ($2 > 31 && $2 != 63) {
			PARSE_ERROR("Invalid destination register index");
		}

		pst.rD = $2;
	}
	;

ADDRESS_DST_MASKED:
	REGISTER_DST_MASKED
	|
	T_ADDRESS_REG '.' DST_MASK_X DST_MASK_Y DST_MASK_Z DST_MASK_W
	{
		pst.wr_x = !($3 == '*');
		pst.wr_y = !($4 == '*');
 		pst.wr_z = !($5 == '*');
		pst.wr_w = !($6 == '*');
		pst.rD = 0;
	}
	;

DST_MASK_X: T_COMPONENT_X | T_COMPONENT_DISABLED;
DST_MASK_Y: T_COMPONENT_Y | T_COMPONENT_DISABLED;
DST_MASK_Z: T_COMPONENT_Z | T_COMPONENT_DISABLED;
DST_MASK_W: T_COMPONENT_W | T_COMPONENT_DISABLED;

REGISTER_SRC_A: REGISTER_SRC
	{
		instr.rA_type = pst.type;
		instr.rA_index = pst.index;
		instr.rA_swizzle_x = pst.swizzle_x;
		instr.rA_swizzle_y = pst.swizzle_y;
		instr.rA_swizzle_z = pst.swizzle_z;
		instr.rA_swizzle_w = pst.swizzle_w;
		instr.rA_negate = pst.negate;
		instr.rA_absolute_value = pst.absolute;
	}
	;

REGISTER_SRC_B: REGISTER_SRC
	{
		instr.rB_type = pst.type;
		instr.rB_index = pst.index;
		instr.rB_swizzle_x = pst.swizzle_x;
		instr.rB_swizzle_y = pst.swizzle_y;
		instr.rB_swizzle_z = pst.swizzle_z;
		instr.rB_swizzle_w = pst.swizzle_w;
		instr.rB_negate = pst.negate;
		instr.rB_absolute_value = pst.absolute;
	}
	;

REGISTER_SRC_C: REGISTER_SRC
	{
		if (pst.rC_used && pst.type != instr.rC_type) {
			PARSE_ERROR("rC type conflict");
		}

		if (pst.rC_used &&
			pst.type == REG_TYPE_TEMPORARY &&
				pst.index != instr.rC_index) {
			PARSE_ERROR("rC number conflict");
		}

		pst.rC_used = 1;

		instr.rC_type = pst.type;
		instr.rC_index = pst.index;
		instr.rC_swizzle_x = pst.swizzle_x;
		instr.rC_swizzle_y = pst.swizzle_y;
		instr.rC_swizzle_z = pst.swizzle_z;
		instr.rC_swizzle_w = pst.swizzle_w;
		instr.rC_negate = pst.negate;
		instr.rC_absolute_value = pst.absolute;
	}
	;

REGISTER_SRC:
	REGISTER_SRC_SWIZZLED
	{
		if ((instr.constant_relative_addressing_enable ||
			instr.attribute_relative_addressing_enable ||
				instr.export_relative_addressing_enable) &&
		    (pst.constant_relative_addressing_enable ||
			pst.attribute_relative_addressing_enable))
		{
			if (pst.address_register_select != instr.address_register_select) {
				PARSE_ERROR("Two different relative addresses used");
			}
		}

		instr.constant_relative_addressing_enable  |=
					pst.constant_relative_addressing_enable;
		instr.attribute_relative_addressing_enable |=
					pst.attribute_relative_addressing_enable;

		if (instr.constant_relative_addressing_enable ||
			instr.attribute_relative_addressing_enable)
		{
			instr.address_register_select = pst.address_register_select;
		}

		pst.negate = 0;
		pst.absolute = 0;
	}
	|
	T_NEG '(' REGISTER_SRC ')'
	{
		pst.negate = 1;
	}
	|
	T_ABS '(' REGISTER_SRC ')'
	{
		pst.absolute = 1;
	}
	;

REGISTER_SRC_SWIZZLED:
	T_REGISTER T_NUMBER '.' SWIZZLE
	{
		if ($2 > 31) {
			PARSE_ERROR("Invalid source register index");
		}

		pst.index = $2;
		pst.type = REG_TYPE_TEMPORARY;
	}
	|
	T_CONSTANT '[' CONSTANT_REG ']' '.' SWIZZLE
	{
		if (pst.uniform_fetch_index > 1023) {
			PARSE_ERROR("Invalid constant index");
		}

		if (pst.uniform_used &&
			pst.uniform_fetch_index != instr.uniform_fetch_index)
		{
			PARSE_ERROR("Two different constant indexes");
		}

		instr.uniform_fetch_index = pst.uniform_fetch_index;

		pst.uniform_used = 1;
		pst.type = REG_TYPE_UNIFORM;
		pst.index = 0;
	}
	|
	T_ATTRIBUTE '[' ATTRIBUTE_REG ']' '.' SWIZZLE
	{
		if (pst.attribute_fetch_index > 15) {
			PARSE_ERROR("Invalid attribute index");
		}

		if (pst.attribute_used &&
			pst.attribute_fetch_index != instr.attribute_fetch_index)
		{
			PARSE_ERROR("Two different attribute indexes");
		}

		instr.attribute_fetch_index = pst.attribute_fetch_index;

		pst.attribute_used = 1;
		pst.type = REG_TYPE_ATTRIBUTE;
		pst.index = 0;
	}
	|
	T_UNDEFINED T_NUMBER '.' SWIZZLE
	{
		if ($2 > 31) {
			PARSE_ERROR("Invalid source register index");
		}

		pst.index = $2;
		pst.type = REG_TYPE_UNDEFINED;
	}
	;

CONSTANT_REG:
	T_NUMBER
	{
		pst.uniform_fetch_index = $1;
	}
	|
	ADDRESS_REG
	{
		pst.constant_relative_addressing_enable = 1;
		pst.address_register_select = $1;
		pst.uniform_fetch_index = 0;
	}
	|
	ADDRESS_REG '+' T_NUMBER
	{
		pst.constant_relative_addressing_enable = 1;
		pst.address_register_select = $1;
		pst.uniform_fetch_index = $3;
	}
	;

ATTRIBUTE_REG:
	T_NUMBER
	{
		pst.attribute_fetch_index = $1;
	}
	|
	ADDRESS_REG
	{
		pst.attribute_relative_addressing_enable = 1;
		pst.address_register_select = $1;
		pst.attribute_fetch_index = 0;
	}
	|
	ADDRESS_REG '+' T_NUMBER
	{
		pst.attribute_relative_addressing_enable = 1;
		pst.address_register_select = $1;
		pst.attribute_fetch_index = $3;
	}
	;

ADDRESS_REG:
	T_ADDRESS_REG '.' COMPONENT
	{
		/* swizzle() coincides with the address register number */
		yyval.c = swizzle($3);
	}

SWIZZLE: COMPONENT COMPONENT COMPONENT COMPONENT
	{
		pst.swizzle_x = swizzle($1);
		pst.swizzle_y = swizzle($2);
		pst.swizzle_z = swizzle($3);
		pst.swizzle_w = swizzle($4);
	}
	;

COMPONENT: T_COMPONENT_X | T_COMPONENT_Y | T_COMPONENT_Z | T_COMPONENT_W;

%%
