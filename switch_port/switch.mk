# Emerald NX game build: the same game tree and bridge as the 3DS/Android
# builds, compiled with devkitA64 for a 36-bit Horizon homebrew process.
#
# Run from a bootstrapped tree's 3ds_port directory:
#   make -C build/upstream/3ds_port -f <repo>/switch_port/switch.mk -j2 \
#        PYTHON=<python3.13>
#
# The loader maps the resulting emerald-game.nro below 4 GiB.  The game keeps
# 32-bit addresses in its data, so PTR64 stays 0 and all code/data/heap/stack
# must remain in the low range.

SHELL := bash
.SHELLFLAGS := -o pipefail -c
.DEFAULT_GOAL := all
.SUFFIXES:

SWITCH_PORT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
ANDROID_PORT := $(abspath $(SWITCH_PORT)/../android_port)
DEVKITPRO ?= C:/devkitPro
DEVKITARM ?= $(DEVKITPRO)/devkitARM
DEVKITA64 ?= $(DEVKITPRO)/devkitA64
CC := $(DEVKITA64)/bin/aarch64-none-elf-gcc
CXX := $(DEVKITA64)/bin/aarch64-none-elf-g++
AR := $(DEVKITA64)/bin/aarch64-none-elf-gcc-ar
OBJCOPY := $(DEVKITARM)/bin/arm-none-eabi-objcopy
READELF := $(DEVKITARM)/bin/arm-none-eabi-readelf
STRIP := $(DEVKITA64)/bin/aarch64-none-elf-strip
HOSTCC ?= gcc
PYTHON ?= python

VOXEL_LIGHTING ?= 1
SHOW_FPS ?= 1
PORT_LANG ?= en
CTR_LANG_PT_BR := $(if $(filter pt_br,$(PORT_LANG)),1,0)
DATA_BACKEND ?= 1

# The bundle/asset scripts read the AArch64 executable with LLVM-compatible
# tool names; these are copies of devkitA64's readelf/nm/objcopy.
export CTR_PTR64 := 0
export CTR_BUNDLE_LOCALS := 1
export CTR_LLVM_BIN := $(SWITCH_PORT)/tools/elfbin

ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CPPFLAGS := -DPORTABLE -DPLATFORM_3DS -DPLATFORM_SWITCH -D__SWITCH__ -Iinclude \
	-Isrc/voxel -I$(SWITCH_PORT)/include -I$(ANDROID_PORT)/include \
	-I$(DEVKITPRO)/portlibs/switch/include -I$(DEVKITPRO)/libnx/include
CFLAGS := $(ARCH) -O2 -g -std=gnu99 -fPIE -ffunction-sections -fdata-sections -Wall -Wextra
NATIVE_CFLAGS := $(CFLAGS)
GAMECFLAGS := $(CFLAGS) -fsigned-char -fno-strict-aliasing -w
GAMECFLAGS += -Wno-error=incompatible-pointer-types -Wno-error=int-conversion \
	-Wno-error=implicit-function-declaration -Wno-error=implicit-int \
	-Wno-error=return-type
GAMEFLAGS := -DPORTABLE -DPLATFORM_3DS -DPLATFORM_SWITCH -DMODERN=1 -DUBFIX -D__INTELLISENSE__
GAMEFLAGS += -iquote include -iquote ../include -iquote ../include/constants -iquote ..
GAMEFLAGS += -DCTR_VOXEL_LIGHTING=$(VOXEL_LIGHTING) -DCTR_SHOW_FPS=$(SHOW_FPS) -DCTR_LANG_PT_BR=$(CTR_LANG_PT_BR)
CPPFLAGS += -DCTR_VOXEL_LIGHTING=$(VOXEL_LIGHTING) -DCTR_SHOW_FPS=$(SHOW_FPS) -DCTR_LANG_PT_BR=$(CTR_LANG_PT_BR)
CPPFLAGS += -DCTR_DATA_BACKEND=$(DATA_BACKEND)

# The shared full.mk rules (asset table, script/song bundles, gamedata) name
# the executable $(TARGET).elf and default their ELF argument to
# emerald3ds.elf, as on 3DS/Android.  Keep that target name and give the NRO
# the loader-facing name.
TARGET := emerald3ds
GAME_NRO := emerald-game.nro
APP_TITLE := Emerald NX
APP_AUTHOR := Zallax / community
APP_VERSION := 0.5.0-switch
ROOT := ..
SCRIPTS := ../tools/port_common
EXESUFFIX :=
BOTTOM_ART_FLAGS := --height 240

LIBS := -L$(DEVKITPRO)/portlibs/switch/lib -L$(DEVKITPRO)/libnx/lib \
	-lEGL -lGLESv2 -lglapi -ldrm_nouveau -lnx -lstdc++ -lm
FS_WRAPPED := fopen rename remove unlink mkdir stat access open
# libnx's switch.specs hardcodes -T switch.ld.  The port's script adds the
# game-data sections to that base script, so the build uses a local copy of
# the specs that names the generated script instead (GNU ld treats the last
# non-INSERT script as the base; keeping the original in front would merge
# both layouts).
comma := ,
# The game's assembled data keeps 4-byte absolute addresses (the image stays
# below 4 GiB).  GNU ld refuses R_AARCH64_ABS32 in a PIE, but every such site
# is in .gamedata, which is NOLOAD in the release link and filled/relocated by
# the bundle loader.  --noinhibit-exec keeps the image the bundle script needs;
# a link check rejects any diagnostic other than those .gamedata ABS32 sites.
LDFLAGS_BASE := $(ARCH) -specs=build/switch_release.specs -Wl,--gc-sections \
	-Wl,-z,notext -Wl,--noinhibit-exec -Wl$(comma)-Map$(comma)build/$(TARGET).map
LDFLAGS := $(LDFLAGS_BASE)
LDFLAGS += $(foreach f,$(FS_WRAPPED),-Wl,--wrap=$(f))

.PHONY: FORCE
build/config: FORCE
	@mkdir -p build
	@printf '%s\n' '$(CPPFLAGS) $(CFLAGS) $(GAMEFLAGS) $(GAMECFLAGS)' > build/config.new
	@cmp -s build/config.new $@ || cp build/config.new $@
	@rm -f build/config.new

include full.mk

# The image link must see every wrap and --undefined that full.mk added to
# LDFLAGS, or the two links would not lay the same symbols out.  Swap only
# the specs file and the map name.
LDFLAGS_IMAGE := $(filter-out -specs=build/switch_release.specs,$(LDFLAGS))
LDFLAGS_IMAGE := $(filter-out -Wl$(comma)-Map$(comma)build/$(TARGET).map,$(LDFLAGS_IMAGE))
LDFLAGS_IMAGE += -specs=build/switch_image.specs -Wl,-Map,build/$(TARGET)-image.map

# Shared backend units that include libctru's <3ds.h> use the Switch shim.
FULLFLAGS += -I$(SWITCH_PORT)/include

# Portable native units of the 3DS backend, as the Android build reuses them.
# 3ds_local.c is the exception: Switch has its own LocalLink stubs.
NATIVE_SRCS := src/3ds_input_decode.c src/3ds_pak.c src/3ds_daynight.c src/3ds_clock_store.c \
	src/3ds_nuzlocke_core.c src/3ds_nuzlocke_store.c src/3ds_encounters.c src/3ds_field_view.c \
	src/3ds_corner_art.c src/3ds_icons_ptbr.c src/3ds_video_decode.c \
	src/3ds_data.c src/3ds_settings.c src/3ds_scenario.c
NATIVE_OBJS := $(patsubst src/%.c,build/%.o,$(NATIVE_SRCS))

build/%.o: src/%.c $(wildcard $(SWITCH_PORT)/include/*.h) $(wildcard $(ANDROID_PORT)/include/*.h)
	@mkdir -p $(dir $@)
	"$(CC)" $(CPPFLAGS) $(NATIVE_CFLAGS) -MMD -MP -c $< -o $@

# Game backend sources: the Android single-screen backend adapted to Switch.
SWITCH_SRCS := \
	$(SWITCH_PORT)/src/switch_main.c \
	$(SWITCH_PORT)/src/switch_arm.c \
	$(SWITCH_PORT)/src/switch_lifecycle.c \
	$(SWITCH_PORT)/src/switch_host.c \
	$(SWITCH_PORT)/src/switch_audio.c \
	$(SWITCH_PORT)/src/switch_input.c \
	$(SWITCH_PORT)/src/switch_fs.c \
	$(SWITCH_PORT)/src/switch_log.c \
	$(SWITCH_PORT)/src/switch_local_stubs.c \
	$(ANDROID_PORT)/src/android_video.c \
	$(ANDROID_PORT)/src/android_c3d.c \
	$(ANDROID_PORT)/src/android_platform.c \
	$(ANDROID_PORT)/src/android_input.c \
	$(ANDROID_PORT)/src/android_stubs.c
SWITCH_OBJS := $(patsubst $(SWITCH_PORT)/src/%.c,build/switch/%.o,$(filter $(SWITCH_PORT)/src/%,$(SWITCH_SRCS)))
SWITCH_OBJS += $(patsubst $(ANDROID_PORT)/src/%.c,build/switch/%.o,$(filter $(ANDROID_PORT)/src/%,$(SWITCH_SRCS)))

build/switch/%.o: $(SWITCH_PORT)/src/%.c $(wildcard $(SWITCH_PORT)/include/*.h)
	@mkdir -p $(dir $@)
	"$(CC)" $(CPPFLAGS) $(NATIVE_CFLAGS) -MMD -MP -c $< -o $@

build/switch/%.o: $(ANDROID_PORT)/src/%.c $(wildcard $(ANDROID_PORT)/include/*.h) $(wildcard $(SWITCH_PORT)/include/*.h)
	@mkdir -p $(dir $@)
	"$(CC)" $(CPPFLAGS) $(NATIVE_CFLAGS) -MMD -MP -c $< -o $@

GAME_OBJS := $(FULL_C_OBJS) $(FULL_DATA_OBJS) $(BACKEND_OBJS)
RW_DATA_OBJS := $(patsubst %,build/rw/%,$(FULL_DATA_OBJS))

build/rw/%.o: %.o $(ANDROID_PORT)/tools/rw_data.sh $(ANDROID_PORT)/tools/elf32to64.py
	bash $(ANDROID_PORT)/tools/rw_data.sh "$(OBJCOPY)" "$(READELF)" $< $@
	"$(PYTHON)" $(ANDROID_PORT)/tools/elf32to64.py $@ $@.64
	mv $@.64 $@

build/buildinfo.c: FORCE
	@mkdir -p build
	@sha=$$(git -C $(ANDROID_PORT) rev-parse HEAD 2>/dev/null || echo unknown); \
	{ printf 'const char gCtrBuildVersion[] = "%s";\n' '$(APP_VERSION)'; \
	  printf 'const char gCtrBuildCommit[] = "%s";\n' "$$sha"; \
	  printf 'const char gCtrBuildBranch[] = "switch";\n'; \
	  printf 'const unsigned gCtrBuildDirty = 0;\n'; \
	  printf 'const char gCtrBuildDiff[] = "none";\n'; \
	  printf 'const char gCtrBuildLibs[] = "libnx";\n'; \
	  printf 'const char gCtrBuildDefines[] = "PLATFORM_SWITCH";\n'; \
	  printf 'const char gCtrBuildCompiler[] = __VERSION__;\n'; \
	  printf 'const char gCtrLinkId[] = "E3ID-switch";\n'; } > $@.new
	@cmp -s $@.new $@ || cp $@.new $@
	@rm -f $@.new
build/buildinfo.o: build/buildinfo.c build/config
	"$(CC)" $(CFLAGS) -c $< -o $@

$(NATIVE_OBJS) $(GAME_OBJS) $(SWITCH_OBJS) $(RW_DATA_OBJS): build/config

LINK_OBJS := $(filter-out $(FULL_DATA_OBJS),$(GAME_OBJS)) $(RW_DATA_OBJS) \
	$(NATIVE_OBJS) $(SWITCH_OBJS) build/buildinfo.o

LD_WRITABLE_DATA := .data .data.[!r]* .data.r[!e]* .data.re[!l]* .data.rel[!.]* .data.rel .data.rel.[!r]* .data.rel.r[!o]*
build/emerald_switch_release.ld: $(SWITCH_PORT)/emerald_switch.ld $(SWITCH_PORT)/switch.mk
	@mkdir -p build
	sed -e 's/@GAMEDATA_TYPE@/(NOLOAD)/' -e 's/CTR_WRITABLE_DATA/$(LD_WRITABLE_DATA)/' $< > $@
build/emerald_switch_image.ld: $(SWITCH_PORT)/emerald_switch.ld $(SWITCH_PORT)/switch.mk
	@mkdir -p build
	sed -e 's/@GAMEDATA_TYPE@//' -e 's/CTR_WRITABLE_DATA/$(LD_WRITABLE_DATA)/' $< > $@
build/switch_release.specs: $(DEVKITPRO)/libnx/switch.specs $(SWITCH_PORT)/switch.mk
	@mkdir -p build
	sed 's|%:getenv(DEVKITPRO /libnx/switch.ld)|build/emerald_switch_release.ld|' $< > $@
build/switch_image.specs: $(DEVKITPRO)/libnx/switch.specs $(SWITCH_PORT)/switch.mk
	@mkdir -p build
	sed 's|%:getenv(DEVKITPRO /libnx/switch.ld)|build/emerald_switch_image.ld|' $< > $@

build/gamedata_image.elf: $(LINK_OBJS) build/emerald_switch_image.ld build/switch_image.specs
	"$(CC)" $(LDFLAGS_IMAGE) -Wl,--emit-relocs $(LINK_OBJS) $(LIBS) -o $@ 2> build/link-image.err
	@if grep -E 'error|undefined reference|multiple definition' build/link-image.err \
		| grep -v 'R_AARCH64_ABS32' | grep -q .; then \
		cat build/link-image.err; exit 1; fi

$(TARGET).elf: $(LINK_OBJS) build/emerald_switch_release.ld build/switch_release.specs
	"$(CC)" $(LDFLAGS) $(LINK_OBJS) $(LIBS) -o $@ 2> build/link-release.err
	@if grep -E 'error|undefined reference|multiple definition' build/link-release.err \
		| grep -v 'R_AARCH64_ABS32' | grep -q .; then \
		cat build/link-release.err; exit 1; fi

# The linker creates the map as a byproduct of the release link.
build/$(TARGET).map: $(TARGET).elf ;

$(TARGET).nacp: Makefile
	$(DEVKITPRO)/tools/bin/nacptool --create "$(APP_TITLE)" "$(APP_AUTHOR)" "$(APP_VERSION)" $@

$(GAME_NRO): $(TARGET).elf $(TARGET).nacp $(SWITCH_PORT)/icon.jpg
	$(DEVKITPRO)/tools/bin/elf2nro $< $@ --icon=$(SWITCH_PORT)/icon.jpg --nacp=$(TARGET).nacp

# Data staging: engine files and game data both live under data/ on the SD
# card and are read through the romfs:/ path wrapper.
ROMFS_SWITCH := $(ROMFS_ASSET_OUTS) $(ROMFS_MAP_OUT) $(ROMFS_SCRIPT_OUTS) $(ROMFS_SOUND_OUTS) \
	romfs/bottom/battle.bin romfs/bottom/menu.bin romfs/battle/hud.bin romfs/stage/leaves.bin \
	$(ROMFS_SHADER_OUTS) $(ROMFS_GAMEDATA_OUTS)

romfs/shaders/voxel.shbin: $(SWITCH_PORT)/switch.mk FORCE
	@mkdir -p $(dir $@)
	printf 'GLES\nvoxel\nweather\n' > $@
romfs/data.embedded:
	printf 'embedded\n' > $@
romfs/engine/abi.bin: $(ROMFS_SWITCH) $(SCRIPTS)/staging.py
	"$(PYTHON)" $(SCRIPTS)/staging.py abi --romfs romfs
romfs/engine/build_id.bin: $(TARGET).elf romfs/engine/abi.bin scripts/gen_build_id.py
	"$(PYTHON)" scripts/gen_build_id.py $(TARGET).elf romfs/engine/abi.bin $@

.PHONY: all romfs-data clean check-save-layout check-overlays
all: $(GAME_NRO) romfs-data

# Layout audits, run by hand (the overlay check preprocesses every game unit
# and takes minutes).  The save check compares devkitARM's 32-bit layout with
# devkitA64's; llvm-dwarfdump comes from MSYS2's LLVM package.
DWARFDUMP ?= llvm-dwarfdump
check-save-layout:
	"$(PYTHON)" $(SWITCH_PORT)/tools/check_save_layout_switch.py \
		"$(DEVKITARM)/bin/arm-none-eabi-gcc.exe" \
		"$(DEVKITA64)/bin/aarch64-none-elf-gcc.exe" \
		"$(DWARFDUMP)" "$(abspath $(ROOT))" build/save_layout_switch.ok
check-overlays:
	"$(PYTHON)" $(SWITCH_PORT)/tools/check_overlays_switch.py --jobs 1 \
		. "$(SWITCH_PORT)/switch.mk" build/overlays_switch.ok
romfs-data: $(ROMFS_SWITCH) romfs/engine/abi.bin romfs/engine/build_id.bin romfs/data.embedded

clean:
	rm -rf build $(TARGET).elf $(GAME_NRO) $(TARGET).nacp $(TARGET).map romfs
