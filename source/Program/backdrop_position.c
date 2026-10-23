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
#include "iconpos.h"

// Arrange backdrop objects
long backdrop_arrange_objects(BackdropInfo *info)
{
	BackdropObject *object;
	long start_x = 0, start_y = 0, last_w = 0, last_h = 0, count = 0;
	short width, height;
	struct MinList *list = 0;

	// Valid list?
	if (IsListEmpty(&info->objects.list))
	{
		return 0;
	}

	// Desktop window
	if (info->flags & BDIF_MAIN_DESKTOP)
	{
		// Get position list
		list = backdrop_build_poslist(
			info, ICONPOSF_DISKS | ICONPOSF_APPICONS | ICONPOSF_LISTERS | ICONPOSF_GROUPS | ICONPOSF_LEFTOUTS);
	}

	// Lock backdrop list
	lock_listlock(&info->objects, 0);

	// If window is backdrop, bounds-check icons
	if (info->window && info->window->Flags & WFLG_BACKDROP)
	{
		// Go through backdrop list
		for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ;
			 object = (BackdropObject *)object->node.ln_Succ)
		{
			// Check if object is outside of display
			if (!(object->flags & BDOF_NO_POSITION) &&
				(object->show_rect.MaxX > info->size.MaxX || object->show_rect.MinX < info->size.MinX ||
				 object->show_rect.MaxY > info->size.MaxY || object->show_rect.MinY < info->size.MinY))
			{
				// Erase icon
				backdrop_render_object(info, object, BRENDERF_CLEAR | BRENDERF_CLIP);

				// Set "no position"
				object->flags |= BDOF_NO_POSITION;
			}
		}
	}

	// Go through backdrop list
	for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ;
		 object = (BackdropObject *)object->node.ln_Succ)
	{
		// Invalid position?
		if (object->flags & BDOF_NO_POSITION)
		{
			// Get icon size
			backdrop_icon_size(info, object, &width, &height);

			// See if this icon is smaller in size to the last
			if (last_w && (width < last_w - 2 || height < last_h - 2))
			{
				// Better start again from the beginning, in case it fits
				start_x = 0;
				start_y = 0;
			}

			// Position it
			backdrop_position_object(info, object, &start_x, &start_y, width, height, list);

			// Save object size
			last_w = width;
			last_h = height;

			// Increment count
			++count;
		}
	}

	// Unlock backdrop list
	unlock_listlock(&info->objects);

	// Free list
	backdrop_free_poslist(list);

	return count;
}

// Place an icon sensibly
void backdrop_place_object(BackdropInfo *info, BackdropObject *icon)
{
	struct MinList *list = 0;

	// Desktop window?
	if (info->flags & BDIF_MAIN_DESKTOP)
	{
		ULONG type;

		// Get icon type
		if ((type = get_list_type(icon)))
		{
			// Get position list
			list = backdrop_build_poslist(info, type);
		}
	}

	// Lock backdrop list
	lock_listlock(&info->objects, 0);

	// Position it
	backdrop_position_object(info, icon, 0, 0, 0, 0, list);

	// Unlock backdrop list
	unlock_listlock(&info->objects);

	// Free list
	backdrop_free_poslist(list);
}

// Place an icon sensibly (mark III)
void backdrop_position_object(BackdropInfo *info,
							  BackdropObject *icon,
							  long *start_x,
							  long *start_y,
							  short width,
							  short height,
							  struct MinList *pos_list)
{
	BackdropObject *first = 0;
	struct Rectangle rect;
	long x, y;
	short border_x = 0, border_y_top = 0, off_x, off_y;
	iconpos_data *pos_area = 0, fake_area;
	ULONG type;

	// No icon?
	if (!icon)
		return;

	// Get icon type; if not a valid type, we don't use the list
	if (!(type = get_list_type(icon)))
	{
		pos_list = 0;
	}

	// Does icon have a position?
	if (!(icon->flags & BDOF_NO_POSITION))
	{
		// Temporarily set no position
		icon->flags |= BDOF_NO_POSITION;

		// If positioning on the desktop, or we don't have an Opus snapshot, the space has to be clear
		if ((!(info->flags & BDIF_MAIN_DESKTOP) && GetIconFlags(icon->icon) & ICONF_POSITION_OK) ||
			!(backdrop_icon_in_rect_full(info, &icon->full_size)))
		{
			// Ok to go here
			icon->flags &= ~BDOF_NO_POSITION;
			return;
		}
	}

	// Invalid virtual size?
	if (info->virt_width < 2 || info->virt_height < 2)
	{
		// Calculate virtual size
		backdrop_calc_virtual_size(info, &info->area);
		info->virt_width = RECTWIDTH(&info->area);
		info->virt_height = RECTHEIGHT(&info->area);

		// No window?
		if (!info->window && (info->virt_width < 2 || info->virt_height < 2))
		{
			// Is it a lister?
			if (info->lister)
			{
				// Get virtual size from lister structure
				info->virt_width = info->lister->dimensions.wd_Normal.Width;
				info->virt_height = info->lister->dimensions.wd_Normal.Height;

				// Fill in area rectangle
				info->area.MinX = 0;
				info->area.MinY = 0;
				info->area.MaxX = info->virt_width - 1;
				info->area.MaxY = info->virt_height - 1;
			}
		}
	}

	// Get icon offset
	off_x = info->size.MinX - info->offset_x;
	off_y = info->size.MinY - info->offset_y;

	// Get icon size if needed
	if (!width)
		backdrop_icon_size(info, icon, &width, &height);

	// Initialise fake area
	fake_area.ip_Node.ln_Succ = 0;
	fake_area.ip_Flags = ICONPOSF_DISKS | ICONPOSF_APPICONS | ICONPOSF_LISTERS | ICONPOSF_GROUPS | ICONPOSF_LEFTOUTS;
	fake_area.ip_Area.Left = 0;
	fake_area.ip_Area.Top = 0;
	fake_area.ip_Area.Width = info->virt_width - (CLEANUP_START_X << 1);
	fake_area.ip_Area.Height = info->virt_height;

	// Get pointer to first area for this icon type
	if (pos_list)
	{
		// Go through list
		for (pos_area = (iconpos_data *)pos_list->mlh_Head; pos_area->ip_Node.ln_Succ;
			 pos_area = (iconpos_data *)pos_area->ip_Node.ln_Succ)
		{
			// Correct type?
			if (pos_area->ip_Flags & type)
				break;
		}
	}

	// Invalid first area?
	if (!pos_area || !pos_area->ip_Node.ln_Succ)
	{
		pos_area = &fake_area;
	}

	// Start at the top
	x = (start_x) ? *start_x : 0;
	y = (start_y) ? *start_y : 0;

	// Check coordinates against grid
	backdrop_check_grid(&x, &y);

	// Adjust start position for area
	x += pos_area->ip_Area.Left;
	y += pos_area->ip_Area.Top;

	// Loop through areas
	while (pos_area)
	{
		BOOL found = 0;
		struct Node *next_ptr;

		// Loop until success
		while (1)
		{
			BackdropObject *incumbent;

			// Get position icon would be in if we put it here
			rect.MinX = x + CLEANUP_START_X + off_x;
			rect.MinY = y + CLEANUP_START_Y + off_y;
			rect.MaxX = rect.MinX + width - 1;
			rect.MaxY = rect.MinY + height - 1;

			// Would this take icon outside the display on a backdrop window?
			if (info->window && info->window->Flags & WFLG_BACKDROP)
			{
				// See if icon is out of display
				if (rect.MaxX > info->area.MaxX || rect.MaxY > info->area.MaxY)
				{
					// Move on to next area
					break;
				}
			}

			// Check position for room
			if ((incumbent = backdrop_icon_in_rect(info, &rect)))
			{
				// If this is the first for the row, save it
				if (x == pos_area->ip_Area.Left)
					first = incumbent;

				// Shunt over to the right ready to try again
				x = incumbent->show_rect.MaxX + environment->env->env_icon_space_x - CLEANUP_START_X + 1;

				// Check coordinates against grid
				x -= pos_area->ip_Area.Left;
				backdrop_check_grid(&x, 0);
				x += pos_area->ip_Area.Left;

				// Does this make us too big for the window now?
				if (x + width > pos_area->ip_Area.Left + pos_area->ip_Area.Width)
				{
					// We need to move down
					x = pos_area->ip_Area.Left;
					y += ((first) ? RECTHEIGHT(&first->show_rect) : height) + environment->env->env_icon_space_y - 1;

					// Check coordinates against grid
					y -= pos_area->ip_Area.Top;
					backdrop_check_grid(0, &y);
					y += pos_area->ip_Area.Top;

					// Reset first pointer
					first = 0;

					// If we have an area, check if we've exceeded it
					if (pos_area != &fake_area)
					{
						if (y > pos_area->ip_Area.Top + pos_area->ip_Area.Height)
						{
							// Break out, move to next area
							break;
						}
					}

					// Otherwise, too high for backdrop window?
					else if (info->window && info->window->Flags & WFLG_BACKDROP &&
							 y > pos_area->ip_Area.Top + pos_area->ip_Area.Height)
					{
						// Fail
						x = 0;
						y = 0;
						found = 1;
						break;
					}
				}

				// Continue looking in area
				continue;
			}

			// Got position
			found = 1;
			break;
		}

		// Got position? break
		if (found)
			break;

		// On last area?
		if (pos_area == &fake_area)
			break;

		// Get next area
		next_ptr = pos_area->ip_Node.ln_Succ;

		// Free that area
		Remove((struct Node *)pos_area);
		FreeVec(pos_area);

		// Find next area of correct type
		while (next_ptr->ln_Succ)
		{
			// Get area pointer
			pos_area = (iconpos_data *)next_ptr;

			// Match type
			if (pos_area->ip_Flags & type)
				break;

			// Get next
			next_ptr = next_ptr->ln_Succ;
		}

		// If no more areas, move to full-screen area
		if (!next_ptr->ln_Succ)
			pos_area = &fake_area;

		// Get starting position
		x = pos_area->ip_Area.Left;
		y = pos_area->ip_Area.Top;
	}

	// Return position
	if (start_x)
		*start_x = x - pos_area->ip_Area.Left;
	if (start_y)
		*start_y = y - pos_area->ip_Area.Top;

	// Non-transparent icon?
	if (backdrop_icon_border(icon))
	{
		border_x = ICON_BORDER_X;
		border_y_top = ICON_BORDER_Y_TOP;
	}

	// Save position
	icon->pos.Left = x + border_x + CLEANUP_START_X;
	icon->pos.Top = y + border_y_top + CLEANUP_START_Y;
	icon->flags &= ~BDOF_NO_POSITION;

	// Get rectangle for this position
	rect.MinX = icon->pos.Left + off_x - border_x;
	rect.MinY = icon->pos.Top + off_y - border_y_top;
	rect.MaxX = rect.MinX + width - 1;
	rect.MaxY = rect.MinY + height - 1;

	// Get actual image width
	width = icon->pos.Width + (border_x << 1);

	// Move icon across to center within display
	icon->pos.Left += (RECTWIDTH(&rect) - width) >> 1;

	// Use as icon size
	icon->show_rect = rect;

	// Full size is the same as show_rect but without the offset
	icon->full_size.MinX = rect.MinX - off_x;
	icon->full_size.MinY = rect.MinY - off_y;
	icon->full_size.MaxX = rect.MaxX - off_x;
	icon->full_size.MaxY = rect.MaxY - off_y;

	// See if icon is outside current virtual size
	if (icon->full_size.MinX < info->area.MinX)
		info->area.MinX = icon->full_size.MinX;
	if (icon->full_size.MinY < info->area.MinY)
		info->area.MinY = icon->full_size.MinY;
	if (icon->full_size.MaxX > info->area.MaxX)
		info->area.MaxX = icon->full_size.MaxX;
	if (icon->full_size.MaxY > info->area.MaxY)
		info->area.MaxY = icon->full_size.MaxY;
}

// Build a position list
struct MinList *backdrop_build_poslist(BackdropInfo *bdinfo, ULONG flags)
{
	struct MinList *list;
	iconpos_data *info;
	Cfg_Desktop *desk;

	// Allocate list
	if (!(list = AllocVec(sizeof(struct MinList), MEMF_CLEAR)))
		return 0;
	NewList((struct List *)list);

	// Lock desktop list
	GetSemaphore(&environment->desktop_lock, SEMF_SHARED, 0);

	// Go through desktop list
	for (desk = (Cfg_Desktop *)environment->desktop.mlh_Head; desk->node.mln_Succ;
		 desk = (Cfg_Desktop *)desk->node.mln_Succ)
	{
		// Icon position?
		if (desk->data.dt_Type == DESKTOP_ICONPOS)
		{
			// The right type of icons?
			if (desk->data.dt_Flags & flags)
			{
				// Allocate structure
				if ((info = AllocVec(sizeof(iconpos_data), MEMF_CLEAR)))
				{
					short pri;

					// Initialise
					info->ip_Flags = desk->data.dt_Flags;
					info->ip_Area = *((struct IBox *)&desk->data.dt_Data);

					// Is the main window open, in backdrop mode?
					if (bdinfo->window && bdinfo->window->Flags & WFLG_BACKDROP)
					{
						// See if area is outside of bounds
						if (info->ip_Area.Left > bdinfo->size.MaxX || info->ip_Area.Top > bdinfo->size.MaxY)
						{
							// Remove completely
							FreeVec(info);
							info = 0;
						}

						// Or just strays outside
						else
						{
							// Out to the left?
							if (info->ip_Area.Left + info->ip_Area.Width - 1 > bdinfo->size.MaxX)
								info->ip_Area.Width = bdinfo->size.MaxX - info->ip_Area.Left + 1;

							// Out to the bottom?
							if (info->ip_Area.Top + info->ip_Area.Height - 1 > bdinfo->size.MaxY)
								info->ip_Area.Height = bdinfo->size.MaxY - info->ip_Area.Top + 1;
						}
					}

					// Still got area?
					if (info)
					{
						// Set priority
						pri = ICONPOSF_PRIORITY(info->ip_Flags) - 127;
						info->ip_Node.ln_Pri = pri;

						// Add to list
						Enqueue((struct List *)list, &info->ip_Node);
					}
				}
			}
		}
	}

	// Unlock list
	FreeSemaphore(&environment->desktop_lock);

	// Is the list empty?
	if (IsListEmpty((struct List *)list))
	{
		// Free it now
		FreeVec(list);
		return 0;
	}

	// Return the list pointer
	return list;
}

// Free a position list
void backdrop_free_poslist(struct MinList *list)
{
	struct Node *node, *next;

	// Valid list?
	if (!list)
		return;

	// Go through list, free nodes
	for (node = (struct Node *)list->mlh_Head; node->ln_Succ; node = next)
	{
		// Cache next
		next = node->ln_Succ;

		// Free node
		FreeVec(node);
	}

	// Free list
	FreeVec(list);
}

// Get list type for an icon
ULONG get_list_type(BackdropObject *icon)
{
	ULONG type = 0;

	if (icon->type == BDO_DISK || icon->type == BDO_BAD_DISK)
		type = ICONPOSF_DISKS;
	else if (icon->type == BDO_APP_ICON)
	{
		if (icon->flags & BDOF_SPECIAL)
			type = ICONPOSF_LISTERS;
		else
			type = ICONPOSF_APPICONS;
	}
	else if (icon->type == BDO_GROUP)
		type = ICONPOSF_GROUPS;
	else if (icon->type == BDO_LEFT_OUT)
		type = ICONPOSF_LEFTOUTS;

	return type;
}
