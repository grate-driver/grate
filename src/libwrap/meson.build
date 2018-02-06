libwrap_sources =  files(
	'cdma_parser.c',
	'cdma_parser.h',
	'cgdrv.c',
	'disasm.c',
	'disasm.h',
	'drm.h',
	'drm_mode.h',
	'host1x.c',
	'host1x.h',
	'list.h',
	'nvhost.c',
	'nvhost.h',
	'syscall.c',
	'syscall.h',
	'tegra_drm.h',
	'utils.c',
	'utils.h'
)

libwrap_sources += files(
	'../libgrate/asm.h',
	'../libgrate/fragment_asm.h',
	'../libgrate/fragment_disasm.c',
	'../libgrate/linker_asm.h',
	'../libgrate/linker_disasm.c',
	'../libgrate/vertex_disasm.c',
	'../libgrate/vpe_vliw.h'
)

libwrap_includes = include_directories(
	'../../include',
	'../../src/libgrate'
)

libwrap_deps = [libdl, dependency('threads')]
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
	include_directories : libwrap_includes,
	c_args: libwrap_c_args
)
