uniform vec4 u;
void main()
{
	gl_Position.x = u.x > 0.0 ? 1.0 : 0.0;
}
