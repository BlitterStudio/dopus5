#ifndef __amigaos3__
	#pragma pack(2)
#endif
typedef struct
{
	struct MinNode node;
	Cfg_ButtonFunction *func;
	IPCData *editor;
} ButtonEditorNode;

typedef struct
{
	struct _bank_node *bank;
	Cfg_Button *button;
	Cfg_Button *orig_button;
} ButtonReturn;

typedef struct
{
	struct Window *window;
	Cfg_Button *button;
	IPCData *owner_ipc;
	struct _ButtonEdData *data;
	FunctionStartup func_startup;
	ButtonReturn retdata;

	const ConfigWindow *win_text_def;
	const ConfigWindow *win_image_def;
	const ObjectDef *obj_def;

	PaletteBoxData palette_data;

	const ObjectDef *obj_name;
	const ObjectDef *obj_image;

	IPTR command_list;

	IPCData *main_owner;

	ULONG flags;
} ButtonEdStartup;

#define BUTTONEDF_NO_ADD (1 << 0)

typedef struct _ButtonEdData
{
	NewConfigWindow newwin;

	IPCData *ipc;
	struct Window *window;	// Window
	ObjectList *objlist;	// List of objects

	Cfg_Button *button;			   // Button being edited
	short which;				   // Function being editor (left/right/etc)
	GL_Object *display;			   // Button display area
	struct Rectangle button_rect;  // Button display rectangle

	struct DOpusLocale *locale;
	ButtonEdStartup *startup;

	Att_List *func_list;

	struct ListLock proc_list;

	/*struct Library		*DOpusBase;
	struct Library		*DOSBase;
	struct Library		*IntuitionBase;
	struct Library		*UtilityBase;
	struct Library		*CxBase;
	struct Library		*WorkbenchBase;
	struct Library		*GfxBase;
	struct Library		*LayersBase;*/
	ULONG pad1[8];

	struct AppWindow *appwindow;
	struct MsgPort *app_port;

	CfgDragInfo drag;

	GL_Object *lister;
	ULONG last_sec, last_mic;
	short change;
	struct MinList editors;

	short change_bit;

	struct IBox display_box;

	ImageRemap remap;
} ButtonEdData;
#ifndef __amigaos3__
	#pragma pack()
#endif

extern const ConfigWindow _button_editor_window;
extern const ObjectDef _button_editor_objects[], _button_editor_image_objects[], _button_editor_label_objects[];

enum {
	FUNCTION_LEFT,
	FUNCTION_RIGHT,
	FUNCTION_MIDDLE,
};

// prototypes
void ButtonEditor(void);
void _buttoned_show_button(ButtonEdData *);
void _button_editor_edit_function(ButtonEdData *);
BOOL _button_editor_receive_edit(ButtonEdData *, FunctionReturn *);
void button_editor_init(ButtonEdStartup *);
void palette_editor_init(PaletteBoxData *);
void function_editor_init(FunctionStartup *, IPTR);
void buttoned_copy_string(ButtonEdData *, char *, char **);
Cfg_ButtonFunction *buttoned_get_function(ButtonEdData *, short);
void buttoned_start_drag(ButtonEdData *, struct IntuiMessage *);
void buttoned_end_drag(ButtonEdData *, BOOL);
short buttoned_get_drop(ButtonEdData *, short, short);
void buttoned_get_button(ButtonEdData *, Cfg_Button *, Point *);
void buttoned_free_function(ButtonEdData *, Cfg_ButtonFunction *, BOOL);
Cfg_ButtonFunction *buttoned_copy_function(ButtonEdData *,
										   Cfg_Button *,
										   Cfg_ButtonFunction *,
										   Cfg_ButtonFunction *,
										   short);
Cfg_ButtonFunction *buttoned_find_function(ButtonEdData *, short);
BOOL buttoned_function_empty(Cfg_ButtonFunction *);
void buttoned_fix_functionlist(ButtonEdData *);
ButtonEditorNode *buttoned_find_editor(ButtonEdData *, Cfg_ButtonFunction *);
void buttoned_add_function(ButtonEdData *);
void buttoned_del_function(ButtonEdData *);
void buttoned_build_function_list(ButtonEdData *data);
BOOL buttoned_swap_functions(ButtonEdData *, short, short);
void buttoned_refresh_functionlist(ButtonEdData *data);
void _button_editor_change_label(ButtonEdData *data, UWORD id, BOOL);
void _buttoned_close_window(ButtonEdData *data);
void buttoned_delete_function(ButtonEdData *data, Cfg_ButtonFunction *func, BOOL);
BOOL buttoned_check_empty(Cfg_ButtonFunction *func);
