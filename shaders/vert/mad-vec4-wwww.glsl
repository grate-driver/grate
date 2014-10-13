uniform vec4 u;
void main()
{
	gl_Position = u * u + u.wwww;
}
