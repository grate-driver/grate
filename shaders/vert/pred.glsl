// uniform mat4 mvp;
// attribute vec4 vert;
// varying vec3 color;

uniform vec4 u[10];
attribute vec4 a0, a1, a2, a3;
varying vec4 v0, v1, v2, v3;
uniform bool ub;
void main()
{
//	gl_Position = mvp * vert;
	gl_Position = a1;
	bvec4 cond = lessThan(a0, u[0]);
	if (cond.x)
		gl_Position.x = a0.x;
	if (cond.y)
		gl_Position.y = a0.y;
	if (!cond.z)
		gl_Position.z = a0.z;
	if (!cond.w)
		gl_Position.w = a0.w;
}
