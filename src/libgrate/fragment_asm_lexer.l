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

%option caseless

%{
#include <stdint.h>
#include "fragment_asm.h"
#include "fragment_asm.tab.h"

#define YY_NO_INPUT

extern void yyerror(char *);

int yywrap(void)
{
	return 1;
}
%}

%%
[ \t]+			/* skip */;
"/*"([^*]|(\*+[^*/]))*\*+\/ /* comment */;
[\n]			yylineno++;

".asm"			return T_ASM;
".constants"		return T_CONSTANTS;
".uniforms"		return T_UNIFORMS;
[-]{0,1}[0-9]+\.[0-9]+	{
				fragment_asmlval.f = atof(yytext);
				return T_FLOAT;
			}
0x[0-9a-f]{1,16}	{
				fragment_asmlval.u = strtoull(yytext + 2, NULL, 16);
				return T_HEX;
			}
EXEC			return T_EXEC;
neg			return T_NEG;
abs			return T_ABS;
sat			return T_SATURATE;
[0-9]+			{
				fragment_asmlval.u = atoi(yytext);
				return T_NUMBER;
			}
\"(.+?)\"		{
				if (yyleng > sizeof(fragment_asmlval.s) - 1)
					return T_SYNTAX_ERROR;

				strcpy(fragment_asmlval.s, yytext + 1);
				fragment_asmlval.s[yyleng - 2] = '\0';

				return T_STRING;
			}
","			return ',';
";"			return ';';
"."			return '.';
"("			return '(';
")"			return ')';
"["			return '[';
"]"			return ']';
"="			return '=';
"-"			return '-';
"*"			return '*';

PSEQ:			return T_PSEQ;
MFU:			return T_MFU;
TEX:			return T_TEX;
ALU0:			return T_ALU0;
ALU1:			return T_ALU1;
ALU2:			return T_ALU2;
ALU3:			return T_ALU3;
ALU_COMPLEMENT:		return T_ALU_COMPLEMENT;
DW:			return T_DW;

NOP			return T_OPCODE_NOP;

RCP			return T_MFU_OPCODE_RCP;
RSQ			return T_MFU_OPCODE_RSQ;
LG2			return T_MFU_OPCODE_LG2;
EX2			return T_MFU_OPCODE_EX2;
SQRT			return T_MFU_OPCODE_SQRT;
SIN			return T_MFU_OPCODE_SIN;
COS			return T_MFU_OPCODE_COS;
FRC			return T_MFU_OPCODE_FRC;
PREEX2			return T_MFU_OPCODE_PREEX2;
PRESIN			return T_MFU_OPCODE_PRESIN;
PRECOS			return T_MFU_OPCODE_PRECOS;

unk			return T_MFU_UNK;

t[0-9]{1,2}		{
				fragment_asmlval.u = atoi(yytext + 1);
				return T_TRAM_ROW;
			}

r[0-9]{1,2}		{
				fragment_asmlval.u = atoi(yytext + 1);
				return T_ROW_REGISTER;
			}
g[0-7]			{
				fragment_asmlval.u = atoi(yytext + 1);
				return T_GLOBAL_REGISTER;
			}
alu[0-3]		{
				fragment_asmlval.u = atoi(yytext + 3);
				return T_ALU_RESULT_REGISTER;
			}
imm[0-2]		{
				fragment_asmlval.u = atoi(yytext + 3);
				return T_IMMEDIATE;
			}
#[0-1]			{
				fragment_asmlval.u = atoi(yytext + 1);
				return T_CONST_0_1;
			}
u[0-9]{1,2}		{
				fragment_asmlval.u = atoi(yytext + 1);
				return T_ALU_UNIFORM;
			}
cr[0-9]{1,2}		{
				fragment_asmlval.u = atoi(yytext + 2);
				return T_ALU_CONDITION_REGISTER;
			}
lp			return T_ALU_LOWP;
posx			return T_POSITION_X;
posy			return T_POSITION_Y;
pface			return T_POLIGON_FACE;
kill			return T_KILL;

rB			return T_ALU_rB;
rC			return T_ALU_rC;

MAD			return T_ALU_OPCODE_MAD;
MUL			return T_ALU_OPCODE_MUL;
MIN			return T_ALU_OPCODE_MIN;
MAX			return T_ALU_OPCODE_MAX;
CSEL			return T_ALU_OPCODE_CSEL;

FX10			return T_FX10;
FP20			return T_FP20;

"l"			return T_LOW;
"h"			return T_HIGH;

this			return T_ALU_ACCUM_THIS;
other			return T_ALU_ACCUM_OTHER;
"eq"			return T_ALU_CC_EQ;
"gt"			return T_ALU_CC_GT;
"ge"			return T_ALU_CC_GE;
"x2"			return T_ALU_X2;
"x4"			return T_ALU_X4;
"/2"			return T_ALU_DIV2;

"alu_buffer_size"	return T_ALU_BUFFER_SIZE;
"pseq_to_dw_exec_nb"	return T_PSEQ_DW_EXEC_NB;

"ipl:"			return T_MFU_FETCH_INTERPOLATE;
"sfu:"			return T_MFU_SFU;
"mul0:"			return T_MFU_MUL0;
"mul1:"			return T_MFU_MUL1;

dst[0-9]{1,2}		{
				fragment_asmlval.u = atoi(yytext + 3);
				return T_MFU_MUL_DST;
			}
"bar"			{
				fragment_asmlval.u = 1;
				return T_MFU_MUL_DST_BARYCENTRIC;
			}

src[0-9]{1,2}		{
				fragment_asmlval.u = atoi(yytext + 3);
				return T_MUL_SRC;
			}
"sfu"			return T_MFU_MUL_SRC_SFU_RESULT;
"bar0"			return T_MFU_MUL_SRC_BARYCENTRIC_0;
"bar1"			return T_MFU_MUL_SRC_BARYCENTRIC_1;

.			return T_SYNTAX_ERROR;
%%
