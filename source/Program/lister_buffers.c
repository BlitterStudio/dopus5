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

// Allocate a new buffer for a lister - list must be locked
DirBuffer *lister_new_buffer(Lister *lister)
{
	DirBuffer *buffer;

	// Try to allocate new buffer
	if ((buffer = buffer_new()))
	{
		// Add buffer to the list
		AddHead(&GUI->buffer_list.list, &buffer->node);

		// Default settings
		buffer->buf_ListFormat = (lister) ? lister->format : environment->env->list_format;

		// Set owner in lister
		buffer->buf_OwnerLister = lister;

		// Increment buffer count
		++GUI->buffer_count;
	}

	return buffer;
}

// Free a buffer from a lister (list must be locked)
void lister_free_buffer(DirBuffer *buffer)
{
	if (buffer)
	{
		Remove(&buffer->node);
		buffer_free(buffer);
		--GUI->buffer_count;
	}
}

// Searches for a named buffer and moves to it if found
// Called from the LISTER PROCESS (unless LISTER_BFPF_DONT_MOVE is set)
DirBuffer *lister_find_buffer(Lister *lister,
							  DirBuffer *start,
							  char *path,
							  struct DateStamp *stamp,
							  char *volume,
							  ULONG flags)
{
	short ret = 0;
	DirBuffer *buffer;
	char *path_buffer;
	struct FileInfoBlock *fib;

	// Got a lister?
	if (lister)
	{
		path_buffer = lister->work_buffer;
		fib = lister->fib;
	}

	// Need to allocate data
	else
	{
		if (!(path_buffer = AllocVec(512, 0)) || !(fib = AllocDosObject(DOS_FIB, 0)))
		{
			FreeVec(path_buffer);
			return 0;
		}
	}

	// Copy path, terminate correctly
	strcpy(path_buffer, path);
	AddPart(path_buffer, "", 512);

	// Lock buffer list
	if (!(flags & LISTER_BFPF_DONT_LOCK))
		lock_listlock(&GUI->buffer_list, FALSE);

	// Get starting position
	if (start)
		buffer = start;
	else
		buffer = (DirBuffer *)GUI->buffer_list.list.lh_Head;

	// Go through all buffers
	for (; buffer->node.ln_Succ; buffer = (DirBuffer *)buffer->node.ln_Succ)
	{
		// Valid path in this directory?
		if (buffer->flags & DWF_VALID &&
			(flags & LISTER_BFPF_DONT_LOCK || !buffer->buf_CurrentLister || buffer->buf_CurrentLister == lister))
		{
			// Does pathname match?
			if (stricmp(path_buffer, buffer->buf_Path) == 0)
			{
				BOOL match = 1;

				// If datestamp is supplied, try that
				if (stamp)
				{
					if (CompareDates(stamp, &buffer->buf_VolumeDate) != 0)
						match = 0;
				}

				// Or if a volume name is provided, test that
				if (volume)
				{
					if (stricmp(volume, buffer->buf_VolumeLabel) != 0)
						match = 0;
				}

				// Matched?
				if (match)
				{
					BPTR lock;

					// Do we need to test dates?
					if (!(flags & LISTER_BFPF_DONT_TEST))
					{
						// Try to lock and examine this directory
						if ((lock = Lock(path_buffer, ACCESS_READ)))
						{
							// Examine and unlock
							Examine(lock, fib);
							UnLock(lock);

							// If datestamp on directory has changed, don't go to it
							if (CompareDates(&fib->fib_Date, &buffer->buf_DirectoryDate) != 0)
								continue;
						}
						else
							continue;
					}

					ret = 1;
					break;
				}
			}
		}
	}

	// Did we find one?
	if (ret)
	{
		// Go to buffer if requested
		if (!(flags & LISTER_BFPF_DONT_MOVE) && lister)
		{
			// Show buffer in lister
			lister_show_buffer(lister, buffer, 1, 1);

			// See if buffer needs re-reading
			lister_check_old_buffer(lister, 0);
		}
	}
	else
		buffer = 0;

	// Was the list locked?
	if (!(flags & LISTER_BFPF_DONT_LOCK))
	{
		// Allowed to unlock?
		if (!(flags & LISTER_BFPF_DONT_UNLOCK) || !buffer)

			// Unlock buffer list
			unlock_listlock(&GUI->buffer_list);
	}

	// Cleanup
	if (!lister)
	{
		FreeVec(path_buffer);
		FreeDosObject(DOS_FIB, fib);
	}

	return buffer;
}

// Searches for an empty buffer, or one with the same name (preferred)
// If a suitable buffer is not found, it uses the current buffer
// Called from the LISTER PROCESS
DirBuffer *lister_buffer_find_empty(Lister *lister, char *path, struct DateStamp *stamp)
{
	DirBuffer *buffer, *first_empty = 0, *first_unlocked = 0;

	// Check we're not showing special
	check_special_buffer(lister, 1);

	// If current buffer is empty, use that one
	if (!(lister->cur_buffer->flags & DWF_VALID) && !lister_dual_other_side_shows(lister, lister->cur_buffer))
	{
		// Lock buffer
		buffer_lock(lister->cur_buffer, TRUE);

		// Free buffer
		buffer_freedir(lister->cur_buffer, 1);

		// Unlock it
		buffer_unlock(lister->cur_buffer);
		return lister->cur_buffer;
	}

	// Lock buffer list
	lock_listlock(&GUI->buffer_list, TRUE);

	// Go through buffers in this list (backwards)
	for (buffer = (DirBuffer *)GUI->buffer_list.list.lh_TailPred; buffer->node.ln_Pred;
		 buffer = (DirBuffer *)buffer->node.ln_Pred)
	{
		// See if this directory is available and matches our path
		if (path && stricmp(buffer->buf_Path, path) == 0 &&
			(!stamp || CompareDates(&buffer->buf_VolumeDate, stamp) == 0))
		{
			// Not locked, or in use by this lister?
			if (!buffer->buf_CurrentLister || buffer->buf_CurrentLister == lister)
			{
				// Store pointer
				first_empty = buffer;
				break;
			}
		}

		// If directory is empty, store pointer (if the first one)
		if (!buffer->buf_CurrentLister && !(buffer->flags & DWF_VALID) && !first_empty)
			first_empty = buffer;

		// First unlocked buffer?
		if (!buffer->buf_CurrentLister && !first_unlocked)
		{
			// Because buffers are moved to the head of the list whenever
			// they are accessed, this will point to the unlocked buffer
			// that was accessed the longest time ago
			first_unlocked = buffer;
		}
	}

	// If we found an empty one, use that one
	if (!(buffer = first_empty))
	{
		// Allocate a new buffer if we're allowed to
		if (GUI->buffer_count < environment->env->settings.max_buffer_count &&
			!(environment->env->settings.dir_flags & DIRFLAGS_DISABLE_CACHING))
		{
			// If this fails, use first unlocked
			buffer = lister_new_buffer(lister);
		}
	}

	// If nothing yet, use first unlocked buffer (if there is one)
	if (!buffer && !(buffer = first_unlocked))
	{
		// The other dual panel may show the current buffer; it gets a new one
		if (lister_dual_other_side_shows(lister, lister->cur_buffer))
			buffer = lister_new_buffer(lister);

		// If not, re-use current buffer
		if (!buffer)
			buffer = lister->cur_buffer;
	}

	// Lock buffer
	buffer_lock(buffer, TRUE);

	// Free buffer
	buffer_freedir(buffer, 1);

	// Unlock buffer
	buffer_unlock(buffer);

	// Show buffer in lister
	lister_show_buffer(lister, buffer, 0, 1);

	// Unlock buffer list
	unlock_listlock(&GUI->buffer_list);

	// Return current buffer
	return lister->cur_buffer;
}

// Checks buffers to see if they need re-reading
// Called from the LISTER PROCESS
void lister_check_old_buffer(Lister *lister, BOOL force)
{
	int reread = 0;

	// If re-read flag is set, and buffer is valid
	if ((force || environment->env->settings.dir_flags & DIRFLAGS_REREAD_CHANGED) &&
		lister->cur_buffer->flags & DWF_VALID && lister->cur_buffer->buf_Path[0] &&
		!lister->cur_buffer->buf_CustomHandler[0])
	{
		DirBuffer *buffer;
		BPTR lock;

		// Get current buffer
		buffer = lister->cur_buffer;

		// Return on a device list
		if (!(IsListEmpty((struct List *)&buffer->entry_list)) &&
			((DirEntry *)buffer->entry_list.mlh_Head)->de_Node.dn_Type == ENTRY_DEVICE)
			return;

		// Lock and examine directory
		if ((lock = Lock(buffer->buf_Path, ACCESS_READ)))
		{
			// Examine and unlock
			Examine(lock, lister->fib);
			UnLock(lock);

			// Make sure it's a directory
			if (lister->fib->fib_DirEntryType > 0)
			{
				// See if datestamp has changed
				if (CompareDates(&buffer->buf_DirectoryDate, &lister->fib->fib_Date) > 0)
					reread = 1;

				// Or, see if disk has changed
				else if (buffer->buf_VolumeLabel[0])
				{
					char rootname[512];

					// Get root of current directory
					if (get_path_root(buffer->buf_Path, rootname, 0))
					{
						// See if disk name is not the same
						if (strcmp(rootname, buffer->buf_VolumeLabel) != 0)
							reread = 1;
					}
				}
			}
		}

		// Did we need to re-read?
		if (reread)
		{
			// Re-read directory
			read_directory(lister, buffer->buf_Path, GETDIRF_RESELECT);
		}
	}
}

// Display a buffer in a lister
// Called from LISTER PROCESS
void lister_show_buffer(Lister *lister, DirBuffer *buffer, int show, BOOL active)
{
	// Different to current buffer?
	if (lister->cur_buffer != buffer)
	{
		// End a direct edit
		lister_end_edit(lister, 0);

		// Clear lock pointer in current buffer, unless the other panel of a
		// dual lister still shows it
		if (!lister_dual_other_side_shows(lister, lister->cur_buffer))
			buffer_clear_lock(lister->cur_buffer, 0);

		// Free icons
		backdrop_free_list(lister->backdrop_info);

		// Store new buffer pointer
		lister->cur_buffer = buffer;
		buffer->buf_CurrentLister = lister;

		// Remove buffer and add to head of list
		Remove(&buffer->node);
		AddHead(&GUI->buffer_list.list, &buffer->node);

		// See if buffer needs to be resorted
		lister_resort(lister, 0);

		// Clear save status flag
		lister->flags &= ~LISTERF_SAVE_STATUS;

		// Does new buffer have a custom handler?
		if (lister->cur_buffer->buf_CustomHandler[0] && active)
		{
			// Send active message
			buffer_active(lister->cur_buffer, 0);
		}

		// Add to history list
		lister_add_history(lister, lister->cur_buffer);
	}

	// Initialise path field
	if (show && lister_valid_window(lister))
	{
		// Is volume present?
		if (!lister->cur_buffer->buf_CustomHandler[0] && VolumePresent(buffer))
		{
			BPTR lock;
			D_S(struct FileInfoBlock, fib)

			// Lock path
			if ((lock = Lock(buffer->buf_Path, ACCESS_READ)))
			{
				// Store full path
				NameFromLock(lock, buffer->buf_ExpandedPath, 512);
				AddPart(buffer->buf_ExpandedPath, "", 512);

				// Examine object
				Examine(lock, fib);
				UnLock(lock);

				// Save object name
				if (fib->fib_FileName[0])
					strcpy(buffer->buf_ObjectName, fib->fib_FileName);
				else
				{
					if (buffer->flags & DWF_ROOT)
					{
						char *ptr;

						stccpy(buffer->buf_ObjectName, buffer->buf_ExpandedPath, GUI->def_filename_length - 1);
						if ((ptr = strchr(buffer->buf_ObjectName, ':')))
							*ptr = 0;
					}
					else
						stccpy(
							buffer->buf_ObjectName, FilePart(buffer->buf_ExpandedPath), GUI->def_filename_length - 1);
				}
			}

			// Failed to lock
			else
				strcpy(buffer->buf_ExpandedPath, buffer->buf_Path);

			// Update disk name and size
			lister_update_name(lister);
		}

		// No, refresh disk name from buffer
		else
			lister_show_name(lister);

		// Re-evaluate gauge visibility (e.g. returning to root from a subdir)
		lister_set_gauge(lister, TRUE);

		// Not showing icons?
		if (!(lister->flags & LISTERF_VIEW_ICONS) || lister->flags & LISTERF_ICON_ACTION)
		{
			// Update path field
			lister_update_pathfield(lister);

			// Refresh window
			lister_refresh_display(lister, REFRESHF_SLIDERS | REFRESHF_STATUS | REFRESHF_CLEAR_ICONS);

			// Show selection information
			select_show_info(lister, 1);
		}
	}
}

// Update the datestamp for a buffered directory
// Called from LISTER PROCESS
void update_buffer_stamp(Lister *lister)
{
	BPTR lock;
	D_S(struct FileInfoBlock, info)
	char path[512];
	/*
		DirBuffer *buffer=0;
	*/

	// Invalid window?
	if (!lister)
		return;

	// Get current path and disk stamp
	strcpy(path, lister->cur_buffer->buf_Path);

	// Lock the path
	if ((lock = Lock(path, ACCESS_READ)))
	{
		// Examine and unlock
		Examine(lock, info);
		UnLock(lock);

		// Store new stamp
		lister->cur_buffer->buf_DirectoryDate = info->fib_Date;
	}

	/*
		// Couldn't lock, use zero datestamp
		else
		{
			info->fib_Date.ds_Days=0;
			info->fib_Date.ds_Minute=0;
			info->fib_Date.ds_Tick=0;
		}

		// Lock buffer list
		lock_listlock(&GUI->buffer_list,TRUE);

		// Find all instances of this path
		while (buffer=
			(lister_find_buffer(
				lister,
				buffer,
				path,
				&lister->cur_buffer->buf_VolumeDate,
				0,
				LISTER_BFPF_DONT_MOVE|LISTER_BFPF_DONT_LOCK|LISTER_BFPF_DONT_TEST)))
		{
			// Not the original buffer?
			if (buffer!=lister->cur_buffer)
			{
				// If buffer is locked, tell its lister to get this copy
				if (buffer->buf_CurrentLister)
				{
					IPC_Command(
						buffer->buf_CurrentLister->ipc,
						LISTER_COPY_BUFFER,
						0,
						lister->cur_buffer,
						0,
						0);
				}

				// Otherwise, copy into the buffer
				else
				{
					buffer_copy(lister->cur_buffer,buffer,0);
				}
			}

			// Get next
			buffer=(DirBuffer *)buffer->node.ln_Succ;
		}

		// While there's parents to look for
		while (path_parent(path))
		{
			// Initialise
			buffer=0;
			lock=0;

			// Look for all parents
			while (buffer=
				(lister_find_buffer(
					lister,
					buffer,
					path,
					&lister->cur_buffer->buf_VolumeDate,
					0,
					LISTER_BFPF_DONT_MOVE|LISTER_BFPF_DONT_LOCK|LISTER_BFPF_DONT_TEST)))
			{
				// Need to get date?
				if (!lock)
				{
					// Lock the path
					if (!(lock=Lock(path,ACCESS_READ)))
						break;

					// Examine and unlock
					Examine(lock,info);
					UnLock(lock);
				}

				// Store datestamp
				buffer->buf_DirectoryDate=info->fib_Date;

				// Get next
				buffer=(DirBuffer *)buffer->node.ln_Succ;
			}
		}

		// Unlock buffer list
		unlock_listlock(&GUI->buffer_list);
	*/
}

// Looks for an empty buffer; buffer list must be locked
DirBuffer *lister_get_empty_buffer(void)
{
	DirBuffer *buffer;

	// Go through buffers
	for (buffer = (DirBuffer *)GUI->buffer_list.list.lh_Head; buffer->node.ln_Succ;
		 buffer = (DirBuffer *)buffer->node.ln_Succ)
	{
		// Is buffer empty?
		if (!buffer->buf_CurrentLister && !(buffer->flags & DWF_VALID))
		{
			buffer_freedir(buffer, TRUE);
			return buffer;
		}
	}
	return 0;
}

// Do a global datestamp update
void update_lister_global(char *path)
{
	IPCData *ipc;
	Lister *lister;

	// Lock lister list
	lock_listlock(&GUI->lister_list, 0);

	// Go through listers
	for (ipc = (IPCData *)GUI->lister_list.list.lh_Head; ipc->node.mln_Succ; ipc = (IPCData *)ipc->node.mln_Succ)
	{
		// Get lister
		lister = IPCDATA(ipc);

		// Compare path
		if (stricmp(lister->cur_buffer->buf_Path, path) == 0)
		{
			// Send update command
			lister_command(lister, LISTER_UPDATE_STAMP, 0, 0, 0, 0);
		}
	}

	// Unlock lister list
	unlock_listlock(&GUI->lister_list);
}

// Fix current directory
void lister_fix_cd(Lister *lister)
{
	BPTR lock;

	// Got a path?
	if (lister->cur_buffer->buf_Path[0] && lister->cur_buffer->flags & DWF_VALID)
	{
		// Lock current path
		if ((lock = Lock(lister->cur_buffer->buf_Path, ACCESS_READ)))
		{
			// CD to it
			UnLock(CurrentDir(lock));
		}
	}
}

// Searches for a named buffer and moves to it if found - special version for ARexx
BOOL lister_find_cached_buffer(Lister *lister, char *path, char *handler)
{
	short ret = 0;
	DirBuffer *buffer;

	// Lock buffer list
	lock_listlock(&GUI->buffer_list, FALSE);

	// Go through all buffers
	for (buffer = (DirBuffer *)GUI->buffer_list.list.lh_Head; buffer->node.ln_Succ;
		 buffer = (DirBuffer *)buffer->node.ln_Succ)
	{
		// Skip current buffer
		if (buffer == lister->cur_buffer)
			continue;

		// See if handler matches
		if (stricmp(buffer->buf_CustomHandler, handler) == 0)
		{
			// Does lister match?
			if (buffer->buf_OwnerLister == lister)
			{
				short res;

				// Does pathname match?
				res =
					(buffer->more_flags & DWF_CASE) ? strcmp(path, buffer->buf_Path) : stricmp(path, buffer->buf_Path);

				// Match?
				if (res == 0)
				{
					ret = 1;
					break;
				}
			}
		}
	}

	// Did we find one?
	if (ret)
	{
		// Show buffer in lister
		lister_show_buffer(lister, buffer, 1, 1);
	}

	// Unlock buffer list
	unlock_listlock(&GUI->buffer_list);

	return ret;
}

// Free buffers from a particular lister
void lister_free_caches(Lister *lister, char *handler)
{
	DirBuffer *buffer;

	// Lock buffer list
	lock_listlock(&GUI->buffer_list, TRUE);

	// Go through all buffers
	for (buffer = (DirBuffer *)GUI->buffer_list.list.lh_Head; buffer->node.ln_Succ;)
	{
		DirBuffer *next;

		// Save next pointer
		next = (DirBuffer *)buffer->node.ln_Succ;

		// See if buffer matches lister and handler
		if (buffer->buf_OwnerLister == lister && stricmp(buffer->buf_CustomHandler, handler) == 0)
		{
			// Check buffer isn't currently displayed
			if (!buffer->buf_CurrentLister)
			{
				// Free buffer
				lister_free_buffer(buffer);
			}
		}

		// Get next pointer
		buffer = next;
	}

	// Unlock buffer list
	unlock_listlock(&GUI->buffer_list);
}

// User selected a cache list entry
BOOL lister_select_cache(Lister *lister, DirBuffer *test_buffer)
{
	DirBuffer *buffer;
	BOOL ret = 0;

	// Lock buffer list
	lock_listlock(&GUI->buffer_list, FALSE);

	// Go through all buffers
	for (buffer = (DirBuffer *)GUI->buffer_list.list.lh_Head; buffer->node.ln_Succ;
		 buffer = (DirBuffer *)buffer->node.ln_Succ)
	{
		// Match buffer?
		if (buffer == test_buffer)
		{
			// See if buffer has a custom handler
			if (buffer->buf_CustomHandler[0])
			{
				// If we're currently displaying a special buffer, return to a normal one
				check_special_buffer(lister, 0);

				// Show buffer in lister
				lister_show_buffer(lister, buffer, 1, 0);

				// Send active message
				buffer_active(buffer, 1);
				ret = 1;
				break;
			}
		}
	}

	// Unlock buffer list
	unlock_listlock(&GUI->buffer_list);
	return ret;
}
