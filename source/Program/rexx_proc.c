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

#include "dopus.h"

//#define RX_DEBUG

#define REFRESH_TIME 5

// Initialise rexx port
IPC_StartupCode(rexx_init, ULONG *, foo, static)
{
	short a;
	struct MsgPort *port = 0;

	// Create a unique port name
	Forbid();

	// Start at 1; only try 100 times (why would you want 100 copies of DOpus? :)
	for (a = 1; a < 100; a++)
	{
		// Build port name
		lsprintf(GUI->rexx_port_name, "DOPUS.%ld", a);

		// See if port already exists
		if (!(FindPort(GUI->rexx_port_name)))
		{
			// It doesn't; try to create it
			if ((port = CreateMsgPort()))
			{
				// Make port public
				port->mp_Node.ln_Pri = 50;
				port->mp_Node.ln_Name = GUI->rexx_port_name;
				AddPort(port);

				// Store copy number
				GUI->dopus_copy = a;
			}
			break;
		}
	}
	Permit();

	// Did we get a port?
	if (port)
	{
		// Return pointer
		ipc->userdata = port;
		return 1;
	}

	// Failed
	return 0;
}

// REXX process
IPC_EntryCode(rexx_proc)
{
	IPCData *ipc;
	struct MsgPort *rexx_port, *reply_port = 0, *notify_port = 0;
	IPCMessage *imsg;
	struct RexxMsg *rmsg;
	RexxSyncMessage *smsg;
	struct AppMessage *amsg;
	DOpusNotify *nmsg;
	long message_count = 0;	 // Count of outstanding messages
	struct MinList msg_list;
	BOOL pending_quit = 0;
	APTR notify_req = 0;
	FunctionHandle *handle;
	TimerHandle *timer = 0;

	// Do startup
	if (!(ipc = IPC_ProcStartup(0, (ULONG (*)(IPCData *, APTR))&rexx_init)) || !(reply_port = CreateMsgPort()) ||
		!(GUI->rexx_app_port = CreateMsgPort()) || !(notify_port = CreateMsgPort()) ||
		!(timer = AllocTimer(UNIT_VBLANK, 0)) || !(rexx_port = (struct MsgPort *)IPCDATA(ipc)))
	{
		FreeTimer(timer);
		DeleteMsgPort(notify_port);
		DeleteMsgPort(GUI->rexx_app_port);
		DeleteMsgPort(reply_port);
		return;
	}

#if 0
	// Check for valid serial number
	SerialCheck(GUI->rego.serial_number,(ULONG *)&GUI->backdrop);
#endif

	// Initialise message list
	NewList((struct List *)&msg_list);

	// Try to create function handle
	if ((handle = function_new_handle(0, 1)))
	{
		// Add notification for dos events
		notify_req = AddNotifyRequest(DN_DOS_ACTION, 0, notify_port);

		// Start timer for five second intervals
		StartTimer(timer, REFRESH_TIME, 0);
	}

	// Event loop
	FOREVER
	{
		// Got notify request
		if (notify_req)
		{
			// Check refresh timer
			if (CheckTimer(timer))
			{
				// See if change list is not empty
				if (!(IsListEmpty(&handle->filechange)))
				{
					// Do changes
					function_filechange_do(handle, 1);
				}

				// Restart timer
				StartTimer(timer, REFRESH_TIME, 0);
			}
		}

		// IPC messages
		while ((imsg = (IPCMessage *)GetMsg(ipc->command_port)))
		{
			// Look at message
			switch (imsg->command)
			{
			// Quit
			case IPC_QUIT:
				pending_quit = 1;
				break;

			// Increment message count
			case REXXCMD_INCREMENT_MSG:
				++message_count;
				break;

			// Goodbye from something
			case IPC_GOODBYE:

				// Check readers
				rexx_goodbye_reader((IPCData *)imsg->data);
				break;

			// Send a rexx command
			case REXXCMD_SEND_MSG:
				if (rexx_send_msg(&msg_list, &imsg, rexx_port))
					++message_count;
				break;

			// Send a REXX message
			case REXXCMD_SEND_RXMSG:
				if (rexx_send_rxmsg(imsg, rexx_port))
					++message_count;
				break;
			}

			// Reply to message
			IPC_Reply(imsg);
		}

		// App messages?
		while ((amsg = (struct AppMessage *)GetMsg(GUI->rexx_app_port)))
		{
			// Intuimessage reply?
			if (amsg->am_Message.mn_Node.ln_Type == NT_REPLYMSG &&
				amsg->am_Message.mn_Length == sizeof(struct IntuiMessage))
			{
				// Just free the reply
				FreeVec(amsg);
			}

			// A real message
			else
			{
				// Handle it
				rexx_handle_appmsg(amsg);

				// Reply to it
				ReplyAppMessage((DOpusAppMessage *)amsg);
			}
		}

		// Notification messages
		while ((nmsg = (DOpusNotify *)GetMsg(notify_port)))
		{
			// Handle notify
			handle_dos_notify(nmsg, handle);
			ReplyFreeMsg(nmsg);
		}

		// REXX messages
		while ((rmsg = (struct RexxMsg *)GetMsg(rexx_port)))
		{
			// Reply message?
			if (rmsg->rm_Node.mn_Node.ln_Type == NT_REPLYMSG)
			{
				RexxMsgTracker *track;

				// Decrement message count
				--message_count;

				// Look for message in 'track' list
				for (track = (RexxMsgTracker *)msg_list.mlh_Head; track->node.mln_Succ;
					 track = (RexxMsgTracker *)track->node.mln_Succ)
				{
					// See if message matches
					if (track->rx_msg == rmsg)
					{
						// Reply to IPC message
						IPC_Reply(track->ipc_msg);

						// Remove and free track node
						Remove((struct Node *)track);
						FreeVec(track);
						break;
					}
				}

				// Free message
				FreeRexxMsgEx(rmsg);
			}

			// Otherwise
			else
			{
				// Is there a quit pending?
				if (pending_quit)
					rmsg->rm_Result1 = RXERRORIMGONE;

				// Otherwise
				else
				{
					// Initialise result fields
					rmsg->rm_Result1 = 0;
					rmsg->rm_Result2 = 0;

#ifdef RX_DEBUG
					{
						short a;

						D(bug("Got REXX message!\n"));
						for (a = 0; a < (rmsg->rm_Action & RXARGMASK); a++)
							D(bug("Arg %ld : %s\n", a, rmsg->rm_Args[a]));
					}
#endif

					// Process message
					if (rexx_process_msg(rmsg, reply_port, &message_count))
					{
						// Clear message pointer
						rmsg = 0;
#ifdef RX_DEBUG
						D(bug("Message swallowed\n"));
#endif
					}
				}

				// Reply to message
				if (rmsg)
				{
#ifdef RX_DEBUG
					D(bug("Replying to message (port %lx)\n", rmsg->rm_Node.mn_ReplyPort));
#endif
					rexx_reply_msg(rmsg);
				}
			}
		}

		// Sync-replies
		while ((smsg = (RexxSyncMessage *)GetMsg(reply_port)))
		{
			// Reply to rexx message
			if (smsg->rmsg)
			{
				char buf[10];

				// Build result string
				lsprintf(buf, "%ld", smsg->msg.mn_Node.ln_Pri);
				rexx_set_return(smsg->rmsg, 0, buf);

				// Reply to message
				rexx_reply_msg(smsg->rmsg);
			}

			// Free reply
			FreeVec(smsg);

			// Decrement message count
			--message_count;
		}

		// Quit?
		if (pending_quit)
		{
			// Remove all AppThings
			rexx_rem_appthing(0, REXXAPP_ALL);

			// If there's no outstanding messages, quit immediately
			if (message_count < 1)
			{
				BOOL ok = 0;

				// Check reader list is empty
				lock_listlock(&GUI->rexx_readers, FALSE);

				// If it is, we can go
				if (IsListEmpty(&GUI->rexx_readers.list))
					ok = 1;

				// Unlock list
				unlock_listlock(&GUI->rexx_readers);

				// Ok to go?
				if (ok)
					break;
			}
		}

		// Wait for a message
		Wait(1 << ipc->command_port->mp_SigBit | 1 << rexx_port->mp_SigBit | 1 << reply_port->mp_SigBit |
			 1 << notify_port->mp_SigBit | 1 << GUI->rexx_app_port->mp_SigBit | 1 << timer->port->mp_SigBit);
	}

	// Stop notification
	RemoveNotifyRequest(notify_req);

	// Free timer
	FreeTimer(timer);

	// Free handle
	function_free(handle);

	// Flush ports
	Forbid();
	RemPort(rexx_port);
	flush_port(rexx_port);
	Permit();
	DeleteMsgPort(rexx_port);
	DeleteMsgPort(reply_port);
	flush_port(notify_port);
	DeleteMsgPort(notify_port);
	DeleteMsgPort(GUI->rexx_app_port);

	// Send goodbye message
	IPC_Goodbye(ipc, &main_ipc, 0);

	// Exit
	IPC_Free(ipc);
}

// Process a REXX message
BOOL rexx_process_msg(struct RexxMsg *msg, struct MsgPort *reply, long *count)
{
	short command, subcommand;
	char *commandptr;
	BOOL ret = 0;

#ifdef RX_DEBUG
	D(bug("Command : %s\n", msg->rm_Args[0]));
#endif

	// Get initial command
	commandptr = (char *)msg->rm_Args[0];
	if (!(command = rexx_get_command(&commandptr)))
		return 0;

#ifdef RX_DEBUG
	D(bug("      # : %ld\n", command));
#endif

	// Internal command?
	if (command == RXCMD_COMMAND)
	{
		Cfg_Function *function;
		Lister *source = 0, *dest = 0;
		BOOL sync = 0, orig = 0;
		short opt;

		// Skip spaces
		rexx_skip_space(&commandptr);

		// Options
		while ((opt = rexx_match_keyword(&commandptr, command_keys, 0)) != -1)
		{
			// Run synchronously?
			if (opt == RXCOM_WAIT)
				sync = 1;

			// Source/dest?
			else if (opt == RXCOM_SOURCE || opt == RXCOM_DEST)
			{
				Lister *lister;

				// Get lister
				if ((lister = (Lister *)rexx_parse_iptr(&commandptr, 0, 0)))
				{
					// Is lister valid?
					if (rexx_lister_valid(lister))
					{
						// Source?
						if (opt == RXCOM_SOURCE)
							source = lister;

						// Dest
						else
							dest = lister;
					}

					// Fail with an error
					else
					{
						rexx_set_return(msg, RXERR_INVALID_HANDLE, 0);
						return 0;
					}
				}

				// Skip spaces
				rexx_skip_space(&commandptr);
			}

			// Original?
			else if (opt == RXCOM_ORIGINAL)
				orig = 1;
		}

#ifdef RX_DEBUG
		D(bug("Command : %s\n", commandptr));
#else

		// Create dummy function
		if ((function = new_default_function(commandptr, 0)))
		{
			struct Message *reply_msg = 0;

			// Want a reply?
			if (sync)
			{
				RexxSyncMessage *rep;

				// Create reply message
				if ((rep = AllocVec(sizeof(RexxSyncMessage), MEMF_CLEAR)))
				{
					// Set reply port
					rep->msg.mn_ReplyPort = reply;

					// Store rexx message pointer
					rep->rmsg = msg;

					// Use as reply message
					reply_msg = (struct Message *)rep;

					// Increment reply count
					++*count;
					ret = 1;
				}
			}

			// Set flag to free function
			function->function.flags2 |= FUNCF2_FREE_FUNCTION;

			// Use original function?
			if (orig)
				function->function.flags2 |= FUNCF2_ORIGINAL;

			// Launch function
			function_launch(
				FUNCTION_RUN_FUNCTION, function, 0, (reply_msg) ? FUNCF_SYNC : 0, source, dest, 0, 0, 0, reply_msg, 0);
		}
#endif
		return ret;
	}

	// Get sub-command
	if (!(subcommand = rexx_get_command(&commandptr)))
		return 0;

#ifdef RX_DEBUG
	D(bug("Sub-Command : %ld\n", subcommand));
#endif

	// Look at initial command
	switch (command)
	{
	// Lister stuff
	case RXCMD_LISTER:

		// New lister?
		if (subcommand == RXCMD_NEW)
		{
			rexx_lister_new(msg, commandptr);
		}

		// Other lister command
		else
			ret = rexx_lister_cmd(msg, subcommand, commandptr);
		break;

	// DOpus stuff
	case RXCMD_DOPUS:

		// Handle command
		ret = rexx_dopus_cmd(msg, subcommand, commandptr);
		break;
	}

	return ret;
}

// Set REXX return codes
void rexx_set_return(struct RexxMsg *msg, long rc, char *result)
{
	// Can only set result string if valid, asked for, and not asynchronous
	if (result && (!rc || result[0]) && msg->rm_Action & RXFF_RESULT && msg->rm_Node.mn_ReplyPort)
	{
		long len = strlen(result);
		while (len > 0 && result[len - 1] == ' ')
			--len;
		if (!(msg->rm_Result2 = (IPTR)CreateArgstring(result, len)))
			msg->rm_Result1 = RXERR_NO_MEMORY;
		else
			msg->rm_Result1 = 0;
	}

	// Otherwise, set RC instead
	else
	{
		msg->rm_Result1 = rc;
		msg->rm_Result2 = 0;
	}
}

// Read a text file through ARexx
RexxReader *rexx_read_file(short command, char *args, struct RexxMsg *msg)
{
	RexxReader *reader;
	BOOL quit = 0, delete = 0, got_pos = 0;
	struct IBox dims = {0, 0, 0, 0};
	short key;

	// Get command
	if (command == RXCMD_SMARTREAD)
		command = FUNC_SMARTREAD;
	else if (command == RXCMD_ANSIREAD)
		command = FUNC_ANSIREAD;
	else if (command == RXCMD_HEXREAD)
		command = FUNC_HEXREAD;
	else
		command = FUNC_READ;

	// Skip spaces
	rexx_skip_space(&args);

	// Lock reader list
	lock_listlock(&GUI->rexx_readers, TRUE);

	// Given a reader?
	if ((reader = (RexxReader *)rexx_parse_iptr(&args, FALSE, 0)))
	{
		RexxReader *test;

		// See if reader is in the list
		for (test = (RexxReader *)GUI->rexx_readers.list.lh_Head; test->node.mln_Succ;
			 test = (RexxReader *)test->node.mln_Succ)
		{
			// Match our reader?
			if (test == reader)
				break;
		}

		// Not in the list?
		if (test != reader)
			reader = 0;
	}

	// Skip spaces
	rexx_skip_space(&args);

	// Check flags
	while ((key = rexx_match_keyword(&args, reader_keys, 0)) != -1)
	{
		// Quit?
		if (key == 0)
			quit = 1;

		// Delete?
		else if (key == 1)
			delete = 1;

		// Position
		else if (key == 2)
		{
			// Get dimensions
			rexx_skip_space(&args);
			dims.Left = rexx_parse_number(&args, 1, 0);
			dims.Top = rexx_parse_number(&args, 1, 0);
			dims.Width = rexx_parse_number(&args, 1, 0);
			dims.Height = rexx_parse_number(&args, 0, 0);
			got_pos = 1;
		}

		// Mode
		else if (key == 3)
			command = FUNC_HEXREAD;
		else if (key == 4)
			command = FUNC_ANSIREAD;
		else if (key == 5)
			command = FUNC_SMARTREAD;

		// File name
		else if (key == 6)
			break;
	}

	// Skip spaces to filename
	rexx_skip_space(&args);

	// Need to create a new reader?
	if (!reader && !quit)
	{
		struct read_startup *startup = 0;
		Att_Node *node;
		BOOL fail = 1;

		// Allocate reader structure and startup packet
		if ((reader = AllocVec(sizeof(RexxReader), MEMF_CLEAR)) &&
			(startup = AllocVec(sizeof(struct read_startup), MEMF_CLEAR)) &&
			(startup->files = (struct List *)Att_NewList(0)) &&
			(node = Att_NewNode((Att_List *)startup->files, args, 0, 0)))
		{
			// Set delete flag
			if (delete)
				node->node.ln_Pri = 1;

			// Save position
			startup->dims = dims;
			startup->got_pos = got_pos;
			startup->wheel_scroll_lines = environment->env->env_wheel_scroll_lines;

			// Set owner for goodbye packet
			startup->owner = GUI->rexx_proc;

			// Add to list
			AddTail(&GUI->rexx_readers.list, (struct Node *)reader);

			// Launch reader
			if (!(reader->ipc = misc_startup("dopus_rexx_reader", command, 0, startup, FALSE)))
			{
				// Remove from reader list
				Remove((struct Node *)reader);
			}
			else
				fail = 0;
		}

		// Free if we failed
		if (fail)
		{
			// Free stuff
			if (startup)
			{
				Att_RemList((Att_List *)startup->files, 0);
				FreeVec(startup);
			}
			FreeVec(reader);
			reader = 0;
		}
	}

	// Otherwise, send command to reader
	else if (reader)
	{
		IPC_Command(reader->ipc, (quit) ? IPC_QUIT : READCOM_READ, delete, args, 0, (quit) ? 0 : REPLY_NO_PORT);
		if (quit)
			reader = 0;
	}

	// Unlock reader list
	unlock_listlock(&GUI->rexx_readers);

	// Return handle
	return reader;
}

// Got goodbye from a reader
void rexx_goodbye_reader(IPCData *ipc)
{
	RexxReader *reader;

	// Lock reader list
	lock_listlock(&GUI->rexx_readers, TRUE);

	// See if reader is in the list
	for (reader = (RexxReader *)GUI->rexx_readers.list.lh_Head; reader->node.mln_Succ;
		 reader = (RexxReader *)reader->node.mln_Succ)
	{
		// Match our reader?
		if (reader->ipc == ipc)
		{
			// Remove from list
			Remove((struct Node *)reader);
			FreeVec(reader);
			break;
		}
	}

	// Unlock reader list
	unlock_listlock(&GUI->rexx_readers);
}

void rexx_reply_msg(struct RexxMsg *rmsg)
{
	if (rmsg->rm_Node.mn_ReplyPort)
		ReplyMsg((struct Message *)rmsg);

	// No reply port; free instead
	else
		FreeRexxMsgEx(rmsg);
}

// Send a message to rexx
void rexx_send_command(char *command, BOOL wait)
{
	char *copy;

	// Copy command
	if ((copy = AllocVec(strlen(command) + 1, 0)))
	{
		// Copy and send it
		strcpy(copy, command);
		IPC_Command(GUI->rexx_proc, REXXCMD_SEND_MSG, wait, 0, copy, (wait) ? REPLY_NO_PORT : 0);
	}
}

// Send a REXX message
BOOL rexx_send_msg(struct MinList *list, IPCMessage **imsg, struct MsgPort *port)
{
	RexxMsgTracker *track;
	struct RexxMsg *msg;
	struct MsgPort *rexx;

	// Create message
	if (!(msg = CreateRexxMsg(port, ".dopus5", port->mp_Node.ln_Name)))
		return 0;

	// Build message
					msg->rm_Args[0] = (IPTR)(*imsg)->data_free;
	FillRexxMsg(msg, 1, 0);

	// Set flags
	msg->rm_Action |= 1 | RXCOMM;
	if (!(*imsg)->flags)
		msg->rm_Action |= RXFF_NONRET;

	// Find rexx port
	Forbid();
	if ((rexx = FindPort("REXX")))
	{
		// Send message
		PutMsg(rexx, (struct Message *)msg);
	}
	Permit();

	// Need to free message if not sent
	if (!rexx)
		FreeRexxMsgEx(msg);

	// Wait for reply?
	if ((*imsg)->flags)
	{
		// Allocate tracker
		if ((track = AllocVec(sizeof(RexxMsgTracker), MEMF_CLEAR)))
		{
			// Fill out tracker, add to list
			track->rx_msg = msg;
			track->ipc_msg = *imsg;
			AddTail((struct List *)list, (struct Node *)track);

			// Clear message pointer
			*imsg = 0;
		}
	}

	return 1;
}

// Send a RxMsg supplied by another task
BOOL rexx_send_rxmsg(IPCMessage *imsg, struct MsgPort *rxport)
{
	return rexx_send_rxmsg_args((RexxDespatch *)imsg->data_free, imsg->flags, rxport);
}

// Send a RxMsg supplied by another task
BOOL rexx_send_rxmsg_args(RexxDespatch *desp, ULONG flags, struct MsgPort *rxport)
{
	struct MsgPort *port;
	BOOL ok = 0;

	// No despatch?
	if (!desp)
		return 0;

	// Find port to send to
	Forbid();
	if (!(port = FindPort(desp->handler)))
	{
		// Port not found
		Permit();

		// Display error?
		if (flags & RXMF_WARN)
		{
			char buf[128];

			// Build error message
			lsprintf(buf, GetString(&locale, MSG_CUSTPORT_NOT_FOUND), desp->handler);

			// Show error
			global_requester(buf);
		}

		// Free message
		FreeRexxMsgEx(desp->rx_msg);
	}

	// Got port
	else
	{
		// Fix reply port
		desp->rx_msg->rm_Node.mn_ReplyPort = rxport;

		// Send the message
		PutMsg(port, (struct Message *)desp->rx_msg);

		// Permit now its sent
		Permit();

		// Increment message count
		ok = 1;
	}

	return ok;
}
