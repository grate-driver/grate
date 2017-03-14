In order to pass data between the vertex and fragment shader, a sort of link-program seems to be copying from the output of the vertex shader into the TRAM (the RAM used as input to rasterization and shading).

This program is located at offset 0x300 and upwards, and consists of a 64-bit words. The first word contains the location in the vertex shader export to copy from, the latter word contains the location in TRAM, interpolation parameters are mixed in.

Prior to linking, the vertex export is being squeezed. For instance, if vertex program writes to the exports 2, 7 and 9 (i.e. these exports are enabled in the output attribute mask), they will be remapped as 2 -> 0, 7 -> 1, 9 -> 2; the linker will see them as 0, 1 and 2 exports.

#### First word

| Bits   | Meaning                          |
|-------:|:---------------------------------|
|      0 | Vertex export VEC4 select        |
|   3..6 | (in) Vertex export index         |
|  9..14 | (out) TRAM row index             |
|     16 | TRAM.x is constant across width ?|
|     17 | TRAM.x is constant across length?|
| 18..19 | TRAM.x across point ?            |
|     20 | TRAM.y is constant across width ?|
|     21 | TRAM.y is constant across length?|
| 22..23 | TRAM.y across point ?            |
|     24 | TRAM.z is constant across width ?|
|     25 | TRAM.z is constant across length?|
| 26..27 | TRAM.z across point ?            |
|     28 | TRAM.w is constant across width ?|
|     29 | TRAM.w is constant across length?|
| 30..31 | TRAM.w across point ?            |

#### Vertex export VEC4 select:

VEC4.xyzw:

- Vertex attribute data will be fetched from VPE export to TRAM row.

VEC4.x = VEC4.z:

- Vertex position.z coordinate will be set to the export.x component and propagated to the fragment shader (gl_FragCoord.z), export.x should be swizzled to TRAM.w (actual TRAM seems untouched).

| Value  | Meaning                  |
|-------:|:-------------------------|
|   0    | VEC4.xyzw                |
|   1    | VEC4.z (VEC4.x = VEC4.z) |

#### Latter word

| Bits   | Meaning                                   |
|-------:|:------------------------------------------|
|   0..1 | TRAM.x destination swizzle                |
|   2..3 | TRAM.x destination type                   |
|   4..5 | TRAM.y destination swizzle                |
|   6..7 | TRAM.y destination type                   |
|   8..9 | TRAM.z destination swizzle                |
| 10..11 | TRAM.z destination type                   |
| 12..13 | TRAM.w destination swizzle                |
| 14..15 | TRAM.w destination type                   |
|     16 | TRAM.x triangle interpolation disable     |
|     17 | TRAM.y triangle interpolation disable     |
|     18 | TRAM.z triangle interpolation disable     |
|     19 | TRAM.w triangle interpolation disable     |

#### TRAM destination type:

| Value  | Meaning         |
|-------:|:----------------|
|   0    | none/skip       |
|   1    | fx10 low halve  |
|   2    | fx10 high halve |
|   3    | fp20            |

#### TRAM destination swizzle:

| Value  | Meaning |
|-------:|:--------|
|   0    | TRAM.x  |
|   1    | TRAM.y  |
|   2    | TRAM.z  |
|   3    | TRAM.w  |

Disabling triangle interpolation picks the value from the third vertex in if drawing triangles. This is what we want for triangle strips and triangle fans, but not normal triangles. Which means we'll probably need to use the "constant across length"-flag in that case.

#### Offset 0xe21:

Defines the number of used TRAM rows.

|   Bits | Meaning                       |
|-------:|:------------------------------|
|  8..14 | number of used TRAM rows      |
|   0..6 | 64 / number of used TRAM rows |
