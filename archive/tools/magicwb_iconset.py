#!/usr/bin/env python3
"""Build the MagicWB icon set that release archives carry in IconSets/MagicWB.

    magicwb_iconset.py MWB_DOpus8c.lha OUTDIR [--basedata FILE] [--update]

MWB_DOpus8c.lha is the MagicWB toolbar and icon collection by Trevor Morris
and Leo Davidson that came with Directory Opus 5.5 (Aminet pix/mwb).  The
script writes OUTDIR/IconSets/MagicWB, laid out like the release archive
root: DOpus5.info and a DOpus5 drawer holding MagicWB versions of the files
the standard set has in DOpus5.  The standard files come from --basedata
(archive/basedata.lha); --update replaces the IconSets/ entries of that
archive with the new tree.

- Images: every MWB_DOpus 8 brush (24x16, 8 colours, two frames with their
  own border) and its icon version in Images/Icons.  Names that exist in
  the standard set keep its spelling.  The default banks also use
  All_s.small (None.small), Makefile.small (Pencil_New.small) and
  Toggle.small, which is None.small with a "~" in place of the "X".
- Icons: the MWB_DOpus 8 images with the tool types, default tool, type,
  stack and drawer window of the standard icons they replace.  Command,
  FTP, FTPDirectory, FTPFile, FTPSite and PrintFilesAppIcon have no
  MWB_DOpus 8 version; they are the empty "Template" project icon with a
  toolbar brush pasted in.
- Buttons: Toolbar, Toolbar.default and FTP_image with borderless buttons,
  because the brushes draw their own border.
- Environment: default and workbench with "Remap icon images" off, so that
  the icons use the MagicWB colours Workbench keeps in pens 4-7.

Needs the "lha" command; --update needs one that can write archives, such
as LHa for UNIX in the build images (lhasa only extracts).  The planar
image and DiskObject layouts are the ones in the NDK (workbench/workbench.h,
intuition/intuition.h); the ANIM brushes use op 5 deltas in XOR mode, as
written by Deluxe Paint.
"""

import argparse
import hashlib
import os
import shutil
import struct
import subprocess
import sys
import tempfile
from collections import deque

MWB_SHA256 = "914d57427412680abd9b2d383e1227a02d6e7be185fe46ce6a976f5c62aec69a"

# Date of the files this script derives (2026-10-16 12:00 UTC)
DERIVED_DATE = 1792152000

# Colours 0, 4 and 5 of the MagicWB palette form the brush background
BACKGROUND = {0, 4, 5}

# CFG_BTNW.flags and BTNWF_BORDERLESS in source/Include/libraries/dopus5.h
BTNW_FLAGS = 128
BTNWF_BORDERLESS = 1 << 4
BORDERLESS_BANKS = ("Toolbar", "Toolbar.default", "FTP_image")

# CFG_ENVR.desktop_flags and DESKTOPF_NO_REMAP ("Remap icon images" off).
# With remapping Opus draws colours 4-7 of 8 colour icons in the last
# four pens of the screen, which on a screen with more than 8 colours do
# not hold the MagicWB colours that Workbench puts in pens 4-7.
ENVR_DESKTOP_FLAGS = 2134
DESKTOPF_NO_REMAP = 1 << 2
ENVIRONMENTS = ("default", "workbench")

# Standard icon -> MWB_DOpus 8 icon whose images replace its images
ICONS = {
    "DOpus5.info": "DOpus_Icons/Storage.info",
    "DOpus5/DirectoryOpus.info": "DOpus_Icons/Icons/Storage/DirectoryOpus.info",
    "DOpus5/Help.info": "DOpus_Icons/Help.info",
    "DOpus5/Help/DOpus5.guide.info": "Help/MWB_DOpus8_Help.guide.info",
    "DOpus5/Help/DOpusM2_ARexx.guide.info": "Help/MWB_DOpus8_Help.guide.info",
    "DOpus5/WBStartup/Opus5_Startup.info": "DOpus_Icons/Icons/Storage/DirectoryOpus.info",
}
for _name in ("AppIcon", "Buttons", "Environment", "Filetype", "Group", "Lister", "Settings"):
    ICONS["DOpus5/Icons/%s.info" % _name] = "DOpus_Icons/Icons/%s.info" % _name

# Standard icon -> brush pasted into the MWB_DOpus 8 Template icon
TEMPLATE = "DOpus_Icons/Icons/FileTypes/Template.info"
TEMPLATE_AT = (6, 8)
COMPOSITES = {
    "DOpus5/Icons/Command.info": "Wand",
    "DOpus5/Icons/FTP.info": "World",
    "DOpus5/Icons/FTPDirectory.info": "DirDrawer",
    "DOpus5/Icons/FTPFile.info": "FTP",
    "DOpus5/Icons/FTPSite.info": "Globe",
    "DOpus5/Icons/PrintFilesAppIcon.info": "Print",
}

# Images the default banks use that MWB_DOpus 8 does not have
IMAGE_COPIES = {"All_s.small": "None.small", "Makefile.small": "Pencil_New.small"}

# "~" drawn over None.small for Toggle.small, at column 2, row 5
TILDE = (
    ".FF......",
    "FFFF....F",
    "F..FF..FF",
    "....FFFF.",
    ".....FF..",
)
TILDE_AT = (2, 5)
GLYPH_BOX = (1, 3, 11, 13)  # the "X" of None.small: x0, y0, x1, y1


# --------------------------------------------------------------------
# IFF
# --------------------------------------------------------------------

def chunks(data, start, end):
    while start + 8 <= end:
        cid = data[start:start + 4]
        size = struct.unpack(">I", data[start + 4:start + 8])[0]
        yield cid, start + 8, size
        start += 8 + size + (size & 1)


def chunk(cid, body):
    return cid + struct.pack(">I", len(body)) + body + (b"\0" if len(body) & 1 else b"")


def unpack_byterun1(src, size):
    out = bytearray()
    i = 0
    while len(out) < size and i < len(src):
        c = src[i]
        i += 1
        if c < 128:
            out += src[i:i + c + 1]
            i += c + 1
        elif c > 128:
            out += bytes([src[i]]) * (257 - c)
            i += 1
    return bytes(out[:size])


class Brush:
    """ILBM brush or ANIM brush: frames of planar bitmaps."""

    def __init__(self, data):
        self.data = data
        self.frames = []
        self._anhd = None
        self._walk(12, len(data))

    def _walk(self, start, end):
        d = self.data
        for cid, o, size in chunks(d, start, end):
            body = d[o:o + size]
            if cid == b"FORM":
                self._walk(o + 4, o + size)
            elif cid == b"BMHD" and not hasattr(self, "width"):
                (self.width, self.height, _x, _y, self.depth, self.masking,
                 self.compression, _pad, _tc) = struct.unpack(">HHhhBBBBH", body[:14])
                self.bpr = ((self.width + 15) // 16) * 2
            elif cid == b"ANHD":
                self._anhd = body
            elif cid == b"BODY":
                self.frames.append(self._body(body))
            elif cid == b"DLTA":
                self.frames.append(self._delta(body))

    def _body(self, body):
        planes = self.depth + (1 if self.masking == 1 else 0)
        size = self.bpr * self.height * planes
        raw = unpack_byterun1(body, size) if self.compression == 1 else body
        bitmap = [bytearray(self.bpr * self.height) for _ in range(self.depth)]
        for y in range(self.height):
            for p in range(self.depth):
                row = (y * planes + p) * self.bpr
                bitmap[p][y * self.bpr:(y + 1) * self.bpr] = raw[row:row + self.bpr]
        return bitmap

    def _delta(self, body):
        op, interleave = self._anhd[0], self._anhd[18]
        xor = struct.unpack(">I", self._anhd[20:24])[0] & 2
        if op != 5:
            raise ValueError("ANIM op %d is not supported" % op)
        back = interleave or 2
        base = self.frames[-back] if len(self.frames) >= back else self.frames[0]
        bitmap = [bytearray(p) for p in base]
        pointers = struct.unpack(">16I", body[:64])
        for p in range(self.depth):
            q = pointers[p]
            if not q:
                continue
            for col in range(self.bpr):
                ops = body[q]
                q += 1
                y = 0
                for _ in range(ops):
                    c = body[q]
                    q += 1
                    if c == 0:
                        values = body[q + 1:q + 2] * body[q]
                        q += 2
                    elif c < 128:
                        y += c
                        continue
                    else:
                        values = body[q:q + (c & 127)]
                        q += c & 127
                    for v in values:
                        i = y * self.bpr + col
                        bitmap[p][i] = bitmap[p][i] ^ v if xor else v
                        y += 1
        return bitmap

    def pixels(self, frame):
        bitmap = self.frames[frame]
        return [[sum(((bitmap[p][y * self.bpr + x // 8] >> (7 - x % 8)) & 1) << p
                     for p in range(self.depth))
                 for x in range(self.width)]
                for y in range(self.height)]

    def planes_of(self, pix):
        bitmap = [bytearray(self.bpr * self.height) for _ in range(self.depth)]
        for y, row in enumerate(pix):
            for x, v in enumerate(row):
                for p in range(self.depth):
                    if v >> p & 1:
                        bitmap[p][y * self.bpr + x // 8] |= 0x80 >> (x % 8)
        return bitmap

    def rewrite(self, frame_pixels):
        """The same ANIM with new frames: BODY for the first, XOR op 5 deltas
        for the others (the last delta of a ping-pong brush returns to the
        first frame)."""
        if self.compression != 0 or self.masking == 1:
            raise ValueError("only uncompressed brushes without a mask plane")
        bitmaps = [self.planes_of(pix) for pix in frame_pixels]
        bitmaps.append(bitmaps[0])
        d = self.data
        forms = []
        for index, (cid, o, size) in enumerate(chunks(d, 12, len(d))):
            inner = b""
            for icid, io, isize in chunks(d, o + 4, o + size):
                body = d[io:io + isize]
                if icid == b"BODY":
                    body = b"".join(bytes(bitmaps[0][p][y * self.bpr:(y + 1) * self.bpr])
                                    for y in range(self.height) for p in range(self.depth))
                elif icid == b"DLTA":
                    body = self._encode_delta(bitmaps[index - 1], bitmaps[index])
                inner += chunk(icid, body)
            forms.append(chunk(b"FORM", d[o:o + 4] + inner))
        return chunk(b"FORM", b"ANIM" + b"".join(forms))

    def _encode_delta(self, old, new):
        # One "uniq" run per changed column
        out = bytearray(64)
        for p in range(self.depth):
            column_data = bytearray()
            for col in range(self.bpr):
                values = bytes(old[p][y * self.bpr + col] ^ new[p][y * self.bpr + col]
                               for y in range(self.height))
                if any(values):
                    column_data += bytes([1, 0x80 | self.height]) + values
                else:
                    column_data.append(0)
            struct.pack_into(">I", out, p * 4, len(out))
            out += column_data
        return bytes(out)


# --------------------------------------------------------------------
# Icons
# --------------------------------------------------------------------

def read_string(d, o):
    size = struct.unpack(">I", d[o:o + 4])[0]
    return d[o + 4:o + 4 + size], o + 4 + size


class Icon:
    """The parts of a DiskObject this script keeps or replaces."""

    def __init__(self, data):
        if struct.unpack(">H", data[:2])[0] != 0xE310:
            raise ValueError("not an icon")
        g = struct.unpack(">IhhhhHHHIIIIIHI", data[4:48])
        render, select, user_data = g[8], g[9], g[14]
        self.type = data[48]
        (default_tool, tool_types, self.x, self.y, drawer_data, tool_window,
         self.stack) = struct.unpack(">IIiiIIi", data[50:78])
        o = 78
        self.drawer = data[o:o + 56] if drawer_data else None
        if drawer_data:
            o += 56
        self.images = []
        for present in (render, select):
            if not present:
                continue
            _le, _te, w, h, depth, image_data, pick, on_off, _next = \
                struct.unpack(">hhhhhIBBI", data[o:o + 20])
            o += 20
            bpr = ((w + 15) // 16) * 2
            planes = []
            for _ in range(depth if image_data else 0):
                planes.append(data[o:o + bpr * h])
                o += bpr * h
            pix = [[0] * w for _ in range(h)]
            for y in range(h):
                for x in range(w):
                    v, index = 0, 0
                    for bit in range(8):
                        if pick & (1 << bit):
                            if index < len(planes) and \
                                    planes[index][y * bpr + x // 8] & (0x80 >> (x % 8)):
                                v |= 1 << bit
                            index += 1
                        elif on_off & (1 << bit):
                            v |= 1 << bit
                    pix[y][x] = v
            self.images.append(pix)
        self.default_tool = None
        if default_tool:
            s, o = read_string(data, o)
            self.default_tool = s.rstrip(b"\0")
        self.tool_types = None
        if tool_types:
            count = struct.unpack(">I", data[o:o + 4])[0] // 4 - 1
            o += 4
            self.tool_types = []
            for _ in range(count):
                s, o = read_string(data, o)
                self.tool_types.append(s.rstrip(b"\0"))
        self.tool_window = None
        if tool_window:
            self.tool_window, o = read_string(data, o)
        self.drawer2 = None
        if drawer_data and (user_data & 0xFF) >= 1:
            self.drawer2 = data[o:o + 6]

    def encode(self):
        """Planar icon, 3 bitplanes, no colour icon chunk."""
        if self.tool_window is not None:
            raise ValueError("icons with a tool window are not supported")
        w, h = len(self.images[0][0]), len(self.images[0])
        if any((len(pix[0]), len(pix)) != (w, h) for pix in self.images):
            raise ValueError("the images of an icon must have the same size")
        drawer = self.type in (1, 2, 5)
        out = struct.pack(">HH", 0xE310, 1)
        out += struct.pack(">IhhhhHHHIIIIIHI", 0, 0, 0, w, h + 1, 6, 3, 1,
                           1, 1 if len(self.images) > 1 else 0, 0, 0, 0, 0, 1)
        out += struct.pack(">BB", self.type, 0)
        out += struct.pack(">IIiiIIi", 1 if self.default_tool is not None else 0,
                           1 if self.tool_types is not None else 0, self.x, self.y,
                           1 if drawer else 0, 0, self.stack)
        if drawer:
            out += self.drawer if self.drawer else bytes(56)
        bpr = ((w + 15) // 16) * 2
        for pix in self.images:
            out += struct.pack(">hhhhhIBBI", 0, 0, w, h, 3, 1, 7, 0, 0)
            for p in range(3):
                plane = bytearray(bpr * h)
                for y, row in enumerate(pix):
                    for x, v in enumerate(row):
                        if v >> p & 1:
                            plane[y * bpr + x // 8] |= 0x80 >> (x % 8)
                out += bytes(plane)
        if self.default_tool is not None:
            out += struct.pack(">I", len(self.default_tool) + 1) + self.default_tool + b"\0"
        if self.tool_types is not None:
            out += struct.pack(">I", (len(self.tool_types) + 1) * 4)
            for s in self.tool_types:
                out += struct.pack(">I", len(s) + 1) + s + b"\0"
        if drawer:
            out += self.drawer2 if self.drawer2 else bytes(6)
        return out


def restyle(standard, images):
    """The standard icon with other images."""
    icon = Icon(standard)
    icon.images = images
    if icon.drawer2 is None and icon.type in (1, 2, 5):
        icon.drawer2 = bytes(6)
    return icon.encode()


# --------------------------------------------------------------------
# Derived images
# --------------------------------------------------------------------

def glyph(brush, frame):
    """The picture of a brush without its border and background: the
    background is what can be reached from the edge through colours 0, 4
    and 5."""
    pix = brush.pixels(frame)
    x0, y0, x1, y1 = 1, 1, brush.width - 2, brush.height - 2
    background = set()
    todo = deque((x, y) for x in range(x0, x1 + 1) for y in (y0, y1))
    todo.extend((x, y) for y in range(y0, y1 + 1) for x in (x0, x1))
    while todo:
        x, y = todo.popleft()
        if not (x0 <= x <= x1 and y0 <= y <= y1) or (x, y) in background \
                or pix[y][x] not in BACKGROUND:
            continue
        background.add((x, y))
        todo.extend(((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)))
    return [[None if (x, y) in background else pix[y][x] for x in range(x0, x1 + 1)]
            for y in range(y0, y1 + 1)]


def composite(template, brush):
    images = []
    for frame, pix in enumerate(Icon(template).images):
        pix = [row[:] for row in pix]
        for y, row in enumerate(glyph(brush, frame)):
            for x, v in enumerate(row):
                if v is not None:
                    pix[TEMPLATE_AT[1] + y][TEMPLATE_AT[0] + x] = v
        images.append(pix)
    return images


def background_pixel(x, y):
    # The brushes' background dither: light grey towards the top left,
    # dark grey towards the bottom right
    s = x + y
    if s <= 9 and s % 2:
        return 5
    if s >= 19 and not s % 2:
        return 4
    return 0


def toggle_frames(none):
    fill = {(TILDE_AT[0] + x, TILDE_AT[1] + y)
            for y, row in enumerate(TILDE) for x, c in enumerate(row) if c == "F"}
    outline = {(x + dx, y + dy) for x, y in fill
               for dx in (-1, 0, 1) for dy in (-1, 0, 1)} - fill
    frames = []
    for frame in range(2):
        pix = none.pixels(frame)
        bx0, by0, bx1, by1 = GLYPH_BOX
        for y in range(by0, by1 + 1):
            for x in range(bx0, bx1 + 1):
                pix[y][x] = background_pixel(x, y)
        for x, y in outline:
            if (x + 1, y + 1) not in fill | outline:
                pix[y + 1][x + 1] = 3
        for x, y in outline:
            pix[y][x] = 1
        for x, y in fill:
            lit = (x, y - 1) in outline or (x - 1, y) in outline
            shaded = (x, y + 1) in outline or (x + 1, y) in outline
            pix[y][x] = 2 if lit and not shaded else 6 if shaded and not lit else 7
        frames.append(pix)
    return frames


def set_flag(data, chunk_id, offset, flag):
    """Sets a bit in the ULONG at offset in the first chunk_id chunk."""
    out = bytearray(data)
    for cid, o, size in chunks(data, 12, len(data)):
        if cid == chunk_id:
            flags = struct.unpack(">I", data[o + offset:o + offset + 4])[0]
            struct.pack_into(">I", out, o + offset, flags | flag)
            return bytes(out)
    raise ValueError("no %s chunk" % chunk_id.decode())


def borderless(bank):
    return set_flag(bank, b"BTNW", BTNW_FLAGS, BTNWF_BORDERLESS)


def no_remap(environment):
    return set_flag(environment, b"ENVR", ENVR_DESKTOP_FLAGS, DESKTOPF_NO_REMAP)


# --------------------------------------------------------------------
# Build
# --------------------------------------------------------------------

def extract(archive, dest):
    subprocess.run(["lha", "xqw=" + dest, archive], check=True)


def build(mwb_archive, basedata, out):
    with open(mwb_archive, "rb") as f:
        digest = hashlib.sha256(f.read()).hexdigest()
    if digest != MWB_SHA256:
        sys.exit("%s is not MWB_DOpus8c.lha (SHA-256 %s)" % (mwb_archive, digest))

    work = tempfile.mkdtemp()
    try:
        extract(mwb_archive, os.path.join(work, "mwb"))
        extract(basedata, os.path.join(work, "base"))
        mwb = os.path.join(work, "mwb", "MWB8")
        base = os.path.join(work, "base")
        root = os.path.join(out, "IconSets", "MagicWB")
        if os.path.exists(root):
            shutil.rmtree(root)

        def read(*path):
            with open(os.path.join(*path), "rb") as f:
                return f.read()

        def write(path, data, like=None):
            # Copies keep the date of their MWB_DOpus 8 file, derived files
            # get DERIVED_DATE, so that a rebuild gives the same archive
            path = os.path.join(root, path)
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "wb") as f:
                f.write(data)
            stamp = os.path.getmtime(like) if like else DERIVED_DATE
            os.utime(path, (stamp, stamp))

        # Images and their icons, with the standard set's spelling
        standard = {}
        for sub in ("Images", "Images/Icons"):
            for name in os.listdir(os.path.join(base, "DOpus5", sub)):
                standard[(sub, name.lower())] = name
        for sub, src in (("Images", "Images"), ("Images/Icons", "Images/icons")):
            for name in sorted(os.listdir(os.path.join(mwb, src))):
                path = os.path.join(mwb, src, name)
                if os.path.isfile(path) and name != "Icons.info":
                    write(os.path.join("DOpus5", sub, standard.get((sub, name.lower()), name)),
                          read(path), like=path)
        for name, src in IMAGE_COPIES.items():
            path = os.path.join(mwb, "Images", src)
            write(os.path.join("DOpus5", "Images", name), read(path), like=path)
        none = Brush(read(mwb, "Images", "None.small"))
        write(os.path.join("DOpus5", "Images", "Toggle.small"), none.rewrite(toggle_frames(none)))

        # Icons
        for target, src in ICONS.items():
            write(target, restyle(read(base, target), Icon(read(mwb, src)).images))
        template = read(mwb, TEMPLATE)
        for target, name in COMPOSITES.items():
            brush = Brush(read(mwb, "Images", name + ".small"))
            write(target, restyle(read(base, target), composite(template, brush)))

        # Banks and environments
        for name in BORDERLESS_BANKS:
            path = os.path.join("DOpus5", "Buttons", name)
            write(path, borderless(read(base, path)))
        for name in ENVIRONMENTS:
            path = os.path.join("DOpus5", "Environment", name)
            write(path, no_remap(read(base, path)))
    finally:
        shutil.rmtree(work)


def update_basedata(basedata, out):
    listing = subprocess.run(["lha", "lq", basedata], check=True,
                             stdout=subprocess.PIPE, text=True).stdout
    # Names may contain spaces, so take everything from "IconSets/" on
    old = [line[line.index(" IconSets/") + 1:].rstrip() for line in listing.splitlines()
           if " IconSets/" in line]
    if old:
        subprocess.run(["lha", "dq", os.path.abspath(basedata)] + old, check=True)
    # Same permissions as the other entries of the base archive
    for top, dirs, files in os.walk(os.path.join(out, "IconSets")):
        os.chmod(top, 0o777)
        os.utime(top, (DERIVED_DATE, DERIVED_DATE))
        for name in files:
            os.chmod(os.path.join(top, name), 0o666)
    subprocess.run(["lha", "aq", os.path.abspath(basedata), "IconSets"], check=True, cwd=out)


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("mwb", help="MWB_DOpus8c.lha")
    parser.add_argument("out", help="directory to write IconSets/MagicWB into")
    parser.add_argument("--basedata", default=os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "basedata.lha"),
        help="base archive to read the standard set from (default archive/basedata.lha)")
    parser.add_argument("--update", action="store_true",
                        help="replace the IconSets/ entries of the base archive")
    args = parser.parse_args(argv)
    build(args.mwb, args.basedata, args.out)
    if args.update:
        update_basedata(args.basedata, args.out)


if __name__ == "__main__":
    main(sys.argv[1:])
