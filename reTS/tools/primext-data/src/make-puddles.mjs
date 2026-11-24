// Puddle decals for PrimeXT (client/render/gl_decals.cpp): a group whose name starts with "puddle" is a puddle.
// No alpha -> APPLY_COLORBLEND: grey 128 leaves the floor as it is, black is full puddle (wet colour + reflection).
// 24-bit TGA, 256x256, border exactly 128 (TF_CLAMP).
import fs from "node:fs";
const N = 256;
const hash = (x, y, s) => { let h = (x * 374761393 + y * 668265263 + s * 982451653) | 0; h = (h ^ (h >>> 13)) * 1274126177 | 0; return ((h ^ (h >>> 16)) >>> 0) / 4294967296; };
const smooth = (t) => t * t * (3 - 2 * t);
const vnoise = (x, y, s) => { const xi = Math.floor(x), yi = Math.floor(y), xf = smooth(x - xi), yf = smooth(y - yi);
  const a = hash(xi, yi, s), b = hash(xi + 1, yi, s), c = hash(xi, yi + 1, s), d = hash(xi + 1, yi + 1, s);
  return a + (b - a) * xf + (c - a) * yf + (a - b - c + d) * xf * yf; };
const fbm = (x, y, s) => { let v = 0, a = 0.5, f = 1; for (let o = 0; o < 4; o++) { v += a * vnoise(x * f, y * f, s + o); a *= 0.5; f *= 2; } return v; };
const tga = (file, px) => { const h = Buffer.alloc(18); h[2] = 2; h.writeUInt16LE(N, 12); h.writeUInt16LE(N, 14); h[16] = 24; h[17] = 0x20;
  const d = Buffer.alloc(N * N * 3); for (let i = 0; i < N * N; i++) d[i * 3] = d[i * 3 + 1] = d[i * 3 + 2] = px[i]; fs.writeFileSync(file, Buffer.concat([h, d])); };
for (const [name, seed, sx, thr] of [["puddle1", 11, 1.0, 0.42], ["puddle2", 23, 1.6, 0.44], ["puddle3", 37, 1.25, 0.40]]) {
  const px = new Uint8Array(N * N);
  for (let j = 0; j < N; j++) for (let i = 0; i < N; i++) {
    const u = (i + 0.5) / N * 2 - 1, v = (j + 0.5) / N * 2 - 1;
    const wu = u + 0.35 * (fbm(u * 2 + 3, v * 2 + 1, seed + 3) - 0.5), wv = v + 0.35 * (fbm(u * 2 + 7, v * 2 + 5, seed + 5) - 0.5);   // domain warp
    const fall = Math.pow(Math.hypot(wu / sx, wv), 2) * 1.5;                                     // keeps it off the border
    const field = fbm(wu * 2.6 + 11, wv * 2.6 + 13, seed) - fall;                               // ragged, lobed rim
    let wet = Math.min(1, Math.max(0, (field - (thr - 0.5) - 0.12) / 0.07));                   // soft rim
    const depth = fbm(u * 5 + 9, v * 5 + 9, seed + 7);
    wet *= 0.7 + 0.3 * Math.min(1, depth * 1.7);                                               // thinner water in places
    const edge = Math.min(i, j, N - 1 - i, N - 1 - j);
    if (edge < 12) wet *= edge / 12;
    px[j * N + i] = Math.round(128 * (1 - wet));
  }
  tga(`${name}.tga`, px);
  fs.writeFileSync(`${name}.pgm`, Buffer.concat([Buffer.from(`P5 ${N} ${N} 255
`), Buffer.from(px.map((x) => Math.min(255, x * 2)))]));
}
console.log("ok");
