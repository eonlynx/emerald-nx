# Emerald NX â€” package

Copy this folder to the SD card as:

```
sdmc:/switch/emerald-nx/
```

Then launch `emerald-nx.nro` from hbmenu as a **title takeover**:
hold **R** while opening a game, then choose the NRO. Do not launch it from
the album/applet mode.

## Files

- `emerald-nx.nro` â€” low-address loader. It reads `emerald-game.nro`, maps it
  below 4 GiB, and starts it.
- `emerald-game.nro` â€” the real Emerald engine for Switch.
- `data/` â€” engine and game data, read through the `romfs:/` path mapping.
- `licences/` â€” the port's licence and the third-party notices.
- `loader.log` â€” written by the loader (created on first run).
- `emerald-nx.log` â€” stdout/stderr of the game.
- `port.log` â€” the engine's own log (created on first run).
- `settings.txt`, `*.sav` â€” created beside the NRO.

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

The `data/` folder is generated from the project's source assets. Do not
redistribute a package containing game data unless you have the rights to do
so. No ROM, save or original cartridge data is included by the build scripts;
the user supplies their own ROM when generating localized data.

## Troubleshooting

If the screen stays black, read `loader.log` first. It reports the low image
address, the heap range, and any `svcMapProcessCodeMemory` /
`svcSetProcessMemoryPermission` failure. The port must be launched as a title
takeover; emulators that do not provide the code-memory syscalls are not
supported by the loader.

