tests = [
	'gles-clip',
	'gles-flatshading',
	'gles-logicop',
	'gles-tex-combiners',
	'gles1-info'
]

foreach test : tests
	src = test + '.c'
	executable(
		test,
		[src, 'common.c'],
		dependencies : [egl, x11, gles1, libpng]
	)
endforeach
