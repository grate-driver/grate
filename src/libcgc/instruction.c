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
#include <stdlib.h>

#include "libcgc-private.h"

struct instruction {
	unsigned int length;
	uint32_t *bits;
};

struct instruction *instruction_create_from_words(uint32_t *words,
						  unsigned int count)
{
	struct instruction *inst;
	unsigned int i;

	inst = calloc(1, sizeof(*inst));
	if (!inst)
		return NULL;

	inst->length = count * 32;

	inst->bits = malloc(sizeof(*words) * count);
	if (!inst->bits) {
		free(inst);
		return NULL;
	}

	for (i = 0; i < count; i++)
		inst->bits[i] = words[count - i - 1];

	return inst;
}

void instruction_free(struct instruction *inst)
{
	if (inst)
		free(inst->bits);

	free(inst);
}

unsigned int instruction_get_bit(struct instruction *inst, unsigned int pos)
{
	unsigned int word = pos / 32;
	unsigned int bit = pos % 32;

	if (pos >= inst->length) {
		fprintf(stderr, "WARNING: bit out of range: %u\n", pos);
		return 0;
	}

	return (inst->bits[word] & (1 << bit)) != 0;
}

uint32_t instruction_extract(struct instruction *inst, unsigned int from,
			     unsigned int to)
{
	unsigned int bits = to - from, i;
	uint32_t value = 0;

	if (to < from || bits > 32) {
		fprintf(stderr, "WARNING: invalid bitfield: %u-%u\n", from,
			to);
		return 0;
	}

	for (i = from; i <= to; i++) {
		unsigned int word = i / 32;
		unsigned int bit = i % 32;

		if (inst->bits[word] & (1 << bit))
			value |= 1 << (i - from);
	}

	return value;
}

void instruction_set_bit(struct instruction *inst, unsigned int pos, int value)
{
	unsigned int word = pos / 32;
	unsigned int bit = pos % 32;

	if (pos >= inst->length)
		fprintf(stderr, "WARNING: bit out of range: %u\n", pos);

	inst->bits[word] &= ~(1 << bit);
	inst->bits[word] |= (value & 1) << bit;
}

void instruction_insert(struct instruction *inst, unsigned int from,
			unsigned int to, uint32_t value)
{
	unsigned int bits = to - from, i;

	if (to < from || bits > 32)
		fprintf(stderr, "WARNING: invalid bitfield: %u-%u\n", from,
			to);

	for (i = from; i <= to; i++) {
		unsigned int word = i / 32;
		unsigned int bit = i % 32;

		inst->bits[word] &= ~(1 << bit);
		if (value & (1 << (i - from)))
			inst->bits[word] |= (1 << bit);
	}
}
