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

// Start dragging
BOOL backdrop_start_drag(BackdropInfo *info, short x, short y)
{
	BackdropObject *object;

	// Set drag flag
	info->flags |= BDIF_DRAGGING;

	// Copy RastPort and initialise GELs
	GUI->drag_screen_rp = GUI->screen_pointer->RastPort;
	GUI->drag_screen_rp.GelsInfo = &GUI->drag_info;
	InitGels(&GUI->drag_head, &GUI->drag_tail, &GUI->drag_info);

	// Custom dragging?
	if (DragCustomOk(GUI->drag_screen_rp.BitMap))
	{
		// Set custom flag
		info->flags |= BDIF_CUSTOM_DRAG;
	}

	// Normal dragging
	else
		info->flags &= ~BDIF_CUSTOM_DRAG;

	// Reset saved coords
	info->last_x = x;
	info->last_y = y;

	// Stop menu actions, start mouse reporting
	info->window->Flags |= WFLG_RMBTRAP | WFLG_REPORTMOUSE;

	// Turn ticks on
	ModifyIDCMP(info->window, info->window->IDCMPFlags | IDCMP_INTUITICKS);
	info->tick_count = 1;
	info->last_tick = 0;

	// Get start time
	CurrentTime(&info->drag_sec, &info->drag_mic);

	// Last selected icon?
	if ((object = info->last_sel_object) && object->state)
	{
		// Check icon isn't locked
		if (object->flags & BDOF_LOCKED)
		{
			// Can't drag at all
			backdrop_stop_drag(info);
			return 0;
		}
	}

	// Go through backdrop list
	for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ;
		 object = (BackdropObject *)object->node.ln_Succ)
	{
		// Is object selected?
		if (object->state)
		{
			// Check icon isn't locked
			if (object->flags & BDOF_LOCKED)
				continue;

			// Start dragging
			if (!(backdrop_drag_object(info, object)))
			{
				// Failed
				backdrop_stop_drag(info);
				return 0;
			}
		}
	}

	return 1;
}

// Start dragging an object
BOOL backdrop_drag_object(BackdropInfo *info, BackdropObject *object)
{
	struct Image *image;
	UWORD *mask;
	ULONG flags = 0;

	// Is object already being dragged?
	if (object->drag_info)
		return 1;

	// Is there a select image?
	if (object->icon->do_Gadget.SelectRender && (object->icon->do_Gadget.Flags & GFLG_GADGHIMAGE))
	{
		image = (struct Image *)object->icon->do_Gadget.SelectRender;
	}

	// No
	else
		image = (struct Image *)object->icon->do_Gadget.GadgetRender;

	// Get flags
	if (info->flags & BDIF_CUSTOM_DRAG)
	{
		// Custom drag
		flags = DRAGF_CUSTOM;

		// Opaque?
		if (environment->env->desktop_flags & DESKTOPF_QUICK_DRAG)
			flags |= DRAGF_NO_MASK;
	}

	// Get drag info
	if (!(object->drag_info = GetDragInfo(info->window,
										  &GUI->drag_screen_rp,
										  (image->Width < object->pos.Width) ? object->pos.Width : image->Width,
										  (image->Height < object->pos.Height) ? object->pos.Height : image->Height,
										  flags)))
		return 0;

	// Get image to drag
	backdrop_draw_object(info, object, 0, &object->drag_info->drag_rp, 0, 0);

	// Get mask
	if (environment->env->desktop_flags & DESKTOPF_QUICK_DRAG)
		object->drag_info->flags |= DRAGF_OPAQUE;
	else if (environment->env->desktop_flags & DESKTOPF_TRANSPARENCY)
		object->drag_info->flags |= DRAGF_TRANSPARENT;
	else
		object->drag_info->flags |= DRAGF_TRANSPARENT | DRAGF_OPAQUE;
	GetDragMask(object->drag_info);

	// GetDragMask() sees only pens; where the icon's own mask is set, the image
	// is opaque even in pen 0 (palette colours remapped to the background)
	if (object->drag_info->bob.ImageShadow &&
		(mask = object->image_mask[(image == (struct Image *)object->icon->do_Gadget.GadgetRender) ? 0 : 1]))
	{
		short row, col, columns = (image->Width + 15) >> 4, drag_columns = (object->drag_info->width + 15) >> 4;
		UWORD *shadow = (UWORD *)object->drag_info->bob.ImageShadow;

		for (row = 0; row < image->Height; row++, mask += columns, shadow += drag_columns)
		{
			for (col = 0; col < columns; col++)
				shadow[col] |= mask[col];
		}
	}

	return 1;
}

// Stop dragging
BOOL backdrop_stop_drag(BackdropInfo *info)
{
	BackdropObject *object;
	BOOL ret = 0;

	// Rubber-banding?
	if (info->flags & BDIF_RUBBERBAND)
	{
		// Erase box
		backdrop_rubber_band(info, 0);

		// Clear flag and mouse reporting (IDCMP_INTUITICKS stays on for
		// lister toolbar tooltip hover tracking)
		info->flags &= ~BDIF_RUBBERBAND;
		info->window->Flags &= ~(WFLG_REPORTMOUSE | WFLG_RMBTRAP);

		// If icon positioning, unlock layers
		if (GUI->flags & GUIF_DRAGGING)
		{
			UnlockLayers(&info->window->WScreen->LayerInfo);
			GUI->flags &= ~GUIF_DRAGGING;
		}
	}

	// Dragging?
	else if (info->flags & BDIF_DRAGGING)
	{
		// Clear drag flag
		info->flags &= ~BDIF_DRAGGING;

		// Are we dragging?
		if (GUI->flags & GUIF_DRAGGING)
		{
			ULONG sec, mic, mic1, mic2;

			// Get stop time
			CurrentTime(&sec, &mic);

			// Normal dragging?
			if (!(info->flags & BDIF_CUSTOM_DRAG))
			{
				// Go through backdrop list
				for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ;
					 object = (BackdropObject *)object->node.ln_Succ)
				{
					// Is object being dragged?
					if (object->drag_info)
					{
						// Hide object
						RemBob(&object->drag_info->bob);
					}
				}
			}

			// Draw GELs list
			SortGList(&GUI->drag_screen_rp);
			DrawDragList(&GUI->drag_screen_rp,
						 &info->window->WScreen->ViewPort,
						 (info->flags & BDIF_CUSTOM_DRAG) ? DRAGF_CUSTOM | DRAGF_REMOVE : 0);

			// Unlock layer
			UnlockLayers(&info->window->WScreen->LayerInfo);

			// Get drag time deltas
			mic1 = (sec * 1000000) + mic;
			mic2 = (info->drag_sec * 1000000) + info->drag_mic;

			// If drag was less than a fifth of a second, nothing moves
			if (mic1 < mic2 + 200000)
				ret = 1;
		}

		// Go through backdrop list
		for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ;
			 object = (BackdropObject *)object->node.ln_Succ)
		{
			// Was object being dragged?
			if (object->drag_info)
			{
				if ((info->flags & BDIF_CUSTOM_DRAG) &&	 // is custom drag?
					(GUI->flags & GUIF_DRAGGING))		 // actually did drag? i.e. AddVSprite called
				{
					// Now is the right time to remove the object from the GELs list. This is
					// done in a different place than RemBob for regular dragging. RemBob
					// needs to be done _before_ DrawDragList()->DrawGList(). With custom
					// dragging the object needs to _stay_ in the GELs list before the call to
					// DrawDragList(..., DRAGF_CUSTOM|DRAGF_REMOVE).
					// Thus, remove the object from the GELs list here instead.
					RemVSprite(&object->drag_info->sprite);
				}
				// Free drag
				FreeDragInfo(object->drag_info);
				object->drag_info = 0;
			}
		}

		// Clear drag flag
		GUI->flags &= ~GUIF_DRAGGING;

		// Start menu actions, stop mouse reporting (IDCMP_INTUITICKS stays
		// on for lister toolbar tooltip hover tracking)
		info->window->Flags &= ~(WFLG_RMBTRAP | WFLG_REPORTMOUSE);

		// Unlock backdrop list
		unlock_listlock(&info->objects);
	}

	return ret;
}

// Show drag objects
void backdrop_show_drag(BackdropInfo *info, BackdropObject *first, short x, short y)
{
	BackdropObject *object;
	BOOL ok = 0;

	// We also won't move if the mouse position hasn't changed
	if (info->last_x == x && info->last_y == y)
		return;

	// Save coords
	info->last_x = x;
	info->last_y = y;

	// Go through backdrop list
	for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ;
		 object = (BackdropObject *)object->node.ln_Succ)
	{
		// Is object being dragged?
		if (object->drag_info)
		{
			short ox, oy;

			// Get object position relative to first object
			ox = object->pos.Left - first->pos.Left;
			oy = object->pos.Top - first->pos.Top;

			// Offset by mouse coordinates
			ox += x;
			oy += y;

			// Offset by drag offset and window offset
			ox += first->drag_x_offset + info->size.MinX - info->offset_x;
			oy += first->drag_y_offset + info->size.MinY - info->offset_y;

			// Position image
			object->drag_info->sprite.X = ox;
			object->drag_info->sprite.Y = oy;
			ok = 1;

			// Not dragging yet?
			if (!(GUI->flags & GUIF_DRAGGING))
			{
				// Add bob to list
				if (!(info->flags & BDIF_CUSTOM_DRAG))
				{
					AddBob(&object->drag_info->bob, &GUI->drag_screen_rp);
				}
				else
				{
					// Custom dragging is abusing the GELs list merely for keeping track of
					// the dragged objects, thus use AddVSprite() to link it into the list
					AddVSprite(&object->drag_info->sprite, &GUI->drag_screen_rp);
				}
			}
		}
	}

	// Update GEL list?
	if (ok)
	{
		// Just started dragging?
		if (!(GUI->flags & GUIF_DRAGGING))
		{
			// Set drag flag
			GUI->flags |= GUIF_DRAGGING;

			// Lock layer
			LockLayers(&info->window->WScreen->LayerInfo);
		}

		// Sort GELs list
		SortGList(&GUI->drag_screen_rp);
		DrawDragList(&GUI->drag_screen_rp,
					 &info->window->WScreen->ViewPort,
					 (info->flags & BDIF_CUSTOM_DRAG) ? DRAGF_CUSTOM : 0);
		WaitTOF();
	}
}

// Drop some objects on another one
void backdrop_drop_object(BackdropInfo *info, BackdropObject *on_object)
{
	BackdropObject *object;
	char *buf, *dbuf;
	D_S(struct InfoData, data)
	BPTR lock;

	if (!(buf = AllocVec(512, 0)))
		return;
	dbuf = buf + 256;

	// Drop on an AppIcon
	if (on_object->type == BDO_APP_ICON)
	{
		DOpusAppMessage *msg;
		struct MsgPort *port;

		// Is the icon busy?
		if (on_object->flags & BDOF_BUSY)
		{
			DisplayBeep(info->window->WScreen);
			FreeVec(buf);
			return;
		}

		// Build AppMessage
		if (!(msg = backdrop_appmessage(info, 1)))
		{
			FreeVec(buf);
			return;
		}

		// Set message type
		msg->da_Msg.am_Type = MTYPE_APPICON;

		// Get AppInfo
		port = WB_AppWindowData((struct AppWindow *)on_object->misc_data, &msg->da_Msg.am_ID, &msg->da_Msg.am_UserData);

		// Send the message
		PutMsg(port, (struct Message *)msg);
		FreeVec(buf);
		return;
	}

	// Only support dropping on disks at the moment
	if ((on_object->type != BDO_DISK && on_object->type != BDO_BAD_DISK) ||
		on_object->flags & (BDOF_ASSIGN | BDOF_CACHE))
	{
		DisplayBeep(info->window->WScreen);
		FreeVec(buf);
		return;
	}

	// Go through backdrop list
	for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ;
		 object = (BackdropObject *)object->node.ln_Succ)
	{
		// Selected disk?
		if (object->type == BDO_DISK && !(object->flags & (BDOF_ASSIGN | BDOF_CACHE)) && object->state)
		{
			BPTR lock;

			// Get disk lock
			if ((lock = backdrop_icon_lock(object)))
			{
				// This is the source
				Info(lock, data);

				// Get device name
				DeviceFromLock(lock, buf);

				// Unlock lock
				UnLock(lock);
			}
			break;
		}
	}

	// Didn't get anything?
	if (!object->node.ln_Succ)
	{
		DisplayBeep(info->window->WScreen);
		FreeVec(buf);
		return;
	}

	// Drop on a bad disk?
	if (on_object->type == BDO_BAD_DISK)
	{
		// Get device name
		strcpy(dbuf, on_object->name);
	}

	// Real disk
	else
	{
		// Get destination disk lock
		if (!(lock = backdrop_icon_lock(on_object)))
		{
			FreeVec(buf);
			return;
		}

		// Destination is the one we dropped it on
		Info(lock, data);

		// Get device name
		DeviceFromLock(lock, dbuf);
		UnLock(lock);
	}

	// Launch diskcopy
	function_launch(FUNCTION_RUN_FUNCTION, def_function_diskcopy, 0, 0, 0, 0, 0, 0, BuildArgArray(buf, dbuf, 0), 0, 0);

	FreeVec(buf);
}
