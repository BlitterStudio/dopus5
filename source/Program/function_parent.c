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

// PARENT, ROOT
DOPUS_FUNC(function_parent)
{
	PathNode *path;
	short ret = 0, a;

	// Get current source lister
	if (!(path = function_path_current(&handle->source_paths)) || !path->lister)
		return 0;

	// Get current path
	strcpy(handle->inst_data, path->path);

	// Try twice
	for (a = 0; a < 2; a++)
	{
		// Do parent/root
		if (command->function == FUNC_PARENT)
			ret = path_parent(handle->inst_data);
		else
			ret = path_root(handle->inst_data);

		// Successful?
		if (ret)
			break;

		// For second time through, expand path
		if (a == 0)
		{
			BPTR lock;

			// Lock path
			if ((lock = Lock(handle->inst_data, ACCESS_READ)))
			{
				// Expand path
				DevNameFromLockDopus(lock, handle->inst_data, 512);
				UnLock(lock);
			}
		}
	}

	// Valid directory?
	if (ret)
	{
		// Read directory
		handle->flags = GETDIRF_CANCHECKBUFS | GETDIRF_CANMOVEEMPTY;
		function_read_directory(handle, path->lister, function_path_side(path), handle->inst_data);
	}
	else
	{
		// if we are already at the root show the device list
		function_unlock_paths(handle, &handle->source_paths, 1);
		// function_launch(FUNCTION_RUN_FUNCTION,def_function_devicelist,0,0,handle->source_paths.current->lister,0,handle->source_paths.current->path,0,0,0,0);
		function_launch_quick(FUNCTION_RUN_FUNCTION, def_function_devicelist, path->lister);
	}

	return 1;
}
