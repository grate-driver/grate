### Overview

The Tegra fragment shader ISA is not as straight forward as [[Vertex Shader ISA]]. It's kind of like a [VLIW](http://en.wikipedia.org/wiki/Very_long_instruction_word)-machine, except the different instructions aren't actually encoded together.

The fragment shader is separated into three different instruction streams.
* MFU - multi-function unit, varying interpolate and special functions
* TEX - texture lookups
* ALU - arithmetic logic unit

The ALU instructions comes in packets of 3 or 4 scalar instructions (the fourth instruction can be traded for embedded constants). Each ALU instruction package seems to run pipelined, and each instruction in a package can use partial results from the previous instruction.

The MFU unit can interpolate 4 component vectors per instruction and/or evaluate scalar special functions

The TEX instructions take the texture coordinate from the VAR unit in the same cycles.

The different units seems to be synchronized by separate timing streams.

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
|     52 | write uniform (?)        |
| 47..51 | destination register     |
|     46 | write high subregister   |
|     45 | write low subregister    |
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
|      0 |    MAD   | Multiply-Add       | rD = rA * rB + rC         |
|      1 |    MIN   | Minimum            | rD = min(rA * rB, rC)     |
|      2 |    MAX   | Maximum            | rD = max(rA * rB, rC)     |
|      3 |   CSEL   | Conditional select | rD = (rA < 0) ? rB : rC ??? |

Condition code:

| Value | Meaning                  |
|------:|:-------------------------|
|     0 | zero                     |
|     1 | non-zero                 |
|     2 | zero or less             |
|     3 | less than zero           |

Operands:

|  Bits | Meaning               |
|------:|:----------------------|
| 12..6 | register selector     |
|     5 | sub-register selector |
|     4 | ???                   |
|     3 | fixed10               |
|     2 | absolute value        |
|     1 | negate                |
|     0 | scale by two          |

### MFU instruction word encoding

Based on [this design](http://arith.polito.it/final/paper-164.pdf)?

|   Bits | Meaning  |
|-------:|:---------|
| 58..63 | register |
| 54..57 | opcode   |
| 28..53 | ???      |
| 21..27 | var3     |
| 14..20 | var2     |
|  7..13 | var1     |
|   0..6 | var0     |

| Bits | Meaning              |
|-----:|:---------------------|
| 5..6 | ???                  |
| 3..4 | destination register |
|    2 | fixed10              |
| 0..1 | ???                  |

| opcode | Mnemonic | Meaning                      | pseudo-code         |
|-------:|:--------:|:-----------------------------|:--------------------|
|      0 |    NOP   | No operation                 |                     |
|      1 |    RCP   | Reciprocal                   | rD = 1.0 / rA       | 
|      2 |    RSQ   | Reciprocal square root       | rD = 1.0 / sqrt(rA) |
|      3 |    LG2   | Logarithm base 2             | rD = log2(rA)       |
|      4 |    EX2   | Exponent base 2, second step | rD = pow(2.0, rA)   |
|      5 |   SQRT   | Square root                  | rD = sqrt(rA)       |
|      6 |    SIN   | Sine, second step            | rD = sin(rA)        |
|      7 |    COS   | Cosine, second step          | rD = cos(rA)        |
|      8 |    FRC   | Fractional value             | rD = rA - floor(rA) |
|      9 |  PREEX2  | Exponent base 2, first step  | rD = pow(2.0, rA)   |
|     10 |  PRESIN  | Sine, first step             | rD = sin(rA)        |
|     11 |  PRECOS  | Cosine, first step           | rD = cos(rA)        |

### TEX instruction word encoding

|   Bits | Meaning           |
|-------:|:------------------|
| 12..31 | ???               |
|     12 | enable bias       |
|     11 | ???               |
|     10 | enable            |
|   6..9 | ???               |
|   4..5 | texcoord from var |
|   0..3 | sampler index     |
