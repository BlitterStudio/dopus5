# Third-Party Notices

Directory Opus 5 is released under the AROS Public License version 1.1; see
`COPYING`. Optional build configurations may include or link third-party
components under their own licenses. Binary distributors should include this
file, or equivalent notices, with releases that enable those components.

## FTP SFTP Support

When the FTP module is built with `sftp=yes`, SFTP support is implemented with
libssh2. The Amiga cross-build helper under `source/Modules/ftp/third_party`
fetches libssh2 1.11.1 by default and builds it as a static library. The SFTP
build also links zlib from the target toolchain. libssh2 may use the target
SSL/crypto implementation configured for that build, for example AmiSSL/OpenSSL
on Amiga targets; those components remain under their own licenses and notices.

## libssh2

Source: https://libssh2.org/

Default version fetched by the Amiga cross-build helper: 1.11.1.

License notice from libssh2 1.11.1:

```text
Copyright (C) 2004-2007 Sara Golemon <sarag@libssh2.org>
Copyright (C) 2005,2006 Mikhail Gusarov <dottedmag@dottedmag.net>
Copyright (C) 2006-2007 The Written Word, Inc.
Copyright (C) 2007 Eli Fant <elifantu@mail.ru>
Copyright (C) 2009-2023 Daniel Stenberg
Copyright (C) 2008, 2009 Simon Josefsson
Copyright (C) 2000 Markus Friedl
Copyright (C) 2015 Microsoft Corp.
All rights reserved.

Redistribution and use in source and binary forms,
with or without modification, are permitted provided
that the following conditions are met:

  Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

  Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the following
  disclaimer in the documentation and/or other materials
  provided with the distribution.

  Neither the name of the copyright holder nor the names
  of any other contributors may be used to endorse or
  promote products derived from this software without
  specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
```

## zlib

Source: https://zlib.net/

The sacredbanana Amiga cross-build image currently provides zlib 1.3.1.1-motley
for the m68k-amigaos target. Other target toolchains may provide a different
compatible zlib build.

License notice from the zlib headers shipped in the cross-build toolchain:

```text
Copyright notice:

 (C) 1995-2024 Jean-loup Gailly and Mark Adler

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Jean-loup Gailly        Mark Adler
  jloup@gzip.org          madler@alumni.caltech.edu
```

## MWB_DOpus 8

Source: https://aminet.net/package/pix/mwb/MWB_DOpus8c

The MagicWB images and icons in `IconSets/MagicWB` of the release archives
come from MWB_DOpus 8.0, MagicWorkbench toolbar icons for Directory Opus 5.5
by Trevor Morris and Leo Davidson (1996), which was also on the Directory
Opus 5.5 Extras disk. `archive/tools/magicwb_iconset.py` builds the set from
`MWB_DOpus8c.lha`: the toolbar images and their icons are copied unchanged;
the program icons use MWB_DOpus 8 images with the tool types and settings of
the standard icons, and the Command, FTP and print icons and `Toggle.small`
are made from MWB_DOpus 8 images.
