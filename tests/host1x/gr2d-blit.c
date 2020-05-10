/*
 * Copyright (c) GRATE-DRIVER project
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

#include "host1x.h"

int main(int argc, char *argv[])
{
	struct host1x_pixelbuffer *src, *dst;
	struct host1x_gr2d *gr2d;
	struct host1x *host1x;
	unsigned int width = 300;
	unsigned int height = 300;
	unsigned int x, y;
	void *smap, *dmap;
	int err;

	host1x = host1x_open(true, -1, -1);
	if (!host1x) {
		host1x_error("host1x_open() failed\n");
		return 1;
	}

	gr2d = host1x_get_gr2d(host1x);
	if (!gr2d) {
		host1x_error("host1x_get_gr2d() failed\n");
		return 1;
	}

	src = host1x_pixelbuffer_create(host1x,
					width, height, width * 4,
					PIX_BUF_FMT_RGBA8888,
					PIX_BUF_LAYOUT_LINEAR);
	if (!src) {
		host1x_error("host1x_pixelbuffer_create() failed\n");
		return 1;
	}

	dst = host1x_pixelbuffer_create(host1x,
					width, height, width * 4,
					PIX_BUF_FMT_RGBA8888,
					PIX_BUF_LAYOUT_LINEAR);
	if (!dst) {
		host1x_error("host1x_pixelbuffer_create() failed\n");
		return 1;
	}

	HOST1X_BO_MMAP(src->bo, &smap);
	HOST1X_BO_MMAP(dst->bo, &dmap);

	/* FRONT/BACK of BO are guarded by host1x_pixelbuffer_setup_guard() */
	smap += src->bo->offset;
	dmap += dst->bo->offset;

	err = host1x_gr2d_clear(gr2d, dst, 0xFFFFFFFF);
	if (err < 0) {
		host1x_error("host1x_gr2d_clear() failed: %d\n", err);
		return 1;
	}

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			uint32_t *pdst = dmap + y * dst->pitch;

			if (pdst[x] != 0xFFFFFFFF) {
				host1x_error("pdst[%u][%u] 0x%08X != 0xFFFFFFFF\n",
					     x, y, pdst[x]);
				return 1;
			}
		}

		err = host1x_gr2d_clear_rect(gr2d, src, y * 0x100ff000 + y,
					     0, y, width, 1);
		if (err < 0) {
			host1x_error("host1x_gr2d_clear_rect() failed: %d\n",
				     err);
			return 1;
		}

		for (x = 0; x < width; x++) {
			uint32_t *psrc = smap + y * src->pitch;

			if (psrc[x] != y * 0x100ff000 + y) {
				host1x_error("psrc[%u][%u] 0x%08X != 0x%08X\n",
					     x, y, psrc[x], y * 0x100ff000 + y);
				return 1;
			}
		}
	}

	err = host1x_gr2d_surface_blit(gr2d,
				       src, dst,
				       0, 0, width, height,
				       0, 0, width,-height);
	if (err < 0) {
		host1x_error("host1x_gr2d_surface_blit() failed: %d\n", err);
		return 1;
	}

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			uint32_t *pdst = dmap + y * dst->pitch;
			unsigned y_inv = height - y - 1;

			if (pdst[x] != y_inv * 0x100ff000 + y_inv) {
				host1x_error("pdst[%u][%u] 0x%08X != 0x%08X\n",
					     x, y, pdst[x], y_inv * 0x100ff000 + y_inv);
				return 1;
			}
		}
	}

	err = host1x_gr2d_clear(gr2d, dst, 0xFFFFFFFF);
	if (err < 0) {
		host1x_error("host1x_gr2d_clear() failed: %d\n", err);
		return 1;
	}

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			uint32_t *pdst = dmap + y * dst->pitch;

			if (pdst[x] != 0xFFFFFFFF) {
				host1x_error("pdst[%u][%u] 0x%08X != 0xFFFFFFFF\n",
					     x, y, pdst[x]);
				return 1;
			}
		}
	}

	err = host1x_gr2d_blit(gr2d, src, dst, 0, 0, 0, 0, width, -height);
	if (err < 0) {
		host1x_error("host1x_gr2d_surface_blit() failed: %d\n", err);
		return 1;
	}

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			uint32_t *pdst = dmap + y * dst->pitch;
			unsigned y_inv = height - y - 1;

			if (pdst[x] != y_inv * 0x100ff000 + y_inv) {
				host1x_error("pdst[%u][%u] 0x%08X != 0x%08X\n",
					     x, y, pdst[x], y_inv * 0x100ff000 + y_inv);
				return 1;
			}
		}
	}

	host1x_close(host1x);

	host1x_info("test passed\n");

	return 0;
}
