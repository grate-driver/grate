noinst_LTLIBRARIES = \
	libcgc.la

noinst_HEADERS = \
	libcgc-private.h

libcgc_la_CPPFLAGS = \
	-I$(top_srcdir)/include

libcgc_la_SOURCES = \
	instruction.c \
	shader.c \
	stream.c

if ENABLE_CGC
libcgc_la_LIBADD = \
	-lcgdrv
else
libcgc_la_SOURCES += \
	dummy.c
endif
