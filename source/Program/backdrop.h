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

#ifndef _DOPUS_BACKDROP
#define _DOPUS_BACKDROP

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct _BackdropObject
{
	struct Node node;

	short type;	  // Type of object
	short state;  // Object state

	struct DiskObject *icon;  // Object icon
	char *name;				  // Object name
	char *path;				  // Path name
	struct DateStamp date;	  // Object date
	char *device_name;		  // Device name (for disks)

	unsigned long flags;  // Flags

	struct IBox pos;			  // Object position
	struct Rectangle show_rect;	  // Rectangle we display in
	struct Rectangle full_size;	  // Full size of object
	struct Rectangle image_rect;  // Rectangle for image
	ULONG custom_pos;			  // Custom position

	UWORD *image_mask[2];  // Image mask

	DragInfo *drag_info;  // Drag info for this object
	short drag_x_offset;
	short drag_y_offset;

	IPTR misc_data;			 // Miscellaneous data
	unsigned long value;	 // Value for position-sorting
	Cfg_Filetype *filetype;	 // Filetype

#ifdef USE_64BIT
	ULONG pen;	 // custom pen for AppIcons
	UQUAD size;	 // File size
#else
	unsigned long size;	 // File size
#endif
} BackdropObject;

typedef struct _BackdropInfo
{
	struct Window *window;				 // Window pointer
	struct SignalSemaphore window_lock;	 // Lock on window
	struct List boopsi_list;			 // BOOPSI list
	struct Gadget *vert_scroller;		 // Scrollers
	struct Gadget *horiz_scroller;

	struct ListLock objects;  // Buffer pointer
	unsigned short flags;	  // Flags

	long virt_width;  // Virtual window size
	long virt_height;
	long offset_x;	// Offset in display
	long offset_y;

	struct Rectangle area;	  // Virtual area needed
	struct Rectangle size;	  // Physical area available
	struct Rectangle select;  // Select area

	struct Region *clip_region;	 // Region for clipping

	BackdropObject *last_sel_object;  // Last selected object
	BackdropObject *first_sel_tool;	  // First tool selected
	unsigned long seconds;			  // Double-click info
	unsigned long micros;
	struct Gadget *down_gadget;	 // Gadget held down

	char buffer[512];  // Some memory to play with
	APTR memory;	   // memory handle to use

	short last_x, last_y;  // Last drag positions

	BackdropObject *current_sel_object;	 // Currently selected object

	struct MsgPort *notify_port;  // Notification port
	APTR notify_req;

	short top_border;
	short bottom_border;
	short left_border;
	short right_border;

	struct RastPort rp;	 // Rastport

	short last_x_pos;
	short last_y_pos;

	unsigned long tick_count;  // Intuiticks count
	unsigned long last_tick;

	Lister *lister;	 // Lister pointer
	IPCData *ipc;	 // Owner IPC

	unsigned short last_button;

	struct SignalSemaphore idcmp_lock;	// Get IDCMP lock

	struct TextFont *font;	// Font for icons

	struct Region *icon_space;	// Free icon space

	REF_CALLBACK callback;	// Refresh callback

	ULONG drag_sec;	 // Drag start seconds
	ULONG drag_mic;	 // Drag start micros

	struct Region *temp_region;
} BackdropInfo;

#ifndef __amigaos3__
	#pragma pack()
#endif

enum {
	BDO_NO_TYPE = -1,
	BDO_DISK,
	BDO_LEFT_OUT,
	BDO_APP_ICON,
	BDO_BAD_DISK,
	BDO_GROUP,
};

#define BDOF_SELECTED (1 << 0)		 // This object is selected
#define BDOF_TEMP_LEFTOUT (1 << 1)	 // Temporarily left-out
#define BDOF_AUTO_POSITION (1 << 2)	 // Position icon automatically
#define BDOF_ICON_VIEW (1 << 3)		 // Object is in an iconview lister
#define BDOF_FAKE_ICON (1 << 4)		 // Not a real icon

#define BDOF_STATE_CHANGE (1 << 6)	   // State changed
#define BDOF_CUSTOM_POS (1 << 7)	   // Custom position
#define BDOF_NO_LABEL (1 << 8)		   // No label
#define BDOF_NO_POSITION (1 << 9)	   // No position set
#define BDOF_CUSTOM_LABEL (1 << 10)	   // Custom label
#define BDOF_LEFTOUT_POS (1 << 11)	   // Left-out position
#define BDOF_LINK_ICON (1 << 12)	   // Icon for a link
#define BDOF_ASSIGN (1 << 13)		   // Not really a disk
#define BDOF_CACHE (1 << 14)		   // Cache directory
#define BDOF_BACKGROUND (1 << 15)	   // Background colour
#define BDOF_LOCKED (1 << 16)		   // Position locked
#define BDOF_BUSY (1 << 17)			   // Busy
#define BDOF_BORDER_ADJUST (1 << 18)   // Adjust for border
#define BDOF_SPECIAL (1 << 19)		   // Special flag
#define BDOF_DESKTOP_FOLDER (1 << 20)  // In desktop folder
#define BDOF_GHOSTED (1 << 21)		   // Icon to be rendered ghosted
#define BDOF_REMAPPED (1 << 22)		   // Icon has been remapped at least once
#define BDOF_SHOW_PENDING (1 << 23)   // LISTER_SHOW_ICON will draw it first

#define BDOF_TEMP_FLAG (1 << 26)	 // A temporary flag
#define BDOF_REFRESH (1 << 27)		 // Icon needs to be refreshed
#define BDOF_AUTO_OPEN (1 << 28)	 // Automatically open this group
#define BDOF_SAVE_LEFTOUT (1 << 29)	 // Need to save left outs
#define BDOF_DO_LEFT_OUT (1 << 30)	 // Need to do left outs
#define BDOF_OK (1 << 31)			 // Object is ok

#define BDIF_DRAGGING (1 << 0)			// Currently dragging
#define BDIF_IGNORE_GADGET (1 << 1)		// Ignore the next gadget
#define BDIF_NEW_SIZE (1 << 2)			// Just re-sized
#define BDIF_RUBBERBAND (1 << 3)		// Rubber-banding select area
#define BDIF_ANT_ON (1 << 4)			// Which ants?
#define BDIF_TICK_SKIP (1 << 5)			// Skip this tick?
#define BDIF_KEY_SELECTION (1 << 6)		// Key selection mode on
#define BDIF_LAST_POS_OK (1 << 7)		// Last position is valid
#define BDIF_REFRESHED (1 << 8)			// Already refreshed
#define BDIF_ORDERING (1 << 9)			// Ordering icons
#define BDIF_LAST_SORT_HORIZ (1 << 10)	// Last sort was horizontal
#define BDIF_MAIN_DESKTOP (1 << 11)		// Main desktop
#define BDIF_GROUP (1 << 12)			// Program group
#define BDIF_CUSTOM_DRAG (1 << 13)		// Custom drag routines
#define BDIF_SHOW_ALL (1 << 14)			// Show All (Listers only)

#define BACKDROP_POS_OK (1 << 31)
#define BACKDROP_GROUP_OBJECT (1 << 30)
#define BACKDROP_ICON_VIEW (1 << 29)

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct
{
	IPCData *ipc;
	struct Screen *screen;
	struct IBox dimensions;
	char name[32];
	BackdropObject *object;

	struct Window *window;
	BackdropInfo *info;

	struct AppWindow *appwindow;
	struct MsgPort *appport;

	TimerHandle *timer;
	BOOL got_quit;

	struct TextFont *font;
	short error_count;

	WindowID id;  // Window ID pointer

	PatternInstance pattern;  // Backdrop pattern

	PopUpHandle *popup;
} GroupData;

#ifndef __amigaos3__
	#pragma pack()
#endif

#define GETICON_CD (1 << 0)
#define GETICON_KEEP (1 << 1)
#define GETICON_SAVE_POS (1 << 2)
#define GETICON_NO_POS (1 << 3)
#define GETICON_POS_ONLY (1 << 4)
#define GETICON_FAIL (1 << 5)
#define GETICON_DEFDIR (1 << 6)
#define GETICON_DEFAULT (1 << 7)
#define GETICON_REMAP (1 << 8)

#define BIDCMPF_DOUBLECLICK (1 << 0)
#define BIDCMPF_LISTER (1 << 1)

#define BDSF_CLEAR (1 << 0)
#define BDSF_CLEAR_ONLY (BDSF_CLEAR | (1 << 1))
#define BDSF_REFRESH (1 << 2)
#define BDSF_REFRESH_DONE (BDSF_REFRESH | (1 << 3))
#define BDSF_RECALC (1 << 4)
#define BDSF_IN_REFRESH (1 << 5)
#define BDSF_RESET (1 << 6)
#define BDSF_NO_CLIP (1 << 7)

#define BDNF_CD (1 << 0)
#define BDNF_KEEP (1 << 1)
#define BDNF_RECALC (1 << 2)
#define BDNF_NO_DISKS (1 << 3)
#define BDNF_NO_ARRANGE (1 << 4)

BackdropInfo *backdrop_new(IPCData *, ULONG);
void backdrop_free(BackdropInfo *info);

BackdropObject *backdrop_new_object(BackdropInfo *, char *, char *, short);

void backdrop_init_info(BackdropInfo *, struct Window *, short);
void backdrop_free_remap(BackdropInfo *, struct Window *);
void backdrop_free_info(BackdropInfo *);
void backdrop_free_list(BackdropInfo *);
void backdrop_remove_object(BackdropInfo *, BackdropObject *);
void backdrop_scroll_objects(BackdropInfo *, short, short);
void backdrop_show_objects(BackdropInfo *, UWORD);
void backdrop_render_object(BackdropInfo *, BackdropObject *, UWORD);
void backdrop_draw_object(BackdropInfo *, BackdropObject *, UWORD, struct RastPort *, short, short);
BOOL backdrop_cleanup_objects(BackdropInfo *, struct Rectangle *, UWORD);
void backdrop_lineup_objects(BackdropInfo *);
long backdrop_arrange_objects(BackdropInfo *);
BackdropObject *backdrop_get_object(BackdropInfo *info, short x, short y, UWORD flags);
BackdropObject *backdrop_icon_in_rect(BackdropInfo *, struct Rectangle *);
BackdropObject *backdrop_icon_in_rect_full(BackdropInfo *, struct Rectangle *);
void backdrop_object_open(BackdropInfo *, BackdropObject *, UWORD, BOOL, long, struct WBArg *);
void backdrop_update_window(BackdropInfo *, BackdropObject *, struct ListerWindow *);
BOOL backdrop_start_drag(BackdropInfo *, short, short);
BOOL backdrop_drag_object(BackdropInfo *, BackdropObject *);
BOOL backdrop_stop_drag(BackdropInfo *);
BOOL backdrop_check_deadlocks(BackdropInfo *);
void backdrop_show_drag(BackdropInfo *, BackdropObject *, short, short);
void backdrop_check_pos(BackdropInfo *);
void backdrop_info(BackdropInfo *, BOOL, BackdropObject *);
void backdrop_snapshot(BackdropInfo *, BOOL, BOOL, BackdropObject *);
void backdrop_select_all(BackdropInfo *, short);
BOOL backdrop_cleanup(BackdropInfo *, short, UWORD);
BOOL backdrop_resize_to_fit(BackdropInfo *);
void backdrop_check_change(BackdropInfo *, BackdropObject *);
void backdrop_get_leftouts(BackdropInfo *, BackdropObject *);
BackdropObject *backdrop_leftout_new(BackdropInfo *, char *, char *, ULONG);
void backdrop_get_icon(BackdropInfo *, BackdropObject *, short);
void backdrop_free_icon(BackdropInfo *, struct DiskObject *);
void backdrop_leave_icons_out(BackdropInfo *info, BackdropObject *, BOOL);
void backdrop_save_leftouts(BackdropInfo *);
BackdropObject *find_backdrop_object(BackdropInfo *, BackdropObject *);
void backdrop_putaway(BackdropInfo *, BackdropObject *);
void backdrop_calc_virtual(BackdropInfo *);
void backdrop_calc_virtual_size(BackdropInfo *, struct Rectangle *);
void backdrop_scroll(BackdropInfo *, short, short);
void backdrop_pos_slider(BackdropInfo *, short);
BackdropObject *backdrop_find_disk(BackdropInfo *, char *);
void backdrop_update_leftouts(BackdropInfo *);
void backdrop_format(BackdropInfo *, BackdropObject *);
void backdrop_eject(BackdropInfo *, BackdropObject *);
void backdrop_drop_object(BackdropInfo *, BackdropObject *);

BOOL backdrop_idcmp(BackdropInfo *, struct IntuiMessage *, unsigned short flags);
void backdrop_install_clip(BackdropInfo *);
void backdrop_rubber_band(BackdropInfo *, BOOL);
void backdrop_select_area(BackdropInfo *, short);

short geo_box_intersect(struct Rectangle *, struct Rectangle *);

BackdropObject *backdrop_add_appicon(AppEntry *appicon, BackdropInfo *info, short);
void backdrop_rem_appicon(BackdropInfo *info, AppEntry *icon);
struct _DOpusAppMessage *backdrop_appmessage(BackdropInfo *info, BOOL);
void backdrop_drop_appwindow(BackdropInfo *info, struct AppWindow *appwindow, short, short);
void backdrop_sort_objects(BackdropInfo *info, short, BOOL);
void backdrop_image_bitmap(BackdropInfo *, struct Image *, UWORD *, struct BitMap *);

BPTR backdrop_icon_lock(BackdropObject *object);

void backdrop_read_groups(BackdropInfo *info);
void backdrop_open_group(BackdropInfo *info, BackdropObject *object, BOOL);
GroupData *backdrop_find_group(BackdropObject *object);
BOOL backdrop_show_group(GroupData *startup);
void backdrop_group_get_font(GroupData *group);
void backdrop_hide_group(GroupData *startup);
void backdrop_free_group(GroupData *startup);
void backdrop_read_group_objects(GroupData *startup);
void backdrop_check_groups(BackdropInfo *info);
void backdrop_group_add_object(char *, BackdropInfo *, char *, short, short);
void backdrop_delete_group(BackdropInfo *info, BackdropObject *object);
void backdrop_remove_group_objects(GroupData *data, BackdropObject *);
BOOL backdrop_group_do_function(GroupData *, IPTR, struct MenuItem *);

BOOL backdrop_new_group_object(BackdropInfo *info, BackdropObject *object, unsigned short);
BOOL backdrop_check_notify(BackdropInfo *info, DOpusNotify *notify, Lister *lister);

void backdrop_snapshot_group(BackdropInfo *info, char *name);
void backdrop_read_appicons(BackdropInfo *info, short);
BackdropObject *backdrop_find_appicon(BackdropInfo *info, AppEntry *appicon);

void backdrop_update_size(BackdropInfo *);

void backdrop_make_visible(BackdropInfo *info, BackdropObject *object);

#define BAPPF_MENU (1 << 15)
#define BAPPF_HELP (1 << 14)
#define BAPPF_CLOSE (1 << 0)
#define BAPPF_UNSNAPSHOT (1 << 1)
#define BAPPF_INFO (1 << 2)

#define BAPPF_ID(id) (id & ~(BAPPF_MENU | BAPPF_HELP))

void backdrop_appicon_message(BackdropObject *object, unsigned short flags);

ULONG backdrop_image_checksum(BackdropObject *object, short which);
void backdrop_get_masks(BackdropObject *object);
BOOL backdrop_masks_misplaced(BackdropObject *object);

short backdrop_get_devices(BackdropInfo *info);
void backdrop_do_leftouts(BackdropInfo *info, BackdropObject *onlyone);
short backdrop_cleanup_list(BackdropInfo *info, long type);
void backdrop_refresh_drives(BackdropInfo *info, BOOL);
void backdrop_remove_leftouts(BackdropInfo *info, BackdropObject *disk);

BOOL backdrop_test_rmb(BackdropInfo *, struct IntuiMessage *, struct IntuiMessage *, ULONG);
BOOL backdrop_popup(BackdropInfo *, short, short, UWORD, long, char *, DirEntry *);

#define BPOPF_ICONS (1 << 0)
#define BPOPF_DIRENTRY (1 << 1)

void backdrop_clear_region(BackdropInfo *info);
void backdrop_add_region(BackdropInfo *info, struct Rectangle *rect);
void backdrop_region_bounds(BackdropInfo *info);

void backdrop_fix_count(BackdropInfo *info, BOOL);

void backdrop_place_object(BackdropInfo *info, BackdropObject *object);
void backdrop_icon_size(BackdropInfo *, BackdropObject *, short *, short *);

BackdropObject *backdrop_next_object(BackdropInfo *info, BackdropObject *object, BackdropObject *only_one);

BOOL backdrop_remove_leftout(BackdropObject *object);

void backdrop_add_leftouts(BackdropInfo *info);
BackdropObject *backdrop_leave_out(BackdropInfo *info, char *name, ULONG flags, short, short);

#define BLNF_CUSTOM_LABEL (1 << 0)

#define BLOF_PERMANENT (1 << 0)
#define BLOF_REFRESH (1 << 1)

#define BDEVF_DISK_REMOVED (1 << 0)
#define BDEVF_DISK_ADDED (1 << 1)

#define BDEVF_FORCE_LOCK (1 << 0)
#define BDEVF_MARK_OK (1 << 1)
#define BDEVF_SHOW (1 << 2)

#define ID_BUSY 0x42555359

void backdrop_get_objects(BackdropInfo *, short);

#define BGOF_DISKS (1 << 0)
#define BGOF_APPICONS (1 << 1)
#define BGOF_LEFTOUT (1 << 2)
#define BGOF_GROUPS (1 << 3)

#define BGOF_DESKTOP (1 << 7)
#define BGOF_SHOW (1 << 8)
#define BGOF_FIRST (1 << 9)
#define BGOF_ALL (BGOF_DISKS | BGOF_APPICONS | BGOF_LEFTOUT | BGOF_GROUPS)

#define BDAF_SHOW (1 << 0)
#define BDAF_LOCK (1 << 1)
#define BDAF_CHANGE (1 << 2)
#define BDAF_NEW_IMAGE (1 << 3)

enum { BSORT_NORM, BSORT_HORIZ, BSORT_NAME, BSORT_TYPE, BSORT_SIZE, BSORT_DATE };

void backdrop_command(BackdropInfo *info, ULONG cmd, ULONG flags);
Cfg_Filetype *backdrop_get_filetype(BackdropInfo *info, BackdropObject *object);

short backdrop_check_icons_ok(BackdropInfo *info);

BOOL backdrop_handle_button(BackdropInfo *info, struct IntuiMessage *msg, unsigned short flags);

void backdrop_erase_icon(BackdropInfo *info, BackdropObject *object, UWORD flags);

void backdrop_run_build_args(Att_List *list, BackdropInfo *info, BackdropObject *exclude);

void backdrop_selection_list(BackdropObject *object);

void backdrop_get_font(BackdropInfo *info);

#define BRENDERF_CLEAR (1 << 0)
#define BRENDERF_CLIP (1 << 1)
#define BRENDERF_LABEL (1 << 2)
#define BRENDERF_REAL (1 << 3)
#define BRENDERF_ERASED (1 << 4)  // Display under the object was just cleared
#define BRENDERF_NEW (1 << 5)	  // Object is not on display yet; draw it on top

unsigned short *backdrop_space_array(BackdropInfo *info, Point *size);
void backdrop_space_add_icon(BackdropInfo *, unsigned short *array, BackdropObject *icon, Point *size);
short backdrop_find_space(BackdropInfo *info, BackdropObject *icon, unsigned short *array, Point *size);

void backdrop_icon_position(BackdropInfo *info, BackdropObject *icon, short x, short y);

short backdrop_get_copy_path(PopUpItem *item, char *path);

void popup_get_appicon(PopUpHandle *, AppEntry *);
BOOL popup_get_filetype(PopUpHandle *, BackdropInfo *, MatchHandle *, BackdropObject *, char *, short, ULONG *);

void backdrop_show_rect(BackdropInfo *, BackdropObject *, short, short);
void backdrop_position_object(BackdropInfo *, BackdropObject *, long *, long *, short, short, struct MinList *);

struct MinList *backdrop_build_poslist(BackdropInfo *, ULONG);
void backdrop_free_poslist(struct MinList *);

void popup_build_copyto(PopUpHandle *, PopUpItem *);
void popup_build_openwith(PopUpHandle *);

BackdropObject *backdrop_create_shortcut(BackdropInfo *, char *, short, short);
void backdrop_unique_name(char *);

void backdrop_check_size(BackdropInfo *);

#define WBICONMAGIC_X 4
#define WBICONMAGIC_Y 3

ULONG get_list_type(BackdropObject *);

// Flags for backdrop_get_object()
#define BDGOF_CHECK_MASK (1 << 0)
#define BDGOF_CHECK_LABEL (1 << 1)

void backdrop_draw_icon_ghost(struct RastPort *, struct Rectangle *, PLANEPTR);
BOOL backdrop_icon_border(BackdropObject *);

enum {
	WBVIEWMODE_DEFAULT,
	WBVIEWMODE_ICON,
	WBVIEWMODE_NAME,
	WBVIEWMODE_DATE,
	WBVIEWMODE_SIZE,
};

#define WBSHOWFLAG_SHOWALL (1 << 1)
#define WBSHOWFLAG_ACTION (1 << 28)

void popup_default_menu(BackdropInfo *, PopUpHandle *, short *);
BOOL popup_lister_view_menu(Lister *, PopUpHandle *);
MatchHandle *popup_build_icon_menu(BackdropInfo *, char *, BackdropObject *, ULONG, short *, PopUpHandle *);

void backdrop_bad_disk_name(BackdropObject *, char *);

void backdrop_check_grid(long *, long *);

void backdrop_replace_icon_image(BackdropInfo *, char *, BackdropObject *);
BOOL backdrop_goodbad_check(char *device, struct List *dos_list);

#define CLEANUPF_ALIGN_OK (1 << 0)
#define CLEANUPF_CHECK_POS (1 << 1)

#define ICON_BORDER_X 4
#define ICON_BORDER_Y_TOP 3
#define ICON_BORDER_Y_BOTTOM 2
#define CLEANUP_SPACE_X 7
#define CLEANUP_SPACE_Y 5
#define CLEANUP_START_X 3
#define CLEANUP_START_Y 7
#define ICON_LABEL_SPACE 3

#define BTRM_WINDOW   0
#define BTRM_NOBORDER 1
#define BTRM_ICON     2

#endif
