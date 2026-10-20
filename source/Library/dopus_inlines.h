/* Automatically generated header! Do not edit! */

#ifndef _INLINE_DOPUS_H
#define _INLINE_DOPUS_H

#ifndef __INLINE_MACROS_H
	#include <inline/macros.h>
#endif /* !__INLINE_MACROS_H */

#ifndef DOPUS_BASE_NAME
	#define DOPUS_BASE_NAME DOpusBase
#endif /* !DOPUS_BASE_NAME */

#define AllocMemH(handle, size) LP2(0x2ac, void *, AllocMemH, void *, handle, a0, ULONG, size, d0, , DOPUS_BASE_NAME)

#define AllocTimer(unit, port) \
	LP2(0x6c, TimerHandle *, AllocTimer, ULONG, unit, d0, struct MsgPort *, port, a0, , DOPUS_BASE_NAME)

#define Att_NewList(flags) LP1(0x1b6, Att_List *, Att_NewList, ULONG, flags, d0, , DOPUS_BASE_NAME)

#define Att_NewNode(list, name, data, flags) \
	LP4(0x1bc,                               \
		Att_Node *,                          \
		Att_NewNode,                         \
		Att_List *,                          \
		list,                                \
		a0,                                  \
		char *,                              \
		name,                                \
		a1,                                  \
		IPTR,                                \
		data,                                \
		d0,                                  \
		ULONG,                               \
		flags,                               \
		d1,                                  \
		,                                    \
		DOPUS_BASE_NAME)

#define Att_RemList(list, flags) LP2NR(0x1ce, Att_RemList, Att_List *, list, a0, long, flags, d0, , DOPUS_BASE_NAME)

#define BuildKeyString(code, qual, mask, same, buffer) \
	LP5NR(0x60,                                        \
		  BuildKeyString,                              \
		  unsigned short,                              \
		  code,                                        \
		  d0,                                          \
		  unsigned short,                              \
		  qual,                                        \
		  d1,                                          \
		  unsigned short,                              \
		  mask,                                        \
		  d2,                                          \
		  unsigned short,                              \
		  same,                                        \
		  d3,                                          \
		  char *,                                      \
		  buffer,                                      \
		  a0,                                          \
		  ,                                            \
		  DOPUS_BASE_NAME)

#define CheckTimer(handle) LP1(0x7e, BOOL, CheckTimer, TimerHandle *, handle, a0, , DOPUS_BASE_NAME)

#define CloseClipBoard(clip) LP1NR(0x4f2, CloseClipBoard, struct ClipHandle *, clip, a0, , DOPUS_BASE_NAME)

#define CopyLocalEnv(base) LP1NR(0x486, CopyLocalEnv, struct Library *, base, a0, , DOPUS_BASE_NAME)

#define DeviceFromHandler(port, name) \
	LP2(0x5d0, struct DosList *, DeviceFromHandler, struct MsgPort *, port, a0, char *, name, a1, , DOPUS_BASE_NAME)

#define DoSimpleRequest(window, simplereq) \
	LP2(0x96,                              \
		short,                             \
		DoSimpleRequest,                   \
		struct Window *,                   \
		window,                            \
		a0,                                \
		struct DOpusSimpleRequest *,       \
		simplereq,                         \
		a1,                                \
		,                                  \
		DOPUS_BASE_NAME)

#define DrawBox(rp, rect, info, recess) \
	LP4NR(0x2b8,                        \
		  DrawBox,                      \
		  struct RastPort *,            \
		  rp,                           \
		  a0,                           \
		  struct Rectangle *,           \
		  rect,                         \
		  a1,                           \
		  struct DrawInfo *,            \
		  info,                         \
		  a2,                           \
		  BOOL,                         \
		  recess,                       \
		  d0,                           \
		  ,                             \
		  DOPUS_BASE_NAME)

#define FindPubScreen(scr, lock) \
	LP2(0x6a8, struct PubScreenNode *, FindPubScreen, struct Screen *, scr, a0, BOOL, lock, d0, , DOPUS_BASE_NAME)

#define FreeMemH(memory) LP1NR(0x2b2, FreeMemH, void *, memory, a0, , DOPUS_BASE_NAME)

#define FreeSemaphore(sem) LP1NR(0x540, FreeSemaphore, struct SignalSemaphore *, sem, a0, , DOPUS_BASE_NAME)

#define FreeTimer(handle) LP1NR(0x72, FreeTimer, TimerHandle *, handle, a0, , DOPUS_BASE_NAME)

#define GetDeviceUnit(startup, device, unit) \
	LP3(0x87c, BOOL, GetDeviceUnit, BPTR, startup, a0, char *, device, a1, short *, unit, a2, , DOPUS_BASE_NAME)

#define GetDosPathList(copy) LP1(0x8a, BPTR, GetDosPathList, BPTR, copy, a0, , DOPUS_BASE_NAME)

#define GetSemaphore(sem, exc, data) \
	LP3(0x53a,                       \
		long,                        \
		GetSemaphore,                \
		struct SignalSemaphore *,    \
		sem,                         \
		a0,                          \
		long,                        \
		exc,                         \
		d0,                          \
		char *,                      \
		data,                        \
		a1,                          \
		,                            \
		DOPUS_BASE_NAME)

#define GetString(locale, num) \
	LP2(0x192, STRPTR, GetString, struct DOpusLocale *, locale, a0, LONG, num, d0, , DOPUS_BASE_NAME)

#define IPC_Command(ipc, command, flags, data, data_free, reply) \
	LP6a(0x228,                                                  \
		 IPTR,                                                   \
		 IPC_Command,                                            \
		 IPCData *,                                              \
		 ipc,                                                    \
		 a0,                                                     \
		 ULONG,                                                  \
		 command,                                                \
		 d0,                                                     \
		 IPTR,                                                   \
		 flags,                                                  \
		 d1,                                                     \
		 APTR,                                                   \
		 data,                                                   \
		 a1,                                                     \
		 APTR,                                                   \
		 data_free,                                              \
		 a2,                                                     \
		 struct MsgPort *,                                       \
		 reply,                                                  \
		 a3,                                                     \
		 ,                                                       \
		 DOPUS_BASE_NAME)

#define IPC_Free(ipc) LP1NR(0x234, IPC_Free, IPCData *, ipc, a0, , DOPUS_BASE_NAME)

#define IPC_Reply(msg) LP1NR(0x22e, IPC_Reply, IPCMessage *, msg, a0, , DOPUS_BASE_NAME)

#define OpenClipBoard(unit) LP1(0x4ec, struct ClipHandle *, OpenClipBoard, ULONG, unit, d0, , DOPUS_BASE_NAME)

#define ReadClipString(clip, string, len) \
	LP3(0x4fe,                            \
		long,                             \
		ReadClipString,                   \
		struct ClipHandle *,              \
		clip,                             \
		a0,                               \
		char *,                           \
		string,                           \
		a1,                               \
		long,                             \
		len,                              \
		d0,                               \
		,                                 \
		DOPUS_BASE_NAME)

#define SendNotifyMsg(type, data, flags, wait, name, fib) \
	LP6NR(0x46e,                                          \
		  SendNotifyMsg,                                  \
		  ULONG,                                          \
		  type,                                           \
		  d0,                                             \
		  IPTR,                                           \
		  data,                                           \
		  d1,                                             \
		  ULONG,                                          \
		  flags,                                          \
		  d2,                                             \
		  short,                                          \
		  wait,                                           \
		  d3,                                             \
		  char *,                                         \
		  name,                                           \
		  a0,                                             \
		  struct FileInfoBlock *,                         \
		  fib,                                            \
		  a1,                                             \
		  ,                                               \
		  DOPUS_BASE_NAME)

#define SetWindowID(window, id_ptr, id, port) \
	LP4NR(0x1a4,                              \
		  SetWindowID,                        \
		  struct Window *,                    \
		  window,                             \
		  a0,                                 \
		  WindowID *,                         \
		  id_ptr,                             \
		  a1,                                 \
		  ULONG,                              \
		  id,                                 \
		  d0,                                 \
		  struct MsgPort *,                   \
		  port,                               \
		  a2,                                 \
		  ,                                   \
		  DOPUS_BASE_NAME)

#define StartTimer(handle, secs, micro) \
	LP3NR(0x78, StartTimer, TimerHandle *, handle, a0, ULONG, secs, d0, ULONG, micro, d1, , DOPUS_BASE_NAME)

#define UpdateMyPaths() LP0NR(0x870, UpdateMyPaths, , DOPUS_BASE_NAME)

#define WB_Install_Patch() LP0NR(0x426, WB_Install_Patch, , DOPUS_BASE_NAME)

#define WriteClipString(clip, string, len) \
	LP3(0x4f8,                             \
		BOOL,                              \
		WriteClipString,                   \
		struct ClipHandle *,               \
		clip,                              \
		a0,                                \
		char *,                            \
		string,                            \
		a1,                                \
		long,                              \
		len,                               \
		d0,                                \
		,                                  \
		DOPUS_BASE_NAME)

#endif /* !_INLINE_DOPUS_H */
