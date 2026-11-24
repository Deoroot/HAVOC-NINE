# PrimeXT sources adapted in the server (mp.dll)

PrimeXT at 64fe05d (C:/tmp/primext/ext/PrimeXT). The repository has no LICENSE file; each file carries its own header.

| Here | From | Header there | What changed |
|---|---|---|---|
| `dlls/primext_server.cpp` `Server_GetPhysicsInterface`, `SV_CheckFeatures` | `server/physic.cpp:308-361` | GPL-3.0-or-later, Uncle Mike (Paranoia 2) | Only the idea and the export's shape: it answers the version and the feature mask. None of PrimeXT's physics callbacks, and the feature list is H9's own (`LOAD_DELUXEDATA`, `LARGE_LIGHTMAPS`, never the wire-changing ones). The struct is re-declared as pointers from FWGS `engine/physint.h` (GPL-3.0, Uncle Mike), layout only. The text of the function is written new. |
| `dlls/primext_server.cpp` `CDynamicLight` (env_dynlight) | `server/lights.cpp:553-830` | Valve HLSDK licence (non-commercial), as reTS | Adapted: same keys, `renderfx` 71, `renderamt` / 8, brightness in `framerate`, style in `skin`, second PVS point, toggling by `EF_NODRAW`. Dropped: the movie light, the style fades, the parent code, `DATADESC` (reTS uses `TYPEDESCRIPTION`). Flags go to `controller[0]`, not `effects`. |
| `dlls/primext_server.h` `H9_RENDERFX_DYNLIGHT`, `H9_RENDERFX_CINEMALIGHT` | `common/const.h:761-762` | (shared header, GPL-3.0 / Valve mix) | Two constant values (71, 72), nothing else. |
| `dlls/client.cpp` AddToFullPack | `server/client.cpp:1070-1085` | Valve HLSDK licence | The idea of a second PVS point for the light; written new. |
| `PRIMEXT-metamod-physint.patch` | none | Metamod-R, GPL-3.0 | New code in Metamod-R's own file. |
| `game/ts/sprites/null.spr` | none | | Generated here (a 1x1 transparent sprite); not PrimeXT's. |
