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

#include <stdio.h>
#include <string.h>

#include "tgr_3d.xml.h"
#include "disasm.h"
#include "utils.h"

static void disasm_reset_vp(struct disasm_state *d)
{
	d->vpe_words_nb = 0;
}

static void disasm_reset_linker(struct disasm_state *d)
{
	d->linker_inst_nb = 0;
}

static void disasm_reset_fp(struct disasm_state *d)
{
	d->pseq_words_nb = 0;
	d->mfu_sched_words_nb = 0;
	d->mfu_words_nb = 0;
	d->tex_words_nb = 0;
	d->alu_complements_nb = 0;
	d->alu_sched_words_nb = 0;
	d->alu_words_nb = 0;
	d->dw_words_nb = 0;
}

void disasm_reset(struct disasm_state *d)
{
	disasm_reset_vp(d);
	disasm_reset_linker(d);
	disasm_reset_fp(d);
}

void disasm_write_reg(void *opaque, uint32_t host1x_class,
		      uint32_t offset, uint32_t value)
{
	struct disasm_state *d = opaque;

	if (!libwrap_verbose)
		return;

	if (host1x_class != 0x60)
		return;

	switch (offset) {
	case TGR3D_VP_UPLOAD_INST:
		if (d->vpe_words_nb == 1024) {
			fprintf(stderr, "%s: ERROR: over 256 vertex instructions\n",
				__func__);
			break;
		}

		d->vpe_words[d->vpe_words_nb++] = value;
		break;

	case TGR3D_LINKER_INSTRUCTION(0) ... TGR3D_LINKER_INSTRUCTION(31):
		d->linker_words[offset & 0x3f] = value;
		break;

	case TGR3D_CULL_FACE_LINKER_SETUP:
		value &= TGR3D_CULL_FACE_LINKER_SETUP_LINKER_INST_COUNT__MASK;
		value >>= TGR3D_CULL_FACE_LINKER_SETUP_LINKER_INST_COUNT__SHIFT;

		d->linker_inst_nb = value + 1;
		break;

	case TGR3D_FP_PSEQ_UPLOAD_INST:
		if (d->pseq_words_nb == 64) {
			fprintf(stderr, "%s: ERROR: over 64 PSEQ instructions\n",
				__func__);
			break;
		}

		d->pseq_instructions[d->pseq_words_nb++].data = value;
		break;

	case TGR3D_FP_UPLOAD_MFU_SCHED:
		if (d->mfu_sched_words_nb == 64) {
			fprintf(stderr, "%s: ERROR: over 64 MFU sched words\n",
				__func__);
			break;
		}

		d->mfu_sched[d->mfu_sched_words_nb++].data = value;
		break;

	case TGR3D_FP_UPLOAD_MFU_INST:
		if (d->mfu_words_nb == 128) {
			fprintf(stderr, "%s: ERROR: over 64 MFU instructions\n",
				__func__);
			break;
		}

		d->mfu_words[d->mfu_words_nb++] = value;
		break;

	case TGR3D_FP_UPLOAD_TEX_INST:
		if (d->tex_words_nb == 64) {
			fprintf(stderr, "%s: ERROR: over 64 TEX instructions\n",
				__func__);
			break;
		}

		d->tex_instructions[d->tex_words_nb++].data = value;
		break;

	case TGR3D_FP_UPLOAD_ALU_SCHED:
		if (d->alu_sched_words_nb == 64) {
			fprintf(stderr, "%s: ERROR: over 64 ALU sched words\n",
				__func__);
			break;
		}

		if (tegra_chip_id() != TEGRA114)
			value = to_114_alu_sched(value).data;

		d->alu_sched[d->alu_sched_words_nb++].data = value;
		break;

	case TGR3D_FP_UPLOAD_ALU_INST:
		if (d->alu_words_nb == 512) {
			fprintf(stderr, "%s: ERROR: over 64 ALU instructions\n",
				__func__);
			break;
		}

		d->alu_words[d->alu_words_nb++] = value;
		break;

	case TGR3D_FP_UPLOAD_ALU_INST_COMPLEMENT:
		if (d->alu_complements_nb == 64) {
			fprintf(stderr, "%s: ERROR: over 64 ALU complement words\n",
				__func__);
			break;
		}

		d->alu_complements[d->alu_complements_nb++] = value;
		break;

	case TGR3D_FP_UPLOAD_DW_INST:
		if (d->dw_words_nb == 64) {
			fprintf(stderr, "%s: ERROR: over 64 DW instructions\n",
				__func__);
			break;
		}

		d->dw_instructions[d->dw_words_nb++].data = value;
		break;

	case TGR3D_DRAW_PRIMITIVES:
		disasm_dump(d);
		break;

	case TGR3D_VP_UPLOAD_INST_ID:
		disasm_reset_vp(d);
		break;

	case TGR3D_FP_UPLOAD_INST_ID_COMMON:
		disasm_reset_fp(d);
		break;

	default:
		break;
	}
}

void disasm_dump(struct disasm_state *d)
{
	unsigned int i, k;
	unsigned int addr;

	printf("=======================\n");
	printf("Vertex instructions: %u\n", d->vpe_words_nb / 4);
	printf("\n");

	if (d->vpe_words_nb & 3)
		fprintf(stderr, "%s: ERROR: unaligned number of vertex instruction words\n",
			__func__);

	for (i = 0; i < d->vpe_words_nb / 4; i++) {
		vpe_instr128 instr;

		instr.part3 = d->vpe_instr[i].part0;
		instr.part2 = d->vpe_instr[i].part1;
		instr.part1 = d->vpe_instr[i].part2;
		instr.part0 = d->vpe_instr[i].part3;

		printf("%s\n", vpe_vliw_disassemble(&instr));
	}
	printf("\n");

	printf("=======================\n");
	printf("Linker instructions: %u\n", d->linker_inst_nb);
	printf("\n");

	for (i = 0; i < d->linker_inst_nb; i++)
		printf("%s\n", linker_instruction_disassemble(&d->lnk_instr[i]));
	printf("\n");

	printf("=======================\n");
	printf("PSEQ instructions: %u\n", d->pseq_words_nb);
	printf("MFU instructions: %u\n", d->mfu_sched_words_nb);
	printf("TEX instructions: %u\n", d->tex_words_nb);
	printf("ALU instructions: %u\n", d->alu_sched_words_nb);
	printf("DW instructions: %u\n", d->dw_words_nb);

	if (d->pseq_words_nb != d->mfu_sched_words_nb ||
	    d->pseq_words_nb != d->tex_words_nb ||
	    d->pseq_words_nb != d->alu_sched_words_nb ||
	    d->pseq_words_nb != d->dw_words_nb)
		fprintf(stderr, "%s: ERROR: invalid number of FP instructions\n",
			__func__);

	if (d->mfu_words_nb & 1)
		fprintf(stderr, "%s: ERROR: unaligned number of MFU instruction words\n",
			__func__);

	if (d->alu_words_nb & 7)
		fprintf(stderr, "%s: ERROR: unaligned number of ALU instruction words\n",
			__func__);

	printf("\n");

	for (i = 0; i < d->pseq_words_nb; i++) {
		pseq_instr *pseq = &d->pseq_instructions[i];
		tex_instr  *tex  = &d->tex_instructions[i];
		dw_instr   *dw   = &d->dw_instructions[i];
		mfu_instr  mfu[3];
		alu_instr  alu[3 + 5];

		addr = d->mfu_sched[i].address;
		for (k = 0; k < d->mfu_sched[i].instructions_nb; k++, addr++) {
			mfu[k].part0 = d->mfu_instructions[addr].part1;
			mfu[k].part1 = d->mfu_instructions[addr].part0;
		}

		addr = d->alu_sched[i].address;
		for (k = 0; k < d->alu_sched[i].instructions_nb; k++, addr++) {
			alu[k].a[0].part0 = d->alu_instructions[addr * 4 + 0].part1;
			alu[k].a[0].part1 = d->alu_instructions[addr * 4 + 0].part0;
			alu[k].a[1].part0 = d->alu_instructions[addr * 4 + 1].part1;
			alu[k].a[1].part1 = d->alu_instructions[addr * 4 + 1].part0;
			alu[k].a[2].part0 = d->alu_instructions[addr * 4 + 2].part1;
			alu[k].a[2].part1 = d->alu_instructions[addr * 4 + 2].part0;
			alu[k].a[3].part0 = d->alu_instructions[addr * 4 + 3].part1;
			alu[k].a[3].part1 = d->alu_instructions[addr * 4 + 3].part0;
			alu[k].complement = d->alu_complements[addr];
		}

		printf("%s\n", fragment_pipeline_disassemble(
					pseq,
					mfu, d->mfu_sched[i].instructions_nb,
					tex,
					alu, d->alu_sched[i].instructions_nb,
					dw));
	}
	printf("\n");
}
