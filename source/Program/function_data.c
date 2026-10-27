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

// List of Internal commands
CommandList commandlist_internal[] = {

	{{0, 0}, FUNC_ABORT, "Abort", 0, FUNCF_PRIVATE, {NULL}, 0, 0},

	{{0, 0},
	 FUNC_ADDICON,
	 "AddIcon",
	 MSG_FUNC_DESC_ADDICON,
	 FUNCF_ASK_PATH | FUNCF_NEED_ENTRIES,
	 {function_addicon},
	 "NAME,BORDER/S,NOBORDER/S,LABEL/S,NOLABEL/S,CHANGE/S",
	 "f0"},	 //,SOURCE=FROM/K,SAVEPOS/S","f0"},

	{{0, 0}, FUNC_ALL, "All", MSG_FUNC_DESC_ALL, FUNCF_NEED_SOURCE, {function_all}, 0, 0},

	{{0, 0},
	 FUNC_ANSIREAD,
	 "AnsiRead",
	 MSG_FUNC_DESC_ANSIREAD,
	 FUNCF_NEED_FILES | FUNCF_SCRIPT_OVERRIDE,
	 {function_show},
	 "NAME/F,WAIT/S",
	 "f0"},

	{{0, 0}, FUNC_ASSIGN, "Assign", MSG_FUNC_DESC_ASSIGN, FUNCF_NEED_SOURCE, {function_assign}, 0, 0},

	{{0, 0},
	 FUNC_BUFFERLIST,
	 "CacheList",
	 MSG_FUNC_DESC_BUFFERLIST,
	 FUNCF_WANT_SOURCE | FUNCF_CREATE_SOURCE | FUNCF_SINGLE_SOURCE | FUNCF_SPECIAL_OK,
	 {function_devicelist},
	 "NEW/S",
	 "n0"},

	{{0, 0},
	 FUNC_CHECKFIT,
	 "CheckFit",
	 MSG_FUNC_DESC_CHECKFIT,
	 FUNCF_NEED_ENTRIES | FUNCF_CAN_DO_ICONS | FUNCF_NEED_DEST | FUNCF_SINGLE_DEST,
	 {function_getsizes},
	 0,
	 0},

	{{0, 0},
	 FUNC_UNBYTE,
	 "ClearSizes",
	 MSG_FUNC_DESC_CLEARSIZES,
	 FUNCF_NEED_SOURCE | FUNCF_NEED_DIRS | FUNCF_PRIVATE,
	 {function_getsizes},
	 0,
	 0},

	{{0, 0}, FUNC_CLI, "CLI", MSG_FUNC_DESC_CLI, 0, {function_cli}, 0, 0},

	{{0, 0},
	 FUNC_CLOSEBUTTONS,
	 "CloseButtons",
	 MSG_FUNC_DESC_CLOSEBUTTONS,
	 0,
	 {function_closebuttons},
	 "NAME,ALL/S,ICONIFY/S,START/S,HIDE/S",
	 0},

	{{0, 0},
	 FUNC_COMMENT,
	 "Comment",
	 MSG_FUNC_DESC_COMMENT,
	 FUNCF_NEED_ENTRIES,
	 {function_change},
	 "NAME,COMMENT,RECURSE/S",
	 "f0a1"},

	{{0, 0}, FUNC_CONFIGURE, "Configure", MSG_FUNC_DESC_CONFIGURE, FUNCF_NEED_SOURCE, {function_configure}, 0, 0},

	{{0, 0}, FUNC_VERIFY, "Confirm", MSG_FUNC_DESC_VERIFY, 0, {function_verify}, "TEXT/F", "a0"},

	{{0, 0},
	 FUNC_COPY,
	 "Copy",
	 MSG_FUNC_DESC_COPY,
	 FUNCF_ASK_PATH | FUNCF_NEED_ENTRIES | FUNCF_CAN_DO_ICONS | FUNCF_NEED_DEST | FUNCF_ASK_FILTER,
	 {function_copy},
	 "NAME,TO,QUIET/S,UPDATE/S,MOVEWHENSAME/S,NEWER/S",
	 "f0d1"},

	{{0, 0},
	 FUNC_COPYAS,
	 "CopyAs",
	 MSG_FUNC_DESC_COPYAS,
	 FUNCF_ASK_PATH | FUNCF_NEED_ENTRIES | FUNCF_CAN_DO_ICONS | FUNCF_NEED_DEST | FUNCF_ASK_FILTER,
	 {function_copy},
	 "NAME,NEWNAME,TO,QUIET/S,MOVEWHENSAME/S",
	 "f0a1d2"},

	{{0, 0},
	 FUNC_DATESTAMP,
	 "DateStamp",
	 MSG_FUNC_DESC_DATESTAMP,
	 FUNCF_NEED_ENTRIES,
	 {function_change},
	 "NAME,RECURSE/S,DATE/F",
	 "f0a2"},

	{{0, 0}, FUNC_DEFFTYPE, "defftype", 0, FUNCF_NEED_FILES | FUNCF_PRIVATE, {function_show}, "NAME/F,WAIT/S", "f0"},

	{{0, 0},
	 FUNC_DELETE,
	 "Delete",
	 MSG_FUNC_DESC_DELETE,
	 FUNCF_NEED_ENTRIES | FUNCF_CAN_DO_ICONS | FUNCF_ASK_FILTER,
	 {function_delete},
	 "NAME,QUIET/S",
	 "f0"},

	{{0, 0}, FUNC_DELETEFILE, "DeleteFile", 0, FUNCF_PRIVATE, {function_deletefile}, "NAME", "f0"},

	{{0, 0},
	 FUNC_DEVICELIST,
	 "DeviceList",
	 MSG_FUNC_DESC_DEVICELIST,
	 FUNCF_WANT_SOURCE | FUNCF_CREATE_SOURCE | FUNCF_SINGLE_SOURCE | FUNCF_SPECIAL_OK,
	 {function_devicelist},
	 "NEW/S,FULL/S,BRIEF/S",
	 "n0"},

	{{0, 0},
	 FUNC_DOUBLECLICK,
	 "DoubleClick",
	 MSG_FUNC_DESC_USER,
	 FUNCF_NEED_ENTRIES | FUNCF_WANT_DEST,
	 {function_user},
	 "ALT/S,CTRL=CONTROL/S,NAME/F",
	 "f2"},

	{{0, 0},
	 FUNC_DRAGNDROP,
	 "DragNDrop",
	 MSG_FUNC_DESC_USER,
	 FUNCF_NEED_ENTRIES | FUNCF_WANT_DEST,
	 {function_user},
	 "ALT/S,CTRL=CONTROL/S,NAME/F",
	 "f2"},

	{{0, 0},
	 FUNC_CLONE,
	 "Duplicate",
	 MSG_FUNC_DESC_CLONE,
	 FUNCF_NEED_ENTRIES | FUNCF_CAN_DO_ICONS | FUNCF_ASK_FILTER,
	 {function_copy},
	 "NAME,NEWNAME,QUIET/S",
	 "f0a1"},

	{{0, 0}, FUNC_DISKCOPY, "Diskcopy", MSG_FUNC_DESC_DISKCOPY, 0, {function_disk}, 0, 0},

	{{0, 0},
	 FUNC_EDITCOMMAND,
	 "EditCommand",
	 0,
	 FUNCF_PRIVATE | FUNCF_NEED_FILES,
	 {function_editcommand},
	 "NAME/F",
	 "f0"},

	{{0, 0},
	 FUNC_EDITFILETYPE,
	 "EditFiletype",
	 0,
	 FUNCF_PRIVATE | FUNCF_NEED_FILES,
	 {function_loadenvironment},
	 "NAME/F",
	 "f0"},

	{{0, 0},
	 FUNC_ENCRYPT,
	 "Encrypt",
	 MSG_FUNC_DESC_ENCRYPT,
	 FUNCF_ASK_PATH | FUNCF_NEED_FILES | FUNCF_NEED_DEST | FUNCF_ASK_FILTER,
	 {function_copy},
	 "NAME,TO,PASSWORD,QUIET/S",
	 "f0d1a2"},

	{{0, 0}, FUNC_HUNT, "FindFile", MSG_FUNC_DESC_FINDFILE, FUNCF_NEED_DIRS, {function_hunt}, 0, 0},

	{{0, 0}, FUNC_ENDFUNCTION, "FinishSection", MSG_FUNC_DESC_FINISHSECTION, 0, {NULL}, 0, 0},

	{{0, 0}, FUNC_FORMAT, "Format", MSG_FUNC_DESC_FORMAT, 0, {function_disk}, "DRIVE/F", 0},

	{{0, 0}, FUNC_CLEARBUFFERS, "FreeCaches", MSG_FUNC_DESC_CLEARBUFFERS, 0, {function_clearbuffers}, 0, 0},

	{{0, 0}, FUNC_BYTE, "GetSizes", MSG_FUNC_DESC_GETSIZES, FUNCF_NEED_DIRS, {function_getsizes}, "FORCE/S", 0},

	{{0, 0},
	 FUNC_HEXREAD,
	 "HexRead",
	 MSG_FUNC_DESC_HEXREAD,
	 FUNCF_NEED_FILES | FUNCF_SCRIPT_OVERRIDE,
	 {function_show},
	 "NAME/F,WAIT/S",
	 "f0"},

	{{0, 0}, FUNC_HIDE, "Hide", MSG_FUNC_DESC_HIDE, 0, {function_special}, 0, 0},

	{{0, 0},
	 FUNC_ICONINFO,
	 "IconInfo",
	 MSG_FUNC_DESC_ICONINFO,
	 FUNCF_NEED_ENTRIES | FUNCF_SCRIPT_OVERRIDE,
	 {function_show},
	 "NAME/F,WAIT/S",
	 "f0"},

	{{0, 0}, FUNC_LEAVEOUT, "LeaveOut", MSG_FUNC_DESC_LEAVEOUT, FUNCF_NEED_ENTRIES, {function_leaveout}, "NAME/F", "f0"},

	{{0, 0},
	 FUNC_LOADBUTTONS,
	 "LoadButtons",
	 MSG_FUNC_DESC_LOADBUTTONS,
	 FUNCF_NEED_ENTRIES,
	 {function_loadbuttons},
	 "NAME,START/S,LABEL/K,IMAGE/K,UNDERMOUSE/S,TOGGLE/S,HIDDEN/S,SHOW/S",
	 "f0"},

	{{0, 0},
	 FUNC_LOADENVIRONMENT,
	 "LoadEnvironment",
	 MSG_FUNC_DESC_LOADENVIRONMENT,
	 FUNCF_NEED_ENTRIES,
	 {function_loadenvironment},
	 "NAME/F",
	 "f0"},

	{{0, 0}, FUNC_LOADFONTS, "LoadFonts", 0, FUNCF_NEED_ENTRIES, {function_loadfonts}, "NAME/F", "f0"},

	{{0, 0},
	 FUNC_MAKEDIR,
	 "MakeDir",
	 MSG_FUNC_DESC_MAKEDIR,
	 FUNCF_NEED_SOURCE | FUNCF_SINGLE_SOURCE,
	 {function_makedir},
	 "NAME,NOICON/S,SELECT/S,NEW/S,READ/S",
	 "S0"},

	{{0, 0},
	 FUNC_MAKEFILE,
	 "MakeFile",
	 MSG_FUNC_DESC_MAKEFILE,
	 FUNCF_NEED_SOURCE | FUNCF_SINGLE_SOURCE,
	 {function_makefile},
	 "NAME,NOICON/S,SELECT/S",
	 "S0"},

	{{0, 0},
	 FUNC_MAKELINK,
	 "MakeLink",
	 MSG_FUNC_DESC_MAKELINK,
	 FUNCF_ASK_PATH | FUNCF_NEED_ENTRIES | FUNCF_CAN_DO_ICONS | FUNCF_NEED_DEST | FUNCF_ASK_FILTER,
	 {function_copy},
	 "NAME,TO,QUIET/S",
	 "f0d1"},

	{{0, 0},
	 FUNC_MAKELINKAS,
	 "MakeLinkAs",
	 MSG_FUNC_DESC_MAKELINKAS,
	 FUNCF_ASK_PATH | FUNCF_NEED_ENTRIES | FUNCF_CAN_DO_ICONS | FUNCF_NEED_DEST | FUNCF_ASK_FILTER,
	 {function_copy},
	 "NAME,NEWNAME,TO,QUIET/S",
	 "f0a1d2"},

	{{0, 0},
	 FUNC_MOVE,
	 "Move",
	 MSG_FUNC_DESC_MOVE,
	 FUNCF_ASK_PATH | FUNCF_NEED_ENTRIES | FUNCF_CAN_DO_ICONS | FUNCF_NEED_DEST | FUNCF_SINGLE_DEST | FUNCF_ASK_FILTER,
	 {function_copy},
	 "NAME,TO,QUIET/S",
	 "f0d1"},

	{{0, 0},
	 FUNC_MOVEAS,
	 "MoveAs",
	 MSG_FUNC_DESC_MOVEAS,
	 FUNCF_ASK_PATH | FUNCF_NEED_ENTRIES | FUNCF_CAN_DO_ICONS | FUNCF_NEED_DEST | FUNCF_SINGLE_DEST | FUNCF_ASK_FILTER,
	 {function_copy},
	 "NAME,NEWNAME,TO,QUIET/S",
	 "f0a1d2"},

	{{0, 0}, FUNC_NONE, "None", MSG_FUNC_DESC_NONE, FUNCF_NEED_SOURCE, {function_none}, 0, 0},

	{{0, 0},
	 FUNC_PARENT,
	 "Parent",
	 MSG_FUNC_DESC_PARENT,
	 FUNCF_NEED_SOURCE | FUNCF_SINGLE_SOURCE,
	 {function_parent},
	 0,
	 0},

	{{0, 0},
	 FUNC_PLAY,
	 "Play",
	 MSG_FUNC_DESC_PLAY,
	 FUNCF_NEED_FILES | FUNCF_SCRIPT_OVERRIDE,
	 {function_show},
	 "NAME,WAIT=SYNC/S,QUIET/S,ICON/S,VOLUME=VOL/K/N",
	 "f0"},

	{{0, 0},
	 FUNC_PRINT,
	 "Print",
	 MSG_FUNC_DESC_PRINT,
	 FUNCF_NEED_FILES | FUNCF_SCRIPT_OVERRIDE,
	 {function_show},
	 "NAME/F,WAIT/S",
	 "f0"},

	{{0, 0},
	 FUNC_PRINTDIR,
	 "PrintDir",
	 MSG_FUNC_DESC_PRINTDIR,
	 FUNCF_NEED_SOURCE | FUNCF_SINGLE_SOURCE,
	 {function_printdir},
	 0,
	 0},

	{{0, 0},
	 FUNC_PROTECT,
	 "Protect",
	 MSG_FUNC_DESC_PROTECT,
	 FUNCF_NEED_ENTRIES,
	 {function_change},
	 "NAME,RECURSE/S,SET=+/K,CLEAR=-/K",
	 "f0"},

	{{0, 0}, FUNC_QUIT, "Quit", MSG_FUNC_DESC_QUIT, 0, {function_special}, "FORCE/S", 0},

	{{0, 0},
	 FUNC_READ,
	 "Read",
	 MSG_FUNC_DESC_READ,
	 FUNCF_NEED_FILES | FUNCF_SCRIPT_OVERRIDE,
	 {function_show},
	 "NAME/F,WAIT/S",
	 "f0"},

	{{0, 0}, FUNC_REM, "Rem", 0, FUNCF_PRIVATE, {NULL}, 0, 0},

	{{0, 0},
	 FUNC_RENAME,
	 "Rename",
	 MSG_FUNC_DESC_RENAME,
	 FUNCF_NEED_ENTRIES | FUNCF_CAN_DO_ICONS,
	 {function_rename},
	 "NAME,NEWNAME",
	 "F0a1"},

	{{0, 0}, FUNC_RESELECT, "Reselect", MSG_FUNC_DESC_RESELECT, FUNCF_NEED_SOURCE, {function_reselect}, 0, 0},

	{{0, 0}, FUNC_RESET, "Reset", MSG_FUNC_DESC_RESET, FUNCF_PRIVATE, {function_reset}, "MODULES/S,SYSTEM/S", 0},

	{{0, 0}, FUNC_REVEAL, "Reveal", MSG_FUNC_DESC_REVEAL, 0, {function_special}, 0, 0},

	{{0, 0}, FUNC_ROOT, "Root", MSG_FUNC_DESC_ROOT, FUNCF_NEED_SOURCE | FUNCF_SINGLE_SOURCE, {function_parent}, 0, 0},

	{{0, 0}, FUNC_RUN, "Run", MSG_FUNC_DESC_RUN, FUNCF_NEED_FILES, {function_runprog}, "NAME/F", "f0"},

	{{0, 0}, FUNC_RUNCOMMAND, "RunCommand", 0, FUNCF_NEED_FILES | FUNCF_PRIVATE, {function_runcommand}, "NAME/F", "f0"},

	{{0, 0},
	 FUNC_SCANDIR,
	 "ScanDir",
	 MSG_FUNC_DESC_SCANDIR,
	 FUNCF_WANT_SOURCE | FUNCF_WANT_DEST | FUNCF_SINGLE_SOURCE | FUNCF_WANT_ENTRIES | FUNCF_NEED_DIRS,
	 {function_scandir},
	 "PATH,NEW/S,MODE/K,SHOWALL/S,CONTAINER/S",
	 "a0n1"},

	{{0, 0}, FUNC_SEARCH, "Search", MSG_FUNC_DESC_SEARCH, FUNCF_NEED_ENTRIES, {function_search}, 0, 0},

	{{0, 0},
	 FUNC_SELECT,
	 "Select",
	 MSG_FUNC_DESC_SELECT,
	 FUNCF_NEED_SOURCE | FUNCF_WANT_DEST,
	 {function_select},
	 "NAME,FROM/K,TO/K,BITSON/K,BITSOFF/K,COMPARE/K,MATCHNAME/S,NOMATCHNAME/S,IGNORENAME/S,"
	 "MATCHDATE/S,NOMATCHDATE/S,IGNOREDATE/S,MATCHBITS/S,NOMATCHBITS/S,IGNOREBITS/S,"
	 "MATCHCOMPARE/S,NOMATCHCOMPARE/S,IGNORECOMPARE/S,"
	 "BOTH/S,FILESONLY/S,DIRSONLY/S,EXCLUDE/S,INCLUDE/S",
	 0},

	{{0, 0}, FUNC_SET, "Set", MSG_FUNC_DESC_SET, FUNCF_WANT_SOURCE, {function_set}, 0, 0},

	{{0, 0},
	 FUNC_SET_BACKGROUND,
	 "SetAsBackground",
	 MSG_FUNC_DESC_SETBACKGROUND,
	 FUNCF_NEED_FILES,
	 {function_setbackground},
	 "NAME,DESKTOP/S,LISTER=WINDOW/S,REQ=REQUESTER/S,TILE/S,CENTER=CENTRE/S,STRETCH/S,PRECISION/K,BORDER/K",
	 "f0"},

	{{0, 0},
	 FUNC_SHOW,
	 "Show",
	 MSG_FUNC_DESC_SHOW,
	 FUNCF_NEED_FILES | FUNCF_SCRIPT_OVERRIDE,
	 {function_show},
	 "NAME/F,WAIT/S",
	 "f0"},

	{{0, 0},
	 FUNC_SMARTREAD,
	 "SmartRead",
	 MSG_FUNC_DESC_SMARTREAD,
	 FUNCF_NEED_FILES | FUNCF_SCRIPT_OVERRIDE,
	 {function_show},
	 "NAME/F,WAIT/S",
	 "f0"},

	{{0, 0}, FUNC_STOPSNIFF, "StopSniffer", MSG_FUNC_DESC_STOPSNIFFER, FUNCF_NEED_SOURCE, {function_stopsniffer}, 0, 0},

	{{0, 0}, FUNC_TOGGLE, "Toggle", MSG_FUNC_DESC_TOGGLE, FUNCF_NEED_SOURCE, {function_toggle}, 0, 0},

	{{0, 0},
	 FUNC_USER1,
	 "User1",
	 MSG_FUNC_DESC_USER,
	 FUNCF_NEED_ENTRIES | FUNCF_WANT_DEST,
	 {function_user},
	 "NAME/F",
	 "f0"},

	{{0, 0},
	 FUNC_USER2,
	 "User2",
	 MSG_FUNC_DESC_USER,
	 FUNCF_NEED_ENTRIES | FUNCF_WANT_DEST,
	 {function_user},
	 "NAME/F",
	 "f0"},

	{{0, 0},
	 FUNC_USER3,
	 "User3",
	 MSG_FUNC_DESC_USER,
	 FUNCF_NEED_ENTRIES | FUNCF_WANT_DEST,
	 {function_user},
	 "NAME/F",
	 "f0"},

	{{0, 0},
	 FUNC_USER4,
	 "User4",
	 MSG_FUNC_DESC_USER,
	 FUNCF_NEED_ENTRIES | FUNCF_WANT_DEST,
	 {function_user},
	 "NAME/F",
	 "f0"},

	{{0, 0},
	 FUNC_USER,
	 "User",
	 MSG_FUNC_DESC_USER,
	 FUNCF_NEED_ENTRIES | FUNCF_WANT_DEST,
	 {function_user},
	 "ID/N,NAME/F",
	 "f1"},

	{{0}}};
