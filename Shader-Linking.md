In order to pass data between the vertex and fragment shader, a sort of link-program seems to be copying from the output of the vertex shader into the TRAM (the RAM used as input to rasterization and shading).

This program is located at offset 0x300 and upwards, and seems to have 64-bit words. Here's an example:

    0x00000008
    0x0000fecd

This copies four floats from each vertex.

The first word seems to contain the location to copy from in the vertex shader,
the latter word contains the location in the fragment shader.

The latter word contains the following for single varyings (for a simple dot product-shader):

| datatype      | value  | var instruction |
|--------------:|:-------|:----------------|
|    lowp float | 00b7   | 00400000        |
| mediump float | 000f   | 00800000        |
|     lowp vec2 | 00b7   | 00800000        |
|  mediump vec2 | 00fe   | 00408000        |
|     lowp vec3 | b7a6   | 00810000        |
|  mediump vec3 | 0dfe   | 00408100        |
|     lowp vec4 | a6b7   | 00810000        |
|  mediump vec4 | cdfe   | 00408102        |

In addition to this, something is going on at offset 0xe20 and upwards:

    0x58000000