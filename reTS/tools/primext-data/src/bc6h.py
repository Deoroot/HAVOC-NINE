"""A small BC6H (signed, BC6H_SF16) block encoder and decoder for one-region modes (our own code).

Written from the format description in Microsoft's Direct3D 11 documentation, "BC6H Format" (modes 11-13,
endpoint unquantization, interpolation weights, signed finish). Only the single-region modes are used:

  mode 11 (bits 00011): two 10-bit endpoints per channel, no delta.
  mode 12 (bits 00111): 11-bit base endpoint, 9-bit signed delta.
  mode 13 (bits 01011): 12-bit base endpoint, 8-bit signed delta (the two top base bits stored reversed).

All three use 4-bit indices (16 weights) with pixel 0 as the anchor (its index has an implied top bit 0).
Each block is tried in each mode with a least-squares endpoint fit and a small local search, and the mode with
the smallest error in float space is kept. Inputs here are non-negative, so only positive halves occur.
"""
import numpy as np

WEIGHTS = np.array([0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64], np.int64)
MODES = {11: (0b00011, 10, None), 12: (0b00111, 11, 9), 13: (0b01011, 12, 8)}


def f2h(x):
    return np.asarray(x, np.float32).astype(np.float16).view(np.uint16).astype(np.int64)


def h2f(h):
    return np.asarray(h, np.int64).astype(np.uint16).view(np.float16).astype(np.float64)


def unquant(e, prec):
    """signed unquantization for non-negative endpoints"""
    e = np.asarray(e, np.int64)
    top = (1 << (prec - 1)) - 1
    u = ((e << 15) + 0x4000) >> (prec - 1)
    return np.where(e == 0, 0, np.where(e >= top, 0x7FFF, u))


def finish(interp):
    return (interp * 31) >> 5           # non-negative values only


def palette(e0, e1, prec):
    """e0, e1: (..., 3) endpoints -> (..., 16, 3) half bits"""
    u0, u1 = unquant(e0, prec), unquant(e1, prec)
    w = WEIGHTS[:, None]
    return finish((u0[..., None, :] * (64 - w) + u1[..., None, :] * w + 32) >> 6)


def quant(U, prec):
    """inverse of unquant (nearest), U: unquantized 16-bit target"""
    top = (1 << (prec - 1)) - 1
    e = np.round((np.asarray(U, np.float64) * (1 << (prec - 1)) - 0x4000) / 32768.0).astype(np.int64)
    return np.clip(e, 0, top)


def best_indices(pal, px):
    """pal (B,16,3) half bits, px (B,16,3) float targets -> idx (B,16), err (B,)"""
    pf = h2f(pal)
    d = ((px[:, :, None, :] - pf[:, None, :, :]) ** 2).sum(-1)        # (B, 16 px, 16 pal)
    idx = d.argmin(-1)
    return idx, np.take_along_axis(d, idx[..., None], -1)[..., 0].sum(-1)


def valid(e0, e1, prec, dbits):
    top = (1 << (prec - 1)) - 1
    ok = (e0 >= 0).all(-1) & (e1 >= 0).all(-1) & (e0 <= top).all(-1) & (e1 <= top).all(-1)
    if dbits:
        lim = (1 << (dbits - 1)) - 1
        ok &= (np.abs(e1 - e0) <= lim).all(-1)
    return ok


def encode_mode(px, mode):
    """px (B,16,3) float >= 0. Returns e0, e1, idx, err for this mode."""
    _, prec, dbits = MODES[mode]
    target = f2h(px).astype(np.float64) * 32.0 / 31.0                 # in interpolation space
    # initial line: per-channel endpoints along the principal axis of the block
    mean = target.mean(1, keepdims=True)
    c = target - mean
    cov = np.einsum("bpi,bpj->bij", c, c)
    _, vec = np.linalg.eigh(cov)
    axis = vec[..., -1]
    t = np.einsum("bpi,bi->bp", c, axis)
    lo = mean[:, 0] + t.min(1)[:, None] * axis
    hi = mean[:, 0] + t.max(1)[:, None] * axis
    e0, e1 = quant(np.clip(lo, 0, None), prec), quant(np.clip(hi, 0, None), prec)
    if dbits:   # pull the second endpoint into the delta range
        lim = (1 << (dbits - 1)) - 1
        e1 = np.clip(e1, e0 - lim, e0 + lim)
    best = (e0, e1) + best_indices(palette(e0, e1, prec), px)
    # least squares refit of the endpoints for the chosen indices, twice
    for _ in range(2):
        w = WEIGHTS[best[2]] / 64.0                                     # (B,16)
        a, b = 1 - w, w
        aa, ab, bb = (a * a).sum(1), (a * b).sum(1), (b * b).sum(1)
        det = aa * bb - ab * ab
        det = np.where(np.abs(det) < 1e-9, 1, det)
        ra = (a[..., None] * target).sum(1)
        rb = (b[..., None] * target).sum(1)
        u0 = (bb[:, None] * ra - ab[:, None] * rb) / det[:, None]
        u1 = (aa[:, None] * rb - ab[:, None] * ra) / det[:, None]
        n0, n1 = quant(np.clip(u0, 0, None), prec), quant(np.clip(u1, 0, None), prec)
        if dbits:
            n1 = np.clip(n1, n0 - lim, n0 + lim)
        cand = (n0, n1) + best_indices(palette(n0, n1, prec), px)
        better = (cand[3] < best[3]) & valid(n0, n1, prec, dbits)
        best = tuple(np.where(better[:, None] if x.ndim == 2 else (better[:, None, None] if x.ndim == 3 else better), y, x)
                     for x, y in zip(best, cand))
    # local search: move one endpoint channel by -2..2 at a time, a few passes
    for _ in range(3):
        for which in (0, 1):
            for ch in range(3):
                for step in (-2, -1, 1, 2):
                    n0, n1 = best[0].copy(), best[1].copy()
                    (n0 if which == 0 else n1)[:, ch] += step
                    ok = valid(n0, n1, prec, dbits)
                    n0, n1 = np.where(ok[:, None], n0, best[0]), np.where(ok[:, None], n1, best[1])
                    idx, err = best_indices(palette(n0, n1, prec), px)
                    better = (err < best[3]) & ok
                    best = (np.where(better[:, None], n0, best[0]), np.where(better[:, None], n1, best[1]),
                            np.where(better[:, None], idx, best[2]), np.where(better, err, best[3]))
    return best


class Bits:
    def __init__(self):
        self.v, self.n = 0, 0

    def put(self, value, count):
        self.v |= (int(value) & ((1 << count) - 1)) << self.n
        self.n += count

    def put_rev(self, value, count):     # most significant of the field first
        for i in reversed(range(count)):
            self.put((int(value) >> i) & 1, 1)


def pack(mode, e0, e1, idx):
    mbits, prec, dbits = MODES[mode]
    e0, e1, idx = [int(x) for x in e0], [int(x) for x in e1], [int(x) for x in idx]
    if idx[0] >= 8:                                     # anchor needs top bit 0: swap the endpoints
        e0, e1, idx = e1, e0, [15 - i for i in idx]
    if dbits is not None:
        # the top base bits (rw[10], rw[10:11]) are only ever zero for values up to ~1.0, which is all this
        # kit writes; refuse anything else rather than rely on an untested bit order.
        assert max(e0) < 1024, "endpoint above 10 bits: not supported here"
    b = Bits()
    b.put(mbits, 5)
    for c in range(3):
        b.put(e0[c], 10)
    for c in range(3):
        if dbits is None:
            b.put(e1[c], 10)
        else:
            b.put(e1[c] - e0[c], dbits)
            if mode == 12:
                b.put(e0[c] >> 10, 1)
            else:
                b.put_rev(e0[c] >> 10, 2)
    assert b.n == 65
    b.put(idx[0], 3)
    for i in idx[1:]:
        b.put(i, 4)
    assert b.n == 128
    return b.v.to_bytes(16, "little")


def encode_image(img, modes=(11, 12, 13)):
    """img (H, W, 3) float >= 0, H and W multiples of 4 (or < 4: padded by edge copy) -> bytes, stats"""
    h, w, _ = img.shape
    ph, pw = max(4, (h + 3) // 4 * 4), max(4, (w + 3) // 4 * 4)
    p = np.pad(img, ((0, ph - h), (0, pw - w), (0, 0)), mode="edge")
    blocks = p.reshape(ph // 4, 4, pw // 4, 4, 3).transpose(0, 2, 1, 3, 4).reshape(-1, 16, 3)
    blocks = h2f(f2h(blocks))                          # what a half can hold
    res = {m: encode_mode(blocks, m) for m in modes}
    errs = np.stack([res[m][3] for m in modes])
    pick = np.array(modes)[errs.argmin(0)]
    out = bytearray()
    for i, m in enumerate(pick):
        e0, e1, idx, _ = res[m]
        out += pack(int(m), e0[i], e1[i], idx[i])
    return bytes(out), {int(m): int((pick == m).sum()) for m in modes}


# ---- decoder for the same modes (used to measure what was written) ----

def unpack(block):
    v = int.from_bytes(block, "little")
    pos = [0]

    def get(n):
        r = (v >> pos[0]) & ((1 << n) - 1)
        pos[0] += n
        return r

    def get_rev(n):
        r = 0
        for _ in range(n):
            r = (r << 1) | get(1)
        return r

    m = get(5)
    mode = {0b00011: 11, 0b00111: 12, 0b01011: 13}[m]
    _, prec, dbits = MODES[mode]
    e0 = [get(10) for _ in range(3)]
    e1 = []
    for c in range(3):
        if dbits is None:
            e1.append(get(10))
        else:
            d = get(dbits)
            hi = get(1) if mode == 12 else get_rev(2)
            e0[c] |= hi << 10
            e1.append(d)
    if dbits is not None:
        for c in range(3):
            d = e1[c] - (1 << dbits) if e1[c] & (1 << (dbits - 1)) else e1[c]
            e1[c] = (e0[c] + d) & ((1 << prec) - 1)
    idx = [get(3)] + [get(4) for _ in range(15)]
    pal = palette(np.array(e0), np.array(e1), prec)
    return h2f(pal[idx])


def decode_image(data, w, h):
    bw, bh = max(1, (w + 3) // 4), max(1, (h + 3) // 4)
    out = np.zeros((bh * 4, bw * 4, 3))
    for by in range(bh):
        for bx in range(bw):
            o = (by * bw + bx) * 16
            out[by * 4:by * 4 + 4, bx * 4:bx * 4 + 4] = unpack(data[o:o + 16]).reshape(4, 4, 3)
    return out[:h, :w]
