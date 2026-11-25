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

## env_postfx_controller

A point entity that is never sent. When it is fired it sends the user message **`PostFxSettings`** (registered last
in `LinkUserMessages`, so no older message changes number; fixed size 52 bytes): 13 floats, in this order, as PrimeXT's
`CPostFxParameters::ParseMessage` reads them: fade time, brightness, saturation, contrast, red, green, blue level,
vignette, film grain, accent scale, accent colour r, g, b (0..1, from `rendercolor` / 255). pxrender.dll hooks the
message in its `HUD_Init` (`bridge/pxr_main.cpp`); the renderer then fades from the look it has to the new one over the
fade time (`postfx_controller.cpp`) and draws it in `RenderPostprocessing` while `r_postfx_enable` is 1 (its default).
Each new map starts neutral (`PXR_VidInit` resets it; PrimeXT itself does not).

Map keys (PrimeXT's fgd): `fade_in_time` (s), `brightness` (-1..1), `saturation`, `contrast`, `red_level`,
`green_level`, `blue_level`, `vignette_scale`, `film_grain_scale`, `color_accent_scale` (0..1), `rendercolor` (the
accent colour), `targetname`, `target` (fired after it). Spawnflags: 1 **Fade From** (sends the neutral look: back to
normal over `fade_in_time`), 2 **Activator Only** (`MSG_ONE` to the player who fired it; otherwise `MSG_ALL`).

- **Per place**: a `trigger_multiple` over the place fires an Activator Only controller; a ring of triggers round it
  (or the rooms beside it) fires one with Fade From | Activator Only. That is how `h9_pxs_test` does it.
- Differences from PrimeXT: a key left out takes the fgd's default (neutral, 3 s fade), not 0 (in PrimeXT a missing
  `saturation` is black and white). Activator Only fired by no player (a timer, `h9_pxt_use`) sends nothing.
- Not carried: the look of a player who joins after a `MSG_ALL` controller fired (PrimeXT neither), and across a
  save/restore on the client (the server keeps the values, the client does not ask again).
- A stock client (`r_primext 0`, or no pxrender.dll) has no hook: the engine drops the message with a developer-only
  line (`UserMsg: No pfn PostFxSettings`). Nothing else changes for it.

## func_monitor, func_portal, info_portal_destination

The renderer finds the faces to draw a view into **by their texture name** (`gl_world_new.cpp`): `monitor*` faces are
screens, `portal*` faces are portals, `mirror*`, `reflect*`, `!reflect*` are mirrors. On those faces of a brush entity
it looks through the entity whose number is in `curstate.sequence` (`gl_subview.cpp` `R_RenderSubview`), with the field
of view in `fuser2` and `CF_MONOCHROME` in `iuser1`. H9's delta.lst has no `fuser2` or `iuser1`, its `sequence` is 8 bits
(the hive has 1984 entities) and its `effects` 8 bits (PrimeXT's `EF_SCREEN`/`EF_PORTAL` are bits 11 and 10). So:

| Meaning | `curstate` field | Wire | Server sets |
|---|---|---|---|
| marker | `renderfx == 73` (`H9_RENDERFX_SUBVIEW`) | 8 bit | always, on func_monitor and func_portal (`renderamt` 255 with it) |
| camera / exit entity | `aiment` | 11 bit | the camera's edict while on, 0 while off |
| field of view | `controller[1]` | 8 bit | key `fov`, 10..120, 0 = 90 |
| switches | `controller[0]` | 8 bit | bit0 0x01 black and white (func_monitor spawnflag 16) |

pxrender.dll's `PXR_AddEntity` copies them for a brush entity with renderfx 73 (`sequence = aiment`, `fuser2 =
controller[1]`, `iuser1 = CF_MONOCHROME`), where PrimeXT's renderer reads them. PrimeXT also sends the camera in
`aiment`; that is what makes the engine merge the camera's view into the client's packet: `EF_MERGE_VISIBILITY`
(1<<29) on the camera, server side only (`sv_frame.c` `SV_AddEntitiesToPacket`, H9's engine the same), so the room the
camera looks at is sent although the player cannot see it. As in PrimeXT, the bit is on only while some client has the
screen in its PVS (a 0.1 s think). Two things differ from PrimeXT's code, both found in the hive (the screens showed a
camera only once the player had been near it): a func_monitor is MOVETYPE_PUSH, whose think runs on its own clock
(`pev->ltime`), so the think is scheduled on that clock (PrimeXT's `SetNextThink` does it; a plain `gpGlobals->time`
ran it seconds apart and the bit was mostly off); and each connected client's own PVS is asked (`ENGINE_SET_PVS` +
`ENGINE_CHECK_VISIBILITY`) instead of `FIND_CLIENT_IN_PVS`, which passes over clients with no health (spectators).

- The camera must be **sent**: reTS's `info_target` has no model, so the monitor gives the camera it names the blank
  `sprites/null.spr` (PrimeXT's info_target needs its spawnflag 1 for that). `info_portal_destination` has it always.
  The camera looks along its `angles` (pitch, yaw, roll); which sign of pitch looks down is not checked in the game yet.
- The engine marks a client entity whose `aiment` is set `MOVETYPE_FOLLOW` (`cl_frame.c:1241`); for a brush model
  nothing reads that (ref_gl and the renderer follow only sprites and studio models), and `CL_ProcessEntityUpdate`
  only moves a FOLLOW entity whose origin is 0 0 0 and whose *sent* movetype is FOLLOW, which a monitor's is not (PUSH).

func_monitor map keys: `camera` (targetname of the camera, an `info_target` or any entity), `fov`, `targetname`,
`master`; spawnflags 1 Start ON, 2 Passable (`SOLID_NOT`), 16 Black/White. Fire it to toggle (USE_ON/OFF/TOGGLE).
Off = no camera: the faces show their own texture (and do on a stock client). Not carried: 4 Usable (PrimeXT puts a
player who uses it behind the camera), the `*player` camera, 8 Square Image (PrimeXT reads it nowhere), parents.

func_portal map keys: `target` (an `info_portal_destination`), `firetarget` (fired at what passes), `master`, and it
**needs an origin brush** (its frame: origin and `angles`; without one it is removed with an error, as PrimeXT).
Spawnflag 1 Start Off (`EF_NODRAW`). Touch: a player or a monster that moves into its forward side (`angles`) is put at
the same place relative to the exit's frame, with its look and velocity turned the same way, unless the exit has no
room (it then stops); the portal rests 0.5 s after each pass. PrimeXT also takes pushables, projectiles and rigid bodies.

**Mapping notes (seen in the game, h9_pxs_test):**
- The screen shader lays the camera's picture along the face's **texture coordinates, 0..1 across the picture**
  (`scene_bmodel_fp.glsl`, MONITOR_BRUSH): fit the texture to the face exactly, one picture across it, or the picture
  repeats or is cut and rolled (h9_pxs_test's first screen showed a seam across its middle).
- The picture is a render target, its **row 0 at the bottom**: v has to run up the face (a negative y scale on a wall,
  offset so the foot is 0), or the camera's view is upside down; draw the screen's own texture (what shows when off)
  upside down to match. u as usual: a world-aligned texture on a west (-x) or north (+y) face runs right to left as
  seen, and the picture comes out mirrored (flip u there). The hive's CCTV bank (`hive-remaster/pxents`) does this.
- The engine **turns a brush entity with an origin brush by its `angles`**, and a func_portal's forward is its local
  +x. So a portal is built as if its angles were 0 (thin along x, its face to the room on its -x side) and turned by
  `angles`: yaw 270 puts it on a south wall, facing the room (+y), entered moving -y.

**Mirrors** need no entity: a face textured `mirror*` / `reflect*` / `!reflect*` (world or brush entity) reflects in
pxrender.dll (`gl_allow_mirrors` 1, the default). PrimeXT has no `func_mirror`. A stock client draws the texture.

Stock client: a func_monitor or func_portal is a brush entity with renderfx 73 and `renderamt` 255, which ref_gl draws
as a normal solid brush (its texture); the camera is a blank sprite. aiment is ignored for a brush model.

## Test aids (server console)

- `h9_pxt_lights`: for every env_dynlight, runs the real `AddToFullPack` and prints the state it sends and what the
  delta quantisation leaves of `framerate` and `scale`.
- `h9_pxt_screens`: every func_monitor / func_portal, whether a client sees it, its camera, the camera's merge bit and
  whether AddToFullPack would send the camera.
- `h9_pxt_touch [<target prefix>] [fire]`: player 1's box against every trigger whose target starts with the prefix;
  with `fire`, each trigger the player stands in is touched by him (its own Touch), for a player who cannot walk.
- `h9_pxt_use <targetname> [on|off|toggle] [<player index>]`: with a player index that player is the activator, so an
  Activator Only env_postfx_controller can be fired from the console (`h9_pxt_use pfx_in toggle 1`).
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

## Test map: h9_pxs_test

`reTS/tools/primext-test-map/make-test.mjs` (needs the SMAK kit, our own textures in `h9_pxs_test.wad`, compiled `-engine xash
-format bsp2` with the textures inside): a hall with a func_monitor on its east wall (camera `cam1` in a sealed room B
with a red block, a yellow pillar and a func_wall that only the merged visibility brings), a mirror slab on the north
wall, a func_portal on the south wall (exit `pdest` in room B), and two trigger_multiples: the east half fires `pfx_in`
(Activator Only: saturation 0.25, contrast 1.5, vignette 0.8, grain 0.35, red accent), the west half `pfx_out` (Fade
From). `pfx_all` (everyone, black and white) is for the console.

Seen in the game (2026-11-25, h9run-srv2, r_primext 1, shots in `C:/tmp/primext/srv2/shots5/`): the monitor shows
room B through `cam1` the right way round, and room B's func_wall is sent (merged visibility; it draws black there,
a lighting question of the renderer, not of these entities); off, it shows `monitor1`. The mirror slab reflects the
hall. The portal shows room B from `pdest`, and walking into it puts the player in room B facing +y. `pfx_in` (fired
with the player as activator) gives the dark, desaturated, grainy, vignetted look with the red kept; `pfx_out` brings
it back; `pfx_all` greys everyone; a Fade From message on a neutral screen changes nothing. Not seen: the triggers
firing them (the test player stays a spectator: H9's match mode keeps a live player behind a grey screen until the
side menu is answered), the black and white monitor, and a stock client.

## The hive remaster as a showcase (h9_tsrp_hive_pxents, 2026-11-25)

Built by `C:/tmp/primext/hive-remaster/pxents/build-pxents.mjs` from the final map (compile as px3b). Security office
c22: a bank of four func_monitor screens right of the door, cameras at c16 (courtyard), c15 (plaza), c13 (cocktail
bar), c18 (sea, black and white); off they show `monitorcctv` (scanlines, a frame). Looks: c17 Midgar warm, c26 the XE
hall cold and grey (its band before the teleport doors fades back), c05 green with light grain, c18 dusk warmth. Each
place's triggers are its walkable air, flooded with the player hull from a seed (`pxents/flood.mjs`). Seen in the game:
every screen showing its camera the right way up from the first frame (spectator), off/on, and each look fired through
its own trigger by a live player (`h9_pxt_touch ... fire`; the test player could not walk, so the engine's own touch on
walking in is not seen). Shots: `C:/tmp/primext/srv2/shots-dbg2/`, `shots-px2/`, `shots-fire/`.
