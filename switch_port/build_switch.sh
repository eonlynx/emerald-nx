#!/usr/bin/env bash
# Build the complete Emerald NX SD-card tree.
#
# Requirements:
#   - devkitPro at /opt/devkitpro (devkitA64, libnx, devkitARM, switch-tools,
#     switch-mesa, switch-libdrm_nouveau)
#   - Python 3.13 with Pillow
#   - a bootstrapped tree at build/upstream (python tools/bootstrap.py)
#
# Output:
#   switch_port/dist/switch/emerald-nx/{emerald-nx.nro,emerald-game.nro,data/}
#   switch_port/dist/emerald-nx.zip
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TREE="${TREE:-$ROOT/build/upstream/3ds_port}"
JOBS="${JOBS:-2}"
# One emerald3ds.pak is the release default; DATA_BACKEND=1 keeps the old
# loose data/ tree for development.
DATA_BACKEND="${DATA_BACKEND:-3}"
PYTHON="${PYTHON:-python3}"
export DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}"
export DEVKITARM="${DEVKITARM:-$DEVKITPRO/devkitARM}"
export DEVKITA64="${DEVKITA64:-$DEVKITPRO/devkitA64}"
export VOXEL_JOBS=1
export PATH="$DEVKITPRO/msys2/mingw64/bin:$DEVKITPRO/msys2/usr/bin:$DEVKITPRO/tools/bin:$DEVKITARM/bin:$DEVKITA64/bin:$PATH"

if [ ! -d "$TREE" ]; then
    echo "build_switch: missing bootstrapped tree $TREE" >&2
    echo "run: python tools/bootstrap.py" >&2
    exit 1
fi

echo "== loader =="
make -C "$ROOT/switch_port/loader" -j"$JOBS"

echo "== game and data =="
# The 3DS build may have left AArch32 objects in build/.  The data under
# romfs/ is useful, so preserve it, drop only the objects, and let the Switch
# build make its own AArch64 objects.  On later runs the AArch64 objects are
# kept; pass CLEAN_BUILD=1 to force a full rebuild.  The one 3DS-only payload
# is the PICA shader: the Switch renderer makes its GLES programs at runtime,
# so replace the marker rather than ship the 3DS binary.
if [ "${CLEAN_BUILD:-0}" = "1" ]; then
    rm -rf "$TREE/build"
fi
rm -f "$TREE/romfs/shaders/voxel.shbin"
if [ "$DATA_BACKEND" = "3" ]; then
    # A marker left by an older loose/romfs build would put the engine in the
    # wrong backend; the pak build does not need it.
    rm -f "$TREE/romfs/data.embedded"
fi

make -C "$TREE" -f "$ROOT/switch_port/switch.mk" -j"$JOBS" PYTHON="$PYTHON" \
    DATA_BACKEND="$DATA_BACKEND"

DIST="$ROOT/switch_port/dist"
APP="$DIST/switch/emerald-nx"
rm -rf "$DIST"
mkdir -p "$APP/data"

cp "$ROOT/switch_port/loader/emerald-nx.nro" "$APP/emerald-nx.nro"
cp "$TREE/emerald-game.nro" "$APP/emerald-game.nro"
if [ "$DATA_BACKEND" = "3" ]; then
    # The pack carries the 6,400 game data files.  data/ keeps only the small
    # engine files the engine opens directly (ABI, asset index, .rel tables,
    # port art).
    "$PYTHON" "$ROOT/tools/port_common/staging.py" engine \
        --romfs "$TREE/romfs" --out "$APP/data"
    rm -f "$APP/data/data.embedded"
    cp "$TREE/build/emerald3ds.pak" "$APP/emerald3ds.pak"
else
    cp -a "$TREE/romfs/." "$APP/data/"
fi
cp "$ROOT/switch_port/PACKAGE_README.txt" "$APP/README.txt"
mkdir -p "$APP/licences"
cp "$ROOT/LICENSE-PORT.md" "$APP/licences/LICENSE-PORT.txt"
cp "$ROOT/NOTICE.md" "$APP/licences/NOTICE.txt"

( cd "$DIST" && rm -f emerald-nx.zip && zip -qr9 emerald-nx.zip switch )

echo
echo "Done: $DIST/switch/emerald-nx/"
echo "Zip:  $ROOT/switch_port/dist/emerald-nx.zip"
echo "Copy the zip's switch/ folder to the SD card root."
