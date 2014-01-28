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

#include <math.h>

#include "matrix.h"

void mat4_multiply(struct mat4 *result, const struct mat4 *a,
		   const struct mat4 *b)
{
	result->xx = a->xx * b->xx + a->xy * b->yx + a->xz * b->zx + a->xw * b->wx;
	result->xy = a->xx * b->xy + a->xy * b->yy + a->xz * b->zy + a->xw * b->wy;
	result->xz = a->xx * b->xz + a->xy * b->yz + a->xz * b->zz + a->xw * b->wz;
	result->xw = a->xx * b->xw + a->xy * b->yw + a->xz * b->zw + a->xw * b->ww;

	result->yx = a->yx * b->xx + a->yy * b->yx + a->yz * b->zx + a->yw * b->wx;
	result->yy = a->yx * b->xy + a->yy * b->yy + a->yz * b->zy + a->yw * b->wy;
	result->yz = a->yx * b->xz + a->yy * b->yz + a->yz * b->zz + a->yw * b->wz;
	result->yw = a->yx * b->xw + a->yy * b->yw + a->yz * b->zw + a->yw * b->ww;

	result->zx = a->zx * b->xx + a->zy * b->yx + a->zz * b->zx + a->zw * b->wx;
	result->zy = a->zx * b->xy + a->zy * b->yy + a->zz * b->zy + a->zw * b->wy;
	result->zz = a->zx * b->xz + a->zy * b->yz + a->zz * b->zz + a->zw * b->wz;
	result->zw = a->zx * b->xw + a->zy * b->yw + a->zz * b->zw + a->zw * b->ww;

	result->wx = a->wx * b->xx + a->wy * b->yx + a->wz * b->zx + a->ww * b->wx;
	result->wy = a->wx * b->xy + a->wy * b->yy + a->wz * b->zy + a->ww * b->wy;
	result->wz = a->wx * b->xz + a->wy * b->yz + a->wz * b->zz + a->ww * b->wz;
	result->ww = a->wx * b->xw + a->wy * b->yw + a->wz * b->zw + a->ww * b->ww;
}

void mat4_zero(struct mat4 *m)
{
	m->xx = m->xy = m->xz = m->xw = 0.0f;
	m->yx = m->yy = m->yz = m->yw = 0.0f;
	m->zx = m->zy = m->zz = m->zw = 0.0f;
	m->wx = m->wy = m->wz = m->ww = 0.0f;
}

void mat4_identity(struct mat4 *m)
{
	mat4_zero(m);

	m->xx = m->yy = m->zz = m->ww = 1.0f;
}

void mat4_translate(struct mat4 *m, float x, float y, float z)
{
	mat4_identity(m);

	m->xw = x;
	m->yw = y;
	m->zw = z;
}

void mat4_scale(struct mat4 *m, float x, float y, float z)
{
	mat4_zero(m);

	m->xx = x;
	m->yy = y;
	m->zz = z;
	m->ww = 1.0f;
}

void mat4_rotate_x(struct mat4 *m, float angle)
{
	float c = cos(angle * M_PI / 180.0);
	float s = sin(angle * M_PI / 180.0);

	mat4_identity(m);

	m->yy =  c;
	m->yz = -s;
	m->zy =  s;
	m->zz =  c;
}

void mat4_rotate_y(struct mat4 *m, float angle)
{
	float c = cos(angle * M_PI / 180.0);
	float s = sin(angle * M_PI / 180.0);

	mat4_identity(m);

	m->xx =  c;
	m->xz =  s;
	m->zx = -s;
	m->zz =  c;
}

void mat4_rotate_z(struct mat4 *m, float angle)
{
	float c = cos(angle * M_PI / 180.0);
	float s = sin(angle * M_PI / 180.0);

	mat4_identity(m);

	m->xx =  c;
	m->xy = -s;
	m->yx =  s;
	m->yy =  c;
}

void mat4_perspective(struct mat4 *m, float fov, float aspect,
		      float near, float far)
{
	float radians = fov / 2 * M_PI / 180;
	double sine, cosine, cotangent;
	double depth = far - near;

	mat4_identity(m);

	sine = sin(radians);
	cosine = cos(radians);

	if (depth == 0 || sine == 0 || aspect == 0)
		return;

	cotangent = cosine / sine;

	m->xx = cotangent / aspect;
	m->yy = cotangent;
	m->zz = -(far + near) / depth;
	m->zw = -1.0f;
	m->wz = -2.0f * near * far / depth;
	m->ww = 0.0f;
}
