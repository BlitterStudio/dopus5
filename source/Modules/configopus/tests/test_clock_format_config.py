#!/usr/bin/env python3
"""Regression checks for the title bar clock format setting."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[4]
CONFIGOPUS_DIR = ROOT / "source" / "Modules" / "configopus"
CONFIG_ENVIRONMENT_C = CONFIGOPUS_DIR / "config_environment.c"
CONFIG_ENVIRONMENT_DATA_C = CONFIGOPUS_DIR / "config_environment_data.c"
CONFIG_ENVIRONMENT_LISTER_C = CONFIGOPUS_DIR / "config_environment_lister.c"
CONFIGOPUS_CD = CONFIGOPUS_DIR / "configopus.cd"
STRING_DATA_H = CONFIGOPUS_DIR / "string_data.h"
DOPUS5_H = ROOT / "source" / "Include" / "libraries" / "dopus5.h"
DOPUS_CONFIG_H = ROOT / "source" / "Program" / "dopus_config.h"
CONFIG_DEFAULT_C = ROOT / "source" / "Library" / "config_default.c"


def read_source(path):
    return path.read_text(encoding="latin-1")


def read_layout_source(layout_name):
    source = read_source(CONFIG_ENVIRONMENT_DATA_C)
    pattern = re.escape(layout_name) + r"\[\]\s*=\s*\{(?P<body>.*?)\n\s*\{OD_END\}\}"
    match = re.search(pattern, source, re.S)
    if not match:
        raise AssertionError(f"Could not find {layout_name} layout")
    return match.group("body")


def read_case_source(function_name, case_name):
    source = read_source(CONFIG_ENVIRONMENT_C)
    start = source.find(f"void {function_name}")
    if start == -1:
        raise AssertionError(f"Could not find {function_name}")
    end = source.find("\nvoid ", start + 1)
    if end == -1:
        end = len(source)
    source = source[start:end]
    pattern = r"\n\tcase " + re.escape(case_name) + r":(?P<body>.*?)(?=\n\tcase |\n\tdefault:|\n\t})"
    match = re.search(pattern, source, re.S)
    if not match:
        raise AssertionError(f"Could not find {case_name} in {function_name}")
    return match.group("body")


class ClockFormatConfigTests(unittest.TestCase):
    def test_environment_structure_stores_clock_format_in_reserved_space(self):
        for header in (DOPUS5_H, DOPUS_CONFIG_H):
            with self.subTest(header=header.name):
                source = read_source(header)
                self.assertIn("#define CONFIG_VERSION_17 17", source)
                self.assertIn("char clock_format[80];", source)
                self.assertIn("ULONG pad_big[276];", source)

    def test_defaults_clear_clock_format_for_old_configs(self):
        source = read_source(CONFIG_DEFAULT_C)

        self.assertIn("if (env->version < CONFIG_VERSION_17)", source)
        self.assertIn("env->clock_format[0] = 0;", source)

    def test_config_ui_exposes_and_persists_clock_format(self):
        env_source = read_source(CONFIG_ENVIRONMENT_C)
        data_source = read_source(CONFIG_ENVIRONMENT_DATA_C)
        date_layout = read_layout_source("_environment_date_gadgets")
        misc_layout = read_layout_source("_environment_misc_gadgets")
        date_load = read_case_source("_config_env_set", "ENVIRONMENT_DATE")
        date_store = read_case_source("_config_env_store", "ENVIRONMENT_DATE")
        misc_load = read_case_source("_config_env_set", "ENVIRONMENT_MISC")
        misc_store = read_case_source("_config_env_store", "ENVIRONMENT_MISC")

        self.assertIn("GAD_ENVIRONMENT_CLOCK_FORMAT", data_source)
        self.assertIn("_environment_clock_format_taglist", data_source)
        self.assertIn("GAD_ENVIRONMENT_CLOCK_FORMAT_LIST", data_source)
        self.assertIn("GAD_ENVIRONMENT_CLOCK_FORMAT", date_layout)
        self.assertIn("GAD_ENVIRONMENT_CLOCK_FORMAT_LIST", date_layout)
        self.assertIn("MSG_ENVIRONMENT_CLOCK_FORMAT", date_layout)
        self.assertIn("{1, 0, 0, 0}", date_layout)
        self.assertIn("{5, 2, 0, 0}", date_layout)
        self.assertIn("{4, 1, 1, 1}", date_layout)
        self.assertIn("{5, 6, 7, 1}", date_layout)
        self.assertIn("{4, 9, 0, 1}", date_layout)
        self.assertIn("{1, 11, 0, 0}", date_layout)
        self.assertIn("{5, 29, 0, 0}", date_layout)
        self.assertIn("{5, 35, 28, 6}", date_layout)
        self.assertIn("{4, 12, SIZE_MAX_LESS - 2, 1}", date_layout)
        self.assertIn("{33, 35, -8, 6}", date_layout)
        self.assertNotIn("GAD_ENVIRONMENT_CLOCK_FORMAT", misc_layout)
        self.assertIn("SetGadgetValue(data->option_list, GAD_ENVIRONMENT_CLOCK_FORMAT", date_load)
        self.assertIn("stccpy(data->config->clock_format", date_store)
        self.assertNotIn("GAD_ENVIRONMENT_CLOCK_FORMAT", misc_load)
        self.assertNotIn("clock_format", misc_store)
        self.assertIn("stricmp(data->config->clock_format, env->env->clock_format)", env_source)

    def test_locale_strings_define_clock_format_label(self):
        cd_source = read_source(CONFIGOPUS_CD)
        string_source = read_source(STRING_DATA_H)

        self.assertIn("MSG_ENVIRONMENT_CLOCK_FORMAT (23038//)", cd_source)
        self.assertIn("_Clock Format...", cd_source)
        self.assertIn("#define MSG_ENVIRONMENT_CLOCK_FORMAT 23038", string_source)
        self.assertIn('#define MSG_ENVIRONMENT_CLOCK_FORMAT_STR "_Clock Format..."', string_source)
        self.assertIn("MSG_ENVIRONMENT_CLOCK_FORMAT, (CONST_STRPTR)MSG_ENVIRONMENT_CLOCK_FORMAT_STR", string_source)

    def test_clock_format_popup_uses_formatdate_codes_with_raw_insertion(self):
        env_source = read_source(CONFIG_ENVIRONMENT_C)
        lister_source = read_source(CONFIG_ENVIRONMENT_LISTER_C)
        cd_source = read_source(CONFIGOPUS_CD)
        string_source = read_source(STRING_DATA_H)

        self.assertIn("case GAD_ENVIRONMENT_CLOCK_FORMAT_LIST:", env_source)
        self.assertIn("MSG_CLOCKFORMAT_CODE_FIRST", env_source)
        self.assertIn("MSG_CLOCKFORMAT_CODE_LAST", env_source)
        self.assertIn("void _config_env_clock_format_list", lister_source)
        self.assertIn("config_env_insert_raw_string(objlist, id, buf);", lister_source)
        self.assertIn("funced_edit_insertstring(objlist, id, buf", lister_source)
        self.assertIn("maxlen = info->MaxChars - 1;", lister_source)
        self.assertIn("if (insert_len > maxlen - len)", lister_source)
        self.assertIn("info->BufferPos = len;", lister_source)
        self.assertIn("MSG_CLOCKFORMAT_CODE_FIRST (23039//)", cd_source)
        self.assertIn("*H\\t24-hour with leading 0", cd_source)
        self.assertIn("*M\\tMinute with leading 0", cd_source)
        self.assertIn("*p\\tAM/PM string", cd_source)
        self.assertIn("MSG_CLOCKFORMAT_CODE_LAST_STR", string_source)
        self.assertIn("\\x00\\x00\\x5A\\x1C\\x00\\x02", string_source)


if __name__ == "__main__":
    unittest.main()
