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

#include "themes.h"

#define ARG_1 "AS=TO"
#define ARG_2 "FILE=FROM,APPLYPALETTE=AP/S,APPLYFONTS=AF/S,APPLYBACKGROUNDS=AB/S,APPLYSOUNDS=AS/S"
#define ARG_3 "FILE=FROM,AS=TO"

char *func_templates[] = {ARG_1, ARG_2, ARG_1, ARG_3};

ModuleInfo_4 module_info = {1,
							"themes.module",
							"themes.catalog",
							0,
							4,
							{{SAVETHEME, "SaveTheme", MSG_SAVE_THEME_DESC, 0, ARG_1},
							 {LOADTHEME, "LoadTheme", MSG_LOAD_THEME_DESC, FUNCF_WANT_ENTRIES, ARG_2},
							 {BUILDTHEME, "BuildTheme", MSG_BUILD_THEME_DESC, 0, ARG_1},
							 {CONVERTTHEME, "ConvertTheme", MSG_CONVERT_THEME_DESC, 0, ARG_3}}};

char *pen_settings[] =
	{"icons", "files", "dirs", "selfiles", "seldirs", "devices", "assigns", "source", "dest", "gauge", "user", 0};

char apply_lookup[] = {'p', 'f', 'b', 's'};

char *sound_lookup[] = {"Bad disk inserted",
						"EmptyRecycleBin",
						"Startup",
						"SystemStart",
						"Shutdown",
						"SystemExit",
						"Hide",
						"SystemExclamation",
						"Reveal",
						".Default",
						"Disk inserted",
						"SystemQuestion",
						"Disk removed",
						"SystemDefault",
						"Open lister",
						"Maximize",
						"Open buttons",
						"MenuPopup",
						"Open group",
						"MenuCommand",
						"Close lister",
						"Minimize",
						"Close buttons",
						"SystemAsterisk",
						"Close group",
						"SystemHand",
						"FTP close connection",
						"Close",
						"FTP connect fail",
						"RestoreDown",
						"FTP connect success",
						"RestoreUp",
						"FTP copy fail",
						"AppGPFault",
						"FTP copy success",
						".Default",
						"FTP error",
						"SystemAsterisk",
						0,
						0};
