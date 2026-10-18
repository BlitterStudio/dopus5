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

// Copies the contents of one window to another
// Called from the LISTER PROCESS
void buffer_copy(DirBuffer *source, DirBuffer *dest, Lister *dest_lister)
{
	DirEntry *entry;
	DirBuffer *buffer;

	// Lock buffer list
	lock_listlock(&GUI->buffer_list, FALSE);

	// Make sure source is valid
	for (buffer = (DirBuffer *)GUI->buffer_list.list.lh_Head; buffer->node.ln_Succ;
		 buffer = (DirBuffer *)buffer->node.ln_Succ)
	{
		if (buffer == source)
			break;
	}

	// Did we get it?
	if (buffer == source)
	{
		ReselectionData reselect;
		struct MinList file_list;
		DirEntry *newentry;
		short item, flags;

		// Lock source and destination buffers
		buffer_lock(source, FALSE);
		buffer_lock(dest, TRUE);

		// Make reselection list
		InitReselect(&reselect);
		MakeReselect(&reselect, dest, RESELF_SAVE_FILETYPES);

		// Free contents of destination window
		buffer_freedir(dest, 1);

		// Initialise file list
		NewList((struct List *)&file_list);

		// Copy entries
		for (entry = (DirEntry *)source->entry_list.mlh_Head; entry->de_Node.dn_Succ;
			 entry = (DirEntry *)entry->de_Node.dn_Succ)
		{
			// Copy entry
			if ((newentry = copy_file_entry(dest, entry)))
				AddTail((struct List *)&file_list, (struct Node *)newentry);
		}

		// Copy rejected entries
		for (entry = (DirEntry *)source->reject_list.mlh_Head; entry->de_Node.dn_Succ;
			 entry = (DirEntry *)entry->de_Node.dn_Succ)
		{
			// Copy entry
			if ((newentry = copy_file_entry(dest, entry)))
				AddTail((struct List *)&file_list, (struct Node *)newentry);
		}

		// Copy filetypes
		GetReselectFiletypes(&reselect, &file_list);

		// Sort into buffer
		buffer_sort_list(dest, &file_list, source->buf_TotalFiles[0], source->buf_TotalDirs[0]);

		// Initialise flags
		flags = SNIFFF_NO_FILETYPES;

		// Go through display items
		for (item = 0; item < DISPLAY_LAST; item++)
		{
			// Show filetypes?
			if (dest->buf_ListFormat.display_pos[item] == DISPLAY_FILETYPE)
				flags &= ~SNIFFF_NO_FILETYPES;

			// Show version?
			else if (dest->buf_ListFormat.display_pos[item] == DISPLAY_FILETYPE)
				flags |= SNIFFF_VERSION;
		}

		// Things to get?
		if (flags != SNIFFF_NO_FILETYPES)
			filetype_find_typelist(dest_lister, dest, flags);

		// Do reselection
		DoReselect(&reselect, dest_lister, 0);
		FreeReselect(&reselect);

		// Copy directory, device and disk name
		strcpy(dest->buf_Path, source->buf_Path);
		strcpy(dest->buf_ExpandedPath, source->buf_ExpandedPath);
		strcpy(dest->buf_VolumeLabel, source->buf_VolumeLabel);

		// Copy flags
		dest->flags = source->flags;

		// Copy directory date stamp
		dest->buf_DirectoryDate = source->buf_DirectoryDate;
		dest->buf_VolumeDate = source->buf_VolumeDate;
		dest->buf_DiskType = source->buf_DiskType;

		// Unlock buffers
		buffer_unlock(source);
		buffer_unlock(dest);
	}

	// Unlock buffer list
	unlock_listlock(&GUI->buffer_list);

	// Do we have a lister?
	if (dest_lister)
	{
		// Fill out and refresh path field
		lister_update_pathfield(dest_lister);

		// Update disk name and size
		lister_refresh_name(dest_lister);

		// Refresh window
		lister_refresh_display(dest_lister, REFRESHF_SLIDERS);

		// Zero the last selected entry pointer
		dest_lister->last_selected_entry = 0;

		// Show selection info
		select_show_info(dest_lister, 1);
	}
}

// Clears the contents of all unshown buffers
void buffers_clear(BOOL check_max)
{
	DirBuffer *buffer;

	// Get buffer list lock
	lock_listlock(&GUI->buffer_list, TRUE);

	// Go through all buffers
	for (buffer = (DirBuffer *)GUI->buffer_list.list.lh_Head; buffer->node.ln_Succ;)
	{
		DirBuffer *next;

		// Save next pointer
		next = (DirBuffer *)buffer->node.ln_Succ;

		// If this isn't currently displayed
		if (!buffer->buf_CurrentLister)
		{
			// Are we only checking against maximum?
			if (!check_max || (GUI->buffer_count > environment->env->settings.max_buffer_count ||
							   (environment->env->settings.dir_flags & DIRFLAGS_DISABLE_CACHING)))
			{
				// Free buffer
				lister_free_buffer(buffer);
			}
		}
		/*
		else if (!check_max) D(bug("** buffer locked - can't free\n"));
		*/

		// Get next pointer
		buffer = next;
	}

	// Free list lock
	unlock_listlock(&GUI->buffer_list);
}

// Check if buffer is a special buffer, and return to normal if so
// Called from the LISTER PROCESS
int check_special_buffer(Lister *lister, BOOL active)
{
	if (lister->cur_buffer == lister->special_buffer)
	{
		// Return to normal
		lister->cur_buffer = lister->old_buffer;

		// Send active message
		if (lister->cur_buffer->buf_CustomHandler[0] && active)
			buffer_active(lister->cur_buffer, 0);

		// Clear device list flag
		lister->flags &= ~LISTERF_DEVICE_LIST;

		// See if lister was source or destination
		check_lister_stored(lister);

		// If lister isn't a source or destination now, clear lock flag
		if (!(lister->flags & (LISTERF_SOURCE | LISTERF_DEST)))
			lister->flags &= ~LISTERF_SOURCEDEST_LOCK;

		// Hidden title?
		if (lister->more_flags & LISTERF_HIDE_TITLE && !(lister->cur_buffer->more_flags & DWF_HIDE_TITLE) &&
			!(lister->flags & LISTERF_VIEW_ICONS))
		{
			// Set title flag
			lister->more_flags |= LISTERF_TITLE;
			lister->more_flags &= ~LISTERF_HIDE_TITLE;

			// Clear file area
			EraseRect(&lister->list_area.rast,
					  lister->list_area.rect.MinX,
					  lister->list_area.rect.MinY,
					  lister->list_area.rect.MaxX,
					  lister->list_area.rect.MaxY);

			// Fix file area
			lister_init_filelist(lister);
		}
		return 1;
	}

	return 0;
}

// Opens the "special" dir in a window for a custom list
// Called from LISTER PROCESS
void buffer_show_special(Lister *lister, char *title)
{
	if (!title)
		title = "";

	// One special buffer per lister: the other dual panel gives it up
	lister_dual_release_special(lister);

	// Free special dir contents
	buffer_freedir(lister->special_buffer, 1);
	lister->special_buffer->buf_FreeDiskSpace = 0;
	lister->special_buffer->buf_TotalDiskSpace = 0;
#ifdef USE_64BIT
	lister->special_buffer->buf_FreeDiskSpace64 = 0;
	lister->special_buffer->buf_TotalDiskSpace64 = 0;
#endif

	// If not already on the special window, save current window and move to it
	if (lister->cur_buffer != lister->special_buffer)
	{
		// Save buffer pointer
		lister->old_buffer = lister->cur_buffer;

		// Send inactive message
		if (lister->cur_buffer->buf_CustomHandler[0])
			buffer_inactive(lister->cur_buffer, 0);

		// Store special window pointer
		lister->cur_buffer = lister->special_buffer;

		// Got a title?
		if (lister->more_flags & LISTERF_TITLE)
		{
			// Hide title
			lister->more_flags &= ~LISTERF_TITLE;
			lister->more_flags |= LISTERF_HIDE_TITLE;

			// Fix file area
			lister_init_filelist(lister);

			// Clear file area
			EraseRect(&lister->list_area.rast,
					  lister->list_area.rect.MinX,
					  lister->list_area.rect.MinY,
					  lister->list_area.rect.MaxX,
					  lister->list_area.rect.MaxY);
		}
	}

	// Store title
	stccpy(lister->special_buffer->buf_VolumeLabel, title, sizeof(lister->special_buffer->buf_VolumeLabel));
	lister->special_buffer->buf_Path[0] = 0;
	lister->special_buffer->buf_ExpandedPath[0] = 0;
	if (lister->special_buffer->buf_ObjectName)
		lister->special_buffer->buf_ObjectName[0] = 0;

	// Clear path field
	lister_update_pathfield(lister);

	// Turn lister off
	lister->flags &= ~(LISTERF_SOURCE | LISTERF_DEST);
}

// Low-memory handler to flush buffers
void buffer_mem_handler(ULONG flags)
{
	DirBuffer *buffer;

	// Don't try for CHIP requests
	if (flags & MEMF_CHIP)
		return;

	// Lock the buffer list
	lock_listlock(&GUI->buffer_list, TRUE);

	// Go through all buffers
	for (buffer = (DirBuffer *)GUI->buffer_list.list.lh_Head; buffer->node.ln_Succ;)
	{
		DirBuffer *next;

		// Save next pointer
		next = (DirBuffer *)buffer->node.ln_Succ;

		// If this isn't currently displayed
		if (!buffer->buf_CurrentLister)
		{
			// Free buffer
			lister_free_buffer(buffer);
		}

		// Get next pointer
		buffer = next;
	}

	// Release the lock
	unlock_listlock(&GUI->buffer_list);
}

// See if a buffer displays filetypes
BOOL buffer_show_filetypes(ListFormat *format)
{
	short a;

	for (a = 0; format->display_pos[a] != -1; a++)
		if (format->display_pos[a] == DISPLAY_FILETYPE)
			return 1;
	return 0;
}

// See if a buffer displays versions
BOOL buffer_show_versions(ListFormat *format)
{
	short a;

	for (a = 0; format->display_pos[a] != -1; a++)
		if (format->display_pos[a] == DISPLAY_VERSION)
			return 1;
	return 0;
}
