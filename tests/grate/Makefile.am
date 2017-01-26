noinst_PROGRAMS = \
	clear \
	cube \
	cube-textured \
	quad \
	triangle \
	triangle-rotate

AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/libgrate

LDADD = \
	$(top_builddir)/src/libgrate/libgrate.la

cube_textured_LDADD = \
	$(top_builddir)/src/libgrate/libgrate.la \
	$(DevIL_LIBS)
