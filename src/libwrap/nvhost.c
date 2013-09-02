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

#include <stdlib.h>
#include <string.h>

#include "nvhost.h"

struct nvmap_file {
	struct list_head handles;
	struct file file;
};

static inline struct nvmap_file *to_nvmap_file(struct file *file)
{
	return container_of(file, struct nvmap_file, file);
}

static const struct ioctl nvmap_ioctls[] = {
	IOCTL(NVMAP_IOCTL_CREATE),
	IOCTL(NVMAP_IOCTL_CLAIM),
	IOCTL(NVMAP_IOCTL_FROM_ID),
	IOCTL(NVMAP_IOCTL_ALLOC),
	IOCTL(NVMAP_IOCTL_FREE),
	IOCTL(NVMAP_IOCTL_MMAP),
	IOCTL(NVMAP_IOCTL_WRITE),
	IOCTL(NVMAP_IOCTL_READ),
	IOCTL(NVMAP_IOCTL_PARAM),
	IOCTL(NVMAP_IOCTL_PIN),
	IOCTL(NVMAP_IOCTL_UNPIN),
	IOCTL(NVMAP_IOCTL_CACHE),
	IOCTL(NVMAP_IOCTL_GET_ID),
};

struct nvmap_handle {
	unsigned long id;
	void *mapped;
	void *buffer;
	size_t size;

	struct list_head list;
};

static struct nvmap_handle *nvmap_handle_new(unsigned long id, size_t size)
{
	struct nvmap_handle *handle;

	handle = calloc(1, sizeof(*handle));
	if (!handle)
		return NULL;

	INIT_LIST_HEAD(&handle->list);
	handle->size = size;
	handle->id = id;

	handle->buffer = calloc(1, size);
	if (!handle->buffer) {
		free(handle);
		return NULL;
	}

	return handle;
}

#if 0
static void nvmap_handle_free(struct nvmap_handle *handle)
{
	free(handle->buffer);
	free(handle);
}
#endif

static struct nvmap_handle *nvmap_file_lookup_handle(struct nvmap_file *nvmap,
						     unsigned long id)
{
	struct nvmap_handle *handle;

	list_for_each_entry(handle, &nvmap->handles, list)
		if (handle->id == id)
			return handle;

	return NULL;
}

static void nvmap_file_enter_ioctl_create(struct nvmap_file *nvmap,
					  struct nvmap_create_handle *args)
{
	printf("  Creating handle:\n");
	printf("    size: %u\n", args->size);
	printf("    handle: %x\n", args->handle);
}

static void nvmap_file_enter_ioctl_alloc(struct nvmap_file *nvmap,
					 struct nvmap_alloc_handle *args)
{
	printf("  Allocating handle:\n");
	printf("    handle: %x\n", args->handle);
	printf("    heap_mask: %x\n", args->heap_mask);
	printf("    flags: %x\n", args->flags);
	printf("    align: %x\n", args->align);
}

static void nvmap_file_enter_ioctl_mmap(struct nvmap_file *nvmap,
					struct nvmap_map_caller *args)
{
	struct nvmap_handle *handle;

	printf("  Mapping handle:\n");
	printf("    handle: %x\n", args->handle);
	printf("    offset: %x\n", args->offset);
	printf("    length: %u\n", args->length);
	printf("    flags: %x\n", args->flags);
	printf("    address: %x\n", args->addr);

	handle = nvmap_file_lookup_handle(nvmap, args->handle);
	if (!handle) {
		fprintf(stderr, "invalid handle: %x\n", args->handle);
		return;
	}

	handle->mapped = (void *)(uintptr_t)args->addr;
}

static void nvmap_file_enter_ioctl_write(struct nvmap_file *nvmap,
					 struct nvmap_rw_handle *op)
{
	struct nvmap_handle *handle;
	void *src, *dest;
	unsigned int i;

	printf("  Write operation:\n");
	printf("    address: %x\n", op->addr);
	printf("    handle: %x\n", op->handle);
	printf("    offset: %x\n", op->offset);
	printf("    elem_size: %u\n", op->elem_size);
	printf("    hmem_stride: %u\n", op->hmem_stride);
	printf("    user_stride: %u\n", op->user_stride);
	printf("    count: %u\n", op->count);

	handle = nvmap_file_lookup_handle(nvmap, op->handle);
	if (!handle) {
		fprintf(stderr, "invalid handle: %x\n", op->handle);
		return;
	}

	dest = handle->buffer + op->offset;
	src = (void *)(uintptr_t)op->addr;

	for (i = 0; i < op->count; i++) {
		print_hexdump(stdout, DUMP_PREFIX_NONE, "  ", src,
			      op->elem_size, 16, true);

		memcpy(dest, src, op->elem_size);

		dest += op->hmem_stride;
		src += op->user_stride;
	}
}

static void nvmap_file_enter_ioctl_param(struct nvmap_file *nvmap,
					 struct nvmap_handle_param *args)
{
	printf("  Parameter:\n");
	printf("    handle: %x\n", args->handle);
	printf("    param: %x\n", args->param);
}

static void nvmap_file_enter_ioctl_pin(struct nvmap_file *nvmap,
				       struct nvmap_pin_handle *args)
{
	printf("  Pinning handle:\n");
	printf("    handles: %x\n", args->handles);
	printf("    addr: %x\n", args->addr);
	printf("    count: %u\n", args->count);
}

static const char *cache_op_names[] = {
	"NVMAP_CACHE_OP_WB",
	"NVMAP_CACHE_OP_INV",
	"NVMAP_CACHE_OP_WB_INV",
};

static void nvmap_file_enter_ioctl_cache(struct nvmap_file *nvmap,
					 struct nvmap_cache_op *args)
{
	printf("  Cache maintenance:\n");
	printf("    address: %x\n", args->addr);
	printf("    handle: %x\n", args->handle);
	printf("    length: %u\n", args->length);
	printf("    op: %x (%s)\n", args->op, cache_op_names[args->op]);
}

static int nvmap_file_enter_ioctl(struct file *file, unsigned long request,
				  void *arg)
{
	struct nvmap_file *nvmap = to_nvmap_file(file);

	switch (request) {
	case NVMAP_IOCTL_CREATE:
		nvmap_file_enter_ioctl_create(nvmap, arg);
		break;

	case NVMAP_IOCTL_ALLOC:
		nvmap_file_enter_ioctl_alloc(nvmap, arg);
		break;

	case NVMAP_IOCTL_MMAP:
		nvmap_file_enter_ioctl_mmap(nvmap, arg);
		break;

	case NVMAP_IOCTL_WRITE:
		nvmap_file_enter_ioctl_write(nvmap, arg);
		break;

	case NVMAP_IOCTL_PARAM:
		nvmap_file_enter_ioctl_param(nvmap, arg);
		break;

	case NVMAP_IOCTL_PIN:
		nvmap_file_enter_ioctl_pin(nvmap, arg);
		break;

	case NVMAP_IOCTL_CACHE:
		nvmap_file_enter_ioctl_cache(nvmap, arg);
		break;
	}

	return 0;
}

static void nvmap_file_leave_ioctl_create(struct nvmap_file *nvmap,
					  struct nvmap_create_handle *args)
{
	struct nvmap_handle *handle;

	printf("  Handle created:\n");
	printf("    size: %u\n", args->size);
	printf("    handle: %x\n", args->handle);

	handle = nvmap_handle_new(args->handle, args->size);
	if (!handle) {
		fprintf(stderr, "failed to create handle\n");
		return;
	}

	list_add_tail(&handle->list, &nvmap->handles);
}

static void nvmap_file_leave_ioctl_alloc(struct nvmap_file *nvmap,
					 struct nvmap_alloc_handle *args)
{
	printf("  Handle allocated:\n");
	printf("    handle: %x\n", args->handle);
	printf("    heap_mask: %x\n", args->heap_mask);
	printf("    flags: %x\n", args->flags);
	printf("    align: %x\n", args->align);
}

static void nvmap_file_leave_ioctl_mmap(struct nvmap_file *nvmap,
					struct nvmap_map_caller *args)
{
	printf("  Handle mapped:\n");
	printf("    handle: %x\n", args->handle);
	printf("    offset: %x\n", args->offset);
	printf("    length: %u\n", args->length);
	printf("    flags: %x\n", args->flags);
	printf("    address: %x\n", args->addr);
}

static void nvmap_file_leave_ioctl_param(struct nvmap_file *nvmap,
					 struct nvmap_handle_param *args)
{
	printf("  Parameter obtained:\n");
	printf("    result: %u\n", args->result);
}

static void nvmap_file_leave_ioctl_pin(struct nvmap_file *nvmap,
				       struct nvmap_pin_handle *args)
{
	printf("  Handle pinned:\n");
	printf("    handles: %x\n", args->handles);
	printf("    addr: %x\n", args->addr);
	printf("    count: %u\n", args->count);
}

static void nvmap_file_leave_ioctl_cache(struct nvmap_file *nvmap,
					 struct nvmap_cache_op *args)
{
	void *virt = (void *)(uintptr_t)args->addr;

	printf("  Maintenance complete:\n");
	printf("    address: %x\n", args->addr);
	printf("    handle: %x\n", args->handle);
	printf("    length: %u\n", args->length);
	printf("    op: %x (%s)\n", args->op, cache_op_names[args->op]);

	print_hexdump(stdout, DUMP_PREFIX_OFFSET, "    ", virt, args->length,
		      16, true);
}

static int nvmap_file_leave_ioctl(struct file *file, unsigned long request,
				  void *arg)
{
	struct nvmap_file *nvmap = to_nvmap_file(file);

	switch (request) {
	case NVMAP_IOCTL_CREATE:
		nvmap_file_leave_ioctl_create(nvmap, arg);
		break;

	case NVMAP_IOCTL_ALLOC:
		nvmap_file_leave_ioctl_alloc(nvmap, arg);
		break;

	case NVMAP_IOCTL_MMAP:
		nvmap_file_leave_ioctl_mmap(nvmap, arg);
		break;

	case NVMAP_IOCTL_PARAM:
		nvmap_file_leave_ioctl_param(nvmap, arg);
		break;

	case NVMAP_IOCTL_PIN:
		nvmap_file_leave_ioctl_pin(nvmap, arg);
		break;

	case NVMAP_IOCTL_CACHE:
		nvmap_file_leave_ioctl_cache(nvmap, arg);
		break;
	}

	return 0;
}

static void nvmap_file_release(struct file *file)
{
	struct nvmap_file *nvmap = to_nvmap_file(file);

	free(file->path);
	free(nvmap);
}

static const struct file_ops nvmap_file_ops = {
	.enter_ioctl = nvmap_file_enter_ioctl,
	.leave_ioctl = nvmap_file_leave_ioctl,
	.release = nvmap_file_release,
};

struct file *nvmap_file_new(const char *path, int fd)
{
	struct nvmap_file *nvmap;

	nvmap = calloc(1, sizeof(*nvmap));
	if (!nvmap)
		return NULL;

	INIT_LIST_HEAD(&nvmap->file.list);
	nvmap->file.path = strdup(path);
	nvmap->file.fd = fd;

	nvmap->file.num_ioctls = ARRAY_SIZE(nvmap_ioctls);
	nvmap->file.ioctls = nvmap_ioctls;
	nvmap->file.ops = &nvmap_file_ops;

	INIT_LIST_HEAD(&nvmap->handles);

	return &nvmap->file;
}

static const struct ioctl nvhost_ctrl_ioctls[] = {
	IOCTL(NVHOST_IOCTL_CTRL_SYNCPT_READ),
	IOCTL(NVHOST_IOCTL_CTRL_SYNCPT_INCR),
	IOCTL(NVHOST_IOCTL_CTRL_SYNCPT_WAIT),
	IOCTL(NVHOST_IOCTL_CTRL_SYNCPT_WAITEX),
	IOCTL(NVHOST_IOCTL_CTRL_GET_VERSION),
};

struct nvhost_ctrl_file {
	struct file file;
};

static inline struct nvhost_ctrl_file *to_nvhost_ctrl_file(struct file *file)
{
	return container_of(file, struct nvhost_ctrl_file, file);
}

static void nvhost_ctrl_file_enter_ioctl_syncpt_read(struct file *file,
						     struct nvhost_ctrl_syncpt_read_args *args)
{
	printf("  Reading sync point %u\n", args->id);
	printf("    value: %u\n", args->value);
}

static void nvhost_ctrl_file_enter_ioctl_syncpt_waitex(struct file *file,
						       struct nvhost_ctrl_syncpt_waitex_args *args)
{
	printf("  Waiting for sync point %u to reach %u, timeout %u\n",
	       args->id, args->thresh, args->timeout);
	printf("    value: %u\n", args->value);
}

static int nvhost_ctrl_file_enter_ioctl(struct file *file,
					unsigned long request, void *arg)
{
	switch (request) {
	case NVHOST_IOCTL_CTRL_SYNCPT_READ:
		nvhost_ctrl_file_enter_ioctl_syncpt_read(file, arg);
		break;

	case NVHOST_IOCTL_CTRL_SYNCPT_WAITEX:
		nvhost_ctrl_file_enter_ioctl_syncpt_waitex(file, arg);
		break;
	}

	return 0;
}

static void nvhost_ctrl_file_leave_ioctl_syncpt_read(struct file *file,
						     struct nvhost_ctrl_syncpt_read_args *args)
{
	printf("  ID: %u\n", args->id);
	printf("  Value: %u\n", args->value);
}

static void nvhost_ctrl_file_leave_ioctl_syncpt_waitex(struct file *file,
						       struct nvhost_ctrl_syncpt_waitex_args *args)
{
	printf("  ID: %u\n", args->id);
	printf("  Threshold: %u\n", args->thresh);
	printf("  Timeout: %u\n", args->timeout);
	printf("  Value: %u\n", args->value);
}

static int nvhost_ctrl_file_leave_ioctl(struct file *file,
					unsigned long request, void *arg)
{
	switch (request) {
	case NVHOST_IOCTL_CTRL_SYNCPT_READ:
		nvhost_ctrl_file_leave_ioctl_syncpt_read(file, arg);
		break;

	case NVHOST_IOCTL_CTRL_SYNCPT_WAITEX:
		nvhost_ctrl_file_leave_ioctl_syncpt_waitex(file, arg);
		break;
	}

	return 0;
}

static void nvhost_ctrl_file_release(struct file *file)
{
	struct nvhost_ctrl_file *nvhost = to_nvhost_ctrl_file(file);

	free(file->path);
	free(nvhost);
}

static const struct file_ops nvhost_ctrl_file_ops = {
	.enter_ioctl = nvhost_ctrl_file_enter_ioctl,
	.leave_ioctl = nvhost_ctrl_file_leave_ioctl,
	.release = nvhost_ctrl_file_release,
};

struct file *nvhost_ctrl_file_new(const char *path, int fd)
{
	struct nvhost_ctrl_file *nvhost;

	nvhost = calloc(1, sizeof(*nvhost));
	if (!nvhost)
		return NULL;

	INIT_LIST_HEAD(&nvhost->file.list);
	nvhost->file.path = strdup(path);
	nvhost->file.fd = fd;

	nvhost->file.num_ioctls = ARRAY_SIZE(nvhost_ctrl_ioctls);
	nvhost->file.ioctls = nvhost_ctrl_ioctls;
	nvhost->file.ops = &nvhost_ctrl_file_ops;

	return &nvhost->file;
}

struct nvhost_file {
	struct file file;

	struct nvhost_job *job;
};

static inline struct nvhost_file *to_nvhost_file(struct file *file)
{
	return container_of(file, struct nvhost_file, file);
}

enum nvhost_opcode {
	NVHOST_OPCODE_SETCL,
	NVHOST_OPCODE_INCR,
	NVHOST_OPCODE_NONINCR,
	NVHOST_OPCODE_MASK,
	NVHOST_OPCODE_IMM,
	NVHOST_OPCODE_RESTART,
	NVHOST_OPCODE_GATHER,
	NVHOST_OPCODE_EXTEND = 14,
	NVHOST_OPCODE_CHDONE = 15,
};

#if 0
static const char *nvhost_opcode_names[] = {
	"NVHOST_OPCODE_SETCL",
	"NVHOST_OPCODE_INCR",
	"NVHOST_OPCODE_NONINCR",
	"NVHOST_OPCODE_MASK",
	"NVHOST_OPCODE_IMM",
	"NVHOST_OPCODE_RESTART",
	"NVHOST_OPCODE_GATHER",
	"NVHOST_OPCODE_UNKNOWN_7",
	"NVHOST_OPCODE_UNKNOWN_8",
	"NVHOST_OPCODE_UNKNOWN_9",
	"NVHOST_OPCODE_UNKNOWN_10",
	"NVHOST_OPCODE_UNKNOWN_11",
	"NVHOST_OPCODE_UNKNOWN_12",
	"NVHOST_OPCODE_UNKNOWN_13",
	"NVHOST_OPCODE_EXTEND",
	"NVHOST_OPCODE_CHDONE",
};
#endif

#define BIT(x) (1 << (x))

struct nvhost_stream {
	unsigned int num_words;
	unsigned int position;
	uint32_t *words;
};

static enum nvhost_opcode nvhost_stream_get_opcode(struct nvhost_stream *stream)
{
	return (stream->words[stream->position] >> 28) & 0xf;
}

static void nvhost_opcode_setcl_dump(struct nvhost_stream *stream)
{
	unsigned int offset, classid, mask, i;

	offset = (stream->words[stream->position] >> 16) & 0xfff;
	classid = (stream->words[stream->position] >> 6) & 0x3ff;
	mask =stream->words[stream->position] & 0x3f;

	printf("      NVHOST_OPCODE_SETCL: offset:%x classid:%x mask:%x\n",
	       offset, classid, mask);

	for (i = 0; i < 6; i++) {
		if (mask & BIT(i)) {
			uint32_t value = stream->words[++stream->position];
			printf("        %08x: %08x\n", offset + i, value);
		}
	}

	stream->position++;
}

static void nvhost_opcode_incr_dump(struct nvhost_stream *stream)
{
	unsigned int offset, count, i;

	offset = (stream->words[stream->position] >> 16) & 0xfff;
	count = stream->words[stream->position] & 0xffff;
	stream->position++;

	printf("      NVHOST_OPCODE_INCR: offset:%x count:%x\n", offset, count);

	for (i = 0; i < count; i++)
		printf("        %08x\n", stream->words[stream->position++]);
}

static void nvhost_opcode_nonincr_dump(struct nvhost_stream *stream)
{
	unsigned int offset, count, i;

	offset = (stream->words[stream->position] >> 16) & 0xfff;
	count = stream->words[stream->position] & 0xffff;
	stream->position++;

	printf("      NVHOST_OPCODE_NONINCR: offset:%x count:%x\n", offset,
	       count);

	for (i = 0; i < count; i++)
		printf("        %08x\n", stream->words[stream->position++]);
}

static void nvhost_opcode_mask_dump(struct nvhost_stream *stream)
{
	unsigned int offset, mask, i;

	offset = (stream->words[stream->position] >> 16) & 0xfff;
	mask = stream->words[stream->position] & 0xffff;
	stream->position++;

	printf("      NVHOST_OPCODE_MASK: offset:%x mask:%x\n", offset, mask);

	for (i = 0; i < 16; i++)
		if (mask & BIT(i))
			printf("        %08x: %08x\n", offset + i,
			       stream->words[stream->position++]);
}

static void nvhost_opcode_imm_dump(struct nvhost_stream *stream)
{
	unsigned int offset, value;

	offset = (stream->words[stream->position] >> 16) & 0xfff;
	value = stream->words[stream->position] & 0xffff;
	stream->position++;

	printf("      NVHOST_OPCODE_IMM: offset:%x value:%x\n", offset, value);
}

static void nvhost_opcode_restart_dump(struct nvhost_stream *stream)
{
	uint32_t offset;

	offset = (stream->words[stream->position] & 0x0fffffff) << 4;
	stream->position++;

	printf("      NVHOST_OPCODE_RESTART: offset:%x\n", offset);
}

static void nvhost_opcode_gather_dump(struct nvhost_stream *stream)
{
	unsigned int offset, count;
	const char *insert;

	offset = (stream->words[stream->position] >> 16) & 0xfff;
	count = stream->words[stream->position] & 0x3fff;

	if (stream->words[stream->position] & BIT(15)) {
		if (stream->words[stream->position] & BIT(14))
			insert = "INCR ";
		else
			insert = "NONINCR ";
	} else {
		insert = "";
	}
	stream->position++;

	printf("      NVHOST_OPCODE_GATHER: offset:%x %scount:%x base:%x\n",
	       offset, insert, count, stream->words[++stream->position]);
}

static void nvhost_opcode_extend_dump(struct nvhost_stream *stream)
{
	uint16_t subop;
	uint32_t value;

	subop = (stream->words[stream->position] >> 24) & 0xf;
	value = stream->words[stream->position] & 0xffffff;

	printf("      NVHOST_OPCODE_EXTEND: subop:%x value:%x\n", subop, value);

	stream->position++;
}

static void nvhost_opcode_chdone_dump(struct nvhost_stream *stream)
{
	printf("      NVHOST_OPCODE_CHDONE\n");
	stream->position++;
}

static void dump_commands(uint32_t *commands, unsigned int count)
{
	struct nvhost_stream stream = {
		.words = commands,
		.num_words = count,
		.position = 0,
	};

	printf("    commands: %u\n", count);

	while (stream.position < stream.num_words) {
		enum nvhost_opcode opcode = nvhost_stream_get_opcode(&stream);

		switch (opcode) {
		case NVHOST_OPCODE_SETCL:
			nvhost_opcode_setcl_dump(&stream);
			break;

		case NVHOST_OPCODE_INCR:
			nvhost_opcode_incr_dump(&stream);
			break;

		case NVHOST_OPCODE_NONINCR:
			nvhost_opcode_nonincr_dump(&stream);
			break;

		case NVHOST_OPCODE_MASK:
			nvhost_opcode_mask_dump(&stream);
			break;

		case NVHOST_OPCODE_IMM:
			nvhost_opcode_imm_dump(&stream);
			break;

		case NVHOST_OPCODE_RESTART:
			nvhost_opcode_restart_dump(&stream);
			break;

		case NVHOST_OPCODE_GATHER:
			nvhost_opcode_gather_dump(&stream);
			break;

		case NVHOST_OPCODE_EXTEND:
			nvhost_opcode_extend_dump(&stream);
			break;

		case NVHOST_OPCODE_CHDONE:
			nvhost_opcode_chdone_dump(&stream);
			break;

		default:
			printf("\n");
			break;
		}
	}
}

struct nvhost_cmdbuf {
	uint32_t mem;
	uint32_t offset;
	uint32_t words;
};

struct nvhost_reloc {
	uint32_t cmdbuf_mem;
	uint32_t cmdbuf_offset;
	uint32_t target_mem;
	uint32_t target_offset;
};

struct nvhost_reloc_shift {
	uint32_t shift;
};

struct nvhost_wait_check {
	uint32_t mem;
	uint32_t offset;
	uint32_t syncpt;
	uint32_t thresh;
};

struct nvhost_pushbuf {
	unsigned int num_words;
	unsigned int position;
	uint32_t *words;
};

static void nvhost_pushbuf_push(struct nvhost_pushbuf *pushbuf, uint32_t *words,
				unsigned int count)
{
	if (pushbuf->words) {
		fprintf(stderr, "WARNING: push buffer already filled\n");
		return;
	}

	pushbuf->num_words = count;

	pushbuf->words = calloc(count, sizeof(uint32_t));
	if (!pushbuf->words) {
		fprintf(stderr, "WARNING: cannot allocate push buffer\n");
		return;
	}

	memcpy(pushbuf->words, words, count * sizeof(uint32_t));
}

struct nvhost_job {
	struct nvhost_submit_hdr_ext submit;

	struct nvhost_pushbuf *pushbufs;
	unsigned int num_pushbufs;

	struct nvhost_reloc *relocs;
	unsigned int num_relocs;

	struct nvhost_wait_check *wait_checks;
	unsigned long wait_check_mask;
	unsigned int num_wait_checks;

	struct nvhost_reloc_shift *shifts;
	unsigned int num_shifts;
};

struct nvhost_job *nvhost_job_new(struct nvhost_submit_hdr_ext *submit)
{
	struct nvhost_job *job;

	job = calloc(1, sizeof(*job));
	if (!job)
		return NULL;

	memcpy(&job->submit, submit, sizeof(*submit));
	job->num_pushbufs = submit->num_cmdbufs;

	job->pushbufs = calloc(job->num_pushbufs, sizeof(struct nvhost_pushbuf));
	if (!job->pushbufs) {
		free(job);
		return NULL;
	}

	job->num_relocs = submit->num_relocs;

	job->relocs = calloc(job->num_relocs, sizeof(struct nvhost_reloc));
	if (!job->relocs) {
		free(job->pushbufs);
		free(job);
		return NULL;
	}

	job->num_wait_checks = submit->num_waitchks;
	job->wait_check_mask = submit->waitchk_mask;

	job->wait_checks = calloc(job->num_wait_checks, sizeof(struct nvhost_wait_check));
	if (!job->wait_checks) {
		free(job->relocs);
		free(job->pushbufs);
		free(job);
		return NULL;
	}

	if (submit->submit_version >= 2) {
		job->num_shifts = submit->num_relocs;

		job->shifts = calloc(job->num_shifts, sizeof(struct nvhost_reloc_shift));
		if (!job->shifts) {
			free(job->wait_checks);
			free(job->relocs);
			free(job->pushbufs);
			free(job);
			return NULL;
		}
	}

	return job;
}

static void nvhost_job_free(struct nvhost_job *job)
{
	free(job->shifts);
	free(job->wait_checks);
	free(job->relocs);
	free(job->pushbufs);
	free(job);
}

static void nvhost_job_add_pushbuf(struct nvhost_job *job, unsigned int index,
				   const struct nvhost_cmdbuf *cmdbuf)
{
	struct file *file = file_find("/dev/nvmap");
	struct nvmap_file *nvmap = to_nvmap_file(file);
	struct nvmap_handle *handle;

	printf("  Command Buffer:\n");
	printf("    mem: %x, offset: %x, words: %u\n", cmdbuf->mem,
	       cmdbuf->offset, cmdbuf->words);

	if (file) {
		handle = nvmap_file_lookup_handle(nvmap, cmdbuf->mem);
		if (handle) {
			uint32_t *commands = handle->mapped + cmdbuf->offset;
			struct nvhost_pushbuf *pushbuf = &job->pushbufs[index];

			if (!handle->mapped)
				commands = handle->buffer + cmdbuf->offset;

			nvhost_pushbuf_push(pushbuf, commands, cmdbuf->words);
			dump_commands(commands, cmdbuf->words);
		}
	} else {
		fprintf(stderr, "nvmap not found!\n");
		exit(1);
	}
}

static void nvhost_job_add_reloc(struct nvhost_job *job, unsigned int index,
				 const struct nvhost_reloc *reloc)
{
	printf("  Relocation:\n");
	printf("    command buffer:\n");
	printf("      %x, offset: %x\n", reloc->cmdbuf_mem,
	       reloc->cmdbuf_offset);
	printf("    target:\n");
	printf("      %x, offset: %x\n", reloc->target_mem,
	       reloc->target_offset);

	memcpy(&job->relocs[index], reloc, sizeof(*reloc));
}

static void nvhost_job_add_wait_check(struct nvhost_job *job, unsigned int index,
				      const struct nvhost_wait_check *check)
{
	printf("  Wait Check:\n");
	printf("    mem: %x, offset: %x\n", check->mem,
	       check->offset);
	printf("    syncpt: %x, threshold: %x\n",
	       check->syncpt, check->thresh);
}

static void nvhost_job_add_shift(struct nvhost_job *job, unsigned int index,
				 const struct nvhost_reloc_shift *shift)
{
	printf("  Relocation Shift:\n");
	printf("    %x\n", shift->shift);
}

static void nvhost_file_enter_ioctl_channel_set_nvmap_fd(struct file *file,
							 struct nvhost_set_nvmap_fd_args *args)
{
	printf("  Setting NVMAP file descriptor:\n");
	printf("    file descriptor: %d\n", args->fd);
}

static void nvhost_file_enter_ioctl_channel_flush(struct file *file,
						  struct nvhost_get_param_args *args)
{
	struct nvhost_file *nvhost = to_nvhost_file(file);
	struct nvhost_job *job = nvhost->job;
	unsigned int i;

	printf("  Flushing channel:\n");

	for (i = 0; i < job->num_relocs; i++) {
		struct nvhost_reloc *reloc = &job->relocs[i];
		struct file *file = file_find("/dev/nvmap");
		struct nvmap_handle *cmdbuf, *target;
		struct nvmap_file *nvmap;

		nvmap = to_nvmap_file(file);

		cmdbuf = nvmap_file_lookup_handle(nvmap, reloc->cmdbuf_mem);
		target = nvmap_file_lookup_handle(nvmap, reloc->target_mem);

		printf("    relocating: %x, offset:%x -> %x, offset:%x\n",
		       reloc->cmdbuf_mem, reloc->cmdbuf_offset,
		       reloc->target_mem, reloc->target_offset);

		if (cmdbuf)
			printf("      cmdbuf: id:%lx, size:%zu\n", cmdbuf->id, cmdbuf->size);

		if (target)
			printf("      target: id:%lx, size:%zu\n", target->id, target->size);
	}
}

static void nvhost_file_enter_ioctl_channel_get_syncpoints(struct file *file,
							   struct nvhost_get_param_args *args)
{
	printf("  Getting syncpoints: %x\n", args->value);
}

static void nvhost_file_enter_ioctl_channel_get_waitbases(struct file *file,
							  struct nvhost_get_param_args *args)
{
	printf("  Getting waitbases: %x\n", args->value);
}

static void nvhost_file_enter_ioctl_channel_submit(struct file *file,
						   struct nvhost_submit_hdr_ext *submit)
{
	struct nvhost_file *nvhost = to_nvhost_file(file);

	printf("  submit header:\n");
	printf("    syncpt: %u, increments: %u\n", submit->syncpt_id,
	       submit->syncpt_incrs);
	printf("    command buffers: %u\n", submit->num_cmdbufs);
	printf("    relocations: %u\n", submit->num_relocs);
	printf("    version: %u\n", submit->submit_version);
	printf("    wait checks: %u\n", submit->num_waitchks);
	printf("    wait check mask: %x\n", submit->waitchk_mask);

	if (nvhost->job)
		nvhost_job_free(nvhost->job);

	nvhost->job = nvhost_job_new(submit);
	if (!nvhost->job)
		fprintf(stderr, "failed to create new job\n");
}

static int nvhost_file_enter_ioctl(struct file *file, unsigned long request,
				   void *arg)
{
	switch (request) {
	case NVHOST_IOCTL_CHANNEL_FLUSH:
		nvhost_file_enter_ioctl_channel_flush(file, arg);
		break;

	case NVHOST_IOCTL_CHANNEL_GET_SYNCPOINTS:
		nvhost_file_enter_ioctl_channel_get_syncpoints(file, arg);
		break;

	case NVHOST_IOCTL_CHANNEL_GET_WAITBASES:
		nvhost_file_enter_ioctl_channel_get_waitbases(file, arg);
		break;

	case NVHOST_IOCTL_CHANNEL_SET_NVMAP_FD:
		nvhost_file_enter_ioctl_channel_set_nvmap_fd(file, arg);
		break;

	case NVHOST_IOCTL_CHANNEL_SUBMIT_EXT:
		nvhost_file_enter_ioctl_channel_submit(file, arg);
		break;

	default:
		break;
	}

	return 0;
}

static void nvhost_file_leave_ioctl_channel_flush(struct file *file,
						  struct nvhost_get_param_args *args)
{
	printf("  Channel flushed\n");
	printf("    Fence: %x, %u\n", args->value, args->value);
}

static void nvhost_file_leave_ioctl_channel_get_syncpoints(struct file *file,
							   struct nvhost_get_param_args *args)
{
	printf("  Syncpoints received: %x\n", args->value);
}

static void nvhost_file_leave_ioctl_channel_get_waitbases(struct file *file,
							  struct nvhost_get_param_args *args)
{
	printf("  Waitbases received: %x\n", args->value);
}

static int nvhost_file_leave_ioctl(struct file *file, unsigned long request,
				   void *arg)
{
	switch (request) {
	case NVHOST_IOCTL_CHANNEL_FLUSH:
		nvhost_file_leave_ioctl_channel_flush(file, arg);
		break;

	case NVHOST_IOCTL_CHANNEL_GET_SYNCPOINTS:
		nvhost_file_leave_ioctl_channel_get_syncpoints(file, arg);
		break;

	case NVHOST_IOCTL_CHANNEL_GET_WAITBASES:
		nvhost_file_leave_ioctl_channel_get_waitbases(file, arg);
		break;
	}

	return 0;
}

static ssize_t nvhost_file_write(struct file *file, const void *buffer,
				 size_t size)
{
	struct nvhost_file *nvhost = to_nvhost_file(file);
	struct nvhost_job *job = nvhost->job;
	size_t pos = 0;

	while (pos < size) {
		if (job->submit.num_cmdbufs) {
			unsigned int index = job->num_pushbufs - job->submit.num_cmdbufs;
			const struct nvhost_cmdbuf *cmdbuf = buffer + pos;

			nvhost_job_add_pushbuf(nvhost->job, index, cmdbuf);

			job->submit.num_cmdbufs--;
			pos += sizeof(*cmdbuf);
		} else if (job->submit.num_relocs) {
			unsigned int index = job->num_relocs - job->submit.num_relocs;
			const struct nvhost_reloc *reloc = buffer + pos;

			nvhost_job_add_reloc(job, index, reloc);

			job->submit.num_relocs--;
			pos += sizeof(*reloc);
		} else if (job->submit.num_waitchks) {
			unsigned int index = job->num_wait_checks - job->submit.num_waitchks;
			const struct nvhost_wait_check *check = buffer + pos;

			nvhost_job_add_wait_check(job, index, check);

			job->submit.num_waitchks--;
			pos += sizeof(*check);
		} else if (job->num_shifts) {
			const struct nvhost_reloc_shift *shift = buffer + pos;
			unsigned int index = 0;

			nvhost_job_add_shift(job, index, shift);

			job->num_shifts--;
			pos += sizeof(*shift);
		}
	}

	return size;
}

static void nvhost_file_release(struct file *file)
{
	struct nvhost_file *nvhost = to_nvhost_file(file);

	free(file->path);
	free(nvhost);
}

static const struct file_ops nvhost_file_ops = {
	.enter_ioctl = nvhost_file_enter_ioctl,
	.leave_ioctl = nvhost_file_leave_ioctl,
	.write = nvhost_file_write,
	.release = nvhost_file_release,
};

const struct ioctl nvhost_ioctls[] = {
	IOCTL(NVHOST_IOCTL_CHANNEL_FLUSH),
	IOCTL(NVHOST_IOCTL_CHANNEL_GET_SYNCPOINTS),
	IOCTL(NVHOST_IOCTL_CHANNEL_GET_WAITBASES),
	IOCTL(NVHOST_IOCTL_CHANNEL_GET_MODMUTEXES),
	IOCTL(NVHOST_IOCTL_CHANNEL_SET_NVMAP_FD),
	IOCTL(NVHOST_IOCTL_CHANNEL_NULL_KICKOFF),
	IOCTL(NVHOST_IOCTL_CHANNEL_SUBMIT_EXT),
	IOCTL(NVHOST_IOCTL_GET_TIMEDOUT),
	IOCTL(NVHOST_IOCTL_SET_PRIORITY),
};

struct file *nvhost_file_new(const char *path, int fd)
{
	struct nvhost_file *nvhost;

	nvhost = calloc(1, sizeof(*nvhost));
	if (!nvhost)
		return NULL;

	INIT_LIST_HEAD(&nvhost->file.list);
	nvhost->file.path = strdup(path);
	nvhost->file.fd = fd;

	nvhost->file.num_ioctls = ARRAY_SIZE(nvhost_ioctls);
	nvhost->file.ioctls = nvhost_ioctls;
	nvhost->file.ops = &nvhost_file_ops;

	return &nvhost->file;
}

static const struct file_table nvhost_files[] = {
	{ "/dev/nvmap", nvmap_file_new },
	{ "/dev/nvhost-ctrl", nvhost_ctrl_file_new },
	{ "/dev/nvhost-gr2d", nvhost_file_new },
	{ "/dev/nvhost-gr3d", nvhost_file_new },
};

void nvhost_register(void)
{
	file_table_register(nvhost_files, ARRAY_SIZE(nvhost_files));
}
