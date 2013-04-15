### instruction word

|     Bits | Meaning                     |
|---------:|:----------------------------|
| 127..128 | ???                         |
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
|   96..97 | ???                         |
|       95 | scalar op enable (?)        |
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
|        6 | ???                         |
|     2..5 | varying write               |
|        1 | ???                         |
|        0 | end of program              |

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
|     3 | varying            |