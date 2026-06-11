/*

Directory Opus 5
Original APL release version 5.82
Copyright 1993-2012 Jonathan Potter & GP Software
Copyright 2012-2013 DOPUS5 Open Source Team
Copyright 2023-2026 Dimitris Panokostas

This program is free software; you can redistribute it and/or
modify it under the terms of the AROS Public License version 1.1.

*/

#include "ftp_lister_entry.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define FTP_FIBF_HIDDEN (1 << 7)
#define FTP_FIBF_SCRIPT (1 << 6)
#define FTP_FIBF_PURE (1 << 5)
#define FTP_FIBF_ARCHIVE (1 << 4)
#define FTP_FIBF_READ (1 << 3)
#define FTP_FIBF_WRITE (1 << 2)
#define FTP_FIBF_EXECUTE (1 << 1)
#define FTP_FIBF_DELETE (1 << 0)

static void ftp_lister_copy_string(char *dest, size_t dest_size, const char *src)
{
	size_t len;

	if (!dest || dest_size == 0)
		return;

	if (!src)
		src = "";

	len = strlen(src);
	if (len >= dest_size)
		len = dest_size - 1;

	if (len)
		memcpy(dest, src, len);
	dest[len] = 0;
}

static int ftp_lister_parse_ulong_token(const char **cursor, unsigned long *value)
{
	char *end;

	if (!cursor || !*cursor || !value || **cursor < '0' || **cursor > '9')
		return 0;

	errno = 0;
	*value = strtoul(*cursor, &end, 10);
	if (errno == ERANGE || end == *cursor || *end != ' ')
		return 0;

	*cursor = end + 1;
	return 1;
}

static int ftp_lister_parse_long_token(const char **cursor, long *value)
{
	char *end;

	if (!cursor || !*cursor || !value)
		return 0;

	errno = 0;
	*value = strtol(*cursor, &end, 10);
	if (errno == ERANGE || end == *cursor || *end != ' ')
		return 0;

	*cursor = end + 1;
	return 1;
}

int ftp_lister_parse_entry_info(const char *fileinfo,
								const char *entryname,
								struct ftp_lister_entry_info *entry)
{
	const char *p;
	const char *comment;
	const char *comment_end;
	size_t comment_len;
	size_t entryname_len;
	unsigned long ignored_selection;
	long type;
	char prot[8];

	if (!fileinfo || !entryname || !entry || !entryname[0])
		return 0;

	entryname_len = strlen(entryname);
	if (strncmp(fileinfo, entryname, entryname_len) != 0 || fileinfo[entryname_len] != ' ')
		return 0;

	memset(entry, 0, sizeof(*entry));
	entry->unixprot = 0777;
	ftp_lister_copy_string(entry->name, sizeof(entry->name), fileinfo);
	entry->name[entryname_len < sizeof(entry->name) ? entryname_len : sizeof(entry->name) - 1] = 0;

	p = fileinfo + entryname_len + 1;
	if (!ftp_lister_parse_ulong_token(&p, &entry->size))
		return 0;

	if (!ftp_lister_parse_long_token(&p, &type) || type < INT_MIN || type > INT_MAX)
		return 0;
	entry->type = (int)type;

	if (!ftp_lister_parse_ulong_token(&p, &ignored_selection))
		return 0;

	if (!ftp_lister_parse_ulong_token(&p, &entry->seconds))
		return 0;

	if (strlen(p) < sizeof(prot) || p[sizeof(prot)] != ' ')
		return 0;
	memcpy(prot, p, sizeof(prot));
	comment = p + sizeof(prot);

	if (prot[0] == 'h')
		entry->prot |= FTP_FIBF_HIDDEN;
	if (prot[1] == 's')
		entry->prot |= FTP_FIBF_SCRIPT;
	if (prot[2] == 'p')
		entry->prot |= FTP_FIBF_PURE;
	if (prot[3] == 'a')
		entry->prot |= FTP_FIBF_ARCHIVE;
	if (prot[4] != 'r')
	{
		entry->prot |= FTP_FIBF_READ;
		entry->unixprot &= ~0444;
	}
	if (prot[5] != 'w')
	{
		entry->prot |= FTP_FIBF_WRITE;
		if (prot[7] != 'd')
			entry->unixprot &= ~0222;
	}
	if (prot[6] != 'e')
	{
		entry->prot |= FTP_FIBF_EXECUTE;
		entry->unixprot &= ~0111;
	}
	if (prot[7] != 'd')
		entry->prot |= FTP_FIBF_DELETE;

	if (comment[0] && comment[1])
	{
		comment_end = strchr(comment, '\n');
		comment_len = comment_end ? (size_t)(comment_end - comment) : strlen(comment);
		if (comment_len >= sizeof(entry->comment))
			comment_len = sizeof(entry->comment) - 1;
		if (comment_len)
			memcpy(entry->comment, comment, comment_len);
		entry->comment[comment_len] = 0;
	}
	return 1;
}

int ftp_lister_quote_entry_name(const char *entryname, char *quoted, size_t quoted_size)
{
	size_t len;
	char quote = '"';

	if (!entryname || !quoted)
		return 0;

	if (strchr(entryname, '"'))
	{
		if (strchr(entryname, '\''))
			return 0;
		quote = '\'';
	}

	len = strlen(entryname);
	if (quoted_size < len + 3)
		return 0;

	quoted[0] = quote;
	memcpy(quoted + 1, entryname, len);
	quoted[len + 1] = quote;
	quoted[len + 2] = 0;
	return 1;
}
