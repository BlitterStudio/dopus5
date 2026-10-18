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
#include <proto/module.h>

// Configure a lister
void lister_configure(Lister *lister)
{
	// Format locked?
	if (lister->cur_buffer->more_flags & DWF_LOCK_FORMAT)
		return;

	// Already configuring?
	if (lister->more_flags & LISTERF_CONFIGURE)
		return;

	// Not available when showing a device/cache list or in icon action mode
	if (lister->cur_buffer->more_flags & (DWF_DEVICE_LIST | DWF_CACHE_LIST))
		return;
	if ((lister->flags & LISTERF_VIEW_ICONS) && (lister->flags & LISTERF_ICON_ACTION))
		return;

	// Run configure function
	function_launch_quick(FUNCTION_RUN_FUNCTION, def_function_configure, lister);
}

// Change a lister's format
void lister_change_format(Lister *lister, ListFormat *format)
{
	ListFormat oldformat;
	short flags = SNIFFF_NO_FILETYPES;

	// Get current format
	oldformat = lister->cur_buffer->buf_ListFormat;

	// Clear special sort flag in buffer
	lister->cur_buffer->flags &= ~DWF_SPECIAL_SORT;

	// Need to show filetypes now?
	if (buffer_show_filetypes(format) && !(buffer_show_filetypes(&oldformat)))
		flags &= ~SNIFFF_NO_FILETYPES;

	// Need to show version now?
	if (buffer_show_versions(format) && !(buffer_show_versions(&oldformat)))
		flags |= SNIFFF_VERSION;

	// Store format in lister
	lister->format = *format;
	lister->user_format = *format;

	// Resort and refresh the display
	lister_refresh_display(lister, REFRESHF_RESORT | REFRESHF_SLIDERS);

	// Show selection information
	select_show_info(lister, 1);

	// Update filetypes or versions?
	if (flags != SNIFFF_NO_FILETYPES)
	{
		// Lock buffer
		buffer_lock(lister->cur_buffer, FALSE);

		// Get filetypes
		filetype_find_typelist(lister, lister->cur_buffer, flags);

		// Unlock buffer
		buffer_unlock(lister->cur_buffer);
	}

	// Fuel gauge
	if ((format->flags & LFORMATF_GAUGE) != (oldformat.flags & LFORMATF_GAUGE))
	{
		// Update fuel gauge
		lister_set_gauge(lister, TRUE);
	}
}

// Change a lister's sort method
void lister_set_sort(Lister *lister, short item, UWORD qual)
{
	ListFormat format;
	BOOL busy = 0;

	// Is lister already busy?
	if (lister->flags & LISTERF_BUSY)
		busy = 1;

	// Lock this lister
	else
		lister_busy(lister, 1);

	// Get existing format
	format = lister->cur_buffer->buf_ListFormat;

	// If method is already selected, we change order
	if (format.sort.sort == item)
		format.sort.sort_flags ^= SORT_REVERSE;

	// Set sort type, clear reverse flag
	else
	{
		format.sort.sort = item;
		format.sort.sort_flags &= ~SORT_REVERSE;
	}

	// Clear special sort flag
	lister->cur_buffer->flags &= ~DWF_SPECIAL_SORT;

	// Store format in lister
	lister->format = format;
	lister->user_format = format;

	// Resort and refresh the display
	lister_refresh_display(lister, REFRESHF_RESORT | ((qual & IEQUALIFIER_CONTROL) ? REFRESHF_SORTSEL : 0));

	// Unlock lister
	if (!busy)
		lister_unbusy(lister, 1);
}

// Resort a lister
BOOL lister_resort(Lister *lister, short type)
{
	DirBuffer *buffer;

	// Valid lister and buffer?
	if (!lister || !(buffer = lister->cur_buffer))
		return 0;

	// Does buffer have a special sort format?
	if (buffer->flags & DWF_SPECIAL_SORT)
		return 0;

	// See if we need to resort
	if (!type && !(type = resort_test(&buffer->buf_ListFormat, &lister->format)))
	{
		buffer->buf_ListFormat = lister->format;
		return 0;
	}

	// Store new sort method in buffer
	buffer->buf_ListFormat = lister->format;

	// Lock buffer
	buffer_lock(lister->cur_buffer, TRUE);

	// Has sort method changed?
	if (type & RESORT_SORT)
	{
		struct MinList temp;
		long file_count, dir_count;

		// Replace rejects if necessary
		if (type & RESORT_REJECTS)
			buffer_replace_rejects(buffer, 0);

		// Copy list pointers
		temp = buffer->entry_list;
		temp.mlh_TailPred->mln_Succ = (struct MinNode *)&temp.mlh_Tail;
		temp.mlh_Head->mln_Pred = (struct MinNode *)&temp.mlh_Head;

		// Store file and directory counts
		file_count = buffer->buf_TotalFiles[0];
		dir_count = buffer->buf_TotalDirs[0];

		// Clear buffer entry pointers
		NewList((struct List *)&buffer->entry_list);
		buffer->first_file = 0;
		buffer->first_dir = 0;

		// Clear counts
		buffer->buf_TotalEntries[0] = 0;
		buffer->buf_TotalFiles[0] = 0;
		buffer->buf_TotalDirs[0] = 0;
		buffer->buf_TotalBytes[0] = 0;
		buffer->buf_SelectedFiles[0] = 0;
		buffer->buf_SelectedDirs[0] = 0;
		buffer->buf_SelectedBytes[0] = 0;

		// Sort the list
		buffer_sort_list(buffer, &temp, file_count, dir_count);
		type = 0;
	}

	// Change reverse sort
	else if (type & RESORT_REVERSE)
	{
		buffer_sort_reversesep(buffer, 1);
	}

	// Separation state changed
	else if (type & RESORT_SEPARATE)
	{
		buffer_sort_reversesep(buffer, 0);
	}

	// Do rejects for this lister
	if (type & RESORT_REJECTS)
	{
		buffer_replace_rejects(buffer, 1);
		buffer_do_rejects(buffer);
	}

	// Unlock buffer
	buffer_unlock(buffer);
	return 1;
}

int resort_test(ListFormat *old, ListFormat *new)
{
	int type = 0;

	// See if sort method has changed
	if (old->sort.sort != new->sort.sort)
		type |= RESORT_SORT;

	// Separation state changed?
	else if (old->sort.separation != new->sort.separation)
	{
		// If we've gone from separated to mixed, we have to resort
		if (new->sort.separation == SEPARATE_MIX)
			type |= RESORT_SORT;

		// Otherwise, we can just rearrange
		else
			type |= RESORT_SEPARATE;
	}

	// Reverse state changed?
	if (old->sort.sort_flags != new->sort.sort_flags)
		type |= RESORT_REVERSE;

	// See if rejects have changed
	if ((old->flags & (LFORMATF_REJECT_ICONS | LFORMATF_HIDDEN_BIT)) !=
		(new->flags &(LFORMATF_REJECT_ICONS | LFORMATF_HIDDEN_BIT)))
		type |= RESORT_REJECTS;

	// See if patterns have changed
	if (stricmp(old->show_pattern, new->show_pattern) != 0 || stricmp(old->hide_pattern, new->hide_pattern) != 0)
		type |= RESORT_REJECTS;

	return type;
}

// Default lister parent popup
static PopUpItem parent_popup[] = {{{0}, (IPTR)MSG_PARENT, PP_PARENT, POPUPF_LOCALE, 0},
								   {{0}, (IPTR)MSG_ROOT, PP_ROOT, POPUPF_LOCALE, 0},
								   {{0}, POPUP_BARLABEL, 0, 0, 0},
								   {{0}, (IPTR)MSG_DEVICE_LIST, PP_DEVICE_LIST, POPUPF_LOCALE, 0},
								   {{0}, (IPTR)MSG_BUFFER_LIST, PP_BUFFER_LIST, POPUPF_LOCALE, 0},
								   {{0}, POPUP_BARLABEL, 0, 0, 0},
								   {{0}, (IPTR)MSG_REREAD_DIR, PP_REREAD_DIR, POPUPF_LOCALE, 0}};

// Do lister parent popup
void lister_parent_popup(Lister *lister, unsigned short code)
{
	PopUpMenu *parent_menu;
	PopUpItem *item;
	APTR memory;
	struct Node *node;
	short a;
	unsigned short res;
	BOOL help = 0;

	// Create a memory handle
	if (!(memory = NewMemHandle(1024, 768, MEMF_CLEAR)))
		return;

	// Initialise parent menu
	if (!(parent_menu = AllocMemH(memory, sizeof(PopUpMenu))))
	{
		FreeMemHandle(memory);
		return;
	}
	NewList((struct List *)parent_menu);
	parent_menu->locale = &locale;
	parent_menu->flags = POPUPMF_HELP;

	// Create default items
	for (a = 0; a < 7; a++)
	{
		// Allocate item
		if ((item = AllocMemH(memory, sizeof(PopUpItem))))
		{
			// Copy template item
			*item = parent_popup[a];

			// Add to list
			AddTail((struct List *)parent_menu, (struct Node *)item);
		}
	}

	// Zero count
	a = 0;

	// Valid history?
	if (lister->path_history)
	{
		// Lock lister history
		LockAttList(lister->path_history, FALSE);

		// Go through lister history
		for (node = lister->path_history->list.lh_Head; node->ln_Succ; node = node->ln_Succ)
		{
			// Should not display current directory
			if (stricmp(node->ln_Name, lister->cur_buffer->buf_Path) == 0)
				continue;

			// First item?
			if (a == 0)
			{
				// Allocate item to be a bar
				if ((item = AllocMemH(memory, sizeof(PopUpItem))))
				{
					// Initialise item
					item->item_name = POPUP_BARLABEL;

					// Add to list
					AddTail((struct List *)parent_menu, (struct Node *)item);
				}
			}

			// Create item
			if ((item = AllocMemH(memory, sizeof(PopUpItem))))
			{
				// Initialise item
				item->item_name = (IPTR)node->ln_Name;
				item->id = PP_BASE + a;
				item->data = (APTR)item->item_name;

				// Add to list
				AddTail((struct List *)parent_menu, (struct Node *)item);
			}

			// Increment count
			++a;
		}

		// Unlock list
		UnlockAttList(lister->path_history);
	}

	// Do pop-up menu
	if ((res = DoPopUpMenu(lister->window, parent_menu, &item, code)) != (UWORD)-1)
	{
		// Help?
		if (res & POPUP_HELPFLAG)
		{
			help = 1;
			res &= ~POPUP_HELPFLAG;
		}

		// Get result
		switch (res)
		{
		// Parent
		case PP_PARENT:
			if (help)
				help_show_help("Parent", 0);
			else
				lister_do_function(lister, MENU_LISTER_PARENT);
			break;

		// Root
		case PP_ROOT:
			if (help)
				help_show_help("Root", 0);
			else
				lister_do_function(lister, MENU_LISTER_ROOT);
			break;

		// Device list
		case PP_DEVICE_LIST:
			if (help)
				help_show_help("DeviceList", 0);
			else
				function_launch_quick(FUNCTION_RUN_FUNCTION, def_function_devicelist, lister);
			break;

		// Buffer list
		case PP_BUFFER_LIST:
			if (help)
				help_show_help("CacheList", 0);
			else
				function_launch_quick(FUNCTION_RUN_FUNCTION, def_function_cachelist, lister);
			break;

		// Re-read
		case PP_REREAD_DIR:

			// Help?
			if (help)
				help_show_help("ReReadDir", 0);

			// Custom handler?
			else if (lister->cur_buffer->buf_CustomHandler[0])
			{
				// Send message
				rexx_handler_msg(0,
								 lister->cur_buffer,
								 RXMF_WARN,
								 HA_String,
								 0,
								 "reread",
								 HA_Value,
								 1,
								 lister,
								 HA_String,
								 2,
								 lister->cur_buffer->buf_Path,
								 TAG_END);
			}

			// Otherwise, check for special buffer first
			else if (!(check_special_buffer(lister, 1)))
			{
				// Re-read current dir
				read_directory(lister, lister->cur_buffer->buf_Path, GETDIRF_RESELECT);
			}

			// Switched from special buffer, need to refresh
			else
				lister_show_buffer(lister, lister->cur_buffer, 1, 1);
			break;

		// Other dir
		default:

			// Help?
			if (help)
				help_show_help("PathHistory", 0);
			else if (item)
			{
				// Read directory
				read_directory(lister, (char *)item->data, GETDIRF_CANCHECKBUFS | GETDIRF_CANMOVEEMPTY);
			}
			break;
		}
	}

	// Free data
	FreeMemHandle(memory);
}

// Add the path of a buffer the lister shows to the history list
void lister_add_history(Lister *lister, DirBuffer *buffer)
{
	Att_Node *node;

	// Is buffer valid?
	if (!lister->path_history || !buffer || !(buffer->flags & DWF_VALID) || !buffer->buf_Path[0])
		return;

	// Lock list
	LockAttList(lister->path_history, TRUE);

	// See if buffer is already in history list
	if (!(node = (Att_Node *)FindNameI(&lister->path_history->list, buffer->buf_Path)))
	{
		// Allocate a history node
		if ((node = Att_NewNode(lister->path_history, buffer->buf_Path, 0, 0)))
		{
			// If count is over 20, remove first entry
			if (lister->history_count == 20)
			{
				// Get first entry
				node = (Att_Node *)lister->path_history->list.lh_Head;

				// Remove and free it
				Att_RemNode(node);
			}

			// Otherwise, increment count
			else
				++lister->history_count;
		}
	}

	// Buffer was already in list
	else
	{
		// Remove and add to head of list
		Remove((struct Node *)node);
		AddHead(&lister->path_history->list, (struct Node *)node);
	}

	// Unlock list
	UnlockAttList(lister->path_history);
}
