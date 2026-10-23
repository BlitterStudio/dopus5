#ifndef CLIB_DOPUS5_PROTOS_H
#define CLIB_DOPUS5_PROTOS_H

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

/*
 *
 * dopus5.library header file
 *
 * Support library for Directory Opus version 5
 *
 * (c) Copyright 1994-2012 Jonathan Potter
 *
 */

void RemovedFunc0(void);
UWORD RandomDopus(int);
//-----
ULONG Atoh(char *, short);
void BtoCStr(BSTR, char *, int);
ULONG DivideU(ULONG, ULONG, ULONG *, struct Library *);
void Itoa(long, char *, char);
void ItoaU(ULONG, char *, char);
void Ito26(ULONG, char *);
//-----
void BytesToString(ULONG, char *, short, char);
void DivideToString(char *, ULONG, ULONG, short, char);
//-----
void SetBusyPointer(struct Window *);
void BuildKeyString(UWORD, UWORD, UWORD, UWORD, char *);
void ActivateStrGad(struct Gadget *, struct Window *);
//----
struct TimerHandle *AllocTimer(ULONG, struct MsgPort *);
void FreeTimer(TimerHandle *);
void StartTimer(TimerHandle *, ULONG, ULONG);
BOOL CheckTimer(TimerHandle *);
void StopTimer(TimerHandle *);
//-----
BPTR GetDosPathList(BPTR);
void FreeDosPathList(BPTR);
//-----
short DoSimpleRequest(struct Window *, struct DOpusSimpleRequest *);
short SimpleRequest(struct Window *, char *, char *, char *, char *, APTR, long, ULONG);

// We need a special definition of SimpleRequestTags because fd2pragma can't generate
// the proper call into SimpleRequest (with tags in the middle of the parameters)
#ifdef __GNUC__
#define SimpleRequestTags(window, title, buttons, message, tags...)                 \
        ({                                                                          \
            IPTR _tags[] = {tags};                                                 \
            SimpleRequest(window, title, buttons, message, 0, (APTR)&_tags, 0, 0);  \
        })
#endif //__GNUC__
short SelectionList(Att_List *,
					struct Window *,
					struct Screen *,
					char *,
					short,
					ULONG,
					char *,
					char *,
					char *,
					char **,
					ULONG *);
//-----
BOOL WriteIcon(char *, struct DiskObject *);
void WriteFileIcon(char *, char *);
//-----
DragInfo *GetDragInfo(struct Window *, struct RastPort *, long, long, long);
void FreeDragInfo(DragInfo *);
void GetDragImage(DragInfo *, ULONG, ULONG);
void ShowDragImage(DragInfo *, ULONG, ULONG);
void HideDragImage(DragInfo *);
void StampDragImage(DragInfo *, ULONG, ULONG);
void GetDragMask(DragInfo *);
BOOL CheckDragDeadlock(DragInfo *);
void AddDragImage(DragInfo *);
void RemDragImage(DragInfo *);
//-----
struct Window *OpenConfigWindow(NewConfigWindow *);
void CloseConfigWindow(struct Window *);
//-----
struct IntuiMessage *GetWindowMsg(struct MsgPort *);
void ReplyWindowMsg(struct IntuiMessage *);
void StripIntuiMessagesDopus(struct Window *);
void CloseWindowSafely(struct Window *);
//-----
int CalcObjectDims(void *,
				   struct TextFont *,
				   struct IBox *,
				   struct IBox *,
				   GL_Object *,
				   ULONG,
				   GL_Object *,
				   GL_Object *);
int CalcWindowDims(struct Screen *, ConfigWindow *, struct IBox *, struct TextFont *, ULONG);
//-----
ObjectList *AddObjectList(struct Window *, const ObjectDef *);
void FreeObject(ObjectList *, GL_Object *);
void FreeObjectList(ObjectList *);
void RefreshObjectList(struct Window *, ObjectList *);
//-----
GL_Object *GetObject(ObjectList *, int);
//-----
void StoreGadgetValue(ObjectList *, struct IntuiMessage *);
void SetGadgetValue(ObjectList *, UWORD, IPTR);
IPTR GetGadgetValue(ObjectList *, UWORD);
void SetObjectKind(ObjectList *, ULONG, UWORD);
void SetGadgetChoices(ObjectList *list, ULONG id, APTR choices);
//-----
BOOL CheckObjectArea(GL_Object *, int, int);
BOOL GetObjectRect(ObjectList *, ULONG, struct Rectangle *);
void DisplayObject(struct Window *, GL_Object *, int fg, int bg, char *txt);
void DisableObject(ObjectList *, ULONG, BOOL);
int BoundsCheckGadget(ObjectList *, ULONG, int, int);
//-----
void AddWindowMenus(struct Window *, const MenuData *);
void FreeWindowMenus(struct Window *);
//-----
void SetWindowBusy(struct Window *);
void ClearWindowBusy(struct Window *);
//-----
STRPTR GetString(struct DOpusLocale *, LONG);
//-----
struct Gadget *FindKeyEquivalent(ObjectList *, struct IntuiMessage *, int);
void ShowProgressBar(struct Window *, GL_Object *, ULONG, ULONG);
//-----
void SetWindowID(struct Window *, WindowID *, ULONG, struct MsgPort *);
ULONG GetWindowID(struct Window *);
struct MsgPort *GetWindowAppPort(struct Window *);
//-----
Att_List *Att_NewList(ULONG);
Att_Node *Att_NewNode(Att_List *list, const char *name, IPTR data, ULONG flags);
void Att_RemNode(Att_Node *node);
void Att_PosNode(Att_List *, Att_Node *, Att_Node *);
void Att_RemList(Att_List *list, long);
Att_Node *Att_FindNode(Att_List *list, long number);
long Att_NodeNumber(Att_List *list, char *name);
Att_Node *Att_FindNodeData(Att_List *list, IPTR data);
long Att_NodeDataNumber(Att_List *list, IPTR data);
char *Att_NodeName(Att_List *list, long number);
long Att_NodeCount(Att_List *list);
void Att_ChangeNodeName(Att_Node *node, char *name);
long Att_FindNodeNumber(Att_List *list, Att_Node *node);
void AddSorted(struct List *list, struct Node *node);
//-----
struct Menu *BuildMenuStrip(const MenuData *, struct DOpusLocale *);
struct MenuItem *FindMenuItem(struct Menu *, UWORD);
UWORD DoPopUpMenu(struct Window *, PopUpMenu *, PopUpItem **, UWORD);
PopUpItem *GetPopUpItem(PopUpMenu *, UWORD);
//-----
int IPC_Startup(IPCData *, APTR, struct MsgPort *);
IPTR IPC_Command(IPCData *, ULONG, IPTR, APTR, APTR, struct MsgPort *);
void IPC_Reply(IPCMessage *);
void IPC_Free(IPCData *);
IPCData *IPC_FindProc(struct ListLock *, char *, BOOL, IPTR);
void IPC_Quit(IPCData *, ULONG, BOOL);
void IPC_Hello(IPCData *, IPCData *);
void IPC_Goodbye(IPCData *, IPCData *, ULONG);
IPTR IPC_GetGoodbye(IPCMessage *);
ULONG IPC_ListQuit(struct ListLock *list, IPCData *owner, ULONG quit_flags, BOOL wait);
void IPC_Flush(IPCData *);
void IPC_ListCommand(struct ListLock *list, ULONG command, IPTR flags, IPTR data, BOOL wait);
IPCData *IPC_ProcStartup(IPTR *, ULONG (*)(IPCData *, APTR));
int IPC_Launch(struct ListLock *, IPCData **, char *, IPTR, ULONG, IPTR, struct Library *);
//-----
APTR OpenImage(char *name, OpenImageInfo *);
void CloseImage(APTR image);
APTR CopyImage(APTR image);
void FlushImages(void);
short RenderImage(struct RastPort *, APTR, UWORD, UWORD, struct TagItem *tags);
void GetImageAttrs(APTR image, struct TagItem *tags);
//-----
void *NewMemHandle(ULONG puddle_size, ULONG thresh_size, ULONG type);
void FreeMemHandle(void *handle);
void ClearMemHandle(void *handle);
void *AllocMemH(void *handle, ULONG size);
void FreeMemH(void *memory);
//-----
void DrawBox(struct RastPort *, struct Rectangle *, struct DrawInfo *, BOOL);
void DrawFieldBox(struct RastPort *, struct Rectangle *, struct DrawInfo *);
//-----
Cfg_Lister *NewLister(char *path);
Cfg_ButtonBank *NewButtonBank(BOOL, short);
Cfg_Button *NewButton(APTR memory);
Cfg_Function *NewFunction(APTR memory, UWORD type);
Cfg_Instruction *NewInstruction(APTR memory, short type, char *string);
short ReadSettings(CFG_SETS *, char *);
Cfg_Lister *ReadListerDef(APTR, ULONG);
Cfg_ButtonBank *OpenButtonBank(char *name);
void DefaultSettings(CFG_SETS *);
void DefaultEnvironment(CFG_ENVR *);
Cfg_ButtonBank *DefaultButtonBank(void);
int SaveSettings(CFG_SETS *, char *name);
long SaveListerDef(APTR, Cfg_Lister *lister);
short SaveButtonBank(Cfg_ButtonBank *bank, char *name);
void CloseButtonBank(Cfg_ButtonBank *bank);
void FreeListerDef(Cfg_Lister *lister);
void FreeButtonList(struct List *list);
void FreeButtonImages(struct List *list);
void FreeButton(Cfg_Button *button);
void FreeFunction(Cfg_Function *function);
void FreeInstruction(Cfg_Instruction *ins);
void FreeInstructionList(Cfg_Function *func);
Cfg_ButtonBank *CopyButtonBank(Cfg_ButtonBank *orig);
Cfg_Button *CopyButton(Cfg_Button *orig, APTR, short);
Cfg_Function *CopyFunction(Cfg_Function *orig, APTR, Cfg_Function *new);
Cfg_Filetype *NewFiletype(APTR memory);
Cfg_FiletypeList *ReadFiletypes(char *name, APTR memory);
int SaveFiletypeList(Cfg_FiletypeList *list, char *name);
void FreeFiletypeList(Cfg_FiletypeList *list);
void FreeFiletype(Cfg_Filetype *type);
Cfg_Filetype *CopyFiletype(Cfg_Filetype *, APTR);
Cfg_Function *FindFunctionType(struct List *list, UWORD type);
short SaveButton(APTR, Cfg_Button *);
Cfg_Button *ReadButton(APTR iff, APTR memory);
Cfg_Function *ReadFunction(APTR iff, APTR memory, struct List *func_list, Cfg_Function *function);
//-----
struct IFFHandle *OpenIFFFile(char *, int, ULONG);
void CloseIFFFile(struct IFFHandle *);
//-----
struct Window *OpenStatusWindow(char *, char *, struct Screen *, LONG, ULONG);
void SetStatusText(struct Window *, char *);
void UpdateStatusGraph(struct Window *, char *, ULONG, ULONG);
//-----
ILBMHandle *ReadILBM(char *, ULONG);
void FreeILBM(ILBMHandle *);
void DecodeILBM(char *, UWORD, UWORD, UWORD, struct BitMap *, ULONG, char);
void DecodeRLE(RLEinfo *);
//-----
void LoadPalette32(struct ViewPort *, ULONG *);
void GetPalette32(struct ViewPort *, ULONG *, UWORD, short);
//-----
APTR OpenBuf(char *name, long mode, long buffer_size);
long CloseBuf(APTR file);
long ReadBuf(APTR file, char *data, long size);
long WriteBuf(APTR file, const char *data, long size);
long FlushBuf(APTR file);
long SeekBuf(APTR file, long offset, long mode);
long ExamineBuf(APTR file, struct FileInfoBlock *fib);
//-----
DiskHandle *OpenDisk(char *, struct MsgPort *);
void CloseDisk(DiskHandle *);
//-----
struct Gadget *AddScrollBars(struct Window *, struct List *, struct DrawInfo *, short);
struct Gadget *FindBOOPSIGadget(struct List *, UWORD);
void BOOPSIFree(struct List *);
//-----
BOOL SerialValid(serial_data *);
//-----
void WB_Install_Patch(void);
BOOL WB_Remove_Patch(void);
struct AppWindow *WB_AddAppWindow(IPTR, IPTR, struct Window *, struct MsgPort *, struct TagItem *);
BOOL WB_RemoveAppWindow(struct AppWindow *);
struct AppWindow *WB_FindAppWindow(struct Window *);
struct MsgPort *WB_AppWindowData(struct AppWindow *, IPTR *, IPTR *);
BOOL WB_AppWindowLocal(struct AppWindow *);
APTR LockAppList(void);
APTR NextAppEntry(APTR, ULONG);
void UnlockAppList(void);
//-----
APTR AddNotifyRequest(ULONG, ULONG, struct MsgPort *);
void RemoveNotifyRequest(APTR);
void SendNotifyMsg(ULONG, IPTR, ULONG, short, char *, struct FileInfoBlock *);
//-----
BOOL StrCombine(char *, char *, char *, int);
BOOL StrConcat(char *, char *, int);
//-----
BOOL WB_Launch(char *, struct Screen *, short);
//-----
void CopyLocalEnv(struct Library *);
//-----
BOOL CLI_Launch(char *, struct Screen *, BPTR, BPTR, BPTR, short, long);
//-----
BOOL SerialCheck(char *, ULONG *);
ULONG ChecksumFile(char *, ULONG);
//-----
void ReplyFreeMsg(APTR);
//-----
BOOL TimerActive(TimerHandle *);
//-----
Cfg_ButtonFunction *NewButtonFunction(APTR memory, UWORD type);
//-----
APTR IFFOpen(char *, UWORD, ULONG);
void IFFClose(APTR);
long IFFPushChunk(APTR, ULONG);
long IFFWriteChunkBytes(APTR, APTR, long);
long IFFPopChunk(APTR);
long IFFWriteChunk(APTR, APTR, ULONG, ULONG);
//-----
struct Node *FindNameI(struct List *, char *name);
//-----
void AnimDecodeRIFFXor(unsigned char *, char *, UWORD, UWORD);
void AnimDecodeRIFFSet(unsigned char *, char *, UWORD, UWORD);
//-----
BOOL ConvertRawKey(UWORD, UWORD, char *);
//-----
struct ClipHandle *OpenClipBoard(ULONG);
void CloseClipBoard(struct ClipHandle *);
BOOL WriteClipString(struct ClipHandle *, char *, long);
long ReadClipString(struct ClipHandle *, char *, long);
//-----
void LockAttList(Att_List *list, short exclusive);
void UnlockAttList(Att_List *list);
//-----
void RemovedFunc1(void);
void RemovedFunc2(void);
void RemovedFunc3(void);
void RemovedFunc4(void);
void RemovedFunc5(void);
void RemovedFunc6(void);
void RemovedFunc7(void);
//-----
long GetSemaphore(struct SignalSemaphore *, long, char *);
void FreeSemaphore(struct SignalSemaphore *);
void ShowSemaphore(struct SignalSemaphore *);
//-----
BOOL SaveFunction(APTR, Cfg_Function *);
//-----
ULONG IFFNextChunk(APTR, ULONG);
long IFFChunkSize(APTR);
long IFFReadChunkBytes(APTR, APTR, long);
APTR IFFFileHandle(APTR);
long IFFChunkRemain(APTR);
ULONG IFFChunkID(APTR);
ULONG IFFGetFORM(APTR);
//-----
ULONG ScreenInfo(struct Screen *);
//-----
struct Hook *GetEditHook(ULONG, ULONG, struct TagItem *tags);
void FreeEditHook(struct Hook *);
//-----
void InitWindowDims(struct Window *, WindowDimensions *);
void StoreWindowDims(struct Window *, WindowDimensions *);
BOOL CheckWindowDims(struct Window *, WindowDimensions *);
//-----
void InitListLock(struct ListLock *, char *);
//-----
void IPC_QuitName(struct ListLock *, char *, ULONG);
//-----
UWORD QualValid(UWORD);
//-----
BPTR FHFromBuf(APTR file);
//-----
ULONG WB_AppIconFlags(struct AppIcon *);
//-----
BOOL GetWBArgPath(struct WBArg *, char *, long);
//-----
void RemovedFunc8(void);
//-----
struct DosList *DeviceFromLock(BPTR, char *);
struct DosList *DeviceFromHandler(struct MsgPort *, char *);
BOOL DevNameFromLockDopus(BPTR, char *, long);
//-----
ULONG GetIconFlags(struct DiskObject *);
void SetIconFlags(struct DiskObject *, ULONG);
void GetIconPosition(struct DiskObject *, short *, short *);
void SetIconPosition(struct DiskObject *, short, short);
//-----
BOOL BuildTransDragMask(UWORD *, UWORD *, short, short, short, long);
//-----
ULONG *GetImagePalette(APTR image);
void FreeImageRemap(ImageRemap *);
//-----
void SwapListNodes(struct List *, struct Node *, struct Node *);
//-----
void RemovedFunc9(void);
//-----
void Seed(int);
//-----
void RemovedFunc10(void);
void RemovedFunc11(void);
void RemovedFunc12(void);
//-----
struct DiskObject *CopyDiskObject(struct DiskObject *, ULONG);
void FreeDiskObjectCopy(struct DiskObject *);
//-----
void IFFFailure(APTR);
//-----
struct DiskObject *GetCachedDefDiskObject(long);
void FreeCachedDiskObject(struct DiskObject *);
struct DiskObject *GetCachedDiskObject(char *, long);
struct DiskObject *GetCachedDiskObjectNew(char *, ULONG);
ULONG IconCheckSum(struct DiskObject *, short);
//-----
APTR OpenProgressWindow(struct TagItem *);
void CloseProgressWindow(APTR);
void HideProgressWindow(APTR);
void ShowProgressWindow(APTR, struct Screen *, struct Window *);
void SetProgressWindow(APTR, struct TagItem *);
void GetProgressWindow(APTR, struct TagItem *);
//-----
void SetNotifyRequest(APTR, ULONG, ULONG);
//-----
void ChangeAppIcon(APTR, struct Image *, struct Image *, char *, ULONG);
//-----
BOOL CheckProgressAbort(APTR);
//-----
char *GetSecureString(struct Gadget *);
//-----
Cfg_Button *NewButtonWithFunc(APTR memory, char *label, short type);
//-----
void FreeButtonFunction(Cfg_ButtonFunction *function);
Cfg_ButtonFunction *CopyButtonFunction(Cfg_ButtonFunction *, APTR, Cfg_ButtonFunction *);
//-----
struct PubScreenNode *FindPubScreen(struct Screen *, BOOL);
//-----
long SetAppIconMenuState(APTR, long, long);
//-----
long SearchFile(APTR, UBYTE *, ULONG, UBYTE *, ULONG);
char *ParseDateStrings(char *, char *, char *, long *);
BOOL DateFromStrings(char *, char *, struct DateStamp *);
//-----
APTR GetMatchHandle(char *);
void FreeMatchHandle(APTR);
BOOL MatchFiletype(APTR, APTR);
//-----
void LayoutResize(struct Window *);
//-----
BOOL GetFileVersion(char *, short *, short *, struct DateStamp *, APTR);
//-----
long AsyncRequest(IPCData *, long, struct Window *, REF_CALLBACK, APTR, struct TagItem *);
struct IntuiMessage *CheckRefreshMsg(struct Window *, ULONG);
//-----
BOOL RemapImage(APTR, struct Screen *, ImageRemap *);
void FreeRemapImage(APTR, ImageRemap *);
//-----
void FreeAppMessage(DOpusAppMessage *);
void ReplyAppMessage(DOpusAppMessage *);
//-----
ULONG SetLibraryFlags(ULONG, ULONG);
//-----
void StartRefreshConfigWindow(struct Window *, long);
void EndRefreshConfigWindow(struct Window *);
//-----
ULONG CompareListFormat(ListFormat *, ListFormat *);
//-----
void UpdateGadgetValue(ObjectList *, struct IntuiMessage *, UWORD);
void UpdateGadgetList(ObjectList *);
//-----
struct BitMap *NewBitMap(ULONG, ULONG, ULONG, ULONG, struct BitMap *);
void DisposeBitMap(struct BitMap *);
//-----
FuncArgs *ParseArgs(char *, char *);
void DisposeArgs(FuncArgs *);
//-----
void SetConfigWindowLimits(struct Window *, const ConfigWindow *, const ConfigWindow *);
//-----
void SetEnv(char *, char *, BOOL);
BOOL IsListLockEmpty(struct ListLock *);
//-----
DOpusAppMessage *AllocAppMessage(APTR, struct MsgPort *, short);
BOOL CheckAppMessage(DOpusAppMessage *);
DOpusAppMessage *CopyAppMessage(DOpusAppMessage *, APTR);
BOOL SetWBArg(DOpusAppMessage *, short, BPTR, char *, APTR);
//-----
BPTR OriginalCreateDir(char *);
long OriginalDeleteFile(char *);
BOOL OriginalSetFileDate(char *, struct DateStamp *);
BOOL OriginalSetComment(char *, char *);
BOOL OriginalSetProtection(char *, ULONG);
BOOL OriginalRename(char *, char *);
BPTR OriginalOpen(char *, LONG);
BOOL OriginalClose(BPTR);
LONG OriginalWrite(BPTR, void *, LONG);
//-----
struct Gadget *CreateTitleGadget(struct Screen *, struct List *, BOOL, short, short, UWORD);
struct Gadget *FindGadgetType(struct Gadget *, UWORD);
void FixTitleGadgets(struct Window *);
//-----
BOOL OriginalRelabel(char *, char *);
//-----
ILBMHandle *FakeILBM(UWORD *, ULONG *, short, short, short, ULONG);
//-----
IPTR IPC_SafeCommand(IPCData *, ULONG, IPTR, APTR, APTR, struct MsgPort *, struct ListLock *);
//-----
void ClearFiletypeCache(void);
//-----
struct Library *GetTimerBase(void);
//-----
BOOL InitDragDBuf(DragInfo *);
//-----
void FreeRexxMsgEx(struct RexxMsg *);
struct RexxMsg *CreateRexxMsgEx(struct MsgPort *, UBYTE *, UBYTE *);
long SetRexxVarEx(struct RexxMsg *, char *, char *, long);
long GetRexxVarEx(struct RexxMsg *, char *, char **);
struct RexxMsg *BuildRexxMsgEx(struct MsgPort *, UBYTE *, UBYTE *, struct TagItem *);
//-----
void NotifyDiskChange(void);
void GetDosListCopy(struct List *, APTR);
void FreeDosListCopy(struct List *);
//-----
BOOL DateFromStringsNew(char *, char *, struct DateStamp *, ULONG);
//-----
BOOL RemapIcon(struct DiskObject *, struct Screen *, short);
struct DiskObject *GetOriginalIcon(struct DiskObject *);
//-----
long CalcPercent(ULONG, ULONG, struct Library *);
//-----
BOOL IsDiskDevice(struct MsgPort *);
//-----
void DrawDragList(struct RastPort *, struct ViewPort *, long);
void RemoveDragImage(DragInfo *);
//-----
void SetNewIconsFlags(ULONG, short);
//-----
long ReadBufLine(APTR file, char *data, long size);
//-----
ULONG GetLibraryFlags(void);
//-----
short GetIconType(struct DiskObject *);
//-----
void SetReqBackFill(struct Hook *hook, struct Screen **);
struct Hook *LockReqBackFill(struct Screen *);
void UnlockReqBackFill(void);
//-----
BOOL DragCustomOk(struct BitMap *);
//-----
BOOL WB_LaunchNew(char *, struct Screen *, short, long, char *);
//-----
void UpdatePathList(void);
void UpdateMyPaths(void);
//-----
void GetPopUpImageSize(struct Window *, PopUpMenu *, short *, short *);
//-----
BOOL GetDeviceUnit(BPTR, char *, short *);
//-----
void StripWindowMessages(struct MsgPort *, struct IntuiMessage *);
//-----
BOOL DeleteIcon(char *);
//-----
void MUFSLogin(struct Window *, char *, char *);
//-----
//-----
void UpdateEnvironment(CFG_ENVR *);
//-----
void ConvertStartMenu(Cfg_ButtonBank *);
//-----
BPTR GetOpusPathList(void);
//-----
long GetStatistics(long id);
//-----
void SetPopUpDelay(short);
//-----
BOOL WB_LaunchNotify(char *, struct Screen *, short, long, char *, struct Process **, IPCData *, ULONG);
//-----
//-----
struct Window *WB_AppWindowWindow(struct AppWindow *);
//-----
BOOL OpenEnvironment(char *, struct OpenEnvironmentData *);
//-----
//-----
//-----
PopUpHandle *PopUpNewHandle(IPTR, REF_CALLBACK, struct DOpusLocale *);
void PopUpFreeHandle(PopUpHandle *);
PopUpItem *PopUpNewItem(PopUpHandle *, IPTR, ULONG, ULONG);
void PopUpSeparator(PopUpHandle *);
BOOL PopUpItemSub(PopUpHandle *, PopUpItem *);
void PopUpEndSub(PopUpHandle *);
ULONG PopUpSetFlags(PopUpMenu *, UWORD, ULONG, ULONG);
//-----
APTR AddAllocBitmapPatch(struct Task *, struct Screen *);
void RemAllocBitmapPatch(APTR);
//-----
BOOL LoadPos(char *, struct IBox *, short *);
BOOL SavePos(char *, struct IBox *, short);
//-----
void DivideU64(UQUAD *, ULONG, UQUAD *, UQUAD *);
void ItoaU64(UQUAD *, char *, int, char);
void DivideToString64(char *, int, UQUAD *, ULONG, int, char);
void BytesToString64(UQUAD *, char *, int, int, char);
BOOL ExamineLock64(BPTR, FileInfoBlock64 *);
BOOL ExamineNext64(BPTR, FileInfoBlock64 *);
BOOL ExamineHandle64(BPTR, FileInfoBlock64 *);
LONG MatchFirst64(STRPTR, struct AnchorPath *);
LONG MatchNext64(struct AnchorPath *);
//-----
//-----
//-----
//----- end "dopus.library" protos

#endif /* CLIB_DOPUS5_PROTOS_H */
