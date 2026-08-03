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

/**
 **	This file should contain any high level functions which
 **	interact directly with an Opus lister and call low level
 **	FTP functions.  File transfer functions should be in
 **	ftp_lister_xfer.c  Connect functions should be in
 **	ftp_lister_connect.c
 **
 **	lister_cwd
 **	lister_cdup
 **	lister_makedir
 **	lister_rename
 **	lister_delete
 **
 **	lister_reread
 **	lister_parent
 **	lister_root
 **	lister_path
 **	lister_cmd
 **	lister_setvar
 **/

/*
 *	15-08-95	Uses FilePart() to construct the local filename
 *				so that CopyReq should work with a full path - should use OS aware routine
 *	17-08-95	Dir cache was invalid if list was aborted
 *	 3-09-95	Directory listing has now sped up to the speed of the command-line ftp
 *				lister display is now updated every second instead of every file
 *	 3-09-95	File transfer requester now swaps between file name and number of bytes
 *				transferred/total bytes every 2 seconds
 *	 6-09-95	When retrieving a file of unknown size, the progress indicator is opened with
 *				size -2 to inhibit bar graph but retain filename/bytes display
 *	 7-09-95	Add 'CopyAs' support in both directions
 *	 8-09-95	'Root' command now works properly for AmigaDOS
 *	14-09-95	Adjusted sockerr() for locale support
 *	22-09-95	MakeDir now supports a 'NAME' argument (not using ReadArgs)
 *	22-09-95	Unix symbolic link entries are now added with type -2 if they don't look like dirs
 *	24-09-95	Added LISTUPDATE config option & variable
 *	25-09-95	Unix symbolic link entries are now added with type 2 if they do look like dirs
 *	25-09-95	Copy & Rename now implemented as IPC - arexx removed
 *	26-09-95	Fixed CPS rate display
 *	 2-10-95	'Replace' and 'Skip' were exchanged in the Resume requester
 *	 2-10-95	Transfer rate numbers are now displayed in Megs, K's, or bytes
 *	 2-10-95	Getting a Unix soft-link would add the file to the destination as an assign
 *	11-10-95	Unix symbolic link entries are now added with type -3 & 3, same as Amiga links
 *	12-10-95	Replace requester only asks 'all files?' when more than one remains
 *	12-10-95	Replace requester no longer assumes 'skip all' from the standard replace requester
 *				(the one without the 'resume' option) means skip resumable files too
 *	12-10-95	Removed the '->' symbol from Unix symbolic links (no longer needed)
 *	13-10-95	Now recognizes 'D' as well as 'd' in the list ouput as a directory
 *	19-10-95	Cleaned up code in sockerr()
 *	28-01-96	Added error 13 - 'Permission denied'
 *	 1-02-96	Converting progress meters to 5.1165 style
 *	 7-02-96	Now shows name of file being transferred when iconified
 *	 8-02-96	Password is now shown as asterisks for security
 *			Now accepts 'ftp' as synonym for 'anonymous'
 *	21-02-96	Now works under AS225 as well as AmiTCP
 *	20-03-96	Added support for asynchronous LIST processing
 *	 1-04-96	Downloaded files now preserve their datestamp
 *	 9-05-96	Displays server error messages in requesters
 *	15-05-96	If the initial path cannot be CWDd to, its parent is tried
 *	20-05-96	Split this file into lister.c lister_connect.c lister_list.c and lister_xfer.c
 *	28-05-96	Trapped Delete command now supports correct template and QUIET option
 *	14-06-96	FTP listers now remember which entries were highlighted in its cache
 *	20-06-96	Replaced calls to SimpleRequest() with calls to AsyncRequest() because of
 *				mysterious crashing bug
 *	12.02.97	AsyncReq calls moved to display_message fn in ftp_util.c
 *	19.02.97	Added NOOP calls via timer to stop remote site timing out
 *
 *	11.04.97	Some complaints about limit of 40 chars for file len so
 *			have increased it to the max 108 (Alst_Add()) Problimatic
 *
 *	 3-12-97	Renamed Alst_ functions to cbak_ to match rexx_ functions
 *
 */

#include "ftp.h"
#include "ftp_arexx.h"
#include "ftp_ipc.h"
#include "ftp_lister.h"
#include "ftp_lister_entry.h"
#include "ftp_opusftp.h"
#include "ftp_recursive.h"
#include "ftp_util.h"
#include "ftp_utf8.h"
#include "ftp_addressbook.h"
#include "ftp_addrsupp_protos.h"
#include "ftp_protect.h"

/********************************/

//
//	Output to log file - this can be specifically used by ftp.c
//	 ** vsprintf() does not check the size of the buffer! **
//
void STDARGS logprintf(char *fmt, ...)
{
	va_list ap;
	char *buf;
	IPCData *master_ipc = GETGLOBAL(FindTask(NULL), g_master_ipc);
	int ok = 0;

	if (master_ipc && strlen(fmt) < 256)
	{
		if ((buf = AllocVec(256, MEMF_CLEAR)))
		{
			va_start(ap, fmt);
			vsprintf(buf, fmt, ap);
			va_end(ap);

			IPC_Command(master_ipc, IPC_PRINT, 0, 0, buf, 0);
			ok = 1;
		}
	}
}

/********************************/
//
//	Add entry to a lister using Opus callback hook
//
void lister_add(struct ftp_node *node, char *name, int size, int type, ULONG seconds, LONG prot, char *comment)
{
	static D_S(struct FileInfoBlock, fib) APTR entry;
	LONG etype;
	struct TagItem tags[2] = {{HFFS_NAME, 0}, {TAG_DONE}};
	DOpusCallbackInfo *infoptr;
	unsigned int entry_size;
	int entry_type = type;

	// Valid?
	if (!node || !name)
		return;

	infoptr = &node->fn_og->og_hooks;

	switch (type)
	{
	case 1:
		etype = ST_USERDIR;
		break;
	case -1:
		etype = ST_FILE;
		break;
	case 3:
		etype = ST_LINKDIR;
		break;
	case -3:
		etype = ST_LINKFILE;
		break;
	default:
		D(bug("** lst_add() bad type %d\n", type));
		entry_type = -1;
		etype = ST_FILE;
		break;
	}

	entry_size = (entry_type > 0) ? 0 : (unsigned int)size;

	fib->fib_DirEntryType = etype;

	stccpy(fib->fib_FileName, name, sizeof(fib->fib_FileName));

	if (comment)
		stccpy(fib->fib_Comment, comment, COMMENTLEN);
	else
		*fib->fib_Comment = 0;

	fib->fib_Size = entry_size;

	seconds_to_datestamp(&fib->fib_Date, seconds);

	fib->fib_Protection = prot;

	if ((entry = (APTR)DC_CALL3(
			 infoptr, dc_CreateFileEntry, DC_REGA0, (APTR)node->fn_handle, DC_REGA1, fib, DC_REGD0, (BPTR)0)))
	// if	((entry = node->fn_og->og_hooks.dc_CreateFileEntry( (ULONG)node->fn_handle, fib, NULL )))
	{
		DC_CALL3(infoptr, dc_AddFileEntry, DC_REGA0, (APTR)node->fn_handle, DC_REGA1, entry, DC_REGD0, TRUE);
		// node->fn_og->og_hooks.dc_AddFileEntry( (ULONG)node->fn_handle, entry, TRUE );

		tags[0].ti_Data = (IPTR)name;

		DC_CALL3(infoptr, dc_FileSet, DC_REGA0, (APTR)node->fn_handle, DC_REGA1, entry, DC_REGA2, tags);
		// node->fn_og->og_hooks.dc_FileSet( (ULONG)node->fn_handle, entry, tags );
	}

	if (node->fn_site.se_env->e_index_enable)
	{
		if (!strcmp(name, NAME_INDEX))
		{
			node->fn_ftp.fi_found_index |= INDEX_INDEX;
			node->fn_ftp.fi_found_index_size = size;
		}

		if (!strcmp(name, NAME_FBBS))
		{
			node->fn_ftp.fi_found_index |= INDEX_FILES;
			node->fn_ftp.fi_found_fbbs_size = size;
		}
	}
}

/********************************/

//
//	Refresh a lister via Opus callback hook or ARexx
//
void ftplister_refresh(struct ftp_node *node, int date)
{
	// DOpusCallbackInfo *infoptr = &node->fn_og->og_hooks;
	// Valid
	if (!node)
	{
		D(bug("** ftplister_refresh invalid args!\n"));
		return;
	}

	/*DC_CALL2(infoptr, dc_RefreshLister,
		DC_REGA0, node->fn_handle,
		DC_REGD0, date);*/
	// node->fn_og->og_hooks.dc_RefreshLister( node->fn_handle, date );

	rexx_lst_refresh(node->fn_opus, node->fn_handle, HOOKREFRESH_DATE ? REFRESH_DATE : 0);
}

/********************************/

//
//	Compare two paths using appropriate case sensitivity.
//	Trailing slashes will be ignored in the comparison.
//	This will break if a path has multiple trailing slashes.
//	Trailing slashes will not be ignored if they are the entire path!
//
int pathcmp(ULONG ftpflags, char *path1, char *path2)
{
	char buf1[PATHLEN + 1];
	char buf2[PATHLEN + 1];
	int len1, len2;

	stccpy(buf1, path1, PATHLEN + 1);
	stccpy(buf2, path2, PATHLEN + 1);

	len1 = strlen(buf1);
	len2 = strlen(buf2);

	if (len1 > 1 && buf1[len1 - 1] == '/')
		buf1[len1 - 1] = 0;

	if (len2 > 1 && buf2[len2 - 1] == '/')
		buf2[len2 - 1] = 0;

	return ftpflags & FTP_IS_UNIX ? strcmp(buf1, buf2) : stricmp(buf1, buf2);
}

/********************************/

int casematchpattern(ULONG ftpflags, char *pat, char *str)
{
	return ftpflags & FTP_IS_UNIX ? MatchPattern(pat, str) : MatchPatternNoCase(pat, str);
}

/********************************/

//
//	Let the main process know which sigs to send to which task if abort is pressed.
//
void lst_addabort(struct ftp_node *ftpnode, ULONG signals, struct Task *task)
{
	if (ftpnode)
	{
		// Clear any (stale) ctrl-D signal (only works for this task)
		SetSignal(0L, SIGBREAKF_CTRL_D);

		ftpnode->fn_ftp.fi_abortsignals = signals;
		ftpnode->fn_signaltask = task;
	}
}

/********************************/

//
//	Let the main process know we're not interested in the abort button for now
//
void lst_remabort(struct ftp_node *ftpnode)
{
	if (ftpnode)
	{
		ftpnode->fn_ftp.fi_abortsignals = 0;
		ftpnode->fn_signaltask = 0;

		// Clear any (stale) ctrl-D signal (only works for this task)
		SetSignal(0L, SIGBREAKF_CTRL_D);
	}
}

/********************************/

//
//	Check certain Opus options
//
unsigned long lister_options(struct ftp_node *node, int type)
{
	unsigned long options = 0;

	// Use global default options or site-specific options?
	if (type != OPTION_COPY || node->fn_site.se_env->e_copy_opus_default)
	{
		options = ftpmod_options(node->fn_og, type);
	}

	else
	{
		if (node->fn_site.se_env->e_copy_datestamp)
			options |= COPY_DATE;
		if (node->fn_site.se_env->e_copy_protection)
			options |= COPY_PROT;
		if (node->fn_site.se_env->e_copy_comment)
			options |= COPY_NOTE;
		if (node->fn_site.se_env->e_copy_set_archive)
			options |= COPY_ARC;
	}

	return options;
}

/********************************/

//
//	Issue a PWD command and store the returned path in char array
//	pointed to by result.  (must be REPLYBUFLEN chars long)
//
int lister_synch_path(struct ftp_info *info, char *result)
{
	int retval = FALSE;
	int reply;

	// Can TIMEOUT
	reply = ftp_pwd(info);

	if (reply / 100 == COMPLETE)
	{
		if (strlen(info->fi_iobuf) >= 5)
		{
			stptok(info->fi_iobuf + 5, result, PATHLEN, "\"");
			// The server reply is UTF-8 when FTP_FEAT_UTF8 is set; convert
			// so callers can treat result consistently with listing names.
			ftp_convert_inplace_utf8_to_local(result, PATHLEN + 1, info);
			retval = TRUE;
		}
	}

	return retval;
}

/********************************/

//
//	Called after an FTP 'CDUP' command, this changes the path in the buffer to its parent
//	It tries to alter the buffer manually first
//	If this fails and get_current is TRUE, an FTP 'PWD' command is issued to find the true current path
//	This is useful for when you CDUP out of an assign.
//
int parent_buffer(struct ftp_info *ftp, char *buf, int get_current)
{
	BOOL ok = FALSE;
	char *p;
	int l = strlen(buf);

	if (l > 1)
	{
		p = buf + l - 1;

		if (*p == '/')
			p--;
		for (; p >= buf; p--)
		{
			// Trim trailing slash
			if (*p == '/')
			{
				// Unless it is the entire directory name
				if (p == buf)
					*(p + 1) = 0;
				else
					*p = 0;

				ok = TRUE;
				break;
			}
		}

		if (!ok && get_current)
		{
			// Can TIMEOUT
			lister_synch_path(ftp, buf);
		}
	}

	return ok;
}

/********************************/

//
//	Get information for an entry from its name
//
BOOL entry_info_from_lister(struct ftp_node *node, char *entryname, struct entry_info *ei, ULONG flags)
{
	char *fileinfo;
	struct ftp_lister_entry_info parsed;
	BOOL retval = FALSE;

	// Defalt settings
	memset(ei, 0, sizeof(struct entry_info));
	ei->ei_unixprot = 0777;

	// Ask Opus for the info on this entry
	if ((fileinfo = rexx_lst_query_entry(node->fn_opus, node->fn_handle, entryname)))
	{
		if (ftp_lister_parse_entry_info(fileinfo, entryname, &parsed))
		{
			retval = TRUE;

			stccpy(ei->ei_name, parsed.name, FILENAMELEN + 1);
			ei->ei_size = parsed.size;
			ei->ei_type = parsed.type;
			ei->ei_seconds = parsed.seconds;
			ei->ei_prot = parsed.prot;
			stccpy(ei->ei_comment, parsed.comment, COMMENTLEN + 1);
			ei->ei_unixprot = parsed.unixprot;
		}

		// Free ARexx result string
		DeleteArgstring(fileinfo);
	}

	if (!retval)
		retval = entry_info_from_remote(node, entryname, ei, flags) != 0;

	if (!retval && (flags & ENTRYFROMF_DEFAULT))
	{
		retval = TRUE;

		stccpy(ei->ei_name, entryname, FILENAMELEN + 1);
		ei->ei_size = flags & ENTRYFROMF_DEFDIR ? 0 : EI_SIZE_UNKNOWN;
		ei->ei_type = flags & ENTRYFROMF_DEFDIR ? 1 : -1;
		ei->ei_seconds = EI_SECONDS_UNKNOWN;
		ei->ei_prot = 0;
		*ei->ei_comment = 0;
		ei->ei_unixprot = 0777;
		ei->ei_linkinfo = 0;
	}

	return retval;
}

/********************************/

//
//	Fill out an entry_info structure using a remote site and some guesswork
//
//	Returns 0 if entry definitely doesn't exist
//	Returns 1 if neither MDTM nor SIZE worked
//	Returns 2 if entry definitely does exist
//
//	Note: MDTM and SIZE seldom seem to work for directories (Win32 WFTPD does work)
//
//	If filename is from requester or we have no source lister, we can't query it
//	"entryname" can be a full path
//
int entry_info_from_remote(struct ftp_node *ftpnode, char *entryname, struct entry_info *ei, ULONG flags)
{
	int size;
	int seconds;
	int result = 0;
	struct ftp_sftp_entry sftp_entry;

	stccpy(ei->ei_name, FilePart(entryname), FILENAMELEN);

	if (ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
	{
		char *utf8_entryname = ftp_convert_path_to_utf8(entryname, &ftpnode->fn_ftp);
		const char *send_entryname = utf8_entryname ? utf8_entryname : entryname;
		int stat_success = ftp_sftp_stat(&ftpnode->fn_sftp, send_entryname, &sftp_entry);
		if (utf8_entryname) ftp_codesets_free(utf8_entryname);
		
		if (!stat_success)
			return 0;

		// Only the server-provided name is UTF-8; FilePart(entryname) is already local.
		if (sftp_entry.name[0])
		{
			stccpy(ei->ei_name, sftp_entry.name, FILENAMELEN + 1);
			ftp_convert_filename_from_utf8(ei, &ftpnode->fn_ftp);
		}
		else
			stccpy(ei->ei_name, FilePart(entryname), FILENAMELEN + 1);

		ei->ei_size = sftp_entry.size;
		ei->ei_type = sftp_entry.type;
		ei->ei_seconds = ftp_sftp_unix_to_amiga_seconds(sftp_entry.seconds);
		ei->ei_prot = prot_unix_to_amiga(sftp_entry.unixprot);
		ei->ei_unixprot = sftp_entry.unixprot;
		stccpy(ei->ei_comment, sftp_entry.comment, COMMENTLEN + 1);

		return 2;
	}

	// Attempt to find file's size
	size = get_file_size(ftpnode, entryname);

	// If result is meaningful, return "defintely exists"
	if (size != -1 && size != -2 && size != -3)
	{
		result = 2;
		ei->ei_size = size;
	}

	// Attempt to find file's timestamp
	seconds = get_file_mdtm(ftpnode, entryname);

	// If MDTM returned "550 xxx: not a plain file.", assume it's a directory
	if (seconds == -2)
	{
		result = 2;
		ei->ei_type = 1;
	}

	// Any other error, assume it doesn't exist
	else if (seconds == -1 || seconds == -3)
	{
		D(bug("** MDTM invalid\n"));
	}

	// If it returned >= 0, it's a file
	else
	{
		result = 2;
		ei->ei_type = -1;
	}

	if (seconds == -1 || seconds == -2 || seconds == -3)
		ei->ei_seconds = get_curr_secs();

	// Assume typical protection bits ----RWED rwxrwxrwx
	ei->ei_prot = 0;
	ei->ei_unixprot = 0777;

	// Is neither command supported?
	// It is not safe for the Replace requester to think this file does not exist
	if ((ftpnode->fn_ftp.fi_flags & FTP_NO_MDTM) && (ftpnode->fn_ftp.fi_flags & FTP_NO_SIZE))
		result = 1;

	return result;
}

/********************************/

//
//	Attempt to find the real size of a symbolic link or hidden file etc.
//
//	>= 0 : file size
//	-1   : 500 Syntax error, command unrecognized; 501 Syntax error; 502 Not implemented
//	-2   : 550 Requested action not taken.  (DOES NOT mean it's probably a directory!)
//	-3   : Some other error
//
int get_file_size(struct ftp_node *n, char *name)
{
	int bytes;
	int reply;
	struct ftp_sftp_entry entry;

	if (n->fn_protocol == FTP_PROTOCOL_SFTP)
	{
		char *utf8_name = ftp_convert_path_to_utf8(name, &n->fn_ftp);
		const char *send_name = utf8_name ? utf8_name : name;
		int stat_success = ftp_sftp_stat(&n->fn_sftp, send_name, &entry);
		if (utf8_name) ftp_codesets_free(utf8_name);
		
		return stat_success && entry.type < 0 ? (int)entry.size : -2;
	}

	// Server supports SIZE command?
	if (!(n->fn_ftp.fi_flags & FTP_NO_SIZE))
	{
		// Can TIMEOUT
		reply = ftp_size(&n->fn_ftp, name);

		// Got size? (should return 213 "File status")
		if (reply / 100 == COMPLETE)
			bytes = atoi(n->fn_ftp.fi_iobuf + 4);

		// (not a plain file, DOES NOT mean "probably a dir")
		else if (reply == 550)
			bytes = -2;

		// SIZE command failed?
		else if (reply >= 500)
			bytes = -1;

		// unknown error
		else
			bytes = -3;
	}
	else
		// SIZE not supported
		bytes = -1;

	return bytes;
}

/********************************/

//
//	Attempt to find the real timestamp of a symbolic link or hidden file etc.
//	>= 0 : file size
//	-1   : 500 Syntax error, command unrecognized; 501 Syntax error; 502 Not implemented
//	-2   : "550 xxx: not a plain file." - probably means its a directory
//	-3   : Some other error
//
ULONG get_file_mdtm(struct ftp_node *n, char *name)
{
	ULONG mdtm = -3;
	int reply;
	char *p;
	int i;
	struct ClockData cd = {0};
	struct ftp_sftp_entry entry;

	if (n->fn_protocol == FTP_PROTOCOL_SFTP)
	{
		char *utf8_name = ftp_convert_path_to_utf8(name, &n->fn_ftp);
		const char *send_name = utf8_name ? utf8_name : name;
		int stat_success = ftp_sftp_stat(&n->fn_sftp, send_name, &entry);
		if (utf8_name) ftp_codesets_free(utf8_name);
		
		if (!stat_success)
			return -3;
		if (entry.type >= 0)
			return -2;
		return ftp_sftp_unix_to_amiga_seconds(entry.seconds);
	}

	// Unless we know the server doesn't support MDTM, we'll try it
	if (!(n->fn_ftp.fi_flags & FTP_NO_MDTM))
	{
		// Can TIMEOUT
		reply = ftp_mdtm(&n->fn_ftp, name);

		if (reply / 100 == COMPLETE)
		{
			p = n->fn_ftp.fi_iobuf + 4;

			i = (*p++ * 1000);
			i += (*p++ * 100);
			i += (*p++ * 10);
			i += *p++;
			i -= ('0' * 1000 + '0' * 100 + '0' * 10 + '0');
			cd.year = i;

			i = (*p++ * 10);
			i += *p++;
			i -= ('0' * 10 + '0');
			cd.month = i;

			i = (*p++ * 10);
			i += *p++;
			i -= ('0' * 10 + '0');
			cd.mday = i;

			i = (*p++ * 10);
			i += *p++;
			i -= ('0' * 10 + '0');
			cd.hour = i;

			i = (*p++ * 10);
			i += *p++;
			i -= ('0' * 10 + '0');
			cd.min = i;

			i = (*p++ * 10);
			i += *p;
			i -= ('0' * 10 + '0');
			cd.sec = i;

			mdtm = Date2Amiga(&cd);
		}

		// Does error message hint that the entry was a directory?
		else if (strstr(n->fn_ftp.fi_iobuf, "not a plain file."))
		{
			mdtm = -2;
		}
	}

	return mdtm;
}

/********************************/

//
//	Return a string explaining the socket error number.
//
char *sockerr(void)
{
	LONG stringnum = -1;
	static char *str;
	char *retval = NULL;
	int l;

	switch (errno)
	{
	case 0:
		stringnum = MSG_FTP_NO_ERR;
		break;

	case EINTR: /* 04 */
		stringnum = MSG_FTP_INT_SYS_CALL;
		break;

	case EACCES: /* 13 */
		stringnum = MSG_FTP_PERMISSION_DENIED;
		break;

	case EPROTONOSUPPORT: /* 43 */
		stringnum = MSG_FTP_PROTO_NOT_SUPP;
		break;

	case EADDRNOTAVAIL: /* 49 */
		stringnum = MSG_FTP_CANT_ASSIGN_ADDR;
		break;

	case ENETDOWN: /* 50 */
		stringnum = MSG_NET_DOWN;
		break;

	case ENETUNREACH: /* 51 */
		stringnum = MSG_NET_UNREACH;
		break;

	case ECONNABORTED: /* 53 */
		stringnum = MSG_CONN_ABORTED;
		break;

	case ECONNRESET: /* 54 */
		stringnum = MSG_FTP_CONN_RESET_PEER;
		break;

	case ETIMEDOUT: /* 60 */
		stringnum = MSG_FTP_TIMEOUT;
		break;

	case ECONNREFUSED: /* 61 */
		stringnum = MSG_FTP_CONN_REFUSED;
		break;

	case EHOSTDOWN: /* 64 */
		stringnum = MSG_HOST_DOWN;
		break;

	case EHOSTUNREACH: /* 65 */
		stringnum = MSG_FTP_NO_ROUTE;
		break;

	default: {
		str = GetString(locale, MSG_ERR_XX);
		l = strlen(str);

		str[l - 3] = errno / 10 + '0';
		str[l - 2] = errno % 10 + '0';
		retval = (char *)(str);
	}
	}

	if (stringnum != -1)
		retval = (char *)GetString(locale, stringnum);

	return retval;
}

/********************************/

//
//	Print the most recent line of reply text from the server
//	 in a requester.
//
void lst_server_reply(struct opusftp_globals *og,
					  struct ftp_node *displaynode,
					  struct ftp_node *errnode,
					  int default_string)
{
	char *str = 0;

	D(bug("lst_server_reply()\n"));

	// Valid?
	if (!og || !displaynode || !errnode || !errnode->fn_ipc)
		return;

	// Requesters disabled?
	if (og->og_noreq || (displaynode->fn_flags & LST_NOREQ) || (errnode->fn_flags & LST_NOREQ))
		return;

	if (errnode->fn_ftp.fi_reply < 600)
	{
		// Remove \r\n from end of line
		if ((str = strchr(errnode->fn_ftp.fi_iobuf + 4, '\r')))
			*str = 0;

		// Include error number or message only?
		str = errnode->fn_ftp.fi_iobuf + (og->og_oc.oc_log_debug ? 0 : 4);
	}
	else if (default_string)
	{
		str = GetString(locale, default_string);
	}

	// display_msg( og, errnode->fn_ipc, ((Lister *)displaynode->fn_handle)->window, 0, str );
	lister_request_tags(displaynode, FR_IPC, errnode->fn_ipc, AR_Message, str);
}

/********************************/

//
//	Print the server error reply text
//
int lst_server_err(struct opusftp_globals *og,
				   struct ftp_node *displaynode,
				   struct ftp_node *errnode,
				   ULONG flags,
				   int default_string)
{
	char handle[FTP_HANDLE_BUFSIZE];
	char *str = 0;
	DOpusCallbackInfo *infoptr = &og->og_hooks;

	D(bug("lst_server_err()\n"));

	// Valid?
	if (!og || !displaynode || !errnode || !errnode->fn_ipc)
		return 0;

	// Trigger script
	if ((og->og_hooks.dc_Script && ((errnode->fn_flags & LST_LOCAL) && og->og_oc.oc_env.e_script_error)) ||
		errnode->fn_site.se_env->e_script_error)
	{
		ftp_format_handle(handle, errnode->fn_handle);

		DC_CALL2(infoptr, dc_Script, DC_REGA0, "FTP error", DC_REGA1, handle);
		// og->og_hooks.dc_Script( "FTP error", handle );
	}

	// Requesters disabled due to shutdown?
	if (og->og_noreq)
		return flags & ERRORREQ_RETRYABORT ? 0 : 1;

	// Requesters disabled at users request?
	if (displaynode->fn_flags & LST_NOREQ || (errnode->fn_flags & LST_NOREQ))
		return flags & ERRORREQ_RETRYABORT ? 2 : 1;

	// if	(flags & ERRORREQ_RETRYABORT)
	//	flags = DSPMSG_RETRYABORT;

	// Valid string in buffer?
	if (*errnode->fn_ftp.fi_serverr)
	{
		// Remove \r\n from end of line
		if ((str = strchr(errnode->fn_ftp.fi_serverr + 4, '\r')))
			*str = 0;

		// Include error number or message only?
		str = errnode->fn_ftp.fi_serverr + (og->og_oc.oc_log_debug ? 0 : 4);
	}
	else if (default_string)
		str = GetString(locale, default_string);
	else
		// add a real default value to make sure we never get a blank msg
		str = GetString(locale, MSG_FTP_SERVER_ERROR);

	return lister_request_tags(displaynode,
							   FR_IPC,
							   errnode->fn_ipc,
							   AR_Message,
							   str,
							   flags & ERRORREQ_RETRYABORT ? FR_ButtonNum : TAG_IGNORE,
							   MSG_TRY_AGAIN,
							   flags & ERRORREQ_RETRYABORT ? FR_ButtonNum : TAG_IGNORE,
							   MSG_FTP_SKIP,
							   flags & ERRORREQ_RETRYABORT ? FR_ButtonNum : TAG_IGNORE,
							   MSG_ABORT,
							   TAG_DONE);
}

/********************************/

//
//	Print the most recent line of reply text from the server in a requester.
//	Optionally with buttons: "Try Again | Skip | Abort"
//
int lst_dos_err(struct opusftp_globals *og, struct ftp_node *ftpnode, ULONG flags, int err)
{
	char handle[FTP_HANDLE_BUFSIZE];
	char buf[80];
	char *msg = GetString(locale, MSG_FTP_DOS_ERROR);
	int result = 0;
	DOpusCallbackInfo *infoptr = &og->og_hooks;

	// Valid?
	if (!og || !ftpnode)
		return 0;

	// Trigger script
	if (og->og_hooks.dc_Script && ((ftpnode->fn_flags & LST_LOCAL) && og->og_oc.oc_env.e_script_error) ||
		ftpnode->fn_site.se_env->e_script_error)
	{
		ftp_format_handle(handle, ftpnode->fn_handle);

		DC_CALL2(infoptr, dc_Script, DC_REGA0, "FTP error", DC_REGA1, handle);
		// og->og_hooks.dc_Script( "FTP error", handle );
	}

	if (!og->og_noreq)
	{
		if (flags & ERRORREQ_RETRYABORT)
			flags = DSPMSG_RETRYABORT;

		// TODO should this be 79 or 80?

		if (!Fault(err, msg, buf, 79))
			stccpy(buf, msg, 80);

		result = display_msg(og, ftpnode->fn_ipc, ((Lister *)ftpnode->fn_handle)->window, flags, buf);
	}
	else
		result = 2;

	return result;
}

//
//	Callback hook for connect and login
//
int message_update(void *update_info, int num, char *text)
{
	struct message_update_info *mu = update_info;
	Att_Node *node;
	char *p;

	// Add line of startup message?
	if (num == -1)
	{
		if (mu->mu_msg && text && (node = Att_NewNode(mu->mu_msg, text, 0, 0)))
		{
			// Get rid of trailing carriage returns and line feeds
			if ((p = strpbrk(node->node.ln_Name, "\r\n")))
				*p = 0;
		}
	}

	// Or update progress bar info?
	else if (mu->mu_node && text)
		lister_prog_info(mu->mu_node, text);

	return 0;
}

/********************************/

int lister_cwd(struct ftp_node *ftpnode, char *path, ULONG flags)
{
	int progress;
	char *oldtitle = 0;
	int reply;
	int pwd_done = 0;
	int (*updatefn)(void *, int, char *) = 0;
	struct message_update_info *mu = 0;

	progress = ftpnode->fn_site.se_env->e_progress_window;

	if (!(flags & CWD_NO_MSG) && ftpnode->fn_site.se_env->e_show_dir && (mu = AllocVec(sizeof(*mu), MEMF_CLEAR)))
	{
		updatefn = message_update;
		mu->mu_node = ftpnode;
		mu->mu_msg = Att_NewList(LISTF_POOL);
	}

	// Show info in progress requester?
	if (progress)
	{
		lister_prog_init(ftpnode,
						 ftpnode->fn_site.se_host,
						 GetString(locale, MSG_FTP_CHANGING_DIR),
						 path,
						 PROGRESS_FILE_OFF,
						 PROGRESS_BAR_OFF);
	}

	// Else show info in title bar
	else
	{
		oldtitle = rexx_lst_title_swap(ftpnode->fn_opus, ftpnode->fn_handle, GetString(locale, MSG_FTP_CHANGING_DIR));

		send_rexxa(ftpnode->fn_opus,
				   FALSE,
				   "lister refresh " FTP_HANDLE_PRINTF " full",
				   FTP_HANDLE_VALUE(ftpnode->fn_handle));
	}

	if (ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
	{
		char *utf8_path = ftp_convert_path_to_utf8(path, &ftpnode->fn_ftp);
		const char *send_path = utf8_path ? utf8_path : path;
		reply = ftp_sftp_cwd(&ftpnode->fn_sftp, send_path) ? 250 : 550;
		if (utf8_path) ftp_codesets_free(utf8_path);
	}
	else
		reply = ftp_cwd(&ftpnode->fn_ftp, updatefn, mu, path);

	// Find new path by sending PWD or calculating it ourselves
	if (reply / 100 == COMPLETE)
	{
		if (mu && mu->mu_msg)
			lister_long_message(ftpnode, mu->mu_msg, LONGMSG_MULTI);

		if (ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
		{
			char utf8_path[PATHLEN + 1];
			if (ftp_sftp_pwd(&ftpnode->fn_sftp, utf8_path, sizeof(utf8_path)))
				ftp_store_utf8_path_as_local(ftpnode->fn_site.se_path, PATHLEN + 1, utf8_path, &ftpnode->fn_ftp);
			pwd_done = 1;
		}

		// Try synching without PWD
		if (!pwd_done && strlen(ftpnode->fn_ftp.fi_iobuf) >= 5 && ftpnode->fn_ftp.fi_iobuf[4] == '"' &&
			strchr(ftpnode->fn_ftp.fi_iobuf + 5, '"'))
		{
			stptok(ftpnode->fn_ftp.fi_iobuf + 5, ftpnode->fn_site.se_path, PATHLEN, "\"");
			// CWD reply echoes the new path in UTF-8 when FTP_FEAT_UTF8 is set.
			ftp_convert_inplace_utf8_to_local(ftpnode->fn_site.se_path, PATHLEN + 1, &ftpnode->fn_ftp);
			pwd_done = 1;
		}

		// Still need to synch?  Send PWD.
		if (!pwd_done)
		{
			if ((flags & CWD_ALWAYS_SYNCH) || ftpnode->fn_site.se_env->e_safe_links)
			{
				// Can TIMEOUT
				if (lister_synch_path(&ftpnode->fn_ftp, ftpnode->fn_site.se_path))
					pwd_done = 1;
			}

			// Still need to synch?  Do it manually.
			if (!pwd_done)
			{
				if (flags & CWD_RELATIVE)
					AddPart(ftpnode->fn_site.se_path, path, PATHLEN);
				else
					strcpy(ftpnode->fn_site.se_path, path);
			}
		}
	}

	if (progress)
		lister_prog_clear(ftpnode);

	else if (oldtitle)
	{
		rexx_lst_title(ftpnode->fn_opus, ftpnode->fn_handle, oldtitle);
		DeleteArgstring(oldtitle);
		send_rexxa(ftpnode->fn_opus,
				   FALSE,
				   "lister refresh " FTP_HANDLE_PRINTF " full",
				   FTP_HANDLE_VALUE(ftpnode->fn_handle));
	}

	if (mu)
	{
		if (mu->mu_msg)
			Att_RemList(mu->mu_msg, 0);

		FreeVec(mu);
	}

	return reply;
}

/********************************/

static int lister_cdup(struct ftp_node *ftpnode)
{
	int progress;
	char *oldtitle = 0;
	int reply;
	int pwd_done = 0;
	int (*updatefn)(void *, int, char *) = 0;
	struct message_update_info *mu = 0;

	progress = ftpnode->fn_site.se_env->e_progress_window;

	if (ftpnode->fn_site.se_env->e_show_dir && (mu = AllocVec(sizeof(*mu), MEMF_CLEAR)))
	{
		updatefn = message_update;
		mu->mu_node = ftpnode;
		mu->mu_msg = Att_NewList(LISTF_POOL);
	}

	if (progress)
	{
		lister_prog_init(ftpnode,
						 ftpnode->fn_site.se_host,
						 GetString(locale, MSG_FTP_CHANGING_DIR),
						 PROGRESS_NO_NAME,
						 PROGRESS_FILE_OFF,
						 PROGRESS_BAR_OFF);
	}
	else
	{
		oldtitle = rexx_lst_title_swap(ftpnode->fn_opus, ftpnode->fn_handle, GetString(locale, MSG_FTP_CHANGING_DIR));

		send_rexxa(ftpnode->fn_opus,
				   FALSE,
				   "lister refresh " FTP_HANDLE_PRINTF " full",
				   FTP_HANDLE_VALUE(ftpnode->fn_handle));
	}

	if (ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
		reply = ftp_sftp_cdup(&ftpnode->fn_sftp) ? 250 : 550;
	else
		reply = ftp_cdup(&ftpnode->fn_ftp, updatefn, mu);

	// Find new path by sending PWD or calculating it ourselves
	if (reply / 100 == COMPLETE)
	{
		if (mu && mu->mu_msg)
			lister_long_message(ftpnode, mu->mu_msg, LONGMSG_MULTI);

		if (ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
		{
			char utf8_path[PATHLEN + 1];
			if (ftp_sftp_pwd(&ftpnode->fn_sftp, utf8_path, sizeof(utf8_path)))
				ftp_store_utf8_path_as_local(ftpnode->fn_site.se_path, PATHLEN + 1, utf8_path, &ftpnode->fn_ftp);
			pwd_done = 1;
		}

		// Try synching without PWD
		if (!pwd_done && strlen(ftpnode->fn_ftp.fi_iobuf) >= 5 && ftpnode->fn_ftp.fi_iobuf[4] == '"' &&
			strchr(ftpnode->fn_ftp.fi_iobuf + 5, '"'))
		{
			stptok(ftpnode->fn_ftp.fi_iobuf + 5, ftpnode->fn_site.se_path, PATHLEN, "\"");
			// CDUP reply echoes the new path in UTF-8 when FTP_FEAT_UTF8 is set.
			ftp_convert_inplace_utf8_to_local(ftpnode->fn_site.se_path, PATHLEN + 1, &ftpnode->fn_ftp);
			pwd_done = 1;
		}

		// Still need to work out new path?
		if (!pwd_done)
		{
			if (ftpnode->fn_site.se_env->e_safe_links)
			{
				// Can TIMEOUT
				if (lister_synch_path(&ftpnode->fn_ftp, ftpnode->fn_site.se_path))
					pwd_done = 1;
			}

			// Still need to work out new path?
			if (!pwd_done)
			{
				// Can TIMEOUT
				parent_buffer(&ftpnode->fn_ftp, ftpnode->fn_site.se_path, TRUE);
			}
		}
	}

	if (progress)
		lister_prog_clear(ftpnode);

	else if (oldtitle)
	{
		rexx_lst_title(ftpnode->fn_opus, ftpnode->fn_handle, oldtitle);
		DeleteArgstring(oldtitle);
		send_rexxa(ftpnode->fn_opus,
				   FALSE,
				   "lister refresh " FTP_HANDLE_PRINTF " full",
				   FTP_HANDLE_VALUE(ftpnode->fn_handle));
	}

	if (mu)
	{
		if (mu->mu_msg)
			Att_RemList(mu->mu_msg, 0);

		FreeVec(mu);
	}

	return reply;
}

/********************************/

//
//	Remove a lister from the global list and free its resources.
//
void lister_remove_node(struct opusftp_globals *og, struct ftp_node *ftpnode)
{
	if (ftpnode)
	{
		ListLockRemove(&og->og_listerlist, (struct Node *)ftpnode, &og->og_listercount);
		ftp_sftp_session_cleanup(&ftpnode->fn_sftp);
		FreeVec(ftpnode);
	}
}

/********************************/

//
//	Cleans up what ftplister_init does
//
static void ftplister_cleanup(struct opusftp_globals *ogp, IPCData *ipc)
{
	struct globals *g = ipc->userdata;

	if (g)
	{
#ifdef __amigaos4__
		if (g->g_isocket)
		{
			DropInterface((struct Interface *)g->g_isocket);
		}
#endif

		if (g->g_socketbase)
		{
			CloseLibrary(g->g_socketbase);
		}
		FreeVec(g);
	}
}

/********************************/

/*
 *	Init code for lister process
 */

//#ifdef __amigaos4__
// static ULONG ASM ftplister_init( REG(a0, IPCData *ipc), REG(a2, int skip), REG(a1, struct subproc_data *data ))
//#else
IPC_StartupCode(ftplister_init, struct subproc_data *, data)
//#endif
{
	struct opusftp_globals *ogp;
	ULONG retval = 0;
	struct globals *g;

	if (data)
	{
		ogp = data->spd_ogp;

		data->spd_ipc = ipc; /* 'ipc' points to this task's tc_UserData field */

		if ((g = ipc->userdata = AllocVec(sizeof(struct globals), MEMF_CLEAR)))
		{
			if ((g->g_socketbase = OpenLibrary("bsdsocket.library", 3)))
			{
#ifdef __amigaos4__
				if ((g->g_isocket = (struct SocketIFace *)GetInterface(g->g_socketbase, "main", 1, NULL)))
#endif
					retval = 1;
#ifdef __amigaos4__
				else
					CloseLibrary(g->g_socketbase);
#endif
			}

			g->g_master_ipc = data->spd_owner_ipc;
		}
		if (!retval)
			ftplister_cleanup(ogp, ipc);
	}

	return retval;
}

/********************************/
/************ Actions ***********/
/********************************/

//
//	Create a new directory on the ftp site
//	Should use true datestamp & protection bits
//
static void lister_makedir(struct ftp_node *ftpnode, IPCMessage *msg)
{
	struct ftp_msg *fm;
	int reply;
	char namebuf[FILENAMELEN + 1] = "";
	char *name = 0;
	struct connect_msg *cm;
	int rexx_result = 0;

	// Valid?
	if (!ftpnode || !msg || !msg->data_free)
		return;

	fm = msg->data_free;

	if (!(name = fm->fm_names) && !ftpnode->fn_og->og_noreq)
	{
		if (lister_request_tags(ftpnode,
								FR_MsgNum,
								MSG_FTP_ENTER_DIRNAME,
								FR_ButtonNum,
								MSG_FTP_OKAY,
								FR_ButtonNum,
								MSG_FTP_CANCEL,
								AR_Buffer,
								namebuf,
								AR_BufLen,
								FILENAMELEN,
								TAG_DONE) &&
			*namebuf)
			name = namebuf;
	}

	if (name)
	{
		rexx_lst_lock(ftpnode->fn_opus, ftpnode->fn_handle);

		if (ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
		{
			char *utf8_name = ftp_convert_path_to_utf8(name, &ftpnode->fn_ftp);
			const char *send_name = utf8_name ? utf8_name : name;
			reply = ftp_sftp_mkdir(&ftpnode->fn_sftp, send_name) ? 250 : 550;
			if (utf8_name) ftp_codesets_free(utf8_name);
		}
		else
			// Can TIMEOUT
			reply = ftp_mkd(&ftpnode->fn_ftp, name);

		if (reply / 100 == COMPLETE)
		{
			int mdtm;
			LONG prot = 0;

			mdtm = get_file_mdtm(ftpnode, name);

			if (mdtm < 0)
				mdtm = get_curr_secs();

			if (ftpnode->fn_protocol == FTP_PROTOCOL_SFTP || ftpnode->fn_ftp.fi_flags & FTP_IS_UNIX)
			{
				if (name[0] == '.')
					prot |= FIBF_HIDDEN;
				else
					prot &= ~FIBF_HIDDEN;
			}

			lister_add(ftpnode,
					   name,   // Name
					   0,	   // Size
					   1,	   // Type
					   mdtm,   // Seconds
					   prot,   // Protection
					   NULL);  // Comment

			if (msg->flags & MKDIR_SELECT)
				rexx_lst_select(ftpnode->fn_opus, ftpnode->fn_handle, name, 1);

			ftplister_refresh(ftpnode, 0);

			if (msg->flags & MKDIR_READ)
			{
				// Can TIMEOUT
				reply = lister_cwd(ftpnode, name, CWD_RELATIVE);

				if (ftpnode->fn_ftp.fi_reply < 500 && ftpnode->fn_ftp.fi_reply != 421)
				{
					// Can TIMEOUT
					lister_list(ftpnode->fn_og, ftpnode, FALSE);
					rexx_result = 1;
				}
			}
			else if (msg->flags & MKDIR_NEW)
			{
				if ((cm = get_blank_connectmsg(ftpnode->fn_og)))
				{
					stccpy(cm->cm_opus, ftpnode->fn_og->og_opusname, PORTNAMELEN + 1);

					copy_site_entry(ftpnode->fn_og, &cm->cm_site, &ftpnode->fn_site);
					stccpy(cm->cm_site.se_path, name, PATHLEN + 1);

					IPC_Command(ftpnode->fn_og->og_main_ipc, IPC_CONNECT, 0, cm, 0, REPLY_NO_PORT);
					rexx_result = 1;

					FreeVec(cm);
				}
			}
			else
				rexx_result = 1;
		}
		else
		{
			if (ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
				sprintf(ftpnode->fn_ftp.fi_serverr, "550 %s\r\n", ftp_sftp_error_message(&ftpnode->fn_sftp));
			lst_server_err(ftpnode->fn_og, ftpnode, ftpnode, 0, MSG_FTP_MAKEDIR_ERROR);
		}

		rexx_lst_unlock(ftpnode->fn_opus, ftpnode->fn_handle);
	}

	if (fm->fm_rxmsg)
		reply_rexx(fm->fm_rxmsg, rexx_result, 0);
}

/********************************/

//
//	Rename selected entries on the ftp site
//	Should report failure
//	Type, size, datestamp, and protection bits may be incorrect if requester is used
//	- should use 'LIST entry'
//	- 'LIST entry' not possible when 'entry' is a dir!
//
static void lister_rename(struct ftp_node *ftpnode, IPCMessage *msg)
{
	IPTR handle = ftpnode->fn_handle;
	struct ftp_msg *fm;
	char newname[FILENAMELEN + 1] = {0};
	char **namearray = 0;
	char **namep;
	BOOL gotfileinfo;  // Real name (for links)
	struct entry_info ei = {0};
	BOOL abort = FALSE;
	int reply;				   // FTP reply
	BOOL exactname = TRUE;	   // Is req'd file's name exact match with one in lister ?
	int total = 0, count = 1;  // Used in bar graph display
	int used_req = FALSE;
	int rexx_result = 0;

	D(bug("lister_rename()\n"));

	// Valid?
	if (!ftpnode || !msg || !msg->data_free)
		return;

	fm = msg->data_free;

	if (fm->fm_names && *fm->fm_names && (namearray = alloc_name_array(fm->fm_names)))
	{
		rexx_lst_lock(ftpnode->fn_opus, handle);

		total = rexx_lst_query_numselfiles(ftpnode->fn_opus, handle);

		lister_prog_init(ftpnode,
						 ftpnode->fn_site.se_host,
						 GetString(locale, MSG_FTP_RENAMING),
						 "",
						 PROGRESS_FILE_OFF,
						 PROGRESS_BAR_ON);

		// For each selected entry
		for (namep = namearray; *namep; namep++)
		{
			rexx_result = 0;

			gotfileinfo = entry_info_from_lister(ftpnode, *namep, &ei, ENTRYFROMF_DEFAULT);

			if (used_req && gotfileinfo && pathcmp(ftpnode->fn_ftp.fi_flags, *namep, ei.ei_name))
				exactname = FALSE;

			// Requested name is not in lister?
			if (used_req && (!gotfileinfo || !exactname))
			{												 /* so we must make some assumptions */
				stccpy(ei.ei_name, *namep, FILENAMELEN);	 /* this bit is simple */
				ei.ei_size = get_file_size(ftpnode, *namep); /* attempt to find file's size */
				ei.ei_type = -1;							 /* assume it's a file */

				ei.ei_seconds = get_file_mdtm(ftpnode, *namep);
				if (ei.ei_seconds == -1)
					ei.ei_seconds = get_curr_secs();

				// Standard protection bits ----RWED
				ei.ei_prot = 0;
			}

			// If we have a file to rename
			if (gotfileinfo || used_req)
			{
				lister_prog_name(ftpnode, ei.ei_name);

				lister_prog_bar(ftpnode, total, count);

				rexx_lst_label(ftpnode->fn_opus, handle, 0, ei.ei_name, 0);

				stccpy(newname, ei.ei_name, FILENAMELEN);

				if (lister_request_tags(ftpnode,
										FR_MsgNum,
										MSG_FTP_ENTER_NEW_FILENAME,
										FR_ButtonNum,
										MSG_FTP_RENAME_UCASE,
										FR_ButtonNum,
										MSG_ABORT,
										AR_Buffer,
										newname,
										AR_BufLen,
										FILENAMELEN,
										TAG_DONE) &&
					*newname)
				{
					if (ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
					{
						char *utf8_oldname = ftp_convert_path_to_utf8(ei.ei_name, &ftpnode->fn_ftp);
						char *utf8_newname = ftp_convert_path_to_utf8(newname, &ftpnode->fn_ftp);
						
						const char *send_oldname = utf8_oldname ? utf8_oldname : ei.ei_name;
						const char *send_newname = utf8_newname ? utf8_newname : newname;
						
						reply = ftp_sftp_rename(&ftpnode->fn_sftp, send_oldname, send_newname) ? 250 : 550;
						
						if (utf8_oldname) ftp_codesets_free(utf8_oldname);
						if (utf8_newname) ftp_codesets_free(utf8_newname);
					}
					else
						reply = ftp_rename(&ftpnode->fn_ftp, ei.ei_name, newname);

					if (reply / 100 == COMPLETE)
					{
						rexx_result = 1;

						// Only remove if name is exact for hidden files
						if (!used_req || exactname)
							rexx_lst_remove(ftpnode->fn_opus, handle, *namep);

						// Only add hidden files if there is no visible file with similar name
						// and we know the file's size or it's a dir

						if ((ei.ei_size >= 0 || ei.ei_type >= 0) && (!gotfileinfo || exactname))
						{
							// Set/clear hidden flag?
							if (ftpnode->fn_protocol == FTP_PROTOCOL_SFTP || ftpnode->fn_ftp.fi_flags & FTP_IS_UNIX)
							{
								if (newname[0] == '.')
									ei.ei_prot |= FIBF_HIDDEN;
								else
									ei.ei_prot &= ~FIBF_HIDDEN;
							}

							lister_add(
								ftpnode, newname, ei.ei_size, ei.ei_type, ei.ei_seconds, ei.ei_prot, ei.ei_comment);
						}

						ftplister_refresh(ftpnode, 0);
					}
					else
					{
						if (ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
						{
							ftpnode->fn_ftp.fi_reply = 550;
							sprintf(ftpnode->fn_ftp.fi_iobuf, "550 %s\r\n", ftp_sftp_error_message(&ftpnode->fn_sftp));
						}
						lst_server_reply(ftpnode->fn_og, ftpnode, ftpnode, MSG_FTP_RENAME_ERROR);
					}

					if (reply == 421)
						abort = TRUE;
				}
				else
					abort = TRUE;
			}
		}

		rexx_lst_label(ftpnode->fn_opus,
					   handle,
					   ftpnode->fn_protocol == FTP_PROTOCOL_SFTP ? "SFTP:" : "FTP:",
					   ftpnode->fn_site.se_host,
					   NULL);
		lister_prog_clear(ftpnode);

		rexx_lst_unlock(ftpnode->fn_opus, handle);

		FreeVec(namearray);
	}

	if (fm->fm_rxmsg)
		reply_rexx(fm->fm_rxmsg, rexx_result, 0);
}

/********************************/

//
//	Hook called before commencing deletion of each entry
//
static int hook_delete_pre(struct hook_rec_data *hc,
						   char *dirname,
						   struct rec_entry_list *destlist,
						   struct entry_info *entry)
{
	char info[256 + 1];

	// Valid?
	if (!hc)
		return 0;

	// Update progress bar name
	if (hc->hc_prognode)
	{
		if (dirname)
			final_path(dirname, hc->hc_fromdirname);
		if (entry)
		{
			lister_prog_name(hc->hc_prognode, entry->ei_name);

			strcpy(info, GetString(locale, MSG_FROM));
			strcat(info, " '");
			strcat(info, hc->hc_fromdirname);
			strcat(info, "'");

			lister_prog_info(hc->hc_prognode, info);
		}
	}

	return 1;
}

//
//	Delete an entry from an ftp site
//
//	Should allow recursive directory deletion
//
static void lister_delete(struct ftp_node *ftpnode, IPCMessage *msg)
{
	struct ftp_msg *fm;
	int entrycount;	  // Total number of entries to delete
	int entrynumber;  // Number of entry being deleted
	int quiet;		  // Don't show confirmation requesters
	endpoint *source;
	struct entry_info *ei;
	int confirm_commence = 0;  // Commence deleting confirmed?
	char path[PATHLEN + 1];	   // Current path in lister
	int success;
	ULONG options;
	struct hook_rec_data hc = {0};
	char startdirname[FILENAMELEN + 1] = {0};
	int rexx_result = 0;

	// Valid?
	if (!ftpnode || !msg || !msg->data_free)
		return;

	// Get delete message
	fm = msg->data_free;

	// Clear abort flag
	ftpnode->fn_flags &= ~LST_ABORT;

	// Got entries to delete?
	if (fm->fm_entries)
	{
		entrycount = fm->fm_filecount + fm->fm_dircount;

		quiet = fm->fm_flags & DELE_OPT_QUIET;

		// Create endpoint
		if ((source = create_endpoint_tags(ftpnode->fn_og, EP_TYPE, ENDPOINT_FTP, EP_FTPNODE, ftpnode, TAG_DONE)))
		{
			// Get 'From' path for progress bar
			final_path(ftpnode->fn_site.se_path, startdirname);
			final_path(ftpnode->fn_site.se_path, hc.hc_fromdirname);

			// Get Delete options
			options = lister_options(ftpnode, OPTION_DELETE);

			// Need to show 'commence deleting' requester?
			if (quiet || !(options & DELETE_ASK))
				confirm_commence = 1;
			else
			{
				// Only one entry?  Could be from RMB popup or command line
				if (entrycount == 1)
				{
					// Does entry contain path information?
					if (strpbrk(fm->fm_entries[0].ei_name, ":/"))
						D(bug("** entry contains path information\n"));
				}

				// Show requester
				confirm_commence = lister_request_tags(ftpnode,
													   FR_FormatString,
													   "%s\n%s:\n\n%ld %s\n%ld %s\n%s %s?",
													   FR_MsgNum,
													   MSG_FTP_WARNING_CANT_GET_BACK,
													   FR_MsgNum,
													   MSG_FTP_OK_TO_DELETE,
													   AR_Message,
													   fm->fm_filecount,
													   FR_MsgNum,
													   MSG_FTP_FILES_AND,
													   AR_Message,
													   fm->fm_dircount,
													   FR_MsgNum,
													   MSG_FTP_DRAWERS,
													   FR_MsgNum,
													   MSG_FTP_FROM,
													   AR_Message,
													   ftpnode->fn_site.se_host,
													   FR_ButtonNum,
													   MSG_FTP_PROCEED,
													   FR_ButtonNum,
													   MSG_FTP_DELETEALL,
													   FR_ButtonNum,
													   MSG_FTP_CANCEL,
													   TAG_DONE);

				// Delete all?  Show no more confirmation requesters
				if (confirm_commence == 2)
					quiet = 1;
			}

			if (confirm_commence)
			{
				// Get path
				lister_get_path(ftpnode, path);

				// Show progress bar
				lister_prog_init(ftpnode,
								 GetString(locale, MSG_FTP_DELETING),  // Title
								 "",								   // Info
								 "",								   // Name
								 PROGRESS_FILE_OFF,					   // File
								 PROGRESS_BAR_ON);					   // Bar

				// Set up delete hook data
				hc.hc_og = ftpnode->fn_og;
				hc.hc_source = source;
				hc.hc_pre = hook_delete_pre;
				hc.hc_opus = ftpnode->fn_opus;
				hc.hc_prognode = ftpnode;
				hc.hc_proghandle = ftpnode->fn_handle;

				if (ftpnode->fn_site.se_env->e_recursive_special)
					hc.hc_recur_flags |= RECURF_BROKEN_LS;

				// Should use callbacks to get entries
				// (except rmb popup coz they won't be highlit)
				for (ei = fm->fm_entries, entrynumber = 1;
					 entrynumber <= entrycount && !(ftpnode->fn_flags & LST_ABORT);
					 ++ei, ++entrynumber)
				{
					int confirm_entry = 1;

					rexx_result = 0;

					// Update progress bar
					lister_prog_bar(ftpnode, entrycount, entrynumber);

					// Confirm dirs?
					if (!quiet && ei->ei_type >= 0 && options & DELETE_DIRS)
					{
						confirm_entry = lister_request_tags(ftpnode,
															FR_FormatString,
															"'%s' %s",
															AR_Message,
															ei->ei_name,
															FR_MsgNum,
															MSG_DELETE_DIRS,
															FR_ButtonNum,
															MSG_DELETE_FILES,
															FR_ButtonNum,
															MSG_FTP_DELETEALL,
															FR_ButtonNum,
															MSG_FTP_SKIP,
															FR_ButtonNum,
															MSG_ABORT,
															TAG_DONE);
					}

					// Confirm files?
					else if (!quiet && ei->ei_type < 0 && options & DELETE_FILES)
					{
						confirm_entry = lister_request_tags(ftpnode,
															FR_FormatString,
															"%s '%s'?",
															FR_MsgNum,
															MSG_DELETE_FILES,
															AR_Message,
															ei->ei_name,
															FR_ButtonNum,
															MSG_DELETE_FILES,
															FR_ButtonNum,
															MSG_FTP_DELETEALL,
															FR_ButtonNum,
															MSG_FTP_SKIP,
															FR_ButtonNum,
															MSG_ABORT,
															TAG_DONE);
					}

					// Delete all?  Show no more confirmation requesters
					if (confirm_entry == 2)
						quiet = 1;

					if (confirm_entry == 1 || confirm_entry == 2)
					{
						// Delete entry (and contents if it's a dir)
						if ((success = recursive_delete(&hc, startdirname, ei) == 2))
						{
							rexx_lst_remove(ftpnode->fn_opus, ftpnode->fn_handle, ei->ei_name);
							rexx_result = 1;
						}
					}

					// Abort hit on confirm requester?
					else if (confirm_entry == 0)
						ftpnode->fn_flags |= LST_ABORT;
				}

				// Remove progress bar
				lister_prog_clear(ftpnode);

				// Show updated contents
				rexx_lst_refresh(ftpnode->fn_opus, ftpnode->fn_handle, 0);
			}

			delete_endpoint(source);
		}
	}

	if (fm->fm_rxmsg)
		reply_rexx(fm->fm_rxmsg, rexx_result, 0);
}

/********************************/

//
//	Re-read the current directory
//
static void lister_reread(struct ftp_node *ftpnode, IPCMessage *msg)
{
	int ok;

	ok = lister_list(ftpnode->fn_og, ftpnode, TRUE);

	if (msg->data)
		reply_rexx((struct RexxMsg *)msg->data, ok, 0);
}

/********************************/

//
//	Change current directory to parent
//
static void lister_parent(struct ftp_node *ftpnode, IPCMessage *msg)
{
	int reply;
	int rexx_result = 0;

	rexx_lst_lock(ftpnode->fn_opus, ftpnode->fn_handle);

	// Can TIMEOUT
	reply = lister_cdup(ftpnode);

	rexx_lst_unlock(ftpnode->fn_opus, ftpnode->fn_handle);

	if (reply / 100 == COMPLETE)
	{
		// Can TIMEOUT
		rexx_result = lister_list(ftpnode->fn_og, ftpnode, FALSE);
	}
	else if (reply != 421)
		lst_server_reply(ftpnode->fn_og, ftpnode, ftpnode, MSG_FTP_PATH_NOT_FOUND);

	if (msg->data)
		reply_rexx((struct RexxMsg *)msg->data, rexx_result, 0);
}

/********************************/

//
//	Change current directory to root - Unix and Amiga only at the moment !!
//
static void lister_root(struct ftp_node *ftpnode, IPCMessage *msg)
{
	int reply;
	char *rootname = ftpnode->fn_systype == FTP_AMIGA ? ":" : "/";
	ULONG cwd_flags = 0;
	int rexx_result = 0;

	if (ftpnode->fn_systype == FTP_AMIGA)
		cwd_flags |= CWD_ALWAYS_SYNCH;

	rexx_lst_lock(ftpnode->fn_opus, ftpnode->fn_handle);

	// Can TIMEOUT
	reply = lister_cwd(ftpnode, rootname, cwd_flags);

	rexx_lst_unlock(ftpnode->fn_opus, ftpnode->fn_handle);

	if (reply / 100 == COMPLETE)
	{
		// Can TIMEOUT
		rexx_result = lister_list(ftpnode->fn_og, ftpnode, FALSE);
	}
	else if (reply != 421)
		lst_server_reply(ftpnode->fn_og, ftpnode, ftpnode, MSG_FTP_PATH_NOT_FOUND);

	if (msg->data)
		reply_rexx((struct RexxMsg *)msg->data, rexx_result, 0);
}

/********************************/

//
//	Change current directory to specified path
//
static void lister_path(struct ftp_node *ftpnode, IPCMessage *msg)
{
	struct ftp_msg *fm;
	BOOL rescan = TRUE;
	int reply;
	int success = 0;

	// Valid?
	if (!ftpnode || !msg || !msg->data_free)
		return;

	fm = msg->data_free;

	if (fm->fm_names && *fm->fm_names)
	{
		// GP 17.04.98 Fix for entering path in path gadget
		// to fix problem with overwriting cache name.
		// The call to lister_list should NOT have rescan set if the entered
		// path if different since this will call lst_clear not lst_empty

		// Must check if paths are different here since they are synced
		// in the lister_cwd call!

		// Path changed?  Rescan if so.

		if (pathcmp(ftpnode->fn_ftp.fi_flags, ftpnode->fn_site.se_path, fm->fm_names))
			rescan = FALSE;

		rexx_lst_lock(ftpnode->fn_opus, ftpnode->fn_handle);

		reply = lister_cwd(ftpnode, fm->fm_names, 0);

		rexx_lst_unlock(ftpnode->fn_opus, ftpnode->fn_handle);

		if (reply / 100 == COMPLETE)
		{
			// Can TIMEOUT
			success = lister_list(ftpnode->fn_og, ftpnode, rescan);
		}
		else if (reply != 421)
		{
			lst_server_reply(ftpnode->fn_og, ftpnode, ftpnode, MSG_FTP_PATH_NOT_FOUND);

			rexx_lst_set_path(ftpnode->fn_opus, ftpnode->fn_handle, ftpnode->fn_site.se_path);
		}
	}

	if (fm->fm_rxmsg)
		reply_rexx(fm->fm_rxmsg, success, 0);
}

/********************************/

//
//	Change current directory to specified path after call from cachelist
//
static void lister_cache_path(struct ftp_node *ftpnode, IPCMessage *msg)
{
	struct ftp_msg *fm;
	int reply;
	int rexx_result = 0;

	fm = msg->data_free;

	if (fm)
	{
		if (fm->fm_names)
		{
			rexx_lst_lock(ftpnode->fn_opus, ftpnode->fn_handle);

			reply = lister_cwd(ftpnode, fm->fm_names, CWD_NO_MSG);

			rexx_lst_unlock(ftpnode->fn_opus, ftpnode->fn_handle);

			if (reply / 100 == COMPLETE)
				rexx_result = 1;

			else if (reply != 421 && reply < 600)
			{
				lst_server_reply(ftpnode->fn_og, ftpnode, ftpnode, MSG_FTP_PATH_NOT_FOUND);

				rexx_lst_set_path(ftpnode->fn_opus, ftpnode->fn_handle, ftpnode->fn_site.se_path);
			}
		}

		// Reply to forwarded ARexx message
		if (fm->fm_rxmsg)
			reply_rexx(fm->fm_rxmsg, rexx_result, 0);
	}
}

/********************************/

//
//	Issue a literal FTP command
//
static int lister_cmd(struct ftp_node *ftpnode, IPCMessage *msg)
{
	struct ftp_msg *fm;
	char cmdbuf[256 + 1] = "";
	char *cmd = NULL;
	int reply = 600;
	int (*updatefn)(void *, int, char *) = 0;
	struct message_update_info *mu = 0;
	int result = 0;

	D(bug("lister_cmd()\n"));

	if ((fm = msg->data))
	{
		if ((fm->fm_flags & CMD_OPT_LONG) && (mu = AllocVec(sizeof(*mu), MEMF_CLEAR)))
		{
			updatefn = message_update;
			mu->mu_node = ftpnode;
			mu->mu_msg = Att_NewList(LISTF_POOL);
		}

		cmd = fm->fm_names;

		if (!cmd || !*cmd)
		{
			// If no arguments were supplied, prompt user for them now
			if (lister_request_tags(ftpnode,
									FR_MsgNum,
									MSG_FTP_CMD_TO_SEND,
									FR_ButtonNum,
									MSG_FTP_OKAY,
									FR_ButtonNum,
									MSG_FTP_CANCEL,
									AR_Buffer,
									cmdbuf,
									AR_BufLen,
									256,
									TAG_DONE) &&
				*cmdbuf)
				cmd = cmdbuf;
		}

		if (cmd)
		{
			rexx_lst_lock(ftpnode->fn_opus, ftpnode->fn_handle);

			lister_prog_init(ftpnode,
							 ftpnode->fn_site.se_host,
							 GetString(locale, MSG_FTP_SENDING_CMD),
							 PROGRESS_NO_NAME,
							 PROGRESS_FILE_OFF,
							 PROGRESS_BAR_OFF);

			reply = _ftpa(&ftpnode->fn_ftp, FTPFLAG_ASYNCH, "%s", cmd);
			if (reply < 600)
				reply = _getreply(&ftpnode->fn_ftp, 0, updatefn, mu);

			// Not an error response?
			if (reply < 500 && reply != 421)
				result = 1;

			// Long response?
			if (mu && mu->mu_msg)
				lister_long_message(ftpnode, mu->mu_msg, 0);

			// Normal response?
			else if (!(fm->fm_flags & CMD_OPT_QUIET) && reply < 600 && reply != 421)
				lst_server_reply(ftpnode->fn_og, ftpnode, ftpnode, 0);

			lister_prog_clear(ftpnode);

			rexx_lst_unlock(ftpnode->fn_opus, ftpnode->fn_handle);
		}

		if (mu)
		{
			if (mu->mu_msg)
				Att_RemList(mu->mu_msg, 0);

			FreeVec(mu);
		}
	}

	msg->command = result;
	return result;
}

/********************************/

//
//	Set a local variable.
//
static int lister_setvar(struct ftp_node *ftpnode, IPCMessage *msg)
{
	char cmdbuf[LSCMDLEN + 1] = "";
	char *cmd;

	D(bug("lister_setvar()\n"));

	// Valid?
	if (!ftpnode || !msg)
		return 0;

	if (msg->flags == SET_LISTCMD || msg->flags == SET_QUIET)
	{
		if ((cmd = msg->data_free))
		{
			stccpy(cmdbuf, cmd, LSCMDLEN + 1);
		}
		else
			lister_request_tags(ftpnode,
								FR_MsgNum,
								MSG_FTP_LOCAL_VAR,
								FR_ButtonNum,
								MSG_FTP_OKAY,
								FR_ButtonNum,
								MSG_FTP_CANCEL,
								AR_Buffer,
								cmdbuf,
								AR_BufLen,
								LSCMDLEN,
								TAG_DONE);

		if (*cmdbuf)
		{
			if (msg->flags == SET_LISTCMD)
			{
				stccpy(ftpnode->fn_lscmd, cmdbuf, LSCMDLEN + 1);
				msg->command = 1;
			}
			else if (msg->flags == SET_QUIET)
			{
				msg->command = 1;

				if (isdigit(*cmdbuf))
				{
					if (atoi(cmdbuf) == 1)
						ftpnode->fn_flags |= LST_NOREQ;
					else if (atoi(cmdbuf) == 0)
						ftpnode->fn_flags &= ~LST_NOREQ;
					else
						msg->command = 0;
				}
				else if (!stricmp(cmdbuf, "on"))
					ftpnode->fn_flags |= LST_NOREQ;
				else if (!stricmp(cmdbuf, "off"))
					ftpnode->fn_flags &= ~LST_NOREQ;
				else
					msg->command = 0;
			}
		}
	}

	return 1;
}

/********************************/

//
//	Do a favour for another (FTP lister) process
//
static int lister_favour(struct ftp_node *ftpnode, IPCMessage *msg)
{
	struct favour_msg *fm;

	if (!ftpnode || !msg)
		return 0;

	fm = msg->data_free;

	switch (fm->fm_ftp_command)
	{
	case FAVOUR_LIST:
		msg->command = (IPTR)fm->fm_endpoint->ep_list(fm->fm_endpoint, fm->fm_arg1);
		break;
	case FAVOUR_CWD:
		msg->command = fm->fm_endpoint->ep_cwd(fm->fm_endpoint, fm->fm_arg1);
		break;
	case FAVOUR_CDUP:
		msg->command = fm->fm_endpoint->ep_cdup(fm->fm_endpoint);
		break;
	case FAVOUR_MKD:
		msg->command = fm->fm_endpoint->ep_mkdir(fm->fm_endpoint, fm->fm_arg1);
		break;
	case FAVOUR_DELE:
		msg->command = fm->fm_endpoint->ep_dele(fm->fm_endpoint, fm->fm_arg1);
		break;
	case FAVOUR_RMD:
		msg->command = fm->fm_endpoint->ep_rmd(fm->fm_endpoint, fm->fm_arg1);
		break;

	case FAVOUR_PORT:
		msg->command = fm->fm_endpoint->ep_port(fm->fm_endpoint, fm->fm_arg1, (IPTR)fm->fm_arg2);
		break;
	case FAVOUR_PASV:
		msg->command = (IPTR)fm->fm_endpoint->ep_pasv(fm->fm_endpoint);
		break;
	case FAVOUR_REST:
		msg->command = fm->fm_endpoint->ep_rest(fm->fm_endpoint, (unsigned int)(IPTR)fm->fm_arg1);
		break;
	case FAVOUR_RETR:
		msg->command = fm->fm_endpoint->ep_retr(fm->fm_endpoint, fm->fm_arg1);
		break;
	case FAVOUR_STOR:
		msg->command = fm->fm_endpoint->ep_stor(fm->fm_endpoint, fm->fm_arg1);
		break;
	case FAVOUR_ABOR:
		msg->command = fm->fm_endpoint->ep_abor(fm->fm_endpoint);
		break;
	case FAVOUR_CHMOD:
		msg->command = fm->fm_endpoint->ep_chmod(fm->fm_endpoint, fm->fm_arg1, (IPTR)fm->fm_arg2);
		break;
	case FAVOUR_SELECT:
		msg->command = fm->fm_endpoint->ep_select(fm->fm_endpoint);
		break;
	case FAVOUR_GETREPLY:
		msg->command = fm->fm_endpoint->ep_getreply(fm->fm_endpoint, (IPTR)fm->fm_arg1);
		break;
	case FAVOUR_GETENTRY:
		msg->command = (IPTR)fm->fm_endpoint->ep_getentry(fm->fm_endpoint, fm->fm_arg1);
		break;
	case FAVOUR_ERRORREQ:
		msg->command = fm->fm_endpoint->ep_errorreq(fm->fm_endpoint, fm->fm_arg1, (IPTR)fm->fm_arg2);
		break;
		case FAVOUR_GET_FILE: {
			struct rec_favour_xfer *xfer = fm->fm_arg1;
			if (fm->fm_endpoint->ep_ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
			{
				fm->fm_endpoint->ep_ftpnode->fn_ftp.fi_aborted = 0;
				fm->fm_endpoint->ep_ftpnode->fn_ftp.fi_errno = 0;
				fm->fm_endpoint->ep_ftpnode->fn_ftp.fi_ioerr = 0;
				*fm->fm_endpoint->ep_ftpnode->fn_ftp.fi_serverr = 0;
				msg->command = ftp_sftp_get(&fm->fm_endpoint->ep_ftpnode->fn_sftp,
											xfer->updatefn,
											xfer->updateinfo,
											xfer->remote_path,
											xfer->local_path,
											xfer->restart);
				if (ftp_sftp_session_error(&fm->fm_endpoint->ep_ftpnode->fn_sftp) != FTP_SFTP_ERROR_NONE)
				{
					if (ftp_sftp_session_error(&fm->fm_endpoint->ep_ftpnode->fn_sftp) == FTP_SFTP_ERROR_ABORTED)
						fm->fm_endpoint->ep_ftpnode->fn_ftp.fi_aborted = 1;
					else
					{
						fm->fm_endpoint->ep_ftpnode->fn_ftp.fi_errno |= FTPERR_XFER_SRCERR;
						sprintf(fm->fm_endpoint->ep_ftpnode->fn_ftp.fi_serverr,
								"550 %s\r\n",
								ftp_sftp_error_message(&fm->fm_endpoint->ep_ftpnode->fn_sftp));
					}
				}
			}
			else
				msg->command = get(&fm->fm_endpoint->ep_ftpnode->fn_ftp,
								   xfer->updatefn,
								   xfer->updateinfo,
								   xfer->remote_path,
								   xfer->local_path,
								   xfer->restart);
			break;
		}
		case FAVOUR_PUT_FILE: {
			struct rec_favour_xfer *xfer = fm->fm_arg1;
			if (fm->fm_endpoint->ep_ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
			{
				fm->fm_endpoint->ep_ftpnode->fn_ftp.fi_aborted = 0;
				fm->fm_endpoint->ep_ftpnode->fn_ftp.fi_errno = 0;
				fm->fm_endpoint->ep_ftpnode->fn_ftp.fi_ioerr = 0;
				*fm->fm_endpoint->ep_ftpnode->fn_ftp.fi_serverr = 0;
				msg->command = ftp_sftp_put(&fm->fm_endpoint->ep_ftpnode->fn_sftp,
											xfer->updatefn,
											xfer->updateinfo,
											xfer->local_path,
											xfer->remote_path,
											xfer->restart);
				if (ftp_sftp_session_error(&fm->fm_endpoint->ep_ftpnode->fn_sftp) != FTP_SFTP_ERROR_NONE)
				{
					if (ftp_sftp_session_error(&fm->fm_endpoint->ep_ftpnode->fn_sftp) == FTP_SFTP_ERROR_ABORTED)
						fm->fm_endpoint->ep_ftpnode->fn_ftp.fi_aborted = 1;
					else
					{
						fm->fm_endpoint->ep_ftpnode->fn_ftp.fi_errno |= FTPERR_XFER_DSTERR;
						sprintf(fm->fm_endpoint->ep_ftpnode->fn_ftp.fi_serverr,
								"550 %s\r\n",
								ftp_sftp_error_message(&fm->fm_endpoint->ep_ftpnode->fn_sftp));
					}
				}
			}
			else
				msg->command = put(&fm->fm_endpoint->ep_ftpnode->fn_ftp,
								   xfer->updatefn,
								   xfer->updateinfo,
								   xfer->local_path,
								   xfer->remote_path,
								   xfer->restart);
			break;
		}
	default:
		D(bug("** unknown favour %ld\n", (long)fm->fm_ftp_command));
		msg->command = 0;
		break;
	}

	return 1;
}

/********************************/

//
//	Support for inline editing of lister fields
//
static void lister_edit(struct ftp_node *ftpnode, IPCMessage *msg)
{
	struct edit_msg *em;
	struct entry_info ei = {0};
	int rexx_result = 0;

	if (!ftpnode || !msg)
		return;

	em = msg->data_free;

	if (!strcmp(em->em_field, "name"))
	{
		if (ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
		{
			char *utf8_oldname = ftp_convert_path_to_utf8(em->em_entry, &ftpnode->fn_ftp);
			char *utf8_newname = ftp_convert_path_to_utf8(em->em_value, &ftpnode->fn_ftp);
			
			const char *send_oldname = utf8_oldname ? utf8_oldname : em->em_entry;
			const char *send_newname = utf8_newname ? utf8_newname : em->em_value;
			
			int rename_success = ftp_sftp_rename(&ftpnode->fn_sftp, send_oldname, send_newname);
			
			if (utf8_oldname) ftp_codesets_free(utf8_oldname);
			if (utf8_newname) ftp_codesets_free(utf8_newname);
			
			if (rename_success)
				rexx_result = 1;
		}
		else if (ftp_rename(&ftpnode->fn_ftp, em->em_entry, em->em_value) == 250)
		{
			rexx_result = 1;
		}

		if (rexx_result)
		{
			if (entry_info_from_lister(ftpnode, em->em_entry, &ei, 0))
			{
				// Set/clear hidden flag?
				if (ftpnode->fn_protocol == FTP_PROTOCOL_SFTP || ftpnode->fn_ftp.fi_flags & FTP_IS_UNIX)
				{
					if (em->em_value[0] == '.')
						ei.ei_prot |= FIBF_HIDDEN;
					else
						ei.ei_prot &= ~FIBF_HIDDEN;
				}

				rexx_lst_remove(ftpnode->fn_opus, ftpnode->fn_handle, em->em_entry);

				lister_add(ftpnode, em->em_value, ei.ei_size, ei.ei_type, ei.ei_seconds, ei.ei_prot, ei.ei_comment);

				ftplister_refresh(ftpnode, 0);
			}
		}
		else
		{
			if (ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
			{
				ftpnode->fn_ftp.fi_reply = 550;
				sprintf(ftpnode->fn_ftp.fi_iobuf, "550 %s\r\n", ftp_sftp_error_message(&ftpnode->fn_sftp));
			}
			lst_server_reply(ftpnode->fn_og, ftpnode, ftpnode, MSG_FTP_RENAME_ERROR);
		}
	}
	else if (!strcmp(em->em_field, "protect"))
	{
		ULONG mode = 0;
		LONG prot = FIBF_READ | FIBF_WRITE | FIBF_EXECUTE | FIBF_DELETE;

		if (em->em_value[4] == 'r')
		{
			mode |= 0444;
			prot &= ~FIBF_READ;
		}

		if (em->em_value[5] == 'w' && em->em_value[7] == 'd')
		{
			mode |= 0222;
			prot &= ~(FIBF_WRITE | FIBF_DELETE);
		}

		if (em->em_value[6] == 'e')
		{
			mode |= 0111;
			prot &= ~FIBF_EXECUTE;
		}

		if (ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
		{
			char *utf8_name = ftp_convert_path_to_utf8(em->em_entry, &ftpnode->fn_ftp);
			const char *send_name = utf8_name ? utf8_name : em->em_entry;
			
			int chmod_success = ftp_sftp_chmod(&ftpnode->fn_sftp, send_name, mode);
			
			if (utf8_name) ftp_codesets_free(utf8_name);
			
			if (chmod_success)
				rexx_result = 1;
		}
		else if (ftp_chmod(&ftpnode->fn_ftp, mode, em->em_entry) == 200)
		{
			rexx_result = 1;
		}

		if (rexx_result)
		{
			if (entry_info_from_lister(ftpnode, em->em_entry, &ei, 0))
			{
				// Don't change HSPA bits
				prot = (ei.ei_prot & 0xf0) | (prot & 0x0f);

				lister_add(ftpnode, ei.ei_name, ei.ei_size, ei.ei_type, ei.ei_seconds, prot, ei.ei_comment);
				ftplister_refresh(ftpnode, 0);
			}
		}
		else
		{
			if (ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
			{
				ftpnode->fn_ftp.fi_reply = 550;
				sprintf(ftpnode->fn_ftp.fi_iobuf, "550 %s\r\n", ftp_sftp_error_message(&ftpnode->fn_sftp));
			}
			lst_server_reply(ftpnode->fn_og, ftpnode, ftpnode, MSG_FTP_CHMOD_ERROR);
		}
	}
	else
		DisplayBeep(ftpnode->fn_og->og_screen);

	if (em->em_rxmsg)
		reply_rexx(em->em_rxmsg, rexx_result, 0);
}

/********************************/

//
//	Support for FindFile command
//
static void lister_findfile(struct ftp_node *ftpnode, IPCMessage *msg)
{
	struct findfile_msg *fm;
	char *names = NULL, *p;
	int entrycount;		  // Total number of entries to search
	int entrynumber = 1;  // Number of entry being searched
	int quiet;			  // Don't show confirmation requesters
	endpoint *source;
	char srcname[FILENAMELEN + 1];	// Source name of this entry
	struct entry_info ei;			// Info on this entry
	char path[PATHLEN + 1];			// Current path in lister
	int success;
	struct hook_rec_data hc = {0};
	int reqresult;
	static char pattern[FILENAMELEN + 1] = "";
	int patresult = -1;
	char fulldirname[FILENAMELEN + 1];
	int rexx_result = 0;

	// Valid?
	if (!ftpnode || !msg || !msg->data_free)
		return;

	// Get delete message
	fm = msg->data_free;

	// Make lister busy
	rexx_lst_lock(ftpnode->fn_opus, ftpnode->fn_handle);

	// Clear abort flag
	ftpnode->fn_flags &= ~LST_ABORT;

	// Got entries to search?
	if ((names = fm->fm_names))
	{
		entrycount = count_entries(names);

		quiet = 0;

		// Create endpoint
		if ((source = create_endpoint_tags(ftpnode->fn_og, EP_TYPE, ENDPOINT_FTP, EP_FTPNODE, ftpnode, TAG_DONE)))
		{
			// Get path
			lister_get_path(ftpnode, path);
			strcpy(fulldirname, path);

			// Requester
			if ((reqresult = lister_request_tags(ftpnode,
												 FR_MsgNum,
												 MSG_FTP_ENTER_PATTERN,
												 AR_CheckMark,
												 GetString(locale, MSG_FTP_SEARCH_COMMENTS),
												 AR_CheckPtr,
												 &hc.hc_match_comment,
												 AR_Flags,
												 SRF_CHECKMARK,
												 FR_ButtonNum,
												 MSG_FTP_OKAY,
												 FR_ButtonNum,
												 MSG_FTP_CANCEL,
												 AR_Buffer,
												 pattern,
												 AR_BufLen,
												 FILENAMELEN + 1,
												 TAG_DONE) &&
							 *pattern))
			{
				if (ftpnode->fn_ftp.fi_flags & FTP_IS_UNIX)
					patresult = ParsePattern(pattern, hc.hc_pattern, FILENAMELEN * 2 + 1);
				else
					patresult = ParsePatternNoCase(pattern, hc.hc_pattern, FILENAMELEN * 2 + 1);
			}

			if (patresult != -1)
			{
				// Show progress bar
				lister_prog_init(ftpnode,
								 GetString(locale, MSG_FTP_SCANNING_DIRS),	// Title
								 0,											// Info
								 "",										// Name
								 PROGRESS_FILE_OFF,							// File
								 PROGRESS_BAR_ON);							// Bar

				// Set up getsizes hook data
				hc.hc_og = ftpnode->fn_og;
				hc.hc_source = source;
				hc.hc_pre = 0;
				hc.hc_opus = ftpnode->fn_opus;
				hc.hc_prognode = ftpnode;
				hc.hc_proghandle = ftpnode->fn_handle;
				hc.hc_ored_ftp_flags = ftpnode->fn_ftp.fi_flags;

				if (ftpnode->fn_site.se_env->e_recursive_special)
					hc.hc_recur_flags |= RECURF_BROKEN_LS;

				// Should use callbacks to get entries
				// (except rmb popup coz they won't be highlit)
				for (p = names + 1; !(ftpnode->fn_flags & LST_ABORT); p += 3)
				{
					// Next source entry name
					p = stptok(p, srcname, FILENAMELEN, "\"\r\n");

					rexx_result = 0;

					// Update progress bar
					lister_prog_bar(ftpnode, entrycount, entrynumber++);

					lister_prog_name(ftpnode, srcname);

					// Get file info
					if (entry_info_from_lister(ftpnode, srcname, &ei, ENTRYFROMF_DEFAULT | ENTRYFROMF_DEFDIR))
					{
						int confirm_entry = 1;

						if (confirm_entry == 2)
							quiet = 1;

						if (confirm_entry == 1 || confirm_entry == 2)
						{
							//
							if ((success = recursive_findfile(&hc, fulldirname, &ei)))
							{
								if (success > 2)
									rexx_result = 1;

								// lister_add( ftpnode,
								rexx_lst_add(ftpnode->fn_opus,
											 ftpnode->fn_handle,
											 ei.ei_name,
											 hc.hc_misc_bytes,
											 ei.ei_type,
											 ei.ei_seconds,
											 ei.ei_prot,
											 ei.ei_comment);
							}

							if (success == 3)
							{
								struct ftp_msg *fm;
								char *pattern;

								if ((fm = AllocVec(sizeof(struct ftp_msg) + strlen(hc.hc_resultpath) + 1, MEMF_CLEAR)))
								{
									fm->fm_names = (char *)(fm + 1);
									strcpy(fm->fm_names, hc.hc_resultpath);

									IPC_Command(ftpnode->fn_ipc, IPC_CD, 0, 0, fm, 0);

									if ((pattern = AllocVec(strlen(hc.hc_pattern) + 1, MEMF_ANY)))
									{
										strcpy(pattern, hc.hc_pattern);
										IPC_Command(ftpnode->fn_ipc, IPC_SELECTPATTERN, 0, 0, pattern, 0);
									}
								}
							}
						}

						// Abort hit on confirm requester?
						else if (confirm_entry == 0)
							ftpnode->fn_flags |= LST_ABORT;
					}

					// Done all entries?
					if (*(p + 1) == 0)
						break;
				}
				// Remove progress bar
				lister_prog_clear(ftpnode);

				// Show updated contents
				rexx_lst_refresh(ftpnode->fn_opus, ftpnode->fn_handle, 0);
			}

			delete_endpoint(source);
		}
	}
	// Make lister non-busy
	rexx_lst_unlock(ftpnode->fn_opus, ftpnode->fn_handle);

	if (fm->fm_rxmsg)
		reply_rexx(fm->fm_rxmsg, rexx_result, 0);
}

/********************************/

//
//	Support for GetSizes command
//
static void lister_getsizes(struct ftp_node *ftpnode, IPCMessage *msg)
{
	struct getsizes_msg *gm;
	char *names = NULL, *p;
	int entrycount;		  // Total number of entries to delete
	int entrynumber = 1;  // Number of entry being deleted
	int dircount = 0;
	int quiet;	// Don't show confirmation requesters
	endpoint *source;
	char srcname[FILENAMELEN + 1];	// Source name of this entry
	struct entry_info ei;			// Info on this entry
	char path[PATHLEN + 1];			// Current path in lister
	int success;
	struct hook_rec_data hc = {0};
	int rexx_result = 0;

	// Valid?
	if (!ftpnode || !msg || !msg->data_free)
		return;

	// Get delete message
	gm = msg->data_free;

	// Make lister busy
	rexx_lst_lock(ftpnode->fn_opus, ftpnode->fn_handle);

	// Clear abort flag
	ftpnode->fn_flags &= ~LST_ABORT;

	// Got entries to delete?
	if ((names = gm->gs_names))
	{
		entrycount = count_entries(names);

		quiet = 0;

		// Create endpoint
		if ((source = create_endpoint_tags(ftpnode->fn_og, EP_TYPE, ENDPOINT_FTP, EP_FTPNODE, ftpnode, TAG_DONE)))
		{
			dircount = entrycount;

			// Get path
			lister_get_path(ftpnode, path);

			// Show progress bar
			lister_prog_init(ftpnode,
							 GetString(locale, MSG_FTP_SCANNING_DIRS),	// Title
							 0,											// Info
							 "",										// Name
							 PROGRESS_FILE_OFF,							// File
							 PROGRESS_BAR_ON);							// Bar

			// Set up getsizes hook data
			hc.hc_og = ftpnode->fn_og;
			hc.hc_source = source;
			hc.hc_pre = 0;
			hc.hc_opus = ftpnode->fn_opus;
			hc.hc_prognode = ftpnode;
			hc.hc_proghandle = ftpnode->fn_handle;

			if (ftpnode->fn_site.se_env->e_recursive_special)
				hc.hc_recur_flags |= RECURF_BROKEN_LS;

			// Should use callbacks to get entries
			// (except rmb popup coz they won't be highlit)
			for (p = names + 1; !(ftpnode->fn_flags & LST_ABORT); p += 3)
			{
				// Next source entry name
				p = stptok(p, srcname, FILENAMELEN, "\"\r\n");

				rexx_result = 0;

				// Update progress bar
				lister_prog_bar(ftpnode, entrycount, entrynumber++);
				lister_prog_name(ftpnode, srcname);

				// Get file info
				if (entry_info_from_lister(ftpnode, srcname, &ei, ENTRYFROMF_DEFAULT | ENTRYFROMF_DEFDIR))
				{
					int confirm_entry = 1;

					if (confirm_entry == 2)
						quiet = 1;

					if (confirm_entry == 1 || confirm_entry == 2)
					{
						hc.hc_misc_bytes = 0;

						// Get entry's size (and contents if it's a dir)
						if ((success = recursive_getsizes(&hc, &ei)))
						{
							if (success > 1)
								rexx_result = 1;

							// ilster_add( ftpnode,
							rexx_lst_add(ftpnode->fn_opus,
										 ftpnode->fn_handle,
										 ei.ei_name,
										 hc.hc_misc_bytes,
										 ei.ei_type,
										 ei.ei_seconds,
										 ei.ei_prot,
										 ei.ei_comment);
						}
					}

					// Abort hit on confirm requester?
					else if (confirm_entry == 0)
						ftpnode->fn_flags |= LST_ABORT;
				}

				// Done all entries?
				if (*(p + 1) == 0)
					break;
			}
			// Remove progress bar
			lister_prog_clear(ftpnode);

			// Show updated contents
			rexx_lst_refresh(ftpnode->fn_opus, ftpnode->fn_handle, 0);

			delete_endpoint(source);
		}
	}
	// Make lister non-busy
	rexx_lst_unlock(ftpnode->fn_opus, ftpnode->fn_handle);

	if (gm->gs_rxmsg)
		reply_rexx(gm->gs_rxmsg, rexx_result, 0);
}

/********************************/

//
//	Snapshot lister
//
static void lister_snapshot(struct ftp_node *ftpnode, IPCMessage *msg)
{
	D(bug("lister_snapshot()\n"));

	if (msg->data)
		reply_rexx(msg->data, 0, 0);
}

/********************************/

//
//	Unsnapshot lister
//
static void lister_unsnapshot(struct ftp_node *ftpnode, IPCMessage *msg)
{
	D(bug("lister_unsnapshot()\n"));

	if (msg->data)
		reply_rexx(msg->data, 0, 0);
}

/********************************/

//
//	Add lister's connection to address book
//
static void lister_ftpadd(struct ftp_node *ftpnode, IPCMessage *msg)
{
	struct connect_msg *sm;

	if ((sm = AllocVec(sizeof(*sm), MEMF_CLEAR)))
	{
		copy_site_entry(ftpnode->fn_og, &sm->cm_site, &ftpnode->fn_site);

		if (IPC_Command(ftpnode->fn_og->og_addrproc, IPC_ADD, 1, sm, 0, REPLY_NO_PORT))
			copy_site_entry(ftpnode->fn_og, &ftpnode->fn_site, &sm->cm_site);

		FreeVec(sm);
	}
}

/********************************/

//
//	Show "List Format" requester
//
static void ftplister_configure(struct ftp_node *ftpnode, IPCMessage *msg)
{
	struct connect_msg *sm;
	struct ftp_msg *fm;
	int retval;

	if ((sm = AllocVec(sizeof(struct connect_msg), MEMF_CLEAR)))
	{
		sm->cm_window = ((Lister *)ftpnode->fn_handle)->window;

		// Copy the Opus lister's format into ours
		ftplister_read_listformat(ftpnode);

		// mark so to NOT overwrite current format
		ftpnode->fn_site.se_preserve_format = TRUE;

		// Copy entire site details into message
		copy_site_entry(ftpnode->fn_og, &sm->cm_site, &ftpnode->fn_site);

		// Tell Opus lister there'll be a lister format editor
		IPC_Command(((Lister *)ftpnode->fn_handle)->ipc, LISTER_CONFIGURE, 1, 0, 0, 0);

		if ((retval = IPC_Command(ftpnode->fn_og->og_addrproc, IPC_CONFIGURE, 1, sm, 0, REPLY_NO_PORT)))
			copy_site_entry(ftpnode->fn_og, &ftpnode->fn_site, &sm->cm_site);

		// Send new format to Opus lister - will de-busy the lister too
		ftplister_write_listformat(ftpnode, retval ? 1 : 0);

		// Clear mark for current format
		ftpnode->fn_site.se_preserve_format = FALSE;

		FreeVec(sm);
	}

	if (msg)
	{
		if ((fm = msg->data_free))
			reply_rexx(fm->fm_rxmsg, 0, 0);
	}
}

/********************************/

//
//	Select all files matching a pattern
//
static void ftplister_select_pattern(struct ftp_node *ftpnode, IPCMessage *msg)
{
	char *names;
	char *p;
	char name[FILENAMELEN + 1];
	struct entry_info ei;
	IPTR result;

	if (!ftpnode || !msg || !msg->data_free)
		return;

	result = send_rexxa(ftpnode->fn_opus,
						REXX_REPLY_RESULT,
						"lister query " FTP_HANDLE_PRINTF " entries",
						FTP_HANDLE_VALUE(ftpnode->fn_handle));
	if (result && result != (IPTR)-1)
	{
		names = (char *)result;

		rexx_lst_lock(ftpnode->fn_opus, ftpnode->fn_handle);

		for (p = names + 1;; p += 3)
		{
			// Next source entry name
			p = stptok(p, name, FILENAMELEN, "\"");

			if (entry_info_from_lister(ftpnode, name, &ei, 0))
			{
				if (ei.ei_type < 0 && casematchpattern(ftpnode->fn_ftp.fi_flags, msg->data_free, name))
				{
					rexx_lst_select(ftpnode->fn_opus, ftpnode->fn_handle, name, 1);
				}
			}

			// Done all entries?
			if (*(p + 1) == 0)
				break;
		}

		ftplister_refresh(ftpnode, 0);

		DeleteArgstring(names);

		rexx_lst_unlock(ftpnode->fn_opus, ftpnode->fn_handle);
	}
}

/********************************/

//
//	Edit site-specific options
//
static void lister_ftpoptions(struct ftp_node *ftpnode, IPCMessage *msg)
{
	struct connect_msg *sm;

	if ((sm = AllocVec(sizeof(*sm), MEMF_CLEAR)))
	{
		sm->cm_window = ((Lister *)ftpnode->fn_handle)->window;

		// Copy the Opus lister's format into ours
		ftplister_read_listformat(ftpnode);

		// mark so to NOT overwrite current format
		ftpnode->fn_site.se_preserve_format = TRUE;

		// Copy entire site details into message
		copy_site_entry(ftpnode->fn_og, &sm->cm_site, &ftpnode->fn_site);

		if (IPC_Command(ftpnode->fn_og->og_addrproc, IPC_OPTIONS, 1, sm, 0, REPLY_NO_PORT))
		{
			copy_site_entry(ftpnode->fn_og, &ftpnode->fn_site, &sm->cm_site);
			ftplister_write_listformat(ftpnode, 1);
		}

		// Clear mark for current format
		ftpnode->fn_site.se_preserve_format = FALSE;

		FreeVec(sm);
	}
}

/********************************/

//
//	Hook called before commencing protection of each entry
//
static int hook_protect_pre(struct hook_rec_data *hc,
							char *unused1,
							struct rec_entry_list *unused2,
							struct entry_info *entry)
{
	// Valid?
	if (!hc)
		return 0;

	// Update progress bar name
	if (hc->hc_prognode && entry)
		lister_prog_info(hc->hc_prognode, entry->ei_name);

	return 1;
}

//
//	Support for Protect command
//
static void lister_protect(struct ftp_node *ftpnode, IPCMessage *msg)
{
	struct protect_msg *pm;
	char *names = NULL, *p;
	int entrycount;	 // Total number of entries to delete
	int filecount = 0, dircount = 0;
	int quiet = 0;	// Don't show confirmation requesters
	endpoint *source;
	char srcname[FILENAMELEN + 1];	// Source name of this entry
	struct entry_info ei;			// Info on this entry
	int confirm_commence = 0;		// Commence deleting confirmed?
	char path[PATHLEN + 1];			// Current path in lister
	int success;
	struct hook_rec_data hc = {0};
	struct protectgui_msg info = {0};
	int rexx_result = 0;

	// Valid?
	if (!ftpnode || !msg || !msg->data_free)
		return;

	// Get delete message
	pm = msg->data_free;

	// Make lister busy
	rexx_lst_lock(ftpnode->fn_opus, ftpnode->fn_handle);

	// Clear abort flag
	ftpnode->fn_flags &= ~LST_ABORT;

	// Got entries to delete?
	if ((names = pm->pm_names))
	{
		entrycount = count_entries(names);

		// Set flags on command line?
		if (pm->pm_flags & PROT_OPT_SET)
		{
			info.pm_set_mask = prot_amiga_to_unix(pm->pm_set_mask);
			quiet = 1;
		}

		// Clear flags on command line?
		if (pm->pm_flags & PROT_OPT_CLEAR)
		{
			info.pm_clear_mask = prot_amiga_to_unix(pm->pm_clear_mask);
			quiet = 1;
		}

		// Create endpoint
		if ((source = create_endpoint_tags(ftpnode->fn_og, EP_TYPE, ENDPOINT_FTP, EP_FTPNODE, ftpnode, TAG_DONE)))
		{
			// Count files and directories

			// Only one entry?  Could be from RMB popup or command line
			if (entrycount == 1)
			{
				// Strip quotes
				stptok(names + 1, srcname, FILENAMELEN, "\"\r\n");

				// Does entry contain path information?
				if (strpbrk(srcname, ":/"))
					D(bug("** entry contains path information\n"));

				if (entry_info_from_lister(ftpnode, srcname, &ei, ENTRYFROMF_DEFAULT))
				{
					if (ei.ei_type >= 0)
						dircount = 1;
					else
						filecount = 1;
				}
				else
					D(bug("** entry not in lister - should use special delete\n"));
			}

			// Multiple entries.  Query using callbacks.
			else
			{
				filecount = rexx_lst_query_numselfiles(ftpnode->fn_opus, ftpnode->fn_handle);
				dircount = rexx_lst_query_numseldirs(ftpnode->fn_opus, ftpnode->fn_handle);
			}

			// Show subdirectory requester?
			if (dircount)
			{
				if (pm->pm_flags & (PROT_OPT_RECURSE | PROT_OPT_SET | PROT_OPT_CLEAR))
					confirm_commence = 1;
				else
					confirm_commence = lister_request_tags(ftpnode,
														   FR_FormatString,
														   "%s %s",
														   FR_MsgNum,
														   MSG_ACT_ON,
														   FR_MsgNum,
														   MSG_FILES_IN_SUBDIRS,
														   FR_ButtonNum,
														   MSG_FTP_YES,
														   FR_ButtonNum,
														   MSG_FTP_NO,
														   FR_ButtonNum,
														   MSG_FTP_CANCEL,
														   TAG_DONE);
			}
			else
				confirm_commence = 2;

			if (confirm_commence)
			{
				// Get path
				lister_get_path(ftpnode, path);

				// Show progress bar
				lister_prog_init(ftpnode,
								 GetString(locale, MSG_FTP_SETTING_PROT),  // Title
								 "",									   // Info
								 0,										   // Name
								 PROGRESS_FILE_OFF,						   // File
								 PROGRESS_BAR_OFF);						   // Bar

				// Set up protect hook data
				hc.hc_og = ftpnode->fn_og;
				hc.hc_source = source;
				hc.hc_pre = hook_protect_pre;
				hc.hc_opus = ftpnode->fn_opus;
				hc.hc_prognode = ftpnode;
				hc.hc_proghandle = ftpnode->fn_handle;
				if (confirm_commence == 2)
					hc.hc_recur_flags |= RECURF_NORECUR;
				if (ftpnode->fn_site.se_env->e_recursive_special)
					hc.hc_recur_flags |= RECURF_BROKEN_LS;

				// Should use callbacks to get entries
				// (except rmb popup coz they won't be highlit)
				for (p = names + 1; !(ftpnode->fn_flags & LST_ABORT); p += 3)
				{
					// Next source entry name
					p = stptok(p, srcname, FILENAMELEN, "\"\r\n");

					rexx_result = 0;

					// Update progress bar
					lister_prog_name(ftpnode, srcname);

					// Get file info
					if (entry_info_from_lister(ftpnode, srcname, &ei, ENTRYFROMF_DEFAULT))
					{
						int confirm_entry = 1;

						// Show protect requester?
						if (!quiet)
						{
							info.pm_window = ((Lister *)ftpnode->fn_handle)->window;
							info.pm_name = ei.ei_name;
							info.pm_current = ei.ei_unixprot;
							confirm_entry = function_change_get_protect(ftpnode->fn_og, &info);
						}

						// All?
						if (confirm_entry == 2)
							quiet = 1;

						// Not skip?
						if (confirm_entry > 0)
						{
							// Delete entry (and contents if it's a dir)
							if ((success = recursive_protect(&hc, &ei, info.pm_set_mask, info.pm_clear_mask) == 2))
							{
								if (success > 1)
									rexx_result = 1;

								lister_add(ftpnode,
										   ei.ei_name,
										   ei.ei_size,
										   ei.ei_type,
										   ei.ei_seconds,
										   ei.ei_prot,
										   ei.ei_comment);
							}
						}

						// Abort hit on confirm requester?
						else if (confirm_entry == -1)
							ftpnode->fn_flags |= LST_ABORT;
					}

					// Done all entries?
					if (*(p + 1) == 0)
						break;
				}
				// Remove progress bar
				lister_prog_clear(ftpnode);

				// Show updated contents
				rexx_lst_refresh(ftpnode->fn_opus, ftpnode->fn_handle, 0);
			}

			delete_endpoint(source);
		}
	}
	// Make lister non-busy
	rexx_lst_unlock(ftpnode->fn_opus, ftpnode->fn_handle);

	if (pm->pm_rxmsg)
		reply_rexx(pm->pm_rxmsg, rexx_result, 0);
}

static int do_noop(struct ftp_info *info)
{
	return (ftp(info, "NOOP\r\n"));
}

static int lister_alive(struct ftp_node *ftpnode, IPCMessage *msg)
{
	if (do_noop(&ftpnode->fn_ftp) == 421)  // connection closed by remote
	{
		lister_request_tags(ftpnode, FR_MsgNum, MSG_FTP_SERVER_CLOSED_CONN, TAG_DONE);
		return FALSE;
	}
	return TRUE;
}

//
// check if the connection is still alive
// by sending a noop to the other (or each) end
//
static BOOL network_available(struct ftp_node *ftpnode, IPCMessage *msg)
{
	BOOL result = FALSE;

	D(bug("check network\n"));

	while (1)  // for colin :)
	{
		// Skip the keep-alive check if the user disabled it in the FTP options
		// (was driven by the DOpus/NoBeeGees env var; now an addressbook flag).

		if (!ftpnode->fn_og->og_oc.oc_no_keep_alive)
		{
			// if getput then check destination connection first

			if (msg->command == IPC_GETPUT)
			{
				struct xfer_msg *xm = msg->data_free;

				D(bug("check network getput\n"));

				// safety if no handles
				if (!xm->xm_rm_src->fn_handle || !xm->xm_rm_dest->fn_handle)
					break;

				// Only the source lister should process this
				if (ftpnode != xm->xm_rm_src)
					break;

				// if alread have stay alive noops then no need to check

				if (!xm->xm_rm_dest->fn_site.se_env->e_noops)
				{
					// Tell other lister in site to sitee to send NOOP
					// if not return 0 then it failed so just return

					if (!IPC_Command(xm->xm_rm_dest->fn_ipc, IPC_ALIVE, 0, 0, 0, REPLY_NO_PORT))
						break;
				}
			}

			// if not in keep alive mode then
			// send noop for this connection to check if alive

			if (!ftpnode->fn_site.se_env->e_noops)
			{
				D(bug("check network this site \n"));

				if (do_noop(&ftpnode->fn_ftp) == 421)  // connection closed by remote
				{
					lister_request_tags(ftpnode, FR_MsgNum, MSG_FTP_SERVER_CLOSED_CONN, TAG_DONE);
					break;
				}
			}
		}

		result = TRUE;
		break;
	}

	return result;
}

/*}******************************/
/*** Message loop and related ***/
/********************************/

struct lister_msg_info
{
	int lmi_command;
	void (*lmi_function)(struct ftp_node *, IPCMessage *);
	int lmi_returns;
	int lmi_needs_network;
};

static struct lister_msg_info lister_msg_table[] = {
	IPC_DOUBLECLICK,
	lister_doubleclick,
	0,
	1,
	IPC_TRAPTEMP,
	lister_traptemp,
	0,
	1,
	IPC_GET,
	lister_xfer,
	0,
	1,
	IPC_PUT,
	lister_xfer,
	0,
	1,
	IPC_GETPUT,
	lister_getput,
	0,
	1,
	IPC_MAKEDIR,
	lister_makedir,
	0,
	1,
	IPC_RENAME,
	lister_rename,
	0,
	1,
	IPC_DELETE,
	lister_delete,
	0,
	1,
	IPC_REREAD,
	lister_reread,
	0,
	0,
	IPC_PARENT,
	lister_parent,
	0,
	0,
	IPC_ROOT,
	lister_root,
	0,
	0,
	IPC_CD,
	lister_path,
	0,
	0,
	IPC_CDCACHE,
	lister_cache_path,
	0,
	0,
	IPC_FTPCOMMAND,
	(void (*)(struct ftp_node *, IPCMessage *))lister_cmd,
	1,
	0,
	IPC_SETVAR,
	(void (*)(struct ftp_node *, IPCMessage *))lister_setvar,
	1,
	0,
	IPC_FAVOUR,
	(void (*)(struct ftp_node *, IPCMessage *))lister_favour,
	1,
	0,
	IPC_EDIT,
	lister_edit,
	0,
	1,
	IPC_FINDFILE,
	lister_findfile,
	0,
	1,
	IPC_GETSIZES,
	lister_getsizes,
	0,
	1,
	IPC_PROTECT,
	lister_protect,
	0,
	1,
	IPC_SNAPSHOT,
	lister_snapshot,
	0,
	0,
	IPC_UNSNAPSHOT,
	lister_unsnapshot,
	0,
	0,
	IPC_ADD,
	lister_ftpadd,
	0,
	0,
	IPC_OPTIONS,
	lister_ftpoptions,
	0,
	0,
	IPC_CONFIGURE,
	ftplister_configure,
	0,
	0,
	IPC_SELECTPATTERN,
	ftplister_select_pattern,
	0,
	0,
	IPC_ALIVE,
	(void (*)(struct ftp_node *, IPCMessage *))lister_alive,
	1,
	0,
	0,
};

//
//	Returns 1 if msg->command is set to meaningful result
//	Returns 0 if msg->command has not been set
//
static int lister_msg_switch(struct opusftp_globals *og, struct ftp_node *ftpnode, IPCMessage *msg)
{
	struct lister_msg_info *lmi;
	int result = 0;
	int found = 0;

	D(bug("lister_msg_switch()\n"));

	for (lmi = lister_msg_table; lmi->lmi_command; ++lmi)
	{
		if (lmi->lmi_command == msg->command)
		{
			found = 1;

			// if passive mode then we must check if connection is alive
			// and has not timed out
			// also check if has auto reconnect enabled. This gives user a
			// lost connection requester as indication of failure
			// always check on getput since lost connection will cause invalid listers and hits

			if (lmi->lmi_needs_network)
			{
				if (msg->command == IPC_GETPUT || ftpnode->fn_site.se_env->e_passive ||
					ftpnode->fn_site.se_env->e_retry_lost)
				{
					D(bug("check alive\n"));

					if (!network_available(ftpnode, msg))
					{
						// check if arexx message and reply to
						// tidy up and unlock lister etc
						struct xfer_msg *xm = msg->data_free;

						if (xm->xm_rxmsg)
							reply_rexx(xm->xm_rxmsg, 0, 0);
						break;
					}
				}
			}

			if (lmi->lmi_returns)
				result = ((int (*)(struct ftp_node *, IPCMessage *))lmi->lmi_function)(ftpnode, msg);
			else
				lmi->lmi_function(ftpnode, msg);

			break;
		}
	}

	if (!found)
		D(bug("lister: unknown IPC msg %lx\n", msg->command));

	return result;
}

/********************************/

static int send_noop(struct opusftp_globals *og, struct ftp_node *ftpnode)
{
	int reply;
	int result = FALSE;

	D(bug("send_noop()\n"));

	// If user has option turned on
	// if	(ftpnode->fn_site.se_env->e_noops)
	{
		ftpnode->fn_ftp.fi_doing_noop = TRUE;

		reply = do_noop(&ftpnode->fn_ftp);

		if (reply == 421)
		{
			// lister_request_tags(
			//	ftpnode,
			//	FR_MsgNum,	MSG_FTP_SERVER_CLOSED_CONN,
			//	TAG_DONE );

			// Quit if not OK response ?
			ftpnode->fn_flags |= LST_NOOP_QUIT;
			result = TRUE;
		}
		else if (reply != 200)
		{
			// lst_server_reply( og, ftpnode, ftpnode, 0 );

			// Quit if not OK response ?
			ftpnode->fn_flags |= LST_NOOP_QUIT;
			result = TRUE;
		}

		// Clear marker to not show output
		ftpnode->fn_ftp.fi_doing_noop = FALSE;
	}

	return result;
}

/********************************/

/*
 *	There are several ways a lister process can end:
 *	  Initial connection to server fails
 *	  User closes the lister             - We must close FTP and close lister
 *	  User clicks on 'devices', 'cache'  - We must close FTP and send command
 *	  FTP command returns a 421 response - We must shutdown FTP and close lister
 *	  Master process sends IPC_QUIT      - We must abort any connect or transfer, close FTP, and close lister
 *	  Control-C signal is received       - ( as above )
 *
 *	GJP 19.2.97 Added timeout timer to keep link active - send NOOPs
 */

#define NOOP_TIME 30

static void lister_msg_loop(struct opusftp_globals *og, struct msg_loop_data *mld)
{
	IPCMessage *imsg;
	ULONG sigbits;
	struct TimerHandle *timer;
	int got_event = FALSE;

	if (!(timer = AllocTimer(UNIT_VBLANK, 0)))
	{
		DisplayBeep(og->og_screen);
		return;
	}

	// start 30 second timer
	StartTimer(timer, NOOP_TIME, 0);

	while (!mld->mld_done && *mld->mld_ftpreply != 421)
	{
		sigbits = Wait(SIGBREAKF_CTRL_C | (1 << mld->mld_ipc->command_port->mp_SigBit) | (1 << timer->port->mp_SigBit));

		// Get Control C from AmiTCP then quit
		if (sigbits & SIGBREAKF_CTRL_C)
		{
			D(bug("*** lister received SIGBREAKF_CTRL_C ***\n"));
			break;
		}

		if (sigbits & (1 << mld->mld_ipc->command_port->mp_SigBit))
		{
			got_event = TRUE;

			while ((imsg = (IPCMessage *)GetMsg(mld->mld_ipc->command_port)))
			{
				switch (imsg->command)
				{
				case IPC_QUIT:
					D(bug("*** lister received IPC_QUIT ***\n"));

					// Has lister already been closed?
					if (mld->mld_node && imsg->flags == (IPTR)-1)
						mld->mld_node->fn_flags &= ~LST_OPENED;

					// Leave lister open?
					else if (mld->mld_node && imsg->flags)
						mld->mld_node->fn_flags |= LST_LEAVEOPEN;

					// Don't reply quit msg until we're finished
					mld->mld_quitmsg = imsg;
					imsg = 0;
					break;

				case IPC_CONNECT:
					mld->mld_node = lister_new_connection(og, mld, imsg);

					// Don't reply here - called function will handle it
					imsg = 0;

					if (mld->mld_node)
						// Set FTP reply pointer so we can watch for 421s
						mld->mld_ftpreply = &mld->mld_node->fn_ftp.fi_reply;
					else
						// Connect failed - terminate process
						mld->mld_done = TRUE;
					break;

				default:
					// Set result if not set already
					if (!lister_msg_switch(og, mld->mld_node, imsg))
						imsg->command = FALSE;
					break;
				}

				if (imsg)
					IPC_Reply(imsg);

				if (mld->mld_quitmsg)
					break;
			}

			if (mld->mld_quitmsg)
				mld->mld_done = TRUE;
		}

		if (!mld->mld_done && (sigbits & (1 << timer->port->mp_SigBit)))
		{
			// If got any event within timeout period then don't send noop
			if (got_event)
				got_event = FALSE;

			// Send NOOP to keep connection?
			else if (mld->mld_node->fn_site.se_env->e_noops)
			{
				// if this returns TRUE then error of some sort. shut down!
				mld->mld_done = send_noop(og, mld->mld_node);
			}

			StartTimer(timer, NOOP_TIME, 0);
		}
	}

	// If connection was lost, try to reconnect
	if (*mld->mld_ftpreply == 421)
		lister_reconnect(og, mld);

	if (timer)
		FreeTimer(timer);
}

/*******************************}*/

//
//	This is the main lister routine.
//	Start up lister process.
//	Call main loop.
//	Disconnect (if we were connected).
//	End lister process.
//
void lister(void)
{
	struct opusftp_globals *og;
	// struct Library         *DOpusBase;
	int zero = 0;
	struct subproc_data *data;	// This is passed from the creator process
	struct msg_loop_data mld;

	/*if	((DOpusBase = OpenLibrary( "dopus5.library", VERSION_DOPUSLIB )))
		{
		#ifdef __amigaos4__
		IDOpus = (struct DOpusIFace *)GetInterface(DOpusBase, "main", 1, NULL);
		#endif*/

	// This returns true if 'data' is filled in correctly
	if (IPC_ProcStartup((IPTR *)&data, (APTR)&ftplister_init))
	{
		og = data->spd_ogp;
		mld.mld_ipc = data->spd_ipc;
		mld.mld_node = NULL;
		mld.mld_ftpreply = &zero;
		mld.mld_quitmsg = NULL;
		mld.mld_quit_command = NULL;
		mld.mld_done = FALSE;
		mld.mld_reconnecting = FALSE;

		lister_msg_loop(og, &mld);

		// We have a lister here (but it may be closed)
		if (mld.mld_node)
			lister_disconnect(og, &mld);

		// Clean up quit message
		if (mld.mld_quitmsg)
		{
			// Our quit message structure attached?
			if (mld.mld_quitmsg->data)
			{
				// ARexx message?
				if (((struct quit_msg *)mld.mld_quitmsg->data)->qm_rxmsg)
					reply_rexx(((struct quit_msg *)mld.mld_quitmsg->data)->qm_rxmsg, 1, 0);

				FreeVec(mld.mld_quitmsg->data);
			}

			mld.mld_quitmsg->command = TRUE;
			IPC_Reply(mld.mld_quitmsg);
		}

		IPC_Flush(data->spd_ipc);
		IPC_Goodbye(data->spd_ipc, data->spd_owner_ipc, 0);
		ftplister_cleanup(og, data->spd_ipc);
		IPC_Free(data->spd_ipc);
		FreeVec(data);
	}

	/*#ifdef __amigaos4__
	DropInterface((struct Interface *)IDOpus);
	#endif
	CloseLibrary(DOpusBase);

	}*/
}

/********************************/

static void lister_get_prog_stuff(struct ftp_node *node, IPTR *handle, int *type)
{
	if (!node || !handle || !type)
	{
		D(bug("** get prog stuff invalid args!\n"));
		D(bug("node:   0x%lx\n", (unsigned long)node));
		D(bug("handle: 0x%lx\n", (unsigned long)handle));
		D(bug("type:   0x%lx\n", (unsigned long)type));

		if (!node)
			return;
	}

	if (node->fn_flags & LST_INVISIBLE)
	{
		*handle = node->fn_proghandle;
		*type = PROGRESS_FREE;
	}
	else
	{
		*handle = node->fn_handle;
		*type = PROGRESS_LISTER;
	}
}

/********************************/

void lister_prog_bar(struct ftp_node *node, int total, int count)
{
	IPTR handle;
	int type;

	lister_get_prog_stuff(node, &handle, &type);

	rexx_prog_bar(node->fn_opus, handle, type, total, count);
}

/********************************/

void lister_prog_bytes(struct ftp_node *node, int total, int count)
{
	IPTR handle;
	int type;

	lister_get_prog_stuff(node, &handle, &type);

	rexx_prog_bytes(node->fn_opus, handle, type, total, count);
}

/********************************/

void lister_prog_clear(struct ftp_node *node)
{
	IPTR handle;
	int type;

	D(bug("Clear progress\n"));

	lister_get_prog_stuff(node, &handle, &type);

	rexx_prog_clear(node->fn_opus, handle, type);

	if (type == PROGRESS_FREE)
		node->fn_proghandle = 0;
}

/********************************/

void lister_prog_info(struct ftp_node *node, char *info)
{
	IPTR handle;
	int type;

	lister_get_prog_stuff(node, &handle, &type);

	rexx_prog_info(node->fn_opus, handle, type, info);
}

void lister_prog_info2(struct ftp_node *node, char *info2)
{
	IPTR handle;
	int type;

	lister_get_prog_stuff(node, &handle, &type);

	rexx_prog_info2(node->fn_opus, handle, type, info2);
}

void lister_prog_info3(struct ftp_node *node, char *info3)
{
	IPTR handle;
	int type;

	lister_get_prog_stuff(node, &handle, &type);

	rexx_prog_info3(node->fn_opus, handle, type, info3);
}

/********************************/

void lister_prog_init(struct ftp_node *node, char *title, char *info, char *name, int file, int bar)
{
	IPTR handle;
	int type;

	lister_get_prog_stuff(node, &handle, &type);

	if (type == PROGRESS_FREE && handle)
	{
		D(bug("** progress bar inited without being cleared!\n"));
		lister_prog_clear(node);
	}

	handle = rexx_prog_init(node->fn_opus, handle, type, title, info, name, file, bar);

	if (type == PROGRESS_FREE)
		node->fn_proghandle = handle;
}

/***************************************************/

void lister_prog_init_multi(struct ftp_node *node, char *title, BOOL short_display, char *name, int file, int bar)
{
	IPTR handle;
	int type;

	lister_get_prog_stuff(node, &handle, &type);

	if (type == PROGRESS_FREE && handle)
	{
		D(bug("** progress bar inited without being cleared!\n"));
		lister_prog_clear(node);
	}

	// -ve flag 0=expended display

	if (short_display)
		handle = rexx_prog_init(node->fn_opus, handle, type, title, "", name, file, bar);
	else
		handle = rexx_prog_init3(node->fn_opus, handle, type, title, "", "", "", name, file, bar);

	if (type == PROGRESS_FREE)
		node->fn_proghandle = handle;
}

/********************************/

void lister_prog_name(struct ftp_node *node, char *name)
{
	IPTR handle;
	int type;

	lister_get_prog_stuff(node, &handle, &type);

	rexx_prog_name(node->fn_opus, handle, type, name);
}

/********************************/

//
//	Copy lister's path into buffer (length must be PATHLEN + 1)
//	Returns 1 for success
//
int lister_get_path(struct ftp_node *node, char *buffer)
{
	char *s;
	int retval = 0;

	if ((s = rexx_lst_query_path(node->fn_opus, node->fn_handle)))
	{
		if (buffer)
			stccpy(buffer, s, PATHLEN + 1);

		DeleteArgstring(s);
		retval = 1;
	}

	return retval;
}

/********************************/

//
//	Returns 1 if lister has a custom handler
//	Returns 0 otherwise
//
int handle_has_handler(const char *opus, IPTR handle)
{
	char *str;
	int retval = 0;
	IPTR result;

	result =
		send_rexxa(opus, REXX_REPLY_RESULT, "lister query " FTP_HANDLE_PRINTF " handler", FTP_HANDLE_VALUE(handle));
	if (result && result != (IPTR)-1)
	{
		str = (char *)result;

		if (*str)
			retval = 1;

		DeleteArgstring(str);
	}

	return retval;
}

/********************************/

//
//	Show a requester over a lister
//	Must provide FR_IPC if not called by lister process!
//	Will add an OK button if no buttons specified
//
//	As with AsyncRequest and the system requester functions,
//	"-1 Means that one of the caller-supplied IDCMPFlags occurred.
//	The IDCMPFlag value is in the longword pointed to by IDCMP_ptr."
//	For us, this usually means enter was pressed in an empty string
//	gadget.
//
// int lister_request( struct ftp_node *node, Tag tag, ... )
int lister_request(struct ftp_node *node, struct TagItem *tags)
{
	IPCData *ipc;

	// Requesters suppressed so we can shut down?  Same as pressing 'Cancel' button...
	if (node->fn_og->og_noreq)
		return 0;

	// Requesters suppressed by the user?  Same as pressing the default (leftmost) button...
	if (node->fn_flags & LST_NOREQ)
		return 1;

	// Get correct IPC pointer
	ipc = (IPCData *)GetTagData(FR_IPC, (IPTR)node->fn_ipc, tags);

	if ((struct Task *)ipc->proc != FindTask(0))
	{
		D(bug("** lister_request called with wrong IPC!\n"));
		return 0;
	}
	else
		return ftpmod_request_tags(node->fn_og, ipc, AR_Window, ((Lister *)node->fn_handle)->window, TAG_MORE, tags);
}

/********************************/

//
//	Structure copied from dopusprog:misc.h
//
struct read_startup
{
	struct List *files;
	char initial_search[80];
	IPCData *owner;
};

int lister_long_message(struct ftp_node *ftpnode, Att_List *list, ULONG flags)
{
	char filename[PATHLEN + 1] = "";
	int node_count;
	BPTR temp_file;
	int i;
	Att_Node *node;
	int retval = 0;

	node_count = Att_NodeCount(list);

	// Need to do anything?
	if (node_count <= (flags & LONGMSG_MULTI) ? 1 : 0)
		return retval;

	// Need to trim last line(s) of multi-line reply?
	if (flags & LONGMSG_MULTI)
	{
		// Trim last line (boring response line)
		--node_count;

		// Trim empty lines between message and boring response line
		for (node = Att_FindNode(list, node_count - 1); node->node.ln_Pred; node = (Att_Node *)node->node.ln_Pred)
		{
			if (strlen(node->node.ln_Name) == 0 ||
				(ftpnode->fn_og->og_oc.oc_log_debug && strlen(node->node.ln_Name) == 4 &&
				 isdigit(node->node.ln_Name[0]) && isdigit(node->node.ln_Name[1]) && isdigit(node->node.ln_Name[2]) &&
				 node->node.ln_Name[3] == '-'))
				--node_count;
			else
				break;
		}
	}

	// Create temporary file
	if ((temp_file = open_temp_file(filename, ftpnode->fn_ipc)))
	{
		for (i = 0, node = (Att_Node *)list->list.lh_Head; i < node_count && node->node.ln_Succ;
			 ++i, node = (Att_Node *)node->node.ln_Succ)
		{
			FWrite(temp_file, node->node.ln_Name, strlen(node->node.ln_Name), 1);
			FPutC(temp_file, '\n');
		}
		Close(temp_file);
	}

	if (*filename)
	{
		char *result;
		IPTR rexx_result;

		rexx_result =
			send_rexxa(ftpnode->fn_opus,
					   REXX_REPLY_RESULT,
					   "dopus read " FTP_HANDLE_PRINTF " delete %s",
					   FTP_HANDLE_VALUE(ftpnode->fn_read_handle),
					   filename);
		if (rexx_result && rexx_result != (IPTR)-1)
		{
			result = (char *)rexx_result;
			ftpnode->fn_read_handle = ftp_parse_handle(result);
			DeleteArgstring(result);
		}
	}

	return retval;
}

/********************************/

/********************************/

//
//	Update the ftp lister's copy of the lister format
//	from the Opus lister's copy
//
void ftplister_read_listformat(struct ftp_node *node)
{
	Lister *lister;

	if (!node || !node->fn_handle)
		return;

	lister = (Lister *)node->fn_handle;
	if (lister->cur_buffer)
		*&node->fn_site.se_listformat = *&lister->cur_buffer->buf_ListFormat;
	else
		*&node->fn_site.se_listformat = *&lister->format;
}

/********************************/

//
//	Tell the Opus lister to update its copy of the lister format
//	by passing it a copy of ours
//
int ftplister_write_listformat(struct ftp_node *node, int ok)
{
	ListFormat *lf = 0;
	int retval = 0;

	if (ok && (lf = AllocVec(sizeof(ListFormat), MEMF_ANY)))
	{
		*lf = *&node->fn_site.se_listformat;
		retval = 1;
	}

	// Tell Opus lister to update its format
	IPC_Command(((Lister *)node->fn_handle)->ipc, LISTER_CONFIGURE, 0, (APTR)(IPTR)ok, lf, 0);

	return retval;
}

/********************************/
