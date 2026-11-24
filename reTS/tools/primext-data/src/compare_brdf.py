"""Compare the new brdf_approx.dds with PrimeXT's, both decoded by Pillow's BC6H decoder (8-bit output, an
independent decoder), and the new file decoded by our own decoder against the float table it was made from."""
import sys, os
import numpy as np
from PIL import Image
sys.path.insert(0, os.path.dirname(__file__))
import bc6h
orig, new, flt = sys.argv[1], sys.argv[2], sys.argv[3]
o = np.asarray(Image.open(orig)).astype(float) / 255
n = np.asarray(Image.open(new)).astype(float) / 255
for c, name in ((0, "R (scale)"), (1, "G (bias)")):
    d = np.abs(o[..., c] - n[..., c])
    print(f"{name}: new vs PrimeXT mean |diff| {d.mean():.5f}  p99 {np.percentile(d, 99):.5f}  max {d.max():.5f}"
          f"  (at row,col {np.unravel_index(d.argmax(), d.shape)})")
t = np.load(flt)
dec = bc6h.decode_image(open(new, "rb").read()[148:148 + 65536], 256, 256)
d = np.abs(dec - t)[..., :2]
print(f"BC6H error of the new file vs its float table: mean {d.mean():.5f}, max {d.max():.5f}")
