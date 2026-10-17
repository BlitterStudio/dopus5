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

#ifndef _DOPUSBASE_H
#define _DOPUSBASE_H

/*
 *
 * dopus5.library header file
 *
 * Support library for Directory Opus version 5
 *
 * (c) Copyright 1994,5 Jonathan Potter
 *
 */

#include "amiga.h"
#include <Program/dopus_config.h>

/*
 *
 * ** DEFINES ** *
 *
 */

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

#define OBJECTF_NO_SELECT_NEXT (1 << 0)	 // Don't select next field
#define OBJECTF_PATH_FILTER (1 << 1)	 // Filter path characters
#define OBJECTF_SECURE (1 << 2)			 // Hide string
#define OBJECTF_INTEGER (1 << 3)		 // Integer gadget
#define OBJECTF_READ_ONLY (1 << 4)		 // Read-only
#define OBJECTF_HOTKEY (1 << 5)			 // Hotkey string
#define OBJECTF_UPPERCASE (1 << 6)		 // Uppercase string

#define MENUFLAG_TEXT_STRING (1 << 16)	// Menu name is a real string
#define MENUFLAG_COMM_SEQ (1 << 17)		// Give menu a command sequence
#define MENUFLAG_AUTO_MUTEX (1 << 18)	// Automatic mutual exclusion
#define MENUFLAG_USE_SEQ (1 << 19)		// Use command sequence supplied

#define MENUFLAG_MAKE_SEQ(c) ((ULONG)(c) << 24)
#define MENUFLAG_GET_SEQ(fl) ((char)((fl) >> 24))

#define NM_NEXT 10
#define NM_BAR_LABEL (IPTR) NM_BARLABEL

#define IS_GADTOOLS(obj) (BOOL)(obj->gl_info.gl_gadget.context != NULL)

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

#define FILE_GLASS_KIND 1000
#define DIR_GLASS_KIND 1001

#define GM_RESIZE (20)

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

#define WDF_VALID (1 << 0)
#define WDF_ZOOMED (1 << 1)

#define LISTF_LOCK (1 << 0)
#define LISTF_POOL (1 << 1)

#define ADDNODE_SORT 1		 // Sort names
#define ADDNODE_EXCLUSIVE 2	 // Exclusive entry
#define ADDNODE_NUMSORT 4	 // Numerical name sort
#define ADDNODE_PRI 8		 // Priority insertion

#define REMLIST_FREEDATA 1	// FreeVec data when freeing list
#define REMLIST_SAVELIST 2	// Don't free list itself
#define REMLIST_FREEMEMH 4	// FreeMemH data when freeing list

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

#define PORT_ASYNC_MAGIC 0x5A  // original dopus5code: #define PF_ASYNC	(1<<7)

#define REPLY_NO_PORT (struct MsgPort *)-1
#define REPLY_NO_PORT_IPC (struct MsgPort *)-2
#define NO_PORT_IPC (struct MsgPort *)-3

#define IPCF_INVALID (1 << 0)
#define IPCF_LISTED (1 << 1)
#define IPCF_STARTED (1 << 2)

#define IPCDATA(ipc) ((APTR)ipc->userdata)
#define SET_IPCDATA(ipc, data) ipc->userdata = (APTR)data

#define IPCF_GETPATH (1 << 31)
#define IPCF_DEBUG (1 << 30)
#define IPCM_STACK(s) (s & 0xffffff)

#if defined(__MORPHOS__)
	#define IPCF_NATIVE (0x1)
#else
	#define IPCF_NATIVE (0x0)
#endif

#define IPC_GET_ENTRY(entry) ((IPTR)(entry) & ~(IPTR)(IPCF_NATIVE))
#define IPC_GET_CODETYPE(entry) ((IPTR)(entry) & (IPTR)(IPCF_NATIVE))

#define IPC_NATIVE(entry) ((IPTR)&entry | (IPTR)(IPCF_NATIVE))

#define IPCSIG_HIDE SIGBREAKF_CTRL_D
#define IPCSIG_SHOW SIGBREAKF_CTRL_E
#define IPCSIG_QUIT SIGBREAKF_CTRL_F

// Requesters - Duplicated from Include/libraries/dopus5.h
#if defined(__MORPHOS__)
typedef VOID (*REF_CALLBACK)(VOID);
#else
typedef VOID ASM (*REF_CALLBACK)(REG(d0, ULONG iclass), REG(a0, APTR window), REG(a1, APTR data));
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

#define POPUPF_LOCALE (1 << 0)	  // Item name is a locale ID
#define POPUPF_CHECKIT (1 << 1)	  // Item can be checked
#define POPUPF_CHECKED (1 << 2)	  // Item is checked
#define POPUPF_SUB (1 << 3)		  // Item has sub-items
#define POPUPF_DISABLED (1 << 4)  // Item is disabled
#define POPUPF_STRING (1 << 5)
#define POPUPF_IMAGE (1 << 6)	  // Image is supplied
#define POPUPF_USERDATA (1 << 7)  // UserData is present

#define POPUP_BARLABEL (IPTR)-1

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

#define POPHF_SEP (1 << 0)

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

#define EDITF_NO_SELECT_NEXT (1 << 0)  // Don't select next field
#define EDITF_PATH_FILTER (1 << 1)	   // Filter path characters
#define EDITF_PASSWORD (1 << 2)		   // Hidden password field

// Screen Info
#define SCRI_LORES (1 << 0)

#define ID_AFS_PRO 0x41465301
#define ID_AFS_USER 0x41465302
#define ID_AFS_MULTI 0x6D754146
#define ID_PFS_FLOPPY 0x50465300
#define ID_PFS_HARD 0x50465301
#define ID_ENVOY_DISK 0x444F5380

// Images
#define IRF_REMAP_COL0 (1 << 0)
#define IRF_PRECISION_EXACT (1 << 1)
#define IRF_PRECISION_ICON (1 << 2)
#define IRF_PRECISION_GUI (1 << 3)

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
#define LIBDF_NO_PATCHES (1 << 11)	// No system patches

// REXX stuff
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

#define NIDOF_REMAPPED (1 << 0)

#define STATID_TASKCOUNT 0
#define STATID_CPU_USAGE 1

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

/*
 *
 * ** structures ** *
 *
 */

#ifndef __amigaos3__
	#pragma pack(2)
#endif

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

typedef struct _MenuData
{
	UBYTE type;	  // Menu type
	IPTR id;	  // Menu ID or user data
	IPTR name;	  // Menu name
	ULONG flags;  // Menu flags
} MenuData;

typedef struct
{
	ULONG magic;			   // Magic ID
	struct Window *window;	   // Pointer back to window
	ULONG window_id;		   // User window ID
	struct MsgPort *app_port;  // "Window's" application port
} WindowID;

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

typedef struct
{
	struct RastPort *rp;
	struct DrawInfo *drawinfo;
	struct Node *node;
	unsigned short line;
	struct IBox box;
} ListViewDraw;

// Window dimensions
typedef struct
{
	struct IBox wd_Normal;
	struct IBox wd_Zoomed;
	unsigned short wd_Flags;
} WindowDimensions;

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

typedef struct _Att_Node
{
	struct Node node;  // Node structure
	Att_List *list;	   // Pointer to list (inefficient!)
	IPTR data;		   // User data
} Att_Node;

struct ListLock
{
	struct List list;
	struct SignalSemaphore lock;
};

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

typedef struct
{
	struct MinList item_list;	 // List of menu items
	struct DOpusLocale *locale;	 // Locale data
	ULONG flags;				 // Flags
	IPTR userdata;				 // User data
	REF_CALLBACK(callback);		 // Refresh callback
	struct TextFont *font;		 // Font to use
	struct Hook *backfill;		 // Backfill hook
} PopUpMenu;

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

// Copy protection (sic)
typedef struct
{
	long serial_number;		 // Actual serial number (Xor)
	long random_key;		 // Random key (xxxx)
	char serial_check_1[6];	 // Check #1 (AAAAA)
	char serial_check_2[5];	 // Check #2 (yyyy)
	char serial_check_3;	 // Check #3 (c)
} serial_data;

// Images
typedef struct
{
	struct Screen *ir_Screen;
	unsigned short *ir_PenArray;
	short ir_PenCount;
	unsigned long ir_Flags;
} ImageRemap;

typedef struct
{
	UWORD *oi_ImageData;
	ULONG *oi_Palette;
	short oi_Width;
	short oi_Height;
	short oi_Depth;
} OpenImageInfo;

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

	struct Hook *render_hook;  // WBAPPICONA_RenderHook from AddAppIconA()
} AppEntry;

struct AppSnapshotMsg
{
	struct AppMessage ap_msg;  // Message
	long position_x;		   // Icon x-position
	long position_y;		   // Icon y-position
	struct IBox window_pos;	   // Window position
	unsigned long flags;	   // Flags
	long id;				   // ID
};

typedef struct _DOpusAppMessage
{
	struct AppMessage da_Msg;
	Point *da_DropPos;
	Point da_DragOffset;
	ULONG da_Flags;
	IPTR da_Custom[4];
} DOpusAppMessage;

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

// doslist
typedef struct
{
	struct Node dle_Node;
	struct DosList dle_DosList;
	char dle_DeviceName[40];
	short dle_DeviceUnit;
} DosListEntry;

struct NewIconDiskObject
{
	struct DiskObject nido_DiskObject;
	struct NewDiskObject *nido_NewDiskObject;
	short nido_Flags;
};

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

/*
 *
 * ** ENUMS ** *
 *
 */

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

enum {
	REQTYPE_FILE,
	REQTYPE_SIMPLE,
};

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

enum { ICON_NORMAL, ICON_CACHED, ICON_NEWICON };

/*
 *
 * ** PROTOS ** *
 *
 */

DragInfo *GetDragInfo(struct Window *, struct RastPort *, long, long, long);
void FreeDragInfo(DragInfo *);
void GetDragImage(DragInfo *, ULONG, ULONG);
void GetDragMask(DragInfo *);
void AddDragImage(DragInfo *);
void RemDragImage(DragInfo *);
void ShowDragImage(DragInfo *, ULONG, ULONG);
void HideDragImage(DragInfo *);
void StampDragImage(DragInfo *, ULONG, ULONG);
BOOL CheckDragDeadlock(DragInfo *);
BOOL BuildTransDragMask(UWORD *, UWORD *, short, short, short, long);
BOOL InitDragDBuf(DragInfo *);
void DrawDragList(struct RastPort *, struct ViewPort *, long);
void RemoveDragImage(DragInfo *);
BOOL DragCustomOk(struct BitMap *);

void InitWindowDims(struct Window *, WindowDimensions *);
void StoreWindowDims(struct Window *, WindowDimensions *);
BOOL CheckWindowDims(struct Window *, WindowDimensions *);

Att_List *Att_NewList(ULONG);
Att_Node *Att_NewNode(Att_List *list, char *name, IPTR data, ULONG flags);
void Att_RemNode(Att_Node *node);
void Att_PosNode(Att_List *, Att_Node *, Att_Node *);
void Att_RemList(Att_List *list, long);
Att_Node *Att_FindNode(Att_List *list, long number);
long Att_NodeNumber(Att_List *list, char *name);
Att_Node *Att_FindNodeData(Att_List *list, IPTR data);
long Att_NodeDataNumber(Att_List *list, IPTR data);
char *Att_NodeName(Att_List *list, long number);
long Att_NodeCount(Att_List *list);
void Att_ChangeNodeName(Att_Node *node, char *name);
long Att_FindNodeNumber(Att_List *list, Att_Node *node);
void AddSorted(struct List *list, struct Node *node);
struct Node *FindNameI(struct List *, char *name);
void LockAttList(Att_List *list, short exclusive);
void UnlockAttList(Att_List *list);
void SwapListNodes(struct List *, struct Node *, struct Node *);
BOOL IsListLockEmpty(struct ListLock *);

TimerHandle *AllocTimer(ULONG, struct MsgPort *);
void FreeTimer(TimerHandle *);
void StartTimer(TimerHandle *, ULONG, ULONG);
BOOL CheckTimer(TimerHandle *);
void StopTimer(TimerHandle *);
BOOL TimerActive(TimerHandle *);
struct Library *GetTimerBase(void);

APTR AddNotifyRequest(ULONG, ULONG, struct MsgPort *);
void RemoveNotifyRequest(APTR);
#ifndef __amigaos4__
void SendNotifyMsg(ULONG, IPTR, ULONG, short, char *, struct FileInfoBlock *);
#endif
void SetNotifyRequest(APTR, ULONG, ULONG);

long AsyncRequest(IPCData *, long, struct Window *, REF_CALLBACK name, APTR, struct TagItem *);
struct IntuiMessage *CheckRefreshMsg(struct Window *, ULONG);

UWORD DoPopUpMenu(struct Window *, PopUpMenu *, PopUpItem **, UWORD);
PopUpItem *GetPopUpItem(PopUpMenu *, UWORD);
void GetPopUpImageSize(struct Window *, PopUpMenu *, short *, short *);
void SetPopUpDelay(short);

PopUpHandle *PopUpNewHandle(IPTR, REF_CALLBACK name, struct DOpusLocale *);
void PopUpFreeHandle(PopUpHandle *);
PopUpItem *PopUpNewItem(PopUpHandle *, IPTR, ULONG, ULONG);
void PopUpSeparator(PopUpHandle *);
BOOL PopUpItemSub(PopUpHandle *, PopUpItem *);
void PopUpEndSub(PopUpHandle *);
ULONG PopUpSetFlags(PopUpMenu *, UWORD, ULONG, ULONG);

long GetSemaphore(struct SignalSemaphore *, long, char *);
void FreeSemaphore(struct SignalSemaphore *);
void ShowSemaphore(struct SignalSemaphore *);
void InitListLock(struct ListLock *, char *);

DiskHandle *OpenDisk(char *, struct MsgPort *);
void CloseDisk(DiskHandle *);

// String edit hook
struct Hook *GetEditHook(ULONG, ULONG, struct TagItem *tags);
struct Hook *STDARGS GetEditHookTags(ULONG, ULONG, Tag, ...);
void FreeEditHook(struct Hook *);
char *GetSecureString(struct Gadget *);

// Screen Info
ULONG ScreenInfo(struct Screen *);

// Images
APTR OpenImage(char *name, OpenImageInfo *);
void CloseImage(APTR image);
APTR CopyImage(APTR image);
void FlushImages(void);
short RenderImage(struct RastPort *, APTR, UWORD, UWORD, struct TagItem *tags);
short RenderImageTags(struct RastPort *, APTR, UWORD, UWORD, Tag, ...);
void GetImageAttrs(APTR image, struct TagItem *tags);
ULONG *GetImagePalette(APTR image);
void FreeImageRemap(ImageRemap *);
BOOL RemapImage(APTR, struct Screen *, ImageRemap *);
void FreeRemapImage(APTR, ImageRemap *);

/*** IFF ***/
APTR IFFOpen(char *, unsigned short, ULONG);
void IFFClose(APTR);
long IFFPushChunk(APTR, ULONG);
long IFFWriteChunkBytes(APTR, APTR, long);
long IFFPopChunk(APTR);
long IFFWriteChunk(APTR, APTR, ULONG, ULONG);
ULONG IFFNextChunk(APTR, ULONG);
long IFFChunkSize(APTR);
long IFFReadChunkBytes(APTR, APTR, long);
APTR IFFFileHandle(APTR);
long IFFChunkRemain(APTR);
ULONG IFFChunkID(APTR);
ULONG IFFGetFORM(APTR);
void IFFFailure(APTR);

/* Function prototypes */

void ActivateStrGad(struct Gadget *, struct Window *);
ULONG Atoh(char *, short);
void Itoa(long, char *, char);
void ItoaU(unsigned long, char *, char);
void Ito26(unsigned long, char *);
void BytesToString(unsigned long, char *, short, char);
void DivideToString(char *, unsigned long, unsigned long, short, char);

ULONG CompareListFormat(ListFormat *, ListFormat *);

void BtoCStr(BSTR, char *, int);

#ifndef __amigaos4__
short DoSimpleRequest(struct Window *, struct DOpusSimpleRequest *);
#endif
short SimpleRequest(struct Window *, char *, char *, char *, char *, APTR, long, ULONG);
short SimpleRequestTags(struct Window *, char *, char *, char *, ...);
short SimpleRequestScreenTags(struct Screen *, char *, char *, char *, ...);
short SelectionList(Att_List *,
					struct Window *,
					struct Screen *,
					char *,
					short,
					ULONG,
					char *,
					char *,
					char *,
					char **,
					ULONG *);

void Seed(int);
UWORD RandomDopus(int);
void BuildKeyString(unsigned short, unsigned short, unsigned short, unsigned short, char *);
UWORD QualValid(unsigned short);
BOOL ConvertRawKey(unsigned short, unsigned short, char *);
void SetBusyPointer(struct Window *);
BOOL StrCombine(char *, char *, char *, int);
BOOL StrConcat(char *, char *, int);
BPTR GetDosPathList(BPTR);
void FreeDosPathList(BPTR);
void CopyLocalEnv(struct Library *);
void UpdatePathList(void);
#ifndef __amigaos4__
void UpdateMyPaths(void);
#endif
BPTR GetOpusPathList(void);

// Layout functions
struct Window *OpenConfigWindow(NewConfigWindow *);
void CloseConfigWindow(struct Window *);
struct IntuiMessage *GetWindowMsg(struct MsgPort *);
void ReplyWindowMsg(struct IntuiMessage *);
ObjectList *AddObjectList(struct Window *, ObjectDef *);
void FreeObject(ObjectList *, GL_Object *);
void FreeObjectList(ObjectList *);
GL_Object *GetObject(ObjectList *, int);
void StoreGadgetValue(ObjectList *, struct IntuiMessage *);
void UpdateGadgetValue(ObjectList *, struct IntuiMessage *, UWORD);
void UpdateGadgetList(ObjectList *);
void SetGadgetValue(ObjectList *, UWORD, IPTR);
IPTR GetGadgetValue(ObjectList *, UWORD);
void SetGadgetChoices(ObjectList *list, ULONG id, APTR choices);
BOOL CheckObjectArea(GL_Object *, int, int);
void DisplayObject(struct Window *, GL_Object *, int fg, int bg, char *txt);
void StripIntuiMessagesDopus(struct Window *);
void StripWindowMessages(struct MsgPort *, struct IntuiMessage *);
void CloseWindowSafely(struct Window *);
void AddWindowMenus(struct Window *, MenuData *);
void FreeWindowMenus(struct Window *);
void SetWindowBusy(struct Window *);
void ClearWindowBusy(struct Window *);
STRPTR GetString(struct DOpusLocale *, LONG);
struct Gadget *FindKeyEquivalent(ObjectList *, struct IntuiMessage *, int);
void ShowProgressBar(struct Window *, GL_Object *, ULONG, ULONG);
void SetObjectKind(ObjectList *, ULONG, UWORD);
void DisableObject(ObjectList *, ULONG, BOOL);
int BoundsCheckGadget(ObjectList *, ULONG, int, int);
void RefreshObjectList(struct Window *, ObjectList *);
struct Menu *BuildMenuStrip(MenuData *, struct DOpusLocale *);
struct MenuItem *FindMenuItem(struct Menu *, UWORD);

void SetWindowID(struct Window *, WindowID *, ULONG, struct MsgPort *);
ULONG GetWindowID(struct Window *);
struct MsgPort *GetWindowAppPort(struct Window *);
BOOL GetObjectRect(ObjectList *, ULONG, struct Rectangle *);
void LayoutResize(struct Window *);
void SetConfigWindowLimits(struct Window *, ConfigWindow *, ConfigWindow *);

void StartRefreshConfigWindow(struct Window *, long);
void EndRefreshConfigWindow(struct Window *);

// Memory allocation
void *NewMemHandle(ULONG puddle_size, ULONG thresh_size, ULONG type);
void FreeMemHandle(void *handle);
void ClearMemHandle(void *handle);
#ifndef __amigaos4__
void *AllocMemH(void *handle, ULONG size);
#endif
void FreeMemH(void *memory);

// configuration

Cfg_Lister *NewLister(char *path);
Cfg_ButtonBank *NewButtonBank(BOOL, short);
Cfg_Button *NewButton(APTR memory);
Cfg_Button *NewButtonWithFunc(APTR memory, char *label, short type);
Cfg_Function *NewFunction(APTR memory, UWORD type);
Cfg_ButtonFunction *NewButtonFunction(APTR memory, UWORD type);
Cfg_Instruction *NewInstruction(APTR memory, short type, char *string);
Cfg_Filetype *NewFiletype(APTR memory);

short ReadSettings(CFG_SETS *, char *);
Cfg_Lister *ReadListerDef(APTR, ULONG);
Cfg_ButtonBank *OpenButtonBank(char *name);
Cfg_FiletypeList *ReadFiletypes(char *name, APTR memory);
Cfg_Button *ReadButton(APTR iff, APTR memory);
Cfg_Function *ReadFunction(APTR iff, APTR memory, struct List *func_list, Cfg_Function *function);
void ConvertStartMenu(Cfg_ButtonBank *);

void DefaultSettings(CFG_SETS *);
void DefaultEnvironment(CFG_ENVR *);
Cfg_ButtonBank *DefaultButtonBank(void);
int SaveFiletypeList(Cfg_FiletypeList *list, char *name);
int SaveSettings(CFG_SETS *, char *name);
long SaveListerDef(APTR, Cfg_Lister *lister);
short SaveButtonBank(Cfg_ButtonBank *bank, char *name);
short SaveButton(APTR, Cfg_Button *);
BOOL SaveFunction(APTR, Cfg_Function *);
void UpdateEnvironment(CFG_ENVR *);

void CloseButtonBank(Cfg_ButtonBank *bank);
void FreeListerDef(Cfg_Lister *lister);
void FreeButtonList(struct List *list);
void FreeButtonImages(struct List *list);
void FreeButton(Cfg_Button *button);
void FreeFunction(Cfg_Function *function);
void FreeButtonFunction(Cfg_ButtonFunction *function);
void FreeInstruction(Cfg_Instruction *ins);
void FreeInstructionList(Cfg_Function *func);
void FreeFiletypeList(Cfg_FiletypeList *list);
void FreeFiletype(Cfg_Filetype *type);

Cfg_ButtonBank *CopyButtonBank(Cfg_ButtonBank *orig);
Cfg_Button *CopyButton(Cfg_Button *orig, APTR, short);
Cfg_Filetype *CopyFiletype(Cfg_Filetype *, APTR);
Cfg_Function *CopyFunction(Cfg_Function *orig, APTR, Cfg_Function *new);
Cfg_ButtonFunction *CopyButtonFunction(Cfg_ButtonFunction *, APTR, Cfg_ButtonFunction *);

Cfg_Function *FindFunctionType(struct List *list, UWORD type);

// IPC
int IPC_Launch(struct ListLock *, IPCData **, char *, IPTR, ULONG, IPTR, struct Library *);
int IPC_Launch_Local(struct ListLock *, IPCData **, char *, IPTR, ULONG, IPTR, struct Library *);
int IPC_Startup(IPCData *, APTR, struct MsgPort *);
IPTR IPC_Command(IPCData *, ULONG, IPTR, APTR, APTR, struct MsgPort *);
IPTR IPC_SafeCommand(IPCData *, ULONG, IPTR, APTR, APTR, struct MsgPort *, struct ListLock *);
void IPC_Reply(IPCMessage *);
void IPC_Free(IPCData *);
void IPC_Flush(IPCData *);
	IPCData *IPC_FindProc(struct ListLock *, char *, BOOL, IPTR);
IPCData *IPC_ProcStartup(IPTR *, ULONG (*ASM)(REG(a0, IPCData *), REG(a1, APTR)));
void IPC_Quit(IPCData *, ULONG, BOOL);
void IPC_Hello(IPCData *, IPCData *);
void IPC_Goodbye(IPCData *, IPCData *, ULONG);
IPTR IPC_GetGoodbye(IPCMessage *);
ULONG IPC_ListQuit(struct ListLock *list, IPCData *owner, ULONG quit_flags, BOOL wait);
void IPC_ListCommand(struct ListLock *list, ULONG command, IPTR flags, IPTR data, BOOL wait);
void IPC_QuitName(struct ListLock *, char *, ULONG);

// gui
void DrawBox(struct RastPort *, struct Rectangle *, struct DrawInfo *, BOOL);
void DrawFieldBox(struct RastPort *, struct Rectangle *, struct DrawInfo *);
BOOL WriteIcon(char *, struct DiskObject *);
BOOL DeleteIcon(char *);

// status
struct Window *OpenStatusWindow(char *, char *, struct Screen *, LONG, ULONG);
void SetStatusText(struct Window *, char *);
void UpdateStatusGraph(struct Window *, char *, ULONG, ULONG);

// clipboard
struct ClipHandle *OpenClipBoard(ULONG);
void CloseClipBoard(struct ClipHandle *);
BOOL WriteClipString(struct ClipHandle *, char *, long);
long ReadClipString(struct ClipHandle *, char *, long);

// palette
void LoadPalette32(struct ViewPort *, unsigned long *);
void GetPalette32(struct ViewPort *, unsigned long *, unsigned short, short);

// buffered_io
APTR OpenBuf(char *name, long mode, long buffer_size);
long CloseBuf(APTR file);
long ReadBuf(APTR file, char *data, long size);
long WriteBuf(APTR file, char *data, long size);
long FlushBuf(APTR file);
long SeekBuf(APTR file, long offset, long mode);
long ExamineBuf(APTR file, struct FileInfoBlock *fib);
BPTR FHFromBuf(APTR file);
long ReadBufLine(APTR file, char *data, long size);

// boopsi
struct Gadget *AddScrollBars(struct Window *, struct List *, struct DrawInfo *, short);
struct Gadget *FindBOOPSIGadget(struct List *, UWORD);
void BOOPSIFree(struct List *);
struct Gadget *CreateTitleGadget(struct Screen *, struct List *, BOOL, short, short, unsigned short);
struct Gadget *FindGadgetType(struct Gadget *, UWORD);
void FixTitleGadgets(struct Window *);

ULONG DivideU(unsigned long, unsigned long, unsigned long *, struct Library *);
BOOL SerialValid(serial_data *);
BOOL SerialCheck(char *, ULONG *);
ULONG ChecksumFile(char *, ULONG);

void ReplyFreeMsg(APTR);
BOOL GetWBArgPath(struct WBArg *, char *, long);

void WriteFileIcon(char *, char *);

struct PubScreenNode *FindPubScreen(struct Screen *, BOOL);

BOOL WB_Launch(char *, struct Screen *, short);
BOOL WB_LaunchNew(char *, struct Screen *, short, long, char *);
BOOL WB_LaunchNotify(char *, struct Screen *, short, long, char *, struct Process **, IPCData *, ULONG);
BOOL CLI_Launch(char *, struct Screen *, BPTR, BPTR, BPTR, short, long);
void MUFSLogin(struct Window *, char *, char *);

// anim
void AnimDecodeRIFFXor(unsigned char *, char *, unsigned short, unsigned short);
void AnimDecodeRIFFSet(unsigned char *, char *, unsigned short, unsigned short);

// devices
struct DosList *DeviceFromLock(BPTR, char *);
#ifndef __amigaos4__
struct DosList *DeviceFromHandler(struct MsgPort *, char *);
#endif
BOOL DevNameFromLockDopus(BPTR, char *, long);
BOOL IsDiskDevice(struct MsgPort *);
BOOL GetDeviceUnit(BPTR, char *, short *);

// Workbench patch stuff
#ifndef __amigaos4__
void WB_Install_Patch(void);
#endif
BOOL WB_Remove_Patch(void);
struct AppWindow *WB_AddAppWindow(IPTR, IPTR, struct Window *, struct MsgPort *, struct TagItem *);
BOOL WB_RemoveAppWindow(struct AppWindow *);
struct AppWindow *WB_FindAppWindow(struct Window *);
struct MsgPort *WB_AppWindowData(struct AppWindow *, IPTR *, IPTR *);
BOOL WB_AppWindowLocal(struct AppWindow *);
struct Window *WB_AppWindowWindow(struct AppWindow *);
ULONG WB_AppIconFlags(struct AppIcon *);
APTR LockAppList(void);
APTR NextAppEntry(APTR, ULONG);
void UnlockAppList(void);

ULONG GetIconFlags(struct DiskObject *);
void SetIconFlags(struct DiskObject *, ULONG);
void GetIconPosition(struct DiskObject *, short *, short *);
void SetIconPosition(struct DiskObject *, short, short);

struct DiskObject *CopyDiskObject(struct DiskObject *, ULONG);
void FreeDiskObjectCopy(struct DiskObject *);

struct DiskObject *GetCachedDefDiskObject(long);
void FreeCachedDiskObject(struct DiskObject *);
struct DiskObject *GetCachedDiskObject(char *, long);
struct DiskObject *GetCachedDiskObjectNew(char *, ULONG);
unsigned long IconCheckSum(struct DiskObject *, short);
BOOL RemapIcon(struct DiskObject *, struct Screen *, short);
struct DiskObject *GetOriginalIcon(struct DiskObject *);

void ChangeAppIcon(APTR, struct Image *, struct Image *, char *, ULONG);
long SetAppIconMenuState(APTR, long, long);

DOpusAppMessage *AllocAppMessage(APTR, struct MsgPort *, short);
void FreeAppMessage(DOpusAppMessage *);
void ReplyAppMessage(DOpusAppMessage *);
BOOL CheckAppMessage(DOpusAppMessage *);
DOpusAppMessage *CopyAppMessage(DOpusAppMessage *, APTR);
BOOL SetWBArg(DOpusAppMessage *, short, BPTR, char *, APTR);

// Progress window
APTR OpenProgressWindow(struct TagItem *);
void CloseProgressWindow(APTR);
void HideProgressWindow(APTR);
void ShowProgressWindow(APTR, struct Screen *, struct Window *);
void SetProgressWindow(APTR, struct TagItem *);
void GetProgressWindow(APTR, struct TagItem *);
BOOL CheckProgressAbort(APTR);
long CalcPercent(ULONG, ULONG, struct Library *);

APTR STDARGS OpenProgressWindowTags(Tag, ...);
void STDARGS SetProgressWindowTags(APTR, Tag, ...);
void STDARGS GetProgressWindowTags(APTR, Tag, ...);

// Search
long SearchFile(APTR, UBYTE *, ULONG, UBYTE *, ULONG);

// Date routines
char *ParseDateStrings(char *, char *, char *, long *);
BOOL DateFromStrings(char *, char *, struct DateStamp *);
BOOL DateFromStringsNew(char *, char *, struct DateStamp *, ULONG);

// Filetype matching
APTR GetMatchHandle(char *);
void FreeMatchHandle(APTR);
BOOL MatchFiletype(APTR, APTR);
void ClearFiletypeCache(void);

// version
BOOL GetFileVersion(char *, short *, short *, struct DateStamp *, APTR);

// Set flags in library
ULONG SetLibraryFlags(ULONG, ULONG);
ULONG GetLibraryFlags(void);

// Bitmap
struct BitMap *NewBitMap(ULONG, ULONG, ULONG, ULONG, struct BitMap *);
void DisposeBitMap(struct BitMap *);

// args
FuncArgs *ParseArgs(char *, char *);
void DisposeArgs(FuncArgs *);

void SetEnv(char *, char *, BOOL);
BOOL SavePos(char *, struct IBox *, short);
BOOL LoadPos(char *, struct IBox *, short *);

// Original DOS functions
BPTR OriginalCreateDir(char *);
long OriginalDeleteFile(char *);
BOOL OriginalSetFileDate(char *, struct DateStamp *);
BOOL OriginalSetComment(char *, char *);
BOOL OriginalSetProtection(char *, ULONG);
BOOL OriginalRename(char *, char *);
BOOL OriginalRelabel(char *, char *);
BPTR OriginalOpen(char *, LONG);
BOOL OriginalClose(BPTR);
LONG OriginalWrite(BPTR, void *, LONG);

// REXX stuff
struct RexxMsg *CreateRexxMsgEx(struct MsgPort *, UBYTE *, UBYTE *);
void FreeRexxMsgEx(struct RexxMsg *);
long SetRexxVarEx(struct RexxMsg *, char *, char *, long);
long GetRexxVarEx(struct RexxMsg *, char *, char **);
struct RexxMsg *BuildRexxMsgEx(struct MsgPort *, UBYTE *, UBYTE *, struct TagItem *);
struct RexxMsg *BuildRexxMsgExTags(struct MsgPort *, UBYTE *, UBYTE *, Tag, ...);

// doslist
void NotifyDiskChange(void);
void GetDosListCopy(struct List *, APTR);
void FreeDosListCopy(struct List *);

short GetIconType(struct DiskObject *);
void SetNewIconsFlags(ULONG, short);
void SetReqBackFill(struct Hook *hook, struct Screen **);

long GetStatistics(long id);

BOOL OpenEnvironment(char *, struct OpenEnvironmentData *);

APTR AddAllocBitmapPatch(struct Task *, struct Screen *);
void RemAllocBitmapPatch(APTR);

#endif
