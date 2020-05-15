noinst_LTLIBRARIES = libcommon.la

libcommon_la_SOURCES = \
	common.c \
	common.h

libcommon_la_LIBADD = $(X11_LIBS) $(GLES2_LIBS) $(PNG_LIBS) $(DevIL_LIBS) -lm

AM_CFLAGS = $(GLES2_CFLAGS) $(PNG_CFLAGS)
AM_CFLAGS += -I$(top_srcdir)/include
AM_CFLAGS += -I$(top_srcdir)/src/libgrate
LDADD = libcommon.la $(top_builddir)/src/libgrate/libgrate.la

noinst_PROGRAMS = \
	egl-clear \
	egl-triangle \
	egl-x11-clear \
	egl-x11-triangle \
	gles-clear \
	gles-cube \
	gles-cube-textured \
	gles-pbuffer-cgshader \
	gles-pbuffer-clear \
	gles-pbuffer-cube \
	gles-pbuffer-fill \
	gles-pbuffer-triangle \
	gles-pbuffer-triangle-rotate \
	gles-pbuffer-uniforms \
	gles-pbuffer-uniforms-rgb \
	gles-pbuffer-uniforms-rg \
	gles-pbuffer-vertex-offset \
	gles-quad-textured \
	gles-quad-textured2 \
	gles-quad-textured3 \
	gles-shader-fill \
	gles-stencil \
	gles2-info
