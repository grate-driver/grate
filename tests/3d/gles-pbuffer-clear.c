#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <GLES2/gl2.h>

#include "common.h"

void pbuffer_draw(struct pbuffer *pbuffer)
{
	printf("=== calling glViewport()\n");
	glViewport(0, 0, pbuffer->width, pbuffer->height);
	glFlush();
	printf("=== calling glClearColor()\n");
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glFlush();
	printf("=== calling glClear()\n");
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();
}

int main(int argc, char *argv[])
{
	struct pbuffer *pbuffer;

	pbuffer = pbuffer_create(32, 32);
	if (!pbuffer) {
		fprintf(stderr, "pbuffer_create() failed\n");
		return 1;
	}

	pbuffer_draw(pbuffer);
	pbuffer_save(pbuffer, "test.png");
	pbuffer_free(pbuffer);

	return 0;
}
