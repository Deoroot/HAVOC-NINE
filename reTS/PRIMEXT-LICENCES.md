# Licences of the PrimeXT files that had none

H9 bookkeeping for the local branch `primext-client`. [PRIMEXT-SOURCES.md](PRIMEXT-SOURCES.md) lists every file taken
from PrimeXT; 36 of them had no licence header. This file gives the verdict for each of the 36, with the evidence.

PrimeXT clone: `C:/tmp/primext/ext/PrimeXT`, commit 64fe05d5 (https://github.com/SNMetamorph/PrimeXT). The repository
has no LICENSE file (GitHub reports none). Its history starts with XashXT imports: 6521289 (2015-12-25, "XashXT v0.65
rev2 upload"), then 0.81 (654c99f, 7478a28, 9d0ee96), then 5bae6bc (2021-08-07, SNMetamorph, "render: wrenched
renderer from Paranoia 2"), which brought the `gl_*` renderer in.

## The sources the verdicts rest on

- **Paranoia 1.2 source code** (BUzer and Wargon, 2008): its readme says "PARANOIA 1.2 source code. Provided under GNU
  General Public License. Read license.txt. Copyright (c) BUzer, Wargon. http://paranoia.level-design.ru 09.01.2008",
  and the `license.txt` beside it is the GPL version 2 text. With no version named, GPL-2.0 section 9 lets the
  recipient pick any version, so it combines with GPL-3.0+. Read from the unofficial mirror
  https://github.com/HLSources/Paranoia (`code_src/readme.txt`, `code_src/license.txt`). The official release is the
  "Paranoia Toolkit" on ModDB (https://www.moddb.com/mods/paranoia/downloads/paranoia-toolkit, added 2008-01-10);
  that page could not be fetched from here (403), so the mirror's text has not been checked against the official archive.
- **XashXT 0.65** (in PrimeXT 6521289): 71 of its 615 files carry Uncle Mike's GPL-3.0-or-later header, among them
  `client/render/r_particle.cpp` (2011), `r_debug.cpp` (2012), `r_warp.cpp` (2011). At 5bae6bc^ also `r_postprocess.cpp`,
  `r_brushdecals.cpp`, `r_studiodecal.cpp` (2018), all GPL-3.0+. None of the files headed "XashXT Group" or "Shambler
  Team" carries any licence text (checked: 11 files in 6521289), and the import has no LICENSE or readme of its own.
- **Paranoia 2 sources**: https://github.com/a1batross/Paranoia2_ancient ("Published by Unkle Mike in 2019"): no LICENSE
  file (GitHub reports none).
- **Xash3D FWGS** (`C:/tmp/primext/ext/xash3d-fwgs`, https://github.com/FWGS/xash3d-fwgs): Uncle Mike's engine code
  under GPL-3.0+ headers, for example `public/crtlib.c` ("Copyright (C) 2011 Uncle Mike", GPL-3.0+).

"Lines in common" below counts the file's non-trivial lines (more than 12 characters once whitespace is removed) that
also appear in the named file; it measures descent, not exact copying.

## The search for a licence of XashXT's shared library (2026-11-15)

The owner asked for this before any rewrite of `mathlib`, `matrix`, `vector` and `stringlib` (`virtualfs` was already
replaced). Nothing found states a licence for these files. Where each place was looked at:

| Where | What it says |
|---|---|
| XashXT original source, https://github.com/a1batross/XashXT_original (uploaded 2015-12-25, "Original Source Code of XashXT Toolkit") | No LICENSE (GitHub reports none). `xash/devkit/source_code/game_shared/` has the same files with the same headers and no licence. The help file `xash/devkit/docs/xashxt-en.chm` (unpacked, 128 pages) lists "The complete source code for server.dll, client.dll and xash.exe" among the contents (setup.htm) and says nothing of a licence; `docs/change.log` neither. Other files of the same tree carry GPL-3.0+, e.g. `server/novodex.cpp` ("Copyright (C) 2012 Uncle Mike"). |
| XashXT on ModDB (v0.6 to v0.81 rev.1, e.g. https://www.moddb.com/engines/xash3d-engine/downloads/xashxt-v081-revision-1-xash3d-mod-toolkit) | The pages refuse this machine (403). Search results show the download pages; the GPL label they show belongs to the Xash3D Engine entry, not to XashXT. |
| XashXT wiki, http://xash3d.ru/doku.php?id=xashxt:en:start | No licence statement. |
| FWGS's XashXT port, https://github.com/FWGS/XashXT | No LICENSE. Same headers on mathlib, matrix, stringlib, vector.h. Its `game_shared/vector.cpp` (not in this module) is GPL-3.0+ ("Copyright (C) 2012 Uncle Mike"). |
| Xash3D engine: FWGS xash3d-fwgs (local clone, 8d6e3b7f "Pure engine source code", 2018-04-13) and https://github.com/a1batross/Xash3D_original | The engine's own `crtlib.c/.h`, `mathlib.c/.h` and `matrixlib.c` are GPL-3.0+ by Uncle Mike: the engine's counterparts, partly the same code (see the rows below), but other files. The launcher's `game_launch/game.rc` names the company "XashXT Group". |
| FWGS hlsdk-portable (its branches) | None carries XashXT's game_shared. |
| Paranoia 2 sources, https://github.com/a1batross/Paranoia2_ancient ("Published by Unkle Mike in 2019") | No LICENSE. `game_shared/stringlib.cpp` and `matrix.cpp` have the same "XashXT Group 2011" headers. The renderer's `gl_rmain.cpp` has the GPL-3.0+ notice; `gl_aurora.cpp` and `gl_sky.cpp` do not. |
| Paranoia 2 SDK (`Paranoia_2_SDK.zip` from the owner's Downloads; extracted to `C:/tmp/primext/ext/p2sdk`, nothing run) | 54 files: docs (`entities.txt`, `grass.txt`, `materials.txt`, `texinfo.txt`, two Xash weapon and ammo texts), example maps, resources, compiled map and model tools, an FGD. No source code, no licence or readme file, and no licence, GPL or copyright text in any text file. Nothing about Paranoia 1. |
| PrimeXT issues and pull requests | A search for licence, license, GPL, copyright finds nothing about licensing; discussions are off. |
| GitHub-wide issue search ("XashXT license", "Uncle Mike license" and the like) | Nothing about XashXT's licence. |
| Diffusion, https://github.com/Aynekko/Diffusion | No LICENSE (GitHub reports none) and its README states none. It ships `game_shared/mathlib`, `matrix`, `stringlib`, `vector.h` and `virtualfs.h` with the same headers. A precedent for shipping them, not a licence. |

The owner's decision is to keep the files as they are. What can be said for that: their author published them as
source with his toolkits and mods for others to build on, and the same author put the matching engine code (75% of
stringlib.cpp) under GPL-3.0+. What cannot be said: that any text grants a licence for these files. A PR has to say so.

## Verdicts

(a) covered by a licence already. (b) can be dropped. (c) must be rewritten. **Done** means replaced on this branch.

| File (under `cl_dll/primext/` unless `game/`) | Verdict | Evidence |
|---|---|---|
| `common/exportdef.h` | (c) **done** (19 lines, commit 6288fa71) | Added by mittorn in PrimeXT 93e9fcc (2020-02-18, "port: client build"); no header. The new file defines only `DLLEXPORT`, the one macro the module uses. pxrender.dll still exports its 7 names (dumpbin). |
| `client/render/tri.h` | (c) **done** (6288fa71) | Added by SNMetamorph in f736186 (2021-08-10); two HLSDK export declarations. |
| `game_shared/mathlib_defs.h` | (c) **done** (6288fa71) | Split from XashXT `mathlib.h` by SNMetamorph in f6b8392 (2021-11-20); XashXT's macros had no licence. The new macros keep the exact results the renderer relies on (`Q_floor` truncates, `Q_ceil` is `(int)(x+1)`, `Q_abs` is `fabsf`). |
| `game_shared/virtualfs.h`, `virtualfs.cpp` | (c) **done** (73aa278c) | "Copyright (C) XashXT Group 2014", from XashXT 0.81 (654c99f), no licence. Written new from the interface and behaviour measured by a black-box test (142 probe lines and the gl_shader.cpp read path over the 77 shader and data files): identical output. |
| `client/render/gl_studio_userdata.h` | (c) **done** (798db24a) | Created by SNMetamorph in aefa2fb (2024-02-16) from structs cut out of `engine/studio.h` (that file is under Valve's HLSDK header, but the structs are XashXT's additions). The new header keeps the members, types, order and constructors (the field list is the interface; the renderer clears one struct with memset). |
| `client/render/gl_cubemap.h` | (c) **done** (c99304fb) | Created by SNMetamorph in d9c5b7f (2022-02-06) moving `mcubemap_t` out of `common/com_model.h` (Uncle Mike, GPL-3.0+), then extended (a0d5c1f, b81cdc5). Would have been (a) by origin; replaced since it was 27 lines. |
| `client/render/gl_debug.h` | (c) **done** (c99304fb) | Created by SNMetamorph in fc87723 (2021-11-20) moving declarations out of `gl_local.h` (GPL-3.0+); same reasoning. |
| `public/build.h` | (a) public domain | Its header: "This is free and unencumbered software released into the public domain" (Unlicense). The earlier table read it as NONE. |
| `game_shared/custom_alloc.h` | (a) GPL (Paranoia 1.2); provenance note added | Header "simple memory allocator by BUzer"; 38 of its 48 lines are in Paranoia 1.2 `code_src/cl_dll/custom_alloc.h`, released under the GPL. In PrimeXT since 6521289. |
| `client/render/gl_aurora.cpp` | (a) GPL-3.0+, by descent; provenance note added | 512 of 637 lines are in XashXT 0.65 `client/render/r_particle.cpp`, "Copyright (C) 2011 Uncle Mike", GPL-3.0+; git sees 5bae6bc as a rename of it (77%). The Paranoia 2 copy lost the header; `gl_aurora.h` keeps it. Remaining doubt: the other 20% are Uncle Mike's Paranoia 2 changes, published without a notice. |
| `client/render/gl_sky.cpp` | (a) GPL, by descent; provenance note added | Header "written by BUzer for HL: Paranoia modification, 2006" (Paranoia 1.2, GPL). 115 of 229 lines are in XashXT `r_warp.cpp` (Uncle Mike, GPL-3.0+; git: rename, 35%). Only 3 lines are still in Paranoia 1.2's `gl_sky.cpp`. |
| `client/render/gl_debug.cpp` | (a) GPL, by descent; provenance note added | Same BUzer/Paranoia header. 39 of 271 lines in XashXT `r_debug.cpp` (GPL-3.0+; git: rename, 28%), 11 in Paranoia 1.2's `gl_debug.cpp`. Fallback (b): replace with a small new file (the error check plus empty debug views); loses only the developer views (`r_show*`, light frusta, tangent spaces), nothing a player sees. |
| `client/render/gl_decals.cpp` | (a) GPL, by descent (weak); provenance note added | Same BUzer/Paranoia header; only 52 of 1080 lines still in Paranoia 1.2's `gl_decals.cpp`, ~3% in XashXT's GPL decal files: Uncle Mike rewrote it for Paranoia 2. Rests on the file being a modified Paranoia 1.2 file (its header says so), which the GPL then covers. Not droppable: brush decals (bullet holes, scorches) and the decal shaders go through it. A rewrite would be ~1,900 lines. |
| `client/render/gl_postprocess.cpp` | (a) GPL, by descent (weakest); provenance note added | Same BUzer/Paranoia header, but 4 of 545 lines in Paranoia 1.2 and 61 in XashXT `r_postprocess.cpp` (GPL-3.0+); most of it is SNMetamorph's (tonemapping and bloom a99f726, auto-exposure 95999af, 2021). Fallback (b) is not a plain drop: `RenderFSQ` / `GL_DrawScreenSpaceQuad` live here and other files use them; dropping the rest loses bloom, tonemapping and auto-exposure, depth of field, sun shafts and the blur effects. Best path: ask SNMetamorph to state the licence of his additions (all his other files carry GPL-3.0+). |
| `client/render/gl_postprocess.h` | as gl_postprocess.cpp; provenance note added | Created by SNMetamorph in 43a9a1a (2021-11-24, "minor gl_postprocess refactoring") from the class in gl_postprocess.cpp. |
| `game_shared/stringlib.cpp`, `stringlib.h` | kept as is (owner, 2026-11-15); no licence statement found | "Copyright (C) XashXT Group 2011", in XashXT 0.65 (6521289). 188 of its 248 code lines (75%) are in Uncle Mike's `engine/common/crtlib.c` ("Copyright (C) 2011 Uncle Mike", GPL-3.0-or-later) as published in his engine source (FWGS xash3d-fwgs 8d6e3b7f, 2018-04-13, "Pure engine source code"; also https://github.com/a1batross/Xash3D_original). "XashXT Group" is Uncle Mike's group name: his Xash3D launcher's `game_launch/game.rc` gives it as the company (`VER_COMPANYNAME_STR "XashXT Group"`, in Xash3D_original). `stringlib.h` has no counterpart there. Not rewritten, by the owner's choice. |
| `game_shared/mathlib.cpp`, `mathlib.h`, `matrix.cpp`, `matrix.h`, `vector.h` | kept as is (owner, 2026-11-15); no licence statement found | "Copyright (C) Shambler Team 2005" (mathlib, vector) / "XashXT Group 2011" (matrix), in XashXT 0.65. Part of it is in Uncle Mike's GPL-3.0+ engine files (`engine/common/mathlib.c`, `matrixlib.c`, `mathlib.h` at xash3d-fwgs 8d6e3b7f): mathlib.cpp 22% of its lines, mathlib.h 13%, matrix.cpp 14%, matrix.h 10%, vector.h 3%. "Shambler Team" is the name on some Spirit of Half-Life code (e.g. `cl_dll/rain.h` in https://github.com/HLSources/Spirit-of-Half-Life: "Copyright (c) 1996-2004, Shambler Team", with Valve-style terms, non-commercial enhancements only); these 2005 files are not in Spirit. Not rewritten, by the owner's choice. |
| `game/ts/gfx/spotlight1.dds` .. `spotlight6.dds` | replaced: our own work (2026-11-24; tools/primext-data/, made from published methods, not from PrimeXT's file) | Added by SNMetamorph in 8ceaf8f (2022-11-02, "added missed textures used by render"); origin not established (probably Paranoia 2's or XashXT's game folder). Not a plain drop: the entity contract lets a light pick texture 1..7 (PRIMEXT-ENTITIES.md), and with the files missing every index falls back to the procedural default, which also makes `gl_scene.cpp`'s test for texture 1 true for every spotlight. Six 512x512 (one 128x128) DXT1 projection masks: small to replace with masks we draw; the look needs the owner. |
| `game/ts/gfx/brdf_approx.dds` | replaced: our own work (2026-11-24; tools/primext-data/, made from published methods, not from PrimeXT's file) | Added by SNMetamorph in 823654b (2022-06-05). 256x256, BC6H signed float (DXGI 96), 9 mips: the split-sum BRDF table of image-based lighting, which is computed, not drawn (Karis, "Real Shading in Unreal Engine 4", 2013). Regenerating needs a BC6H encoder or a format the loader takes uncompressed; then a game run to compare. |
| `game/ts/gfx/normalsfitting.dds` | replaced: our own work (2026-11-24; tools/primext-data/, made from published methods, not from PrimeXT's file) | Added in 8ceaf8f. 512x512 8-bit: looks like Crytek's best-fit-normals table (Kaplanyan, "CryENGINE 3: Reaching the Speed of Light", SIGGRAPH 2010), whose licence is unknown. The method is published; regenerating it is a brute-force search, feasible offline. |
| `game/ts/gfx/decals/decalinfo.txt` | replaced: our own work (2026-11-24; tools/primext-data/, made from published methods, not from PrimeXT's file) | Added in 54045da (2022-03-15). A list of decal groups and sizes (161 lines) for Half-Life's decal names. Facts more than expression, but written for HL: a TS list from TS's decals.wad is better anyway. Missing, gl_decals.cpp logs an error; what the decals then look like needs a game run. |
| `game/ts/gfx/particles/effects.txt` | replaced: our own work (2026-11-24; tools/primext-data/, made from published methods, not from PrimeXT's file) | Added in 54045da. Particle definitions whose textures (`gfx/particles/*.dds`) are not shipped in H9, so they draw nothing useful today. Drop, or write our own with our own textures. |
| `game/ts/gfx/grass/grassinfo.txt` | replaced: our own work (2026-11-24; tools/primext-data/, made from published methods, not from PrimeXT's file) | Added in 54045da. Three lines naming `gfx/grass/devgrass*.tga`, which H9 does not ship: no grass works with it today. Without it gl_grass.cpp logs "couldn't load grassinfo.txt". |
| `game/ts/scripts/materials.def`, `default_hl1.mat` (now `default_ts.mat`, made from TS's own `sound/materials.txt`; `default_hl1.mat` deleted) | replaced: our own work (2026-11-24; tools/primext-data/, made from published methods, not from PrimeXT's file) | Added in 54045da / 6b3f7ce / b4e2fbd (2022). Material kinds (impact decal, particles, sounds) and a texture-to-material list for Half-Life's own textures (1,552 lines). The HL list does not fit TS maps; a TS list made from TS's materials.txt is our own work. |

## Left after this branch

- The provenance notes are in `gl_aurora.cpp`, `gl_sky.cpp`, `gl_debug.cpp`, `gl_decals.cpp`, `gl_postprocess.cpp/.h`
  and `custom_alloc.h`. Each records where the file comes from and what its source states; it grants nothing itself.
- XashXT's shared library (`mathlib`, `matrix`, `vector`, `stringlib`) stays as it is by the owner's decision. No
  licence statement was found (section above); a PR has to say so.
- The 13 data files are replaced with our own (2026-11-24): `reTS/tools/primext-data/` holds the generators and a README
  with each file's method, licence and how close it comes to PrimeXT's. Game run: the hive remaster, no new log line;
  the split-sum table's look is untested in the game (no map with cubemaps and glossy models yet).
- The puddle decals `gfx/decals/puddle1-3.tga` are our own too: `tools/primext-data/src/make-puddles.mjs` (value noise,
  written for the hive v2 work, 2026-11-17) makes them byte for byte.
- **Open, for 2cwldys:** the module is GPL code built with 42 Valve HLSDK headers, and reTS itself is under the HLSDK
  licence (Valve's terms, free of charge only). The two licences do not combine cleanly. Whether the PR can ship
  pxrender.dll in reTS, and how it says so, is for 2cwldys to decide.
- The Paranoia 1.2 GPL statement has been read only on the unofficial mirror. The official Paranoia Toolkit archive
  (ModDB, 2008-01-10) is still to be checked; the Paranoia 2 SDK says nothing about it.
