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

// Display an error message
int status_display_error(Lister *lister, int errcode)
{
	return status_display_error_side(lister, 0, errcode);
}

// Display an error message for a dual panel (1-based side, 0 = active)
int status_display_error_side(Lister *lister, ULONG side, int errcode)
{
	static char error_text[80];

	// Need to get error code ourselves?
	if (errcode == -1)
		errcode = IoErr();

	// Valid code?
	if (errcode == 0)
		return 0;

	// Get error message
	Fault(errcode, GetString(&locale, MSG_DOS_ERROR), error_text, 80);

	// Print error
	status_text_side(lister, side, error_text);
	return 1;
}

// Display status text
void status_text(Lister *lister, char *text)
{
	status_text_side(lister, 0, text);
}

// Display status text, stored with a dual panel (1-based side, 0 = active)
void status_text_side(Lister *lister, ULONG side, char *text)
{
	if (lister)
	{
		char *copy;

		// Copy text
		if ((copy = AllocVec(strlen(text) + 1, 0)))
			strcpy(copy, text);

		IPC_Command(lister->ipc, LISTER_STATUS, side, text, copy, 0);
	}
}

// Display OK status text
void status_okay(Lister *lister)
{
	status_text(lister, GetString(&locale, MSG_OKAY));
}
