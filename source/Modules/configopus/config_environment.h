#define SCRIPTF_NO_LOCK (1 << 0)
#define SCRIPTF_NO_SOUND (1 << 1)

enum {
	PICMENU_TYPE,
	PICMENU_CENTERED,
	PICMENU_TILED,
	PICMENU_STRETCHED,
	PICMENU_REMAP,
	PICMENU_NONE,
	PICMENU_GUI,
	PICMENU_ICON,
	PICMENU_IMAGE,
	PICMENU_EXACT,
	PICMENU_BORDER,
	PICMENU_NORMAL,
	PICMENU_BLACK,
	PICMENU_WHITE,
};

#ifndef __amigaos3__
	#pragma pack(2)
#endif
typedef struct
{
	NewConfigWindow newwin;

	struct Window *window;
	ObjectList *objlist;
	ObjectList *option_list;
	CFG_ENVR *config;

	Att_List *mode_list;				 // List of screen modes
	char mode_name[128];				 // Current mode name
	struct Rectangle mode_size_limit;	 // Limits of screen mode size
	struct Rectangle mode_size_default;	 // Screen mode default size
	unsigned short mode_max_colours;	 // Screen mode max colours
	short pad;

	// struct Library		*GfxBase;
	ULONG pad2;

	short palette_colours;	  // Number of colours in the palette
	short palette_count;	  // Total number of colours
	short palette_array[16];  // Palette array
	short *palette_table;

	short sel_colour;  // Colour selected in palette
	char gun_bits[4];  // Bits per gun
	short slide_gun;   // Gun we are sliding

	Att_List *options;

	Att_List *lister_items;			 // Lister display items
	struct FontRequester *font_req;	 // Font requester

	ULONG pad1;

	struct MsgPort *app_port;
	struct AppWindow *appwindow;

	GL_Object *example;
	UBYTE *colour_selection;

	short palette_edit_pens[16];
	unsigned short palette_edit_penalloc;
	unsigned short palette_edit_freepen;
	short alloc_pen_count;

	short output_dims[4];
	IPCData *ipc;

	short own_screen;
	Cfg_Environment *env;

	IPCData *main_ipc;

	short option;
	UWORD initial_pen_alloc;
	Att_Node *option_node;

	unsigned long font_pens;
	unsigned short font_pen_count;
	unsigned char font_pen_table[16];

	Att_List *desktop_drives[2];

	struct FontRequester *icon_font_req;  // Font requester

	short orig_colour_sel;
	GL_Object *palette_gadget;
	short cyber_flag;
	short colour_number;

	short custom_pen[2];

	short hide_type;

	char last_saved[256];
	char *settings_name;

	Att_List *icon_settings;
	Att_List *path_list;

	CfgDragInfo drag;

	struct MinList sound_list;
	Att_List *script_list;

	// Desired state of the dopus/DOSPatch ENV var for the open dialog.
	// `dos_patch_initial` is captured at dialog open from the current
	// ENVARC: state; `dos_patch_state` mirrors the checkbox; the stored
	// state is committed by _config_env_store(LISTER_DISPLAY) which sets
	// `dos_patch_modified` only when the new state differs from the
	// initial snapshot - so a user who toggles ON then OFF (or vice
	// versa) and saves doesn't trigger a redundant ENVARC: write.  The
	// success block at the bottom of L_Config_Environment writes/deletes
	// the ENVARC: variable only when modified is set.
	BOOL dos_patch_initial;
	BOOL dos_patch_state;
	BOOL dos_patch_modified;
} config_env_data;
#ifndef __amigaos3__
	#pragma pack()
#endif

extern const ObjectDef _lister_format_gadgets[];

BOOL _config_env_open(config_env_data *, struct Screen *);
void _config_env_close(config_env_data *);
void _config_env_cleanup(config_env_data *data);
void _config_env_set(config_env_data *data, short option);
void _config_env_store(config_env_data *data, short option);
void _config_env_screenmode_init(config_env_data *data, BOOL);
void _config_env_screenmode_fix_gadgets(config_env_data *data);
void _config_env_screenmode_init_mode(config_env_data *data, BOOL);
void _config_env_screenmode_fix_gadgets(config_env_data *data);
void _config_env_screenmode_set_mode(config_env_data *data, Att_Node *node);
void _config_env_screenmode_check_values(config_env_data *data);
void _config_env_palette_update_sliders(config_env_data *data);
void _config_env_palette_change_colour(config_env_data *data, int gun, ULONG val);
void _config_env_palette_redraw(config_env_data *data, short pen);
void _config_env_palette_presets(config_env_data *data);
void set_rgb32(config_env_data *data, short col, ULONG *palette);
BOOL get_palette_file(config_env_data *data, char *name);
void _config_env_update_example(config_env_data *data, UBYTE *ptr);
void _config_env_palette_init(config_env_data *data);
// ULONG STDARGS my_SetGadgetAttrs(struct Gadget *gad,struct Window *win,Tag tag,...);
void _config_env_colours_init(config_env_data *data);
void _config_env_update_output(config_env_data *data);
void _config_env_update_output_dims(config_env_data *data);

// struct Window *STDARGS my_OpenWindowTags(Tag tag,...);
IPCMessage *_config_env_output_window_set(config_env_data *data, UWORD);
void _config_env_size_instructions(struct Window *window, UWORD);
void _config_env_reset_palette(config_env_data *data);
void _config_env_load_palette(config_env_data *data);
void _config_env_update_listersize(config_env_data *data);
void _config_env_status_list(ObjectList *list, ULONG id, long, long);
void _config_env_clock_format_list(ObjectList *list, ULONG id, long, long);
void _config_env_fix_font_pens(config_env_data *data);
unsigned char _env_map_font_colour(config_env_data *data, unsigned char col);
void _config_env_build_drive_list(config_env_data *data, short);
void _config_env_fix_drive_list(config_env_data *data, short);
BOOL _config_env_update_drives_list(config_env_data *data, short);
void config_env_check_device(config_env_data *data);

void _env_select_icon_font(config_env_data *data,
						   char *name,
						   UBYTE *size,
						   UBYTE *fpen,
						   UBYTE *bpen,
						   UBYTE *mode,
						   short bufsize);

enum {
	MAINCMD_BASE = 0x40000,
	MAINCMD_RESET,			  // Reset display
	MAINCMD_CLOSE_DISPLAY,	  // Close display
	MAINCMD_OPEN_DISPLAY,	  // Open display
	MAINCMD_GET_SCREEN_DATA,  // Get screen data
	MAINCMD_RESET_TOOLMENU,	  // Reset Tools menu
	MAINCMD_ADD_APPWINDOW,
	MAINCMD_REM_APPWINDOW,

	GROUP_NEW_FONT,		 // New font for Groups
	GROUP_NEW_NAME,		 // Group's name changed
	GROUP_ADD_ICON,		 // Show icon in group
	GROUP_NEW_BACKFILL,	 // Backfill pattern changed

	REXXCMD_INCREMENT_MSG,	// Increment message count

	GROUP_DELETE,  // Delete from group

	MAINCMD_COMMAND,  // Command

	MAINCMD_GET_LIST,  // Get command list

	MAINCMD_SAVE_POSITIONS,	 // Save position list
	MAINCMD_LOAD_POSITIONS,	 // Load position list

	REXXCMD_SEND_MSG,	 // Send a REXX command
	REXXCMD_SEND_RXMSG,	 // Send a REXX message

	MAINCMD_GET_PEN,	  // Get custom pen
	MAINCMD_RELEASE_PEN,  // Free custom pen

	MAINCMD_GET_ICON,  // Get an icon

	MAINCMD_SAVE_ENV,  // Save environment
};

extern const MenuData button_toolbar_menu[];

void config_env_fix_picture_gads(config_env_data *);
void config_env_picture_menu(config_env_data *, short);

void _config_env_screenmode_show_depth(config_env_data *);

void config_env_build_pathlist(config_env_data *, BOOL);
void config_env_path_add(config_env_data *);
void config_env_path_sel(config_env_data *);
void config_env_path_del(config_env_data *);
void config_env_path_edit(config_env_data *);
void config_env_paths_end_drag(config_env_data *, BOOL);

extern const UWORD _environment_iconsettings_labels[];

void config_env_show_sound(config_env_data *);
void config_env_store_sound(config_env_data *);
void config_env_test_sound(config_env_data *);
void env_init_sounds(config_env_data *);
