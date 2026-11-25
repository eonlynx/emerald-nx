#!/usr/bin/env python3
"""Fail the build if the save's layout differs between 32-bit ARM and AArch64.

This is the Switch port's version of android_port/tools/check_save_layout.py.
The Android tool compiles with the NDK's clang and dumps DWARF with
llvm-dwarfdump; the Switch build uses devkitARM and devkitA64 GCC, then reads
the same debug information with llvm-dwarfdump (from MSYS2's LLVM package).

The save file is the save blocks' bytes as they are in memory, so a save
written by the GBA, an emulator, the 3DS port or the Android port can be read
only while every member of those structures sits at the same offset. Pointers
are what move on a 64-bit build; the saved ones are 4 bytes wide (u32, PTR32
in global.h), and this checks nothing else slipped in.

  check_save_layout_switch.py ARM_GCC A64_GCC DWARFDUMP GAME_ROOT STAMP
"""

from __future__ import annotations

import re
import subprocess
import sys
import tempfile
from pathlib import Path

# The GBA's sizes (pokeemerald built by agbcc).
SAVED = {"SaveBlock1": 0x3D88, "SaveBlock2": 0xF2C, "PokemonStorage": 0x83D0}

UNIT = """#include "global.h"
#include "pokemon_storage_system.h"
struct SaveBlock1 gCheckSaveBlock1;
struct SaveBlock2 gCheckSaveBlock2;
struct PokemonStorage gCheckPokemonStorage;
"""


def layout(compiler: Path, dwarfdump: Path, root: Path, flags: list[str],
           name: str, work: Path) -> dict[str, list[tuple[str, int]]]:
    source = work / "save_layout.c"
    obj = work / ("save_layout_%s.o" % name)
    source.write_text(UNIT)
    subprocess.run([str(compiler), *flags, str(source), "-o", str(obj)], check=True)
    dump = subprocess.run([str(dwarfdump), "--debug-info", str(obj)],
                          check=True, capture_output=True, text=True).stdout

    # Every DIE: its tag, name, member offset, type reference and size, and
    # its parent - dwarfdump indents a DIE's children under it.
    dies: dict[int, dict] = {}
    children: dict[int, list[int]] = {}
    stack: list[tuple[int, int]] = []  # (indent, DIE)
    current = None
    for line in dump.splitlines():
        m = re.match(r"^0x([0-9a-f]+):(\s+)(DW_TAG_\w+|NULL)", line)
        if m:
            addr, indent, tag = int(m.group(1), 16), len(m.group(2)), m.group(3)
            while stack and stack[-1][0] >= indent:
                stack.pop()
            if tag == "NULL":
                current = None
                continue
            current = {"tag": tag}
            dies[addr] = current
            if stack:
                children.setdefault(stack[-1][1], []).append(addr)
            stack.append((indent, addr))
            continue
        if current is None:
            continue
        m = re.match(r"\s+(DW_AT_\w+)\s+\((.*)\)$", line)
        if not m:
            continue
        attr, value = m.groups()
        if attr == "DW_AT_name":
            current["name"] = value.strip('"')
        elif attr == "DW_AT_data_member_location":
            try:
                current["offset"] = int(value, 0)
            except ValueError:
                # GCC may emit a location expression for a member offset.
                m2 = re.search(r"(?:DW_OP_plus_uconst\s+)?(0x[0-9a-fA-F]+|\d+)", value)
                if m2:
                    current["offset"] = int(m2.group(1), 0)
        elif attr == "DW_AT_type":
            current["type"] = int(value.split()[0], 16)
        elif attr == "DW_AT_byte_size":
            current["size"] = int(value, 0)
        elif attr == "DW_AT_data_bit_offset":
            current["bits"] = int(value, 0)
    order = sorted(dies)

    def resolve(ref: int) -> int:
        """Through typedefs, const, volatile and arrays to the base type."""
        seen = 0
        while dies.get(ref, {}).get("tag") in ("DW_TAG_typedef", "DW_TAG_const_type",
                                                 "DW_TAG_volatile_type", "DW_TAG_array_type"):
            if "type" not in dies[ref] or seen > 32:
                break
            ref = dies[ref]["type"]
            seen += 1
        return ref

    def walk(aggregate: int, base: int, path: str, out: list[tuple[str, int]]) -> None:
        for member in children.get(aggregate, []):
            d = dies[member]
            if d["tag"] != "DW_TAG_member":
                continue
            name = path + "." + d.get("name", "?")
            offset = base + d.get("offset", 0)
            out.append((name, offset * 8 + d.get("bits", 0)))
            target = resolve(d.get("type", -1))
            tdie = dies.get(target, {})
            if tdie.get("tag") in ("DW_TAG_structure_type", "DW_TAG_union_type"):
                walk(target, offset, name, out)
            elif tdie.get("tag") == "DW_TAG_pointer_type":
                out.append((name + " is a pointer", 0))

    result = {}
    for addr in order:
        d = dies[addr]
        if d.get("tag") == "DW_TAG_structure_type" and d.get("name") in SAVED and "size" in d:
            members: list[tuple[str, int]] = [("sizeof", d["size"])]
            walk(addr, 0, d["name"], members)
            result[d["name"]] = members
    return result


def main() -> None:
    if len(sys.argv) != 6:
        sys.exit(__doc__)
    arm_gcc, a64_gcc, dwarfdump, root, stamp = (
        Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3]),
        Path(sys.argv[4]), Path(sys.argv[5]))

    common = ["-DPORTABLE", "-DPLATFORM_3DS", "-DMODERN=1", "-DUBFIX", "-D__INTELLISENSE__",
              "-DPORT_BRIDGE", "-std=gnu99", "-fsigned-char", "-w", "-g", "-c",
              "-iquote", str(root / "3ds_port/compat"), "-iquote", str(root / "3ds_port/include"),
              "-iquote", str(root / "include"), "-iquote", str(root / "include/constants"),
              "-iquote", str(root)]
    arm_flags = ["-march=armv6k", "-mtune=mpcore", "-mfloat-abi=hard", "-mtp=soft", *common]
    a64_flags = ["-march=armv8-a+crc+crypto", "-mtune=cortex-a57", "-mtp=soft",
                 "-DPLATFORM_SWITCH", "-D__SWITCH__", *common]

    with tempfile.TemporaryDirectory() as tmp:
        a = layout(arm_gcc, dwarfdump, root, arm_flags, "arm32", Path(tmp))
        b = layout(a64_gcc, dwarfdump, root, a64_flags, "aarch64", Path(tmp))

    problems = []
    for name, gba_size in SAVED.items():
        if name not in a or name not in b:
            problems.append("%s: not found in the debug information" % name)
            continue
        for label, members in (("arm32", a[name]), ("aarch64", b[name])):
            if members[0][1] != gba_size:
                problems.append("%s is %d bytes on %s, the GBA's is %d"
                                % (name, members[0][1], label, gba_size))
            pointers = [m for m, _ in members if m.endswith(" is a pointer")]
            if pointers:
                problems.append("%s holds pointers on %s (they must be u32): %s"
                                % (name, label, ", ".join(p[:-len(" is a pointer")] for p in pointers[:5])))
        if a[name] != b[name]:
            diff = [(x, y) for x, y in zip(a[name], b[name]) if x != y][:5]
            problems.append("%s differs between arm32 and aarch64: %s" % (name, diff))
    if problems:
        sys.exit("check_save_layout_switch: " + "\n  ".join(problems))
    stamp.parent.mkdir(parents=True, exist_ok=True)
    stamp.write_text("".join("%s %d members\n" % (n, len(a[n])) for n in SAVED))
    print("check_save_layout_switch: %s laid out as on the GBA on both architectures (%s members)"
          % (", ".join(SAVED), " + ".join(str(len(a[n]) - 1) for n in SAVED)))


if __name__ == "__main__":
    main()
