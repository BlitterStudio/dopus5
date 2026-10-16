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
# The handlers are for redirections such as >NIL:.
STOCK_ASSIGNS = ("SYS:", "C:", "S:", "L:", "LIBS:", "DEVS:", "FONTS:", "ENV:",
                 "ENVARC:", "T:", "RAM:", "DOPUS5:", "REXX:", "HELP:", "LOCALE:",
                 "PROGDIR:", "D5THEMES:",
                 "NIL:", "CON:", "RAW:", "CONSOLE:", "PIPE:", "PRT:", "PAR:", "SER:")


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


PIPE = object()  # a "|", "||" or "&&" between two commands


class Quoted(str):
    """A word that was quoted, so the shell takes it literally."""


def shell_lex(line):
    """Split one line into words and command separators the way the AmigaDOS
    shell does.  A quote starts quoting only at the start of a word; inside
    quotes "*" escapes ("*"" is a quote, "*N" a newline, "**" a star) and a
    closing quote ends the word.  "|", "||" and "&&" outside quotes separate
    commands (shell V45+); an unquoted ";" starts a comment."""
    escapes = {'"': '"', "n": "\n", "N": "\n", "e": "\x1b", "E": "\x1b", "*": "*"}
    tokens, word, in_word, quoted, index = [], "", False, False, 0
    while index < len(line):
        char = line[index]
        if quoted:
            if char == "*" and index + 1 < len(line):
                word += escapes.get(line[index + 1], line[index:index + 2])
                index += 2
                continue
            if char == '"':
                tokens.append(Quoted(word))
                word, in_word, quoted = "", False, False
            else:
                word += char
            index += 1
            continue
        if char in " \t":
            if in_word:
                tokens.append(word)
                word, in_word = "", False
        elif char == ";":
            break
        elif char == "|" or line.startswith("&&", index):
            if in_word:
                tokens.append(word)
                word, in_word = "", False
            tokens.append(PIPE)
            index += 2 if line.startswith(("||", "&&"), index) else 1
            continue
        elif char == '"' and not in_word:
            quoted = in_word = True
        else:
            word += char
            in_word = True
        index += 1
    if in_word:
        tokens.append(Quoted(word) if quoted else word)
    return tokens


def shell_commands(text):
    """Return the commands a piece of shell text makes, as word lists: one
    per line and per side of each "|", "||" or "&&"."""
    commands = []
    for line in text.replace("\r", "\n").split("\n"):
        current = []
        for token in shell_lex(line):
            if token is PIPE:
                commands.append(current)
                current = []
            else:
                current.append(token)
        commands.append(current)
    return commands


def redirection(word):
    """Return the redirection operator a word starts with, or None."""
    if isinstance(word, Quoted):
        return None
    for operator in ("*>", ">>", ">", "<"):
        if word.startswith(operator):
            return operator
    return None


def parse_command(kind, words):
    """Return (program, arguments, redirection targets) of one command.

    kind 1 (AmigaDOS): skip a leading "run".  Kinds 1 and 2 (AmigaDOS,
    Workbench via dopusrt5) are shell command lines whose redirections the
    shell opens; they are collected wherever they appear, so a target
    cannot escape the assign check.  Quoted words are never redirections.
    Kinds 3 and 4 (Script, ARexx) get "execute " or "rx " prepended: the
    first word is the script."""
    words = list(words)
    if kind == 1 and words and words[0].lower() == "run":
        words = words[1:]
    if kind not in (1, 2):
        return (words[0] if words else ""), words[1:], []

    rest, targets = [], []
    while words:
        word = words.pop(0)
        operator = redirection(word)
        if operator:
            target = word[len(operator):]
            if not target and words:
                target = words.pop(0)
            targets.append(target)
        else:
            rest.append(word)
    return (rest[0] if rest else ""), rest[1:], targets


def instruction_commands(inst, text):
    """Return (program, arguments, redirection targets) for every command an
    external instruction makes.  The once-only marker is removed once, and
    for Workbench instructions a literal "run " in the first byte, as
    function_script.c does.  The first command follows the instruction's
    own rules; the others are AmigaDOS commands."""
    text = command_text(text)
    if inst == 2 and text[:4].lower() == "run ":
        text = text[4:]
    return [parse_command(inst if index == 0 else 1, words)
            for index, words in enumerate(shell_commands(text))]


def program_word(inst, text):
    """Return the program or script the first command of an instruction runs."""
    commands = instruction_commands(inst, text)
    return commands[0][0] if commands else ""


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
            # neither does a command line with nothing to run ("run >NIL:").
            # Keep the raw text: program_word() and command_text() remove the
            # once-only marker themselves, exactly once, like Opus.
            usable = []
            for inst, raw in func.instructions:
                if inst == INST_LABEL:
                    continue
                text = raw.decode("latin-1")

                if inst == 0:
                    # function_parse.c looks internal commands up by their raw
                    # text, before the once-only marker is handled:
                    # function_find_internal() compares a command name from the
                    # first byte and needs a space or the end of the string
                    # after it.  The name Opus looks up is the text up to the
                    # first space; other whitespace, a '#' or a control
                    # character in it means no command is found.
                    name = text.split(" ", 1)[0]
                    if name and name.isprintable() and not any(c.isspace() for c in name) and name[0] != "#":
                        usable.append((inst, text))
                    elif text.strip():
                        problems.append("%s: internal command %r is looked up as %r, so Opus finds no "
                                        "command to run" % (ftype.name, text, name))
                    continue

                # External instructions go into a script verbatim, so a line
                # break or a "|", "||" or "&&" starts another command.  The
                # first follows the instruction's own rules, the others are
                # AmigaDOS commands.
                # Tabs are ordinary shell whitespace; other control
                # characters (line breaks above all) are not
                if any(char != "\t" and not char.isprintable() for char in text):
                    problems.append("%s: instruction %r contains control characters; a line break in "
                                    "it starts another command in the script Opus writes" % (ftype.name, text))
                commands = instruction_commands(inst, text)
                if any(program for program, _arguments, _targets in commands):
                    usable.append((inst, text))
                for program, _arguments, targets in commands:
                    # The shell also has to open redirection targets
                    for used in [program] + targets:
                        prefix = path_prefix(used)
                        if prefix and prefix not in STOCK_ASSIGNS:
                            problems.append("%s: needs %s for %r" % (ftype.name, prefix, text))
                # function_script.c strips only a literal "run " in the first
                # byte; any other form of run reaches dopusrt5 as the program
                if inst == 2 and commands and commands[0][0].lower() == "run":
                    problems.append("%s: Workbench instruction %r keeps \"run\" (Opus only removes a "
                                    "leading \"run \"), so dopusrt5 is asked to start a program called run"
                                    % (ftype.name, text))

            if not usable:
                if not func.is_menu or label != b"---":
                    problems.append("%s: %s has no commands" % (ftype.name, function_text(func)))
                continue
            # A "---" label only adds a separator; the commands are unreachable
            if func.is_menu and label == b"---":
                problems.append("%s: %s is a separator, so its commands never run"
                                % (ftype.name, function_text(func)))
            # A drag'n'drop action that is only a plain Copy (internal, or an
            # AmigaDOS Copy after run/redirections) replaces the default
            if not func.is_menu and func.event == DRAG_DROP and len(usable) == 1:
                inst, text = usable[0]
                if inst == 0:
                    plain = [word.lower() for word in text.split()] == ["copy"]
                else:
                    # Only a single shell command (no second line or operator;
                    # a comment is fine) with no arguments is a plain Copy
                    commands = instruction_commands(inst, text)
                    plain = inst == 1 and len(commands) == 1 and commands[0][0].lower() == "copy" \
                        and not commands[0][1]
                if plain:
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
