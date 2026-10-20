/* Automatically generated header! Do not edit! */

#ifndef _INLINE_CONFIGOPUS_H
#define _INLINE_CONFIGOPUS_H

#ifndef AROS_LIBCALL_H
	#include <aros/libcall.h>
#endif /* !AROS_LIBCALL_H */

#ifndef CONFIGOPUS_BASE_NAME
	#define CONFIGOPUS_BASE_NAME ConfigOpusBase
#endif /* !CONFIGOPUS_BASE_NAME */

#define Config_Buttons(___bank, ___ipc, ___owner, ___screen, ___cmdlist) \
	AROS_LC5(int,                                                        \
			 Config_Buttons,                                             \
			 AROS_LCA(ButtonsStartup *, (___bank), A0),                  \
			 AROS_LCA(IPCData *, (___ipc), A1),                          \
			 AROS_LCA(IPCData *, (___owner), A2),                        \
			 AROS_LCA(struct Screen *, (___screen), A3),                 \
			 AROS_LCA(IPTR, (___cmdlist), D0),                           \
			 struct Library *,                                           \
			 CONFIGOPUS_BASE_NAME,                                       \
			 8,                                                          \
			 /* s */)

#define Config_EditFunction(___ipc, ___opus, ___window, ___func, ___memory, ___cmdlist) \
	AROS_LC6(Cfg_Function *,                                                            \
			 Config_EditFunction,                                                       \
			 AROS_LCA(IPCData *, (___ipc), A0),                                         \
			 AROS_LCA(IPCData *, (___opus), A1),                                        \
			 AROS_LCA(struct Window *, (___window), A2),                                \
			 AROS_LCA(Cfg_Function *, (___func), A3),                                   \
			 AROS_LCA(APTR, (___memory), D0),                                           \
			 AROS_LCA(IPTR, (___cmdlist), D1),                                          \
			 struct Library *,                                                          \
			 CONFIGOPUS_BASE_NAME,                                                      \
			 15,                                                                        \
			 /* s */)

#define Config_Environment(___env, ___screen, ___table, ___ipc, ___mainipc, ___alloc, ___change, ___name, ___scripts) \
	AROS_LC9(unsigned long,                                                                                           \
			 Config_Environment,                                                                                      \
			 AROS_LCA(Cfg_Environment *, (___env), A0),                                                               \
			 AROS_LCA(struct Screen *, (___screen), A1),                                                              \
			 AROS_LCA(UWORD *, (___table), A2),                                                                       \
			 AROS_LCA(IPCData *, (___ipc), A3),                                                                       \
			 AROS_LCA(IPCData *, (___mainipc), A4),                                                                   \
			 AROS_LCA(UWORD, (___alloc), D0),                                                                         \
			 AROS_LCA(ULONG *, (___change), D1),                                                                      \
			 AROS_LCA(char *, (___name), D2),                                                                         \
			 AROS_LCA(Att_List *, (___scripts), D3),                                                                  \
			 struct Library *,                                                                                        \
			 CONFIGOPUS_BASE_NAME,                                                                                    \
			 6,                                                                                                       \
			 /* s */)

#define Config_Filetypes(___screen, ___ipc, ___owner, ___cmdlist, ___name) \
	AROS_LC5(short,                                                        \
			 Config_Filetypes,                                             \
			 AROS_LCA(struct Screen *, (___screen), A0),                   \
			 AROS_LCA(IPCData *, (___ipc), A1),                            \
			 AROS_LCA(IPCData *, (___owner), A2),                          \
			 AROS_LCA(IPTR, (___cmdlist), D0),                             \
			 AROS_LCA(char *, (___name), A3),                              \
			 struct Library *,                                             \
			 CONFIGOPUS_BASE_NAME,                                         \
			 10,                                                           \
			 /* s */)

#define Config_ListerButtons(___name, ___ipc, ___owner, ___screen, ___bank, ___cmdlist, ___init) \
	AROS_LC7(ULONG,                                                                              \
			 Config_ListerButtons,                                                               \
			 AROS_LCA(char *, (___name), A0),                                                    \
			 AROS_LCA(IPCData *, (___ipc), A1),                                                  \
			 AROS_LCA(IPCData *, (___owner), A2),                                                \
			 AROS_LCA(struct Screen *, (___screen), A3),                                         \
			 AROS_LCA(Cfg_ButtonBank *, (___bank), D0),                                          \
			 AROS_LCA(IPTR, (___cmdlist), D1),                                                 \
			 AROS_LCA(LONG, (___init), D2),                                                      \
			 struct Library *,                                                                   \
			 CONFIGOPUS_BASE_NAME,                                                               \
			 11,                                                                                 \
			 /* s */)

#define Config_Menu(                                                                                     \
	___name, ___ipc, ___owner, ___screen, ___bank, ___title, ___cmdlist, ___def, ___hotkeys, ___scripts) \
	AROS_LC10(ULONG,                                                                                     \
			  Config_Menu,                                                                               \
			  AROS_LCA(char *, (___name), A0),                                                           \
			  AROS_LCA(IPCData *, (___ipc), A1),                                                         \
			  AROS_LCA(IPCData *, (___owner), A2),                                                       \
			  AROS_LCA(struct Screen *, (___screen), A3),                                                \
			  AROS_LCA(Cfg_ButtonBank *, (___bank), D0),                                                 \
			  AROS_LCA(char *, (___title), D1),                                                          \
			  AROS_LCA(IPTR, (___cmdlist), D2),                                                          \
			  AROS_LCA(char *, (___def), D3),                                                            \
			  AROS_LCA(short, (___hotkeys), D4),                                                         \
			  AROS_LCA(Att_List *, (___scripts), D5),                                                    \
			  struct Library *,                                                                          \
			  CONFIGOPUS_BASE_NAME,                                                                      \
			  12,                                                                                        \
			  /* s */)

#define Config_Menus(___ipc, ___owner, ___screen, ___menu, ___cmdlist, ___type, ___name) \
	AROS_LC7(IPTR,                                                                       \
			 Config_Menus,                                                               \
			 AROS_LCA(IPCData *, (___ipc), A0),                                          \
			 AROS_LCA(IPCData *, (___owner), A1),                                        \
			 AROS_LCA(struct Screen *, (___screen), A2),                                 \
			 AROS_LCA(Cfg_ButtonBank *, (___menu), A3),                                  \
			 AROS_LCA(IPTR, (___cmdlist), D0),                                           \
			 AROS_LCA(ULONG, (___type), D1),                                             \
			 AROS_LCA(char *, (___name), D2),                                            \
			 struct Library *,                                                           \
			 CONFIGOPUS_BASE_NAME,                                                       \
			 13,                                                                         \
			 /* s */)

#define Config_Settings(___env, ___screen, ___ipc, ___mainipc, ___name) \
	AROS_LC5(int,                                                       \
			 Config_Settings,                                           \
			 AROS_LCA(Cfg_Environment *, (___env), A0),                 \
			 AROS_LCA(struct Screen *, (___screen), A1),                \
			 AROS_LCA(IPCData *, (___ipc), A2),                         \
			 AROS_LCA(IPCData *, (___mainipc), A3),                     \
			 AROS_LCA(char *, (___name), D0),                           \
			 struct Library *,                                          \
			 CONFIGOPUS_BASE_NAME,                                      \
			 5,                                                         \
			 /* s */)

#define ConvertConfig(___name, ___screen, ___owner)      \
	AROS_LC3(BOOL,                                       \
			 ConvertConfig,                              \
			 AROS_LCA(char *, (___name), A0),            \
			 AROS_LCA(struct Screen *, (___screen), A1), \
			 AROS_LCA(IPCData *, (___owner), A2),        \
			 struct Library *,                           \
			 CONFIGOPUS_BASE_NAME,                       \
			 9,                                          \
			 /* s */)

#define EditFiletype(___type, ___window, ___ipc, ___dopus, ___flags) \
	AROS_LC5(Cfg_Filetype *,                                         \
			 EditFiletype,                                           \
			 AROS_LCA(Cfg_Filetype *, (___type), A0),                \
			 AROS_LCA(struct Window *, (___window), A1),             \
			 AROS_LCA(IPCData *, (___ipc), A2),                      \
			 AROS_LCA(IPCData *, (___dopus), A3),                    \
			 AROS_LCA(ULONG, (___flags), D0),                        \
			 struct Library *,                                       \
			 CONFIGOPUS_BASE_NAME,                                   \
			 14,                                                     \
			 /* s */)

#define FunctionExportASCII(___name, ___button, ___func, ___type) \
	AROS_LC4(BOOL,                                                \
			 FunctionExportASCII,                                 \
			 AROS_LCA(char *, (___name), A0),                     \
			 AROS_LCA(Cfg_Button *, (___button), A1),             \
			 AROS_LCA(Cfg_Function *, (___func), A2),             \
			 AROS_LCA(ULONG, (___type), D0),                      \
			 struct Library *,                                    \
			 CONFIGOPUS_BASE_NAME,                                \
			 17,                                                  \
			 /* s */)

#define ShowPaletteBox(___window, ___data, ___fgpen, ___bgpen, ___font, ___spec, ___pen) \
	AROS_LC7(long,                                                                       \
			 ShowPaletteBox,                                                             \
			 AROS_LCA(struct Window *, (___window), A0),                                 \
			 AROS_LCA(DOpusScreenData *, (___data), A1),                                 \
			 AROS_LCA(short *, (___fgpen), A2),                                          \
			 AROS_LCA(short *, (___bgpen), A3),                                          \
			 AROS_LCA(struct TextAttr *, (___font), A4),                                 \
			 AROS_LCA(ColourSpec32 *, (___spec), A5),                                    \
			 AROS_LCA(short *, (___pen), D0),                                            \
			 struct Library *,                                                           \
			 CONFIGOPUS_BASE_NAME,                                                       \
			 16,                                                                         \
			 /* s */)

#endif /* !_INLINE_CONFIGOPUS_H */
