#ifndef GRATE_LIBCGDRV_H
#define GRATE_LIBCGDRV_H 1

struct instruction;

struct instruction *instruction_create_from_words(uint32_t *words,
						  unsigned int count);
void instruction_free(struct instruction *inst);
unsigned int instruction_get_bit(struct instruction *inst, unsigned int pos);
uint32_t instruction_extract(struct instruction *inst, unsigned int from,
			     unsigned int to);

#endif
