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

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

extern void *__libc_dlsym(void *handle, const char *name);

static void *dlopen_helper(const char *name)
{
	void *ret;

	ret = dlopen(name, RTLD_LAZY);
	if (!ret)
		return NULL;

	return ret;
}

static void *dlsym_helper(const char *name)
{
	static void *cgdrv = NULL;

	if (!cgdrv)
		cgdrv = dlopen_helper("libcgdrv.so");

	return __libc_dlsym(cgdrv, name);
}

#if 0
static void print_pointer(FILE *fp, void *ptr)
{
	unsigned long value = (unsigned long)ptr;
	unsigned int i, nibbles, shift;

	nibbles = sizeof(value) * 2;
	shift = (nibbles - 1) * 4;

	for (i = 0; i < nibbles; i++) {
		unsigned char c = (value >> shift) & 0xf;

		if (c >= 0 && c <= 9)
			fputc('0' + c, fp);
		else
			fputc('a' + c, fp);

		value <<= 4;
	}
}

void *malloc(size_t size)
{
	static typeof(malloc) *orig = NULL;
	void *ret;

	if (!orig)
		orig = __libc_dlsym(RTLD_NEXT, __func__);

	printf("%s(size=%zu)\n", __func__, size);

	ret = orig(size);

	printf("%s() = %p\n", __func__, ret);
	return ret;
}

void free(void *ptr)
{
	static typeof(free) *orig = NULL;

	fputs("free(ptr=0x", stdout);
	print_pointer(stdout, ptr);
	fputs(")\n", stdout);

	if (!orig)
		orig = __libc_dlsym(RTLD_NEXT, __func__);

	orig(ptr);

	fputs("free()\n", stdout);
}
#endif

struct cgdrv {
	unsigned int unknown00;
	const char *error;
	const char *log;
	unsigned int unknown0c;
	unsigned int unknown10;
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

struct cgdrv *CgDrv_Create(void)
{
	static typeof(CgDrv_Create) *orig = NULL;
	struct cgdrv *cgdrv;

	printf("%s()\n", __func__);

	if (!orig)
		orig = dlsym_helper(__func__);

	cgdrv = orig();

	printf("%s() = %p\n", __func__, cgdrv);

	return cgdrv;
}

void CgDrv_Delete(struct cgdrv *cgdrv)
{
	static typeof(CgDrv_Delete) *orig = NULL;

	printf("%s(cgdrv=%p)\n", __func__, cgdrv);

	if (!orig)
		orig = dlsym_helper(__func__);

	orig(cgdrv);

	printf("%s()\n", __func__);
}

void CgDrv_CleanUp(struct cgdrv *cgdrv)
{
	static typeof(CgDrv_CleanUp) *orig = NULL;

	printf("%s(cgdrv=%p)\n", __func__, cgdrv);

	if (!orig)
		orig = dlsym_helper(__func__);

	orig(cgdrv);

	printf("%s()\n", __func__);
}

void CgDrv_NumInstructions(struct cgdrv *cgdrv)
{
	static typeof(CgDrv_NumInstructions) *orig = NULL;

	printf("%s(cgdrv=%p)\n", __func__, cgdrv);

	if (!orig)
		orig = dlsym_helper(__func__);

	orig(cgdrv);

	printf("%s()\n", __func__);
}

void CgDrv_GetCgBin(struct cgdrv *cgdrv)
{
	static typeof(CgDrv_GetCgBin) *orig = NULL;

	printf("%s(cgdrv=%p)\n", __func__, cgdrv);

	if (!orig)
		orig = dlsym_helper(__func__);

	orig(cgdrv);

	printf("%s()\n", __func__);
}

int CgDrv_Compile(struct cgdrv *cgdrv, int unknown, int type,
		  const char *code, size_t length, int unknown2)
{
	static typeof(CgDrv_Compile) *orig = NULL;
	size_t i;
	int ret;

	printf("%s(cgdrv=%p, unknown=%d, type=%d, code=%p, length=%zu, unknown2=%d)\n",
	       __func__, cgdrv, unknown, type, code, length, unknown2);

	fputs("  | ", stdout);

	for (i = 0; i < length; i++) {
		putc(code[i], stdout);

		if (code[i] == '\n')
			fputs("  | ", stdout);
	}

	fputs("\n", stdout);

	if (!orig)
		orig = dlsym_helper(__func__);

	ret = orig(cgdrv, unknown, type, code, length, unknown2);

	if (cgdrv->error)
		printf("  error: %s\n", cgdrv->error);

	printf("  %s\n", cgdrv->log);

	printf("  binary: %zu bytes\n", cgdrv->binary_size);
	print_hexdump(stdout, DUMP_PREFIX_OFFSET, "    ", cgdrv->binary,
		      cgdrv->binary_size, 16, true);

	printf("%s() = %d\n", __func__, ret);

	return ret;
}

void *dlsym(void *handle, const char *name)
{
	if (strcmp(name, "CgDrv_Create") == 0)
		return CgDrv_Create;

	if (strcmp(name, "CgDrv_CleanUp") == 0)
		return CgDrv_CleanUp;

	if (strcmp(name, "CgDrv_NumInstructions") == 0)
		return CgDrv_NumInstructions;

	if (strcmp(name, "CgDrv_GetCgBin") == 0)
		return CgDrv_GetCgBin;

	if (strcmp(name, "CgDrv_Compile") == 0)
		return CgDrv_Compile;

	return __libc_dlsym(handle, name);
}
