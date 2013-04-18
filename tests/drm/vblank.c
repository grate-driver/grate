#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <xf86drm.h>

int main(int argc, char *argv[])
{
	drmVBlank vbl;
	int fd, err;

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "open() failed: %m\n");
		return 1;
	}

	while (true) {
		memset(&vbl, 0, sizeof(vbl));
		vbl.request.type = DRM_VBLANK_RELATIVE;
		vbl.request.sequence = 1;
		vbl.request.signal = 0;

		err = drmWaitVBlank(fd, &vbl);
		if (err < 0) {
			fprintf(stderr, "drmWaitVBlank() failed: %m\n");
			return 1;
		}

		fprintf(stdout, "reply:\n");
		fprintf(stdout, "  type: %x\n", vbl.reply.type);
		fprintf(stdout, "  sequence: %u\n", vbl.reply.sequence);
		fprintf(stdout, "  tval_sec: %ld\n", vbl.reply.tval_sec);
		fprintf(stdout, "  tval_usec: %ld\n", vbl.reply.tval_usec);
	}

	close(fd);
	return 0;
}
