#ifndef FRAGMENT_ASM_H
#define FRAGMENT_ASM_H

#include <stdint.h>

#define FRAG_ROW_REG_0				0
#define FRAG_ROW_REG_15				15
#define FRAG_ROW_REG(r)				(FRAG_ROW_REG_0 + (r))
#define FRAG_GENERAL_PURPOSE_REG_0		16
#define FRAG_GENERAL_PURPOSE_REG_7		23
#define FRAG_GENERAL_PURPOSE_REG(r)		(FRAG_GENERAL_PURPOSE_REG_0 + (r))
#define FRAG_ALU_RESULT_REG_0			24
#define FRAG_ALU_RESULT_REG_3			27
#define FRAG_ALU_RESULT_REG(r)			(FRAG_ALU_RESULT_REG_0 + (r))
#define FRAG_EMBEDDED_CONSTANT_0		28
#define FRAG_EMBEDDED_CONSTANT_2		30
#define FRAG_EMBEDDED_CONSTANT(r)		(FRAG_EMBEDDED_CONSTANT_0 + (r))
#define FRAG_LOWP_VEC2_0_1			31
#define FRAG_UNIFORM_REG_0			32
#define FRAG_UNIFORM_REG_31			63
#define FRAG_UNIFORM_REG(r)			(FRAG_UNIFORM_REG_0 + (r))
#define FRAG_CONDITION_REG_0			64
#define FRAG_CONDITION_REG_7			71
#define FRAG_CONDITION_REG(r)			(FRAG_CONDITION_REG_0 + (r))
#define FRAGMENT_POS_X				72
#define FRAGMENT_POS_Y				73
#define FRAG_POLYGON_FACE			75

typedef union fragment_instruction_schedule {
	struct __attribute__((packed)) {
		unsigned instructions_nb:2;
		unsigned address:6;
	};

	uint32_t data;
} instr_sched;

#define MFU_NOP		0
#define MFU_RCP		1
#define MFU_RSQ		2
#define MFU_LG2		3
#define MFU_EX2		4
#define MFU_SQRT	5
#define MFU_SIN		6
#define MFU_COS		7
#define MFU_FRC		8
#define MFU_PREEX2	9
#define MFU_PRESIN	10
#define MFU_PRECOS	11

#define MFU_VAR_NOP	0
#define MFU_VAR_FP20	1
#define MFU_VAR_FX10	2

typedef union fragment_mfu_instruction {
	struct __attribute__((packed)) {
		unsigned var0_saturate:1;
		unsigned var0_opcode:2;
		unsigned var0_source:4;

		unsigned var1_saturate:1;
		unsigned var1_opcode:2;
		unsigned var1_source:4;

		unsigned var2_saturate:1;
		unsigned var2_opcode:2;
		unsigned var2_source:4;

		unsigned var3_saturate:1;
		unsigned var3_opcode:2;
		unsigned var3_source:4;

		uint64_t unk_varying:36;
	};

	struct __attribute__((packed)) {
		uint32_t __pad1;
		unsigned __pad2:22;

		unsigned opcode:4;
		unsigned reg:6;
	};

	struct __attribute__((packed)) {
		uint32_t part0;
		uint32_t part1;
	};
} mfu_instr;

#define ALU_CC_NOP			0
#define ALU_CC_ZERO			1
#define ALU_CC_GREATER_THAN_ZERO	2
#define ALU_CC_ZERO_OR_GREATER		3

#define ALU_OPCODE_MAD			0
#define ALU_OPCODE_MIN			1
#define ALU_OPCODE_MAX			2
#define ALU_OPCODE_CSEL			3

#define ALU_NO_SCALE			0
#define ALU_SCALE_X2			1
#define ALU_SCALE_X4			2
#define ALU_SCALE_DIV2			3

union fragment_alu_instruction {
	struct __attribute__((packed)) {
		unsigned rD_fixed10:1;
		unsigned rD_absolute_value:1;
		unsigned rD_enable:1;
		unsigned rD_minus_one:1;
		unsigned rD_sub_reg_select_high:1;
		unsigned rD_reg_select:1;

		unsigned rC_scale_by_two:1;
		unsigned rC_negate:1;
		unsigned rC_absolute_value:1;
		unsigned rC_fixed10:1;
		unsigned rC_minus_one:1;
		unsigned rC_sub_reg_select_high:1;
		unsigned rC_reg_select:7;

		unsigned rB_scale_by_two:1;
		unsigned rB_negate:1;
		unsigned rB_absolute_value:1;
		unsigned rB_fixed10:1;
		unsigned rB_minus_one:1;
		unsigned rB_sub_reg_select_high:1;
		unsigned rB_reg_select:7;

		unsigned rA_scale_by_two:1;
		unsigned rA_negate:1;
		unsigned rA_absolute_value:1;
		unsigned rA_fixed10:1;
		unsigned rA_minus_one:1;
		unsigned rA_sub_reg_select_high:1;
		unsigned rA_reg_select:7;

		unsigned write_low_sub_reg:1;
		unsigned write_high_sub_reg:1;
		unsigned dst_reg:5;
		unsigned write_uniform:1;
		unsigned write_condition_reg:1;
		unsigned condition_code:2;
		unsigned saturate_result:1;
		unsigned scale_result:2;

		unsigned addition_disable:1;
		unsigned accumulate_result_this:1;
		unsigned accumulate_result_other:1;
		unsigned opcode:2;
	};

	struct __attribute__((packed)) {
		uint32_t part0;
		uint32_t part1;
	};
};

typedef union fragment_alu_instruction_x4 {
	struct __attribute__((packed)) {
		union fragment_alu_instruction a[4];
	};

	union {
		struct __attribute__((packed)) {
			uint64_t __pad1;
			uint64_t __pad2;
			uint64_t __pad3;
			unsigned __pad4:4;
			unsigned fx10_low:10;
			unsigned fx10_high:10;
		};

		struct __attribute__((packed)) {
			uint64_t __pad5;
			uint64_t __pad6;
			uint64_t __pad7;
			unsigned __pad8:4;
			unsigned fp20:20;
		};
	} imm0;

	union {
		struct __attribute__((packed)) {
			uint64_t __pad1;
			uint64_t __pad2;
			uint64_t __pad3;
			unsigned __pad4:24;
			unsigned fx10_low:10;
			unsigned fx10_high:10;
		};

		struct __attribute__((packed)) {
			uint64_t __pad5;
			uint64_t __pad6;
			uint64_t __pad7;
			unsigned __pad8:24;
			unsigned fp20:20;
		};
	} imm1;

	union {
		struct __attribute__((packed)) {
			uint64_t __pad1;
			uint64_t __pad2;
			uint64_t __pad3;
			uint32_t __pad4;
			unsigned __pad5:12;
			unsigned fx10_low:10;
			unsigned fx10_high:10;
		};

		struct __attribute__((packed)) {
			uint64_t __pad6;
			uint64_t __pad7;
			uint64_t __pad8;
			uint32_t __pad9;
			unsigned __pad10:12;
			unsigned fp20:20;
		};
	} imm2;

	struct __attribute__((packed)) {
		uint32_t part0;
		uint32_t part1;
		uint32_t part2;
		uint32_t part3;
		uint32_t part4;
		uint32_t part5;
		uint32_t part6;
		uint32_t part7;

		uint32_t complement;
	};
} alu_instr;

typedef union fragment_pseq_instruction {
	uint32_t data;
} pseq_instr;

typedef union fragment_tex_instruction {
	uint32_t data;
} tex_instr;

typedef union fragment_dw_instruction {
	uint32_t data;
} dw_instr;

#endif // FRAGMENT_ASM_H
