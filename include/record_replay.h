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

#ifndef GRATE_RECORD_REPLAY_H
#define GRATE_RECORD_REPLAY_H 1

#include <stdint.h>

#define REC_MAGIC	"grateREC"

enum record_action {
	REC_START,
	REC_INFO,
	REC_CTX_CREATE,
	REC_CTX_DESTROY,
	REC_BO_CREATE,
	REC_BO_DESTROY,
	REC_BO_LOAD_DATA,
	REC_BO_SET_FLAGS,
	REC_ADD_FRAMEBUFFER,
	REC_DEL_FRAMEBUFFER,
	REC_DISP_FRAMEBUFFER,
	REC_JOB_CTX_CREATE,
	REC_JOB_CTX_DESTROY,
	REC_JOB_SUBMIT,
};

struct __attribute__((packed)) record_gather {
	uint16_t id;
	uint16_t ctx_id;
	uint32_t offset;
	uint16_t num_words;
};

struct __attribute__((packed)) record_reloc {
	uint16_t id;
	uint16_t ctx_id;
	uint16_t gather_id;
	uint32_t offset;
	uint32_t patch_offset;
};

enum record_compression {
	REC_UNCOMPRESSED,
	REC_ZLIB,
	REC_LZ4,
};

struct __attribute__((packed)) record_act {
	uint32_t act;

	union {
		struct header {
			char magic[8];
			uint16_t version;
			uint16_t reserved[32];
		} header;

		struct record_info {
			uint16_t drm;
			uint16_t compression;
			uint16_t reserved[32];
		} record_info;

		struct ctx_create {
			uint16_t id;
		} ctx_create;

		struct ctx_destroy {
			uint16_t id;
		} ctx_destroy;

		struct bo_create {
			uint16_t id;
			uint16_t ctx_id;
			uint32_t num_pages;
			uint32_t flags;
		} bo_create;

		struct bo_destroy {
			uint16_t id;
			uint16_t ctx_id;
		} bo_destroy;

		struct bo_load {
			uint16_t id;
			uint16_t ctx_id;
			uint32_t page_id;
			uint16_t data_size;
		} bo_load;

		struct bo_set_flags {
			uint16_t id;
			uint16_t ctx_id;
			uint32_t flags;
		} bo_set_flags;

		struct add_framebuffer {
			uint16_t bo_id;
			uint16_t ctx_id;
			uint16_t width;
			uint16_t height;
			uint16_t pitch;
			uint32_t format;
			uint32_t flags;
		} add_framebuffer;

		struct del_framebuffer {
			uint16_t bo_id;
			uint16_t ctx_id;
		} del_framebuffer;

		struct disp_framebuffer {
			uint16_t bo_id;
			uint16_t ctx_id;
		} disp_framebuffer;

		struct job_ctx_create {
			uint16_t id;
			uint16_t gr2d;
		} job_ctx_create;

		struct job_ctx_destroy {
			uint16_t id;
		} job_ctx_destroy;

		struct job_submit {
			uint16_t job_ctx_id;
			uint16_t num_gathers;
			uint16_t num_relocs;
			uint16_t num_syncpt_incrs;
		} job_submit;
	} data;
};

#endif
