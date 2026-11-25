# Emerald NX — Switch port work area

This directory is the Nintendo Switch port of Emerald 3DS. It is built with
devkitA64/libnx and uses the same game tree as the 3DS/Android ports.

## Current stage

The first milestone is a **loader spike**, not the full game:

- `loader/` builds `emerald-nx.nro`, a small two-stage loader.
- `spike/` builds `emerald-game.nro`, an address-probe program.

The loader maps the game NRO below 4 GiB with
`svcMapProcessCodeMemory`, gives it a low heap/stack through the homebrew ABI,
and jumps to its entrypoint. The probe prints code, data, heap, stack and
32-bit function-pointer addresses and reports PASS/FAIL.

The full Emerald game backend is not wired yet. Do not expect gameplay from
this build.

## Build

Use devkitPro's MSYS2 shell (or any shell with `DEVKITPRO` set):

```sh
export DEVKITPRO=/opt/devkitpro
make -C switch_port/loader -j4
make -C switch_port/spike  -j4
```

Outputs:

- `switch_port/loader/emerald-nx.nro`
- `switch_port/spike/emerald-game.nro`

A ready-to-copy layout is produced under
`switch_port/spike/dist/switch/emerald-nx/`:

```
emerald-nx.nro
emerald-game.nro
README.txt
```

## Run the spike on hardware

1. Copy the contents of `switch_port/spike/dist/switch/emerald-nx/` to
   `sdmc:/switch/emerald-nx/` on the SD card.
2. On the Switch, open hbmenu and launch `emerald-nx.nro` as a **title
   takeover**: hold **R** while opening a game, then choose the NRO. Do not
   launch it from the album/applet mode.
3. The probe waits 10 seconds. It writes:
   - `sdmc:/switch/emerald-nx/loader.log`
   - `sdmc:/switch/emerald-nx/game.log`

Expected log lines:

```
game: RESULT PASS
game: range=PASS (<4 GiB)
game: 32-bit probe=... call=PASS
```

If the loader fails, `loader.log` says which syscall or range failed.

## Why a two-stage loader

Emerald keeps addresses in 32-bit fields. hbloader maps an NRO with
`virtmemFindCodeMemory`, which can return an address above 4 GiB. The loader
forces the game image and its heap/stack into the low range
(128 MiB–2 GiB), so those 32-bit addresses remain valid without a full 64-bit
engine migration.

This is the same mechanism used by toniisound's `smbr-nx` loader and by
`nx-hbloader`; see `../SWITCH_PORT_PLAN.md` for the full analysis.

## Layout of the full port (next milestones)

- `loader/` — low-address NRO loader.
- `src/` — Switch host layer (video/audio/input/FS/lifecycle).
- `game/` — the Emerald game NRO built from `3ds_port/full.mk`.
- `data/` — generated game data, never committed.
- `patches/` — SDL/audio patches inherited from toniisound's ports.
- `build/`, `dist/` — generated outputs, ignored by git.

No ROMs, saves, extracted assets or other protected content belong in this
repository.
