noinst_PROGRAMS = \
	assembler \
	hex2float \
	fp20 \
	fx10

assembler_CPPFLAGS = \
	-I$(top_srcdir)/src/libgrate

assembler_LDADD = \
	../src/libgrate/libgrate.la

if ENABLE_CGC
noinst_PROGRAMS += \
	cgc

cgc_CPPFLAGS = \
	-I$(top_srcdir)/include

cgc_LDADD = \
	../src/libcgc/libcgc.la
endif
