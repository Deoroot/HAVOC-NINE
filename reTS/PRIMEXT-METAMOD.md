# Metamod-R and Server_GetPhysicsInterface

**Finding: Metamod-R blocks it, and a small forward fixes it.**

The engine looks the symbol up on the library it loaded as the game (`sv_phys.c:2105`,
`COM_GetProcAddress( svgame.hInstance, "Server_GetPhysicsInterface" )`). H9's `liblist.gam` loads
`addons\metamod\dlls\metamod.dll`, so that is metamod.dll. Metamod-R (`metamod/src/dllapi.cpp:224`) exports the
symbol itself, with `// TODO: provide physint to plugins`, zeroes the whole table, sets only `SV_CreateEntity`
(for its linkent replacement) and returns TRUE. It never asks mp.dll. So the engine gets
`SV_CheckFeatures == NULL`, features stay 0 (`Host_ValidateEngineFeatures( mask, 0 )`), and mp.dll's export is never called.

## Evidence (C:/tmp/primext/srv/, test game C:/tmp/primext/h9run-srv, map smak_messhall)

| Run | Metamod | Log |
|---|---|---|
| `t1` | stock | no `PrimeXT:` line, no `EXT:` line: the export is never called, features 0 |
| `t2` | none (`gamedll "dlls\mp.dll"`, control) | `PrimeXT: SV_CheckFeatures -> 0x14`, `EXT: Deluxemap Support is enabled`, `EXT: Large Lightmaps is enabled` |
| `t3` | patched | `PrimeXT: Server_GetPhysicsInterface(version 6) called`, `SV_CheckFeatures -> 0x14`, both `EXT:` lines, and again on the client |

## The patch

`PRIMEXT-metamod-physint.patch` (against `metamod-r/metamod/src/dllapi.cpp` as H9 carries it, upstream 56590c5):
Metamod's `Server_GetPhysicsInterface` calls the game library's own (found with `g_GameDLL.sys_module.getsym`) into
a static table, copies that table to the engine's, and puts its own `SV_CreateEntity` on top (the game's
`SV_CreateEntity`, if it has one, is tried first). Built with `msvc\metamod.vcxproj`, toolset v145
(353,792 -> 356,864 bytes) and tested in the runs above.
It is carried in H9's `metamod-r/` as `h9-physint.patch` (applied in the source there, listed in `metamod-r/H9-SOURCE.txt`), and
`H9XASH/ts/addons/metamod/dlls/metamod.dll` is built with it (2026-11-24, 356,864 bytes; its `.patch` note). Plugins still do not see physint (the upstream TODO stays).

Without the patch, mp.dll works exactly as before (features 0): the lights still spawn, but deluxe data is not loaded.
