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

enum {
	MAKEDIR_NAME,
	MAKEDIR_NOICON,
	MAKEDIR_SELECT,
	MAKEDIR_NEW,
	MAKEDIR_READ,
};

// MAKEDIR internal function
DOPUS_FUNC(function_makedir)
{
	char *dir_path, *dir_name;
	PathNode *path;
	short ret = 1, ask_flag = 1, lister_flag = 1, icon_flag = 1, def_icon = 0, select = 0, new_flag = 0, read_flag = 0;
	short add_flag = 1;
	FileChange *change;

	// Allocate buffer for directory path
	if (!(dir_path = AllocVec(768, MEMF_CLEAR)))
		return 0;
	dir_name = dir_path + 512;

	// Get current lister
	path = function_path_current(&handle->source_paths);

	// Default to icons?
	if (GUI->flags & GUIF_SAVE_ICONS || handle->flags & FUNCF_ICONS ||
		(path && path->lister && path->lister->flags & LISTERF_VIEW_ICONS))
		def_icon = 1;

	// Parsed arguments?
	if (instruction->funcargs)
	{
		// No icons?
		if (instruction->funcargs->FA_Arguments[MAKEDIR_NOICON])
			icon_flag = 0;

		// Select?
		if (instruction->funcargs->FA_Arguments[MAKEDIR_SELECT])
			select = 1;

		// New?
		if (instruction->funcargs->FA_Arguments[MAKEDIR_NEW])
			new_flag = 1;

		// Read?
		if (instruction->funcargs->FA_Arguments[MAKEDIR_READ])
			read_flag = 1;

		// Directory?
		if (instruction->funcargs->FA_Arguments[MAKEDIR_NAME])
		{
			char *path = (char *)instruction->funcargs->FA_Arguments[MAKEDIR_NAME];

			// Don't need to ask
			ask_flag = 0;

			// See if it's a full pathname
			if (strchr(path, '/') || strchr(path, ':'))
			{
				strcpy(dir_path, path);
				lister_flag = 0;
			}

			// It's just a name
			else
			{
				// Build path
				strcpy(dir_path, handle->source_path);
				AddPart(dir_path, path, 512);
			}
		}
	}

	// If no path, and we need one, return
	if (!function_valid_path(path) && lister_flag)
		return 0;

	// Loop until successful or aborted
	FOREVER
	{
		BPTR lock;

		// Do we need to ask?
		if (ask_flag)
		{
			// Get directory name from user
			if (!(ret = function_request(handle,
										 GetString(&locale, MSG_ENTER_DIRECTORY_NAME),
										 SRF_BUFFER | SRF_PATH_FILTER,
										 dir_name,
										 GUI->def_filename_length,
										 GetString(&locale, (def_icon) ? MSG_MAKEDIR_ICON : MSG_MAKEDIR_NOICON),
										 GetString(&locale, (def_icon) ? MSG_MAKEDIR_NOICON : MSG_MAKEDIR_ICON),
										 GetString(&locale, MSG_CANCEL),
										 0)))
			{
				ret = -1;
				break;
			}

			// Return pressed on string gadget
			else if (ret == -1)
			{
				ret = 1;
			}

			// If name is empty, abort
			if (!dir_name[0])
			{
				ret = -1;
				break;
			}

			// Build path name
			strcpy(dir_path, handle->source_path);
			AddPart(dir_path, dir_name, 512);

			// No icon?
			if (def_icon == ret - 1)
				icon_flag = 0;
		}

		// If we want an icon, check name isn't too long
		if (icon_flag)
		{
			char *name;

			// Get filename
			name = FilePart(dir_path);

			// Maximum 25 characters
			if (strlen(name) > GUI->def_filename_length - 5)
				name[GUI->def_filename_length - 5] = 0;
		}

		// Create directory
		if ((lock = OriginalCreateDir(dir_path)))
		{
			// Examine the new drawer
#ifdef USE_64BIT
			ExamineLock64(lock, (FileInfoBlock64 *)handle->s_info);
#else
			Examine(lock, handle->s_info);
#endif

			// Unlock the new directory
			UnLock(lock);
			break;
		}

		// Put up error requester
		if (!ask_flag || (function_error(handle, dir_path, MSG_ERROR_CREATING_DIRECTORY, -IoErr())) == -1)
		{
			if (!ask_flag)
				ret = 0;
			else
				ret = -1;
			break;
		}
	}

	// Aborted?
	if (ret < 1)
	{
		if (ret == -1)
			function_abort(handle);
		FreeVec(dir_path);
		return 0;
	}

	// Write icon?
	if (icon_flag)
	{
		// Write icon
		if ((icon_write(ICONTYPE_DRAWER, dir_path, 0, 0, 0, 0)) != ICONWRITE_OK)
			icon_flag = 0;
	}

	// Got a lister?
	if (lister_flag)
	{
		// Read directory into lister?
		if (read_flag)
		{
			// Read directory
			function_read_directory(handle, path->lister, function_path_side(path), dir_path);
			add_flag = 0;
		}

		// Keep same lister
		else
		{
			// Tell lister to update it's stamp
			path->flags |= LISTNF_UPDATE_STAMP;

			// Display status text
			function_text(handle, GetString(&locale, MSG_DIRECTORY_CREATED));
		}
	}

	// Read into new lister?
	if (new_flag)
	{
		// Create a new lister
		read_new_lister(dir_path, 0, IEQUALIFIER_LSHIFT);
	}

	// No path supplied?
	if (!lister_flag)
	{
		BPTR lock;

		// Lock path
		if ((lock = Lock(dir_path, ACCESS_READ)))
		{
			// Get full pathname
			DevNameFromLockDopus(lock, dir_path, 512);
			UnLock(lock);
		}

		// Clear name pointer
		if ((dir_name = FilePart(dir_path)))
			*dir_name = 0;

		// Use path pointer
		dir_name = dir_path;
	}

	// Use path in lister
	else
		dir_name = path->path;

	// Ok to add?
	if (add_flag)
	{
		// Add entry to the list
		if ((change = function_filechange_addfile(handle, dir_name, handle->s_info, 0, 0)))
		{
			// Select/show it
			change->node.ln_Pri = (select) ? FCF_SHOW | FCF_SELECT : FCF_SHOW;
		}

		// Need to add icon?
		if (icon_flag &&
			(change = function_filechange_loadfile(handle, dir_name, handle->s_info->fib_FileName, FFLF_ICON)))
		{
			// Select/show it
			if (select && environment->env->settings.icon_flags & ICONFLAG_AUTOSELECT)
				change->node.ln_Pri = FCF_SELECT;
		}
	}

	// Free buffer
	FreeVec(dir_path);
	return 1;
}
