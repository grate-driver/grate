CC = gcc
CFLAGS = -O2 -g -Wall -Werror

all: wrap/libwrap.so

libwrap_OBJS = wrap/syscall.o
libwrap_CFLAGS = $(CFLAGS) -fPIC
libwrap_LIBS = -ldl

$(libwrap_OBJS): %.o: %.c
	$(CC) $(libwrap_CFLAGS) -o $@ -c $<

wrap/libwrap.so: $(libwrap_OBJS)
	$(CC) $(libwrap_CFLAGS) -o $@ -shared $(libwrap_OBJS) $(libwrap_LIBS)

tests/3d/egl-triangle: %: %.c
	$(CC) $(CFLAGS) -o $@ $< -lEGL -lGLESv2

clean:
	rm -f wrap/libwrap.so $(libwrap_OBJS)
