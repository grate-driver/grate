A very approximate MMIO-register overview:

| Range       | Use                    |
|------------:|:-----------------------|
| 0x0XX-0x0FF | synchronization        |
| 0x1XX-0x1FF | geometry submission    |
| 0x2XX-0x2FF | vertex shader          |
| 0x3XX-0x3FF | primitive processing   |
| 0x4XX-0x4FF | depth/stencil          |
| 0x6XX-0x6FF | special functions unit |
| 0x7XX-0x7FF | texture mapping        |
| 0x8XX-0x8FF | arithmetic-logic unit  |
| 0x9XX-0x9FF | depth mask             |
| 0xEXX-0xEFF | framebuffer            |
