noinst_LTLIBRARIES = libcommon.la

libcommon_la_SOURCES = \
	common.c \
	common.h

libcommon_la_LIBADD = $(X11_LIBS) $(GLES1_LIBS) $(PNG_LIBS)

AM_CPPFLAGS = -I$(top_srcdir)/src/libgrate
LDADD = libcommon.la

noinst_PROGRAMS = \
	gles-logicop

gles_logicop_LDADD = \
	$(top_builddir)/src/libgrate/libgrate.la \
	libcommon.la
