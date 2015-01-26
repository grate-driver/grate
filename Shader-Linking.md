In order to pass data between the vertex and fragment shader, a sort of link-program seems to be copying from the output of the vertex shader into the TRAM (the RAM used as input to rasterization and shading).

This program is located at offset 0x300 and upwards, and seems to have 64-bit words. Here's an example:

    0x00000008
    0x0000fecd

This copies four floats from each vertex.

The first word seems to contain the location to copy from in the vertex shader,
the latter word contains the location in the fragment shader.

The latter word contains the following for single varyings (for a simple dot product-shader):

| datatype      | value  |
|--------------:|:-------|
|    lowp float | 0x00b7 |
| mediump float | 0x000f |
|     lowp vec2 | 0x00b7 |
|  mediump vec2 | 0x00fe |
|     lowp vec3 | 0xb7a6 |
|  mediump vec3 | 0x0dfe |
|     lowp vec4 | 0xa6b7 |
|  mediump vec4 | 0xcdfe |

In addition to this, something is going on at offset 0xe20 and upwards:

    0x58000000