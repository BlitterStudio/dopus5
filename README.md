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
- [Downloads](#downloads)
- [Building](#building)
  - [Per-platform make targets](#per-platform-make-targets)
  - [Release archives](#release-archives)
  - [SFTP support](#sftp-support)
  - [xadopus.module on AROS](#xadopusmodule-on-aros)
- [Testing](#testing)
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

## Downloads

Pre-built binaries for every supported platform are published under
[Releases](https://github.com/BlitterStudio/dopus5/releases). Each tagged
release ships one `DOpus5_<rev>_<platform>.lha` per target.

## Building

The project builds with GCC-based cross-compilation toolchains. The
recommended way is via the prebuilt Docker images used by CI:

| Target              | Docker image                                 |
| ------------------- | -------------------------------------------- |
| AmigaOS 3 (m68k)    | `sacredbanana/amiga-compiler:m68k-amigaos`   |
| AmigaOS 4 (PPC)     | `sacredbanana/amiga-compiler:ppc-amigaos`    |
| MorphOS (PPC)       | `sacredbanana/amiga-compiler:ppc-morphos`    |
| AROS i386 (ABIv0)   | `midwan/aros-compiler:i386-aros`             |
| AROS x86_64 (ABIv11)| `midwan/aros-compiler:x86_64-aros`           |
| AROS aarch64 (ABIv11)| `midwan/aros-compiler:aarch64-aros`          |

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
variant).

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
