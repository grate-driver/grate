#include <dlfcn.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/ioctl.h>

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
	int ret;

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
	ssize_t ret;

	if (!orig)
		orig = dlsym_helper(__func__);

	printf("%s(fd=%d, buffer=%p, size=%zu)\n", __func__, fd, buffer, size);

	ret = orig(fd, buffer, size);

	printf("%s() = %zd\n", __func__, ret);
	return ret;
}

int ioctl(int fd, unsigned long request, ...)
{
	static typeof(ioctl) *orig = NULL;
	int size = _IOC_SIZE(request);
	int ret;

	if (!orig)
		orig = dlsym_helper(__func__);

	if (size) {
		va_list ap;
		void *arg;

		va_start(ap, request);
		arg = va_arg(ap, void *);
		va_end(ap);

		printf("%s(fd=%d, request=%#lx, arg=%p)\n", __func__, fd, request, arg);
		ret = orig(fd, request, arg);
	} else {
		printf("%s(fd=%d, request=%#lx)\n", __func__, fd, request);
		ret = orig(fd, request);
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
