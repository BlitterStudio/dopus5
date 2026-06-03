#!/usr/bin/env python3
"""Regression checks for empty Comment argument handling (issue #141)."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[3]
FUNCTION_CHANGE_C = ROOT / "source" / "Program" / "function_change.c"


def read_source():
    return FUNCTION_CHANGE_C.read_text(encoding="latin-1")


class CommentEmptyArgumentTests(unittest.TestCase):
    def test_shared_empty_arg_predicate_exists(self):
        source = read_source()
        self.assertIn("static BOOL function_change_arg_empty(char *arg)", source)
        # Old date-specific name must be fully gone
        self.assertNotIn("function_change_date_arg_empty", source)

    def test_empty_comment_clears_filenote_without_prompting(self):
        source = read_source()
        block = source[
            source.index("if (command->function == FUNC_COMMENT)")
            : source.index("else if (command->function == FUNC_PROTECT)")
        ]
        self.assertIn(
            "function_change_arg_empty((char *)instruction->funcargs->FA_Arguments[1])",
            block,
        )
        self.assertIn("data->comment[0] = 0;", block)
        self.assertIn("INSTF_NO_ASK", block)


if __name__ == "__main__":
    unittest.main()
