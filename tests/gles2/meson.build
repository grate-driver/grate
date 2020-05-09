tests = [
	'egl-clear',
	'egl-triangle',
	'egl-x11-clear',
	'egl-x11-triangle',
	'gles-clear',
	'gles-cube',
	'gles-cube-textured',
	'gles-pbuffer-cgshader',
	'gles-pbuffer-clear',
	'gles-pbuffer-cube',
	'gles-pbuffer-fill',
	'gles-pbuffer-triangle',
	'gles-pbuffer-triangle-rotate',
	'gles-pbuffer-uniforms',
	'gles-pbuffer-uniforms-rgb',
	'gles-pbuffer-uniforms-rg',
	'gles-pbuffer-vertex-offset',
	'gles-quad-textured',
	'gles-quad-textured2',
	'gles-quad-textured3',
	'gles-shader-fill',
	'gles-stencil',
	'gles2-info'
]

include_dirs = [
	'../../include',
	'../../src/libgrate',
]

foreach test : tests
	src = test + '.c'
	executable(
		test,
		[src, 'common.c'],
		include_directories : include_directories(include_dirs),
		dependencies : [math, egl, x11, gles2, libpng, devil],
		link_with : libgrate
	)
endforeach
