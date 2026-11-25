// h9_pxs_test: a test map for PrimeXT's env_postfx_controller, func_monitor, func_portal and a mirror texture.
// Hall A (the player's) and room B (sealed off: what the camera and the portal see) share no visibility.
// node make-test.mjs <outdir>
import { writeFileSync, mkdirSync, readFileSync } from "node:fs";
import { join } from "node:path";
const KIT = "file:///C:/Users/User/Documents/Visual%20Studio%20Projects/smak-wt/main/";
const { carve, box, fitted, light, playerStart, writeMap, brushEntity, pointEntity, originBrush } = await import(KIT + "make/maps/source.mjs");
const { checkPlan } = await import(KIT + "check/maps/plan.mjs");
const { findCompilers, compileMap, compileSummary } = await import(KIT + "make/maps/compile.mjs");
const { writeWad } = await import(KIT + "core/maps/wad.mjs");

const out = process.argv[2] ?? "out";
mkdirSync(out, { recursive: true });

// ── textures: our own, drawn here ──
const tex = (name, size, fn) => {
  const [w, h] = size;
  const rgb = new Uint8Array(w * h * 3);
  for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) rgb.set(fn(x, y, w, h), (y * w + x) * 3);
  return { name, image: { width: w, height: h, rgb } };
};
const textures = [
  tex("pxwall", [64, 64], (x, y) => ((y % 32 < 2) || ((x + (y >= 32 ? 32 : 0)) % 64 < 2) ? [90, 90, 90] : [170, 160, 140])),
  tex("pxfloor", [64, 64], (x, y) => ((x < 32) !== (y < 32) ? [60, 70, 90] : [200, 200, 190])),
  tex("pxceil", [64, 64], () => [210, 210, 210]),
  tex("pxred", [64, 64], (x, y) => (x % 16 < 2 || y % 16 < 2 ? [120, 0, 0] : [230, 40, 30])),
  tex("pxblue", [64, 64], (x, y) => (x % 16 < 2 || y % 16 < 2 ? [0, 0, 120] : [40, 80, 240])),
  tex("pxyellow", [64, 64], (x, y) => ((x + y) % 32 < 16 ? [240, 220, 30] : [30, 30, 30])),
  // a func_monitor's screen: what shows when the monitor is off (and on a stock client)
  tex("monitor1", [128, 96], (x, y) => (y % 4 < 2 ? [20, 40, 30] : [10, 20, 15])),
  tex("mirror1", [64, 64], () => [180, 190, 200]),
  tex("portal1", [64, 128], () => [80, 20, 120]),
  tex("aaatrigger", [16, 16], () => [200, 100, 0]),
  tex("origin", [16, 16], () => [255, 128, 0]),
];
const wadFile = join(out, "h9_pxs_test.wad");
writeFileSync(wadFile, writeWad(textures));

// ── the rooms ──
const ROOMS = { floor: "pxfloor", wall: "pxwall", ceiling: "pxceil" };
const HALL = { min: [0, 0, 0], max: [512, 384, 192] };
const ROOMB = { min: [0, 640, 0], max: [384, 960, 192], texture: { floor: "pxfloor", wall: "pxblue", ceiling: "pxceil" } };
const world = carve([HALL, ROOMB], 16, ROOMS);

// the mirror: a slab on the hall's north wall (y 384), its face to the room wears mirror1
const mirror = box([224, 376, 16], [352, 384, 144], { sides: "pxwall", top: "pxwall", bottom: "pxwall", south: "mirror1" });
// room B's furniture: a red block and a yellow pillar, so the camera's picture is told apart from the hall
const redBlock = box([200, 760, 0], [280, 840, 80], "pxred");
const pillar = box([120, 880, 0], [160, 920, 160], "pxyellow");

// ── entities ──
const entities = [
  playerStart([64, 192, 0], 0, "info_player_deathmatch"),
  playerStart([64, 96, 0], 0, "info_player_deathmatch"),
  light([128, 192, 176], { brightness: 300 }),
  light([400, 192, 176], { brightness: 300 }),
  light([192, 800, 176], { color: [255, 200, 160], brightness: 350 }),

  // the monitor on the hall's east wall (x 512), its screen facing west, looking through cam1 in room B
  // the screen shader lays the camera's picture along the face's texture axes: on a west face the world grid runs
  // right to left as seen (the picture came out mirrored), so u is flipped (scale -1)
  brushEntity("func_monitor", [box([500, 128, 48], [512, 256, 144], { sides: "pxwall", top: "pxwall", bottom: "pxwall", west: { name: "monitor1", scale: [-1, 1] } })],
    { camera: "cam1", fov: 90, spawnflags: 1, targetname: "mon1" }),
  pointEntity("info_target", [24, 800, 96], { targetname: "cam1", angles: [0, 0, 0] }),
  // something only the merged visibility brings: a func_wall in room B (an entity, not world)
  brushEntity("func_wall", [box([300, 680, 0], [360, 740, 120], "pxyellow")]),

  // the portal on the hall's south wall (y 0..8), its face to the hall. The engine turns a brush entity with an origin
  // brush by its angles, and the portal's forward is its local +x: so it is drawn as at angles 0 (thin along x, its
  // face to the room on its -x side) and turned by yaw 270 (local +x -> world -y, into the wall; -x -> +y, the hall)
  brushEntity("func_portal", [box([140, -44, 0], [148, 52, 112], fitted("portal1", [64, 128], [140, -44, 0], [148, 52, 112], { whole: true, faces: ["west"], rest: "pxwall" })), originBrush([144, 4, 56], 8)],
    { target: "pdest", angles: [0, 270, 0], targetname: "portal1" }),
  pointEntity("info_portal_destination", [192, 700, 56], { targetname: "pdest", angles: [0, 90, 0] }),

  // the place with its own look: the hall's east half. In = activator-only controller; the west half fades back
  brushEntity("trigger_multiple", [box([288, 16, 0], [496, 368, 128], "aaatrigger")], { target: "pfx_in", wait: 1 }),
  brushEntity("trigger_multiple", [box([16, 16, 0], [240, 368, 128], "aaatrigger")], { target: "pfx_out", wait: 1 }),
  pointEntity("env_postfx_controller", [400, 192, 160], {
    targetname: "pfx_in", spawnflags: 2, fade_in_time: 1.5, brightness: -0.05, saturation: 0.25, contrast: 1.5,
    red_level: 1, green_level: 0.85, blue_level: 0.7, vignette_scale: 0.8, film_grain_scale: 0.35,
    color_accent_scale: 0.6, rendercolor: [255, 40, 40],
  }),
  pointEntity("env_postfx_controller", [128, 192, 160], { targetname: "pfx_out", spawnflags: 3, fade_in_time: 1.5 }),
  // the same look for everyone, for the console (h9_pxt_use pfx_all)
  pointEntity("env_postfx_controller", [256, 160, 160], { targetname: "pfx_reset", spawnflags: 1, fade_in_time: 0.2 }),
  pointEntity("env_postfx_controller", [256, 192, 160], { targetname: "pfx_all", fade_in_time: 0.5, saturation: 0, contrast: 1.2, vignette_scale: 1 }),
];
const map = { worldspawn: { message: "SMAK test: PrimeXT entities", wad: [wadFile] }, brushes: [...world, mirror, redBlock, pillar], entities };
const mapFile = join(out, "h9_pxs_test.map");
writeFileSync(mapFile, writeMap(map));
const plan = checkPlan(map, { textures: textures.map((t) => t.name) });
plan.facts.forEach((f) => console.log("  " + f));
plan.warnings.forEach((w) => console.log("  WARN " + w));
plan.errors.forEach((e) => console.log("  ERROR " + e));
if (plan.errors.length) process.exit(1);

const compilers = findCompilers({ folder: "C:/tmp/primext/sdhlt-cubemap/tools-out" });
const result = compileMap(mapFile, { wads: [wadFile], compilers, wadinclude: true, engine: "xash", format: "bsp2" });
console.log(compileSummary(result));
if (!result.bsp) process.exit(1);
console.log("bsp", result.bsp);
