#!/usr/bin/env python3
"""Regression checks for Duplicate datestamp handling."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[3]
PROGRAM = ROOT / "source" / "Program"


def read_program_source(name):
    return (PROGRAM / name).read_text(encoding="latin-1")


class DuplicateDatestampTests(unittest.TestCase):
    def test_duplicate_does_not_restore_source_datestamp(self):
        function_data_c = read_program_source("function_data.c")
        function_copy_c = read_program_source("function_copy.c")

        self.assertRegex(
            function_data_c,
            r'(?s)FUNC_CLONE,\s*"Duplicate".*?function_copy,',
        )

        copy_setup = re.search(
            r"(?s)function = command->function;\s*"
            r"copy_flags = environment->env->settings.copy_flags;.*?"
            r"// Icon copy\?",
            function_copy_c,
        )
        self.assertIsNotNone(copy_setup)
        self.assertRegex(
            copy_setup.group(0),
            r"if \(function == FUNC_CLONE\)\s*"
            r"copy_flags &= ~COPY_DATE;",
        )

        clone_clear = function_copy_c.index("copy_flags &= ~COPY_DATE;")
        copy_date_restore = function_copy_c.index("// Copy date?")
        self.assertLess(clone_clear, copy_date_restore)


if __name__ == "__main__":
    unittest.main()
