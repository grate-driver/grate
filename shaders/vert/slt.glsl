uniform vec4 u;
void main()
{
	gl_Position.x = u.x < u.y ? 1.0 : 0.0;
}
