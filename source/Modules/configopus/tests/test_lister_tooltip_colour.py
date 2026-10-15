#!/usr/bin/env python3
"""Regression checks for configurable lister toolbar tooltip colours."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[4]
CONFIGOPUS_DIR = ROOT / "source" / "Modules" / "configopus"
CONFIG_ENVIRONMENT_C = CONFIGOPUS_DIR / "config_environment.c"
CONFIGOPUS_CD = CONFIGOPUS_DIR / "configopus.cd"
STRING_DATA_H = CONFIGOPUS_DIR / "string_data.h"
DOPUS5_H = ROOT / "source" / "Include" / "libraries" / "dopus5.h"
DOPUS_CONFIG_H = ROOT / "source" / "Program" / "dopus_config.h"
CONFIG_DEFAULT_C = ROOT / "source" / "Library" / "config_default.c"
LISTER_TOOLBAR_C = ROOT / "source" / "Program" / "lister_toolbar.c"
LISTER_WINDOW_C = ROOT / "source" / "Program" / "lister_window.c"
GUIDE = ROOT / "documents" / "DOpus5.guide"


def read_source(path):
    return path.read_text(encoding="latin-1")


class ListerTooltipColourTests(unittest.TestCase):
    def test_environment_structure_has_tooltip_colour_slot(self):
        for header in (DOPUS5_H, DOPUS_CONFIG_H):
            with self.subTest(header=header.name):
                source = read_source(header)
                self.assertIn("#define CONFIG_VERSION_17 17", source)
                self.assertIn("UBYTE tooltip_col[2];", source)
                self.assertRegex(source, r"(?s)ENVCOL_GAUGE,\s*\n\s*ENVCOL_TOOLTIP,")
                self.assertIn("ULONG pad[13];", source)

    def test_default_and_legacy_environment_initialise_tooltip_colours(self):
        source = read_source(CONFIG_DEFAULT_C)

        self.assertIn("env->tooltip_col[0] = 1;", source)
        self.assertIn("env->tooltip_col[1] = 0;", source)
        self.assertIn("if (env->version < CONFIG_VERSION_16)", source)
        self.assertIn("env->version = CONFIG_VERSION_17;", source)

    def test_lister_colours_editor_exposes_tooltip_item(self):
        config_source = read_source(CONFIG_ENVIRONMENT_C)
        cd_source = read_source(CONFIGOPUS_CD)
        string_source = read_source(STRING_DATA_H)
        guide_source = read_source(GUIDE)

        self.assertIn("MSG_ENVIRONMENT_TOOLTIP_COLOUR", cd_source)
        self.assertIn("Tooltips", cd_source)
        self.assertIn('#define MSG_ENVIRONMENT_TOOLTIP_COLOUR_STR "Tooltips"', string_source)
        self.assertIn("MSG_ENVIRONMENT_TOOLTIP_COLOUR, (CONST_STRPTR)MSG_ENVIRONMENT_TOOLTIP_COLOUR_STR", string_source)
        self.assertIn("MSG_ENVIRONMENT_TOOLTIP_COLOUR", config_source)
        self.assertIn("ptr = data->config->tooltip_col;", config_source)
        self.assertIn("data->config->tooltip_col[a] != env->env->tooltip_col[a]", config_source)
        self.assertIn("'Tooltips' item controls", guide_source)

    def test_toolbar_tooltip_uses_lister_colour_slot(self):
        toolbar_source = read_source(LISTER_TOOLBAR_C)
        window_source = read_source(LISTER_WINDOW_C)

        self.assertIn("lister_init_colour(lister, ENVCOL_TOOLTIP, FALSE);", toolbar_source)
        self.assertIn("lister_init_colour(lister, ENVCOL_TOOLTIP, TRUE);", toolbar_source)
        self.assertIn("lister_toolbar_tooltip_close(lister);", toolbar_source)
        self.assertIn("tip_fpen = lister->lst_Colours[ENVCOL_TOOLTIP].cr_Pen[0];", toolbar_source)
        self.assertIn("tip_bpen = lister->lst_Colours[ENVCOL_TOOLTIP].cr_Pen[1];", toolbar_source)
        self.assertRegex(toolbar_source, r"(?s)SetAPen\(rp, tip_bpen\);\s*RectFill")
        self.assertRegex(toolbar_source, r"(?s)SetAPen\(rp, tip_fpen\);\s*SetBPen\(rp, tip_bpen\);")
        tick_body = toolbar_source.split("void lister_toolbar_tooltip_tick", 1)[1].split(
            "static void lister_toolbar_tooltip_show", 1
        )[0]
        self.assertNotIn("CloseWindow", tick_body)
        self.assertIn("case ENVCOL_TOOLTIP:", window_source)


if __name__ == "__main__":
    unittest.main()
