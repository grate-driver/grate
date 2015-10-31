### Geometry Submission (IDX)

|         Offset | Description                                                  |
|---------------:|:-------------------------------------------------------------|
| 0x100 .. 0x1FF | Attribute buffers memory base address and format description |
| 0x120          | Input/output buffers bit masks                               |
| 0x121          | Index buffer memory base address                             |
| 0x122          | Drawing config                                               |
| 0x123          | Drawing elements number                                      |

#### Offset range 0x100 .. 0x1FF:

Two consecutive words are used to describe the input attribute buffer: first is memory address and latter is buffer format description. AR20 has total 16 input attribute buffers. 

**First word**

Attribute buffer memory base address.

|    Bits | Meaning             |
|--------:|:--------------------|
| 0 .. 31 | Memory base address |

**Latter word**

Attribute buffer format description.

|    Bits | Meaning          |
|--------:|:-----------------|
| 0 .. 3  | Attribute format |
| 4 .. 6  | Attribute size   |
| 8 .. 19 | Attribute stride |

#### Offset 0x120:

In / out buffers bit masks selects attribute input buffers that would be loaded from the DRAM into VPE and what output buffers would be allowed to be written by VS (to spare some clock cycles presumably).

|     Bits | Meaning                              |
|---------:|:-------------------------------------|
|  0 .. 15 | Output buffers write-enable bit mask |
| 16 .. 31 | Input buffers read-enable bit mask   |

#### Offset 0x121:

Memory base address of the index buffer to be fetched into VPE.

|    Bits | Meaning              |
|--------:|:---------------------|
| 0 .. 31 | Memory base address  |

#### Offset 0x122:

Drawing config, specifies how VPE should interpret input vertices's: triangles type / lines / points, use index buffer or not.

|     Bits | Meaning           |
|---------:|:------------------|
| 24 .. 26 | Primitive type    |
|       27 | Flat-shade last   |
| 28 .. 29 | Index buffer type |

#### Offset 0x123:

Specifies drawing elements number.

|     Bits | Meaning    |
|---------:|:-----------|
| 20 .. 31 | Number + 1 |
