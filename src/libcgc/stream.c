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

#include "host1x.h"

enum host1x_opcode {
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

void host1x_stream_init(struct host1x_stream *stream, const void *buffer,
			size_t size)
{
	stream->words = stream->ptr = buffer;
	stream->end = buffer + size;
}

void host1x_stream_interpret(struct host1x_stream *stream)
{
	while (stream->ptr < stream->end) {
		uint32_t opcode = (*stream->ptr >> 28) & 0xf;
		uint16_t offset, classid, count, mask, value, i;

		switch (opcode) {
		case HOST1X_OPCODE_SETCL:
			offset = (*stream->ptr >> 16) & 0xfff;
			classid = (*stream->ptr >> 6) & 0x3ff;
			mask = *stream->ptr & 0x3f;
			stream->ptr++;

			stream->classid = classid;

			for (i = 0; i < 16; i++)
				if (mask & BIT(i))
					stream->write_word(stream->user,
					                   stream->classid,
					                   offset + i,
					                   *stream->ptr++);

			break;

		case HOST1X_OPCODE_INCR:
			offset = (*stream->ptr >> 16) & 0xfff;
			count = *stream->ptr & 0xffff;
			stream->ptr++;

			for (i = 0; i < count; i++)
				stream->write_word(stream->user,
				                   stream->classid,
				                   offset + i,
				                   *stream->ptr++);

			break;

		case HOST1X_OPCODE_NONINCR:
			offset = (*stream->ptr >> 16) & 0xfff;
			count = *stream->ptr & 0xffff;
			stream->ptr++;

			for (i = 0; i < count; i++)
				stream->write_word(stream->user,
				                   stream->classid,
				                   offset,
				                   *stream->ptr++);

			break;

		case HOST1X_OPCODE_MASK:
			offset = (*stream->ptr >> 16) & 0xfff;
			mask = *stream->ptr & 0xffff;
			stream->ptr++;

			for (i = 0; i < 16; i++)
				if (mask & BIT(i))
					stream->write_word(stream->user,
					                   stream->classid,
					                   offset + i,
					                   *stream->ptr++);

			break;

		case HOST1X_OPCODE_IMM:
			offset = (*stream->ptr >> 16) & 0xfff;
			value = *stream->ptr & 0xffff;
			stream->ptr++;

			stream->write_word(stream->user,
			                   stream->classid,
			                   offset,
			                   value);

			break;

		case HOST1X_OPCODE_RESTART:
			stream->ptr++;
			fprintf(stderr, "HOST1X_OPCODE_RESTART\n");
			break;

		case HOST1X_OPCODE_GATHER:
			stream->ptr++;
			fprintf(stderr, "HOST1X_OPCODE_GATHER\n");
			break;

		case HOST1X_OPCODE_EXTEND:
			stream->ptr++;
			fprintf(stderr, "HOST1X_OPCODE_EXTEND\n");
			break;

		case HOST1X_OPCODE_CHDONE:
			stream->ptr++;
			fprintf(stderr, "HOST1X_OPCODE_CHDONE\n");
			break;

		default:
			fprintf(stderr, "UNKNOWN: 0x%08x\n", *stream->ptr++);
			break;
		}
	}
}
