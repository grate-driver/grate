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
#include "linker_asm.h"
#include "linker_asm.tab.h"

#define YY_NO_INPUT

extern void yyerror(char *);

int yywrap(void)
{
	return 1;
}
%}

%%
[ \t]+				/* skip */;
"/*"([^*]|(\*+[^*/]))*\*+\/ 	/* multi line comment */;
"//".*				/* single line comment */;
[\n]				yylineno++;
"."				return '.';
"("				return '(';
")"				return ')';
","				return ',';

export[0-9]{1,2}	{
				linker_asmlval.u = atoi(yytext + 6);
				return T_EXPORT;
			}
tram[0-9]{1,2}		{
				linker_asmlval.u = atoi(yytext + 4);
				return T_TRAM_ROW;
			}

link			return T_LINK;

x			return T_COMPONENT_X;
y			return T_COMPONENT_Y;
z			return T_COMPONENT_Z;
w			return T_COMPONENT_W;

"nop"			return T_NOP;

"cw"			return T_CONST_ACCROSS_WIDTH;
"cl"			return T_CONST_ACCROSS_LENGTH;
"dis"			return T_INTERPOLATION_DISABLE;

"fx10.l"		return T_FX10_LOW;
"fx10.h"		return T_FX10_HIGH;

fp20			return T_FP20;

.			return T_SYNTAX_ERROR;
%%
