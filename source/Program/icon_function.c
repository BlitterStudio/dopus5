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

// Run a function on some icons
void icon_function(BackdropInfo *info, BackdropObject *only_one, char *data, Cfg_Function *func, ULONG flags)
{
	short count = 0;
	struct ArgArray *array;
	BackdropObject *object;
	DirBuffer *buffer = 0;
	char *source_path = 0;

	// Lock backdrop list
	lock_listlock(&info->objects, 1);

	// Go through backdrop list
	for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ;
		 object = (BackdropObject *)object->node.ln_Succ)
	{
		// Skip invalid icons
		if (object->type != BDO_LEFT_OUT || (!info->lister && !(object->flags & BDOF_DESKTOP_FOLDER)))
			continue;

		// Want this icon?
		if (!only_one || only_one == object)
		{
			// Is object selected?
			if ((only_one || object->state) && object->icon)
			{
				// Increment counts
				if (object->icon->do_Type == WBDRAWER || object->icon->do_Type == WBGARBAGE ||
					object->icon->do_Type == WBPROJECT || object->icon->do_Type == WBTOOL)
					++count;

				// Only doing one?
				if (only_one)
					break;
			}
		}
	}

	// Nothing to work on?
	if (count == 0 || !(array = NewArgArray()))
	{
		unlock_listlock(&info->objects);
		return;
	}

	// Got a lister?
	if (info->lister)
	{
		// Lock buffer
		buffer_lock((buffer = info->lister->cur_buffer), FALSE);
	}

	// Source path from icon?
	else if (only_one && only_one->path && (source_path = AllocVec(strlen(only_one->path) + 1, 0)))
		strcpy(source_path, only_one->path);

	// Otherwise, assume this is the desktop folder
	else if (info->flags & BDIF_MAIN_DESKTOP &&
			 (source_path = AllocVec(strlen(environment->env->desktop_location) + 1, 0)))
		strcpy(source_path, environment->env->desktop_location);

	// Go through backdrop list again
	for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ;
		 object = (BackdropObject *)object->node.ln_Succ)
	{
		// Skip invalid icons
		if (object->type != BDO_LEFT_OUT || (!info->lister && !(object->flags & BDOF_DESKTOP_FOLDER)))
			continue;

		// Want this icon?
		if (!only_one || only_one == object)
		{
			// Is object selected?
			if ((only_one || object->state) && object->icon)
			{
				char name[80];
				BOOL dir = 0, link = 0, icon = 0;
				DirEntry *entry = 0;
				struct ArgArrayEntry *aae;

				// Build name
				stccpy(name, object->name, sizeof(name));

				// Got a buffer?
				if (buffer)
				{
					// See if we can find this entry
					if ((entry = find_entry(&buffer->entry_list, object->name, 0, buffer->more_flags & DWF_CASE)) ||
						(entry = find_entry(&buffer->reject_list, object->name, 0, buffer->more_flags & DWF_CASE)))
					{
						// Directory?
						if (entry->de_Node.dn_Type >= ENTRY_DEVICE)
							dir = 1;

						// Link?
						if (entry->de_Flags & ENTF_LINK)
							link = 1;

						// See if entry has icon
						if (find_entry(&buffer->entry_list,
									   object->name,
									   0,
									   (buffer->more_flags & DWF_CASE) | FINDENTRY_ICON) ||
							find_entry(&buffer->reject_list,
									   object->name,
									   0,
									   (buffer->more_flags & DWF_CASE) | FINDENTRY_ICON))
							icon = 1;
					}

					// If not, use the .info name
					else
					{
						// Add .info
						strcat(name, ".info");
					}
				}

				// Desktop folder?
				else if (info->flags & BDIF_MAIN_DESKTOP)
				{
					// Assume the file has an icon
					icon = 1;
				}

				// Get type from icon
				if (!entry)
				{
					if (object->icon->do_Type == WBDRAWER || object->icon->do_Type == WBGARBAGE)
						dir = 1;
					if (object->flags & BDOF_LINK_ICON)
						link = 1;
				}

				// Tack on a / for directories
				if (dir)
					strcat(name, "/");

				// Allocate array entry
				if ((aae = NewArgArrayEntry(array, name)))
				{
					// Dir?
					if (dir)
						aae->ae_Flags |= AEF_DIR;

					// Link?
					if (link)
						aae->ae_Flags |= AEF_LINK;

					// No icon?
					if (!icon)
						aae->ae_Flags |= AEF_FAKE_ICON;
				}

				// Only doing one?
				if (only_one)
					break;
			}
		}
	}

	// Unlock buffer
	if (buffer)
		buffer_unlock(buffer);

	// Get flags we need
	flags |= (data && *data) ? FUNCF_ICONS : FUNCF_ICONS | FUNCF_ASK_DEST;

	// Launch the function
	function_launch(FUNCTION_RUN_FUNCTION_EXTERNAL,
					func,
					0,
					flags,
					info->lister,
					0,
					(info->lister) ? info->lister->cur_buffer->buf_Path : source_path,
					data,
					array,
					0,
					0);

	// Free source path
	FreeVec(source_path);

	// Unlock list
	unlock_listlock(&info->objects);
}

// Collect the selected objects of a desktop backdrop into an argument array,
// in the same form the icon context menu supplies them. Returns NULL when no
// object is selected. Used by the desktop selection fallback for User Menu and
// Button Bank launches.
struct ArgArray *desktop_selection_argarray(BackdropInfo *info)
{
	struct ArgArray *array;
	BackdropObject *object;
	short count = 0;

	// Lock backdrop list
	lock_listlock(&info->objects, FALSE);

	// Go through backdrop list, count the objects we can supply
	for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ;
		 object = (BackdropObject *)object->node.ln_Succ)
	{
		// Selected object of a type we supply?
		if (object->state && object->icon &&
			(object->type == BDO_DISK || object->type == BDO_BAD_DISK || object->type == BDO_LEFT_OUT))
			++count;
	}

	// Nothing to work on?
	if (count == 0 || !(array = NewArgArray()))
	{
		unlock_listlock(&info->objects);
		return 0;
	}

	// Go through backdrop list again
	for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ;
		 object = (BackdropObject *)object->node.ln_Succ)
	{
		char name[80];
		BOOL dir = 0, link = 0;
		struct ArgArrayEntry *aae;

		// Selected object of a type we supply?
		if (!(object->state && object->icon) ||
			!(object->type == BDO_DISK || object->type == BDO_BAD_DISK || object->type == BDO_LEFT_OUT))
			continue;

		// Drive/disk: supply the device name
		if (object->type == BDO_DISK || object->type == BDO_BAD_DISK)
		{
			// Need a device name
			if (!object->device_name)
				continue;
			stccpy(name, object->device_name, sizeof(name));
		}

		// Left-out icon: supply the object name
		else
		{
			stccpy(name, object->name, sizeof(name));

			// Directory or link?
			if (object->icon->do_Type == WBDRAWER || object->icon->do_Type == WBGARBAGE)
				dir = 1;
			if (object->flags & BDOF_LINK_ICON)
				link = 1;

			// Tack on a / for directories
			if (dir)
				strcat(name, "/");
		}

		// Allocate array entry
		if ((aae = NewArgArrayEntry(array, name)))
		{
			// Dir?
			if (dir)
				aae->ae_Flags |= AEF_DIR;

			// Link?
			if (link)
				aae->ae_Flags |= AEF_LINK;
		}
	}

	// Unlock backdrop list
	unlock_listlock(&info->objects);

	return array;
}
