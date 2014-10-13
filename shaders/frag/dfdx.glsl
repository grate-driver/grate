#extension GL_OES_standard_derivatives : enable

uniform mediump vec4 u;
void main()
{
	gl_FragColor.x = dFdx(u.x);
}
