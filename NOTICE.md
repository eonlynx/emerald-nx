# Notices

Emerald 3DS combines original port code with the pret decompilation and a few
third-party components. Each keeps its own terms; `LICENSE-PORT.md`
(CC BY-NC-SA 4.0) covers only the original Emerald 3DS work.

## Upstream decompilation

- **pret/pokeemerald** — https://github.com/pret/pokeemerald, pinned in
  `upstream.lock`. Fetched by `tools/bootstrap.py`, never vendored here. The
  repository does not state a licence; Emerald 3DS does not relicense it. The
  files in `patches/pokeemerald/` carry the port's changes and, as patch
  context, lines of the upstream files they modify.

## Code derived from third parties

- **pokeemerald-multiplatform** (gradenGnostic/pokeemerald-multiplatform,
  commit db1cab3d2dc9f0e0a9f4a3d67983e5acf30a9a46), MIT. The voxel overworld
  in `3ds_port/src/voxel/` reuses the logic of its SDL2/OpenGL voxel renderer
  (map instances, metatile classification, atlas composition, follow camera).
  Full text and scope: `3ds_port/src/voxel/NOTICE.md`.
- **dlmalloc 2.8.6** (Doug Lea), public domain (CC0 1.0), unmodified in
  `android_port/third_party/dlmalloc/`: the arm64 Android build's heap below
  4 GiB (`android_port/src/android_lowmem.c`).
- **devkitARM `3dsx.ld`**, Mozilla Public License 2.0.
  `3ds_port/emerald3ds.ld.in` is a modified copy and stays under the MPL 2.0
  (https://mozilla.org/MPL/2.0/).

## Switch port

- **nx-hbloader** (switchbrew), ISC-style licence. `switch_port/loader/` follows
  its NRO load path (`loadNro`: NroHeader, `svcMapProcessCodeMemory`, segment
  permissions) and reuses its low-address mapping model for Emerald's 32-bit
  data. The source is a separate implementation for a two-stage NRO loader.
- **smbr-nx** (toniisound), MIT. The loader's page-map permission pass follows
  `source/so_util.c` (`so_finalize`): set executable pages RX before any page
  becomes writable, then the remaining pages RW. The audout feeder follows the
  same `audoutAppendAudioOutBuffer`/resampler pattern.
- **oracles-nx** and **The-Oracles-Project-NX** (toniisound), MIT. Reviewed for
  the NRO/CMake packaging, `sdmc:/switch/...` layout, title-takeover launch and
  log placement; the Makefile build keeps the Emerald tree's own structure.
- **Dusklight's Switch port** (Alexgg1014), GPL-2.0 with the funchook linking
  exception. `src/dusk/switch_elf_memory.cpp` was reviewed for the
  stage -> `svcMapProcessCodeMemory` -> per-range permissions -> cache flush
  sequence and for the exit-cleanup lesson (unmap if returning to hbloader).
  No Dusklight code is included; the loader is written from the ISC/MIT
  references above.
- **libnx** (devkitPro), MIT. `switch_port/emerald_switch.ld` is a modified copy
  of libnx's `switch.ld`, and the NRO build uses libnx's `switch.specs`,
  `elf2nro` and `nacptool`.
- **switch-mesa / libdrm_nouveau** (devkitPro), MIT. Linked for EGL/GLES3.
- **devkitA64 / GCC / newlib** — toolchain and C library.

## Libraries used by the build

- **libctru, citro2d, citro3d** (devkitPro), zlib licence — linked into the 3DSX.
- **devkitARM / GCC / newlib** — toolchain and C library.

## Bundled with the Windows builder

- **Python** (PSF License Agreement) and its standard library, **Tcl/Tk**
  (BSD-style) and **Pillow** (MIT-CMU / HPND), packaged by **PyInstaller**
  (GPL 2.0 with the bootloader exception).

## Game content

Pokémon Emerald's code, data, graphics, audio and text are © Nintendo, Game
Freak and Creatures. None of it is included in this repository or in the
releases; the builder produces the data pack on the player's computer from
their own ROM.

**HeartGold and SoulSilver's follower sprites and messages**, fetched by the
builder on the player's computer, never included in this repository or the
releases: the overworld sprites from pret/pokeheartgold
(https://github.com/pret/pokeheartgold, commit
`3291d88a7e3e264b503024ead80f4d8b55b835cd`), the messages in the ROM's
language from Poké Corpus by abcboy101 (https://github.com/abcboy101/poke-corpus,
commit `5d1da078d70dea5c9feaaba7575626eab4f10a08`). The builder downloads both
from forks that keep those commits (https://github.com/ZallaxDev/pokeheartgold,
https://github.com/ZallaxDev/poke-corpus), each file checked against
the SHA-256 listed in `builder/emerald3ds_builder/followers_sources.py` and
converted for the console. They are Nintendo, Game Freak and Creatures' work;
not relicensed.
