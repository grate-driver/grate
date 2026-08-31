varying mediump vec4 v0;
void main()
{
   mediump vec4 a0 = v0 * 1.013 + 0.101;
   mediump vec4 a1 = a0 * 1.027 + 0.203;
   mediump vec4 a2 = a1 * 1.041 + 0.307;
   mediump vec4 a3 = a2 * 1.057 + 0.401;
   mediump vec4 a4 = a3 * 1.071 + 0.503;
   mediump vec4 a5 = a4 * 1.087 + 0.607;
   mediump vec4 a6 = a5 * 1.103 + 0.701;
   mediump vec4 a7 = a6 * 1.127 + 0.809;
   mediump vec4 a8 = a7 * 1.139 + 0.907;
   mediump vec4 a9 = a8 * 1.151 + 1.013;
   mediump vec4 a10 = a9 * 1.163 + 1.107;
   mediump vec4 a11 = a10 * 1.179 + 1.211;
   mediump vec4 a12 = a11 * 1.193 + 1.307;
   mediump vec4 a13 = a12 * 1.207 + 1.409;
   mediump vec4 a14 = a13 * 1.223 + 1.503;
   mediump vec4 a15 = a14 * 1.237 + 1.607;
   gl_FragColor =
       a0 * a15 +
       a1 * a14 +
       a2 * a13 +
       a3 * a12 +
       a4 * a11 +
       a5 * a10 +
       a6 * a9 +
       a7 * a8;
}
