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
#ifndef _DOPUSLIB_H
#define _DOPUSLIB_H

#include "dopusbase.h"
#include "read_ilbm.h"
#include "images.h"
#include "iff.h"
#include "progress_win.h"  // for progress structs
#include "clipboard.h"	   // for clipboard's structs
#include "string_hook.h"   // for string_hook's structs

#ifdef __amigaos3__
	#include "dopus_inlines.h"
#elif defined(__MORPHOS__) || defined(__AROS__)
	#include <exec/execbase.h>
	#include <proto/alib.h>
#endif
#include <proto/Picasso96.h>
/* cybergraphics.library is kept as a fallback for CGX-only installs;
 * its header provides RECTFMT_* constants used by the fallback path in
 * read_ilbm.c. The OS3 SDK exposes the header at <cybergraphics/...>;
 * every other target uses the <cybergraphx/...> alias. */
#ifdef __amigaos3__
	#include <cybergraphics/cybergraphics.h>
#else
	#include <cybergraphx/cybergraphics.h>
#endif
#include <proto/cybergraphics.h>

// 64-bit types QUAD and UQUAD, duplicated from libraries/dopus5.h
#ifdef __amigaos4__
typedef uint64 UQUAD;
typedef int64 QUAD;
#elif defined(__amigaos3__)
typedef unsigned long long UQUAD;
typedef signed long long QUAD;
#endif

// 64-bit FIB
#if defined(__MORPHOS__) || defined(__AROS__)
typedef struct FileInfoBlock FileInfoBlock64;
#else
// added a _s suffix, since AROS might use struct FileInfoBlock64 in the future
typedef struct FileInfoBlock64_s
{
	LONG fib_DiskKey;
	LONG fib_DirEntryType;
	char fib_FileName[108];
	LONG fib_Protection;
	/*LONG             fib_EntryType;
	LONG             fib_Size;*/
	UQUAD fib_Size64;
	LONG fib_NumBlocks;
	struct DateStamp fib_Date;
	char fib_Comment[80];
	UWORD fib_OwnerUID;
	UWORD fib_OwnerGID;
	char fib_Reserved[32];
} FileInfoBlock64;
#endif

#if defined(__MORPHOS__) || defined(__AROS__)
	#include <proto/dopus5.h>
#endif

// ---------------------------------------------------------------
// Moved to Include/dopus/stack.h
//#define STACK_DEFAULT	6144
//#define STACK_LARGE	8192

// Locale
#define CATCOMP_NUMBERS
#include "string_data.h"

#ifndef __amigaos3__
	#pragma pack(2)
#endif

struct MyLibrary
{
	struct Library libBase;
	UWORD initialized;
	struct Library *sysBase;
	BPTR segList;
	struct SignalSemaphore libSem;
	APTR ml_UserData;
};

#if defined(__amigaos4__) || defined(__AROS__)
extern APTR dopuslibbase_global;
#endif

typedef struct	// Moved here from wb.h
{
	short patch_count;
	struct SignalSemaphore patch_lock;
	struct MinList app_list;
	// struct Library		*wb_base;
	// struct Library		*int_base;
	ULONG pad2[2];
	struct MyLibrary *dopus_base;
	APTR *old_function;

	UWORD lock_count;
	UWORD pad;

	// struct Library		*utility_base;
	ULONG pad3;
	struct MinList rem_app_list;
	ULONG pad1[3];
} WB_Data;

struct LibData
{
	WB_Data wb_data;
	char null_string[4];

	struct MinList notify_list;
	struct SignalSemaphore notify_lock;

	// struct Library		*dos_base;
	// struct Library		*icon_base;
	ULONG pad1[2];

	IPCData *launch_proc;
	struct MinList launch_list;
	struct SignalSemaphore launch_lock;
	ULONG launch_count;

	BPTR path_list;
	struct SignalSemaphore path_lock;

	// struct Library		*int_base;
	ULONG pad2;
	struct MinList error_list;
	ULONG error_wait;

	struct DOpusLocale locale;

	struct ListLock semaphores;

	APTR memory;

	struct ListLock device_list;
	ULONG flags;

	Class *dt_class;
	ULONG old_openlib;
	// struct MyLibrary	dt_lib;
	ULONG pad3;

	IPCData *fake_wb;
	struct MyLibrary *dopus_base;
	// struct Library		*gfx_base;
	ULONG pad4;

	struct ListLock file_list;
	BPTR last_file;

	struct ListLock filetype_cache;
	ULONG ft_cache_count;
	ULONG ft_cache_max;

	struct Library *TimerBase;
	struct IORequest timer_io;

	struct MinList dos_list;
	APTR dos_list_memory;
	struct SignalSemaphore dos_lock;

	// Pool for FIB+512 scratch buffers used by the patched DOS functions
	// (CreateDir, SetFileDate, SetComment, SetProtection, Rename) via
	// dospatch_fib().  Avoids hitting the OS heap on every patched call
	// when the LIBDF_DOS_PATCH flag is set.  See dos_patch.c.
	APTR dos_patch_memory;

	// struct Library		*new_icon_base;
	ULONG pad5;

	ULONG NewIconsFlags;
	short NewIconsPrecision;

	struct SignalSemaphore backfill_lock;
	struct Hook *backfill;
	struct Screen **backfill_screen;

	struct Interrupt low_mem_handler;
	short low_mem_signal;

	IPCData *launcher;

	long task_count;
	short popup_delay;

	struct Process *open_window_kludge;

	struct ListLock allocbitmap_patch;

#if defined(__MORPHOS__)
	struct EmulLibEntry low_mem_handler_trap;
#endif
};

#ifndef __amigaos3__
	#pragma pack()
#endif

#include "wb.h"
#include "filetypes.h"
#include "dos_patch.h"

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct
{
	struct MinNode node;
	struct MsgPort *port;
	ULONG userdata;
	ULONG type;
} NotifyNode;

typedef struct
{
	struct Node node;
	unsigned long flags;
	struct DosList *dos_ptr;
	struct MsgPort *dol_Task;
	char dol_Name[34];
} DeviceNode;

#define STRING_CONVERT_KEY 3000

#define DNF_OK (1 << 0)

extern struct TextFont *topaz_font;

extern struct List image_list;
extern struct SignalSemaphore image_lock;
extern APTR image_memory;
// extern APTR			chip_memory;

extern Class *listview_class, *image_class, *button_class, *palette_class, *string_class, *check_class, *view_class,
	*frame_class, *gauge_class;

extern char decimal_point;

extern IPCData *launcher_ipc;

#define GET_DOPUSLIB ((struct MyLibrary *)FindName(&((struct ExecBase *)SysBase)->LibList, "dopus5.library"))

#if defined(__amigaos3__)
extern ULONG GlobalStuffChar;
	#define DOPUS_RAWFMTFUNC (APTR) & GlobalStuffChar
#else
	#define DOPUS_RAWFMTFUNC NULL
#endif

void lsprintfa __ARGS((char *, ...));  // unused ??

#ifndef __amigaos3__
	#pragma pack()
#endif

#include "pools.h"

char *strstri(char *, char *);

void draw_field_box(struct RastPort *rp, struct Rectangle *rect, struct DrawInfo *info, struct Library *GfxBase);

BOOL rtg_bitmap(struct BitMap *bm);

void launcher_proc(void);

// datatypes
ULONG ASM dt_dispatch(REG(a0, Class *cl), REG(a2, Object *obj), REG(a1, Msg msg));
void dt_install_patch(struct LibData *data);
void dt_remove_patch(struct LibData *data);

//-----PROTOTYPES-----//

// anim.c
LIBPROTO(L_AnimDecodeRIFFXor,
		 void,
		 REG(a0, unsigned char *delta),
		 REG(a1, char *plane),
		 REG(d0, unsigned short rowbytes),
		 REG(d1, unsigned short sourcebytes));

LIBPROTO(L_AnimDecodeRIFFSet,
		 void,
		 REG(a0, unsigned char *delta),
		 REG(a1, char *plane),
		 REG(d0, unsigned short rowbytes),
		 REG(d1, unsigned short sourcebytes));

// appmsg.c
LIBPROTO(L_AllocAppMessage,
		 DOpusAppMessage *,
		 REG(a0, APTR memory),
		 REG(a1, struct MsgPort *reply),
		 REG(d0, short num));

LIBPROTO(L_FreeAppMessage, void, REG(a0, DOpusAppMessage *msg));

LIBPROTO(L_ReplyAppMessage, void, REG(a0, DOpusAppMessage *msg));

LIBPROTO(L_CheckAppMessage, BOOL, REG(a0, DOpusAppMessage *msg));

LIBPROTO(L_CopyAppMessage, DOpusAppMessage *, REG(a0, DOpusAppMessage *orig), REG(a1, APTR memory));

LIBPROTO(L_SetWBArg,
		 BOOL,
		 REG(a0, DOpusAppMessage *msg),
		 REG(d0, short num),
		 REG(d1, BPTR lock),
		 REG(a1, char *name),
		 REG(a2, APTR memory));

// args.c
LIBPROTO(L_ParseArgs, FuncArgs *, REG(a0, char *template), REG(a1, char *args));

LIBPROTO(L_DisposeArgs, void, REG(a0, FuncArgs *args));

// bitmap.c
LIBPROTO(L_NewBitMap,
		 struct BitMap *,
		 REG(d0, ULONG sizex),
		 REG(d1, ULONG sizey),
		 REG(d2, ULONG depth),
		 REG(d3, ULONG flags),
		 REG(a0, struct BitMap *friend_bitmap));

LIBPROTO(L_DisposeBitMap, void, REG(a0, struct BitMap *bm));

// boopsi.c
LIBPROTO(L_AddScrollBars,
		 struct Gadget *,
		 REG(a0, struct Window *window),
		 REG(a1, struct List *list),
		 REG(a2, struct DrawInfo *draw_info),
		 REG(d0, short noidcmpupdate));

LIBPROTO(L_FindBOOPSIGadget, struct Gadget *, REG(a0, struct List *list), REG(d0, UWORD id));

LIBPROTO(L_BOOPSIFree, void, REG(a0, struct List *list));

LIBPROTO(L_CreateTitleGadget,
		 struct Gadget *,
		 REG(a0, struct Screen *screen),
		 REG(a1, struct List *list),
		 REG(d0, BOOL cover_zoom),
		 REG(d1, short offset),
		 REG(d2, short type),
		 REG(d3, unsigned short id),
		 REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_FindGadgetType, struct Gadget *, REG(a0, struct Gadget *gadget), REG(d0, UWORD type));

LIBPROTO(L_FixTitleGadgets, void, REG(a0, struct Window *window));

// buffered_io.c
LIBPROTO(L_OpenBuf, APTR, REG(a0, char *name), REG(d0, long mode), REG(d1, long buffer_size));

LIBPROTO(L_CloseBuf, long, REG(a0, APTR file));

LIBPROTO(L_ReadBuf, long, REG(a0, APTR file), REG(a1, char *data), REG(d0, long size));

LIBPROTO(L_WriteBuf, long, REG(a0, APTR file), REG(a1, char *data), REG(d0, long size));

LIBPROTO(L_FlushBuf, long, REG(a0, APTR file));

LIBPROTO(L_SeekBuf, long, REG(a0, APTR file), REG(d0, long offset), REG(d1, long mode));

LIBPROTO(L_ExamineBuf, long, REG(a0, APTR file), REG(a1, struct FileInfoBlock *fib));

LIBPROTO(L_FHFromBuf, BPTR, REG(a0, APTR file));

LIBPROTO(L_ReadBufLine, long, REG(a0, APTR file), REG(a1, char *data), REG(d0, long size));

// clipboard.c
LIBPROTO(L_OpenClipBoard, ClipHandle *, REG(d0, ULONG unit));

LIBPROTO(L_CloseClipBoard, void, REG(a0, ClipHandle *clip));

LIBPROTO(L_WriteClipString, BOOL, REG(a0, ClipHandle *clip), REG(a1, char *string), REG(d0, long length));

LIBPROTO(L_ReadClipString, long, REG(a0, ClipHandle *clip), REG(a1, char *string), REG(d0, long length));

// config_close.c
LIBPROTO(L_CloseButtonBank, void, REG(a0, Cfg_ButtonBank *bank));

LIBPROTO(L_FreeListerDef, void, REG(a0, Cfg_Lister *lister));

LIBPROTO(L_FreeButtonList, void, REG(a0, struct List *list));

LIBPROTO(L_FreeButtonImages, void, REG(a0, struct List *list));

LIBPROTO(L_FreeButton, void, REG(a0, Cfg_Button *button));

LIBPROTO(L_FreeFunction, void, REG(a0, Cfg_Function *function));

LIBPROTO(L_FreeButtonFunction, void, REG(a0, Cfg_ButtonFunction *function));

LIBPROTO(L_FreeInstruction, void, REG(a0, Cfg_Instruction *ins));

LIBPROTO(L_FreeInstructionList, void, REG(a0, Cfg_Function *func));

LIBPROTO(L_FreeFiletypeList, void, REG(a0, Cfg_FiletypeList *list));

LIBPROTO(L_FreeFiletype, void, REG(a0, Cfg_Filetype *type));

// config_copy.c
LIBPROTO(L_CopyButtonBank, Cfg_ButtonBank *, REG(a0, Cfg_ButtonBank *orig));

LIBPROTO(L_CopyButton, Cfg_Button *, REG(a0, Cfg_Button *orig), REG(a1, APTR memory), REG(d0, short type));

LIBPROTO(L_CopyFiletype, Cfg_Filetype *, REG(a0, Cfg_Filetype *orig), REG(a1, APTR memory));

LIBPROTO(L_CopyFunction,
		 Cfg_Function *,
		 REG(a0, Cfg_Function *orig),
		 REG(a1, APTR memory),
		 REG(a2, Cfg_Function *newfunc));

LIBPROTO(L_CopyButtonFunction,
		 Cfg_ButtonFunction *,
		 REG(a0, Cfg_ButtonFunction *func),
		 REG(a1, APTR memory),
		 REG(a2, Cfg_ButtonFunction *newfunc));

// config_default.c
LIBPROTO(L_DefaultSettings, void, REG(a0, CFG_SETS *settings));

LIBPROTO(L_DefaultEnvironment, void, REG(a0, CFG_ENVR *env));

LIBPROTO(L_DefaultButtonBank, Cfg_ButtonBank *);

LIBPROTO(L_UpdateEnvironment, void, REG(a0, CFG_ENVR *env));

// config_misc.c
LIBPROTO(L_FindFunctionType, Cfg_Function *, REG(a0, struct List *list), REG(d0, UWORD type));

// config_new.c
LIBPROTO(L_NewLister, Cfg_Lister *, REG(a0, char *path));

LIBPROTO(L_NewButtonBank, Cfg_ButtonBank *, REG(d0, BOOL init), REG(d1, short type));

LIBPROTO(L_NewButton, Cfg_Button *, REG(a0, APTR memory));

LIBPROTO(L_NewFunction, Cfg_Function *, REG(a0, APTR memory), REG(d0, UWORD type));

LIBPROTO(L_NewButtonFunction, Cfg_ButtonFunction *, REG(a0, APTR memory), REG(d0, UWORD type));

LIBPROTO(L_NewInstruction, Cfg_Instruction *, REG(a0, APTR memory), REG(d0, short type), REG(a1, char *string));

LIBPROTO(L_NewFiletype, Cfg_Filetype *, REG(a0, APTR memory));

LIBPROTO(L_NewButtonWithFunc, Cfg_Button *, REG(a0, APTR memory), REG(a1, char *label), REG(d0, short type));

// config_open.c
LIBPROTO(L_OpenEnvironment, BOOL, REG(a0, char *name), REG(a1, struct OpenEnvironmentData *data));

LIBPROTO(L_ReadSettings, short, REG(a0, CFG_SETS *settings), REG(a1, char *name));

LIBPROTO(L_ReadListerDef, Cfg_Lister *, REG(a0, struct _IFFHandle *iff), REG(d0, ULONG id));

LIBPROTO(L_OpenButtonBank, Cfg_ButtonBank *, REG(a0, char *name));

LIBPROTO(L_ReadFiletypes, Cfg_FiletypeList *, REG(a0, char *name), REG(a1, APTR memory));

LIBPROTO(L_ReadButton, Cfg_Button *, REG(a0, struct _IFFHandle *iff), REG(a1, APTR memory));

LIBPROTO(L_ReadFunction,
		 Cfg_Function *,
		 REG(a0, struct _IFFHandle *iff),
		 REG(a1, APTR memory),
		 REG(a2, struct List *func_list),
		 REG(a3, Cfg_Function *function));

LIBPROTO(L_ConvertStartMenu, void, REG(a0, Cfg_ButtonBank *bank));

// config_save.c
LIBPROTO(L_SaveListerDef, long, REG(a0, struct _IFFHandle *iff), REG(a1, Cfg_Lister *lister));

LIBPROTO(L_SaveButton, short, REG(a0, struct _IFFHandle *iff), REG(a1, Cfg_Button *button));

LIBPROTO(L_SaveFunction, BOOL, REG(a0, struct _IFFHandle *iff), REG(a1, Cfg_Function *function));

LIBPROTO(L_SaveFiletypeList,
		 int,
		 REG(a0, Cfg_FiletypeList *list),
		 REG(a1, char *name),
		 REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_SaveSettings, int, REG(a0, CFG_SETS *settings), REG(a1, char *name), REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_SaveButtonBank, int, REG(a0, Cfg_ButtonBank *bank), REG(a1, char *name), REG(a6, struct MyLibrary *libbase));

// dates.c
LIBPROTO(L_ParseDateStrings,
		 char *,
		 REG(a0, char *string),
		 REG(a1, char *date_buffer),
		 REG(a2, char *time_buffer),
		 REG(a3, long *range));

LIBPROTO(L_DateFromStrings, BOOL, REG(a0, char *date), REG(a1, char *time), REG(a2, struct DateStamp *ds));

LIBPROTO(L_DateFromStringsNew,
		 BOOL,
		 REG(a0, char *date),
		 REG(a1, char *time),
		 REG(a2, struct DateStamp *ds),
		 REG(d0, ULONG method));

// devices.c
LIBPROTO(L_DeviceFromLock,
		 struct DosList *,
		 REG(a0, BPTR lock),
		 REG(a1, char *name),
		 REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_DeviceFromHandler,
		 struct DosList *,
		 REG(a0, struct MsgPort *port),
		 REG(a1, char *name),
		 REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_DevNameFromLockDopus,
		 BOOL,
		 REG(d1, BPTR lock),
		 REG(d2, char *buffer),
		 REG(d3, long len),
		 REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_GetDeviceUnit, BOOL, REG(a0, BPTR dol_Startup), REG(a1, char *device), REG(a2, short *unit));

LIBPROTO(L_IsDiskDevice, BOOL, REG(a0, struct MsgPort *port));

// diskio
LIBPROTO(L_OpenDisk, DiskHandle *, REG(a0, char *disk), REG(a1, struct MsgPort *port));

LIBPROTO(L_CloseDisk, void, REG(a0, DiskHandle *handle));

// dos_patch.c
#if defined(__MORPHOS__)
BPTR L_PatchedCreateDir(void);
long L_PatchedDeleteFile(void);
BOOL L_PatchedSetFileDate(void);
BOOL L_PatchedSetComment(void);
BOOL L_PatchedSetProtection(void);
BOOL L_PatchedRename(void);
BPTR L_PatchedOpen(void);
BOOL L_PatchedClose(void);
LONG L_PatchedWrite(void);
BOOL L_PatchedRelabel(void);
#else
LIBPROTO(L_PatchedCreateDir, BPTR, REG(d1, char *name));
LIBPROTO(L_PatchedDeleteFile, long, REG(d1, char *name));
LIBPROTO(L_PatchedSetFileDate, BOOL, REG(d1, char *name), REG(d2, struct DateStamp *date));
LIBPROTO(L_PatchedSetComment, BOOL, REG(d1, char *name), REG(d2, char *comment));
LIBPROTO(L_PatchedSetProtection, BOOL, REG(d1, char *name), REG(d2, ULONG mask));
LIBPROTO(L_PatchedRename, BOOL, REG(d1, char *oldname), REG(d2, char *newname));
LIBPROTO(L_PatchedOpen, BPTR, REG(d1, char *name), REG(d2, LONG accessMode));
LIBPROTO(L_PatchedClose, BOOL, REG(d1, BPTR file));
LIBPROTO(L_PatchedWrite, LONG, REG(d1, BPTR file), REG(d2, void *data), REG(d3, LONG length));
LIBPROTO(L_PatchedRelabel, BOOL, REG(d1, char *volumename), REG(d2, char *name));
#endif

LIBPROTO(L_OriginalCreateDir, BPTR, REG(d1, char *name), REG(a6, struct MyLibrary *libbase));
LIBPROTO(L_OriginalDeleteFile, long, REG(d1, char *name), REG(a6, struct MyLibrary *libbase));
LIBPROTO(L_OriginalSetFileDate,
		 BOOL,
		 REG(d1, char *name),
		 REG(d2, struct DateStamp *date),
		 REG(a6, struct MyLibrary *libbase));
LIBPROTO(L_OriginalSetComment, BOOL, REG(d1, char *name), REG(d2, char *comment), REG(a6, struct MyLibrary *libbase));
LIBPROTO(L_OriginalSetProtection, BOOL, REG(d1, char *name), REG(d2, ULONG mask), REG(a6, struct MyLibrary *libbase));
LIBPROTO(L_OriginalRename, BOOL, REG(d1, char *oldname), REG(d2, char *newname), REG(a6, struct MyLibrary *libbase));
LIBPROTO(L_OriginalOpen, BPTR, REG(d1, char *name), REG(d2, LONG accessMode), REG(a6, struct MyLibrary *libbase));
LIBPROTO(L_OriginalClose, BOOL, REG(d1, BPTR file), REG(a6, struct MyLibrary *libbase));
LIBPROTO(L_OriginalWrite,
		 LONG,
		 REG(d1, BPTR file),
		 REG(d2, void *data),
		 REG(d3, LONG length),
		 REG(a6, struct MyLibrary *libbase));
LIBPROTO(L_OriginalRelabel, BOOL, REG(d1, char *volumename), REG(d2, char *name), REG(a6, struct MyLibrary *libbase));

// drag_routines.c
LIBPROTO(L_GetDragInfo,
		 DragInfo *,
		 REG(a0, struct Window *window),
		 REG(a1, struct RastPort *rast),
		 REG(d0, long width),
		 REG(d1, long height),
		 REG(d2, long flags),
		 REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_FreeDragInfo, void, REG(a0, DragInfo *drag));

LIBPROTO(L_HideDragImage, void, REG(a0, DragInfo *drag));

LIBPROTO(L_GetDragImage, void, REG(a0, DragInfo *drag), REG(d0, ULONG x), REG(d1, ULONG y));

LIBPROTO(L_GetDragMask, void, REG(a0, DragInfo *drag));

LIBPROTO(L_ShowDragImage, void, REG(a0, DragInfo *drag), REG(d0, ULONG x), REG(d1, ULONG y));

LIBPROTO(L_AddDragImage, void, REG(a0, DragInfo *drag));

LIBPROTO(L_RemDragImage, void, REG(a0, DragInfo *drag));

LIBPROTO(L_StampDragImage, void, REG(a0, DragInfo *drag), REG(d0, ULONG x), REG(d1, ULONG y));

LIBPROTO(L_CheckDragDeadlock, BOOL, REG(a0, DragInfo *drag));

LIBPROTO(L_BuildTransDragMask,
		 BOOL,
		 REG(a0, UWORD *mask),
		 REG(a1, UWORD *image),
		 REG(d0, short width),
		 REG(d1, short height),
		 REG(d2, short depth),
		 REG(d3, long flags));

LIBPROTO(L_DrawDragList, void, REG(a0, struct RastPort *rp), REG(a1, struct ViewPort *vp), REG(d0, long flags));

LIBPROTO(L_RemoveDragImage, void, REG(a0, DragInfo *drag));

LIBPROTO(L_DragCustomOk, BOOL, REG(a0, struct BitMap *bm), REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_InitDragDBuf, BOOL, REG(a0, DragInfo *drag));

// filetypes.c
LIBPROTO(L_GetMatchHandle, APTR, REG(a0, char *name), REG(a6, struct MyLibrary *lib));

LIBPROTO(L_FreeMatchHandle, void, REG(a0, MatchHandle *handle));

LIBPROTO(L_MatchFiletype,
		 BOOL,
		 REG(a0, MatchHandle *handle),
		 REG(a1, Cfg_Filetype *type),
		 REG(a6, struct MyLibrary *lib));

LIBPROTO(L_ClearFiletypeCache, void, REG(a6, struct MyLibrary *lib));

// functions.c (few of them was on ASM):
LIBPROTO(L_DivideU,
		 ULONG,
		 REG(d0, ULONG num),
		 REG(d1, ULONG div),
		 REG(a0, ULONG *rem),
		 REG(a1, struct Library *lib));

LIBPROTO(L_BtoCStr, void, REG(a0, BSTR bstr), REG(a1, char *cstr), REG(d0, int len));

LIBPROTO(L_Seed, void, REG(d0, int seed));

LIBPROTO(L_RandomDopus, void, REG(d0, int limit));

// gui.c
LIBPROTO(L_DrawBox,
		 void,
		 REG(a0, struct RastPort *rp),
		 REG(a1, struct Rectangle *rect),
		 REG(a2, struct DrawInfo *info),
		 REG(d0, BOOL recessed));

LIBPROTO(L_DrawFieldBox,
		 void,
		 REG(a0, struct RastPort *rp),
		 REG(a1, struct Rectangle *rect),
		 REG(a2, struct DrawInfo *info));

LIBPROTO(L_WriteIcon,
		 BOOL,
		 REG(a0, char *name),
		 REG(a1, struct DiskObject *diskobj),
		 REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_DeleteIcon, BOOL, REG(a0, char *name), REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_ScreenInfo, ULONG, REG(a0, struct Screen *screen));

// icon_cache.c
LIBPROTO(L_GetCachedDefDiskObject, struct DiskObject *, REG(d0, long type), REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_FreeCachedDiskObject, void, REG(a0, struct DiskObject *icon), REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_GetCachedDiskObject,
		 struct DiskObject *,
		 REG(a0, char *name),
		 REG(d0, ULONG flags),
		 REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_GetCachedDiskObjectNew,
		 struct DiskObject *,
		 REG(a0, char *name),
		 REG(d0, ULONG flags),
		 REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_IconCheckSum, unsigned long, REG(a0, struct DiskObject *icon), REG(d0, short which));

LIBPROTO(L_GetIconType, short, REG(a0, struct DiskObject *icon));

LIBPROTO(L_SetNewIconsFlags, void, REG(d0, ULONG flags), REG(d1, short precision), REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_GetOriginalIcon, struct DiskObject *, REG(a0, struct DiskObject *icon));

LIBPROTO(L_RemapIcon,
		 BOOL,
		 REG(a0, struct DiskObject *icon),
		 REG(a1, struct Screen *screen),
		 REG(d0, short free_remap));

// icons.c
LIBPROTO(L_GetIconFlags, ULONG, REG(a0, struct DiskObject *icon));

LIBPROTO(L_SetIconFlags, void, REG(a0, struct DiskObject *icon), REG(d0, ULONG flags));

LIBPROTO(L_GetIconPosition, void, REG(a0, struct DiskObject *icon), REG(a1, short *x), REG(a2, short *y));

LIBPROTO(L_SetIconPosition, void, REG(a0, struct DiskObject *icon), REG(d0, short x), REG(d1, short y));

// iff.c
LIBPROTO(L_IFFOpen, struct _IFFHandle *, REG(a0, char *name), REG(d0, unsigned short mode), REG(d1, ULONG form));

LIBPROTO(L_IFFClose, void, REG(a0, struct _IFFHandle *handle));

LIBPROTO(L_IFFPushChunk, long, REG(a0, struct _IFFHandle *handle), REG(d0, ULONG id));

LIBPROTO(L_IFFWriteChunkBytes, long, REG(a0, struct _IFFHandle *handle), REG(a1, char *data), REG(d0, long size));

LIBPROTO(L_IFFPopChunk, long, REG(a0, struct _IFFHandle *handle));

LIBPROTO(L_IFFWriteChunk,
		 long,
		 REG(a0, struct _IFFHandle *handle),
		 REG(a1, char *data),
		 REG(d0, ULONG chunk),
		 REG(d1, ULONG size));

LIBPROTO(L_IFFNextChunk, unsigned long, REG(a0, struct _IFFHandle *handle), REG(d0, unsigned long form));

LIBPROTO(L_IFFChunkSize, long, REG(a0, struct _IFFHandle *handle));

LIBPROTO(L_IFFReadChunkBytes, long, REG(a0, struct _IFFHandle *handle), REG(a1, APTR buffer), REG(d0, long size));

LIBPROTO(L_IFFFileHandle, APTR, REG(a0, struct _IFFHandle *handle));

LIBPROTO(L_IFFChunkRemain, long, REG(a0, struct _IFFHandle *handle));

LIBPROTO(L_IFFChunkID, unsigned long, REG(a0, struct _IFFHandle *handle));

LIBPROTO(L_IFFGetFORM, unsigned long, REG(a0, struct _IFFHandle *handle));

LIBPROTO(L_IFFFailure, void, REG(a0, IFFHandle *iff));

// images.c
LIBPROTO(L_OpenImage, APTR, REG(a0, char *name), REG(a1, OpenImageInfo *info));

LIBPROTO(L_CloseImage, void, REG(a0, APTR image));

LIBPROTO(L_CopyImage, APTR, REG(a0, APTR image));

LIBPROTO(L_FlushImages, void);

LIBPROTO(L_RenderImage,
		 short,
		 REG(a0, struct RastPort *rp),
		 REG(a1, APTR image),
		 REG(d0, unsigned short left),
		 REG(d1, unsigned short top),
		 REG(a2, struct TagItem *tags));

LIBPROTO(L_GetImageAttrs, void, REG(a0, APTR imptr), REG(a1, struct TagItem *tags));

LIBPROTO(L_RemapImage, BOOL, REG(a0, Image_Data *image), REG(a1, struct Screen *screen), REG(a2, ImageRemap *remap));

LIBPROTO(L_FreeRemapImage, void, REG(a0, Image_Data *image), REG(a1, ImageRemap *remap));

LIBPROTO(L_FreeImageRemap, void, REG(a0, ImageRemap *remap));

LIBPROTO(L_GetImagePalette, ULONG *, REG(a0, APTR ptr));

// ipc.c
LIBPROTO(L_IPC_Launch,
		 int,
		 REG(a0, struct ListLock *list),
		 REG(a1, IPCData **storage),
		 REG(a2, char *name),
		 REG(d0, IPTR entry),
		 REG(d1, ULONG stack),
		 REG(d2, IPTR data),
		 REG(a3, struct Library *dos_base),
		 REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_IPC_Startup, int, REG(a0, IPCData *ipc), REG(a1, APTR data), REG(a2, struct MsgPort *reply));

LIBPROTO(L_IPC_Command,
		 IPTR,
		 REG(a0, IPCData *ipc),
		 REG(d0, ULONG command),
		 REG(d1, IPTR flags),
		 REG(a1, APTR data),
		 REG(a2, APTR data_free),
		 REG(a3, struct MsgPort *reply));

LIBPROTO(L_IPC_Reply, void, REG(a0, IPCMessage *msg));

LIBPROTO(L_IPC_Free, void, REG(a0, IPCData *ipc));

LIBPROTO(L_IPC_FindProc,
		 IPCData *,
		 REG(a0, struct ListLock *list),
		 REG(a1, char *name),
		 REG(d0, BOOL activate),
		 REG(d1, IPTR data));

LIBPROTO(L_IPC_ProcStartup,
		 IPCData *,
		 REG(a0, IPTR *data),
		 REG(a1, ULONG (*ASM code)(REG(a0, IPCData *), REG(a1, APTR))));

LIBPROTO(L_IPC_Quit, void, REG(a0, IPCData *ipc), REG(d0, ULONG quit_flags), REG(d1, BOOL wait));

LIBPROTO(L_IPC_Hello, void, REG(a0, IPCData *ipc), REG(a1, IPCData *owner));

LIBPROTO(L_IPC_Goodbye, void, REG(a0, IPCData *ipc), REG(a1, IPCData *owner), REG(d0, ULONG goodbye_flags));

LIBPROTO(L_IPC_GetGoodbye, IPTR, REG(a0, IPCMessage *msg));

LIBPROTO(L_IPC_ListQuit,
		 ULONG,
		 REG(a0, struct ListLock *list),
		 REG(a1, IPCData *owner),
		 REG(d0, ULONG quit_flags),
		 REG(d1, BOOL wait));

LIBPROTO(L_IPC_Flush, void, REG(a0, IPCData *ipc));

LIBPROTO(L_IPC_ListCommand,
		 void,
		 REG(a0, struct ListLock *list),
		 REG(d0, ULONG command),
		 REG(d1, IPTR flags),
		 REG(d2, IPTR data),
		 REG(d3, BOOL wait));

LIBPROTO(L_IPC_QuitName, void, REG(a0, struct ListLock *list), REG(a1, char *name), REG(d0, ULONG quit_flags));

LIBPROTO(L_IPC_SafeCommand,
		 IPTR,
		 REG(a0, IPCData *ipc),
		 REG(d0, ULONG command),
		 REG(d1, IPTR flags),
		 REG(a1, APTR data),
		 REG(a2, APTR data_free),
		 REG(a3, struct MsgPort *reply),
		 REG(a4, struct ListLock *list));

// launcher.c
LIBPROTO(L_WB_LaunchNotify,
		 BOOL,
		 REG(a0, char *name),
		 REG(a1, struct Screen *errors),
		 REG(d0, short wait),
		 REG(d1, long stack),
		 REG(a2, char *default_tool),
		 REG(a3, struct Process **proc_ptr),
		 REG(a4, IPCData *notify_ipc),
		 REG(d2, ULONG flags));

LIBPROTO(L_MUFSLogin, void, REG(a0, struct Window *window), REG(a1, char *name), REG(a2, char *password));

LIBPROTO(L_WB_Launch, BOOL, REG(a0, char *name), REG(a1, struct Screen *errors), REG(d0, short wait));

LIBPROTO(L_WB_LaunchNew,
		 BOOL,
		 REG(a0, char *name),
		 REG(a1, struct Screen *errors),
		 REG(d0, short wait),
		 REG(d1, long stack),
		 REG(a2, char *default_tool));

LIBPROTO(L_FreeDosListCopy, void, REG(a0, struct List *list), REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_GetDosListCopy, void, REG(a0, struct List *list), REG(a1, APTR memory), REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_NotifyDiskChange, void);

LIBPROTO(L_CLI_Launch,
		 BOOL,
		 REG(a0, char *name),
		 REG(a1, struct Screen *errors),
		 REG(d0, BPTR currentdir),
		 REG(d1, BPTR input),
		 REG(d2, BPTR output),
		 REG(d3, short wait),
		 REG(d4, long stack));

// layout_resize.c
LIBPROTO(L_LayoutResize, void, REG(a0, struct Window *window));

LIBPROTO(L_SetConfigWindowLimits,
		 void,
		 REG(a0, struct Window *window),
		 REG(a1, ConfigWindow *mindims),
		 REG(a2, ConfigWindow *maxdims));

// layout_routines.c
LIBPROTO(L_OpenConfigWindow, struct Window *, REG(a0, NewConfigWindow *newwindow), REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_CloseConfigWindow, void, REG(a0, struct Window *window), REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_CalcObjectDims,
		 int,
		 REG(a0, void *parent),
		 REG(a1, struct TextFont *use_font),
		 REG(a2, struct IBox *pos),
		 REG(a3, struct IBox *dest_pos),
		 REG(a4, GL_Object *last_ob),
		 REG(d0, ULONG flags),
		 REG(d1, GL_Object *this_ob),
		 REG(d2, GL_Object *parent_ob));

LIBPROTO(L_CalcWindowDims,
		 int,
		 REG(a0, struct Screen *screen),
		 REG(a1, ConfigWindow *win_pos),
		 REG(a2, struct IBox *dest_pos),
		 REG(a3, struct TextFont *font),
		 REG(d0, ULONG flags));

LIBPROTO(L_AddObjectList,
		 ObjectList *,
		 REG(a0, struct Window *window),
		 REG(a1, ObjectDef *objects),
		 REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_FreeObject, void, REG(a0, ObjectList *objlist), REG(a1, GL_Object *object));

LIBPROTO(L_FreeObjectList, void, REG(a0, ObjectList *objlist));

LIBPROTO(L_GetObject, GL_Object *, REG(a0, ObjectList *list), REG(d0, int id));

LIBPROTO(L_DisplayObject,
		 void,
		 REG(a0, struct Window *window),
		 REG(a1, GL_Object *object),
		 REG(d0, int fg),
		 REG(d1, int bg),
		 REG(a2, char *txt));

LIBPROTO(L_AddWindowMenus, void, REG(a0, struct Window *window), REG(a1, MenuData *menudata));

LIBPROTO(L_FreeWindowMenus, void, REG(a0, struct Window *window));

LIBPROTO(L_RefreshObjectList, void, REG(a0, struct Window *window), REG(a1, ObjectList *ref_list));

// layout_support.c
LIBPROTO(L_StripIntuiMessagesDopus, void, REG(a0, struct Window *window));

LIBPROTO(L_StripWindowMessages, void, REG(a0, struct MsgPort *port), REG(a1, struct IntuiMessage *except));

LIBPROTO(L_CloseWindowSafely, void, REG(a0, struct Window *window));

LIBPROTO(L_StoreGadgetValue,
		 void,
		 REG(a0, ObjectList *list),
		 REG(a1, struct IntuiMessage *msg),
		 REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_UpdateGadgetValue,
		 void,
		 REG(a0, ObjectList *list),
		 REG(a1, struct IntuiMessage *msg),
		 REG(d0, UWORD id),
		 REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_SetGadgetValue, void, REG(a0, ObjectList *list), REG(d0, UWORD id), REG(d1, IPTR value));

LIBPROTO(L_GetGadgetValue, IPTR, REG(a0, ObjectList *list), REG(a1, UWORD id), REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_CheckObjectArea, BOOL, REG(a0, GL_Object *object), REG(d0, int x), REG(d1, int y));

LIBPROTO(L_SetWindowBusy, void, REG(a0, struct Window *window));

LIBPROTO(L_ClearWindowBusy, void, REG(a0, struct Window *window));

LIBPROTO(L_GetString, STRPTR, REG(a0, struct DOpusLocale *li), REG(d0, LONG stringNum));

LIBPROTO(L_FindKeyEquivalent,
		 struct Gadget *,
		 REG(a0, ObjectList *list),
		 REG(a1, struct IntuiMessage *msg),
		 REG(d0, int process));

LIBPROTO(L_ShowProgressBar,
		 void,
		 REG(a0, struct Window *window),
		 REG(a1, GL_Object *object),
		 REG(d0, ULONG total),
		 REG(d1, ULONG count));

LIBPROTO(L_SetObjectKind, void, REG(a0, ObjectList *list), REG(d0, ULONG id), REG(d1, UWORD kind));

LIBPROTO(L_DisableObject, void, REG(a0, ObjectList *list), REG(d0, ULONG id), REG(d1, BOOL state));

LIBPROTO(L_BoundsCheckGadget, int, REG(a0, ObjectList *list), REG(d0, ULONG id), REG(d1, int min), REG(d2, int max));

LIBPROTO(L_GetWindowMsg, struct IntuiMessage *, REG(a0, struct MsgPort *port), REG(a6, struct MyLibrary *lib));

LIBPROTO(L_ReplyWindowMsg, void, REG(a0, struct IntuiMessage *msg));

LIBPROTO(L_SetGadgetChoices, void, REG(a0, ObjectList *list), REG(d0, ULONG id), REG(a1, APTR choices));

LIBPROTO(L_SetWindowID,
		 void,
		 REG(a0, struct Window *window),
		 REG(a1, WindowID *id),
		 REG(d0, ULONG window_id),
		 REG(a2, struct MsgPort *port));

LIBPROTO(L_GetWindowID, ULONG, REG(a0, struct Window *window));

LIBPROTO(L_GetWindowAppPort, struct MsgPort *, REG(a0, struct Window *window));

LIBPROTO(L_GetObjectRect, BOOL, REG(a0, ObjectList *list), REG(d0, ULONG id), REG(a1, struct Rectangle *rect));

LIBPROTO(L_StartRefreshConfigWindow, void, REG(a0, struct Window *window), REG(d0, long final_state));

LIBPROTO(L_EndRefreshConfigWindow, void, REG(a0, struct Window *window));

LIBPROTO(L_UpdateGadgetList, void, REG(a0, ObjectList *list), REG(a6, struct MyLibrary *libbase));

// layout_utils.c
LIBPROTO(L_InitWindowDims, void, REG(a0, struct Window *window), REG(a1, WindowDimensions *dims));

LIBPROTO(L_StoreWindowDims, void, REG(a0, struct Window *window), REG(a1, WindowDimensions *dims));

LIBPROTO(L_CheckWindowDims, BOOL, REG(a0, struct Window *window), REG(a1, WindowDimensions *dims));

// list_management.c
LIBPROTO(L_Att_NewList, Att_List *, REG(d0, ULONG flags));

LIBPROTO(L_Att_NewNode,
		 Att_Node *,
		 REG(a0, Att_List *list),
		 REG(a1, char *name),
		 REG(d0, IPTR data),
		 REG(d1, ULONG flags));

LIBPROTO(L_Att_RemNode, void, REG(a0, Att_Node *node));

LIBPROTO(L_Att_PosNode, void, REG(a0, Att_List *list), REG(a1, Att_Node *node), REG(a2, Att_Node *before));

LIBPROTO(L_Att_RemList, void, REG(a0, Att_List *list), REG(d0, long flags));

LIBPROTO(L_Att_FindNode, Att_Node *, REG(a0, Att_List *list), REG(d0, long number));

LIBPROTO(L_Att_NodeNumber, long, REG(a0, Att_List *list), REG(a1, char *name));

LIBPROTO(L_Att_FindNodeData, Att_Node *, REG(a0, Att_List *list), REG(d0, IPTR data));

LIBPROTO(L_Att_NodeDataNumber, long, REG(a0, Att_List *list), REG(d0, IPTR data));

LIBPROTO(L_Att_NodeName, char *, REG(a0, Att_List *list), REG(d0, long number));

LIBPROTO(L_Att_NodeCount, long, REG(a0, Att_List *list));

LIBPROTO(L_Att_ChangeNodeName, void, REG(a0, Att_Node *node), REG(a1, char *name));

LIBPROTO(L_Att_FindNodeNumber, long, REG(a0, Att_List *list), REG(a1, Att_Node *node));

LIBPROTO(L_AddSorted, void, REG(a0, struct List *list), REG(a1, struct Node *node));

LIBPROTO(L_FindNameI, struct Node *, REG(a0, struct List *list), REG(a1, char *name));

LIBPROTO(L_LockAttList, void, REG(a0, Att_List *list), REG(d0, short exclusive));

LIBPROTO(L_UnlockAttList, void, REG(a0, Att_List *list));

LIBPROTO(L_SwapListNodes, void, REG(a0, struct List *list), REG(a1, struct Node *swap1), REG(a2, struct Node *swap2));

LIBPROTO(L_IsListLockEmpty, BOOL, REG(a0, struct ListLock *list));

// memory.c
LIBPROTO(L_NewMemHandle, void *, REG(d0, ULONG puddle_size), REG(d1, ULONG thresh_size), REG(d2, ULONG type));

LIBPROTO(L_FreeMemHandle, void, REG(a0, MemHandle *handle));

LIBPROTO(L_ClearMemHandle, void, REG(a0, MemHandle *handle));

LIBPROTO(L_AllocMemH, void *, REG(a0, MemHandle *handle), REG(d0, ULONG size));

LIBPROTO(L_FreeMemH, void, REG(a0, void *memory));

// menu_routines.c
LIBPROTO(L_BuildMenuStrip, struct Menu *, REG(a0, MenuData *menudata), REG(a1, struct DOpusLocale *locale));

LIBPROTO(L_FindMenuItem, struct MenuItem *, REG(a0, struct Menu *menu), REG(d0, UWORD id));

// misc.c
LIBPROTO(L_BuildKeyString,
		 void,
		 REG(d0, unsigned short code),
		 REG(d1, unsigned short qual),
		 REG(d2, unsigned short qual_mask),
		 REG(d3, unsigned short qual_same),
		 REG(a0, char *buffer));

LIBPROTO(L_QualValid, UWORD, REG(d0, unsigned short qual));

LIBPROTO(L_ConvertRawKey, BOOL, REG(d0, UWORD code), REG(d1, UWORD qual), REG(a0, char *key));

LIBPROTO(L_SetBusyPointer, void, REG(a0, struct Window *wind));

LIBPROTO(L_ActivateStrGad, void, REG(a0, struct Gadget *gad), REG(a1, struct Window *win));
/*
LIBPROTO(L_RefreshStrGad, void,	REG(a0, struct Gadget *gad),
								REG(a1, struct Window *win));
*/

LIBPROTO(L_Itoa, void, REG(d0, long num), REG(a0, char *str), REG(d1, char sep));

LIBPROTO(L_ItoaU, void, REG(d0, unsigned long num), REG(a0, char *str), REG(d1, char sep));

LIBPROTO(L_BytesToString,
		 void,
		 REG(d0, unsigned long bytes),
		 REG(a0, char *string),
		 REG(d1, short places),
		 REG(d2, char sep));

LIBPROTO(L_DivideToString,
		 void,
		 REG(a0, char *string),
		 REG(d0, unsigned long bytes),
		 REG(d1, unsigned long div),
		 REG(d2, short places),
		 REG(d3, char sep));

LIBPROTO(L_Ito26, void, REG(d0, unsigned long num), REG(a0, char *str));

LIBPROTO(L_SerialValid, BOOL, REG(a0, serial_data *data));

LIBPROTO(L_WriteFileIcon, void, REG(a0, char *source), REG(a1, char *dest), REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_GetWBArgPath,
		 BOOL,
		 REG(a0, struct WBArg *arg),
		 REG(a1, char *buffer),
		 REG(d0, long size),
		 REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_FindPubScreen, struct PubScreenNode *, REG(a0, struct Screen *screen), REG(d0, BOOL lock));

LIBPROTO(L_SetEnv, void, REG(a0, char *name), REG(a1, char *data), REG(d0, BOOL save));

LIBPROTO(L_GetStatistics, long, REG(d0, long id), REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_GetLibraryFlags, ULONG, REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_CompareListFormat, ULONG, REG(a0, ListFormat *format1), REG(a1, ListFormat *format2));

LIBPROTO(L_SetLibraryFlags, ULONG, REG(d0, ULONG flags), REG(d1, ULONG mask), REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_SerialCheck, BOOL, REG(a0, char *sernum), REG(a1, ULONG *nullp));

LIBPROTO(L_ChecksumFile, ULONG, REG(a0, char *filename), REG(d0, ULONG skip_marker));

LIBPROTO(L_ReplyFreeMsg, void, REG(a0, struct Message *msg));

LIBPROTO(L_OpenIFFFile, struct IFFHandle *, REG(a0, char *name), REG(d0, int mode), REG(d1, ULONG check_id));
LIBPROTO(L_CloseIFFFile, void, REG(a0, struct IFFHandle *iff));

LIBPROTO(L_SetReqBackFill,
		 void,
		 REG(a0, struct Hook *hook),
		 REG(a1, struct Screen **screen),
		 REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_LockReqBackFill, struct Hook *, REG(a0, struct Screen *screen), REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_UnlockReqBackFill, void, REG(a6, struct MyLibrary *libbase));

// notify.c
LIBPROTO(L_AddNotifyRequest,
		 APTR,
		 REG(d0, ULONG type),
		 REG(d1, ULONG userdata),
		 REG(a0, struct MsgPort *port),
		 REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_RemoveNotifyRequest, void, REG(a0, NotifyNode *node), REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_SendNotifyMsg,
		 void,
		 REG(d0, ULONG type),
		 REG(d1, IPTR data),
		 REG(d2, ULONG flags),
		 REG(d3, short wait),
		 REG(a0, char *name),
		 REG(a1, struct FileInfoBlock *fib),
		 REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_SetNotifyRequest,
		 void,
		 REG(a0, NotifyNode *node),
		 REG(d0, ULONG new_flags),
		 REG(d1, ULONG mask),
		 REG(a6, struct MyLibrary *libbase));

// palette_routines.c
LIBPROTO(L_LoadPalette32, void, REG(a0, struct ViewPort *vp), REG(a1, ULONG *palette));

LIBPROTO(L_GetPalette32,
		 void,
		 REG(a0, struct ViewPort *vp),
		 REG(a1, ULONG *palette),
		 REG(d0, unsigned short count),
		 REG(d1, short first));

// pathlist.c
LIBPROTO(L_GetDosPathList, BPTR, REG(a0, BPTR copy_list));

LIBPROTO(L_FreeDosPathList, void, REG(a0, BPTR list));

LIBPROTO(L_CopyLocalEnv, void, REG(a0, struct Library *DOSBase));

LIBPROTO(L_GetOpusPathList, BPTR, REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_UpdatePathList, void, REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_UpdateMyPaths, void, REG(a6, struct MyLibrary *libbase));

// popup_menus.c
LIBPROTO(L_DoPopUpMenu,
		 UWORD,
		 REG(a0, struct Window *window),
		 REG(a1, PopUpMenu *menu),
		 REG(a2, PopUpItem **sel_item),
		 REG(d0, UWORD code),
		 REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_GetPopUpItem, PopUpItem *, REG(a0, PopUpMenu *menu), REG(d0, UWORD id));

LIBPROTO(L_SetPopUpDelay, void, REG(d0, short delay), REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_GetPopUpImageSize,
		 void,
		 REG(a0, struct Window *window),
		 REG(a1, PopUpMenu *menu),
		 REG(a2, short *width),
		 REG(a3, short *height));

// popup_support.c
LIBPROTO(L_PopUpNewHandle,
		 PopUpHandle *,
		 REG(d0, IPTR userdata),
		 REG(a0, REF_CALLBACK callback),
		 REG(a1, struct DOpusLocale *locale));

LIBPROTO(L_PopUpFreeHandle, void, REG(a0, PopUpHandle *handle));

LIBPROTO(L_PopUpNewItem,
		 PopUpItem *,
		 REG(a0, PopUpHandle *handle),
		 REG(d0, IPTR string),
		 REG(d1, ULONG id),
		 REG(d2, ULONG flags));

LIBPROTO(L_PopUpSeparator, void, REG(a0, PopUpHandle *handle));

LIBPROTO(L_PopUpItemSub, BOOL, REG(a0, PopUpHandle *menu), REG(a1, PopUpItem *item));

LIBPROTO(L_PopUpEndSub, void, REG(a0, PopUpHandle *menu));

LIBPROTO(L_PopUpSetFlags,
		 ULONG,
		 REG(a0, PopUpMenu *menu),
		 REG(d0, UWORD id),
		 REG(d1, ULONG value),
		 REG(d2, ULONG mask));

// progress_win.c
LIBPROTO(L_OpenProgressWindow, struct _ProgressWindow *, REG(a0, struct TagItem *tags), REG(a6, struct MyLibrary *lib));

LIBPROTO(L_CloseProgressWindow, void, REG(a0, struct _ProgressWindow *prog));

LIBPROTO(L_HideProgressWindow, void, REG(a0, struct _ProgressWindow *prog));

LIBPROTO(L_ShowProgressWindow,
		 void,
		 REG(a0, struct _ProgressWindow *prog),
		 REG(a1, struct Screen *screen),
		 REG(a2, struct Window *window));

LIBPROTO(L_SetProgressWindow, void, REG(a0, struct _ProgressWindow *prog), REG(a1, struct TagItem *tags));

LIBPROTO(L_CheckProgressAbort, BOOL, REG(a0, struct _ProgressWindow *prog));

LIBPROTO(L_CalcPercent, long, REG(d0, ULONG amount), REG(d1, ULONG total), REG(a0, struct Library *UtilityBase));

LIBPROTO(L_GetProgressWindow, void, REG(a0, ProgressWindow *prog), REG(a1, struct TagItem *tags));

// read_ilbm.c
LIBPROTO(L_ReadILBM, ILBMHandle *, REG(a0, char *name), REG(d0, ULONG flags));

LIBPROTO(L_FreeILBM, void, REG(a0, ILBMHandle *ilbm));

LIBPROTO(L_DecodeILBM,
		 void,
		 REG(a0, char *source),
		 REG(d0, unsigned short width),
		 REG(d1, unsigned short height),
		 REG(d2, unsigned short depth),
		 REG(a1, struct BitMap *dest),
		 REG(d3, unsigned long flags),
		 REG(d4, char comp));

LIBPROTO(L_DecodeRLE, void, REG(a0, RLEinfo *rle));

LIBPROTO(L_FakeILBM,
		 ILBMHandle *,
		 REG(a0, UWORD *imagedata),
		 REG(a1, ULONG *palette),
		 REG(d0, short width),
		 REG(d1, short height),
		 REG(d2, short depth),
		 REG(d3, ULONG flags));

// requesters.c
LIBPROTO(L_AsyncRequest,
		 long,
		 REG(a0, IPCData *my_ipc),
		 REG(d0, long type),
		 REG(a1, struct Window *window),
		 REG(a2, REF_CALLBACK callback),
		 REG(a3, APTR data),
		 REG(d1, struct TagItem *tags),
		 REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_CheckRefreshMsg, struct IntuiMessage *, REG(a0, struct Window *window), REG(d0, ULONG mask));

// rexx.c
LIBPROTO(L_FreeRexxMsgEx, void, REG(a0, struct RexxMsg *msg));

LIBPROTO(L_CreateRexxMsgEx,
		 struct RexxMsg *,
		 REG(a0, struct MsgPort *port),
		 REG(a1, UBYTE *extension),
		 REG(d0, UBYTE *host));

LIBPROTO(L_SetRexxVarEx,
		 long,
		 REG(a0, struct RexxMsg *msg),
		 REG(a1, char *varname),
		 REG(d0, char *value),
		 REG(d1, long length));

LIBPROTO(L_GetRexxVarEx, long, REG(a0, struct RexxMsg *msg), REG(a1, char *varname), REG(a2, char **bufpointer));

LIBPROTO(L_BuildRexxMsgEx,
		 struct RexxMsg *,
		 REG(a0, struct MsgPort *port),
		 REG(a1, UBYTE *extension),
		 REG(d0, UBYTE *host),
		 REG(a2, struct TagItem *tags));

// savepos.c
LIBPROTO(L_SavePos, BOOL, REG(a0, char *name), REG(a1, struct IBox *box), REG(d0, short fontsize));

LIBPROTO(L_LoadPos, BOOL, REG(a0, char *name), REG(a1, struct IBox *box), REG(d0, short *fontsize));

// search.c
LIBPROTO(L_SearchFile,
		 long,
		 REG(a0, APTR file),
		 REG(a1, UBYTE *search_text),
		 REG(d0, ULONG flags),
		 REG(a2, UBYTE *buffer),
		 REG(d1, ULONG buffer_size));

// selection_list.c
LIBPROTO(L_SelectionList,
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
         REG(d5, char **switch_txt),
         REG(d6, ULONG *switch_flags),
		 REG(a6, struct MyLibrary *lib));

// semaphores.c
LIBPROTO(L_GetSemaphore, long, REG(a0, struct SignalSemaphore *sem), REG(d0, long exclusive), REG(a1, char *data));

LIBPROTO(L_FreeSemaphore, void, REG(a0, struct SignalSemaphore *sem));

LIBPROTO(L_ShowSemaphore, void, REG(a0, struct SignalSemaphore *sem));

LIBPROTO(L_InitListLock, void, REG(a0, struct ListLock *ll), REG(a1, char *name));

// simplerequest.c
LIBPROTO(L_DoSimpleRequest,
		 short,
		 REG(a0, struct Window *parent),
		 REG(a1, struct DOpusSimpleRequest *simple),
		 REG(a6, struct Library *libbase));

LIBPROTO(L_SimpleRequest,
		 short,
		 REG(a0, struct Window *parent),
		 REG(a1, char *title),
		 REG(a2, char *buttons),
		 REG(a3, char *message),
         REG(d0, char *buffer),
         REG(d1, APTR params),
         REG(d2, long buffersize),
         REG(d3, ULONG flags),
		 REG(a6, struct Library *libbase));

// status_window.c
LIBPROTO(L_OpenStatusWindow,
		 struct Window *,
		 REG(a0, char *title),
		 REG(a1, char *text),
		 REG(a2, struct Screen *screen),
		 REG(d1, LONG graph),
		 REG(d0, ULONG flags),
		 REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_SetStatusText, void, REG(a0, struct Window *window), REG(a1, char *text));

LIBPROTO(L_UpdateStatusGraph,
		 void,
		 REG(a0, struct Window *window),
		 REG(a1, char *text),
		 REG(d0, ULONG total),
		 REG(d1, ULONG count));

// string_hook.c
LIBPROTO(L_GetSecureString, char *, REG(a0, struct Gadget *gadget));

LIBPROTO(L_GetEditHook, HookData *, REG(d0, ULONG type), REG(d1, ULONG flags), REG(a0, struct TagItem *tags));

LIBPROTO(L_FreeEditHook, void, REG(a0, APTR hook));

// strings.c
LIBPROTO(L_StrCombine, BOOL, REG(a0, char *buf), REG(a1, char *one), REG(a2, char *two), REG(d0, int lim));

LIBPROTO(L_StrConcat, BOOL, REG(a0, char *buf), REG(a1, char *cat), REG(d0, int lim));

LIBPROTO(L_Atoh, ULONG, REG(a0, unsigned char *buf), REG(d0, short len));

// timer.c
LIBPROTO(L_AllocTimer, struct TimerHandle *, REG(d0, ULONG unit), REG(a0, struct MsgPort *port));

LIBPROTO(L_FreeTimer, void, REG(a0, struct TimerHandle *handle));

LIBPROTO(L_StartTimer, void, REG(a0, TimerHandle *handle), REG(d0, ULONG seconds), REG(d1, ULONG micros));

LIBPROTO(L_CheckTimer, BOOL, REG(a0, TimerHandle *handle));

LIBPROTO(L_StopTimer, void, REG(a0, TimerHandle *handle));

LIBPROTO(L_GetTimerBase, struct Library *, REG(a6, struct MyLibrary *lib));

LIBPROTO(L_TimerActive, BOOL, REG(a0, TimerHandle *handle));

// version.c
LIBPROTO(L_GetFileVersion,
		 BOOL,
		 REG(a0, char *name),
		 REG(d0, short *version),
		 REG(d1, short *revision),
		 REG(a1, struct DateStamp *date),
		 REG(a2, APTR progress));

// wb.c
LIBPROTO(L_WB_Install_Patch, void, REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_WB_Remove_Patch, BOOL, REG(a6, struct MyLibrary *libbase));

#if !defined(__MORPHOS__)
LIBPROTO(L_WB_AddAppWindow,
		 struct AppWindow *,
		 REG(d0, IPTR id),
		 REG(d1, IPTR userdata),
		 REG(a0, struct Window *window),
		 REG(a1, struct MsgPort *port),
		 REG(a2, struct TagItem *tags));

LIBPROTO(L_WB_RemoveAppWindow, BOOL, REG(a0, struct AppWindow *window));

LIBPROTO(L_WB_AddAppIcon,
		 struct AppIcon *,
		 REG(d0, IPTR id),
		 REG(d1, IPTR userdata),
		 REG(a0, char *text),
		 REG(a1, struct MsgPort *port),
		 REG(a2, BPTR lock),
		 REG(a3, struct DiskObject *object),
		 REG(a4, struct TagItem *tags));

LIBPROTO(L_WB_RemoveAppIcon, BOOL, REG(a0, struct AppIcon *icon));

LIBPROTO(L_WB_AddAppMenuItem,
		 struct AppMenuItem *,
		 REG(d0, IPTR id),
		 REG(d1, IPTR userdata),
		 REG(a0, char *text),
		 REG(a1, struct MsgPort *port),
		 REG(a2, struct TagItem *tags));

LIBPROTO(L_WB_RemoveAppMenuItem, BOOL, REG(a0, struct AppMenuItem *item));
LIBPROTO(L_WB_AddPort, void, REG(a1, struct MsgPort *port));
LIBPROTO(L_WB_CloseWindow, void, REG(a0, struct Window *window));

LIBPROTO(L_PatchedWBInfo, ULONG, REG(a0, BPTR lock), REG(a1, char *name), REG(a2, struct Screen *screen));

LIBPROTO(L_PatchedAddTask, APTR, REG(a1, struct Task *task), REG(a2, APTR initialPC), REG(a3, APTR finalPC));

LIBPROTO(L_PatchedRemTask, void, REG(a1, struct Task *task));

LIBPROTO(L_PatchedFindTask, struct Task *, REG(a1, char *name));

LIBPROTO(L_PatchedOpenWindowTags, struct Window *, REG(a0, struct NewWindow *newwin), REG(a1, struct TagItem *tags));
#endif

LIBPROTO(L_WB_FindAppWindow, struct AppWindow *, REG(a0, struct Window *window), REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_WB_AppWindowData,
		 struct MsgPort *,
		 REG(a0, struct AppWindow *window),
		 REG(a1, IPTR *id),
		 REG(a2, IPTR *userdata));

LIBPROTO(L_WB_AppWindowLocal, BOOL, REG(a0, struct AppWindow *window));

/*
LIBPROTO(L_WB_AppIconSnapshot, BOOL,	REG(a0, struct AppIcon *icon));
*/
LIBPROTO(L_LockAppList, APTR, REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_NextAppEntry, APTR, REG(a0, APTR last), REG(d0, ULONG type), REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_UnlockAppList, void, REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_WB_CloseWorkBench, LONG);

LIBPROTO(L_WB_OpenWorkBench, ULONG);

#if defined(__MORPHOS__)
BOOL L_WB_PutDiskObject(void);
BOOL L_WB_DeleteDiskObject(void);
#else
LIBPROTO(L_WB_PutDiskObject, BOOL, REG(a0, char *name), REG(a1, struct DiskObject *diskobj));
LIBPROTO(L_WB_DeleteDiskObject, BOOL, REG(a0, char *name));
#endif

LIBPROTO(L_CopyDiskObject,
		 struct DiskObject *,
		 REG(a0, struct DiskObject *icon),
		 REG(d0, ULONG flags),
		 REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_FreeDiskObjectCopy, void, REG(a0, struct DiskObject *icon), REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_ChangeAppIcon,
		 void,
		 REG(a0, APTR appicon),
		 REG(a1, struct Image *render),
		 REG(a2, struct Image *select),
		 REG(a3, char *title),
		 REG(d0, ULONG flags),
		 REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_SetAppIconMenuState, long, REG(a0, APTR appicon), REG(d0, long item), REG(d1, long state));

/*
LIBPROTO(L_PatchedAllocBitmap, struct BitMap,	REG(d0, ULONG sizex),
												REG(d1, ULONG sizey),
												REG(d2, ULONG depth),
												REG(d3, ULONG flags),
												REG(a0, struct BitMap *friend),
												REG(a6, struct Library *GfxBase));
*/

LIBPROTO(L_AddAllocBitmapPatch,
		 APTR,
		 REG(a0, struct Task *task),
		 REG(a1, struct Screen *screen),
		 REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_RemAllocBitmapPatch, void, REG(a0, APTR handle), REG(a6, struct MyLibrary *libbase));

LIBPROTO(L_WB_AppWindowWindow, struct Window *, REG(a0, struct AppWindow *window));

LIBPROTO(L_WB_AppIconFlags, unsigned long, REG(a0, struct AppIcon *icon));

// 64bit.c
LIBPROTO(L_DivideU64, void, REG(a0, UQUAD *num), REG(d0, ULONG div), REG(a1, UQUAD *rem), REG(a2, UQUAD *quo));

LIBPROTO(L_ItoaU64, void, REG(a0, UQUAD *num), REG(a1, char *str), REG(d0, int str_size), REG(d1, char sep));

LIBPROTO(L_DivideToString64,
		 void,
		 REG(a0, char *string),
		 REG(d0, int str_size),
		 REG(a1, UQUAD *bytes),
		 REG(d1, ULONG div),
		 REG(d2, int places),
		 REG(d3, char sep));

LIBPROTO(L_BytesToString64,
		 void,
		 REG(a0, UQUAD *bytes),
		 REG(a1, char *string),
		 REG(d0, int str_size),
		 REG(d1, int places),
		 REG(d2, char sep));

LIBPROTO(L_ExamineLock64, BOOL, REG(d0, BPTR lock), REG(a0, FileInfoBlock64 *fib));

LIBPROTO(L_ExamineNext64, BOOL, REG(d0, BPTR lock), REG(a0, FileInfoBlock64 *fib));

LIBPROTO(L_ExamineHandle64, BOOL, REG(d0, BPTR fh), REG(a0, FileInfoBlock64 *fib));

LIBPROTO(L_MatchFirst64, LONG, REG(a0, STRPTR pat), REG(a1, struct AnchorPath *panchor));

LIBPROTO(L_MatchNext64, LONG, REG(a0, struct AnchorPath *panchor));

/**********************************************************************
	Function entrances
**********************************************************************/

#if defined(__MORPHOS__)
	#define ENTRANCE_2(ret, name, r1, t1, n1, r2, t2, n2)                  \
		ret name##PPC(void);                                               \
		static struct EmulLibEntry name = {TRAP_LIB, 0, (APTR)&name##PPC}; \
		ret name##PPC(void)                                                \
		{                                                                  \
			t1 n1 = (t1)REG_##r1;                                          \
			t2 n2 = (t2)REG_##r2;
#else
	#define ENTRANCE_2(ret, name, r1, t1, n1, r2, t2, n2) \
		ret name(REG(r1, t1 n1), REG(r2, t2 n2))          \
		{
#endif

#define ENTRANCE_END }

#endif
