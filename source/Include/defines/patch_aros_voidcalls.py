#!/usr/bin/env python3
"""Patch generated AROS inline headers for ABIv11 void-return calls.

AROS_LCn(void, ...) does not compile on x86_64, where the macro stores the
result in a variable. AROS_LCnNR(void, ...) is the system's form for calls
without a result; like AROS_LCn it loads the library base into r12 on x86_64.
"""

from pathlib import Path
import re


DEFINE_DIR = Path(__file__).resolve().parent
HEADERS = (
    "dopus5.h",
    "newicon.h",
    "music.h",
    "multiuser.h",
    "SysInfo.h",
)
# Earlier versions of this script routed void calls through this header and
# inserted its include after one of these lines
OLD_VOIDCALL_INCLUDE = "#include <defines/aros_voidcall.h>"
OLD_INCLUDE_ANCHORS = (
    ("#include <aros/preprocessor/variadic/cast2iptr.hpp>", "\n"),
    ("#endif /* !AROS_LIBCALL_H */", "\n\n"),
)


def remove_old_voidcall_include(text):
    for anchor, separator in OLD_INCLUDE_ANCHORS:
        text = text.replace(f"{anchor}{separator}{OLD_VOIDCALL_INCLUDE}", anchor, 1)
    return text


def remove_dopus_local_helpers(text):
    start = text.find("#define DOPUS_LC0NR")
    if start == -1:
        return text

    end_marker = "#define ActivateStrGad"
    end = text.find(end_marker, start)
    if end == -1:
        return text

    return text[:start] + text[end:]


def patch_header(path):
    text = path.read_text(encoding="utf-8")
    text = text.replace("\r\n", "\n")
    text = remove_dopus_local_helpers(text)
    text = re.sub(r"\bDOPUS_LC([0-9]+)NR\(", r"AROS_LC\1NR(void, ", text)
    text = re.sub(r"\bAROS_VOID_LC([0-9]+)\(", r"AROS_LC\1NR(void, ", text)
    text = re.sub(r"\bAROS_LC([0-9]+)\(\s*void\s*,\s*", r"AROS_LC\1NR(void, ", text)
    text = remove_old_voidcall_include(text)
    path.write_text(text, encoding="utf-8", newline="\n")


def main():
    for header in HEADERS:
        patch_header(DEFINE_DIR / header)


if __name__ == "__main__":
    main()
