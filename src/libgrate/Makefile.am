noinst_LTLIBRARIES = libgrate.la

libgrate_la_CPPFLAGS = \
	-I$(top_srcdir)/include

libgrate_la_CFLAGS = \
	$(PNG_CFLAGS)

libgrate_la_SOURCES = \
	gr2d.c \
	gr2d.h \
	gr3d.c \
	gr3d.h \
	grate.c \
	grate.h \
	nvhost.c \
	nvhost.h \
	nvmap.c \
	nvmap.h \
	matrix.c \
	matrix.h

libgrate_la_LIBADD = \
	$(PNG_LIBS) \
	-lm

if ENABLE_CGC
libgrate_la_SOURCES += \
	shader-cgc.c

libgrate_la_LIBADD += \
	../libcgc/libcgc.la
else
libgrate_la_SOURCES += \
	shader-dummy.c
endif
