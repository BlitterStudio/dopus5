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

#ifndef _DOPUS_LISTER
#define _DOPUS_LISTER

#define SLIDER_VERT (1 << 0)
#define SLIDER_HORZ (1 << 1)
#define SLIDER_VERT_DISPLAY (1 << 2)
#define SLIDER_HORZ_DISPLAY (1 << 3)
#define SLIDER_SCROLL (1 << 6)
#define UPDATE_POS_ONLY (1 << 7)

#define LISTER_BFPF_CHECK_LOCKS (1 << 0)
#define LISTER_BFPF_DONT_MOVE (1 << 1)
#define LISTER_BFPF_DONT_LOCK (1 << 2)
#define LISTER_BFPF_DONT_TEST (1 << 3)
#define LISTER_BFPF_DONT_UNLOCK (1 << 5)

#define LISTERF_MAKE_SOURCE (1 << 0)
#define LISTERF_ACTIVATE (1 << 1)
#define LISTERF_DUAL (1 << 2)
#define LISTERF_DEVICES 0x400000

#define LISTER_DUAL_TOGGLE -1
#define LISTER_DUAL_OFF 0
#define LISTER_DUAL_ON 1
#define LISTER_DUAL_LEFT 0
#define LISTER_DUAL_RIGHT 1
#define LISTER_DUAL_SIDES 2

#define KEY_SEL_OFFSET 12

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct
{
	struct MinNode node;
	IPCMessage *msg;
	struct Message *msg2;
	short type;
} ListerWaiter;

#ifndef __amigaos3__
	#pragma pack()
#endif

#define LISTERWAIT_ALL 0
#define LISTERWAIT_UNBUSY 1
#define LISTERWAIT_BUSY 2
#define LISTERWAIT_AUTO_CMD 3

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct
{
	ULONG total;
	char *operation;
	ULONG flags;
} ProgressPacket;

#ifndef __amigaos3__
	#pragma pack()
#endif

enum {
	GAD_PATH,
	GAD_PARENT,

	GAD_ABORT = 8,
	GAD_ROOT,
	GAD_ICONIFY,
	GAD_LOCK,
	GAD_GAUGE,
	GAD_ZOOM,
};

enum {
	LISTER_OPEN,		 // Open lister display					0
	LISTER_CLOSE,		 // Close lister display
	LISTER_MAKE_SOURCE,	 // Make this lister the source
	LISTER_ACTIVATE,	 // Change activation status
	LISTER_BUSY,		 // Set or clear busy state
	LISTER_MAKE_DEST,	 // Make this lister the destination		5
	LISTER_OFF,			 // Turn this lister off

	LISTER_CHECK_SPECIAL_BUFFER,  // Check if currently showing a special buffer
	LISTER_SHOW_SPECIAL_BUFFER,	  // Show special buffer
	LISTER_BUFFER_FIND,			  // Searches for a named buffer

	LISTER_BUFFER_FIND_EMPTY,  // Finds an empty buffer			10
	LISTER_BUFFER_LIST,		   // List available buffers
	LISTER_GET_PATH,		   // Get current path
	LISTER_REFRESH_PATH,	   // Refresh path field
	LISTER_REFRESH_WINDOW,	   // Refresh lister
	LISTER_REFRESH_TITLE,	   // Refresh lister title				15
	LISTER_REFRESH_SLIDERS,	   // Refresh lister sliders
	LISTER_ADD_FILE,		   // Add file to list
	LISTER_REMOVE_FILE,		   // Remove file from list
	LISTER_RELOAD_FILE,		   // Reload a file in the list

	LISTER_MAKE_RESELECT,	// Make reselection list			20
	LISTER_DO_RESELECT,		// Reselect from a list
	LISTER_DO_PARENT_ROOT,	// Do parent/root read
	LISTER_FIND_FIRST_SEL,	// Scroll to first selected entry
	LISTER_FIND_ENTRY,		// Scroll to show an entry
	LISTER_STORE_POS,		// Store lister position				25
	LISTER_RESTORE_POS,		// Restore old position in lister
	LISTER_SHOW_INFO,		// Show selection information
	LISTER_REMOVE_SIZES,	// Remove directory sizes
	LISTER_SET_LOCKER,		// Set locker port

	LISTER_INIT,				  // Initialise and (maybe) open lister	30
	LISTER_SELECT_GLOBAL_STATE,	  // Select global state
	LISTER_SELECT_GLOBAL_TOGGLE,  // Toggle global state
	LISTER_STATUS,				  // Lister status text
	LISTER_PROGRESS_ON,			  // Progress indicator on
	LISTER_PROGRESS_OFF,		  // Progress indicator off			35
	LISTER_PROGRESS_UPDATE,		  // Progress indicator update
	LISTER_UNLOCK,				  // Unlock this lister
	LISTER_SELECT_WILD,			  // Do wildcard selection
	LISTER_DEVICE_LIST,			  // Display device list in this lister

	LISTER_PARENT,			// Read parent							40
	LISTER_ROOT,			// Read root
	LISTER_UPDATE_STAMP,	// Update buffer stamp
	LISTER_COPY_BUFFER,		// Copy a buffer from somewhere
	LISTER_RESCAN,			// Rescan contents
	LISTER_RESCAN_CHECK,	// Rescan contents if date changed		45
	LISTER_SPLIT,			// Split display
	LISTER_WAIT,			// Wait for lister to finish
	LISTER_FILETYPE_SNIFF,	// Sniffed out a filetype
	LISTER_REFRESH_NAME,	// Refresh lister name
	LISTER_GET_ICONS,		// Read icons							50

	LISTER_REMOVE_ENTRY,  // Remove an entry
	LISTER_GET_HANDLE,	  // Get lister's handle
	LISTER_REFRESH_FREE,  // Refresh free space

	LISTER_BACKFILL_CHANGE,	 // Backfill pattern has changed

	LISTER_ICONIFY,	   // Iconify lister						55
	LISTER_UNICONIFY,  // Uniconify lister

	LISTER_MODE,	 // Change mode
	LISTER_TOOLBAR,	 // New toolbar

	LISTER_WAIT_BUSY,  // Wait for lister to go busy

	LISTER_SHOW_BUFFER,	 // Show a buffer in a lister			60

	LISTER_FILE_PROGRESS_TOTAL,	  // Set file progress total
	LISTER_FILE_PROGRESS_UPDATE,  // Set file progress update
	LISTER_GET_ICON,			  // Get specific icon

	LISTER_DO_FUNCTION,	   // Do a function
	LISTER_CHECK_REFRESH,  // Check for refresh				65

	LISTER_UPDATE_DEVICE,  // Change a device name

	LISTER_HIGHLIGHT,			// Highlight an entry (perhaps)
	LISTER_SHOW_ICON,			// Show a specific icon
	LISTER_FILE_PROGRESS_INFO,	// Set progress info string
	LISTER_PROGRESS_TOTAL,		// Set total progress				70
	LISTER_FILE_PROGRESS_SET,	// Set total file stuff
	LISTER_PROGRESS_TITLE,		// Set progress title
	LISTER_PROGRESS_COUNT,		// Update bar count

	LISTER_SET_SNIFF,			// Set sniff flag
	LISTER_CONFIGURE,			// Change display format				75
	LISTER_SELSHOW,				// Select/show entry
	LISTER_FIND_CACHED_BUFFER,	// Find a buffer in the cache
	LISTER_SET_GAUGE,			// Set gauge state

	LISTER_FILE_PROGRESS_TOTAL64,	// Set 64bit file progress total
	LISTER_FILE_PROGRESS_UPDATE64,	// Set 64bit file progress update
	LISTER_DUAL,					// Toggle dual lister mode
};

#define LISTER_SHOW_BUFFER_SIDE_SHIFT 16
#define LISTER_SHOW_BUFFER_SIDE_MASK 0xff
#define LISTER_SHOW_BUFFER_FLAGS_MASK 0xffff
#define LISTER_SHOW_BUFFER_PACK_FLAGS(flags, side) \
	(((ULONG)(flags) & LISTER_SHOW_BUFFER_FLAGS_MASK) | \
	 (((ULONG)(side) & LISTER_SHOW_BUFFER_SIDE_MASK) << LISTER_SHOW_BUFFER_SIDE_SHIFT))
#define LISTER_SHOW_BUFFER_SIDE(flags) \
	(((ULONG)(flags) >> LISTER_SHOW_BUFFER_SIDE_SHIFT) & LISTER_SHOW_BUFFER_SIDE_MASK)
#define LISTER_SHOW_BUFFER_BASE_FLAGS(flags) ((ULONG)(flags) & LISTER_SHOW_BUFFER_FLAGS_MASK)

// Maximum length we can display in a window
#define MAXDISPLAYLENGTH 256

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct
{
	short tb_Left;
	short tb_Right;
	short tb_Item;
	short tb_Position;
	short tb_Width;
	short tb_ItemNum;
} TitleBox;

typedef struct
{
	short cr_Pen[2];
	short cr_Alloc;	 // COLRF_ flags for each pen
} ColourRecord;

#define COLRF_CUSTOM(num) (1 << (num))		   // Exclusive pen from GetCustomPen()
#define COLRF_SHARED(num) (1 << ((num) + 2))  // Closest shared pen from ObtainBestPen()

// Defines a directory lister
typedef struct ListerWindow
{
	IPCData *ipc;  // IPC data

	struct Window *window;		  // Lister window
	struct IBox pos;			  // Window position
	struct AppWindow *appwindow;  // AppWindow pointer
	struct AppIcon *appicon;	  // AppIcon pointer
	struct List _boopsi_list;	  // List of boopsi objects

	GUI_Element name_area;			 // Disk name area
	GUI_Element status_area;		 // Lister status area
	GUI_Element list_area;			 // Full lister area
	GUI_Element text_area;			 // Area for the text
	UWORD text_width;				 // Displayed columns of text
	UWORD text_height;				 // Displayed lines of text
	struct Rectangle scroll_border;	 // Scroll border

	struct Gadget *_vert_scroll;   // Vertical scroller
	struct Gadget *_horiz_scroll;  // Horizontal scroller
	struct Gadget *path_field;	   // Path field
	char path_buffer[512];		   // Path field buffer
	struct Gadget parent_button;   // Parent button

	Cfg_Lister *lister;	 // Lister pointer

	struct DirectoryBuffer *cur_buffer;		 // Current directory buffer
	struct DirectoryBuffer *special_buffer;	 // Special buffer
	struct DirectoryBuffer *old_buffer;		 // Last buffer displayed

	ListFormat format;	// List format

	struct Gadget *down_gadget;	 // Gadget held down

	struct _BackdropInfo *backdrop_info;  // Backdrop information

	long selector_pos;	// Selector position

	struct DiskObject *appicon_icon;  // AppIcon icon
	char icon_name[36];				  // Icon name

	struct Hook *path_edit_hook;  // Edit hook for path field
	Att_List *path_history;		  // Path history

	WindowDimensions dimensions;  // Stored dimensions

	short abort_signal;	 // Signal for abort

	ULONG flags;  // Flags

	char *title;  // Title buffer

	struct BitMap *render_bitmap;  // Bitmap for file display
	long render_bitmap_width;	   // Width
	struct Layer_Info *render_layerinfo;
	struct Layer *render_layer;

	short title_drag_x;

	ULONG sep_old_seconds;	// Old event seconds
	ULONG sep_old_micros;	// Old event micros

	short tool_hover_but;			 // Hovered toolbar button (-1 = none)
	short tool_hover_ticks;			 // IntuiTicks elapsed on current hover
	struct Window *tool_tip_window;	 // Tooltip popup (0 = not shown)
	char pad2[42];					 // Reserved; dual lister stores a private APTR state slot here

	DragInfo *drag_info;	   // File drag information
	struct MsgPort *app_port;  // Application message port

	struct Requester busy_req;		 // Busy requester
	struct ListerWindow *lock_list;	 // Lister we are locked by
	struct IBox win_limits;			 // Normal window size limits

	IPCData *locker_ipc;  // Locker's IPC data

	ULONG seconds;		// Event seconds
	ULONG micros;		// Event micros
	ULONG old_seconds;	// Old event seconds
	ULONG old_micros;	// Old event micros

	struct DrawInfo *drawinfo;	// DrawInfo
	APTR _visualinfo;			// VisualInfo

	short normal_pri;  // Priority
	short busy_pri;

	APTR progress_window;  // Progress window
	APTR progress_memory;  // Memory for progress names

	struct TextAttr lister_font;  // Font used by lister
	struct TextAttr screen_font;  // Font used by screen

	struct MsgPort *timer_port;	 // Timer port
	TimerHandle *busy_timer;	 // Timer request for busy

	PopUpMenu lister_menu;	// Lister menus
	PopUpMenu lister_tool_menu_old;
	APTR memory;  // Lister memory

	ULONG rmb_old_seconds;	// Old event seconds
	ULONG rmb_old_micros;	// Old event micros

	WindowID id;  // Window ID pointer

	struct DirectoryEntry *selected_entry;		 // Stores selected entry
	struct DirectoryEntry *last_selected_entry;	 // Last selected entry

	struct TextFont *font;	// Font we use
	char font_name[33];		// Font name
	UBYTE font_size;		// Font size

	char work_buffer[1024];		// Work space
	struct FileInfoBlock *fib;	// FIB to play with

	char display_buf[MAXDISPLAYLENGTH];	 // Display buffer

	TimerHandle *scroll_timer;	// Scroll timer

	ToolBarInfo *toolbar;	   // Toolbar
	GUI_Element toolbar_area;  // Area for the toolbar

	short tool_type;			  // Function type
	short tool_sel;				  // Selected button
	short tool_button;			  // Mouse button down
	short tool_selected;		  // Button is selected
	Cfg_Button *tool_sel_button;  // Selected button

	struct _BackdropObject *backdrop;  // Backdrop object we came from

	TimerHandle *foo_timer;	 // Another timer

	GUI_Element parent_area;   // Parent/history popup
	GUI_Element command_area;  // Command popup

	struct Gadget size_cover_gadget;  // Covers sizing gadget
	struct Gadget zoom_cover_gadget;  // Covers zoom gadget

	struct Gadget *hijacked_zoom;	  // System zoom gadget we repurposed (to restore on close)
	UWORD		   hijacked_zoom_type;
	UWORD		   hijacked_zoom_id;

	struct ListLock user_data_list;	 // User data list

	short history_count;

	short toolbar_offset;  // Toolbar offset
	short toolbar_show;	   // Number of displayed buttons

	short drag_x;  // Drag shit
	short drag_y;

	TimerHandle *icon_drag_timer;  // Timer for dragging icons

	struct Rectangle path_border;  // Path field border size

	struct position_record *pos_rec;  // Original position record

	PatternInstance pattern;  // Backfill pattern

	ToolBarInfo *toolbar_alloc;	 // Allocated ToolBar

	ULONG old_flags;

	char toolbar_path[256];	 // Toolbar path

	short tool_row;	 // Selected button row

	APTR toolbar_image[2];	// Images of current button

	struct MinList wait_list;

	ReselectionData reselect;  // Reselection information

	struct DirectoryEntry *last_highlight;	// Highlighted entry
	short last_highpos;						// position

	APTR notify;  // Lister notify
	struct MsgPort *notify_port;

	char hot_name[32];	// Hot name
	short hot_name_pos;
	struct Requester *hot_name_req;
	short hot_name_bit;
	struct Hook *hot_name_hook;

	char filter_name[40];			// Live filter pattern edit buffer
	struct Requester *filter_name_req;
	short filter_name_bit;
	struct Hook *filter_name_hook;

	ListFormat user_format;	 // User-defined format

	ULONG more_flags;  // Some more flags

	GUI_Element title_area;		  // List title
	struct Region *title_region;  // Region for title clipping

	TitleBox title_boxes[16];  // Position of title boxes

	short title_click;	// Title clicked on
	short title_sel;	// Selected or not

	struct IBox other_dims;	 // Dimensions in other mode

	short title_click_x;
	short title_click_y;

	long icon_pos_x;
	long icon_pos_y;

	short cursor_pos;					// Position of cursor in display
	short cursor_line;					// Line in the display being edited
	short edit_type;					// Item being edited
	short edit_pos;						// Position of cursor in item
	TimerHandle *edit_timer;			// Timer for editing
	struct DirectoryEntry *edit_entry;	// Entry being edited
	short edit_length;					// Length of entry
	short edit_max_length;				// Maximum length of entry
	char edit_buffer[256];				// Edit buffer
	short edit_field_pos;				// Field position

	UWORD flags2;  // More flags

	char *edit_ptr;		   // Edit pointer
	char *edit_old_ptr;	   // Old edit pointer
	ULONG edit_flags;	   // Edit flags
	short edit_old_width;  // Last max width
	short edit_v_scroll;   // Amount to scroll by

	short edit_old_offset;	// Offset for new edit
	short edit_new_offset;

	short toolbar_arrow_left;  // Arrow position
	short toolbar_arrow_right;

	struct Gadget *gauge_gadget;  // Gauge
	short old_border_left;
	short pad;

	struct Region *refresh_extra;  // Refresh region

	struct IBox restore_dims;  // Dimensions when restored from zoom

	PopUpHandle *lister_tool_menu;

	ColourRecord lst_Colours[ENVCOL_MAX];

	struct RastPort render_rast;
} Lister;

#ifndef __amigaos3__
	#pragma pack()
#endif

#define LISTERF_BUSY (1 << 1)			   // Lister is busy
#define LISTERF_SOURCE (1 << 2)			   // Lister is a source
#define LISTERF_DEST (1 << 3)			   // Lister is a destination
#define LISTERF_STORED_SOURCE (1 << 4)	   // Lister was a source
#define LISTERF_STORED_DEST (1 << 5)	   // Lister was a destination
#define LISTERF_LOCK (1 << 6)			   // Lister is locked
#define LISTERF_LOCKED (1 << 7)			   // Lister is locked (but not actually busy)
#define LISTERF_PROGRESS (1 << 8)		   // Progress indicator is displayed
#define LISTERF_ABORTED (1 << 9)		   // As operation has been aborted
#define LISTERF_SOURCEDEST_LOCK (1 << 10)  // Source/Destination locked
#define LISTERF_SNIFFING (1 << 11)		   // Lister is sniffing things
#define LISTERF_RESCAN (1 << 12)		   // Rescan when we become unbusy
#define LISTERF_BUSY_VISUAL (1 << 13)	   // Visually busy
#define LISTERF_KEY_SELECTION (1 << 14)	   // Keyboard selection active
#define LISTERF_SAVE_STATUS (1 << 15)	   // Save status message
#define LISTERF_ICONIFIED (1 << 16)		   // Lister is iconified
#define LISTERF_CLOSING (1 << 17)		   // Lister is closing
#define LISTERF_CHANGED_MODE (1 << 18)	   // Just changed mode
#define LISTERF_RESIZED (1 << 19)		   // Just resized
#define LISTERF_NO_REFRESH (1 << 20)	   // Don't refresh files
#define LISTERF_TITLE_DEFERRED (1 << 21)   // Title set deferred
#define LISTERF_ICON_ACTION (1 << 22)	   // Icon Action mode
#define LISTERF_SHOW_TITLE (1 << 23)	   // Update title
#define LISTERF_SHOW_ALL (1 << 24)		   // Show All Files
#define LISTERF_PATH_FIELD (1 << 25)	   // Path field is added
#define LISTERF_VIEW_ICONS (1 << 26)	   // Icons being viewed
#define LISTERF_LOCK_POS (1 << 27)		   // Position is locked
#define LISTERF_TEMP_FLAG (1 << 28)		   // Temporary magic flag
#define LISTERF_TOOLBAR (1 << 29)		   // Valid toolbar
#define LISTERF_DEVICE_LIST (1 << 30)	   // Showing device list
#define LISTERF_FIRST_TIME (1 << 31)	   // First initialisation

#define LISTERF2_UNAVAILABLE (1 << 0)  // Lister is unavailable for functions
#define LISTERF2_DUAL (1 << 1)		 // Lister is in dual-panel mode

#define LISTERF_PROP_FONT (1 << 0)	   // Proportional font
#define LISTERF_TITLE (1 << 1)		   // Title for fields
#define LISTERF_HIDE_TITLE (1 << 2)	   // Title is hidden
#define LISTERF_CONFIGURE (1 << 3)	   // Configuration window up
#define LISTERF_LOCK_FORMAT (1 << 4)   // Sort format is locked
#define LISTERF_MODE_CHANGED (1 << 5)  // Mode was just changed
#define LISTERF_NO_ACTIVE (1 << 6)	   // Don't activate lister
#define LISTERF_TITLEBARRED (1 << 7)   // Shrunk to title bar
#define LISTERF_NEED_RECALC (1 << 8)   // Contents changed, needs recalc

#define EDITF_REFRESH (1 << 0)	  // Refresh needed
#define EDITF_CANCEL (1 << 1)	  // Cancel
#define EDITF_LONGEST (1 << 2)	  // This becomes the longest entry
#define EDITF_RECALC (1 << 3)	  // Recalculate horizontal width
#define EDITF_NOREFRESH (1 << 4)  // Don't refresh

#define EDITF_KEEP (1 << 0)		 // Keep changes
#define EDITF_OFFSET (1 << 1)	 // Calculate offset
#define EDITF_NO_TITLE (1 << 2)	 // Don't refresh title

extern UWORD lister_popup_data[];

#define REFRESHF_SLIDERS (1 << 0)  // 95, 185, 85, A5
#define REFRESHF_NAME (1 << 1)
#define REFRESHF_STATUS (1 << 2)  // 95, 185, 85, A5
#define REFRESHF_RESORT (1 << 3)
#define REFRESHF_ICONS (1 << 4)		   // 95, 85
#define REFRESHF_CLEAR_ICONS (1 << 5)  // A5
#define REFRESHF_NO_ICONS (1 << 6)
#define REFRESHF_UPDATE_NAME (1 << 7)  // 95, 185, 85, A5
#define REFRESHF_CD (1 << 8)		   // 185
#define REFRESHF_FULL (1 << 9)
#define REFRESHF_DATESTAMP (1 << 10)
#define REFRESHF_REFRESH (1 << 11)
#define REFRESHF_PATH (1 << 12)
#define REFRESHF_SORTSEL (1 << 13)
#define REFRESHF_COLOUR (1 << 14)

enum { PP_PARENT = 1, PP_ROOT, PP_DEVICE_LIST, PP_BUFFER_LIST, PP_REREAD_DIR, PP_BASE };

// protos
Lister *lister_new(Cfg_Lister *cfg_lister);
void lister_free(Lister *lister);
ULONG lister_command(Lister *, ULONG, ULONG, APTR, APTR, struct MsgPort *);
Lister *find_lister_path(char *);
void lister_update(Lister *);
void listers_update(BOOL, BOOL);
void lister_init_new(Cfg_Lister *cfg, Lister *lister);
void lister_tile(long id);
BOOL set_list_selector(Lister *lister, long pos);
BOOL lister_iconify(Lister *lister);
BOOL lister_new_waiter(Lister *lister, IPCMessage *msg, struct Message *msg2, short type);
void lister_relieve_waiters(Lister *, short);
void lister_smart_source(Lister *);

IPC_EntryProto(lister_code, extern);
void lister_cleanup(Lister *, BOOL);
struct DirectoryBuffer *lister_new_buffer(Lister *);
void lister_free_buffer(struct DirectoryBuffer *);
void lister_process_msg(Lister *, struct IntuiMessage *);
void lister_send_abort(Lister *);
int lister_do_function(Lister *, IPTR);

void lister_busy(Lister *, int);
void lister_unbusy(Lister *, int);
void lister_progress_on(Lister *, ProgressPacket *);
void lister_progress_calc_size(Lister *, struct Screen *, long);
void lister_progress_off(Lister *);
void lister_progress_update(Lister *, char *, ULONG);
void check_lister_stored(Lister *lister);
void lister_progress_filetotal(Lister *lister, long total);
void lister_progress_filetotal64(Lister *lister, QUAD *total);
void lister_progress_fileprogress(Lister *lister, long count);
void lister_progress_info(Lister *lister, char *info);
void lister_progress_total(Lister *lister, long total, long count);
void lister_progress_file(Lister *lister, long total, long count);
void lister_progress_title(Lister *lister, char *info);

void lister_show_name(Lister *);
void lister_clear_title(Lister *);
void lister_show_status(Lister *);
void lister_title_pens(struct RastPort *, Lister *, int);
void lister_status(Lister *, char *);
void lister_clear_title_area(GUI_Element *area);

Lister *lister_source(void);
Lister *lister_dest(void);
Lister *lister_default(ULONG, BOOL);
Lister *lister_dual_default(void);
void lister_check_source(Lister *);
void lister_check_dest(Lister *);
void lister_split_display(Lister *lister, Lister *other_lister);
BOOL lister_dual_toggle(Lister *lister, short mode);
BOOL lister_dual_enter(Lister *lister);
void lister_dual_leave(Lister *lister);
void lister_dual_activate(Lister *lister);
void lister_dual_lock_source(Lister *lister);
void lister_dual_unlock(Lister *lister);
void lister_dual_detach_closing(Lister *lister);
Lister *lister_dual_active_side(Lister *lister);
short lister_dual_active_index(Lister *lister);
short lister_dual_launch_side(Lister *lister);
void lister_dual_view_side(Lister *lister, short side);
BOOL lister_dual_view_buffer(Lister *lister, struct DirectoryBuffer *buffer);
struct DirectoryBuffer *lister_dual_side_buffer(Lister *lister, short side);
short lister_dual_side_at(Lister *lister, short x, short y);
BOOL lister_dual_other_side_shows(Lister *lister, struct DirectoryBuffer *buffer);
void lister_dual_set_drop_side(Lister *lister, short side);
short lister_dual_take_drop_side(Lister *lister);
short lister_dual_highlight_side(Lister *lister);
void lister_dual_set_highlight_side(Lister *lister, short side);
void lister_dual_show_status(Lister *lister);
void lister_dual_queue_rescan(Lister *lister);
BOOL lister_dual_do_rescans(Lister *lister);
BOOL lister_dual_is_side(Lister *lister);
BOOL lister_dual_one_window(Lister *lister);
BOOL lister_dual_init_filelist(Lister *lister);
BOOL lister_dual_init_display(Lister *lister);
void lister_dual_refresh(Lister *lister, unsigned short mode);
void lister_dual_finish_frame_refresh(Lister *lister);
void lister_dual_refresh_display(Lister *lister, ULONG flags);
BOOL lister_dual_refresh_all(Lister *lister, ULONG flags);
void lister_dual_close_display(Lister *lister);
void lister_dual_activate_at(Lister *lister, UWORD x, UWORD y);
void lister_dual_activate_other(Lister *lister);
short lister_dual_dest_index(Lister *lister);
void lister_dual_release_special(Lister *lister);
void lister_dual_get_format(Lister *lister, short side, ListFormat *format);
void lister_dual_set_format(Lister *lister, short side, ListFormat *format);
BOOL lister_dual_update_pathfields(Lister *lister);
BOOL lister_dual_disable_pathfields(Lister *lister, short disable);
BOOL lister_dual_handle_path_gadget(Lister *lister, struct Gadget *gadget);
BOOL lister_dual_activate_pathfield(Lister *lister);
BOOL lister_dual_update_sliders(Lister *lister, int which);
BOOL lister_dual_handle_scroll_gadget(Lister *lister, struct Gadget *gadget, short which);
BOOL lister_dual_handle_close_button(Lister *lister, UWORD x, UWORD y);
BOOL lister_dual_help_at(Lister *lister, UWORD x, UWORD y);

void lister_configure(Lister *);
void lister_change_format(Lister *, ListFormat *);
void lister_set_sort(Lister *lister, short item, UWORD);
BOOL lister_resort(Lister *, short);
int resort_test(ListFormat *, ListFormat *);
void lister_parent_popup(Lister *lister, unsigned short code);
void lister_add_history(Lister *lister, struct DirectoryBuffer *buffer);

struct DirectoryBuffer *lister_find_buffer(Lister *,
										   struct DirectoryBuffer *,
										   char *,
										   struct DateStamp *,
										   char *,
										   ULONG);
struct DirectoryBuffer *lister_buffer_find_empty(Lister *, char *, struct DateStamp *);
void lister_check_old_buffer(Lister *, BOOL);
void lister_show_buffer(Lister *, struct DirectoryBuffer *, int, BOOL);
void update_buffer_stamp(Lister *);
struct DirectoryBuffer *lister_get_empty_buffer(void);
void remove_file_global(char *, char *, BOOL);
void update_lister_global(char *path);

// lister_show.c
void lister_display_dir(Lister *);
void display_entry(struct DirectoryEntry *, Lister *, int);
void lister_draw_entry(Lister *, struct DirectoryEntry *, short, short, short, short, short);
void setdispcol(struct DirectoryEntry *, Lister *);
void getdispcol(struct DirectoryEntry *, Lister *, short *, short *);
void builddisplaystring(struct DirectoryEntry *, char *, Lister *, struct DirectoryBuffer *);
void entry_highlight(Lister *, short);
void lister_update_name(Lister *);
void lister_refresh_display(Lister *, ULONG);
void lister_refresh_name(Lister *);
void select_show_info(Lister *, BOOL);

// lister_toolbar.c
void lister_show_toolbar(Lister *);
void lister_toolbar_click(Lister *, short, short, unsigned short, unsigned short);
void lister_toolbar_function(Lister *);
void lister_build_menu(Lister *lister);
void lister_get_toolbar(BOOL, Cfg_ButtonBank *);
void lister_toolbar_free_cache(void);
Cfg_Button *lister_get_toolbar_button(Lister *, short, short, short *);
void lister_toolbar_tooltip_tick(Lister *);
void lister_toolbar_tooltip_hide(Lister *);

// lister_icons.c
struct _FunctionHandle;
struct icon_copy_data;
void lister_get_icons(struct _FunctionHandle *, Lister *lister, char *, BOOL);
void lister_icon_copy(Lister *dest, Lister *source, struct _DOpusAppMessage *msg);
void lister_icon_copy_cleanup(struct icon_copy_data *data);
void lister_icon_copy_add(struct icon_copy_data *data);
void lister_icon_copy_rem(Lister *lister, char *name);

// lister_window.c
struct Window *lister_open(Lister *lister, struct Screen *screen);
void lister_close(Lister *lister, BOOL);

// lister_display.c
void lister_init_display(Lister *lister);
void lister_refresh(Lister *lister, unsigned short mode);
void lister_init_filelist(Lister *lister);
void lister_init_lister_area(Lister *lister);
void lister_update_slider(Lister *lister, int which);
BOOL lister_fix_horiz_len(Lister *lister);
void lister_add_pathfield(Lister *lister);
void lister_remove_pathfield(Lister *lister, BOOL);
void lister_disable_pathfield(Lister *lister, short disable);
void lister_mode_change(Lister *, struct IBox *);

void lister_toolbar_update_cache(void);
void center_erase_text(struct RastPort *rp, char *text, short minx, short maxx, short y);
void lister_check_refresh(Lister *lister);
BOOL lister_valid_toolbar(Lister *lister);
void lister_new_toolbar(Lister *lister, char *name, ToolBarInfo *);
void lister_toolbar_edit(short);

void lister_show_title(Lister *lister, BOOL);
void lister_title_highlight(Lister *lister, short item, short);

void lister_calc_limits(Lister *lister, struct Screen *screen);
void lister_set_limits(Lister *lister);
void lister_update_pathfield(Lister *lister);

void lister_fix_priority(Lister *);

#define FIELD_FONT lister->window->WScreen->RastPort.Font

#define LREFRESH_FULL (1 << 0)
#define LREFRESH_CLEAR (1 << 1)
#define LREFRESH_NOICON (1 << 2)
#define LREFRESH_REFRESH (1 << 3)
#define LREFRESH_FULL_ICON (1 << 4)
#define LREFRESH_SIMPLEREFRESH (1 << 5)

void lister_receive_drop(Lister *dest, struct _DOpusAppMessage *amsg);

#define DROPF_DEVICE (1 << 0)	  // Dropped item is a device
#define DROPF_ICON_MODE (1 << 1)  // Source lister in icon mode
#define DROPF_PARENT (1 << 2)	  // Dropped on parent area

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct
{
	char old_name[32];
	char new_name[32];
	struct DateStamp date;
} devname_change;

#ifndef __amigaos3__
	#pragma pack()
#endif

void lister_diskinfo(Lister *lister);

BOOL lister_check_valid(Lister *lister);

struct DirectoryEntry *lister_test_drag(Lister *, DragInfo *, short, short, Lister **);
struct DirectoryEntry *lister_highlight(Lister *, short x, short y, DragInfo *drag);

void lister_show_icon(Lister *lister, struct _BackdropObject *object);

void lister_clip_entries(Lister *lister, unsigned short qual);

void lister_set_title(Lister *lister);

void lister_set_busy_icon(Lister *lister);
void lister_clear_busy_icon(Lister *lister);

void lister_build_icon_name(Lister *lister);

void lister_fix_menus(Lister *lister, BOOL);

void lister_fix_cd(Lister *lister);

void lister_add_hotname(Lister *lister, char);
void lister_rem_hotname(Lister *lister);
void lister_handle_hotname(Lister *lister);

void lister_add_filter(Lister *lister);
void lister_rem_filter(Lister *lister);
void lister_handle_filter(Lister *lister);

#define LISTERPOPUP_CLOSE 0

void lister_clear_title(Lister *lister);

Lister *lister_open_new(char *, struct _BackdropObject *, struct Window *, Lister *);
void lister_title_drop(Lister *lister, short item, short x, short y);

void lister_update_icon(Lister *lister);

void lister_end_edit(Lister *, short);
BOOL lister_edit_key(Lister *, struct IntuiMessage *);
BOOL lister_start_edit(Lister *, short, short, short);
BOOL lister_edit_init(Lister *, BOOL);
short lister_edit_next(Lister *, short);
BOOL lister_edit_check_item(Lister *);
BOOL lister_edit_check_cursor(Lister *, short);
BOOL lister_edit_key_press(Lister *, unsigned char, UWORD);
BOOL lister_edit_key_string(Lister *, unsigned char, UWORD);
BOOL lister_edit_key_protect(Lister *, unsigned char);
BOOL lister_edit_key_netprotect(Lister *, unsigned char);

REF_CALLBACK_PROTO(void, lister_refresh_callback, d0, ULONG, type, a0, struct Window *, window, a1, Lister *, lister);

void lister_edit_error(Lister *, short);

char *lister_title_string(struct DirectoryBuffer *buffer, short item);

BOOL lister_find_cached_buffer(Lister *lister, char *path, char *handler);
void lister_free_caches(Lister *lister, char *handler);
BOOL lister_select_cache(Lister *, struct DirectoryBuffer *);
void lister_set_gauge(Lister *, BOOL);

void lister_init_colour(Lister *, short, short);
void lister_init_display_colours(Lister *);
short lister_colour_pen(Lister *, short, short);

#define GETICONSF_CLEAN (1 << 0)
#define GETICONSF_NO_REFRESH (1 << 1)
#define GETICONSF_SHOW (1 << 2)

BOOL lister_check_ftp(Lister *, char *);
BOOL lister_want_gauge(Lister *);

struct Window *lister_valid_window(Lister *);
void lister_zoom_window(Lister *);

struct BitMap *builddisplaystring_prop(struct DirectoryEntry *, char *, Lister *, short);

UWORD lister_listerpopup(Lister *, UWORD);

void lister_edit_calc_position(Lister *lister, short type);

void lister_title_highlight_sep(Lister *lister);

#define LISTER_TABSIZE 3

#endif
