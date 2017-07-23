/*
 * Copyright (c) 2017 Dmitry Osipenko
 * Copyright (c) 2017 Erik Faye-Lund
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

#include <string.h>

#include "host1x-private.h"

static int host1x_dummy_bo_mmap(struct host1x_bo *bo)
{
	return 0;
}

static void host1x_dummy_bo_free(struct host1x_bo *bo)
{
	free(bo->ptr);
	free(bo);
}

static struct host1x_bo *host1x_dummy_bo_clone(struct host1x_bo *bo)
{
	struct host1x_bo *clone = malloc(sizeof(*bo));

	if (!clone)
		return NULL;

	memcpy(clone, bo, sizeof(*bo));

	return clone;
}

static struct host1x_bo *host1x_dummy_bo_create(struct host1x *host1x,
						struct host1x_bo_priv *priv,
						size_t size,
						unsigned long flags)
{
	struct host1x_bo *bo;

	bo = calloc(1, sizeof(*bo));
	if (!bo)
		return NULL;

	bo->ptr = malloc(size);
	if (!bo->ptr) {
		free(bo);
		return NULL;
	}

	bo->priv = priv;
	bo->priv->mmap = host1x_dummy_bo_mmap;
	bo->priv->free = host1x_dummy_bo_free;
	bo->priv->clone = host1x_dummy_bo_clone;

	return bo;
}

static void host1x_dummy_close(struct host1x *host1x)
{
	free(host1x);
}

static int host1x_dummy_submit(struct host1x_client *client,
			       struct host1x_job *job)
{
	return 0;
}

static int host1x_dummy_flush(struct host1x_client *client, uint32_t *fence)
{
	return 0;
}

static int host1x_dummy_wait(struct host1x_client *client, uint32_t fence,
			     uint32_t timeout)
{
	return 0;
}

static struct host1x_syncpt syncpt;

static struct host1x_client dummy_client = {
	.submit = host1x_dummy_submit,
	.flush = host1x_dummy_flush,
	.wait = host1x_dummy_wait,
	.syncpts = &syncpt,
	.num_syncpts = 1,
};

static struct host1x_gr2d dummy_gr2d = {
	.client = &dummy_client,
};

static struct host1x_gr3d dummy_gr3d = {
	.client = &dummy_client,
};

struct host1x *host1x_dummy_open(void)
{
	struct host1x *host1x;
	int err;

	host1x = calloc(1, sizeof(*host1x));
	if (!host1x)
		return NULL;

	host1x->bo_create = host1x_dummy_bo_create;
	host1x->close = host1x_dummy_close;

	host1x->gr2d = &dummy_gr2d;
	host1x->gr3d = &dummy_gr3d;

	err = host1x_gr2d_init(host1x, host1x->gr2d);
	if (err)
		return NULL;

	err = host1x_gr3d_init(host1x, host1x->gr3d);
	if (err)
		return NULL;

	return host1x;
}
