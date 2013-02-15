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
#define NVHOST_OPCODE_NONINCR(offset, count) \
	((0x2 << 28) | (((offset) & 0xfff) << 16) | ((count) & 0xffff))
#define NVHOST_OPCODE_MASK(offset, mask) \
	((0x3 << 28) | (((offset) & 0xfff) << 16) | ((mask) & 0xffff))
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

struct nvhost_gr2d {
	struct nvmap_handle *buffer;
	struct nvmap_handle *fb;
	struct nvhost_ctrl *ctrl;
	struct nvmap *nvmap;
	int fd;

	struct nvhost_reloc relocs[4];
	unsigned int num_relocs;

	struct nvhost_reloc_shift shifts[4];
	unsigned int num_shifts;
};

struct nvhost_gr2d *nvhost_gr2d_open(struct nvmap *nvmap, struct nvhost_ctrl *ctrl)
{
	struct nvhost_set_nvmap_fd_args args;
	struct nvhost_gr2d *gr2d;
	int err;

	gr2d = calloc(1, sizeof(*gr2d));
	if (!gr2d)
		return NULL;

	gr2d->fd = open("/dev/nvhost-gr2d", O_RDWR);
	if (gr2d->fd < 0) {
		free(gr2d);
		return NULL;
	}

	gr2d->nvmap = nvmap;
	gr2d->ctrl = ctrl;

	memset(&args, 0, sizeof(args));
	args.fd = nvmap->fd;

	err = ioctl(gr2d->fd, NVHOST_IOCTL_CHANNEL_SET_NVMAP_FD, &args);
	if (err < 0) {
		close(gr2d->fd);
		free(gr2d);
		return NULL;
	}

	gr2d->buffer = nvmap_handle_create(nvmap, 8 * 4096);
	if (!gr2d->buffer) {
		close(gr2d->fd);
		free(gr2d);
		return NULL;
	}

	err = nvmap_handle_alloc(nvmap, gr2d->buffer, 0x0a000001, 0x20);
	if (err < 0) {
		nvmap_handle_free(nvmap, gr2d->buffer);
		close(gr2d->fd);
		free(gr2d);
		return NULL;
	}

	err = nvmap_handle_mmap(nvmap, gr2d->buffer);
	if (err < 0) {
		nvmap_handle_free(nvmap, gr2d->buffer);
		close(gr2d->fd);
		free(gr2d);
		return NULL;
	}

	return gr2d;
}

void nvhost_gr2d_close(struct nvhost_gr2d *gr2d)
{
	if (gr2d) {
		if (gr2d->fd >= 0)
			close(gr2d->fd);

		nvmap_handle_free(gr2d->nvmap, gr2d->buffer);
	}

	free(gr2d);
}

int nvhost_gr2d_submit(struct nvhost_gr2d *gr2d, uint32_t *fencep)
{
	struct nvhost_get_param_args fence;
	struct nvhost_submit_hdr_ext args;
	struct nvhost_cmdbuf cmdbuf;
	int err;

	memset(&args, 0, sizeof(args));
	args.syncpt_id = 18;
	args.syncpt_incrs = 1;
	args.num_cmdbufs = 1;
	args.num_relocs = gr2d->num_relocs;
	args.submit_version = 2;
	args.num_waitchks = 0;
	args.waitchk_mask = 0;

	err = ioctl(gr2d->fd, NVHOST_IOCTL_CHANNEL_SUBMIT_EXT, &args);
	if (err < 0) {
		fprintf(stderr, "NVHOST_IOCTL_CHANNEL_SUBMIT_EXT: %d\n",
			errno);
		return -errno;
	}

	memset(&cmdbuf, 0, sizeof(cmdbuf));
	cmdbuf.mem = gr2d->buffer->id;
	cmdbuf.offset = 0;
	cmdbuf.words = 23;

	err = write(gr2d->fd, &cmdbuf, sizeof(cmdbuf));
	if (err < 0) {
	}

	err = write(gr2d->fd, gr2d->relocs, sizeof(struct nvhost_reloc) * gr2d->num_relocs);
	if (err < 0) {
	}

	err = write(gr2d->fd, gr2d->shifts, sizeof(struct nvhost_reloc_shift) * gr2d->num_relocs);
	if (err < 0) {
	}

	err = ioctl(gr2d->fd, NVHOST_IOCTL_CHANNEL_FLUSH, &fence);
	if (err < 0)
		return -errno;

	printf("fence: %u\n", fence.value);
	*fencep = fence.value;

	return 0;
}

int nvhost_gr2d_wait(struct nvhost_gr2d *gr2d, uint32_t fence, uint32_t timeout)
{
	struct nvhost_ctrl_syncpt_waitex_args args;
	int err;

	memset(&args, 0, sizeof(args));
	args.id = 18;
	args.thresh = fence;
	args.timeout = timeout;

	err = ioctl(gr2d->ctrl->fd, NVHOST_IOCTL_CTRL_SYNCPT_WAITEX, &args);
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

void nvhost_gr2d_fill(struct nvhost_gr2d *gr2d)
{
	uint32_t *ptr = gr2d->buffer->ptr;
	unsigned long words, length;
	uint32_t fence;
	int err;

	*ptr++ = NVHOST_OPCODE_SETCL(0, 0x51, 0);
	*ptr++ = NVHOST_OPCODE_SETCL(0, 0x51, 0);
	*ptr++ = NVHOST_OPCODE_EXTEND(0, 0x01);
	*ptr++ = NVHOST_OPCODE_MASK(0x09, 9);
	*ptr++ = 0x0000003a;
	*ptr++ = 0x00000000;
	*ptr++ = NVHOST_OPCODE_MASK(0x1e, 7);
	*ptr++ = 0x00000000;
	//*ptr++ = 0x00010044; /* 16-bit depth */
	*ptr++ = 0x00020044; /* 32-bit depth */
	*ptr++ = 0x000000cc;
	*ptr++ = NVHOST_OPCODE_MASK(0x2b, 9);
	*ptr++ = 0xdeadbeef;
	//*ptr++ = 0x00000040; /* 64 byte stride */
	*ptr++ = 0x00000080; /* 128 byte stride */
	*ptr++ = NVHOST_OPCODE_NONINCR(0x35, 1);
	//*ptr++ = 0x0000f800; /* 16-bit red */
	*ptr++ = 0xff0000ff; /* 32-bit red */
	*ptr++ = NVHOST_OPCODE_NONINCR(0x46, 1);
	*ptr++ = 0x00100000;
	*ptr++ = NVHOST_OPCODE_MASK(0x38, 5);
	*ptr++ = 0x00200020;
	*ptr++ = 0x00000000;
	*ptr++ = NVHOST_OPCODE_EXTEND(1, 1);
	*ptr++ = NVHOST_OPCODE_NONINCR(0x00, 1);
	*ptr++ = 0x00000112;

	words = ptr - (uint32_t *)gr2d->buffer->ptr;
	length = words * 4;

	printf("%lu words, %lu bytes\n", words, length);

	err = nvmap_handle_writeback_invalidate(gr2d->nvmap, gr2d->buffer, 0,
						length);
	if (err < 0) {
	}

	gr2d->relocs[0].cmdbuf_mem = gr2d->buffer->id;
	gr2d->relocs[0].cmdbuf_offset = 0x2c;
	gr2d->relocs[0].target_mem = gr2d->fb->id;
	gr2d->relocs[0].target_offset = 0;
	gr2d->shifts[0].shift = 0;

	gr2d->num_relocs = 1;

	err = nvhost_gr2d_submit(gr2d, &fence);
	if (err < 0) {
	}

	err = nvhost_gr2d_wait(gr2d, fence, -1);
	if (err < 0) {
	}
}

int main(int argc, char *argv[])
{
	struct nvmap_handle *buffer;
	unsigned int width, height;
	struct nvhost_gr2d *gr2d;
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

	gr2d = nvhost_gr2d_open(nvmap, ctrl);
	if (!gr2d) {
		return 1;
	}

	gr2d->fb = buffer;

	nvhost_gr2d_fill(gr2d);

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

	nvhost_gr2d_close(gr2d);
	nvmap_handle_free(nvmap, buffer);
	nvhost_ctrl_close(ctrl);
	nvmap_close(nvmap);

	return 0;
}
