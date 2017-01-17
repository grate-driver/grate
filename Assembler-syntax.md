## Vertex assembler

The vertex assembler program is defined by the four code sections:

#### .attributes section

Defines input vertex attributes that will be enabled in the input attributes mask.

Example:

	.attributes
		[0] = "position";
		[1] = "color";

Here attributes 0 and 1 will be enable in the input mask. 

#### .exports section

Defines output vertex registers that will be enabled in the output attributes mask.

Example:

	.exports
		[0] = "gl_Position";
		[7] = "vcolor";

Here export registers 0 and 7 will be enable in the output mask. 

#### .constants section

Defines 32bit float constants that will be populated into the constant/uniform vec4 vertex registers.

Example:

	.constants
		[0].y  = 1.5;
		[12].z = -100.0;

Here a float value 1.5 will be loaded to the "y" component of the constant register 0 and value -100.0 to the "z" of register 12.

#### .asm section

Defines the actual vertex assembler. Instruction consists of the "EXEC" preamble, modifiers given in parens, one vector operation and one scalar operation, a termination semicolon.

Preamble:

There are two kinds of the instruction preamble: a regular "EXEC" and "EXEC_END". The latter will set the instructions "end of program" bit. 

Modifiers:
- p.xyzw - predicate swizzle, swizzles the condition register components when performing condition check
- cs - condition set
- eq - condition register state test: equal to 0
- lt - condition register state test: less than 0
- gt - condition register state test: greater than 0
- cc - condition check enable
- cwr - condition register write enable
- cr=n - selects condition register "n" to use, where "n" is either 0 or 1.
- export[n]=src - selects export register "n" and it's "src", where "src" is either "vector" or "scalar"

Operations:

A typical operation takes the following form:

	OPCODE rD.mask, -mod(rA.swizzle), -mod(rB.swizzle), -mod(rC.swizzle)

- OPCODE is one of scalar/vector opcodes.
- rD mask "xyzw" / "*" defines the write-enable mask.
- The source register modifier "mod" is optional, where "mod" is "abs" (absolute).
- The scalar NOPs or vector NOPv operations could be omitted for brevity.
- Source registers (rA, rB, rC) are: general purpose rN, where N is 0-31; constant c[N], where N is 0-255; attribute
a[N], where N is 0-31.
- Destination register rD is rN or address register A0, depending on the operation.
- Relative addressing could be applied to the constant, attribute and export registers, hence they are given in square brackets.
- Source register swizzle "xyzw", swizzles the respective components of the source register.

Example:

	.asm
	EXEC(export[A0.z + 3]=vector)
		MOVv r0.x**w, -abs(a[0].xyzw)
		NOPs
	;

Here scalar operation is NOP, vector operation is MOV: the content of "x" and "w" components of an attribute register a0 (absolute'd and negated) will be written to the respective components of the destination register r0, as well as to the export register, which is addressed relatively to the address register A0.z by +3. 

## Fragment assembler

The fragment assembler is defined by the fragment program parameters, the uniform constants and the instructions.

#### Parameters

Fragment program parameters must precede the .constants and .asm sections, they are:
- alu_buffer_size = N, where N is 1..4 - somewhat defines the number of pixel packets (row registers r[i].xyzw) kept in the ALU buffer.
- pseq_to_dw_exec_nb = N, where N is 0..? - the number of PSEQ instructions executed before the non-NOP DW operation.

#### Constants

The .constants code section defines the fragment uniform constants. The postfix of the constant register index defines the format of the float value and it's sub-location within the register value:
- ".l" and ".h" the float is converted to fx10 and stored in low/high halves of the constant register respectively
- no postfix - float is converted to fp20

A 32bit raw hex value could be used as well.

Example:

	.constants
		[0]   = 0.1;
		[1].l = 0.2;
		[1].h = -0.3;
		[2]   = 0x12345678;

Here: 0.1 is stored in the uniform register 0 as fp20, 0.2 is stored in the low halve uniform register 1 as fx10, -0.3 is stored in the high halve uniform register 1 as fx10, a raw hex value 0x12345678 is written to the uniform register 2.

#### Instructions

The .asm section code section defines the actual fragment program instructions. The instruction starts with the "EXEC" preamble, consists of 5 sub-instructions, each representing the respective stage of the fragment processor pipeline, and ends with the termination semicolon.

Fragment instruction takes the following form:

	EXEC
		PSEQ:	OPCODE operands
		MFU:	OPCODE operands
		TEX:	OPCODE operands
		ALU0:	OPCODE operands
		ALU1:	OPCODE operands
		ALU2:	OPCODE operands
		ALU3:	OPCODE operands
		ALU_COMPLEMENT: HEX
		DW:	OPCODE operands
	;

For brevity, "OPCODE operands" could be replaced with a "NOP", so the sub-instruction will be set to the respective No-Operation opcode.

Example:

	EXEC
		PSEQ:	NOP
		MFU:	NOP
		TEX:	NOP
		ALU0:	NOP
		ALU1:	NOP
		ALU2:	NOP
		ALU3:	NOP
		DW:	NOP
	;

Here all sub-instructions are NOP's.

The instruction schedule allows up to 3 MFU and 3 ALU sub-instructions to be executed in series, as well as skipping them.

To execute more than one MFU and/or ALU instruction in series, additional sub-instruction simply should be appended to the respective stage.

Example:

	EXEC
		PSEQ:	OPCODE operands

		MFU:	OPCODE operands
		MFU:	OPCODE operands
		MFU:	OPCODE operands

		TEX:	OPCODE operands

		ALU0:	OPCODE operands
		ALU1:	OPCODE operands
		ALU2:	OPCODE operands
		ALU3:	OPCODE operands

		ALU0:	OPCODE operands
		ALU1:	OPCODE operands
		ALU2:	OPCODE operands
		ALU3:	OPCODE operands
		ALU_COMPLEMENT: HEX

		DW:	OPCODE operands
	;

Here three MFU sub-instructions and two ALU sub-instructions will be executed consecutively.

To skip the MFU and/or ALU stages, i.e. to not schedule them during an exec batch, they (sub-instructions) should omitted from the instruction.

Example:

	EXEC
		PSEQ:	OPCODE operands
		TEX:	OPCODE operands
		DW:	OPCODE operands
	;

Here no MFU and no ALU sub-instructions will be scheduled-executed.

For brevity, PSEQ/TEX/DW sub-instructions and ALU_COMPLEMENT could be omitted from the instruction exec batch, they will be NOP's effectively.

Example:

	EXEC
		MFU:	OPCODE operands
	;

Here PSEQ/TEX/DW are NOP's, one MFU sub-instruction is scheduled and ALU stage is skipped.

Since not all sub-instructions are known yet, the "OPCODE operands" could be replaced with a raw hex value. The number of the hex words depends on the size of the sub-instruction; for example MFU instruction is 64bit sized, so two hex words, separated by a comma, will be necessary for it.

Example:

	EXEC
		MFU:	0x00000000, 0x12345678
	;

Here the MFU sub-instruction is represented in raw by a two hex values, 0x00000000 is a high 64bit halve and 0x12345678 is low.

#### PSEQ sub-instruction

Opcodes are unknown, available only in a hex form.

Example:

	EXEC
		PSEQ:	0x00000000
	;

#### MFU sub-instruction

There are two forms of the MFU sub-instruction: varying and special function.

The varying operation:

	var unk(HEX value) mod(tN.fmt), mod(tN.fmt), mod(tN.fmt), mod(tN.fmt)

- an optional unk(HEX value) - defines the unknown bits o the varying operation, interpolation parameters it seems. When omitted, it is treated as unk(0x0).
- tN.fmt - TRAM row N will be read in as one fp20 or two fx10 (fmt), or could be a NOP to skip the read of a row component. The first tN.fmt operand is the TRAM's row "x" component and so on.
- an optional sat(tN.fmt) saturate modifier could be applied to the operand

Example:

	EXEC
		MFU:	var unk(0x104E51BA0) t2.fx10, t7.fp20, NOP, sat(t5.fp20)
	;

Here the varying sub-instruction performs a read from TRAM, interpolates the read components and stores the result into the "pixel packet" r0, r1, r2, r3 row registers.
- tram2.x read as two fx10 and stored into the r0 of the "pixel packet" row
- tram7.y read as fp20 and stored into the r1 of the "pixel packet" row
- the r2 of the "pixel packet" row is untouched
- tram5.w read as fp20, clamped to 0..1 and stored into the r3 of the "pixel packet" row

The special function:

	OPCODE operand

- OPCODE is one of the MFU special function operations.
- operand is a "pixel packet" row register in fp20 format to which the operation will be applied.

Example:

	EXEC
		MFU:	RCP r1
	;

Here a reciprocal operation is applied to the register r1, r1 = 1.0 / r1.

#### TEX sub-instruction

Opcodes are unknown, available only in a hex form.

Example:

	EXEC
		TEX:	0x00000000
	;

#### ALU sub-instruction

The ALU sub-instruction consists of four ALU instructions, one per ALU, and an ALU_COMPLEMENT hex value (one per exec batch).

ALU instruction takes the following form:

	OPCODE rDst.mask, -mod(rA)*2-1, -mod(rA)*2-1, -mod(rA)*2-1, -mod(rA)*2-1 (modifier)

- OPCODE is one of the ALU operations: MAD, MUL, MIN, MAX or CSEL. The MUL operations is effectively a MAD with an "addition disable" bit set.
- rDst is the destination register; it's mask is 'lh', 'l\*', '\*h' or '\*\*', where 'l' - enabled write to the low halve, 'h' - to the high and '*' disables the write to the respective halve. The write-mask defines the destination format for the MAD/CSEL instructions: both halves - fp20, one halve - fx10 low/high.
- rA, rB, rC and rD are the source registers. The format is defined by a register postfix:
	- ".l" - fx10 low halve of the source register is read
	- ".h" - fx10 high halve of the source register is read
	- no postfix - the source register is read as fp20
- optional operand rX modifier, X is for A-B-C-D:
	- absolute modifier, abs(rX)
	- multiply by 2, rX * 2 (not applicable to rD)
	- decrement by 1.0, rX - 1; only affects the fx10 typed rX
	- negate modifier, -rX (not applicable to rD)
- optional instruction modifiers are given in parens after the operands:
	- this - Sets "accumulate this" ALU bit.
	- other - Sets "accumulate other" ALU bit.
	- eq - rDst result "equal to 0" ? 1.0 : 0.0.
	- gt - rDst result "greater than 0" ? 1.0 : 0.0.
	- ge - rDst result "greater or equal to 0" ? 1.0 : 0.0.
	- x2 - rDst result multiplied by 2.
	- x4 - rDst result multiplied by 4.
	- /2 - rDst result divided by 2.
	- sat - rDst result is clamped to 0..1.

ALU registers:
- rN - ALU buffer row register, N is 0..15 (4 registers per row)
- gN - General purpose register, N is 0..8
- aluN - ALUN result register, N is 0..3
- immN - ALU3 immediate constant, N is 0..2
- lp - Low precision register, the "lp" form is used only for the destination register, while source registers uses "#0" or "#1" form. When rD is set to #1, it's enable bit is unset, making rD 1.0 effectively.
- uN - Uniform register, N is 0..31
- crN - Condition register, N is 0..15
- posx - Fragment position X + 8192.0
- posy - 8192.0 + (target height - 1) - Fragment position Y
- pface - Polygon face direction
- rB / rC - Only applicable to the rD. The rD source is a copy of either ALU source register rB or rC.
- kill - discard the fragment

Example:

	EXEC
		ALU0:	MUL  u3.lh,  r3,         posx,       r1.l - 1,   rB        (x2)(this)
		ALU1:	CSEL cr6,    r2,         u5.h,       #0,         #1
		ALU2:	MIN  r2.*h,  r1,         #1,         r2 * 2,     abs(rC)
		ALU3:	MAD  r2.l*,  r0,         #1,         alu2,       #1
	;

The ALU3 could be traded for the immediate constants, it takes the following form:

	imm0 = float, imm1 = float, imm2 = float

If immN is postfix'ed with ".l" / ".h", the fx10 representation of a float value will be loaded into the respective halve of the immediate constant. When not prefix'ed, the fp20 representation is loaded. 

Example:

	EXEC
		ALU0:	MAD  cr0,    imm0.h,     #1,         imm1,       rB   (gt)
		ALU1:	MAD  lp,     -posx,      #1,         #0,         #1   
		ALU2:	NOP 
		ALU3:	imm0.h = -0.187500, imm1 = 8192.000000
	;

#### DW sub-instruction

Opcodes are unknown, available only in a hex form.

Example:

	EXEC
		DW:	0x00020005
	;

## Linker assembler

The linker assembler defines which vertex export registers will be copied to the TRAM, hence the (from)export register-(to)TRAM row locations, to what format the exported vertex register components will be converted during the copying to the TRAM and how the TRAM row component will be interpolated during the rasterization stage of graphics pipeline.

There is only one LINK instruction, which takes the following form:

	LINK fmt (mod), fmt (mod), fmt (mod), fmt (mod), tramN.swizzle, exportM

- fmt - destination TRAM component format:
	- fp20 - 20bit float
	- fx10.l - 10bit fixed point float, the low halve of the TRAM component 
	- fx10.h - 10bit fixed point float, the high halve of the TRAM component 
	- NOP - the TRAM component is "skipped", i.e. unaffected by the LINK operation
- mod - interpolation modifiers, given in parens:
	- dis - interpolation disable
- tramN.swizzle - the TRAM row N, where N is 0..15 and destination components swizzle is "xyzw"
- exportM - the exported vertex register M, where M is 0..15

Example:

	LINK fp20, NOP, fp20 (dis), fx10.h, tram0.ywzx, export1

Here the content of the VEC4 vertex export register 1 copied to the TRAM row 0, so that:
- export1.x => converted to fp20 => copied to the tram0.y
- export1.y => skipped => the content of tram0.w is not altered
- export1.z => converted to fp20 => copied to the tram0.z and interpolation parameter "interpolation disable" is set for the the tram0.z
- export1.w => converted to fx10 => copied to the high halve of the tram0.x
