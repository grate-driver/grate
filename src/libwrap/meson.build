libwrap_sources =  files(
	'cgdrv.c',
	'list.h',
	'nvhost.c',
	'nvhost.h',
	'syscall.c',
	'utils.c',
	'utils.h'
)

libwrap = shared_library(
	'wrap',
	libwrap_sources,
	dependencies: [libdl, librnn],
	cpp_args : rnn_enable
)
