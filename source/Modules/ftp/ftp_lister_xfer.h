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

#ifndef _LISTER_XFER_H
#define _LISTER_XFER_H

int replace_requester(struct hook_rec_data *, ULONG flags, struct entry_info *src, struct entry_info *dst);

int xfer_update(void *ui, unsigned int total, unsigned int bytes);
int getput_update(void *ui, unsigned int total, unsigned int bytes);

//
//	Flags for replace_requester()
//
enum {
	REPLACEF_NORESUME = (1 << 0),  // Resume option not supported on some endpoint
	REPLACEF_UPDATE = (1 << 1),	   // Handle the 'Copy UPDATE' option
	REPLACEF_NEWER = (1 << 2),	   // Handle the 'Copy NEWER' option
};

//
//	Returned by replace_requester()
//
enum {
	REPLACE_ABORT,
	REPLACE_RESUME,
	REPLACE_RESUMEALL,
	REPLACE_REPLACE,
	REPLACE_REPLACEALL,
	REPLACE_SKIP,
	REPLACE_SKIPALL,
	REPLACE_INVALID
};

//
//	Used by handle_samefilename()
//
enum { SKIP_RESUME_AND_REPLACE = 0, SKIP_REPLACE = 1, SKIP_RESUME = 2, REQUEST_ALL = 3 };

#endif
