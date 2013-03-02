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

#ifndef GRATE_NVHOST_H
#define GRATE_NVHOST_H 1

#include <stdint.h>

#include <sys/ioctl.h>

#include "utils.h"

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

void nvhost_register(void);

#endif
