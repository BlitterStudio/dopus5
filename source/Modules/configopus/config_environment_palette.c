#include "config_lib.h"
#include "config_environment.h"
#include <proto/module.h>
#include <prefs/palette.h>

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

	/*
		// On a cybergfx screen we need to redraw the colour square
		if (data->cyber_flag)
			SetGadgetAttrs(GADGET(data->palette_gadget),data->window,0,
				DPG_Redraw,data->orig_colour_sel,
				TAG_END);
	*/
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

/*
// Grab Workbench palette
void _config_env_grabwb(config_env_data *data)
{
	struct Screen *screen;
	ULONG palette[12];
	short num;

	// Get Workbench
	if (!(screen=LockPubScreen("Workbench")))
		return;

	// Get first four
	GetPalette32(&screen->ViewPort,palette,4,0);

	// Load first four
	for (num=0;num<4;num++)
		set_rgb32(data,num,palette+(num*3));

	// Copy palette
	CopyMem((char *)palette,(char *)(data->config->palette+1),sizeof(palette));

	// Under 39 we have top four colours
	if (GfxBase->LibNode.lib_Version>=39)
	{
		ULONG palette2[12];

		// Workbench that deep?
		if (screen->RastPort.BitMap->Depth>2)
		{
			// Get top four
			GetPalette32(&screen->ViewPort,palette,4,(1<<screen->RastPort.BitMap->Depth)-4);
		}

		// Reverse the order
		for (num=0;num<4;num++)
		{
			palette2[(3-num)*3+0]=palette[num*3+0];
			palette2[(3-num)*3+1]=palette[num*3+1];
			palette2[(3-num)*3+2]=palette[num*3+2];
		}

		// Load top four
		for (num=4;num<8;num++)
			set_rgb32(data,num,palette2+((num-4)*3));

		// Copy palette
		CopyMem((char *)palette2,(char *)(data->config->palette+13),sizeof(palette));
	}

	// Unlock Workbench
	UnlockPubScreen(0,screen);
}
*/

// Load palette file
void _config_env_grabwb(config_env_data *data)
{
	char buf[256];
	struct FileRequester *req;

	// Make window busy
	SetWindowBusy(data->window);

	// Get requester pointer
	req = WINREQUESTER(data->window);

	// Show requester
	char path3[256];
	BPTR lock3 = Lock("sys:prefs/presets", ACCESS_READ);
	if (lock3)
	{
		NameFromLock(lock3, path3, sizeof(path3));
		UnLock(lock3);
	}
	else
	{
		path3[0] = 0;
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
	ULONG *palette;
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

	// Get palette pointer
	palette = data->config->palette + 1;

	// Go through pens
	for (pen = 0; pen < 8; pen++)
	{
		short index_pen;

		// Get index pen number (top 4 are reversed order)
		index_pen = (pen < 4) ? pen : (11 - pen);

		// Store in config
		*palette++ = (prefs.pap_Colors[index_pen].Red << 16) | prefs.pap_Colors[index_pen].Red;
		*palette++ = (prefs.pap_Colors[index_pen].Green << 16) | prefs.pap_Colors[index_pen].Green;
		*palette++ = (prefs.pap_Colors[index_pen].Blue << 16) | prefs.pap_Colors[index_pen].Blue;

		// On own screen?
		if (data->own_screen)
		{
			// Deep enough?
			if (pen < 4 || data->own_screen > 4)
			{
				// Set colour
				set_rgb32(data, pen, palette - 3);
			}
		}
	}

	return 1;
}
