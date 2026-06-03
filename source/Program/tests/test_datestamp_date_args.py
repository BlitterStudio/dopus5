#!/usr/bin/env python3
"""Regression checks for DateStamp date argument handling."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[3]
FUNCTION_CHANGE_C = ROOT / "source" / "Program" / "function_change.c"


def read_source():
    return FUNCTION_CHANGE_C.read_text(encoding="latin-1")


class DateStampDateArgumentTests(unittest.TestCase):
    def test_empty_date_argument_uses_current_datestamp_before_parsing(self):
        source = read_source()
        setup = source[source.index("// Arguments supplied?") : source.index("// Any directories selected?")]

        self.assertIn("function_change_arg_empty", source)
        self.assertIn("if (function_change_arg_empty((char *)instruction->funcargs->FA_Arguments[2]))", setup)
        self.assertRegex(
            setup,
            re.compile(
                r"function_change_arg_empty\(\(char \*\)instruction->funcargs->FA_Arguments\[2\]\)\)\s*"
                r"DateStamp\(&data->date\);.*?"
                r"else\s*\{.*?ParseDateStrings",
                re.S,
            ),
        )

    def test_empty_arg_helper_handles_quoted_empty_strings(self):
        source = read_source()
        helper = source[source.index("static BOOL function_change_arg_empty") :]

        self.assertIn("*arg == '\"'", helper)
        self.assertIn("*arg == '\\''", helper)
        self.assertRegex(helper, r"while \(\*arg && isspace\(\*arg\)\)")
        self.assertRegex(helper, r"return \(end == arg\);")


if __name__ == "__main__":
    unittest.main()
