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
#include "replace.h"

// Check that source and destination paths are different
short function_check_same_path(char *source_path, char *dest_path)
{
	BPTR lock1, lock2;
	short ret;

	// Compare the strings first
	if (stricmp(source_path, dest_path) == 0)
		return LOCK_SAME;

	// Lock paths
	if (!(lock1 = Lock(source_path, ACCESS_READ)) || !(lock2 = Lock(dest_path, ACCESS_READ)))
	{
		UnLock(lock1);
		return LOCK_DIFFERENT;
	}

	// Compare locks
	ret = SameLock(lock1, lock2);

	// Unlock locks
	UnLock(lock1);
	UnLock(lock2);

	// Return success code
	return ret;
}

// Check that a destination file is ok to write to
int check_file_destination(FunctionHandle *handle, FunctionEntry *entry, char *destination, short *confirm_flags)
{
	BPTR lock, dest_lock = 0;
	short confirm;
	char name[35];

	// Get confirm flags
	confirm = *confirm_flags;

	// If entry is an icon, it's ok
	if (entry->flags & FUNCENTF_ICON)
		return 1;

	// If destination doesn't exist it's ok
	if (!(lock = Lock(destination, ACCESS_READ)))
		return 1;

	// Get destination info
	Examine(lock, handle->d_info);

	// Get parent directory if we need it
	if (handle->d_info->fib_DirEntryType < 0 && entry->type < 0 && !(confirm & COPYF_SKIP_ALL) &&
		!(confirm & COPYF_DELETE_ALL))
		dest_lock = ParentDir(lock);

	// Unlock lock
	UnLock(lock);

	// Get truncated filename
	get_trunc_filename(FilePart(destination), name);

	// If destination is a directory
	if (handle->d_info->fib_DirEntryType > 0)
	{
		// If source is also a directory, that's fine, we can return
		if (entry->type > 0)
			return 1;

		// Can't copy a file over a directory
		lsprintf(handle->work_buffer, GetString(&locale, MSG_ERROR_DEST_IS_DIRECTORY), (IPTR)name);

		// Put up requester
		if (function_request(
				handle, handle->work_buffer, 0, GetString(&locale, MSG_SKIP), GetString(&locale, MSG_ABORT), 0))
			return 0;
		return -1;
	}

	// Destination must be a file; is source a directory?
	if (entry->type > 0)
	{
		int ret;

		// Build requester text
		lsprintf(handle->work_buffer, GetString(&locale, MSG_ERROR_DEST_IS_FILE), name);

		// Put up requester
		if ((ret = function_request(handle,
									handle->work_buffer,
									0,
									GetString(&locale, MSG_SKIP),
									GetString(&locale, MSG_REPLACE),
									GetString(&locale, MSG_ABORT),
									0)) == 0)
			return -1;

		// Skip?
		if (ret == 1)
			return 0;
	}

	// Skip all?
	else if (confirm & COPYF_SKIP_ALL)
		return 0;

	// Do we need to ask?
	else if (!(confirm & COPYF_DELETE_ALL))
	{
		short ret;
		PathNode *path;
		struct Window *win = 0;
		BPTR lock, source_lock;

		// Build source name
		function_build_source(handle, entry, handle->work_buffer);

		// Get source information
		if (!(lock = Lock(handle->work_buffer, ACCESS_READ)))
		{
			UnLock(dest_lock);
			return 0;
		}

		// Get information
		Examine(lock, handle->s_info);

		// Get parent dir
		source_lock = ParentDir(lock);
		UnLock(lock);

		// Get window
		if ((path = function_path_current(&handle->source_paths)) && path->lister)
			win = path->lister->window;

		// Show requester
		ret = SmartAskReplace(win,
							  GUI->screen_pointer,
							  handle->ipc,
							  dest_lock,
							  handle->d_info,
							  source_lock,
							  handle->s_info,
							  handle->replace_option);

		// Unlock locks
		UnLock(source_lock);
		UnLock(dest_lock);

		// Abort?
		if (ret == REPLACE_ABORT)
			return -1;

		// Remember setting
		handle->replace_option = ret & ~REPLACE_ALL;

		// Skip
		if (ret == REPLACE_LEAVE)
			return 0;

		// Skip all?
		if (ret == (REPLACE_LEAVE | REPLACE_ALL))
		{
			*confirm_flags |= COPYF_SKIP_ALL;
			return 0;
		}

		// Replace all?
		else if (ret == (REPLACE_REPLACE | REPLACE_ALL))
			*confirm_flags |= COPYF_DELETE_ALL;
	}

	// Is destination file delete protected?
	if (handle->d_info->fib_Protection & FIBF_DELETE)
	{
		// Unprotect all not set?
		if (!(confirm & COPYF_UNPROTECT_ALL))
		{
			short ret;

			// Build requester text
			lsprintf(handle->work_buffer, GetString(&locale, MSG_DELETE_PROTECTED), name);

			// Put up requester
			if ((ret = function_request(handle,
										handle->work_buffer,
										0,
										GetString(&locale, MSG_UNPROTECT),
										GetString(&locale, MSG_UNPROTECT_ALL),
										GetString(&locale, MSG_ABORT),
										GetString(&locale, MSG_SKIP),
										0)) == 3)
				return -1;

			// Skip?
			if (ret == 0)
				return 0;

			// Unprotect all?
			if (ret == 2)
				*confirm_flags |= COPYF_UNPROTECT_ALL;
		}

		// Clear delete bit
		SetProtection(destination, handle->d_info->fib_Protection & (~FIBF_DELETE));
	}

	// Delete destination file
	while (!(DeleteFile(destination)))
	{
		short ret;

		// Display error requester
		if ((ret = function_error(handle, handle->d_info->fib_FileName, MSG_DELETING, IoErr())) != 1)
			return ret;
	}

	// Okay to proceed
	return 1;
}

// Handle an error
int function_error(FunctionHandle *handle, char *entry_name, int action_msg, int error_code)
{
	BOOL skip = 1;
	char name[35];
	int ret;

	// Get truncated file name
	get_trunc_filename(entry_name, name);

	// If error code is negative it indicates we don't want a "skip" button
	if (error_code < 0)
	{
		error_code = -error_code;
		skip = 0;
	}

	// Get error message
	if (error_code > 0)
	{
		Fault(error_code, GetString(&locale, MSG_DOS_ERROR), handle->work_buffer, 60);
		lsprintf(handle->work_buffer + 768, handle->work_buffer, error_code);
	}
	else
		strcpy(handle->work_buffer + 768, GetString(&locale, MSG_ERROR_UNKNOWN_CODE));

	// Build requester text
	lsprintf(handle->work_buffer,
			 GetString(&locale, MSG_ERROR_OCCURED),
			 GetString(&locale, action_msg),
			 name,
			 handle->work_buffer + 768);

	// Display requester
	ret = function_request(handle,
						   handle->work_buffer,
						   0,
						   GetString(&locale, MSG_TRY_AGAIN),
						   (skip) ? GetString(&locale, MSG_SKIP) : (unsigned char *)-1,
						   GetString(&locale, MSG_ABORT),
						   0);

	// Return appropriate value
	if (ret == 0)
		return -1;
	if (ret == 2)
		return 0;
	return ret;
}

// See if there's any directories in the entry list
BOOL function_check_dirs(FunctionHandle *handle)
{
	FunctionEntry *entry;

	// Go through entry list
	for (entry = (FunctionEntry *)handle->entry_list.lh_Head; entry->node.mln_Succ;
		 entry = (FunctionEntry *)entry->node.mln_Succ)
	{
		// Is this a directory?
		if (entry->type > 0)
			return 1;
	}

	return 0;
}

// Handle abort for a function
void function_abort(FunctionHandle *handle)
{
	PathNode *node;

	// Get current lister
	if ((node = handle->source_paths.current) && node->node.mln_Succ)
	{
		// Display abort text
		status_text_side(node->lister, function_path_side(node), GetString(&locale, MSG_ABORTED));

		// Set flag to save title
		node->flags |= LISTNF_NO_TITLE;
	}
}

// Display an error message in the title bar of a lister
void function_error_text(FunctionHandle *handle, int code)
{
	PathNode *node;

	// Get current lister
	if ((node = handle->source_paths.current) && node->node.mln_Succ)
	{
		// Display abort text
		status_display_error_side(node->lister, function_path_side(node), code);

		// Set flag to save title
		node->flags |= LISTNF_NO_TITLE;
	}
}

// Display text in the title bar
void function_text(FunctionHandle *handle, char *text)
{
	PathNode *node;

	// Get current lister
	if ((node = handle->source_paths.current) && node->node.mln_Succ)
	{
		// Display text
		status_text_side(node->lister, function_path_side(node), text);

		// Set flag to save title
		node->flags |= LISTNF_NO_TITLE;
	}
}

// Display a requester for a function
int function_request_args(FunctionHandle *handle, char *message, IPTR *flags)
{
	PathNode *path;
	APTR parent;
	short ret;

	// Get window
	if ((path = function_path_current(&handle->source_paths)) && path->lister)
		parent = path->lister->window;

	// Use screen
	else
	{
		parent = GUI->window;
		// flags|=SRF_MOUSE_POS;
		flags[0] |= SRF_MOUSE_POS;
	}

	// Do requester
	ret = super_request(parent, message, handle->ipc, flags);

	// Let lister update itself
	if (path && path->lister)
		IPC_Command(path->lister->ipc, LISTER_CHECK_REFRESH, 0, 0, 0, REPLY_NO_PORT);

	return ret;
}

// Cleanup after a function
void function_cleanup(FunctionHandle *handle, PathNode *node, BOOL full)
{
	// Valid node?
	if (node)
	{
		FunctionEntry *entry;
		DirBuffer *buffer = 0;
		Lister *lister;

		// Get lister
		if ((lister = node->lister))
		{
			/* If the lister that a xad lister is started from with
			 * a double click is closed, we get a corrupt buffer
			 * when the xad lister is closed. This pointer check
			 * is a workaround to prevent illegal memory access.
			 */
			if ((IPTR)lister->cur_buffer <= 0L)
				return;

			// Get source buffer and lock it
			buffer = function_path_buffer(node);
			buffer_lock(buffer, TRUE);
		}

		// Go through entry list
		for (entry = (FunctionEntry *)handle->entry_list.lh_Head; entry->node.mln_Succ;)
		{
			FunctionEntry *next = (FunctionEntry *)entry->node.mln_Succ;

			// Does entry need to be removed?
			if (entry->flags & FUNCENTF_REMOVE)
			{
				// Add change for remove
				function_filechange_delfile(handle, node->path, entry->name, 0, 0);

				// Remove entry
				Remove((struct Node *)entry);
			}

			// Deselected?
			else if (entry->flags & FUNCENTF_UNSELECT)
			{
				// Valid buffer and entry?
				if (buffer && entry->entry)
				{
					// Icon?
					if (entry->flags & FUNCENTF_ICON_ACTION)
					{
						// Deselect icon
						((BackdropObject *)entry->entry)->state = 0;

						// If icon is borderless, show it immediately
						if (!backdrop_icon_border((BackdropObject *)entry->entry))
						{
							// Erase it
							backdrop_erase_icon(lister->backdrop_info, (BackdropObject *)entry->entry, 0);
						}

						// Fix selection count
						backdrop_fix_count(lister->backdrop_info, 0);
					}

					// Normal file
					else if (entry->entry->de_Flags & ENTF_SELECTED)
						deselect_entry(buffer, entry->entry);
				}

				// Clear flag
				entry->flags &= ~FUNCENTF_UNSELECT;
			}

			// Get next
			entry = next;
		}

		// Unlock buffer
		if (buffer)
			buffer_unlock(buffer);

		// Got a lister?
		if (lister)
		{
			// Full cleanup?
			if (full)
			{
				// Do any other changes
				function_perform_changes(handle, node);

				// Turn progress indicator off
				IPC_Command(lister->ipc, LISTER_PROGRESS_OFF, 0, 0, 0, 0);

				// Was it the main progress indicator?
				if (handle->progress_lister == lister)
					handle->progress_lister = 0;
			}
		}
	}
}

// Do changes to listers
void function_do_lister_changes(FunctionHandle *handle, PathList *list)
{
	PathNode *lister;

	// Go through listers
	for (lister = (PathNode *)list->list.mlh_Head; lister->node.mln_Succ; lister = (PathNode *)lister->node.mln_Succ)
	{
		// Do changes for this lister
		function_cleanup(handle, lister, 1);
	}
}

// Do changes to a lister
void function_perform_changes(FunctionHandle *handle, PathNode *path)
{
	ULONG side;

	side = (path->flags & LISTNF_DUAL_SIDE) ? path->dual_side + 1 : 0;

	// Rescan directory?
	if (path->flags & LISTNF_RESCAN && path->lister)
	{
		// Rescan
		if (path->lister)
			IPC_Command(path->lister->ipc, LISTER_RESCAN, side, 0, 0, 0);

		// Clear flag
		path->flags &= ~LISTNF_RESCAN;
	}

	// Update datestamp?
	else if (path->flags & LISTNF_UPDATE_STAMP)
	{
		// Valid lister?
		if (path->lister)
			IPC_Command(path->lister->ipc, LISTER_UPDATE_STAMP, side, 0, 0, 0);

		// Otherwise, do global lister update
		else
			update_lister_global(path->path);

		// Clear flag
		path->flags &= ~LISTNF_UPDATE_STAMP;
	}
}

// Build information string
void function_build_info(FunctionHandle *handle, char *src, char *dst, short which)
{
	PathNode *source = 0, *dest = 0;

	// Get current source and destination
	if (!src)
		source = function_path_current(&handle->source_paths);
	if (!dst && (which & 2))
		dest = function_path_current(&handle->dest_paths);

	// If no dest, use source
	if (!dst && !dest && which & 2)
	{
		if (!(dest = source))
			dst = src;
	}

	// Get string pointers
	if (!src && source)
		src = source->path;
	if (!dst && dest)
		dst = dest->path;

	// Get paths
	if (src)
		final_path(src, handle->work_buffer + 256);
	if (dst)
		final_path(dst, handle->work_buffer + 384);

	// Destination?
	if (which & 2)
	{
		// Build information string
		lsprintf(
			handle->work_buffer, GetString(&locale, MSG_FROM_TO), handle->work_buffer + 256, handle->work_buffer + 384);
	}

	// Only source
	else
	{
		// Build information string
		lsprintf(handle->work_buffer, GetString(&locale, MSG_FROM), handle->work_buffer + 256);
	}

	// Set information string
	function_progress_info(handle, handle->work_buffer);
}

void get_trunc_filename(char *source, char *dest)
{
	if (!source)
		*dest = 0;
	else
	{
		stccpy(dest, source, 31);
		if (strlen(dest) < strlen(source))
			strcpy(dest + 27, "...");
	}
}

void get_trunc_path(char *source, char *dest)
{
	if (!source)
		*dest = 0;
	else
	{
		short pos, len;
		for (pos = 0, len = 0; source[pos]; pos++)
		{
			if (source[pos] == ':' || source[pos] == '/')
			{
				*dest++ = source[pos];
				len = 0;
			}
			else if (len == 30)
			{
				*(dest - 2) = '.';
				*(dest - 1) = '.';
				*dest++ = '.';
				len = 31;
			}
			else if (len < 30)
			{
				*dest++ = source[pos];
				++len;
			}
		}
		*dest = 0;
	}
}
