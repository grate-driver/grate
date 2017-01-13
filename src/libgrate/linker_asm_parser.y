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

#define PARSE_ERROR(txt)	\
	{			\
		yyerror(txt);	\
		YYABORT;	\
	}

extern int linker_asmlex(void);
extern int linker_asmlineno;
extern int linker_asmdebug;

void __attribute__((weak)) yyerror(char *err)
{
	fprintf(stderr, "linker: line %d: %s\n", linker_asmlineno, err);
}

link_instr asm_linker_instructions[32];
unsigned asm_linker_instructions_nb;
unsigned asm_linker_used_tram_rows_nb;

%}

%token <u> T_EXPORT
%token <u> T_TRAM_ROW

%token T_COMPONENT_X
%token T_COMPONENT_Y
%token T_COMPONENT_Z
%token T_COMPONENT_W

%token T_NOP

%token T_CONST_ACCROSS_WIDTH
%token T_CONST_ACCROSS_LENGTH
%token T_INTERPOLATION_DISABLE

%token T_FX10_LOW
%token T_FX10_HIGH
%token T_FP20

%token T_LINK

%token T_SYNTAX_ERROR

%type <u> TYPE
%type <u> COMPONENT

%type <component> COLUMN
%type <component> MODIFIERS
%type <component> MODIFIER

%type <instr> instruction

%union {
	unsigned u;

	struct {
		unsigned const_accross_width;
		unsigned const_accross_length;
		unsigned interpolation_disable;
		unsigned type;
	} component;

	link_instr instr;
}

%%

instructions: instructions instruction
	{
		if (asm_linker_instructions_nb == 32) {
			PARSE_ERROR("Too many instructions, 32 maximum");
		}

		asm_linker_instructions[asm_linker_instructions_nb++] = $2;
	}
	|
	{
		memset(asm_linker_instructions, 0, sizeof(asm_linker_instructions));
		asm_linker_instructions_nb = 0;
		asm_linker_used_tram_rows_nb = 1;

		linker_asmlineno = 1;
		linker_asmdebug = 0;
	}
	;

instruction:
	T_LINK COLUMN ',' COLUMN ',' COLUMN ',' COLUMN ','
		T_TRAM_ROW ',' T_EXPORT '.' COMPONENT COMPONENT COMPONENT COMPONENT
	{
		memset(&yyval.instr, 0, sizeof(yyval.instr));

		if ($10 > 31) {
			PARSE_ERROR("Invalid TRAM row index, 31 maximum");
		}

		if ($12 > 15) {
			PARSE_ERROR("Invalid vertex export index, 15 maximum");
		}

		yyval.instr.vec4_select			= 0;
		yyval.instr.vertex_export_index		= $12;
		yyval.instr.tram_row_index		= $10;

		yyval.instr.const_x_across_width	= $2.const_accross_width;
		yyval.instr.const_x_across_length	= $2.const_accross_length;
		yyval.instr.interpolation_disable_x	= $2.interpolation_disable;
		yyval.instr.tram_dst_type_x		= $2.type;
		yyval.instr.swizzle_x			= $14;

		yyval.instr.const_y_across_width	= $4.const_accross_width;
		yyval.instr.const_y_across_length	= $4.const_accross_length;
		yyval.instr.interpolation_disable_y	= $4.interpolation_disable;
		yyval.instr.tram_dst_type_y		= $4.type;
		yyval.instr.swizzle_y			= $15;

		yyval.instr.const_z_across_width	= $6.const_accross_width;
		yyval.instr.const_z_across_length	= $6.const_accross_length;
		yyval.instr.interpolation_disable_z	= $6.interpolation_disable;
		yyval.instr.tram_dst_type_z		= $6.type;
		yyval.instr.swizzle_z			= $16;

		yyval.instr.const_w_across_width	= $8.const_accross_width;
		yyval.instr.const_w_across_length	= $8.const_accross_length;
		yyval.instr.interpolation_disable_w	= $8.interpolation_disable;
		yyval.instr.tram_dst_type_w		= $8.type;
		yyval.instr.swizzle_w			= $17;

		if ($10 > asm_linker_used_tram_rows_nb) {
			asm_linker_used_tram_rows_nb = $10;
		}
	}
	;

COMPONENT:
	T_COMPONENT_X
	{
		yyval.u = LINK_SWIZZLE_X;
	}
	|
	T_COMPONENT_Y
	{
		yyval.u = LINK_SWIZZLE_Y;
	}
	|
	T_COMPONENT_Z
	{
		yyval.u = LINK_SWIZZLE_Z;
	}
	|
	T_COMPONENT_W
	{
		yyval.u = LINK_SWIZZLE_W;
	}
	;

COLUMN: TYPE MODIFIERS
	{
		yyval.component      = $2;
		yyval.component.type = $1;
	}
	;

TYPE:
	T_FX10_LOW
	{
		yyval.u = TRAM_DST_FX10_LOW;
	}
	|
	T_FX10_HIGH
	{
		yyval.u = TRAM_DST_FX10_HIGH;
	}
	|
	T_FP20
	{
		yyval.u = TRAM_DST_FP20;
	}
	|
	T_NOP
	{
		yyval.u = TRAM_DST_NONE;
	}
	;

MODIFIERS: MODIFIERS MODIFIER
	{
		yyval.component.const_accross_width |= $2.const_accross_width;
		yyval.component.const_accross_length |= $2.const_accross_length;
		yyval.component.interpolation_disable |= $2.interpolation_disable;
	}
	|
	{
		memset(&yyval.component, 0, sizeof(yyval.component));
	}
	;

MODIFIER:
	'(' T_CONST_ACCROSS_WIDTH ')'
	{
		memset(&yyval.component, 0, sizeof(yyval.component));
		yyval.component.const_accross_width = 1;
	}
	|
	'(' T_CONST_ACCROSS_LENGTH ')'
	{
		memset(&yyval.component, 0, sizeof(yyval.component));
		yyval.component.const_accross_length = 1;
	}
	|
	'(' T_INTERPOLATION_DISABLE ')'
	{
		memset(&yyval.component, 0, sizeof(yyval.component));
		yyval.component.interpolation_disable = 1;
	}
	;

%%
