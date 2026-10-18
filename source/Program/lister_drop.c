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

static void lister_drop_on_panel(Lister *dest, DOpusAppMessage *msg, short side);

// Handle some dropped files. On a dual lister they go to the panel under the
// pointer, which is not necessarily the active one.
void lister_receive_drop(Lister *dest, DOpusAppMessage *msg)
{
	short side;

	if ((side = lister_dual_side_at(dest, msg->da_Msg.am_MouseX, msg->da_Msg.am_MouseY)) >= 0)
		lister_dual_view_side(dest, side);
	lister_drop_on_panel(dest, msg, side);
	lister_dual_active_side(dest);
}

static void lister_drop_on_panel(Lister *dest, DOpusAppMessage *msg, short side)
{
	Lister *source = 0;
	IPTR source_data = 0, flags = 0, over_entry_data = 0;
	char pathname[256];
	struct ArgArray *arg_array;
	long func_type;
	Cfg_Function *function;
	BackdropInfo *info;
	DirEntry *over_entry = 0;
	UWORD qual;
	BOOL ok = 1;

	// Cache backdrop pointer
	info = dest->backdrop_info;

	// Get internal app message data
	get_appmsg_data(msg, &source_data, &flags, &over_entry_data);
	source = (Lister *)source_data;
	over_entry = (DirEntry *)over_entry_data;

	// Get qualifiers
	qual = (InputBase) ? PeekQualifier() : 0;

	// Does current buffer have a custom handler?
	if (dest->cur_buffer->buf_CustomHandler[0])
	{
		// Does message have lister in it?
		if (source == dest)
		{
			// Clear lister pointer
			set_appmsg_data(msg, 0, flags, 0);
		}

		// Build destination path
		strcpy(info->buffer, dest->cur_buffer->buf_Path);
		if (over_entry)
			AddPart(info->buffer, over_entry->de_Node.dn_Name, 512);
		else if (flags & DROPF_PARENT)
			path_parent(info->buffer);

		// Send to rexx handler
		if (over_entry || flags & DROPF_PARENT)
			qual |= IEQUALIFIER_SUBDROP;
		rexx_custom_app_msg(msg, dest->cur_buffer, "drop", source, info->buffer, qual);
		return;
	}

	// Is first entry a directory?
	if (WBArgDir(&msg->da_Msg.am_ArgList[0]))
	{
		// Get first path name
		if (DevNameFromLockDopus(msg->da_Msg.am_ArgList[0].wa_Lock, pathname, 256))
		{
			BOOL read = 0;

			// If shift is down, we read it, no questions asked
			if (qual & IEQUAL_ANYSHIFT)
				read = 1;

			// Is it a device?
			else if (pathname[strlen(pathname) - 1] == ':')
				read = 1;

			// Can only read if lister is open, and not in icon mode
			if (!dest->window || dest->flags & LISTERF_VIEW_ICONS)
				read = 0;

			// Want to read directory?
			if (read)
			{
				// Load directory
				read_directory(dest, pathname, GETDIRF_CANCHECKBUFS | GETDIRF_CANMOVEEMPTY);
				return;
			}

			// In icon mode, showing device list?
			if (dest->flags & LISTERF_VIEW_ICONS && dest->cur_buffer->more_flags & DWF_DEVICE_LIST)
			{
				// Do assign
				function_launch(
					FUNCTION_RUN_FUNCTION, def_function_assign, 0, FUNCF_DRAG_DROP, 0, dest, pathname, 0, 0, 0, 0);
				return;
			}
		}

		// Error
		else
			return;
	}

	// Lister must have a valid buffer
	if (!(dest->cur_buffer->flags & DWF_VALID))
		return;

	// Normal filetype action to start with
	func_type = FUNCTION_FILETYPE;
	function = 0;
	strcpy(info->buffer, dest->cur_buffer->buf_Path);

	// Is destination lister in icon mode?
	if (dest->flags & LISTERF_VIEW_ICONS)
	{
		BackdropObject *drop_on;

		// Lock backdrop list
		lock_listlock(&info->objects, 0);

		// See if it's dropped on anything
		if ((drop_on = backdrop_get_object(info, msg->da_Msg.am_MouseX, msg->da_Msg.am_MouseY, 0)))
		{
			// Is shift/alt down?
			if ((qual & (IEQUALIFIER_LSHIFT | IEQUALIFIER_LALT)) == (IEQUALIFIER_LSHIFT | IEQUALIFIER_LALT))
			{
				// Get path of first file
				GetWBArgPath(&msg->da_Msg.am_ArgList[0], pathname, 256);

				// Replace the image
				backdrop_replace_icon_image(info, pathname, drop_on);
				ok = 0;
			}

			// Is this a drawer?
			else if (drop_on->icon->do_Type == WBDRAWER || drop_on->icon->do_Type == WBGARBAGE)
			{
				// Move into this drawer
				func_type = FUNCTION_RUN_FUNCTION_EXTERNAL;
				function = def_function_copy;
				AddPart(info->buffer, drop_on->name, 512);
			}

			// Dropped on a tool?
			else if (drop_on->icon->do_Type == WBTOOL)
			{
				// Run program with args
				backdrop_object_open(info, drop_on, 0, 0, msg->da_Msg.am_NumArgs, msg->da_Msg.am_ArgList);
				ok = 0;
			}
		}

		// If not, just use copy command
		else
		{
			function = def_function_copy;
			func_type = FUNCTION_RUN_FUNCTION_EXTERNAL;
		}

		// Unlock backdrop list
		unlock_listlock(&info->objects);
	}

	// Drop was in name mode; if drop was into a sub-directory, add that to path
	else if (over_entry)
		AddPart(info->buffer, over_entry->de_Node.dn_Name, 512);

	// Or drop into parent area
	else if (flags & DROPF_PARENT)
		path_parent(info->buffer);

	// Ok to do filetype?
	if (ok)
	{
		short action;

		// Get arg array
		if (!(arg_array = AppArgArray(msg, AAF_ALLOW_DIRS)))
			return;

		// Get path of first file
		DevNameFromLockDopus(msg->da_Msg.am_ArgList[0].wa_Lock, pathname, 256);

		// Need source directory; if this is a drawer, get parent
		if (WBArgDir(&msg->da_Msg.am_ArgList[0]))
		{
			// Get parent of drawer
			path_parent(pathname);
		}

		/* ACTION_CHANGE
				// If source is in icon mode, don't use it
				if (source && (source->flags&LISTERF_VIEW_ICONS) && !(source->flags&LISTERF_ICON_ACTION))
					source=0;
		*/

		// Get filetype action
		if (qual & IEQUALIFIER_CONTROL)
			action = FTTYPE_CTRL_DRAGDROP;
		else if (qual & (IEQUALIFIER_LALT | IEQUALIFIER_RALT))
			action = FTTYPE_ALT_DRAGDROP;
		else
			action = FTTYPE_DRAG_DROP;

		// Dual panel drop: the source panel stays active, the dropped-on
		// panel becomes the destination of the function
		if (side >= 0)
		{
			lister_dual_set_drop_side(dest, side);
			lister_dual_active_side(dest);
		}

		// Do filetype action on a file
		function_launch(func_type,
						function,
						action,
						(flags & DROPF_ICON_MODE) ? FUNCF_ICONS | FUNCF_DRAG_DROP : FUNCF_DRAG_DROP,
						source,
						dest,  //(dest && dest->window)?dest:0,
						pathname,
						info->buffer,
						arg_array,
						0,
						(Buttons *)CopyAppMessage(msg, global_memory_pool));
	}
}

// Test to see whether a drag is over a lister
DirEntry *lister_test_drag(Lister *our_lister, DragInfo *drag, short x, short y, Lister **last_lister)
{
	struct Window *window;
	Lister *lister = 0;
	DirEntry *entry = 0;

	// Lock the lister list
	lock_listlock(&GUI->lister_list, FALSE);

	// Find window we're over
	if (x > -1 && (window = WhichWindow(GUI->screen_pointer, x, y, WWF_NO_LOCK)))
	{
		ULONG id;

		// Is this a lister?
		if ((id = GetWindowID(window)) == WINDOW_LISTER)
		{
			IPCData *ipc;

			// Get IPC pointer
			if ((ipc = (IPCData *)GetWindowAppPort(window)))
			{
				// Get lister pointer
				if ((lister = (Lister *)IPCDATA(ipc)))
				{
					DirBuffer *buffer;

					// Buffer of the dual panel under the pointer
					buffer = lister_dual_side_buffer(
						lister, lister_dual_side_at(lister, x - window->LeftEdge, y - window->TopEdge));

					// If lister is busy, can't use it
					if (lister->flags & LISTERF_BUSY)
						lister = 0;

					// Custom handler, not enabled?
					else if (buffer->buf_CustomHandler[0] && !(buffer->cust_flags & CUSTF_SUBDROP))
						lister = 0;
				}
			}
		}

		/*
				// Or a start-menu?
				else
				if (id==WINDOW_START)
				{
					StartMenu *menu;

					// Get start menu pointer
					if (menu=(StartMenu *)GetWindowAppPort(window))
					{
						// Menu not already popped?
						if (!(menu->flags&STARTMENUF_POPPED))
						{
							// Send open command
							IPC_Command(menu->ipc,IPC_OPEN,0,0,0,0);
						}
					}
				}
		*/
	}

	// Had a 'last lister' that was different?
	if (*last_lister && *last_lister != lister)
	{
		// Was the last lister our lister?
		if (*last_lister == our_lister)
		{
			// Remove highlight
			lister_highlight(our_lister, -1, -1, drag);
		}

		// Is old lister still valid?
		else if (lister_check_valid(*last_lister))
		{
			// Tell old lister to stop highlighting
			IPC_Command((*last_lister)->ipc, LISTER_HIGHLIGHT, (IPTR)-1, drag, 0, REPLY_NO_PORT);
		}
	}

	// Got a new lister?
	if (lister)
	{
		// Is it our lister?
		if (lister == our_lister)
		{
			// Show highlight
			entry = lister_highlight(our_lister, x, y, drag);
		}

		// Another lister
		else
		{
			// Tell lister to highlight
			entry = (DirEntry *)IPC_Command(lister->ipc, LISTER_HIGHLIGHT, (y << 16) | x, drag, 0, REPLY_NO_PORT);
		}
	}

	// Save lister pointer
	*last_lister = lister;

	// Unlock lister list
	unlock_listlock(&GUI->lister_list);

	return entry;
}

static DirEntry *lister_highlight_panel(Lister *lister, short x, short y, DragInfo *drag);

// Highlight an entry if mouse is over it. A dual lister highlights in the
// panel under the pointer. Called from the LISTER PROCESS.
DirEntry *lister_highlight(Lister *lister, short x, short y, DragInfo *drag)
{
	DirEntry *entry;
	short side = -1, old_side;

	if ((old_side = lister_dual_highlight_side(lister)) < 0)
		return lister_highlight_panel(lister, x, y, drag);

	if (x > -1 && lister_valid_window(lister))
		side = lister_dual_side_at(lister, x - lister->window->LeftEdge, y - lister->window->TopEdge);

	// Moved to the other panel (or away)? Clear the old highlight there
	if (lister->last_highlight && side != old_side)
	{
		lister_dual_view_side(lister, old_side);
		lister_highlight_panel(lister, -1, -1, drag);
	}

	entry = 0;
	if (side >= 0)
	{
		lister_dual_view_side(lister, side);
		lister_dual_set_highlight_side(lister, side);
		entry = lister_highlight_panel(lister, x, y, drag);
	}
	lister_dual_active_side(lister);
	return entry;
}

static DirEntry *lister_highlight_panel(Lister *lister, short x, short y, DragInfo *drag)
{
	DirEntry *entry = 0;
	short line = 0, scroll = 0;

	// Ignore if in icon mode or no window
	if (lister->flags & LISTERF_VIEW_ICONS || !lister_valid_window(lister))
		return 0;

	// Convert coordinates to lister-relative
	x -= lister->window->LeftEdge;
	y -= lister->window->TopEdge;

	// Lock buffer
	buffer_lock(lister->cur_buffer, FALSE);

	// Can scroll?
	if (x >= lister->text_area.rect.MinX && x <= lister->text_area.rect.MaxX)
	{
		// Scroll down?
		if (y > lister->text_area.rect.MaxY && y < lister->text_area.rect.MaxY + lister->text_area.font->tf_YSize)
		{
			// Ok to scroll?
			if (lister->cur_buffer->buf_VertOffset + lister->text_height < lister->cur_buffer->buf_TotalEntries[0])
			{
				scroll = 1;
				line = lister->last_highpos;
			}
		}

		// Scroll up?
		else if (y < lister->text_area.rect.MinY && y > lister->text_area.rect.MinY - lister->text_area.font->tf_YSize)
		{
			// Ok to scroll?
			if (lister->cur_buffer->buf_VertOffset > 0)
			{
				scroll = -1;
				line = 0;
			}
		}

		// Is mouse within text area?
		else if (point_in_element(&lister->text_area, x, y))
		{
			short num;

			// Get line clicked on
			line = (y - lister->text_area.rect.MinY) / lister->text_area.font->tf_YSize;
			num = line + lister->cur_buffer->buf_VertOffset;

			// Valid entry?
			if (num >= 0 && num < lister->cur_buffer->buf_TotalEntries[0])
			{
				// Find entry
				for (entry = (DirEntry *)lister->cur_buffer->entry_list.mlh_Head; num > 0 && entry->de_Node.dn_Succ;
					 entry = (DirEntry *)entry->de_Node.dn_Succ, num--)
					;

				// Invalid entry, or not a directory?
				if (!entry->de_Node.dn_Succ || entry->de_Node.dn_Type <= 0)
					entry = 0;
			}
		}
	}

	// Has highlighted entry changed?
	if (lister->last_highlight != entry || scroll)
	{
		DirEntry *last;

		// Cache old and store new last highlight pointer
		last = lister->last_highlight;
		lister->last_highlight = entry;

		// Got draginfo?
		if (drag)
		{
			// Save drag position
			x = drag->sprite.X;
			y = drag->sprite.Y;

			// Clear drag stuff
			HideDragImage(drag);
		}

		// Was there a valid last highlight?
		if (last)
		{
			// Clear flag in last highlight
			last->de_Flags &= ~ENTF_HIGHLIGHT;

			// Show last highlight again
			display_entry(last, lister, lister->last_highpos);
		}

		// Want to scroll?
		if (scroll)
			lister_scroll(lister, 0, scroll);

		// Got a valid entry?
		if (entry)
		{
			// Save position
			lister->last_highpos = line;

			// Set highlight flag
			entry->de_Flags |= ENTF_HIGHLIGHT;

			// Show new highlight
			display_entry(entry, lister, line);
		}

		// Show drag again
		if (drag)
			ShowDragImage(drag, x, y);
	}

	// Unlock buffer
	buffer_unlock(lister->cur_buffer);

	return entry;
}
