tools = [
	'assembler',
	'cgc',
	'hex2float',
	'fp20',
	'fx10'
]

includes = include_directories(
	'../../include',
	'../../src/libgrate'
)

foreach tool : tools
	src = tool + '.c'
	executable(
		tool,
		src,
		include_directories : includes,
		dependencies : math,
		link_with : [libgrate, libhost1x, libcgc]
	)
endforeach
