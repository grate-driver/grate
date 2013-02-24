noinst_PROGRAMS = \
	hex2float \
	fp20 \
	fx10

if ENABLE_CGC
noinst_PROGRAMS += \
	cgc

cgc_CPPFLAGS = \
	-I$(top_srcdir)/include

cgc_LDADD = \
	../src/libcgc/libcgc.la
endif
