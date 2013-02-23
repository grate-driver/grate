#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	uint32_t value;

	value = strtoul(argv[1], NULL, 16);

	value &= 0x3ff;

	if ((value >> 9) & 0x1)
		printf("%f\n", -(((value - 1) ^ 0x3ff) / 256.0f));
	else
		printf("%f\n", value / 256.0f);

	return 0;
}
