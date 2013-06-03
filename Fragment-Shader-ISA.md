### Overview

The Tegra fragment shader ISA is not as straight forward as [[Vertex Shader ISA]]. It's kind of like a VLIW-machine, except the different instructions aren't actually encoded together.

The fragment shader is separated into three different instruction streams.
* VAR/SFU - varying interpolate and special function unit
* TEX - texture lookups
* ALU - arithmetic logic unit

The ALU instructions comes in packets of 3 or 4 instructions (the fourth instruction can be traded for embedded constants). Each ALU instruction package seems to run pipelined, and each instruction in a package can use partial results from the previous instruction.

The TEX instructions take the texture coordinate from the VAR unit in the same cycles.

The different units seems to be synchronized by some sort of separate timing streams.

### ALU instruction word encoding

|   Bits | Meaning                  |
|-------:|:-------------------------|
| 62..63 | opcode                   |
|     61 | accumulate result        |
| 59..60 | ???                      |
| 57..58 | scale result             |
|     56 | saturate result          |
| 54..55 | condition code           |
|     53 | write condition register |
|     52 | ???                      |
| 46..51 | destination register     |
|     45 | ???                      |
| 32..44 | operand rA               |
| 19..31 | operand rB               |
|  6..18 | operand rC               |
|      5 | rC scale by rC           |
|   3..4 | ???                      |
|      2 | scale rC by rB or rC     |
|   0..1 | ???                      |

Opcodes:

| opcode | Mnemonic | Meaning            | pseudo-code               |
|-------:|:--------:|:-------------------|:--------------------------|
|      0 |    MAD   | Multiply-Add       | rD = rA + rB * rC         |
|      1 |    MIN   | Minimum            | rD = min(rA * rB, rC)     |
|      2 |    MAX   | Maximum            | rD = max(rA * rB, rC)     |
|      3 |   CSEL   | Conditional select | ???                       |

Condition code:

| Value | Meaning                  |
|------:|:-------------------------|
|     0 | zero                     |
|     1 | non-zero                 |
|     2 | zero or less             |
|     3 | less than zero           |

Operands:

|  Bits | Meaning        |
|------:|:---------------|
|    12 | read built-in  |
|    11 | read constant  |
| 8..10 | ???            |
|  5..7 | constant index |
|     4 | ???            |
|     3 | fixed10        |
|     2 | absolute value |
|     1 | negate         |
|     0 | scale by two   |

### VAR/SFU instruction word encoding

|   Bits | Meaning        |
|-------:|:---------------|
| 58..63 | ???            |
| 54..57 | opcode         |
| 29..53 | ???            |
| 24..28 | varying read   |
|  0..23 | ???            |
