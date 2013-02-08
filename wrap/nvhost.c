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

	print_hexdump(stdout, DUMP_PREFIX_ADDRESS, "    ", virt, args->length,
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

static void nvhost_ctrl_file_release(struct file *file)
{
	struct nvhost_ctrl_file *nvhost = to_nvhost_ctrl_file(file);

	free(file->path);
	free(nvhost);
}

static const struct file_ops nvhost_ctrl_file_ops = {
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

	struct nvhost_submit_hdr_ext submit;
	unsigned int num_reloc_shifts;
};

static inline struct nvhost_file *to_nvhost_file(struct file *file)
{
	return container_of(file, struct nvhost_file, file);
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

	if (submit->submit_version >= 2)
		nvhost->num_reloc_shifts = submit->num_relocs;

	memcpy(&nvhost->submit, submit, sizeof(*submit));

	printf("  relocation shifts: %u\n", nvhost->num_reloc_shifts);
}

static int nvhost_file_enter_ioctl(struct file *file, unsigned long request,
				   void *arg)
{
	switch (request) {
	case NVHOST_IOCTL_CHANNEL_SUBMIT_EXT:
		nvhost_file_enter_ioctl_channel_submit(file, arg);
		break;

	default:
		break;
	}

	return 0;
}

static int nvhost_file_leave_ioctl(struct file *file, unsigned long request,
				   void *arg)
{
	return 0;
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

#define BIT(x) (1 << (x))

static void dump_commands(uint32_t *commands, unsigned int count)
{
	unsigned int i, j;

	printf("    commands: %u\n", count);

	for (i = 0; i < count; i++) {
		enum nvhost_opcode opcode = (commands[i] >> 28) & 0xf;
		uint16_t classid, mask, subop;
		uint32_t offset, value;
		const char *insert;

		printf("      %08x\n", commands[i]);
		printf("        %x (%s)", opcode, nvhost_opcode_names[opcode]);

		switch (opcode) {
		case NVHOST_OPCODE_SETCL:
			offset = (commands[i] >> 16) & 0xfff;
			classid = (commands[i] >> 6) & 0x3ff;
			mask = commands[i] & 0x3f;

			printf(" offset:%x classid:%x mask:%x\n", offset,
			       classid, mask);

			for (j = 0; j < 6; j++) {
				if (mask & BIT(j)) {
					printf("          %08x: %08x\n",
					       offset + j, commands[++i]);
				}
			}

			break;

		case NVHOST_OPCODE_INCR:
			offset = (commands[i] >> 16) & 0xfff;
			value = commands[i] & 0xffff;

			printf(" offset:%x count:%x\n", offset, value);

			for (j = 0; j < value; j++) {
				printf("          %08x\n", commands[++i]);
			}

			break;

		case NVHOST_OPCODE_NONINCR:
			offset = (commands[i] >> 16) & 0xfff;
			value = commands[i] & 0xffff;

			printf(" offset:%x count:%x\n", offset, value);

			for (j = 0; j < value; j++) {
				printf("          %08x\n", commands[++i]);
			}

			break;

		case NVHOST_OPCODE_MASK:
			offset = (commands[i] >> 16) & 0xfff;
			mask = commands[i] & 0xffff;

			printf(" offset:%x mask:%x\n", offset, mask);

			for (j = 0; j < 16; j++) {
				if (mask & BIT(j))
					printf("          %08x: %08x\n",
					       offset + j, commands[++i]);
			}

			break;

		case NVHOST_OPCODE_IMM:
			offset = (commands[i] >> 16) & 0xfff;
			value = commands[i] & 0xffff;

			printf(" offset:%x value:%x\n", offset, value);
			break;

		case NVHOST_OPCODE_RESTART:
			offset = (commands[i] & 0x0fffffff) << 4;
			printf(" offset:%x\n", offset);
			break;

		case NVHOST_OPCODE_GATHER:
			offset = (commands[i] >> 16) & 0xfff;
			value = commands[i] & 0x3fff;

			if (commands[i] & BIT(15)) {
				if (commands[i] & BIT(14))
					insert = "INCR ";
				else
					insert = "NONINCR ";
			} else {
				insert = "";
			}

			printf(" offset:%x %scount:%x base:%x\n", offset,
			       insert, value, commands[++i]);
			break;

		case NVHOST_OPCODE_EXTEND:
			subop = (commands[i] >> 24) & 0xf;
			value = commands[i] & 0xffffff;

			printf(" subop:%x value:%x\n", subop, value);
			break;

		default:
			printf("\n");
			break;
		}
	}
}

static ssize_t nvhost_file_write(struct file *file, const void *buffer,
				 size_t size)
{
	struct nvhost_file *nvhost = to_nvhost_file(file);
	size_t pos = 0;

	while (pos < size) {
		if (nvhost->submit.num_cmdbufs) {
			const struct nvhost_cmdbuf *cmdbuf = buffer + pos;

			printf("  Command Buffer:\n");
			printf("    mem: %x, offset: %x, words: %u\n",
			       cmdbuf->mem, cmdbuf->offset, cmdbuf->words);

			if (1) {
				struct file *file = file_find("/dev/nvmap");
				struct nvmap_file *nvmap = to_nvmap_file(file);
				struct nvmap_handle *handle;

				if (nvmap) {
					handle = nvmap_file_lookup_handle(nvmap, cmdbuf->mem);
					if (handle) {
						uint32_t *commands = handle->mapped + cmdbuf->offset;

						if (!handle->mapped)
							commands = handle->buffer + cmdbuf->offset;

						dump_commands(commands, cmdbuf->words);
					}
				}
			}

			nvhost->submit.num_cmdbufs--;
			pos += sizeof(*cmdbuf);
		} else if (nvhost->submit.num_relocs) {
			const struct nvhost_reloc *reloc = buffer + pos;

			printf("  Relocation:\n");
			printf("    command buffer:\n");
			printf("      %x, offset: %x\n", reloc->cmdbuf_mem,
			       reloc->cmdbuf_offset);
			printf("    target:\n");
			printf("      %x, offset: %x\n", reloc->target_mem,
			       reloc->target_offset);

			nvhost->submit.num_relocs--;
			pos += sizeof(*reloc);
		} else if (nvhost->submit.num_waitchks) {
			const struct nvhost_wait_check *check = buffer + pos;

			printf("  Wait Check:\n");
			printf("    mem: %x, offset: %x\n", check->mem,
			       check->offset);
			printf("    syncpt: %x, threshold: %x\n",
			       check->syncpt, check->thresh);

			nvhost->submit.num_waitchks--;
			pos += sizeof(*check);
		} else if (nvhost->num_reloc_shifts) {
			const struct nvhost_reloc_shift *shift = buffer + pos;

			printf("  Relocation Shift:\n");
			printf("    %x\n", shift->shift);

			nvhost->num_reloc_shifts--;
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

static void __attribute__((constructor)) nvhost_register(void)
{
	file_table_register(nvhost_files, ARRAY_SIZE(nvhost_files));
}
