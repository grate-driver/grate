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

#define _LARGEFILE64_SOURCE

#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/ioctl.h>

#include "host1x.h"
#include "nvhost.h"
#include "syscall.h"
#include "utils.h"
#include "list.h"

static pthread_mutex_t ioctl_lock = PTHREAD_MUTEX_INITIALIZER;
bool libwrap_verbose = true;

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
	static void *libc = NULL;

	if (!libc)
		libc = dlopen_helper("libc.so.6");

	return dlsym(libc, name);
}

static void init_verbosity(void)
{
	const char *str = getenv("LIBWRAP_SILENT");

	if (str && strcmp(str, "1") == 0)
		libwrap_verbose = false;
}

int open(const char *pathname, int flags, ...)
{
	static typeof(open) *orig = NULL;
	static bool initialized = false;
	int ret;

	if (!initialized) {
		init_verbosity();
		nvhost_register();
		host1x_register();
		initialized = true;
	}

	PRINTF("%s(pathname=%s, flags=%x)\n", __func__, pathname, flags);

	if (!orig)
		orig = dlsym_helper(__func__);

	if (flags & O_CREAT) {
		mode_t mode;
		va_list ap;

		va_start(ap, flags);
		mode = (mode_t)va_arg(ap, int);
		va_end(ap);

		ret = orig(pathname, flags, mode);
	} else {
		ret = orig(pathname, flags);
	}

	if (ret >= 0)
		file_open(pathname, ret);

	PRINTF("%s() = %d\n", __func__, ret);
	return ret;
}

int open64(const char *pathname, int flags, ...)
{
	va_list argp;
	int ret;

	va_start(argp, flags);
	ret = open(pathname, flags | O_LARGEFILE, argp);
	va_end(argp);

	return ret;
}

int close(int fd)
{
	static typeof(close) *orig = NULL;
	int ret;

	if (!orig)
		orig = dlsym_helper(__func__);

	PRINTF("%s(fd=%d)\n", __func__, fd);

	ret = orig(fd);
	file_close(fd);

	PRINTF("%s() = %d\n", __func__, ret);
	return ret;
}

ssize_t read(int fd, void *buffer, size_t size)
{
	static typeof(read) *orig = NULL;
	ssize_t ret;

	if (!orig)
		orig = dlsym_helper(__func__);

	PRINTF("%s(fd=%d, buffer=%p, size=%zu)\n", __func__, fd, buffer, size);

	ret = orig(fd, buffer, size);

	PRINTF("%s() = %zd\n", __func__, ret);
	return ret;
}

ssize_t write(int fd, const void *buffer, size_t size)
{
	static typeof(write) *orig = NULL;
	struct file *file;
	ssize_t ret;

	if (!orig)
		orig = dlsym_helper(__func__);

	PRINTF("%s(fd=%d, buffer=%p, size=%zu)\n", __func__, fd, buffer, size);

	print_hexdump(stdout, DUMP_PREFIX_OFFSET, "  ", buffer, size, 16,
		      true);

	file = file_lookup(fd);

	if (file && file->ops && file->ops->write)
		file->ops->write(file, buffer, size);

	ret = orig(fd, buffer, size);

	PRINTF("%s() = %zd\n", __func__, ret);
	return ret;
}

static long timespec_diff_in_us(struct timespec *t1, struct timespec *t2)
{
	struct timespec diff;
	if (t2->tv_nsec-t1->tv_nsec < 0) {
		diff.tv_sec  = t2->tv_sec - t1->tv_sec - 1;
		diff.tv_nsec = t2->tv_nsec - t1->tv_nsec + 1000000000;
	} else {
		diff.tv_sec  = t2->tv_sec - t1->tv_sec;
		diff.tv_nsec = t2->tv_nsec - t1->tv_nsec;
	}
	return (diff.tv_sec * 1000000.0 + diff.tv_nsec / 1000);
}

int ioctl(int fd, unsigned long request, ...)
{
	static typeof(ioctl) *orig = NULL;
	const struct ioctl *ioc = NULL;
	struct timespec t1, t2;
	struct file *file;
	va_list ap;
	long diff;
	void *arg;
	int ret;

	if (!orig)
		orig = dlsym_helper(__func__);

	file = file_lookup(fd);
	if (file) {
		unsigned int i;

		for (i = 0; i < file->num_ioctls; i++) {
			if (file->ioctls[i].request == request) {
				ioc = &file->ioctls[i];
				break;
			}
		}
	}

	va_start(ap, request);
	arg = va_arg(ap, void *);
	va_end(ap);

	PRINTF("%s(fd=%d, request=%#lx, arg=%p)\n", __func__, fd, request, arg);

	if (file && file->ops && file->ops->enter_ioctl) {
		pthread_mutex_lock(&ioctl_lock);
		file->ops->enter_ioctl(file, request, arg);
		pthread_mutex_unlock(&ioctl_lock);

		clock_gettime(CLOCK_MONOTONIC, &t1);
	}

	ret = orig(fd, request, arg);

	if (file && file->ops && file->ops->leave_ioctl) {
		clock_gettime(CLOCK_MONOTONIC, &t2);

		pthread_mutex_lock(&ioctl_lock);
		file->ops->leave_ioctl(file, request, arg);
		pthread_mutex_unlock(&ioctl_lock);
	}

	if (!ioc) {
		PRINTF("  dir:%lx type:'%c' nr:%lx size:%lu\n",
		       _IOC_DIR(request), (char)_IOC_TYPE(request),
		       _IOC_NR(request), _IOC_SIZE(request));
	} else {
		diff = timespec_diff_in_us(&t1, &t2);
		PRINTF("  %s (%'ld us)\n", ioc->name, diff);
	}

	PRINTF("%s() = %d\n", __func__, ret);
	return ret;
}

void *mmap_orig(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	static typeof(mmap_orig) *orig = NULL;

	if (!orig)
		orig = dlsym_helper("mmap");

	return orig(addr, length, prot, flags, fd, offset);
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	void *ret;

	PRINTF("%s(addr=%p, length=%zu, prot=%#x, flags=%#x, fd=%d, offset=%lu)\n",
	       __func__, addr, length, prot, flags, fd, offset);

	ret = mmap_orig(addr, length, prot, flags, fd, offset);

	PRINTF("%s() = %p\n", __func__, ret);
	return ret;
}

int munmap_orig(void *addr, size_t length)
{
	static typeof(munmap_orig) *orig = NULL;

	if (!orig)
		orig = dlsym_helper("munmap");

	return orig(addr, length);
}

int munmap(void *addr, size_t length)
{
	int ret;

	PRINTF("%s(addr=%p, length=%zu)\n", __func__, addr, length);

	ret = munmap_orig(addr, length);

	PRINTF("%s() = %d\n", __func__, ret);
	return ret;
}
