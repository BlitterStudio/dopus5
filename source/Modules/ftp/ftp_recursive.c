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

//
//	Recursive FTP commands!  Can you believe it?
//
// gp 4/5/98 changed site-site selectwait time to 1 secs from 20 micros

#include <dopus/common.h>
#include <proto/dopus5.h>
#include <proto/module.h>

#include "ftp.h"
#include "ftp_recursive.h"
#include "ftp_opusftp.h"
#include "ftp_util.h"
#include "ftp_list.h"
#include "ftp_listcmd.h"
#include "ftp_ipc.h"
#include "ftp_utf8.h"

#include "ftp_arexx.h"
#include "ftp_lister.h"
#include "ftp_lister_xfer.h"

#include "ftp_addressbook.h"
#include "ftp_addrsupp_protos.h"

#define min(a, b) (((a) <= (b)) ? (a) : (b))

static unsigned int recursive_getput(endpoint *source,
									 endpoint *dest,
									 int (*updatefn)(void *, unsigned int, unsigned int),
									 void *updateinfo,
									 char *remote_path,
									 char *local_path,
									 unsigned int restart);
static unsigned int recursive_getput_file(endpoint *ep,
										  int favour,
										  int (*updatefn)(void *, unsigned int, unsigned int),
										  void *updateinfo,
										  char *remote_path,
										  char *local_path,
										  unsigned int restart);
static unsigned int recursive_getput_via_temp(struct hook_rec_data *hc,
											  struct entry_info *entry,
											  char *destname,
											  BOOL resume);
static void recursive_getput_clear_errors(endpoint *source, endpoint *dest);
// static int callback_func( void *, char *str );
static IPTR rec_ask_lister_favour(int favour, endpoint *, void *arg1, void *arg2);

/**
 *{	Endpoint functions
 **/

//
//	Allocate and fill in an endpoint structure
//
endpoint *create_endpoint(struct opusftp_globals *og, struct TagItem *tags)
{
	endpoint *ep;
	int success = 1;
	int type;
	struct ftp_node *ftpn;
	char *path;

	if (!tags)
		return 0;

	type = GetTagData(EP_TYPE, -1, tags);

	if (type != ENDPOINT_FILESYS && type != ENDPOINT_FTP)
		return 0;

	if (!(ep = AllocVec(sizeof(endpoint), MEMF_CLEAR)))
		return 0;

	ep->ep_type = type;

	if (type == ENDPOINT_FTP)
	{
		if ((ftpn = (struct ftp_node *)GetTagData(EP_FTPNODE, 0, tags)))
		{
			ep->ep_ftpnode = ftpn;
			ep->ep_list = rec_ftp_list;
			ep->ep_cwd = rec_ftp_cwd;
			ep->ep_cdup = rec_ftp_cdup;
			ep->ep_mkdir = rec_ftp_mkdir;
			ep->ep_dele = rec_ftp_dele;
			ep->ep_rmd = rec_ftp_rmd;
			ep->ep_port = rec_ftp_port;
			ep->ep_pasv = rec_ftp_pasv;
			ep->ep_rest = rec_ftp_rest;
			ep->ep_retr = rec_ftp_retr;
			ep->ep_stor = rec_ftp_stor;
			ep->ep_abor = rec_ftp_abor;
			ep->ep_setdate = (int (*)(endpoint *, char *, ULONG))rec_ftp_dummy;
			ep->ep_chmod = rec_ftp_chmod;
			ep->ep_setnote = (int (*)(endpoint *, char *, char *))rec_ftp_dummy;
			ep->ep_setprot = (int (*)(endpoint *, char *, LONG))rec_ftp_dummy;
			ep->ep_select = rec_ftp_select;
			ep->ep_getreply = rec_ftp_getreply;
			ep->ep_getentry = rec_ftp_getentry;
			ep->ep_errorreq = rec_ftp_errorreq;
			ep->ep_opts = rec_ftp_opts;
		}
		else
			success = 0;
	}
	else
	{
		if (!(ftpn = (struct ftp_node *)GetTagData(EP_FTPNODE, 0, tags)))
			if ((ftpn = AllocVec(sizeof(struct ftp_node), MEMF_CLEAR)))
				ep->ep_freenode = TRUE;

		if (ftpn)
		{
			ep->ep_ftpnode = ftpn;
			ep->ep_ftpnode->fn_og = og;
			if ((path = (char *)GetTagData(EP_PATH, 0, tags)))
			{
				BPTR lock;

				stccpy(ep->ep_ftpnode->fn_site.se_path, path, PATHLEN + 1);

				if ((lock = Lock(path, ACCESS_READ)))
				{
					UnLock(CurrentDir(lock));

					ep->ep_list = rec_filesys_list;
					ep->ep_cwd = rec_filesys_cwd;
					ep->ep_cdup = rec_filesys_cdup;
					ep->ep_mkdir = rec_filesys_mkdir;
					ep->ep_dele = rec_filesys_dele;
					ep->ep_rmd = rec_filesys_rmd;
					ep->ep_port = rec_filesys_port;
					ep->ep_pasv = rec_filesys_pasv;
					ep->ep_rest = rec_filesys_rest;
					ep->ep_retr = rec_filesys_retr;
					ep->ep_stor = rec_filesys_stor;
					ep->ep_abor = (int (*)(endpoint *))rec_filesys_dummy;
					ep->ep_setdate = (int (*)(endpoint *, char *, ULONG))rec_filesys_setdate;
					ep->ep_chmod = rec_filesys_chmod;
					ep->ep_setnote = (int (*)(endpoint *, char *, char *))rec_filesys_setnote;
					ep->ep_setprot = (int (*)(endpoint *, char *, LONG))rec_filesys_setprot;
					ep->ep_select = (int (*)(endpoint *))rec_filesys_dummy;
					ep->ep_getreply = rec_filesys_getreply;
					ep->ep_getentry = rec_filesys_getentry;
					ep->ep_errorreq = rec_filesys_errorreq;
					ep->ep_opts = rec_filesys_opts;
				}
				else
					success = 0;
			}
		}
		else
			success = 0;
	}

	if (!success)
	{
		if (ep)
			FreeVec(ep);
		ep = 0;
	}

	return ep;
}

//
//	Free an endpoint structure
//
void delete_endpoint(endpoint *ep)
{
	if (ep)
	{
		if (ep->ep_freenode && ep->ep_ftpnode)
			FreeVec(ep->ep_ftpnode);

		FreeVec(ep);
	}
}

/**
 }{	Recursive copy functions
 **/

//
//	Keep retrying a DELE as long as user presses "Try Again"...
//
//	Returns 0/1 for success
//
int rec_retry_dele(endpoint *ep, struct ftp_node *prognode, struct entry_info *entry)
{
	int reqresult = 1;
	int result = 0;

	// Aborted?
	if (prognode->fn_flags & LST_ABORT)
		return 0;

	while (reqresult == 1)
	{
		reqresult = -1;

		if (ep->ep_dele(ep, entry))
			result = 1;
		else
			reqresult = ep->ep_errorreq(ep, prognode, ERRORREQ_RETRYABORT);
	}

	// Aborted?
	if (reqresult == 0)
		prognode->fn_flags |= LST_ABORT;

	return result;
}

//
//	Keep retrying a RMD as long as user presses "Try Again"...
//
//	Returns 0/1 for success
//
int rec_retry_rmd(endpoint *ep, struct ftp_node *prognode, struct entry_info *entry)
{
	int reqresult = 1;
	int result = 0;

	// Aborted?
	if (prognode->fn_flags & LST_ABORT)
		return 0;

	while (reqresult == 1)
	{
		reqresult = -1;

		if (ep->ep_rmd(ep, entry))
			result = 1;
		else
			reqresult = ep->ep_errorreq(ep, prognode, ERRORREQ_RETRYABORT);
	}

	// Aborted?
	if (reqresult == 0)
		prognode->fn_flags |= LST_ABORT;

	return result;
}

//
//	Keep retrying a GET as long as user presses "Try Again"...
//
static int rec_retry_get(struct hook_rec_data *hc,
						 char *startdirname,
						 char *fulldirname,
						 struct entry_info *entry,
						 BOOL resume)
{
	ULONG actual;
	ULONG options;
	int reqresult = 1;
	int retval = 0;
	char *destname;

	// Aborted?
	if (hc->hc_prognode->fn_flags & LST_ABORT)
		return 0;

	// Passive mode required?
	hc->hc_source->ep_ftpnode->fn_ftp.fi_flags &= ~FTP_PASSIVE;
	if (hc->hc_source->ep_ftpnode->fn_site.se_env->e_passive)
		hc->hc_source->ep_ftpnode->fn_ftp.fi_flags |= FTP_PASSIVE;

	while (reqresult == 1)
	{
		reqresult = -1;

		hc->hc_ui.ui_bytes_so_far = 0;
		hc->hc_ui.ui_total_bytes = entry->ei_size;
		hc->hc_ui.ui_flags &= ~UI_FIRSTDONE;

		init_xfer_time(&hc->hc_ui);

		lst_addabort(hc->hc_source->ep_ftpnode, SIGBREAKF_CTRL_D, 0);

		// Use new name for CopyAs/MoveAs at top level
		if (hc->hc_basedirname == startdirname && hc->hc_newname)
			destname = hc->hc_newname;
		else
			destname = entry->ei_name;

		actual = recursive_getput_file(hc->hc_source,
									   FAVOUR_GET_FILE,
									   (int (*)(void *, unsigned int, unsigned int))xfer_update,
									   &hc->hc_ui,
									   entry->ei_name,
									   destname,
									   resume);

		D(bug("** GET errno=%ld, actual=%lu\n", (long)hc->hc_source->ep_ftpnode->fn_ftp.fi_errno, (unsigned long)actual));

		// Any errors?
		switch (hc->hc_source->ep_ftpnode->fn_ftp.fi_errno & FTPERR_XFER_MASK)
		{
		case FTPERR_XFER_SRCERR:
			D(bug("** get src err\n"));
			reqresult = hc->hc_source->ep_errorreq(hc->hc_source, hc->hc_prognode, ERRORREQ_RETRYABORT);
			retval = 0;
			break;
		case FTPERR_XFER_DSTERR:
			reqresult = hc->hc_dest->ep_errorreq(hc->hc_dest, hc->hc_prognode, ERRORREQ_RETRYABORT);
			retval = 0;
			break;
		case FTPERR_XFER_RARERR:
			retval = 0;
			break;
		case 0:
			// Update size
			entry->ei_size = actual;

			// Do Copy options
			options = hc->hc_source->ep_opts(hc->hc_source, OPTION_COPY);

			D(bug("opts %lx\n", (unsigned long)options));

			// Copy Date? (only possible on filesystem)
			if (options & COPY_DATE)
				hc->hc_dest->ep_setdate(hc->hc_dest, destname, entry->ei_seconds);

			// Copy Protection bits? (possible on wu-ftpd servers)
			if (options & COPY_PROT)
				hc->hc_dest->ep_chmod(hc->hc_dest, destname, entry->ei_unixprot);

			// Copy FileNote? (comment) (only possible on filesystem)

			// Abort message in comment field?
			if (hc->hc_prognode->fn_flags & LST_ABORT)
			{
				D(bug("Adding 'aborted' comment\n"));

				lsprintf(hc->hc_url, GetString(locale, MSG_ABORT_COMMENT_START));
				if (hc->hc_ui.ui_total_bytes == 0xffffffff)
					strcat(hc->hc_url, GetString(locale, MSG_ABORT_COMMENT_UNKNOWN_SIZE));
				else
					lsprintf(hc->hc_url + strlen(hc->hc_url), "%lu", hc->hc_ui.ui_total_bytes);

				lsprintf(hc->hc_url + strlen(hc->hc_url), GetString(locale, MSG_ABORT_COMMENT_END));

				D(bug("  comment: '%s'\n", hc->hc_url));

				hc->hc_dest->ep_setnote(hc->hc_dest, destname, hc->hc_url);
			}

			// URL in comment field?
			else if (hc->hc_env->e_url_comment)
			{
				build_url(hc->hc_url,
						  hc->hc_source->ep_ftpnode->fn_site.se_user,
						  0,
						  hc->hc_source->ep_ftpnode->fn_site.se_host,
						  hc->hc_source->ep_ftpnode->fn_site.se_port,
						  fulldirname,
						  0);

				hc->hc_dest->ep_setnote(hc->hc_dest, destname, hc->hc_url);
			}

			else if (options & COPY_NOTE)
				hc->hc_dest->ep_setnote(hc->hc_dest, destname, entry->ei_comment);

			// Set source archive bit?
			// if	(options & COPY_ARC)
			//	hc->hc_source->ep_setprot( hc->hc_source, entry->ei_name, FIBF_ARCHIVE );

			break;
		}

		// Completely successful?
		if (actual == entry->ei_size)
		{
			retval = 2;
			// If Move, delete from source
			if (hc->hc_copy_flags & XFER_MOVE)
			{
				if (rec_retry_dele(hc->hc_source, hc->hc_prognode, entry))
					retval = 3;
			}
		}

		lst_remabort(hc->hc_source->ep_ftpnode);
	}

	// Aborted?
	if (reqresult == 0)
		hc->hc_prognode->fn_flags |= LST_ABORT;

	return retval;
}

//
//	Keep retrying a PUT as long as user presses "Try Again"...
//
static int rec_retry_put(struct hook_rec_data *hc, struct entry_info *entry, char *startdirname, BOOL resume)
{
	ULONG actual;
	ULONG options;
	int reqresult = 1;
	int retval = 0;
	char destname[PATHLEN + 1];

	D(bug("rec_retry_put()\n"));
	D(bug("entry: '%s'\n", entry->ei_name));
	D(bug("startdir: '%s'\n", startdirname));

	// Aborted?
	if (hc->hc_prognode->fn_flags & LST_ABORT)
		return 0;

	// Passive mode required?
	hc->hc_dest->ep_ftpnode->fn_ftp.fi_flags &= ~FTP_PASSIVE;
	if (hc->hc_dest->ep_ftpnode->fn_site.se_env->e_passive)
		hc->hc_dest->ep_ftpnode->fn_ftp.fi_flags |= FTP_PASSIVE;

	while (reqresult == 1)
	{
		reqresult = -1;

		hc->hc_ui.ui_bytes_so_far = 0;
		hc->hc_ui.ui_total_bytes = entry->ei_size;
		hc->hc_ui.ui_flags &= ~UI_FIRSTDONE;

		init_xfer_time(&hc->hc_ui);

		lst_addabort(hc->hc_source->ep_ftpnode, SIGBREAKF_CTRL_D, 0);

		// Use new name for CopyAs/MoveAs at top level
		if (hc->hc_basedirname == startdirname && hc->hc_newname)
			stccpy(destname, hc->hc_newname, PATHLEN + 1);
		else
			stccpy(destname, entry->ei_name, PATHLEN + 1);

		actual = recursive_getput_file(hc->hc_dest,
									   FAVOUR_PUT_FILE,
									   (int (*)(void *, unsigned int, unsigned int))xfer_update,
									   &hc->hc_ui,
									   destname,
									   entry->ei_name,
									   resume ? hc->hc_misc_bytes : 0);

		D(bug("** PUT errno=%ld, actual=%lu\n", (long)hc->hc_source->ep_ftpnode->fn_ftp.fi_errno, (unsigned long)actual));

		// Any errors?
		switch (hc->hc_dest->ep_ftpnode->fn_ftp.fi_errno & FTPERR_XFER_MASK)
		{
		case FTPERR_XFER_DSTERR:
			retval = 0;
			reqresult = hc->hc_dest->ep_errorreq(hc->hc_dest, hc->hc_prognode, ERRORREQ_RETRYABORT);
			break;
		case FTPERR_XFER_SRCERR:
			retval = 0;
			reqresult = hc->hc_source->ep_errorreq(hc->hc_source, hc->hc_prognode, ERRORREQ_RETRYABORT);
			break;
		case FTPERR_XFER_RARERR:
			retval = 0;
			break;
		case 0:
			// Update size
			entry->ei_size = actual;

			// Do Copy options
			options = hc->hc_dest->ep_opts(hc->hc_dest, OPTION_COPY);

			// Copy Date? (only possible on filesystem)
			// if	(options & COPY_DATE)
			//	hc->hc_dest->ep_setdate( hc->hc_dest, destname, entry->ei_seconds );

			// Copy Protection bits? (possible on wu-ftpd servers)
			if (options & COPY_PROT)
				hc->hc_dest->ep_chmod(hc->hc_dest, destname, entry->ei_unixprot);

			// Copy FileNote? (comment) (only possible on filesystem)
			// if	(options & COPY_NOTE)
			//	hc->hc_dest->ep_setnote( hc->hc_dest, destname, entry->ei_comment );

			// Set source archive bit?
			if (options & COPY_ARC)
				hc->hc_source->ep_setprot(hc->hc_source, entry->ei_name, FIBF_ARCHIVE);

			break;
		}

		// Completely successful?
		if (actual == entry->ei_size)
		{
			retval = 2;
			// If Move, delete from source
			if (hc->hc_copy_flags & XFER_MOVE)
			{
				if (rec_retry_dele(hc->hc_source, hc->hc_prognode, entry))
					retval = 3;
			}
		}

		lst_remabort(hc->hc_source->ep_ftpnode);
	}

	// Aborted?
	if (reqresult == 0)
		hc->hc_prognode->fn_flags |= LST_ABORT;

	return retval;
}

//
//	Keep retrying a GETPUT as long as user presses "Try Again"...
//
static int rec_retry_getput(struct hook_rec_data *hc, struct entry_info *entry, char *startdirname, BOOL resume)
{
	ULONG actual;
	ULONG options;
	int reqresult = 1;
	int retval = 0;
	char *destname;

	D(bug("rec_retry_getput()\n"));

	// Aborted?
	if (hc->hc_prognode->fn_flags & LST_ABORT)
		return 0;

	while (reqresult == 1)
	{
		int transfer_aborted = 0;

		reqresult = -1;

		// Use new name for CopyAs/MoveAs at top level
		if (hc->hc_basedirname == startdirname && hc->hc_newname)
			destname = hc->hc_newname;
		else
			destname = entry->ei_name;

		if (ftp_tls_modes_allow_server_transfer(hc->hc_source->ep_ftpnode->fn_ftp.fi_tls_mode,
												hc->hc_dest->ep_ftpnode->fn_ftp.fi_tls_mode))
		{
			hc->hc_ui.ui_bytes_so_far = 0;
			// hc->hc_ui.ui_total_bytes = entry->ei_size;
			hc->hc_ui.ui_total_bytes = 3;
			hc->hc_ui.ui_flags &= ~UI_FIRSTDONE;

			init_xfer_time(&hc->hc_ui);

			// We use the destination lister for abort signalling
			// because it's the operating process during a direct getput transfer
			lst_addabort(hc->hc_source->ep_ftpnode,
						 SIGBREAKF_CTRL_D,
						 (struct Task *)hc->hc_dest->ep_ftpnode->fn_ipc->proc);

			actual = recursive_getput(hc->hc_source,
									  hc->hc_dest,
									  (int (*)(void *, unsigned int, unsigned int))getput_update,
									  &hc->hc_ui,
									  entry->ei_name,
									  destname,
									  resume ? entry->ei_size : 0);

			lst_remabort(hc->hc_source->ep_ftpnode);

			if (actual == REC_GETPUT_ERROR_START && !hc->hc_source->ep_ftpnode->fn_ftp.fi_aborted &&
				!hc->hc_dest->ep_ftpnode->fn_ftp.fi_aborted)
			{
				D(bug("** getput direct setup failed; trying temp fallback\n"));
				recursive_getput_clear_errors(hc->hc_source, hc->hc_dest);
				actual = recursive_getput_via_temp(hc, entry, destname, resume);
			}
		}
		else
			actual = recursive_getput_via_temp(hc, entry, destname, resume);

		D(bug("** getput actual 1 %ld\n", (long)actual));

		// Fix value of actual (getput returns special values)

		// Transfer successful?  'actual' should be file size
		if (actual == REC_GETPUT_OK)
			actual = entry->ei_size;

		// Transfer aborted?  'actual' should be less than file size
		else if (actual == REC_GETPUT_ABORTED)
		{
			actual = entry->ei_size / 2;
			transfer_aborted = 1;
		}

		// Transfer error at beginning of transfer?  'actual' should be 0
		else if (actual == REC_GETPUT_ERROR_START)
			actual = 0;

		// Transfer error at end of transfer?  'actual' should be less than file size
		else if (actual == REC_GETPUT_ERROR_END)
			actual = entry->ei_size / 2;

		D(bug("** getput actual 2 %ld\n", (long)actual));

		if (transfer_aborted)
		{
			reqresult = 0;
			retval = 0;
		}
		else
		{
			//	endpoint *ep;

			int fooerrno = 0;

			if (hc->hc_source->ep_ftpnode->fn_ftp.fi_errno)
			{
				fooerrno = hc->hc_source->ep_ftpnode->fn_ftp.fi_errno;
				//	ep = hc->hc_source;
			}
			else if (hc->hc_dest->ep_ftpnode->fn_ftp.fi_errno)
			{
				fooerrno = hc->hc_dest->ep_ftpnode->fn_ftp.fi_errno;
				//	ep = hc->hc_dest;
			}

			// Any errors?
			switch (fooerrno & FTPERR_XFER_MASK)
			{
			case FTPERR_XFER_SRCERR:
				D(bug("FTPERR_XFER_SRCERR\n"));
				reqresult = hc->hc_source->ep_errorreq(hc->hc_source, hc->hc_prognode, ERRORREQ_RETRYABORT);
				retval = 0;
				break;

			case FTPERR_XFER_DSTERR:
				D(bug("FTPERR_XFER_DSTERR\n"));
				reqresult = hc->hc_dest->ep_errorreq(hc->hc_dest, hc->hc_prognode, ERRORREQ_RETRYABORT);
				retval = 0;
				break;

			case FTPERR_XFER_RARERR:
				D(bug("FTPERR_XFER_RARERR\n"));
				hc->hc_source->ep_errorreq(hc->hc_source, hc->hc_prognode, 0);
				reqresult = 0;
				retval = 0;
				break;

			default:
				D(bug("** getput unknown error\n"));
				reqresult = 0;
				retval = 0;
				break;
			case 0:
				D(bug("** getput no errors\n"));

				// Update size
				entry->ei_size = actual;

				// Do Copy options
				options = hc->hc_source->ep_opts(hc->hc_source, OPTION_COPY);

				// Copy Date? (only possible on filesystem)
				// if	(options & COPY_DATE)
				//	hc->hc_dest->ep_setdate( hc->hc_dest, destname, entry->ei_seconds );

				// Copy Protection bits? (possible on wu-ftpd servers)
				if (options & COPY_PROT)
					hc->hc_dest->ep_chmod(hc->hc_dest, destname, entry->ei_unixprot);

				// Copy FileNote? (comment) (only possible on filesystem)
				// if	(options & COPY_NOTE)
				//	hc->hc_dest->ep_setnote( hc->hc_dest, destname, entry->ei_comment );

				// Set source archive bit?
				// if	(options & COPY_ARC)
				//	hc->hc_source->ep_setprot( hc->hc_source, entry->ei_name, FIBF_ARCHIVE );

				break;
			}
			// Completely successful?
			if (actual == entry->ei_size)
			{
				retval = 2;

				// If Move, delete from source
				if (hc->hc_copy_flags & XFER_MOVE)
				{
					if (rec_retry_dele(hc->hc_source, hc->hc_prognode, entry))
						retval = 3;
				}
			}
		}

	}

	// Aborted?
	if (reqresult == 0)
		hc->hc_prognode->fn_flags |= LST_ABORT;

	return retval;
}

static void recursive_getput_clear_errors(endpoint *source, endpoint *dest)
{
	source->ep_ftpnode->fn_ftp.fi_aborted = 0;
	source->ep_ftpnode->fn_ftp.fi_errno = 0;
	source->ep_ftpnode->fn_ftp.fi_ioerr = 0;
	*source->ep_ftpnode->fn_ftp.fi_serverr = 0;

	dest->ep_ftpnode->fn_ftp.fi_aborted = 0;
	dest->ep_ftpnode->fn_ftp.fi_errno = 0;
	dest->ep_ftpnode->fn_ftp.fi_ioerr = 0;
	*dest->ep_ftpnode->fn_ftp.fi_serverr = 0;

	errno = 0;
}

static void recursive_getput_xfer_ui(struct hook_rec_data *hc, unsigned int total)
{
	hc->hc_ui.ui_bytes_so_far = 0;
	hc->hc_ui.ui_total_bytes = total;
	hc->hc_ui.ui_flags &= ~UI_FIRSTDONE;

	init_xfer_time(&hc->hc_ui);
}

static unsigned int recursive_getput_file(endpoint *ep,
										  int favour,
										  int (*updatefn)(void *, unsigned int, unsigned int),
										  void *updateinfo,
										  char *remote_path,
										  char *local_path,
										  unsigned int restart)
{
	struct rec_favour_xfer xfer;
	// The server speaks UTF-8 when advertised; the lister works in local charset.
	// Convert once and use for both the direct calls and the forwarded xfer.
	char *utf8_remote = ftp_convert_path_to_utf8(remote_path, &ep->ep_ftpnode->fn_ftp);
	const char *send_remote = utf8_remote ? utf8_remote : remote_path;
	unsigned int result;

	if (ep->ep_ftpnode->fn_ftp.fi_task == FindTask(0))
	{
		if (favour == FAVOUR_GET_FILE)
		{
			if (ep->ep_ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
			{
				ep->ep_ftpnode->fn_ftp.fi_aborted = 0;
				ep->ep_ftpnode->fn_ftp.fi_errno = 0;
				ep->ep_ftpnode->fn_ftp.fi_ioerr = 0;
				*ep->ep_ftpnode->fn_ftp.fi_serverr = 0;
				result = ftp_sftp_get(&ep->ep_ftpnode->fn_sftp,
									  updatefn,
									  updateinfo,
									  send_remote,
									  local_path,
									  restart);
				if (ftp_sftp_session_error(&ep->ep_ftpnode->fn_sftp) != FTP_SFTP_ERROR_NONE)
				{
					if (ftp_sftp_session_error(&ep->ep_ftpnode->fn_sftp) == FTP_SFTP_ERROR_ABORTED)
						ep->ep_ftpnode->fn_ftp.fi_aborted = 1;
					else
					{
						ep->ep_ftpnode->fn_ftp.fi_errno |= FTPERR_XFER_SRCERR;
						sprintf(ep->ep_ftpnode->fn_ftp.fi_serverr,
								"550 %s\r\n",
								ftp_sftp_error_message(&ep->ep_ftpnode->fn_sftp));
					}
				}
				goto done;
			}
			result = get(&ep->ep_ftpnode->fn_ftp, updatefn, updateinfo, (char *)send_remote, local_path, restart);
			goto done;
		}

		if (ep->ep_ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
		{
			ep->ep_ftpnode->fn_ftp.fi_aborted = 0;
			ep->ep_ftpnode->fn_ftp.fi_errno = 0;
			ep->ep_ftpnode->fn_ftp.fi_ioerr = 0;
			*ep->ep_ftpnode->fn_ftp.fi_serverr = 0;
			result = ftp_sftp_put(&ep->ep_ftpnode->fn_sftp,
								  updatefn,
								  updateinfo,
								  local_path,
								  send_remote,
								  restart);
			if (ftp_sftp_session_error(&ep->ep_ftpnode->fn_sftp) != FTP_SFTP_ERROR_NONE)
			{
				if (ftp_sftp_session_error(&ep->ep_ftpnode->fn_sftp) == FTP_SFTP_ERROR_ABORTED)
					ep->ep_ftpnode->fn_ftp.fi_aborted = 1;
				else
				{
					ep->ep_ftpnode->fn_ftp.fi_errno |= FTPERR_XFER_DSTERR;
					sprintf(ep->ep_ftpnode->fn_ftp.fi_serverr,
							"550 %s\r\n",
							ftp_sftp_error_message(&ep->ep_ftpnode->fn_sftp));
				}
			}
			goto done;
		}
		result = put(&ep->ep_ftpnode->fn_ftp, updatefn, updateinfo, local_path, (char *)send_remote, restart);
		goto done;
	}

	xfer.updatefn = NULL;
	xfer.updateinfo = NULL;
	// The IPC favour handlers read but never modify remote_path; cast away const.
	xfer.remote_path = (char *)send_remote;
	xfer.local_path = local_path;
	xfer.restart = restart;

	// rec_ask_lister_favour is synchronous; send_remote stays valid for the whole call.
	result = rec_ask_lister_favour(favour, ep, &xfer, 0);

done:
	if (utf8_remote)
		ftp_codesets_free(utf8_remote);
	return result;
}

static void recursive_getput_temp_error(struct ftp_info *info, ULONG xfer_error, int msg)
{
	info->fi_errno &= ~FTPERR_XFER_MASK;
	info->fi_errno |= xfer_error;
	stccpy(info->fi_serverr, GetString(locale, msg), IOBUFSIZE + 1);
}

static unsigned int recursive_getput_via_temp(struct hook_rec_data *hc,
											  struct entry_info *entry,
											  char *destname,
											  BOOL resume)
{
	BPTR temp_file;
	char tempname[PATHLEN + 1] = {0};
	unsigned int actual = 0;
	unsigned int result = REC_GETPUT_ERROR_START;

	D(bug("recursive_getput_via_temp()\n"));

	if (!(temp_file = open_temp_file(tempname, hc->hc_dest->ep_ftpnode->fn_ipc)))
	{
		recursive_getput_temp_error(&hc->hc_source->ep_ftpnode->fn_ftp, FTPERR_XFER_SRCERR, MSG_ERROR_SAVING);
		return REC_GETPUT_ERROR_START;
	}

	Close(temp_file);

	hc->hc_source->ep_ftpnode->fn_ftp.fi_flags &= ~FTP_PASSIVE;
	if (hc->hc_source->ep_ftpnode->fn_site.se_env->e_passive)
		hc->hc_source->ep_ftpnode->fn_ftp.fi_flags |= FTP_PASSIVE;

	recursive_getput_xfer_ui(hc, entry->ei_size);

	lst_addabort(hc->hc_source->ep_ftpnode, SIGBREAKF_CTRL_D, 0);

	actual = recursive_getput_file(hc->hc_source,
								   FAVOUR_GET_FILE,
								   (int (*)(void *, unsigned int, unsigned int))xfer_update,
								   &hc->hc_ui,
								   entry->ei_name,
								   tempname,
								   FALSE);

	lst_remabort(hc->hc_source->ep_ftpnode);

	if (hc->hc_source->ep_ftpnode->fn_ftp.fi_aborted && !hc->hc_source->ep_ftpnode->fn_ftp.fi_errno)
		result = REC_GETPUT_ABORTED;
	else if (hc->hc_source->ep_ftpnode->fn_ftp.fi_errno || actual != entry->ei_size)
	{
		if (hc->hc_source->ep_ftpnode->fn_ftp.fi_errno & FTPERR_XFER_DSTERR)
			recursive_getput_temp_error(&hc->hc_source->ep_ftpnode->fn_ftp, FTPERR_XFER_SRCERR, MSG_ERROR_SAVING);
		else if (!hc->hc_source->ep_ftpnode->fn_ftp.fi_errno)
			hc->hc_source->ep_ftpnode->fn_ftp.fi_errno |= FTPERR_XFER_SRCERR;

		result = actual ? REC_GETPUT_ERROR_END : REC_GETPUT_ERROR_START;
	}
	else
	{
		hc->hc_dest->ep_ftpnode->fn_ftp.fi_flags &= ~FTP_PASSIVE;
		if (hc->hc_dest->ep_ftpnode->fn_site.se_env->e_passive)
			hc->hc_dest->ep_ftpnode->fn_ftp.fi_flags |= FTP_PASSIVE;

		recursive_getput_xfer_ui(hc, entry->ei_size);

		lst_addabort(hc->hc_dest->ep_ftpnode,
					 SIGBREAKF_CTRL_D,
					 (struct Task *)hc->hc_dest->ep_ftpnode->fn_ipc->proc);

		actual = recursive_getput_file(hc->hc_dest,
									   FAVOUR_PUT_FILE,
									   (int (*)(void *, unsigned int, unsigned int))xfer_update,
									   &hc->hc_ui,
									   destname,
									   tempname,
									   resume ? hc->hc_misc_bytes : 0);

		lst_remabort(hc->hc_dest->ep_ftpnode);

		if (hc->hc_dest->ep_ftpnode->fn_ftp.fi_aborted && !hc->hc_dest->ep_ftpnode->fn_ftp.fi_errno)
			result = REC_GETPUT_ABORTED;
		else if (hc->hc_dest->ep_ftpnode->fn_ftp.fi_errno || actual != entry->ei_size)
		{
			if (hc->hc_dest->ep_ftpnode->fn_ftp.fi_errno & FTPERR_XFER_SRCERR)
				recursive_getput_temp_error(&hc->hc_dest->ep_ftpnode->fn_ftp, FTPERR_XFER_DSTERR, MSG_ERROR_LOADING);
			else if (!hc->hc_dest->ep_ftpnode->fn_ftp.fi_errno)
				hc->hc_dest->ep_ftpnode->fn_ftp.fi_errno |= FTPERR_XFER_DSTERR;

			result = REC_GETPUT_ERROR_END;
		}
		else
			result = REC_GETPUT_OK;
	}

	DeleteFile(tempname);

	return result;
}

struct copy_locals
{
	struct rec_entry_list *source_list;
	struct entry_info *source_subentry;
	int subrv;		// Accumulated worst result from recursive calls
	BOOL abort;		// As in old lister_xfer();
	int reqresult;	// Result from error requester
	int replace_result;
	char *destname;
	char myfulldirname[PATHLEN + 1];
};

static int recursive_use_cwd_first(struct hook_rec_data *hc)
{
	if (!hc)
		return 0;

	if (hc->hc_recur_flags & RECURF_BROKEN_LS)
		return 1;

	if (!hc->hc_source || hc->hc_source->ep_type != ENDPOINT_FTP || !hc->hc_source->ep_ftpnode)
		return 0;

	/*
	 * Some MLSD servers return names prefixed by the directory argument.
	 * The cwd-first path lists the current directory instead, so recursive
	 * children are handled as basenames.
	 */
	return hc->hc_source->ep_ftpnode->fn_protocol != FTP_PROTOCOL_SFTP &&
		   ftp_listcmd_is_mlsd(hc->hc_source->ep_ftpnode->fn_lscmd);
}

//
//	The faster, less accurate method
//
int do_normal_rec_copy(struct hook_rec_data *hc,
					   struct copy_locals *l,
					   struct entry_info *entry,
					   struct rec_entry_list *dest_list,
					   int retval)
{
	// Get list of entryinfos from file system or FTP server
	if ((hc->hc_recur_flags & RECURF_NORECUR) ||
		(l->source_list = hc->hc_source->ep_list(hc->hc_source, entry->ei_name)))
	{
		// Make dir on dest
		if (hc->hc_dest->ep_mkdir(hc->hc_dest, l->destname))
		{
			// Copy Protection bits? (possible on wu-ftpd servers)
			if (hc->hc_options & COPY_PROT)
			{
				if (!hc->hc_dest->ep_chmod(hc->hc_dest, l->destname, entry->ei_unixprot))
					hc->hc_dest->ep_errorreq(hc->hc_dest, hc->hc_prognode, ERRORREQ_RETRYABORT);
			}

			// Set source archive bit?
			if (hc->hc_options & COPY_ARC)
				hc->hc_source->ep_setprot(hc->hc_source, entry->ei_name, FIBF_ARCHIVE);

			// Copy FileNote? (comment) (only possible on filesystem)
			if (hc->hc_env->e_url_comment)
			{
				build_url(hc->hc_url,
						  hc->hc_source->ep_ftpnode->fn_site.se_user,
						  0,
						  hc->hc_source->ep_ftpnode->fn_site.se_host,
						  hc->hc_source->ep_ftpnode->fn_site.se_port,
						  l->myfulldirname,
						  0);

				hc->hc_dest->ep_setnote(hc->hc_dest, l->destname, hc->hc_url);
			}
		}
		else
		{
			// We can't always complain when this fails because we don't
			// care if the dir already existed.  If a file of the same
			// name existed (or some other error), we don't seem to be
			// able to detect this ):
			l->reqresult = hc->hc_dest->ep_errorreq(hc->hc_dest, hc->hc_prognode, ERRORREQ_RETRYABORT);
		}

		// Directory contains more entries?
		if (!(hc->hc_recur_flags & RECURF_NORECUR) && l->source_list->rl_entry_count)
		{
			// CD dest to new dir
			if (hc->hc_dest->ep_cwd(hc->hc_dest, l->destname))
			{
				// CD source to dir
				if (hc->hc_source->ep_cwd(hc->hc_source, entry->ei_name))
				{
					int len = strlen(l->myfulldirname);

					// Get destination entryinfos if needed for comparison
					if (hc->hc_need_dest)
						dest_list = hc->hc_dest->ep_list(hc->hc_dest, 0);
					else
						dest_list = 0;

					// Go through entries
					for (l->source_subentry = (struct entry_info *)l->source_list->rl_list.lh_Head;
						 l->source_subentry->ei_node.ln_Succ;
						 l->source_subentry = (struct entry_info *)l->source_subentry->ei_node.ln_Succ)
					{
						// Each entry fails unless proven otherwise

						// Aborted?
						if (hc->hc_source->ep_ftpnode->fn_flags & LST_ABORT)
							break;

						// Copy dir and contents
						AddPart(l->myfulldirname, entry->ei_name, PATHLEN + 1);

						l->subrv = recursive_copy(hc, entry->ei_name, l->myfulldirname, l->source_subentry, dest_list);

						l->myfulldirname[len] = 0;
					}

					// Dir had subentries that failed?
					// Or couldn't copy/move dir itself?
					if (l->subrv == 0 || l->subrv == 1)
						retval = min(1, retval);

					// Dir (and all subentries) copied/moved ok
					else
						retval = min(l->subrv, retval);

					// Free destination entry list
					if (dest_list)
						free_entry_list(dest_list);

					// CD source back up to original dir
					hc->hc_source->ep_cdup(hc->hc_source);
				}
				else
				{
					// Not allowed acces to source dir!
					hc->hc_source->ep_errorreq(hc->hc_source, hc->hc_prognode, ERRORREQ_RETRYABORT);
					retval = 0;
				}

				// CD dest back up to original dir
				hc->hc_dest->ep_cdup(hc->hc_dest);
			}
			else
			{
				// Dir wasn't made
				hc->hc_dest->ep_errorreq(hc->hc_dest, hc->hc_prognode, ERRORREQ_RETRYABORT);
				retval = 0;
			}
		}
		else if (hc->hc_recur_flags & RECURF_NORECUR)
		{
			retval = 3;
		}

		// If Move, delete dir itself from source
		if (retval == 3 && hc->hc_copy_flags & XFER_MOVE)
		{
			if (!hc->hc_source->ep_rmd(hc->hc_source, entry))
			{
				hc->hc_source->ep_errorreq(hc->hc_source, hc->hc_prognode, ERRORREQ_RETRYABORT);
				retval = 2;
			}
		}

		// Free source entry list
		if (l->source_list)
			free_entry_list(l->source_list);
	}
	else
		retval = 0;

	return retval;
}

//
//	The slower, more accurate version
//
int do_broken_rec_copy(struct hook_rec_data *hc,
					   struct copy_locals *l,
					   struct entry_info *entry,
					   struct rec_entry_list *dest_list,
					   int retval)
{
	// CD source to dir
	if (hc->hc_source->ep_cwd(hc->hc_source, entry->ei_name))
	{
		// Get list of entryinfos from file system or FTP server
		if ((hc->hc_recur_flags & RECURF_NORECUR) || (l->source_list = hc->hc_source->ep_list(hc->hc_source, 0)))
		{
			// Make dir on dest
			if (hc->hc_dest->ep_mkdir(hc->hc_dest, l->destname))
			{
				// Copy Protection bits? (possible on wu-ftpd servers)
				if (hc->hc_options & COPY_PROT)
				{
					if (!hc->hc_dest->ep_chmod(hc->hc_dest, l->destname, entry->ei_unixprot))
						hc->hc_dest->ep_errorreq(hc->hc_dest, hc->hc_prognode, ERRORREQ_RETRYABORT);
				}

				// Set source archive bit?
				if (hc->hc_options & COPY_ARC)
					hc->hc_source->ep_setprot(hc->hc_source, entry->ei_name, FIBF_ARCHIVE);

				// Copy FileNote? (comment) (only possible on filesystem)
				if (hc->hc_env->e_url_comment)
				{
					build_url(hc->hc_url,
							  hc->hc_source->ep_ftpnode->fn_site.se_user,
							  0,
							  hc->hc_source->ep_ftpnode->fn_site.se_host,
							  hc->hc_source->ep_ftpnode->fn_site.se_port,
							  l->myfulldirname,
							  0);

					hc->hc_dest->ep_setnote(hc->hc_dest, l->destname, hc->hc_url);
				}
			}
			else
			{
				// We can't always complain when this fails because we don't
				// care if the dir already existed.  If a file of the same
				// name existed (or some other error), we don't seem to be
				// able to detect this ):
				l->reqresult = hc->hc_dest->ep_errorreq(hc->hc_dest, hc->hc_prognode, ERRORREQ_RETRYABORT);
			}

			// Directory contains more entries?
			if (!(hc->hc_recur_flags & RECURF_NORECUR) && l->source_list->rl_entry_count)
			{
				// CD dest to new dir
				if (hc->hc_dest->ep_cwd(hc->hc_dest, l->destname))
				{
					int len = strlen(l->myfulldirname);

					// Get destination entryinfos if needed for comparison
					if (hc->hc_need_dest)
						dest_list = hc->hc_dest->ep_list(hc->hc_dest, 0);
					else
						dest_list = 0;

					// Go through entries
					for (l->source_subentry = (struct entry_info *)l->source_list->rl_list.lh_Head;
						 l->source_subentry->ei_node.ln_Succ;
						 l->source_subentry = (struct entry_info *)l->source_subentry->ei_node.ln_Succ)
					{
						// Each entry fails unless proven otherwise

						// Aborted?
						if (hc->hc_source->ep_ftpnode->fn_flags & LST_ABORT)
							break;

						// Copy dir and contents
						AddPart(l->myfulldirname, entry->ei_name, PATHLEN + 1);

						l->subrv = recursive_copy(hc, entry->ei_name, l->myfulldirname, l->source_subentry, dest_list);

						l->myfulldirname[len] = 0;
					}

					// Dir had subentries that failed?
					// Or couldn't copy/move dir itself?
					if (l->subrv == 0 || l->subrv == 1)
						retval = min(1, retval);

					// Dir (and all subentries) copied/moved ok
					else
						retval = min(l->subrv, retval);

					// Free destination entry list
					if (dest_list)
						free_entry_list(dest_list);

					// CD dest back up to original dir
					hc->hc_dest->ep_cdup(hc->hc_dest);
				}
				else
				{
					// Dir wasn't made
					hc->hc_dest->ep_errorreq(hc->hc_dest, hc->hc_prognode, ERRORREQ_RETRYABORT);
					retval = 0;
				}
			}

			// Free source entry list
			if (l->source_list)
				free_entry_list(l->source_list);
		}
		else if (hc->hc_recur_flags & RECURF_NORECUR)
		{
			retval = 3;
		}

		// CD source back up to original dir
		hc->hc_source->ep_cdup(hc->hc_source);

		// If Move, delete dir itself from source
		if (retval == 3 && hc->hc_copy_flags & XFER_MOVE)
		{
			if (!hc->hc_source->ep_rmd(hc->hc_source, entry))
			{
				hc->hc_source->ep_errorreq(hc->hc_source, hc->hc_prognode, ERRORREQ_RETRYABORT);
				retval = 2;
			}
		}
	}
	else
	{
		// Not allowed acces to source dir!
		hc->hc_source->ep_errorreq(hc->hc_source, hc->hc_prognode, ERRORREQ_RETRYABORT);
		retval = 0;
	}

	return retval;
}

//
//	Recursive GET, PUT, or GETPUT
//	Copies a directory plus its contents
//
//	Returns 3 if everything was moved
//	Returns 2 if everything was copied but not deleted
//	Returns 1 if some subentries weren't copied/deleted
//	Returns 0 if some entries weren't copied/deleted
//	Returns 0 for abort and other errors too
//	Directory links and unknown entries are not deleted
//
//	Needed:
//		DONE Update progress bar for each entry in name field
//		     Recursive filtering
//		     Replace/Resume/Skip/Abort requester (must handle different src/dest types!)
//		DONE Progress bar updating during xfer (bytes/total; bytes only; total only)
//		DONE Copy protection flags (check options)
//		     Set archive bit option
//		DONE Update progress bar for each current dir change in info field
//
//	Callbacks:
//		DONE precopy(dirname,entryinfo) - handles progress name/info + replace requester
//		???? copy(bytes) - handles progress bar graph + bytes per second
//		???? postcopy(entryinfo) - handles protection flags?, deleting source fore 'move'
//
int recursive_copy(struct hook_rec_data *hc,
				   char *startdir,
				   char *fulldirname,
				   struct entry_info *entry,
				   struct rec_entry_list *dest_list)
{
	struct copy_locals *l;
	int retval = 3;	 // Overall return value

	if (!(l = AllocVec(sizeof(*l), MEMF_ANY)))
		return 0;

	// Initialize locals
	l->source_list = 0;
	l->subrv = 3;
	l->abort = FALSE;
	l->reqresult = -1;	// Result from error requester
	l->replace_result = REPLACE_REPLACE;
	l->destname = 0;

	// Valid?
	if (!hc || !entry)
		goto return_0;

	if (!hc->hc_source->ep_list || !hc->hc_source->ep_cwd || !hc->hc_source->ep_cdup)
		goto return_0;

	if (!hc->hc_dest->ep_cwd || !hc->hc_dest->ep_cdup || !hc->hc_dest->ep_mkdir)
		goto return_0;

	// Aborted?
	if (hc->hc_source->ep_ftpnode->fn_flags & LST_ABORT)
		goto return_0;

	// Build our full dir name
	strcpy(l->myfulldirname, fulldirname);

	// Update progress bar, handle Replace requester for top-level entries
	if (hc->hc_pre)
		l->replace_result = hc->hc_pre(hc, startdir, dest_list, entry);

	// Skip this file?
	switch (l->replace_result)
	{
	case REPLACE_SKIP:
	case REPLACE_SKIPALL:
		goto return_0;
		break;
	}

	switch (entry->ei_type)
	{
	case -3:  // Link to File
		// Confirm link copy and fall through (enabled by default)
		D(bug("file link '%s'\n", entry->ei_name));

	case -1:  // File
		// Init update info callback structure (from old lister_xfer())
		InitSemaphore(&hc->hc_ui.ui_sem);
		hc->hc_ui.ui_og = hc->hc_og;
		hc->hc_ui.ui_opus = hc->hc_opus;
		hc->hc_ui.ui_abort = &l->abort;

		// Init update info callback structure (from old get())
		*hc->hc_ui.ui_abort = FALSE;

		// Download?
		if (hc->hc_source->ep_type == ENDPOINT_FTP && hc->hc_dest->ep_type == ENDPOINT_FILESYS)
		{
			// Which lister if any can show the progress bar?
			hc->hc_ui.ui_ftpnode = hc->hc_source->ep_ftpnode;
			hc->hc_ui.ui_handle = hc->hc_source->ep_ftpnode->fn_handle;

			retval = rec_retry_get(hc,
								   startdir,
								   l->myfulldirname,
								   entry,
								   l->replace_result == REPLACE_RESUME || l->replace_result == REPLACE_RESUMEALL);
		}

		// Upload?
		else if (hc->hc_source->ep_type == ENDPOINT_FILESYS && hc->hc_dest->ep_type == ENDPOINT_FTP)
		{
			// Which lister if any can show the progress bar?
			if (hc->hc_source->ep_ftpnode && hc->hc_source->ep_ftpnode->fn_handle)
			{
				hc->hc_ui.ui_ftpnode = hc->hc_source->ep_ftpnode;
				hc->hc_ui.ui_handle = hc->hc_source->ep_ftpnode->fn_handle;
			}
			else
			{
				hc->hc_ui.ui_ftpnode = hc->hc_dest->ep_ftpnode;
				hc->hc_ui.ui_handle = hc->hc_dest->ep_ftpnode->fn_handle;
			}

			retval = rec_retry_put(
				hc, entry, startdir, l->replace_result == REPLACE_RESUME || l->replace_result == REPLACE_RESUMEALL);
		}

		// FTP-FTP?
		else if (hc->hc_source->ep_type == ENDPOINT_FTP && hc->hc_dest->ep_type == ENDPOINT_FTP)
		{
			// Which lister if any can show the progress bar?
			hc->hc_ui.ui_ftpnode = hc->hc_source->ep_ftpnode;
			hc->hc_ui.ui_handle = hc->hc_source->ep_ftpnode->fn_handle;

			retval = rec_retry_getput(
				hc, entry, startdir, l->replace_result == REPLACE_RESUME || l->replace_result == REPLACE_RESUMEALL);
		}
		else
		{
			D(bug("** unsupported endpoint combination\n"));
			retval = 0;
		}
		break;

	case 3:	 // Link to Dir
		// Confirm link copy and fall through (disabled by default)
		break;

	case 1:	 // Dir
		// Ignore . and .. on ALL systems
		if (hc->hc_source->ep_type == ENDPOINT_FTP && (!strcmp(entry->ei_name, ".") || !strcmp(entry->ei_name, "..")))
		{
			retval = 0;
			break;
		}

		// Use new name for CopyAs/MoveAs at top level
		if (hc->hc_basedirname == startdir && hc->hc_newname)
		{
			D(bug("destname = hc->hc_newname\n"));
			l->destname = hc->hc_newname;
		}
		else
		{
			D(bug("destname = entry->ei_name\n"));
			l->destname = entry->ei_name;
		}

		if (recursive_use_cwd_first(hc))
			retval = do_broken_rec_copy(hc, l, entry, dest_list, retval);
		else
			retval = do_normal_rec_copy(hc, l, entry, dest_list, retval);

		break;

	default:  // This is not possible
		D(bug("** (??) %s\n", entry->ei_name));
		retval = 0;
		break;
	}

	D(bug("** copy = %ld\n", (long)retval));

	// Only do the following if we jumped there specifically
	goto free_locals;

return_0:
	retval = 0;

free_locals:
	FreeVec(l);

	return retval;
}

//
//	Does a similar job to the low-level get() and put() functions
//
static unsigned int recursive_getput(endpoint *source,
									 endpoint *dest,
									 int (*updatefn)(void *, unsigned int, unsigned int),
									 void *updateinfo,
									 char *remote_path,
									 char *local_path,
									 unsigned int restart)
{
	unsigned int total = 0xffffffff;
	unsigned int bytes = REC_GETPUT_ERROR_START;
	struct sockaddr_in *address;
	int srcnotdone = 1, dstnotdone = 1;
	int srcselectresult, dstselectresult;
	int aborted = 0;
	int reply;
	int keep_trying = 1;

	D(bug("recursive_getput()\n"));

	// No abort/error yet
	recursive_getput_clear_errors(source, dest);

	// Don't attempt transfer if we know neither side supports PASV
	if (source->ep_ftpnode->fn_ftp.fi_flags & dest->ep_ftpnode->fn_ftp.fi_flags & FTP_NO_PASV)
	{
		D(bug("** getput neither side supports PASV\n"));
		source->ep_ftpnode->fn_ftp.fi_errno |= FTPERR_XFER_SRCERR;
		stccpy(source->ep_ftpnode->fn_ftp.fi_serverr, GetString(locale, MSG_FTP_GETPUT_NO_PASV), IOBUFSIZE + 1);
		keep_trying = 0;
	}

	// Server-to-server transfers do not let us negotiate or protect the data channel
	if (keep_trying && !ftp_tls_modes_allow_server_transfer(source->ep_ftpnode->fn_ftp.fi_tls_mode,
														   dest->ep_ftpnode->fn_ftp.fi_tls_mode))
	{
		D(bug("** getput FTPS data channel unsupported\n"));
		source->ep_ftpnode->fn_ftp.fi_errno |= FTPERR_XFER_SRCERR;
		stccpy(source->ep_ftpnode->fn_ftp.fi_serverr, GetString(locale, MSG_FTP_GETPUT_NO_FTPS), IOBUFSIZE + 1);
		keep_trying = 0;
	}

	// Don't attempt to resume if we know one or both sides doesn't support REST
	if (restart && !(source->ep_ftpnode->fn_ftp.fi_flags & dest->ep_ftpnode->fn_ftp.fi_flags & FTP_NO_REST))
	{
		D(bug("** getput no restart on one or both ends\n"));
		restart = 0;
	}

	while (keep_trying)
	{
		keep_trying = 0;

		bytes = REC_GETPUT_ERROR_START;

		if ((address = dest->ep_pasv(dest)))
		{
			if (source->ep_port(source, address, 0))
			{
				// Check for same filename
				// Send REST if resuming

				//	source->ep_rest( source, restart );

				if (source->ep_retr(source, remote_path))
				{
					char *p;

					// Does the RETR reply contain the file length?
					if ((p = strstr(source->ep_ftpnode->fn_ftp.fi_iobuf, " bytes)")))
					{
						if (isdigit(*--p))
						{
							while (isdigit(*p--))
								;
							total = atoi(p + 2);
						}
					}

					//	dest->ep_rest( dest, restart );
					if (dest->ep_stor(dest, local_path))
					{
						// Callback with 0% (or restart offset?)
						// First update tells callback where we're starting from
						if (updatefn)
							(*updatefn)(updateinfo, total, 0);

						// Callback every so often until done
						// And abort if abort button pressed

						// We need to call SelectWait in a loop with a nice timeout value
						while (srcnotdone || dstnotdone)
						{
							srcselectresult = dest->ep_select(dest);
							dstselectresult = source->ep_select(source);

							if (srcselectresult != 0)
								srcnotdone = 0;
							if (dstselectresult != 0)
								dstnotdone = 0;

							if (updatefn)
								(*updatefn)(updateinfo, total, 1);

							// Abort detected by either end?
							// Don't abort if either side has completed though
							if ((srcselectresult != 1 && dstselectresult != 1 && srcselectresult == -1) ||
								dstselectresult == -1)
							{
								D(bug("*** getput() CTRL-D SIGNAL ***\n"));
								source->ep_ftpnode->fn_ftp.fi_aborted = 1;

								// Send ABOR
								source->ep_abor(source);

								// Get reply to ABOR (426)
								// Reply to RETR might be arriving first though (225)
								reply = source->ep_getreply(source, 0);

								if (reply == 225)
								{
									D(bug("** ABOR sent but RETR already completed\n"));
								}
								else
								{
									aborted = 1;
								}
							}
						}

						// Get 2nd reply to RETR (226)
						// Reply to ABOR might be here if it was sent late (225)
						reply = source->ep_getreply(source, 0);

						if (reply != 225 && reply != 226)
						{
							source->ep_ftpnode->fn_ftp.fi_errno |= FTPERR_XFER_SRCERR;
							stccpy(source->ep_ftpnode->fn_ftp.fi_serverr,
								   source->ep_ftpnode->fn_ftp.fi_iobuf,
								   IOBUFSIZE + 1);
							bytes = REC_GETPUT_ERROR_END;
						}

						// Get 2nd reply to STOR
						if (dest->ep_getreply(dest, 0) != 226)
						{
							dest->ep_ftpnode->fn_ftp.fi_errno |= FTPERR_XFER_DSTERR;
							stccpy(
								dest->ep_ftpnode->fn_ftp.fi_serverr, dest->ep_ftpnode->fn_ftp.fi_iobuf, IOBUFSIZE + 1);
							bytes = REC_GETPUT_ERROR_END;
						}

						if (bytes != REC_GETPUT_ERROR_END)
						{
							if (aborted)
								bytes = REC_GETPUT_ABORTED;
							else
								bytes = REC_GETPUT_OK;
						}

						// Callback with 100% (unless aborted)
						if (updatefn && !aborted)
							(*updatefn)(updateinfo, total, 0);
					}
					else
					{
						struct sockaddr_in fake_addr = {0};

						dest->ep_ftpnode->fn_ftp.fi_errno |= FTPERR_XFER_DSTERR;
						stccpy(dest->ep_ftpnode->fn_ftp.fi_serverr, dest->ep_ftpnode->fn_ftp.fi_iobuf, IOBUFSIZE + 1);

						// Attempt to break deadlock with PORT 0,0,0,0,0,0
						dest->ep_port(dest, &fake_addr, PORT_QUIET);
						dest->ep_getreply(dest, GETREPLY_QUIET);

						// Get response to broken connection
						// (May already be complete)
						source->ep_getreply(source, 0);

						// Hopefully, machines that don't like the PORT
						// trick to break the data connection don't
						// need it anyway (:
					}
				}
				else
				{
					source->ep_ftpnode->fn_ftp.fi_errno |= FTPERR_XFER_SRCERR;
					stccpy(source->ep_ftpnode->fn_ftp.fi_serverr, source->ep_ftpnode->fn_ftp.fi_iobuf, IOBUFSIZE + 1);
				}
			}
			else
			{
				D(bug("** PORT fail\n"));
				source->ep_ftpnode->fn_ftp.fi_errno |= FTPERR_XFER_SRCERR;
				stccpy(source->ep_ftpnode->fn_ftp.fi_serverr, source->ep_ftpnode->fn_ftp.fi_iobuf, IOBUFSIZE + 1);
			}

			FreeVec(address);
		}
		else
		{
			D(bug("** PASV fail\n"));
			dest->ep_ftpnode->fn_ftp.fi_errno |= FTPERR_XFER_DSTERR;
			stccpy(dest->ep_ftpnode->fn_ftp.fi_serverr, dest->ep_ftpnode->fn_ftp.fi_iobuf, IOBUFSIZE + 1);
		}
	}

	return bytes;
}

/**
 }{	Recursive delete functions
 **/

struct delete_locals
{
	struct rec_entry_list *entry_list;
	struct entry_info *subentry;
	int subrv;	// Result from recursive call
};

int do_normal_rec_delete(struct hook_rec_data *hc, struct delete_locals *l, struct entry_info *entry, int retval)
{
	D(bug("normal delete\n"));

	// Get list of entry_infos from FTP server
	if ((l->entry_list = hc->hc_source->ep_list(hc->hc_source, entry->ei_name)))
	{
		// Directory contains more entries?
		if (l->entry_list->rl_entry_count)
		{
			// CD source to dir
			if (hc->hc_source->ep_cwd(hc->hc_source, entry->ei_name))
			{
				// Go through entries
				for (l->subentry = (struct entry_info *)l->entry_list->rl_list.lh_Head; l->subentry->ei_node.ln_Succ;
					 l->subentry = (struct entry_info *)l->subentry->ei_node.ln_Succ)
				{
					// Aborted?
					if (hc->hc_source->ep_ftpnode->fn_flags & LST_ABORT)
						break;

					// Delete dir and contents
					l->subrv = recursive_delete(hc, entry->ei_name, l->subentry);

					// Couldn't delete dir itself?
					if (l->subrv == 0)
						retval = min(1, retval);

					// Dir had subentries that failed?
					else if (l->subrv == 1)
						retval = min(1, retval);

					// Dir (and all subentries) deleted ok
					else
						retval = min(2, retval);
				}
				// CD source back up to original dir
				hc->hc_source->ep_cdup(hc->hc_source);
			}
		}

		// Delete directory itself (if we've been successful so far)
		if (retval == 2 && !(hc->hc_source->ep_ftpnode->fn_flags & LST_ABORT))
		{
			if (!rec_retry_rmd(hc->hc_source, hc->hc_prognode, entry))
				retval = 0;
		}

		// Free entry list
		free_entry_list(l->entry_list);
	}

	return retval;
}

int do_broken_rec_delete(struct hook_rec_data *hc, struct delete_locals *l, struct entry_info *entry, int retval)
{
	D(bug("broken delete\n"));

	// CD source to dir
	if (hc->hc_source->ep_cwd(hc->hc_source, entry->ei_name))
	{
		// Get list of entry_infos from FTP server
		if ((l->entry_list = hc->hc_source->ep_list(hc->hc_source, 0)))
		{
			// Directory contains more entries?
			if (l->entry_list->rl_entry_count)
			{
				// Go through entries
				for (l->subentry = (struct entry_info *)l->entry_list->rl_list.lh_Head; l->subentry->ei_node.ln_Succ;
					 l->subentry = (struct entry_info *)l->subentry->ei_node.ln_Succ)
				{
					// Aborted?
					if (hc->hc_source->ep_ftpnode->fn_flags & LST_ABORT)
						break;

					// Delete dir and contents
					l->subrv = recursive_delete(hc, entry->ei_name, l->subentry);

					// Couldn't delete dir itself?
					if (l->subrv == 0)
						retval = min(1, retval);

					// Dir had subentries that failed?
					else if (l->subrv == 1)
						retval = min(1, retval);

					// Dir (and all subentries) deleted ok
					else
						retval = min(2, retval);
				}
			}
			// Free entry list
			free_entry_list(l->entry_list);
		}

		// CD source back up to original dir
		hc->hc_source->ep_cdup(hc->hc_source);

		// Delete directory itself (if we've been successful so far)
		if (retval == 2 && !(hc->hc_source->ep_ftpnode->fn_flags & LST_ABORT))
		{
			if (!rec_retry_rmd(hc->hc_source, hc->hc_prognode, entry))
				retval = 0;
		}
	}

	return retval;
}

//
//	Recursive delete
//
//	Returns 2 if everything was deleted
//	Returns 1 if some sub-entries weren't deleted (if rec. call was 0)
//	Returns 0 if some entries weren't deleted
//	Returns 0 for abort and other errors too
//	Directory links and unknown entries are not deleted
//
//	Needed:
//		Update progress bar for each entry
//		Confirm delete requester (checks options, different for files/dirs)
//		Update progress bar for each current dir change
//
int recursive_delete(struct hook_rec_data *hc, char *startdir, struct entry_info *entry)
{
	struct delete_locals *l;
	int retval = 2;	 // Result we will return

	D(bug("recursive_delete()\n"));

	if (!(l = AllocVec(sizeof(*l), MEMF_ANY)))
		return 0;

	// Valid source?
	if (!hc || !entry)
		goto return_0;
	if (!hc->hc_source->ep_list || !hc->hc_source->ep_cwd || !hc->hc_source->ep_cdup || !hc->hc_source->ep_dele ||
		!hc->hc_source->ep_rmd)
		goto return_0;

	// Aborted?
	if (hc->hc_source->ep_ftpnode->fn_flags & LST_ABORT)
		goto return_0;

	// Update progress bar name
	if (hc->hc_pre)
		hc->hc_pre(hc, startdir, 0, entry);

	// Confirm delete

	switch (entry->ei_type)
	{
	case -1:  // File
		if (!rec_retry_dele(hc->hc_source, hc->hc_prognode, entry))
			retval = 0;
		break;

	case -3:  // Link to File
		if (!rec_retry_dele(hc->hc_source, hc->hc_prognode, entry))
			retval = 0;
		break;

	case 1:	 // Dir
		// Ignore . and .. on ALL systems
		if (hc->hc_source->ep_type == ENDPOINT_FTP && (!strcmp(entry->ei_name, ".") || !strcmp(entry->ei_name, "..")))
		{
			retval = 0;
			break;
		}

		if (recursive_use_cwd_first(hc))
			retval = do_broken_rec_delete(hc, l, entry, retval);
		else
			retval = do_normal_rec_delete(hc, l, entry, retval);

		break;

	case 3:	 // Link to Dir (don't delete)
		// TODO Tell user we don't delete dir links
		retval = 0;
		break;

	default:  // Shouldn't happen (so don't try to delete)
		D(bug("** recursive_delete()\n   unexpected type %ld '%s'\n", (long)entry->ei_type, entry->ei_name));
		retval = 0;
		break;
	}

	goto free_locals;

return_0:
	retval = 0;

free_locals:
	FreeVec(l);

	return retval;
}

/**
 }{	Recursive protect functions
 **/

struct protect_locals
{
	struct rec_entry_list *entry_list;
	struct entry_info *subentry;
	int subrv;	// Result from recursive call
};

int do_normal_rec_protect(struct hook_rec_data *hc,
						  struct protect_locals *l,
						  struct entry_info *entry,
						  LONG set,
						  LONG clr,
						  int retval)
{
	D(bug("normal protect\n"));

	if ((l->entry_list = hc->hc_source->ep_list(hc->hc_source, entry->ei_name)))
	{
		// Directory contains more entries?
		if (l->entry_list->rl_entry_count)
		{
			// CD source to dir
			if (hc->hc_source->ep_cwd(hc->hc_source, entry->ei_name))
			{
				// Go through entries
				for (l->subentry = (struct entry_info *)l->entry_list->rl_list.lh_Head; l->subentry->ei_node.ln_Succ;
					 l->subentry = (struct entry_info *)l->subentry->ei_node.ln_Succ)
				{
					// Aborted?
					if (hc->hc_source->ep_ftpnode->fn_flags & LST_ABORT)
						break;

					// Protect dir and contents
					l->subrv = recursive_protect(hc, l->subentry, set, clr);

					// Couldn't protect dir itself?
					if (l->subrv == 0)
						retval = min(1, retval);

					// Dir had subentries that failed?
					else if (l->subrv == 1)
						retval = min(1, retval);

					// Dir (and all subentries) deleted ok
					else
						retval = min(2, retval);
				}
				// CD source back up to original dir
				hc->hc_source->ep_cdup(hc->hc_source);
			}
		}

		// Free entry list
		free_entry_list(l->entry_list);
	}

	return retval;
}

int do_broken_rec_protect(struct hook_rec_data *hc,
						  struct protect_locals *l,
						  struct entry_info *entry,
						  LONG set,
						  LONG clr,
						  int retval)
{
	D(bug("broken protect\n"));

	// CD source to dir
	if (hc->hc_source->ep_cwd(hc->hc_source, entry->ei_name))
	{
		if ((l->entry_list = hc->hc_source->ep_list(hc->hc_source, 0)))
		{
			// Directory contains more entries?
			if (l->entry_list->rl_entry_count)
			{
				// Go through entries
				for (l->subentry = (struct entry_info *)l->entry_list->rl_list.lh_Head; l->subentry->ei_node.ln_Succ;
					 l->subentry = (struct entry_info *)l->subentry->ei_node.ln_Succ)
				{
					// Aborted?
					if (hc->hc_source->ep_ftpnode->fn_flags & LST_ABORT)
						break;

					// Protect dir and contents
					l->subrv = recursive_protect(hc, l->subentry, set, clr);

					// Couldn't protect dir itself?
					if (l->subrv == 0)
						retval = min(1, retval);

					// Dir had subentries that failed?
					else if (l->subrv == 1)
						retval = min(1, retval);

					// Dir (and all subentries) deleted ok
					else
						retval = min(2, retval);
				}
			}
			// Free entry list
			free_entry_list(l->entry_list);
		}
		// CD source back up to original dir
		hc->hc_source->ep_cdup(hc->hc_source);
	}

	return retval;
}

//
//	Recursive protect
//
//	Returns 2 if everything was protected
//	Returns 1 if some sub-entries weren't protected (if rec. call was 0)
//	Returns 0 if some entries weren't protected
//	Returns 0 for abort and other errors too
//	Directory links and unknown entries are not recursed into
//
int recursive_protect(struct hook_rec_data *hc, struct entry_info *entry, LONG set, LONG clr)
{
	struct protect_locals *l;
	int retval = 2;	 // Result we will return

	if (!(l = AllocVec(sizeof(*l), MEMF_ANY)))
		return 0;

	// Valid source?
	if (!hc || !entry)
		goto return_0;
	if (!hc->hc_source->ep_list || !hc->hc_source->ep_cwd || !hc->hc_source->ep_cdup || !hc->hc_source->ep_chmod)
		goto return_0;

	// Aborted?
	if (hc->hc_source->ep_ftpnode->fn_flags & LST_ABORT)
		goto return_0;

	// Update progress bar name, show requester
	if (hc->hc_pre)
		hc->hc_pre(hc, 0, 0, entry);

	switch (entry->ei_type)
	{
	case -1:  // File
	case -3:  // Link to File
		entry->ei_unixprot &= ~clr;
		entry->ei_unixprot |= set;
		entry->ei_unixprot &= 0777;

		if (hc->hc_source->ep_chmod(hc->hc_source, entry->ei_name, entry->ei_unixprot))
			entry->ei_prot = prot_unix_to_amiga(entry->ei_unixprot);
		else
			retval = 0;
		break;

	case 1:	 // Dir
		// Ignore . and .. on ALL systems
		if (hc->hc_source->ep_type == ENDPOINT_FTP && (!strcmp(entry->ei_name, ".") || !strcmp(entry->ei_name, "..")))
		{
			retval = 0;
			break;
		}

		// If recursive, get list of entry_infos from FTP server
		if (!(hc->hc_recur_flags & RECURF_NORECUR))
		{
			if (recursive_use_cwd_first(hc))
				retval = do_broken_rec_protect(hc, l, entry, set, clr, retval);
			else
				retval = do_normal_rec_protect(hc, l, entry, set, clr, retval);
		}
		else if (hc->hc_recur_flags & RECURF_NORECUR)
			retval = 2;
		else
			retval = 0;

		// Protect directory itself (unless we found CHMOD to be unsupported)
		if (retval == 2)
		{
			entry->ei_unixprot &= ~clr;
			entry->ei_unixprot |= set;
			entry->ei_unixprot &= 0777;

			if (hc->hc_source->ep_chmod(hc->hc_source, entry->ei_name, entry->ei_unixprot))
				entry->ei_prot = prot_unix_to_amiga(entry->ei_unixprot);
			else
			{
				hc->hc_source->ep_errorreq(hc->hc_source, hc->hc_prognode, ERRORREQ_RETRYABORT);
				retval = 0;
			}
		}
		break;

	case 3:	 // Link to Dir (Protect but don't recur)
		// TODO Tell user we don't recurse down dir links
		retval = 0;
		break;

	default:  // Shouldn't happen (Try to protect anyway)
		D(bug("** (??) %s\n", entry->ei_name));
		retval = 0;
		break;
	}

	goto free_locals;

return_0:
	retval = 0;

free_locals:
	FreeVec(l);

	return retval;
}

/**
 }{	Recursive getsizes functions
 **/

struct getsizes_locals
{
	struct rec_entry_list *entry_list;
	struct entry_info *subentry;
	int subrv;	// Result from recursive call
};

int do_normal_rec_getsizes(struct hook_rec_data *hc, struct getsizes_locals *l, struct entry_info *entry, int retval)
{
	// We only need do this when recursion is specified by the user
	// Get list of entry_infos from FTP server
	if ((l->entry_list = hc->hc_source->ep_list(hc->hc_source, entry->ei_name)))
	{
		// Directory contains more entries?
		if (l->entry_list->rl_entry_count)
		{
			// CD source to dir
			if (hc->hc_source->ep_cwd(hc->hc_source, entry->ei_name))
			{
				// Go through entries
				for (l->subentry = (struct entry_info *)l->entry_list->rl_list.lh_Head; l->subentry->ei_node.ln_Succ;
					 l->subentry = (struct entry_info *)l->subentry->ei_node.ln_Succ)
				{
					// Aborted?
					if (hc->hc_source->ep_ftpnode->fn_flags & LST_ABORT)
						break;

					// Get sizes of dir and contents
					l->subrv = recursive_getsizes(hc, l->subentry);

					// Couldn't get size of dir itself?
					if (l->subrv == 0)
						retval = min(1, retval);

					// Dir had subentries that failed?
					else if (l->subrv == 1)
						retval = min(1, retval);

					// Got dir's (and all subentries') sizes ok
					else
						retval = min(2, retval);
				}
				// CD source back up to original dir
				hc->hc_source->ep_cdup(hc->hc_source);
			}
		}

		// Free entry list
		free_entry_list(l->entry_list);
	}

	return retval;
}

int do_broken_rec_getsizes(struct hook_rec_data *hc, struct getsizes_locals *l, struct entry_info *entry, int retval)
{
	D(bug("broken getsizes\n"));

	// We only need do this when recursion is specified by the user
	// CD source to dir
	if (hc->hc_source->ep_cwd(hc->hc_source, entry->ei_name))
	{
		// Get list of entry_infos from FTP server
		if ((l->entry_list = hc->hc_source->ep_list(hc->hc_source, 0)))
		{
			// Directory contains more entries?
			if (l->entry_list->rl_entry_count)
			{
				// Go through entries
				for (l->subentry = (struct entry_info *)l->entry_list->rl_list.lh_Head; l->subentry->ei_node.ln_Succ;
					 l->subentry = (struct entry_info *)l->subentry->ei_node.ln_Succ)
				{
					// Aborted?
					if (hc->hc_source->ep_ftpnode->fn_flags & LST_ABORT)
						break;

					// Get sizes of dir and contents
					l->subrv = recursive_getsizes(hc, l->subentry);

					// Couldn't get size of dir itself?
					if (l->subrv == 0)
						retval = min(1, retval);

					// Dir had subentries that failed?
					else if (l->subrv == 1)
						retval = min(1, retval);

					// Got dir's (and all subentries') sizes ok
					else
						retval = min(2, retval);
				}
			}
			// Free entry list
			free_entry_list(l->entry_list);
		}

		// CD source back up to original dir
		hc->hc_source->ep_cdup(hc->hc_source);
	}

	return retval;
}

//
//	Recursive getsizes
//
//	Returns 2 if everything was sized
//	Returns 1 if some sub-entries weren't sized (if rec. call was 0)
//	Returns 0 if some entries weren't sized
//	Returns 0 for abort and other errors too
//	Directory links are not recursed into
//
//	Needed:
//		Update progress bar for each entry
//		Confirm delete requester (checks options, different for files/dirs)
//
int recursive_getsizes(struct hook_rec_data *hc, struct entry_info *entry)
{
	struct getsizes_locals *l;
	int retval = 2;	 // Result we will return

	if (!(l = AllocVec(sizeof(*l), MEMF_ANY)))
		return 0;

	// Valid source?
	if (!hc || !entry)
		goto return_0;
	if (!hc->hc_source->ep_list || !hc->hc_source->ep_cwd || !hc->hc_source->ep_cdup)
		goto return_0;

	// Aborted?
	if (hc->hc_source->ep_ftpnode->fn_flags & LST_ABORT)
		goto return_0;

	// Update progress bar name, show requester
	if (hc->hc_pre)
		hc->hc_pre(hc, 0, 0, entry);

	switch (entry->ei_type)
	{
	case -3:  // Link to File - fall through to file

	case -1:  // File - add length to total
		hc->hc_misc_bytes += entry->ei_size;
		break;

	case 1:	 // Dir
		// Ignore . and .. on ALL systems
		if (hc->hc_source->ep_type == ENDPOINT_FTP && (!strcmp(entry->ei_name, ".") || !strcmp(entry->ei_name, "..")))
		{
			break;
		}

		if (recursive_use_cwd_first(hc))
			retval = do_broken_rec_getsizes(hc, l, entry, retval);
		else
			retval = do_normal_rec_getsizes(hc, l, entry, retval);

		break;

	case 3:	 // Link to Dir
		// TODO Tell user we don't recurse down dir links
		retval = 0;
		break;

	default:  // Shouldn't happen (and shouldn't matter)
		D(bug("** (??) %s\n", entry->ei_name));
		hc->hc_misc_bytes += entry->ei_size;
		// retval = 0;
		break;
	}

	goto free_locals;

return_0:
	retval = 0;

free_locals:
	FreeVec(l);

	return retval;
}

/**
 }{	Recursive findfile functions
 **/

int rec_findfile_newlister(struct ftp_node *ftpnode, const char *path, const char *pattern)
{
	struct connect_msg *cm;
	char *patterncopy;
	struct ftp_node *newnode;
	int ok = 0;

	if ((cm = get_blank_connectmsg(ftpnode->fn_og)))
	{
		cm->cm_handle = 0;

		stccpy(cm->cm_opus, ftpnode->fn_og->og_opusname, PORTNAMELEN + 1);

		copy_site_entry(ftpnode->fn_og, &cm->cm_site, &ftpnode->fn_site);
		stccpy(cm->cm_site.se_path, path, PATHLEN + 1);

		if (IPC_Command(ftpnode->fn_og->og_main_ipc, IPC_CONNECT, 0, cm, 0, REPLY_NO_PORT))
		{
			if ((patterncopy = AllocVec(strlen(pattern) + 1, MEMF_ANY)))
			{
				if ((newnode = find_ftpnode(ftpnode->fn_og, cm->cm_handle)))
				{
					strcpy(patterncopy, pattern);
					IPC_Command(newnode->fn_ipc, IPC_SELECTPATTERN, 0, 0, patterncopy, 0);
					ok = 1;
				}
			}
		}

		FreeVec(cm);
	}

	return ok;
}

struct findfile_locals
{
	struct rec_entry_list *entry_list;
	struct entry_info *subentry;
	int subrv;	// Result from recursive call
	int reqresult;
	char *field;  // Match name or comment field?
	char myfulldirname[PATHLEN + 1];
};

int do_normal_rec_findfile(struct hook_rec_data *hc, struct findfile_locals *l, struct entry_info *entry, int retval)
{
	// We only need do this when recursion is specified by the user
	// Get list of entry_infos from FTP server
	if ((l->entry_list = hc->hc_source->ep_list(hc->hc_source, entry->ei_name)))
	{
		// Directory contains more entries?
		if (l->entry_list->rl_entry_count)
		{
			// CD source to dir
			if (hc->hc_source->ep_cwd(hc->hc_source, entry->ei_name))
			{
				int len = strlen(l->myfulldirname);

				// Go through entries
				for (l->subentry = (struct entry_info *)l->entry_list->rl_list.lh_Head; l->subentry->ei_node.ln_Succ;
					 l->subentry = (struct entry_info *)l->subentry->ei_node.ln_Succ)
				{
					// Aborted?
					if (hc->hc_source->ep_ftpnode->fn_flags & LST_ABORT)
						break;

					// Copy dir and contents
					AddPart(l->myfulldirname, entry->ei_name, PATHLEN + 1);

					// Go through subentries
					l->subrv = recursive_findfile(hc, l->myfulldirname, l->subentry);

					l->myfulldirname[len] = 0;

					// Couldn't get size of dir itself?
					if (l->subrv == 0)
						retval = min(1, retval);

					// Dir had subentries that failed?
					else if (l->subrv == 1)
						retval = min(1, retval);

					// Got dir's (and all subentries') sizes ok
					else
						// retval = min(2,retval);
						retval = l->subrv;

					D(bug("** subrv %ld retval %ld\n", (long)l->subrv, (long)retval));
				}
				// CD source back up to original dir
				hc->hc_source->ep_cdup(hc->hc_source);
			}
		}

		// Free entry list
		free_entry_list(l->entry_list);
	}

	return retval;
}

int do_broken_rec_findfile(struct hook_rec_data *hc, struct findfile_locals *l, struct entry_info *entry, int retval)
{
	D(bug("broken findfile\n"));

	// CD source to dir
	if (hc->hc_source->ep_cwd(hc->hc_source, entry->ei_name))
	{
		// Get list of entry_infos from FTP server
		if ((l->entry_list = hc->hc_source->ep_list(hc->hc_source, 0)))
		{
			// Directory contains more entries?
			if (l->entry_list->rl_entry_count)
			{
				int len = strlen(l->myfulldirname);

				// Go through entries
				for (l->subentry = (struct entry_info *)l->entry_list->rl_list.lh_Head; l->subentry->ei_node.ln_Succ;
					 l->subentry = (struct entry_info *)l->subentry->ei_node.ln_Succ)
				{
					// Aborted?
					if (hc->hc_source->ep_ftpnode->fn_flags & LST_ABORT)
						break;

					// Copy dir and contents
					AddPart(l->myfulldirname, entry->ei_name, PATHLEN + 1);

					// Go through subentries
					l->subrv = recursive_findfile(hc, l->myfulldirname, l->subentry);

					l->myfulldirname[len] = 0;

					// Couldn't get size of dir itself?
					if (l->subrv == 0)
						retval = min(1, retval);

					// Dir had subentries that failed?
					else if (l->subrv == 1)
						retval = min(1, retval);

					// Got dir's (and all subentries') sizes ok
					else
						// retval = min(2,retval);
						retval = l->subrv;

					D(bug("** subrv %ld retval %ld\n", (long)l->subrv, (long)retval));
				}
			}
			// Free entry list
			free_entry_list(l->entry_list);
		}

		// CD source back up to original dir
		hc->hc_source->ep_cdup(hc->hc_source);
	}

	return retval;
}

//
//	Recursive findfile
//
//	Returns 4 if result was 'New Lister' button
//	Returns 3 if result was 'Yes' button
//	Returns 2 if everything was scanned but no matches found
//	Returns 1 if some sub-entries weren't scanneded (if rec. call was 0)
//	Returns 0 if some entries weren't scanned
//	Returns 0 for abort and other errors too
//	Directory links are not recursed into
//
int recursive_findfile(struct hook_rec_data *hc, char *fulldirname, struct entry_info *entry)
{
	struct findfile_locals *l;
	int retval = 2;	 // Result we will return

	D(bug("full %s\n", fulldirname));

	if (!(l = AllocVec(sizeof(*l), MEMF_ANY)))
		return 0;

	// Valid source?
	if (!hc || !entry)
		goto return_0;
	if (!hc->hc_source->ep_list || !hc->hc_source->ep_cwd || !hc->hc_source->ep_cdup)
		goto return_0;

	// Aborted?
	if (hc->hc_source->ep_ftpnode->fn_flags & LST_ABORT)
		goto return_0;

	// Build our full dir name
	strcpy(l->myfulldirname, fulldirname);

	// Update progress bar name, show requester
	if (hc->hc_pre)
		hc->hc_pre(hc, 0, 0, entry);

	switch (entry->ei_type)
	{
	case -3:  // Link to File - fall through to file

	case -1:  // File - add length to total
		hc->hc_misc_bytes += entry->ei_size;

		if (hc->hc_match_comment == TRUE)
			l->field = entry->ei_comment;
		else
			l->field = entry->ei_name;

		if (casematchpattern(hc->hc_ored_ftp_flags, /* flags to show case sensitivity */
							 hc->hc_pattern,
							 l->field))
		{
			l->reqresult = lister_request_tags(hc->hc_source->ep_ftpnode,
											   FR_FormatString,
											   "%s '%s' %s\n'%s'\n%s",
											   FR_MsgNum,
											   MSG_FTP_FOUND_FILE,
											   AR_Message,
											   entry->ei_name,
											   FR_MsgNum,
											   MSG_FTP_IN_THE_DIR,
											   AR_Message,
											   l->myfulldirname,
											   FR_MsgNum,
											   MSG_FTP_SHALL_I_GO,
											   FR_ButtonNum,
											   MSG_FTP_YES,
											   FR_ButtonNum,
											   MSG_FTP_NEW_LISTER,
											   FR_ButtonNum,
											   MSG_FTP_NO,
											   FR_ButtonNum,
											   MSG_ABORT,
											   TAG_DONE);

			// 'Yes' and 'Abort' cause searching to stop
			if (l->reqresult < 2)
			{
				hc->hc_source->ep_ftpnode->fn_flags |= LST_ABORT;

				if (l->reqresult == 0)
					retval = 0;
				else if (l->reqresult == 1)
				{
					stccpy(hc->hc_resultpath, l->myfulldirname, PATHLEN + 1);
					retval = 3;
				}
			}
			else if (l->reqresult == 2)
			{
				rec_findfile_newlister(hc->hc_source->ep_ftpnode, l->myfulldirname, hc->hc_pattern);
				retval = 4;
			}
		}
		break;

	case 1:	 // Dir
		// Ignore . and .. on ALL systems
		if (hc->hc_source->ep_type == ENDPOINT_FTP && (!strcmp(entry->ei_name, ".") || !strcmp(entry->ei_name, "..")))
		{
			break;
		}

		if (recursive_use_cwd_first(hc))
			retval = do_broken_rec_findfile(hc, l, entry, retval);
		else
			retval = do_normal_rec_findfile(hc, l, entry, retval);

		break;

	case 3:	 // Link to Dir
		// TODO Tell user we don't recurse down dir links
		retval = 0;
		break;

	default:  // Shouldn't happen (and shouldn't matter)
		D(bug("** (??) %s\n", entry->ei_name));
		hc->hc_misc_bytes += entry->ei_size;
		// retval = 0;
		break;
	}

	goto free_locals;

return_0:
	retval = 0;

free_locals:
	FreeVec(l);

	return retval;
}

/**
 }{	Support functions for recursive stuff
 **/

static void rec_filesys_set_ioerr(endpoint *ep, LONG ioerr)
{
	if (ep && ep->ep_ftpnode)
		ep->ep_ftpnode->fn_ftp.fi_ioerr = ioerr;
}

//
//	Get a list of entries from a file system
//
struct rec_entry_list *rec_filesys_list(endpoint *ep, char *dirname)
{
	int bad = FALSE;
	struct rec_entry_list *rel;
	BPTR lock;
	D_S(struct FileInfoBlock, fib)
	struct entry_info *entry;

	// D(bug( "FS  LIST %s\n", dir->ei_name ));

	// Valid?
	if (!*ep->ep_ftpnode->fn_site.se_path)
		return 0;

	// Allocate entry list
	if (!(rel = AllocVec(sizeof(struct rec_entry_list), MEMF_CLEAR)))
		return 0;

	// Initialize list
	NewList(&rel->rl_list);

	// Lock directory
	if ((lock = Lock((char *)dirname ? dirname : "", ACCESS_READ)))
	{
		// Examine directory
		if (Examine(lock, fib))
		{
			// Loop
			while (!bad && ExNext(lock, fib))
			{
				// Make a new entry
				if (!(entry = AllocVec(sizeof(struct entry_info), MEMF_CLEAR)))
					bad = 1;
				else
				{
					// Fill it out
					fileinfoblock_to_entryinfo(entry, fib);

					// Add to list
					AddTail(&rel->rl_list, &entry->ei_node);
					++rel->rl_entry_count;
				}
			}
			// Normal end of scan?
			if (IoErr() != ERROR_NO_MORE_ENTRIES)
				D(bug("** scan aborted abnormally (%ld)\n", IoErr()));
		}

		// Unlock directory
		UnLock(lock);
	}

	// Delete the list if we have to
	if (bad)
	{
		free_entry_list(rel);
		rel = 0;
	}

	return rel;
}

//
//	Update structure for recursive list callback
//
struct rec_updateinfo
{
	struct rec_entry_list *ui_list;	 // List to add entry to
	int (*ui_ls_to_entryinfo)(struct entry_info *, const char *line, ULONG flags);
	ULONG ui_flags;	 // Flags relevant to FTP server
	struct ftp_node *ui_ftpnode;	 // FTP node for UTF-8 conversion
};

//
//	FTP LIST callback function
//	  1 success
//	-10 out of memory
//
static int callback_func(struct rec_updateinfo *ui, char *line)
{
	struct entry_info *entry;
	int retval = 1;	 // <= 0 is an error

	// Make a new entry
	if (!(entry = AllocVec(sizeof(struct entry_info), MEMF_CLEAR)))
		return -10;
	else
	{
		// Can we fill it out?
		if (ui->ui_ls_to_entryinfo(entry, line, ui->ui_flags))
		{
			// Convert UTF-8 filename to local charset if server supports UTF-8.
			// Mirrors sftp_callback_func so recursive plain-FTP operations see
			// the same local-charset ei_name as the interactive lister.
			if (ui->ui_ftpnode)
				ftp_convert_filename_from_utf8(entry, &ui->ui_ftpnode->fn_ftp);

			// Add to list
			AddTail(&ui->ui_list->rl_list, &entry->ei_node);
			++ui->ui_list->rl_entry_count;
		}

		// No - delete it
		else
			FreeVec(entry);
	}

	return retval;
}

static int sftp_callback_func(void *userdata, const struct ftp_sftp_entry *sftp_entry)
{
	struct rec_updateinfo *ui = userdata;
	struct entry_info *entry;

	if (!ui || !sftp_entry)
		return 1;

	if (!(entry = AllocVec(sizeof(struct entry_info), MEMF_CLEAR)))
		return -10;

	stccpy(entry->ei_name, sftp_entry->name, FILENAMELEN + 1);
	entry->ei_size = sftp_entry->size;
	entry->ei_type = sftp_entry->type;
	entry->ei_seconds = ftp_sftp_unix_to_amiga_seconds(sftp_entry->seconds);
	entry->ei_prot = prot_unix_to_amiga(sftp_entry->unixprot);
	entry->ei_unixprot = sftp_entry->unixprot;
	stccpy(entry->ei_comment, sftp_entry->comment, COMMENTLEN + 1);

	// Convert UTF-8 filename to local charset if server supports UTF-8
	if (ui->ui_ftpnode)
		ftp_convert_filename_from_utf8(entry, &ui->ui_ftpnode->fn_ftp);

	AddTail(&ui->ui_list->rl_list, &entry->ei_node);
	++ui->ui_list->rl_entry_count;

	return 1;
}

static void rec_entry_list_clear(struct rec_entry_list *rel)
{
	struct entry_info *entry, *next;

	if (!rel)
		return;

	for (entry = (struct entry_info *)rel->rl_list.lh_Head; entry->ei_node.ln_Succ; entry = next)
	{
		next = (struct entry_info *)entry->ei_node.ln_Succ;
		FreeVec(entry);
	}

	NewList(&rel->rl_list);
	rel->rl_entry_count = 0;
}

//
//	Get a list of entries from an FTP server
//
struct rec_entry_list *rec_ftp_list(endpoint *ep, char *dirname)
{
	struct rec_entry_list *rel;
	struct rec_updateinfo ui = {0};
	int list_result;

	// Valid?
	if (!ep)
		return 0;

	// Are we the owning task of this lister?
	if (ep->ep_ftpnode->fn_ftp.fi_task != FindTask(0))

		// Ask the appropriate lister to do it for us
		return (struct rec_entry_list *)rec_ask_lister_favour(FAVOUR_LIST, ep, dirname, 0);

	// D(bug( "FTP LIST %s\n", dir->ei_name ));

	// Allocate entry list
	if (!(rel = AllocVec(sizeof(struct rec_entry_list), MEMF_CLEAR)))
		return 0;

	// Initialize list
	NewList(&rel->rl_list);

	// Initialize callback info
	ui.ui_list = rel;
	ui.ui_ls_to_entryinfo = ep->ep_ftpnode->fn_ls_to_entryinfo;
	ui.ui_ftpnode = ep->ep_ftpnode;  // Set FTP node for UTF-8 conversion
	if (ep->ep_ftpnode->fn_systype == FTP_MACOS)
		ui.ui_flags |= UI_NO_LINK_FIELD;
	if (ep->ep_ftpnode->fn_ftp.fi_flags & FTP_IS_UNIX)
		ui.ui_flags |= UI_DOT_HIDDEN;

	// Passive mode required?
	ep->ep_ftpnode->fn_ftp.fi_flags &= ~FTP_PASSIVE;
	if (ep->ep_ftpnode->fn_site.se_env->e_passive)
		ep->ep_ftpnode->fn_ftp.fi_flags |= FTP_PASSIVE;

	// Call FTP LIST command. Both SFTP and plain FTP need UTF-8 when the server
	// advertises it, so convert once and use the result for either branch.
	{
		char *utf8_dirname = ftp_convert_path_to_utf8(dirname, &ep->ep_ftpnode->fn_ftp);
		const char *send_dirname = utf8_dirname ? utf8_dirname : dirname;

		if (ep->ep_ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
		{
			list_result = ftp_sftp_list(&ep->ep_ftpnode->fn_sftp, send_dirname, sftp_callback_func, &ui) ? 0 : -1;
		}
		else
		{
			for (;;)
			{
				ui.ui_ls_to_entryinfo = ep->ep_ftpnode->fn_ls_to_entryinfo;
				list_result = list(&ep->ep_ftpnode->fn_ftp, (int (*)(void *, const char *))callback_func, &ui, ep->ep_ftpnode->fn_lscmd, send_dirname);
				if (list_result == -2 && lister_fallback_list_command(ep->ep_ftpnode))
					rec_entry_list_clear(rel);
				else
					break;
			}
		}

		if (utf8_dirname)
			ftp_codesets_free(utf8_dirname);
	}

	if (list_result != 0)
	{
		rec_entry_list_clear(rel);
		FreeVec(rel);
		return 0;
	}

	return rel;
}

//
//	Free an entry list
//
void free_entry_list(struct rec_entry_list *rel)
{
	// Valid?
	if (!rel)
		return;

	rec_entry_list_clear(rel);

	// Free list
	FreeVec(rel);
}

//
//	Change dir on an FTP server
//
int rec_ftp_cwd(endpoint *ep, char *dirname)
{
	// Are we the owning task of this lister?
	if (ep->ep_ftpnode->fn_ftp.fi_task != FindTask(0))

		// Ask the appropriate lister to do it for us
		return rec_ask_lister_favour(FAVOUR_CWD, ep, dirname, 0);

	D(bug("FTP CWD  %s\n", dirname));

	if (ep->ep_ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
	{
		char *utf8_dirname = ftp_convert_path_to_utf8(dirname, &ep->ep_ftpnode->fn_ftp);
		const char *send_dirname = utf8_dirname ? utf8_dirname : dirname;
		int cwd_success = ftp_sftp_cwd(&ep->ep_ftpnode->fn_sftp, send_dirname);
		if (utf8_dirname) ftp_codesets_free(utf8_dirname);
		return cwd_success;
	}

	return ftp_cwd(&ep->ep_ftpnode->fn_ftp, 0, 0, dirname) == 250 ? 1 : 0;
}

//
//	Change dir on a file system
//
int rec_filesys_cwd(endpoint *ep, char *dirname)
{
	BPTR lock;

	D(bug("FS  CWD  %s\n", dirname));
	rec_filesys_set_ioerr(ep, 0);

	if ((lock = Lock(dirname, ACCESS_READ)))
	{
		UnLock(CurrentDir(lock));
		return 1;
	}

	rec_filesys_set_ioerr(ep, IoErr());
	return 0;
}

//
//	Change to parent of current dir on an FTP server
//
int rec_ftp_cdup(endpoint *ep)
{
	// Are we the owning task of this lister?
	if (ep->ep_ftpnode->fn_ftp.fi_task != FindTask(0))

		// Ask the appropriate lister to do it for us
		return rec_ask_lister_favour(FAVOUR_CDUP, ep, 0, 0);

	D(bug("FTP CDUP\n"));

	if ((ep->ep_ftpnode->fn_protocol == FTP_PROTOCOL_SFTP && ftp_sftp_cdup(&ep->ep_ftpnode->fn_sftp)) ||
		(ep->ep_ftpnode->fn_protocol != FTP_PROTOCOL_SFTP && ftp_cdup(&ep->ep_ftpnode->fn_ftp, 0, 0) == 250))
		return 1;
	else
		return 0;
}

//
//	Change to parent of current dir on a filesystem
//
int rec_filesys_cdup(endpoint *ep)
{
	BPTR lock, newlock;
	int ret_val = 0;

	D(bug("FS  CDUP\n"));

	// Valid?
	if (!ep)
		return 0;

	rec_filesys_set_ioerr(ep, 0);

	/******************************
	Problems with this code. It does not check first lock is 0 i.e. on a root
	 filesystem. This results in the currentdir being set to sys:

	lock = Lock( "", ACCESS_READ );
	newlock = ParentDir( lock );
	UnLock( lock );
	UnLock( CurrentDir( newlock ) );
	return 1;
	***********************/

	if ((lock = Lock("", ACCESS_READ)))
	{
		if ((newlock = ParentDir(lock)))
		{
			UnLock(CurrentDir(newlock));
			ret_val = 1;
		}
		UnLock(lock);
	}

	if (!ret_val)
		rec_filesys_set_ioerr(ep, IoErr());

	return (ret_val);
}

//
//	Make a new subdir on an FTP server
//
//	Returns 0 if dir was neither created or already existing
//	Returns 1 if dir was created
//	Returns 2 if dir already existed
//	Returns 3 if not possible to distinguish between 0 and 2
//
int rec_ftp_mkdir(endpoint *ep, char *dirname)
{
	int reply;
	int retval;

	// Are we the owning task of this lister?
	if (ep->ep_ftpnode->fn_ftp.fi_task != FindTask(0))

		// Ask the appropriate lister to do it for us
		return rec_ask_lister_favour(FAVOUR_MKD, ep, dirname, 0);

	// D(bug( "FTP MKD  %s\n", dir->ei_name ));

	// Can TIMEOUT
	if (ep->ep_ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
	{
		char *utf8_dirname = ftp_convert_path_to_utf8(dirname, &ep->ep_ftpnode->fn_ftp);
		const char *send_dirname = utf8_dirname ? utf8_dirname : dirname;
		reply = ftp_sftp_mkdir(&ep->ep_ftpnode->fn_sftp, send_dirname) ? 250 : 550;
		if (utf8_dirname) ftp_codesets_free(utf8_dirname);
	}
	else
		reply = ftp_mkd(&ep->ep_ftpnode->fn_ftp, dirname);

	if (reply / 100 == COMPLETE)
		retval = 1;
	else if (reply == 421)
		retval = 0;
	else
		retval = 3;

	D(bug("** FTP MKDIR = %ld\n", (long)retval));

	return retval;
}

//
//	Make a new subdir on a file system
//
//	Returns 0 if dir was neither created or already existing
//	Returns 1 if dir was created
//	Returns 2 if dir already existed
//	Returns 3 if not possible to distinguish between 0 and 2
//
//	Note: If we attempt to make a dir that already exists as a dir
//	we usually don't care.
//	If we attempt to make a dir that already exists as a file
//	we care but we don't seem to be able to test for this ):
//
int rec_filesys_mkdir(endpoint *ep, char *dirname)
{
	BPTR lock;
	D_S(struct FileInfoBlock, fib)
	int retval = 0;
	LONG ioerr = 0;

	D(bug("FS  MKD  <%s>\n", dirname));
	rec_filesys_set_ioerr(ep, 0);

	// Try to create dir
	if ((lock = CreateDir(dirname)))
	{
		UnLock(lock);
		retval = 1;
	}

	// Already existed?
	else
	{
		ioerr = IoErr();
		rec_filesys_set_ioerr(ep, ioerr);

		if (ioerr == ERROR_OBJECT_EXISTS)
		{
			// Yes but is it a dir?
			if ((lock = Lock(dirname, ACCESS_READ)))
			{
				if (Examine(lock, fib))
				{
					if (fib->fib_DirEntryType >= 0)
						// if	(fib->fib_EntryType >= 0)
						retval = 2;
				}
				UnLock(lock);
			}
			else
				rec_filesys_set_ioerr(ep, IoErr());
		}
	}

	if (retval)
		rec_filesys_set_ioerr(ep, 0);

	D(bug("** FS MKDIR = %ld\n", (long)retval));

	return retval;
}

//
//	Delete a file on an FTP server
//
int rec_ftp_dele(endpoint *ep, struct entry_info *file)
{
	D(bug("rec_ftp_dele()\n"));

	// Are we the owning task of this lister?
	if (ep->ep_ftpnode->fn_ftp.fi_task != FindTask(0))

		// Ask the appropriate lister to do it for us
		return rec_ask_lister_favour(FAVOUR_DELE, ep, file, 0);

	D(bug("FTP DELE %s\n", file->ei_name));

	if (ep->ep_ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
	{
		char *utf8_name = ftp_convert_path_to_utf8(file->ei_name, &ep->ep_ftpnode->fn_ftp);
		const char *send_name = utf8_name ? utf8_name : file->ei_name;
		int delete_success = ftp_sftp_delete(&ep->ep_ftpnode->fn_sftp, send_name);
		if (utf8_name) ftp_codesets_free(utf8_name);
		return delete_success;
	}

	return ftp_dele(&ep->ep_ftpnode->fn_ftp, file->ei_name) == 250 ? 1 : 0;
}

//
//	Delete a file on an file system
//
int rec_filesys_dele(endpoint *ep, struct entry_info *file)
{
	D(bug("FS  DELE %s\n", file->ei_name));

	return DeleteFile(file->ei_name);
}

//
//	Delete a directory on an FTP server
//
int rec_ftp_rmd(endpoint *ep, struct entry_info *dir)
{
	// Are we the owning task of this lister?
	if (ep->ep_ftpnode->fn_ftp.fi_task != FindTask(0))

		// Ask the appropriate lister to do it for us
		return rec_ask_lister_favour(FAVOUR_RMD, ep, dir, 0);

	D(bug("FTP RMD %s\n", dir->ei_name));

	if (ep->ep_ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
	{
		char *utf8_name = ftp_convert_path_to_utf8(dir->ei_name, &ep->ep_ftpnode->fn_ftp);
		const char *send_name = utf8_name ? utf8_name : dir->ei_name;
		int rmdir_success = ftp_sftp_rmdir(&ep->ep_ftpnode->fn_sftp, send_name);
		if (utf8_name) ftp_codesets_free(utf8_name);
		return rmdir_success;
	}

	return ftp_rmd(&ep->ep_ftpnode->fn_ftp, dir->ei_name) / 100 == COMPLETE ? 1 : 0;
}

//
//	Delete a directory on a file system
//
int rec_filesys_rmd(endpoint *ep, struct entry_info *dir)
{
	D(bug("FS  RMD %s\n", dir->ei_name));

	return DeleteFile(dir->ei_name);
}

//
//	Tell an FTP server to connect to a data port
//
int rec_ftp_port(endpoint *ep, struct sockaddr_in *addr, ULONG flags)
{
	// Are we the owning task of this lister?
	if (ep->ep_ftpnode->fn_ftp.fi_task != FindTask(0))

		// Ask the appropriate lister to do it for us
		return rec_ask_lister_favour(FAVOUR_PORT, ep, addr, (void *)(IPTR)flags);

	D(bug("FTP PORT\n"));

	if (ep->ep_ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
		return 0;

	// if	(ftp_port( &ep->ep_ftpnode->fn_ftp, flags, (char *)&addr->sin_addr, (char *)&addr->sin_port ) == 200)
	if (ftp_port(&ep->ep_ftpnode->fn_ftp, flags, addr) == 200)
		return 1;
	else
		return 0;
}

//
//	Tell a filesystem to connect to an FTP data port
//
int rec_filesys_port(endpoint *ep, struct sockaddr_in *addr, ULONG flags)
{
	D(bug("FS  PORT %lu:%lu\n", (unsigned long)addr->sin_addr.s_addr, (unsigned long)addr->sin_port));

	return 0;
}

//
//	Ask an FTP server for a port to connect to
//
struct sockaddr_in *rec_ftp_pasv(endpoint *ep)
{
	struct sockaddr_in *address = 0;

	// Are we the owning task of this lister?
	if (ep->ep_ftpnode->fn_ftp.fi_task != FindTask(0))
	{
		// Ask the appropriate lister to do it for us
		return (struct sockaddr_in *)rec_ask_lister_favour(FAVOUR_PASV, ep, 0, 0);
	}

	D(bug("FTP PASV\n"));

	if (ep->ep_ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
		return 0;

	// Send PASV command to FTP server
	if (ftp_pasv(&ep->ep_ftpnode->fn_ftp) == 227)
	{
		// Allocate socket address
		if ((address = AllocVec(sizeof(struct sockaddr_in), MEMF_CLEAR)))
		{
			if (!pasv_to_address(address, ep->ep_ftpnode->fn_ftp.fi_iobuf))
			{
				D(bug("** couldn't convert PASV reply\n"));

				if (address)
					FreeVec(address);

				address = 0;
			}
		}
	}

	return address;
}

//
//	Ask a filesystem for a TCP data port to connect to
//
struct sockaddr_in *rec_filesys_pasv(endpoint *ep)
{
	D(bug("FS  PASV\n"));

	return 0;
}

//
//	Tell an FTP server to restart the next retrieve or store at a certain offset
//
int rec_ftp_rest(endpoint *ep, unsigned int offset)
{
	// Are we the owning task of this lister?
	if (ep->ep_ftpnode->fn_ftp.fi_task != FindTask(0))

		// Ask the appropriate lister to do it for us
		return rec_ask_lister_favour(FAVOUR_REST, ep, (void *)(IPTR)offset, 0);

	D(bug("FTP REST %lu\n", (unsigned long)offset));

	if (ep->ep_ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
		return offset == 0;

	return ftpa(&ep->ep_ftpnode->fn_ftp, "REST %lu", offset);
}

//
//	Tell a filesystem to restart the next retrieve or store at a certain offset
//
int rec_filesys_rest(endpoint *ep, unsigned int offset)
{
	D(bug("FS  REST %lu\n", (unsigned long)offset));

	return 0;
}

//
//	Tell an FTP server to send us a file we want to retrieve
//
int rec_ftp_retr(endpoint *ep, char *file)
{
	int reply;

	// Are we the owning task of this lister?
	if (ep->ep_ftpnode->fn_ftp.fi_task != FindTask(0))

		// Ask the appropriate lister to do it for us
		return rec_ask_lister_favour(FAVOUR_RETR, ep, file, 0);

	D(bug("FTP RETR %s\n", file));

	if (ep->ep_ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
		return 0;

	{
		char *utf8_file = NULL;
		
		// Convert filename to UTF-8 if server supports UTF-8
		utf8_file = ftp_convert_path_to_utf8(file, &ep->ep_ftpnode->fn_ftp);
		
		reply = ftpa(&ep->ep_ftpnode->fn_ftp, "RETR %s", utf8_file ? utf8_file : file);
		
		// Clean up converted filename
		if (utf8_file)
			ftp_codesets_free(utf8_file);
	}

	return reply == 150 || reply == 125;
}

//
//	Tell a filesystem to send a file so it can be retrieved
//
int rec_filesys_retr(endpoint *ep, char *file)
{
	D(bug("FS  RETR %s\n", file));

	return 0;
}

//
//	Tell an FTP server to receive a file we want to store
//
int rec_ftp_stor(endpoint *ep, char *file)
{
	int reply;

	// Are we the owning task of this lister?
	if (ep->ep_ftpnode->fn_ftp.fi_task != FindTask(0))

		// Ask the appropriate lister to do it for us
		return rec_ask_lister_favour(FAVOUR_STOR, ep, file, 0);

	D(bug("FTP STOR %s\n", file));

	if (ep->ep_ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
		return 0;

	{
		char *utf8_file = NULL;
		
		// Convert filename to UTF-8 if server supports UTF-8
		utf8_file = ftp_convert_path_to_utf8(file, &ep->ep_ftpnode->fn_ftp);
		
		reply = ftpa(&ep->ep_ftpnode->fn_ftp, "STOR %s", utf8_file ? utf8_file : file);
		
		// Clean up converted filename
		if (utf8_file)
			ftp_codesets_free(utf8_file);
	}

	return reply == 150 || reply == 125;
}

//
//	Tell a filesystem to receive a file we want to store
//
int rec_filesys_stor(endpoint *ep, char *file)
{
	D(bug("FS  STOR %s\n", file));

	return 0;
}

//
//	Tell an FTP server to abort a transfer
//
int rec_ftp_abor(endpoint *ep)
{
	D(bug("rec_ftp_abor()\n"));

	// Are we the owning task of this lister?
	if (ep->ep_ftpnode->fn_ftp.fi_task != FindTask(0))
	{
		// Ask the appropriate lister to do it for us
		return rec_ask_lister_favour(FAVOUR_ABOR, ep, 0, 0);
	}

	ftp_abor(&ep->ep_ftpnode->fn_ftp);

	return 1;
}

//
//	Set date on a filesystem
//
int rec_filesys_setdate(endpoint *ep, char *name, ULONG seconds)
{
	struct DateStamp ds = {0};

	seconds_to_datestamp(&ds, seconds);

	return SetFileDate(name, &ds);
}

//
//	Set (Unix-style) protection bits on an FTP site
//
int rec_ftp_chmod(endpoint *ep, char *name, ULONG mode)
{
	// Are we the owning task of this lister?
	if (ep->ep_ftpnode->fn_ftp.fi_task != FindTask(0))

		// Ask the appropriate lister to do it for us
		return rec_ask_lister_favour(FAVOUR_CHMOD, ep, name, (void *)(IPTR)mode);

	if (ep->ep_ftpnode->fn_protocol == FTP_PROTOCOL_SFTP)
	{
		char *utf8_name = ftp_convert_path_to_utf8(name, &ep->ep_ftpnode->fn_ftp);
		const char *send_name = utf8_name ? utf8_name : name;
		int chmod_success = ftp_sftp_chmod(&ep->ep_ftpnode->fn_sftp, send_name, mode);
		if (utf8_name) ftp_codesets_free(utf8_name);
		return chmod_success;
	}

	if (ep->ep_ftpnode->fn_ftp.fi_flags & FTP_NO_CHMOD)
		return 0;

	return ftp_chmod(&ep->ep_ftpnode->fn_ftp, mode, name);
}

//
//	Set (Unix-style) protection bits on a filesystem
//
int rec_filesys_chmod(endpoint *ep, char *name, ULONG mode)
{
	LONG mask = 0x0f;

	if (mode & 0400)
		mask &= ~FIBF_READ;

	if (mode & 0200)
		mask &= ~(FIBF_WRITE | FIBF_DELETE);

	if (mode & 0100)
		mask &= ~FIBF_EXECUTE;

	return SetProtection(name, mask);
}

//
//	Set a filenote (comment) on a filesystem
//
int rec_filesys_setnote(endpoint *ep, char *name, char *comment)
{
	if (strlen(comment) > 79)
	{
		D(bug("**Comment Trimmed ** - "));
		D(bug("name:<%s> Comment:<%s>\n", name, comment));
		comment[79] = 0;
	}
	return (SetComment(name, comment));
}

//
//	Set protection bits (only used for archive bit for now) on filesystem
//
int rec_filesys_setprot(endpoint *ep, char *name, LONG prot)
{
	BPTR lock;
	D_S(struct FileInfoBlock, fib)
	int retval = 0;

	if ((lock = Lock(name, ACCESS_READ)))
	{
		if (Examine(lock, fib))
		{
			// Reverse status of RWED bits
			fib->fib_Protection ^= FIBF_READ | FIBF_WRITE | FIBF_EXECUTE | FIBF_DELETE;
			fib->fib_Protection |= prot;
			fib->fib_Protection ^= FIBF_READ | FIBF_WRITE | FIBF_EXECUTE | FIBF_DELETE;

			retval = SetProtection(name, fib->fib_Protection);
		}

		UnLock(lock);
	}

	return retval;
}

//
//	Get an FTP reply from an FTP server
//
int rec_ftp_getreply(endpoint *ep, ULONG flags)
{
	// Are we the owning task of this lister?
	if (ep->ep_ftpnode->fn_ftp.fi_task != FindTask(0))

		// Ask the appropriate lister to do it for us
		return rec_ask_lister_favour(FAVOUR_GETREPLY, ep, (void *)(IPTR)flags, 0);

	return _getreply(&ep->ep_ftpnode->fn_ftp, flags, 0, 0);
}

//
//	Get an FTP-style reply from a file system
//
int rec_filesys_getreply(endpoint *ep, ULONG flags)
{
	D(bug("FS  getreply\n"));

	return 600;
}

//
//	Wait for something to happen on control socket
//
int rec_ftp_select(endpoint *ep)
{
	struct opusftp_globals *ogp = ep->ep_ftpnode->fn_og;
	struct timeval t = {0};
	fd_set rd, ex;
	ULONG flags = SIGBREAKF_CTRL_D;
	int nds;
	int retval = 0;

	// Are we the owning task of this lister?
	if (ep->ep_ftpnode->fn_ftp.fi_task != FindTask(0))

		// Ask the appropriate lister to do it for us
		return rec_ask_lister_favour(FAVOUR_SELECT, ep, 0, 0);

	D(bug("FTP select\n"));

	/**********************************
	* far too short a timeout

	t.tv_secs = 0;
	t.tv_micro = 20;

	*******************************/

	t.tv_secs = 0;
	t.tv_micro = 500000;

	FD_ZERO(&rd);
	FD_ZERO(&ex);

	FD_SET(ep->ep_ftpnode->fn_ftp.fi_cs, &rd);
	FD_SET(ep->ep_ftpnode->fn_ftp.fi_cs, &ex);

	if ((nds = WaitSelect(ep->ep_ftpnode->fn_ftp.fi_cs + 1, &rd, 0L, &ex, &t, &flags)) > 0)
	{
		// Data ready?
		if (FD_ISSET(ep->ep_ftpnode->fn_ftp.fi_cs, &rd))
			retval = 1;

		// Socket closed etc?
		else if (FD_ISSET(ep->ep_ftpnode->fn_ftp.fi_cs, &ex))
		{
			D(bug("** getput select exception\n"));
			retval = -2;
		}
	}

	// Timeout/Abort?
	else if (nds == 0)
	{
		// Abort?
		if (flags & SIGBREAKF_CTRL_D)
		{
			D(bug("** getput select abort\n"));

			// Prevent more aborts coming in
			// ** This doesn't take into account abort signals sent from the
			// ** other lister!
			ep->ep_ftpnode->fn_ftp.fi_abortsignals = 0;

			retval = -1;
		}
		else
			retval = 0;
	}

	// Select error
	else
	{
		D(bug("** getput select error\n"));
		retval = -3;
	}

	return retval;
}

//
//	Show FTP error requester
//
int rec_ftp_errorreq(endpoint *ep, struct ftp_node *prognode, ULONG flags)
{
	// Are we the owning task of this lister?
	if (ep->ep_ftpnode->fn_ftp.fi_task != FindTask(0))

		// Ask the appropriate lister to do it for us
		return rec_ask_lister_favour(FAVOUR_ERRORREQ, ep, prognode, (void *)(IPTR)flags);

	return lst_server_err(ep->ep_ftpnode->fn_og, prognode, ep->ep_ftpnode, flags, 0);
}

//
//	Show AmigaDos error requester
//
int rec_filesys_errorreq(endpoint *ep, struct ftp_node *prognode, ULONG flags)
{
	LONG ioerr = 0;

	if (!prognode)
		return 0;

	if (ep && ep->ep_ftpnode)
		ioerr = ep->ep_ftpnode->fn_ftp.fi_ioerr;

	if (!ioerr)
		ioerr = prognode->fn_ftp.fi_ioerr;

	return lst_dos_err(prognode->fn_og, prognode, flags, ioerr);
}

//
//	Get Copy or Delete options for this site
//
unsigned long rec_ftp_opts(endpoint *ep, int type)
{
	return lister_options(ep->ep_ftpnode, type);
}

//
//	Get global Copy or Delete options
//
unsigned long rec_filesys_opts(endpoint *ep, int type)
{
	D(bug("**** FILESYS OPTIONS!! ****\n"));
	return ftpmod_options(ep->ep_ftpnode->fn_og, type);
}

//
//	Create an entry_info from FTP server info
//
struct entry_info *rec_ftp_getentry(endpoint *ep, char *name)
{
	struct entry_info *ei;

	D(bug("FTP getentry\n"));

	// Are we the owning task of this lister?
	if (ep->ep_ftpnode->fn_ftp.fi_task != FindTask(0))

		// Ask the appropriate lister to do it for us
		return (struct entry_info *)rec_ask_lister_favour(FAVOUR_GETENTRY, ep, name, 0);

	if ((ei = AllocVec(sizeof(struct entry_info), MEMF_CLEAR)))
	{
		if (!entry_info_from_remote(ep->ep_ftpnode, name, ei, 0))
		{
			FreeVec(ei);
			ei = 0;
		}
	}

	return ei;
}

//
//	Create an entry_info from filesystem info
//
struct entry_info *rec_filesys_getentry(endpoint *ep, char *name)
{
	BPTR lock;
	D_S(struct FileInfoBlock, fib)
	struct entry_info *ei;
	int success = 0;

	D(bug("FS  getentry\n"));

	if ((ei = AllocVec(sizeof(struct entry_info), MEMF_CLEAR)))
	{
		if ((lock = Lock(name, ACCESS_READ)))
		{
			if (Examine(lock, fib))
			{
				success = 1;
				fileinfoblock_to_entryinfo(ei, fib);
			}
			UnLock(lock);
		}

		if (success == 0)
		{
			FreeVec(ei);
			ei = 0;
		}
	}

	return ei;
}

//
//	Catch unimplemented commands on FTP server
//
int rec_ftp_dummy(endpoint *ep, struct entry_info *entry)
{
	D(bug("FTP DUMMY\n"));

	return 0;
}

//
//	Catch unimplemented commands on filesystem
//
int rec_filesys_dummy(endpoint *ep, struct entry_info *entry)
{
	D(bug("FS  DUMMY\n"));

	return 0;
}

//
//	Ask another lister to do an FTP command for us
//	(This is because each process can only access its own sockets)
//
static IPTR rec_ask_lister_favour(int favour, endpoint *ep, void *arg1, void *arg2)
{
	struct favour_msg *fm;
	IPTR retval = 0;

	if (favour < FAVOUR_LIST || favour >= FAVOUR_ENDLIST)
		return 0;

	if ((fm = AllocVec(sizeof(*fm), MEMF_CLEAR)))
	{
		fm->fm_ftp_command = favour;
		fm->fm_endpoint = ep;
		fm->fm_arg1 = arg1;
		fm->fm_arg2 = arg2;

		retval = IPC_Command(ep->ep_ftpnode->fn_ipc, IPC_FAVOUR, 0, 0, fm, REPLY_NO_PORT);
	}

	return retval;
}

/**
 }*
 **/
