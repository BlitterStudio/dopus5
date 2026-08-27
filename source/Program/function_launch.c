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

IPC_EntryProto(function_launch_code, static);

BOOL function_launch_quick(ULONG command, Cfg_Function *function, Lister *source_list)
{
	return function_launch(command, function, 0, 0, source_list, 0, 0, 0, 0, 0, 0);
}

// Launches a function process
BOOL function_launch(ULONG command,
					 Cfg_Function *function,
					 ULONG data,
					 ULONG flags,
					 Lister *source_list,
					 Lister *dest_list,
					 char *source_path,
					 char *dest_path,
					 struct ArgArray *args,
					 struct Message *done_msg,
					 Buttons *buttons)
{
	FunctionHandle *handle;
	IPCData *ipc;
	char *dual_dest_path = 0;
	short dual_dest_side = -1;

	// Allocate handle
	if (!(handle = function_new_handle(0, 0)))
	{
		// Free handle
		if (handle)
			handle->function = function;
		function_free(handle);
		return 0;
	}

	// Valid function?
	if (function)
	{
		// If it's not already ours to play with, copy it
		if (!(function->function.flags2 & FUNCF2_FREE_FUNCTION))
		{
			Cfg_Function *copy;

			// Copy function
			if ((copy = CopyFunction(function, handle->memory, 0)))
				function = copy;
		}
	}

	// Initialise handle
	handle->function = function;
	handle->command = command;
	handle->data = data;
	handle->flags = flags;

	// AppMessage?
	if (flags & FUNCF_DRAG_DROP)
	{
		// Get AppMessage
		if ((handle->app_msg = (DOpusAppMessage *)buttons))
		{
			// Free the locks in the message
			UnlockWBArg(handle->app_msg->da_Msg.am_ArgList, handle->app_msg->da_Msg.am_NumArgs);
		}
	}

	// Filetypes?
	else if (command == FUNCTION_FILETYPE)
		handle->arg_passthru = (struct ArgArray *)buttons;

	// Buttons
	else
		handle->buttons = buttons;

	// Source path
	if (source_path)
	{
		strcpy(handle->source_path, source_path);
		if (source_path[0])
			AddPart(handle->source_path, "", 512);
	}

	// Dual lister functions use the inactive side as destination.
	if (source_list && !dest_list && !dest_path && (dual_dest_path = lister_dual_dest_path(source_list)) &&
		dual_dest_path[0])
	{
		dest_path = dual_dest_path;
		dest_list = source_list;
		dual_dest_side = lister_dual_dest_index(source_list);
	}

	// Destination path
	if (dest_path)
	{
		// Fake destination path
		if (dest_path[0] == ':' && dest_path[1] == ':' && dest_path[2] == 0)
		{
			// Set flag to ask for path
			handle->flags |= FUNCF_ASK_DEST;
			dest_path = 0;
		}

		// Copy path
		else
		{
			strcpy(handle->dest_path, dest_path);
			if (dest_path[0])
				AddPart(handle->dest_path, "", 512);
			else
				dest_path = 0;
		}
	}

	// Listers
	handle->source_lister = source_list;
	handle->dest_lister = dest_list;

	// Add initial source and destination listers to the lists
	if (handle->source_lister || source_path)
	{
		// Add source path
		function_add_path(handle, &handle->source_paths, handle->source_lister, handle->source_path);
	}

	if (handle->dest_lister || dest_path)
	{
		PathNode *path;

		// Add destination path
		if ((path = function_add_path(handle, &handle->dest_paths, handle->dest_lister, handle->dest_path)))
		{
			// Dual lister destination paths target the inactive panel.
			if (dual_dest_side >= 0)
			{
				path->dual_side = dual_dest_side;
				path->flags |= LISTNF_DUAL_SIDE;
			}

			// Is this a drag'n'drop to a lister?
			if (handle->dest_lister && (data == FTTYPE_DRAG_DROP || data == FTTYPE_SHIFT_DRAGDROP ||
										data == FTTYPE_CTRL_DRAGDROP || data == FTTYPE_ALT_DRAGDROP))
			{
				short len;

				// Get lister path length
				len = strlen(handle->dest_lister->cur_buffer->buf_Path);

				// To a directory inside the lister?
				if (strncmp(handle->dest_lister->cur_buffer->buf_Path, handle->dest_path, len) == 0 &&
					handle->dest_path[len])
				{
					// Mark node as having a changed path
					path->flags |= LISTNF_CHANGED;
				}
			}
		}
	}

	// Store arguments
	handle->args = args;

	// Done message
	handle->done_msg = done_msg;

	// Launch process
	if (!(IPC_Launch(&GUI->function_list,
					 &ipc,
					 "dopus_function",
					 (IPTR)&function_launch_code,
					 STACK_DEFAULT,
					 (IPTR)handle,
					 (struct Library *)DOSBase)))
	{
		if (!ipc)
			function_free(handle);
		return 0;
	}

	// Process is now on its own
	return 1;
}

// Allocate a new handle
FunctionHandle *function_new_handle(struct MsgPort *port, BOOL small)
{
	FunctionHandle *handle;

	// Allocate handle
	if (!(handle = AllocVec(sizeof(FunctionHandle), MEMF_CLEAR)) ||
		!(handle->memory = NewMemHandle(2048, 1024, MEMF_CLEAR)))
	{
		if (handle)
			FreeVec(handle);
		return 0;
	}

	// Not small?
	if (!small)
	{
		// Allocate other things
		if (!(handle->entry_memory = NewMemHandle(2048, sizeof(FunctionEntry) + 256, MEMF_CLEAR)) ||
			!(handle->anchor = AllocMemH(handle->memory, sizeof(struct AnchorPath) + 256)) ||
			!(handle->filereq = AllocAslRequest(ASL_FileRequest, 0)) ||
			!(handle->s_info = AllocDosObject(DOS_FIB, 0)) || !(handle->d_info = AllocDosObject(DOS_FIB, 0)) ||
			!(handle->recurse_entry_data = AllocMemH(handle->memory, sizeof(FunctionEntry))))
		{
			if (handle->entry_memory)
				FreeMemHandle(handle->entry_memory);
			if (handle->filereq)
				FreeAslRequest(handle->filereq);
			if (handle->s_info)
				FreeDosObject(DOS_FIB, handle->s_info);
			if (handle->d_info)
				FreeDosObject(DOS_FIB, handle->d_info);
			FreeMemHandle(handle->memory);
			FreeVec(handle);
			return 0;
		}
	}

	// Get current requester coordinates
	GetSemaphore(&GUI->req_lock, SEMF_SHARED, 0);
	handle->coords = GUI->req_coords;
	FreeSemaphore(&GUI->req_lock);

	// Initialise paths
	NewList((struct List *)&handle->source_paths.list);
	handle->source_paths.current = 0;
	NewList((struct List *)&handle->dest_paths.list);
	handle->dest_paths.current = 0;

	// Initialise handle
	function_handle_init(handle, 0);
	NewList((struct List *)&handle->external_list);
	strcpy(handle->file_filter, "#?");

	// Install reply port
	handle->reply_port = port;
	return handle;
}

// Initialise a function handle
void function_handle_init(FunctionHandle *handle, BOOL clear)
{
	// Clear memory handles
	if (clear)
		ClearMemHandle(handle->entry_memory);

	// Initialise function
	handle->func_flags = 0;
	handle->func_current_arg = 0;
	handle->func_iterations = 0;
	handle->func_source_num = 0;
	handle->instruction_flags = 0;

	// Initialise recursive stuff
	handle->recurse_return = -1;
	handle->anchor_path = handle->anchor->ap_Buf;

	// Initialise lists
	NewList(&handle->entry_list);
	NewList(&handle->dest_entry_list);
	NewList((struct List *)&handle->func_instructions);
	NewList(&handle->func_arguments);
	NewList(&handle->recurse_list);
	handle->current_entry = 0;
	handle->dest_current_entry = 0;
	handle->dest_entry_count = 0;
	handle->dest_entry_path = 0;
	handle->dest_last_filename[0] = 0;
	NewList(&handle->filechange);

	// Script file stuff
	handle->script_file = 0;

	// Do we have a source list?
	if (!(IsListEmpty((struct List *)&handle->source_paths)))
		handle->func_flags |= FUNCF_GOT_SOURCE;

	// Do we have a destination list?
	if (!(IsListEmpty((struct List *)&handle->dest_paths)))
		handle->func_flags |= FUNCF_GOT_DEST;
}

// Free a function handle
void function_free(FunctionHandle *handle)
{
	// Valid handle?
	if (handle)
	{
		// Free dos objects
		FreeDosObject(DOS_FIB, handle->s_info);
		FreeDosObject(DOS_FIB, handle->d_info);

		// Free function?
		if (handle->function && handle->function->function.flags2 & FUNCF2_FREE_FUNCTION)
			FreeFunction(handle->function);

		// Free file requester
		FreeAslRequest(handle->filereq);

		// AppMessage
		FreeAppMessage(handle->app_msg);

		// Arguments
		if (handle->args)
			FreeArgArray(handle->args);
		if (handle->arg_passthru)
			FreeArgArray(handle->arg_passthru);

		// Free handle memory
		FreeMemHandle(handle->memory);
		FreeMemHandle(handle->entry_memory);
		FreeVec(handle);
	}
}

// Initialise a function
IPC_StartupCode(function_init, FunctionHandle *, handle, static)
{
	// Store IPC pointer
	handle->ipc = ipc;
	SET_IPCDATA(ipc, handle);

	// Create message port
	if (!(handle->reply_port = CreateMsgPort()))
		return 0;
	handle->reply_port->mp_Node.ln_Pri =
		PORT_ASYNC_MAGIC;  // original dopus5code:	handle->reply_port->mp_Flags|=PF_ASYNC;

	return 1;
}

// Code that runs a function process
IPC_EntryCode(function_launch_code, static)
{
	FunctionHandle *handle;
	IPCData *ipc;

	// Get startup message
	if (!(ipc = IPC_ProcStartup((IPTR *)&handle, (ULONG (*)(IPCData *, APTR))&function_init)))
	{
		function_free(handle);
		return;
	}

	// Look at command
	switch (handle->command)
	{
	// Read a directory
	case FUNCTION_READ_DIRECTORY:
		if (function_lock_paths(handle, &handle->source_paths, 3))
		{
			Lister *source_list;

			// Get source list
			if ((source_list = function_lister_current(&handle->source_paths)))
			{
				// Read directory
				function_read_directory(handle, source_list, handle->source_path);

				// If the lister is in icon mode, we don't refresh it
				if (source_list->flags & (LISTERF_VIEW_ICONS | LISTERF_ICON_ACTION))
				{
					// Set 'no refresh' flag
					((PathNode *)handle->source_paths.list.mlh_Head)->flags |= LISTNF_NO_REFRESH;
				}

				// Clear 'first time' flag
				source_list->flags2 &= ~LISTERF2_UNAVAILABLE;
			}
		}
		break;

	// Get icons
	case FUNCTION_GET_ICONS:
		if (function_lock_paths(handle, &handle->source_paths, 2))
		{
			Lister *source_list;

			// Get source list
			if ((source_list = function_lister_current(&handle->source_paths)))
			{
				// Get icons
				lister_get_icons(handle, source_list, 0, 0);

				// Set 'no refresh' flag
				((PathNode *)handle->source_paths.list.mlh_Head)->flags |= LISTNF_NO_REFRESH;
			}
		}
		break;

	// Run a function with external arguments
	case FUNCTION_RUN_FUNCTION_EXTERNAL:

		// Convert arguments to external file list
		function_files_from_args(handle);

	// Run a function
	case FUNCTION_RUN_FUNCTION:
		function_run_function(handle);
		break;

	// Do a filetype on a function
	case FUNCTION_FILETYPE:
		function_lock_paths(handle, &handle->source_paths, 1);
		function_lock_paths(handle, &handle->dest_paths, 1);
		function_filetype(handle);
		break;
	}

	// Do any file changes
	function_filechange_do(handle, 0);

	// Unlock listers
	function_unlock_paths(handle, &handle->source_paths, 1);
	function_unlock_paths(handle, &handle->dest_paths, 0);

	// Update desktop?
	if (handle->flags & FUNCF_RESCAN_DESKTOP)
	{
		// Update the desktop folder
		misc_startup("dopus_desktop_update", MENU_UPDATE_DESKTOP, GUI->window, 0, TRUE);
	}

	// Send goodbye message
	IPC_Goodbye(ipc, &main_ipc, WINDOW_FUNCTION);

	// Done message?
	if (handle->done_msg)
	{
		handle->done_msg->mn_Node.ln_Pri = handle->ret_code;
		PutMsg(handle->done_msg->mn_ReplyPort, handle->done_msg);
	}

	// Free reply port
	DeleteMsgPort(handle->reply_port);

	// Free data
	function_free(handle);

	// Cleanup process
	IPC_Free(ipc);
}

// Check for abort
BOOL function_check_abort(FunctionHandle *handle)
{
	IPCMessage *msg;
	BOOL abort = 0;
	PathNode *node;

	// Valid port?
	if (!handle || !handle->ipc->command_port)
		return 0;

	// Any messages?
	while ((msg = (IPCMessage *)GetMsg(handle->ipc->command_port)))
	{
		// Abort?
		if (msg->command == IPC_ABORT)
		{
			abort = 1;
		}

		// Activate?
		else if (msg->command == IPC_ACTIVATE)
			handle->flags2 |= FUNCF_ACTIVATE_ME;

		// Remove a lister?
		else if (msg->command == IPC_REMOVE)
		{
			Lister *lister;

			// Get lister
			if ((lister = (Lister *)msg->data))
			{
				// Go through lists and see if lister in there
				for (node = (PathNode *)handle->source_paths.list.mlh_Head; node->node.mln_Succ;
					 node = (PathNode *)node->node.mln_Succ)
				{
					// Match lister?
					if (node->lister == lister)
					{
						// Remove this node, and clear lister pointer
						Remove((struct Node *)node);
						node->lister = 0;
						break;
					}
				}

				// Go through lists and see if lister in there
				for (node = (PathNode *)handle->dest_paths.list.mlh_Head; node->node.mln_Succ;
					 node = (PathNode *)node->node.mln_Succ)
				{
					// Match lister?
					if (node->lister == lister)
					{
						// Remove this node, and clear lister pointer
						Remove((struct Node *)node);
						node->lister = 0;
						break;
					}
				}
			}
		}

		// Reply message
		IPC_Reply(msg);
	}

	// Got it already?
	if (abort)
		return 1;

	// Go through lists and see if the abort flag is set
	for (node = (PathNode *)handle->source_paths.list.mlh_Head; node->node.mln_Succ;
		 node = (PathNode *)node->node.mln_Succ)
	{
		// Valid lister?
		if (node->lister && (node->lister->flags & LISTERF_ABORTED))
			return 1;
	}

	// Go through lists and see if the abort flag is set
	for (node = (PathNode *)handle->dest_paths.list.mlh_Head; node->node.mln_Succ;
		 node = (PathNode *)node->node.mln_Succ)
	{
		// Valid lister?
		if (node->lister && (node->lister->flags & LISTERF_ABORTED))
			return 1;
	}

	return abort;
}

// Check that a lister is valid for a function (list must be locked)
Lister *function_valid_lister(FunctionHandle *handle, Lister *lister)
{
	struct MinNode *node;

	// If no lister needed, return ok
	if (!lister)
		return 0;

	// Go through lister list
	for (node = (struct MinNode *)GUI->lister_list.list.lh_Head; node->mln_Succ; node = node->mln_Succ)
	{
		// Is this the lister we want?
		if (lister == (Lister *)IPCDATA(((IPCData *)node)))
		{
			// Check it's not closing
			if (lister->flags & LISTERF_CLOSING)
				return 0;

			// Showing special buffer?
			if ((lister->cur_buffer == lister->special_buffer) && !(handle->func_flags & FUNCF_SPECIAL_OK))
			{
				// Can only read a directory or get icons in this case
				if (handle->command != FUNCTION_READ_DIRECTORY && handle->command != FUNCTION_GET_ICONS)
					return 0;
			}

			// Make sure it's not busy
			if (lister->flags & LISTERF_BUSY)
			{
				// Are we not the ones making it busy?
				if (lister->locker_ipc != handle->ipc)
					return 0;
			}

			// Ok to use
			return lister;
		}
	}

	// Couldn't find it
	return 0;
}

// Lock the paths for a function
// swapped result status around (5th may)
BOOL function_lock_paths(FunctionHandle *handle, PathList *list, int locker)
{
	BOOL result = 1;
	PathNode *node;

	// Lock lister list
	lock_listlock(&GUI->lister_list, FALSE);

	// Go through list of paths
	for (node = (PathNode *)list->list.mlh_Head; node->node.mln_Succ;)
	{
		PathNode *next = (PathNode *)node->node.mln_Succ;

		// Check path not already locked
		if (node->lister && !(node->flags & LISTNF_LOCKED))
		{
			Lister *lister;

			function_apply_path_side(node);

			// Validate the lister before dereferencing it. The stored pointer may be
			// stale if the lister was closed while this function was pending; the
			// shared lock on GUI->lister_list keeps a validated lister alive until we
			// either lock it busy or release the list lock.
			lister = function_valid_lister(handle, node->lister);

			// Lister is gone (freed or closing)
			if (!lister)
			{
				node->lister = 0;
			}

			// Lister not available first time unless this is a read dir op
			else if (lister->flags2 & LISTERF2_UNAVAILABLE && locker != 3)
			{
				// Clear lister
				node->lister = 0;
			}

			// No lister sources?
			else if (handle->flags & FUNCF_NO_SOURCE)
			{
				// Clear lister
				node->lister = 0;
			}

			// Lister is valid, lock it
			else
			{
				IPCMessage *msg;

				node->lister = lister;

				// Already locked through another path node in this function.
				if ((lister->flags & LISTERF_BUSY) && lister->locker_ipc == handle->ipc)
				{
					node->flags |= LISTNF_LOCKED | LISTNF_SHARED_LOCK;
				}

				// Make lister busy
				else
				{
					IPC_Command(node->lister->ipc, LISTER_BUSY, 0, (APTR)1, 0, handle->reply_port);

					// Unlock lister list
					unlock_listlock(&GUI->lister_list);

					// Wait for a reply
					WaitPort(handle->reply_port);
					msg = (IPCMessage *)GetMsg(handle->reply_port);

					// Ok to use lister?
					if (msg->command != IPC_ABORT)
					{
						// Set lister locker
						if (locker)
						{
							IPC_Command(node->lister->ipc, LISTER_SET_LOCKER, 0, handle->ipc, 0, REPLY_NO_PORT);
						}

						// Set lock flag
						node->flags |= LISTNF_LOCKED;
					}

					// Don't use lister
					else
					{
						node->lister = 0;
						result = 0;
					}

					// Free reply
					IPC_Reply(msg);

					// Lock lister list again
					lock_listlock(&GUI->lister_list, FALSE);

					// Start again from start of list
					next = (PathNode *)list->list.mlh_Head;
				}
			}
		}

		// Get next
		node = next;
	}

	// Unlock lister list
	unlock_listlock(&GUI->lister_list);

	return result;
}

// Unlock source and destination paths
void function_unlock_paths(FunctionHandle *handle, PathList *list, int locker)
{
	PathNode *node;

	// Lock lister list
	lock_listlock(&GUI->lister_list, FALSE);

	// Go through list of listers
	for (node = (PathNode *)list->list.mlh_Head; node->node.mln_Succ; node = (PathNode *)node->node.mln_Succ)
	{
		// Valid lister?
		if (node->lister)
		{
			struct MinNode *lnode;
			BOOL still_listed = FALSE;

			// The stored lister pointer may be stale if the lister was closed while
			// we held it busy. Confirm it is still in GUI->lister_list before any
			// dereference; the list lock keeps a found lister alive until we release.
			for (lnode = (struct MinNode *)GUI->lister_list.list.lh_Head;
				 lnode->mln_Succ;
				 lnode = lnode->mln_Succ)
			{
				if (node->lister == (Lister *)IPCDATA(((IPCData *)lnode)))
				{
					still_listed = TRUE;
					break;
				}
			}

			if (!still_listed)
			{
				node->flags &= ~LISTNF_LOCKED;
				node->lister = 0;
			}

			// Check list is locked by us
			else if (node->flags & LISTNF_LOCKED)
			{
				ULONG ref;
				ULONG side;
				BOOL shared;

				// Clear lock flag
				shared = (node->flags & LISTNF_SHARED_LOCK) ? TRUE : FALSE;
				node->flags &= ~(LISTNF_LOCKED | LISTNF_SHARED_LOCK);
				side = (node->flags & LISTNF_DUAL_SIDE) ? node->dual_side + 1 : 0;

				if (!shared)
				{
					// Clear locker
					if (locker)
						IPC_Command(node->lister->ipc, LISTER_SET_LOCKER, 0, 0, 0, REPLY_NO_PORT);

					// Clear busy
					IPC_Command(node->lister->ipc, LISTER_BUSY, 0, (APTR)0, 0, (locker) ? REPLY_NO_PORT : 0);
				}

				// Allowed to refresh?
				if (!(node->flags & LISTNF_NO_REFRESH))
				{
					// Refresh flags
					ref = REFRESHF_SLIDERS;
					if (handle->result_flags & FRESULTF_RESORT)
						ref |= REFRESHF_RESORT;
					if (node->lister->flags & LISTERF_VIEW_ICONS)
						ref |= REFRESHF_ICONS;
					if (!(node->flags & LISTNF_NO_TITLE))
						ref |= REFRESHF_UPDATE_NAME | REFRESHF_STATUS;

					// Refresh lister
					IPC_Command(node->lister->ipc, LISTER_REFRESH_WINDOW, ref, (APTR)(IPTR)side, 0, 0);

					// Update title
					IPC_Command(node->lister->ipc, LISTER_SHOW_INFO, side, 0, 0, 0);
				}

				// Set flag to indicate deferred title update
				if (node->flags & LISTNF_NO_TITLE)
					node->lister->flags |= LISTERF_SHOW_TITLE;
			}
		}
	}

	// Unlock lister list
	unlock_listlock(&GUI->lister_list);
}

// Display progress requester
void function_progress_on(FunctionHandle *handle, char *operation, ULONG total, ULONG flags)
{
	Lister *lister;

	// Get current source lister
	if (!(lister = function_lister_current(&handle->source_paths)))
	{
		// No source, get current destination
		lister = function_lister_current(&handle->dest_paths);
	}

	// Got a lister?
	if (lister)
	{
		ProgressPacket *packet;

		// Allocate packet
		if ((packet = AllocVec(sizeof(ProgressPacket), MEMF_CLEAR)))
		{
			// Remember lister
			handle->progress_lister = lister;

			// Fill out packet
			packet->total = total;
			packet->operation = operation;
			packet->flags = flags;

			// Turn progress requester on
			IPC_Command(lister->ipc, LISTER_PROGRESS_ON, 0, 0, packet, REPLY_NO_PORT);
		}
	}
}

// Update progress requester
BOOL function_progress_update(FunctionHandle *handle, FunctionEntry *entry, ULONG count)
{
	char *name, *file;

	// Got a lister with the progress requester?
	if (handle->progress_lister)
	{
		// Allocate memory for name
		if ((name = AllocMemH(handle->progress_lister->progress_memory, strlen((file = FilePart(entry->name))) + 1)))
		{
			// Copy name
			strcpy(name, file);

			// Send to lister
			IPC_Command(handle->progress_lister->ipc, LISTER_PROGRESS_UPDATE, count, name, 0, 0);
		}
	}

	return function_check_abort(handle);
}

// File progress
void function_progress_file(FunctionHandle *handle, long size, long count)
{
	// Got a lister with the progress requester?
	if (!handle || !handle->progress_lister)
		return;

	// Setting size?
	if (size)
		IPC_Command(handle->progress_lister->ipc, LISTER_FILE_PROGRESS_TOTAL, 0, (APTR)size, 0, 0);

	// Setting count
	else
		IPC_Command(handle->progress_lister->ipc, LISTER_FILE_PROGRESS_UPDATE, 0, (APTR)count, 0, 0);
}

// File progress
void function_progress_file64(FunctionHandle *handle, QUAD *size, QUAD *count)
{
	// Got a lister with the progress requester?
	if (!handle || !handle->progress_lister)
		return;

	// Setting size?
	if (size)
		IPC_Command(handle->progress_lister->ipc, LISTER_FILE_PROGRESS_TOTAL64, 0, (APTR)size, 0, 0);

	// Setting count
	else
		IPC_Command(handle->progress_lister->ipc, LISTER_FILE_PROGRESS_UPDATE64, 0, (APTR)count, 0, 0);
}

// Progress information
void function_progress_info(FunctionHandle *handle, char *info)
{
	// Got a lister with the progress requester?
	if (!handle || !handle->progress_lister)
		return;

	// Set info string
	IPC_Command(handle->progress_lister->ipc, LISTER_FILE_PROGRESS_INFO, 0, info, 0, REPLY_NO_PORT);
}

// Get the paths for a function
Lister *function_get_paths(FunctionHandle *handle, PathList *list, ULONG flags, short one_only)
{
	Lister *first = 0, *lister;
	PathNode *node;
	IPCData *ipc;

	// Is there a path already in the list?
	if (!(IsListEmpty((struct List *)list)))
		first = ((PathNode *)list->list.mlh_Head)->lister;

	// No first lister yet?
	if (!first)
	{
		// Get default first
		if ((first = lister_default(flags, (flags & LISTERF_DEST) ? TRUE : FALSE)))
		{
			// Add to head of the list
			if ((node = AllocMemH(handle->memory, sizeof(PathNode))))
			{
				function_init_path_node(node);
				node->lister = first;
				function_capture_path_side(node);
				AddHead((struct List *)list, (struct Node *)node);
			}
		}
	}

	// Set current pointer
	if (IsListEmpty((struct List *)list))
		list->current = 0;
	else
		list->current = (PathNode *)list->list.mlh_Head;

	// Lock lister list
	lock_listlock(&GUI->lister_list, FALSE);

	// Need more than one?
	if (!one_only)
	{
		// Go through lister list
		for (ipc = (IPCData *)GUI->lister_list.list.lh_Head; ipc->node.mln_Succ; ipc = (IPCData *)ipc->node.mln_Succ)
		{
			// Get lister
			lister = IPCDATA(ipc);

			// Dual lister sides are resolved locally by their launch path.
			if (lister_dual_is_side(lister))
				continue;

			// Is this lister the right type and not busy, or an icon view?
			if (lister->flags & flags && !(lister->flags & LISTERF_BUSY) &&
				(!(lister->flags & LISTERF_VIEW_ICONS) || lister->flags & LISTERF_ICON_ACTION))
			{
				// Not the one we already have?
				if (lister != first)
				{
					// Valid path or custom handler?
					if (lister->cur_buffer->buf_Path[0] || lister->cur_buffer->buf_CustomHandler[0])
					{
						// Allocate lister node
						if ((node = AllocMemH(handle->memory, sizeof(PathNode))))
						{
							function_init_path_node(node);

							// Add to list
							node->lister = lister;
							function_capture_path_side(node);
							AddTail((struct List *)list, (struct Node *)node);
						}
					}
				}
			}
		}
	}

	// Unlock lister list
	unlock_listlock(&GUI->lister_list);

	// Return first pointer
	return first;
}

// Check whether any source lister currently has a selection. This is the
// precedence guard for the desktop selection fallback: when a lister already
// provides entries, the desktop must not be snapshotted.
BOOL lister_has_selection(void)
{
	Lister *lister;
	IPCData *ipc;
	BOOL found = FALSE;

	// Lock lister list
	lock_listlock(&GUI->lister_list, FALSE);

	// Go through lister list
	for (ipc = (IPCData *)GUI->lister_list.list.lh_Head; ipc->node.mln_Succ; ipc = (IPCData *)ipc->node.mln_Succ)
	{
		// Get lister
		lister = IPCDATA(ipc);

		// Dual lister sides are resolved locally by their launch path.
		if (lister_dual_is_side(lister))
			continue;

		// Same candidacy as function_get_paths for a source lister: must be a
		// source, not busy, and an icon view only when in icon action mode.
		if (!(lister->flags & LISTERF_SOURCE) || lister->flags & LISTERF_BUSY ||
			(lister->flags & LISTERF_VIEW_ICONS && !(lister->flags & LISTERF_ICON_ACTION)))
			continue;

		// Icon mode: look for a selected backdrop object
		if (lister->flags & (LISTERF_VIEW_ICONS | LISTERF_ICON_ACTION))
		{
			BackdropInfo *info;
			BackdropObject *object;

			if ((info = lister->backdrop_info))
			{
				// Lock backdrop list
				lock_listlock(&info->objects, FALSE);

				// Go through backdrop list
				for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ;
					 object = (BackdropObject *)object->node.ln_Succ)
				{
					// Selected?
					if (object->state)
					{
						found = TRUE;
						break;
					}
				}

				// Unlock backdrop list
				unlock_listlock(&info->objects);
			}
		}

		// Normal mode: look for a selected buffer entry
		else
		{
			DirBuffer *buffer;
			DirEntry *entry;

			if ((buffer = lister->cur_buffer))
			{
				// Lock buffer
				buffer_lock(buffer, FALSE);

				// Go through buffer
				for (entry = (DirEntry *)buffer->entry_list.mlh_Head; entry->de_Node.dn_Succ;
					 entry = (DirEntry *)entry->de_Node.dn_Succ)
				{
					// Selected?
					if (entry->de_Flags & ENTF_SELECTED)
					{
						found = TRUE;
						break;
					}
				}

				// Unlock buffer
				buffer_unlock(buffer);
			}
		}

		// Found a selection?
		if (found)
			break;
	}

	// Unlock lister list
	unlock_listlock(&GUI->lister_list);

	return found;
}
