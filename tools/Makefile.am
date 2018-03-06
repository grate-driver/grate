noinst_PROGRAMS = \
	assembler \
	cgc \
	hex2float \
	fp20 \
	fx10 \
	replay

AM_LDFLAGS = -static

assembler_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/libgrate

assembler_LDADD = \
	../src/libgrate/libgrate.la

cgc_CPPFLAGS = \
	-I$(top_srcdir)/include

cgc_LDADD = \
	../src/libcgc/libcgc.la

replay_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/libhost1x \
	-I$(top_srcdir)/src/libgrate

replay_CFLAGS = $(ZLIB_CFLAGS)

replay_LDADD = \
	../src/libgrate/libgrate.la \
	 -lz
