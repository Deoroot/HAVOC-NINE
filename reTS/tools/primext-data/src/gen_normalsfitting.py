"""gfx/normalsfitting.dds: the best-fit-normals scale table, computed from first principles (our own code).

Method (published): A. Kaplanyan, "CryENGINE 3: Reaching the Speed of Light", SIGGRAPH 2010 course
"Advances in Real-Time Rendering in 3D Graphics and Games", section "Best fit for normals". A unit normal is
stored in 8 bits per component. Instead of storing n itself, store s * n / max|n_i|: the direction of the
quantized vector is what matters, and among the 255 lengths along the ray some quantize far closer to the
true direction than the plain normalised one. The table holds, per direction, the best length s.

How H9's renderer reads it (cl_dll/primext/client/render/gl_backend.cpp, CompressNormalizedVector):
  a = |n|, M = max(a); (u, v) = the two components other than the major one, swapped so u >= v; v /= u;
  texel = table[int(v * 512)][int(u * 512)]  (8-bit, alpha-only DDS, 512x512, mip 0 only, CPU side);
  stored = FloatToChar(n / M * texel / 255), FloatToChar(f) = (signed char) clamp(f * 127, -128, 127)
  (truncation toward zero, game_shared/mathlib.cpp). The GPU reads it back as a normalised signed byte.

So for each texel we rebuild the direction at its centre, (M, u, u*v) with M = sqrt(1 - u^2 - u^2 v^2),
and search all 255 scales for the one whose truncated signed-byte vector has the smallest angle to it
(ties: the largest scale, which keeps the most precision). Texels with u^2 (1 + v^2) > 1 are not a
direction and are written 0; texels with u > M are never read (u is a minor component) but are filled the
same way for a smooth table.

Each texel covers a small patch of directions, and the renderer uses its one scale for all of them; with the
truncating signed-byte encoder a scale that is ideal at the centre can be poor at the edge. So by default the
search scores a scale by its worst angle over a 4x4 grid of directions inside the texel (--sub 1: centre only,
as in the published method). Scales below 128/255 are not tried by default (--min 128): with fewer than 64
steps per axis a truncated vector can land far off for directions between the grid samples (measured: worst
angle 3.0 deg with all scales, 1.1 deg with this floor, the mean unchanged).

Usage: python gen_normalsfitting.py <out.dds> [--quantizer h9|unorm] [--sub N]
  h9    (default) optimise for H9's real encoder above.
  unorm optimise for the classic 8-bit unsigned encoding round((x * 0.5 + 0.5) * 255), used only to check
        this generator against the published method.
"""
import sys
import numpy as np
sys.path.insert(0, __import__("os").path.dirname(__file__))
import ddsio

W = H = 512


def directions(fu=0.5, fv=0.5):
    u = (np.arange(W) + fu) / W
    v = (np.arange(H) + fv) / H
    uu, vv = np.meshgrid(u, v)          # [row = v][col = u]
    a, b = uu, uu * vv
    m2 = 1.0 - a * a - b * b
    valid = m2 > 0
    m = np.sqrt(np.clip(m2, 0, None))
    n = np.stack([m, a, b], -1)
    return n, valid


def quantize(f, quantizer):
    if quantizer == "h9":
        return np.trunc(np.clip(f * 127.0, -128, 127))
    return np.round((f * 0.5 + 0.5) * 255.0) / 255.0 * 2.0 - 1.0


def best_scales(samples, quantizer, smin=1):
    """samples: list of (H, W, 3) direction sets inside each texel. The error of a scale for a texel is the
    worst angle over its samples (the renderer uses one scale for every normal falling in the texel)."""
    flats = [n.reshape(-1, 3) for n in samples]
    ds = [f / f.max(1, keepdims=True).clip(1e-12) for f in flats]      # the vector the renderer scales
    units = [f / np.linalg.norm(f, axis=1, keepdims=True).clip(1e-12) for f in flats]
    best = np.zeros(len(flats[0]), np.int32)
    besterr = np.full(len(flats[0]), np.inf)
    for s in range(smin, 256):
        err = np.zeros(len(flats[0]))
        for d, unit in zip(ds, units):
            q = quantize(d * (s / 255.0), quantizer)
            ln = np.linalg.norm(q, axis=1)
            cosang = np.where(ln > 0, (q * unit).sum(1) / np.where(ln > 0, ln, 1), -1.0)
            err = np.maximum(err, 1.0 - cosang)
        better = err <= besterr + 1e-15          # <= : ties go to the larger scale
        best[better] = s
        besterr[better] = np.minimum(err, besterr)[better]
    return best.reshape(H, W)


def build(quantizer="h9", sub=1, smin=1):
    """sub = 1: the texel centre only (the published method). sub = k: a k x k grid of directions spread over
    the texel, best worst-case."""
    n, valid = directions()
    if sub == 1:
        samples = [n]
    else:
        offs = (np.arange(sub) + 0.5) / sub
        samples = []
        for fu in offs:
            for fv in offs:
                ns, ok = directions(fu, fv)
                samples.append(np.where(ok[..., None], ns, n))   # outside the sphere: keep the centre
    table = best_scales(samples, quantizer, smin).astype(np.uint8)
    table[~valid] = 0
    return table


def write(path, table):
    levels = [table.astype(np.float64)]
    for (w, h) in ddsio.mip_sizes(W, H)[1:]:
        p = levels[-1]
        levels.append(p.reshape(h, p.shape[0] // h, w, p.shape[1] // w).mean((1, 3)))
    data = b"".join(np.clip(np.round(l), 0, 255).astype(np.uint8).tobytes() for l in levels)
    hdr = ddsio.header(W, H, len(levels), ddsio.DDPF_ALPHA, bitcount=8, amask=0xFF,
                       extra_caps=ddsio.DDSCAPS_ALPHA)
    open(path, "wb").write(hdr + data)


if __name__ == "__main__":
    out = sys.argv[1]
    qz = sys.argv[sys.argv.index("--quantizer") + 1] if "--quantizer" in sys.argv else "h9"
    sub = int(sys.argv[sys.argv.index("--sub") + 1]) if "--sub" in sys.argv else 4
    smin = int(sys.argv[sys.argv.index("--min") + 1]) if "--min" in sys.argv else 128
    t = build(qz, sub, smin)
    write(out, t)
    np.save(out + ".npy", t)
    print("wrote", out, "quantizer", qz)
