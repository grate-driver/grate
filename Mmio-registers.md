A very approximate MMIO-register overview:

|          Range | Codename  | Use                    |
|---------------:|:---------:|:-----------------------|
| 0x000 .. 0x015 | HOST1X    | synchronization        |
| 0x100 .. 0x126 | IDX       | geometry submission    |
| 0x200 .. 0x20b | VPE       | vertex shader          |
| 0x300 .. 0x364 | SU        | program vertex to fragment shader linker engine<br>(VPE clip output to ATRAST) |
| 0x400 .. 0x411 | QRast     | (Quad Rasterizer) depth/stencil/VCAA             |
| 0x500 .. 0x546 | PSEQ      | (Program Sequencer) control pixel pipeline flow  |
| 0x600 .. 0x60E | ATRAST    | (Attribute Rasterizer) attribute interpolate     |
| 0x700 .. 0x741 | TEX       | texture mapping                                  |
| 0x800 .. 0x83F | ALU       | fragment shader arithmetic-logic unit            |
| 0x900 .. 0x903 | DW        | (Data Write) shader export unit, drive PSEQ<br>output to FDC using specified pixel color format<br>or back to PSEQ |
| 0xA00 .. 0xA0C | FDC       | configure and control (flush to DRAM)<br>unified memory cache used by QRast, PSEQ, DW |
| 0xB00 .. 0xB01 | GSHIM     | ???                    |
| 0xC00 .. 0xC01 | PIPEALIAS | ???                    |
| 0xE00 .. 0xE6F | GLOBAL    | store framebuffer base addresses used for<br>intermediate (depth buffer, stencil...) and final<br>render results, various engines configs used to<br>orchestrate their coordination and miscellaneous<br>stuff |
