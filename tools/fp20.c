#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	uint32_t value, sign, mantissa, exponent;
	union {
		uint32_t u;
		float f;
	} bits;

	value = strtoul(argv[1], NULL, 16);

	sign = (value >> 19) & 0x1;
	exponent = (value >> 13) & 0x3f;
	mantissa = (value >>  0) & 0x1fff;

	if (exponent == 0x3f)
		exponent = 0xff;
	else
		exponent += 127 - 31;

	mantissa = mantissa << (23 - 13);

	bits.u = sign << 31 | (exponent << 23) | mantissa;

	printf("%f\n", bits.f);

	return 0;
}
