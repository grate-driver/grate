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
|     44 | enable extended operands |
|     43 | rA read constant         |
| 37..42 | rA register              |
|     36 | ???                      |
|     35 | rA read fixed10          |
|     34 | rA absolute value        |
|     33 | rA negate                |
|     32 | rA scale by two          |
|     31 | ???                      |
|     30 | rB read constant         |
| 24..29 | rB register              |
|     23 | ???                      |
|     22 | rB read fixed10          |
|     21 | rB absolute value        |
|     20 | rB negate                |
|     19 | rB scale by two          |
|     18 | ???                      |
|     17 | rC read constant         |
| 11..16 | rC register              |
|     10 | ???                      |
|      9 | rC read fixed10          |
|      8 | rC absolute value        |
|      7 | rC negate                |
|      6 | rC scale by two          |
|      5 | rC scale by rC           |
|   3..4 | ???                      |
|      2 | scale rC by rB or rC     |
|   0..1 | ???                      |

Condition code:

| Value | Meaning                  |
|------:|:-------------------------|
|     0 | zero                     |
|     1 | non-zero                 |
|     2 | zero or less             |
|     3 | less than zero           |
