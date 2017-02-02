noinst_LTLIBRARIES = libgrate.la

libgrate_la_CPPFLAGS = \
	-I$(top_srcdir)/include

libgrate_la_CFLAGS = \
	$(PNG_CFLAGS)

libgrate_la_SOURCES = \
	display.c \
	grate.c \
	grate.h \
	grate-asm.c \
	grate-texture.c \
	grate-2d.c \
	grate-3d.c \
	grate-3d-ctx.c \
	libgrate-private.h \
	matrix.c \
	matrix.h \
	profile.c \
	shader-cgc.c

libgrate_la_LIBADD = \
	../libcgc/libcgc.la \
	../libhost1x/libhost1x.la \
	$(DevIL_LIBS) \
	$(PNG_LIBS) \
	-lm \
	-lrt

BUILT_SOURCES = \
	lex.fragment_asm.c \
	lex.linker_asm.c \
	lex.vertex_asm.c \
	fragment_asm.tab.c \
	linker_asm.tab.c \
	vertex_asm.tab.c

CLEANFILES = \
	fragment_asm.tab.h \
	fragment_asm.tab.c \
	linker_asm.tab.h \
	linker_asm.tab.c \
	vertex_asm.tab.h \
	vertex_asm.tab.c \
	lex.fragment_asm.c \
	lex.linker_asm.c \
	lex.vertex_asm.c

lex.vertex_asm.c: vertex_asm_lexer.l vertex_asm.tab.c
	$(LEX) -P vertex_asm --nounput vertex_asm_lexer.l

vertex_asm.tab.c: vertex_asm_parser.y
	$(YACC) -p vertex_asm -b vertex_asm -d --debug vertex_asm_parser.y

lex.fragment_asm.c: fragment_asm_lexer.l fragment_asm.tab.c
	$(LEX) -P fragment_asm --nounput fragment_asm_lexer.l

fragment_asm.tab.c: fragment_asm_parser.y
	$(YACC) -p fragment_asm -b fragment_asm -d --debug fragment_asm_parser.y

lex.linker_asm.c: linker_asm_lexer.l linker_asm.tab.c
	$(LEX) -P linker_asm --nounput linker_asm_lexer.l

linker_asm.tab.c: linker_asm_parser.y
	$(YACC) -p linker_asm -b linker_asm -d --debug linker_asm_parser.y

libgrate_la_SOURCES += \
	fragment_asm.tab.c \
	fragment_disasm.c \
	lex.fragment_asm.c \
	lex.linker_asm.c \
	lex.vertex_asm.c \
	linker_asm.tab.c \
	linker_disasm.c \
	vertex_asm.tab.c \
	vertex_disasm.c
