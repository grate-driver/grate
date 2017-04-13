noinst_LTLIBRARIES = libcommon.la

libcommon_la_SOURCES = \
	common.c \
	common.h

libcommon_la_LIBADD = $(X11_LIBS) $(GLES2_LIBS) $(PNG_LIBS) $(DevIL_LIBS)

AM_CFLAGS = -I$(top_srcdir)/src/libgrate $(GLES2_CFLAGS) $(PNG_CFLAGS)
AM_LDFLAGS = -static
LDADD = libcommon.la

noinst_PROGRAMS = \
	egl-clear \
	egl-triangle \
	egl-x11-clear \
	egl-x11-triangle \
	gles-clear \
	gles-cube \
	gles-cube-textured \
	gles-info \
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
	gles-shader-fill \
	gles-stencil

gles_cube_LDADD = \
	$(top_builddir)/src/libgrate/libgrate.la \
	libcommon.la

gles_cube_textured_LDADD = \
	$(top_builddir)/src/libgrate/libgrate.la \
	libcommon.la

gles_pbuffer_triangle_rotate_LDADD = \
	$(top_builddir)/src/libgrate/libgrate.la \
	libcommon.la

gles_pbuffer_cube_LDADD = \
	$(top_builddir)/src/libgrate/libgrate.la \
	libcommon.la

gles_quad_textured_LDADD = \
	$(top_builddir)/src/libgrate/libgrate.la \
	libcommon.la

gles_quad_textured2_LDADD = \
	$(top_builddir)/src/libgrate/libgrate.la \
	libcommon.la

gles_stencil_LDADD = \
	$(top_builddir)/src/libgrate/libgrate.la \
	libcommon.la
