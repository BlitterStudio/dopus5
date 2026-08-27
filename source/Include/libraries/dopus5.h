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
/*
 *
 * dopus5.library header file
 * mix of dopus_config.h , dopusbase.h, read_ilbm.h and iffp.h
 *
 */

#ifndef _DOPUS5_STRUCTS_H_
#define _DOPUS5_STRUCTS_H_

#ifndef PROTO_DOS_H
	#include <proto/dos.h>
#endif

#ifndef PROTO_GRAPHICS_H
	#include <proto/graphics.h>
#endif

#ifndef PROTO_INTUITION_H
	#include <proto/intuition.h>
#endif

#ifndef PROTO_WB_H
	#include <proto/wb.h>
#endif

#include <SDI/SDI_compiler.h>  // for cross-platform hooks

#ifndef __amigaos3__
	#pragma pack(2)
#endif

#define CONFIG_VERSION_3 2
#define CONFIG_VERSION_2 3
#define CONFIG_VERSION_4 4
#define CONFIG_VERSION_5 5
#define CONFIG_VERSION_6 6
#define CONFIG_VERSION_7 7
#define CONFIG_VERSION_8 8
#define CONFIG_VERSION_9 9
#define CONFIG_VERSION_10 10
#define CONFIG_VERSION_11 11
#define CONFIG_VERSION_12 12
#define CONFIG_VERSION_13 13
#define CONFIG_VERSION_14 14
#define CONFIG_VERSION_15 15
#define CONFIG_VERSION_16 16
#define CONFIG_VERSION_17 17

#define SORT_REVERSE 1

#define LFORMATF_REJECT_ICONS (1 << 0)	// Reject icons
#define LFORMATF_HIDDEN_BIT (1 << 1)	// Respect the H bit
#define LFORMATF_ICON_VIEW (1 << 2)		// Default to icon view
#define LFORMATF_SHOW_ALL (1 << 3)		// Show all
#define LFORMATF_TEMP_ICONS (1 << 4)
#define LFORMATF_GAUGE (1 << 5)	   // Free space gauge
#define LFORMATF_INHERIT (1 << 6)  // Inherit format

enum {
	DISPLAY_NAME,
	DISPLAY_SIZE,
	DISPLAY_PROTECT,
	DISPLAY_DATE,
	DISPLAY_COMMENT,
	DISPLAY_FILETYPE,
	DISPLAY_OWNER,
	DISPLAY_GROUP,
	DISPLAY_NETPROT,
	DISPLAY_VERSION,

	DISPLAY_LAST
};

enum {
	SEPARATE_MIX,
	SEPARATE_DIRSFIRST,
	SEPARATE_FILESFIRST,
};

#define SHOWFREE_BYTES (1 << 0)
#define SHOWFREE_KILO (1 << 1)
#define SHOWFREE_BLOCKS (1 << 2)
#define SHOWFREE_PERCENT (1 << 3)

enum {
	ITEM_FILENAME,
	ITEM_FILESIZE,
	ITEM_PROTECTION,
	ITEM_DATE,
	ITEM_COMMENT,
	ITEM_FILETYPE,
	ITEM_OWNER,
	ITEM_GROUP,
	ITEM_NETPROT,
	ITEM_VERSION,
};

/****************************************************************************
						  IFF Configuration Storage
 ****************************************************************************/

// Opus FORM ids.  Both identify a Directory Opus 5 IFF config file; the
// difference is which on-disk struct schema is used:
//   ID_OPUS  = legacy schema (CFG_ENVR/CFG_BTNW/CFG_LSTR pre-extension).
//              Readers in config_open.c run a convert_*() migration to
//              the current in-memory layout.  Settings, filetype lists,
//              position-info, FTP address books, exported function files
//              and clipboard copy/paste still write this id.
//   ID_EPUS  = current schema (extended structs read directly via
//              L_IFFReadChunkBytes).  Originally introduced for the
//              environment file (hence the historical "E"); now also
//              written by button banks (Buttons / Hotkeys / Start Menu)
//              and the configopus environment converter.
// File-type recognition rules that want to identify any DOpus 5 config
// file should match FORM == OPUS *or* FORM == EPUS.
#define ID_EPUS MAKE_ID('E', 'P', 'U', 'S')	 // Opus FORM (current schema)
#define ID_OPUS MAKE_ID('O', 'P', 'U', 'S')	 // Opus FORM (legacy schema, migrated on read)
#define ID_BTBK MAKE_ID('B', 'T', 'B', 'K')	 // Button bank to open
#define ID_BANK MAKE_ID('B', 'A', 'N', 'K')	 // Button bank to open
#define ID_BTNW MAKE_ID('B', 'T', 'N', 'W')	 // Button window
#define ID_FUNC MAKE_ID('F', 'U', 'N', 'C')	 // Function definition
#define ID_BUTN MAKE_ID('B', 'U', 'T', 'N')	 // Button definition
#define ID_TYPE MAKE_ID('T', 'Y', 'P', 'E')	 // Filetype definition
#define ID_HKEY MAKE_ID('H', 'K', 'E', 'Y')	 // Hotkey definition
#define ID_SETS MAKE_ID('S', 'E', 'T', 'S')	 // Settings
#define ID_LSTR MAKE_ID('L', 'S', 'T', 'R')	 // Lister definition
#define ID_DLST MAKE_ID('D', 'L', 'S', 'T')	 // Lister definition
#define ID_LBUT MAKE_ID('L', 'B', 'U', 'T')	 // Lister buttons
#define ID_ENVR MAKE_ID('E', 'N', 'V', 'R')	 // Environment definition
#define ID_TBAR MAKE_ID('T', 'B', 'A', 'R')	 // Toolbar pathname
#define ID_LMEN MAKE_ID('L', 'M', 'E', 'N')	 // Lister menu pathname
#define ID_UMEN MAKE_ID('U', 'M', 'E', 'N')	 // User menu pathname
#define ID_SCRP MAKE_ID('S', 'C', 'R', 'P')	 // Scripts pathname
#define ID_DESK MAKE_ID('D', 'E', 'S', 'K')	 // Desktop
#define ID_IPOS MAKE_ID('I', 'P', 'O', 'S')	 // Icon positioning
#define ID_STRT MAKE_ID('S', 'T', 'R', 'T')	 // Start Menu
#define ID_PATH MAKE_ID('P', 'A', 'T', 'H')	 // Path list
#define ID_BPIC MAKE_ID('B', 'P', 'I', 'C')	 // Background picture
#define ID_SNDX MAKE_ID('S', 'N', 'D', 'X')	 // Sound entry

// FUNC - Function definition
typedef struct
{
	ULONG flags;	  // Function flags
	ULONG flags2;	  // More function flags
	ULONG pad_1;	  // Some padding
	UWORD code;		  // Function key code
	UWORD qual;		  // Function qualifier
	UWORD func_type;  // Function type
	UWORD qual_mask;  // Qualifier mask
	UWORD qual_same;  // Qualifier same

	//	Followed by function string
} CFG_FUNC;

#define FUNCF_OUTPUT_WINDOW (1 << 0)	 // Output window
#define FUNCF_OUTPUT_FILE (1 << 1)		 // Output to file
#define FUNCF_WORKBENCH_OUTPUT (1 << 2)	 // Open window on Workbench
#define FUNCF_RUN_ASYNC (1 << 4)		 // Run asynchronously
#define FUNCF_CD_SOURCE (1 << 5)		 // CD source
#define FUNCF_CD_DESTINATION (1 << 6)	 // CD destination
#define FUNCF_DO_ALL_FILES (1 << 7)		 // Repeat for all files
#define FUNCF_RECURSE_DIRS (1 << 8)		 // Recursive directories
#define FUNCF_RELOAD_FILES (1 << 9)		 // Reload each file when done
#define FUNCF_NO_QUOTES (1 << 11)		 // Don't quote filenames
#define FUNCF_RESCAN_SOURCE (1 << 12)	 // Rescan source directory
#define FUNCF_RESCAN_DEST (1 << 13)		 // Rescan destination directory
#define FUNCF_WAIT_CLOSE (1 << 16)		 // Wait for click before closing window

#define FUNCF_ORIGINAL_FLAGS                                                                                 \
	(FUNCF_OUTPUT_WINDOW | FUNCF_OUTPUT_FILE | FUNCF_WORKBENCH_OUTPUT | FUNCF_RUN_ASYNC | FUNCF_CD_SOURCE |  \
	 FUNCF_CD_DESTINATION | FUNCF_DO_ALL_FILES | FUNCF_RECURSE_DIRS | FUNCF_RELOAD_FILES | FUNCF_NO_QUOTES | \
	 FUNCF_RESCAN_SOURCE | FUNCF_RESCAN_DEST | FUNCF_WAIT_CLOSE)

#define FUNCF2_HOTKEY_FUNC (1 << 0)	   // Function is a hotkey function
#define FUNCF2_FILETYPE_FUNC (1 << 1)  // Function is a filetype function
#define FUNCF2_VALID_IX (1 << 2)	   // Valid IX information (qual mask/same)
#define FUNCF2_LABEL_FUNC (1 << 3)	   // Function has a label associated with it

// These flags are used internally by Opus and not stored
#define FUNCF2_ORIGINAL (1 << 30)		// Call original function
#define FUNCF2_FREE_FUNCTION (1 << 31)	// Free after use

#define FTYPE_NONE 0
#define FTYPE_LEFT_BUTTON 0
#define FTYPE_RIGHT_BUTTON 1
#define FTYPE_MID_BUTTON 2
#define FTYPE_LIST 100
#define FTYPE_SPECIAL_FLAG (1 << 14)

// BTNW - Button window
typedef struct
{
	char name[32];		 // Button window name
	struct IBox pos;	 // Window position 8
	char font_name[80];	 // Font to use
	char pad[3];
	UBYTE font_size;  // Font size
	UWORD columns;	  // Number of columns 8
	UWORD rows;		  // Number of rows
	ULONG flags;	  // Flags
} CFG_BTNW;

#define BTNWF_ICONIFIED (1 << 0)		 // Window iconified initially
#define BTNWF_GFX (1 << 1)				 // Buttons are graphical in nature
#define BTNWF_LISTER_BUTTONS (1 << 2)	 // Lister buttons
#define BTNWF_NO_BORDER (1 << 3)		 // No border
#define BTNWF_BORDERLESS (1 << 4)		 // Borderless buttons
#define BTNWF_SMART_REFRESH (1 << 5)	 // Smart refresh window
#define BTNWF_HORIZ (1 << 6)			 // Horizontal
#define BTNWF_VERT (1 << 7)				 // Vertical
#define BTNWF_TOOLBAR (1 << 8)			 // Toolbar bank
#define BTNWF_NO_DOGEARS (1 << 9)		 // No dog-ears
#define BTNWF_CHANGED (1 << 10)			 // Bank has changed
#define BTNWF_DRAG_RIGHT (1 << 11)		 // Drag bar on right side
#define BTNWF_NO_SCALE_IMAGES (1 << 12)	 // No image scaling
#define BTNWF_NONE (1 << 13)			 // No drag bar
#define BTNWF_AUTO_CLOSE (1 << 14)		 // Auto close bank
#define BTNWF_AUTO_ICONIFY (1 << 15)	 // Auto iconify bank
#define BTNWF_RIGHT_BELOW (1 << 16)		 // Border right or below
#define BTNWF_FIX_MENU (1 << 17)		 // Menus have been fixed
#define BTNWF_ACTIVE_POPUP (1 << 18)	 // Active popups

// BUTN - Button definition
typedef struct
{
	UBYTE fpen;	  // Text pen
	UBYTE bpen;	  // Background pen
	ULONG flags;  // Flags
	short count;  // Number of functions
	short pad_1;
	ULONG pad_2;

	// Followed by name/image filename(s)
} CFG_BUTN;

#define BUTNF_RIGHT_FUNC (1 << 0)  // Button has a right button function
#define BUTNF_MID_FUNC (1 << 1)	   // Button has a middle button function
#define BUTNF_GRAPHIC (1 << 2)	   // Button is a graphic button
#define BUTNF_NO_BORDER (1 << 3)   // No border for button
#define BUTNF_NEW_FORMAT (1 << 4)  // New format button
#define BUTNF_GLOBAL (1 << 5)	   // System-global
#define BUTNF_TITLE (1 << 6)	   // Menu title
#define BUTNF_ITEM (1 << 7)		   // Menu item
#define BUTNF_SUB (1 << 8)		   // Sub item

#define BUTNF_TEMP (1 << 31)  // Temporary flag

// STRT - StartMenu definition
typedef struct
{
	char label[40];	   // Label
	char image[256];   // Image
	ULONG flags;	   // Flags
	short fpen;		   // Pen
	short label_fpen;  // Label pen
	short sel_fpen;	   // Selected pen
	short pad1;
	ULONG pad[8];
	char label_fontname[80];  // Label font
	short label_fontsize;	  // Label font size
} CFG_STRT;

#define STRTF_VERSION2 (1 << 8)

// TYPE - Filetype definition
typedef struct
{
	char name[32];	// Filetype name
	char id[8];		// Filetype ID
	UWORD flags;	// Flags
	char priority;	// Priority
	char pad;		// Padding
	UWORD count;	// Number of functions defined

	// Followed by recognition string
	// Followed by icon path
	// Followed by action strings
} CFG_TYPE;

#define FILETYPEF_CHECKMOD (1 << 0)	 // Check for ST module
#define FILETYPEF_OVERRIDE (1 << 1)	 // Over-ride

// HKEY - Hotkey definition
typedef struct
{
	ULONG flags;  // Flags

	// Followed by name
} CFG_HKEY;

// LBUT - Lister button definition
typedef struct
{
	ULONG flags;  // Flags
	// Followed by buttons
} CFG_LBUT;

#define DIRFLAGS_DISABLE_CACHING (1 << 0)  // Disable caching
#define DIRFLAGS_REREAD_CHANGED (1 << 1)   // Re-read changed buffers
#define DIRFLAGS_EXPANDPATHS (1 << 7)	   // Expand pathnames

#define SCRFLAGS_DEFWIDTH (1 << 0)	 // Default width
#define SCRFLAGS_DEFHEIGHT (1 << 1)	 // Default height

#define MODE_WORKBENCHUSE 1
#define MODE_WORKBENCHCLONE 2
#define MODE_PUBLICSCREEN 3

#define ENVRF_AUTO_TILE (1 << 0)  // Do auto-tile on startup

#define FONT_SCREEN 0
#define FONT_DIRS 1
#define FONT_ICONS 2
#define FONT_WINDOW_ICONS 3

// env_flags
#define ENVF_CLOCK (1 << 0)			  // Clock on
#define ENVF_BACKDROP (1 << 1)		  // Backdrop window
#define ENVF_CHANGED (1 << 2)		  // Environment has changed
#define ENVF_DESKTOP_FOLDER (1 << 3)  // Desktop folder popup
#define ENVF_USE_PATHLIST (1 << 4)	  // Use path list
#define ENVF_BENIFY (1 << 5)		  // Drop on Desktop saves position permanently (was dopus/ReturnOfBenify)
#define ENVF_ENABLE_SHORTCUTS (1 << 6) // Extra desktop popup keyboard shortcuts (was dopus/EnableShortcuts)

#define ENVNIF_ENABLE (1 << 0)
#define ENVNIF_DISCOURAGE (1 << 1)
#define ENVNIF_DITHERING (1 << 2)
#define ENVNIF_RTG (1 << 3)

// env_BackgroundFlags
#define ENVBF_CENTER_PIC (1 << 0)
#define ENVBF_PRECISION_NONE (1 << 1)
#define ENVBF_PRECISION_GUI (1 << 2)
#define ENVBF_PRECISION_ICON (1 << 3)
#define ENVBF_PRECISION_IMAGE (1 << 4)
#define ENVBF_PRECISION_EXACT (1 << 5)
#define ENVBF_STRETCH_PIC (1 << 6)
#define ENVBF_USE_COLOUR (1 << 7)

#define ENVBF_COL_R_GET(x) (((x) >> 24) & 0xff)
#define ENVBF_COL_R_SET(x) ((x) << 24)
#define ENVBF_COL_G_GET(x) (((x) >> 16) & 0xff)
#define ENVBF_COL_G_SET(x) ((x) << 16)
#define ENVBF_COL_B_GET(x) (((x) >> 8) & 0xff)
#define ENVBF_COL_B_SET(x) ((x) << 8)
#define ENVBF_COL_MASK 0xffffff00

enum {
	DESKTOP_POPUP_NONE,
	DESKTOP_POPUP_LEFTOUT,
	DESKTOP_POPUP_MOVE,
	DESKTOP_POPUP_COPY,
};

// colours
enum {
	ENVCOL_FILES_UNSEL,
	ENVCOL_DIRS_UNSEL,
	ENVCOL_FILES_SEL,
	ENVCOL_DIRS_SEL,
	ENVCOL_SOURCE,
	ENVCOL_DEST,
	ENVCOL_DEVICES,
	ENVCOL_VOLUMES,
	ENVCOL_GAUGE,
	ENVCOL_TOOLTIP,

	ENVCOL_MAX = 16
};

// display_options
#define DISPOPTF_NO_BACKDROP (1 << 0)		// No display backdrop
#define DISPOPTF_NO_ALLOCPEN (1 << 1)		// No alloc pens
#define DISPOPTF_SHOW_APPICONS (1 << 2)		// Show AppIcons
#define DISPOPTF_SHOW_TOOLS (1 << 3)		// Show Tools menu
#define DISPOPTF_HIDE_BAD (1 << 4)			// Hide bad disks
#define DISPOPTF_DEFPUBSCR (1 << 5)			// Default public screen
#define DISPOPTF_ICON_POS (1 << 6)			// Use Workbench icon positions
#define DISPOPTF_SHIFT_APPICONS (1 << 7)	// Shift AppIcons to Tools menu
#define DISPOPTF_USE_WBPATTERN (1 << 8)		// Use WBPattern prefs
#define DISPOPTF_REMOPUSPOS (1 << 9)		// Remove Opus icon positions
#define DISPOPTF_REALTIME_SCROLL (1 << 10)	// Real-time icon scrolling
#define DISPOPTF_THIN_BORDERS (1 << 11)		// Thin borders
#define DISPOPTF_SHOW_WBLEFTOUTS (1 << 12)	// show workbench's leftout icons from the .backdrop files
#define DISPOPTF_WB_TITLE (1 << 13)			// Workbench-compatible title bar mode for WB-style patches
#define DISPOPTF_USE_WBINFO (1 << 14)		// Route Information requests through OS WBInfo() (e.g. SwazInfo / RAWBInfo)
#define DISPOPTF_SHOW_DATATYPES_FIRST (1 << 15)	// show.module: prefer datatypes IFF over its built-in IFF reader

// Module mod_data flags (passed in d1 to Module_Entry).  Each module owns its
// own d1 namespace, so different modules can reuse the same bit for unrelated
// per-module options without conflict.
#define ICON_USE_WBINFO_FLAG (1 << 16)		// icon.module: caller wants it to defer to WBInfo()
#define SHOW_DATATYPES_FIRST_FLAG (1 << 16)	// show.module: caller wants it to try datatypes first

// lister options
#define LISTEROPTF_DEVICES (1 << 0)	   // Device list in new lister
#define LISTEROPTF_2XCLICK (1 << 1)	   // Double-click editing
#define LISTEROPTF_SIMPLE (1 << 2)	   // Simple-refresh windows
#define LISTEROPTF_TITLES (1 << 3)	   // Want field titles
#define LISTEROPTF_SNAPSHOT (1 << 4)   // Always use snapshot position
#define LISTEROPTF_POPUP (1 << 5)	   // PopUp in name mode
#define LISTEROPTF_EDIT_LEFT (1 << 6)  // Editing with left button
#define LISTEROPTF_EDIT_MID (1 << 7)   // Editing with middle button
#define LISTEROPTF_EDIT_BOTH (LISTEROPTF_EDIT_LEFT | LISTEROPTF_EDIT_MID)
#define LISTEROPTF_FUEL_GAUGE (1 << 8)		 // Free space gauge
#define LISTEROPTF_SUBDROP (1 << 9)			 // Drop into sub-directories
#define LISTEROPTF_NOACTIVESELECT (1 << 10)	 // No file select on window activation
#define LISTEROPTF_VOSTY_ZOOM (1 << 11)		 // Ben Vost zoom mode
#define LISTEROPTF_FULL_PATH (1 << 12)		 // Show full path in lister title
#define LISTEROPTF_NO_PADLOCK (1 << 13)		 // Hide padlock gadget from lister title bars
#define LISTEROPTF_DUAL_DEFAULT (1 << 14)	 // Open new listers in dual mode

// desktop flags
#define DESKTOPF_DISTINCT (1 << 0)		  // Distinct icon positions
#define DESKTOPF_NO_CACHE (1 << 1)		  // Don't cache icons
#define DESKTOPF_NO_REMAP (1 << 2)		  // Don't remap icons
#define DESKTOPF_NO_BORDERS (1 << 3)	  // No icon borders
#define DESKTOPF_NO_ARROW (1 << 4)		  // Disable left-out arrow
#define DESKTOPF_SPLIT_LABELS (1 << 5)	  // Split long icon labels
#define DESKTOPF_TRANSPARENCY (1 << 6)	  // Full icon transparency
#define DESKTOPF_NO_NOLABELS (1 << 7)	  // No labelless icons
#define DESKTOPF_SMART_ICONCOPY (1 << 8)  // Smart icon copying
#define DESKTOPF_FORCE_SPLIT (1 << 9)	  // Force split of labels
#define DESKTOPF_QUICK_DRAG (1 << 10)	  // Quick icon dragging
#define DESKTOPF_NO_CUSTOMDRAG (1 << 11)  // No custom dragging

// main_window_type
enum { MAINWINDOW_ICONS, MAINWINDOW_DEVICES, MAINWINDOW_EMPTY };

#define DLSTF_ICONIFIED (1 << 0)	 // Lister opens iconified
#define DLSTF_DEVICE_LIST (1 << 1)	 // Lister shows device list
#define DLSTF_NO_POSITION (1 << 2)	 // No position set
#define DLSTF_LOCK_POS (1 << 3)		 // Position is locked
#define DLSTF_SOURCE (1 << 4)		 // Lister starts as a source
#define DLSTF_DEST (1 << 5)			 // Starts as destination
#define DLSTF_LOCKED (1 << 6)		 // Source/dest locked
#define DLSTF_ICON (1 << 7)			 // View as icons
#define DLSTF_SHOW_ALL (1 << 8)		 // Show all
#define DLSTF_CACHE_LIST (1 << 9)	 // Cache list
#define DLSTF_ICON_ACTION (1 << 10)	 // Icon action
#define DLSTF_DEV_FULL (1 << 11)	 // Full device list
#define DLSTF_DEV_BRIEF (1 << 12)	 // Brief device list
#define DLSTF_INVISIBLE (1 << 13)	 // Opens invisible
#define DLSTF_NOACTIVE (1 << 14)	 // No activate

// Error codes
#define ERROR_OLD_CONFIG (-1)

// Desktop entry
typedef struct
{
	UWORD dt_Type;
	UWORD dt_Size;
	ULONG dt_Flags;
	ULONG dt_Data;
} CFG_DESK;

enum {
	DESKTOP_ICON,
	DESKTOP_HIDE,
	DESKTOP_LEAVEOUT,
	DESKTOP_ICONPOS,
	DESKTOP_HIDE_BAD,
};

#define ICONPOSF_APPICONS (1 << 0)
#define ICONPOSF_DISKS (1 << 1)
#define ICONPOSF_LISTERS (1 << 2)
#define ICONPOSF_GROUPS (1 << 3)
#define ICONPOSF_LEFTOUTS (1 << 4)
#define ICONPOSF_PRIORITY(x) (((x) >> 24) & 0xff)
#define ICONPOSF_CLEARPRI(x) ((x)&0x00ffffff)
#define ICONPOSF_MAKEPRI(x) ((x) << 24)

/****************************************************************************
					 Application Configuration Structures
 ****************************************************************************/

// Function instruction
typedef struct
{
	struct MinNode node;
	short type;
	char *string;
} Cfg_Instruction;

enum {
	INST_COMMAND,	 // Internal command
	INST_AMIGADOS,	 // AmigaDOS command
	INST_WORKBENCH,	 // Workbench command
	INST_SCRIPT,	 // Batch file
	INST_AREXX,		 // ARexx script
	INST_LABEL,		 // Label

	INST_DIR = 100,
	INST_PROJECT,
};

// Function definition
typedef struct
{
	struct Node node;
	CFG_FUNC function;
	struct MinList instructions;
} Cfg_Function;

// Button function
typedef struct
{
	struct Node node;
	CFG_FUNC function;
	struct MinList instructions;
	char *label;
	APTR image;
} Cfg_ButtonFunction;

// Button definition
typedef struct
{
	struct Node node;
	CFG_BUTN button;			   // Button definition
	struct MinList function_list;  // Button functions
	short current;				   // Current selection
	short pad;
} Cfg_Button;

// Button bank definition
typedef struct
{
	struct Node node;
	CFG_BTNW window;			  // Window definition
	struct List buttons;		  // List of buttons
	char path[256];				  // Full path of button bank
	APTR memory;				  // Memory pool for this bank
	struct SignalSemaphore lock;  // Semaphore lock
	CFG_STRT *startmenu;		  // Start menu data
	char backpic[256];			  // Background picture
} Cfg_ButtonBank;

// List of filetypes (for storage)
typedef struct
{
	struct Node node;
	char path[256];				// File path
	struct DateStamp date;		// File datestamp
	struct List filetype_list;	// List of types
	ULONG flags;
} Cfg_FiletypeList;

#define FTLISTF_INTERNAL (1 << 0)	 // Internal list, not to be modified
#define FTLISTF_CHANGED (1 << 1)	 // List has been modified
#define FTLISTF_STORE (1 << 2)		 // List is to be stored
#define FTLISTF_NOT_FOUND (1 << 31)	 // Special flag; internal use only

// Filetype definition
typedef struct
{
	struct Node node;
	CFG_TYPE type;				 // Filetype information
	unsigned char *recognition;	 // Recognition string
	char *icon_path;			 // Icon path
	char **actions;				 // Action strings
	struct List function_list;	 // Function list
	Cfg_FiletypeList *list;		 // List we are in
} Cfg_Filetype;

// Hotkey definition
typedef struct
{
	struct Node node;
	CFG_HKEY hotkey;			// Hotkey definition
	char *name;					// Hotkey name
	struct List function_list;	// Hotkey function
} Cfg_Hotkey;

// Lister buttons
typedef struct
{
	CFG_LBUT header;		  // Lister buttons header
	struct List button_list;  // List of buttons
} Cfg_ListerButtons;

// Desktop entry
typedef struct
{
	struct MinNode node;
	CFG_DESK data;
} Cfg_Desktop;

#define CFGSEF_CHANGED (1 << 15)

typedef struct
{
	struct Node dse_Node;
	char dse_Name[32];
	short dse_Volume;
	unsigned short dse_Flags;
	char dse_Sound[256];
	char dse_Random[256];
	short dse_RandomVolume;
} Cfg_SoundEntry;

/****************************************************************************
						   Other useful structures
 ****************************************************************************/

typedef struct
{
	struct Screen *screen;
	struct DrawInfo *draw_info;
	unsigned short depth;
	unsigned short pen_alloc;
	unsigned short pen_array[16];
	short pen_count;
} DOpusScreenData;

/****************************************************************************
							 Configuration flags
 ****************************************************************************/

// CFG_SETS		copy_flags
#define COPY_DATE (1 << 0)
#define COPY_PROT (1 << 1)
#define COPY_NOTE (1 << 2)
#define COPY_ARC (1 << 3)
#define COPY_CHECK (1 << 4)
#define COPY_UPDATE (1 << 5)

// CFG_SETS		date_flags
#define DATE_SUBST (1 << 0)
#define DATE_12HOUR (1 << 1)
#define DATE_1000SEP (1 << 2)

// CFG_SETS		delete_flags
#define DELETE_ASK (1 << 0)
#define DELETE_FILES (1 << 1)
#define DELETE_DIRS (1 << 2)
#define DELETE_SET (1 << 3)

// CFG_SETS		icon_flags
#define ICONFLAG_DOUNTOICONS (1 << 1)
#define ICONFLAG_AUTOSELECT (1 << 2)
#define ICONFLAG_TRAP_MORE (1 << 3)

// CFG_SETS		replace_method
enum {
	REPLACE_ALWAYS,
	REPLACE_NEVER,
	REPLACE_OLDER,
	REPLACE_ASK,
};

// CFG_SETS		replace_flags
#define REPLACEF_CHECK_VERSION (1 << 0)
#define REPLACEF_VERBOSE_REPLACE (1 << 1)

// CFG_SETS		update_flags
#define UPDATEF_SCROLL (1 << 1)
#define UPDATEF_NOTIFY (1 << 3)

// CFG_SETS		hide_method
enum { HIDE_CLOCK, HIDE_NONE, HIDE_ICON, HIDE_MENU };

// CFG_SETS		general_flags
#define GENERALF_MOUSE_HOOK (1 << 20)
#define GENERALF_VERBOSE_REPLACE (1 << 21)
#define GENERALF_QUICK_QUIT (1 << 22)
#define GENERALF_EXTENDED_KEY (1 << 23)
#define GENERALF_FILETYPE_SNIFFER (1 << 24)
#define GENERALF_VALID_POPKEY (1 << 25)
#define GENERALF_ENABLE_MUFS (1 << 26)
#define GENERALF_EXCLUSIVE_STARTUP_SND (1 << 27)

/* ---------- BitMapHeader ---------------------------------------------*/
/*  Required Bitmap header (BMHD) structure describes an ILBM */
typedef struct
{
	UWORD w, h;		   /* Width, height in pixels */
	WORD x, y;		   /* x, y position for this bitmap  */
	UBYTE nPlanes;	   /* # of planes (not including mask) */
	UBYTE masking;	   /* a masking technique listed above */
	UBYTE compression; /* cmpNone or cmpByteRun1 */
	UBYTE flags;	   /* as defined or approved by Commodore */
	UWORD transparentColor;
	UBYTE xAspect, yAspect;
	WORD pageWidth, pageHeight;
} BitMapHeader;

typedef struct
{
	UWORD version;
	UWORD nframes;
	unsigned char framespersecond;
	char pad;
	UWORD flags;
} DPAnimChunk;

typedef struct
{
	UBYTE operation;
	UBYTE mask;
	UWORD w, h;
	WORD x, y;
	ULONG abstime;
	ULONG reltime;
	UBYTE interleave;
	UBYTE pad0;
	ULONG bits;
	UBYTE pad[16];
} AnimHdr;

typedef struct
{
	struct MinNode node;

	AnimHdr header;		   // ANIM header
	unsigned char *delta;  // DLTA data
} ANIMFrame;

typedef struct
{
	DPAnimChunk header;	 // DeluxePaint anim header

	struct MinList frames;		 // Frame list (for ANIMs)
	ANIMFrame *current;			 // Current frame
	unsigned short frame_count;	 // Number of frames
} ANIMHandle;

typedef struct
{
	APTR memory;  // Memory handle

	BitMapHeader header;  // ILBM header
	ULONG mode_id;		  // Mode ID
	ULONG flags;		  // Flags

	union
	{
		struct
		{
			unsigned char *data;  // BODY data
			unsigned long size;	  // BODY size
		} body;

		struct
		{
			struct BitMap *bitmap;	// Displayable bitmap
		} bitmap;

		struct
		{
			unsigned short **planes;  // Image plane data (not chip memory)
		} planes;
	} image;

	ULONG *palette;	 // Image palette

	ANIMHandle *anim;  // Animation data
} ILBMHandle;

typedef struct
{
	unsigned char *sourceptr;	 // Source data
	unsigned char **destplanes;	 // Destination planes
	unsigned short imagebpr;	 // Bytes-per-row in source
	unsigned short imageheight;	 // Image height
	unsigned short imagedepth;	 // Image depth
	unsigned short destbpr;		 // Bytes-per-row in destination
	unsigned short destheight;	 // Destination height
	unsigned short destdepth;	 // Destination depth
	char masking;				 // Image masking
	char compression;			 // Compression type
	unsigned long offset;		 // Offset in destination
} RLEinfo;

typedef struct _DragInfo
{
	struct RastPort *rastport;	// Stores RastPort this bob belongs to
	struct ViewPort *viewport;	// Stores ViewPort

	WORD width;	  // Bob width
	WORD height;  // Bob height

	struct VSprite sprite;	// VSprite structure
	struct Bob bob;			// BOB structure

	unsigned long flags;  // Flags

	struct RastPort drag_rp;  // RastPort we can draw into
	struct BitMap drag_bm;	  // BitMap we can draw into

	struct Window *window;	// Window pointer

	long offset_x;
	long offset_y;
	long userdata;
	long pad[4];

	struct DBufPacket *dbuf;  // Double-buffering packet

	long dest_width;
	long dest_height;
} DragInfo;

typedef struct
{
	struct VSprite head;   // GEL list head sprite
	struct VSprite tail;   // GEL list tail sprite
	struct GelsInfo info;  // GEL info
} DragInfoExtra;

#define DRAGF_VALID (1 << 0)		// Bob is valid
#define DRAGF_OPAQUE (1 << 1)		// Bob should be opaque
#define DRAGF_DONE_GELS (1 << 2)	// Installed GelsInfo
#define DRAGF_NO_LOCK (1 << 3)		// Don't lock layers
#define DRAGF_TRANSPARENT (1 << 4)	// Bob should be transparent (use with opaque)
#define DRAGF_DBUF (1 << 5)			// Double-buffering
#define DRAGF_NOSORT (1 << 6)		// Don't resort list on draw
#define DRAGF_NOWAIT (1 << 7)		// Don't WaitTOF on draw
#define DRAGF_FREE (1 << 8)
#define DRAGF_CUSTOM (1 << 9)	 // Custom rendering
#define DRAGF_NO_MASK (1 << 10)	 // No masking

#define DRAGF_NEED_GELS (1 << 0)  // Need GELs initialised
#define DRAGF_REMOVE (1 << 0)	  // Remove only

// Layout routines
#define POS_CENTER -1
#define POS_RIGHT_JUSTIFY -2
#define POS_MOUSE_CENTER -3
#define POS_MOUSE_REL -4
#define POS_PROPORTION 1024
#define POS_SQUARE 1124
#define POS_REL_RIGHT (1 << 14)

#define FPOS_TEXT_OFFSET 16384

#define SIZE_MAXIMUM -1
#define SIZE_MAX_LESS -101

// Defines a window
typedef struct _ConfigWindow
{
	struct IBox char_dim;
	struct IBox fine_dim;
} ConfigWindow;

// Opens a window
typedef struct _NewConfigWindow
{
	void *parent;				 // Parent to open on
	ConfigWindow *dims;			 // Window dimensions
	char *title;				 // Window title
	struct DOpusLocale *locale;	 // Locale to use
	struct MsgPort *port;		 // Message port to use
	ULONG flags;				 // Flags
	struct TextFont *font;		 // Alternative font to use
} NewConfigWindow;

// Set by the user
#define WINDOW_SCREEN_PARENT (1 << 0)  // Parent is a screen
#define WINDOW_NO_CLOSE (1 << 1)	   // No close gadget
#define WINDOW_NO_BORDER (1 << 2)	   // No border
#define WINDOW_LAYOUT_ADJUST (1 << 3)  // Adjust window size to fit objects
#define WINDOW_SIMPLE (1 << 4)		   // Simple refresh
#define WINDOW_AUTO_REFRESH (1 << 5)   // Refresh window automatically
#define WINDOW_AUTO_KEYS (1 << 6)	   // Handle keys automatically
#define WINDOW_OBJECT_PARENT (1 << 7)  // Parent is an existing object
#define WINDOW_REQ_FILL (1 << 8)	   // Backfill as a requester
#define WINDOW_NO_ACTIVATE (1 << 9)	   // Don't activate
#define WINDOW_VISITOR (1 << 10)	   // Open as visitor window
#define WINDOW_SIZE_RIGHT (1 << 11)	   // Size gadget, in right border
#define WINDOW_SIZE_BOTTOM (1 << 12)   // Size gadget, in bottom border
#define WINDOW_ICONIFY (1 << 13)	   // Iconify gadget

// Set by the system
#define OPEN_USED_DEFAULT (1 << 16)	 // To open had to use default font
#define OPEN_USED_TOPAZ (1 << 17)	 // To open had to use topaz
#define OPEN_SHRUNK_VERT (1 << 18)	 // Window is not full vertical size requested
#define OPEN_SHRUNK_HORIZ (1 << 19)	 // Window is not full horizontal size requested
#define OPEN_SHRUNK (OPEN_SHRUNK_VERT | OPEN_SHRUNK_HORIZ)

#define GAD_ID_ICONIFY 0xffa0

// Defines an object
typedef struct _ObjectDef
{
	BYTE type;
	UBYTE object_kind;
	struct IBox char_dims;
	struct IBox fine_dims;
	IPTR gadget_text;
	ULONG flags;
	UWORD gadgetid;
	const struct TagItem *taglist;
} ObjectDef;

#define GADFLAG_GLASS (1 << 16)	 // Magnifying-glass gadget

#define BUTTONFLAG_IMMEDIATE (1 << 8)
#define BUTTONFLAG_OKAY_BUTTON (1 << 18)	// Button is an "ok" button
#define BUTTONFLAG_CANCEL_BUTTON (1 << 19)	// Button is a "cancel" button
#define BUTTONFLAG_TOGGLE_SELECT (1 << 20)	// Button is toggle-select
#define LISTVIEWFLAG_CURSOR_KEYS (1 << 21)	// Lister responds to cursor
#define BUTTONFLAG_THIN_BORDERS (1 << 22)	// Button has thin borders
#define FILEBUTFLAG_SAVE (1 << 21)			// Save mode

#define TEXTFLAG_RIGHT_JUSTIFY (1 << 1)	 // Right-justify text
#define TEXTFLAG_CENTER (1 << 2)		 // Center text
#define TEXTFLAG_TEXT_STRING (1 << 17)	 // Text is a string, not a Locale ID
#define TEXTFLAG_NO_USCORE (1 << 18)	 // No underscore in text
#define TEXTFLAG_ADJUST_TEXT (1 << 23)	 // Adjust for text

#define POSFLAG_ADJUST_POS_X (1 << 24)	// Position adjustor
#define POSFLAG_ADJUST_POS_Y (1 << 25)	// Position adjustor
#define POSFLAG_ALIGN_POS_X (1 << 26)	// Align
#define POSFLAG_ALIGN_POS_Y (1 << 27)	// Align

#define AREAFLAG_RAISED (1 << 8)		// Raised rectangle
#define AREAFLAG_RECESSED (1 << 9)		// Recessed rectangle
#define AREAFLAG_THIN (1 << 10)			// Thin borders
#define AREAFLAG_ICON (1 << 11)			// Icon drop box
#define AREAFLAG_ERASE (1 << 12)		// Erase interior
#define AREAFLAG_LINE (1 << 13)			// Line (separator)
#define AREAFLAG_GRAPH (1 << 14)		// Graph display
#define AREAFLAG_OPTIM (1 << 15)		// Optimised refreshing
#define AREAFLAG_TITLE (1 << 16)		// Box with a title
#define AREAFLAG_NOFILL (1 << 18)		// No fill
#define AREAFLAG_FILL_COLOUR (1 << 19)	// Always fill with colour

#define OBJECTFLAG_DRAWN (1 << 31)	// Object has been drawn

#define OD_END 0	 // End of a list
#define OD_GADGET 1	 // A gadget
#define OD_TEXT 2	 // Some text
#define OD_AREA 3	 // A rectangular area
#define OD_IMAGE 4	 // An image
#define OD_SKIP -1	 // Skip this entry

typedef struct _GL_Object
{
	struct _GL_Object *next;  // Next object
	short type;				  // Type of object
	unsigned char key;		  // Key equivalent

	unsigned char flags2;  // Additional flags

	unsigned short id;			// Object ID
	unsigned short control_id;	// Object that this controls
	struct IBox dims;			// Object dimensions
	ULONG flags;				// Object flags
	char *text;					// Text
	UWORD object_kind;			// Object kind

	union
	{
		struct _gl_gadget
		{
			struct Gadget *context;	 // Context data for the gadget
			struct Gadget *gadget;	 // The gadget itself
			int components;			 // Number of component gadgets
			IPTR data;				 // Some data for the gadget
			short choice_max;		 // Number of choices
			short choice_min;		 // Minimum choice
			struct Image *image;	 // Gadget image
		} gl_gadget;

		struct _gl_text
		{
			struct IBox text_pos;  // Text position
			WORD base_pos;		   // Baseline position
			short uscore_pos;	   // Underscore position
		} gl_text;

		struct _gl_area
		{
			struct IBox text_pos;  // Text position within area
			struct IBox area_pos;  // Area position
			int frametype;		   // Frame type
		} gl_area;

		struct _gl_image
		{
			struct IBox image_pos;	// Image position
			struct Image *image;	// Image
		} gl_image;
	} gl_info;

	APTR memory;  // Any other memory

	char *original_text;  // Original text string
	char fg, bg;		  // Current pen colours

	IPTR data_ptr;	 // Pointer to other data

	struct TagItem *tags;  // Copy of tags

	struct IBox char_dims;	// Original dimensions
	struct IBox fine_dims;
} GL_Object;

typedef struct _ObjectList
{
	struct MinNode node;
	GL_Object *firstobject;	 // First object
	struct TextAttr attr;	 // Font used
	struct Window *window;	 // Window used
} ObjectList;

#define OBJECTF_NO_SELECT_NEXT (1 << 0)	 // Don't select next field
#define OBJECTF_PATH_FILTER (1 << 1)	 // Filter path characters
#define OBJECTF_SECURE (1 << 2)			 // Hide string
#define OBJECTF_INTEGER (1 << 3)		 // Integer gadget
#define OBJECTF_READ_ONLY (1 << 4)		 // Read-only
#define OBJECTF_HOTKEY (1 << 5)			 // Hotkey string
#define OBJECTF_UPPERCASE (1 << 6)		 // Uppercase string

typedef struct _MenuData
{
	UBYTE type;	  // Menu type
	IPTR id;	  // Menu ID or user data
	IPTR name;	  // Menu name
	ULONG flags;  // Menu flags
} MenuData;

#define MENUFLAG_TEXT_STRING (1 << 16)	// Menu name is a real string
#define MENUFLAG_COMM_SEQ (1 << 17)		// Give menu a command sequence
#define MENUFLAG_AUTO_MUTEX (1 << 18)	// Automatic mutual exclusion
#define MENUFLAG_USE_SEQ (1 << 19)		// Use command sequence supplied

#define MENUFLAG_MAKE_SEQ(c) ((ULONG)(c) << 24)
#define MENUFLAG_GET_SEQ(fl) ((char)((fl) >> 24))

#define NM_NEXT 10
#define NM_BAR_LABEL (IPTR) NM_BARLABEL

#define IS_GADTOOLS(obj) (BOOL)(obj->gl_info.gl_gadget.context)

typedef struct
{
	ULONG magic;			   // Magic ID
	struct Window *window;	   // Pointer back to window
	ULONG window_id;		   // User window ID
	struct MsgPort *app_port;  // "Window's" application port
} WindowID;

#define WINDOW_MAGIC 0x83224948
#define WINDOW_UNKNOWN (ULONG) - 1
#define WINDOW_UNDEFINED 0

// Window types
#define WINDOW_BACKDROP 0x4000001
#define WINDOW_LISTER 0x4000002
#define WINDOW_BUTTONS 0x4000004
#define WINDOW_GROUP 0x4000008
#define WINDOW_LISTER_ICONS 0x4000010
#define WINDOW_FUNCTION 0x4000020  // not really a window
#define WINDOW_START 0x4000040

#define WINDOW_POPUP_MENU 0x0001200
#define WINDOW_TEXT_VIEWER 0x0001300
#define WINDOW_ICONPOS 0x0001400
#define WINDOW_KEYFINDER 0x0001500
#define WINDOW_ICONINFO 0x0001600

typedef struct _WindowData
{
	WindowID id;  // Window ID information

	ULONG pad3;
	struct FileRequester *request;	// Window's file requester
	APTR visinfo;					// Visual info
	struct DrawInfo *drawinfo;		// Draw info
	struct DOpusLocale *locale;		// Locale info
	struct MsgPort *window_port;	// Window message port (if supplied)
	struct NewMenu *new_menu;		// NewMenu structure allocated
	struct Menu *menu_strip;		// Menu strip allocated
	struct Requester *busy_req;		// Window busy requester
	IPTR data;						// Window-specific data
	ULONG flags;					// Flags
	APTR memory;					// User memory chain, freed when window closes

	ULONG pad;
	struct FontRequester *font_request;	 // Window's font requester

	IPTR userdata;
	struct TagItem *user_tags;

	struct List boopsi_list;  // BOOPSI list

	struct Hook *backfill;	// Backfill hook

	struct MinList object_list;
} WindowData;

#define FILE_GLASS_KIND 1000
#define DIR_GLASS_KIND 1001

#define GM_RESIZE (20)

struct gpResize
{
	ULONG MethodID;
	struct GadgetInfo *gpr_GInfo;
	struct RastPort *gpr_RPort;
	struct IBox gpr_Size;
	long gpr_Redraw;
	struct Window *gpr_Window;
	struct Requester *gpr_Requester;
};

// Custom tags
#define GTCustom_LocaleLabels TAG_USER + 0	   // Points to list of Locale IDs
#define GTCustom_Image TAG_USER + 1			   // Image for gadget
#define GTCustom_CallBack TAG_USER + 2		   // Tag ID and data filled in by callback
#define GTCustom_LayoutRel TAG_USER + 3		   // Layout relative to this object ID
#define GTCustom_Control TAG_USER + 4		   // Controls another gadget
#define GTCustom_TextAttr TAG_USER + 6		   // TextAttr to use
#define GTCustom_MinMax TAG_USER + 24		   // Minimum and maximum bounds
#define GTCustom_BorderButton TAG_USER + 26	   // Border button
#define GTCustom_ThinBorders TAG_USER + 27	   // Gadget has thin borders
#define GTCustom_TextInside TAG_USER + 28	   // Gadget has text inside it
#define GTCustom_LocaleKey TAG_USER + 29	   // Key from locale string
#define GTCustom_Flags TAG_USER + 30		   // Flags
#define GTCustom_NoSelectNext TAG_USER + 31	   // Don't select next field
#define GTCustom_PathFilter TAG_USER + 32	   // Filter path characters
#define GTCustom_History TAG_USER + 33		   // History
#define GTCustom_CopyTags TAG_USER + 34		   // Copy tags
#define GTCustom_FontPens TAG_USER + 35		   // Place to store pens and style
#define GTCustom_FontPenCount TAG_USER + 36	   // Number of pens for font requester
#define GTCustom_FontPenTable TAG_USER + 37	   // Table of pens for font requester
#define GTCustom_Bold TAG_USER + 38			   // Bold pen
#define GTCustom_Secure TAG_USER + 39		   // Secure string field
#define GTCustom_Integer TAG_USER + 40		   // Integer gadget
#define GTCustom_TextPlacement TAG_USER + 41   // Position of text
#define GTCustom_NoGhost TAG_USER + 42		   // Disable without ghosting
#define GTCustom_LayoutFlags TAG_USER + 43	   // Layout flags
#define GTCustom_Style TAG_USER + 44		   // Pen styles
#define GTCustom_FrameFlags TAG_USER + 45	   // Frame flags
#define GTCustom_ChangeSigTask TAG_USER + 46   // Task to signal on change
#define GTCustom_ChangeSigBit TAG_USER + 47	   // Signal bit to use
#define GTCustom_LayoutPos TAG_USER + 49	   // Use with the POSFLAGs
#define GTCustom_Borderless TAG_USER + 50	   // Borderless
#define GTCustom_Justify TAG_USER + 51		   // Justification
#define GTCustom_UpperCase TAG_USER + 53	   // Uppercase
#define GTCustom_Recessed TAG_USER + 54		   // Recessed border
#define GTCustom_FixedWidthOnly TAG_USER + 55  // Only show fixed-width fonts

#define LAYOUTF_SAME_HEIGHT (1 << 0)
#define LAYOUTF_SAME_WIDTH (1 << 1)
#define LAYOUTF_TOP_ALIGN (1 << 2)
#define LAYOUTF_BOTTOM_ALIGN (1 << 3)
#define LAYOUTF_LEFT_ALIGN (1 << 4)
#define LAYOUTF_RIGHT_ALIGN (1 << 5)

#define JUSTIFY_LEFT 0
#define JUSTIFY_RIGHT 1
#define JUSTIFY_CENTER 2

#define DIA_Type TAG_USER + 5		   // Image type
#define DIA_FrontPen TAG_USER + 7	   // Image front pen
#define DIA_ThinBorders TAG_USER + 27  // Thin borders

#define IM_ARROW_UP 0
#define IM_ARROW_DOWN 1
#define IM_CHECK 2
#define IM_DRAWER 3
#define IM_BORDER_BOX 4
#define IM_BBOX 5
#define IM_ICONIFY 6
#define IM_CROSS 7
#define IM_LOCK 8
#define IM_GAUGE 9	// Not really an image

#define MY_LISTVIEW_KIND 127   // Custom listview gadget
#define FILE_BUTTON_KIND 126   // File button gadget
#define DIR_BUTTON_KIND 125	   // Directory button gadget
#define FONT_BUTTON_KIND 124   // Font button gadget
#define FIELD_KIND 123		   // Text field (no editing)
#define FRAME_KIND 122		   // Frame
#define HOTKEY_KIND 121		   // Hotkey field
#define POPUP_BUTTON_KIND 120  // File button gadget (no popup)

// Listview tags
#define DLV_ScrollUp TAG_USER + 7			// Scroll list up
#define DLV_ScrollDown TAG_USER + 8			// Scroll list down
#define DLV_SelectPrevious TAG_USER + 11	// Select previous item
#define DLV_SelectNext TAG_USER + 12		// Select next item
#define DLV_Labels GTLV_Labels				// Labels
#define DLV_Top GTLV_Top					// Top item
#define DLV_MakeVisible GTLV_MakeVisible	// Make visible
#define DLV_Selected GTLV_Selected			// Selected
#define DLV_ScrollWidth GTLV_ScrollWidth	// Scroller width
#define DLV_ShowSelected GTLV_ShowSelected	// Show selected
#define DLV_Check TAG_USER + 10				// Check selection
#define DLV_Highlight TAG_USER + 14			// Highlight selection
#define DLV_MultiSelect TAG_USER + 9		// Multi-selection
#define DLV_ReadOnly GTLV_ReadOnly			// Read only
#define DLV_Lines TAG_USER + 13				// Visible lines (get only)
#define DLV_ShowChecks TAG_USER + 15		// Show checkmarks
#define DLV_Flags TAG_USER + 16				// Layout flags
#define DLV_NoScroller TAG_USER + 17		// No scroller necessary
#define DLV_TopJustify TAG_USER + 18		// Top-justify items
#define DLV_RightJustify TAG_USER + 19		// Right-justify items
#define DLV_DragNotify TAG_USER + 20		// Notify of drags
#define DLV_GetLine TAG_USER + 25			// Get line from coordinate
#define DLV_DrawLine TAG_USER + 26			// Draw a line from the listview
#define DLV_Object TAG_USER + 27			// Pointer to object
#define DLV_DoubleClick TAG_USER + 28		// Indicates double-click
#define DLV_ShowFilenames TAG_USER + 48		// Show filenames only
#define DLV_ShowSeparators TAG_USER + 52	// Show --- as separator
#define DLV_PageUp TAG_USER + 53
#define DLV_PageDown TAG_USER + 54
#define DLV_Home TAG_USER + 55
#define DLV_End TAG_USER + 56
#define DLV_ScrollLeft TAG_USER + 57
#define DLV_ThinBorder TAG_USER + 58  // Thin borders

typedef struct
{
	struct RastPort *rp;
	struct DrawInfo *drawinfo;
	struct Node *node;
	unsigned short line;
	struct IBox box;
} ListViewDraw;

// Listview node data
#define lve_Flags ln_Type		// Listview entry flags
#define lve_Pen ln_Pri			// Listview entry pen
#define LVEF_SELECTED (1 << 0)	// Entry is selected
#define LVEF_USE_PEN (1 << 1)	// Use pen to render entry
#define LVEF_TEMP (1 << 2)		// Temporary flag for something

// Gauge tags
#define DGG_Total (TAG_USER + 0x64)			// Total size
#define DGG_Free (TAG_USER + 0x65)			// Free size
#define DGG_Flags (TAG_USER + 0x66)			// Flags
#define DGG_FillPen (TAG_USER + 0x67)		// Pen to fill gauge with
#define DGG_FillPenAlert (TAG_USER + 0x68)	// Pen when space is running out

// File button tags
#define DFB_DefPath TAG_USER + 19  // Default path

// Palette tags
#define DPG_Redraw TAG_USER + 20		  // Redraw a pen
#define DPG_Pen TAG_USER + 21			  // Ordinal selected pen
#define DPG_SelectPrevious TAG_USER + 22  // Select previous pen
#define DPG_SelectNext TAG_USER + 23	  // Select next pen

// Some useful macros
#define GADGET(obj) (obj->gl_info.gl_gadget.gadget)
#define AREA(obj) obj->gl_info.gl_area
#define DATA(win) ((WindowData *)win->UserData)
#define WINFLAG(win) (DATA(win)->flags)
#define WINMEMORY(win) (DATA(win)->memory)
#define WINREQUESTER(win) (DATA(win)->request)
#define OBJLIST(win) ((ObjectList *)(DATA(win)->object_list.mlh_Head))
#define DRAWINFO(win) (DATA(win)->drawinfo)
#define VISINFO(win) (DATA(win)->visinfo)
#define GADSPECIAL(list, id) (GADGET(L_GetObject(list, id))->SpecialInfo)
#define GADSTRING(list, id) ((struct StringInfo *)GADSPECIAL(list, id))->Buffer
#define GADNUMBER(list, id) ((struct StringInfo *)GADSPECIAL(list, id))->LongInt
#define GADSEL(list, id) (GADGET(L_GetObject(list, id))->Flags & GFLG_SELECTED)
#define GADGET_SPECIAL(list, id) (GADGET(GetObject(list, id))->SpecialInfo)
#define GADGET_STRING(list, id) ((struct StringInfo *)GADGET_SPECIAL(list, id))->Buffer
#define GADGET_NUMBER(list, id) ((struct StringInfo *)GADGET_SPECIAL(list, id))->LongInt
#define GADGET_SEL(list, id) (GADGET(GetObject(list, id))->Flags & GFLG_SELECTED)
#define CFGDATA(win) (((WindowData *)win->UserData)->data)

#define MENUID(menu) ((ULONG)(IPTR)GTMENUITEM_USERDATA(menu))

// Window dimensions
typedef struct
{
	struct IBox wd_Normal;
	struct IBox wd_Zoomed;
	unsigned short wd_Flags;
} WindowDimensions;

#define WDF_VALID (1 << 0)
#define WDF_ZOOMED (1 << 1)

// Locale marker
struct DOpusLocale
{
	APTR li_LocaleBase;
	APTR li_Catalog;
	char *li_BuiltIn;
	struct Locale *li_Locale;
};

// List management stuff
typedef struct _Att_List
{
	struct List list;			  // List structure
	struct SignalSemaphore lock;  // Semaphore for locking
	ULONG flags;				  // Flags
	APTR memory;				  // Memory pool
	struct _Att_Node *current;	  // Current node (application use)
} Att_List;

#define LISTF_LOCK (1 << 0)
#define LISTF_POOL (1 << 1)

typedef struct _Att_Node
{
	struct Node node;  // Node structure
	Att_List *list;	   // Pointer to list (inefficient!)
	IPTR data;		   // User data
} Att_Node;

#define ADDNODE_SORT 1		 // Sort names
#define ADDNODE_EXCLUSIVE 2	 // Exclusive entry
#define ADDNODE_NUMSORT 4	 // Numerical name sort
#define ADDNODE_PRI 8		 // Priority insertion

#define REMLIST_FREEDATA 1	// FreeVec data when freeing list
#define REMLIST_SAVELIST 2	// Don't free list itself
#define REMLIST_FREEMEMH 4	// FreeMemH data when freeing list

// Timer handle
typedef struct TimerHandle
{
	struct MsgPort *port;
	struct timerequest req;
	struct MsgPort *my_port;
	short active;
} TimerHandle;

// Notification message
typedef struct
{
	struct Message dn_Msg;
	ULONG dn_Type;
	ULONG dn_UserData;
	IPTR dn_Data;
	ULONG dn_Flags;
	struct FileInfoBlock *dn_Fib;
	char dn_Name[1];
} DOpusNotify;

#define NT_DOPUS_NOTIFY 199
#define DN_WRITE_ICON (1 << 0)		  // Icon written
#define DN_APP_ICON_LIST (1 << 1)	  // AppIcon added/removed
#define DN_APP_MENU_LIST (1 << 2)	  // AppMenu added/removed
#define DN_CLOSE_WORKBENCH (1 << 3)	  // Workbench closed
#define DN_OPEN_WORKBENCH (1 << 4)	  // Workbench opened
#define DN_RESET_WORKBENCH (1 << 5)	  // Workbench reset
#define DN_DISKCHANGE (1 << 6)		  // Disk inserted/removed
#define DN_OPUS_QUIT (1 << 7)		  // Main program quit
#define DN_OPUS_HIDE (1 << 8)		  // Main program hide
#define DN_OPUS_SHOW (1 << 9)		  // Main program show
#define DN_OPUS_START (1 << 10)		  // Main program start
#define DN_DOS_ACTION (1 << 11)		  // DOS action
#define DN_REXX_UP (1 << 12)		  // REXX started
#define DN_FLUSH_MEM (1 << 13)		  // Flush memory
#define DN_APP_WINDOW_LIST (1 << 14)  // AppWindow added/removed

#define DNF_ICON_REMOVED (1 << 0)		 // Icon removed
#define DNF_WINDOW_REMOVED (1 << 0)		 // AppWindow removed
#define DNF_ICON_CHANGED (1 << 1)		 // Icon changed
#define DNF_ICON_IMAGE_CHANGED (1 << 2)	 // Image was changed

#define DNF_DOS_CREATEDIR (1 << 0)		// CreateDir
#define DNF_DOS_DELETEFILE (1 << 1)		// DeleteFile
#define DNF_DOS_SETFILEDATE (1 << 2)	// SetFileDate
#define DNF_DOS_SETCOMMENT (1 << 3)		// SetComment
#define DNF_DOS_SETPROTECTION (1 << 4)	// SetProtection
#define DNF_DOS_RENAME (1 << 5)			// Rename
#define DNF_DOS_CREATE (1 << 6)			// Open file (create)
#define DNF_DOS_CLOSE (1 << 7)			// Close file
#define DNF_DOS_RELABEL (1 << 8)		// Relabel disk

// Inter-Process Communication
typedef struct
{
	struct Message msg;	  // Exec message
	IPTR command;		  // Message command/result
	IPTR flags;			  // Message flags
	APTR data;			  // Message data
	APTR data_free;		  // Data to be FreeVec()ed automatically
	struct _IPC *sender;  // Sender IPC
} IPCMessage;

//#define PF_ASYNC	(1<<7)
#define PORT_ASYNC_MAGIC 0x5A  // original dopus5code: #define PF_ASYNC	(1<<7)

#define REPLY_NO_PORT (struct MsgPort *)-1
#define REPLY_NO_PORT_IPC (struct MsgPort *)-2
#define NO_PORT_IPC (struct MsgPort *)-3

typedef struct _IPC
{
	struct MinNode node;
	struct Process *proc;		   // Process pointer
	IPCMessage startup_msg;		   // Startup message
	struct MsgPort *command_port;  // Port to send commands to
	struct ListLock *list;		   // List we're a member of
	APTR userdata;
	APTR memory;				 // Memory
	struct MsgPort *reply_port;	 // Port for replies
	ULONG flags;				 // Flags
} IPCData;

typedef struct
{
	Cfg_ButtonBank *bank;
	IPCData *ipc;
	long flag;
	unsigned long button;
} ButtonsStartup;

#define IPCF_INVALID (1 << 0)
#define IPCF_LISTED (1 << 1)
#define IPCF_STARTED (1 << 2)

#define IPCDATA(ipc) ((APTR)ipc->userdata)
#define SET_IPCDATA(ipc, data) ipc->userdata = (APTR)data

#define IPCF_GETPATH (1 << 31)
#define IPCF_DEBUG (1 << 30)
#define IPCM_STACK(s) (s & 0xffffff)

enum {
	IPC_COMMAND_BASE = 0x8000000,
	IPC_STARTUP,
	IPC_ABORT,
	IPC_QUIT,
	IPC_ACTIVATE,
	IPC_HELLO,
	IPC_GOODBYE,
	IPC_HIDE,
	IPC_SHOW,
	IPC_RESET,
	IPC_HELP,
	IPC_NEW,
	IPC_GOT_GOODBYE,
	IPC_IDENTIFY,
	IPC_PRIORITY,
	IPC_REMOVE,
	IPC_OPEN,
};

#define IPCSIG_HIDE SIGBREAKF_CTRL_D
#define IPCSIG_SHOW SIGBREAKF_CTRL_E
#define IPCSIG_QUIT SIGBREAKF_CTRL_F

// Requesters - Duplicated in Library/dopusbase.h
#if defined(__amigaos4__)
typedef VOID (*REF_CALLBACK)(REG(d0, ULONG iclass), REG(a0, APTR window), REG(a1, APTR data));
#elif defined(__MORPHOS__)
typedef VOID (*REF_CALLBACK)(VOID);
#elif defined(__AROS__)
typedef VOID (*REF_CALLBACK)(ULONG iclass, APTR window, APTR data);
#else
// typedef VOID (*REF_CALLBACK)(REG(d0, ULONG iclass), REG(a0, APTR window), REG(a1, APTR data));
typedef VOID (*REF_CALLBACK)(ULONG iclass __asm("d0"), APTR window __asm("a0"), APTR data __asm("a1"));
#endif

#if defined(__MORPHOS__)
	#define REFCALL(callback, iclass, window, data)       \
		({                                                \
			REG_D0 = (ULONG)iclass;                       \
			REG_A0 = (ULONG)window;                       \
			REG_A1 = (ULONG)data;                         \
			(*MyEmulHandle->EmulCallDirect68k)(callback); \
		})
#else
	#define REFCALL(callback, iclass, window, data) callback(iclass, window, data)
#endif

#if defined(__MORPHOS__)
	#define GET_CALLBACK(name) &name##_trap
	#define REF_CALLBACK_PROTO(ret, name, r1, t1, n1, r2, t2, n2, r3, t3, n3) \
		extern ret name(t1 n1, t2 n2, t3 n3);                                 \
		extern struct EmulLibEntry name##_trap
	#define REF_CALLBACK_BEGIN(ret, name, r1, t1, n1, r2, t2, n2, r3, t3, n3, ...)                   \
		__VA_ARGS__ ret name(t1 n1, t2 n2, t3 n3);                                                   \
		static ret name##stubs(void) { return (ret)name((t1)REG_##r1, (t2)REG_##r2, (t3)REG_##r2); } \
		struct EmulLibEntry name##_trap = {TRAP_LIB, 0, (APTR)&name##stubs};                         \
		__VA_ARGS__ ret name(REG(r1, t1 n1), REG(r2, t2 n2), REG(r3, t3 n3))
#else
	#define GET_CALLBACK(name) ((REF_CALLBACK)(name))
	#define REF_CALLBACK_PROTO(ret, name, r1, t1, n1, r2, t2, n2, r3, t3, n3) \
		extern ret name(REG(r1, t1 n1), REG(r2, t2 n2), REG(r3, t3 n3))
	#define REF_CALLBACK_BEGIN(ret, name, r1, t1, n1, r2, t2, n2, r3, t3, n3, ...) \
		__VA_ARGS__ ret ASM SAVEDS name(REG(r1, t1 n1), REG(r2, t2 n2), REG(r3, t3 n3))
#endif

enum {
	REQTYPE_FILE,
	REQTYPE_SIMPLE,
};

#define AR_Requester TAG_USER + 1  // Pointer to requester

#define AR_Window TAG_USER + 2		// Window
#define AR_Screen TAG_USER + 3		// Screen
#define AR_Message TAG_USER + 4		// Text message
#define AR_Button TAG_USER + 5		// Button label
#define AR_ButtonCode TAG_USER + 6	// Code for this button
#define AR_Title TAG_USER + 7		// Title string
#define AR_Buffer TAG_USER + 8		// String buffer
#define AR_BufLen TAG_USER + 9		// Buffer length
#define AR_History TAG_USER + 10	// History list
#define AR_CheckMark TAG_USER + 11	// Check mark text
#define AR_CheckPtr TAG_USER + 12	// Check mark data storage
#define AR_Flags TAG_USER + 13		// Flags
#define AR_Buffer2 TAG_USER + 14	// String buffer 2
#define AR_BufLen2 TAG_USER + 15	// Buffer length 2

// Pop-up menu stuff
typedef struct
{
	struct MinNode node;
	IPTR item_name;	  // Menu item name or locale string ID
	UWORD id;		  // Menu ID
	UWORD flags;	  // Menu item flags
	APTR data;		  // Menu item data
	APTR image;		  // Menu item image
	APTR userdata;	  // Menu item user data
} PopUpItem;

#define POPUPF_LOCALE (1 << 0)	  // Item name is a locale ID
#define POPUPF_CHECKIT (1 << 1)	  // Item can be checked
#define POPUPF_CHECKED (1 << 2)	  // Item is checked
#define POPUPF_SUB (1 << 3)		  // Item has sub-items
#define POPUPF_DISABLED (1 << 4)  // Item is disabled
#define POPUPF_STRING (1 << 5)
#define POPUPF_IMAGE (1 << 6)	  // Image is supplied
#define POPUPF_USERDATA (1 << 7)  // UserData is present

#define POPUP_BARLABEL (IPTR)-1

#define POPUP_HELPFLAG (1 << 15)  // Set if help key pressed

typedef struct
{
	struct MinList item_list;	 // List of menu items
	struct DOpusLocale *locale;	 // Locale data
	ULONG flags;				 // Flags
	IPTR userdata;				 // User data
	REF_CALLBACK callback;		 // Refresh callback
	struct TextFont *font;		 // Font to use
	struct Hook *backfill;		 // Backfill hook
} PopUpMenu;

#define POPUPMF_HELP (1 << 0)	   // Supports help
#define POPUPMF_REFRESH (1 << 1)   // Use refresh callback
#define POPUPMF_ABOVE (1 << 2)	   // Open above parent window
#define POPUPMF_STICKY (1 << 3)	   // Go sticky immediately
#define POPUPMF_FONT (1 << 4)	   // Use font
#define POPUPMF_NO_SCALE (1 << 5)  // No image scaling
#define POPUPMF_BACKFILL (1 << 6)  // Backfill hook is valid
#define POPUPMF_USE_PEN (1 << 7)   // Use pen in flags
#define POPUPMF_PEN(x) ((x) << 24)

#define POPUP_HELPFLAG (1 << 15)

typedef struct _PopUpHandle
{
	PopUpMenu ph_Menu;
	APTR ph_Memory;
	ULONG ph_Flags;
	struct List *ph_List;
	struct MinList ph_SubStack;
} PopUpHandle;

typedef struct
{
	struct MinNode ss_Node;
	struct List *ss_List;
} SubStack;

#define POPHF_SEP (1 << 0)

struct ListLock
{
	struct List list;
	struct SignalSemaphore lock;
};

#define SEMF_SHARED 0
#define SEMF_EXCLUSIVE (1 << 0)
#define SEMF_ATTEMPT (1 << 1)

// Image routine tags
#define IM_Width TAG_USER + 0		   // Width of image
#define IM_Height TAG_USER + 1		   // Height of image
#define IM_State TAG_USER + 2		   // 1 = selected, 0 = normal (default)
#define IM_Rectangle TAG_USER + 3	   // Rectangle to center within
#define IM_Mask TAG_USER + 4		   // 1 = mask image
#define IM_Depth TAG_USER + 5		   // Depth of image
#define IM_ClipBoundary TAG_USER + 6   // Clip boundary size
#define IM_Erase TAG_USER + 7		   // Erase background
#define IM_NoDrawInvalid TAG_USER + 8  // Don't draw if image is invalid
#define IM_NoIconRemap TAG_USER + 9	   // Don't remap icons
#define IM_Scale TAG_USER + 10		   // Scale image to IM_Rectangle
#define IM_Backfill TAG_USER + 11

/* SimpleRequest structure */

struct DOpusSimpleRequest
{
	char *title;			// Title
	char *message;			// Text to display
	char **gadgets;			// Gadget names
	int *return_values;		// Gadget return values
	char *string_buffer;	// Buffer for string gadget
	int string_len;			// Maximum string length
	int flags;				// Flags, see below
	struct TextFont *font;	// Font to use
	IPCData *ipc;			// IPC port to listen to
	Att_List *history;		// History list
	char *check_text;		// Text for checkmark
	short *check_ptr;		// Result of checkmark
	APTR filereq;			// File requester
	char *string_buffer_2;
	int string_len_2;
};

#define SRF_LONGINT (1 << 0)		// Integer gadget
#define SRF_CENTJUST (1 << 1)		// Center justify
#define SRF_RIGHTJUST (1 << 2)		// Right justify
#define SRF_SCREEN_PARENT (1 << 5)	// Parent is a screen
#define SRF_IPC (1 << 6)			// Listen to IPC port
#define SRF_SIGNAL (1 << 7)			// Wait for a signal
#define SRF_HISTORY (1 << 8)		// History supplied
#define SRF_PATH_FILTER (1 << 9)	// Filter path characters
#define SRF_BUFFER (1 << 10)		// Buffer supplied (not really used)
#define SRF_CHECKMARK (1 << 11)		// Checkmark supplied
#define SRF_SECURE (1 << 12)		// Secure field
#define SRF_MOUSE_POS (1 << 13)		// Position over mouse pointer
#define SRF_FILEREQ (1 << 14)		// File requester supplied
#define SRF_BUFFER2 (1 << 15)		// Second string buffer

// SelectionList

#define SLF_DIR_FIELD (1 << 0)	 // Directory field
#define SLF_FILE_FIELD (1 << 1)	 // File field, set SLF_DIR_FIELD as well
#define SLF_USE_INITIAL (1 << 2)
#define SLF_SWITCHES (1 << 3)	  // Switches
#define SLF_RETURN_PATH (1 << 4)  // Return if path is changed

// Disk IO stuff
typedef struct
{
	struct MsgPort *dh_port;			   // Message port
	struct IOExtTD *dh_io;				   // IO request
	struct FileSysStartupMsg *dh_startup;  // Startup message
	struct DosEnvec *dh_geo;			   // Disk geometry
	char dh_name[32];					   // Disk name
	char dh_device[32];					   // Device name
	struct InfoData dh_info;			   // Disk information
	unsigned long dh_result;
	unsigned long dh_root;		 // Root block
	unsigned long dh_blocksize;	 // Block size
	struct DateStamp dh_stamp;	 // DateStamp
} DiskHandle;

#define ID_AFS_PRO (0x41465301L)	   // AFS\01 - Ami-FileSafe1
#define ID_AFS_USER (0x41465302L)	   // AFS\02 - Ami-FileSafe2
#define ID_AFS_MULTI (0x6D754146L)	   // muAFS  - AFS multiuser
#define ID_PFS_FLOPPY (0x50465300L)	   // PFS\00 - floppyPFS
#define ID_PFS_HARD (0x50465301L)	   // PFS\01 - PFS1
#define ID_PFS2_DISK (0x50465302L)	   // PFS\02 - PFS2
#define ID_PFS3_DISK (0x50465303L)	   // PFS\03 - PFS3
#define ID_PFS2_SC_DISK (0x50445302L)  // PDS\02 - PFS2 Direct SCSI
#define ID_PFS3_SC_DISK (0x50445303L)  // PDS\03 - PFS3 Direct SCSI
#define ID_PFS3_MULTI (0x6d755046L)	   // muPF   - PFS3 multiuser
#define ID_ENVOY_DISK (0x444F5380L)
#define ID_FFS7_DISK (0x444f5307L)	 // FFS\07 - FastFileSystem7 : Long filenames
#define ID_VP255_DISK (0x444F5309L)	 // DOS\09 - OS4's V53 dostype is for newer vector-port based filesystems
#define ID_SFS0_DISK (0x53465300L)	 // SFS\00 - SmartFileSystem0
#define ID_SFS1_DISK (0x53465301L)	 // SFS\01 - SmartFileSystem1
#define ID_SFS2_DISK (0x53465302L)	 // SFS\02 - SmartFileSystem2
#define ID_SFS3_DISK (0x53465303L)	 // SFS\03 - SmartFileSystem3
#define ID_JXFS_DISK (0x4a584604L)	 // JXFS\4 - JXFileSystem
#ifndef ID_BOX0_DISK
	#define ID_BOX0_DISK (0x424F5800L)	// BOX\0  - BoxFileSystem
#endif
#ifndef ID_FAT32_DISK
	#define ID_FAT32_DISK (0x46415432L)	 // FAT2   - Fat32
#endif
#ifndef ID_EXFAT_DISK
	#define ID_EXFAT_DISK (0x46415458L)	 // FATX   - exFATFileSystem
#endif
#ifndef ID_EXT2_DISK
	#define ID_EXT2_DISK (0x45585402L)	// EXT\2  - FuseExt2FileSystem
#endif
#ifndef ID_HFS_DISK
	#define ID_HFS_DISK (0x48465300L)  // HFS\0  - FuseHFS
#endif
#ifndef ID_NTFS_DISK
	#define ID_NTFS_DISK (0x4e544653L)	// NTFS   - NTFileSystem3G
#endif
#define ID_SWAP_DISK (0x53574150L)	// SWAP   - OS4's SWAP

#define EDITF_NO_SELECT_NEXT (1 << 0)  // Don't select next field
#define EDITF_PATH_FILTER (1 << 1)	   // Filter path characters
#define EDITF_PASSWORD (1 << 2)		   // Hidden password field

// Copy protection (sic)
typedef struct
{
	long serial_number;		 // Actual serial number (Xor)
	long random_key;		 // Random key (xxxx)
	char serial_check_1[6];	 // Check #1 (AAAAA)
	char serial_check_2[5];	 // Check #2 (yyyy)
	char serial_check_3;	 // Check #3 (c)
} serial_data;

// Screen Info

#define SCRI_LORES (1 << 0)

// Images
typedef struct
{
	struct Screen *ir_Screen;
	unsigned short *ir_PenArray;
	short ir_PenCount;
	unsigned long ir_Flags;
} ImageRemap;

#define IRF_REMAP_COL0 (1 << 0)
#define IRF_PRECISION_EXACT (1 << 1)
#define IRF_PRECISION_ICON (1 << 2)
#define IRF_PRECISION_GUI (1 << 3)

typedef struct
{
	UWORD *oi_ImageData;
	ULONG *oi_Palette;
	short oi_Width;
	short oi_Height;
	short oi_Depth;
} OpenImageInfo;

/*** IFF ***/

#define IFF_READ MODE_OLDFILE
#define IFF_WRITE MODE_NEWFILE
#define IFF_CLIP 0x8000
#define IFF_CLIP_READ (IFF_CLIP | IFF_READ)
#define IFF_CLIP_WRITE (IFF_CLIP | IFF_WRITE)
#define IFF_SAFE 0x4000
#define IFF_ASYNC 0x2000

#define READCFG_FAIL 0
#define READCFG_OK 1
#define READCFG_WRONG 2

#define SCROLL_NOIDCMP (1 << 0)
#define SCROLL_VERT (1 << 1)
#define SCROLL_HORIZ (1 << 2)

#define LAUNCH_REPLY -1
#define LAUNCH_NOWAIT 0
#define LAUNCH_WAIT 1
#define LAUNCH_WAIT_TIMEOUT 2

#define LAUNCHF_USE_STACK (1 << 14)

#define LAUNCHF_OPEN_UNDER_MOUSE (1 << 30)

#define ERROR_NOT_CONFIG -1

#define RECTWIDTH(rect) (1 + (rect)->MaxX - (rect)->MinX)
#define RECTHEIGHT(rect) (1 + (rect)->MaxY - (rect)->MinY)

enum {
	GAD_xxx = 1,

	GAD_VERT_SCROLLER,
	GAD_VERT_ARROW_UP,
	GAD_VERT_ARROW_DOWN,

	GAD_HORIZ_SCROLLER,
	GAD_HORIZ_ARROW_LEFT,
	GAD_HORIZ_ARROW_RIGHT,
};

// Workbench patch stuff
enum { APP_ICON, APP_WINDOW, APP_MENU };

typedef struct
{
	struct MinNode node;
	ULONG type;	 // Type of entry

	IPTR id;		 // ID
	IPTR userdata;	 // User data

	APTR object;		   // Type-specific object
	char *text;			   // If text is needed
	struct MsgPort *port;  // Message port

	APTR os_object;	 // Object from OS routine

	unsigned short flags;  // Some flags

	struct MinList menu;  // Menu items

	unsigned short data;
	long menu_id_base;	// Base ID for menu
} AppEntry;

#define DAE_Local TAG_USER + 0			// Local entry
#define DAE_SnapShot TAG_USER + 1		// Supports snapshot
#define DAE_Menu TAG_USER + 2			// Menu item
#define DAE_Close TAG_USER + 3			// Close item
#define DAE_Background TAG_USER + 4		// Background colour
#define DAE_ToggleMenu TAG_USER + 5		// Toggle item
#define DAE_ToggleMenuSel TAG_USER + 6	// Toggle item (selected)
#define DAE_Info TAG_USER + 7			// Supports Information
#define DAE_Locked TAG_USER + 8			// Position locked
#define DAE_MenuBase TAG_USER + 9		// Menu ID base
#define DAE_Special TAG_USER + 10		// Special

#define ln_MenuFlags ln_Type

#define MNF_TOGGLE (1 << 0)
#define MNF_SEL (1 << 1)

#define APPENTF_LOCAL (1 << 0)
#define APPENTF_SNAPSHOT (1 << 1)
#define APPENTF_ICON_COPY (1 << 2)
#define APPENTF_CLOSE (1 << 3)
#define APPENTF_NO_OPEN (1 << 4)
#define APPENTF_BACKGROUND (1 << 5)
#define APPENTF_INFO (1 << 6)
#define APPENTF_LOCKED (1 << 7)
#define APPENTF_BUSY (1 << 8)
#define APPENTF_SPECIAL (1 << 9)
#define APPENTF_GHOSTED (1 << 10)

#define MTYPE_APPSNAPSHOT 0x3812
#define MTYPE_APPICONMENU 0x3813

struct AppSnapshotMsg
{
	struct AppMessage ap_msg;  // Message
	long position_x;		   // Icon x-position
	long position_y;		   // Icon y-position
	struct IBox window_pos;	   // Window position
	unsigned long flags;	   // Flags
	long id;				   // ID
};

#define APPSNAPF_UNSNAPSHOT (1 << 0)  // Set "no position"
#define APPSNAPF_WINDOW_POS (1 << 1)  // Window position supplied
#define APPSNAPF_MENU (1 << 2)		  // Menu operation
#define APPSNAPF_CLOSE (1 << 3)		  // Close command
#define APPSNAPF_HELP (1 << 4)		  // Help on a command
#define APPSNAPF_INFO (1 << 5)		  // Information command

#define ICONF_POSITION_OK (1 << 31)	 // Opus position stored in icon
#define ICONF_BORDER_ON (1 << 30)	 // Icon border on
#define ICONF_ICON_VIEW (1 << 29)	 // View as icons
#define ICONF_BORDER_OFF (1 << 28)	 // Icon border off
#define ICONF_NO_LABEL (1 << 27)	 // No label

#define DOCF_COPYALL (1 << 0)  // Copy tooltypes, etc
#define DOCF_NOIMAGE (1 << 1)  // Don't copy image data

#define GCDOF_NOCACHE (1 << 28)
#define GCDOFN_REAL_ICON (1 << 0)

#define CAIF_RENDER (1 << 0)
#define CAIF_SELECT (1 << 1)
#define CAIF_TITLE (1 << 2)
#define CAIF_LOCKED (1 << 3)
#define CAIF_SET (1 << 4)
#define CAIF_BUSY (1 << 5)
#define CAIF_UNBUSY (1 << 6)
#define CAIF_GHOST (1 << 7)

#define MTYPE_DOPUS (UWORD) - 1

typedef struct _DOpusAppMessage
{
	struct AppMessage da_Msg;
	Point *da_DropPos;
	Point da_DragOffset;
	ULONG da_Flags;
	IPTR da_Custom[4];
} DOpusAppMessage;

#define DAPPF_ICON_DROP (1 << 16)  // Dropped with icon

#define PW_Screen TAG_USER + 0		 // Screen to open on
#define PW_Window TAG_USER + 1		 // Owner window
#define PW_Title TAG_USER + 2		 // Window title
#define PW_SigTask TAG_USER + 3		 // Task to signal
#define PW_SigBit TAG_USER + 4		 // Signal bit
#define PW_Flags TAG_USER + 5		 // Flags
#define PW_FileName TAG_USER + 6	 // File name
#define PW_FileSize TAG_USER + 7	 // File size
#define PW_FileDone TAG_USER + 8	 // File done
#define PW_FileCount TAG_USER + 9	 // Number of files
#define PW_FileNum TAG_USER + 10	 // Current number
#define PW_Info TAG_USER + 11		 // Information line
#define PW_Info2 TAG_USER + 12		 // Information line
#define PW_Info3 TAG_USER + 13		 // Information line
#define PW_FileInc TAG_USER + 14	 // Increment file number
#define PW_FileSize64 TAG_USER + 15	 // 64bit file size
#define PW_FileDone64 TAG_USER + 16	 // 64bit File done

#define PWF_FILENAME (1 << 0)	// Filename display
#define PWF_FILESIZE (1 << 1)	// Filesize display
#define PWF_INFO (1 << 2)		// Information line
#define PWF_GRAPH (1 << 3)		// Bar graph display
#define PWF_NOABORT (1 << 4)	// No abort gadget
#define PWF_INVISIBLE (1 << 5)	// Open invisibly
#define PWF_ABORT (1 << 6)		// Want abort gadget
#define PWF_SWAP (1 << 7)		// Swap bar and size displays
#define PWF_DEBUG (1 << 8)		// Debug stuff
#define PWF_NOIPC (1 << 9)		// No IPC from sender
#define PWF_INFO2 (1 << 10)		// Information line
#define PWF_INFO3 (1 << 11)		// Information line

// Search Flags
#define SEARCH_NOCASE (1 << 0)	   // Not case sensitive
#define SEARCH_WILDCARD (1 << 1)   // Wildcards
#define SEARCH_ONLYWORDS (1 << 2)  // Only match whole words

#define RANGE_BETWEEN 1
#define RANGE_AFTER 2
#define RANGE_WEIRD -1

#define LIBDF_NO_CACHING (1 << 1)
#define LIBDF_NOSTIPPLE (1 << 2)
#define LIBDF_DOS_PATCH (1 << 3)
#define LIBDF_3DLOOK (1 << 4)
#define LIBDF_FT_CACHE (1 << 5)
#define LIBDF_REDIRECT_TOOLS (1 << 6)
#define LIBDF_BORDERS_OFF (1 << 7)
#define LIBDF_NO_CUSTOM_DRAG (1 << 8)
#define LIBDF_THIN_BORDERS (1 << 9)
#define LIBDF_USING_OS35 (1 << 10)	// gjp

// Args
typedef struct
{
	struct RDArgs *FA_RDArgs;	  // RDArgs structure
	struct RDArgs *FA_RDArgsRes;  // Return from ReadArgs()
	char *FA_ArgString;			  // Copy of argument string (with newline)
	IPTR *FA_ArgArray;			  // Argument array pointer
	IPTR *FA_Arguments;			  // Argument array you should use
	short FA_Count;				  // Number of arguments
	short FA_DoneArgs;			  // DOpus uses this flag for its own purposes
} FuncArgs;

#define RexxTag_Arg0 (TAG_USER + 0x1)
#define RexxTag_Arg1 (TAG_USER + 0x2)
#define RexxTag_Arg2 (TAG_USER + 0x3)
#define RexxTag_Arg3 (TAG_USER + 0x4)
#define RexxTag_Arg4 (TAG_USER + 0x5)
#define RexxTag_Arg5 (TAG_USER + 0x6)
#define RexxTag_Arg6 (TAG_USER + 0x7)
#define RexxTag_Arg7 (TAG_USER + 0x8)
#define RexxTag_Arg8 (TAG_USER + 0x9)
#define RexxTag_Arg9 (TAG_USER + 0xa)
#define RexxTag_Arg10 (TAG_USER + 0xb)
#define RexxTag_Arg11 (TAG_USER + 0xc)
#define RexxTag_Arg12 (TAG_USER + 0xd)
#define RexxTag_Arg13 (TAG_USER + 0xe)
#define RexxTag_Arg14 (TAG_USER + 0xf)
#define RexxTag_Arg15 (TAG_USER + 0x10)
#define RexxTag_VarName (TAG_USER + 0x11)
#define RexxTag_VarValue (TAG_USER + 0x12)

// doslist

typedef struct
{
	struct Node dle_Node;
	struct DosList dle_DosList;
	char dle_DeviceName[40];
	short dle_DeviceUnit;
} DosListEntry;

#ifndef MTYPE_APPWINDOW
	#define MTYPE_APPWINDOW 7
#endif
#ifndef MTYPE_APPICON
	#define MTYPE_APPICON 8
#endif
#ifndef MTYPE_APPMENUITEM
	#define MTYPE_APPMENUITEM 9
#endif

#define PAGEUP 0x3f
#define PAGEDOWN 0x1f
#define HOME 0x3d
#define END 0x1d

struct NewIconDiskObject
{
	struct DiskObject nido_DiskObject;
	struct NewDiskObject *nido_NewDiskObject;
	short nido_Flags;
};

#define NIDOF_REMAPPED (1 << 0)

enum { ICON_NORMAL, ICON_CACHED, ICON_NEWICON };

typedef struct
{
	ULONG cs_Red;
	ULONG cs_Green;
	ULONG cs_Blue;
} ColourSpec32;

struct BackFillInfo
{
	struct Layer *layer;
	struct Rectangle bounds;
	LONG offsetx;
	LONG offsety;
};

struct DOpusSemaphore
{
	struct SignalSemaphore sem;
	APTR main_ipc;
	struct ListLock modules;
};

#define STATID_TASKCOUNT 0
#define STATID_CPU_USAGE 1

typedef struct
{
	struct Node node;
	struct IBox pos;
	ULONG flags;
	ULONG icon_pos_x;
	ULONG icon_pos_y;
} ButtonBankNode;

typedef struct
{
	struct Node node;
	APTR *lister;
} OpenListerNode;

#define OEDF_ENVR (1 << 0)
#define OEDF_SETS (1 << 1)
#define OEDF_TBAR (1 << 2)
#define OEDF_LMEN (1 << 3)
#define OEDF_UMEN (1 << 4)
#define OEDF_SCRP (1 << 5)
#define OEDF_HKEY (1 << 6)
#define OEDF_BANK (1 << 7)
#define OEDF_LSTR (1 << 8)
#define OEDF_DESK (1 << 9)
#define OEDF_PATH (1 << 10)
#define OEDF_SNDX (1 << 11)
#define OEDF_ALL 0xffffffff

// SETS - Settings
typedef struct
{
	ULONG copy_flags;		// Copy flags
	ULONG delete_flags;		// Delete flags
	ULONG error_flags;		// Error flags
	ULONG general_flags;	// General flags
	ULONG icon_flags;		// Icon flags
	unsigned short replace_method;	// Replace method
	unsigned short replace_flags;	// Replace flags
	ULONG update_flags;		// Update flags
	ULONG dir_flags;		// Directory flags
	ULONG view_flags;		// View flags
	unsigned char hide_method;		// Hide method
	unsigned char pad1;
	unsigned short pad2;
	unsigned short max_buffer_count;  // Maximum buffer count
	unsigned short date_format;		  // Date format
	unsigned short date_flags;		  // Date flags
	char pri_main[2];				  // Main priority
	char pri_lister[2];				  // Lister priority
	ULONG flags;
	UWORD pop_code;
	UWORD pop_qual;
	UWORD pop_qual_mask;
	UWORD pop_qual_same;
	short popup_delay;
	short max_openwith;
	short command_line_length;
	short max_filename;
	ULONG pad[16];
} CFG_SETS;

// Sort format
typedef struct
{
	BYTE sort;		  // Sort method
	BYTE sort_flags;  // Sort flags
	BYTE separation;  // File separation
} SortFormat;

// List format
typedef struct
{
	UBYTE files_unsel[2];  // Unselected files
	UBYTE files_sel[2];	   // Selected files
	UBYTE dirs_unsel[2];   // Unselected directories
	UBYTE dirs_sel[2];	   // Selected directories

	SortFormat sort;	   // Sort method
	BYTE display_pos[16];  // Item display position
	BYTE display_len[15];  // Display length

	UBYTE flags;

	BYTE show_free;			// Show free space type
	char show_pattern[40];	// Show pattern
	char hide_pattern[40];	// Hide pattern

	char show_pattern_p[82];  // Show pattern parsed
	char hide_pattern_p[82];  // Hide pattern parsed
} ListFormat;

// List format storage structure
typedef struct
{
	UBYTE files_unsel[2];  // Unselected files
	UBYTE files_sel[2];	   // Selected files
	UBYTE dirs_unsel[2];   // Unselected directories
	UBYTE dirs_sel[2];	   // Selected directories

	SortFormat sort;	   // Sort method
	BYTE display_pos[16];  // Item display position
	BYTE display_len[15];  // Display length

	UBYTE flags;

	BYTE show_free;			// Show free space type
	char show_pattern[40];	// Show pattern
	char hide_pattern[40];	// Hide pattern
} ListFormatStorage;

// LSTR - defines a lister
typedef struct
{
	struct IBox pos[2];	 // Window position
	ULONG icon_x;		 // Icon position
	ULONG icon_y;
	ListFormat format;	// Lister format
	ULONG flags;		// Flags
	ULONG pad[4];

	// Followed by path to load
} CFG_LSTR;

typedef struct
{
	struct IBox pos;	// Window position
	ListFormat format;	// Lister format
	ULONG flags;		// Flags

	// Followed by path to load
} CFG_DLST;

#define CUST_PENS 16

// Lister definition
typedef struct
{
	struct Node node;
	CFG_LSTR lister;  // Lister definition
	char *path;		  // Path to load
} Cfg_Lister;

// THIS STRUCTURE IS DUPLICATED IN Program/dopus_config.h
// ENVR - defines the screen and other display info
typedef struct
{
	ULONG screen_mode;			 // Screen mode
	UWORD screen_flags;			 // Screen flags
	UWORD screen_depth;			 // Screen depth
	UWORD screen_width;			 // Screen width
	UWORD screen_height;		 // Screen height
	ULONG palette[50];			 // Screen palette
	char pubscreen_name[80];	 // Public screen name to open on
	struct IBox window_pos;		 // Window dimensions
	ULONG general_screen_flags;	 // General screen flags

	UBYTE source_col[2];  // Source window colours
	UBYTE dest_col[2];	  // Destination window colours

	WORD palette_count;	 // Number of user colours
	UWORD pad1;

	char font_name[4][80];	// Fonts to use

	ULONG env_Colours[CUST_PENS][2][3];	 // Custom colours
	ULONG env_ColourFlag;				 // Which custom colours are in use

	ULONG env_NewIconsFlags;  // NewIcons flags

	ULONG pad3[2];

	char status_text[80];  // Status bar text

	UBYTE font_size[4];	 // Font sizes

	char pad4[12];

	UWORD display_options;	 // Display options
	UWORD main_window_type;	 // Main window type

	ULONG hotkey_flags;	 // Hotkey flags
	UWORD hotkey_code;	 // Hotkey code
	UWORD hotkey_qual;	 // Hotkey qualifier

	ListFormat list_format;	 // Default list format

	char backdrop_prefs[80];  // Backdrop pattern prefs file

	char desktop_location[240];	 // Desktop folder location

	char output_window[80];	 // Output window
	char output_device[80];	 // Output device

	ULONG default_stack;  // Default stack size

	/*
		char		scr_title_text[120];	// Screen title text

		ULONG		pad6[17];
	*/

	char scr_title_text[188];  // Screen title text

	UWORD lister_options;  // Lister options

	ULONG flags;  // Misc. flags

	UWORD lister_popup_code;  // Lister popup code
	UWORD lister_popup_qual;  // Lister popup qualifier

	ULONG env_flags;  // Some other flags

	unsigned short clock_left;	// Clock left position
	unsigned short clock_top;	// Clock top position

	UBYTE devices_col[2];  // Source window colours
	UBYTE volumes_col[2];  // Destination window colours

	UWORD lister_width;	 // Default lister sizes
	UWORD lister_height;

	short version;

	UBYTE gauge_col[2];	 // Free space gauge

	unsigned char icon_fpen;
	unsigned char icon_bpen;
	unsigned char icon_style;

	char env_NewIconsPrecision;

	ULONG desktop_flags;

	unsigned char iconw_fpen;
	unsigned char iconw_bpen;
	unsigned char iconw_style;

	unsigned char desktop_popup_default;

	// Promoted from ENV: vars in CONFIG_VERSION_14 (issue #49). Steals 8 of
	// pad[17]'s 68 reserved bytes so on-disk layout stays compatible.
	// IMPORTANT: if you ever grow CFG_ENVR past its current 72-byte footprint
	// you must memset(0) the destination before L_IFFReadChunkBytes in
	// config_open.c -- the IFF reader does NOT zero-fill the trailing buffer
	// when the on-disk chunk is shorter than sizeof(CFG_ENVR), and the
	// version-gated migrations in config_default.c rely on the new fields
	// being zero in pre-v14 configs.
	UWORD env_icon_space_x;	 // Desktop icon X spacing (was dopus/IconSpaceX)
	UWORD env_icon_space_y;	 // Desktop icon Y spacing (was dopus/IconSpaceY)
	UWORD env_icon_grid_x;	 // Desktop icon X grid step, 1 = no grid
	UWORD env_icon_grid_y;	 // Desktop icon Y grid step, 1 = no grid

	// Promoted in CONFIG_VERSION_15.
	UWORD env_wheel_scroll_lines;  // Mouse wheel lines per notch (was dopus/WheelScrollLines)
	UWORD env_pad_15;			   // Pad to keep ULONG alignment

	// Promoted in CONFIG_VERSION_16. Steals 4 reserved bytes so the on-disk
	// layout after pad stays compatible.
	UBYTE tooltip_col[2];	// Lister toolbar tooltip colours
	UBYTE env_pad_16[2];	// Pad to keep ULONG alignment

	ULONG pad[13];	// Save recompilation

	char env_BackgroundPic[4][256];	 // Background pictures
	UWORD env_BackgroundFlags[4];	 // Background flags

	CFG_SETS settings;
	UWORD pad2;

	ULONG env_BackgroundBorderColour[4];

	char themes_location[256];	// Themes folder location

	// Promoted in CONFIG_VERSION_17. Steals 80 reserved bytes so the on-disk
	// layout stays compatible.
	char clock_format[80];  // Title bar clock format string

	ULONG pad_big[276];
} CFG_ENVR;

// Environment configuration
typedef struct
{
	APTR memory;				  // Memory handle
	struct SignalSemaphore lock;  // Lock

	CFG_ENVR *env;	 // Environment data
	char path[256];	 // Current environment path

	ULONG pad[65];

	struct List buttons;  // Button banks to open
	struct List listers;  // Listers to open

	struct MinList path_formats;  // Specific path formats

	APTR volatile_memory;	   // Volatile memory handle
	char toolbar_path[256];	   // Lister toolbar path
	APTR settings_memory;	   // Settings memory handle
	char menu_path[256];	   // Lister menu path
	char user_menu_path[256];  // User menu path
	char hotkeys_path[256];	   // Hotkeys path
	char scripts_path[256];	   // Scripts path

	struct MinList desktop;	 // Desktop information
	APTR desktop_memory;
	struct SignalSemaphore desktop_lock;

	struct MinList iconpos;	 // Icon areas
	struct List startmenus;	 // StartMenus to open

	struct MinList path_list;  // Path list

	struct MinList sound_list;	// Sounds
	struct SignalSemaphore sound_lock;
} Cfg_Environment;

struct OpenEnvironmentData
{
	APTR memory;
	APTR volatile_memory;
	ULONG flags;

	CFG_ENVR env;
	struct MinList desktop;
	struct MinList pathlist;
	struct MinList soundlist;
	struct MinList startmenus;
	struct MinList buttons;
	struct MinList listers;
	char toolbar_path[256];
	char menu_path[256];
	char user_menu_path[256];
	char scripts_path[256];
	char hotkeys_path[256];
};

struct AllocBitmapPatchNode
{
	struct MinNode abp_Node;
	struct Screen *abp_Screen;
	struct Task *abp_Task;
};

#ifndef __amigaos3__
	#pragma pack()
#endif

/*****************************************************************************

 Directory Opus Direct Hooks

 *****************************************************************************/

// Values for dc_FileSet/dc_FileQuery

#define HFFS_NAME (TAG_USER + 0x1)		  // char * 		- Entry name
#define HFFS_SIZE (TAG_USER + 0x2)		  // ULONG 		- Entry size
#define HFFS_PROTECTION (TAG_USER + 0x3)  // ULONG 		- Protection flags
#define HFFS_DATE (TAG_USER + 0x4)		  // struct Datestamp *	- Entry date
#define HFFS_COMMENT (TAG_USER + 0x5)	  // char *		- Comment
#define HFFS_SELECTED (TAG_USER + 0x6)	  // BOOL			- Selected state
#define HFFS_LINK (TAG_USER + 0x7)		  // BOOL			- Set if a link
#define HFFS_COLOUR (TAG_USER + 0x8)	  // ULONG		- 1 = device, 2 = assign
#define HFFS_USERDATA (TAG_USER + 0x9)	  // IPTR		- Userdata
#define HFFS_FILETYPE (TAG_USER + 0xa)	  // char *		- Filetype description
#define HFFS_DISPLAY (TAG_USER + 0xb)	  // char *		- Custom display string
#define HFFS_VERSION (TAG_USER + 0xc)	  // VersionInfo *	- Version information
#define HFFS_MENU (TAG_USER + 0xd)		  // struct List *	- Custom menus for entry
#define HFFS_ICON (TAG_USER + 0xe)		  // struct DiskObject *	- not implemented

// Additional file requester flags
#define DCRFF_PATTERN (1 << 29)
#define DCRFF_SCREENPARENT (1 << 30)
#define DCRFF_REJECTICONS (1 << 31)

// Flags for dc_RefreshLister()
#define HOOKREFRESH_DATE (1 << 0)
#define HOOKREFRESH_FULL (1 << 1)

// Type value for dc_ExamineEntry()
enum {
	EE_NAME,  // Get pointer to name (READ ONLY!)
	EE_TYPE,  // Get type (<0 = file, >0 = dir)
};

// Flags for dc_ReplaceReq()
#define REPREQF_NOVERSION (1 << 16)	 // No 'version' button

#ifndef __amigaos3__
	#pragma pack(2)
#endif

// Sort format
struct SortFormat
{
	BYTE sf_Sort;		 // Sort method
	BYTE sf_SortFlags;	 // Sort flags
	BYTE sf_Separation;	 // File separation
};

// List format
struct ListFormat
{
	// Colour fields, not used at present
	UBYTE lf_FilesUnsel[2];	 // Unselected files
	UBYTE lf_FilesSel[2];	 // Selected files
	UBYTE lf_DirsUnsel[2];	 // Unselected directories
	UBYTE lf_DirsSel[2];	 // Selected directories

	// Sort information
	struct SortFormat lf_Sort;	// Sort method
	BYTE lf_DisplayPos[16];		// Item display position
	BYTE lf_DisplayLen[15];		// Display length (not used)

	UBYTE lf_Flags;	 // See LFORMATF_xxx below

	// Not used
	BYTE lf_ShowFree;  // Show free space type

	// You must call ParsePattern() yourself
	char lf_ShowPattern[40];   // Show pattern
	char lf_HidePattern[40];   // Hide pattern
	char lf_ShowPatternP[82];  // Show pattern parsed
	char lf_HidePatternP[82];  // Hide pattern parsed
};

#define LFORMATF_REJECT_ICONS (1 << 0)	// Reject icons
#define LFORMATF_HIDDEN_BIT (1 << 1)	// Respect the H bit
#define LFORMATF_ICON_VIEW (1 << 2)		// Default to icon view
#define LFORMATF_SHOW_ALL (1 << 3)		// Show all

// Used for the dc_GetPointer/dc_FreePointer hooks
struct GetPointerPkt
{
	ULONG gpp_Type;
	APTR gpp_Ptr;
	ULONG gpp_Flags;
};

#define GETPTR_HANDLE 2	   // Get a function handle
#define GETPTR_COMMANDS 4  // Get internal command list

#ifndef DEF_DOPUSCOMMANDLIST
	#define DEF_DOPUSCOMMANDLIST

struct DOpusCommandList
{
	struct Node dcl_Node;	// Command name in ln_Name
	ULONG dcl_Flags;		// Function flags
	char *dcl_Description;	// Description string
	char *dcl_Template;		// Command template
	char *dcl_Module;		// Module command resides in
	char *dcl_Help;			// Help file for command
};

#endif

// Use this command to get the address of the hooks from the Opus process.
// Send it to the main Opus IPC, and supply the address of a DOpusCallbackInfo
// structure in the data field of the message.

#define HOOKCMD_GET_CALLBACKS 0x40100

#define DOPUS_HOOK_COUNT 52

// This structure will grow in the future, correct use of the dc_Count field
// ensures innocent memory won't get overwritten.

typedef struct
{
	// Set to the number of hooks you want
	UWORD dc_Count;

	// Create a file entry
	APTR ASM (*dc_CreateFileEntry)(REG(a0, APTR lister), REG(a1, struct FileInfoBlock *fib), REG(d0, BPTR lock));

	// Change parameters of a file entry
	VOID ASM (*dc_FileSet)(REG(a0, APTR lister), REG(a1, APTR entry), REG(a2, struct TagItem *tags));

	// Sort list of entries into a lister
	VOID ASM (*dc_SortFileList)(REG(a0, APTR lister),
								REG(a1, struct List *list),
								REG(d0, long file_count),
								REG(d1, long dir_count));

	// Add single entry to a lister
	APTR ASM (*dc_AddFileEntry)(REG(a0, APTR lister), REG(a1, APTR entry), REG(d0, BOOL sort));

	// Resort a lister
	VOID ASM (*dc_ResortLister)(REG(a0, APTR lister), REG(a1, struct ListFormat *format));

	// Refresh a lister
	VOID ASM (*dc_RefreshLister)(REG(a0, APTR lister), REG(d0, ULONG flags));

	// Lock lister file list
	VOID ASM (*dc_LockFileList)(REG(a0, APTR lister), REG(d0, BOOL exclusive));

	// Unlock file list
	VOID ASM (*dc_UnlockFileList)(REG(a0, APTR lister));

	// Find entry in a lister by name
	APTR ASM (*dc_FindFileEntry)(REG(a0, APTR lister), REG(a1, char *name));

	// Change comment of an entry
	BOOL ASM (*dc_SetFileComment)(REG(a0, APTR lister), REG(a1, char *name), REG(a2, char *comment));

	// Remove file entry from a lister
	VOID ASM (*dc_RemoveFileEntry)(REG(a0, APTR lister), REG(a1, APTR entry));

	// Query file entry
	BOOL ASM (*dc_FileQuery)(REG(a0, APTR lister), REG(a1, APTR entry), REG(a2, struct TagItem *tags));

	// Show help
	void ASM (*dc_ShowHelp)(REG(a0, char *file_name), REG(a1, char *node_name));

	// Convert entry pointer from one type to another
	APTR ASM (*dc_ConvertEntry)(REG(a0, APTR entry));

	// Get lister pointer from a path handle
	APTR ASM (*dc_GetLister)(REG(a0, APTR path));

	// Get first source lister
	APTR ASM (*dc_GetSource)(REG(a0, APTR handle), REG(a1, char *path));

	// Get next source lister
	APTR ASM (*dc_NextSource)(REG(a0, APTR handle), REG(a1, char *path));

	// Unlock source listers
	void ASM (*dc_UnlockSource)(REG(a0, APTR handle));

	// Get next destination lister
	APTR ASM (*dc_GetDest)(REG(a0, APTR handle), REG(a1, char *path));

	// End use of source lister
	void ASM (*dc_EndSource)(REG(a0, APTR handle), REG(d0, long complete));

	// End use of destination lister
	void ASM (*dc_EndDest)(REG(a0, APTR handle), REG(d0, long complete));

	// Get next selected entry
	APTR ASM (*dc_GetEntry)(REG(a0, APTR handle));

	// Examine an entry
	IPTR ASM (*dc_ExamineEntry)(REG(a0, APTR entry), REG(d0, long type));

	// End use of an entry
	void ASM (*dc_EndEntry)(REG(a0, APTR handle), REG(a1, APTR entry), REG(d0, BOOL deselect));

	// Mark an entry for removal
	void ASM (*dc_RemoveEntry)(REG(a0, APTR entry));

	// Return count of selected entries
	long ASM (*dc_EntryCount)(REG(a0, APTR handle));

	// Mark an entry to be reloaded
	void ASM (*dc_ReloadEntry)(REG(a0, APTR handle), REG(a1, APTR entry));

	// Add a file to a lister
	void ASM (*dc_AddFile)(REG(a0, APTR handle),
						   REG(a1, char *path),
						   REG(a2, struct FileInfoBlock *fib),
						   REG(a3, APTR lister));

	// Delete a file from a lister
	void ASM (*dc_DelFile)(REG(a0, APTR handle), REG(a1, char *path), REG(a2, char *name), REG(a3, APTR lister));

	// Load/reload a file in a lister
	void ASM (*dc_LoadFile)(REG(a0, APTR handle),
							REG(a1, char *path),
							REG(a2, char *name),
							REG(d0, long flags),
							REG(d1, BOOL reload));

	// Perform changes on a lister
	void ASM (*dc_DoChanges)(REG(a0, APTR handle));

	// Check for user abort
	BOOL ASM (*dc_CheckAbort)(REG(a0, APTR handle));

	// Get window pointer from a path handle
	struct Window *ASM (*dc_GetWindow)(REG(a0, APTR path));

	// Get Opus ARexx port name
	struct MsgPort *ASM (*dc_GetPort)(REG(a0, char *name));

	// Get Opus public screen name
	struct Screen *ASM (*dc_GetScreen)(REG(a0, char *name));

	// Get information about the Opus screen
	struct DOpusScreenData *ASM (*dc_GetScreenData)(void);

	// Free screen data structure
	void ASM (*dc_FreeScreenData)(void);

	// Open progress indicator in a lister
	void ASM (*dc_OpenProgress)(REG(a0, APTR path), REG(a1, char *operation), REG(d0, long total));

	// Update progress indicator
	void ASM (*dc_UpdateProgress)(REG(a0, APTR path), REG(a1, char *name), REG(d0, long count));

	// Close progress indicator
	void ASM (*dc_CloseProgress)(REG(a0, APTR path));

	// Show 'File exists - Replace?' requester
	long ASM (*dc_ReplaceReq)(REG(a0, struct Window *window),
							  REG(a1, struct Screen *screen),
							  REG(a2, IPCData *ipc),
							  REG(a3, struct FileInfoBlock *file1),
							  REG(a4, struct FileInfoBlock *file2),
							  REG(d0, long flags));

	// Get pointer to Opus internal things
	APTR ASM (*dc_GetPointer)(REG(a0, struct GetPointerPkt *));
	void ASM (*dc_FreePointer)(REG(a0, struct GetPointerPkt *));

	// Send an ARexx command direct to Opus
	ULONG ASM (*dc_SendCommand)(REG(a0, APTR handle),
								REG(a1, char *command),
								REG(a2, char **result),
								REG(d0, ULONG flags));

	// Make DOpus check if the desktop needs updating
	void ASM (*dc_CheckDesktop)(REG(a0, char *path));

	// Get desktop path
	short ASM (*dc_GetDesktop)(REG(a0, char *buffer));

	// Run script
	short ASM (*dc_Script)(REG(a0, char *name), REG(a1, char *data));

	// Popup desktop popup
	short ASM (*dc_DesktopPopup)(REG(a0, ULONG flags));

	// Reset file entries
	void ASM (*dc_FirstEntry)(REG(a0, APTR handle));

	// Send rexx command direct to DOpus
	long ASM (*dc_RexxCommand)(REG(a0, char *command),
							   REG(a1, char *result),
							   REG(d0, long length),
							   REG(a2, struct MsgPort *replyport),
							   REG(d1, ULONG flags));

	// Show file requester
	long ASM (*dc_FileRequest)(REG(a0, struct Window *parent),
							   REG(a1, char *title),
							   REG(a2, char *initial_path),
							   REG(a3, char *pathname),
							   REG(d0, ULONG flags),
							   REG(d1, char *pattern));

	// Get themes path
	void ASM (*dc_GetThemes)(REG(a0, char *buffer));

} DOpusCallbackInfo;

#ifndef __amigaos3__
	#pragma pack()
#endif

#if defined(__MORPHOS__)

	#define DC_REGA0 REG_A0
	#define DC_REGA1 REG_A0
	#define DC_REGA2 REG_A0
	#define DC_REGA3 REG_A0
	#define DC_REGA4 REG_A0
	#define DC_REGD0 REG_A0
	#define DC_REGD1 REG_D1
	#define DC_REGD2 REG_D2

	#define DC_CALL1(dc, func, t1, v1)                    \
		({                                                \
			t1 = (ULONG)v1;                               \
			(*MyEmulHandle->EmulCallDirect68k)(dc->func); \
		})

	#define DC_CALL2(dc, func, t1, v1, t2, v2)            \
		({                                                \
			t1 = (ULONG)v1;                               \
			t2 = (ULONG)v2;                               \
			(*MyEmulHandle->EmulCallDirect68k)(dc->func); \
		})

	#define DC_CALL3(dc, func, t1, v1, t2, v2, t3, v3)    \
		({                                                \
			t1 = (ULONG)v1;                               \
			t2 = (ULONG)v2;                               \
			t3 = (ULONG)v3;                               \
			(*MyEmulHandle->EmulCallDirect68k)(dc->func); \
		})

	#define DC_CALL4(dc, func, t1, v1, t2, v2, t3, v3, t4, v4) \
		({                                                     \
			t1 = (ULONG)v1;                                    \
			t2 = (ULONG)v2;                                    \
			t3 = (ULONG)v3;                                    \
			t4 = (ULONG)v4;                                    \
			(*MyEmulHandle->EmulCallDirect68k)(dc->func);      \
		})

	#define DC_CALL5(dc, func, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5) \
		({                                                             \
			t1 = (ULONG)v1;                                            \
			t2 = (ULONG)v2;                                            \
			t3 = (ULONG)v3;                                            \
			t4 = (ULONG)v4;                                            \
			t5 = (ULONG)v5;                                            \
			(*MyEmulHandle->EmulCallDirect68k)(dc->func);              \
		})

	#define DC_CALL6(dc, func, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6) \
		({                                                                     \
			t1 = (ULONG)v1;                                                    \
			t2 = (ULONG)v2;                                                    \
			t3 = (ULONG)v3;                                                    \
			t4 = (ULONG)v4;                                                    \
			t5 = (ULONG)v5;                                                    \
			t6 = (ULONG)v6;                                                    \
			(*MyEmulHandle->EmulCallDirect68k)(dc->func);                      \
		})

#else

	#define DC_REGA0
	#define DC_REGA1
	#define DC_REGA2
	#define DC_REGA3
	#define DC_REGA4
	#define DC_REGD0
	#define DC_REGD1
	#define DC_REGD2

	#define DC_CALL1(dc, func, t1, v1) dc->func(v1)

	#define DC_CALL2(dc, func, t1, v1, t2, v2) dc->func(v1, v2)

	#define DC_CALL3(dc, func, t1, v1, t2, v2, t3, v3) dc->func(v1, v2, v3)

	#define DC_CALL4(dc, func, t1, v1, t2, v2, t3, v3, t4, v4) dc->func(v1, v2, v3, v4)

	#define DC_CALL5(dc, func, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5) dc->func(v1, v2, v3, v4, v5)

	#define DC_CALL6(dc, func, t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6) dc->func(v1, v2, v3, v4, v5, v6)

#endif

/*********************************************************************/

// All supported Amiga flavours (OS3/OS4/MOS/AROS) now compile IPC entry
// points and startup callbacks as direct functions in the host architecture
// (PPC on MOS/OS4, m68k on OS3, native on AROS). The old MorphOS-specific
// EmulLibEntry/TRAP_LIBNR trampolines — which launched child processes as
// m68k and let the kernel trap back to PPC — don't reliably survive modern
// MorphOS process startup and caused the 5.94 MorphOS build to hang partway
// through the init progress bar (first observable at step 7, the ARexx
// handler launch). Using native functions everywhere requires L_IPC_Launch
// to set NP_CodeType=CODETYPE_PPC on MorphOS (handled in ipc.c) and
// CallStartupCode to invoke the callback directly instead of via
// EmulCallDirect68k.
#define IPC_StartupCode(name, t2, n2, ...) ULONG __VA_ARGS__ ASM SAVEDS name(REG(a0, IPCData *ipc), REG(a1, t2 n2))
#define IPC_EntryCode(name, ...) __VA_ARGS__ VOID SAVEDS name()
#define IPC_EntryProto(name, ...) __VA_ARGS__ VOID SAVEDS name(void)

/*********************************************************************/

// 64-bit types QUAD and UQUAD
#ifdef __amigaos4__
typedef uint64 UQUAD;
typedef int64 QUAD;
#elif defined(__amigaos3__)
typedef unsigned long long UQUAD;
typedef signed long long QUAD;
#endif

// 64-bit FIB
#if defined(__MORPHOS__) || defined(__AROS__)
typedef struct FileInfoBlock FileInfoBlock64;
#else
// added a _s suffix, since AROS might use struct FileInfoBlock64 in the future
typedef struct FileInfoBlock64_s
{
	LONG fib_DiskKey;
	LONG fib_DirEntryType;
	char fib_FileName[108];
	LONG fib_Protection;
	/*LONG             fib_EntryType;
	LONG             fib_Size;*/
	UQUAD fib_Size64;
	LONG fib_NumBlocks;
	struct DateStamp fib_Date;
	char fib_Comment[80];
	UWORD fib_OwnerUID;
	UWORD fib_OwnerGID;
	char fib_Reserved[32];
} FileInfoBlock64;
#endif

// Returns file size from FIB pointer
#ifdef USE_64BIT
	#ifdef __AROS__
		#define GETFIBSIZE(fib) (((FileInfoBlock64 *)(fib))->fib_Size)
	#else
		#define GETFIBSIZE(fib) (((FileInfoBlock64 *)(fib))->fib_Size64)
	#endif
#else
	#define GETFIBSIZE(fib) ((fib)->fib_Size)
#endif

#endif
