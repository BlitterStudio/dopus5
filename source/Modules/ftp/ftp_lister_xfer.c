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

/**
 **	This file should contain any high level functions which
 **	do the uploading and downloading of files.  Currently:
 **
 **	lister_xfer
 **	lister_doubleclick
 **	lister_traptemp
 **	lister_xferindex
 **	lister_getput
 **/

// Greg remove this define if the filter has problems
#define ILLEGAL_CHAR_FILTER

#include "ftp.h"
#include "ftp_arexx.h"
#include "ftp_ipc.h"
#include "ftp_lister.h"
#include "ftp_opusftp.h"
#include "ftp_recursive.h"
#include "ftp_util.h"
#include "ftp_lister_xfer.h"
#include "ftp_addressbook.h"
#include "ftp_addrsupp_protos.h"
#include "ftp_utf8.h"

/********************************/

//
//	Used by file transfer functions for timing purposes
//
void init_xfer_time(struct update_info *ui)
{
#if defined(__AROS__) || defined(__amigaos3__)
	struct Device *TimerBase = (struct Device *)GetTimerBase();
#else
	struct Library *TimerBase = GetTimerBase();
#endif
#ifdef __amigaos4__
	struct TimerIFace *ITimer = (struct TimerIFace *)GetInterface(TimerBase, "main", 1, NULL);
#endif

	GetSysTime(&ui->ui_start);
	// gettimeofday(&ui->ui_start, NULL);
	*&ui->ui_last = *&ui->ui_start;

#ifdef __amigaos4__
	DropInterface((struct Interface *)ITimer);
#endif
}

static void time_from_seconds(char *buf, ULONG seconds)
{
	int hrs, mins, secs;

	hrs = seconds / 3600;

	mins = (seconds % 3600) / 60;

	secs = seconds % 60;

	if (hrs)
		sprintf(buf, " %d:%02d:%02d", hrs, mins, secs);
	else
		sprintf(buf, " %02d:%02d", mins, secs);
}
/********************************/

//
//	Add an abort trap to a local lister (not controlled by us)
//
//	This requires setting up a fake ftp_node structure
//	Which in turn requires a fake IPCData structure with its proc field filled out
//	We then add our ARexx handler to the lister so we can trap the 'abort' message
//
static struct ftp_node *add_abort_trap(struct opusftp_globals *ogp, char *opus, IPTR lclhandle, IPCData *rmtipc)
{
	struct ftp_node *tmpnode;
	int ok = 0;

	if ((tmpnode = AllocVec(sizeof(struct ftp_node), MEMF_CLEAR)))
	{
		tmpnode->fn_og = ogp;
		tmpnode->fn_handle = lclhandle;
		tmpnode->fn_ftp.fi_og = tmpnode->fn_og = ogp;
		tmpnode->fn_flags = LST_LOCAL;
		strcpy(tmpnode->fn_opus, opus);

		if ((tmpnode->fn_ipc = rmtipc))
		{
			ListLockAddHead(&ogp->og_listerlist, (struct Node *)tmpnode, &ogp->og_listercount);
			ok = 1;
		}

		if (ok)
			send_rexxa(opus,
					   REXX_REPLY_NONE,
					   "lister set " FTP_HANDLE_PRINTF " handler '" PORTNAME "' leavegauge",
					   FTP_HANDLE_VALUE(lclhandle));
	}

	if (!ok)
	{
		FreeVec(tmpnode);
		tmpnode = 0;
	}

	return tmpnode;
}

/********************************/

//
//	Remove an abort trap from a local lister (not controlled by us)
//
static void rem_abort_trap(struct opusftp_globals *ogp, char *opus, struct ftp_node *tmpnode, IPTR lclhandle)
{
	if (tmpnode)
	{
		send_rexxa(opus,
				   REXX_REPLY_NONE,
				   "lister set " FTP_HANDLE_PRINTF " handler ''",
				   FTP_HANDLE_VALUE(lclhandle));

		ListLockRemove(&ogp->og_listerlist, (struct Node *)tmpnode, &ogp->og_listercount);
		//	if	(tmpnode->fn_ipc)
		//		FreeVec( tmpnode->fn_ipc );
		FreeVec(tmpnode);
	}
}

/********************************/

//
//	Show the replace requester if copying an entry which already exists
//	Show the resume requester if about to replace a shorter file
//	Should also handle the UPDATE and NEWER switches
//	Should accept flag field with 'NORESUME' option
//
//	Returns 0 for Abort
//	Returns 1 for Resume
//	Returns 2 for Resume all
//	Returns 3 for Replace
//	Returns 4 for Replace all
//	Returns 5 for Skip
//	Returns 6 for Skip all
//
//	UPDATE/S - only files which do not already exist will be copied.
//
//	NEWER/S - similar to the UPDATE/S switch except will only copy
//		files that either do not exist or which have a later
//		datestamp than the existing file.
//
int replace_requester(struct hook_rec_data *hc, ULONG flags, struct entry_info *src, struct entry_info *dst)
{
	char srcsize[10 + 1], dstsize[10 + 1];
	char newdate[150], olddate[150];
	int retval = REPLACE_SKIP;	// Skip if anything goes wrong

	// Copying a dir over a dir is not checked
	if (src->ei_type >= 0 && dst->ei_type >= 0)
		return 3;

	// Copying a file over a dir?
	if (src->ei_type < 0 && dst->ei_type >= 0)
	{
		switch (lister_request_tags(hc->hc_prognode,
									FR_FormatString,
									"%s '%s' %s.",
									FR_MsgNum,
									MSG_DESTINATION,
									AR_Message,
									dst->ei_name,
									FR_MsgNum,
									MSG_IS_A_DIR,
									FR_ButtonNum,
									MSG_FTP_SKIP,
									FR_ButtonNum,
									MSG_ABORT,
									TAG_DONE))
		{
		case 0:
			retval = REPLACE_ABORT;
			break;	// Abort
		case 1:
			retval = REPLACE_SKIP;
			break;	// Skip
		}
	}

	// Copying a dir over a file?
	else if (src->ei_type >= 0 && dst->ei_type < 0)
	{
		switch (lister_request_tags(hc->hc_prognode,
									FR_FormatString,
									"%s '%s' %s.",
									FR_MsgNum,
									MSG_DESTINATION,
									AR_Message,
									dst->ei_name,
									FR_MsgNum,
									MSG_IS_A_FILE,
									FR_ButtonNum,
									MSG_FTP_SKIP,
									FR_ButtonNum,
									MSG_FTP_REPLACE,
									FR_ButtonNum,
									MSG_ABORT,
									TAG_DONE))
		{
		case 0:
			retval = REPLACE_ABORT;
			break;	// Abort
		case 1:
			retval = REPLACE_SKIP;
			break;	// Skip
		case 2:
			retval = REPLACE_REPLACE;
			break;	// Replace
		}
	}

	// Copying a file with the same datestamp and size
	// UPDATE and NEWER cause SKIP
	else if (src->ei_size == dst->ei_size && src->ei_seconds == dst->ei_seconds)
	{
		/*gp moved this to be first checked */

		if (flags & (REPLACEF_UPDATE | REPLACEF_NEWER))
		{
			return REPLACE_SKIP;
		}

		if (hc->hc_recur_flags & RECURF_REPLACEALL)
			return REPLACE_REPLACE;
		else if (hc->hc_recur_flags & RECURF_SKIPALL)
			return REPLACE_SKIP;

		/*
			if	(flags & (REPLACEF_UPDATE | REPLACEF_NEWER))
				return REPLACE_SKIP;
		*/

		switch (lister_request_tags(hc->hc_prognode,
									FR_FormatString,
									"%s '%s' %s\n%s",
									FR_MsgNum,
									MSG_FTP_FILE,
									AR_Message,
									dst->ei_name,
									FR_MsgNum,
									MSG_FTP_WOULD_BE_REPLACED,
									FR_MsgNum,
									MSG_FTP_APPEAR_SAME,
									FR_ButtonNum,
									MSG_FTP_REPLACE,
									FR_ButtonNum,
									MSG_FTP_REPLACEALL,
									FR_ButtonNum,
									MSG_FTP_SKIP,
									FR_ButtonNum,
									MSG_FTP_SKIPALL,
									FR_ButtonNum,
									MSG_ABORT,
									TAG_DONE))
		{
		case 0:
			retval = REPLACE_ABORT;
			break;	// Abort
		case 1:
			retval = REPLACE_REPLACE;
			break;	// Replace
		case 2:
			retval = REPLACE_REPLACEALL;
			break;	// Replace All
		case 3:
			retval = REPLACE_SKIP;
			break;	// Skip
		case 4:
			retval = REPLACE_SKIPALL;
			break;	// Skip All
		}
	}

	// Copying a file with different datestamp, size, or both
	// UPDATE always causes skip, NEWER does if file is same age or older
	else
	{
		// Get both size strings
		if (src->ei_size == EI_SIZE_UNKNOWN)
			strcpy(srcsize, "???");
		else
			lsprintf(srcsize, "%lu", src->ei_size);

		if (dst->ei_size == EI_SIZE_UNKNOWN)
			strcpy(dstsize, "???");
		else
			lsprintf(dstsize, "%lu", dst->ei_size);

		// Get both date strings
		seconds_to_datestring(src->ei_seconds, newdate);
		seconds_to_datestring(dst->ei_seconds, olddate);

		if (flags & REPLACEF_UPDATE)
			return REPLACE_SKIP;

		if (src->ei_seconds < dst->ei_seconds && (flags & REPLACEF_NEWER))
			return REPLACE_SKIP;

		// Copying a file longer than the old version (if resume supported)
		if (src->ei_size != EI_SIZE_UNKNOWN && dst->ei_size != EI_SIZE_UNKNOWN && src->ei_size > dst->ei_size &&
			!(flags & REPLACEF_NORESUME))
		{
			if (hc->hc_recur_flags & RECURF_REPLACEALL)
				return REPLACE_REPLACE;
			else if (hc->hc_recur_flags & RECURF_SKIPALL)
				return REPLACE_SKIP;
			else if (hc->hc_recur_flags & RECURF_RESUMEALL)
				return REPLACE_RESUME;

			switch (lister_request_tags(hc->hc_prognode,
										FR_FormatString,
										"%s '%s' %s\n"
										"%s\n\n"
										"%s :%s  %s : %s\n"
										"%s :%s  %s : %s",
										FR_MsgNum,
										MSG_FTP_FILE,
										AR_Message,
										dst->ei_name,
										FR_MsgNum,
										MSG_IS_LONGER,
										FR_MsgNum,
										MSG_RESUME_XFER,
										FR_MsgNum,
										MSG_FTP_NEWSIZE,
										AR_Message,
										srcsize,
										FR_MsgNum,
										MSG_FTP_DATE,
										AR_Message,
										newdate,
										FR_MsgNum,
										MSG_FTP_OLDSIZE,
										AR_Message,
										dstsize,
										FR_MsgNum,
										MSG_FTP_DATE,
										AR_Message,
										olddate,
										FR_ButtonNum,
										MSG_FTP_RESUME,
										FR_ButtonNum,
										MSG_FTP_REPLACE,
										FR_ButtonNum,
										MSG_FTP_SKIP,
										FR_ButtonNum,
										MSG_ABORT,
										TAG_DONE))
			{
			case 0:
				retval = REPLACE_ABORT;
				break;	// Abort
			case 1:
				retval = REPLACE_RESUME;
				break;	// Resume
			case 2:
				retval = REPLACE_REPLACE;
				break;	// Replace
			case 3:
				retval = REPLACE_SKIP;
				break;	// Skip
			}
		}

		// Copying a file with different date/size but can't be resumed
		else
		{
			if (hc->hc_recur_flags & RECURF_REPLACEALL)
				return REPLACE_REPLACE;
			else if (hc->hc_recur_flags & RECURF_SKIPALL)
				return REPLACE_SKIP;

			switch (lister_request_tags(hc->hc_prognode,
										FR_FormatString,
										"%s '%s' %s. %s\n\n"
										"%s :%s  %s : %s\n"
										"%s :%s  %s : %s",
										FR_MsgNum,
										MSG_FTP_FILE,
										AR_Message,
										dst->ei_name,
										FR_MsgNum,
										MSG_FTP_ALREADY_EXISTS,
										FR_MsgNum,
										MSG_FTP_REPLACE_IT,
										FR_MsgNum,
										MSG_FTP_NEWSIZE,
										AR_Message,
										srcsize,
										FR_MsgNum,
										MSG_FTP_DATE,
										AR_Message,
										newdate,
										FR_MsgNum,
										MSG_FTP_OLDSIZE,
										AR_Message,
										dstsize,
										FR_MsgNum,
										MSG_FTP_DATE,
										AR_Message,
										olddate,
										FR_ButtonNum,
										MSG_FTP_REPLACE,
										FR_ButtonNum,
										MSG_FTP_REPLACEALL,
										FR_ButtonNum,
										MSG_FTP_SKIP,
										FR_ButtonNum,
										MSG_FTP_SKIPALL,
										FR_ButtonNum,
										MSG_ABORT,
										TAG_DONE))
			{
			case 0:
				retval = REPLACE_ABORT;
				break;	// Abort
			case 1:
				retval = REPLACE_REPLACE;
				break;	// Replace
			case 2:
				retval = REPLACE_REPLACEALL;
				break;	// Replace All
			case 3:
				retval = REPLACE_SKIP;
				break;	// Skip
			case 4:
				retval = REPLACE_SKIPALL;
				break;	// Skip All
			}
		}
	}

	return retval;
}

/********************************/

static int transfer_aborted(struct update_info *ui)
{
	if (!ui)
		return 0;
	if (ui->ui_abort && *ui->ui_abort)
		return *ui->ui_abort;
	if (ui->ui_ftpnode && (ui->ui_ftpnode->fn_flags & LST_ABORT))
		return 1;
	return 0;
}

/********************************/

//
//	Gets called every so often by the low-level get()/put() function
//	Updates the progress bar
//	The return value is interpreted as an error code if non-zero (see ftp.h)
//	Always called at start and end of file (start may be 0% or resuming somewhere)
//
int xfer_update(struct update_info *ui, unsigned int total, unsigned int bytes)
{
	char info_bytes[256];
	char info_time[256];
#if defined(__AROS__) || defined(__amigaos3__)
	struct Device *TimerBase = (struct Device *)GetTimerBase();
#else
	struct Library *TimerBase = GetTimerBase();
#endif
#ifdef __amigaos4__
	struct TimerIFace *ITimer = (struct TimerIFace *)GetInterface(TimerBase, "main", 1, NULL);
#endif

	// init buffers
	*info_bytes = *info_time = 0;

	// Unknown total is represented by 0xffffffff
	if (total != 0xffffffff)
		ui->ui_total_bytes = total;

	ui->ui_bytes_so_far += bytes;

	// On initial callback, set place where resume started
	// If total and bytes are both 0xffffffff this means REST failed
	if (!(ui->ui_flags & UI_FIRSTDONE))
	{
		if (total == 0xffffffff && bytes == 0xffffffff)
		{
			lister_request_tags(ui->ui_ftpnode, FR_MsgNum, MSG_RESUME_FAILURE, TAG_DONE);
		}
		else
		{
			ui->ui_resumed_bytes = bytes;
			ui->ui_flags |= UI_FIRSTDONE;
		}
	}

	// On final callback, Show progress bar at 100%
	else if (bytes == 0)
		ui->ui_bytes_so_far = ui->ui_total_bytes;

	// Update progress bar
	if (ui->ui_ftpnode)
	{
		lister_prog_bytes(ui->ui_ftpnode, ui->ui_total_bytes, ui->ui_bytes_so_far);

		// want an expanded display?
		// -ve flag 0=TRUE
		if (!ui->ui_info_type)
		{
			GetSysTime(&ui->ui_curr);
			// gettimeofday(&ui->ui_curr, NULL);

			// Need to update?
			if (ui->ui_curr.tv_secs - ui->ui_last.tv_secs >= 1)
			{
				char buf[80];
				int percent = 0, remain, time, rate;

				// Prevent divide by zero
				if ((time = ui->ui_curr.tv_secs - ui->ui_start.tv_secs) == 0)
					time = 1;

				// calculate byte values
				strcpy(info_bytes, GetString(locale, MSG_TRANSFER_DISPLAY_BYTES));
				strcat(info_bytes, " ");
				cat_bytes(info_bytes, ui->ui_bytes_so_far);
				strcat(info_bytes, "/");

				if (ui->ui_total_bytes == 0xffffffff)
					strcat(info_bytes, "???");
				else
				{
					cat_bytes(info_bytes, ui->ui_total_bytes);
					if (ui->ui_total_bytes > 0)
						percent = (100 * ui->ui_bytes_so_far) / ui->ui_total_bytes;

					if (percent)
					{
						sprintf(buf, " %d%%", percent);
						strcat(info_bytes, buf);
					}
				}

				// calc speed
				if ((rate = (ui->ui_bytes_so_far - ui->ui_resumed_bytes) / time) > 1)
				{
					strcat(info_bytes, " ");
					cat_bytes(info_bytes, rate);
					strcat(info_bytes, "/s ");
				}

				// display line 2
				lister_prog_info2(ui->ui_ftpnode, info_bytes);

				// calc times and estimated remaining
				time_from_seconds(buf, ui->ui_curr.tv_secs - ui->ui_start.tv_secs);
				strcpy(info_time, GetString(locale, MSG_TRANSFER_DISPLAY_TIMES));
				strcat(info_time, buf);

				if (rate > 1)
				{
					remain = (ui->ui_total_bytes - ui->ui_bytes_so_far) / rate;
					time_from_seconds(buf, remain);
					strcat(info_time, buf);
				}

				// display line 3
				lister_prog_info3(ui->ui_ftpnode, info_time);
			}
			ui->ui_last = ui->ui_curr;
		}
	}

#ifdef __amigaos4__
	DropInterface((struct Interface *)ITimer);
#endif

	return transfer_aborted(ui);
}

/********************************/

//
//	Gets called every so often by the pseudo-low-level recursive_getput() function
//	Updates the progress bar
//	The return value is interpreted as an error code if non-zero (see ftp.h)
//	Always called at start and end of file (start may be 0% or resuming somewhere)
//
int getput_update(struct update_info *ui, unsigned int total, unsigned int bytes)
{
#if defined(__AROS__) || defined(__amigaos3__)
	struct Device *TimerBase = (struct Device *)GetTimerBase();
#else
	struct Library *TimerBase = GetTimerBase();
#endif
#ifdef __amigaos4__
	struct TimerIFace *ITimer = (struct TimerIFace *)GetInterface(TimerBase, "main", 1, NULL);
#endif

	// Did we get an improved byte total?
	if (total != 0xffffffff)
		ui->ui_total_bytes = total;

	ui->ui_bytes_so_far += bytes;

	// On initial callback, set place where resume started
	if (!(ui->ui_flags & UI_FIRSTDONE))
	{
		if (total == 0xffffffff && bytes == 0xffffffff)
		{
			lister_request_tags(ui->ui_ftpnode, FR_MsgNum, MSG_RESUME_FAILURE, TAG_DONE);
		}
		else
		{
			ui->ui_resumed_bytes = bytes;
			ui->ui_flags |= UI_FIRSTDONE;
		}
	}

	// On final callback, Show progress bar at 100%
	else if (bytes == 0)
		ui->ui_bytes_so_far = 3;

	if (ui->ui_ftpnode)
	{
		lister_prog_bytes(ui->ui_ftpnode, 3, ui->ui_bytes_so_far & 3);

		// extended display?

		// -ve flag 0=TRUE
		if (!ui->ui_info_type)
		{
			GetSysTime(&ui->ui_curr);
			// gettimeofday(&ui->ui_curr, NULL);

			// Need to update?
			if (ui->ui_curr.tv_secs - ui->ui_last.tv_secs >= 1)
			{
				char info_time[80];
				char info_bytes[80];
				char buf[80];

				if (ui->ui_total_bytes)
				{
					strcpy(info_bytes, GetString(locale, MSG_TRANSFER_DISPLAY_BYTES));
					strcat(info_bytes, " ");

					if (ui->ui_total_bytes == 0xffffffff)
						strcat(info_bytes, "???");
					else
						cat_bytes(info_bytes, ui->ui_total_bytes);

					lister_prog_info2(ui->ui_ftpnode, info_bytes);
				}

				time_from_seconds(buf, ui->ui_curr.tv_secs - ui->ui_start.tv_secs);

				strcpy(info_time, GetString(locale, MSG_TRANSFER_DISPLAY_TIMES));
				strcat(info_time, buf);

				lister_prog_info3(ui->ui_ftpnode, info_time);
			}

			ui->ui_last = ui->ui_curr;
		}
	}

#ifdef __amigaos4__
	DropInterface((struct Interface *)ITimer);
#endif

	return transfer_aborted(ui);
}

/**************************************************************
 *
 * 	continue the setup of a multi line progress display
 *
 ***************************************************************/

static void init_multi_display(struct ftp_node *node, BOOL short_display, int size)
{
	if (!short_display)
	{
		char buf[80];

		strcpy(buf, GetString(locale, MSG_TRANSFER_DISPLAY_BYTES));
		strcat(buf, " ");

		if (size && size != 0xffffffff)
			cat_bytes(buf, size);

		lister_prog_info2(node, buf);

		lister_prog_info3(node, GetString(locale, MSG_TRANSFER_DISPLAY_TIMES));
	}
}

/********************************/
/********************************/

//
//	Hook called before commencing transfer of each entry
//
//	Handles progress bar
//	Handles replace/resume requesters
//	Handles Copy UPDATE and NEWER options
//
static int hook_copy_pre(struct hook_rec_data *hc,
						 char *dirname,
						 struct rec_entry_list *dstlist,
						 struct entry_info *srcent)
{
	struct entry_info *dstent;
	int replace_flags = 0;
	int replace_option = REPLACE_INVALID;

	D(bug("hook_copy_pre()\n"));

	// Valid?
	if (!hc)
		return replace_option;

	// Reset dest length / resume point / getsizes accumulator
	hc->hc_misc_bytes = 0;

	// Update progress bar name
	if (hc->hc_opus && hc->hc_proghandle)
	{
		if (dirname)
			final_path(dirname, hc->hc_fromdirname);

		if (srcent)
		{
			sprintf(hc->hc_ui.ui_infotext_path,
					"%s '%s' %s '%s'",
					GetString(locale, MSG_FROM),
					hc->hc_fromdirname,
					GetString(locale, MSG_TO),
					hc->hc_todirname);

			lister_prog_name(hc->hc_prognode, srcent->ei_name);
			lister_prog_info(hc->hc_prognode, hc->hc_ui.ui_infotext_path);

			// -ve flag 0=TRUE
			if (!hc->hc_ui.ui_info_type)
				init_multi_display(hc->hc_prognode, hc->hc_ui.ui_info_type, srcent->ei_size);

			rexx_lst_label(hc->hc_opus, hc->hc_proghandle, 0, srcent->ei_name, 0);
		}
	}

	// Replace requester
	if (!hc->hc_need_dest)
		return replace_option;

	// Set replace checking flags
	if (hc->hc_anded_ftp_flags & FTP_NO_REST)
		replace_flags |= REPLACEF_NORESUME;

	if ((hc->hc_copy_flags & XFER_OPT_UPDATE) || (hc->hc_env->e_copy_type == COPY_TYPE_UPDATE))
		replace_flags |= REPLACEF_UPDATE;

	if ((hc->hc_copy_flags & XFER_OPT_NEWER) || (hc->hc_env->e_copy_type == COPY_TYPE_NEWER))
		replace_flags |= REPLACEF_NEWER;

	// Get destination entry and call resume/replace/update/newer function

	// Is this a top-level entry (visible in lister)?
	if (hc->hc_basedirname == dirname && !(hc->hc_copy_flags & XFER_SUBDIR))
	{
		char *destname;

		if (hc->hc_newname)
			destname = hc->hc_newname;
		else
			destname = srcent->ei_name;

		replace_option = REPLACE_REPLACE;

		// Is destination an FTP site?
		if (hc->hc_dest->ep_type == ENDPOINT_FTP)
		{
			if ((dstent = AllocVec(sizeof(struct entry_info), MEMF_CLEAR)))
			{
				if (entry_info_from_lister(hc->hc_dest->ep_ftpnode, destname, dstent, 0))
				{
					// Dest length / resume point
					hc->hc_misc_bytes = dstent->ei_size;

					replace_option = replace_requester(hc, replace_flags, srcent, dstent);

					D(bug("** replace option %ld\n", (long)replace_option));
				}

				FreeVec(dstent);
			}
			else
				replace_option = REPLACE_SKIP;
		}

		// No destination lister - destination is filesystem
		else
		{
			if ((dstent = rec_filesys_getentry(hc->hc_dest, destname)))
			{
				// Dest length / resume point
				hc->hc_misc_bytes = dstent->ei_size;

				replace_option = replace_requester(hc, replace_flags, srcent, dstent);

				D(bug("** B replace option %ld\n", (long)replace_option));

				FreeVec(dstent);
			}
		}
	}

	// Not a top level entry - info provided in list
	else
	{
		if (dstlist && (dstent = rec_find_entry(dstlist, srcent, hc->hc_ored_ftp_flags)))
		{
			// Dest length / resume point
			hc->hc_misc_bytes = dstent->ei_size;

			replace_option = replace_requester(hc, replace_flags, srcent, dstent);
			D(bug("** C replace option %ld\n", (long)replace_option));
		}
	}

	// Set some flags based on what replace requester button was pressed
	if (replace_option == REPLACE_ABORT)
		hc->hc_source->ep_ftpnode->fn_flags |= LST_ABORT;
	else if (replace_option == REPLACE_SKIPALL)
		hc->hc_recur_flags |= RECURF_SKIPALL;
	else if (replace_option == REPLACE_REPLACEALL)
	{
		// Download this and all other existing files we come to
		hc->hc_recur_flags |= RECURF_REPLACEALL;

		if (!(hc->hc_copy_flags & XFER_OPT_UPDATE) && !(hc->hc_env->e_copy_type == COPY_TYPE_UPDATE) &&
			!(hc->hc_copy_flags & XFER_OPT_NEWER) && !(hc->hc_env->e_copy_type == COPY_TYPE_NEWER))

			hc->hc_need_dest = 0;
	}
	else if (replace_option == REPLACE_RESUMEALL)
	{
		// Resume downloading all resumable files, what of other files???
		hc->hc_recur_flags |= RECURF_RESUMEALL;
	}

	return replace_option;
}

/********************************/

//
//	Request a new name for a file copied with CopyAs or MoveAs
//	Returns 0 for skip
//	Returns 1 for copy
//	Returns 2 for abort
//
static int request_newname(struct ftp_node *ftpnode, char *buffer, char *deflt, BOOL move)
{
	int retval;

	stccpy(buffer, deflt, FILENAMELEN + 1);

	retval = lister_request_tags(ftpnode,
								 FR_MsgNum,
								 MSG_FTP_ENTER_NEW_FILENAME,
								 FR_ButtonNum,
								 move ? MSG_MOVE : MSG_FTP_COPY_UCASE,
								 FR_ButtonNum,
								 MSG_ABORT,
								 FR_ButtonNum,
								 MSG_FTP_SKIP,
								 AR_Buffer,
								 buffer,
								 AR_BufLen,
								 FILENAMELEN + 1,
								 AR_Flags,
								 SRF_PATH_FILTER,
								 TAG_DONE);

	return retval;
}

/********************************/

typedef struct
{
	char srcname[FILENAMELEN + 1];	// Source name of this entry
	char newname[FILENAMELEN + 1];	// New name for CopyAs
	struct hook_rec_data hc;
	char startdirname[FILENAMELEN + 1];
	char fulldirname[FILENAMELEN + 1];
	char url[1024 + 1];
} xfer_locals;

//
//	Transfer a group of files between a local directory and an FTP host
//
void lister_xfer(struct ftp_node *remotenode, IPCMessage *msg)
{
	struct xfer_msg *xm;
	struct ftp_node *srcnode, *destnode, *prognode;
	IPTR srchandle, desthandle, proghandle;
	char *names, *p;  // Quoted names of all selected entries
	int entrycount;
	int dircount = 0;
	// int                  filecount = 0;
	int entrynumber = 1;
	int quiet;
	endpoint *source, *dest;
	struct entry_info ei;  // Info on this entry
	int confirm_commence;
	int success;
	TimerHandle *timer;
	struct rec_entry_list *dest_list = 0;
	char *comment;
	xfer_locals *l;
	int rexx_result = 0;
	DOpusCallbackInfo *infoptr;

	D(bug("lister_xfer()\n"));

	// Valid?
	if (!remotenode || !msg || !msg->data_free)
		return;

	infoptr = &remotenode->fn_og->og_hooks;

	// Allocate locals
	if (!(l = AllocVec(sizeof(xfer_locals), MEMF_ANY)))
		return;

	// Initialize locals
	memset(&l->hc, 0, sizeof(l->hc));
	*l->startdirname = 0;

	// Get transfer message
	xm = msg->data_free;

	// Make listers busy (we may only have one)
	rexx_lst_lock(remotenode->fn_opus, remotenode->fn_handle);
	if (xm->xm_otherhandle)
		rexx_lst_lock(remotenode->fn_opus, xm->xm_otherhandle);
	else
		D(bug("** no local handle!\n"));

	// -ve flag 0=TRUE
	l->hc.hc_ui.ui_info_type = remotenode->fn_site.se_env->e_transfer_details;

	// Set up nodes and handles
	if (msg->command == IPC_GET)
	{
		srcnode = remotenode;
		destnode = find_ftpnode(remotenode->fn_og, xm->xm_otherhandle);
		srchandle = remotenode->fn_handle;
		desthandle = xm->xm_otherhandle;
	}
	else
	{
		srcnode = find_ftpnode(remotenode->fn_og, xm->xm_otherhandle);
		destnode = remotenode;
		srchandle = xm->xm_otherhandle;
		desthandle = remotenode->fn_handle;
	}

	// Clear abort flag
	remotenode->fn_flags &= ~LST_ABORT;

	// Set up timer
	if ((timer = AllocTimer(UNIT_VBLANK, 0)))
		StartTimer(timer, remotenode->fn_site.se_env->e_script_time, 0);

	// Got any entries to transfer?
	// Will never be null
	if ((names = xm->xm_names))
	{
		char *path = 0;
		int freepath = 0;

		entrycount = count_entries(names);

		// "Quiet" option set?
		quiet = xm->xm_flags & XFER_OPT_QUIET;

		// Add an ARexx handler to the source lister so we can trap the abort button
		if (msg->command == IPC_PUT)
			srcnode = add_abort_trap(remotenode->fn_og, remotenode->fn_opus, srchandle, remotenode->fn_ipc);

		// Select node and handle for progress bar (should be source)
		if (srchandle)
		{
			prognode = srcnode;
			proghandle = srchandle;
		}
		else
		{
			prognode = destnode;
			proghandle = desthandle;
		}

		// Create endpoints
		if (msg->command == IPC_GET)
		{
			source = create_endpoint_tags(remotenode->fn_og, EP_TYPE, ENDPOINT_FTP, EP_FTPNODE, remotenode, TAG_DONE);

			// Dest path on command line or subdir drop?
			if (xm->xm_dstpath)
				path = xm->xm_dstpath;

			// Dest lister?  Query its path
			else if (xm->xm_otherhandle)
			{
				if ((path = rexx_lst_query_path(remotenode->fn_opus, xm->xm_otherhandle)))
					freepath = 1;
			}

			dest = create_endpoint_tags(remotenode->fn_og, EP_TYPE, ENDPOINT_FILESYS, EP_PATH, path, TAG_DONE);

			// Get 'To' path for progress bar
			final_path(path, l->hc.hc_todirname);

			if (freepath)
				DeleteArgstring(path);

			// Get 'From' path for progress bar
			if ((path = rexx_lst_query_path(remotenode->fn_opus, remotenode->fn_handle)))
			{
				strcpy(l->fulldirname, path);
				final_path(path, l->startdirname);
				final_path(path, l->hc.hc_fromdirname);
				DeleteArgstring(path);
			}
		}

		// Create PUT endpoints
		else
		{
			// Src path on command line?
			if (xm->xm_srcpath)
				path = xm->xm_srcpath;

			// Src lister?  Query its path
			else if (xm->xm_otherhandle)
			{
				if ((path = rexx_lst_query_path(remotenode->fn_opus, xm->xm_otherhandle)))
					freepath = 1;
			}

			source = create_endpoint_tags(
				remotenode->fn_og, EP_TYPE, ENDPOINT_FILESYS, EP_FTPNODE, srcnode, EP_PATH, path, TAG_DONE);

			// Destination directory
			//		if	(xm->xm_flags & XFER_SUBDIR)
			//			final_path( xm->xm_dstpath, l->startdirname );
			//		else
			{
				strcpy(l->fulldirname, path);
				final_path(path, l->startdirname);
			}

			// Get 'From' path for progress bar
			final_path(path, l->hc.hc_fromdirname);

			if (freepath)
				DeleteArgstring(path);

			// Dest path from subdir drop?
			if (xm->xm_dstpath)
			{
				path = xm->xm_dstpath;
				final_path(path, l->hc.hc_todirname);
			}

			// Get 'To' path for progress bar
			else if ((path = rexx_lst_query_path(remotenode->fn_opus, remotenode->fn_handle)))
			{
				final_path(path, l->hc.hc_todirname);
				DeleteArgstring(path);
			}

			dest = create_endpoint_tags(remotenode->fn_og, EP_TYPE, ENDPOINT_FTP, EP_FTPNODE, remotenode, TAG_DONE);
		}

		// Does entry contain path information?
		if (xm->xm_srcpath && strpbrk(xm->xm_srcpath, ":/"))
		{
			D(bug("** entry contains source path information\n"));
			D(bug("** '%s'\n", xm->xm_srcpath));
		}
		if (xm->xm_dstpath && strpbrk(xm->xm_dstpath, ":/"))
		{
			D(bug("** entry contains dest path information\n"));
			D(bug("** '%s'\n", xm->xm_dstpath));
		}

		// Endpoints created?
		if (source && dest)
		{
			// Count files and directories

			// Need to query each entry?
			if (entrycount == 1 || (xm->xm_flags & (XFER_DROP | XFER_DROPFROM)))
			{
				for (p = names + 1; !(prognode->fn_flags & LST_ABORT); p += 3)
				{
					p = stptok(p, l->srcname, FILENAMELEN, "\"\r\n");

					// Does entry contain path information?
					if (strpbrk(l->srcname, ":/"))
					{
						D(bug("** entry contains path information\n"));
						D(bug("** '%s'\n", l->srcname));
					}

					if (entry_info_from_lister(srcnode, l->srcname, &ei, ENTRYFROMF_DEFAULT))
					{
						if (ei.ei_type >= 0)
							++dircount;
						//	else
						//		++ filecount;
					}

					// Done all entries?
					if (*(p + 1) == 0)
						break;
				}
			}

			// Multiple entries.  Query using callbacks.
			else if (srchandle)
			{
				//	filecount = rexx_lst_query_numselfiles( remotenode->fn_opus, srchandle );
				dircount = rexx_lst_query_numseldirs(remotenode->fn_opus, srchandle);
			}

			// Multiple entries with no source lister handle??  Impossible
			else
			{
				D(bug("** copy multiple entries without source lister!\n"));
			}

			// Show warning requester for recursive copy
			if (!quiet && dircount)
			{
				confirm_commence = lister_request_tags(prognode,
													   FR_FormatString,
													   "%s %s",
													   FR_MsgNum,
													   xm->xm_flags & XFER_MOVE ? MSG_MOVE : MSG_FTP_COPY_UCASE,
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
				confirm_commence = 1;

			if (confirm_commence)
			{
				lister_prog_init_multi(prognode,
									   GetString(locale, xm->xm_flags & XFER_MOVE ? MSG_FTP_MOVING : MSG_FTP_COPYING),
									   remotenode->fn_site.se_env->e_transfer_details,
									   "",
									   PROGRESS_FILE_ON,
									   PROGRESS_BAR_ON);

				// Set up copy hook data
				l->hc.hc_og = remotenode->fn_og;
				l->hc.hc_source = source;
				l->hc.hc_dest = dest;
				l->hc.hc_pre = hook_copy_pre;
				l->hc.hc_need_dest = 1;	 // Support Replace
				l->hc.hc_opus = remotenode->fn_opus;
				l->hc.hc_prognode = prognode;
				l->hc.hc_proghandle = proghandle;
				l->hc.hc_env = remotenode->fn_site.se_env;
				l->hc.hc_basedirname = l->startdirname;
				l->hc.hc_copy_flags = xm->xm_flags;
				l->hc.hc_ored_ftp_flags = remotenode->fn_ftp.fi_flags;
				l->hc.hc_anded_ftp_flags = remotenode->fn_ftp.fi_flags;

				if (remotenode->fn_site.se_env->e_recursive_special)
					l->hc.hc_recur_flags |= RECURF_BROKEN_LS;

				if (confirm_commence == 2)
					l->hc.hc_recur_flags |= RECURF_NORECUR;

				if (remotenode->fn_site.se_env->e_copy_replace == COPY_REPLACE_ALWAYS)
				{
					l->hc.hc_recur_flags |= RECURF_REPLACEALL;
					// l->hc.hc_need_dest = 0;
				}
				else if (remotenode->fn_site.se_env->e_copy_replace == COPY_REPLACE_NEVER)
					l->hc.hc_recur_flags |= RECURF_SKIPALL;

				l->hc.hc_options = lister_options(remotenode, OPTION_COPY);

				// Need to CD to subdir?
				if ((xm->xm_flags & (XFER_DROP | XFER_SUBDIR)) == (XFER_DROP | XFER_SUBDIR))
				{
					if (remotenode->fn_protocol == FTP_PROTOCOL_SFTP)
					{
						char *utf8_path = ftp_convert_path_to_utf8(xm->xm_dstpath, &remotenode->fn_ftp);
						const char *send_path = utf8_path ? utf8_path : xm->xm_dstpath;
						ftp_sftp_cwd(&remotenode->fn_sftp, send_path);
						if (utf8_path) ftp_codesets_free(utf8_path);
					}
					else
						ftp_cwd(&remotenode->fn_ftp, 0, 0, xm->xm_dstpath);
					dest_list = dest->ep_list(dest, 0);
				}

				// Should use callbacks to get entries
				// (except drag'n'drop coz they won't be highlit)
				for (p = names + 1; !(prognode->fn_flags & LST_ABORT); p += 3)
				{
					int confirm_entry = 1;

					rexx_result = 0;

					// Next source entry name
					p = stptok(p, l->srcname, FILENAMELEN, "\"\r\n");

					// Update progress bar for each top level entry
					lister_prog_bar(prognode, entrycount, entrynumber++);

#ifdef ILLEGAL_CHAR_FILTER
					// Filter illegal characters
					if (!(xm->xm_flags & (XFER_OPT_NAME | XFER_AS)) && strpbrk(l->srcname, ":/"))
					{
						int i;

						for (i = 0;; ++i)
						{
							if (l->srcname[i] == ':')
								l->newname[i] = '_';
							else
								l->newname[i] = l->srcname[i];
							if (l->srcname[i] == 0)
								break;
						}

						l->hc.hc_newname = l->newname;
					}

					// Transer As?
					else if (xm->xm_flags & XFER_AS)
#else
					D(bug("illegal char filter OFF\n"));
					if (xm->xm_flags & XFER_AS)
#endif
					{
						// Old and new names provided?
						if (xm->xm_newname && xm->xm_flags & XFER_OPT_NAME)
						{
							strcpy(l->newname, xm->xm_newname);
							l->hc.hc_newname = l->newname;
						}
						else
						{
							switch (request_newname(prognode, l->newname, l->srcname, xm->xm_flags & XFER_MOVE))
							{
							case 0:
								*l->newname = 0;
								confirm_entry = 0;
								break;
							case 1:
							case 65535:
								l->hc.hc_newname = l->newname;
								break;
							case 2:
								*l->newname = 0;
								prognode->fn_flags |= LST_ABORT;
								confirm_entry = 0;
								break;
							default:
								*l->newname = 0;
								confirm_entry = 0;
								break;
							}
						}
					}

					// Plain Copy/Move, not CopyAs/MoveAs
					else
					{
						*l->newname = 0;
					}

					// Get file info
					if (confirm_entry && entry_info_from_lister(srcnode, l->srcname, &ei, ENTRYFROMF_DEFAULT))
					{
						// Transfer all?  Show no more confirmation requesters
						// if	(confirm_entry == ?)
						// quiet = 1;

						// if	(confirm_entry == ?)
						{
							// Transfer entry (and contents if it's a dir)
							if ((success = recursive_copy(&l->hc, l->startdirname, l->fulldirname, &ei, dest_list)))
							{
								// Get result for ARexx
								if (success == 3 && xm->xm_flags & XFER_MOVE)
									rexx_result = 1;
								else if (success == 2 && !(xm->xm_flags & XFER_MOVE))
									rexx_result = 1;

								if (desthandle && !(xm->xm_flags & XFER_SUBDIR))
								{
									// Add 'aborted' comment?
									if (remotenode->fn_flags & LST_ABORT)
									{
										lsprintf(l->url, GetString(locale, MSG_ABORT_COMMENT_START));
										if (l->hc.hc_ui.ui_total_bytes == 0xffffffff)
											strcat(l->url, GetString(locale, MSG_ABORT_COMMENT_UNKNOWN_SIZE));
										else
											lsprintf(l->url + strlen(l->url), "%lu", l->hc.hc_ui.ui_total_bytes);

										lsprintf(l->url + strlen(l->url), GetString(locale, MSG_ABORT_COMMENT_END));

										comment = l->url;
									}

									// URL in comment?
									else if (remotenode->fn_site.se_env->e_url_comment)
									{
										build_url(l->url,
												  remotenode->fn_site.se_user,
												  0,
												  remotenode->fn_site.se_host,
												  remotenode->fn_site.se_port,
												  remotenode->fn_site.se_path,
												  0);

										comment = l->url;
									}

									else if (l->hc.hc_options & COPY_NOTE)
										comment = ei.ei_comment;
									else
										comment = 0;

									// Add entry to destination
									rexx_lst_add(remotenode->fn_opus,
												 desthandle,
												 *l->newname ? l->newname : ei.ei_name,
												 ei.ei_size,
												 ei.ei_type < 0 ? -1 : 1,
												 l->hc.hc_options & COPY_DATE ? ei.ei_seconds : 0,
												 l->hc.hc_options & COPY_PROT ? ei.ei_prot : 0,
												 comment ? comment : "");

									// Re-add entry to source if needed
									if (msg->command == IPC_PUT && l->hc.hc_options & COPY_ARC)
										rexx_lst_add(remotenode->fn_opus,
													 srchandle,
													 ei.ei_name,
													 ei.ei_size,
													 ei.ei_type,
													 ei.ei_seconds,
													 ei.ei_prot | FIBF_ARCHIVE,
													 ei.ei_comment);
								}

								// Deselect or remove entry from source
								if (srchandle)
								{
									if (success == 3 && xm->xm_flags & XFER_MOVE)
										rexx_lst_remove(remotenode->fn_opus, srchandle, ei.ei_name);
									else if (success == 2 && !(xm->xm_flags & XFER_MOVE))
										rexx_lst_select(remotenode->fn_opus, srchandle, ei.ei_name, 0);
								}
							}
						}
						// else if	(confirm_entry == 0)
						// remotenode->fn_flags |= LST_ABORT;
					}

					// Done all entries?
					if (*(p + 1) == 0)
						break;
				}

				// Need to CD out of subdir?
				if ((xm->xm_flags & XFER_DROP) && (xm->xm_flags & XFER_SUBDIR))
				{
					if (dest_list)
						free_entry_list(dest_list);
					if (remotenode->fn_protocol == FTP_PROTOCOL_SFTP)
						ftp_sftp_cdup(&remotenode->fn_sftp);
					else
						ftp_cdup(&remotenode->fn_ftp, 0, 0);
				}

				// Remove progress bar
				lister_prog_clear(prognode);

				// Show updated contents in destination
				rexx_lst_refresh(remotenode->fn_opus, desthandle, REFRESH_DATE);

				// Show updated contents in source
				if (srchandle)
					rexx_lst_refresh(remotenode->fn_opus, srchandle, REFRESH_NODATE);
			}
		}

		// Free endpoints
		if (source)
			delete_endpoint(source);
		if (dest)
			delete_endpoint(dest);

		// Remove the custom handler from the source lister
		if (msg->command == IPC_PUT && srcnode)
			rem_abort_trap(remotenode->fn_og, remotenode->fn_opus, srcnode, srchandle);
	}

	// Rescan destination?
	if (msg->command == IPC_PUT && remotenode->fn_site.se_env->e_rescan && remotenode->fn_ftp.fi_reply != 421)
		lister_list(remotenode->fn_og, remotenode, TRUE);

	// Make listers non-busy
	rexx_lst_unlock(remotenode->fn_opus, remotenode->fn_handle);

	if (xm->xm_otherhandle)
		rexx_lst_unlock(remotenode->fn_opus, xm->xm_otherhandle);

	// Trigger script
	if (remotenode->fn_og->og_hooks.dc_Script && !(remotenode->fn_flags & LST_ABORT) && timer && CheckTimer(timer))
	{
		char handle[FTP_HANDLE_BUFSIZE];

		ftp_format_handle(handle, srchandle);

		if (rexx_result && remotenode->fn_site.se_env->e_script_copy_ok)
		{
			D(bug("** script copy success\n"));
			DC_CALL2(infoptr, dc_Script, DC_REGA0, "FTP copy success", DC_REGA1, handle);
			// remotenode->fn_og->og_hooks.dc_Script( "FTP copy success", handle );
		}
		else if (!rexx_result && remotenode->fn_site.se_env->e_script_copy_fail)
		{
			D(bug("** script copy fail\n"));
			DC_CALL2(infoptr, dc_Script, DC_REGA0, "FTP copy fail", DC_REGA1, handle);
			// remotenode->fn_og->og_hooks.dc_Script( "FTP copy fail", handle );
		}
	}

	if (timer)
		FreeTimer(timer);

	// Reply to forwarded ARexx message
	if (xm->xm_rxmsg)
		reply_rexx(xm->xm_rxmsg, rexx_result, 0);

	// Free locals
	FreeVec(l);
}

/********************************/

//
//	Respond to a double-click on a lister entry.
//
void lister_doubleclick(struct ftp_node *node, IPCMessage *msg)
{
	IPTR handle = node->fn_handle;	 // Handle of the lister
	struct ftp_msg *fm;
	struct connect_msg *cm;
	struct entry_info ei = {0};
	BOOL gotfileinfo;	  // Info for this entry?
	unsigned int actual;  // Length actually read
	BOOL cwdok = FALSE;	  // TRUE if 'CWD' worked (means its a directory)
	int reply;			  // FTP reply code
	BOOL abort;			  // passed to get(), (ignored)
	struct update_info ui = {0};
	ULONG cwd_flags = CWD_RELATIVE;
	int rexx_result = 0;

	D(bug("lister_doubleclick()\n"));

	// Valid?
	if (!node || !msg || !msg->data_free)
		return;

	// Get FTP message
	fm = msg->data_free;

	// Shift-doubleclick?
	if (fm->fm_flags)
	{
		if ((cm = get_blank_connectmsg(node->fn_og)))
		{
			stccpy(cm->cm_opus, node->fn_og->og_opusname, PORTNAMELEN + 1);

			copy_site_entry(node->fn_og, &cm->cm_site, &node->fn_site);
			AddPart(cm->cm_site.se_path, fm->fm_names, PATHLEN + 1);

			IPC_Command(node->fn_og->og_main_ipc, IPC_CONNECT, 0, 0, cm, 0);
			rexx_result = 1;
		}
	}
	else
	{
		InitSemaphore(&ui.ui_sem);

		// Passive mode required?
		node->fn_ftp.fi_flags &= ~FTP_PASSIVE;
		if (node->fn_site.se_env->e_passive)
			node->fn_ftp.fi_flags |= FTP_PASSIVE;

		ui.ui_ftpnode = node;
		ui.ui_handle = handle;
		ui.ui_opus = node->fn_opus;
		ui.ui_abort = &abort;

		if ((gotfileinfo = entry_info_from_lister(node, fm->fm_names, &ei, 0)))
		{
			// Directory or link?
			if (ei.ei_type >= 0)
			{
				// Attempt to change directory

				// Don't do anything for doubleclick on '.' or '..' !
				if (strcmp(ei.ei_name, ".") && strcmp(ei.ei_name, ".."))
				{
					// Lock lister
					rexx_lst_lock(node->fn_opus, node->fn_handle);

					// Link to a directory?  Will need PWD.
					if (ei.ei_type <= 0)
						cwd_flags |= CWD_ALWAYS_SYNCH;

					// Can TIMEOUT
					if (strlen(ei.ei_comment) > 4 && !strncmp(ei.ei_comment, " -> ", 4) &&
						!strstr(ei.ei_comment + 4, "/../"))
					{
						reply = lister_cwd(node, ei.ei_comment + 4, cwd_flags);
					}
					else
						reply = lister_cwd(node, ei.ei_name, cwd_flags);

					// CD worked?
					if (reply < 500 && node->fn_ftp.fi_reply != 421)
					{
						// This is a DIR or a LINK to DIR
						cwdok = TRUE;
					}
					else if (reply != 421 && ei.ei_type != 0)
					{
						lst_server_reply(node->fn_og, node, node, MSG_FTP_PATH_NOT_FOUND);
					}

					rexx_lst_label(node->fn_opus,
								   handle,
								   node->fn_protocol == FTP_PROTOCOL_SFTP ? "SFTP:" : "FTP:",
								   node->fn_site.se_host,
								   NULL);

					rexx_lst_unlock(node->fn_opus, node->fn_handle);

					if (reply < 500 && node->fn_ftp.fi_reply != 421)
					{
						// Can TIMEOUT
						lister_list(node->fn_og, node, FALSE);
						rexx_result = 1;
					}
				}
			}

			// File or link to a file
			if (ei.ei_type < 0 || (ei.ei_type == 0 && !cwdok && node->fn_ftp.fi_reply != 421))
			{
				// Attempt to download file

				// Tempory file name
				char temp[PATHLEN] = "T:";
				AddPart(temp, ei.ei_name, PATHLEN);

				rexx_lst_lock(node->fn_opus, handle);

				// Link?
				if (ei.ei_type == 0)
				{
					// Try to find file size
					// Can TIMEOUT
					ei.ei_size = get_file_size(node, ei.ei_name);
				}

				ui.ui_bytes_so_far = 0;
				ui.ui_total_bytes = ei.ei_size;

				lister_prog_init_multi(node,
									   GetString(locale, MSG_FTP_RETRIEVING_FILE),
									   node->fn_site.se_env->e_transfer_details,
									   ei.ei_name,
									   PROGRESS_FILE_ON,
									   PROGRESS_BAR_ON);

				rexx_lst_label(node->fn_opus, handle, 0, ei.ei_name, 0);
				lister_prog_bar(node, 1, 1);

				// init the 2nd 3rd lines of display
				init_multi_display(node, node->fn_site.se_env->e_transfer_details, ei.ei_size);

				lst_addabort(node, SIGBREAKF_CTRL_D, 0);

				init_xfer_time(&ui);

				{
					// The lister holds local-charset names; the server expects UTF-8.
					char *utf8_name = ftp_convert_path_to_utf8(ei.ei_name, &node->fn_ftp);
					const char *send_name = utf8_name ? utf8_name : ei.ei_name;

					if (node->fn_protocol == FTP_PROTOCOL_SFTP)
					{
						node->fn_ftp.fi_aborted = 0;
						node->fn_ftp.fi_errno = 0;
						node->fn_ftp.fi_ioerr = 0;
						*node->fn_ftp.fi_serverr = 0;
						actual = ftp_sftp_get(&node->fn_sftp, (ftp_sftp_xfer_callback)xfer_update, &ui, send_name, temp, FALSE);
						if (ftp_sftp_session_error(&node->fn_sftp) != FTP_SFTP_ERROR_NONE)
						{
							if (ftp_sftp_session_error(&node->fn_sftp) == FTP_SFTP_ERROR_ABORTED)
								node->fn_ftp.fi_aborted = 1;
							else
							{
								node->fn_ftp.fi_errno |= FTPERR_XFER_SRCERR;
								sprintf(node->fn_ftp.fi_serverr, "550 %s\r\n", ftp_sftp_error_message(&node->fn_sftp));
							}
						}
					}
					else
						actual = get(&node->fn_ftp, (int (*)(void *, unsigned int, unsigned int))xfer_update, &ui, send_name, temp, FALSE);

					if (utf8_name)
						ftp_codesets_free(utf8_name);
				}

				lst_remabort(node);

				// If we knew the size and it matches
				if (ui.ui_total_bytes != 0xffffffff && ui.ui_total_bytes == actual)
					rexx_result = 1;

				// If we didn't know the size and there were no errors
				else if (ui.ui_total_bytes == 0xffffffff && ei.ei_type == -3 && node->fn_ftp.fi_errno == 0)
					rexx_result = 1;

				if (rexx_result)
					rexx_doubleclick(node->fn_opus, temp);

				// Report errors
				if (node->fn_ftp.fi_errno & FTPERR_XFER_DSTERR)
				{
					lst_dos_err(node->fn_og, node, 0, node->fn_ftp.fi_ioerr);
				}

				else if (node->fn_ftp.fi_errno & FTPERR_XFER_SRCERR)
					lst_server_err(node->fn_og, node, node, 0, 0);

				else if (node->fn_ftp.fi_errno)
					DisplayBeep(node->fn_og->og_screen);

					rexx_lst_label(node->fn_opus,
								   handle,
								   node->fn_protocol == FTP_PROTOCOL_SFTP ? "SFTP:" : "FTP:",
								   node->fn_site.se_host,
								   NULL);
				lister_prog_clear(node);
				rexx_lst_unlock(node->fn_opus, handle);
			}

			if (node->fn_ftp.fi_reply == 421)
				abort = TRUE;
		}
	}

	// Reply to ARexx message
	if (fm->fm_rxmsg)
		reply_rexx(fm->fm_rxmsg, rexx_result, 0);
}

/********************************/

//
//	Do an Opus command on some files eg 'SmartRead'
//
void lister_traptemp(struct ftp_node *node, IPCMessage *msg)
{
	struct traptemp_msg *tm;			 // Our message
	IPTR handle = node->fn_handle;		 // Lister handle
	char *names, name[FILENAMELEN], *p;	 // Files to act on
	BOOL gotfileinfo;					 // Real name (for links)
	struct entry_info ei = {0};
	unsigned int actual;  // Size of file actually read
	BOOL abort = FALSE;	  // Has the user aborted?
	BOOL didit;			  // We did the command to this file
	struct update_info ui = {0};
	int total, count = 1;
	int size_known = TRUE;
	int rexx_result = 0;

	tm = msg->data_free;

	if (node && msg && tm)
	{
		InitSemaphore(&ui.ui_sem);

		// Passive mode required?
		node->fn_ftp.fi_flags &= ~FTP_PASSIVE;
		if (node->fn_site.se_env->e_passive)
			node->fn_ftp.fi_flags |= FTP_PASSIVE;

		ui.ui_ftpnode = node;
		ui.ui_handle = handle;
		ui.ui_opus = node->fn_opus;
		ui.ui_abort = &abort;

		node->fn_flags &= ~LST_ABORT;

		if ((names = tm->tm_names))
		{
			rexx_lst_lock(node->fn_opus, handle);

			if ((total = rexx_lst_query_numselfiles(node->fn_opus, handle)))
			{
				lister_prog_init_multi(node,
									   GetString(locale, MSG_FTP_RETRIEVING_FILE),
									   node->fn_site.se_env->e_transfer_details,
									   "",
									   PROGRESS_FILE_ON,
									   PROGRESS_BAR_ON);

				for (p = names + 1; !(node->fn_flags & LST_ABORT); p += 3)
				{
					rexx_result = 0;
					didit = FALSE;
					p = stptok(p, name, FILENAMELEN, "\"\r\n");

					gotfileinfo = entry_info_from_lister(node, name, &ei, 0);

					if (ei.ei_type <= 0 && gotfileinfo)
					{
						char temp[PATHLEN] = "T:";

						AddPart(temp, ei.ei_name, PATHLEN);

						// A link?
						if (ei.ei_type == 0)
							ei.ei_size = get_file_size(node, ei.ei_name); /* Can TIMEOUT */

						ui.ui_flags &= ~UI_FIRSTDONE;
						ui.ui_bytes_so_far = 0;
						ui.ui_total_bytes = ei.ei_size;

						lister_prog_name(node, ei.ei_name);

						lister_prog_bar(node, total, count++);

						rexx_lst_label(node->fn_opus, handle, 0, ei.ei_name, 0);

						// init the 2nd 3rd lines of display
						init_multi_display(node, node->fn_site.se_env->e_transfer_details, ei.ei_size);

						lst_addabort(node, SIGBREAKF_CTRL_D, 0);

						init_xfer_time(&ui);

						{
							// The lister holds local-charset names; the server expects UTF-8.
							char *utf8_name = ftp_convert_path_to_utf8(ei.ei_name, &node->fn_ftp);
							const char *send_name = utf8_name ? utf8_name : ei.ei_name;

							if (node->fn_protocol == FTP_PROTOCOL_SFTP)
							{
								node->fn_ftp.fi_aborted = 0;
								node->fn_ftp.fi_errno = 0;
								node->fn_ftp.fi_ioerr = 0;
								*node->fn_ftp.fi_serverr = 0;
								actual = ftp_sftp_get(&node->fn_sftp, (ftp_sftp_xfer_callback)xfer_update, &ui, send_name, temp, FALSE);
								if (ftp_sftp_session_error(&node->fn_sftp) != FTP_SFTP_ERROR_NONE)
								{
									if (ftp_sftp_session_error(&node->fn_sftp) == FTP_SFTP_ERROR_ABORTED)
										node->fn_ftp.fi_aborted = 1;
									else
									{
										node->fn_ftp.fi_errno |= FTPERR_XFER_SRCERR;
										sprintf(node->fn_ftp.fi_serverr, "550 %s\r\n", ftp_sftp_error_message(&node->fn_sftp));
									}
								}
							}
							else
								actual = get(&node->fn_ftp, (int (*)(void *, unsigned int, unsigned int))xfer_update, &ui, send_name, temp, FALSE);

							if (utf8_name)
								ftp_codesets_free(utf8_name);
						}

						lst_remabort(node);

						// If we knew the size and it matches
						if (ui.ui_total_bytes != 0xffffffff && ui.ui_total_bytes == actual)
							rexx_result = 1;

						// If we didn't know the size and there were no errors
						else if (ui.ui_total_bytes == 0xffffffff && ei.ei_type == -3 && node->fn_ftp.fi_errno == 0)
							rexx_result = 1;

						if (rexx_result)
						{
							send_rexxa(node->fn_opus, REXX_REPLY_NONE, "command %s %s", tm->tm_command, temp);
							didit = TRUE;
						}

						// Report errors
						if (node->fn_ftp.fi_errno & FTPERR_XFER_DSTERR)
						{
							lst_dos_err(node->fn_og, node, 0, node->fn_ftp.fi_ioerr);
						}

						else if (node->fn_ftp.fi_errno & FTPERR_XFER_SRCERR)
							lst_server_err(node->fn_og, node, node, 0, 0);

						else if (node->fn_ftp.fi_errno)
							DisplayBeep(node->fn_og->og_screen);

						if (size_known != (ei.ei_size >= 0))
							lister_prog_clear(node);

						size_known = ei.ei_size >= 0;

						if (didit)
						{
							rexx_lst_select(node->fn_opus, handle, name, 0);
							ftplister_refresh(node, 0);

							if (node->fn_ftp.fi_reply == 421)
								abort = TRUE;
						}
					}

					if (*(p + 1) == 0)
						break;
				}
					rexx_lst_label(node->fn_opus,
								   handle,
								   node->fn_protocol == FTP_PROTOCOL_SFTP ? "SFTP:" : "FTP:",
								   node->fn_site.se_host,
								   NULL);
				lister_prog_clear(node);
			}
			rexx_lst_unlock(node->fn_opus, handle);
		}
	}

	if (tm && tm->tm_rxmsg)
		reply_rexx(tm->tm_rxmsg, rexx_result, 0);
}

/********************************/

//
//	Grab an index file to T:
//
BOOL lister_xferindex(struct ftp_node *ftpnode, char *localname, char *remotename, int size)
{
	IPTR handle;		  // Handle of the lister
	unsigned int actual;  // Length actually read
	BOOL abort;			  // passed to get(), (ignored)
	struct update_info ui = {0};
	BOOL retval = TRUE;

	if (!(handle = ftpnode->fn_handle))
		return FALSE;

	InitSemaphore(&ui.ui_sem);

	ui.ui_ftpnode = ftpnode;
	ui.ui_handle = handle;
	ui.ui_opus = ftpnode->fn_opus;
	ui.ui_abort = &abort;

	// Attempt to download the file

	rexx_lst_lock(ftpnode->fn_opus, handle);

	// Passive mode required?
	ftpnode->fn_ftp.fi_flags &= ~FTP_PASSIVE;
	if (ftpnode->fn_site.se_env->e_passive)
		ftpnode->fn_ftp.fi_flags |= FTP_PASSIVE;

	ui.ui_bytes_so_far = 0;
	ui.ui_total_bytes = size;

	lister_prog_init_multi(ftpnode,
						   GetString(locale, MSG_FTP_RETRIEVING_FILE),
						   ftpnode->fn_site.se_env->e_transfer_details,
						   remotename,
						   PROGRESS_FILE_ON,
						   PROGRESS_BAR_ON);

	lister_prog_bar(ftpnode, 1, 1);
	rexx_lst_label(ftpnode->fn_opus, handle, 0, remotename, 0);

	// init the 2nd 3rd lines of display
	init_multi_display(ftpnode, ftpnode->fn_site.se_env->e_transfer_details, size);

	lst_addabort(ftpnode, SIGBREAKF_CTRL_D, 0);

	init_xfer_time(&ui);

	{
		// The caller provides a local-charset name; the server expects UTF-8.
		char *utf8_name = ftp_convert_path_to_utf8(remotename, &ftpnode->fn_ftp);
		const char *send_name = utf8_name ? utf8_name : remotename;

		if (ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
		{
			ftpnode->fn_ftp.fi_aborted = 0;
			ftpnode->fn_ftp.fi_errno = 0;
			ftpnode->fn_ftp.fi_ioerr = 0;
			*ftpnode->fn_ftp.fi_serverr = 0;
			actual = ftp_sftp_get(&ftpnode->fn_sftp, (ftp_sftp_xfer_callback)xfer_update, &ui, send_name, localname, FALSE);
			if (ftp_sftp_session_error(&ftpnode->fn_sftp) != FTP_SFTP_ERROR_NONE)
			{
				if (ftp_sftp_session_error(&ftpnode->fn_sftp) == FTP_SFTP_ERROR_ABORTED)
					ftpnode->fn_ftp.fi_aborted = 1;
				else
				{
					ftpnode->fn_ftp.fi_errno |= FTPERR_XFER_SRCERR;
					sprintf(ftpnode->fn_ftp.fi_serverr, "550 %s\r\n", ftp_sftp_error_message(&ftpnode->fn_sftp));
				}
			}
		}
		else
			actual = get(&ftpnode->fn_ftp, (int (*)(void *, unsigned int, unsigned int))xfer_update, &ui, send_name, localname, FALSE);

		if (utf8_name)
			ftp_codesets_free(utf8_name);
	}

	lst_remabort(ftpnode);

	// If we got it all
	if (actual == size)
		retval = TRUE;

	// Report errors
	if (ftpnode->fn_ftp.fi_errno & FTPERR_XFER_DSTERR)
	{
		lst_dos_err(ftpnode->fn_og, ftpnode, 0, ftpnode->fn_ftp.fi_ioerr);
	}

	else if (ftpnode->fn_ftp.fi_errno & FTPERR_XFER_SRCERR)
		lst_server_err(ftpnode->fn_og, ftpnode, ftpnode, 0, 0);

	else if (ftpnode->fn_ftp.fi_errno)
		DisplayBeep(ftpnode->fn_og->og_screen);

	rexx_lst_label(ftpnode->fn_opus,
				   handle,
				   ftpnode->fn_protocol == FTP_PROTOCOL_SFTP ? "SFTP:" : "FTP:",
				   ftpnode->fn_site.se_host,
				   NULL);
	lister_prog_clear(ftpnode);

	// Do this AFTER adding comments
	// lst_unlock( ftpnode->fn_opus, handle );
	// in ftp_lister_list.c

	return retval;
}

/********************************/

//
//	Transfer between two remote FTP sites
//	ONLY the DEST lister calls this function and this function
//	'thisnode' must always point to the source node
//
void lister_getput(struct ftp_node *thisnode, IPCMessage *msg)
{
	struct xfer_msg *xm;
	struct ftp_node *srcnode, *destnode, *prognode;
	IPTR srchandle, desthandle, proghandle;
	char *names, *p;  // Quoted names of all selected entries
	int entrycount;
	int dircount = 0;
	// int                    filecount = 0;
	int entrynumber = 1;
	int quiet;
	endpoint *source, *dest;
	char srcname[FILENAMELEN + 1];	// Source name of this entry
	char newname[FILENAMELEN + 1];	// New name for CopyAs
	struct entry_info ei;			// Info on this entry
	int confirm_commence;
	int success;
	struct hook_rec_data hc = {0};
	char startdirname[FILENAMELEN + 1] = {0};
	char fulldirname[PATHLEN + 1];
	TimerHandle *timer;
	// struct rec_entry_list *dest_list = 0;
	int rexx_result = 0;
	DOpusCallbackInfo *infoptr;

	// Valid?
	if (!thisnode || !msg || !msg->data_free)
		return;

	infoptr = &thisnode->fn_og->og_hooks;

	D(bug("getput \n"));

	// Get transfer message
	xm = msg->data_free;

	// Valid message?
	if (!xm->xm_rm_src->fn_handle || !xm->xm_rm_dest->fn_handle)
		goto reply;

	// Only the source lister should process this
	if (thisnode != xm->xm_rm_src)
		goto reply;

	// Drag and drop in a single FTP lister?
	if (thisnode == xm->xm_rm_dest)
	{
		D(bug("** can't drag and drop within a single FTP lister\n"));
		DisplayBeep(thisnode->fn_og->og_screen);
		goto reply;
	}

	// Localhost being used?  This is not possible and I'm not sure why.
	// If PORT 127.0.0.1 is sent to the remote machine it would connect to itself.
	// However, even if PASV is sent to the remote machine, the remote machine
	// connects to itself.  I spent half a day trying to trace this to no avail.

	if (thisnode->fn_ftp.fi_addr.sin_addr.s_addr == 0x7f000001 ||
		xm->xm_rm_dest->fn_ftp.fi_addr.sin_addr.s_addr == 0x7f000001)
	{
		extern struct opusftp_globals og;

		lister_request_tags(thisnode, FR_MsgNum, MSG_NO_LOCALHOST, TAG_DONE);

		goto reply;
	}

	// Make listers busy (always 2 for getput)
	rexx_lst_lock(thisnode->fn_opus, thisnode->fn_handle);
	rexx_lst_lock(thisnode->fn_opus, xm->xm_rm_dest->fn_handle);

	// -ve flag 0=TRUE
	hc.hc_ui.ui_info_type = thisnode->fn_site.se_env->e_transfer_details;

	// Set up nodes and handles
	{
		srcnode = thisnode;
		destnode = xm->xm_rm_dest;
		srchandle = srcnode->fn_handle;
		desthandle = destnode->fn_handle;
	}

	// Select node and handle for progress bar (should be source)
	{
		prognode = srcnode;
		proghandle = srchandle;
	}

	// Clear abort flags
	srcnode->fn_flags &= ~LST_ABORT;
	destnode->fn_flags &= ~LST_ABORT;

	// Set up timer
	if ((timer = AllocTimer(UNIT_VBLANK, 0)))
		StartTimer(timer, srcnode->fn_site.se_env->e_script_time, 0);

	// Got any entries to transfer?
	// Will never be null
	if ((names = xm->xm_names))
	{
		entrycount = count_entries(names);

		// "Quiet" option set?
		quiet = xm->xm_flags & XFER_OPT_QUIET;

		// Don't need to trap abort in local lister for getput

		// Create endpoints
		source = create_endpoint_tags(thisnode->fn_og, EP_TYPE, ENDPOINT_FTP, EP_FTPNODE, srcnode, TAG_DONE);

		dest = create_endpoint_tags(thisnode->fn_og, EP_TYPE, ENDPOINT_FTP, EP_FTPNODE, destnode, TAG_DONE);

		if (source && dest)
		{
			char *path;

			// Get 'From' path for progress bar
			if ((path = rexx_lst_query_path(thisnode->fn_opus, srchandle)))
			{
				strcpy(fulldirname, path);
				final_path(path, startdirname);
				final_path(path, hc.hc_fromdirname);
				DeleteArgstring(path);
			}

			// Get 'To' path for progress bar
			if ((path = rexx_lst_query_path(thisnode->fn_opus, desthandle)))
			{
				final_path(path, hc.hc_todirname);
				DeleteArgstring(path);
			}

			// Does entry contain path information?
			if (xm->xm_srcpath && strpbrk(xm->xm_srcpath, ":/"))
				D(bug("** entry contains source path information\n"));

			if (xm->xm_dstpath && strpbrk(xm->xm_dstpath, ":/"))
				D(bug("** entry contains dest path information\n"));

			// Count files and directories

			// Need to query each entry?
			if (entrycount == 1 || (xm->xm_flags & (XFER_DROP | XFER_DROPFROM)))
			{
				for (p = names + 1; !(prognode->fn_flags & LST_ABORT); p += 3)
				{
					p = stptok(p, srcname, FILENAMELEN, "\"\r\n");

					// Does entry contain path information?
					if (strpbrk(srcname, ":/"))
						D(bug("** entry contains path information\n"));

					if (entry_info_from_lister(srcnode, srcname, &ei, ENTRYFROMF_DEFAULT))
					{
						if (ei.ei_type >= 0)
							++dircount;
						//	else
						//		++ filecount;
					}

					// Done all entries?
					if (*(p + 1) == 0)
						break;
				}
			}

			// Multiple entries.  Query using callbacks.
			else if (srchandle)
			{
				//	filecount = rexx_lst_query_numselfiles( thisnode->fn_opus, srchandle );
				dircount = rexx_lst_query_numseldirs(thisnode->fn_opus, srchandle);
			}

			// Multiple entries with no source lister handle??  Impossible
			else
			{
				D(bug("** copy multiple entries without source lister!\n"));
			}

			// Show warning requester for recursive copy
			if (!quiet && dircount)
			{
				confirm_commence = lister_request_tags(thisnode,
													   FR_FormatString,
													   "%s %s",
													   FR_MsgNum,
													   xm->xm_flags & XFER_MOVE ? MSG_MOVE : MSG_FTP_COPY_UCASE,
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
				confirm_commence = 1;

			if (confirm_commence)
			{
				// Show progress bar
				lister_prog_init_multi(thisnode,
									   GetString(locale, xm->xm_flags & XFER_MOVE ? MSG_FTP_MOVING : MSG_FTP_COPYING),
									   thisnode->fn_site.se_env->e_transfer_details,
									   "",
									   PROGRESS_FILE_ON,
									   PROGRESS_BAR_ON);

				// Set up copy hook data
				hc.hc_og = thisnode->fn_og;
				hc.hc_source = source;
				hc.hc_dest = dest;
				hc.hc_pre = hook_copy_pre;
				hc.hc_need_dest = 1;  // Do we need to LIST the destination?
				hc.hc_opus = thisnode->fn_opus;
				hc.hc_prognode = prognode;
				hc.hc_proghandle = proghandle;
				hc.hc_env = srcnode->fn_site.se_env;
				hc.hc_basedirname = startdirname;
				hc.hc_copy_flags = xm->xm_flags;
				hc.hc_ored_ftp_flags = srcnode->fn_ftp.fi_flags | destnode->fn_ftp.fi_flags;
				hc.hc_anded_ftp_flags = srcnode->fn_ftp.fi_flags & destnode->fn_ftp.fi_flags;

				// (Previously toggled via DOpus/ftp_broken_list env var; now a per-site flag.)
				if (srcnode->fn_site.se_env->e_recursive_special)
					hc.hc_recur_flags |= RECURF_BROKEN_LS;

				if (confirm_commence == 2)
					hc.hc_recur_flags |= RECURF_NORECUR;

				if (srcnode->fn_site.se_env->e_copy_replace == COPY_REPLACE_ALWAYS)
				{
					hc.hc_recur_flags |= RECURF_REPLACEALL;
					// hc.hc_need_dest = 0;
				}
				else if (srcnode->fn_site.se_env->e_copy_replace == COPY_REPLACE_NEVER)
					hc.hc_recur_flags |= RECURF_SKIPALL;

				hc.hc_options = lister_options(srcnode, OPTION_COPY);

				// Should use callbacks to get entries
				// (except drag'n'drop coz they won't be highlit)
				for (p = names + 1; !(prognode->fn_flags & LST_ABORT); p += 3)
				{
					int confirm_entry = 1;

					rexx_result = 0;

					// Next source entry name
					p = stptok(p, srcname, FILENAMELEN, "\"\r\n");

					// Update progress bar
					lister_prog_bar(thisnode, entrycount, entrynumber++);

					if (xm->xm_flags & XFER_AS)
					{
						// Old and new names provided?
						if (xm->xm_newname && xm->xm_flags & XFER_OPT_NAME)
						{
							strcpy(newname, xm->xm_newname);
							hc.hc_newname = newname;
						}
						else
						{
							switch (request_newname(prognode, newname, srcname, xm->xm_flags & XFER_MOVE))
							{
							case 0:
								*newname = 0;
								confirm_entry = 0;
								break;
							case 1:
							case 65535:
								hc.hc_newname = newname;
								break;
							case 2:
								*newname = 0;
								prognode->fn_flags |= LST_ABORT;
								confirm_entry = 0;
								break;
							default:
								*newname = 0;
								confirm_entry = 0;
								break;
							}
						}
					}

					// Plain Copy/Move, not CopyAs/MoveAs
					else
					{
						*newname = 0;
					}

					// Get file info
					if (confirm_entry && entry_info_from_lister(srcnode, srcname, &ei, ENTRYFROMF_DEFAULT))
					{
						// Confirm dirs?

						// Confirm files?

						// Transfer all?  Show no more confirmation requesters
						// if	(confirm_entry == ?)
						//	quiet = 1;

						// if	(confirm_entry == ?)
						{
							// Transfer entry (and contents if it's a dir)
							if ((success = recursive_copy(&hc, startdirname, fulldirname, &ei, 0)))
							{
								// Set ARexx result
								if (success == 3 && xm->xm_flags & XFER_MOVE)
									rexx_result = 1;
								else if (success == 2 && !(xm->xm_flags & XFER_MOVE))
									rexx_result = 1;

								// Add entry to destination
								// TODO add new name for CopyAs/MoveAs
								// Convert link types to non-link types
								if (!(xm->xm_flags & XFER_SUBDIR))
									rexx_lst_add(thisnode->fn_opus,
												 desthandle,
												 *newname ? newname : ei.ei_name,
												 ei.ei_size,
												 ei.ei_type < 0 ? -1 : 1,
												 hc.hc_options & COPY_DATE ? ei.ei_seconds : 0,
												 hc.hc_options & COPY_PROT ? ei.ei_prot : 0,
												 "");

								// Deselect or remove entry from source
								if (srchandle)
								{
									if (success == 3 && xm->xm_flags & XFER_MOVE)
										rexx_lst_remove(thisnode->fn_opus, srchandle, ei.ei_name);
									else if (success == 2 && !(xm->xm_flags & XFER_MOVE))
										rexx_lst_select(thisnode->fn_opus, srchandle, ei.ei_name, 0);
								}
							}
						}
						// else if	(confirm_entry == 0)
						// remotenode->fn_flags |= LST_ABORT;
					}

					// Done all entries?
					if (*(p + 1) == 0)
						break;
				}

				// Remove progress bar
				lister_prog_clear(thisnode);

				// Show updated contents in destination
				// rexx_lst_refresh( thisnode->fn_opus, desthandle, REFRESH_DATE );
				ftplister_refresh(destnode, 0);
				ftplister_refresh(destnode, HOOKREFRESH_DATE);

				// Show updated contents in source
				ftplister_refresh(srcnode, 0);
			}
		}

		// Free endpoints
		if (source)
			delete_endpoint(source);
		if (dest)
			delete_endpoint(dest);

		// No abort handler to remove for getput
	}

	/* NOTE: BUG
		destnode->fn_site can be null if dest has timedout!
		Causes mungwall hits
		why?

	*/

	// Rescan destination?
	if (destnode->fn_site.se_env->e_rescan && destnode->fn_ftp.fi_reply != 421)
		IPC_Command(destnode->fn_ipc, IPC_REREAD, 0, 0, 0, 0);

	// Make listers non-busy
	rexx_lst_unlock(thisnode->fn_opus, thisnode->fn_handle);
	rexx_lst_unlock(thisnode->fn_opus, xm->xm_rm_dest->fn_handle);

	// Trigger script
	if (thisnode->fn_og->og_hooks.dc_Script && !(prognode->fn_flags & LST_ABORT) && timer && CheckTimer(timer))
	{
		char handle[FTP_HANDLE_BUFSIZE];

		ftp_format_handle(handle, srchandle);

		if (rexx_result && (srcnode->fn_site.se_env->e_script_copy_ok || destnode->fn_site.se_env->e_script_copy_ok))
		{
			DC_CALL2(infoptr, dc_Script, DC_REGA0, "FTP copy success", DC_REGA1, handle);
			// thisnode->fn_og->og_hooks.dc_Script( "FTP copy success", handle );
		}
		else if (srcnode->fn_site.se_env->e_script_copy_fail || destnode->fn_site.se_env->e_script_copy_fail)
		{
			DC_CALL2(infoptr, dc_Script, DC_REGA0, "FTP copy fail", DC_REGA1, handle);
			// thisnode->fn_og->og_hooks.dc_Script( "FTP copy fail", handle );
		}
	}

	if (timer)
		FreeTimer(timer);

reply:

	// Reply to forwarded ARexx message
	if (xm->xm_rxmsg)
		reply_rexx(xm->xm_rxmsg, rexx_result, 0);
}
