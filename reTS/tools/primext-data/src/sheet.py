"""Comparison sheet: PrimeXT's data (top of each pair) against ours (below), all decoded from the .dds files.
Usage: sheet.py <primext gfx dir> <our gfx dir> <out.png>"""
import sys
import numpy as np
from PIL import Image, ImageDraw

orig, new, out = sys.argv[1:4]
T = 200


def a8(p):
    a = np.frombuffer(open(p, "rb").read(), np.uint8, 512 * 512, 128).reshape(512, 512)
    return Image.fromarray(a).convert("RGB")


def brdf(p, ch):
    a = np.asarray(Image.open(p))[..., ch]
    return Image.fromarray(a).convert("RGB")


cells = [("brdf R", lambda d: brdf(d + "/brdf_approx.dds", 0)),
         ("brdf G", lambda d: brdf(d + "/brdf_approx.dds", 1)),
         ("brdf RG", lambda d: Image.open(d + "/brdf_approx.dds").convert("RGB")),
         ("normalsfit", lambda d: a8(d + "/normalsfitting.dds"))]
cells += [(f"spot{i}", (lambda i: lambda d: Image.open(f"{d}/spotlight{i}.dds").convert("RGB"))(i)) for i in range(1, 7)]
W = len(cells) * (T + 6) + 70
sheet = Image.new("RGB", (W, 2 * (T + 22) + 10), (30, 30, 30))
dr = ImageDraw.Draw(sheet)
for row, (label, d) in enumerate((("PrimeXT", orig), ("ours", new))):
    y = 18 + row * (T + 22)
    dr.text((4, y + T // 2), label, fill=(230, 230, 230))
    for k, (name, f) in enumerate(cells):
        x = 70 + k * (T + 6)
        sheet.paste(f(d).resize((T, T), Image.BILINEAR), (x, y))
        if row == 0:
            dr.text((x, 3), name, fill=(230, 230, 230))
sheet.save(out)
print("wrote", out)
