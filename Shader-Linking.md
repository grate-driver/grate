In order to pass data between the vertex and fragment shader, a sort of link-program seems to be copying from the output of the vertex shader into the TRAM (the RAM used as input to rasterization and shading).

This program is located at offset 0x300 and upwards, and seems to have 64-bit words. Here's an example:

    0x00000008
    0x0000fecd

This copies four floats from each vertex.

The first word seems to contain the location to copy from in the vertex shader,
the latter word contains the location in TRAM.

The latter word contains the following for single varyings (for a simple dot product-shader):

| datatype      | value  | VAR instruction | 0 | 1 | 2 | 3 |
|--------------:|:-------|:----------------|---|---|---|---|
|    lowp float | 00b7   | 00800000        | 0 | 0 | 0 | 4 |
| mediump float | 000f   | 00400000        | 0 | 0 | 0 | 2 |
|     lowp vec2 | 00b7   | 00800000        | 0 | 0 | 0 | 4 |
|  mediump vec2 | 00fe   | 00408000        | 0 | 0 | 2 | 2 |
|     lowp vec3 | b7a6   | 00810000        | 0 | 0 | 4 | 4 |
|  mediump vec3 | 0dfe   | 00408100        | 0 | 2 | 2 | 2 |
|     lowp vec4 | a6b7   | 00810000        | 0 | 0 | 4 | 4 |
|  mediump vec4 | cdfe   | 00408102        | 2 | 2 | 2 | 2 |

Those 0, 1, 2, 3-values are the bits from n * 7 to n * 7 + 2 in the VAR instruction.

splitting 0xa6b7 into 4 fields of 4-bit each:

    1010
    0110
    1011
    0111


**First word**

| Bits   | ?                        | Description           |
|--------|--------------------------|-----------------------|
|   0..1 | src:2                    | source select (0=VPE) |
|      2 | undefined_bit_2:1        |                       |
|   3..6 | vc_row:4                 | VPE row               |
|   7..8 | undefined_bits_7_8:2     |                       |
|  9..14 | tram_row:6               | TRAM row              |
|     15 | undefined_bit_15:1       |                       |
|     16 | x_const_accross_width:1  |                       |
|     17 | x_const_accross_length:1 |                       |
| 18..19 | x_point:2                |                       |
|     20 | y_const_accross_width:1  |                       |
|     21 | y_const_accross_length:1 |                       |
| 22..23 | y_point:2                |                       |
|     24 | z_const_accross_width:1  |                       |
|     25 | z_const_accross_length:1 |                       |
| 26..27 | z_point:2                |                       |
|     28 | w_const_accross_width:1  |                       |
|     29 | w_const_accross_length:1 |                       |
| 30..31 | w_point:2                |                       |

**Latter word**

| Bits   | ?                       | Description                               |
|--------|-------------------------|-------------------------------------------|
|   0..1 | x_tram_col:2            |                                           |
|   2..3 | x_tram_fmt:2            | 0=NOP; 1=Lower half; 2=Upper half; 3=Both |
|   4..5 | y_tram_col:2            |                                           |
|   6..7 | y_tram_fmt:2            | 0=NOP; 1=Lower half; 2=Upper half; 3=Both |
|   8..9 | z_tram_col:2            |                                           |
| 10..11 | z_tram_fmt:2            | 0=NOP; 1=Lower half; 2=Upper half; 3=Both |
| 12..13 | w_tram_col:2            |                                           |
| 14..15 | w_tram_fmt:2            | 0=NOP; 1=Lower half; 2=Upper half; 3=Both |
|     16 | x_tri_shade_mode:1      |                                           |
|     17 | y_tri_shade_mode:1      |                                           |
|     18 | z_tri_shade_mode:1      |                                           |
|     19 | w_tri_shade_mode:1      |                                           |
| 20..31 | undefined_bits_20_31:12 |                                           |

In addition to this, something is going on at offset 0xe20 and upwards:

    0x58000000