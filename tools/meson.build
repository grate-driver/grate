tools = [
	'assembler',
	'cgc',
	'hex2float',
	'fp20',
	'fx10',
	'replay'
]

includes = include_directories(
	'../../include',
	'../../src/libgrate',
	'../../src/libhost1x'
)

foreach tool : tools
	src = tool + '.c'
	executable(
		tool,
		src,
		include_directories : includes,
		dependencies : [math, dependency('zlib')],
		link_with : [libgrate, libhost1x, libcgc]
	)
endforeach
