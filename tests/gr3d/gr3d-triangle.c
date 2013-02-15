#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include <png.h>

#define ATTRIB_BASE(x) (0x100 + ((x) * 2))
#define ATTRIB_MODE(x) (0x101 + ((x) * 2))

#define INDEX_BASE 0x121
#define DRAW_PARAMS 0x122
#define DRAW_PRIMITIVES 0x123

#define DEPTH_BASE 0xe00
#define COLOR_BASE 0xe01

struct nvmap_create_handle {
	union {
		uint32_t key;
		uint32_t id;
		uint32_t size;
	};
	uint32_t handle;
};

struct nvmap_alloc_handle {
	uint32_t handle;
	uint32_t heap_mask;
	uint32_t flags;
	uint32_t align;
};

struct nvmap_map_caller {
	uint32_t handle;
	uint32_t offset;
	uint32_t length;
	uint32_t flags;
	uint32_t addr;
};

struct nvmap_rw_handle {
	uint32_t addr;
	uint32_t handle;
	uint32_t offset;
	uint32_t elem_size;
	uint32_t hmem_stride;
	uint32_t user_stride;
	uint32_t count;
};

struct nvmap_handle_param {
	uint32_t handle;
	uint32_t param;
	uint32_t result;
};

struct nvmap_pin_handle {
	uint32_t handles;
	uint32_t addr;
	uint32_t count;
};

struct nvmap_cache_op {
	uint32_t addr;
	uint32_t handle;
	uint32_t length;
	uint32_t op;
};

#define NVMAP_IOCTL_MAGIC 'N'

#define NVMAP_IOCTL_CREATE _IOWR(NVMAP_IOCTL_MAGIC, 0, struct nvmap_create_handle)
#define NVMAP_IOCTL_CLAIM _IOWR(NVMAP_IOCTL_MAGIC, 1, struct nvmap_create_handle)
#define NVMAP_IOCTL_FROM_ID _IOWR(NVMAP_IOCTL_MAGIC, 2, struct nvmap_create_handle)
#define NVMAP_IOCTL_ALLOC _IOW(NVMAP_IOCTL_MAGIC, 3, struct nvmap_alloc_handle)
#define NVMAP_IOCTL_FREE _IO(NVMAP_IOCTL_MAGIC, 4)
#define NVMAP_IOCTL_MMAP _IOWR(NVMAP_IOCTL_MAGIC, 5, struct nvmap_map_caller)
#define NVMAP_IOCTL_WRITE _IOW(NVMAP_IOCTL_MAGIC, 6, struct nvmap_rw_handle)
#define NVMAP_IOCTL_READ _IOW(NVMAP_IOCTL_MAGIC, 7, struct nvmap_rw_handle)
#define NVMAP_IOCTL_PARAM _IOWR(NVMAP_IOCTL_MAGIC, 8, struct nvmap_handle_param)
#define NVMAP_IOCTL_PIN _IOWR(NVMAP_IOCTL_MAGIC, 10, struct nvmap_pin_handle)
#define NVMAP_IOCTL_UNPIN _IOW(NVMAP_IOCTL_MAGIC, 11, struct nvmap_pin_handle)
#define NVMAP_IOCTL_CACHE _IOW(NVMAP_IOCTL_MAGIC, 12, struct nvmap_cache_op)
#define NVMAP_IOCTL_GET_ID _IOWR(NVMAP_IOCTL_MAGIC, 13, struct nvmap_create_handle)

struct nvmap {
	int fd;
};

struct nvmap *nvmap_open(void)
{
	struct nvmap *nvmap;

	nvmap = calloc(1, sizeof(*nvmap));
	if (!nvmap)
		return NULL;

	nvmap->fd = open("/dev/nvmap", O_RDWR);
	if (nvmap->fd < 0) {
		free(nvmap);
		return NULL;
	}

	return nvmap;
}

void nvmap_close(struct nvmap *nvmap)
{
	if (nvmap) {
		if (nvmap->fd >= 0)
			close(nvmap->fd);
	}

	free(nvmap);
}

struct nvmap_handle {
	size_t size;
	uint32_t id;
	void *ptr;
};

struct nvmap_handle *nvmap_handle_create(struct nvmap *nvmap, size_t size)
{
	struct nvmap_create_handle args;
	struct nvmap_handle *handle;
	int err;

	handle = calloc(1, sizeof(*handle));
	if (!handle)
		return NULL;

	handle->size = size;

	memset(&args, 0, sizeof(args));
	args.size = size;

	err = ioctl(nvmap->fd, NVMAP_IOCTL_CREATE, &args);
	if (err < 0) {
		free(handle);
		return NULL;
	}

	handle->id = args.handle;

	return handle;
}

void nvmap_handle_free(struct nvmap *nvmap, struct nvmap_handle *handle)
{
	int err;

	err = ioctl(nvmap->fd, NVMAP_IOCTL_FREE, handle->id);
	if (err < 0) {
		fprintf(stderr, "failed to free nvmap handle\n");
	}

	free(handle);
}

int nvmap_handle_alloc(struct nvmap *nvmap, struct nvmap_handle *handle,
		       unsigned long flags, unsigned long align)
{
	struct nvmap_alloc_handle args;
	int err;

	memset(&args, 0, sizeof(args));
	args.handle = handle->id;
	args.heap_mask = 1 << 0;
	args.flags = flags;
	args.align = align;

	err = ioctl(nvmap->fd, NVMAP_IOCTL_ALLOC, &args);
	if (err < 0) {
		return -errno;
	}

	return 0;
}

#define ROUNDUP(n, d) ((((n) + (d) - 1) / (d)) * (d))

int nvmap_handle_mmap(struct nvmap *nvmap, struct nvmap_handle *handle)
{
	uint32_t size = ROUNDUP(handle->size, 4096);
	struct nvmap_map_caller args;
	int err;

	handle->ptr = mmap(NULL, size, 3, 1, nvmap->fd, 0);

	memset(&args, 0, sizeof(args));
	args.handle = handle->id;
	args.offset = 0;
	args.length = size;
	args.flags = 0;
	args.addr = (uintptr_t)handle->ptr;

	err = ioctl(nvmap->fd, NVMAP_IOCTL_MMAP, &args);
	if (err < 0)
		return -errno;

	return 0;
}

struct nvhost_get_param_args {
	uint32_t value;
};

struct nvhost_set_nvmap_fd_args {
	uint32_t fd;
};

struct nvhost_submit_hdr_ext {
	uint32_t syncpt_id;
	uint32_t syncpt_incrs;
	uint32_t num_cmdbufs;
	uint32_t num_relocs;
	uint32_t submit_version;
	uint32_t num_waitchks;
	uint32_t waitchk_mask;
	uint32_t pad[5];
};

struct nvhost_set_priority_args {
	uint32_t priority;
};

#define NVHOST_IOCTL_MAGIC 'H'

#define NVHOST_IOCTL_CHANNEL_FLUSH           _IOR(NVHOST_IOCTL_MAGIC,  1, struct nvhost_get_param_args)
#define NVHOST_IOCTL_CHANNEL_GET_SYNCPOINTS  _IOR(NVHOST_IOCTL_MAGIC,  2, struct nvhost_get_param_args)
#define NVHOST_IOCTL_CHANNEL_GET_WAITBASES   _IOR(NVHOST_IOCTL_MAGIC,  3, struct nvhost_get_param_args)
#define NVHOST_IOCTL_CHANNEL_GET_MODMUTEXES  _IOR(NVHOST_IOCTL_MAGIC,  4, struct nvhost_get_param_args)
#define NVHOST_IOCTL_CHANNEL_SET_NVMAP_FD    _IOW(NVHOST_IOCTL_MAGIC,  5, struct nvhost_set_nvmap_fd_args)
#define NVHOST_IOCTL_CHANNEL_NULL_KICKOFF    _IOR(NVHOST_IOCTL_MAGIC,  6, struct nvhost_get_param_args)
#define NVHOST_IOCTL_CHANNEL_SUBMIT_EXT      _IOW(NVHOST_IOCTL_MAGIC,  7, struct nvhost_submit_hdr_ext)
#define NVHOST_IOCTL_GET_TIMEDOUT            _IOR(NVHOST_IOCTL_MAGIC, 12, struct nvhost_get_param_args)
#define NVHOST_IOCTL_SET_PRIORITY            _IOW(NVHOST_IOCTL_MAGIC, 13, struct nvhost_set_priority_args)

#define NVHOST_OPCODE_SETCL(offset, classid, mask) \
	((0x0 << 28) | (((offset) & 0xfff) << 16) | (((classid) & 0x3ff) << 6) | ((mask) & 0x3f))
#define NVHOST_OPCODE_INCR(offset, count) \
	((0x1 << 28) | (((offset) & 0xfff) << 16) | ((count) & 0xffff))
#define NVHOST_OPCODE_NONINCR(offset, count) \
	((0x2 << 28) | (((offset) & 0xfff) << 16) | ((count) & 0xffff))
#define NVHOST_OPCODE_MASK(offset, mask) \
	((0x3 << 28) | (((offset) & 0xfff) << 16) | ((mask) & 0xffff))
#define NVHOST_OPCODE_IMM(offset, data) \
	((0x4 << 28) | (((offset) & 0xfff) << 16) | ((data) & 0xffff))
#define NVHOST_OPCODE_EXTEND(subop, value) \
	((0xe << 28) | (((subop) & 0xf) << 24) | ((value) & 0xffffff))

struct nvhost_ctrl_syncpt_read_args {
	uint32_t id;
	uint32_t value;
};

struct nvhost_ctrl_syncpt_incr_args {
	uint32_t id;
};

struct nvhost_ctrl_syncpt_wait_args {
	uint32_t id;
	uint32_t thresh;
	uint32_t timeout;
};

struct nvhost_ctrl_syncpt_waitex_args {
	uint32_t id;
	uint32_t thresh;
	uint32_t timeout;
	uint32_t value;
};

#define NVHOST_IOCTL_CTRL_SYNCPT_READ _IOWR(NVHOST_IOCTL_MAGIC, 1, struct nvhost_ctrl_syncpt_read_args)
#define NVHOST_IOCTL_CTRL_SYNCPT_INCR _IOW(NVHOST_IOCTL_MAGIC, 2, struct nvhost_ctrl_syncpt_incr_args)
#define NVHOST_IOCTL_CTRL_SYNCPT_WAIT _IOW(NVHOST_IOCTL_MAGIC, 3, struct nvhost_ctrl_syncpt_wait_args)
#define NVHOST_IOCTL_CTRL_SYNCPT_WAITEX _IOWR(NVHOST_IOCTL_MAGIC, 6, struct nvhost_ctrl_syncpt_waitex_args)
#define NVHOST_IOCTL_CTRL_GET_VERSION _IOR(NVHOST_IOCTL_MAGIC, 7, struct nvhost_get_param_args)

struct nvhost_ctrl {
	int fd;
};

struct nvhost_ctrl *nvhost_ctrl_open(void)
{
	struct nvhost_ctrl *ctrl;

	ctrl = calloc(1, sizeof(*ctrl));
	if (!ctrl)
		return NULL;

	ctrl->fd = open("/dev/nvhost-ctrl", O_RDWR);
	if (ctrl->fd < 0) {
		free(ctrl);
		return NULL;
	}

	return ctrl;
}

void nvhost_ctrl_close(struct nvhost_ctrl *ctrl)
{
	if (ctrl) {
		if (ctrl->fd >= 0)
			close(ctrl->fd);
	}

	free(ctrl);
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

struct nvhost_gr3d {
	struct nvmap_handle *attributes;
	struct nvmap_handle *buffer;
	struct nvmap_handle *fb;
	struct nvhost_ctrl *ctrl;
	struct nvmap *nvmap;
	int fd;

	unsigned long words;

	struct nvhost_reloc relocs[4];
	unsigned int num_relocs;

	struct nvhost_reloc_shift shifts[4];
	unsigned int num_shifts;
};

struct nvhost_gr3d *nvhost_gr3d_open(struct nvmap *nvmap, struct nvhost_ctrl *ctrl)
{
	struct nvhost_set_nvmap_fd_args args;
	struct nvhost_gr3d *gr3d;
	int err;

	gr3d = calloc(1, sizeof(*gr3d));
	if (!gr3d)
		return NULL;

	gr3d->fd = open("/dev/nvhost-gr3d", O_RDWR);
	if (gr3d->fd < 0) {
		free(gr3d);
		return NULL;
	}

	gr3d->nvmap = nvmap;
	gr3d->ctrl = ctrl;

	memset(&args, 0, sizeof(args));
	args.fd = nvmap->fd;

	err = ioctl(gr3d->fd, NVHOST_IOCTL_CHANNEL_SET_NVMAP_FD, &args);
	if (err < 0) {
		close(gr3d->fd);
		free(gr3d);
		return NULL;
	}

	gr3d->buffer = nvmap_handle_create(nvmap, 8 * 4096);
	if (!gr3d->buffer) {
		close(gr3d->fd);
		free(gr3d);
		return NULL;
	}

	err = nvmap_handle_alloc(nvmap, gr3d->buffer, 0x0a000001, 0x20);
	if (err < 0) {
		nvmap_handle_free(nvmap, gr3d->buffer);
		close(gr3d->fd);
		free(gr3d);
		return NULL;
	}

	err = nvmap_handle_mmap(nvmap, gr3d->buffer);
	if (err < 0) {
		nvmap_handle_free(nvmap, gr3d->buffer);
		close(gr3d->fd);
		free(gr3d);
		return NULL;
	}

	gr3d->attributes = nvmap_handle_create(nvmap, 8 * 4096);
	if (!gr3d->attributes) {
	}

#error implement attributes buffer

	return gr3d;
}

void nvhost_gr3d_close(struct nvhost_gr3d *gr3d)
{
	if (gr3d) {
		if (gr3d->fd >= 0)
			close(gr3d->fd);

		nvmap_handle_free(gr3d->nvmap, gr3d->buffer);
	}

	free(gr3d);
}

int nvhost_gr3d_submit(struct nvhost_gr3d *gr3d, uint32_t *fencep)
{
	struct nvhost_get_param_args fence;
	struct nvhost_submit_hdr_ext args;
	struct nvhost_cmdbuf cmdbuf;
	int err;

	memset(&args, 0, sizeof(args));
	args.syncpt_id = 22;
	args.syncpt_incrs = 1;
	args.num_cmdbufs = 1;
	args.num_relocs = gr3d->num_relocs;
	args.submit_version = 2;
	args.num_waitchks = 0;
	args.waitchk_mask = 0;

	err = ioctl(gr3d->fd, NVHOST_IOCTL_CHANNEL_SUBMIT_EXT, &args);
	if (err < 0) {
		fprintf(stderr, "NVHOST_IOCTL_CHANNEL_SUBMIT_EXT: %d\n",
			errno);
		return -errno;
	}

	memset(&cmdbuf, 0, sizeof(cmdbuf));
	cmdbuf.mem = gr3d->buffer->id;
	cmdbuf.offset = 0;
	cmdbuf.words = 23;

	err = write(gr3d->fd, &cmdbuf, sizeof(cmdbuf));
	if (err < 0) {
	}

	err = write(gr3d->fd, gr3d->relocs, sizeof(struct nvhost_reloc) * gr3d->num_relocs);
	if (err < 0) {
	}

	err = write(gr3d->fd, gr3d->shifts, sizeof(struct nvhost_reloc_shift) * gr3d->num_relocs);
	if (err < 0) {
	}

	err = ioctl(gr3d->fd, NVHOST_IOCTL_CHANNEL_FLUSH, &fence);
	if (err < 0)
		return -errno;

	printf("fence: %u\n", fence.value);
	*fencep = fence.value;

	return 0;
}

int nvhost_gr3d_wait(struct nvhost_gr3d *gr3d, uint32_t fence, uint32_t timeout)
{
	struct nvhost_ctrl_syncpt_waitex_args args;
	int err;

	memset(&args, 0, sizeof(args));
	args.id = 22;
	args.thresh = fence;
	args.timeout = timeout;

	err = ioctl(gr3d->ctrl->fd, NVHOST_IOCTL_CTRL_SYNCPT_WAITEX, &args);
	if (err < 0)
		return -errno;

	if (args.value != args.thresh)
		fprintf(stderr, "syncpt %u: value:%u != thresh:%u\n",
			args.id, args.value, args.thresh);

	return 0;
}

int nvmap_handle_invalidate(struct nvmap *nvmap, struct nvmap_handle *handle,
			    unsigned long offset, unsigned long length)
{
	struct nvmap_cache_op args;
	int err;

	memset(&args, 0, sizeof(args));
	args.addr = (uintptr_t)handle->ptr + offset;
	args.handle = handle->id;
	args.length = length;
	args.op = 1;

	err = ioctl(nvmap->fd, NVMAP_IOCTL_CACHE, &args);
	if (err < 0)
		return -errno;

	return 0;
}

int nvmap_handle_writeback_invalidate(struct nvmap *nvmap,
				      struct nvmap_handle *handle,
				      unsigned long offset,
				      unsigned long length)
{
	struct nvmap_cache_op args;
	int err;

	memset(&args, 0, sizeof(args));
	args.addr = (uintptr_t)handle->ptr + offset;
	args.handle = handle->id;
	args.length = length;
	args.op = 2;

	err = ioctl(nvmap->fd, NVMAP_IOCTL_CACHE, &args);
	if (err < 0)
		return -errno;

	return 0;
}

static inline unsigned long get_offset(struct nvmap_handle *handle,
				       uint32_t *ptr)
{
	return (unsigned long)ptr - (unsigned long)handle->ptr;
}

void nvhost_gr3d_triangle(struct nvhost_gr3d *gr3d)
{
	uint32_t *ptr = gr3d->buffer->ptr;
	unsigned long length;
	uint32_t fence;
	int err;

	/*
	  Command Buffer:
	    mem: e462a7c0, offset: 3b4c, words: 103
	    commands: 103
	*/

	*ptr++ = NVHOST_OPCODE_SETCL(0, 0x60, 0);
	*ptr++ = NVHOST_OPCODE_INCR(0x404, 2);
	*ptr++ = 0x00000000;
	*ptr++ = 0x000fffff;
	*ptr++ = NVHOST_OPCODE_MASK(0x354, 9);
	*ptr++ = 0x3efffff0;
	*ptr++ = 0x3efffff0;
	*ptr++ = NVHOST_OPCODE_IMM(0x740, 0x035);
	*ptr++ = NVHOST_OPCODE_IMM(0xe26, 0x779);
	*ptr++ = NVHOST_OPCODE_INCR(0x346, 0x2);
	*ptr++ = 0x00001401;
	*ptr++ = 0x3f800000;
	*ptr++ = NVHOST_OPCODE_INCR(0x34c, 0x2);
	*ptr++ = 0x00000002;
	*ptr++ = 0x3f000000;
	*ptr++ = NVHOST_OPCODE_NONINCR(0x000, 0x1);
	*ptr++ = 0x00000216;
	*ptr++ = NVHOST_OPCODE_NONINCR(0x000, 0x1);
	*ptr++ = 0x00000216;
	*ptr++ = NVHOST_OPCODE_MASK(0x352, 0x1b);
	*ptr++ = 0x43800000;
	*ptr++ = 0x43800000;
	*ptr++ = 0x43800000;
	*ptr++ = 0x43800000;
	*ptr++ = NVHOST_OPCODE_INCR(0x358, 0x03);
	*ptr++ = 0x4376f000;
	*ptr++ = 0x4376f000;
	*ptr++ = 0x40dfae14;
	*ptr++ = NVHOST_OPCODE_INCR(0x343, 0x01);
	*ptr++ = 0xb8e00000;
	*ptr++ = NVHOST_OPCODE_INCR(0x350, 0x02);
	*ptr++ = 0x00000020;
	*ptr++ = 0x00000020;
	*ptr++ = NVHOST_OPCODE_INCR(0xe11, 0x01);
	*ptr++ = 0x04004019;
	*ptr++ = NVHOST_OPCODE_INCR(0x903, 0x01);
	*ptr++ = 0x00000002;
	*ptr++ = NVHOST_OPCODE_INCR(0xe15, 0x01);
	*ptr++ = 0x08000001;
	*ptr++ = NVHOST_OPCODE_INCR(0xe16, 0x01);
	*ptr++ = 0x08000001;
	*ptr++ = NVHOST_OPCODE_INCR(0xe17, 0x01);
	*ptr++ = 0x08000001;
	*ptr++ = NVHOST_OPCODE_INCR(0xe18, 0x01);
	*ptr++ = 0x08000001;
	*ptr++ = NVHOST_OPCODE_INCR(0xe19, 0x01);
	*ptr++ = 0x08000001;
	*ptr++ = NVHOST_OPCODE_INCR(0xe1a, 0x01);
	*ptr++ = 0x08000001;
	*ptr++ = NVHOST_OPCODE_INCR(0xe1b, 0x01);
	*ptr++ = 0x08000001;
	*ptr++ = NVHOST_OPCODE_INCR(0xe19, 0x01);
	*ptr++ = 0x08000001;
	*ptr++ = NVHOST_OPCODE_INCR(0xe10, 0x01);
	*ptr++ = 0x0c000000;
	*ptr++ = NVHOST_OPCODE_INCR(0xe13, 0x01);
	*ptr++ = 0x0c000000;
	*ptr++ = NVHOST_OPCODE_INCR(0xe12, 0x01);
	*ptr++ = 0x0c000000;
	*ptr++ = NVHOST_OPCODE_INCR(0x348, 0x04);
	*ptr++ = 0x3f800000;
	*ptr++ = 0x00000000;
	*ptr++ = 0x00000000;
	*ptr++ = 0x3f800000;
	*ptr++ = NVHOST_OPCODE_IMM(0xe27, 0x01);
	*ptr++ = NVHOST_OPCODE_INCR(0xa02, 0x06);
	*ptr++ = 0x000001ff;
	*ptr++ = 0x000001ff;
	*ptr++ = 0x000001ff;
	*ptr++ = 0x00000030;
	*ptr++ = 0x00000020;
	*ptr++ = 0x00000030;
	*ptr++ = NVHOST_OPCODE_IMM(0xa00, 0xe00);
	*ptr++ = NVHOST_OPCODE_IMM(0xa08, 0x100);
	*ptr++ = NVHOST_OPCODE_INCR(0xa02, 0x06);
	*ptr++ = 0x000001ff;
	*ptr++ = 0x000001ff;
	*ptr++ = 0x000001ff;
	*ptr++ = 0x00000030;
	*ptr++ = 0x00000020;
	*ptr++ = 0x00000030;
	*ptr++ = NVHOST_OPCODE_IMM(0xa00, 0xe00);
	*ptr++ = NVHOST_OPCODE_IMM(0xa08, 0x100);
	*ptr++ = NVHOST_OPCODE_IMM(0x403, 0x610);
	*ptr++ = NVHOST_OPCODE_INCR(0xa02, 0x6);
	*ptr++ = 0x000001ff;
	*ptr++ = 0x000001ff;
	*ptr++ = 0x000001ff;
	*ptr++ = 0x00000030;
	*ptr++ = 0x00000020;
	*ptr++ = 0x00000030;
	*ptr++ = NVHOST_OPCODE_IMM(0xa00, 0xe00);
	*ptr++ = NVHOST_OPCODE_IMM(0xa08, 0x100);
	*ptr++ = NVHOST_OPCODE_INCR(0xa02, 0x06);
	*ptr++ = 0x000001ff;
	*ptr++ = 0x000001ff;
	*ptr++ = 0x000001ff;
	*ptr++ = 0x00000030;
	*ptr++ = 0x00000020;
	*ptr++ = 0x00000030;
	*ptr++ = NVHOST_OPCODE_IMM(0xa00, 0xe00);
	*ptr++ = NVHOST_OPCODE_IMM(0xa08, 0x100);
	*ptr++ = NVHOST_OPCODE_NONINCR(0x000, 0x01);
	*ptr++ = 0x00000216;
	*ptr++ = NVHOST_OPCODE_NONINCR(0x000, 0x01);
	*ptr++ = 0x00000216;

	/*
	  Command Buffer:
	    mem: e462ae40, offset: 0, words: 10
	    commands: 10
	*/

	*ptr++ = NVHOST_OPCODE_IMM(0x205, 0x00);
	*ptr++ = NVHOST_OPCODE_NONINCR(0x206, 0x08);
	*ptr++ = 0x401f9c6c;
	*ptr++ = 0x0040000d;
	*ptr++ = 0x8106c083;
	*ptr++ = 0x6041ff80;
	*ptr++ = 0x401f9c6c;
	*ptr++ = 0x0040010d;
	*ptr++ = 0x8106c083;
	*ptr++ = 0x6041ff9d;

	/*
	  Command Buffer:
	    mem: e462a7c0, offset: 3ce8, words: 16
	    commands: 16
	*/

	*ptr++ = NVHOST_OPCODE_INCR(0x343, 0x01);
	*ptr++ = 0xb8e00000;
	*ptr++ = NVHOST_OPCODE_INCR(0x300, 0x02);
	*ptr++ = 0x00000008;
	*ptr++ = 0x0000fecd;
	*ptr++ = NVHOST_OPCODE_INCR(0xe20, 0x01);
	*ptr++ = 0x58000000;
	*ptr++ = NVHOST_OPCODE_IMM(0x503, 0x00);
	*ptr++ = NVHOST_OPCODE_IMM(0x545, 0x00);
	*ptr++ = NVHOST_OPCODE_IMM(0xe22, 0x00);
	*ptr++ = NVHOST_OPCODE_IMM(0x603, 0x00);
	*ptr++ = NVHOST_OPCODE_IMM(0x803, 0x00);
	*ptr++ = NVHOST_OPCODE_INCR(0x520, 0x01);
	*ptr++ = 0x20006001;
	*ptr++ = NVHOST_OPCODE_INCR(0x546, 0x01);
	*ptr++ = 0x00000040;

	/*
	  Command Buffer:
	    mem: e462aec0, offset: 10, words: 26
	    commands: 26
	*/

	*ptr++ = NVHOST_OPCODE_NONINCR(0x541, 0x01);
	*ptr++ = 0x00000000;
	*ptr++ = NVHOST_OPCODE_INCR(0x500, 0x01);
	*ptr++ = 0x00000000;
	*ptr++ = NVHOST_OPCODE_NONINCR(0x601, 0x01);
	*ptr++ = 0x00000001;
	*ptr++ = NVHOST_OPCODE_NONINCR(0x604, 0x02);
	*ptr++ = 0x104e51ba;
	*ptr++ = 0x00408102;
	*ptr++ = NVHOST_OPCODE_NONINCR(0x701, 0x01);
	*ptr++ = 0x00000000;
	*ptr++ = NVHOST_OPCODE_NONINCR(0x801, 0x01);
	*ptr++ = 0x00000001;
	*ptr++ = NVHOST_OPCODE_NONINCR(0x804, 0x08);
	*ptr++ = 0x0001c0c0;
	*ptr++ = 0x3f41f200;
	*ptr++ = 0x0001a080;
	*ptr++ = 0x3f41f200;
	*ptr++ = 0x00014000;
	*ptr++ = 0x3f41f200;
	*ptr++ = 0x00012040;
	*ptr++ = 0x3f41f200;
	*ptr++ = NVHOST_OPCODE_NONINCR(0x806, 0x01);
	*ptr++ = 0x00000000;
	*ptr++ = NVHOST_OPCODE_NONINCR(0x901, 0x01);
	*ptr++ = 0x00028005;

	/*
	  Command Buffer:
	    mem: e462a7c0, offset: 3d28, words: 66
	    commands: 66
	*/

	*ptr++ = NVHOST_OPCODE_INCR(0xa02, 0x06);
	*ptr++ = 0x000001ff;
	*ptr++ = 0x000001ff;
	*ptr++ = 0x000001ff;
	*ptr++ = 0x00000030;
	*ptr++ = 0x00000020;
	*ptr++ = 0x00000030;
	*ptr++ = NVHOST_OPCODE_IMM(0xa00, 0xe00);
	*ptr++ = NVHOST_OPCODE_IMM(0xa08, 0x100);
	*ptr++ = NVHOST_OPCODE_IMM(0x40c, 0x06);
	*ptr++ = NVHOST_OPCODE_INCR(0x120, 0x01);
	*ptr++ = 0x00030081;
	*ptr++ = NVHOST_OPCODE_SETCL(0x00, 0x01, 0x00);
	*ptr++ = NVHOST_OPCODE_NONINCR(0x008, 0x01);
	*ptr++ = 0x120000b1;
	*ptr++ = NVHOST_OPCODE_SETCL(0x00, 0x60, 0x00);
	*ptr++ = NVHOST_OPCODE_INCR(0x344, 0x02);
	*ptr++ = 0x00000000;
	*ptr++ = 0x00000000;
	*ptr++ = NVHOST_OPCODE_SETCL(0x00, 0x60, 0x00);
	*ptr++ = NVHOST_OPCODE_NONINCR(0x000, 0x01);
	*ptr++ = 0x00000116;
	*ptr++ = NVHOST_OPCODE_SETCL(0x00, 0x01, 0x00);
	*ptr++ = NVHOST_OPCODE_NONINCR(0x009, 0x01);
	*ptr++ = 0x16030005;
	*ptr++ = NVHOST_OPCODE_SETCL(0x00, 0x60, 0x00);
	*ptr++ = NVHOST_OPCODE_IMM(0xa00, 0xe01);
	*ptr++ = NVHOST_OPCODE_SETCL(0x00, 0x60, 0x00);
	*ptr++ = NVHOST_OPCODE_NONINCR(0x00, 0x01);
	*ptr++ = 0x00000216;
	*ptr++ = NVHOST_OPCODE_SETCL(0x00, 0x01, 0x00);
	*ptr++ = NVHOST_OPCODE_NONINCR(0x009, 0x01);
	*ptr++ = 0x16030006;
	*ptr++ = NVHOST_OPCODE_SETCL(0x00, 0x60, 0x00);
	*ptr++ = NVHOST_OPCODE_INCR(0xe01, 0x01);

	gr3d->relocs[0].cmdbuf_mem = gr3d->buffer->id;
	gr3d->relocs[0].cmdbuf_offset = get_offset(gr3d->buffer, ptr);
	gr3d->relocs[0].target_mem = /* TODO */;
	gr3d->relocs[0].target_offset = 0;
	gr3d->shifts[0].shift = 0;

	*ptr++ = 0xdeadbeef;
	*ptr++ = NVHOST_OPCODE_INCR(0xe31, 0x01);
	*ptr++ = 0x00000000;
	*ptr++ = NVHOST_OPCODE_INCR(0x100, 0x01);

	gr3d->relocs[1].cmdbuf_mem = gr3d->buffer->id;
	gr3d->relocs[1].cmdbuf_offset = get_offset(gr3d->buffer, ptr);
	gr3d->relocs[1].target_mem = /* TODO */;
	gr3d->relocs[1].target_offset = 0;
	gr3d->shifts[1].shift = 0;

	*ptr++ = 0xdeadbeef;
	*ptr++ = NVHOST_OPCODE_INCR(0x102, 0x01);

	gr3d->relocs[2].cmdbuf_mem = gr3d->buffer->id;
	gr3d->relocs[2].cmdbuf_offset = get_offset(gr3d->buffer, ptr);
	gr3d->relocs[2].target_mem = /* TODO */;
	gr3d->relocs[2].target_offset = 0;
	gr3d->shifts[2].shift = 0;

	*ptr++ = 0xdeadbeef;
	*ptr++ = NVHOST_OPCODE_INCR(0x121, 0x03);

	gr3d->relocs[3].cmdbuf_mem = gr3d->buffer->id;
	gr3d->relocs[3].cmdbuf_offset = get_offset(gr3d->buffer, ptr);
	gr3d->relocs[3].target_mem = /* TODO */;
	gr3d->relocs[3].target_offset = 0;
	gr3d->shifts[3].shift = 0;

	*ptr++ = 0xdeadbeef;
	*ptr++ = 0xec000000;
	*ptr++ = 0x00200000;
	*ptr++ = NVHOST_OPCODE_IMM(0xe27, 0x02);
	*ptr++ = NVHOST_OPCODE_NONINCR(0x000, 0x01);
	*ptr++ = 0x00000216;
	*ptr++ = NVHOST_OPCODE_SETCL(0x00, 0x60, 0x00);
	*ptr++ = NVHOST_OPCODE_NONINCR(0x000, 0x01);
	*ptr++ = 0x00000116;
	*ptr++ = NVHOST_OPCODE_SETCL(0x00, 0x01, 0x00);
	*ptr++ = NVHOST_OPCODE_NONINCR(0x09, 0x01);
	*ptr++ = 0x16030008;
	*ptr++ = NVHOST_OPCODE_SETCL(0x00, 0x60, 0x00);
	*ptr++ = NVHOST_OPCODE_SETCL(0x00, 0x01, 0x00);
	*ptr++ = NVHOST_OPCODE_NONINCR(0x0c, 0x01);
	*ptr++ = 0x03000008;
	*ptr++ = NVHOST_OPCODE_SETCL(0x00, 0x60, 0x00);
	*ptr++ = NVHOST_OPCODE_NONINCR(0x00, 0x01);
	*ptr++ = 0x00000116;
	*ptr++ = NVHOST_OPCODE_SETCL(0x00, 0x01, 0x00);
	*ptr++ = NVHOST_OPCODE_NONINCR(0x0c, 0x01);
	*ptr++ = 0x03000001;
	*ptr++ = NVHOST_OPCODE_SETCL(0x00, 0x60, 0x00);

	gr3d->words = ptr - (uint32_t *)gr3d->buffer->ptr;
	length = gr3d->words * 4;

	err = nvmap_handle_writeback_invalidate(gr3d->nvmap, gr3d->buffer, 0,
						length);
	if (err < 0) {
	}

	err = nvhost_gr3d_submit(gr3d, &fence);
	if (err < 0) {
	}

	err = nvhost_gr3d_wait(gr3d, fence, -1);
	if (err < 0) {
	}
}

int main(int argc, char *argv[])
{
	struct nvmap_handle *buffer;
	unsigned int width, height;
	struct nvhost_gr3d *gr3d;
	struct nvhost_ctrl *ctrl;
	struct nvmap *nvmap;
	int err;

	width = height = 32;

	nvmap = nvmap_open();
	if (!nvmap) {
		return 1;
	}

	ctrl = nvhost_ctrl_open();
	if (!ctrl) {
		return 1;
	}

	buffer = nvmap_handle_create(nvmap, width * height * 4);
	if (!buffer) {
		return 1;
	}

	err = nvmap_handle_alloc(nvmap, buffer, 1 << 0, 0x100);
	if (err < 0) {
		return 1;
	}

	gr3d = nvhost_gr3d_open(nvmap, ctrl);
	if (!gr3d) {
		return 1;
	}

	gr3d->fb = buffer;

	nvhost_gr3d_triangle(gr3d);

	if (1) {
		struct nvmap_handle_param args;

		memset(&args, 0, sizeof(args));
		args.handle = buffer->id;
		args.param = 1;

		err = ioctl(nvmap->fd, NVMAP_IOCTL_PARAM, &args);
		if (err < 0) {
		}
	}

	err = nvmap_handle_mmap(nvmap, buffer);
	if (err < 0) {
		return 1;
	}

	err = nvmap_handle_invalidate(nvmap, buffer, 0, buffer->size);
	if (err < 0) {
		return 1;
	}

	if (1) {
		const char *filename = "test.png";
		png_structp png;
		png_bytep *rows;
		png_infop info;
		unsigned int i;
		size_t stride;
		FILE *fp;

		fp = fopen(filename, "wb");
		if (!fp) {
			fprintf(stderr, "failed to write `%s'\n", filename);
			return 1;
		}

		png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (!png) {
			return 1;
		}

		info = png_create_info_struct(png);
		if (!info) {
			return 1;
		}

		if (setjmp(png_jmpbuf(png))) {
			return 1;
		}

		png_init_io(png, fp);

		if (setjmp(png_jmpbuf(png))) {
			return 1;
		}

		png_set_IHDR(png, info, 32, 32, 8, PNG_COLOR_TYPE_RGBA,
			     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
			     PNG_FILTER_TYPE_BASE);
		png_write_info(png, info);

		if (setjmp(png_jmpbuf(png))) {
			fprintf(stderr, "failed to write IHDR\n");
			return 1;
		}

		stride = 32 * (32 / 8);

		rows = malloc(32 * sizeof(png_bytep));
		if (!rows) {
			fprintf(stderr, "out-of-memory\n");
			return 1;
		}

		for (i = 0; i < 32; i++)
			rows[32 - i - 1] = buffer->ptr + i * stride;

		png_write_image(png, rows);

		free(rows);

		if (setjmp(png_jmpbuf(png))) {
			return 1;
		}

		png_write_end(png, NULL);

		fclose(fp);
	}

	nvhost_gr3d_close(gr3d);
	nvmap_handle_free(nvmap, buffer);
	nvhost_ctrl_close(ctrl);
	nvmap_close(nvmap);

	return 0;
}
