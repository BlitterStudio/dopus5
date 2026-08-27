/*

Directory Opus 5
Original APL release version 5.82
Copyright 1993-2012 Jonathan Potter & GP Software
Copyright 2012-2013 DOPUS5 Open Source Team
Copyright 2023-2026 Dimitris Panokostas (dopus5 fork)

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

#include <proto/configopus.h>

IPC_EntryProto(start_proc, static);

// Open a new start menu
IPCData *start_new(char *buttons, char *label, char *image, short x, short y)
{
	StartMenu *menu;
	IPCData *ipc;

	// Allocate data
	if (!(menu = AllocVec(sizeof(StartMenu), MEMF_CLEAR)) || !(menu->memory = NewMemHandle(1024, 512, MEMF_CLEAR)))
	{
		FreeVec(menu);
		return 0;
	}

	// Fill it out
	if (buttons)
		strcpy(menu->buttons, buttons);
	if (label)
		strcpy(menu->data.label, label);
	if (image)
		strcpy(menu->data.image, image);

	// Initialise coordinates
	menu->x = x;
	menu->y = y;

	// Launch start process
	if ((IPC_Launch(&GUI->startmenu_list,
					&ipc,
					"dopus_start_menu",
					(IPTR)&start_proc,
					STACK_DEFAULT,
					(IPTR)menu,
					(struct Library *)DOSBase)) &&
		ipc)
		return ipc;

	// Failed
	if (!ipc)
	{
		FreeMemHandle(menu->memory);
		FreeVec(menu);
	}
	return 0;
}

// Initialise a start menu
IPC_StartupCode(start_init, StartMenu *, menu, static)
{
	// Store IPC pointer
	menu->ipc = ipc;

	// Set menu pointer
	SET_IPCDATA(menu->ipc, menu);

	// Create a new bank?
	if (!menu->buttons[0])
		menu->bank = NewButtonBank(0, 0);

	// Load button bank information
	else
		menu->bank = OpenButtonBank(menu->buttons);

	// No bank?
	if (!menu->bank)
		return 0;

	// Get current path
	stccpy(menu->buttons, menu->bank->path, sizeof(menu->buttons));

	// If bank doesn't have start menu information, allocate it
	if (!menu->bank->startmenu)
	{
		// Allocate it
		if ((menu->bank->startmenu = AllocMemH(menu->bank->memory, sizeof(CFG_STRT))))
		{
			// Store label/image
			CopyMem((char *)&menu->data, (char *)menu->bank->startmenu, sizeof(CFG_STRT));
			menu->data.fpen = 1;
			menu->data.label_fpen = 1;
			menu->data.sel_fpen = 2;
			menu->flags = STRTF_VERSION2;
		}
	}

	// Otherwise, copy from the bank
	else
	{
		// Get label/image
		CopyMem((char *)menu->bank->startmenu, (char *)&menu->data, sizeof(CFG_STRT));
	}

	// Build menu
	if ((menu->menu = popup_from_bank(menu->bank, &menu->last_id)))
		menu->menu->ph_Menu.flags |= POPUPMF_ABOVE;

	// Get coordinates if not supplied
	if (menu->x == -1 || menu->y == -1)
	{
		// Get position from button
		menu->x = menu->bank->window.pos.Left;
		menu->y = menu->bank->window.pos.Top;
	}

	// Create message port
	menu->app_port = CreateMsgPort();

	// Initialise semaphore lock
	InitSemaphore(&menu->lock);
	return 1;
}

// Start menu process
IPC_EntryCode(start_proc, static)
{
	StartMenu *menu = 0;
	short quit_flag = 0;

	// Do startup
	if (!(IPC_ProcStartup((IPTR *)&menu, (ULONG (*)(IPCData *, APTR))&start_init)))
	{
		// Failed
		start_cleanup(menu);
		return;
	}

	// If path is empty, go straight into edit mode
	if (!menu->buttons[0])
	{
		// If edit is cancelled, don't open
		if (!(start_edit(menu)))
		{
			// Clean up
			start_cleanup(menu);
			return;
		}

		// Make sure button is set to have a drag bar
		menu->bank->window.flags &= ~BTNWF_NO_BORDER;

		// Show bank
		if (GUI->window)
			start_show(menu, GUI->screen_pointer);
	}

	// Event loop
	FOREVER
	{
		IPCMessage *imsg;

		// Commands
		while ((imsg = (IPCMessage *)GetMsg(menu->ipc->command_port)))
		{
			// Look at command
			switch (imsg->command)
			{
			// Quit
			case IPC_QUIT:
				quit_flag = 1;
				break;

			// Hide
			case IPC_HIDE:
				start_hide(menu);
				break;

			// Show
			case IPC_SHOW:
				start_show(menu, (struct Screen *)imsg->data);
				break;

			// Open
			case IPC_OPEN:
				quit_flag = start_popup(menu);
				break;
			}

			// Reply the message
			IPC_Reply(imsg);
		}

		// AppWindow messages
		if (menu->app_window)
		{
			struct AppMessage *msg;
			short arg;

			// Get messages
			while ((msg = (struct AppMessage *)GetMsg(menu->app_port)))
			{
				// Go through arguments
				for (arg = 0; arg < msg->am_NumArgs; arg++)
				{
					char path[256];

					// Get pathname
					if (GetWBArgPath(&msg->am_ArgList[arg], path, 256))
					{
						// Add to start menu
						start_add_item(menu, path);
					}
				}

				// Reply to message
				ReplyMsg((struct Message *)msg);
			}
		}

		// Window messages
		if (menu->window)
		{
			struct IntuiMessage *msg;

			// Get messages
			while ((msg = (struct IntuiMessage *)GetMsg(menu->window->UserPort)))
			{
				struct IntuiMessage msg_copy;

				// Copy message and reply
				msg_copy = *msg;
				ReplyMsg((struct Message *)msg);

				// Mouse button down?
				if (msg_copy.Class == IDCMP_MOUSEBUTTONS && !(msg_copy.Code & IECODE_UP_PREFIX))
				{
					UWORD res;
					PopUpItem *item;

					// Over the drag gadget?
					if (!(menu->bank->window.flags & BTNWF_NO_BORDER) && msg_copy.MouseX >= menu->drag.LeftEdge &&
						msg_copy.MouseY >= menu->drag.TopEdge &&
						msg_copy.MouseX < menu->drag.LeftEdge + menu->drag.Width &&
						msg_copy.MouseY < menu->drag.TopEdge + menu->drag.Height)
					{
						// Do pop-up
						if (msg_copy.Code == MENUDOWN)
							quit_flag = start_popup(menu);
					}

					// Over the 'button'?
					else if (msg_copy.MouseX >= menu->button.MinX && msg_copy.MouseY >= menu->button.MinY &&
							 msg_copy.MouseX <= menu->button.MaxX && msg_copy.MouseY <= menu->button.MaxY)
					{
						// Control key does things
						if (msg_copy.Qualifier & IEQUALIFIER_CONTROL)
						{
							// Left button toggles the drag gadget
							if (msg_copy.Code == SELECTDOWN)
								start_toggle_drag(menu, -1);

							// Do pop-up for right button
							else if (msg_copy.Code == MENUDOWN)
								quit_flag = start_popup(menu);
							break;
						}

						// Recess border
						start_draw_button(menu, 1, 0);

						// Set popped flag
						menu->flags |= STARTMENUF_POPPED;

						// Do pop-up menu
						if (menu->menu &&
							(res = DoPopUpMenu(menu->window, &menu->menu->ph_Menu, &item, msg_copy.Code)) != (UWORD)-1)
						{
							// Get item; check it's not a sub-item header
							if (item && !(item->flags & POPUPF_SUB))
							{
								Cfg_Function *function;

								// Get function
								function = (Cfg_Function *)item->data;

								// Help?
								if (res & POPUP_HELPFLAG)
									help_button_help(0, function, msg_copy.Qualifier, 0, 0);

								// Otherwise
								else
								{
									// Launch function
									function_launch_quick(FUNCTION_RUN_FUNCTION, function, 0);
								}
							}
						}

						// Clear popped flag
						menu->flags &= ~STARTMENUF_POPPED;

						// Fix border
						start_draw_button(menu, 0, 0);
					}
				}

				// New size means border has been toggled, need to redraw
				else if (msg_copy.Class == IDCMP_NEWSIZE)
				{
					// Fix size of 'button'
					start_fix_button(menu);

					// Redraw the button
					start_draw_button(menu, 0, 2);
				}

				// Window moved
				else if (msg_copy.Class == IDCMP_CHANGEWINDOW)
				{
					// Has it moved?
					if (menu->window->LeftEdge != menu->bank->window.pos.Left ||
						menu->window->TopEdge != menu->bank->window.pos.Top)
					{
						// Set change flag
						menu->changed = 1;

						// Store new position
						menu->bank->window.pos.Left = menu->window->LeftEdge;
						menu->bank->window.pos.Top = menu->window->TopEdge;
					}
				}

				// Key press
				else if (msg_copy.Class == IDCMP_RAWKEY)
				{
					Cfg_Function *function;

					// Can we match a function key?
					if ((function =
							 match_function_key(msg_copy.Code, msg_copy.Qualifier, menu->bank, 0, WINDOW_START, 0)))
					{
						// Run function
						function_launch_quick(FUNCTION_RUN_FUNCTION, function, 0);
					}
				}

				// Window active
				else if (msg_copy.Class == IDCMP_ACTIVEWINDOW)
				{
					// Check pointer is over window
					if (msg_copy.MouseX >= 0 && msg_copy.MouseY >= 0 && msg_copy.MouseX < menu->window->Width &&
						msg_copy.MouseY < menu->window->Height)
					{
						// Bring window to the front
						WindowToFront(menu->window);
					}
				}

				// Is window still open?
				if (!menu->window)
					break;
			}
		}

		// Quit?
		if (quit_flag || (GUI->flags & GUIF_PENDING_QUIT))
			break;

		// Wait for events
		Wait(1 << menu->ipc->command_port->mp_SigBit | ((menu->window) ? 1 << menu->window->UserPort->mp_SigBit : 0) |
			 ((menu->app_window) ? 1 << menu->app_port->mp_SigBit : 0));
	}

	// Clean up
	start_cleanup(menu);
}

// Clean up start process
void start_cleanup(StartMenu *menu)
{
	IPCData *ipc;

	// Invalid?
	if (!menu)
		return;

	// Save IPC pointer
	ipc = menu->ipc;

	// Close display
	start_hide(menu);

	// Free buttons
	CloseButtonBank(menu->bank);

	// Free message port
	DeleteMsgPort(menu->app_port);

	// Free data
	PopUpFreeHandle(menu->menu);
	FreeMemHandle(menu->memory);
	FreeVec(menu);

	// Forbid so we don't get unloaded too soon
	Forbid();

	// Flush port
	IPC_Flush(ipc);

	// Send goodbye
	IPC_Goodbye(ipc, &main_ipc, 0);

	// Free IPC
	IPC_Free(ipc);
}

#define DRAG_WIDTH 8

// Show start menu
void start_show(StartMenu *menu, struct Screen *screen)
{
	short width = 0, height = 0;
	struct TagItem tags[3];
	struct TextAttr font;

	// Already visible?
	if (menu->window)
		return;

	// Got an image?
	if (menu->data.image[0] && (menu->imptr = OpenImage(menu->data.image, 0)))
	{
		// Get tags for image size
		tags[0].ti_Tag = IM_Width;
		tags[1].ti_Tag = IM_Height;
		tags[2].ti_Tag = TAG_END;

		// Get image size
		GetImageAttrs(menu->imptr, tags);

		// Store image size
		menu->im_width = tags[0].ti_Data;
		menu->im_height = tags[1].ti_Data;

		// Increase size by image size
		width = tags[0].ti_Data;
		height = tags[1].ti_Data;
	}

	// No label?
	else if (!menu->data.label[0])
		strcpy(menu->data.label, "Start");

	// Fill out font structure for main font
	font.ta_Name = menu->bank->window.font_name;
	font.ta_YSize = menu->bank->window.font_size;
	font.ta_Style = 0;
	font.ta_Flags = 0;

	// Get menu font
	if ((menu->font = OpenDiskFont(&font)))
	{
		if (menu->menu)
			menu->menu->ph_Menu.flags |= POPUPMF_FONT;
	}
	else
	{
		if (menu->menu)
			menu->menu->ph_Menu.flags &= ~POPUPMF_FONT;
	}

	// Set menu font pointer
	if (menu->menu)
		menu->menu->ph_Menu.font = (menu->font) ? menu->font : screen->RastPort.Font;

	// Label font?
	if (menu->data.label_fontname[0])
	{
		// Fill out font structure for label font
		font.ta_Name = menu->data.label_fontname;
		font.ta_YSize = menu->data.label_fontsize;

		// Get label font
		menu->label_font = OpenDiskFont(&font);
	}

	// Get size from label
	if (menu->data.label[0])
	{
		struct RastPort rp;

		// Initialise fake rastport with font
		InitRastPort(&rp);
		SetFont(&rp, (menu->label_font) ? menu->label_font : (menu->menu) ? menu->menu->ph_Menu.font : rp.Font);

		// Get label size
		menu->label_width = TextLength(&rp, menu->data.label, strlen(menu->data.label));
		width += menu->label_width;
		if (rp.TxHeight > height)
			height = rp.TxHeight;
		if (menu->imptr)
			width += 2;
	}

	// Initialise drag gadget
	menu->drag.NextGadget = 0;
	menu->drag.LeftEdge = 0;
	menu->drag.TopEdge = 0;
	menu->drag.GadgetType = GTYP_WDRAGGING;
	menu->drag.Activation = GACT_IMMEDIATE | GACT_RELVERIFY;
	menu->drag.Flags = GFLG_GADGHNONE | GFLG_GADGIMAGE | GFLG_RELHEIGHT;
	menu->drag.GadgetID = DRAG_GADGET;
	menu->drag.Width = DRAG_WIDTH;

	// Get image for button
	menu->drag.GadgetRender =
		NewObject(0, "dopusiclass", DIA_Type, IM_BBOX, IA_Width, DRAG_WIDTH, IA_Height, 0, TAG_END);

	// Not borderless?
	if (!(menu->bank->window.flags & BTNWF_BORDERLESS))
	{
		// Increase size for borders
		width += 4;
		height += 4;
	}

	// Want drag gadget?
	if (!(menu->bank->window.flags & BTNWF_NO_BORDER))
	{
		// Increase size for drag gadget
		width += menu->drag.Width;

		// Drag gadget on right side?
		if (menu->bank->window.flags & BTNWF_DRAG_RIGHT)
			menu->drag.LeftEdge = width - menu->drag.Width;
	}

	// Coordinates for bottom-left?
	if (menu->y == -1)
		menu->y = screen->Height - height;

	// Initialise popup menu to no backfill or pen
	menu->menu->ph_Menu.flags &= ~(POPUPMF_BACKFILL | POPUPMF_USE_PEN | POPUPMF_PEN(255));
	menu->menu->ph_Menu.flags |= POPUPMF_ABOVE;

	// Load background picture
	if (menu->bank->backpic[0])
	{
		// Fill out WBPatternPrefs
		menu->pattern_data.prefs.wbp_Flags = 0;
		menu->pattern_data.prefs.wbp_Which = PATTERN_BUTTONS;
		menu->pattern_data.prefs.wbp_DataLength = strlen(menu->bank->backpic);
		menu->pattern_data.data = menu->bank->backpic;
		menu->pattern_data.precision = 3;
		menu->pattern_data.flags = PATF_TILE;
		GetPattern(&menu->pattern_data, screen, 0);

// Initialise backfill hook
#if defined(__MORPHOS__)
		menu->pattern.hook.h_Entry = (HOOKFUNC)HookEntry;
		menu->pattern.hook.h_SubEntry = (ULONG(*)())PatternBackfill;
#else
		menu->pattern.hook.h_Entry = (ULONG(*)())PatternBackfill;
#endif
		menu->pattern.hook.h_Data = 0;
		menu->pattern.pattern = &menu->pattern_data;

		// Set popup menu to use hook
		if (menu->pattern_data.valid)
		{
			menu->menu->ph_Menu.flags |= POPUPMF_BACKFILL;
			menu->menu->ph_Menu.backfill = (struct Hook *)&menu->pattern;
		}
	}

	// Set menu pen
	if (menu->data.flags & STRTF_VERSION2)
	{
		short fpen;
		fpen = menu->data.fpen;
		if (fpen >= 4 && fpen < 252)
			fpen = GUI->pens[fpen - 4];
		menu->menu->ph_Menu.flags |= POPUPMF_USE_PEN | POPUPMF_PEN(fpen);
	}

	// Open window
	if (!(menu->window = OpenWindowTags(
			  0,
			  WA_Left,
			  menu->x,
			  WA_Top,
			  menu->y,
			  WA_Width,
			  width,
			  WA_Height,
			  height,
			  WA_IDCMP,
			  IDCMP_MOUSEBUTTONS | IDCMP_ACTIVEWINDOW | IDCMP_NEWSIZE | IDCMP_CHANGEWINDOW | IDCMP_RAWKEY,
			  WA_Borderless,
			  TRUE,
			  WA_RMBTrap,
			  TRUE,
			  WA_CustomScreen,
			  screen,
			  WA_ScreenTitle,
			  GUI->screen_title,
			  WA_WindowName,
			  DOPUS_WIN_NAME,
			  (menu->pattern_data.valid) ? WA_BackFill : TAG_IGNORE,
			  &menu->pattern,
			  TAG_END)))
	{
		// Clean up
		start_hide(menu);
		return;
	}

	// Set font in window
	SetFont(
		menu->window->RPort,
		(menu->label_font) ? menu->label_font : (menu->menu) ? menu->menu->ph_Menu.font : menu->window->RPort->Font);

	// Got port?
	if (menu->app_port)
	{
		// Add AppWindow
		menu->app_window = AddAppWindowA(0, 0, menu->window, menu->app_port, 0);
	}

	// Got drag image?
	if (menu->drag.GadgetRender)
	{
		// Fix height
		menu->drag.Height = menu->window->Height;
		((struct Image *)menu->drag.GadgetRender)->Height = menu->window->Height;
	}

	// Want drag gadget?
	if (!(menu->bank->window.flags & BTNWF_NO_BORDER))
	{
		// Add dragging gadget
		AddGList(menu->window, &menu->drag, -1, 1, 0);
		RefreshGList(&menu->drag, menu->window, 0, -1);
	}

	// Initialise ID
	SetWindowID(menu->window, &menu->id, WINDOW_START, (struct MsgPort *)menu);

	// Get drawinfo
	menu->drawinfo = GetScreenDrawInfo(menu->window->WScreen);

	// Get size of 'button'
	start_fix_button(menu);

	// Remap image
	if (menu->imptr)
		RemapImage(menu->imptr, menu->window->WScreen, &menu->remap);

	// Draw button
	start_draw_button(menu, 0, 1);

	// Remap bank images
	if (menu->menu)
		start_remap_bank(menu, (struct List *)&menu->menu->ph_Menu, 0);
}

// Draw button
void start_draw_button(StartMenu *menu, short recessed, short all)
{
	struct RastPort *rp;

	// Cache RastPort
	rp = menu->window->RPort;

	// Clear button?
	if (all)
	{
		short border = 0;

		// Extra for border
		if (!(menu->bank->window.flags & BTNWF_BORDERLESS))
			border = 1;

		// Erase button
		EraseRect(rp,
				  menu->button.MinX + border,
				  menu->button.MinY + border,
				  menu->button.MaxX - border,
				  menu->button.MaxY - border);
	}

	// Draw button border unless borderless
	if (!(menu->bank->window.flags & BTNWF_BORDERLESS))
		DrawBox(rp, &menu->button, menu->drawinfo, recessed);

	// Got image?
	if (menu->imptr)
	{
		// Don't draw if no alt image
		if (all || !menu->no_alt)
		{
			struct TagItem draw_tags[6];

			// Fill out tags
			draw_tags[0].ti_Tag = IM_State;
			draw_tags[0].ti_Data = recessed;
			draw_tags[1].ti_Tag = IM_Erase;
			draw_tags[1].ti_Data = 0;
			draw_tags[2].ti_Tag = IM_NoDrawInvalid;
			draw_tags[2].ti_Data = 1;
			draw_tags[3].ti_Tag = IM_Backfill;
			draw_tags[3].ti_Data = menu->pattern_data.valid;
			draw_tags[4].ti_Tag = IM_Mask;
			draw_tags[4].ti_Data = menu->pattern_data.valid;
			draw_tags[5].ti_Tag = TAG_END;

			// Draw image
			if (!(RenderImage(rp,
							  menu->imptr,
							  menu->button.MinX + ((menu->bank->window.flags & BTNWF_BORDERLESS) ? 0 : 2),
							  ((menu->window->Height - menu->im_height) >> 1),
							  draw_tags)))
			{
				// No alt image?
				if (recessed)
					menu->no_alt = 1;
			}
		}
	}

	// Got label?
	if (menu->data.label[0] && all)
	{
		short fg;

		// Get pen
		if (menu->data.flags & STRTF_VERSION2)
		{
			fg = menu->data.label_fpen;
			if (fg >= 4 && fg < 252)
				fg = GUI->pens[fg - 4];
		}
		else
			fg = GUI->draw_info->dri_Pens[TEXTPEN];

		// Set pen
		SetAPen(rp, fg);
		SetDrMd(rp, JAM1);

		// Position for text
		Move(rp,
			 menu->button.MaxX - menu->label_width - ((menu->bank->window.flags & BTNWF_BORDERLESS) ? -1 : 2),
			 ((menu->window->Height - rp->TxHeight) >> 1) + rp->TxBaseline);

		// Draw label
		Text(rp, menu->data.label, strlen(menu->data.label));
	}
}

// Hide start menu
void start_hide(StartMenu *menu)
{
	// Open?
	if (menu->window)
	{
		// Free remap bank images
		if (menu->menu)
			start_remap_bank(menu, (struct List *)&menu->menu->ph_Menu, 1);

		// Got image?
		if (menu->imptr)
		{
			// Free image remap
			FreeRemapImage(menu->imptr, &menu->remap);
		}

		// Free remap stuff
		FreeImageRemap(&menu->remap);

		// Save position
		menu->x = menu->window->LeftEdge;
		menu->y = menu->window->TopEdge;

		// Remove AppWindow
		if (menu->app_window)
			RemoveAppWindow(menu->app_window);
		menu->app_window = 0;

		// Free pattern
		FreePattern(&menu->pattern_data);

		// Free draw info
		if (menu->drawinfo)
		{
			FreeScreenDrawInfo(menu->window->WScreen, menu->drawinfo);
			menu->drawinfo = 0;
		}

		// Close window
		CloseWindow(menu->window);
		menu->window = 0;

		// Close fonts
		if (menu->font)
		{
			CloseFont(menu->font);
			menu->font = 0;
		}
		if (menu->label_font)
		{
			CloseFont(menu->label_font);
			menu->label_font = 0;
		}
	}

	// Got image?
	if (menu->imptr)
	{
		// Free image
		CloseImage(menu->imptr);
		menu->imptr = 0;
	}

	// Free drag image
	if (menu->drag.GadgetRender)
	{
		DisposeObject((Object *)menu->drag.GadgetRender);
		menu->drag.GadgetRender = 0;
	}
}

// Start menu popup
BOOL start_popup(StartMenu *menu)
{
	PopUpHandle *handle;
	PopUpItem *item;
	UWORD res;

	// Create menu
	if (!(handle = PopUpNewHandle(0, 0, &locale)))
		return 0;

	// Backfill hook?
	if (menu->pattern_data.valid)
	{
		handle->ph_Menu.flags |= POPUPMF_BACKFILL;
		handle->ph_Menu.backfill = (struct Hook *)&menu->pattern;
	}

	// Set menu pen
	if (menu->data.flags & STRTF_VERSION2)
	{
		short fpen;
		fpen = menu->data.fpen;
		if (fpen >= 4 && fpen < 252)
			fpen = GUI->pens[fpen - 4];
		handle->ph_Menu.flags |= POPUPMF_USE_PEN | POPUPMF_PEN(fpen);
	}

	// Set popped flag
	menu->flags |= STARTMENUF_POPPED;

	// Build start menu popup
	PopUpNewItem(handle, MSG_EDIT_MENU, MENU_EDIT_STARTMENU, 0);
	if ((item = PopUpNewItem(handle, MSG_START_APPEARANCE, 0, POPUPF_SUB)))
	{
		// Create list for submenu
		if (PopUpItemSub(handle, item))
		{
			short border = 0;

			// Get border state
			if (!(menu->bank->window.flags & BTNWF_NO_BORDER))
				border = (menu->bank->window.flags & BTNWF_DRAG_RIGHT) ? 2 : 1;

			// Add appearance items
			PopUpNewItem(handle, MSG_START_IMAGE, MENU_START_IMAGE, 0);
			PopUpNewItem(handle, MSG_START_LABEL, MENU_START_LABEL, 0);
			PopUpNewItem(handle, MSG_START_FONT, MENU_START_FONT, 0);
			PopUpNewItem(handle, MSG_START_LABEL_FONT, MENU_START_LABEL_FONT, 0);
			PopUpNewItem(handle, MSG_START_PICTURE, MENU_START_PICTURE, 0);
			PopUpSeparator(handle);

			// Drag bar sub-menu
			if ((item = PopUpNewItem(handle, MSG_START_DRAGBAR, 0, POPUPF_SUB)))
			{
				// Create list for submenu
				if (PopUpItemSub(handle, item))
				{
					// Add sub-menu items
					PopUpNewItem(handle,
								 MSG_START_DRAGBAR_LEFT,
								 MENU_START_DRAGBAR_LEFT,
								 POPUPF_CHECKIT | ((border == 1) ? POPUPF_CHECKED : 0));
					PopUpNewItem(handle,
								 MSG_START_DRAGBAR_RIGHT,
								 MENU_START_DRAGBAR_RIGHT,
								 POPUPF_CHECKIT | ((border == 2) ? POPUPF_CHECKED : 0));
					PopUpNewItem(handle,
								 MSG_START_DRAGBAR_OFF,
								 MENU_START_DRAGBAR_OFF,
								 POPUPF_CHECKIT | ((border == 0) ? POPUPF_CHECKED : 0));
				}

				// End submenu
				PopUpEndSub(handle);
			}

			// More items
			PopUpNewItem(handle,
						 MSG_START_BORDERLESS,
						 MENU_START_BORDERLESS,
						 POPUPF_CHECKIT | ((menu->bank->window.flags & BTNWF_BORDERLESS) ? 0 : POPUPF_CHECKED));
			PopUpNewItem(handle,
						 MSG_START_SCALING,
						 MENU_START_SCALING,
						 POPUPF_CHECKIT | ((menu->bank->window.flags & BTNWF_NO_SCALE_IMAGES) ? 0 : POPUPF_CHECKED));

			// End submenu
			PopUpEndSub(handle);
		}
	}

	PopUpSeparator(handle);
	if (menu->changed)
		PopUpNewItem(handle, MSG_SAVE, MENU_START_SAVE, 0);
	PopUpNewItem(handle, MSG_CLOSE, MENU_CLOSE_BUTTONS, 0);

	// Do pop-up menu
	res = DoPopUpMenu(menu->window, &handle->ph_Menu, 0, MENUDOWN);

	// Free menu
	PopUpFreeHandle(handle);

	// Clear popped flag
	menu->flags &= ~STARTMENUF_POPPED;

	// Look at selection
	switch (res)
	{
	// Close
	case MENU_CLOSE_BUTTONS:
		return 1;

	// Toggle button border
	case MENU_START_BORDERLESS:

		// Toggle the border
		start_toggle_border(menu);
		break;

	// Toggle image scaling
	case MENU_START_SCALING:

		// Toggle flag
		menu->bank->window.flags ^= BTNWF_NO_SCALE_IMAGES;
		if (menu->menu)
		{
			if (menu->bank->window.flags & BTNWF_NO_SCALE_IMAGES)
				menu->menu->ph_Menu.flags |= POPUPMF_NO_SCALE;
			else
				menu->menu->ph_Menu.flags &= ~POPUPMF_NO_SCALE;
		}

		// Set change flag
		menu->changed = 1;
		break;

	// Toggle drag bar
	case MENU_START_DRAGBAR_LEFT:
	case MENU_START_DRAGBAR_RIGHT:
	case MENU_START_DRAGBAR_OFF: {
		short state;

		// Turn off
		if (res == MENU_START_DRAGBAR_OFF)
		{
			if (menu->bank->window.flags & BTNWF_NO_BORDER)
				break;
			state = -1;
		}

		// Left
		else if (res == MENU_START_DRAGBAR_LEFT)
		{
			state = 1;
			menu->bank->window.flags &= ~BTNWF_DRAG_RIGHT;
		}

		// Right
		else
		{
			state = 1;
			menu->bank->window.flags |= BTNWF_DRAG_RIGHT;
		}

		// Refresh the drag gadget
		start_toggle_drag(menu, state);
	}
	break;

	// Edit
	case MENU_EDIT_STARTMENU:
		start_edit(menu);
		break;

	// Change image
	case MENU_START_IMAGE:
		start_change_image(menu);
		break;

	// Change label
	case MENU_START_LABEL:
		start_change_label(menu);
		break;

	// Change font
	case MENU_START_FONT:
	case MENU_START_LABEL_FONT:
		start_change_font(menu, res);
		break;

	// Change picture
	case MENU_START_PICTURE:
		start_change_picture(menu);
		break;

	// Save
	case MENU_START_SAVE:
		start_save(menu);
		break;
	}

	return 0;
}

// Toggle the drag gadget
void start_toggle_drag(StartMenu *menu, short state)
{
	short left, width;

	// Got drag image?
	if (!menu->drag.GadgetRender)
		return;

	// Get position and width of window
	left = menu->window->LeftEdge;
	width = menu->window->Width;

	// Gadget currently on?
	if (!(menu->bank->window.flags & BTNWF_NO_BORDER))
	{
		// Turn it off?
		if (!state || state == -1)
		{
			// Turn off
			menu->bank->window.flags |= BTNWF_NO_BORDER;

			// Decrease window size
			width -= menu->drag.Width;

			// Remove dragging gadget
			RemoveGList(menu->window, &menu->drag, 1);

			// If border was on the right, shift window over
			if (menu->bank->window.flags & BTNWF_DRAG_RIGHT)
				left += menu->drag.Width;
		}
	}

	// Currently off
	else if (menu->bank->window.flags & BTNWF_NO_BORDER)
	{
		// Turn it on?
		if (state)
		{
			// Turn on
			menu->bank->window.flags &= ~BTNWF_NO_BORDER;

			// Increase window size
			width += menu->drag.Width;

			// Add dragging gadget
			AddGList(menu->window, &menu->drag, -1, 1, 0);

			// If border is on the right, shift window over
			if (menu->bank->window.flags & BTNWF_DRAG_RIGHT)
				left -= menu->drag.Width;
		}
	}

	// Drag gadget on right side?
	if (menu->bank->window.flags & BTNWF_DRAG_RIGHT)
		menu->drag.LeftEdge = width - menu->drag.Width;
	else
		menu->drag.LeftEdge = 0;

	// Resize/reposition the window
	ChangeWindowBox(menu->window, left, menu->window->TopEdge, width, menu->window->Height);

	// Refresh drag gadget if turned on
	if (!(menu->bank->window.flags & BTNWF_NO_BORDER))
		RefreshGList(&menu->drag, menu->window, 0, -1);

	// Set change flag
	menu->changed = 1;
}

// Toggle the border
void start_toggle_border(StartMenu *menu)
{
	short width, height;

	// Get size of window
	width = menu->window->Width;
	height = menu->window->Height;

	// Borders currently on?
	if (!(menu->bank->window.flags & BTNWF_BORDERLESS))
	{
		// Turn off
		menu->bank->window.flags |= BTNWF_BORDERLESS;

		// Decrease window size
		width -= 4;
		height -= 4;
	}

	// Currently on
	else
	{
		// Turn on
		menu->bank->window.flags &= ~BTNWF_BORDERLESS;

		// Increase window size
		width += 4;
		height += 4;
	}

	// Got drag image?
	if (menu->drag.GadgetRender)
	{
		// Fix height
		menu->drag.Height = height;
		((struct Image *)menu->drag.GadgetRender)->Height = height;

		// Drag gadget on right side?
		if (menu->bank->window.flags & BTNWF_DRAG_RIGHT)
			menu->drag.LeftEdge = width - menu->drag.Width;
	}

	// Resize the window
	ChangeWindowBox(menu->window, menu->window->LeftEdge, menu->window->TopEdge, width, height);

	// Set change flag
	menu->changed = 1;
}

// Fix size of button
void start_fix_button(StartMenu *menu)
{
	// Got a drag bar?
	if (!(menu->bank->window.flags & BTNWF_NO_BORDER))
	{
		// Drag bar on right side?
		if (menu->bank->window.flags & BTNWF_DRAG_RIGHT)
		{
			menu->button.MinX = 0;
			menu->button.MaxX = menu->drag.LeftEdge - 1;
		}

		// Drag bar on left side
		else
		{
			menu->button.MinX = menu->drag.Width;
			menu->button.MaxX = menu->window->Width - 1;
		}
	}

	// No drag bar
	else
	{
		menu->button.MinX = 0;
		menu->button.MaxX = menu->window->Width - 1;
	}

	// Get height
	menu->button.MinY = 0;
	menu->button.MaxY = menu->window->Height - 1;
}

// Edit the start menu
BOOL start_edit(StartMenu *menu)
{
#ifndef __amigaos3__
	struct Library *ConfigOpusBase;
	#ifdef __amigaos4__
	struct ConfigOpusIFace *IConfigOpus;
	#endif
#endif
	Cfg_ButtonBank *bank;
	BOOL ret = 0, show = 0;

	// Open config library
	if (!(ConfigOpusBase = OpenModule(config_name))
#ifdef __amigaos4__
		|| !(IConfigOpus = (struct ConfigOpusIFace *)GetInterface(ConfigOpusBase, "main", 1, NULL))
#endif
	)
	{
		CloseLibrary(ConfigOpusBase);  // In case module opens but interface doesn't
		return 0;
	}

	// Currently visible?
	if (menu->window)
	{
		// Hide the start menu
		start_hide(menu);
		show = 1;
	}

	// Store label/image
	if (menu->bank->startmenu)
		CopyMem((char *)&menu->data, (char *)menu->bank->startmenu, sizeof(CFG_STRT));

	// Edit the current bank
	if ((bank = (Cfg_ButtonBank *)Config_Menus(
			 menu->ipc, &main_ipc, GUI->screen_pointer, menu->bank, (IPTR)&GUI->command_list.list, 3, menu->buttons)))
	{
		// Lock bank lock
		GetSemaphore(&menu->lock, SEMF_EXCLUSIVE, 0);

		// Free existing bank
		CloseButtonBank(menu->bank);

		// Free menu
		ClearMemHandle(menu->memory);

		// Use new bank
		menu->bank = bank;

		// Build new menu
		PopUpFreeHandle(menu->menu);
		menu->menu = popup_from_bank(menu->bank, &menu->last_id);

		// If changed, set flag
		if (menu->bank->window.flags & BTNWF_CHANGED)
			menu->changed = 1;

		// Get current path
		stccpy(menu->buttons, menu->bank->path, sizeof(menu->buttons));

		// Set result
		ret = 1;

		// Free bank lock
		FreeSemaphore(&menu->lock);
	}

	// Show start menu
	if (show)
		start_show(menu, GUI->screen_pointer);

// Close config library
#ifdef __amigaos4__
	DropInterface((struct Interface *)IConfigOpus);
#endif
	CloseLibrary(ConfigOpusBase);
	return ret;
}

// Save the start menu
BOOL start_save(StartMenu *menu)
{
	BOOL ret = 0;

	// Set busy pointer
	SetBusyPointer(menu->window);

	// Store label/image
	if (menu->bank->startmenu)
		CopyMem((char *)&menu->data, (char *)menu->bank->startmenu, sizeof(CFG_STRT));

	// Save the button bank
	if (!SaveButtonBank(menu->bank, menu->buttons))
	{
		menu->changed = 0;
		ret = 1;
	}

	// Reset pointer
	ClearPointer(menu->window);

	return ret;
}

// Change the image
void start_change_image(StartMenu *menu)
{
	// Hide the start menu
	start_hide(menu);

	// Request a new image
	if (request_file(
			GUI->window, GetString(&locale, MSG_START_SELECT_IMAGE), menu->data.image, "DOpus5:Images/", 1 << 31, 0))
	{
		// Set change flag
		menu->changed = 1;
	}

	// Show start menu (complete with new image)
	start_show(menu, GUI->screen_pointer);
}

// Change the label
void start_change_label(StartMenu *menu)
{
	// Hide the start menu
	start_hide(menu);

	// Request a new label
	if (super_request_args(GUI->window,
						   GetString(&locale, MSG_START_SELECT_LABEL),
						   SRF_BUFFER | SRF_IPC,
						   menu->data.label,
						   39,
						   menu->ipc,
						   GetString(&locale, MSG_OKAY),
						   GetString(&locale, MSG_CANCEL),
						   0))
	{
		// Set change flag
		menu->changed = 1;
	}

	// Show start menu (complete with new label)
	start_show(menu, GUI->screen_pointer);
}

// Change the picture
void start_change_picture(StartMenu *menu)
{
	// Hide the start menu
	start_hide(menu);

	// Request a new picture
	if (request_file(GUI->window,
					 GetString(&locale, MSG_START_SELECT_PICTURE),
					 menu->bank->backpic,
					 0,	 //"DOpus5:Images/",
					 1 << 31,
					 0))
	{
		// Set change flag
		menu->changed = 1;
	}

	// Show start menu (complete with new image)
	start_show(menu, GUI->screen_pointer);
}

// Create a new Start Menu
void start_create_new(BOOL load)
{
	IPCData *ipc;
	char path[256];

	// Clear path
	*path = 0;

	// Load a new one?
	if (load)
	{
		// Request start menu to load
		if (!(request_file(GUI->window, GetString(&locale, MSG_START_SELECT_MENU), path, "DOpus5:Buttons/", 0, 0)))
			return;
	}

	// Create a start menu
	if ((ipc = start_new(path, "Start!", 0, -1, -1)))
	{
		// Show new start menu
		if (load && GUI->window)
			IPC_Command(ipc, IPC_SHOW, 0, GUI->screen_pointer, 0, 0);
	}
}

// Add a new item to the Start Menu
void start_add_item(StartMenu *menu, char *pathname)
{
	PopUpItem *item;
	Cfg_Button *button;
	Cfg_ButtonFunction *func;
	Cfg_Instruction *ins;
	char *string, *label;
	BPTR lock;
	D_S(struct FileInfoBlock, fib)
	short type;

	// Valid bank?
	if (!menu->bank || !pathname || !*pathname)
		return;

	// Lock file
	if (!(lock = Lock(pathname, ACCESS_READ)))
	{
		DisplayBeep(menu->window->WScreen);
		return;
	}

	// Get label pointer
	if (!(label = FilePart(pathname)) || !*label)
		label = pathname;

	// Allocate buffer, button
	if (!(string = AllocVec(512, MEMF_CLEAR)) ||
		!(button = NewButtonWithFunc(menu->bank->memory, label, FTYPE_LEFT_BUTTON)))
	{
		DisplayBeep(menu->window->WScreen);
		FreeVec(string);
		UnLock(lock);
		return;
	}

	// Set title flag so item will appear in main menu
	button->button.flags = BUTNF_TITLE;

	// Examine the file
	Examine(lock, fib);

	// Is it a directory?
	if (fib->fib_DirEntryType > 0)
	{
		// Make this a scandir
		lsprintf(string, "ScanDir \"%s\" NEW", pathname);
		type = INST_COMMAND;
	}

	// It's a file
	else
	{
		BOOL quote = 0;

		// Unlock file
		UnLock(lock);

		// Try for lock on icon
		strcpy(string, pathname);
		if (!sufcmp(string, ".info"))
			strcat(string, ".info");

		// See if icon is present
		if ((lock = Lock(string, ACCESS_READ)))
		{
			// It'll be a Workbench function
			type = INST_WORKBENCH;
		}

		// No icon, so run as DOS function
		else
		{
			// DOS function
			type = INST_AMIGADOS;
		}

		// Does the path need quotes?
		if (strchr(pathname, ' '))
			quote = 1;

		// Build string
		if (quote)
		{
			string[0] = '\"';
			string[1] = 0;
		}
		else
			string[0] = 0;
		strcat(string, pathname);
		if (quote)
			strcat(string, "\"");
	}

	// Unlock the lock
	UnLock(lock);

	// Get function pointer
	func = (Cfg_ButtonFunction *)FindFunctionType((struct List *)&button->function_list, FTYPE_LEFT_BUTTON);

	// Make instruction
	if ((ins = NewInstruction(menu->bank->memory, type, string)))
	{
		// Add instruction to function
		AddTail((struct List *)&func->instructions, (struct Node *)ins);
	}

	// Add button to bank
	AddTail(&menu->bank->buttons, &button->node);

	// Increment button row count
	menu->bank->window.rows++;

	// Free menu
	ClearMemHandle(menu->memory);

	// Build new menu
	if ((item = PopUpNewItem(menu->menu, (IPTR)function_label(func), menu->last_id++, POPUPF_STRING)))
		item->data = func;

	// Set changed flag
	menu->changed = 1;

	// Free string
	FreeVec(string);
}

// Remap bank images
void start_remap_bank(StartMenu *menu, struct List *list, BOOL free)
{
	PopUpItem *item;

	// Go through menu
	for (item = (PopUpItem *)list->lh_Head; item->node.mln_Succ; item = (PopUpItem *)item->node.mln_Succ)
	{
		Cfg_ButtonFunction *func = 0;

		// Sub-menu?
		if (item->flags & POPUPF_SUB)
		{
			// Recurse into sub-menu
			start_remap_bank(menu, (struct List *)item->data, free);

			// Function present?
			if (item->flags & POPUPF_USERDATA)
				func = (Cfg_ButtonFunction *)item->userdata;
		}

		// Image to free?
		else if (free)
		{
			// Image?
			if (item->image)
			{
				// Free remap stuff
				FreeRemapImage(item->image, &menu->remap);

				// Free image
				CloseImage(item->image);

				// Clear image pointer
				item->image = 0;
				item->flags &= ~POPUPF_IMAGE;
			}
		}

		// Valid item; get function pointer
		else
			func = (Cfg_ButtonFunction *)item->data;

		// Got function pointer?
		if (func)
		{
			char *label = 0;

			// Does function have a label?
			if (func->function.flags2 & FUNCF2_LABEL_FUNC)
			{
				Cfg_Instruction *ins;

				// Find label instruction
				for (ins = (Cfg_Instruction *)func->instructions.mlh_Head; ins->node.mln_Succ;
					 ins = (Cfg_Instruction *)ins->node.mln_Succ)
				{
					// Label?
					if (ins->type == INST_LABEL)
					{
						// Get pointer to label string
						label = ins->string;
						break;
					}
				}
			}

			// Got image name?
			if (label)
			{
				// Try to load image
				if ((item->image = OpenImage(label, 0)))
				{
					// Remap the image
					RemapImage(item->image, menu->window->WScreen, &menu->remap);

					// Set image flag
					item->flags |= POPUPF_IMAGE;
				}
			}
		}
	}
}

// Change start menu font
void start_change_font(StartMenu *menu, short id)
{
	short size;
	char name[256];
	unsigned short font_pen_count;
	unsigned char font_pen_table[16];
	short a, pen, fpen;

	// Hide the start menu
	start_hide(menu);

	// Get name/size
	strcpy(name, (id == MENU_START_FONT) ? menu->bank->window.font_name : menu->data.label_fontname);
	size = (id == MENU_START_FONT) ? menu->bank->window.font_size : menu->data.label_fontsize;

	// Initialise font pens
	font_pen_count = 4;
	for (pen = 0; pen < 4; pen++)
		font_pen_table[pen] = pen;

	// Under 39 OS has 8 pens
	if (((struct Library *)GfxBase)->lib_Version >= 39)
	{
		font_pen_count += 4;
		for (; pen < 8; pen++)
			font_pen_table[pen] = 248 + pen;
	}

	// User pens
	font_pen_count += environment->env->palette_count;
	for (a = 0; a < environment->env->palette_count; a++, pen++)
		font_pen_table[pen] = GUI->pens[a];

	// Initial pen
	fpen = (id == MENU_START_FONT) ? menu->data.fpen : menu->data.label_fpen;
	if (fpen >= 4 && fpen < 252)
		fpen = GUI->pens[fpen - 4];

	// Request a new image
	if (request_font(GUI->window,
					 GetString(&locale, MSG_START_SELECT_FONT),
					 name,
					 &size,
					 FOF_DOFRONTPEN,
					 font_pen_count,
					 font_pen_table,
					 &fpen,
					 0))
	{
		// Store new font
		stccpy((id == MENU_START_FONT) ? menu->bank->window.font_name : menu->data.label_fontname, name, 30);
		if (id == MENU_START_FONT)
		{
			menu->bank->window.font_size = size;
			menu->data.fpen = fpen;
		}
		else
		{
			menu->data.label_fontsize = size;
			menu->data.label_fpen = fpen;
		}

		// Set change flag
		menu->changed = 1;
	}

	// Show start menu (complete with new font)
	start_show(menu, GUI->screen_pointer);
}
