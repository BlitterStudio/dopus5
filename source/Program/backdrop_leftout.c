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

#define is_digit(c) ((c) >= '0' && (c) <= '9')

// Buffers for reading Workbench .backdrop files, too big for the stack
struct wbleftout_buffers
{
	char path[2048];
	char line[2048];
};

// Get a new left-out object
BackdropObject *backdrop_leftout_new(BackdropInfo *info, char *name, char *parent_dir, ULONG flags)
{
	BackdropObject *newob;
	short size;
	char path[262];

	// Not given a path?
	if (!parent_dir)
	{
		BPTR lock, parent;

		// Lock object
		if (!(lock = Lock(name, ACCESS_READ)))
		{
			// Couldn't lock object.. see if it's "Disk"
			if (stricmp(FilePart(name), "disk") == 0)
				return 0;

			// Not a disk.info, try for .info file
			StrCombine(path, name, ".info", sizeof(path));
			if (!(lock = Lock(path, ACCESS_READ)))
				return 0;
		}

		// Get parent dir
		if ((parent = ParentDir(lock)))
		{
			// Get path
			DevNameFromLockDopus(parent, path, 260);
			parent_dir = path;
		}

		// Unlock locks
		UnLock(lock);
		UnLock(parent);
	}

	// Get size
	size = sizeof(BackdropObject) + (parent_dir ? strlen(parent_dir) + 1 : 0) + GUI->def_filename_length + 1;
	if (flags & BLNF_CUSTOM_LABEL)
		size += GUI->def_filename_length + 1;

	// Allocate a new object
	if ((newob = AllocMemH(info->memory, size)))
	{
		// Fill out object
		newob->type = BDO_LEFT_OUT;
		newob->flags = BDOF_NO_POSITION;

		// Get object name
		newob->name = (char *)(newob + 1);
		newob->node.ln_Name = newob->name;
		strcpy(newob->name, FilePart(name));

		// Get path pointer
		if (parent_dir)
			newob->path = newob->name + GUI->def_filename_length + 1;

		// Custom label?
		if (flags & BLNF_CUSTOM_LABEL)
		{
			// Get label pointer
			newob->device_name = newob->name + GUI->def_filename_length + 1;
			newob->flags |= BDOF_CUSTOM_LABEL;

			// Bump path pointer
			if (parent_dir)
				newob->path += GUI->def_filename_length + 1;
		}

		// Copy path
		if (parent_dir)
			strcpy(newob->path, parent_dir);

		// Add to backdrop list
		AddTail(&info->objects.list, &newob->node);
	}

	return newob;
}

// Leave things out
void backdrop_leave_icons_out(BackdropInfo *info, BackdropObject *only_one, BOOL shortcut)
{
	BackdropObject *object = 0;
	BOOL save = 0;

	// Lock icon list
	lock_listlock(&info->objects, 0);

	// Go through selected icons
	while ((object = backdrop_next_object(info, object, only_one)))
	{
		// Create shortcut?
		if (shortcut)
		{
			BPTR dir;
			char path[256];

			// Get icon lock
			if ((dir = backdrop_icon_lock(object)))
			{
				// Get icon path name
				DevNameFromLockDopus(dir, path, 256);
				AddPart(path, object->name, 256);

				// New shortcut
				backdrop_create_shortcut(GUI->backdrop, path, -1, -1);

				// Unlock lock
				UnLock(dir);
			}
		}

		// Left-out object?
		else if (object->type == BDO_LEFT_OUT)
		{
			// Temporary leave-out?
			if (object->flags & BDOF_TEMP_LEFTOUT)
			{
				// Clear temporary flag
				object->flags &= ~BDOF_TEMP_LEFTOUT;

				// Set flag to save leftouts
				save = 1;
			}

			// Otherwise, only leave out if not already on main window
			else if (info != GUI->backdrop)
			{
				BPTR dir;
				char path[256];

				// Get icon lock
				if ((dir = backdrop_icon_lock(object)))
				{
					// Get icon path name
					DevNameFromLockDopus(dir, path, 256);
					AddPart(path, object->name, 256);

					// New left-out
					if (backdrop_leave_out(GUI->backdrop, path, BLOF_PERMANENT | BLOF_REFRESH, -1, -1))
						save = 1;

					// Unlock lock
					UnLock(dir);
				}
			}
		}
	}

	// Unlock list
	unlock_listlock(&info->objects);

	// Need to save?
	if (save)
		backdrop_save_leftouts(GUI->backdrop);
}

// Leave out something
BackdropObject *backdrop_leave_out(BackdropInfo *info, char *name, ULONG flags, short x, short y)
{
	BPTR lock, parent;
	D_S(struct FileInfoBlock, fib)
	BackdropObject *object;
	struct List *search;
	char path[262];

	// Lock object
	if (!(lock = Lock(name, ACCESS_READ)))
	{
		// Try for icon
		StrCombine(info->buffer, name, ".info", sizeof(info->buffer));
		if (!(lock = Lock(info->buffer, ACCESS_READ)))
			return 0;

		// Isolate filename
		stccpy(fib->fib_FileName, FilePart(name), sizeof(fib->fib_FileName));
	}

	// Locked object, get some info
	else
		Examine(lock, fib);

	// Get parent lock
	if ((parent = ParentDir(lock)))
	{
		// Get parent path
		DevNameFromLockDopus(parent, path, 260);
		UnLock(parent);
	}
	else
		path[0] = 0;

	// Lock backdrop list
	lock_listlock(&info->objects, 1);

	// See if object is already in list
	search = &info->objects.list;
	while ((object = (BackdropObject *)FindNameI(search, fib->fib_FileName)))
	{
		// See if parents are the same
		if (object->type == BDO_LEFT_OUT && stricmp(path, object->path) == 0)
			break;

		// Keep searching from this object
		search = (struct List *)object;
	}

	// Unlock object
	UnLock(lock);

	// Was object not already in list?
	if (!object)
	{
		// Get new left out
		if ((object = backdrop_leftout_new(info, name, path, BLNF_CUSTOM_LABEL)))
		{
			// Set name
			stccpy(object->device_name, FilePart(name), GUI->def_filename_length);

			// Set temporary flag if not permanent
			if (!(flags & BLOF_PERMANENT))
				object->flags |= BDOF_TEMP_LEFTOUT;

			// Position supplied?
			if (x > -1 && y > -1)
			{
				// Set position
				object->custom_pos = (x << 16) | y;
				object->flags |= BDOF_LEFTOUT_POS | BDOF_BORDER_ADJUST;
				object->flags &= ~BDOF_NO_POSITION;
			}

			// Get icon
			backdrop_get_icon(info, object, GETICON_CD);

			// Refresh?
			if (flags & BLOF_REFRESH)
			{
				// Arrange new icon
				backdrop_place_object(info, object);

				// Show icons
				backdrop_show_objects(info, BDSF_RECALC);
			}
		}
	}
	else
		object = 0;

	// Unlock backdrop list
	unlock_listlock(&info->objects);

	return object;
}

// Save .backdrop file
void backdrop_save_leftouts(BackdropInfo *info)
{
	BackdropObject *object;
	BOOL save = 0;

	// Lock backdrop list
	lock_listlock(&info->objects, 0);

	// Lock position list
	lock_listlock(&GUI->positions, TRUE);

	// Go through objects
	for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ;
		 object = (BackdropObject *)object->node.ln_Succ)
	{
		// Is this a permanent left-out?
		if (object->type == BDO_LEFT_OUT && !(object->flags & (BDOF_TEMP_LEFTOUT | BDOF_DESKTOP_FOLDER)))
		{
			struct Node *entry;
			struct List *search;

			// Get full path of object
			stccpy(info->buffer, object->path, sizeof(info->buffer));
			AddPart(info->buffer, object->name, 256);

			// See if it's already in the list
			search = (struct List *)&GUI->positions;
			while ((entry = FindNameI(search, info->buffer)))
			{
				// Left-out?
				if (entry->ln_Type == PTYPE_LEFTOUT)
					break;
				search = (struct List *)entry;
			}

			// Not found?
			if (!entry)
			{
				leftout_record *left;

				// Create new entry
				if ((left = AllocMemH(GUI->position_memory, sizeof(leftout_record) + strlen(info->buffer))))
				{
					// Valid icon position?
					if (!(object->flags & BDOF_NO_POSITION))
					{
						short x, y;
						ULONG iflags;

						// Get position
						x = object->pos.Left;
						y = object->pos.Top;

						// Adjust for borders?
						if (!((iflags = GetIconFlags(object->icon)) & ICONF_BORDER_OFF) &&
							(!(environment->env->desktop_flags & DESKTOPF_NO_BORDERS) || (iflags & ICONF_BORDER_ON)))
						{
							// Shift back by border size
							x -= ICON_BORDER_X;
							y -= ICON_BORDER_Y_TOP;
						}

						// Store position
						left->icon_x = x;
						left->icon_y = y;
					}

					// No position
					else
						left->flags = LEFTOUTF_NO_POSITION;

					// Fill out name and type
					stccpy(left->icon_label, FilePart(info->buffer), sizeof(left->icon_label));
					strcpy(left->name, info->buffer);
					left->node.ln_Name = left->name;
					left->node.ln_Type = PTYPE_LEFTOUT;

					// Set pointer in object
					object->misc_data = (IPTR)left;

					// Add to list
					AddTail((struct List *)&GUI->positions, (struct Node *)left);
					save = 1;
				}
			}
		}
	}

	// Save positions?
	if (save)
		SavePositions(&GUI->positions.list, GUI->position_name);

	// Unlock position list
	unlock_listlock(&GUI->positions);

	// Unlock backdrop list
	unlock_listlock(&info->objects);
}

// Put objects away
void backdrop_putaway(BackdropInfo *info, BackdropObject *only_one)
{
	BackdropObject *object;
	BOOL save = 0;

	// Lock backdrop list
	lock_listlock(&info->objects, 1);

	// Lock position list
	lock_listlock(&GUI->positions, TRUE);

	// Go through backdrop list
	for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ;)
	{
		BackdropObject *next = (BackdropObject *)object->node.ln_Succ;

		// Is object left out and selected?
		if (object->type == BDO_LEFT_OUT && !(object->flags & BDOF_DESKTOP_FOLDER) &&
			((only_one && object == only_one) || (!only_one && object->state)))
		{
			// Erase object
			backdrop_erase_icon(info, object, 0);

			// If left-out wasn't temporary, delete from list
			if (!(object->flags & BDOF_TEMP_LEFTOUT))
			{
				if (backdrop_remove_leftout(object))
				{
					save = 1;
				}
			}

			// Remove object
			backdrop_remove_object(info, object);
		}

		// Get next
		object = next;
	}

	// Recalc virtual
	backdrop_calc_virtual(info);

	// Save leftouts?
	if (save)
		SavePositions(&GUI->positions.list, GUI->position_name);

	// Unlock position list
	unlock_listlock(&GUI->positions);

	// Unlock backdrop list
	unlock_listlock(&info->objects);
}

// Remove a left-out from the list
BOOL backdrop_remove_leftout(BackdropObject *object)
{
	char buf[256];
	struct Node *node;
	struct List *search;

	// Get full path of object
	stccpy(buf, object->path, 256);
	AddPart(buf, object->name, 256);

	// Look for object in list
	search = (struct List *)&GUI->positions;
	while ((node = FindNameI(search, buf)))
	{
		// Left-out?
		if (node->ln_Type == PTYPE_LEFTOUT)
		{
			// Remove and free
			Remove(node);
			FreeMemH(node);

			// Return success
			return 1;
		}

		// Keep looking
		search = (struct List *)node;
	}

	// Failed
	return 0;
}

// Add left-out icons (icon list must be locked)
void backdrop_add_leftouts(BackdropInfo *info)
{
	leftout_record *left;
	BackdropObject *object;
	struct wbleftout_buffers *wbl;

	// Lock position list
	lock_listlock(&GUI->positions, FALSE);

	// Go through leftouts
	for (left = (leftout_record *)GUI->positions.list.lh_Head; left->node.ln_Succ;
		 left = (leftout_record *)left->node.ln_Succ)
	{
		// Left-out?
		if (left->node.ln_Type == PTYPE_LEFTOUT)
		{
			struct List *search;
			char *name;

			// Get filename
			name = FilePart(left->node.ln_Name);

			// Go through objects, see if this is already there
			search = &info->objects.list;
			while ((object = (BackdropObject *)FindNameI(search, name)))
			{
				// Is this a left-out?
				if (object->type == BDO_LEFT_OUT)
				{
					// Match entry pointer
					if (object->misc_data == (IPTR)left)
						break;
				}

				// Continue search
				search = (struct List *)object;
			}

			// Not there?
			if (!object)
			{
				// Get new left-out object
				if ((object = backdrop_leftout_new(info, left->node.ln_Name, 0, BLNF_CUSTOM_LABEL)))
				{
					// Valid position?
					if (!(left->flags & LEFTOUTF_NO_POSITION))
					{
						// Store position
						object->flags |= BDOF_LEFTOUT_POS;
						object->custom_pos = (left->icon_x << 16) | left->icon_y;
					}

					// Copy label, store entry pointer
					stccpy(object->device_name, left->icon_label, GUI->def_filename_length + 1);
					object->misc_data = (IPTR)left;

					// Fix size and position
					backdrop_get_icon(info, object, GETICON_CD);
				}
			}
		}
	}

	// added option to show workbench leftouts, set in config module
	if ((environment->env->display_options & DISPOPTF_SHOW_WBLEFTOUTS) &&
		(wbl = AllocVec(sizeof(*wbl), MEMF_ANY)))
	{
		BackdropObject *object, *newobject;
		char *buf = wbl->path, *leftout = wbl->line;
		BPTR backdrop;
		char *pos;

		APTR oldwin = ((struct Process *)FindTask(0))->pr_WindowPtr;
		((struct Process *)FindTask(0))->pr_WindowPtr = (APTR)-1;

		for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ;
			 object = (BackdropObject *)object->node.ln_Succ)
		{
			// Is this a disk?
			if (object->type == BDO_DISK)
			{
				snprintf(buf, sizeof(wbl->path), "%s:.backdrop", object->name);

				if ((backdrop = Open(buf, MODE_OLDFILE)))
				{
					while (FGets(backdrop, leftout, sizeof(wbl->line) - 1))
					{
						// clean any newlines from the string
						if ((pos = strchr(leftout, '\r')) || (pos = strchr(leftout, '\n')))
							*pos = 0;

						if (*leftout)
						{
							snprintf(buf, sizeof(wbl->path), "%s%s", object->name, leftout);
#if 1
							backdrop_leave_out(info, buf, BLOF_PERMANENT | BLOF_REFRESH, -1, -1);
#else
							if ((newobject = backdrop_leftout_new(info, buf, 0, BLNF_CUSTOM_LABEL)))
							{
								stccpy(newobject->device_name, FilePart(buf), GUI->def_filename_length + 1);

								backdrop_get_icon(info, newobject, GETICON_CD);
							}
#endif
						}
					}

					Close(backdrop);
				}
			}
		}

		((struct Process *)FindTask(0))->pr_WindowPtr = oldwin;
		FreeVec(wbl);
	}

	// Lock position list
	unlock_listlock(&GUI->positions);
}

// Create a shortcut
BackdropObject *backdrop_create_shortcut(BackdropInfo *info, char *name, short x, short y)
{
	BPTR lock;
	D_S(struct FileInfoBlock, fib)

	// Lock object
	if (!(lock = Lock(name, ACCESS_READ)))
	{
		// Try for icon
		StrCombine(info->buffer, name, ".info", sizeof(info->buffer));
		if (!(lock = Lock(info->buffer, ACCESS_READ)))
			return 0;

		// Isolate filename
		stccpy(fib->fib_FileName, FilePart(name), sizeof(fib->fib_FileName));
	}

	// Locked object, get some info
	else
		Examine(lock, fib);

	// Unlock object
	UnLock(lock);

	// Get destination (shortcut) pathname
	StrCombine(info->buffer, "DOpus5:Desktop/", fib->fib_FileName, sizeof(info->buffer));

	// Make name unique
	backdrop_unique_name(info->buffer);

	// Make the link
	if (MakeLink(info->buffer, (SIPTR)name, TRUE))
	{
		struct DiskObject *icon;

		// Get icon for new link
		if ((icon = GetProperIcon(name, 0, GCDOF_NOCACHE)))
		{
			// Set "position ok" flag
			SetIconFlags(icon, GetIconFlags(icon) | ICONF_POSITION_OK);

			// Update icon position
			SetIconPosition(icon, x, y);

			// Write icon for new link
			PutDiskObject(info->buffer, icon);

			// Free icon
			FreeCachedDiskObject(icon);
		}
	}

	return 0;
}

// Make a unique filename
void backdrop_unique_name(char *name)
{
	BPTR lock;
	short count = 1,
	try
		;
	char *ptr = 0;

	// Does name already have a count?
	if (name[strlen(name) - 1] == ')')
	{
		// Find start of number
		ptr = name + strlen(name) - 1;
		while (ptr > name)
			if (!(is_digit(*ptr)))
				break;

		// Found it?
		if (ptr > name && is_digit(*(ptr + 1)))
		{
			// Bump pointer
			++ptr;

			// Get initial count
			count = atoi(ptr);
		}
	}

	// Loop for a few tries
	for (try = 0; try < 100; try ++)
	{
		// See if file exists
		if (!(lock = Lock(name, ACCESS_READ)))
			return;

		// Unlock the lock
		UnLock(lock);

		// No number yet?
		if (!ptr)
		{
			// Get end of name, check not too long
			if (strlen(FilePart(name)) > 26)
			{
				ptr = name + strlen(name) - 4;
			}
			else
			{
				strcat(name, " ");
				ptr = name + strlen(name) - 1;
			}

			// Add opening bracket
			*ptr++ = '(';
			*ptr = 0;
		}

		// Increment count
		++count;

		// Print into name
		lsprintf(ptr, "%ld)", count);
	}
}
