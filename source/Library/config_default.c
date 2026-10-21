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

#include "dopuslib.h"
#include "config.h"
#include <Program/dopus_config.h>
#include "configflags.h"

static ULONG default_palette[] = {0xefffffff, 0xafffffff, 0x4fffffff, 0x7fffffff, 0x00000000, 0x7fffffff,
								  0xffffffff, 0xffffffff, 0x00000000, 0xcfffffff, 0x2fffffff, 0x00000000,
								  0xffffffff, 0x8fffffff, 0x00000000, 0xffffffff, 0x00000000, 0xffffffff,
								  0x9fffffff, 0x6fffffff, 0x3fffffff, 0x00000000, 0xffffffff, 0x9fffffff};

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

static void set_default_tooltip_colours(CFG_ENVR *env)
{
	env->tooltip_col[0] = 1;
	env->tooltip_col[1] = 0;
}

// Striping starts off. The stripe colour is a light grey custom colour;
// white is used if no pen can be had for it. The foreground pen is not used.
static void set_default_stripe_colours(CFG_ENVR *env)
{
	short a;

	env->env_flags &= ~ENVF_STRIPED_ROWS;
	env->stripe_col[0] = 1;
	env->stripe_col[1] = 2;
	for (a = 0; a < 3; a++)
	{
		env->env_Colours[ENVCOL_STRIPE][0][a] = 0;
		env->env_Colours[ENVCOL_STRIPE][1][a] = 0xbbbbbbbb;
	}
	env->env_ColourFlag &= ~(1 << ENVCOL_STRIPE);
	env->env_ColourFlag |= 1 << (ENVCOL_STRIPE + CUST_PENS);
}

#ifdef __AROS__
static void set_default_list_display(ListFormat *format)
{
	short a;
	static const BYTE default_display[] = {DISPLAY_NAME, DISPLAY_SIZE, DISPLAY_DATE, DISPLAY_PROTECT, DISPLAY_COMMENT};

	for (a = 0; a < ARRAY_SIZE(format->display_pos); a++)
		format->display_pos[a] = (a < ARRAY_SIZE(default_display)) ? default_display[a] : -1;
	for (a = 0; a < ARRAY_SIZE(format->display_len); a++)
		format->display_len[a] = -1;

	if (format->sort.sort < 0 || format->sort.sort >= DISPLAY_LAST)
		format->sort.sort = DISPLAY_NAME;
}
#endif

static void fix_list_format_display(ListFormat *format)
{
#ifdef __AROS__
	short a;
	short valid = 0;
	BOOL seen[DISPLAY_LAST];

	for (a = 0; a < ARRAY_SIZE(seen); a++)
		seen[a] = FALSE;

	for (a = 0; a < ARRAY_SIZE(format->display_pos); a++)
	{
		UBYTE display = (UBYTE)format->display_pos[a];

		if (display == 0xff)
			break;
		if (display >= DISPLAY_LAST || seen[display])
		{
			set_default_list_display(format);
			return;
		}

		seen[display] = TRUE;
		++valid;
	}

	if (valid == 0 || a == ARRAY_SIZE(format->display_pos))
		set_default_list_display(format);
#else
	(void)format;
#endif
}

#ifdef __AROS__
static void fix_list_pen_pair(UBYTE *pair, UBYTE foreground, UBYTE background)
{
	if (pair[0] == pair[1])
	{
		pair[0] = foreground;
		pair[1] = background;

		if (pair[0] == pair[1])
			pair[1] = (pair[0] == 0) ? 1 : 0;
	}
}
#endif

static void fix_list_format_colours(ListFormat *format, short old_environment)
{
#ifdef __AROS__
	BOOL bad_default_colours;

	if (old_environment && format->files_unsel[0] == 1 && format->files_unsel[1] == 2 &&
		format->dirs_unsel[0] == 3 && format->dirs_unsel[1] == 2)
	{
		format->files_unsel[1] = 0;
		format->dirs_unsel[1] = 0;
	}

	bad_default_colours = old_environment ||
		(format->files_unsel[0] == format->files_unsel[1] && format->files_sel[0] == format->files_sel[1] &&
		 format->dirs_unsel[0] == format->dirs_unsel[1] && format->dirs_sel[0] == format->dirs_sel[1]);

	if (bad_default_colours)
	{
		fix_list_pen_pair(format->files_unsel, 1, 0);
		fix_list_pen_pair(format->files_sel, 2, 1);
		fix_list_pen_pair(format->dirs_unsel, 3, 0);
		fix_list_pen_pair(format->dirs_sel, 2, 3);
	}
#else
	(void)format;
	(void)old_environment;
#endif
}

void LIBFUNC L_DefaultSettings(REG(a0, CFG_SETS *settings))
{
	// Set some default values
	settings->copy_flags = COPY_DATE | COPY_PROT | COPY_NOTE;
	settings->delete_flags = DELETE_ASK;
	settings->error_flags = ERROR_ENABLE_OPUS;
	settings->icon_flags = ICONFLAG_DOUNTOICONS;
	settings->replace_method = REPLACE_ASK;
	settings->update_flags = UPDATE_FREE | UPDATE_SCROLL | UPDATE_REDRAW;
	settings->dir_flags = DIRFLAGS_REREAD_CHANGED;
	settings->view_flags = VIEWBITS_SHOWBLACK | VIEWBITS_TEXTBORDERS;
	settings->max_buffer_count = 20;
	settings->date_format = FORMAT_DOS;
	settings->date_flags = DATE_SUBST | DATE_12HOUR;
	settings->hide_method = HIDE_CLOCK;
	settings->general_flags = GENERALF_MOUSE_HOOK;
	settings->replace_flags = REPLACEF_VERBOSE_REPLACE;
	settings->pri_main[0] = 0;
	settings->pri_main[1] = 0;
	settings->pri_lister[0] = 0;
	settings->pri_lister[1] = 0;
	settings->flags = 0;
	settings->popup_delay = 10;
	settings->max_openwith = 10;
	settings->max_filename = 30;
	settings->command_line_length = 512;
}

void LIBFUNC L_DefaultEnvironment(REG(a0, CFG_ENVR *env))
{
	short a;
	struct Screen *wbscreen;

	env->screen_mode = MODE_WORKBENCHCLONE;
	env->screen_flags = SCRFLAGS_DEFWIDTH | SCRFLAGS_DEFHEIGHT;
	env->screen_depth = 3;
	env->screen_width = (UWORD)-1;
	env->screen_height = (UWORD)-1;

	// Get Workbench screen
	if ((wbscreen = LockPubScreen(0)))
	{
		short num;

		// Number of colours
		num = 1 << wbscreen->RastPort.BitMap->Depth;

		// Get Workbench palette
		if (((struct Library *)GfxBase)->lib_Version >= 39)
		{
			short a;

			GetRGB32(wbscreen->ViewPort.ColorMap, 0, 4, env->palette + 1);
			GetRGB32(wbscreen->ViewPort.ColorMap, num - 4, 4, env->palette + 25);
			for (a = 0; a < 4; a++)
			{
				env->palette[13 + a * 3] = env->palette[25 + (3 - a) * 3];
				env->palette[14 + a * 3] = env->palette[26 + (3 - a) * 3];
				env->palette[15 + a * 3] = env->palette[27 + (3 - a) * 3];
			}
		}
		else
		{
			short a;
			unsigned short val;

			for (a = 0; a < 4; a++)
			{
				val = GetRGB4(wbscreen->ViewPort.ColorMap, a);
				env->palette[1 + a * 3] = ((val >> 8) & 0xf) << 28;
				env->palette[2 + a * 3] = ((val >> 4) & 0xf) << 28;
				env->palette[3 + a * 3] = (val & 0xf) << 28;
			}
			for (a = 0; a < 4; a++)
			{
				env->palette[13 + (3 - a) * 3] = env->palette[1 + a * 3];
				env->palette[14 + (3 - a) * 3] = env->palette[2 + a * 3];
				env->palette[15 + (3 - a) * 3] = env->palette[3 + a * 3];
			}
		}
		UnlockPubScreen(0, wbscreen);
	}

	// Copy default user colours
	CopyMem((char *)default_palette, (char *)(env->palette + 25), sizeof(default_palette));

	// Default window position
	env->window_pos.Left = (WORD)-1;
	env->window_pos.Top = (WORD)-1;
	env->window_pos.Width = (WORD)-1;
	env->window_pos.Height = (WORD)-1;

	env->display_options = DISPOPTF_SHOW_APPICONS | DISPOPTF_SHOW_TOOLS | DISPOPTF_ICON_POS |
						   DISPOPTF_USE_WBPATTERN | DISPOPTF_REALTIME_SCROLL | DISPOPTF_THIN_BORDERS;
	env->main_window_type = MAINWINDOW_ICONS;
	env->backdrop_prefs[0] = 0;
	for (a = 0; a < 4; a++)
	{
		env->env_BackgroundPic[a][0] = 0;
		env->env_BackgroundFlags[a] = ENVBF_PRECISION_IMAGE;
	}

	env->hotkey_code = (UWORD)~0;
	env->hotkey_qual = IEQUALIFIER_CONTROL | IEQUALIFIER_LSHIFT | IEQUALIFIER_LALT;

	env->list_format.files_unsel[0] = 1;
	env->list_format.files_unsel[1] = 0;
	env->list_format.files_sel[0] = 2;
	env->list_format.files_sel[1] = 1;
	env->list_format.dirs_unsel[0] = 3;
	env->list_format.dirs_unsel[1] = 0;
	env->list_format.dirs_sel[0] = 2;
	env->list_format.dirs_sel[1] = 3;
	env->list_format.sort.sort = 0;
	env->list_format.sort.separation = 1;
	env->list_format.display_pos[0] = ITEM_FILENAME;
	env->list_format.display_pos[1] = ITEM_FILESIZE;
	env->list_format.display_pos[2] = ITEM_DATE;
	env->list_format.display_pos[3] = ITEM_PROTECTION;
	env->list_format.display_pos[4] = ITEM_COMMENT;
	for (a = 5; a < 16; a++)
		env->list_format.display_pos[a] = -1;
	for (a = 0; a < ARRAY_SIZE(env->list_format.display_len); a++)
		env->list_format.display_len[a] = -1;
	env->list_format.show_free = SHOWFREE_KILO;
	fix_list_format_display(&env->list_format);
	fix_list_format_colours(&env->list_format, 0);

	env->source_col[0] = 1;
	env->source_col[1] = 253;
	env->dest_col[0] = 2;
	env->dest_col[1] = 252;
	env->devices_col[0] = 1;
	env->devices_col[1] = 2;
	env->volumes_col[0] = 3;
	env->volumes_col[1] = 2;
	env->palette_count = 0;

	strcpy(env->output_window, "20/10/600/180/Directory Opus Output");
	strcpy(env->output_device, "CON:");

	// Default font
	strcpy(env->font_name[FONT_DIRS], ((struct GfxBase *)GfxBase)->DefaultFont->tf_Message.mn_Node.ln_Name);
	env->font_size[FONT_DIRS] = ((struct GfxBase *)GfxBase)->DefaultFont->tf_YSize;
	strcpy(env->font_name[FONT_ICONS], "topaz.font");
	env->font_size[FONT_ICONS] = 8;
	strcpy(env->font_name[FONT_WINDOW_ICONS], "topaz.font");
	env->font_size[FONT_WINDOW_ICONS] = 8;
	env->icon_fpen = 1;
	env->icon_bpen = 0;
	env->icon_style = JAM2;
	env->iconw_fpen = 1;
	env->iconw_bpen = 0;
	env->iconw_style = JAM2;

	// Default status text
	strcpy(env->status_text, "D:[%ds/%dt] F:[%fs/%ft] B:[%bs/%bt] %h*%");

	env->lister_popup_code = 0xffff;
	env->lister_popup_qual = 0;

	env->lister_options = LISTEROPTF_2XCLICK | LISTEROPTF_TITLES | LISTEROPTF_POPUP | LISTEROPTF_EDIT_BOTH;
	env->lister_width = 320;
	env->lister_height = 200;

	// Gauge colour
	env->gauge_col[0] = 3;
	env->gauge_col[1] = 3;
	set_default_tooltip_colours(env);

	// Initialise custom colours
	for (a = 0; a < 16; a++)
	{
		short b;

		for (b = 0; b < 2; b++)
		{
			env->env_Colours[a][b][0] = 0;
			env->env_Colours[a][b][1] = 0;
			env->env_Colours[a][b][2] = 0;
		}
	}
	env->env_ColourFlag = 0;

	// Default stack
	env->default_stack = STACK_DEFAULT;

	// Desktop folder location
	strcpy(env->desktop_location, "DOpus5:Desktop/");
	env->desktop_popup_default = DESKTOP_POPUP_NONE;
	env->desktop_flags = DESKTOPF_NO_BORDERS;
	env->env_flags = ENVF_BACKDROP;
	set_default_stripe_colours(env);

	// NewIcons stuff
	env->env_NewIconsFlags = ENVNIF_ENABLE;
	env->env_NewIconsPrecision = 16;

	// Icon layout (matches CLEANUP_SPACE_X/Y in backdrop.h; grid 1 = no grid)
	env->env_icon_space_x = 7;
	env->env_icon_space_y = 5;
	env->env_icon_grid_x = 1;
	env->env_icon_grid_y = 1;

	// Mouse wheel scroll lines (matches the legacy startup default)
	env->env_wheel_scroll_lines = 3;

	// Title bar clock format
	env->clock_format[0] = 0;

	// Set version
	env->version = CONFIG_VERSION_18;

	// Get default settings
	L_DefaultSettings(&env->settings);
}

Cfg_ButtonBank *LIBFUNC L_DefaultButtonBank(void)
{
	Cfg_ButtonBank *bank;

	// Allocate button bank
	if (!(bank = L_NewButtonBank(1, 0)))
		return 0;

	// Initialise button bank
	strcpy(bank->window.name, "Default button bank");

	return bank;
}

void LIBFUNC L_UpdateEnvironment(REG(a0, CFG_ENVR *env))
{
	// Old-old-old version?
	if (env->version < CONFIG_VERSION_3)
	{
		short a;

		// Clear some things
		env->font_name[0][0] = 0;
		env->font_size[0] = 0;
		for (a = 0; a < ARRAY_SIZE(env->pad3); a++)
			env->pad3[a] = 0;
		for (a = 0; a < ARRAY_SIZE(env->pad4); a++)
			env->pad4[0] = 0;

		/* changed 8/11/99 gjp
				for (a=0;a<68;a++)
					env->pad6[0]=0;
		*/

		// Fix status text
		strcpy(env->status_text, "D:[%ds/%dt] F:[%fs/%ft] B:[%bs/%bt] %h*%");
	}

	// Old-old version?
	if (env->version < CONFIG_VERSION_2)
	{
		APTR iff;

		// Default to topaz
		strcpy(env->font_name[FONT_ICONS], "topaz.font");
		env->font_size[FONT_ICONS] = 8;
		env->icon_fpen = 1;
		env->icon_bpen = 0;
		env->icon_style = JAM2;

		// Try to open font prefs file
		if ((iff = L_IFFOpen("env:sys/font.prefs", IFF_READ, ID_PREF)))
		{
			// Look for FONT chunks
			while (L_IFFNextChunk(iff, ID_FONT))
			{
				struct FontPrefs font;
				BOOL got_font = FALSE;

#ifdef __AROS__
				// FONT chunks contain the 32-bit, 156-byte FontPrefs layout.
				UBYTE font_data[156];

				if (L_IFFChunkSize(iff) >= sizeof(font_data) &&
					L_IFFReadChunkBytes(iff, font_data, sizeof(font_data)) == sizeof(font_data))
				{
					memset(&font, 0, sizeof(font));
					font.fp_Type = ((UWORD)font_data[14] << 8) | font_data[15];
					font.fp_FrontPen = font_data[16];
					font.fp_BackPen = font_data[17];
					font.fp_DrawMode = font_data[18];
					font.fp_TextAttr.ta_YSize = ((UWORD)font_data[24] << 8) | font_data[25];
					CopyMem(font_data + 28, font.fp_Name, sizeof(font.fp_Name));
					got_font = TRUE;
				}
#else
				if (L_IFFReadChunkBytes(iff, &font, sizeof(font)))
					got_font = TRUE;
#endif
				// Workbench font?
				if (got_font && font.fp_Type == FP_WBFONT)
				{
					// Get icon font
					strcpy(env->font_name[FONT_ICONS], font.fp_Name);
					env->font_size[FONT_ICONS] = font.fp_TextAttr.ta_YSize;

					// Get icon text colours
					env->icon_fpen = font.fp_FrontPen;
					env->icon_bpen = font.fp_BackPen;
					env->icon_style = DOPUS_FONTPREFS_ICON_STYLE(&font);
					break;
				}
			}

			// Close iff file
			L_IFFClose(iff);
		}
	}

	// Pre-Version 4?
	if (env->version < CONFIG_VERSION_4)
	{
		// Check output device
		strcpy(env->output_device, "CON:");
	}

	// Pre-version 5
	if (env->version < CONFIG_VERSION_5)
	{
		short a;

		// Fix windows icon font
		strcpy(env->font_name[FONT_WINDOW_ICONS], env->font_name[FONT_ICONS]);
		env->font_size[FONT_WINDOW_ICONS] = env->font_size[FONT_ICONS];
		env->iconw_fpen = env->icon_fpen;
		env->iconw_bpen = env->icon_bpen;
		env->iconw_style = env->icon_style;

		// Gauge colour
		env->gauge_col[0] = 3;
		env->gauge_col[1] = 3;

		// Initialise custom colours
		for (a = 0; a < 16; a++)
		{
			short b;

			for (b = 0; b < 2; b++)
			{
				env->env_Colours[a][b][0] = 0;
				env->env_Colours[a][b][1] = 0;
				env->env_Colours[a][b][2] = 0;
			}
		}
		env->env_ColourFlag = 0;
	}

	// Pre-version 6
	if (env->version < CONFIG_VERSION_6)
	{
		// Default stack
		env->default_stack = STACK_DEFAULT;
	}

	// Pre-version 7
	if (env->version < CONFIG_VERSION_7)
	{
		// Desktop folder location
		strcpy(env->desktop_location, "DOpus5:Desktop/");
		env->desktop_popup_default = DESKTOP_POPUP_NONE;
	}

	// Pre-version 8
	if (env->version < CONFIG_VERSION_8)
	{
		// NewIcons stuff
		env->env_NewIconsFlags = ENVNIF_ENABLE;
		env->env_NewIconsPrecision = 16;
	}

	// Pre-version 9
	if (env->version < CONFIG_VERSION_9)
	{
		short a;

		// Set 'use wbpattern' flag
		env->display_options |= DISPOPTF_USE_WBPATTERN;

		// Initialise picture fields
		for (a = 0; a < 4; a++)
		{
			env->env_BackgroundPic[a][0] = 0;
			env->env_BackgroundFlags[a] = ENVBF_PRECISION_IMAGE;
		}
	}

	// Pre-version 10
	if (env->version < CONFIG_VERSION_10)
	{
		// New flag for fuel gauge default
		if (env->lister_options & LISTEROPTF_FUEL_GAUGE)
			env->list_format.flags |= LFORMATF_GAUGE;
	}

	// Pre-version 11
	if (env->version < CONFIG_VERSION_11)
	{
		char buf[10];

		// Environment variables turned into config options
		if (GetVar("dopus/PopUpDelay", buf, sizeof(buf), GVF_GLOBAL_ONLY) > 0)
		{
			if ((env->settings.popup_delay = atoi(buf)) < 1)
				env->settings.popup_delay = 1;
		}
		else
			env->settings.popup_delay = 10;
		env->settings.max_openwith = 10;
		if (GetVar("dopus/CommandLineLength", buf, sizeof(buf), GVF_GLOBAL_ONLY) > 0)
		{
			if ((env->settings.command_line_length = atoi(buf)) < 256)
				env->settings.command_line_length = 256;
		}
		env->settings.command_line_length = 512;
	}

	// Pre-version 12
	if (env->version < CONFIG_VERSION_12)
	{
		env->settings.max_filename = 30;
	}

	// Pre-version 13: import legacy ENV: flags into the config so users see
	// their previous behaviour after upgrading. Empty values count as "off".
	if (env->version < CONFIG_VERSION_13)
	{
		char buf[4];

		if (GetVar("dopus/HidePadlock", buf, sizeof(buf), GVF_GLOBAL_ONLY) > 0 && buf[0] != '0')
			env->lister_options |= LISTEROPTF_NO_PADLOCK;

		if (GetVar("dopus/WorkbenchTitle", buf, sizeof(buf), GVF_GLOBAL_ONLY) > 0 && buf[0] != '0')
			env->display_options |= DISPOPTF_WB_TITLE;

		// Note: legacy UseWBInfo used `> 0` (any value enables) -- preserve that semantics
		if (GetVar("dopus/UseWBInfo", buf, sizeof(buf), GVF_GLOBAL_ONLY) > 0)
			env->display_options |= DISPOPTF_USE_WBINFO;
	}

	// Pre-version 14: import desktop icon-layout vars and ReturnOfBenify into config.
	// Old configs have these fields zeroed, so unconditionally fall back to defaults.
	if (env->version < CONFIG_VERSION_14)
	{
		char buf[14];

		// Legacy semantics: an explicit env value clamps to a minimum of 1;
		// only an absent env var falls back to the historical defaults (7/5).
		// (atoi() can return a negative number for "-3"; clamp BEFORE the
		// UWORD cast or the signedness flip would store 65533.)
		if (GetVar("dopus/IconSpaceX", buf, sizeof(buf), GVF_GLOBAL_ONLY) > 0)
		{
			int v = atoi(buf);
			if (v < 1)
				v = 1;
			env->env_icon_space_x = (UWORD)v;
		}
		if (env->env_icon_space_x < 1)
			env->env_icon_space_x = 7;

		if (GetVar("dopus/IconSpaceY", buf, sizeof(buf), GVF_GLOBAL_ONLY) > 0)
		{
			int v = atoi(buf);
			if (v < 1)
				v = 1;
			env->env_icon_space_y = (UWORD)v;
		}
		if (env->env_icon_space_y < 1)
			env->env_icon_space_y = 5;

		if (GetVar("dopus/IconGridX", buf, sizeof(buf), GVF_GLOBAL_ONLY) > 0)
		{
			int v = atoi(buf);
			if (v < 1)
				v = 1;
			env->env_icon_grid_x = (UWORD)v;
		}
		if (env->env_icon_grid_x < 1)
			env->env_icon_grid_x = 1;

		if (GetVar("dopus/IconGridY", buf, sizeof(buf), GVF_GLOBAL_ONLY) > 0)
		{
			int v = atoi(buf);
			if (v < 1)
				v = 1;
			env->env_icon_grid_y = (UWORD)v;
		}
		if (env->env_icon_grid_y < 1)
			env->env_icon_grid_y = 1;

		if (GetVar("dopus/ReturnOfBenify", buf, sizeof(buf), GVF_GLOBAL_ONLY) > 0 && buf[0] != '0')
			env->env_flags |= ENVF_BENIFY;
	}

	// Pre-version 15: import WheelScrollLines, EnableShortcuts and ShowUseDatatypesFirst.
	if (env->version < CONFIG_VERSION_15)
	{
		char buf[14];

		if (GetVar("dopus/WheelScrollLines", buf, sizeof(buf), GVF_GLOBAL_ONLY) > 0)
		{
			int v = atoi(buf);
			if (v < 1)
				v = 1;
			env->env_wheel_scroll_lines = (UWORD)v;
		}
		if (env->env_wheel_scroll_lines < 1)
			env->env_wheel_scroll_lines = 3;

		if (GetVar("dopus/EnableShortcuts", buf, sizeof(buf), GVF_GLOBAL_ONLY) > 0 && buf[0] != '0')
			env->env_flags |= ENVF_ENABLE_SHORTCUTS;

		// Legacy ShowUseDatatypesFirst used `> 0` (any value enables) -- preserve that
		if (GetVar("dopus/ShowUseDatatypesFirst", buf, sizeof(buf), GVF_GLOBAL_ONLY) > 0)
			env->display_options |= DISPOPTF_SHOW_DATATYPES_FIRST;
	}

	// Pre-version 16
	if (env->version < CONFIG_VERSION_16)
		set_default_tooltip_colours(env);

	// Pre-version 17
	if (env->version < CONFIG_VERSION_17)
		env->clock_format[0] = 0;

	// Pre-version 18
	if (env->version < CONFIG_VERSION_18)
		set_default_stripe_colours(env);

	fix_list_format_display(&env->list_format);
	fix_list_format_colours(&env->list_format, env->version < CONFIG_VERSION_12);

	// Fix version
	env->version = CONFIG_VERSION_18;

	// Is themes path empty?
	if (!env->themes_location[0])
		strcpy(env->themes_location, "DOpus5:Themes/");
}
