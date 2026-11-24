"""A small DXT1 (BC1) encoder, opaque four-colour mode only (our own code).

From the public format description (Microsoft, "Block Compression (Direct3D 10)", BC1): two RGB565 endpoints
c0 > c1, palette c0, c1, (2 c0 + c1) / 3, (c0 + 2 c1) / 3, 2-bit index per pixel, rows top to bottom, pixel 0 in
the low bits. Endpoints start at the ends of the block's principal colour axis, are refitted by least squares
for the chosen indices, and a small search moves each endpoint channel by one step.
"""
import numpy as np

SEL = np.array([0, 2, 3, 1])         # palette order -> index code (0 = c0, 1 = c1, 2 = 2/3 c0, 3 = 1/3 c0)
WT = np.array([1.0, 2 / 3, 1 / 3, 0.0])   # weight of c0 for palette entries in order c0, 2/3, 1/3, c1


def to565(c):
    c = np.clip(np.round(c), 0, 255)
    r = np.round(c[..., 0] * 31 / 255).astype(np.int64)
    g = np.round(c[..., 1] * 63 / 255).astype(np.int64)
    b = np.round(c[..., 2] * 31 / 255).astype(np.int64)
    return (r << 11) | (g << 5) | b


def from565(v):
    r, g, b = (v >> 11) & 31, (v >> 5) & 63, v & 31
    return np.stack([(r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2)], -1).astype(np.float64)


def palette(e0, e1):
    a, b = from565(e0), from565(e1)
    return WT[None, :, None] * a[:, None, :] + (1 - WT[None, :, None]) * b[:, None, :]


def fit(px, e0, e1):
    pal = palette(e0, e1)
    d = ((px[:, :, None, :] - pal[:, None, :, :]) ** 2).sum(-1)
    idx = d.argmin(-1)
    return idx, np.take_along_axis(d, idx[..., None], -1)[..., 0].sum(-1)


def encode_image(img):
    """img (H, W, 3) 0..255 -> bytes"""
    h, w, _ = img.shape
    ph, pw = max(4, h), max(4, w)
    p = np.pad(img, ((0, ph - h), (0, pw - w), (0, 0)), mode="edge").astype(np.float64)
    px = p.reshape(ph // 4, 4, pw // 4, 4, 3).transpose(0, 2, 1, 3, 4).reshape(-1, 16, 3)
    mean = px.mean(1, keepdims=True)
    c = px - mean
    _, vec = np.linalg.eigh(np.einsum("bpi,bpj->bij", c, c))
    axis = vec[..., -1]
    t = np.einsum("bpi,bi->bp", c, axis)
    e0 = to565(mean[:, 0] + t.max(1)[:, None] * axis)
    e1 = to565(mean[:, 0] + t.min(1)[:, None] * axis)
    idx, err = fit(px, e0, e1)
    for _ in range(2):                                 # least-squares refit
        wa = WT[idx]
        wb = 1 - wa
        aa, ab, bb = (wa * wa).sum(1), (wa * wb).sum(1), (wb * wb).sum(1)
        det = np.where(np.abs(aa * bb - ab * ab) < 1e-9, 1, aa * bb - ab * ab)
        ra, rb = (wa[..., None] * px).sum(1), (wb[..., None] * px).sum(1)
        n0 = to565((bb[:, None] * ra - ab[:, None] * rb) / det[:, None])
        n1 = to565((aa[:, None] * rb - ab[:, None] * ra) / det[:, None])
        i2, e2 = fit(px, n0, n1)
        b = e2 < err
        e0, e1, idx, err = np.where(b, n0, e0), np.where(b, n1, e1), np.where(b[:, None], i2, idx), np.minimum(e2, err)
    for shift, top in ((11, 31), (5, 63), (0, 31)):    # one-step search per channel
        for which in (0, 1):
            for step in (-1, 1):
                n0, n1 = e0.copy(), e1.copy()
                tgt = n0 if which == 0 else n1
                ch = (tgt >> shift) & top
                ok = (ch + step >= 0) & (ch + step <= top)
                tgt[ok] += step << shift
                i2, e2 = fit(px, n0, n1)
                b = (e2 < err) & ok
                e0, e1, idx, err = np.where(b, n0, e0), np.where(b, n1, e1), np.where(b[:, None], i2, idx), np.where(b, e2, err)
    out = bytearray()
    for k in range(len(px)):
        a, b, ix = int(e0[k]), int(e1[k]), SEL[idx[k]]
        if a < b:                                      # four-colour mode needs c0 > c1: swap
            a, b, ix = b, a, ix ^ 1
        elif a == b:                                   # flat block: every pixel c0
            ix = np.zeros(16, np.int64)
        bits = 0
        for i, v in enumerate(ix):
            bits |= int(v) << (2 * i)
        out += a.to_bytes(2, "little") + b.to_bytes(2, "little") + bits.to_bytes(4, "little")
    return bytes(out)
