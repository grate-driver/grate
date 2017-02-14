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
	void *unknown20;
	unsigned int unknown24;
	unsigned int unknown28;
	unsigned int unknown2c;
	void *unknown30;
	unsigned int unknown34;
	unsigned int unknown38;
	unsigned int unknown3c;
	void *unknown40;
	unsigned int unknown44;
	unsigned int unknown48;
	unsigned int unknown4c;
	unsigned int unknown50;
	unsigned int unknown54;
};

struct CgDrv *CgDrv_Create(void);
void CgDrv_Delete(struct CgDrv *cgdrv);
void CgDrv_CleanUp(struct CgDrv *cgdrv);
int CgDrv_Compile(struct CgDrv *cgdrv, int unknown, int type,
		  const char *code, size_t length, int unknown2, int unknown3);

struct instruction;

struct instruction *instruction_create_from_words(uint32_t *words,
						  unsigned int count);
void instruction_free(struct instruction *inst);
void instruction_print_raw(struct instruction *inst);
void instruction_print_unknown(struct instruction *inst);
unsigned int instruction_get_bit(struct instruction *inst, unsigned int pos);
uint32_t instruction_extract(struct instruction *inst, unsigned int from,
			     unsigned int to);
void instruction_set_bit(struct instruction *inst, unsigned int pos,
			 unsigned int value);
void instruction_insert(struct instruction *inst, unsigned int from,
			unsigned int to, uint32_t value);

#endif
