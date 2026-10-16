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

#ifndef _DOPUS_DISPLAY
#define _DOPUS_DISPLAY

#define NAME_CLOCK "dopus_clock"
#define NAME_HIDDEN_CLOCK "dopus_hidden_clock"
#define NAME_BUTTON_EDITOR "dopus_config_buttons"
#define NAME_BUTTON_EDITOR_RUN "dopus_config_buttons_run"
#define NAME_PATTERNS "dopus_pattern_stuff"

// Defines information about a GUI element
#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct
{
	BOOL visible;  // Indicates visibility

	struct IBox box;		// Dimensions in an IBox
	struct Rectangle rect;	// Dimensions in a Rectangle

	UWORD fg_pen;			// Foreground pen to use
	UWORD bg_pen;			// Background pen to use
	struct TextFont *font;	// Font to use

	struct RastPort rast;  // RastPort copy
} GUI_Element;

// Defines the entire display
typedef struct
{
	struct Window *window;	// Window pointer
	struct Screen *screen;	// Screen pointer

	struct DrawInfo *draw_info;	  // Screen DrawInfo
	APTR visual_info;			  // VisualInfo for the screen
	struct AppWindow *appwindow;  // AppWindow pointer

	UWORD pens[16];	  // Pen table
	UWORD pen_alloc;  // Allocation flags

#ifdef __amigaos3__
	UWORD magicwb_pen_alloc;  // MagicWB pens 4-7 we locked in place of Workbench
#endif

	struct Menu *menus;					// Menu strip
	struct DOpusRemember *menu_memory;	// Memory for the menus

	struct TextFont *font[12];	// Fonts used in screen display

	struct ListLock lister_list;   // List of listers
	struct ListLock buffer_list;   // List of buffers
	struct ListLock buttons_list;  // List of button banks

	struct ListLock process_list;  // List of processes

	struct MsgPort *notify_port;  // DOpus Notify port

	struct SignalSemaphore select_lock;
	SelectData select_data;	 // Global select data

	struct SignalSemaphore req_lock;  // Requester lock
	struct IBox req_coords;			  // Requester coordinates

	struct Screen *screen_pointer;	// Screen we are using
	short scr_depth;				// Current depth

	APTR filetype_memory;					// Memory for the filetypes
	struct ListLock filetypes;				// List of filetype lists
	struct NotifyRequest _filetype_notify;	// Filetype notification

	BYTE screen_signal;	 // Screen signal bit

	unsigned short buffer_count;  // Current buffer count

	IPCData *clock;	 // Clock process

	unsigned char decimal_sep;	// Decimal separator

	Cfg_ButtonBank *lister_menu;			  // Lister menu
	struct SignalSemaphore lister_menu_lock;  // Menu lock

	Cfg_ButtonBank *user_menu;				// User menu
	struct SignalSemaphore user_menu_lock;	// Lock

	struct ListLock backdrop_list;	   // Backdrop object list
	struct FontPrefs icon_font_prefs;  // Font prefs for icons

	long max_open_with;	 // Maximum size of 'open with' menu

	char *screen_title;	 // Screen title buffer

	char ver_kickstart[16];
	char ver_workbench[16];
	char ver_cpu[64];  // later machines have more elaborate CPU names
	char ver_fpu[64];
	char ver_chips[32];	 // need more room to hold the graphics card info

	ULONG foo2pad[5];

	ULONG flags;

	struct VSprite drag_head;	// GEL list head sprite
	struct VSprite drag_tail;	// GEL list tail sprite
	struct GelsInfo drag_info;	// GEL info

	BOOL backdrop_window;	  // Window is backdrop?
	struct List boopsi_list;  // BOOPSI objects

	struct _BackdropInfo *backdrop;	 // Backdrop info
	WindowID id;					 // Window ID info

	MenuData *user_menu_data;  // User menu definition

	struct _CxData *cx;	 // Commodities data

	struct MsgPort *appmsg_port;  // App Message port

	long def_filename_length;  // Default filename length

	long cx_pri;  // CX priority

	long wheel_lines;  // Obsolete since CONFIG_VERSION_15: now CFG_ENVR.env_wheel_scroll_lines (#49)

	ULONG pad0[15];

	char rexx_port_name[30];  // ARexx port name

	struct RastPort drag_screen_rp;

	struct NotifyRequest *filetype_notify;	// Filetype notification
	struct NotifyRequest *pattern_notify;	// Pattern file notification
	struct NotifyRequest *font_notify;		// Font prefs notification

	IPCData *hide_clock;  // Hide stuff
	struct AppIcon *hide_appicon;
	struct DiskObject *hide_diskobject;
	struct AppMenuItem *hide_appitem;

	APTR toolbar_arrow_image;  // Toolbar arrow image

	char work_buffer[256];

	struct SignalSemaphore findfile_lock;  // FindFile data
	char findfile_string[40];

	BPTR path_list;	 // Our "path list"

	struct SignalSemaphore filter_lock;	 // Filter data
	char *filter_string;

	char fpad[27];

	struct ListLock notify_process_list;  // Another list of processes

	char *global_undo_buffer;  // A string undo buffer

	struct TextAttr screen_font;  // Screen font
	char screen_font_name[50];

	char poopad[54];

	long dopus_copy;  // Copy number running

	unsigned long dc_seconds[3];  // Double-click stuff
	unsigned long dc_micros[3];
	long dc_button;

	unsigned long last_double_click;  // What was double-clicked?

	struct NotifyRequest *modules_notify;  // Modules notify

	Point dc_pos[3];  // Double-click position

	struct ListerWindow *current_lister;  // Active lister

	struct NotifyRequest *env_notify;		// ENV: notify
	struct NotifyRequest *commands_notify;	// Commands  notify

	char pad[76];

	ULONG orig_flags;  // original flags

	APTR notify_req;  // DOpus Notify request

	struct _rego_data rego;

	short date_length;	// Length needed for a date

	struct BitMap *friend_bitmap;

	char startup_pic[80];  // Startup picture

	UWORD old_pub_modes;

	short startup_pic_delay;

	// Icons for iconified listers and button banks, loaded by get_iconify_icon()
	struct DiskObject *lister_icon;
	struct DiskObject *button_icon;
	struct Screen *iconify_icon_screen;	 // Screen they are remapped for
	struct SignalSemaphore iconify_icon_lock;

	struct SignalSemaphore eject_lock;	// One disk eject at a time

	struct ListLock function_traps;	 // Function trap list

	struct ListLock positions;	// Position list
	char *position_name;		// Name of position file

	PatternData *pattern;		   // Backdrop patterns
	PatternInstance main_pattern;  // Main window pattern

	IPCData *rexx_proc;

	APTR position_memory;  // Memory handle for positions

	Cfg_ButtonBank *hotkeys;			  // Hotkeys
	struct SignalSemaphore hotkeys_lock;  // Lock

	ToolBarInfo *toolbar;  // Global toolbar

	Att_List *command_history;	// Command history

	struct ListLock rexx_readers;  // ARexx text viewers

	struct ListLock foo_user_menu;	// User menu

	ULONG screen_info;	// Display info flags

	Cfg_ButtonBank *scripts;  // Scripts
	struct SignalSemaphore scripts_lock;

	struct ListLock function_list;	// Running functions

	struct ListLock rexx_apps;	// ARexx AppThings
	struct MsgPort *rexx_app_port;

	char null_string[2];

	ImageRemap remap;  // Remap data

	struct SignalSemaphore pattern_lock;
	volatile BOOL pattern_main_pending;	 // Pattern process has not settled the main backdrop yet

	Att_List *icon_sel_list;  // List of selected icons

	struct ListLock command_list;  // Commands
	struct ListLock modules_list;  // List of modules

	struct ListLock group_list;			// List of groups
	struct ListLock original_cmd_list;	// Original commands

	struct ListLock popupext_list;	// PopUp Extensions

	struct ListLock iconpos_list;  // Icon positioning information
	struct MsgPort *iconpos_port;  // Port for icon positioning

	unsigned long flags2;  // More flags

	struct Gadget *iconpos_gadget;	// Icon positioning gadgets

	struct ListLock startmenu_list;	 // Start Menus

	struct SignalSemaphore custom_pen_lock;
	short custom_pens[CUST_PENS][2];
	unsigned long custom_pen_alloc;
	long custom_pen_count[CUST_PENS][2];

	struct _PopUpHandle *desktop_menu;	// Desktop drag'n'drop popup

	struct ListLock open_with_list;

	PatternInstance req_pattern;  // Backfill pattern for requesters

	struct NotifyRequest *desktop_notify;  // Font prefs notification

	long command_line_length;
	short icon_space_x;	 // Obsolete since CONFIG_VERSION_14: now CFG_ENVR.env_icon_space_x (#49)
	short icon_space_y;	 // Obsolete since CONFIG_VERSION_14: now CFG_ENVR.env_icon_space_y (#49)
	short icon_grid_x;	 // Obsolete since CONFIG_VERSION_14: now CFG_ENVR.env_icon_grid_x (#49)
	short icon_grid_y;	 // Obsolete since CONFIG_VERSION_14: now CFG_ENVR.env_icon_grid_y (#49)

	char env_BackgroundPic[4][256];		  // Background pictures
	UWORD env_BackgroundFlags[4];		  // Background flags
	ULONG env_BackgroundBorderColour[4];  // Border colour
} GUI_Glue;

#ifndef __amigaos3__
	#pragma pack()
#endif

#define PATTERN_MAIN 0
#define PATTERN_LISTER 1
#define PATTERN_REQ 2
#define PATTERN_BUTTONS 3

#define GUIF_DRAGGING (1 << 0)
#define GUIF_HIDE_START (1 << 1)	  // Start hidden
#define GUIF_QUIET (1 << 2)			  // Start quiet
#define GUIF_DONE_STARTUP (1 << 3)	  // Done startup script
#define GUIF_DO_WBSTARTUP (1 << 4)	  // Do WBStartup
#define GUIF_BUTTON_EDITOR (1 << 5)	  // Button editor is up
#define GUIF_SAVE_ICONS (1 << 6)	  // Save icons
#define GUIF_FILE_FILTER (1 << 7)	  // Recursive file filter
#define GUIF_CLOCK (1 << 8)			  // Clock is on
#define GUIF_PENDING_QUIT (1 << 9)	  // Quit pending
#define GUIF_LOCALE_OK (1 << 10)	  // Locale patches are ok
#define GUIF_VIEW_ICONS (1 << 11)	  // View icons by default
#define GUIF_SHOW_ALL (1 << 12)		  // Show all by default
#define GUIF_SAVE_LAYOUT (1 << 13)	  // Save lister layout
#define GUIF_DEFPUBSCR (1 << 14)	  // Default public screen
#define GUIF_NO_TOOLS_MENU (1 << 15)  // Remove tools menu
#define GUIF_CLOSED (1 << 16)		  // Closed by CloseWorkbench()
#define GUIF_SYSIHACK (1 << 17)		  // SysIHack is running
#define GUIF_LAYER_LOCKED (1 << 18)	  // LayerInfo is locked
#define GUIF_ICON_ACTION (1 << 19)	  // Default to icon action mode
#define GUIF_REXX (1 << 20)			  // REXX is running
#define GUIF_CLOSE_PENDING (1 << 21)  // Pending CloseWorkbench()

#define GUIF_NO_NOTIFY (1 << 23)   // No module notification
#define GUIF_NO_BORDERS (1 << 25)  // No icon borders
#define GUIF_OPEN (1 << 27)		   // Display is open
#define GUIF_OK (1 << 28)		   // Set by pirate checking
#define GUIF_DONE_PATCH (1 << 29)
#define GUIF_GOT_PALETTE (1 << 30)	  // Allocated palette
#define GUIF_LISTER_COOKIE (1 << 31)  // Done lister cookie

#define GUIF2_ICONPOS (1 << 0)		   // Doing icon positioning
#define GUIF2_NO_SCREENFRONT (1 << 5)  // Don't bring screen to front
#define GUIF2_BACKFILL_SET (1 << 7)	   // Set backfill
#define GUIF2_KEY_FINDER (1 << 9)	   // Key Finder active
#define GUIF2_PATTERN_SHOWN (1 << 11)  // display_open drew the backdrop picture already

// The following GUIF2 bits were retired in #49 - all four moved into the
// CFG_ENVR struct so they can be persisted as part of the user's prefs:
//   bit 1  GUIF2_WB_TITLE          -> DISPOPTF_WB_TITLE
//   bit 3  GUIF2_ENABLE_SHORTCUTS  -> ENVF_ENABLE_SHORTCUTS
//   bit 8  GUIF2_NO_PADLOCK        -> LISTEROPTF_NO_PADLOCK
//   bit 10 GUIF2_BENIFY            -> ENVF_BENIFY
// Bits 1, 3, 8, 10 are intentionally left unassigned in the GUI->flags2
// namespace to avoid a future contributor accidentally re-using them with
// different semantics.

extern GUI_Glue *GUI;

#define POPUP_NORMAL 0
#define POPUP_ACTIVATE 1

// Close flags
#define CLOSE_WINDOW (1 << 0)					 // Close the window
#define CLOSE_SCREEN (1 << 1)					 // Close the screen
#define CLOSE_STOREPOS (1 << 2)					 // Store window position
#define CLOSE_CHILDREN (1 << 3)					 // Close child windows
#define CLOSE_ALL (CLOSE_WINDOW | CLOSE_SCREEN)	 // Close all
#define CLOSE_KEEP_PATTERN (1 << 4)
#define CLOSE_KEEP_REMAP (1 << 5)

// Prototypes
BOOL display_open(long);
BOOL display_rtg_bitmap(struct BitMap *bm);
struct DiskObject *get_iconify_icon(BOOL buttons);
void close_display(int, BOOL);
void display_box_to_rect(struct IBox *, struct Rectangle *);
void display_gui_complete(GUI_Element *, struct RastPort *);
void display_gui_border(GUI_Element *);
void display_gui_clear(GUI_Element *);
BOOL point_in_element(GUI_Element *, UWORD, UWORD);
void display_lock_all(int);
void display_popup(short);
struct MenuItem *find_menu_item(struct Menu *menu, UWORD id);
void display_build_user_menu(void);
void display_reset_menus(BOOL main_only, BOOL fix_only);
void display_get_menu(struct Window *window);
void display_free_menu(struct Window *window);
void display_fix_menu(struct Window *window, long type, APTR);
void menu_check_disable(struct MenuItem *item, unsigned long type);
short display_get_pattern(BOOL);
void display_free_pattern(void);
void display_store_pos(void);

void hide_display(void);
void display_set_shanghai(BOOL);

void display_update_pattern(short which, short valid);

#define DSPOPENF_NO_REMAP (1 << 0)
#define DSPOPENF_DESKTOP (1 << 7)

#define DEF_OPEN_WITH 10

#define MODE_SHADOW 2
#define MODE_OUTLINE 3

#endif
