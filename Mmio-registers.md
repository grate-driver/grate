A very approximate MMIO-register overview:

|          Range | Use                    |
|---------------:|:-----------------------|
| 0x000 .. 0x014 | synchronization        |
| 0x100 .. 0x125 | geometry submission    |
| 0x200 .. 0x209 | vertex shader          |
| 0x300 .. 0x364 | primitive processing   |
| 0x400 .. 0x411 | depth/stencil          |
| 0x500 .. 0x540 | ???                    |
| 0x600 .. 0x60E | special functions unit |
| 0x700 .. 0x741 | texture mapping        |
| 0x800 .. 0x83F | arithmetic-logic unit  |
| 0x900 .. 0x903 | import/export unit     |
| 0xA02 .. 0xA0B | ???                    |
| 0xE00 .. 0xE2A | framebuffer            |