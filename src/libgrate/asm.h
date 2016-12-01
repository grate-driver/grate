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

#ifndef GRATE_ASM_H
#define GRATE_ASM_H

#include "vpe_vliw.h"

struct yy_buffer_state;

extern struct yy_buffer_state *yy_scan_string(const char *);
extern int yyparse(void);

struct asm_vec_component {
	uint32_t value;
	int dirty;
};

typedef struct asm_vec4 {
	struct asm_vec_component x;
	struct asm_vec_component y;
	struct asm_vec_component z;
	struct asm_vec_component w;
} asm_vec4;

typedef struct asm_const {
	asm_vec4 vector;
	int used;
} asm_const;

typedef struct asm_in_out {
	char name[256];
	int used;
} asm_in_out;

extern vpe_instr128 asm_vs_instructions[256];
extern asm_const asm_vs_constants[256];
extern asm_in_out asm_vs_attributes[16];
extern asm_in_out asm_vs_exports[16];
extern int asm_vs_instructions_nb;

extern void reset_asm_parser_state(void);

char * vpe_vliw_disassemble(const vpe_instr128 *ins);

#endif
