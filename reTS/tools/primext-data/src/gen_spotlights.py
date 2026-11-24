"""gfx/spotlight1.dds .. spotlight6.dds: projected-light masks drawn for H9 (our own work).

How the renderer uses them (cl_dll/primext/client/render/gl_export.cpp, gl_scene.cpp; glsl/forward/light_*_fp.glsl):
an env_dynlight with a cone (scale > 0) picks gfx/spotlight<rendermode> (1..7; 0 or a missing file = the
procedural default), loaded with TF_SPOTLIGHT (no mipmaps used, clamp-to-border), and the shader multiplies the
light by textureProj(u_ProjectMap, ...).rgb: the colour of the mask is the colour of the light. So the border
must be black, the centre is the beam's colour and brightness.

Format kept from the files PrimeXT shipped: DXT1, 512x512 with 10 mip levels for 1..5, 128x128 with 8 for 6.
Each mask here is a drawn radial profile: a bright beam with a soft edge, a dimmer spill ring around it that
fades to black before the border, and a gentle centre lift; mask 6 is a small glint (a hot core and soft rays).
The colours are lamp families picked for H9: 1 cool blue (moonlight / LED), 2 sodium orange, 3 mercury green-
white, 4 warm incandescent, 5 neutral white, 6 cyan-white glint.

Usage: python gen_spotlights.py <out gfx folder> [--png folder]
"""
import os
import sys
import numpy as np
sys.path.insert(0, os.path.dirname(__file__))
import ddsio
import dxt1


def smoothstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0, 1)
    return t * t * (3 - 2 * t)


def beam(n, colour, core, spill, spill_level, lift=0.06):
    """core: radius where the beam edge falls (0..1 of the half size); spill: radius where the spill ends"""
    c = n / 2
    y, x = np.mgrid[:n, :n]
    r = np.hypot(x + 0.5 - c, y + 0.5 - c) / c
    main = 1 - smoothstep(core - 0.10, core + 0.06, r)
    halo = spill_level * (1 - smoothstep(core, spill, r))
    centre = lift * (1 - smoothstep(0, core, r))
    v = np.clip(main * (1 - spill_level) + halo + centre, 0, 1)
    v = v * (1 - smoothstep(0.94, 0.985, r))                 # black before the border
    return v[..., None] * np.array(colour)[None, None, :] * 255


def glint(n, colour, rays=12):
    c = n / 2
    y, x = np.mgrid[:n, :n]
    dx, dy = (x + 0.5 - c) / c, (y + 0.5 - c) / c
    r = np.hypot(dx, dy)
    a = np.arctan2(dy, dx)
    core = np.exp(-(r / 0.10) ** 2)
    ray = (0.5 + 0.5 * np.cos(rays * a)) ** 6 * np.exp(-r / 0.35)
    disc = 0.45 * (1 - smoothstep(0.55, 0.92, r))
    tint = np.array(colour)
    v = core[..., None] * 1.0 + (ray * 0.55 + disc)[..., None] * tint[None, None, :]
    v = v * (1 - smoothstep(0.94, 0.985, r))[..., None]
    return np.clip(v, 0, 1) * 255


MASKS = {
    1: lambda: beam(512, (0.26, 0.40, 0.46), core=0.62, spill=0.90, spill_level=0.45),
    2: lambda: beam(512, (0.84, 0.38, 0.24), core=0.60, spill=0.92, spill_level=0.40),
    3: lambda: beam(512, (0.74, 0.78, 0.52), core=0.58, spill=0.92, spill_level=0.42),
    4: lambda: beam(512, (0.92, 0.89, 0.64), core=0.60, spill=0.92, spill_level=0.40),
    5: lambda: beam(512, (0.99, 0.99, 0.97), core=0.62, spill=0.93, spill_level=0.40),
    6: lambda: glint(128, (0.45, 0.80, 0.82)),
}


def write(path, img):
    n = img.shape[0]
    levels = [img.astype(np.float64)]
    for (w, h) in ddsio.mip_sizes(n, n)[1:]:
        p = levels[-1]
        if p.shape[0] == 1:
            levels.append(p)
        else:
            levels.append(p.reshape(h, 2, w, 2, 3).mean((1, 3)))
    data = b"".join(dxt1.encode_image(l) for l in levels)
    hdr = ddsio.header(n, n, len(levels), ddsio.DDPF_FOURCC, fourcc=b"DXT1", linear_size=n * n // 2)
    open(path, "wb").write(hdr + data)


if __name__ == "__main__":
    out = sys.argv[1]
    png = sys.argv[sys.argv.index("--png") + 1] if "--png" in sys.argv else None
    for i, f in MASKS.items():
        img = f()
        write(os.path.join(out, f"spotlight{i}.dds"), img)
        if png:
            from PIL import Image
            Image.fromarray(np.clip(np.round(img), 0, 255).astype(np.uint8)).save(os.path.join(png, f"new_spot{i}.png"))
        print("wrote spotlight", i)
