void main()
{
	gl_FragColor.x = float(gl_FrontFacing);
	gl_FragColor.y = -float(gl_FrontFacing);
	gl_FragColor.z = abs(float(gl_FrontFacing));
	gl_FragColor.w = float(gl_FrontFacing) * 2.0;
}
