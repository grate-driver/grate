// #extension GL_EXT_shader_framebuffer_fetch: require
#extension GL_NV_shader_framebuffer_fetch: require

// mediump vec4 gl_LastFragData[gl_MaxDrawBuffers];

void main()
{
	gl_FragColor  = gl_LastFragData[0] * 2.0;
	gl_FragColor += gl_LastFragData[1] + 1.0;
//	gl_FragData[2] = gl_LastFragData[1] * 2.0;
//	gl_FragData[3] = gl_LastFragData[0] * 2.0;
}
