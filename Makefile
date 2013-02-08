# beautify output
ifneq ($(V),1)
	Q = @
else
	Q =
endif

CC = $(CROSS_COMPILE)gcc
CFLAGS = -O2 -g -Wall -Werror $(EXTRA_CFLAGS)
LDFLAGS = $(EXTRA_LDFLAGS)

LIBRARIES = wrap/libwrap.so
PROGRAMS = tests/3d/egl-triangle tests/3d/egl-clear tests/3d/egl-x11-clear

all: $(LIBRARIES) $(PROGRAMS)

wrap/libwrap.so_SOURCES = wrap/syscall.c wrap/utils.c wrap/nvhost.c
wrap/libwrap.so_CFLAGS = -fPIC
wrap/libwrap.so_LIBS = -ldl

tests/3d/egl-triangle_SOURCES = tests/3d/egl-triangle.c
tests/3d/egl-triangle_LIBS = -lEGL -lGLESv2

tests/3d/egl-clear_SOURCES = tests/3d/egl-clear.c
tests/3d/egl-clear_LIBS = -lEGL -lGLESv2

tests/3d/egl-x11-clear_SOURCES = tests/3d/egl-x11-clear.c
tests/3d/egl-x11-clear_LIBS = -lX11 -lEGL -lGLESv2

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
	@$$(CC) $$(CFLAGS) $$($(library)_CFLAGS) -o $$@ -shared $$($(library)_OBJECTS) $$($(library)_LIBS)

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

clean:
	rm -f $(CLEANFILES)
