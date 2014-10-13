varying highp vec4 v0, v1, v2;
void main()
{
	gl_FragColor.x = clamp((v0.x + v1.x * v2.x) * 1.0, 0.0, 1.0);
	gl_FragColor.y = clamp((v0.y + v1.y * v2.y) * 2.0, 0.0, 1.0);
	gl_FragColor.z = clamp((v0.z + v1.z * v2.z) * 4.0, 0.0, 1.0);
	gl_FragColor.w = clamp((v0.w + v1.w * v2.w) * 0.5, 0.0, 1.0);
}
