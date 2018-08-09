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

tools_deps = [math]
tools_c_args = []

libz = cc.find_library('z', required : false)
if libz.found() and cc.has_header('zlib.h')
	tools_deps += [libz]
	tools_c_args += ['-DENABLE_ZLIB']
endif

liblz4 = cc.find_library('lz4', required : false)
if liblz4.found() and cc.has_header('lz4.h')
	tools_deps += [liblz4]
	tools_c_args += ['-DENABLE_LZ4']
endif

foreach tool : tools
	src = tool + '.c'
	executable(
		tool,
		src,
		include_directories : includes,
		dependencies : tools_deps,
		link_with : [libgrate, libhost1x, libcgc],
		c_args: tools_c_args,
	)
endforeach
