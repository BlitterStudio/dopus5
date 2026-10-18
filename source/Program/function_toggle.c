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

// TOGGLE internal function
DOPUS_FUNC(function_toggle)
{
	Lister *lister;
	ULONG side;

	// Get current lister
	if ((lister = function_lister_current(&handle->source_paths)))
	{
		side = function_path_side(handle->source_paths.current);

		// Select global state
		IPC_Command(lister->ipc, LISTER_SELECT_GLOBAL_TOGGLE, side, 0, 0, REPLY_NO_PORT);

		// Refresh window
		IPC_Command(lister->ipc, LISTER_REFRESH_WINDOW, 0, (APTR)(IPTR)side, 0, REPLY_NO_PORT);
	}

	return 1;
}
