uniform mediump vec4 u;
void main()
{
	gl_FragColor.x = u.x == 0.0 ? u.y : u.z;
	gl_FragColor.y = u.y != 0.0 ? u.y : u.z;
	gl_FragColor.z = u.z <= 0.0 ? u.y : u.z;
	gl_FragColor.w = u.w < 0.0 ? u.y : u.z;
}
