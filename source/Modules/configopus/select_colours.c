#include "config_lib.h"

/*#define DOpusBase		(data->DOpusBase)
#define IntuitionBase	(data->IntuitionBase)*/

struct Window *palette_box_open(PaletteBoxData *data, ObjectDef *, short *, short *);
void palette_box_colour(PaletteBoxData *, GL_Object *, short, ULONG *, short, ULONG);

void SAVEDS PaletteBox(void)
{
	PaletteBoxData *data;
	struct Window *window;
	IPCData *ipc;

	// Do startup
	if (!(ipc = Local_IPC_ProcStartup((IPTR *)&data, 0)))
	{
		Forbid();
		return;
	}

	// Get pen count
	data->pen_count = data->screen_data.pen_count;

	// Open window
	if ((window = palette_box_open(data, data->stuff.stuff1.object_def, data->pen_array, data->pen_array)))
	{
		// Event loop
		FOREVER
		{
			struct IntuiMessage *msg;
			IPCMessage *imsg;
			short break_flag = 0;

			// IPC messages
			while ((imsg = (IPCMessage *)GetMsg(ipc->command_port)))
			{
				switch (imsg->command)
				{
				// Quit
				case IPC_QUIT:
					break_flag = 1;
					break;

				// Activate
				case IPC_ACTIVATE:
					if (window)
					{
						WindowToFront(window);
						ActivateWindow(window);
					}
					break;

				// Hide
				case IPC_HIDE:
					CloseConfigWindow(window);
					window = 0;
					break;

				// Show
				case IPC_SHOW:
					data->newwin.parent = (void *)imsg->data;
					if (!(window =
							  palette_box_open(data, data->stuff.stuff1.object_def, data->pen_array, data->pen_array)))
						break_flag = 1;
					break;
				}

				IPC_Reply(imsg);
			}

			// Intuition messages
			if (window)
			{
				while ((msg = GetWindowMsg(window->UserPort)))
				{
					struct IntuiMessage msg_copy;
					msg_copy = *msg;
					ReplyWindowMsg(msg);

					// Close
					if (msg_copy.Class == IDCMP_CLOSEWINDOW)
						break_flag = 1;

					// Gadget
					else if (msg_copy.Class == IDCMP_GADGETUP)
					{
						short fg, bg;

						// Get foreground and background colours
						fg = GetGadgetValue(OBJLIST(window), GAD_PALETTE_FOREGROUND);
						bg = GetGadgetValue(OBJLIST(window), GAD_PALETTE_BACKGROUND);

						// Fix pens
						if (((struct Library *)IntuitionBase)->lib_Version >= 39)
						{
							if (fg >= 4 && fg < 8)
								fg = 248 + fg;
							else if (fg > 7)
								fg -= 4;
							if (bg >= 4 && bg < 8)
								bg = 248 + bg;
							else if (bg > 7)
								bg -= 4;
						}
						data->fgpen = fg;
						data->bgpen = bg;

						// Send new colours
						IPC_Command(data->owner_ipc, BUTTONEDIT_NEW_COLOURS, data->fgpen, (APTR)(IPTR)data->bgpen, 0, 0);
					}

					// Key
					else if (msg_copy.Class == IDCMP_RAWKEY)
					{
						// Help?
						if (msg_copy.Code == 0x5f && !(msg_copy.Qualifier & VALID_QUALIFIERS))
						{
							// Set busy pointer
							SetWindowBusy(window);

							// Send help command
							IPC_Command(data->main_ipc, IPC_HELP, (1 << 31), "Select Colours", 0, REPLY_NO_PORT);

							// Clear busy pointer
							ClearWindowBusy(window);
						}
					}
				}
			}

			if (break_flag)
				break;

			Wait(1 << ipc->command_port->mp_SigBit | ((window) ? 1 << window->UserPort->mp_SigBit : 0));
		}

		CloseConfigWindow(window);
	}

	// Say goodbye
	IPC_Goodbye(ipc, data->owner_ipc, -1);

	// Delete IPC data
	Forbid();
	IPC_Free(ipc);
}

struct Window *palette_box_open(PaletteBoxData *data, ObjectDef *objects, short *pen_array1, short *pen_array2)
{
	// Open window
	if ((data->window = OpenConfigWindow(&data->newwin)) && (data->list = AddObjectList(data->window, objects)))
	{
		short fpen, bpen, a;
		struct Gadget *gadget;
		struct TagItem tags[3];

		// Two gadgets
		for (a = 0; a < 2; a++)
		{
			// Get gadget
			gadget = GADGET(GetObject(data->list, GAD_PALETTE_FOREGROUND + a));

			// Fill out tags
			tags[0].ti_Tag = GTPA_NumColors;
			tags[0].ti_Data = data->pen_count + ((((struct Library *)IntuitionBase)->lib_Version >= 39) ? 8 : 4);
			tags[1].ti_Tag = GTPA_ColorTable;
			tags[1].ti_Data = (IPTR)((a == 0) ? pen_array1 : pen_array2);
			tags[2].ti_Tag = TAG_END;

			// Check number of pens
			if (tags[0].ti_Data > (1 << data->window->RPort->BitMap->Depth))
				tags[0].ti_Data = 1 << data->window->RPort->BitMap->Depth;

			// Initialise gadget
			SetGadgetAttrsA(gadget, data->window, 0, tags);
		}

		// Get initial pens
		fpen = data->fgpen;
		bpen = data->bgpen;

		// Map top colours
		if (((struct Library *)IntuitionBase)->lib_Version >= 39)
		{
			if (fpen >= 252)
				fpen -= 248;
			else if (fpen >= 4)
				fpen += 4;
			if (bpen >= 252)
				bpen -= 248;
			else if (bpen >= 4)
				bpen += 4;
		}

		// Initialise colours
		SetGadgetValue(data->list, GAD_PALETTE_FOREGROUND, fpen);
		SetGadgetValue(data->list, GAD_PALETTE_BACKGROUND, bpen);

		// No foreground editing?
		if (data->flags & PBF_NO_FG)
		{
			DisableObject(data->list, GAD_PALETTE_FOREGROUND, TRUE);
		}
		return data->window;
	}
	CloseConfigWindow(data->window);
	return 0;
}

/*#undef DOpusBase
#undef IntuitionBase*/

// Show palette box
long LIBFUNC L_ShowPaletteBox(REG(a0, struct Window *parent),
							  REG(a1, DOpusScreenData *screen_data),
							  REG(a2, short *fgpen),
							  REG(a3, short *bgpen),
                              REG(d0, struct TextAttr *font),
                              REG(d1, ColourSpec32 *spec),
                              REG(d2, short *spec_pen))
{
	PaletteBoxData *data;
	short a, extra = 0;
	struct Window *window;
	ConfigWindow dims;
	short pen_array2[26], slide_gun = -1, ret = -1;
	short quit_flag = 0;
	ColourSpec32 spec_backup[2];

	// Allocate data
	if (!(data = AllocVec(sizeof(PaletteBoxData), MEMF_CLEAR)))
		return -1;

	// Backup colours
	if (spec)
		CopyMem((char *)spec, (char *)spec_backup, sizeof(ColourSpec32) * 2);

	// Got extra pens?
	if (spec && spec_pen && (spec_pen[0] > -1 || spec_pen[1] > -1))
		extra = 1;

	// Get dimensions, set to mouse centering
	CopyMem((char *)(extra ? &_palette_box_cust_window : &_palette_box_window), (char *)&dims, sizeof(ConfigWindow));
	dims.char_dim.Left = POS_MOUSE_CENTER;
	dims.char_dim.Top = POS_MOUSE_CENTER;

	// Initialise data
	data->newwin.parent = parent;
	data->newwin.dims = &dims;
	data->newwin.title = GetString(locale, MSG_SELECT_COLOURS);
	data->newwin.locale = locale;
	data->newwin.flags = WINDOW_VISITOR | WINDOW_REQ_FILL | WINDOW_AUTO_KEYS;

	// Copy pen array
	for (a = 0; a < 16; a++)
	{
		data->pen_array[a] = screen_data->pen_array[a];
		pen_array2[a] = screen_data->pen_array[a];
	}
	for (; a < 24; a++)
	{
		data->pen_array[a] = screen_data->pen_array2[a - 16];
		pen_array2[a] = screen_data->pen_array2[a - 16];
	}

	// Get pen count
	data->pen_count = screen_data->pen_count;

	// Extra custom pen?
	if (extra)
	{
		// Store pens
		data->pen_array[data->pen_count + ((((struct Library *)IntuitionBase)->lib_Version >= 39) ? 8 : 4)] =
			spec_pen[0];
		pen_array2[data->pen_count + ((((struct Library *)IntuitionBase)->lib_Version >= 39) ? 8 : 4)] = spec_pen[1];
		// Add another pen
		data->pen_count++;
	}

	// Libraries
	/*data->DOpusBase=DOpusBase;
	data->IntuitionBase=(struct Library *)IntuitionBase;*/

	// Bits per gun defaults to 4
	data->stuff.stuff2.gun_bits[0] = 4;
	data->stuff.stuff2.gun_bits[1] = 4;
	data->stuff.stuff2.gun_bits[2] = 4;

	// Set initial pens
	data->fgpen = *fgpen;
	data->bgpen = *bgpen;

	// Open window
	if (!(window = palette_box_open(
			  data, (extra) ? _palette_box_cust_objects : _palette_box_objects, data->pen_array, pen_array2)))
	{
		// Failed
		FreeVec(data);
		return -1;
	}

	// Get gadgets
	data->stuff.stuff2.palette_gadget[0] = GetObject(data->list, GAD_PALETTE_FOREGROUND);
	data->stuff.stuff2.palette_gadget[1] = GetObject(data->list, GAD_PALETTE_BACKGROUND);

	// If v39, ask display database for bpg
	if (((struct Library *)GfxBase)->lib_Version >= 39)
	{
		struct DisplayInfo dispinfo;
		short a, b;
		unsigned short max;
		ULONG *ptr;

		// Get bits per gun from environment database
		if (GetDisplayInfoData(0,
							   (char *)&dispinfo,
							   sizeof(struct DisplayInfo),
							   DTAG_DISP,
							   GetVPModeID(&data->window->WScreen->ViewPort)))
		{
			data->stuff.stuff2.gun_bits[0] = dispinfo.RedBits;
			data->stuff.stuff2.gun_bits[1] = dispinfo.GreenBits;
			data->stuff.stuff2.gun_bits[2] = dispinfo.BlueBits;
		}

		// Got sliders?
		if (extra)
		{
			// Fix sliders
			for (b = 0; b < 2; b++)
			{
				// Get palette pointer
				ptr = (ULONG *)&spec[b];

				// 3 guns
				for (a = 0; a < 3; a++)
				{
					// Maximum value
					max = (1 << data->stuff.stuff2.gun_bits[a]) - 1;

					// Set limit
					SetGadgetChoices(data->list, GAD_PALETTE_RED_FORE + (b * 3) + a, (APTR)(IPTR)((IPTR)max << 16));

					// Initialise value
					SetGadgetValue(data->list,
								   GAD_PALETTE_RED_FORE + (b * 3) + a,
								   ptr[a] >> (32 - data->stuff.stuff2.gun_bits[a]));

					// Set initial colour
					palette_box_colour(data,
									   data->stuff.stuff2.palette_gadget[b],
									   a,
									   ptr,
									   spec_pen[b],
									   ptr[a] >> (32 - data->stuff.stuff2.gun_bits[a]));
				}
			}
		}
	}

	// Event loop
	FOREVER
	{
		struct IntuiMessage *msg;

		// Intuition messages
		while ((msg = GetWindowMsg(window->UserPort)))
		{
			struct IntuiMessage msg_copy;
			short id;

			// Copy message and reply
			msg_copy = *msg;
			ReplyWindowMsg(msg);

			// Look at message
			switch (msg_copy.Class)
			{
			// Close
			case IDCMP_CLOSEWINDOW:
				quit_flag = (extra) ? 1 : 2;
				break;

			// Gadget
			case IDCMP_GADGETUP:
			case IDCMP_GADGETDOWN:

				// Look at ID
				id = ((struct Gadget *)msg_copy.IAddress)->GadgetID;
				switch (id)
				{
				// Use
				case GAD_PALETTE_USE:
					quit_flag = 2;
					break;

				// Cancel
				case GAD_PALETTE_CANCEL:
					quit_flag = 1;
					break;

				// Foreground/background
				case GAD_PALETTE_FOREGROUND:
				case GAD_PALETTE_BACKGROUND: {
					short fg, bg;

					// Get foreground and background colours
					fg = GetGadgetValue(OBJLIST(window), GAD_PALETTE_FOREGROUND);
					bg = GetGadgetValue(OBJLIST(window), GAD_PALETTE_BACKGROUND);

					// Set flags to indicate custom colours
					ret = 0;
					if (extra)
					{
						if (fg ==
							data->pen_count + ((((struct Library *)IntuitionBase)->lib_Version >= 39) ? 8 : 4) - 1)
							ret |= 1 << 0;
						if (bg ==
							data->pen_count + ((((struct Library *)IntuitionBase)->lib_Version >= 39) ? 8 : 4) - 1)
							ret |= 1 << 1;
					}

					// Fix pens
					if (((struct Library *)IntuitionBase)->lib_Version >= 39)
					{
						if (fg >= 4 && fg < 8)
							fg = 248 + fg;
						else if (fg > 7)
							fg -= 4;
						if (bg >= 4 && bg < 8)
							bg = 248 + bg;
						else if (bg > 7)
							bg -= 4;
					}

					// Store new pens
					data->fgpen = fg;
					data->bgpen = bg;
				}
				break;

				// Palette sliders for foreground
				case GAD_PALETTE_RED_FORE:
				case GAD_PALETTE_GREEN_FORE:
				case GAD_PALETTE_BLUE_FORE:

					// Change colour
					palette_box_colour(data,
									   data->stuff.stuff2.palette_gadget[0],
									   id - GAD_PALETTE_RED_FORE,
									   (ULONG *)&spec[0],
									   spec_pen[0],
									   msg_copy.Code);

					// Remember this one
					if (msg_copy.Class == IDCMP_GADGETDOWN)
						slide_gun = id;
					else
						slide_gun = -1;
					break;

				// Palette sliders for background
				case GAD_PALETTE_RED_BACK:
				case GAD_PALETTE_GREEN_BACK:
				case GAD_PALETTE_BLUE_BACK:

					// Change colour
					palette_box_colour(data,
									   data->stuff.stuff2.palette_gadget[1],
									   id - GAD_PALETTE_RED_BACK,
									   (ULONG *)&spec[1],
									   spec_pen[1],
									   msg_copy.Code);

					// Remember this one
					if (msg_copy.Class == IDCMP_GADGETDOWN)
						slide_gun = id;
					else
						slide_gun = -1;
					break;
				}
				break;

			// Mouse move
			case IDCMP_MOUSEMOVE:

				// See if sliding a gun
				if (slide_gun > -1)
				{
					short id, which;

					// Get which one
					if (slide_gun >= GAD_PALETTE_RED_BACK)
					{
						id = slide_gun - GAD_PALETTE_RED_BACK;
						which = 1;
					}
					else
					{
						id = slide_gun - GAD_PALETTE_RED_FORE;
						which = 0;
					}

					// Change colour
					palette_box_colour(data,
									   data->stuff.stuff2.palette_gadget[which],
									   id,
									   (ULONG *)&spec[which],
									   spec_pen[which],
									   msg_copy.Code);
					break;
				}
				break;
			}
		}

		// Check quit
		if (quit_flag)
			break;

		// Wait for event
		Wait(1 << window->UserPort->mp_SigBit);
	}

	// Ok to use?
	if (quit_flag == 2)
	{
		// Store new pens
		*fgpen = data->fgpen;
		*bgpen = data->bgpen;
	}

	// Cancel
	else
	{
		// Nothing selected
		ret = -1;

		// If we have extra, restore the colours
		if (spec && extra)
		{
			short a, b;
			ULONG *ptr;

			// Restore colours
			CopyMem((char *)spec_backup, (char *)spec, sizeof(ColourSpec32) * 2);

			// Fix colours
			for (b = 0; b < 2; b++)
			{
				// Get palette pointer
				ptr = (ULONG *)&spec[b];

				// 3 guns
				for (a = 0; a < 3; a++)
				{
					// Set initial colour
					palette_box_colour(data,
									   data->stuff.stuff2.palette_gadget[b],
									   a,
									   ptr,
									   spec_pen[b],
									   ptr[a] >> (32 - data->stuff.stuff2.gun_bits[a]));
				}
			}
		}
	}

	// Close window
	CloseConfigWindow(window);

	// Free data
	FreeVec(data);
	return ret;
}

// Change colour
void palette_box_colour(PaletteBoxData *data, GL_Object *gadget, short gun, ULONG *spec, short pen, ULONG value)
{
	// This only works under 39
	if (((struct Library *)GfxBase)->lib_Version < 39)
		return;

	// Set new value
	spec[gun] = value << (32 - data->stuff.stuff2.gun_bits[gun]);

	// Set new colour register
	SetRGB32(&data->window->WScreen->ViewPort, pen, spec[0], spec[1], spec[2]);

	// Redraw square
	SetGadgetAttrs(GADGET(gadget),
				   data->window,
				   0,
				   DPG_Redraw,
				   data->pen_count + ((((struct Library *)IntuitionBase)->lib_Version >= 39) ? 8 : 4) - 1,
				   TAG_END);
}
