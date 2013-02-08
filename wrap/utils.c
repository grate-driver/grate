#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

static LIST_HEAD(files);

void print_hexdump(FILE *fp, int prefix_type, const char *prefix,
		   const void *buffer, size_t size, size_t columns,
		   bool ascii)
{
	const unsigned char *ptr = buffer;
	size_t i, j;

	for (j = 0; j < size; j += columns) {
		const char *space = "";

		if (prefix)
			fputs(prefix, fp);

		switch (prefix_type) {
		case DUMP_PREFIX_NONE:
		default:
			break;

		case DUMP_PREFIX_OFFSET:
			fprintf(fp, "%08x: ", j);
			break;

		case DUMP_PREFIX_ADDRESS:
			fprintf(fp, "%p: ", buffer + j);
			break;
		}

		for (i = 0; (i < columns) && (j + i < size); i++) {
			fprintf(fp, "%s%02x", space, ptr[j + i]);
			space = " ";
		}

		for (i = i; i < columns; i++)
			fprintf(fp, "   ");

		fputs(" | ", fp);

		if (ascii) {
			for (i = 0; (i < columns) && (j + i < size); i++) {
				if (isprint(ptr[j + i]))
					fprintf(fp, "%c", ptr[j + i]);
				else
					fprintf(fp, ".");
			}
		}

		fputs("\n", fp);
	}
}

void file_add(struct file *file)
{
	if (file)
		list_add_tail(&file->list, &files);
}

struct file *file_lookup(int fd)
{
	struct file *file;

	list_for_each_entry(file, &files, list)
		if (file->fd == fd)
			return file;

	return NULL;
}

struct file *file_find(const char *path)
{
	struct file *file;

	list_for_each_entry(file, &files, list)
		if (strcmp(file->path, path) == 0)
			return file;

	return NULL;
}

void file_put(struct file *file)
{
	if (file && file->ops && file->ops->release)
		file->ops->release(file);
}

void file_close(int fd)
{
	struct file *file;

	printf("> %s(fd=%d)\n", __func__, fd);

	list_for_each_entry(file, &files, list) {
		if (file->fd == fd) {
			printf("closing %s\n", file->path);
			list_del(&file->list);
			file_put(file);
			break;
		}
	}

	printf("< %s()\n", __func__);
}
