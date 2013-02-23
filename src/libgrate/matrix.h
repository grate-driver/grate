#ifndef GRATE_NVHOST_MATRIX_H
#define GRATE_NVHOST_MATRIX_H 1

#include <GLES2/gl2.h>

struct mat4 {
	GLfloat xx, xy, xz, xw;
	GLfloat yx, yy, yz, yw;
	GLfloat zx, zy, zz, zw;
	GLfloat wx, wy, wz, ww;
};

void mat4_multiply(struct mat4 *result, const struct mat4 *a,
		   const struct mat4 *b);
void mat4_zero(struct mat4 *m);
void mat4_identity(struct mat4 *m);
void mat4_rotate_x(struct mat4 *m, GLfloat angle);
void mat4_rotate_y(struct mat4 *m, GLfloat angle);
void mat4_rotate_z(struct mat4 *m, GLfloat angle);

#endif
