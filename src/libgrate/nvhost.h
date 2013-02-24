#ifndef GRATE_NVHOST_H
#define GRATE_NVHOST_H 1

#include "nvmap.h"

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

struct nvhost_ctrl {
	int fd;
};

struct nvhost_ctrl *nvhost_ctrl_open(void);
void nvhost_ctrl_close(struct nvhost_ctrl *ctrl);

struct nvhost_pushbuf_reloc {
	unsigned long source_offset;
	unsigned long target_handle;
	unsigned long target_offset;
	unsigned long shift;
};

struct nvhost_pushbuf {
	struct nvmap_handle *handle;
	unsigned long offset;
	unsigned long length;

	struct nvhost_pushbuf_reloc *relocs;
	unsigned long num_relocs;

	uint32_t *ptr;
};

struct nvhost_pushbuf *nvhost_pushbuf_create(struct nvmap_handle *handle,
					     unsigned long offset);

struct nvhost_job {
	uint32_t syncpt;
	uint32_t syncpt_incrs;

	struct nvhost_pushbuf *pushbufs;
	unsigned int num_pushbufs;
};

struct nvhost_job *nvhost_job_create(uint32_t syncpt, uint32_t increments);
void nvhost_job_free(struct nvhost_job *job);
struct nvhost_pushbuf *nvhost_job_append(struct nvhost_job *job,
					 struct nvmap_handle *handle,
					 unsigned long offset);
int nvhost_pushbuf_push(struct nvhost_pushbuf *pb, uint32_t word);
int nvhost_pushbuf_relocate(struct nvhost_pushbuf *pb,
			    struct nvmap_handle *target, unsigned long offset,
			    unsigned long shift);

struct nvhost_client {
	struct nvhost_ctrl *ctrl;
	struct nvmap *nvmap;
	uint32_t syncpt;
	int fd;
};

int nvhost_client_init(struct nvhost_client *client, struct nvmap *nvmap,
		       struct nvhost_ctrl *ctrl, uint32_t syncpt, int fd);
void nvhost_client_exit(struct nvhost_client *client);
int nvhost_client_submit(struct nvhost_client *client, struct nvhost_job *job);
int nvhost_client_flush(struct nvhost_client *client, uint32_t *fencep);
int nvhost_client_wait(struct nvhost_client *client, uint32_t fence,
		       uint32_t timeout);

#endif
