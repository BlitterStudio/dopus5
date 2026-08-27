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

#include "dopuslib.h"
#include "clipboard.h"

#define LAUNCH_TIMEOUT 6

#define IPC_LAUNCH 1000
#define IPC_LOGIN 1001

#define LAUNCH_WB 0
#define LAUNCH_DOS 1

#define DOSLIST_TIMER 10

#define ID_BUSY 0x42555359

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct
{
	char *name;					// Command to launch
	struct Screen *errors;		// Screen for error requesters
	BPTR cd;					// Current directory
	BPTR in;					// Input file handle
	BPTR out;					// Output file handle
	struct LibData *data;		// Library data pointer
	IPCMessage *exit_reply;		// Message to reply to on exit
	short wait;					// Wait for return?
	short type;					// Type of launch
	long stack;					// Stack size
	char *default_tool;			// Default tool
	struct Process **proc_ptr;	// Returns process pointer to the user
	IPCData *notify_ipc;		// User signalled when process quits
	ULONG flags;				// Flags
} LaunchPacket;

typedef struct
{
	struct Node node;
	char name[40];
	IPCMessage *exit_reply;
	IPCData *notify_ipc;
	struct Process *proc;
	TimerHandle *timeout;
	long flags;
	struct WBStartup startup;
	char toolwindow[256];
	struct WBArg args[1];
} LaunchProc;

typedef struct
{
	struct MinNode node;
	struct Window *window;
	LaunchProc *proc;
} ErrorNode;

#define LAUNCHF_LAUNCHOK (1 << 0)

typedef struct
{
	BPTR next;
	BPTR lock;
} PathListEntry;

struct LoginPkt
{
	struct Window *window;
	char *name;
	char *password;
};

#ifndef __amigaos3__
	#pragma pack()
#endif

BOOL LIBFUNC L_WB_LaunchNew(REG(a0, char *), REG(a1, struct Screen *), REG(d0, short), REG(d1, long), REG(a2, char *));
BOOL LIBFUNC L_WB_LaunchNotify(REG(a0, char *),
							   REG(a1, struct Screen *),
							   REG(d0, short),
							   REG(d1, long),
							   REG(a2, char *),
							   REG(a3, struct Process **),
							   REG(a4, IPCData *),
							   REG(d2, ULONG));

BOOL LIBFUNC L_WB_Launch(REG(a0, char *name), REG(a1, struct Screen *errors), REG(d0, short wait))
{
	return L_WB_LaunchNotify(name, errors, wait, STACK_DEFAULT, 0, 0, 0, 0);
}

BOOL LIBFUNC L_WB_LaunchNew(REG(a0, char *name),
							REG(a1, struct Screen *errors),
							REG(d0, short wait),
							REG(d1, long stack),
							REG(a2, char *default_tool))
{
	return L_WB_LaunchNotify(name, errors, wait, stack, default_tool, 0, 0, 0);
}

BOOL LIBFUNC L_WB_LaunchNotify(REG(a0, char *name),
							   REG(a1, struct Screen *errors),
							   REG(d0, short wait),
							   REG(d1, long stack),
							   REG(a2, char *default_tool),
							   REG(a3, struct Process **proc_ptr),
							   REG(a4, IPCData *notify_ipc),
							   REG(d2, ULONG flags))
{
	LaunchPacket *packet;
	BOOL result;
	short len;

	// Calculate size
	len = sizeof(LaunchPacket) + strlen(name) + 1;
	if (default_tool)
		len += strlen(default_tool) + 1;

	// Allocate launch packet
	if (!(packet = AllocVec(len, MEMF_CLEAR)))
		return 0;

	// Fill out launch packet
	packet->name = (char *)(packet + 1);
	packet->errors = errors;
	strcpy(packet->name, name);
	packet->cd = Lock("", ACCESS_READ);
	packet->wait = (wait > 0) ? wait : 0;
	packet->stack = stack;
	packet->proc_ptr = proc_ptr;
	packet->notify_ipc = notify_ipc;
	packet->flags = flags;

	// Default tool
	if (default_tool)
	{
		packet->default_tool = packet->name + strlen(name) + 1;
		strcpy(packet->default_tool, default_tool);
	}

	// Send command to launch process
	result = (BOOL)L_IPC_Command(launcher_ipc, IPC_LAUNCH, 0, packet, 0, (wait) ? REPLY_NO_PORT_IPC : 0);

	return result;
}

BOOL LIBFUNC L_CLI_Launch(REG(a0, char *name),
						  REG(a1, struct Screen *errors),
						  REG(d0, BPTR currentdir),
						  REG(d1, BPTR input),
						  REG(d2, BPTR output),
						  REG(d3, short wait),
						  REG(d4, long stack))
{
	LaunchPacket *packet;
	BOOL result;

	// Use stack parameter?
	if (wait & LAUNCHF_USE_STACK)
	{
		// Use stack, clear flag
		wait &= ~LAUNCHF_USE_STACK;
	}

	// Use default stack
	else
		stack = STACK_DEFAULT;

	// Allocate launch packet
	if (!(packet = AllocVec(sizeof(LaunchPacket) + strlen(name) + 1, MEMF_CLEAR)))
	{
		// Free things
		UnLock(currentdir);
		if (input)
			Close(input);
		if (output)
			Close(output);
		return 0;
	}

	// Fill out launch packet
	packet->name = (char *)(packet + 1);
	packet->errors = errors;
	strcpy(packet->name, name);
	packet->cd = (currentdir) ? currentdir : Lock("", ACCESS_READ);
	packet->in = input;
	packet->out = output;
	packet->wait = (wait > 0) ? wait : 0;
	packet->type = LAUNCH_DOS;
	packet->stack = stack;

	// Send command to launch process
	result = (BOOL)L_IPC_Command(launcher_ipc, IPC_LAUNCH, 0, packet, 0, (wait) ? REPLY_NO_PORT_IPC : 0);

	return result;
}

void LIBFUNC L_MUFSLogin(REG(a0, struct Window *window), REG(a1, char *name), REG(a2, char *password))
{
	struct LoginPkt *pkt;
	long size;

	// Get packet size
	size = sizeof(struct LoginPkt);
	if (name)
		size += strlen(name) + 1;
	if (password)
		size += strlen(password) + 1;

	// Fill out packet
	if ((pkt = AllocVec(size, MEMF_CLEAR)))
	{
		// Fill out
		pkt->window = window;
		if (name)
		{
			pkt->name = (char *)(pkt + 1);
			strcpy(pkt->name, name);
		}
		if (password)
		{
			pkt->password = (name) ? (pkt->name + strlen(name) + 1) : (char *)(pkt + 1);
			strcpy(pkt->password, password);
		}

		// Send command to launch process
		L_IPC_Command(launcher_ipc, IPC_LOGIN, 0, 0, pkt, 0);
	}
}

/*#define DOSBase			(data->dos_base)
#define IconBase		(data->icon_base)*/
#define DOpusBase (data->dopus_base)
/*#define IntuitionBase	(data->int_base)*/

LaunchProc *launcher_launch(struct LibData *, LaunchPacket *packet, struct MinList *, struct MsgPort *);
void launch_cleanup(struct LibData *, LaunchProc *);
void launch_setarg(LaunchProc *, short, BPTR, char *);
char *launcher_parse(char *, char *, short);
BPTR launcher_get_parent(struct LibData *, char *);
void free_launch_packet(struct LibData *data, LaunchPacket *packet);
static ErrorNode *launch_error(struct LibData *data, LaunchPacket *packet, short msg, short buttons, APTR args);

#define v_launch_error(data, packet, msg, buttons, ...)          \
	({                                                           \
		IPTR __args[] = {DOPUS_VARIADIC_IPTR(__VA_ARGS__)};      \
		launch_error(data, packet, msg, buttons, (APTR)&__args); \
	})

#ifdef FAKEWB
BOOL install_fake_workbench(struct LibData *);
void remove_fake_workbench(struct LibData *);
void SAVEDS fake_workbench(void);
#endif

#define DLGF_FORCE (1 << 0)
#define DLGF_NOTIFY (1 << 1)

BOOL doslist_get(struct LibData *, struct MinList *, APTR, ULONG);
void doslist_free(struct LibData *, struct MinList *);
BOOL doslist_check_double(struct LibData *, struct List *, char *);

void do_mufs_logout(struct LoginPkt *);

// Launch exit code
ENTRANCE_2(static void ASM, launch_exit_code, d0, LONG, return_code, d1, LaunchPacket *, packet)
{
	struct LibData *data;
	char *name = packet->name;

	// Get library data pointer
	data = packet->data;

	// Message to reply to?
	if (packet->exit_reply)
		IPC_Reply(packet->exit_reply);

	// Free launch packet
	free_launch_packet(data, packet);

	// Lock launch list
	GetSemaphore(&data->launch_lock, SEMF_EXCLUSIVE, 0);

	// Decrement launch count, check for zero count
	if (--data->launch_count == 0)
	{
		// Decrement library open count
		--DOpusBase->libBase.lib_OpenCnt;
#warning D(bug) printf there cause crash on os3 build (and on real, and on emulation on os3/mos). TODO: invistigate and make a proper one
#ifndef __amigaos3__
		D(bug("lib_OpenCnt decreased to %d by '%s'\n", DOpusBase->libBase.lib_OpenCnt, name));
#endif

		// Signal to check for pending quit
		Signal((struct Task *)data->launch_proc->proc, IPCSIG_QUIT);
	}

	// Unlock launch list
	FreeSemaphore(&data->launch_lock);
}
ENTRANCE_END

void check_app_list(struct LibData *data, BOOL purge)
{
	AppEntry *entry, *next;
	short count = 0;

	// Lock AppEntry list
	GetSemaphore(&data->wb_data.patch_lock, SEMF_EXCLUSIVE, 0);

	// Go through free list
	for (entry = (AppEntry *)data->wb_data.rem_app_list.mlh_Head; entry->node.mln_Succ; entry = next)
	{
		// Get next entry
		next = (AppEntry *)entry->node.mln_Succ;

		// Has this entry's time expired?
		if (++entry->menu_id_base == 5 || purge)
		{
			free_app_entry(entry, &data->wb_data);
			++count;
		}
	}

	// Is the list empty now?
	if (count > 0 && IsListEmpty((struct List *)&data->wb_data.app_list) &&
		IsListEmpty((struct List *)&data->wb_data.rem_app_list))
	{
		// Decrement library open count so we can get expunged
		--DOpusBase->libBase.lib_OpenCnt;
#warning D(bug) printf there cause crash on os3 build (and on real, and on emulation on os3/mos). TODO: invistigate and make a proper one
#ifndef __amigaos3__
		D(bug("lib_OpenCnt decreased to %d\n", DOpusBase->libBase.lib_OpenCnt));
#endif
	}

	// Unlock AppEntry list
	FreeSemaphore(&data->wb_data.patch_lock);
}

void SAVEDS launcher_proc(void)
{
	IPCData *ipc;
	struct LibData *data;
	IPCMessage *msg, *quit_msg = 0;
	struct WBStartup *startup;
	struct MsgPort *reply_port;
	IPCMessage *pending_quit = 0;
	struct TimerHandle *timer, *secondtimer = 0;

	// Do startup
	if (!(ipc = L_IPC_ProcStartup((IPTR *)&data, 0)))
		return;

#ifdef FAKEWB
	// Install fake workbench task
	install_fake_workbench(data);
#endif

	// Set up timers
	if ((timer = AllocTimer(UNIT_VBLANK, 0)))
	{
		StartTimer(timer, DOSLIST_TIMER, 0);
		if ((secondtimer = AllocTimer(UNIT_VBLANK, timer->port)))
			StartTimer(secondtimer, 1, 0);
	}

	// Store process pointer
	data->launch_proc = ipc;

	// Create reply port (better not fail!)
	reply_port = CreateMsgPort();

	// Copy environment variables
	CopyLocalEnv((struct Library *)DOSBase);

	// Set program name
	SetProgramName(ipc->proc->pr_Task.tc_Node.ln_Name);

	// We need to initialise task count
	Forbid();
	data->task_count = L_Att_NodeCount((Att_List *)&((struct ExecBase *)SysBase)->TaskReady);
	data->task_count += L_Att_NodeCount((Att_List *)&((struct ExecBase *)SysBase)->TaskWait);

	// Install patches
	if (!(data->flags & LIBDF_NO_PATCHES))
		WB_Install_Patch();
	Permit();

	// Get a dos list copy
	doslist_get(data, &data->dos_list, data->dos_list_memory, DLGF_FORCE);

	// Allocate a signal bit
	data->low_mem_signal = AllocSignal(-1);

	// Event loop
	FOREVER
	{
		LaunchProc *proc;
		ULONG waitres;

		// Timer?
		if (timer)
		{
			// DOS list timer?
			if (CheckTimer(timer))
			{
				// Update dos list (check for diskchange)
				if (!(doslist_get(data, &data->dos_list, data->dos_list_memory, DLGF_NOTIFY)))
				{
					// Failed, so we try again soon
					StartTimer(timer, 1, 0);
				}

				// Restart timer
				else
					StartTimer(timer, DOSLIST_TIMER, 0);
			}

			// Second timer
			if (CheckTimer(secondtimer))
			{
				check_app_list(data, FALSE);

				// Restart timer
				StartTimer(secondtimer, 1, 0);
			}
		}

		// Any error requesters open?
		// This will need to be protected if it gets taken out of the launcher
		if (!(IsListEmpty((struct List *)&data->error_list)))
		{
			ErrorNode *error, *next;

			// Go through error requesters
			for (error = (ErrorNode *)data->error_list.mlh_Head; error->node.mln_Succ; error = next)
			{
				short ret;

				// Cache next node
				next = (ErrorNode *)error->node.mln_Succ;

				// Process messages from error requester
				if ((ret = SysReqHandler(error->window, 0, FALSE)) >= 0)
				{
					// Requester has to go away
					FreeSysRequest(error->window);
					Remove((struct Node *)error);

					// Error is from a launch process waiting?
					if (error->proc)
					{
						// Lock launch list
						GetSemaphore(&data->launch_lock, SEMF_EXCLUSIVE, 0);

						// Find process in the launch list
						for (proc = (LaunchProc *)data->launch_list.mlh_Head; proc->node.ln_Succ;
							 proc = (LaunchProc *)proc->node.ln_Succ)
						{
							// Found it?
							if (proc == error->proc)
								break;
						}

						// Got process?
						if (proc)
						{
							// Wait some more?
							if (ret == 1)
							{
								// Restart wait timer
								if (proc->timeout)
									StartTimer(proc->timeout, LAUNCH_TIMEOUT, 0);
							}

							// Don't wait any more
							else
							{
								// Message to reply to?
								if (proc->exit_reply)
								{
									free_launch_packet(data, (LaunchPacket *)proc->exit_reply->data);
									IPC_Reply(proc->exit_reply);
								}

								// Clear reply field
								proc->exit_reply = 0;
							}
						}

						// Unlock launch list
						FreeSemaphore(&data->launch_lock);
					}

					// Free error node
					FreeVec(error);
				}
			}
		}

		// Lock launch list
		GetSemaphore(&data->launch_lock, SEMF_EXCLUSIVE, 0);

		// Any returned startup messages?
		while ((startup = (struct WBStartup *)GetMsg(reply_port)))
		{
			// Find process in the launch list
			for (proc = (LaunchProc *)data->launch_list.mlh_Head; proc->node.ln_Succ;
				 proc = (LaunchProc *)proc->node.ln_Succ)
			{
				// Does this match?
				if (startup == &proc->startup)
				{
					char *procname = proc->name;

					// Remove from the list
					Remove((struct Node *)proc);

					// Message to reply to?
					if (proc->exit_reply)
					{
						ErrorNode *error, *next;

						// Reply to exit message
						free_launch_packet(data, (LaunchPacket *)proc->exit_reply->data);
						IPC_Reply(proc->exit_reply);

						// Go through error requesters
						for (error = (ErrorNode *)data->error_list.mlh_Head; error->node.mln_Succ; error = next)
						{
							// Cache next node
							next = (ErrorNode *)error->node.mln_Succ;

							// Is error from this launch?
							if (error->proc == proc)
							{
								// Requester has to go away
								FreeSysRequest(error->window);
								Remove((struct Node *)error);

								// Free error node
								FreeVec(error);
							}
						}
					}

					// Message port to notify?
					if (proc->notify_ipc)
					{
						// Send message
						IPC_Command(proc->notify_ipc, IPC_GOT_GOODBYE, 0, proc->proc, 0, 0);
					}

					// Do process cleanup
					launch_cleanup(data, proc);

					// Decrement count and break out
					--data->launch_count;

					// Zero count? Decrement library open count
					if (data->launch_count == 0)
					{
						--DOpusBase->libBase.lib_OpenCnt;
#warning D(bug) printf there cause crash on os3 build (and on real, and on emulation on os3/mos). TODO: invistigate and make a proper one
#ifndef __amigaos3__
						D(bug("lib_OpenCnt decreased to %d by '%s'\n", DOpusBase->libBase.lib_OpenCnt, procname));
#endif
					}
					break;
				}
			}
		}

		// Go through the launch list
		for (proc = (LaunchProc *)data->launch_list.mlh_Head; proc->node.ln_Succ;
			 proc = (LaunchProc *)proc->node.ln_Succ)
		{
			// Timeout timer completed?
			if (proc->timeout && CheckTimer(proc->timeout))
			{
				ErrorNode *error;

				// Show error
				if ((error = v_launch_error(data, 0, MSG_SICK_OF_WAITING, MSG_WAIT_CANCEL, (IPTR)proc->name)))
				{
					// Store launch proc in error node
					error->proc = proc;
				}
			}
		}

		// Unlock launch list
		FreeSemaphore(&data->launch_lock);

		// Quit?
		if (quit_msg)
			break;

		// IPC messages?
		while ((msg = (IPCMessage *)GetMsg(ipc->command_port)))
		{
			// Quit?
			if (msg->command == IPC_QUIT)
			{
				// Any outstanding messages?
				if (data->launch_count > 0)
				{
					// Make pending quit
					if (!pending_quit)
						pending_quit = msg;
					msg = 0;
				}

				// Otherwise, quit immediately
				else
				{
					quit_msg = msg;
					msg = 0;
				}
			}

			// Launch something
			else if (msg->command == IPC_LAUNCH)
			{
				BPTR old;
				LaunchPacket *packet;
				LaunchProc *proc;

				// Get launch packet
				packet = (LaunchPacket *)msg->data;

				// Save library pointer
				packet->data = data;

				// Change current dir
				old = CurrentDir(packet->cd);

				// DOS launch?
				if (packet->type == LAUNCH_DOS)
				{
					// Launch program
					if (SystemTags(packet->name,
								   SYS_Input,
								   packet->in,
								   SYS_Output,
								   packet->out,
								   SYS_Asynch,
								   TRUE,
								   SYS_UserShell,
								   TRUE,
								   NP_Cli,
								   TRUE,
								   NP_StackSize,
								   packet->stack,
								   NP_WindowPtr,
								   0,
								   (packet->wait) ? NP_ExitCode : TAG_IGNORE,
								   &launch_exit_code,
								   NP_ExitData,
								   packet,
								   TAG_DONE) != -1)
					{
						// Clear input/output pointers
						packet->in = 0;
						packet->out = 0;

						// Set success
						msg->command = 1;

						// Wait for reply?
						if (packet->wait)
						{
							// Lock launch list
							GetSemaphore(&data->launch_lock, SEMF_EXCLUSIVE, 0);

							// Increment count
							++data->launch_count;

							// First program? Increment open count so we can't be flushed
							if (data->launch_count == 1)
							{
								++DOpusBase->libBase.lib_OpenCnt;
#warning D(bug) printf there cause crash on os3 build (and on real, and on emulation on os3/mos). TODO: invistigate and make a proper one
#ifndef __amigaos3__
								D(bug("lib_OpenCnt bumped to %d by '%s'\n",
									  DOpusBase->libBase.lib_OpenCnt,
									  packet->name));
#endif
							}

							// Unlock launch list
							FreeSemaphore(&data->launch_lock);

							// Save reply message
							packet->exit_reply = msg;
							msg = 0;
						}
					}

					// Failed
					else
						msg->command = 0;
				}

				// Workbench launch
				else
				{
					// Lock launch list
					GetSemaphore(&data->launch_lock, SEMF_EXCLUSIVE, 0);

					// Launch program
					if ((proc = launcher_launch(data, packet, &data->launch_list, reply_port)))
					{
						// Success
						msg->command = 1;

						// Increment count
						++data->launch_count;

						// First program? Increment open count so we can't be flushed
						if (data->launch_count == 1)
						{
							++DOpusBase->libBase.lib_OpenCnt;
#warning D(bug) printf there cause crash on os3 build (and on real, and on emulation on os3/mos). TODO: invistigate and make a proper one
#ifndef __amigaos3__
							D(bug("lib_OpenCnt bumped to %d by '%s'\n", DOpusBase->libBase.lib_OpenCnt, packet->name));
#endif
						}

						// Wait for reply?
						if (packet->wait && !packet->notify_ipc)
						{
							// Save reply message
							proc->exit_reply = msg;
							msg = 0;

							// Timeout?
							if (packet->wait == LAUNCH_WAIT_TIMEOUT)
							{
								// Get timeout timer
								if ((proc->timeout = AllocTimer(UNIT_VBLANK, timer->port)))
								{
									// Start timeout timer
									StartTimer(proc->timeout, LAUNCH_TIMEOUT, 0);
								}
							}
						}
					}

					// Failed
					else
						msg->command = 0;

					// Unlock launch list
					FreeSemaphore(&data->launch_lock);
				}

				// Restore current dir
				CurrentDir(old);

				// Free packet (unless waiting)
				if (msg)
					free_launch_packet(data, packet);
			}

			// MUFS login
			else if (msg->command == IPC_LOGIN)
			{
				// Do it
				//				do_mufs_logout((struct LoginPkt *)msg->data_free);
			}

			// Clip something
			else if (msg->command == CLIP_PUTSTRING || msg->command == CLIP_GETSTRING)
			{
				ClipData *clip;
				ClipHandle *cb;

				// Get clip packet
				clip = (ClipData *)msg->data;
				clip->result = 0;

				// Open clipboard
				if ((cb = OpenClipBoard(0)))
				{
					// Put string?
					if (msg->command == CLIP_PUTSTRING)
						clip->result = WriteClipString(cb, clip->string, clip->length);

					// Get string
					else
						clip->result = ReadClipString(cb, clip->string, clip->length);

					// Close clipboard
					CloseClipBoard(cb);
				}
			}

			// Convert key
			else if (msg->command == STRING_CONVERT_KEY)
			{
				// Build the key string
				BuildKeyString((msg->flags >> 16) & 0xffff, msg->flags & 0xffff, 0xffff, 0, (char *)msg->data);
			}

			// Reinstall fake Workbench
			else if (msg->command == IPC_RESET)
			{
				// Get new path list
				UpdateMyPaths();

#ifdef FAKEWB
				// Re-install the fake Workbench program
				if (data->fake_wb)
				{
					// Remove and re-install it
					remove_fake_workbench(data);
					install_fake_workbench(data);
				}
#endif
			}

			// Reply to the message
			IPC_Reply(msg);

			// Quit?
			if (quit_msg)
				break;
		}

		// Lock launch list
		GetSemaphore(&data->launch_lock, SEMF_SHARED, 0);

		// Got zero count and pending quit?
		if (data->launch_count == 0 && pending_quit)
		{
			// Set quit flag
			quit_msg = pending_quit;
		}

		// Unlock launch list
		FreeSemaphore(&data->launch_lock);

		// Quit?
		if (quit_msg)
			break;

		// Wait for message
		if ((waitres =
				 Wait(1 << ipc->command_port->mp_SigBit | 1 << reply_port->mp_SigBit | data->error_wait |
					  ((timer) ? 1 << timer->port->mp_SigBit : 0) | 1 << data->low_mem_signal | SIGBREAKF_CTRL_F)) &
			SIGBREAKF_CTRL_F)
		{
			// Update dos list
			if (doslist_get(data, &data->dos_list, data->dos_list_memory, DLGF_NOTIFY))
			{
				// Restart timer
				StartTimer(timer, DOSLIST_TIMER, 0);
			}
		}

		// Low memory handler?
		else if (waitres & (1 << data->low_mem_signal))
		{
			// Broadcast notify message
			SendNotifyMsg(DN_FLUSH_MEM, 0, 0, 0, 0, 0);
		}
	}

	// Free signal bit
	if (data->low_mem_signal != -1)
	{
		// Free it
		FreeSignal(data->low_mem_signal);
		data->low_mem_signal = -1;
	}

	// Any outstanding error requesters?
	if (!(IsListEmpty((struct List *)&data->error_list)))
	{
		ErrorNode *error, *next;

		// Go through error requesters
		for (error = (ErrorNode *)data->error_list.mlh_Head; error->node.mln_Succ; error = next)
		{
			// Cache next node
			next = (ErrorNode *)error->node.mln_Succ;

			// Remove requester
			FreeSysRequest(error->window);
			FreeVec(error);
		}
	}

	// Delete reply port
	DeleteMsgPort(reply_port);

	// Free timers
	FreeTimer(secondtimer);
	FreeTimer(timer);

#ifdef FAKEWB
	// Kill fake workbench
	remove_fake_workbench(data);
#endif

	// Free dos list copy
	doslist_free(data, &data->dos_list);

	// Forbid while we reply and exit
	Forbid();

	// Reply to quit message
	IPC_Reply(quit_msg);

	// Exit
	IPC_Free(ipc);
}

LaunchProc *launcher_launch(struct LibData *data, LaunchPacket *packet, struct MinList *list, struct MsgPort *reply)
{
	char *arg_ptr, *ptr;
	char name[278], buf[278], *run_name;
	struct DiskObject *icon;
	LaunchProc *launch;
	short arg_count, arg = 0;
	BPTR old_dir, cur_dir = 0, parent;
	short result = 0;
	char *proc_name, *default_tool = 0;

	// Get program name
	arg_ptr = launcher_parse(packet->name, name, 256);

	// Count arguments
	for (arg_count = 0, ptr = arg_ptr; ptr; ptr = launcher_parse(ptr, 0, 0), arg_count++)
		;

	// Get icon
	if ((icon = GetDiskObject(name)) && icon->do_Type == WBPROJECT)
		default_tool = icon->do_DefaultTool;

	// If we couldn't get icon, or it's not a tool or a project, fail
	if (!icon || (icon->do_Type != WBTOOL && (icon->do_Type != WBPROJECT || !icon->do_DefaultTool)))
	{
		// Free icon
		if (icon)
		{
			FreeDiskObject(icon);
			icon = 0;
		}

		// Default tool supplied?
		if (packet->default_tool)
			default_tool = packet->default_tool;

		// Otherwise, failed
		else
			return 0;
	}

	// Allocate launch data
	if (!(launch = AllocVec(sizeof(LaunchProc) + sizeof(struct WBArg) * (arg_count + 1), MEMF_CLEAR)))
	{
		// Failed
		if (icon)
			FreeDiskObject(icon);
		return 0;
	}

	// Save notify port
	launch->notify_ipc = packet->notify_ipc;

	// Initialise startup message
	launch->startup.sm_Message.mn_ReplyPort = reply;
	launch->startup.sm_NumArgs = arg_count + 1;
	launch->startup.sm_ArgList = launch->args;

	// Is this a project?
	if (default_tool)
	{
		// We actually run the default tool
#ifdef __AROS__
		run_name = default_tool;
#else
		run_name = FilePart(default_tool);
#endif

		// Get parent
		parent = launcher_get_parent(data, default_tool);

		// Set first argument to default tool
		launch_setarg(launch, 0, parent, run_name);

		// Bump first argument
		++arg;
		++launch->startup.sm_NumArgs;
	}

	// Not a project, get original name
	else
		run_name = name;

	// Get parent of name
	parent = launcher_get_parent(data, name);

	// Fill out first argument
	launch_setarg(launch, arg++, parent, FilePart(name));

	// Fill in arguments
	for (; arg < launch->startup.sm_NumArgs; arg++)
	{
		// Get next argument
		arg_ptr = launcher_parse(arg_ptr, buf, 256);
		if (!buf[0])
			break;

		// Get parent
		parent = launcher_get_parent(data, buf);

		// Fill out argument
		launch_setarg(launch, arg, parent, FilePart(buf));
	}

	// Change current directory
#ifndef __AROS__
	old_dir = CurrentDir(DupLock(launch->startup.sm_ArgList[0].wa_Lock));
#endif

	// Get process name
	proc_name = FilePart(run_name);

	// Load program
	if (!(launch->startup.sm_Segment = (BPTR)LoadSeg(run_name)))
	{
		// If not an absolute path
		if (!strchr(run_name, ':') && !strchr(run_name, '/'))
		{
			PathListEntry *entry;

			// Lock path list
			GetSemaphore(&data->path_lock, SEMF_SHARED, 0);

			// Go through pathlist
			for (entry = (PathListEntry *)BADDR(data->path_list); entry; entry = (PathListEntry *)BADDR(entry->next))
			{
				BPTR old;

				// Change directory to here
				old = CurrentDir(entry->lock);

				// Load program
				launch->startup.sm_Segment = (BPTR)LoadSeg(proc_name);

				// Restore directory
				CurrentDir(old);

				// Did we get the program?
				if (launch->startup.sm_Segment)
				{
					// Duplicate the home directory
					cur_dir = DupLock(entry->lock);
					break;
				}
			}

			// Unlock path list
			FreeSemaphore(&data->path_lock);
		}
	}

	// Got segment?
	if (launch->startup.sm_Segment)
	{
		long stack;
		BPTR homedir;
		struct Process *proc;
		BPTR mycdir = 0;  // currentdir is 0 for non-AROS platforms

		// Default stack size
		stack = (icon) ? ((icon->do_StackSize + 3) & (~3)) : packet->stack;
		if (stack < packet->stack)
			stack = packet->stack;
		if (stack < STACK_DEFAULT)
			stack = STACK_DEFAULT;

		// Get home directory
		homedir = DupLock((cur_dir) ? cur_dir : launch->startup.sm_ArgList[0].wa_Lock);
#ifdef __AROS__
		mycdir = DupLock(homedir);
#endif

		// Launch program
		if ((proc = CreateNewProcTags(NP_Seglist,
									  launch->startup.sm_Segment,
									  NP_FreeSeglist,
									  FALSE,
									  NP_StackSize,
									  stack,
									  NP_Name,
									  proc_name,
									  NP_Priority,
									  0,
									  NP_HomeDir,
									  homedir,
									  NP_CurrentDir,
									  mycdir,
									  NP_Input,
									  0,
									  NP_Output,
									  0,
									  NP_CloseInput,
									  FALSE,
									  NP_CloseOutput,
									  FALSE,
									  NP_WindowPtr,
									  0,
									  TAG_END)))
		{
			// Save process pointer in packets
			if (packet->proc_ptr)
				*(packet->proc_ptr) = proc;
			launch->proc = proc;

			// Set process pointer
			launch->startup.sm_Process = &proc->pr_MsgPort;

			// Set tool window
			if (icon && icon->do_ToolWindow)
			{
				strcpy(launch->toolwindow, icon->do_ToolWindow);
				launch->startup.sm_ToolWindow = launch->toolwindow;
			}

			// Set flag
			launch->flags |= LAUNCHF_LAUNCHOK;

			// Open window under mouse?
			if (packet->flags & LAUNCHF_OPEN_UNDER_MOUSE)
				data->open_window_kludge = proc;

			// Send startup message
			PutMsg(launch->startup.sm_Process, (struct Message *)&launch->startup);

			// Store name
			stccpy(launch->name, proc_name, 39);

			// Add launch info to list
			AddTail((struct List *)list, (struct Node *)launch);
			launch->node.ln_Name = proc->pr_Task.tc_Node.ln_Name;

			// Return success
			result = 1;
		}

		// Failed
		else
			UnLock(homedir);
	}

	// Restore current directory
#ifndef __AROS__
	UnLock(CurrentDir(old_dir));
#endif
	if (cur_dir)
		UnLock(cur_dir);

	// If failed, show error
	if (!result)
		v_launch_error(data, packet, MSG_UNABLE_TO_OPEN_TOOL, MSG_OK, (IPTR)proc_name);

	// Free icon
	if (icon)
		FreeDiskObject(icon);

	// If failed, clean up
	if (!result)
	{
		launch_cleanup(data, launch);
		launch = 0;
	}

	// Return launch process
	return launch;
}

// Clean up launch data
void launch_cleanup(struct LibData *data, LaunchProc *launch)
{
	if (launch)
	{
		short arg;

		// Unload program
		if (launch->startup.sm_Segment)
			UnLoadSeg(launch->startup.sm_Segment);

		// Go through arguments
		for (arg = 0; arg < launch->startup.sm_NumArgs; arg++)
		{
			// Free arg
			UnLock(launch->startup.sm_ArgList[arg].wa_Lock);
			FreeVec(launch->startup.sm_ArgList[arg].wa_Name);
		}

		// Free timer
		FreeTimer(launch->timeout);

		// Free launch packet
		FreeVec(launch);
	}
}

// Fill out a WBArg
void launch_setarg(LaunchProc *launch, short arg, BPTR lock, char *name)
{
	// Store lock
	launch->startup.sm_ArgList[arg].wa_Lock = lock;

	// Copy name
	if (name && (launch->startup.sm_ArgList[arg].wa_Name = AllocVec(strlen(name) + 1, 0)))
		strcpy(launch->startup.sm_ArgList[arg].wa_Name, name);
}

// Parse string for launcher
char *launcher_parse(char *string, char *buffer, short bufsize)
{
	BOOL quote = 0;
	short len = 0;

	// Skip leading spaces
	while (*string && *string == ' ')
		++string;

	// Quote?
	if (*string == '\"')
	{
		quote = 1;
		++string;
	}

	// Clear buffer
	if (buffer)
		*buffer = 0;

	// Copy into buffer
	while (*string)
	{
		// Space and not in quotes?
		if (*string == ' ' && !quote)
			break;

		// End of quotes?
		else if (*string == '\"' && quote)
		{
			++string;
			break;
		}

		// Store in buffer if there's room
		if (buffer && len < bufsize)
		{
			*buffer++ = *string;
			++len;
		}

		// Increment string pointer
		++string;
	}

	// Null-terminate buffer
	if (buffer)
		*buffer = 0;

	// Skip to next word
	while (*string && *string == ' ')
		++string;

	// Return new pointer
	return (*string) ? string : 0;
}

// Get parent lock
BPTR launcher_get_parent(struct LibData *data, char *name)
{
	char buf[256], *ptr, c;
	BPTR lock, parent;

	// Copy name
	strcpy(buf, name);

	// Lock file
	if (!(lock = Lock(name, ACCESS_READ)))
	{
		// Try for icon
		strcat(buf, ".info");
		lock = Lock(buf, ACCESS_READ);
	}

	// Got lock?
	if (lock)
	{
		// Is it a directory?
		if ((c = name[strlen(name) - 1]) == '/' || c == ':')
		{
			// Use the lock itself, clear the name
			*name = 0;
			return lock;
		}

		// Get parent directory
		parent = ParentDir(lock);
		UnLock(lock);
		return parent;
	}

	// Strip file, lock path
	if ((ptr = FilePart(buf)))
	{
		*ptr = 0;
		lock = Lock(buf, ACCESS_READ);
	}

	// No lock, get current dir
	if (!lock)
		lock = Lock("", ACCESS_READ);
	return lock;
}

// Free launch packet
void free_launch_packet(struct LibData *data, LaunchPacket *packet)
{
	if (packet)
	{
		UnLock(packet->cd);
		if (packet->in)
			Close(packet->in);
		if (packet->out)
			Close(packet->out);
		FreeVec(packet);
	}
}

// Signal an error
static ErrorNode *launch_error(struct LibData *data, LaunchPacket *packet, short msg, short buttons, APTR args)
{
	ErrorNode *error;
	struct EasyStruct easy;
	struct Window *parent = 0;

	// No errors?
	if (packet && packet->errors == (struct Screen *)-1)
		return 0;

	// Allocate error node
	if (!(error = AllocVec(sizeof(ErrorNode), MEMF_CLEAR)))
		return 0;

	// Fill out requester struct
	easy.es_StructSize = sizeof(easy);
	easy.es_Flags = 0;
	easy.es_Title = "Directory Opus";
	easy.es_TextFormat = GetString(&data->locale, msg);
	easy.es_GadgetFormat = GetString(&data->locale, buttons);

	// Get parent window
	if (packet && packet->errors)
		parent = packet->errors->FirstWindow;

	error->window = BuildEasyRequestArgs(parent, &easy, 0, args);
	// Display error
	if (!error->window)
	{
		// Failed
		if (packet)
			DisplayBeep(packet->errors);
		FreeVec(error);
		return 0;
	}

	// Get bit to wait on
	data->error_wait |= 1 << error->window->UserPort->mp_SigBit;

	// Add to error list
	AddTail((struct List *)&data->error_list, (struct Node *)error);

	return error;
}

#ifdef FAKEWB
// Install fake workbench task
BOOL install_fake_workbench(struct LibData *data)
{
	// If workbench is already running, do nothing
	if (FindTask("Workbench"))
		return 0;

	// Launch workbench task
	IPC_Launch(
		0, &data->fake_wb, "Workbench", IPC_NATIVE(fake_workbench), STACK_DEFAULT | IPCF_GETPATH, (IPTR)data, DOSBase);

	// Success?
	return (BOOL)((data->fake_wb) ? 1 : 0);
}

// Remove fake workbench task
void remove_fake_workbench(struct LibData *data)
{
	// Send quit command
	if (data->fake_wb)
	{
		IPC_Quit(data->fake_wb, 0, TRUE);
		data->fake_wb = 0;
	}
}

// Fake workbench task
void SAVEDS fake_workbench(void)
{
	IPCData *ipc;
	struct LibData *data;
	IPCMessage *msg = 0;

	// Do startup
	if (!(ipc = L_IPC_ProcStartup((IPTR *)&data, 0)))
		return;

	// Set program name
	SetProgramName(ipc->proc->pr_Task.tc_Node.ln_Name);

	// Loop for quit message
	FOREVER
	{
		// Any messages?
		if (msg = (IPCMessage *)GetMsg(ipc->command_port))
		{
			// Quit?
			if (msg->command == IPC_QUIT)
				break;

			// Reply it
			IPC_Reply(msg);
		}

		// Wait for messages
		Wait(1 << ipc->command_port->mp_SigBit);
	}

	// Forbid so library doesn't unload before we quit
	Forbid();

	// Reply to the quit message
	IPC_Reply(msg);

	// Free IPC data and exit
	IPC_Free(ipc);
}
#endif

#define DL_DELETE 0
#define DL_RETAINED 1
#define DL_ADDED 2

// Get a copy of the dos list
BOOL doslist_get(struct LibData *data, struct MinList *list, APTR memory, ULONG flags)
{
	struct DosList *dos, *doslist;
	DosListEntry *found, *next, *search;
	D_S(struct InfoData, info)
	struct Node *node;
	Att_List *msg_list = 0;
	char name[80];

	// Lock our list
	GetSemaphore(&data->dos_lock, SEMF_EXCLUSIVE, 0);

	// Lock the DOS list
	if (!(doslist = (flags & DLGF_FORCE) ? LockDosList(LDF_DEVICES | LDF_VOLUMES | LDF_READ)
										 : AttemptLockDosList(LDF_DEVICES | LDF_VOLUMES | LDF_READ)) ||
		(doslist == (struct DosList *)1))
	{
		// Failed; unlock our semaphore
		FreeSemaphore(&data->dos_lock);
		return 0;
	}

	// Clear flags; anything not set at the end will be deleted
	for (node = (struct Node *)list->mlh_Head; node->ln_Succ; node = node->ln_Succ)
	{
		// Clear flag
		node->ln_Pri = DL_DELETE;
	}

	/*************** Scan the DOS list for devices *****************/

	dos = doslist;
	while ((dos = NextDosEntry(dos, LDF_DEVICES)))
	{
		DosListEntry *found;

		// Invalid?
		if (!dos->dol_Name || !dos->dol_Task || !dos->dol_misc.dol_handler.dol_Startup)
			continue;

		// Convert name
		lsprintf(name, "%b", dos->dol_Name);

		// See if node exists
		search = (DosListEntry *)list;
		while ((found = (DosListEntry *)FindName((struct List *)search, name)))
		{
			// Right type?
			if (found->dle_Node.ln_Type == DLT_DEVICE)
			{
				found->dle_Node.ln_Pri = DL_RETAINED;
				break;
			}

			// Keep searching
			search = found;
		}

		// Was node not found?
		if (!found)
		{
			// Allocate new entry
			if ((found = AllocMemH(memory, sizeof(DosListEntry) + strlen(name) + 1)))
			{
				// Store name
				found->dle_Node.ln_Name = (char *)(found + 1);
				strcpy(found->dle_Node.ln_Name, name);

				// Set type, and flag to say found
				found->dle_Node.ln_Type = dos->dol_Type;
				found->dle_Node.ln_Pri = DL_ADDED;

				// Get device and unit
				GetDeviceUnit(dos->dol_misc.dol_handler.dol_Startup, found->dle_DeviceName, &found->dle_DeviceUnit);
				// Add to list
				AddHead((struct List *)list, (struct Node *)found);
			}
		}

		// Got node?
		if (found)
		{
			// Copy DOS list entry
			CopyMem((char *)dos, (char *)&found->dle_DosList, sizeof(struct DosList));
		}
	}

	/*************** Scan the DOS list for volumes *****************/

	dos = doslist;
	while ((dos = NextDosEntry(dos, LDF_VOLUMES)))
	{
		char device[40];
		DosListEntry *found = 0;

		// Invalid name, or removed device?
		if (!dos->dol_Name || !dos->dol_Task)
			continue;
		device[0] = 0;

		// Get device name
		DeviceFromHandler(dos->dol_Task, device);

		// Convert volume name
		lsprintf(name, "%b", dos->dol_Name);

		// Valid device?
		if (*device)
		{
			// Go through list
			for (search = (DosListEntry *)list->mlh_Head; search->dle_Node.ln_Succ;
				 search = (DosListEntry *)search->dle_Node.ln_Succ)
			{
				// Is it a volume?
				if (search->dle_Node.ln_Type == DLT_VOLUME)
				{
					// Match device name and datestamp to be the same
					if (strcmp(device, search->dle_DeviceName) == 0 &&
						CompareDates(&dos->dol_misc.dol_volume.dol_VolumeDate,
									 &search->dle_DosList.dol_misc.dol_volume.dol_VolumeDate) == 0)
					{
						// Set flag to say found
						search->dle_Node.ln_Pri = DL_RETAINED;
						found = search;
						break;
					}
				}
			}
		}

		// No device, so we have to look by name
		else
		{
			// See if node exists
			search = (DosListEntry *)list;
			while ((found = (DosListEntry *)FindName((struct List *)search, name)))
			{
				// Right type?
				if (found->dle_Node.ln_Type == DLT_VOLUME)
				{
					// Mark it as got
					found->dle_Node.ln_Pri = DL_RETAINED;
					break;
				}

				// Keep searching
				search = found;
			}
		}

		// Was node not found?
		if (!found)
		{
			// Allocate new entry
			if ((found = AllocMemH(memory, sizeof(DosListEntry) + strlen(name) + 1)))
			{
				// Store name
				found->dle_Node.ln_Name = (char *)(found + 1);
				strcpy(found->dle_Node.ln_Name, name);

				// Set type, and flag to say found
				found->dle_Node.ln_Type = dos->dol_Type;
				found->dle_Node.ln_Pri = DL_ADDED;

				// Store device name
				strcpy(found->dle_DeviceName, device);

				// Add to list
				AddHead((struct List *)list, (struct Node *)found);
			}
		}

		// Got node?
		if (found)
		{
			// Copy DOS list entry
			CopyMem((char *)dos, (char *)&found->dle_DosList, sizeof(struct DosList));
		}
	}

	/***************************************************************/

	// Unlock dos list
	UnLockDosList(LDF_DEVICES | LDF_VOLUMES | LDF_READ);

	/***************************************************************/

	// Go through DOS list copy
	for (found = (DosListEntry *)list->mlh_Head; found->dle_Node.ln_Succ; found = next)
	{
		DosListEntry *test;
		struct DevProc *devproc;
		char buf[80];
		LONG res = 0;

		// Cache next entry
		next = (DosListEntry *)found->dle_Node.ln_Succ;

		// Skip volumes for now
		if (found->dle_Node.ln_Type == DLT_VOLUME)
			continue;

		// Removed device (unlikely)?
		if (found->dle_Node.ln_Pri == DL_DELETE)
			continue;

		// Go through DOS list, look for a volume to match this device
		for (test = (DosListEntry *)list->mlh_Head; test->dle_Node.ln_Succ;
			 test = (DosListEntry *)test->dle_Node.ln_Succ)
		{
			// Volume?
			if (test->dle_Node.ln_Type == DLT_VOLUME)
			{
				// Does it match this device
				if (test->dle_DosList.dol_Task == found->dle_DosList.dol_Task)
					break;
			}
		}

		// Found valid volume?
		if (test->dle_Node.ln_Succ)
			continue;

		// Build name for 'bad' disk
		lsprintf(name, ":BAD:%b", found->dle_DosList.dol_Name);

		// Get existing bad entry if there is one
		if ((test = (DosListEntry *)FindName((struct List *)list, name)))
		{
			// Default to deleting entry
			test->dle_Node.ln_Pri = DL_DELETE;
		}

		// Build name buffer with colon
		lsprintf(buf, "%s:", (IPTR)name + 5);

		// Get handler
		if (!(devproc = GetDeviceProc(buf, NULL)))
		{
			continue;
		}

		// Send packet to get disk information
#ifdef __AROS__
		if (((struct Library *)DOSBase)->lib_Version < 50)
		{
			// if (Info(devproc->dvp_Lock,info))
			BPTR lock;
			if ((lock = Lock(buf, SHARED_LOCK)))
			{
				res = Info(lock, info);
				UnLock(lock);
			}
		}
		else
#endif
			res = DoPkt(devproc->dvp_Port, ACTION_DISK_INFO, (SIPTR)MKBADDR(info), 0, 0, 0, 0);

		if (res)
		{
			// Check for bad disk
			if (info->id_DiskType == ID_UNREADABLE_DISK)
			{
				// Have an existing one?
				if (test)
				{
					// Set flag to say found
					test->dle_Node.ln_Pri = DL_RETAINED;
				}

				// Need to create it
				else if ((test = AllocMemH(memory, sizeof(DosListEntry) + strlen(name) + 1)))
				{
					// Store name
					test->dle_Node.ln_Name = (char *)(test + 1);
					strcpy(test->dle_Node.ln_Name, name);

					// Set type and flag so we don't get deleted
					test->dle_Node.ln_Type = DLT_VOLUME;
					test->dle_Node.ln_Pri = DL_ADDED;

					// Add to list
					AddTail((struct List *)list, (struct Node *)test);
				}
			}

			// If drive is busy, we ignore it for this round
			else if (info->id_DiskType == ID_BUSY)
			{
				// If we had an entry, keep it
				if (test)
					test->dle_Node.ln_Pri = DL_RETAINED;
			}
		}

		// Free handler
		FreeDeviceProc(devproc);
	}

	/***************************************************************/

	// Allocate message list if needed
	if (flags & DLGF_NOTIFY)
		msg_list = Att_NewList(0);

	// Go through DOS list
	for (found = (DosListEntry *)list->mlh_Head; found->dle_Node.ln_Succ; found = next)
	{
		// Cache next
		next = (DosListEntry *)found->dle_Node.ln_Succ;

		// If notify flag (checking for new disks) is set, see if this is a volume
		if ((flags & DLGF_NOTIFY) && found->dle_Node.ln_Type == DLT_VOLUME)
		{
			// Entry recently added or removed
			if (found->dle_Node.ln_Pri == DL_ADDED || found->dle_Node.ln_Pri == DL_DELETE)
			{
				// Is it a bad disk?
				if (found->dle_Node.ln_Name && strncmp(found->dle_Node.ln_Name, ":BAD:", 5) == 0)
				{
					// Was it added?
					if (found->dle_Node.ln_Pri == DL_ADDED)
					{
						// Check that this isn't a doubled-up disk
						if (doslist_check_double(data, (struct List *)list, found->dle_Node.ln_Name + 5))
						{
							// Copy name, with trailing colon
							lsprintf(name, "%s:", (IPTR)found->dle_Node.ln_Name + 5);

							// Send notification
							if (msg_list)
								Att_NewNode(msg_list, name, (ULONG)-1, ADDNODE_EXCLUSIVE);
						}
					}

					// Removed; send notification
					else if (msg_list)
						Att_NewNode(msg_list, name, (ULONG)-2, ADDNODE_EXCLUSIVE);
				}

				// Find device entry for this volume
				else if (DeviceFromHandler(found->dle_DosList.dol_Task, name))
				{
					// Send notification
					if (msg_list)
						Att_NewNode(msg_list, name, (found->dle_Node.ln_Pri == DL_ADDED) ? 1 : 0, ADDNODE_EXCLUSIVE);
				}

				// No device found
				else if (found->dle_Node.ln_Name)
				{
					char name[80];

					// Get volume name
					lsprintf(name, "%s:", (IPTR)found->dle_Node.ln_Name);

					// Send notification based on name
					if (msg_list)
						Att_NewNode(msg_list, name, (found->dle_Node.ln_Pri == DL_ADDED) ? 1 : 0, ADDNODE_EXCLUSIVE);
				}
			}
		}

		// Remove an entry?
		if (found->dle_Node.ln_Pri == DL_DELETE)
		{
			// Remove from list and free it
			Remove((struct Node *)found);
			FreeMemH(found);
		}
	}

	// Got list of messages to send?
	if (msg_list)
	{
		Att_Node *node;

		// Go through list
		for (node = (Att_Node *)msg_list->list.lh_Head; node->node.ln_Succ; node = (Att_Node *)node->node.ln_Succ)
		{
			// Send notify message
			SendNotifyMsg(DN_DISKCHANGE, 0, node->data, FALSE, node->node.ln_Name, 0);
		}

		// Free list
		Att_RemList(msg_list, 0);
	}

	// Release our semaphore lock
	FreeSemaphore(&data->dos_lock);
	return 1;
}

// Free a dos list copy
void doslist_free(struct LibData *data, struct MinList *list)
{
	struct Node *node, *next;

	// Go through list
	for (node = (struct Node *)list->mlh_Head; node->ln_Succ; node = next)
	{
		// Cache next
		next = node->ln_Succ;

		// Free this entry
		FreeMemH(node);
	}

	// Re-initialise list
	NewList((struct List *)list);
}

// Signal launcher to update drives
void LIBFUNC L_NotifyDiskChange(void)
{
	// Got launcher?
	if (launcher_ipc)
	{
		// Signal it
		Signal((struct Task *)launcher_ipc->proc, SIGBREAKF_CTRL_F);
	}
}

char *device_check[] =
	{"trackdisk.device", "diskspare.device", "floppy.device", "mfm.device", "mfm.device", "diskspare.device", 0};

char *device_check_name[] = {"DF", "DS", "FS", "PC", "MC", "PS", 0};

// Check that a bad disk isn't doubled up with a good disk
BOOL doslist_check_double(struct LibData *data, struct List *list, char *name)
{
	DosListEntry *disk;
	char device[40];
	short a, testdev, unit;

	// Find entry in list
	if (!(disk = (DosListEntry *)FindName(list, name)))
		return TRUE;

	// Get device name and unit
	strcpy(device, disk->dle_DeviceName);
	unit = disk->dle_DeviceUnit;

	// Valid device?
	if (!*device)
		return TRUE;

	// Go through list of test devices, match this device
	for (testdev = 0; device_check[testdev]; testdev++)
		if (stricmp(device, device_check[testdev]) == 0)
			break;

	// Doesn't match? Return OK
	if (!device_check[testdev])
		return TRUE;

	// Go through device list
	for (disk = (DosListEntry *)list->lh_Head; disk->dle_Node.ln_Succ; disk = (DosListEntry *)disk->dle_Node.ln_Succ)
	{
		// Device with a valid device name?
		if (disk->dle_Node.ln_Type == DLT_DEVICE && *disk->dle_DeviceName)
		{
			// Go through list of test devices, see if this matches
			for (a = 0; device_check[a]; a++)
			{
				// Skip over the one we're testing against
				if (a == testdev)
					continue;

				// Match device name and unit
				if (stricmp(disk->dle_DeviceName, device_check[a]) == 0 && disk->dle_DeviceUnit == unit)
				{
					BPTR lock;
					char name[20];

					// Build disk name
					lsprintf(name, "%s%ld:", (IPTR)device_check_name[a], unit);

					// See if disk is valid
					if ((lock = Lock(name, ACCESS_READ)))
					{
						// We found a volume that matches the device, so there's a good disk too
						UnLock(lock);
						return FALSE;
					}
				}
			}
		}
	}

	// No good disks so we can signal bad
	return TRUE;
}

// Get a user copy of the dos list
void LIBFUNC L_GetDosListCopy(REG(a0, struct List *list), REG(a1, APTR memory), REG(a6, struct MyLibrary *libbase))
{
	struct LibData *data;
	DosListEntry *entry;

#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Get data pointer
	data = (struct LibData *)libbase->ml_UserData;

	// Initialise the list
	NewList(list);

	// Try and get the list
	if (doslist_get(data, (struct MinList *)list, memory, 0))
		return;

	// Lock the dos list copy
	GetSemaphore(&data->dos_lock, SEMF_EXCLUSIVE, 0);

	// Go through the list
	for (entry = (DosListEntry *)data->dos_list.mlh_Head; entry->dle_Node.ln_Succ;
		 entry = (DosListEntry *)entry->dle_Node.ln_Succ)
	{
		DosListEntry *new;
		char *name;
		short length = 0;

		// Get name pointer, and length
		if ((name = entry->dle_Node.ln_Name))
			length = strlen(name);

		// Create a new entry
		if ((new = AllocMemH(memory, sizeof(DosListEntry) + length + 1)))
		{
			// Set name
			if (name)
			{
				// Set pointer and copy
				new->dle_Node.ln_Name = (char *)(new + 1);
				strcpy(new->dle_Node.ln_Name, name);
			}

			// Copy doslist entry
			CopyMem((char *)&entry->dle_DosList, (char *)&new->dle_DosList, sizeof(struct DosList));

			// Add to list
			AddTail(list, (struct Node *)new);
		}
	}

	// Release list lock
	FreeSemaphore(&data->dos_lock);
}

// Free a user copy of the dos list
void LIBFUNC L_FreeDosListCopy(REG(a0, struct List *list), REG(a6, struct MyLibrary *libbase))
{
	struct LibData *data;

#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Get data pointer
	data = (struct LibData *)libbase->ml_UserData;

	// Free copy
	doslist_free(data, (struct MinList *)list);
}

#undef DOSBase
