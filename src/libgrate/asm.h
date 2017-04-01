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

#include "fragment_asm.h"
#include "linker_asm.h"
#include "vpe_vliw.h"

struct yy_buffer_state;

extern struct yy_buffer_state *vertex_asm_scan_string(const char *);
extern int vertex_asmparse(void);
extern int vertex_asmlex_destroy(void);

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
	union {
		int used;
		int type;
	};
} asm_in_out;

extern vpe_instr128 asm_vs_instructions[256];
extern asm_const asm_vs_constants[256];
extern asm_in_out asm_vs_attributes[16];
extern asm_in_out asm_vs_exports[16];
extern asm_in_out asm_vs_uniforms[256];
extern int asm_vs_instructions_nb;

const char * vpe_vliw_disassemble(const vpe_instr128 *ins);

extern struct yy_buffer_state *fragment_asm_scan_string(const char *);
extern int fragment_asmparse(void);
extern int fragment_asmlex_destroy(void);

extern pseq_instr	asm_pseq_instructions[64];
extern mfu_instr	asm_mfu_instructions[64];
extern tex_instr	asm_tex_instructions[64];
extern alu_instr	asm_alu_instructions[64];
extern dw_instr		asm_dw_instructions[64];

extern instr_sched	asm_mfu_sched[64];
extern instr_sched	asm_alu_sched[64];

extern uint32_t		asm_fs_constants[32];

#define FS_UNIFORM_FX10_LOW	1
#define FS_UNIFORM_FX10_HIGH	2
#define FS_UNIFORM_FP20		3

extern asm_in_out	asm_fs_uniforms[32 * 2];

extern unsigned asm_fs_instructions_nb;
extern unsigned asm_mfu_instructions_nb;
extern unsigned asm_alu_instructions_nb;

extern unsigned asm_alu_buffer_size;
extern unsigned asm_pseq_to_dw_exec_nb;

const char * fragment_pipeline_disassemble(
	const pseq_instr *pseq,
	const mfu_instr *mfu, unsigned mfu_nb,
	const tex_instr *tex,
	const alu_instr *alu, unsigned alu_nb,
	const dw_instr *dw);

extern struct yy_buffer_state *linker_asm_scan_string(const char *);
extern int linker_asmparse(void);
extern int linker_asmlex_destroy(void);

extern link_instr	asm_linker_instructions[32];

extern unsigned asm_linker_instructions_nb;
extern unsigned asm_linker_used_tram_rows_nb;

const char * linker_instruction_disassemble(const link_instr *instr);

#endif
