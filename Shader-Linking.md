In order to pass data between the vertex and fragment shader, a sort of link-program seems to be copying from the output of the vertex shader into the TRAM (the RAM used as input to rasterization and shading).

This program is located at offset 0x300 and upwards, and consists of a 64-bit words. The first word contains the location in the vertex shader export to copy from, the latter word contains the location in TRAM, interpolation parameters are mixed in.

#### First word

| Bits   | Meaning                          |
|-------:|:---------------------------------|
|   0..1 | VEC4 select:<br>0 = full VEC4<br>1 = VEC4.z (VEC4.x = VEC4.z) |
|   3..6 | (in) Vertex export index         |
|  9..14 | (out) TRAM row number            |
|     16 | TRAM.x is constant across width  |
|     17 | TRAM.x is constant across length |
| 18..19 | TRAM.x across point ?            |
|     20 | TRAM.y is constant across width  |
|     21 | TRAM.y is constant across length |
| 22..23 | TRAM.y across point ?            |
|     24 | TRAM.z is constant across width  |
|     25 | TRAM.z is constant across length |
| 26..27 | TRAM.z across point ?            |
|     28 | TRAM.w is constant across width  |
|     29 | TRAM.w is constant across length |
| 30..31 | TRAM.w across point ?            |

#### Latter word

| Bits   | Meaning                                   |
|-------:|:------------------------------------------|
|   0..1 | Vertex component swizzle to TRAM.x        |
|   2..3 | TRAM.x destination type                   |
|   4..5 | Vertex component swizzle to TRAM.y        |
|   6..7 | TRAM.y destination type                   |
|   8..9 | Vertex component swizzle to TRAM.z        |
| 10..11 | TRAM.z destination type                   |
| 12..13 | Vertex component swizzle to TRAM.w        |
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

#### Vertex component swizzle:

| Value  | Meaning |
|-------:|:--------|
|   0    | VEC4.x  |
|   1    | VEC4.y  |
|   2    | VEC4.z  |
|   3    | VEC4.w  |

Disabling triangle interpolation picks the value from the third vertex in if drawing triangles. This is what we want for triangle strips and triangle fans, but not normal triangles. Which means we'll probably need to use the "constant across length"-flag in that case.

#### Offset 0xe21:

Defines the number of used TRAM rows.

|   Bits | Meaning                       |
|-------:|:------------------------------|
|  8..14 | number of used TRAM rows      |
|   0..6 | 64 / number of used TRAM rows |
