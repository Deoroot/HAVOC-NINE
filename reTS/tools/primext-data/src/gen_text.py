"""The text definitions the H9 renderer reads, written for H9 (our own work).

  gfx/decals/decalinfo.txt   decal groups (gl_decals.cpp, DecalsInit)
  gfx/grass/grassinfo.txt    grass kinds (gl_grass.cpp, R_GrassInit)
  gfx/particles/effects.txt  particle effects (gl_rpart.cpp, ParsePartInfos)
  scripts/materials.def      material kinds: impact and step sounds (game_shared/material.cpp, COM_InitMatdef)
  scripts/default_ts.mat     texture -> material kind for The Specialists' own textures, made from the TS
                             install's sound/materials.txt (replaces default_hl1.mat, which listed Half-Life's)

What H9 uses of them, from the code (2026-11-24):
  - decals: textures come from gfx/decals/<name>.tga; H9 ships only the puddle textures, and the only decals
    placed are env_static_decal groups from the map (hv2_core: puddle_hv1..3) or the "pastedecal" command.
  - grass: an entry applies to surfaces whose texture name matches; H9 ships no grass textures and no map uses
    one, so the file is present (no "couldn't load" error) and lists nothing.
  - particles: no H9 code calls CreateEffect, and gfx/particles/*.dds are not shipped: present, empty.
  - material kinds: the client stores them per texture (matdesc->effects) but nothing reads the sounds or the
    impact effects yet (the precache is server-only and the H9 server is reTS's). The kinds named by H9's map
    .mat files must exist, or the loader logs "material specific ... not found": default, metal, wood, glass.

Usage: python gen_text.py <out ts folder> <TS sound/materials.txt>
"""
import os
import sys

DECALINFO = """\
// H9 decal groups. Read by the renderer at start-up (gl_decals.cpp).
//
// A group is one or more names, then a block of entries, one per line:
//     <texture>  <width>  <height>  <overlay>
// <texture> is gfx/decals/<texture>.tga (with optional _norm, _gloss and _hmap maps beside it); width and height
// are the decal's size in world units, overlay how far it may lie off the surface. A '$' before the texture
// name marks it as solid. Several entries in a group: one is picked at random each time.
//
// H9 places decals from the map only (env_static_decal, "texture" = a group name). A group whose name starts
// with "puddle" is a puddle: traced straight down, drawn with reflections (gl_realtime_puddles).

// hive v2 floor puddles
puddle_hv1
{
	puddle1		192	192	0.2
}

puddle_hv2
{
	puddle2		256	160	0.2
}

puddle_hv3
{
	puddle3		224	176	0.2
}
"""

GRASSINFO = """\
// H9 grass kinds. Read by the renderer for each map (gl_grass.cpp).
//
// One kind per line:
//     <surface texture name>  <image>  <density>  <min scale>  <max scale>  [seed]
// Faces whose texture is named <surface texture name> grow tufts of <image> (a .tga with alpha).
//
// H9 ships no grass images and no map asks for grass, so no kind is listed.
"""

EFFECTS = """\
// H9 particle effects. Read by the renderer at each map start (gl_rpart.cpp).
//
// An effect is a name followed by a { } block of particle settings. Nothing in H9 starts one of these effects
// yet and their images (gfx/particles/) are not shipped, so none is defined.
"""

# kind -> (impact sounds, step sounds); every file named here is in Half-Life's valve/sound
KINDS = [
    ("default", "concrete, plaster and anything not listed",
     ["debris/concrete%d.wav" % i for i in (1, 2, 3)], ["player/pl_step%d.wav" % i for i in (1, 2, 3, 4)]),
    ("metal", "sheet and solid metal",
     ["debris/metal%d.wav" % i for i in (1, 2, 3, 4, 5, 6)], ["player/pl_metal%d.wav" % i for i in (1, 2, 3, 4)]),
    ("grate", "metal grating and mesh",
     ["debris/metal%d.wav" % i for i in (1, 3, 5)], ["player/pl_grate%d.wav" % i for i in (1, 2, 3, 4)]),
    ("vent", "ducts",
     ["debris/metal%d.wav" % i for i in (2, 4, 6)], ["player/pl_duct%d.wav" % i for i in (1, 2, 3, 4)]),
    ("wood", "planks, crates, furniture",
     ["debris/wood%d.wav" % i for i in (1, 2, 3, 4)], ["player/pl_step%d.wav" % i for i in (1, 2, 3, 4)]),
    ("glass", "windows and glass panels",
     ["debris/glass%d.wav" % i for i in (1, 2, 3, 4)], ["player/pl_tile%d.wav" % i for i in (1, 2, 3, 4, 5)]),
    ("tile", "ceramic tiles",
     ["debris/concrete%d.wav" % i for i in (1, 2, 3)], ["player/pl_tile%d.wav" % i for i in (1, 2, 3, 4, 5)]),
    ("dirt", "earth, sand, grass ground",
     ["debris/concrete%d.wav" % i for i in (2, 3)], ["player/pl_dirt%d.wav" % i for i in (1, 2, 3, 4)]),
    ("water", "shallow liquid",
     ["debris/flesh%d.wav" % i for i in (1, 2, 3)], ["player/pl_slosh%d.wav" % i for i in (1, 2, 3, 4)]),
    ("computer", "screens, machines, electronics",
     ["debris/glass%d.wav" % i for i in (1, 3)] + ["debris/metal%d.wav" % i for i in (2, 5)],
     ["player/pl_metal%d.wav" % i for i in (1, 2, 3, 4)]),
]

# TS's materials.txt letters (its header: M metal, V ventilation, D dirt, S slosh liquid, T tile, G grate,
# W wood, P computer, Y glass; concrete is the default) -> our kinds
LETTER = {"m": "metal", "v": "vent", "d": "dirt", "s": "water", "t": "tile", "g": "grate", "w": "wood",
          "p": "computer", "y": "glass"}


def q(items):
    return " ".join('"%s"' % s for s in items)


def materials_def():
    out = ["// H9 material kinds. Read at start-up (game_shared/material.cpp). A texture names its kind with",
           '// "material" "<kind>" in a scripts/*.mat file; "default" is used for every texture that names none.',
           "// Keys: impact_sound and step_sound, lists of wave files (also accepted: impact_decal <decal group>,",
           "// impact_parts <effect names>).", ""]
    for name, what, imp, step in KINDS:
        out += ["// " + what, '"%s"' % name, "{", '\t"impact_sound"\t' + q(imp), '\t"step_sound"\t' + q(step), "}", ""]
    return "\n".join(out)


def default_ts_mat(materials_txt):
    seen, rows = set(), []
    for line in open(materials_txt, encoding="latin-1"):
        line = line.split("//")[0].strip()
        parts = line.split()
        if len(parts) < 2 or parts[0].lower() not in LETTER:
            continue
        tex = parts[1]
        if tex.lower() in seen:
            continue
        seen.add(tex.lower())
        rows.append((tex, LETTER[parts[0].lower()]))
    out = ["// The Specialists' world textures and their material kind (scripts/materials.def), made by",
           "// px-data/src/gen_text.py from the texture types in TS's sound/materials.txt. A map's own .mat file",
           "// can override any of them. Textures not listed get the kind \"default\".", ""]
    for tex, kind in rows:
        out += ['"%s"' % tex, "{", '\t"material" "%s"' % kind, "}"]
    return "\n".join(out) + "\n", len(rows)


if __name__ == "__main__":
    ts, mt = sys.argv[1], sys.argv[2]
    files = {"gfx/decals/decalinfo.txt": DECALINFO, "gfx/grass/grassinfo.txt": GRASSINFO,
             "gfx/particles/effects.txt": EFFECTS, "scripts/materials.def": materials_def() + "\n"}
    text, n = default_ts_mat(mt)
    files["scripts/default_ts.mat"] = text
    for rel, body in files.items():
        p = os.path.join(ts, rel)
        os.makedirs(os.path.dirname(p), exist_ok=True)
        open(p, "w", newline="\r\n").write(body)
        print("wrote", rel)
    print("default_ts.mat:", n, "textures")
