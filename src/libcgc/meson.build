libcgc_sources =  files(
	'instruction.c',
	'shader.c',
	'stream.c',
)
libcgc_deps = []

if cgdrv.found()
	libcgc_deps += cgdrv
else
	libcgc_sources += files('dummy.c')
endif

libcgc_lib = shared_library('cgc',
	libcgc_sources,
	include_directories : include_directories('../../include'),
	dependencies: libcgc_deps,
)

libcgc_idep = declare_dependency(
  link_with : libcgc_lib,
  dependencies : libcgc_deps
)
