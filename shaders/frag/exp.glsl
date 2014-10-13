uniform mediump float u;
void main()
{
	gl_FragColor.x = exp2(u);
}
