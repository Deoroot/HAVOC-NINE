# PrimeXT entities: what the server sends, what the client reads

Server: `reTS/dlls/primext_server.cpp` (mp.dll). Client (agent A's renderer) reads `cl_entity_t::curstate`.
Every field below was checked against `H9XASH/ts/delta.lst` (= `reTS/game/ts/delta.lst`, section `entity_state_t`).
H9's engine widens only `modelindex` (12 bits) and the entity number; nothing else changes.

## env_dynlight

Identified by `curstate.renderfx == 71` (`H9_RENDERFX_DYNLIGHT`; 72 is reserved for a movie light and never sent).
renderfx 64 is H9's xray, 0..20 are Valve/reTS; 71 and 72 collide with nothing in reTS, H9XASH or FWGS `const.h`.
The entity is a sprite (`sprites/null.spr`, a 1x1 blank, shipped in `reTS/game/ts/sprites/`) because an entity
with no model is not sent. **The client must not draw it** (the stock client does, and it is blank).

| Meaning | `curstate` field | Wire (delta.lst) | Range / precision | Server sets |
|---|---|---|---|---|
| position | `origin` | 19 bit /8 | 1/8 unit | the entity origin |
| spot direction | `angles` | 16 bit angle | | `angles` key (pitch, yaw, roll as PrimeXT) |
| colour | `rendercolor.r/g/b` | 8 bit each | 0..255 | `rendercolor` key |
| radius | `renderamt` | 8 bit | **radius = renderamt * 8**, 8..2040 units | key `renderamt` (distance) / 8 |
| cutoff angle | `scale` | 16 bit /256 | 0 = omni; > 0 = spot, degrees, up to 255 | key `scale` |
| brightness | `framerate` | **signed 8 bit /16** | 0..7.9375, steps of 1/16 (default 2) | key `brightness`, clamped to 7.9 |
| light style | `skin & 0x3F` | 12 bit signed | 0..63; 0-11 stock styles, >= 32 custom (`pattern` key, sent by LIGHT_STYLE) | key `style` |
| spot texture | `rendermode` | 8 bit | 0..7 (PrimeXT's) | key `rendermode` |
| **switches** | `controller[0]` | 8 bit | bit0 0x01 NOSHADOW, bit1 0x02 NOBUMP, bit2 0x04 LENSFLARE | spawnflags 2, 4, 8 |

- **Off = the entity is not sent.** `use` toggles `EF_NODRAW`; `AddToFullPack` then drops the entity, so the client
  sees the light disappear (the start-off flag, spawnflag 1, is the same). The client needs no flag for it.
- **Why not `effects`.** It is 8 bits on H9's delta.lst, so Xash's `EF_NOSHADOW` (1<<28), `EF_NOBUMP` (1<<17) and
  `EF_LENSFLARE` (1<<18) would be cut. `controller[0]` is an 8-bit byte nothing else uses on a sprite entity.
- **Not carried**, because delta.lst has no field for it on `entity_state_t`: `fuser2` (the movie's time), so no cinema
  light. `body`, `aiment` and `movetype` ride as for any entity (a light parented with `aiment` + `MOVETYPE_FOLLOW`
  would work as in PrimeXT's `GetLightVectors`) but reTS has no parent system, so no map uses it yet.
- A spotlight has a second PVS point (an `info_target` where the cone ends, moved every 0.1 s) so it is sent when its
  pool is visible but its origin is not. Client side nothing differs.
- Count: nothing limits env_dynlights on the server; PrimeXT draws 64 a frame. A map should keep under that.
- `renderamt`, `scale` and `framerate` are written once in `Spawn`; a `scale` of 0 is an omni light.

Map keys (as PrimeXT's fgd): `rendercolor`, `renderamt` (distance), `brightness`, `scale`, `rendermode`, `style`,
`pattern`, `angles`, `targetname`, spawnflags (1 start off, 2 no shadow, 4 no bump, 8 lens flare). Fire with
`USE_ON`, `USE_OFF`, `USE_TOGGLE`. PrimeXT's `m_iTurnOnStyle` and friends, and `message` (movie), are accepted and ignored.

## Test aids (server console)

- `h9_pxt_lights`: for every env_dynlight, runs the real `AddToFullPack` and prints the state it sends and what the
  delta quantisation leaves of `framerate` and `scale`.
- `h9_pxt_use <targetname> [on|off|toggle]`.
- `H9_PXT_NULLSPR=sprites/glow01.spr` (environment, before the game starts): the lights are sent with a visible
  sprite, so a stock client shows a glow where each light is (this is how toggling was seen on the client).

## Engine features (`Server_GetPhysicsInterface`)

mp.dll answers `SV_CheckFeatures` with `ENGINE_LOAD_DELUXEDATA | ENGINE_LARGE_LIGHTMAPS` (0x14). The engine sends
the mask to the client at connect, so `host.features` is the same on both sides.
`ENGINE_LINEAR_GAMMA_SPACE` (0x100) is not set: see below. `WRITE_LARGE_COORD`, `COMPENSATE_QUAKE_BUG` and
`PHYSICS_PUSHER_EXT` are masked out in code and cannot be set at all.

Environment `H9_PXT_FEATURES` (read when the library loads): `0` = off, `0x14` = default, `0x114` = with linear gamma.
Compile-time default: `-DH9_PXT_DEFAULT_FEATURES=<mask>`. (A cvar would be read too late: the engine asks before
`GameDLLInit`.)

- LOAD_DELUXEDATA: `mod_bmodel.c:3964` loads BSPX `LIGHTINGDIR` / XASH `LIGHTVECS` and sets `FWORLD_HAS_DELUXEMAP`; a map
  with neither is unchanged. PrimeXT's static bump needs it.
- LARGE_LIGHTMAPS: `gl_rsurf.c:3928` makes lightmap blocks 1024 instead of 128; ref_gl already does it for every BSP2
  map (all H9 RP maps), so those are unchanged. Only v30 maps change (they stop being split into many 128 pages).
- LINEAR_GAMMA_SPACE: `gamma.c:177-205` and `gl_local.h:719-743` turn off the engine's texgamma, lightgamma and screen
  tables. It is for a custom renderer that does its own gamma; with the stock ref_gl every map would change brightness.
  Turn it on (`0x114`) only on a server whose clients all run the PrimeXT renderer.
