|  Bits  | Meaning          |
|-------:|:-----------------|
|     31 | 0                |
| 28..30 | Opcode           |
| 16..27 | Offset           |
|  0..59 | Opcode dependent |

### setclass

|  Bits  | Meaning  |
|-------:|:---------|
| 28..31 | 0        |
|  6..15 | class id |
|   0..5 | mask     |

### incr/nonincr

|  Bits  | Meaning               |
|-------:|:----------------------|
| 28..31 | incr = 1, nonincr = 2 |
|  0..15 | count                 |

### mask

|  Bits  | Meaning |
|-------:|:--------|
| 28..31 | 3       |
|  0..15 | mask    |

### imm

|  Bits  | Meaning |
|-------:|:--------|
| 28..31 | 4       |
|  0..15 | value   |
