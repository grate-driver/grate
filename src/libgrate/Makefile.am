noinst_LTLIBRARIES = libgrate.la

libgrate_la_CPPFLAGS = \
	-I$(top_srcdir)/include

libgrate_la_CFLAGS = \
	$(PNG_CFLAGS)

libgrate_la_SOURCES = \
	display.c \
	grate.c \
	grate.h \
	libgrate-private.h \
	matrix.c \
	matrix.h \
	profile.c

libgrate_la_LIBADD = \
	../libhost1x/libhost1x.la \
	$(PNG_LIBS) \
	-lm \
	-lrt

if ENABLE_CGC
BUILT_SOURCES = \
	vertex_asm.tab.c \
	lex.vertex_asm.c

CLEANFILES = \
	vertex_asm.tab.h \
	vertex_asm.tab.c \
	lex.vertex_asm.c

lex.vertex_asm.c: vertex_asm_lexer.l
	$(LEX) -P vertex_asm --nounput vertex_asm_lexer.l

vertex_asm.tab.c: vertex_asm_parser.y
	$(YACC) -p vertex_asm -b vertex_asm -d --debug vertex_asm_parser.y

libgrate_la_SOURCES += \
	vertex_asm.tab.c \
	lex.vertex_asm.c \
	asm.h \
	vertex_disasm.c \
	shader-cgc.c

libgrate_la_LIBADD += \
	../libcgc/libcgc.la
else
libgrate_la_SOURCES += \
	shader-dummy.c
endif
