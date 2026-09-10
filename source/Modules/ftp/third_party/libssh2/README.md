# libssh2 for Amiga cross builds

This directory contains the build glue for the FTP module's optional SFTP
backend. It fetches a pristine libssh2 release tarball and builds a static
`libssh2.a` for the selected Amiga target.

libssh2 and zlib are third-party components with their own license notices.
Distributions that ship an SFTP-enabled FTP module should include the
repository's `THIRD_PARTY_NOTICES.md` file, or equivalent notices, alongside
the binaries.

The FTP module makefiles use this automatically when built with `sftp=yes`.
For example, inside the corresponding toolchain Docker image listed in the
[root README](../../../../../README.md):

```sh
make -f makefile.os3 sftp=yes
make -f makefile.os4 sftp=yes
make -f makefile.mos sftp=yes
```

To use a preinstalled libssh2 instead, pass `SFTP_BACKEND=system` and provide
`SFTP_CFLAGS` and `SFTP_LIBS` on the make command line.
