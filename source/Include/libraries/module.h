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

#ifndef _DOPUS_MODULES
#define _DOPUS_MODULES

#ifndef __amigaos3__
	#pragma pack(2)
#endif
typedef struct
{
	ULONG id;		 // Function ID code
	char *name;		 // Function name
	ULONG desc;		 // Locale string ID for function description
	ULONG flags;	 // Function flags
	char *template;	 // Command template
} ModuleFunction;

// Function flags
#define FUNCF_NEED_SOURCE (1 << 0)	// Needs a source directory
#define FUNCF_NEED_DEST (1 << 1)	// Needs a destination directory
#define FUNCF_NEED_FILES (1 << 2)	// Needs some files to work with
#define FUNCF_NEED_DIRS (1 << 3)	// Needs some files to work with
#define FUNCF_NEED_ENTRIES (FUNCF_NEED_FILES | FUNCF_NEED_DIRS)
#define FUNCF_CAN_DO_ICONS (1 << 6)		// Function can do icons
#define FUNCF_SINGLE_SOURCE (1 << 8)	// Only a single source needed
#define FUNCF_SINGLE_DEST (1 << 9)		// Only a single destination needed
#define FUNCF_NO_LOOP_SOURCE (1 << 10)	// Don't loop through sources
#define FUNCF_WANT_DEST (1 << 11)		// Want destinations, don't need them
#define FUNCF_WANT_SOURCE (1 << 12)		// Want source, don't need it
#define FUNCF_CREATE_SOURCE (1 << 13)	// Can create our own source

#define FUNCF_WANT_ENTRIES (1 << 19)  // Want entries
#define FUNCF_NO_ARGS (1 << 20)		  // Don't pass arguments
#define FUNCF_NO_BUSY (1 << 21)		  // Don't send lister busy

typedef struct
{
	ULONG ver;					 // Module version
	char *name;					 // Module name
	char *locale_name;			 // Catalog name
	ULONG flags;				 // Module flags
	ULONG function_count;		 // Number of functions in module
	ModuleFunction function[1];	 // Definition of first function
} ModuleInfo;
#ifndef __amigaos3__
	#pragma pack()
#endif

#define MODULEF_CALL_STARTUP (1 << 0)	  // Call ModuleEntry() on startup
#define MODULEF_STARTUP_SYNC (1 << 1)	  // Run Synchronously on startup
#define MODULEF_CATALOG_VERSION (1 << 2)  // Use version for catalog
#define MODULEF_HELP_AVAILABLE (1 << 3)	  // Help is available

#define FUNCID_STARTUP 0xffffffff

// Callback commands
enum {
	EXTCMD_GET_SOURCE,		   // Get current source path
	EXTCMD_NEXT_SOURCE,		   // Get next source path
	EXTCMD_UNLOCK_SOURCE,	   // Unlock source paths
	EXTCMD_GET_ENTRY,		   // Get entry
	EXTCMD_END_ENTRY,		   // End entry
	EXTCMD_RELOAD_ENTRY,	   // Reload entry
	EXTCMD_OPEN_PROGRESS,	   // Open progress indicator
	EXTCMD_UPDATE_PROGRESS,	   // Update progress indicator
	EXTCMD_CLOSE_PROGRESS,	   // Close progress indicator
	EXTCMD_CHECK_ABORT,		   // Check abort status
	EXTCMD_ENTRY_COUNT,		   // Get entry count
	EXTCMD_GET_WINDOW,		   // Get window handle
	EXTCMD_GET_DEST,		   // Get next destination
	EXTCMD_END_SOURCE,		   // Cleanup current source path
	EXTCMD_END_DEST,		   // Cleanup current destination path
	EXTCMD_ADD_CHANGE,		   // Add a change to a lister
	EXTCMD_ADD_FILE,		   // Add a file to a lister
	EXTCMD_GET_HELP,		   // Get help on a topic
	EXTCMD_GET_PORT,		   // Get ARexx port name
	EXTCMD_GET_SCREEN,		   // Get public screen name
	EXTCMD_REPLACE_REQ,		   // Show exists/replace? requester
	EXTCMD_REMOVE_ENTRY,	   // Mark an entry for removal
	EXTCMD_GET_SCREENDATA,	   // Get DOpus screen data
	EXTCMD_FREE_SCREENDATA,	   // Free screen data      23
	EXTCMD_CREATE_FILE_ENTRY,  // Create a file entry   24
	EXTCMD_FREE_FILE_ENTRY,	   // Free a file entry     25
	EXTCMD_SORT_FILELIST,	   // Sort an entire list of file entries    26
	EXTCMD_NEW_LISTER,		   // Open a new lister     27
	EXTCMD_GET_POINTER,		   // Get a pointer         28
	EXTCMD_FREE_POINTER,	   // Free a pointer        29
	EXTCMD_SEND_COMMAND,	   // Send a command to DOpus
	EXTCMD_DEL_FILE,		   // Delete a file from a lister
	EXTCMD_DO_CHANGES,		   // Perform changes
	EXTCMD_LOAD_FILE,		   // Load files to listers
};

#ifndef __amigaos3__
	#pragma pack(2)
#endif
struct progress_packet
{
	struct _PathNode *path;
	char *name;
	ULONG count;
};

struct endentry_packet
{
	struct _FunctionEntry *entry;
	BOOL deselect;
};

struct addchange_packet
{
	struct _PathNode *path;
	UWORD change;
	APTR data;
};

struct addfile_packet
{
	char *path;
	struct FileInfoBlock *fib;
	struct ListerWindow *lister;
};

struct delfile_packet
{
	char *path;
	char *name;
	struct ListerWindow *lister;
};

struct loadfile_packet
{
	char *path;
	char *name;
	short flags;
	short reload;
};

struct sortlist_packet
{
	struct _PathNode *path;
	struct List *list;
	long file_count;
	long dir_count;
};

struct replacereq_packet
{
	struct Window *window;
	struct Screen *screen;
	IPCData *ipc;
	struct FileInfoBlock *file1;
	struct FileInfoBlock *file2;
	short default_option;
};

struct DOpusScreenData
{
	struct Screen *screen;
	struct DrawInfo *draw_info;
	unsigned short depth;
	unsigned short pen_alloc;
	unsigned short pen_array[16];
	short pen_count;
	// User pens 9-16 of a palette-box pen table (system pens, top four OS
	// pens, then the user pens); appended for CONFIG_VERSION_19. get_screen_data
	// only fills pen_array, where all sixteen user pens fit.
	unsigned short pen_array2[8];
};
#ifndef __amigaos3__
	#pragma pack()
#endif

#if !defined(__amigaos4__) && !defined(__MORPHOS__)
struct _FunctionHandle;
IPTR ASM SAVEDS function_external_hook(REG(d0, ULONG command),
										REG(a0, struct _FunctionHandle *handle),
										REG(a1, APTR packet));
#endif

#define EXT_FUNC(name) IPTR ASM (*name)(REG(d0, ULONG), REG(a0, APTR), REG(a1, APTR))
#define TYPE_EXT(var) (IPTR (*)()) var

#define IDCMP_FUNC(name) unsigned long ASM (*name)(REG(d0, ULONG), REG(a0, struct IntuiMessage *))

#define SHOWF_SELECTED (1 << 0)
#define SHOWF_DELETE (1 << 1)

enum {
	MODPTR_OPTIONS,
	MODPTR_FILETYPES,
	MODPTR_HANDLE,
	MODPTR_DEFFORMAT,
	MODPTR_COMMANDS,
	MODPTR_SCRIPTS,
};

#define POINTERF_COPY (1 << 0)
#define POINTERF_LOCKED (1 << 16)

#ifndef __amigaos3__
	#pragma pack(2)
#endif
struct pointer_packet
{
	ULONG type;
	APTR pointer;
	ULONG flags;
};

struct command_packet
{
	char *command;
	ULONG flags;
	char *result;
	ULONG rc;
};
#ifndef __amigaos3__
	#pragma pack()
#endif

#define COMMANDF_RESULT (1 << 0)
#define COMMANDF_RUN_SCRIPT (1 << 1)

#ifndef DEF_DOPUSCOMMANDLIST
	#define DEF_DOPUSCOMMANDLIST

	#ifndef __amigaos3__
		#pragma pack(2)
	#endif
struct DOpusCommandList
{
	struct Node dcl_Node;
	ULONG dcl_Flags;
	char *dcl_Description;
	char *dcl_Template;
	char *dcl_Module;
	char *dcl_Help;
};
	#ifndef __amigaos3__
		#pragma pack()
	#endif

#endif /* DEF_DOPUSCOMMANDLIST */

/***************************************************************************/

#ifndef __amigaos3__
	#pragma pack(2)
#endif
struct LibraryHeader
{
	struct Library libBase;
	UWORD initialized;
	struct Library *sysBase;
	BPTR segList;
	struct SignalSemaphore libSem;
};
#ifndef __amigaos3__
	#pragma pack()
#endif

/***************************************************************************/

#if defined(__MORPHOS__)
	#define __TEXTSEGMENT__ __attribute__((section(".text")))
#else
	#define __TEXTSEGMENT__
#endif

/***************************************************************************/

#endif
