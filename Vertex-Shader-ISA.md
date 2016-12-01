## Overview

The Tegra vertex shader ISA is a relatively straight-forward implementation of the Shader Model 2 instruction set. The instruction set seems to be a strict subset of the NV30 vertex-shader.

Each instruction contains up to two operations; one 4-component vector ALU (arithmetic logic unit) operation, and one 1-component SFU (special function unit) operation. The result of both units is a 4 component-vector, limited by a write-mask.

There's five operands, one destination register per unit (referred to as rD), and three source operands (referred to as rA, rB and rC). The ALU can use up to all three source operands, while the SFU only operates on rC.

Vertex processor has 32 local vec4 registers, 16 input vec4 attribute registers, 256 input vec4 constant registers, 16 export vec4 registers, 2 condition registers, 4 address registers. Maximum size of vertex program is 256 VLIW instructions.

### See also

Nouveau:

http://cgit.freedesktop.org/mesa/mesa/tree/src/gallium/drivers/nouveau/nv30/nv30_vertprog.h  
http://cgit.freedesktop.org/mesa/mesa/tree/src/gallium/drivers/nouveau/nv30/nv30_vertprog.c  
http://cgit.freedesktop.org/mesa/mesa/tree/src/gallium/drivers/nouveau/nv30/nvfx_vertprog.c  

Instruction set specifications:

https://www.opengl.org/registry/specs/NV/vertex_program.txt  
https://www.opengl.org/registry/specs/NV/vertex_program2.txt  
https://www.opengl.org/registry/specs/NV/vertex_program3.txt  

Patents:

https://www.google.com/patents/US7755634  

### Instruction word encoding

|     Bits | Meaning                     |
|---------:|:----------------------------|
|      127 | ???                         |
|      126 | export vector write enable  |
|      125 | condition flags write enable|
|      124 | export relative addressing enable |
|      123 | attribute relative addressing enable |
|      122 | saturate result             |
|      121 | condition register index    |
|      120 | ???                         |
|      119 | rC absolute value           |
|      118 | rB absolute value           |
|      117 | rA absolute value           |
| 111..116 | vector destination register |
|      110 | condition set               |
|      109 | condition check             |
|      108 | predicate - greater than 0  |
|      107 | predicate - equal to 0      |
|      106 | predicate - less than 0     |
|  98..105 | predicate swizzle           |
|   96..97 | address register select     |
|   91..95 | scalar opcode               |
|   86..90 | vector opcode               |
|   76..85 | constant fetch index        |
|   72..75 | attribute fetch index       |
|       71 | rA negate                   |
|   63..70 | rA swizzle                  |
|   57..62 | rA register                 |
|   55..56 | rA type                     |
|       54 | rB negate                   |
|   46..53 | rB swizzle                  |
|   40..45 | rB register                 |
|   38..39 | rB type                     |
|       37 | rC negate                   |
|   29..36 | rC swizzle / IADDR          |
|   23..28 | rC register                 |
|   21..22 | rC type                     |
|   17..20 | scalar op write-mask        |
|   13..16 | vector op write-mask        |
|    7..12 | scalar destination register |
|     2..6 | export write index          |
|        1 | constant relative addressing enable |
|        0 | end of program              |

### vector opcodes

|  Value | Mnemonic | Meaning                                 |
|-------:|:---------|:----------------------------------------|
|      0 | NOP      |                                         |
|      1 | MOV      | rD = rA                                 |
|      2 | MUL      | rD = rA * rB                            |
|      3 | ADD      | rD = rA + rC                            |
|      4 | MAD      | rD = rA * rB + rC                       |
|      5 | DP3      | rD = dot(rA.xyz, rB.xyz)                |
|      6 | DPH      | rD = dot(rA, vec4(rB.xyz, 1.0))         |
|      7 | DP4      | rD = dot(rA, rB)                        |
|      8 | DST      | rD = vec4(1.0, rA.y * rB.y, rB.z, rA.w) |
|      9 | MIN      | rD = min(rA, rB)                        |
|     10 | MAX      | rD = max(rA, rB)                        |
|     11 | SLT      | rD = lessThan(rA, rB)                   |
|     12 | SGE      | rD = greaterThanEqual(rA, rB)           |
|     13 | ARL      | A0 = floor(rA)                          |
|     14 | FRC      | rD = fract(rA)                          |
|     15 | FLR      | rD = floor(rA)                          |
|     16 | SEQ      | rD = equal(rA, rB)                      |
|     17 | SFL      | rD = bvec4(false, false, false, false)  |
|     18 | SGT      | rD = greaterThan(rA, rB)                |
|     19 | SLE      | rD = lessThanEqual(rA, rB)              |
|     20 | SNE      | rD = notEqual(rA, rB)                   |
|     21 | STR      | rD = bvec4(true, true, true, true)      |
|     22 | SSG      | rD = sign(rA)                           |
|     23 | ARR      | A0 = round(rA)                          |
|     24 | ARA      | A0.x = A0.z = A0.x + A0.z<br>A0.y = A0.w = A0.y + A0.w |
|     25 | TXL ?    | ???                                     |
|     26 | PUSHA    | push(A0)                                |
|     27 | POPA     | A0 = pop()                              |
| 28..31 | ???      | ???                                     |

### scalar opcodes

|  Value | Mnemonic | Meaning                                                               |
|-------:|:---------|:----------------------------------------------------------------------|
|      0 | NOP      |                                                                       |
|      1 | MOV      | rD = rC                                                               |
|      2 | RCP      | rD = 1.0 / rC                                                         |
|      3 | RCC ?    | rD = clamp(1.0 / abs(rC), pow(2.0, -64.0), pow(2.0, 64.0)) * sign(rC) |
|      4 | RSQ      | rD = 1.0 / sqrt(rC)                                                   |
|      5 | EXP      | rD = vec4(pow(2.0, floor(rC.x)), fract(rC.x), pow(2.0, rC.x), 1.0)    |
|      6 | LOG ?    | rD = vec4(floor(log2(abs(rC.x))), abs(rC.x) / pow(2.0, floor(log2(rC.x))), log2(abs(rC.x)), 1.0) |
|      7 | LIT ?    | rD = vec4(1.0, max(rD.x, 0.0), rD.x > 0.0 ? pow(max(rC.y, 0.0), clamp(rC.w, -128.0, 128.0) : 0.0, 1.0) |
|      9 | BRA      | jump(IADDR)                                                           |
|     11 | CAL      | push(IP); jump(IADDR)                                                 |
|     12 | RET      | jump( pop() )                                                         |
|     13 | LG2      | rD = log2(rC)                                                         |
|     14 | EX2      | rD = exp2(rC)                                                         |
|     15 | SIN      | rD = sin(rC)                                                          |
|     16 | COS      | rD = cos(rC)                                                          |
| 17..18 | ???      | ???                                                                   |
|     19 | PUSHA    | push(A0)                                                              |
|     20 | POPA     | A0 = pop()                                                            |

### swizzle

| Bits | Meaning            |
|-----:|:-------------------|
| 6..7 | select x-component |
| 4..5 | select y-component |
| 2..3 | select z-component |
| 0..1 | select w-component |

| Value | Meaning            |
|------:|:-------------------|
|     0 | source x-component |
|     1 | source y-component |
|     2 | source z-component |
|     3 | source w-component |

### write mask

| Bits | Meaning           |
|-----:|:------------------|
|    3 | write x-component |
|    2 | write y-component |
|    1 | write z-component |
|    0 | write w-component |

### source operand type

| Value | Meaning            |
|------:|:-------------------|
|     0 | invalid/attribute  |
|     1 | temporary          |
|     2 | attribute          |
|     3 | constant           |

## Predicates

There are 2 condition registers, bit "condition register index" selects register to use. Condition register state is stored per .xyzw component, it is set to "equal to 0" on start of vertex program execution, it is altered accordingly to the rD.xyzw of the executed instruction (per-component) to one of the following states:

1. rD.c less than 0.0
2. rD.c equal to 0.0
3. rD.c greater than 0.0

To update the content of the condition register, bits "condition set" and "condition flags write enable" must be set and resultant register component must be enabled in the op write-mask. Result of a vector operation takes precedence.

To execute instruction conditionally: "condition check" bit needs to be enabled combined with the "predicate - *" bits. The resultant register component will be updated if corresponding condition register component state, selected by predicate swizzle, satisfies the tested predicate.

As the result of a predicate vector instruction, corresponding components of the destination register are set to 1.0f (true) or 0.0f (false).

## Scalar instructions
If vector opcode isn't NOP and rD is same as scalar's, then vector result takes precedence.

### MOV instruction
Scalar's MOV acts as vector's MOV, i.e. it fetches and writes all .xyzw components.

## Execution abortion
Program execution aborts if:

  1. rD, rA, rB or rC is set to an invalid value, even if it's not used by a particular instruction. For rA, rB and rC the invalid range is 32-63, for rD it is 32-62.
  2. Export register number is invalid value 16-30.
  3. On stack overflow/underflow.

## Export

In order to use result of scalar or vector operation further in shader pipeline, it needs to be stored in the export register.

To write to the export:

1. "export write index" bitfield selected to the required export valid register number (0-15, 31).
2. To write the result of the vector instruction, bit "export vector write enable" needs be set and rD must be either a valid destination register 0-31 or a dummy 63, which is used when only write out is desired without clobbering some of the local registers.
3. To write the result of the scalar instruction, bit "export vector write enable" needs be unset.

When export register number is 31, export is disabled.

The respective components of the resultant vector of the executed instruction, enabled by the vector/scalar write-mask, will be written to the export register, so consecutively executed instructions may alter only required export register components.

To use relative addressing:

  1. Bit "export relative addressing enable" needs to be set.
  2. Bitfield "address register select" selected to the required address register.

<!-- Out-of-bounds export not checked -->

    export write index = A0.c + export write index

## Address registers
There are 4 relative base address registers (A0.xyzw). The ARL (address register load, rA floored) and ARR (address register load, rA rounded) vector operations are altering content of the address registers, so that each component of source register rA.xyzw represents the corresponding address register. The ARA (address register addition) adds 2 address register components together, so that A0 = (A0.x + A0.z, A0.y + A0.w, A0.x + A0.z, A0.y + A0.w).

Destination vector register write mask enables write to the address register component, the actual destination vector register isn't getting affected (like nv30). The address register value can be negative.

Note on ARR/ARL/ARA instructions: the destination vector register (rD) should be even value, otherwise address register isn't updated.

Note on a bit 120: when it is set, the fetched address register is overridden as A0.xyzw = (0.0f, 0.0f, 0.0f, 0.0f).

| Address register select | Component |
|:-----------------------:|:---------:|
|                       0 | A0.x      |
|                       1 | A0.y      |
|                       2 | A0.z      |
|                       3 | A0.w      |

Address register A0.xyzw can be pushed/popped to the stack with a PUSHA/POPA scalar and vector operations. Simultaneous and identical scalar-vector push/pop operation within one instruction results into only one push/pop actually being performed, while simultaneous and non-identical into the no-op.

## Constant registers
To multiplex source register rA/rB/rC to constant, its type needs to be set to "constant" and bitfield "constant fetch index" (in range of 0..1023) pointed to the required constant.

To use relative addressing:
  1. Bit "constant relative addressing enable" needs to be set.
  2. Bitfield "address register select" selected to the required address register.

Since the range of "constant fetch index" is 0..1023, the valid address register range is -1023..1023.

When constant index is out of range, the fetched constant value is assigned to vec4(0.0f, 0.0f, 0.0f, 0.0f) if fetched constant index > 1023 and to constant[1] if fetched constant index < 0.

    fetched constant index = A0.c + constant fetch index

## Attribute registers
To multiplex source register rA/rB/rC to attribute, its type needs to be set to "attribute" and bitfield "attribute fetch index" pointed to the required vertex attribute.

To use relative addressing:
  1. Bit "attribute relative addressing enable" needs to be set.
  2. Bitfield "address register select" selected to the required address register.

<!-- Out-of-bounds indexes not checked -->

    fetched attribute index = A0.c + attribute fetch index

## Branching
Scalars BRA operation is used to jump to an arbitrary instruction. Following nouveau terminology, an argument of the BRA operation is the destination instruction ID (IADDR), it is embedded into the rC swizzle bitfield.

Branching is performed only if the instruction predicate test is positive. So use of "predicate - *" bits is mandatory for the branching instruction.

## Functions
Scalar instructions CAL and RET are used for the function calling and returning. Function call is essentially a branching operation with an instruction pointer being pushed to the stack. Return from the function is a branching to the address popped from the stack. Requirement of the predicate use is the same as for branching operation for both of CAL and RET operations, CAL's IADDR is embedded to the rC swizzle as well.

## Stack
Stack is shared by the PUSHA/POPA and CAL/RET operations. Maximum depth of the stack is 8.
