/*

Directory Opus 5
Original APL release version 5.82
Copyright 1993-2012 Jonathan Potter & GP Software
Copyright 2012-2013 DOPUS5 Open Source Team
Copyright 2023-2026 Dimitris Panokostas

This program is free software; you can redistribute it and/or
modify it under the terms of the AROS Public License version 1.1.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
AROS Public License for more details.

The release of Directory Opus 5 under the GPL in NO WAY affects
the existing commercial status of Directory Opus for Windows.

For more information on Directory Opus for Windows please see:

				 http://www.gpsoft.com.au

*/

/* util.c */

/*	19.3.99	Modified unix_line_to_entryinfo to account for foreign date strings
 *		Dates for these are set at 0 time 1/1/78
 */

#include <dopus/common.h>
#include <proto/dopus5.h>
#include <proto/module.h>

#include "ftp.h"
#include "ftp_opusftp.h"
#include "ftp_list.h"
#include "ftp_lister.h"
#include "ftp_recursive.h"
#include "ftp_util.h"

#define min(a, b) (((a) <= (b)) ? (a) : (b))

extern struct opusftp_globals og;

IPTR ftp_parse_handle(const char *text)
{
	UQUAD value = 0;

	if (!text)
		return 0;

	while (*text >= '0' && *text <= '9')
	{
		value = (value * 10) + (*text - '0');
		++text;
	}

	return (IPTR)value;
}

void ftp_format_handle(char *buf, IPTR handle)
{
	char tmp[FTP_HANDLE_BUFSIZE];
	int pos = sizeof(tmp) - 1;
	UQUAD value = (UQUAD)handle;

	tmp[pos] = 0;
	do
	{
		tmp[--pos] = '0' + (value % 10);
		value /= 10;
	} while (value && pos > 0);

	strcpy(buf, tmp + pos);
}

// Months of the year for converting FTP LIST output
const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

/*********************************/

//
//  display a message using Opus AsyncReq call
//
//	NB: 	The ipc must be the calling process's ipc data
//		Not that of the main opus ipc
//
long display_msg(struct opusftp_globals *ogp, IPCData *ipc, struct Window *win, ULONG flags, char *str)
{
	long result;
	struct TagItem tags[] = {{AR_Window, 0},
							 {AR_Screen, 0},
							 {AR_Message, 0},
							 {AR_Button, 0},
							 {TAG_IGNORE, 0},
							 {TAG_IGNORE, 0},
							 {TAG_DONE}};

	if (!ipc || !str)
		return 0;

	if ((struct Task *)ipc->proc != FindTask(0))
		D(bug("** DISPLAYMSG bad task!!\n"));

	D(bug("display_msg() <%s>\n", str));

	// see if they supplied a window
	if (win)
	{
		tags[0].ti_Data = (IPTR)win;
		tags[1].ti_Data = (IPTR)win->WScreen;
	}

	// if not then use the global pointer to get the Opus screen
	else if (ogp)
	{
		tags[1].ti_Data = (IPTR)ogp->og_screen;
	}

	tags[2].ti_Data = (IPTR)str;

	// Retry/Abort buttons?
	if (flags & DSPMSG_RETRYABORT)
	{
		tags[3].ti_Data = (IPTR)GetString(locale, MSG_TRY_AGAIN);
		tags[4].ti_Tag = AR_Button;
		tags[4].ti_Data = (IPTR)GetString(locale, MSG_FTP_SKIP);
		tags[5].ti_Tag = AR_Button;
		tags[5].ti_Data = (IPTR)GetString(locale, MSG_ABORT);
	}

	// Just an OK button
	else
	{
		tags[3].ti_Data = (IPTR)GetString(locale, MSG_FTP_OKAY);
	}

	result = AsyncRequest(ipc, REQTYPE_SIMPLE, win, NULL, NULL, tags);

	return result;
}

/********************************/

//
//	Get the name of the logged-in user on the Amiga
//
char *getlogname(char *buf)
{
	if (GetVar("LOGNAME", buf, USERNAMELEN, 0) <= 0)
	{
		// Empty string if unsuccessful
		strcpy(buf, "''");
	}

	return buf;
}

/********************************/

/*
 *	Get the hostname of the user's Amiga (for anonymous FTPs)
 ***********************************************************
 *	***	MAKESURE socketlib initialised has	****
 *	***	been opened before this call		****
 *	****************************************************
 */

char *getuseraddress(char *buf)
{
	char logname[USERNAMELEN + 1];
	int l;

	sprintf(buf, "%s@", getlogname(logname));

	l = strlen(buf);

	// Empty string if unsuccessful
	if (gethostname(buf + l, ADDRESSLEN - l) < 0)
		strcpy(buf, "''");

	return buf;
}

/********************************/

/*
 *	Initialize a ListLock and counter
 */

void ad_InitListLock(struct ListLock *l, int *count)
{
	NewList(&l->list);
	InitSemaphore(&l->lock);
	*count = 0;
}

/********************************/

/*
 *	Remove a node from a ListLock and decrement its counter
 */

void ListLockRemove(struct ListLock *l, struct Node *n, int *count)
{
	ObtainSemaphore(&l->lock);
	Remove(n);
	(*count)--;
	ReleaseSemaphore(&l->lock);
}

/********************************/

void ListLockAddHead(struct ListLock *l, struct Node *n, int *count)
{
	ObtainSemaphore(&l->lock);
	AddHead(&l->list, n);
	(*count)++;
	ReleaseSemaphore(&l->lock);
}

/********************************/
/*
BOOL IsListLockEmpty( struct ListLock *l )
{
BOOL retval;

ObtainSemaphore( &l->lock );
retval = (BOOL)(IsListEmpty( &l->list ));
ReleaseSemaphore( &l->lock );

return retval;
}
*/

/********************************/

/**
 **	Time and date conversion functions
 **/

/*
 *	Converts a DateStamp to a number of seconds
 */

ULONG datestamp_to_seconds(struct DateStamp *ds)
{
	ULONG secs;

	secs = ds->ds_Days * 86400;
	secs += ds->ds_Minute * 60;
	secs += ds->ds_Tick / TICKS_PER_SECOND;

	return secs;
}

/*
 *	Converts a number of seconds to a DateStamp
 */

void seconds_to_datestamp(struct DateStamp *ds, ULONG secs)
{
	ds->ds_Days = secs / 86400;
	secs %= 86400;
	ds->ds_Minute = secs / 60;
	secs %= 60;
	ds->ds_Tick = secs * TICKS_PER_SECOND;
}

//
//	convert the number of seconds since Jan 1 1978 to an AmigaDOS time string
//
void seconds_to_datestring(ULONG seconds, char *buf)
{
	struct DateTime dt;
	char datebuf[80], timebuf[80];
	char ampm;

	if (seconds != EI_SECONDS_UNKNOWN)
	{
		seconds_to_datestamp(&dt.dat_Stamp, seconds);

		dt.dat_Format = FORMAT_DOS;	 // environment->settings->date_format
		dt.dat_Flags = DTF_SUBST;	 // also from environment somewhere
		dt.dat_StrDay = 0;
		dt.dat_StrDate = datebuf;
		dt.dat_StrTime = timebuf;
		DateToStr(&dt);
		ampm = dt.dat_Stamp.ds_Minute / (12 * 60) ? 'p' : 'a';
	}
	else
	{
		strcpy(datebuf, "???");
		strcpy(timebuf, "??");
		ampm = '?';
	}

	sprintf(buf, "%s   %s%lc", datebuf, timebuf, ampm);
}

/********************************/

/*
 *	Call SetFileDate() with seconds only
 */

int setfiledate_secs(char *path, ULONG secs)
{
	struct DateStamp ds;

	ds.ds_Days = secs / 86400;
	secs %= 86400;
	ds.ds_Minute = secs / 60;
	secs %= 60;
	ds.ds_Tick = secs * TICKS_PER_SECOND;

	return SetFileDate(path, &ds);
}

/********************************/

//
//
//
ULONG getfiledate_secs(char *path)
{
	BPTR lock;
	D_S(struct FileInfoBlock, fib)
	ULONG secs = 0;

	if ((lock = Lock(path, ACCESS_READ)))
	{
		Examine(lock, fib);

		secs = datestamp_to_seconds(&fib->fib_Date);

		UnLock(lock);
	}

	return secs;
}

/********************************/

//
//
//
ULONG get_curr_secs(void)
{
	struct DateStamp ds = {0};
	ULONG secs;

	DateStamp(&ds);

	secs = ds.ds_Days * 86400;
	secs += ds.ds_Minute * 60;
	secs += ds.ds_Tick / TICKS_PER_SECOND;

	return secs;
}

/********************************/

//
//	Converts a string of quoted names (as passed from Opus) to an array of strings
//
char **alloc_name_array(char *initstring)
{
	char *p1, *p2;
	int ok = TRUE;
	int wordcount = 0;
	int charcount = 0;
	APTR buf;
	char **cpp;
	char *cp;

	// Count words and characters

	p1 = initstring + 1;
	p2 = initstring + 1;

	while (ok)
	{
		while (ok)
		{
			p2++;
			if (*p2 == '\"')
			{
				wordcount++;
				charcount += (p2 - p1);
				break;
			}
			else if (*p2 == 0 || *p2 == '\n' || *p2 == '\r')
				ok = FALSE;
		}
		if (*(p2 + 1) == 0)
			break;
		p2 += 3;
		p1 = p2;
	}

	if ((buf = AllocVec((wordcount + 1) * sizeof(char *) + charcount + wordcount, MEMF_CLEAR)))
	{
		cpp = buf;
		cp = (char *)(cpp + wordcount + 1);

		p1 = initstring + 1;
		p2 = initstring + 1;

		while (ok)
		{
			while (ok)
			{
				p2++;
				if (*p2 == '\"')
				{
					*cpp++ = cp;
					stccpy(cp, p1, p2 - p1 + 1);
					cp += (p2 - p1 + 1);
					break;
				}
				else if (*p2 == 0 || *p2 == '\n' || *p2 == '\r')
					ok = FALSE;
			}
			if (*(p2 + 1) == 0)
				break;
			p2 += 3;
			p1 = p2;
		}
	}

	return buf;
}

/********************************/

//
//
//	Concatenate a value onto a string in bytes, kilobytes, or megabytes format
//
void cat_bytes(char *dest, unsigned long value)
{
	char buf[24];

	// Less than one kilobyte
	if (value < 1024)
		sprintf(buf, "%lu", value);

	else if (value < 0x100000)
		// Less than one megabyte - one decimal place
		sprintf(buf, "%lu.%luK", value >> 10, ((value & 1023) * 10) >> 10);
	else
		// One megabyte or more - two decimal places
		sprintf(buf, "%lu.%02luM", value >> 20, ((value & 0xfffff) * 100) >> 20);

	strcat(dest, buf);
}

/********************************/

//
//	Count the filenames in a concatenated string delimited by double quotes
//
int count_entries(char *names)
{
	char *p;
	int ok = TRUE;
	int count = 0;

	p = names + 1;

	while (ok)
	{
		while (ok)
		{
			p++;
			if (*p == '\"')
			{
				count++;
				break;
			}
			else if (*p == 0 || *p == '\n' || *p == '\r')
				ok = FALSE;
		}

		// There should be a space or a NULL byte
		if (*(p + 1) == 0)
			break;
		p += 3;
	}
	return count;
}

/********************************/

//
//	Convert an FTP LIST line to an entry_info structure
//	Returns 1 if conversion succeeded, 0 otherwise
//	Entry must point to cleared memory
//
//	19.3.99	 modified for foreigh date strings
//	Dates for these are set at 0 time 1/1/78

int unix_line_to_entryinfo(struct entry_info *entry, const char *line, ULONG flags)
{
	char *p, *p2;						 // Pointer to character in line we're parsing
	char own_grp_byt[OGBLEN + 1] = {0};	 // Could be Owner, Group, or Bytes
	char grp_byt_mon[GBMLEN + 1] = {0};	 // Could be Group, Bytes, or Month
	char byt_dat[BDLEN + 1] = {0};		 // Could be Bytes or Date
	int nametail;						 // Last character of file name
	int commenttail = 0;				 // Last character of file comment
	IPTR fred_hack;						 // Number of spaces between date and year/time
	BOOL date_ok = TRUE;				 // For foreign dates in non english

	struct ClockData filedate = {0};
	struct tm *today;

	// char              env;

	// Valid?
	if (!entry || !line)
		return 0;

	// Short line?
	if (strlen(line) < 27)
		return 0;

	// Known bad line?
	if (!strcmp(line, "Volume or directory is empty."))
		return 0;

	D(bug("line <%s>\n", line));

	// Type - Directory, Link, or File?
	switch (line[0])
	{
	// Directory link
	case 'D':
		entry->ei_type = 3;
		break;

	// Directory
	case 'd':
		entry->ei_type = 1;
		break;

	// File link
	case 'h':
		entry->ei_type = -3;
		break;

	// Unknown link (usually a softlink which can be to a file or a dir!)
	case 'l':
		entry->ei_type = 0;
		break;

	// File (usually '-')
	default:
		entry->ei_type = -1;
		break;
	}

	D(bug("type %ld ", (long)entry->ei_type));

	entry->ei_prot = FIBF_READ | FIBF_WRITE | FIBF_EXECUTE | FIBF_DELETE;

	// Readable? (Owner)
	if (tolower(line[1]) == 'r')
	{
		entry->ei_prot &= ~FIBF_READ;
		entry->ei_unixprot |= 0400;
	}

	// Writeable? (Owner)
	if (tolower(line[2]) == 'w')
	{
		entry->ei_prot &= ~(FIBF_WRITE | FIBF_DELETE);
		entry->ei_unixprot |= 0200;
	}

	// Executable? (Owner)
	if (tolower(line[3]) == 'x')
	{
		entry->ei_prot &= ~FIBF_EXECUTE;
		entry->ei_unixprot |= 0100;
	}

	// Readable? (Group)
	if (tolower(line[1]) == 'r')
		entry->ei_unixprot |= 0040;

	// Writeable? (Group)
	if (tolower(line[2]) == 'w')
		entry->ei_unixprot |= 0020;

	// Executable? (Group)
	if (tolower(line[3]) == 'x')
		entry->ei_unixprot |= 0010;

	// Readable? (Other)
	if (tolower(line[1]) == 'r')
		entry->ei_unixprot |= 0004;

	// Writeable? (Other)
	if (tolower(line[2]) == 'w')
		entry->ei_unixprot |= 0002;

	// Executable? (Owner)
	if (tolower(line[3]) == 'x')
		entry->ei_unixprot |= 0001;

	// Must be a blank
	if (line[10] != ' ')
		return 0;

	// Skip to next field
	p = stpblk(line + 10);

	D(bug("<%s>\n", p));

	// Skip 'links' field if there is one
	if (!(flags & UI_NO_LINK_FIELD) && (!(p = strchr(p, ' '))))
		return 0;

	// Skip to next field
	p = stpblk(p);

	// Get next field (Owner, Group, or Bytes)
	p = stptok(p, own_grp_byt, OGBLEN, " ");
	p = stpblk(p);

	// Get next field (Group, Bytes, or Month)
	p = stptok(p, grp_byt_mon, GBMLEN, " ");
	p = stpblk(p);

	// If not numeric, this is the Month field
	if (!isdigit(*p))
	{
		// So GBM was the Bytes field
		entry->ei_size = atoi(grp_byt_mon);
	}

	// Otherwise it's either the Bytes or Date field
	else
	{
		// Get it for now, decide later
		p = stptok(p, byt_dat, BDLEN, " ");
		p = stpblk(p);

		// Is this the Month field?
		if (!isdigit(*p))
		{
			// So BD was the Bytes field
			entry->ei_size = atoi(byt_dat);
		}
		// Must be hour/min or year
		else
		{
			// So OGB was the Bytes field
			entry->ei_size = atoi(own_grp_byt);

			// Skip Time
			p = strchr(p, ' ');
		}
	}

	// We're now at the Date & Time field

	// Convert Month field to number
	for (filedate.month = 1; filedate.month <= 12 && strncmp(months[filedate.month - 1], p, 3); ++filedate.month)
		;

	// Skip Month field
	if (!(p = strchr(p, ' ')))
		return 0;

	// Skip whitespace
	p = stpblk(p);

	// Valid month?
	if (filedate.month > 12)
	{
		// Foreign date
		date_ok = FALSE;

		// No longer bail out if unresolved month string
		// so will set date to 1/1/78
		// return 0;
	}

	// Get Day of Month field
	filedate.mday = atoi(p);

	// Skip Day
	if (!(p = strchr(p, ' ')))
		return 0;

	// Keep track of number of spaces between date and year/time for Fred
	fred_hack = (IPTR)p;

	p = stpblk(p);

	fred_hack = (IPTR)p - fred_hack;

	// Hour/Minute format?
	if (p[1] == ':' || p[2] == ':')
	{
#ifdef __MORPHOS__
		struct DateStamp ds;
		struct ClockData cd;
		DateStamp(&ds);
		Amiga2Date(ds.ds_Days * 86400 + ds.ds_Minute * 60 + ds.ds_Tick / TICKS_PER_SECOND, &cd);
		filedate.year = cd.year;
		if (filedate.month > cd.month)
			--filedate.year;
#else
		time_t tt = time(0);
		today = localtime(&tt);

		// For files from the 60's or earlier, this means the 2060's (:

		filedate.year = today->tm_year + (today->tm_year >= 70 ? 1900 : 2000);

		// If file month is later than local month
		// ** ClockDate month starts at 1, time_t month starts at 0!!
		if (filedate.month > (today->tm_mon + 1))
			--filedate.year;
#endif

		filedate.hour = atoi(p);
		filedate.min = atoi(p + 3);
	}

	// Just the year then
	else
	{
		filedate.year = atoi(p);

		fred_hack = 3 - fred_hack;
	}

	// Calculate seconds
	if (date_ok)
		entry->ei_seconds = Date2Amiga(&filedate);
	else
		entry->ei_seconds = 0;

	// Skip Time/Year
	if (!(p = strchr(p, ' ')))
		return 0;

	// A hack for Frederic Steinfels <fst@active.ch>
	// Should allow entries which begin with space characters to work.
	// Note that this makes assumptions about how many spaces will be
	// between the date, year/time, and filename fields.
	// (Previously toggled via DOpus/ftp_fred_hack env var; now a per-site flag.)

	if (flags & UI_SPECIAL_DIR)
		p += fred_hack;
	else
		p = stpblk(p);

	// Time done, get name

	// Link name after file name?
	if (!(p2 = strstr(p, " ->")))
	{
		// No, so find end of line
		p2 = p + strlen(p);
		while (*(p2 - 1) == '\n' || *(p2 - 1) == '\r')
			--p2;
	}

	// Special identity character caused by 'ls -F'?
	nametail = *(p2 - 1);

	// Get everything but that
	if (strchr("/*@#\n\r", nametail))
		stccpy(entry->ei_name, p, min(p2 - p, FILENAMELEN + 1));
	else
		stccpy(entry->ei_name, p, min(p2 - p + 1, FILENAMELEN + 1));

	p = p2;

	p = stpblk(p);

	// . and .. are not supported!
	if (!strcmp(entry->ei_name, ".") || !strcmp(entry->ei_name, ".."))
		return 0;

	// Hidden entry?
	if ((flags & UI_DOT_HIDDEN) && entry->ei_name[0] == '.')
		entry->ei_prot |= FIBF_HIDDEN;

	// Link?
	if (entry->ei_type == 3 || entry->ei_type == -3 || entry->ei_type == 0)
	{
		// Source of link in comment field
		stptok(p, entry->ei_comment, COMMENTLEN, "\n\r");

		// Unknown link? - Let's think about that
		if (entry->ei_type == 0)
		{
			if (*entry->ei_comment)
				commenttail = entry->ei_comment[strlen(entry->ei_comment) - 1];

			// '/' at end means link to dir
			// '@' at end means softlink
			if (nametail == '/' || commenttail == '/')
				entry->ei_type = 3;

			// Assume softlinks are to dirs if we've found no better hints
			else if (nametail == '@')
			{
				if (flags & UI_LINKS_ARE_FILES)
					entry->ei_type = -3;
				else
					entry->ei_type = 3;
			}

			// Otherwise, link to file
			else
				entry->ei_type = -3;
		}
	}

	return 1;
}

/********************************/

//
//	Convert a Windows NT style FTP LIST line to an entry_info structure
//	Returns 1 if conversion succeeded, 0 otherwise
//	Entry must point to cleared memory
//
int nt_line_to_entryinfo(struct entry_info *entry, const char *line, ULONG flags)
{
	const char *p = line;
	struct ClockData filedate = {0};

	// Valid?
	if (!entry || !line)
		return 0;

	// Valid date format?
	if (!isdigit(p[0]) || !isdigit(p[1]) || p[2] != '-' || !isdigit(p[3]) || !isdigit(p[4]) || p[5] != '-' ||
		!isdigit(p[6]) || !isdigit(p[7]))
		return 0;

	// Get date
	filedate.month = atoi(line);
	filedate.mday = atoi(line + 3);

	// For files from the 60's or earlier, this means the 2060's (:
	filedate.year = atoi(line + 6);
	filedate.year += filedate.year >= 70 ? 1900 : 2000;

	// Skip to time field
	p = stpblk(p + 8);

	// Valid time format?
	if (!isdigit(p[0]) || !isdigit(p[1]) || p[2] != ':' || !isdigit(p[3]) || !isdigit(line[4]) ||
		(p[5] != 'A' && p[5] != 'P') || p[6] != 'M')
		return 0;

	// Get time
	filedate.hour = atoi(p);
	filedate.min = atoi(p + 3);

	// Calculate seconds
	entry->ei_seconds = Date2Amiga(&filedate);

	// Skip to dir/size field
	p = stpblk(p + 7);

	// File or dir?
	if (!strncmp(p, "<DIR>", 5))
	{
		entry->ei_type = 1;
		entry->ei_size = 0;
	}
	else if (isdigit(*p))
	{
		entry->ei_type = -1;
		entry->ei_size = atoi(p);
	}
	else
		return 0;

	// Skip to name field
	p = strchr(p, ' ');
	p = stpblk(p);

	// Get name
	stptok(p, entry->ei_name, FILENAMELEN, "/*@#\n\r");

	// . and .. are NOT supported!
	if (!strcmp(entry->ei_name, ".") || !strcmp(entry->ei_name, ".."))
		return 0;

	// NT has no protection field
	entry->ei_prot = 0;
	entry->ei_unixprot = 0777;

	return 1;
}

/********************************/

//
//	Convert a FileInfoBlock into an entry_info
//
//	How much info do we need?
//	The replace requester uses name, size, and date
//	We also keep track of link status here
//	We ignore the comment field
//
void fileinfoblock_to_entryinfo(struct entry_info *ei, struct FileInfoBlock *fib)
{
	stccpy(ei->ei_name, fib->fib_FileName, FILENAMELEN + 1);

	ei->ei_prot = fib->fib_Protection;

	if (!(fib->fib_Protection & FIBF_READ))
		ei->ei_unixprot |= 0444;

	if (!(fib->fib_Protection & (FIBF_WRITE | FIBF_DELETE)))
		ei->ei_unixprot |= 0222;

	if (!(fib->fib_Protection & FIBF_EXECUTE))
		ei->ei_unixprot |= 0111;

	ei->ei_size = fib->fib_Size;

	ei->ei_seconds = datestamp_to_seconds(&fib->fib_Date);

	// Sort out file/dir/link info

	// File?
	if (fib->fib_DirEntryType < 0)
	{
		// Link to file?
		if (fib->fib_DirEntryType == ST_LINKFILE)
			ei->ei_type = -3;

		// Normal file (usually - could be something fancy though)
		else
			ei->ei_type = -1;
	}
	// Dir
	else
	{
		// Soft link (usually to a directory, but not always!)
		if (fib->fib_DirEntryType == ST_SOFTLINK)
		{
			ei->ei_type = 3;
			ei->ei_linkinfo = EI_LINK_ADOS_SOFTLINK;
		}

		// Hard link to a directory?
		else if (fib->fib_DirEntryType == ST_LINKDIR)
			ei->ei_type = 3;

		// Normal directory
		else
			ei->ei_type = 1;
	}
}

/********************************/

int mlsd_line_to_entryinfo(struct entry_info *entry, const char *line, ULONG flags)
{
	const char *str = line;
	const char *facts_end;
	const char *name;
	// char pair[256];
	char *pair;
	struct ClockData filedate = {0};
	int length;
	BOOL skip_entry = FALSE;
	size_t name_len;

	if (!str || !strchr(str, ';') || !(facts_end = strchr(str, ' ')))
		return 0;

	length = strlen(line) + 1;

	if (!(pair = AllocVec(length, MEMF_CLEAR)))
		return 0;

	D(bug("line <%s>\n", line));

	// partially based on the yafc parser
	while (str < facts_end)
	{
		const char *next = strchr(str, ';');
		char *factname;
		char *value;
		size_t pair_len;

		if (!next || next > facts_end)
			next = facts_end;

		pair_len = min((size_t)(next - str), (size_t)length - 1);
		memcpy(pair, str, pair_len);
		pair[pair_len] = 0;

		if (next < facts_end && *next == ';')
			str = next + 1;
		else
			str = next;

		if (!*pair)
			continue;

		D(bug("pair '%s'\n", pair));

		factname = pair;
		value = strchr(pair, '=');

		if (value)
		{
			*value = '\0';
			value++;
		}

		D(bug("factname '%s' value '%s'\n", factname, value));

		if (!value)
			continue;
		else if (!stricmp(factname, "size"))
		{
			entry->ei_size = atoi(value);
			D(bug(" -> size %d\n", entry->ei_size));
		}
		else if (!stricmp(factname, "type"))
		{
			if (!stricmp(value, "file"))
			{
				entry->ei_type = -1;
				D(bug(" -> file (type %d)\n", entry->ei_type));
			}
			else if (!stricmp(value, "dir"))
			{
				entry->ei_type = 1;
				D(bug(" -> directory (type %d)\n", entry->ei_type));
			}
			else if (!stricmp(value, "cdir") || !stricmp(value, "pdir"))
			{
				// MLSD cdir/pdir describe the listed directory and its parent, not children.
				skip_entry = TRUE;
				D(bug(" -> current/parent directory marker\n"));
			}
		}
		else if (!stricmp(factname, "modify"))
		{
			sscanf(value,
				   "%04hd%02hd%02hd%02hd%02hd%02hd",
				   &filedate.year,
				   &filedate.month,
				   &filedate.mday,
				   &filedate.hour,
				   &filedate.min,
				   &filedate.sec);
			entry->ei_seconds = Date2Amiga(&filedate);
			D(bug(" -> year %d month %d mday %d hour %d min %d sec %d -> seconds %u\n",
				  filedate.year,
				  filedate.month,
				  filedate.mday,
				  filedate.hour,
				  filedate.min,
				  filedate.sec,
				  entry->ei_seconds));
		}
		else if (!stricmp(factname, "UNIX.mode"))
		{
			entry->ei_unixprot = atoi(value);
			entry->ei_prot = prot_unix_to_amiga(entry->ei_unixprot);
			// entry->ei_prot = FIBF_READ|FIBF_WRITE|FIBF_EXECUTE|FIBF_DELETE;
			D(bug(" -> unix protection bits %d amiga protection bits %d\n", entry->ei_unixprot, entry->ei_prot));
		}
	}

	FreeVec(pair);

	if (skip_entry)
		return 0;

	name = facts_end + 1;
	name_len = strlen(name);
	while (name_len && (name[name_len - 1] == '\n' || name[name_len - 1] == '\r'))
		--name_len;
	name_len = min(name_len, FILENAMELEN);
	memcpy(entry->ei_name, name, name_len);
	entry->ei_name[name_len] = 0;
	D(bug(" -> filename '%s'\n", entry->ei_name));

	if (!entry->ei_name[0])
		return 0;

	// . and .. are not supported!
	if (!strcmp(entry->ei_name, ".") || !strcmp(entry->ei_name, ".."))
		return 0;

	// Hidden entry?
	if ((flags & UI_DOT_HIDDEN) && entry->ei_name[0] == '.')
		entry->ei_prot |= FIBF_HIDDEN;

	return 1;
}

/********************************/

// Taken from dopussrc:Program/misc.c
// Get final path from a pathname
void final_path(char *path, char *buf)
{
	char *ptr, *end_ptr;
	short len;

	// Empty or null string?
	if (!path || !*path)
	{
		*buf = 0;
		return;
	}

	// Get end of string
	end_ptr = path + strlen(path) - 1;

	// Go backwards
	for (ptr = end_ptr; ptr > path; --ptr)
	{
		// Directory separator?
		if (*ptr == '/')
		{
			// Last character?
			if (*(ptr + 1) == 0)
			{
				// Save new end pointer
				end_ptr = ptr - 1;
				continue;
			}

			// Break here
			break;
		}

		// Device separator?
		else if (*ptr == ':')
		{
			// If last character, go to start of string
			if (*(ptr + 1) == 0)
				ptr = path;

			// Break here
			break;
		}
	}

	// If not at the start of the string, skip forward one
	if (ptr > path)
		++ptr;

	// Copy into buffer
	len = end_ptr - ptr + 1;
	strncpy(buf, ptr, len);
	buf[len] = 0;
}

/********************************/

//
//	Find a file/dir entry in an entry list
//
struct entry_info *rec_find_entry(struct rec_entry_list *entry_list, struct entry_info *match_entry, ULONG cmpflags)
{
	struct entry_info *entry;

	if (!entry_list || !match_entry)
		return 0;

	for (entry = (struct entry_info *)entry_list->rl_list.lh_Head; entry->ei_node.ln_Succ;
		 entry = (struct entry_info *)entry->ei_node.ln_Succ)
	{
		if (!pathcmp(cmpflags, match_entry->ei_name, entry->ei_name))
			return entry;
	}

	return 0;
}

/********************************/

void build_url_flags(char *buffer,
					 const char *user,
					 const char *pass,
					 const char *host,
					 int port,
					 const char *path,
					 const char *entry,
					 ULONG flags)
{
	// Valid?
	if (!buffer || !host || !*host)
		return;

	// Start with an empty string since we use concatenation
	*buffer = 0;

	if (user && *user && stricmp(user, "anonymous") && stricmp(user, "ftp"))
	{
		strcat(buffer, user);
		if ((flags & BUILDURLF_INCLUDE_PASSWORD) && pass && *pass)
		{
			strcat(buffer, ":");
			strcat(buffer, pass);
		}
		strcat(buffer, "@");
	}
	strcat(buffer, host);
	if (port && port != 21)
	{
		strcat(buffer, ":");
		sprintf(buffer + strlen(buffer), "%ld", (long)port);
	}
	if (path && *path)
	{
		strcat(buffer, "/");
		strcat(buffer, path);
	}
	if (entry && *entry)
	{
		if (!strchr("/:", buffer[strlen(buffer) - 1]))
			strcat(buffer, "/");
		strcat(buffer, entry);
	}
}

/********************************/

void build_url(char *buffer,
			   const char *user,
			   const char *pass,
			   const char *host,
			   int port,
			   const char *path,
			   const char *entry)
{
	build_url_flags(buffer, user, pass, host, port, path, entry, BUILDURLF_INCLUDE_PASSWORD);
}

/********************************/

int split_url(const char *url, char *p_user, char *p_pass, char *p_host, LONG *p_port, char *p_path)
{
	const char *user;
	const char *pass;
	const char *host;
	const char *port;
	const char *path;

	// Valid?
	if (!url || !*url || !p_host)
		return 0;

	if ((host = strchr(url, '@')))
	{
		user = url;

		pass = strchr(user, ':');

		// Don't confuse password field for port field
		if (pass > host)
			pass = 0;

		if (pass)
		{
			if (p_user)
				stccpy(p_user, user, min(USERNAMELEN, pass - user) + 1);
			++pass;
			if (p_pass)
				stccpy(p_pass, pass, min(PASSWORDLEN, host - pass) + 1);
		}
		else if (p_user)
			stccpy(p_user, user, min(USERNAMELEN, host - user) + 1);
		++host;
	}
	else
		host = url;

	port = strchr(host, ':');
	path = strchr(host, '/');
	if (path && path < port)
		port = 0;

	if (port)
	{
		if (p_host)
			stccpy(p_host, host, min(HOSTNAMELEN, port - host) + 1);
		++port;
		if (p_port)
			*p_port = atoi(port);
	}

	if (path)
	{
		if (!port && p_host)
			stccpy(p_host, host, min(HOSTNAMELEN, path - host) + 1);
		++path;
		if (p_path)
			stccpy(p_path, path, PATHLEN + 1);
	}

	if (!port && !path && p_host)
		stccpy(p_host, host, HOSTNAMELEN + 1);

	return 1;
}

/***********************************************/

//
//	Create a desktop function file
//	This function kindly provided by Jon
//
// 	GP changed 15.4.98 to add safety for icon type etc

BOOL CreateFunctionFile(char *name, short type, char *instruction, char *icon)
{
	Cfg_Function *func;
	Cfg_Instruction *ins;
	APTR iff;
	struct DiskObject *obj;

	// Create function
	if (!(func = NewFunction(0, FTYPE_LEFT_BUTTON)))
		return 0;

	// Create instruction (you could have multiple instructions if you wanted)
	if ((ins = NewInstruction(0, type, instruction)))
		AddTail((struct List *)&func->instructions, (struct Node *)ins);

	// Open IFF file
	if (!(iff = IFFOpen(name, IFF_WRITE, ID_OPUS)))
	{
		FreeFunction(func);
		return 0;
	}

	// Save the function
	SaveFunction(iff, func);

	// Close file
	IFFClose(iff);

	// Free function
	FreeFunction(func);

	// Add our icon
	if (icon)
	{
		// is our object there?
		// if not get the default command icon
		if (!(obj = GetDiskObject(icon)))
			obj = GetDiskObject("DOpus5:Icons/Command");

		if (obj)
		{
			// Safety in case of idiots who have changed icon types
			obj->do_Type = WBPROJECT;
			PutDiskObject(name, obj);
		}
	}

	return 1;
}

/********************************/

//
//	Show a requester
//	Will add an OK button if no buttons specified
//
//	As with AsyncRequest and the system requester functions,
//	"-1 Means that one of the caller-supplied IDCMPFlags occurred.
//	The IDCMPFlag value is in the longword pointed to by IDCMP_ptr."
//	For us, this usually means enter was pressed in an empty string
//	gadget.
//
// int ftpmod_request( struct opusftp_globals *og, IPCData *ipc, Tag tag, ... )
int ftpmod_request(struct opusftp_globals *og, IPCData *ipc, struct TagItem *taglist)
{
	char buffer[1024 + 1];
	struct Window *window;
	struct TagItem /**taglist,*/ *fmttag, *tstate, *msgtags, *tagptr;
	int tagcount = 0;
	char *array[16];
	int i = 0;
	int result = 1;
	struct TagItem tags[] = {
		TAG_IGNORE,
		0,	// Default screen if needed
		TAG_IGNORE,
		0,	// Default button if needed
		TAG_IGNORE,
		0,	// String created by sprintf
		TAG_MORE,
		0,
	};

	// Called by non-owning process?
	if ((struct Task *)ipc->proc != FindTask(0))
	{
		D(bug("** ftpmod_request called with wrong IPC!\n"));
		return 0;
	}

	// Requesters suppressed?
	else if (og->og_noreq)
		return 0;

	else  // if	((taglist = CloneTagItems( (struct TagItem *)&tag )))
	{
		// Sort out default window / screen
		if (!(window = (struct Window *)GetTagData(AR_Window, 0, taglist)))
			if (!GetTagData(AR_Screen, 0, taglist))
			{
				tags[0].ti_Tag = AR_Screen;
				tags[0].ti_Data = (IPTR)og->og_screen;
			}

		// Need to add OK button?
		if (GetTagData(AR_Button, -1, taglist) == -1 && GetTagData(FR_ButtonNum, -1, taglist) == -1)
		{
			tags[1].ti_Tag = AR_Button;
			tags[1].ti_Data = (IPTR)GetString(locale, MSG_FTP_OKAY);
		}

		// Look up locale strings
		tstate = taglist;
		while ((tagptr = NextTagItem(&tstate)))
		{
			if (tagptr->ti_Tag == FR_MsgNum)
			{
				tagptr->ti_Tag = AR_Message;
				tagptr->ti_Data = (IPTR)GetString(locale, tagptr->ti_Data);
			}
			else if (tagptr->ti_Tag == FR_ButtonNum)
			{
				tagptr->ti_Tag = AR_Button;
				tagptr->ti_Data = (IPTR)GetString(locale, tagptr->ti_Data);
			}
		}

		// Do printf-style formatting?
		if ((fmttag = FindTagItem(FR_FormatString, taglist)))
		{
			tstate = fmttag;

			// Skip format string itself
			NextTagItem(&tstate);

			// Count following message tags
			if ((msgtags = NextTagItem(&tstate)))
			{
				++tagcount;
				while ((tagptr = NextTagItem(&tstate)) && (tagptr->ti_Tag == AR_Message || tagptr->ti_Tag == FR_MsgNum))
					++tagcount;

				if (tagcount)
				{
					tstate = msgtags;
					while ((tagptr = NextTagItem(&tstate)) &&
						   (tagptr->ti_Tag == AR_Message || tagptr->ti_Tag == FR_MsgNum))
					{
						if (tagptr->ti_Tag == FR_MsgNum)
							array[i++] = GetString(locale, tagptr->ti_Data);
						else
							array[i++] = (char *)tagptr->ti_Data;
					}
					// vsprintf( buffer, (char *)fmttag->ti_Data, (va_list)array );
					LSprintf(buffer, (char *)fmttag->ti_Data, array);
					tags[2].ti_Tag = AR_Message;
					tags[2].ti_Data = (IPTR)buffer;

					fmttag->ti_Tag = TAG_SKIP;
					fmttag->ti_Data = tagcount;
				}
			}
		}

		// Link user tags
		tags[3].ti_Data = (IPTR)taglist;

		result = AsyncRequest(ipc, REQTYPE_SIMPLE, window, 0, 0, tags);

		// FreeTagItems( taglist );
	}

	return result;
}

/********************************/

//
//	Check certain Opus options
//
unsigned long ftpmod_options(struct opusftp_globals *og, int type)
{
	struct pointer_packet pp;
	CFG_SETS *options;
	unsigned long rv = 0;

	pp.type = MODPTR_OPTIONS;
	pp.pointer = 0;
	pp.flags = 0;

	REFCALL(og->og_func_callback, EXTCMD_GET_POINTER, IPCDATA(og->og_main_ipc), &pp);
	//	og->og_func_callback(EXTCMD_GET_POINTER, IPCDATA(og->og_main_ipc), &pp);

	options = (CFG_SETS *)pp.pointer;

	// Copy options?
	if (type == OPTION_COPY && options)
		rv = options->copy_flags;

	// Delete options?
	else if (type == OPTION_DELETE)
		rv = options->delete_flags;

	if (pp.flags & POINTERF_LOCKED)
		REFCALL(og->og_func_callback, EXTCMD_FREE_POINTER, IPCDATA(og->og_main_ipc), &pp);
	//	og->og_func_callback(EXTCMD_FREE_POINTER,IPCDATA(og->og_main_ipc), &pp);

	return rv;
}

/********************************/

BPTR open_temp_file(char *filename, IPCData *ipc)
{
	unsigned short a;
	ULONG micros, secs;
	IPTR temp_key;
	BPTR file = NULL;

	D(bug("open_temp_file()\n"));

	// Get temporary key
	temp_key = (IPTR)ipc;
	CurrentTime(&secs, &micros);
	if (micros)
		temp_key *= micros;
	temp_key += secs;

	// Create a temporary script file; try up to 99 times
	for (a = 0; a < 99; a++)
	{
		// Build script name
		lsprintf(filename, "T:dopus-%lx-tmp", temp_key + a);

		// Try to open file
		if ((file = Open(filename, MODE_NEWFILE)))
			break;
	}

	return file;
}

/********************************/

//
//	Count the number of components in a path.
//
int count_path_parts(const char *path)
{
	const char *p = path;
	int n = 0;

	while (*p)
	{
		++n;
		while (*p)
		{
			++p;
			if (p[-1] == '/' || p[-1] == ':')
				break;
		}
	}

	return n;
}

/********************************/

// Forward an IPCMessage
int ipc_forward(IPCData *ipc, IPCMessage *msg, ULONG flags)
{
	PutMsg(ipc->command_port, (struct Message *)msg);

	return 1;
}

/********************************/

ULONG prot_amiga_to_unix(LONG bits)
{
	ULONG mode = 0777;

	if (bits & FIBF_READ)
		mode &= ~0444;
	if (bits & (FIBF_WRITE | FIBF_DELETE))
		mode &= ~0222;
	if (bits & FIBF_EXECUTE)
		mode &= ~0111;

	return mode;
}

/********************************/

LONG prot_unix_to_amiga(ULONG mode)
{
	LONG bits = 0xf;

	// we don't need the group/other permissions
	mode /= 100;

	if (mode & 4)
		bits &= ~FIBF_READ;
	if (mode & 2)
		bits &= ~(FIBF_WRITE | FIBF_DELETE);
	if (mode & 1)
		bits &= ~FIBF_EXECUTE;

	return bits;
}

/********************************/

// Taken from dopussrc:Program/misc.c
// Get protection value from a string
long prot_from_string(char *buf)
{
	short a;
	char ch;
	long protection;

	protection = FIBF_READ | FIBF_WRITE | FIBF_EXECUTE | FIBF_DELETE;
	for (a = 0; buf[a]; a++)
	{
		ch = tolower(buf[a]);
		if (ch == 'h')
			protection |= 1 << 7;
		else if (ch == 's')
			protection |= FIBF_SCRIPT;
		else if (ch == 'p')
			protection |= FIBF_PURE;
		else if (ch == 'a')
			protection |= FIBF_ARCHIVE;
		else if (ch == 'r')
			protection &= ~FIBF_READ;
		else if (ch == 'w')
			protection &= ~FIBF_WRITE;
		else if (ch == 'e')
			protection &= ~FIBF_EXECUTE;
		else if (ch == 'd')
			protection &= ~FIBF_DELETE;
	}

	return protection;
}

/********************************/
