A very approximate MMIO-register overview:

| Start | Stop  | Use                    |
|------:|:-----:|:-----------------------|
| 0x000 | 0x0FF | synchronization        |
| 0x100 | 0x1FF | geometry submission    |
| 0x200 | 0x2FF | vertex shader          |
| 0x300 | 0x3FF | primitive processing   |
| 0x400 | 0x4FF | depth/stencil          |
| 0x600 | 0x6FF | special functions unit |
| 0x700 | 0x7FF | texture mapping        |
| 0x800 | 0x8FF | arithmetic-logic unit  |
| 0x900 | 0x9FF | import/export unit     |
| 0xE00 | 0xEFF | framebuffer            |
