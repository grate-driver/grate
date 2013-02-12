# beautify output
ifneq ($(V),1)
	Q = @
else
	Q =
endif

CC = $(CROSS_COMPILE)gcc
CFLAGS = -O2 -g -Wall -Werror $(EXTRA_CFLAGS)
LDFLAGS = $(EXTRA_LDFLAGS)

AR = ar
ARFLAGS = rcu

ARCHIVES = tests/3d/libcommon.a
LIBRARIES = wrap/libwrap.so

PROGRAMS  = tests/3d/egl-triangle tests/3d/egl-x11-triangle
PROGRAMS += tests/3d/egl-clear tests/3d/egl-x11-clear
PROGRAMS += tests/3d/gles-clear tests/3d/gles-shader-fill
PROGRAMS += tests/3d/gles-pbuffer-clear tests/3d/gles-pbuffer-fill

all: $(ARCHIVES) $(LIBRARIES) $(PROGRAMS)

tests/3d/libcommon.a_SOURCES = tests/3d/common.c

wrap/libwrap.so_SOURCES = wrap/syscall.c wrap/utils.c wrap/nvhost.c
wrap/libwrap.so_CFLAGS = -fPIC
wrap/libwrap.so_LIBS = -ldl

tests/3d/egl-triangle_SOURCES = tests/3d/egl-triangle.c
tests/3d/egl-triangle_LIBS = -lEGL -lGLESv2

tests/3d/egl-x11-triangle_SOURCES = tests/3d/egl-x11-triangle.c
tests/3d/egl-x11-triangle_LIBS = -lX11 -lEGL -lGLESv2

tests/3d/egl-clear_SOURCES = tests/3d/egl-clear.c
tests/3d/egl-clear_LIBS = -lEGL -lGLESv2

tests/3d/egl-x11-clear_SOURCES = tests/3d/egl-x11-clear.c
tests/3d/egl-x11-clear_LIBS = -lX11 -lEGL -lGLESv2

tests/3d/gles-clear_SOURCES = tests/3d/gles-clear.c
tests/3d/gles-clear_LIBS = tests/3d/libcommon.a -lpng -lX11 -lEGL -lGLESv2
tests/3d/gles-clear: tests/3d/libcommon.a

tests/3d/gles-shader-fill_SOURCES = tests/3d/gles-shader-fill.c
tests/3d/gles-shader-fill_LIBS = tests/3d/libcommon.a -lpng -lX11 -lEGL -lGLESv2
tests/3d/gles-shader-fill: tests/3d/libcommon.a

tests/3d/gles-pbuffer-clear_SOURCES = tests/3d/gles-pbuffer-clear.c
tests/3d/gles-pbuffer-clear_LIBS = tests/3d/libcommon.a -lpng -lX11 -lEGL -lGLESv2
tests/3d/gles-pbuffer-clear: tests/3d/libcommon.a

tests/3d/gles-pbuffer-fill_SOURCES = tests/3d/gles-pbuffer-fill.c
tests/3d/gles-pbuffer-fill_LIBS = tests/3d/libcommon.a -lpng -lX11 -lEGL -lGLESv2
tests/3d/gles-pbuffer-fill: tests/3d/libcommon.a

define archives_rule
$(archive)_OBJECTS = $$($(archive)_SOURCES:.c=.o)
$(archive)_DEPENDS = $$($(archive)_SOURCES:.c=.d)

$$($(archive)_OBJECTS): %.o: %.c
ifneq ($$(V),1)
	@echo '  CC      $$@'
endif
	$(Q)$$(CC) -MD $$(CFLAGS) $$($(archive)_CFLAGS) -o $$@ -c $$<

$(archive): $$($(archive)_OBJECTS)
ifneq ($$(V),1)
	@echo '  AR      $$@'
endif
	$(Q)$$(AR) $$(ARFLAGS) $$($(archive)_ARFLAGS) -o $$@ $$($(archive)_OBJECTS)

-include $$($(archive)_DEPENDS)

CLEANFILES += $$($(archive)_OBJECTS) $$($(archive)_DEPENDS) $(archive)
endef

$(foreach archive,$(ARCHIVES),$(eval $(archives_rule)))

define libraries_rule
$(library)_OBJECTS = $$($(library)_SOURCES:.c=.o)
$(library)_DEPENDS = $$($(library)_SOURCES:.c=.d)

$$($(library)_OBJECTS): %.o: %.c
ifneq ($$(V),1)
	@echo '  CC      $$@'
endif
	$(Q)$$(CC) -MD $$(CFLAGS) $$($(library)_CFLAGS) -o $$@ -c $$<

$(library): $$($(library)_OBJECTS)
ifneq ($$(V),1)
	@echo '  LD      $$@'
endif
	$(Q)$$(CC) $$(CFLAGS) $$($(library)_CFLAGS) -o $$@ -shared $$($(library)_OBJECTS) $$($(library)_LIBS)

-include $$($(library)_DEPENDS)

CLEANFILES += $$($(library)_OBJECTS) $$($(library)_DEPENDS) $(library)
endef

$(foreach library,$(LIBRARIES),$(eval $(libraries_rule)))

define programs_rule
$(program)_OBJECTS = $$($(program)_SOURCES:.c=.o)
$(program)_DEPENDS = $$($(program)_SOURCES:.c=.d)

$$($(program)_OBJECTS): %.o: %.c
ifneq ($$(V),1)
	@echo '  CC      $$@'
endif
	$(Q)$$(CC) -MD $$(CFLAGS) -o $$@ -c $$<

$(program): $$($(program)_OBJECTS)
ifneq ($$(V),1)
	@echo '  LD      $$@'
endif
	$(Q)$$(CC) $$(CFLAGS) $$($(program)_CFLAGS) -o $$@ $$($(program)_OBJECTS) $$(LIBS) $$($(program)_LIBS)

-include $$($(program)_DEPENDS)

CLEANFILES += $$($(program)_OBJECTS) $$($(program)_DEPENDS) $(program)
endef

$(foreach program,$(PROGRAMS),$(eval $(programs_rule)))

gr2d-test: gr2d-test.c
	$(CC) $(CFLAGS) -o $@ $< -lpng

hex2float: hex2float.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(CLEANFILES)
