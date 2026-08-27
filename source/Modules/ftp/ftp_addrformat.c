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

/***********************************************************
 *
 *	Code to handle format requester for FTP as seperate task
 *
 *	14.3.98	GJP
 *
 ************************************************************/

#include "ftp.h"
#include "ftp_ipc.h"
#include "ftp_addressbook.h"
#include "ftp_addrformat.h"

/************ used in listerformat.module ******************/

struct formats
{
	ListFormat *def_opus_format;
	ListFormat *def_ftp_format;	 // NULL if no reset to ftp available
	ListFormat *current;
};

// Sub-process data used when launching new processes
struct format_data
{
	struct opusftp_globals *fd_ogp;
	IPCData *fd_ipc;
	IPCData *fd_main_ipc;  // Main DOPUS ipc passed to listformat.module
	IPCMessage *fd_imsg;   // from caller - if called bt IPC_CONFIGURE from a lister

	struct Window *fd_win;	// window to open on, of caller

	struct window_params *fd_wp;  // wp of calling config window

	struct formats fd_f;

	struct ftp_environment *fd_env;
};

/***********************************************************
 *
 *	Get current default lister format from Opus
 *	copy and store it in og->og_opus_format
 *
 */
ListFormat *get_opus_format(struct opusftp_globals *og)
{
	struct pointer_packet pp;
	ListFormat *opus;
	DOpusCallbackInfo *infoptr = &og->og_hooks;

	pp.type = MODPTR_DEFFORMAT;
	pp.pointer = 0;
	pp.flags = 0;

	if (DC_CALL1(infoptr, dc_GetPointer, DC_REGA0, (struct GetPointerPkt *)&pp))
	// if	(og->og_hooks.dc_GetPointer(&pp ))
	{
		opus = (ListFormat *)pp.pointer;

		*(&og->og_opus_format) = *opus;

		if (pp.flags & POINTERF_LOCKED)
			DC_CALL1(infoptr, dc_FreePointer, DC_REGA0, (struct GetPointerPkt *)&pp);
		// og->og_hooks.dc_FreePointer(&pp );
	}

	return (&og->og_opus_format);
}

// Init code for process
//#ifdef __amigaos4__
// static ULONG ASM format_init( REG(a0, IPCData *ipc), REG(a2, int skip), REG(a1, struct format_data *data ))
//#else
IPC_StartupCode(format_init, struct format_data *, data)
//#endif
{
	ULONG ok = FALSE;

	if (data)
	{
		data->fd_ipc = ipc; /* 'ipc' points to this task's tc_UserData field */

		// store back pointer so we can send this task a quit!
		if (data->fd_imsg == NULL)	// NOT called from a lister but from my main code
		{
			if (data->fd_wp)
				data->fd_wp->wp_extask_ipc = ipc;
		}
		ok = TRUE;
	}

	return (ok);
}

// Code for the spawned sub-task

static void format_code(void)
{
	// struct Library	*DOpusBase;
	struct format_data *data = 0;
	struct Library *ModuleBase = NULL;
#ifdef __amigaos4__
	struct ModuleIFace *IModule = NULL;
#endif
	BOOL result = 0;
	int function_no = 2;

	/*if	((DOpusBase = OpenLibrary( "dopus5.library", VERSION_DOPUSLIB )))
		{
		#ifdef __amigaos4__
		IDOpus = (struct DOpusIFace *)GetInterface(DOpusBase, "main", 1, NULL);
		#endif*/

	if (IPC_ProcStartup((IPTR *)&data, (APTR)&format_init))
	{
		struct window_params *wp = NULL;

		// if from our task then get semaphore for orderly abort from addressbook
		if (!data->fd_imsg)
		{
			// safety
			if ((wp = data->fd_wp))
				ObtainSemaphore(&wp->wp_extask_semaphore);
		}

		// if we default ftp format and opus NOT the same then use big requester
		if (CompareListFormat(data->fd_f.def_opus_format, data->fd_f.def_ftp_format))
			function_no = 3;

		// Get lister format module
		if ((ModuleBase = OpenLibrary("DOpus5:Modules/listerformat.module", LIB_VERSION)) &&
			GETINTERFACE(IModule, ModuleBase))
		{
			// Edit format
			result = Module_Entry((struct List *)data->fd_f.current,
								  (struct Screen *)data->fd_win,
								  data->fd_ipc,
								  data->fd_main_ipc,
								  function_no,
								  (IPTR)&data->fd_f);
#ifdef __amigaos4__
			DropInterface((struct Interface *)IModule);
#endif
			CloseLibrary(ModuleBase);
		}
		else
			CloseLibrary(ModuleBase);

		if (result == 1)  // convert no change into cancel
			result = 0;

		// do we have a calling msg?
		if (data->fd_imsg)
		{
			data->fd_imsg->command = result;
			IPC_Reply(data->fd_imsg);
		}
		else
		{
			// No msg? Then must be called from addressbook or options

			ClearWindowBusy(data->fd_win);	// unblock calling window

			// release the semaphore on the window data
			if (wp)	 // safety
			{
				// adjust flag to reflect if we have custom or default formats
				if (CompareListFormat(data->fd_f.current, data->fd_f.def_ftp_format))
					data->fd_env->e_custom_format = TRUE;
				else
					data->fd_env->e_custom_format = FALSE;

				//  Tell adr process we have changed format.
				IPC_Command(wp->wp_dg->dg_ipc, IPC_GOODBYE, result, (APTR)wp, 0, 0);

				ReleaseSemaphore(&wp->wp_extask_semaphore);
				wp->wp_extask_ipc = NULL;
			}
		}

		IPC_Free(data->fd_ipc);
		FreeVec(data);
	}
	/*#ifdef __amigaos4__
	DropInterface((struct Interface *)IDOpus);
	#endif
	CloseLibrary(DOpusBase);

	}*/
}

static BOOL do_list_format(struct opusftp_globals *og,
						   struct Window *win,
						   struct ftp_environment *env,
						   struct formats *forms,
						   IPCMessage *imsg)
{
	struct format_data *data;
	IPCData *format_ipc = NULL;
	BOOL result = 0;

	if ((data = AllocVec(sizeof(struct format_data), MEMF_CLEAR)))
	{
		data->fd_ogp = og;

		data->fd_win = win;
		data->fd_main_ipc = og->og_main_ipc;
		*(&data->fd_f) = *forms;

		data->fd_env = env;

		// imsg is valid for called from IPC_CONFIGURE
		// but NULL if called from other parts of addressbook code

		if (imsg)
			data->fd_imsg = imsg;
		else
			data->fd_wp = FINDWP(win);

		result = IPC_Launch(0,								 // List to add task to (optional, but useful)
							&format_ipc,					 // IPCData ** to store task IPC pointer in (optional)
							"dopus_ftp_format",				 // Name
							IPC_NATIVE(format_code),	 // Code
							STACK_DEFAULT,					 // Stack size
							(IPTR)data,					 // Data passed to task
							(struct Library *)DOSBase);		 // Needs pointer to dos.library

		// failed? then dump
		if (!result)
			FreeVec(data);
	}

	return (result);
}

/************************************************************
 *
 *	Return the default format
 *	either from ftp default env or opus
 */

static ListFormat *get_default_format(struct opusftp_globals *og)
{
	ListFormat *format;

	// Have we set a default ftp format?
	if (og->og_oc.oc_env.e_custom_format)
		format = &og->og_oc.oc_env.e_listformat;
	else
		format = get_opus_format(og);

	return (format);
}

/************************************************************
 *
 *	Display format requester from Options requester ONLY
 *
 */
BOOL get_listformat(struct window_params *wp)
{
	struct display_globals *dg;
	struct formats forms;
	BOOL result;
	struct ftp_environment *env;

	dg = wp->wp_dg;

	forms.def_opus_format = get_opus_format(dg->dg_og);

	// what env are we dealing with?

	if (wp->wp_type == WT_DEFOPT)
	{
		env = &dg->dg_oc.oc_env;
		// for default config default format is always Opus default
		forms.def_ftp_format = get_opus_format(dg->dg_og);
	}
	else
	{
		env = &wp->wp_se_copy->se_env_private;

		// for custom config default format can be Opus default or FTP default
		// get the default format to use according to settings in default env

		forms.def_ftp_format = get_default_format(dg->dg_og);
	}

	// if no custom format set the update private copy to use for editing

	if (!env->e_custom_format)
		*(&env->e_listformat) = *forms.def_ftp_format;

	forms.current = &env->e_listformat;

	// Make window busy
	SetWindowBusy(wp->wp_win);

	result = do_list_format(dg->dg_og, wp->wp_win, env, &forms, NULL);

	return (result);
}

/************************************************************
 *
 *	Display format requester from EXTERNAL IPC_CONFIGURE command ONLY
 *
 */

BOOL configure_format(struct display_globals *dg, struct subproc_data *data, IPCMessage *imsg)
{
	struct connect_msg *cm;
	struct formats forms;
	struct site_entry *e;
	BOOL result = FALSE;

	cm = (struct connect_msg *)imsg->data;

	if ((e = &cm->cm_site))
	{
		struct ftp_environment *env = &e->se_env_private;

		forms.def_opus_format = get_opus_format(dg->dg_og);

		forms.current = &e->se_listformat;

		forms.def_ftp_format = get_default_format(dg->dg_og);

		result = do_list_format(dg->dg_og, cm->cm_window, env, &forms, imsg);
	}
	return (result);
}
