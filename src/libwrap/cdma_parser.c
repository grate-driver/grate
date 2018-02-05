/*
 * Copyright (c) 2012, 2013 Erik Faye-Lund
 * Copyright (c) 2013 Avionic Design GmbH
 * Copyright (c) 2013 Thierry Reding
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

#ifdef ENABLE_RNN
#include <envytools/rnn.h>
#include <envytools/rnndec.h>
#endif

#include "cdma_parser.h"

enum cdma_opcode {
	HOST1X_OPCODE_SETCL,
	HOST1X_OPCODE_INCR,
	HOST1X_OPCODE_NONINCR,
	HOST1X_OPCODE_MASK,
	HOST1X_OPCODE_IMM,
	HOST1X_OPCODE_RESTART,
	HOST1X_OPCODE_GATHER,
	HOST1X_OPCODE_EXTEND = 14,
	HOST1X_OPCODE_CHDONE = 15,
};

#if 0
static const char *cdma_opcode_names[] = {
	"HOST1X_OPCODE_SETCL",
	"HOST1X_OPCODE_INCR",
	"HOST1X_OPCODE_NONINCR",
	"HOST1X_OPCODE_MASK",
	"HOST1X_OPCODE_IMM",
	"HOST1X_OPCODE_RESTART",
	"HOST1X_OPCODE_GATHER",
	"HOST1X_OPCODE_UNKNOWN_7",
	"HOST1X_OPCODE_UNKNOWN_8",
	"HOST1X_OPCODE_UNKNOWN_9",
	"HOST1X_OPCODE_UNKNOWN_10",
	"HOST1X_OPCODE_UNKNOWN_11",
	"HOST1X_OPCODE_UNKNOWN_12",
	"HOST1X_OPCODE_UNKNOWN_13",
	"HOST1X_OPCODE_EXTEND",
	"HOST1X_OPCODE_CHDONE",
};
#endif

#define BIT(x) (1 << (x))

struct cdma_stream {
	uint32_t num_words;
	uint32_t position;
	uint32_t *words;
};

static enum cdma_opcode cdma_stream_get_opcode(struct cdma_stream *stream)
{
	return (stream->words[stream->position] >> 28) & 0xf;
}

#ifdef ENABLE_RNN

static void cdma_dump_register_write(uint32_t offset, uint32_t value)
{
	struct rnndecaddrinfo *info;
	static struct rnndeccontext *vc;
	static struct rnndb *db;
	static struct rnndomain *tgr3d_dom;
	static int rnn_inited;
	if (!rnn_inited) {
		int use_colors = getenv("LIBWRAP_NO_COLORS") == NULL;
		rnn_init();

		db = rnn_newdb();
		rnn_parsefile(db, "tgr_3d.xml");
		rnn_prepdb(db);
		vc = rnndec_newcontext(db);
		vc->colors = use_colors ? &envy_def_colors : &envy_null_colors;

		tgr3d_dom = rnn_finddomain(db, "TGR3D");
		if (!tgr3d_dom)
			fprintf(stderr, "Could not find domain\n");

		rnn_inited = 1;
	}

	if (tgr3d_dom) {
		info = rnndec_decodeaddr(vc, tgr3d_dom, offset, 0);
		if (info && info->typeinfo)
			printf("        %s <= %s\n", info->name,
			    rnndec_decodeval(vc, info->typeinfo, value, info->width));
		else if (info)
			printf("        %s <= 0x%08x\n", info->name, value);
		else
			printf("        %03x <= 0x%08x\n", offset, value);
	} else
		printf("        %03x <= 0x%08x\n", offset, value);
}

#else

static void cdma_dump_register_write(uint32_t offset, uint32_t value)
{
	printf("        %03x <= 0x%08x\n", offset, value);
}

#endif

static void cdma_opcode_setcl_dump(struct cdma_stream *stream)
{
	uint32_t offset, classid, mask, i;

	offset = (stream->words[stream->position] >> 16) & 0xfff;
	classid = (stream->words[stream->position] >> 6) & 0x3ff;
	mask =stream->words[stream->position] & 0x3f;

	printf("      HOST1X_OPCODE_SETCL: offset:%x classid:%x mask:%x\n",
	       offset, classid, mask);

	for (i = 0; i < 6; i++) {
		if (mask & BIT(i)) {
			uint32_t value = stream->words[++stream->position];
			cdma_dump_register_write(offset + i, value);
		}
	}

	stream->position++;
}

static void cdma_opcode_incr_dump(struct cdma_stream *stream)
{
	uint32_t offset, count, i;

	offset = (stream->words[stream->position] >> 16) & 0xfff;
	count = stream->words[stream->position] & 0xffff;
	stream->position++;

	printf("      HOST1X_OPCODE_INCR: offset:%x count:%x\n", offset, count);

	for (i = 0; i < count; i++)
		cdma_dump_register_write(offset + i, stream->words[stream->position++]);
}

static void cdma_opcode_nonincr_dump(struct cdma_stream *stream)
{
	uint32_t offset, count, i;

	offset = (stream->words[stream->position] >> 16) & 0xfff;
	count = stream->words[stream->position] & 0xffff;
	stream->position++;

	printf("      HOST1X_OPCODE_NONINCR: offset:%x count:%x\n", offset,
	       count);

	for (i = 0; i < count; i++)
		cdma_dump_register_write(offset, stream->words[stream->position++]);
}

static void cdma_opcode_mask_dump(struct cdma_stream *stream)
{
	uint32_t offset, mask, i;

	offset = (stream->words[stream->position] >> 16) & 0xfff;
	mask = stream->words[stream->position] & 0xffff;
	stream->position++;

	printf("      HOST1X_OPCODE_MASK: offset:%x mask:%x\n", offset, mask);

	for (i = 0; i < 16; i++)
		if (mask & BIT(i))
			cdma_dump_register_write(offset + i,
			       stream->words[stream->position++]);
}

static void cdma_opcode_imm_dump(struct cdma_stream *stream)
{
	uint32_t offset, value;

	offset = (stream->words[stream->position] >> 16) & 0xfff;
	value = stream->words[stream->position] & 0xffff;
	stream->position++;

	printf("      HOST1X_OPCODE_IMM: offset:%x value:%x\n", offset, value);
	cdma_dump_register_write(offset, value);
}

static void cdma_opcode_restart_dump(struct cdma_stream *stream)
{
	uint32_t offset;

	offset = (stream->words[stream->position] & 0x0fffffff) << 4;
	stream->position++;

	printf("      HOST1X_OPCODE_RESTART: offset:%x\n", offset);
}

static void cdma_opcode_gather_dump(struct cdma_stream *stream)
{
	uint32_t offset, count;
	const char *insert;

	offset = (stream->words[stream->position] >> 16) & 0xfff;
	count = stream->words[stream->position] & 0x3fff;

	if (stream->words[stream->position] & BIT(15)) {
		if (stream->words[stream->position] & BIT(14))
			insert = "INCR ";
		else
			insert = "NONINCR ";
	} else {
		insert = "";
	}
	stream->position++;

	printf("      HOST1X_OPCODE_GATHER: offset:%x %scount:%x base:%x\n",
	       offset, insert, count, stream->words[++stream->position]);
}

static void cdma_opcode_extend_dump(struct cdma_stream *stream)
{
	uint16_t subop;
	uint32_t value;

	subop = (stream->words[stream->position] >> 24) & 0xf;
	value = stream->words[stream->position] & 0xffffff;

	printf("      HOST1X_OPCODE_EXTEND: subop:%x value:%x\n", subop, value);

	stream->position++;
}

static void cdma_opcode_chdone_dump(struct cdma_stream *stream)
{
	printf("      HOST1X_OPCODE_CHDONE\n");
	stream->position++;
}

void cdma_dump_commands(uint32_t *commands, unsigned int count)
{
	struct cdma_stream stream = {
		.words = commands,
		.num_words = count,
		.position = 0,
	};

	printf("    commands: %u\n", count);

	while (stream.position < stream.num_words) {
		enum cdma_opcode opcode = cdma_stream_get_opcode(&stream);

		switch (opcode) {
		case HOST1X_OPCODE_SETCL:
			cdma_opcode_setcl_dump(&stream);
			break;

		case HOST1X_OPCODE_INCR:
			cdma_opcode_incr_dump(&stream);
			break;

		case HOST1X_OPCODE_NONINCR:
			cdma_opcode_nonincr_dump(&stream);
			break;

		case HOST1X_OPCODE_MASK:
			cdma_opcode_mask_dump(&stream);
			break;

		case HOST1X_OPCODE_IMM:
			cdma_opcode_imm_dump(&stream);
			break;

		case HOST1X_OPCODE_RESTART:
			cdma_opcode_restart_dump(&stream);
			break;

		case HOST1X_OPCODE_GATHER:
			cdma_opcode_gather_dump(&stream);
			break;

		case HOST1X_OPCODE_EXTEND:
			cdma_opcode_extend_dump(&stream);
			break;

		case HOST1X_OPCODE_CHDONE:
			cdma_opcode_chdone_dump(&stream);
			break;

		default:
			printf("\n");
			break;
		}
	}
}
