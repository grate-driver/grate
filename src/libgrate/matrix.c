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

void mat4_rotate_x(struct mat4 *m, GLfloat angle)
{
	GLfloat c = cos(angle * M_PI / 180.0);
	GLfloat s = sin(angle * M_PI / 180.0);

	mat4_identity(m);

	m->yy =  c;
	m->yz = -s;
	m->zy =  s;
	m->zz =  c;
}

void mat4_rotate_y(struct mat4 *m, GLfloat angle)
{
	GLfloat c = cos(angle * M_PI / 180.0);
	GLfloat s = sin(angle * M_PI / 180.0);

	mat4_identity(m);

	m->xx =  c;
	m->xz =  s;
	m->zx = -s;
	m->zz =  c;
}

void mat4_rotate_z(struct mat4 *m, GLfloat angle)
{
	GLfloat c = cos(angle * M_PI / 180.0);
	GLfloat s = sin(angle * M_PI / 180.0);

	mat4_identity(m);

	m->xx =  c;
	m->xy = -s;
	m->yx =  s;
	m->yy =  c;
}
