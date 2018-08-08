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

extern void *_dl_sym(void *, const char *, void *);
extern void *__libc_malloc(size_t size);
extern void *__libc_realloc(void *ptr, size_t size);
extern void *__libc_free(void *ptr);

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

	return _dl_sym(cgdrv, name, dlsym);
}

#if 0
void *malloc(size_t size)
{
	void *ret;

	PRINTF("%s(size=%zu)\n", __func__, size);

	ret = __libc_malloc(size);

	PRINTF("%s() = %p\n", __func__, ret);
	return ret;
}

void *realloc(void *ptr, size_t size)
{
	void *ret;

	PRINTF("%s(ptr=%p, size=%zu)\n", __func__, ptr, size);

	ret = __libc_realloc(ptr, size);

	PRINTF("%s() = %p\n", __func__, ret);
	return ret;
}

void free(void *ptr)
{
	PRINTF("%s(ptr=%p)\n", __func__, ptr);

	__libc_free(ptr);

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

	PRINTF("%s()\n", __func__);

	if (!orig)
		orig = dlsym_helper(__func__);

	cgdrv = orig();

	PRINTF("%s() = %p\n", __func__, cgdrv);

	return cgdrv;
}

void CgDrv_Delete(struct cgdrv *cgdrv)
{
	static typeof(CgDrv_Delete) *orig = NULL;

	PRINTF("%s(cgdrv=%p)\n", __func__, cgdrv);

	if (!orig)
		orig = dlsym_helper(__func__);

	orig(cgdrv);

	PRINTF("%s()\n", __func__);
}

void CgDrv_CleanUp(struct cgdrv *cgdrv)
{
	static typeof(CgDrv_CleanUp) *orig = NULL;

	PRINTF("%s(cgdrv=%p)\n", __func__, cgdrv);

	if (!orig)
		orig = dlsym_helper(__func__);

	orig(cgdrv);

	PRINTF("%s()\n", __func__);
}

void CgDrv_NumInstructions(struct cgdrv *cgdrv)
{
	static typeof(CgDrv_NumInstructions) *orig = NULL;

	PRINTF("%s(cgdrv=%p)\n", __func__, cgdrv);

	if (!orig)
		orig = dlsym_helper(__func__);

	orig(cgdrv);

	PRINTF("%s()\n", __func__);
}

void CgDrv_GetCgBin(struct cgdrv *cgdrv)
{
	static typeof(CgDrv_GetCgBin) *orig = NULL;

	PRINTF("%s(cgdrv=%p)\n", __func__, cgdrv);

	if (!orig)
		orig = dlsym_helper(__func__);

	orig(cgdrv);

	PRINTF("%s()\n", __func__);
}

int CgDrv_Compile(struct cgdrv *cgdrv, int unknown, int type,
		  const char *code, size_t length, int unknown2, int unknown3)
{
	static typeof(CgDrv_Compile) *orig = NULL;
	size_t i;
	int ret;

	PRINTF("%s(cgdrv=%p, unknown=%d, type=%d, code=%p, length=%zu, unknown2=%d)\n",
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

	ret = orig(cgdrv, unknown, type, code, length, unknown2, unknown3);

	if (cgdrv->error)
		PRINTF("  error: %s\n", cgdrv->error);

	PRINTF("  %s\n", cgdrv->log);

	PRINTF("  binary: %zu bytes\n", cgdrv->binary_size);
	print_hexdump(stdout, DUMP_PREFIX_OFFSET, "    ", cgdrv->binary,
		      cgdrv->binary_size, 16, true);

	PRINTF("%s() = %d\n", __func__, ret);

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

	return _dl_sym(handle, name, dlsym);
}
