/*
 * Copyright (c) 2012, 2013 Erik Faye-Lund
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

#include "libgrate-private.h"
#include "grate.h"
#include "host1x.h"

struct grate_texture {
	struct host1x_texture *base;
};

struct grate_texture *grate_texture_load(struct grate *grate,
					 const char *path)
{
	struct grate_texture *texture;
	struct host1x_texture *base;

	base = host1x_texture_load(grate->host1x, path);
	if (!base)
		return NULL;

	texture = calloc(1, sizeof(*texture));
	if (!texture) {
		host1x_texture_free(base);
		return NULL;
	}

	texture->base = base;

	return texture;
}

void grate_texture_free(struct grate_texture *texture)
{
	if (texture)
		host1x_texture_free(texture->base);

	free(texture);
}
