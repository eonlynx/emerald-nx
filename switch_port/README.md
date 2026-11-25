# Emerald NX - Switch port work area

This directory is the Nintendo Switch port of Emerald 3DS. It is built with
devkitA64/libnx and uses the same game tree as the 3DS/Android ports.

## Current stage

The port now builds the complete two-stage game, not only the loader spike:

- `loader/` builds `emerald-nx.nro`, a small two-stage loader.
- The game NRO (`emerald-game.nro`) is the real Emerald engine, built from
  `3ds_port/full.mk` with the Switch backend in `src/` and
  `android_port/src/` (EGL/GLES3 video, audout audio, libnx input,
  `sdmc:/switch/emerald-nx/` filesystem wrappers).
- `romfs/` is staged as `data/` beside the two NROs; no protected data is kept
  in the repository.

The loader maps the game NRO below 4 GiB with
`svcMapProcessCodeMemory`, gives it a low heap/stack through the homebrew ABI,
and jumps to its entrypoint. The game image, its C heap, its thread stacks and
the newlib heap all stay below 4 GiB so the engine's 32-bit address fields keep
working.

The game services `appletMainLoop()` every frame: HOME/exit requests stop the
loop cleanly, focus loss (HOME, a library applet or console sleep) pauses the
audout feeder, and returning resets the frame clocks. The video backend checks
`appletGetOperationMode()` every frame, so handheld 1280x720 and docked
1920x1080 recreate the EGL surface without restarting the game.

Hardware testing is still the last step: the build and the static layout
audits complete on the host, but no console has run the final package yet.

## Build

Bootstrap the pinned game tree once, then run the complete build:

```sh
export DEVKITPRO=/opt/devkitpro
python tools/bootstrap.py
bash switch_port/build_switch.sh
```

Requirements: devkitPro with devkitA64, devkitARM, libnx, switch-tools,
switch-mesa and switch-libdrm_nouveau; Python 3.13 with Pillow; `make`, `zip`
and `llvm-dwarfdump` (MSYS2 LLVM) for the optional audits.

Outputs:

```
switch_port/dist/switch/emerald-nx/
    emerald-nx.nro      the low-address loader
    emerald-game.nro    the real Emerald engine
    data/               engine and game data
    README.txt
    licences/
switch_port/dist/emerald-nx.zip
```

`build_switch.sh` preserves the generated `romfs/` data from an earlier
3DS/Android build and rebuilds only the platform objects, bundles and the
Switch executables. To rebuild from an empty object directory:

```sh
CLEAN_BUILD=1 bash switch_port/build_switch.sh
```

`JOBS=N`, `PYTHON=...` and `TREE=...` override the defaults; inside
devkitPro's MSYS2 shell, `PYTHON` can be the Windows Python path. The package
is generated output and must not be committed.

Static layout audits (run from the bootstrapped tree's `3ds_port/`):

```sh
make -f ../../switch_port/switch.mk check-save-layout
make -f ../../switch_port/switch.mk check-overlays
```

The save check compares devkitARM's 32-bit layout with devkitA64's for
`SaveBlock1`, `SaveBlock2` and `PokemonStorage`; the overlay check reports
structures laid over GBA-sized storage. The reviewed Switch exceptions are in
`tools/check_overlays.allow`.

## Run the package on hardware

1. Unzip `switch_port/dist/emerald-nx.zip` at the SD card's root, so the
   folder is `sdmc:/switch/emerald-nx/`.
2. On the Switch, open hbmenu and launch `emerald-nx.nro` as a **title
   takeover**: hold **R** while opening a game, then choose the NRO. Do not
   launch it from the album/applet mode; the loader needs the code-memory
   syscalls and a process handle.
3. The game writes:
   - `sdmc:/switch/emerald-nx/loader.log`
   - `sdmc:/switch/emerald-nx/emerald-nx.log`
   - `sdmc:/switch/emerald-nx/port.log`
   - saves and `settings.txt` in the same folder.

If the screen stays black, read `loader.log` first: it reports the low image
address, the heap range, and any `svcMapProcessCodeMemory` /
`svcSetProcessMemoryPermission` failure. The loader also refuses to start when
the launcher did not hint those syscalls.

## Why a two-stage loader

Emerald keeps addresses in 32-bit fields. hbloader maps an NRO with
`virtmemFindCodeMemory`, which can return an address above 4 GiB. The loader
forces the game image and its heap/stack into the low range
(128 MiB-2 GiB), so those 32-bit addresses remain valid without a full 64-bit
engine migration.

This is the same mechanism used by toniisound's `smbr-nx` loader and by
`nx-hbloader`; see `SWITCH_PORT_PLAN.md` for the full analysis. The loader
exits the process when the game returns instead of handing the hbloader heap
back with code mappings still attached; Dusklight's Switch port showed the
`InvalidCurrentMemory` failure that cleanup avoids.

## Reference reuse checklist (mandatory)

Before closing any subsystem, compare it with the local reference ports and
record the result:

- `toniisound`: `The-Oracles-Project-NX`, `oracles-nx`, `smbr-nx`,
  `DKC1/2/3Recomp-NX`.
- `Alexgg1014`: `The-Legend-of-Zelda-Twilight-Princess-Alek-s-Ultimate-NX-Edition`.

Checklist:

- [x] Loader: `smbr-nx/source/so_util.c`, `nx-hbloader/source/main.c`, TP
      `switch_elf_memory.cpp`/`code_arena.c`; exit cleanup. Adopted the page
      permission order and the syscall-hint check; kept the low-address gap
      search and process exit that Emerald needs.
- [x] Build/NRO: Oracles `CMakeLists.txt`, `build_switch.sh`,
      `nx_create_nro`, `Switch.cmake` flags; `.nacp`/icon. Kept the Makefile
      for the large game tree and copied the release layout.
- [x] Video: Oracles `SDL_WINDOW_OPENGL`/`opengles2`, smbr `egl_setup`,
      DKC presenter; 720p/1080p and dock changes. Used direct NWindow/EGL
      (closest to `android_video.c`), with 1280x720 / 1920x1080 mode changes.
- [x] Audio: Oracles/oracles-nx SDL3 audio and
      `sdl-switch-audio.patch`; smbr audout feeder; underrun/drift handling.
      Kept the Android ring/resampler and used libnx audout directly; no SDL3
      dependency.
- [x] Input: Oracles `sdl_window.c`, oracles-nx `app.c`/`controls.c`, DKC
      `switch_gamepad.c`; letter mapping, ZL/ZR, touch, HOME exclusion. Used
      libnx pad/touch with letter-based A/B/X/Y and panel-only touch.
- [x] Filesystem: Oracles `sdmc:/switch/...`, oracles-nx ROM installer, TP
      startup/FS layer; `data/` external, saves/logs. Kept path wrappers
      because Emerald hardcodes 3DS paths.
- [x] Lifecycle: Oracles 8 MiB thread/log, TP `AppletType`, `userAppInit`/
      `userAppExit`, code-memory cleanup. The game runs from the loader's
      config entries and exits the process through `switch_after_game`.
- [x] If the reference is more complete, reuse/adapt it; if ours is better for
      Emerald's 32-bit model, keep ours and note the edge cases adopted.

## Layout

- `loader/` - low-address NRO loader.
- `src/` - Switch host layer (video/audio/input/FS/lifecycle).
- `game/` - the Emerald game NRO built from `3ds_port/full.mk`.
- `data/` - generated game data, never committed.
- `patches/` - port and platform patches.
- `build/`, `dist/` - generated outputs, ignored by git.

No ROMs, saves, extracted assets or other protected content belong in this
repository.
