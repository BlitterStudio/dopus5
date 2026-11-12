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

#include <dopus/common.h>
#include <proto/dopus5.h>
#include <proto/configopus.h>
#include <proto/module.h>

#include "ftp.h"
#include "ftp_ipc.h"
#include "ftp_addressbook.h"
#include "ftp_addrsupp.h"
#include "ftp_addrformat.h"
#include "ftp_recursive.h"
#include "ftp_util.h"
#include "ftp_protocol.h"

char *CONFIGFILE = "DOpus5:System/ftp.config";
char *FTP_OPTIONS_NAME = "DOpus5:System/ftp_options";
char *FTP_ADDR_NAME = "DOpus5:System/ftp_sites";

enum {
	ERROR_READ,
	ERROR_WRITE,
};

#define SMALL_SIZE (offsetof(struct site_entry, se_env_private))
#define LARGE_SIZE (offsetof(struct site_entry, se_env))

static Att_List *do_import_amftp(struct display_globals *dg, char *path);

static char *head1 =
	"#\
# Directory Opus 5.8 FTP client site list\n\
#\n\
# Site Template is	ANONYMOUS=ANON/S,USERACCOUNT=ACCT/S,ALIAS=NAME/K,HOST,\n\
#			ADDRESS=ADDR/K,PORT/N,PATH=DIR,USER/K,PASSWORD=PASS/K,PROTOCOL=PROTO/K\n\
#\n";

#define LINEBUFLEN 512

//
//	Read in the old ascii config file
//

VOID read_old_config(struct ftp_config *oc)
{
	struct ftp_environment *env;
	FuncArgs *fa;
	char linebuf[LINEBUFLEN];  // Buffer for each line read
	BPTR cfp;

	env = &oc->oc_env;

	if ((cfp = Open(CONFIGFILE, MODE_OLDFILE)))
	{
		while (FGets(cfp, linebuf, LINEBUFLEN))
		{
			if (*linebuf == '#' || *linebuf == '\n')
				continue;

			if ((fa = ParseArgs(CONFIG_TEMPLATE, linebuf)))
			{
				if (fa->FA_Arguments[C_OPT_LOG])
				{
					stccpy(oc->oc_logname, (char *)fa->FA_Arguments[C_OPT_LOG], LOGNAMELEN);
					oc->oc_enable_log = TRUE;
				}

				if (fa->FA_Arguments[C_OPT_LOGOFF])
					oc->oc_enable_log = FALSE;

				if (fa->FA_Arguments[C_OPT_TIME])
				{
					env->e_timeout = *(int *)fa->FA_Arguments[C_OPT_TIME];

					if (env->e_timeout > 999 || env->e_timeout < 0)
						env->e_timeout = REPLYTIMEOUT;
				}

				if (fa->FA_Arguments[C_OPT_UPDT])
				{
					env->e_list_update = *(int *)fa->FA_Arguments[C_OPT_UPDT];

					if (env->e_list_update > 99 || env->e_list_update < 0)
						env->e_list_update = 1;
				}

				if (fa->FA_Arguments[C_OPT_DBUG])
					oc->oc_log_debug = *(int *)fa->FA_Arguments[C_OPT_DBUG];

				if (fa->FA_Arguments[C_OPT_AUTOINDEX])
					env->e_index_auto = TRUE;

				if (fa->FA_Arguments[C_OPT_SHOWINDEX])
					env->e_index_enable = TRUE;

				if (fa->FA_Arguments[C_OPT_NOOP])
					env->e_noops = TRUE;

				if (fa->FA_Arguments[C_OPT_INDEXSIZE])
				{
					env->e_indexsize = *(int *)fa->FA_Arguments[C_OPT_INDEXSIZE];

					if (env->e_indexsize > 9999 || env->e_indexsize <= 0)
						env->e_indexsize = 30;
				}

				if (fa->FA_Arguments[C_OPT_ANONPASS])
				{
					stccpy(oc->oc_anonpass, (char *)fa->FA_Arguments[C_OPT_ANONPASS], PASSWORDLEN);
					oc->oc_user_password = TRUE;
				}

				DisposeArgs(fa);
			}
		}
		Close(cfp);
	}
}

BPTR setup_config(struct opusftp_globals *ogp)
{
	BPTR lfp = NULL;
	struct ftp_config *oc;
	BPTR lock;

	oc = &ogp->og_oc;

	// initialise blank config and read config from disk

	if (!get_global_options(ogp))
	{
		set_config_to_default(oc);

		// check for old config and read if there

		if ((lock = Lock(CONFIGFILE, ACCESS_READ)))
		{
			UnLock(lock);
			read_old_config(oc);
		}
	}

	// One-shot migration: import the legacy DOpus/NoBeeGees env var into
	// the new oc_no_keep_alive bit, then delete the var from both ENV:
	// and ENVARC: so the migration runs exactly once per machine.
	// Without the DeleteVar a user who turns the box OFF in the GUI and
	// saves would still see it re-enabled on the next launch (the saved
	// bit is back to 0, and the legacy env var would re-assert it).
	{
		char env;
		if (GetVar("DOpus/NoBeeGees", &env, 1, GVF_GLOBAL_ONLY) != -1)
		{
			if (!oc->oc_no_keep_alive)
				oc->oc_no_keep_alive = 1;
			DeleteVar("DOpus/NoBeeGees", GVF_GLOBAL_ONLY | GVF_SAVE_VAR);
		}
	}

	// Open log file
	if (*oc->oc_logname && oc->oc_enable_log)
	{
		if ((lfp = Open(oc->oc_logname, MODE_NEWFILE)))
			ogp->og_log_open = TRUE;
	}

	return lfp;
}

/*
 *	Clean up anything that was set up in the log file
 */

void cleanup_config(BPTR cfp)
{
	if (cfp)
		Close(cfp);
}

/******************
 *
 *	Write the config plus site details
 *	Remember to make sure that the buf len does not exceed 512 bytes
 *
 * these two fns return TRUE if write OK, FALSE if failure
 */

static BOOL write_entry(BPTR cf, struct site_entry *e)
{
	static char num[16];
	char buf[LINEBUFLEN]; /* Buffer for each line writen */

	char *p;

	*buf = 0;

	strcpy(buf, e->se_anon ? "ANON " : "ACCT ");

	if (*e->se_name && strcmp(e->se_name, e->se_host))
	{
		strcat(buf, "NAME ");

		// check if user has puts in spaces!

		if ((p = strchr(e->se_name, ' ')))
			strcat(buf, "\"");

		strcat(buf, e->se_name);

		if (p)
			strcat(buf, "\"");

		strcat(buf, " ");
	}

	strcat(buf, "HOST ");
	strcat(buf, e->se_host);
	strcat(buf, " ");

	if (e->se_protocol != FTP_PROTOCOL_FTP)
	{
		strcat(buf, "PROTO ");
		strcat(buf, ftp_protocol_name(e->se_protocol));
		strcat(buf, " ");
	}

	if (e->se_port != ftp_protocol_default_port(e->se_protocol))
	{
		sprintf(num, "%ld", (long)e->se_port);  // se_port is LONG
		// stcul_d(num, e->se_port);
		strcat(buf, "PORT ");
		strcat(buf, num);
		strcat(buf, " ");
	}

	if (!e->se_anon)
	{
		strcat(buf, "USER ");
		strcat(buf, e->se_user);
		strcat(buf, " ");

		if (*e->se_pass)
		{
			strcat(buf, "PASS ");
			strcat(buf, e->se_pass);
			strcat(buf, " ");
		}
	}

	if (*e->se_path)
	{
		strcat(buf, "DIR ");

		if ((p = strchr(e->se_path, ' ')))
			strcat(buf, "\"");

		strcat(buf, e->se_path);

		if (p)
			strcat(buf, "\"");
	}

	strcat(buf, "\n");

	if (Write(cf, buf, strlen(buf)) < 0)
		return (FALSE);

	return (TRUE);
}

static BOOL write_site_protocol(APTR iff, LONG protocol)
{
#ifdef __AROS__
	LONG protocol_be = AROS_LONG2BE(protocol);

	return IFFPushChunk(iff, ID_FTPSITE_PROTOCOL) && IFFWriteChunkBytes(iff, (char *)&protocol_be, sizeof(protocol_be)) &&
		   IFFPopChunk(iff);
#else
	return IFFPushChunk(iff, ID_FTPSITE_PROTOCOL) && IFFWriteChunkBytes(iff, (char *)&protocol, sizeof(protocol)) &&
		   IFFPopChunk(iff);
#endif
}

static int disk_error(struct opusftp_globals *ogp, struct Window *win, LONG err, int type, IPCData *ipc)
{
	char buf[80], error_text[140];
	int result = 1;
	struct TagItem tags[] = {
		{AR_Window, 0}, {AR_Screen, 0}, {AR_Message, 0}, {AR_Button, 0}, {AR_Button, 0}, {TAG_DONE}};

	struct Screen *screen;

	char *title = GetString(locale, type == ERROR_READ ? MSG_ERROR_LOADING : MSG_ERROR_SAVING);

	if (win)
		screen = win->WScreen;
	else
		screen = ogp->og_screen;

	Fault(err, "", buf, 80);  // dos error

	// Build requester text
	lsprintf(error_text, "%s\n%s %ld%s", title, GetString(locale, MSG_FTP_DOS_ERROR), err, buf);

	if (!ogp->og_noreq)
	{
		tags[0].ti_Data = (IPTR)win;
		tags[1].ti_Data = (IPTR)screen;
		tags[2].ti_Data = (IPTR)error_text;
		tags[3].ti_Data = (IPTR)GetString(locale, MSG_RETRY);
		tags[4].ti_Data = (IPTR)GetString(locale, MSG_FTP_CANCEL);

		result = AsyncRequest(ipc, REQTYPE_SIMPLE, win, NULL, NULL, tags);
	}

	return (result);
}

/*******************************************************
 *
 *	Display a filerequester in a configwindow
 *
 */

static BOOL do_filereq(struct Window *win, char *path, int title, ULONG flags)
{
	BOOL ok;

	char *ptr = 0;
	char file[40];

	if (!win)
		return (FALSE);

	// Invalid path?
	if (!path[0])
		strcpy(path, "DOpus5:System/");

	// Get filename
	else if ((ptr = FilePart(path)))
	{
		strcpy(file, ptr);
		*ptr = 0;
		ptr = file;
	}

	// Display file requester
	if ((ok = AslRequestTags(DATA(win)->request,
							 ASLFR_Window,
							 win,
							 ASLFR_TitleText,
							 GetString(locale, title),
							 ASLFR_InitialFile,
							 (ptr) ? ptr : "",
							 ASLFR_InitialDrawer,
							 path,
							 ASLFR_Flags1,
							 FRF_PRIVATEIDCMP | flags,
							 TAG_END)))
	{
		// Get new path
		strcpy(path, DATA(win)->request->fr_Drawer);
		AddPart(path, DATA(win)->request->fr_File, 256);
	}

	return (ok);
}

static BOOL do_export_sites(struct display_globals *dg, char *path, LONG *diskerr)
{
	BPTR cf;
	struct site_entry *e;
	struct Node *node, *next;
	BOOL ok = FALSE;

	if ((cf = Open(path, MODE_NEWFILE)))
	{
		if ((ok = Write(cf, head1, strlen(head1))))
		{
			node = dg->dg_og->og_SiteList->list.lh_Head;
			while ((next = node->ln_Succ))
			{
				e = (struct site_entry *)((Att_Node *)node)->data;
				if (!(ok = write_entry(cf, e)))
					break;
				node = next;
			}
		}
		Close(cf);
	}

	if (!ok)
		*diskerr = IoErr();

	return (ok);
}

BOOL export_sites(struct display_globals *dg, struct Window *win)
{
	short loop;
	char path[256];
	BOOL ret = FALSE;

	// Make window busy
	if (win)
		SetWindowBusy(win);

	// clear path
	*path = 0;

	// Loop until successful
	do
	{
		// Clear loop flag
		loop = 0;

		if (do_filereq(win, path, MSG_FTP_SAVE_SITES, FRF_DOSAVEMODE))
		{
			LONG err = 0;
			// Save TRUE == ok
			if (do_export_sites(dg, path, &err))
				ret = TRUE;

			// Error?
			else
			{
				// Show error
				loop = disk_error(dg->dg_og, win, err, ERROR_WRITE, dg->dg_ipc);
			}
		}

	} while (loop);

	// Make window unbusy
	if (win)
		ClearWindowBusy(win);

	return (ret);
}

/***********************************************
 *
 *	Actually read the config data according to type
 *	Either custom for ftp_config for individual site
 *	or full default options
 *	treat size as full ftp_config and select sub-set if required
 */

static struct ftp_config *do_read_options_iff(char *filename, int opt_type, LONG *diskerr)
{
	APTR iff;
	BOOL ok = FALSE;
	struct ftp_config *conf;
	LONG chunk;

	D(bug("read options\n"));

	if ((conf = AllocVec(sizeof(struct ftp_config), MEMF_CLEAR)))
	{
		// Open IFF file
		if ((iff = IFFOpen(filename, IFF_READ, ID_OPUS)))
		{
			chunk = IFFNextChunk(iff, 0);

			if (opt_type == WT_OPT && chunk != ID_ENV)
				*diskerr = 212;
			else
			{
				// Adjustment for changed ListFormat size
				int chunksize = (int)IFFChunkSize(iff);
				int size = sizeof(struct ftp_config);

				if (chunk == ID_OPTIONS)
				{
					if ((chunksize < size) && (size - chunksize == 84))
					{
						if ((ok = (chunksize == IFFReadChunkBytes(iff, (char *)conf, chunksize))))
							;
						{
							char patterns[82] = {'\0'};
							int i = 0;

							CopyMem(conf->oc_env.e_listformat.show_pattern_p, patterns, 80);
							memset(conf->oc_env.e_listformat.show_pattern_p, 0, 160);
							for (i = 0; i < 40; i++)
							{
								conf->oc_env.e_listformat.show_pattern_p[i] = patterns[i];
								conf->oc_env.e_listformat.hide_pattern_p[i] = patterns[i + 40];
							}
						}
					}
					else
						ok = ((sizeof(struct ftp_config)) ==
							  IFFReadChunkBytes(iff, (char *)conf, sizeof(struct ftp_config)));
				}
				else if (chunk == ID_ENV)
				{
					ok = ((sizeof(struct ftp_environment)) ==
						  IFFReadChunkBytes(iff, (char *)&conf->oc_env, sizeof(struct ftp_environment)));
				}
				else
					*diskerr = 212;
			}
			// Close file
			IFFClose(iff);

			if (ok)
			{
#ifdef __AROS__
				conf->bitfield1 = AROS_LONG2BE(conf->bitfield1);

				conf->oc_env.e_retry_count = AROS_BE2LONG(conf->oc_env.e_retry_count);
				conf->oc_env.e_retry_delay = AROS_BE2LONG(conf->oc_env.e_retry_delay);
				conf->oc_env.e_list_update = AROS_BE2LONG(conf->oc_env.e_list_update);
				conf->oc_env.e_timeout = AROS_BE2LONG(conf->oc_env.e_timeout);
				conf->oc_env.e_script_time = AROS_BE2LONG(conf->oc_env.e_script_time);
				conf->oc_env.e_indexsize = AROS_BE2LONG(conf->oc_env.e_indexsize);

				conf->oc_env.bitfield1 = AROS_BE2LONG(conf->oc_env.bitfield1);
				conf->oc_env.bitfield2 = AROS_BE2LONG(conf->oc_env.bitfield2);
#endif
				return (conf);
			}
		}

		FreeVec(conf);
	}

	if (!*diskerr)
		*diskerr = IoErr();

	return (NULL);
}

/*************************************
 * Do save of options of either full ftp_config or sub set of ftp_environment
 * Custom environments attached to a site_entry are sub-set only
 *
 */

static BOOL do_save_options_iff(struct window_params *wp, char *filename, int type, LONG *diskerr)
{
	struct display_globals *dg;
	APTR iff;
	void *data;
	int size;
	BOOL ok = FALSE;
#ifdef __AROS__
	struct ftp_environment *env_be;
#endif

#ifdef __AROS__
	// struct ftp_config has a struct ftp_environment at the end, so this buffer is large enough to hold both
	if (!(data = AllocVec(sizeof(struct ftp_config), MEMF_CLEAR)))
	{
		*diskerr = IoErr();
		return (FALSE);
	}
#endif

	dg = wp->wp_dg;

	// Open IFF file
	if ((iff = IFFOpen(filename, IFF_WRITE | IFF_SAFE, ID_OPUS)))
	{
		if (type == ID_OPTIONS)
		{
			size = sizeof(struct ftp_config);
#ifdef __AROS__
			CopyMem(&dg->dg_oc, data, size);
			{
				struct ftp_config *conf_be = (struct ftp_config *)data;

				conf_be->bitfield1 = AROS_LONG2BE(conf_be->bitfield1);
				env_be = &conf_be->oc_env;
			}
#else
			data = &dg->dg_oc;
#endif
		}
		else
		{
			size = sizeof(struct ftp_environment);
#ifdef __AROS__
			CopyMem(wp->wp_se_copy->se_env, data, size);
			env_be = (struct ftp_environment *)data;
#else
			data = wp->wp_se_copy->se_env;
#endif

			// sould be ? data=&wp->wp_se_copy.se_env_private;;
		}

#ifdef __AROS__
		env_be->e_retry_count = AROS_LONG2BE(env_be->e_retry_count);
		env_be->e_retry_delay = AROS_LONG2BE(env_be->e_retry_delay);
		env_be->e_list_update = AROS_LONG2BE(env_be->e_list_update);
		env_be->e_timeout = AROS_LONG2BE(env_be->e_timeout);
		env_be->e_script_time = AROS_LONG2BE(env_be->e_script_time);
		env_be->e_indexsize = AROS_LONG2BE(env_be->e_indexsize);

		env_be->bitfield1 = AROS_LONG2BE(env_be->bitfield1);
		env_be->bitfield2 = AROS_LONG2BE(env_be->bitfield2);
#endif

		ok = IFFWriteChunk(iff, data, type, size);

		// Close file
		IFFClose(iff);
	}

#ifdef __AROS__
	FreeVec(data);
#endif

	if (ok)
		return (TRUE);

	*diskerr = IoErr();
	return (FALSE);
}

static BOOL do_save_sites_iff(struct display_globals *dg, char *filename, LONG *diskerr)
{
	APTR iff;
	BOOL ok = FALSE;
	struct Node *node, *next;
	struct site_entry *e;
#ifdef __AROS__
	struct site_entry *entry_be;

	if (!(entry_be = AllocVec(sizeof(struct site_entry), MEMF_CLEAR)))
	{
		//*diskerr = ERROR_NO_FREE_STORE;
		*diskerr = IoErr();
		return 0;
	}
#endif

	// Open IFF file
	if ((iff = IFFOpen(filename, IFF_WRITE | IFF_SAFE, ID_OPUS)))
	{
		LONG type;
		int size;

		ok = TRUE;	// mark in case no entries in list

		node = dg->dg_og->og_SiteList->list.lh_Head;
		while ((next = node->ln_Succ))
		{
			e = (struct site_entry *)((Att_Node *)node)->data;

			if (e->se_has_custom_env)
			{
				type = ID_FTPSITE_LONG;
				// full site so include ftp_environment
				size = LARGE_SIZE;
			}
			else
			{
				type = ID_FTPSITE_SHORT;
				// short site so exclude ftp_environment
				size = SMALL_SIZE;
			}

			// Write site_entry
			if (!(ok = IFFPushChunk(iff, type)))
				break;

#ifdef __AROS__
			CopyMem(e, entry_be, size);

			entry_be->se_anon = AROS_WORD2BE(entry_be->se_anon);
			entry_be->se_port = AROS_LONG2BE(entry_be->se_port);
			entry_be->se_has_custom_env = AROS_WORD2BE(entry_be->se_has_custom_env);
			if (size == LARGE_SIZE)
			{
				entry_be->se_env_private.e_retry_count = AROS_LONG2BE(entry_be->se_env_private.e_retry_count);
				entry_be->se_env_private.e_retry_delay = AROS_LONG2BE(entry_be->se_env_private.e_retry_delay);
				entry_be->se_env_private.e_list_update = AROS_LONG2BE(entry_be->se_env_private.e_list_update);
				entry_be->se_env_private.e_timeout = AROS_LONG2BE(entry_be->se_env_private.e_timeout);
				entry_be->se_env_private.e_script_time = AROS_LONG2BE(entry_be->se_env_private.e_script_time);
				entry_be->se_env_private.e_indexsize = AROS_LONG2BE(entry_be->se_env_private.e_indexsize);

				entry_be->se_env_private.bitfield1 = AROS_LONG2BE(entry_be->se_env_private.bitfield1);
				entry_be->se_env_private.bitfield2 = AROS_LONG2BE(entry_be->se_env_private.bitfield2);
			}
			if (!(ok = IFFWriteChunkBytes(iff, (char *)entry_be, size)))
#else
			if (!(ok = IFFWriteChunkBytes(iff, (char *)e, size)))
#endif
				break;

			if (!(ok = IFFPopChunk(iff)))
				break;

			if (e->se_protocol != FTP_PROTOCOL_FTP && !(ok = write_site_protocol(iff, e->se_protocol)))
				break;

			node = next;
		}

		// Close file
		IFFClose(iff);
	}

#ifdef __AROS__
	FreeVec(entry_be);
#endif

	if (ok)
		return (TRUE);

	*diskerr = IoErr();
	return (FALSE);
}

static Att_List *do_read_sites_iff(struct opusftp_globals *og, char *filename, LONG *diskerr)
{
	APTR iff;
	BOOL err = FALSE;
	Att_List *list;

	D(bug("read sites\n"));

	if (!(list = Att_NewList(LISTF_POOL)))
		return (NULL);

	// Open IFF file
	if ((iff = IFFOpen(filename, IFF_READ, ID_OPUS)))
	{
		struct site_entry *e;
		struct site_entry *last_site = NULL;
		ULONG chunk;

		while (!err && (chunk = IFFNextChunk(iff, 0)))
		{
			if (chunk == ID_FTPSITE_PROTOCOL)
			{
				LONG protocol = FTP_PROTOCOL_FTP;

				if (IFFChunkSize(iff) >= sizeof(protocol) &&
					IFFReadChunkBytes(iff, (char *)&protocol, sizeof(protocol)) == sizeof(protocol))
				{
#ifdef __AROS__
					protocol = AROS_BE2LONG(protocol);
#endif
					if (last_site && protocol == FTP_PROTOCOL_SFTP)
						last_site->se_protocol = protocol;
				}
				continue;
			}

			if ((e = AllocVec(sizeof(struct site_entry), MEMF_CLEAR)))
			{
				int size;
				int chunksize = 0;

				if (chunk == ID_FTPSITE_SHORT)
					size = SMALL_SIZE;
				else if (chunk == ID_FTPSITE_LONG)
					size = LARGE_SIZE;
				else
				{
					*diskerr = 212;
					err = TRUE;
					FreeVec(e);
					break;
				}

				// Adjustment for changed ListFormat size
				chunksize = (int)IFFChunkSize(iff);
				if ((chunksize < size) && (size - chunksize == 84))
				{
					if (!(err = (chunksize != IFFReadChunkBytes(iff, (char *)e, chunksize))))
						;
					{
						char patterns[82] = {'\0'};
						int i = 0;

						CopyMem(e->se_listformat.show_pattern_p, patterns, 80);
						memset(e->se_listformat.show_pattern_p, 0, 160);
						for (i = 0; i < 40; i++)
						{
							e->se_listformat.show_pattern_p[i] = patterns[i];
							e->se_listformat.hide_pattern_p[i] = patterns[i + 40];
						}
					}
				}
				else
					err = (size != IFFReadChunkBytes(iff, (char *)e, size));

				if (!err)
				{
#ifdef __AROS__
					e->se_anon = AROS_BE2WORD(e->se_anon);
					e->se_port = AROS_BE2LONG(e->se_port);
					e->se_has_custom_env = AROS_BE2WORD(e->se_has_custom_env);
					if (size == LARGE_SIZE)
					{
						e->se_env_private.e_retry_count = AROS_BE2LONG(e->se_env_private.e_retry_count);
						e->se_env_private.e_retry_delay = AROS_BE2LONG(e->se_env_private.e_retry_delay);
						e->se_env_private.e_list_update = AROS_BE2LONG(e->se_env_private.e_list_update);
						e->se_env_private.e_timeout = AROS_BE2LONG(e->se_env_private.e_timeout);
						e->se_env_private.e_script_time = AROS_BE2LONG(e->se_env_private.e_script_time);
						e->se_env_private.e_indexsize = AROS_BE2LONG(e->se_env_private.e_indexsize);

						e->se_env_private.bitfield1 = AROS_BE2LONG(e->se_env_private.bitfield1);
						e->se_env_private.bitfield2 = AROS_BE2LONG(e->se_env_private.bitfield2);
					}
#endif

					//  adjust ptrs for environment
					// if custom env then set ptr to internal private copy
					// else set it to point to global ftp default
					if (e->se_has_custom_env)
						e->se_env = &e->se_env_private;
					else
						e->se_env = &og->og_oc.oc_env;

					e->se_protocol = FTP_PROTOCOL_FTP;

					Att_NewNode(list, e->se_name, (IPTR)e, 0);
					last_site = e;
				}
				else
					FreeVec(e);
			}
			else
				err = TRUE;
		}

		// Close file
		IFFClose(iff);

		if (!err)
			return (list);
	}

	Att_RemList(list, REMLIST_FREEDATA);

	if (!*diskerr)
		*diskerr = IoErr();

	return (NULL);
}

/**********************************************************
 *
 *	Save site list.
 *
 *	May be called form non-adr task so window may be normal win or NULL
 */

BOOL save_sites(struct display_globals *dg, struct Window *win, short gadgetid)
{
	short ok = 1, loop;
	char path[256];
	BOOL ret = FALSE;

	// Make window busy
	if (win)
		SetWindowBusy(win);

	// Copy path
	strcpy(path, FTP_ADDR_NAME);

	// Loop until successful
	do
	{
		// Clear loop flag
		loop = 0;

		if (gadgetid == MENU_FTP_SAVEAS || !path[0])
			ok = do_filereq(win, path, MSG_FTP_SAVE_SITES, FRF_DOSAVEMODE);

		// Ok to save?
		if (ok)
		{
			LONG err = 0;
			// Save TRUE == ok
			if (do_save_sites_iff(dg, path, &err))
			{
				// Clear change flag
				dg->dg_site_change = 0;

				ret = TRUE;
			}

			// Error?
			else
			{
				// Show error
				loop = disk_error(dg->dg_og, win, err, ERROR_WRITE, dg->dg_ipc);
			}
		}

	} while (loop);

	// Make window unbusy
	if (win)
		ClearWindowBusy(win);

	return (ret);
}

BOOL save_options(struct window_params *wp, int opt_type, short gadgetid)
{
	struct display_globals *dg;
	short ok = 1, loop;
	char path[256];
	BOOL ret = FALSE;
	int type;

	if (!wp)
	{
		DisplayBeep(0);
		return (FALSE);
	}

	dg = wp->wp_dg;

	// Make window busy
	SetWindowBusy(wp->wp_win);

	if (opt_type == WT_DEFOPT)
	{
		type = ID_OPTIONS;

		// Copy default path
		strcpy(path, FTP_OPTIONS_NAME);
	}
	else
	{
		type = ID_ENV;
		*path = 0;
	}

	// Loop until successful
	do
	{
		// Clear loop flag
		loop = 0;

		if (gadgetid == MENU_ENV_SAVEAS || !path[0])
			ok = do_filereq(wp->wp_win,
							path,
							opt_type == WT_OPT ? MSG_ENV_SAVE_OPTIONS : MSG_ENV_SAVE_DEFAULT_OPTIONS,
							FRF_DOSAVEMODE);

		// Ok to save?
		if (ok)
		{
			LONG err = 0;
			// Save options TRUE == ok
			if (do_save_options_iff(wp, path, type, &err))
			{
				// Clear change flag
				dg->dg_options_change = 0;

				ret = TRUE;
			}

			// Error?
			else
			{
				// Show error
				loop = disk_error(dg->dg_og, wp->wp_win, err, ERROR_WRITE, dg->dg_ipc);
			}
		}

	} while (loop);

	// Make window unbusy
	ClearWindowBusy(wp->wp_win);

	return (ret);
}

/*************************************************
 *
 *	Read new addressbook from disk
 *	Called from menu_open
 *
 */

Att_List *read_sites(struct window_params *wp, short gadgetid)
{
	struct display_globals *dg;
	short loop;
	char path[256];

	Att_List *sitelist = NULL;

	if (!wp)
		return (NULL);

	dg = wp->wp_dg;

	// Make window busy
	SetWindowBusy(wp->wp_win);

	// Copy path use dg->dg_addressbook_name ?;
	strcpy(path, FTP_ADDR_NAME);

	// Loop until successful
	do
	{
		// Clear loop flag
		loop = 0;

		// Ok to read?
		if (do_filereq(wp->wp_win, path, MSG_FTP_READ_SITES, 0))
		{
			LONG err = 0;

			if ((sitelist = do_read_sites_iff(dg->dg_og, path, &err)))
			{
				// Clear change flag
				dg->dg_site_change = 0;
			}

			// Error?
			else
			{
				// Show error
				loop = disk_error(dg->dg_og, wp->wp_win, err, ERROR_READ, dg->dg_ipc);
			}
		}
	} while (loop);

	// Make window unbusy
	ClearWindowBusy(wp->wp_win);

	return (sitelist);
}

/**************************************************
 *
 *	Read options/environment file
 *
 *	opt_type===WT_OPT then read individual ftp_environment file for current wp
 *	otherwise we are reading the defauls global ftp_config
 *
 */

struct ftp_config *read_options(struct window_params *wp, int opt_type)
{
	struct display_globals *dg;
	short loop;
	char path[256];
	struct ftp_config *conf = NULL;

	if (!wp)
		return (NULL);

	dg = wp->wp_dg;

	// Make window busy
	SetWindowBusy(wp->wp_win);

	// Set up path
	if (opt_type == WT_OPT)
		*path = 0;
	else
		strcpy(path, FTP_OPTIONS_NAME);

	// Loop until successful
	do
	{
		// Clear loop flag
		loop = 0;

		// Ok to read?
		if (do_filereq(
				wp->wp_win, path, opt_type == WT_OPT ? MSG_ENV_READ_OPTIONS : MSG_ENV_READ_DEFAULT_OPTIONS, 0))
		{
			LONG err = 0;

			// read options TRUE == ok
			if ((conf = do_read_options_iff(path, opt_type, &err)))
			{
				// set change flag
				dg->dg_options_change = 1;
			}

			// Error?
			else
			{
				// Show error
				loop = disk_error(dg->dg_og, wp->wp_win, err, ERROR_READ, dg->dg_ipc);
			}
		}

	} while (loop);

	// Make window unbusy
	ClearWindowBusy(wp->wp_win);

	return (conf);
}

/*
static void set_lister_format(ListFormat *f)
{
f->sort.separation=SEPARATE_DIRSFIRST;

//fix for JPs -1 back fill in structure

memset(f->display_pos,-1,sizeof(f->display_pos));


f->display_pos[0]=DISPLAY_NAME;
f->display_pos[1]=DISPLAY_SIZE;
f->display_pos[2]=DISPLAY_DATE;
f->display_pos[3]=DISPLAY_PROTECT;
f->display_pos[4]=DISPLAY_COMMENT;
f->display_pos[5]=-1;

// to hide .* files!

f->flags=LFORMATF_HIDDEN_BIT;


}
*/

/******************************************************************
 *
 *	Sets or resets the global config to default values
 *
 *
 */

void set_config_to_default(struct ftp_config *oc)
{
	struct ftp_environment *env;

	// clear out all data to give false for defaults.

	memset(oc, 0, sizeof(struct ftp_config));

	sprintf(oc->oc_logname, "CON:/300/500/150/FTP Log/Auto/Close/Screen DOPUS.1");

	env = &oc->oc_env;

	env->e_retry_count = 5;
	env->e_retry_delay = 30;
	env->e_list_update = 10;
	env->e_timeout = 60;
	env->e_script_time = 120;
	env->e_indexsize = 30;

	// only need to set only those which need to be TRUE

	env->e_retry = env->e_retry_lost = env->e_index_enable = env->e_progress_window = env->e_index_auto =
		env->e_url_comment = TRUE;

	env->e_custom_format = FALSE;

	env->e_copy_type = COPY_TYPE_DEFAULT;

	env->e_copy_replace = COPY_REPLACE_ASK;

	// set_lister_format(&env->e_listformat);
}

/****************************************************
 *
 *	Called by startup routines for main ftp module
 *
 */

BOOL get_global_options(struct opusftp_globals *og)
{
	struct ftp_config *conf;
	LONG err = 0;

	if ((conf = do_read_options_iff(FTP_OPTIONS_NAME, WT_DEFOPT, &err)))
	{
		// copy new one over orig
		*(&og->og_oc) = *conf;

		FreeVec(conf);
		return (TRUE);
	}

	return (FALSE);
}

static BOOL read_addrbook(struct opusftp_globals *ogp, IPCData *ipc)
{
	BOOL ok = FALSE;
	Att_List *list;
	LONG err = 0;
	int loop = 0;

	do
	{
		loop = 0;
		if ((list = do_read_sites_iff(ogp, FTP_ADDR_NAME, &err)))
		{
			ogp->og_SiteList = list;
			ok = TRUE;
		}

		else if (err != 205)  // Error? but ignore object not found
		{
			// Show error
			loop = disk_error(ogp, NULL, err, ERROR_READ, ipc);
		}

	} while (loop);

	return (ok);
}

static Att_List *do_import_sites(struct opusftp_globals *og, char *path)
{
	FuncArgs *fa;
	struct site_entry *e;
	int anon, acct;
	BPTR cf;
	char *buf;
	Att_List *list = NULL;

	if (!(buf = AllocVec(TMPBUFLEN, MEMF_CLEAR)))
	{
		DisplayBeep(NULL);
		return (NULL);
	}

	if ((list = Att_NewList(LISTF_POOL)))
	{
		if ((cf = Open(path, MODE_OLDFILE)))
		{
			while (FGets(cf, buf, TMPBUFLEN))
			{
				if (*buf == '#' || *buf == '\n')
					continue;

				/* skip config entries */

				if ((fa = ParseArgs(CONFIG_TEMPLATE, buf)))
				{
					DisposeArgs(fa);
					continue;
				}

				/* handle addressbook stuff*/
				if ((fa = ParseArgs(ADDR_TEMPLATE, buf)))
				{
					anon = fa->FA_Arguments[A_OPT_ANON];
					acct = fa->FA_Arguments[A_OPT_ACCT];

					// skip any lines starting with '---'

					if (fa->FA_Arguments[A_OPT_HOST] && strncmp((char *)fa->FA_Arguments[A_OPT_HOST], "---", 3) == 0)
					{
						DisposeArgs(fa);
						continue;
					}

					if (((anon == 0) != (acct == 0))			  /* Can't be both or neither */
						&& (anon || fa->FA_Arguments[A_OPT_USER]) /* User Account must have user name */
						&& (fa->FA_Arguments[A_OPT_HOST] ||
							fa->FA_Arguments[A_OPT_ADDR]) /* Must have Hostname or Address */
					)
					{
						if ((e = AllocVec(sizeof(struct site_entry), MEMF_CLEAR)))
						{
							e->se_anon = anon;
							e->se_protocol = FTP_PROTOCOL_FTP;

							if (!anon)
							{
								if (fa->FA_Arguments[A_OPT_USER])
									stccpy(e->se_user, (char *)fa->FA_Arguments[A_OPT_USER], USERNAMELEN);

								if (fa->FA_Arguments[A_OPT_PASS])
									stccpy(e->se_pass, (char *)fa->FA_Arguments[A_OPT_PASS], PASSWORDLEN);
							}

							/* Use Hostname or Address for the socket */
							if (fa->FA_Arguments[A_OPT_HOST])
							{
								const char *host = (char *)fa->FA_Arguments[A_OPT_HOST];
								const char *without_scheme;
								int protocol;

								if (ftp_protocol_from_url_scheme(host, &without_scheme, &protocol))
								{
									e->se_protocol = protocol;
									host = without_scheme;
								}
								stccpy(e->se_host, host, HOSTNAMELEN);
							}
							else
							{
								const char *host = (char *)fa->FA_Arguments[A_OPT_ADDR];
								const char *without_scheme;
								int protocol;

								if (ftp_protocol_from_url_scheme(host, &without_scheme, &protocol))
								{
									e->se_protocol = protocol;
									host = without_scheme;
								}
								stccpy(e->se_host, host, HOSTNAMELEN);
							}

							if (fa->FA_Arguments[A_OPT_PROTOCOL])
							{
								int protocol;

								if (ftp_protocol_from_text((char *)fa->FA_Arguments[A_OPT_PROTOCOL], &protocol))
									e->se_protocol = protocol;
							}

							// special port specified ?

							if (fa->FA_Arguments[A_OPT_PORT])
								e->se_port = *(int *)fa->FA_Arguments[A_OPT_PORT];

							if (e->se_port <= 0 || e->se_port > 9999)
								e->se_port = ftp_protocol_default_port(e->se_protocol);

							/* Use Alias, Hostname, or Address in requester */
							if (fa->FA_Arguments[A_OPT_ALIS])
								stccpy(e->se_name, (char *)fa->FA_Arguments[A_OPT_ALIS], HOSTNAMELEN);
							else
								stccpy(e->se_name, e->se_host, HOSTNAMELEN);

							if (fa->FA_Arguments[A_OPT_PATH])
								stccpy(e->se_path, (char *)fa->FA_Arguments[A_OPT_PATH], PATHLEN);

							// set env to point to global default

							e->se_env = &og->og_oc.oc_env;
							e->se_has_custom_env = FALSE;

							Att_NewNode(list, e->se_name, (IPTR)e, ADDNODE_SORT);
						}
					}
					DisposeArgs(fa);
				}
			}
			Close(cf);
		}
	}

	FreeVec(buf);

	return (list);
}

/**************************************************************
 *
 *	Read the addressbook ONLY not the config. Done in setup_config
 *
 */
void read_build_addressbook(struct opusftp_globals *ogp, IPCData *ipc)
{
	D(bug("read_build_addressbook()\n"));

	if (ogp->og_SiteList)
	{
		DisplayBeep(NULL);
		return;
	}

	// read new one ok then return
	if (read_addrbook(ogp, ipc))
		return;

	// try to read old one

	if (!(ogp->og_SiteList = do_import_sites(ogp, CONFIGFILE)))
	{
		// failed?
		DisplayBeep(0);
		//	display_msg(ogp,ipc,NULL,0,GetString(locale,MSG_BADADRBOOK));
	}
}

Att_List *import_sites(struct window_params *wp, short gadgetid)
{
	struct display_globals *dg;
	char path[256];
	Att_List *sitelist = NULL;

	if (!wp)
		return (NULL);

	dg = wp->wp_dg;

	// Make window busy
	SetWindowBusy(wp->wp_win);

	if (gadgetid == MENU_FTP_IMPORT_AMFTP)
		strcpy(path, ".AmFTPProfiles");
	else
		strcpy(path, "DOpus5:System/");

	// Display file requester
	if (AslRequestTags(DATA(wp->wp_win)->request,
					   ASLFR_Window,
					   wp->wp_win,
					   ASLFR_TitleText,
					   GetString(locale, MSG_FTP_IMPORT_SITES),
					   ASLFR_InitialFile,
					   (gadgetid == MENU_FTP_IMPORT_AMFTP) ? path : "",
					   ASLFR_InitialDrawer,
					   (gadgetid == MENU_FTP_IMPORT_AMFTP) ? "" : path,
					   ASLFR_Flags1,
					   FRF_PRIVATEIDCMP,
					   TAG_END))
	{
		// Get new path
		strcpy(path, DATA(wp->wp_win)->request->fr_Drawer);
		AddPart(path, DATA(wp->wp_win)->request->fr_File, 256);

		if (gadgetid == MENU_FTP_IMPORT)
			sitelist = do_import_sites(dg->dg_og, path);
		else if (gadgetid == MENU_FTP_IMPORT_AMFTP)
			sitelist = do_import_amftp(dg, path);
	}

	// Make window unbusy
	ClearWindowBusy(wp->wp_win);

	return (sitelist);
}

void sort_list(Att_List *input_list)
{
	struct Node *node, *tmpnode, *next_node;
	struct List *list, sorted;

	list = &input_list->list;

	if (list && !IsListEmpty(list))
	{
		NewList(&sorted);

		while ((node = (struct Node *)(list->lh_Head)) && list->lh_Head->ln_Succ != NULL) /* list not empty!*/
		{
			tmpnode = node;
			while ((next_node = node->ln_Succ))
			{
				if (stricmp(tmpnode->ln_Name, node->ln_Name) > 0)
					tmpnode = node;

				node = next_node;
			}
			Remove(tmpnode);
			AddTail(&sorted, tmpnode);
		}

		while ((node = sorted.lh_Head) && sorted.lh_Head->ln_Succ != NULL) /* list not empty!*/
		{
			Remove(node);
			AddTail(list, node);
		}
	}
}

/*******************************************************
 *
 *	Initialse a site entry format to appropriate one
 *		either from default ftp env or opus default
 */

static VOID get_site_format(struct opusftp_globals *og, struct site_entry *e)
{
	ListFormat *format;

	// does entry have custom format?

	if (e->se_env->e_custom_format)
		format = &e->se_env->e_listformat;
	else
		format = get_opus_format(og);

	// copy correct format to site entry
	*(&e->se_listformat) = *format;
}

/*******************************************************
 *
 *	Allocate a blank connect msg structure
 *	Initialise the environment ptr to default
 *	Initialse the site entry format to appropriate one
 *		either from default ftp env or opus default
 */
struct connect_msg *get_blank_connectmsg(struct opusftp_globals *og)
{
	struct connect_msg *cm;

	if ((cm = AllocVec(sizeof(struct connect_msg), MEMF_CLEAR)))
	{
		cm->cm_protocol = FTP_PROTOCOL_FTP;
		cm->cm_site.se_protocol = FTP_PROTOCOL_FTP;

		// initialise pointer to default environment
		cm->cm_site.se_env = &og->og_oc.oc_env;
		cm->cm_site.se_has_custom_env = FALSE;

		// fill in the site entry format structure with appropriate format
		get_site_format(og, &cm->cm_site);

		return (cm);
	}

	return (NULL);
}

/*******************************************************
 *
 *	Allocate a blank site entry structure
 *	Initialise the environment ptr to default
 *	Initialse the site entry format to appropriate one
 *		either from default ftp env or opus default
 */
struct site_entry *get_blank_site_entry(struct opusftp_globals *og)
{
	struct site_entry *e;

	if ((e = AllocVec(sizeof(struct site_entry), MEMF_CLEAR)))
	{
		e->se_anon = TRUE;
		e->se_protocol = FTP_PROTOCOL_FTP;
		e->se_port = ftp_protocol_default_port(e->se_protocol);

		// initialise pointer to default environment
		e->se_env = &og->og_oc.oc_env;
		e->se_has_custom_env = FALSE;

		// fill in the site entry format structure with appropriate format
		get_site_format(og, e);

		return (e);
	}

	return (NULL);
}

enum {
	MATCH_NONE,
	MATCH_NAME,
	MATCH_HOST,
};

/****************************************************************
 *
 *	Find a site in the addressbook from supplied site_entry
 *	with se_name OR se_host
 *	If found fill in supplied site_entry and return TRUE
 *	Else return FALSE
 */

int get_site_entry(struct opusftp_globals *og, struct site_entry *e, IPCData *ipc)
{
	struct List *list;
	struct Node *node, *next;
	int type = MATCH_NONE;
	int result = 0;
	struct site_entry *site;

	if (!e)
		return (FALSE);

	ObtainSemaphoreShared(&og->og_SiteList_semaphore);

	if (!og->og_SiteList)
		read_build_addressbook(og, ipc);

	if (!og->og_SiteList)
		goto done;

	if (*e->se_name)
		type = MATCH_NAME;

	else if (*e->se_host)
		type = MATCH_HOST;
	else
		goto done;

	list = &og->og_SiteList->list;

	if (list && !IsListEmpty(list))
	{
		if ((node = list->lh_Head))
		{
			int count = 0;
			while ((next = node->ln_Succ))
			{
				int found;

				count++;

				site = (struct site_entry *)((Att_Node *)node)->data;

				if (type == MATCH_NAME)
					found = stricmp(e->se_name, site->se_name);
				else
					found = stricmp(e->se_host, site->se_host);

				if (found == 0)
				{
					// copy site entry
					copy_site_entry(og, e, site);

					// keep match position as return value
					result = count;
					break;
				}

				node = next;
			}
		}
	}

done:

	ReleaseSemaphore(&og->og_SiteList_semaphore);

	return (result);
}

/**************************************************************
 *
 *	Copy a site entry and update internal env ptr
 *	and make sure listFormat structure is correctly set
 *
 */
VOID copy_site_entry(struct opusftp_globals *og, struct site_entry *to, struct site_entry *from)
{
	*to = *from;

	if (!to->se_protocol)
		to->se_protocol = FTP_PROTOCOL_FTP;

	if (to->se_has_custom_env)
		to->se_env = &to->se_env_private;
	else
	{
		to->se_env = &og->og_oc.oc_env;
		*(&to->se_env_private) = *(&og->og_oc.oc_env);
	}

	// fill in the site entry format as appropriate from either default or custom env;
	// fills out &e->se_listformat
	// UNLESS we are editing an existing one in a lister then just preserve it

	if (!to->se_preserve_format)
		get_site_format(og, to);
}

/*************************** read AMFTP profiles **********************/

struct amftp_profile
{
	char amftp_name[39 + 1];
	char amftp_host[127 + 1];
	int amftp_pad1[8];
	char amftp_pass[63 + 1];
	char amftp_remote[123 + 1];
	int amftp_pad2;
	char amftp_local[119 + 1];	// Length is 124 - causes errors
	char amftp_port[4 + 1];
	char amftp_pad3[8];
	char amftp_login[63 + 1];
	// more...
};

static ULONG encrypt(ULONG *magic)
{
	ULONG d0, d1;

	d0 = *magic;

	d1 = (((((((((d0 << 3) - d0) << 3) + d0) << 1) + d0) << 4) - d0) << 1) - d0;
	d1 = (d1 + 0xe60) & 0x7fffffff;
	*magic = d1 - 1;

	return d1;
}

#define AMFTP_LEN 589

static BOOL check_amftpfile(struct display_globals *dg, char *path)
{
	BOOL ok = FALSE;
	BPTR fp;

	if ((fp = Open(path, MODE_OLDFILE)))
	{
		int size;

		Seek(fp, 0, OFFSET_END);
		size = Seek(fp, 0, OFFSET_BEGINNING);

		Close(fp);

		if (((size - 6) % AMFTP_LEN) == 0)
			ok = TRUE;
	}

	if (!ok)
	{
		ok = ftpmod_request_tags(dg->dg_og,
								 dg->dg_ipc,
								 AR_Window,
								 dg->dg_addrwp->wp_win,
								 FR_MsgNum,
								 MSG_FTP_IMPORT_BAD_AMFTPFILE,
								 FR_ButtonNum,
								 MSG_FTP_YES,
								 FR_ButtonNum,
								 MSG_FTP_NO,
								 TAG_DONE,
								 NULL);
	}
	return (ok);
}

static Att_List *do_import_amftp(struct display_globals *dg, char *path)
{
	struct opusftp_globals *og;
	Att_List *list = NULL;
	char *buf;
	og = dg->dg_og;

	if (!check_amftpfile(dg, path))
		return (NULL);

	if ((buf = AllocVec(AMFTP_LEN, MEMF_CLEAR)))
	{
		struct amftp_profile *p = (struct amftp_profile *)buf;

		if ((list = Att_NewList(LISTF_POOL)))
		{
			BPTR fp;

			if ((fp = Open(path, MODE_OLDFILE)))
			{
				WORD a;
				LONG b;

				// read intro unknown stuff
				Read(fp, &a, 2);
				Read(fp, &b, 4);

				while (AMFTP_LEN == Read(fp, buf, AMFTP_LEN))
				{
					ULONG magic = -1;
					int i;
					struct site_entry *e;

					for (i = 0; i < AMFTP_LEN; ++i)
						buf[i] ^= encrypt(&magic);

					if ((e = AllocVec(sizeof(struct site_entry), MEMF_CLEAR)))
					{
						// get main settings from amftp config

						if (*p->amftp_name)
							stccpy(e->se_name, p->amftp_name, HOSTNAMELEN);

						if (*p->amftp_host)
							stccpy(e->se_host, p->amftp_host, HOSTNAMELEN);

						e->se_protocol = FTP_PROTOCOL_FTP;

						if (buf[520])
							e->se_anon = TRUE;

						if (!e->se_anon)
						{
							if (*p->amftp_login)
								stccpy(e->se_user, p->amftp_login, USERNAMELEN);

							if (*p->amftp_pass)
								stccpy(e->se_pass, p->amftp_pass, PASSWORDLEN);
						}

						e->se_port = atoi(p->amftp_port);

						if (e->se_port <= 0 || e->se_port > 16384)
							e->se_port = ftp_protocol_default_port(e->se_protocol);

						if (*p->amftp_remote)
							stccpy(e->se_path, p->amftp_remote, PATHLEN);

						// get global default  env and mark as custom

						*(&e->se_env_private) = *(&og->og_oc.oc_env);
						e->se_env = &e->se_env_private;

						e->se_has_custom_env = TRUE;

						// Clear format marker
						e->se_env->e_custom_format = FALSE;

						// get retry and save last dir settings
						if ((e->se_env->e_retry_count = buf[521]))
							e->se_env->e_retry = TRUE;

						if (buf[522])
							e->se_env->e_keep_last_dir = TRUE;

						Att_NewNode(list, e->se_name, (IPTR)e, ADDNODE_SORT);
					}
				}
				Close(fp);
			}
		}
		FreeVec(buf);
	}

	return list;
}
