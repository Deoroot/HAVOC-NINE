# Review notes: PrimeXT renderer and the remastered Hive

This branch has two commits on main 87170691:

1. **PrimeXT renderer for H9.** It sits behind `r_primext` and is off by default.
2. **`h9_tsrp_hive_rm`.** A remaster of Hive City that keeps v1's layout and entities.

Nothing changes for a player who leaves `r_primext` at 0. The other reTS clients and the server work as on main.

## How to try it

1. Build `menu.dll` with `xash.bat`. This branch carries `mainui-h9-primext.patch` but not a rebuilt `menu.dll`.
2. Install H9XASH from this branch.
3. Open Options → Video and tick "PrimeXT renderer (next start)", or put `r_primext 1` in `userconfig.cfg`.
4. Restart the game. The console prints `r_primext 1: PrimeXT's renderer draws the scene`.
5. Run `map h9_tsrp_hive_rm`. Use `maxplayers 32` for the RP plugins.
6. Try the "PrimeXT quality" selector in Video options (Low, Medium, High, Custom), or set `px_quality 0`, `1` or `2`.

## What is in it

- **Client.**
  - `cl_dlls/pxrender.dll` is loaded by `client.dll` at start when `r_primext` is 1.
  - It uses normal and gloss maps, deluxe lightmaps, cubemaps, dynamic lights, HDR and tonemapping, bloom (`px_bloom`) and puddles.
  - Water on brush entities and additive `!` textures are drawn as the engine draws them.
- **Settings.** All of them are listed in CVARS.md, in the "PrimeXT renderer" part.
  - Renamed so they no longer clash with the engine's cvars: `px_bloom`, `px_bloom_scale` and `px_shadows`.
  - One shared table, so the settings survive a start with PrimeXT off.
  - `px_quality` presets. Changing a single setting by hand switches to Custom.
  - `gl_hdr` is held at 1. Setting it to 0 makes PrimeXT draw a black screen.
  - MSAA is used only when `gl_msaa_samples` is 2 or more.
  - Debug cvars are no longer archived.
- **Server.**
  - `Server_GetPhysicsInterface` turns on deluxe data and large lightmaps.
  - Adds `env_dynlight`.
  - Metamod-R forwards the interface through `metamod-r/h9-physint.patch`, with a rebuilt `metamod.dll`.
- **`ts/autoexec.cfg`.** The `wait 2` and `exec userconfig.cfg` lines are gone. They pushed `config.cfg` behind the menu and dropped `+commands` from the command line. `config.cfg` already ends with `exec userconfig.cfg`.
- **Build and staging.** `build-win32-dropin.ps1` with `RETS_REBUILD=1` and the stage lists now ship `pxrender.dll`, `glsl/`, `gfx/` and `scripts/`.
- **The map.**
  - 22 places were retextured and dressed after repaints of in-game shots.
  - All 1717 v1 entities are unchanged, so doors, spawns and targetnames match v1. Its Apollo RP sql and inis, tsm spawns, mapconfig and waypoints are v1's, renamed.
  - `ARP_PropertyTax` treats it as the hive, through a prefix match.
  - The textures are CC0, credited in `CREDITS-h9_tsrp_hive_rm.txt` and in `THIRD_PARTY_LICENSES`.
  - Files go through Git LFS: the bsp, the wad, the models and `ts/textures/*.tga`.

## Licences: decisions for you

- **Data files.** Every PrimeXT data file of unknown origin has been replaced by our own work: the BRDF table, the normals-fitting table, the spotlight cookies and the text definitions. The generators are in `reTS/tools/primext-data`.
- **Code.** PRIMEXT-SOURCES.md and PRIMEXT-LICENCES.md list every file taken from PrimeXT, with its licence.
  - Most of the renderer is GPL-3.0+: Uncle Mike's code from Paranoia 2 and XashXT, plus SNMetamorph's additions.
  - Putting GPL code into a client under the HLSDK licence is your decision.
  - The owner kept seven small files that state no licence anywhere (mathlib, matrix, vector, stringlib and others).

## Tested, and not

**Tested in a Windows copy of H9 (Xash3D FWGS build 4223, RX 6600M):**
- The hive at 29 cameras, with PrimeXT on and with it off.
- The water views.
- ts_central, ts_awaken, h9_te_neuropolis, alexandria_msr_b5, rc_race01, smak_rp_byz02s and paradisecity, each loaded at spawn.
- Settings survive restarts.
- Clicks in the Video options page, with `menu.dll` built from mainui plus H9's patches.
- Apollo RP on the new map: 392 SQL statements, 0 errors.

**Not tested:**
- **Linux.** The PrimeXT server files and the patched Metamod compile with clang under Linux settings. Building `ts_i386.so` and Linux Metamod needs `build-linux.sh`.
- **The web client.** It gets no PrimeXT row.
- **The official `menu.dll` build.**
- **The newer plugins.** Only ARP was tested at main's version; the other plugins in the test copy were older.
- **v1's propclip and waypoints in play on the new map.**
- **Real multiplayer.**

## Known issues

- `r_lighting_brdf 1` turns the world almost black, so it stays off.
- Real MSAA (`gl_msaa_samples 4`) costs a lot: about 40–50 fps in the hive.
- The void's `!fog` clouds are dimmer than the engine draws them.
- In the void there are white blocks visible only from inside a sky brush. This comes from v1.
- `bloom` is subtle at `px_bloom_scale` 0.7.
- TS fog (`gl_fog`) is left as on main.
