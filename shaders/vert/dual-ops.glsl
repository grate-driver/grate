uniform vec4 u;
void main()
{
	gl_Position.xyz = normalize(u.xyz);
	gl_Position.w = dot(u, u);
}
