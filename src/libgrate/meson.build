libgrate_sources = files(
	'display.c',
	'fragment_asm.h',
	'grate.c',
	'grate.h',
	'grate-asm.c',
	'grate-font.c',
	'grate-texture.c',
	'grate-2d.c',
	'grate-3d.c',
	'grate-3d.h',
	'grate-3d-ctx.c',
	'grate-3d-ctx.h',
	'libgrate-private.h',
	'linker_asm.h',
	'matrix.c',
	'matrix.h',
	'profile.c',
	'shader-cgc.c',
	'vpe_vliw.h',
	'fragment_disasm.c',
	'vertex_disasm.c',
	'linker_disasm.c'
)

parser_gen = generator(
	yacc,
	output : ['@BASENAME@.tab.c', '@BASENAME@.tab.h'],
	arguments : ['-d', '-o', '@OUTPUT0@', '@EXTRA_ARGS@', '@INPUT@']
)

lex_gen = generator(
	lex,
	output : 'lex.@BASENAME@.c',
	arguments : ['@EXTRA_ARGS@', '--nounput', '-o', '@OUTPUT@', '@INPUT@']
)

vertex_asm_parser = parser_gen.process('vertex_asm.y', extra_args : ['-p', 'vertex_asm'])
lex_vertex_asm = lex_gen.process('vertex_asm.l', extra_args : ['-P', 'vertex_asm'])

fragment_asm_parser = parser_gen.process('fragment_asm.y', extra_args : ['-p', 'fragment_asm'])
lex_fragment_asm = lex_gen.process('fragment_asm.l', extra_args : ['-P', 'fragment_asm'])

linker_asm_parser = parser_gen.process('linker_asm.y', extra_args : ['-p', 'linker_asm'])
lex_linker_asm = lex_gen.process('linker_asm.l', extra_args : ['-P', 'linker_asm'])

libgrate = shared_library('grate',
	libgrate_sources,
	vertex_asm_parser, lex_vertex_asm,
	fragment_asm_parser, lex_fragment_asm,
	linker_asm_parser, lex_linker_asm,
	include_directories : include_directories('../../include'),
	dependencies : [math, devil],
	link_with : [libcgc, libhost1x]
)
