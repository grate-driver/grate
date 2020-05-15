noinst_LTLIBRARIES = libcommon.la

libcommon_la_SOURCES = \
	common.c \
	common.h

libcommon_la_LIBADD = $(X11_LIBS) $(GLES1_LIBS) $(PNG_LIBS)

AM_CFLAGS = -I$(top_srcdir)/src/libgrate $(GLES1_CFLAGS) $(PNG_CFLAGS)
LDADD = libcommon.la

noinst_PROGRAMS = \
	gles-clip \
	gles-flatshading \
	gles-logicop \
	gles-tex-combiners \
	gles1-info
