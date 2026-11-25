"""Animated water normal maps for the PrimeXT renderer: gfx/water/water_normal_0.tga .. _<N-1>.tga.

How they are read: gl_export.cpp loads gfx/water/water_normal_<i> for i = 0, 1, ... until one is missing
(TF_NORMALMAP); gl_world_new.cpp binds frame int(time * WATER_ANIMTIME) % count (WATER_ANIMTIME = 20 frames a
second) as u_NormalMap of every liquid surface, at the surface's own texture coordinates. An uncompressed RGB
texture is read by glsl/texfetch.h, normalmap2D: N = 2 * rgb - 1, then N.y = -N.y (green points down the
texture, the DirectX convention).

What they are: a height field made of a sum of travelling sine waves, written as its normals. Each wave has a
whole number of cycles across the tile in x and in y (so the tile repeats without a seam) and a whole number of
cycles over the loop of frames (so the last frame runs into the first). Their speeds follow deep-water
dispersion (temporal frequency ~ sqrt of the spatial one), most of them run with one wind direction and a few
across it. Our own work for H9 (2026-11), from that textbook model; no other picture was used.

    python src/gen_water_normals.py <out gfx/water folder> [--frames 32] [--size 256] [--png preview.png]
"""
import argparse
import math
import os

import numpy as np


def waves(seed):
    """(kx, ky, cycles over the loop, amplitude, phase) for each wave."""
    rng = np.random.default_rng(seed)
    out = []
    wind = math.radians(35.0)
    # (count, |k| range in cycles per tile, spread of direction around the wind in degrees, amplitude scale)
    bands = [(5, (2, 4), 35, 1.0), (7, (4, 8), 60, 0.55), (8, (8, 16), 90, 0.25), (4, (3, 7), 180, 0.35)]
    for count, (k0, k1), spread, amp in bands:
        for _ in range(count):
            for _try in range(100):
                k = rng.uniform(k0, k1)
                a = wind + math.radians(rng.uniform(-spread, spread))
                kx, ky = round(k * math.cos(a)), round(k * math.sin(a))
                if (kx, ky) != (0, 0) and all((kx, ky) != (w[0], w[1]) for w in out):
                    break
            kk = math.hypot(kx, ky)
            cycles = max(1, round(0.75 * math.sqrt(kk)))  # deep water: omega ~ sqrt(k)
            out.append((kx, ky, cycles, amp / kk, rng.uniform(0, 2 * math.pi)))
    return out


def frame(ws, size, t, slope):
    """Normals of the height field at loop time t in [0, 1): an (size, size, 3) array in -1..1."""
    u = (np.arange(size) + 0.5) / size
    x, y = np.meshgrid(u, u)  # y: rows, down the texture
    dx = np.zeros((size, size))
    dy = np.zeros((size, size))
    for kx, ky, cycles, amp, phase in ws:
        arg = 2 * math.pi * (kx * x + ky * y - cycles * t) + phase
        c = np.cos(arg) * amp * 2 * math.pi
        dx += c * kx
        dy += c * ky
    n = np.stack([-dx * slope, -dy * slope, np.ones_like(dx)], axis=-1)
    return n / np.linalg.norm(n, axis=-1, keepdims=True)


def write_tga(path, rgb):
    """24-bit uncompressed TGA, origin top-left (descriptor bit 5), rows as given."""
    h, w, _ = rgb.shape
    header = bytes([0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, w & 255, w >> 8, h & 255, h >> 8, 24, 0x20])
    with open(path, "wb") as f:
        f.write(header)
        f.write(np.ascontiguousarray(rgb[:, :, ::-1]).tobytes())  # BGR


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--frames", type=int, default=32)
    ap.add_argument("--size", type=int, default=256)
    ap.add_argument("--seed", type=int, default=9)
    ap.add_argument("--slope", type=float, default=0.0, help="height scale; 0: the one that gives --tilt")
    ap.add_argument("--tilt", type=float, default=0.11, help="mean |N.xy| wanted (PrimeXT refracts by N.xy)")
    ap.add_argument("--png", help="a preview sheet of some frames (needs Pillow)")
    args = ap.parse_args()

    ws = waves(args.seed)
    slope = args.slope
    if slope <= 0:
        n = frame(ws, args.size, 0.0, 1.0)
        mean = float(np.mean(np.hypot(n[..., 0], n[..., 1]) / n[..., 2]))  # mean |gradient| at slope 1
        slope = args.tilt / mean
    os.makedirs(args.out, exist_ok=True)
    frames = []
    for i in range(args.frames):
        n = frame(ws, args.size, i / args.frames, slope)
        n[..., 1] = -n[..., 1]  # texfetch.h flips green back
        rgb = np.clip(np.round((n * 0.5 + 0.5) * 255), 0, 255).astype(np.uint8)
        write_tga(os.path.join(args.out, f"water_normal_{i}.tga"), rgb)
        frames.append(rgb)
    tilt = np.mean([np.hypot(f[..., 0] / 127.5 - 1, f[..., 1] / 127.5 - 1) for f in frames])
    print(f"{args.frames} frames of {args.size}x{args.size}, {len(ws)} waves, slope {slope:.4f}, mean |N.xy| {tilt:.3f}")

    if args.png:
        from PIL import Image
        pick = list(range(0, args.frames, max(1, args.frames // 4)))[:4]
        sheet = Image.new("RGB", (args.size * (len(pick) + 1), args.size))
        for j, i in enumerate(pick):
            sheet.paste(Image.fromarray(frames[i]), (j * args.size, 0))
        # the tile 2x2 at half size: a seam would show as a cross
        tile = np.tile(frames[0], (2, 2, 1))
        sheet.paste(Image.fromarray(tile).resize((args.size, args.size)), (len(pick) * args.size, 0))
        sheet.save(args.png)


if __name__ == "__main__":
    main()
