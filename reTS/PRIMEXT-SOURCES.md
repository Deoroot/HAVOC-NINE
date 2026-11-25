# Files taken from PrimeXT

H9 bookkeeping for the local branch `primext-client` (r_primext, the PrimeXT renderer as `cl_dlls/pxrender.dll`).
Origin: https://github.com/SNMetamorph/PrimeXT at **64fe05d** plus the local BSP2 fix, branch `s0-bsp2-nodes`
**9f112ca** (clone `C:/tmp/primext/ext/PrimeXT`). The origin path is relative to that tree. The licence column is
read from the file header in the origin; "changed" compares the copy with the origin (every H9 change carries a
comment starting `H9:`).

The repository itself has no LICENSE file. The GPL-3.0+ files are Uncle Mike's renderer (from Paranoia 2) and
SNMetamorph's additions; putting them in a client under the HLSDK licence is the owner's question, to be said in the PR.
Files marked NO LICENCE are not to be shipped in a PR: they are rewritten or dropped first. The verdict for every
file that had no licence header, with its sources, is in [PRIMEXT-LICENCES.md](PRIMEXT-LICENCES.md); the rows below say
which were replaced and which are still open.

Written new by H9 (not from PrimeXT): `cl_dll/primext_bridge.h`, `cl_dll/h9_primext.cpp`,
`cl_dll/primext/bridge/pxr_main.cpp`, `pxr_module.h`, `pxr_shells.cpp`, `cl_dll/primext/CMakeLists.txt`, and
`cl_dll/primext/client/hud.h` (replaces PrimeXT's). Written new from the interface to replace PrimeXT files that had
no licence: `common/exportdef.h`, `client/render/tri.h`, `game_shared/mathlib_defs.h`, `game_shared/virtualfs.h`,
`game_shared/virtualfs.cpp`, `client/render/gl_studio_userdata.h`, `client/render/gl_cubemap.h`,
`client/render/gl_debug.h` (all under `cl_dll/primext/`).

In `pxr_main.cpp`, `PXR_MsgPostFxSettings` (the PostFxSettings hook) follows PrimeXT `client/hud_msg.cpp`
`CHud::MsgFunc_PostFxSettings` (Valve HLSDK header there, as reTS): five lines, re-typed; the function body is the
same calls (`BEGIN_READ`, `ParseMessage`, `UpdateState`, `END_READ`). The renderfx-73 field copy beside it is H9's.

Totals as taken from PrimeXT: GPL-3.0+ 178, NONE 36, Valve HLSDK 42 (256 files). Of the 36 NONE: 8 replaced by new
files, 1 public domain, 7 GPL by descent (provenance comment added), 7 code files kept as is by the owner with no licence statement found, 13 data files replaced with our own work (2026-11-24, reTS/tools/primext-data/).

| File in reTS | Origin in PrimeXT | Licence | Changed | Note |
|---|---|---|---|---|
| `cl_dll/primext/client/brush_material.h` | `client/brush_material.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/enginecallback.h` | `client/enginecallback.h` | GPL-3.0+ | yes (marked H9) |  |
| `cl_dll/primext/client/entities/cl_env_dynlight.cpp` | `client/entities/cl_env_dynlight.cpp` | GPL-3.0+ | yes (marked H9) |  |
| `cl_dll/primext/client/entities/cl_env_dynlight.h` | `client/entities/cl_env_dynlight.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/hud.h` | `client/hud.h` | Valve HLSDK | replaced by a new file (H9) |  |
| `cl_dll/primext/client/parsemsg.cpp` | `client/parsemsg.cpp` | Valve HLSDK | no |  |
| `cl_dll/primext/client/parsemsg.h` | `client/parsemsg.h` | Valve HLSDK | no |  |
| `cl_dll/primext/client/postfx_controller.cpp` | `client/postfx_controller.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/postfx_controller.h` | `client/postfx_controller.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/postfx_parameters.cpp` | `client/postfx_parameters.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/postfx_parameters.h` | `client/postfx_parameters.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/r_view.h` | `client/r_view.h` | Valve HLSDK | no |  |
| `cl_dll/primext/client/render/cl_dlight.h` | `client/render/cl_dlight.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_aurora.cpp` | `client/render/gl_aurora.cpp` | NONE (no header) | yes (H9 provenance comment at the top) | GPL-3.0+ by descent: 80% of its lines are XashXT `r_particle.cpp` (Uncle Mike, GPL-3.0+); see PRIMEXT-LICENCES.md |
| `cl_dll/primext/client/render/gl_aurora.h` | `client/render/gl_aurora.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_backend.cpp` | `client/render/gl_backend.cpp` | GPL-3.0+ | yes (marked H9) |  |
| `cl_dll/primext/client/render/gl_cubemap.cpp` | `client/render/gl_cubemap.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_cubemap.h` | `client/render/gl_cubemap.h` | NONE there | replaced by a new file (H9, written from the interface) | licence cleared: no PrimeXT text left |
| `cl_dll/primext/client/render/gl_cull.cpp` | `client/render/gl_cull.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_cvars.cpp` | `client/render/gl_cvars.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_cvars.h` | `client/render/gl_cvars.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_debug.cpp` | `client/render/gl_debug.cpp` | NONE (no header) | yes (H9 provenance comment at the top) | GPL by descent: Paranoia 1.2 source (BUzer, GPL) and XashXT `r_debug.cpp` (GPL-3.0+); see PRIMEXT-LICENCES.md |
| `cl_dll/primext/client/render/gl_debug.h` | `client/render/gl_debug.h` | NONE there | replaced by a new file (H9, written from the interface) | licence cleared: no PrimeXT text left |
| `cl_dll/primext/client/render/gl_debug_overlay_2d.cpp` | `client/render/gl_debug_overlay_2d.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_debug_overlay_2d.h` | `client/render/gl_debug_overlay_2d.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_debug_scope.cpp` | `client/render/gl_debug_scope.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_debug_scope.h` | `client/render/gl_debug_scope.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_debug_visualizer_backend.cpp` | `client/render/gl_debug_visualizer_backend.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_debug_visualizer_backend.h` | `client/render/gl_debug_visualizer_backend.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_decals.cpp` | `client/render/gl_decals.cpp` | NONE (no header) | yes (H9 provenance comment at the top) | GPL by descent: Paranoia 1.2 source (BUzer, GPL), reworked for Paranoia 2; see PRIMEXT-LICENCES.md |
| `cl_dll/primext/client/render/gl_decals.h` | `client/render/gl_decals.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_dlight.cpp` | `client/render/gl_dlight.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_export.cpp` | `client/render/gl_export.cpp` | GPL-3.0+ | yes (marked H9) |  |
| `cl_dll/primext/client/render/gl_export.h` | `client/render/gl_export.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_framebuffer.cpp` | `client/render/gl_framebuffer.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_framebuffer.h` | `client/render/gl_framebuffer.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_grass.cpp` | `client/render/gl_grass.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_grass.h` | `client/render/gl_grass.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_lightmap.cpp` | `client/render/gl_lightmap.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_local.h` | `client/render/gl_local.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_movie.cpp` | `client/render/gl_movie.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_occlusion.cpp` | `client/render/gl_occlusion.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_occlusion.h` | `client/render/gl_occlusion.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_postprocess.cpp` | `client/render/gl_postprocess.cpp` | NONE (no header) | yes (H9 provenance comment at the top) | GPL by descent (weakest case): Paranoia 1.2 header, XashXT `r_postprocess.cpp` (GPL-3.0+); see PRIMEXT-LICENCES.md |
| `cl_dll/primext/client/render/gl_postprocess.h` | `client/render/gl_postprocess.h` | NONE (no header) | yes (H9 provenance comment at the top) | split out of gl_postprocess.cpp (PrimeXT 43a9a1a): as that file; see PRIMEXT-LICENCES.md |
| `cl_dll/primext/client/render/gl_primitive.cpp` | `client/render/gl_primitive.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_primitive.h` | `client/render/gl_primitive.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_rmain.cpp` | `client/render/gl_rmain.cpp` | GPL-3.0+ | yes (marked H9) |  |
| `cl_dll/primext/client/render/gl_rmisc.cpp` | `client/render/gl_rmisc.cpp` | GPL-3.0+ | yes (marked H9) |  |
| `cl_dll/primext/client/render/gl_rpart.cpp` | `client/render/gl_rpart.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_rpart.h` | `client/render/gl_rpart.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_rsurf.cpp` | `client/render/gl_rsurf.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_scene.cpp` | `client/render/gl_scene.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_shader.cpp` | `client/render/gl_shader.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_shader.h` | `client/render/gl_shader.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_shadowmap.cpp` | `client/render/gl_shadowmap.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_shadows.cpp` | `client/render/gl_shadows.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_sky.cpp` | `client/render/gl_sky.cpp` | NONE (no header) | yes (H9 provenance comment at the top) | GPL by descent: half its lines are XashXT `r_warp.cpp` (Uncle Mike, GPL-3.0+), header Paranoia 1.2 (GPL); see PRIMEXT-LICENCES.md |
| `cl_dll/primext/client/render/gl_slight.cpp` | `client/render/gl_slight.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_sprite.cpp` | `client/render/gl_sprite.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_sprite.h` | `client/render/gl_sprite.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_studio.h` | `client/render/gl_studio.h` | GPL-3.0+ | yes (marked H9) |  |
| `cl_dll/primext/client/render/gl_studio_draw.cpp` | `client/render/gl_studio_draw.cpp` | GPL-3.0+ | yes (marked H9) |  |
| `cl_dll/primext/client/render/gl_studio_init.cpp` | `client/render/gl_studio_init.cpp` | GPL-3.0+ | yes (marked H9) |  |
| `cl_dll/primext/client/render/gl_studio_userdata.h` | `client/render/gl_studio_userdata.h` | NONE there | replaced by a new file (H9, written from the interface) | licence cleared: no PrimeXT text left |
| `cl_dll/primext/client/render/gl_studiodecal.h` | `client/render/gl_studiodecal.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_studiodecal_new.cpp` | `client/render/gl_studiodecal_new.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_studiovbo.cpp` | `client/render/gl_studiovbo.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_subview.cpp` | `client/render/gl_subview.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_unit_cube.cpp` | `client/render/gl_unit_cube.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_unit_cube.h` | `client/render/gl_unit_cube.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_viewport.cpp` | `client/render/gl_viewport.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_viewport.h` | `client/render/gl_viewport.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_world.h` | `client/render/gl_world.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/gl_world_new.cpp` | `client/render/gl_world_new.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/r_weather.cpp` | `client/render/r_weather.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/r_weather.h` | `client/render/r_weather.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/tbnfile.h` | `client/render/tbnfile.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/texture_handle.cpp` | `client/render/texture_handle.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/texture_handle.h` | `client/render/texture_handle.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/render/tri.cpp` | `client/render/tri.cpp` | Valve HLSDK | yes (marked H9) |  |
| `cl_dll/primext/client/render/tri.h` | `client/render/tri.h` | NONE there | replaced by a new file (H9, written from the interface) | licence cleared: no PrimeXT text left |
| `cl_dll/primext/client/render/vertex_fmt.h` | `client/render/vertex_fmt.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/studio_material.h` | `client/studio_material.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/utils.cpp` | `client/utils.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/utils.h` | `client/utils.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/visualizer/debug_visualizer.cpp` | `client/visualizer/debug_visualizer.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/client/visualizer/debug_visualizer.h` | `client/visualizer/debug_visualizer.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/common/VFileSystem009.h` | `common/VFileSystem009.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/common/areanode.h` | `common/areanode.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/common/beamdef.h` | `common/beamdef.h` | Valve HLSDK | no |  |
| `cl_dll/primext/common/bspfile.h` | `common/bspfile.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/common/cl_entity.h` | `common/cl_entity.h` | Valve HLSDK | no |  |
| `cl_dll/primext/common/color24.h` | `common/color24.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/common/com_model.h` | `common/com_model.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/common/const.h` | `common/const.h` | Valve HLSDK | no |  |
| `cl_dll/primext/common/cvardef.h` | `common/cvardef.h` | Valve HLSDK | no |  |
| `cl_dll/primext/common/dlight.h` | `common/dlight.h` | Valve HLSDK | no |  |
| `cl_dll/primext/common/enginefeatures.h` | `common/enginefeatures.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/common/entity_state.h` | `common/entity_state.h` | Valve HLSDK | no |  |
| `cl_dll/primext/common/entity_types.h` | `common/entity_types.h` | Valve HLSDK | no |  |
| `cl_dll/primext/common/event_api.h` | `common/event_api.h` | Valve HLSDK | no |  |
| `cl_dll/primext/common/event_args.h` | `common/event_args.h` | Valve HLSDK | no |  |
| `cl_dll/primext/common/exportdef.h` | `common/exportdef.h` | NONE there | replaced by a new file (H9, written from the interface) | licence cleared: no PrimeXT text left |
| `cl_dll/primext/common/lightlimits.h` | `common/lightlimits.h` | Valve HLSDK | no |  |
| `cl_dll/primext/common/lightstyle.h` | `common/lightstyle.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/common/particledef.h` | `common/particledef.h` | Valve HLSDK | no |  |
| `cl_dll/primext/common/pmtrace.h` | `common/pmtrace.h` | Valve HLSDK | no |  |
| `cl_dll/primext/common/port.h` | `common/port.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/common/r_efx.h` | `common/r_efx.h` | Valve HLSDK | no |  |
| `cl_dll/primext/common/r_studioint.h` | `common/r_studioint.h` | Valve HLSDK | no |  |
| `cl_dll/primext/common/ref_params.h` | `common/ref_params.h` | Valve HLSDK | no |  |
| `cl_dll/primext/common/render_api.h` | `common/render_api.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/common/screenfade.h` | `common/screenfade.h` | Valve HLSDK | no |  |
| `cl_dll/primext/common/shader.h` | `common/shader.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/common/triangleapi.h` | `common/triangleapi.h` | Valve HLSDK | no |  |
| `cl_dll/primext/common/usercmd.h` | `common/usercmd.h` | Valve HLSDK | no |  |
| `cl_dll/primext/common/weaponinfo.h` | `common/weaponinfo.h` | Valve HLSDK | no |  |
| `cl_dll/primext/common/wrect.h` | `common/wrect.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/engine/alert.h` | `engine/alert.h` | Valve HLSDK | no |  |
| `cl_dll/primext/engine/anorms.h` | `engine/anorms.h` | Valve HLSDK | no |  |
| `cl_dll/primext/engine/cdll_int.h` | `engine/cdll_int.h` | Valve HLSDK | no |  |
| `cl_dll/primext/engine/custom.h` | `engine/custom.h` | Valve HLSDK | no |  |
| `cl_dll/primext/engine/mobility_int.h` | `engine/mobility_int.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/engine/shake.h` | `engine/shake.h` | Valve HLSDK | no |  |
| `cl_dll/primext/engine/sprite.h` | `engine/sprite.h` | Valve HLSDK | no |  |
| `cl_dll/primext/engine/studio.h` | `engine/studio.h` | Valve HLSDK | no |  |
| `cl_dll/primext/game_shared/bone_setup.cpp` | `game_shared/bone_setup.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/bounding_box.cpp` | `game_shared/bounding_box.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/bounding_box.h` | `game_shared/bounding_box.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/bs_defs.h` | `game_shared/bs_defs.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/cdll_dll.h` | `game_shared/cdll_dll.h` | Valve HLSDK | no |  |
| `cl_dll/primext/game_shared/clipfile.h` | `game_shared/clipfile.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/common.cpp` | `game_shared/common.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/cubemap.h` | `game_shared/cubemap.h` | Valve HLSDK | no |  |
| `cl_dll/primext/game_shared/custom_alloc.h` | `game_shared/custom_alloc.h` | NONE (no header) | yes (H9 provenance comment at the top) | GPL: 79% of its lines are Paranoia 1.2 `cl_dll/custom_alloc.h` (BUzer, GPL); see PRIMEXT-LICENCES.md |
| `cl_dll/primext/game_shared/filesystem_manager.cpp` | `game_shared/filesystem_manager.cpp` | GPL-3.0+ | yes (marked H9) |  |
| `cl_dll/primext/game_shared/filesystem_manager.h` | `game_shared/filesystem_manager.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/filesystem_utils.cpp` | `game_shared/filesystem_utils.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/filesystem_utils.h` | `game_shared/filesystem_utils.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/frustum.cpp` | `game_shared/frustum.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/frustum.h` | `game_shared/frustum.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/ikcontext.cpp` | `game_shared/ikcontext.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/ikcontext.h` | `game_shared/ikcontext.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/iksolver.h` | `game_shared/iksolver.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/jigglebones.cpp` | `game_shared/jigglebones.cpp` | Valve HLSDK | no |  |
| `cl_dll/primext/game_shared/jigglebones.h` | `game_shared/jigglebones.h` | Valve HLSDK | no |  |
| `cl_dll/primext/game_shared/material.cpp` | `game_shared/material.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/material.h` | `game_shared/material.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/mathlib.cpp` | `game_shared/mathlib.cpp` | NONE (XashXT Group / Shambler Team, Uncle Mike) | no | kept as is (owner, 2026-11-15): no licence statement found anywhere (see PRIMEXT-LICENCES.md) |
| `cl_dll/primext/game_shared/mathlib.h` | `game_shared/mathlib.h` | NONE (XashXT Group / Shambler Team, Uncle Mike) | no | kept as is (owner, 2026-11-15): no licence statement found anywhere (see PRIMEXT-LICENCES.md) |
| `cl_dll/primext/game_shared/mathlib_defs.h` | `game_shared/mathlib_defs.h` | NONE there | replaced by a new file (H9, written from the interface) | licence cleared: no PrimeXT text left |
| `cl_dll/primext/game_shared/matrix.cpp` | `game_shared/matrix.cpp` | NONE (XashXT Group / Shambler Team, Uncle Mike) | no | kept as is (owner, 2026-11-15): no licence statement found anywhere (see PRIMEXT-LICENCES.md) |
| `cl_dll/primext/game_shared/matrix.h` | `game_shared/matrix.h` | NONE (XashXT Group / Shambler Team, Uncle Mike) | no | kept as is (owner, 2026-11-15): no licence statement found anywhere (see PRIMEXT-LICENCES.md) |
| `cl_dll/primext/game_shared/meshdesc.cpp` | `game_shared/meshdesc.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/meshdesc.h` | `game_shared/meshdesc.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/meshdesc_factory.cpp` | `game_shared/meshdesc_factory.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/meshdesc_factory.h` | `game_shared/meshdesc_factory.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/pm_defs.h` | `game_shared/pm_defs.h` | Valve HLSDK | no |  |
| `cl_dll/primext/game_shared/pm_info.h` | `game_shared/pm_info.h` | Valve HLSDK | no |  |
| `cl_dll/primext/game_shared/pm_movevars.h` | `game_shared/pm_movevars.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/procbones.cpp` | `game_shared/procbones.cpp` | Valve HLSDK | no |  |
| `cl_dll/primext/game_shared/randomrange.h` | `game_shared/randomrange.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/stringlib.cpp` | `game_shared/stringlib.cpp` | NONE (XashXT Group / Shambler Team, Uncle Mike) | no | kept as is (owner, 2026-11-15): no licence statement found anywhere (see PRIMEXT-LICENCES.md) |
| `cl_dll/primext/game_shared/stringlib.h` | `game_shared/stringlib.h` | NONE (XashXT Group / Shambler Team, Uncle Mike) | no | kept as is (owner, 2026-11-15): no licence statement found anywhere (see PRIMEXT-LICENCES.md) |
| `cl_dll/primext/game_shared/trace.cpp` | `game_shared/trace.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/trace.h` | `game_shared/trace.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/game_shared/utlarray.h` | `game_shared/utlarray.h` | Valve HLSDK | no |  |
| `cl_dll/primext/game_shared/utlblockmemory.h` | `game_shared/utlblockmemory.h` | Valve HLSDK | no |  |
| `cl_dll/primext/game_shared/utllinkedlist.h` | `game_shared/utllinkedlist.h` | Valve HLSDK | no |  |
| `cl_dll/primext/game_shared/utlmemory.h` | `game_shared/utlmemory.h` | Valve HLSDK | no |  |
| `cl_dll/primext/game_shared/vector.h` | `game_shared/vector.h` | NONE (XashXT Group / Shambler Team, Uncle Mike) | no | kept as is (owner, 2026-11-15): no licence statement found anywhere (see PRIMEXT-LICENCES.md) |
| `cl_dll/primext/game_shared/virtualfs.cpp` | `game_shared/virtualfs.cpp` | NONE there | replaced by a new file (H9, written from the interface) | licence cleared: no PrimeXT text left |
| `cl_dll/primext/game_shared/virtualfs.h` | `game_shared/virtualfs.h` | NONE there | replaced by a new file (H9, written from the interface) | licence cleared: no PrimeXT text left |
| `cl_dll/primext/public/basetypes.h` | `public/basetypes.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/public/build.h` | `public/build.h` | public domain (Unlicense text in its header) | no |  |
| `cl_dll/primext/public/crclib.cpp` | `public/crclib.cpp` | GPL-3.0+ | no |  |
| `cl_dll/primext/public/crclib.h` | `public/crclib.h` | GPL-3.0+ | no |  |
| `cl_dll/primext/public/profiler.h` | `public/profiler.h` | GPL-3.0+ | no |  |
| `game/ts/gfx/brdf_approx.dds` | `game_dir/gfx/brdf_approx.dds` | ours (as the rest of reTS's H9 code) | no | **replaced: our own work (2026-11-24; tools/primext-data/, made from published methods, not from PrimeXT's file)** (see PRIMEXT-LICENCES.md) |
| `game/ts/gfx/decals/decalinfo.txt` | `game_dir/gfx/decals/decalinfo.txt` | ours (as the rest of reTS's H9 code) | no | **replaced: our own work (2026-11-24; tools/primext-data/, made from published methods, not from PrimeXT's file)** (see PRIMEXT-LICENCES.md) |
| `game/ts/gfx/grass/grassinfo.txt` | `game_dir/gfx/grass/grassinfo.txt` | ours (as the rest of reTS's H9 code) | no | **replaced: our own work (2026-11-24; tools/primext-data/, made from published methods, not from PrimeXT's file)** (see PRIMEXT-LICENCES.md) |
| `game/ts/gfx/normalsfitting.dds` | `game_dir/gfx/normalsfitting.dds` | ours (as the rest of reTS's H9 code) | no | **replaced: our own work (2026-11-24; tools/primext-data/, made from published methods, not from PrimeXT's file)** (see PRIMEXT-LICENCES.md) |
| `game/ts/gfx/particles/effects.txt` | `game_dir/gfx/particles/effects.txt` | ours (as the rest of reTS's H9 code) | no | **replaced: our own work (2026-11-24; tools/primext-data/, made from published methods, not from PrimeXT's file)** (see PRIMEXT-LICENCES.md) |
| `game/ts/gfx/spotlight1.dds` | `game_dir/gfx/spotlight1.dds` | ours (as the rest of reTS's H9 code) | no | **replaced: our own work (2026-11-24; tools/primext-data/, made from published methods, not from PrimeXT's file)** (see PRIMEXT-LICENCES.md) |
| `game/ts/gfx/spotlight2.dds` | `game_dir/gfx/spotlight2.dds` | ours (as the rest of reTS's H9 code) | no | **replaced: our own work (2026-11-24; tools/primext-data/, made from published methods, not from PrimeXT's file)** (see PRIMEXT-LICENCES.md) |
| `game/ts/gfx/spotlight3.dds` | `game_dir/gfx/spotlight3.dds` | ours (as the rest of reTS's H9 code) | no | **replaced: our own work (2026-11-24; tools/primext-data/, made from published methods, not from PrimeXT's file)** (see PRIMEXT-LICENCES.md) |
| `game/ts/gfx/spotlight4.dds` | `game_dir/gfx/spotlight4.dds` | ours (as the rest of reTS's H9 code) | no | **replaced: our own work (2026-11-24; tools/primext-data/, made from published methods, not from PrimeXT's file)** (see PRIMEXT-LICENCES.md) |
| `game/ts/gfx/spotlight5.dds` | `game_dir/gfx/spotlight5.dds` | ours (as the rest of reTS's H9 code) | no | **replaced: our own work (2026-11-24; tools/primext-data/, made from published methods, not from PrimeXT's file)** (see PRIMEXT-LICENCES.md) |
| `game/ts/gfx/spotlight6.dds` | `game_dir/gfx/spotlight6.dds` | ours (as the rest of reTS's H9 code) | no | **replaced: our own work (2026-11-24; tools/primext-data/, made from published methods, not from PrimeXT's file)** (see PRIMEXT-LICENCES.md) |
| `game/ts/glsl/alpha2coverage.h` | `game_dir/glsl/alpha2coverage.h` | GPL-3.0+ | no |  |
| `game/ts/glsl/common/debug_draw_fp.glsl` | `game_dir/glsl/common/debug_draw_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/common/debug_draw_vp.glsl` | `game_dir/glsl/common/debug_draw_vp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/common/debug_overlay2d_fp.glsl` | `game_dir/glsl/common/debug_overlay2d_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/common/debug_overlay2d_vp.glsl` | `game_dir/glsl/common/debug_overlay2d_vp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/common/ibl_filter_specular_fp.glsl` | `game_dir/glsl/common/ibl_filter_specular_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/common/ibl_filter_specular_vp.glsl` | `game_dir/glsl/common/ibl_filter_specular_vp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/const.h` | `game_dir/glsl/const.h` | GPL-3.0+ | no |  |
| `game/ts/glsl/cubemap.h` | `game_dir/glsl/cubemap.h` | GPL-3.0+ | no |  |
| `game/ts/glsl/deluxemap.h` | `game_dir/glsl/deluxemap.h` | GPL-3.0+ | no |  |
| `game/ts/glsl/fog.h` | `game_dir/glsl/fog.h` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/decal_bmodel_fp.glsl` | `game_dir/glsl/forward/decal_bmodel_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/decal_bmodel_vp.glsl` | `game_dir/glsl/forward/decal_bmodel_vp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/decal_studio_fp.glsl` | `game_dir/glsl/forward/decal_studio_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/decal_studio_vp.glsl` | `game_dir/glsl/forward/decal_studio_vp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/depth_bmodel_fp.glsl` | `game_dir/glsl/forward/depth_bmodel_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/depth_bmodel_vp.glsl` | `game_dir/glsl/forward/depth_bmodel_vp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/depth_grass_fp.glsl` | `game_dir/glsl/forward/depth_grass_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/depth_grass_vp.glsl` | `game_dir/glsl/forward/depth_grass_vp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/depth_studio_fp.glsl` | `game_dir/glsl/forward/depth_studio_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/depth_studio_vp.glsl` | `game_dir/glsl/forward/depth_studio_vp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/generic_vp.glsl` | `game_dir/glsl/forward/generic_vp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/light_bmodel_fp.glsl` | `game_dir/glsl/forward/light_bmodel_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/light_bmodel_vp.glsl` | `game_dir/glsl/forward/light_bmodel_vp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/light_grass_fp.glsl` | `game_dir/glsl/forward/light_grass_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/light_grass_vp.glsl` | `game_dir/glsl/forward/light_grass_vp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/light_studio_fp.glsl` | `game_dir/glsl/forward/light_studio_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/light_studio_vp.glsl` | `game_dir/glsl/forward/light_studio_vp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/scene_bmodel_fp.glsl` | `game_dir/glsl/forward/scene_bmodel_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/scene_bmodel_vp.glsl` | `game_dir/glsl/forward/scene_bmodel_vp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/scene_grass_fp.glsl` | `game_dir/glsl/forward/scene_grass_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/scene_grass_vp.glsl` | `game_dir/glsl/forward/scene_grass_vp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/scene_studio_fp.glsl` | `game_dir/glsl/forward/scene_studio_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/scene_studio_vp.glsl` | `game_dir/glsl/forward/scene_studio_vp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/forward/skybox_fp.glsl` | `game_dir/glsl/forward/skybox_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/fresnel.h` | `game_dir/glsl/fresnel.h` | GPL-3.0+ | no |  |
| `game/ts/glsl/grassdata.h` | `game_dir/glsl/grassdata.h` | GPL-3.0+ | no |  |
| `game/ts/glsl/ibl.h` | `game_dir/glsl/ibl.h` | GPL-3.0+ | no |  |
| `game/ts/glsl/indirect.h` | `game_dir/glsl/indirect.h` | GPL-3.0+ | no |  |
| `game/ts/glsl/lightmodel.h` | `game_dir/glsl/lightmodel.h` | GPL-3.0+ | no |  |
| `game/ts/glsl/material.h` | `game_dir/glsl/material.h` | GPL-3.0+ | no |  |
| `game/ts/glsl/mathlib.h` | `game_dir/glsl/mathlib.h` | GPL-3.0+ | no |  |
| `game/ts/glsl/matrix.h` | `game_dir/glsl/matrix.h` | GPL-3.0+ | no |  |
| `game/ts/glsl/parallax.h` | `game_dir/glsl/parallax.h` | GPL-3.0+ | no |  |
| `game/ts/glsl/postfx/bloom_fp.glsl` | `game_dir/glsl/postfx/bloom_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/postfx/dofbokeh_fp.glsl` | `game_dir/glsl/postfx/dofbokeh_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/postfx/downscale_luminance_fp.glsl` | `game_dir/glsl/postfx/downscale_luminance_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/postfx/drawshafts_fp.glsl` | `game_dir/glsl/postfx/drawshafts_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/postfx/gaussblur_fp.glsl` | `game_dir/glsl/postfx/gaussblur_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/postfx/gaussblurmip_fp.glsl` | `game_dir/glsl/postfx/gaussblurmip_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/postfx/generate_exposure_fp.glsl` | `game_dir/glsl/postfx/generate_exposure_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/postfx/generate_luminance_fp.glsl` | `game_dir/glsl/postfx/generate_luminance_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/postfx/generic_vp.glsl` | `game_dir/glsl/postfx/generic_vp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/postfx/genshafts_fp.glsl` | `game_dir/glsl/postfx/genshafts_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/postfx/postprocessing_fp.glsl` | `game_dir/glsl/postfx/postprocessing_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/postfx/tonemap_fp.glsl` | `game_dir/glsl/postfx/tonemap_fp.glsl` | GPL-3.0+ | no |  |
| `game/ts/glsl/screen.h` | `game_dir/glsl/screen.h` | GPL-3.0+ | no |  |
| `game/ts/glsl/shadow_omni.h` | `game_dir/glsl/shadow_omni.h` | GPL-3.0+ | no |  |
| `game/ts/glsl/shadow_proj.h` | `game_dir/glsl/shadow_proj.h` | GPL-3.0+ | no |  |
| `game/ts/glsl/shadow_spot.h` | `game_dir/glsl/shadow_spot.h` | GPL-3.0+ | no |  |
| `game/ts/glsl/skinning.h` | `game_dir/glsl/skinning.h` | GPL-3.0+ | no |  |
| `game/ts/glsl/terrain.h` | `game_dir/glsl/terrain.h` | GPL-3.0+ | no |  |
| `game/ts/glsl/texfetch.h` | `game_dir/glsl/texfetch.h` | GPL-3.0+ | no |  |
| `game/ts/glsl/tnbasis.h` | `game_dir/glsl/tnbasis.h` | GPL-3.0+ | no |  |
| `game/ts/scripts/default_ts.mat` (replaces `default_hl1.mat`, deleted) | `game_dir/scripts/default_hl1.mat` (not used) | ours (as the rest of reTS's H9 code) | no | **replaced: our own work (2026-11-24; tools/primext-data/, made from published methods, not from PrimeXT's file)** (see PRIMEXT-LICENCES.md) |
| `game/ts/scripts/materials.def` | `game_dir/scripts/materials.def` | ours (as the rest of reTS's H9 code) | no | **replaced: our own work (2026-11-24; tools/primext-data/, made from published methods, not from PrimeXT's file)** (see PRIMEXT-LICENCES.md) |
