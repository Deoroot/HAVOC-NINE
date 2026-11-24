"""gfx/brdf_approx.dds: the split-sum environment BRDF table (our own code; maths in brdf_lut.py).

Format, as the renderer loads it (gl_export.cpp: LOAD_TEXTURE("gfx/brdf_approx.dds", TF_KEEP_SOURCE|TF_CLAMP)):
256x256, DX10 header, DXGI_FORMAT_BC6H_SF16 (96), 9 mip levels (256 .. 1), R = scale on F0, G = bias, B = 0.
The rows run from roughness 1 at the top of the file (first row) to roughness 0 at the bottom, as in the file
PrimeXT shipped (checked: matching our table against it needs exactly this flip); n.v runs left (0) to right (1).
Mips are 2x2 box averages of the float table, then each level is BC6H-encoded by bc6h.py.

Usage: python gen_brdf.py <out.dds> [samples]
"""
import os
import sys
import numpy as np
sys.path.insert(0, os.path.dirname(__file__))
import brdf_lut
import bc6h
import ddsio

SIZE = 256


def build(samples=4096):
    t = brdf_lut.table(SIZE, samples)[::-1]                 # first row = roughness 1
    return np.concatenate([t, np.zeros(t.shape[:2] + (1,))], -1)


def write(path, img):
    levels = [img]
    for (w, h) in ddsio.mip_sizes(SIZE, SIZE)[1:]:
        p = levels[-1]
        levels.append(p.reshape(h, 2, w, 2, 3).mean((1, 3)))
    data, stats = b"", {}
    for l in levels:
        d, st = bc6h.encode_image(l)
        data += d
        for k, v in st.items():
            stats[k] = stats.get(k, 0) + v
    hdr = ddsio.header(SIZE, SIZE, len(levels), ddsio.DDPF_FOURCC, fourcc=b"DX10",
                       linear_size=SIZE * SIZE, dx10=(96, 3, 3))
    open(path, "wb").write(hdr + data)
    return stats


if __name__ == "__main__":
    out = sys.argv[1]
    n = int(sys.argv[2]) if len(sys.argv) > 2 else 4096
    img = build(n)
    np.save(out + ".npy", img)
    print("wrote", out, "blocks per mode", write(out, img))
