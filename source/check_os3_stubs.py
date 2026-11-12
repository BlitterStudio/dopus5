#!/usr/bin/env python3
"""Reject AmigaOS 3 binaries whose library-call stubs load a broken base.

    check_os3_stubs.py <file or directory> [...]

A library-call stub loads its base with `move.l <base>,a6` and an absolute
relocation against the hunk holding <base> (data or bss).  binutils
2.46.0.20260504 relocated that load against the code hunk with an offset of 0
in LTO links, so a6 got the module's first code bytes and the first call
through the stub jumped through garbage.  Any such stub fails the check.
Files that are not HUNK executables are skipped.
"""
import os
import struct
import sys

HUNK_HEADER = 0x3F3
HUNK_CODE, HUNK_DATA, HUNK_BSS = 0x3E9, 0x3EA, 0x3EB
HUNK_RELOC32, HUNK_SYMBOL, HUNK_DEBUG, HUNK_END = 0x3EC, 0x3F0, 0x3F1, 0x3F2
HUNK_DREL32, HUNK_RELOC32SHORT = 0x3F7, 0x3FC
MOVE_L_ABS_A6 = b"\x2c\x79"


def read_hunks(data):
    """Return (hunks, relocs) of a HUNK executable, or None for other files."""
    pos = 0

    def long():
        nonlocal pos
        value = struct.unpack_from(">I", data, pos)[0]
        pos += 4
        return value

    def word():
        nonlocal pos
        value = struct.unpack_from(">H", data, pos)[0]
        pos += 2
        return value

    if len(data) < 4 or long() != HUNK_HEADER:
        return None
    while True:  # resident library names
        count = long()
        if count == 0:
            break
        pos += count * 4
    long()  # table size
    first, last = long(), long()
    for _ in range(last - first + 1):
        if long() >> 30 == 3:
            long()  # extra memory flags
    hunks, relocs = [], []
    while pos < len(data):
        kind = long() & 0x3FFFFFFF
        if kind in (HUNK_CODE, HUNK_DATA):
            size = (long() & 0x3FFFFFFF) * 4
            hunks.append((kind, data[pos:pos + size]))
            pos += size
        elif kind == HUNK_BSS:
            long()
            hunks.append((kind, b""))
        elif kind == HUNK_RELOC32:
            while True:
                count = long()
                if count == 0:
                    break
                target = long()
                relocs += [(len(hunks) - 1, target, long()) for _ in range(count)]
        elif kind in (HUNK_RELOC32SHORT, HUNK_DREL32):
            start = pos
            while True:
                count = word()
                if count == 0:
                    break
                target = word()
                relocs += [(len(hunks) - 1, target, word()) for _ in range(count)]
            if (pos - start) % 4:
                pos += 2
        elif kind == HUNK_SYMBOL:
            while True:
                count = long() & 0x00FFFFFF  # top byte is the symbol type
                if count == 0:
                    break
                pos += count * 4 + 4
        elif kind == HUNK_DEBUG:
            size = long() * 4  # read first: `pos += long()` adds to the old pos
            pos += size
        elif kind != HUNK_END:
            raise ValueError(f"unknown hunk type {kind:#x} at offset {pos - 4:#x}")
    return hunks, relocs


def broken_stubs(hunks, relocs):
    """Return (stubs, broken) counts of `move.l <abs>,a6` base loads."""
    stubs = broken = 0
    for hunk, target, offset in relocs:
        kind, body = hunks[hunk]
        if kind != HUNK_CODE or offset < 2 or offset + 4 > len(body):
            continue
        if body[offset - 2:offset] != MOVE_L_ABS_A6:
            continue
        stubs += 1
        if hunks[target][0] == HUNK_CODE and body[offset:offset + 4] == b"\0\0\0\0":
            broken += 1
    return stubs, broken


def main(args):
    if not args:
        print(f"usage: {sys.argv[0]} <file or directory> [...]", file=sys.stderr)
        return 2
    paths = []
    for arg in args:
        if os.path.isdir(arg):
            for root, _, names in os.walk(arg):
                paths += [os.path.join(root, name) for name in names]
        else:
            paths.append(arg)
    checked = total_stubs = failed = 0
    for path in sorted(paths):
        with open(path, "rb") as f:
            data = f.read()
        try:
            parsed = read_hunks(data)
        except (ValueError, struct.error, IndexError) as error:
            print(f"{path}: cannot parse: {error}", file=sys.stderr)
            failed = 1
            continue
        if parsed is None:
            continue
        checked += 1
        stubs, broken = broken_stubs(*parsed)
        total_stubs += stubs
        if broken:
            print(f"{path}: {broken} of {stubs} library base loads relocated against code")
            failed = 1
    if checked == 0:
        print("no HUNK executables found", file=sys.stderr)
        return 1
    print(f"checked {checked} binaries, {total_stubs} library base loads"
          + (": broken stubs found" if failed else ": ok"))
    return failed


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
