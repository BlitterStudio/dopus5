#!/usr/bin/env python3
"""Regression checks for xadopus callback structures on pointer-sized ABIs."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[4]
XADOPUS_H = ROOT / "source" / "Modules" / "xadopus" / "XADopus.h"
XADOPUS_C = ROOT / "source" / "Modules" / "xadopus" / "XADopus.c"


def read_source(path):
    return path.read_text(encoding="latin-1")


def struct_body(source, name):
    pattern = r"struct\s+" + re.escape(name) + r"\s*\{(?P<body>.*?)\n\};"
    match = re.search(pattern, source, re.S)
    if not match:
        raise AssertionError(f"Could not find struct {name}")
    return match.group("body")


class XADopusAROS64LayoutTests(unittest.TestCase):
    def test_callback_nodes_use_real_min_nodes(self):
        source = read_source(XADOPUS_H)

        for name in ("function_entry", "path_node"):
            body = struct_body(source, name)
            self.assertIn("struct MinNode node;", body)
            self.assertNotIn("ULONG pad[2]", body)

    def test_lister_handle_is_not_truncated_to_ulong(self):
        header = read_source(XADOPUS_H)
        source = read_source(XADOPUS_C)
        data_body = struct_body(header, "xoData")

        self.assertRegex(data_body, r"\bIPTR\s+listh\s*;")
        self.assertIn("char lists[24]", data_body)
        self.assertIn("data.listh = (IPTR)data.listp2->lister;", source)
        self.assertIn("xad_format_handle(data.lists, data.listh);", source)
        self.assertIn("data.listh = xad_parse_handle(result);", source)
        self.assertNotIn("data.listh = (ULONG)data.listp2->lister;", source)
        self.assertNotIn('sprintf(data.lists, "%lu"', source)
        self.assertNotIn('"command source %lu', source)
        self.assertNotIn("atol(result)", source)
        self.assertNotIn("(ULONG)data.destp->lister", source)
        self.assertNotIn("ti_Data = (ULONG)arcname", source)

    def test_lister_handle_text_helpers_use_full_pointer_width(self):
        source = read_source(XADOPUS_C)

        self.assertRegex(source, r"static\s+IPTR\s+xad_parse_handle\(const char \*text\)")
        self.assertRegex(source, r"static\s+void\s+xad_format_handle\(char \*buf, IPTR handle\)")
        self.assertIn("UQUAD value = 0;", source)
        self.assertIn("UQUAD value = (UQUAD)handle;", source)

    def test_new_lister_result_is_checked_before_use(self):
        source = read_source(XADOPUS_C)
        result_pos = source.index('"lister new"')
        parse_pos = source.index("data.listh = xad_parse_handle(result);")
        block = source[result_pos : source.index("if (AllocPort(&data))", parse_pos)]

        self.assertIn("if (!result)", block)
        self.assertIn("if (!data.listh)", block)
        self.assertIn("FreeMemHandle(data.rhand);", block)

    def test_module_registration_fits_function_array(self):
        deps = read_source(ROOT / "source" / "Modules" / "xadopus" / "module_deps.h")
        data_c = read_source(ROOT / "source" / "Modules" / "xadopus" / "XADopus_data.c")

        match = re.search(r"ModuleFunction function\[(\d+)\];", deps)
        self.assertIsNotNone(match, "module_deps.h must size the ModuleInfo_2 function array")
        capacity = int(match.group(1))

        count = re.search(r"^\t(\d+),\t+// Number of functions", data_c, re.M)
        self.assertIsNotNone(count, "XADopus_data.c must declare the function count")
        entries = len(re.findall(r'\{\d+,\s*"', data_c))
        self.assertEqual(int(count.group(1)), entries)
        self.assertLessEqual(entries, capacity)


if __name__ == "__main__":
    unittest.main()
