#!/usr/bin/env python3
"""Regression checks for the OS3 screen-title clock path."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[3]
CLOCK_TASK_C = ROOT / "source" / "Program" / "clock_task.c"
MAKEFILE_COMMON = ROOT / "source" / "makefile.common"


def read_source(path):
    return path.read_text(encoding="latin-1")


class OS3ScreenTitleClockTests(unittest.TestCase):
    def test_os3_uses_clock_local_screen_title_mode(self):
        clock_source = read_source(CLOCK_TASK_C)
        make_source = read_source(MAKEFILE_COMMON)

        self.assertIn("#if defined(__amigaos3__) || defined(USE_SCREENTITLE)", clock_source)
        self.assertIn("#define CLOCK_USE_SCREENTITLE", clock_source)
        self.assertNotIn("-DUSE_SCREENTITLE", make_source)

    def test_os3_screen_title_setup_skips_public_screen_lock(self):
        source = read_source(CLOCK_TASK_C)

        os3_branch = source.index("#if defined(__amigaos3__) && defined(CLOCK_USE_SCREENTITLE)")
        lock_branch = source.index("if (FindPubScreen(screen, TRUE))", os3_branch)

        self.assertLess(os3_branch, lock_branch)
        self.assertIn("Screen title mode does not touch the screen layers directly.", source)

    def test_os3_screen_title_path_keeps_clock_right_aligned(self):
        source = read_source(CLOCK_TASK_C)

        self.assertIn("clock_append_screen_title_clock(struct RastPort *rp, short clock_x, char *titlebuf)", source)
        self.assertIn("short title_width = TextLength(rp, GUI->screen_title, title_len);", source)
        self.assertIn("short target_x = clock_x - 5;", source)
        self.assertIn("TextLength(&GUI->screen_pointer->RastPort, titlebuf, (WORD)strlen(titlebuf))", source)
        self.assertIn("clock_x = (clock_width < bar_x) ? bar_x - clock_width : 0", source)
        self.assertIn("clock_append_screen_title_clock(", source)

    def test_os3_screen_title_clock_omits_seconds_without_throttling_ram_counter(self):
        source = read_source(CLOCK_TASK_C)

        self.assertIn("show_seconds = FALSE;", source)
        self.assertIn("clock_build_title_text(&date, datebuf, titlebuf, show_seconds);", source)
        self.assertIn('format = (show_seconds) ? "%Q:%M:%S %p" : "%Q:%M %p";', source)
        self.assertIn("FormatDate(locale.li_Locale, format, stamp, &hook);", source)
        self.assertIn("REG(a1, IPTR ch)", source)
        self.assertIn("GetLocaleStr(locale.li_Locale, (hours > 11) ? PM_STR : AM_STR)", source)
        self.assertNotIn("%lcm", source)
        self.assertIn("strcmp(last_screen_title, GUI->screen_title) != 0", source)
        self.assertNotIn("refresh_titlebar", source)
        self.assertNotIn("last_title_minute", source)

    def test_os3_screen_title_updates_when_active_window_changes(self):
        source = read_source(CLOCK_TASK_C)

        self.assertIn("struct Window *last_title_window = 0;", source)
        self.assertIn("active_window = IntuitionBase->ActiveWindow;", source)
        self.assertIn("tit_window = active_window;", source)
        self.assertIn("last_title_window != active_window", source)
        self.assertIn("SetWindowTitles(active_window, (char *)-1, (char *)GUI->screen_title);", source)
        self.assertIn("last_title_window = active_window;", source)

    def test_custom_clock_format_and_screen_title_token_use_locale_formatter(self):
        source = read_source(CLOCK_TASK_C)

        self.assertIn("char *format = environment->env->clock_format;", source)
        self.assertIn("clock_format_date(titlebuf, TITLE_SIZE, format, &date->dat_Stamp)", source)
        self.assertIn("FormatDate(locale.li_Locale, format, stamp, &hook);", source)
        self.assertIn("clock_title_format_uses_clock(environment->env->scr_title_text)", source)
        self.assertIn("(custom_title_uses_clock) ? titlebuf : 0", source)
        self.assertIn("Clock text", source)

    def test_clock_format_hook_uses_standard_hook_argument_order(self):
        source = read_source(CLOCK_TASK_C)

        # A0=hook, A2=object, A1=message. On every non-m68k target REG() is a no-op, so the
        # declaration order is the calling convention: the character must be the third parameter.
        self.assertIn(
            "clock_format_hook(REG(a0, struct Hook *hook), REG(a2, APTR dummy), REG(a1, IPTR ch))",
            source,
        )
        self.assertIn("return clock_format_append(hook, (ULONG)ch);", source)

        # A single definition has to serve all targets; no platform-specific parameter orders.
        self.assertEqual(source.count("clock_format_hook(REG("), 1)
        self.assertNotIn("clock_format_hook(struct Hook *hook, APTR dummy, IPTR ch)", source)
        self.assertNotIn("REG(a1, ULONG ch), REG(a2, APTR dummy)", source)

        self.assertIn("#if defined(__AROS__) || defined(__MORPHOS__)", source)
        self.assertIn("hook.h_Entry = (HOOKFUNC)HookEntry;", source)
        self.assertIn("hook.h_SubEntry = (HOOKFUNC)clock_format_hook;", source)

    def test_aros_clock_format_corrects_12_hour_tokens(self):
        source = read_source(CLOCK_TASK_C)

        self.assertIn("static BOOL clock_format_date_aros", source)
        self.assertIn("AROS locale.library expands 12-hour tokens as 0-11", source)
        self.assertIn("clock_format_append_12hour(buffer, size, &pos, stamp, TRUE);", source)
        self.assertIn("clock_format_append_12hour(buffer, size, &pos, stamp, FALSE);", source)
        self.assertIn("case 'r':", source)
        self.assertIn('lsprintf(timebuf, ":%02ld:%02ld "', source)
        self.assertIn("clock_format_append_ampm(buffer, size, &pos, stamp);", source)
        self.assertIn("return clock_format_date_aros(buffer, size, format, stamp);", source)


if __name__ == "__main__":
    unittest.main()
