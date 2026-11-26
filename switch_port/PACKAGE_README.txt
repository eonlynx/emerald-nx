# Emerald NX — package

Copy this folder to the SD card as:

```
sdmc:/switch/emerald-nx/
```

Then launch `emerald-nx.nro` from hbmenu as a **title takeover**:
hold **R** while opening a game, then choose the NRO. Do not launch it from
the album/applet mode.

## Files

- `emerald-nx.nro` — low-address loader. It reads `emerald-game.nro`, maps it
  below 4 GiB, and starts it.
- `emerald-game.nro` — the real Emerald engine for Switch.
- `emerald3ds.pak` — all game data in one pack: graphics, maps, scripts,
  songs, tables and voxel data. It is read in place; do not extract it.
- `data/` — the small engine files the engine opens directly (boot marker,
  ABI/build id, asset index, relocation tables and port art).
- `licences/` — the port's licence and the third-party notices.
- `loader.log` — the loader's report, recreated on every launch.
- `emerald-nx.log` — the game's log, recreated on every launch; the previous
  run is kept as `emerald-nx-prev.log` and each run stops at 1 MiB.
- `boot.log` — early-startup breadcrumbs, recreated on every launch.
- `settings.txt`, `*.sav` — created beside the NRO.

The normal log is small: startup steps, errors and one progress line per
minute. For a detailed report, create an empty file named `debug.txt` in this
folder and reproduce the problem; the log then includes the verbose
diagnostics, still capped at 1 MiB. Delete `debug.txt` when finished.

If you are updating from an older package that used the loose `data/` tree,
delete that folder before extracting this one; only the 15 engine files are
needed now.

## Controls

- A/B/X: the Game Boy Advance A/B/X buttons, by their printed letters.
- Y: switch VOXEL 3D on or off (3D/2D toggle), anywhere.
- D-pad / left stick: movement.
- L/R: the GBA L/R buttons.
- ZL/ZR: hold either for the MAX SPEED (4x by default); release returns to 1x.
  Both together lock the MAX SPEED, or return to 1x.
- Left stick click: SHOW FPS on or off; its notice stays over the counter for
  a few seconds.
- Right stick click: DAY / NIGHT between AUTO and OFF; its notice stays over
  the counter for a few seconds.
- Plus: Start. Minus: Select (the registered item; in a battle action menu,
  the quick ball).
- X: open the bottom-screen panel, also during conversations; in battle it
  shows the bottom battle screen (again to close, or B from its main menu).
- Touch screen (handheld): the bottom screen's window; it is also shown on the
  top screen as in the Android single-screen port.

DAY / NIGHT starts in AUTO. In the bottom panel, UP enters the category on
show at its last item, and DOWN from that last item returns to the category;
the NUZLOCKE tracker also takes the D-pad and A.

In GENERAL > SYSTEM, SHOW SPEED (OFF/AUTO/ON) puts the current speed under the
FPS counter, and HOLD SPEED (OFF/1.5x/2x/3x/4x) chooses the MAX SPEED.
CLOSE AFTER SAVE (OFF/ON) asks whether to close the game after saving and
return to the launcher.
BATTLE > SCENE has 3D BATTLE (OFF/AUTO/ON; AUTO follows VOXEL 3D) and
BATTLE MENU (CLASSIC/NEW; NEW uses the bottom screen's battle menu on your
turn).

Fast-forward keeps the full 2D detail at every speed: the picture does not
drop scanlines or change resolution.

## Data / legal

`emerald3ds.pak` and `data/` are generated from the project's source assets. Do not
redistribute a package containing game data unless you have the rights to do
so. No ROM, save or original cartridge data is included by the build scripts;
the user supplies their own ROM when generating localized data.

## Troubleshooting

If the screen stays black, read `loader.log` first. It reports the low image
address, the heap range, and any `svcMapProcessCodeMemory` /
`svcSetProcessMemoryPermission` failure. The port must be launched as a title
takeover; emulators that do not provide the code-memory syscalls are not
supported by the loader.

