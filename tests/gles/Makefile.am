noinst_LTLIBRARIES = libcommon.la

libcommon_la_SOURCES = \
	common.c \
	common.h

libcommon_la_LIBADD = $(X11_LIBS) $(GLES2_LIBS) $(PNG_LIBS)

LDADD = libcommon.la

noinst_PROGRAMS = \
	egl-clear \
	egl-triangle \
	egl-x11-clear \
	egl-x11-triangle \
	gles-clear \
	gles-pbuffer-clear \
	gles-pbuffer-fill \
	gles-shader-fill
