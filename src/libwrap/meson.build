libwrap_sources =  files(
	'cgdrv.c',
	'list.h',
	'nvhost.c',
	'nvhost.h',
	'syscall.c',
	'utils.c',
	'utils.h'
)

libwrap_deps = [libdl]
libwrap_c_args = ['-U_FILE_OFFSET_BITS']

librnn = cc.find_library('rnn', required : false)
if librnn.found() and cc.has_header('envytools/rnn.h')
	libenvyutil = cc.find_library('envyutil')
	libxml2 = cc.find_library('xml2')
	libwrap_deps += [librnn, libenvyutil, libxml2]
	libwrap_c_args += ['-DENABLE_RNN']
endif

libwrap = shared_library(
	'wrap',
	libwrap_sources,
	dependencies: libwrap_deps,
	c_args: libwrap_c_args
)
