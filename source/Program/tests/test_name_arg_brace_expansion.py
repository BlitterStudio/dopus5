#!/usr/bin/env python3
"""Static checks for {fu}/{ou} NAME arguments on internal commands (issue #141).

An internal command (e.g. Protect/Comment) invoked with NAME={fu} or NAME={ou} must operate
on the whole current selection and leave it selected. The mechanism: drop the NAME so the
entry list is built from the live selection, then rewind and flag every entry no-unselect
before the command runs.
"""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[3]
PROGRAM = ROOT / "source" / "Program"


def read(name):
    return (PROGRAM / name).read_text(encoding="latin-1")


class NameArgBraceExpansionTests(unittest.TestCase):
    def test_standalone_helper_matches_codes_and_rejects_literals(self):
        files_c = read("function_files.c")
        self.assertIn("static BOOL function_arg_is_no_unselect_only(const char *arg)", files_c)
        body = files_c[files_c.index("static BOOL function_arg_is_no_unselect_only") :]
        body = body[: body.index("\n}\n")]
        # Must recognise both no-unselect codes ({ou} = FUNC_ONE_FILE_NO_UNSELECT,
        # {fu} = FUNC_ONE_PATH_NO_UNSELECT)
        self.assertIn("FUNC_ONE_FILE_NO_UNSELECT", body)
        self.assertIn("FUNC_ONE_PATH_NO_UNSELECT", body)
        # Tolerate the code's own quote/suffix modifier bytes so plain {fu}/{ou} still match
        self.assertIn("FUNC_NORMAL", body)
        # Reject anything else (literal text, e.g. {fu}.info) via a default that returns 0
        self.assertIn("default:", body)
        self.assertIn("return 0;", body)

    def test_gate_is_scoped_to_standalone_no_unselect_internal_commands(self):
        files_c = read("function_files.c")
        self.assertIn("short function_brace_name_arg(InstructionParsed *instruction)", files_c)
        gate = files_c[files_c.index("short function_brace_name_arg") :]
        gate = gate[: gate.index("\n}\n")]
        # Internal commands only, excluding external functions
        self.assertIn("INST_COMMAND", gate)
        self.assertIn("FUNCF_EXTERNAL_FUNCTION", gate)
        # The file-supplying argument
        self.assertIn("FUNCKEY_FILE", gate)
        self.assertIn("FUNCKEY_FILENO", gate)
        # Scoped to a STANDALONE no-unselect code, not any arg that merely contains one
        self.assertIn("function_arg_is_no_unselect_only", gate)
        # The old generic control-byte helper must be gone
        self.assertNotIn("function_arg_has_control_bytes", files_c)

    def test_drop_clears_the_name_argument(self):
        files_c = read("function_files.c")
        self.assertIn("void function_drop_brace_name_arg(InstructionParsed *instruction)", files_c)
        body = files_c[files_c.index("void function_drop_brace_name_arg") :]
        body = body[: body.index("\n}\n")]
        # Gate on the predicate, then null the NAME so build_list gathers the selection
        self.assertIn("function_brace_name_arg(instruction)", body)
        self.assertIn("instruction->funcargs->FA_Arguments[num] = 0;", body)
        # The throwaway gather/expand/discard approach must be gone
        self.assertNotIn("NewList(&handle->entry_list);", body)
        self.assertNotIn("function_parse_arguments", body)

    def test_prototypes_present(self):
        launch_h = read("function_launch.h")
        self.assertIn("void function_drop_brace_name_arg(InstructionParsed *);", launch_h)
        self.assertIn("short function_brace_name_arg(InstructionParsed *);", launch_h)
        # The no-unselect helper is file-local (static), so not exported
        self.assertNotIn("function_arg_is_no_unselect_only", launch_h)
        self.assertNotIn("function_arg_has_no_unselect", launch_h)
        # The old resolver prototype must be gone
        self.assertNotIn("function_resolve_name_args", launch_h)

    def test_run_drops_name_before_building_list(self):
        run_c = read("function_run.c")
        idx_drop = run_c.index("function_drop_brace_name_arg(instruction)")
        idx_build = run_c.index("function_build_list(handle, &path, instruction)")
        self.assertLess(idx_drop, idx_build)

    def test_run_rewinds_and_keeps_whole_selection_selected(self):
        run_c = read("function_run.c")
        # The rewind/flag lives between argument parsing and the command call
        block = run_c[run_c.index("function_parse_arguments(handle, instruction);") :]
        block = block[: block.index("function_internal_command")]
        self.assertIn("function_brace_name_arg(instruction) >= 0", block)
        # Rewind current_entry to the head of the selection
        self.assertIn("handle->current_entry = (FunctionEntry *)handle->entry_list.lh_Head;", block)
        # Walk the whole list marking every entry no-unselect
        self.assertIn("bent->node.mln_Succ", block)
        self.assertIn("bent->flags |= FUNCENTF_NO_UNSELECT;", block)

    def test_change_reselects_no_unselect_entries(self):
        # Changing protection/comment/date replaces the lister entry, which clears its selected
        # state; a no-unselect entry must be reselected via FCF_SELECT on the filechange.
        change_c = read("function_change.c")
        block = change_c[change_c.index("function_filechange_addfile") :]
        block = block[: block.index("FUNCENTF_REMOVE")]
        self.assertIn("entry->flags & FUNCENTF_NO_UNSELECT", block)
        self.assertIn("FCF_SELECT", block)


if __name__ == "__main__":
    unittest.main()
