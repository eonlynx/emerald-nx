# Emerald NX - final pre-hardware audit (2026-11-25)

## 1. Reference ports

Checked against the local oracles-nx / The-Oracles-Project-NX trees, the
shallow clones of smbr-nx and Dusklight (Alexgg1014), and nx-hbloader.

| Subsystem | Finding | Decision |
|---|---|---|
| Loader | nx-hbloader's `loadNro`: `header.size` includes the 0x80 CRT0/NroHeader prefix; icon/NACP follow it; only the image plus zeroed bss may be mapped. smbr's `so_finalize` sets RX pages before any writable page. Dusklight's `switch_elf_memory.cpp` flushes/invalidate caches and unmaps at exit when control returns to hbloader. | Adopted the header/bss handling and RX-first page map; the loader exits the process instead of returning to hbloader, so it never hands back aliased code pages. |
| Video/EGL | smbr creates the EGL surface from `nwindowGetDefault`, sets `nwindowSetDimensions`, uses `eglSwapInterval`; Dusklight polls `appletGetOperationMode()` every frame. | The existing Android GLES renderer already uses the same NWindow/EGL model. `SyncWindows` recreates the surface when the operation mode changes (720p handheld / 1080p docked). |
| Audio | smbr's audout feeder uses `audoutGetReleasedAudioOutBuffer`; the Oracles ports use SDL3 with toniisound's Switch audio patch. | Kept the direct libnx audout feeder (no SDL3 dependency); switched reclaim to `audoutGetReleasedAudioOutBuffer`. The SDL patch is unnecessary for this design and remains documented as the reference if SDL3 is added. |
| Input/touch | smbr configures `NpadStandard`, initializes the touch screen and reads focus state; Oracles maps printed A/B/X/Y. | The Switch host maps libnx pad/touch to the Android host input surface, with A/B/X/Y by letter, ZL/ZR speed and panel-only touch. HOME is not mapped as a game button. |
| Dock/handheld | Dusklight polls the applet mode per frame; smbr fixes a size at startup. | Per-frame `appletGetOperationMode()` and surface recreation; no restart. |
| Home/sleep | smbr runs `while (appletMainLoop())` and checks `appletGetFocusState()`; Oracles handles `WILL_ENTER_BACKGROUND` through SDL. DKC1Recomp-NX registers an `appletHook` for exit/focus and persists its SRAM there. | Added `switch_lifecycle.c`: `appletHook` records exit/focus, `appletMainLoop()` runs every frame, focus loss pauses audout, focus return resets clocks. Emerald's saves are written synchronously by its own save code, and settings/logs flush in `CtrPlatform_Shutdown`, so no extra SRAM hook is needed. HOME/exit leaves the loop cleanly. |

## 2. Packaging

Matched the Oracles/Dusklight release shape:

```
switch/emerald-nx/
    emerald-nx.nro
    emerald-game.nro
    data/
    README.txt
    licences/
```

`build_switch.sh` builds the loader, the game and the staged data, then writes
`switch_port/dist/emerald-nx.zip` with the `switch/` directory at the archive
root. The game and loader now use `switch_port/icon.jpg` (256x256, derived
from the project artwork) instead of the libnx default icon.

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

- Switch: full AArch64 build, NRO, data bundles and package pass. Link check
  accepts only the expected `R_AARCH64_ABS32` sites in `.gamedata`. Final
  build id `ac68ac2c0e6ccbff`; package SHA-256
  `A51947B218BD45F940A092D1050118ECFAFF0114C2F2C8FEA0E1A7EA47839755`.
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


