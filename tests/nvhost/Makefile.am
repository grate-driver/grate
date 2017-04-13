noinst_LTLIBRARIES = libnvhost.la

libnvhost_la_SOURCES = \
	gr2d.c \
	gr2d.h \
	gr3d.c \
	gr3d.h \
	nvhost.c \
	nvhost.h \
	nvmap.c \
	nvmap.h

AM_CFLAGS = $(PNG_CFLAGS)

noinst_PROGRAMS = \
	gr2d-fill \
	gr3d-triangle

LDADD = libnvhost.la $(PNG_LIBS)
