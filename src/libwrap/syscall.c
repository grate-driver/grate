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

#include <dlfcn.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <sys/ioctl.h>

#include "nvhost.h"
#include "utils.h"
#include "list.h"

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

int open(const char *pathname, int flags, ...)
{
	static typeof(open) *orig = NULL;
	static bool initialized = false;
	int ret;

	if (!initialized) {
		nvhost_register();
		initialized = true;
	}

	printf("%s(pathname=%s, flags=%x)\n", __func__, pathname, flags);

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

	printf("%s() = %d\n", __func__, ret);
	return ret;
}

int close(int fd)
{
	static typeof(close) *orig = NULL;
	int ret;

	if (!orig)
		orig = dlsym_helper(__func__);

	printf("%s(fd=%d)\n", __func__, fd);

	ret = orig(fd);
	file_close(fd);

	printf("%s() = %d\n", __func__, ret);
	return ret;
}

ssize_t read(int fd, void *buffer, size_t size)
{
	static typeof(read) *orig = NULL;
	ssize_t ret;

	if (!orig)
		orig = dlsym_helper(__func__);

	printf("%s(fd=%d, buffer=%p, size=%zu)\n", __func__, fd, buffer, size);

	ret = orig(fd, buffer, size);

	printf("%s() = %zd\n", __func__, ret);
	return ret;
}

ssize_t write(int fd, const void *buffer, size_t size)
{
	static typeof(write) *orig = NULL;
	struct file *file;
	ssize_t ret;

	if (!orig)
		orig = dlsym_helper(__func__);

	printf("%s(fd=%d, buffer=%p, size=%zu)\n", __func__, fd, buffer, size);

	print_hexdump(stdout, DUMP_PREFIX_OFFSET, "  ", buffer, size, 16,
		      true);

	file = file_lookup(fd);

	if (file && file->ops && file->ops->write)
		file->ops->write(file, buffer, size);

	ret = orig(fd, buffer, size);

	printf("%s() = %zd\n", __func__, ret);
	return ret;
}

int ioctl(int fd, unsigned long request, ...)
{
	static typeof(ioctl) *orig = NULL;
	const struct ioctl *ioc = NULL;
	int size = _IOC_SIZE(request);
	struct file *file;
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

	if (size) {
		va_list ap;
		void *arg;

		va_start(ap, request);
		arg = va_arg(ap, void *);
		va_end(ap);

		printf("%s(fd=%d, request=%#lx, arg=%p)\n", __func__, fd, request, arg);

		if (file && file->ops && file->ops->enter_ioctl)
			file->ops->enter_ioctl(file, request, arg);

		ret = orig(fd, request, arg);

		if (file && file->ops && file->ops->leave_ioctl)
			file->ops->leave_ioctl(file, request, arg);
	} else {
		printf("%s(fd=%d, request=%#lx)\n", __func__, fd, request);

		if (file && file->ops && file->ops->enter_ioctl)
			file->ops->enter_ioctl(file, request, NULL);

		ret = orig(fd, request);

		if (file && file->ops && file->ops->leave_ioctl)
			file->ops->leave_ioctl(file, request, NULL);
	}

	if (!ioc) {
		printf("  dir:%lx type:'%c' nr:%lx size:%lu\n",
		       _IOC_DIR(request), (char)_IOC_TYPE(request),
		       _IOC_NR(request), _IOC_SIZE(request));
	} else {
		printf("  %s\n", ioc->name);
	}

	printf("%s() = %d\n", __func__, ret);
	return ret;
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	static typeof(mmap) *orig = NULL;
	void *ret;

	if (!orig)
		orig = dlsym_helper(__func__);

	printf("%s(addr=%p, length=%zu, prot=%#x, flags=%#x, fd=%d, offset=%lu)\n",
	       __func__, addr, length, prot, flags, fd, offset);

	ret = orig(addr, length, prot, flags, fd, offset);

	printf("%s() = %p\n", __func__, ret);
	return ret;
}

int munmap(void *addr, size_t length)
{
	static typeof(munmap) *orig = NULL;
	int ret;

	if (!orig)
		orig = dlsym_helper(__func__);

	printf("%s(addr=%p, length=%zu)\n", __func__, addr, length);

	ret = orig(addr, length);

	printf("%s() = %d\n", __func__, ret);
	return ret;
}
