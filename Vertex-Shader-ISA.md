### Overview

The Tegra vertex shader ISA is a relatively straight-forward implementation of the Shader Model 2 instruction set (minus the flow control bits). The instruction set seems to be a strict subset of
the NV30 vertex-shader, with control flow related features stripped.

Each instruction contains up to two operations; one 4-component vector ALU (arithmetic logic unit) operation, and one 1-component SFU (special function unit) operation. The result of both units is a 4 component-vector, limited by a write-mask.

There's five operands, one destination register per unit (referred to as rD), and three source operands (referred to as rA, rB and rC). The ALU can use up to all three source operands, while the SFU only operates on rC.

There's no branching what-so-ever in the instruction set. Instead, predicated operations as well as normal ALU operations are used. This means that all loops must be unrolled, among other things.

### Instruction word encoding

|     Bits | Meaning                     |
|---------:|:----------------------------|
|      127 | ???                         |
|      126 | varying write enable        |
|      125 | write condition flags (?)   |
| 123..124 | ???                         |
|      122 | saturate result             |
| 120..121 | ???                         |
|      119 | rC absolute value           |
|      118 | rB absolute value           |
|      117 | rA absolute value           |
| 111..116 | vector destination register |
|      110 | ???                         |
|      109 | predicate enable            |
|      108 | ??? (predicate-related?)    |
|      107 | predicate negate            |
|      106 | ??? (predicate-related?)    |
|  98..105 | predicate swizzle           |
|   96..97 | address register select     |
|       95 | ???                         |
|   91..94 | scalar opcode               |
|   86..90 | vector opcode               |
|   84..85 | ???                         |
|   76..83 | constant register fetch     |
|   72..75 | attribute fetch             |
|       71 | rA negate                   |
|   63..70 | rA swizzle                  |
|   57..62 | rA register                 |
|   55..56 | rA type                     |
|       54 | rB negate                   |
|   46..53 | rB swizzle                  |
|   40..45 | rB register                 |
|   38..39 | rB type                     |
|       37 | rC negate                   |
|   29..36 | rC swizzle                  |
|   23..28 | rC register                 |
|   21..22 | rC type                     |
|   17..20 | scalar op write-mask        |
|   13..16 | vector op write-mask        |
|    7..12 | scalar destination register |
|        6 | ??? (related to ARL)        |
|     2..5 | varying write               |
|        1 | constant fetch offset       |
|        0 | end of program              |

### vector opcodes

|  Value | Mnemonic | Meaning                                 |
|-------:|:---------|:----------------------------------------|
|      0 | NOP ?    |                                         |
|      1 | MOV      | rD = rA                                 |
|      2 | MUL      | rD = rA * rB                            |
|      3 | ADD      | rD = rA + rC                            |
|      4 | MAD      | rD = rA * rB + rC                       |
|      5 | DP3      | rD = dot(rA.xyz, rB.xyz)                |
|      6 | DPH ?    | rD = dot(vec4(rA.xyz, 1.0), rB)         |
|      7 | DP4      | rD = dot(rA, rB)                        |
|      8 | DST ?    | rD = vec4(1.0, rA.y * rB.y, rA.z, rB.w) |
|      9 | MIN      | rD = min(rA, rB)                        |
|     10 | MAX      | rD = max(rA, rB)                        |
|     11 | SLT      | rD = lessThan(rA, rB)                   |
|     12 | SGE      | rD = greaterThanEqual(rA, rB)           |
|     13 | ARL      | rD = floor(rA)                          |
|     14 | FRC      | rD = fract(rA)                          |
|     15 | FLR      | rD = floor(rA)                          |
|     16 | SEQ      | rD = equal(rA, rB)                      |
|     17 | SFL ?    | ???                                     |
|     18 | SGT      | rD = greaterThan(rA, rB)                |
|     19 | SLE      | rD = lessThanEqual(rA, rB)              |
|     20 | SNE      | rD = notEqual(rA, rB)                   |
|     21 | STR ?    | ???                                     |
|     22 | SSG ?    | ???                                     |
|     23 | ARR ?    | ???                                     |
|     24 | ARA ?    | ???                                     |
| 25..31 | ???      | ???                                     |

### scalar opcodes

|  Value | Mnemonic | Meaning             |
|-------:|:---------|:--------------------|
|      0 | COS      | rD = cos(rC)        |
|      1 | MOV      | rD = rC             |
|      2 | RCP      | rD = 1.0 / rC       |
|      4 | RSQ      | rD = 1.0 / sqrt(rC) |
|     13 | LG2      | rD = log2(rC)       |
|     14 | EX2      | rD = exp2(rC)       |
|     15 | SIN      | rD = sin(rC)        |

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
|     0 | invalid/unknown    |
|     1 | temporary          |
|     2 | attribute          |
|     3 | constant           |