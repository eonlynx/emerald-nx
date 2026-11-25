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
- `loader.log` — written by the loader (created on first run).
- `emerald-nx.log` — stdout/stderr of the game.
- `port.log` — the engine's own log (created on first run).
- `settings.txt`, `*.sav` — created beside the NRO.

If you are updating from an older package that used the loose `data/` tree,
delete that folder before extracting this one; only the 15 engine files are
needed now.

## Controls

- A/B/X/Y: the Game Boy Advance A/B/X/Y buttons, by their printed letters.
- D-pad / left stick: movement.
- L/R: the GBA L/R buttons.
- ZL/ZR: fast-forward speed, or battle speed in battle (the same behavior as
  the Android port).
- Plus: Start. Minus: Select.
- Touch screen (handheld): the bottom screen's window; it is also shown on the
  top screen as in the Android single-screen port.

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

