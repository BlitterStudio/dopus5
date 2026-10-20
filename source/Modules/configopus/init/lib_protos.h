
#ifndef _LIB_PROTOS_H
#define _LIB_PROTOS_H

#include <dopus/common.h>
#include <proto/dopus5.h>
#include <proto/module.h>

/* init.c */
ULONG freeBase(struct LibraryHeader *lib);
ULONG initBase(struct LibraryHeader *lib);

// protos:

LIBPROTO(L_Config_Settings,
		 int,
		 REG(a0, Cfg_Environment *env),
		 REG(a1, struct Screen *screen),
		 REG(a2, IPCData *ipc),
		 REG(a3, IPCData *main_ipc),
		 REG(d0, char *settings_name));

LIBPROTO(L_Config_Environment,
		 unsigned long,
		 REG(a0, Cfg_Environment *env),
		 REG(a1, struct Screen *screen),
		 REG(a2, UWORD *pen_table),
		 REG(a3, IPCData *ipc),
         REG(a4, IPCData *main_ipc),
         REG(d0, UWORD pen_alloc),
         REG(d1, ULONG *change_flags),
         REG(d2, char *settings_name),
         REG(d3, Att_List *script_list));

LIBPROTO(L_Config_Buttons,
		 int,
		 REG(a0, ButtonsStartup *startup),
		 REG(a1, IPCData *ipc),
		 REG(a2, IPCData *owner_ipc),
		 REG(a3, struct Screen *screen),
		 REG(d0, IPTR command_list));

LIBPROTO(L_ConvertConfig, BOOL, REG(a0, char *name), REG(a1, struct Screen *parent), REG(a2, IPCData *owner_ipc));

LIBPROTO(L_Config_Filetypes,
		 short,
		 REG(a0, struct Screen *screen),
		 REG(a1, IPCData *ipc),
		 REG(a2, IPCData *owner_ipc),
		 REG(d0, IPTR command_list),
		 REG(a3, char *name));

LIBPROTO(L_Config_ListerButtons,
		 ULONG,
		 REG(a0, char *bank_name),
		 REG(a1, IPCData *ipc),
         REG(a2, IPCData *owner_ipc),
         REG(a3, struct Screen *screen),
         REG(d0, Cfg_ButtonBank *def_bank),
         REG(d1, IPTR command_list),
         REG(d2, LONG initial));

LIBPROTO(L_Config_Menu,
		 ULONG,
		 REG(a0, char *menu_name),
		 REG(a1, IPCData *ipc),
		 REG(a2, IPCData *owner_ipc),
         REG(a3, struct Screen *screen),
         REG(d0, Cfg_ButtonBank *def_bank),
         REG(d1, char *title),
         REG(d2, IPTR command_list),
         REG(d3, char *default_name),
         REG(d4, short type),
         REG(d5, Att_List *script_list));

LIBPROTO(L_Config_Menus,
		 IPTR,
		 REG(a0, IPCData *ipc),
		 REG(a1, IPCData *owner_ipc),
		 REG(a2, struct Screen *screen),
		 REG(a3, Cfg_ButtonBank *def_bank),
		 REG(d0, IPTR command_list),
		 REG(d1, ULONG type),
		 REG(d2, char *menu_path));

LIBPROTO(L_EditFiletype,
		 Cfg_Filetype *,
		 REG(a0, Cfg_Filetype *type),
		 REG(a1, struct Window *window),
		 REG(a2, IPCData *ipc),
		 REG(a3, IPCData *dopus_ipc),
		 REG(d0, ULONG flags));

LIBPROTO(L_Config_EditFunction,
		 Cfg_Function *,
		 REG(a0, IPCData *ipc),
		 REG(a1, IPCData *opus_ipc),
		 REG(a2, struct Window *window),
		 REG(a3, Cfg_Function *function),
         REG(d0, APTR memory),
         REG(d1, IPTR command_list));

LIBPROTO(L_ShowPaletteBox,
		 long,
		 REG(a0, struct Window *parent),
		 REG(a1, DOpusScreenData *screen_data),
		 REG(a2, short *fgpen),
		 REG(a3, short *bgpen),
         REG(d0, struct TextAttr *font),
         REG(d1, ColourSpec32 *spec),
         REG(d2, short *spec_pen));

LIBPROTO(L_FunctionExportASCII,
		 BOOL,
		 REG(a0, char *filename),
		 REG(a1, Cfg_Button *button),
		 REG(a2, Cfg_Function *function),
		 REG(d0, ULONG a4));

#endif /* _LIB_PROTOS_H */
