#include "config_lib.h"
#include "config_environment.h"
#include <proto/module.h>
#include <prefs/palette.h>

enum
{
	PALMENU_WORKBENCH,
	PALMENU_WB31,
	PALMENU_MAGICWB,
};

// Preset colours as 0xRRGGBB in screen pen order; 4-7 are the last four pens
static const ULONG palette_presets[2][8] = {
	// Workbench 3.1
	{0xaaaaaa, 0x000000, 0xffffff, 0x6688bb, 0xee4444, 0x55dd55, 0x0044dd, 0xee9900},
	// MagicWB
	{0x959595, 0x000000, 0xffffff, 0x3b67a2, 0x7b7b7b, 0xafafaf, 0xaa907c, 0xffa997}};

// Update palette sliders
void _config_env_palette_update_sliders(config_env_data *data)
{
	short a;

	// Bits per gun defaults to 4
	data->gun_bits[0] = 4;
	data->gun_bits[1] = 4;
	data->gun_bits[2] = 4;

	// If v39, ask display database for bpg
	if (((struct Library *)GfxBase)->lib_Version >= 39)
	{
		struct DisplayInfo dispinfo;

		// Get bits per gun from environment database
		if (GetDisplayInfoData(0,
							   (char *)&dispinfo,
							   sizeof(struct DisplayInfo),
							   DTAG_DISP,
							   GetVPModeID(&data->window->WScreen->ViewPort)))
		{
			data->gun_bits[0] = dispinfo.RedBits;
			data->gun_bits[1] = dispinfo.GreenBits;
			data->gun_bits[2] = dispinfo.BlueBits;
		}
	}

	// Fix sliders
	for (a = 0; a < 3; a++)
	{
		unsigned short max;

		// Maximum value
		max = (1 << data->gun_bits[a]) - 1;

		// Set limit
		SetGadgetChoices(data->option_list, GAD_ENVIRONMENT_PALETTE_RED + a, (APTR)(IPTR)((IPTR)max << 16));

		// Set current level
		SetGadgetValue(data->option_list,
					   GAD_ENVIRONMENT_PALETTE_RED + a,
					   data->config->palette[data->sel_colour * 3 + a + 1] >> (32 - data->gun_bits[a]));
	}
}

// Change colour
void _config_env_palette_change_colour(config_env_data *data, int gun, ULONG val)
{
	// Set new value
	data->config->palette[data->sel_colour * 3 + gun + 1] = (val << (32 - data->gun_bits[gun]));

	// Load colour register
	set_rgb32(data, data->sel_colour, data->config->palette + (data->sel_colour * 3) + 1);

	// A truecolour screen doesn't show the new colour until the square is redrawn
	_config_env_palette_redraw(data, data->orig_colour_sel);
}

// Redraw one palette square, or all of them if pen is -1
void _config_env_palette_redraw(config_env_data *data, short pen)
{
	// Changing a pen only shows by itself on a colour-mapped screen
	if (!data->cyber_flag || !data->palette_gadget)
		return;

	if (pen > -1)
		SetGadgetAttrs(GADGET(data->palette_gadget), data->window, 0, DPG_Redraw, pen, TAG_END);
	else
		RefreshGList(GADGET(data->palette_gadget), data->window, 0, 1);
}

// Set the eight OS colours from 32-bit RGB triplets in screen pen order
static void _config_env_palette_os_colours(config_env_data *data, const ULONG *rgb)
{
	ULONG *palette;
	short pen;

	// Get palette pointer
	palette = data->config->palette + 1;

	// Go through pens
	for (pen = 0; pen < 8; pen++, palette += 3)
	{
		const ULONG *col;

		// The config stores the top four in reverse order
		col = rgb + ((pen < 4) ? pen : (11 - pen)) * 3;

		// Store in config
		palette[0] = col[0];
		palette[1] = col[1];
		palette[2] = col[2];

		// On own screen and deep enough, set colour
		if (data->own_screen && (pen < 4 || data->own_screen > 4))
			set_rgb32(data, pen, palette);
	}

	// Show the new colours
	_config_env_palette_redraw(data, -1);
}

// Copy the Workbench screen's OS colours
static void _config_env_palette_grab_workbench(config_env_data *data)
{
	struct Screen *screen;
	ULONG rgb[24];
	short depth, num, count, a, gun;

	// Get Workbench
	if (!(screen = LockPubScreen("Workbench")))
		return;

	// RTG screens can report more than 8 planes; the OS pens are within 256
	if ((depth = screen->RastPort.BitMap->Depth) > 8)
		depth = 8;

	// A one-plane screen has only two colours
	num = 1 << depth;
	count = (num < 4) ? num : 4;

	// Get the first four
	if (((struct Library *)GfxBase)->lib_Version >= 39)
		GetRGB32(screen->ViewPort.ColorMap, 0, count, rgb);
	else
	{
		for (a = 0; a < count; a++)
		{
			UWORD val = GetRGB4(screen->ViewPort.ColorMap, a);

			rgb[a * 3] = ((val >> 8) & 0xf) * 0x11111111;
			rgb[a * 3 + 1] = ((val >> 4) & 0xf) * 0x11111111;
			rgb[a * 3 + 2] = (val & 0xf) * 0x11111111;
		}
	}

	// Fewer than four colours; repeat the last one
	for (a = count; a < 4; a++)
		for (gun = 0; gun < 3; gun++)
			rgb[a * 3 + gun] = rgb[(count - 1) * 3 + gun];

	// Get the last four; under 39, or with no more than four colours, use the first four
	if (((struct Library *)GfxBase)->lib_Version >= 39 && num > 4)
		GetRGB32(screen->ViewPort.ColorMap, num - 4, 4, rgb + 12);
	else
		CopyMem((char *)rgb, (char *)(rgb + 12), sizeof(ULONG) * 12);

	// Unlock Workbench
	UnlockPubScreen(0, screen);

	// Use the colours
	_config_env_palette_os_colours(data, rgb);
}

// Set the OS colours to a preset
static void _config_env_palette_set_preset(config_env_data *data, const ULONG *preset)
{
	ULONG rgb[24];
	short a;

	// Expand to 32 bits per gun
	for (a = 0; a < 8; a++)
	{
		rgb[a * 3] = ((preset[a] >> 16) & 0xff) * 0x01010101;
		rgb[a * 3 + 1] = ((preset[a] >> 8) & 0xff) * 0x01010101;
		rgb[a * 3 + 2] = (preset[a] & 0xff) * 0x01010101;
	}

	_config_env_palette_os_colours(data, rgb);
}

// Show the palette presets menu
void _config_env_palette_presets(config_env_data *data)
{
	PopUpHandle *menu;
	short res;

	// Create menu
	if (!(menu = PopUpNewHandle(0, 0, locale)))
		return;

	PopUpNewItem(menu, MSG_ENVIRONMENT_PALETTE_WORKBENCH, PALMENU_WORKBENCH, 0);
	PopUpSeparator(menu);
	PopUpNewItem(menu, MSG_ENVIRONMENT_PALETTE_WB31, PALMENU_WB31, 0);
	PopUpNewItem(menu, MSG_ENVIRONMENT_PALETTE_MAGICWB, PALMENU_MAGICWB, 0);

	// Do popup menu
	if ((res = DoPopUpMenu(data->window, &menu->ph_Menu, 0, SELECTDOWN)) >= 0)
	{
		if (res == PALMENU_WORKBENCH)
			_config_env_palette_grab_workbench(data);
		else
			_config_env_palette_set_preset(data, palette_presets[res - PALMENU_WB31]);
	}

	// Free menu
	PopUpFreeHandle(menu);
}

void set_rgb32(config_env_data *data, short col, ULONG *palette)
{
	short reg;

	// Not on own screen?
	if (!data->own_screen)
	{
		if (col > 7)
			col -= 8;
	}

	// Otherwise, fix top four mapping
	else if (((struct Library *)GfxBase)->lib_Version >= 39 && data->own_screen > 4)
	{
		if (col >= 4 && col < 8)
			col = 7 - (col - 4);
	}

	// Get register to use
	if ((reg = data->palette_edit_pens[col]) == -1 || !(data->palette_edit_penalloc & (1 << col)))
		return;
	if (((struct Library *)GfxBase)->lib_Version >= 39)
	{
		SetRGB32(&data->window->WScreen->ViewPort, reg, palette[0], palette[1], palette[2]);
	}
	else
	{
		SetRGB4(&data->window->WScreen->ViewPort,
				reg,
				(palette[0] >> 28) & 0xf,
				(palette[1] >> 28) & 0xf,
				(palette[2] >> 28) & 0xf);
	}
}

// Update lister colour example
void _config_env_update_example(config_env_data *data, UBYTE *ptr)
{
	short fg, bg, base;

	// Get pens
	fg = ptr[0];
	bg = ptr[1];

	// Under 39 there are 4 extra pens
	if (((struct Library *)GfxBase)->lib_Version >= 39 && data->window->RPort->BitMap->Depth > 2)
		base = 4;
	else
		base = 0;

	// Get colour maps
	if (fg >= 4 && fg < 252)
		fg = data->palette_array[fg + base];
	if (bg >= 4 && bg < 252)
		bg = data->palette_array[bg + base];

	// Use custom pens?
	if (((struct Library *)GfxBase)->lib_Version >= 39)
	{
		// Foreground?
		if (data->config->env_ColourFlag & (1 << data->colour_number) && data->custom_pen[0] > -1)
			fg = data->custom_pen[0];

		// Background?
		if (data->config->env_ColourFlag & (1 << (data->colour_number + CUST_PENS)) && data->custom_pen[1] > -1)
			bg = data->custom_pen[1];
	}

	// Update example
	DisplayObject(data->window, data->example, fg, bg, 0);
}

// Initialise palette gadget
void _config_env_palette_init(config_env_data *data)
{
	short count;

	// Initialise colour information
	_config_env_colours_init(data);

	// Get number of colours
	count = data->palette_colours;
	if (data->own_screen > 4)
		count += 8;
	else if (data->own_screen)
		count += 4;

	// Set number of colours and palette table
	SetGadgetAttrs(GADGET(data->palette_gadget),
				   data->window,
				   0,
				   GTPA_NumColors,
				   count,
				   GTPA_ColorTable,
				   data->palette_edit_pens,
				   TAG_END);

	// Disable sliders if no colours
	DisableObject(data->option_list, GAD_ENVIRONMENT_PALETTE_RED, (count < 1));
	DisableObject(data->option_list, GAD_ENVIRONMENT_PALETTE_GREEN, (count < 1));
	DisableObject(data->option_list, GAD_ENVIRONMENT_PALETTE_BLUE, (count < 1));

	// Update sliders
	_config_env_palette_update_sliders(data);

	// Under 37, set user colours to maximum and disable slider
	if (((struct Library *)GfxBase)->lib_Version < 39)
	{
		unsigned short max;

		if (data->own_screen > 4)
		{
			max = data->own_screen - 4;
			if (max > 8)
				max = 8;
		}
		else
			max = 0;
		SetGadgetChoices(data->option_list, GAD_ENVIRONMENT_PALETTE_COUNT, (APTR)(IPTR)(max << 16));
		DisableObject(data->option_list, GAD_ENVIRONMENT_PALETTE_COUNT, TRUE);
	}

	// Under 39, set to maximum
	else
	{
		short max;

		max = (1 << data->window->WScreen->RastPort.BitMap->Depth) - 8;
		if (max < 0)
			max = 0;
		if (max > 8)
			max = 8;
		SetGadgetChoices(data->option_list, GAD_ENVIRONMENT_PALETTE_COUNT, (APTR)(IPTR)(max << 16));
		if (max == 0)
			DisableObject(data->option_list, GAD_ENVIRONMENT_PALETTE_COUNT, TRUE);
	}
}

// Initialise colours
void _config_env_colours_init(config_env_data *data)
{
	short a, b, base = 0;
	short num;

	// Get number of palette colours
	data->palette_colours = (data->palette_table) ? data->config->palette_count : 0;
	if (data->palette_colours > 8)
		data->palette_colours = 8;

	// Number of screen colours
	num = 1 << data->window->RPort->BitMap->Depth;

	// Under 39?
	if (((struct Library *)GfxBase)->lib_Version >= 39)
	{
		// Check we have room for pens
		if (data->palette_colours > num - 8)
			data->palette_colours = num - 8;
		if (data->palette_colours < 0)
			data->palette_colours = 0;
	}

	// Earlier
	else
	{
		// On Workbench we can't have pens
		if (!data->own_screen)
			data->palette_colours = 0;

		// On our own screen
		else
		{
			// Check we have room for pens
			if (data->palette_colours > num - 8)
				data->palette_colours = num - 8;
			if (data->palette_colours < 0)
				data->palette_colours = 0;
		}
	}

	// Number of colours including OS pens
	data->palette_count = 4 + data->palette_colours;

	// Initialise pen array
	for (a = 4; a < 16; a++)
		data->palette_array[a] = -1;

	// Initialise palette array; OS colours first
	for (a = 0; a < 4; a++)
		data->palette_array[a] = a;

	// Under 39 we have another 4 OS pens
	if (((struct Library *)GfxBase)->lib_Version >= 39 && data->window->RPort->BitMap->Depth > 2)
	{
		// OS colours
		for (; a < 8; a++)
			data->palette_array[a] = 248 + a;
		data->palette_count += 4;
	}

	// Get base
	if (data->own_screen > 4)
		base = 8;
	else if (data->own_screen)
		base = 4;

	// User colours
	for (b = 0; b < data->palette_colours; b++, a++)
		data->palette_array[a] = data->palette_edit_pens[base + b];
}

// Reset palette
void _config_env_reset_palette(config_env_data *data)
{
	short col = 0, num;

	// OS colours
	if (data->own_screen)
	{
		// Bottom four
		for (col = 0; col < 4; col++)
			set_rgb32(data, col, data->env->env->palette + (col * 3) + 1);

		// Top four?
		if (data->own_screen > 4)
		{
			for (; col < 8; col++)
			{
				set_rgb32(data, col, data->env->env->palette + (col * 3) + 1);
				//					data->env->env->palette+((4+(7-col))*3)+1);
			}
		}
	}

	// Load palette
	for (num = 0; num < data->palette_colours; num++, col++)
		set_rgb32(data, col, data->env->env->palette + ((num + 8) * 3) + 1);
}

// Load palette file
void _config_env_load_palette(config_env_data *data)
{
	char buf[256];
	struct FileRequester *req;

	// Make window busy
	SetWindowBusy(data->window);

	// Get requester pointer
	req = WINREQUESTER(data->window);

	// Show requester
	char path3[256];
	BPTR lock3;
	path3[0] = 0;
	lock3 = Lock("sys:prefs/presets", ACCESS_READ);
	if (lock3)
	{
		if (!(NameFromLock(lock3, path3, sizeof(path3))))
			path3[0] = 0;
		UnLock(lock3);
	}
	if (AslRequestTags(req,
					   ASLFR_Window,
					   data->window,
					   ASLFR_TitleText,
					   GetString(locale, MSG_ENVIRONMENT_SELECT_PALETTE),
					   ASLFR_InitialDrawer,
					   path3,
					   TAG_END))
	{
		// Get path
		stccpy(buf, req->fr_Drawer, sizeof(buf));
		AddPart(buf, req->fr_File, 256);

		// Load palette
		get_palette_file(data, buf);
	}

	// Unbusy
	ClearWindowBusy(data->window);
}

#if 0
typedef struct {
	UWORD	w, h;		/* Width, height in pixels */
	WORD	x, y;		/* x, y position for this bitmap  */
	UBYTE	nPlanes;	/* # of planes (not including mask) */
	UBYTE	masking;	/* a masking technique listed above */
	UBYTE	compression;	/* cmpNone or cmpByteRun1 */
	UBYTE	flags;		/* as defined or approved by Commodore */
	UWORD	transparentColor;
	UBYTE	xAspect, yAspect;
	WORD	pageWidth, pageHeight;
} BitMapHeader;
#endif

#define BMHDB_CMAPOK 7
#define BMHDF_CMAPOK (1 << BMHDB_CMAPOK)

typedef struct
{
	UBYTE red, green, blue; /* MUST be UBYTEs so ">> 4" won't sign extend.*/
} ColorRegister;

// Load an external palette file
BOOL get_palette_file(config_env_data *data, char *name)
{
	BOOL ret = 0;
	short pen;
	struct PalettePrefs prefs;
	ULONG rgb[24];
	APTR iff;

	// Open as IFF PREF file
	if (!(iff = IFFOpen(name, IFF_READ, ID_PREF)) && !(iff = IFFOpen(name, IFF_READ, ID_ILBM)))
		return 0;

	// ILBM?
	if (IFFGetFORM(iff) == ID_ILBM)
	{
		BOOL eight = 0;

		// Eight bits per gun?
		if (IFFNextChunk(iff, ID_BMHD))
		{
			BitMapHeader bmhd;

			// Read header
			if (IFFReadChunkBytes(iff, &bmhd, sizeof(bmhd)) == sizeof(bmhd))
			{
				// See if CMAPOK bit is set
				if (bmhd.flags & BMHDF_CMAPOK)
					eight = 1;
			}
		}

		// Find palette chunk
		if (IFFNextChunk(iff, ID_CMAP))
		{
			short num, pen;
			ColorRegister creg[16];

			// Get count
			num = IFFChunkSize(iff) / sizeof(ColorRegister);

			// Check not too many colours
			if (num > 16)
				num = 16;

			// Read colours
			if (IFFReadChunkBytes(iff, creg, num * sizeof(ColorRegister)) == num * sizeof(ColorRegister))
			{
				// Convert colours
				for (pen = 0; pen < num; pen++)
				{
					// If not eight bit colour, convert to such
					if (!eight)
					{
						creg[pen].red |= (creg[pen].red >> 4);
						creg[pen].green |= (creg[pen].green >> 4);
						creg[pen].blue |= (creg[pen].blue >> 4);
					}

					// Fill out Prefs structure
					prefs.pap_Colors[pen].Red = (creg[pen].red << 8) | creg[pen].red;
					prefs.pap_Colors[pen].Green = (creg[pen].green << 8) | creg[pen].green;
					prefs.pap_Colors[pen].Blue = (creg[pen].blue << 8) | creg[pen].blue;
				}

				// Set ok
				ret = 1;
			}
		}
	}

	// Find palette chunk in prefs file
	else if (IFFNextChunk(iff, ID_PALT))
	{
		// Read prefs
		if (IFFReadChunkBytes(iff, &prefs, sizeof(prefs)) == sizeof(prefs))
			ret = 1;
	}

	// Close file
	IFFClose(iff);

	// Not found?
	if (!ret)
		return 0;

	// Convert to 32 bits per gun
	for (pen = 0; pen < 8; pen++)
	{
		rgb[pen * 3] = (prefs.pap_Colors[pen].Red << 16) | prefs.pap_Colors[pen].Red;
		rgb[pen * 3 + 1] = (prefs.pap_Colors[pen].Green << 16) | prefs.pap_Colors[pen].Green;
		rgb[pen * 3 + 2] = (prefs.pap_Colors[pen].Blue << 16) | prefs.pap_Colors[pen].Blue;
	}

	// Use the colours
	_config_env_palette_os_colours(data, rgb);

	return 1;
}
