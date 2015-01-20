AC_PREREQ(2.59)
AC_INIT([grate], [0.0.0], [thierry.reding@avionic-design.de])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADER([config.h])
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([no-dist-gzip dist-xz foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_LIBTOOL
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL

PKG_CHECK_MODULES(GLES1, [egl glesv1_cm], [HAVE_GLES1=1], [HAVE_GLES1=0])
AM_CONDITIONAL([USE_GLES1], [test "$HAVE_GLES1" -eq 1])

PKG_CHECK_MODULES(GLES2, [egl glesv2], [HAVE_GLES2=1], [HAVE_GLES2=0])
AM_CONDITIONAL([USE_GLES2], [test "$HAVE_GLES2" -eq 1])

PKG_CHECK_MODULES(X11, x11)
PKG_CHECK_MODULES(PNG, libpng)
PKG_CHECK_MODULES(DRM, libdrm)

CFLAGS="$CFLAGS -Wall"

AC_ARG_ENABLE([werror],
	[AS_HELP_STRING([--enable-werror],
		[Treat warnings as errors (default: disabled)])],
		[enable_werror="$enableval"],
		[enable_werror=no])
if test "x$enable_werror" = "xyes"; then
	CFLAGS="$CFLAGS -Werror"
fi

AC_CHECK_LIB([cgdrv], [CgDrv_Create], [enable_cgc=yes], [enable_cgc=no])
AM_CONDITIONAL([ENABLE_CGC], [test "x$enable_cgc" = "xyes"])

AC_CHECK_HEADER([envytools/rnn.h],
	[AC_CHECK_LIB([rnn], [rnn_init], [enable_rnn=yes], [enable_rnn=no], [-lenvyutil -lxml2])])
AM_CONDITIONAL([ENABLE_RNN], [test "x$enable_rnn" = "xyes"])

AC_OUTPUT([
	Makefile
	include/Makefile
	src/Makefile
	src/libcgc/Makefile
	src/libgrate/Makefile
	src/libhost1x/Makefile
	src/libwrap/Makefile
	tests/Makefile
	tests/drm/Makefile
	tests/gles1/Makefile
	tests/gles2/Makefile
	tests/grate/Makefile
	tests/host1x/Makefile
	tests/nvhost/Makefile
	tools/Makefile
])
