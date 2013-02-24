noinst_LTLIBRARIES = \
	libcgc.la

noinst_HEADERS = \
	libcgc.h

libcgc_la_CPPFLAGS = \
	-I$(top_srcdir)/include

libcgc_la_SOURCES = \
	instruction.c \
	shader.c

libcgc_la_LIBADD = \
	../libhost1x/libhost1x.la \
	-lcgdrv
