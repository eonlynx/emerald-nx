<div align="center">
  <img src="switch_port/icon.jpg" alt="Emerald NX" width="160">

  <h1>Emerald NX</h1>

  <p>
    <strong>Hoenn, natively on Nintendo Switch.</strong><br>
    A homebrew <code>.nro</code> port of <a href="https://github.com/ZallaxDev/emerald-3ds">Emerald 3DS</a>,
    built on the same native engine as the 3DS and Android versions.
  </p>

  <p>
    <a href="#install">Install</a> ·
    <a href="#features">Features</a> ·
    <a href="#controls">Controls</a> ·
    <a href="#building-from-source">Building</a> ·
    <a href="#status">Status</a> ·
    <a href="#credits">Credits</a>
  </p>
</div>

> [!IMPORTANT]
> **No ROMs or saves are included.** Emerald NX does not contain Pokémon Emerald,
> a ROM, a save file or extracted game data in this repository. You need your own
> legally obtained copy of the game for any data generated from it. This is an
> unofficial fan project, not affiliated with or endorsed by Nintendo, Game Freak
> or Creatures.

> [!WARNING]
> **The first hardware test is still pending.** The Switch build, package and
> static audits are complete, but no console has run the final package yet.
> Back up your SD card and your saves before testing, and expect issues on the
> first boot.

> [!NOTE]
> **This repository remains multiplatform.** The 3DS and Android ports are still
> built from the same tree. Emerald NX is the main development focus; the
> original platform documentation is kept in
> [README-emerald-3ds.md](README-emerald-3ds.md) and
> [README-emerald-android.md](README-emerald-android.md).

---

## What it is

Emerald NX runs the Pokémon Emerald decompilation **natively on Nintendo
Switch** — no emulator. It is a port of the Emerald 3DS engine to Horizon:
the same game bridge, touch interface, voxel overworld and enhancements, with
a Switch backend for video, audio, input, filesystem and lifecycle.

A small loader (`emerald-nx.nro`) maps the real game NRO below 4 GiB and starts
it. The engine keeps addresses in 32-bit fields, so the game image, heap,
stack and newlib heap all stay in the low address range; the code itself runs
as native AArch64.

The game is presented on the Switch's single screen: the 432×240 top picture
follows the display's aspect, and the bottom-screen interface is drawn as a
panel over it, as in the Android single-screen version. Handheld mode renders
at 1280×720, docked at 1920×1080.

## Features

| | What to expect |
| :--- | :--- |
| **Native Switch homebrew** | A libnx `.nro` running the real engine on the console's ARM CPU, with EGL/GLES3 video and libnx audout audio. No emulator. |
| **Single-screen layout** | The game on top, with the bottom screen's touch interface as a panel over it. Docked and handheld modes are handled without restarting. |
| **Low-address loader** | A two-stage NRO loader reserves the image, heap and stack below 4 GiB with `svcMapProcessCodeMemory`, then enters the game. |
| **Single-file game data** | `emerald3ds.pak` holds graphics, maps, scripts, songs, tables and voxel data in one file, so the SD card copy is one `data/` folder of 15 engine files plus the pack, not 6,400 loose files. |
| **Full resolution** | 1280×720 handheld / 1920×1080 docked; frame pacing uses the same 59.8261 Hz clock as the 3DS and Android ports. |
| **HOME, sleep and dock** | `appletMainLoop`/`appletHook` handle HOME and exit, focus loss pauses audio, and the EGL surface is recreated when the console changes mode. |
| **Optional voxel overworld** | Modelled buildings, trees, signposts and terrain relief from Emerald 3DS, rendered with the same voxel renderer through GLES3. |
| **HD battle interface** | Wide-screen battle boxes, the 3D battle view and the bottom-screen interface. |
| **Follower Pokémon** | Your lead Pokémon walks behind you with its HeartGold/SoulSilver sprite and messages, as in Emerald 3DS. |
| **Language builds** | English, Spanish, French, Italian, German and experimental Japanese game data, bootstrapped from a clean cartridge of that language. The prebuilt ZIP uses the language of the tree it was built from (English by default). |
| **Day/night and enhancements** | Optional outdoor lighting, lit windows, a faster clock, physical/special split, IVs/EVs, infinite TMs, running indoors, Repel prompt, visible wild Pokémon and more. |
| **QoL options** | BATTLE SPEED (AUTO or explicit), progressive/classic map markers and the existing speed controls. These change no story or events. |
| **3DS and Android still build** | The shared engine and ports remain in the repository; the Switch backend is additive. |

### A new perspective on Hoenn

The voxel overworld is **off by default**. Open **OPTION** and enable
**VOXEL 3D**, then adjust **3D ANGLE** and **3D ZOOM**. As in Emerald 3DS,
many towns and routes are modelled while large parts of the map and most
interiors are still flat; it is a work in progress.

## Status

| | Status |
| :--- | :--- |
| Switch full build, NRO and single-file data pack | Working |
| Switch loader and low-address memory | Built and audited; hardware run pending |
| Video, EGL/GLES3, handheld/dock | Implemented |
| Audio, audout feeder/resampler | Implemented |
| Input, touch and ZL/ZR speed | Implemented |
| HOME, sleep and exit handling | Implemented with libnx applet hooks |
| Save layout across ARM32/AArch64 | Verified |
| Overlay layout audit | 13 reviewed findings, 0 not allowed |
| Local wireless on Switch | Not part of this milestone (stubbed) |
| Nintendo 3DS build | Working |
| Android build | Working, with the Android SDK/NDK |
| First hardware boot | Pending |

The audits and the full build are described in
[the Switch plan](switch_port/SWITCH_PORT_PLAN.md) and
[the final audit](switch_port/FINAL_AUDIT_2026-11-25.md).

## Install

Download the latest `emerald-nx.zip` from the repository's
[Releases](releases/latest) page (or build it yourself; see
[Building](#building-from-source)).

1. Unzip the archive at the **root** of the SD card. It contains:

   ```
   sdmc:/switch/emerald-nx/
       emerald-nx.nro
       emerald-game.nro
       emerald3ds.pak     all game data in one file
       data/              small engine files only (15 files)
       README.txt
       licences/
   ```

   `emerald3ds.pak` replaces the old `data/` tree of 6,400 files. It is read
   in place; do not extract it. The `data/` folder keeps only the engine files
   the game opens directly: the boot marker, ABI/build id, asset index, script
   and song relocation tables, and the port's own art.

   If you are updating from a package that used the old loose `data/` tree,
   delete that folder before extracting; the new package recreates only the
   15 engine files.

2. On the Switch, open hbmenu and launch `emerald-nx.nro` as a **title
   takeover**: hold **R** while opening a game, then choose the NRO. Do not
   start it from the album/applet mode; the loader needs the code-memory
   syscalls and a process handle.

3. The game writes:

   - `sdmc:/switch/emerald-nx/loader.log` — the loader's image/heap/syscall
     log, recreated on every launch.
   - `sdmc:/switch/emerald-nx/emerald-nx.log` — the game's log, recreated on
     every launch. The previous run is kept as `emerald-nx-prev.log`, and a
     run stops writing at 1 MiB, so the two files together never exceed
     2 MiB.
   - `sdmc:/switch/emerald-nx/boot.log` — a few early-startup breadcrumbs,
     recreated on every launch (capped at 4 KiB).
   - `sdmc:/switch/emerald-nx/settings.txt` and `*.sav` — settings and saves.

   By default the log keeps startup steps, errors and one progress line per
   minute. To diagnose a problem, create an empty file named
   `sdmc:/switch/emerald-nx/debug.txt`: the next launch adds the verbose
   diagnostics (display registers, tilemaps, per-frame timings and asset
   reads), still under the 1 MiB cap. Delete it afterwards.

4. If the screen stays black, read `loader.log` first. It reports the low image
   base, the heap range and any `svcMapProcessCodeMemory` /
   `svcSetProcessMemoryPermission` failure. Emulators that do not provide the
   code-memory syscalls are not supported by the loader.

`emerald3ds.pak` and `data/` are generated by the build. They contain no ROM
or save file, but they are still game content: do not redistribute a package
containing them unless you have the rights. See
[Data and legal](#data-and-legal).

## Controls

| Button | Action |
| :--- | :--- |
| A / B / X | The GBA A / B / X buttons, by their printed letters |
| Y | Switch VOXEL 3D on or off (the 3D/2D toggle), anywhere |
| D-pad / left stick | Move |
| L / R | GBA L / R |
| ZL / ZR | Hold either for the MAX SPEED; release returns to 1x. Both together lock the MAX SPEED, or 1x again |
| Plus | Start |
| Minus | Select (the registered item; in a battle's action menu, the quick ball) |
| X | Open the bottom-screen panel, also during conversations; in battle, show the bottom battle screen (again to close, or B from its main menu) |
| Touch screen (handheld) | The bottom-screen panel |

HOME is handled by the system; it is not mapped as a game button. The Switch
lifecycle pauses the audio feeder while the HOME menu or sleep owns the screen.

## Building from source

### Nintendo Switch

Requirements:

- devkitPro with **devkitA64**, **libnx**, **switch-tools**, **switch-mesa** and
  **switch-libdrm_nouveau**.
- Python 3.13 with Pillow for the data generators.
- `make`, `zip` and a POSIX shell (devkitPro's MSYS2 shell on Windows).

Build a bootstrapped tree once, then the full package:

```sh
export DEVKITPRO=/opt/devkitpro
python tools/bootstrap.py
bash switch_port/build_switch.sh
```

Outputs:

```
switch_port/dist/switch/emerald-nx/
    emerald-nx.nro
    emerald-game.nro
    emerald3ds.pak
    data/
    README.txt
    licences/
switch_port/dist/emerald-nx.zip
```

`build_switch.sh` keeps the generated `romfs/` data from an earlier 3DS or
Android build and rebuilds only the platform objects, bundles and Switch
executables. The release default is `DATA_BACKEND=3` (one `emerald3ds.pak`);
`DATA_BACKEND=1` keeps the loose `data/` tree for development. Use
`CLEAN_BUILD=1 bash switch_port/build_switch.sh` after a foreign-platform
build.

Optional static audits:

```sh
cd build/upstream/3ds_port
make -f ../../../switch_port/switch.mk check-save-layout
make -f ../../../switch_port/switch.mk check-overlays
```

The save audit compares the 32-bit ARM and AArch64 layouts of `SaveBlock1`,
`SaveBlock2` and `PokemonStorage`. The overlay audit reports structures laid
over GBA-sized storage; the reviewed Switch exceptions are in
[`switch_port/tools/check_overlays.allow`](switch_port/tools/check_overlays.allow).

### Nintendo 3DS

The original 3DS port is unchanged:

```sh
export DEVKITPRO=/opt/devkitpro
python tools/bootstrap.py --make
cd build/upstream/3ds_port
make -j8 PYTHON=python3
```

The result is `emerald3ds.3dsx`. See
[3ds_port/README.md](3ds_port/README.md) and
[docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) for the release/data-pack steps.

### Android

The Android port lives in [`android_port/`](android_port/):

```sh
python tools/bootstrap.py
android_port/tools/build.sh                # development APK
android_port/tools/build.sh --release      # release APK
```

It needs the Android SDK and NDK, a Python with Pillow, libclang's Python
bindings for the layout check and the supported ROMs. The original Android
documentation is kept in
[README-emerald-android.md](README-emerald-android.md).

## Technical overview

- **Loader.** `switch_port/loader` follows `nx-hbloader`'s NRO load path:
  the NRO header, `svcMapProcessCodeMemory`, per-segment permissions and a low
  heap/stack through the homebrew ABI. It uses a page map so RX pages are set
  before any writable page, and exits the process when the game returns instead
  of handing aliased code pages back to hbloader.
- **Video.** The shared `android_video.c` renderer draws the GBA picture and
  uploads it through EGL/GLES3 from `nwindowGetDefault()`. `SyncWindows`
  recreates the surface when `appletGetOperationMode()` changes.
- **Audio.** The shared mixer ring feeds libnx `audout` at 48 kHz through a
  phase-continuous linear resampler. Released buffers are drained directly.
- **Input.** `switch_input.c` maps libnx pad and touch to the shared input
  decoder; the touch is restricted to the bottom-screen panel and followed
  until release.
- **Filesystem.** `switch_fs.c` translates the 3DS paths used by the engine:
  `romfs:/` → `sdmc:/switch/emerald-nx/data/`, the old `sdmc:/3ds/emerald3ds/`
  → `sdmc:/switch/emerald-nx/`, and legacy Pokeemerald paths to `legacy/`.
- **Lifecycle.** `switch_lifecycle.c` is the only unit that includes
  `<switch.h>` beside the libctru-compatible shim; it exposes `appletMainLoop`
  and focus state without mixing `Result`/`Handle` definitions.
- **Data.** `emerald3ds.pak` is the same single-file pack the 3DS release
  uses. Its 6,425 entries cover graphics, maps, scripts, songs, tables and
  voxel data; the engine opens them through `CtrData_*`, which seeks inside
  one file under a lock. Only the engine ABI, asset index, relocation tables
  and port art stay loose under `data/`. The Switch build uses `elf32to64.py`
  for assembled data and the PIE load bias, with all 32-bit absolute addresses
  confined to `.gamedata`.

More detail is in [switch_port/README.md](switch_port/README.md) and
[switch_port/SWITCH_PORT_PLAN.md](switch_port/SWITCH_PORT_PLAN.md).

## Repository layout

| Path | Role |
| :--- | :--- |
| [`switch_port/`](switch_port/) | Emerald NX loader, Switch host layer, Makefile, package script and audits. |
| [`3ds_port/`](3ds_port/) | The original Nintendo 3DS backend and game bridge. |
| [`android_port/`](android_port/) | The Android backend, app shell and in-app data builder. |
| [`patches/pokeemerald/`](patches/pokeemerald/) | Changes to the pinned upstream decompilation. |
| [`builder/`](builder/) | The data builder used by the 3DS and Android releases. |
| [`docs/`](docs/) | Architecture, development, installation and feature documentation. |
| [`upstream.lock`](upstream.lock) | The pinned pret/pokeemerald revision and tool versions. |

## Credits

- **[ZallaxDev](https://github.com/ZallaxDev)** and the Emerald 3DS
  contributors: the original native 3DS port, engine bridge, touch interface,
  voxel overworld, data pipeline and documentation. All the 3DS work is theirs.
- **[Cer0-D](https://github.com/Cer0-D)** and the Emerald Android contributors:
  the Android backend, GLES renderer, portable platform layer and app builder.
- **[toniisound](https://github.com/toniisound)**: the Switch ports
  ([The Oracles Project NX](https://github.com/toniisound/The-Oracles-Project-NX),
  [oracles-nx](https://github.com/toniisound/oracles-nx),
  [smbr-nx](https://github.com/toniisound/smbr-nx),
  [DKC1/2/3Recomp-NX](https://github.com/toniisound/DKC1Recomp-NX)) — NRO
  packaging, EGL/GLES, audout, input and applet patterns.
- **[Alexgg1014](https://github.com/Alexgg1014)**: Dusklight's Switch port —
  low-address code-memory staging, per-range permissions, cache handling and
  display-mode polling.
- **[switchbrew](https://github.com/switchbrew)**: `nx-hbloader`, the NRO load
  path and the homebrew ABI used by the loader.
- **[devkitPro](https://devkitpro.org)**: devkitARM, devkitA64, libnx,
  switch-tools, switch-mesa and libdrm_nouveau.
- **pret/pokeemerald** and contributors: the decompilation pinned in
  [`upstream.lock`](upstream.lock).
- **pokeemerald-multiplatform**: the voxel renderer logic reused by the port;
  see [3ds_port/src/voxel/NOTICE.md](3ds_port/src/voxel/NOTICE.md).
- The HeartGold/SoulSilver follower sprites and texts come from
  pret/pokeheartgold and Poké Corpus, fetched by the builder; they are not
  included in this repository.

The complete third-party inventory and licence texts are in
[NOTICE.md](NOTICE.md); the release package ships them under `licences/`,
and its install notes are in
[`switch_port/PACKAGE_README.txt`](switch_port/PACKAGE_README.txt).

## Data and legal

**This repository contains no ROMs, saves or extracted game data.** The
engine is the pret/pokeemerald decompilation; Emerald's code, graphics, audio,
text and characters are the property of Nintendo, Game Freak and Creatures.
The original port code is licensed under
[CC BY-NC-SA 4.0](LICENSE-PORT.md); third-party components keep their own
licences, listed in [NOTICE.md](NOTICE.md).

A generated `emerald3ds.pak` or `data/` folder, and any release package
containing them, still contains game content. Do not redistribute it unless
you have the rights. The project does not provide ROMs or saves, and does not
condone piracy.

## Reporting problems

Open an issue on this repository with:

- the console model and firmware,
- whether the app was launched as a title takeover,
- the newest `loader.log` and `emerald-nx.log` (and `boot.log` if it exists);
  add `debug.txt` before reproducing if the problem needs the verbose log,
- the exact steps and the first visible error.

Please report Emerald NX problems here, not to the Emerald 3DS or Android
projects. The 3DS and Android READMEs are kept for their own platforms.

## Reference ports

| Upstream | Switch port |
| :--- | :--- |
| [TwilitRealm/dusklight](https://github.com/TwilitRealm/dusklight) | [Dusklight — Alek's Ultimate NX Edition](https://github.com/Alexgg1014/The-Legend-of-Zelda-Twilight-Princess-Alek-s-Ultimate-NX-Edition) |
| [thibautbus/top](https://github.com/thibautbus/top) | [The Oracles Project NX](https://github.com/toniisound/The-Oracles-Project-NX) |
| [kirby-letsgo/oracles-decomp](https://github.com/kirby-letsgo/oracles-decomp) | [oracles-nx](https://github.com/toniisound/oracles-nx) |

The Switch port's structure, release layout and subsystem choices are
documented against these references in
[switch_port/SWITCH_PORT_PLAN.md](switch_port/SWITCH_PORT_PLAN.md).
