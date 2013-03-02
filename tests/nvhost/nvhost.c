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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include "nvhost.h"

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

int nvhost_client_init(struct nvhost_client *client, struct nvmap *nvmap,
		       struct nvhost_ctrl *ctrl, uint32_t syncpt, int fd)
{
	struct nvhost_set_nvmap_fd_args args;
	int err;

	memset(&args, 0, sizeof(args));
	args.fd = nvmap->fd;

	err = ioctl(fd, NVHOST_IOCTL_CHANNEL_SET_NVMAP_FD, &args);
	if (err < 0)
		return -errno;

	client->syncpt = syncpt;
	client->nvmap = nvmap;
	client->ctrl = ctrl;
	client->fd = fd;

	return 0;
}

void nvhost_client_exit(struct nvhost_client *client)
{
	close(client->fd);
}

struct nvhost_job *nvhost_job_create(uint32_t syncpt, uint32_t increments)
{
	struct nvhost_job *job;

	job = calloc(1, sizeof(*job));
	if (!job)
		return NULL;

	job->syncpt = syncpt;
	job->syncpt_incrs = increments;

	return job;
}

void nvhost_job_free(struct nvhost_job *job)
{
	unsigned int i;

	for (i = 0; i < job->num_pushbufs; i++) {
		struct nvhost_pushbuf *pb = &job->pushbufs[i];
		free(pb->relocs);
	}

	free(job->pushbufs);
	free(job);
}

struct nvhost_pushbuf *nvhost_job_append(struct nvhost_job *job,
					 struct nvmap_handle *handle,
					 unsigned long offset)
{
	struct nvhost_pushbuf *pb;
	size_t size;

	if (!handle->ptr)
		return NULL;

	size = (job->num_pushbufs + 1) * sizeof(*pb);

	pb = realloc(job->pushbufs, size);
	if (!pb)
		return NULL;

	job->pushbufs = pb;

	pb = &job->pushbufs[job->num_pushbufs++];
	memset(pb, 0, sizeof(*pb));

	pb->ptr = handle->ptr + offset;
	pb->handle = handle;
	pb->offset = offset;

	return pb;
}

int nvhost_pushbuf_push(struct nvhost_pushbuf *pb, uint32_t word)
{
	*pb->ptr++ = word;
	pb->length++;

	return 0;
}

int nvhost_pushbuf_relocate(struct nvhost_pushbuf *pb,
			    struct nvmap_handle *target, unsigned long offset,
			    unsigned long shift)
{
	struct nvhost_pushbuf_reloc *reloc;
	size_t size;

	size = (pb->num_relocs + 1) * sizeof(*reloc);

	reloc = realloc(pb->relocs, size);
	if (!reloc)
		return -ENOMEM;

	pb->relocs = reloc;

	reloc = &pb->relocs[pb->num_relocs++];

	reloc->source_offset = nvmap_handle_get_offset(pb->handle, pb->ptr);
	reloc->target_handle = target->id;
	reloc->target_offset = offset;
	reloc->shift = shift;

	return 0;
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

int nvhost_client_submit(struct nvhost_client *client, struct nvhost_job *job)
{
	struct nvhost_submit_hdr_ext args;
	unsigned long num_relocs = 0;
	unsigned int i, j;
	int err;

	for (i = 0; i < job->num_pushbufs; i++) {
		struct nvhost_pushbuf *pb = &job->pushbufs[i];
		unsigned long length = pb->length * 4;

		err = nvmap_handle_writeback_invalidate(client->nvmap,
							pb->handle, pb->offset,
							length);
		if (err < 0) {
		}

		num_relocs += pb->num_relocs;
	}

	memset(&args, 0, sizeof(args));
	args.syncpt_id = job->syncpt;
	args.syncpt_incrs = job->syncpt_incrs;
	args.num_cmdbufs = job->num_pushbufs;
	args.num_relocs = num_relocs;
	args.submit_version = 2;
	/* XXX: implement wait checks */
	args.num_waitchks = 0;
	args.waitchk_mask = 0;

	err = ioctl(client->fd, NVHOST_IOCTL_CHANNEL_SUBMIT_EXT, &args);
	if (err < 0) {
		fprintf(stderr, "NVHOST_IOCTL_CHANNEL_SUBMIT_EXT: %d\n",
			errno);
		return -errno;
	}

	for (i = 0; i < job->num_pushbufs; i++) {
		struct nvhost_pushbuf *pb = &job->pushbufs[i];
		struct nvhost_cmdbuf cmdbuf;

		memset(&cmdbuf, 0, sizeof(cmdbuf));
		cmdbuf.mem = pb->handle->id;
		cmdbuf.offset = pb->offset;
		cmdbuf.words = pb->length;

		err = write(client->fd, &cmdbuf, sizeof(cmdbuf));
		if (err < 0) {
		}
	}

	for (i = 0; i < job->num_pushbufs; i++) {
		struct nvhost_pushbuf *pb = &job->pushbufs[i];

		for (j = 0; j < pb->num_relocs; j++) {
			struct nvhost_pushbuf_reloc *r = &pb->relocs[j];
			struct nvhost_reloc reloc;

			memset(&reloc, 0, sizeof(reloc));
			reloc.cmdbuf_mem = pb->handle->id;
			reloc.cmdbuf_offset = r->source_offset;
			reloc.target_mem = r->target_handle;
			reloc.target_offset = r->target_offset;

			err = write(client->fd, &reloc, sizeof(reloc));
			if (err < 0) {
			}
		}
	}

	for (i = 0; i < job->num_pushbufs; i++) {
		struct nvhost_pushbuf *pb = &job->pushbufs[i];

		for (j = 0; j < pb->num_relocs; j++) {
			struct nvhost_pushbuf_reloc *reloc = &pb->relocs[j];
			struct nvhost_reloc_shift shift;

			memset(&shift, 0, sizeof(shift));
			shift.shift = reloc->shift;

			err = write(client->fd, &shift, sizeof(shift));
			if (err < 0) {
			}
		}
	}

	return 0;
}

int nvhost_client_flush(struct nvhost_client *client, uint32_t *fencep)
{
	struct nvhost_get_param_args fence;
	int err;

	err = ioctl(client->fd, NVHOST_IOCTL_CHANNEL_FLUSH, &fence);
	if (err < 0)
		return -errno;

	*fencep = fence.value;

	return 0;
}

int nvhost_client_wait(struct nvhost_client *client, uint32_t fence,
		       uint32_t timeout)
{
	struct nvhost_ctrl_syncpt_waitex_args args;
	int err;

	memset(&args, 0, sizeof(args));
	args.id = client->syncpt;
	args.thresh = fence;
	args.timeout = timeout;

	err = ioctl(client->ctrl->fd, NVHOST_IOCTL_CTRL_SYNCPT_WAITEX, &args);
	if (err < 0)
		return -errno;

	if (args.value != args.thresh)
		fprintf(stderr, "syncpt %u: value:%u != thresh:%u\n",
			args.id, args.value, args.thresh);

	return 0;
}
