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

	aos4_ppc_libstubs.c - OS4 Dopus5 library stubs
*/

#include "dopuslib.h"

// dopus5 stubs:

LIBSTUB(L_DivideU,
		ULONG,
		REG(d0, ULONG num),
		REG(d1, ULONG div),
		REG(a0, ULONG *rem),
		REG(a1, struct Library *lib))
{
	return L_DivideU(num, div, rem, lib);
}

// functions.a
LIBSTUB(L_BtoCStr, void, REG(a0, BSTR bstr), REG(a1, char *cstr), REG(d0, int len))
{
	return L_BtoCStr(bstr, cstr, len);
}

// timer.c
LIBSTUB(L_AllocTimer, struct TimerHandle *, REG(d0, ULONG unit), REG(a0, struct MsgPort *port))
{
	return L_AllocTimer(unit, port);
}

LIBSTUB(L_FreeTimer, void, REG(a0, struct TimerHandle *handle))
{
	return L_FreeTimer(handle);
}
LIBSTUB(L_StartTimer, void, REG(a0, TimerHandle *handle), REG(d0, ULONG seconds), REG(d1, ULONG micros))
{
	return L_StartTimer(handle, seconds, micros);
}

LIBSTUB(L_CheckTimer, BOOL, REG(a0, TimerHandle *handle))
{
	return L_CheckTimer(handle);
}

LIBSTUB(L_StopTimer, void, REG(a0, TimerHandle *handle))
{
	return L_StopTimer(handle);
}

// pathlist.c
LIBSTUB(L_GetDosPathList, BPTR, REG(a0, BPTR copy_list))
{
	return L_GetDosPathList(copy_list);
}

LIBSTUB(L_FreeDosPathList, void, REG(a0, BPTR list))
{
	return L_FreeDosPathList(list);
}

LIBSTUB(L_CopyLocalEnv, void, REG(a0, struct Library *DOSBase))
{
	return L_CopyLocalEnv(DOSBase);
}

// strings.c
LIBSTUB(L_StrCombine, BOOL, REG(a0, char *buf), REG(a1, char *one), REG(a2, char *two), REG(d0, int lim))
{
	return L_StrCombine(buf, one, two, lim);
}
LIBSTUB(L_StrConcat, BOOL, REG(a0, char *buf), REG(a1, char *cat), REG(d0, int lim))
{
	return L_StrConcat(buf, cat, lim);
}

LIBSTUB(L_Atoh, ULONG, REG(a0, unsigned char *buf), REG(d0, short len))
{
	return L_Atoh(buf, len);
}

// gui.c
LIBSTUB(L_DrawBox,
		void,
		REG(a0, struct RastPort *rp),
		REG(a1, struct Rectangle *rect),
		REG(a2, struct DrawInfo *info),
		REG(d0, BOOL recessed))
{
	return L_DrawBox(rp, rect, info, recessed);
}

LIBSTUB(L_DrawFieldBox,
		void,
		REG(a0, struct RastPort *rp),
		REG(a1, struct Rectangle *rect),
		REG(a2, struct DrawInfo *info))
{
	return L_DrawFieldBox(rp, rect, info);
}

LIBSTUB(L_WriteIcon, BOOL, REG(a0, char *name), REG(a1, struct DiskObject *diskobj), REG(a6, struct MyLibrary *libbase))
{
	return L_WriteIcon(name, diskobj, libbase);
}

LIBSTUB(L_DeleteIcon, BOOL, REG(a0, char *name), REG(a6, struct MyLibrary *libbase))
{
	return L_DeleteIcon(name, libbase);
}

LIBSTUB(L_ScreenInfo, ULONG, REG(a0, struct Screen *screen))
{
	return L_ScreenInfo(screen);
}

// misc.c
LIBSTUB(L_BuildKeyString,
		void,
		REG(d0, unsigned short code),
		REG(d1, unsigned short qual),
		REG(d2, unsigned short qual_mask),
		REG(d3, unsigned short qual_same),
		REG(a0, char *buffer))
{
	return L_BuildKeyString(code, qual, qual_mask, qual_same, buffer);
}

LIBSTUB(L_QualValid, UWORD, REG(d0, unsigned short qual))
{
	return L_QualValid(qual);
}

LIBSTUB(L_ConvertRawKey, BOOL, REG(d0, UWORD code), REG(d1, UWORD qual), REG(a0, char *key))
{
	return L_ConvertRawKey(code, qual, key);
}

LIBSTUB(L_SetBusyPointer, void, REG(a0, struct Window *wind))
{
	return L_SetBusyPointer(wind);
}

LIBSTUB(L_ActivateStrGad, void, REG(a0, struct Gadget *gad), REG(a1, struct Window *win))
{
	return L_ActivateStrGad(gad, win);
}

LIBSTUB(L_Itoa, void, REG(d0, long num), REG(a0, char *str), REG(d1, char sep))
{
	return L_Itoa(num, str, sep);
}

LIBSTUB(L_ItoaU, void, REG(d0, unsigned long num), REG(a0, char *str), REG(d1, char sep))
{
	return L_ItoaU(num, str, sep);
}

LIBSTUB(L_BytesToString,
		void,
		REG(d0, unsigned long bytes),
		REG(a0, char *string),
		REG(d1, short places),
		REG(d2, char sep))
{
	return L_BytesToString(bytes, string, places, sep);
}

LIBSTUB(L_DivideToString,
		void,
		REG(a0, char *string),
		REG(d0, unsigned long bytes),
		REG(d1, unsigned long div),
		REG(d2, short places),
		REG(d3, char sep))
{
	return L_DivideToString(string, bytes, div, places, sep);
}

LIBSTUB(L_Ito26, void, REG(d0, unsigned long num), REG(a0, char *str))
{
	return L_Ito26(num, str);
}

LIBSTUB(L_SerialValid, BOOL, REG(a0, serial_data *data))
{
	return L_SerialValid(data);
}

LIBSTUB(L_WriteFileIcon, void, REG(a0, char *source), REG(a1, char *dest), REG(a6, struct MyLibrary *libbase))
{
	return L_WriteFileIcon(source, dest, libbase);
}

LIBSTUB(L_GetWBArgPath,
		BOOL,
		REG(a0, struct WBArg *arg),
		REG(a1, char *buffer),
		REG(d0, long size),
		REG(a6, struct MyLibrary *libbase))
{
	return L_GetWBArgPath(arg, buffer, size, libbase);
}

LIBSTUB(L_FindPubScreen, struct PubScreenNode *, REG(a0, struct Screen *screen), REG(d0, BOOL lock))
{
	return L_FindPubScreen(screen, lock);
}

// simplerequest.c
LIBSTUB(L_DoSimpleRequest,
		short,
		REG(a0, struct Window *parent),
		REG(a1, struct DOpusSimpleRequest *simple),
		REG(a6, struct Library *libbase))
{
	return L_DoSimpleRequest(parent, simple, libbase);
}

LIBSTUB(L_SimpleRequest,
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
{
	return L_SimpleRequest(parent, title, buttons, message, buffer, params, buffersize, flags, libbase);
}

// drag_routines.c
LIBSTUB(L_GetDragInfo,
		DragInfo *,
		REG(a0, struct Window *window),
		REG(a1, struct RastPort *rast),
		REG(d0, long width),
		REG(d1, long height),
		REG(d2, long flags),
		REG(a6, struct MyLibrary *libbase))
{
	return L_GetDragInfo(window, rast, width, height, flags, libbase);
}

LIBSTUB(L_FreeDragInfo, void, REG(a0, DragInfo *drag))
{
	return L_FreeDragInfo(drag);
}

LIBSTUB(L_HideDragImage, void, REG(a0, DragInfo *drag))
{
	return L_HideDragImage(drag);
}

LIBSTUB(L_GetDragImage, void, REG(a0, DragInfo *drag), REG(d0, ULONG x), REG(d1, ULONG y))
{
	return L_GetDragImage(drag, x, y);
}

LIBSTUB(L_GetDragMask, void, REG(a0, DragInfo *drag))
{
	return L_GetDragMask(drag);
}

LIBSTUB(L_ShowDragImage, void, REG(a0, DragInfo *drag), REG(d0, ULONG x), REG(d1, ULONG y))
{
	return L_ShowDragImage(drag, x, y);
}

LIBSTUB(L_AddDragImage, void, REG(a0, DragInfo *drag))
{
	return L_AddDragImage(drag);
}

LIBSTUB(L_RemDragImage, void, REG(a0, DragInfo *drag))
{
	return L_RemDragImage(drag);
}

LIBSTUB(L_StampDragImage, void, REG(a0, DragInfo *drag), REG(d0, ULONG x), REG(d1, ULONG y))
{
	return L_StampDragImage(drag, x, y);
}

LIBSTUB(L_CheckDragDeadlock, BOOL, REG(a0, DragInfo *drag))
{
	return L_CheckDragDeadlock(drag);
}

LIBSTUB(L_BuildTransDragMask,
		BOOL,
		REG(a0, UWORD *mask),
		REG(a1, UWORD *image),
		REG(d0, short width),
		REG(d1, short height),
		REG(d2, short depth),
		REG(d3, long flags))
{
	return L_BuildTransDragMask(mask, image, width, height, depth, flags);
}

LIBSTUB(L_DrawDragList, void, REG(a0, struct RastPort *rp), REG(a1, struct ViewPort *vp), REG(d0, long flags))
{
	return L_DrawDragList(rp, vp, flags);
}

LIBSTUB(L_RemoveDragImage, void, REG(a0, DragInfo *drag))
{
	return L_RemoveDragImage(drag);
}

LIBSTUB(L_DragCustomOk, BOOL, REG(a0, struct BitMap *bm), REG(a6, struct MyLibrary *libbase))
{
	return L_DragCustomOk(bm, libbase);
}

// layout_routines.c
LIBSTUB(L_OpenConfigWindow, struct Window *, REG(a0, NewConfigWindow *newwindow), REG(a6, struct MyLibrary *libbase))
{
	return L_OpenConfigWindow(newwindow, libbase);
}

LIBSTUB(L_CloseConfigWindow, void, REG(a0, struct Window *window), REG(a6, struct MyLibrary *libbase))
{
	return L_CloseConfigWindow(window, libbase);
}

LIBSTUB(L_StripIntuiMessagesDopus, void, REG(a0, struct Window *window))
{
	return L_StripIntuiMessagesDopus(window);
}

LIBSTUB(L_StripWindowMessages, void, REG(a0, struct MsgPort *port), REG(a1, struct IntuiMessage *except))
{
	return L_StripWindowMessages(port, except);
}

LIBSTUB(L_CloseWindowSafely, void, REG(a0, struct Window *window))
{
	return L_CloseWindowSafely(window);
}

LIBSTUB(L_CalcObjectDims,
		int,
		REG(a0, void *parent),
		REG(a1, struct TextFont *use_font),
		REG(a2, struct IBox *pos),
		REG(a3, struct IBox *dest_pos),
		REG(a4, GL_Object *last_ob),
		REG(d0, ULONG flags),
		REG(d1, GL_Object *this_ob),
		REG(d2, GL_Object *parent_ob))
{
	return L_CalcObjectDims(parent, use_font, pos, dest_pos, last_ob, flags, this_ob, parent_ob);
}

LIBSTUB(L_CalcWindowDims,
		int,
		REG(a0, struct Screen *screen),
		REG(a1, const ConfigWindow *win_pos),
		REG(a2, struct IBox *dest_pos),
		REG(a3, struct TextFont *font),
		REG(d0, ULONG flags))
{
	return L_CalcWindowDims(screen, win_pos, dest_pos, font, flags);
}

LIBSTUB(L_AddObjectList,
		ObjectList *,
		REG(a0, struct Window *window),
		REG(a1, const ObjectDef *objects),
		REG(a6, struct MyLibrary *libbase))
{
	return L_AddObjectList(window, objects, libbase);
}

LIBSTUB(L_FreeObject, void, REG(a0, ObjectList *objlist), REG(a1, GL_Object *object))
{
	return L_FreeObject(objlist, object);
}

LIBSTUB(L_FreeObjectList, void, REG(a0, ObjectList *objlist))
{
	return L_FreeObjectList(objlist);
}

LIBSTUB(L_GetObject, GL_Object *, REG(a0, ObjectList *list), REG(d0, int id))
{
	return L_GetObject(list, id);
}

LIBSTUB(L_StoreGadgetValue,
		void,
		REG(a0, ObjectList *list),
		REG(a1, struct IntuiMessage *msg),
		REG(a6, struct MyLibrary *libbase))
{
	return L_StoreGadgetValue(list, msg, libbase);
}

LIBSTUB(L_UpdateGadgetValue,
		void,
		REG(a0, ObjectList *list),
		REG(a1, struct IntuiMessage *msg),
		REG(d0, UWORD id),
		REG(a6, struct MyLibrary *libbase))
{
	return L_UpdateGadgetValue(list, msg, id, libbase);
}

LIBSTUB(L_SetGadgetValue, void, REG(a0, ObjectList *list), REG(d0, UWORD id), REG(d1, IPTR value))
{
	return L_SetGadgetValue(list, id, value);
}

LIBSTUB(L_GetGadgetValue, IPTR, REG(a0, ObjectList *list), REG(a1, UWORD id), REG(a6, struct MyLibrary *libbase))
{
	return L_GetGadgetValue(list, id, libbase);
}

LIBSTUB(L_CheckObjectArea, BOOL, REG(a0, GL_Object *object), REG(d0, int x), REG(d1, int y))
{
	return L_CheckObjectArea(object, x, y);
}

LIBSTUB(L_DisplayObject,
		void,
		REG(a0, struct Window *window),
		REG(a1, GL_Object *object),
		REG(d0, int fg),
		REG(d1, int bg),
		REG(a2, char *txt))
{
	return L_DisplayObject(window, object, fg, bg, txt);
}

LIBSTUB(L_AddWindowMenus, void, REG(a0, struct Window *window), REG(a1, const MenuData *menudata))
{
	return L_AddWindowMenus(window, menudata);
}

LIBSTUB(L_FreeWindowMenus, void, REG(a0, struct Window *window))
{
	return L_FreeWindowMenus(window);
}

LIBSTUB(L_SetWindowBusy, void, REG(a0, struct Window *window))
{
	return L_SetWindowBusy(window);
}

LIBSTUB(L_ClearWindowBusy, void, REG(a0, struct Window *window))
{
	return L_ClearWindowBusy(window);
}

LIBSTUB(L_GetString, STRPTR, REG(a0, struct DOpusLocale *li), REG(d0, LONG stringNum))
{
	return L_GetString(li, stringNum);
}

LIBSTUB(L_FindKeyEquivalent,
		struct Gadget *,
		REG(a0, ObjectList *list),
		REG(a1, struct IntuiMessage *msg),
		REG(d0, int process))
{
	return L_FindKeyEquivalent(list, msg, process);
}

LIBSTUB(L_ShowProgressBar,
		void,
		REG(a0, struct Window *window),
		REG(a1, GL_Object *object),
		REG(d0, ULONG total),
		REG(d1, ULONG count))
{
	return L_ShowProgressBar(window, object, total, count);
}

LIBSTUB(L_SetObjectKind, void, REG(a0, ObjectList *list), REG(d0, ULONG id), REG(d1, UWORD kind))
{
	return L_SetObjectKind(list, id, kind);
}

LIBSTUB(L_DisableObject, void, REG(a0, ObjectList *list), REG(d0, ULONG id), REG(d1, BOOL state))
{
	return L_DisableObject(list, id, state);
}

LIBSTUB(L_BoundsCheckGadget, int, REG(a0, ObjectList *list), REG(d0, ULONG id), REG(d1, int min), REG(d2, int max))
{
	return L_BoundsCheckGadget(list, id, min, max);
}

LIBSTUB(L_RefreshObjectList, void, REG(a0, struct Window *window), REG(a1, ObjectList *ref_list))
{
	return L_RefreshObjectList(window, ref_list);
}

LIBSTUB(L_GetWindowMsg, struct IntuiMessage *, REG(a0, struct MsgPort *port), REG(a6, struct MyLibrary *lib))
{
	return L_GetWindowMsg(port, lib);
}

LIBSTUB(L_ReplyWindowMsg, void, REG(a0, struct IntuiMessage *msg))
{
	return L_ReplyWindowMsg(msg);
}

LIBSTUB(L_SetGadgetChoices, void, REG(a0, ObjectList *list), REG(d0, ULONG id), REG(a1, APTR choices))
{
	return L_SetGadgetChoices(list, id, choices);
}

LIBSTUB(L_SetWindowID,
		void,
		REG(a0, struct Window *window),
		REG(a1, WindowID *id),
		REG(d0, ULONG window_id),
		REG(a2, struct MsgPort *port))
{
	return L_SetWindowID(window, id, window_id, port);
}

LIBSTUB(L_GetWindowID, ULONG, REG(a0, struct Window *window))
{
	return L_GetWindowID(window);
}

LIBSTUB(L_GetWindowAppPort, struct MsgPort *, REG(a0, struct Window *window))
{
	return L_GetWindowAppPort(window);
}

LIBSTUB(L_GetObjectRect, BOOL, REG(a0, ObjectList *list), REG(d0, ULONG id), REG(a1, struct Rectangle *rect))
{
	return L_GetObjectRect(list, id, rect);
}

LIBSTUB(L_StartRefreshConfigWindow, void, REG(a0, struct Window *window), REG(d0, long final_state))
{
	return L_StartRefreshConfigWindow(window, final_state);
}

LIBSTUB(L_EndRefreshConfigWindow, void, REG(a0, struct Window *window))
{
	return L_EndRefreshConfigWindow(window);
}

// menu_routines.c
LIBSTUB(L_BuildMenuStrip, struct Menu *, REG(a0, const MenuData *menudata), REG(a1, struct DOpusLocale *locale))
{
	return L_BuildMenuStrip(menudata, locale);
}

LIBSTUB(L_FindMenuItem, struct MenuItem *, REG(a0, struct Menu *menu), REG(d0, UWORD id))
{
	return L_FindMenuItem(menu, id);
}

// list_management.c
LIBSTUB(L_Att_NewList, Att_List *, REG(d0, ULONG flags))
{
	return L_Att_NewList(flags);
}

LIBSTUB(L_Att_NewNode,
		Att_Node *,
		REG(a0, Att_List *list),
		REG(a1, const char *name),
		REG(d0, IPTR data),
		REG(d1, ULONG flags))
{
	return L_Att_NewNode(list, name, data, flags);
}

LIBSTUB(L_Att_RemNode, void, REG(a0, Att_Node *node))
{
	return L_Att_RemNode(node);
}

LIBSTUB(L_Att_PosNode, void, REG(a0, Att_List *list), REG(a1, Att_Node *node), REG(a2, Att_Node *before))
{
	return L_Att_PosNode(list, node, before);
}

LIBSTUB(L_Att_RemList, void, REG(a0, Att_List *list), REG(d0, long flags))
{
	return L_Att_RemList(list, flags);
}

LIBSTUB(L_Att_FindNode, Att_Node *, REG(a0, Att_List *list), REG(d0, long number))
{
	return L_Att_FindNode(list, number);
}

LIBSTUB(L_Att_NodeNumber, long, REG(a0, Att_List *list), REG(a1, char *name))
{
	return L_Att_NodeNumber(list, name);
}

LIBSTUB(L_Att_FindNodeData, Att_Node *, REG(a0, Att_List *list), REG(d0, IPTR data))
{
	return L_Att_FindNodeData(list, data);
}

LIBSTUB(L_Att_NodeDataNumber, long, REG(a0, Att_List *list), REG(d0, IPTR data))
{
	return L_Att_NodeDataNumber(list, data);
}

LIBSTUB(L_Att_NodeName, char *, REG(a0, Att_List *list), REG(d0, long number))
{
	return L_Att_NodeName(list, number);
}

LIBSTUB(L_Att_NodeCount, long, REG(a0, Att_List *list))
{
	return L_Att_NodeCount(list);
}

LIBSTUB(L_Att_ChangeNodeName, void, REG(a0, Att_Node *node), REG(a1, char *name))
{
	return L_Att_ChangeNodeName(node, name);
}

LIBSTUB(L_Att_FindNodeNumber, long, REG(a0, Att_List *list), REG(a1, Att_Node *node))
{
	return L_Att_FindNodeNumber(list, node);
}

LIBSTUB(L_AddSorted, void, REG(a0, struct List *list), REG(a1, struct Node *node))
{
	return L_AddSorted(list, node);
}

LIBSTUB(L_FindNameI, struct Node *, REG(a0, struct List *list), REG(a1, char *name))
{
	return L_FindNameI(list, name);
}

LIBSTUB(L_LockAttList, void, REG(a0, Att_List *list), REG(d0, short exclusive))
{
	return L_LockAttList(list, exclusive);
}

LIBSTUB(L_UnlockAttList, void, REG(a0, Att_List *list))
{
	return L_UnlockAttList(list);
}

LIBSTUB(L_SwapListNodes, void, REG(a0, struct List *list), REG(a1, struct Node *swap1), REG(a2, struct Node *swap2))
{
	return L_SwapListNodes(list, swap1, swap2);
}

LIBSTUB(L_IsListLockEmpty, BOOL, REG(a0, struct ListLock *list))
{
	return L_IsListLockEmpty(list);
}

// memory.c
LIBSTUB(L_NewMemHandle, void *, REG(d0, ULONG puddle_size), REG(d1, ULONG thresh_size), REG(d2, ULONG type))
{
	return L_NewMemHandle(puddle_size, thresh_size, type);
}

LIBSTUB(L_FreeMemHandle, void, REG(a0, MemHandle *handle))
{
	return L_FreeMemHandle(handle);
}

LIBSTUB(L_ClearMemHandle, void, REG(a0, MemHandle *handle))
{
	return L_ClearMemHandle(handle);
}

LIBSTUB(L_AllocMemH, void *, REG(a0, MemHandle *handle), REG(d0, ULONG size))
{
	return L_AllocMemH(handle, size);
}

LIBSTUB(L_FreeMemH, void, REG(a0, void *memory))
{
	return L_FreeMemH(memory);
}

// config routines
LIBSTUB(L_NewLister, Cfg_Lister *, REG(a0, char *path))
{
	return L_NewLister(path);
}

LIBSTUB(L_NewButtonBank, Cfg_ButtonBank *, REG(d0, BOOL init), REG(d1, short type))
{
	return L_NewButtonBank(init, type);
}

LIBSTUB(L_NewButton, Cfg_Button *, REG(a0, APTR memory))
{
	return L_NewButton(memory);
}

LIBSTUB(L_NewFunction, Cfg_Function *, REG(a0, APTR memory), REG(d0, UWORD type))
{
	return L_NewFunction(memory, type);
}

LIBSTUB(L_NewButtonFunction, Cfg_ButtonFunction *, REG(a0, APTR memory), REG(d0, UWORD type))
{
	return L_NewButtonFunction(memory, type);
}

LIBSTUB(L_NewInstruction, Cfg_Instruction *, REG(a0, APTR memory), REG(d0, short type), REG(a1, char *string))
{
	return L_NewInstruction(memory, type, string);
}

LIBSTUB(L_NewFiletype, Cfg_Filetype *, REG(a0, APTR memory))
{
	return L_NewFiletype(memory);
}

LIBSTUB(L_ReadSettings, short, REG(a0, CFG_SETS *settings), REG(a1, char *name))
{
	return L_ReadSettings(settings, name);
}

LIBSTUB(L_ReadListerDef, Cfg_Lister *, REG(a0, struct _IFFHandle *iff), REG(d0, ULONG id))
{
	return L_ReadListerDef(iff, id);
}

LIBSTUB(L_OpenButtonBank, Cfg_ButtonBank *, REG(a0, char *name))
{
	return L_OpenButtonBank(name);
}

LIBSTUB(L_ReadFiletypes, Cfg_FiletypeList *, REG(a0, char *name), REG(a1, APTR memory))
{
	return L_ReadFiletypes(name, memory);
}

LIBSTUB(L_ReadButton, Cfg_Button *, REG(a0, struct _IFFHandle *iff), REG(a1, APTR memory))
{
	return L_ReadButton(iff, memory);
}

LIBSTUB(L_ReadFunction,
		Cfg_Function *,
		REG(a0, struct _IFFHandle *iff),
		REG(a1, APTR memory),
		REG(a2, struct List *func_list),
		REG(a3, Cfg_Function *function))
{
	return L_ReadFunction(iff, memory, func_list, function);
}

LIBSTUB(L_ConvertStartMenu, void, REG(a0, Cfg_ButtonBank *bank))
{
	return L_ConvertStartMenu(bank);
}

LIBSTUB(L_DefaultSettings, void, REG(a0, CFG_SETS *settings))
{
	return L_DefaultSettings(settings);
}

LIBSTUB(L_DefaultEnvironment, void, REG(a0, CFG_ENVR *env))
{
	return L_DefaultEnvironment(env);
}

LIBSTUB(L_DefaultButtonBank, Cfg_ButtonBank *)
{
	return L_DefaultButtonBank();
}

LIBSTUB(L_UpdateEnvironment, void, REG(a0, CFG_ENVR *env))
{
	return L_UpdateEnvironment(env);
}

LIBSTUB(L_SaveListerDef, long, REG(a0, struct _IFFHandle *iff), REG(a1, Cfg_Lister *lister))
{
	return L_SaveListerDef(iff, lister);
}

LIBSTUB(L_SaveButton, short, REG(a0, struct _IFFHandle *iff), REG(a1, Cfg_Button *button))
{
	return L_SaveButton(iff, button);
}

LIBSTUB(L_SaveFunction, BOOL, REG(a0, struct _IFFHandle *iff), REG(a1, Cfg_Function *function))
{
	return L_SaveFunction(iff, function);
}

LIBSTUB(L_CloseButtonBank, void, REG(a0, Cfg_ButtonBank *bank))
{
	return L_CloseButtonBank(bank);
}

LIBSTUB(L_FreeListerDef, void, REG(a0, Cfg_Lister *lister))
{
	return L_FreeListerDef(lister);
}

LIBSTUB(L_FreeButtonList, void, REG(a0, struct List *list))
{
	return L_FreeButtonList(list);
}

LIBSTUB(L_FreeButtonImages, void, REG(a0, struct List *list))
{
	return L_FreeButtonImages(list);
}

LIBSTUB(L_FreeButton, void, REG(a0, Cfg_Button *button))
{
	return L_FreeButton(button);
}

LIBSTUB(L_FreeFunction, void, REG(a0, Cfg_Function *function))
{
	return L_FreeFunction(function);
}

LIBSTUB(L_FreeButtonFunction, void, REG(a0, Cfg_ButtonFunction *function))
{
	return L_FreeButtonFunction(function);
}

LIBSTUB(L_FreeInstruction, void, REG(a0, Cfg_Instruction *ins))
{
	return L_FreeInstruction(ins);
}

LIBSTUB(L_FreeInstructionList, void, REG(a0, Cfg_Function *func))
{
	return L_FreeInstructionList(func);
}

LIBSTUB(L_FreeFiletypeList, void, REG(a0, Cfg_FiletypeList *list))
{
	return L_FreeFiletypeList(list);
}

LIBSTUB(L_FreeFiletype, void, REG(a0, Cfg_Filetype *type))
{
	return L_FreeFiletype(type);
}

LIBSTUB(L_CopyButtonBank, Cfg_ButtonBank *, REG(a0, Cfg_ButtonBank *orig))
{
	return L_CopyButtonBank(orig);
}

LIBSTUB(L_CopyButton, Cfg_Button *, REG(a0, Cfg_Button *orig), REG(a1, APTR memory), REG(d0, short type))
{
	return L_CopyButton(orig, memory, type);
}

LIBSTUB(L_CopyFiletype, Cfg_Filetype *, REG(a0, Cfg_Filetype *orig), REG(a1, APTR memory))
{
	return L_CopyFiletype(orig, memory);
}

LIBSTUB(L_CopyFunction,
		Cfg_Function *,
		REG(a0, Cfg_Function *orig),
		REG(a1, APTR memory),
		REG(a2, Cfg_Function *newfunc))
{
	return L_CopyFunction(orig, memory, newfunc);
}

LIBSTUB(L_CopyButtonFunction,
		Cfg_ButtonFunction *,
		REG(a0, Cfg_ButtonFunction *func),
		REG(a1, APTR memory),
		REG(a2, Cfg_ButtonFunction *newfunc))
{
	return L_CopyButtonFunction(func, memory, newfunc);
}

LIBSTUB(L_FindFunctionType, Cfg_Function *, REG(a0, struct List *list), REG(d0, UWORD type))
{
	return L_FindFunctionType(list, type);
}

// popup menus
LIBSTUB(L_DoPopUpMenu,
		UWORD,
		REG(a0, struct Window *window),
		REG(a1, PopUpMenu *menu),
		REG(a2, PopUpItem **sel_item),
		REG(d0, UWORD code),
		REG(a6, struct MyLibrary *libbase))
{
	return L_DoPopUpMenu(window, menu, sel_item, code, libbase);
}

LIBSTUB(L_GetPopUpItem, PopUpItem *, REG(a0, PopUpMenu *menu), REG(d0, UWORD id))
{
	return L_GetPopUpItem(menu, id);
}

LIBSTUB(L_SetPopUpDelay, void, REG(d0, short delay), REG(a6, struct MyLibrary *libbase))
{
	return L_SetPopUpDelay(delay, libbase);
}

// ipc
LIBSTUB(L_IPC_Launch,
		int,
		REG(a0, struct ListLock *list),
		REG(a1, IPCData **storage),
		REG(a2, char *name),
		REG(d0, IPTR entry),
		REG(d1, ULONG stack),
		REG(d2, IPTR data),
		REG(a3, struct Library *dos_base),
		REG(a6, struct MyLibrary *libbase))
{
	return L_IPC_Launch(list, storage, name, entry, stack, data, dos_base, libbase);
}

LIBSTUB(L_IPC_Startup, int, REG(a0, IPCData *ipc), REG(a1, APTR data), REG(a2, struct MsgPort *reply))
{
	return L_IPC_Startup(ipc, data, reply);
}

LIBSTUB(L_IPC_Command,
		IPTR,
		REG(a0, IPCData *ipc),
		REG(d0, ULONG command),
		REG(d1, IPTR flags),
		REG(a1, APTR data),
		REG(a2, APTR data_free),
		REG(a3, struct MsgPort *reply))
{
	return L_IPC_Command(ipc, command, flags, data, data_free, reply);
}

LIBSTUB(L_IPC_Reply, void, REG(a0, IPCMessage *msg))
{
	return L_IPC_Reply(msg);
}

LIBSTUB(L_IPC_Free, void, REG(a0, IPCData *ipc))
{
	return L_IPC_Free(ipc);
}

LIBSTUB(L_IPC_FindProc,
		IPCData *,
		REG(a0, struct ListLock *list),
		REG(a1, char *name),
		REG(d0, BOOL activate),
		REG(d1, IPTR data))
{
	return L_IPC_FindProc(list, name, activate, data);
}

LIBSTUB(L_IPC_ProcStartup,
		IPCData *,
		REG(a0, IPTR *data),
		REG(a1, ULONG (*ASM code)(REG(a0, IPCData *), REG(a1, APTR))))
{
	return L_IPC_ProcStartup(data, code);
}

LIBSTUB(L_IPC_Quit, void, REG(a0, IPCData *ipc), REG(d0, ULONG quit_flags), REG(d1, BOOL wait))
{
	return L_IPC_Quit(ipc, quit_flags, wait);
}

LIBSTUB(L_IPC_Hello, void, REG(a0, IPCData *ipc), REG(a1, IPCData *owner))
{
	return L_IPC_Hello(ipc, owner);
}

LIBSTUB(L_IPC_Goodbye, void, REG(a0, IPCData *ipc), REG(a1, IPCData *owner), REG(d0, ULONG goodbye_flags))
{
	return L_IPC_Goodbye(ipc, owner, goodbye_flags);
}

LIBSTUB(L_IPC_GetGoodbye, IPTR, REG(a0, IPCMessage *msg))
{
	return L_IPC_GetGoodbye(msg);
}

LIBSTUB(L_IPC_ListQuit,
		ULONG,
		REG(a0, struct ListLock *list),
		REG(a1, IPCData *owner),
		REG(d0, ULONG quit_flags),
		REG(d1, BOOL wait))
{
	return L_IPC_ListQuit(list, owner, quit_flags, wait);
}

LIBSTUB(L_IPC_Flush, void, REG(a0, IPCData *ipc))
{
	return L_IPC_Flush(ipc);
}

LIBSTUB(L_IPC_ListCommand,
		void,
		REG(a0, struct ListLock *list),
		REG(d0, ULONG command),
		REG(d1, IPTR flags),
		REG(d2, IPTR data),
		REG(d3, BOOL wait))
{
	return L_IPC_ListCommand(list, command, flags, data, wait);
}

LIBSTUB(L_IPC_QuitName, void, REG(a0, struct ListLock *list), REG(a1, char *name), REG(d0, ULONG quit_flags))
{
	return L_IPC_QuitName(list, name, quit_flags);
}

// Image routines
LIBSTUB(L_OpenImage, APTR, REG(a0, char *name), REG(a1, OpenImageInfo *info))
{
	return L_OpenImage(name, info);
}

LIBSTUB(L_CloseImage, void, REG(a0, APTR image))
{
	return L_CloseImage(image);
}

LIBSTUB(L_CopyImage, APTR, REG(a0, APTR image))
{
	return L_CopyImage(image);
}

LIBSTUB(L_FlushImages, void)
{
	return L_FlushImages();
}

LIBSTUB(L_RenderImage,
		short,
		REG(a0, struct RastPort *rp),
		REG(a1, APTR image),
		REG(d0, unsigned short left),
		REG(d1, unsigned short top),
		REG(a2, struct TagItem *tags))
{
	return L_RenderImage(rp, image, left, top, tags);
}

LIBSTUB(L_GetImageAttrs, void, REG(a0, APTR imptr), REG(a1, struct TagItem *tags))
{
	return L_GetImageAttrs(imptr, tags);
}

LIBSTUB(L_RemapImage, BOOL, REG(a0, Image_Data *image), REG(a1, struct Screen *screen), REG(a2, ImageRemap *remap))
{
	return L_RemapImage(image, screen, remap);
}

LIBSTUB(L_FreeRemapImage, void, REG(a0, Image_Data *image), REG(a1, ImageRemap *remap))
{
	return L_FreeRemapImage(image, remap);
}

// status window
LIBSTUB(L_OpenStatusWindow,
		struct Window *,
		REG(a0, char *title),
		REG(a1, char *text),
		REG(a2, struct Screen *screen),
		REG(d1, LONG graph),
		REG(d0, ULONG flags),
		REG(a6, struct MyLibrary *libbase))
{
	return L_OpenStatusWindow(title, text, screen, graph, flags, libbase);
}

LIBSTUB(L_SetStatusText, void, REG(a0, struct Window *window), REG(a1, char *text))
{
	return L_SetStatusText(window, text);
}

LIBSTUB(L_UpdateStatusGraph,
		void,
		REG(a0, struct Window *window),
		REG(a1, char *text),
		REG(d0, ULONG total),
		REG(d1, ULONG count))
{
	return L_UpdateStatusGraph(window, text, total, count);
}

// read_ilbm
LIBSTUB(L_ReadILBM, ILBMHandle *, REG(a0, char *name), REG(d0, ULONG flags))
{
	return L_ReadILBM(name, flags);
}

LIBSTUB(L_FreeILBM, void, REG(a0, ILBMHandle *ilbm))
{
	return L_FreeILBM(ilbm);
}

LIBSTUB(L_DecodeILBM,
		void,
		REG(a0, char *source),
		REG(d0, unsigned short width),
		REG(d1, unsigned short height),
		REG(d2, unsigned short depth),
		REG(a1, struct BitMap *dest),
		REG(d3, unsigned long flags),
		REG(d4, char comp))
{
	return L_DecodeILBM(source, width, height, depth, dest, flags, comp);
}

LIBSTUB(L_DecodeRLE, void, REG(a0, RLEinfo *rle))
{
	return L_DecodeRLE(rle);
}

LIBSTUB(L_FakeILBM,
		ILBMHandle *,
		REG(a0, UWORD *imagedata),
		REG(a1, ULONG *palette),
		REG(d0, short width),
		REG(d1, short height),
		REG(d2, short depth),
		REG(d3, ULONG flags))
{
	return L_FakeILBM(imagedata, palette, width, height, depth, flags);
}

// anim
LIBSTUB(L_AnimDecodeRIFFXor,
		void,
		REG(a0, unsigned char *delta),
		REG(a1, char *plane),
		REG(d0, unsigned short rowbytes),
		REG(d1, unsigned short sourcebytes))
{
	return L_AnimDecodeRIFFXor(delta, plane, rowbytes, sourcebytes);
}

LIBSTUB(L_AnimDecodeRIFFSet,
		void,
		REG(a0, unsigned char *delta),
		REG(a1, char *plane),
		REG(d0, unsigned short rowbytes),
		REG(d1, unsigned short sourcebytes))
{
	return L_AnimDecodeRIFFSet(delta, plane, rowbytes, sourcebytes);
}

// palette
LIBSTUB(L_LoadPalette32, void, REG(a0, struct ViewPort *vp), REG(a1, ULONG *palette))
{
	return L_LoadPalette32(vp, palette);
}

LIBSTUB(L_GetPalette32,
		void,
		REG(a0, struct ViewPort *vp),
		REG(a1, ULONG *palette),
		REG(d0, unsigned short count),
		REG(d1, short first))
{
	return L_GetPalette32(vp, palette, count, first);
}

// buffered_io
LIBSTUB(L_OpenBuf, APTR, REG(a0, char *name), REG(d0, long mode), REG(d1, long buffer_size))
{
	return L_OpenBuf(name, mode, buffer_size);
}

LIBSTUB(L_CloseBuf, long, REG(a0, APTR file))
{
	return L_CloseBuf(file);
}

LIBSTUB(L_ReadBuf, long, REG(a0, APTR file), REG(a1, char *data), REG(d0, long size))
{
	return L_ReadBuf(file, data, size);
}

LIBSTUB(L_WriteBuf, long, REG(a0, APTR file), REG(a1, const char *data), REG(d0, long size))
{
	return L_WriteBuf(file, data, size);
}

LIBSTUB(L_FlushBuf, long, REG(a0, APTR file))
{
	return L_FlushBuf(file);
}

LIBSTUB(L_SeekBuf, long, REG(a0, APTR file), REG(d0, long offset), REG(d1, long mode))
{
	return L_SeekBuf(file, offset, mode);
}

LIBSTUB(L_ExamineBuf, long, REG(a0, APTR file), REG(a1, struct FileInfoBlock *fib))
{
	return L_ExamineBuf(file, fib);
}

LIBSTUB(L_FHFromBuf, BPTR, REG(a0, APTR file))
{
	return L_FHFromBuf(file);
}

LIBSTUB(L_ReadBufLine, long, REG(a0, APTR file), REG(a1, char *data), REG(d0, long size))
{
	return L_ReadBufLine(file, data, size);
}

// diskio
LIBSTUB(L_OpenDisk, DiskHandle *, REG(a0, char *disk), REG(a1, struct MsgPort *port))
{
	return L_OpenDisk(disk, port);
}

LIBSTUB(L_CloseDisk, void, REG(a0, DiskHandle *handle))
{
	return L_CloseDisk(handle);
}

// boopsi
LIBSTUB(L_AddScrollBars,
		struct Gadget *,
		REG(a0, struct Window *window),
		REG(a1, struct List *list),
		REG(a2, struct DrawInfo *draw_info),
		REG(d0, short noidcmpupdate))
{
	return L_AddScrollBars(window, list, draw_info, noidcmpupdate);
}

LIBSTUB(L_FindBOOPSIGadget, struct Gadget *, REG(a0, struct List *list), REG(d0, UWORD id))
{
	return L_FindBOOPSIGadget(list, id);
}

LIBSTUB(L_BOOPSIFree, void, REG(a0, struct List *list))
{
	return L_BOOPSIFree(list);
}

LIBSTUB(L_CreateTitleGadget,
		struct Gadget *,
		REG(a0, struct Screen *screen),
		REG(a1, struct List *list),
		REG(d0, BOOL cover_zoom),
		REG(d1, short offset),
		REG(d2, short type),
		REG(d3, unsigned short id),
		REG(a6, struct MyLibrary *libbase))
{
	return L_CreateTitleGadget(screen, list, cover_zoom, offset, type, id, libbase);
}

LIBSTUB(L_FindGadgetType, struct Gadget *, REG(a0, struct Gadget *gadget), REG(d0, UWORD type))
{
	return L_FindGadgetType(gadget, type);
}

LIBSTUB(L_FixTitleGadgets, void, REG(a0, struct Window *window))
{
	return L_FixTitleGadgets(window);
}

// iff
LIBSTUB(L_IFFOpen, struct _IFFHandle *, REG(a0, char *name), REG(d0, unsigned short mode), REG(d1, ULONG form))
{
	return L_IFFOpen(name, mode, form);
}

LIBSTUB(L_IFFClose, void, REG(a0, struct _IFFHandle *handle))
{
	return L_IFFClose(handle);
}

LIBSTUB(L_IFFPushChunk, long, REG(a0, struct _IFFHandle *handle), REG(d0, ULONG id))
{
	return L_IFFPushChunk(handle, id);
}

LIBSTUB(L_IFFWriteChunkBytes, long, REG(a0, struct _IFFHandle *handle), REG(a1, char *data), REG(d0, long size))
{
	return L_IFFWriteChunkBytes(handle, data, size);
}

LIBSTUB(L_IFFPopChunk, long, REG(a0, struct _IFFHandle *handle))
{
	return L_IFFPopChunk(handle);
}

LIBSTUB(L_IFFWriteChunk,
		long,
		REG(a0, struct _IFFHandle *handle),
		REG(a1, char *data),
		REG(d0, ULONG chunk),
		REG(d1, ULONG size))
{
	return L_IFFWriteChunk(handle, data, chunk, size);
}

LIBSTUB(L_IFFNextChunk, unsigned long, REG(a0, struct _IFFHandle *handle), REG(d0, unsigned long form))
{
	return L_IFFNextChunk(handle, form);
}

LIBSTUB(L_IFFChunkSize, long, REG(a0, struct _IFFHandle *handle))
{
	return L_IFFChunkSize(handle);
}

LIBSTUB(L_IFFReadChunkBytes, long, REG(a0, struct _IFFHandle *handle), REG(a1, APTR buffer), REG(d0, long size))
{
	return L_IFFReadChunkBytes(handle, buffer, size);
}

LIBSTUB(L_IFFFileHandle, APTR, REG(a0, struct _IFFHandle *handle))
{
	return L_IFFFileHandle(handle);
}

LIBSTUB(L_IFFChunkRemain, long, REG(a0, struct _IFFHandle *handle))
{
	return L_IFFChunkRemain(handle);
}

LIBSTUB(L_IFFChunkID, unsigned long, REG(a0, struct _IFFHandle *handle))
{
	return L_IFFChunkID(handle);
}

LIBSTUB(L_IFFGetFORM, unsigned long, REG(a0, struct _IFFHandle *handle))
{
	return L_IFFGetFORM(handle);
}

LIBSTUB(L_GetSemaphore, long, REG(a0, struct SignalSemaphore *sem), REG(d0, long exclusive), REG(a1, char *data))
{
	return L_GetSemaphore(sem, exclusive, data);
}

LIBSTUB(L_FreeSemaphore, void, REG(a0, struct SignalSemaphore *sem))
{
	return L_FreeSemaphore(sem);
}

LIBSTUB(L_ShowSemaphore, void, REG(a0, struct SignalSemaphore *sem))
{
	return L_ShowSemaphore(sem);
}

LIBSTUB(L_InitListLock, void, REG(a0, struct ListLock *ll), REG(a1, char *name))
{
	return L_InitListLock(ll, name);
}

// layout_utils.c
LIBSTUB(L_InitWindowDims, void, REG(a0, struct Window *window), REG(a1, WindowDimensions *dims))
{
	return L_InitWindowDims(window, dims);
}

LIBSTUB(L_StoreWindowDims, void, REG(a0, struct Window *window), REG(a1, WindowDimensions *dims))
{
	return L_StoreWindowDims(window, dims);
}

LIBSTUB(L_CheckWindowDims, BOOL, REG(a0, struct Window *window), REG(a1, WindowDimensions *dims))
{
	return L_CheckWindowDims(window, dims);
}

LIBSTUB(L_LayoutResize, void, REG(a0, struct Window *window))
{
	return L_LayoutResize(window);
}

// devices
LIBSTUB(L_DeviceFromLock, struct DosList *, REG(a0, BPTR lock), REG(a1, char *name), REG(a6, struct MyLibrary *libbase))
{
	return L_DeviceFromLock(lock, name, libbase);
}

LIBSTUB(L_DeviceFromHandler,
		struct DosList *,
		REG(a0, struct MsgPort *port),
		REG(a1, char *name),
		REG(a6, struct MyLibrary *libbase))
{
	return L_DeviceFromHandler(port, name, libbase);
}

LIBSTUB(L_DevNameFromLockDopus,
		BOOL,
		REG(d1, BPTR lock),
		REG(d2, char *buffer),
		REG(d3, long len),
		REG(a6, struct MyLibrary *libbase))
{
	return L_DevNameFromLockDopus(lock, buffer, len, libbase);
}

LIBSTUB(L_GetDeviceUnit, BOOL, REG(a0, BPTR dol_Startup), REG(a1, char *device), REG(a2, short *unit))
{
	return L_GetDeviceUnit(dol_Startup, device, unit);
}

// icon cache
LIBSTUB(L_GetCachedDefDiskObject, struct DiskObject *, REG(d0, long type), REG(a6, struct MyLibrary *libbase))
{
	return L_GetCachedDefDiskObject(type, libbase);
}

LIBSTUB(L_FreeCachedDiskObject, void, REG(a0, struct DiskObject *icon), REG(a6, struct MyLibrary *libbase))
{
	return L_FreeCachedDiskObject(icon, libbase);
}

LIBSTUB(L_GetCachedDiskObject,
		struct DiskObject *,
		REG(a0, char *name),
		REG(d0, ULONG flags),
		REG(a6, struct MyLibrary *libbase))
{
	return L_GetCachedDiskObject(name, flags, libbase);
}

LIBSTUB(L_GetCachedDiskObjectNew,
		struct DiskObject *,
		REG(a0, char *name),
		REG(d0, ULONG flags),
		REG(a6, struct MyLibrary *libbase))
{
	return L_GetCachedDiskObjectNew(name, flags, libbase);
}

LIBSTUB(L_IconCheckSum, unsigned long, REG(a0, struct DiskObject *icon), REG(d0, short which))
{
	return L_IconCheckSum(icon, which);
}

LIBSTUB(L_GetIconType, short, REG(a0, struct DiskObject *icon))
{
	return L_GetIconType(icon);
}

// progress
LIBSTUB(L_OpenProgressWindow, struct _ProgressWindow *, REG(a0, struct TagItem *tags), REG(a6, struct MyLibrary *lib))
{
	return L_OpenProgressWindow(tags, lib);
}

LIBSTUB(L_CloseProgressWindow, void, REG(a0, struct _ProgressWindow *prog))
{
	return L_CloseProgressWindow(prog);
}

LIBSTUB(L_HideProgressWindow, void, REG(a0, struct _ProgressWindow *prog))
{
	return L_HideProgressWindow(prog);
}

LIBSTUB(L_ShowProgressWindow,
		void,
		REG(a0, struct _ProgressWindow *prog),
		REG(a1, struct Screen *screen),
		REG(a2, struct Window *window))
{
	return L_ShowProgressWindow(prog, screen, window);
}

LIBSTUB(L_SetProgressWindow, void, REG(a0, struct _ProgressWindow *prog), REG(a1, struct TagItem *tags))
{
	return L_SetProgressWindow(prog, tags);
}

LIBSTUB(L_CheckProgressAbort, BOOL, REG(a0, struct _ProgressWindow *prog))
{
	return L_CheckProgressAbort(prog);
}

LIBSTUB(L_CalcPercent, long, REG(d0, ULONG amount), REG(d1, ULONG total), REG(a0, struct Library *UtilityBase))
{
	return L_CalcPercent(amount, total, UtilityBase);
}

// string_hook
LIBSTUB(L_GetSecureString, char *, REG(a0, struct Gadget *gadget))
{
	return L_GetSecureString(gadget);
}

// search
LIBSTUB(L_SearchFile,
		long,
		REG(a0, APTR file),
		REG(a1, UBYTE *search_text),
		REG(d0, ULONG flags),
		REG(a2, UBYTE *buffer),
		REG(d1, ULONG buffer_size))
{
	return L_SearchFile(file, search_text, flags, buffer, buffer_size);
}

// dates
LIBSTUB(L_ParseDateStrings,
		char *,
		REG(a0, char *string),
		REG(a1, char *date_buffer),
		REG(a2, char *time_buffer),
		REG(a3, long *range))
{
	return L_ParseDateStrings(string, date_buffer, time_buffer, range);
}

LIBSTUB(L_DateFromStrings, BOOL, REG(a0, char *date), REG(a1, char *time), REG(a2, struct DateStamp *ds))
{
	return L_DateFromStrings(date, time, ds);
}

LIBSTUB(L_DateFromStringsNew,
		BOOL,
		REG(a0, char *date),
		REG(a1, char *time),
		REG(a2, struct DateStamp *ds),
		REG(d0, ULONG method))
{
	return L_DateFromStringsNew(date, time, ds, method);
}

// filetypes
LIBSTUB(L_GetMatchHandle, APTR, REG(a0, char *name), REG(a6, struct MyLibrary *lib))
{
	return L_GetMatchHandle(name, lib);
}

LIBSTUB(L_FreeMatchHandle, void, REG(a0, MatchHandle *handle))
{
	return L_FreeMatchHandle(handle);
}

LIBSTUB(L_MatchFiletype,
		BOOL,
		REG(a0, MatchHandle *handle),
		REG(a1, Cfg_Filetype *type),
		REG(a6, struct MyLibrary *lib))
{
	return L_MatchFiletype(handle, type, lib);
}

// dos patches
LIBSTUB(L_PatchedCreateDir, BPTR, REG(d1, char *name))
{
	return L_PatchedCreateDir(name);
}

LIBSTUB(L_OriginalCreateDir, BPTR, REG(d1, char *name), REG(a6, struct MyLibrary *libbase))
{
	return L_OriginalCreateDir(name, libbase);
}

LIBSTUB(L_PatchedDeleteFile, BPTR, REG(d1, char *name))
{
	return L_PatchedDeleteFile(name);
}

LIBSTUB(L_OriginalDeleteFile, long, REG(d1, char *name), REG(a6, struct MyLibrary *libbase))
{
	return L_OriginalDeleteFile(name, libbase);
}

// requesters
LIBSTUB(L_AsyncRequest,
		long,
		REG(a0, IPCData *my_ipc),
		REG(d0, long type),
		REG(a1, struct Window *window),
		REG(a2, REF_CALLBACK(callback)),
		REG(a3, APTR data),
		REG(d1, struct TagItem *tags),
		REG(a6, struct MyLibrary *libbase))
{
	return L_AsyncRequest(my_ipc, type, window, callback, data, tags, libbase);
}

LIBSTUB(L_CheckRefreshMsg, struct IntuiMessage *, REG(a0, struct Window *window), REG(d0, ULONG mask))
{
	return L_CheckRefreshMsg(window, mask);
}

// bitmap.c
LIBSTUB(L_NewBitMap,
		struct BitMap *,
		REG(d0, ULONG sizex),
		REG(d1, ULONG sizey),
		REG(d2, ULONG depth),
		REG(d3, ULONG flags),
		REG(a0, struct BitMap *friend_bitmap))
{
	return L_NewBitMap(sizex, sizey, depth, flags, friend_bitmap);
}

LIBSTUB(L_DisposeBitMap, void, REG(a0, struct BitMap *bm))
{
	return L_DisposeBitMap(bm);
}

// args.c
LIBSTUB(L_ParseArgs, FuncArgs *, REG(a0, char *template), REG(a1, char *args))
{
	return L_ParseArgs(template, args);
}

LIBSTUB(L_DisposeArgs, void, REG(a0, FuncArgs *args))
{
	return L_DisposeArgs(args);
}

// appmsg.c
LIBSTUB(L_AllocAppMessage, DOpusAppMessage *, REG(a0, APTR memory), REG(a1, struct MsgPort *reply), REG(d0, short num))
{
	return L_AllocAppMessage(memory, reply, num);
}

LIBSTUB(L_FreeAppMessage, void, REG(a0, DOpusAppMessage *msg))
{
	return L_FreeAppMessage(msg);
}

LIBSTUB(L_ReplyAppMessage, void, REG(a0, DOpusAppMessage *msg))
{
	return L_ReplyAppMessage(msg);
}

LIBSTUB(L_CheckAppMessage, BOOL, REG(a0, DOpusAppMessage *msg))
{
	return L_CheckAppMessage(msg);
}

LIBSTUB(L_CopyAppMessage, DOpusAppMessage *, REG(a0, DOpusAppMessage *orig), REG(a1, APTR memory))
{
	return L_CopyAppMessage(orig, memory);
}

LIBSTUB(L_SetWBArg,
		BOOL,
		REG(a0, DOpusAppMessage *msg),
		REG(d0, short num),
		REG(d1, BPTR lock),
		REG(a1, char *name),
		REG(a2, APTR memory))
{
	return L_SetWBArg(msg, num, lock, name, memory);
}

// notify.c
LIBSTUB(L_AddNotifyRequest,
		APTR,
		REG(d0, ULONG type),
		REG(d1, ULONG userdata),
		REG(a0, struct MsgPort *port),
		REG(a6, struct MyLibrary *libbase))
{
	return L_AddNotifyRequest(type, userdata, port, libbase);
}

LIBSTUB(L_RemoveNotifyRequest, void, REG(a0, NotifyNode *node), REG(a6, struct MyLibrary *libbase))
{
	return L_RemoveNotifyRequest(node, libbase);
}

LIBSTUB(L_SendNotifyMsg,
		void,
		REG(d0, ULONG type),
		REG(d1, IPTR data),
		REG(d2, ULONG flags),
		REG(d3, short wait),
		REG(a0, char *name),
		REG(a1, struct FileInfoBlock *fib),
		REG(a6, struct MyLibrary *libbase))
{
	return L_SendNotifyMsg(type, data, flags, wait, name, fib, libbase);
}

// backfill stuff
LIBSTUB(L_SetReqBackFill,
		void,
		REG(a0, struct Hook *hook),
		REG(a1, struct Screen **screen),
		REG(a6, struct MyLibrary *libbase))
{
	return L_SetReqBackFill(hook, screen, libbase);
}

LIBSTUB(L_LockReqBackFill, struct Hook *, REG(a0, struct Screen *screen), REG(a6, struct MyLibrary *libbase))
{
	return L_LockReqBackFill(screen, libbase);
}

LIBSTUB(L_UnlockReqBackFill, void, REG(a6, struct MyLibrary *libbase))
{
	return L_UnlockReqBackFill(libbase);
}

LIBSTUB(L_SetEnv, void, REG(a0, char *name), REG(a1, char *data), REG(d0, BOOL save))
{
	return L_SetEnv(name, data, save);
}

LIBSTUB(L_SavePos, BOOL, REG(a0, char *name), REG(a1, struct IBox *box), REG(d0, short fontsize))
{
	return L_SavePos(name, box, fontsize);
}

LIBSTUB(L_LoadPos, BOOL, REG(a0, char *name), REG(a1, struct IBox *box), REG(d0, short *fontsize))
{
	return L_LoadPos(name, box, fontsize);
}

LIBSTUB(L_SetConfigWindowLimits,
		void,
		REG(a0, struct Window *window),
		REG(a1, const ConfigWindow *mindims),
		REG(a2, const ConfigWindow *maxdims))
{
	return L_SetConfigWindowLimits(window, mindims, maxdims);
}

LIBSTUB(L_OpenIFFFile, struct IFFHandle *, REG(a0, char *name), REG(d0, int mode), REG(d1, ULONG check_id))
{
	return L_OpenIFFFile(name, mode, check_id);
}

LIBSTUB(L_CloseIFFFile, void, REG(a0, struct IFFHandle *iff))
{
	return L_CloseIFFFile(iff);
}

LIBSTUB(L_SelectionList,
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
{
	return L_SelectionList(
		list, parent, screen, title, selection, flags, buffer, okay_txt, cancel_txt, switch_txt, switch_flags, lib);
}

LIBSTUB(L_SaveSettings, int, REG(a0, CFG_SETS *settings), REG(a1, char *name), REG(a6, struct MyLibrary *libbase))
{
	return L_SaveSettings(settings, name, libbase);
}

LIBSTUB(L_SaveButtonBank, int, REG(a0, Cfg_ButtonBank *bank), REG(a1, char *name), REG(a6, struct MyLibrary *libbase))
{
	return L_SaveButtonBank(bank, name, libbase);
}

LIBSTUB(L_SaveFiletypeList,
		int,
		REG(a0, Cfg_FiletypeList *list),
		REG(a1, char *name),
		REG(a6, struct MyLibrary *libbase))
{
	return L_SaveFiletypeList(list, name, libbase);
}

LIBSTUB(L_WB_Launch, BOOL, REG(a0, char *name), REG(a1, struct Screen *errors), REG(d0, short wait))
{
	return L_WB_Launch(name, errors, wait);
}

LIBSTUB(L_CLI_Launch,
		BOOL,
		REG(a0, char *name),
		REG(a1, struct Screen *errors),
		REG(d0, BPTR currentdir),
		REG(d1, BPTR input),
		REG(d2, BPTR output),
		REG(d3, short wait),
		REG(d4, long stack))
{
	return L_CLI_Launch(name, errors, currentdir, input, output, wait, stack);
}

LIBSTUB(L_SerialCheck, BOOL, REG(a0, char *sernum), REG(a1, ULONG *nullp))
{
	return L_SerialCheck(sernum, nullp);
}
LIBSTUB(L_ChecksumFile, ULONG, REG(a0, char *filename), REG(d0, ULONG skip_marker))
{
	return L_ChecksumFile(filename, skip_marker);
}

LIBSTUB(L_ReplyFreeMsg, void, REG(a0, struct Message *msg))
{
	return L_ReplyFreeMsg(msg);
}

LIBSTUB(L_TimerActive, BOOL, REG(a0, TimerHandle *handle))
{
	return L_TimerActive(handle);
}

LIBSTUB(L_WB_AppIconFlags, unsigned long, REG(a0, struct AppIcon *icon))
{
	return L_WB_AppIconFlags(icon);
}

LIBSTUB(L_GetIconFlags, ULONG, REG(a0, struct DiskObject *icon))
{
	return L_GetIconFlags(icon);
}

LIBSTUB(L_SetIconFlags, void, REG(a0, struct DiskObject *icon), REG(d0, ULONG flags))
{
	return L_SetIconFlags(icon, flags);
}

LIBSTUB(L_GetIconPosition, void, REG(a0, struct DiskObject *icon), REG(a1, short *x), REG(a2, short *y))
{
	return L_GetIconPosition(icon, x, y);
}

LIBSTUB(L_SetIconPosition, void, REG(a0, struct DiskObject *icon), REG(d0, short x), REG(d1, short y))
{
	return L_SetIconPosition(icon, x, y);
}

LIBSTUB(L_GetImagePalette, ULONG *, REG(a0, APTR ptr))
{
	return L_GetImagePalette(ptr);
}

LIBSTUB(L_FreeImageRemap, void, REG(a0, ImageRemap *remap))
{
	return L_FreeImageRemap(remap);
}

LIBSTUB(L_IFFFailure, void, REG(a0, IFFHandle *iff))
{
	return L_IFFFailure(iff);
}

LIBSTUB(L_GetProgressWindow, void, REG(a0, ProgressWindow *prog), REG(a1, struct TagItem *tags))
{
	return L_GetProgressWindow(prog, tags);
}

LIBSTUB(L_SetNotifyRequest,
		void,
		REG(a0, NotifyNode *node),
		REG(d0, ULONG new_flags),
		REG(d1, ULONG mask),
		REG(a6, struct MyLibrary *libbase))
{
	return L_SetNotifyRequest(node, new_flags, mask, libbase);
}

LIBSTUB(L_NewButtonWithFunc, Cfg_Button *, REG(a0, APTR memory), REG(a1, char *label), REG(d0, short type))
{
	return L_NewButtonWithFunc(memory, label, type);
}

LIBSTUB(L_GetFileVersion,
		BOOL,
		REG(a0, char *name),
		REG(d0, short *version),
		REG(d1, short *revision),
		REG(a1, struct DateStamp *date),
		REG(a2, APTR progress))
{
	return L_GetFileVersion(name, version, revision, date, progress);
}

LIBSTUB(L_SetLibraryFlags, ULONG, REG(d0, ULONG flags), REG(d1, ULONG mask), REG(a6, struct MyLibrary *libbase))
{
	return L_SetLibraryFlags(flags, mask, libbase);
}

LIBSTUB(L_CompareListFormat, ULONG, REG(a0, ListFormat *format1), REG(a1, ListFormat *format2))
{
	return L_CompareListFormat(format1, format2);
}

LIBSTUB(L_UpdateGadgetList, void, REG(a0, ObjectList *list), REG(a6, struct MyLibrary *libbase))
{
	return L_UpdateGadgetList(list, libbase);
}

LIBSTUB(L_IPC_SafeCommand,
		IPTR,
		REG(a0, IPCData *ipc),
		REG(d0, ULONG command),
		REG(d1, IPTR flags),
		REG(a1, APTR data),
		REG(a2, APTR data_free),
		REG(a3, struct MsgPort *reply),
		REG(a4, struct ListLock *list))
{
	return L_IPC_SafeCommand(ipc, command, flags, data, data_free, reply, list);
}

LIBSTUB(L_ClearFiletypeCache, void, REG(a6, struct MyLibrary *lib))
{
	return L_ClearFiletypeCache(lib);
}

LIBSTUB(L_GetTimerBase, struct Library *, REG(a6, struct MyLibrary *lib))
{
	return L_GetTimerBase(lib);
}

LIBSTUB(L_InitDragDBuf, BOOL, REG(a0, DragInfo *drag))
{
	return L_InitDragDBuf(drag);
}

LIBSTUB(L_FreeRexxMsgEx, void, REG(a0, struct RexxMsg *msg))
{
	return L_FreeRexxMsgEx(msg);
}

LIBSTUB(L_CreateRexxMsgEx,
		struct RexxMsg *,
		REG(a0, struct MsgPort *port),
		REG(a1, UBYTE *extension),
		REG(d0, UBYTE *host))
{
	return L_CreateRexxMsgEx(port, extension, host);
}

LIBSTUB(L_SetRexxVarEx,
		long,
		REG(a0, struct RexxMsg *msg),
		REG(a1, char *varname),
		REG(d0, char *value),
		REG(d1, long length))
{
	return L_SetRexxVarEx(msg, varname, value, length);
}

LIBSTUB(L_GetRexxVarEx, long, REG(a0, struct RexxMsg *msg), REG(a1, char *varname), REG(a2, char **bufpointer))
{
	return L_GetRexxVarEx(msg, varname, bufpointer);
}

LIBSTUB(L_BuildRexxMsgEx,
		struct RexxMsg *,
		REG(a0, struct MsgPort *port),
		REG(a1, UBYTE *extension),
		REG(d0, UBYTE *host),
		REG(a2, struct TagItem *tags))
{
	return L_BuildRexxMsgEx(port, extension, host, tags);
}

LIBSTUB(L_NotifyDiskChange, void)
{
	return L_NotifyDiskChange();
}

LIBSTUB(L_GetDosListCopy, void, REG(a0, struct List *list), REG(a1, APTR memory), REG(a6, struct MyLibrary *libbase))
{
	return L_GetDosListCopy(list, memory, libbase);
}

LIBSTUB(L_FreeDosListCopy, void, REG(a0, struct List *list), REG(a6, struct MyLibrary *libbase))
{
	return L_FreeDosListCopy(list, libbase);
}

LIBSTUB(L_RemapIcon, BOOL, REG(a0, struct DiskObject *icon), REG(a1, struct Screen *screen), REG(d0, short free_remap))
{
	return L_RemapIcon(icon, screen, free_remap);
}

LIBSTUB(L_GetOriginalIcon, struct DiskObject *, REG(a0, struct DiskObject *icon))
{
	return L_GetOriginalIcon(icon);
}

LIBSTUB(L_IsDiskDevice, BOOL, REG(a0, struct MsgPort *port))
{
	return L_IsDiskDevice(port);
}

LIBSTUB(L_SetNewIconsFlags, void, REG(d0, ULONG flags), REG(d1, short precision), REG(a6, struct MyLibrary *libbase))
{
	return L_SetNewIconsFlags(flags, precision, libbase);
}

LIBSTUB(L_GetLibraryFlags, ULONG, REG(a6, struct MyLibrary *libbase))
{
	return L_GetLibraryFlags(libbase);
}

LIBSTUB(L_WB_LaunchNew,
		BOOL,
		REG(a0, char *name),
		REG(a1, struct Screen *errors),
		REG(d0, short wait),
		REG(d1, long stack),
		REG(a2, char *default_tool))
{
	return L_WB_LaunchNew(name, errors, wait, stack, default_tool);
}

LIBSTUB(L_UpdatePathList, void, REG(a6, struct MyLibrary *libbase))
{
	return L_UpdatePathList(libbase);
}

LIBSTUB(L_UpdateMyPaths, void, REG(a6, struct MyLibrary *libbase))
{
	return L_UpdateMyPaths(libbase);
}

LIBSTUB(L_GetPopUpImageSize,
		void,
		REG(a0, struct Window *window),
		REG(a1, PopUpMenu *menu),
		REG(a2, short *width),
		REG(a3, short *height))
{
	return L_GetPopUpImageSize(window, menu, width, height);
}

LIBSTUB(L_MUFSLogin, void, REG(a0, struct Window *window), REG(a1, char *name), REG(a2, char *password))
{
	return L_MUFSLogin(window, name, password);
}

LIBSTUB(L_GetOpusPathList, BPTR, REG(a6, struct MyLibrary *libbase))
{
	return L_GetOpusPathList(libbase);
}

LIBSTUB(L_GetStatistics, long, REG(d0, long id), REG(a6, struct MyLibrary *libbase))
{
	return L_GetStatistics(id, libbase);
}

LIBSTUB(L_WB_LaunchNotify,
		BOOL,
		REG(a0, char *name),
		REG(a1, struct Screen *errors),
		REG(d0, short wait),
		REG(d1, long stack),
		REG(a2, char *default_tool),
		REG(a3, struct Process **proc_ptr),
		REG(a4, IPCData *notify_ipc),
		REG(d2, ULONG flags))
{
	return L_WB_LaunchNotify(name, errors, wait, stack, default_tool, proc_ptr, notify_ipc, flags);
}

LIBSTUB(L_WB_AppWindowWindow, struct Window *, REG(a0, struct AppWindow *window))
{
	return L_WB_AppWindowWindow(window);
}

LIBSTUB(L_OpenEnvironment, BOOL, REG(a0, char *name), REG(a1, struct OpenEnvironmentData *data))
{
	return L_OpenEnvironment(name, data);
}

LIBSTUB(L_PopUpNewHandle,
		PopUpHandle *,
		REG(d0, IPTR userdata),
		REG(a0, REF_CALLBACK(callback)),
		REG(a1, struct DOpusLocale *locale))
{
	return L_PopUpNewHandle(userdata, callback, locale);
}

LIBSTUB(L_PopUpFreeHandle, void, REG(a0, PopUpHandle *handle))
{
	return L_PopUpFreeHandle(handle);
}

LIBSTUB(L_PopUpNewItem,
		PopUpItem *,
		REG(a0, PopUpHandle *handle),
		REG(d0, IPTR string),
		REG(d1, ULONG id),
		REG(d2, ULONG flags))
{
	return L_PopUpNewItem(handle, string, id, flags);
}

LIBSTUB(L_PopUpSeparator, void, REG(a0, PopUpHandle *handle))
{
	return L_PopUpSeparator(handle);
}

LIBSTUB(L_PopUpItemSub, BOOL, REG(a0, PopUpHandle *menu), REG(a1, PopUpItem *item))
{
	return L_PopUpItemSub(menu, item);
}

LIBSTUB(L_PopUpEndSub, void, REG(a0, PopUpHandle *menu))
{
	return L_PopUpEndSub(menu);
}

LIBSTUB(L_PopUpSetFlags, ULONG, REG(a0, PopUpMenu *menu), REG(d0, UWORD id), REG(d1, ULONG value), REG(d2, ULONG mask))
{
	return L_PopUpSetFlags(menu, id, value, mask);
}

LIBSTUB(L_AddAllocBitmapPatch,
		APTR,
		REG(a0, struct Task *task),
		REG(a1, struct Screen *screen),
		REG(a6, struct MyLibrary *libbase))
{
	return L_AddAllocBitmapPatch(task, screen, libbase);
}

LIBSTUB(L_RemAllocBitmapPatch, void, REG(a0, APTR handle), REG(a6, struct MyLibrary *libbase))
{
	return L_RemAllocBitmapPatch(handle, libbase);
}

LIBSTUB(L_Seed, void, REG(d0, int seed))
{
	return L_Seed(seed);
}

LIBSTUB(L_RandomDopus, void, REG(d0, int limit))
{
	return L_RandomDopus(limit);
}

// clipboard (protos in clipboard.h)

LIBSTUB(L_OpenClipBoard, ClipHandle *, REG(d0, ULONG unit))
{
	return L_OpenClipBoard(unit);
}

LIBSTUB(L_CloseClipBoard, void, REG(a0, ClipHandle *clip))
{
	return L_CloseClipBoard(clip);
}

LIBSTUB(L_WriteClipString, BOOL, REG(a0, ClipHandle *clip), REG(a1, char *string), REG(d0, long length))
{
	return L_WriteClipString(clip, string, length);
}

LIBSTUB(L_ReadClipString, long, REG(a0, ClipHandle *clip), REG(a1, char *string), REG(d0, long length))
{
	return L_ReadClipString(clip, string, length);
}

// workbench (protos in wb.h)

LIBSTUB(L_WB_Install_Patch, void, REG(a6, struct MyLibrary *libbase))
{
	return L_WB_Install_Patch(libbase);
}

LIBSTUB(L_WB_Remove_Patch, BOOL, REG(a6, struct MyLibrary *libbase))
{
	return L_WB_Remove_Patch(libbase);
}

LIBSTUB(L_WB_AddAppWindow,
		struct AppWindow *,
		REG(d0, IPTR id),
		REG(d1, IPTR userdata),
		REG(a0, struct Window *window),
		REG(a1, struct MsgPort *port),
		REG(a2, struct TagItem *tags))
{
	return L_WB_AddAppWindow(id, userdata, window, port, tags);
}

LIBSTUB(L_WB_RemoveAppWindow, BOOL, REG(a0, struct AppWindow *window))
{
	return L_WB_RemoveAppWindow(window);
}

LIBSTUB(L_WB_AddAppIcon,
		struct AppIcon *,
		REG(d0, IPTR id),
		REG(d1, IPTR userdata),
		REG(a0, char *text),
		REG(a1, struct MsgPort *port),
		REG(a2, BPTR lock),
		REG(a3, struct DiskObject *object),
		REG(a4, struct TagItem *tags))
{
	return L_WB_AddAppIcon(id, userdata, text, port, lock, object, tags);
}

LIBSTUB(L_WB_RemoveAppIcon, BOOL, REG(a0, struct AppIcon *icon))
{
	return L_WB_RemoveAppIcon(icon);
}

LIBSTUB(L_WB_AddAppMenuItem,
		struct AppMenuItem *,
		REG(d0, IPTR id),
		REG(d1, IPTR userdata),
		REG(a0, char *text),
		REG(a1, struct MsgPort *port),
		REG(a2, struct TagItem *tags))
{
	return L_WB_AddAppMenuItem(id, userdata, text, port, tags);
}

LIBSTUB(L_WB_RemoveAppMenuItem, BOOL, REG(a0, struct AppMenuItem *item))
{
	return L_WB_RemoveAppMenuItem(item);
}

LIBSTUB(L_WB_FindAppWindow, struct AppWindow *, REG(a0, struct Window *window), REG(a6, struct MyLibrary *libbase))
{
	return L_WB_FindAppWindow(window, libbase);
}

LIBSTUB(L_WB_AppWindowData,
		struct MsgPort *,
		REG(a0, struct AppWindow *window),
		REG(a1, IPTR *id),
		REG(a2, IPTR *userdata))
{
	return L_WB_AppWindowData(window, id, userdata);
}

LIBSTUB(L_WB_AppWindowLocal, BOOL, REG(a0, struct AppWindow *window))
{
	return L_WB_AppWindowLocal(window);
}

/*
LIBSTUB(L_WB_AppIconSnapshot, BOOL,	REG(a0, struct AppIcon *icon))
{
	return L_WB_AppIconSnapshot(icon);
}
*/
LIBSTUB(L_LockAppList, APTR, REG(a6, struct MyLibrary *libbase))
{
	return L_LockAppList(libbase);
}

LIBSTUB(L_NextAppEntry, APTR, REG(a0, APTR last), REG(d0, ULONG type), REG(a6, struct MyLibrary *libbase))
{
	return L_NextAppEntry(last, type, libbase);
}

LIBSTUB(L_UnlockAppList, void, REG(a6, struct MyLibrary *libbase))
{
	return L_UnlockAppList(libbase);
}

LIBSTUB(L_WB_CloseWorkBench, LONG)
{
	return L_WB_CloseWorkBench();
}

LIBSTUB(L_WB_OpenWorkBench, ULONG)
{
	return L_WB_OpenWorkBench();
}

LIBSTUB(L_WB_PutDiskObject, BOOL, REG(a0, char *name), REG(a1, struct DiskObject *diskobj))
{
	return L_WB_PutDiskObject(name, diskobj);
}

LIBSTUB(L_CopyDiskObject,
		struct DiskObject *,
		REG(a0, struct DiskObject *icon),
		REG(d0, ULONG flags),
		REG(a6, struct MyLibrary *libbase))
{
	return L_CopyDiskObject(icon, flags, libbase);
}

LIBSTUB(L_FreeDiskObjectCopy, void, REG(a0, struct DiskObject *icon), REG(a6, struct MyLibrary *libbase))
{
	return L_FreeDiskObjectCopy(icon, libbase);
}

LIBSTUB(L_WB_DeleteDiskObject, BOOL, REG(a0, char *name))
{
	return L_WB_DeleteDiskObject(name);
}

LIBSTUB(L_ChangeAppIcon,
		void,
		REG(a0, APTR appicon),
		REG(a1, struct Image *render),
		REG(a2, struct Image *select),
		REG(a3, char *title),
		REG(d0, ULONG flags),
		REG(a6, struct MyLibrary *libbase))
{
	return L_ChangeAppIcon(appicon, render, select, title, flags, libbase);
}

LIBSTUB(L_SetAppIconMenuState, long, REG(a0, APTR appicon), REG(d0, long item), REG(d1, long state))
{
	return L_SetAppIconMenuState(appicon, item, state);
}

LIBSTUB(L_WB_AddPort, void, REG(a1, struct MsgPort *port))
{
	return L_WB_AddPort(port);
}

LIBSTUB(L_WB_CloseWindow, void, REG(a0, struct Window *window))
{
	return L_WB_CloseWindow(window);
}

LIBSTUB(L_PatchedWBInfo, ULONG, REG(a0, BPTR lock), REG(a1, char *name), REG(a2, struct Screen *screen))
{
	return L_PatchedWBInfo(lock, name, screen);
}

LIBSTUB(L_PatchedAddTask, APTR, REG(a1, struct Task *task), REG(a2, APTR initialPC), REG(a3, APTR finalPC))
{
	return L_PatchedAddTask(task, initialPC, finalPC);
}

LIBSTUB(L_PatchedRemTask, void, REG(a1, struct Task *task))
{
	return L_PatchedRemTask(task);
}

LIBSTUB(L_PatchedFindTask, struct Task *, REG(a1, char *name))
{
	return L_PatchedFindTask(name);
}

LIBSTUB(L_PatchedOpenWindowTags, struct Window *, REG(a0, struct NewWindow *newwin), REG(a1, struct TagItem *tags))
{
	return L_PatchedOpenWindowTags(newwin, tags);
}

/*
LIBSTUB(L_PatchedAllocBitmap, struct BitMap,	REG(d0, ULONG sizex),
												REG(d1, ULONG sizey),
												REG(d2, ULONG depth),
												REG(d3, ULONG flags),
												REG(a0, struct BitMap *friend),
												REG(a6, struct Library *GfxBase))
{
	return L_PatchedAllocBitmap(sizex,sizey,depth,flags,friend,GfxBase);
}
*/

// string_hooks (proto in string_hook.h)

LIBSTUB(L_GetEditHook, HookData *, REG(d0, ULONG type), REG(d1, ULONG flags), REG(a0, struct TagItem *tags))
{
	return L_GetEditHook(type, flags, tags);
}

LIBSTUB(L_FreeEditHook, void, REG(a0, APTR hook))
{
	return L_FreeEditHook(hook);
}

// more dopus patched from dos_patch.h

LIBSTUB(L_PatchedSetFileDate, BOOL, REG(d1, char *name), REG(d2, struct DateStamp *date))
{
	return L_PatchedSetFileDate(name, date);
}

LIBSTUB(L_OriginalSetFileDate,
		BOOL,
		REG(d1, char *name),
		REG(d2, struct DateStamp *date),
		REG(a6, struct MyLibrary *libbase))
{
	return L_OriginalSetFileDate(name, date, libbase);
}

LIBSTUB(L_PatchedSetComment, BOOL, REG(d1, char *name), REG(d2, char *comment))
{
	return L_PatchedSetComment(name, comment);
}

LIBSTUB(L_OriginalSetComment, BOOL, REG(d1, char *name), REG(d2, char *comment), REG(a6, struct MyLibrary *libbase))
{
	return L_OriginalSetComment(name, comment, libbase);
}

LIBSTUB(L_PatchedSetProtection, BOOL, REG(d1, char *name), REG(d2, ULONG mask))
{
	return L_PatchedSetProtection(name, mask);
}

LIBSTUB(L_OriginalSetProtection, BOOL, REG(d1, char *name), REG(d2, ULONG mask), REG(a6, struct MyLibrary *libbase))
{
	return L_OriginalSetProtection(name, mask, libbase);
}

LIBSTUB(L_PatchedRename, BOOL, REG(d1, char *oldname), REG(d2, char *newname))
{
	return L_PatchedRename(oldname, newname);
}

LIBSTUB(L_OriginalRename, BOOL, REG(d1, char *oldname), REG(d2, char *newname), REG(a6, struct MyLibrary *libbase))
{
	return L_OriginalRename(oldname, newname, libbase);
}

LIBSTUB(L_PatchedOpen, BPTR, REG(d1, char *name), REG(d2, LONG accessMode))
{
	return L_PatchedOpen(name, accessMode);
}

LIBSTUB(L_OriginalOpen, BPTR, REG(d1, char *name), REG(d2, LONG accessMode), REG(a6, struct MyLibrary *libbase))
{
	return L_OriginalOpen(name, accessMode, libbase);
}

LIBSTUB(L_PatchedClose, BOOL, REG(d1, BPTR file))
{
	return L_PatchedClose(file);
}

LIBSTUB(L_OriginalClose, BOOL, REG(d1, BPTR file), REG(a6, struct MyLibrary *libbase))
{
	return L_OriginalClose(file, libbase);
}

LIBSTUB(L_PatchedWrite, LONG, REG(d1, BPTR file), REG(d2, void *data), REG(d3, LONG length))
{
	return L_PatchedWrite(file, data, length);
}

LIBSTUB(L_OriginalWrite,
		LONG,
		REG(d1, BPTR file),
		REG(d2, void *data),
		REG(d3, LONG length),
		REG(a6, struct MyLibrary *libbase))
{
	return L_OriginalWrite(file, data, length, libbase);
}

LIBSTUB(L_PatchedRelabel, BOOL, REG(d1, char *volumename), REG(d2, char *name))
{
	return L_PatchedRelabel(volumename, name);
}

LIBSTUB(L_OriginalRelabel, BOOL, REG(d1, char *volumename), REG(d2, char *name), REG(a6, struct MyLibrary *libbase))
{
	return L_OriginalRelabel(volumename, name, libbase);
}

// 64bit.c
LIBSTUB(L_DivideU64, void, REG(a0, UQUAD *num), REG(d0, ULONG div), REG(a1, UQUAD *rem), REG(a2, UQUAD *quo))
{
	return L_DivideU64(num, div, rem, quo);
}

LIBSTUB(L_ItoaU64, void, REG(a0, UQUAD *num), REG(a1, char *str), REG(d0, int str_size), REG(d1, char sep))
{
	return L_ItoaU64(num, str, str_size, sep);
}

LIBSTUB(L_DivideToString64,
		void,
		REG(a0, char *string),
		REG(d0, int str_size),
		REG(a1, UQUAD *bytes),
		REG(d1, ULONG div),
		REG(d2, int places),
		REG(d3, char sep))
{
	return L_DivideToString64(string, str_size, bytes, div, places, sep);
}

LIBSTUB(L_BytesToString64,
		void,
		REG(a0, UQUAD *bytes),
		REG(a1, char *string),
		REG(d0, int str_size),
		REG(d1, int places),
		REG(d2, char sep))
{
	return L_BytesToString64(bytes, string, str_size, places, sep);
}

LIBSTUB(L_ExamineLock64, BOOL, REG(d0, BPTR lock), REG(a0, FileInfoBlock64 *fib))
{
	return L_ExamineLock64(lock, fib);
}

LIBSTUB(L_ExamineNext64, BOOL, REG(d0, BPTR lock), REG(a0, FileInfoBlock64 *fib))
{
	return L_ExamineNext64(lock, fib);
}

LIBSTUB(L_ExamineHandle64, BOOL, REG(d0, BPTR fh), REG(a0, FileInfoBlock64 *fib))
{
	return L_ExamineHandle64(fh, fib);
}

LIBSTUB(L_MatchFirst64, LONG, REG(a0, STRPTR pat), REG(a1, struct AnchorPath *panchor))
{
	return L_MatchFirst64(pat, panchor);
}

LIBSTUB(L_MatchNext64, LONG, REG(a0, struct AnchorPath *panchor))
{
	return L_MatchNext64(panchor);
}
