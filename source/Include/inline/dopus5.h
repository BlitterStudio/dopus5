/* Automatically generated header (sfdc 1.11)! Do not edit! */

#ifndef _INLINE_DOPUS_H
#define _INLINE_DOPUS_H

#ifndef _SFDC_VARARG_DEFINED
#define _SFDC_VARARG_DEFINED
#ifdef __HAVE_IPTR_ATTR__
typedef APTR _sfdc_vararg __attribute__((iptr));
#else
typedef ULONG _sfdc_vararg;
#endif /* __HAVE_IPTR_ATTR__ */
#endif /* _SFDC_VARARG_DEFINED */

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif /* !__INLINE_MACROS_H */

#ifndef DOPUS_BASE_NAME
#define DOPUS_BASE_NAME DOpusBase
#endif /* !DOPUS_BASE_NAME */

#define RemovedFunc0() \
      LP0(0x1e, ULONG, RemovedFunc0 ,\
      , DOPUS_BASE_NAME)

#define RandomDopus(___limit) \
      LP1(0x24, UWORD, RandomDopus , int, ___limit, d0,\
      , DOPUS_BASE_NAME)

#define Atoh(___str, ___len) \
      LP2(0x2a, ULONG, Atoh , char *, ___str, a0, short, ___len, d0,\
      , DOPUS_BASE_NAME)

#define BtoCStr(___bstr, ___cstr, ___len) \
      LP3NR(0x30, BtoCStr , BSTR, ___bstr, a0, char *, ___cstr, a1, int, ___len, d0,\
      , DOPUS_BASE_NAME)

#define DivideU(___num, ___div, ___rem, ___utility) \
      LP4(0x36, ULONG, DivideU , ULONG, ___num, d0, ULONG, ___div, d1, ULONG *, ___rem, a0, struct Library *, ___utility, a1,\
      , DOPUS_BASE_NAME)

#define Itoa(___num, ___str, ___sep) \
      LP3NR(0x3c, Itoa , long, ___num, d0, char *, ___str, a0, char, ___sep, d1,\
      , DOPUS_BASE_NAME)

#define ItoaU(___num, ___str, ___sep) \
      LP3NR(0x42, ItoaU , ULONG, ___num, d0, char *, ___str, a0, char, ___sep, d1,\
      , DOPUS_BASE_NAME)

#define Ito26(___num, ___str) \
      LP2NR(0x48, Ito26 , ULONG, ___num, d0, char *, ___str, a0,\
      , DOPUS_BASE_NAME)

#define BytesToString(___bytes, ___string, ___places, ___sep) \
      LP4NR(0x4e, BytesToString , ULONG, ___bytes, d0, char *, ___string, a0, short, ___places, d1, char, ___sep, d2,\
      , DOPUS_BASE_NAME)

#define DivideToString(___string, ___num, ___div, ___places, ___sep) \
      LP5NR(0x54, DivideToString , char *, ___string, a0, ULONG, ___num, d0, ULONG, ___div, d1, short, ___places, d2, char, ___sep, d3,\
      , DOPUS_BASE_NAME)

#define SetBusyPointer(___window) \
      LP1NR(0x5a, SetBusyPointer , struct Window *, ___window, a0,\
      , DOPUS_BASE_NAME)

#define BuildKeyString(___code, ___qual, ___mask, ___same, ___buffer) \
      LP5NR(0x60, BuildKeyString , UWORD, ___code, d0, UWORD, ___qual, d1, UWORD, ___mask, d2, UWORD, ___same, d3, char *, ___buffer, a0,\
      , DOPUS_BASE_NAME)

#define ActivateStrGad(___gadget, ___window) \
      LP2NR(0x66, ActivateStrGad , struct Gadget *, ___gadget, a0, struct Window *, ___window, a1,\
      , DOPUS_BASE_NAME)

#define AllocTimer(___unit, ___port) \
      LP2(0x6c, struct TimerHandle *, AllocTimer , ULONG, ___unit, d0, struct MsgPort *, ___port, a0,\
      , DOPUS_BASE_NAME)

#define FreeTimer(___handle) \
      LP1NR(0x72, FreeTimer , TimerHandle *, ___handle, a0,\
      , DOPUS_BASE_NAME)

#define StartTimer(___handle, ___secs, ___micro) \
      LP3NR(0x78, StartTimer , TimerHandle *, ___handle, a0, ULONG, ___secs, d0, ULONG, ___micro, d1,\
      , DOPUS_BASE_NAME)

#define CheckTimer(___handle) \
      LP1(0x7e, BOOL, CheckTimer , TimerHandle *, ___handle, a0,\
      , DOPUS_BASE_NAME)

#define StopTimer(___handle) \
      LP1NR(0x84, StopTimer , TimerHandle *, ___handle, a0,\
      , DOPUS_BASE_NAME)

#define GetDosPathList(___copy) \
      LP1(0x8a, BPTR, GetDosPathList , BPTR, ___copy, a0,\
      , DOPUS_BASE_NAME)

#define FreeDosPathList(___list) \
      LP1NR(0x90, FreeDosPathList , BPTR, ___list, a0,\
      , DOPUS_BASE_NAME)

#define DoSimpleRequest(___window, ___simplereq) \
      LP2(0x96, short, DoSimpleRequest , struct Window *, ___window, a0, struct DOpusSimpleRequest *, ___simplereq, a1,\
      , DOPUS_BASE_NAME)

#define SimpleRequest(___parent, ___title, ___buttons, ___message, ___buffer, ___data, ___bufsize, ___flags) \
      LP8(0x9c, short, SimpleRequest , struct Window *, ___parent, a0, char *, ___title, a1, char *, ___buttons, a2, char *, ___message, a3, char *, ___buffer, d0, APTR, ___data, d1, long, ___bufsize, d2, ULONG, ___flags, d3,\
      , DOPUS_BASE_NAME)

#define SelectionList(___list, ___parent, ___screen, ___title, ___ok, ___cancel, ___sel, ___flags, ___buffer, ___switchtxt, ___switchflags) \
      LP11(0xa2, short, SelectionList , Att_List *, ___list, a0, struct Window *, ___parent, a1, struct Screen *, ___screen, a2, char *, ___title, a3, short, ___ok, d0, ULONG, ___cancel, d1, char *, ___sel, d2, char *, ___flags, d3, char *, ___buffer, d4, char **, ___switchtxt, d5, ULONG *, ___switchflags, d6,\
      , DOPUS_BASE_NAME)

#define WriteIcon(___name, ___obj) \
      LP2(0xa8, BOOL, WriteIcon , char *, ___name, a0, struct DiskObject *, ___obj, a1,\
      , DOPUS_BASE_NAME)

#define WriteFileIcon(___source, ___dest) \
      LP2NR(0xae, WriteFileIcon , char *, ___source, a0, char *, ___dest, a1,\
      , DOPUS_BASE_NAME)

#define GetDragInfo(___window, ___rast, ___width, ___height, ___flags) \
      LP5(0xb4, DragInfo *, GetDragInfo , struct Window *, ___window, a0, struct RastPort *, ___rast, a1, long, ___width, d0, long, ___height, d1, long, ___flags, d2,\
      , DOPUS_BASE_NAME)

#define FreeDragInfo(___draginfo) \
      LP1NR(0xba, FreeDragInfo , DragInfo *, ___draginfo, a0,\
      , DOPUS_BASE_NAME)

#define GetDragImage(___draginfo, ___x, ___y) \
      LP3NR(0xc0, GetDragImage , DragInfo *, ___draginfo, a0, ULONG, ___x, d0, ULONG, ___y, d1,\
      , DOPUS_BASE_NAME)

#define ShowDragImage(___draginfo, ___x, ___y) \
      LP3NR(0xc6, ShowDragImage , DragInfo *, ___draginfo, a0, ULONG, ___x, d0, ULONG, ___y, d1,\
      , DOPUS_BASE_NAME)

#define HideDragImage(___draginfo) \
      LP1NR(0xcc, HideDragImage , DragInfo *, ___draginfo, a0,\
      , DOPUS_BASE_NAME)

#define StampDragImage(___draginfo, ___x, ___y) \
      LP3NR(0xd2, StampDragImage , DragInfo *, ___draginfo, a0, ULONG, ___x, d0, ULONG, ___y, d1,\
      , DOPUS_BASE_NAME)

#define GetDragMask(___drag) \
      LP1NR(0xd8, GetDragMask , DragInfo *, ___drag, a0,\
      , DOPUS_BASE_NAME)

#define CheckDragDeadlock(___drag) \
      LP1(0xde, BOOL, CheckDragDeadlock , DragInfo *, ___drag, a0,\
      , DOPUS_BASE_NAME)

#define AddDragImage(___drag) \
      LP1NR(0xe4, AddDragImage , DragInfo *, ___drag, a0,\
      , DOPUS_BASE_NAME)

#define RemDragImage(___drag) \
      LP1NR(0xea, RemDragImage , DragInfo *, ___drag, a0,\
      , DOPUS_BASE_NAME)

#define OpenConfigWindow(___newwindow) \
      LP1(0xf0, struct Window *, OpenConfigWindow , NewConfigWindow *, ___newwindow, a0,\
      , DOPUS_BASE_NAME)

#define CloseConfigWindow(___window) \
      LP1NR(0xf6, CloseConfigWindow , struct Window *, ___window, a0,\
      , DOPUS_BASE_NAME)

#define GetWindowMsg(___port) \
      LP1(0xfc, struct IntuiMessage *, GetWindowMsg , struct MsgPort *, ___port, a0,\
      , DOPUS_BASE_NAME)

#define ReplyWindowMsg(___msg) \
      LP1NR(0x102, ReplyWindowMsg , struct IntuiMessage *, ___msg, a0,\
      , DOPUS_BASE_NAME)

#define StripIntuiMessagesDopus(___window) \
      LP1NR(0x108, StripIntuiMessagesDopus , struct Window *, ___window, a0,\
      , DOPUS_BASE_NAME)

#define CloseWindowSafely(___window) \
      LP1NR(0x10e, CloseWindowSafely , struct Window *, ___window, a0,\
      , DOPUS_BASE_NAME)

#define CalcObjectDims(___parent, ___font, ___pos, ___dest, ___last, ___flags, ___obj, ___ob) \
      LP8A4(0x114, int, CalcObjectDims , void *, ___parent, a0, struct TextFont *, ___font, a1, struct IBox *, ___pos, a2, struct IBox *, ___dest, a3, GL_Object *, ___last, d7, ULONG, ___flags, d0, GL_Object *, ___obj, d1, GL_Object *, ___ob, d2,\
      , DOPUS_BASE_NAME)

#define CalcWindowDims(___screen, ___pos, ___dest, ___font, ___flags) \
      LP5(0x11a, int, CalcWindowDims , struct Screen *, ___screen, a0, ConfigWindow *, ___pos, a1, struct IBox *, ___dest, a2, struct TextFont *, ___font, a3, ULONG, ___flags, d0,\
      , DOPUS_BASE_NAME)

#define AddObjectList(___window, ___objects) \
      LP2(0x120, ObjectList *, AddObjectList , struct Window *, ___window, a0, const ObjectDef *, ___objects, a1,\
      , DOPUS_BASE_NAME)

#define FreeObject(___objlist, ___object) \
      LP2NR(0x126, FreeObject , ObjectList *, ___objlist, a0, GL_Object *, ___object, a1,\
      , DOPUS_BASE_NAME)

#define FreeObjectList(___objlist) \
      LP1NR(0x12c, FreeObjectList , ObjectList *, ___objlist, a0,\
      , DOPUS_BASE_NAME)

#define RefreshObjectList(___window, ___list) \
      LP2NR(0x132, RefreshObjectList , struct Window *, ___window, a0, ObjectList *, ___list, a1,\
      , DOPUS_BASE_NAME)

#define GetObject(___list, ___id) \
      LP2(0x138, GL_Object *, GetObject , ObjectList *, ___list, a0, int, ___id, d0,\
      , DOPUS_BASE_NAME)

#define StoreGadgetValue(___list, ___msg) \
      LP2NR(0x13e, StoreGadgetValue , ObjectList *, ___list, a0, struct IntuiMessage *, ___msg, a1,\
      , DOPUS_BASE_NAME)

#define SetGadgetValue(___list, ___id, ___value) \
      LP3NR(0x144, SetGadgetValue , ObjectList *, ___list, a0, UWORD, ___id, d0, IPTR, ___value, d1,\
      , DOPUS_BASE_NAME)

#define GetGadgetValue(___list, ___id) \
      LP2(0x14a, IPTR, GetGadgetValue , ObjectList *, ___list, a0, UWORD, ___id, a1,\
      , DOPUS_BASE_NAME)

#define SetObjectKind(___list, ___id, ___kind) \
      LP3NR(0x150, SetObjectKind , ObjectList *, ___list, a0, ULONG, ___id, d0, UWORD, ___kind, d1,\
      , DOPUS_BASE_NAME)

#define SetGadgetChoices(___list, ___id, ___choices) \
      LP3NR(0x156, SetGadgetChoices , ObjectList *, ___list, a0, ULONG, ___id, d0, APTR, ___choices, a1,\
      , DOPUS_BASE_NAME)

#define CheckObjectArea(___object, ___x, ___y) \
      LP3(0x15c, BOOL, CheckObjectArea , GL_Object *, ___object, a0, int, ___x, d0, int, ___y, d1,\
      , DOPUS_BASE_NAME)

#define GetObjectRect(___list, ___id, ___rect) \
      LP3(0x162, BOOL, GetObjectRect , ObjectList *, ___list, a0, ULONG, ___id, d0, struct Rectangle *, ___rect, a1,\
      , DOPUS_BASE_NAME)

#define DisplayObject(___window, ___object, ___fg, ___bg, ___txt) \
      LP5NR(0x168, DisplayObject , struct Window *, ___window, a0, GL_Object *, ___object, a1, int, ___fg, d0, int, ___bg, d1, char *, ___txt, a2,\
      , DOPUS_BASE_NAME)

#define DisableObject(___list, ___id, ___state) \
      LP3NR(0x16e, DisableObject , ObjectList *, ___list, a0, ULONG, ___id, d0, BOOL, ___state, d1,\
      , DOPUS_BASE_NAME)

#define BoundsCheckGadget(___list, ___id, ___min, ___max) \
      LP4(0x174, int, BoundsCheckGadget , ObjectList *, ___list, a0, ULONG, ___id, d0, int, ___min, d1, int, ___max, d2,\
      , DOPUS_BASE_NAME)

#define AddWindowMenus(___window, ___data) \
      LP2NR(0x17a, AddWindowMenus , struct Window *, ___window, a0, const MenuData *, ___data, a1,\
      , DOPUS_BASE_NAME)

#define FreeWindowMenus(___window) \
      LP1NR(0x180, FreeWindowMenus , struct Window *, ___window, a0,\
      , DOPUS_BASE_NAME)

#define SetWindowBusy(___window) \
      LP1NR(0x186, SetWindowBusy , struct Window *, ___window, a0,\
      , DOPUS_BASE_NAME)

#define ClearWindowBusy(___window) \
      LP1NR(0x18c, ClearWindowBusy , struct Window *, ___window, a0,\
      , DOPUS_BASE_NAME)

#define GetString(___locale, ___num) \
      LP2(0x192, STRPTR, GetString , struct DOpusLocale *, ___locale, a0, LONG, ___num, d0,\
      , DOPUS_BASE_NAME)

#define FindKeyEquivalent(___list, ___msg, ___process) \
      LP3(0x198, struct Gadget *, FindKeyEquivalent , ObjectList *, ___list, a0, struct IntuiMessage *, ___msg, a1, int, ___process, d0,\
      , DOPUS_BASE_NAME)

#define ShowProgressBar(___window, ___object, ___total, ___count) \
      LP4NR(0x19e, ShowProgressBar , struct Window *, ___window, a0, GL_Object *, ___object, a1, ULONG, ___total, d0, ULONG, ___count, d1,\
      , DOPUS_BASE_NAME)

#define SetWindowID(___window, ___id_ptr, ___id, ___port) \
      LP4NR(0x1a4, SetWindowID , struct Window *, ___window, a0, WindowID *, ___id_ptr, a1, ULONG, ___id, d0, struct MsgPort *, ___port, a2,\
      , DOPUS_BASE_NAME)

#define GetWindowID(___window) \
      LP1(0x1aa, ULONG, GetWindowID , struct Window *, ___window, a0,\
      , DOPUS_BASE_NAME)

#define GetWindowAppPort(___window) \
      LP1(0x1b0, struct MsgPort *, GetWindowAppPort , struct Window *, ___window, a0,\
      , DOPUS_BASE_NAME)

#define Att_NewList(___flags) \
      LP1(0x1b6, Att_List *, Att_NewList , ULONG, ___flags, d0,\
      , DOPUS_BASE_NAME)

#define Att_NewNode(___list, ___name, ___data, ___flags) \
      LP4(0x1bc, Att_Node *, Att_NewNode , Att_List *, ___list, a0, const char *, ___name, a1, IPTR, ___data, d0, ULONG, ___flags, d1,\
      , DOPUS_BASE_NAME)

#define Att_RemNode(___node) \
      LP1NR(0x1c2, Att_RemNode , Att_Node *, ___node, a0,\
      , DOPUS_BASE_NAME)

#define Att_PosNode(___list, ___node, ___before) \
      LP3NR(0x1c8, Att_PosNode , Att_List *, ___list, a0, Att_Node *, ___node, a1, Att_Node *, ___before, a2,\
      , DOPUS_BASE_NAME)

#define Att_RemList(___list, ___flags) \
      LP2NR(0x1ce, Att_RemList , Att_List *, ___list, a0, long, ___flags, d0,\
      , DOPUS_BASE_NAME)

#define Att_FindNode(___list, ___number) \
      LP2(0x1d4, Att_Node *, Att_FindNode , Att_List *, ___list, a0, long, ___number, d0,\
      , DOPUS_BASE_NAME)

#define Att_NodeNumber(___list, ___name) \
      LP2(0x1da, long, Att_NodeNumber , Att_List *, ___list, a0, char *, ___name, a1,\
      , DOPUS_BASE_NAME)

#define Att_FindNodeData(___list, ___data) \
      LP2(0x1e0, Att_Node *, Att_FindNodeData , Att_List *, ___list, a0, IPTR, ___data, d0,\
      , DOPUS_BASE_NAME)

#define Att_NodeDataNumber(___list, ___data) \
      LP2(0x1e6, long, Att_NodeDataNumber , Att_List *, ___list, a0, IPTR, ___data, d0,\
      , DOPUS_BASE_NAME)

#define Att_NodeName(___list, ___number) \
      LP2(0x1ec, char *, Att_NodeName , Att_List *, ___list, a0, long, ___number, d0,\
      , DOPUS_BASE_NAME)

#define Att_NodeCount(___list) \
      LP1(0x1f2, long, Att_NodeCount , Att_List *, ___list, a0,\
      , DOPUS_BASE_NAME)

#define Att_ChangeNodeName(___node, ___name) \
      LP2NR(0x1f8, Att_ChangeNodeName , Att_Node *, ___node, a0, char *, ___name, a1,\
      , DOPUS_BASE_NAME)

#define Att_FindNodeNumber(___list, ___node) \
      LP2(0x1fe, long, Att_FindNodeNumber , Att_List *, ___list, a0, Att_Node *, ___node, a1,\
      , DOPUS_BASE_NAME)

#define AddSorted(___list, ___node) \
      LP2NR(0x204, AddSorted , struct List *, ___list, a0, struct Node *, ___node, a1,\
      , DOPUS_BASE_NAME)

#define BuildMenuStrip(___data, ___locale) \
      LP2(0x20a, struct Menu *, BuildMenuStrip , const MenuData *, ___data, a0, struct DOpusLocale *, ___locale, a1,\
      , DOPUS_BASE_NAME)

#define FindMenuItem(___menu, ___id) \
      LP2(0x210, struct MenuItem *, FindMenuItem , struct Menu *, ___menu, a0, UWORD, ___id, d0,\
      , DOPUS_BASE_NAME)

#define DoPopUpMenu(___window, ___menu, ___item, ___code) \
      LP4(0x216, UWORD, DoPopUpMenu , struct Window *, ___window, a0, PopUpMenu *, ___menu, a1, PopUpItem **, ___item, a2, UWORD, ___code, d0,\
      , DOPUS_BASE_NAME)

#define GetPopUpItem(___menu, ___id) \
      LP2(0x21c, PopUpItem *, GetPopUpItem , PopUpMenu *, ___menu, a0, UWORD, ___id, d0,\
      , DOPUS_BASE_NAME)

#define IPC_Startup(___ipc, ___data, ___reply) \
      LP3(0x222, int, IPC_Startup , IPCData *, ___ipc, a0, APTR, ___data, a1, struct MsgPort *, ___reply, a2,\
      , DOPUS_BASE_NAME)

#define IPC_Command(___ipc, ___command, ___flags, ___data, ___data_free, ___reply) \
      LP6(0x228, IPTR, IPC_Command , IPCData *, ___ipc, a0, ULONG, ___command, d0, IPTR, ___flags, d1, APTR, ___data, a1, APTR, ___data_free, a2, struct MsgPort *, ___reply, a3,\
      , DOPUS_BASE_NAME)

#define IPC_Reply(___msg) \
      LP1NR(0x22e, IPC_Reply , IPCMessage *, ___msg, a0,\
      , DOPUS_BASE_NAME)

#define IPC_Free(___ipc) \
      LP1NR(0x234, IPC_Free , IPCData *, ___ipc, a0,\
      , DOPUS_BASE_NAME)

#define IPC_FindProc(___list, ___name, ___act, ___data) \
      LP4(0x23a, IPCData *, IPC_FindProc , struct ListLock *, ___list, a0, char *, ___name, a1, BOOL, ___act, d0, IPTR, ___data, d1,\
      , DOPUS_BASE_NAME)

#define IPC_Quit(___ipc, ___flags, ___wait) \
      LP3NR(0x240, IPC_Quit , IPCData *, ___ipc, a0, ULONG, ___flags, d0, BOOL, ___wait, d1,\
      , DOPUS_BASE_NAME)

#define IPC_Hello(___ipc, ___owner) \
      LP2NR(0x246, IPC_Hello , IPCData *, ___ipc, a0, IPCData *, ___owner, a1,\
      , DOPUS_BASE_NAME)

#define IPC_Goodbye(___ipc, ___owner, ___flags) \
      LP3NR(0x24c, IPC_Goodbye , IPCData *, ___ipc, a0, IPCData *, ___owner, a1, ULONG, ___flags, d0,\
      , DOPUS_BASE_NAME)

#define IPC_GetGoodbye(___msg) \
      LP1(0x252, IPTR, IPC_GetGoodbye , IPCMessage *, ___msg, a0,\
      , DOPUS_BASE_NAME)

#define IPC_ListQuit(___list, ___owner, ___flags, ___wait) \
      LP4(0x258, ULONG, IPC_ListQuit , struct ListLock *, ___list, a0, IPCData *, ___owner, a1, ULONG, ___flags, d0, BOOL, ___wait, d1,\
      , DOPUS_BASE_NAME)

#define IPC_Flush(___port) \
      LP1NR(0x25e, IPC_Flush , IPCData *, ___port, a0,\
      , DOPUS_BASE_NAME)

#define IPC_ListCommand(___list, ___command, ___flags, ___data, ___wait) \
      LP5NR(0x264, IPC_ListCommand , struct ListLock *, ___list, a0, ULONG, ___command, d0, IPTR, ___flags, d1, IPTR, ___data, d2, BOOL, ___wait, d3,\
      , DOPUS_BASE_NAME)

#define IPC_ProcStartup(___data, ___code) \
      LP2FP(0x26a, IPCData *, IPC_ProcStartup , IPTR *, ___data, a0, __fpt, ___code, a1,\
      , DOPUS_BASE_NAME, ULONG (*__fpt)(IPCData *,APTR))

#define IPC_Launch(___list, ___ipc, ___name, ___entry, ___stack, ___data, ___dos) \
      LP7(0x270, int, IPC_Launch , struct ListLock *, ___list, a0, IPCData **, ___ipc, a1, char *, ___name, a2, IPTR, ___entry, d0, ULONG, ___stack, d1, IPTR, ___data, d2, struct Library *, ___dos, a3,\
      , DOPUS_BASE_NAME)

#define OpenImage(___name, ___info) \
      LP2(0x276, APTR, OpenImage , char *, ___name, a0, OpenImageInfo *, ___info, a1,\
      , DOPUS_BASE_NAME)

#define CloseImage(___image) \
      LP1NR(0x27c, CloseImage , APTR, ___image, a0,\
      , DOPUS_BASE_NAME)

#define CopyImage(___image) \
      LP1(0x282, APTR, CopyImage , APTR, ___image, a0,\
      , DOPUS_BASE_NAME)

#define FlushImages() \
      LP0NR(0x288, FlushImages ,\
      , DOPUS_BASE_NAME)

#define RenderImage(___rp, ___image, ___left, ___top, ___tags) \
      LP5(0x28e, short, RenderImage , struct RastPort *, ___rp, a0, APTR, ___image, a1, UWORD, ___left, d0, UWORD, ___top, d1, struct TagItem *, ___tags, a2,\
      , DOPUS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define RenderImageTags(___rp, ___image, ___left, ___top, ___tags, ...) \
    ({_sfdc_vararg _tags[] = { ___tags, __VA_ARGS__ }; RenderImage((___rp), (___image), (___left), (___top), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define GetImageAttrs(___image, ___tags) \
      LP2NR(0x294, GetImageAttrs , APTR, ___image, a0, struct TagItem *, ___tags, a1,\
      , DOPUS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define GetImageAttrsTags(___image, ___tags, ...) \
    ({_sfdc_vararg _tags[] = { ___tags, __VA_ARGS__ }; GetImageAttrs((___image), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define NewMemHandle(___puddle, ___thresh, ___type) \
      LP3(0x29a, void *, NewMemHandle , ULONG, ___puddle, d0, ULONG, ___thresh, d1, ULONG, ___type, d2,\
      , DOPUS_BASE_NAME)

#define FreeMemHandle(___handle) \
      LP1NR(0x2a0, FreeMemHandle , void *, ___handle, a0,\
      , DOPUS_BASE_NAME)

#define ClearMemHandle(___handle) \
      LP1NR(0x2a6, ClearMemHandle , void *, ___handle, a0,\
      , DOPUS_BASE_NAME)

#define AllocMemH(___handle, ___size) \
      LP2(0x2ac, void *, AllocMemH , void *, ___handle, a0, ULONG, ___size, d0,\
      , DOPUS_BASE_NAME)

#define FreeMemH(___memory) \
      LP1NR(0x2b2, FreeMemH , void *, ___memory, a0,\
      , DOPUS_BASE_NAME)

#define DrawBox(___rp, ___rect, ___info, ___recess) \
      LP4NR(0x2b8, DrawBox , struct RastPort *, ___rp, a0, struct Rectangle *, ___rect, a1, struct DrawInfo *, ___info, a2, BOOL, ___recess, d0,\
      , DOPUS_BASE_NAME)

#define DrawFieldBox(___rp, ___rect, ___info) \
      LP3NR(0x2be, DrawFieldBox , struct RastPort *, ___rp, a0, struct Rectangle *, ___rect, a1, struct DrawInfo *, ___info, a2,\
      , DOPUS_BASE_NAME)

#define NewLister(___path) \
      LP1(0x2c4, Cfg_Lister *, NewLister , char *, ___path, a0,\
      , DOPUS_BASE_NAME)

#define NewButtonBank(___init, ___type) \
      LP2(0x2ca, Cfg_ButtonBank *, NewButtonBank , BOOL, ___init, d0, short, ___type, d1,\
      , DOPUS_BASE_NAME)

#define NewButton(___memory) \
      LP1(0x2d0, Cfg_Button *, NewButton , APTR, ___memory, a0,\
      , DOPUS_BASE_NAME)

#define NewFunction(___memory, ___type) \
      LP2(0x2d6, Cfg_Function *, NewFunction , APTR, ___memory, a0, UWORD, ___type, d0,\
      , DOPUS_BASE_NAME)

#define NewInstruction(___memory, ___type, ___string) \
      LP3(0x2dc, Cfg_Instruction *, NewInstruction , APTR, ___memory, a0, short, ___type, d0, char *, ___string, a1,\
      , DOPUS_BASE_NAME)

#define ReadSettings(___env, ___name) \
      LP2(0x2e2, short, ReadSettings , CFG_SETS *, ___env, a0, char *, ___name, a1,\
      , DOPUS_BASE_NAME)

#define ReadListerDef(___iff, ___id) \
      LP2(0x2e8, Cfg_Lister *, ReadListerDef , APTR, ___iff, a0, ULONG, ___id, d0,\
      , DOPUS_BASE_NAME)

#define OpenButtonBank(___name) \
      LP1(0x2ee, Cfg_ButtonBank *, OpenButtonBank , char *, ___name, a0,\
      , DOPUS_BASE_NAME)

#define DefaultSettings(___settings) \
      LP1NR(0x2f4, DefaultSettings , CFG_SETS *, ___settings, a0,\
      , DOPUS_BASE_NAME)

#define DefaultEnvironment(___env) \
      LP1NR(0x2fa, DefaultEnvironment , CFG_ENVR *, ___env, a0,\
      , DOPUS_BASE_NAME)

#define DefaultButtonBank() \
      LP0(0x300, Cfg_ButtonBank *, DefaultButtonBank ,\
      , DOPUS_BASE_NAME)

#define SaveSettings(___env, ___name) \
      LP2(0x306, int, SaveSettings , CFG_SETS *, ___env, a0, char *, ___name, a1,\
      , DOPUS_BASE_NAME)

#define SaveListerDef(___iff, ___lister) \
      LP2(0x30c, long, SaveListerDef , APTR, ___iff, a0, Cfg_Lister *, ___lister, a1,\
      , DOPUS_BASE_NAME)

#define SaveButtonBank(___bank, ___name) \
      LP2(0x312, short, SaveButtonBank , Cfg_ButtonBank *, ___bank, a0, char *, ___name, a1,\
      , DOPUS_BASE_NAME)

#define CloseButtonBank(___bank) \
      LP1NR(0x318, CloseButtonBank , Cfg_ButtonBank *, ___bank, a0,\
      , DOPUS_BASE_NAME)

#define FreeListerDef(___lister) \
      LP1NR(0x31e, FreeListerDef , Cfg_Lister *, ___lister, a0,\
      , DOPUS_BASE_NAME)

#define FreeButtonList(___list) \
      LP1NR(0x324, FreeButtonList , struct List *, ___list, a0,\
      , DOPUS_BASE_NAME)

#define FreeButtonImages(___list) \
      LP1NR(0x32a, FreeButtonImages , struct List *, ___list, a0,\
      , DOPUS_BASE_NAME)

#define FreeButton(___button) \
      LP1NR(0x330, FreeButton , Cfg_Button *, ___button, a0,\
      , DOPUS_BASE_NAME)

#define FreeFunction(___function) \
      LP1NR(0x336, FreeFunction , Cfg_Function *, ___function, a0,\
      , DOPUS_BASE_NAME)

#define FreeInstruction(___ins) \
      LP1NR(0x33c, FreeInstruction , Cfg_Instruction *, ___ins, a0,\
      , DOPUS_BASE_NAME)

#define FreeInstructionList(___func) \
      LP1NR(0x342, FreeInstructionList , Cfg_Function *, ___func, a0,\
      , DOPUS_BASE_NAME)

#define CopyButtonBank(___bank) \
      LP1(0x348, Cfg_ButtonBank *, CopyButtonBank , Cfg_ButtonBank *, ___bank, a0,\
      , DOPUS_BASE_NAME)

#define CopyButton(___button, ___memory, ___type) \
      LP3(0x34e, Cfg_Button *, CopyButton , Cfg_Button *, ___button, a0, APTR, ___memory, a1, short, ___type, d0,\
      , DOPUS_BASE_NAME)

#define CopyFunction(___function, ___memory, ___newfunc) \
      LP3(0x354, Cfg_Function *, CopyFunction , Cfg_Function *, ___function, a0, APTR, ___memory, a1, Cfg_Function *, ___newfunc, a2,\
      , DOPUS_BASE_NAME)

#define NewFiletype(___memory) \
      LP1(0x35a, Cfg_Filetype *, NewFiletype , APTR, ___memory, a0,\
      , DOPUS_BASE_NAME)

#define ReadFiletypes(___name, ___memory) \
      LP2(0x360, Cfg_FiletypeList *, ReadFiletypes , char *, ___name, a0, APTR, ___memory, a1,\
      , DOPUS_BASE_NAME)

#define SaveFiletypeList(___list, ___name) \
      LP2(0x366, int, SaveFiletypeList , Cfg_FiletypeList *, ___list, a0, char *, ___name, a1,\
      , DOPUS_BASE_NAME)

#define FreeFiletypeList(___list) \
      LP1NR(0x36c, FreeFiletypeList , Cfg_FiletypeList *, ___list, a0,\
      , DOPUS_BASE_NAME)

#define FreeFiletype(___type) \
      LP1NR(0x372, FreeFiletype , Cfg_Filetype *, ___type, a0,\
      , DOPUS_BASE_NAME)

#define CopyFiletype(___orig, ___memory) \
      LP2(0x378, Cfg_Filetype *, CopyFiletype , Cfg_Filetype *, ___orig, a0, APTR, ___memory, a1,\
      , DOPUS_BASE_NAME)

#define FindFunctionType(___list, ___type) \
      LP2(0x37e, Cfg_Function *, FindFunctionType , struct List *, ___list, a0, UWORD, ___type, d0,\
      , DOPUS_BASE_NAME)

#define SaveButton(___iff, ___button) \
      LP2(0x384, short, SaveButton , APTR, ___iff, a0, Cfg_Button *, ___button, a1,\
      , DOPUS_BASE_NAME)

#define ReadButton(___iff, ___memory) \
      LP2(0x38a, Cfg_Button *, ReadButton , APTR, ___iff, a0, APTR, ___memory, a1,\
      , DOPUS_BASE_NAME)

#define ReadFunction(___iff, ___memory, ___funclist, ___function) \
      LP4(0x390, Cfg_Function *, ReadFunction , APTR, ___iff, a0, APTR, ___memory, a1, struct List *, ___funclist, a2, Cfg_Function *, ___function, a3,\
      , DOPUS_BASE_NAME)

#define OpenIFFFile(___name, ___mode, ___id) \
      LP3(0x396, struct IFFHandle *, OpenIFFFile , char *, ___name, a0, int, ___mode, d0, ULONG, ___id, d1,\
      , DOPUS_BASE_NAME)

#define CloseIFFFile(___iff) \
      LP1NR(0x39c, CloseIFFFile , struct IFFHandle *, ___iff, a0,\
      , DOPUS_BASE_NAME)

#define OpenStatusWindow(___title, ___text, ___screen, ___graph, ___flags) \
      LP5(0x3a2, struct Window *, OpenStatusWindow , char *, ___title, a0, char *, ___text, a1, struct Screen *, ___screen, a2, LONG, ___graph, d1, ULONG, ___flags, d0,\
      , DOPUS_BASE_NAME)

#define SetStatusText(___window, ___text) \
      LP2NR(0x3a8, SetStatusText , struct Window *, ___window, a0, char *, ___text, a1,\
      , DOPUS_BASE_NAME)

#define UpdateStatusGraph(___window, ___text, ___total, ___count) \
      LP4NR(0x3ae, UpdateStatusGraph , struct Window *, ___window, a0, char *, ___text, a1, ULONG, ___total, d0, ULONG, ___count, d1,\
      , DOPUS_BASE_NAME)

#define ReadILBM(___name, ___flags) \
      LP2(0x3b4, ILBMHandle *, ReadILBM , char *, ___name, a0, ULONG, ___flags, d0,\
      , DOPUS_BASE_NAME)

#define FreeILBM(___ilbm) \
      LP1NR(0x3ba, FreeILBM , ILBMHandle *, ___ilbm, a0,\
      , DOPUS_BASE_NAME)

#define DecodeILBM(___src, ___w, ___h, ___d, ___dst, ___mask, ___comp) \
      LP7NR(0x3c0, DecodeILBM , char *, ___src, a0, UWORD, ___w, d0, UWORD, ___h, d1, UWORD, ___d, d2, struct BitMap *, ___dst, a1, ULONG, ___mask, d3, char, ___comp, d4,\
      , DOPUS_BASE_NAME)

#define DecodeRLE(___rleinfo) \
      LP1NR(0x3c6, DecodeRLE , RLEinfo *, ___rleinfo, a0,\
      , DOPUS_BASE_NAME)

#define LoadPalette32(___vp, ___palette) \
      LP2NR(0x3cc, LoadPalette32 , struct ViewPort *, ___vp, a0, ULONG *, ___palette, a1,\
      , DOPUS_BASE_NAME)

#define GetPalette32(___vp, ___palette, ___count, ___first) \
      LP4NR(0x3d2, GetPalette32 , struct ViewPort *, ___vp, a0, ULONG *, ___palette, a1, UWORD, ___count, d0, short, ___first, d1,\
      , DOPUS_BASE_NAME)

#define OpenBuf(___name, ___mode, ___buffer_size) \
      LP3(0x3d8, APTR, OpenBuf , char *, ___name, a0, long, ___mode, d0, long, ___buffer_size, d1,\
      , DOPUS_BASE_NAME)

#define CloseBuf(___file) \
      LP1(0x3de, long, CloseBuf , APTR, ___file, a0,\
      , DOPUS_BASE_NAME)

#define ReadBuf(___file, ___data, ___size) \
      LP3(0x3e4, long, ReadBuf , APTR, ___file, a0, char *, ___data, a1, long, ___size, d0,\
      , DOPUS_BASE_NAME)

#define WriteBuf(___file, ___data, ___size) \
      LP3(0x3ea, long, WriteBuf , APTR, ___file, a0, const char *, ___data, a1, long, ___size, d0,\
      , DOPUS_BASE_NAME)

#define FlushBuf(___file) \
      LP1(0x3f0, long, FlushBuf , APTR, ___file, a0,\
      , DOPUS_BASE_NAME)

#define SeekBuf(___file, ___offset, ___mode) \
      LP3(0x3f6, long, SeekBuf , APTR, ___file, a0, long, ___offset, d0, long, ___mode, d1,\
      , DOPUS_BASE_NAME)

#define ExamineBuf(___file, ___fib) \
      LP2(0x3fc, long, ExamineBuf , APTR, ___file, a0, struct FileInfoBlock *, ___fib, a1,\
      , DOPUS_BASE_NAME)

#define OpenDisk(___disk, ___port) \
      LP2(0x402, DiskHandle *, OpenDisk , char *, ___disk, a0, struct MsgPort *, ___port, a1,\
      , DOPUS_BASE_NAME)

#define CloseDisk(___handle) \
      LP1NR(0x408, CloseDisk , DiskHandle *, ___handle, a0,\
      , DOPUS_BASE_NAME)

#define AddScrollBars(___win, ___list, ___drawinfo, ___noidcmp) \
      LP4(0x40e, struct Gadget *, AddScrollBars , struct Window *, ___win, a0, struct List *, ___list, a1, struct DrawInfo *, ___drawinfo, a2, short, ___noidcmp, d0,\
      , DOPUS_BASE_NAME)

#define FindBOOPSIGadget(___list, ___id) \
      LP2(0x414, struct Gadget *, FindBOOPSIGadget , struct List *, ___list, a0, UWORD, ___id, d0,\
      , DOPUS_BASE_NAME)

#define BOOPSIFree(___list) \
      LP1NR(0x41a, BOOPSIFree , struct List *, ___list, a0,\
      , DOPUS_BASE_NAME)

#define SerialValid(___data) \
      LP1(0x420, BOOL, SerialValid , serial_data *, ___data, a0,\
      , DOPUS_BASE_NAME)

#define WB_Install_Patch() \
      LP0NR(0x426, WB_Install_Patch ,\
      , DOPUS_BASE_NAME)

#define WB_Remove_Patch() \
      LP0(0x42c, BOOL, WB_Remove_Patch ,\
      , DOPUS_BASE_NAME)

#define WB_AddAppWindow(___id, ___data, ___window, ___port, ___tags) \
      LP5(0x432, struct AppWindow *, WB_AddAppWindow , IPTR, ___id, d0, IPTR, ___data, d1, struct Window *, ___window, a0, struct MsgPort *, ___port, a1, struct TagItem *, ___tags, a2,\
      , DOPUS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define WB_AddAppWindowTags(___id, ___data, ___window, ___port, ___tags, ...) \
    ({_sfdc_vararg _tags[] = { ___tags, __VA_ARGS__ }; WB_AddAppWindow((___id), (___data), (___window), (___port), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define WB_RemoveAppWindow(___window) \
      LP1(0x438, BOOL, WB_RemoveAppWindow , struct AppWindow *, ___window, a0,\
      , DOPUS_BASE_NAME)

#define WB_FindAppWindow(___window) \
      LP1(0x43e, struct AppWindow *, WB_FindAppWindow , struct Window *, ___window, a0,\
      , DOPUS_BASE_NAME)

#define WB_AppWindowData(___window, ___id, ___userdata) \
      LP3(0x444, struct MsgPort *, WB_AppWindowData , struct AppWindow *, ___window, a0, IPTR *, ___id, a1, IPTR *, ___userdata, a2,\
      , DOPUS_BASE_NAME)

#define WB_AppWindowLocal(___window) \
      LP1(0x44a, BOOL, WB_AppWindowLocal , struct AppWindow *, ___window, a0,\
      , DOPUS_BASE_NAME)

#define LockAppList() \
      LP0(0x450, APTR, LockAppList ,\
      , DOPUS_BASE_NAME)

#define NextAppEntry(___last, ___type) \
      LP2(0x456, APTR, NextAppEntry , APTR, ___last, a0, ULONG, ___type, d0,\
      , DOPUS_BASE_NAME)

#define UnlockAppList() \
      LP0NR(0x45c, UnlockAppList ,\
      , DOPUS_BASE_NAME)

#define AddNotifyRequest(___type, ___data, ___port) \
      LP3(0x462, APTR, AddNotifyRequest , ULONG, ___type, d0, ULONG, ___data, d1, struct MsgPort *, ___port, a0,\
      , DOPUS_BASE_NAME)

#define RemoveNotifyRequest(___node) \
      LP1NR(0x468, RemoveNotifyRequest , APTR, ___node, a0,\
      , DOPUS_BASE_NAME)

#define SendNotifyMsg(___type, ___data, ___flags, ___wait, ___name, ___fib) \
      LP6NR(0x46e, SendNotifyMsg , ULONG, ___type, d0, IPTR, ___data, d1, ULONG, ___flags, d2, short, ___wait, d3, char *, ___name, a0, struct FileInfoBlock *, ___fib, a1,\
      , DOPUS_BASE_NAME)

#define StrCombine(___s1, ___s2, ___s3, ___len) \
      LP4(0x474, BOOL, StrCombine , char *, ___s1, a0, char *, ___s2, a1, char *, ___s3, a2, int, ___len, d0,\
      , DOPUS_BASE_NAME)

#define StrConcat(___s1, ___s2, ___len) \
      LP3(0x47a, BOOL, StrConcat , char *, ___s1, a0, char *, ___s2, a1, int, ___len, d0,\
      , DOPUS_BASE_NAME)

#define WB_Launch(___name, ___screen, ___wait) \
      LP3(0x480, BOOL, WB_Launch , char *, ___name, a0, struct Screen *, ___screen, a1, short, ___wait, d0,\
      , DOPUS_BASE_NAME)

#define CopyLocalEnv(___base) \
      LP1NR(0x486, CopyLocalEnv , struct Library *, ___base, a0,\
      , DOPUS_BASE_NAME)

#define CLI_Launch(___name, ___screen, ___cd, ___in, ___out, ___wait, ___stack) \
      LP7(0x48c, BOOL, CLI_Launch , char *, ___name, a0, struct Screen *, ___screen, a1, BPTR, ___cd, d0, BPTR, ___in, d1, BPTR, ___out, d2, short, ___wait, d3, long, ___stack, d4,\
      , DOPUS_BASE_NAME)

#define SerialCheck(___num, ___p) \
      LP2(0x492, BOOL, SerialCheck , char *, ___num, a0, ULONG *, ___p, a1,\
      , DOPUS_BASE_NAME)

#define ChecksumFile(___file, ___skip) \
      LP2(0x498, ULONG, ChecksumFile , char *, ___file, a0, ULONG, ___skip, d0,\
      , DOPUS_BASE_NAME)

#define ReplyFreeMsg(___msg) \
      LP1NR(0x49e, ReplyFreeMsg , APTR, ___msg, a0,\
      , DOPUS_BASE_NAME)

#define TimerActive(___timer) \
      LP1(0x4a4, BOOL, TimerActive , TimerHandle *, ___timer, a0,\
      , DOPUS_BASE_NAME)

#define NewButtonFunction(___memory, ___type) \
      LP2(0x4aa, Cfg_ButtonFunction *, NewButtonFunction , APTR, ___memory, a0, UWORD, ___type, d0,\
      , DOPUS_BASE_NAME)

#define IFFOpen(___name, ___mode, ___form) \
      LP3(0x4b0, APTR, IFFOpen , char *, ___name, a0, UWORD, ___mode, d0, ULONG, ___form, d1,\
      , DOPUS_BASE_NAME)

#define IFFClose(___handle) \
      LP1NR(0x4b6, IFFClose , APTR, ___handle, a0,\
      , DOPUS_BASE_NAME)

#define IFFPushChunk(___handle, ___id) \
      LP2(0x4bc, long, IFFPushChunk , APTR, ___handle, a0, ULONG, ___id, d0,\
      , DOPUS_BASE_NAME)

#define IFFWriteChunkBytes(___handle, ___data, ___size) \
      LP3(0x4c2, long, IFFWriteChunkBytes , APTR, ___handle, a0, APTR, ___data, a1, long, ___size, d0,\
      , DOPUS_BASE_NAME)

#define IFFPopChunk(___handle) \
      LP1(0x4c8, long, IFFPopChunk , APTR, ___handle, a0,\
      , DOPUS_BASE_NAME)

#define IFFWriteChunk(___handle, ___data, ___chunk, ___size) \
      LP4(0x4ce, long, IFFWriteChunk , APTR, ___handle, a0, APTR, ___data, a1, ULONG, ___chunk, d0, ULONG, ___size, d1,\
      , DOPUS_BASE_NAME)

#define FindNameI(___list, ___name) \
      LP2(0x4d4, struct Node *, FindNameI , struct List *, ___list, a0, char *, ___name, a1,\
      , DOPUS_BASE_NAME)

#define AnimDecodeRIFFXor(___delta, ___plane, ___rowbytes, ___sourcebytes) \
      LP4NR(0x4da, AnimDecodeRIFFXor , unsigned char *, ___delta, a0, char *, ___plane, a1, UWORD, ___rowbytes, d0, UWORD, ___sourcebytes, d1,\
      , DOPUS_BASE_NAME)

#define AnimDecodeRIFFSet(___delta, ___plane, ___rowbytes, ___sourcebytes) \
      LP4NR(0x4e0, AnimDecodeRIFFSet , unsigned char *, ___delta, a0, char *, ___plane, a1, UWORD, ___rowbytes, d0, UWORD, ___sourcebytes, d1,\
      , DOPUS_BASE_NAME)

#define ConvertRawKey(___code, ___qual, ___key) \
      LP3(0x4e6, BOOL, ConvertRawKey , UWORD, ___code, d0, UWORD, ___qual, d1, char *, ___key, a0,\
      , DOPUS_BASE_NAME)

#define OpenClipBoard(___unit) \
      LP1(0x4ec, struct ClipHandle *, OpenClipBoard , ULONG, ___unit, d0,\
      , DOPUS_BASE_NAME)

#define CloseClipBoard(___clip) \
      LP1NR(0x4f2, CloseClipBoard , struct ClipHandle *, ___clip, a0,\
      , DOPUS_BASE_NAME)

#define WriteClipString(___clip, ___string, ___len) \
      LP3(0x4f8, BOOL, WriteClipString , struct ClipHandle *, ___clip, a0, char *, ___string, a1, long, ___len, d0,\
      , DOPUS_BASE_NAME)

#define ReadClipString(___clip, ___string, ___len) \
      LP3(0x4fe, long, ReadClipString , struct ClipHandle *, ___clip, a0, char *, ___string, a1, long, ___len, d0,\
      , DOPUS_BASE_NAME)

#define LockAttList(___list, ___exclusive) \
      LP2NR(0x504, LockAttList , Att_List *, ___list, a0, short, ___exclusive, d0,\
      , DOPUS_BASE_NAME)

#define UnlockAttList(___list) \
      LP1NR(0x50a, UnlockAttList , Att_List *, ___list, a0,\
      , DOPUS_BASE_NAME)

#define RemovedFunc1() \
      LP0NR(0x510, RemovedFunc1 ,\
      , DOPUS_BASE_NAME)

#define RemovedFunc2() \
      LP0NR(0x516, RemovedFunc2 ,\
      , DOPUS_BASE_NAME)

#define RemovedFunc3() \
      LP0NR(0x51c, RemovedFunc3 ,\
      , DOPUS_BASE_NAME)

#define RemovedFunc4() \
      LP0NR(0x522, RemovedFunc4 ,\
      , DOPUS_BASE_NAME)

#define RemovedFunc5() \
      LP0NR(0x528, RemovedFunc5 ,\
      , DOPUS_BASE_NAME)

#define RemovedFunc6() \
      LP0NR(0x52e, RemovedFunc6 ,\
      , DOPUS_BASE_NAME)

#define RemovedFunc7() \
      LP0NR(0x534, RemovedFunc7 ,\
      , DOPUS_BASE_NAME)

#define GetSemaphore(___sem, ___exc, ___data) \
      LP3(0x53a, long, GetSemaphore , struct SignalSemaphore *, ___sem, a0, long, ___exc, d0, char *, ___data, a1,\
      , DOPUS_BASE_NAME)

#define FreeSemaphore(___sem) \
      LP1NR(0x540, FreeSemaphore , struct SignalSemaphore *, ___sem, a0,\
      , DOPUS_BASE_NAME)

#define ShowSemaphore(___sem) \
      LP1NR(0x546, ShowSemaphore , struct SignalSemaphore *, ___sem, a0,\
      , DOPUS_BASE_NAME)

#define SaveFunction(___iff, ___func) \
      LP2(0x54c, BOOL, SaveFunction , APTR, ___iff, a0, Cfg_Function *, ___func, a1,\
      , DOPUS_BASE_NAME)

#define IFFNextChunk(___iff, ___form) \
      LP2(0x552, ULONG, IFFNextChunk , APTR, ___iff, a0, ULONG, ___form, d0,\
      , DOPUS_BASE_NAME)

#define IFFChunkSize(___iff) \
      LP1(0x558, long, IFFChunkSize , APTR, ___iff, a0,\
      , DOPUS_BASE_NAME)

#define IFFReadChunkBytes(___iff, ___buffer, ___size) \
      LP3(0x55e, long, IFFReadChunkBytes , APTR, ___iff, a0, APTR, ___buffer, a1, long, ___size, d0,\
      , DOPUS_BASE_NAME)

#define IFFFileHandle(___iff) \
      LP1(0x564, APTR, IFFFileHandle , APTR, ___iff, a0,\
      , DOPUS_BASE_NAME)

#define IFFChunkRemain(___iff) \
      LP1(0x56a, long, IFFChunkRemain , APTR, ___iff, a0,\
      , DOPUS_BASE_NAME)

#define IFFChunkID(___iff) \
      LP1(0x570, ULONG, IFFChunkID , APTR, ___iff, a0,\
      , DOPUS_BASE_NAME)

#define IFFGetFORM(___iff) \
      LP1(0x576, ULONG, IFFGetFORM , APTR, ___iff, a0,\
      , DOPUS_BASE_NAME)

#define ScreenInfo(___screen) \
      LP1(0x57c, ULONG, ScreenInfo , struct Screen *, ___screen, a0,\
      , DOPUS_BASE_NAME)

#define GetEditHook(___type, ___flags, ___tags) \
      LP3(0x582, struct Hook *, GetEditHook , ULONG, ___type, d0, ULONG, ___flags, d1, struct TagItem *, ___tags, a0,\
      , DOPUS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define GetEditHookTags(___type, ___flags, ___tags, ...) \
    ({_sfdc_vararg _tags[] = { ___tags, __VA_ARGS__ }; GetEditHook((___type), (___flags), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define FreeEditHook(___hook) \
      LP1NR(0x588, FreeEditHook , struct Hook *, ___hook, a0,\
      , DOPUS_BASE_NAME)

#define InitWindowDims(___window, ___dims) \
      LP2NR(0x58e, InitWindowDims , struct Window *, ___window, a0, WindowDimensions *, ___dims, a1,\
      , DOPUS_BASE_NAME)

#define StoreWindowDims(___window, ___dims) \
      LP2NR(0x594, StoreWindowDims , struct Window *, ___window, a0, WindowDimensions *, ___dims, a1,\
      , DOPUS_BASE_NAME)

#define CheckWindowDims(___window, ___dims) \
      LP2(0x59a, BOOL, CheckWindowDims , struct Window *, ___window, a0, WindowDimensions *, ___dims, a1,\
      , DOPUS_BASE_NAME)

#define InitListLock(___ll, ___name) \
      LP2NR(0x5a0, InitListLock , struct ListLock *, ___ll, a0, char *, ___name, a1,\
      , DOPUS_BASE_NAME)

#define IPC_QuitName(___list, ___name, ___flags) \
      LP3NR(0x5a6, IPC_QuitName , struct ListLock *, ___list, a0, char *, ___name, a1, ULONG, ___flags, d0,\
      , DOPUS_BASE_NAME)

#define QualValid(___qual) \
      LP1(0x5ac, UWORD, QualValid , UWORD, ___qual, d0,\
      , DOPUS_BASE_NAME)

#define FHFromBuf(___file) \
      LP1(0x5b2, BPTR, FHFromBuf , APTR, ___file, a0,\
      , DOPUS_BASE_NAME)

#define WB_AppIconFlags(___icon) \
      LP1(0x5b8, ULONG, WB_AppIconFlags , struct AppIcon *, ___icon, a0,\
      , DOPUS_BASE_NAME)

#define GetWBArgPath(___arg, ___buf, ___size) \
      LP3(0x5be, BOOL, GetWBArgPath , struct WBArg *, ___arg, a0, char *, ___buf, a1, long, ___size, d0,\
      , DOPUS_BASE_NAME)

#define RemovedFunc8() \
      LP0NR(0x5c4, RemovedFunc8 ,\
      , DOPUS_BASE_NAME)

#define DeviceFromLock(___lock, ___name) \
      LP2(0x5ca, struct DosList *, DeviceFromLock , BPTR, ___lock, a0, char *, ___name, a1,\
      , DOPUS_BASE_NAME)

#define DeviceFromHandler(___port, ___name) \
      LP2(0x5d0, struct DosList *, DeviceFromHandler , struct MsgPort *, ___port, a0, char *, ___name, a1,\
      , DOPUS_BASE_NAME)

#define DevNameFromLockDopus(___lock, ___buffer, ___len) \
      LP3(0x5d6, BOOL, DevNameFromLockDopus , BPTR, ___lock, d1, char *, ___buffer, d2, long, ___len, d3,\
      , DOPUS_BASE_NAME)

#define GetIconFlags(___icon) \
      LP1(0x5dc, ULONG, GetIconFlags , struct DiskObject *, ___icon, a0,\
      , DOPUS_BASE_NAME)

#define SetIconFlags(___icon, ___flags) \
      LP2NR(0x5e2, SetIconFlags , struct DiskObject *, ___icon, a0, ULONG, ___flags, d0,\
      , DOPUS_BASE_NAME)

#define GetIconPosition(___icon, ___x, ___y) \
      LP3NR(0x5e8, GetIconPosition , struct DiskObject *, ___icon, a0, short *, ___x, a1, short *, ___y, a2,\
      , DOPUS_BASE_NAME)

#define SetIconPosition(___icon, ___x, ___y) \
      LP3NR(0x5ee, SetIconPosition , struct DiskObject *, ___icon, a0, short, ___x, d0, short, ___y, d1,\
      , DOPUS_BASE_NAME)

#define BuildTransDragMask(___mask, ___image, ___width, ___height, ___depth, ___flags) \
      LP6(0x5f4, BOOL, BuildTransDragMask , UWORD *, ___mask, a0, UWORD *, ___image, a1, short, ___width, d0, short, ___height, d1, short, ___depth, d2, long, ___flags, d3,\
      , DOPUS_BASE_NAME)

#define GetImagePalette(___image) \
      LP1(0x5fa, ULONG *, GetImagePalette , APTR, ___image, a0,\
      , DOPUS_BASE_NAME)

#define FreeImageRemap(___remap) \
      LP1NR(0x600, FreeImageRemap , ImageRemap *, ___remap, a0,\
      , DOPUS_BASE_NAME)

#define SwapListNodes(___list, ___s1, ___s2) \
      LP3NR(0x606, SwapListNodes , struct List *, ___list, a0, struct Node *, ___s1, a1, struct Node *, ___s2, a2,\
      , DOPUS_BASE_NAME)

#define RemovedFunc9() \
      LP0NR(0x60c, RemovedFunc9 ,\
      , DOPUS_BASE_NAME)

#define Seed(___seed) \
      LP1NR(0x612, Seed , int, ___seed, d0,\
      , DOPUS_BASE_NAME)

#define RemovedFunc10() \
      LP0NR(0x618, RemovedFunc10 ,\
      , DOPUS_BASE_NAME)

#define RemovedFunc11() \
      LP0NR(0x61e, RemovedFunc11 ,\
      , DOPUS_BASE_NAME)

#define RemovedFunc12() \
      LP0NR(0x624, RemovedFunc12 ,\
      , DOPUS_BASE_NAME)

#define CopyDiskObject(___icon, ___flags) \
      LP2(0x62a, struct DiskObject *, CopyDiskObject , struct DiskObject *, ___icon, a0, ULONG, ___flags, d0,\
      , DOPUS_BASE_NAME)

#define FreeDiskObjectCopy(___icon) \
      LP1NR(0x630, FreeDiskObjectCopy , struct DiskObject *, ___icon, a0,\
      , DOPUS_BASE_NAME)

#define IFFFailure(___iff) \
      LP1NR(0x636, IFFFailure , APTR, ___iff, a0,\
      , DOPUS_BASE_NAME)

#define GetCachedDefDiskObject(___type) \
      LP1(0x63c, struct DiskObject *, GetCachedDefDiskObject , long, ___type, d0,\
      , DOPUS_BASE_NAME)

#define FreeCachedDiskObject(___icon) \
      LP1NR(0x642, FreeCachedDiskObject , struct DiskObject *, ___icon, a0,\
      , DOPUS_BASE_NAME)

#define GetCachedDiskObject(___name, ___flags) \
      LP2(0x648, struct DiskObject *, GetCachedDiskObject , char *, ___name, a0, long, ___flags, d0,\
      , DOPUS_BASE_NAME)

#define GetCachedDiskObjectNew(___name, ___flags) \
      LP2(0x64e, struct DiskObject *, GetCachedDiskObjectNew , char *, ___name, a0, ULONG, ___flags, d0,\
      , DOPUS_BASE_NAME)

#define IconCheckSum(___icon, ___which) \
      LP2(0x654, ULONG, IconCheckSum , struct DiskObject *, ___icon, a0, short, ___which, d0,\
      , DOPUS_BASE_NAME)

#define OpenProgressWindow(___tags) \
      LP1(0x65a, APTR, OpenProgressWindow , struct TagItem *, ___tags, a0,\
      , DOPUS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define OpenProgressWindowTags(___tags, ...) \
    ({_sfdc_vararg _tags[] = { ___tags, __VA_ARGS__ }; OpenProgressWindow((struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CloseProgressWindow(___win) \
      LP1NR(0x660, CloseProgressWindow , APTR, ___win, a0,\
      , DOPUS_BASE_NAME)

#define HideProgressWindow(___win) \
      LP1NR(0x666, HideProgressWindow , APTR, ___win, a0,\
      , DOPUS_BASE_NAME)

#define ShowProgressWindow(___prog, ___scr, ___win) \
      LP3NR(0x66c, ShowProgressWindow , APTR, ___prog, a0, struct Screen *, ___scr, a1, struct Window *, ___win, a2,\
      , DOPUS_BASE_NAME)

#define SetProgressWindow(___win, ___tags) \
      LP2NR(0x672, SetProgressWindow , APTR, ___win, a0, struct TagItem *, ___tags, a1,\
      , DOPUS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define SetProgressWindowTags(___win, ___tags, ...) \
    ({_sfdc_vararg _tags[] = { ___tags, __VA_ARGS__ }; SetProgressWindow((___win), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define GetProgressWindow(___win, ___tags) \
      LP2NR(0x678, GetProgressWindow , APTR, ___win, a0, struct TagItem *, ___tags, a1,\
      , DOPUS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define GetProgressWindowTags(___win, ___tags, ...) \
    ({_sfdc_vararg _tags[] = { ___tags, __VA_ARGS__ }; GetProgressWindow((___win), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define SetNotifyRequest(___req, ___flags, ___mask) \
      LP3NR(0x67e, SetNotifyRequest , APTR, ___req, a0, ULONG, ___flags, d0, ULONG, ___mask, d1,\
      , DOPUS_BASE_NAME)

#define ChangeAppIcon(___icon, ___render, ___select, ___title, ___flags) \
      LP5NR(0x684, ChangeAppIcon , APTR, ___icon, a0, struct Image *, ___render, a1, struct Image *, ___select, a2, char *, ___title, a3, ULONG, ___flags, d0,\
      , DOPUS_BASE_NAME)

#define CheckProgressAbort(___win) \
      LP1(0x68a, BOOL, CheckProgressAbort , APTR, ___win, a0,\
      , DOPUS_BASE_NAME)

#define GetSecureString(___gad) \
      LP1(0x690, char *, GetSecureString , struct Gadget *, ___gad, a0,\
      , DOPUS_BASE_NAME)

#define NewButtonWithFunc(___mem, ___label, ___type) \
      LP3(0x696, Cfg_Button *, NewButtonWithFunc , APTR, ___mem, a0, char *, ___label, a1, short, ___type, d0,\
      , DOPUS_BASE_NAME)

#define FreeButtonFunction(___func) \
      LP1NR(0x69c, FreeButtonFunction , Cfg_ButtonFunction *, ___func, a0,\
      , DOPUS_BASE_NAME)

#define CopyButtonFunction(___function, ___memory, ___newfunc) \
      LP3(0x6a2, Cfg_ButtonFunction *, CopyButtonFunction , Cfg_ButtonFunction *, ___function, a0, APTR, ___memory, a1, Cfg_ButtonFunction *, ___newfunc, a2,\
      , DOPUS_BASE_NAME)

#define FindPubScreen(___scr, ___lock) \
      LP2(0x6a8, struct PubScreenNode *, FindPubScreen , struct Screen *, ___scr, a0, BOOL, ___lock, d0,\
      , DOPUS_BASE_NAME)

#define SetAppIconMenuState(___icon, ___item, ___state) \
      LP3(0x6ae, long, SetAppIconMenuState , APTR, ___icon, a0, long, ___item, d0, long, ___state, d1,\
      , DOPUS_BASE_NAME)

#define SearchFile(___file, ___text, ___flags, ___buffer, ___size) \
      LP5(0x6b4, long, SearchFile , APTR, ___file, a0, UBYTE *, ___text, a1, ULONG, ___flags, d0, UBYTE *, ___buffer, a2, ULONG, ___size, d1,\
      , DOPUS_BASE_NAME)

#define ParseDateStrings(___string, ___date, ___time, ___range) \
      LP4(0x6ba, char *, ParseDateStrings , char *, ___string, a0, char *, ___date, a1, char *, ___time, a2, long *, ___range, a3,\
      , DOPUS_BASE_NAME)

#define DateFromStrings(___date, ___time, ___ds) \
      LP3(0x6c0, BOOL, DateFromStrings , char *, ___date, a0, char *, ___time, a1, struct DateStamp *, ___ds, a2,\
      , DOPUS_BASE_NAME)

#define GetMatchHandle(___name) \
      LP1(0x6c6, APTR, GetMatchHandle , char *, ___name, a0,\
      , DOPUS_BASE_NAME)

#define FreeMatchHandle(___handle) \
      LP1NR(0x6cc, FreeMatchHandle , APTR, ___handle, a0,\
      , DOPUS_BASE_NAME)

#define MatchFiletype(___handle, ___type) \
      LP2(0x6d2, BOOL, MatchFiletype , APTR, ___handle, a0, APTR, ___type, a1,\
      , DOPUS_BASE_NAME)

#define LayoutResize(___window) \
      LP1NR(0x6d8, LayoutResize , struct Window *, ___window, a0,\
      , DOPUS_BASE_NAME)

#define GetFileVersion(___name, ___ver, ___rev, ___date, ___prog) \
      LP5(0x6de, BOOL, GetFileVersion , char *, ___name, a0, short *, ___ver, d0, short *, ___rev, d1, struct DateStamp *, ___date, a1, APTR, ___prog, a2,\
      , DOPUS_BASE_NAME)

#define AsyncRequest(___ipc, ___type, ___window, ___callback, ___data, ___tags) \
      LP6(0x6e4, long, AsyncRequest , IPCData *, ___ipc, a0, long, ___type, d0, struct Window *, ___window, a1, REF_CALLBACK, ___callback, a2, APTR, ___data, a3, struct TagItem *, ___tags, d1,\
      , DOPUS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define AsyncRequestTags(___ipc, ___type, ___window, ___callback, ___data, ___tags, ...) \
    ({_sfdc_vararg _tags[] = { ___tags, __VA_ARGS__ }; AsyncRequest((___ipc), (___type), (___window), (___callback), (___data), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define CheckRefreshMsg(___window, ___mask) \
      LP2(0x6ea, struct IntuiMessage *, CheckRefreshMsg , struct Window *, ___window, a0, ULONG, ___mask, d0,\
      , DOPUS_BASE_NAME)

#define RemapImage(___image, ___screen, ___remap) \
      LP3(0x6f0, BOOL, RemapImage , APTR, ___image, a0, struct Screen *, ___screen, a1, ImageRemap *, ___remap, a2,\
      , DOPUS_BASE_NAME)

#define FreeRemapImage(___image, ___remap) \
      LP2NR(0x6f6, FreeRemapImage , APTR, ___image, a0, ImageRemap *, ___remap, a1,\
      , DOPUS_BASE_NAME)

#define FreeAppMessage(___msg) \
      LP1NR(0x6fc, FreeAppMessage , DOpusAppMessage *, ___msg, a0,\
      , DOPUS_BASE_NAME)

#define ReplyAppMessage(___msg) \
      LP1NR(0x702, ReplyAppMessage , DOpusAppMessage *, ___msg, a0,\
      , DOPUS_BASE_NAME)

#define SetLibraryFlags(___flags, ___mask) \
      LP2(0x708, ULONG, SetLibraryFlags , ULONG, ___flags, d0, ULONG, ___mask, d1,\
      , DOPUS_BASE_NAME)

#define StartRefreshConfigWindow(___win, ___state) \
      LP2NR(0x70e, StartRefreshConfigWindow , struct Window *, ___win, a0, long, ___state, d0,\
      , DOPUS_BASE_NAME)

#define EndRefreshConfigWindow(___win) \
      LP1NR(0x714, EndRefreshConfigWindow , struct Window *, ___win, a0,\
      , DOPUS_BASE_NAME)

#define CompareListFormat(___f1, ___f2) \
      LP2(0x71a, ULONG, CompareListFormat , ListFormat *, ___f1, a0, ListFormat *, ___f2, a1,\
      , DOPUS_BASE_NAME)

#define UpdateGadgetValue(___list, ___msg, ___id) \
      LP3NR(0x720, UpdateGadgetValue , ObjectList *, ___list, a0, struct IntuiMessage *, ___msg, a1, UWORD, ___id, d0,\
      , DOPUS_BASE_NAME)

#define UpdateGadgetList(___list) \
      LP1NR(0x726, UpdateGadgetList , ObjectList *, ___list, a0,\
      , DOPUS_BASE_NAME)

#define NewBitMap(___w, ___h, ___d, ___f, ___b) \
      LP5(0x72c, struct BitMap *, NewBitMap , ULONG, ___w, d0, ULONG, ___h, d1, ULONG, ___d, d2, ULONG, ___f, d3, struct BitMap *, ___b, a0,\
      , DOPUS_BASE_NAME)

#define DisposeBitMap(___b) \
      LP1NR(0x732, DisposeBitMap , struct BitMap *, ___b, a0,\
      , DOPUS_BASE_NAME)

#define ParseArgs(___temp, ___args) \
      LP2(0x738, FuncArgs *, ParseArgs , char *, ___temp, a0, char *, ___args, a1,\
      , DOPUS_BASE_NAME)

#define DisposeArgs(___args) \
      LP1NR(0x73e, DisposeArgs , FuncArgs *, ___args, a0,\
      , DOPUS_BASE_NAME)

#define SetConfigWindowLimits(___window, ___min, ___max) \
      LP3NR(0x744, SetConfigWindowLimits , struct Window *, ___window, a0, const ConfigWindow *, ___min, a1, const ConfigWindow *, ___max, a2,\
      , DOPUS_BASE_NAME)

#define SetEnv(___name, ___data, ___save) \
      LP3NR(0x74a, SetEnv , char *, ___name, a0, char *, ___data, a1, BOOL, ___save, d0,\
      , DOPUS_BASE_NAME)

#define IsListLockEmpty(___list) \
      LP1(0x750, BOOL, IsListLockEmpty , struct ListLock *, ___list, a0,\
      , DOPUS_BASE_NAME)

#define AllocAppMessage(___mem, ___reply, ___num) \
      LP3(0x756, DOpusAppMessage *, AllocAppMessage , APTR, ___mem, a0, struct MsgPort *, ___reply, a1, short, ___num, d0,\
      , DOPUS_BASE_NAME)

#define CheckAppMessage(___msg) \
      LP1(0x75c, BOOL, CheckAppMessage , DOpusAppMessage *, ___msg, a0,\
      , DOPUS_BASE_NAME)

#define CopyAppMessage(___msg, ___mem) \
      LP2(0x762, DOpusAppMessage *, CopyAppMessage , DOpusAppMessage *, ___msg, a0, APTR, ___mem, a1,\
      , DOPUS_BASE_NAME)

#define SetWBArg(___msg, ___num, ___lock, ___name, ___mem) \
      LP5(0x768, BOOL, SetWBArg , DOpusAppMessage *, ___msg, a0, short, ___num, d0, BPTR, ___lock, d1, char *, ___name, a1, APTR, ___mem, a2,\
      , DOPUS_BASE_NAME)

#define OriginalCreateDir(___name) \
      LP1(0x76e, BPTR, OriginalCreateDir , char *, ___name, d1,\
      , DOPUS_BASE_NAME)

#define OriginalDeleteFile(___name) \
      LP1(0x774, long, OriginalDeleteFile , char *, ___name, d1,\
      , DOPUS_BASE_NAME)

#define OriginalSetFileDate(___name, ___date) \
      LP2(0x77a, BOOL, OriginalSetFileDate , char *, ___name, d1, struct DateStamp *, ___date, d2,\
      , DOPUS_BASE_NAME)

#define OriginalSetComment(___name, ___comment) \
      LP2(0x780, BOOL, OriginalSetComment , char *, ___name, d1, char *, ___comment, d2,\
      , DOPUS_BASE_NAME)

#define OriginalSetProtection(___name, ___mask) \
      LP2(0x786, BOOL, OriginalSetProtection , char *, ___name, d1, ULONG, ___mask, d2,\
      , DOPUS_BASE_NAME)

#define OriginalRename(___oldname, ___newname) \
      LP2(0x78c, BOOL, OriginalRename , char *, ___oldname, d1, char *, ___newname, d2,\
      , DOPUS_BASE_NAME)

#define OriginalOpen(___name, ___access) \
      LP2(0x792, BPTR, OriginalOpen , char *, ___name, d1, LONG, ___access, d2,\
      , DOPUS_BASE_NAME)

#define OriginalClose(___file) \
      LP1(0x798, BOOL, OriginalClose , BPTR, ___file, d1,\
      , DOPUS_BASE_NAME)

#define OriginalWrite(___file, ___data, ___length) \
      LP3(0x79e, LONG, OriginalWrite , BPTR, ___file, d1, void *, ___data, d2, LONG, ___length, d3,\
      , DOPUS_BASE_NAME)

#define CreateTitleGadget(___scr, ___list, ___zoom, ___offset, ___type, ___id) \
      LP6(0x7a4, struct Gadget *, CreateTitleGadget , struct Screen *, ___scr, a0, struct List *, ___list, a1, BOOL, ___zoom, d0, short, ___offset, d1, short, ___type, d2, UWORD, ___id, d3,\
      , DOPUS_BASE_NAME)

#define FindGadgetType(___gad, ___type) \
      LP2(0x7aa, struct Gadget *, FindGadgetType , struct Gadget *, ___gad, a0, UWORD, ___type, d0,\
      , DOPUS_BASE_NAME)

#define FixTitleGadgets(___win) \
      LP1NR(0x7b0, FixTitleGadgets , struct Window *, ___win, a0,\
      , DOPUS_BASE_NAME)

#define OriginalRelabel(___dev, ___name) \
      LP2(0x7b6, BOOL, OriginalRelabel , char *, ___dev, d1, char *, ___name, d2,\
      , DOPUS_BASE_NAME)

#define FakeILBM(___data, ___pal, ___w, ___h, ___d, ___flags) \
      LP6(0x7bc, ILBMHandle *, FakeILBM , UWORD *, ___data, a0, ULONG *, ___pal, a1, short, ___w, d0, short, ___h, d1, short, ___d, d2, ULONG, ___flags, d3,\
      , DOPUS_BASE_NAME)

#define IPC_SafeCommand(___ipc, ___command, ___flags, ___data, ___data_free, ___reply, ___list) \
      LP7A4(0x7c2, IPTR, IPC_SafeCommand , IPCData *, ___ipc, a0, ULONG, ___command, d0, IPTR, ___flags, d1, APTR, ___data, a1, APTR, ___data_free, a2, struct MsgPort *, ___reply, a3, struct ListLock *, ___list, d7,\
      , DOPUS_BASE_NAME)

#define ClearFiletypeCache() \
      LP0NR(0x7c8, ClearFiletypeCache ,\
      , DOPUS_BASE_NAME)

#define GetTimerBase() \
      LP0(0x7ce, struct Library *, GetTimerBase ,\
      , DOPUS_BASE_NAME)

#define InitDragDBuf(___drag) \
      LP1(0x7d4, BOOL, InitDragDBuf , DragInfo *, ___drag, a0,\
      , DOPUS_BASE_NAME)

#define FreeRexxMsgEx(___msg) \
      LP1NR(0x7da, FreeRexxMsgEx , struct RexxMsg *, ___msg, a0,\
      , DOPUS_BASE_NAME)

#define CreateRexxMsgEx(___port, ___extension, ___host) \
      LP3(0x7e0, struct RexxMsg *, CreateRexxMsgEx , struct MsgPort *, ___port, a0, UBYTE *, ___extension, a1, UBYTE *, ___host, d0,\
      , DOPUS_BASE_NAME)

#define SetRexxVarEx(___msg, ___varname, ___value, ___length) \
      LP4(0x7e6, long, SetRexxVarEx , struct RexxMsg *, ___msg, a0, char *, ___varname, a1, char *, ___value, d0, long, ___length, d1,\
      , DOPUS_BASE_NAME)

#define GetRexxVarEx(___msg, ___varname, ___bufpointer) \
      LP3(0x7ec, long, GetRexxVarEx , struct RexxMsg *, ___msg, a0, char *, ___varname, a1, char **, ___bufpointer, a2,\
      , DOPUS_BASE_NAME)

#define BuildRexxMsgEx(___port, ___extension, ___host, ___tags) \
      LP4(0x7f2, struct RexxMsg *, BuildRexxMsgEx , struct MsgPort *, ___port, a0, UBYTE *, ___extension, a1, UBYTE *, ___host, d0, struct TagItem *, ___tags, a2,\
      , DOPUS_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define BuildRexxMsgExTags(___port, ___extension, ___host, ___tags, ...) \
    ({_sfdc_vararg _tags[] = { ___tags, __VA_ARGS__ }; BuildRexxMsgEx((___port), (___extension), (___host), (struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define NotifyDiskChange() \
      LP0NR(0x7f8, NotifyDiskChange ,\
      , DOPUS_BASE_NAME)

#define GetDosListCopy(___list, ___memory) \
      LP2NR(0x7fe, GetDosListCopy , struct List *, ___list, a0, APTR, ___memory, a1,\
      , DOPUS_BASE_NAME)

#define FreeDosListCopy(___list) \
      LP1NR(0x804, FreeDosListCopy , struct List *, ___list, a0,\
      , DOPUS_BASE_NAME)

#define DateFromStringsNew(___date, ___time, ___ds, ___method) \
      LP4(0x80a, BOOL, DateFromStringsNew , char *, ___date, a0, char *, ___time, a1, struct DateStamp *, ___ds, a2, ULONG, ___method, d0,\
      , DOPUS_BASE_NAME)

#define RemapIcon(___icon, ___screen, ___free) \
      LP3(0x810, BOOL, RemapIcon , struct DiskObject *, ___icon, a0, struct Screen *, ___screen, a1, short, ___free, d0,\
      , DOPUS_BASE_NAME)

#define GetOriginalIcon(___icon) \
      LP1(0x816, struct DiskObject *, GetOriginalIcon , struct DiskObject *, ___icon, a0,\
      , DOPUS_BASE_NAME)

#define CalcPercent(___amount, ___total, ___utilbase) \
      LP3(0x81c, long, CalcPercent , ULONG, ___amount, d0, ULONG, ___total, d1, struct Library *, ___utilbase, a0,\
      , DOPUS_BASE_NAME)

#define IsDiskDevice(___port) \
      LP1(0x822, BOOL, IsDiskDevice , struct MsgPort *, ___port, a0,\
      , DOPUS_BASE_NAME)

#define DrawDragList(___rp, ___vp, ___flags) \
      LP3NR(0x828, DrawDragList , struct RastPort *, ___rp, a0, struct ViewPort *, ___vp, a1, long, ___flags, d0,\
      , DOPUS_BASE_NAME)

#define RemoveDragImage(___drag) \
      LP1NR(0x82e, RemoveDragImage , DragInfo *, ___drag, a0,\
      , DOPUS_BASE_NAME)

#define SetNewIconsFlags(___flags, ___prec) \
      LP2NR(0x834, SetNewIconsFlags , ULONG, ___flags, d0, short, ___prec, d1,\
      , DOPUS_BASE_NAME)

#define ReadBufLine(___file, ___data, ___size) \
      LP3(0x83a, long, ReadBufLine , APTR, ___file, a0, char *, ___data, a1, long, ___size, d0,\
      , DOPUS_BASE_NAME)

#define GetLibraryFlags() \
      LP0(0x840, ULONG, GetLibraryFlags ,\
      , DOPUS_BASE_NAME)

#define GetIconType(___icon) \
      LP1(0x846, short, GetIconType , struct DiskObject *, ___icon, a0,\
      , DOPUS_BASE_NAME)

#define SetReqBackFill(___hook, ___scr) \
      LP2NR(0x84c, SetReqBackFill , struct Hook *, ___hook, a0, struct Screen **, ___scr, a1,\
      , DOPUS_BASE_NAME)

#define LockReqBackFill(___scr) \
      LP1(0x852, struct Hook *, LockReqBackFill , struct Screen *, ___scr, a0,\
      , DOPUS_BASE_NAME)

#define UnlockReqBackFill() \
      LP0NR(0x858, UnlockReqBackFill ,\
      , DOPUS_BASE_NAME)

#define DragCustomOk(___bm) \
      LP1(0x85e, BOOL, DragCustomOk , struct BitMap *, ___bm, a0,\
      , DOPUS_BASE_NAME)

#define WB_LaunchNew(___name, ___scr, ___wait, ___stack, ___tool) \
      LP5(0x864, BOOL, WB_LaunchNew , char *, ___name, a0, struct Screen *, ___scr, a1, short, ___wait, d0, long, ___stack, d1, char *, ___tool, a2,\
      , DOPUS_BASE_NAME)

#define UpdatePathList() \
      LP0NR(0x86a, UpdatePathList ,\
      , DOPUS_BASE_NAME)

#define UpdateMyPaths() \
      LP0NR(0x870, UpdateMyPaths ,\
      , DOPUS_BASE_NAME)

#define GetPopUpImageSize(___window, ___menu, ___width, ___height) \
      LP4NR(0x876, GetPopUpImageSize , struct Window *, ___window, a0, PopUpMenu *, ___menu, a1, short *, ___width, a2, short *, ___height, a3,\
      , DOPUS_BASE_NAME)

#define GetDeviceUnit(___startup, ___device, ___unit) \
      LP3(0x87c, BOOL, GetDeviceUnit , BPTR, ___startup, a0, char *, ___device, a1, short *, ___unit, a2,\
      , DOPUS_BASE_NAME)

#define StripWindowMessages(___port, ___except) \
      LP2NR(0x882, StripWindowMessages , struct MsgPort *, ___port, a0, struct IntuiMessage *, ___except, a1,\
      , DOPUS_BASE_NAME)

#define DeleteIcon(___name) \
      LP1(0x888, BOOL, DeleteIcon , char *, ___name, a0,\
      , DOPUS_BASE_NAME)

#define MUFSLogin(___window, ___name, ___password) \
      LP3NR(0x88e, MUFSLogin , struct Window *, ___window, a0, char *, ___name, a1, char *, ___password, a2,\
      , DOPUS_BASE_NAME)

#define UpdateEnvironment(___env) \
      LP1NR(0x894, UpdateEnvironment , CFG_ENVR *, ___env, a0,\
      , DOPUS_BASE_NAME)

#define ConvertStartMenu(___bank) \
      LP1NR(0x89a, ConvertStartMenu , Cfg_ButtonBank *, ___bank, a0,\
      , DOPUS_BASE_NAME)

#define GetOpusPathList() \
      LP0(0x8a0, BPTR, GetOpusPathList ,\
      , DOPUS_BASE_NAME)

#define GetStatistics(___id) \
      LP1(0x8a6, long, GetStatistics , long, ___id, d0,\
      , DOPUS_BASE_NAME)

#define SetPopUpDelay(___delay) \
      LP1NR(0x8ac, SetPopUpDelay , short, ___delay, d0,\
      , DOPUS_BASE_NAME)

#define WB_LaunchNotify(___name, ___scr, ___wait, ___stack, ___tool, ___proc, ___notify, ___flags) \
      LP8A4(0x8b2, BOOL, WB_LaunchNotify , char *, ___name, a0, struct Screen *, ___scr, a1, short, ___wait, d0, long, ___stack, d1, char *, ___tool, a2, struct Process **, ___proc, a3, IPCData *, ___notify, d7, ULONG, ___flags, d2,\
      , DOPUS_BASE_NAME)

#define WB_AppWindowWindow(___appwindow) \
      LP1(0x8b8, struct Window *, WB_AppWindowWindow , struct AppWindow *, ___appwindow, a0,\
      , DOPUS_BASE_NAME)

#define OpenEnvironment(___name, ___data) \
      LP2(0x8be, BOOL, OpenEnvironment , char *, ___name, a0, struct OpenEnvironmentData *, ___data, a1,\
      , DOPUS_BASE_NAME)

#define PopUpNewHandle(___data, ___callback, ___locale) \
      LP3(0x8c4, PopUpHandle *, PopUpNewHandle , IPTR, ___data, d0, REF_CALLBACK, ___callback, a0, struct DOpusLocale *, ___locale, a1,\
      , DOPUS_BASE_NAME)

#define PopUpFreeHandle(___handle) \
      LP1NR(0x8ca, PopUpFreeHandle , PopUpHandle *, ___handle, a0,\
      , DOPUS_BASE_NAME)

#define PopUpNewItem(___handle, ___string, ___id, ___flags) \
      LP4(0x8d0, PopUpItem *, PopUpNewItem , PopUpHandle *, ___handle, a0, IPTR, ___string, d0, ULONG, ___id, d1, ULONG, ___flags, d2,\
      , DOPUS_BASE_NAME)

#define PopUpSeparator(___handle) \
      LP1NR(0x8d6, PopUpSeparator , PopUpHandle *, ___handle, a0,\
      , DOPUS_BASE_NAME)

#define PopUpItemSub(___handle, ___item) \
      LP2(0x8dc, BOOL, PopUpItemSub , PopUpHandle *, ___handle, a0, PopUpItem *, ___item, a1,\
      , DOPUS_BASE_NAME)

#define PopUpEndSub(___handle) \
      LP1NR(0x8e2, PopUpEndSub , PopUpHandle *, ___handle, a0,\
      , DOPUS_BASE_NAME)

#define PopUpSetFlags(___menu, ___id, ___value, ___mask) \
      LP4(0x8e8, ULONG, PopUpSetFlags , PopUpMenu *, ___menu, a0, UWORD, ___id, d0, ULONG, ___value, d1, ULONG, ___mask, d2,\
      , DOPUS_BASE_NAME)

#define AddAllocBitmapPatch(___task, ___screen) \
      LP2(0x8ee, APTR, AddAllocBitmapPatch , struct Task *, ___task, a0, struct Screen *, ___screen, a1,\
      , DOPUS_BASE_NAME)

#define RemAllocBitmapPatch(___handle) \
      LP1NR(0x8f4, RemAllocBitmapPatch , APTR, ___handle, a0,\
      , DOPUS_BASE_NAME)

#define LoadPos(___name, ___pos, ___font) \
      LP3(0x8fa, BOOL, LoadPos , char *, ___name, a0, struct IBox *, ___pos, a1, short *, ___font, d0,\
      , DOPUS_BASE_NAME)

#define SavePos(___name, ___pos, ___font) \
      LP3(0x900, BOOL, SavePos , char *, ___name, a0, struct IBox *, ___pos, a1, short, ___font, d0,\
      , DOPUS_BASE_NAME)

#define DivideU64(___num, ___div, ___rem, ___quo) \
      LP4NR(0x906, DivideU64 , UQUAD *, ___num, a0, ULONG, ___div, d0, UQUAD *, ___rem, a1, UQUAD *, ___quo, a2,\
      , DOPUS_BASE_NAME)

#define ItoaU64(___num, ___str, ___str_size, ___sep) \
      LP4NR(0x90c, ItoaU64 , UQUAD *, ___num, a0, char *, ___str, a1, int, ___str_size, d0, char, ___sep, d1,\
      , DOPUS_BASE_NAME)

#define DivideToString64(___string, ___str_size, ___bytes, ___div, ___places, ___sep) \
      LP6NR(0x912, DivideToString64 , char *, ___string, a0, int, ___str_size, d0, UQUAD *, ___bytes, a1, ULONG, ___div, d1, int, ___places, d2, char, ___sep, d3,\
      , DOPUS_BASE_NAME)

#define BytesToString64(___bytes, ___string, ___str_size, ___places, ___sep) \
      LP5NR(0x918, BytesToString64 , UQUAD *, ___bytes, a0, char *, ___string, a1, int, ___str_size, d0, int, ___places, d1, char, ___sep, d2,\
      , DOPUS_BASE_NAME)

#define ExamineLock64(___lock, ___fib) \
      LP2(0x91e, BOOL, ExamineLock64 , BPTR, ___lock, d0, FileInfoBlock64 *, ___fib, a0,\
      , DOPUS_BASE_NAME)

#define ExamineNext64(___lock, ___fib) \
      LP2(0x924, BOOL, ExamineNext64 , BPTR, ___lock, d0, FileInfoBlock64 *, ___fib, a0,\
      , DOPUS_BASE_NAME)

#define ExamineHandle64(___fh, ___fib) \
      LP2(0x92a, BOOL, ExamineHandle64 , BPTR, ___fh, d0, FileInfoBlock64 *, ___fib, a0,\
      , DOPUS_BASE_NAME)

#define MatchFirst64(___pat, ___panchor) \
      LP2(0x930, LONG, MatchFirst64 , STRPTR, ___pat, a0, struct AnchorPath *, ___panchor, a1,\
      , DOPUS_BASE_NAME)

#define MatchNext64(___panchor) \
      LP1(0x936, LONG, MatchNext64 , struct AnchorPath *, ___panchor, a0,\
      , DOPUS_BASE_NAME)

#endif /* !_INLINE_DOPUS_H */
