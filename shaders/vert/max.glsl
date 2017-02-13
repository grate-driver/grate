uniform vec4 u;
attribute vec4 a;
void main()
{
	gl_Position = max(u, a);
}
