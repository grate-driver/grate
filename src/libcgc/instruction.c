#include <stdio.h>

#include "cgdrv.h"

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
