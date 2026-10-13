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

#include "dopus.h"

// Stack settings
#if defined(__amigaos4__)
// Stack cookie for OS4
STATIC CONST_STRPTR USED stack = "\0$STACK:" STACK_STRING "\0";
#else
// Stack size for libnix
USED_VAR unsigned long __stack = STACK_LARGE;
#endif

// SAS Detach information
#if defined(__amigaos3__)
long __priority = 0;
long __BackGroundIO = 0;
char *__procname = "dopus";
#endif

// os4 ifaces
#ifdef __amigaos4__
struct DOpusIFace *IDOpus = NULL;
/*struct ConfigOpusIFace	*IConfigOpus	= NULL;
struct ModuleIFace		*IModule 		= NULL;

struct SysInfoIFace		*ISysInfo 		= NULL;*/
struct muIFace *Imu = NULL;

struct UtilityIFace *IUtility = NULL;
struct LocaleIFace *ILocale = NULL;
struct CommoditiesIFace *ICommodities = NULL;
struct ConsoleIFace *IConsole = NULL;
struct GraphicsIFace *IGraphics = NULL;
struct P96IFace *IP96 = NULL;
struct CyberGfxIFace *ICyberGfx = NULL;
struct IntuitionIFace *IIntuition = NULL;
struct InputIFace *IInput = NULL;
struct GadToolsIFace *IGadTools = NULL;
struct AslIFace *IAsl = NULL;
struct LayersIFace *ILayers = NULL;
struct DiskfontIFace *IDiskfont = NULL;
struct IconIFace *IIcon = NULL;
struct WorkbenchIFace *IWorkbench = NULL;
struct DataTypesIFace *IDataTypes = NULL;
struct RexxSysIFace *IRexxSys = NULL;
struct NewIconIFace *INewIcon = NULL;
struct TimerIFace *ITimer = NULL;
struct AmigaGuideIFace *IAmigaGuide = NULL;
#endif

// bases
#if defined(__amigaos4__) || defined(__AROS__)
struct UtilityBase *UtilityBase = NULL;
#else
struct Library *UtilityBase = NULL;
#endif
struct IntuitionBase *IntuitionBase = NULL;
struct GfxBase *GfxBase = NULL;
struct Library *DOpusBase = NULL;
struct Library *IconBase = NULL;
struct Library *DiskfontBase = NULL;
struct Library *LayersBase = NULL;
struct Library *WorkbenchBase = NULL;
struct Library *CxBase = NULL;
#if defined(__amigaos3__)
struct Library *ConfigOpusBase = NULL;
#endif
#ifdef __AROS__
struct RxsLib *RexxSysBase = NULL;
#elif !defined(__amigaos3__)
struct Library *RexxSysBase = NULL;
#endif
struct Library *GadToolsBase = NULL;
struct Library *AslBase = NULL;
struct Library *DataTypesBase = NULL;
#if defined(__amigaos3__) || defined(__AROS__) || defined(__amigaos4__)
struct Device *InputBase = NULL;
#else
struct Library *InputBase = NULL;
#endif

#ifdef __MORPHOS__
struct Library *TimerBase = NULL;
#else
struct Device *TimerBase = NULL;
#endif

#ifdef __MORPHOS__
struct Library *ConsoleDevice = NULL;
#else
struct Device *ConsoleDevice = NULL;
#endif
struct Library *AmigaGuideBase = NULL;
struct Library *P96Base = NULL;
/* Kept alongside P96Base: backdrop_render.c on MorphOS still uses
 * BltBitMapRastPortAlpha / ProcessPixelArray, which live in
 * cybergraphics.library (no P96 equivalent). */
struct Library *CyberGfxBase = NULL;
struct NewIconBase *NewIconBase = NULL;
struct Library *muBase = NULL;

//-----

struct Process *main_proc;		   // Main process pointer
IPCData main_ipc;				   // Main IPC data
APTR global_memory_pool = 0;	   // Global memory pool for anything to use
char *str_space_string;			   // A string of spaces
struct DOpusLocale locale;		   // Locale data
APTR main_status = 0;			   // Main status window
Cfg_Environment *environment = 0;  // Environment data
GUI_Glue *GUI = 0;				   // GUI data
BPTR old_current_dir = 0;		   // Old current directory
struct IOStdReq input_req;
struct IOStdReq timer_req;
struct IOStdReq console_req;

struct DOpusSemaphore pub_semaphore;

long main_lister_count = 0;

// Internal function definitions
Cfg_Function *def_function_leaveout = 0;
Cfg_Function *def_function_iconinfo = 0;
Cfg_Function *def_function_format = 0;
Cfg_Function *def_function_diskcopy = 0;
Cfg_Function *def_function_select = 0;
Cfg_Function *def_function_diskinfo = 0;
Cfg_Function *def_function_devicelist = 0;
Cfg_Function *def_function_cachelist = 0;
Cfg_Function *def_function_all = 0;
Cfg_Function *def_function_rename = 0;
Cfg_Function *def_function_delete = 0;
Cfg_Function *def_function_delete_quiet = 0;
Cfg_Function *def_function_makedir = 0;
Cfg_Function *def_function_makefile = 0;
Cfg_Function *def_function_copy = 0;
Cfg_Function *def_function_move = 0;
Cfg_Function *def_function_assign = 0;
Cfg_Function *def_function_configure = 0;
Cfg_Function *def_function_cli = 0;
Cfg_Function *def_function_devicelist_full = 0;
Cfg_Function *def_function_devicelist_brief = 0;
Cfg_Function *def_function_loadtheme = 0;
Cfg_Function *def_function_savetheme = 0;
Cfg_Function *def_function_buildtheme = 0;
Cfg_Filetype *default_filetype = 0;
Cfg_Filetype *run_filetype = 0;
Cfg_Filetype *command_filetype = 0;

char *config_name = "configopus.module";
char *dopus_name = "Directory Opus";

char *string_no_owner;
char *string_no_group;
char *string_empty;

// Shortcut arrow
UWORD arrow_hi_data[11][2] = {{0xffe0, 0x8020}, {0x8fa0, 0x87a0}, {0x8fa0, 0x9ea0},
							  {0xbc20, 0xb820}, {0xb820, 0x8020}, {0xffe0, 0x0000},
							  {0x7fc0, 0x7040}, {0x7840, 0x7040}, {0x6140, 0x43c0},
							  {0x47c0, 0x47c0}, {0x7fc0, 0x0000}},

	  arrow_lo_data[11][2] = {{0xfff8, 0x8008}, {0x8fe8, 0x87e8}, {0x8fe8, 0x9f68},
							  {0xbe08, 0xbc08}, {0xbc08, 0x8008}, {0xfff8, 0x0000},
							  {0x7ff0, 0x7010}, {0x7810, 0x7010}, {0x6090, 0x41f0},
							  {0x43f0, 0x43f0}, {0x7ff0, 0x0000}};

struct Image arrow_image[2] = {{0, 0, 11, 11, 2, (UWORD *)arrow_hi_data, 3, 0, 0}, {0, 0, 13, 11, 2, (UWORD *)arrow_lo_data, 3, 0, 0}};

// used in graphics.c
UWORD small_arrow[7] = {0x8000, 0xc000, 0xe000, 0xf000, 0xe000, 0xc000, 0x8000},
	  big_arrow[11] = {0x8000, 0xc000, 0xe000, 0xf000, 0xf800, 0xfc00, 0xf800, 0xf000, 0xe000, 0xc000, 0x8000};

#ifndef USE_SCREENTITLE
// Moon phases
const UWORD moon_big_data[8][2][13] =
	{{{0x0000, 0x0f80, 0x3fe0, 0x3fe0, 0x7ff0, 0x7ff0, 0x7ff0, 0x7ff0, 0x7ff0, 0x3fe0, 0x3fe0, 0x0f80, 0x0000},
	  {0xf078, 0xc018, 0x8008, 0x8008, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x8008, 0x8008, 0xc018, 0xf078}},

	 {{0x0000, 0x0e00, 0x3f00, 0x3f80, 0x7fc0, 0x7fc0, 0x7fc0, 0x7fc0, 0x7fc0, 0x3f80, 0x3f00, 0x0e00, 0x0000},
	  {0xf078, 0xc198, 0x80e8, 0x8068, 0x0030, 0x0030, 0x0030, 0x0030, 0x0030, 0x8068, 0x80e8, 0xc198, 0xf078}},

	 {{0x0000, 0x0c00, 0x3e00, 0x3e00, 0x7f00, 0x7f00, 0x7f00, 0x7f00, 0x7f00, 0x3e00, 0x3e00, 0x0c00, 0x0000},
	  {0xf078, 0xc398, 0x81e8, 0x81e8, 0x00f0, 0x00f0, 0x00f0, 0x00f0, 0x00f0, 0x81e8, 0x81e8, 0xc398, 0xf078}},

	 {{0x0000, 0x0000, 0x3000, 0x3000, 0x7800, 0x7800, 0x7800, 0x7800, 0x7800, 0x3000, 0x3000, 0x0000, 0x0000},
	  {0xf078, 0xcf98, 0x8fe8, 0x8fe8, 0x07f0, 0x07f0, 0x07f0, 0x07f0, 0x07f0, 0x8fe8, 0x8fe8, 0xcf98, 0xf078}},

	 {{0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
	  {0xf078, 0xcf98, 0xbfe8, 0xbfe8, 0x7ff0, 0x7ff0, 0x7ff0, 0x7ff0, 0x7ff0, 0xbfe8, 0xbfe8, 0xcf98, 0xf078}},

	 {{0x0000, 0x0000, 0x0060, 0x0060, 0x00f0, 0x00f0, 0x00f0, 0x00f0, 0x00f0, 0x0060, 0x0060, 0x0000, 0x0000},
	  {0xf078, 0xcf98, 0xbf88, 0xbf88, 0x7f00, 0x7f00, 0x7f00, 0x7f00, 0x7f00, 0xbf88, 0xbf88, 0xcf98, 0xf078}},

	 {{0x0000, 0x0180, 0x03e0, 0x03e0, 0x07f0, 0x07f0, 0x07f0, 0x07f0, 0x07f0, 0x03e0, 0x03e0, 0x0180, 0x0000},
	  {0xf078, 0xce18, 0xbc08, 0xbc08, 0x7800, 0x7800, 0x7800, 0x7800, 0x7800, 0xbc08, 0xbc08, 0xce18, 0xf078}},

	 {{0x0000, 0x0380, 0x07e0, 0x0fe0, 0x1ff0, 0x1ff0, 0x1ff0, 0x1ff0, 0x1ff0, 0x0fe0, 0x07e0, 0x0380, 0x0000},
	  {0xf078, 0xcc18, 0xb808, 0xb008, 0x6000, 0x6000, 0x6000, 0x6000, 0x6000, 0xb008, 0xb808, 0xcc18, 0xf078}}},

			moon_small_data[8][2][9] = {

				{{0x0000, 0x1c00, 0x3e00, 0x7f00, 0x7f00, 0x7f00, 0x3e00, 0x1c00, 0x0000},
				 {0xe3ff, 0x80ff, 0x80ff, 0x007f, 0x007f, 0x007f, 0x80ff, 0x80ff, 0xe3ff}},

				{{0x0000, 0x1800, 0x3c00, 0x7e00, 0x7e00, 0x7e00, 0x3c00, 0x1800, 0x0000},
				 {0xe3ff, 0x84ff, 0x82ff, 0x017f, 0x017f, 0x017f, 0x82ff, 0x84ff, 0xe3ff}},

				{{0x0000, 0x1000, 0x3800, 0x7c00, 0x7c00, 0x7c00, 0x3800, 0x1000, 0x0000},
				 {0xe3ff, 0x8cff, 0x86ff, 0x037f, 0x037f, 0x037f, 0x86ff, 0x8cff, 0xe3ff}},

				{{0x0000, 0x0000, 0x2000, 0x7000, 0x7000, 0x7000, 0x2000, 0x0000, 0x0000},
				 {0xe3ff, 0x9cff, 0x9eff, 0x0f7f, 0x0f7f, 0x0f7f, 0x9eff, 0x9cff, 0xe3ff}},

				{{0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
				 {0xe3ff, 0x9cff, 0xbeff, 0x7f7f, 0x7f7f, 0x7f7f, 0xbeff, 0x9cff, 0xe3ff}},

				{{0x0000, 0x0000, 0x0200, 0x0700, 0x0700, 0x0700, 0x0200, 0x0000, 0x0000},
				 {0xe3ff, 0x9cff, 0xbcff, 0x787f, 0x787f, 0x787f, 0xbcff, 0x9cff, 0xe3ff}},

				{{0x0000, 0x0400, 0x0e00, 0x1f00, 0x1f00, 0x1f00, 0x0e00, 0x0400, 0x0000},
				 {0xe3ff, 0x98ff, 0xb0ff, 0x607f, 0x607f, 0x607f, 0xb0ff, 0x98ff, 0xe3ff}},

				{{0x0000, 0x0c00, 0x1e00, 0x3f00, 0x3f00, 0x3f00, 0x1e00, 0x0c00, 0x0000},
				 {0xe3ff, 0x90ff, 0xa0ff, 0x407f, 0x407f, 0x407f, 0xa0ff, 0x90ff, 0xe3ff}}};
#endif

// Lister arrows
UWORD command_arrow[7] = {0x3c00, 0x3c00, 0x3c00, 0xff00, 0x7e00, 0x3c00, 0x1800},
	  parent_arrow[6] = {0x0e00, 0x3800, 0xe000, 0xe000, 0x3800, 0x0e00};

#ifdef __amigaos3__
// initialized in startup_init_icons
UWORD *arrow_hi_data_chip = NULL;
UWORD *arrow_lo_data_chip = NULL;
UWORD *small_arrow_chip = NULL;
UWORD *big_arrow_chip = NULL;
	#ifndef USE_SCREENTITLE
UWORD *moon_big_data_chip = NULL;
UWORD *moon_small_data_chip = NULL;
	#endif
UWORD *command_arrow_chip = NULL;
UWORD *parent_arrow_chip = NULL;
#endif
