tests = [
	'gr2d-clear',
	'gr3d-triangle',
]

includes = include_directories(
	'../../include',
	'../../src/libhost1x'
)

foreach test : tests
	src = test + '.c'
	executable(
		test,
		src,
		include_directories : includes,
		link_with : libhost1x
	)
endforeach
