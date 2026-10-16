#!/usr/bin/env python3
"""Regression checks for defaults shipped in basedata.lha and the installer."""

from pathlib import Path
import re
import shutil
import subprocess
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import filetype_tool  # noqa: E402


ROOT = Path(__file__).resolve().parents[2]
BASEDATA = ROOT / "archive" / "basedata.lha"
INSTALLER = ROOT / "archive" / "installer" / "Install DOpus5"

REQUIRED_PLACEHOLDERS = (
    "DOpus5/Commands/.dummy",
    "DOpus5/Desktop/.dummy",
    "DOpus5/Groups/.dummy",
    "DOpus5/Themes/.dummy",
    "DOpus5/Sounds/.dummy",
)


def archive_tool():
    tool = shutil.which("lha")
    if tool:
        return "lha", tool

    tool = shutil.which("7z")
    if tool:
        return "7z", tool

    raise unittest.SkipTest("lha or 7z is required to inspect basedata.lha")


def archive_listing():
    _name, tool = archive_tool()

    return subprocess.run(
        [tool, "l", str(BASEDATA)],
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    ).stdout


def archive_member(member):
    name, tool = archive_tool()
    if name == "lha":
        command = [tool, "pq", str(BASEDATA), member]
    else:
        command = [tool, "x", "-so", str(BASEDATA), member]

    return subprocess.run(
        command,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    ).stdout


def filetype_members(drawer):
    members = []
    for line in archive_listing().splitlines():
        index = line.find(drawer)
        if index >= 0:
            member = line[index:].rstrip()
            if not member.endswith("/"):
                members.append(member)

    return members


def filetype_name(data):
    types = filetype_tool.parse(data)
    if len(types) != 1:
        raise AssertionError("expected one filetype, found %d" % len(types))

    return types[0].name


class BasedataDefaultDrawerTests(unittest.TestCase):
    def test_default_empty_drawers_have_placeholders(self):
        listing = archive_listing()

        for path in REQUIRED_PLACEHOLDERS:
            with self.subTest(path=path):
                self.assertIn(path, listing)


class BasedataDefaultScriptTests(unittest.TestCase):
    def test_disk_inserted_leftover_sound_command_is_removed(self):
        scripts = archive_member("DOpus5/Buttons/Scripts")

        self.assertNotIn(b"REM Play QUIET work2:32dohs!.SND", scripts)
        self.assertNotIn(b"32dohs", scripts)


class BasedataDefaultEnvironmentTests(unittest.TestCase):
    def test_workbench_environment_matches_default_environment(self):
        default = archive_member("DOpus5/Environment/default")
        workbench = archive_member("DOpus5/Environment/workbench")

        self.assertEqual(default, workbench)


class BasedataFiletypeTests(unittest.TestCase):
    def test_tool_round_trips_every_shipped_filetype(self):
        # archive/tools/filetype_tool.py is used to review and edit filetype
        # files; it must reproduce every shipped file byte for byte.
        members = filetype_members("DOpus5/Filetypes/") + filetype_members("DOpus5/Storage/Filetypes/")
        self.assertTrue(members)

        for member in members:
            with self.subTest(member=member):
                data = archive_member(member)
                self.assertEqual(data, filetype_tool.encode(filetype_tool.parse(data)))

    def test_stored_filetypes_are_named_after_the_filetype(self):
        # The filetype finder installs a stored filetype under its name, the
        # editor installs it under its file name; they must agree.
        members = filetype_members("DOpus5/Storage/Filetypes/")
        self.assertTrue(members)

        for member in members:
            with self.subTest(member=member):
                self.assertEqual(member.rsplit("/", 1)[1], filetype_name(archive_member(member)))

    def test_installed_filetypes_are_not_also_stored(self):
        installed = {filetype_name(archive_member(m)).lower() for m in filetype_members("DOpus5/Filetypes/")}
        stored = {filetype_name(archive_member(m)).lower() for m in filetype_members("DOpus5/Storage/Filetypes/")}

        self.assertLessEqual({"ascii", "amigaguide"}, installed)
        self.assertEqual(set(), installed & stored)


class InstallerTests(unittest.TestCase):
    def setUp(self):
        self.script = INSTALLER.read_text(encoding="latin-1")

    def test_string_literals_fit_the_installer_limit(self):
        # The OS 3.2 Installer refuses to compile a script with a 539-byte
        # string ("Symbol or String too long"); split long texts instead.
        for match in re.finditer(r'"((?:[^"\\]|\\.)*)"', self.script):
            with self.subTest(line=self.script.count("\n", 0, match.start()) + 1):
                self.assertLessEqual(len(match.group(1)), 400)

    def test_upgrade_handles_every_drawer_and_file_in_basedata(self):
        # An upgrade copies drawer by drawer; a new top-level entry in
        # basedata that the script does not name would never reach users.
        handled = set(re.findall(r'\(set #pdir "([^"/]+)"\)', self.script))
        handled |= set(re.findall(r'\(set #pfile "([^"]+)"\)', self.script))

        top_level = set()
        for line in archive_listing().splitlines():
            index = line.find("DOpus5/")
            if index >= 0:
                parts = line[index:].rstrip().split("/")
                if len(parts) > 1 and parts[1]:
                    top_level.add(parts[1])

        self.assertTrue(top_level)
        self.assertEqual(set(), top_level - handled)

    def test_old_stored_filetype_replacements_exist(self):
        # The cleanup only removes an old file when its replacement is
        # present, so a stale replacement name silently disables it.
        stored = {m.rsplit("/", 1)[1] for m in filetype_members("DOpus5/Storage/Filetypes/")}
        installed = {m.rsplit("/", 1)[1] for m in filetype_members("DOpus5/Filetypes/")}

        replacements = re.findall(r'#new \(tackon #olds "([^"]+)"\)', self.script)
        self.assertTrue(replacements)
        self.assertEqual(set(), set(replacements) - stored)

        installed_replacements = re.findall(r'#new \(tackon #dest "Filetypes/([^"]+)"\)', self.script)
        self.assertTrue(installed_replacements)
        self.assertEqual(set(), set(installed_replacements) - installed)


if __name__ == "__main__":
    unittest.main()
