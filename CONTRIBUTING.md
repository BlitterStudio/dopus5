# Contributing to Directory Opus 5

Thanks for your interest in helping out — patches, bug reports, ports and
documentation improvements are all welcome.

This document covers the practical bits: where to discuss work, how to build
and test, and what we expect from a pull request.

## Before you start

For anything bigger than a small fix or typo, **please open an issue first**
so we can sync on direction. Some areas of the codebase have non-obvious
constraints (e.g. byte-order portability for AROS x86_64, OS3 stack budgets,
the 5.90-era IFF schema migration in `config_open.c`) that are easier to
flag up front than to chase down in code review.

- 🐛 Bugs: <https://github.com/BlitterStudio/dopus5/issues>
- 💬 Design / "is this a good idea" questions: open a `Task` issue or use
  the project's [Discussions](https://github.com/BlitterStudio/dopus5/discussions)
- 🔍 Looking for something to work on: search the issue tracker for `good
  first issue` or check `local-notes` items mentioned in the ChangeLog.

## Building

The build system is GCC + GNU make, cross-compiled from a Unix-like host.
The recommended path is the prebuilt Docker images that CI uses — they
match exactly what runs on the build server, so "works on my machine" maps
1:1 to "works in CI".

| Target              | Docker image                                 |
| ------------------- | -------------------------------------------- |
| AmigaOS 3 (m68k)    | `sacredbanana/amiga-compiler:m68k-amigaos`   |
| AmigaOS 4 (PPC)     | `sacredbanana/amiga-compiler:ppc-amigaos`    |
| MorphOS (PPC)       | `sacredbanana/amiga-compiler:ppc-morphos`    |
| AROS i386 (ABIv0)   | `midwan/aros-compiler:i386-aros`             |
| AROS x86_64 (ABIv11)| `midwan/aros-compiler:x86_64-aros`           |
| AROS aarch64 (ABIv11)| `midwan/aros-compiler:aarch64-aros`          |

A typical build looks like:

```sh
docker run --rm -v "$PWD":/work sacredbanana/amiga-compiler:m68k-amigaos \
    sh -c "cd /work/source && make os3 clean && make os3 all"
```

Available platform targets are `os3`, `os3-68000`, `os4`, `mos`,
`i386-aros`, `x86_64-aros`, and `aarch64-aros`. Add `debug=no` for an
optimised release build, or `release` to also produce the `.lha` archive in
`releases/`.

The full reference (including `release-package`, the SFTP build flow and
native-Amiga builds) is in [`README.md`](README.md).

## Testing

If you touched anything in `source/Modules/ftp/`, run the host-side parser
/ protocol / TLS tests before pushing:

```sh
make -C source/Modules/ftp -f makefile.tests test
```

If you have `libssh2` development headers installed locally, also run:

```sh
make -C source/Modules/ftp -f makefile.tests test-libssh2
```

CI runs both of these before the platform builds, so a green CI run is
a good sanity check, but iterating locally is much faster.

For program / library / module changes, do at minimum a clean OS3 build
and check that no new warnings appeared in the area you touched. CI
covers the full matrix on PR.

## Code style

- Follow the surrounding style of whichever file you're editing. The
  codebase uses tabs for indentation and Allman-ish braces, but the
  exact dialect varies a bit between files inherited from the original
  GP Software / DOPUS5 Open Source Team trees.
- Don't reformat code you're not actually changing — drive-by reformats
  make reviews much harder and pollute `git blame`.
- Don't add new compiler warnings. If GCC starts complaining about your
  changes on any platform, the warning needs to go away before the PR
  can land.
- Avoid dragging in new third-party dependencies casually. Open an issue
  first to discuss the trade-off (especially given the OS3 binary size
  budget and the AROS-x86_64 cross-compiler quirks).

## Pull request expectations

- One logical change per PR. Keep the diff focused and easy to review.
- Reference the issue you're addressing in the PR body (e.g. "Resolves
  #42").
- Add a `ChangeLog` entry at the top of the file in the existing format:

  ```
  [dopus5 next] <short headline>
  <one-line version-status sentence; "ships in <upcoming version>"
  if no module/library version bump, otherwise mention the bump>

  <category>:
  - bullet describing what changed and (briefly) why
  ```

  See the existing `[dopus5 5.101]` entries for examples.
- Update `documents/DOpus5.guide` if your change affects user-facing
  behaviour and bump its `@$VER` line.
- Bump the relevant `*_REVISION` constant in
  `source/Include/dopus/version.h` if your change affects the program,
  library / modules, or the small command binaries (DOpusRT5 / LoadDB /
  ViewFont). The release process consolidates revision numbers, so
  individual PRs only need to bump them when they actually touch
  versioned binary surfaces.
- CI (GitHub Actions) must be green before merge. The matrix builds all
  platforms in both debug and release configurations, plus runs the FTP
  tests; flaky failures should be flagged in the PR.

## Branch naming

Anything reasonably descriptive is fine — most of the existing branches
follow `feature/...`, `fix/...`, `docs/...`, `chore/...` prefixes, but
that's a convention rather than a hard rule.

(Note for AI-assistant users: please **don't** prefix branch names with
the assistant's name — keep the branch name about the change itself.)

## License

By submitting a PR you confirm that your contribution is provided under
the [AROS Public License v1.1](COPYING) (the project licence) and that
you have the right to license it under those terms.
