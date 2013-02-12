#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	union {
		uint32_t u;
		float f;
	} value;

	value.u = strtoul(argv[1], NULL, 16);
	printf("%f\n", value.f);

	return 0;
}
