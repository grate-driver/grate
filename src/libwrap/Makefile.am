lib_LTLIBRARIES = libgrate-wrap.la

if ENABLE_RNN
libgrate_wrap_la_CPPFLAGS = -DENABLE_RNN
endif

libgrate_wrap_la_SOURCES = \
	cgdrv.c \
	drm.h \
	drm_mode.h \
	list.h \
	nvhost.c \
	nvhost.h \
	syscall.c \
	tegra_drm.h \
	utils.c \
	utils.h

libgrate_wrap_la_LIBADD = -ldl
if ENABLE_RNN
libgrate_wrap_la_LIBADD += -lrnn -lenvyutil -lxml2
endif
