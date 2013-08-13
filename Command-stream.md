|  Bits  | Meaning          |
|-------:|:-----------------|
|     31 | 0                |
| 28..30 | opcode           |
|  0..27 | opcode dependent |

### setclass

|  Bits  | Meaning  |
|-------:|:---------|
| 28..31 | 0        |
| 16..27 | offset   |
|  6..15 | class id |
|   0..5 | mask     |

### incr/nonincr

|  Bits  | Meaning               |
|-------:|:----------------------|
| 28..31 | incr = 1, nonincr = 2 |
| 16..27 | offset                |
|  0..15 | count                 |

### mask

|  Bits  | Meaning |
|-------:|:--------|
| 28..31 | 3       |
| 16..27 | offset  |
|  0..15 | mask    |

### imm

|  Bits  | Meaning |
|-------:|:--------|
| 28..31 | 4       |
| 16..27 | offset  |
|  0..15 | value   |

### restart

|  Bits  | Meaning |
|-------:|:--------|
| 28..31 | 5       |
|  0..27 | address |

### gather

|  Bits  | Meaning |
|-------:|:--------|
| 28..31 | 6       |
| 16..27 | offset  |
|     15 | insert  |
|     14 | incr    |
|  0..13 | count   |
