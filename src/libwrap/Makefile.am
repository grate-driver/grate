lib_LTLIBRARIES = libwrap.la

if ENABLE_RNN
libwrap_la_CPPFLAGS = -DENABLE_RNN
endif

libwrap_la_SOURCES = \
	cgdrv.c \
	list.h \
	nvhost.c \
	nvhost.h \
	syscall.c \
	utils.c \
	utils.h

libwrap_la_LIBADD = -ldl
if ENABLE_RNN
libwrap_la_LIBADD += -lrnn -lenvyutil -lxml2
endif
