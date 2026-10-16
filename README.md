# Directory Opus 5

[![Build](https://github.com/BlitterStudio/dopus5/actions/workflows/makefile.yml/badge.svg?branch=master)](https://github.com/BlitterStudio/dopus5/actions/workflows/makefile.yml)
[![Latest release](https://img.shields.io/github/v/release/BlitterStudio/dopus5?display_name=tag&sort=semver)](https://github.com/BlitterStudio/dopus5/releases/latest)
[![License: APL 1.1](https://img.shields.io/badge/license-APL%201.1-blue.svg)](COPYING)
[![Issues](https://img.shields.io/github/issues/BlitterStudio/dopus5)](https://github.com/BlitterStudio/dopus5/issues)
[![Last commit](https://img.shields.io/github/last-commit/BlitterStudio/dopus5/master)](https://github.com/BlitterStudio/dopus5/commits/master)

A maintained continuation of **Directory Opus 5 Magellan II** for AmigaOS 3,
AmigaOS 4, MorphOS, and AROS, building on the 2012 APL source release with
GCC / cross-toolchain support, modern platform fixes, and ongoing module
work.

> Looking for the original 2012 APL release notes? See [`README.legacy`](README.legacy).
> The project was historically known as **dopus5allamigas**; the
> repository and project name are now just **dopus5**.

## Table of contents

- [Supported platforms](#supported-platforms)
- [Changes since 5.102](#changes-since-5102)
- [Downloads](#downloads)
  - [Installing](#installing)
- [Building](#building)
  - [Per-platform make targets](#per-platform-make-targets)
  - [Release archives](#release-archives)
  - [SFTP support](#sftp-support)
  - [xadopus.module](#xadopusmodule)
- [Testing](#testing)
  - [Filetype files](#filetype-files)
  - [MagicWB icon set](#magicwb-icon-set)
- [Project status](#project-status)
- [License and trademarks](#license-and-trademarks)
- [Credits](#credits)
- [Links](#links)

## Supported platforms

| Platform   | Architectures                              | Notes                                              |
| ---------- | ------------------------------------------ | -------------------------------------------------- |
| AmigaOS 3  | m68k 68020-60 (default) and plain m68k 68000 | Tested on OS 3.1, 3.1.4, 3.2 and 3.9               |
| AmigaOS 4  | PPC                                        |                                                    |
| MorphOS    | PPC                                        |                                                    |
| AROS       | i386 (ABIv0), x86_64 / aarch64 (ABIv11)  | All three architectures have CI debug + release parity |

The 68000 OS3 variant ships in its own archive (`DOpus5_<rev>_os3-68000.lha`)
so users on stock 68000-based hardware (A500, A600, A1000, A2000, CDTV) can
run DOpus5 without needing the 020+ build. A1200 / A3000 / A4000 owners
should use the regular `os3` archive, which targets `-m68020-60 -mtune=68030`.

## Changes since 5.102

These are on `master` and go into the next release. The
[`ChangeLog`](ChangeLog) has the details.

- **Installer:** every release archive has an `Install DOpus5` script for
  new installations and upgrades, with Workbench replacement on AmigaOS 3.
  See [Installing](#installing).
- **MagicWB images and icons** for 8 and 16 colour screens, chosen in the
  installer. When Opus replaces Workbench it locks the MagicWB pens as
  Workbench does, and with "Set MagicWB Colors" on it no longer remaps
  icons on the Workbench screen.
- **Filetypes:** ASCII and AmigaGuide are installed by default, the stored
  filetypes in `DOpus5:Storage/Filetypes` are cleaned up and renamed, and
  the File Type Editor keeps assigns in icon paths.
- **Eject Disk** in the Icons menu and the pop-up menu of disk icons.
- **Environment editor:** Palette has Presets (Workbench Screen,
  Workbench 3.1, MagicWB) and redraws its colour squares on RTG screens.
  The custom colours in Lister Colours now apply to every lister element.
- **Menus:** the Buttons menu is now Settings > Buttons.
- **XADOpus:** the new `XADOpenInLister` command shows an archive in the
  lister it was opened from. Dragging an archive onto a lister extracts it
  and refreshes that lister instead of opening a second one.
- **AmigaOS 3.2:** scrollers inside windows use the system knob imagery,
  and icon labels take the Text+Outline and Text+Shadow modes from Font
  prefs (also on AmigaOS 4). Outlined labels no longer have gaps.
- **Startup:** module commands are cached in `DOpus5:System/module-cache`,
  filetypes and the desktop backdrop load in their own processes, and
  toolbar images remap with far fewer pen lookups.
- **RTG screens:** icon masks and remapped image planes stay in fast
  memory, and colours are right after Opus moves from an RTG screen to a
  native one.
- **Fixes:** FTP and SFTP connections no longer overflow the lister stack,
  the lister toolbar is no taller than its buttons, and the title bar
  padlock uses the shine pen.
- **Builds:** new AROS aarch64 target.

## Downloads

Pre-built binaries for every supported platform are published under
[Releases](https://github.com/BlitterStudio/dopus5/releases). Each tagged
release ships one `DOpus5_<rev>_<platform>.lha` per target.

### Installing

Unpack the archive anywhere, for example to `RAM:`, and double-click
`Install DOpus5`. The script needs the Installer program that comes with
each supported system.

- **New installation:** choose the drawer to install into; the script
  creates a `DOpus5` drawer there.
- **Upgrade:** when the `DOpus5:` assign points to an existing
  installation, the script offers to upgrade it. Quit Directory Opus
  first. The program, libraries, modules, commands, manuals, ARexx
  scripts, images and stored items are replaced; configuration drawers
  (`Buttons`, `Environment`, `Settings`, `Filetypes`, `Commands`,
  `Desktop`, `Groups`, `Themes`, `Sounds`, `System`) only receive files
  they do not have yet.
- **Images and icons:** the standard set for RTG and other screens with
  many colours, or the 8 colour MagicWB set (MWB_DOpus 8 by Trevor
  Morris and Leo Davidson) for 8 and 16 colour screens, with borderless
  lister toolbar and FTP bank buttons and "Remap icon images" off in new
  installations. Suggested for AmigaOS 3 without RTG; remembered in
  `ENVARC:DOpus5Icons`. An upgrade that changes the set offers to replace
  `Buttons/Toolbar` and `Buttons/FTP_image` (keeping the old ones as
  `.old`). The MagicWB files are also in `IconSets/MagicWB` of the
  archive, laid out like the archive root.
- **Start mode:** manually, from an `Opus5_Startup` icon in
  `SYS:WBStartup` when Workbench starts, or (AmigaOS 3) instead of
  Workbench. Workbench replacement makes `DOpus5:C/LoadDB` resident as
  `LoadWB` in `S:User-Startup`, so the Startup-Sequence is not changed;
  hold Shift during boot to get Workbench. The installer remembers the
  choice in `ENVARC:DOpus5Startup`.
- **Optional:** a `;BEGIN DOpus5` block in `S:User-Startup` with the
  `DOpus5:` assign and `Path DOpus5:C ADD` (always written for
  Workbench replacement). Declining it later empties the block.

Directory Opus assigns `DOpus5:` to its own drawer when it starts, so
copying the `DOpus5` drawer by hand also works for a new installation.

## Building

The project builds with GCC-based cross-compilation toolchains. The
recommended way is via the prebuilt Docker images used by CI:

| Target              | Docker image                                 |
| ------------------- | -------------------------------------------- |
| AmigaOS 3 (m68k)    | `sacredbanana/amiga-compiler:m68k-amigaos`   |
| AmigaOS 4 (PPC)     | `amigadev/crosstools:ppc-amigaos`             |
| MorphOS (PPC)       | `amigadev/crosstools:ppc-morphos`             |
| AROS i386 (ABIv0)   | `midwan/aros-compiler:i386-aros`             |
| AROS x86_64 (ABIv11)| `midwan/aros-compiler:x86_64-aros`           |
| AROS aarch64 (ABIv11)| `midwan/aros-compiler:aarch64-aros`          |

CI pins the tested Amiga images by digest in
[the build workflow](.github/workflows/makefile.yml). Use those digest references
instead of the mutable tags above to reproduce a CI build.

The OS3 build stays on the GCC 6.5.0b toolchain: the GCC 16.2.0b
`amigadev/crosstools:m68k-amigaos-gcc10` image ships a binutils build whose
linker emits broken library-call stub relocations (the base reference lands in
the text hunk instead of the data hunk, crashing at runtime on the first such
call). That toolchain is preserved on the `toolchain/gcc16-os3` branch until
binutils is fixed upstream.

The OS4 and MorphOS images include the SDKs required by the FTP module
(AmiSSL and codesets on OS4, codesets on MorphOS).

Native builds on Amiga / MorphOS / AROS are also possible if you have a
working GCC + libc + AmigaOS SDK installed.

### Per-platform make targets

From inside the toolchain environment:

```sh
cd source

make os3            # AmigaOS 3 (default: -m68020-60 -mtune=68030)
make os3-68000      # AmigaOS 3, plain 68000 (-m68000)
make os4            # AmigaOS 4 (PPC)
make mos            # MorphOS (PPC)
make i386-aros      # AROS i386 (ABIv0)
make x86_64-aros    # AROS x86_64 (ABIv11)
make aarch64-aros   # AROS aarch64 (ABIv11, Raspberry Pi 3/4/5 64-bit)
```

By default the build is a debug build. Add `debug=no` for the optimised
release build:

```sh
make os3 debug=no
```

### Release archives

Building the `.lha` archive uses the same makefile:

```sh
cd source
make os3 release debug=no
```

The release archive ends up under `releases/` as
`DOpus5_<rev>_<platform>.lha` (or `..._dev_<platform>.lha` for the debug
variant). It holds the `DOpus5` drawer, the `Install DOpus5` script and
the `IconSets/MagicWB` drawer. `make <platform> release-package` packs an
existing build without compiling again.

### SFTP support

The FTP module supports SFTP via [libssh2](https://libssh2.org/).

- On **AmigaOS 3 / AmigaOS 4 / MorphOS** SFTP is **always built in**:
  the cross-build helper at `source/Modules/ftp/third_party/libssh2/`
  fetches and statically links libssh2 1.11.1 by default.
- On **AROS i386 / x86_64** SFTP is opt-in (those toolchain images already
  provide a shared `libssh2`):

  ```sh
  cd source
  make i386-aros sftp=yes
  make x86_64-aros sftp=yes
  ```

  The current `aarch64-aros` SDK does not ship libssh2, so `sftp=yes` is not
  yet available for that target.

### xadopus.module

`xadopus.module` (archive-as-folder support via xadmaster.library) is
built by default for every target. The CI Docker images ship the
required `proto/xadmaster.h` headers.

If the headers aren't installed in a local/native build, it fails with
`fatal error: proto/xadmaster.h: No such file or directory`, which
is the expected outcome; install xadmaster's developer headers.

Consult `source/makefile`, `source/makefile.common` and the per-platform /
per-module makefiles for the full list of targets and options.

## Testing

The FTP parser / protocol / TLS tests run on a normal Unix-like host:

```sh
make -C source/Modules/ftp -f makefile.tests test
```

If `libssh2` development headers are installed, the SFTP backend tests can
also be run:

```sh
make -C source/Modules/ftp -f makefile.tests test-libssh2
```

CI runs both of these before kicking off the platform release builds.

The checks for the base archive and the installer script need `lha` (or
`7z`) on the path:

```sh
python3 archive/tests/test_basedata_default_drawers.py
```

### Filetype files

`archive/tools/filetype_tool.py` reads the filetype files Directory Opus
keeps in `DOpus5:Filetypes` and `DOpus5:Storage/Filetypes`:

```sh
python3 archive/tools/filetype_tool.py dump  FILE...  # recognition, icon and actions
python3 archive/tools/filetype_tool.py lint  FILE...  # problems to fix before shipping
python3 archive/tools/filetype_tool.py check FILE...  # decode/re-encode round trip
```

`lint` reports a file name that differs from the filetype name, damaged or
partly ignored data, icon paths and commands that need assigns a stock
system lacks, internal commands that neither Opus nor its modules provide
(checked against `source/`, so run it from a checkout), recognition by file
name alone, upper-case datatype IDs and drag'n'drop actions that replace the
default `Copy MOVEWHENSAME` with a plain `Copy`. Use it to review
contributed filetypes before adding them to `basedata.lha`.

### MagicWB icon set

`archive/tools/magicwb_iconset.py` builds `IconSets/MagicWB` of
`basedata.lha` from
[MWB_DOpus8c.lha](https://aminet.net/package/pix/mwb/MWB_DOpus8c) and the
standard files in `basedata.lha`. Run it after changing a default button
bank or an icon the set replaces; it needs LHa for UNIX, so run it in one of
the build images:

```sh
docker run --rm -v "$PWD:/work" -v "$HOME/Downloads:/mwb" \
  sacredbanana/amiga-compiler:m68k-amigaos \
  sh -c 'cd /work && python3 archive/tools/magicwb_iconset.py /mwb/MWB_DOpus8c.lha /tmp/out --update'
```

The basedata tests fail when the set's banks or environments no longer match
the standard ones, or a default bank uses an image the set lacks.

## Project status

Active development happens here. See:

- [`ChangeLog`](ChangeLog) — full change history (most recent at the top)
- [Releases](https://github.com/BlitterStudio/dopus5/releases) — packaged builds
- [Issues](https://github.com/BlitterStudio/dopus5/issues) — open work and bug reports

Contributions are welcome via pull requests. Please open an issue first if
you're planning a larger change so we can sync on direction.

## License and trademarks

Directory Opus 5 is released under the **AROS Public License (APL) version
1.1** — see [`COPYING`](COPYING).

Optional builds may include or link third-party components. In particular,
the FTP module's SFTP support uses libssh2 and zlib; see
[`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) for the relevant license
texts.

"Directory Opus" is a registered trademark of GP Software. The APL trademark
licence limits use of the mark to Amiga-like platforms (AmigaOS, MorphOS,
AROS, and compatibles); use on Windows, macOS, Linux, etc. is **not**
permitted.

The release of Directory Opus 5 under the APL does **not** affect the
commercial status of *Directory Opus for Windows*. See
<https://www.gpsoft.com.au> for that product.

## Credits

- **Jonathan Potter / GP Software** — original Directory Opus 5 / Magellan II
  (1993-2012)
- **DOPUS5 Open Source Team** — initial post-APL GCC ports and platform work
  for OS3 / OS4 / MorphOS / AROS (2012-2013)
- **Dimitris Panokostas** ([@midwan](https://github.com/midwan)) — current
  maintainer (2023-present)
- All other contributors listed in [`AUTHORS`](AUTHORS) and the project's
  [contributor graph](https://github.com/BlitterStudio/dopus5/graphs/contributors)

Special thanks to Olaf Schönweiß and the
[power2people.org](https://www.power2people.org) contributors who funded the
original 2012 open-source release.

## Links

- **Issue tracker:** [GitHub Issues](https://github.com/BlitterStudio/dopus5/issues)
- **CI / build status:** [GitHub Actions](https://github.com/BlitterStudio/dopus5/actions/workflows/makefile.yml)
- **Historical project site:** <https://sourceforge.net/p/dopus5allamigas/>
- **Directory Opus for Windows (commercial):** <https://www.gpsoft.com.au>
