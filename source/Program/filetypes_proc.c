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

IPC_EntryProto(filetype_sniffer_proc, static);

// Get the filetype sniffer
IPCData *filetype_get_sniffer(void)
{
	IPCData *ipc;

	// Lock process list
	lock_listlock(&GUI->process_list, FALSE);

	// See if filetypes process is already active
	if (!(ipc = IPC_FindProc(&GUI->process_list, "dopus_filetype_sniffer", FALSE, 0)))
	{
		// Unlock process list
		unlock_listlock(&GUI->process_list);

		// It's not; launch it
		if (!(IPC_Launch(&GUI->process_list,
						 &ipc,
						 "dopus_filetype_sniffer",
						 (IPTR)&filetype_sniffer_proc,
						 STACK_DEFAULT,
						 0,
						 (struct Library *)DOSBase)))
			return 0;

		// Lock process list
		lock_listlock(&GUI->process_list, FALSE);
	}

	return ipc;
}

// Find a file's type for a lister
void filetype_find_type(Lister *lister, DirBuffer *buffer, char *name, short flags)
{
	IPCData *ipc;

	// Get sniffer
	if ((ipc = filetype_get_sniffer()))
	{
		SniffData *data;

		// Create packet
		if ((data = AllocVec(sizeof(SniffData) + strlen(name), MEMF_CLEAR)))
		{
			// Fill out packet
			data->lister = lister;
			data->buffer = buffer;
			data->flags = flags;
			strcpy(data->name, name);

			// Send the command
			IPC_Command(ipc, 0, 0, 0, data, 0);
		}

		// Unlock process list
		unlock_listlock(&GUI->process_list);
	}
}

// Find the filetypes for a list of files in a buffer the lister shows
void filetype_find_typelist(Lister *lister, DirBuffer *buffer, short flags)
{
	IPCData *ipc;

	// Get sniffer
	if ((ipc = filetype_get_sniffer()))
	{
		DirEntry *entry;
		SniffData *data;

		// Create packet for start command
		if ((data = AllocVec(sizeof(SniffData), MEMF_CLEAR)))
		{
			// Fill out packet
			data->lister = lister;
			data->buffer = buffer;
			data->flags = SNIFFF_START;

			// Send the command
			IPC_Command(ipc, 0, 0, 0, data, 0);
		}

		// Go through files
		for (entry = (DirEntry *)buffer->entry_list.mlh_Head; entry->de_Node.dn_Succ;
			 entry = (DirEntry *)entry->de_Node.dn_Succ)
		{
			BOOL ok = 0;

			// Need to get filetype?
			if (!(flags & SNIFFF_NO_FILETYPES) && entry->de_Node.dn_Type < 0 &&
				!(GetTagData(DE_Filetype, 0, entry->de_Tags)))
				ok = 1;

			// Need to get version?
			else if (flags & SNIFFF_VERSION && entry->de_Node.dn_Type < 0 && !(entry->de_Flags & ENTF_VERSION))
				ok = 1;

			// Ok to do this one?
			if (ok)
			{
				// Create packet
				if ((data = AllocVec(sizeof(SniffData) + strlen(entry->de_Node.dn_Name), MEMF_CLEAR)))
				{
					// Fill out packet
					data->lister = lister;
					data->buffer = buffer;
					data->flags = flags;
					strcpy(data->name, entry->de_Node.dn_Name);

					// Send the command
					IPC_Command(ipc, 0, 0, 0, data, 0);
				}
			}
		}

		// Create packet for stop command
		if ((data = AllocVec(sizeof(SniffData), MEMF_CLEAR)))
		{
			// Fill out packet
			data->lister = lister;
			data->buffer = buffer;
			data->flags = SNIFFF_STOP;

			// Send the command
			IPC_Command(ipc, 0, 0, 0, data, 0);
		}

		// Unlock process list
		unlock_listlock(&GUI->process_list);
	}
}

// Process to find filetypes for listers
IPC_EntryCode(filetype_sniffer_proc, static)
{
	IPCData *ipc;
	BPTR old = 0, our_lock = 0;
	DirBuffer *last_buf = 0;
	Lister *ignore_lister = 0;
	TimerHandle *timer;

	// Do startup
	ipc = IPC_ProcStartup(0, 0);

	// Open timer
	if ((timer = AllocTimer(UNIT_VBLANK, 0)))
	{
		// Send 5 second request
		StartTimer(timer, 5, 0);
	}

	// Wait for something to happen
	FOREVER
	{
		IPCMessage *msg;
		BOOL break_flag = 0, restart = 0;

		// Has timer returned?
		if (CheckTimer(timer))
		{
			// Do we have something locked?
			if (our_lock)
			{
				// Restore original path
				CurrentDir(old);

				// Unlock our lock
				UnLock(our_lock);
				our_lock = 0;

				// Clear last buffer pointer
				last_buf = 0;

				// Start timer for 4 second timeout
				StartTimer(timer, 4, 0);
			}

			// Nothing's happening, let's quit
			else
				break_flag = 1;
		}

		// IPC message
		while ((msg = (IPCMessage *)GetMsg(ipc->command_port)))
		{
			// Quit?
			if (msg->command == IPC_QUIT)
			{
				break_flag = 1;
				break;
			}

			// Abort
			else if (msg->command == IPC_ABORT)
			{
				// Save lister pointer
				ignore_lister = (Lister *)msg->data;
			}

			// Another message
			else if (msg->command == 0)
			{
				SniffData *data;
				Cfg_Filetype *type = 0;

				// Get sniffer data
				data = (SniffData *)msg->data_free;

				// Stop command?
				if (data->flags & SNIFFF_STOP && ignore_lister == data->lister)
					ignore_lister = 0;

				// Don't ignore this?
				if (!ignore_lister || ignore_lister != data->lister)
				{
					short send = 0;

					// Clear ignore pointer
					ignore_lister = 0;

					// Start or stop?
					if (data->flags & (SNIFFF_START | SNIFFF_STOP))
					{
						// Do we have a lister?
						if (data->lister)
						{
							// Lock lister list
							lock_listlock(&GUI->lister_list, FALSE);

							// See if lister is valid
							if (rexx_lister_valid(data->lister))
							{
								// Send command
								IPC_Command(data->lister->ipc, LISTER_SET_SNIFF, (data->flags & SNIFFF_START), 0, 0, 0);
							}

							// It's not, ignore this lister from now on
							else
								ignore_lister = data->lister;

							// Unlock lister list
							unlock_listlock(&GUI->lister_list);
						}

						// Fix flags so we won't do anything
						data->flags = SNIFFF_NO_FILETYPES;
					}

					// Different buffer?
					if (data->buffer && data->buffer != last_buf)
					{
						BPTR lock, temp;

						// Lock new path
						if ((lock = Lock(data->buffer->buf_Path, ACCESS_READ)))
						{
							// Change to this path
							temp = CurrentDir(lock);

							// Had we already changed dir?
							if (our_lock)
							{
								// Unlock the old directory
								UnLock(our_lock);
							}

							// Otherwise, save original directory
							else
								old = temp;

							// Save lock pointer
							our_lock = lock;
						}
					}

					// Save buffer pointer
					last_buf = data->buffer;

					// Allowed to get filetype?
					if (!(data->flags & SNIFFF_NO_FILETYPES))
					{
						// See if type is matched
						type = filetype_identify(data->name, FTTYPE_WANT_NAME, data->type_name, 0);
						if (data->type_name[0])
						{
							// Store filetype pointer
							data->type = type;
							send |= SNIFFF_FILETYPES;
						}
					}

					// Want version?
					if (data->flags & SNIFFF_VERSION)
					{
						short ver, rev;
						struct DateTime dt;

						// Get version
						if (GetFileVersion(data->name, &ver, &rev, &dt.dat_Stamp, 0))
						{
							// Store data
							data->ver = ver;
							data->rev = rev;
							data->days = dt.dat_Stamp.ds_Days;
							send |= SNIFFF_VERSION;
						}

						// Failed
						else
							data->flags &= ~SNIFFF_VERSION;
					}

					// Got something?
					if (send)
					{
						// Do we have a lister?
						if (data->lister)
						{
							// Lock lister list
							lock_listlock(&GUI->lister_list, FALSE);

							// See if lister is valid
							if (rexx_lister_valid(data->lister))
							{
								// Send command
								IPC_Command(data->lister->ipc, LISTER_FILETYPE_SNIFF, 0, 0, data, 0);
								msg->data_free = 0;
							}

							// It's not, ignore this lister from now on
							else
								ignore_lister = data->lister;

							// Unlock lister list
							unlock_listlock(&GUI->lister_list);
						}

						// Or just a buffer?
						else if (data->buffer)
						{
							DirEntry *entry;

							// Lock buffer
							buffer_lock(data->buffer, FALSE);

							// Find entry
							if ((entry = find_entry(
									 &data->buffer->entry_list, data->name, 0, data->buffer->more_flags & DWF_CASE)))
							{
								char *ptr;

								// Filetype?
								if (send & SNIFFF_FILETYPES)
								{
									// Get description pointer
									if (type)
										ptr = type->type.name;
									else
										ptr = data->type_name;

									// Valid description?
									if (ptr && *ptr)
									{
										// Set filetype string
										direntry_add_string(data->buffer, entry, DE_Filetype, ptr);
									}
								}

								// Version?
								if (send & SNIFFF_VERSION)
								{
									// Set version info
									direntry_add_version(data->buffer, entry, data->ver, data->rev, data->days);
								}
							}

							// Unlock buffer
							buffer_unlock(data->buffer);
						}
					}
				}

				// Restart timer
				restart = 1;
			}

			// Reply message
			IPC_Reply(msg);

			// Check signals for abort message
			if (SetSignal(0, 0) & SIGBREAKF_CTRL_C)
				break;
		}

		// Restart timer?
		if (restart)
		{
			// Resend second timeout
			StartTimer(timer, (our_lock) ? 1 : 5, 0);
		}

		// Quit set?
		else if (break_flag)
			break;

		// Wait for an event
		if (Wait(1 << ipc->command_port->mp_SigBit | SIGBREAKF_CTRL_C | ((timer) ? (1 << timer->port->mp_SigBit) : 0)) &
			SIGBREAKF_CTRL_C)
		{
			Lister *strip = 0;

			// Forbid to check port
			Forbid();

			// Go through command port in reverse
			for (msg = (IPCMessage *)ipc->command_port->mp_MsgList.lh_TailPred; msg->msg.mn_Node.ln_Pred;)
			{
				IPCMessage *pred;

				// Cache previous message
				pred = (IPCMessage *)msg->msg.mn_Node.ln_Pred;

				// Abort message?
				if (msg->command == IPC_ABORT)
				{
					// Save lister pointer
					strip = (Lister *)msg->data;
					ignore_lister = strip;

					// Remove this message and reply
					Remove((struct Node *)msg);
					IPC_Reply(msg);
				}

				// Sniff message
				else if (msg->command == 0 && msg->data && !(((SniffData *)msg->data)->flags & SNIFFF_STOP))
				{
					// Lister we're stripping?
					if (((SniffData *)msg->data)->lister == strip)
					{
						// Remove this message and reply
						Remove((struct Node *)msg);
						IPC_Reply(msg);
					}
				}

				// Move to previous message
				msg = pred;
			}

			// Finished with port
			Permit();

			// Clear signal
			SetSignal(0, SIGBREAKF_CTRL_C);
		}
	}

	// Close timer
	FreeTimer(timer);

	// Do we have something locked?
	if (our_lock)
	{
		// Restore original path
		CurrentDir(old);

		// Unlock our lock
		UnLock(our_lock);
	}

	// Send goodbye message
	IPC_Goodbye(ipc, &main_ipc, 0);

	// Exit
	IPC_Free(ipc);
}
