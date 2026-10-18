#!/usr/bin/env python3
"""Static checks for destination-lister argument expansion."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[3]
PROGRAM = ROOT / "source" / "Program"
CONFIGOPUS = ROOT / "source" / "Modules" / "configopus"


def read_program_source(name):
    return (PROGRAM / name).read_text(encoding="latin-1")


def read_repo_file(*parts):
    return ROOT.joinpath(*parts).read_text(encoding="latin-1")


def read_configopus_source(name):
    return (CONFIGOPUS / name).read_text(encoding="latin-1")


class DestinationEntryArgumentTests(unittest.TestCase):
    def test_function_handle_keeps_destination_entries_separate(self):
        function_launch_h = read_program_source("function_launch.h")
        function_files_c = read_program_source("function_files.c")

        self.assertIn("struct List dest_entry_list;", function_launch_h)
        self.assertIn("FunctionEntry *dest_current_entry;", function_launch_h)
        self.assertIn("PathNode *dest_entry_path;", function_launch_h)
        self.assertIn("char dest_last_filename[512];", function_launch_h)
        self.assertIn("int function_build_dest_list(FunctionHandle *function, PathNode **, InstructionParsed *);",
                      function_launch_h)
        self.assertIn("FunctionEntry *function_get_dest_entry(FunctionHandle *handle);", function_launch_h)
        self.assertIn("NewList(&handle->dest_entry_list);", function_files_c)
        self.assertIn("function_new_entry_on_list(handle, &handle->dest_entry_list", function_files_c)
        self.assertNotIn("function_end_entry(handle, entry", self._dest_build_section(function_files_c))

    def test_parser_recognizes_destination_entry_tokens(self):
        function_data_h = read_program_source("function_data.h")
        function_launch_h = read_program_source("function_launch.h")
        function_parse_c = read_program_source("function_parse.c")

        for symbol in (
            "FUNC_DEST_LAST_FILE",
            "FUNC_DEST_LAST_PATH",
            "FUNC_DEST_ONE_FILE",
            "FUNC_DEST_ALL_FILES",
            "FUNC_DEST_ONE_PATH",
            "FUNC_DEST_ALL_PATHS",
        ):
            self.assertIn(symbol, function_data_h)

        self.assertIn("#define FUNCF_NEED_DEST_ENTRIES", function_launch_h)
        self.assertIn("#define FUNCF_WANT_DEST_ENTRIES", function_launch_h)
        self.assertIn("#define FUNCF_LAST_DEST_FILE_FLAG", function_launch_h)
        self.assertRegex(
            function_parse_c,
            r"(?s)case 'd':.*"
            r"string\[func_pos \+ 1\] == 'f'.*"
            r"string\[func_pos \+ 1\] == 'F'.*"
            r"string\[func_pos \+ 1\] == 'o'.*"
            r"string\[func_pos \+ 1\] == 'O'.*"
            r"FUNCF_NEED_DEST \| FUNCF_NEED_DEST_ENTRIES",
        )
        self.assertIn("FUNCF_WANT_DEST | FUNCF_WANT_DEST_ENTRIES", function_parse_c)

    def test_build_instruction_consumes_destination_entries_only(self):
        function_data_c = read_program_source("function_data.c")
        function_parse_c = read_program_source("function_parse.c")
        function_run_c = read_program_source("function_run.c")

        self.assertIn("static int function_prepare_dest_entries", function_parse_c)
        self.assertIn("function_build_dest_list(handle, &path, ins)", function_parse_c)
        self.assertIn("case FUNC_DEST_ONE_PATH:", function_parse_c)
        self.assertIn("case FUNC_DEST_ONE_FILE:", function_parse_c)
        self.assertIn("case FUNC_DEST_ALL_PATHS:", function_parse_c)
        self.assertIn("case FUNC_DEST_ALL_FILES:", function_parse_c)
        self.assertIn("function_get_dest_entry(handle)", function_parse_c)
        self.assertIn("function_end_dest_entry(handle, entry)", function_parse_c)
        self.assertIn("handle->dest_last_filename", function_parse_c)
        self.assertNotRegex(
            self._dest_parse_section(function_parse_c),
            r"function_end_entry\(handle, entry",
        )
        self.assertIn("Destination selections are snapshotted per instruction.", function_run_c)
        self.assertGreaterEqual(function_run_c.count("handle->dest_entry_path = 0;"), 2)
        self.assertIn("parse_ret = function_build_instruction(handle, instruction, 0, args);", function_run_c)
        self.assertIn("if (parse_ret >= PARSE_OK)", function_run_c)
        for template_key in ('"f0a1d2"', '"f0a1"', '"F0a1"'):
            self.assertIn(template_key, function_data_c)

    def test_internal_commands_without_arguments_still_run(self):
        function_data_c = read_program_source("function_data.c")
        function_run_c = read_program_source("function_run.c")

        self.assertIn('"DeviceList"', function_data_c)
        self.assertIn('"NEW/S,FULL/S,BRIEF/S"', function_data_c)
        self.assertIn("if (instruction->string && instruction->string[0])", function_run_c)
        self.assertIn("args[0] = 0;", function_run_c)
        self.assertIn("parse_ret = PARSE_OK;", function_run_c)

    def test_readargs_argument_expansion_does_not_literalize_quotes(self):
        function_parse_c = read_program_source("function_parse.c")

        self.assertIn("Parse a single ReadArgs value without adding command-line quotes.", function_parse_c)
        self.assertRegex(
            function_parse_c,
            r"(?s)old_flags = handle->func_parameters.flags;.*"
            r"handle->func_parameters.flags \|= FUNCF_NO_QUOTES;.*"
            r"function_build_instruction\(handle, ins, \(unsigned char \*\)ins->funcargs->FA_ArgArray\[arg\], buf\);.*"
            r"handle->func_parameters.flags = old_flags;",
        )

    def test_user_facing_strings_docs_and_changelog_are_updated(self):
        string_data_h = read_configopus_source("string_data.h")
        configopus_cd = read_configopus_source("configopus.cd")
        guide = read_repo_file("documents", "DOpus5.guide")
        changelog = read_repo_file("ChangeLog")

        for token in ("{df}", "{dF}", "{do}", "{dO}", "{df!}", "{dF!}", "{do!}", "{dO!}"):
            self.assertIn(token, string_data_h)
            self.assertIn(token, configopus_cd)
        self.assertIn("Destination lister selections", guide)
        self.assertIn("Dual Lister mode the", guide)
        self.assertIn("destination lister with `{df}` / `{dF}`", changelog)
        self.assertIn("including the inactive panel in Dual Lister mode", changelog)

    @staticmethod
    def _dest_build_section(function_files_c):
        return re.search(
            r"(?s)int function_build_dest_list\(FunctionHandle \*handle.*?"
            r"\n}\n\n// Add a new entry",
            function_files_c,
        ).group(0)

    @staticmethod
    def _dest_parse_section(function_parse_c):
        return re.search(
            r"(?s)// Single destination pathname.*?"
            r"\n\t\t// Source directory",
            function_parse_c,
        ).group(0)


if __name__ == "__main__":
    unittest.main()
