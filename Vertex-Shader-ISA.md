|     Bits | Meaning                    |
|---------:|:---------------------------|
| 120..128 | ???                        |
|      119 | rC absolute value          |
|      118 | rB absolute value          |
|      117 | rA absolute value          |
|  95..116 | ???                        |
|   91..94 | scalar opcode              |
|   86..90 | vector opcode              |
|   84..85 | ???                        |
|   76..83 | constant register fetch    |
|   72..75 | attribute fetch (?)        |
|       71 | rA negate                  |
|   69..70 | rA swizzle x-component     |
|   67..68 | rA swizzle y-component     |
|   65..66 | rA swizzle z-component     |
|   63..64 | rA swizzle w-component     |
|   55..62 | ???                        |
|       54 | rB negate                  |
|   52..53 | rB swizzle x-component     |
|   50..51 | rB swizzle y-component     |
|   48..49 | rB swizzle z-component     |
|   46..47 | rB swizzle w-component     |
|   38..45 | ???                        |
|       37 | rC negate                  |
|   35..36 | rC swizzle x-component     |
|   33..34 | rC swizzle y-component     |
|   31..32 | rC swizzle z-component     |
|   29..30 | rC swizzle w-component     |
|   17..28 | ???                        |
|       16 | write x-component          |
|       15 | write y-component          |
|       14 | write z-component          |
|       13 | write w-component          |
|    1..12 | ???                        |
|        0 | end of program (?)         |

