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

| Bits   | Meaning                          |
|-------:|:---------------------------------|
|   0..1 | VEC4 select:<br>0 = full VEC4<br>1 = VEC4.z (VEC4.x = VEC4.z) |
|   3..6 | VPE output row number            |
|  9..14 | TRAM row number                  |
|     16 | VEC4.x is constant across width  |
|     17 | VEC4.x is constant across length |
| 18..19 | VEC4.x across point              |
|     20 | VEC4.y is constant across width  |
|     21 | VEC4.y is constant across length |
| 22..23 | VEC4.y across point              |
|     24 | VEC4.z is constant across width  |
|     25 | VEC4.z is constant across length |
| 26..27 | VEC4.z across point              |
|     28 | VEC4.w is constant across width  |
|     29 | VEC4.w is constant across length |
| 30..31 | VEC4.w across point              |

**Latter word**

| Bits   | Meaning                               |
|-------:|:--------------------------------------|
|   0..1 | VEC4.x TRAM column number             |
|   2..3 | VEC4.x TRAM write mask (column halve) |
|   4..5 | VEC4.y TRAM column number             |
|   6..7 | VEC4.y TRAM write mask (column halve) |
|   8..9 | VEC4.z TRAM column number             |
| 10..11 | VEC4.z TRAM write mask (column halve) |
| 12..13 | VEC4.w TRAM column number             |
| 14..15 | VEC4.w TRAM write mask (column halve) |
|     16 | VEC4.x triangle interpolation disable |
|     17 | VEC4.y triangle interpolation disable |
|     18 | VEC4.z triangle interpolation disable |
|     19 | VEC4.w triangle interpolation disable |

Disabling triangle interpolation picks the value from the third vertex in if drawing triangles. This is what we want for triangle strips and triangle fans, but not normal triangles. Which means we'll probably need to use the "constant across length"-flag in that case.

In addition to this, something is going on at offset 0xe20 and upwards:

    0x58000000