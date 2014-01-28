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

#ifndef GRATE_NVHOST_MATRIX_H
#define GRATE_NVHOST_MATRIX_H 1

struct mat4 {
	float xx, xy, xz, xw;
	float yx, yy, yz, yw;
	float zx, zy, zz, zw;
	float wx, wy, wz, ww;
};

void mat4_multiply(struct mat4 *result, const struct mat4 *a,
		   const struct mat4 *b);
void mat4_zero(struct mat4 *m);
void mat4_identity(struct mat4 *m);
void mat4_translate(struct mat4 *m, float x, float y, float z);
void mat4_scale(struct mat4 *m, float x, float y, float z);
void mat4_rotate_x(struct mat4 *m, float angle);
void mat4_rotate_y(struct mat4 *m, float angle);
void mat4_rotate_z(struct mat4 *m, float angle);

void mat4_perspective(struct mat4 *m, float fov, float aspect,
		      float near, float far);

#endif
