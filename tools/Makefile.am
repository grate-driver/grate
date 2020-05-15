noinst_PROGRAMS = \
	assembler \
	cgc \
	hex2float \
	fp20 \
	fx10 \
	replay \
	reset3d

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

if ENABLE_ZLIB
replay_CPPFLAGS += -DENABLE_ZLIB
endif

if ENABLE_LZ4
replay_CPPFLAGS += -DENABLE_LZ4
endif

replay_CFLAGS = $(ZLIB_CFLAGS)

replay_LDADD = \
	../src/libgrate/libgrate.la
if ENABLE_ZLIB
replay_LDADD += -lz
endif
if ENABLE_LZ4
replay_LDADD += -llz4
endif
