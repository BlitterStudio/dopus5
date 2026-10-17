/* Automatically generated header! Do not edit! */

#ifndef _INLINE_DOPUS_H
#define _INLINE_DOPUS_H

#ifndef AROS_LIBCALL_H
	#include <aros/libcall.h>
#endif /* !AROS_LIBCALL_H */

#include <aros/preprocessor/variadic/cast2iptr.hpp>

#ifndef DOPUS_BASE_NAME
	#define DOPUS_BASE_NAME DOpusBase
#endif /* !DOPUS_BASE_NAME */

#define ActivateStrGad(___gadget, ___window)             \
	AROS_LC2NR(void,                                        \
			 ActivateStrGad,                             \
			 AROS_LCA(struct Gadget *, (___gadget), A0), \
			 AROS_LCA(struct Window *, (___window), A1), \
			 struct Library *,                           \
			 DOPUS_BASE_NAME,                            \
			 17,                                         \
			 /* s */)

#define AddAllocBitmapPatch(___task, ___screen)          \
	AROS_LC2(APTR,                                       \
			 AddAllocBitmapPatch,                        \
			 AROS_LCA(struct Task *, (___task), A0),     \
			 AROS_LCA(struct Screen *, (___screen), A1), \
			 struct Library *,                           \
			 DOPUS_BASE_NAME,                            \
			 381,                                        \
			 /* s */)

#define AddDragImage(___drag) \
	AROS_LC1NR(void,  AddDragImage, AROS_LCA(DragInfo *, (___drag), A0), struct Library *, DOPUS_BASE_NAME, 38, /* s */)

#define AddNotifyRequest(___type, ___data, ___port)     \
	AROS_LC3(APTR,                                      \
			 AddNotifyRequest,                          \
			 AROS_LCA(ULONG, (___type), D0),            \
			 AROS_LCA(ULONG, (___data), D1),            \
			 AROS_LCA(struct MsgPort *, (___port), A0), \
			 struct Library *,                          \
			 DOPUS_BASE_NAME,                           \
			 187,                                       \
			 /* s */)

#define AddObjectList(___window, ___objects)             \
	AROS_LC2(ObjectList *,                               \
			 AddObjectList,                              \
			 AROS_LCA(struct Window *, (___window), A0), \
			 AROS_LCA(ObjectDef *, (___objects), A1),    \
			 struct Library *,                           \
			 DOPUS_BASE_NAME,                            \
			 48,                                         \
			 /* s */)

#define AddScrollBars(___win, ___list, ___drawinfo, ___noidcmp) \
	AROS_LC4(struct Gadget *,                                   \
			 AddScrollBars,                                     \
			 AROS_LCA(struct Window *, (___win), A0),           \
			 AROS_LCA(struct List *, (___list), A1),            \
			 AROS_LCA(struct DrawInfo *, (___drawinfo), A2),    \
			 AROS_LCA(short, (___noidcmp), D0),                 \
			 struct Library *,                                  \
			 DOPUS_BASE_NAME,                                   \
			 173,                                               \
			 /* s */)

#define AddSorted(___list, ___node)                  \
	AROS_LC2NR(void,                                    \
			 AddSorted,                              \
			 AROS_LCA(struct List *, (___list), A0), \
			 AROS_LCA(struct Node *, (___node), A1), \
			 struct Library *,                       \
			 DOPUS_BASE_NAME,                        \
			 86,                                     \
			 /* s */)

#define AddWindowMenus(___window, ___data)               \
	AROS_LC2NR(void,                                        \
			 AddWindowMenus,                             \
			 AROS_LCA(struct Window *, (___window), A0), \
			 AROS_LCA(MenuData *, (___data), A1),        \
			 struct Library *,                           \
			 DOPUS_BASE_NAME,                            \
			 63,                                         \
			 /* s */)

#define AllocAppMessage(___mem, ___reply, ___num)        \
	AROS_LC3(DOpusAppMessage *,                          \
			 AllocAppMessage,                            \
			 AROS_LCA(APTR, (___mem), A0),               \
			 AROS_LCA(struct MsgPort *, (___reply), A1), \
			 AROS_LCA(short, (___num), D0),              \
			 struct Library *,                           \
			 DOPUS_BASE_NAME,                            \
			 313,                                        \
			 /* s */)

#define AllocMemH(___handle, ___size)           \
	AROS_LC2(void *,                            \
			 AllocMemH,                         \
			 AROS_LCA(void *, (___handle), A0), \
			 AROS_LCA(ULONG, (___size), D0),    \
			 struct Library *,                  \
			 DOPUS_BASE_NAME,                   \
			 114,                               \
			 /* s */)

#define AllocTimer(___unit, ___port)                    \
	AROS_LC2(struct TimerHandle *,                      \
			 AllocTimer,                                \
			 AROS_LCA(ULONG, (___unit), D0),            \
			 AROS_LCA(struct MsgPort *, (___port), A0), \
			 struct Library *,                          \
			 DOPUS_BASE_NAME,                           \
			 18,                                        \
			 /* s */)

#define AnimDecodeRIFFSet(___delta, ___plane, ___rowbytes, ___sourcebytes) \
	AROS_LC4NR(void,                                                          \
			 AnimDecodeRIFFSet,                                            \
			 AROS_LCA(unsigned char *, (___delta), A0),                    \
			 AROS_LCA(char *, (___plane), A1),                             \
			 AROS_LCA(UWORD, (___rowbytes), D0),                           \
			 AROS_LCA(UWORD, (___sourcebytes), D1),                        \
			 struct Library *,                                             \
			 DOPUS_BASE_NAME,                                              \
			 208,                                                          \
			 /* s */)

#define AnimDecodeRIFFXor(___delta, ___plane, ___rowbytes, ___sourcebytes) \
	AROS_LC4NR(void,                                                          \
			 AnimDecodeRIFFXor,                                            \
			 AROS_LCA(unsigned char *, (___delta), A0),                    \
			 AROS_LCA(char *, (___plane), A1),                             \
			 AROS_LCA(UWORD, (___rowbytes), D0),                           \
			 AROS_LCA(UWORD, (___sourcebytes), D1),                        \
			 struct Library *,                                             \
			 DOPUS_BASE_NAME,                                              \
			 207,                                                          \
			 /* s */)

#define AsyncRequest(___ipc, ___type, ___window, ___callback, ___data, ___tags) \
	AROS_LC6(long,                                                              \
			 AsyncRequest,                                                      \
			 AROS_LCA(IPCData *, (___ipc), A0),                                 \
			 AROS_LCA(long, (___type), D0),                                     \
			 AROS_LCA(struct Window *, (___window), A1),                        \
			 AROS_LCA(REF_CALLBACK, (___callback), A2),                         \
			 AROS_LCA(APTR, (___data), A3),                                     \
			 AROS_LCA(struct TagItem *, (___tags), D1),                         \
			 struct Library *,                                                  \
			 DOPUS_BASE_NAME,                                                   \
			 294,                                                               \
			 /* s */)

#ifndef NO_INLINE_STDARG
	#define AsyncRequestTags(a0, a1, a2, a3, a4, ...)                            \
		({                                                                       \
			IPTR _tags[] = {AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__)};            \
			AsyncRequest((a0), (a1), (a2), (a3), (a4), (struct TagItem *)_tags); \
		})
#endif /* !NO_INLINE_STDARG */

#define Atoh(___str, ___len)                 \
	AROS_LC2(ULONG,                          \
			 Atoh,                           \
			 AROS_LCA(char *, (___str), A0), \
			 AROS_LCA(short, (___len), D0),  \
			 struct Library *,               \
			 DOPUS_BASE_NAME,                \
			 7,                              \
			 /* s */)

#define Att_ChangeNodeName(___node, ___name)      \
	AROS_LC2NR(void,                                 \
			 Att_ChangeNodeName,                  \
			 AROS_LCA(Att_Node *, (___node), A0), \
			 AROS_LCA(char *, (___name), A1),     \
			 struct Library *,                    \
			 DOPUS_BASE_NAME,                     \
			 84,                                  \
			 /* s */)

#define Att_FindNode(___list, ___number)          \
	AROS_LC2(Att_Node *,                          \
			 Att_FindNode,                        \
			 AROS_LCA(Att_List *, (___list), A0), \
			 AROS_LCA(long, (___number), D0),     \
			 struct Library *,                    \
			 DOPUS_BASE_NAME,                     \
			 78,                                  \
			 /* s */)

#define Att_FindNodeData(___list, ___data)        \
	AROS_LC2(Att_Node *,                          \
			 Att_FindNodeData,                    \
			 AROS_LCA(Att_List *, (___list), A0), \
			 AROS_LCA(IPTR, (___data), D0),       \
			 struct Library *,                    \
			 DOPUS_BASE_NAME,                     \
			 80,                                  \
			 /* s */)

#define Att_FindNodeNumber(___list, ___node)      \
	AROS_LC2(long,                                \
			 Att_FindNodeNumber,                  \
			 AROS_LCA(Att_List *, (___list), A0), \
			 AROS_LCA(Att_Node *, (___node), A1), \
			 struct Library *,                    \
			 DOPUS_BASE_NAME,                     \
			 85,                                  \
			 /* s */)

#define Att_NewList(___flags) \
	AROS_LC1(Att_List *, Att_NewList, AROS_LCA(ULONG, (___flags), D0), struct Library *, DOPUS_BASE_NAME, 73, /* s */)

#define Att_NewNode(___list, ___name, ___data, ___flags) \
	AROS_LC4(Att_Node *,                                 \
			 Att_NewNode,                                \
			 AROS_LCA(Att_List *, (___list), A0),        \
			 AROS_LCA(char *, (___name), A1),            \
			 AROS_LCA(IPTR, (___data), D0),              \
			 AROS_LCA(ULONG, (___flags), D1),            \
			 struct Library *,                           \
			 DOPUS_BASE_NAME,                            \
			 74,                                         \
			 /* s */)

#define Att_NodeCount(___list) \
	AROS_LC1(long, Att_NodeCount, AROS_LCA(Att_List *, (___list), A0), struct Library *, DOPUS_BASE_NAME, 83, /* s */)

#define Att_NodeDataNumber(___list, ___data)      \
	AROS_LC2(long,                                \
			 Att_NodeDataNumber,                  \
			 AROS_LCA(Att_List *, (___list), A0), \
			 AROS_LCA(IPTR, (___data), D0),       \
			 struct Library *,                    \
			 DOPUS_BASE_NAME,                     \
			 81,                                  \
			 /* s */)

#define Att_NodeName(___list, ___number)          \
	AROS_LC2(char *,                              \
			 Att_NodeName,                        \
			 AROS_LCA(Att_List *, (___list), A0), \
			 AROS_LCA(long, (___number), D0),     \
			 struct Library *,                    \
			 DOPUS_BASE_NAME,                     \
			 82,                                  \
			 /* s */)

#define Att_NodeNumber(___list, ___name)          \
	AROS_LC2(long,                                \
			 Att_NodeNumber,                      \
			 AROS_LCA(Att_List *, (___list), A0), \
			 AROS_LCA(char *, (___name), A1),     \
			 struct Library *,                    \
			 DOPUS_BASE_NAME,                     \
			 79,                                  \
			 /* s */)

#define Att_PosNode(___list, ___node, ___before)    \
	AROS_LC3NR(void,                                   \
			 Att_PosNode,                           \
			 AROS_LCA(Att_List *, (___list), A0),   \
			 AROS_LCA(Att_Node *, (___node), A1),   \
			 AROS_LCA(Att_Node *, (___before), A2), \
			 struct Library *,                      \
			 DOPUS_BASE_NAME,                       \
			 76,                                    \
			 /* s */)

#define Att_RemList(___list, ___flags)            \
	AROS_LC2NR(void,                                 \
			 Att_RemList,                         \
			 AROS_LCA(Att_List *, (___list), A0), \
			 AROS_LCA(long, (___flags), D0),      \
			 struct Library *,                    \
			 DOPUS_BASE_NAME,                     \
			 77,                                  \
			 /* s */)

#define Att_RemNode(___node) \
	AROS_LC1NR(void,  Att_RemNode, AROS_LCA(Att_Node *, (___node), A0), struct Library *, DOPUS_BASE_NAME, 75, /* s */)

#define BOOPSIFree(___list)                                                                                         \
	AROS_LC1NR(void,  BOOPSIFree, AROS_LCA(struct List *, (___list), A0), struct Library *, DOPUS_BASE_NAME, 175, /* s \
																												*/)

#define BoundsCheckGadget(___list, ___id, ___min, ___max) \
	AROS_LC4(int,                                         \
			 BoundsCheckGadget,                           \
			 AROS_LCA(ObjectList *, (___list), A0),       \
			 AROS_LCA(ULONG, (___id), D0),                \
			 AROS_LCA(int, (___min), D1),                 \
			 AROS_LCA(int, (___max), D2),                 \
			 struct Library *,                            \
			 DOPUS_BASE_NAME,                             \
			 62,                                          \
			 /* s */)

#define BtoCStr(___bstr, ___cstr, ___len)     \
	AROS_LC3NR(void,                             \
			 BtoCStr,                         \
			 AROS_LCA(BSTR, (___bstr), A0),   \
			 AROS_LCA(char *, (___cstr), A1), \
			 AROS_LCA(int, (___len), D0),     \
			 struct Library *,                \
			 DOPUS_BASE_NAME,                 \
			 8,                               \
			 /* s */)

#define BuildKeyString(___code, ___qual, ___mask, ___same, ___buffer) \
	AROS_LC5NR(void,                                                     \
			 BuildKeyString,                                          \
			 AROS_LCA(UWORD, (___code), D0),                          \
			 AROS_LCA(UWORD, (___qual), D1),                          \
			 AROS_LCA(UWORD, (___mask), D2),                          \
			 AROS_LCA(UWORD, (___same), D3),                          \
			 AROS_LCA(char *, (___buffer), A0),                       \
			 struct Library *,                                        \
			 DOPUS_BASE_NAME,                                         \
			 16,                                                      \
			 /* s */)

#define BuildMenuStrip(___data, ___locale)                    \
	AROS_LC2(struct Menu *,                                   \
			 BuildMenuStrip,                                  \
			 AROS_LCA(MenuData *, (___data), A0),             \
			 AROS_LCA(struct DOpusLocale *, (___locale), A1), \
			 struct Library *,                                \
			 DOPUS_BASE_NAME,                                 \
			 87,                                              \
			 /* s */)

#define BuildRexxMsgEx(___port, ___extension, ___host, ___tags) \
	AROS_LC4(struct RexxMsg *,                                  \
			 BuildRexxMsgEx,                                    \
			 AROS_LCA(struct MsgPort *, (___port), A0),         \
			 AROS_LCA(UBYTE *, (___extension), A1),             \
			 AROS_LCA(UBYTE *, (___host), D0),                  \
			 AROS_LCA(struct TagItem *, (___tags), A2),         \
			 struct Library *,                                  \
			 DOPUS_BASE_NAME,                                   \
			 339,                                               \
			 /* s */)

#ifndef NO_INLINE_STDARG
	#define BuildRexxMsgExTags(a0, a1, a2, ...)                        \
		({                                                             \
			IPTR _tags[] = {AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__)};  \
			BuildRexxMsgEx((a0), (a1), (a2), (struct TagItem *)_tags); \
		})
#endif /* !NO_INLINE_STDARG */

#define BuildTransDragMask(___mask, ___image, ___width, ___height, ___depth, ___flags) \
	AROS_LC6(BOOL,                                                                     \
			 BuildTransDragMask,                                                       \
			 AROS_LCA(UWORD *, (___mask), A0),                                         \
			 AROS_LCA(UWORD *, (___image), A1),                                        \
			 AROS_LCA(short, (___width), D0),                                          \
			 AROS_LCA(short, (___height), D1),                                         \
			 AROS_LCA(short, (___depth), D2),                                          \
			 AROS_LCA(long, (___flags), D3),                                           \
			 struct Library *,                                                         \
			 DOPUS_BASE_NAME,                                                          \
			 254,                                                                      \
			 /* s */)

#define BytesToString(___bytes, ___string, ___places, ___sep) \
	AROS_LC4NR(void,                                             \
			 BytesToString,                                   \
			 AROS_LCA(ULONG, (___bytes), D0),                 \
			 AROS_LCA(char *, (___string), A0),               \
			 AROS_LCA(short, (___places), D1),                \
			 AROS_LCA(char, (___sep), D2),                    \
			 struct Library *,                                \
			 DOPUS_BASE_NAME,                                 \
			 13,                                              \
			 /* s */)

#define BytesToString64(___bytes, ___string, ___str_size, ___places, ___sep) \
	AROS_LC5NR(void,                                                            \
			 BytesToString64,                                                \
			 AROS_LCA(UQUAD *, (___bytes), A0),                              \
			 AROS_LCA(char *, (___string), A1),                              \
			 AROS_LCA(int, (___str_size), D0),                               \
			 AROS_LCA(int, (___places), D1),                                 \
			 AROS_LCA(char, (___sep), D2),                                   \
			 struct Library *,                                               \
			 DOPUS_BASE_NAME,                                                \
			 388,                                                            \
			 /* s */)

#define CLI_Launch(___name, ___screen, ___cd, ___in, ___out, ___wait, ___stack) \
	AROS_LC7(BOOL,                                                              \
			 CLI_Launch,                                                        \
			 AROS_LCA(char *, (___name), A0),                                   \
			 AROS_LCA(struct Screen *, (___screen), A1),                        \
			 AROS_LCA(BPTR, (___cd), D0),                                       \
			 AROS_LCA(BPTR, (___in), D1),                                       \
			 AROS_LCA(BPTR, (___out), D2),                                      \
			 AROS_LCA(short, (___wait), D3),                                    \
			 AROS_LCA(long, (___stack), D4),                                    \
			 struct Library *,                                                  \
			 DOPUS_BASE_NAME,                                                   \
			 194,                                                               \
			 /* s */)

#define CalcObjectDims(___parent, ___font, ___pos, ___dest, ___last, ___flags, ___obj, ___ob) \
	AROS_LC8(int,                                                                             \
			 CalcObjectDims,                                                                  \
			 AROS_LCA(void *, (___parent), A0),                                               \
			 AROS_LCA(struct TextFont *, (___font), A1),                                      \
			 AROS_LCA(struct IBox *, (___pos), A2),                                           \
			 AROS_LCA(struct IBox *, (___dest), A3),                                          \
			 AROS_LCA(GL_Object *, (___last), A4),                                            \
			 AROS_LCA(ULONG, (___flags), D0),                                                 \
			 AROS_LCA(GL_Object *, (___obj), D1),                                             \
			 AROS_LCA(GL_Object *, (___ob), D2),                                              \
			 struct Library *,                                                                \
			 DOPUS_BASE_NAME,                                                                 \
			 46,                                                                              \
			 /* s */)

#define CalcPercent(___amount, ___total, ___utilbase)       \
	AROS_LC3(long,                                          \
			 CalcPercent,                                   \
			 AROS_LCA(ULONG, (___amount), D0),              \
			 AROS_LCA(ULONG, (___total), D1),               \
			 AROS_LCA(struct Library *, (___utilbase), A0), \
			 struct Library *,                              \
			 DOPUS_BASE_NAME,                               \
			 346,                                           \
			 /* s */)

#define CalcWindowDims(___screen, ___pos, ___dest, ___font, ___flags) \
	AROS_LC5(int,                                                     \
			 CalcWindowDims,                                          \
			 AROS_LCA(struct Screen *, (___screen), A0),              \
			 AROS_LCA(ConfigWindow *, (___pos), A1),                  \
			 AROS_LCA(struct IBox *, (___dest), A2),                  \
			 AROS_LCA(struct TextFont *, (___font), A3),              \
			 AROS_LCA(ULONG, (___flags), D0),                         \
			 struct Library *,                                        \
			 DOPUS_BASE_NAME,                                         \
			 47,                                                      \
			 /* s */)

#define ChangeAppIcon(___icon, ___render, ___select, ___title, ___flags) \
	AROS_LC5NR(void,                                                        \
			 ChangeAppIcon,                                              \
			 AROS_LCA(APTR, (___icon), A0),                              \
			 AROS_LCA(struct Image *, (___render), A1),                  \
			 AROS_LCA(struct Image *, (___select), A2),                  \
			 AROS_LCA(char *, (___title), A3),                           \
			 AROS_LCA(ULONG, (___flags), D0),                            \
			 struct Library *,                                           \
			 DOPUS_BASE_NAME,                                            \
			 278,                                                        \
			 /* s */)

#define CheckAppMessage(___msg)                                                                                        \
	AROS_LC1(BOOL, CheckAppMessage, AROS_LCA(DOpusAppMessage *, (___msg), A0), struct Library *, DOPUS_BASE_NAME, 314, \
			 /* s */)

#define CheckDragDeadlock(___drag)                                                                                \
	AROS_LC1(BOOL, CheckDragDeadlock, AROS_LCA(DragInfo *, (___drag), A0), struct Library *, DOPUS_BASE_NAME, 37, \
			 /* s */)

#define CheckObjectArea(___object, ___x, ___y)       \
	AROS_LC3(BOOL,                                   \
			 CheckObjectArea,                        \
			 AROS_LCA(GL_Object *, (___object), A0), \
			 AROS_LCA(int, (___x), D0),              \
			 AROS_LCA(int, (___y), D1),              \
			 struct Library *,                       \
			 DOPUS_BASE_NAME,                        \
			 58,                                     \
			 /* s */)

#define CheckProgressAbort(___win) \
	AROS_LC1(BOOL, CheckProgressAbort, AROS_LCA(APTR, (___win), A0), struct Library *, DOPUS_BASE_NAME, 279, /* s */)

#define CheckRefreshMsg(___window, ___mask)              \
	AROS_LC2(struct IntuiMessage *,                      \
			 CheckRefreshMsg,                            \
			 AROS_LCA(struct Window *, (___window), A0), \
			 AROS_LCA(ULONG, (___mask), D0),             \
			 struct Library *,                           \
			 DOPUS_BASE_NAME,                            \
			 295,                                        \
			 /* s */)

#define CheckTimer(___handle)                                                                                        \
	AROS_LC1(BOOL, CheckTimer, AROS_LCA(TimerHandle *, (___handle), A0), struct Library *, DOPUS_BASE_NAME, 21, /* s \
																												 */)

#define CheckWindowDims(___window, ___dims)               \
	AROS_LC2(BOOL,                                        \
			 CheckWindowDims,                             \
			 AROS_LCA(struct Window *, (___window), A0),  \
			 AROS_LCA(WindowDimensions *, (___dims), A1), \
			 struct Library *,                            \
			 DOPUS_BASE_NAME,                             \
			 239,                                         \
			 /* s */)

#define ChecksumFile(___file, ___skip)        \
	AROS_LC2(ULONG,                           \
			 ChecksumFile,                    \
			 AROS_LCA(char *, (___file), A0), \
			 AROS_LCA(ULONG, (___skip), D0),  \
			 struct Library *,                \
			 DOPUS_BASE_NAME,                 \
			 196,                             \
			 /* s */)

#define ClearFiletypeCache() AROS_LC0NR(void,  ClearFiletypeCache, struct Library *, DOPUS_BASE_NAME, 332, /* s */)

#define ClearMemHandle(___handle) \
	AROS_LC1NR(void,  ClearMemHandle, AROS_LCA(void *, (___handle), A0), struct Library *, DOPUS_BASE_NAME, 113, /* s */)

#define ClearWindowBusy(___window)                                                                                     \
	AROS_LC1NR(void,  ClearWindowBusy, AROS_LCA(struct Window *, (___window), A0), struct Library *, DOPUS_BASE_NAME, 66, \
			 /* s */)

#define CloseBuf(___file) \
	AROS_LC1(long, CloseBuf, AROS_LCA(APTR, (___file), A0), struct Library *, DOPUS_BASE_NAME, 165, /* s */)

#define CloseButtonBank(___bank)                                                                                       \
	AROS_LC1NR(void,  CloseButtonBank, AROS_LCA(Cfg_ButtonBank *, (___bank), A0), struct Library *, DOPUS_BASE_NAME, 132, \
			 /* s */)

#define CloseClipBoard(___clip)                                                                                     \
	AROS_LC1NR(void,                                                                                                        \
		 CloseClipBoard, AROS_LCA(struct ClipHandle *, (___clip), A0), struct Library *, DOPUS_BASE_NAME, 211, \
		/* s */)

#define CloseConfigWindow(___window)                                                                                \
	AROS_LC1NR(void,                                                                                                        \
		 CloseConfigWindow, AROS_LCA(struct Window *, (___window), A0), struct Library *, DOPUS_BASE_NAME, 41, \
		/* s */)

#define CloseDisk(___handle)                                                                                        \
	AROS_LC1NR(void,  CloseDisk, AROS_LCA(DiskHandle *, (___handle), A0), struct Library *, DOPUS_BASE_NAME, 172, /* s \
																												*/)

#define CloseIFFFile(___iff)                                                                                         \
	AROS_LC1NR(void,  CloseIFFFile, AROS_LCA(struct IFFHandle *, (___iff), A0), struct Library *, DOPUS_BASE_NAME, 154, \
			 /* s */)

#define CloseImage(___image) \
	AROS_LC1NR(void,  CloseImage, AROS_LCA(APTR, (___image), A0), struct Library *, DOPUS_BASE_NAME, 106, /* s */)

#define CloseProgressWindow(___win) \
	AROS_LC1NR(void,  CloseProgressWindow, AROS_LCA(APTR, (___win), A0), struct Library *, DOPUS_BASE_NAME, 272, /* s */)

#define CloseWindowSafely(___window)                                                                                \
	AROS_LC1NR(void,                                                                                                        \
		 CloseWindowSafely, AROS_LCA(struct Window *, (___window), A0), struct Library *, DOPUS_BASE_NAME, 45, \
		/* s */)

#define CompareListFormat(___f1, ___f2)           \
	AROS_LC2(ULONG,                               \
			 CompareListFormat,                   \
			 AROS_LCA(ListFormat *, (___f1), A0), \
			 AROS_LCA(ListFormat *, (___f2), A1), \
			 struct Library *,                    \
			 DOPUS_BASE_NAME,                     \
			 303,                                 \
			 /* s */)

#define ConvertRawKey(___code, ___qual, ___key) \
	AROS_LC3(BOOL,                              \
			 ConvertRawKey,                     \
			 AROS_LCA(UWORD, (___code), D0),    \
			 AROS_LCA(UWORD, (___qual), D1),    \
			 AROS_LCA(char *, (___key), A0),    \
			 struct Library *,                  \
			 DOPUS_BASE_NAME,                   \
			 209,                               \
			 /* s */)

#define ConvertStartMenu(___bank)                                                                                  \
	AROS_LC1NR(void,                                                                                                       \
		 ConvertStartMenu, AROS_LCA(Cfg_ButtonBank *, (___bank), A0), struct Library *, DOPUS_BASE_NAME, 367, \
		/* s */)

#define CopyAppMessage(___msg, ___mem)                  \
	AROS_LC2(DOpusAppMessage *,                         \
			 CopyAppMessage,                            \
			 AROS_LCA(DOpusAppMessage *, (___msg), A0), \
			 AROS_LCA(APTR, (___mem), A1),              \
			 struct Library *,                          \
			 DOPUS_BASE_NAME,                           \
			 315,                                       \
			 /* s */)

#define CopyButton(___button, ___memory, ___type)     \
	AROS_LC3(Cfg_Button *,                            \
			 CopyButton,                              \
			 AROS_LCA(Cfg_Button *, (___button), A0), \
			 AROS_LCA(APTR, (___memory), A1),         \
			 AROS_LCA(short, (___type), D0),          \
			 struct Library *,                        \
			 DOPUS_BASE_NAME,                         \
			 141,                                     \
			 /* s */)

#define CopyButtonBank(___bank)                         \
	AROS_LC1(Cfg_ButtonBank *,                          \
			 CopyButtonBank,                            \
			 AROS_LCA(Cfg_ButtonBank *, (___bank), A0), \
			 struct Library *,                          \
			 DOPUS_BASE_NAME,                           \
			 140,                                       \
			 /* s */)

#define CopyButtonFunction(___function, ___memory, ___newfunc)  \
	AROS_LC3(Cfg_ButtonFunction *,                              \
			 CopyButtonFunction,                                \
			 AROS_LCA(Cfg_ButtonFunction *, (___function), A0), \
			 AROS_LCA(APTR, (___memory), A1),                   \
			 AROS_LCA(Cfg_ButtonFunction *, (___newfunc), A2),  \
			 struct Library *,                                  \
			 DOPUS_BASE_NAME,                                   \
			 283,                                               \
			 /* s */)

#define CopyDiskObject(___icon, ___flags)                  \
	AROS_LC2(struct DiskObject *,                          \
			 CopyDiskObject,                               \
			 AROS_LCA(struct DiskObject *, (___icon), A0), \
			 AROS_LCA(ULONG, (___flags), D0),              \
			 struct Library *,                             \
			 DOPUS_BASE_NAME,                              \
			 263,                                          \
			 /* s */)

#define CopyFiletype(___orig, ___memory)              \
	AROS_LC2(Cfg_Filetype *,                          \
			 CopyFiletype,                            \
			 AROS_LCA(Cfg_Filetype *, (___orig), A0), \
			 AROS_LCA(APTR, (___memory), A1),         \
			 struct Library *,                        \
			 DOPUS_BASE_NAME,                         \
			 148,                                     \
			 /* s */)

#define CopyFunction(___function, ___memory, ___newfunc)  \
	AROS_LC3(Cfg_Function *,                              \
			 CopyFunction,                                \
			 AROS_LCA(Cfg_Function *, (___function), A0), \
			 AROS_LCA(APTR, (___memory), A1),             \
			 AROS_LCA(Cfg_Function *, (___newfunc), A2),  \
			 struct Library *,                            \
			 DOPUS_BASE_NAME,                             \
			 142,                                         \
			 /* s */)

#define CopyImage(___image) \
	AROS_LC1(APTR, CopyImage, AROS_LCA(APTR, (___image), A0), struct Library *, DOPUS_BASE_NAME, 107, /* s */)

#define CopyLocalEnv(___base)                                                                                       \
	AROS_LC1NR(void,  CopyLocalEnv, AROS_LCA(struct Library *, (___base), A0), struct Library *, DOPUS_BASE_NAME, 193, \
			 /* s */)

#define CreateRexxMsgEx(___port, ___extension, ___host) \
	AROS_LC3(struct RexxMsg *,                          \
			 CreateRexxMsgEx,                           \
			 AROS_LCA(struct MsgPort *, (___port), A0), \
			 AROS_LCA(UBYTE *, (___extension), A1),     \
			 AROS_LCA(UBYTE *, (___host), D0),          \
			 struct Library *,                          \
			 DOPUS_BASE_NAME,                           \
			 336,                                       \
			 /* s */)

#define CreateTitleGadget(___scr, ___list, ___zoom, ___offset, ___type, ___id) \
	AROS_LC6(struct Gadget *,                                                  \
			 CreateTitleGadget,                                                \
			 AROS_LCA(struct Screen *, (___scr), A0),                          \
			 AROS_LCA(struct List *, (___list), A1),                           \
			 AROS_LCA(BOOL, (___zoom), D0),                                    \
			 AROS_LCA(short, (___offset), D1),                                 \
			 AROS_LCA(short, (___type), D2),                                   \
			 AROS_LCA(UWORD, (___id), D3),                                     \
			 struct Library *,                                                 \
			 DOPUS_BASE_NAME,                                                  \
			 326,                                                              \
			 /* s */)

#define DateFromStrings(___date, ___time, ___ds)        \
	AROS_LC3(BOOL,                                      \
			 DateFromStrings,                           \
			 AROS_LCA(char *, (___date), A0),           \
			 AROS_LCA(char *, (___time), A1),           \
			 AROS_LCA(struct DateStamp *, (___ds), A2), \
			 struct Library *,                          \
			 DOPUS_BASE_NAME,                           \
			 288,                                       \
			 /* s */)

#define DateFromStringsNew(___date, ___time, ___ds, ___method) \
	AROS_LC4(BOOL,                                             \
			 DateFromStringsNew,                               \
			 AROS_LCA(char *, (___date), A0),                  \
			 AROS_LCA(char *, (___time), A1),                  \
			 AROS_LCA(struct DateStamp *, (___ds), A2),        \
			 AROS_LCA(ULONG, (___method), D0),                 \
			 struct Library *,                                 \
			 DOPUS_BASE_NAME,                                  \
			 343,                                              \
			 /* s */)

#define DecodeILBM(___src, ___w, ___h, ___d, ___dst, ___mask, ___comp) \
	AROS_LC7NR(void,                                                      \
			 DecodeILBM,                                               \
			 AROS_LCA(char *, (___src), A0),                           \
			 AROS_LCA(UWORD, (___w), D0),                              \
			 AROS_LCA(UWORD, (___h), D1),                              \
			 AROS_LCA(UWORD, (___d), D2),                              \
			 AROS_LCA(struct BitMap *, (___dst), A1),                  \
			 AROS_LCA(ULONG, (___mask), D3),                           \
			 AROS_LCA(char, (___comp), D4),                            \
			 struct Library *,                                         \
			 DOPUS_BASE_NAME,                                          \
			 160,                                                      \
			 /* s */)

#define DecodeRLE(___rleinfo) \
	AROS_LC1NR(void,  DecodeRLE, AROS_LCA(RLEinfo *, (___rleinfo), A0), struct Library *, DOPUS_BASE_NAME, 161, /* s */)

#define DefaultButtonBank() \
	AROS_LC0(Cfg_ButtonBank *, DefaultButtonBank, struct Library *, DOPUS_BASE_NAME, 128, /* s */)

#define DefaultEnvironment(___env)                                                                                 \
	AROS_LC1NR(void,  DefaultEnvironment, AROS_LCA(CFG_ENVR *, (___env), A0), struct Library *, DOPUS_BASE_NAME, 127, \
			 /* s */)

#define DefaultSettings(___settings)                                                                                 \
	AROS_LC1NR(void,  DefaultSettings, AROS_LCA(CFG_SETS *, (___settings), A0), struct Library *, DOPUS_BASE_NAME, 126, \
			 /* s */)

#define DeleteIcon(___name) \
	AROS_LC1(BOOL, DeleteIcon, AROS_LCA(char *, (___name), A0), struct Library *, DOPUS_BASE_NAME, 364, /* s */)

#define DevNameFromLockDopus(___lock, ___buffer, ___len) \
	AROS_LC3(BOOL,                                       \
			 DevNameFromLockDopus,                       \
			 AROS_LCA(BPTR, (___lock), D1),              \
			 AROS_LCA(char *, (___buffer), D2),          \
			 AROS_LCA(long, (___len), D3),               \
			 struct Library *,                           \
			 DOPUS_BASE_NAME,                            \
			 249,                                        \
			 /* s */)

#define DeviceFromHandler(___port, ___name)             \
	AROS_LC2(struct DosList *,                          \
			 DeviceFromHandler,                         \
			 AROS_LCA(struct MsgPort *, (___port), A0), \
			 AROS_LCA(char *, (___name), A1),           \
			 struct Library *,                          \
			 DOPUS_BASE_NAME,                           \
			 248,                                       \
			 /* s */)

#define DeviceFromLock(___lock, ___name)      \
	AROS_LC2(struct DosList *,                \
			 DeviceFromLock,                  \
			 AROS_LCA(BPTR, (___lock), A0),   \
			 AROS_LCA(char *, (___name), A1), \
			 struct Library *,                \
			 DOPUS_BASE_NAME,                 \
			 247,                             \
			 /* s */)

#define DisableObject(___list, ___id, ___state)     \
	AROS_LC3NR(void,                                   \
			 DisableObject,                         \
			 AROS_LCA(ObjectList *, (___list), A0), \
			 AROS_LCA(ULONG, (___id), D0),          \
			 AROS_LCA(BOOL, (___state), D1),        \
			 struct Library *,                      \
			 DOPUS_BASE_NAME,                       \
			 61,                                    \
			 /* s */)

#define DisplayObject(___window, ___object, ___fg, ___bg, ___txt) \
	AROS_LC5NR(void,                                                 \
			 DisplayObject,                                       \
			 AROS_LCA(struct Window *, (___window), A0),          \
			 AROS_LCA(GL_Object *, (___object), A1),              \
			 AROS_LCA(int, (___fg), D0),                          \
			 AROS_LCA(int, (___bg), D1),                          \
			 AROS_LCA(char *, (___txt), A2),                      \
			 struct Library *,                                    \
			 DOPUS_BASE_NAME,                                     \
			 60,                                                  \
			 /* s */)

#define DisposeArgs(___args) \
	AROS_LC1NR(void,  DisposeArgs, AROS_LCA(FuncArgs *, (___args), A0), struct Library *, DOPUS_BASE_NAME, 309, /* s */)

#define DisposeBitMap(___b)                                                                                      \
	AROS_LC1NR(void,  DisposeBitMap, AROS_LCA(struct BitMap *, (___b), A0), struct Library *, DOPUS_BASE_NAME, 307, \
			 /* s */)

#define DivideToString(___string, ___num, ___div, ___places, ___sep) \
	AROS_LC5NR(void,                                                    \
			 DivideToString,                                         \
			 AROS_LCA(char *, (___string), A0),                      \
			 AROS_LCA(ULONG, (___num), D0),                          \
			 AROS_LCA(ULONG, (___div), D1),                          \
			 AROS_LCA(short, (___places), D2),                       \
			 AROS_LCA(char, (___sep), D3),                           \
			 struct Library *,                                       \
			 DOPUS_BASE_NAME,                                        \
			 14,                                                     \
			 /* s */)

#define DivideToString64(___string, ___str_size, ___bytes, ___div, ___places, ___sep) \
	AROS_LC6NR(void,                                                                     \
			 DivideToString64,                                                        \
			 AROS_LCA(char *, (___string), A0),                                       \
			 AROS_LCA(int, (___str_size), D0),                                        \
			 AROS_LCA(UQUAD *, (___bytes), A1),                                       \
			 AROS_LCA(ULONG, (___div), D1),                                           \
			 AROS_LCA(int, (___places), D2),                                          \
			 AROS_LCA(char, (___sep), D3),                                            \
			 struct Library *,                                                        \
			 DOPUS_BASE_NAME,                                                         \
			 387,                                                                     \
			 /* s */)

#define DivideU(___num, ___div, ___rem, ___utility)        \
	AROS_LC4(ULONG,                                        \
			 DivideU,                                      \
			 AROS_LCA(ULONG, (___num), D0),                \
			 AROS_LCA(ULONG, (___div), D1),                \
			 AROS_LCA(ULONG *, (___rem), A0),              \
			 AROS_LCA(struct Library *, (___utility), A1), \
			 struct Library *,                             \
			 DOPUS_BASE_NAME,                              \
			 9,                                            \
			 /* s */)

#define DivideU64(___num, ___div, ___rem, ___quo) \
	AROS_LC4NR(void,                                 \
			 DivideU64,                           \
			 AROS_LCA(UQUAD *, (___num), A0),     \
			 AROS_LCA(ULONG, (___div), D0),       \
			 AROS_LCA(UQUAD *, (___rem), A1),     \
			 AROS_LCA(UQUAD *, (___quo), A2),     \
			 struct Library *,                    \
			 DOPUS_BASE_NAME,                     \
			 385,                                 \
			 /* s */)

#define DoPopUpMenu(___window, ___menu, ___item, ___code) \
	AROS_LC4(UWORD,                                       \
			 DoPopUpMenu,                                 \
			 AROS_LCA(struct Window *, (___window), A0),  \
			 AROS_LCA(PopUpMenu *, (___menu), A1),        \
			 AROS_LCA(PopUpItem **, (___item), A2),       \
			 AROS_LCA(UWORD, (___code), D0),              \
			 struct Library *,                            \
			 DOPUS_BASE_NAME,                             \
			 89,                                          \
			 /* s */)

#define DoSimpleRequest(___window, ___simplereq)                        \
	AROS_LC2(short,                                                     \
			 DoSimpleRequest,                                           \
			 AROS_LCA(struct Window *, (___window), A0),                \
			 AROS_LCA(struct DOpusSimpleRequest *, (___simplereq), A1), \
			 struct Library *,                                          \
			 DOPUS_BASE_NAME,                                           \
			 25,                                                        \
			 /* s */)

#define DragCustomOk(___bm)                                                                                      \
	AROS_LC1(BOOL, DragCustomOk, AROS_LCA(struct BitMap *, (___bm), A0), struct Library *, DOPUS_BASE_NAME, 357, \
			 /* s */)

#define DrawBox(___rp, ___rect, ___info, ___recess)       \
	AROS_LC4NR(void,                                         \
			 DrawBox,                                     \
			 AROS_LCA(struct RastPort *, (___rp), A0),    \
			 AROS_LCA(struct Rectangle *, (___rect), A1), \
			 AROS_LCA(struct DrawInfo *, (___info), A2),  \
			 AROS_LCA(BOOL, (___recess), D0),             \
			 struct Library *,                            \
			 DOPUS_BASE_NAME,                             \
			 116,                                         \
			 /* s */)

#define DrawDragList(___rp, ___vp, ___flags)           \
	AROS_LC3NR(void,                                      \
			 DrawDragList,                             \
			 AROS_LCA(struct RastPort *, (___rp), A0), \
			 AROS_LCA(struct ViewPort *, (___vp), A1), \
			 AROS_LCA(long, (___flags), D0),           \
			 struct Library *,                         \
			 DOPUS_BASE_NAME,                          \
			 348,                                      \
			 /* s */)

#define DrawFieldBox(___rp, ___rect, ___info)             \
	AROS_LC3NR(void,                                         \
			 DrawFieldBox,                                \
			 AROS_LCA(struct RastPort *, (___rp), A0),    \
			 AROS_LCA(struct Rectangle *, (___rect), A1), \
			 AROS_LCA(struct DrawInfo *, (___info), A2),  \
			 struct Library *,                            \
			 DOPUS_BASE_NAME,                             \
			 117,                                         \
			 /* s */)

#define EndRefreshConfigWindow(___win)                                                                                 \
	AROS_LC1NR(void,                                                                                                           \
		 EndRefreshConfigWindow, AROS_LCA(struct Window *, (___win), A0), struct Library *, DOPUS_BASE_NAME, 302, \
		/* s */)

#define ExamineBuf(___file, ___fib)                          \
	AROS_LC2(long,                                           \
			 ExamineBuf,                                     \
			 AROS_LCA(APTR, (___file), A0),                  \
			 AROS_LCA(struct FileInfoBlock *, (___fib), A1), \
			 struct Library *,                               \
			 DOPUS_BASE_NAME,                                \
			 170,                                            \
			 /* s */)

#define FHFromBuf(___file) \
	AROS_LC1(BPTR, FHFromBuf, AROS_LCA(APTR, (___file), A0), struct Library *, DOPUS_BASE_NAME, 243, /* s */)

#define FakeILBM(___data, ___pal, ___w, ___h, ___d, ___flags) \
	AROS_LC6(ILBMHandle *,                                    \
			 FakeILBM,                                        \
			 AROS_LCA(UWORD *, (___data), A0),                \
			 AROS_LCA(ULONG *, (___pal), A1),                 \
			 AROS_LCA(short, (___w), D0),                     \
			 AROS_LCA(short, (___h), D1),                     \
			 AROS_LCA(short, (___d), D2),                     \
			 AROS_LCA(ULONG, (___flags), D3),                 \
			 struct Library *,                                \
			 DOPUS_BASE_NAME,                                 \
			 330,                                             \
			 /* s */)

#define FindBOOPSIGadget(___list, ___id)             \
	AROS_LC2(struct Gadget *,                        \
			 FindBOOPSIGadget,                       \
			 AROS_LCA(struct List *, (___list), A0), \
			 AROS_LCA(UWORD, (___id), D0),           \
			 struct Library *,                       \
			 DOPUS_BASE_NAME,                        \
			 174,                                    \
			 /* s */)

#define FindFunctionType(___list, ___type)           \
	AROS_LC2(Cfg_Function *,                         \
			 FindFunctionType,                       \
			 AROS_LCA(struct List *, (___list), A0), \
			 AROS_LCA(UWORD, (___type), D0),         \
			 struct Library *,                       \
			 DOPUS_BASE_NAME,                        \
			 149,                                    \
			 /* s */)

#define FindGadgetType(___gad, ___type)               \
	AROS_LC2(struct Gadget *,                         \
			 FindGadgetType,                          \
			 AROS_LCA(struct Gadget *, (___gad), A0), \
			 AROS_LCA(UWORD, (___type), D0),          \
			 struct Library *,                        \
			 DOPUS_BASE_NAME,                         \
			 327,                                     \
			 /* s */)

#define FindKeyEquivalent(___list, ___msg, ___process)      \
	AROS_LC3(struct Gadget *,                               \
			 FindKeyEquivalent,                             \
			 AROS_LCA(ObjectList *, (___list), A0),         \
			 AROS_LCA(struct IntuiMessage *, (___msg), A1), \
			 AROS_LCA(int, (___process), D0),               \
			 struct Library *,                              \
			 DOPUS_BASE_NAME,                               \
			 68,                                            \
			 /* s */)

#define FindMenuItem(___menu, ___id)                 \
	AROS_LC2(struct MenuItem *,                      \
			 FindMenuItem,                           \
			 AROS_LCA(struct Menu *, (___menu), A0), \
			 AROS_LCA(UWORD, (___id), D0),           \
			 struct Library *,                       \
			 DOPUS_BASE_NAME,                        \
			 88,                                     \
			 /* s */)

#define FindNameI(___list, ___name)                  \
	AROS_LC2(struct Node *,                          \
			 FindNameI,                              \
			 AROS_LCA(struct List *, (___list), A0), \
			 AROS_LCA(char *, (___name), A1),        \
			 struct Library *,                       \
			 DOPUS_BASE_NAME,                        \
			 206,                                    \
			 /* s */)

#define FindPubScreen(___scr, ___lock)                \
	AROS_LC2(struct PubScreenNode *,                  \
			 FindPubScreen,                           \
			 AROS_LCA(struct Screen *, (___scr), A0), \
			 AROS_LCA(BOOL, (___lock), D0),           \
			 struct Library *,                        \
			 DOPUS_BASE_NAME,                         \
			 284,                                     \
			 /* s */)

#define FixTitleGadgets(___win)                                                                                      \
	AROS_LC1NR(void,  FixTitleGadgets, AROS_LCA(struct Window *, (___win), A0), struct Library *, DOPUS_BASE_NAME, 328, \
			 /* s */)

#define FlushBuf(___file) \
	AROS_LC1(long, FlushBuf, AROS_LCA(APTR, (___file), A0), struct Library *, DOPUS_BASE_NAME, 168, /* s */)

#define FlushImages() AROS_LC0NR(void,  FlushImages, struct Library *, DOPUS_BASE_NAME, 108, /* s */)

#define FreeAppMessage(___msg)                                                                                        \
	AROS_LC1NR(void,  FreeAppMessage, AROS_LCA(DOpusAppMessage *, (___msg), A0), struct Library *, DOPUS_BASE_NAME, 298, \
			 /* s */)

#define FreeButton(___button)                                                                                        \
	AROS_LC1NR(void,  FreeButton, AROS_LCA(Cfg_Button *, (___button), A0), struct Library *, DOPUS_BASE_NAME, 136, /* s \
																												 */)

#define FreeButtonFunction(___func)                         \
	AROS_LC1NR(void,                                           \
			 FreeButtonFunction,                            \
			 AROS_LCA(Cfg_ButtonFunction *, (___func), A0), \
			 struct Library *,                              \
			 DOPUS_BASE_NAME,                               \
			 282,                                           \
			 /* s */)

#define FreeButtonImages(___list)                                                                                    \
	AROS_LC1NR(void,  FreeButtonImages, AROS_LCA(struct List *, (___list), A0), struct Library *, DOPUS_BASE_NAME, 135, \
			 /* s */)

#define FreeButtonList(___list)                                                                                    \
	AROS_LC1NR(void,  FreeButtonList, AROS_LCA(struct List *, (___list), A0), struct Library *, DOPUS_BASE_NAME, 134, \
			 /* s */)

#define FreeCachedDiskObject(___icon)                      \
	AROS_LC1NR(void,                                          \
			 FreeCachedDiskObject,                         \
			 AROS_LCA(struct DiskObject *, (___icon), A0), \
			 struct Library *,                             \
			 DOPUS_BASE_NAME,                              \
			 267,                                          \
			 /* s */)

#define FreeDiskObjectCopy(___icon)                        \
	AROS_LC1NR(void,                                          \
			 FreeDiskObjectCopy,                           \
			 AROS_LCA(struct DiskObject *, (___icon), A0), \
			 struct Library *,                             \
			 DOPUS_BASE_NAME,                              \
			 264,                                          \
			 /* s */)

#define FreeDosListCopy(___list)                                                                                    \
	AROS_LC1NR(void,  FreeDosListCopy, AROS_LCA(struct List *, (___list), A0), struct Library *, DOPUS_BASE_NAME, 342, \
			 /* s */)

#define FreeDosPathList(___list) \
	AROS_LC1NR(void,  FreeDosPathList, AROS_LCA(BPTR, (___list), A0), struct Library *, DOPUS_BASE_NAME, 24, /* s */)

#define FreeDragInfo(___draginfo)                                                                                \
	AROS_LC1NR(void,  FreeDragInfo, AROS_LCA(DragInfo *, (___draginfo), A0), struct Library *, DOPUS_BASE_NAME, 31, \
			 /* s */)

#define FreeEditHook(___hook)                                                                                    \
	AROS_LC1NR(void,  FreeEditHook, AROS_LCA(struct Hook *, (___hook), A0), struct Library *, DOPUS_BASE_NAME, 236, \
			 /* s */)

#define FreeFiletype(___type)                                                                                     \
	AROS_LC1NR(void,  FreeFiletype, AROS_LCA(Cfg_Filetype *, (___type), A0), struct Library *, DOPUS_BASE_NAME, 147, \
			 /* s */)

#define FreeFiletypeList(___list)                                                                                    \
	AROS_LC1NR(void,                                                                                                         \
		 FreeFiletypeList, AROS_LCA(Cfg_FiletypeList *, (___list), A0), struct Library *, DOPUS_BASE_NAME, 146, \
		/* s */)

#define FreeFunction(___function)                                                                                     \
	AROS_LC1NR(void,  FreeFunction, AROS_LCA(Cfg_Function *, (___function), A0), struct Library *, DOPUS_BASE_NAME, 137, \
			 /* s */)

#define FreeILBM(___ilbm) \
	AROS_LC1NR(void,  FreeILBM, AROS_LCA(ILBMHandle *, (___ilbm), A0), struct Library *, DOPUS_BASE_NAME, 159, /* s */)

#define FreeImageRemap(___remap)                                                                                   \
	AROS_LC1NR(void,  FreeImageRemap, AROS_LCA(ImageRemap *, (___remap), A0), struct Library *, DOPUS_BASE_NAME, 256, \
			 /* s */)

#define FreeInstruction(___ins)                                                                                        \
	AROS_LC1NR(void,  FreeInstruction, AROS_LCA(Cfg_Instruction *, (___ins), A0), struct Library *, DOPUS_BASE_NAME, 138, \
			 /* s */)

#define FreeInstructionList(___func)                                                                                \
	AROS_LC1NR(void,                                                                                                        \
		 FreeInstructionList, AROS_LCA(Cfg_Function *, (___func), A0), struct Library *, DOPUS_BASE_NAME, 139, \
		/* s */)

#define FreeListerDef(___lister)                                                                                   \
	AROS_LC1NR(void,  FreeListerDef, AROS_LCA(Cfg_Lister *, (___lister), A0), struct Library *, DOPUS_BASE_NAME, 133, \
			 /* s */)

#define FreeMatchHandle(___handle) \
	AROS_LC1NR(void,  FreeMatchHandle, AROS_LCA(APTR, (___handle), A0), struct Library *, DOPUS_BASE_NAME, 290, /* s */)

#define FreeMemH(___memory) \
	AROS_LC1NR(void,  FreeMemH, AROS_LCA(void *, (___memory), A0), struct Library *, DOPUS_BASE_NAME, 115, /* s */)

#define FreeMemHandle(___handle) \
	AROS_LC1NR(void,  FreeMemHandle, AROS_LCA(void *, (___handle), A0), struct Library *, DOPUS_BASE_NAME, 112, /* s */)

#define FreeObject(___objlist, ___object)              \
	AROS_LC2NR(void,                                      \
			 FreeObject,                               \
			 AROS_LCA(ObjectList *, (___objlist), A0), \
			 AROS_LCA(GL_Object *, (___object), A1),   \
			 struct Library *,                         \
			 DOPUS_BASE_NAME,                          \
			 49,                                       \
			 /* s */)

#define FreeObjectList(___objlist)                                                                                  \
	AROS_LC1NR(void,  FreeObjectList, AROS_LCA(ObjectList *, (___objlist), A0), struct Library *, DOPUS_BASE_NAME, 50, \
			 /* s */)

#define FreeRemapImage(___image, ___remap)           \
	AROS_LC2NR(void,                                    \
			 FreeRemapImage,                         \
			 AROS_LCA(APTR, (___image), A0),         \
			 AROS_LCA(ImageRemap *, (___remap), A1), \
			 struct Library *,                       \
			 DOPUS_BASE_NAME,                        \
			 297,                                    \
			 /* s */)

#define FreeRexxMsgEx(___msg)                                                                                       \
	AROS_LC1NR(void,  FreeRexxMsgEx, AROS_LCA(struct RexxMsg *, (___msg), A0), struct Library *, DOPUS_BASE_NAME, 335, \
			 /* s */)

#define FreeSemaphore(___sem)                                                                                          \
	AROS_LC1NR(void,                                                                                                           \
		 FreeSemaphore, AROS_LCA(struct SignalSemaphore *, (___sem), A0), struct Library *, DOPUS_BASE_NAME, 224, \
		/* s */)

#define FreeTimer(___handle)                                                                                        \
	AROS_LC1NR(void,  FreeTimer, AROS_LCA(TimerHandle *, (___handle), A0), struct Library *, DOPUS_BASE_NAME, 19, /* s \
																												*/)

#define FreeWindowMenus(___window)                                                                                     \
	AROS_LC1NR(void,  FreeWindowMenus, AROS_LCA(struct Window *, (___window), A0), struct Library *, DOPUS_BASE_NAME, 64, \
			 /* s */)

#define GetCachedDefDiskObject(___type)     \
	AROS_LC1(struct DiskObject *,           \
			 GetCachedDefDiskObject,        \
			 AROS_LCA(long, (___type), D0), \
			 struct Library *,              \
			 DOPUS_BASE_NAME,               \
			 266,                           \
			 /* s */)

#define GetCachedDiskObject(___name, ___flags) \
	AROS_LC2(struct DiskObject *,              \
			 GetCachedDiskObject,              \
			 AROS_LCA(char *, (___name), A0),  \
			 AROS_LCA(long, (___flags), D0),   \
			 struct Library *,                 \
			 DOPUS_BASE_NAME,                  \
			 268,                              \
			 /* s */)

#define GetCachedDiskObjectNew(___name, ___flags) \
	AROS_LC2(struct DiskObject *,                 \
			 GetCachedDiskObjectNew,              \
			 AROS_LCA(char *, (___name), A0),     \
			 AROS_LCA(ULONG, (___flags), D0),     \
			 struct Library *,                    \
			 DOPUS_BASE_NAME,                     \
			 269,                                 \
			 /* s */)

#define GetDeviceUnit(___startup, ___device, ___unit) \
	AROS_LC3(BOOL,                                    \
			 GetDeviceUnit,                           \
			 AROS_LCA(BPTR, (___startup), A0),        \
			 AROS_LCA(char *, (___device), A1),       \
			 AROS_LCA(short *, (___unit), A2),        \
			 struct Library *,                        \
			 DOPUS_BASE_NAME,                         \
			 362,                                     \
			 /* s */)

#define GetDosListCopy(___list, ___memory)           \
	AROS_LC2NR(void,                                    \
			 GetDosListCopy,                         \
			 AROS_LCA(struct List *, (___list), A0), \
			 AROS_LCA(APTR, (___memory), A1),        \
			 struct Library *,                       \
			 DOPUS_BASE_NAME,                        \
			 341,                                    \
			 /* s */)

#define GetDosPathList(___copy) \
	AROS_LC1(BPTR, GetDosPathList, AROS_LCA(BPTR, (___copy), A0), struct Library *, DOPUS_BASE_NAME, 23, /* s */)

#define GetDragImage(___draginfo, ___x, ___y)         \
	AROS_LC3NR(void,                                     \
			 GetDragImage,                            \
			 AROS_LCA(DragInfo *, (___draginfo), A0), \
			 AROS_LCA(ULONG, (___x), D0),             \
			 AROS_LCA(ULONG, (___y), D1),             \
			 struct Library *,                        \
			 DOPUS_BASE_NAME,                         \
			 32,                                      \
			 /* s */)

#define GetDragInfo(___window, ___rast, ___width, ___height, ___flags) \
	AROS_LC5(DragInfo *,                                               \
			 GetDragInfo,                                              \
			 AROS_LCA(struct Window *, (___window), A0),               \
			 AROS_LCA(struct RastPort *, (___rast), A1),               \
			 AROS_LCA(long, (___width), D0),                           \
			 AROS_LCA(long, (___height), D1),                          \
			 AROS_LCA(long, (___flags), D2),                           \
			 struct Library *,                                         \
			 DOPUS_BASE_NAME,                                          \
			 30,                                                       \
			 /* s */)

#define GetDragMask(___drag) \
	AROS_LC1NR(void,  GetDragMask, AROS_LCA(DragInfo *, (___drag), A0), struct Library *, DOPUS_BASE_NAME, 36, /* s */)

#define GetEditHook(___type, ___flags, ___tags)         \
	AROS_LC3(struct Hook *,                             \
			 GetEditHook,                               \
			 AROS_LCA(ULONG, (___type), D0),            \
			 AROS_LCA(ULONG, (___flags), D1),           \
			 AROS_LCA(struct TagItem *, (___tags), A0), \
			 struct Library *,                          \
			 DOPUS_BASE_NAME,                           \
			 235,                                       \
			 /* s */)

#ifndef NO_INLINE_STDARG
	#define GetEditHookTags(a0, a1, ...)                              \
		({                                                            \
			IPTR _tags[] = {AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__)}; \
			GetEditHook((a0), (a1), (struct TagItem *)_tags);         \
		})
#endif /* !NO_INLINE_STDARG */

#define GetFileVersion(___name, ___ver, ___rev, ___date, ___prog) \
	AROS_LC5(BOOL,                                                \
			 GetFileVersion,                                      \
			 AROS_LCA(char *, (___name), A0),                     \
			 AROS_LCA(short *, (___ver), D0),                     \
			 AROS_LCA(short *, (___rev), D1),                     \
			 AROS_LCA(struct DateStamp *, (___date), A1),         \
			 AROS_LCA(APTR, (___prog), A2),                       \
			 struct Library *,                                    \
			 DOPUS_BASE_NAME,                                     \
			 293,                                                 \
			 /* s */)

#define GetGadgetValue(___list, ___id)              \
	AROS_LC2(IPTR,                                  \
			 GetGadgetValue,                        \
			 AROS_LCA(ObjectList *, (___list), A0), \
			 AROS_LCA(UWORD, (___id), A1),          \
			 struct Library *,                      \
			 DOPUS_BASE_NAME,                       \
			 55,                                    \
			 /* s */)

#define GetIconFlags(___icon)                                                                                      \
	AROS_LC1(                                                                                                      \
		ULONG, GetIconFlags, AROS_LCA(struct DiskObject *, (___icon), A0), struct Library *, DOPUS_BASE_NAME, 250, \
		/* s */)

#define GetIconPosition(___icon, ___x, ___y)               \
	AROS_LC3NR(void,                                          \
			 GetIconPosition,                              \
			 AROS_LCA(struct DiskObject *, (___icon), A0), \
			 AROS_LCA(short *, (___x), A1),                \
			 AROS_LCA(short *, (___y), A2),                \
			 struct Library *,                             \
			 DOPUS_BASE_NAME,                              \
			 252,                                          \
			 /* s */)

#define GetIconType(___icon)                                                                                           \
	AROS_LC1(short, GetIconType, AROS_LCA(struct DiskObject *, (___icon), A0), struct Library *, DOPUS_BASE_NAME, 353, \
			 /* s */)

#define GetImageAttrs(___image, ___tags)                \
	AROS_LC2NR(void,                                       \
			 GetImageAttrs,                             \
			 AROS_LCA(APTR, (___image), A0),            \
			 AROS_LCA(struct TagItem *, (___tags), A1), \
			 struct Library *,                          \
			 DOPUS_BASE_NAME,                           \
			 110,                                       \
			 /* s */)

#ifndef NO_INLINE_STDARG
	#define GetImageAttrsTags(a0, ...)                                \
		({                                                            \
			IPTR _tags[] = {AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__)}; \
			GetImageAttrs((a0), (struct TagItem *)_tags);             \
		})
#endif /* !NO_INLINE_STDARG */

#define GetImagePalette(___image)                                                                                   \
	AROS_LC1(ULONG *, GetImagePalette, AROS_LCA(APTR, (___image), A0), struct Library *, DOPUS_BASE_NAME, 255, /* s \
																												*/)

#define GetLibraryFlags() AROS_LC0(ULONG, GetLibraryFlags, struct Library *, DOPUS_BASE_NAME, 352, /* s */)

#define GetMatchHandle(___name) \
	AROS_LC1(APTR, GetMatchHandle, AROS_LCA(char *, (___name), A0), struct Library *, DOPUS_BASE_NAME, 289, /* s */)

#define GetObject(___list, ___id)                   \
	AROS_LC2(GL_Object *,                           \
			 GetObject,                             \
			 AROS_LCA(ObjectList *, (___list), A0), \
			 AROS_LCA(int, (___id), D0),            \
			 struct Library *,                      \
			 DOPUS_BASE_NAME,                       \
			 52,                                    \
			 /* s */)

#define GetObjectRect(___list, ___id, ___rect)            \
	AROS_LC3(BOOL,                                        \
			 GetObjectRect,                               \
			 AROS_LCA(ObjectList *, (___list), A0),       \
			 AROS_LCA(ULONG, (___id), D0),                \
			 AROS_LCA(struct Rectangle *, (___rect), A1), \
			 struct Library *,                            \
			 DOPUS_BASE_NAME,                             \
			 59,                                          \
			 /* s */)

#define GetOpusPathList() AROS_LC0(BPTR, GetOpusPathList, struct Library *, DOPUS_BASE_NAME, 368, /* s */)

#define GetOriginalIcon(___icon)                           \
	AROS_LC1(struct DiskObject *,                          \
			 GetOriginalIcon,                              \
			 AROS_LCA(struct DiskObject *, (___icon), A0), \
			 struct Library *,                             \
			 DOPUS_BASE_NAME,                              \
			 345,                                          \
			 /* s */)

#define GetPalette32(___vp, ___palette, ___count, ___first) \
	AROS_LC4NR(void,                                           \
			 GetPalette32,                                  \
			 AROS_LCA(struct ViewPort *, (___vp), A0),      \
			 AROS_LCA(ULONG *, (___palette), A1),           \
			 AROS_LCA(UWORD, (___count), D0),               \
			 AROS_LCA(short, (___first), D1),               \
			 struct Library *,                              \
			 DOPUS_BASE_NAME,                               \
			 163,                                           \
			 /* s */)

#define GetPopUpImageSize(___window, ___menu, ___width, ___height) \
	AROS_LC4NR(void,                                                  \
			 GetPopUpImageSize,                                    \
			 AROS_LCA(struct Window *, (___window), A0),           \
			 AROS_LCA(PopUpMenu *, (___menu), A1),                 \
			 AROS_LCA(short *, (___width), A2),                    \
			 AROS_LCA(short *, (___height), A3),                   \
			 struct Library *,                                     \
			 DOPUS_BASE_NAME,                                      \
			 361,                                                  \
			 /* s */)

#define GetPopUpItem(___menu, ___id)               \
	AROS_LC2(PopUpItem *,                          \
			 GetPopUpItem,                         \
			 AROS_LCA(PopUpMenu *, (___menu), A0), \
			 AROS_LCA(UWORD, (___id), D0),         \
			 struct Library *,                     \
			 DOPUS_BASE_NAME,                      \
			 90,                                   \
			 /* s */)

#define GetProgressWindow(___win, ___tags)              \
	AROS_LC2NR(void,                                       \
			 GetProgressWindow,                         \
			 AROS_LCA(APTR, (___win), A0),              \
			 AROS_LCA(struct TagItem *, (___tags), A1), \
			 struct Library *,                          \
			 DOPUS_BASE_NAME,                           \
			 276,                                       \
			 /* s */)

#ifndef NO_INLINE_STDARG
	#define GetProgressWindowTags(a0, ...)                            \
		({                                                            \
			IPTR _tags[] = {AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__)}; \
			GetProgressWindow((a0), (struct TagItem *)_tags);         \
		})
#endif /* !NO_INLINE_STDARG */

#define GetRexxVarEx(___msg, ___varname, ___bufpointer) \
	AROS_LC3(long,                                      \
			 GetRexxVarEx,                              \
			 AROS_LCA(struct RexxMsg *, (___msg), A0),  \
			 AROS_LCA(char *, (___varname), A1),        \
			 AROS_LCA(char **, (___bufpointer), A2),    \
			 struct Library *,                          \
			 DOPUS_BASE_NAME,                           \
			 338,                                       \
			 /* s */)

#define GetSecureString(___gad)                                                                                        \
	AROS_LC1(char *, GetSecureString, AROS_LCA(struct Gadget *, (___gad), A0), struct Library *, DOPUS_BASE_NAME, 280, \
			 /* s */)

#define GetSemaphore(___sem, ___exc, ___data)                  \
	AROS_LC3(long,                                             \
			 GetSemaphore,                                     \
			 AROS_LCA(struct SignalSemaphore *, (___sem), A0), \
			 AROS_LCA(long, (___exc), D0),                     \
			 AROS_LCA(char *, (___data), A1),                  \
			 struct Library *,                                 \
			 DOPUS_BASE_NAME,                                  \
			 223,                                              \
			 /* s */)

#define GetStatistics(___id) \
	AROS_LC1(long, GetStatistics, AROS_LCA(long, (___id), D0), struct Library *, DOPUS_BASE_NAME, 369, /* s */)

#define GetString(___locale, ___num)                          \
	AROS_LC2(STRPTR,                                          \
			 GetString,                                       \
			 AROS_LCA(struct DOpusLocale *, (___locale), A0), \
			 AROS_LCA(LONG, (___num), D0),                    \
			 struct Library *,                                \
			 DOPUS_BASE_NAME,                                 \
			 67,                                              \
			 /* s */)

#define GetTimerBase() AROS_LC0(struct Library *, GetTimerBase, struct Library *, DOPUS_BASE_NAME, 333, /* s */)

#define GetWBArgPath(___arg, ___buf, ___size)        \
	AROS_LC3(BOOL,                                   \
			 GetWBArgPath,                           \
			 AROS_LCA(struct WBArg *, (___arg), A0), \
			 AROS_LCA(char *, (___buf), A1),         \
			 AROS_LCA(long, (___size), D0),          \
			 struct Library *,                       \
			 DOPUS_BASE_NAME,                        \
			 245,                                    \
			 /* s */)

#define GetWindowAppPort(___window)                      \
	AROS_LC1(struct MsgPort *,                           \
			 GetWindowAppPort,                           \
			 AROS_LCA(struct Window *, (___window), A0), \
			 struct Library *,                           \
			 DOPUS_BASE_NAME,                            \
			 72,                                         \
			 /* s */)

#define GetWindowID(___window)                                                                                      \
	AROS_LC1(ULONG, GetWindowID, AROS_LCA(struct Window *, (___window), A0), struct Library *, DOPUS_BASE_NAME, 71, \
			 /* s */)

#define GetWindowMsg(___port)                           \
	AROS_LC1(struct IntuiMessage *,                     \
			 GetWindowMsg,                              \
			 AROS_LCA(struct MsgPort *, (___port), A0), \
			 struct Library *,                          \
			 DOPUS_BASE_NAME,                           \
			 42,                                        \
			 /* s */)

#define HideDragImage(___draginfo)                                                                                \
	AROS_LC1NR(void,  HideDragImage, AROS_LCA(DragInfo *, (___draginfo), A0), struct Library *, DOPUS_BASE_NAME, 34, \
			 /* s */)

#define HideProgressWindow(___win) \
	AROS_LC1NR(void,  HideProgressWindow, AROS_LCA(APTR, (___win), A0), struct Library *, DOPUS_BASE_NAME, 273, /* s */)

#define IFFChunkID(___iff) \
	AROS_LC1(ULONG, IFFChunkID, AROS_LCA(APTR, (___iff), A0), struct Library *, DOPUS_BASE_NAME, 232, /* s */)

#define IFFChunkRemain(___iff) \
	AROS_LC1(long, IFFChunkRemain, AROS_LCA(APTR, (___iff), A0), struct Library *, DOPUS_BASE_NAME, 231, /* s */)

#define IFFChunkSize(___iff) \
	AROS_LC1(long, IFFChunkSize, AROS_LCA(APTR, (___iff), A0), struct Library *, DOPUS_BASE_NAME, 228, /* s */)

#define IFFClose(___handle) \
	AROS_LC1NR(void,  IFFClose, AROS_LCA(APTR, (___handle), A0), struct Library *, DOPUS_BASE_NAME, 201, /* s */)

#define IFFFailure(___iff) \
	AROS_LC1NR(void,  IFFFailure, AROS_LCA(APTR, (___iff), A0), struct Library *, DOPUS_BASE_NAME, 265, /* s */)

#define IFFFileHandle(___iff) \
	AROS_LC1(APTR, IFFFileHandle, AROS_LCA(APTR, (___iff), A0), struct Library *, DOPUS_BASE_NAME, 230, /* s */)

#define IFFGetFORM(___iff) \
	AROS_LC1(ULONG, IFFGetFORM, AROS_LCA(APTR, (___iff), A0), struct Library *, DOPUS_BASE_NAME, 233, /* s */)

#define IFFNextChunk(___iff, ___form)        \
	AROS_LC2(ULONG,                          \
			 IFFNextChunk,                   \
			 AROS_LCA(APTR, (___iff), A0),   \
			 AROS_LCA(ULONG, (___form), D0), \
			 struct Library *,               \
			 DOPUS_BASE_NAME,                \
			 227,                            \
			 /* s */)

#define IFFOpen(___name, ___mode, ___form)    \
	AROS_LC3(APTR,                            \
			 IFFOpen,                         \
			 AROS_LCA(char *, (___name), A0), \
			 AROS_LCA(UWORD, (___mode), D0),  \
			 AROS_LCA(ULONG, (___form), D1),  \
			 struct Library *,                \
			 DOPUS_BASE_NAME,                 \
			 200,                             \
			 /* s */)

#define IFFPopChunk(___handle) \
	AROS_LC1(long, IFFPopChunk, AROS_LCA(APTR, (___handle), A0), struct Library *, DOPUS_BASE_NAME, 204, /* s */)

#define IFFPushChunk(___handle, ___id)        \
	AROS_LC2(long,                            \
			 IFFPushChunk,                    \
			 AROS_LCA(APTR, (___handle), A0), \
			 AROS_LCA(ULONG, (___id), D0),    \
			 struct Library *,                \
			 DOPUS_BASE_NAME,                 \
			 202,                             \
			 /* s */)

#define IFFReadChunkBytes(___iff, ___buffer, ___size) \
	AROS_LC3(long,                                    \
			 IFFReadChunkBytes,                       \
			 AROS_LCA(APTR, (___iff), A0),            \
			 AROS_LCA(APTR, (___buffer), A1),         \
			 AROS_LCA(long, (___size), D0),           \
			 struct Library *,                        \
			 DOPUS_BASE_NAME,                         \
			 229,                                     \
			 /* s */)

#define IFFWriteChunk(___handle, ___data, ___chunk, ___size) \
	AROS_LC4(long,                                           \
			 IFFWriteChunk,                                  \
			 AROS_LCA(APTR, (___handle), A0),                \
			 AROS_LCA(APTR, (___data), A1),                  \
			 AROS_LCA(ULONG, (___chunk), D0),                \
			 AROS_LCA(ULONG, (___size), D1),                 \
			 struct Library *,                               \
			 DOPUS_BASE_NAME,                                \
			 205,                                            \
			 /* s */)

#define IFFWriteChunkBytes(___handle, ___data, ___size) \
	AROS_LC3(long,                                      \
			 IFFWriteChunkBytes,                        \
			 AROS_LCA(APTR, (___handle), A0),           \
			 AROS_LCA(APTR, (___data), A1),             \
			 AROS_LCA(long, (___size), D0),             \
			 struct Library *,                          \
			 DOPUS_BASE_NAME,                           \
			 203,                                       \
			 /* s */)

#define IPC_Command(___ipc, ___command, ___flags, ___data, ___data_free, ___reply) \
	AROS_LC6(IPTR,                                                                 \
			 IPC_Command,                                                          \
			 AROS_LCA(IPCData *, (___ipc), A0),                                    \
			 AROS_LCA(ULONG, (___command), D0),                                    \
			 AROS_LCA(IPTR, (___flags), D1),                                       \
			 AROS_LCA(APTR, (___data), A1),                                        \
			 AROS_LCA(APTR, (___data_free), A2),                                   \
			 AROS_LCA(struct MsgPort *, (___reply), A3),                           \
			 struct Library *,                                                     \
			 DOPUS_BASE_NAME,                                                      \
			 92,                                                                   \
			 /* s */)

#define IPC_FindProc(___list, ___name, ___act, ___data)  \
	AROS_LC4(IPCData *,                                  \
			 IPC_FindProc,                               \
			 AROS_LCA(struct ListLock *, (___list), A0), \
			 AROS_LCA(char *, (___name), A1),            \
			 AROS_LCA(BOOL, (___act), D0),               \
			 AROS_LCA(IPTR, (___data), D1),              \
			 struct Library *,                           \
			 DOPUS_BASE_NAME,                            \
			 95,                                         \
			 /* s */)

#define IPC_Flush(___port) \
	AROS_LC1NR(void,  IPC_Flush, AROS_LCA(IPCData *, (___port), A0), struct Library *, DOPUS_BASE_NAME, 101, /* s */)

#define IPC_Free(___ipc) \
	AROS_LC1NR(void,  IPC_Free, AROS_LCA(IPCData *, (___ipc), A0), struct Library *, DOPUS_BASE_NAME, 94, /* s */)

#define IPC_GetGoodbye(___msg)                                                                                   \
	AROS_LC1(IPTR, IPC_GetGoodbye, AROS_LCA(IPCMessage *, (___msg), A0), struct Library *, DOPUS_BASE_NAME, 99,  \
			 /* s */)

#define IPC_Goodbye(___ipc, ___owner, ___flags)   \
	AROS_LC3NR(void,                                 \
			 IPC_Goodbye,                         \
			 AROS_LCA(IPCData *, (___ipc), A0),   \
			 AROS_LCA(IPCData *, (___owner), A1), \
			 AROS_LCA(ULONG, (___flags), D0),     \
			 struct Library *,                    \
			 DOPUS_BASE_NAME,                     \
			 98,                                  \
			 /* s */)

#define IPC_Hello(___ipc, ___owner)               \
	AROS_LC2NR(void,                                 \
			 IPC_Hello,                           \
			 AROS_LCA(IPCData *, (___ipc), A0),   \
			 AROS_LCA(IPCData *, (___owner), A1), \
			 struct Library *,                    \
			 DOPUS_BASE_NAME,                     \
			 97,                                  \
			 /* s */)

#define IPC_Launch(___list, ___ipc, ___name, ___entry, ___stack, ___data, ___dos) \
	AROS_LC7(int,                                                                 \
			 IPC_Launch,                                                          \
			 AROS_LCA(struct ListLock *, (___list), A0),                          \
			 AROS_LCA(IPCData **, (___ipc), A1),                                  \
			 AROS_LCA(char *, (___name), A2),                                     \
			 AROS_LCA(IPTR, (___entry), D0),                                     \
			 AROS_LCA(ULONG, (___stack), D1),                                     \
			 AROS_LCA(IPTR, (___data), D2),                                      \
			 AROS_LCA(struct Library *, (___dos), A3),                            \
			 struct Library *,                                                    \
			 DOPUS_BASE_NAME,                                                     \
			 104,                                                                 \
			 /* s */)

#define IPC_ListCommand(___list, ___command, ___flags, ___data, ___wait) \
	AROS_LC5NR(void,                                                        \
			 IPC_ListCommand,                                            \
			 AROS_LCA(struct ListLock *, (___list), A0),                 \
			 AROS_LCA(ULONG, (___command), D0),                          \
			 AROS_LCA(IPTR, (___flags), D1),                             \
			 AROS_LCA(IPTR, (___data), D2),                             \
			 AROS_LCA(BOOL, (___wait), D3),                              \
			 struct Library *,                                           \
			 DOPUS_BASE_NAME,                                            \
			 102,                                                        \
			 /* s */)

#define IPC_ListQuit(___list, ___owner, ___flags, ___wait) \
	AROS_LC4(ULONG,                                        \
			 IPC_ListQuit,                                 \
			 AROS_LCA(struct ListLock *, (___list), A0),   \
			 AROS_LCA(IPCData *, (___owner), A1),          \
			 AROS_LCA(ULONG, (___flags), D0),              \
			 AROS_LCA(BOOL, (___wait), D1),                \
			 struct Library *,                             \
			 DOPUS_BASE_NAME,                              \
			 100,                                          \
			 /* s */)

#define IPC_ProcStartup(___data, ___code)                         \
	AROS_LC2(IPCData *,                                           \
			 IPC_ProcStartup,                                     \
			 AROS_LCA(IPTR *, (___data), A0),                    \
			 AROS_LCA(ULONG (*)(IPCData *, APTR), (___code), A1), \
			 struct Library *,                                    \
			 DOPUS_BASE_NAME,                                     \
			 103,                                                 \
			 /* s */)

#define IPC_Quit(___ipc, ___flags, ___wait)     \
	AROS_LC3NR(void,                               \
			 IPC_Quit,                          \
			 AROS_LCA(IPCData *, (___ipc), A0), \
			 AROS_LCA(ULONG, (___flags), D0),   \
			 AROS_LCA(BOOL, (___wait), D1),     \
			 struct Library *,                  \
			 DOPUS_BASE_NAME,                   \
			 96,                                \
			 /* s */)

#define IPC_QuitName(___list, ___name, ___flags)         \
	AROS_LC3NR(void,                                        \
			 IPC_QuitName,                               \
			 AROS_LCA(struct ListLock *, (___list), A0), \
			 AROS_LCA(char *, (___name), A1),            \
			 AROS_LCA(ULONG, (___flags), D0),            \
			 struct Library *,                           \
			 DOPUS_BASE_NAME,                            \
			 241,                                        \
			 /* s */)

#define IPC_Reply(___msg) \
	AROS_LC1NR(void,  IPC_Reply, AROS_LCA(IPCMessage *, (___msg), A0), struct Library *, DOPUS_BASE_NAME, 93, /* s */)

#define IPC_SafeCommand(___ipc, ___command, ___flags, ___data, ___data_free, ___reply, ___list) \
	AROS_LC7(IPTR,                                                                              \
			 IPC_SafeCommand,                                                                   \
			 AROS_LCA(IPCData *, (___ipc), A0),                                                 \
			 AROS_LCA(ULONG, (___command), D0),                                                 \
			 AROS_LCA(IPTR, (___flags), D1),                                                    \
			 AROS_LCA(APTR, (___data), A1),                                                     \
			 AROS_LCA(APTR, (___data_free), A2),                                                \
			 AROS_LCA(struct MsgPort *, (___reply), A3),                                        \
			 AROS_LCA(struct ListLock *, (___list), A4),                                        \
			 struct Library *,                                                                  \
			 DOPUS_BASE_NAME,                                                                   \
			 331,                                                                               \
			 /* s */)

#define IPC_Startup(___ipc, ___data, ___reply)           \
	AROS_LC3(int,                                        \
			 IPC_Startup,                                \
			 AROS_LCA(IPCData *, (___ipc), A0),          \
			 AROS_LCA(APTR, (___data), A1),              \
			 AROS_LCA(struct MsgPort *, (___reply), A2), \
			 struct Library *,                           \
			 DOPUS_BASE_NAME,                            \
			 91,                                         \
			 /* s */)

#define IconCheckSum(___icon, ___which)                    \
	AROS_LC2(ULONG,                                        \
			 IconCheckSum,                                 \
			 AROS_LCA(struct DiskObject *, (___icon), A0), \
			 AROS_LCA(short, (___which), D0),              \
			 struct Library *,                             \
			 DOPUS_BASE_NAME,                              \
			 270,                                          \
			 /* s */)

#define InitDragDBuf(___drag) \
	AROS_LC1(BOOL, InitDragDBuf, AROS_LCA(DragInfo *, (___drag), A0), struct Library *, DOPUS_BASE_NAME, 334, /* s */)

#define InitListLock(___ll, ___name)                   \
	AROS_LC2NR(void,                                      \
			 InitListLock,                             \
			 AROS_LCA(struct ListLock *, (___ll), A0), \
			 AROS_LCA(char *, (___name), A1),          \
			 struct Library *,                         \
			 DOPUS_BASE_NAME,                          \
			 240,                                      \
			 /* s */)

#define InitWindowDims(___window, ___dims)                \
	AROS_LC2NR(void,                                         \
			 InitWindowDims,                              \
			 AROS_LCA(struct Window *, (___window), A0),  \
			 AROS_LCA(WindowDimensions *, (___dims), A1), \
			 struct Library *,                            \
			 DOPUS_BASE_NAME,                             \
			 237,                                         \
			 /* s */)

#define IsDiskDevice(___port)                                                                                       \
	AROS_LC1(BOOL, IsDiskDevice, AROS_LCA(struct MsgPort *, (___port), A0), struct Library *, DOPUS_BASE_NAME, 347, \
			 /* s */)

#define IsListLockEmpty(___list)                                                                                   \
	AROS_LC1(                                                                                                      \
		BOOL, IsListLockEmpty, AROS_LCA(struct ListLock *, (___list), A0), struct Library *, DOPUS_BASE_NAME, 312, \
		/* s */)

#define Ito26(___num, ___str)                \
	AROS_LC2NR(void,                            \
			 Ito26,                          \
			 AROS_LCA(ULONG, (___num), D0),  \
			 AROS_LCA(char *, (___str), A0), \
			 struct Library *,               \
			 DOPUS_BASE_NAME,                \
			 12,                             \
			 /* s */)

#define Itoa(___num, ___str, ___sep)         \
	AROS_LC3NR(void,                            \
			 Itoa,                           \
			 AROS_LCA(long, (___num), D0),   \
			 AROS_LCA(char *, (___str), A0), \
			 AROS_LCA(char, (___sep), D1),   \
			 struct Library *,               \
			 DOPUS_BASE_NAME,                \
			 10,                             \
			 /* s */)

#define ItoaU(___num, ___str, ___sep)        \
	AROS_LC3NR(void,                            \
			 ItoaU,                          \
			 AROS_LCA(ULONG, (___num), D0),  \
			 AROS_LCA(char *, (___str), A0), \
			 AROS_LCA(char, (___sep), D1),   \
			 struct Library *,               \
			 DOPUS_BASE_NAME,                \
			 11,                             \
			 /* s */)

#define ItoaU64(___num, ___str, ___str_size, ___sep) \
	AROS_LC4NR(void,                                    \
			 ItoaU64,                                \
			 AROS_LCA(UQUAD *, (___num), A0),        \
			 AROS_LCA(char *, (___str), A1),         \
			 AROS_LCA(int, (___str_size), D0),       \
			 AROS_LCA(char, (___sep), D1),           \
			 struct Library *,                       \
			 DOPUS_BASE_NAME,                        \
			 386,                                    \
			 /* s */)

#define LayoutResize(___window)                                                                                      \
	AROS_LC1NR(void,  LayoutResize, AROS_LCA(struct Window *, (___window), A0), struct Library *, DOPUS_BASE_NAME, 292, \
			 /* s */)

#define LoadPalette32(___vp, ___palette)               \
	AROS_LC2NR(void,                                      \
			 LoadPalette32,                            \
			 AROS_LCA(struct ViewPort *, (___vp), A0), \
			 AROS_LCA(ULONG *, (___palette), A1),      \
			 struct Library *,                         \
			 DOPUS_BASE_NAME,                          \
			 162,                                      \
			 /* s */)

#define LoadPos(___name, ___pos, ___font)           \
	AROS_LC3(BOOL,                                  \
			 LoadPos,                               \
			 AROS_LCA(char *, (___name), A0),       \
			 AROS_LCA(struct IBox *, (___pos), A1), \
			 AROS_LCA(short *, (___font), D0),      \
			 struct Library *,                      \
			 DOPUS_BASE_NAME,                       \
			 383,                                   \
			 /* s */)

#define LockAppList() AROS_LC0(APTR, LockAppList, struct Library *, DOPUS_BASE_NAME, 184, /* s */)

#define LockAttList(___list, ___exclusive)        \
	AROS_LC2NR(void,                                 \
			 LockAttList,                         \
			 AROS_LCA(Att_List *, (___list), A0), \
			 AROS_LCA(short, (___exclusive), D0), \
			 struct Library *,                    \
			 DOPUS_BASE_NAME,                     \
			 214,                                 \
			 /* s */)

#define LockReqBackFill(___scr)                       \
	AROS_LC1(struct Hook *,                           \
			 LockReqBackFill,                         \
			 AROS_LCA(struct Screen *, (___scr), A0), \
			 struct Library *,                        \
			 DOPUS_BASE_NAME,                         \
			 355,                                     \
			 /* s */)

#define MUFSLogin(___window, ___name, ___password)       \
	AROS_LC3NR(void,                                        \
			 MUFSLogin,                                  \
			 AROS_LCA(struct Window *, (___window), A0), \
			 AROS_LCA(char *, (___name), A1),            \
			 AROS_LCA(char *, (___password), A2),        \
			 struct Library *,                           \
			 DOPUS_BASE_NAME,                            \
			 365,                                        \
			 /* s */)

#define MatchFiletype(___handle, ___type)     \
	AROS_LC2(BOOL,                            \
			 MatchFiletype,                   \
			 AROS_LCA(APTR, (___handle), A0), \
			 AROS_LCA(APTR, (___type), A1),   \
			 struct Library *,                \
			 DOPUS_BASE_NAME,                 \
			 291,                             \
			 /* s */)

#define NewBitMap(___w, ___h, ___d, ___f, ___b)     \
	AROS_LC5(struct BitMap *,                       \
			 NewBitMap,                             \
			 AROS_LCA(ULONG, (___w), D0),           \
			 AROS_LCA(ULONG, (___h), D1),           \
			 AROS_LCA(ULONG, (___d), D2),           \
			 AROS_LCA(ULONG, (___f), D3),           \
			 AROS_LCA(struct BitMap *, (___b), A0), \
			 struct Library *,                      \
			 DOPUS_BASE_NAME,                       \
			 306,                                   \
			 /* s */)

#define NewButton(___memory)                                                                                        \
	AROS_LC1(Cfg_Button *, NewButton, AROS_LCA(APTR, (___memory), A0), struct Library *, DOPUS_BASE_NAME, 120, /* s \
																												*/)

#define NewButtonBank(___init, ___type)      \
	AROS_LC2(Cfg_ButtonBank *,               \
			 NewButtonBank,                  \
			 AROS_LCA(BOOL, (___init), D0),  \
			 AROS_LCA(short, (___type), D1), \
			 struct Library *,               \
			 DOPUS_BASE_NAME,                \
			 119,                            \
			 /* s */)

#define NewButtonFunction(___memory, ___type) \
	AROS_LC2(Cfg_ButtonFunction *,            \
			 NewButtonFunction,               \
			 AROS_LCA(APTR, (___memory), A0), \
			 AROS_LCA(UWORD, (___type), D0),  \
			 struct Library *,                \
			 DOPUS_BASE_NAME,                 \
			 199,                             \
			 /* s */)

#define NewButtonWithFunc(___mem, ___label, ___type) \
	AROS_LC3(Cfg_Button *,                           \
			 NewButtonWithFunc,                      \
			 AROS_LCA(APTR, (___mem), A0),           \
			 AROS_LCA(char *, (___label), A1),       \
			 AROS_LCA(short, (___type), D0),         \
			 struct Library *,                       \
			 DOPUS_BASE_NAME,                        \
			 281,                                    \
			 /* s */)

#define NewFiletype(___memory)                                                                                     \
	AROS_LC1(Cfg_Filetype *, NewFiletype, AROS_LCA(APTR, (___memory), A0), struct Library *, DOPUS_BASE_NAME, 143, \
			 /* s */)

#define NewFunction(___memory, ___type)       \
	AROS_LC2(Cfg_Function *,                  \
			 NewFunction,                     \
			 AROS_LCA(APTR, (___memory), A0), \
			 AROS_LCA(UWORD, (___type), D0),  \
			 struct Library *,                \
			 DOPUS_BASE_NAME,                 \
			 121,                             \
			 /* s */)

#define NewInstruction(___memory, ___type, ___string) \
	AROS_LC3(Cfg_Instruction *,                       \
			 NewInstruction,                          \
			 AROS_LCA(APTR, (___memory), A0),         \
			 AROS_LCA(short, (___type), D0),          \
			 AROS_LCA(char *, (___string), A1),       \
			 struct Library *,                        \
			 DOPUS_BASE_NAME,                         \
			 122,                                     \
			 /* s */)

#define NewLister(___path)                                                                                          \
	AROS_LC1(Cfg_Lister *, NewLister, AROS_LCA(char *, (___path), A0), struct Library *, DOPUS_BASE_NAME, 118, /* s \
																												*/)

#define NewMemHandle(___puddle, ___thresh, ___type) \
	AROS_LC3(void *,                                \
			 NewMemHandle,                          \
			 AROS_LCA(ULONG, (___puddle), D0),      \
			 AROS_LCA(ULONG, (___thresh), D1),      \
			 AROS_LCA(ULONG, (___type), D2),        \
			 struct Library *,                      \
			 DOPUS_BASE_NAME,                       \
			 111,                                   \
			 /* s */)

#define NextAppEntry(___last, ___type)       \
	AROS_LC2(APTR,                           \
			 NextAppEntry,                   \
			 AROS_LCA(APTR, (___last), A0),  \
			 AROS_LCA(ULONG, (___type), D0), \
			 struct Library *,               \
			 DOPUS_BASE_NAME,                \
			 185,                            \
			 /* s */)

#define NotifyDiskChange() AROS_LC0NR(void,  NotifyDiskChange, struct Library *, DOPUS_BASE_NAME, 340, /* s */)

#define OpenBuf(___name, ___mode, ___buffer_size)  \
	AROS_LC3(APTR,                                 \
			 OpenBuf,                              \
			 AROS_LCA(char *, (___name), A0),      \
			 AROS_LCA(long, (___mode), D0),        \
			 AROS_LCA(long, (___buffer_size), D1), \
			 struct Library *,                     \
			 DOPUS_BASE_NAME,                      \
			 164,                                  \
			 /* s */)

#define OpenButtonBank(___name)                                                                                    \
	AROS_LC1(                                                                                                      \
		Cfg_ButtonBank *, OpenButtonBank, AROS_LCA(char *, (___name), A0), struct Library *, DOPUS_BASE_NAME, 125, \
		/* s */)

#define OpenClipBoard(___unit)                                                                                      \
	AROS_LC1(                                                                                                       \
		struct ClipHandle *, OpenClipBoard, AROS_LCA(ULONG, (___unit), D0), struct Library *, DOPUS_BASE_NAME, 210, \
		/* s */)

#define OpenConfigWindow(___newwindow)                        \
	AROS_LC1(struct Window *,                                 \
			 OpenConfigWindow,                                \
			 AROS_LCA(NewConfigWindow *, (___newwindow), A0), \
			 struct Library *,                                \
			 DOPUS_BASE_NAME,                                 \
			 40,                                              \
			 /* s */)

#define OpenDisk(___disk, ___port)                      \
	AROS_LC2(DiskHandle *,                              \
			 OpenDisk,                                  \
			 AROS_LCA(char *, (___disk), A0),           \
			 AROS_LCA(struct MsgPort *, (___port), A1), \
			 struct Library *,                          \
			 DOPUS_BASE_NAME,                           \
			 171,                                       \
			 /* s */)

#define OpenEnvironment(___name, ___data)                           \
	AROS_LC2(BOOL,                                                  \
			 OpenEnvironment,                                       \
			 AROS_LCA(char *, (___name), A0),                       \
			 AROS_LCA(struct OpenEnvironmentData *, (___data), A1), \
			 struct Library *,                                      \
			 DOPUS_BASE_NAME,                                       \
			 373,                                                   \
			 /* s */)

#define OpenIFFFile(___name, ___mode, ___id)  \
	AROS_LC3(struct IFFHandle *,              \
			 OpenIFFFile,                     \
			 AROS_LCA(char *, (___name), A0), \
			 AROS_LCA(int, (___mode), D0),    \
			 AROS_LCA(ULONG, (___id), D1),    \
			 struct Library *,                \
			 DOPUS_BASE_NAME,                 \
			 153,                             \
			 /* s */)

#define OpenImage(___name, ___info)                    \
	AROS_LC2(APTR,                                     \
			 OpenImage,                                \
			 AROS_LCA(char *, (___name), A0),          \
			 AROS_LCA(OpenImageInfo *, (___info), A1), \
			 struct Library *,                         \
			 DOPUS_BASE_NAME,                          \
			 105,                                      \
			 /* s */)

#define OpenProgressWindow(___tags)                                                                                  \
	AROS_LC1(                                                                                                        \
		APTR, OpenProgressWindow, AROS_LCA(struct TagItem *, (___tags), A0), struct Library *, DOPUS_BASE_NAME, 271, \
		/* s */)

#ifndef NO_INLINE_STDARG
	#define OpenProgressWindowTags(...)                               \
		({                                                            \
			IPTR _tags[] = {AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__)}; \
			OpenProgressWindow((struct TagItem *)_tags);              \
		})
#endif /* !NO_INLINE_STDARG */

#define OpenStatusWindow(___title, ___text, ___screen, ___graph, ___flags) \
	AROS_LC5(struct Window *,                                              \
			 OpenStatusWindow,                                             \
			 AROS_LCA(char *, (___title), A0),                             \
			 AROS_LCA(char *, (___text), A1),                              \
			 AROS_LCA(struct Screen *, (___screen), A2),                   \
			 AROS_LCA(LONG, (___graph), D1),                               \
			 AROS_LCA(ULONG, (___flags), D0),                              \
			 struct Library *,                                             \
			 DOPUS_BASE_NAME,                                              \
			 155,                                                          \
			 /* s */)

#define OriginalClose(___file) \
	AROS_LC1(BOOL, OriginalClose, AROS_LCA(BPTR, (___file), D1), struct Library *, DOPUS_BASE_NAME, 324, /* s */)

#define OriginalCreateDir(___name)                                                                                  \
	AROS_LC1(BPTR, OriginalCreateDir, AROS_LCA(char *, (___name), D1), struct Library *, DOPUS_BASE_NAME, 317, /* s \
																												*/)

#define OriginalDeleteFile(___name)                                                                                  \
	AROS_LC1(long, OriginalDeleteFile, AROS_LCA(char *, (___name), D1), struct Library *, DOPUS_BASE_NAME, 318, /* s \
																												 */)

#define OriginalOpen(___name, ___access)      \
	AROS_LC2(BPTR,                            \
			 OriginalOpen,                    \
			 AROS_LCA(char *, (___name), D1), \
			 AROS_LCA(LONG, (___access), D2), \
			 struct Library *,                \
			 DOPUS_BASE_NAME,                 \
			 323,                             \
			 /* s */)

#define OriginalRelabel(___dev, ___name)      \
	AROS_LC2(BOOL,                            \
			 OriginalRelabel,                 \
			 AROS_LCA(char *, (___dev), D1),  \
			 AROS_LCA(char *, (___name), D2), \
			 struct Library *,                \
			 DOPUS_BASE_NAME,                 \
			 329,                             \
			 /* s */)

#define OriginalRename(___oldname, ___newname)   \
	AROS_LC2(BOOL,                               \
			 OriginalRename,                     \
			 AROS_LCA(char *, (___oldname), D1), \
			 AROS_LCA(char *, (___newname), D2), \
			 struct Library *,                   \
			 DOPUS_BASE_NAME,                    \
			 322,                                \
			 /* s */)

#define OriginalSetComment(___name, ___comment)  \
	AROS_LC2(BOOL,                               \
			 OriginalSetComment,                 \
			 AROS_LCA(char *, (___name), D1),    \
			 AROS_LCA(char *, (___comment), D2), \
			 struct Library *,                   \
			 DOPUS_BASE_NAME,                    \
			 320,                                \
			 /* s */)

#define OriginalSetFileDate(___name, ___date)             \
	AROS_LC2(BOOL,                                        \
			 OriginalSetFileDate,                         \
			 AROS_LCA(char *, (___name), D1),             \
			 AROS_LCA(struct DateStamp *, (___date), D2), \
			 struct Library *,                            \
			 DOPUS_BASE_NAME,                             \
			 319,                                         \
			 /* s */)

#define OriginalSetProtection(___name, ___mask) \
	AROS_LC2(BOOL,                              \
			 OriginalSetProtection,             \
			 AROS_LCA(char *, (___name), D1),   \
			 AROS_LCA(ULONG, (___mask), D2),    \
			 struct Library *,                  \
			 DOPUS_BASE_NAME,                   \
			 321,                               \
			 /* s */)

#define OriginalWrite(___file, ___data, ___length) \
	AROS_LC3(LONG,                                 \
			 OriginalWrite,                        \
			 AROS_LCA(BPTR, (___file), D1),        \
			 AROS_LCA(void *, (___data), D2),      \
			 AROS_LCA(LONG, (___length), D3),      \
			 struct Library *,                     \
			 DOPUS_BASE_NAME,                      \
			 325,                                  \
			 /* s */)

#define ParseArgs(___temp, ___args)           \
	AROS_LC2(FuncArgs *,                      \
			 ParseArgs,                       \
			 AROS_LCA(char *, (___temp), A0), \
			 AROS_LCA(char *, (___args), A1), \
			 struct Library *,                \
			 DOPUS_BASE_NAME,                 \
			 308,                             \
			 /* s */)

#define ParseDateStrings(___string, ___date, ___time, ___range) \
	AROS_LC4(char *,                                            \
			 ParseDateStrings,                                  \
			 AROS_LCA(char *, (___string), A0),                 \
			 AROS_LCA(char *, (___date), A1),                   \
			 AROS_LCA(char *, (___time), A2),                   \
			 AROS_LCA(long *, (___range), A3),                  \
			 struct Library *,                                  \
			 DOPUS_BASE_NAME,                                   \
			 287,                                               \
			 /* s */)

#define PopUpEndSub(___handle)                                                                                    \
	AROS_LC1NR(void,  PopUpEndSub, AROS_LCA(PopUpHandle *, (___handle), A0), struct Library *, DOPUS_BASE_NAME, 379, \
			 /* s */)

#define PopUpFreeHandle(___handle)                                                                                    \
	AROS_LC1NR(void,  PopUpFreeHandle, AROS_LCA(PopUpHandle *, (___handle), A0), struct Library *, DOPUS_BASE_NAME, 375, \
			 /* s */)

#define PopUpItemSub(___handle, ___item)               \
	AROS_LC2(BOOL,                                     \
			 PopUpItemSub,                             \
			 AROS_LCA(PopUpHandle *, (___handle), A0), \
			 AROS_LCA(PopUpItem *, (___item), A1),     \
			 struct Library *,                         \
			 DOPUS_BASE_NAME,                          \
			 378,                                      \
			 /* s */)

#define PopUpNewHandle(___data, ___callback, ___locale)       \
	AROS_LC3(PopUpHandle *,                                   \
			 PopUpNewHandle,                                  \
			 AROS_LCA(IPTR, (___data), D0),                   \
			 AROS_LCA(REF_CALLBACK, (___callback), A0),       \
			 AROS_LCA(struct DOpusLocale *, (___locale), A1), \
			 struct Library *,                                \
			 DOPUS_BASE_NAME,                                 \
			 374,                                             \
			 /* s */)

#define PopUpNewItem(___handle, ___string, ___id, ___flags) \
	AROS_LC4(PopUpItem *,                                   \
			 PopUpNewItem,                                  \
			 AROS_LCA(PopUpHandle *, (___handle), A0),      \
			 AROS_LCA(IPTR, (___string), D0),               \
			 AROS_LCA(ULONG, (___id), D1),                  \
			 AROS_LCA(ULONG, (___flags), D2),               \
			 struct Library *,                              \
			 DOPUS_BASE_NAME,                               \
			 376,                                           \
			 /* s */)

#define PopUpSeparator(___handle)                                                                                    \
	AROS_LC1NR(void,  PopUpSeparator, AROS_LCA(PopUpHandle *, (___handle), A0), struct Library *, DOPUS_BASE_NAME, 377, \
			 /* s */)

#define PopUpSetFlags(___menu, ___id, ___value, ___mask) \
	AROS_LC4(ULONG,                                      \
			 PopUpSetFlags,                              \
			 AROS_LCA(PopUpMenu *, (___menu), A0),       \
			 AROS_LCA(UWORD, (___id), D0),               \
			 AROS_LCA(ULONG, (___value), D1),            \
			 AROS_LCA(ULONG, (___mask), D2),             \
			 struct Library *,                           \
			 DOPUS_BASE_NAME,                            \
			 380,                                        \
			 /* s */)

#define QualValid(___qual) \
	AROS_LC1(UWORD, QualValid, AROS_LCA(UWORD, (___qual), D0), struct Library *, DOPUS_BASE_NAME, 242, /* s */)

#define RandomDopus(___limit) \
	AROS_LC1(UWORD, RandomDopus, AROS_LCA(int, (___limit), D0), struct Library *, DOPUS_BASE_NAME, 6, /* s */)

#define ReadBuf(___file, ___data, ___size)    \
	AROS_LC3(long,                            \
			 ReadBuf,                         \
			 AROS_LCA(APTR, (___file), A0),   \
			 AROS_LCA(char *, (___data), A1), \
			 AROS_LCA(long, (___size), D0),   \
			 struct Library *,                \
			 DOPUS_BASE_NAME,                 \
			 166,                             \
			 /* s */)

#define ReadBufLine(___file, ___data, ___size) \
	AROS_LC3(long,                             \
			 ReadBufLine,                      \
			 AROS_LCA(APTR, (___file), A0),    \
			 AROS_LCA(char *, (___data), A1),  \
			 AROS_LCA(long, (___size), D0),    \
			 struct Library *,                 \
			 DOPUS_BASE_NAME,                  \
			 351,                              \
			 /* s */)

#define ReadButton(___iff, ___memory)         \
	AROS_LC2(Cfg_Button *,                    \
			 ReadButton,                      \
			 AROS_LCA(APTR, (___iff), A0),    \
			 AROS_LCA(APTR, (___memory), A1), \
			 struct Library *,                \
			 DOPUS_BASE_NAME,                 \
			 151,                             \
			 /* s */)

#define ReadClipString(___clip, ___string, ___len)         \
	AROS_LC3(long,                                         \
			 ReadClipString,                               \
			 AROS_LCA(struct ClipHandle *, (___clip), A0), \
			 AROS_LCA(char *, (___string), A1),            \
			 AROS_LCA(long, (___len), D0),                 \
			 struct Library *,                             \
			 DOPUS_BASE_NAME,                              \
			 213,                                          \
			 /* s */)

#define ReadFiletypes(___name, ___memory)     \
	AROS_LC2(Cfg_FiletypeList *,              \
			 ReadFiletypes,                   \
			 AROS_LCA(char *, (___name), A0), \
			 AROS_LCA(APTR, (___memory), A1), \
			 struct Library *,                \
			 DOPUS_BASE_NAME,                 \
			 144,                             \
			 /* s */)

#define ReadFunction(___iff, ___memory, ___funclist, ___function) \
	AROS_LC4(Cfg_Function *,                                      \
			 ReadFunction,                                        \
			 AROS_LCA(APTR, (___iff), A0),                        \
			 AROS_LCA(APTR, (___memory), A1),                     \
			 AROS_LCA(struct List *, (___funclist), A2),          \
			 AROS_LCA(Cfg_Function *, (___function), A3),         \
			 struct Library *,                                    \
			 DOPUS_BASE_NAME,                                     \
			 152,                                                 \
			 /* s */)

#define ReadILBM(___name, ___flags)           \
	AROS_LC2(ILBMHandle *,                    \
			 ReadILBM,                        \
			 AROS_LCA(char *, (___name), A0), \
			 AROS_LCA(ULONG, (___flags), D0), \
			 struct Library *,                \
			 DOPUS_BASE_NAME,                 \
			 158,                             \
			 /* s */)

#define ReadListerDef(___iff, ___id)       \
	AROS_LC2(Cfg_Lister *,                 \
			 ReadListerDef,                \
			 AROS_LCA(APTR, (___iff), A0), \
			 AROS_LCA(ULONG, (___id), D0), \
			 struct Library *,             \
			 DOPUS_BASE_NAME,              \
			 124,                          \
			 /* s */)

#define ReadSettings(___env, ___name)            \
	AROS_LC2(short,                              \
			 ReadSettings,                       \
			 AROS_LCA(CFG_SETS *, (___env), A0), \
			 AROS_LCA(char *, (___name), A1),    \
			 struct Library *,                   \
			 DOPUS_BASE_NAME,                    \
			 123,                                \
			 /* s */)

#define RefreshObjectList(___window, ___list)            \
	AROS_LC2NR(void,                                        \
			 RefreshObjectList,                          \
			 AROS_LCA(struct Window *, (___window), A0), \
			 AROS_LCA(ObjectList *, (___list), A1),      \
			 struct Library *,                           \
			 DOPUS_BASE_NAME,                            \
			 51,                                         \
			 /* s */)

#define RemAllocBitmapPatch(___handle)                                                                           \
	AROS_LC1NR(void,  RemAllocBitmapPatch, AROS_LCA(APTR, (___handle), A0), struct Library *, DOPUS_BASE_NAME, 382, \
			 /* s */)

#define RemDragImage(___drag) \
	AROS_LC1NR(void,  RemDragImage, AROS_LCA(DragInfo *, (___drag), A0), struct Library *, DOPUS_BASE_NAME, 39, /* s */)

#define RemapIcon(___icon, ___screen, ___free)             \
	AROS_LC3(BOOL,                                         \
			 RemapIcon,                                    \
			 AROS_LCA(struct DiskObject *, (___icon), A0), \
			 AROS_LCA(struct Screen *, (___screen), A1),   \
			 AROS_LCA(short, (___free), D0),               \
			 struct Library *,                             \
			 DOPUS_BASE_NAME,                              \
			 344,                                          \
			 /* s */)

#define RemapImage(___image, ___screen, ___remap)        \
	AROS_LC3(BOOL,                                       \
			 RemapImage,                                 \
			 AROS_LCA(APTR, (___image), A0),             \
			 AROS_LCA(struct Screen *, (___screen), A1), \
			 AROS_LCA(ImageRemap *, (___remap), A2),     \
			 struct Library *,                           \
			 DOPUS_BASE_NAME,                            \
			 296,                                        \
			 /* s */)

#define RemoveDragImage(___drag)                                                                                 \
	AROS_LC1NR(void,  RemoveDragImage, AROS_LCA(DragInfo *, (___drag), A0), struct Library *, DOPUS_BASE_NAME, 349, \
			 /* s */)

#define RemoveNotifyRequest(___node)                                                                                \
	AROS_LC1NR(void,  RemoveNotifyRequest, AROS_LCA(APTR, (___node), A0), struct Library *, DOPUS_BASE_NAME, 188, /* s \
																												*/)

#define RemovedFunc1() AROS_LC0NR(void,  RemovedFunc1, struct Library *, DOPUS_BASE_NAME, 216, /* s */)

#define RemovedFunc10() AROS_LC0NR(void,  RemovedFunc10, struct Library *, DOPUS_BASE_NAME, 260, /* s */)

#define RemovedFunc11() AROS_LC0NR(void,  RemovedFunc11, struct Library *, DOPUS_BASE_NAME, 261, /* s */)

#define RemovedFunc12() AROS_LC0NR(void,  RemovedFunc12, struct Library *, DOPUS_BASE_NAME, 262, /* s */)

#define RemovedFunc2() AROS_LC0NR(void,  RemovedFunc2, struct Library *, DOPUS_BASE_NAME, 217, /* s */)

#define RemovedFunc3() AROS_LC0NR(void,  RemovedFunc3, struct Library *, DOPUS_BASE_NAME, 218, /* s */)

#define RemovedFunc4() AROS_LC0NR(void,  RemovedFunc4, struct Library *, DOPUS_BASE_NAME, 219, /* s */)

#define RemovedFunc5() AROS_LC0NR(void,  RemovedFunc5, struct Library *, DOPUS_BASE_NAME, 220, /* s */)

#define RemovedFunc6() AROS_LC0NR(void,  RemovedFunc6, struct Library *, DOPUS_BASE_NAME, 221, /* s */)

#define RemovedFunc7() AROS_LC0NR(void,  RemovedFunc7, struct Library *, DOPUS_BASE_NAME, 222, /* s */)

#define RemovedFunc8() AROS_LC0NR(void,  RemovedFunc8, struct Library *, DOPUS_BASE_NAME, 246, /* s */)

#define RemovedFunc9() AROS_LC0NR(void,  RemovedFunc9, struct Library *, DOPUS_BASE_NAME, 258, /* s */)

#define RenderImage(___rp, ___image, ___left, ___top, ___tags) \
	AROS_LC5(short,                                            \
			 RenderImage,                                      \
			 AROS_LCA(struct RastPort *, (___rp), A0),         \
			 AROS_LCA(APTR, (___image), A1),                   \
			 AROS_LCA(UWORD, (___left), D0),                   \
			 AROS_LCA(UWORD, (___top), D1),                    \
			 AROS_LCA(struct TagItem *, (___tags), A2),        \
			 struct Library *,                                 \
			 DOPUS_BASE_NAME,                                  \
			 109,                                              \
			 /* s */)

#ifndef NO_INLINE_STDARG
	#define RenderImageTags(a0, a1, a2, a3, ...)                          \
		({                                                                \
			IPTR _tags[] = {AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__)};     \
			RenderImage((a0), (a1), (a2), (a3), (struct TagItem *)_tags); \
		})
#endif /* !NO_INLINE_STDARG */

#define ReplyAppMessage(___msg)                                                                                        \
	AROS_LC1NR(void,  ReplyAppMessage, AROS_LCA(DOpusAppMessage *, (___msg), A0), struct Library *, DOPUS_BASE_NAME, 299, \
			 /* s */)

#define ReplyFreeMsg(___msg) \
	AROS_LC1NR(void,  ReplyFreeMsg, AROS_LCA(APTR, (___msg), A0), struct Library *, DOPUS_BASE_NAME, 197, /* s */)

#define ReplyWindowMsg(___msg)                                                                                      \
	AROS_LC1NR(void,                                                                                                        \
		 ReplyWindowMsg, AROS_LCA(struct IntuiMessage *, (___msg), A0), struct Library *, DOPUS_BASE_NAME, 43, \
		/* s */)

#define SaveButton(___iff, ___button)                 \
	AROS_LC2(short,                                   \
			 SaveButton,                              \
			 AROS_LCA(APTR, (___iff), A0),            \
			 AROS_LCA(Cfg_Button *, (___button), A1), \
			 struct Library *,                        \
			 DOPUS_BASE_NAME,                         \
			 150,                                     \
			 /* s */)

#define SaveButtonBank(___bank, ___name)                \
	AROS_LC2(short,                                     \
			 SaveButtonBank,                            \
			 AROS_LCA(Cfg_ButtonBank *, (___bank), A0), \
			 AROS_LCA(char *, (___name), A1),           \
			 struct Library *,                          \
			 DOPUS_BASE_NAME,                           \
			 131,                                       \
			 /* s */)

#define SaveFiletypeList(___list, ___name)                \
	AROS_LC2(int,                                         \
			 SaveFiletypeList,                            \
			 AROS_LCA(Cfg_FiletypeList *, (___list), A0), \
			 AROS_LCA(char *, (___name), A1),             \
			 struct Library *,                            \
			 DOPUS_BASE_NAME,                             \
			 145,                                         \
			 /* s */)

#define SaveFunction(___iff, ___func)                 \
	AROS_LC2(BOOL,                                    \
			 SaveFunction,                            \
			 AROS_LCA(APTR, (___iff), A0),            \
			 AROS_LCA(Cfg_Function *, (___func), A1), \
			 struct Library *,                        \
			 DOPUS_BASE_NAME,                         \
			 226,                                     \
			 /* s */)

#define SaveListerDef(___iff, ___lister)              \
	AROS_LC2(long,                                    \
			 SaveListerDef,                           \
			 AROS_LCA(APTR, (___iff), A0),            \
			 AROS_LCA(Cfg_Lister *, (___lister), A1), \
			 struct Library *,                        \
			 DOPUS_BASE_NAME,                         \
			 130,                                     \
			 /* s */)

#define SavePos(___name, ___pos, ___font)           \
	AROS_LC3(BOOL,                                  \
			 SavePos,                               \
			 AROS_LCA(char *, (___name), A0),       \
			 AROS_LCA(struct IBox *, (___pos), A1), \
			 AROS_LCA(short, (___font), D0),        \
			 struct Library *,                      \
			 DOPUS_BASE_NAME,                       \
			 384,                                   \
			 /* s */)

#define SaveSettings(___env, ___name)            \
	AROS_LC2(int,                                \
			 SaveSettings,                       \
			 AROS_LCA(CFG_SETS *, (___env), A0), \
			 AROS_LCA(char *, (___name), A1),    \
			 struct Library *,                   \
			 DOPUS_BASE_NAME,                    \
			 129,                                \
			 /* s */)

#define ScreenInfo(___screen)                                                                                       \
	AROS_LC1(ULONG, ScreenInfo, AROS_LCA(struct Screen *, (___screen), A0), struct Library *, DOPUS_BASE_NAME, 234, \
			 /* s */)

#define SearchFile(___file, ___text, ___flags, ___buffer, ___size) \
	AROS_LC5(long,                                                 \
			 SearchFile,                                           \
			 AROS_LCA(APTR, (___file), A0),                        \
			 AROS_LCA(UBYTE *, (___text), A1),                     \
			 AROS_LCA(ULONG, (___flags), D0),                      \
			 AROS_LCA(UBYTE *, (___buffer), A2),                   \
			 AROS_LCA(ULONG, (___size), D1),                       \
			 struct Library *,                                     \
			 DOPUS_BASE_NAME,                                      \
			 286,                                                  \
			 /* s */)

#define Seed(___seed) \
	AROS_LC1NR(void,  Seed, AROS_LCA(int, (___seed), D0), struct Library *, DOPUS_BASE_NAME, 259, /* s */)

#define SeekBuf(___file, ___offset, ___mode)  \
	AROS_LC3(long,                            \
			 SeekBuf,                         \
			 AROS_LCA(APTR, (___file), A0),   \
			 AROS_LCA(long, (___offset), D0), \
			 AROS_LCA(long, (___mode), D1),   \
			 struct Library *,                \
			 DOPUS_BASE_NAME,                 \
			 169,                             \
			 /* s */)

#define SelectionList(___list,                            \
					  ___parent,                          \
					  ___screen,                          \
					  ___title,                           \
					  ___ok,                              \
					  ___cancel,                          \
					  ___sel,                             \
					  ___flags,                           \
					  ___buffer,                          \
					  ___switchtxt,                       \
					  ___switchflags)                     \
	AROS_LC11(short,                                      \
			  SelectionList,                              \
			  AROS_LCA(Att_List *, (___list), A0),        \
			  AROS_LCA(struct Window *, (___parent), A1), \
			  AROS_LCA(struct Screen *, (___screen), A2), \
			  AROS_LCA(char *, (___title), A3),           \
			  AROS_LCA(short, (___ok), D0),               \
			  AROS_LCA(ULONG, (___cancel), D1),           \
			  AROS_LCA(char *, (___sel), D2),             \
			  AROS_LCA(char *, (___flags), D3),           \
			  AROS_LCA(char *, (___buffer), D4),          \
			  AROS_LCA(char **, (___switchtxt), A4),      \
			  AROS_LCA(ULONG *, (___switchflags), A5),    \
			  struct Library *,                           \
			  DOPUS_BASE_NAME,                            \
			  27,                                         \
			  /* s */)

#define SendNotifyMsg(___type, ___data, ___flags, ___wait, ___name, ___fib) \
	AROS_LC6NR(void,                                                           \
			 SendNotifyMsg,                                                 \
			 AROS_LCA(ULONG, (___type), D0),                                \
			 AROS_LCA(IPTR, (___data), D1),                                 \
			 AROS_LCA(ULONG, (___flags), D2),                               \
			 AROS_LCA(short, (___wait), D3),                                \
			 AROS_LCA(char *, (___name), A0),                               \
			 AROS_LCA(struct FileInfoBlock *, (___fib), A1),                \
			 struct Library *,                                              \
			 DOPUS_BASE_NAME,                                               \
			 189,                                                           \
			 /* s */)

#define SerialCheck(___num, ___p)            \
	AROS_LC2(BOOL,                           \
			 SerialCheck,                    \
			 AROS_LCA(char *, (___num), A0), \
			 AROS_LCA(ULONG *, (___p), A1),  \
			 struct Library *,               \
			 DOPUS_BASE_NAME,                \
			 195,                            \
			 /* s */)

#define SerialValid(___data)                                                                                         \
	AROS_LC1(BOOL, SerialValid, AROS_LCA(serial_data *, (___data), A0), struct Library *, DOPUS_BASE_NAME, 176, /* s \
																												 */)

#define SetAppIconMenuState(___icon, ___item, ___state) \
	AROS_LC3(long,                                      \
			 SetAppIconMenuState,                       \
			 AROS_LCA(APTR, (___icon), A0),             \
			 AROS_LCA(long, (___item), D0),             \
			 AROS_LCA(long, (___state), D1),            \
			 struct Library *,                          \
			 DOPUS_BASE_NAME,                           \
			 285,                                       \
			 /* s */)

#define SetBusyPointer(___window)                                                                                     \
	AROS_LC1NR(void,  SetBusyPointer, AROS_LCA(struct Window *, (___window), A0), struct Library *, DOPUS_BASE_NAME, 15, \
			 /* s */)

#define SetConfigWindowLimits(___window, ___min, ___max) \
	AROS_LC3NR(void,                                        \
			 SetConfigWindowLimits,                      \
			 AROS_LCA(struct Window *, (___window), A0), \
			 AROS_LCA(ConfigWindow *, (___min), A1),     \
			 AROS_LCA(ConfigWindow *, (___max), A2),     \
			 struct Library *,                           \
			 DOPUS_BASE_NAME,                            \
			 310,                                        \
			 /* s */)

#define SetEnv(___name, ___data, ___save)     \
	AROS_LC3NR(void,                             \
			 SetEnv,                          \
			 AROS_LCA(char *, (___name), A0), \
			 AROS_LCA(char *, (___data), A1), \
			 AROS_LCA(BOOL, (___save), D0),   \
			 struct Library *,                \
			 DOPUS_BASE_NAME,                 \
			 311,                             \
			 /* s */)

#define SetGadgetChoices(___list, ___id, ___choices) \
	AROS_LC3NR(void,                                    \
			 SetGadgetChoices,                       \
			 AROS_LCA(ObjectList *, (___list), A0),  \
			 AROS_LCA(ULONG, (___id), D0),           \
			 AROS_LCA(APTR, (___choices), A1),       \
			 struct Library *,                       \
			 DOPUS_BASE_NAME,                        \
			 57,                                     \
			 /* s */)

#define SetGadgetValue(___list, ___id, ___value)    \
	AROS_LC3NR(void,                                   \
			 SetGadgetValue,                        \
			 AROS_LCA(ObjectList *, (___list), A0), \
			 AROS_LCA(UWORD, (___id), D0),          \
			 AROS_LCA(IPTR, (___value), D1),        \
			 struct Library *,                      \
			 DOPUS_BASE_NAME,                       \
			 54,                                    \
			 /* s */)

#define SetIconFlags(___icon, ___flags)                    \
	AROS_LC2NR(void,                                          \
			 SetIconFlags,                                 \
			 AROS_LCA(struct DiskObject *, (___icon), A0), \
			 AROS_LCA(ULONG, (___flags), D0),              \
			 struct Library *,                             \
			 DOPUS_BASE_NAME,                              \
			 251,                                          \
			 /* s */)

#define SetIconPosition(___icon, ___x, ___y)               \
	AROS_LC3NR(void,                                          \
			 SetIconPosition,                              \
			 AROS_LCA(struct DiskObject *, (___icon), A0), \
			 AROS_LCA(short, (___x), D0),                  \
			 AROS_LCA(short, (___y), D1),                  \
			 struct Library *,                             \
			 DOPUS_BASE_NAME,                              \
			 253,                                          \
			 /* s */)

#define SetLibraryFlags(___flags, ___mask)    \
	AROS_LC2(ULONG,                           \
			 SetLibraryFlags,                 \
			 AROS_LCA(ULONG, (___flags), D0), \
			 AROS_LCA(ULONG, (___mask), D1),  \
			 struct Library *,                \
			 DOPUS_BASE_NAME,                 \
			 300,                             \
			 /* s */)

#define SetNewIconsFlags(___flags, ___prec)   \
	AROS_LC2NR(void,                             \
			 SetNewIconsFlags,                \
			 AROS_LCA(ULONG, (___flags), D0), \
			 AROS_LCA(short, (___prec), D1),  \
			 struct Library *,                \
			 DOPUS_BASE_NAME,                 \
			 350,                             \
			 /* s */)

#define SetNotifyRequest(___req, ___flags, ___mask) \
	AROS_LC3NR(void,                                   \
			 SetNotifyRequest,                      \
			 AROS_LCA(APTR, (___req), A0),          \
			 AROS_LCA(ULONG, (___flags), D0),       \
			 AROS_LCA(ULONG, (___mask), D1),        \
			 struct Library *,                      \
			 DOPUS_BASE_NAME,                       \
			 277,                                   \
			 /* s */)

#define SetObjectKind(___list, ___id, ___kind)      \
	AROS_LC3NR(void,                                   \
			 SetObjectKind,                         \
			 AROS_LCA(ObjectList *, (___list), A0), \
			 AROS_LCA(ULONG, (___id), D0),          \
			 AROS_LCA(UWORD, (___kind), D1),        \
			 struct Library *,                      \
			 DOPUS_BASE_NAME,                       \
			 56,                                    \
			 /* s */)

#define SetPopUpDelay(___delay) \
	AROS_LC1NR(void,  SetPopUpDelay, AROS_LCA(short, (___delay), D0), struct Library *, DOPUS_BASE_NAME, 370, /* s */)

#define SetProgressWindow(___win, ___tags)              \
	AROS_LC2NR(void,                                       \
			 SetProgressWindow,                         \
			 AROS_LCA(APTR, (___win), A0),              \
			 AROS_LCA(struct TagItem *, (___tags), A1), \
			 struct Library *,                          \
			 DOPUS_BASE_NAME,                           \
			 275,                                       \
			 /* s */)

#ifndef NO_INLINE_STDARG
	#define SetProgressWindowTags(a0, ...)                            \
		({                                                            \
			IPTR _tags[] = {AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__)}; \
			SetProgressWindow((a0), (struct TagItem *)_tags);         \
		})
#endif /* !NO_INLINE_STDARG */

#define SetReqBackFill(___hook, ___scr)                \
	AROS_LC2NR(void,                                      \
			 SetReqBackFill,                           \
			 AROS_LCA(struct Hook *, (___hook), A0),   \
			 AROS_LCA(struct Screen **, (___scr), A1), \
			 struct Library *,                         \
			 DOPUS_BASE_NAME,                          \
			 354,                                      \
			 /* s */)

#define SetRexxVarEx(___msg, ___varname, ___value, ___length) \
	AROS_LC4(long,                                            \
			 SetRexxVarEx,                                    \
			 AROS_LCA(struct RexxMsg *, (___msg), A0),        \
			 AROS_LCA(char *, (___varname), A1),              \
			 AROS_LCA(char *, (___value), D0),                \
			 AROS_LCA(long, (___length), D1),                 \
			 struct Library *,                                \
			 DOPUS_BASE_NAME,                                 \
			 337,                                             \
			 /* s */)

#define SetStatusText(___window, ___text)                \
	AROS_LC2NR(void,                                        \
			 SetStatusText,                              \
			 AROS_LCA(struct Window *, (___window), A0), \
			 AROS_LCA(char *, (___text), A1),            \
			 struct Library *,                           \
			 DOPUS_BASE_NAME,                            \
			 156,                                        \
			 /* s */)

#define SetWBArg(___msg, ___num, ___lock, ___name, ___mem) \
	AROS_LC5(BOOL,                                         \
			 SetWBArg,                                     \
			 AROS_LCA(DOpusAppMessage *, (___msg), A0),    \
			 AROS_LCA(short, (___num), D0),                \
			 AROS_LCA(BPTR, (___lock), D1),                \
			 AROS_LCA(char *, (___name), A1),              \
			 AROS_LCA(APTR, (___mem), A2),                 \
			 struct Library *,                             \
			 DOPUS_BASE_NAME,                              \
			 316,                                          \
			 /* s */)

#define SetWindowBusy(___window)                                                                                     \
	AROS_LC1NR(void,  SetWindowBusy, AROS_LCA(struct Window *, (___window), A0), struct Library *, DOPUS_BASE_NAME, 65, \
			 /* s */)

#define SetWindowID(___window, ___id_ptr, ___id, ___port) \
	AROS_LC4NR(void,                                         \
			 SetWindowID,                                 \
			 AROS_LCA(struct Window *, (___window), A0),  \
			 AROS_LCA(WindowID *, (___id_ptr), A1),       \
			 AROS_LCA(ULONG, (___id), D0),                \
			 AROS_LCA(struct MsgPort *, (___port), A2),   \
			 struct Library *,                            \
			 DOPUS_BASE_NAME,                             \
			 70,                                          \
			 /* s */)

#define ShowDragImage(___draginfo, ___x, ___y)        \
	AROS_LC3NR(void,                                     \
			 ShowDragImage,                           \
			 AROS_LCA(DragInfo *, (___draginfo), A0), \
			 AROS_LCA(ULONG, (___x), D0),             \
			 AROS_LCA(ULONG, (___y), D1),             \
			 struct Library *,                        \
			 DOPUS_BASE_NAME,                         \
			 33,                                      \
			 /* s */)

#define ShowProgressBar(___window, ___object, ___total, ___count) \
	AROS_LC4NR(void,                                                 \
			 ShowProgressBar,                                     \
			 AROS_LCA(struct Window *, (___window), A0),          \
			 AROS_LCA(GL_Object *, (___object), A1),              \
			 AROS_LCA(ULONG, (___total), D0),                     \
			 AROS_LCA(ULONG, (___count), D1),                     \
			 struct Library *,                                    \
			 DOPUS_BASE_NAME,                                     \
			 69,                                                  \
			 /* s */)

#define ShowProgressWindow(___prog, ___scr, ___win)   \
	AROS_LC3NR(void,                                     \
			 ShowProgressWindow,                      \
			 AROS_LCA(APTR, (___prog), A0),           \
			 AROS_LCA(struct Screen *, (___scr), A1), \
			 AROS_LCA(struct Window *, (___win), A2), \
			 struct Library *,                        \
			 DOPUS_BASE_NAME,                         \
			 274,                                     \
			 /* s */)

#define ShowSemaphore(___sem)                                                                                          \
	AROS_LC1NR(void,                                                                                                           \
		 ShowSemaphore, AROS_LCA(struct SignalSemaphore *, (___sem), A0), struct Library *, DOPUS_BASE_NAME, 225, \
		/* s */)

#define SimpleRequest(___parent, ___title, ___buttons, ___message, ___buffer, ___data, ___bufsize, ___flags) \
	AROS_LC8(short,                                                                                          \
			 SimpleRequest,                                                                                  \
			 AROS_LCA(struct Window *, (___parent), A0),                                                     \
			 AROS_LCA(char *, (___title), A1),                                                               \
			 AROS_LCA(char *, (___buttons), A2),                                                             \
			 AROS_LCA(char *, (___message), A3),                                                             \
			 AROS_LCA(char *, (___buffer), A4),                                                              \
			 AROS_LCA(APTR, (___data), A5),                                                                  \
			 AROS_LCA(long, (___bufsize), D0),                                                               \
			 AROS_LCA(ULONG, (___flags), D1),                                                                \
			 struct Library *,                                                                               \
			 DOPUS_BASE_NAME,                                                                                \
			 26,                                                                                             \
			 /* s */)

#ifndef NO_INLINE_STDARG
	#define SimpleRequestTags(window, title, buttons, message, ...)                 \
		({                                                                          \
			IPTR __args[] = {AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__)};              \
			SimpleRequest(window, title, buttons, message, 0, (APTR)&__args, 0, 0); \
		})
#endif /* !NO_INLINE_STDARG */

#define StampDragImage(___draginfo, ___x, ___y)       \
	AROS_LC3NR(void,                                     \
			 StampDragImage,                          \
			 AROS_LCA(DragInfo *, (___draginfo), A0), \
			 AROS_LCA(ULONG, (___x), D0),             \
			 AROS_LCA(ULONG, (___y), D1),             \
			 struct Library *,                        \
			 DOPUS_BASE_NAME,                         \
			 35,                                      \
			 /* s */)

#define StartRefreshConfigWindow(___win, ___state)    \
	AROS_LC2NR(void,                                     \
			 StartRefreshConfigWindow,                \
			 AROS_LCA(struct Window *, (___win), A0), \
			 AROS_LCA(long, (___state), D0),          \
			 struct Library *,                        \
			 DOPUS_BASE_NAME,                         \
			 301,                                     \
			 /* s */)

#define StartTimer(___handle, ___secs, ___micro)       \
	AROS_LC3NR(void,                                      \
			 StartTimer,                               \
			 AROS_LCA(TimerHandle *, (___handle), A0), \
			 AROS_LCA(ULONG, (___secs), D0),           \
			 AROS_LCA(ULONG, (___micro), D1),          \
			 struct Library *,                         \
			 DOPUS_BASE_NAME,                          \
			 20,                                       \
			 /* s */)

#define StopTimer(___handle)                                                                                        \
	AROS_LC1NR(void,  StopTimer, AROS_LCA(TimerHandle *, (___handle), A0), struct Library *, DOPUS_BASE_NAME, 22, /* s \
																												*/)

#define StoreGadgetValue(___list, ___msg)                   \
	AROS_LC2NR(void,                                           \
			 StoreGadgetValue,                              \
			 AROS_LCA(ObjectList *, (___list), A0),         \
			 AROS_LCA(struct IntuiMessage *, (___msg), A1), \
			 struct Library *,                              \
			 DOPUS_BASE_NAME,                               \
			 53,                                            \
			 /* s */)

#define StoreWindowDims(___window, ___dims)               \
	AROS_LC2NR(void,                                         \
			 StoreWindowDims,                             \
			 AROS_LCA(struct Window *, (___window), A0),  \
			 AROS_LCA(WindowDimensions *, (___dims), A1), \
			 struct Library *,                            \
			 DOPUS_BASE_NAME,                             \
			 238,                                         \
			 /* s */)

#define StrCombine(___s1, ___s2, ___s3, ___len) \
	AROS_LC4(BOOL,                              \
			 StrCombine,                        \
			 AROS_LCA(char *, (___s1), A0),     \
			 AROS_LCA(char *, (___s2), A1),     \
			 AROS_LCA(char *, (___s3), A2),     \
			 AROS_LCA(int, (___len), D0),       \
			 struct Library *,                  \
			 DOPUS_BASE_NAME,                   \
			 190,                               \
			 /* s */)

#define StrConcat(___s1, ___s2, ___len)     \
	AROS_LC3(BOOL,                          \
			 StrConcat,                     \
			 AROS_LCA(char *, (___s1), A0), \
			 AROS_LCA(char *, (___s2), A1), \
			 AROS_LCA(int, (___len), D0),   \
			 struct Library *,              \
			 DOPUS_BASE_NAME,               \
			 191,                           \
			 /* s */)

#define StripIntuiMessagesDopus(___window)               \
	AROS_LC1NR(void,                                        \
			 StripIntuiMessagesDopus,                    \
			 AROS_LCA(struct Window *, (___window), A0), \
			 struct Library *,                           \
			 DOPUS_BASE_NAME,                            \
			 44,                                         \
			 /* s */)

#define StripWindowMessages(___port, ___except)                \
	AROS_LC2NR(void,                                              \
			 StripWindowMessages,                              \
			 AROS_LCA(struct MsgPort *, (___port), A0),        \
			 AROS_LCA(struct IntuiMessage *, (___except), A1), \
			 struct Library *,                                 \
			 DOPUS_BASE_NAME,                                  \
			 363,                                              \
			 /* s */)

#define SwapListNodes(___list, ___s1, ___s2)         \
	AROS_LC3NR(void,                                    \
			 SwapListNodes,                          \
			 AROS_LCA(struct List *, (___list), A0), \
			 AROS_LCA(struct Node *, (___s1), A1),   \
			 AROS_LCA(struct Node *, (___s2), A2),   \
			 struct Library *,                       \
			 DOPUS_BASE_NAME,                        \
			 257,                                    \
			 /* s */)

#define TimerActive(___timer)                                                                                    \
	AROS_LC1(BOOL, TimerActive, AROS_LCA(TimerHandle *, (___timer), A0), struct Library *, DOPUS_BASE_NAME, 198, \
			 /* s */)

#define UnlockAppList() AROS_LC0NR(void,  UnlockAppList, struct Library *, DOPUS_BASE_NAME, 186, /* s */)

#define UnlockAttList(___list)                                                                                      \
	AROS_LC1NR(void,  UnlockAttList, AROS_LCA(Att_List *, (___list), A0), struct Library *, DOPUS_BASE_NAME, 215, /* s \
																												*/)

#define UnlockReqBackFill() AROS_LC0NR(void,  UnlockReqBackFill, struct Library *, DOPUS_BASE_NAME, 356, /* s */)

#define UpdateEnvironment(___env)                                                                                 \
	AROS_LC1NR(void,  UpdateEnvironment, AROS_LCA(CFG_ENVR *, (___env), A0), struct Library *, DOPUS_BASE_NAME, 366, \
			 /* s */)

#define UpdateGadgetList(___list)                                                                                   \
	AROS_LC1NR(void,  UpdateGadgetList, AROS_LCA(ObjectList *, (___list), A0), struct Library *, DOPUS_BASE_NAME, 305, \
			 /* s */)

#define UpdateGadgetValue(___list, ___msg, ___id)           \
	AROS_LC3NR(void,                                           \
			 UpdateGadgetValue,                             \
			 AROS_LCA(ObjectList *, (___list), A0),         \
			 AROS_LCA(struct IntuiMessage *, (___msg), A1), \
			 AROS_LCA(UWORD, (___id), D0),                  \
			 struct Library *,                              \
			 DOPUS_BASE_NAME,                               \
			 304,                                           \
			 /* s */)

#define UpdateMyPaths() AROS_LC0NR(void,  UpdateMyPaths, struct Library *, DOPUS_BASE_NAME, 360, /* s */)

#define UpdatePathList() AROS_LC0NR(void,  UpdatePathList, struct Library *, DOPUS_BASE_NAME, 359, /* s */)

#define UpdateStatusGraph(___window, ___text, ___total, ___count) \
	AROS_LC4NR(void,                                                 \
			 UpdateStatusGraph,                                   \
			 AROS_LCA(struct Window *, (___window), A0),          \
			 AROS_LCA(char *, (___text), A1),                     \
			 AROS_LCA(ULONG, (___total), D0),                     \
			 AROS_LCA(ULONG, (___count), D1),                     \
			 struct Library *,                                    \
			 DOPUS_BASE_NAME,                                     \
			 157,                                                 \
			 /* s */)

#define WB_AddAppWindow(___id, ___data, ___window, ___port, ___tags) \
	AROS_LC5(struct AppWindow *,                                     \
			 WB_AddAppWindow,                                        \
			 AROS_LCA(IPTR, (___id), D0),                            \
			 AROS_LCA(IPTR, (___data), D1),                          \
			 AROS_LCA(struct Window *, (___window), A0),             \
			 AROS_LCA(struct MsgPort *, (___port), A1),              \
			 AROS_LCA(struct TagItem *, (___tags), A2),              \
			 struct Library *,                                       \
			 DOPUS_BASE_NAME,                                        \
			 179,                                                    \
			 /* s */)

#ifndef NO_INLINE_STDARG
	#define WB_AddAppWindowTags(a0, a1, a2, a3, ...)                          \
		({                                                                    \
			IPTR _tags[] = {AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__)};         \
			WB_AddAppWindow((a0), (a1), (a2), (a3), (struct TagItem *)_tags); \
		})
#endif /* !NO_INLINE_STDARG */

#define WB_AppIconFlags(___icon)                                                                                   \
	AROS_LC1(                                                                                                      \
		ULONG, WB_AppIconFlags, AROS_LCA(struct AppIcon *, (___icon), A0), struct Library *, DOPUS_BASE_NAME, 244, \
		/* s */)

#define WB_AppWindowData(___window, ___id, ___userdata)     \
	AROS_LC3(struct MsgPort *,                              \
			 WB_AppWindowData,                              \
			 AROS_LCA(struct AppWindow *, (___window), A0), \
			 AROS_LCA(IPTR *, (___id), A1),                 \
			 AROS_LCA(IPTR *, (___userdata), A2),           \
			 struct Library *,                              \
			 DOPUS_BASE_NAME,                               \
			 182,                                           \
			 /* s */)

#define WB_AppWindowLocal(___window)                        \
	AROS_LC1(BOOL,                                          \
			 WB_AppWindowLocal,                             \
			 AROS_LCA(struct AppWindow *, (___window), A0), \
			 struct Library *,                              \
			 DOPUS_BASE_NAME,                               \
			 183,                                           \
			 /* s */)

#define WB_AppWindowWindow(___appwindow)                       \
	AROS_LC1(struct Window *,                                  \
			 WB_AppWindowWindow,                               \
			 AROS_LCA(struct AppWindow *, (___appwindow), A0), \
			 struct Library *,                                 \
			 DOPUS_BASE_NAME,                                  \
			 372,                                              \
			 /* s */)

#define WB_FindAppWindow(___window)                      \
	AROS_LC1(struct AppWindow *,                         \
			 WB_FindAppWindow,                           \
			 AROS_LCA(struct Window *, (___window), A0), \
			 struct Library *,                           \
			 DOPUS_BASE_NAME,                            \
			 181,                                        \
			 /* s */)

#define WB_Install_Patch() AROS_LC0NR(void,  WB_Install_Patch, struct Library *, DOPUS_BASE_NAME, 177, /* s */)

#define WB_Launch(___name, ___screen, ___wait)           \
	AROS_LC3(BOOL,                                       \
			 WB_Launch,                                  \
			 AROS_LCA(char *, (___name), A0),            \
			 AROS_LCA(struct Screen *, (___screen), A1), \
			 AROS_LCA(short, (___wait), D0),             \
			 struct Library *,                           \
			 DOPUS_BASE_NAME,                            \
			 192,                                        \
			 /* s */)

#define WB_LaunchNew(___name, ___scr, ___wait, ___stack, ___tool) \
	AROS_LC5(BOOL,                                                \
			 WB_LaunchNew,                                        \
			 AROS_LCA(char *, (___name), A0),                     \
			 AROS_LCA(struct Screen *, (___scr), A1),             \
			 AROS_LCA(short, (___wait), D0),                      \
			 AROS_LCA(long, (___stack), D1),                      \
			 AROS_LCA(char *, (___tool), A2),                     \
			 struct Library *,                                    \
			 DOPUS_BASE_NAME,                                     \
			 358,                                                 \
			 /* s */)

#define WB_LaunchNotify(___name, ___scr, ___wait, ___stack, ___tool, ___proc, ___notify, ___flags) \
	AROS_LC8(BOOL,                                                                                 \
			 WB_LaunchNotify,                                                                      \
			 AROS_LCA(char *, (___name), A0),                                                      \
			 AROS_LCA(struct Screen *, (___scr), A1),                                              \
			 AROS_LCA(short, (___wait), D0),                                                       \
			 AROS_LCA(long, (___stack), D1),                                                       \
			 AROS_LCA(char *, (___tool), A2),                                                      \
			 AROS_LCA(struct Process **, (___proc), A3),                                           \
			 AROS_LCA(IPCData *, (___notify), A4),                                                 \
			 AROS_LCA(ULONG, (___flags), D2),                                                      \
			 struct Library *,                                                                     \
			 DOPUS_BASE_NAME,                                                                      \
			 371,                                                                                  \
			 /* s */)

#define WB_RemoveAppWindow(___window)                       \
	AROS_LC1(BOOL,                                          \
			 WB_RemoveAppWindow,                            \
			 AROS_LCA(struct AppWindow *, (___window), A0), \
			 struct Library *,                              \
			 DOPUS_BASE_NAME,                               \
			 180,                                           \
			 /* s */)

#define WB_Remove_Patch() AROS_LC0(BOOL, WB_Remove_Patch, struct Library *, DOPUS_BASE_NAME, 178, /* s */)

#define WriteBuf(___file, ___data, ___size)   \
	AROS_LC3(long,                            \
			 WriteBuf,                        \
			 AROS_LCA(APTR, (___file), A0),   \
			 AROS_LCA(char *, (___data), A1), \
			 AROS_LCA(long, (___size), D0),   \
			 struct Library *,                \
			 DOPUS_BASE_NAME,                 \
			 167,                             \
			 /* s */)

#define WriteClipString(___clip, ___string, ___len)        \
	AROS_LC3(BOOL,                                         \
			 WriteClipString,                              \
			 AROS_LCA(struct ClipHandle *, (___clip), A0), \
			 AROS_LCA(char *, (___string), A1),            \
			 AROS_LCA(long, (___len), D0),                 \
			 struct Library *,                             \
			 DOPUS_BASE_NAME,                              \
			 212,                                          \
			 /* s */)

#define WriteFileIcon(___source, ___dest)       \
	AROS_LC2NR(void,                               \
			 WriteFileIcon,                     \
			 AROS_LCA(char *, (___source), A0), \
			 AROS_LCA(char *, (___dest), A1),   \
			 struct Library *,                  \
			 DOPUS_BASE_NAME,                   \
			 29,                                \
			 /* s */)

#define WriteIcon(___name, ___obj)                        \
	AROS_LC2(BOOL,                                        \
			 WriteIcon,                                   \
			 AROS_LCA(char *, (___name), A0),             \
			 AROS_LCA(struct DiskObject *, (___obj), A1), \
			 struct Library *,                            \
			 DOPUS_BASE_NAME,                             \
			 28,                                          \
			 /* s */)

#define ExamineHandle64(___fh, ___fib)                  \
	AROS_LC2(BOOL,                                      \
			 ExamineHandle64,                           \
			 AROS_LCA(BPTR, (___fh), D0),               \
			 AROS_LCA(FileInfoBlock64 *, (___fib), A0), \
			 struct Library *,                          \
			 DOPUS_BASE_NAME,                           \
			 391,                                       \
			 /* s */)

#define ExamineLock64(___lock, ___fib)                  \
	AROS_LC2(BOOL,                                      \
			 ExamineLock64,                             \
			 AROS_LCA(BPTR, (___lock), D0),             \
			 AROS_LCA(FileInfoBlock64 *, (___fib), A0), \
			 struct Library *,                          \
			 DOPUS_BASE_NAME,                           \
			 389,                                       \
			 /* s */)

#define ExamineNext64(___lock, ___fib)                  \
	AROS_LC2(BOOL,                                      \
			 ExamineNext64,                             \
			 AROS_LCA(BPTR, (___lock), D0),             \
			 AROS_LCA(FileInfoBlock64 *, (___fib), A0), \
			 struct Library *,                          \
			 DOPUS_BASE_NAME,                           \
			 390,                                       \
			 /* s */)

#define MatchFirst64(___pat, ___panchor)                      \
	AROS_LC2(LONG,                                            \
			 MatchFirst64,                                    \
			 AROS_LCA(STRPTR, (___pat), A0),                  \
			 AROS_LCA(struct AnchorPath *, (___panchor), A1), \
			 struct Library *,                                \
			 DOPUS_BASE_NAME,                                 \
			 392,                                             \
			 /* s */)

#define MatchNext64(___panchor)                                                                                     \
	AROS_LC1(                                                                                                       \
		LONG, MatchNext64, AROS_LCA(struct AnchorPath *, (___panchor), A0), struct Library *, DOPUS_BASE_NAME, 393, \
		/* s */)

#endif /* !_INLINE_DOPUS_H */
