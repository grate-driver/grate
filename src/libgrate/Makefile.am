noinst_LTLIBRARIES = libgrate.la

libgrate_la_CPPFLAGS = \
	-I$(top_srcdir)/include

libgrate_la_CFLAGS = \
	$(PNG_CFLAGS)

libgrate_la_YFLAGS = \
	-d --debug

libgrate_la_LFLAGS = \
	--nounput

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
	libgrate_la-asm_parser.h

CLEANFILES = \
	libgrate_la-asm_parser.h \
	libgrate_la-asm_parser.c \
	libgrate_la-asm_lexer.c

libgrate_la_SOURCES += \
	asm_lexer.l \
	asm_parser.y \
	asm.h \
	disasm.c \
	shader-cgc.c

libgrate_la_LIBADD += \
	../libcgc/libcgc.la
else
libgrate_la_SOURCES += \
	shader-dummy.c
endif
