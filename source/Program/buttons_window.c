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

// Open a button bank display
BOOL buttons_open(Buttons *buttons, struct Screen *screen, short flags)
{
	BOOL border = 1, simple = 1;
	struct Gadget *gadget = 0;
	short width_extra = 0, height_extra = 0;

	// Store path as last saved if we don't have one
	if (!buttons->last_saved[0] && buttons->bank)
		strcpy(buttons->last_saved, buttons->bank->path);

	// Make backup of bank for toolbar
	if (buttons->flags & BUTTONF_TOOLBAR && !buttons->backup)
		buttons->backup = CopyButtonBank(buttons->bank);

	// If button bank is already open, return
	if (!buttons || buttons->window)
		return 1;

	// Iconify initially?
	if (buttons->flags & BUTTONF_ICONIFIED)
	{
		if (buttons_iconify(buttons))
			return 1;
	}

	// Do we have to ask for a bank?
	if (buttons->flags & BUTTONF_LOAD)
	{
		// Clear load flag
		buttons->flags &= ~BUTTONF_LOAD;

		// Try to open a bank
		if (!(buttons_load(buttons, screen, 0)))
			return 0;

		// Coordinates not supplied?
		if (!(buttons->flags & BUTTONF_COORDS_SUPPLIED))
			buttons->pos = buttons->bank->window.pos;
	}

	// Get font we want
	buttons_get_font(buttons);

	// Is button bank empty?
	if (IsListEmpty(&buttons->bank->buttons))
	{
		Cfg_Button *button;

		// Create a new button
		if ((button = NewButton(buttons->bank->memory)))
		{
			// Add to bank
			AddTail(&buttons->bank->buttons, &button->node);
			buttons->bank->window.rows = 1;
			buttons->bank->window.columns = 1;
		}
	}

	// Clear borderless flag
	buttons->flags &= ~BUTTONF_BORDERLESS;

	// Check columns/rows for 0
	if (buttons->bank->window.columns == 0)
		buttons->bank->window.columns = 1;
	if (buttons->bank->window.rows == 0)
		buttons->bank->window.rows = 1;

	// Borderless or toolbar?
	if (buttons->bank->window.flags & BTNWF_NO_BORDER || buttons->flags & BUTTONF_TOOLBAR)
	{
		unsigned short max_width, max_height;

		// Get maximum button size
		buttons_get_max_size(buttons, &max_width, &max_height);

		// Set borderless flag if not a toolbar
		if (!(buttons->flags & BUTTONF_TOOLBAR))
		{
			border = 0;
			buttons->flags |= BUTTONF_BORDERLESS;
		}

		// Is a toolbar; check only 1 row
		else if (buttons->bank->window.rows > 1)
		{
			// Transmogrify into one row
			buttons->bank->window.columns *= buttons->bank->window.rows;
			buttons->bank->window.rows = 1;
		}

		// Fixed window size calculated from buttons
		buttons->pos.Width = max_width * buttons->bank->window.columns;
		buttons->pos.Height = max_height * buttons->bank->window.rows;
	}

	// Smart refresh?
	if (buttons->bank->window.flags & BTNWF_SMART_REFRESH && !(buttons->flags & BUTTONF_TOOLBAR))
		simple = 0;

	// Load background picture
	if (buttons->bank->backpic[0])
	{
		// Fill out WBPatternPrefs
		buttons->pattern_data.prefs.wbp_Flags = 0;
		buttons->pattern_data.prefs.wbp_Which = PATTERN_BUTTONS;
		buttons->pattern_data.prefs.wbp_DataLength = strlen(buttons->bank->backpic);
		buttons->pattern_data.data = buttons->bank->backpic;
		buttons->pattern_data.precision = 3;
		buttons->pattern_data.flags = PATF_TILE;
		GetPattern(&buttons->pattern_data, screen, 0);

// Initialise backfill hook
#if defined(__AROS__) || defined(__MORPHOS__)
		buttons->pattern.hook.h_Entry = (HOOKFUNC)HookEntry;
		buttons->pattern.hook.h_SubEntry = (HOOKFUNC)PatternBackfill;
#else
		buttons->pattern.hook.h_Entry = (ULONG(*)())PatternBackfill;
#endif
		buttons->pattern.hook.h_Data = 0;
		buttons->pattern.pattern = &buttons->pattern_data;
	}

	// Check window size
	if (buttons->pos.Height == -1)
	{
		if (buttons->font)
			buttons->pos.Height = buttons->font->tf_YSize + 2;
		else
			buttons->pos.Height = ((struct GfxBase *)GfxBase)->DefaultFont->tf_YSize + 2;
	}

	// Open under mouse?
	if (buttons->flags & BUTTONF_UNDERMOUSE)
	{
		// Get position from mouse coordinates
		buttons->pos.Left = screen->MouseX - (buttons->pos.Width >> 1);
		buttons->pos.Top = screen->MouseY - (buttons->pos.Height >> 1);
		buttons->flags &= ~BUTTONF_UNDERMOUSE;
	}

	// No position?
	else if (buttons->pos.Left == -1)
	{
		// Place in top-left corner of screen
		buttons->pos.Left = screen->Width / 10;
		buttons->pos.Top = screen->Height / 10;
	}

	// Common drag-gadget initialisers
	buttons->drag_gadget.NextGadget = 0;
	buttons->drag_gadget.LeftEdge = 0;
	buttons->drag_gadget.GadgetType = GTYP_WDRAGGING;
	buttons->drag_gadget.Activation = GACT_IMMEDIATE | GACT_RELVERIFY;
	buttons->drag_gadget.GadgetID = DRAG_GADGET;

	// Borderless?
	if (!border)
	{
		// Fill out dragging gadget
		buttons->drag_gadget.TopEdge = 0;
		buttons->drag_gadget.Flags = GFLG_GADGHNONE | GFLG_GADGIMAGE;

		// Get image
		if ((buttons->drag_gadget.GadgetRender =
				 NewObject(0, "dopusiclass", DIA_Type, IM_BBOX, IA_Width, 0, IA_Height, 0, TAG_END)))
			DoMethod((Object *)buttons->drag_gadget.GadgetRender, OM_ADDTAIL, &buttons->boopsi_list);

		// Fix position
		buttons_fix_drag(buttons);

		// Get extra height for horizontal drag
		if (buttons->drag_gadget.Flags & GFLG_RELWIDTH)
			height_extra = buttons->drag_gadget.Height;

		// Get extra width for vertical drag
		else if (buttons->drag_gadget.Flags & GFLG_RELHEIGHT)
			width_extra = buttons->drag_gadget.Width;
	}

	// Otherwise, create iconify gadget
	else if (!(buttons->flags & BUTTONF_TOOLBAR))
		gadget = create_iconify_gadget(screen, &buttons->boopsi_list, FALSE);

	// Open button window
	if (!(buttons->window = OpenWindowTags(
			  0,
			  WA_Left,
			  buttons->pos.Left,
			  WA_Top,
			  buttons->pos.Top,
			  WA_InnerWidth,
			  buttons->pos.Width + width_extra,
			  WA_InnerHeight,
			  buttons->pos.Height + height_extra,
			  WA_MinWidth,
			  10,
			  WA_MinHeight,
			  10,
			  WA_MaxWidth,
			  (ULONG)~0,
			  WA_MaxHeight,
			  (ULONG)~0,
			  WA_IDCMP,
			  IDCMP_ACTIVEWINDOW | IDCMP_CHANGEWINDOW | IDCMP_CLOSEWINDOW | IDCMP_GADGETDOWN | IDCMP_GADGETUP |
				  IDCMP_IDCMPUPDATE | IDCMP_INACTIVEWINDOW | IDCMP_INTUITICKS | IDCMP_MENUHELP | IDCMP_MENUPICK |
				  IDCMP_MENUVERIFY | IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_RAWKEY | IDCMP_REFRESHWINDOW,
			  WA_Borderless,
			  !border,
			  WA_CloseGadget,
			  border,
			  WA_DepthGadget,
			  border,
			  WA_DragBar,
			  border,
			  WA_SizeGadget,
			  border,
			  (border) ? WA_Title : TAG_IGNORE,
			  buttons->bank->window.name,
			  WA_SizeBRight,
			  TRUE,
			  WA_SizeBBottom,
			  TRUE,
			  WA_NewLookMenus,
			  TRUE,
			  WA_MenuHelp,
			  TRUE,
			  WA_ReportMouse,
			  TRUE,
			  (simple) ? WA_SimpleRefresh : TAG_IGNORE,
			  TRUE,
			  WA_CustomScreen,
			  screen,
			  WA_ScreenTitle,
			  GUI->screen_title,
			  WA_WindowName,
			  DOPUS_WIN_NAME,
			  WA_Gadgets,
			  gadget,
			  (buttons->pattern_data.valid) ? WA_BackFill : TAG_IGNORE,
			  &buttons->pattern,
			  TAG_END)))
		return 0;

	// Show busy pointer
	SetBusyPointer(buttons->window);

	// Save width
	buttons->pos.Width = buttons->window->GZZWidth;
	buttons->pos.Height = buttons->window->GZZHeight;

	// Fix title gadgets
	if (border)
		FixTitleGadgets(buttons->window);

	// Initialise ID
	SetWindowID(buttons->window, &buttons->id, WINDOW_BUTTONS, (struct MsgPort *)buttons->ipc);

	// Get drawinfo
	buttons->drawinfo = GetScreenDrawInfo(buttons->window->WScreen);

	// Add AppWindow
	buttons->appwindow = AddAppWindowA(WINDOW_BUTTONS, 0, buttons->window, buttons->app_port, 0);

	// Toolbar?
	if (buttons->flags & BUTTONF_TOOLBAR)
	{
		struct Menu *menu = 0;
		APTR vi;

		// Create menu strip and layout menus
		if ((vi = GetVisualInfoA(buttons->window->WScreen, 0)) &&
			(menu = BuildMenuStrip(button_toolbar_menu, &locale)) &&
			LayoutMenus(menu, vi, GTMN_NewLookMenus, TRUE, TAG_END))
		{
			// Add to window
			SetMenuStrip(buttons->window, menu);
		}

		// Failed
		else
			FreeMenus(menu);

		// Free VisualInfo
		FreeVisualInfo(vi);
	}

	// Use normal menus
	else
	{
		// Set menus
		display_get_menu(buttons->window);
		display_fix_menu(buttons->window, WINDOW_BUTTONS, 0);
	}

	// Get internal dimensions
	buttons->internal.Left = buttons->window->BorderLeft;
	buttons->internal.Top = buttons->window->BorderTop;
	buttons->internal.Width = buttons->window->Width - buttons->window->BorderLeft - buttons->window->BorderRight;
	buttons->internal.Height = buttons->window->Height - buttons->window->BorderTop - buttons->window->BorderBottom;

	// Window with borders?
	if (border)
	{
		short scroll;
		struct Gadget *gadget;

		// Get scroll flags
		scroll = SCROLL_HORIZ;
		if (!(buttons->flags & BUTTONF_TOOLBAR))
			scroll |= SCROLL_VERT;

		// Add scroll bars
		if (!(AddScrollBars(buttons->window, &buttons->boopsi_list, GUI->draw_info, scroll)))
		{
			buttons_close(buttons, 0);
			return 0;
		}

		// Get scroller pointers
		gadget = buttons->horiz_scroll = FindBOOPSIGadget(&buttons->boopsi_list, GAD_HORIZ_SCROLLER);
		if ((buttons->vert_scroll = FindBOOPSIGadget(&buttons->boopsi_list, GAD_VERT_SCROLLER)))
			gadget = buttons->vert_scroll;

		// Add gadgets
		AddGList(buttons->window, gadget, -1, -1, 0);
		RefreshGList(gadget, buttons->window, 0, -1);

		// Fill out dragging gadget
		buttons->drag_gadget.TopEdge = buttons->window->BorderTop;
		buttons->drag_gadget.Width = buttons->window->BorderLeft;
		buttons->drag_gadget.Height = 0;
		buttons->drag_gadget.Flags = GFLG_GADGHNONE | GFLG_RELHEIGHT;
		buttons->drag_gadget.GadgetRender = 0;
	}

	// No borders
	else
	{
		// Clear scroller pointers
		buttons->vert_scroll = 0;
		buttons->horiz_scroll = 0;

		// Fix drag gadget size
		buttons_fix_drag(buttons);

		// Fix internal dimensions
		buttons_fix_internal(buttons);
	}

	// Add dragging gadget
	AddGList(buttons->window, &buttons->drag_gadget, -1, 1, 0);

	// Set font
	if (buttons->font)
		SetFont(buttons->window->RPort, buttons->font);

	// Store position
	buttons->last_position = *((struct IBox *)&buttons->window->LeftEdge);

	// Remap buttons
	if (flags & BUTOPENF_REMAP)
		buttons_remap(buttons, 1);

	// Setup buttons display
	SetDrMd(buttons->window->RPort, JAM1);
	buttons_refresh(buttons, BUTREFRESH_RESIZE | BUTREFRESH_REFRESH);

	// First time we've opened?
	if (buttons->flags & BUTTONF_FIRST_TIME)
	{
		// Run script
		if (!(buttons->flags & BUTTONF_TOOLBAR))
			RunScript(SCRIPT_OPEN_BUTTONS, buttons->bank->window.name);
		buttons->flags &= ~BUTTONF_FIRST_TIME;
	}

	// Clear busy pointer
	ClearPointer(buttons->window);

	return 1;
}

// Close a button bank
void buttons_close(Buttons *buttons, short flags)
{
	// If editor is open, tell it to edit something else
	if (buttons->editor)
	{
		IPC_Command(buttons->editor, BUTTONEDIT_PISS_OFF, 0, buttons->bank, 0, 0);
		buttons->editor = 0;
	}

	// Close window
	if (buttons->window)
	{
		// Run script
		if (flags & BUTCLOSEF_SCRIPT && !(buttons->flags & BUTTONF_TOOLBAR))
			RunScript(SCRIPT_CLOSE_BUTTONS, buttons->bank->window.name);

		// Remember window coordinates
		buttons->pos.Left = buttons->window->LeftEdge;
		buttons->pos.Top = buttons->window->TopEdge;
		buttons->pos.Width = buttons->window->GZZWidth;
		buttons->pos.Height = buttons->window->GZZHeight;

		// Make sure window isn't busy
		buttons_unbusy(buttons);

		// Remove AppWindow
		RemoveAppWindow(buttons->appwindow);
		buttons->appwindow = 0;

		// Free DrawInfo
		FreeScreenDrawInfo(buttons->window->WScreen, buttons->drawinfo);

		// Free menus
		display_free_menu(buttons->window);

		// Can free remap?
		if (!(flags & BUTCLOSEF_NO_REMAP))
		{
			// Free remap buttons
			buttons_remap(buttons, 0);

			// Free pens
			FreeImageRemap(&buttons->remap);
		}

		// Free pattern
		FreePattern(&buttons->pattern_data);

		// Close window
		CloseWindow(buttons->window);
		buttons->window = 0;

		// Clear drag gadget image pointer
		buttons->drag_gadget.GadgetRender = 0;
	}

	// Free font
	if (buttons->font)
	{
		CloseFont(buttons->font);
		buttons->font = 0;
	}

	// Free BOOPSI objects
	BOOPSIFree(&buttons->boopsi_list);

	// Clear resized and edit-request flag
	buttons->flags &= ~(BUTTONF_RESIZED | BUTTONF_EDIT_REQUEST);
}

// Iconify a button bank
BOOL buttons_iconify(Buttons *buttons)
{
	struct TagItem tags[4];
	struct DiskObject *icon;

	// Already iconified?
	if (buttons->appicon)
		return 0;

	// Check there's an icon to use
	if (!(icon = get_iconify_icon(TRUE)))
		return 0;

	// Have we got a distinct position?
	if (buttons->icon_pos_x != NO_ICON_POSITION)
	{
		// Try to copy icon
		if ((buttons->appicon_icon = CopyDiskObject(icon, DOCF_NOIMAGE)))
		{
			// Use the copy
			icon = buttons->appicon_icon;

			// Set position
			icon->do_CurrentX = buttons->icon_pos_x - WBICONMAGIC_X;
			icon->do_CurrentY = buttons->icon_pos_y - WBICONMAGIC_Y;
		}
	}

	// Tags
	tags[0].ti_Tag = DAE_Local;
	tags[0].ti_Data = 1;
	tags[1].ti_Tag = DAE_Menu;
	tags[1].ti_Data = (IPTR)GetString(&locale, MSG_CLOSE);
	tags[2].ti_Tag = DAE_Special;
	tags[2].ti_Data = 1;
	tags[3].ti_Tag = TAG_END;

	// Try to add AppIcon
	if (!(buttons->appicon =
			  AddAppIconA(WINDOW_BUTTONS, 1, buttons->bank->window.name, buttons->app_port, 0, icon, tags)))
	{
		// Failed; free icon if we had one
		if (buttons->appicon_icon)
		{
			FreeDiskObjectCopy(buttons->appicon_icon);
			buttons->appicon_icon = 0;
		}
	}

	buttons->flags |= BUTTONF_ICONIFIED;
	return 1;
}

// Fix drag gadget size and position
BOOL buttons_fix_drag(Buttons *buttons)
{
	short horiz = 0, ret = 0;
	struct Image *image;

	// No drag bar?
	if (buttons->bank->window.flags & BTNWF_NONE)
		horiz = -1;

	// Horizontal drag bar? (does 'automatic' too if neither flags are set)
	else if (buttons->bank->window.flags & BTNWF_HORIZ ||
			 (!(buttons->bank->window.flags & BTNWF_VERT) && buttons->pos.Width < buttons->pos.Height))
		horiz = 1;

	// Set gadget size and position
	buttons->drag_gadget.LeftEdge = 0;
	buttons->drag_gadget.TopEdge = 0;
	buttons->drag_gadget.Width = (horiz) ? 0 : DRAG_WIDTH;
	buttons->drag_gadget.Height =
		(horiz == 1) ? ((GUI->screen_info & SCRI_LORES) ? DRAG_LO_HEIGHT : DRAG_HI_HEIGHT) : 0;
	if (buttons->bank->window.flags & BTNWF_RIGHT_BELOW)
	{
		if (horiz)
			buttons->drag_gadget.TopEdge = buttons->window->Height - buttons->drag_gadget.Height;
		else
			buttons->drag_gadget.LeftEdge = buttons->window->Width - buttons->drag_gadget.Width;
	}

	// Fix flags
	buttons->drag_gadget.Flags &= ~(GFLG_RELWIDTH | GFLG_RELHEIGHT);

	// Set relative flags
	if (horiz > -1)
		buttons->drag_gadget.Flags |= (horiz) ? GFLG_RELWIDTH : GFLG_RELHEIGHT;

	// Got image and window?
	if (buttons->window && (image = (struct Image *)buttons->drag_gadget.GadgetRender))
	{
		// Fix image size
		image->Width = (horiz == -1) ? 0 : (horiz) ? buttons->window->Width : DRAG_WIDTH;

		image->Height = (horiz == -1) ? 0
									  : (horiz) ? ((GUI->screen_info & SCRI_LORES) ? DRAG_LO_HEIGHT : DRAG_HI_HEIGHT)
												: buttons->window->Height;
	}

	// Save border type
	if (buttons->border_type !=
		(buttons->bank->window.flags & (BTNWF_HORIZ | BTNWF_VERT | BTNWF_NONE | BTNWF_RIGHT_BELOW)))
		ret = 1;
	buttons->border_type = buttons->bank->window.flags & (BTNWF_HORIZ | BTNWF_VERT | BTNWF_NONE | BTNWF_RIGHT_BELOW);
	return ret;
}

// Get internal dimensions
void buttons_fix_internal(Buttons *buttons)
{
	// No border?
	if (buttons->window->Flags & WFLG_BORDERLESS)
	{
		// Use absolute size of window
		buttons->internal.Left = 0;
		buttons->internal.Top = 0;
		buttons->internal.Width = buttons->window->Width;
		buttons->internal.Height = buttons->window->Height;

		// Horizontal drag gadget?
		if (buttons->drag_gadget.Flags & GFLG_RELWIDTH)
		{
			if (!(buttons->bank->window.flags & BTNWF_RIGHT_BELOW))
				buttons->internal.Top += buttons->drag_gadget.Height;
			buttons->internal.Height -= buttons->drag_gadget.Height;
		}

		// Vertical
		else if (buttons->drag_gadget.Flags & GFLG_RELHEIGHT)
		{
			if (!(buttons->bank->window.flags & BTNWF_RIGHT_BELOW))
				buttons->internal.Left += buttons->drag_gadget.Width;
			buttons->internal.Width -= buttons->drag_gadget.Width;
		}
	}

	// Normal borders
	else
	{
		// Get dimensions from window
		buttons->internal.Left = buttons->window->BorderLeft;
		buttons->internal.Top = buttons->window->BorderTop;
		buttons->internal.Width = buttons->window->Width - buttons->window->BorderLeft - buttons->window->BorderRight;
		buttons->internal.Height = buttons->window->Height - buttons->window->BorderTop - buttons->window->BorderBottom;
	}
}

// Remap buttons
void buttons_remap(Buttons *buttons, short remap)
{
	Cfg_Button *button;
	Cfg_ButtonFunction *func;

	// Go through buttons
	for (button = (Cfg_Button *)buttons->bank->buttons.lh_Head; button->node.ln_Succ;
		 button = (Cfg_Button *)button->node.ln_Succ)
	{
		// Go through functions
		for (func = (Cfg_ButtonFunction *)button->function_list.mlh_Head; func->node.ln_Succ;
			 func = (Cfg_ButtonFunction *)func->node.ln_Succ)
		{
			// Got an image?
			if (func->image)
			{
				// Remap if asked to
				if (remap)
					RemapImage(func->image, GUI->screen_pointer, &buttons->remap);

				// Else free current remapping
				else
					FreeRemapImage(func->image, &buttons->remap);
			}
		}
	}
}
