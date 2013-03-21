noinst_LTLIBRARIES = \
	libhost1x.la

libhost1x_la_CPPFLAGS = \
	-I$(top_srcdir)/include

libhost1x_la_CFLAGS = \
	$(DRM_CFLAGS) \
	$(PNG_CFLAGS)

libhost1x_la_SOURCES = \
	host1x.c \
	host1x-drm.c \
	host1x-framebuffer.c \
	host1x-gr2d.c \
	host1x-gr3d.c \
	host1x-nvhost.c \
	host1x-private.h \
	nvhost.c \
	nvhost-gr2d.c \
	nvhost-gr2d.h \
	nvhost-gr3d.c \
	nvhost-gr3d.h \
	nvhost.h \
	nvhost-nvmap.c \
	nvhost-nvmap.h \
	stream.c

libhost1x_la_LIBADD = $(DRM_LIBS) $(PNG_LIBS)
