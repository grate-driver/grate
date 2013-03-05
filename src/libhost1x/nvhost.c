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

#define NVHOST_IOCTL_MAGIC 'H'

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

int nvhost_ctrl_read_syncpt(struct nvhost_ctrl *ctrl, uint32_t syncpt,
			    uint32_t *value)
{
	struct nvhost_ctrl_syncpt_read_args args;
	int err;

	if (!ctrl || !value)
		return -EINVAL;

	memset(&args, 0, sizeof(args));
	args.id = syncpt;

	err = ioctl(ctrl->fd, NVHOST_IOCTL_CTRL_SYNCPT_READ, &args);
	if (err < 0)
		return -errno;

	*value = args.value;

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

#define NVHOST_IOCTL_CHANNEL_FLUSH           _IOR(NVHOST_IOCTL_MAGIC,  1, struct nvhost_get_param_args)
#define NVHOST_IOCTL_CHANNEL_GET_SYNCPOINTS  _IOR(NVHOST_IOCTL_MAGIC,  2, struct nvhost_get_param_args)
#define NVHOST_IOCTL_CHANNEL_GET_WAITBASES   _IOR(NVHOST_IOCTL_MAGIC,  3, struct nvhost_get_param_args)
#define NVHOST_IOCTL_CHANNEL_GET_MODMUTEXES  _IOR(NVHOST_IOCTL_MAGIC,  4, struct nvhost_get_param_args)
#define NVHOST_IOCTL_CHANNEL_SET_NVMAP_FD    _IOW(NVHOST_IOCTL_MAGIC,  5, struct nvhost_set_nvmap_fd_args)
#define NVHOST_IOCTL_CHANNEL_NULL_KICKOFF    _IOR(NVHOST_IOCTL_MAGIC,  6, struct nvhost_get_param_args)
#define NVHOST_IOCTL_CHANNEL_SUBMIT_EXT      _IOW(NVHOST_IOCTL_MAGIC,  7, struct nvhost_submit_hdr_ext)
#define NVHOST_IOCTL_GET_TIMEDOUT            _IOR(NVHOST_IOCTL_MAGIC, 12, struct nvhost_get_param_args)
#define NVHOST_IOCTL_SET_PRIORITY            _IOW(NVHOST_IOCTL_MAGIC, 13, struct nvhost_set_priority_args)

static int nvhost_client_syncpt_init(struct nvhost_client *client)
{
	struct host1x_client *base = &client->base;
	struct nvhost_ctrl *ctrl = client->ctrl;
	struct nvhost_get_param_args args;
	struct host1x_syncpt *syncpt;
	unsigned int i, j;
	int err;

	memset(&args, 0, sizeof(args));

	err = ioctl(client->fd, NVHOST_IOCTL_CHANNEL_GET_SYNCPOINTS, &args);
	if (err < 0)
		return -errno;

	for (i = 0; i < 32; i++)
		if (args.value & (1 << i))
			base->num_syncpts++;

	base->syncpts = calloc(base->num_syncpts, sizeof(*syncpt));
	if (!base->syncpts)
		return -ENOMEM;

	//printf("%u syncpoints: %08x\n", base->num_syncpts, args.value);

	for (i = 0, j = 0; i < 32; i++) {
		if (args.value & (1 << i)) {
			syncpt = &base->syncpts[j++];
			syncpt->id = i;

			err = nvhost_ctrl_read_syncpt(ctrl, i, &syncpt->value);
			if (err < 0) {
				fprintf(stderr, "failed to read syncpt: %d\n",
					err);
				continue;
			}

			//printf("  %u: %u\n", i, syncpt->value);
		}
	}

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

static int nvhost_client_submit(struct host1x_client *client,
				struct host1x_job *job)
{
	struct nvhost_client *nvhost = to_nvhost_client(client);
	struct nvhost_submit_hdr_ext args;
	unsigned long num_relocs = 0;
	unsigned int i, j;
	int err;

	for (i = 0; i < job->num_pushbufs; i++) {
		struct host1x_pushbuf *pb = &job->pushbufs[i];
		struct nvhost_bo *bo = to_nvhost_bo(pb->bo);
		struct nvmap_handle *handle = bo->handle;
		unsigned long length = pb->length * 4;

		err = nvmap_handle_writeback_invalidate(nvhost->nvmap, handle,
							pb->offset, length);
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

	err = ioctl(nvhost->fd, NVHOST_IOCTL_CHANNEL_SUBMIT_EXT, &args);
	if (err < 0) {
		fprintf(stderr, "NVHOST_IOCTL_CHANNEL_SUBMIT_EXT: %d\n",
			errno);
		return -errno;
	}

	for (i = 0; i < job->num_pushbufs; i++) {
		struct host1x_pushbuf *pb = &job->pushbufs[i];
		struct nvhost_bo *bo = to_nvhost_bo(pb->bo);
		struct nvhost_cmdbuf cmdbuf;

		memset(&cmdbuf, 0, sizeof(cmdbuf));
		cmdbuf.mem = bo->handle->id;
		cmdbuf.offset = pb->offset;
		cmdbuf.words = pb->length;

		err = write(nvhost->fd, &cmdbuf, sizeof(cmdbuf));
		if (err < 0) {
		}
	}

	for (i = 0; i < job->num_pushbufs; i++) {
		struct host1x_pushbuf *pb = &job->pushbufs[i];

		for (j = 0; j < pb->num_relocs; j++) {
			struct host1x_pushbuf_reloc *r = &pb->relocs[j];
			struct nvhost_bo *bo = to_nvhost_bo(pb->bo);
			struct nvhost_reloc reloc;

			memset(&reloc, 0, sizeof(reloc));
			reloc.cmdbuf_mem = bo->handle->id;
			reloc.cmdbuf_offset = r->source_offset;
			reloc.target_mem = r->target_handle;
			reloc.target_offset = r->target_offset;

			err = write(nvhost->fd, &reloc, sizeof(reloc));
			if (err < 0) {
			}
		}
	}

	for (i = 0; i < job->num_pushbufs; i++) {
		struct host1x_pushbuf *pb = &job->pushbufs[i];

		for (j = 0; j < pb->num_relocs; j++) {
			struct host1x_pushbuf_reloc *reloc = &pb->relocs[j];
			struct nvhost_reloc_shift shift;

			memset(&shift, 0, sizeof(shift));
			shift.shift = reloc->shift;

			err = write(nvhost->fd, &shift, sizeof(shift));
			if (err < 0) {
			}
		}
	}

	return 0;
}

static int nvhost_client_flush(struct host1x_client *client, uint32_t *fencep)
{
	struct nvhost_client *nvhost = to_nvhost_client(client);
	struct nvhost_get_param_args fence;
	int err;

	err = ioctl(nvhost->fd, NVHOST_IOCTL_CHANNEL_FLUSH, &fence);
	if (err < 0)
		return -errno;

	*fencep = fence.value;

	return 0;
}

static int nvhost_client_wait(struct host1x_client *client, uint32_t fence,
			      uint32_t timeout)
{
	struct nvhost_client *nvhost = to_nvhost_client(client);
	struct host1x_syncpt *syncpt = &client->syncpts[0];
	struct nvhost_ctrl_syncpt_waitex_args args;
	int err;

	memset(&args, 0, sizeof(args));
	args.id = syncpt->id;
	args.thresh = fence;
	args.timeout = timeout;

	err = ioctl(nvhost->ctrl->fd, NVHOST_IOCTL_CTRL_SYNCPT_WAITEX, &args);
	if (err < 0)
		return -errno;

	if (args.value != args.thresh)
		fprintf(stderr, "syncpt %u: value:%u != thresh:%u\n",
			args.id, args.value, args.thresh);

	return 0;
}

int nvhost_client_init(struct nvhost_client *client, struct nvmap *nvmap,
		       struct nvhost_ctrl *ctrl, int fd)
{
	struct nvhost_set_nvmap_fd_args args;
	struct nvhost_get_param_args param;
	int err;

	client->nvmap = nvmap;
	client->ctrl = ctrl;
	client->fd = fd;

	memset(&args, 0, sizeof(args));
	args.fd = nvmap->fd;

	err = ioctl(fd, NVHOST_IOCTL_CHANNEL_SET_NVMAP_FD, &args);
	if (err < 0)
		return -errno;

	memset(&param, 0, sizeof(param));

	err = ioctl(fd, NVHOST_IOCTL_CHANNEL_GET_WAITBASES, &param);
	if (err < 0)
		return -errno;

	//printf("waitbases: %08x\n", param.value);

	err = nvhost_client_syncpt_init(client);
	if (err < 0)
		return err;

	client->base.submit = nvhost_client_submit;
	client->base.flush = nvhost_client_flush;
	client->base.wait = nvhost_client_wait;

	return 0;
}

void nvhost_client_exit(struct nvhost_client *client)
{
	close(client->fd);
}
