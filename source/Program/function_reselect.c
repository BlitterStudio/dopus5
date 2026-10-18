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

// RESELECT function
DOPUS_FUNC(function_reselect)
{
	Lister *lister;
	ListerReselectData *packet;
	ULONG side;

	// Get current lister
	if ((lister = function_lister_current(&handle->source_paths)))
	{
		side = function_path_side(handle->source_paths.current);

		if ((packet = AllocVec(sizeof(ListerReselectData), MEMF_CLEAR)))
		{
			packet->reselect = &lister->reselect;
			packet->flags = 1;
			packet->side = side;

			// Call reselect
			if (!IPC_Command(lister->ipc, LISTER_DO_RESELECT, 0, packet, packet, (struct MsgPort *)-1))
				FreeVec(packet);
		}
		else if (!side)
			IPC_Command(lister->ipc, LISTER_DO_RESELECT, 1, (APTR)&lister->reselect, 0, (struct MsgPort *)-1);
	}

	return 1;
}
