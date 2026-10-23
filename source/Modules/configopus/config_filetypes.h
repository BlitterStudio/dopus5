#ifndef __amigaos3__
	#pragma pack(2)
#endif
typedef struct
{
	Cfg_Filetype *type;
	IPCData *editor;
} FiletypeNode;

typedef struct
{
	short match_type;
	char match_data[80];
} FileclassNode;

typedef struct
{
	NewConfigWindow newwin;	 // New window definition
	struct Window *window;	 // Our window
	ObjectList *objlist;	 // Object list

	struct List list_list;	  // List of filetype lists
	Att_List *filetype_list;  // List of filetypes
	Att_Node *sel_filetype;	  // Selected filetype

	struct ListLock proc_list;	// List of launched processes
	IPCData *ipc;				// Our IPC port

	APTR memory;  // Memory for filetype storage

	ULONG seconds;	// Seconds for double-click
	ULONG micros;	// Micros for double-click

	IPTR command_list;
	IPCData *owner_ipc;

	short change;
} config_filetypes_data;

typedef struct
{
	IPCData *ipc;		 // IPC pointer
	IPCData *owner_ipc;	 // Owner's IPC port

	FiletypeNode *node;	 // Filetype node to edit
	Cfg_Filetype *type;	 // Copy of filetype

	FunctionStartup func_startup;  // Data for function editor

	const ObjectDef *obj_def;	 // Pointer to objects

	NewConfigWindow new_win;  // New window data
	struct Window *window;	  // Window pointer
	ObjectList *objlist;	  // Object list

	struct ListLock proc_list;	// List of processes
	IPCData *editor[32];		// Function editors

	Att_List *action_list;	// Action list
	const short *action_lookup;	// Lookup table

	IPCData *class_editor;	  // File class editor
	const ConfigWindow *class_win;  // Class editor window
	const ObjectDef *class_obj;	  // Class editor objects
	const short *class_lookup;	  // Class lookup table
	const UWORD *class_strings;// Match strings
	short edit_flag;		  // Go straight to class editor?

	APTR icon_image;				// Icon image
	struct FileRequester *filereq;	// File requester

	struct MsgPort *app_port;  // AppWindow stuff
	struct AppWindow *app_window;

	ULONG last_sec, last_mic;
	Att_Node *last_sel;

	Att_List *icon_list;
	Att_Node *last_icon;

	CfgDragInfo drag;
} filetype_ed_data;

typedef struct
{
	IPCData *ipc;		 // IPC port
	IPCData *owner_ipc;	 // Owner's IPC port

	Cfg_Filetype *type;	 // Copy of recognition string

	// struct Library		*dopus_base;	// Library pointers
	// struct Library		*dos_base;
	// struct Library		*int_base;
	// struct Library		*gfx_base;
	ULONG pad1[4];

	NewConfigWindow new_win;  // New window data
	struct Window *window;	  // Window pointer
	const ObjectDef *obj_def;	  // Object definitions
	ObjectList *objlist;	  // Object list

	const short *lookup;	// Class lookup table
	const UWORD *strings;

	Att_List *match_list;  // List of matches
	Att_Node *match_edit;  // Current match being edited

	Att_List *command_list;	 // List of match commands
	short last_selection;

	IPCData *main_owner;  // Main program

	// struct Library		*rexx_base;	// Rexx library
	ULONG pad2;
	struct MsgPort *reply_port;	 // ReplyPort

	// struct Library		*asl_base;
	ULONG pad3;

	struct MinList readers;	 // Text readers
} fileclass_ed_data;

typedef struct
{
	struct MinNode node;
	IPTR handle;
} ReaderNode;
#ifndef __amigaos3__
	#pragma pack()
#endif

extern const short filetype_action_lookup[];
extern const short fileclass_match_lookup[];
extern const UWORD matchtype_labels[];

enum {
	FTTYPE_USER_1,			   // UserFunc1
	FTTYPE_USER_2,			   // UserFunc2
	FTTYPE_DOUBLE_CLICK,	   // Double-click
	FTTYPE_DRAG_DROP,		   // Drag 'n' drop
	FTTYPE_CTRL_DOUBLECLICK,   // Control double-click
	FTTYPE_USER_3,			   // UserFunc3
	FTTYPE_ALT_DOUBLECLICK,	   // Alt double-click
	FTTYPE_ALT_DRAGDROP,	   // Alt drag/drop
	FTTYPE_CTRL_DRAGDROP,	   // Control drag/drop
	FTTYPE_USER_4,			   // UserFunc4
	FTTYPE_SHIFT_DOUBLECLICK,  // Shift double-click
	FTTYPE_SHIFT_DRAGDROP,	   // Shift drag/drop
	FTTYPE_USER_5,			   // UserFunc5
	FTTYPE_USER_6,			   // UserFunc6
	FTTYPE_USER_7,			   // UserFunc7
	FTTYPE_USER_8,			   // UserFunc8
	FTTYPE_USER_9,			   // UserFunc9
	FTTYPE_USER_10,			   // UserFunc10
};

enum {
	FTOP_NOOP,			// No operation
	FTOP_MATCH,			// Match text
	FTOP_MATCHNAME,		// Match filename
	FTOP_MATCHBITS,		// Match protection bits
	FTOP_MATCHCOMMENT,	// Match comment
	FTOP_MATCHSIZE,		// Match size
	FTOP_MATCHDATE,		// Match date
	FTOP_MOVETO,		// Move to absolute location
	FTOP_MOVE,			// Move to relative location
	FTOP_SEARCHFOR,		// Search for text
	FTOP_MATCHFORM,		// Match an IFF FORM
	FTOP_FINDCHUNK,		// Find an IFF chunk
	FTOP_MATCHDTGROUP,	// Match datatypes group
	FTOP_MATCHDTID,		// Match datatypes ID
	FTOP_MATCHNOCASE,	// Match text case insensitive
	FTOP_DIRECTORY,		// Match directory
	FTOP_MODULE,		// Sound module
	FTOP_DISK,			// Match disk
	FTOP_SEARCHRANGE,	// Search for text (limited range)
	FTOP_MATCHCHUNK,	// Match an IFF chunk

	FTOP_LAST,	// Last valid command

	FTOP_SPECIAL = 252,	 // Start of special instructions

	FTOP_OR,		 // Or
	FTOP_AND,		 // And
	FTOP_ENDSECTION	 // End of a section
};

typedef struct
{
	Cfg_Function *func;
	IPCData *editor;
} func_node;

/* Internal flag bit set on a Cfg_Function created by
 * filetypeed_add_iconmenu to mark it as a not-yet-accepted placeholder.
 * filetypeed_check_iconmenu treats functions carrying this bit as needing
 * cleanup (so an Add-then-Cancel round-trip discards the row even when
 * the seeded INST_LABEL keeps the instructions list non-empty).
 *
 * The bit lives on Cfg_Function rather than func_node because update_iconmenu
 * recreates every func_node from scratch via AllocVec(MEMF_CLEAR) — any
 * func_node-side flag would get wiped whenever a parallel placeholder
 * triggered a rebuild. The Cfg_Function survives that rebuild, so the
 * marker survives with it.
 *
 * filetypeed_receive_edit clears the bit on the func_copy that replaces
 * the placeholder, so it never reaches a save path; the existing
 * FUNCF2_ORIGINAL / FUNCF2_FREE_FUNCTION public defines establish the
 * precedent of using high flags2 bits for transient module-private state.
 */
#define FUNCF2_PLACEHOLDER (1 << 29)

void filetype_read_list(APTR, struct List *);
void filetype_build_list(config_filetypes_data *);
Att_Node *filetype_add_entry(config_filetypes_data *, Cfg_Filetype *);
void filetype_edit(config_filetypes_data *, FiletypeNode *, short);
BOOL filetype_receive_edit(config_filetypes_data *, Cfg_Filetype *, FiletypeNode *);
BOOL filetype_remove(config_filetypes_data *, Att_Node *, short);
short filetype_save(config_filetypes_data *);

void FiletypeEditor(void);
void filetypeed_update_actions(filetype_ed_data *);
void filetypeed_edit_action(filetype_ed_data *, short, char *);
short filetypeed_receive_edit(filetype_ed_data *, FunctionReturn *);
void filetypeed_edit_definition(filetype_ed_data *);
void filetypeed_receive_class(filetype_ed_data *, Cfg_Filetype *);
void filetypeed_show_icon(filetype_ed_data *);
BOOL filetypeed_pick_icon(filetype_ed_data *);

void FileclassEditor(void);
void _fileclassed_build_list(fileclass_ed_data *);
void _fileclassed_build_recognition(fileclass_ed_data *);
Att_Node *_fileclassed_new_entry(fileclass_ed_data *, short, char *);
void _fileclassed_build_display(fileclass_ed_data *, Att_Node *);
void _fileclassed_start_edit(fileclass_ed_data *, Att_Node *);
void _fileclassed_end_edit(fileclass_ed_data *, BOOL);
void fileclassed_view_file(fileclass_ed_data *data);
void classed_send_rexx(fileclass_ed_data *data, char *command, short open);

void filetypeed_update_iconmenu(filetype_ed_data *data);
void filetypeed_add_iconmenu(filetype_ed_data *data);
void filetypeed_edit_iconmenu(filetype_ed_data *data, Att_Node *node);
BOOL filetypeed_check_iconmenu(filetype_ed_data *data, Att_Node *node, BOOL);
BOOL filetypeed_move_iconmenu(filetype_ed_data *data, BOOL up);

void filetypeed_start_drag(filetype_ed_data *data, struct IntuiMessage *msg);
BOOL filetypeed_end_drag(filetype_ed_data *data, BOOL ok);
void filetypeed_no_iconsel(filetype_ed_data *data);
BOOL filetypeed_get_button(filetype_ed_data *data, Cfg_Button *button, Point *pos);

void filetype_edit_name(config_filetypes_data *data, char *name);

void filetype_new_node(config_filetypes_data *data, Att_Node *node);
BOOL filetypeed_del_action(filetype_ed_data *data, short action);

BOOL filetypeed_sel_icon(filetype_ed_data *data, short item);

void filetype_able_buttons(config_filetypes_data *data, short state);
