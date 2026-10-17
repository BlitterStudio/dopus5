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

#define CATCOMP_NUMBERS
#include "format.strings"
#include "module_deps.h"

#ifndef __amigaos3__
	#pragma pack(2)
#endif
typedef struct
{
	struct Screen *screen;
	struct Window *window;
	IPCData *ipc;

	ConfigWindow win_dims;
	NewConfigWindow new_win;
	ObjectList *list;

	Att_List *device_list;

	struct InfoData info;
	struct FileInfoBlock fib;
	char disk_name[32];

	char default_name[32];
	char default_ffs;
	char default_int;
	char default_cache;
	char default_lnfs;
	char default_trash;
	char default_boot;
	char default_verify;

	Point window_pos;
	char pos_valid;

	char status_title[80];
	unsigned long dos_type;

	char selection[80];

	short abort_bit;
} format_data;
#ifndef __amigaos3__
	#pragma pack()
#endif

BOOL format_open(format_data *data, BOOL);
void format_close(format_data *data);
void format_free(format_data *data);
Att_List *get_device_list(char *);
void show_device_info(format_data *data);
BOOL start_format(format_data *data, unsigned short type, BOOL);
BOOL do_format(format_data *data, DiskHandle *disk, unsigned short type, short *);
// BOOL do_raw_format(format_data *data,DiskHandle *disk,struct Window *status);
// void do_install(format_data *data,DiskHandle *disk,struct Window *status);
BOOL do_raw_format(format_data *data, DiskHandle *disk, APTR status);
void do_install(format_data *data, DiskHandle *disk, APTR status);

extern ConfigWindow format_window;
extern ObjectDef format_objects[];

enum {
	GAD_BASE,

	GAD_FORMAT_LAYOUT,

	GAD_FORMAT_DEVICES,
	GAD_FORMAT_NAME,
	GAD_FORMAT_FFS,
	GAD_FORMAT_INTERNATIONAL,
	GAD_FORMAT_CACHING,
	GAD_FORMAT_LNFS,
	GAD_FORMAT_TRASHCAN,
	GAD_FORMAT_INSTALL,
	GAD_FORMAT_VERIFY,
	GAD_FORMAT_STATUS,
	GAD_FORMAT_FORMAT,
	GAD_FORMAT_QUICK_FORMAT,
	GAD_FORMAT_CANCEL,
};

extern ULONG bootblock_20[22];
