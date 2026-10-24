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
import magicwb_iconset  # noqa: E402


ROOT = Path(__file__).resolve().parents[2]
BASEDATA = ROOT / "archive" / "basedata.lha"
INSTALLER = ROOT / "archive" / "installer" / "Install DOpus5"
MAGICWB = "IconSets/MagicWB/"

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


def drawer_files(drawer):
    # Names follow a space in the listing; the anchor keeps "DOpus5/" from
    # matching inside "IconSets/MagicWB/DOpus5/".
    members = []
    for line in archive_listing().splitlines():
        index = line.find(" " + drawer)
        if index >= 0:
            member = line[index + 1:].rstrip()
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
        members = drawer_files("DOpus5/Filetypes/") + drawer_files("DOpus5/Storage/Filetypes/")
        self.assertTrue(members)

        for member in members:
            with self.subTest(member=member):
                data = archive_member(member)
                self.assertEqual(data, filetype_tool.encode(filetype_tool.parse(data)))

    def test_stored_filetypes_are_named_after_the_filetype(self):
        # The filetype finder installs a stored filetype under its name, the
        # editor installs it under its file name; they must agree.
        members = drawer_files("DOpus5/Storage/Filetypes/")
        self.assertTrue(members)

        for member in members:
            with self.subTest(member=member):
                self.assertEqual(member.rsplit("/", 1)[1], filetype_name(archive_member(member)))

    def test_installed_filetypes_are_not_also_stored(self):
        installed = {filetype_name(archive_member(m)).lower() for m in drawer_files("DOpus5/Filetypes/")}
        stored = {filetype_name(archive_member(m)).lower() for m in drawer_files("DOpus5/Storage/Filetypes/")}

        self.assertLessEqual({"ascii", "amigaguide"}, installed)
        self.assertEqual(set(), installed & stored)

    def test_filetype_icons_are_shipped(self):
        # Icon lookups use the first matching filetype with an icon path; a
        # path to a file that is not there (DOpus5:Icons/def/..., another
        # system's ENVARC:Sys deficons) replaces the icon icon.library would
        # have chosen with a failed load.
        shipped = {m.lower() for m in drawer_files("DOpus5/")}
        members = drawer_files("DOpus5/Filetypes/") + drawer_files("DOpus5/Storage/Filetypes/")

        for member in members:
            icon = filetype_tool.parse(archive_member(member))[0].icon.decode("latin-1")
            if icon:
                with self.subTest(member=member, icon=icon):
                    self.assertTrue(icon.lower().startswith("dopus5:"))
                    self.assertIn("dopus5/" + icon[len("dopus5:"):].lower(), shipped)


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
            index = line.find(" DOpus5/")
            if index >= 0:
                parts = line[index + 1:].rstrip().split("/")
                if len(parts) > 1 and parts[1]:
                    top_level.add(parts[1])

        self.assertTrue(top_level)
        self.assertEqual(set(), top_level - handled)

    def test_old_stored_filetype_replacements_exist(self):
        # The cleanup only removes an old file when its replacement is
        # present, so a stale replacement name silently disables it.
        stored = {m.rsplit("/", 1)[1] for m in drawer_files("DOpus5/Storage/Filetypes/")}
        installed = {m.rsplit("/", 1)[1] for m in drawer_files("DOpus5/Filetypes/")}

        replacements = re.findall(r'#new \(tackon #olds "([^"]+)"\)', self.script)
        self.assertTrue(replacements)
        self.assertEqual(set(), set(replacements) - stored)

        installed_replacements = re.findall(r'#new \(tackon #dest "Filetypes/([^"]+)"\)', self.script)
        self.assertTrue(installed_replacements)
        self.assertEqual(set(), set(installed_replacements) - installed)

    def test_upgrade_handles_every_magicwb_file(self):
        # The MagicWB set is copied drawer by drawer on an upgrade; anything
        # the script does not name only reaches new installations.  The
        # environments are configuration, which an upgrade keeps.
        replaced = set(re.findall(r'\(set #pdir "([^"]+)"\)\s+\(P_SET_REPLACE\)', self.script))
        banks = set(re.findall(r'\(set #bank "([^"]+)"\)', self.script))
        files = set(re.findall(r'\(set #pfile "([^"]+)"\)', self.script))

        self.assertIn('(set #mwb "%s")' % MAGICWB.rstrip("/"), self.script)
        for member in drawer_files(MAGICWB + "DOpus5/"):
            path = member[len(MAGICWB + "DOpus5/"):]
            with self.subTest(member=member):
                top = path.split("/")[0]
                if top == "Buttons":
                    self.assertIn(path.split("/")[1], banks)
                elif top == "Environment":
                    continue
                elif "/" in path:
                    self.assertIn(top, replaced)
                else:
                    self.assertIn(path, files)

    def test_replaced_banks_exist_in_both_sets(self):
        # P_BANK skips a bank it cannot find, so a misspelt name would leave
        # the old images' bank in place without a word.
        banks = re.findall(r'\(set #bank "([^"]+)"\)', self.script)
        self.assertTrue(banks)
        standard = set(drawer_files("DOpus5/Buttons/"))
        magicwb = set(drawer_files(MAGICWB + "DOpus5/Buttons/"))
        for bank in banks:
            with self.subTest(bank=bank):
                self.assertIn("DOpus5/Buttons/" + bank, standard)
                self.assertIn(MAGICWB + "DOpus5/Buttons/" + bank, magicwb)

    def test_translation_choices_are_the_shipped_languages(self):
        # The release archive has Catalogs/<language> for the "## language"
        # of every translation; the installer lists the languages twice
        # (choices and P_CATALOG_LANGUAGE), and bit n of the answer must
        # name the same language in both, so a language missing from
        # either list, or listed in another order, is never installed.
        shipped = set()
        for ct in (ROOT / "catalogs").glob("*/*.ct"):
            match = re.search(rb"^## language[ \t]*(\S+)", ct.read_bytes(), re.M)
            self.assertIsNotNone(match, ct)
            shipped.add(match.group(1).decode("latin-1"))

        select = re.search(r'\(select #cat-n((?:\s+"[^"]*")+)\)', self.script)
        choices = re.search(r'\(choices((?:\s+"[^"]*")+)\)\s+\(default #cat-default\)', self.script)
        count = re.search(r'\(set #cat-count (\d+)\)', self.script)
        self.assertIsNotNone(select)
        self.assertIsNotNone(choices)
        self.assertIsNotNone(count)

        languages = re.findall(r'"([^"]*)"', select.group(1))
        self.assertEqual(languages, re.findall(r'"([^"]*)"', choices.group(1)))
        self.assertEqual(len(languages), int(count.group(1)))
        self.assertEqual(shipped, set(languages))


def image_references(bank):
    """Images a button bank loads, relative to DOpus5: and in lower case."""
    return {m.decode("latin-1").lower() for m in re.findall(rb"(?i)dopus5:(images/[^\x00]+)", bank)}


class MagicWBIconSetTests(unittest.TestCase):
    def test_default_banks_only_use_magicwb_images(self):
        # After a MagicWB installation the default banks are the MagicWB
        # ones where the set has them, the standard ones elsewhere; none
        # may load a standard (256 colour) image.
        images = {m[len(MAGICWB + "DOpus5/"):].lower() for m in drawer_files(MAGICWB + "DOpus5/Images/")}
        overridden = {m.rsplit("/", 1)[1] for m in drawer_files(MAGICWB + "DOpus5/Buttons/")}

        checked = 0
        for member in drawer_files("DOpus5/Buttons/"):
            name = member.rsplit("/", 1)[1]
            if name in overridden:
                member = MAGICWB + member
            refs = image_references(archive_member(member))
            checked += len(refs)
            with self.subTest(bank=member):
                self.assertEqual(set(), refs - images)
        self.assertTrue(checked)

    def test_magicwb_banks_are_the_standard_banks_without_borders(self):
        # The MagicWB images draw their own border.  Apart from that the
        # banks must follow the standard ones, or an edit of the default
        # toolbar would not reach MagicWB users.
        members = drawer_files(MAGICWB + "DOpus5/Buttons/")
        self.assertTrue(members)

        for member in members:
            with self.subTest(bank=member):
                standard = archive_member(member[len(MAGICWB):])
                self.assertEqual(magicwb_iconset.borderless(standard), archive_member(member))

    def test_magicwb_environments_are_the_standard_ones_without_icon_remapping(self):
        # Remapping draws colours 4-7 of the MagicWB icons in the last four
        # pens; on the Workbench screen the MagicWB colours are in pens 4-7.
        members = drawer_files(MAGICWB + "DOpus5/Environment/")
        self.assertEqual({"default", "workbench"}, {m.rsplit("/", 1)[1] for m in members})

        for member in members:
            with self.subTest(environment=member):
                standard = archive_member(member[len(MAGICWB):])
                self.assertEqual(magicwb_iconset.no_remap(standard), archive_member(member))

    def test_magicwb_icons_keep_the_standard_icon_settings(self):
        # Only the images change: tool types, default tool, type and stack
        # are those of the standard icon.  The icons hold nothing but the
        # 8 colour planar images, so no colour icon data is drawn instead.
        members = [m for m in drawer_files(MAGICWB) if m.endswith(".info") and "/Images/" not in m]
        self.assertTrue(members)

        for member in members:
            with self.subTest(icon=member):
                data = archive_member(member)
                icon = magicwb_iconset.Icon(data)
                standard = magicwb_iconset.Icon(archive_member(member[len(MAGICWB):]))
                self.assertEqual((standard.type, standard.default_tool, standard.tool_types, standard.stack),
                                 (icon.type, icon.default_tool, icon.tool_types, icon.stack))
                self.assertEqual(data, icon.encode())
                self.assertTrue(all(v < 8 for image in icon.images for row in image for v in row))


if __name__ == "__main__":
    unittest.main()
