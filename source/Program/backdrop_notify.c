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

// Got a notify message from PutDiskObject()
BOOL backdrop_check_notify(BackdropInfo *info, DOpusNotify *notify, Lister *lister)
{
	char *name_buf;
	BOOL disk = 0, ret = 0;
	struct List *search;
	BackdropObject *object;

	if (!(name_buf = AllocVec(256, 0)))
		return 0;

	// Disk icon?
	if (notify->dn_Name[strlen(notify->dn_Name) - 1] == ':')
	{
		char *ptr;

		// Get volume name
		if ((ptr = strchr(notify->dn_Name, ':')))
		{
			stccpy(name_buf, notify->dn_Name, ptr - notify->dn_Name + 1);
			disk = 1;
		}
	}

	// Otherwise copy name and clear it
	else
	{
		// Get name pointer
		char *name = FilePart(notify->dn_Name);

		// Copy name
		strcpy(name_buf, name);
		*name = 0;

		// Strip trailing '/'
		if (*(name - 1) == '/')
			*(name - 1) = 0;
	}

	// Is this a lister?
	if (lister)
	{
		short len;
		BOOL ok = 0;

		// Match length
		len = strlen(notify->dn_Name);

		// See if strings match
		if (strnicmp(lister->cur_buffer->buf_Path, notify->dn_Name, len) == 0)
		{
			// Check termination
			if (lister->cur_buffer->buf_Path[len] == '\0')
				ok = 1;

			// / can terminate too
			else if (lister->cur_buffer->buf_Path[len] == '/' && lister->cur_buffer->buf_Path[len + 1] == '\0')
				ok = 1;
		}

		// Didn't match?
		if (!ok)
		{
			// Free message
			ReplyFreeMsg(notify);
			FreeVec(name_buf);
			return 0;
		}
	}

	// Lock backdrop list
	lock_listlock(&info->objects, 1);

	// See if there's an icon of this name
	search = &info->objects.list;
	while ((object = (BackdropObject *)FindNameI(search, name_buf)))
	{
		// Disk?
		if (object->type == BDO_DISK && disk)
		{
			// Matched
			break;
		}

		// Valid object?
		else if (object->type != BDO_APP_ICON && object->type != BDO_BAD_DISK && object->path)
		{
			char *path = 0;
			BPTR lock;

			// If no lister, get full path of object
			if (!lister && (path = AllocVec(512, 0)))
			{
				// Lock path
				if ((lock = Lock(object->path, ACCESS_READ)))
				{
					// Get full path
					DevNameFromLockDopus(lock, path, 512);
					UnLock(lock);
				}

				// Failed
				else
					strcpy(path, object->path);
			}

			// Objects in same directory?
			if (lister || stricmp(notify->dn_Name, (path) ? path : object->path) == 0)
			{
				// Free path
				if (path)
					FreeVec(path);

				// Matched
				break;
			}

			// Free path
			if (path)
				FreeVec(path);
		}

		// If this is a lister, there could only be one
		if (lister)
		{
			object = 0;
			break;
		}

		// Keep searching from this object
		search = (struct List *)object;
	}

	// Got object?
	if (object)
	{
		ULONG old_image1 = 0, old_image2 = 0, new_image1, new_image2, old_flags = 0, new_flags;
		BOOL redraw = 0;
		struct DiskObject *old;

		// Save old icon
		old = object->icon;
		object->icon = 0;

		// Not deleted?
		if (!notify->dn_Flags)
		{
			// Get image checksums
			old_image1 = IconCheckSum(old, 0);
			old_image2 = IconCheckSum(old, 1);
			old_flags = GetIconFlags(old);

			// Get new icon
			backdrop_get_icon(info, object, GETICON_CD | GETICON_NO_POS | GETICON_FAIL);
		}

		// No icon now?
		if (!object->icon)
		{
			// Replace old icon
			object->icon = old;

			// Erase old object
			backdrop_erase_icon(info, object, BDSF_RECALC);

			// Is object a disk?
			if (object->type == BDO_DISK)
			{
				// Signal to refresh drives
				IPC_Command(info->ipc, MAINCMD_REFRESH_DRIVES, 0, 0, 0, 0);
			}

			// Remove object from list
			backdrop_remove_object(info, object);
		}

		// Ok to keep going
		else
		{
			// Get image checksums
			new_image1 = IconCheckSum(object->icon, 0);
			new_image2 = IconCheckSum(object->icon, 1);
			new_flags = GetIconFlags(object->icon);

			// Mask out uninteresting flag bits
			old_flags &= ICONF_BORDER_OFF | ICONF_BORDER_ON | ICONF_NO_LABEL;
			new_flags &= ICONF_BORDER_OFF | ICONF_BORDER_ON | ICONF_NO_LABEL;

			// Need to redraw?
			if (old_image1 != new_image1 || old_image2 != new_image2 || old_flags != new_flags)
			{
				// Erase old object
				backdrop_erase_icon(info, object, 0);
				redraw = 1;
			}

			// Free old icon
			if (old)
				backdrop_free_icon(info, old);

			// Fix new icon size
			backdrop_get_icon(info, object, GETICON_POS_ONLY | GETICON_SAVE_POS | GETICON_KEEP);

			// Need to get masks?
			if (!backdrop_icon_border(object))
			{
				// Get masks for this icon
				backdrop_get_masks(object);
			}

			// Show new icon
			if (redraw)
				backdrop_render_object(info, object, BRENDERF_CLIP);
		}

		ret = 1;
	}

	// Otherwise, got lister?
	else if (lister)
	{
		// Tell lister to get icons
		IPC_Command(lister->ipc, LISTER_GET_ICONS, 0, 0, 0, 0);
		ret = 1;
	}

	// Or, on the desktop?
	else if (info->flags & BDIF_MAIN_DESKTOP)
	{
		BPTR lock1, lock2;

		// Lock the desktop folder and the changed directory
		if ((lock1 = Lock(environment->env->desktop_location, ACCESS_READ)) &&
			(lock2 = Lock(notify->dn_Name, ACCESS_READ)))
		{
			// Same directory?
			if (SameLock(lock1, lock2) == LOCK_SAME)
			{
				// Update the desktop folder
				misc_startup("dopus_desktop_update", MENU_UPDATE_DESKTOP, GUI->window, 0, TRUE);
			}

			// Unlock second lock
			UnLock(lock2);
		}

		// Unlock first lock
		UnLock(lock1);
	}

	// Unlock list
	unlock_listlock(&info->objects);

	// Free message
	ReplyFreeMsg(notify);
	FreeVec(name_buf);
	return ret;
}
