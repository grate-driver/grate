noinst_LTLIBRARIES = libcommon.la

libcommon_la_SOURCES = \
	common.c \
	common.h

libcommon_la_LIBADD = $(X11_LIBS) $(GLES1_LIBS) $(PNG_LIBS)

AM_CPPFLAGS = -I$(top_srcdir)/src/libgrate $(GLES1_CFLAGS)
LDADD = libcommon.la

noinst_PROGRAMS = \
	gles-clip \
	gles-flatshading \
	gles-info \
	gles-logicop \
	gles-tex-combiners
