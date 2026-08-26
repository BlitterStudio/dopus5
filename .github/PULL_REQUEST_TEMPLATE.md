<!--
Thanks for opening a PR!  Please fill in the sections below so the
review goes smoothly.  See CONTRIBUTING.md for the full expectations.
-->

## Summary

<!-- One or two sentences on what this PR does and why. -->

## Issue

<!-- Reference the related issue, e.g. "Resolves #42" or "Refs #42". -->

## Changes

<!-- Bullet-point list of the user-visible / behavioural changes. -->
- 

## Affected platforms

<!-- Tick the platforms you've actually built / tested.  CI will cover
the rest, but please be honest about what you exercised locally. -->
- [ ] AmigaOS 3 (`os3`)
- [ ] AmigaOS 3 / 68000 (`os3-68000`)
- [ ] AmigaOS 4 (`os4`)
- [ ] MorphOS (`mos`)
- [ ] AROS i386 (`i386-aros`)
- [ ] AROS x86_64 (`x86_64-aros`)
- [ ] AROS aarch64 (`aarch64-aros`)

## Verification

<!-- How did you verify this works?  e.g. "Booted on real A1200, opened
a lister, dragged 50 icons", "ran make -C source/Modules/ftp -f
makefile.tests test", "smoke-tested under WinUAE 4.10.x", etc. -->

## Checklist

- [ ] CI is green (or expected failures are explained).
- [ ] `ChangeLog` updated with a `[dopus5 next]` entry.
- [ ] `documents/DOpus5.guide` updated and `@$VER` bumped if user-visible
      behaviour changed.
- [ ] Relevant `*_REVISION` in `source/Include/dopus/version.h` bumped if
      a versioned binary changed.
- [ ] No new compiler warnings introduced on any platform.
- [ ] No drive-by reformatting of unrelated code.
