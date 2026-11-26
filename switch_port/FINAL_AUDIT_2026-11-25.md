# Emerald NX - final pre-hardware audit (2026-11-25)

## 1. Reference ports

Checked against the local oracles-nx / The-Oracles-Project-NX trees, the
shallow clones of smbr-nx and Dusklight (Alexgg1014), and nx-hbloader.

| Subsystem | Finding | Decision |
|---|---|---|
| Loader | nx-hbloader's `loadNro`: `header.size` includes the 0x80 CRT0/NroHeader prefix; icon/NACP follow it; only the image plus zeroed bss may be mapped. smbr's `so_finalize` sets RX pages before any writable page and flushes caches on the source before mapping. Dusklight's `switch_elf_memory.cpp` invalidates the instruction cache only after the RX permissions exist. | Adopted the header/bss handling, RX-first page map and post-permission icache invalidation; the loader exits the process instead of returning to hbloader, so it never hands back aliased code pages. |
| Video/EGL | smbr creates the EGL surface from `nwindowGetDefault`, sets `nwindowSetDimensions`, uses `eglSwapInterval`; Dusklight polls `appletGetOperationMode()` every frame. | The existing Android GLES renderer already uses the same NWindow/EGL model. `SyncWindows` recreates the surface when the operation mode changes (720p handheld / 1080p docked). |
| Audio | smbr's audout feeder uses `audoutGetReleasedAudioOutBuffer` and a libnx `threadCreate` feeder; the Oracles ports use SDL3 with toniisound's Switch audio patch. | Kept the direct libnx audout feeder (no SDL3 dependency); the feeder is a libnx `threadCreate` thread, and reclaim uses `audoutGetReleasedAudioOutBuffer`. The SDL patch is unnecessary for this design and remains documented as the reference if SDL3 is added. |
| Startup/logging | smbr opens its log synchronously in `userAppInit` and uses libnx threads for its engine/audio; DKC opens `debug.log` with `freopen` and registers an `appletHook`; Dusklight uses `userAppInit`/`userAppExit` for `romfsInit`/`nvInitialize` and writes early exit traces through the FS service. | The game opens `emerald-nx.log` and `port.log` directly, with no newlib writer thread; startup logs each platform step; `boot.log` breadcrumbs go through the FS service before `emerald-nx.log` exists. |
| Input/touch | smbr configures `NpadStandard`, initializes the touch screen and reads focus state; Oracles maps printed A/B/X/Y. | The Switch host maps libnx pad/touch to the Android host input surface, with A/B/X/Y by letter, ZL/ZR speed and panel-only touch. HOME is not mapped as a game button. |
| Dock/handheld | Dusklight polls the applet mode per frame; smbr fixes a size at startup. | Per-frame `appletGetOperationMode()` and surface recreation; no restart. |
| Home/sleep | smbr runs `while (appletMainLoop())` and checks `appletGetFocusState()`; Oracles handles `WILL_ENTER_BACKGROUND` through SDL. DKC1Recomp-NX registers an `appletHook` for exit/focus and persists its SRAM there. | Added `switch_lifecycle.c`: `appletHook` records exit/focus, `appletMainLoop()` runs every frame, focus loss pauses audout, focus return resets clocks. Emerald's saves are written synchronously by its own save code, and settings/logs flush in `CtrPlatform_Shutdown`, so no extra SRAM hook is needed. HOME/exit leaves the loop cleanly. |

## 2. Packaging

Matched the Oracles/Dusklight release shape:

```
switch/emerald-nx/
    emerald-nx.nro
    emerald-game.nro
    emerald3ds.pak
    data/
    README.txt
    licences/
```

`build_switch.sh` builds the loader, the game and the staged data, then writes
`switch_port/dist/emerald-nx.zip` with the `switch/` directory at the archive
root. The game and loader now use `switch_port/icon.jpg` (256x256, derived
from the project artwork) instead of the libnx default icon.

The 6,425 data files are packed as `emerald3ds.pak`; `data/` keeps only the
15 engine files the engine opens directly (boot marker, ABI/build id, asset
index, relocation tables and port art). This is the same single-file approach
as the 3DS release and as toniisound's `smbr-nx`, which ships its game data as
one `game.pck`; the Oracles ports and Dusklight instead require a ROM or disc
image. A runtime ZIP reader was not added: the engine already has a pack
backend, and a ZIP would need a new inflate/decompression path on Horizon.

## 3. Proginji2000 branches

Audited every remote branch:

- `feat/battle-speed-options-20261122`: took `ee834a5` (BATTLE SPEED row,
  AUTO follows world speed, French trade-evolution label). No story/events.
- `feat/map-display-mode-20261122` / `feat/visited-city-markers`: took the
  visited-town map read/draw commits, the optional `map_progressive` setting
  (default CLASSIC), and the runtime fixes for partial redraws, progress
  counter and locale.
- `fix/thor-runtime-feedback-20261123`: the ZL/ZR battle routing and the
  read-only imported-save diagnostic were already picked; took the map runtime
  fixes and locale commit. Tests/tooling/CI/docs were left out.
- `fix/release-audit-provenance`, `tooling/android-fr-preflight`, the roadmap
  docs and CI-only commits contain no runtime change.

## 4. 3DS vs Android code

Switch-specific native pieces now replace Android services:

- `switch_fs.c`: native `sdmc:`/`romfs:` path translation.
- `switch_audio.c`: native libnx audout feeder/resampler.
- `switch_input.c`: native libnx pad/touch.
- `switch_lifecycle.c`: native libnx applet loop/focus.
- `switch_arm.c`: counter and `posix_memalign`.

The remaining Android-named layer is the portable part: `android_platform.c`
(frame pacing shared with 3ds_platform.c), `android_video.c` (software GBA
renderer plus GLES3), `android_c3d.c` (Citro3D-on-GLES), `android_input.c`
(the shared input decoder) and `android_stubs.c`. The 3DS PICA video path
cannot be reused on Horizon, so those are deliberate, not accidental bridges.

The pthread thread wrapper remains because libctru's opaque `Thread` return,
detached flag and `threadJoin`/`threadFree` do not match libnx's
`threadCreate(Thread*, ...)` API; its stacks come from the loader's low heap.

## 5. Builds

- Switch: full AArch64 build, NRO, data bundles, single-file pack and package
  pass. Link check accepts only the expected `R_AARCH64_ABS32` sites in
  `.gamedata`. Final build id `c61ce9837dd5b518`; package 13,358,545 bytes,
  SHA-256 `F98D37AC73F2CA06859EAF4E3286739BEC1AAEF092C7FE06712EE365E4B4A403`.
  `emerald-game.nro` SHA-256
  `7D329935D22ECC8736890F3DC3C087DF32CB825C9D0AD59B1793B4BF74362BB3`,
  `emerald3ds.pak` SHA-256
  `40D1FA8CD0E33C0226F24E69B0DF7B74A035808EF59EE21D785D3A09E7B2EF7C`
  (6,425 entries, 55,899,776 bytes, ABI `898dbec9`), loader SHA-256
  `29ED37F56F6EB0D2621B3E574C8FFC1BFAA30324CA9696085207EBFE8A6D9E2B`.
- 3DS: rebuilt from a clean copy of the bootstrapped tree with devkitARM;
  `emerald3ds.elf` 31,901,540 bytes and `emerald3ds.3dsx` 62,087,788 bytes,
  rc=0.
- Android: `android_port/android.mk`, `build_apk.sh` and the JNI/Java sources
  are untouched; the two shared files changed (`android_platform.c` lifecycle,
  `android_video.c` NWindow branch) keep the Android paths behind
  `#ifdef __SWITCH__`. The shared map/battle units compile in the 3DS and
  Switch builds; a full Android build needs the NDK, which is not installed
  here.

## 6. Simplification/performance

- audout reclaim now drains released buffers directly.
- Loader permissions use one page map, every page set once.
- The loader copies only NRO image bytes and zeroes bss.
- Link-time script/sound/asset/gamedata bundles are unchanged from the 3DS
  format; the only Switch-specific part is `elf32to64.py` and the PIE load
  bias.
- Frame clocks reset on focus return; dock changes recreate only the EGL
  surface.
- Game data now goes through the existing 3DS pack backend: one
  `emerald3ds.pak` (6,425 entries) instead of 6,400 small files. The pack is
  read in place with the same seek/lock path used on 3DS, so it adds no
  decompression or per-file SD metadata. The release zip drops from 15.24 MB
  to 13.36 MB because the pack replaces thousands of directory entries.
  `DATA_BACKEND=1` still builds the loose tree for development.



