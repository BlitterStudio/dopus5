/* Automatically generated header! Do not edit! */

#ifndef _PPCINLINE_DOPUS_H
#define _PPCINLINE_DOPUS_H

#ifndef __PPCINLINE_MACROS_H
	#include <ppcinline/macros.h>
#endif /* !__PPCINLINE_MACROS_H */

#ifndef DOPUS_BASE_NAME
	#define DOPUS_BASE_NAME DOpusBase
#endif /* !DOPUS_BASE_NAME */

#define DrawDragList(__p0, __p1, __p2) \
	LP3NR(2088,                        \
		  DrawDragList,                \
		  struct RastPort *,           \
		  __p0,                        \
		  a0,                          \
		  struct ViewPort *,           \
		  __p1,                        \
		  a1,                          \
		  long,                        \
		  __p2,                        \
		  d0,                          \
		  ,                            \
		  DOPUS_BASE_NAME,             \
		  0,                           \
		  0,                           \
		  0,                           \
		  0,                           \
		  0,                           \
		  0)

#define RefreshObjectList(__p0, __p1) \
	LP2NR(306,                        \
		  RefreshObjectList,          \
		  struct Window *,            \
		  __p0,                       \
		  a0,                         \
		  ObjectList *,               \
		  __p1,                       \
		  a1,                         \
		  ,                           \
		  DOPUS_BASE_NAME,            \
		  0,                          \
		  0,                          \
		  0,                          \
		  0,                          \
		  0,                          \
		  0)

#define IFFChunkRemain(__p0) LP1(1386, long, IFFChunkRemain, APTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define OriginalSetComment(__p0, __p1) \
	LP2(1920, BOOL, OriginalSetComment, char *, __p0, d1, char *, __p1, d2, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CloseConfigWindow(__p0) \
	LP1NR(246, CloseConfigWindow, struct Window *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define IFFFileHandle(__p0) LP1(1380, APTR, IFFFileHandle, APTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define SerialValid(__p0) LP1(1056, BOOL, SerialValid, serial_data *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define SetReqBackFill(__p0, __p1) \
	LP2NR(2124,                    \
		  SetReqBackFill,          \
		  struct Hook *,           \
		  __p0,                    \
		  a0,                      \
		  struct Screen **,        \
		  __p1,                    \
		  a1,                      \
		  ,                        \
		  DOPUS_BASE_NAME,         \
		  0,                       \
		  0,                       \
		  0,                       \
		  0,                       \
		  0,                       \
		  0)

#define SetWindowBusy(__p0) LP1NR(390, SetWindowBusy, struct Window *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetIconFlags(__p0) \
	LP1(1500, ULONG, GetIconFlags, struct DiskObject *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define IPC_Launch(__p0, __p1, __p2, __p3, __p4, __p5, __p6) \
	LP7(624,                                                 \
		int,                                                 \
		IPC_Launch,                                          \
		struct ListLock *,                                   \
		__p0,                                                \
		a0,                                                  \
		IPCData **,                                          \
		__p1,                                                \
		a1,                                                  \
		char *,                                              \
		__p2,                                                \
		a2,                                                  \
		IPTR,                                                \
		__p3,                                                \
		d0,                                                  \
		ULONG,                                               \
		__p4,                                                \
		d1,                                                  \
		IPTR,                                                \
		__p5,                                                \
		d2,                                                  \
		struct Library *,                                    \
		__p6,                                                \
		a3,                                                  \
		,                                                    \
		DOPUS_BASE_NAME,                                     \
		0,                                                   \
		0,                                                   \
		0,                                                   \
		0,                                                   \
		0,                                                   \
		0)

#define DragCustomOk(__p0) LP1(2142, BOOL, DragCustomOk, struct BitMap *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define RemDragImage(__p0) LP1NR(234, RemDragImage, DragInfo *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FreeObjectList(__p0) LP1NR(300, FreeObjectList, ObjectList *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define NotifyDiskChange() LP0NR(2040, NotifyDiskChange, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define SetConfigWindowLimits(__p0, __p1, __p2) \
	LP3NR(1860,                                 \
		  SetConfigWindowLimits,                \
		  struct Window *,                      \
		  __p0,                                 \
		  a0,                                   \
		  const ConfigWindow *,                 \
		  __p1,                                 \
		  a1,                                   \
		  const ConfigWindow *,                 \
		  __p2,                                 \
		  a2,                                   \
		  ,                                     \
		  DOPUS_BASE_NAME,                      \
		  0,                                    \
		  0,                                    \
		  0,                                    \
		  0,                                    \
		  0,                                    \
		  0)

#define SearchFile(__p0, __p1, __p2, __p3, __p4) \
	LP5(1716,                                    \
		long,                                    \
		SearchFile,                              \
		APTR,                                    \
		__p0,                                    \
		a0,                                      \
		UBYTE *,                                 \
		__p1,                                    \
		a1,                                      \
		ULONG,                                   \
		__p2,                                    \
		d0,                                      \
		UBYTE *,                                 \
		__p3,                                    \
		a2,                                      \
		ULONG,                                   \
		__p4,                                    \
		d1,                                      \
		,                                        \
		DOPUS_BASE_NAME,                         \
		0,                                       \
		0,                                       \
		0,                                       \
		0,                                       \
		0,                                       \
		0)

#define FindPubScreen(__p0, __p1) \
	LP2(1704,                     \
		struct PubScreenNode *,   \
		FindPubScreen,            \
		struct Screen *,          \
		__p0,                     \
		a0,                       \
		BOOL,                     \
		__p1,                     \
		d0,                       \
		,                         \
		DOPUS_BASE_NAME,          \
		0,                        \
		0,                        \
		0,                        \
		0,                        \
		0,                        \
		0)

#define NewFunction(__p0, __p1) \
	LP2(726, Cfg_Function *, NewFunction, APTR, __p0, a0, UWORD, __p1, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define ChangeAppIcon(__p0, __p1, __p2, __p3, __p4) \
	LP5NR(1668,                                     \
		  ChangeAppIcon,                            \
		  APTR,                                     \
		  __p0,                                     \
		  a0,                                       \
		  struct Image *,                           \
		  __p1,                                     \
		  a1,                                       \
		  struct Image *,                           \
		  __p2,                                     \
		  a2,                                       \
		  char *,                                   \
		  __p3,                                     \
		  a3,                                       \
		  ULONG,                                    \
		  __p4,                                     \
		  d0,                                       \
		  ,                                         \
		  DOPUS_BASE_NAME,                          \
		  0,                                        \
		  0,                                        \
		  0,                                        \
		  0,                                        \
		  0,                                        \
		  0)

#define IconCheckSum(__p0, __p1) \
	LP2(1620, ULONG, IconCheckSum, struct DiskObject *, __p0, a0, short, __p1, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CopyButton(__p0, __p1, __p2) \
	LP3(846,                         \
		Cfg_Button *,                \
		CopyButton,                  \
		Cfg_Button *,                \
		__p0,                        \
		a0,                          \
		APTR,                        \
		__p1,                        \
		a1,                          \
		short,                       \
		__p2,                        \
		d0,                          \
		,                            \
		DOPUS_BASE_NAME,             \
		0,                           \
		0,                           \
		0,                           \
		0,                           \
		0,                           \
		0)

#define AddNotifyRequest(__p0, __p1, __p2) \
	LP3(1122,                              \
		APTR,                              \
		AddNotifyRequest,                  \
		ULONG,                             \
		__p0,                              \
		d0,                                \
		ULONG,                             \
		__p1,                              \
		d1,                                \
		struct MsgPort *,                  \
		__p2,                              \
		a0,                                \
		,                                  \
		DOPUS_BASE_NAME,                   \
		0,                                 \
		0,                                 \
		0,                                 \
		0,                                 \
		0,                                 \
		0)

#define OpenBuf(__p0, __p1, __p2) \
	LP3(984, APTR, OpenBuf, char *, __p0, a0, long, __p1, d0, long, __p2, d1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define WB_AppIconFlags(__p0) \
	LP1(1464, ULONG, WB_AppIconFlags, struct AppIcon *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetEditHook(__p0, __p1, __p2) \
	LP3(1410,                         \
		struct Hook *,                \
		GetEditHook,                  \
		ULONG,                        \
		__p0,                         \
		d0,                           \
		ULONG,                        \
		__p1,                         \
		d1,                           \
		struct TagItem *,             \
		__p2,                         \
		a0,                           \
		,                             \
		DOPUS_BASE_NAME,              \
		0,                            \
		0,                            \
		0,                            \
		0,                            \
		0,                            \
		0)

#define GetString(__p0, __p1) \
	LP2(402, STRPTR, GetString, struct DOpusLocale *, __p0, a0, LONG, __p1, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define OpenProgressWindow(__p0) \
	LP1(1626, APTR, OpenProgressWindow, struct TagItem *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetPopUpImageSize(__p0, __p1, __p2, __p3) \
	LP4NR(2166,                                   \
		  GetPopUpImageSize,                      \
		  struct Window *,                        \
		  __p0,                                   \
		  a0,                                     \
		  PopUpMenu *,                            \
		  __p1,                                   \
		  a1,                                     \
		  short *,                                \
		  __p2,                                   \
		  a2,                                     \
		  short *,                                \
		  __p3,                                   \
		  a3,                                     \
		  ,                                       \
		  DOPUS_BASE_NAME,                        \
		  0,                                      \
		  0,                                      \
		  0,                                      \
		  0,                                      \
		  0,                                      \
		  0)

#define ConvertRawKey(__p0, __p1, __p2) \
	LP3(1254,                           \
		BOOL,                           \
		ConvertRawKey,                  \
		UWORD,                          \
		__p0,                           \
		d0,                             \
		UWORD,                          \
		__p1,                           \
		d1,                             \
		char *,                         \
		__p2,                           \
		a0,                             \
		,                               \
		DOPUS_BASE_NAME,                \
		0,                              \
		0,                              \
		0,                              \
		0,                              \
		0,                              \
		0)

#define IPC_Flush(__p0) LP1NR(606, IPC_Flush, IPCData *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define LoadPos(__p0, __p1, __p2) \
	LP3(2298,                     \
		BOOL,                     \
		LoadPos,                  \
		char *,                   \
		__p0,                     \
		a0,                       \
		struct IBox *,            \
		__p1,                     \
		a1,                       \
		short *,                  \
		__p2,                     \
		d0,                       \
		,                         \
		DOPUS_BASE_NAME,          \
		0,                        \
		0,                        \
		0,                        \
		0,                        \
		0,                        \
		0)

#define SetGadgetValue(__p0, __p1, __p2) \
	LP3NR(324,                           \
		  SetGadgetValue,                \
		  ObjectList *,                  \
		  __p0,                          \
		  a0,                            \
		  UWORD,                         \
		  __p1,                          \
		  d0,                            \
		  IPTR,                          \
		  __p2,                          \
		  d1,                            \
		  ,                              \
		  DOPUS_BASE_NAME,               \
		  0,                             \
		  0,                             \
		  0,                             \
		  0,                             \
		  0,                             \
		  0)

#define IsListLockEmpty(__p0) \
	LP1(1872, BOOL, IsListLockEmpty, struct ListLock *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define RemovedFunc10() LP0NR(1560, RemovedFunc10, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define RemovedFunc11() LP0NR(1566, RemovedFunc11, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define StopTimer(__p0) LP1NR(132, StopTimer, TimerHandle *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define RemovedFunc12() LP0NR(1572, RemovedFunc12, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define IPC_ListCommand(__p0, __p1, __p2, __p3, __p4) \
	LP5NR(612,                                        \
		  IPC_ListCommand,                            \
		  struct ListLock *,                          \
		  __p0,                                       \
		  a0,                                         \
		  ULONG,                                      \
		  __p1,                                       \
		  d0,                                         \
		  IPTR,                                       \
		  __p2,                                       \
		  d1,                                         \
		  IPTR,                                       \
		  __p3,                                       \
		  d2,                                         \
		  BOOL,                                       \
		  __p4,                                       \
		  d3,                                         \
		  ,                                           \
		  DOPUS_BASE_NAME,                            \
		  0,                                          \
		  0,                                          \
		  0,                                          \
		  0,                                          \
		  0,                                          \
		  0)

#define FakeILBM(__p0, __p1, __p2, __p3, __p4, __p5) \
	LP6(1980,                                        \
		ILBMHandle *,                                \
		FakeILBM,                                    \
		UWORD *,                                     \
		__p0,                                        \
		a0,                                          \
		ULONG *,                                     \
		__p1,                                        \
		a1,                                          \
		short,                                       \
		__p2,                                        \
		d0,                                          \
		short,                                       \
		__p3,                                        \
		d1,                                          \
		short,                                       \
		__p4,                                        \
		d2,                                          \
		ULONG,                                       \
		__p5,                                        \
		d3,                                          \
		,                                            \
		DOPUS_BASE_NAME,                             \
		0,                                           \
		0,                                           \
		0,                                           \
		0,                                           \
		0,                                           \
		0)

#define DefaultSettings(__p0) LP1NR(756, DefaultSettings, CFG_SETS *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define ConvertStartMenu(__p0) \
	LP1NR(2202, ConvertStartMenu, Cfg_ButtonBank *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define StartRefreshConfigWindow(__p0, __p1) \
	LP2NR(1806,                              \
		  StartRefreshConfigWindow,          \
		  struct Window *,                   \
		  __p0,                              \
		  a0,                                \
		  long,                              \
		  __p1,                              \
		  d0,                                \
		  ,                                  \
		  DOPUS_BASE_NAME,                   \
		  0,                                 \
		  0,                                 \
		  0,                                 \
		  0,                                 \
		  0,                                 \
		  0)

#define UnlockAppList() LP0NR(1116, UnlockAppList, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define Att_RemNode(__p0) LP1NR(450, Att_RemNode, Att_Node *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MatchFiletype(__p0, __p1) \
	LP2(1746, BOOL, MatchFiletype, APTR, __p0, a0, APTR, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define InitWindowDims(__p0, __p1) \
	LP2NR(1422,                    \
		  InitWindowDims,          \
		  struct Window *,         \
		  __p0,                    \
		  a0,                      \
		  WindowDimensions *,      \
		  __p1,                    \
		  a1,                      \
		  ,                        \
		  DOPUS_BASE_NAME,         \
		  0,                       \
		  0,                       \
		  0,                       \
		  0,                       \
		  0,                       \
		  0)

#define FreeFunction(__p0) LP1NR(822, FreeFunction, Cfg_Function *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define OpenImage(__p0, __p1) \
	LP2(630, APTR, OpenImage, char *, __p0, a0, OpenImageInfo *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define SetRexxVarEx(__p0, __p1, __p2, __p3) \
	LP4(2022,                                \
		long,                                \
		SetRexxVarEx,                        \
		struct RexxMsg *,                    \
		__p0,                                \
		a0,                                  \
		char *,                              \
		__p1,                                \
		a1,                                  \
		char *,                              \
		__p2,                                \
		d0,                                  \
		long,                                \
		__p3,                                \
		d1,                                  \
		,                                    \
		DOPUS_BASE_NAME,                     \
		0,                                   \
		0,                                   \
		0,                                   \
		0,                                   \
		0,                                   \
		0)

#define HideDragImage(__p0) LP1NR(204, HideDragImage, DragInfo *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define StrCombine(__p0, __p1, __p2, __p3) \
	LP4(1140,                              \
		BOOL,                              \
		StrCombine,                        \
		char *,                            \
		__p0,                              \
		a0,                                \
		char *,                            \
		__p1,                              \
		a1,                                \
		char *,                            \
		__p2,                              \
		a2,                                \
		int,                               \
		__p3,                              \
		d0,                                \
		,                                  \
		DOPUS_BASE_NAME,                   \
		0,                                 \
		0,                                 \
		0,                                 \
		0,                                 \
		0,                                 \
		0)

#define BuildMenuStrip(__p0, __p1) \
	LP2(522,                       \
		struct Menu *,             \
		BuildMenuStrip,            \
		const MenuData *,          \
		__p0,                      \
		a0,                        \
		struct DOpusLocale *,      \
		__p1,                      \
		a1,                        \
		,                          \
		DOPUS_BASE_NAME,           \
		0,                         \
		0,                         \
		0,                         \
		0,                         \
		0,                         \
		0)

#define WB_LaunchNew(__p0, __p1, __p2, __p3, __p4) \
	LP5(2148,                                      \
		BOOL,                                      \
		WB_LaunchNew,                              \
		char *,                                    \
		__p0,                                      \
		a0,                                        \
		struct Screen *,                           \
		__p1,                                      \
		a1,                                        \
		short,                                     \
		__p2,                                      \
		d0,                                        \
		long,                                      \
		__p3,                                      \
		d1,                                        \
		char *,                                    \
		__p4,                                      \
		a2,                                        \
		,                                          \
		DOPUS_BASE_NAME,                           \
		0,                                         \
		0,                                         \
		0,                                         \
		0,                                         \
		0,                                         \
		0)

#define ParseArgs(__p0, __p1) \
	LP2(1848, FuncArgs *, ParseArgs, char *, __p0, a0, char *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetLibraryFlags() LP0(2112, ULONG, GetLibraryFlags, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define IPC_Command(__p0, __p1, __p2, __p3, __p4, __p5) \
	LP6(552,                                            \
		IPTR,                                           \
		IPC_Command,                                    \
		IPCData *,                                      \
		__p0,                                           \
		a0,                                             \
		ULONG,                                          \
		__p1,                                           \
		d0,                                             \
		IPTR,                                            \
		__p2,                                           \
		d1,                                             \
		APTR,                                           \
		__p3,                                           \
		a1,                                             \
		APTR,                                           \
		__p4,                                           \
		a2,                                             \
		struct MsgPort *,                               \
		__p5,                                           \
		a3,                                             \
		,                                               \
		DOPUS_BASE_NAME,                                \
		0,                                              \
		0,                                              \
		0,                                              \
		0,                                              \
		0,                                              \
		0)

#define FreeObject(__p0, __p1) \
	LP2NR(294, FreeObject, ObjectList *, __p0, a0, GL_Object *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define MUFSLogin(__p0, __p1, __p2) \
	LP3NR(2190,                     \
		  MUFSLogin,                \
		  struct Window *,          \
		  __p0,                     \
		  a0,                       \
		  char *,                   \
		  __p1,                     \
		  a1,                       \
		  char *,                   \
		  __p2,                     \
		  a2,                       \
		  ,                         \
		  DOPUS_BASE_NAME,          \
		  0,                        \
		  0,                        \
		  0,                        \
		  0,                        \
		  0,                        \
		  0)

#define SetStatusText(__p0, __p1) \
	LP2NR(936, SetStatusText, struct Window *, __p0, a0, char *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define DeleteIcon(__p0) LP1(2184, BOOL, DeleteIcon, char *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FreeMemH(__p0) LP1NR(690, FreeMemH, void *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CloseIFFFile(__p0) LP1NR(924, CloseIFFFile, struct IFFHandle *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define DivideU(__p0, __p1, __p2, __p3) \
	LP4(54,                             \
		ULONG,                          \
		DivideU,                        \
		ULONG,                          \
		__p0,                           \
		d0,                             \
		ULONG,                          \
		__p1,                           \
		d1,                             \
		ULONG *,                        \
		__p2,                           \
		a0,                             \
		struct Library *,               \
		__p3,                           \
		a1,                             \
		,                               \
		DOPUS_BASE_NAME,                \
		0,                              \
		0,                              \
		0,                              \
		0,                              \
		0,                              \
		0)

#define DivideU64(___num, ___div, ___rem, ___quo) \
	LP4NR(0x906,                                  \
		  DivideU64,                              \
		  UQUAD *,                                \
		  ___num,                                 \
		  a0,                                     \
		  ULONG,                                  \
		  ___div,                                 \
		  d0,                                     \
		  UQUAD *,                                \
		  ___rem,                                 \
		  a1,                                     \
		  UQUAD *,                                \
		  ___quo,                                 \
		  a2,                                     \
		  ,                                       \
		  DOPUS_BASE_NAME,                        \
		  IF_CACHEFLUSHALL,                       \
		  NULL,                                   \
		  0,                                      \
		  IF_CACHEFLUSHALL,                       \
		  NULL,                                   \
		  0)

#define FreeEditHook(__p0) LP1NR(1416, FreeEditHook, struct Hook *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define Att_NewNode(__p0, __p1, __p2, __p3) \
	LP4(444,                                \
		Att_Node *,                         \
		Att_NewNode,                        \
		Att_List *,                         \
		__p0,                               \
		a0,                                 \
		const char *,                       \
		__p1,                               \
		a1,                                 \
		IPTR,                               \
		__p2,                               \
		d0,                                 \
		ULONG,                              \
		__p3,                               \
		d1,                                 \
		,                                   \
		DOPUS_BASE_NAME,                    \
		0,                                  \
		0,                                  \
		0,                                  \
		0,                                  \
		0,                                  \
		0)

#define IPC_GetGoodbye(__p0) \
	LP1(594, IPTR, IPC_GetGoodbye, IPCMessage *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FreeWindowMenus(__p0) \
	LP1NR(384, FreeWindowMenus, struct Window *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetSecureString(__p0) \
	LP1(1680, char *, GetSecureString, struct Gadget *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define IFFNextChunk(__p0, __p1) \
	LP2(1362, ULONG, IFFNextChunk, APTR, __p0, a0, ULONG, __p1, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FreeImageRemap(__p0) LP1NR(1536, FreeImageRemap, ImageRemap *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define UpdateEnvironment(__p0) \
	LP1NR(2196, UpdateEnvironment, CFG_ENVR *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define SetBusyPointer(__p0) LP1NR(90, SetBusyPointer, struct Window *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CopyImage(__p0) LP1(642, APTR, CopyImage, APTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define WriteFileIcon(__p0, __p1) \
	LP2NR(174, WriteFileIcon, char *, __p0, a0, char *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define SetEnv(__p0, __p1, __p2) \
	LP3NR(1866, SetEnv, char *, __p0, a0, char *, __p1, a1, BOOL, __p2, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define IsDiskDevice(__p0) \
	LP1(2082, BOOL, IsDiskDevice, struct MsgPort *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define SetObjectKind(__p0, __p1, __p2) \
	LP3NR(336,                          \
		  SetObjectKind,                \
		  ObjectList *,                 \
		  __p0,                         \
		  a0,                           \
		  ULONG,                        \
		  __p1,                         \
		  d0,                           \
		  UWORD,                        \
		  __p2,                         \
		  d1,                           \
		  ,                             \
		  DOPUS_BASE_NAME,              \
		  0,                            \
		  0,                            \
		  0,                            \
		  0,                            \
		  0,                            \
		  0)

#define StampDragImage(__p0, __p1, __p2) \
	LP3NR(210,                           \
		  StampDragImage,                \
		  DragInfo *,                    \
		  __p0,                          \
		  a0,                            \
		  ULONG,                         \
		  __p1,                          \
		  d0,                            \
		  ULONG,                         \
		  __p2,                          \
		  d1,                            \
		  ,                              \
		  DOPUS_BASE_NAME,               \
		  0,                             \
		  0,                             \
		  0,                             \
		  0,                             \
		  0,                             \
		  0)

#define SaveButtonBank(__p0, __p1) \
	LP2(786, short, SaveButtonBank, Cfg_ButtonBank *, __p0, a0, char *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetDragImage(__p0, __p1, __p2) \
	LP3NR(192,                         \
		  GetDragImage,                \
		  DragInfo *,                  \
		  __p0,                        \
		  a0,                          \
		  ULONG,                       \
		  __p1,                        \
		  d0,                          \
		  ULONG,                       \
		  __p2,                        \
		  d1,                          \
		  ,                            \
		  DOPUS_BASE_NAME,             \
		  0,                           \
		  0,                           \
		  0,                           \
		  0,                           \
		  0,                           \
		  0)

#define AnimDecodeRIFFXor(__p0, __p1, __p2, __p3) \
	LP4NR(1242,                                   \
		  AnimDecodeRIFFXor,                      \
		  unsigned char *,                        \
		  __p0,                                   \
		  a0,                                     \
		  char *,                                 \
		  __p1,                                   \
		  a1,                                     \
		  UWORD,                                  \
		  __p2,                                   \
		  d0,                                     \
		  UWORD,                                  \
		  __p3,                                   \
		  d1,                                     \
		  ,                                       \
		  DOPUS_BASE_NAME,                        \
		  0,                                      \
		  0,                                      \
		  0,                                      \
		  0,                                      \
		  0,                                      \
		  0)

#define Seed(__p0) LP1NR(1554, Seed, int, __p0, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define NewMemHandle(__p0, __p1, __p2) \
	LP3(666,                           \
		void *,                        \
		NewMemHandle,                  \
		ULONG,                         \
		__p0,                          \
		d0,                            \
		ULONG,                         \
		__p1,                          \
		d1,                            \
		ULONG,                         \
		__p2,                          \
		d2,                            \
		,                              \
		DOPUS_BASE_NAME,               \
		0,                             \
		0,                             \
		0,                             \
		0,                             \
		0,                             \
		0)

#define Att_FindNodeData(__p0, __p1) \
	LP2(480, Att_Node *, Att_FindNodeData, Att_List *, __p0, a0, IPTR, __p1, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define QualValid(__p0) LP1(1452, UWORD, QualValid, UWORD, __p0, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define IPC_Goodbye(__p0, __p1, __p2) \
	LP3NR(588,                        \
		  IPC_Goodbye,                \
		  IPCData *,                  \
		  __p0,                       \
		  a0,                         \
		  IPCData *,                  \
		  __p1,                       \
		  a1,                         \
		  ULONG,                      \
		  __p2,                       \
		  d0,                         \
		  ,                           \
		  DOPUS_BASE_NAME,            \
		  0,                          \
		  0,                          \
		  0,                          \
		  0,                          \
		  0,                          \
		  0)

#define StrConcat(__p0, __p1, __p2) \
	LP3(1146, BOOL, StrConcat, char *, __p0, a0, char *, __p1, a1, int, __p2, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CalcObjectDims(__p0, __p1, __p2, __p3, __p4, __p5, __p6, __p7) \
	LP8(276,                                                           \
		int,                                                           \
		CalcObjectDims,                                                \
		void *,                                                        \
		__p0,                                                          \
		a0,                                                            \
		struct TextFont *,                                             \
		__p1,                                                          \
		a1,                                                            \
		struct IBox *,                                                 \
		__p2,                                                          \
		a2,                                                            \
		struct IBox *,                                                 \
		__p3,                                                          \
		a3,                                                            \
		GL_Object *,                                                   \
		__p4,                                                          \
		a4,                                                            \
		ULONG,                                                         \
		__p5,                                                          \
		d0,                                                            \
		GL_Object *,                                                   \
		__p6,                                                          \
		d1,                                                            \
		GL_Object *,                                                   \
		__p7,                                                          \
		d2,                                                            \
		,                                                              \
		DOPUS_BASE_NAME,                                               \
		0,                                                             \
		0,                                                             \
		0,                                                             \
		0,                                                             \
		0,                                                             \
		0)

#define StartTimer(__p0, __p1, __p2) \
	LP3NR(120,                       \
		  StartTimer,                \
		  TimerHandle *,             \
		  __p0,                      \
		  a0,                        \
		  ULONG,                     \
		  __p1,                      \
		  d0,                        \
		  ULONG,                     \
		  __p2,                      \
		  d1,                        \
		  ,                          \
		  DOPUS_BASE_NAME,           \
		  0,                         \
		  0,                         \
		  0,                         \
		  0,                         \
		  0,                         \
		  0)

#define ReadBuf(__p0, __p1, __p2) \
	LP3(996, long, ReadBuf, APTR, __p0, a0, char *, __p1, a1, long, __p2, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define NewLister(__p0) LP1(708, Cfg_Lister *, NewLister, char *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define LayoutResize(__p0) LP1NR(1752, LayoutResize, struct Window *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FreeButtonFunction(__p0) \
	LP1NR(1692, FreeButtonFunction, Cfg_ButtonFunction *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define Att_RemList(__p0, __p1) \
	LP2NR(462, Att_RemList, Att_List *, __p0, a0, long, __p1, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define InitDragDBuf(__p0) LP1(2004, BOOL, InitDragDBuf, DragInfo *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define WB_RemoveAppWindow(__p0) \
	LP1(1080, BOOL, WB_RemoveAppWindow, struct AppWindow *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define AddObjectList(__p0, __p1) \
	LP2(288,                      \
		ObjectList *,             \
		AddObjectList,            \
		struct Window *,          \
		__p0,                     \
		a0,                       \
		const ObjectDef *,        \
		__p1,                     \
		a1,                       \
		,                         \
		DOPUS_BASE_NAME,          \
		0,                        \
		0,                        \
		0,                        \
		0,                        \
		0,                        \
		0)

#define AddScrollBars(__p0, __p1, __p2, __p3) \
	LP4(1038,                                 \
		struct Gadget *,                      \
		AddScrollBars,                        \
		struct Window *,                      \
		__p0,                                 \
		a0,                                   \
		struct List *,                        \
		__p1,                                 \
		a1,                                   \
		struct DrawInfo *,                    \
		__p2,                                 \
		a2,                                   \
		short,                                \
		__p3,                                 \
		d0,                                   \
		,                                     \
		DOPUS_BASE_NAME,                      \
		0,                                    \
		0,                                    \
		0,                                    \
		0,                                    \
		0,                                    \
		0)

#define FreeButtonImages(__p0) \
	LP1NR(810, FreeButtonImages, struct List *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define BuildTransDragMask(__p0, __p1, __p2, __p3, __p4, __p5) \
	LP6(1524,                                                  \
		BOOL,                                                  \
		BuildTransDragMask,                                    \
		UWORD *,                                               \
		__p0,                                                  \
		a0,                                                    \
		UWORD *,                                               \
		__p1,                                                  \
		a1,                                                    \
		short,                                                 \
		__p2,                                                  \
		d0,                                                    \
		short,                                                 \
		__p3,                                                  \
		d1,                                                    \
		short,                                                 \
		__p4,                                                  \
		d2,                                                    \
		long,                                                  \
		__p5,                                                  \
		d3,                                                    \
		,                                                      \
		DOPUS_BASE_NAME,                                       \
		0,                                                     \
		0,                                                     \
		0,                                                     \
		0,                                                     \
		0,                                                     \
		0)

#define OpenStatusWindow(__p0, __p1, __p2, __p3, __p4) \
	LP5(930,                                           \
		struct Window *,                               \
		OpenStatusWindow,                              \
		char *,                                        \
		__p0,                                          \
		a0,                                            \
		char *,                                        \
		__p1,                                          \
		a1,                                            \
		struct Screen *,                               \
		__p2,                                          \
		a2,                                            \
		LONG,                                          \
		__p3,                                          \
		d1,                                            \
		ULONG,                                         \
		__p4,                                          \
		d0,                                            \
		,                                              \
		DOPUS_BASE_NAME,                               \
		0,                                             \
		0,                                             \
		0,                                             \
		0,                                             \
		0,                                             \
		0)

#define SetLibraryFlags(__p0, __p1) \
	LP2(1800, ULONG, SetLibraryFlags, ULONG, __p0, d0, ULONG, __p1, d1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetOpusPathList() LP0(2208, BPTR, GetOpusPathList, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define EndRefreshConfigWindow(__p0) \
	LP1NR(1812, EndRefreshConfigWindow, struct Window *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetIconType(__p0) \
	LP1(2118, short, GetIconType, struct DiskObject *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define ExamineBuf(__p0, __p1) \
	LP2(1020, long, ExamineBuf, APTR, __p0, a0, struct FileInfoBlock *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FreeDosListCopy(__p0) LP1NR(2052, FreeDosListCopy, struct List *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetOriginalIcon(__p0) \
	LP1(2070, struct DiskObject *, GetOriginalIcon, struct DiskObject *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define Att_NewList(__p0) LP1(438, Att_List *, Att_NewList, ULONG, __p0, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetPalette32(__p0, __p1, __p2, __p3) \
	LP4NR(978,                               \
		  GetPalette32,                      \
		  struct ViewPort *,                 \
		  __p0,                              \
		  a0,                                \
		  ULONG *,                           \
		  __p1,                              \
		  a1,                                \
		  UWORD,                             \
		  __p2,                              \
		  d0,                                \
		  short,                             \
		  __p3,                              \
		  d1,                                \
		  ,                                  \
		  DOPUS_BASE_NAME,                   \
		  0,                                 \
		  0,                                 \
		  0,                                 \
		  0,                                 \
		  0,                                 \
		  0)

#define CheckObjectArea(__p0, __p1, __p2) \
	LP3(348,                              \
		BOOL,                             \
		CheckObjectArea,                  \
		GL_Object *,                      \
		__p0,                             \
		a0,                               \
		int,                              \
		__p1,                             \
		d0,                               \
		int,                              \
		__p2,                             \
		d1,                               \
		,                                 \
		DOPUS_BASE_NAME,                  \
		0,                                \
		0,                                \
		0,                                \
		0,                                \
		0,                                \
		0)

#define FindKeyEquivalent(__p0, __p1, __p2) \
	LP3(408,                                \
		struct Gadget *,                    \
		FindKeyEquivalent,                  \
		ObjectList *,                       \
		__p0,                               \
		a0,                                 \
		struct IntuiMessage *,              \
		__p1,                               \
		a1,                                 \
		int,                                \
		__p2,                               \
		d0,                                 \
		,                                   \
		DOPUS_BASE_NAME,                    \
		0,                                  \
		0,                                  \
		0,                                  \
		0,                                  \
		0,                                  \
		0)

#define IPC_QuitName(__p0, __p1, __p2) \
	LP3NR(1446,                        \
		  IPC_QuitName,                \
		  struct ListLock *,           \
		  __p0,                        \
		  a0,                          \
		  char *,                      \
		  __p1,                        \
		  a1,                          \
		  ULONG,                       \
		  __p2,                        \
		  d0,                          \
		  ,                            \
		  DOPUS_BASE_NAME,             \
		  0,                           \
		  0,                           \
		  0,                           \
		  0,                           \
		  0,                           \
		  0)

#define GetObject(__p0, __p1) \
	LP2(312, GL_Object *, GetObject, ObjectList *, __p0, a0, int, __p1, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define NewBitMap(__p0, __p1, __p2, __p3, __p4) \
	LP5(1836,                                   \
		struct BitMap *,                        \
		NewBitMap,                              \
		ULONG,                                  \
		__p0,                                   \
		d0,                                     \
		ULONG,                                  \
		__p1,                                   \
		d1,                                     \
		ULONG,                                  \
		__p2,                                   \
		d2,                                     \
		ULONG,                                  \
		__p3,                                   \
		d3,                                     \
		struct BitMap *,                        \
		__p4,                                   \
		a0,                                     \
		,                                       \
		DOPUS_BASE_NAME,                        \
		0,                                      \
		0,                                      \
		0,                                      \
		0,                                      \
		0,                                      \
		0)

#define DateFromStrings(__p0, __p1, __p2) \
	LP3(1728,                             \
		BOOL,                             \
		DateFromStrings,                  \
		char *,                           \
		__p0,                             \
		a0,                               \
		char *,                           \
		__p1,                             \
		a1,                               \
		struct DateStamp *,               \
		__p2,                             \
		a2,                               \
		,                                 \
		DOPUS_BASE_NAME,                  \
		0,                                \
		0,                                \
		0,                                \
		0,                                \
		0,                                \
		0)

#define SaveFiletypeList(__p0, __p1) \
	LP2(870, int, SaveFiletypeList, Cfg_FiletypeList *, __p0, a0, char *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FreeFiletypeList(__p0) \
	LP1NR(876, FreeFiletypeList, Cfg_FiletypeList *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define StripIntuiMessagesDopus(__p0) \
	LP1NR(264, StripIntuiMessagesDopus, struct Window *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define HideProgressWindow(__p0) LP1NR(1638, HideProgressWindow, APTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CloseDisk(__p0) LP1NR(1032, CloseDisk, DiskHandle *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CloseBuf(__p0) LP1(990, long, CloseBuf, APTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define PopUpEndSub(__p0) LP1NR(2274, PopUpEndSub, PopUpHandle *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define DisableObject(__p0, __p1, __p2) \
	LP3NR(366,                          \
		  DisableObject,                \
		  ObjectList *,                 \
		  __p0,                         \
		  a0,                           \
		  ULONG,                        \
		  __p1,                         \
		  d0,                           \
		  BOOL,                         \
		  __p2,                         \
		  d1,                           \
		  ,                             \
		  DOPUS_BASE_NAME,              \
		  0,                            \
		  0,                            \
		  0,                            \
		  0,                            \
		  0,                            \
		  0)

#define CheckTimer(__p0) LP1(126, BOOL, CheckTimer, TimerHandle *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define PopUpItemSub(__p0, __p1) \
	LP2(2268, BOOL, PopUpItemSub, PopUpHandle *, __p0, a0, PopUpItem *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define BoundsCheckGadget(__p0, __p1, __p2, __p3) \
	LP4(372,                                      \
		int,                                      \
		BoundsCheckGadget,                        \
		ObjectList *,                             \
		__p0,                                     \
		a0,                                       \
		ULONG,                                    \
		__p1,                                     \
		d0,                                       \
		int,                                      \
		__p2,                                     \
		d1,                                       \
		int,                                      \
		__p3,                                     \
		d2,                                       \
		,                                         \
		DOPUS_BASE_NAME,                          \
		0,                                        \
		0,                                        \
		0,                                        \
		0,                                        \
		0,                                        \
		0)

#define GetDosListCopy(__p0, __p1) \
	LP2NR(2046, GetDosListCopy, struct List *, __p0, a0, APTR, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define StoreGadgetValue(__p0, __p1) \
	LP2NR(318,                       \
		  StoreGadgetValue,          \
		  ObjectList *,              \
		  __p0,                      \
		  a0,                        \
		  struct IntuiMessage *,     \
		  __p1,                      \
		  a1,                        \
		  ,                          \
		  DOPUS_BASE_NAME,           \
		  0,                         \
		  0,                         \
		  0,                         \
		  0,                         \
		  0,                         \
		  0)

#define RemovedFunc0() LP0NR(30, RemovedFunc0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define RemovedFunc1() LP0NR(1296, RemovedFunc1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define RemovedFunc2() LP0NR(1302, RemovedFunc2, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetCachedDiskObjectNew(__p0, __p1) \
	LP2(1614,                              \
		struct DiskObject *,               \
		GetCachedDiskObjectNew,            \
		char *,                            \
		__p0,                              \
		a0,                                \
		ULONG,                             \
		__p1,                              \
		d0,                                \
		,                                  \
		DOPUS_BASE_NAME,                   \
		0,                                 \
		0,                                 \
		0,                                 \
		0,                                 \
		0,                                 \
		0)

#define RemovedFunc3() LP0NR(1308, RemovedFunc3, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define InitListLock(__p0, __p1) \
	LP2NR(1440, InitListLock, struct ListLock *, __p0, a0, char *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define RemovedFunc4() LP0NR(1314, RemovedFunc4, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FreeRexxMsgEx(__p0) LP1NR(2010, FreeRexxMsgEx, struct RexxMsg *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define SimpleRequest(__p0, __p1, __p2, __p3, __p4, __p5, __p6, __p7) \
	LP8(156,                                                          \
		short,                                                        \
		SimpleRequest,                                                \
		struct Window *,                                              \
		__p0,                                                         \
		a0,                                                           \
		char *,                                                       \
		__p1,                                                         \
		a1,                                                           \
		char *,                                                       \
		__p2,                                                         \
		a2,                                                           \
		char *,                                                       \
		__p3,                                                         \
		a3,                                                           \
		char *,                                                       \
		__p4,                                                         \
		a4,                                                           \
		APTR,                                                         \
		__p5,                                                         \
		a5,                                                           \
		long,                                                         \
		__p6,                                                         \
		d0,                                                           \
		ULONG,                                                        \
		__p7,                                                         \
		d1,                                                           \
		,                                                             \
		DOPUS_BASE_NAME,                                              \
		0,                                                            \
		0,                                                            \
		0,                                                            \
		0,                                                            \
		0,                                                            \
		0)

#define ReadClipString(__p0, __p1, __p2) \
	LP3(1278,                            \
		long,                            \
		ReadClipString,                  \
		struct ClipHandle *,             \
		__p0,                            \
		a0,                              \
		char *,                          \
		__p1,                            \
		a1,                              \
		long,                            \
		__p2,                            \
		d0,                              \
		,                                \
		DOPUS_BASE_NAME,                 \
		0,                               \
		0,                               \
		0,                               \
		0,                               \
		0,                               \
		0)

#define RemovedFunc5() LP0NR(1320, RemovedFunc5, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetWBArgPath(__p0, __p1, __p2) \
	LP3(1470,                          \
		BOOL,                          \
		GetWBArgPath,                  \
		struct WBArg *,                \
		__p0,                          \
		a0,                            \
		char *,                        \
		__p1,                          \
		a1,                            \
		long,                          \
		__p2,                          \
		d0,                            \
		,                              \
		DOPUS_BASE_NAME,               \
		0,                             \
		0,                             \
		0,                             \
		0,                             \
		0,                             \
		0)

#define RemovedFunc6() LP0NR(1326, RemovedFunc6, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define StoreWindowDims(__p0, __p1) \
	LP2NR(1428,                     \
		  StoreWindowDims,          \
		  struct Window *,          \
		  __p0,                     \
		  a0,                       \
		  WindowDimensions *,       \
		  __p1,                     \
		  a1,                       \
		  ,                         \
		  DOPUS_BASE_NAME,          \
		  0,                        \
		  0,                        \
		  0,                        \
		  0,                        \
		  0,                        \
		  0)

#define RemoveDragImage(__p0) LP1NR(2094, RemoveDragImage, DragInfo *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define ReadFunction(__p0, __p1, __p2, __p3) \
	LP4(912,                                 \
		Cfg_Function *,                      \
		ReadFunction,                        \
		APTR,                                \
		__p0,                                \
		a0,                                  \
		APTR,                                \
		__p1,                                \
		a1,                                  \
		struct List *,                       \
		__p2,                                \
		a2,                                  \
		Cfg_Function *,                      \
		__p3,                                \
		a3,                                  \
		,                                    \
		DOPUS_BASE_NAME,                     \
		0,                                   \
		0,                                   \
		0,                                   \
		0,                                   \
		0,                                   \
		0)

#define RemovedFunc7() LP0NR(1332, RemovedFunc7, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define RemovedFunc8() LP0NR(1476, RemovedFunc8, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetIconPosition(__p0, __p1, __p2) \
	LP3NR(1512,                           \
		  GetIconPosition,                \
		  struct DiskObject *,            \
		  __p0,                           \
		  a0,                             \
		  short *,                        \
		  __p1,                           \
		  a1,                             \
		  short *,                        \
		  __p2,                           \
		  a2,                             \
		  ,                               \
		  DOPUS_BASE_NAME,                \
		  0,                              \
		  0,                              \
		  0,                              \
		  0,                              \
		  0,                              \
		  0)

#define RemovedFunc9() LP0NR(1548, RemovedFunc9, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define IFFClose(__p0) LP1NR(1206, IFFClose, APTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetRexxVarEx(__p0, __p1, __p2) \
	LP3(2028,                          \
		long,                          \
		GetRexxVarEx,                  \
		struct RexxMsg *,              \
		__p0,                          \
		a0,                            \
		char *,                        \
		__p1,                          \
		a1,                            \
		char **,                       \
		__p2,                          \
		a2,                            \
		,                              \
		DOPUS_BASE_NAME,               \
		0,                             \
		0,                             \
		0,                             \
		0,                             \
		0,                             \
		0)

#define RandomDopus(__p0) LP1(36, UWORD, RandomDopus, int, __p0, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define ReadFiletypes(__p0, __p1) \
	LP2(864, Cfg_FiletypeList *, ReadFiletypes, char *, __p0, a0, APTR, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define ParseDateStrings(__p0, __p1, __p2, __p3) \
	LP4(1722,                                    \
		char *,                                  \
		ParseDateStrings,                        \
		char *,                                  \
		__p0,                                    \
		a0,                                      \
		char *,                                  \
		__p1,                                    \
		a1,                                      \
		char *,                                  \
		__p2,                                    \
		a2,                                      \
		long *,                                  \
		__p3,                                    \
		a3,                                      \
		,                                        \
		DOPUS_BASE_NAME,                         \
		0,                                       \
		0,                                       \
		0,                                       \
		0,                                       \
		0,                                       \
		0)

#define WB_Launch(__p0, __p1, __p2) \
	LP3(1152,                       \
		BOOL,                       \
		WB_Launch,                  \
		char *,                     \
		__p0,                       \
		a0,                         \
		struct Screen *,            \
		__p1,                       \
		a1,                         \
		short,                      \
		__p2,                       \
		d0,                         \
		,                           \
		DOPUS_BASE_NAME,            \
		0,                          \
		0,                          \
		0,                          \
		0,                          \
		0,                          \
		0)

#define PopUpFreeHandle(__p0) LP1NR(2250, PopUpFreeHandle, PopUpHandle *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FreeTimer(__p0) LP1NR(114, FreeTimer, TimerHandle *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define DecodeRLE(__p0) LP1NR(966, DecodeRLE, RLEinfo *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define NewButton(__p0) LP1(720, Cfg_Button *, NewButton, APTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define DeviceFromLock(__p0, __p1) \
	LP2(1482, struct DosList *, DeviceFromLock, BPTR, __p0, a0, char *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define IFFFailure(__p0) LP1NR(1590, IFFFailure, APTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define SelectionList(__p0, __p1, __p2, __p3, __p4, __p5, __p6, __p7, __p8, __p9, __p10) \
	LP11(162,                                                                            \
		 short,                                                                          \
		 SelectionList,                                                                  \
		 Att_List *,                                                                     \
		 __p0,                                                                           \
		 a0,                                                                             \
		 struct Window *,                                                                \
		 __p1,                                                                           \
		 a1,                                                                             \
		 struct Screen *,                                                                \
		 __p2,                                                                           \
		 a2,                                                                             \
		 char *,                                                                         \
		 __p3,                                                                           \
		 a3,                                                                             \
		 short,                                                                          \
		 __p4,                                                                           \
		 d0,                                                                             \
		 ULONG,                                                                          \
		 __p5,                                                                           \
		 d1,                                                                             \
		 char *,                                                                         \
		 __p6,                                                                           \
		 d2,                                                                             \
		 char *,                                                                         \
		 __p7,                                                                           \
		 d3,                                                                             \
		 char *,                                                                         \
		 __p8,                                                                           \
		 d4,                                                                             \
		 char **,                                                                        \
		 __p9,                                                                           \
		 a4,                                                                             \
		 ULONG *,                                                                        \
		 __p10,                                                                          \
		 a5,                                                                             \
		 ,                                                                               \
		 DOPUS_BASE_NAME,                                                                \
		 0,                                                                              \
		 0,                                                                              \
		 0,                                                                              \
		 0,                                                                              \
		 0,                                                                              \
		 0)

#define ActivateStrGad(__p0, __p1) \
	LP2NR(102,                     \
		  ActivateStrGad,          \
		  struct Gadget *,         \
		  __p0,                    \
		  a0,                      \
		  struct Window *,         \
		  __p1,                    \
		  a1,                      \
		  ,                        \
		  DOPUS_BASE_NAME,         \
		  0,                       \
		  0,                       \
		  0,                       \
		  0,                       \
		  0,                       \
		  0)

#define FreeButton(__p0) LP1NR(816, FreeButton, Cfg_Button *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define UpdateGadgetValue(__p0, __p1, __p2) \
	LP3NR(1824,                             \
		  UpdateGadgetValue,                \
		  ObjectList *,                     \
		  __p0,                             \
		  a0,                               \
		  struct IntuiMessage *,            \
		  __p1,                             \
		  a1,                               \
		  UWORD,                            \
		  __p2,                             \
		  d0,                               \
		  ,                                 \
		  DOPUS_BASE_NAME,                  \
		  0,                                \
		  0,                                \
		  0,                                \
		  0,                                \
		  0,                                \
		  0)

#define GetWindowAppPort(__p0) \
	LP1(432, struct MsgPort *, GetWindowAppPort, struct Window *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define ChecksumFile(__p0, __p1) \
	LP2(1176, ULONG, ChecksumFile, char *, __p0, a0, ULONG, __p1, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define AllocTimer(__p0, __p1) \
	LP2(108,                   \
		struct TimerHandle *,  \
		AllocTimer,            \
		ULONG,                 \
		__p0,                  \
		d0,                    \
		struct MsgPort *,      \
		__p1,                  \
		a0,                    \
		,                      \
		DOPUS_BASE_NAME,       \
		0,                     \
		0,                     \
		0,                     \
		0,                     \
		0,                     \
		0)

#define SaveFunction(__p0, __p1) \
	LP2(1356, BOOL, SaveFunction, APTR, __p0, a0, Cfg_Function *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CheckAppMessage(__p0) \
	LP1(1884, BOOL, CheckAppMessage, DOpusAppMessage *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FreeButtonList(__p0) LP1NR(804, FreeButtonList, struct List *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CopyFiletype(__p0, __p1) \
	LP2(888,                     \
		Cfg_Filetype *,          \
		CopyFiletype,            \
		Cfg_Filetype *,          \
		__p0,                    \
		a0,                      \
		APTR,                    \
		__p1,                    \
		a1,                      \
		,                        \
		DOPUS_BASE_NAME,         \
		0,                       \
		0,                       \
		0,                       \
		0,                       \
		0,                       \
		0)

#define IFFGetFORM(__p0) LP1(1398, ULONG, IFFGetFORM, APTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define NewInstruction(__p0, __p1, __p2) \
	LP3(732,                             \
		Cfg_Instruction *,               \
		NewInstruction,                  \
		APTR,                            \
		__p0,                            \
		a0,                              \
		short,                           \
		__p1,                            \
		d0,                              \
		char *,                          \
		__p2,                            \
		a1,                              \
		,                                \
		DOPUS_BASE_NAME,                 \
		0,                               \
		0,                               \
		0,                               \
		0,                               \
		0,                               \
		0)

#define Att_NodeDataNumber(__p0, __p1) \
	LP2(486, long, Att_NodeDataNumber, Att_List *, __p0, a0, IPTR, __p1, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define NewButtonWithFunc(__p0, __p1, __p2) \
	LP3(1686,                               \
		Cfg_Button *,                       \
		NewButtonWithFunc,                  \
		APTR,                               \
		__p0,                               \
		a0,                                 \
		char *,                             \
		__p1,                               \
		a1,                                 \
		short,                              \
		__p2,                               \
		d0,                                 \
		,                                   \
		DOPUS_BASE_NAME,                    \
		0,                                  \
		0,                                  \
		0,                                  \
		0,                                  \
		0,                                  \
		0)

#define SetWindowID(__p0, __p1, __p2, __p3) \
	LP4NR(420,                              \
		  SetWindowID,                      \
		  struct Window *,                  \
		  __p0,                             \
		  a0,                               \
		  WindowID *,                       \
		  __p1,                             \
		  a1,                               \
		  ULONG,                            \
		  __p2,                             \
		  d0,                               \
		  struct MsgPort *,                 \
		  __p3,                             \
		  a2,                               \
		  ,                                 \
		  DOPUS_BASE_NAME,                  \
		  0,                                \
		  0,                                \
		  0,                                \
		  0,                                \
		  0,                                \
		  0)

#define FlushBuf(__p0) LP1(1008, long, FlushBuf, APTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define SetWBArg(__p0, __p1, __p2, __p3, __p4) \
	LP5(1896,                                  \
		BOOL,                                  \
		SetWBArg,                              \
		DOpusAppMessage *,                     \
		__p0,                                  \
		a0,                                    \
		short,                                 \
		__p1,                                  \
		d0,                                    \
		BPTR,                                  \
		__p2,                                  \
		d1,                                    \
		char *,                                \
		__p3,                                  \
		a1,                                    \
		APTR,                                  \
		__p4,                                  \
		a2,                                    \
		,                                      \
		DOPUS_BASE_NAME,                       \
		0,                                     \
		0,                                     \
		0,                                     \
		0,                                     \
		0,                                     \
		0)

#define OpenButtonBank(__p0) \
	LP1(750, Cfg_ButtonBank *, OpenButtonBank, char *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FreeMatchHandle(__p0) LP1NR(1740, FreeMatchHandle, APTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define Att_FindNodeNumber(__p0, __p1) \
	LP2(510, long, Att_FindNodeNumber, Att_List *, __p0, a0, Att_Node *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CheckProgressAbort(__p0) \
	LP1(1674, BOOL, CheckProgressAbort, APTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetPopUpItem(__p0, __p1) \
	LP2(540, PopUpItem *, GetPopUpItem, PopUpMenu *, __p0, a0, UWORD, __p1, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define UnlockAttList(__p0) LP1NR(1290, UnlockAttList, Att_List *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define SetIconPosition(__p0, __p1, __p2) \
	LP3NR(1518,                           \
		  SetIconPosition,                \
		  struct DiskObject *,            \
		  __p0,                           \
		  a0,                             \
		  short,                          \
		  __p1,                           \
		  d0,                             \
		  short,                          \
		  __p2,                           \
		  d1,                             \
		  ,                               \
		  DOPUS_BASE_NAME,                \
		  0,                              \
		  0,                              \
		  0,                              \
		  0,                              \
		  0,                              \
		  0)

#define ReadBufLine(__p0, __p1, __p2) \
	LP3(2106, long, ReadBufLine, APTR, __p0, a0, char *, __p1, a1, long, __p2, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define SetPopUpDelay(__p0) LP1NR(2220, SetPopUpDelay, short, __p0, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CompareListFormat(__p0, __p1) \
	LP2(1818,                         \
		ULONG,                        \
		CompareListFormat,            \
		ListFormat *,                 \
		__p0,                         \
		a0,                           \
		ListFormat *,                 \
		__p1,                         \
		a1,                           \
		,                             \
		DOPUS_BASE_NAME,              \
		0,                            \
		0,                            \
		0,                            \
		0,                            \
		0,                            \
		0)

#define ReadILBM(__p0, __p1) \
	LP2(948, ILBMHandle *, ReadILBM, char *, __p0, a0, ULONG, __p1, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define OpenEnvironment(__p0, __p1)   \
	LP2(2238,                         \
		BOOL,                         \
		OpenEnvironment,              \
		char *,                       \
		__p0,                         \
		a0,                           \
		struct OpenEnvironmentData *, \
		__p1,                         \
		a1,                           \
		,                             \
		DOPUS_BASE_NAME,              \
		0,                            \
		0,                            \
		0,                            \
		0,                            \
		0,                            \
		0)

#define ClearFiletypeCache() LP0NR(1992, ClearFiletypeCache, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define IPC_Hello(__p0, __p1) \
	LP2NR(582, IPC_Hello, IPCData *, __p0, a0, IPCData *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetDragMask(__p0) LP1NR(216, GetDragMask, DragInfo *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CalcPercent(__p0, __p1, __p2) \
	LP3(2076,                         \
		long,                         \
		CalcPercent,                  \
		ULONG,                        \
		__p0,                         \
		d0,                           \
		ULONG,                        \
		__p1,                         \
		d1,                           \
		struct Library *,             \
		__p2,                         \
		a0,                           \
		,                             \
		DOPUS_BASE_NAME,              \
		0,                            \
		0,                            \
		0,                            \
		0,                            \
		0,                            \
		0)

#define ShowSemaphore(__p0) \
	LP1NR(1350, ShowSemaphore, struct SignalSemaphore *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define NewButtonBank(__p0, __p1) \
	LP2(714, Cfg_ButtonBank *, NewButtonBank, BOOL, __p0, d0, short, __p1, d1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define IFFWriteChunkBytes(__p0, __p1, __p2) \
	LP3(1218,                                \
		long,                                \
		IFFWriteChunkBytes,                  \
		APTR,                                \
		__p0,                                \
		a0,                                  \
		APTR,                                \
		__p1,                                \
		a1,                                  \
		long,                                \
		__p2,                                \
		d0,                                  \
		,                                    \
		DOPUS_BASE_NAME,                     \
		0,                                   \
		0,                                   \
		0,                                   \
		0,                                   \
		0,                                   \
		0)

#define AllocAppMessage(__p0, __p1, __p2) \
	LP3(1878,                             \
		DOpusAppMessage *,                \
		AllocAppMessage,                  \
		APTR,                             \
		__p0,                             \
		a0,                               \
		struct MsgPort *,                 \
		__p1,                             \
		a1,                               \
		short,                            \
		__p2,                             \
		d0,                               \
		,                                 \
		DOPUS_BASE_NAME,                  \
		0,                                \
		0,                                \
		0,                                \
		0,                                \
		0,                                \
		0)

#define RemAllocBitmapPatch(__p0) LP1NR(2292, RemAllocBitmapPatch, APTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define DateFromStringsNew(__p0, __p1, __p2, __p3) \
	LP4(2058,                                      \
		BOOL,                                      \
		DateFromStringsNew,                        \
		char *,                                    \
		__p0,                                      \
		a0,                                        \
		char *,                                    \
		__p1,                                      \
		a1,                                        \
		struct DateStamp *,                        \
		__p2,                                      \
		a2,                                        \
		ULONG,                                     \
		__p3,                                      \
		d0,                                        \
		,                                          \
		DOPUS_BASE_NAME,                           \
		0,                                         \
		0,                                         \
		0,                                         \
		0,                                         \
		0,                                         \
		0)

#define LoadPalette32(__p0, __p1) \
	LP2NR(972, LoadPalette32, struct ViewPort *, __p0, a0, ULONG *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetDeviceUnit(__p0, __p1, __p2) \
	LP3(2172,                           \
		BOOL,                           \
		GetDeviceUnit,                  \
		BPTR,                           \
		__p0,                           \
		a0,                             \
		char *,                         \
		__p1,                           \
		a1,                             \
		short *,                        \
		__p2,                           \
		a2,                             \
		,                               \
		DOPUS_BASE_NAME,                \
		0,                              \
		0,                              \
		0,                              \
		0,                              \
		0,                              \
		0)

#define GetMatchHandle(__p0) LP1(1734, APTR, GetMatchHandle, char *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CopyButtonFunction(__p0, __p1, __p2) \
	LP3(1698,                                \
		Cfg_ButtonFunction *,                \
		CopyButtonFunction,                  \
		Cfg_ButtonFunction *,                \
		__p0,                                \
		a0,                                  \
		APTR,                                \
		__p1,                                \
		a1,                                  \
		Cfg_ButtonFunction *,                \
		__p2,                                \
		a2,                                  \
		,                                    \
		DOPUS_BASE_NAME,                     \
		0,                                   \
		0,                                   \
		0,                                   \
		0,                                   \
		0,                                   \
		0)

#define GetGadgetValue(__p0, __p1) \
	LP2(330, IPTR, GetGadgetValue, ObjectList *, __p0, a0, UWORD, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define ReadButton(__p0, __p1) \
	LP2(906, Cfg_Button *, ReadButton, APTR, __p0, a0, APTR, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define WriteBuf(__p0, __p1, __p2) \
	LP3(1002, long, WriteBuf, APTR, __p0, a0, const char *, __p1, a1, long, __p2, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define WB_FindAppWindow(__p0) \
	LP1(1086, struct AppWindow *, WB_FindAppWindow, struct Window *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FindNameI(__p0, __p1) \
	LP2(1236, struct Node *, FindNameI, struct List *, __p0, a0, char *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define IFFOpen(__p0, __p1, __p2) \
	LP3(1200, APTR, IFFOpen, char *, __p0, a0, UWORD, __p1, d0, ULONG, __p2, d1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define BOOPSIFree(__p0) LP1NR(1050, BOOPSIFree, struct List *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FreeDosPathList(__p0) LP1NR(144, FreeDosPathList, BPTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define SaveListerDef(__p0, __p1) \
	LP2(780, long, SaveListerDef, APTR, __p0, a0, Cfg_Lister *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define DivideToString(__p0, __p1, __p2, __p3, __p4) \
	LP5NR(84,                                        \
		  DivideToString,                            \
		  char *,                                    \
		  __p0,                                      \
		  a0,                                        \
		  ULONG,                                     \
		  __p1,                                      \
		  d0,                                        \
		  ULONG,                                     \
		  __p2,                                      \
		  d1,                                        \
		  short,                                     \
		  __p3,                                      \
		  d2,                                        \
		  char,                                      \
		  __p4,                                      \
		  d3,                                        \
		  ,                                          \
		  DOPUS_BASE_NAME,                           \
		  0,                                         \
		  0,                                         \
		  0,                                         \
		  0,                                         \
		  0,                                         \
		  0)

#define DivideToString64(___string, ___str_size, ___bytes, ___div, ___places, ___sep) \
	LP6NR(0x912,                                                                      \
		  DivideToString64,                                                           \
		  char *,                                                                     \
		  ___string,                                                                  \
		  a0,                                                                         \
		  int,                                                                        \
		  ___str_size,                                                                \
		  d0,                                                                         \
		  UQUAD *,                                                                    \
		  ___bytes,                                                                   \
		  a1,                                                                         \
		  ULONG,                                                                      \
		  ___div,                                                                     \
		  d1,                                                                         \
		  int,                                                                        \
		  ___places,                                                                  \
		  d2,                                                                         \
		  char,                                                                       \
		  ___sep,                                                                     \
		  d3,                                                                         \
		  ,                                                                           \
		  DOPUS_BASE_NAME,                                                            \
		  IF_CACHEFLUSHALL,                                                           \
		  NULL,                                                                       \
		  0,                                                                          \
		  IF_CACHEFLUSHALL,                                                           \
		  NULL,                                                                       \
		  0)

#define AddDragImage(__p0) LP1NR(228, AddDragImage, DragInfo *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CLI_Launch(__p0, __p1, __p2, __p3, __p4, __p5, __p6) \
	LP7(1164,                                                \
		BOOL,                                                \
		CLI_Launch,                                          \
		char *,                                              \
		__p0,                                                \
		a0,                                                  \
		struct Screen *,                                     \
		__p1,                                                \
		a1,                                                  \
		BPTR,                                                \
		__p2,                                                \
		d0,                                                  \
		BPTR,                                                \
		__p3,                                                \
		d1,                                                  \
		BPTR,                                                \
		__p4,                                                \
		d2,                                                  \
		short,                                               \
		__p5,                                                \
		d3,                                                  \
		long,                                                \
		__p6,                                                \
		d4,                                                  \
		,                                                    \
		DOPUS_BASE_NAME,                                     \
		0,                                                   \
		0,                                                   \
		0,                                                   \
		0,                                                   \
		0,                                                   \
		0)

#define OriginalWrite(__p0, __p1, __p2) \
	LP3(1950,                           \
		LONG,                           \
		OriginalWrite,                  \
		BPTR,                           \
		__p0,                           \
		d1,                             \
		void *,                         \
		__p1,                           \
		d2,                             \
		LONG,                           \
		__p2,                           \
		d3,                             \
		,                               \
		DOPUS_BASE_NAME,                \
		0,                              \
		0,                              \
		0,                              \
		0,                              \
		0,                              \
		0)

#define CloseProgressWindow(__p0) LP1NR(1632, CloseProgressWindow, APTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetProgressWindow(__p0, __p1) \
	LP2NR(1656, GetProgressWindow, APTR, __p0, a0, struct TagItem *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define OpenConfigWindow(__p0) \
	LP1(240, struct Window *, OpenConfigWindow, NewConfigWindow *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CloseWindowSafely(__p0) \
	LP1NR(270, CloseWindowSafely, struct Window *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define RemapImage(__p0, __p1, __p2) \
	LP3(1776,                        \
		BOOL,                        \
		RemapImage,                  \
		APTR,                        \
		__p0,                        \
		a0,                          \
		struct Screen *,             \
		__p1,                        \
		a1,                          \
		ImageRemap *,                \
		__p2,                        \
		a2,                          \
		,                            \
		DOPUS_BASE_NAME,             \
		0,                           \
		0,                           \
		0,                           \
		0,                           \
		0,                           \
		0)

#define FreeListerDef(__p0) LP1NR(798, FreeListerDef, Cfg_Lister *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define SetNewIconsFlags(__p0, __p1) \
	LP2NR(2100, SetNewIconsFlags, ULONG, __p0, d0, short, __p1, d1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define LockAppList() LP0(1104, APTR, LockAppList, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define ClearWindowBusy(__p0) \
	LP1NR(396, ClearWindowBusy, struct Window *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define UnlockReqBackFill() LP0NR(2136, UnlockReqBackFill, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define DisplayObject(__p0, __p1, __p2, __p3, __p4) \
	LP5NR(360,                                      \
		  DisplayObject,                            \
		  struct Window *,                          \
		  __p0,                                     \
		  a0,                                       \
		  GL_Object *,                              \
		  __p1,                                     \
		  a1,                                       \
		  int,                                      \
		  __p2,                                     \
		  d0,                                       \
		  int,                                      \
		  __p3,                                     \
		  d1,                                       \
		  char *,                                   \
		  __p4,                                     \
		  a2,                                       \
		  ,                                         \
		  DOPUS_BASE_NAME,                          \
		  0,                                        \
		  0,                                        \
		  0,                                        \
		  0,                                        \
		  0,                                        \
		  0)

#define AnimDecodeRIFFSet(__p0, __p1, __p2, __p3) \
	LP4NR(1248,                                   \
		  AnimDecodeRIFFSet,                      \
		  unsigned char *,                        \
		  __p0,                                   \
		  a0,                                     \
		  char *,                                 \
		  __p1,                                   \
		  a1,                                     \
		  UWORD,                                  \
		  __p2,                                   \
		  d0,                                     \
		  UWORD,                                  \
		  __p3,                                   \
		  d1,                                     \
		  ,                                       \
		  DOPUS_BASE_NAME,                        \
		  0,                                      \
		  0,                                      \
		  0,                                      \
		  0,                                      \
		  0,                                      \
		  0)

#define CloseClipBoard(__p0) \
	LP1NR(1266, CloseClipBoard, struct ClipHandle *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define OriginalRename(__p0, __p1) \
	LP2(1932, BOOL, OriginalRename, char *, __p0, d1, char *, __p1, d2, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define WB_AppWindowWindow(__p0) \
	LP1(2232, struct Window *, WB_AppWindowWindow, struct AppWindow *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetDosPathList(__p0) LP1(138, BPTR, GetDosPathList, BPTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define IPC_Startup(__p0, __p1, __p2) \
	LP3(546,                          \
		int,                          \
		IPC_Startup,                  \
		IPCData *,                    \
		__p0,                         \
		a0,                           \
		APTR,                         \
		__p1,                         \
		a1,                           \
		struct MsgPort *,             \
		__p2,                         \
		a2,                           \
		,                             \
		DOPUS_BASE_NAME,              \
		0,                            \
		0,                            \
		0,                            \
		0,                            \
		0,                            \
		0)

#define DefaultEnvironment(__p0) \
	LP1NR(762, DefaultEnvironment, CFG_ENVR *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetCachedDefDiskObject(__p0) \
	LP1(1596, struct DiskObject *, GetCachedDefDiskObject, long, __p0, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetStatistics(__p0) LP1(2214, long, GetStatistics, long, __p0, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetObjectRect(__p0, __p1, __p2) \
	LP3(354,                            \
		BOOL,                           \
		GetObjectRect,                  \
		ObjectList *,                   \
		__p0,                           \
		a0,                             \
		ULONG,                          \
		__p1,                           \
		d0,                             \
		struct Rectangle *,             \
		__p2,                           \
		a1,                             \
		,                               \
		DOPUS_BASE_NAME,                \
		0,                              \
		0,                              \
		0,                              \
		0,                              \
		0,                              \
		0)

#define FixTitleGadgets(__p0) \
	LP1NR(1968, FixTitleGadgets, struct Window *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define WriteClipString(__p0, __p1, __p2) \
	LP3(1272,                             \
		BOOL,                             \
		WriteClipString,                  \
		struct ClipHandle *,              \
		__p0,                             \
		a0,                               \
		char *,                           \
		__p1,                             \
		a1,                               \
		long,                             \
		__p2,                             \
		d0,                               \
		,                                 \
		DOPUS_BASE_NAME,                  \
		0,                                \
		0,                                \
		0,                                \
		0,                                \
		0,                                \
		0)

#define CreateTitleGadget(__p0, __p1, __p2, __p3, __p4, __p5) \
	LP6(1956,                                                 \
		struct Gadget *,                                      \
		CreateTitleGadget,                                    \
		struct Screen *,                                      \
		__p0,                                                 \
		a0,                                                   \
		struct List *,                                        \
		__p1,                                                 \
		a1,                                                   \
		BOOL,                                                 \
		__p2,                                                 \
		d0,                                                   \
		short,                                                \
		__p3,                                                 \
		d1,                                                   \
		short,                                                \
		__p4,                                                 \
		d2,                                                   \
		UWORD,                                                \
		__p5,                                                 \
		d3,                                                   \
		,                                                     \
		DOPUS_BASE_NAME,                                      \
		0,                                                    \
		0,                                                    \
		0,                                                    \
		0,                                                    \
		0,                                                    \
		0)

#define Att_NodeCount(__p0) LP1(498, long, Att_NodeCount, Att_List *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FreeRemapImage(__p0, __p1) \
	LP2NR(1782, FreeRemapImage, APTR, __p0, a0, ImageRemap *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define StripWindowMessages(__p0, __p1) \
	LP2NR(2178,                         \
		  StripWindowMessages,          \
		  struct MsgPort *,             \
		  __p0,                         \
		  a0,                           \
		  struct IntuiMessage *,        \
		  __p1,                         \
		  a1,                           \
		  ,                             \
		  DOPUS_BASE_NAME,              \
		  0,                            \
		  0,                            \
		  0,                            \
		  0,                            \
		  0,                            \
		  0)

#define DisposeArgs(__p0) LP1NR(1854, DisposeArgs, FuncArgs *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetDragInfo(__p0, __p1, __p2, __p3, __p4) \
	LP5(180,                                      \
		DragInfo *,                               \
		GetDragInfo,                              \
		struct Window *,                          \
		__p0,                                     \
		a0,                                       \
		struct RastPort *,                        \
		__p1,                                     \
		a1,                                       \
		long,                                     \
		__p2,                                     \
		d0,                                       \
		long,                                     \
		__p3,                                     \
		d1,                                       \
		long,                                     \
		__p4,                                     \
		d2,                                       \
		,                                         \
		DOPUS_BASE_NAME,                          \
		0,                                        \
		0,                                        \
		0,                                        \
		0,                                        \
		0,                                        \
		0)

#define FreeMemHandle(__p0) LP1NR(672, FreeMemHandle, void *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FindFunctionType(__p0, __p1) \
	LP2(894,                         \
		Cfg_Function *,              \
		FindFunctionType,            \
		struct List *,               \
		__p0,                        \
		a0,                          \
		UWORD,                       \
		__p1,                        \
		d0,                          \
		,                            \
		DOPUS_BASE_NAME,             \
		0,                           \
		0,                           \
		0,                           \
		0,                           \
		0,                           \
		0)

#define IPC_Reply(__p0) LP1NR(558, IPC_Reply, IPCMessage *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define NextAppEntry(__p0, __p1) \
	LP2(1110, APTR, NextAppEntry, APTR, __p0, a0, ULONG, __p1, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define SendNotifyMsg(__p0, __p1, __p2, __p3, __p4, __p5) \
	LP6NR(1134,                                           \
		  SendNotifyMsg,                                  \
		  ULONG,                                          \
		  __p0,                                           \
		  d0,                                             \
		  IPTR,                                           \
		  __p1,                                           \
		  d1,                                             \
		  ULONG,                                          \
		  __p2,                                           \
		  d2,                                             \
		  short,                                          \
		  __p3,                                           \
		  d3,                                             \
		  char *,                                         \
		  __p4,                                           \
		  a0,                                             \
		  struct FileInfoBlock *,                         \
		  __p5,                                           \
		  a1,                                             \
		  ,                                               \
		  DOPUS_BASE_NAME,                                \
		  0,                                              \
		  0,                                              \
		  0,                                              \
		  0,                                              \
		  0,                                              \
		  0)

#define DeviceFromHandler(__p0, __p1) \
	LP2(1488,                         \
		struct DosList *,             \
		DeviceFromHandler,            \
		struct MsgPort *,             \
		__p0,                         \
		a0,                           \
		char *,                       \
		__p1,                         \
		a1,                           \
		,                             \
		DOPUS_BASE_NAME,              \
		0,                            \
		0,                            \
		0,                            \
		0,                            \
		0,                            \
		0)

#define NewFiletype(__p0) LP1(858, Cfg_Filetype *, NewFiletype, APTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define UpdateMyPaths() LP0NR(2160, UpdateMyPaths, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define Ito26(__p0, __p1) LP2NR(72, Ito26, ULONG, __p0, d0, char *, __p1, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define IFFReadChunkBytes(__p0, __p1, __p2) \
	LP3(1374,                               \
		long,                               \
		IFFReadChunkBytes,                  \
		APTR,                               \
		__p0,                               \
		a0,                                 \
		APTR,                               \
		__p1,                               \
		a1,                                 \
		long,                               \
		__p2,                               \
		d0,                                 \
		,                                   \
		DOPUS_BASE_NAME,                    \
		0,                                  \
		0,                                  \
		0,                                  \
		0,                                  \
		0,                                  \
		0)

#define SetNotifyRequest(__p0, __p1, __p2) \
	LP3NR(1662, SetNotifyRequest, APTR, __p0, a0, ULONG, __p1, d0, ULONG, __p2, d1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define OriginalSetFileDate(__p0, __p1) \
	LP2(1914,                           \
		BOOL,                           \
		OriginalSetFileDate,            \
		char *,                         \
		__p0,                           \
		d1,                             \
		struct DateStamp *,             \
		__p1,                           \
		d2,                             \
		,                               \
		DOPUS_BASE_NAME,                \
		0,                              \
		0,                              \
		0,                              \
		0,                              \
		0,                              \
		0)

#define OriginalClose(__p0) LP1(1944, BOOL, OriginalClose, BPTR, __p0, d1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define IPC_Free(__p0) LP1NR(564, IPC_Free, IPCData *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define OriginalCreateDir(__p0) \
	LP1(1902, BPTR, OriginalCreateDir, char *, __p0, d1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CopyButtonBank(__p0) \
	LP1(840, Cfg_ButtonBank *, CopyButtonBank, Cfg_ButtonBank *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CopyLocalEnv(__p0) LP1NR(1158, CopyLocalEnv, struct Library *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define WB_AppWindowLocal(__p0) \
	LP1(1098, BOOL, WB_AppWindowLocal, struct AppWindow *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CopyAppMessage(__p0, __p1) \
	LP2(1890,                      \
		DOpusAppMessage *,         \
		CopyAppMessage,            \
		DOpusAppMessage *,         \
		__p0,                      \
		a0,                        \
		APTR,                      \
		__p1,                      \
		a1,                        \
		,                          \
		DOPUS_BASE_NAME,           \
		0,                         \
		0,                         \
		0,                         \
		0,                         \
		0,                         \
		0)

#define UpdateStatusGraph(__p0, __p1, __p2, __p3) \
	LP4NR(942,                                    \
		  UpdateStatusGraph,                      \
		  struct Window *,                        \
		  __p0,                                   \
		  a0,                                     \
		  char *,                                 \
		  __p1,                                   \
		  a1,                                     \
		  ULONG,                                  \
		  __p2,                                   \
		  d0,                                     \
		  ULONG,                                  \
		  __p3,                                   \
		  d1,                                     \
		  ,                                       \
		  DOPUS_BASE_NAME,                        \
		  0,                                      \
		  0,                                      \
		  0,                                      \
		  0,                                      \
		  0,                                      \
		  0)

#define AddWindowMenus(__p0, __p1) \
	LP2NR(378, AddWindowMenus, struct Window *, __p0, a0, const MenuData *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetWindowMsg(__p0) \
	LP1(252, struct IntuiMessage *, GetWindowMsg, struct MsgPort *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define OriginalRelabel(__p0, __p1) \
	LP2(1974, BOOL, OriginalRelabel, char *, __p0, d1, char *, __p1, d2, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define IFFChunkSize(__p0) LP1(1368, long, IFFChunkSize, APTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define SaveButton(__p0, __p1) \
	LP2(900, short, SaveButton, APTR, __p0, a0, Cfg_Button *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define BuildKeyString(__p0, __p1, __p2, __p3, __p4) \
	LP5NR(96,                                        \
		  BuildKeyString,                            \
		  UWORD,                                     \
		  __p0,                                      \
		  d0,                                        \
		  UWORD,                                     \
		  __p1,                                      \
		  d1,                                        \
		  UWORD,                                     \
		  __p2,                                      \
		  d2,                                        \
		  UWORD,                                     \
		  __p3,                                      \
		  d3,                                        \
		  char *,                                    \
		  __p4,                                      \
		  a0,                                        \
		  ,                                          \
		  DOPUS_BASE_NAME,                           \
		  0,                                         \
		  0,                                         \
		  0,                                         \
		  0,                                         \
		  0,                                         \
		  0)

#define PopUpSeparator(__p0) LP1NR(2262, PopUpSeparator, PopUpHandle *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define IPC_SafeCommand(__p0, __p1, __p2, __p3, __p4, __p5, __p6) \
	LP7(1986,                                                     \
		IPTR,                                                     \
		IPC_SafeCommand,                                          \
		IPCData *,                                                \
		__p0,                                                     \
		a0,                                                       \
		ULONG,                                                    \
		__p1,                                                     \
		d0,                                                       \
		IPTR,                                                     \
		__p2,                                                     \
		d1,                                                       \
		APTR,                                                     \
		__p3,                                                     \
		a1,                                                       \
		APTR,                                                     \
		__p4,                                                     \
		a2,                                                       \
		struct MsgPort *,                                         \
		__p5,                                                     \
		a3,                                                       \
		struct ListLock *,                                        \
		__p6,                                                     \
		a4,                                                       \
		,                                                         \
		DOPUS_BASE_NAME,                                          \
		0,                                                        \
		0,                                                        \
		0,                                                        \
		0,                                                        \
		0,                                                        \
		0)

#define RemapIcon(__p0, __p1, __p2) \
	LP3(2064,                       \
		BOOL,                       \
		RemapIcon,                  \
		struct DiskObject *,        \
		__p0,                       \
		a0,                         \
		struct Screen *,            \
		__p1,                       \
		a1,                         \
		short,                      \
		__p2,                       \
		d0,                         \
		,                           \
		DOPUS_BASE_NAME,            \
		0,                          \
		0,                          \
		0,                          \
		0,                          \
		0,                          \
		0)

#define FreeAppMessage(__p0) \
	LP1NR(1788, FreeAppMessage, DOpusAppMessage *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define PopUpNewItem(__p0, __p1, __p2, __p3) \
	LP4(2256,                                \
		PopUpItem *,                         \
		PopUpNewItem,                        \
		PopUpHandle *,                       \
		__p0,                                \
		a0,                                  \
		IPTR,                                \
		__p1,                                \
		d0,                                  \
		ULONG,                               \
		__p2,                                \
		d1,                                  \
		ULONG,                               \
		__p3,                                \
		d2,                                  \
		,                                    \
		DOPUS_BASE_NAME,                     \
		0,                                   \
		0,                                   \
		0,                                   \
		0,                                   \
		0,                                   \
		0)

#define FreeSemaphore(__p0) \
	LP1NR(1344, FreeSemaphore, struct SignalSemaphore *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define ReplyWindowMsg(__p0) \
	LP1NR(258, ReplyWindowMsg, struct IntuiMessage *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define IFFChunkID(__p0) LP1(1392, ULONG, IFFChunkID, APTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FindBOOPSIGadget(__p0, __p1) \
	LP2(1044,                        \
		struct Gadget *,             \
		FindBOOPSIGadget,            \
		struct List *,               \
		__p0,                        \
		a0,                          \
		UWORD,                       \
		__p1,                        \
		d0,                          \
		,                            \
		DOPUS_BASE_NAME,             \
		0,                           \
		0,                           \
		0,                           \
		0,                           \
		0,                           \
		0)

#define ScreenInfo(__p0) LP1(1404, ULONG, ScreenInfo, struct Screen *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define WB_AppWindowData(__p0, __p1, __p2) \
	LP3(1092,                              \
		struct MsgPort *,                  \
		WB_AppWindowData,                  \
		struct AppWindow *,                \
		__p0,                              \
		a0,                                \
		IPTR *,                            \
		__p1,                              \
		a1,                                \
		IPTR *,                            \
		__p2,                              \
		a2,                                \
		,                                  \
		DOPUS_BASE_NAME,                   \
		0,                                 \
		0,                                 \
		0,                                 \
		0,                                 \
		0,                                 \
		0)

#define RenderImage(__p0, __p1, __p2, __p3, __p4) \
	LP5(654,                                      \
		short,                                    \
		RenderImage,                              \
		struct RastPort *,                        \
		__p0,                                     \
		a0,                                       \
		APTR,                                     \
		__p1,                                     \
		a1,                                       \
		UWORD,                                    \
		__p2,                                     \
		d0,                                       \
		UWORD,                                    \
		__p3,                                     \
		d1,                                       \
		struct TagItem *,                         \
		__p4,                                     \
		a2,                                       \
		,                                         \
		DOPUS_BASE_NAME,                          \
		0,                                        \
		0,                                        \
		0,                                        \
		0,                                        \
		0,                                        \
		0)

#define FreeDragInfo(__p0) LP1NR(186, FreeDragInfo, DragInfo *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define IPC_FindProc(__p0, __p1, __p2, __p3) \
	LP4(570,                                 \
		IPCData *,                           \
		IPC_FindProc,                        \
		struct ListLock *,                   \
		__p0,                                \
		a0,                                  \
		char *,                              \
		__p1,                                \
		a1,                                  \
		BOOL,                                \
		__p2,                                \
		d0,                                  \
		IPTR,                                \
		__p3,                                \
		d1,                                  \
		,                                    \
		DOPUS_BASE_NAME,                     \
		0,                                   \
		0,                                   \
		0,                                   \
		0,                                   \
		0,                                   \
		0)

#define FreeInstructionList(__p0) \
	LP1NR(834, FreeInstructionList, Cfg_Function *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FreeFiletype(__p0) LP1NR(882, FreeFiletype, Cfg_Filetype *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define WriteIcon(__p0, __p1) \
	LP2(168, BOOL, WriteIcon, char *, __p0, a0, struct DiskObject *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define IPC_Quit(__p0, __p1, __p2) \
	LP3NR(576, IPC_Quit, IPCData *, __p0, a0, ULONG, __p1, d0, BOOL, __p2, d1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define BtoCStr(__p0, __p1, __p2) \
	LP3NR(48, BtoCStr, BSTR, __p0, a0, char *, __p1, a1, int, __p2, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetImageAttrs(__p0, __p1) \
	LP2NR(660, GetImageAttrs, APTR, __p0, a0, struct TagItem *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CheckRefreshMsg(__p0, __p1) \
	LP2(1770,                       \
		struct IntuiMessage *,      \
		CheckRefreshMsg,            \
		struct Window *,            \
		__p0,                       \
		a0,                         \
		ULONG,                      \
		__p1,                       \
		d0,                         \
		,                           \
		DOPUS_BASE_NAME,            \
		0,                          \
		0,                          \
		0,                          \
		0,                          \
		0,                          \
		0)

#define OriginalOpen(__p0, __p1) \
	LP2(1938, BPTR, OriginalOpen, char *, __p0, d1, LONG, __p1, d2, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FindGadgetType(__p0, __p1) \
	LP2(1962,                      \
		struct Gadget *,           \
		FindGadgetType,            \
		struct Gadget *,           \
		__p0,                      \
		a0,                        \
		UWORD,                     \
		__p1,                      \
		d0,                        \
		,                          \
		DOPUS_BASE_NAME,           \
		0,                         \
		0,                         \
		0,                         \
		0,                         \
		0,                         \
		0)

#define ReadListerDef(__p0, __p1) \
	LP2(744, Cfg_Lister *, ReadListerDef, APTR, __p0, a0, ULONG, __p1, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define IFFPopChunk(__p0) LP1(1224, long, IFFPopChunk, APTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define PopUpNewHandle(__p0, __p1, __p2) \
	LP3(2244,                            \
		PopUpHandle *,                   \
		PopUpNewHandle,                  \
		IPTR,                            \
		__p0,                            \
		d0,                              \
		REF_CALLBACK,                    \
		__p1,                            \
		a0,                              \
		struct DOpusLocale *,            \
		__p2,                            \
		a1,                              \
		,                                \
		DOPUS_BASE_NAME,                 \
		0,                               \
		0,                               \
		0,                               \
		0,                               \
		0,                               \
		0)

#define GetSemaphore(__p0, __p1, __p2) \
	LP3(1338,                          \
		long,                          \
		GetSemaphore,                  \
		struct SignalSemaphore *,      \
		__p0,                          \
		a0,                            \
		long,                          \
		__p1,                          \
		d0,                            \
		char *,                        \
		__p2,                          \
		a1,                            \
		,                              \
		DOPUS_BASE_NAME,               \
		0,                             \
		0,                             \
		0,                             \
		0,                             \
		0,                             \
		0)

#define SeekBuf(__p0, __p1, __p2) \
	LP3(1014, long, SeekBuf, APTR, __p0, a0, long, __p1, d0, long, __p2, d1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define IPC_ProcStartup(__p0, __p1)       \
	LP2FP(618,                            \
		  IPCData *,                      \
		  IPC_ProcStartup,                \
		  ULONG *,                        \
		  __p0,                           \
		  a0,                             \
		  __ft,                           \
		  __p1,                           \
		  a1,                             \
		  ,                               \
		  DOPUS_BASE_NAME,                \
		  ULONG (*__ft)(IPCData *, APTR), \
		  0,                              \
		  0,                              \
		  0,                              \
		  0,                              \
		  0,                              \
		  0)

#define IFFWriteChunk(__p0, __p1, __p2, __p3) \
	LP4(1230,                                 \
		long,                                 \
		IFFWriteChunk,                        \
		APTR,                                 \
		__p0,                                 \
		a0,                                   \
		APTR,                                 \
		__p1,                                 \
		a1,                                   \
		ULONG,                                \
		__p2,                                 \
		d0,                                   \
		ULONG,                                \
		__p3,                                 \
		d1,                                   \
		,                                     \
		DOPUS_BASE_NAME,                      \
		0,                                    \
		0,                                    \
		0,                                    \
		0,                                    \
		0,                                    \
		0)

#define CheckDragDeadlock(__p0) \
	LP1(222, BOOL, CheckDragDeadlock, DragInfo *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define Att_FindNode(__p0, __p1) \
	LP2(468, Att_Node *, Att_FindNode, Att_List *, __p0, a0, long, __p1, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FlushImages() LP0NR(648, FlushImages, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define RemoveNotifyRequest(__p0) LP1NR(1128, RemoveNotifyRequest, APTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define WB_Remove_Patch() LP0(1068, BOOL, WB_Remove_Patch, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define ShowProgressBar(__p0, __p1, __p2, __p3) \
	LP4NR(414,                                  \
		  ShowProgressBar,                      \
		  struct Window *,                      \
		  __p0,                                 \
		  a0,                                   \
		  GL_Object *,                          \
		  __p1,                                 \
		  a1,                                   \
		  ULONG,                                \
		  __p2,                                 \
		  d0,                                   \
		  ULONG,                                \
		  __p3,                                 \
		  d1,                                   \
		  ,                                     \
		  DOPUS_BASE_NAME,                      \
		  0,                                    \
		  0,                                    \
		  0,                                    \
		  0,                                    \
		  0,                                    \
		  0)

#define GetCachedDiskObject(__p0, __p1) \
	LP2(1608,                           \
		struct DiskObject *,            \
		GetCachedDiskObject,            \
		char *,                         \
		__p0,                           \
		a0,                             \
		long,                           \
		__p1,                           \
		d0,                             \
		,                               \
		DOPUS_BASE_NAME,                \
		0,                              \
		0,                              \
		0,                              \
		0,                              \
		0,                              \
		0)

#define CreateRexxMsgEx(__p0, __p1, __p2) \
	LP3(2016,                             \
		struct RexxMsg *,                 \
		CreateRexxMsgEx,                  \
		struct MsgPort *,                 \
		__p0,                             \
		a0,                               \
		UBYTE *,                          \
		__p1,                             \
		a1,                               \
		UBYTE *,                          \
		__p2,                             \
		d0,                               \
		,                                 \
		DOPUS_BASE_NAME,                  \
		0,                                \
		0,                                \
		0,                                \
		0,                                \
		0,                                \
		0)

#define OpenClipBoard(__p0) \
	LP1(1260, struct ClipHandle *, OpenClipBoard, ULONG, __p0, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define ClearMemHandle(__p0) LP1NR(678, ClearMemHandle, void *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define SwapListNodes(__p0, __p1, __p2) \
	LP3NR(1542,                         \
		  SwapListNodes,                \
		  struct List *,                \
		  __p0,                         \
		  a0,                           \
		  struct Node *,                \
		  __p1,                         \
		  a1,                           \
		  struct Node *,                \
		  __p2,                         \
		  a2,                           \
		  ,                             \
		  DOPUS_BASE_NAME,              \
		  0,                            \
		  0,                            \
		  0,                            \
		  0,                            \
		  0,                            \
		  0)

#define WB_Install_Patch() LP0NR(1062, WB_Install_Patch, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CalcWindowDims(__p0, __p1, __p2, __p3, __p4) \
	LP5(282,                                         \
		int,                                         \
		CalcWindowDims,                              \
		struct Screen *,                             \
		__p0,                                        \
		a0,                                          \
		ConfigWindow *,                              \
		__p1,                                        \
		a1,                                          \
		struct IBox *,                               \
		__p2,                                        \
		a2,                                          \
		struct TextFont *,                           \
		__p3,                                        \
		a3,                                          \
		ULONG,                                       \
		__p4,                                        \
		d0,                                          \
		,                                            \
		DOPUS_BASE_NAME,                             \
		0,                                           \
		0,                                           \
		0,                                           \
		0,                                           \
		0,                                           \
		0)

#define Att_PosNode(__p0, __p1, __p2) \
	LP3NR(456,                        \
		  Att_PosNode,                \
		  Att_List *,                 \
		  __p0,                       \
		  a0,                         \
		  Att_Node *,                 \
		  __p1,                       \
		  a1,                         \
		  Att_Node *,                 \
		  __p2,                       \
		  a2,                         \
		  ,                           \
		  DOPUS_BASE_NAME,            \
		  0,                          \
		  0,                          \
		  0,                          \
		  0,                          \
		  0,                          \
		  0)

#define Att_ChangeNodeName(__p0, __p1) \
	LP2NR(504, Att_ChangeNodeName, Att_Node *, __p0, a0, char *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define DisposeBitMap(__p0) LP1NR(1842, DisposeBitMap, struct BitMap *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define AddAllocBitmapPatch(__p0, __p1) \
	LP2(2286,                           \
		APTR,                           \
		AddAllocBitmapPatch,            \
		struct Task *,                  \
		__p0,                           \
		a0,                             \
		struct Screen *,                \
		__p1,                           \
		a1,                             \
		,                               \
		DOPUS_BASE_NAME,                \
		0,                              \
		0,                              \
		0,                              \
		0,                              \
		0,                              \
		0)

#define CopyDiskObject(__p0, __p1) \
	LP2(1578,                      \
		struct DiskObject *,       \
		CopyDiskObject,            \
		struct DiskObject *,       \
		__p0,                      \
		a0,                        \
		ULONG,                     \
		__p1,                      \
		d0,                        \
		,                          \
		DOPUS_BASE_NAME,           \
		0,                         \
		0,                         \
		0,                         \
		0,                         \
		0,                         \
		0)

#define FindMenuItem(__p0, __p1) \
	LP2(528,                     \
		struct MenuItem *,       \
		FindMenuItem,            \
		struct Menu *,           \
		__p0,                    \
		a0,                      \
		UWORD,                   \
		__p1,                    \
		d0,                      \
		,                        \
		DOPUS_BASE_NAME,         \
		0,                       \
		0,                       \
		0,                       \
		0,                       \
		0,                       \
		0)

#define SetIconFlags(__p0, __p1) \
	LP2NR(1506, SetIconFlags, struct DiskObject *, __p0, a0, ULONG, __p1, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define WB_LaunchNotify(__p0, __p1, __p2, __p3, __p4, __p5, __p6, __p7) \
	LP8(2226,                                                           \
		BOOL,                                                           \
		WB_LaunchNotify,                                                \
		char *,                                                         \
		__p0,                                                           \
		a0,                                                             \
		struct Screen *,                                                \
		__p1,                                                           \
		a1,                                                             \
		short,                                                          \
		__p2,                                                           \
		d0,                                                             \
		long,                                                           \
		__p3,                                                           \
		d1,                                                             \
		char *,                                                         \
		__p4,                                                           \
		a2,                                                             \
		struct Process **,                                              \
		__p5,                                                           \
		a3,                                                             \
		IPCData *,                                                      \
		__p6,                                                           \
		a4,                                                             \
		ULONG,                                                          \
		__p7,                                                           \
		d2,                                                             \
		,                                                               \
		DOPUS_BASE_NAME,                                                \
		0,                                                              \
		0,                                                              \
		0,                                                              \
		0,                                                              \
		0,                                                              \
		0)

#define SavePos(__p0, __p1, __p2) \
	LP3(2304,                     \
		BOOL,                     \
		SavePos,                  \
		char *,                   \
		__p0,                     \
		a0,                       \
		struct IBox *,            \
		__p1,                     \
		a1,                       \
		short,                    \
		__p2,                     \
		d0,                       \
		,                         \
		DOPUS_BASE_NAME,          \
		0,                        \
		0,                        \
		0,                        \
		0,                        \
		0,                        \
		0)

#define ReadSettings(__p0, __p1) \
	LP2(738, short, ReadSettings, CFG_SETS *, __p0, a0, char *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define ReplyAppMessage(__p0) \
	LP1NR(1794, ReplyAppMessage, DOpusAppMessage *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define Att_NodeNumber(__p0, __p1) \
	LP2(474, long, Att_NodeNumber, Att_List *, __p0, a0, char *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CopyFunction(__p0, __p1, __p2) \
	LP3(852,                           \
		Cfg_Function *,                \
		CopyFunction,                  \
		Cfg_Function *,                \
		__p0,                          \
		a0,                            \
		APTR,                          \
		__p1,                          \
		a1,                            \
		Cfg_Function *,                \
		__p2,                          \
		a2,                            \
		,                              \
		DOPUS_BASE_NAME,               \
		0,                             \
		0,                             \
		0,                             \
		0,                             \
		0,                             \
		0)

#define OriginalDeleteFile(__p0) \
	LP1(1908, long, OriginalDeleteFile, char *, __p0, d1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define AddSorted(__p0, __p1) \
	LP2NR(516, AddSorted, struct List *, __p0, a0, struct Node *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define ItoaU(__p0, __p1, __p2) \
	LP3NR(66, ItoaU, ULONG, __p0, d0, char *, __p1, a0, char, __p2, d1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define ItoaU64(___num, ___str, ___str_size, ___sep) \
	LP4NR(0x90c,                                     \
		  ItoaU64,                                   \
		  UQUAD *,                                   \
		  ___num,                                    \
		  a0,                                        \
		  char *,                                    \
		  ___str,                                    \
		  a1,                                        \
		  int,                                       \
		  ___str_size,                               \
		  d0,                                        \
		  char,                                      \
		  ___sep,                                    \
		  d1,                                        \
		  ,                                          \
		  DOPUS_BASE_NAME,                           \
		  IF_CACHEFLUSHALL,                          \
		  NULL,                                      \
		  0,                                         \
		  IF_CACHEFLUSHALL,                          \
		  NULL,                                      \
		  0)

#define CheckWindowDims(__p0, __p1) \
	LP2(1434,                       \
		BOOL,                       \
		CheckWindowDims,            \
		struct Window *,            \
		__p0,                       \
		a0,                         \
		WindowDimensions *,         \
		__p1,                       \
		a1,                         \
		,                           \
		DOPUS_BASE_NAME,            \
		0,                          \
		0,                          \
		0,                          \
		0,                          \
		0,                          \
		0)

#define IPC_ListQuit(__p0, __p1, __p2, __p3) \
	LP4(600,                                 \
		ULONG,                               \
		IPC_ListQuit,                        \
		struct ListLock *,                   \
		__p0,                                \
		a0,                                  \
		IPCData *,                           \
		__p1,                                \
		a1,                                  \
		ULONG,                               \
		__p2,                                \
		d0,                                  \
		BOOL,                                \
		__p3,                                \
		d1,                                  \
		,                                    \
		DOPUS_BASE_NAME,                     \
		0,                                   \
		0,                                   \
		0,                                   \
		0,                                   \
		0,                                   \
		0)

#define NewButtonFunction(__p0, __p1) \
	LP2(1194,                         \
		Cfg_ButtonFunction *,         \
		NewButtonFunction,            \
		APTR,                         \
		__p0,                         \
		a0,                           \
		UWORD,                        \
		__p1,                         \
		d0,                           \
		,                             \
		DOPUS_BASE_NAME,              \
		0,                            \
		0,                            \
		0,                            \
		0,                            \
		0,                            \
		0)

#define SetAppIconMenuState(__p0, __p1, __p2) \
	LP3(1710,                                 \
		long,                                 \
		SetAppIconMenuState,                  \
		APTR,                                 \
		__p0,                                 \
		a0,                                   \
		long,                                 \
		__p1,                                 \
		d0,                                   \
		long,                                 \
		__p2,                                 \
		d1,                                   \
		,                                     \
		DOPUS_BASE_NAME,                      \
		0,                                    \
		0,                                    \
		0,                                    \
		0,                                    \
		0,                                    \
		0)

#define FHFromBuf(__p0) LP1(1458, BPTR, FHFromBuf, APTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define DecodeILBM(__p0, __p1, __p2, __p3, __p4, __p5, __p6) \
	LP7NR(960,                                               \
		  DecodeILBM,                                        \
		  char *,                                            \
		  __p0,                                              \
		  a0,                                                \
		  UWORD,                                             \
		  __p1,                                              \
		  d0,                                                \
		  UWORD,                                             \
		  __p2,                                              \
		  d1,                                                \
		  UWORD,                                             \
		  __p3,                                              \
		  d2,                                                \
		  struct BitMap *,                                   \
		  __p4,                                              \
		  a1,                                                \
		  ULONG,                                             \
		  __p5,                                              \
		  d3,                                                \
		  char,                                              \
		  __p6,                                              \
		  d4,                                                \
		  ,                                                  \
		  DOPUS_BASE_NAME,                                   \
		  0,                                                 \
		  0,                                                 \
		  0,                                                 \
		  0,                                                 \
		  0,                                                 \
		  0)

#define ShowProgressWindow(__p0, __p1, __p2) \
	LP3NR(1644,                              \
		  ShowProgressWindow,                \
		  APTR,                              \
		  __p0,                              \
		  a0,                                \
		  struct Screen *,                   \
		  __p1,                              \
		  a1,                                \
		  struct Window *,                   \
		  __p2,                              \
		  a2,                                \
		  ,                                  \
		  DOPUS_BASE_NAME,                   \
		  0,                                 \
		  0,                                 \
		  0,                                 \
		  0,                                 \
		  0,                                 \
		  0)

#define DoPopUpMenu(__p0, __p1, __p2, __p3) \
	LP4(534,                                \
		UWORD,                              \
		DoPopUpMenu,                        \
		struct Window *,                    \
		__p0,                               \
		a0,                                 \
		PopUpMenu *,                        \
		__p1,                               \
		a1,                                 \
		PopUpItem **,                       \
		__p2,                               \
		a2,                                 \
		UWORD,                              \
		__p3,                               \
		d0,                                 \
		,                                   \
		DOPUS_BASE_NAME,                    \
		0,                                  \
		0,                                  \
		0,                                  \
		0,                                  \
		0,                                  \
		0)

#define UpdatePathList() LP0NR(2154, UpdatePathList, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define ReplyFreeMsg(__p0) LP1NR(1182, ReplyFreeMsg, APTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define PopUpSetFlags(__p0, __p1, __p2, __p3) \
	LP4(2280,                                 \
		ULONG,                                \
		PopUpSetFlags,                        \
		PopUpMenu *,                          \
		__p0,                                 \
		a0,                                   \
		UWORD,                                \
		__p1,                                 \
		d0,                                   \
		ULONG,                                \
		__p2,                                 \
		d1,                                   \
		ULONG,                                \
		__p3,                                 \
		d2,                                   \
		,                                     \
		DOPUS_BASE_NAME,                      \
		0,                                    \
		0,                                    \
		0,                                    \
		0,                                    \
		0,                                    \
		0)

#define DevNameFromLockDopus(__p0, __p1, __p2) \
	LP3(1494,                                  \
		BOOL,                                  \
		DevNameFromLockDopus,                  \
		BPTR,                                  \
		__p0,                                  \
		d1,                                    \
		char *,                                \
		__p1,                                  \
		d2,                                    \
		long,                                  \
		__p2,                                  \
		d3,                                    \
		,                                      \
		DOPUS_BASE_NAME,                       \
		0,                                     \
		0,                                     \
		0,                                     \
		0,                                     \
		0,                                     \
		0)

#define SaveSettings(__p0, __p1) \
	LP2(774, int, SaveSettings, CFG_SETS *, __p0, a0, char *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define UpdateGadgetList(__p0) \
	LP1NR(1830, UpdateGadgetList, ObjectList *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define LockReqBackFill(__p0) \
	LP1(2130, struct Hook *, LockReqBackFill, struct Screen *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define SerialCheck(__p0, __p1) \
	LP2(1170, BOOL, SerialCheck, char *, __p0, a0, ULONG *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define BytesToString(__p0, __p1, __p2, __p3) \
	LP4NR(78,                                 \
		  BytesToString,                      \
		  ULONG,                              \
		  __p0,                               \
		  d0,                                 \
		  char *,                             \
		  __p1,                               \
		  a0,                                 \
		  short,                              \
		  __p2,                               \
		  d1,                                 \
		  char,                               \
		  __p3,                               \
		  d2,                                 \
		  ,                                   \
		  DOPUS_BASE_NAME,                    \
		  0,                                  \
		  0,                                  \
		  0,                                  \
		  0,                                  \
		  0,                                  \
		  0)

#define BytesToString64(___bytes, ___string, ___str_size, ___places, ___sep) \
	LP5NR(0x918,                                                             \
		  BytesToString64,                                                   \
		  UQUAD *,                                                           \
		  ___bytes,                                                          \
		  a0,                                                                \
		  char *,                                                            \
		  ___string,                                                         \
		  a1,                                                                \
		  int,                                                               \
		  ___str_size,                                                       \
		  d0,                                                                \
		  int,                                                               \
		  ___places,                                                         \
		  d1,                                                                \
		  char,                                                              \
		  ___sep,                                                            \
		  d2,                                                                \
		  ,                                                                  \
		  DOPUS_BASE_NAME,                                                   \
		  IF_CACHEFLUSHALL,                                                  \
		  NULL,                                                              \
		  0,                                                                 \
		  IF_CACHEFLUSHALL,                                                  \
		  NULL,                                                              \
		  0)

#define Att_NodeName(__p0, __p1) \
	LP2(492, char *, Att_NodeName, Att_List *, __p0, a0, long, __p1, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CloseButtonBank(__p0) \
	LP1NR(792, CloseButtonBank, Cfg_ButtonBank *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define OpenIFFFile(__p0, __p1, __p2) \
	LP3(918,                          \
		struct IFFHandle *,           \
		OpenIFFFile,                  \
		char *,                       \
		__p0,                         \
		a0,                           \
		int,                          \
		__p1,                         \
		d0,                           \
		ULONG,                        \
		__p2,                         \
		d1,                           \
		,                             \
		DOPUS_BASE_NAME,              \
		0,                            \
		0,                            \
		0,                            \
		0,                            \
		0,                            \
		0)

#define Atoh(__p0, __p1) LP2(42, ULONG, Atoh, char *, __p0, a0, short, __p1, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetFileVersion(__p0, __p1, __p2, __p3, __p4) \
	LP5(1758,                                        \
		BOOL,                                        \
		GetFileVersion,                              \
		char *,                                      \
		__p0,                                        \
		a0,                                          \
		short *,                                     \
		__p1,                                        \
		d0,                                          \
		short *,                                     \
		__p2,                                        \
		d1,                                          \
		struct DateStamp *,                          \
		__p3,                                        \
		a1,                                          \
		APTR,                                        \
		__p4,                                        \
		a2,                                          \
		,                                            \
		DOPUS_BASE_NAME,                             \
		0,                                           \
		0,                                           \
		0,                                           \
		0,                                           \
		0,                                           \
		0)

#define BuildRexxMsgEx(__p0, __p1, __p2, __p3) \
	LP4(2034,                                  \
		struct RexxMsg *,                      \
		BuildRexxMsgEx,                        \
		struct MsgPort *,                      \
		__p0,                                  \
		a0,                                    \
		UBYTE *,                               \
		__p1,                                  \
		a1,                                    \
		UBYTE *,                               \
		__p2,                                  \
		d0,                                    \
		struct TagItem *,                      \
		__p3,                                  \
		a2,                                    \
		,                                      \
		DOPUS_BASE_NAME,                       \
		0,                                     \
		0,                                     \
		0,                                     \
		0,                                     \
		0,                                     \
		0)

#define LockAttList(__p0, __p1) \
	LP2NR(1284, LockAttList, Att_List *, __p0, a0, short, __p1, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define CloseImage(__p0) LP1NR(636, CloseImage, APTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define DrawBox(__p0, __p1, __p2, __p3) \
	LP4NR(696,                          \
		  DrawBox,                      \
		  struct RastPort *,            \
		  __p0,                         \
		  a0,                           \
		  struct Rectangle *,           \
		  __p1,                         \
		  a1,                           \
		  struct DrawInfo *,            \
		  __p2,                         \
		  a2,                           \
		  BOOL,                         \
		  __p3,                         \
		  d0,                           \
		  ,                             \
		  DOPUS_BASE_NAME,              \
		  0,                            \
		  0,                            \
		  0,                            \
		  0,                            \
		  0,                            \
		  0)

#define GetImagePalette(__p0) LP1(1530, ULONG *, GetImagePalette, APTR, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FreeDiskObjectCopy(__p0) \
	LP1NR(1584, FreeDiskObjectCopy, struct DiskObject *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define DefaultButtonBank() LP0(768, Cfg_ButtonBank *, DefaultButtonBank, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define OpenDisk(__p0, __p1) \
	LP2(1026, DiskHandle *, OpenDisk, char *, __p0, a0, struct MsgPort *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FreeInstruction(__p0) \
	LP1NR(828, FreeInstruction, Cfg_Instruction *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define AsyncRequest(__p0, __p1, __p2, __p3, __p4, __p5) \
	LP6(1764,                                            \
		long,                                            \
		AsyncRequest,                                    \
		IPCData *,                                       \
		__p0,                                            \
		a0,                                              \
		long,                                            \
		__p1,                                            \
		d0,                                              \
		struct Window *,                                 \
		__p2,                                            \
		a1,                                              \
		REF_CALLBACK,                                    \
		__p3,                                            \
		a2,                                              \
		APTR,                                            \
		__p4,                                            \
		a3,                                              \
		struct TagItem *,                                \
		__p5,                                            \
		d1,                                              \
		,                                                \
		DOPUS_BASE_NAME,                                 \
		0,                                               \
		0,                                               \
		0,                                               \
		0,                                               \
		0,                                               \
		0)

#define TimerActive(__p0) LP1(1188, BOOL, TimerActive, TimerHandle *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define SetGadgetChoices(__p0, __p1, __p2) \
	LP3NR(342,                             \
		  SetGadgetChoices,                \
		  ObjectList *,                    \
		  __p0,                            \
		  a0,                              \
		  ULONG,                           \
		  __p1,                            \
		  d0,                              \
		  APTR,                            \
		  __p2,                            \
		  a1,                              \
		  ,                                \
		  DOPUS_BASE_NAME,                 \
		  0,                               \
		  0,                               \
		  0,                               \
		  0,                               \
		  0,                               \
		  0)

#define AllocMemH(__p0, __p1) \
	LP2(684, void *, AllocMemH, void *, __p0, a0, ULONG, __p1, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define FreeCachedDiskObject(__p0) \
	LP1NR(1602, FreeCachedDiskObject, struct DiskObject *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define ShowDragImage(__p0, __p1, __p2) \
	LP3NR(198,                          \
		  ShowDragImage,                \
		  DragInfo *,                   \
		  __p0,                         \
		  a0,                           \
		  ULONG,                        \
		  __p1,                         \
		  d0,                           \
		  ULONG,                        \
		  __p2,                         \
		  d1,                           \
		  ,                             \
		  DOPUS_BASE_NAME,              \
		  0,                            \
		  0,                            \
		  0,                            \
		  0,                            \
		  0,                            \
		  0)

#define DrawFieldBox(__p0, __p1, __p2) \
	LP3NR(702,                         \
		  DrawFieldBox,                \
		  struct RastPort *,           \
		  __p0,                        \
		  a0,                          \
		  struct Rectangle *,          \
		  __p1,                        \
		  a1,                          \
		  struct DrawInfo *,           \
		  __p2,                        \
		  a2,                          \
		  ,                            \
		  DOPUS_BASE_NAME,             \
		  0,                           \
		  0,                           \
		  0,                           \
		  0,                           \
		  0,                           \
		  0)

#define GetWindowID(__p0) LP1(426, ULONG, GetWindowID, struct Window *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define Itoa(__p0, __p1, __p2) \
	LP3NR(60, Itoa, long, __p0, d0, char *, __p1, a0, char, __p2, d1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define IFFPushChunk(__p0, __p1) \
	LP2(1212, long, IFFPushChunk, APTR, __p0, a0, ULONG, __p1, d0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define SetProgressWindow(__p0, __p1) \
	LP2NR(1650, SetProgressWindow, APTR, __p0, a0, struct TagItem *, __p1, a1, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define DoSimpleRequest(__p0, __p1)  \
	LP2(150,                         \
		short,                       \
		DoSimpleRequest,             \
		struct Window *,             \
		__p0,                        \
		a0,                          \
		struct DOpusSimpleRequest *, \
		__p1,                        \
		a1,                          \
		,                            \
		DOPUS_BASE_NAME,             \
		0,                           \
		0,                           \
		0,                           \
		0,                           \
		0,                           \
		0)

#define FreeILBM(__p0) LP1NR(954, FreeILBM, ILBMHandle *, __p0, a0, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define OriginalSetProtection(__p0, __p1) \
	LP2(1926, BOOL, OriginalSetProtection, char *, __p0, d1, ULONG, __p1, d2, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define GetTimerBase() LP0(1998, struct Library *, GetTimerBase, , DOPUS_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define WB_AddAppWindow(__p0, __p1, __p2, __p3, __p4) \
	LP5(1074,                                         \
		struct AppWindow *,                           \
		WB_AddAppWindow,                              \
		IPTR,                                         \
		__p0,                                         \
		d0,                                           \
		IPTR,                                         \
		__p1,                                         \
		d1,                                           \
		struct Window *,                              \
		__p2,                                         \
		a0,                                           \
		struct MsgPort *,                             \
		__p3,                                         \
		a1,                                           \
		struct TagItem *,                             \
		__p4,                                         \
		a2,                                           \
		,                                             \
		DOPUS_BASE_NAME,                              \
		0,                                            \
		0,                                            \
		0,                                            \
		0,                                            \
		0,                                            \
		0)

#define ExamineHandle64(___fh, ___fib) \
	LP2(0x92a,                         \
		BOOL,                          \
		ExamineHandle64,               \
		BPTR,                          \
		___fh,                         \
		d0,                            \
		FileInfoBlock64 *,             \
		___fib,                        \
		a0,                            \
		,                              \
		DOPUS_BASE_NAME,               \
		IF_CACHEFLUSHALL,              \
		NULL,                          \
		0,                             \
		IF_CACHEFLUSHALL,              \
		NULL,                          \
		0)

#define ExamineLock64(___lock, ___fib) \
	LP2(0x91e,                         \
		BOOL,                          \
		ExamineLock64,                 \
		BPTR,                          \
		___lock,                       \
		d0,                            \
		FileInfoBlock64 *,             \
		___fib,                        \
		a0,                            \
		,                              \
		DOPUS_BASE_NAME,               \
		IF_CACHEFLUSHALL,              \
		NULL,                          \
		0,                             \
		IF_CACHEFLUSHALL,              \
		NULL,                          \
		0)

#define ExamineNext64(___lock, ___fib) \
	LP2(0x924,                         \
		BOOL,                          \
		ExamineNext64,                 \
		BPTR,                          \
		___lock,                       \
		d0,                            \
		FileInfoBlock64 *,             \
		___fib,                        \
		a0,                            \
		,                              \
		DOPUS_BASE_NAME,               \
		IF_CACHEFLUSHALL,              \
		NULL,                          \
		0,                             \
		IF_CACHEFLUSHALL,              \
		NULL,                          \
		0)

#define MatchFirst64(___pat, ___panchor) \
	LP2(0x930,                           \
		LONG,                            \
		MatchFirst64,                    \
		STRPTR,                          \
		___pat,                          \
		a0,                              \
		struct AnchorPath *,             \
		___panchor,                      \
		a1,                              \
		,                                \
		DOPUS_BASE_NAME,                 \
		IF_CACHEFLUSHALL,                \
		NULL,                            \
		0,                               \
		IF_CACHEFLUSHALL,                \
		NULL,                            \
		0)

#define MatchNext64(___panchor) \
	LP1(0x936,                  \
		LONG,                   \
		MatchNext64,            \
		struct AnchorPath *,    \
		___panchor,             \
		a0,                     \
		,                       \
		DOPUS_BASE_NAME,        \
		IF_CACHEFLUSHALL,       \
		NULL,                   \
		0,                      \
		IF_CACHEFLUSHALL,       \
		NULL,                   \
		0)

#define SimpleRequestTags(window, title, buttons, message, ...)                        \
	({                                                                                 \
		IPTR __args[] = {__VA_ARGS__};                                                 \
		(short)SimpleRequest(window, title, buttons, message, 0, (APTR)&__args, 0, 0); \
	})

#define OpenProgressWindowTags(...)                           \
	({                                                        \
		IPTR __tags[] = {__VA_ARGS__};                        \
		(APTR) OpenProgressWindow((struct TagItem *)&__tags); \
	})

#define SetProgressWindowTags(prog, ...)                          \
	({                                                            \
		IPTR __tags[] = {__VA_ARGS__};                            \
		(void)SetProgressWindow(prog, (struct TagItem *)&__tags); \
	})

#define AsyncRequestTags(my_ipc, type, window, callback, data, ...)                          \
	({                                                                                       \
		IPTR __tags[] = {__VA_ARGS__};                                                       \
		(long)AsyncRequest(my_ipc, type, window, callback, data, (struct TagItem *)&__tags); \
	})

#define GetEditHookTags(type, flags, ...)                                   \
	({                                                                      \
		IPTR __tags[] = {__VA_ARGS__};                                      \
		(struct Hook *)GetEditHook(type, flags, (struct TagItem *)&__tags); \
	})

#define BuildRexxMsgExTags(port, ext, host, ...)                                      \
	({                                                                                \
		IPTR __tags[] = {__VA_ARGS__};                                                \
		(struct RexxMsg *)BuildRexxMsgEx(port, ext, host, (struct TagItem *)&__tags); \
	})

#define GetProgressWindowTags(prog, ...)                          \
	({                                                            \
		IPTR __tags[] = {__VA_ARGS__};                            \
		(void)GetProgressWindow(prog, (struct TagItem *)&__tags); \
	})

#endif /* !_PPCINLINE_DOPUS_H */
