/***************************************************************************

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

Directory Opus 5 project: https://github.com/BlitterStudio/dopus5

For more information on Directory Opus for Windows please see:

				 http://www.gpsoft.com.au

***************************************************************************/

#include <proto/locale.h>
#include <exec/resident.h>

#include "dopuslib.h"
#include <dopus/lib_macros.h>
#include "base.h"
#include "lib_protos.h"

#include "boopsi.h"

/****************************************************************************/

/*
 * The system (and compiler) rely on a symbol named _start which marks
 * the beginning of execution of an ELF file. To prevent others from
 * executing this library, and to keep the compiler/linker happy, we
 * define an empty _start symbol here.
 *
 * On the classic system (pre-AmigaOS4) this was usually done by
 * moveq #0,d0
 * rts
 *
 * On MorphOS we just mark library as non-executable.
 */

#if defined(__amigaos3__) || defined(__amigaos4__)
USED int _start(void)  // must be first for 68k library
{
	return RETURN_FAIL;
}

#if defined(__amigaos3__)
__stdargs void exit(int status)
{
	Exit(status);
}
#endif
#endif

#if defined(__MORPHOS__)
void exit(int status) __attribute__((noreturn));
void exit(int status)
{
	Exit(status);
	__builtin_unreachable();
}
#endif

#ifdef FBASEREL
void __restore_a4(void)
{
	__asm volatile("\tlea ___a4_init, a4");
}
#endif


/****************************************************************************/

#define MIN_STACKSIZE 512

#if defined(__amigaos4__)
// stack cookie for shell v45+
static const char USED_VAR stack_size[] = "$STACK:" STR(MIN_STACKSIZE) "\n";
#endif
static const char USED_VAR copyright[] = COPYRIGHT;

/****************************************************************************/

int UserLibInit(REG(a6, struct MyLibrary *libbase));
void UserLibCleanup(REG(a6, struct MyLibrary *libbase));
#ifdef __amigaos4__
int low_mem_handler(struct ExecBase *ExecBase, struct MemHandlerData *memh, struct LibData *data);
#else
int low_mem_handler(REG(a0, struct MemHandlerData *), REG(a1, struct LibData *));
#endif

#if defined(__MORPHOS__)
static int low_mem_handler_entry(void)
{
	return low_mem_handler((APTR)REG_A0, (APTR)REG_A1);
}
#endif

#if defined(__amigaos4__)
struct Library *SysBase = NULL;
struct ExecIFace *IExec = NULL;
	#if defined(__NEWLIB__)
struct Library *NewlibBase = NULL;
struct NewlibIFace *INewlib = NULL;
	#endif
#else
struct ExecBase *SysBase = NULL;
#endif

#ifdef __AROS__
	#include <aros/symbolsets.h>
THIS_PROGRAM_HANDLES_SYMBOLSET(PROGRAM_ENTRIES)
THIS_PROGRAM_HANDLES_SYMBOLSET(INIT)
THIS_PROGRAM_HANDLES_SYMBOLSET(EXIT)
THIS_PROGRAM_HANDLES_SYMBOLSET(LIBS)
DEFINESET(INIT)
DEFINESET(EXIT)
DEFINESET(LIBS)
#endif

/* reorganize it to match necessary declarations for MORPHOS and AROS */

#if defined(__amigaos4__)

struct Library *DOSBase = NULL;
struct Library *GfxBase = NULL;
struct Library *IntuitionBase = NULL;
struct Library *LocaleBase = NULL;
struct Library *RexxSysBase = NULL;

struct DOSIFace *IDOS = NULL;
struct UtilityIFace *IUtility = NULL;
struct LocaleIFace *ILocale = NULL;
// struct ConsoleIFace 	*IConsole = NULL;
struct GraphicsIFace *IGraphics = NULL;
struct P96IFace *IP96 = NULL;
struct CyberGfxIFace *ICyberGfx = NULL;
struct IntuitionIFace *IIntuition = NULL;
struct GadToolsIFace *IGadTools = NULL;
struct AslIFace *IAsl = NULL;
struct LayersIFace *ILayers = NULL;
struct DiskFontIFace *IDiskFont = NULL;
struct IconIFace *IIcon = NULL;
struct WorkbenchIFace *IWorkbench = NULL;
struct DataTypesIFace *IDataTypes = NULL;
struct RexxSysIFace *IRexxSys = NULL;
struct NewIconIFace *INewIcon = NULL;

	#if !defined(__NEWLIB__)
struct UtilityIFace *__IUtility = NULL;	 // clib2
	#endif
// struct ModuleIFace		*IModule = NULL;
// struct MusicIFace		*IMUSIC = NULL;

#elif defined(__MORPHOS__)
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct DosLibrary *DOSBase;
struct Library *LocaleBase;
struct Library *RexxSysBase;
#else

struct IntuitionBase *IntuitionBase = NULL;
struct GfxBase *GfxBase = NULL;
struct DosLibrary *DOSBase = NULL;
struct LocaleBase *LocaleBase = NULL;
struct RxsLib *RexxSysBase = NULL;
#endif

struct Library *P96Base = NULL;
/* cybergraphics.library kept as a fallback for systems without Picasso96
 * (older OS3 setups, PPC accelerators with CyberStorm+CGX, etc.). Opened
 * below only if resident; read_ilbm.c prefers P96 and falls back to this. */
struct Library *CyberGfxBase = NULL;
struct Library *GadToolsBase = NULL;
struct Library *AslBase = NULL;
struct Library *LayersBase = NULL;
struct Library *DiskFontBase = NULL;
struct Library *IconBase = NULL;
struct Library *WorkbenchBase = NULL;
struct Library *DataTypesBase = NULL;
struct Library *NewIconBase = NULL;

/*#if defined(__MORPHOS__)
struct Library  			*ConsoleDevice = NULL;
#else
struct Device  			*ConsoleDevice = NULL;
#endif*/

#ifdef __AROS__
struct UtilityBase *UtilityBase = NULL;
#elif defined(__libnix__)
extern struct Library *UtilityBase;
#elif defined(__amigaos4__)
struct Library *UtilityBase = NULL;
#else
struct UtilityBase *UtilityBase = NULL;
extern struct Library __UtilityBase = NULL;	 // required by clib2
#endif

/****************************************************************************/

#if defined(__MORPHOS__)
extern void L_WB_AddAppWindow(void);
extern void L_WB_RemoveAppWindow(void);
	#define libstub_L_WB_AddAppWindow L_WB_AddAppWindow
	#define libstub_L_WB_RemoveAppWindow L_WB_RemoveAppWindow
#endif

/**************************************************************************/
char *_ProgramName = "dopus5.library";

void init_locale_data(struct DOpusLocale *locale);
void free_locale_data(struct DOpusLocale *locale);

static struct TextAttr topaz_attr = {"topaz.font", 8, 0, 0};

char *version = "\0$VER: dopus5.library " LIB_STRING;

static const char UserLibName[] = "dopus5.library";
static const char __TEXTSEGMENT__ UserLibID[] = "\0$VER: dopus5.library " LIB_STRING;
/**************************************************************************/

// first function should be FAS (without _), or at least LibNull if it removed function (like in our case it is
// RemovedFunc() ).

#define libvector                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
	LibNull LFUNC_FA_(L_RandomDopus) LFUNC_FA_(L_Atoh) LFUNC_FA_(L_BtoCStr) LFUNC_FA_(L_DivideU) LFUNC_FA_(L_Itoa) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
		L_ItoaU) LFUNC_FA_(L_Ito26) LFUNC_FA_(L_BytesToString) LFUNC_FA_(L_DivideToString) LFUNC_FA_(L_SetBusyPointer)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
		LFUNC_FA_(L_BuildKeyString) LFUNC_FA_(L_ActivateStrGad) LFUNC_FA_(L_AllocTimer) LFUNC_FA_(L_FreeTimer) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
			L_StartTimer) LFUNC_FA_(L_CheckTimer) LFUNC_FA_(L_StopTimer) LFUNC_FA_(L_GetDosPathList) LFUNC_FA_(L_FreeDosPathList)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
			LFUNC_FA_(L_DoSimpleRequest) LFUNC_FA_(L_SimpleRequest) LFUNC_FA_(L_SelectionList) LFUNC_FA_(L_WriteIcon) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
				L_WriteFileIcon) LFUNC_FA_(L_GetDragInfo) LFUNC_FA_(L_FreeDragInfo) LFUNC_FA_(L_GetDragImage)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
				LFUNC_FA_(L_ShowDragImage) LFUNC_FA_(L_HideDragImage) LFUNC_FA_(L_StampDragImage) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 \
					L_GetDragMask) LFUNC_FA_(L_CheckDragDeadlock) LFUNC_FA_(L_AddDragImage) LFUNC_FA_(L_RemDragImage)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
					LFUNC_FA_(L_OpenConfigWindow) LFUNC_FA_(L_CloseConfigWindow) LFUNC_FA_(L_GetWindowMsg) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
						L_ReplyWindowMsg) LFUNC_FA_(L_StripIntuiMessagesDopus) LFUNC_FA_(L_CloseWindowSafely)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
						LFUNC_FA_(L_CalcObjectDims) LFUNC_FA_(L_CalcWindowDims) LFUNC_FA_(L_AddObjectList) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
							L_FreeObject) LFUNC_FA_(L_FreeObjectList) LFUNC_FA_(L_RefreshObjectList) LFUNC_FA_(L_GetObject)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  \
							LFUNC_FA_(L_StoreGadgetValue) LFUNC_FA_(L_SetGadgetValue) LFUNC_FA_(L_GetGadgetValue) LFUNC_FA_(L_SetObjectKind) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
								L_SetGadgetChoices) LFUNC_FA_(L_CheckObjectArea) LFUNC_FA_(L_GetObjectRect) LFUNC_FA_(L_DisplayObject)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
								LFUNC_FA_(L_DisableObject) LFUNC_FA_(L_BoundsCheckGadget) LFUNC_FA_(L_AddWindowMenus) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
									L_FreeWindowMenus) LFUNC_FA_(L_SetWindowBusy) LFUNC_FA_(L_ClearWindowBusy)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
									LFUNC_FA_(L_GetString) LFUNC_FA_(L_FindKeyEquivalent) LFUNC_FA_(L_ShowProgressBar) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
										L_SetWindowID) LFUNC_FA_(L_GetWindowID) LFUNC_FA_(L_GetWindowAppPort) LFUNC_FA_(L_Att_NewList)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
										LFUNC_FA_(L_Att_NewNode) LFUNC_FA_(L_Att_RemNode) LFUNC_FA_(L_Att_PosNode) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
											L_Att_RemList) LFUNC_FA_(L_Att_FindNode) LFUNC_FA_(L_Att_NodeNumber)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
											LFUNC_FA_(L_Att_FindNodeData) LFUNC_FA_(L_Att_NodeDataNumber) LFUNC_FA_(L_Att_NodeName) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
												L_Att_NodeCount) LFUNC_FA_(L_Att_ChangeNodeName) LFUNC_FA_(L_Att_FindNodeNumber)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
												LFUNC_FA_(L_AddSorted) LFUNC_FA_(L_BuildMenuStrip) LFUNC_FA_(L_FindMenuItem) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
													L_DoPopUpMenu) LFUNC_FA_(L_GetPopUpItem) LFUNC_FA_(L_IPC_Startup) LFUNC_FA_(L_IPC_Command)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
													LFUNC_FA_(L_IPC_Reply) LFUNC_FA_(L_IPC_Free) LFUNC_FA_(L_IPC_FindProc) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
														L_IPC_Quit) LFUNC_FA_(L_IPC_Hello) LFUNC_FA_(L_IPC_Goodbye) LFUNC_FA_(L_IPC_GetGoodbye)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
														LFUNC_FA_(L_IPC_ListQuit) LFUNC_FA_(L_IPC_Flush) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
															L_IPC_ListCommand) LFUNC_FA_(L_IPC_ProcStartup) LFUNC_FA_(L_IPC_Launch)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
															LFUNC_FA_(L_OpenImage) LFUNC_FA_(L_CloseImage) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
																L_CopyImage) LFUNC_FA_(L_FlushImages) LFUNC_FA_(L_RenderImage)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
																LFUNC_FA_(L_GetImageAttrs) LFUNC_FA_(L_NewMemHandle) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
																	L_FreeMemHandle) LFUNC_FA_(L_ClearMemHandle)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
																	LFUNC_FA_(L_AllocMemH) LFUNC_FA_(L_FreeMemH) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  \
																		L_DrawBox) LFUNC_FA_(L_DrawFieldBox) LFUNC_FA_(L_NewLister)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
																		LFUNC_FA_(L_NewButtonBank) LFUNC_FA_(L_NewButton) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
																			L_NewFunction) LFUNC_FA_(L_NewInstruction)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
																			LFUNC_FA_(L_ReadSettings) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
																				L_ReadListerDef) LFUNC_FA_(L_OpenButtonBank)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 \
																				LFUNC_FA_(L_DefaultSettings) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
																					L_DefaultEnvironment) LFUNC_FA_(L_DefaultButtonBank)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
																					LFUNC_FA_(L_SaveSettings) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
																						L_SaveListerDef) LFUNC_FA_(L_SaveButtonBank)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
																						LFUNC_FA_(L_CloseButtonBank) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
																							L_FreeListerDef) LFUNC_FA_(L_FreeButtonList)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
																							LFUNC_FA_(L_FreeButtonImages) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
																								L_FreeButton)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
																								LFUNC_FA_(L_FreeFunction) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
																									L_FreeInstruction)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
																									LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
																										L_FreeInstructionList) LFUNC_FA_(L_CopyButtonBank) LFUNC_FA_(L_CopyButton) LFUNC_FA_(L_CopyFunction) LFUNC_FA_(L_NewFiletype) LFUNC_FA_(L_ReadFiletypes) LFUNC_FA_(L_SaveFiletypeList) LFUNC_FA_(L_FreeFiletypeList) LFUNC_FA_(L_FreeFiletype) LFUNC_FA_(L_CopyFiletype) LFUNC_FA_(L_FindFunctionType) LFUNC_FA_(L_SaveButton) LFUNC_FA_(L_ReadButton) LFUNC_FA_(L_ReadFunction) LFUNC_FA_(L_OpenIFFFile) LFUNC_FA_(L_CloseIFFFile) LFUNC_FA_(L_OpenStatusWindow) LFUNC_FA_(L_SetStatusText) LFUNC_FA_(L_UpdateStatusGraph) LFUNC_FA_(L_ReadILBM) LFUNC_FA_(L_FreeILBM) LFUNC_FA_(L_DecodeILBM) LFUNC_FA_(L_DecodeRLE) LFUNC_FA_(L_LoadPalette32) LFUNC_FA_(L_GetPalette32) LFUNC_FA_(L_OpenBuf) LFUNC_FA_(L_CloseBuf) LFUNC_FA_(L_ReadBuf) LFUNC_FA_(L_WriteBuf) LFUNC_FA_(L_FlushBuf) LFUNC_FA_(L_SeekBuf) LFUNC_FA_(L_ExamineBuf) \
																										LFUNC_FA_(L_OpenDisk) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
																											L_CloseDisk) LFUNC_FA_(L_AddScrollBars)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
																											LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
																												L_FindBOOPSIGadget)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
																												LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
																													L_BOOPSIFree)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
																													LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
																														L_SerialValid)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
																														LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           \
																															L_WB_Install_Patch) LFUNC_FA_(L_WB_Remove_Patch) LFUNC_FA_(L_WB_AddAppWindow) LFUNC_FA_(L_WB_RemoveAppWindow) LFUNC_FA_(L_WB_FindAppWindow) LFUNC_FA_(L_WB_AppWindowData) LFUNC_FA_(L_WB_AppWindowLocal) LFUNC_FA_(L_LockAppList) LFUNC_FA_(L_NextAppEntry) LFUNC_FA_(L_UnlockAppList) LFUNC_FA_(L_AddNotifyRequest) LFUNC_FA_(L_RemoveNotifyRequest) LFUNC_FA_(L_SendNotifyMsg) LFUNC_FA_(L_StrCombine) LFUNC_FA_(L_StrConcat) LFUNC_FA_(L_WB_Launch) LFUNC_FA_(L_CopyLocalEnv) LFUNC_FA_(L_CLI_Launch) LFUNC_FA_(L_SerialCheck) LFUNC_FA_(L_ChecksumFile) LFUNC_FA_(L_ReplyFreeMsg) LFUNC_FA_(L_TimerActive) LFUNC_FA_(L_NewButtonFunction) LFUNC_FA_(L_IFFOpen) LFUNC_FA_(L_IFFClose) LFUNC_FA_(L_IFFPushChunk)                                                                                               \
																															LFUNC_FA_(L_IFFWriteChunkBytes) LFUNC_FA_(L_IFFPopChunk) LFUNC_FA_(L_IFFWriteChunk) LFUNC_FA_(L_FindNameI) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
																																L_AnimDecodeRIFFXor) LFUNC_FA_(L_AnimDecodeRIFFSet) LFUNC_FA_(L_ConvertRawKey)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
																																LFUNC_FA_(L_OpenClipBoard) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
																																	L_CloseClipBoard)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
																																	LFUNC_FA_(L_WriteClipString) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  \
																																		L_ReadClipString)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
																																		LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           \
																																			L_LockAttList)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
																																			LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
																																				L_UnlockAttList),                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
		LibNull, LibNull, LibNull, LibNull, LibNull, LibNull,                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
		LibNull LFUNC_FA_(L_GetSemaphore) LFUNC_FA_(L_FreeSemaphore) LFUNC_FA_(L_ShowSemaphore)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
			LFUNC_FA_(L_SaveFunction) LFUNC_FA_(L_IFFNextChunk) LFUNC_FA_(L_IFFChunkSize)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
				LFUNC_FA_(L_IFFReadChunkBytes) LFUNC_FA_(L_IFFFileHandle) LFUNC_FA_(L_IFFChunkRemain)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
					LFUNC_FA_(L_IFFChunkID) LFUNC_FA_(L_IFFGetFORM) LFUNC_FA_(L_ScreenInfo) LFUNC_FA_(L_GetEditHook)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
						LFUNC_FA_(L_FreeEditHook) LFUNC_FA_(L_InitWindowDims) LFUNC_FA_(L_StoreWindowDims)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
							LFUNC_FA_(L_CheckWindowDims) LFUNC_FA_(L_InitListLock) LFUNC_FA_(L_IPC_QuitName)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 \
								LFUNC_FA_(L_QualValid) LFUNC_FA_(L_FHFromBuf) LFUNC_FA_(L_WB_AppIconFlags)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
									LFUNC_FA_(L_GetWBArgPath),                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
		LibNull LFUNC_FA_(L_DeviceFromLock) LFUNC_FA_(L_DeviceFromHandler) LFUNC_FA_(L_DevNameFromLockDopus)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 \
			LFUNC_FA_(L_GetIconFlags) LFUNC_FA_(L_SetIconFlags) LFUNC_FA_(L_GetIconPosition)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 \
				LFUNC_FA_(L_SetIconPosition) LFUNC_FA_(L_BuildTransDragMask) LFUNC_FA_(L_GetImagePalette)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
					LFUNC_FA_(L_FreeImageRemap) LFUNC_FA_(L_SwapListNodes),                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  \
		LibNull LFUNC_FA_(L_Seed), LibNull, LibNull,                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
		LibNull                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
			LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
				L_CopyDiskObject) LFUNC_FA_(L_FreeDiskObjectCopy) LFUNC_FA_(L_IFFFailure) LFUNC_FA_(L_GetCachedDefDiskObject) LFUNC_FA_(L_FreeCachedDiskObject) LFUNC_FA_(L_GetCachedDiskObject) LFUNC_FA_(L_GetCachedDiskObjectNew) LFUNC_FA_(L_IconCheckSum) LFUNC_FA_(L_OpenProgressWindow) LFUNC_FA_(L_CloseProgressWindow) LFUNC_FA_(L_HideProgressWindow) LFUNC_FA_(L_ShowProgressWindow) LFUNC_FA_(L_SetProgressWindow) LFUNC_FA_(L_GetProgressWindow) LFUNC_FA_(L_SetNotifyRequest) LFUNC_FA_(L_ChangeAppIcon) LFUNC_FA_(L_CheckProgressAbort) LFUNC_FA_(L_GetSecureString) LFUNC_FA_(L_NewButtonWithFunc) LFUNC_FA_(L_FreeButtonFunction) LFUNC_FA_(L_CopyButtonFunction)                                                                                                                                                                                                                                                           \
				LFUNC_FA_(L_FindPubScreen) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
					L_SetAppIconMenuState) LFUNC_FA_(L_SearchFile) LFUNC_FA_(L_ParseDateStrings) LFUNC_FA_(L_DateFromStrings) LFUNC_FA_(L_GetMatchHandle) LFUNC_FA_(L_FreeMatchHandle)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
					LFUNC_FA_(L_MatchFiletype) LFUNC_FA_(L_LayoutResize) LFUNC_FA_(L_GetFileVersion) LFUNC_FA_(L_AsyncRequest) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
						L_CheckRefreshMsg) LFUNC_FA_(L_RemapImage) LFUNC_FA_(L_FreeRemapImage) LFUNC_FA_(L_FreeAppMessage) LFUNC_FA_(L_ReplyAppMessage)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
						LFUNC_FA_(L_SetLibraryFlags) LFUNC_FA_(L_StartRefreshConfigWindow) LFUNC_FA_(L_EndRefreshConfigWindow) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
							L_CompareListFormat) LFUNC_FA_(L_UpdateGadgetValue) LFUNC_FA_(L_UpdateGadgetList) LFUNC_FA_(L_NewBitMap)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
							LFUNC_FA_(L_DisposeBitMap) LFUNC_FA_(L_ParseArgs) LFUNC_FA_(L_DisposeArgs) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
								L_SetConfigWindowLimits) LFUNC_FA_(L_SetEnv) LFUNC_FA_(L_IsListLockEmpty) LFUNC_FA_(L_AllocAppMessage)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
								LFUNC_FA_(L_CheckAppMessage) LFUNC_FA_(L_CopyAppMessage) LFUNC_FA_(L_SetWBArg) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
									L_OriginalCreateDir) LFUNC_FA_(L_OriginalDeleteFile) LFUNC_FA_(L_OriginalSetFileDate) LFUNC_FA_(L_OriginalSetComment)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
									LFUNC_FA_(L_OriginalSetProtection) LFUNC_FA_(L_OriginalRename) LFUNC_FA_(L_OriginalOpen) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
										L_OriginalClose) LFUNC_FA_(L_OriginalWrite) LFUNC_FA_(L_CreateTitleGadget) LFUNC_FA_(L_FindGadgetType)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
										LFUNC_FA_(L_FixTitleGadgets) LFUNC_FA_(L_OriginalRelabel) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 \
											L_FakeILBM) LFUNC_FA_(L_IPC_SafeCommand) LFUNC_FA_(L_ClearFiletypeCache) LFUNC_FA_(L_GetTimerBase)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
											LFUNC_FA_(L_InitDragDBuf) LFUNC_FA_(L_FreeRexxMsgEx) LFUNC_FA_(L_CreateRexxMsgEx) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
												L_SetRexxVarEx) LFUNC_FA_(L_GetRexxVarEx) LFUNC_FA_(L_BuildRexxMsgEx)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
												LFUNC_FA_(L_NotifyDiskChange) LFUNC_FA_(L_GetDosListCopy) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
													L_FreeDosListCopy) LFUNC_FA_(L_DateFromStringsNew) LFUNC_FA_(L_RemapIcon)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
													LFUNC_FA_(L_GetOriginalIcon) LFUNC_FA_(L_CalcPercent) LFUNC_FA_(L_IsDiskDevice) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
														L_DrawDragList) LFUNC_FA_(L_RemoveDragImage) LFUNC_FA_(L_SetNewIconsFlags)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           \
														LFUNC_FA_(L_ReadBufLine) LFUNC_FA_(L_GetLibraryFlags) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
															L_GetIconType) LFUNC_FA_(L_SetReqBackFill) LFUNC_FA_(L_LockReqBackFill)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
															LFUNC_FA_(L_UnlockReqBackFill) LFUNC_FA_(L_DragCustomOk) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
																L_WB_LaunchNew) LFUNC_FA_(L_UpdatePathList) LFUNC_FA_(L_UpdateMyPaths)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
																LFUNC_FA_(L_GetPopUpImageSize) LFUNC_FA_(L_GetDeviceUnit) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
																	L_StripWindowMessages) LFUNC_FA_(L_DeleteIcon)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           \
																	LFUNC_FA_(L_MUFSLogin) LFUNC_FA_(L_UpdateEnvironment) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
																		L_ConvertStartMenu) LFUNC_FA_(L_GetOpusPathList)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
																		LFUNC_FA_(L_GetStatistics) LFUNC_FA_(L_SetPopUpDelay) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
																			L_WB_LaunchNotify) LFUNC_FA_(L_WB_AppWindowWindow)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
																			LFUNC_FA_(L_OpenEnvironment) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
																				L_PopUpNewHandle) LFUNC_FA_(L_PopUpFreeHandle)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
																				LFUNC_FA_(L_PopUpNewItem) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
																					L_PopUpSeparator) LFUNC_FA_(L_PopUpItemSub)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
																					LFUNC_FA_(L_PopUpEndSub) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
																						L_PopUpSetFlags) LFUNC_FA_(L_AddAllocBitmapPatch)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
																						LFUNC_FA_(L_RemAllocBitmapPatch) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
																							L_LoadPos) LFUNC_FA_(L_SavePos)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  \
																							LFUNC_FA_(L_DivideU64) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
																								L_ItoaU64) LFUNC_FA_(L_DivideToString64)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
																								LFUNC_FA_(L_BytesToString64) LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
																									L_ExamineLock64)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         \
																									LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
																										L_ExamineNext64)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
																										LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           \
																											L_ExamineHandle64)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               \
																											LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       \
																												L_MatchFirst64)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              \
																												LFUNC_FA_(                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
																													L_MatchNext64)

/****************************************************************************/

#if defined(__amigaos4__)

static struct MyLibrary *LIBFUNC LibInit(struct MyLibrary *base, BPTR librarySegment, struct ExecIFace *pIExec);
static BPTR LIBFUNC LibExpunge(struct LibraryManagerInterface *Self);
static struct MyLibrary *LIBFUNC LibOpen(struct LibraryManagerInterface *Self, ULONG version);
static BPTR LIBFUNC LibClose(struct LibraryManagerInterface *Self);
static LONG LIBFUNC LibNull(void);

#elif defined(__MORPHOS__)

static struct MyLibrary *LIBFUNC LibInit(struct MyLibrary *base, BPTR librarySegment, struct ExecBase *sb);
static BPTR LIBFUNC LibExpunge(void);
static struct MyLibrary *LIBFUNC LibOpen(void);
static BPTR LIBFUNC LibClose(void);
static LONG LIBFUNC LibNull(void);

#elif defined(__AROS__)

	#include <aros/libcall.h>

	#define DOpus_LibOpen LibOpen
	#define DOpus_LibClose LibClose
	#define DOpus_LibExpunge LibExpunge

static AROS_UFP3(struct MyLibrary *,
				 LibInit,
				 AROS_UFPA(struct MyLibrary *, base, D0),
				 AROS_UFPA(BPTR, librarySegment, A0),
				 AROS_UFPA(struct ExecBase *, sb, A6));
static AROS_LD1(struct MyLibrary *, LibOpen, AROS_LPA(UNUSED ULONG, version, D0), struct MyLibrary *, base, 1, DOpus);
static AROS_LD0(BPTR, LibClose, struct MyLibrary *, base, 2, DOpus);
static AROS_LD1(BPTR,
				LibExpunge,
				AROS_LPA(UNUSED struct MyLibrary *, __extrabase, D0),
				struct MyLibrary *,
				base,
				3,
				DOpus);

#else

static struct MyLibrary *LIBFUNC LibInit(REG(d0, struct MyLibrary *lh),
										 REG(a0, BPTR Segment),
										 REG(a6, struct ExecBase *sb));
static BPTR LIBFUNC LibExpunge(REG(a6, struct MyLibrary *base));
static struct MyLibrary *LIBFUNC LibOpen(REG(d0, ULONG version), REG(a6, struct MyLibrary *base));
static BPTR LIBFUNC LibClose(REG(a6, struct MyLibrary *base));
static LONG LIBFUNC LibNull(void);
struct WBenchMsg *_WBenchMsg = NULL;
#endif

/****************************************************************************/

static LONG LIBFUNC LibNull(VOID)
{
	return (0);
}

/****************************************************************************/

#if defined(__amigaos4__)
/* ------------------- OS4 Manager Interface ------------------------ */
STATIC uint32 _manager_Obtain(struct LibraryManagerInterface *Self)
{
	uint32 res;
	__asm__ __volatile__(
		"1: lwarx  %0,0,%1\n"
		"   addic  %0,%0,1\n"
		"   stwcx. %0,0,%1\n"
		"   bne-   1b"
		: "=&r"(res)
		: "r"(&Self->Data.RefCount)
		: "cc", "memory");

	return res;
}

STATIC uint32 _manager_Release(struct LibraryManagerInterface *Self)
{
	uint32 res;
	__asm__ __volatile__(
		"1: lwarx  %0,0,%1\n"
		"   addic  %0,%0,-1\n"
		"   stwcx. %0,0,%1\n"
		"   bne-   1b"
		: "=&r"(res)
		: "r"(&Self->Data.RefCount)
		: "cc", "memory");

	return res;
}

STATIC CONST CONST_APTR lib_manager_vectors[] = {(CONST_APTR)_manager_Obtain,
												 (CONST_APTR)_manager_Release,
												 (CONST_APTR)NULL,
												 (CONST_APTR)NULL,
												 (CONST_APTR)LibOpen,
												 (CONST_APTR)LibClose,
												 (CONST_APTR)LibExpunge,
												 (CONST_APTR)NULL,
												 (CONST_APTR)-1};

STATIC CONST struct TagItem lib_managerTags[] = {{MIT_Name, (Tag) "__library"},
												 {MIT_VectorTable, (Tag)lib_manager_vectors},
												 {MIT_Version, 1},
												 {TAG_DONE, 0}};

/* ------------------- Library Interface(s) ------------------------ */

ULONG LibObtain(UNUSED struct Interface *Self)
{
	return 0;
}

ULONG LibRelease(UNUSED struct Interface *Self)
{
	return 0;
}

STATIC CONST CONST_APTR main_vectors[] = {(CONST_APTR)LibObtain,
										  (CONST_APTR)LibRelease,
										  (CONST_APTR)NULL,
										  (CONST_APTR)NULL,
										  (CONST_APTR)libvector,
										  (CONST_APTR)-1};

STATIC CONST struct TagItem mainTags[] = {{MIT_Name, (Tag) "main"},
										  {MIT_VectorTable, (Tag)main_vectors},
										  {MIT_Version, 1},
										  {TAG_DONE, 0}};

STATIC CONST CONST_APTR libInterfaces[] = {(CONST_APTR)lib_managerTags, (CONST_APTR)mainTags, (CONST_APTR)NULL};

	// Our libraries always have to carry a 68k jump table with it, so
	// lets define it here as extern, as we are going to link it to
	// our binary here.
	#ifndef NO_VECTABLE68K
extern CONST APTR VecTable68K[];
	#endif

STATIC CONST struct TagItem libCreateTags[] = {{CLT_DataSize, sizeof(struct MyLibrary)},
											   {CLT_InitFunc, (Tag)LibInit},
											   {CLT_Interfaces, (Tag)libInterfaces},
	#ifndef NO_VECTABLE68K
											   {CLT_Vector68K, (Tag)VecTable68K},
	#endif
											   {TAG_DONE, 0}};

#else

STATIC CONST CONST_APTR LibVectors[] = {
	#ifdef __MORPHOS__
	(CONST_APTR)FUNCARRAY_32BIT_NATIVE,
	#endif
	#if defined(__AROS__)
	(CONST_APTR)AROS_SLIB_ENTRY(LibOpen, DOpus, 1),
	(CONST_APTR)AROS_SLIB_ENTRY(LibClose, DOpus, 2),
	(CONST_APTR)AROS_SLIB_ENTRY(LibExpunge, DOpus, 3),
	#else
	(CONST_APTR)LibOpen,
	(CONST_APTR)LibClose,
	(CONST_APTR)LibExpunge,
	#endif
	(CONST_APTR)LibNull,
	(CONST_APTR)libvector,
	(CONST_APTR)-1};

STATIC CONST IPTR LibInitTab[] = {sizeof(struct MyLibrary), (IPTR)LibVectors, (IPTR)NULL, (IPTR)LibInit};

#endif

/****************************************************************************/

static const USED_VAR __TEXTSEGMENT__ struct Resident ROMTag = {
	RTC_MATCHWORD,
	(struct Resident *)&ROMTag,
	(struct Resident *)(&ROMTag + 1),
#if defined(__amigaos4__)
	RTF_AUTOINIT | RTF_NATIVE,	// The Library should be set up according to the given table.
#elif defined(__MORPHOS__)
	RTF_AUTOINIT | RTF_EXTENDED | RTF_PPC,
#elif defined(__AROS__)
	RTF_AUTOINIT | RTF_EXTENDED,
#else
	RTF_AUTOINIT,
#endif
	LIB_VERSION,
	NT_LIBRARY,
	0,
	(char *)UserLibName,
	(char *)UserLibID + 6,	// +6 to skip '$VER: '
#if defined(__amigaos4__)
	(APTR)libCreateTags	 // This table is for initializing the Library.
#else
	(APTR)LibInitTab,
#endif
#if defined(__MORPHOS__) || defined(__AROS__)
		LIB_REVISION,
	0
#endif
};

#if defined(__MORPHOS__)
/*
 * To tell the loader that this is a new emulppc elf and not
 * one for the ppc.library.
 * ** IMPORTANT **
 */
const USED_VAR ULONG __abox__ = 1;

#endif /* __MORPHOS */

/****************************************************************************/

#if defined(MIN_STACKSIZE) && !defined(__amigaos4__)

/* generic StackSwap() function which calls function() surrounded by
   StackSwap() calls */

	#if defined(__amigaos3__)
ULONG stackswap_call(struct StackSwapStruct *stack, ULONG (*function)(struct MyLibrary *), struct MyLibrary *arg);

asm(".text                    \n\
     .even                    \n\
     .globl _stackswap_call   \n\
   _stackswap_call:           \n\
      moveml #0x3022,sp@-     \n\
      movel sp@(20),d3        \n\
      movel sp@(24),a2        \n\
      movel sp@(28),d2        \n\
      movel _SysBase,a6       \n\
      movel d3,a0             \n\
      jsr a6@(-732:W)         \n\
      movel d2,sp@-           \n\
      jbsr a2@                \n\
      movel d0,d2             \n\
      addql #4,sp             \n\
      movel _SysBase,a6       \n\
      movel d3,a0             \n\
      jsr a6@(-732:W)         \n\
      movel d2,d0             \n\
      moveml sp@+,#0x440c     \n\
      rts");
	#elif defined(__MORPHOS__)
ULONG stackswap_call(struct StackSwapStruct *stack, ULONG (*function)(struct MyLibrary *), struct MyLibrary *arg)
{
	struct PPCStackSwapArgs swapargs;

	swapargs.Args[0] = (ULONG)arg;

	return NewPPCStackSwap(stack, function, &swapargs);
}
	#elif defined(__AROS__)
ULONG stackswap_call(struct StackSwapStruct *stack, ULONG (*function)(struct MyLibrary *), struct MyLibrary *arg)
{
	struct StackSwapArgs swapargs;

	swapargs.Args[0] = (IPTR)arg;

	return NewStackSwap(stack, function, &swapargs);
}
	#else
		#error Bogus operating system
	#endif

static BOOL callLibFunction(ULONG (*function)(struct MyLibrary *), struct MyLibrary *arg)
{
	BOOL success = FALSE;
	struct Task *tc;
	ULONG stacksize;

	// retrieve the task structure for the
	// current task
	tc = FindTask(NULL);

	#if defined(__MORPHOS__)
	// In MorphOS we have two stacks. One for PPC code and another for 68k code.
	// We are only interested in the PPC stack.
	NewGetTaskAttrsA(tc, &stacksize, sizeof(ULONG), TASKINFOTYPE_STACKSIZE, NULL);
	#else
	// on all other systems we query via SPUpper-SPLower calculation
	stacksize = (UBYTE *)tc->tc_SPUpper - (UBYTE *)tc->tc_SPLower;
	#endif

	// Swap stacks only if current stack is insufficient
	if (stacksize < MIN_STACKSIZE)
	{
		struct StackSwapStruct *stack;

		if ((stack = AllocVec(sizeof(*stack), MEMF_PUBLIC)) != NULL)
		{
			if ((stack->stk_Lower = AllocVec(MIN_STACKSIZE, MEMF_PUBLIC)) != NULL)
			{
	// perform the StackSwap
	#if defined(__AROS__)
				// AROS uses an APTR type for stk_Upper
				stack->stk_Upper = (APTR)((IPTR)stack->stk_Lower + MIN_STACKSIZE);
	#else
				// all other systems use ULONG
				stack->stk_Upper = (ULONG)stack->stk_Lower + MIN_STACKSIZE;
	#endif
				stack->stk_Pointer = (APTR)stack->stk_Upper;

				// call routine but with embedding it into a [NewPPC]StackSwap()
				success = stackswap_call(stack, function, arg);

				FreeVec(stack->stk_Lower);
			}
			FreeVec(stack);
		}
	}
	else
		success = function(arg);

	return success;
}
#else  // MIN_STACKSIZE && !__amigaos4__
	#define callLibFunction(func, arg) func(arg)
#endif	// MIN_STACKSIZE && !__amigaos4__

/****************************************************************************/

#if defined(__amigaos4__)
static struct MyLibrary *LibInit(struct MyLibrary *base, BPTR librarySegment, struct ExecIFace *pIExec)
{
	struct ExecBase *sb = (struct ExecBase *)pIExec->Data.LibBase;
	IExec = pIExec;
#elif defined(__MORPHOS__)
static struct MyLibrary *LibInit(struct MyLibrary *base, BPTR librarySegment, struct ExecBase *sb)
{
#elif defined(__AROS__)
static AROS_UFH3(struct MyLibrary *,
				 LibInit,
				 AROS_UFHA(struct MyLibrary *, base, D0),
				 AROS_UFHA(BPTR, librarySegment, A0),
				 AROS_UFHA(struct ExecBase *, sb, A6))
{
	AROS_USERFUNC_INIT
#else
static struct MyLibrary *LIBFUNC LibInit(REG(d0, struct MyLibrary *base),
										 REG(a0, BPTR librarySegment),
										 REG(a6, struct ExecBase *sb))
{
#endif

	base->sysBase = (APTR)sb;
	SysBase = (APTR)sb;

// make sure that this is really a 68020+ machine if optimized for 020+
#if _M68060 || _M68040 || _M68030 || _M68020 || __mc68020 || __mc68030 || __mc68040 || __mc68060
	if (!(SysBase->AttnFlags & AFF_68020))
		return (NULL);
#endif

#if defined(__amigaos4__) && defined(__NEWLIB__)
	if ((NewlibBase = OpenLibrary("newlib.library", 3)) && GETINTERFACE(INewlib, NewlibBase))
#endif
#ifdef __AROS__
	if (set_open_libraries() && set_call_funcs(SETNAME(INIT), 1, 1))
#endif
	{
		// cleanup the library header structure beginning with the
		// library base.
		base->libBase.lib_Node.ln_Type = NT_LIBRARY;
		base->libBase.lib_Node.ln_Pri = 0;
		base->libBase.lib_Node.ln_Name = (char *)UserLibName;
		base->libBase.lib_Flags = LIBF_CHANGED | LIBF_SUMUSED;
		base->libBase.lib_Version = LIB_VERSION;
		base->libBase.lib_Revision = LIB_REVISION;
		base->libBase.lib_IdString = (char *)(UserLibID + 6);

		memset(&base->libSem, 0, sizeof(base->libSem));
		InitSemaphore(&base->libSem);

		// everything was successfully so lets
		// set the initialized value and contiue
		// with the class open phase
		base->segList = librarySegment;

		// return the library base as success
		return base;
	}

	return NULL;
#ifdef __AROS__
	AROS_USERFUNC_EXIT
#endif
}

/****************************************************************************/

#ifndef __amigaos4__
	#define DeleteLibrary(LIB) \
		FreeMem((STRPTR)(LIB) - (LIB)->lib_NegSize, (ULONG)((LIB)->lib_NegSize + (LIB)->lib_PosSize))
#endif

STATIC BPTR LibDelete(struct MyLibrary *base)
{
#if defined(__amigaos4__)
	struct ExecIFace *IExec = (struct ExecIFace *)((struct ExecBase *)SysBase)->MainInterface;
#endif
	BPTR rc;

	// make sure to restore the SysBase
	SysBase = (APTR)base->sysBase;

	// remove the library base from exec's lib list in advance
	Remove((struct Node *)base);

#if defined(__amigaos4__) && defined(__NEWLIB__)
	if (NewlibBase)
	{
		DROPINTERFACE(INewlib);
		CloseLibrary(NewlibBase);
		NewlibBase = NULL;
	}
#endif
#ifdef __AROS__
	set_call_funcs(SETNAME(EXIT), -1, 0);
	set_close_libraries();
#endif

	// make sure the system deletes the library as well.
	rc = base->segList;
	DeleteLibrary(&base->libBase);

	return rc;
}

#if defined(__amigaos4__)
static BPTR LibExpunge(struct LibraryManagerInterface *Self)
{
	struct MyLibrary *base = (struct MyLibrary *)Self->Data.LibBase;
#elif defined(__MORPHOS__)
static BPTR LibExpunge(void)
{
	struct MyLibrary *base = (struct MyLibrary *)REG_A6;
#elif defined(__AROS__)
static AROS_LH1(BPTR,
				LibExpunge,
				AROS_LHA(UNUSED struct MyLibrary *, __extrabase, D0),
				struct MyLibrary *,
				base,
				3,
				DOpus)
{
	AROS_LIBFUNC_INIT
#else
static BPTR LIBFUNC LibExpunge(REG(a6, struct MyLibrary *base))
{
#endif
	BPTR rc;

	// in case our open counter is still > 0, we have
	// to set the late expunge flag and return immediately
	if (base->libBase.lib_OpenCnt > 0)
	{
		base->libBase.lib_Flags |= LIBF_DELEXP;
		rc = 0;
	}
	// in case the open counter is <= 0 we can
	// make sure that we free everything
	else
	{
		// free all our private data and stuff.
		ObtainSemaphore(&base->libSem);

		if (base->initialized == 1)
		{
			base->initialized = 0;
			// make sure we have enough stack here
			callLibFunction(freeBase, base);
		}

		// unprotect
		ReleaseSemaphore(&base->libSem);

		rc = LibDelete(base);
	}

	return rc;
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

/****************************************************************************/

#if defined(__amigaos4__)
static struct MyLibrary *LibOpen(struct LibraryManagerInterface *Self, ULONG version UNUSED)
{
	struct MyLibrary *base = (struct MyLibrary *)Self->Data.LibBase;
#elif defined(__MORPHOS__)
static struct MyLibrary *LibOpen(void)
{
	struct MyLibrary *base = (struct MyLibrary *)REG_A6;
#elif defined(__AROS__)
static AROS_LH1(struct MyLibrary *, LibOpen, AROS_LHA(UNUSED ULONG, version, D0), struct MyLibrary *, base, 1, DOpus)
{
	AROS_LIBFUNC_INIT
#else
static struct MyLibrary *LIBFUNC LibOpen(REG(d0, UNUSED ULONG version), REG(a6, struct MyLibrary *base))
{
#endif
	BOOL success = FALSE;

	// LibOpen(), LibClose() and LibExpunge() are called while the system is in
	// Forbid() state. That means that these functions should be quick and should
	// not break this Forbid()!! Therefore the open counter should be increased
	// as the very first instruction during LibOpen(), because a ClassOpen()
	// which breaks a Forbid() and another task calling LibExpunge() will cause
	// to expunge this library while it is not yet fully initialized. A crash
	// is unavoidable then. Even the semaphore does not guarantee 100% protection
	// against such a race condition, because waiting for the semaphore to be
	// obtained will effectively break the Forbid()!

	// increase the open counter ahead of anything else
	base->libBase.lib_OpenCnt++;

	// delete the late expunge flag
	base->libBase.lib_Flags &= ~LIBF_DELEXP;

	// protect access to initBase()
	ObtainSemaphore(&base->libSem);

	if (base->initialized == 0)
	{
		// If we are not running on AmigaOS4 (no stackswap required) we go and
		// do an explicit StackSwap() in case the user wants to make sure we
		// have enough stack for his user functions
		success = callLibFunction(initBase, base);

		if (success)
			base->initialized = 1;
	}

	// unprotect initBase()
	ReleaseSemaphore(&base->libSem);

	return base->initialized ? base : NULL;
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

/****************************************************************************/

#if defined(__amigaos4__)
static BPTR LibClose(struct LibraryManagerInterface *Self)
{
	struct MyLibrary *base = (struct MyLibrary *)Self->Data.LibBase;
#elif defined(__MORPHOS__)
static BPTR LibClose(void)
{
	struct MyLibrary *base = (struct MyLibrary *)REG_A6;
#elif defined(__AROS__)
static AROS_LH0(BPTR, LibClose, struct MyLibrary *, base, 2, DOpus)
{
	AROS_LIBFUNC_INIT
#else
static BPTR LIBFUNC LibClose(REG(a6, struct MyLibrary *base))
{
#endif
	BPTR rc = 0;

	// decrease the open counter
	base->libBase.lib_OpenCnt--;

	// check if all the AppIcons/AppWindows/AppMenus have been removed
	// and purge them immediately to reduce the open count
	if (base->initialized == 1)
	{
		extern void check_app_list(struct LibData * data, BOOL purge);
		check_app_list((struct LibData *)base->ml_UserData, TRUE);
	}

	// in case the open counter is <= 0 we can
	// make sure that we free everything
	if (base->libBase.lib_OpenCnt <= 0)
	{
		// free all our private data and stuff.
		ObtainSemaphore(&base->libSem);

		if (base->initialized == 1)
		{
			base->initialized = 0;
			// make sure we have enough stack here
			callLibFunction(freeBase, base);
		}

		// unprotect
		ReleaseSemaphore(&base->libSem);

		// in case the late expunge flag is set we go and
		// expunge the library base right now
		if (base->libBase.lib_Flags & LIBF_DELEXP)
		{
			rc = LibDelete(base);
		}
	}

	return rc;
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

/**************************************************************************/
/**************************************************************************/

ULONG freeBase(struct MyLibrary *lib)
{
	if (lib)
		UserLibCleanup((struct MyLibrary *)lib);

	// close Picasso96API.library
	if (P96Base != NULL)
	{
		DROPINTERFACE(IP96);
		CloseLibrary((struct Library *)P96Base);
		P96Base = NULL;
	}

	// close cybergraphics.library (fallback path for read_ilbm.c)
	if (CyberGfxBase != NULL)
	{
		DROPINTERFACE(ICyberGfx);
		CloseLibrary((struct Library *)CyberGfxBase);
		CyberGfxBase = NULL;
	}

	// close newicon.library
	if (NewIconBase != NULL)
	{
		DROPINTERFACE(INewIcon);
		CloseLibrary((struct Library *)NewIconBase);
		NewIconBase = NULL;
	}

	// close locale.library
	if (LocaleBase != NULL)
	{
		DROPINTERFACE(ILocale);
		CloseLibrary((struct Library *)LocaleBase);
		LocaleBase = NULL;
	}

	// close rexxsyslib.library
	if (RexxSysBase != NULL)
	{
		DROPINTERFACE(IRexxSys);
		CloseLibrary((struct Library *)RexxSysBase);
		RexxSysBase = NULL;
	}

	// close datatypes.library
	if (DataTypesBase != NULL)
	{
		DROPINTERFACE(IDataTypes);
		CloseLibrary((struct Library *)DataTypesBase);
		DataTypesBase = NULL;
	}

	// close workbench.library
	if (WorkbenchBase != NULL)
	{
		DROPINTERFACE(IWorkbench);
		CloseLibrary((struct Library *)WorkbenchBase);
		WorkbenchBase = NULL;
	}

	// close icon.library
	if (IconBase != NULL)
	{
		DROPINTERFACE(IIcon);
		CloseLibrary((struct Library *)IconBase);
		IconBase = NULL;
	}

	// close diskfont.library
	if (DiskFontBase != NULL)
	{
		DROPINTERFACE(IDiskFont);
		CloseLibrary((struct Library *)DiskFontBase);
		DiskFontBase = NULL;
	}

	// close layers.library
	if (LayersBase != NULL)
	{
		DROPINTERFACE(ILayers);
		CloseLibrary((struct Library *)LayersBase);
		LayersBase = NULL;
	}

	// close asl.library
	if (AslBase != NULL)
	{
		DROPINTERFACE(IAsl);
		CloseLibrary((struct Library *)AslBase);
		AslBase = NULL;
	}

	// close gadtools.library
	if (GadToolsBase != NULL)
	{
		DROPINTERFACE(IGadTools);
		CloseLibrary((struct Library *)GadToolsBase);
		GadToolsBase = NULL;
	}

	// close intuition.library
	if (IntuitionBase != NULL)
	{
		DROPINTERFACE(IIntuition);
		CloseLibrary((struct Library *)IntuitionBase);
		IntuitionBase = NULL;
	}

	// close graphics.library
	if (GfxBase != NULL)
	{
		DROPINTERFACE(IGraphics);
		CloseLibrary((struct Library *)GfxBase);
		GfxBase = NULL;
	}

	// close utility.library
	if (UtilityBase != NULL)
	{
		DROPINTERFACE(IUtility);
		CloseLibrary((struct Library *)UtilityBase);
		UtilityBase = NULL;
	}

	// close dos.library
	if (DOSBase != NULL)
	{
		DROPINTERFACE(IDOS);
		CloseLibrary((struct Library *)DOSBase);
		DOSBase = NULL;
	}

	return TRUE;
}

/***********************************************************************/

ULONG initBase(struct MyLibrary *lib)
{
	if ((DOSBase = (APTR)OpenLibrary("dos.library", 37)) != NULL && GETINTERFACE(IDOS, DOSBase))
		if ((UtilityBase = (APTR)OpenLibrary("utility.library", 37)) != NULL && GETINTERFACE(IUtility, UtilityBase))
			if ((GfxBase = (APTR)OpenLibrary("graphics.library", 37)) != NULL && GETINTERFACE(IGraphics, GfxBase))
				if ((IntuitionBase = (APTR)OpenLibrary("intuition.library", 37)) != NULL &&
					GETINTERFACE(IIntuition, IntuitionBase))
					if ((GadToolsBase = (APTR)OpenLibrary("gadtools.library", 37)) != NULL &&
						GETINTERFACE(IGadTools, GadToolsBase))
						if ((AslBase = (APTR)OpenLibrary("asl.library", 37)) != NULL && GETINTERFACE(IAsl, AslBase))
							if ((LayersBase = (APTR)OpenLibrary("layers.library", 37)) != NULL &&
								GETINTERFACE(ILayers, LayersBase))
								if ((DiskFontBase = (APTR)OpenLibrary("diskfont.library", 37)) != NULL &&
									GETINTERFACE(IDiskFont, DiskFontBase))
									if ((IconBase = (APTR)OpenLibrary("icon.library", 37)) != NULL &&
										GETINTERFACE(IIcon, IconBase))
										if ((WorkbenchBase = (APTR)OpenLibrary("workbench.library", 37)) != NULL &&
											GETINTERFACE(IWorkbench, WorkbenchBase))
											if ((DataTypesBase = (APTR)OpenLibrary("datatypes.library", 37)) != NULL &&
												GETINTERFACE(IDataTypes, DataTypesBase))
												if ((RexxSysBase = (APTR)OpenLibrary("rexxsyslib.library", 0)) !=
														NULL &&
													GETINTERFACE(IRexxSys, RexxSysBase))
													if ((LocaleBase = (APTR)OpenLibrary("locale.library", 37)) !=
															NULL &&
														GETINTERFACE(ILocale, LocaleBase))
													{
// we have to please the internal utilitybase
// pointers of libnix and clib2
#if !defined(__libnix__) && !defined(__NEWLIB__) && !defined(__AROS__)
														__UtilityBase = (APTR)UtilityBase;
	#if defined(__amigaos4__)
														__IUtility = IUtility;
	#endif
#endif

														UserLibInit((struct MyLibrary *)lib);

														return TRUE;
													}

	D(bug("dopus5.library failed to open a required system library\n"))
	Printf("dopus5.library failed to open a required system library\n");

	freeBase(NULL);

	return FALSE;
}

/***********************************************************************/

// Initialise some other libraries we need together with dopus5 datas and structures
int UserLibInit(REG(a6, struct MyLibrary *libbase))
{
	struct LibData *data;
	char buf[16];

	// Initialise
	topaz_font = 0;
	NewList(&image_list);
	InitSemaphore(&image_lock);
	image_memory = 0;
	// chip_memory=0;
	listview_class = 0;
	image_class = 0;
	button_class = 0;
	string_class = 0;
	check_class = 0;
	view_class = 0;
	palette_class = 0;
	gauge_class = 0;
	launcher_ipc = 0;

	// Get library data
	if (!(data = AllocVec(sizeof(struct LibData), MEMF_CLEAR)))
		return 1;

	libbase->ml_UserData = data;

	// Check for OS 3.5 icon library
	// if <44 then try for NewIcons. Don't open NewIcons under V44

	if (IconBase->lib_Version >= 44)
	{
		data->flags |= LIBDF_USING_OS35;
	}
	else
	{
		NewIconBase = (struct Library *)OpenLibrary("newicon.library", 0);
#ifdef __amigaos4__
		GETINTERFACE(INewIcon, NewIconBase);
#endif
	}

	// Open timer device
	if (OpenDevice("timer.device", UNIT_VBLANK, &data->timer_io, 0))
		return 1;
	data->TimerBase = (struct Library *)data->timer_io.io_Device;

	// Is Picasso96API already in system? If so, open it for ourselves.
	// On OS4 require the interface to come up too; roll the library back
	// on failure so the "base non-NULL implies interface non-NULL"
	// invariant holds, and the cleanup path's DROPINTERFACE(IP96) is safe.
	if (FindName(&((struct ExecBase *)SysBase)->LibList, "Picasso96API.library"))
	{
		if ((P96Base = OpenLibrary("Picasso96API.library", 2)))
		{
#ifdef __amigaos4__
			GETINTERFACE(IP96, P96Base);
			if (!IP96)
			{
				CloseLibrary((struct Library *)P96Base);
				P96Base = NULL;
			}
#endif
		}
	}

	// Also check for cybergraphics.library - used as a fallback in the
	// 24-bit ILBM write path when P96 is unavailable (older OS3 setups,
	// PPC accelerators with CGX but no Picasso96, etc.). On most modern
	// setups P96 ships a cybergraphics compat wrapper so both are resident
	// and P96 wins; the fallback only matters on CGX-only installs.
	// Same interface-acquisition contract as P96 above.
	if (FindName(&((struct ExecBase *)SysBase)->LibList, "cybergraphics.library"))
	{
		if ((CyberGfxBase = OpenLibrary("cybergraphics.library", 0)))
		{
#ifdef __amigaos4__
			GETINTERFACE(ICyberGfx, CyberGfxBase);
			if (!ICyberGfx)
			{
				CloseLibrary((struct Library *)CyberGfxBase);
				CyberGfxBase = NULL;
			}
#endif
		}
	}

	// Get topaz font
	if (!(topaz_font = OpenFont(&topaz_attr)))
		return 1;

	// Initialise stuff
	data->low_mem_signal = -1;
	/*data->wb_data.wb_base=WorkbenchBase;
	data->wb_data.int_base=(struct Library *)IntuitionBase;*/
	data->wb_data.dopus_base = libbase;
	/*data->wb_data.utility_base=UtilityBase;
	data->dos_base=(struct Library *)DOSBase;
	data->icon_base=IconBase;
	data->new_icon_base=(struct Library *)NewIconBase;
	data->int_base=(struct Library *)IntuitionBase;*/
	data->dopus_base = libbase;
	/*data->gfx_base=(struct Library *)GfxBase;*/
	NewList((struct List *)&data->wb_data.app_list);
	NewList((struct List *)&data->wb_data.rem_app_list);
	NewList((struct List *)&data->notify_list);
	NewList((struct List *)&data->launch_list);
	NewList((struct List *)&data->error_list);
	NewList(&data->semaphores.list);
	NewList(&data->device_list.list);
	NewList(&data->file_list.list);
	NewList(&data->filetype_cache.list);
	NewList(&data->allocbitmap_patch.list);
	NewList((struct List *)&data->dos_list);
	InitSemaphore(&data->dos_lock);
	InitSemaphore(&data->wb_data.patch_lock);
	InitSemaphore(&data->notify_lock);
	InitSemaphore(&data->launch_lock);
	InitSemaphore(&data->path_lock);
	InitSemaphore(&data->semaphores.lock);
	InitSemaphore(&data->device_list.lock);
	InitSemaphore(&data->file_list.lock);
	InitSemaphore(&data->filetype_cache.lock);
	InitSemaphore(&data->allocbitmap_patch.lock);
	InitSemaphore(&data->backfill_lock);
	data->popup_delay = 10;

#if defined(__amigaos4__) || defined(__AROS__)
	dopuslibbase_global = libbase;
#endif

	// Initialise NewIcons settings
	data->NewIconsFlags = ENVNIF_ENABLE;
	data->NewIconsPrecision = 16;

	// See if SysIHack is running
	if (FindTask("« sysihack »"))
		data->flags |= LIBDF_3DLOOK;

	// Or variable is set for 3d gadgets
	else if (GetVar("dopus/3DLook", buf, 2, GVF_GLOBAL_ONLY) > -1)
		data->flags |= LIBDF_3DLOOK;

	// Variable set for no stippling of requesters
	if (GetVar("dopus/OuEstLeMinibar", buf, 2, GVF_GLOBAL_ONLY) > -1)
		data->flags |= LIBDF_NOSTIPPLE;

	// Variable set to install DOS patches
	if (GetVar("dopus/DOSPatch", buf, 2, GVF_GLOBAL_ONLY) > -1)
		data->flags |= LIBDF_DOS_PATCH;

	// Variable set to install filetype cache
	if (GetVar("dopus/FiletypeCache", buf, 14, GVF_GLOBAL_ONLY) > -1)
	{
		// Get cache size
		data->ft_cache_max = atoi(buf);
		if (data->ft_cache_max < 10)
			data->ft_cache_max = 10;

		// Set cache flag
		data->flags |= LIBDF_FT_CACHE;
	}
	// Variable set to NOT install any system patches
	if (GetVar("dopus/NoSysPatch", buf, 2, GVF_GLOBAL_ONLY) > -1)
		data->flags |= LIBDF_NO_PATCHES;

	// Create some memory handles
	data->memory = L_NewMemHandle(sizeof(IPCMessage) << 5, sizeof(IPCMessage) << 4, MEMF_CLEAR | MEMF_PUBLIC);
	data->dos_list_memory = L_NewMemHandle(1024, 512, MEMF_CLEAR);
	// Scratch pool for dos_patch.c (FIB + 512-byte path buffer = ~764 bytes
	// per call).  Puddle holds ~5 allocations; threshold is set so they all
	// flow through the pool rather than direct AllocMem.  MEMF_PUBLIC flips
	// on the pool's internal semaphore since patched DOS functions can run
	// concurrently from any task.
	data->dos_patch_memory = L_NewMemHandle(4096, 1024, MEMF_CLEAR | MEMF_PUBLIC);

	// Memory handle for a bit of chip memory
	// chip_memory=L_NewMemHandle(4096,2048,MEMF_CLEAR|MEMF_CHIP|MEMF_PUBLIC);

	// Initialise boopsi classes
	if (!(image_class = init_class(
			  data, "dopusiclass", "imageclass", (IPTR (*)())image_dispatch, sizeof(BoopsiImageData))) ||

		!(button_class = init_class(
			  data, "dopusbuttongclass", "gadgetclass", (IPTR (*)())button_dispatch, sizeof(ButtonData))) ||

		!(string_class = init_class(
			  data, "dopusstrgclass", "strgclass", (IPTR (*)())button_dispatch, sizeof(StringData))) ||

		!(check_class = init_class(
			  data, "dopuscheckgclass", "gadgetclass", (IPTR (*)())button_dispatch, sizeof(CheckData))) ||

		!(view_class = init_class(
			  data, "dopusviewgclass", "gadgetclass", (IPTR (*)())button_dispatch, sizeof(ButtonData))) ||

		!(palette_class = init_class(
			  data, "dopuspalettegclass", "gadgetclass", (IPTR (*)())palette_dispatch, sizeof(PaletteData))) ||

		!(frame_class = init_class(
			  data, "dopusframeclass", "gadgetclass", (IPTR (*)())button_dispatch, sizeof(ButtonData))) ||

		!(gauge_class = init_class(
			  data, "dopusgaugeclass", "gadgetclass", (IPTR (*)())button_dispatch, sizeof(GaugeData))) ||

		!(listview_class = init_class(data,
									  "dopuslistviewgclass",
									  "gadgetclass",
									  (IPTR (*)())listview_dispatch,
									  sizeof(ListViewData))))
		return 1;

	// Set flag to identify some classes
	if (string_class)
		string_class->cl_UserData = CLASS_STRINGGAD;
	if (check_class)
		check_class->cl_UserData = CLASS_CHECKGAD;
	if (view_class)
		view_class->cl_UserData = CLASS_VIEW;
	if (frame_class)
		frame_class->cl_UserData = CLASS_FRAME;
	if (gauge_class)
		gauge_class->cl_UserData = CLASS_GAUGE;

	// Initialise locale
	init_locale_data(&data->locale);

	// Did we get locale library?
	if ((LocaleBase = data->locale.li_LocaleBase))
	{
		// Get decimal point
		if (data->locale.li_Locale && data->locale.li_Locale->loc_DecimalPoint)
			decimal_point = data->locale.li_Locale->loc_DecimalPoint[0];
	}

	// Get a path list
	data->path_list = L_GetDosPathList(0);

	// Create launcher process
	if (!(L_IPC_Launch(0,
					   &launcher_ipc,
					   "DO_LAUNCHER",
					   IPC_NATIVE(launcher_proc),
					   STACK_LARGE | IPCF_GETPATH,
					   (IPTR)data,
					   (struct Library *)DOSBase,
					   libbase)) ||
		!launcher_ipc)
		return 1;
	data->launcher = launcher_ipc;

	// Initialise low-memory handler
	if (((struct Library *)SysBase)->lib_Version >= 39)
	{
		// Initialise interrupt
		data->low_mem_handler.is_Node.ln_Pri = 50;
#ifdef __amigaos4__
		data->low_mem_handler.is_Node.ln_Type = NT_EXTINTERRUPT;
#else
		data->low_mem_handler.is_Node.ln_Type = NT_INTERRUPT;
#endif
		data->low_mem_handler.is_Node.ln_Name = "dopus memhandler";
		data->low_mem_handler.is_Data = data;

#if defined(__MORPHOS__)
		data->low_mem_handler_trap.Trap = TRAP_LIB;
		data->low_mem_handler_trap.Func = (void (*)())low_mem_handler_entry;
		data->low_mem_handler.is_Code = (void (*)()) & data->low_mem_handler_trap;
#else
		data->low_mem_handler.is_Code = (void (*)())low_mem_handler;
#endif

		// Add the handler
		AddMemHandler(&data->low_mem_handler);
	}

	// Succeeded
	return 0;
}

// Clean up
void UserLibCleanup(REG(a6, struct MyLibrary *libbase))
{
	WB_Data *wb_data;
	struct LibData *data;

	L_FlushImages();

	// Launcher?
	if (launcher_ipc)
	{
		L_IPC_Command(launcher_ipc, IPC_QUIT, 0, 0, 0, REPLY_NO_PORT_IPC);
	}

	// Library data?
	if ((data = (struct LibData *)libbase->ml_UserData))
	{
		// Get workbench data
		wb_data = &data->wb_data;

		// Remove low-memory handler
		if (data->low_mem_handler.is_Node.ln_Pri == 50)
			RemMemHandler(&data->low_mem_handler);

		// Remove patch information
		if (wb_data)
		{
			if (wb_data->patch_count > 0)
			{
				wb_data->patch_count = 1;
				L_WB_Remove_Patch(libbase);
			}
		}

		// Free path list
		L_FreeDosPathList(data->path_list);

		// Free locale stuff
		free_locale_data(&data->locale);

		// Free memory
		L_FreeMemHandle(data->memory);
		L_FreeMemHandle(data->dos_list_memory);
		L_FreeMemHandle(data->dos_patch_memory);

		// Close timer
		if (data->TimerBase)
			CloseDevice(&data->timer_io);

		// Free library data
		FreeVec(data);
	}

	// L_FreeMemHandle(chip_memory);
	class_free(listview_class);
	class_free(button_class);
	class_free(string_class);
	class_free(check_class);
	class_free(view_class);
	class_free(frame_class);
	class_free(palette_class);
	class_free(gauge_class);
	class_free(image_class);
	if (topaz_font)
		CloseFont(topaz_font);
}

// Low memory handler
#ifndef __amigaos4__
int low_mem_handler(REG(a0, struct MemHandlerData *memh), REG(a1, struct LibData *data))
{
	// Is this the first time?
	if (!(memh->memh_Flags & MEMHF_RECYCLE))
	{
		// Signal the launcher to send the message
		if (data->low_mem_signal > -1)
			Signal((struct Task *)data->launcher->proc, 1 << data->low_mem_signal);

		// Tell it to try again
		return MEM_TRY_AGAIN;
	}

	// We don't actually do anything
	return MEM_DID_NOTHING;
}
#else
int low_mem_handler(struct ExecBase *ExecBase, struct MemHandlerData *memh, struct LibData *data)
{
	// Check if the launcher has set the low mem signal
	if (data->low_mem_signal > -1)
	{
		// Signal the launcher to send the message
		Signal((struct Task *)data->launcher->proc, 1 << data->low_mem_signal);

		// Tell exec we're done
		return MEM_ALL_DONE;
	}

	// We don't actually do anything
	return MEM_DID_NOTHING;
}
#endif
