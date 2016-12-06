#ifndef VPE_VLIW_H
#define VPE_VLIW_H

#include <stdint.h>

#define REG_TYPE_UNDEFINED	0
#define REG_TYPE_TEMPORARY	1
#define REG_TYPE_ATTRIBUTE	2
#define REG_TYPE_UNIFORM	3

#define SWIZZLE_X		0
#define SWIZZLE_Y		1
#define SWIZZLE_Z		2
#define SWIZZLE_W		3

#define VECTOR_OPCODE_NOP	0
#define VECTOR_OPCODE_MOV	1
#define VECTOR_OPCODE_MUL	2
#define VECTOR_OPCODE_ADD	3
#define VECTOR_OPCODE_MAD	4
#define VECTOR_OPCODE_DP3	5
#define VECTOR_OPCODE_DPH	6
#define VECTOR_OPCODE_DP4	7
#define VECTOR_OPCODE_DST	8
#define VECTOR_OPCODE_MIN	9
#define VECTOR_OPCODE_MAX	10
#define VECTOR_OPCODE_SLT	11
#define VECTOR_OPCODE_SGE	12
#define VECTOR_OPCODE_ARL	13
#define VECTOR_OPCODE_FRC	14
#define VECTOR_OPCODE_FLR	15
#define VECTOR_OPCODE_SEQ	16
#define VECTOR_OPCODE_SFL	17
#define VECTOR_OPCODE_SGT	18
#define VECTOR_OPCODE_SLE	19
#define VECTOR_OPCODE_SNE	20
#define VECTOR_OPCODE_STR	21
#define VECTOR_OPCODE_SSG	22
#define VECTOR_OPCODE_ARR	23
#define VECTOR_OPCODE_ARA	24
#define VECTOR_OPCODE_TXL	25
#define VECTOR_OPCODE_PUSHA	26
#define VECTOR_OPCODE_POPA	27

#define SCALAR_OPCODE_NOP	0
#define SCALAR_OPCODE_MOV	1
#define SCALAR_OPCODE_RCP	2
#define SCALAR_OPCODE_RCC	3
#define SCALAR_OPCODE_RSQ	4
#define SCALAR_OPCODE_EXP	5
#define SCALAR_OPCODE_LOG	6
#define SCALAR_OPCODE_LIT	7
#define SCALAR_OPCODE_BRA	9
#define SCALAR_OPCODE_CAL	11
#define SCALAR_OPCODE_RET	12
#define SCALAR_OPCODE_LG2	13
#define SCALAR_OPCODE_EX2	14
#define SCALAR_OPCODE_SIN	15
#define SCALAR_OPCODE_COS	16
#define SCALAR_OPCODE_PUSHA	19
#define SCALAR_OPCODE_POPA	20

typedef union vpe_instruction_u {
	struct __attribute__((packed)) {
		unsigned end_of_program:1;			//   0
		unsigned constant_relative_addressing_enable:1;	//   1
		unsigned export_write_index:5;			//   2 .. 6
		unsigned scalar_rD_index:6;			//   7 .. 12
		unsigned vector_op_write_w_enable:1;		//  13
		unsigned vector_op_write_z_enable:1;		//  14
		unsigned vector_op_write_y_enable:1;		//  15
		unsigned vector_op_write_x_enable:1;		//  16
		unsigned scalar_op_write_w_enable:1;		//  17
		unsigned scalar_op_write_z_enable:1;		//  18
		unsigned scalar_op_write_y_enable:1;		//  19
		unsigned scalar_op_write_x_enable:1;		//  20
		unsigned rC_type:2;				//  21 .. 22
		unsigned rC_index:6;				//  23 .. 28
		unsigned rC_swizzle_w:2;			//  29 .. 30
		unsigned rC_swizzle_z:2;			//  31 .. 32
		unsigned rC_swizzle_y:2;			//  33 .. 34
		unsigned rC_swizzle_x:2;			//  35 .. 36
		unsigned rC_negate:1;				//  37
		unsigned rB_type:2;				//  38 .. 39
		unsigned rB_index:6;				//  40 .. 45
		unsigned rB_swizzle_w:2;			//  46 .. 47
		unsigned rB_swizzle_z:2;			//  48 .. 49
		unsigned rB_swizzle_y:2;			//  50 .. 51
		unsigned rB_swizzle_x:2;			//  52 .. 53
		unsigned rB_negate:1;				//  54
		unsigned rA_type:2;				//  55 .. 56
		unsigned rA_index:6;				//  57 .. 62
		unsigned rA_swizzle_w:2;			//  63 .. 64
		unsigned rA_swizzle_z:2;			//  65 .. 66
		unsigned rA_swizzle_y:2;			//  67 .. 68
		unsigned rA_swizzle_x:2;			//  69 .. 70
		unsigned rA_negate:1;				//  71
		unsigned attribute_fetch_index:4;		//  72 .. 75
		unsigned uniform_fetch_index:10;		//  76 .. 85
		unsigned vector_opcode:5;			//  86 .. 90
		unsigned scalar_opcode:5;			//  91 .. 95
		unsigned address_register_select:2;		//  96 .. 97
		unsigned predicate_swizzle_w:2;			//  98 .. 99
		unsigned predicate_swizzle_z:2;			// 100 .. 101
		unsigned predicate_swizzle_y:2;			// 102 .. 103
		unsigned predicate_swizzle_x:2;			// 104 .. 105
		unsigned predicate_lt:1;			// 106
		unsigned predicate_eq:1;			// 107
		unsigned predicate_gt:1;			// 108
		unsigned condition_check:1;			// 109
		unsigned condition_set:1;			// 110
		unsigned vector_rD_index:6;			// 111 .. 116
		unsigned rA_absolute_value:1;			// 117
		unsigned rB_absolute_value:1;			// 118
		unsigned rC_absolute_value:1;			// 119
		unsigned bit120:1;				// 120
		unsigned condition_register_index:1;		// 121
		unsigned saturate_result:1;			// 122
		unsigned attribute_relative_addressing_enable:1;// 123
		unsigned export_relative_addressing_enable:1;	// 124
		unsigned condition_flags_write_enable:1;	// 125
		unsigned export_vector_write_enable:1;			// 126
		unsigned bit127:1;				// 127
	};

	struct __attribute__((packed)) {
		unsigned __pad:29;				//   0 .. 28
		unsigned iaddr:8;				//  29 .. 36
	};

	struct __attribute__((packed)) {
		uint32_t part0;
		uint32_t part1;
		uint32_t part2;
		uint32_t part3;
	};
} vpe_instr128;

#endif // VPE_VLIW_H
