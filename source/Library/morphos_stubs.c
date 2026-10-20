
#include <ppcinline/macros.h>

int errno;	// just temporarily defined here

#define REG(r, t) REG_##r

#define LIBSTUB(func, ret)   \
	extern ULONG func(VOID); \
	ULONG libstub_##func(void) { return func(); }
#define LIBSTUB_1(func, ret, r1) \
	extern ULONG func(ULONG);    \
	ULONG libstub_##func(void) { return func(r1); }
#define LIBSTUB_2(func, ret, r1, r2) \
	extern ULONG func(ULONG, ULONG); \
	ULONG libstub_##func(void) { return func(r1, r2); }
#define LIBSTUB_3(func, ret, r1, r2, r3)    \
	extern ULONG func(ULONG, ULONG, ULONG); \
	ULONG libstub_##func(void) { return func(r1, r2, r3); }
#define LIBSTUB_4(func, ret, r1, r2, r3, r4)       \
	extern ULONG func(ULONG, ULONG, ULONG, ULONG); \
	ULONG libstub_##func(void) { return func(r1, r2, r3, r4); }
#define LIBSTUB_5(func, ret, r1, r2, r3, r4, r5)          \
	extern ULONG func(ULONG, ULONG, ULONG, ULONG, ULONG); \
	ULONG libstub_##func(void) { return func(r1, r2, r3, r4, r5); }
#define LIBSTUB_6(func, ret, r1, r2, r3, r4, r5, r6)             \
	extern ULONG func(ULONG, ULONG, ULONG, ULONG, ULONG, ULONG); \
	ULONG libstub_##func(void) { return func(r1, r2, r3, r4, r5, r6); }
#define LIBSTUB_7(func, ret, r1, r2, r3, r4, r5, r6, r7)                \
	extern ULONG func(ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG); \
	ULONG libstub_##func(void) { return func(r1, r2, r3, r4, r5, r6, r7); }
#define LIBSTUB_8(func, ret, r1, r2, r3, r4, r5, r6, r7, r8)                   \
	extern ULONG func(ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG); \
	ULONG libstub_##func(void) { return func(r1, r2, r3, r4, r5, r6, r7, r8); }
#define LIBSTUB_9(func, ret, r1, r2, r3, r4, r5, r6, r7, r8, r9)                      \
	extern ULONG func(ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG); \
	ULONG libstub_##func(void) { return func(r1, r2, r3, r4, r5, r6, r7, r8, r9); }
#define LIBSTUB_12(func, ret, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12)                           \
	extern ULONG func(ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG); \
	ULONG libstub_##func(void) { return func(r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12); }

// dopus5 stubs:

LIBSTUB_4(L_DivideU,
		  ULONG,
		  REG(d0, ULONG num),
		  REG(d1, ULONG div),
		  REG(a0, ULONG *rem),
		  REG(a1, struct Library *lib))

// functions.a
LIBSTUB_3(L_BtoCStr, void, REG(a0, BSTR bstr), REG(a1, char *cstr), REG(d0, int len))

// timer.c
LIBSTUB_2(L_AllocTimer, struct TimerHandle *, REG(d0, ULONG unit), REG(a0, struct MsgPort *port))

LIBSTUB_1(L_FreeTimer, void, REG(a0, struct TimerHandle *handle))
LIBSTUB_3(L_StartTimer, void, REG(a0, TimerHandle *handle), REG(d0, ULONG seconds), REG(d1, ULONG micros))

LIBSTUB_1(L_CheckTimer, BOOL, REG(a0, TimerHandle *handle))
LIBSTUB_1(L_StopTimer, void, REG(a0, TimerHandle *handle))

// pathlist.c
LIBSTUB_1(L_GetDosPathList, BPTR, REG(a0, BPTR copy_list))

LIBSTUB_1(L_FreeDosPathList, void, REG(a0, BPTR list))

LIBSTUB_1(L_CopyLocalEnv, void, REG(a0, struct Library *DOSBase))

// strings.c
LIBSTUB_4(L_StrCombine, BOOL, REG(a0, char *buf), REG(a1, char *one), REG(a2, char *two), REG(d0, int lim))
LIBSTUB_3(L_StrConcat, BOOL, REG(a0, char *buf), REG(a1, char *cat), REG(d0, int lim))

LIBSTUB_2(L_Atoh, ULONG, REG(a0, unsigned char *buf), REG(d0, short len))

// gui.c
LIBSTUB_4(L_DrawBox,
		  void,
		  REG(a0, struct RastPort *rp),
		  REG(a1, struct Rectangle *rect),
		  REG(a2, struct DrawInfo *info),
		  REG(d0, BOOL recessed))

LIBSTUB_3(L_DrawFieldBox,
		  void,
		  REG(a0, struct RastPort *rp),
		  REG(a1, struct Rectangle *rect),
		  REG(a2, struct DrawInfo *info))

LIBSTUB_3(L_WriteIcon,
		  BOOL,
		  REG(a0, char *name),
		  REG(a1, struct DiskObject *diskobj),
		  REG(a6, struct MyLibrary *libbase))

LIBSTUB_2(L_DeleteIcon, BOOL, REG(a0, char *name), REG(a6, struct MyLibrary *libbase))

LIBSTUB_1(L_ScreenInfo, ULONG, REG(a0, struct Screen *screen))

// misc.c
LIBSTUB_5(L_BuildKeyString,
		  void,
		  REG(d0, unsigned short code),
		  REG(d1, unsigned short qual),
		  REG(d2, unsigned short qual_mask),
		  REG(d3, unsigned short qual_same),
		  REG(a0, char *buffer))

LIBSTUB_1(L_QualValid, UWORD, REG(d0, unsigned short qual))
LIBSTUB_3(L_ConvertRawKey, BOOL, REG(d0, UWORD code), REG(d1, UWORD qual), REG(a0, char *key))
LIBSTUB_1(L_SetBusyPointer, void, REG(a0, struct Window *wind))
LIBSTUB_2(L_ActivateStrGad, void, REG(a0, struct Gadget *gad), REG(a1, struct Window *win))
LIBSTUB_3(L_Itoa, void, REG(d0, long num), REG(a0, char *str), REG(d1, char sep))

LIBSTUB_3(L_ItoaU, void, REG(d0, unsigned long num), REG(a0, char *str), REG(d1, char sep))

LIBSTUB_4(L_BytesToString,
		  void,
		  REG(d0, unsigned long bytes),
		  REG(a0, char *string),
		  REG(d1, short places),
		  REG(d2, char sep))

LIBSTUB_5(L_DivideToString,
		  void,
		  REG(a0, char *string),
		  REG(d0, unsigned long bytes),
		  REG(d1, unsigned long div),
		  REG(d2, short places),
		  REG(d3, char sep))
LIBSTUB_2(L_Ito26, void, REG(d0, unsigned long num), REG(a0, char *str))
LIBSTUB_1(L_SerialValid, BOOL, REG(a0, serial_data *data))
LIBSTUB_3(L_WriteFileIcon, void, REG(a0, char *source), REG(a1, char *dest), REG(a6, struct MyLibrary *libbase))

LIBSTUB_4(L_GetWBArgPath,
		  BOOL,
		  REG(a0, struct WBArg *arg),
		  REG(a1, char *buffer),
		  REG(d0, long size),
		  REG(a6, struct MyLibrary *libbase))
LIBSTUB_2(L_FindPubScreen, struct PubScreenNode *, REG(a0, struct Screen *screen), REG(d0, BOOL lock))

// simplerequest.c
LIBSTUB_3(L_DoSimpleRequest,
		  short,
		  REG(a0, struct Window *parent),
		  REG(a1, struct DOpusSimpleRequest *simple),
		  REG(a6, struct Library *libbase))

LIBSTUB_9(L_SimpleRequest,
		  short,
		  REG(a0, struct Window *parent),
		  REG(a1, char *title),
		  REG(a2, char *buttons),
		  REG(a3, char *message),
		  REG(a4, char *buffer),
		  REG(a5, APTR params),
		  REG(d0, long buffersize),
		  REG(d1, ULONG flags),
		  REG(a6, struct Library *libbase))

// drag_routines.c
LIBSTUB_6(L_GetDragInfo,
		  DragInfo *,
		  REG(a0, struct Window *window),
		  REG(a1, struct RastPort *rast),
		  REG(d0, long width),
		  REG(d1, long height),
		  REG(d2, long flags),
		  REG(a6, struct MyLibrary *libbase))

LIBSTUB_1(L_FreeDragInfo, void, REG(a0, DragInfo *drag))
LIBSTUB_1(L_HideDragImage, void, REG(a0, DragInfo *drag))
LIBSTUB_3(L_GetDragImage, void, REG(a0, DragInfo *drag), REG(d0, ULONG x), REG(d1, ULONG y))

LIBSTUB_1(L_GetDragMask, void, REG(a0, DragInfo *drag))

LIBSTUB_3(L_ShowDragImage, void, REG(a0, DragInfo *drag), REG(d0, ULONG x), REG(d1, ULONG y))

LIBSTUB_1(L_AddDragImage, void, REG(a0, DragInfo *drag))
LIBSTUB_1(L_RemDragImage, void, REG(a0, DragInfo *drag))
LIBSTUB_3(L_StampDragImage, void, REG(a0, DragInfo *drag), REG(d0, ULONG x), REG(d1, ULONG y))
LIBSTUB_1(L_CheckDragDeadlock, BOOL, REG(a0, DragInfo *drag))
LIBSTUB_6(L_BuildTransDragMask,
		  BOOL,
		  REG(a0, UWORD *mask),
		  REG(a1, UWORD *image),
		  REG(d0, short width),
		  REG(d1, short height),
		  REG(d2, short depth),
		  REG(d3, long flags))
LIBSTUB_3(L_DrawDragList, void, REG(a0, struct RastPort *rp), REG(a1, struct ViewPort *vp), REG(d0, long flags))
LIBSTUB_1(L_RemoveDragImage, void, REG(a0, DragInfo *drag))
LIBSTUB_2(L_DragCustomOk, BOOL, REG(a0, struct BitMap *bm), REG(a6, struct MyLibrary *libbase))

// layout_routines.c
LIBSTUB_2(L_OpenConfigWindow, struct Window *, REG(a0, NewConfigWindow *newwindow), REG(a6, struct MyLibrary *libbase))
LIBSTUB_2(L_CloseConfigWindow, void, REG(a0, struct Window *window), REG(a6, struct MyLibrary *libbase))
LIBSTUB_1(L_StripIntuiMessagesDopus, void, REG(a0, struct Window *window))
LIBSTUB_2(L_StripWindowMessages, void, REG(a0, struct MsgPort *port), REG(a1, struct IntuiMessage *except))
LIBSTUB_1(L_CloseWindowSafely, void, REG(a0, struct Window *window))
LIBSTUB_8(L_CalcObjectDims,
		  int,
		  REG(a0, void *parent),
		  REG(a1, struct TextFont *use_font),
		  REG(a2, struct IBox *pos),
		  REG(a3, struct IBox *dest_pos),
		  REG(a4, GL_Object *last_ob),
		  REG(d0, ULONG flags),
		  REG(d1, GL_Object *this_ob),
		  REG(d2, GL_Object *parent_ob))

LIBSTUB_5(L_CalcWindowDims,
		  int,
		  REG(a0, struct Screen *screen),
		  REG(a1, ConfigWindow *win_pos),
		  REG(a2, struct IBox *dest_pos),
		  REG(a3, struct TextFont *font),
		  REG(d0, ULONG flags))

LIBSTUB_3(L_AddObjectList,
		  ObjectList *,
		  REG(a0, struct Window *window),
		  REG(a1, ObjectDef *objects),
		  REG(a6, struct MyLibrary *libbase))
LIBSTUB_2(L_FreeObject, void, REG(a0, ObjectList *objlist), REG(a1, GL_Object *object))
LIBSTUB_1(L_FreeObjectList, void, REG(a0, ObjectList *objlist))
LIBSTUB_2(L_GetObject, GL_Object *, REG(a0, ObjectList *list), REG(d0, int id))
LIBSTUB_3(L_StoreGadgetValue,
		  void,
		  REG(a0, ObjectList *list),
		  REG(a1, struct IntuiMessage *msg),
		  REG(a6, struct MyLibrary *libbase))
LIBSTUB_4(L_UpdateGadgetValue,
		  void,
		  REG(a0, ObjectList *list),
		  REG(a1, struct IntuiMessage *msg),
		  REG(d0, UWORD id),
		  REG(a6, struct MyLibrary *libbase))
LIBSTUB_3(L_SetGadgetValue, void, REG(a0, ObjectList *list), REG(d0, UWORD id), REG(d1, IPTR value))
LIBSTUB_3(L_GetGadgetValue, IPTR, REG(a0, ObjectList *list), REG(a1, UWORD id), REG(a6, struct MyLibrary *libbase))
LIBSTUB_3(L_CheckObjectArea, BOOL, REG(a0, GL_Object *object), REG(d0, int x), REG(d1, int y))
LIBSTUB_5(L_DisplayObject,
		  void,
		  REG(a0, struct Window *window),
		  REG(a1, GL_Object *object),
		  REG(d0, int fg),
		  REG(d1, int bg),
		  REG(a2, char *txt))
LIBSTUB_2(L_AddWindowMenus, void, REG(a0, struct Window *window), REG(a1, MenuData *menudata))
LIBSTUB_1(L_FreeWindowMenus, void, REG(a0, struct Window *window))
LIBSTUB_1(L_SetWindowBusy, void, REG(a0, struct Window *window))
LIBSTUB_1(L_ClearWindowBusy, void, REG(a0, struct Window *window))
LIBSTUB_2(L_GetString, STRPTR, REG(a0, struct DOpusLocale *li), REG(d0, LONG stringNum))
LIBSTUB_3(L_FindKeyEquivalent,
		  struct Gadget *,
		  REG(a0, ObjectList *list),
		  REG(a1, struct IntuiMessage *msg),
		  REG(d0, int process))
LIBSTUB_4(L_ShowProgressBar,
		  void,
		  REG(a0, struct Window *window),
		  REG(a1, GL_Object *object),
		  REG(d0, ULONG total),
		  REG(d1, ULONG count))
LIBSTUB_3(L_SetObjectKind, void, REG(a0, ObjectList *list), REG(d0, ULONG id), REG(d1, UWORD kind))
LIBSTUB_3(L_DisableObject, void, REG(a0, ObjectList *list), REG(d0, ULONG id), REG(d1, BOOL state))
LIBSTUB_4(L_BoundsCheckGadget, int, REG(a0, ObjectList *list), REG(d0, ULONG id), REG(d1, int min), REG(d2, int max))
LIBSTUB_2(L_RefreshObjectList, void, REG(a0, struct Window *window), REG(a1, ObjectList *ref_list))
LIBSTUB_2(L_GetWindowMsg, struct IntuiMessage *, REG(a0, struct MsgPort *port), REG(a6, struct MyLibrary *lib))
LIBSTUB_1(L_ReplyWindowMsg, void, REG(a0, struct IntuiMessage *msg))
LIBSTUB_3(L_SetGadgetChoices, void, REG(a0, ObjectList *list), REG(d0, ULONG id), REG(a1, APTR choices))
LIBSTUB_4(L_SetWindowID,
		  void,
		  REG(a0, struct Window *window),
		  REG(a1, WindowID *id),
		  REG(d0, ULONG window_id),
		  REG(a2, struct MsgPort *port))
LIBSTUB_1(L_GetWindowID, ULONG, REG(a0, struct Window *window))
LIBSTUB_1(L_GetWindowAppPort, struct MsgPort *, REG(a0, struct Window *window))
LIBSTUB_3(L_GetObjectRect, BOOL, REG(a0, ObjectList *list), REG(d0, ULONG id), REG(a1, struct Rectangle *rect))
LIBSTUB_2(L_StartRefreshConfigWindow, void, REG(a0, struct Window *window), REG(d0, long final_state))
LIBSTUB_1(L_EndRefreshConfigWindow, void, REG(a0, struct Window *window))

// menu_routines.c
LIBSTUB_2(L_BuildMenuStrip, struct Menu *, REG(a0, MenuData *menudata), REG(a1, struct DOpusLocale *locale))
LIBSTUB_2(L_FindMenuItem, struct MenuItem *, REG(a0, struct Menu *menu), REG(d0, UWORD id))

// list_management.c
LIBSTUB_1(L_Att_NewList, Att_List *, REG(d0, ULONG flags))
LIBSTUB_4(L_Att_NewNode,
		  Att_Node *,
		  REG(a0, Att_List *list),
		  REG(a1, char *name),
		  REG(d0, IPTR data),
		  REG(d1, ULONG flags))
LIBSTUB_1(L_Att_RemNode, void, REG(a0, Att_Node *node))
LIBSTUB_3(L_Att_PosNode, void, REG(a0, Att_List *list), REG(a1, Att_Node *node), REG(a2, Att_Node *before))
LIBSTUB_2(L_Att_RemList, void, REG(a0, Att_List *list), REG(d0, long flags))
LIBSTUB_2(L_Att_FindNode, Att_Node *, REG(a0, Att_List *list), REG(d0, long number))
LIBSTUB_2(L_Att_NodeNumber, long, REG(a0, Att_List *list), REG(a1, char *name))
LIBSTUB_2(L_Att_FindNodeData, Att_Node *, REG(a0, Att_List *list), REG(d0, IPTR data))
LIBSTUB_2(L_Att_NodeDataNumber, long, REG(a0, Att_List *list), REG(d0, IPTR data))
LIBSTUB_2(L_Att_NodeName, char *, REG(a0, Att_List *list), REG(d0, long number))
LIBSTUB_1(L_Att_NodeCount, long, REG(a0, Att_List *list))
LIBSTUB_2(L_Att_ChangeNodeName, void, REG(a0, Att_Node *node), REG(a1, char *name))
LIBSTUB_2(L_Att_FindNodeNumber, long, REG(a0, Att_List *list), REG(a1, Att_Node *node))
LIBSTUB_2(L_AddSorted, void, REG(a0, struct List *list), REG(a1, struct Node *node))
LIBSTUB_2(L_FindNameI, struct Node *, REG(a0, struct List *list), REG(a1, char *name))
LIBSTUB_2(L_LockAttList, void, REG(a0, Att_List *list), REG(d0, short exclusive))
LIBSTUB_1(L_UnlockAttList, void, REG(a0, Att_List *list))
LIBSTUB_3(L_SwapListNodes, void, REG(a0, struct List *list), REG(a1, struct Node *swap1), REG(a2, struct Node *swap2))
LIBSTUB_1(L_IsListLockEmpty, BOOL, REG(a0, struct ListLock *list))

// memory.c
LIBSTUB_3(L_NewMemHandle, void *, REG(d0, ULONG puddle_size), REG(d1, ULONG thresh_size), REG(d2, ULONG type))
LIBSTUB_1(L_FreeMemHandle, void, REG(a0, MemHandle *handle))
LIBSTUB_1(L_ClearMemHandle, void, REG(a0, MemHandle *handle))
LIBSTUB_2(L_AllocMemH, void *, REG(a0, MemHandle *handle), REG(d0, ULONG size))
LIBSTUB_1(L_FreeMemH, void, REG(a0, void *memory))

// config routines
LIBSTUB_1(L_NewLister, Cfg_Lister *, REG(a0, char *path))
LIBSTUB_2(L_NewButtonBank, Cfg_ButtonBank *, REG(d0, BOOL init), REG(d1, short type))
LIBSTUB_1(L_NewButton, Cfg_Button *, REG(a0, APTR memory))
LIBSTUB_2(L_NewFunction, Cfg_Function *, REG(a0, APTR memory), REG(d0, UWORD type))
LIBSTUB_2(L_NewButtonFunction, Cfg_ButtonFunction *, REG(a0, APTR memory), REG(d0, UWORD type))
LIBSTUB_3(L_NewInstruction, Cfg_Instruction *, REG(a0, APTR memory), REG(d0, short type), REG(a1, char *string))
LIBSTUB_1(L_NewFiletype, Cfg_Filetype *, REG(a0, APTR memory))
LIBSTUB_2(L_ReadSettings, short, REG(a0, CFG_SETS *settings), REG(a1, char *name))
LIBSTUB_2(L_ReadListerDef, Cfg_Lister *, REG(a0, struct _IFFHandle *iff), REG(d0, ULONG id))
LIBSTUB_1(L_OpenButtonBank, Cfg_ButtonBank *, REG(a0, char *name))
LIBSTUB_2(L_ReadFiletypes, Cfg_FiletypeList *, REG(a0, char *name), REG(a1, APTR memory))

LIBSTUB_2(L_ReadButton, Cfg_Button *, REG(a0, struct _IFFHandle *iff), REG(a1, APTR memory))

LIBSTUB_4(L_ReadFunction,
		  Cfg_Function *,
		  REG(a0, struct _IFFHandle *iff),
		  REG(a1, APTR memory),
		  REG(a2, struct List *func_list),
		  REG(a3, Cfg_Function *function))

LIBSTUB_1(L_ConvertStartMenu, void, REG(a0, Cfg_ButtonBank *bank))

LIBSTUB_1(L_DefaultSettings, void, REG(a0, CFG_SETS *settings))

LIBSTUB_1(L_DefaultEnvironment, void, REG(a0, CFG_ENVR *env))
LIBSTUB(L_DefaultButtonBank, Cfg_ButtonBank *)

LIBSTUB_1(L_UpdateEnvironment, void, REG(a0, CFG_ENVR *env))

LIBSTUB_2(L_SaveListerDef, long, REG(a0, struct _IFFHandle *iff), REG(a1, Cfg_Lister *lister))

LIBSTUB_2(L_SaveButton, short, REG(a0, struct _IFFHandle *iff), REG(a1, Cfg_Button *button))

LIBSTUB_2(L_SaveFunction, BOOL, REG(a0, struct _IFFHandle *iff), REG(a1, Cfg_Function *function))

LIBSTUB_1(L_CloseButtonBank, void, REG(a0, Cfg_ButtonBank *bank))

LIBSTUB_1(L_FreeListerDef, void, REG(a0, Cfg_Lister *lister))

LIBSTUB_1(L_FreeButtonList, void, REG(a0, struct List *list))

LIBSTUB_1(L_FreeButtonImages, void, REG(a0, struct List *list))

LIBSTUB_1(L_FreeButton, void, REG(a0, Cfg_Button *button))

LIBSTUB_1(L_FreeFunction, void, REG(a0, Cfg_Function *function))

LIBSTUB_1(L_FreeButtonFunction, void, REG(a0, Cfg_ButtonFunction *function))

LIBSTUB_1(L_FreeInstruction, void, REG(a0, Cfg_Instruction *ins))

LIBSTUB_1(L_FreeInstructionList, void, REG(a0, Cfg_Function *func))

LIBSTUB_1(L_FreeFiletypeList, void, REG(a0, Cfg_FiletypeList *list))

LIBSTUB_1(L_FreeFiletype, void, REG(a0, Cfg_Filetype *type))

LIBSTUB_1(L_CopyButtonBank, Cfg_ButtonBank *, REG(a0, Cfg_ButtonBank *orig))

LIBSTUB_3(L_CopyButton, Cfg_Button *, REG(a0, Cfg_Button *orig), REG(a1, APTR memory), REG(d0, short type))

LIBSTUB_2(L_CopyFiletype, Cfg_Filetype *, REG(a0, Cfg_Filetype *orig), REG(a1, APTR memory))

LIBSTUB_3(L_CopyFunction,
		  Cfg_Function *,
		  REG(a0, Cfg_Function *orig),
		  REG(a1, APTR memory),
		  REG(a2, Cfg_Function *newfunc))

LIBSTUB_3(L_CopyButtonFunction,
		  Cfg_ButtonFunction *,
		  REG(a0, Cfg_ButtonFunction *func),
		  REG(a1, APTR memory),
		  REG(a2, Cfg_ButtonFunction *newfunc))

LIBSTUB_2(L_FindFunctionType, Cfg_Function *, REG(a0, struct List *list), REG(d0, UWORD type))

// popup menus
LIBSTUB_5(L_DoPopUpMenu,
		  UWORD,
		  REG(a0, struct Window *window),
		  REG(a1, PopUpMenu *menu),
		  REG(a2, PopUpItem **sel_item),
		  REG(d0, UWORD code),
		  REG(a6, struct MyLibrary *libbase))

LIBSTUB_2(L_GetPopUpItem, PopUpItem *, REG(a0, PopUpMenu *menu), REG(d0, UWORD id))

LIBSTUB_2(L_SetPopUpDelay, void, REG(d0, short delay), REG(a6, struct MyLibrary *libbase))

// ipc
LIBSTUB_8(L_IPC_Launch,
		  int,
		  REG(a0, struct ListLock *list),
		  REG(a1, IPCData **storage),
		  REG(a2, char *name),
		  REG(d0, IPTR entry),
		  REG(d1, ULONG stack),
		  REG(d2, IPTR data),
		  REG(a3, struct Library *dos_base),
		  REG(a6, struct MyLibrary *libbase))
LIBSTUB_3(L_IPC_Startup, int, REG(a0, IPCData *ipc), REG(a1, APTR data), REG(a2, struct MsgPort *reply))

LIBSTUB_6(L_IPC_Command,
		  IPTR,
		  REG(a0, IPCData *ipc),
		  REG(d0, ULONG command),
		  REG(d1, IPTR flags),
		  REG(a1, APTR data),
		  REG(a2, APTR data_free),
		  REG(a3, struct MsgPort *reply))

LIBSTUB_1(L_IPC_Reply, void, REG(a0, IPCMessage *msg))

LIBSTUB_1(L_IPC_Free, void, REG(a0, IPCData *ipc))

LIBSTUB_4(L_IPC_FindProc,
		  IPCData *,
		  REG(a0, struct ListLock *list),
		  REG(a1, char *name),
		  REG(d0, BOOL activate),
		  REG(d1, IPTR data))

LIBSTUB_2(L_IPC_ProcStartup,
		  IPCData *,
		  REG(a0, IPTR *data),
		  REG(a1, ULONG (*ASM code)(REG(a0, IPCData *), REG(a1, APTR))))

LIBSTUB_3(L_IPC_Quit, void, REG(a0, IPCData *ipc), REG(d0, ULONG quit_flags), REG(d1, BOOL wait))

LIBSTUB_2(L_IPC_Hello, void, REG(a0, IPCData *ipc), REG(a1, IPCData *owner))

LIBSTUB_3(L_IPC_Goodbye, void, REG(a0, IPCData *ipc), REG(a1, IPCData *owner), REG(d0, ULONG goodbye_flags))

LIBSTUB_1(L_IPC_GetGoodbye, IPTR, REG(a0, IPCMessage *msg))

LIBSTUB_4(L_IPC_ListQuit,
		  ULONG,
		  REG(a0, struct ListLock *list),
		  REG(a1, IPCData *owner),
		  REG(d0, ULONG quit_flags),
		  REG(d1, BOOL wait))

LIBSTUB_1(L_IPC_Flush, void, REG(a0, IPCData *ipc))

LIBSTUB_5(L_IPC_ListCommand,
		  void,
		  REG(a0, struct ListLock *list),
		  REG(d0, ULONG command),
		  REG(d1, IPTR flags),
		  REG(d2, IPTR data),
		  REG(d3, BOOL wait))

LIBSTUB_3(L_IPC_QuitName, void, REG(a0, struct ListLock *list), REG(a1, char *name), REG(d0, ULONG quit_flags))

// Image routines
LIBSTUB_2(L_OpenImage, APTR, REG(a0, char *name), REG(a1, OpenImageInfo *info))

LIBSTUB_1(L_CloseImage, void, REG(a0, APTR image))

LIBSTUB_1(L_CopyImage, APTR, REG(a0, APTR image))

LIBSTUB(L_FlushImages, void)
LIBSTUB_5(L_RenderImage,
		  short,
		  REG(a0, struct RastPort *rp),
		  REG(a1, APTR image),
		  REG(d0, unsigned short left),
		  REG(d1, unsigned short top),
		  REG(a2, struct TagItem *tags))
LIBSTUB_2(L_GetImageAttrs, void, REG(a0, APTR imptr), REG(a1, struct TagItem *tags))

LIBSTUB_3(L_RemapImage, BOOL, REG(a0, Image_Data *image), REG(a1, struct Screen *screen), REG(a2, ImageRemap *remap))

LIBSTUB_2(L_FreeRemapImage, void, REG(a0, Image_Data *image), REG(a1, ImageRemap *remap))

// status window
LIBSTUB_6(L_OpenStatusWindow,
		  struct Window *,
		  REG(a0, char *title),
		  REG(a1, char *text),
		  REG(a2, struct Screen *screen),
		  REG(d1, LONG graph),
		  REG(d0, ULONG flags),
		  REG(a6, struct MyLibrary *libbase))

LIBSTUB_2(L_SetStatusText, void, REG(a0, struct Window *window), REG(a1, char *text))

LIBSTUB_4(L_UpdateStatusGraph,
		  void,
		  REG(a0, struct Window *window),
		  REG(a1, char *text),
		  REG(d0, ULONG total),
		  REG(d1, ULONG count))

// read_ilbm
LIBSTUB_2(L_ReadILBM, ILBMHandle *, REG(a0, char *name), REG(d0, ULONG flags))

LIBSTUB_1(L_FreeILBM, void, REG(a0, ILBMHandle *ilbm))

LIBSTUB_7(L_DecodeILBM,
		  void,
		  REG(a0, char *source),
		  REG(d0, unsigned short width),
		  REG(d1, unsigned short height),
		  REG(d2, unsigned short depth),
		  REG(a1, struct BitMap *dest),
		  REG(d3, unsigned long flags),
		  REG(d4, char comp))
LIBSTUB_1(L_DecodeRLE, void, REG(a0, RLEinfo *rle))

LIBSTUB_6(L_FakeILBM,
		  ILBMHandle *,
		  REG(a0, UWORD *imagedata),
		  REG(a1, ULONG *palette),
		  REG(d0, short width),
		  REG(d1, short height),
		  REG(d2, short depth),
		  REG(d3, ULONG flags))

// anim
LIBSTUB_4(L_AnimDecodeRIFFXor,
		  void,
		  REG(a0, unsigned char *delta),
		  REG(a1, char *plane),
		  REG(d0, unsigned short rowbytes),
		  REG(d1, unsigned short sourcebytes))

LIBSTUB_4(L_AnimDecodeRIFFSet,
		  void,
		  REG(a0, unsigned char *delta),
		  REG(a1, char *plane),
		  REG(d0, unsigned short rowbytes),
		  REG(d1, unsigned short sourcebytes))

// palette
LIBSTUB_2(L_LoadPalette32, void, REG(a0, struct ViewPort *vp), REG(a1, ULONG *palette))

LIBSTUB_4(L_GetPalette32,
		  void,
		  REG(a0, struct ViewPort *vp),
		  REG(a1, ULONG *palette),
		  REG(d0, unsigned short count),
		  REG(d1, short first))

// buffered_io
LIBSTUB_3(L_OpenBuf, APTR, REG(a0, char *name), REG(d0, long mode), REG(d1, long buffer_size))

LIBSTUB_1(L_CloseBuf, long, REG(a0, APTR file))

LIBSTUB_3(L_ReadBuf, long, REG(a0, APTR file), REG(a1, char *data), REG(d0, long size))

LIBSTUB_3(L_WriteBuf, long, REG(a0, APTR file), REG(a1, char *data), REG(d0, long size))

LIBSTUB_1(L_FlushBuf, long, REG(a0, APTR file))

LIBSTUB_3(L_SeekBuf, long, REG(a0, APTR file), REG(d0, long offset), REG(d1, long mode))

LIBSTUB_2(L_ExamineBuf, long, REG(a0, APTR file), REG(a1, struct FileInfoBlock *fib))

LIBSTUB_1(L_FHFromBuf, BPTR, REG(a0, APTR file))

LIBSTUB_3(L_ReadBufLine, long, REG(a0, APTR file), REG(a1, char *data), REG(d0, long size))

// diskio
LIBSTUB_2(L_OpenDisk, DiskHandle *, REG(a0, char *disk), REG(a1, struct MsgPort *port))

LIBSTUB_1(L_CloseDisk, void, REG(a0, DiskHandle *handle))

// boopsi
LIBSTUB_4(L_AddScrollBars,
		  struct Gadget *,
		  REG(a0, struct Window *window),
		  REG(a1, struct List *list),
		  REG(a2, struct DrawInfo *draw_info),
		  REG(d0, short noidcmpupdate))

LIBSTUB_2(L_FindBOOPSIGadget, struct Gadget *, REG(a0, struct List *list), REG(d0, UWORD id))

LIBSTUB_1(L_BOOPSIFree, void, REG(a0, struct List *list))

LIBSTUB_7(L_CreateTitleGadget,
		  struct Gadget *,
		  REG(a0, struct Screen *screen),
		  REG(a1, struct List *list),
		  REG(d0, BOOL cover_zoom),
		  REG(d1, short offset),
		  REG(d2, short type),
		  REG(d3, unsigned short id),
		  REG(a6, struct MyLibrary *libbase))

LIBSTUB_2(L_FindGadgetType, struct Gadget *, REG(a0, struct Gadget *gadget), REG(d0, UWORD type))

LIBSTUB_1(L_FixTitleGadgets, void, REG(a0, struct Window *window))

// iff
LIBSTUB_3(L_IFFOpen, struct _IFFHandle *, REG(a0, char *name), REG(d0, unsigned short mode), REG(d1, ULONG form))

LIBSTUB_1(L_IFFClose, void, REG(a0, struct _IFFHandle *handle))
LIBSTUB_2(L_IFFPushChunk, long, REG(a0, struct _IFFHandle *handle), REG(d0, ULONG id))

LIBSTUB_3(L_IFFWriteChunkBytes, long, REG(a0, struct _IFFHandle *handle), REG(a1, char *data), REG(d0, long size))

LIBSTUB_1(L_IFFPopChunk, long, REG(a0, struct _IFFHandle *handle))

LIBSTUB_4(L_IFFWriteChunk,
		  long,
		  REG(a0, struct _IFFHandle *handle),
		  REG(a1, char *data),
		  REG(d0, ULONG chunk),
		  REG(d1, ULONG size))

LIBSTUB_2(L_IFFNextChunk, unsigned long, REG(a0, struct _IFFHandle *handle), REG(d0, unsigned long form))

LIBSTUB_1(L_IFFChunkSize, long, REG(a0, struct _IFFHandle *handle))

LIBSTUB_3(L_IFFReadChunkBytes, long, REG(a0, struct _IFFHandle *handle), REG(a1, APTR buffer), REG(d0, long size))

LIBSTUB_1(L_IFFFileHandle, APTR, REG(a0, struct _IFFHandle *handle))

LIBSTUB_1(L_IFFChunkRemain, long, REG(a0, struct _IFFHandle *handle))

LIBSTUB_1(L_IFFChunkID, unsigned long, REG(a0, struct _IFFHandle *handle))

LIBSTUB_1(L_IFFGetFORM, unsigned long, REG(a0, struct _IFFHandle *handle))

LIBSTUB_3(L_GetSemaphore, long, REG(a0, struct SignalSemaphore *sem), REG(d0, long exclusive), REG(a1, char *data))

LIBSTUB_1(L_FreeSemaphore, void, REG(a0, struct SignalSemaphore *sem))

LIBSTUB_1(L_ShowSemaphore, void, REG(a0, struct SignalSemaphore *sem))

LIBSTUB_2(L_InitListLock, void, REG(a0, struct ListLock *ll), REG(a1, char *name))

// layout_utils.c
LIBSTUB_2(L_InitWindowDims, void, REG(a0, struct Window *window), REG(a1, WindowDimensions *dims))

LIBSTUB_2(L_StoreWindowDims, void, REG(a0, struct Window *window), REG(a1, WindowDimensions *dims))

LIBSTUB_2(L_CheckWindowDims, BOOL, REG(a0, struct Window *window), REG(a1, WindowDimensions *dims))

LIBSTUB_1(L_LayoutResize, void, REG(a0, struct Window *window))

// devices
LIBSTUB_3(L_DeviceFromLock,
		  struct DosList *,
		  REG(a0, BPTR lock),
		  REG(a1, char *name),
		  REG(a6, struct MyLibrary *libbase))

LIBSTUB_3(L_DeviceFromHandler,
		  struct DosList *,
		  REG(a0, struct MsgPort *port),
		  REG(a1, char *name),
		  REG(a6, struct MyLibrary *libbase))

LIBSTUB_4(L_DevNameFromLockDopus,
		  BOOL,
		  REG(d1, BPTR lock),
		  REG(d2, char *buffer),
		  REG(d3, long len),
		  REG(a6, struct MyLibrary *libbase))

LIBSTUB_3(L_GetDeviceUnit, BOOL, REG(a0, BPTR dol_Startup), REG(a1, char *device), REG(a2, short *unit))

// icon cache
LIBSTUB_2(L_GetCachedDefDiskObject, struct DiskObject *, REG(d0, long type), REG(a6, struct MyLibrary *libbase))

LIBSTUB_2(L_FreeCachedDiskObject, void, REG(a0, struct DiskObject *icon), REG(a6, struct MyLibrary *libbase))
LIBSTUB_3(L_GetCachedDiskObject,
		  struct DiskObject *,
		  REG(a0, char *name),
		  REG(d0, ULONG flags),
		  REG(a6, struct MyLibrary *libbase))

LIBSTUB_3(L_GetCachedDiskObjectNew,
		  struct DiskObject *,
		  REG(a0, char *name),
		  REG(d0, ULONG flags),
		  REG(a6, struct MyLibrary *libbase))

LIBSTUB_2(L_IconCheckSum, unsigned long, REG(a0, struct DiskObject *icon), REG(d0, short which))

LIBSTUB_1(L_GetIconType, short, REG(a0, struct DiskObject *icon))

// progress
LIBSTUB_2(L_OpenProgressWindow, struct _ProgressWindow *, REG(a0, struct TagItem *tags), REG(a6, struct MyLibrary *lib))

LIBSTUB_1(L_CloseProgressWindow, void, REG(a0, struct _ProgressWindow *prog))

LIBSTUB_1(L_HideProgressWindow, void, REG(a0, struct _ProgressWindow *prog))

LIBSTUB_3(L_ShowProgressWindow,
		  void,
		  REG(a0, struct _ProgressWindow *prog),
		  REG(a1, struct Screen *screen),
		  REG(a2, struct Window *window))

LIBSTUB_2(L_SetProgressWindow, void, REG(a0, struct _ProgressWindow *prog), REG(a1, struct TagItem *tags))

LIBSTUB_1(L_CheckProgressAbort, BOOL, REG(a0, struct _ProgressWindow *prog))

LIBSTUB_3(L_CalcPercent, long, REG(d0, ULONG amount), REG(d1, ULONG total), REG(a0, struct Library *UtilityBase))

// string_hook
LIBSTUB_1(L_GetSecureString, char *, REG(a0, struct Gadget *gadget))

// search
LIBSTUB_5(L_SearchFile,
		  long,
		  REG(a0, APTR file),
		  REG(a1, UBYTE *search_text),
		  REG(d0, ULONG flags),
		  REG(a2, UBYTE *buffer),
		  REG(d1, ULONG buffer_size))

// dates
LIBSTUB_4(L_ParseDateStrings,
		  char *,
		  REG(a0, char *string),
		  REG(a1, char *date_buffer),
		  REG(a2, char *time_buffer),
		  REG(a3, long *range))

LIBSTUB_3(L_DateFromStrings, BOOL, REG(a0, char *date), REG(a1, char *time), REG(a2, struct DateStamp *ds))

LIBSTUB_4(L_DateFromStringsNew,
		  BOOL,
		  REG(a0, char *date),
		  REG(a1, char *time),
		  REG(a2, struct DateStamp *ds),
		  REG(d0, ULONG method))

// filetypes
LIBSTUB_2(L_GetMatchHandle, APTR, REG(a0, char *name), REG(a6, struct MyLibrary *lib))

LIBSTUB_1(L_FreeMatchHandle, void, REG(a0, MatchHandle *handle))
LIBSTUB_3(L_MatchFiletype,
		  BOOL,
		  REG(a0, MatchHandle *handle),
		  REG(a1, Cfg_Filetype *type),
		  REG(a6, struct MyLibrary *lib))

// dos patches
LIBSTUB_1(L_PatchedCreateDir, BPTR, REG(d1, char *name))
LIBSTUB_2(L_OriginalCreateDir, BPTR, REG(d1, char *name), REG(a6, struct MyLibrary *libbase))
LIBSTUB_2(L_OriginalDeleteFile, long, REG(d1, char *name), REG(a6, struct MyLibrary *libbase))

// requesters
LIBSTUB_7(L_AsyncRequest,
		  long,
		  REG(a0, IPCData *my_ipc),
		  REG(d0, long type),
		  REG(a1, struct Window *window),
		  REG(a2, REF_CALLBACK callback),
		  REG(a3, APTR data),
		  REG(d1, struct TagItem *tags),
		  REG(a6, struct MyLibrary *libbase))

LIBSTUB_2(L_CheckRefreshMsg, struct IntuiMessage *, REG(a0, struct Window *window), REG(d0, ULONG mask))

// bitmap.c
LIBSTUB_5(L_NewBitMap,
		  struct BitMap *,
		  REG(d0, ULONG sizex),
		  REG(d1, ULONG sizey),
		  REG(d2, ULONG depth),
		  REG(d3, ULONG flags),
		  REG(a0, struct BitMap *friend_bitmap))
LIBSTUB_1(L_DisposeBitMap, void, REG(a0, struct BitMap *bm))

// args.c
LIBSTUB_2(L_ParseArgs, FuncArgs *, REG(a0, char *template), REG(a1, char *args))
LIBSTUB_1(L_DisposeArgs, void, REG(a0, FuncArgs *args))

// appmsg.c
LIBSTUB_3(L_AllocAppMessage,
		  DOpusAppMessage *,
		  REG(a0, APTR memory),
		  REG(a1, struct MsgPort *reply),
		  REG(d0, short num))
LIBSTUB_1(L_FreeAppMessage, void, REG(a0, DOpusAppMessage *msg))
LIBSTUB_1(L_ReplyAppMessage, void, REG(a0, DOpusAppMessage *msg))
LIBSTUB_1(L_CheckAppMessage, BOOL, REG(a0, DOpusAppMessage *msg))
LIBSTUB_2(L_CopyAppMessage, DOpusAppMessage *, REG(a0, DOpusAppMessage *orig), REG(a1, APTR memory))
LIBSTUB_5(L_SetWBArg,
		  BOOL,
		  REG(a0, DOpusAppMessage *msg),
		  REG(d0, short num),
		  REG(d1, BPTR lock),
		  REG(a1, char *name),
		  REG(a2, APTR memory))

// notify.c
LIBSTUB_4(L_AddNotifyRequest,
		  APTR,
		  REG(d0, ULONG type),
		  REG(d1, ULONG userdata),
		  REG(a0, struct MsgPort *port),
		  REG(a6, struct MyLibrary *libbase))
LIBSTUB_2(L_RemoveNotifyRequest, void, REG(a0, NotifyNode *node), REG(a6, struct MyLibrary *libbase))
LIBSTUB_7(L_SendNotifyMsg,
		  void,
		  REG(d0, ULONG type),
		  REG(d1, IPTR data),
		  REG(d2, ULONG flags),
		  REG(d3, short wait),
		  REG(a0, char *name),
		  REG(a1, struct FileInfoBlock *fib),
		  REG(a6, struct MyLibrary *libbase))

// backfill stuff
LIBSTUB_3(L_SetReqBackFill,
		  void,
		  REG(a0, struct Hook *hook),
		  REG(a1, struct Screen **screen),
		  REG(a6, struct MyLibrary *libbase))
LIBSTUB_2(L_LockReqBackFill, struct Hook *, REG(a0, struct Screen *screen), REG(a6, struct MyLibrary *libbase))
LIBSTUB_1(L_UnlockReqBackFill, void, REG(a6, struct MyLibrary *libbase))
LIBSTUB_3(L_SetEnv, void, REG(a0, char *name), REG(a1, char *data), REG(d0, BOOL save))
LIBSTUB_3(L_SavePos, BOOL, REG(a0, char *name), REG(a1, struct IBox *box), REG(d0, short fontsize))
LIBSTUB_3(L_LoadPos, BOOL, REG(a0, char *name), REG(a1, struct IBox *box), REG(d0, short *fontsize))
LIBSTUB_3(L_SetConfigWindowLimits,
		  void,
		  REG(a0, struct Window *window),
		  REG(a1, ConfigWindow *mindims),
		  REG(a2, ConfigWindow *maxdims))
LIBSTUB_3(L_OpenIFFFile, struct IFFHandle *, REG(a0, char *name), REG(d0, int mode), REG(d1, ULONG check_id))
LIBSTUB_1(L_CloseIFFFile, void, REG(a0, struct IFFHandle *iff))
LIBSTUB_12(L_SelectionList,
		   short,
		   REG(a0, Att_List *list),
		   REG(a1, struct Window *parent),
		   REG(a2, struct Screen *screen),
		   REG(a3, char *title),
		   REG(d0, short selection),
		   REG(d1, ULONG flags),
		   REG(d2, char *buffer),
		   REG(d3, char *okay_txt),
		   REG(d4, char *cancel_txt),
		   REG(a4, char **switch_txt),
		   REG(a5, ULONG *switch_flags),
		   REG(a6, struct MyLibrary *lib))
LIBSTUB_3(L_SaveSettings, int, REG(a0, CFG_SETS *settings), REG(a1, char *name), REG(a6, struct MyLibrary *libbase))
LIBSTUB_3(L_SaveButtonBank, int, REG(a0, Cfg_ButtonBank *bank), REG(a1, char *name), REG(a6, struct MyLibrary *libbase))
LIBSTUB_3(L_SaveFiletypeList,
		  int,
		  REG(a0, Cfg_FiletypeList *list),
		  REG(a1, char *name),
		  REG(a6, struct MyLibrary *libbase))
LIBSTUB_3(L_WB_Launch, BOOL, REG(a0, char *name), REG(a1, struct Screen *errors), REG(d0, short wait))
LIBSTUB_7(L_CLI_Launch,
		  BOOL,
		  REG(a0, char *name),
		  REG(a1, struct Screen *errors),
		  REG(d0, BPTR currentdir),
		  REG(d1, BPTR input),
		  REG(d2, BPTR output),
		  REG(d3, short wait),
		  REG(d4, long stack))
LIBSTUB_2(L_SerialCheck, BOOL, REG(a0, char *sernum), REG(a1, ULONG *nullp))
LIBSTUB_2(L_ChecksumFile, ULONG, REG(a0, char *filename), REG(d0, ULONG skip_marker))
LIBSTUB_1(L_ReplyFreeMsg, void, REG(a0, struct Message *msg))
LIBSTUB_1(L_TimerActive, BOOL, REG(a0, TimerHandle *handle))

LIBSTUB_1(L_WB_AppIconFlags, unsigned long, REG(a0, struct AppIcon *icon))

LIBSTUB_1(L_GetIconFlags, ULONG, REG(a0, struct DiskObject *icon))

LIBSTUB_2(L_SetIconFlags, void, REG(a0, struct DiskObject *icon), REG(d0, ULONG flags))

LIBSTUB_3(L_GetIconPosition, void, REG(a0, struct DiskObject *icon), REG(a1, short *x), REG(a2, short *y))

LIBSTUB_3(L_SetIconPosition, void, REG(a0, struct DiskObject *icon), REG(d0, short x), REG(d1, short y))

LIBSTUB_1(L_GetImagePalette, ULONG *, REG(a0, APTR ptr))

LIBSTUB_1(L_FreeImageRemap, void, REG(a0, ImageRemap *remap))

LIBSTUB_1(L_IFFFailure, void, REG(a0, IFFHandle *iff))

LIBSTUB_2(L_GetProgressWindow, void, REG(a0, ProgressWindow *prog), REG(a1, struct TagItem *tags))

LIBSTUB_4(L_SetNotifyRequest,
		  void,
		  REG(a0, NotifyNode *node),
		  REG(d0, ULONG new_flags),
		  REG(d1, ULONG mask),
		  REG(a6, struct MyLibrary *libbase))

LIBSTUB_3(L_NewButtonWithFunc, Cfg_Button *, REG(a0, APTR memory), REG(a1, char *label), REG(d0, short type))

LIBSTUB_5(L_GetFileVersion,
		  BOOL,
		  REG(a0, char *name),
		  REG(d0, short *version),
		  REG(d1, short *revision),
		  REG(a1, struct DateStamp *date),
		  REG(a2, APTR progress))

LIBSTUB_3(L_SetLibraryFlags, ULONG, REG(d0, ULONG flags), REG(d1, ULONG mask), REG(a6, struct MyLibrary *libbase))

LIBSTUB_2(L_CompareListFormat, ULONG, REG(a0, ListFormat *format1), REG(a1, ListFormat *format2))

LIBSTUB_2(L_UpdateGadgetList, void, REG(a0, ObjectList *list), REG(a6, struct MyLibrary *libbase))

LIBSTUB_7(L_IPC_SafeCommand,
		  IPTR,
		  REG(a0, IPCData *ipc),
		  REG(d0, ULONG command),
		  REG(d1, IPTR flags),
		  REG(a1, APTR data),
		  REG(a2, APTR data_free),
		  REG(a3, struct MsgPort *reply),
		  REG(a4, struct ListLock *list))

LIBSTUB_1(L_ClearFiletypeCache, void, REG(a6, struct MyLibrary *lib))

LIBSTUB_1(L_GetTimerBase, struct Library *, REG(a6, struct MyLibrary *lib))

LIBSTUB_1(L_InitDragDBuf, BOOL, REG(a0, DragInfo *drag))

LIBSTUB_1(L_FreeRexxMsgEx, void, REG(a0, struct RexxMsg *msg))

LIBSTUB_3(L_CreateRexxMsgEx,
		  struct RexxMsg *,
		  REG(a0, struct MsgPort *port),
		  REG(a1, UBYTE *extension),
		  REG(d0, UBYTE *host))
LIBSTUB_4(L_SetRexxVarEx,
		  long,
		  REG(a0, struct RexxMsg *msg),
		  REG(a1, char *varname),
		  REG(d0, char *value),
		  REG(d1, long length))

LIBSTUB_3(L_GetRexxVarEx, long, REG(a0, struct RexxMsg *msg), REG(a1, char *varname), REG(a2, char **bufpointer))

LIBSTUB_4(L_BuildRexxMsgEx,
		  struct RexxMsg *,
		  REG(a0, struct MsgPort *port),
		  REG(a1, UBYTE *extension),
		  REG(d0, UBYTE *host),
		  REG(a2, struct TagItem *tags))

LIBSTUB(L_NotifyDiskChange, void)

LIBSTUB_3(L_GetDosListCopy, void, REG(a0, struct List *list), REG(a1, APTR memory), REG(a6, struct MyLibrary *libbase))

LIBSTUB_2(L_FreeDosListCopy, void, REG(a0, struct List *list), REG(a6, struct MyLibrary *libbase))

LIBSTUB_3(L_RemapIcon,
		  BOOL,
		  REG(a0, struct DiskObject *icon),
		  REG(a1, struct Screen *screen),
		  REG(d0, short free_remap))

LIBSTUB_1(L_GetOriginalIcon, struct DiskObject *, REG(a0, struct DiskObject *icon))

LIBSTUB_1(L_IsDiskDevice, BOOL, REG(a0, struct MsgPort *port))

LIBSTUB_3(L_SetNewIconsFlags, void, REG(d0, ULONG flags), REG(d1, short precision), REG(a6, struct MyLibrary *libbase))

LIBSTUB_1(L_GetLibraryFlags, ULONG, REG(a6, struct MyLibrary *libbase))

LIBSTUB_5(L_WB_LaunchNew,
		  BOOL,
		  REG(a0, char *name),
		  REG(a1, struct Screen *errors),
		  REG(d0, short wait),
		  REG(d1, long stack),
		  REG(a2, char *default_tool))

LIBSTUB_1(L_UpdatePathList, void, REG(a6, struct MyLibrary *libbase))

LIBSTUB_1(L_UpdateMyPaths, void, REG(a6, struct MyLibrary *libbase))

LIBSTUB_4(L_GetPopUpImageSize,
		  void,
		  REG(a0, struct Window *window),
		  REG(a1, PopUpMenu *menu),
		  REG(a2, short *width),
		  REG(a3, short *height))

LIBSTUB_3(L_MUFSLogin, void, REG(a0, struct Window *window), REG(a1, char *name), REG(a2, char *password))

LIBSTUB_1(L_GetOpusPathList, BPTR, REG(a6, struct MyLibrary *libbase))

LIBSTUB_2(L_GetStatistics, long, REG(d0, long id), REG(a6, struct MyLibrary *libbase))
LIBSTUB_8(L_WB_LaunchNotify,
		  BOOL,
		  REG(a0, char *name),
		  REG(a1, struct Screen *errors),
		  REG(d0, short wait),
		  REG(d1, long stack),
		  REG(a2, char *default_tool),
		  REG(a3, struct Process **proc_ptr),
		  REG(a4, IPCData *notify_ipc),
		  REG(d2, ULONG flags))

LIBSTUB_1(L_WB_AppWindowWindow, struct Window *, REG(a0, struct AppWindow *window))

LIBSTUB_2(L_OpenEnvironment, BOOL, REG(a0, char *name), REG(a1, struct OpenEnvironmentData *data))

LIBSTUB_3(L_PopUpNewHandle,
		  PopUpHandle *,
		  REG(d0, IPTR userdata),
		  REG(a0, REF_CALLBACK(callback)),
		  REG(a1, struct DOpusLocale *locale))

LIBSTUB_1(L_PopUpFreeHandle, void, REG(a0, PopUpHandle *handle))
LIBSTUB_4(L_PopUpNewItem,
		  PopUpItem *,
		  REG(a0, PopUpHandle *handle),
		  REG(d0, IPTR string),
		  REG(d1, ULONG id),
		  REG(d2, ULONG flags))
LIBSTUB_1(L_PopUpSeparator, void, REG(a0, PopUpHandle *handle))
LIBSTUB_2(L_PopUpItemSub, BOOL, REG(a0, PopUpHandle *menu), REG(a1, PopUpItem *item))
LIBSTUB_1(L_PopUpEndSub, void, REG(a0, PopUpHandle *menu))
LIBSTUB_4(L_PopUpSetFlags,
		  ULONG,
		  REG(a0, PopUpMenu *menu),
		  REG(d0, UWORD id),
		  REG(d1, ULONG value),
		  REG(d2, ULONG mask))
LIBSTUB_3(L_AddAllocBitmapPatch,
		  APTR,
		  REG(a0, struct Task *task),
		  REG(a1, struct Screen *screen),
		  REG(a6, struct MyLibrary *libbase))
LIBSTUB_2(L_RemAllocBitmapPatch, void, REG(a0, APTR handle), REG(a6, struct MyLibrary *libbase))
LIBSTUB_1(L_Seed, void, REG(d0, int seed))
LIBSTUB_1(L_RandomDopus, void, REG(d0, int limit))

// clipboard (protos in clipboard.h)

LIBSTUB_1(L_OpenClipBoard, ClipHandle *, REG(d0, ULONG unit))
LIBSTUB_1(L_CloseClipBoard, void, REG(a0, ClipHandle *clip))
LIBSTUB_3(L_WriteClipString, BOOL, REG(a0, ClipHandle *clip), REG(a1, char *string), REG(d0, long length))
LIBSTUB_3(L_ReadClipString, long, REG(a0, ClipHandle *clip), REG(a1, char *string), REG(d0, long length))

// workbench (protos in wb.h)

LIBSTUB_1(L_WB_Install_Patch, void, REG(a6, struct MyLibrary *libbase))
LIBSTUB_1(L_WB_Remove_Patch, BOOL, REG(a6, struct MyLibrary *libbase))
LIBSTUB_7(L_WB_AddAppIcon,
		  struct AppIcon *,
		  REG(d0, IPTR id),
		  REG(d1, IPTR userdata),
		  REG(a0, char *text),
		  REG(a1, struct MsgPort *port),
		  REG(a2, BPTR lock),
		  REG(a3, struct DiskObject *object),
		  REG(a4, struct TagItem *tags))
LIBSTUB_1(L_WB_RemoveAppIcon, BOOL, REG(a0, struct AppIcon *icon))

LIBSTUB_5(L_WB_AddAppMenuItem,
		  struct AppMenuItem *,
		  REG(d0, IPTR id),
		  REG(d1, IPTR userdata),
		  REG(a0, char *text),
		  REG(a1, struct MsgPort *port),
		  REG(a2, struct TagItem *tags))
LIBSTUB_1(L_WB_RemoveAppMenuItem, BOOL, REG(a0, struct AppMenuItem *item))
LIBSTUB_2(L_WB_FindAppWindow, struct AppWindow *, REG(a0, struct Window *window), REG(a6, struct MyLibrary *libbase))
LIBSTUB_3(L_WB_AppWindowData,
		  struct MsgPort *,
		  REG(a0, struct AppWindow *window),
		  REG(a1, IPTR *id),
		  REG(a2, IPTR *userdata))

LIBSTUB_1(L_WB_AppWindowLocal, BOOL, REG(a0, struct AppWindow *window))

LIBSTUB_1(L_LockAppList, APTR, REG(a6, struct MyLibrary *libbase))

LIBSTUB_3(L_NextAppEntry, APTR, REG(a0, APTR last), REG(d0, ULONG type), REG(a6, struct MyLibrary *libbase))

LIBSTUB_1(L_UnlockAppList, void, REG(a6, struct MyLibrary *libbase))

LIBSTUB_2(L_WB_PutDiskObject, BOOL, REG(a0, char *name), REG(a1, struct DiskObject *diskobj))

LIBSTUB_3(L_CopyDiskObject,
		  struct DiskObject *,
		  REG(a0, struct DiskObject *icon),
		  REG(d0, ULONG flags),
		  REG(a6, struct MyLibrary *libbase))

LIBSTUB_2(L_FreeDiskObjectCopy, void, REG(a0, struct DiskObject *icon), REG(a6, struct MyLibrary *libbase))

LIBSTUB_1(L_WB_DeleteDiskObject, BOOL, REG(a0, char *name))

LIBSTUB_6(L_ChangeAppIcon,
		  void,
		  REG(a0, APTR appicon),
		  REG(a1, struct Image *render),
		  REG(a2, struct Image *select),
		  REG(a3, char *title),
		  REG(d0, ULONG flags),
		  REG(a6, struct MyLibrary *libbase))

LIBSTUB_3(L_SetAppIconMenuState, long, REG(a0, APTR appicon), REG(d0, long item), REG(d1, long state))

// string_hooks (proto in string_hook.h)

LIBSTUB_3(L_GetEditHook, HookData *, REG(d0, ULONG type), REG(d1, ULONG flags), REG(a0, struct TagItem *tags))

LIBSTUB_1(L_FreeEditHook, void, REG(a0, APTR hook))

// more dopus patched from dos_patch.h

LIBSTUB_3(L_OriginalSetFileDate,
		  BOOL,
		  REG(d1, char *name),
		  REG(d2, struct DateStamp *date),
		  REG(a6, struct MyLibrary *libbase))

LIBSTUB_3(L_OriginalSetComment, BOOL, REG(d1, char *name), REG(d2, char *comment), REG(a6, struct MyLibrary *libbase))

LIBSTUB_3(L_OriginalSetProtection, BOOL, REG(d1, char *name), REG(d2, ULONG mask), REG(a6, struct MyLibrary *libbase))
LIBSTUB_3(L_OriginalRename, BOOL, REG(d1, char *oldname), REG(d2, char *newname), REG(a6, struct MyLibrary *libbase))

LIBSTUB_3(L_OriginalOpen, BPTR, REG(d1, char *name), REG(d2, LONG accessMode), REG(a6, struct MyLibrary *libbase))

LIBSTUB_2(L_OriginalClose, BOOL, REG(d1, BPTR file), REG(a6, struct MyLibrary *libbase))

LIBSTUB_4(L_OriginalWrite,
		  LONG,
		  REG(d1, BPTR file),
		  REG(d2, void *data),
		  REG(d3, LONG length),
		  REG(a6, struct MyLibrary *libbase))

LIBSTUB_3(L_OriginalRelabel, BOOL, REG(d1, char *volumename), REG(d2, char *name), REG(a6, struct MyLibrary *libbase))

// 64bit.c
LIBSTUB_4(L_DivideU64, void, REG(a0, UQUAD *num), REG(d0, ULONG div), REG(a1, UQUAD *rem), REG(a2, UQUAD *quo))

LIBSTUB_4(L_ItoaU64, void, REG(a0, UQUAD *num), REG(a1, char *str), REG(d0, int str_size), REG(d1, char sep))

LIBSTUB_6(L_DivideToString64,
		  void,
		  REG(a0, char *string),
		  REG(d0, int str_size),
		  REG(a1, UQUAD *bytes),
		  REG(d1, ULONG div),
		  REG(d2, int places),
		  REG(d3, char sep))

LIBSTUB_5(L_BytesToString64,
		  void,
		  REG(a0, UQUAD *bytes),
		  REG(a1, char *string),
		  REG(d0, int str_size),
		  REG(d1, int places),
		  REG(d2, char sep))

LIBSTUB_2(L_ExamineLock64, BOOL, REG(d0, BPTR lock), REG(a0, FileInfoBlock64 *fib))

LIBSTUB_2(L_ExamineNext64, BOOL, REG(d0, BPTR lock), REG(a0, FileInfoBlock64 *fib))

LIBSTUB_2(L_ExamineHandle64, BOOL, REG(d0, BPTR fh), REG(a0, FileInfoBlock64 *fib))

LIBSTUB_2(L_MatchFirst64, LONG, REG(a0, STRPTR pat), REG(a1, struct AnchorPath *panchor))

LIBSTUB_1(L_MatchNext64, LONG, REG(a0, struct AnchorPath *panchor))
