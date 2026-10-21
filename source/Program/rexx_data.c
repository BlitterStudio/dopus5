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

RexxCommandList rexx_commands[] = {

	{"lister", RXCMD_LISTER},
	{"new", RXCMD_NEW},
	{"close", RXCMD_CLOSE},
	{"query", RXCMD_QUERY},
	{"set", RXCMD_SET},
	{"addstem", RXCMD_ADD_STEM},
	{"add", RXCMD_ADD},
	{"reload", RXCMD_RELOAD},
	{"remove", RXCMD_REMOVE},
	{"select", RXCMD_SELECT},
	{"refresh", RXCMD_REFRESH},
	{"clear", RXCMD_CLEAR},
	{"empty", RXCMD_EMPTY},
	{"wait", RXCMD_WAIT},
	{"read", RXCMD_READ},
	{"copy", RXCMD_COPY},
	{"iconify", RXCMD_ICONIFY},
	{"move", RXCMD_MOVEMENT},
	{"findcache", RXCMD_FINDCACHE},
	{"freecaches", RXCMD_FREECACHES},

	{"path", RXCMD_PATH},
	{"position", RXCMD_POSITION},
	{"busy", RXCMD_BUSY},
	{"handler", RXCMD_HANDLER},
	{"visible", RXCMD_VISIBLE},
	{"title", RXCMD_TITLE},
	{"header", RXCMD_HEADER},
	{"toolbar", RXCMD_TOOLBAR},
	{"mode", RXCMD_MODE},
	{"label", RXCMD_LABEL},
	{"field", RXCMD_FIELD},
	{"window", RXCMD_WINDOW},
	{"proc", RXCMD_PROC},
	{"value", RXCMD_VALUE},

	{"files", RXCMD_FILES},
	{"dirs", RXCMD_DIRS},
	{"entries", RXCMD_ENTRIES},
	{"selfiles", RXCMD_SELFILES},
	{"seldirs", RXCMD_SELDIRS},
	{"selentries", RXCMD_SELENTRIES},
	{"firstsel", RXCMD_FIRSTSEL},
	{"entry", RXCMD_ENTRY},
	{"numfiles", RXCMD_NUMFILES},
	{"numdirs", RXCMD_NUMDIRS},
	{"numentries", RXCMD_NUMENTRIES},
	{"numbytes", RXCMD_NUMBYTES},
	{"numselfiles", RXCMD_NUMSELFILES},
	{"numseldirs", RXCMD_NUMSELDIRS},
	{"numselentries", RXCMD_NUMSELENTRIES},
	{"numselbytes", RXCMD_NUMSELBYTES},

	{"sort", RXCMD_SORT},
	{"separate", RXCMD_SEPARATE},
	{"display", RXCMD_DISPLAY},
	{"flags", RXCMD_FLAGS},
	{"show", RXCMD_SHOW},
	{"hide", RXCMD_HIDE},
	{"default", RXCMD_DEFAULT},

	{"source", RXCMD_SOURCE},
	{"dest", RXCMD_DEST},
	{"all", RXCMD_ALL},
	{"active", RXCMD_ACTIVE},
	{"lock", RXCMD_LOCK},
	{"off", RXCMD_OFF},

	{"progress", RXCMD_PROGRESS},
	{"newprogress", RXCMD_NEWPROGRESS},
	{"count", RXCMD_COUNT},
	{"name", RXCMD_NAME},

	{"case", RXCMD_CASE},
	{"namelength", RXCMD_NAMELENGTH},
	{"commentlength", RXCMD_COMMENTLENGTH},

	{"abort", RXCMD_ABORT},

	{"dopus", RXCMD_DOPUS},
	{"back", RXCMD_BACK},
	{"front", RXCMD_FRONT},
	{"getstring", RXCMD_GETSTRING},
	{"request", RXCMD_REQUEST},
	{"getfiletype", RXCMD_GETFILETYPE},
	{"addtrap", RXCMD_ADDTRAP},
	{"remtrap", RXCMD_REMTRAP},
	{"version", RXCMD_VERSION},
	{"screen", RXCMD_SCREEN},
	{"error", RXCMD_ERROR},
	{"addappicon", RXCMD_ADDAPPICON},
	{"remappicon", RXCMD_REMAPPICON},
	{"setappicon", RXCMD_SETAPPICON},
	{"hexread", RXCMD_HEXREAD},
	{"ansiread", RXCMD_ANSIREAD},
	{"smartread", RXCMD_SMARTREAD},
	{"send", RXCMD_SEND},
	{"rx", RXCMD_RX},
	{"checkdesktop", RXCMD_CHECKDESKTOP},
	{"getdesktop", RXCMD_GETDESKTOP},
	{"matchdesktop", RXCMD_MATCHDESKTOP},
	{"desktoppopup", RXCMD_DESKTOPPOPUP},
	{"script", RXCMD_SCRIPT},
	{"background", RXCMD_BACKGROUND},
	{"sound", RXCMD_SOUND},
	{"pens", RXCMD_PENS},
	{"palette", RXCMD_PALETTE},
	{"font", RXCMD_FONT},
	{"icons", RXCMD_ICONS},

	{"command", RXCMD_COMMAND},

	{0, 0}};

short rexx_commands_can_do_all[] = {RXCMD_CLOSE, RXCMD_REFRESH, RXCMD_ICONIFY, 0};

char *
	sort_method_strings[] =
		{"name", "size", "protect", "date", "comment", "filetype", "owner", "group", "netprot", "version", 0},

   *separate_method_strings[] = {"mix", "dirsfirst", "filesfirst", 0},

   *sort_flags_strings[] = {"reverse", "noicons", "hidden", 0},

   *full_string[] = {"full", "date", 0},

   *quick_string[] = {"quick", 0},

   *on_off_strings[] = {"off", "on", "0", "1", 0},

   *on_off_strings2[] = {"off", "on", 0},

   *force_string[] = {"force", 0},

   *getfiletype_keys[] = {"id", 0},

   *custhandler_keys[] =
	   {"quotes", "fullpath", "editing", "nopopups", "gauge", "subdrop", "inherit", "leavegauge", "synctraps", 0},

   *port_keys[] = {"port", 0},

   *all_keys[] = {"all", 0},

   *new_lister_keys[] = {"toolbar", "parent", "inactive", "invisible", "iconify", "fromicon", "mode", 0},

   *mode_keys[] = {"name", "icon", "showall", "action", 0},

   *quit_keys[] = {"quit", "delete", 0},

   *reader_keys[] = {"quit", "delete", "pos", "hex", "ansi", "smart", "file", 0},

   *command_keys[] = {"wait", "source", "dest", "original", 0},

   *pos_keys[] = {"pos", "quotes", "info", "snap", "close", "icon", "menu", "local", "locked", 0},

   *seticon_keys[] = {"text", "busy", "locked", 0},

   *app_commands[] =
	   {"removed", "doubleclick", "dropfrom", "snapshot", "unsnapshot", "close", "info", "menu", "menuhelp"},

   *progress_keys[] = {"name", "file", "info", "bar", "abort", "title", "off", "info2", "info3", 0},

   *secure_keys[] = {"secure", 0},

   *excommand_keys[] = {"desc",
						"template",
						"program",
						"source",
						"dest",
						"ext",
						"type",
						"private",
						"help",
						"remove",
						"handler",
						"temp",
						0},

   *lock_keys[] = {"state", "format", 0},

   *wait_keys[] = {"wait", 0},

   *move_keys[] =
	   {"to", "up", "down", "pgup", "pgdn", "top", "bottom", "left", "right", "pglt", "pgrt", "home", "end", "show", 0},

   *update_key[] = {"update", 0},

   *background_key[] = {"desktop", "lister", "req", "tile", "center", "stretch", "precision", "custom", "border", 0},

   *custom_key[] = {"custom", 0},

   *precision_key[] = {"none", "gui", "icon", "image", "exact", 0},

   *labelcol_keys[] = {"desktop", "windows", "jam1", "jam2", "shadow", "outline", 0},

   *pens_key[] =
	   {"icons", "files", "dirs", "selfiles", "seldirs", "devices", "assigns", "source", "dest", "gauge", "user", "tooltips", "stripes", 0},

   *font_key[] = {"screen", "listers", "iconsd", "iconsw", 0};

unsigned short rexx_error_table[] = {1,	 MSG_RXERR_1,  5,  MSG_RXERR_5,	 6,	 MSG_RXERR_6,	   8,  MSG_RXERR_8,
									 10, MSG_RXERR_10, 12, MSG_RXERR_12, 13, MSG_RXERR_13,	   14, MSG_RXERR_14,
									 15, MSG_RXERR_15, 20, MSG_RXERR_20, 0,	 MSG_RXERR_UNKNOWN};
