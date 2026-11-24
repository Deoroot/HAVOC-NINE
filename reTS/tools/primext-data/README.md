# H9 PrimeXT renderer data: our own replacements

These files replace the 13 PrimeXT data files that had no licence and an unknown origin (PRIMEXT-LICENCES.md, the
rows marked "OPEN"). Each one was made here from first principles by the scripts in `src/`, from public
format descriptions and published methods. None was derived from PrimeXT's files: they were only decoded to
check orientation and to measure how close ours come.

**Licence: our own work, written for H9 in 2026-11.** It may go under the same terms as the rest of reTS's H9
code. TS texture names in `default_ts.mat` and Half-Life sound file names in `materials.def` are facts (names of
files), not copied expression.

`out/` mirrors the mod folder (`out/ts/...` = `game/ts/...` in reTS). In this repository the outputs are
`reTS/game/ts/...` and the shipped copies `H9XASH/ts/...`; the `game/` test notes below refer to the original
working folder and are kept as the record of that test. Requirements: Python 3 with numpy (Pillow
only for the checks and pictures).

| File | Made by | How |
|---|---|---|
| `gfx/brdf_approx.dds` | `src/gen_brdf.py` (`brdf_lut.py`, `bc6h.py`) | Split-sum environment BRDF, computed |
| `gfx/normalsfitting.dds` | `src/gen_normalsfitting.py` | Best-fit-normals scale table, brute-force search |
| `gfx/spotlight1..6.dds` | `src/gen_spotlights.py` (`dxt1.py`) | Light masks we drew (radial profiles) |
| `gfx/decals/decalinfo.txt`, `gfx/grass/grassinfo.txt`, `gfx/particles/effects.txt`, `scripts/materials.def`, `scripts/default_ts.mat` | `src/gen_text.py` | Written for H9, with only what H9 uses |

Commands (from this folder):

    python src/gen_brdf.py out/ts/gfx/brdf_approx.dds 4096
    python src/gen_normalsfitting.py out/ts/gfx/normalsfitting.dds          # defaults: --sub 4 --min 128
    python src/gen_spotlights.py out/ts/gfx --png work
    python src/gen_text.py out/ts <TS install>/ts/sound/materials.txt
    python src/eval_normalsfitting.py <primext>/normalsfitting.dds out/ts/gfx/normalsfitting.dds
    python src/compare_brdf.py <primext>/brdf_approx.dds out/ts/gfx/brdf_approx.dds work/brdf_float.npy
    python src/sheet.py <primext gfx> out/ts/gfx data-sheet.png

## gfx/brdf_approx.dds

How it is read: `gl_export.cpp` loads it (`TF_KEEP_SOURCE|TF_CLAMP`); `glsl/lightmodel.h`, `ComputeSpecularIBL`,
samples `.rg` at `(n.v, roughness)` with `roughness = (1 - smoothness)^2` and uses `F0 * r + g`.

Format, as the original: 256x256, DX10 header, `DXGI_FORMAT_BC6H_SF16` (96), 9 mips, R = A (scale on F0),
G = B (bias), B = 0. The first row of the file is roughness 1 and the last roughness 0 (the original is stored
this way: our table matches it only with this flip). Mips are 2x2 box means of the float table.

Maths (Karis 2013, "Real Shading in Unreal Engine 4", SIGGRAPH course *Physically Based Shading in Theory and
Practice*): `A = 1/N sum (1-Fc) G_vis`, `B = 1/N sum Fc G_vis`, `Fc = (1-v.h)^5`, `G_vis = G v.h / (n.h n.v)`,
Smith-Schlick `G1(x) = x / (x(1-k)+k)` with `k = alpha/2`, `alpha = roughness^2`; half-vectors drawn from GGX
(Walter et al. 2007) at the 4096 Hammersley points, at texel centres.

Encoding: our BC6H encoder (`bc6h.py`, from Microsoft's "BC6H Format" description), single-region modes 11/12/13
per block, least-squares endpoints plus a local search; checked against Pillow's independent BC6H decoder.
The encoder refuses endpoints above 10 bits (the bit order of modes 12/13's top bits is untested; values up to
~1.0 never need them).

Closeness to PrimeXT's (both decoded by Pillow, 8-bit): R mean |diff| 0.0024, p99 0.024, max 0.051; G mean
0.0005, p99 0.008, max 0.031. The largest differences sit in column 0 (n.v -> 0), where the table is steep and
both files' block compression errs. Our BC6H error against our own float table: mean 0.00056, max 0.040.

## gfx/normalsfitting.dds

How it is read: only on the CPU, `gl_backend.cpp`, `CompressNormalizedVector` (studio and grass vertex
normals, tangents): the two minor components (u >= v) of |n| index `table[int(v/u*512)][int(u*512)]`; the normal
divided by its largest component is scaled by `texel/255` and stored by `FloatToChar` (`(signed char)` of
`f*127`, truncating). Format, as the original: 512x512, 8-bit alpha-only (`DDPF_ALPHA`, mask 0xFF), 10 mips (only
mip 0 is read).

Method (Kaplanyan 2010, "CryENGINE 3: Reaching the Speed of Light", SIGGRAPH course *Advances in Real-Time
Rendering*): for each texel, try every scale 1..255 and keep the one whose quantised vector is closest in angle
to the true direction. Ours differs from the published table on purpose: it optimises for H9's real encoder
(signed, truncating bytes) rather than unsigned rounded bytes, scores each scale by its worst angle over a 4x4
grid of directions inside the texel, and does not go below 128/255. Texels that are not a direction are 0.

Closeness and quality, through H9's encoder on 2 million random unit normals:

| table | mean error | p99 | max |
|---|---|---|---|
| PrimeXT's | 0.248 deg | 0.797 | 25.4 |
| none (scale 1) | 0.254 | 0.514 | 0.635 |
| ours, published method (unsigned, centre only) | 0.226 | 0.611 | 1.02 |
| **ours, shipped** | **0.067** | 0.319 | 1.11 |

The published-method run (`--quantizer unorm --sub 1 --min 1`) agrees with PrimeXT's texel for texel on 43.5%
(mean |diff| 10.8 of 255), so the original is that kind of table; the shipped one agrees on 0.8% because it
answers H9's encoder instead, and it is 3.7x more accurate on average.

## gfx/spotlight1.dds .. spotlight6.dds

How it is read: an `env_dynlight` with a cone uses `gfx/spotlight<rendermode>` (`gl_scene.cpp`), loaded
`TF_SPOTLIGHT` (no mips used, clamp to border); the light shaders multiply the light by its `.rgb`, so the mask's
colour is the light's colour and the border must be black. Format, as the original: DXT1, 512x512 with 10 mips
(1..5), 128x128 with 8 mips (6). Encoded by our DXT1 encoder (`dxt1.py`, four-colour mode).

Drawn: a beam with a soft edge, a dimmer spill ring fading to black before the border, a slight centre lift.
Colours by lamp family, near the originals' so a map choosing an index keeps its look: 1 cool blue, 2 sodium
orange, 3 mercury green-white, 4 warm incandescent, 5 neutral white; 6 is a small cyan-white glint (hot core, 12
soft rays). Our spill ring is softer than PrimeXT's sharper two-step disc. No H9 map has an `env_dynlight` today,
so the look of these is for the owner to judge when one is placed.

## Text files

Read by `gl_decals.cpp` (decalinfo), `gl_grass.cpp` (grassinfo), `gl_rpart.cpp` (effects),
`game_shared/material.cpp` (materials.def and every `scripts/*.mat`). What H9 uses, from the code:

- **decalinfo.txt**: decal images come from `gfx/decals/<name>.tga`; H9 has only the puddles, placed by
  `env_static_decal` (hv2_core: `puddle_hv1..3`). The file lists those three groups (H9's own values) and none of
  Half-Life's (their images do not exist in H9). The puddle images `puddle1..3.tga` are ours as well: `src/make-puddles.mjs` (Node, value noise with a domain
  warp, written for the hive v2 work on 2026-11-17) writes them byte for byte; run it in `game/ts/gfx/decals`.
- **grassinfo.txt**: format comment only, no entries (no grass images shipped, no map asks for grass). Present so
  the loader logs no "couldn't load".
- **effects.txt**: comment only. Nothing in H9 calls `CreateEffect`, and `gfx/particles/*` is not shipped.
- **materials.def**: material kinds `default, metal, grate, vent, wood, glass, tile, dirt, water, computer`, each
  with impact and step sounds that exist in Half-Life's `valve/sound`. The kinds H9's map `.mat` files name
  (default, metal, wood, glass) all exist. Nothing in the H9 client reads the sounds yet (the precache is
  server-only); `impact_decal`/`impact_parts` are left out.
- **default_ts.mat** (replaces `default_hl1.mat`, which listed Half-Life's textures): 1,079 TS world textures with
  their kind, generated from the texture types in TS's own `sound/materials.txt` (letters M V D S T G W P Y ->
  metal vent dirt water tile grate wood computer glass). **`default_hl1.mat` should be deleted**, not replaced
  under its old name. A map's own `.mat` still overrides any entry.

## Game test

Own copy `C:/tmp/primext/h9run-pxd` (port 27036), map `h9_tsrp_hive_rm`, 8 cameras from
`hive-remaster/cams-rm.json`; config files backed up and restored around each launch, `cache/glsl/*.bin` cleared.
Runner `game/run.sh` (and `run_ctl.sh`); launch log `game/launches.txt`. **3 launches**: PrimeXT data, our data, and a
control with an all-zero BRDF table.

- `game/game-sheet.png`: PrimeXT data | ours | |difference| x8. Mean pixel difference 0.01 - 0.9 of 255 per
  camera; the only visible differences are dropped items and a bot (random each run) in c26/c07.
- Engine log: no new error or warning with our files (the only log differences are the random bot's name).
- Control: zeroing the BRDF table changes the shots by the same tiny amount. In this map the split-sum table has
  no visible effect: `ComputeSpecularIBL` runs only in the studio-model shader and the map has no
  `env_cubemap`. So the game run proves our files load cleanly, not that the IBL looks the same; that rests on the
  numeric comparison above. A map with cubemaps and glossy models would be the real look test.
