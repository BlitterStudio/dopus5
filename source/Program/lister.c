/*

Directory Opus 5
Original APL release version 5.82
Copyright 1993-2012 Jonathan Potter & GP Software
Copyright 2012-2013 DOPUS5 Open Source Team
Copyright 2023-2026 Dimitris Panokostas (dopus5 fork)

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

// Create a new lister
Lister *lister_new(Cfg_Lister *cfg_lister)
{
	Lister *lister;
	IPCData *ipc;

	// Allocate new lister structure
	if (!(lister = AllocVec(sizeof(Lister), MEMF_CLEAR)) || !(lister->memory = NewMemHandle(1500, 400, MEMF_CLEAR)) ||
		!(lister->backdrop_info = backdrop_new(0, 0)) || !(lister->title = AllocMemH(lister->memory, 300)) ||
		!(lister->fib = AllocDosObject(DOS_FIB, 0)))
	{
		if (lister)
		{
			lister->lister = cfg_lister;
			lister_free(lister);
		}
		else
			FreeListerDef(cfg_lister);
		return 0;
	}

	// Initialise backdrop info
	lister->backdrop_info->lister = lister;

	// Memory handle for progress names
	lister->progress_memory = NewMemHandle(640, 60, MEMF_PUBLIC | MEMF_CLEAR);

	// Initialise stuff
	NewList((struct List *)&lister->wait_list);
	InitListLock(&lister->user_data_list, 0);

	// If no lister configuration supplied, create one
	if (!cfg_lister)
	{
		if (!(cfg_lister = NewLister(0)))
		{
			lister_free(lister);
			return 0;
		}

		// Initialise lister
		lister_init_new(cfg_lister, 0);
	}

	// Store lister pointer
	lister->lister = cfg_lister;

	// Initialise lister
	lister->format = environment->env->list_format;
	lister->user_format = environment->env->list_format;
	strcpy(lister->font_name, environment->env->font_name[FONT_DIRS]);
	lister->font_size = environment->env->font_size[FONT_DIRS];
	lister->toolbar = GUI->toolbar;
	strcpy(lister->toolbar_path, environment->toolbar_path);
	lister->abort_signal = -1;
	lister->edit_type = -1;
	lister->cursor_line = -1;

	// Icon position from configuration?
	if (cfg_lister && cfg_lister->lister.icon_x)
	{
		// Get icon position
		lister->icon_pos_x = cfg_lister->lister.icon_x;
		lister->icon_pos_y = cfg_lister->lister.icon_y;
	}

	// Default icon position
	else
		lister->icon_pos_x = NO_ICON_POSITION;

	// Initialise position
	lister->dimensions.wd_Normal = cfg_lister->lister.pos[0];

	// Set refresh callback
	lister->backdrop_info->callback = GET_CALLBACK(lister_refresh_callback);

	// Start lister process
	if (!(IPC_Launch(&GUI->lister_list,
					 &ipc,
					 "dopus_lister",
					 (IPTR)&lister_code,
					 STACK_DEFAULT,
					 (IPTR)lister,
					 (struct Library *)DOSBase)))
	{
		if (!ipc)
			lister_free(lister);
		return 0;
	}

	return lister;
}

// Free lister data
void lister_free(Lister *lister)
{
	if (lister)
	{
		FreeMemHandle(lister->progress_memory);
		backdrop_free(lister->backdrop_info);
		FreeDosObject(DOS_FIB, lister->fib);
		FreeListerDef(lister->lister);
		FreeMemHandle(lister->memory);
		FreeVec(lister);
	}
}

// Find a lister showing a path
Lister *find_lister_path(char *path)
{
	Lister *lister = 0;
	IPCData *ipc;

	// Get list lock
	lock_listlock(&GUI->lister_list, FALSE);

	// Go through listers
	for (ipc = (IPCData *)GUI->lister_list.list.lh_Head; ipc->node.mln_Succ; ipc = (IPCData *)ipc->node.mln_Succ)
	{
		// Get lister
		lister = IPCDATA(ipc);

		// Compare path
		if (stricmp(lister->cur_buffer->buf_Path, path) == 0)
			break;
	}

	// Free list lock
	unlock_listlock(&GUI->lister_list);

	return (lister && ipc->node.mln_Succ) ? lister : 0;
}

// Send a command to a lister
ULONG lister_command(Lister *lister, ULONG command, ULONG flags, APTR data, APTR data_free, struct MsgPort *reply)
{
	// Valid lister?
	if (lister)
		return (ULONG)IPC_Command(lister->ipc, command, flags, data, data_free, reply);
	return 0;
}

// Update lister data
void lister_update(Lister *lister)
{
	if (lister && lister->lister)
	{
		char *path;

		// If lister is open, get current position
		if (lister_valid_window(lister))
			StoreWindowDims(lister->window, &lister->dimensions);

		// Show device list?
		if (lister->flags & LISTERF_DEVICE_LIST)
			lister->lister->lister.flags |= DLSTF_DEVICE_LIST;
		else
			lister->lister->lister.flags &= ~DLSTF_DEVICE_LIST;

		// Locked?
		if (lister->flags & LISTERF_LOCK_POS)
			lister->lister->lister.flags |= DLSTF_LOCK_POS;
		else
			lister->lister->lister.flags &= ~DLSTF_LOCK_POS;

		// Iconified?
		if (lister->flags & LISTERF_ICONIFIED)
			lister->lister->lister.flags |= DLSTF_ICONIFIED;
		else
			lister->lister->lister.flags &= ~DLSTF_ICONIFIED;

		// Source?
		if (lister->flags & LISTERF_SOURCE)
			lister->lister->lister.flags |= DLSTF_SOURCE;
		else
			lister->lister->lister.flags &= ~DLSTF_SOURCE;

		// Destination?
		if (lister->flags & LISTERF_DEST)
			lister->lister->lister.flags |= DLSTF_DEST;
		else
			lister->lister->lister.flags &= ~DLSTF_DEST;

		// Source/dest locked?
		if (lister->flags & LISTERF_SOURCEDEST_LOCK)
			lister->lister->lister.flags |= DLSTF_LOCKED;
		else
			lister->lister->lister.flags &= ~DLSTF_LOCKED;

		// Icon mode?
		if (lister->flags & LISTERF_VIEW_ICONS)
			lister->lister->lister.flags |= DLSTF_ICON;
		else
			lister->lister->lister.flags &= ~DLSTF_ICON;

		// Icon action mdoe?
		if (lister->flags & LISTERF_ICON_ACTION)
			lister->lister->lister.flags |= DLSTF_ICON_ACTION;
		else
			lister->lister->lister.flags &= ~DLSTF_ICON_ACTION;

		// Show all?
		if (lister->flags & LISTERF_SHOW_ALL)
			lister->lister->lister.flags |= DLSTF_SHOW_ALL;
		else
			lister->lister->lister.flags &= ~DLSTF_SHOW_ALL;

		// Update position in configuration structure
		lister->lister->lister.pos[0] = lister->dimensions.wd_Normal;

		// Update list format
		lister->lister->lister.format = lister->format;

		// Free current path
		FreeMemH(lister->lister->path);
		lister->lister->path = 0;

		// Get current path
		path = lister->cur_buffer->buf_Path;
		if (path[0] && (lister->lister->path = AllocMemH(0, strlen(path) + 1)))
			strcpy(lister->lister->path, path);

		// Is lister iconified?
		if (lister->flags & LISTERF_ICONIFIED)
		{
			// Update icon position
			lister_update_icon(lister);

			// Save in config
			lister->lister->lister.icon_x = lister->icon_pos_x;
			lister->lister->lister.icon_y = lister->icon_pos_y;
		}
	}
}

void lister_update_icon(Lister *lister)
{
	BackdropObject *object;

	// Got an icon?
	if (!lister->appicon)
		return;

	// Lock main icon list
	lock_listlock(&GUI->backdrop->objects, 0);

	// Find object
	if ((object = backdrop_find_appicon(GUI->backdrop, (AppEntry *)lister->appicon)))
	{
		// Save position
		lister->icon_pos_x = object->pos.Left;
		lister->icon_pos_y = object->pos.Top;
	}

	// Unlock main icon list
	unlock_listlock(&GUI->backdrop->objects);
}

// Update all listers
void listers_update(BOOL close, BOOL reopen)
{
	IPCData *ipc;

	// Close listers?
	if (close)
		IPC_ListCommand(&GUI->lister_list, IPC_HIDE, 0, 0, 1);

	// Lock lister list
	lock_listlock(&GUI->lister_list, FALSE);

	// Go through listers
	for (ipc = (IPCData *)GUI->lister_list.list.lh_Head; ipc->node.mln_Succ; ipc = (IPCData *)ipc->node.mln_Succ)
	{
		Lister *lister;

		// Get lister pointer
		lister = IPCDATA(ipc);

		// Copy font into this lister
		strcpy(lister->font_name, environment->env->font_name[FONT_DIRS]);
		lister->font_size = environment->env->font_size[FONT_DIRS];

		// Update font definition
		lister->lister_font.ta_Name = lister->font_name;
		lister->lister_font.ta_YSize = lister->font_size;
		lister->lister_font.ta_Flags = 0;
		lister->lister_font.ta_Style = 0;

		// Supply toolbar pointer
		if (lister->toolbar_alloc)
			lister->toolbar = lister->toolbar_alloc;
		else
			lister->toolbar = GUI->toolbar;

		// Build tool menu
		lister_build_menu(lister);
	}

	// Unlock lister list
	unlock_listlock(&GUI->lister_list);

	// Reopen listers?
	if (reopen)
	{
		IPC_ListCommand(&GUI->lister_list, IPC_SHOW, 0, (IPTR)GUI->screen_pointer, 0);
	}

	// Just refresh
	else
	{
		// Pens the listers keep still have the old custom colours
		UpdateCustomPens();
		IPC_ListCommand(
			&GUI->lister_list, LISTER_REFRESH_WINDOW, REFRESHF_UPDATE_NAME | REFRESHF_STATUS | REFRESHF_COLOUR, 0, 0);
	}
}

// Initialise a new lister from an old one
void lister_init_new(Cfg_Lister *cfg, Lister *lister)
{
	// Valid lister?
	if (lister_valid_window(lister))
	{
		// Fill out lister
		cfg->lister.pos[0].Left = lister->window->LeftEdge + lister->window->BorderTop;
		cfg->lister.pos[0].Top = lister->window->TopEdge + lister->window->BorderTop;
		cfg->lister.pos[0].Width = lister->window->Width;
		cfg->lister.pos[0].Height = lister->window->Height;
		cfg->lister.format = lister->format;
	}

	// Default settings
	else
	{
		if (GUI->screen_pointer)
		{
			cfg->lister.pos[0].Left = GUI->screen_pointer->MouseX - 32;
			cfg->lister.pos[0].Top = GUI->screen_pointer->MouseY - 32;
		}
		cfg->lister.pos[0].Width = (environment->env->lister_width < 64) ? 320 : environment->env->lister_width;
		cfg->lister.pos[0].Height = (environment->env->lister_height < 32) ? 200 : environment->env->lister_height;
		cfg->lister.format = environment->env->list_format;
	}

	// Got lister?
	if (lister)
	{
		// Icon mode?
		if (lister->flags & LISTERF_VIEW_ICONS)
			cfg->lister.flags |= DLSTF_ICON;

		// Action mode?
		if (lister->flags & LISTERF_ICON_ACTION)
			cfg->lister.flags |= DLSTF_ICON_ACTION;

		// Show all?
		if (lister->flags & LISTERF_SHOW_ALL)
			cfg->lister.flags |= DLSTF_SHOW_ALL;
	}
}

// Tile listers
void lister_tile(long id)
{
	IPCData *ipc;
	TileInfo *info;
	TileNode *node;
	APTR mem;

	// Main window must be open
	if (!GUI->window)
		return;

	// Allocate a quick memory handle
	if (!(mem = NewMemHandle(512, 256, MEMF_CLEAR)) || !(info = AllocMemH(mem, sizeof(TileInfo))))
	{
		FreeMemHandle(mem);
		return;
	}

	// Initialise list
	NewList((struct List *)&info->box_list);

	// Lock lister list
	lock_listlock(&GUI->lister_list, FALSE);

	// And lock layers to stop dragging
	LockLayers(&GUI->screen_pointer->LayerInfo);

	// Go through listers
	for (ipc = (IPCData *)GUI->lister_list.list.lh_Head; ipc->node.mln_Succ; ipc = (IPCData *)ipc->node.mln_Succ)
	{
		Lister *lister;

		// Get lister pointer
		lister = IPCDATA(ipc);

		// Skip lister if it's locked
		if (lister->flags & LISTERF_LOCK_POS)
			continue;

		// Is lister open?
		if (lister_valid_window(lister) || id == LISTER_TILE_ALL)
		{
			// Allocate tile node
			if ((node = AllocMemH(mem, sizeof(TileNode))))
			{
				// Fill in node pointer
				node->ptr = lister;

				// Add to list
				AddTail((struct List *)&info->box_list, (struct Node *)node);
				++info->count;
			}
		}
	}

	// Fill in base window information
	info->base.Left = GUI->window->LeftEdge + GUI->window->BorderLeft;
	info->base.Top = GUI->window->TopEdge + GUI->window->BorderTop;
	info->base.Width = GUI->window->GZZWidth;
	info->base.Height = GUI->window->GZZHeight;

	// Cascade offsets
	info->x_offset = 19;
	info->y_offset = GUI->window->BorderTop;

	// Get mode
	if (id == MENU_TILE_HORIZ)
		info->mode = TILE_TILE_H;
	else if (id == MENU_TILE_VERT || id == LISTER_TILE_ALL)
		info->mode = TILE_TILE_V;
	else
		info->mode = TILE_CASCADE;

	// Do this business
	tile_windows(info);

	// Unlock layers
	UnlockLayers(&GUI->screen_pointer->LayerInfo);

	// Go through tile nodes
	for (node = (TileNode *)info->box_list.mlh_Head; node->node.mln_Succ; node = (TileNode *)node->node.mln_Succ)
	{
		Lister *lister;

		// Get lister pointer
		lister = (Lister *)node->ptr;

		// Is window is open?
		if (lister_valid_window(lister))
		{
			// Does window need to change?
			if (lister->window->LeftEdge != node->dims.Left || lister->window->TopEdge != node->dims.Top ||
				lister->window->Width != node->dims.Width || lister->window->Height != node->dims.Height)
			{
				// Change window position
				ChangeWindowBox(lister->window, node->dims.Left, node->dims.Top, node->dims.Width, node->dims.Height);
			}
		}

		// Otherwise, store new coordinates
		else
		{
			lister->dimensions.wd_Normal = node->dims;
			lister->lister->lister.pos[0] = node->dims;
		}
	}

	// Unlock lister list
	unlock_listlock(&GUI->lister_list);

	// Free memory
	FreeMemHandle(mem);
}

// Set selector position
BOOL set_list_selector(Lister *lister, long pos)
{
	// Bounds check new position
	if (pos > lister->text_height - 1)
		pos = lister->text_height - 1;
	if (lister->cur_buffer->buf_VertOffset + pos >= lister->cur_buffer->buf_TotalEntries[0])
		pos = lister->cur_buffer->buf_TotalEntries[0] - lister->cur_buffer->buf_VertOffset - 1;
	if (pos < 0)
		pos = 0;

	// Different position?
	if (pos != lister->selector_pos)
	{
		// Erase old selector
		show_list_selector(lister, lister->selector_pos, 1);

		// Show new selector position
		lister->selector_pos = pos;
		show_list_selector(lister, pos, 0);
		return 1;
	}

	return 0;
}

// Add a new waiter
BOOL lister_new_waiter(Lister *lister, IPCMessage *msg, struct Message *msg2, short type)
{
	ListerWaiter *wait;

	// Allocate new waiter
	if (!(wait = AllocVec(sizeof(ListerWaiter), MEMF_CLEAR)))
	{
		// Fail
		if (msg2)
			ReplyMsg(msg2);
		return 0;
	}

	// Fill out and add to list
	wait->msg = msg;
	wait->msg2 = msg2;
	wait->type = type;
	AddTail((struct List *)&lister->wait_list, (struct Node *)wait);

	return 1;
}

// Relieve waiters
void lister_relieve_waiters(Lister *lister, short type)
{
	ListerWaiter *waiter, *next;

	// Go through waiters
	for (waiter = (ListerWaiter *)lister->wait_list.mlh_Head; waiter->node.mln_Succ; waiter = next)
	{
		// Cache next
		next = (ListerWaiter *)waiter->node.mln_Succ;

		// Check type
		if (type == LISTERWAIT_ALL || waiter->type == type ||
			(waiter->type == LISTERWAIT_AUTO_CMD && type == LISTERWAIT_UNBUSY))
		{
			// Remove
			Remove((struct Node *)waiter);

			// Reply to message's message
			if (waiter->msg2)
				ReplyMsg(waiter->msg2);

			// Send command to ourselves?
			if (waiter->type == LISTERWAIT_AUTO_CMD && type != LISTERWAIT_ALL)
				PutMsg(lister->ipc->command_port, (struct Message *)waiter->msg);

			// Reply to this message
			else
				IPC_Reply(waiter->msg);

			// Free waiter
			FreeVec(waiter);
		}
	}
}

// Try and be smart about setting source/destination
void lister_smart_source(Lister *ours)
{
	IPCData *ipc;
	Lister *lister;
	Lister *source = 0, *dest = 0;
	Lister *set_source = 0, *set_dest = 0;

	// Get list lock
	lock_listlock(&GUI->lister_list, FALSE);

	// Lister supplied?
	if (ours)
	{
		// Already used?
		if (ours->flags & (LISTERF_BUSY | LISTERF_SOURCE | LISTERF_DEST | LISTERF_STORED_SOURCE | LISTERF_STORED_DEST))
			ours = 0;

		// View icon mode?
		else if (ours->flags & LISTERF_VIEW_ICONS && !(ours->flags & LISTERF_ICON_ACTION))
			ours = 0;

		// Iconified?
		else if (ours->flags & LISTERF_ICONIFIED)
			ours = 0;
	}

	// Go through listers
	for (ipc = (IPCData *)GUI->lister_list.list.lh_Head; ipc->node.mln_Succ; ipc = (IPCData *)ipc->node.mln_Succ)
	{
		// Get lister
		lister = IPCDATA(ipc);

		// Dual lister sides keep source/dest roles local to their pair.
		if (lister_dual_is_side(lister))
			continue;

		// Is lister available?
		if (!(lister->flags & (LISTERF_BUSY | LISTERF_ICONIFIED)) &&
			(!(lister->flags & LISTERF_VIEW_ICONS) || lister->flags & LISTERF_ICON_ACTION))
		{
			// Is this a source?
			if (lister->flags & LISTERF_SOURCE)
				source = lister;

			// Or a destination?
			else if (lister->flags & LISTERF_DEST)
				dest = lister;

			// Or a stored source?
			else if (lister->flags & LISTERF_STORED_SOURCE)
				set_source = lister;

			// Or a stored destination?
			else if (lister->flags & LISTERF_STORED_DEST)
				set_dest = lister;

			// Make this the new source?
			else if (!set_source)
				set_source = lister;

			// Make it the new destination?
			else if (!set_dest)
				set_dest = lister;
		}
	}

	// Need to set a source?
	if (!source && set_source)
	{
		IPC_Command((ours) ? ours->ipc : set_source->ipc, LISTER_MAKE_SOURCE, 0, 0, 0, 0);
		ours = 0;
	}
	else if (set_source)
		set_dest = set_source;

	// Need to set a destination?
	if (!dest && set_dest)
	{
		IPC_Command((ours) ? ours->ipc : set_dest->ipc, LISTER_MAKE_DEST, 0, 0, 0, 0);
	}

	// Unlock list lock
	unlock_listlock(&GUI->lister_list);
}

// Fix lister priorities
void lister_fix_priority(Lister *lister)
{
	// Running for a lister?
	if (lister)
	{
		short pri;

		// Busy?
		if (lister->flags & LISTERF_BUSY)
			pri = lister->busy_pri;
		else
			pri = lister->normal_pri;

		// Set lister priority
		SetTaskPri((struct Task *)lister->ipc->proc, pri);

		// Got a locker?
		if (lister->locker_ipc)
			SetTaskPri((struct Task *)lister->locker_ipc->proc, pri);
		return;
	}

	// Send command to all listers
	IPC_ListCommand(&GUI->lister_list,
					IPC_PRIORITY,
					environment->env->settings.pri_lister[1],
					environment->env->settings.pri_lister[0],
					FALSE);
}

// Check a lister is valid (list must be locked)
BOOL lister_check_valid(Lister *lister)
{
	IPCData *ipc;

	// Go through listers
	for (ipc = (IPCData *)GUI->lister_list.list.lh_Head; ipc->node.mln_Succ; ipc = (IPCData *)ipc->node.mln_Succ)
	{
		// Check for a match
		if (lister == (Lister *)IPCDATA(ipc))
			return TRUE;
	}

	return FALSE;
}

// Open a new lister for a path
Lister *lister_open_new(char *path, BackdropObject *object, struct Window *parent, Lister *lister)
{
	Cfg_Lister *cfg;
	short mode;
	position_rec *pos;
	BPTR lock;
	char *full_path, device[40], *ptr;

	// Allocate buffer for full path
	if (!(full_path = AllocVec(512, 0)))
		return 0;

	// Get a new lister definition
	if (!(cfg = NewLister(path)))
	{
		FreeVec(full_path);
		return 0;
	}

	// Lock path
	if ((lock = Lock(path, ACCESS_READ)))
	{
		// Get full path for position
		NameFromLock(lock, full_path, 512);
		UnLock(lock);
	}
	else
		strcpy(full_path, path);

	// Get device name
	strncpy(device, path, 39);
	if ((ptr = strchr(device, ':')))
		*(ptr + 1) = 0;

	// Check path is terminated
	AddPart(full_path, "", 512);

	// Get position
	pos = GetListerPosition(full_path,
							device,
							(object) ? object->icon : 0,
							&cfg->lister.pos[0],
							&mode,
							&cfg->lister.format,
							parent,
							lister,
							0);

	// Free buffer
	FreeVec(full_path);

	// Create new lister
	if (!(lister = lister_new(cfg)))
		return 0;

	// Store position record
	lister->pos_rec = pos;

	// Store backdrop pointer
	lister->backdrop = object;

	// Open in icon mode?
	if (mode & LISTERMODE_ICON)
	{
		// Force new lister into icon view mode
		lister->flags |= LISTERF_VIEW_ICONS;
		if (mode & LISTERMODE_ICON_ACTION)
			lister->flags |= LISTERF_ICON_ACTION;

		// Show all?
		if (mode & LISTERMODE_SHOW_ALL)
			lister->flags |= LISTERF_SHOW_ALL;
	}

	return lister;
}

// Valid window?
struct Window *lister_valid_window(Lister *lister)
{
	return (lister && lister->window && !(lister->more_flags & LISTERF_TITLEBARRED)) ? lister->window : 0;
}

// Zoom the window
void lister_zoom_window(Lister *lister)
{
	// No window?
	if (!lister->window)
		return;

	// In zoomed mode?
	if (lister->more_flags & LISTERF_TITLEBARRED)
	{
		// Clear flag
		lister->more_flags &= ~LISTERF_TITLEBARRED;

		// Add scrollers back again
		if (lister->backdrop_info->vert_scroller)
			AddGList(lister->window, lister->backdrop_info->vert_scroller, -1, -1, 0);

		// Resize to restored dimensions
		ChangeWindowBox(lister->window,
						lister->window->LeftEdge,
						lister->window->TopEdge,
						lister->restore_dims.Width,
						lister->restore_dims.Height);

		// Set window limits
		WindowLimits(lister->window, lister->win_limits.Left, lister->win_limits.Top, (UWORD)~0, (UWORD)~0);

		// Restore pointer to current lister
		GUI->current_lister = lister;
	}

	// Ben Vost zoom mode?
	else if (environment->env->lister_options & LISTEROPTF_VOSTY_ZOOM)
	{
		// Save old dimensions
		lister->restore_dims.Width = lister->window->Width;
		lister->restore_dims.Height = lister->window->Height;

		// Set window limits
		WindowLimits(lister->window, lister->win_limits.Left * 2, lister->window->BorderTop, (UWORD)~0, (UWORD)~0);

		// Remove path field
		if (lister->flags & LISTERF_PATH_FIELD)
			lister_remove_pathfield(lister, 0);

		// Remove scrollers
		if (lister->backdrop_info->vert_scroller)
			RemoveGList(lister->window, lister->backdrop_info->vert_scroller, (lister->gauge_gadget) ? 7 : 6);

		// Shrink to title bar
		ChangeWindowBox(lister->window,
						lister->window->LeftEdge,
						lister->window->TopEdge,
						lister->win_limits.Left * 2,
						lister->window->BorderTop);

		// Set flag
		lister->more_flags |= LISTERF_TITLEBARRED;

		// Clear pointer to current lister (titlebar-minimized listers aren't eligible)
		Forbid();
		if (GUI->current_lister == lister)
			GUI->current_lister = 0;
		Permit();
	}

	// Normal mode
	else
	{
		ZipWindow(lister->window);
		return;
	}

	// Refresh title bar
	lister_show_name(lister);
}
