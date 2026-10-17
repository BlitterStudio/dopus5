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

#include "show.h"

static unsigned short pens[1] = {(unsigned short)~0};

int LIBFUNC L_Module_Entry(REG(a0, struct List *files),
						   REG(a1, struct Screen *screen),
						   REG(a2, IPCData *ipc),
						   REG(a3, IPCData *main_ipc),
						   REG(d0, IPTR mod_id),
						   REG(d1, ULONG mod_data))
{
	show_data *data;
	struct Node *node;
	static unsigned short __chip null_pointer[6];
	short quit_flag = 0;
	BOOL dt_first = 0;

	// Allocate data
	if (!(data = AllocVec(sizeof(show_data), MEMF_CLEAR)))
		return 0;

	// Save datatypes library base
	// data->dt_base = DataTypesBase;

	// Open datatypes library - Already opened in libinit.c
	/*DataTypesBase=OpenLibrary("datatypes.library",0);
	#ifdef __amigaos4__
	IDataTypes=(struct DataTypesIFace *)GetInterface(DataTypesBase,"main",1,NULL);
	#endif*/

	// Open timer
	if (!(data->timer = AllocTimer(UNIT_MICROHZ, 0)))
	{
		show_free(data);
		return 0;
	}

	// Store IPC pointer
	data->ipc = ipc;

	// Use datatypes first?  The user's preference is now passed in via
	// the caller (DISPOPTF_SHOW_DATATYPES_FIRST -> SHOW_DATATYPES_FIRST_FLAG
	// bit on mod_data).
	if (mod_data & SHOW_DATATYPES_FIRST_FLAG)
	{
		dt_first = 1;
	}

	// Go through files
	for (node = files->lh_Head; node->ln_Succ; node = node->ln_Succ)
	{
		// Reset quit flag
		quit_flag = 0;

		// Store file name pointer
		data->file = node->ln_Name;
		data->pic_ok = 0;

		// Try datatypes first?
		if (dt_first)
			show_get_dtpic(data, node);

		// If no datatypes picture, try to get ILBM
		if (!data->pic_ok && (data->ilbm = ReadILBM(node->ln_Name, ILBMF_GET_BODY)))
		{
			BOOL ok = 1;

			// Check for palette file (small body of zeroes)
			if (data->ilbm->image.body.size < 16)
			{
				short a;

				// Check for all zeroes
				for (a = 0; a < data->ilbm->image.body.size; a++)
					if (data->ilbm->image.body.data[a])
						break;

				// All zeroes?
				if (a == data->ilbm->image.body.size)
					ok = 0;
			}

			// Ok to decode?
			if (ok)
			{
				// We can't show a 24 bit ILBM unless we're running a hicolor display
				if (data->ilbm->header.nPlanes > 8)
				{
					ok = 0;
					if (((struct Library *)GfxBase)->lib_Version >= 39)
					{
						if (screen && GetBitMapAttr(screen->RastPort.BitMap, BMA_DEPTH) > 8)
							ok = 1;
						else if (((struct IntuitionBase *)IntuitionBase)->ActiveScreen &&
								 GetBitMapAttr(((struct IntuitionBase *)IntuitionBase)->ActiveScreen->RastPort.BitMap,
											   BMA_DEPTH) > 8)
							ok = 1;
					}
				}

				// Still ok?
				if (ok)
				{
					// Get mode and size
					data->modeid = data->ilbm->mode_id;
					data->width = data->ilbm->header.w;
					data->height = data->ilbm->header.h;
					data->depth = data->ilbm->header.nPlanes;
					data->picture_type = "IFF ILBM";
					data->pic_ok = 1;
				}
			}
		}

		// No picture yet? Try datatypes if we haven't already
		if (!data->pic_ok && !dt_first)
			show_get_dtpic(data, node);

		// Ignore malformed successes that did not leave a drawable source
		if (data->pic_ok && !data->dt_object && !data->ilbm)
			data->pic_ok = 0;

		// Got a picture?
		if (data->pic_ok)
		{
			// Clear naughty bits if not a 32 bit mode
			if (!data->dt_object)
				data->modeid &= ~(GENLOCK_VIDEO | GENLOCK_AUDIO | VP_HIDE | SPRITES | PFBA);

			// 24 bit ILBM?
			if (data->depth == 24 && data->modeid == 0)
			{
				// Get best available mode (under 39)
				if (((struct Library *)GfxBase)->lib_Version >= 39)
					data->modeid = BestModeID(BIDTAG_NominalWidth,
											  data->width,
											  BIDTAG_NominalHeight,
											  data->height,
											  BIDTAG_Depth,
											  24,
											  (screen) ? BIDTAG_MonitorID : TAG_IGNORE,
											  (screen) ? (GetVPModeID(&screen->ViewPort) & MONITOR_ID_MASK) : 0,
											  TAG_END);
			}

			// Check if mode is unavailable
			if (ModeNotAvailable(data->modeid))
			{
				// 24 bit mode? Cut back to 8
				if (data->depth == 24)
					data->depth = 8;

				// Get best available mode (under 39)
				if (((struct Library *)GfxBase)->lib_Version >= 39)
				{
					ULONG modeid;

					// Get best mode
					if ((modeid = BestModeID(BIDTAG_NominalWidth,
											 data->width,
											 BIDTAG_NominalHeight,
											 data->height,
											 BIDTAG_DesiredWidth,
											 data->width,
											 BIDTAG_DesiredHeight,
											 data->height,
											 BIDTAG_Depth,
											 data->depth,
											 BIDTAG_SourceID,
											 data->modeid,
											 (screen) ? BIDTAG_MonitorID : TAG_IGNORE,
											 (screen) ? (GetVPModeID(&screen->ViewPort) & MONITOR_ID_MASK) : 0,
											 TAG_END)) != INVALID_ID)
						data->modeid = modeid;
				}

				// Still unavailable?
				if (ModeNotAvailable(data->modeid))
				{
					ULONG modeid = 0;

					// Low or high res?
					if (data->width > 320)
						modeid |= HIRES;

					// Lace?
					if (data->height > 256)
						modeid |= LACE;

					// Ham?
					if (data->modeid & HAM_KEY)
						modeid |= HAM_KEY;

					// EHB
					else if (data->modeid & EXTRAHALFBRITE_KEY)
						modeid |= EXTRAHALFBRITE_KEY;

					// Use new mode
					data->modeid = modeid;
				}
			}

			// Open screen
			if ((data->display_screen = OpenScreenTags(0,
													   SA_Width,
													   data->width,
													   SA_Height,
													   (data->height < 16) ? 16 : data->height,
													   SA_Depth,
													   data->depth,
													   SA_Quiet,
													   TRUE,
													   SA_Type,
													   CUSTOMSCREEN,
													   SA_DisplayID,
													   data->modeid,
													   SA_AutoScroll,
													   TRUE,
													   SA_Pens,
													   (IPTR)pens,
													   SA_ErrorCode,
													   (IPTR)&data->error,
													   SA_Overscan,
													   OSCAN_MAX,
													   SA_Behind,
													   TRUE,
													   TAG_END)))
			{
				// Datatypes object?
				if (data->dt_object)
				{
					unsigned long i, r, g, b;

					// Valid palette?
					if (data->cregs)
					{
						ULONG *temp;

						// Try allocate temporary palette
						if ((temp = AllocVec(((data->numcolours * 3) + 2) * sizeof(ULONG), MEMF_CLEAR)))
						{
							// Copy palette
							CopyMem((char *)data->cregs, (char *)(temp + 1), data->numcolours * 3 * sizeof(ULONG));

							// Initialise palette run
							temp[0] = data->numcolours << 16;

							// Load palette
							LoadRGB32(&data->display_screen->ViewPort, temp);
							FreeVec(temp);
						}

						// Do it manually
						else
						{
							for (i = 0; i < data->numcolours; i++)
							{
								r = data->cregs[i * 3 + 0];
								g = data->cregs[i * 3 + 1];
								b = data->cregs[i * 3 + 2];
								SetRGB32(&data->display_screen->ViewPort, i, r, g, b);
							}
						}
					}
				}

				// Otherwise, load ILBM palette
				else if (data->ilbm && data->ilbm->palette)
					LoadPalette32(&data->display_screen->ViewPort, data->ilbm->palette);

				// Open window
				data->display_window =
					OpenWindowTags(0,
								   WA_Width,
								   data->display_screen->Width,
								   WA_Height,
								   data->display_screen->Height,
								   WA_IDCMP,
								   IDCMP_RAWKEY | IDCMP_VANILLAKEY | IDCMP_MOUSEBUTTONS | IDCMP_INACTIVEWINDOW,
								   WA_CustomScreen,
								   data->display_screen,
								   WA_WindowName,
								   DOPUS_WIN_NAME,
								   WA_Borderless,
								   TRUE,
								   WA_Activate,
								   TRUE,
								   WA_RMBTrap,
								   TRUE,
								   TAG_END);

				data->active = 1;
			}
		}

		// Do we have a window?
		if (data->display_window)
		{
			// Bring screen to front
			if (mod_id != 666)
				ScreenToFront(data->display_screen);

			// Datatypes?
			if (data->dt_object)
			{
				// Blit from datatypes bitmap to main bitmap
				BltBitMapRastPort(data->dt_bm,
								  0,
								  0,
								  data->display_window->RPort,
								  0,
								  0,
								  GetBitMapAttr(data->dt_bm, BMA_WIDTH),
								  GetBitMapAttr(data->dt_bm, BMA_HEIGHT),
								  0xc0);
				WaitBlit();

				// Free datatypes object
				DisposeDTObject(data->dt_object);
				data->dt_object = 0;
			}

			// ILBM?
			else if (data->ilbm)
			{
				ULONG flags = 0;

				// Is this an animation?
				if (data->ilbm->flags & ILBMF_IS_ANIM)
				{
					// If not a brush anim, get a double-buffer
					if (!(data->ilbm->flags & ILBMF_IS_ANIM_BRUSH))
					{
						// Allocate bitmap for double-buffering
						if ((data->double_bm = NewBitMap(
								 data->ilbm->header.w, data->ilbm->header.h, data->ilbm->header.nPlanes, 0, 0)))
							data->anim_ok = 1;
					}
					else
						data->anim_ok = 1;
				}

				// Under 39?
				if (((struct Library *)GfxBase)->lib_Version >= 39)
				{
					// Get bitmap flags
					if (!(GetBitMapAttr(data->display_screen->RastPort.BitMap, BMA_FLAGS) & BMF_STANDARD))
						flags = DIF_WRITEPIX;
				}

				// Masking?
				if (data->ilbm->header.masking == 1)
					flags |= DIF_MASK;

				// Decode data into display
				DecodeILBM((STRPTR)data->ilbm->image.body.data,
						   data->ilbm->header.w,
						   data->ilbm->header.h,
						   data->ilbm->header.nPlanes,
						   data->display_screen->RastPort.BitMap,
						   flags,
						   data->ilbm->header.compression);

				// Free body data
				FreeMemH(data->ilbm->image.body.data);
			}

			// Hide mouse pointer
			SetPointer(data->display_window, null_pointer, 1, 16, 0, 0);

			// Bring screen to front if not done already
			if (mod_id == 666)
				ScreenToFront(data->display_screen);

			// Animation?
			if (data->anim_ok)
			{
				// First bitmap
				data->anim_bm[0] = data->display_screen->RastPort.BitMap;
				data->anim_bitmap = 0;

				// Copy into double-buffer if this is an animation
				if (!(data->ilbm->flags & ILBMF_IS_ANIM_BRUSH))
				{
					BltBitMap(data->display_screen->RastPort.BitMap,
							  0,
							  0,
							  data->double_bm,
							  0,
							  0,
							  data->ilbm->header.w,
							  data->ilbm->header.h,
							  0xc0,
							  0xff,
							  0);

					// Get second bitmap pointer
					data->anim_bm[1] = data->double_bm;

					// Do first frame
					show_next_frame(data);
				}

				// Get initial frame speed
				if (data->ilbm->anim->header.framespersecond > 0)
					data->frame_speed = data->ilbm->anim->header.framespersecond;
				else
					data->frame_speed = 30;
				data->original_speed = data->frame_speed;

				// Calculate frame time
				data->frame_secs = 0;
				data->frame_micros = UDivMod32(1000000, data->frame_speed);
				if (data->frame_micros == 1000000)
				{
					data->frame_secs = 1;
					data->frame_micros = 0;
				}

				// Start animation playing
				data->anim_playing = 1;

				// Send time request
				StartTimer(data->timer, data->frame_secs, data->frame_micros);

				// Get proper frame count
				if (data->ilbm->flags & ILBMF_IS_ANIM_BRUSH)
					data->frame_count = data->ilbm->anim->frame_count;
				else
					data->frame_count = 1 + (data->ilbm->anim->frame_count - 2);
				data->frame_num = 1;
			}

			// Event loop
			FOREVER
			{
				struct IntuiMessage *msg;
				IPCMessage *imsg;

				// Timer complete?
				if (data->anim_ok && (CheckTimer(data->timer)))
				{
					// If animation is playing, show next frame
					if (data->anim_playing)
						show_next_frame(data);

					// Resend time request
					StartTimer(data->timer, data->frame_secs, data->frame_micros);
				}

				// IPC messages?
				if (ipc)
				{
					while ((imsg = (IPCMessage *)GetMsg(ipc->command_port)))
					{
						// Abort?
						if (imsg->command == IPC_ABORT || imsg->command == IPC_QUIT)
							quit_flag = -1;
						IPC_Reply(imsg);
					}
				}

				// Intuition messages
				while ((msg = (struct IntuiMessage *)GetMsg(data->display_window->UserPort)))
				{
					struct IntuiMessage msg_copy;
					BOOL recalc = 0;

					// Copy message and reply
					msg_copy = *msg;
					ReplyMsg((struct Message *)msg);

					// If in "start up pic mode", don't respond
					if (mod_id == 666)
						continue;

					switch (msg_copy.Class)
					{
					// Key press
					case IDCMP_RAWKEY:

						// Function keys give varying speeds
						if (msg_copy.Code >= 0x50 && msg_copy.Code <= 0x59)
						{
							msg_copy.Code -= 0x4f;
							data->frame_speed = UDivMod32(60, msg_copy.Code);
							recalc = 1;
							break;
						}

					case IDCMP_VANILLAKEY:

						// Ignore repeat
						if (msg_copy.Qualifier & IEQUALIFIER_REPEAT)
							break;

						switch (msg_copy.Code)
						{
						// Escape
						case 0x1b:
							quit_flag = -1;
							break;

						// Delete will mark for deletion
						case 0x7f:
							quit_flag = 1;
							node->lve_Flags |= SHOWF_DELETE;
							break;

						// Quit
						case 'q':
							node->lve_Flags |= SHOWF_SELECTED;
							quit_flag = 1;
							break;

						// Help
						case 0x5f:
						case 'p':
						case ' ':
							if (!(show_help(data)))
								quit_flag = -1;
							break;

						// Next frame
						case 'n':
							if (data->anim_ok)
							{
								data->anim_playing = 0;
								show_next_frame(data);
							}
							break;

						// Start/stop playing
						case 's':
							data->anim_playing = 1 - data->anim_playing;
							break;

						// Slow animation down
						case '-':
							if (data->frame_speed > 1)
							{
								--data->frame_speed;
								recalc = 1;
							}
							break;

						// Speed animation up
						case '=':
							if (data->frame_speed < 120)
							{
								++data->frame_speed;
								recalc = 1;
							}
							break;

						// Original speed
						case '\\':
							data->frame_speed = data->original_speed;
							recalc = 1;
							break;
						}
						break;

					// Mouse buttons
					case IDCMP_MOUSEBUTTONS:

						// Not active?
						if (!data->active)
						{
							data->active = 1;
							break;
						}

						// Quit type
						if (msg_copy.Code == SELECTDOWN)
							quit_flag = 1;
						else if (msg_copy.Code == MENUDOWN)
							quit_flag = -1;
						else if (msg_copy.Code == MIDDLEDOWN)
						{
							node->lve_Flags |= SHOWF_SELECTED;
							quit_flag = 1;
						}
						break;

					// Inactive
					case IDCMP_INACTIVEWINDOW:
						data->active = 0;
						break;
					}

					// Recalculate frame time?
					if (data->anim_ok && recalc)
					{
						// Calculate frame time
						data->frame_secs = 0;
						data->frame_micros = UDivMod32(1000000, data->frame_speed);
						if (data->frame_micros == 1000000)
						{
							data->frame_secs = 1;
							data->frame_micros = 0;
						}
					}
				}

				// Check quit flag
				if (quit_flag)
					break;

				// Wait for an event
				Wait(1 << data->display_window->UserPort->mp_SigBit | 1 << data->timer->port->mp_SigBit |
					 ((ipc) ? (1 << ipc->command_port->mp_SigBit) : 0));
			}
		}

		// Close window
		if (data->display_window)
		{
			if (data->display_screen)
				ScreenToBack(data->display_screen);
			CloseWindow(data->display_window);
			data->display_window = 0;
		}

		// Close screen
		if (data->display_screen)
		{
			// Make sure original bitmap is shown
			if (data->anim_ok && !(data->ilbm->flags & ILBMF_IS_ANIM_BRUSH))
				data->display_screen->ViewPort.RasInfo->BitMap = data->anim_bm[0];
			CloseScreen(data->display_screen);
			data->display_screen = 0;
		}

		// Free ILBM
		if (data->ilbm)
		{
			// Free double-buffer bitmap
			DisposeBitMap(data->double_bm);
			data->double_bm = 0;

			// Free ilbm
			FreeILBM(data->ilbm);
			data->ilbm = 0;
		}

		// Or datatypes object
		else if (data->dt_object)
		{
			DisposeDTObject(data->dt_object);
			data->dt_object = 0;
		}

		// Clear some stuff
		data->anim_ok = 0;
		data->cregs = 0;
		data->dt_bm = 0;

		// Abort?
		if (quit_flag == -1)
			break;
	}

	// Free data
	show_free(data);

	return (quit_flag == -1) ? -1 : 1;
}

// Free show data
void show_free(show_data *data)
{
	if (data)
	{
		// Close display window
		if (data->display_window)
			CloseWindow(data->display_window);

		// Close display screen
		if (data->display_screen)
			CloseScreen(data->display_screen);

		// Close timer
		FreeTimer(data->timer);

		// Close library
		/*#ifdef __amigaos4__
		DropInterface((struct Interface *)IDataTypes);
		#endif
		CloseLibrary(DataTypesBase);*/

		// Free data
		FreeVec(data);
	}
}

// Show information on a picture
BOOL show_help(show_data *data)
{
	ULONG mode_id = HIRES_KEY;
	ULONG wb_mode;
	BOOL screen_flag = 1;
	struct Screen *help_screen;
	struct Window *help_window;
	NewConfigWindow new_win;
	BOOL ret = 1;
	ObjectList *list;

	// Is display screen open?
	if (data->display_screen)
	{
		// Get monitor
		mode_id = GetVPModeID(&data->display_screen->ViewPort) & MONITOR_ID_MASK;
	}

	// Lock Workbench screen
	if ((help_screen = LockPubScreen("Workbench")))
	{
		// Get mode from workbench
		wb_mode = GetVPModeID(&help_screen->ViewPort);

		// Unlock Workbench
		UnlockPubScreen(0, help_screen);

		// Isolate display mode
		wb_mode &= ~MONITOR_ID_MASK;
		mode_id |= wb_mode;
	}

	// Can't lock Workbench, default to hires
	else
	{
		mode_id |= HIRES_KEY;
		wb_mode = mode_id;
	}

	// Under 39, use BestModeID
	if (((struct Library *)GfxBase)->lib_Version >= 39)
	{
		mode_id = BestModeID(BIDTAG_NominalWidth,
							 640,
							 BIDTAG_NominalHeight,
							 (wb_mode & LORESLACE_KEY) ? 400 : 200,
							 BIDTAG_Depth,
							 2,
							 BIDTAG_MonitorID,
							 mode_id & MONITOR_ID_MASK,
							 BIDTAG_SourceID,
							 wb_mode,
							 TAG_END);
	}

	// See if mode is available
	if (ModeNotAvailable(mode_id))
	{
		// Clear out display mode, make hires
		mode_id &= MONITOR_ID_MASK;
		mode_id |= HIRES_KEY;
	}

	// Open help screen
	if (!(help_screen = OpenScreenTags(0,
									   SA_Width,
									   STDSCREENWIDTH,
									   SA_Height,
									   STDSCREENHEIGHT,
									   SA_Overscan,
									   OSCAN_TEXT,
									   SA_DisplayID,
									   mode_id,
									   SA_Depth,
									   2,
									   SA_Pens,
									   (IPTR)pens,
									   SA_SysFont,
									   1,
									   SA_Title,
									   GetString(locale, MSG_INFO_TITLE),
									   SA_Parent,
									   data->display_screen,
									   SA_Type,
									   CUSTOMSCREEN,
									   TAG_END)))
	{
		// Get default public screen
		if (!(help_screen = LockPubScreen(0)))
			return 1;
		screen_flag = 0;
		ScreenToFront(help_screen);
	}

	// Fill out new window
	new_win.parent = help_screen;
	new_win.dims = (data->anim_ok) ? &anim_info_window : &picture_info_window;
	new_win.title = GetString(locale, MSG_INFO_TITLE);
	new_win.locale = locale;
	new_win.port = 0;
	new_win.flags = WINDOW_SCREEN_PARENT | WINDOW_SIMPLE | WINDOW_AUTO_REFRESH | WINDOW_AUTO_KEYS | WINDOW_REQ_FILL;
	new_win.font = 0;

	// Open info window
	if ((help_window = OpenConfigWindow(&new_win)))
	{
		char buf[sizeof(struct NameInfo)];
		ULONG mode_id;

		// Add objects
		if (data->anim_ok)
		{
			list = AddObjectList(help_window, anim_info_objects);
		}
		else
		{
			list = AddObjectList(help_window, picture_info_objects);
		}
		AddObjectList(help_window, show_info_objects);
		AddObjectList(help_window, print_objects);

		// File name
		SetGadgetValue(list, GAD_INFO_FILE, (IPTR)FilePart(data->file));

		// Image size
		lsprintf(buf, "%ld x %ld", data->width, data->height);
		SetGadgetValue(list, GAD_INFO_IMAGE_SIZE, (IPTR)buf);

		// Get mode
		mode_id = GetVPModeID(&data->display_screen->ViewPort);

		// Number of colours
		lsprintf(buf, "%ld", 1 << data->depth);
		if (data->depth)
		{
			// HAM mode?
			if (mode_id & HAM_KEY)
			{
				if (data->depth == 8)
					strcpy(buf, "256K (HAM8)");
				else
					strcpy(buf, "4096 (HAM6)");
			}

			// EHB mode
			else if (mode_id & EXTRAHALFBRITE_KEY)
			{
				lsprintf(buf, "%ld (EHB)", (1 << data->depth));
			}
		}
		SetGadgetValue(list, GAD_INFO_COLOURS, (IPTR)buf);

		// Get display mode name
		if (GetDisplayInfoData(0, buf, sizeof(struct NameInfo), DTAG_NAME, (mode_id & ~(HAM_KEY | EXTRAHALFBRITE_KEY))))
			SetGadgetValue(list, GAD_INFO_MODE, (IPTR)((struct NameInfo *)buf)->Name);

		// Animation info?
		if (data->anim_ok)
		{
			// Frame number
			lsprintf(buf, GetString(locale, MSG_INFO_FRAME_NUM), data->frame_num, data->frame_count, data->frame_speed);
			SetGadgetValue(list, GAD_INFO_FRAME, (IPTR)buf);

			// Anim type
			if (data->last_frame)
			{
				lsprintf(buf, "ANIM Op %ld", data->last_frame->header.operation);
				SetGadgetValue(list, GAD_INFO_ANIM, (IPTR)buf);
			}
		}

		// Otherwise, picture type
		else
			SetGadgetValue(list, GAD_INFO_TYPE, (IPTR)data->picture_type);

		// Get preferences
		GetPrefs(&data->prefs, sizeof(struct Preferences));

		// Initialise printer controls
		SetGadgetValue(list, GAD_PRINT_ASPECT, data->prefs.PrintAspect);
		SetGadgetValue(list, GAD_PRINT_IMAGE, data->prefs.PrintImage);
		SetGadgetValue(list, GAD_PRINT_SHADE, data->prefs.PrintShade);
		SetGadgetValue(list, GAD_PRINT_FORM_FEED, 1);

		// Event loop
		FOREVER
		{
			struct IntuiMessage *msg;
			IPCMessage *imsg;
			BOOL quit_flag = 0;

			// IPC messages?
			if (data->ipc)
			{
				while ((imsg = (IPCMessage *)GetMsg(data->ipc->command_port)))
				{
					// Abort?
					if (imsg->command == IPC_ABORT || imsg->command == IPC_QUIT)
					{
						quit_flag = 1;
						ret = 0;
					}
					IPC_Reply(imsg);
				}
			}

			// Intuition messages
			while ((msg = GetWindowMsg(help_window->UserPort)))
			{
				struct IntuiMessage msg_copy;

				// Copy message and reply
				msg_copy = *msg;
				ReplyWindowMsg(msg);

				switch (msg_copy.Class)
				{
				// Close window
				case IDCMP_CLOSEWINDOW:
					quit_flag = 1;
					break;

				// Gadget
				case IDCMP_GADGETUP:
					switch (((struct Gadget *)msg_copy.IAddress)->GadgetID)
					{
					// Ok
					case GAD_OK:
						quit_flag = 1;
						break;

					// Print
					case GAD_PRINT:
						if ((show_print(data, help_window, list)) == -1)
						{
							quit_flag = 1;
							ret = 0;
						}
						break;
					}
					break;
				}
			}

			if (quit_flag)
				break;

			Wait(1 << help_window->UserPort->mp_SigBit | ((data->ipc) ? (1 << data->ipc->command_port->mp_SigBit) : 0));
		}

		// Close information window
		CloseConfigWindow(help_window);
	}

	// Did we have a screen?
	if (help_screen)
	{
		// Close or unlock it as appropriate
		if (screen_flag)
			CloseScreen(help_screen);
		else
			UnlockPubScreen(0, help_screen);
	}

	return ret;
}

// Next animation frame
void show_next_frame(show_data *data)
{
	ANIMFrame *frame;

	// If we're not an anim brush, do double-buffering
	if (!(data->ilbm->flags & ILBMF_IS_ANIM_BRUSH))
	{
		// Switch to show other bitmap
		data->display_screen->ViewPort.RasInfo->BitMap = data->anim_bm[data->anim_bitmap];
		ScrollVPort(&data->display_screen->ViewPort);
		data->anim_bitmap = 1 - data->anim_bitmap;
	}

	// Make sure frame has been displayed
	WaitBOVP(&data->display_screen->ViewPort);

	// Back to first frame?
	if (!data->ilbm->anim->current || !data->ilbm->anim->current->node.mln_Succ)
	{
		// Get first frame?
		data->ilbm->anim->current = (ANIMFrame *)data->ilbm->anim->frames.mlh_Head;

		// Skip first frame unless an anim brush
		if (!(data->ilbm->flags & ILBMF_IS_ANIM_BRUSH))
			data->ilbm->anim->current = (ANIMFrame *)data->ilbm->anim->current->node.mln_Succ;

		// Reset frame number
		data->frame_num = 1;
	}

	// Get current frame
	frame = data->ilbm->anim->current;

	// Valid frame?
	if (frame && frame->node.mln_Succ)
	{
		// Increment frame number
		if ((++data->frame_num) > data->frame_count)
			data->frame_num = 1;

		// Save frame pointer
		data->last_frame = frame;

		// Decode type?
		switch (frame->header.operation)
		{
		// RLE
		case 0:
			DecodeILBM((STRPTR)frame->delta,
					   data->ilbm->header.w,
					   data->ilbm->header.h,
					   data->ilbm->header.nPlanes,
					   data->anim_bm[data->anim_bitmap],
					   data->ilbm->header.masking,
					   data->ilbm->header.compression);
			break;

		// Option 5
		case 5:
			do_riff(frame->delta,
					data->anim_bm[data->anim_bitmap],
					(frame->header.bits & ANIMF_XOR),
					data->ilbm->header.w,
					0);
			break;

		// Option 7
		case 7:
			do_riff_7(frame->delta,
					  data->anim_bm[data->anim_bitmap],
					  data->ilbm->header.w,
					  (frame->header.bits & ANIMF_LONG));
			break;
		}

		// Get next frame
		data->ilbm->anim->current = (ANIMFrame *)frame->node.mln_Succ;
	}
}

// A DLTA chunk starts with a table of big-endian 32-bit offsets from its start
static ULONG delta_offset(const unsigned char *delta, unsigned short index)
{
	const unsigned char *ptr = delta + index * 4;

	return ((ULONG)ptr[0] << 24) | ((ULONG)ptr[1] << 16) | ((ULONG)ptr[2] << 8) | (ULONG)ptr[3];
}

void do_riff(unsigned char *delta,
			 struct BitMap *bitmap,
			 unsigned short xor
			 ,
			 unsigned short sourcewidth,
			 unsigned short size)
{
	unsigned short plane;
	ULONG offset;
	unsigned char *data;

	// Round up source width
	sourcewidth += 15;
	sourcewidth >>= 4;
	sourcewidth <<= 1;

	// Got delta?
	if (delta)
	{
		// Go through planes
		for (plane = 0; plane < bitmap->Depth; plane++)
		{
			// Valid delta for this plane?
			if ((offset = delta_offset(delta, plane)))
			{
				// Get pointer to plane delta
				data = delta + offset;

				// Normal RIFF
				if (size == 0)
				{
					// XOR?
					if (xor)
					{
						AnimDecodeRIFFXor(data, (STRPTR)bitmap->Planes[plane], bitmap->BytesPerRow, sourcewidth);
					}

					// Set
					else
					{
						AnimDecodeRIFFSet(data, (STRPTR)bitmap->Planes[plane], bitmap->BytesPerRow, sourcewidth);
					}
				}

				/*
								// Short RIFF?
								else
								if (size==1)
								{
									decode_riff_int(
										(unsigned short *)data,
										bitmap->Planes[plane],
										bitmap->BytesPerRow,
										sourcewidth);
								}

								// Long riff
								else
								{
									decode_riff_long(
										(unsigned long *)data,
										bitmap->Planes[plane],
										bitmap->BytesPerRow,
										sourcewidth);
								}
				*/
			}
		}
	}
}

void do_riff_7(unsigned char *delta, struct BitMap *bitmap, unsigned short sourcewidth, unsigned short size)
{
	unsigned short plane;
	ULONG op_offset, data_offset;
	unsigned char *data;
	unsigned char *opcode;

	// Round up source width
	sourcewidth += 15;
	sourcewidth >>= 4;
	sourcewidth <<= 1;

	// Got delta?
	if (delta)
	{
		// Go through planes
		for (plane = 0; plane < bitmap->Depth; plane++)
		{
			// Valid delta for this plane? (opcode lists, then data lists 8 entries later)
			op_offset = delta_offset(delta, plane);
			data_offset = delta_offset(delta, plane + 8);
			if (op_offset && data_offset)
			{
				// Get opcodes
				opcode = delta + op_offset;

				// Get pointer to plane delta
				data = delta + data_offset;

				// Short?
				if (size == 0)
				{
					decode_riff7_short(
						(unsigned short *)data, opcode, bitmap->Planes[plane], bitmap->BytesPerRow, sourcewidth);
				}

				// Long
				else
				{
					decode_riff7_long(
						(ULONG *)data, opcode, bitmap->Planes[plane], bitmap->BytesPerRow, sourcewidth);
				}
			}
		}
	}
}

/*
void decode_riff_int(delta,plane,rowbytes,sourcebytes)
Uint *delta;
unsigned char *plane;
int rowbytes,sourcebytes;
{
	int column,opcount;
	unsigned char *data;
	Uint ch;

	sourcebytes/=2;

	for (column=0;column<sourcebytes;column++) {
		data=(unsigned char *)(plane+(column*2));
		opcount=*delta++;
		while (opcount--) {
			ch=*delta++;
			if (ch) {
				if (!(ch&0x8000)) data+=rowbytes*ch;
				else {
					ch&=0x7fff;
					while (ch--) {
						*(Uint *)data=*delta++;
						data+=rowbytes;
					}
				}
			}
			else {
				ch=*delta++;
				while (ch--) {
					*(Uint *)data=*delta;
					data+=rowbytes;
				}
				++delta;
			}
		}
	}
}

void decode_riff_long(delta,plane,rowbytes,sourcebytes)
ULONG *delta;
unsigned char *plane;
int rowbytes,sourcebytes;
{
	int column,opcount;
	unsigned char *data;
	ULONG ch;

	sourcebytes/=4;

	for (column=0;column<sourcebytes;column++) {
		data=(unsigned char *)(plane+(column*4));
		opcount=*delta++;
		while (opcount--) {
			ch=*delta++;
			if (ch) {
				if (!(ch&0x80000000)) data+=rowbytes*ch;
				else {
					ch&=0x7fffffff;
					while (ch--) {
						*(ULONG *)data=*delta++;
						data+=rowbytes;
					}
				}
			}
			else {
				ch=*delta++;
				while (ch--) {
					*(ULONG *)data=*delta;
					data+=rowbytes;
				}
				++delta;
			}
		}
	}
}
*/

void decode_riff7_short(unsigned short *delta,
						unsigned char *opcode,
						unsigned char *plane,
						unsigned short rowbytes,
						unsigned short sourcebytes)
{
	unsigned short column, opcount;
	unsigned char *data, ch;

	// Number of source bytes
	sourcebytes >>= 1;

	// Go through columns
	for (column = 0; column < sourcebytes; column++)
	{
		// Get data pointer
		data = (unsigned char *)(plane + (column * 2));

		// Get operation count
		opcount = *opcode++;

		// Do operations
		while (opcount--)
		{
			// Get operation
			ch = *opcode++;

			// !=0 means a run
			if (ch)
			{
				// High bit not set means skip
				if (!(ch & 0x80))
					data += rowbytes * (unsigned short)ch;

				// High bit set
				else
				{
					// Mask out high bit
					ch &= 0x7f;

					// Run of data
					while (ch--)
					{
						*(unsigned short *)data = *delta++;
						data += rowbytes;
					}
				}
			}

			// ==0 means a repeated byte
			else
			{
				// Get count
				ch = *opcode++;

				// Do count
				while (ch--)
				{
					*(unsigned short *)data = *delta;
					data += rowbytes;
				}
				++delta;
			}
		}
	}
}

void decode_riff7_long(ULONG *delta,
					   unsigned char *opcode,
					   unsigned char *plane,
					   unsigned short rowbytes,
					   unsigned short sourcebytes)
{
	unsigned short column, opcount;
	unsigned char *data, ch;

	// Number of source bytes
	sourcebytes >>= 2;

	// Go through columns
	for (column = 0; column < sourcebytes; column++)
	{
		// Get data pointer
		data = (unsigned char *)(plane + (column * 4));

		// Get operation count
		opcount = *opcode++;

		// Do operations
		while (opcount--)
		{
			// Get operation
			ch = *opcode++;

			// !=0 means a run
			if (ch)
			{
				// High bit not set means skip
				if (!(ch & 0x80))
					data += rowbytes * (unsigned short)ch;

				// High bit set
				else
				{
					// Mask out high bit
					ch &= 0x7f;

					// Run of data
					while (ch--)
					{
						*(ULONG *)data = *delta++;
						data += rowbytes;
					}
				}
			}

			// ==0 means a repeated byte
			else
			{
				// Get count
				ch = *opcode++;

				// Do count
				while (ch--)
				{
					*(ULONG *)data = *delta;
					data += rowbytes;
				}
				++delta;
			}
		}
	}
}

// Print picture
short show_print(show_data *data, struct Window *help_window, ObjectList *list)
{
	struct Window *status;
	struct MsgPort *printer_port;
	union printerIO *printer_io = 0;
	struct PrinterData *printer_data;
	struct Preferences *prefs;
	short ret = 0;

	// Make window busy
	SetWindowBusy(help_window);

	// Open status window
	status = OpenStatusWindow(0, GetString(locale, MSG_OPENING_PRINTER), help_window->WScreen, 0, 0);

	// Create printer request
	if ((printer_port = CreateMsgPort()) &&
		(printer_io = (union printerIO *)CreateIORequest(printer_port, sizeof(union printerIO))) &&
		(OpenDevice("printer.device", 0, (struct IORequest *)printer_io, 0) == 0))
	{
		// Get printer data
		printer_data = (struct PrinterData *)printer_io->iodrp.io_Device;

		// Get printer preferences
		prefs = &printer_data->pd_Preferences;

		// Set preferences values
		prefs->PrintAspect = GetGadgetValue(list, GAD_PRINT_ASPECT);
		prefs->PrintImage = GetGadgetValue(list, GAD_PRINT_IMAGE);
		prefs->PrintShade = GetGadgetValue(list, GAD_PRINT_SHADE);
		prefs->PrintXOffset = 0;

		// Update status text
		SetStatusText(status, GetString(locale, MSG_INITIALISING_PRINTER));

		// Initialise printer
		printer_io->ios.io_Command = CMD_WRITE;
		printer_io->ios.io_Data = "\033#1";
		printer_io->ios.io_Length = -1;
		if (DoIO((struct IORequest *)printer_io) == 0)
		{
			// Print title?
			if (GetGadgetValue(list, GAD_PRINT_PRINT_TITLE))
			{
				char title[80];

				// Build title
				if (data->anim_ok)
				{
					lsprintf(title,
							 GetString(locale, MSG_ANIM_TITLE),
							 (IPTR)FilePart(data->file),
							 data->frame_num,
							 data->frame_count,
							 data->ilbm->header.w,
							 data->ilbm->header.h,
							 data->ilbm->header.nPlanes);
				}
				else
				{
					lsprintf(title,
							 GetString(locale, MSG_ILBM_TITLE),
							 (IPTR)FilePart(data->file),
							 data->width,
							 data->height,
							 data->depth);
				}

				// Print title
				printer_io->ios.io_Command = CMD_WRITE;
				printer_io->ios.io_Data = title;
				printer_io->ios.io_Length = -1;
				DoIO((struct IORequest *)printer_io);
			}

			// Fill out print request
			printer_io->iodrp.io_Command = PRD_DUMPRPORT;
			printer_io->iodrp.io_RastPort = data->display_window->RPort;
			printer_io->iodrp.io_ColorMap = data->display_window->WScreen->ViewPort.ColorMap;
			printer_io->iodrp.io_Modes = GetVPModeID(&data->display_window->WScreen->ViewPort);
			printer_io->iodrp.io_SrcX = 0;
			printer_io->iodrp.io_SrcY = 0;
			printer_io->iodrp.io_SrcWidth = data->display_window->Width;
			printer_io->iodrp.io_SrcHeight = data->display_window->Height;
			printer_io->iodrp.io_Special = SPECIAL_FULLCOLS | SPECIAL_ASPECT;
			if (!(GetGadgetValue(list, GAD_PRINT_FORM_FEED)))
				printer_io->iodrp.io_Special |= SPECIAL_NOFORMFEED;
			if (!(GetGadgetValue(list, GAD_PRINT_PLACEMENT)))
				printer_io->iodrp.io_Special |= SPECIAL_CENTER;

			// Send print dump
			SendIO((struct IORequest *)printer_io);

			// Update status text
			ClearWindowBusy(status);
			SetStatusText(status, GetString(locale, MSG_PRINTING_PICTURE));

			// Wait until done or aborted
			FOREVER
			{
				struct IntuiMessage *msg;
				IPCMessage *imsg;
				short quit_flag = 0;

				// Has printer request finished?
				if (CheckIO((struct IORequest *)printer_io))
				{
					ret = 1;
					break;
				}

				// IPC messages?
				if (data->ipc)
				{
					while ((imsg = (IPCMessage *)GetMsg(data->ipc->command_port)))
					{
						// Abort?
						if (imsg->command == IPC_ABORT || imsg->command == IPC_QUIT)
						{
							quit_flag = 1;
							ret = -1;
						}
						IPC_Reply(imsg);
					}
				}

				// Intuition messages?
				if (status)
				{
					while ((msg = GetWindowMsg(status->UserPort)))
					{
						// Close window?
						if (msg->Class == IDCMP_CLOSEWINDOW)
							quit_flag = 1;
						ReplyWindowMsg(msg);
					}
				}

				// Check quit flag
				if (quit_flag)
					break;

				// Wait for a message
				Wait(1 << printer_port->mp_SigBit | ((data->ipc) ? (1 << data->ipc->command_port->mp_SigBit) : 0) |
					 ((status) ? (1 << status->UserPort->mp_SigBit) : 0));
			}

			// If printer hasn't finished, abort it
			if (!(CheckIO((struct IORequest *)printer_io)))
			{
				SetStatusText(status, GetString(locale, MSG_ABORTING_PRINT));
				SetWindowBusy(status);
				AbortIO((struct IORequest *)printer_io);
			}
			WaitIO((struct IORequest *)printer_io);
		}
	}

	// Printer open?
	if (printer_io)
	{
		if (printer_io->iodrp.io_Device)
			CloseDevice((struct IORequest *)printer_io);
		DeleteIORequest((struct IORequest *)printer_io);
	}
	if (printer_port)
		DeleteMsgPort(printer_port);

	// Close status window
	CloseConfigWindow(status);

	// Make window unbusy
	ClearWindowBusy(help_window);

	return ret;
}

// Get picture through datatypes
BOOL show_get_dtpic(show_data *data, struct Node *node)
{
	// Enable DOS requesters
	((struct Process *)FindTask(NULL))->pr_WindowPtr = 0;

	// Try for datatypes
	if (DataTypesBase)
	{
		if ((data->dt_object = NewDTObject(node->ln_Name, PDTA_Remap, FALSE, DTA_GroupID, GID_PICTURE, TAG_END)))
		{
			// Get desired display info
			data->framebox.MethodID = DTM_FRAMEBOX;
			data->framebox.dtf_GInfo = 0;
			data->framebox.dtf_ContentsInfo = 0;
			data->framebox.dtf_FrameInfo = &data->frameinfo;
			data->framebox.dtf_SizeFrameInfo = sizeof(struct FrameInfo);
			data->framebox.dtf_FrameFlags = 0;
			DoMethodA(data->dt_object, (Msg)&data->framebox);

			// Do layout
			data->layout.MethodID = DTM_PROCLAYOUT;
			data->layout.gpl_GInfo = 0;
			data->layout.gpl_Initial = 1;
			if (!(DoMethodA(data->dt_object, (Msg)&data->layout)))
			{
				// Failed; free object
				DisposeDTObject(data->dt_object);
				data->dt_object = 0;
			}

			// Successfully laid out
			else
			{
				struct DataType *dt;
				IPTR modeid = data->modeid, numcolours = data->numcolours;
				// long horiz,vert;

				// Get attributes of picture (datatypes store an IPTR for each one)
				GetDTAttrs(data->dt_object,
						   PDTA_ModeID,
						   &modeid,
						   PDTA_CRegs,
						   &data->cregs,
						   PDTA_NumColors,
						   &numcolours,
						   PDTA_BitMap,
						   &data->dt_bm,
						   DTA_DataType,
						   (ULONG *)&dt,
						   /*DTA_NominalVert,&vert,
						   DTA_NominalHoriz,&horiz,*/
						   TAG_END);
				data->modeid = modeid;
				data->numcolours = numcolours;

#ifdef __AROS__
				if (!data->dt_bm)
					GetDTAttrs(data->dt_object, PDTA_DestBitMap, &data->dt_bm, TAG_END);
#endif

				// Store screen size
				data->width = data->frameinfo.fri_Dimensions.Width;
				data->height = data->frameinfo.fri_Dimensions.Height;
				data->depth = data->frameinfo.fri_Dimensions.Depth;

				// AROS can return a laid-out picture datatype without a usable bitmap
				if (!data->dt_bm || data->width <= 0 || data->height <= 0 || data->depth <= 0)
				{
					DisposeDTObject(data->dt_object);
					data->dt_object = 0;
					data->cregs = 0;
					data->dt_bm = 0;
					data->numcolours = 0;
					((struct Process *)FindTask(NULL))->pr_WindowPtr = (APTR)-1;
					return 0;
				}

				// Get type
				if (dt)
					data->picture_type = dt->dtn_Header->dth_Name;
				else
					data->picture_type = 0;

				// Get number of colours if not set
				if (!data->numcolours)
					data->numcolours = 2 << (data->depth - 1);

				// Disable DOS requesters
				((struct Process *)FindTask(NULL))->pr_WindowPtr = (APTR)-1;

				// Return success
				data->pic_ok = 1;
				return 1;
			}
		}
	}

	// Disable DOS requesters
	((struct Process *)FindTask(NULL))->pr_WindowPtr = (APTR)-1;

	// Failed
	return 0;
}
