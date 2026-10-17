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

#include "format.h"

ModuleInfo module_info =
	{1, "format.module", "format.catalog", 0, 1, {{0, "Format", MSG_FORMAT_DESC, FUNCF_NO_ARGS, 0}}};

ConfigWindow format_window = {{POS_CENTER, POS_CENTER, 40, 10}, {0, 0, 44, 72}};

struct TagItem

	format_layout_tags[] = {{GTCustom_LayoutRel, GAD_FORMAT_LAYOUT}, {TAG_DONE}},

	format_devices_tags[] = {{DLV_ShowSelected, 0}, {TAG_MORE, (IPTR)&format_layout_tags}},

	format_name_tags[] = {{GTST_MaxChars, 30}, {TAG_MORE, (IPTR)&format_layout_tags}};

ObjectDef format_objects[] = {

	// Background area
	{OD_AREA,
	 0,
	 {0, 0, SIZE_MAXIMUM, SIZE_MAX_LESS - 1},
	 {2, 2, -2, -10},
	 0,
	 AREAFLAG_RECESSED | AREAFLAG_ERASE,
	 GAD_FORMAT_LAYOUT,
	 0},

	// Device lister
	{OD_GADGET,
	 MY_LISTVIEW_KIND,
	 {0, 0, 10, 8},
	 {4, 4, 24, 40},
	 0,
	 LISTVIEWFLAG_CURSOR_KEYS,
	 GAD_FORMAT_DEVICES,
	 format_devices_tags},

	// Name
	{OD_GADGET,
	 STRING_KIND,
	 {16, 0, SIZE_MAXIMUM, 1},
	 {36, 4, -4, 4},
	 MSG_FORMAT_NAME,
	 0,
	 GAD_FORMAT_NAME,
	 format_name_tags},

	// FFS
	{OD_GADGET,
	 CHECKBOX_KIND,
	 {10, 1, 0, 1},
	 {36, 10, 26, 4},
	 MSG_FORMAT_FFS,
	 PLACETEXT_RIGHT,
	 GAD_FORMAT_FFS,
	 format_layout_tags},

	// International
	{OD_GADGET,
	 CHECKBOX_KIND,
	 {10, 2, 0, 1},
	 {36, 15, 26, 4},
	 MSG_FORMAT_INTERNATIONAL,
	 PLACETEXT_RIGHT,
	 GAD_FORMAT_INTERNATIONAL,
	 format_layout_tags},

	// Caching
	{OD_GADGET,
	 CHECKBOX_KIND,
	 {10, 3, 0, 1},
	 {36, 20, 26, 4},
	 MSG_FORMAT_CACHING,
	 PLACETEXT_RIGHT,
	 GAD_FORMAT_CACHING,
	 format_layout_tags},

	// Long File Names
	{OD_GADGET,
	 CHECKBOX_KIND,
	 {10, 4, 0, 1},
	 {36, 25, 26, 4},
	 MSG_FORMAT_LNFS,
	 PLACETEXT_RIGHT,
	 GAD_FORMAT_LNFS,
	 format_layout_tags},

	// Install
	{OD_GADGET,
	 CHECKBOX_KIND,
	 {10, 5, 0, 1},
	 {36, 30, 26, 4},
	 MSG_FORMAT_INSTALL,
	 PLACETEXT_RIGHT,
	 GAD_FORMAT_INSTALL,
	 format_layout_tags},

	// Trashcan
	{OD_GADGET,
	 CHECKBOX_KIND,
	 {10, 6, 0, 1},
	 {36, 35, 26, 4},
	 MSG_FORMAT_TRASHCAN,
	 PLACETEXT_RIGHT,
	 GAD_FORMAT_TRASHCAN,
	 format_layout_tags},

	// Verify
	{OD_GADGET,
	 CHECKBOX_KIND,
	 {10, 7, 0, 1},
	 {36, 40, 26, 4},
	 MSG_FORMAT_VERIFY,
	 PLACETEXT_RIGHT,
	 GAD_FORMAT_VERIFY,
	 format_layout_tags},

	// Status bar
	{OD_AREA,
	 TEXTPEN,
	 {0, 8, SIZE_MAXIMUM, 1},
	 {4, 48, -4, 6},
	 0,
	 AREAFLAG_RECESSED | AREAFLAG_ERASE | TEXTFLAG_CENTER,
	 GAD_FORMAT_STATUS,
	 format_layout_tags},

	// Format
	{OD_GADGET, BUTTON_KIND, {0, POS_RIGHT_JUSTIFY, 12, 1}, {2, -2, 12, 6}, MSG_FORMAT_FORMAT, 0, GAD_FORMAT_FORMAT, 0},

	// Quick format
	{OD_GADGET,
	 BUTTON_KIND,
	 {POS_CENTER, POS_RIGHT_JUSTIFY, 12, 1},
	 {0, -2, 12, 6},
	 MSG_FORMAT_QUICK_FORMAT,
	 0,
	 GAD_FORMAT_QUICK_FORMAT,
	 0},

	// Cancel
	{OD_GADGET,
	 BUTTON_KIND,
	 {POS_RIGHT_JUSTIFY, POS_RIGHT_JUSTIFY, 12, 1},
	 {-2, -2, 12, 6},
	 MSG_FORMAT_CANCEL,
	 BUTTONFLAG_CANCEL_BUTTON,
	 GAD_FORMAT_CANCEL,
	 0},

	{OD_END}};

// 68000 code of the Kickstart 2.0 bootblock, as 32-bit big-endian words
ULONG bootblock_20[22] = {0x00000370,

								  0x43fa003e, 0x70254eae, 0xfdd84a80, 0x670c2240, 0x08e90006, 0x00224eae, 0xfe6243fa,
								  0x00184eae, 0xffa04a80, 0x670a2040, 0x20680016, 0x70004e75, 0x70ff4e75, 0x646f732e,
								  0x6c696272, 0x61727900, 0x65787061, 0x6e73696f, 0x6e2e6c69, 0x62726172, 0x79000000};
