"""Compare normals-fitting tables: texel agreement, and the angular error each gives through H9's real encoder
(CompressNormalizedVector + FloatToChar) on random unit normals. Usage: eval_normalsfitting.py orig.dds new.dds [more.dds]"""
import sys
import numpy as np


def load(p):
    return np.frombuffer(open(p, "rb").read(), np.uint8, 512 * 512, 128).reshape(512, 512)


def encode(n, table):
    a = np.abs(n)
    M = a.max(1)
    # minor components exactly as gl_backend.cpp picks them
    zmax, ymax = a[:, 2] >= M, a[:, 1] >= M
    u = np.where(~zmax, np.where(~ymax, a[:, 1], a[:, 0]), a[:, 0])
    v = np.where(~zmax, a[:, 2], a[:, 1])
    sw = u < v
    u, v = np.where(sw, v, u), np.where(sw, u, v)
    v = np.where(u != 0, v / np.where(u != 0, u, 1), v)
    x = np.minimum((u * 512).astype(np.int64), 511)
    y = np.minimum((v * 512).astype(np.int64), 511)
    s = table[y, x] / 255.0 if table is not None else np.ones(len(n))
    f = n / M[:, None] * s[:, None]
    return np.trunc(np.clip(f * 127, -128, 127))


def angerr(n, q):
    ln = np.linalg.norm(q, axis=1)
    c = (q * n).sum(1) / np.where(ln > 0, ln, 1)
    return np.degrees(np.arccos(np.clip(c, -1, 1)))


rng = np.random.default_rng(1)
n = rng.normal(size=(2_000_000, 3))
n /= np.linalg.norm(n, axis=1, keepdims=True)
tabs = [load(p) for p in sys.argv[1:]]
o = tabs[0]
for p, t in zip(sys.argv[1:], tabs):
    used = np.zeros((512, 512), bool)
    used[:, :363] = True
    d = np.abs(t.astype(int) - o.astype(int))[used & (o > 0)]
    e = angerr(n, encode(n, t))
    print(f"{p}: texels equal to first {np.mean(d == 0) * 100:.1f}%, mean|diff| {d.mean():.2f}, max {d.max()}"
          f" | encoder angular error mean {e.mean():.4f} deg, p99 {np.percentile(e, 99):.4f}, max {e.max():.4f}")
e = angerr(n, encode(n, None))
print(f"no table (scale 1): mean {e.mean():.4f} deg, p99 {np.percentile(e, 99):.4f}, max {e.max():.4f}")
