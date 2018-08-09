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

#ifndef GRATE_UTILS_H
#define GRATE_UTILS_H 1

#include <stdbool.h>
#include <stdio.h>

#include "list.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define PRINTF(...)				\
	do {					\
		if (libwrap_verbose)		\
			printf(__VA_ARGS__);	\
	} while (0)

extern bool libwrap_verbose;

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
	int dup_fds[8];

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

struct file *file_open(const char *path, int fd);
struct file *file_lookup(int fd);
struct file *file_find(const char *path);
void file_close(int fd);
void file_dup(struct file *file, int fd);

struct file_table {
	const char *path;
	struct file *(*open)(const char *path, int fd);
};

void file_table_register(const struct file_table *table, unsigned int count);

#endif
