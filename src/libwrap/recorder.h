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

#ifndef GRATE_RECORDER_H
#define GRATE_RECORDER_H 1

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "record_replay.h"

struct rec_ctx {
	unsigned int id;
};

struct bo_page {
	uint8_t data[4096];
};

struct rec_page_meta {
	unsigned long chksum;
};

struct bo_rec {
	struct rec_page_meta *page_meta;
	struct bo_page *page_data;
	struct rec_ctx *ctx;
	unsigned int num_pages;
	unsigned int id;
	bool captured;
	unsigned int width;
	unsigned int height;
	unsigned int pitch;
	uint32_t format;
	uint32_t flags;
	bool is_framebuffer;
	unsigned int fb_id;
};

struct job_ctx_rec {
	unsigned int id;
	bool gr2d;
};

struct job_rec {
	unsigned int num_syncpt_incrs;

	struct record_gather *gathers;
	unsigned int num_gathers;

	struct record_reloc *relocs;
	unsigned int num_relocs;

	struct job_ctx_rec *ctx;
};

#define REC_VER		0x0001

struct recorder {
	bool inited;
	bool enabled;

	FILE *fout;
	unsigned int ctx_cnt;
	unsigned int bos_cnt;
	unsigned int job_ctx_cnt;
	unsigned long zeroed_page_chksum;
};

bool recorder_enabled(void);
struct rec_ctx *record_create_ctx(void);
void record_destroy_ctx(struct rec_ctx *ctx);
struct bo_rec *record_create_bo(struct rec_ctx *ctx, size_t size,
				uint32_t flags);
void record_set_bo_data(struct bo_rec *bo, void *data);
void record_destroy_bo(struct bo_rec *bo);
void record_capture_bo_data(struct bo_rec *bo, bool force);
void record_set_bo_flags(struct bo_rec *bo, uint32_t flags);
void record_add_framebuffer(struct bo_rec *bo, uint32_t flags);
void record_del_framebuffer(struct bo_rec *bo);
struct job_ctx_rec *record_job_ctx_create(bool gr2d);
void record_job_ctx_destroy(struct job_ctx_rec *ctx);
void record_job_submit(struct job_rec *job);

#endif
