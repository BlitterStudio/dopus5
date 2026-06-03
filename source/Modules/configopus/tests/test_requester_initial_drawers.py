#!/usr/bin/env python3
"""Regression checks for requester initial drawer path buffers."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[4]
CONFIGOPUS_DIR = ROOT / "source" / "Modules" / "configopus"
FUNCTION_EDITOR_C = CONFIGOPUS_DIR / "function_editor.c"
FILETYPE_EDITOR_C = CONFIGOPUS_DIR / "filetype_editor.c"
PALETTE_C = CONFIGOPUS_DIR / "config_environment_palette.c"


def read_source(path):
    return path.read_text(encoding="latin-1")


def source_between(source, start_text, end_text):
    start = source.index(start_text)
    end = source.index(end_text, start)
    return source[start:end]


def assert_buffer_cleared_before_lock(testcase, source, buffer_name, assign_name):
    declaration = f"char {buffer_name}[256];"
    lock_call = f'Lock("{assign_name}", ACCESS_READ)'

    start = source.index(declaration)
    end = source.index(lock_call, start)
    init = source.index(f"{buffer_name}[0] = 0;", start)

    testcase.assertLess(init, end)


def assert_namefromlock_failure_clears_buffer(testcase, source, lock_name, buffer_name, count=1):
    pattern = (
        r"if\s*\(\s*!\s*\(?\s*NameFromLock\s*\(\s*"
        + re.escape(lock_name)
        + r"\s*,\s*"
        + re.escape(buffer_name)
        + r"\s*,\s*sizeof\s*\(\s*"
        + re.escape(buffer_name)
        + r"\s*\)\s*\)\s*\)?\s*\)\s*"
        + re.escape(buffer_name)
        + r"\[0\]\s*=\s*0\s*;"
    )

    testcase.assertGreaterEqual(len(re.findall(pattern, source)), count)


class RequesterInitialDrawerTests(unittest.TestCase):
    def test_function_export_drawer_buffer_is_cleared_before_resolution(self):
        source = read_source(FUNCTION_EDITOR_C)
        block = source_between(source, "char path1[256];", "tags[4].ti_Tag")

        assert_buffer_cleared_before_lock(self, block, "path1", "DOpus5:Commands")
        assert_namefromlock_failure_clears_buffer(self, block, "lock1", "path1")

    def test_function_command_drawers_clear_on_resolution_failure(self):
        source = read_source(FUNCTION_EDITOR_C)
        block = source_between(source, "char path2[256];", "tags[3].ti_Tag")

        assert_namefromlock_failure_clears_buffer(self, block, "lock2", "path2", count=2)

    def test_filetype_icon_drawer_clears_on_resolution_failure(self):
        source = read_source(FILETYPE_EDITOR_C)
        block = source_between(source, 'Lock("ENVARC:Sys", ACCESS_READ)', "// Build pattern")

        assert_namefromlock_failure_clears_buffer(self, block, "lock", "path")

    def test_palette_drawer_buffer_is_cleared_before_resolution(self):
        source = read_source(PALETTE_C)
        block = source_between(source, "char path3[256];", "if (AslRequestTags")

        assert_buffer_cleared_before_lock(self, block, "path3", "sys:prefs/presets")
        assert_namefromlock_failure_clears_buffer(self, block, "lock3", "path3")


if __name__ == "__main__":
    unittest.main()
