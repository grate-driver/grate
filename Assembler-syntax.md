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

	OPCODE rD.mask, mod(rA.swizzle), mod(rB.swizzle), mod(rC.swizzle)

- OPCODE is one of scalar/vector opcodes.
- rD mask "xyzw" / "*" defines the write-enable mask.
- The source register modifier "mod" is optional, where "mod" is "abs" (absolute) or "neg" (negate) or it's combination.
- The scalar NOPs or vector NOPv operations could be omitted for brevity.
- Source registers (rA, rB, rC) are: general purpose rN, where N is 0-31; constant c[N], where N is 0-255; attribute
a[N], where N is 0-31.
- Destination register rD is rN or address register A0, depending on the operation.
- Relative addressing could be applied to the constant, attribute and export registers, hence they are given in square brackets.
- Source register swizzle "xyzw", swizzles the respective components of the source register.

Example:

	.asm
	EXEC(export[A0.z + 3]=vector)
		MOVv r0.x**w, neg(abs(a[0].xyzw))
		NOPs
	;

Here scalar operation is NOP, vector operation is MOV: the content of "x" and "w" components of an attribute register a0 (absolute'd and negated) will be written to the respective components of the destination register r0, as well as to the export register, which is addressed relatively to the address register A0.z by +3. 

## Fragment assembler

TBD

## Linker assembler

The linker assembler defines which vertex export registers will be copied to the TRAM, hence the (from)export register-(to)TRAM row locations, to what format the exported vertex register components will be converted during the copying to the TRAM and how the TRAM row component will be interpolated during the rasterization stage of graphics pipeline.

There is only one LINK instruction, which takes the following form:

	LINK fmt (mod), fmt (mod), fmt (mod), fmt (mod), tramN, exportM.swizzle

- fmt - destination TRAM component format:
	- fp20 - 20bit float
	- fx10.l - 10bit fixed point float, the low halve of the TRAM component 
	- fx10.h - 10bit fixed point float, the high halve of the TRAM component 
	- NOP - the TRAM component is "skipped", i.e. unaffected by the LINK operation
- mod - interpolation modifiers, given in parens:
	- dis - interpolation disable
- tramN - the TRAM row N, where N is 0..31
- exportM.swizzle - the exported vertex register M, where M is 0..15 and swizzle is "xyzw"

The first "fmt" operand represents the TRAM's row "x" component, second "y", third "w" and fourth "w".

Example:

	LINK fp20, NOP, fp20 (dis), fx10.h, tram0, export1.zyzx

Here the content of the VEC4 vertex export register 1 will be swizzled and copied to the TRAM row 0, so that:
- export1.z => converted to fp20 => copied to the tram0.x
- export1.y => skipped => the content of tram0.y is not altered
- export1.z => converted to fp20 => copied to the tram0.z and interpolation parameter "interpolation disable" is set for the the tram0.z
- export1.x => converted to fx10 => copied to the high halve of the tram0.w

