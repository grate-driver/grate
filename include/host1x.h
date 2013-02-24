#ifndef GRATE_HOST1X_H
#define GRATE_HOST1X_H 1

#include <stdint.h>
#include <stdio.h>

#ifndef BIT
#define BIT(x) (1 << (x))
#endif

struct host1x_stream {
	const uint32_t *words;
	const uint32_t *ptr;
	const uint32_t *end;
};

void host1x_stream_init(struct host1x_stream *stream, const void *buffer,
			size_t size);
void host1x_stream_dump(struct host1x_stream *stream, FILE *fp);

#endif
