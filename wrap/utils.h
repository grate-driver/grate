#ifndef GRATE_UTILS_H
#define GRATE_UTILS_H 1

#include <stdbool.h>
#include <stdio.h>

#include "list.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

struct ioctl {
	unsigned long request;
	const char *name;
};

#define IOCTL(request) \
	{ request, #request }

enum {
	DUMP_PREFIX_NONE,
	DUMP_PREFIX_OFFSET,
	DUMP_PREFIX_ADDRESS,
};

void print_hexdump(FILE *fp, int prefix_type, const char *prefix,
		   const void *buffer, size_t size, size_t columns,
		   bool ascii);

struct file_ops;

struct file {
	struct list_head list;
	char *path;
	int fd;

	const struct ioctl *ioctls;
	unsigned int num_ioctls;

	const struct file_ops *ops;
};

struct file_ops {
	int (*enter_ioctl)(struct file *file, unsigned long request, void *arg);
	int (*leave_ioctl)(struct file *file, unsigned long request, void *arg);
	ssize_t (*write)(struct file *file, const void *buffer, size_t size);
	ssize_t (*read)(struct file *file, void *buffer, size_t size);
	void (*release)(struct file *file);
};

void file_add(struct file *file);
struct file *file_lookup(int fd);
struct file *file_find(const char *path);
void file_put(struct file *file);
void file_close(int fd);

#endif
