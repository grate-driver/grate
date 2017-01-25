noinst_PROGRAMS = \
	assembler \
	cgc \
	hex2float \
	fp20 \
	fx10

assembler_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/libgrate

assembler_LDADD = \
	../src/libgrate/libgrate.la

cgc_CPPFLAGS = \
	-I$(top_srcdir)/include

cgc_LDADD = \
	../src/libcgc/libcgc.la
