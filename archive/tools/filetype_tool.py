#!/usr/bin/env python3
"""Read, check and review Directory Opus 5 filetype files.

    filetype_tool.py dump  FILE...   show what each filetype recognises and does
    filetype_tool.py check FILE...   decode and re-encode; report files that differ
    filetype_tool.py lint  FILE...   point out problems worth a look before shipping

Filetype files live in DOpus5:Filetypes (installed) and
DOpus5:Storage/Filetypes (stored).  Extract archives first, for example
with "lha x".  The format is the one read by L_ReadFiletypes() in
source/Library/config_open.c and written by L_SaveFiletypeList() in
source/Library/config_save.c.
"""

import os
import struct
import sys

# FTOP_* in source/Library/filetypes.h
OPS = [
    "NOOP", "MATCH", "MATCHNAME", "MATCHBITS", "MATCHCOMMENT", "MATCHSIZE",
    "MATCHDATE", "MOVETO", "MOVE", "SEARCHFOR", "MATCHFORM", "FINDCHUNK",
    "MATCHDTGROUP", "MATCHDTID", "MATCHNOCASE", "DIRECTORY", "MODULE", "DISK",
    "SEARCHRANGE", "MATCHCHUNK",
]
FTOP_OR, FTOP_AND, FTOP_ENDSECTION = 253, 254, 255
SPECIAL = {FTOP_OR: "OR", FTOP_AND: "AND", FTOP_ENDSECTION: "END"}

# FTTYPE_* in source/Program/filetypes.h
EVENTS = [
    "User1", "User2", "Double-click", "Drag'n'drop", "Ctrl double-click", "User3",
    "Alt double-click", "Alt drag'n'drop", "Ctrl drag'n'drop", "User4",
    "Shift double-click", "Shift drag'n'drop", "User5", "User6", "User7",
    "User8", "User9", "User10",
]
DRAG_DROP = 3

# INST_* in source/Include/libraries/dopus5.h
INSTRUCTIONS = {0: "Command", 1: "AmigaDOS", 2: "Workbench", 3: "Script", 4: "ARexx", 5: "Label"}
INST_LABEL = 5

# FUNCF_* in source/Include/libraries/dopus5.h
FUNC_FLAGS = [
    (1 << 0, "output window"), (1 << 1, "output to file"), (1 << 2, "WB output"),
    (1 << 4, "async"), (1 << 5, "CD source"), (1 << 6, "CD dest"),
    (1 << 7, "all files"), (1 << 8, "recurse"), (1 << 9, "reload"),
    (1 << 11, "no quotes"), (1 << 12, "rescan source"), (1 << 13, "rescan dest"),
    (1 << 16, "wait close"),
]
# FUNCF_ORIGINAL_FLAGS: the only function flags the editor saves
STORED_FUNC_FLAGS = sum(bit for bit, _name in FUNC_FLAGS)
FUNCF2_LABEL_FUNC = 1 << 3
# FUNCF2_HOTKEY_FUNC .. FUNCF2_LABEL_FUNC: the flags2 bits that are stored
STORED_FUNC_FLAGS2 = 0x0000000F
# Transient state, never meant to be stored: FUNCF2_PLACEHOLDER (bit 29, the
# Filetype Manager deletes such functions when its editor closes),
# FUNCF2_ORIGINAL and FUNCF2_FREE_FUNCTION (Opus frees a function that is
# still linked into the filetype list).
FUNCF2_INTERNAL = (1 << 29) | (1 << 30) | (1 << 31)

# Path prefixes that exist on a stock system or in DOpus5:.  PROGDIR: is
# provided by AmigaDOS for every program; Opus assigns D5THEMES: itself.
STOCK_ASSIGNS = ("SYS:", "C:", "S:", "L:", "LIBS:", "DEVS:", "FONTS:", "ENV:",
                 "ENVARC:", "T:", "RAM:", "DOPUS5:", "REXX:", "HELP:", "LOCALE:",
                 "PROGDIR:", "D5THEMES:")


class Filetype:
    def __init__(self, header, recognition, icon, actions):
        self.header = bytearray(header)  # CFG_TYPE, 46 bytes
        self.recognition = recognition
        self.icon = icon
        self.actions = actions            # 16 status strings
        self.functions = []

    @property
    def name(self):
        return self.header[:32].split(b"\0", 1)[0].decode("latin-1")

    @property
    def id(self):
        return self.header[32:40].split(b"\0", 1)[0].decode("latin-1")

    @property
    def flags(self):
        return struct.unpack(">H", self.header[40:42])[0]

    @property
    def priority(self):
        return struct.unpack(">b", self.header[42:43])[0]


class Function:
    def __init__(self, header, instructions):
        self.header = bytearray(header)   # CFG_FUNC, 22 bytes
        self.instructions = instructions  # [(type, bytes)]

    @property
    def flags(self):
        return struct.unpack(">I", self.header[0:4])[0]

    @property
    def is_menu(self):
        return bool(struct.unpack(">I", self.header[4:8])[0] & FUNCF2_LABEL_FUNC)

    @property
    def event(self):
        return struct.unpack(">H", self.header[16:18])[0]


def parse(data):
    if data[:4] != b"FORM" or data[8:12] != b"OPUS":
        raise ValueError("not an Opus IFF file")

    types = []
    pos = 12
    while pos + 8 <= len(data):
        chunk = data[pos:pos + 4]
        size = struct.unpack(">I", data[pos + 4:pos + 8])[0]
        if pos + 8 + size > len(data):
            raise ValueError("%r chunk runs past the end of the file" % chunk)
        body = data[pos + 8:pos + 8 + size]
        pos += 8 + size + (size & 1)

        if chunk == b"TYPE":
            if len(body) < 46:
                raise ValueError("TYPE chunk is shorter than CFG_TYPE (46 bytes)")
            # Opus uses name[32] and id[8] as C strings
            if b"\0" not in body[:32]:
                raise ValueError("filetype name fills all 32 bytes without a NUL")
            if b"\0" not in body[32:40]:
                raise ValueError("filetype ID fills all 8 bytes without a NUL")
            # L_SaveFiletypeList() writes 16 actions and fails if count is higher
            if struct.unpack(">H", body[44:46])[0] > 16:
                raise ValueError("CFG_TYPE count %d is above 16, so Opus cannot save the filetype again"
                                 % struct.unpack(">H", body[44:46])[0])
            strings = body[46:].split(b"\0")
            if len(strings) != 19 or strings[-1] != b"":
                raise ValueError("TYPE chunk does not hold recognition, icon and 16 actions")
            types.append(Filetype(body[:46], strings[0], strings[1], strings[2:18]))
        elif chunk == b"FUNC":
            if not types:
                raise ValueError("FUNC chunk before any TYPE chunk")
            if len(body) < 22:
                raise ValueError("FUNC chunk is shorter than CFG_FUNC (22 bytes)")
            rest = body[22:]
            instructions = []
            while rest:
                end = rest.find(b"\0")
                if end < 0 or len(rest) < end + 3:
                    raise ValueError("truncated instruction in FUNC chunk")
                kind = struct.unpack(">h", rest[end + 1:end + 3])[0]
                # Opus treats any other type as an external command and
                # indexes its script prefix table with it, unchecked.
                if kind not in INSTRUCTIONS:
                    raise ValueError("instruction type %d is not one of INST_COMMAND..INST_LABEL" % kind)
                instructions.append((kind, rest[:end]))
                rest = rest[end + 3:]
            flags2 = struct.unpack(">I", body[4:8])[0]
            if flags2 & FUNCF2_INTERNAL:
                raise ValueError("FUNC sets transient flags2 bits 0x%08x "
                                 "(FUNCF2_PLACEHOLDER/ORIGINAL/FREE_FUNCTION)" % (flags2 & FUNCF2_INTERNAL))
            types[-1].functions.append(Function(body[:22], instructions))
        else:
            raise ValueError("unexpected chunk %r" % chunk)

    if pos != len(data):
        raise ValueError("missing pad byte after the last chunk" if pos > len(data)
                         else "trailing data after the last chunk")
    if not types:
        raise ValueError("no TYPE chunk, so Opus loads no filetype from it")
    return types


def chunk(chunk_id, body):
    return chunk_id + struct.pack(">I", len(body)) + body + (b"\0" if len(body) & 1 else b"")


def encode(types):
    out = b""
    for ftype in types:
        body = bytes(ftype.header) + ftype.recognition + b"\0" + ftype.icon + b"\0"
        body += b"".join(action + b"\0" for action in ftype.actions)
        out += chunk(b"TYPE", body)
        for func in ftype.functions:
            body = bytes(func.header)
            body += b"".join(text + b"\0" + struct.pack(">h", kind) for kind, text in func.instructions)
            out += chunk(b"FUNC", body)
    return b"FORM" + struct.pack(">I", len(out) + 4) + b"OPUS" + out


def recognition_clauses(recognition):
    """Yield (operation, argument) pairs and "OR"/"AND"/"END" markers."""
    operation = None
    argument = b""
    for byte in recognition + b"\0":
        if operation is None:
            if 0 < byte < len(OPS):
                operation, argument = OPS[byte], b""
            elif byte in SPECIAL:
                yield SPECIAL[byte], None
            elif byte == 0:
                return
        elif byte > 252 or byte == 0:
            yield operation, argument.decode("latin-1")
            operation = None
            if byte in SPECIAL:
                yield SPECIAL[byte], None
            if byte == 0:
                return
        else:
            argument += bytes([byte])


def recognition_text(recognition):
    parts = []
    for operation, argument in recognition_clauses(recognition):
        if argument is None:
            parts.append(operation)
        else:
            parts.append("%s %r" % (operation, argument))
    return " ".join(parts)


def flag_text(flags):
    names = [name for bit, name in FUNC_FLAGS if flags & bit]
    return (" [" + ", ".join(names) + "]") if names else ""


def function_text(func):
    if func.is_menu:
        kind = "Menu"
    elif func.event < len(EVENTS):
        kind = EVENTS[func.event]
    else:
        kind = "event %d" % func.event
    steps = []
    for inst, text in func.instructions:
        steps.append("%s: %s" % (INSTRUCTIONS.get(inst, "type %d" % inst), text.decode("latin-1")))
    return "%s%s: %s" % (kind, flag_text(func.flags), "; ".join(steps) if steps else "(empty)")


def emit(line=""):
    """Print a line with control characters escaped, so a contributed file
    cannot send escape sequences to the reviewer's terminal."""
    print("".join(char if " " <= char < "\x7f" or char >= "\xa0" else "\\x%02x" % ord(char)
                  for char in line))


def dump(path, types):
    emit("== %s" % path)
    for ftype in types:
        emit("  Name:     %s" % ftype.name)
        emit("  ID:       %s   priority %d   flags 0x%04x" % (ftype.id, ftype.priority, ftype.flags))
        emit("  Icon:     %s" % (ftype.icon.decode("latin-1") or "-"))
        emit("  Matches:  %s" % recognition_text(ftype.recognition))
        for index, action in enumerate(ftype.actions):
            if action:
                event = EVENTS[index] if index < len(EVENTS) else str(index)
                emit("  Status:   %s: %s" % (event, action.decode("latin-1")))
        for func in ftype.functions:
            emit("  Action:   %s" % function_text(func))
    emit()


def command_text(text):
    """Return an instruction as Opus passes it on: function_parse.c drops a
    once-only '#' marker, but only in the first byte."""
    return text[1:] if text.startswith("#") else text


def shell_words(text):
    """Split a command line into words, keeping "quoted" words together."""
    words, word, quoted = [], "", False
    for char in text:
        if char == '"':
            quoted = not quoted
        elif char in " \t" and not quoted:
            if word:
                words.append(word)
            word = ""
        else:
            word += char
    if word:
        words.append(word)
    return words


def program_word(inst, text):
    """Return the program or script an instruction runs.

    AmigaDOS instructions are shell command lines: skip a leading "run" and
    I/O redirections.  For Workbench instructions function_script.c strips
    "run " only when it starts in the first byte and passes the rest to
    dopusrt5.  Script and ARexx instructions get "execute " or "rx "
    prepended, so their first word is the script."""
    text = command_text(text)
    if inst == 2 and text[:4].lower() == "run ":
        text = text[4:]
    words = shell_words(text)
    if inst == 1:
        if words and words[0].lower() == "run":
            words = words[1:]
        while words and words[0].startswith(("<", ">", "*>")):
            words = words[1:]
    return words[0] if words else ""


def path_prefix(text):
    """Return the device or assign a path starts with, or None."""
    if ":" in text:
        return text[:text.index(":") + 1].upper()
    return None


def lint(path, types):
    problems = []
    if len(types) == 1 and os.path.basename(path) != types[0].name.replace("/", "\\"):
        problems.append("file name differs from the filetype name %r; the Filetype Finder "
                        "installs under the filetype name, the Filetype Manager under the "
                        "file name" % types[0].name)

    for ftype in types:
        if not ftype.name.strip():
            problems.append("filetype has no name")
        if not ftype.id.strip():
            problems.append("%s: filetype has no ID" % ftype.name)

        icon = ftype.icon.decode("latin-1")
        if icon:
            prefix = path_prefix(icon)
            if prefix is None:
                problems.append("%s: relative icon path %r" % (ftype.name, icon))
            elif prefix not in STOCK_ASSIGNS:
                problems.append("%s: icon path uses %s %r" % (ftype.name, prefix, icon))

        clauses = [(op, arg) for op, arg in recognition_clauses(ftype.recognition) if arg is not None]
        if not clauses:
            problems.append("%s: no recognition clauses, so it never matches" % ftype.name)
        elif all(op == "MATCHNAME" for op, arg in clauses) and \
                not any(arg in ("#?", "*") for op, arg in clauses):
            problems.append("%s: recognised by file name only" % ftype.name)
        for op, arg in recognition_clauses(ftype.recognition):
            if op == "MATCHDTID" and arg != arg.lower():
                problems.append("%s: MATCHDTID %r is lower-cased before the comparison, so it "
                                "only matches datatypes with a lower-case ID" % (ftype.name, arg))

        seen_events = set()
        for func in ftype.functions:
            # backdrop_popup.c uses the first INST_LABEL and ignores the rest
            label = next((text for inst, text in func.instructions if inst == INST_LABEL), b"")
            if func.is_menu:
                # backdrop_popup.c only adds a menu item for an INST_LABEL
                if not label.strip():
                    problems.append("%s: %s has no usable menu label (first label missing or blank)"
                                    % (ftype.name, function_text(func)))
            elif func.event >= len(EVENTS):
                problems.append("%s: %s is not an Opus filetype event, so it never runs"
                                % (ftype.name, function_text(func)))
            elif func.event in seen_events:
                # FindFunctionType() returns the first function for an event
                problems.append("%s: %s repeats an event, so only the first one runs"
                                % (ftype.name, function_text(func)))
            else:
                seen_events.add(func.event)

            # Higher bits are runtime FUNCF_* flags of function handles
            if func.flags & ~STORED_FUNC_FLAGS:
                problems.append("%s: %s sets function flags 0x%08x that the editor never saves"
                                % (ftype.name, function_text(func), func.flags & ~STORED_FUNC_FLAGS))
            flags2 = struct.unpack(">I", func.header[4:8])[0]
            if flags2 & ~STORED_FUNC_FLAGS2:
                problems.append("%s: %s sets unknown flags2 bits 0x%08x"
                                % (ftype.name, function_text(func), flags2 & ~STORED_FUNC_FLAGS2))

            # Opus skips empty instructions, so they do not count as commands;
            # neither does a command line with nothing to run ("run >NIL:")
            commands = [(inst, command_text(text.decode("latin-1")))
                        for inst, text in func.instructions if inst != INST_LABEL]
            commands = [(inst, text) for inst, text in commands
                        if (text.strip() if inst == 0 else program_word(inst, text))]
            if not commands:
                if not func.is_menu or label != b"---":
                    problems.append("%s: %s has no commands" % (ftype.name, function_text(func)))
                continue
            # A "---" label only adds a separator; the commands are unreachable
            if func.is_menu and label == b"---":
                problems.append("%s: %s is a separator, so its commands never run"
                                % (ftype.name, function_text(func)))
            for inst, text in commands:
                prefix = path_prefix(program_word(inst, text))
                if inst in (1, 2, 3, 4) and prefix and prefix not in STOCK_ASSIGNS:
                    problems.append("%s: needs %s for %r" % (ftype.name, prefix, text))
                # function_script.c only strips "run " from the first byte
                if inst == 2 and text[:4].lower() != "run " and text.lstrip()[:4].lower() == "run ":
                    problems.append("%s: Workbench instruction %r has \"run\" after leading spaces, "
                                    "so dopusrt5 is asked to start a program called run" % (ftype.name, text))
            if not func.is_menu and func.event == DRAG_DROP and \
                    [(i, t.strip().lower()) for i, t in commands] in ([(0, "copy")], [(1, "copy")]):
                problems.append("%s: drag'n'drop runs a plain Copy, overriding the default "
                                "Copy MOVEWHENSAME" % ftype.name)

    for problem in problems:
        emit("%s: %s" % (path, problem))
    return problems


def main(argv):
    if len(argv) < 3 or argv[1] not in ("dump", "check", "lint"):
        sys.stderr.write(__doc__)
        return 2

    status = 0
    for path in argv[2:]:
        try:
            with open(path, "rb") as handle:
                data = handle.read()
            types = parse(data)
        except (OSError, ValueError) as error:
            emit("%s: %s" % (path, error))
            status = 1
            continue

        if argv[1] == "dump":
            dump(path, types)
        elif argv[1] == "check":
            if encode(types) != data:
                emit("%s: re-encoded file differs" % path)
                status = 1
        elif lint(path, types):
            status = 1
    return status


if __name__ == "__main__":
    sys.exit(main(sys.argv))
