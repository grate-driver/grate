uniform vec4 u;
void main()
{
	gl_Position.x = dot(u, u);
}
