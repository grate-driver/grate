libcgc_sources =  files(
	'instruction.c',
	'shader.c',
	'stream.c',

	'dummy.c' # temp
)

libcgc_lib = shared_library('cgc',
	libcgc_sources,
	include_directories : include_directories('../../include')
)

libcgc_idep = declare_dependency(
  link_with : libcgc_lib,
  dependencies : libcgc_deps
)
