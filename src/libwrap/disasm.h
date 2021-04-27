/*
 * Copyright (c) Dmitry Osipenko
 * Copyright (c) Erik Faye-Lund
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

#ifndef GRATE_DISASM_H
#define GRATE_DISASM_H 1

#include "asm.h"

struct disasm_state {
	union {
		vpe_instr128 vpe_instr[256];
		uint32_t vpe_words[1024];
	};
	unsigned int vpe_words_nb;

	union {
		link_instr lnk_instr[32];
		uint32_t linker_words[64];
	};
	unsigned int linker_inst_nb;

	pseq_instr pseq_instructions[64];
	unsigned int pseq_words_nb;

	instr_sched mfu_sched[64];
	unsigned int mfu_sched_words_nb;

	union {
		mfu_instr mfu_instructions[64];
		uint32_t mfu_words[128];

	};
	unsigned int mfu_words_nb;

	tex_instr tex_instructions[64];
	unsigned int tex_words_nb;

	union {
		union fragment_alu_instruction alu_instructions[256];
		uint32_t alu_words[512];
	};
	unsigned int alu_words_nb;

	uint32_t alu_complements[64];
	unsigned int alu_complements_nb;

	alu_instr_sched_t114 alu_sched[64];
	unsigned int alu_sched_words_nb;

	dw_instr dw_instructions[64];
	unsigned int dw_words_nb;
};

void disasm_reset(struct disasm_state *d);
void disasm_write_reg(void *opaque, uint32_t host1x_class,
		      uint32_t offset, uint32_t value);
void disasm_dump(struct disasm_state *d);

#endif
