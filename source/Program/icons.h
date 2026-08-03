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

#ifndef _DOPUS_ICONS
#define _DOPUS_ICONS

#define ICONWRITE_ABORTED -1
#define ICONWRITE_FAILED 0
#define ICONWRITE_OK 1

#define ICONTYPE_DRAWER 0
#define ICONTYPE_TOOL 1
#define ICONTYPE_PROJECT 2
#define ICONTYPE_GROUP 3

// Prototypes
int icon_write(short, char *, BOOL, ULONG, ULONG, short *);
char *isicon(char *);

void icon_function(BackdropInfo *, BackdropObject *, char *, Cfg_Function *, ULONG);

struct ArgArray *desktop_selection_argarray(BackdropInfo *info);

void icon_rename(IPCData *, BackdropInfo *, BackdropObject *);

struct DiskObject *GetProperIcon(char *, short *, ULONG);

#endif
