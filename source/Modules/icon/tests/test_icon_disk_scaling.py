#!/usr/bin/env python3
"""Regression checks for auto-scaling disk values in the Icon Info (device) requester."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[4]
ICON_C = ROOT / "source" / "Modules" / "icon" / "icon.c"


def read_source(path):
    return path.read_text(encoding="latin-1")


def disk_info_block(source):
    """The Size/Used/Free region only, so negative checks aren't fooled by
    unrelated code elsewhere in the file."""
    start = source.index("// Disk size")
    end = source.index("// Disk type", start)
    return source[start:end]


class IconDiskScalingTests(unittest.TestCase):
    def test_disk_fields_use_bytestostring64_in_64bit_branch(self):
        source = read_source(ICON_C)

        self.assertEqual(
            source.count("BytesToString64(&tmp, buf, sizeof(buf), 1, data->decimal_sep);"),
            3,
        )

    def test_disk_fields_use_bytestostring_in_32bit_branch(self):
        source = read_source(ICON_C)

        self.assertIn(
            "BytesToString(data->info.id_NumBlocks * data->info.id_BytesPerBlock, buf, 1, data->decimal_sep);",
            source,
        )
        self.assertIn(
            "BytesToString(data->info.id_NumBlocksUsed * data->info.id_BytesPerBlock, buf, 1, data->decimal_sep);",
            source,
        )
        self.assertIn(
            "BytesToString((data->info.id_NumBlocks - data->info.id_NumBlocksUsed) * data->info.id_BytesPerBlock,",
            source,
        )

    def test_old_kilobyte_formatting_removed_from_disk_fields(self):
        block = disk_info_block(read_source(ICON_C))

        self.assertNotIn("tmp >>= 10;", block)
        self.assertNotIn('strcat(buf, "K");', block)
        self.assertNotIn("(data->info.id_NumBlocks * data->info.id_BytesPerBlock) >> 10", block)
        self.assertNotIn("(data->info.id_NumBlocksUsed * data->info.id_BytesPerBlock) >> 10", block)


if __name__ == "__main__":
    unittest.main()
