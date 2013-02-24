#ifndef GRATE_CGDRV_H
#define GRATE_CGDRV_H 1

#include <stdint.h>
#include <stdlib.h>

enum cgdrv_shader_type {
	CGDRV_SHADER_VERTEX = 1,
	CGDRV_SHADER_FRAGMENT,
};

struct CgDrv {
	unsigned int unknown00;
	const char *error;
	const char *log;
	size_t length;
	void *stream;
	unsigned int unknown14;
	void *binary;
	size_t binary_size;
	unsigned int unknown20;
	unsigned int unknown24;
	unsigned int unknown28;
	unsigned int unknown2c;
	unsigned int unknown30;
	unsigned int unknown34;
	unsigned int unknown38;
	unsigned int unknown3c;
	unsigned int unknown40;
	unsigned int unknown44;
	unsigned int unknown48;
	unsigned int unknown4c;
};

struct CgDrv *CgDrv_Create(void);
void CgDrv_Delete(struct CgDrv *cgdrv);
void CgDrv_CleanUp(struct CgDrv *cgdrv);
int CgDrv_Compile(struct CgDrv *cgdrv, int unknown, int type,
		  const char *code, size_t length, int unknown2);

struct cgdrv_shader_symbol {
	uint32_t unknown00;
	uint32_t unknown01;
	/*
	 * type? seen:
	 * - 0x1005 for attributes and output variables
	 * - 0x1006 for uniforms
	 * - 0x1007 for constants
	 */
	uint32_t unknown02;
	uint32_t unknown03;
	uint32_t name_offset;
	uint32_t values_offset;
	uint32_t unknown06;
	uint32_t alt_offset;
	uint32_t unknown08;
	uint32_t unknown09;
	uint32_t unknown10;
	uint32_t unknown11;
};

struct cgdrv_shader_header {
	uint32_t type;
	uint32_t unknown00;
	uint32_t size;
	uint32_t num_symbols;
	uint32_t bar_size;
	uint32_t bar_offset;
	uint32_t binary_size;
	uint32_t binary_offset;
	uint32_t unknown01;
	uint32_t unknown02;
	uint32_t unknown03;
	uint32_t unknown04;

	struct cgdrv_shader_symbol symbol[0];
};

struct cgdrv_vertex_shader {
	uint32_t unknown00;
	uint32_t unknown04;
	uint32_t unknown08;
	uint32_t unknown0c;
	uint32_t unknown10;
	uint32_t unknown14;
	uint32_t unknown18;
	uint32_t unknown1c;
	uint32_t unknown20;
	uint32_t unknown24;
	uint32_t unknown28;
	uint32_t unknown2c;
	uint32_t unknown30;
	uint32_t unknown34;
	uint32_t unknown38;
	uint32_t unknown3c;
	uint32_t unknown40;
	uint32_t unknown44;
	uint32_t unknown48;
	uint32_t unknown4c;
	uint32_t unknown50;
	uint32_t unknown54;
	uint32_t unknown58;
	uint32_t unknown5c;
	uint32_t unknown60;
	uint32_t unknown64;
	uint32_t unknown68;
	uint32_t unknown6c;
	uint32_t unknown70;
	uint32_t unknown74;
	uint32_t unknown78;
	uint32_t unknown7c;
	uint32_t unknown80;
	uint32_t unknown84;
	uint32_t unknown88;
	uint32_t unknown8c;
	uint32_t unknown90;
	uint32_t unknown94;
	uint32_t unknown98;
	uint32_t unknown9c;
	uint32_t unknowna0;
	uint32_t unknowna4;
	uint32_t unknowna8;
	uint32_t unknownac;
	uint32_t unknownb0;
	uint32_t unknownb4;
	uint32_t unknownb8;
	uint32_t unknownbc;
	uint32_t unknownc0;
	uint32_t unknownc4;
	uint32_t unknownc8;
	uint32_t unknowncc;
	uint32_t unknownd0;
	uint32_t unknownd4;
	uint32_t unknownd8;
	uint32_t unknowndc;
	uint32_t unknowne0;
	uint32_t unknowne4;
	uint32_t unknowne8;
	uint32_t unknownec;
	uint32_t unknownf0;
	uint32_t unknownf4;
	uint32_t unknownf8;
	uint32_t unknownfc;
};

struct cgdrv_fragment_shader {
	char signature[8];
	uint32_t unknown0;
	uint32_t unknown1;
	uint32_t words[0];
};

struct cgdrv_shader {
	enum cgdrv_shader_type type;

	void *binary;
	size_t size;

	void *stream;
	size_t length;
};

struct cgdrv_shader *cgdrv_compile(enum cgdrv_shader_type type,
				   const char *code, size_t size);
void cgdrv_shader_free(struct cgdrv_shader *shader);
void cgdrv_shader_dump(struct cgdrv_shader *shader, FILE *fp);

#endif
