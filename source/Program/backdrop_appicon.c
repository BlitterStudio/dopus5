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

// Read AppIcon list
void backdrop_read_appicons(BackdropInfo *info, short flags)
{
	AppEntry *icon;
	BackdropObject *object;

	// Lock backdrop list
	if (flags & BDAF_LOCK)
		lock_listlock(&info->objects, 1);

	// Lock AppList
	icon = LockAppList();

	// Go through AppIcons
	while ((icon = NextAppEntry(icon, APP_ICON)))
	{
		// Showing AppIcons, or a local icon?
		if (environment->env->display_options & DISPOPTF_SHOW_APPICONS || icon->flags & APPENTF_LOCAL)
		{
			// See if icon is already in list
			if (!(object = backdrop_find_appicon(info, icon)))
			{
				// Add this AppIcon
				object = backdrop_add_appicon(icon, info, flags & BDAF_SHOW);
			}

			// Mark as ok if we have an icon
			if (object)
				object->flags |= BDOF_OK;
		}
	}

	// Unlock AppList
	UnlockAppList();

	// Go through backdrop list
	for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ;)
	{
		BackdropObject *next = (BackdropObject *)object->node.ln_Succ;

		// AppIcon?
		if (object->type == BDO_APP_ICON)
		{
			// Marked as ok?
			if (object->flags & BDOF_OK)
				object->flags &= ~BDOF_OK;

			// Not marked; remove icon
			else
			{
				// Erase icon
				if (flags & BDAF_SHOW)
					backdrop_erase_icon(info, object, 0);

				// Remove from list
				backdrop_remove_object(info, object);
			}
		}

		// Get next
		object = next;
	}

	// Unlock backdrop list
	if (flags & BDAF_LOCK)
		unlock_listlock(&info->objects);
}

// Add an AppIcon backdrop object
BackdropObject *backdrop_add_appicon(AppEntry *appicon, BackdropInfo *info, short flags)
{
	BackdropObject *object;
	leftout_record *left;

	// Lock backdrop list
	if (flags & BDAF_LOCK)
		lock_listlock(&info->objects, 0);

	// See if AppIcon is already in list
	if ((object = backdrop_find_appicon(info, appicon)))
	{
		// Image changed?
		if (flags & BDAF_CHANGE)
		{
			// Has name changed?
			if (strcmp(object->name, appicon->text))
			{
				// Erase object label
				backdrop_render_object(info, object, BRENDERF_CLEAR | BRENDERF_CLIP | BRENDERF_LABEL);

				// Get new name
				strcpy(object->name, appicon->text);
			}

			// Update locked state
			if (appicon->flags & APPENTF_LOCKED)
				object->flags |= BDOF_LOCKED;
			else
				object->flags &= ~BDOF_LOCKED;

			// Update busy state
			if (appicon->flags & APPENTF_BUSY)
				object->flags |= BDOF_BUSY;
			else
				object->flags &= ~BDOF_BUSY;

			// Update ghosted state
			if (appicon->flags & APPENTF_GHOSTED)
				object->flags |= BDOF_GHOSTED;
			else
				object->flags &= ~BDOF_GHOSTED;

			// Did image change?
			if (flags & BDAF_NEW_IMAGE)
			{
				// Need new masks?
				if (!backdrop_icon_border(object))
				{
					// Get new masks
					backdrop_get_masks(object);
				}
			}

			// Show image
			backdrop_render_object(info, object, BRENDERF_CLIP);
		}
	}

	// Unlock backdrop list
	if (flags & BDAF_LOCK)
		unlock_listlock(&info->objects);

	// Already in list?
	if (object || flags & BDAF_CHANGE)
		return object;

	// Valid text?
	if (appicon->text && *appicon->text)
	{
		// Lock position list
		lock_listlock(&GUI->positions, FALSE);

		// Look for entry for icon
		for (left = (leftout_record *)&GUI->positions.list.lh_Head; left->node.ln_Succ;
			 left = (leftout_record *)left->node.ln_Succ)
		{
			// Leftout?
			if (left->node.ln_Type == PTYPE_APPICON)
			{
				// Match this icon?
				if (strcmp(left->icon_label, appicon->text) == 0)
				{
					// Set position in icon
					((struct DiskObject *)appicon->object)->do_CurrentX = left->icon_x;
					((struct DiskObject *)appicon->object)->do_CurrentY = left->icon_y;
					break;
				}
			}
		}

		// Unlock position list
		unlock_listlock(&GUI->positions);
	}

	// Allocate a new object
	if ((object = backdrop_new_object(info, appicon->text, 0, BDO_APP_ICON)))
	{
		// Lock backdrop list
		if (flags & BDAF_LOCK)
			lock_listlock(&info->objects, 1);

		// Store icon and owner
		object->icon = appicon->object;
		object->misc_data = (IPTR)appicon;

		// Background colour?
		if (((AppEntry *)appicon)->flags & APPENTF_BACKGROUND)
		{
			// Store background colour
#ifdef USE_64BIT
			object->pen = ((AppEntry *)appicon)->data;
#else
			object->size = ((AppEntry *)appicon)->data;
#endif
			object->flags |= BDOF_BACKGROUND;
		}

		// Locked?
		if (((AppEntry *)appicon)->flags & APPENTF_LOCKED)
			object->flags |= BDOF_LOCKED;

		// Special?
		if (((AppEntry *)appicon)->flags & APPENTF_SPECIAL)
			object->flags |= BDOF_SPECIAL;

		// Fix size and position
		backdrop_get_icon(info, object, GETICON_CD);

		// Add to backdrop list
		AddTail(&info->objects.list, &object->node);

		// Unlock backdrop list
		if (flags & BDAF_LOCK)
			unlock_listlock(&info->objects);

		// Refresh display?
		if (flags & BDAF_SHOW)
		{
			// Position new icon
			backdrop_place_object(info, object);

			// Show desktop
			backdrop_show_objects(info, BDSF_RECALC);
		}
	}

	return object;
}

// Remove an AppIcon backdrop object
void backdrop_rem_appicon(BackdropInfo *info, AppEntry *icon)
{
	BackdropObject *object;
	BOOL redraw = 0;

	// Lock backdrop list
	lock_listlock(&info->objects, 1);

	// Find AppIcon
	if ((object = backdrop_find_appicon(info, icon)))
	{
		// Erase object
		backdrop_erase_icon(info, object, 0);

		// Remove object
		backdrop_remove_object(info, object);
		redraw = 1;
	}

	// Unlock backdrop list
	unlock_listlock(&info->objects);

	// Recalculate backdrop
	if (redraw)
		backdrop_calc_virtual(info);
}

// Look for an AppIcon (list must be locked)
BackdropObject *backdrop_find_appicon(BackdropInfo *info, AppEntry *appicon)
{
	BackdropObject *object;

	// Go through backdrop list
	for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ;
		 object = (BackdropObject *)object->node.ln_Succ)
	{
		// Check if we already have this appicon
		if (object->type == BDO_APP_ICON && object->misc_data == (IPTR)appicon)
			return object;
	}

	return 0;
}

// Build an AppMessage from dropped objects
DOpusAppMessage *backdrop_appmessage(BackdropInfo *info, BOOL need_obj)
{
	DOpusAppMessage *msg;
	BackdropObject *object, *first;
	short arg = 0, count = 0, beep = 0;

	// Go through backdrop list, count selections
	for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ;
		 object = (BackdropObject *)object->node.ln_Succ)
	{
		// Set beep if wrong type
		if (object->type == BDO_GROUP || object->type == BDO_BAD_DISK)
			beep = 1;

		// Selected?
		else if (object->state)
			++count;
	}

	// No objects?
	if (!count && need_obj)
	{
		// Beep?
		if (beep)
			DisplayBeep(info->window->WScreen);
		return 0;
	}

	// Allocate AppMessage
	if (!(msg = AllocAppMessage(global_memory_pool, GUI->appmsg_port, count)))
		return 0;

	// Set icon flag
	msg->da_Flags |= DAPPF_ICON_DROP;

	// Pointer to first selected object
	if ((first = info->last_sel_object))
	{
		// Save drag offset
		msg->da_DragOffset.x = first->image_rect.MinX - first->pos.Left + first->drag_x_offset - info->offset_x;
		msg->da_DragOffset.y = first->image_rect.MinY - first->pos.Top + first->drag_y_offset - info->offset_y;
	}

	// Go through backdrop list, fill in arguments
	for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ && arg < count;
		 object = (BackdropObject *)object->node.ln_Succ)
	{
		// Selected?
		if (object->state && object->type != BDO_BAD_DISK && object->type != BDO_GROUP)
		{
			// Got position array?
			if (first)
			{
				// Store object position relative to first object
				msg->da_DropPos[arg].x = object->pos.Left - first->pos.Left;
				msg->da_DropPos[arg].y = object->pos.Top - first->pos.Top;
			}

			// AppIcon?
			if (object->type == BDO_APP_ICON)
			{
				// Copy name
				SetWBArg(msg, arg, 0, object->name, global_memory_pool);
			}

			// Is object a directory?
			else if (object->icon->do_Type == WBDRAWER || object->icon->do_Type == WBGARBAGE)
			{
				BPTR old, lock;

				// Get icon lock
				if ((lock = backdrop_icon_lock(object)))
				{
					// Change directory
					old = CurrentDir(lock);

					// Lock sub-directory
					if (!(msg->da_Msg.am_ArgList[arg].wa_Lock = Lock(object->name, ACCESS_READ)))
					{
						char *name;

						// Couldn't lock directory, so steal lock for parent
						if ((name = AllocVec(strlen(object->name) + 8, 0)))
						{
							msg->da_Msg.am_ArgList[arg].wa_Lock = lock;
							lock = 0;

							// Get filename
							strcpy(name, object->name);
							strcat(name, ".info");
							SetWBArg(msg, arg, 0, name, global_memory_pool);
						}
					}

					// Restore directory
					CurrentDir(old);
					if (lock)
						UnLock(lock);
				}
			}

			// Otherwise
			else
			{
				// Duplicate lock (unless we've already got one)
				if (!msg->da_Msg.am_ArgList[arg].wa_Lock)
					msg->da_Msg.am_ArgList[arg].wa_Lock = backdrop_icon_lock(object);

				// If not a disk, copy filename
				if (object->icon->do_Type != WBDISK)
				{
					SetWBArg(msg, arg, 0, object->name, global_memory_pool);
				}
			}

			// If no filename given, create a dummy one
			if (!msg->da_Msg.am_ArgList[arg].wa_Name)
				SetWBArg(msg, arg, 0, 0, global_memory_pool);
			++arg;
		}
	}

	// Set flags in message
	set_appmsg_data(msg, (IPTR)info->lister, DROPF_ICON_MODE, 0);
	return msg;
}

// Drop stuff on an AppWindow (nb: will be in Forbid() when we get here)
void backdrop_drop_appwindow(BackdropInfo *info, struct AppWindow *appwindow, short x, short y)
{
	DOpusAppMessage *msg;
	struct MsgPort *port;

	// Build AppMessage
	if ((msg = backdrop_appmessage(info, 1)))
	{
		// Complete message
		msg->da_Msg.am_Type = MTYPE_APPWINDOW;
		msg->da_Msg.am_MouseX = x;
		msg->da_Msg.am_MouseY = y;
		port = WB_AppWindowData(appwindow, &msg->da_Msg.am_ID, &msg->da_Msg.am_UserData);

		// Send the message
		PutMsg(port, (struct Message *)msg);
	}
}

// Send message to an AppIcon
void backdrop_appicon_message(BackdropObject *object, unsigned short flags)
{
	struct AppSnapshotMsg *msg;
	struct MsgPort *port;

	// Allocate message
	if (!(msg = AllocMemH(global_memory_pool, sizeof(struct AppSnapshotMsg))))
		return;

	// Fill out message
	msg->ap_msg.am_Message.mn_ReplyPort = GUI->appmsg_port;
	msg->ap_msg.am_Version = AM_VERSION;
	CurrentTime(&msg->ap_msg.am_Seconds, &msg->ap_msg.am_Micros);

	// Message type
	msg->ap_msg.am_Type = MTYPE_APPSNAPSHOT;

	// Set icon position
	msg->position_x = object->pos.Left;
	msg->position_y = object->pos.Top;

	// Adjust for border
	if (backdrop_icon_border(object))
	{
		msg->position_x -= ICON_BORDER_X;
		msg->position_y -= ICON_BORDER_Y_TOP;
	}

	// Adjust
	// Menu operation?
	if (flags & BAPPF_MENU)
	{
		// Get ID
		msg->id = BAPPF_ID(flags);

		// Set flags
		msg->flags |= APPSNAPF_MENU;
		if (flags & BAPPF_HELP)
			msg->flags |= APPSNAPF_HELP;
	}

	// Close?
	else if (flags & BAPPF_CLOSE)
		msg->flags |= APPSNAPF_CLOSE;

	// Unsnapshot?
	else if (flags & BAPPF_UNSNAPSHOT)
		msg->flags |= APPSNAPF_UNSNAPSHOT;

	// Info?
	else if (flags & BAPPF_INFO)
		msg->flags |= APPSNAPF_INFO;

	// Get AppInfo
	port = WB_AppWindowData((struct AppWindow *)object->misc_data, &msg->ap_msg.am_ID, &msg->ap_msg.am_UserData);

	// Send the message
	PutMsg(port, (struct Message *)msg);
}
