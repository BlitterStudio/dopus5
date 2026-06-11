/*

Directory Opus 5
Original APL release version 5.82
Copyright 1993-2012 Jonathan Potter & GP Software
Copyright 2012-2013 DOPUS5 Open Source Team
Copyright 2023-2026 Dimitris Panokostas

This program is free software; you can redistribute it and/or
modify it under the terms of the AROS Public License version 1.1.

*/

#ifndef FTP_LISTER_ENTRY_H
#define FTP_LISTER_ENTRY_H

#include <stddef.h>

#define FTP_LISTER_ENTRY_NAME_BUFSIZE 256
#define FTP_LISTER_ENTRY_COMMENT_BUFSIZE 79
#define FTP_LISTER_QUOTED_NAME_BUFSIZE (FTP_LISTER_ENTRY_NAME_BUFSIZE + 3)

struct ftp_lister_entry_info
{
	char name[FTP_LISTER_ENTRY_NAME_BUFSIZE + 1];
	unsigned long size;
	int type;
	unsigned long seconds;
	long prot;
	char comment[FTP_LISTER_ENTRY_COMMENT_BUFSIZE + 1];
	unsigned long unixprot;
};

int ftp_lister_parse_entry_info(const char *fileinfo,
								const char *entryname,
								struct ftp_lister_entry_info *entry);
int ftp_lister_quote_entry_name(const char *entryname, char *quoted, size_t quoted_size);

#endif
