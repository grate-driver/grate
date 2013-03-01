#ifndef GRATE_LIBCGC_PRIVATE_H
#define GRATE_LIBCGC_PRIVATE_H 1

#include <stdint.h>

struct CgDrv {
	unsigned int unknown00;
	const char *error;
	const char *log;
	size_t length;
	void *stream;
	unsigned int unknown14;
	void *binary;
	size_t binary_size;
	unsigned int unknown20;
	unsigned int unknown24;
	unsigned int unknown28;
	unsigned int unknown2c;
	unsigned int unknown30;
	unsigned int unknown34;
	unsigned int unknown38;
	unsigned int unknown3c;
	unsigned int unknown40;
	unsigned int unknown44;
	unsigned int unknown48;
	unsigned int unknown4c;
};

struct CgDrv *CgDrv_Create(void);
void CgDrv_Delete(struct CgDrv *cgdrv);
void CgDrv_CleanUp(struct CgDrv *cgdrv);
int CgDrv_Compile(struct CgDrv *cgdrv, int unknown, int type,
		  const char *code, size_t length, int unknown2);

struct instruction;

struct instruction *instruction_create_from_words(uint32_t *words,
						  unsigned int count);
void instruction_free(struct instruction *inst);
unsigned int instruction_get_bit(struct instruction *inst, unsigned int pos);
uint32_t instruction_extract(struct instruction *inst, unsigned int from,
			     unsigned int to);

#endif
