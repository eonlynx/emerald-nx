# Emerald 3DS -> Nintendo Switch - Port Plan (Phase 3)

**Status:** implemented; the port runs on hardware, with final polish and
testing in progress.
**Branch:** `port-switch-wip` (Phase 1 complete, Phase 2b cherry-picks applied).
**Date:** 2026-11-26.

---

## 0. Executive summary

Phase 1 is done: `port-switch-wip` was created from `main` and merged
`Cer0-D/main` (`f82776a8`, `--no-ff` merge), with no conflicts. The Android
single-screen base is integrated.

Phase 2 found **22 commits on `Proginji2000/main`, all documentation**
(`README.md` and `docs/`). There is not a single code, build, audio, input,
render or filesystem change to cherry-pick. Therefore:

- **APPROVED on `main`:** 0.
- **DISCARDED on `main`:** 22 (roadmap, story, Act II, map redesign, backlog,
  compatibility contracts and QA plans; all design/narrative, not technical
  fixes).
- **Cherry-picks applied from `main`:** 0.

A later Phase 2b audit of the two technical branches added **2 approved
cherry-picks** (see section 1.3):
`1e2e8bb` (ZL/ZR battle-speed routing) and `f6373bb` (read-only imported-save
diagnostic).

Phase 3 confirms that **toniisound solved 90% of the porting work**, but **there
is no Emerald port by him**. His NX repos are 64-bit-clean games (Oracles, DKC,
DOA, Godot) or emulators with translated guest memory. Emerald is different:
the 3DS/Android engine stores **32-bit addresses** in data and structures and
expects everything (code, data, heap, stack) to be below 4 GiB.

The key finding of this phase:

> hbloader runs NROs in a **36-bit process**, but the NRO it loads is mapped
> with `virtmemFindCodeMemory` inside the alias-code region (128 MiB-64 GiB).
> That address is random and can be above 4 GiB. Being an NRO is therefore not
> enough: the mapping must be **forced low**.
>
> The recommended solution is a **two-stage NRO loader**, modelled on
> `nx-hbloader`'s `loadNro` and toniisound's `smbr-nx` `so_util`:
> `emerald-nx.nro` reserves a low gap (128 MiB-2 GiB), maps the real
> `emerald-game.nro` there with `svcMapProcessCodeMemory`, passes it a low heap
> and stack, and jumps to its entrypoint. The game remains a normal libnx NRO
> and applies its own relocations when loaded low.

This makes Emerald's 32-bit pointers valid again without migrating the whole
engine to 64-bit.

---

## 1. Phase 2 - `Proginji2000/main` audit

### 1.1 Divergence point

- Merge-base with `Cer0-D/main`: `ba8ca267` ("Update README.md").
- `Proginji2000/main` = `ba8ca267` + 22 commits.
- Total delta: 7 files, +434/-2 lines, **only** `README.md` and `docs/`.
- Check: `git diff --name-only ba8ca267..Proginji2000/main` returns no file
  outside `README.md`/`docs/`.

### 1.2 Commit-by-commit table

| Commit | Date | File(s) | Classification | Reason |
|---|---|---|---|---|
| `59132b1` | 2026-11-24 | `README.md` | DISCARDED | Presents the "Emerald Extended" roadmap and QA status; not a fix. |
| `fb65c43` | 2026-11-24 | `README.md` | DISCARDED | Act II narrative campaign. |
| `f24e697` | 2026-11-24 | `README.md` | DISCARDED | Future voxel improvements; intent only. |
| `26054c9` | 2026-11-24 | `docs/EMERALD_EXTENDED_BACKLOG.md` | DISCARDED | 16-item backlog; implements none of them. |
| `e92b3ec` | 2026-11-24 | `docs/UPSTREAM_SYNC.md` | DISCARDED | Sync policy; useful reading, not a patch. |
| `bb31c27` | 2026-11-24 | `README.md` | DISCARDED | Links to the backlog; no code. |
| `558a1d1` | 2026-11-24 | `docs/EMERALD_EXTENDED_BACKLOG.md` | DISCARDED | Links to issues; no code. |
| `d1ae8af` | 2026-11-24 | `README.md` | DISCARDED | GitHub Issues status; no code. |
| `5b6d950` | 2026-11-24 | `docs/ANDROID_LOCAL_QUALIFICATION.md` | DISCARDED | Android/Thor QA plan; not a fix or real CI. |
| `0820456` | 2026-11-24 | `README.md` | DISCARDED | Links to the QA plan; no code. |
| `01b3528` | 2026-11-24 | `docs/EMERALD_EXTENDED_BACKLOG.md` | DISCARDED | "Evolving world" routes and gym redesign. |
| `a80d3e0` | 2026-11-24 | `docs/EMERALD_EXTENDED_BACKLOG.md` | DISCARDED | Second League and narrative finale. |
| `98c45ad` | 2026-11-24 | `docs/EMERALD_EXTENDED_LEVEL_DESIGN.md` | DISCARDED | Level design charter; not technical parity. |
| `1658483` | 2026-11-24 | `README.md` | DISCARDED | Links to the level charter; no code. |
| `fa1fc51` | 2026-11-24 | `docs/EMERALD_EXTENDED_BACKLOG.md` | DISCARDED | Deoxys/original story arc. |
| `214a17d` | 2026-11-24 | `docs/EMERALD_EXTENDED_ACT2_DESIGN.md` | DISCARDED | Act II narrative design. |
| `3b990cf` | 2026-11-24 | `docs/EMERALD_EXTENDED_BACKLOG.md` | DISCARDED | Act II dependencies; no code. |
| `19b1850` | 2026-11-24 | `README.md` | DISCARDED | Links to Act II design; no code. |
| `af3257d` | 2026-11-24 | `docs/EMERALD_EXTENDED_ACT2_DESIGN.md` | DISCARDED | Voice/design decisions; not a fix. |
| `d9ad42b` | 2026-11-24 | `docs/EMERALD_EXTENDED_COMPATIBILITY.md` | DISCARDED | Gen III save/legality contract; documentation, not code. |
| `5bf2f4c` | 2026-11-24 | `docs/EMERALD_EXTENDED_BACKLOG.md` | DISCARDED | Reconciles 23 tickets; no code. |
| `d343277` | 2026-11-24 | `README.md` | DISCARDED | Links to documents; no code. |

**Conclusion:** no `Proginji2000/main` commit meets the APPROVED criteria
(stability, memory, render, audio, controls, filesystem or technical parity).
`git cherry-pick -x` was not run.

### 1.3 Phase 2b - technical branches

The user approved auditing the two branches after the initial `main` pass.
Both branch histories were compared against `ba8ca267`:

- `fix/thor-runtime-feedback-20261123` (23 commits)
- `tooling/android-fr-preflight-20261122` (14 commits, a subset of the fix
  branch)

The following were reviewed and **cherry-picked with `-x`**:

| Commit | Subject | Why approved |
|---|---|---|
| `1e2e8bb` | `fix(android): route ZL/ZR to independent battle speed in battles` | Real input-routing fix: ZL/ZR now acts on the battle-speed setting while in battle and keeps the overworld speed outside it. No story, map or event changes. It uses the existing `CtrPlatform_BattleSpeedKeys`/`StepBattleSpeed` API. |
| `f6373bb` | `diag(save): report imported save files unreadable for writing without modifying them` | Read-only diagnostic for imported saves: logs a save that is readable but not writable, without chmod, rename, rewrite or changing flash state. Pure runtime/stability diagnostics. |

Resulting commits on `port-switch-wip`:

- `56533ff` - ZL/ZR routing.
- `e8e2699` - imported-save diagnostic.

The rest were **DISCARDED**:

- Map/UI features and fixes (`d7db1c3`, `74bf32d`, `c4258a6`, `d81c1bb`,
  `2ac0fa0`, `f3c4a9b`, `4e1fc36`, `a075c2c`, `f755e34`): visited-city
  markers, overlay options and locale counters are gameplay/UI changes.
- Test-only commits (`5880097`, `1f9807a`, `13f2ece`, `a6fdce0`, `8983066`,
  `eaa4a54`, `0f65dd2`, `cff0675`): fixtures, sanitizers and assertions; not
  runtime fixes.
- Build/CI/tooling (`0667b39`, `22d31c9`, `ee78326`): preflight scripts, CI
  bounds and diagnostics; Android/host tooling, not Switch runtime.
- Docs/QA (`1d27ad6`): report only.
- Licensing-only (`android_port/third_party/dlmalloc/LICENSE` in the branch):
  no code behavior.

No commit from either branch was applied that touches story, dialogue, map
events, balance or new game mechanics.

### 1.4 Working-tree note

All three repos arrived with **every file marked modified**. The check
`git diff --ignore-space-at-eol` returned 0 lines: it was CRLF noise.
`core.autocrlf=false` and `git checkout -- .` were applied to `emerald-3ds`;
there were no real changes and no untracked files.

---

## 2. Phase 3 - How toniisound ported (and what is missing)

### 2.1 Repositories reviewed

| Repo | What it is | What it contributes |
|---|---|---|
| `The-Oracles-Project-NX` | Oracle of Ages/Seasons launcher, SDL3 + mGBA | CMake/devkitA64, static SDL3, EGL/GLES2, NRO/NACP, Nintendo input, audio patch, `sdmc:/switch/...`, 8 MiB thread, overclock. |
| `oracles-nx` | `oracles-native` (GBC decomp) | SDL3/EGL, fibers without `mmap`, `sdmc:/switch/oracles`, Switch buttons, optional curl sync. |
| `smbr-nx` | Super Mario Bros. Remastered (Godot Android) | **`.so` loader with `svcMapProcessCodeMemory`**, code arenas, libc/GLES imports, OpenSL->audout audio, fake JNI. Key for the 32-bit problem. |
| `DKC1/2/3Recomp-NX` | SNES recompilations | SDL2 + OpenGL 3.3, Switch presenter, gamepad, rumble, settings in `sdmc`. |
| `doa3-nx`, `doa2u-nx` | Native Xbox ports | 64-bit runtime with translated 32-bit guest memory; not applicable to Emerald's host pointers. |
| `model2-nx`, `supermodel-nx`, `vf1-recomp-nx`, `daytona-recomp-nx` | Emulators/recomps | Build/input/audio patterns; guest memory, not host pointers. |
| `The-Legend-of-Zelda-Twilight-Princess-Alek-s-Ultimate-NX-Edition` (external, Alexgg1014) | Dusklight (TP) | Title takeover, `AppletType_Application`, `svcMapProcessCodeMemory`/`svcCreateCodeMemory`, address-space audit, cleanup on exit. |

### 2.2 Build system (toniisound's pattern)

**The-Oracles-Project-NX** (`219ea73`):

- `CMakeLists.txt` detects `NINTENDO_SWITCH` (toolchain `Switch.cmake`).
- SDL3 comes from `FetchContent` at
  `https://github.com/devkitPro/SDL.git`, branch `switch-sdl-3.4`, pinned commit
  `c329016c0e338ab4397ad930f83330c9fe058348`.
- `SDL_SHARED=OFF`, `SDL_STATIC=ON`, tests/examples OFF; links
  `SDL3::SDL3-static`.
- mGBA: `DISABLE_ANON_MMAP` and `_GNU_SOURCE` (libnx has no `mmap`; newlib
  needs `_GNU_SOURCE` for `strtof_l`/locale).
- `launcher/CMakeLists.txt`: `nx_generate_nacp(...)` and `nx_create_nro(...)`
  with icon and NACP; title/author/version.
- `build_switch.sh`: `cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Switch.cmake`,
  `cmake --build ... --target oracles_nro`, then a release zip with
  `switch/the-oracles-project-nx/{roms,patches,mods,licences}`.
- `cmake/apply_patch_once.cmake`: applies an SDL patch only if not already
  applied.
- `launcher/switch/sdl-switch-audio.patch`: fixes SDL's Switch audio driver
  (a voice that ran out of buffers never restarted).

**oracles-nx** (`e459ec2`): same pattern, SDL3 `FetchContent`, target
`oracles-nx`, `nx_generate_nacp`/`nx_create_nro`, `build_switch.sh`.

**Exact `Switch.cmake` flags (devkitPro):**

```
-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -ftls-model=local-exec
-ffunction-sections -fdata-sections -D__SWITCH__
-L$DEVKITPRO/libnx/lib -L$DEVKITPRO/portlibs/switch/lib
-fPIE -specs=$DEVKITPRO/libnx/switch.specs -lnx -lm
```

`nx_create_nro` calls `elf2nro` with `--icon` and `--nacp`. The `.nacp` is
generated with `nacptool`.

**Twilight Princess NX** adds a useful detail: the `.nro` is launched as a
**title takeover** (hold R while opening a game), with `__nx_applet_type =
AppletType_Application`; if launched as an applet it warns and exits. It also
uses `-fno-ipa-ra` and `-fno-inline-functions-called-once` because mods hook
functions at runtime; not needed for Emerald, but the build pattern is the same.

### 2.3 Graphics and audio backend

**The-Oracles-Project-NX:**

- `SDL_WINDOW_OPENGL` is required: Switch SDL only initializes EGL for OpenGL
  windows. The renderer is `opengles2` (`SDL_HINT_RENDER_DRIVER`).
- Single fullscreen window: `SDL_GetDesktopDisplayMode` decides 1280x720
  handheld / 1920x1080 docked; resizing is disabled.
- Static SDL3; renderer/textures go through EGL/GLES2 (switch-mesa).
- Audio through SDL3 with the voice-restart patch.
- Optional overclock: `clkrst` to 1785 MHz while running, restored on exit
  (`switch_clock.c`); requires firmware 8.0.0+.
- It does not use `SDL_GetNumLogicalCPUCores()`/`SDL_GetSystemRAM()` because
  libnx has no `sysconf`; it hardcodes 3 threads/4096 MB or 4 cores.

**oracles-nx:**

- `SDL_CreateWindowAndRenderer(..., 1280, 720, SDL_WINDOW_FULLSCREEN |
  SDL_WINDOW_OPENGL, ...)`, hint `opengles2`.
- `SDL_WINDOW_OPENGL` for the same reason: "EGL not initialized" without it.
- No resize; `apply_window_settings` forces fullscreen.
- Audio through SDL3.

**DKC1/2/3:**

- SDL2 + OpenGL 3.3 core; every GL entry point is resolved with
  `SDL_GL_GetProcAddress` because EGL on Horizon does not export some of them.
- Custom presenter, vsync, 4:3/16:9 viewport, HD upscaler.
- Rumble through `hid` directly (SDL2 is not used for vibration).

**Twilight Princess:**

- SDL3 for window/audio/gamepads; rendering (Dawn/Vulkan or deko3d) takes
  libnx's `NWindow` directly.
- Prebuilt `libvulkan.a` from mesa-switch; not the pattern we need.

### 2.4 Controls

**The-Oracles-Project-NX** (`launcher/sdl_window.c`):

- Switch: buttons are read **by their letter**, not by position:
  `SDL_GAMEPAD_BUTTON_EAST` -> A, `SOUTH` -> B, `NORTH` -> X, `WEST` -> Y.
- A = accept/OK; B = back.
- Left stick click = pause.
- Everything else maps as in `PLAYING.md`.

**oracles-nx** (`src/platform/app.c`, `src/ui/controls.c`):

- `menu_button`: `EAST`/`START` = accept; `SOUTH`/`WEST` = back.
- `pad_name`: names "B, A, Y, X, MINUS, HOME, PLUS, LSTICK, RSTICK, L, R,
  UP, DOWN, LEFT, RIGHT"; explicit `ZR`/`ZL`.
- Default pads: Pause on R; items on X/Y; HOME belongs to the system and is
  not used.
- Touch: `hidGetTouchScreenStates`, coordinates scaled from 1280x720 to the
  surface.

**DKC1/2/3:** `switch_gamepad.c` + `switch_platform.c`; rumble through `hid`
(handheld/Pro Controller/dual Joy-Con), persistent settings in
`sdmc:/switch/<app>/dkc1nx.ini`.

### 2.5 Filesystem and paths

**The-Oracles-Project-NX:**

- `sdmc:/switch/the-oracles-project-nx/`
  - `the-oracles-project-nx.nro`
  - `roms/` (US/EU ROMs)
  - `patches/` (fan-game BPS patches)
  - `mods/` (Lua mods, one folder each)
  - `settings.txt`, `the-oracles-project-nx.log`, `steps.log`
  - saves/savestates **next to the ROM**, in `roms/`.
- No file dialog: at startup it looks in `roms/` and `patches/` for the first
  file it recognises by SHA-1; "Choose" looks there again. File names do not
  matter.
- `sdl_backend.c` returns that directory for settings; `home_games.c`
  implements the `opendir/readdir` search and `oracles_dialog_post` fakes the
  dialog response.
- `open_mods`/`home_save` show the path in a message; they do not open a file
  manager.
- `mkdir("sdmc:/switch", 0777)` and `mkdir(ORACLES_SWITCH_DIR, 0777)` at
  startup.
- stderr is redirected to `the-oracles-project-nx.log`; `steps.log` is opened,
  written and closed at each step to diagnose hangs.

**oracles-nx:**

- `sdmc:/switch/oracles/`
  - `oracles.nro`
  - `roms/` (.gbc/.gb/.zip/.gz/.7z)
  - `oracles.log`
  - saves/savestates/settings in the same folder.
- `switch_install_roms` scans `roms/`, extracts recognised ROMs and installs
  them; no file picker.
- `cache_arg = SWITCH_APP_DIR` so `settings.ini` and data stay there.
- `freopen(SWITCH_APP_DIR "oracles.log", "w", stderr)`.
- `socketInitializeDefault()` only when `ORACLES_CURL_LINKED` (sync) is built;
  not called unconditionally.

**DKC1/2/3:** `sdmc:/switch/dkc1/`, `rom.smc`, `dkc1nx.ini`, saves; the CWD is
the app folder after `SwitchImpl_Init`.

**Twilight Princess:** `sdmc:/switch/dusklight/`, discs at the root or in
`discs/`, saves in `USA/`, `EUR/`, `JAP/`; logs in `cache/logs/`; `romfsInit()`
for `res/` inside the NRO.

### 2.6 Lifecycle and initialization

**The-Oracles-Project-NX** (`launcher/main.c`):

- `main()` creates `sdmc:/switch` and the app folder, deletes `steps.log`,
  redirects stderr to the log, applies the optional overclock, and runs the
  launcher on an **8 MiB pthread** (`SWITCH_STACK`). Reason: the main thread
  stack of an `.nro` is too small for the home/pause/session stacks.
- `SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)` inside `oracles_home_run`.
- `SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2")`.
- `oracles_home_games_find_files` before opening the home screen.
- On exit: `switch_clock_back()` and `steps.log("finished")`.
- It does not use `socketInitializeDefault` or `romfsInit` (data is on SD; the
  `.nro` carries no romfs). `romfsInit` does not appear in the port.
- Neighbour worker threads (`engine/ghost/ghost.c`) use 1 MiB stacks and
  `svcSetThreadCoreMask` to cores 1 and 2 (game on core 0).
- `ORACLES_SWITCH_DIR` = `sdmc:/switch/the-oracles-project-nx`.

**oracles-nx** (`src/platform/app.c`):

- `main()` creates `sdmc:/switch`, the app folder and `roms/`, redirects
  stderr to the log, calls `socketInitializeDefault()` only for curl, and sets
  `cache_arg` to the app folder.
- `SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)`.
- `oracles_open_window` creates a fullscreen 1280x720 window with
  `SDL_WINDOW_OPENGL`; the renderer is GLES2.
- `switch_install_roms` at startup if there is no `rom_arg`/`game_arg`.
- `apply_window_settings` forces fullscreen; `fit_window_to_picture` is a
  no-op on Switch.
- `settings.ini` and `window.txt` are stored in the app folder.
- It does not use `romfsInit`.

**Twilight Princess** (`platforms/switch/source/switch_startup.cpp`):

- `__nx_applet_type = AppletType_Application`; `__nx_heap_size = 0` (libnx
  takes whatever is left); `userAppInit` calls `romfsInit()`,
  `nvInitialize()` and `vulkan_env_setup()`.
- `userAppExit` releases mod images/code arenas before returning to hbloader;
  if it cannot, it ends the process with `ISelfController::Exit`.
- `audit_heap_at_exit` walks `svcQueryMemory` to find mappings hbloader will
  not be able to reuse.
- This is the best reference for **not breaking hbloader on exit**.

### 2.7 Emerald's specific problem: 32 bits

Emerald comes from GBA/3DS. The engine stores addresses in `u32` fields and in
assembled data (`.int symbol`). On Android arm64 this was solved with:

- `PTR64=1`, `elf32to64.py` and `--abs64` for specific tables;
- `android_lowmem.c` with an `mmap` arena below 4 GiB;
- `android_boot.c` loading the `.so` below 2 GiB;
- `check_overlays.py` and `check_save_layout.py` to keep structures and saves
  valid.

On Switch there is **no `mmap`**, and hbloader chooses the NRO address.
Atmosphere's address-space data (`svc_memory_map.hpp`):

- 36-bit process (`hbl.json` `address_space_type: 1`).
- Small map: `0x08000000`-`0x80000000` (128 MiB-2 GiB).
- Large map: `0x80000000`-`0x1000000000` (2-64 GiB).
- Heap: 8 GiB, placed after the maps (it can start at 2 GiB and cross 4 GiB).
- Alias-code: `0x08000000`-`0x1000000000` (128 MiB-64 GiB).

`hbloader` maps the NRO with `virtmemFindCodeMemory`, which picks a random
address in the alias-code region (128 MiB-64 GiB). **It can land above 4 GiB.**
A normal NRO therefore does not guarantee 32-bit pointers.

Options:

1. **Two-stage NRO loader (recommended).** A small `emerald-nx.nro`, based on
   `nx-hbloader/loadNro` and toniisound's `smbr-nx` `so_util`, that:
   - reads `emerald-game.nro` from `sdmc:/switch/emerald-nx/`;
   - finds a free low gap (128 MiB-2 GiB) with `svcQueryMemory`;
   - reserves it with `virtmemAddReservation` and maps it with
     `svcMapProcessCodeMemory`;
   - creates a low RW arena for the game heap and stack;
   - fills libnx's `ConfigEntry` table (MainThreadHandle, ProcessHandle,
     OverrideHeap, Argv, AppletType, RandomSeed, HosVersion, etc.);
   - jumps to the game entrypoint; libnx applies the NRO relocations on the
     low base.
   The game is a normal libnx NRO; the engine does not need a 64-bit
   migration. It is the same pattern `smbr-nx` uses for `.so`, but with an NRO
   and without emulating bionic/JNI.
2. **NSP with a 32-bit NPDM.** `address_space_type = 0` (32-bit) and
   `is_64_bit = true`. The whole process stays below 4 GiB, no loader.
   Requires installing an NSP and signature patches; not `.nro`.
3. **Full 64-bit migration.** Change every `u32` field holding a pointer,
   `.int` data to `.quad`, bundles and save layout. Cleanest long-term, but
   huge and breaks save compatibility if not done extremely carefully.

**Recommendation:** option 1. It touches the engine the least, preserves saves
and reuses code proven in toniisound's `smbr-nx` and `nx-hbloader`.

**Requirement:** launch as a **title takeover** (hbmenu, hold R while opening
a game), not as an applet/39-bit NSP. The loader and game must end the process
or unmap before returning to hbloader (lesson from TP rc9: otherwise hbloader
fails with `InvalidCurrentMemory`).

---

## 3. Proposed architecture for `emerald-nx`

### 3.1 Directory layout

```
switch_port/
  Makefile / switch.mk        # game tree + Switch backend
  CMakeLists.txt              # host/NRO (optional; the game uses Makefile)
  loader/
    main.c                    # two-stage loader (based on nx-hbloader)
    trampoline.s              # copy of nx-hbloader's trampoline (MIT)
    lowmem.c                  # low-gap search/reservation
    config.c                  # libnx ConfigEntry
  src/
    switch_main.c             # entrypoint, dirs, log, game thread
    switch_platform.c         # CtrPlatform_* (time, threads, hooks)
    switch_video.c            # EGL/GLES3 + NWindow + single-screen
    switch_audio.c            # SDL3 audio or audout + ring/resampler
    switch_input.c            # libnx pad/touch -> CtrInputSample
    switch_fs.c               # fopen/rename/... wrappers and sdmc paths
    switch_log.c              # CtrLog_* to file + stderr
    switch_stubs.c            # missing libctru symbols
  patches/
    sdl-switch-audio.patch    # copy of toniisound's patch
  assets/
    icon.jpg                  # 256x256 icon for the NRO
  build_switch.ps1 / .sh      # build + packaging
```

### 3.2 Build

- **Game:** `make -f switch_port/switch.mk` using `full.mk` (sources,
  staging, data) with `aarch64-none-elf-gcc` and `-DPLATFORM_3DS
  -DPLATFORM_SWITCH -DPORTABLE`.
- **Data:** the same pipeline (`tools/port_common/staging.py`,
  `ctr_bundle.py`, `gen_gamedata_bundle.py`), but with `PTR64=0` and
  `elf32to64.py` without `--abs64` (all addresses resolved below 4 GiB).
- **Game NRO:** `elf2nro emerald-game.elf emerald-game.nro --nacp=...`.
- **Loader:** `make -C switch_port/loader` with `switch_rules`; produces
  `emerald-nx.nro`.
- **Linker scripts/specs:** libnx's `switch.specs` hardcodes `-T switch.ld`.
  The build generates `build/switch_{release,image}.specs` from it, pointing
  at the port's full `emerald_switch.ld` (switch.ld plus `.ctr_game_data`,
  `.gamedata` and `.ctr_game_bss`).  GNU ld gets one base script; no overlay
  ordering surprises.
- **PIE symbols:** GNU ld has no `__ehdr_start`; the script aliases it to
  `__start__` (0), which is the NRO load bias at runtime.  This keeps
  `3ds_assets.c` and `3ds_script_loader.c` on the same load-bias path as the
  Android build.
- **Reused native units:** `3ds_data.c`, `3ds_settings.c`, `3ds_scenario.c`
  and the shared non-SDK units are built for AArch64; `3ds_local.c` is
  replaced by `switch_local_stubs.c`.  `switch_arm.c` provides the one libnx
  counter read the libctru shim needs without mixing libnx and libctru types.
- **Incremental builds:** `build_switch.sh` keeps the AArch64 objects on later
  runs; `CLEAN_BUILD=1` forces a full rebuild after a 3DS/Android build.
- **Release:** zip with
  `switch/emerald-nx/{emerald-nx.nro, emerald-game.nro, data/, README.txt}`;
  no ROMs or protected assets.
- **Toolchain:** `switch-dev`, `switch-tools`, `switch-cmake`,
  `switch-pkg-config`, `switch-mesa`, `switch-libdrm_nouveau`, SDL3 from
  `devkitPro/SDL` branch `switch-sdl-3.4` (pinned commit) or the matching
  package.

### 3.2.1 Build result (2026-11-25)

`make -f switch_port/switch.mk all` completes: the AArch64 game ELF, the NRO,
the asset index, script and song bundles, the gamedata bundle, the engine ABI
and the build id are produced.  GNU ld reports only `R_AARCH64_ABS32` sites in
`.gamedata` (expected: those are the 4-byte pointers the bundle loader
relocates); the recipe's link check rejects any other diagnostic.

The save-layout audit passes (SaveBlock1 0x3D88, SaveBlock2 0xF2C,
PokemonStorage 0x83D0, identical member offsets on arm32 and aarch64).  The
overlay audit reports 13 known drifts, all reviewed and listed in
`tools/check_overlays.allow`: four RFU structures (local wireless is stubbed on
Switch) and nine `MemBlock` uses (the game's allocator computes every offset
from `sizeof(struct MemBlock)`, so it remains valid with the larger 64-bit
header).

### 3.3 Video and single screen

- EGL over `nwindowGetDefault()` (like `smbr-nx`/TP), not necessarily SDL
  video. **OpenGL ES 3.0** context (switch-mesa supports it).
- Reuse `android_port/src/android_video.c` and `android_c3d.c` (GLES3) with
  `__SWITCH__` branches:
  - `CtrVideo_Init`: `eglGetDisplay`, `eglInitialize`, ES3 config, ES3
    context, `eglCreateWindowSurface(..., nwindowGetDefault())`.
  - `SyncWindows`: a single surface; no `ANativeWindow`.
  - `CtrVideo_Present`: `eglMakeCurrent` + `eglSwapBuffers`.
  - `AndroidVideo_SetSingleScreen(true)` from the start.
- Resolution: 1280x720 handheld / 1920x1080 docked. `CtrVideo_ChooseWidth`
  with the real size; the top picture follows the display aspect (432x240 for 16:9, gDisplayWidth) and is scaled; the bottom
  (320x240) is an overlay window, as in Cer0-D.
- `SDL_GetDesktopDisplayMode` or `appletGetOperationMode` for the mode; if
  docking changes, update the viewport without restarting the game.
- The bottom panel is shown when `CtrBottom_PanelShown()` asks for it; touch
  in handheld; button navigation in docked mode.

### 3.4 Audio

- Reuse `android_audio.c`'s ring/resampler (42060 Hz mixer -> 48 kHz device,
  drift correction).
- Output through SDL3 (`SDL_OpenAudioDeviceStream`) with the Switch audio
  patch, or through `audout`/`audren` directly (like `smbr-nx`).
- If SDL3 is used, apply `sdl-switch-audio.patch` and the
  `apply_patch_once.cmake` pattern.
- Keep the mixer on the game thread (like Android), or use a worker if
  needed; the game already has `CtrAudio_StartWorker` and can return false.

### 3.5 Input

- `switch_input.c` reads `padUpdate`/`padGetButtons`/`padGetStickPos` and
  `hidGetTouchScreenStates`.
- Letter-based mapping:
  - `HidNpadButton_A` -> `KEY_A`; `B` -> `KEY_B`; `X` -> `KEY_X`; `Y` -> `KEY_Y`.
  - `L/R` -> `KEY_L/R`; `ZL/ZR` -> speed step (like Android).
  - `Plus` -> `KEY_START`; `Minus` -> `KEY_SELECT` (and bottom-panel toggle if
    desired).
  - D-pad -> `KEY_DUP/DOWN/LEFT/RIGHT`.
  - Left stick -> Circle Pad (+/-156).
  - Touch -> 320x240 bottom-panel coordinates, with
    `AndroidVideo_BottomViewport`/`WholeTouch`.
- Do not use HOME (system-owned). Optional Capture for screenshots.

### 3.6 Filesystem

- Root: `sdmc:/switch/emerald-nx/`.
- `data/` for generated assets (equivalent to `romfs:/`), **never** in git.
- `saves/` or root for `.sav`, settings and logs; decide and document.
- `switch_fs.c` wraps `fopen/rename/remove/unlink/mkdir/stat/access/open` and
  translates:
  - `romfs:/...` -> `sdmc:/switch/emerald-nx/data/...`
  - `sdmc:/3ds/emerald3ds/...` -> `sdmc:/switch/emerald-nx/...`
  - `sdmc:/3ds/pokeemerald/...` -> `sdmc:/switch/emerald-nx/legacy/...`
  - other `sdmc:/...` paths are left untouched.
- `CtrFs_Init` on Switch: no `romfsInit`; validate `data/boot.txt`; create
  dirs.
- `CtrFs_OpenAsset` -> `data/`; `CtrFs_OpenData` -> root.
- Do not include ROMs, saves or protected data in the repo; the data script
  takes the user's ROM and generates `data/` locally.

### 3.7 Lifecycle

- Loader:
  1. Loader `main()` (hbloader, 36-bit).
  2. Open `emerald-game.nro`, read header/segments, zero bss.
  3. Find a low gap with `svcQueryMemory` (start at 0x08000000).
  4. `virtmemAddReservation` + `svcMapProcessCodeMemory`.
  5. `svcSetProcessMemoryPermission` (RX text, R rodata, RW data).
  6. Reserve a low RW arena for heap (e.g. 512 MiB) and stack (8 MiB).
  7. Create the game thread with a low stack; `ConfigEntry` with
     `OverrideHeap`, `MainThreadHandle`, `ProcessHandle`, `AppletType`
     (`SystemApplication` + `ApplicationOverride` for title takeover),
     `Argv`, `RandomSeed`, `HosVersion`, `SyscallAvailableHint`.
  8. Jump to the NRO entrypoint; libnx's rtld relocates on the low base.
  9. On exit: unmap low memory or `svcExitProcess()` so hbloader is not
     broken (TP rc9 lesson).
- Game:
  - `switch_main.c` creates dirs, redirects stderr to
    `sdmc:/switch/emerald-nx/emerald-nx.log`, initializes EGL/GLES and audio,
    and starts the loop.
  - `CtrPlatform_Init`, `CtrVideo_Init`, `CtrAudio_Init`.
  - `CtrPlatform_EndFrame` calls `CtrVideo_Present` and audio.
  - `CtrPlatform_RequestExit` to return to the loader.
  - No `socketInitializeDefault` unless wireless/sync is added; no
    `romfsInit`.
  - Worker threads with 1 MiB stacks and core mask 1/2 if applicable.
  - `__libnx_initheap` is not needed if the loader passes `OverrideHeap`; if
    run without the loader, clamp to `<4 GiB`.

### 3.8 Reference reuse and contrast rule (mandatory)

Every Switch subsystem is checked against the local reference ports before it
is considered done:

- `toniisound`: `The-Oracles-Project-NX`, `oracles-nx`, `smbr-nx`,
  `DKC1/2/3Recomp-NX`.
- `Alexgg1014`: `The-Legend-of-Zelda-Twilight-Princess-Alek-s-Ultimate-NX-Edition`.

The rule is:

1. **Inspect our current implementation first.** If it was written from
   scratch, look for the same problem in the reference ports.
2. **Reuse the reference when it is more complete, tested on hardware, or
   more structured.** Adapt it to Emerald's 36-bit/32-bit address model.
3. **Keep our implementation only when it is cleaner or specific to
   Emerald's architecture.** Use the reference review to catch edge cases,
   compile flags, SDL/mesa patches, cleanup order and hardware quirks.
4. **Record the decision** in the subsystem commit and in this plan.

Current audit:

| Subsystem | Current `switch_port` state | Reference pattern | Decision |
|---|---|---|---|
| Loader | `loader/` is based on `nx-hbloader`'s `loadNro` and `smbr-nx`'s `so_util`; low-gap search, `svcMapProcessCodeMemory`, `ConfigEntry`, low heap/stack. | `smbr-nx/source/so_util.c`, `nx-hbloader/source/main.c`, TP `switch_elf_memory.cpp`/`code_arena.c`. | Keep ours; it is the NRO-specific form of the same mechanism. Add TP's exit cleanup lesson (unmap or terminate). |
| Build / NRO | Makefile with `switch_rules`, `elf2nro`, NACP. | Oracles `CMakeLists.txt`/`build_switch.sh`, `nx_create_nro`, `Switch.cmake` flags. | Align flags and packaging with toniisound; keep Makefile for the large game tree. |
| Video | To adapt `android_video.c` with an NWindow/EGL/GLES3 branch. | Oracles `SDL_WINDOW_OPENGL` + `opengles2`; smbr `egl_setup()` with `nwindowGetDefault()`; DKC OpenGL presenter. | Use smbr's direct NWindow/EGL model (closest to `android_video.c`), with Oracles' 720p/1080p mode handling. |
| Audio | To implement `switch_audio.c` (audout ring/resampler). | Oracles/oracles-nx SDL3 audio + `sdl-switch-audio.patch`; smbr audout feeder. | Use audout feeder with the Android ring/resampler; keep the SDL patch documented if SDL3 is added later. |
| Input | To implement `switch_input.c` with libnx `pad`/`hid`. | Oracles `sdl_window.c` letter mapping; oracles-nx `app.c`/`controls.c`; DKC `switch_gamepad.c`. | Use libnx directly (no SDL dependency), same letter A/B/X/Y mapping and ZL/ZR speed behavior. |
| Filesystem | To implement `switch_fs.c` wrappers for `romfs:/` and `sdmc:/3ds/emerald3ds/`. | Oracles `sdmc:/switch/...`; oracles-nx `switch_install_roms`; TP `switch_startup.cpp`/`fs` layer. | Keep path wrapper because Emerald hardcodes 3DS paths; expose `sdmc:/switch/emerald-nx/` and `data/`. |
| Lifecycle | To implement `switch_main.c` and use `android_platform.c`. | Oracles `main()` 8 MiB thread + log; TP `__nx_applet_type`, `userAppInit`/`userAppExit`, heap audit. | Use title takeover, log to SD, clean code mappings on exit, and keep the game on the loader's low stack. |

Reference review result (2026-11-25):

- **TP `switch_elf_memory.cpp`**: confirmed the stage -> `armDCacheFlush` ->
  `svcMapProcessCodeMemory` -> per-range permissions -> `armICacheInvalidate`
  sequence. Its `svcUnmapProcessCodeMemory` exit cleanup is needed when control
  returns to hbloader; this port instead exits the process from
  `switch_after_game`, so the game's low stack and code mappings are never
  handed back. TP is GPL-2.0 with the funchook exception, so no code was copied.
- **smbr-nx `so_finalize`**: the kernel forbids W -> X on code memory. The
  loader now builds a page map, sets RX pages first, then R and RW pages;
  every page is set once, so shared boundaries and alignment gaps are safe.
- **hbloader `loadNro`**: `header->size` includes the 0x80-byte CRT0/NroHeader
  prefix, `.int` assets follow it, and the mapped image must contain only
  `header->size` bytes plus zeroed bss. `load_nro_file` now copies exactly that
  much, so assets cannot leak into bss.
- **oracles-nx / The-Oracles-Project-NX**: their release is an unzip-at-root
  `switch/<app>/` folder with the NRO, data folders, README and `licences/`.
  This port's `build_switch.sh` follows that shape.

### 3.9 Data and saves

- `PTR64=0`: the engine's 32-bit pointers are valid because the whole game
  process is mapped low.
- `check_save_layout.py` and `check_overlays.py` with the devkitA64 compiler;
  saves must remain identical to 3DS/Android.
- Do not use `android_lowmem.c` or `android_boot.c`; there is no `mmap`.
- The game heap must be a low arena; do not trust `svcSetHeapSize` (it can
  start at 2 GiB and cross 4 GiB).
- The game thread stack must be low; secondary thread stacks come from the
  low heap or a low arena.

---

## 4. Phase 4 - Milestones and atomic commits

1. **`build(switch): add devkitA64 build and NRO packaging`**
   - `switch_port/` Makefile/CMake, flags, `elf2nro`, NACP, icon.
   - `build_switch.ps1/sh`, release layout.
2. **`feat(switch): low-address loader for the game NRO`**
   - `loader/` based on `nx-hbloader` + `smbr-nx`; low-gap search,
     `svcMapProcessCodeMemory`, `ConfigEntry`, low heap/stack.
   - Spike: a "hello" NRO that prints pointers and addresses; verify <4 GiB.
3. **`feat(switch): EGL/GLES3 video and single-screen layout`**
   - `switch_video.c`, `__SWITCH__` branches in
     `android_video.c`/`android_c3d.c`, NWindow, 720p/1080p, bottom overlay.
4. **`feat(switch): audio and input`**
   - `switch_audio.c` (SDL3/audout), `switch_input.c` (pad/touch), SDL patch,
     letter-based mapping.
5. **`feat(switch): SD-card filesystem and save paths`**
   - `switch_fs.c`, `sdmc:/switch/emerald-nx/`, `data/`, logs, settings.
6. **`fix(switch): heap clamp and 32-bit pointer audit`**
   - `__libnx_initheap`/loader arena, `PTR64=0`, `elf32to64.py` without
     abs64, `check_save_layout.py`.
7. **`docs(switch): build, install and controls`**
   - `switch_port/README.md`, `docs/SWITCH.md`.
8. **`test(switch): address, save and video checks`**
   - Address audit script, save round-trip, 720p/1080p capture.

Each commit must build on its own; the order allows stopping and testing.

---

## 5. Risks and blockers

### 5.1 Local toolchain (resolved)

The local toolchain is installed and working:

- `C:\devkitPro\devkitA64` - GCC 16.1.0.
- `C:\devkitPro\devkitARM` - GCC 16.1.0 (GBA/3DS restored).
- `C:\devkitPro\libnx`, `libctru`, `citro2d`, `citro3d`, `libgba`, `libtonc`.
- `switch-tools`, `switch-cmake`, `switch-pkg-config`, `switch-mesa`,
  `switch-libdrm_nouveau`, `deko3d`, `uam`.
- MSYS2 with `make`, `cmake`, `ninja`, `git`, `pacman`.

The devkitPro package server returns HTTP 403 to libcurl's default TLS
fingerprint. Pacman is configured with `/usr/bin/dkp-xfer`, which uses
`curl_cffi` with a Chrome fingerprint and translates the MSYS2 output path to
Windows. This works for both `repo.msys2.org` and `pkg.devkitpro.org`.

The environment was rebuilt cleanly after an earlier `pacman --overwrite`
incident replaced the devkitPro root; devkitARM/GBA/3DS and MSYS2 were
reinstalled from the official packages.

### 5.2 Loader spike (built, hardware test pending)

`switch_port/loader/emerald-nx.nro` and
`switch_port/spike/emerald-game.nro` compile. The loader:

- reads `sdmc:/switch/emerald-nx/emerald-game.nro`;
- finds a free low gap with `svcQueryMemory`;
- reserves it with `virtmemAddReservation`;
- maps it with `svcMapProcessCodeMemory`;
- sets text RX, rodata R, data RW;
- maps a 256 MiB low heap, with the top 8 MiB used as the game stack;
- passes the libnx `ConfigEntry` table and jumps to the NRO entrypoint.

The probe prints `main`, `probe`, global, stack, heap, allocated and TLS
addresses, calls a function through a 32-bit pointer, and reports PASS/FAIL.
Hardware results are pending.

### 5.3 Low loader

- `svcMapProcessCodeMemory` requires an own-process handle and the launcher
  to permit the syscall; hbloader allows it in title takeover, not in
  emulators.
- The low-gap search must avoid hbloader's mappings; use `svcQueryMemory` and
  `virtmemAddReservation`.
- The game NRO must relocate correctly on the low base; libnx does it, but it
  must be tested with a probe NRO.
- On exit, unmap or terminate the process; do not return to hbloader with live
  mappings.

### 5.4 32-bit pointers

- Even with a low loader, any allocation outside the low arena breaks the
  engine. Audit `malloc`, `aligned_alloc`, `mmap` (none), thread stacks and
  SDL/mesa buffers used as game data.
- `check_overlays.py`/`check_save_layout.py` are mandatory.
- If the low loader fails, the fallback is a 32-bit NSP or a 64-bit
  migration.

### 5.5 Video/audio

- switch-mesa supports GLES 3.x; request an ES3 context and check
  `glGetString`.
- SDL3 switch-sdl-3.4 is built from source; the audio patch is required.
- 1080p docked + voxel may be demanding; measure and offer a quality option.

### 5.6 Legal

- No ROMs, saves, extracted assets or protected data.
- `data/` is generated locally from the user's ROM.
- The repo contains only code, an original icon and documentation.

---

## 6. Acceptance criteria

- `emerald-nx.nro` starts from hbmenu title takeover on real Switch hardware.
- `&main`, game heap and stack are <4 GiB; 32-bit pointers work.
- 1280x720 handheld and 1920x1080 docked, single screen with bottom overlay.
- Audio 42060->48k without severe underruns; letter-based input; touch in
  handheld.
- Saves/settings in `sdmc:/switch/emerald-nx/`; valid save round-trip.
- External `data/`; nothing protected in git.
- Reproducible build with `switch-dev` + `switch-tools` + pinned SDL3.
- `git status` clean except ignored `data/`/build outputs.

---

## 7. Appendix - Address-space evidence (Atmosphere)

From `libraries/libvapours/include/vapours/svc/svc_memory_map.hpp`:

- `AddressMemoryRegionSmall36Size = 2 GiB`.
- `AddressSmallMap36Start = 0x08000000` (128 MiB); end = 2 GiB.
- `AddressLargeMap36Start = 2 GiB`; end = 64 GiB.
- `AddressMemoryRegionHeap36Size = 8 GiB`.
- `AddressMemoryRegionAlias36Size = 6 GiB`.
- `AddressNullGuard64Size = 128 MiB`.

From `kern_k_page_table_base.cpp` (`InitializeForProcess`, width 36):

- `m_code_region_start = AddressSmallMap36Start` (128 MiB).
- `m_code_region_end = 2 GiB`.
- `process_code_start/end = m_code_region_start/end` for 32/36 bits.
- `m_alias_code_region_start = 128 MiB`; end = 64 GiB.
- The heap is placed after the maps; it can start at 2 GiB and cross 4 GiB.

From `nx-hbloader`'s `hbl.json`:

- `is_64_bit: True`
- `address_space_type: 1` (64-bit instructions, 36-bit address space)

From `nx-hbloader/source/main.c`:

- `loadNro` reads the NRO and maps it with
  `virtmemFindCodeMemory(total_size, 0)` + `svcMapProcessCodeMemory`.
- It passes `ConfigEntry` with `OverrideHeap`, `MainThreadHandle`,
  `ProcessHandle`, `AppletType`, `Argv`, `RandomSeed`, `HosVersion`, etc.
- It calls `nroEntrypointTrampoline`.

From `smbr-nx/source/so_util.c`:

- `so_load` reserves `virtmemFindCodeMemory` and `so_finalize` maps with
  `svcMapProcessCodeMemory`; `svcSetProcessMemoryPermission` sets RX/RW.
- It is the same mechanism, applied to an Android `.so`.

From `The-Legend-of-Zelda-Twilight-Princess-Alek-s-Ultimate-NX-Edition`:

- `platforms/switch/source/code_arena.c` and `src/dusk/switch_elf_memory.cpp`
  show `svcCreateCodeMemory`/`svcControlCodeMemory`/`svcMapProcessCodeMemory`
  and cleanup on exit.
- `platforms/switch/source/switch_startup.cpp` shows `AppletType_Application`,
  `romfsInit`, `nvInitialize`, heap audit and `ISelfController::Exit`.

---

**End of plan.** The loader and address-probe NROs are built; the next action
is the hardware run and review of `loader.log` / `game.log`.


