uniform vec4 u[10];
attribute vec4 a;
void main()
{
	gl_Position.x = u[int(a.x)].x;
	gl_Position.y = u[int(a.y)].x;
	gl_Position.z = u[int(a.z)].x;
	gl_Position.w = u[int(a.w)].x;
}
