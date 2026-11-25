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
PYTHON="${PYTHON:-C:/Users/Felipe/AppData/Local/Programs/Python/Python313/python.exe}"
export DEVKITPRO="${DEVKITPRO:-C:/devkitPro}"
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

make -C "$TREE" -f "$ROOT/switch_port/switch.mk" -j"$JOBS" PYTHON="$PYTHON"

DIST="$ROOT/switch_port/dist"
APP="$DIST/switch/emerald-nx"
rm -rf "$DIST"
mkdir -p "$APP/data"

cp "$ROOT/switch_port/loader/emerald-nx.nro" "$APP/emerald-nx.nro"
cp "$TREE/emerald-game.nro" "$APP/emerald-game.nro"
cp -a "$TREE/romfs/." "$APP/data/"
cp "$ROOT/switch_port/PACKAGE_README.txt" "$APP/README.txt"
mkdir -p "$APP/licenses"
cp "$ROOT/LICENSE-PORT.md" "$APP/licenses/LICENSE-PORT.txt"
cp "$ROOT/NOTICE.md" "$APP/licenses/NOTICE.txt"

( cd "$DIST" && rm -f ../emerald-nx.zip && zip -qr ../emerald-nx.zip switch )

echo
echo "Done: $DIST/switch/emerald-nx/"
echo "Zip:  $ROOT/switch_port/dist/emerald-nx.zip"
echo "Copy the zip's switch/ folder to the SD card root."
