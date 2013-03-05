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

void host1x_stream_dump(struct host1x_stream *stream, FILE *fp)
{
	while (stream->ptr < stream->end) {
		uint32_t opcode = (*stream->ptr >> 28) & 0xf;
		uint16_t offset, count, mask, value, i;

		fprintf(fp, "    %08x: ", *stream->ptr);

		switch (opcode) {
		case HOST1X_OPCODE_SETCL:
			fprintf(fp, "HOST1X_OPCODE_SETCL\n");
			stream->ptr++;
			break;

		case HOST1X_OPCODE_INCR:
			offset = (*stream->ptr >> 16) & 0xfff;
			count = *stream->ptr & 0xffff;

			fprintf(fp, "HOST1X_OPCODE_INCR    0x%03x, 0x%04x\n",
				offset, count);

			stream->ptr++;

			for (i = 0; i < count; i++)
				fprintf(fp, "      0x%08x\n", *stream->ptr++);

			break;

		case HOST1X_OPCODE_NONINCR:
			offset = (*stream->ptr >> 16) & 0xfff;
			count = *stream->ptr & 0xffff;

			fprintf(fp, "HOST1X_OPCODE_NONINCR 0x%03x, 0x%04x\n",
				offset, count);

			stream->ptr++;

			switch (offset) {
			case 0x604: /* LUT */
#if 0
				dump_lut(stream, count);
#else
				for (i = 0; i < count; i++)
					fprintf(fp, "      0x%08x\n",
						*stream->ptr++);
#endif

				break;

			case 0x804: /* ALU */
#if 0
				dump_alu(stream, count);
#else
				for (i = 0; i < count; i++)
					fprintf(fp, "      0x%08x\n",
						*stream->ptr++);
#endif

				break;

			default:
				for (i = 0; i < count; i++)
					fprintf(fp, "      0x%08x\n",
						*stream->ptr++);

				break;
			}

			break;

		case HOST1X_OPCODE_MASK:
			offset = (*stream->ptr >> 16) & 0xfff;
			mask = *stream->ptr & 0xffff;

			fprintf(fp, "HOST1X_OPCODE_MASK    0x%03x 0x%04x\n",
				offset, mask);

			stream->ptr++;

			for (i = 0; i < 16; i++)
				if (mask & BIT(i))
					fprintf(fp, "      %08x: 0x%08x\n",
						offset + i, *stream->ptr++);

			break;

		case HOST1X_OPCODE_IMM:
			offset = (*stream->ptr >> 16) & 0xfff;
			value = *stream->ptr & 0xffff;

			fprintf(fp, "HOST1X_OPCODE_IMM     0x%03x, 0x%04x\n",
				offset, value);

			stream->ptr++;
			break;

		case HOST1X_OPCODE_RESTART:
			fprintf(fp, "HOST1X_OPCODE_RESTART\n");
			break;

		case HOST1X_OPCODE_GATHER:
			fprintf(fp, "HOST1X_OPCODE_GATHER\n");
			break;

		case HOST1X_OPCODE_EXTEND:
			fprintf(fp, "HOST1X_OPCODE_EXTEND\n");
			break;

		case HOST1X_OPCODE_CHDONE:
			fprintf(fp, "HOST1X_OPCODE_CHDONE\n");
			break;

		default:
			fprintf(fp, "UNKNOWN: 0x%08x\n", *stream->ptr++);
			break;
		}
	}
}
