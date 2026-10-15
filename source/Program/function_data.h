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

#ifndef _DOPUS_FUNCTION_DATA
#define _DOPUS_FUNCTION_DATA

#define FUNCKEY_FILE 'f'
#define FUNCKEY_FILENO 'F'
#define FUNCKEY_SOURCE 'S'
#define FUNCKEY_DIR 'd'
#define FUNCKEY_ARG 'a'
#define FUNCKEY_NEW 'n'

// Internal command IDs
enum {
	FUNC_BASE = 0x1000,

	FUNC_ADDICON,
	FUNC_ALL,
	FUNC_BUFFERLIST,
	FUNC_CHECKFIT,
	FUNC_CLEARBUFFERS,
	FUNC_UNBYTE,
	FUNC_CLONE,
	FUNC_COMMENT,
	FUNC_COPY,
	FUNC_COPYAS,
	FUNC_DATESTAMP,
	FUNC_DELETE,
	FUNC_DEVICELIST,
	FUNC_DIRTREE,
	FUNC_DISKCOPY,
	FUNC_DISKINFO,
	FUNC_ENCRYPT,
	FUNC_HUNT,
	FUNC_ENDFUNCTION,
	FUNC_FORMAT,
	FUNC_BYTE,
	FUNC_LEAVEOUT,
	FUNC_LOADBUTTONS,
	FUNC_MAKEDIR,
	FUNC_MOVE,
	FUNC_MOVEAS,
	FUNC_NONE,
	FUNC_PLAY,
	FUNC_PRINT,
	FUNC_oldPRINTDIR,
	FUNC_PROTECT,
	FUNC_RENAME,
	FUNC_RESELECT,
	FUNC_RUN,
	FUNC_SCANDIR,
	FUNC_SEARCH,
	FUNC_SELECT,
	FUNC_SHOW,
	FUNC_TOGGLE,
	FUNC_READ,
	FUNC_ANSIREAD,
	FUNC_HEXREAD,
	FUNC_SMARTREAD,
	FUNC_LOADENVIRONMENT,
	FUNC_LOADSETTINGS,
	FUNC_PARENT,
	FUNC_ROOT,
	FUNC_USER1,
	FUNC_USER2,
	FUNC_USER3,
	FUNC_USER4,
	FUNC_DOUBLECLICK,
	FUNC_DRAGNDROP,
	FUNC_VERIFY,
	FUNC_CLI,
	FUNC_PRINTDIR,
	FUNC_BEEP,

	FUNC_REREAD,
	FUNC_ABORT,
	FUNC_QUIT,

	FUNC_SET,
	FUNC_CLOSEBUTTONS,

	FUNC_HIDE,
	FUNC_REVEAL,
	FUNC_PLAY_QUIET,
	FUNC_REM,
	FUNC_MAKELINK,
	FUNC_MAKELINKAS,
	FUNC_EDITFILETYPE,
	FUNC_TEST,
	FUNC_ASSIGN,
	FUNC_DEFFTYPE,
	FUNC_CONFIGURE,
	FUNC_DELETEFILE,
	FUNC_PLAY_ICON,
	FUNC_SET_BACKGROUND,
	FUNC_RUNCOMMAND,
	FUNC_EDITCOMMAND,
	FUNC_USER,
	FUNC_ICONINFO,
	FUNC_RESET,
	FUNC_LOADFONTS,
	FUNC_STOPSNIFF,
	FUNC_MAKEFILE,
};

#ifndef __amigaos3__
	#pragma pack(2)
#endif

struct _InstructionParsed;
struct _FunctionHandle;

// Command entry (looks like a (struct Node) as far as ln_Name goes)
typedef struct _CommandList
{
	struct MinNode node;
	UWORD function;	 // Function ID
	char *name;		 // Function name
	IPTR desc;		 // Function description
	ULONG flags;	 // Function flags

	// Function code
	union
	{
		int (*code)(struct _CommandList *, struct _FunctionHandle *, char *args, struct _InstructionParsed *);
		char *module_name;
	} stuff;

	char *template;
	char *template_key;

	char *help_name;
} CommandList;

#ifndef __amigaos3__
	#pragma pack()
#endif

extern CommandList commandlist_internal[];

// Internal function flags
#define FUNCFLAGS_DIRS (1 << 0)			   // Operates on directories
#define FUNCFLAGS_DEVS (1 << 1)			   // Operates on devices
#define FUNCFLAGS_FILES (1 << 2)		   // Operates on files
#define FUNCFLAGS_ANYTHING (1 << 3)		   // Operates on files or directories
#define FUNCFLAGS_COPYISCLONE (1 << 8)	   // COPY is actually a CLONE
#define FUNCFLAGS_BYTEISCHECKFIT (1 << 8)  // BYTE is actually a CHECKFIT

// Curly-bracket sequences
#define FUNC_END_ARG 1
#define FUNC_VARIABLE 2
#define FUNC_REQUESTER 3
#define FUNC_QUERY_INFO 4
#define FUNC_LAST_FILE 5
#define FUNC_LAST_PATH 6
#define FUNC_ONE_FILE 14
#define FUNC_ALL_FILES 15
#define FUNC_ONE_PATH 16
#define FUNC_ALL_PATHS 17
#define FUNC_SOURCE 18
#define FUNC_DEST 19
#define FUNC_STRIP_SUFFIX 20
#define FUNC_QUOTES 21
#define FUNC_NO_QUOTES 22
#define FUNC_NORMAL 23
#define FUNC_DEST_LAST_FILE 24
#define FUNC_DEST_LAST_PATH 25
#define FUNC_DEST_ONE_FILE 26
#define FUNC_DEST_ALL_FILES 27
#define FUNC_DEST_ONE_PATH 28
#define FUNC_DEST_ALL_PATHS 29
#define FUNC_ONE_FILE_NO_UNSELECT 30
#define FUNC_ONE_PATH_NO_UNSELECT 31

// Function types
enum {
	FT_INTERNAL,	// Internal
	FT_EXECUTABLE,	// AmigaDOS
	FT_WORKBENCH,	// Workbench
	FT_BATCH,		// Batch
	FT_AREXX,		// ARexx
	FT_REQUESTER	// Requester
};

// Function type characters
#define FC_INTERNAL '*'
#define FC_WORKBENCH '%'
#define FC_BATCH '$'
#define FC_AREXX '&'
#define FC_REQUESTER '@'

// ARexx Status values
enum {
	RXSTATUS_CURRENT_DIRECTORY = 1,
	RXSTATUS_VERSION,
	RXSTATUS_ACTIVE_WINDOW,
	RXSTATUS_NUMBER_OF_FILES,
	RXSTATUS_NUMBER_OF_DIRS,
	RXSTATUS_NUMBER_OF_ENTRIES,
	RXSTATUS_NUMBER_SELECTED_FILES,
	RXSTATUS_NUMBER_SELECTED_DIRS,
	RXSTATUS_NUMBER_SELECTED_ENTRIES,
	RXSTATUS_TOTAL_BYTES,
	RXSTATUS_TOTAL_SELECTED_BYTES,
	RXSTATUS_SELECT_PATTERN,
	RXSTATUS_DIRECTORY_NAME,
	RXSTATUS_DISK_NAME,
	RXSTATUS_DISK_FREE_BYTES,
	RXSTATUS_DISK_TOTAL_BYTES,
	RXSTATUS_BUFFERED_DIRECTORY_NAME,
	RXSTATUS_BUFFERED_DISKNAME,
	RXSTATUS_BUFFERED_DISK_FREE_BYTES,
	RXSTATUS_BUFFERED_DISK_TOTAL_BYTES,
	RXSTATUS_BUFFER_DISPLAYED,
	RXSTATUS_DISPLAY_OFFSET,
	RXSTATUS_ENTRIES_PER_PAGE,
	RXSTATUS_CONFIGURATION_CHANGED,
	RXSTATUS_OKAY_STRING,
	RXSTATUS_CANCEL_STRING,
	RXSTATUS_ICONIFIED,
	RXSTATUS_TOP_TEXT_JUSTIFY,
	RXSTATUS_CONFIGURATION_ADDRESS,
	RXSTATUS_FIRST_NODE,
	RXSTATUS_CURRENT_BANK_NUMBER
};

// Copy return codes
#define COPY_ABORTED -1	 // Aborted
#define COPY_FAILED 0	 // Failed
#define COPY_OK 1		 // Ok

#ifndef __amigaos3__
	#pragma pack(2)
#endif

// Keeps track of what's in the modules directory
typedef struct _ModuleNode
{
	struct Node node;
	struct DateStamp date;
	ULONG size;
	short flags;
} ModuleNode;

#ifndef __amigaos3__
	#pragma pack()
#endif

#define MNF_TEMP (1 << 0)
#define MNF_CACHE (1 << 1)	// Commands known, can go in the module cache

#endif
