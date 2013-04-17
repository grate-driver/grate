noinst_LTLIBRARIES = libcommon.la

libcommon_la_SOURCES = \
	common.c \
	common.h

libcommon_la_LIBADD = $(X11_LIBS) $(GLES2_LIBS) $(PNG_LIBS)

AM_CPPFLAGS = -I$(top_srcdir)/src/libgrate
LDADD = libcommon.la

noinst_PROGRAMS = \
	egl-clear \
	egl-triangle \
	egl-x11-clear \
	egl-x11-triangle \
	gles-clear \
	gles-pbuffer-clear \
	gles-pbuffer-cube \
	gles-pbuffer-fill \
	gles-pbuffer-triangle \
	gles-pbuffer-triangle-rotate \
	gles-pbuffer-uniforms \
	gles-pbuffer-uniforms-rgb \
	gles-pbuffer-uniforms-rg \
	gles-pbuffer-vertex-offset \
	gles-shader-fill

gles_pbuffer_triangle_rotate_LDADD = \
	$(top_builddir)/src/libgrate/libgrate.la \
	libcommon.la

gles_pbuffer_cube_LDADD = \
	$(top_builddir)/src/libgrate/libgrate.la \
	libcommon.la
