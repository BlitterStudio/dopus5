#!/usr/bin/env python3
"""Static checks for the desktop selection fallback (issue #155).

User Menu and Button Bank launches must honour selected desktop icons when no
lister provides a selection, mirroring the icon context menu. These checks pin
the three pieces of wiring: the lister-selection guard, the desktop-selection
collection helper, and the two launch sites that consult them.
"""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[3]
PROGRAM = ROOT / "source" / "Program"


def read_program_source(name):
    return (PROGRAM / name).read_text(encoding="latin-1")


class DesktopSelectionFallbackTests(unittest.TestCase):
    def test_lister_selection_guard_present(self):
        function_launch_c = read_program_source("function_launch.c")
        function_launch_h = read_program_source("function_launch.h")

        # Guard is declared and defined
        self.assertIn("BOOL lister_has_selection(void);", function_launch_h)
        self.assertIn("BOOL lister_has_selection(void)", function_launch_c)

        # Everything after the definition is the guard body (it is the last
        # function in the file), so it must carry the locking and candidacy.
        body = function_launch_c.split("BOOL lister_has_selection(void)", 1)[1]
        self.assertIn("lock_listlock(&GUI->lister_list", body)
        self.assertIn("unlock_listlock(&GUI->lister_list)", body)
        self.assertIn("LISTERF_SOURCE", body)
        self.assertIn("LISTERF_BUSY", body)
        self.assertIn("ENTF_SELECTED", body)

    def test_desktop_collection_helper_present(self):
        icon_function_c = read_program_source("icon_function.c")
        icons_h = read_program_source("icons.h")

        # Helper is declared and defined
        self.assertIn("desktop_selection_argarray", icons_h)
        self.assertIn(
            "struct ArgArray *desktop_selection_argarray(BackdropInfo *info)",
            icon_function_c,
        )

        # The body supplies drive/disk icons by device name, locks the object
        # list while iterating, and builds argument-array entries.
        body = icon_function_c.split(
            "struct ArgArray *desktop_selection_argarray(BackdropInfo *info)", 1
        )[1]
        self.assertIn("device_name", body)
        self.assertIn("lock_listlock(&info->objects", body)
        self.assertIn("unlock_listlock(&info->objects)", body)
        self.assertIn("NewArgArrayEntry", body)
        self.assertIn("BDO_DISK", body)
        self.assertIn("BDO_LEFT_OUT", body)

    def test_launch_sites_wired(self):
        event_loop_c = read_program_source("event_loop.c")
        buttons_run_c = read_program_source("buttons_run.c")

        # Both launch sites consult the guard and the collection helper.
        for source in (event_loop_c, buttons_run_c):
            self.assertIn("lister_has_selection", source)
            self.assertIn("desktop_selection_argarray", source)


if __name__ == "__main__":
    unittest.main()
