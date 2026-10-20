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

// Load position list
void GetPositions(struct ListLock *list, APTR memory, char *name)
{
	ULONG id;
	APTR iff;

	// Open file
	if (!(iff = IFFOpen(name, IFF_READ, ID_OPUS)))
		return;

	// Lock list
	lock_listlock(list, TRUE);

	// Initialise list
	NewList(&list->list);

	// Read file
	while ((id = IFFNextChunk(iff, 0)))
	{
		struct Node *entry;

		// Valid chunk?
		if (id != ID_POSI && id != ID_LOUT && id != ID_ICON && id != ID_STRT)
			continue;

		// Allocate record
		if (!(entry = AllocMemH(memory, IFFChunkSize(iff) + sizeof(struct Node))))
			continue;

		// Read chunk data
		IFFReadChunkBytes(iff, (APTR)(entry + 1), -1);

		// Set type and name pointer
		if (id == ID_POSI)
		{
			position_rec *rec = (position_rec *)entry;

#ifdef __AROS__
			rec->icon_x = AROS_BE2WORD(rec->icon_x);
			rec->icon_y = AROS_BE2WORD(rec->icon_y);

			rec->text_dims.Left = AROS_BE2WORD(rec->text_dims.Left);
			rec->text_dims.Top = AROS_BE2WORD(rec->text_dims.Top);
			rec->text_dims.Width = AROS_BE2WORD(rec->text_dims.Width);
			rec->text_dims.Height = AROS_BE2WORD(rec->text_dims.Height);
			rec->icon_dims.Left = AROS_BE2WORD(rec->icon_dims.Left);
			rec->icon_dims.Top = AROS_BE2WORD(rec->icon_dims.Top);
			rec->icon_dims.Width = AROS_BE2WORD(rec->icon_dims.Width);
			rec->icon_dims.Height = AROS_BE2WORD(rec->icon_dims.Height);

			rec->code = AROS_BE2WORD(rec->code);
			rec->qual = AROS_BE2WORD(rec->qual);
			rec->qual_mask = AROS_BE2WORD(rec->qual_mask);
			rec->qual_same = AROS_BE2WORD(rec->qual_same);
			rec->flags = AROS_BE2LONG(rec->flags);
#endif

			// Set type and name
			entry->ln_Type = PTYPE_POSITION;
			entry->ln_Name = rec->name;

			// Check flags
			if (!(rec->flags & POSITIONF_NEW_FLAG))
				rec->flags |= POSITIONF_POSITION | POSITIONF_NEW_FLAG;
		}
		else if (id == ID_LOUT)
		{
#ifdef __AROS__
			leftout_record *rec = (leftout_record *)entry;

			rec->icon_x = AROS_BE2WORD(rec->icon_x);
			rec->icon_y = AROS_BE2WORD(rec->icon_y);
			rec->flags = AROS_BE2LONG(rec->flags);
#endif

			entry->ln_Type = PTYPE_LEFTOUT;
			entry->ln_Name = ((leftout_record *)entry)->name;
		}
		else if (id == ID_ICON)
		{
#ifdef __AROS__
			leftout_record *rec = (leftout_record *)entry;

			rec->icon_x = AROS_BE2WORD(rec->icon_x);
			rec->icon_y = AROS_BE2WORD(rec->icon_y);
			rec->flags = AROS_BE2LONG(rec->flags);
#endif

			entry->ln_Type = PTYPE_APPICON;
			entry->ln_Name = ((leftout_record *)entry)->name;
		}

		// Add to list
		AddTail((struct List *)list, entry);
	}

	// Close file
	IFFClose(iff);

	// Unlock list
	unlock_listlock(list);
}

// Save all positions
void SavePositions(struct List *list, char *name)
{
	APTR iff;
	struct Node *node;

	// Open file
	if (!(iff = IFFOpen(name, IFF_WRITE | IFF_SAFE, ID_OPUS)))
		return;


	// Go through entries
	for (node = list->lh_Head; node->ln_Succ; node = node->ln_Succ)
	{
		// Valid path, or AppIcon?
		if ((node->ln_Name && *node->ln_Name) || node->ln_Type == PTYPE_APPICON)
		{
			ULONG id = 0;
			short size = 0;
#ifdef __AROS__
			BOOL okay = FALSE;
			struct Node *node_be;
#endif

			// Position?
			if (node->ln_Type == PTYPE_POSITION)
			{
				// Skip temporaries
				if (((position_rec *)node)->flags & POSITIONF_TEMP)
					continue;

				// Get id and chunk size
				id = ID_POSI;
				size = sizeof(position_rec) - sizeof(struct Node);
			}

			// Left-out?
			else if (node->ln_Type == PTYPE_LEFTOUT)
			{
				// Get id and chunk size
				id = ID_LOUT;
				size = sizeof(leftout_record) - sizeof(struct Node);
			}

			// AppIcon?
			else if (node->ln_Type == PTYPE_APPICON)
			{
				// Get id and chunk size
				id = ID_ICON;
				size = sizeof(leftout_record) - sizeof(struct Node);
			}

#ifdef __AROS__
			if ((node_be = AllocVec(sizeof(struct Node) + size + strlen(node->ln_Name) + 1, MEMF_CLEAR)))
			{
				CopyMem(node, node_be, sizeof(struct Node) + size + strlen(node->ln_Name) + 1);

				if (node->ln_Type == PTYPE_POSITION)
				{
					position_rec *rec_be = (position_rec *)node_be;

					rec_be->icon_x = AROS_WORD2BE(rec_be->icon_x);
					rec_be->icon_y = AROS_WORD2BE(rec_be->icon_y);

					rec_be->text_dims.Left = AROS_WORD2BE(rec_be->text_dims.Left);
					rec_be->text_dims.Top = AROS_WORD2BE(rec_be->text_dims.Top);
					rec_be->text_dims.Width = AROS_WORD2BE(rec_be->text_dims.Width);
					rec_be->text_dims.Height = AROS_WORD2BE(rec_be->text_dims.Height);
					rec_be->icon_dims.Left = AROS_WORD2BE(rec_be->icon_dims.Left);
					rec_be->icon_dims.Top = AROS_WORD2BE(rec_be->icon_dims.Top);
					rec_be->icon_dims.Width = AROS_WORD2BE(rec_be->icon_dims.Width);
					rec_be->icon_dims.Height = AROS_WORD2BE(rec_be->icon_dims.Height);

					rec_be->code = AROS_WORD2BE(rec_be->code);
					rec_be->qual = AROS_WORD2BE(rec_be->qual);
					rec_be->qual_mask = AROS_WORD2BE(rec_be->qual_mask);
					rec_be->qual_same = AROS_WORD2BE(rec_be->qual_same);
					rec_be->flags = AROS_LONG2BE(rec_be->flags);
				}
				else
				{
					leftout_record *rec_be = (leftout_record *)node_be;

					rec_be->icon_x = AROS_WORD2BE(rec_be->icon_x);
					rec_be->icon_y = AROS_WORD2BE(rec_be->icon_y);
					rec_be->flags = AROS_LONG2BE(rec_be->flags);
				}

				if ((IFFWriteChunk(iff, (APTR)(node_be + 1), id, size + strlen(node_be->ln_Name))))
					okay = TRUE;

				FreeVec(node_be);
			}

			if (!okay)
#else
			// Write chunk
			if (!(IFFWriteChunk(iff, (APTR)(node + 1), id, size + strlen(node->ln_Name))))
#endif
			{
				// Mark failure
				IFFFailure(iff);
				break;
			}
		}
	}

	// Close file
	IFFClose(iff);
}

// Get the position for a lister
position_rec *GetListerPosition(char *path,
								char *device,
								struct DiskObject *icon,
								struct IBox *position,
								short *mode,
								ListFormat *format,
								struct Window *parent,
								Lister *lister,
								unsigned long flags)
{
	position_rec *pos;
	struct List *search;
	struct Node *entry;

	// Lock position list
	lock_listlock(&GUI->positions, FALSE);

	// Default to text mode
	if (mode && !(flags & GLPF_USE_MODE))
		*mode = 0;

	// Use Workbench icon positions?
	if (environment->env->display_options & DISPOPTF_ICON_POS)
	{
		// Is position stored in the icon, and we're using icon mode?
		if (icon && icon->do_DrawerData)
		{
			// Get position from icon
			*position = *((struct IBox *)&icon->do_DrawerData->dd_NewWindow.LeftEdge);

			// Want mode?
			if (mode)
			{
				// Get modes from icon
				if (icon->do_DrawerData->dd_ViewModes == WBVIEWMODE_ICON ||
					icon->do_DrawerData->dd_ViewModes == WBVIEWMODE_DEFAULT)
				{
					// Icon mode
					*mode |= LISTERMODE_ICON;

					// Action?
					if (icon->do_DrawerData->dd_Flags & WBSHOWFLAG_ACTION)
						*mode |= LISTERMODE_ICON_ACTION;

					// Show all?
					if (icon->do_DrawerData->dd_Flags & WBSHOWFLAG_SHOWALL)
						*mode |= LISTERMODE_SHOW_ALL;
				}
			}

			// Got position
			position = 0;
		}
	}

	// Default format
	if (format)
		*format = environment->env->list_format;

	// Look for position in list
	search = (struct List *)&GUI->positions;
	while ((entry = FindNameI(search, path)))
	{
		// Position info?
		if (entry->ln_Type == PTYPE_POSITION)
			break;
		search = (struct List *)entry;
	}

	// Not found, and have device name?
	if (!entry && device && *device)
	{
		// Look for device
		search = (struct List *)&GUI->positions;
		while ((entry = FindNameI(search, path)))
		{
			// Position info?
			if (entry->ln_Type == PTYPE_POSITION)
				break;
			search = (struct List *)entry;
		}
	}

	// Found in list?
	if ((pos = (position_rec *)entry))
	{
		// Get format
		if (format && (pos->flags & POSITIONF_FORMAT))
		{
			// Copy format
			CopyMem((char *)&pos->format, (char *)format, sizeof(ListFormatStorage));

			// Parse wildcard stuff
			ParsePatternNoCase(format->show_pattern, format->show_pattern_p, 80);
			ParsePatternNoCase(format->hide_pattern, format->hide_pattern_p, 80);
		}

		// Want position?
		if (position)
		{
			// See if it has a valid position
			if (pos->flags & POSITIONF_POSITION)
			{
				// Mode supplied?
				if (mode && flags & GLPF_USE_MODE)
				{
					// Icon mode?
					if ((*mode) & LISTERMODE_ICON)
						*position = pos->icon_dims;

					// Text mode
					else
						*position = pos->text_dims;
				}

				// Get mode from file; icon mode?
				else if (pos->flags & POSITIONF_ICON)
				{
					// Want mode?
					if (mode)
					{
						*mode |= LISTERMODE_ICON;
						if (pos->flags & POSITIONF_ICON_ACTION)
							*mode |= LISTERMODE_ICON_ACTION;
					}

					// Store position
					*position = pos->icon_dims;
				}

				// Text mode
				else
					*position = pos->text_dims;

				// Show all?
				if (mode && !(flags & GLPF_USE_MODE) && pos->flags & POSITIONF_SHOW_ALL)
					*mode |= LISTERMODE_SHOW_ALL;
			}
			else
				pos = 0;
		}
	}

	// Unlock list
	unlock_listlock(&GUI->positions);

	// Got a position?
	if (pos || !position)
		return pos;

	// Need to get mode?
	if (!(flags & GLPF_USE_MODE))
	{
		// See if icon view mode is set in icon
		if (icon && GetIconFlags(icon) & ICONF_ICON_VIEW)
		{
			// Get mode
			*mode |= LISTERMODE_ICON;
		}

		// Parent lister?
		else if (lister)
		{
			// Get modes from lister
			if (lister->flags & LISTERF_VIEW_ICONS)
			{
				*mode |= LISTERMODE_ICON;
			}
			if (lister->flags & LISTERF_ICON_ACTION)
				*mode |= LISTERMODE_ICON_ACTION;
			if (lister->flags & LISTERF_SHOW_ALL)
				*mode |= LISTERMODE_SHOW_ALL;
		}

		// Use global settings
		else
		{
			// Get modes from global settings
			if (GUI->flags & GUIF_VIEW_ICONS)
			{
				*mode |= LISTERMODE_ICON;
			}
			if (GUI->flags & GUIF_ICON_ACTION)
				*mode |= LISTERMODE_ICON | LISTERMODE_ICON_ACTION;
			if (GUI->flags & GUIF_SHOW_ALL)
				*mode |= LISTERMODE_SHOW_ALL;
		}
	}

	// Is position stored in the icon, and we're using icon mode?
	if (icon && icon->do_DrawerData && (*mode) & LISTERMODE_ICON)
	{
		// Get position from icon
		*position = *((struct IBox *)&icon->do_DrawerData->dd_NewWindow.LeftEdge);
	}

	// Otherwise
	else
	{
		/*
				// Default position relative to parent
				if (parent)
				{
					position->Left=parent->LeftEdge+parent->BorderLeft;
					position->Top=parent->TopEdge+parent->BorderTop;
				}

				// Relative to mouse
				else
		*/
		if (GUI->screen_pointer)
		{
			position->Left = GUI->screen_pointer->MouseX - 32;
			position->Top = GUI->screen_pointer->MouseY - 32;
		}

		// Default size
		position->Width = (environment->env->lister_width < 64) ? 320 : environment->env->lister_width;
		position->Height = (environment->env->lister_height < 32) ? 200 : environment->env->lister_height;
	}

	return 0;
}

// Update position list for a lister
position_rec *PositionUpdate(Lister *lister, short flags)
{
	position_rec *pos = 0;
	struct IBox *dims;
	struct List *search;
	struct Node *entry;
	struct DiskObject *icon = 0;
	char icon_path[512];
	BOOL new = 0, disk_flag = 0;

	// Just update position, in workbench icon mode?
	if (flags == 0 && environment->env->display_options & DISPOPTF_ICON_POS && lister && lister->backdrop)
	{
		IPCData *ipc;

		// Get lister list lock
		lock_listlock(&GUI->lister_list, FALSE);

		// Go through listers
		for (ipc = (IPCData *)GUI->lister_list.list.lh_Head; ipc; ipc = (IPCData *)ipc->node.mln_Succ)
		{
			BackdropInfo *info = 0;

			// Valid lister?
			if (ipc->node.mln_Succ)
			{
				Lister *test;

				// Get lister
				if ((test = IPCDATA(ipc)))
				{
					// If lister is in icon mode, get backdrop info
					if (test->flags & LISTERF_VIEW_ICONS)
						info = test->backdrop_info;
				}
			}

			// Otherwise, use desktop
			else
			{
				info = GUI->backdrop;
			}

			// Valid backdrop?
			if (info)
			{
				BackdropObject *object;

				// Lock icon list
				lock_listlock(&info->objects, 0);

				// Go through icons
				for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ;
					 object = (BackdropObject *)object->node.ln_Succ)
				{
					// Is lister from this icon?
					if (lister->backdrop == object)
					{
						// If we have an icon, set info in that
						if (object->icon && object->icon->do_DrawerData)
						{
							struct IBox *dims;

							// Get position
							if (pos)
								dims = &pos->icon_dims;
							else if (lister_valid_window(lister))
								dims = (struct IBox *)&lister->window->LeftEdge;
							else
								dims = &lister->cur_buffer->dimensions;

							// Set position in icon
							*((struct IBox *)&object->icon->do_DrawerData->dd_NewWindow.LeftEdge) = *dims;

							// Clear flags
							object->icon->do_DrawerData->dd_Flags &= ~(WBSHOWFLAG_SHOWALL | WBSHOWFLAG_ACTION);

							// Currently in icon mode?
							if (lister->flags & LISTERF_VIEW_ICONS)
							{
								// Set flags
								object->icon->do_DrawerData->dd_ViewModes = WBVIEWMODE_ICON;

								// Icon action?
								if (lister->flags & LISTERF_ICON_ACTION)
									object->icon->do_DrawerData->dd_Flags |= WBSHOWFLAG_ACTION;

								// Show all?
								if (lister->flags & LISTERF_SHOW_ALL)
									object->icon->do_DrawerData->dd_Flags |= WBSHOWFLAG_SHOWALL;
							}

							// Or in text mode
							else
							{
								// Set viewmode
								object->icon->do_DrawerData->dd_ViewModes = WBVIEWMODE_NAME;
							}
						}
						break;
					}
				}

				// Unlock icon list
				unlock_listlock(&info->objects);

				// Matched?
				if (object->node.ln_Succ)
					break;
			}
		}

		// Unlock lister list lock
		unlock_listlock(&GUI->lister_list);
	}

	// Save?
	if (flags & POSUPF_SAVE)
	{
		// Custom handler?
		if (lister->cur_buffer->buf_CustomHandler[0])
		{
			// Send SnapShot message
			rexx_handler_msg(0, lister->cur_buffer, RXMF_WARN, HA_String, 0, "snapshot", HA_Value, 1, lister, TAG_END);
			return 0;
		}
	}

	// Invalid path?
	if (!lister->cur_buffer->buf_ExpandedPath[0] || !(lister->cur_buffer->flags & DWF_VALID))
		return 0;

	// Lock list
	lock_listlock(&GUI->positions, TRUE);

	// Use Workbench icon positions?
	if (environment->env->display_options & DISPOPTF_ICON_POS && !(flags & POSUPF_FORMAT))
	{
		char *ptr;

		// Get lister path, strip trailing '/' character
		strcpy(icon_path, lister->cur_buffer->buf_Path);
		if (*(ptr = icon_path + strlen(icon_path) - 1) == '/')
			*ptr = 0;

		// Disk?
		if (*ptr == ':')
		{
			strcat(ptr, "Disk");
			disk_flag = 1;
		}

		// Try to get icon
		if ((icon = GetDiskObject(icon_path)))
		{
			// No drawer info?
			if (!icon->do_DrawerData)
			{
				// Can't use icon
				FreeDiskObject(icon);
				icon = 0;
			}
		}
	}

	// Store in position-info if no icon
	if (!icon)
	{
		// Find position in list
		search = (struct List *)&GUI->positions;
		while ((entry = FindNameI(search, lister->cur_buffer->buf_ExpandedPath)))
		{
			// Is this a position record?
			if (entry->ln_Type == PTYPE_POSITION)
				break;
			search = (struct List *)entry;
		}

		// Not found?
		if (!(pos = (position_rec *)entry))
		{
			// Fail?
			if (flags & POSUPF_FAIL)
				return 0;

			// Allocate new entry
			if (!(pos = AllocMemH(GUI->position_memory,
								  sizeof(position_rec) + strlen(lister->cur_buffer->buf_ExpandedPath))))
			{
				// Fail
				unlock_listlock(&GUI->positions);
				return 0;
			}

			// Initialise entry
			pos->node.ln_Type = PTYPE_POSITION;
			pos->node.ln_Name = pos->name;
			strcpy(pos->name, lister->cur_buffer->buf_ExpandedPath);
			pos->code = 0xffff;
			new = 1;

			// Store default format
			CopyMem((char *)&environment->env->list_format, (char *)&pos->format, sizeof(ListFormatStorage));

			// Add to list
			AddTail((struct List *)&GUI->positions, (struct Node *)pos);

			// Initially a temporary entry
			pos->flags |= POSITIONF_TEMP | POSITIONF_NEW_FLAG;
		}

		// Store position pointer
		lister->pos_rec = pos;

		// Just storing format?
		if (flags & POSUPF_FORMAT)
		{
			// Store format
			CopyMem((char *)&lister->cur_buffer->buf_ListFormat, (char *)&pos->format, sizeof(ListFormatStorage));

			// Default format?
			if (flags & POSUPF_DEFAULT)
			{
				// Clear format flag
				pos->flags &= ~POSITIONF_FORMAT;

				// No position saved?
				if (!(pos->flags & POSITIONF_POSITION))
				{
					// Remove from list and free
					Remove((struct Node *)pos);
					FreeMemH((struct Node *)pos);
					pos = 0;
				}
			}

			// Set format flag
			else
				pos->flags |= POSITIONF_FORMAT;
		}

		// Otherwise, snapshot position
		else
		{
			// Get dimensions pointer
			if (lister_valid_window(lister))
				dims = (struct IBox *)&lister->window->LeftEdge;
			else
				dims = &lister->cur_buffer->dimensions;

			// Currently in icon mode?
			if (lister->flags & LISTERF_VIEW_ICONS)
			{
				// Set flags
				pos->flags |= POSITIONF_ICON;
				if (lister->flags & LISTERF_ICON_ACTION)
					pos->flags |= POSITIONF_ICON_ACTION;
				else
					pos->flags &= ~POSITIONF_ICON_ACTION;

				// Save icon dimensions
				pos->icon_dims = *dims;
				if (new)
					pos->text_dims = *dims;
			}

			// Or in text mode
			else
			{
				// Clear flags
				pos->flags &= ~(POSITIONF_ICON | POSITIONF_ICON_ACTION);

				// Save text dimensions
				pos->text_dims = *dims;
				if (new)
					pos->icon_dims = *dims;
			}

			// Show all?
			if (lister->flags & LISTERF_SHOW_ALL)
				pos->flags |= POSITIONF_SHOW_ALL;
			else
				pos->flags &= ~POSITIONF_SHOW_ALL;

			// Set position flag
			pos->flags |= POSITIONF_POSITION;
		}
	}

	// Save position?
	if (flags & POSUPF_SAVE)
	{
		// If we have an icon, save to that
		if (icon)
		{
			struct IBox *dims;

			// Get position
			if (pos)
				dims = &pos->icon_dims;
			else if (lister_valid_window(lister))
				dims = (struct IBox *)&lister->window->LeftEdge;
			else
				dims = &lister->cur_buffer->dimensions;

			// Set position in icon
			CopyMem((char *)dims, (char *)&icon->do_DrawerData->dd_NewWindow.LeftEdge, sizeof(struct IBox));

			// Clear flags
			icon->do_DrawerData->dd_Flags &= ~(WBSHOWFLAG_SHOWALL | WBSHOWFLAG_ACTION);

			// Currently in icon mode?
			if (lister->flags & LISTERF_VIEW_ICONS)
			{
				// Set flags
				icon->do_DrawerData->dd_ViewModes = WBVIEWMODE_ICON;

				// Icon action?
				if (lister->flags & LISTERF_ICON_ACTION)
					icon->do_DrawerData->dd_Flags |= WBSHOWFLAG_ACTION;

				// Show all?
				if (lister->flags & LISTERF_SHOW_ALL)
					icon->do_DrawerData->dd_Flags |= WBSHOWFLAG_SHOWALL;
			}

			// Or in text mode
			else
			{
				// Set viewmode
				icon->do_DrawerData->dd_ViewModes = WBVIEWMODE_NAME;
			}

			// Fix offset
			icon->do_DrawerData->dd_CurrentX = lister->backdrop_info->offset_x - lister->backdrop_info->area.MinX;
			icon->do_DrawerData->dd_CurrentY = lister->backdrop_info->offset_y - lister->backdrop_info->area.MinY;

			// Save icon
			PutDiskObject(icon_path, icon);

			// If it was a disk, signal to update datestamp so we don't reread
			if (disk_flag)
				IPC_Command(lister->ipc, LISTER_UPDATE_STAMP, 0, 0, 0, 0);
		}

		// Save position info
		else
		{
			// Clear temp flag and save list
			if (pos)
				pos->flags &= ~POSITIONF_TEMP;
			SavePositions(&GUI->positions.list, GUI->position_name);
		}
	}

	// Free icon
	if (icon)
		FreeDiskObject(icon);

	// Unlock list
	unlock_listlock(&GUI->positions);

	return pos;
}

// Remove position list for a lister
void PositionRemove(Lister *lister, BOOL save)
{
	struct List *search;
	position_rec *entry;

	// Save?
	if (save)
	{
		// Custom handler?
		if (lister->cur_buffer->buf_CustomHandler[0])
		{
			// Send SnapShot message
			rexx_handler_msg(
				0, lister->cur_buffer, RXMF_WARN, HA_String, 0, "unsnapshot", HA_Value, 1, lister, TAG_END);
			return;
		}
	}

	// Invalid path?
	if (!lister->cur_buffer->buf_ExpandedPath[0] || !(lister->cur_buffer->flags & DWF_VALID))
		return;

	// Lock list
	lock_listlock(&GUI->positions, TRUE);

	// Find position in list
	search = (struct List *)&GUI->positions;
	while ((entry = (position_rec *)FindNameI(search, lister->cur_buffer->buf_ExpandedPath)))
	{
		// Is this a position record?
		if (entry->node.ln_Type == PTYPE_POSITION)
			break;
		search = (struct List *)entry;
	}

	// Found?
	if (entry)
	{
		// Clear position flag
		entry->flags &= ~POSITIONF_POSITION;

		// Remove and free if no format
		if (!(entry->flags & POSITIONF_FORMAT))
		{
			// Remove and free
			Remove((struct Node *)entry);
			FreeMemH((struct Node *)entry);

			// Clear pointer
			lister->pos_rec = 0;
		}

		// Save position?
		if (save)
			SavePositions(&GUI->positions.list, GUI->position_name);
	}

	// Unlock list
	unlock_listlock(&GUI->positions);
}

// Copy position list
void CopyPositions(struct ListLock *source, struct List *dest, APTR memory)
{
	struct Node *node, *new;
	short size;

	// Lock list
	lock_listlock(source, FALSE);

	// Initialise destination list
	NewList(dest);

	// Go through position list
	for (node = source->list.lh_Head; node->ln_Succ; node = node->ln_Succ)
	{
		// Position?
		if (node->ln_Type == PTYPE_POSITION)
			size = sizeof(position_rec);

		// Leftout
		else
			size = sizeof(leftout_record);

		// Allocate copy
		if ((new = AllocMemH(memory, size + 256)))
		{
			// Copy data across
			CopyMem((char *)node, (char *)new, size);

			// Set name pointer
			if (node->ln_Type == PTYPE_POSITION)
				new->ln_Name = ((position_rec *)new)->name;
			else
				new->ln_Name = ((leftout_record *)new)->name;

			// Copy name
			strcpy(new->ln_Name, node->ln_Name);

			// Add to new list
			AddTail(dest, new);
		}
	}

	// Unlock list
	unlock_listlock(source);
}

// Free position list
void FreePositions(struct List *list)
{
	struct Node *pos, *next;

	// Go through list
	for (pos = list->lh_Head; pos->ln_Succ; pos = next)
	{
		// Store next
		next = pos->ln_Succ;

		// Free this node
		FreeMemH(pos);
	}

	// Initialise list
	NewList(list);
}
