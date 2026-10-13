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

/*
 *	17-08-95	lister process always named 'dopus_ftp_lister' no matter how its launched
 *	 1-09-95	now uses module callback function to get Opus port and screen
 *	 1-09-95	traps 'quit' command from opus when opus quits
 *	 5-09-95	Arexx commands for AddressBook, CD, Command, Connect, Delete, SetVar,
 *				and Quit replaced with module/ipc commands
 *	19-09-95	fixed potential bug which could could queue multiple abort signals
 *				fixed bug which prevented Opus from fully quitting after using ftp
 *	22-09-95	MakeDir now sends arguments
 *	24-09-95	Added LISTUPDATE config option
 *	26-09-95	drop + dropfrom were checking for exactly 4 args, now 4 or more
 *	 2-10-95	addabort() and remabort() now clear the control-D signals
 *	19-10-95	tidied up code for handle_rexx()
 *	21-02-96	Now works under AS225 as well as AmiTCP
 *	 6-03-96	No longer warns that the 'active' command is not supported
 *	19-03-96	"Get" is now asynchronous
 *	20-03-96	"List" is now also asynchronous
 *	03-05-96	Replaced the 'active command not supported' warning to help debug Opus
 *	08-05-96	Removed the 'active command not supported' warning again!
 *			FTP commands which needed a buffer supplied now just use the info->iobuf
 *				instead.
 *	20-05-96	Cleaned everything up to make maintainence easier
 *	21-05-96	Quitting the FTP now aborts all operations in progress.
 *	23-05-96	Completely erradicated global variables.
 *	24-05-96	Copy now takes the standard template "NAME,TO,QUIET/S,UPDATE/S"
 *			and supports the NAME and TO keywords, QUIET and UPDATE are ignored
 *	17-06-96	Changed template for trapping Copy command because internal template was changed
 *
 *	Jan 97		Many Changes GJP
 *			Changed so socket lib is ONLY opened when user tries to
 *			connect, not for all modules i.e. addressbook.
 *
 *	feb-may 	other stuff....
 *
 *	03-06-97	Added support for DnD to dirs using argv[5] path
 *			But ONLY for dnf ftp->local
 *
 *	05-09-98 gp	Fixed problme with leftout files for known SITE
 *			not having correct full path to the file
 *
 */

#include "ftp.h"
#include "ftp_arexx.h"
#include "ftp_module.h"
#include "ftp_ipc.h"
#include "ftp_lister.h"
#include "ftp_opusftp.h"
#include "ftp_recursive.h"
#include "ftp_util.h"
#include "ftp_addressbook.h"
#include "ftp_addrsupp_protos.h"

#define MAX_REXX_ARGS 16
#define REXX_ARGLEN 256

// extern struct Library *DOpusBase;

typedef int (*rxfuncptr)(struct opusftp_globals *, struct RexxMsg *, int argc, char *argv[]);

// Sub processes
void addressbook(void);
void lister(void);

// What state is the main process in?
enum {
	STATE_RUNNING,				 // Running normally
	STATE_START_QUITTING,		 // Just got a quit message - letting subtasks know
	STATE_START_QUITTING_FORCE,	 // Just got a quit force message - letting subtasks know
	STATE_CONTINUE_QUITTING,	 // Waiting for subtasks to quit
	STATE_DONE					 // All subtasks have quit, we can exit now
};

// Used by the main loop and its helper functions
struct main_event_data
{
	char med_opus[PORTNAMELEN + 1];	 // Opus's arexx port name
	IPCData *med_ipc;
	struct ListLock med_tasklist;
	int med_status;
	BPTR med_log_fp;
	IPCMessage *med_quitmsg;
	struct RexxMsg *med_rxquitmsg;
};

/********************************/

//
//	Check for AmiTCP or AS225
//
static int check_socketlib(struct opusftp_globals *og, IPCData *ipc)
{
	int retval = 0;
	struct Library *L_SocketBase = NULL;
#ifdef __amigaos4__
	struct SocketIFace *L_ISocket = NULL;
#endif

	// D(bug( "** %s requested\n", og->og_socketlib == NOSOCK ? "no specific library" : og->og_socketlib == AMITCPSOCK ?
	// "bsdsocket.library" : og->og_socketlib == INETSOCK ? "socket.library" : "some unknown socket library" ));

	if ((L_SocketBase = OpenLibrary("bsdsocket.library", 3)))
	{
#ifdef __amigaos4__
		if ((L_ISocket = (struct SocketIFace *)GetInterface(L_SocketBase, "main", 1, NULL)))
#endif
		{
			retval = 1;
#ifdef __amigaos4__
			DropInterface((struct Interface *)L_ISocket);
#endif
		}
		CloseLibrary(L_SocketBase);
	}

	if (!retval)
	{
		if (!og->og_noreq)
			display_msg(og, ipc, NULL, 0, GetString(locale, MSG_MAIN_NEED_TCPIP));
	}

	return retval;
}

/********************************/
/******* Generic Routines *******/
/********************************/

//
//	Get the source lister handle given a function ipc
//	(Equivalent to ARexx "lister query source")
//
IPTR handle_from_function_handle(struct opusftp_globals *og, APTR function_handle)
{
	APTR path_handle;
	IPTR handle = 0;
	DOpusCallbackInfo *infoptr = &og->og_hooks;

	if ((path_handle = DC_CALL2(infoptr, dc_GetSource, DC_REGA0, function_handle, DC_REGA1, 0)))
	{
		handle = (IPTR)DC_CALL1(infoptr, dc_GetLister, DC_REGA0, path_handle);
		DC_CALL2(infoptr, dc_EndSource, DC_REGA0, function_handle, DC_REGD0, 0);
	}
	/*if	((path_handle = og->og_hooks.dc_GetSource( function_handle, 0 )))
		{
		if	(!(handle = og->og_hooks.dc_GetLister( path_handle )))
			;

		og->og_hooks.dc_EndSource( function_handle, 0 );
		}*/

	return handle;
}

/********************************/

//
//	Find the ftp node for a handle
//
struct ftp_node *find_ftpnode(struct opusftp_globals *og, IPTR handle)
{
	struct ftp_node *n;

	if (!handle)
		return 0;

	ObtainSemaphoreShared(&og->og_listerlist.lock);

	// Scan list
	for (n = (struct ftp_node *)og->og_listerlist.list.lh_Head; n->fn_handle != handle && n->fn_node.ln_Succ;
		 n = (struct ftp_node *)n->fn_node.ln_Succ)
		;

	ReleaseSemaphore(&og->og_listerlist.lock);

	// Return node
	return n->fn_node.ln_Succ ? n : 0;
}

//	Launch a sub process (with global data pointer)
//
static IPCData *launch(struct opusftp_globals *og,
					   IPCData *ipc,
					   struct ListLock *tasklist,
					   char *name,
					   void proc_code(void),
					   int stack)
{
	struct subproc_data *data;
	IPCData *ipcd = NULL;

	if ((data = AllocVec(sizeof(struct subproc_data), MEMF_CLEAR)))
	{
		data->spd_ogp = og;
		data->spd_owner_ipc = ipc;

		if (!IPC_Launch(
				tasklist, &ipcd, name, IPC_NATIVE(proc_code), stack, (IPTR)data, (struct Library *)DOSBase))
			ipcd = NULL;
	}

	return ipcd;
}

/********************************/
/********* IPC Routines *********/
/********************************/

//
//	Sent from addressbook to change log details
//
static int ipc_updateconfig(struct opusftp_globals *og, struct main_event_data *med, LONG flags)
{
	BPTR old_fp;
	int result = FALSE;

	if (flags & ID_LOGNAME_F)
	{
		old_fp = med->med_log_fp;

		// If logfile name changed and wants log on then
		// close current and reopen
		if (og->og_log_open)
		{
			if (old_fp)
				Close(old_fp);

			if ((med->med_log_fp = Open(og->og_oc.oc_logname, MODE_NEWFILE)))
				result = TRUE;
		}

		// Else just close it. Name has been changed
		else
		{
			if (old_fp)
			{
				Close(old_fp);
				med->med_log_fp = 0;
				result = TRUE;
			}
		}
	}

	if (flags & ID_LOGON_F)
	{
		old_fp = med->med_log_fp;

		// If log should be open then open it if not open
		if (og->og_log_open)
		{
			// Log is open - leave it
			if (!old_fp)
				if ((med->med_log_fp = Open(og->og_oc.oc_logname, MODE_NEWFILE)))
					result = TRUE;
		}
		else
		{
			if (old_fp)
			{
				Close(old_fp);
				med->med_log_fp = 0;
				result = TRUE;
			}
		}
	}

	return result;
}

enum { OPT_VAR, OPT_VAL, NUM_SETVAR_OPTS };

//
//	Set an internal FTP variable
//	args will end with a LF character - guaranteed
//
//	Returns 1 if message forwarded
//	Returns 0 if message still needs replying
//
static int ipc_setvar(struct opusftp_globals *og, struct main_event_data *med, IPCMessage *msg)
{
	char *args;
	FuncArgs *fa;
	struct ftp_msg *fm = msg->data;
	int sent = 0;
	int local = 0;	// Is this a local variable?
	ULONG flags = 0;

	if ((args = fm->fm_names))
	{
		if ((fa = ParseArgs(SETVAR_TEMPLATE, args)) && fa->FA_Arguments[OPT_VAR])
		{
			// LogFile
			// Global Variable

			// SetVar LogFile LOGNAME
			if (!stricmp((char *)fa->FA_Arguments[OPT_VAR], "logfile"))
			{
				BPTR log_fp2, old_fp;

				// SetVar LogFile "newlogname"
				if (fa->FA_Arguments[OPT_VAL])
				{
					if ((log_fp2 = Open((char *)fa->FA_Arguments[OPT_VAL], MODE_NEWFILE)))
					{
						stccpy(og->og_oc.oc_logname, (char *)fa->FA_Arguments[OPT_VAL], LOGNAMELEN);
						old_fp = med->med_log_fp;
						med->med_log_fp = log_fp2;
						if (old_fp)
							Close(old_fp);
					}
				}

				// SetVar LogFile (toggle open/closed)
				else
				{
					// Log is open - close it
					if (med->med_log_fp)
					{
						old_fp = med->med_log_fp;

						// Stop log from being used
						med->med_log_fp = NULL;

						// Close it
						Close(old_fp);
					}

					// Log is closed - open it
					else if ((log_fp2 = Open(og->og_oc.oc_logname, MODE_NEWFILE)))

						// Enable log use
						med->med_log_fp = log_fp2;
				}
			}

			// Debug
			// Global Variable

			// SetVar Debug VALUE/N
			else if (!stricmp((char *)fa->FA_Arguments[OPT_VAR], "debug"))
			{
				if (fa->FA_Arguments[OPT_VAL])

					// Set new value
					og->og_oc.oc_log_debug = atoi((char *)fa->FA_Arguments[OPT_VAL]);
				else

					// Toggle on/off
					og->og_oc.oc_log_debug = og->og_oc.oc_log_debug ? 0 : 1;
			}

			// TimeOut
			// Global Variable

			// SetVar TimeOut VALUE/N
			else if (!stricmp((char *)fa->FA_Arguments[OPT_VAR], "timeout"))
			{
				// SetVar Timout <value>
				if (fa->FA_Arguments[OPT_VAL])
					og->og_oc.oc_env.e_timeout = atoi((char *)fa->FA_Arguments[OPT_VAL]);
			}

			// ListUpdate
			// Global Variable

			// SetVar ListUpdate VALUE/N
			else if (!stricmp((char *)fa->FA_Arguments[OPT_VAR], "listupdate"))
			{
				// SetVar ListUpdate <value>
				if (fa->FA_Arguments[OPT_VAL])
					og->og_oc.oc_env.e_list_update = atoi((char *)fa->FA_Arguments[OPT_VAL]);
			}

			// List
			// Local Variable

			// SetVar List COMMAND/F
			else if (!stricmp((char *)fa->FA_Arguments[OPT_VAR], "list"))
			{
				flags = SET_LISTCMD;
				local = 1;
			}

			else if (!stricmp((char *)fa->FA_Arguments[OPT_VAR], "quiet"))
			{
				flags = SET_QUIET;
				local = 1;
			}

			if (local)
			{
				// To lister - needs a handle

				// SetVar List <command>
				// SetVar Quiet

				struct ftp_node *node;
				char *cmd = NULL;

				if ((node = find_ftpnode(og, handle_from_function_handle(og, fm->fm_function_handle))))
				{
					if (fa->FA_Arguments[OPT_VAL])
					{
						if ((cmd = AllocVec(strlen((char *)fa->FA_Arguments[OPT_VAL]) + 1, MEMF_CLEAR)))
							strcpy(cmd, (char *)fa->FA_Arguments[OPT_VAL]);
					}

					if (!fa->FA_Arguments[OPT_VAL] || cmd)
					{
						// Send to lister
						msg->flags = flags;
						msg->data_free = cmd;
						ipc_forward(node->fn_ipc, msg, 0);
						sent = 1;
					}
				}
			}
		}

		if (fa)
			DisposeArgs(fa);
	}

	return sent;
}

/********************************/

//
//	Summons the address book requester
//
static IPCData *ensure_addrproc(struct opusftp_globals *og, struct main_event_data *med)
{
	if (!og->og_addrproc || (og->og_addrproc->flags & IPCF_INVALID))
	{
		og->og_addrproc = launch(og, med->med_ipc, &med->med_tasklist, "dopus_ftp_address_book", addressbook, STACK_DEFAULT);

		if (og->og_addrproc)
			IPC_Command(og->og_addrproc, IPC_HURRYUP, 0, 0, 0, REPLY_NO_PORT);
	}

	if (og->og_addrproc && !(og->og_addrproc->flags & IPCF_INVALID))
		return og->og_addrproc;

	return NULL;
}

static int ipc_addrbook(struct opusftp_globals *og, struct main_event_data *med, IPCMessage *msg)
{
	IPCData *addrproc;
	int sent = 0;

	if ((addrproc = ensure_addrproc(og, med)))
	{
		ipc_forward(addrproc, msg, 0);
		sent = 1;
	}

	return sent;
}

/********************************/

//
//	Receive an options message
//
static int ipc_options(struct opusftp_globals *og, struct main_event_data *med, IPCMessage *msg)
{
	struct ftp_node *node;
	struct connect_msg *sm;
	int need_reply = 1;
	int sent = 0;
	DOpusCallbackInfo *infoptr = &og->og_hooks;

	sm = msg->data;

	// Default options?
	if (msg->flags)
	{
		DC_CALL1(infoptr, dc_UnlockSource, DC_REGA0, sm->cm_function_handle);
		// og->og_hooks.dc_UnlockSource( sm->cm_function_handle );

		msg->flags = 0;

		if (ensure_addrproc(og, med))
		{
			need_reply = 0;
			ipc_forward(og->og_addrproc, msg, 0);
			sent = 1;
		}
	}

	// Site-specific options?
	else if ((node = find_ftpnode(og, handle_from_function_handle(og, sm->cm_function_handle))))
	{
		need_reply = 0;

		ipc_forward(node->fn_ipc, msg, 0);
		sent = 1;
	}

	if (need_reply && msg)
	{
		msg->command = FALSE;
		IPC_Reply(msg);
	}

	return sent;
}

/********************************/

static int ipc_remember_path(struct opusftp_globals *og, struct main_event_data *med, IPCMessage *msg)
{
	int sent = 0;

	if (ensure_addrproc(og, med))
	{
		Forbid();
		ipc_forward(og->og_addrproc, msg, 0);
		Permit();
		sent = 1;
	}

	return sent;
}

/********************************/

//
//	Receive a connect message (from the address book or FTPConnect command)
//	Create a new lister process and forward the connect message
//
static void ipc_append_connect_arg(char *command, int command_size, const char *arg)
{
	int len;
	int arg_len;

	if (!command || !arg || command_size <= 0)
		return;

	len = strlen(command);
	arg_len = strlen(arg);
	if (len < command_size && arg_len < command_size - len)
		strcpy(command + len, arg);
}

static void ftp_lister_quit(IPCData *ipc, IPTR flags, struct quit_msg *qm)
{
	if (!ipc || !ipc->proc)
	{
		FreeVec(qm);
		return;
	}

	Signal((struct Task *)ipc->proc, IPCSIG_QUIT);

	if (!IPC_Command(ipc, IPC_QUIT, flags, qm, 0, 0))
		FreeVec(qm);
}

static int ipc_connect(struct opusftp_globals *og, IPCData *ipc, struct ListLock *tasklist, IPCMessage *msg)
{
	struct connect_msg *cm = msg->data_free;
	struct ftp_node *node;
	char buffer[1024 + 1];
	char handlebuf[FTP_HANDLE_BUFSIZE];
	struct quit_msg *qm;
	IPCData *listerproc;
	int sent = 0;

	// Valid?
	if (cm)
	{
		// Handle of ftp lister?
		if ((node = find_ftpnode(og, cm->cm_handle)))
		{
			ftp_format_handle(handlebuf, cm->cm_handle);
			if (*cm->cm_site.se_name)
			{
				sprintf(buffer, "FTPConnect LISTER=%s SITE=\"%s\"", handlebuf, cm->cm_site.se_name);
			}
			else
			{
				sprintf(buffer, "FTPConnect LISTER=%s %s", handlebuf, cm->cm_site.se_host);
			}

			if (!*cm->cm_site.se_name && cm->cm_site.se_env)
			{
				if (cm->cm_protocol == FTP_PROTOCOL_SFTP)
					ipc_append_connect_arg(buffer, sizeof(buffer), " PROTOCOL=SFTP");
				else if (ftp_tls_mode_uses_control_tls(cm->cm_site.se_env->e_tls_mode))
				{
					ipc_append_connect_arg(buffer, sizeof(buffer), " TLS=explicit");
					ipc_append_connect_arg(
						buffer, sizeof(buffer), cm->cm_site.se_env->e_tls_verify_peer ? " TLSVERIFY" : " NOVERIFY");
				}
				else
					ipc_append_connect_arg(buffer, sizeof(buffer), " TLS=off");
			}

			if ((qm = AllocVec(sizeof(*qm) + strlen(buffer) + 1, MEMF_CLEAR)))
			{
				qm->qm_command = (char *)(qm + 1);
				strcpy(qm->qm_command, buffer);

				ftp_lister_quit(node->fn_ipc, 1, qm);
			}
			else
				DisplayBeep(og->og_screen);
		}

		// Ignore if source has a handler
		else if (handle_has_handler(cm->cm_opus, cm->cm_handle))
		{
			D(bug("** can't drag site to non-FTP handler\n"));
			DisplayBeep(og->og_screen);
		}
		else
		{
			// TCP/IP running?
			if (check_socketlib(og, ipc))
			{
				// Launch FTP lister task
				// Listers run the FTP, TLS and SFTP code, which needs more than STACK_DEFAULT
				if ((listerproc = launch(og, ipc, tasklist, "dopus_ftp_lister", lister, STACK_LARGE)))
				{
					// Send connect message
					ipc_forward(listerproc, msg, 0);
					sent = 1;
				}
			}
		}
	}

	if (!sent)
	{
		if (msg)
		{
			msg->command = FALSE;
			IPC_Reply(msg);
		}
	}

	return sent;
}

/********************************/

#if defined(__AROS__) && defined(__x86_64__)
static BPTR open_aros_debug_log(void)
{
	BPTR file;

	if ((file = Open("PROGDIR:DOpus5-ftp-debug.log", MODE_NEWFILE)))
		return file;

	return Open("T:DOpus5-ftp-debug.log", MODE_NEWFILE);
}
#endif

static void ensure_debug_log(struct opusftp_globals *og, struct main_event_data *med)
{
#if defined(__AROS__) && defined(__x86_64__)
	if (og && med && og->og_oc.oc_log_debug && !med->med_log_fp)
	{
		if ((med->med_log_fp = open_aros_debug_log()))
			og->og_log_open = TRUE;
	}
#endif
}

/********************************/

static int handle_ipc_msg(struct opusftp_globals *og, struct main_event_data *med)
{
	IPCMessage *msg;
	struct ftp_node *node;
	int more = FALSE;

	if ((msg = (IPCMessage *)GetMsg(med->med_ipc->command_port)))
	{
		more = TRUE;

		if (msg->msg.mn_Node.ln_Type == NT_REPLYMSG)
			D(bug("** main got reply msg\n"));

		switch (msg->command)
		{
		// A process has quit
		case IPC_GOODBYE:
			IPC_GetGoodbye(msg);

			// Have all processes quit?
			if (IsListLockEmpty(&med->med_tasklist))
				med->med_status = STATE_DONE;
			break;

		// Print to log file
		// We want to print the listers' goodbye messages even if we are quitting
		case IPC_PRINT:
			if (!msg->data_free)
				break;

			D(bug("LOG: %s", (char *)msg->data_free));

			ensure_debug_log(og, med);

			if (med->med_log_fp)
			{
				FWrite(med->med_log_fp, msg->data_free, strlen((char *)msg->data_free), 1);
				Flush(med->med_log_fp);
			}
			break;

		case IPC_ADDRBOOK:
			if (med->med_status == STATE_RUNNING)
			{
				if (ipc_addrbook(og, med, msg))
					msg = 0;
				else
					msg->command = FALSE;
			}
			else
				msg->command = FALSE;
			break;

		case IPC_OPTIONS:
			if (med->med_status == STATE_RUNNING)
			{
				ipc_options(og, med, msg);
				msg = 0;
			}
			else
				msg->command = FALSE;
			break;

		case IPC_REMEMBERPATH:
			if (med->med_status == STATE_RUNNING)
			{
				if (ipc_remember_path(og, med, msg))
					msg = 0;
			}
			else
				msg->command = FALSE;
			break;

		case IPC_CONNECT:
			if (med->med_status == STATE_RUNNING)
			{
				ipc_connect(og, med->med_ipc, &med->med_tasklist, msg);
				msg = 0;
			}
			else
				msg->command = FALSE;
			break;

		case IPC_FTPCOMMAND:
		case IPC_ADD:
			if (med->med_status == STATE_RUNNING)
			{
				struct ftp_msg *fm;
				IPTR handle;

				if ((fm = (struct ftp_msg *)msg->data))
				{
					if ((handle = handle_from_function_handle(og, fm->fm_function_handle)))
					{
						if ((node = find_ftpnode(og, handle)))
						{
							ipc_forward(node->fn_ipc, msg, 0);
							msg = 0;
						}
						else
							D(bug("** add/cmd node 0x%lx not found\n", (unsigned long)handle));
					}
					else
						D(bug("** add/cmd can't get handle from func handle 0x%lx\n", (unsigned long)fm->fm_function_handle));
				}
			}

			// Something went wrong?
			if (msg)
				msg->command = FALSE;
			break;

		case IPC_UPDATECONFIG:
			msg->command = ipc_updateconfig(og, med, msg->flags);
			break;

		case IPC_SETVAR:
			if (med->med_status == STATE_RUNNING)
			{
				if (ipc_setvar(og, med, msg))
					msg = 0;
				else
					msg->command = FALSE;
			}
			else
				msg->command = FALSE;
			break;

		case IPC_QUIT:
		case IPC_ABORT:
			D(bug("** ftp_main got IPC_QUIT/IPC_ABORT\n"));

			if (med->med_status == STATE_RUNNING)
			{
				med->med_quitmsg = msg;

				if (msg->flags)
					med->med_status = STATE_START_QUITTING_FORCE;
				else
					med->med_status = STATE_START_QUITTING;

				msg = NULL;
			}
			else
			{
				D(bug("** got QUIT while quitting\n"));
				if (msg->data)
					FreeVec(msg->data);
				msg->command = FALSE;
			}
			break;

		default:
			D(bug("** main ipc default: %lx\n", msg->command));
			msg->command = FALSE;
			break;
		}

		if (msg)
			IPC_Reply(msg);
	}

	return more;
}

/********************************/
/******** ARexx Routines ********/
/********************************/

//
//	Respond to files being dragged BETWEEN two of our listers
//
//	argv[1] = src handle (dest from "drop" command)
//	argv[2] = file names
//	argv[3] = dest handle (src from "drop" command)
//
static void opus_dnd_remote(struct opusftp_globals *og, int argc, char **argv)
{
	struct ftp_node *srcnode, *dstnode, *tmpnode;
	struct xfer_msg *xm;

	D(bug("opus_dnd_remote()\n"));

	if (argc >= 4 && (srcnode = find_ftpnode(og, ftp_parse_handle(argv[1]))) && (dstnode = find_ftpnode(og, ftp_parse_handle(argv[3]))))
	{
		// We are usually called from the "dropfrom" command
		// In case this changes, swap the source and destination for the "drop" command
		if (!stricmp(argv[0], "drop"))
		{
			tmpnode = srcnode;
			srcnode = dstnode;
			dstnode = tmpnode;
		}

		// Allocate message
		if ((xm = AllocVec(sizeof(struct xfer_msg) + strlen(argv[2]) + 1, MEMF_CLEAR)))
		{
			xm->xm_rm_src = srcnode;
			xm->xm_rm_dest = dstnode;
			xm->xm_names = (char *)(xm + 1);
			strcpy(xm->xm_names, argv[2]);

			IPC_Command(srcnode->fn_ipc, IPC_GETPUT, 0, 0, xm, 0);
		}
	}
}

/********************************/

/*
 *	Is a path that of a directory?
 *
 *	 1 - a directory/disk
 *	-1 - a file
 *	 0 - couldn't lock path
 */

static BOOL check_is_dir(char *name)
{
	D_S(struct FileInfoBlock, fib)
	BPTR lock;
	BOOL result = 0;

	if ((lock = Lock(name, ACCESS_READ)))
	{
		if (Examine(lock, fib))
		{
			if (fib->fib_DirEntryType > 0)
			{
				result = 1;
			}
			else
			{
				result = -1;
			}
		}
		UnLock(lock);
	}

	return result;
}

/*****************************************/
//
//	ARexx message functions
//
/*****************************************/

/*****************************************/
//
//	Standard Opus Arexx Handler events
//
/*****************************************/

//
//
//	Receive an opus 'active' from cachlist command
//
//	get active msg when our buffer is changed by Dopus
//
//		a) when Opus displays an entry from the cache
//		b) when user accesses an item from the CachList
//
//	check that the path displayed is not the same as the one in the lister
//	if so then we MUST not ask to cache for it or we get feedback loop
//
//	The "active" message now has a flag in Arg3 - if 0, it is a normal "active"
//	message, and if it is 1 it means it has come from a selected cache list
//
//	Returns 1 if ARexx message was forwarded, else 0
//
static int opus_active(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;
	struct ftp_msg *fm;
	int retval = 0;

	D(bug("opus_active(%s)\n", argv[3]));

	if (argc < 5 || !(node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
		return 0;

	// Redisplay the list if not same path
	if (atoi(argv[3]) == 1)
	{
		// Path from cachelist. Lister is already displayed
		// Do a FTP CD to this place
		if ((fm = AllocVec(sizeof(struct ftp_msg) + strlen(argv[4]) + 1, MEMF_CLEAR)))
		{
			fm->fm_rxmsg = rxmsg;
			fm->fm_names = (char *)(fm + 1);
			strcpy(fm->fm_names, argv[4]);
			IPC_Command(node->fn_ipc, IPC_CDCACHE, 0, 0, fm, 0);
			retval = 1;
		}
	}

	return retval;
}

/********************************/

//
//	Receive an opus doubleclick arexx function call
//	Send a doubleclick message to the appropriate lister process
//
//	argv[1] = handle, argv[2] = name, argv[3] = (not used)
//
static int opus_doubleclick(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;
	struct ftp_msg *fm;
	int retval = 0;

	if (argc >= 7 && (node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
	{
		if ((fm = AllocVec(sizeof(struct ftp_msg) + FILENAMELEN + 1, MEMF_CLEAR)))
		{
			fm->fm_rxmsg = rxmsg;
			fm->fm_names = (char *)(fm + 1);

			// Shift-doubleclick?
			if (strstr(argv[6], "shift"))
				fm->fm_flags = 1;

			stccpy(fm->fm_names, argv[2], FILENAMELEN + 1);
			IPC_Command(node->fn_ipc, IPC_DOUBLECLICK, 0, 0, fm, 0);
			retval = 1;
		}
	}

	return retval;
}

/********************************/

//
//	Respond to files being dragged TO our lister from another
//
//	argv[1] = dest handle
//	argv[2] = file names (or desktop icon names) - in quotes either way
//	argv[3] = source handle
//                0 if source is not a lister (icon)
//	          NULL if source is illegal (none known)
//	argv[4]
//	argv[5] = full path dropped on in our lister
//	argv[6] = which qualifier keys held down
//
//	Some icons may be appicons!
//	Only icons with absolute paths will be valid (those containing a ':')
//	Dropping a disk icon closes the FTP connection and execute the ScanDir command
//	When multiple icons are dropped, only the first is checked for being a disk
//	The first icon is the one the mouse was pointing to during the drag
//	See opus_quitcommand() and lister_disconnect() for the ScanDir stuff
//
static int opus_drop(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;			  // Lister receiving drop event
	IPTR srchandle;				  // Source lister's handle if there is one
	struct xfer_msg *xm;			  // Xfer message we will send
	ULONG flags = XFER_DROP;		  // Flags to put in Xfer message
	char firstname[FILENAMELEN + 1];  // First file in message
	struct MsgPort *mp;				  // Used for disk checking
	struct quit_msg *qm;
	int retval = 0;

	// Valid?
	if (argc < 4 || !(node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
		return 0;

	// Source is a lister?
	if ((srchandle = ftp_parse_handle(argv[3])))
	{
		// Ignore Drop if both are FTP listers
		if (find_ftpnode(og, srchandle))
			return 0;

		// Ignore if source has a handler
		if (handle_has_handler(node->fn_opus, srchandle))
		{
			DisplayBeep(og->og_screen);
			D(bug("** can't drag to FTP from other handler\n"));
			return retval;
		}

		// Drag to subdir?
		if (strstr(argv[6], "subdrop"))
			flags |= XFER_SUBDIR;
	}

	// Source is not a lister - one or more icons from desktop
	else
	{
		D(bug("** drop source not lister\n"));

		// Get first icon name
		stptok(argv[2] + 1, firstname, FILENAMELEN, "\"\r\n");
		D(bug("** drop file '%s'\n", firstname));

		// Ends in colon?
		if (*firstname && (firstname[strlen(firstname) - 1] == ':'))
		{
			D(bug("** drop file ends in colon\n"));

			// Really the name of a device?
			if ((mp = DeviceProc(firstname)))
			{
				D(bug("** drop DeviceProc ok\n"));

				// Quit commands are freed with FreeVec()
				if ((qm = AllocVec(sizeof(struct quit_msg) + strlen("ScanDir") + 1 + strlen(firstname) + 1, MEMF_ANY)))
				{
					qm->qm_rxmsg = rxmsg;
					qm->qm_command = (char *)(qm + 1);

					// Build and send quit message
					strcpy(qm->qm_command, "ScanDir ");
					strcat(qm->qm_command, firstname);
					ftp_lister_quit(node->fn_ipc, 1, qm);
					retval = 1;
				}

				return retval;
			}
		}

		D(bug("** icons dropped from desktop\n"));
		flags |= XFER_DROP_FROM_DESKTOP;
	}

	// Build and send xfer message
	if ((xm = AllocVec(sizeof(struct xfer_msg) + strlen(argv[2]) + 1 + strlen(argv[5]) + 1, MEMF_CLEAR)))
	{
		xm->xm_rxmsg = rxmsg;
		xm->xm_otherhandle = srchandle;
		xm->xm_flags = flags;
		xm->xm_names = (char *)(xm + 1);

		strcpy(xm->xm_names, argv[2]);

		xm->xm_dstpath = xm->xm_names + strlen(argv[2]) + 1;
		strcpy(xm->xm_dstpath, argv[5]);

		IPC_Command(node->fn_ipc, IPC_PUT, 0, 0, xm, 0);
		retval = 1;
	}

	return retval;
}

/********************************/

//
//	Leave entries out on the desktop
//
static void opus_leaveout(struct opusftp_globals *og, struct ftp_node *node, char *names)
{
	char command[1024 + 1];
	char desktop[PATHLEN + 1];
	char path[PATHLEN + 1];
	struct entry_info ei = {0};
	char *name;
	char *p;
	DOpusCallbackInfo *infoptr = &og->og_hooks;

	D(bug("opus_leaveout()\n"));

	DC_CALL1(infoptr, dc_GetDesktop, DC_REGA0, desktop);
	// og->og_hooks.dc_GetDesktop( desktop );

	strcpy(path, desktop);

	name = path + strlen(path);

	// Each entry to leave out
	for (p = names + 1;; p += 3)
	{
		// Next source entry name
		p = stptok(p, name, FILENAMELEN, "\"\r\n");

		if (entry_info_from_lister(node, name, &ei, 0))
		{
			// Directory?
			if (ei.ei_type >= 0)
			{
				D(bug("  directory\n"));

				strcpy(command, "FTPConnect ");

				if (node->fn_site.se_name && *node->fn_site.se_name)
				{
					sprintf(command + strlen(command),
							"SITE=\"%s\" DIR=\"%s",
							node->fn_site.se_name,
							node->fn_site.se_path);
					AddPart(command, name, 1024);
					strcat(command, "\"");
				}
				else
				{
					build_url_flags(command + strlen(command),
									node->fn_site.se_user,
									node->fn_site.se_pass,
									node->fn_site.se_host,
									node->fn_site.se_port,
									node->fn_site.se_path,
									0,
									0);

					AddPart(command, name, 1024);
				}

				D(bug("    path: '%s'\n", path));
				D(bug("    command: '%s'\n", command));

				CreateFunctionFile(path, INST_COMMAND, command, "DOpus5:Icons/FTPDirectory");
			}

			// Otherwise a file
			else
			{
				D(bug("  file\n"));

				strcpy(command, "DOpus5:ARexx/ftp_file.dopus5 ");

				// Site in address book?
				/***********************
				This ONLY works if file is in root or current dir, not if in sub dir.
				Also, if the user changes the dir in the site or details then it will not work.


							if	(node->fn_site.se_name && *node->fn_site.se_name)
								sprintf( command + strlen(command),
									"SITE \"%s\" \"%s\"",
									node->fn_site.se_name,
									name);

							// Site not in address book
							else
				*************/

				{
					strcat(command, "URL \"");

					build_url_flags(command + strlen(command),
									node->fn_site.se_user,
									node->fn_site.se_pass,
									node->fn_site.se_host,
									node->fn_site.se_port,
									node->fn_site.se_path,
									0,
									0);

					sprintf(command + strlen(command), "\" \"%s\"", name);
				}

				D(bug("    name: '%s' path '%s'\n", name, node->fn_site.se_path));
				D(bug("    path: '%s'\n", path));
				D(bug("    command: '%s'\n", command));

				CreateFunctionFile(path, INST_AREXX, command, "DOpus5:Icons/FTPFile");
			}
		}

		// Done all entries?
		if (*(p + 1) == 0)
			break;
	}

	// Make icons appear
	DC_CALL1(infoptr, dc_CheckDesktop, DC_REGA0, desktop);
	// og->og_hooks.dc_CheckDesktop( desktop );
}

/********************************/

//
//	Respond to files being dragged FROM our lister
//	Destination may be one of:
//		A standard Opus lister
//		Another FTP lister
//		A lister with some other handler
//		A disk or directory icon
//		Some other icon
//		The desktop - copy or move
//		The desktop - leave out
//
//	argv[1] = Src handle
//	argv[2] = File names
//	argv[3] = Dest handle or 0
//		  if 0 then is drop onto desktop or icon etc. Object is in argv[5]
//	argv[5] = Full path to check if DnD to DIR
//	argv[6] = Which qualifier keys held down
//
static int opus_dropfrom(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;		  // Lister that received 'dropfrom'
	IPTR desthandle;			  // Other lister if there is one
	struct xfer_msg *xm;		  // Xfer message to send
	ULONG flags = XFER_DROPFROM;  // Xfer flags
	char desktop[256 + 1];		  // Desktop path
	char *p;
	int retval = 0;	 // 1 if ARexx msg has been forwarded
	DOpusCallbackInfo *infoptr = &og->og_hooks;

	D(bug("opus_dropfrom()\n"));

	// Valid?
	if (argc < 4 || !(node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
		return 0;

	// Destination is a lister?
	if ((desthandle = ftp_parse_handle(argv[3])))
	{
		// Drag to subdir?
		if (strstr(argv[6], "subdrop"))
			flags |= XFER_SUBDIR;

		// Destination is also an FTP lister?
		if (find_ftpnode(og, desthandle))
		{
			// FTP-FTP subdirectory drag and drop not supported
			if (flags & XFER_SUBDIR)
				DisplayBeep(og->og_screen);

			// Handle FTP-FTP drag-n-drop
			else
				opus_dnd_remote(og, argc, argv);

			return retval;
		}

		// Ignore if destination has a handler
		if (handle_has_handler(node->fn_opus, desthandle))
		{
			DisplayBeep(og->og_screen);
			return retval;
		}

		// Allow 256 extra space in alloc in case of path
		if ((xm = AllocVec(sizeof(struct xfer_msg) + strlen(argv[2]) + 1 + PATHLEN + 1, MEMF_CLEAR)))
		{
			xm->xm_rxmsg = rxmsg;
			xm->xm_otherhandle = desthandle;

			p = (char *)(xm + 1);
			xm->xm_names = p;
			strcpy(p, argv[2]);

			// Full path if DnD to DIR?
			if (flags & XFER_SUBDIR)
			{
				p += strlen(p) + 1;
				xm->xm_dstpath = p;
				stccpy(p, argv[5], PATHLEN + 1);
			}

			xm->xm_flags = flags;

			IPC_Command(node->fn_ipc, IPC_GET, 0, 0, xm, 0);
			retval = 1;
		}
	}

	// Destination is not a lister
	// Must be an icon or desktop (or something incompatible)
	// We *must* find a path
	else
	{
		// Dragged somewhere incompatible? (KingCon etc)
		if (!argv[5])
		{
			DisplayBeep(og->og_screen);
			return retval;
		}

		// Dragged to desktop or path in argv[5]
		// must allocate size large enough for Jons 256 bytes of desktop path
		if ((xm = AllocVec(sizeof(struct xfer_msg) + strlen(argv[2]) + 1 + 256 + 1, MEMF_CLEAR)))
		{
			char *p;

			xm->xm_rxmsg = rxmsg;
			xm->xm_otherhandle = desthandle;

			// Point to space for names
			p = (char *)(xm + 1);
			xm->xm_names = p;
			strcpy(p, argv[2]);

			// Point to space for optional path
			p += strlen(p) + 1;
			xm->xm_dstpath = p;

			// Dragged to desktop?
			if (!stricmp(argv[5], "desktop"))
			{
				// Get desktop path and popup setting
				int result = DC_CALL1(infoptr, dc_GetDesktop, DC_REGA0, desktop);
				// int result = og->og_hooks.dc_GetDesktop( desktop );

				// Popup not disabled?
				if (result == 1 || (argv[6] && strstr(argv[6], "shift")))
				{
					// Opus version supports callback?
					if (og->og_hooks.dc_DesktopPopup)
					{
						result = DC_CALL1(infoptr, dc_DesktopPopup, DC_REGA0, 0);
						// result = og->og_hooks.dc_DesktopPopup( 0 );

						switch (result)
						{
						case 0:	 // Cancel
							result = -1;
							break;

						case 1:	 // Leave out
							result = 2;
							break;

						case 2:	 // Copy
							result = 4;
							break;

						case 3:	 // Move
							result = 3;
							break;
						}
					}
				}

				switch (result)
				{
				case 0:	 // Disabled
					flags |= XFER_DROPFROM_POPUP_DISABLED;
					break;

				case 1:	 // No default action (so do what?)
					flags |= XFER_DROPFROM_POPUP_NO_DEFAULT;
					break;

				case 2:	 // Create leftout
					/* flags |= XFER_DROPFROM_CREATE_LEFTOUT; */
					opus_leaveout(og, node, argv[2]);
					return retval;
					break;

				case 3:	 // Move to desktop
					strcpy(p, desktop);
					flags |= XFER_DROPFROM_DESKTOP_MOVE | XFER_MOVE;
					break;

				case 4:	 // Copy to desktop
					strcpy(p, desktop);
					flags |= XFER_DROPFROM_DESKTOP_COPY;
					break;

				// Some new setting?
				default:
					D(bug("** unknown desktop setting %ld\n", (long)result));
					break;
				}
			}

			// Dragged to icon
			else
			{
				D(bug("** dragged to icon\n"));

				// To directory icon?
				if (check_is_dir(argv[5]) == 1)
				{
					D(bug("** dragged to directory icon\n"));

					strcpy(p, argv[5]);
					flags |= XFER_DROPFROM_TO_ICON;
				}

				// To non-directory icon
				else
				{
					DisplayBeep(og->og_screen);
					FreeVec(xm);
					return retval;
				}
			}

			// Did we get a path?
			if (*xm->xm_dstpath)
			{
				// Send xfer message
				xm->xm_flags = flags;
				IPC_Command(node->fn_ipc, IPC_GET, 0, 0, xm, 0);
				retval = 1;
			}
			else
				FreeVec(xm);
		}
	}

	return retval;
}

/********************************/

//
//	Handles inline editing
//
//	argv[1] = src handle
//	argv[2] = entry name
//	argv[3] = 'name', 'date', 'protect', etc
//	argv[4] = new value for this field
//	argv[5] = 0
//	argv[6] = empty
//
static int opus_edit(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;
	struct edit_msg *em;

	D(bug("opus_edit()\n"));

	if (argc < 5 || !(node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
		return 0;

	if (!(em = AllocVec(sizeof(struct edit_msg) + strlen(argv[3]) + 1 + strlen(argv[2]) + 1 + strlen(argv[4]) + 1,
						MEMF_CLEAR)))
		return 0;

	em->em_rxmsg = rxmsg;
	em->em_field = (char *)(em + 1);
	em->em_entry = (char *)(em + 1) + strlen(argv[3]) + 1;
	em->em_value = (char *)(em + 1) + strlen(argv[3]) + 1 + strlen(argv[2]) + 1;

	strcpy(em->em_field, argv[3]);
	strcpy(em->em_entry, argv[2]);
	strcpy(em->em_value, argv[4]);

	IPC_Command(node->fn_ipc, IPC_EDIT, 0, 0, em, 0);

	return 1;
}

/********************************/

//
//	Receive an opus 'inactive'
// 	we get this whenever our buffer goes out of the lister
//
//	check for inactive after lister clear or quit by user
//	if lister is still there then ignore it if it is not then
//	user probably has closed the lister so tidy up.
//
//	argv[3] contains 1 if the lister is closed, 0 otherwise
//
static int opus_inactive(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;
	int retval = 0;

	D(bug("opus_inactive(%s)\n", argv[3]));

	if (argc >= 4 && (node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
	{
		// Has lister disappeared?  Ignore if lister is supposed to be invisible
		if (ftp_parse_handle(argv[3]))
		{
			ftp_lister_quit(node->fn_ipc, (IPTR)-1, NULL);

			if (rxmsg)
			{
				reply_rexx(rxmsg, 0, 0);
				retval = 1;
			}
		}
	}

	if (rxmsg && retval == 0)
	{
		reply_rexx(rxmsg, 0, 0);
		retval = 1;
	}

	return retval;
}

/********************************/

//
//	Caused by the "Parent" command, the left border parent gadget,
//	or the '/' key
//
static int opus_parent(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;
	int retval = 0;

	if ((node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
	{
		IPC_Command(node->fn_ipc, IPC_PARENT, 0, rxmsg, 0, 0);
		retval = 1;
	}

	return retval;
}

/********************************/

//
//	A new path has been entered in the path gadget
//
static int opus_path(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;
	struct quit_msg *qm;
	struct ftp_msg *fm;
	int len;
	int retval = 0;

	if ((node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
	{
		const char *url_body;
		int url_protocol;
		int url_mode;
		const char *tls_arg = "";
		const char *protocol_arg = "";

		if (ftp_protocol_from_url_scheme(argv[2], &url_body, &url_protocol))
		{
			if (url_protocol == FTP_PROTOCOL_SFTP)
				protocol_arg = " PROTOCOL=SFTP";
			else if (ftp_tls_mode_from_url_scheme(argv[2], &url_body, &url_mode))
			{
				if (ftp_tls_mode_uses_control_tls(url_mode))
					tls_arg = " TLS=explicit";
				else
					tls_arg = " TLS=off";
			}

			len = strlen("FTPConnect LISTER=") + strlen(argv[1]) + 1 + strlen(url_body) + strlen(tls_arg) +
				  strlen(protocol_arg) + 1;

			if ((qm = AllocVec(sizeof(struct quit_msg) + len, MEMF_CLEAR)))
			{
				qm->qm_rxmsg = rxmsg;
				qm->qm_command = (char *)(qm + 1);
				sprintf(qm->qm_command, "FTPConnect LISTER=%s %s%s%s", argv[1], url_body, tls_arg, protocol_arg);

				ftp_lister_quit(node->fn_ipc, 1, qm);
				retval = 1;
			}
		}
		else if ((fm = AllocVec(sizeof(struct ftp_msg) + strlen(argv[2]) + 1, MEMF_CLEAR)))
		{
			fm->fm_rxmsg = rxmsg;
			fm->fm_names = (char *)(fm + 1);
			strcpy(fm->fm_names, argv[2]);

			IPC_Command(node->fn_ipc, IPC_CD, 0, 0, fm, 0);
			retval = 1;
		}
	}

	return retval;
}

/********************************/

//
//	"Re-read Directory" is chosen from the lister popup menu
//
static int opus_reread(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;
	int retval = 0;

	if ((node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
	{
		IPC_Command(node->fn_ipc, IPC_REREAD, 0, rxmsg, 0, 0);
		retval = 1;
	}

	return retval;
}

/********************************/

//
//	Caused by the "Root" command, or the ':' key
//
static int opus_root(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;
	int retval = 0;

	if ((node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
	{
		IPC_Command(node->fn_ipc, IPC_ROOT, 0, rxmsg, 0, 0);
		retval = 1;
	}

	return retval;
}

/********************************/

//
//	From the lister menu, or pressing 'Save' on the List format editor
//
static int opus_snapshot(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;
	int retval = 0;

	if ((node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
	{
		IPC_Command(node->fn_ipc, IPC_SNAPSHOT, 0, rxmsg, 0, 0);
		retval = 1;
	}

	return retval;
}

/********************************/

//
//	From the lister menu
//
static int opus_unsnapshot(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;
	int retval = 0;

	if ((node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
	{
		IPC_Command(node->fn_ipc, IPC_UNSNAPSHOT, 0, rxmsg, 0, 0);
		retval = 1;
	}

	return retval;
}

/********************************/

//
//	Added by us to the lister [srce/dest] popup menu
//
static int popup_add(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;
	int retval = 0;

	if ((node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
	{
		IPC_Command(node->fn_ipc, IPC_ADD, 0, 0, 0, 0);
		retval = 1;
	}

	return retval;
}

/********************************/

//
//	Added by us to the lister [srce/dest] popup menu
//
static int popup_options(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;
	int retval = 0;

	if ((node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
	{
		IPC_Command(node->fn_ipc, IPC_OPTIONS, 0, 0, 0, 0);
		retval = 1;
	}

	return retval;
}

/*****************************************/
//
//	Specific trapped events
//
/*****************************************/

//
//	Receive an abort message from Opus
//
static int trap_abort(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;
	int retval = 0;

	if ((node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
	{
		// Stop high level stuff
		node->fn_flags |= LST_ABORT;

		// Stop low level stuff
		if (node->fn_ftp.fi_abortsignals)
		{
			// Signal another task, or this lister's task?
			if (node->fn_signaltask)
			{
				D(bug("*** SENDING SIGNAL (0x%08lx -> 0x%08lx) ***\n", (unsigned long)node->fn_ipc->proc, (unsigned long)node->fn_signaltask));
				Signal(node->fn_signaltask, node->fn_ftp.fi_abortsignals);
				//	retval = 1;
			}
			//	else
			{
				D(bug("*** SENDING SIGNAL (0x%08lx) ***\n", (unsigned long)node->fn_ipc->proc));
				Signal((struct Task *)node->fn_ipc->proc, node->fn_ftp.fi_abortsignals);
				retval = 1;
			}

			// Don't allow signals to queue
			//	node->fn_ftp.fi_abortsignals = 0;
		}
	}

	if (retval && rxmsg)
		reply_rexx(rxmsg, 0, 0);

	return retval;
}

/********************************/

//
//	Receive a configure message from Opus
//
static int trap_configure(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;
	struct ftp_msg *fm;
	int retval = 0;

	D(bug("trap_configure()\n"));

	// Valid?
	if (argc < 5 || !argv[1] || !(node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
		return 0;

	if ((fm = AllocVec(sizeof(*fm), MEMF_CLEAR)))
	{
		fm->fm_rxmsg = rxmsg;

		IPC_Command(node->fn_ipc, IPC_CONFIGURE, 0, 0, fm, 0);
		retval = 1;
	}

	if (!retval && rxmsg)
		reply_rexx(rxmsg, 0, 0);

	return retval;
}

/********************************/

//
//	Respond to a 'Copy' command with FTP sites as source, destination, or both
//
//	argv[1]=srchandle
//	argv[2]=files
//	argv[3]=desthandle
//	argv[4]=srcpath,
//	argv[5]=args
//	argv[6]
//	argv[7]=destpath
//

//	Copy:	NAME,TO,QUIET/S,UPDATE/S,MOVEWHENSAME/S,NEWER/S
//	CopyAs:	NAME,NEWNAME,TO,QUIET/S,MOVEWHENSAME/S
//	Move:	NAME,TO,QUIET/S
//	MoveAs:	NAME,NEWNAME,TO,QUIET/S

//	NAME:		Copy this entry instead of those selected in lister
//			Could contain path information!!
//	TO:		Copy to this path
//			Could contain path information!!
//			Always to be treated as a directory
//	QUIET:		Don't show requesters (assumes delete all and unprotect all will be pressed)
//	UPDATE:		Only copy files which don't already exist
//	MOVEWHENSAME:	If files are on the same device then move instead of copy
//	NEWER:		Only copy files which don't already exist or are newer than existing files
//	NEWNAME:	Name of the new copy (for CopyAs and MoveAs)
//			Ignored unless NAME is also specified

static int trap_copy(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *srcnode, *dstnode, *remotenode;
	char *template;
	int opt_name = 0;
	int opt_to;
	int opt_quiet;
	int opt_update = -1;
	int opt_movewhensame = -1;
	int opt_newer = -1;
	int opt_newname = -1;
	int flags = 0;
	FuncArgs *fa;
	char *args = argv[5];
	int memsize;
	char *srcpath;
	char *dstpath;
	char *newname = 0;
	char *p;
	int retval = 0;

	D(bug("trap_copy() %s %s\n", argv[0], args));

	// Workaround weird jon-ism
	if (!args || !*args)
		args = " ";

	// Valid?
	if (argc < 4 || !argv[1] || !argv[2] || !argv[3])
		return 0;

	// Setup for ReadArgs for the different commands
	if (!stricmp(argv[0], "copy"))
	{
		template = "NAME,TO,QUIET/S,UPDATE/S,MOVEWHENSAME/S,NEWER/S";
		opt_to = 1;
		opt_quiet = 2;
		opt_update = 3;
		opt_movewhensame = 4;
		opt_newer = 5;
	}
	else if (!stricmp(argv[0], "copyas"))
	{
		template = "NAME,NEWNAME,TO,QUIET/S,MOVEWHENSAME/S";
		opt_newname = 1;
		opt_to = 2;
		opt_quiet = 3;
		opt_movewhensame = 4;

		flags |= XFER_AS;
	}
	else if (!stricmp(argv[0], "move"))
	{
		template = "NAME,TO,QUIET/S";
		opt_to = 1;
		opt_quiet = 2;

		flags |= XFER_MOVE;
	}
	else
	{
		template = "NAME,NEWNAME,TO,QUIET/S";
		opt_newname = 1;
		opt_to = 2;
		opt_quiet = 3;

		flags |= (XFER_AS | XFER_MOVE);
	}

	// This template MUST be identical to the current internal command template!
	if ((fa = ParseArgs(template, args)))
	{
		// Check options
		if (opt_name != -1 && fa->FA_Arguments[opt_name])
		{
			D(bug("ftp_main: XFER_OPT_NAME\n"));
			flags |= XFER_OPT_NAME;
		}
		if (opt_to != -1 && fa->FA_Arguments[opt_to])
			flags |= XFER_OPT_TO;
		if (opt_quiet != -1 && fa->FA_Arguments[opt_quiet])
			flags |= XFER_OPT_QUIET;
		if (opt_update != -1 && fa->FA_Arguments[opt_update])
			flags |= XFER_OPT_UPDATE;
		if (opt_movewhensame != -1 && fa->FA_Arguments[opt_movewhensame])
			flags |= XFER_OPT_MOVEWHENSAME;
		if (opt_newer != -1 && fa->FA_Arguments[opt_newer])
			flags |= XFER_OPT_NEWER;
		if (opt_newname != -1 && fa->FA_Arguments[opt_newname])
			flags |= XFER_OPT_NEWNAME;

		// Which listers are FTP sites?
		srcnode = find_ftpnode(og, ftp_parse_handle(argv[1]));
		dstnode = find_ftpnode(og, ftp_parse_handle(argv[3]));

		// One end local and one end FTP?
		if ((srcnode || dstnode) && !(srcnode && dstnode))
		{
			struct xfer_msg *xm;

			remotenode = srcnode ? srcnode : dstnode;

			// Check for supplied path
			srcpath = srcnode ? (char *)fa->FA_Arguments[opt_to] : 0;
			dstpath = dstnode ? (char *)fa->FA_Arguments[opt_name] : 0;

			// If no dest lister, check for dest path from requester
			if (srcnode && !ftp_parse_handle(argv[3]) && !dstpath && argv[7] && *argv[7])
				dstpath = argv[7];

			// If filename only supplied, don't pass path
			if (dstpath && dstpath == (char *)PathPart(dstpath))
				dstpath = NULL;

			// If this is CopyAs/MoveAs, check for supplied newname
			if ((flags & XFER_AS) && fa->FA_Arguments[opt_newname])
				newname = (char *)fa->FA_Arguments[opt_newname];

			memsize = sizeof(struct xfer_msg) + strlen(argv[2]) + 1;

			if (srcpath)
				memsize += strlen(srcpath) + 1;

			if (dstpath)
				memsize += strlen(dstpath) + 1;

			if (newname)
				memsize += strlen(newname) + 1;

			if ((xm = AllocVec(memsize, MEMF_CLEAR)))
			{
				xm->xm_rxmsg = rxmsg;

				if (srcnode)
				{
					if (ftp_parse_handle(argv[3]))
					{
						xm->xm_otherhandle = ftp_parse_handle(argv[3]);
					}
					else
					{
						xm->xm_otherhandle = 0;
					}
				}
				else
				{
					xm->xm_otherhandle = ftp_parse_handle(argv[1]);
				}

				xm->xm_flags = flags;

				p = (char *)(xm + 1);
				xm->xm_names = p;

				strcpy(p, argv[2]);

				if (srcpath)
				{
					p = p + strlen(p) + 1;
					xm->xm_srcpath = p;
					strcpy(p, srcpath);
				}

				if (dstpath)
				{
					p = p + strlen(p) + 1;
					xm->xm_dstpath = p;
					strcpy(p, dstpath);
				}

				if (newname)
				{
					p = p + strlen(p) + 1;
					xm->xm_newname = p;
					strcpy(p, newname);
				}

				IPC_Command(remotenode->fn_ipc, srcnode ? IPC_GET : IPC_PUT, 0, 0, xm, 0);
				retval = 1;
			}
		}

		// Both ends FTP?
		else if (srcnode && dstnode)
		{
			struct xfer_msg *xm;

			// If this is CopyAs/MoveAs, check for supplied newname
			if ((flags & XFER_AS) && fa->FA_Arguments[opt_newname])
				newname = (char *)fa->FA_Arguments[opt_newname];

			memsize = sizeof(struct xfer_msg) + strlen(argv[2]) + 1;

			if (newname)
				memsize += strlen(newname) + 1;

			if ((xm = AllocVec(memsize, MEMF_CLEAR)))
			{
				xm->xm_rxmsg = rxmsg;
				xm->xm_rm_src = srcnode;
				xm->xm_rm_dest = dstnode;
				xm->xm_flags = flags;

				p = (char *)(xm + 1);
				xm->xm_names = p;

				strcpy(p, argv[2]);

				if (newname)
				{
					p = p + strlen(p) + 1;
					xm->xm_newname = p;
					strcpy(p, newname);
				}

				IPC_Command(srcnode->fn_ipc, IPC_GETPUT, 0, 0, xm, 0);
				retval = 1;
			}
		}

		DisposeArgs(fa);
	}

	return retval;
}

/********************************/

//
//	Respond to a 'Delete' command on an FTP lister
//
//	argv[1] = srchandle
//	argv[2] = files
//	argv[3] = desthandle
//	argv[4] = srcpath
//	argv[8] = callback handle
//

// Delete:	NAME,QUIET/S

enum { OPT_DEL_NAME, OPT_DEL_QUIET, NUM_DEL_OPTS };

static int trap_delete(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;
	struct ftp_msg *fm;
	int flags = 0;	 // Old generic flags
	int dflags = 0;	 // New delete-specific flags
	FuncArgs *fa;
	char *args = argv[5];
	int retval = 0;
	DOpusCallbackInfo *h = &og->og_hooks;
	APTR function_handle;
	APTR entry, entry2;
	int num;
	int i = 0;
	struct entry_info ei = {0};
	struct DateStamp ds = {0};
	struct TagItem tags[] = {HFFS_NAME, 0, HFFS_SIZE, 0, HFFS_DATE, 0, HFFS_PROTECTION, 0, HFFS_COMMENT, 0, TAG_DONE};

	// Workaround weird jon-ism
	if (!args || !*args)
		args = " ";

	// Valid?
	if (argc < 8 || !argv[1] || !argv[2] || !(node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
		return 0;

	// This template MUST be identical to the current internal command template!
	if ((fa = ParseArgs("NAME,QUIET/S", args)))
	{
		if (fa->FA_Arguments[OPT_DEL_NAME])
		{
			flags |= CMD_HAS_ARGS;	  // Old generic flag
			dflags |= DELE_OPT_NAME;  // New delete flag
		}
		if (fa->FA_Arguments[OPT_DEL_QUIET])
		{
			flags |= CMD_QUIET;		   // Old generic flag
			dflags |= DELE_OPT_QUIET;  // New delete flag
		}

		tags[0].ti_Data = (IPTR)ei.ei_name;
		tags[1].ti_Data = (IPTR)&ei.ei_size;
		tags[2].ti_Data = (IPTR)&ds;
		tags[3].ti_Data = (IPTR)&ei.ei_prot;
		tags[4].ti_Data = (IPTR)ei.ei_comment;

		function_handle = (APTR)ftp_parse_handle(argv[8]);

		if (DC_CALL2(h, dc_GetSource, DC_REGA0, function_handle, DC_REGA1, 0))
		// if	(h->dc_GetSource( function_handle, 0 ))
		{
			num = DC_CALL1(h, dc_EntryCount, DC_REGA0, function_handle);
			// num = h->dc_EntryCount( function_handle );

			if (num)
			{
				DC_CALL1(h, dc_FirstEntry, DC_REGA0, function_handle);
				// h->dc_FirstEntry( function_handle );

				if ((fm = AllocVec(sizeof(struct ftp_msg) + num * sizeof(struct entry_info) + strlen(argv[2]) + 1,
								   MEMF_CLEAR)))
				{
					fm->fm_rxmsg = rxmsg;
					fm->fm_names = (char *)(fm + 1);
					strcpy(fm->fm_names, argv[2]);
					fm->fm_flags = dflags;
					fm->fm_entries = (struct entry_info *)(fm->fm_names + strlen(argv[2]) + 1);

					while ((entry = DC_CALL1(h, dc_GetEntry, DC_REGA0, function_handle)))
					// while	((entry = h->dc_GetEntry( function_handle )))
					{
						ei.ei_type = DC_CALL2(h, dc_ExamineEntry, DC_REGA0, entry, DC_REGD0, EE_TYPE);
						// ei.ei_type = h->dc_ExamineEntry( entry, EE_TYPE );

						// Invisible entry?  Assume a file
						if (ei.ei_type == 0)
							ei.ei_type = -1;

						// Map callback types to ARexx types
						else if (ei.ei_type == 4)
							ei.ei_type = 3;
						else if (ei.ei_type == 2)
							ei.ei_type = 1;
						else if (ei.ei_type == -3)
							ei.ei_type = -1;
						else if (ei.ei_type == -4)
							ei.ei_type = -3;
						else
							D(bug("** ExamineEntry bad type %ld\n", (long)ei.ei_type));

						if (ei.ei_type >= 0)
							++fm->fm_dircount;
						else
							++fm->fm_filecount;

						if ((entry2 = DC_CALL1(h, dc_ConvertEntry, DC_REGA0, entry)))
							DC_CALL3(
								h, dc_FileQuery, DC_REGA0, (APTR)node->fn_handle, DC_REGA1, entry2, DC_REGA2, tags);
						else
							stccpy(ei.ei_name,
								   (char *)DC_CALL2(h, dc_ExamineEntry, DC_REGA0, entry, DC_REGD0, EE_NAME),
								   FILENAMELEN + 1);

						/*if	((entry2 = h->dc_ConvertEntry( entry )))
							h->dc_FileQuery( (ULONG)node->fn_handle, entry2, tags );
						else
							stccpy( ei.ei_name, (char *)h->dc_ExamineEntry( entry, EE_NAME ), FILENAMELEN + 1 );*/

						fm->fm_entries[i++] = ei;

						DC_CALL3(h, dc_EndEntry, DC_REGA0, function_handle, DC_REGA1, entry, DC_REGD0, 0);
						// h->dc_EndEntry( function_handle, entry, 0 );
					}

					IPC_Command(node->fn_ipc, IPC_DELETE, flags, 0, fm, 0);
					retval = 1;
				}
			}

			DC_CALL2(h, dc_EndSource, DC_REGA0, function_handle, DC_REGD0, 0);
			// h->dc_EndSource( function_handle, 0 );
		}

		DisposeArgs(fa);
	}

	return retval;
}

/********************************/

//
//	Respond to a 'FindFile' command on an FTP lister
//
//	argv[1]=handle
//	argv[2]=dirnames
//	argv[3]="0"
//	argv[4]=path
//

// FindFile:	<no template> (should have a comment switch)

static int trap_findfile(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;
	struct findfile_msg *fm;
	int retval = 0;

	// Valid?
	if (argc < 5 || !argv[1] || !(node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
		return retval;

	if ((fm = AllocVec(sizeof(struct findfile_msg) + strlen(argv[2]) + 1, MEMF_CLEAR)))
	{
		fm->fm_rxmsg = rxmsg;
		fm->fm_names = (char *)(fm + 1);
		strcpy(fm->fm_names, argv[2]);

		IPC_Command(node->fn_ipc, IPC_FINDFILE, 0, 0, fm, 0);
		retval = 1;
	}

	return retval;
}

/********************************/

//
//	Respond to a 'GetSizes' command on an FTP lister
//
//	argv[1]=handle
//	argv[2]=dirnames
//	argv[3]="0"
//	argv[4]=path
//

// GetSizes:	FORCE/S

enum { OPT_GS_FORCE, NUM_GS_OPTS };

static int trap_getsizes(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;
	struct getsizes_msg *gm;
	int gsflags = 0;  // New getsizes-specific flags
	FuncArgs *fa;
	char *args = argv[5];
	int retval = 0;

	D(bug("trap_getsizes()\n"));

	// Workaround weird jon-ism
	if (!args || !*args)
		args = " ";

	// Valid?
	if (argc < 5 || !argv[1] || !(node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
		return 0;

	// This template MUST be identical to the current internal command template!
	if ((fa = ParseArgs("FORCE/S", args)))
	{
		if (fa->FA_Arguments[OPT_GS_FORCE])
			gsflags |= GS_OPT_FORCE;

		if ((gm = AllocVec(sizeof(struct getsizes_msg) + strlen(argv[2]) + 1, MEMF_CLEAR)))
		{
			gm->gs_rxmsg = rxmsg;
			gm->gs_names = (char *)(gm + 1);
			strcpy(gm->gs_names, argv[2]);
			gm->gs_flags = gsflags;

			IPC_Command(node->fn_ipc, IPC_GETSIZES, 0, 0, gm, 0);
			retval = 1;
		}

		DisposeArgs(fa);
	}

	return retval;
}

/********************************/

//
//	Respond to a 'MakeDir' command on an FTP lister
//
//	argv[1]=handle
//	argv[2]=NULL
//	argv[3]="0"
//	argv[4]=path
//	argv[5]=args
//	argv[6]=NULL
//

// MakeDir:	NAME,NOICON/S,SELECT/S,NEW/S,READ/S

static int trap_makedir(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;
	FuncArgs *fa;
	char *args = argv[5];
	int flags = 0;
	char *name = 0;
	int len;
	struct ftp_msg *fm;
	int retval = 0;

	// Workaround weird jon-ism
	if (!args || !*args)
		args = " ";

	// Valid?
	if (argc < 5 || !argv[1] || !(node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
		return 0;

	// This template MUST be identical to the current internal command template!
	if ((fa = ParseArgs("NAME,NOICON/S,SELECT/S,NEW/S,READ/S", args)))
	{
		if (fa->FA_Arguments[0])
			name = (char *)fa->FA_Arguments[0];
		if (fa->FA_Arguments[2])
			flags |= MKDIR_SELECT;
		if (fa->FA_Arguments[3])
			flags |= MKDIR_NEW;
		if (fa->FA_Arguments[4])
			flags |= MKDIR_READ;

		len = sizeof(struct ftp_msg);

		if (name && *name)
			len += strlen(name) + 1;

		if ((fm = AllocVec(len, MEMF_CLEAR)))
		{
			fm->fm_rxmsg = rxmsg;

			if (name && *name)
			{
				fm->fm_names = (char *)(fm + 1);
				strcpy(fm->fm_names, name);
			}

			IPC_Command(node->fn_ipc, IPC_MAKEDIR, flags, 0, fm, 0);
			retval = 1;
		}

		DisposeArgs(fa);
	}

	return retval;
}

/********************************/

//
//	Respond to a 'Protect' command on an FTP lister
//
//	argv[1]=handle
//	argv[2]=names
//	argv[3]="0"
//	argv[4]=path
//

// Protect:	NAME,RECURSE/S,SET=+/K,CLEAR=-/K

enum { OPT_PROT_NAME, OPT_PROT_RECURSE, OPT_PROT_SET, OPT_PROT_CLEAR, NUM_PROT_OPTS };

static int trap_protect(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;
	struct protect_msg *pm;
	int pflags = 0;	 // New delete-specific flags
	FuncArgs *fa;
	char *args = argv[5];
	int retval = 0;
	ULONG set_mask = 0;
	ULONG clear_mask = 0;

	D(bug("trap_protect()\n"));

	// Workaround weird jon-ism
	if (!args || !*args)
		args = " ";

	// Valid?
	if (argc < 5 || !argv[1] || !(node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
		return retval;

	// This template MUST be identical to the current internal command template!
	if ((fa = ParseArgs("NAME,RECURSE/S,SET=+/K,CLEAR=-/K", args)))
	{
		if (fa->FA_Arguments[OPT_PROT_NAME])
			pflags |= PROT_OPT_NAME;

		if (fa->FA_Arguments[OPT_PROT_RECURSE])
			pflags |= PROT_OPT_RECURSE;

		if (fa->FA_Arguments[OPT_PROT_SET])
		{
			pflags |= PROT_OPT_SET;
			set_mask = prot_from_string((char *)fa->FA_Arguments[OPT_PROT_SET]);
			if ((set_mask & (FIBF_WRITE | FIBF_DELETE)) != (FIBF_WRITE | FIBF_DELETE))
				set_mask &= ~(FIBF_WRITE | FIBF_DELETE);
		}

		if (fa->FA_Arguments[OPT_PROT_CLEAR])
		{
			pflags |= PROT_OPT_CLEAR;
			clear_mask = prot_from_string((char *)fa->FA_Arguments[OPT_PROT_CLEAR]);
			if ((clear_mask & (FIBF_WRITE | FIBF_DELETE)) != (FIBF_WRITE | FIBF_DELETE))
				clear_mask &= ~(FIBF_WRITE | FIBF_DELETE);
		}

		if ((pm = AllocVec(sizeof(struct protect_msg) + strlen(argv[2]) + 1, MEMF_CLEAR)))
		{
			pm->pm_rxmsg = rxmsg;
			pm->pm_names = (char *)(pm + 1);
			strcpy(pm->pm_names, argv[2]);
			pm->pm_flags = pflags;
			pm->pm_set_mask = set_mask;
			pm->pm_clear_mask = clear_mask;

			IPC_Command(node->fn_ipc, IPC_PROTECT, 0, 0, pm, 0);
			retval = 1;
		}

		DisposeArgs(fa);
	}

	return retval;
}

/********************************/

//
//	Respond to a 'Rename' command on an FTP lister
//
//	argv[1]=handle
//	argv[2]=files
//	argv[3]=not used
//	argv[4]=path
//

// Rename:	NAME/NEWNAME

static int trap_rename(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;
	struct ftp_msg *fm;
	int retval = 0;

	if (argc >= 3 && argv[1] && argv[2] && (node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
	{
		if ((fm = AllocVec(sizeof(struct ftp_msg) + strlen(argv[2]) + 1, MEMF_CLEAR)))
		{
			fm->fm_rxmsg = rxmsg;
			fm->fm_names = (char *)(fm + 1);
			strcpy(fm->fm_names, argv[2]);
			IPC_Command(node->fn_ipc, IPC_RENAME, 0, 0, fm, 0);
			retval = 1;
		}
	}

	return retval;
}

/********************************/
//
//	Generic trapped events
//
/********************************/

//
//	Respond to a trapped command that can use a temporary file
//
//	argv[1]=srchandle
//	argv[2]=files
//	argv[3]=desthandle
//	argv[4]=srcpath
//

// Play:	NAME,WAIT=SYNC/S,QUIET/S,ICON/S
// Run:		NAME/F
// others:	NAME/F,WAIT/S

static int opus_traptemp(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;
	struct traptemp_msg *tm;
	int retval = 0;

	// Valid?
	if (argc < 2 || !argv[0] || !argv[1] || !argv[2] || !(node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
		return 0;

	if ((tm = AllocVec(sizeof(*tm) + strlen(argv[2]) + 1, MEMF_CLEAR)))
	{
		tm->tm_rxmsg = rxmsg;
		stccpy(tm->tm_command, argv[0], TRAPTEMPCMDLEN + 1);
		tm->tm_names = (char *)(tm + 1);
		strcpy(tm->tm_names, argv[2]);

		IPC_Command(node->fn_ipc, IPC_TRAPTEMP, 0, 0, tm, 0);
		retval = 1;
	}

	return retval;
}

/************************************************/
//
//	Trapped events which take over the lister
//
/************************************************/

//
//	Receive an opus  'DeviceList'
//	Tell the apropriate lister process to quit
//	Uses a slight kludge - also used by opus_drop()
//
static int opus_devicelist(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;
	BOOL args = FALSE;
	int len;
	struct quit_msg *qm;

	if (argc < 2 || !(node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
		return 0;

	if (argc >= 6 && argv[5] && *argv[5])
		args = TRUE;

	len = strlen(argv[0]) + 1;
	if (args)
		len += strlen(argv[5]) + 1;

	if ((qm = AllocVec(sizeof(struct quit_msg) + len, MEMF_CLEAR)))
	{
		qm->qm_rxmsg = rxmsg;
		qm->qm_command = (char *)(qm + 1);
		sprintf(qm->qm_command, "%s%s%s", argv[0], args ? " " : "", args ? argv[5] : "");

		ftp_lister_quit(node->fn_ipc, 1, qm);
		return 1;
	}

	ftp_lister_quit(node->fn_ipc, 0, NULL);
	return 0;
}

/********************************/

//
//	Respond to a 'ScanDir' command on an FTP lister
//	Check for our added custom switch 'FTP'
//	Tell the apropriate lister process to quit
//	Uses a slight kludge - also used by opus_drop()
//

// ScanDir:	PATH,NEW/S,MODE/K,SHOWALL/S,CONTAINER/S,FTP/S

enum { OPT_SCAN_PATH, OPT_SCAN_NEW, OPT_SCAN_MODE, OPT_SCAN_SHOWALL, OPT_SCAN_CONTAINER, OPT_SCAN_FTP, NUM_SCAN_OPTS };

static int opus_scandir(struct opusftp_globals *og, struct RexxMsg *rxmsg, int argc, char **argv)
{
	struct ftp_node *node;
	FuncArgs *fa;
	char *args = argv[5];
	int len;
	struct ftp_msg *fm;
	struct quit_msg *qm;
	int retval = 0;

	// Workaround weird jon-ism
	if (!args || !*args)
		args = " ";

	// Valid?
	if (argc < 2 || !(node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
		return 0;

	// This template MUST be identical to the current internal command template...
	// But with the switch "FTP/S" appened.
	if ((fa = ParseArgs("PATH,NEW/S,MODE/K,SHOWALL/S,CONTAINER/S,FTP/S", args)))
	{
		if (fa->FA_Arguments[OPT_SCAN_NEW])
			;
		if (fa->FA_Arguments[OPT_SCAN_MODE])
			;
		if (fa->FA_Arguments[OPT_SCAN_SHOWALL])
			;
		if (fa->FA_Arguments[OPT_SCAN_CONTAINER])
			;

		// If ScanDir has no args don't quit - just reread
		if (!fa->FA_Arguments[OPT_SCAN_PATH])
		{
			IPC_Command(node->fn_ipc, IPC_REREAD, 0, rxmsg, 0, 0);
			retval = 1;
		}
		else if (fa->FA_Arguments[OPT_SCAN_FTP])
		{
			if ((fm = AllocVec(sizeof(struct ftp_msg) + strlen((char *)fa->FA_Arguments[OPT_SCAN_PATH]) + 1,
							   MEMF_CLEAR)))
			{
				fm->fm_rxmsg = rxmsg;
				fm->fm_names = (char *)(fm + 1);
				strcpy(fm->fm_names, (char *)fa->FA_Arguments[OPT_SCAN_PATH]);
				IPC_Command(node->fn_ipc, IPC_CD, 0, 0, fm, 0);
				retval = 1;
			}
		}
		else
		{
			len = strlen(argv[0]) + (args ? strlen(argv[5]) + 2 : 1);

			if ((qm = AllocVec(sizeof(struct quit_msg) + len, MEMF_CLEAR)))
			{
				qm->qm_rxmsg = rxmsg;
				qm->qm_command = (char *)(qm + 1);
				sprintf(qm->qm_command, "%s%s%s", argv[0], args ? " " : "", args ? argv[5] : "");
				ftp_lister_quit(node->fn_ipc, 1, qm);
				retval = 1;
			}
		}

		DisposeArgs(fa);
	}

	return retval;
}

/********************************/
/********************************/

struct rexx_func_info
{
	char *rfi_name;
	rxfuncptr rfi_function;
};

static struct rexx_func_info rexx_func_table[] = {
	// Standard Opus Arexx Handler events
	"active",
	opus_active,
	"doubleclick",
	opus_doubleclick,
	"drop",
	opus_drop,
	"dropfrom",
	opus_dropfrom,
	"edit",
	opus_edit,
	"inactive",
	opus_inactive,
	"parent",
	opus_parent,
	"path",
	opus_path,
	"reread",
	opus_reread,
	"root",
	opus_root,
	"snapshot",
	opus_snapshot,
	"unsnapshot",
	opus_unsnapshot,

	// Commands added to [source/dest] lister popup
	"add",
	popup_add,
	"options",
	popup_options,

	// Specific trapped events
	"abort",
	trap_abort,
	"configure",
	trap_configure,	 // <no template>
	"copy",
	trap_copy,	// Done	// NAME,TO,QUIET/S,UPDATE/S,MOVEWHENSAME/S,NEWER/S
	"copyas",
	trap_copy,	// Done	// NAME,NEWNAME,TO,QUIET/S,MOVEWHENSAME/S
	"delete",
	trap_delete,  // Done	// NAME,QUIET/S
	"findfile",
	trap_findfile,	// Done	// <no template>
	"getsizes",
	trap_getsizes,	// Done	// FORCE/S
	"makedir",
	trap_makedir,  // Done	// NAME,NOICON/S,SELECT/S,NEW/S,READ/S
	"move",
	trap_copy,	// Done	// NAME,TO,QUIET/S
	"moveas",
	trap_copy,	// Done	// NAME,NEWNAME,TO,QUIET/S
	"protect",
	trap_protect,  // Done	// NAME,RECURSE/S,SET=+/K,CLEAR=-/K
	"rename",
	trap_rename,  // Done	// NAME,NEWNAME

	// Generic trapped events
	"ansiread",
	opus_traptemp,	// Done	// NAME/F,WAIT/S
	"hexread",
	opus_traptemp,	// NAME/F,WAIT/S
	"play",
	opus_traptemp,	// NAME,WAIT=SYNC/S,QUIET/S,ICON/S
	"print",
	opus_traptemp,	// NAME/F,WAIT/S
	"read",
	opus_traptemp,	// NAME/F,WAIT/S
	"run",
	opus_traptemp,	// NAME/F
	"show",
	opus_traptemp,	// NAME/F,WAIT/S
	"smartread",
	opus_traptemp,	// NAME/F,WAIT/S

	// Trapped events which take over the lister
	"devicelist",
	opus_devicelist,  // NEW/S,FULL/S,BRIEF/S
	"scandir",
	opus_scandir,  // PATH,NEW/S,MODE/K,SHOWALL/S,CONTAINER/S
	0};

//
//	Handles incoming ARexx messages
//
//	Each supported message has its own function
//	Functions return 0 if they didn't reply to their ARexx message
//	Functions return 1 if they did
//
static void handle_rexx(struct opusftp_globals *og, struct main_event_data *med, struct MsgPort *rexport)
{
	struct RexxMsg *rxmsg;
	int argc;
	char **argv;
	struct rexx_func_info *rfi;
	int result;

	while ((rxmsg = (struct RexxMsg *)GetMsg(rexport)))
	{
		result = 0;

		if (med->med_status == STATE_RUNNING)
		{
			if ((rxmsg->rm_Action & 0xffffff00) == RXFUNC)
			{
				argc = (rxmsg->rm_Action & 0xff);
				argv = (char **)rxmsg->rm_Args;

				// Limit args to 16
				if (argc > 16)
					argc = 16;

				if (argc < 1 || !argv || !argv[0])
				{
					reply_rexx(rxmsg, 10, 0);
					continue;
				}

				// Find function in array
				for (rfi = rexx_func_table; rfi->rfi_name; ++rfi)
					if (!stricmp(argv[0], rfi->rfi_name))
					{
						D(bug("** trapped '%s'\n", argv[0]));
						result = rfi->rfi_function(og, rxmsg, argc, argv);
						break;
					}

				if (!rfi->rfi_name)
				{
					D(bug("** didn't trap '%s'\n", argv[0]));

					// Handle quit message
					if (!stricmp(argv[0], "quit"))
					{
						med->med_status = STATE_START_QUITTING_FORCE;
						med->med_rxquitmsg = rxmsg;
						rxmsg = 0;
					}

					// Warn about unsupported command
					else if (!og->og_noreq && argc > 1 && argv[1])
					{
						struct ftp_node *node;

						if ((node = find_ftpnode(og, ftp_parse_handle(argv[1]))))
						{
							lister_request_tags(node,
												FR_IPC,
												med->med_ipc,
												FR_FormatString,
												"%s '%s' %s",
												FR_MsgNum,
												MSG_MAIN_THE,
												AR_Message,
												argv[0],
												FR_MsgNum,
												MSG_MAIN_CMD_NOT_SUPP,
												TAG_DONE);
						}
					}
				}

				// Reply only if function didn't
				if (result == 0 && rxmsg)
					reply_rexx(rxmsg, 0, 0);
			}

			// Not RXFUNC - return an error
			else
				reply_rexx(rxmsg, 10, 0);
		}

		// Not running - return an error
		else
			reply_rexx(rxmsg, 10, 0);
	}
}

/********************************/
/**** Main Process Routines *****/
/********************************/

static void ipc_setup(struct ListLock *tasklist)
{
	NewList(&tasklist->list);
	InitSemaphore(&tasklist->lock);
}

/********************************/

static void addtraps(char *opus)
{
	char *commands[] = {// These can be supported directly
						"configure",
						"copy",
						"copyas",
						"delete",
						"findfile",
						"getsizes",
						"makedir",
						"move",
						"moveas",
						"parent",
						"protect",
						"rename",
						"root",

						// These can be supported via temporary files
						"ansiread",
						"hexread",
						"play",
						"print",
						"read",
						"run",
						"show",
						"smartread",

						// These should close the FTP but leave the lister open
						"devicelist",
						"scandir",

						// These need to be suppressed
						"addicon",
						"checkfit",
						"comment",
						"datestamp",
						"diskinfo",
						"duplicate",
						"encrypt",
						"search",
						"user1",
						"user2",
						"user3",
						"user4",
						"user",

						// And these ones are a bit special :)
						"abort",
						"quit",

						NULL};
	char **p = commands;

	while (*p)
	{
		send_rexxa(opus, REXX_REPLY_NONE, "dopus addtrap %s '%s'", *p++, PORTNAME);
	}
}

/********************************/

void free_address_book(struct opusftp_globals *og)
{
	if (og->og_SiteList && AttemptSemaphore(&og->og_SiteList_semaphore))
	{
		Att_RemList(og->og_SiteList, REMLIST_FREEDATA);
		og->og_SiteList = NULL;
		ReleaseSemaphore(&og->og_SiteList_semaphore);
	}
	else if (og->og_SiteList)
	{
		D(bug("** address book not freed!\n"));
		D(bug("    because couldn't get semaphore\n"));
	}
}

/********************************/

//
//	Opus sends us a message whenever its screen is opened or closed
//	Returns 1 if we should quit becuase there are no open listers
//
static int handle_notify(struct opusftp_globals *og, struct MsgPort *nfyport)
{
	DOpusNotify *nmsg;
	int quit = 0;

	while ((nmsg = (DOpusNotify *)GetMsg(nfyport)))
	{
		if (nmsg->dn_Type & DN_OPUS_HIDE)
		{
			// Screen is not valid to use in this time
			og->og_screen = 0;

			IPC_Command(og->og_addrproc, IPC_HIDE, 0, 0, 0, 0);
		}

		else if (nmsg->dn_Type & DN_OPUS_SHOW)
		{
			// Requester code may still rely on this
			if (og->og_screen != ((struct Window *)nmsg->dn_Data)->WScreen)
				og->og_screen = ((struct Window *)nmsg->dn_Data)->WScreen;

			IPC_Command(og->og_addrproc, IPC_SHOW, 0, 0, 0, 0);
		}

		// Low memory handler
		else if (nmsg->dn_Type & DN_FLUSH_MEM)
		{
			// Quit address book
			free_address_book(og);
		}
		else
			D(bug("** notify UNEXPECTED 0x%lx\n", (unsigned long)nmsg->dn_Type));

		ReplyFreeMsg(nmsg);
	}

	return quit;
}

/********************************/

//#ifdef __amigaos4__
// static ULONG ASM dopus_ftp_init( REG(a0, IPCData *ipc), REG(a2, int skip), REG(a1, struct modlaunch_data *mldata ))
//#else
IPC_StartupCode(dopus_ftp_init, struct modlaunch_data *, mldata)
//#endif
{
	// Store IPC pointer
	mldata->mld_ftp_ipc = ipc;

	mldata->mld_okay = TRUE;

	return 1;
}

/********************************/

//
//	Send each lister its appropriate signals to abort it
//
static void ipc_list_signal(struct ListLock *listerlist, int force)
{
	struct ftp_node *node;

	ObtainSemaphoreShared(&listerlist->lock);

	for (node = (struct ftp_node *)listerlist->list.lh_Head; node->fn_node.ln_Succ;
		 node = (struct ftp_node *)node->fn_node.ln_Succ)
	{
		if (force)
			Signal((struct Task *)node->fn_ipc->proc, SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);

		else if (node->fn_ftp.fi_abortsignals)
		{
			D(bug("*** SENDING SIGNAL ***\n"));
			Signal((struct Task *)node->fn_ipc->proc, node->fn_ftp.fi_abortsignals);

			// Don't allow signals to queue
			// this mustn't work since it's commented out
			// node->fn_ftp.fi_abortsignals = 0;
		}
	}

	ReleaseSemaphore(&listerlist->lock);
}

/********************************/

//
//	Add the 'Options' and 'Add' entries to the
//
static void add_lister_popup_extensions(const char *opus)
{
	send_rexxa(opus,
			   REXX_REPLY_NONE,
			   "dopus command " PORTNAME
			   " ext Options"
			   " type lister2"
			   " private"
			   " handler");

	send_rexxa(opus,
			   REXX_REPLY_NONE,
			   "dopus command " PORTNAME
			   " ext Add"
			   " type lister2"
			   " private"
			   " handler");
}

/********************************/

//
//	Remove the 'Options' and 'Add' entries to the
//
static void remove_lister_popup_extensions(const char *opus)
{
	send_rexxa(opus, REXX_REPLY_NONE, "dopus command " PORTNAME " remove");
}

/********************************/

//
//	Create the script file if it doesn't exist.
//	Also creates the scripts directory if needed.
//
static void check_script_file(void)
{
	BPTR lock1, lock2;
	BPTR fh;

	if ((lock1 = Lock("DOpus5:System", ACCESS_READ)))
	{
		lock1 = CurrentDir(lock1);

		if (!(lock2 = Lock("Scripts", ACCESS_WRITE)))
			lock2 = CreateDir("Scripts");

		if (lock2)
		{
			lock2 = CurrentDir(lock2);

			if (!(fh = Open("ftp.scp", MODE_OLDFILE)))
			{
				if ((fh = Open("ftp.scp", MODE_NEWFILE)))
				{
					FPuts(fh, "FTP connect success,1\n");
					FPuts(fh, "FTP connect fail,1\n");
					FPuts(fh, "FTP copy success,1\n");
					FPuts(fh, "FTP copy fail,1\n");
					FPuts(fh, "FTP error,1\n");
					FPuts(fh, "FTP close connection,1\n");
				}
			}

			if (fh)
				Close(fh);

			UnLock(CurrentDir(lock2));
		}
		UnLock(CurrentDir(lock1));
	}
}

/********************************/

void dopus_ftp(void)
{
	struct main_event_data med = {0};
	struct opusftp_globals *og;
	struct Library /* *L_DOpusBase,*/ *ourbase = NULL; /* Local copy needed before we have A4 ! */
#ifdef __amigaos4__
	struct ModuleIFace *IModule = NULL;
#endif
	struct modlaunch_data *mldata;		   /* Data from the module when we are launched */
	struct MsgPort *rexport, *nfyport = 0; /* Our ARexx and Opus Notify  message ports */
	ULONG sigbits, ipcbit, rexbit, nfybit = 0, waitbits; /* Signal bits we wait on */
	APTR notify_req = 0;				   /* Opus Notify stuff */
	struct Message *msg;
	int quittry = 0;

	D(bug("dopus_ftp()\n"));

	// Initialize event loop data
	med.med_status = STATE_RUNNING;

	// Open our module so we can't be expunged
	if (!(ourbase = OpenLibrary("ftp.module", 0)))
		if (!(ourbase = OpenLibrary("PROGDIR:Modules/ftp.module", 0)))
			ourbase = OpenLibrary("DOpus5:Modules/ftp.module", 0);

	if (ourbase && GETINTERFACE(IModule, ourbase))
	{
		/*	if	((L_DOpusBase = OpenLibrary( "dopus5.library", VERSION_DOPUSLIB )))
				{
				#ifdef __amigaos4__
				IDOpus = (struct DOpusIFace *)GetInterface(L_DOpusBase, "main", 1, NULL);
				#endif
		#define DOpusBase L_DOpusBase
		*/
		// Process startup function
		if (IPC_ProcStartup((IPTR *)&mldata, (APTR)&dopus_ftp_init))
		//#undef DOpusBase
		{
			// Fix pointer to global info
			og = mldata->mld_og;

			og->og_func_callback = mldata->mld_func_callback;

			// IPC for "dopus_ftp" process
			og->og_main_ipc = mldata->mld_ftp_ipc;
			med.med_ipc = mldata->mld_ftp_ipc;

			// Module_Entry already queried the launcher for the Opus ARexx port.
			stccpy(med.med_opus, og->og_opusname, PORTNAMELEN + 1);

			// Scan configuration file
			med.med_log_fp = setup_config(og);

			// Make sure the script file has been created
			check_script_file();

			if ((rexport = CreateMsgPort()))
			{
				rexbit = 1 << rexport->mp_SigBit;

				// Make the ARexx port public so Opus can find it
				rexport->mp_Node.ln_Name = PORTNAME;
				AddPort(rexport);
				D(bug("**** OPUSFTP PORT ADDED ****\n"));
			}

			if (rexport)
			{
				if ((nfyport = CreateMsgPort()))
				{
					nfybit = 1 << nfyport->mp_SigBit;

					// Ask Opus to tell us when it will be hidden or revealed
					notify_req = AddNotifyRequest(DN_OPUS_HIDE | DN_OPUS_SHOW | DN_FLUSH_MEM, 0, nfyport);
				}

					ipc_setup(&med.med_tasklist);
					ipcbit = 1 << mldata->mld_ftp_ipc->command_port->mp_SigBit;

					// Set up info for last login
					ad_InitListLock(&og->og_listerlist, &og->og_listercount);

					// init addressbook memory semaphore
					InitSemaphore(&og->og_SiteList_semaphore);

					// Trap some internal Opus commands
					addtraps(med.med_opus);

					add_lister_popup_extensions(med.med_opus);

					// Spawn address book
					og->og_addrproc =
						launch(og, mldata->mld_ftp_ipc, &med.med_tasklist, "dopus_ftp_address_book", addressbook, STACK_DEFAULT);

					// Make sure address book has set up before continuing
					if (og->og_addrproc)
						IPC_Command(og->og_addrproc, IPC_HURRYUP, 0, 0, 0, REPLY_NO_PORT);

					// Event loop
					while (med.med_status < STATE_DONE)
					{
						waitbits = SIGBREAKF_CTRL_C | ipcbit | rexbit;
						if (notify_req)
							waitbits |= nfybit;
						sigbits = Wait(waitbits);

						// Check for Break signal
						if (sigbits & SIGBREAKF_CTRL_C && med.med_status == STATE_RUNNING)
						{
							med.med_status = STATE_START_QUITTING_FORCE;
							sigbits &= ~SIGBREAKF_CTRL_C;
						}

						// Check for IPC, ARexx, and Notify messages
						while (sigbits & (ipcbit | rexbit | ((notify_req) ? nfybit : 0)))
						{
							// Handle a single IPC message
							if (!(sigbits & ipcbit) && (SetSignal(0L, 0L) & ipcbit))
								sigbits |= ipcbit;

							if (sigbits & ipcbit)
							{
								if (!handle_ipc_msg(og, &med))
									sigbits &= ~ipcbit;
							}

							// Handle ARexx messages
							if (!(sigbits & rexbit) && (SetSignal(0L, 0L) & rexbit))
								sigbits |= rexbit;

							if (sigbits & rexbit)
							{
								handle_rexx(og, &med, rexport);
								sigbits &= ~rexbit;
							}

							// Handle Opus Notify messages
							// Can quit on low memory
							if (notify_req)
							{
								if (!(sigbits & nfybit) && (SetSignal(0L, 0L) & nfybit))
									sigbits |= nfybit;

								if (sigbits & nfybit)
								{
									if (handle_notify(og, nfyport))
										med.med_status = STATE_START_QUITTING;

									sigbits &= ~nfybit;
								}
							}
						}

						if ((quittry < 3 && med.med_status == STATE_START_QUITTING) ||
							med.med_status == STATE_START_QUITTING_FORCE || med.med_status == STATE_CONTINUE_QUITTING)
						{
							// Should only send signals if trapped ARexx quit event
							// or if FTPQuit FORCE is used
							// or if received CTRL C

							if (med.med_status == STATE_START_QUITTING_FORCE)
							{
								// Turn all requesters off for quick quit
								og->og_noreq = TRUE;

								ipc_list_signal(&og->og_listerlist, 1);
							}

							D(bug("** ABORT %ld listers...\n", (long)og->og_listercount));

							IPC_ListQuit(&med.med_tasklist, mldata->mld_ftp_ipc, 0, FALSE);

							++quittry;
							Delay(50);

							med.med_status = STATE_CONTINUE_QUITTING;
						}
					}

					if (med.med_quitmsg)
					{
						med.med_quitmsg->command = TRUE;
						IPC_Reply(med.med_quitmsg);
					}

					// Reply to arexx quit command
					if (med.med_rxquitmsg)
						reply_rexx(med.med_rxquitmsg, 0, 0);

					remove_lister_popup_extensions(med.med_opus);

					send_rexxa(med.med_opus, REXX_REPLY_NONE, "dopus remtrap '*' '%s'", PORTNAME);

					if (notify_req)
						RemoveNotifyRequest(notify_req);
			}

			// Remove, flush and delete ARexx port
			if (rexport)
			{
				RemPort(rexport);
				D(bug("**** OPUSFTP PORT REMOVED ****\n"));
				flush_arexxport(rexport);
				DeleteMsgPort(rexport);
			}

			// Flush and delete notify port
			if (nfyport)
			{
				while ((msg = GetMsg(nfyport)))
					ReplyFreeMsg(msg);

				DeleteMsgPort(nfyport);
			}

			cleanup_config(med.med_log_fp);

			IPC_Flush(mldata->mld_ftp_ipc);
			IPC_Free(mldata->mld_ftp_ipc);
			FreeVec(mldata);

			// so we do not re-start prog
			// og_ftp_launched is initialised by mod_init and
			// cleared when process quits mod_quit
			og->og_ftp_launched = FALSE;
		}

		// Don't close via the global pointer
		// because A4 might not be valid !!
		/*if (L_DOpusBase) {
			#ifdef __amigaos4__
			DropInterface((struct Interface *)IDOpus);
			#endif
			CloseLibrary( L_DOpusBase );
			}
		}*/
#ifdef __amigaos4__
		DropInterface((struct Interface *)IModule);
#endif
		CloseLibrary(ourbase);
	}
	else
		CloseLibrary(ourbase);

	D(bug("dopus_ftp() returning\n"));
}

/********************************/
/********************************/
/********************************/
