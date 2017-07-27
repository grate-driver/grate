tests = [
	'clear',
	'cube',
	'cube-textured',
	'cube-textured2',
	'interactive',
	'quad',
	'stencil',
	'texture-filter',
	'texture-wrap',
	'triangle',
	'triangle-rotate'
]

includes = include_directories(
	'../../include',
	'../../src/libgrate'
)

foreach test : tests
	src = test + '.c'
	executable(
		test,
		src,
		include_directories : includes,
		dependencies : math,
		link_with : [libgrate, libhost1x]
	)
endforeach
