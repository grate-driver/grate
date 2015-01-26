In order to pass data between the vertex and fragment shader, a sort of link-program seems to be copying from the output of the vertex shader into the TRAM (the RAM used as input to rasterization and shading).

This program is located at offset 0x300 and upwards, and seems to have 64-bit words. Here's an example:

    0x00000008
    0x0000fecd

This copies four floats from each vertex.

The first word seems to contain the location to copy from in the vertex shader,
the latter word contains the location in the fragment shader.

The latter word contains the following for single varyings (for a simple dot product-shader):

| datatype      | value  | VAR instruction | 0 | 1 | 2 | 3 |
|--------------:|:-------|:----------------|---|---|---|---|
|    lowp float | 00b7   | 00800000        | 0 | 0 | 0 | 4 |
| mediump float | 00b7   | 00400000        | 0 | 0 | 0 | 2 |
|     lowp vec2 | 00b7   | 00800000        | 0 | 0 | 0 | 4 |
|  mediump vec2 | 00fe   | 00408000        | 0 | 0 | 2 | 2 |
|     lowp vec3 | b7a6   | 00810000        | 0 | 0 | 4 | 4 |
|  mediump vec3 | 0dfe   | 00408100        | 0 | 2 | 2 | 2 |
|     lowp vec4 | a6b7   | 00810000        | 0 | 0 | 4 | 4 |
|  mediump vec4 | cdfe   | 00408102        | 2 | 2 | 2 | 2 |

Those 0, 1, 2, 3-values are the bits from n * 7 to n * 7 + 2 in the VAR instruction.

In addition to this, something is going on at offset 0xe20 and upwards:

    0x58000000