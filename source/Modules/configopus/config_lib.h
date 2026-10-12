
#include <dopus/common.h>
#include <proto/dopus5.h>
#include <proto/configopus.h>
#include <proto/module.h>
#include <dopus/lib_macros.h>

#include "module_deps.h"

#define CATCOMP_NUMBERS
#include "string_data.h"

extern struct DOpusLocale *locale;

#ifndef __amigaos3__
	#pragma pack(2)
#endif
// Config sub-option handles
typedef struct _SubOptionHandle
{
	int num;				   // Option number
	ULONG name;				   // Option name ID
	const ObjectDef *objects;  // Object list
} SubOptionHandle;
#ifndef __amigaos3__
	#pragma pack()
#endif

void init_locale_data(struct DOpusLocale *);
void lsprintf __ARGS((char *, ...));

Att_List *build_sub_options(const SubOptionHandle *);

/*IPCData *SAVEDS Local_IPC_ProcStartup(
	ULONG *data,
	ULONG (*code)(IPCData *,APTR));*/
#define Local_IPC_ProcStartup(data, code) IPC_ProcStartup(data, code)

#define VALID_QUALIFIERS                                                                                           \
	(IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND | IEQUALIFIER_CONTROL | IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT | \
	 IEQUALIFIER_LALT | IEQUALIFIER_RALT)

#ifndef __amigaos3__
	#pragma pack(2)
#endif
typedef struct
{
	struct Window *window;

	DragInfo *drag;
	Att_Node *drag_node;
	short drag_x;
	short drag_y;

	long tick_count;
	long last_tick;
	TimerHandle *timer;

	unsigned long old_flags;
	unsigned long old_idcmp;

	short lock_count;
	short flags;
} CfgDragInfo;
#ifndef __amigaos3__
	#pragma pack()
#endif

BOOL config_drag_check(CfgDragInfo *drag);
void config_drag_move(CfgDragInfo *drag);
void config_drag_start(CfgDragInfo *, Att_List *, short, struct TagItem *, BOOL);
struct Window *config_drag_end(CfgDragInfo *, short);
BOOL config_drag_send_button(CfgDragInfo *, IPCData *, Cfg_Button *, Cfg_ButtonFunction *);
Cfg_Button *config_drag_get_button(Cfg_Button *button, Cfg_ButtonFunction *func);
void config_drag_start_window(CfgDragInfo *, struct Window *, struct Rectangle *, short, short);

char *function_label(Cfg_ButtonFunction *);
BOOL config_filereq(struct Window *window, ULONG title, char *path, char *defpath, short flags);
BOOL config_valid_path(char *path);

Cfg_Instruction *instruction_from_wbarg(struct WBArg *, APTR);
void parse_number(char **, unsigned short *);
void store_window_pos(struct Window *window, struct IBox *pos);

long LIBFUNC L_ShowPaletteBox(REG(a0, struct Window *),
                              REG(a1, DOpusScreenData *),
                              REG(a2, short *),
                              REG(a3, short *),
                              REG(d0, struct TextAttr *),
                              REG(d1, ColourSpec32 *),
                              REG(d2, short *));

BOOL LIBFUNC L_FunctionExportASCII(REG(a0, char *), REG(a1, Cfg_Button *), REG(a2, Cfg_Function *), REG(d0, ULONG));

short error_saving(short, struct Window *);

#include "enums.h"
#include "config_data.h"
#include "function_export.h"
#include "function_editor.h"
#include "select_colours.h"
#include "button_editor.h"

/* IPC_Launch */

#if defined(__MORPHOS__)
	#define IPCF_NATIVE (0x1)
#else
	#define IPCF_NATIVE (0x0)
#endif

#define IPC_NATIVE(entry) ((IPTR)&entry | (IPTR)(IPCF_NATIVE))
