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

%option caseless

%{
#include <stdint.h>
#include "vertex_asm.tab.h"

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
".exports"		return T_EXPORTS;
".constants"		return T_CONSTANTS;
".attributes"		return T_ATTRIBUTES;
[-]{0,1}[0-9]+"."[0-9]+ {
				vertex_asmlval.f = atof(yytext);
				return T_FLOAT;
			}
0x[0-9a-f]{1,8}		{
				vertex_asmlval.u = strtoul(yytext + 2, NULL, 16);
				return T_HEX;
			}
EXEC			return T_EXEC;
EXEC_END		return T_EXEC_END;
A0			return T_ADDRESS_REG;
r[0-9]{1,2}		{
				vertex_asmlval.u = atoi(yytext + 1);
				return T_REGISTER;
			}
a			return T_ATTRIBUTE;
c			return T_CONSTANT;
u			{
				vertex_asmlval.u = atoi(yytext + 1);
				return T_UNDEFINED;
			}
neg			return T_NEG;
abs			return T_ABS;
export			return T_EXPORT;
vector			return T_VECTOR;
scalar			return T_SCALAR;
saturate		return T_SATURATE;
[0-9]+			{
				vertex_asmlval.u = atoi(yytext);
				return T_NUMBER;
			}
p			return T_PREDICATE;
cs			return T_SET_CONDITION;
eq			return T_CHECK_CONDITION_EQ;
lt			return T_CHECK_CONDITION_LT;
gt			return T_CHECK_CONDITION_GT;
cc			return T_CHECK_CONDITION_CHECK;
cwr			return T_CHECK_CONDITION_WR;
cr			return T_CONDITION_REGISTER;
x			{
				vertex_asmlval.c = yytext[0];
				return T_COMPONENT_X;
			}
y			{
				vertex_asmlval.c = yytext[0];
				return T_COMPONENT_Y;
			}
z			{
				vertex_asmlval.c = yytext[0];
				return T_COMPONENT_Z;
			}
w			{
				vertex_asmlval.c = yytext[0];
				return T_COMPONENT_W;
			}
"*"			{
				vertex_asmlval.c = yytext[0];
				return T_COMPONENT_DISABLED;
			}
\"(.+?)\"		{
				if (yyleng > sizeof(vertex_asmlval.s) - 1)
					return T_SYNTAX_ERROR;

				strcpy(vertex_asmlval.s, yytext + 1);
				vertex_asmlval.s[yyleng - 2] = '\0';

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
"+"			return '+';

NOPv			return T_VECTOR_OPCODE_NOP;
MOVv			return T_VECTOR_OPCODE_MOV;
MULv			return T_VECTOR_OPCODE_MUL;
ADDv			return T_VECTOR_OPCODE_ADD;
MADv			return T_VECTOR_OPCODE_MAD;
DP3v			return T_VECTOR_OPCODE_DP3;
DPHv			return T_VECTOR_OPCODE_DPH;
DP4v			return T_VECTOR_OPCODE_DP4;
DSTv			return T_VECTOR_OPCODE_DST;
MINv			return T_VECTOR_OPCODE_MIN;
MAXv			return T_VECTOR_OPCODE_MAX;
SLTv			return T_VECTOR_OPCODE_SLT;
SGEv			return T_VECTOR_OPCODE_SGE;
ARLv			return T_VECTOR_OPCODE_ARL;
FRCv			return T_VECTOR_OPCODE_FRC;
FLRv			return T_VECTOR_OPCODE_FLR;
SEQv			return T_VECTOR_OPCODE_SEQ;
SFLv			return T_VECTOR_OPCODE_SFL;
SGTv			return T_VECTOR_OPCODE_SGT;
SLEv			return T_VECTOR_OPCODE_SLE;
SNEv			return T_VECTOR_OPCODE_SNE;
STRv			return T_VECTOR_OPCODE_STR;
SSGv			return T_VECTOR_OPCODE_SSG;
ARRv			return T_VECTOR_OPCODE_ARR;
ARAv			return T_VECTOR_OPCODE_ARA;
TXLv			return T_VECTOR_OPCODE_TXL;
PUSHAv			return T_VECTOR_OPCODE_PUSHA;
POPAv			return T_VECTOR_OPCODE_POPA;

NOPs			return T_SCALAR_OPCODE_NOP;
MOVs			return T_SCALAR_OPCODE_MOV;
RCPs			return T_SCALAR_OPCODE_RCP;
RCCs			return T_SCALAR_OPCODE_RCC;
RSQs			return T_SCALAR_OPCODE_RSQ;
EXPs			return T_SCALAR_OPCODE_EXP;
LOGs			return T_SCALAR_OPCODE_LOG;
LITs			return T_SCALAR_OPCODE_LIT;
BRAs			return T_SCALAR_OPCODE_BRA;
CALs			return T_SCALAR_OPCODE_CAL;
RETs			return T_SCALAR_OPCODE_RET;
LG2s			return T_SCALAR_OPCODE_LG2;
EX2s			return T_SCALAR_OPCODE_EX2;
SINs			return T_SCALAR_OPCODE_SIN;
COSs			return T_SCALAR_OPCODE_COS;
PUSHAs			return T_SCALAR_OPCODE_PUSHA;
POPAs			return T_SCALAR_OPCODE_POPA;

bit120			return T_BIT120;

.			return T_SYNTAX_ERROR;
%%
