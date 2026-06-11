/*

Directory Opus 5
Original APL release version 5.82
Copyright 1993-2012 Jonathan Potter & GP Software

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

/*
 *	18-09-95	Added "lister refresh full" after "lister set title"
 *	22-09-95	Added "lister set namelength 256" and "lister set case on" to lst_new
 *	25-09-95	Now that 'lister set busy' is synchronous, lst_lock() doesn't poll
 *	06-10-95	On very fast machines 'lister set busy' didn't work first time
 *				so the polling is back in lst_lock()
 *	12-10-95	Added lst_query_numselentries()
 *	 1-02-96	Converting progress meters to 5.1165 style
 *	 7-02-96	Now shows name of file being transferred when iconified
 *	 4-06-96	lst_set_busy now uses 'lister set busy on wait'
 *
 *	30-01-97	GJP various changes
 *			Changed mechanism of returning RC or RESULT
 *			added lst_title
 *			added lst_findcache
 *
 * 	18.4.97 	Added namelength and case on to get new buffer obtained
 *			from lister empty to support ftp parameters.
 *
 *	 3-12-97	Renamed lst_ functions to rexx_ to differentiate from cbak_
 *				Added rexx_getstring()
 *
 */

#include "ftp.h"
#include "ftp_arexx.h"
#include "ftp_lister_entry.h"
#include "ftp_opusftp.h"
#include "ftp_util.h"

/*
	#define	DEBUG
	#define LOUD 1
*/

// Debugging info from this file can be annoying
#undef DEBUG
#define LOUD 0

static char *rexx_result_string(IPTR result)
{
	if (!result || result == (IPTR)-1)
		return NULL;

	return (char *)result;
}

/*

// call "lister getstring" if we have a handle, "dopus getstring" otherwise
char *rexx_getstring(
	const char *opus,
	ULONG       handle,
	char       *text,
	BOOL        secure,
	int         length,
	char       *deflt,
	char       *buttons )
{
char *result = 0;

if	(handle)
	result = (char *)send_rexxa( opus, REXX_REPLY_RESULT,
		"lister getstring %lu '%s'%s%ld '%s' %s",
		handle,
		text,
		secure ? " secure " : " ",
		length,
		deflt,
		buttons );
else
	result = (char *)send_rexxa( opus, REXX_REPLY_RESULT,
		"dopus getstring '%s'%s%ld '%s' %s",
		text,
		secure ? " secure " : " ",
		length,
		deflt,
		buttons );

return result;
}
*/

// Add an entry to a lister which is NOT under our control

IPTR rexx_lst_add(const char *opus,
				  IPTR handle,
				  char *name,
				  unsigned int size,
				  int type,
				  ULONG seconds,
				  LONG prot,
				  char *comment)
{
	char protbuf[9];

	// Convert protection bits to ascii
	protbuf[0] = prot & FIBF_HIDDEN ? 'h' : '-';
	protbuf[1] = prot & FIBF_SCRIPT ? 's' : '-';
	protbuf[2] = prot & FIBF_PURE ? 'p' : '-';
	protbuf[3] = prot & FIBF_ARCHIVE ? 'a' : '-';
	protbuf[4] = prot & FIBF_READ ? '-' : 'r';
	protbuf[5] = prot & FIBF_WRITE ? '-' : 'w';
	protbuf[6] = prot & FIBF_EXECUTE ? '-' : 'e';
	protbuf[7] = prot & FIBF_DELETE ? '-' : 'd';
	protbuf[8] = 0;

	return send_rexxa(opus,
					  REXX_REPLY_RC,
					  "lister add " FTP_HANDLE_PRINTF " \"%s\" %lu %ld %lu %s%s%s",
					  FTP_HANDLE_VALUE(handle),
					  name,
					  (unsigned long)size,
					  (long)type,
					  (unsigned long)seconds,
					  protbuf,
					  comment ? " " : "",
					  comment ? comment : "");
}

/********************************/

void rexx_doubleclick(const char *opus, const char *path)
{
	send_rexxa(opus, REXX_REPLY_NONE, "command doubleclick %s", path);
}

/********************************/

//
//	Refresh a lister
//
void rexx_lst_refresh(const char *opus, IPTR handle, int date)
{
	D(bug("Refresh lister - start - "));

	send_rexxa(opus, FALSE, "lister refresh " FTP_HANDLE_PRINTF, FTP_HANDLE_VALUE(handle));
	if (date == REFRESH_DATE)
		send_rexxa(opus, FALSE, "lister refresh " FTP_HANDLE_PRINTF " date", FTP_HANDLE_VALUE(handle));

	D(bug("end\n"));
}

/********************************/

/*
 *	Set or clear a lister's busy state
 */

void rexx_lst_busy(const char *opus, IPTR handle, int val)
{
	/*
	if	(val)
		D(bug( "rexx_lst_busy()\n" ));
	else
		D(bug( "rexx_lst_unlock()\n" ));
	*/

	send_rexxa(opus, REXX_REPLY_NONE, "lister set " FTP_HANDLE_PRINTF " busy %d wait", FTP_HANDLE_VALUE(handle), val);
}

/********************************/

/*
 *	Tell Opus to cache the current buffer and moves to a new one
 *	18.4.97 Added namelength and case on to et new buffer obtained
 *	from lister empty to support ftp parameters.
 *
 */

void rexx_lst_empty(const char *opus, IPTR handle)
{
	D(bug("rexx_lst_empty(%ld)\n", handle));

	send_rexxa(opus, REXX_REPLY_NONE, "lister empty " FTP_HANDLE_PRINTF, FTP_HANDLE_VALUE(handle));
	send_rexxa(opus, REXX_REPLY_NONE, "lister set " FTP_HANDLE_PRINTF " namelength 256", FTP_HANDLE_VALUE(handle));
	send_rexxa(opus, REXX_REPLY_NONE, "lister set " FTP_HANDLE_PRINTF " case on", FTP_HANDLE_VALUE(handle));
}

/********************************/

/*
 *	Opus clears the current buffer, does not move to a new one
 */

void rexx_lst_clear(const char *opus, IPTR handle)
{
	D(bug("rexx_lst_clear(%ld)\n", handle));

	send_rexxa(opus, REXX_REPLY_NONE, "lister clear " FTP_HANDLE_PRINTF, FTP_HANDLE_VALUE(handle));
}

/********************************/

/*
 *	Close a lister and clear cache buffers
 */

void rexx_lst_close(const char *opus, IPTR handle)
{
	// new command : lister freecaches <handle> <handler>
	// eg, lister freecaches 129384849 _OPUS_FTP_
	send_rexxa(opus, REXX_REPLY_NONE, "lister freecaches " FTP_HANDLE_PRINTF " " PORTNAME "", FTP_HANDLE_VALUE(handle));

	// Remove handler so we don't get 'inactive' msg
	send_rexxa(opus, REXX_REPLY_NONE, "lister set " FTP_HANDLE_PRINTF " handler ''", FTP_HANDLE_VALUE(handle));

	// Close lister
	send_rexxa(opus, REXX_REPLY_NONE, "lister close " FTP_HANDLE_PRINTF, FTP_HANDLE_VALUE(handle));
}

/*
 *	check if dir is in Opus cache. If so then Opus it will display it
 *
 *	returns 1/0 for entry cached or not
 */
int rexx_lst_findcache(const char *opus, IPTR handle, char *path)
{
	char *string;
	int value = 0;

	if ((string = rexx_result_string(
			 send_rexxa(opus, REXX_REPLY_RESULT, "lister findcache " FTP_HANDLE_PRINTF " %s", FTP_HANDLE_VALUE(handle), path))))
	{
		value = atoi(string);
		DeleteArgstring(string);
	}

	return value;
}

// Remember to do a refresh after this
void rexx_lst_title(const char *opus, IPTR handle, char *title)
{
	if (strncmp(title, "FTP:", 4) && strncmp(title, "SFTP:", 5))
		send_rexxa(opus, REXX_REPLY_NONE, "lister set " FTP_HANDLE_PRINTF " title FTP:%s", FTP_HANDLE_VALUE(handle), title);
	else
		send_rexxa(opus, REXX_REPLY_NONE, "lister set " FTP_HANDLE_PRINTF " title %s", FTP_HANDLE_VALUE(handle), title);
}

// Remember to do a refresh after this
char *rexx_lst_title_swap(const char *opus, IPTR handle, char *title)
{
	if (strncmp(title, "FTP:", 4) && strncmp(title, "SFTP:", 5))
		return rexx_result_string(send_rexxa(
			opus, REXX_REPLY_RESULT, "lister set " FTP_HANDLE_PRINTF " title FTP:%s", FTP_HANDLE_VALUE(handle), title));
	else
		return rexx_result_string(
			send_rexxa(opus, REXX_REPLY_RESULT, "lister set " FTP_HANDLE_PRINTF " title %s", FTP_HANDLE_VALUE(handle), title));
}

/********************************/

//
//	Set the label that the lister will have while iconified
//
void rexx_lst_label(const char *opus, IPTR handle, char *pref, char *label, char *suff)
{
	if (!pref)
		pref = "";
	if (!suff)
		suff = "";

	if (label)
		send_rexxa(opus,
				   REXX_REPLY_NONE,
				   "lister set " FTP_HANDLE_PRINTF " label (%s%s%s)",
				   FTP_HANDLE_VALUE(handle),
				   pref,
				   label,
				   suff);
	else
		send_rexxa(opus, REXX_REPLY_NONE, "lister set " FTP_HANDLE_PRINTF " label", FTP_HANDLE_VALUE(handle));
}

/********************************/

//
//	Ensure a lister becomes busy
//
void rexx_lst_lock(const char *opus, IPTR handle)
{
	int tries = 20;
	char *r;

	D(bug("rexx_lst_lock()\n"));

	if (!opus || !*opus || !handle)
		return;

	while (tries--)
	{
		IPTR result;

		rexx_lst_busy(opus, handle, 1);

		result = send_rexxa(opus, REXX_REPLY_RESULT, "lister query " FTP_HANDLE_PRINTF " busy", FTP_HANDLE_VALUE(handle));
		if (result == (IPTR)-1)
			return;

		if ((r = rexx_result_string(result)))
		{
			if (*r == '1')
			{
				DeleteArgstring(r);
				return;
			}

			DeleteArgstring(r);
		}

		if (tries)
			Delay(5);
	}
}

/********************************/

//
//	Create a new lister and set it's title and handler
//
IPTR rexx_lst_new(const char *opus, IPTR handle, char *host, const char *toolbar)
{
	char *asciihandle;
	IPTR new_handle = 0;
	BPTR dir_lock = 0, cd = 0, toolbar_lock = 0;

	if (toolbar && *toolbar && (dir_lock = Lock("DOpus5:Buttons/", ACCESS_READ)))
	{
		cd = CurrentDir(dir_lock);
		toolbar_lock = Lock((char *)toolbar, ACCESS_READ);
	}

	if (handle)
	{
		new_handle = handle;

		//	TODO DeviceList causes problems when the list is not cleared
		//	rexx_lst_clear( opus, new_handle )

		// if there is special toolbar then set it.
		if (toolbar_lock)
			send_rexxa(opus,
					   REXX_REPLY_NONE,
					   "lister set " FTP_HANDLE_PRINTF " toolbar %s",
					   FTP_HANDLE_VALUE(new_handle),
					   toolbar);
	}
	else
	{
		if (toolbar_lock)
			asciihandle = rexx_result_string(send_rexxa(opus, REXX_REPLY_RESULT, "lister new toolbar %s", toolbar));
		else
			asciihandle = rexx_result_string(send_rexxa(opus, REXX_REPLY_RESULT, "lister new"));
		if (asciihandle)
		{
			new_handle = ftp_parse_handle(asciihandle);
			DeleteArgstring(asciihandle);
		}
	}

	if (new_handle)
	{
		send_rexxa(opus,
				   REXX_REPLY_NONE,
				   "lister set " FTP_HANDLE_PRINTF " handler " PORTNAME " quotes editing subdrop synctraps",
				   FTP_HANDLE_VALUE(new_handle));
		send_rexxa(opus, REXX_REPLY_NONE, "lister set " FTP_HANDLE_PRINTF " mode name", FTP_HANDLE_VALUE(new_handle));

#if 0
	send_rexxa( opus, REXX_REPLY_NONE, "lister set %lu title FTP:%s", new_handle, host );
	send_rexxa( opus, REXX_REPLY_NONE, "lister set %lu namelength 256", new_handle );
	send_rexxa( opus, REXX_REPLY_NONE, "lister set %lu case on", new_handle );
	send_rexxa( opus, REXX_REPLY_NONE, "lister refresh %lu full", new_handle );
	rexx_lst_label( opus, new_handle, "FTP:", host, NULL );
#endif

		send_rexxa(opus, REXX_REPLY_NONE, "lister wait " FTP_HANDLE_PRINTF " quick", FTP_HANDLE_VALUE(new_handle));
	}

	if (toolbar_lock)
		UnLock(toolbar_lock);

	if (dir_lock)
	{
		CurrentDir(cd);
		UnLock(dir_lock);
	}

	return new_handle;
}

/*********************************/

BOOL rexx_lst_query_handler(const char *opus, IPTR handle)
{
	char *handler;
	BOOL result = FALSE;

	if ((handler = rexx_result_string(
			 send_rexxa(opus, REXX_REPLY_RESULT, "lister query " FTP_HANDLE_PRINTF " handler", FTP_HANDLE_VALUE(handle)))))
	{
		if (*handler)
			result = TRUE;

		DeleteArgstring(handler);
	}

	return result;
}

/*********************************/

//
//	Is lister visible?
//	Still there after an inactive msg?
//
int rexx_lst_query_visible(const char *opus, IPTR handle)
{
	char *string;
	int value = 0;

	if ((string = rexx_result_string(
			 send_rexxa(opus, REXX_REPLY_RESULT, "lister query " FTP_HANDLE_PRINTF " visible", FTP_HANDLE_VALUE(handle)))))
	{
		D(bug("** query visible '%s'\n", string));
		value = atoi(string);
		DeleteArgstring(string);
	}

	return value;
}

/********************************/

/*
 *	Returns 1st dest handle
 */

IPTR rexx_lst_query_dest1(const char *opus)
{
	char *dst;
	IPTR result = 0;

	if ((dst = rexx_result_string(send_rexx(opus, REXX_REPLY_RESULT, "lister query dest"))))
	{
		result = ftp_parse_handle(dst);
		DeleteArgstring(dst);
	}

	return result;
}

/********************************/

/*
 *	Get the fileinfo string for a list entry
 */

char *rexx_lst_query_entry(const char *opus, IPTR handle, char *entry)
{
	char quoted[FTP_LISTER_QUOTED_NAME_BUFSIZE];

	if (!ftp_lister_quote_entry_name(entry, quoted, sizeof(quoted)))
		return NULL;

	return rexx_result_string(send_rexxa(
		opus, REXX_REPLY_RESULT, "lister query " FTP_HANDLE_PRINTF " entry %s", FTP_HANDLE_VALUE(handle), quoted));
}

/********************************/

static int rexx_lst_query_numblah(const char *opus, IPTR handle, const char *blah)
{
	char *s;
	int n = 0;

	if ((s = rexx_result_string(send_rexxa(
			 opus, REXX_REPLY_RESULT, "lister query " FTP_HANDLE_PRINTF " num%s", FTP_HANDLE_VALUE(handle), blah))))
	{
		n = atoi(s);
		DeleteArgstring(s);
	}

	return n;
}

/********************************/

int rexx_lst_query_numentries(const char *opus, IPTR handle)
{
	return rexx_lst_query_numblah(opus, handle, "entries");
}

/********************************/

int rexx_lst_query_numdirs(const char *opus, IPTR handle)
{
	return rexx_lst_query_numblah(opus, handle, "dirs");
}

/********************************/

int rexx_lst_query_numfiles(const char *opus, IPTR handle)
{
	return rexx_lst_query_numblah(opus, handle, "files");
}

/********************************/

int rexx_lst_query_numseldirs(const char *opus, IPTR handle)
{
	return rexx_lst_query_numblah(opus, handle, "seldirs");
}

/********************************/

int rexx_lst_query_numselentries(const char *opus, IPTR handle)
{
	return rexx_lst_query_numblah(opus, handle, "selentries");
}

/********************************/

int rexx_lst_query_numselfiles(const char *opus, IPTR handle)
{
	return rexx_lst_query_numblah(opus, handle, "selfiles");
}

/********************************/

char *rexx_lst_query_path(const char *opus, IPTR handle)
{
	char *path;

	D(bug("rexx_lst_query_path(%ld)\n", handle));

	path = rexx_result_string(
		send_rexxa(opus, REXX_REPLY_RESULT, "lister query " FTP_HANDLE_PRINTF " path", FTP_HANDLE_VALUE(handle)));

	D(bug("-> '%s'\n", path));

	return path;
}

/********************************/

//
//	Returns 1st source handle
//
IPTR rexx_lst_query_src1(const char *opus)
{
	char *src;
	IPTR result = 0;

	if ((src = rexx_result_string(send_rexx(opus, REXX_REPLY_RESULT, "lister query source"))))
	{
		result = ftp_parse_handle(src);
		DeleteArgstring(src);
	}

	return result;
}

/********************************/

void rexx_lst_remove(const char *opus, IPTR handle, char *name)
{
	send_rexxa(opus, REXX_REPLY_NONE, "lister remove " FTP_HANDLE_PRINTF " \"%s\"", FTP_HANDLE_VALUE(handle), name);
}

/********************************/

/*
 *	Select or deselect an entry in a lister
 */

void rexx_lst_select(const char *opus, IPTR handle, char *name, int state)
{
	send_rexxa(
		opus, REXX_REPLY_NONE, "lister select " FTP_HANDLE_PRINTF " \"%s\" %d", FTP_HANDLE_VALUE(handle), name, state);
}

/********************************/

void rexx_lst_set_path(const char *opus, IPTR handle, char *path)
{
	D(bug("rexx_lst_set_path(%ld)\n", handle));
	D(bug("-> '%s'\n", path));

	send_rexxa(opus, REXX_REPLY_NONE, "lister set " FTP_HANDLE_PRINTF " path %s", FTP_HANDLE_VALUE(handle), path);

	// DeleteArgstring( rexx_lst_query_path( opus, handle ) );
}

/********************************/

void rexx_prog_bar(const char *opus, IPTR handle, int type, int total, int count)
{
	if (type == PROGRESS_FREE)
		send_rexxa(
			opus, REXX_REPLY_NONE, "dopus progress " FTP_HANDLE_PRINTF " bar %ld %ld", FTP_HANDLE_VALUE(handle), total, count);
	else
		send_rexxa(opus,
				   REXX_REPLY_NONE,
				   "lister set " FTP_HANDLE_PRINTF " newprogress bar %ld %ld",
				   FTP_HANDLE_VALUE(handle),
				   total,
				   count);
}

/********************************/

void rexx_prog_bytes(const char *opus, IPTR handle, int type, int total, int count)
{
	if (type == PROGRESS_FREE)
		send_rexxa(opus,
				   REXX_REPLY_NONE,
				   "dopus progress " FTP_HANDLE_PRINTF " file %ld %ld",
				   FTP_HANDLE_VALUE(handle),
				   total,
				   count);
	else
		send_rexxa(opus,
				   REXX_REPLY_NONE,
				   "lister set " FTP_HANDLE_PRINTF " newprogress file %ld %ld",
				   FTP_HANDLE_VALUE(handle),
				   total,
				   count);
}

/********************************/

void rexx_prog_clear(const char *opus, IPTR handle, int type)
{
	if (type == PROGRESS_FREE)
		send_rexxa(opus, REXX_REPLY_NONE, "dopus progress " FTP_HANDLE_PRINTF " off", FTP_HANDLE_VALUE(handle));
	else
		send_rexxa(opus, REXX_REPLY_NONE, "lister clear " FTP_HANDLE_PRINTF " progress", FTP_HANDLE_VALUE(handle));
}

/********************************/

//
//	Initialize a progress bar.
//	If no handle is passed in, a free-floating progress bar will be created.
//	Returns the handle passed in or the handle of the free-floating bar.
//
IPTR rexx_prog_init(const char *opus, IPTR handle, int type, char *title, char *info, char *name, int file, int bar)
{
	char buf[80];
	char *str;

	if (type == PROGRESS_FREE)
		sprintf(buf, "dopus progress abort");
	else
		sprintf(buf, "lister set " FTP_HANDLE_PRINTF " newprogress abort", FTP_HANDLE_VALUE(handle));

	if (info)
		strcat(buf, " info");

	if (name)
		strcat(buf, " name");
	if (file)
		strcat(buf, " file");
	if (bar)
		strcat(buf, " bar");

	if (type == PROGRESS_FREE)
	{
		if ((str = rexx_result_string(send_rexx(opus, REXX_REPLY_RESULT, buf))))
		{
			handle = ftp_parse_handle(str);
			DeleteArgstring(str);
		}
	}
	else
		send_rexx(opus, REXX_REPLY_NONE, buf);

	if (handle)
	{
		if (title && *title)
		{
			if (type == PROGRESS_FREE)
				send_rexxa(opus,
						   REXX_REPLY_NONE,
						   "dopus progress " FTP_HANDLE_PRINTF " title %s",
						   FTP_HANDLE_VALUE(handle),
						   title);
			else
				send_rexxa(opus,
						   REXX_REPLY_NONE,
						   "lister set " FTP_HANDLE_PRINTF " newprogress title %s",
						   FTP_HANDLE_VALUE(handle),
						   title);
		}
		if (info && *info)
		{
			if (type == PROGRESS_FREE)
				send_rexxa(
					opus, REXX_REPLY_NONE, "dopus progress " FTP_HANDLE_PRINTF " info %s", FTP_HANDLE_VALUE(handle), info);
			else
				send_rexxa(opus,
						   REXX_REPLY_NONE,
						   "lister set " FTP_HANDLE_PRINTF " newprogress info %s",
						   FTP_HANDLE_VALUE(handle),
						   info);
		}
		if (name && *name)
		{
			if (type == PROGRESS_FREE)
				send_rexxa(
					opus, REXX_REPLY_NONE, "dopus progress " FTP_HANDLE_PRINTF " name %s", FTP_HANDLE_VALUE(handle), name);
			else
				send_rexxa(opus,
						   REXX_REPLY_NONE,
						   "lister set " FTP_HANDLE_PRINTF " newprogress name %s",
						   FTP_HANDLE_VALUE(handle),
						   name);
		}
	}

	return handle;
}

/********************************/

//
//	Initialize a progress bar.
//	If no handle is passed in, a free-floating progress bar will be created.
//	Returns the handle passed in or the handle of the free-floating bar.
//
IPTR rexx_prog_init3(const char *opus,
					  IPTR handle,
					  int type,
					  char *title,
					  char *info,

					  char *info2,
					  char *info3,

					  char *name,
					  int file,
					  int bar)
{
	char buf[80];
	char *str;

	if (type == PROGRESS_FREE)
		sprintf(buf, "dopus progress abort");
	else
		sprintf(buf, "lister set " FTP_HANDLE_PRINTF " newprogress abort", FTP_HANDLE_VALUE(handle));

	if (info)
		strcat(buf, " info");
	if (info2)
		strcat(buf, " info2");
	if (info3)
		strcat(buf, " info3");
	if (name)
		strcat(buf, " name");
	if (file)
		strcat(buf, " file");
	if (bar)
		strcat(buf, " bar");

	if (type == PROGRESS_FREE)
	{
		if ((str = rexx_result_string(send_rexx(opus, REXX_REPLY_RESULT, buf))))
		{
			handle = ftp_parse_handle(str);
			DeleteArgstring(str);
		}
	}
	else
		send_rexx(opus, REXX_REPLY_NONE, buf);

	if (handle)
	{
		if (title && *title)
		{
			if (type == PROGRESS_FREE)
				send_rexxa(opus,
						   REXX_REPLY_NONE,
						   "dopus progress " FTP_HANDLE_PRINTF " title %s",
						   FTP_HANDLE_VALUE(handle),
						   title);
			else
				send_rexxa(opus,
						   REXX_REPLY_NONE,
						   "lister set " FTP_HANDLE_PRINTF " newprogress title %s",
						   FTP_HANDLE_VALUE(handle),
						   title);
		}
		if (info && *info)
		{
			if (type == PROGRESS_FREE)
				send_rexxa(
					opus,
					REXX_REPLY_NONE,
					"dopus progress " FTP_HANDLE_PRINTF " info %s info2 %s info3 %s ",
					FTP_HANDLE_VALUE(handle),
					info,
					info2,
					info3);
			else
				send_rexxa(opus,
						   REXX_REPLY_NONE,
						   "lister set " FTP_HANDLE_PRINTF " newprogress info %s info2 %s info3 %s ",
						   FTP_HANDLE_VALUE(handle),
						   info,
						   info2,
						   info3);
		}
		if (name && *name)
		{
			if (type == PROGRESS_FREE)
				send_rexxa(
					opus, REXX_REPLY_NONE, "dopus progress " FTP_HANDLE_PRINTF " name %s", FTP_HANDLE_VALUE(handle), name);
			else
				send_rexxa(opus,
						   REXX_REPLY_NONE,
						   "lister set " FTP_HANDLE_PRINTF " newprogress name %s",
						   FTP_HANDLE_VALUE(handle),
						   name);
		}
	}

	return handle;
}

/********************************/

void rexx_prog_name(const char *opus, IPTR handle, int type, char *name)
{
	if (type == PROGRESS_FREE)
		send_rexxa(opus, REXX_REPLY_NONE, "dopus progress " FTP_HANDLE_PRINTF " name %s", FTP_HANDLE_VALUE(handle), name);
	else
		send_rexxa(
			opus, REXX_REPLY_NONE, "lister set " FTP_HANDLE_PRINTF " newprogress name %s", FTP_HANDLE_VALUE(handle), name);
}

/********************************/

void rexx_prog_info(const char *opus, IPTR handle, int type, char *info)
{
	if (type == PROGRESS_FREE)
		send_rexxa(opus, REXX_REPLY_NONE, "dopus progress " FTP_HANDLE_PRINTF " info %s", FTP_HANDLE_VALUE(handle), info);
	else
		send_rexxa(
			opus, REXX_REPLY_NONE, "lister set " FTP_HANDLE_PRINTF " newprogress info %s", FTP_HANDLE_VALUE(handle), info);
}

void rexx_prog_info2(const char *opus, IPTR handle, int type, char *info)
{
	if (type == PROGRESS_FREE)
		send_rexxa(opus, REXX_REPLY_NONE, "dopus progress " FTP_HANDLE_PRINTF " info2 %s", FTP_HANDLE_VALUE(handle), info);
	else
		send_rexxa(
			opus, REXX_REPLY_NONE, "lister set " FTP_HANDLE_PRINTF " newprogress info2 %s", FTP_HANDLE_VALUE(handle), info);
}

void rexx_prog_info3(const char *opus, IPTR handle, int type, char *info)
{
	if (type == PROGRESS_FREE)
		send_rexxa(opus, REXX_REPLY_NONE, "dopus progress " FTP_HANDLE_PRINTF " info %s", FTP_HANDLE_VALUE(handle), info);
	else
		send_rexxa(
			opus, REXX_REPLY_NONE, "lister set " FTP_HANDLE_PRINTF " newprogress info3 %s", FTP_HANDLE_VALUE(handle), info);
}

/********************************/

/*
 *	Send an Arexx command, get an optional result back
 */

IPTR STDARGS send_rexxa(const char *dest_portname, int reply_type, const char *fmt, ...)
{
	va_list ap;
	IPTR retval = 0;
	char *buf;

	if ((buf = AllocVec(1024, MEMF_ANY)))
	{
		va_start(ap, fmt);
		vsprintf(buf, fmt, ap);
		va_end(ap);

		retval = send_rexx(dest_portname, reply_type, buf);
		FreeVec(buf);
	}

	return retval;
}

/********************************/

//
//	Send a command to Dopus
//	 Will return a LONG from result1 for numeric results
//	 or a char* from result2 for string results
//	 or -1 if it couldn't even be sent
//
//	gjp changed to make return RC/RESULT easier by using
//	reply_type is emum REXX_REPLY_NONE,REXX_REPLY_RESULT,REXX_REPLY_RC;
//
IPTR send_rexx(const char *dest_portname, int reply_type, const char *cmd)
{
	IPTR retval = -1;				// The result
	struct MsgPort *destport, *rp;	// The destination and reply ports
	UBYTE *argstr;					// The arexx argument string
	struct RexxMsg *rxmsg;			// The arexx message

#ifdef DEBUG
	BOOL quiet = FALSE;
	BOOL show_debug = FALSE;
#endif

	if ((rp = CreateMsgPort()))
	{
		if ((rxmsg = CreateRexxMsg(rp, NULL, NULL)))
		{
#ifdef DEBUG
			quiet = !(strnicmp(cmd, "lister add", 10) && strnicmp(cmd, "lister refresh", 13) &&
					  (!strstr(cmd, "progress count")) && (!strstr(cmd, "newprogress file")) &&
					  (!strstr(cmd, "abort")) /*&& (!strstr(cmd,"busy"))*/
					  && (!strstr(cmd, "addtrap")));

			if (LOUD || !quiet)
			{
				show_debug = TRUE;
				D(bug("%s: '%s'\n", dest_portname, cmd));
			}
#endif
			if ((argstr = (UBYTE *)CreateArgstring((char *)cmd, strlen(cmd))))
			{
				// We only send commands
				rxmsg->rm_Action = RXCOMM;

				if (reply_type == REXX_REPLY_RESULT)
				{
					// Do we need a reply?
					rxmsg->rm_Action |= RXFF_RESULT;
				}

#ifdef __AROS__
				rxmsg->rm_Args[0] = (IPTR)argstr;
#else
				rxmsg->rm_Args[0] = (char *)argstr;
#endif

				Forbid();
				if ((destport = FindPort((char *)dest_portname)))
					PutMsg(destport, (struct Message *)rxmsg);
				Permit();
				if (destport)
				{
					WaitPort(rp);
					GetMsg(rp);
					retval = reply_type == REXX_REPLY_RC ? rxmsg->rm_Result1 : rxmsg->rm_Result2;
#ifdef DEBUG
					if (reply_type != REXX_REPLY_NONE && show_debug)
					{
						D(bug("RC==> 1: %ld  ", rxmsg->rm_Result1));
						if (reply_type == REXX_REPLY_RESULT)
							D(bug("RES==> 2: '%s'\n", retval));
						D(bug("\n"));
					}
#endif
				}

				DeleteArgstring((char *)argstr);
			}
			DeleteRexxMsg(rxmsg);
		}
		DeleteMsgPort(rp);
	}

	return retval;
}

/********************************/

//
//	Reply to an arexx message
//
void reply_rexx(struct RexxMsg *msg, LONG r1, IPTR r2)
{
	if (msg)
	{
		msg->rm_Result1 = r1;

		if (msg->rm_Action & RXFF_RESULT)
			msg->rm_Result2 = r2;

		ReplyMsg((struct Message *)msg);
	}
}

/********************************/

//
//	Flush ARexx port
//
void flush_arexxport(struct MsgPort *port)
{
	struct RexxMsg *msg;

	if (port)
	{
		while ((msg = (struct RexxMsg *)GetMsg(port)))
			reply_rexx(msg, 10, 0);
	}
}
