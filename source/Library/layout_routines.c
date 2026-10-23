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
#include "layout_routines.h"
#include "string_hook.h"
#include <Program/pattern.h>

/****************************************************************************
						 Open a configuration window
 ****************************************************************************/

struct Window *LIBFUNC L_OpenConfigWindow(REG(a0, NewConfigWindow *newwindow), REG(a6, struct MyLibrary *libbase))
{
	struct IBox dims;
	struct Window *parent_window = 0, *window;
	struct TextFont *font;
	struct Screen *screen;
	WindowData *data;
	struct LibData *ldata;
	ULONG flags;
	APTR memory;
	struct Gadget *gadget = 0;

#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Get flags
	flags = newwindow->flags;

	// If no parent supplied window, get default public screen
	if (!newwindow->parent)
	{
		if (!(screen = LockPubScreen(0)))
			return 0;
	}
	else
	{
		// See if supplied parent is a screen
		if (flags & WINDOW_SCREEN_PARENT)
			screen = newwindow->parent;

		// It's a window
		else
		{
			parent_window = newwindow->parent;
			screen = parent_window->WScreen;
		}
	}

	// Get font to use
	font = (newwindow->font) ? newwindow->font : screen->RastPort.Font;

	// Get lib data
	ldata = (struct LibData *)libbase->ml_UserData;

	// No stipple?
	if (ldata->flags & LIBDF_NOSTIPPLE)
		flags &= ~WINDOW_REQ_FILL;

	// Calculate window dimensions
	if (!(L_CalcWindowDims(screen, newwindow->dims, &dims, font, flags)))
	{
		// If window was too large, try again with system default text
		font = ((struct GfxBase *)GfxBase)->DefaultFont;
		if (!(L_CalcWindowDims(screen, newwindow->dims, &dims, font, flags)))
		{
			// If window was too large, try again with topaz
			font = topaz_font;

			// Try again
			if (!(L_CalcWindowDims(screen, newwindow->dims, &dims, font, flags)))
			{
				// Unlock public screen if it was locked
				if (!newwindow->parent)
					UnlockPubScreen(0, screen);

				// Failed
				return 0;
			}

			flags |= OPEN_USED_TOPAZ;
		}
		else
			flags |= OPEN_USED_DEFAULT;
	}

	// If parent window, position relative to parent
	if (parent_window)
	{
		// If right-justify is set, position at top/right of parent
		if (newwindow->dims->char_dim.Left == POS_RIGHT_JUSTIFY)
		{
			dims.Left = parent_window->LeftEdge + parent_window->Width;
			dims.Top = parent_window->TopEdge;
		}

		// Otherwise do we need to center in parent?
		else if (newwindow->dims->char_dim.Left == POS_CENTER)
		{
			dims.Left = parent_window->LeftEdge + ((parent_window->Width - dims.Width) >> 1);
			dims.Top = parent_window->TopEdge + ((parent_window->Height - dims.Height) >> 1);
		}

		// Center over pointer?
		else if (newwindow->dims->char_dim.Left == POS_MOUSE_CENTER)
		{
			dims.Left = parent_window->WScreen->MouseX - (dims.Width >> 1);
			dims.Top = parent_window->WScreen->MouseY - (dims.Height >> 1);
		}

		// Pointer relative
		else if (newwindow->dims->char_dim.Left == POS_MOUSE_REL)
		{
			dims.Left = parent_window->WScreen->MouseX;
			dims.Top = parent_window->WScreen->MouseY;
		}
	}

	// Allocate memory handle and data structure
	if (!(memory = L_NewMemHandle(2048, 1024, MEMF_CLEAR)) || !(data = L_AllocMemH(memory, sizeof(WindowData))))
	{
		L_FreeMemHandle(memory);
		if (!newwindow->parent)
			UnlockPubScreen(0, screen);
		DisplayBeep(screen);
		return 0;
	}

	// Store memory pointer
	data->memory = memory;

	// Get draw info
	data->drawinfo = GetScreenDrawInfo(screen);

	// Initialise lists
	NewList(&data->boopsi_list);
	NewList((struct List *)&data->object_list);

	// Want iconify gadget?
	if (flags & WINDOW_ICONIFY && screen)
	{
		// Create iconify gadget
		gadget = L_CreateTitleGadget(screen,
									 &data->boopsi_list,
									 !(flags & (WINDOW_SIZE_RIGHT | WINDOW_SIZE_BOTTOM)),
									 0,
									 IM_ICONIFY,
									 GAD_ID_ICONIFY,
									 libbase);
	}

	// Get backfill hook
	if ((data->backfill = L_LockReqBackFill(screen, libbase)))
	{
		// Don't need to stipple?
		if (((PatternInstance *)data->backfill)->pattern && ((PatternInstance *)data->backfill)->pattern->valid)
		{
			// We have a backfill so turn off stippling
			flags &= ~WINDOW_REQ_FILL;
		}
	}

	// Open requester window
	if (!(window = OpenWindowTags(0,
								  WA_Left,
								  dims.Left,
								  WA_Top,
								  dims.Top,
								  WA_Width,
								  dims.Width,
								  WA_Height,
								  dims.Height,
								  WA_MinWidth,
								  dims.Width,
								  WA_MinHeight,
								  dims.Height,
								  WA_MaxWidth,
								  ~0,
								  WA_MaxHeight,
								  ~0,
								  WA_Flags,
								  ((flags & WINDOW_NO_ACTIVATE) ? 0 : WFLG_ACTIVATE) |
									  ((flags & WINDOW_NO_CLOSE) ? 0 : WFLG_CLOSEGADGET) |
									  ((flags & WINDOW_NO_BORDER) ? 0 : (WFLG_DEPTHGADGET | WFLG_DRAGBAR)) |
									  WFLG_NEWLOOKMENUS | WFLG_RMBTRAP,
								  (flags & WINDOW_SIMPLE) ? WA_SimpleRefresh : WA_SmartRefresh,
								  TRUE,
								  (flags & WINDOW_VISITOR) ? WA_PubScreen : WA_CustomScreen,
								  (IPTR)screen,
								  (flags & WINDOW_VISITOR) ? WA_PubScreenFallBack : TAG_IGNORE,
								  TRUE,
								  (flags & (WINDOW_SIZE_RIGHT | WINDOW_SIZE_BOTTOM)) ? WA_SizeGadget : TAG_IGNORE,
								  TRUE,
								  WA_SizeBRight,
								  flags & WINDOW_SIZE_RIGHT,
								  WA_SizeBBottom,
								  flags & WINDOW_SIZE_BOTTOM,
								  WA_Title,
								  newwindow->title,
								  WA_WindowName,
								  DOPUS_WIN_NAME,
								  WA_AutoAdjust,
								  TRUE,
								  (gadget) ? WA_Gadgets : TAG_IGNORE,
								  gadget,
								  (data->backfill) ? WA_BackFill : TAG_IGNORE,
								  data->backfill,
								  TAG_DONE)))
	{
		// Release backfill hook
		if (data->backfill)
			L_UnlockReqBackFill(libbase);

		// Free draw info
		FreeScreenDrawInfo(screen, data->drawinfo);
		DisplayBeep(screen);

		// Unlock public screen if it was locked
		if (!newwindow->parent)
			UnlockPubScreen(0, screen);

		// Free boopsi objects
		L_BOOPSIFree(&data->boopsi_list);

		// Free memory
		L_FreeMemHandle(data->memory);
		return 0;
	}

	// Had an iconify gadget?
	if (gadget)
	{
		// Fix title gadgets
		L_FixTitleGadgets(window);
	}

	// Store data and window pointer
	window->UserData = (APTR)data;

	// Initialise ID
	L_SetWindowID(window, 0, WINDOW_UNDEFINED, 0);

	// Back-fill?
#ifndef BACKFILL
	if (flags & WINDOW_REQ_FILL)
	{
		_req_backfill(window);
	}
#endif

	// Is window the full size we requested?
	if (window->Width < dims.Width)
		flags |= OPEN_SHRUNK_HORIZ;
	if (window->Height < dims.Height)
		flags |= OPEN_SHRUNK_VERT;

	// Set window port
	window->UserPort = newwindow->port;

	// Set IDCMP flags
	ModifyIDCMP(window,
				IDCMP_CLOSEWINDOW | IDCMP_GADGETDOWN | IDCMP_GADGETUP | IDCMP_IDCMPUPDATE | IDCMP_INACTIVEWINDOW |
					IDCMP_INTUITICKS | IDCMP_MENUPICK | IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_NEWSIZE |
					IDCMP_REFRESHWINDOW | IDCMP_RAWKEY | IDCMP_SIZEVERIFY |
					((flags & WINDOW_AUTO_KEYS) ? IDCMP_VANILLAKEY : 0));

	// Set the window's font and draw mode
	SetFont(window->RPort, font);
	SetDrMd(window->RPort, JAM1);

	// Allocate window filerequester
	data->request = AllocAslRequest(ASL_FileRequest, 0);

	// Get visual info
	data->visinfo = GetVisualInfoA(window->WScreen, 0);

	// Store locale info
	data->locale = newwindow->locale;

	// Store window port
	data->window_port = newwindow->port;

	// Flags
	data->flags = flags;

	// Unlock public screen if it was locked
	if (!newwindow->parent)
		UnlockPubScreen(0, screen);

	// Return window
	return window;
}

/****************************************************************************
						 Close a configuration window
 ****************************************************************************/

void LIBFUNC L_CloseConfigWindow(REG(a0, struct Window *window), REG(a6, struct MyLibrary *libbase))
{
#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Check valid window
	if (window)
	{
		WindowData *wdata = (WindowData *)window->UserData;
		struct Screen *screen;

		// Save screen pointer
		screen = window->WScreen;

		// Free window menus
		L_FreeWindowMenus(window);

		// Check busy requester
		if (wdata->busy_req)
			EndRequest(wdata->busy_req, window);

		// Free object lists
		while (!(IsListEmpty((struct List *)&wdata->object_list)))
			L_FreeObjectList((ObjectList *)wdata->object_list.mlh_Head);

		// If window has its own message port, close it normally
		if (!wdata->window_port)
			CloseWindow(window);

		// Otherwise close it safely
		else
			L_CloseWindowSafely(window);

		// Release backfill hook
		if (wdata->backfill)
			L_UnlockReqBackFill(libbase);

		// Free draw info
		FreeScreenDrawInfo(screen, wdata->drawinfo);

		// Free visual info
		FreeVisualInfo(wdata->visinfo);

		// Free requesters
		if (wdata->request)
			FreeAslRequest(wdata->request);
		if (wdata->font_request)
			FreeAslRequest(wdata->font_request);

		// Free tags
		FreeTagItems(wdata->user_tags);

		// Free boopsi objects
		L_BOOPSIFree(&wdata->boopsi_list);

		// Free memory
		L_FreeMemHandle(wdata->memory);
	}
}

/****************************************************************************
				   Calculate an object's position and size
 ****************************************************************************/

int LIBFUNC L_CalcObjectDims(REG(a0, void *parent),
							 REG(a1, struct TextFont *use_font),
							 REG(a2, struct IBox *pos),
							 REG(a3, struct IBox *dest_pos),
							 REG(a4, GL_Object *last_ob),
							 REG(d0, ULONG flags),
							 REG(d1, GL_Object *this_ob),
							 REG(d2, GL_Object *parent_ob))
{
	struct TextFont *font;
	struct IBox inner;
	ULONG val, div;
	int success = 1;
	struct Window *window = 0;
	struct Screen *screen = 0;
	long sizex = 0, sizey = 0;
	BOOL square_w = 0, square_h = 0;

	if (flags & WINDOW_SCREEN_PARENT)
		screen = parent;
	else
		window = parent;

	/* Font to use for calculations */
	if (use_font)
		font = use_font;
	else if (window)
		font = window->RPort->Font;
	else
		font = screen->RastPort.Font;

	// Clear destination size
	dest_pos->Left = 0;
	dest_pos->Top = 0;
	dest_pos->Width = 0;
	dest_pos->Height = 0;

	/* Calculate window's inner dimensions */
	if (parent_ob && (flags & WINDOW_OBJECT_PARENT))
	{
		// Title box?
		if (parent_ob->type == OD_AREA && parent_ob->flags & AREAFLAG_TITLE)
		{
			inner.Left = parent_ob->dims.Left + 2;
			inner.Top = parent_ob->dims.Top + 1 + (font->tf_YSize >> 1);
			inner.Width = parent_ob->dims.Width - 4;
			inner.Height = parent_ob->dims.Height - 2 - (font->tf_YSize >> 1);
		}
		else
		{
			inner.Left = parent_ob->dims.Left + 2;
			inner.Top = parent_ob->dims.Top + 1;
			inner.Width = parent_ob->dims.Width - 4;
			inner.Height = parent_ob->dims.Height - 2;
		}
	}
	else if (window)
	{
		inner.Left = window->BorderLeft;
		inner.Top = window->BorderTop;
		inner.Width = window->Width - window->BorderLeft - window->BorderRight;
		inner.Height = window->Height - window->BorderTop - window->BorderBottom;
	}
	else
	{
		struct Rectangle rect;

		// Get overscan size
		QueryOverscan(GetVPModeID(&screen->ViewPort), &rect, OSCAN_TEXT);

		// Position window relative to display clip
		if (screen->LeftEdge < 0)
			inner.Left = -screen->LeftEdge;
		else
			inner.Left = 0;
		if (screen->TopEdge < 0)
			inner.Top = -screen->TopEdge;
		else
			inner.Top = 0;
		inner.Width = rect.MaxX - rect.MinX + 1;
		if (inner.Width > screen->Width)
			inner.Width = screen->Width;
		inner.Height = rect.MaxY - rect.MinY + 1;
		if (inner.Height > screen->Height)
			inner.Height = screen->Height;
	}

	/****** Calculate left position *******/

	// Relative to last object?
	if (pos[0].Left & POS_REL_RIGHT)
	{
		short left = pos[0].Left & ~POS_REL_RIGHT;

		// Calculate relative pos
		left *= font->tf_XSize;
		left += pos[1].Left;

		// Got a last object?
		if (last_ob)
		{
			// Add left from last object
			left += last_ob->dims.Left + last_ob->dims.Width;
		}

		// Store left
		dest_pos->Left = left;
	}

	// Proportion of window space
	else if (pos[0].Left >= POS_PROPORTION)
	{
		div = (pos[0].Left - POS_PROPORTION) * (inner.Width - pos[1].Left);
		val = UDivMod32(div, 100);
		dest_pos->Left = inner.Left + val + pos[1].Left;
	}

	// Normal positioning
	else if (pos[0].Left >= 0)
	{
		dest_pos->Left = inner.Left + (pos[0].Left * font->tf_XSize) + pos[1].Left;
	}

	// Adjust for text size?
	if (this_ob && this_ob->flags & TEXTFLAG_ADJUST_TEXT && this_ob->text)
	{
		struct RastPort rp;
		short len;

		// Dummy rastport
		InitRastPort(&rp);
		SetFont(&rp, font);

		// Get length
		len = TextLength(&rp, this_ob->text, strlen(this_ob->text));

		// To the left?
		if (this_ob->flags & PLACETEXT_LEFT)
			dest_pos->Left += len + 8;
		else
			dest_pos->Left -= len + 8;
	}

	/************ Calculate top position ************/

	// Relative to last object?
	if (pos[0].Top & POS_REL_RIGHT)
	{
		short top = pos[0].Top & ~POS_REL_RIGHT;

		// Calculate relative pos
		top *= font->tf_YSize;
		top += pos[1].Top;

		// Got a last object?
		if (last_ob)
		{
			// Add top from last object
			top += last_ob->dims.Top + last_ob->dims.Height;
		}

		// Store top
		dest_pos->Top = top;
	}

	// Proportion of space across window
	else if (pos[0].Top >= POS_PROPORTION)
	{
		div = (pos[0].Top - POS_PROPORTION) * inner.Height;
		val = UDivMod32(div, 100);
		dest_pos->Top = inner.Top + val + pos[1].Top;
	}

	// Normal positioning
	else if (pos[0].Top >= 0)
	{
		dest_pos->Top = inner.Top + (pos[0].Top * font->tf_YSize) + pos[1].Top;
	}

	// Adjust from parent object?
	if (parent_ob && this_ob && !(flags & WINDOW_OBJECT_PARENT))
	{
		// Increment position
		if (this_ob->flags & POSFLAG_ADJUST_POS_X)
			dest_pos->Left += parent_ob->dims.Left + parent_ob->dims.Width - inner.Left;
		else if (this_ob->flags & POSFLAG_ALIGN_POS_X)
			dest_pos->Left += parent_ob->dims.Left - inner.Left;

		if (this_ob->flags & POSFLAG_ADJUST_POS_Y)
			dest_pos->Top += parent_ob->dims.Top + parent_ob->dims.Height - inner.Top;
		else if (this_ob->flags & POSFLAG_ALIGN_POS_Y)
			dest_pos->Top += parent_ob->dims.Top - inner.Top;
	}

	/* Calculate width */
	if (pos[0].Width <= SIZE_MAXIMUM)
	{
		/* SIZE_MAXIMUM means from here to the maximum */
		dest_pos->Width = (inner.Width - (dest_pos->Left - inner.Left)) + pos[1].Width;

		/* SIZE_MAX_LESS means from here to the maximum less some chars */
		if (pos[0].Width < SIZE_MAX_LESS)
		{
			div = pos[0].Width - SIZE_MAX_LESS;
			dest_pos->Width += (font->tf_XSize * div);
		}
	}
	else
	{
		// Square?
		if (pos[0].Width == POS_SQUARE)
			square_w = 1;

		/* a proportion of space */
		else if (pos[0].Width >= POS_PROPORTION)
		{
			div = (pos[0].Width - POS_PROPORTION) * (inner.Width - pos[1].Left);
			val = UDivMod32(div, 100);
			dest_pos->Width = val + pos[1].Width;
		}

		/* Normal operation */
		else
		{
			dest_pos->Width = (pos[0].Width * font->tf_XSize) + pos[1].Width;
		}
	}

	/* Is this gadget to be left-positioned specially ? */
	if (pos[0].Left == POS_CENTER)
	{
		/* Centered */
		dest_pos->Left = ((inner.Width - dest_pos->Width) >> 1) + inner.Left;

		// Offset by character base?
		if (pos[1].Left > FPOS_TEXT_OFFSET)
			dest_pos->Left -= (pos[1].Left - FPOS_TEXT_OFFSET) * font->tf_XSize;

		// Fine positioning
		else
			dest_pos->Left += pos[1].Left;
	}
	else if (pos[0].Left <= POS_RIGHT_JUSTIFY)
	{
		/* Right-justified */
		dest_pos->Left = ((inner.Left + inner.Width) - dest_pos->Width) + pos[1].Left;

		/* Adjust by any character factor */
		dest_pos->Left += (pos[0].Left - POS_RIGHT_JUSTIFY) * font->tf_XSize;
	}

	/* Calculate height */

	if (pos[0].Height < 0)
	{
		/* SIZE_MAXIMUM means from here to the maximum */
		dest_pos->Height = (inner.Height - (dest_pos->Top - inner.Top)) + pos[1].Height;

		/* SIZE_MAX_LESS means from here to the maximum less some chars */
		if (pos[0].Height < SIZE_MAX_LESS)
		{
			div = pos[0].Height - SIZE_MAX_LESS;
			dest_pos->Height += (font->tf_YSize * div);
		}
	}
	else
	{
		// Square?
		if (pos[0].Height == POS_SQUARE)
			square_h = 1;

		/* a proportion of space */
		if (pos[0].Height >= POS_PROPORTION)
		{
			div = (pos[0].Height - POS_PROPORTION) * inner.Height;
			val = UDivMod32(div, 100);
			dest_pos->Height = val + pos[1].Height;
		}

		/* Normal operation */
		else
		{
			dest_pos->Height = (pos[0].Height * font->tf_YSize) + pos[1].Height;
		}
	}

	/* Is this gadget to be top-positioned specially ? */
	if (pos[0].Top == POS_CENTER)
	{
		/* Centered */
		dest_pos->Top = ((inner.Height - dest_pos->Height) >> 1) + inner.Top + pos[1].Top;
	}
	else if (pos[0].Top <= POS_RIGHT_JUSTIFY)
	{
		/* Right-justified */
		dest_pos->Top = ((inner.Top + inner.Height) - dest_pos->Height) + pos[1].Top;

		/* Adjust by any character factor */
		dest_pos->Top += (pos[0].Top - POS_RIGHT_JUSTIFY) * font->tf_YSize;
	}

	// Square width?
	if (square_w)
		dest_pos->Width = dest_pos->Height;
	else if (square_h)
		dest_pos->Height = dest_pos->Width;

	/* Adjust size if too large */
	if (!(flags & WINDOW_LAYOUT_ADJUST))
	{
		if (dest_pos->Width > inner.Width)
		{
			dest_pos->Width = inner.Width;
			success = 0;
		}
		if (dest_pos->Height > inner.Height)
		{
			dest_pos->Height = inner.Height;
			success = 0;
		}
	}

	/* Adjust position if too wayout */
	if (dest_pos->Left + dest_pos->Width > inner.Width + inner.Left)
	{
		if (flags & WINDOW_LAYOUT_ADJUST && window)
			sizex = (dest_pos->Left + dest_pos->Width + 4) - inner.Width;
		else
			dest_pos->Left = (inner.Width + inner.Left) - dest_pos->Width;
	}

	if (dest_pos->Top + dest_pos->Height > inner.Height + inner.Top)
	{
		if (flags & WINDOW_LAYOUT_ADJUST && window)
			sizey = (dest_pos->Top + dest_pos->Height + 4) - inner.Height;
		else
			dest_pos->Top = (inner.Height + inner.Top) - dest_pos->Height;
	}

	// Size window?
	if (window && (sizex || sizey))
		SizeWindow(window, sizex, sizey);

	return success;
}

/****************************************************************************
					Calculate a window's position and size
 ****************************************************************************/

int LIBFUNC L_CalcWindowDims(REG(a0, struct Screen *screen),
							 REG(a1, ConfigWindow *win_pos),
							 REG(a2, struct IBox *dest_pos),
							 REG(a3, struct TextFont *font),
							 REG(d0, ULONG flags))
{
	int width, height, ret;
	struct IBox pos[2];

	// Get copy of position
	pos[0].Left = win_pos->char_dim.Left;
	pos[0].Top = win_pos->char_dim.Top;
	pos[0].Width = win_pos->char_dim.Width;
	pos[0].Height = win_pos->char_dim.Height;
	pos[1].Left = win_pos->fine_dim.Left;
	pos[1].Top = win_pos->fine_dim.Top;
	pos[1].Width = win_pos->fine_dim.Width;
	pos[1].Height = win_pos->fine_dim.Height;

	// Get width and height extra needed for border sizes
	width = screen->WBorLeft + screen->WBorRight;
	height = screen->WBorTop + screen->WBorBottom;
	if (!(flags & WINDOW_NO_BORDER))
		height += screen->RastPort.TxHeight + 1;

	// Size gadget?
	if (flags & (WINDOW_SIZE_BOTTOM | WINDOW_SIZE_RIGHT))
	{
		struct DrawInfo *dr;

		// Get DrawInfo
		if ((dr = GetScreenDrawInfo(screen)))
		{
			struct Image *image;

			// Get size image
			if ((image =
					 (struct Image *)NewObject(0, "sysiclass", SYSIA_DrawInfo, dr, SYSIA_Which, SIZEIMAGE, TAG_END)))
			{
				// Adjust size for size gadget
				if (flags & WINDOW_SIZE_BOTTOM)
					height -= screen->WBorBottom - image->Height;
				else if (flags & WINDOW_SIZE_RIGHT)
					width -= screen->WBorRight - image->Width;

				// Free image
				DisposeObject((Object *)image);
			}

			// Free DrawInfo
			FreeScreenDrawInfo(screen, dr);
		}
	}

	// Add these to the fine position
	pos[1].Width += width;
	pos[1].Height += height;

	// Calculate size for this object
	ret = L_CalcObjectDims(screen, font, pos, dest_pos, 0, WINDOW_SCREEN_PARENT, 0, 0);

	return ret;
}

/****************************************************************************
					  Add a list of objects to a window
 ****************************************************************************/

ObjectList *LIBFUNC L_AddObjectList(REG(a0, struct Window *window),
									REG(a1, const ObjectDef *objects),
									REG(a6, struct MyLibrary *libbase))
{
	ObjectList *list;
	GL_Object *new_object = 0, *last_object = 0;
	struct IBox dims[2];
	struct NewGadget newgad = {0};
	struct Gadget *tempgad, *first_gad = 0;
	WindowData *data;
	APTR visinfo;
	short num;
	struct LibData *ldata;

#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Check valid window and object list
	if (!window || !objects)
		return 0;

	// Get data and visual info
	data = (WindowData *)window->UserData;
	ldata = (struct LibData *)libbase->ml_UserData;
	visinfo = data->visinfo;

	// Allocate ObjectList
	if (!(list = L_AllocMemH(data->memory, sizeof(ObjectList))))
		return 0;

	// Add list to window
	AddTail((struct List *)&data->object_list, (struct Node *)list);

	// Get font for window, store window
	AskFont(window->RPort, &list->attr);
	list->window = window;

	// Go through object list
	for (num = 0; objects[num].type != OD_END; num++)
	{
		BOOL done = 0;
		struct TagItem *tag;

		// Skip?
		if (objects[num].type == OD_SKIP)
			continue;

		// Create object
		if (!new_object)
		{
			// Allocate object
			if (!(new_object = L_AllocMemH(data->memory, sizeof(GL_Object))))
				break;

			// Link object in
			if (!last_object)
				list->firstobject = new_object;
			else
				last_object->next = new_object;
		}

		// Store stuff
		new_object->type = objects[num].type;
		new_object->id = objects[num].gadgetid;
		new_object->flags = objects[num].flags;
		new_object->object_kind = objects[num].object_kind;
		new_object->control_id = (unsigned short)-1;

		// Get object text
		if (objects[num].gadget_text)
		{
			// Is it a real string?
			if (objects[num].flags & TEXTFLAG_TEXT_STRING)
				new_object->original_text = (char *)objects[num].gadget_text;

			// Otherwise it's a locale string
			else
				new_object->original_text = L_GetString(data->locale, objects[num].gadget_text);

			// Get copy of original text
			if ((new_object->text = alloc_object_memory(&new_object->memory, strlen(new_object->original_text) + 1)))
				strcpy(new_object->text, new_object->original_text);
		}

		// Copy dimensions
		dims[0] = objects[num].char_dims;
		dims[1] = objects[num].fine_dims;

		// Relative to an existing object?
		if ((tag = FindTagItem(GTCustom_LayoutPos, objects[num].taglist)) ||
			(tag = FindTagItem(GTCustom_LayoutRel, objects[num].taglist)))
		{
			GL_Object *object;

			if ((object = L_GetObject(list, tag->ti_Data)) || (object = L_GetObject(OBJLIST(window), tag->ti_Data)))
			{
				L_CalcObjectDims(window,
								 window->RPort->Font,
								 dims,
								 (struct IBox *)(APTR)&newgad,
								 last_object,
								 (tag->ti_Tag == GTCustom_LayoutRel)
									 ? WINDOW_OBJECT_PARENT
									 : ((WINFLAG(window) & WINDOW_LAYOUT_ADJUST) ? WINDOW_LAYOUT_ADJUST : 0),
								 new_object,
								 object);
				done = 1;
			}
		}

		// Get object dimensions
		if (!done)
			L_CalcObjectDims(window,
							 window->RPort->Font,
							 dims,
							 (struct IBox *)(APTR)&newgad,
							 last_object,
							 (WINFLAG(window) & WINDOW_LAYOUT_ADJUST) ? WINDOW_LAYOUT_ADJUST : 0,
							 new_object,
							 0);

		// Copy dimensions
		new_object->dims.Left = newgad.ng_LeftEdge;
		new_object->dims.Top = newgad.ng_TopEdge;
		new_object->dims.Width = newgad.ng_Width;
		new_object->dims.Height = newgad.ng_Height;

		// Store original dimensions
		new_object->char_dims = objects[num].char_dims;
		new_object->fine_dims = objects[num].fine_dims;

		// Set as last object
		last_object = new_object;

		// Get object type
		switch (objects[num].type)
		{
		// Gadget
		case OD_GADGET: {
			struct TagItem *taglist;
			struct TagItem *tag, *lasttag;
			struct Gadget *firstgad;
			HookData *hook = 0;
			short place;

			// Set gadget text and font
			newgad.ng_GadgetText = new_object->text;
			newgad.ng_TextAttr = &list->attr;

			// Fill out other newgadget fields
			newgad.ng_GadgetID = objects[num].gadgetid;
			newgad.ng_Flags = objects[num].flags & ((1 << 16) - 1);
			newgad.ng_VisualInfo = visinfo;
			newgad.ng_UserData = (APTR)&objects[num];

			// Get text placement
			place = objects[num].flags & (PLACETEXT_LEFT | PLACETEXT_RIGHT | PLACETEXT_IN | PLACETEXT_ABOVE);

			// Make a copy of the taglist
			taglist = CloneTagItems(objects[num].taglist);

			// Look for call-back hooks
			lasttag = 0;
			while ((tag = FindTagItem(GTCustom_CallBack, taglist)))
			{
				// Check that this tag hasn't been found before
				if (lasttag == tag)
					break;

#ifdef __AROS__
				// Call the call-back with its real arguments
				((void (*)(struct TagItem *, struct Window *))(IPTR)tag->ti_Data)(tag, window);
#else
				{
					struct Hook hook;

					// Fill out a hook
					hook.h_Entry = (ULONG(*)())tag->ti_Data;
					hook.h_SubEntry = 0;
					hook.h_Data = 0;

					// Call the hook
					CallHookPkt(&hook, (void *)window, (void *)tag);
				}
#endif

				// Remember this tag
				lasttag = tag;
			}

			// Key equivalent?
			if ((tag = FindTagItem(GTCustom_LocaleKey, taglist)))
			{
				char *text;
				short pos;

				// Get text string
				text = L_GetString(data->locale, tag->ti_Data);

				// Look for underscore
				for (pos = 0; text[pos]; pos++)
				{
					if (text[pos] == '_')
					{
						new_object->key = (unsigned char)text[pos + 1];
						if (new_object->key >= 'A' && new_object->key <= 'Z')
							new_object->key += ('a' - 'A');
						break;
					}
				}
			}

			// If this is a CYCLE_KIND or MX_KIND gadget, search tag list
			// for GTCustom_LocaleLabels (to handle Locale)
			if (new_object->object_kind == CYCLE_KIND || new_object->object_kind == MX_KIND)
			{
				if ((tag = FindTagItem(GTCustom_LocaleLabels, taglist)))
				{
					short count;
					char **labels;

					// Count labels
					for (count = 0; ((UWORD *)tag->ti_Data)[count]; count++)
						;

					// Allocate label buffer
					if ((labels = alloc_object_memory(&new_object->memory, sizeof(char *) * (count + 1))))
					{
						// Get labels
						for (count = 0; ((UWORD *)tag->ti_Data)[count]; count++)
							labels[count] = L_GetString(data->locale, ((UWORD *)tag->ti_Data)[count]);
					}

					// Set label pointer, change tag
					if (new_object->object_kind == CYCLE_KIND)
						tag->ti_Tag = GTCY_Labels;
					else
						tag->ti_Tag = GTMX_Labels;
					tag->ti_Data = (IPTR)labels;

					// Store label count
					new_object->gl_info.gl_gadget.choice_max = count;
				}
			}

			// Or a slider?
			else if (new_object->object_kind == SLIDER_KIND)
			{
				if ((tag = FindTagItem(GTSL_Min, taglist)))
					new_object->gl_info.gl_gadget.choice_min = tag->ti_Data;
				if ((tag = FindTagItem(GTSL_Max, taglist)))
					new_object->gl_info.gl_gadget.choice_max = tag->ti_Data + 1;
			}

			/*
								// Or a scroller?
								else if (new_object->object_kind==SLIDER_KIND)
								{
									new_object->gl_info.gl_gadget.choice_min=0;
									if (tag=FindTagItem(GTSL_Max,taglist))
										new_object->gl_info.gl_gadget.choice_max=tag->ti_Data-1;
								}
			*/

			// Something else
			else if ((tag = FindTagItem(GTCustom_MinMax, taglist)))
			{
				*((unsigned short *)&new_object->gl_info.gl_gadget.choice_min) =
					(unsigned short)(tag->ti_Data & 0xffff);
				*((unsigned short *)&new_object->gl_info.gl_gadget.choice_max) =
					(unsigned short)((tag->ti_Data >> 16) & 0xffff);
			}

			// Otherwise clear min/max
			else
			{
				new_object->gl_info.gl_gadget.choice_min = 0;
				new_object->gl_info.gl_gadget.choice_max = 0;
			}

			// String?
			if (new_object->object_kind == STRING_KIND || new_object->object_kind == FIELD_KIND ||
				new_object->object_kind == HOTKEY_KIND || new_object->object_kind == INTEGER_KIND)
			{
				// Allocate hook
				if ((hook = alloc_object_memory(&new_object->memory, sizeof(HookData))))
				{
					struct TagItem *tag;

// Fill out hook
#if defined(__AROS__) || defined(__MORPHOS__)
					hook->hook.h_Entry = (HOOKFUNC)HookEntry;
					hook->hook.h_SubEntry = (HOOKFUNC)string_edit_hook;
#else
					hook->hook.h_Entry = (ULONG(*)())string_edit_hook;
#endif

					// Don't select next field?
					if (GetTagData(GTCustom_NoSelectNext, 0, taglist))
						new_object->flags2 |= OBJECTF_NO_SELECT_NEXT;

					// Path filter?
					if (GetTagData(GTCustom_PathFilter, 0, taglist))
						new_object->flags2 |= OBJECTF_PATH_FILTER;

					// Integer gadget?
					if (new_object->object_kind == INTEGER_KIND || GetTagData(GTCustom_Integer, 0, taglist))
						new_object->flags2 |= OBJECTF_INTEGER;

					// Uppercase only?
					if (GetTagData(GTCustom_UpperCase, 0, taglist))
						new_object->flags2 |= OBJECTF_UPPERCASE;

					// Field kind (means no editing)
					if (new_object->object_kind == FIELD_KIND)
					{
						// Set flag
						new_object->flags2 |= OBJECTF_READ_ONLY;

						// Change type to normal string
						new_object->object_kind = STRING_KIND;
					}

					// Hotkey kind
					else if (new_object->object_kind == HOTKEY_KIND)
					{
						// Set flag
						new_object->flags2 |= OBJECTF_HOTKEY;

						// Change type to normal string
						new_object->object_kind = STRING_KIND;
					}

					// Secure?
					if ((tag = FindTagItem(GTCustom_Secure, taglist)) && tag->ti_Data)
					{
						// Set secure flag
						new_object->flags2 |= OBJECTF_SECURE;

						// Change tag to put gadget into replace mode
						tag->ti_Tag = STRINGA_ReplaceMode;
					}

					// History list
					hook->history = (Att_List *)GetTagData(GTCustom_History, 0, taglist);

					// Change data
					hook->change_task = (struct Task *)GetTagData(GTCustom_ChangeSigTask, 0, taglist);
					hook->change_bit = GetTagData(GTCustom_ChangeSigBit, 0, taglist);

					// Store flags
					hook->flags = new_object->flags2;
				}
			}

			// Control tag
			if ((tag = FindTagItem(GTCustom_Control, taglist)))
				new_object->control_id = tag->ti_Data;

			// Type of gadget?
			switch (new_object->object_kind)
			{
			// Custom Listview
			case MY_LISTVIEW_KIND: {
				BOOL thin = 0;

				// Thin borders?
				if (GetTagData(DLV_ThinBorder, 0, taglist) || ldata->flags & LIBDF_THIN_BORDERS)
					thin = 1;

				new_object->gl_info.gl_gadget.gadget = (struct Gadget *)NewObject(0,
																				  "dopuslistviewgclass",
																				  GA_Left,
																				  newgad.ng_LeftEdge,
																				  GA_Top,
																				  newgad.ng_TopEdge,
																				  GA_Width,
																				  newgad.ng_Width,
																				  GA_Height,
																				  newgad.ng_Height,
																				  GA_ID,
																				  newgad.ng_GadgetID,
																				  GA_UserData,
																				  newgad.ng_UserData,
																				  GA_RelVerify,
																				  TRUE,
																				  GA_Text,
																				  newgad.ng_GadgetText,
																				  GTCustom_TextAttr,
																				  &list->attr,
																				  DLV_ThinBorder,
																				  thin,
																				  ICA_TARGET,
																				  ICTARGET_IDCMP,
																				  TAG_MORE,
																				  (IPTR)taglist);
			}
			break;

			// Button/Frame
			case BUTTON_KIND:
			case FRAME_KIND: {
				BOOL thin = 0;

				// Thin borders?
				if (objects[num].flags & BUTTONFLAG_THIN_BORDERS || GetTagData(GTCustom_ThinBorders, 0, taglist) ||
					ldata->flags & LIBDF_THIN_BORDERS)
					thin = 1;

				new_object->gl_info.gl_gadget.gadget = (struct Gadget *)NewObject(
					0,
					(new_object->object_kind == BUTTON_KIND) ? "dopusbuttongclass" : "dopusframeclass",
					GA_Left,
					newgad.ng_LeftEdge,
					GA_Top,
					newgad.ng_TopEdge,
					GA_Width,
					newgad.ng_Width,
					GA_Height,
					newgad.ng_Height,
					GA_ID,
					newgad.ng_GadgetID,
					GA_UserData,
					newgad.ng_UserData,
					GA_RelVerify,
					TRUE,
					GA_Immediate,
					(objects[num].flags & BUTTONFLAG_IMMEDIATE) ? TRUE : FALSE,
					GA_Text,
					newgad.ng_GadgetText,
					GTCustom_TextAttr,
					&list->attr,
					GTCustom_ThinBorders,
					thin,
					GTCustom_Bold,
					(objects[num].flags & BUTTONFLAG_OKAY_BUTTON) ? 1 : 0,
					GTCustom_TextPlacement,
					place,
					ICA_TARGET,
					ICTARGET_IDCMP,
					TAG_MORE,
					(IPTR)taglist);
			}
			break;

			// String
			case STRING_KIND:
			case INTEGER_KIND: {
				BOOL thin = 0;

				// Thin borders?
				if (GetTagData(GTCustom_ThinBorders, 0, taglist) || ldata->flags & LIBDF_THIN_BORDERS)
					thin = 1;

				// Create gadget
				new_object->gl_info.gl_gadget.gadget =
					(struct Gadget *)NewObject(0,
											   "dopusstrgclass",
											   GA_Left,
											   newgad.ng_LeftEdge,
											   GA_Top,
											   newgad.ng_TopEdge,
											   GA_Width,
											   newgad.ng_Width,
											   GA_Height,
											   newgad.ng_Height,
											   GA_ID,
											   newgad.ng_GadgetID,
											   GA_UserData,
											   newgad.ng_UserData,
											   GA_RelVerify,
											   TRUE,
											   GA_Text,
											   newgad.ng_GadgetText,
											   GTCustom_TextAttr,
											   &list->attr,
											   GTCustom_ThinBorders,
											   thin,
											   (hook) ? STRINGA_EditHook : TAG_IGNORE,
											   hook,
											   GTCustom_TextPlacement,
											   place,
											   ICA_TARGET,
											   ICTARGET_IDCMP,
											   TAG_MORE,
											   (IPTR)taglist);
			}
			break;

			// Check box
			case CHECKBOX_KIND: {
				BOOL thin = 0;

				// Thin borders?
				if (objects[num].flags & BUTTONFLAG_THIN_BORDERS || GetTagData(GTCustom_ThinBorders, 0, taglist) ||
					ldata->flags & LIBDF_THIN_BORDERS)
					thin = 1;

				new_object->gl_info.gl_gadget.gadget = (struct Gadget *)NewObject(0,
																				  "dopuscheckgclass",
																				  GA_Left,
																				  newgad.ng_LeftEdge,
																				  GA_Top,
																				  newgad.ng_TopEdge,
																				  GA_Width,
																				  newgad.ng_Width,
																				  GA_Height,
																				  newgad.ng_Height,
																				  GA_ID,
																				  newgad.ng_GadgetID,
																				  GA_UserData,
																				  newgad.ng_UserData,
																				  GA_RelVerify,
																				  TRUE,
																				  GA_Text,
																				  newgad.ng_GadgetText,
																				  GTCustom_TextAttr,
																				  &list->attr,
																				  GTCustom_ThinBorders,
																				  thin,
																				  GTCustom_TextPlacement,
																				  place,
																				  ICA_TARGET,
																				  ICTARGET_IDCMP,
																				  TAG_MORE,
																				  (IPTR)taglist);
			}
			break;

			// Number/Text view
			case NUMBER_KIND:
			case TEXT_KIND: {
				BOOL thin = 0;

				// Thin borders?
				if (objects[num].flags & BUTTONFLAG_THIN_BORDERS || GetTagData(GTCustom_ThinBorders, 0, taglist) ||
					ldata->flags & LIBDF_THIN_BORDERS)
					thin = 1;

				// Create object
				new_object->gl_info.gl_gadget.gadget = (struct Gadget *)NewObject(0,
																				  "dopusviewgclass",
																				  GA_Left,
																				  newgad.ng_LeftEdge,
																				  GA_Top,
																				  newgad.ng_TopEdge,
																				  GA_Width,
																				  newgad.ng_Width,
																				  GA_Height,
																				  newgad.ng_Height,
																				  GA_ID,
																				  newgad.ng_GadgetID,
																				  GA_UserData,
																				  newgad.ng_UserData,
																				  GA_Text,
																				  newgad.ng_GadgetText,
																				  GTCustom_TextAttr,
																				  &list->attr,
																				  GTCustom_ThinBorders,
																				  thin,
																				  GTCustom_TextPlacement,
																				  place,
																				  ICA_TARGET,
																				  ICTARGET_IDCMP,
																				  TAG_MORE,
																				  (IPTR)taglist);
			}
			break;

			// Palette box
			case PALETTE_KIND: {
				BOOL thin = 0;

				// Thin borders?
				if (objects[num].flags & BUTTONFLAG_THIN_BORDERS || GetTagData(GTCustom_ThinBorders, 0, taglist) ||
					ldata->flags & LIBDF_THIN_BORDERS)
					thin = 1;

				new_object->gl_info.gl_gadget.gadget = (struct Gadget *)NewObject(0,
																				  "dopuspalettegclass",
																				  GA_Left,
																				  newgad.ng_LeftEdge,
																				  GA_Top,
																				  newgad.ng_TopEdge,
																				  GA_Width,
																				  newgad.ng_Width,
																				  GA_Height,
																				  newgad.ng_Height,
																				  GA_ID,
																				  newgad.ng_GadgetID,
																				  GA_UserData,
																				  newgad.ng_UserData,
																				  GA_RelVerify,
																				  TRUE,
																				  GA_Immediate,
																				  TRUE,
																				  GA_Text,
																				  newgad.ng_GadgetText,
																				  GTCustom_TextAttr,
																				  &list->attr,
																				  GTCustom_ThinBorders,
																				  thin,
																				  ICA_TARGET,
																				  ICTARGET_IDCMP,
																				  TAG_MORE,
																				  (IPTR)taglist);
			}
			break;

			// File/dir button
			case FILE_BUTTON_KIND:
			case DIR_BUTTON_KIND:
			case FONT_BUTTON_KIND:
			case POPUP_BUTTON_KIND: {
				struct TagItem *tag;
				BOOL thin = 0;

				// Thin borders?
				if (objects[num].flags & BUTTONFLAG_THIN_BORDERS || GetTagData(GTCustom_ThinBorders, 0, taglist) ||
					ldata->flags & LIBDF_THIN_BORDERS)
					thin = 1;

				new_object->gl_info.gl_gadget.image = (struct Image *)NewObject(0,
																				"dopusiclass",
																				IA_Width,
																				newgad.ng_Width,
																				IA_Height,
																				newgad.ng_Height,
																				DIA_Type,
																				IM_DRAWER,
																				TAG_DONE);

				new_object->gl_info.gl_gadget.gadget = (struct Gadget *)NewObject(0,
																				  "dopusbuttongclass",
																				  GA_Left,
																				  newgad.ng_LeftEdge,
																				  GA_Top,
																				  newgad.ng_TopEdge,
																				  GA_Width,
																				  newgad.ng_Width,
																				  GA_Height,
																				  newgad.ng_Height,
																				  GA_ID,
																				  newgad.ng_GadgetID,
																				  GA_UserData,
																				  newgad.ng_UserData,
																				  GA_RelVerify,
																				  TRUE,
																				  GA_Image,
																				  new_object->gl_info.gl_gadget.image,
																				  GA_Text,
																				  newgad.ng_GadgetText,
																				  GTCustom_TextPlacement,
																				  PLACETEXT_LEFT,
																				  GTCustom_ThinBorders,
																				  thin,
																				  GTCustom_TextAttr,
																				  &list->attr,
																				  TAG_MORE,
																				  (IPTR)taglist);

				// Default path?
				if ((tag = FindTagItem(DFB_DefPath, taglist)))
					new_object->data_ptr = tag->ti_Data;
			}
			break;

			// Normal gadtools gadget
			default: {
				// Create gadget context
				if (CreateContext(&new_object->gl_info.gl_gadget.context))
				{
					new_object->gl_info.gl_gadget.gadget = CreateGadget(new_object->object_kind,
																		new_object->gl_info.gl_gadget.context,
																		&newgad,
																		GT_Underscore,
																		(ULONG)'_',
																		GTCB_Scaled,
																		TRUE,
																		GTMX_Scaled,
																		TRUE,
																		GTMX_Spacing,
																		2,
																		(hook) ? STRINGA_EditHook : TAG_IGNORE,
																		hook,
																		TAG_MORE,
																		(IPTR)taglist);
				}
			}
			break;
			}

			// Want to keep the taglist?
			if (FindTagItem(GTCustom_CopyTags, taglist))
			{
				// Store taglist pointer
				new_object->tags = taglist;
			}

			// Free the temporary taglist
			else
			{
				FreeTagItems(taglist);
				taglist = 0;
			}

			// Did gadget fail?
			if (!new_object->gl_info.gl_gadget.gadget)
			{
				// It failed, free stuff
				FreeGadgets(new_object->gl_info.gl_gadget.context);
				DisposeObject((Object *)new_object->gl_info.gl_gadget.image);
				L_FreeMemHandle(new_object->memory);
				if (taglist)
					FreeTagItems(taglist);
				new_object->memory = 0;
				new_object->tags = 0;
				break;
			}

			// Count the number of components in this gadget
			if (!(firstgad = new_object->gl_info.gl_gadget.context))
				firstgad = new_object->gl_info.gl_gadget.gadget;
			for (tempgad = firstgad, new_object->gl_info.gl_gadget.components = 0; tempgad;
				 new_object->gl_info.gl_gadget.components++)
			{
				// Get next gadget
				tempgad = tempgad->NextGadget;
			}

			// See if the gadget has a user-supplied image
			if ((tag = FindTagItem(GTCustom_Image, objects[num].taglist)))
			{
				_layout_add_gad_image(&new_object->memory, GADGET(new_object), (struct Image *)tag->ti_Data);
			}

			// Toggle-select gadget?
			if (objects[num].flags & BUTTONFLAG_TOGGLE_SELECT)
			{
				GADGET(new_object)->Activation |= GACT_TOGGLESELECT;
			}

			// String gadgets get their data pointer set to point at the buffer
			if (new_object->object_kind == STRING_KIND)
				new_object->gl_info.gl_gadget.data =
					(IPTR)((struct StringInfo *)GADGET(new_object)->SpecialInfo)->Buffer;

			// Add gadget to window
			AddGList(window, firstgad, -1, new_object->gl_info.gl_gadget.components, 0);

			// Is this the first new gadget?
			if (!first_gad)
				first_gad = firstgad;
		}

			// Set to allocate a new object
			new_object = 0;
			break;

		// Area
		case OD_AREA:

			// Title area?
			if (objects[num].flags & AREAFLAG_TITLE)
			{
				// Not height-relative to something?
				if (objects[num].char_dims.Height >= 0)
				{
					// Increase height by half font-height
					new_object->dims.Height += window->RPort->Font->tf_YSize >> 1;

					// Bottom-relative?
					if (objects[num].char_dims.Top == POS_RIGHT_JUSTIFY)
					{
						// Move up
						new_object->dims.Top -= window->RPort->Font->tf_YSize >> 1;
					}
				}
			}

			// Store area size
			new_object->gl_info.gl_area.area_pos = new_object->dims;

			// Derive frametype
			if (objects[num].flags & AREAFLAG_ICON)
				new_object->gl_info.gl_area.frametype = BBFT_ICONDROPBOX;
			else
				new_object->gl_info.gl_area.frametype = BBFT_BUTTON;

			// Fall through to text

		// Text
		case OD_TEXT: {
			struct IBox dims;

			// Copy dimensions
			dims = new_object->dims;

			// See if there's any text
			if (new_object->text)
			{
				int tlen, len;

				// Handle underscores for text
				if (objects[num].type == OD_TEXT)
				{
					short uscore = -1;

					// Allowed underscores?
					if (!(new_object->flags & TEXTFLAG_NO_USCORE))
					{
						// Any underscores?
						for (tlen = 0; new_object->text[tlen]; tlen++)
						{
							if (new_object->text[tlen] == '_')
								uscore = tlen;
						}
					}

					// If so, we need to strip it out
					if (uscore > -1)
					{
						// Copy from original text, skipping underscore
						for (len = 0, tlen = 0; new_object->original_text[len]; len++)
						{
							// Ok to copy?
							if (len != uscore)
								new_object->text[tlen++] = new_object->original_text[len];
						}

						// Null-terminate
						new_object->text[tlen] = 0;

						// Store underscore position
						new_object->gl_info.gl_text.uscore_pos = uscore;
					}

					// Clear underscore position
					else
						new_object->gl_info.gl_text.uscore_pos = -1;
				}

				// Get string length
				len = TextLength(window->RPort, new_object->text, (tlen = strlen(new_object->text)));

				// Fix justification
				if (new_object->flags & TEXTFLAG_RIGHT_JUSTIFY)
					//							dims.Left-=len;
					dims.Left += dims.Width - len;

				else if (objects[num].flags & TEXTFLAG_CENTER)
				{
					dims.Left += (dims.Width - len) >> 1;

					// Center vertically too for areas
					if (objects[num].type == OD_AREA)
						dims.Top += (dims.Height - window->RPort->TxHeight) >> 1;
				}

				// Store string size
				new_object->gl_info.gl_text.text_pos.Width = len;
				new_object->gl_info.gl_text.text_pos.Height = window->RPort->TxHeight;
			}
			else
			{
				// Store string size
				new_object->gl_info.gl_text.text_pos.Width = dims.Width;
				new_object->gl_info.gl_text.text_pos.Height = dims.Height;

				// Clear underscore for text
				if (new_object->type == OD_TEXT)
					new_object->gl_info.gl_text.uscore_pos = -1;
			}

			// Store string position
			new_object->gl_info.gl_text.text_pos.Left = dims.Left;
			new_object->gl_info.gl_text.text_pos.Top = dims.Top;
			if (objects[num].type == OD_TEXT)
				new_object->gl_info.gl_text.base_pos = dims.Top + window->RPort->TxBaseline;

			// Store pens
			new_object->fg = data->drawinfo->dri_Pens[new_object->object_kind];
			new_object->bg = data->drawinfo->dri_Pens[BACKGROUNDPEN];

			// Want to keep the taglist?
			if (FindTagItem(GTCustom_CopyTags, objects[num].taglist))
			{
				// Make a copy of the taglist
				new_object->tags = CloneTagItems(objects[num].taglist);
			}

			// Set to allocate a new object
			new_object = 0;
		}
		break;
		}
	}

	// Refresh window and gadgets
	L_RefreshObjectList(window, list);
	if (first_gad)
	{
		RefreshGList(first_gad, window, 0, -1);
		GT_RefreshWindow(window, 0);
	}

	return list;
}

/****************************************************************************
								Free an object
 ****************************************************************************/

void LIBFUNC L_FreeObject(REG(a0, ObjectList *objlist), REG(a1, GL_Object *object))
{
	GL_Object *link, *last = 0;
	int count;

	// Check for valid list and object
	if (!objlist || !object)
		return;

	// See if this is a gadget
	if (object->type == OD_GADGET)
	{
		struct Gadget *firstgad, *lastgad;

		// Gadtools gadget?
		if (IS_GADTOOLS(object))
			firstgad = object->gl_info.gl_gadget.context;

		// BOOPSI gadget
		else
			firstgad = object->gl_info.gl_gadget.gadget;

		// Find last gadget component
		for (count = 0, lastgad = firstgad; count < (object->gl_info.gl_gadget.components - 1) && lastgad; count++)
			lastgad = lastgad->NextGadget;

		// Remove this gadget from the list
		RemoveGList(objlist->window, firstgad, object->gl_info.gl_gadget.components);

		// Clear the NextGadget pointer of the last gadget component to isolate it
		if (lastgad)
			lastgad->NextGadget = 0;

		// Free this gadget
		if (IS_GADTOOLS(object))
			FreeGadgets(object->gl_info.gl_gadget.context);
		else
			DisposeObject((Object *)object->gl_info.gl_gadget.gadget);

		// Free image
		DisposeObject((Object *)object->gl_info.gl_gadget.image);
	}

	// Free object memory
	L_FreeMemHandle(object->memory);

	// Free tags
	if (object->tags)
		FreeTagItems(object->tags);

	// Detach this object from the list
	if ((link = objlist->firstobject) == object)
		objlist->firstobject = object->next;
	else
	{
		while (link)
		{
			if (link == object)
			{
				if (last)
					last->next = link->next;
				break;
			}
			last = link;
			link = link->next;
		}
	}

	// Free this object
	L_FreeMemH(object);
}

/****************************************************************************
							Free a list of objects
 ****************************************************************************/

void LIBFUNC L_FreeObjectList(REG(a0, ObjectList *objlist))
{
	// Valid list?
	if (objlist)
	{
		// Free objects
		while (objlist->firstobject)
			L_FreeObject(objlist, objlist->firstobject);

		// Remove object list from window
		Remove((struct Node *)objlist);

		// Free object list memory
		L_FreeMemH(objlist);
	}
}

/****************************************************************************
						Get an object by ID from a list
 ****************************************************************************/

GL_Object *LIBFUNC L_GetObject(REG(a0, ObjectList *list), REG(d0, int id))
{
	GL_Object *object;

	// Go through linked lists
	for (; list->node.mln_Succ; list = (ObjectList *)list->node.mln_Succ)
	{
		// Get first
		object = list->firstobject;

		// Go through objects
		while (object)
		{
			// Check ID
			if (object->id == id)
				return object;

			// Get next
			object = object->next;
		}
	}

	// Not found
	return 0;
}

/****************************************************************************
							  Display an object
 ****************************************************************************/

void LIBFUNC L_DisplayObject(REG(a0, struct Window *window),
							 REG(a1, GL_Object *object),
							 REG(d0, int fg),
							 REG(d1, int bg),
							 REG(a2, char *txt))
{
	struct RastPort rp;
	struct IBox text_pos, area_pos = {0};
	short text_width = 0, text_len = 0;
	BOOL first = 0;
	BOOL optimref = 0;
	WindowData *data;

	// Valid window and object?
	if (!window || !object)
		return;

	// Don't do anything for gadgets (yet)
	if (object->type == OD_GADGET)
		return;

	// Get window data
	data = (WindowData *)window->UserData;

	// Get pens
	if (fg == -1)
		fg = object->fg;
	else
		object->fg = fg;
	if (bg == -1)
		bg = object->bg;
	else
		object->bg = bg;

	// Use original text?
	if (txt == (char *)-1)
		txt = object->original_text;

	// First draw?
	if (!(object->flags & OBJECTFLAG_DRAWN))
	{
		object->flags |= OBJECTFLAG_DRAWN;
		first = 1;
	}

	// Change text?
	if (txt && (object->text == 0 || (strcmp(txt, object->text) != 0)))
	{
		// Free existing text
		L_FreeMemH(object->text);

		// Copy text
		if ((object->text = alloc_object_memory(&object->memory, strlen(txt) + 1)))
			strcpy(object->text, txt);
	}

	// Get text pointer
	if (!(txt = object->text))
		txt = object->original_text;

	// Get text length
	if (txt)
		text_len = strlen(txt);

	// Get rastport copy
	rp = *(window->RPort);

	// Get default text position
	text_pos = object->gl_info.gl_text.text_pos;

	// Get object type
	switch (object->type)
	{
	// Area
	case OD_AREA:

		// Erase interior if asked for
		if (object->flags & AREAFLAG_ERASE || (first && !(object->flags & AREAFLAG_NOFILL)))
		{
			struct IBox box;

			// Get object box
			box = object->gl_info.gl_area.area_pos;

			// Border?
			if ((object->flags & AREAFLAG_RAISED) || (object->flags & AREAFLAG_RECESSED))
			{
				// Move vertical borders
				box.Top += 1;
				box.Height -= 2;

				// Thin border?
				if (object->flags & AREAFLAG_THIN)
				{
					box.Left += 1;
					box.Width -= 2;
				}

				// Thick border
				else
				{
					box.Left += 2;
					box.Width -= 4;
				}
			}

			// Got a title?
			if (object->flags & AREAFLAG_TITLE)
			{
				box.Top += (rp.TxHeight >> 1);
				box.Height -= (rp.TxHeight >> 1) + 2;
				box.Left += 2;
				box.Width -= 4;
			}

			// Erase background
			if (bg > 0 || object->flags & AREAFLAG_FILL_COLOUR)
			{
				SetAPen(&rp, bg);
				RectFill(&rp, box.Left - 1, box.Top, box.Left + box.Width - 1, box.Top + box.Height - 1);
			}
			else
			{
				EraseRect(&rp, box.Left - 1, box.Top, box.Left + box.Width - 1, box.Top + box.Height - 1);
			}
		}

		// Optimised refreshing?
		else if (object->flags & AREAFLAG_OPTIM)
		{
			optimref = 1;
			area_pos = object->gl_info.gl_area.area_pos;
		}

		// Box with a title?
		if (object->flags & AREAFLAG_TITLE)
		{
			short width = 0;

			// Got text?
			if (txt)
			{
				struct TextExtent extent;

				// Set bold
				SetSoftStyle(&rp, FSF_BOLD, FSF_BOLD);

				// Get amount of text we can display
				text_len = TextFit(&rp,
								   txt,
								   text_len,
								   &extent,
								   0,
								   1,
								   object->gl_info.gl_area.area_pos.Width - rp.TxWidth * 2 - 8,
								   rp.TxHeight);

				// Calculate text position
				text_pos.Left = object->gl_info.gl_area.area_pos.Left + 3 + rp.TxWidth;
				text_pos.Top = object->gl_info.gl_area.area_pos.Top - (rp.TxHeight >> 1);
				width = extent.te_Width;
			}

			// Draw lines (shadow first)
			if (first)
			{
				SetAPen(&rp, data->drawinfo->dri_Pens[SHADOWPEN]);
				Move(&rp, object->gl_info.gl_area.area_pos.Left + 3, object->gl_info.gl_area.area_pos.Top);
				Draw(&rp, object->gl_info.gl_area.area_pos.Left, object->gl_info.gl_area.area_pos.Top);
				Draw(&rp,
					 object->gl_info.gl_area.area_pos.Left,
					 object->gl_info.gl_area.area_pos.Top + object->gl_info.gl_area.area_pos.Height - 1);
				Move(&rp,
					 object->gl_info.gl_area.area_pos.Left + 4 + rp.TxWidth * 2 + width,
					 object->gl_info.gl_area.area_pos.Top);
				Draw(&rp,
					 object->gl_info.gl_area.area_pos.Left + object->gl_info.gl_area.area_pos.Width - 2,
					 object->gl_info.gl_area.area_pos.Top);
				Draw(&rp,
					 object->gl_info.gl_area.area_pos.Left + object->gl_info.gl_area.area_pos.Width - 2,
					 object->gl_info.gl_area.area_pos.Top + object->gl_info.gl_area.area_pos.Height - 2);
				Draw(&rp,
					 object->gl_info.gl_area.area_pos.Left + 1,
					 object->gl_info.gl_area.area_pos.Top + object->gl_info.gl_area.area_pos.Height - 2);

				// Shine lines
				SetAPen(&rp, data->drawinfo->dri_Pens[SHINEPEN]);
				Move(&rp, object->gl_info.gl_area.area_pos.Left + 3, object->gl_info.gl_area.area_pos.Top + 1);
				Draw(&rp, object->gl_info.gl_area.area_pos.Left + 1, object->gl_info.gl_area.area_pos.Top + 1);
				Draw(&rp,
					 object->gl_info.gl_area.area_pos.Left + 1,
					 object->gl_info.gl_area.area_pos.Top + object->gl_info.gl_area.area_pos.Height - 1);
				Draw(&rp,
					 object->gl_info.gl_area.area_pos.Left + object->gl_info.gl_area.area_pos.Width - 1,
					 object->gl_info.gl_area.area_pos.Top + object->gl_info.gl_area.area_pos.Height - 1);
				Draw(&rp,
					 object->gl_info.gl_area.area_pos.Left + object->gl_info.gl_area.area_pos.Width - 1,
					 object->gl_info.gl_area.area_pos.Top);
				Move(&rp,
					 object->gl_info.gl_area.area_pos.Left + 4 + rp.TxWidth * 2 + width,
					 object->gl_info.gl_area.area_pos.Top + 1);
				Draw(&rp,
					 object->gl_info.gl_area.area_pos.Left + object->gl_info.gl_area.area_pos.Width - 3,
					 object->gl_info.gl_area.area_pos.Top + 1);
			}
		}

		// See if we have to draw a rectangle
		else if (first && ((object->flags & AREAFLAG_RAISED) || (object->flags & AREAFLAG_RECESSED)))
		{
			// Thin border?
			if (object->flags & AREAFLAG_THIN)
			{
				struct Rectangle rect;

				// Get rectangle
				rect.MinX = object->gl_info.gl_area.area_pos.Left;
				rect.MinY = object->gl_info.gl_area.area_pos.Top;
				rect.MaxX = rect.MinX + object->gl_info.gl_area.area_pos.Width - 1;
				rect.MaxY = rect.MinY + object->gl_info.gl_area.area_pos.Height - 1;

				// Draw box
				L_DrawBox(&rp, &rect, DRAWINFO(window), (object->flags & AREAFLAG_RECESSED) ? 1 : 0);
			}

			// Thick border
			else
			{
				struct TagItem box_tags[4];

				// Fill out tags
				box_tags[0].ti_Tag = (object->flags & AREAFLAG_RECESSED) ? GTBB_Recessed : TAG_IGNORE;
				box_tags[0].ti_Data = TRUE;
				box_tags[1].ti_Tag = GTBB_FrameType;
				box_tags[1].ti_Data = object->gl_info.gl_area.frametype;
				box_tags[2].ti_Tag = GT_VisualInfo;
				box_tags[2].ti_Data = (IPTR)VISINFO(window);
				box_tags[3].ti_Tag = TAG_END;

				// Draw frame
				DrawBevelBoxA(&rp,
							  object->gl_info.gl_area.area_pos.Left,
							  object->gl_info.gl_area.area_pos.Top,
							  object->gl_info.gl_area.area_pos.Width,
							  object->gl_info.gl_area.area_pos.Height,
							  box_tags);
			}

			// Graph display?
			if (object->flags & AREAFLAG_GRAPH)
			{
				short bottom, quarter, right;

				// Get bottom and right of area
				bottom = object->gl_info.gl_area.area_pos.Top + object->gl_info.gl_area.area_pos.Height;
				right = object->gl_info.gl_area.area_pos.Left + object->gl_info.gl_area.area_pos.Width - 1;

				// Get quarter of width
				quarter = object->gl_info.gl_area.area_pos.Width >> 2;

				// Draw lines at 0, 25, 50 and 100%
				SetAPen(&rp, data->drawinfo->dri_Pens[SHADOWPEN]);
				RectFill(&rp,
						 object->gl_info.gl_area.area_pos.Left,
						 bottom,
						 object->gl_info.gl_area.area_pos.Left + 1,
						 bottom + 2);
				RectFill(&rp,
						 object->gl_info.gl_area.area_pos.Left + quarter,
						 bottom,
						 object->gl_info.gl_area.area_pos.Left + quarter + 1,
						 bottom + 2);
				RectFill(&rp,
						 object->gl_info.gl_area.area_pos.Left + (object->gl_info.gl_area.area_pos.Width >> 1),
						 bottom,
						 object->gl_info.gl_area.area_pos.Left + (object->gl_info.gl_area.area_pos.Width >> 1) + 1,
						 bottom + 2);
				RectFill(&rp, right - quarter, bottom, (right - quarter) + 1, bottom + 2);
				RectFill(&rp, right - 1, bottom, right, bottom + 2);
			}
		}

		// Or a line
		else if (first && (object->flags & AREAFLAG_LINE))
		{
			// Shine (top line)
			SetAPen(&rp, ((WindowData *)window->UserData)->drawinfo->dri_Pens[SHINEPEN]);
			Move(&rp, object->gl_info.gl_area.area_pos.Left, object->gl_info.gl_area.area_pos.Top);
			Draw(&rp,
				 object->gl_info.gl_area.area_pos.Left + object->gl_info.gl_area.area_pos.Width - 1,
				 object->gl_info.gl_area.area_pos.Top);

			// Shadow (bottom line)
			SetAPen(&rp, ((WindowData *)window->UserData)->drawinfo->dri_Pens[SHADOWPEN]);
			Move(&rp, object->gl_info.gl_area.area_pos.Left, object->gl_info.gl_area.area_pos.Top + 1);
			Draw(&rp,
				 object->gl_info.gl_area.area_pos.Left + object->gl_info.gl_area.area_pos.Width - 1,
				 object->gl_info.gl_area.area_pos.Top + 1);
		}

		// If we have text, see if we need to position it
		if (txt && !(object->flags & AREAFLAG_TITLE))
		{
			struct TextExtent extent;

			// Get amount we can display
			text_len = TextFit(&rp,
							   txt,
							   text_len,
							   &extent,
							   0,
							   1,
							   object->gl_info.gl_area.area_pos.Width,
							   object->gl_info.gl_area.area_pos.Height);

			if (object->flags & TEXTFLAG_CENTER)
			{
				text_pos.Left = object->gl_info.gl_area.area_pos.Left +
								((object->gl_info.gl_area.area_pos.Width - TextLength(&rp, txt, text_len)) >> 1);
				text_pos.Top = object->gl_info.gl_area.area_pos.Top +
							   ((object->gl_info.gl_area.area_pos.Height - rp.TxHeight) >> 1);
			}
		}

		// Need JAM2 for text if optimised refreshing
		if (optimref)
			SetDrMd(&rp, JAM2);

		// Fall through to display text

	// Text
	case OD_TEXT:

		// If there's any text, we display the string
		if (txt)
		{
			// Get colours
			SetAPen(&rp, fg);
			SetBPen(&rp, bg);

			// Display string
			Move(&rp, text_pos.Left, text_pos.Top + rp.TxBaseline);
			Text(&rp, txt, text_len);

			// Underscores?
			if (object->type == OD_TEXT && object->gl_info.gl_text.uscore_pos > -1)
			{
				short x;

				x = text_pos.Left;
				if (object->gl_info.gl_text.uscore_pos > 0)
					x += TextLength(&rp, txt, object->gl_info.gl_text.uscore_pos);

				Move(&rp, x, text_pos.Top + rp.TxBaseline + 1);
				Draw(&rp,
					 x + TextLength(&rp, txt + object->gl_info.gl_text.uscore_pos, 1) - 1,
					 text_pos.Top + rp.TxBaseline + 1);
			}

			// Get length of text
			if (optimref)
				text_width = rp.cp_x;
		}

		// Otherwise, clear width
		else
			text_width = 0;

		// Optimised refresh?
		if (optimref)
		{
			SetAPen(&rp, bg);

			// No text?
			if (text_width == 0)
			{
				if (bg > 0 || object->flags & AREAFLAG_FILL_COLOUR)
				{
					RectFill(&rp,
							 object->gl_info.gl_area.area_pos.Left + 2,
							 object->gl_info.gl_area.area_pos.Top + 1,
							 object->gl_info.gl_area.area_pos.Left + object->gl_info.gl_area.area_pos.Width - 3,
							 object->gl_info.gl_area.area_pos.Top + object->gl_info.gl_area.area_pos.Height - 2);
				}
				else
				{
					EraseRect(&rp,
							  object->gl_info.gl_area.area_pos.Left + 2,
							  object->gl_info.gl_area.area_pos.Top + 1,
							  object->gl_info.gl_area.area_pos.Left + object->gl_info.gl_area.area_pos.Width - 3,
							  object->gl_info.gl_area.area_pos.Top + object->gl_info.gl_area.area_pos.Height - 2);
				}
			}

			else
			{
				// Refresh to left of text
				if (area_pos.Left + 2 < text_pos.Left)
				{
					if (bg > 0 || object->flags & AREAFLAG_FILL_COLOUR)
					{
						RectFill(&rp,
								 area_pos.Left + 2,
								 area_pos.Top + 1,
								 text_pos.Left - 1,
								 area_pos.Top + area_pos.Height - 2);
					}
					else
					{
						EraseRect(&rp,
								  area_pos.Left + 2,
								  area_pos.Top + 1,
								  text_pos.Left - 1,
								  area_pos.Top + area_pos.Height - 2);
					}
				}

				// Refresh to right of text
				if (text_width < area_pos.Left + area_pos.Width - 3)
				{
					if (bg > 0 || object->flags & AREAFLAG_FILL_COLOUR)
					{
						RectFill(&rp,
								 text_width,
								 area_pos.Top + 1,
								 area_pos.Left + area_pos.Width - 3,
								 area_pos.Top + area_pos.Height - 2);
					}
					else
					{
						EraseRect(&rp,
								  text_width,
								  area_pos.Top + 1,
								  area_pos.Left + area_pos.Width - 3,
								  area_pos.Top + area_pos.Height - 2);
					}
				}
			}
		}
		break;
	}
}

/****************************************************************************
						  Attach a menu to a window
 ****************************************************************************/

void LIBFUNC L_AddWindowMenus(REG(a0, struct Window *window), REG(a1, const MenuData *menudata))
{
	struct TagItem tags[2];

	// Get window data
	WindowData *wdata = (WindowData *)window->UserData;

	// Free any existing menus
	L_FreeWindowMenus(window);

	// Fill out tags
	tags[0].ti_Tag = GTMN_NewLookMenus;
	tags[0].ti_Data = TRUE;
	tags[1].ti_Tag = TAG_END;

	// Create menu strip, layout menus
	if (!(wdata->menu_strip = L_BuildMenuStrip(menudata, wdata->locale)) ||
		!(LayoutMenusA(wdata->menu_strip, wdata->visinfo, tags)))
	{
		L_FreeWindowMenus(window);
		return;
	}

	// Attach menus
	SetMenuStrip(window, wdata->menu_strip);

	// Clear RMBTRAP flag in window
	window->Flags &= ~WFLG_RMBTRAP;
}

/****************************************************************************
						  Free a window's menus
 ****************************************************************************/

void LIBFUNC L_FreeWindowMenus(REG(a0, struct Window *window))
{
	// See if menu strip exists
	if (((WindowData *)window->UserData)->menu_strip)
	{
		ClearMenuStrip(window);
		FreeMenus(((WindowData *)window->UserData)->menu_strip);
		((WindowData *)window->UserData)->menu_strip = 0;
	}

	// Set RMBTRAP flag in window
	window->Flags |= WFLG_RMBTRAP;
}

/****************************************************************************
							  Refresh a list of objects
 ****************************************************************************/

void LIBFUNC L_RefreshObjectList(REG(a0, struct Window *window), REG(a1, ObjectList *ref_list))
{
	ObjectList *list;
	GL_Object *object;

	// If no list supplied, get first list
	if (!ref_list)
		list = (ObjectList *)((WindowData *)window->UserData)->object_list.mlh_Head;
	else
		list = ref_list;

	// Go through lists
	for (; list->node.mln_Succ; list = (ObjectList *)list->node.mln_Succ)
	{
		// Go through objects
		for (object = list->firstobject; object; object = object->next)

			// Refresh the object
			L_DisplayObject(window, object, -1, -1, 0);

		// If one list was supplied, stop at this point
		if (ref_list)
			break;
	}
}

/****************************************************************************
							 Misc. local routines
 ****************************************************************************/

// Add image to a gadget
void _layout_add_gad_image(APTR *memory, struct Gadget *gadget, struct Image *imagedata)
{
	struct Image *image;

	// Allocate Image structure
	if ((image = alloc_object_memory(memory, sizeof(struct Image))))
	{
		// Fill out Image structure
		CopyMem((char *)imagedata, (char *)image, sizeof(struct Image));
		image->LeftEdge = (gadget->Width - image->Width) >> 1;
		image->TopEdge = (gadget->Height - image->Height) >> 1;

		// Add to gadget's list of images
		if (gadget->GadgetRender)
		{
			struct Image *last = (struct Image *)gadget->GadgetRender;
			while (last->NextImage)
				last = last->NextImage;
			last->NextImage = image;
		}
		else
			gadget->GadgetRender = (APTR)image;

		// Set image flags
		gadget->Flags |= GFLG_GADGIMAGE;
	}
}

// Backfill a window
void _req_backfill(struct Window *window)
{
	struct RastPort copy;
	WindowData *data;
	UWORD dither_pattern[2];

	// Copy rastport
	copy = *window->RPort;

	// Get window data
	data = DATA(window);

	// Initialise pattern
	dither_pattern[0] = 0xaaaa;
	dither_pattern[1] = 0x5555;

	// Set pattern and pen
	SetAPen(&copy, data->drawinfo->dri_Pens[SHINEPEN]);
	SetBPen(&copy, data->drawinfo->dri_Pens[BACKGROUNDPEN]);
	SetAfPt(&copy, dither_pattern, 1);
	SetDrMd(&copy, JAM2);

	// Back fill window
	RectFill(&copy,
			 window->BorderLeft,
			 window->BorderTop,
			 window->Width - window->BorderRight - 1,
			 window->Height - window->BorderBottom - 1);
}

// Allocate object memory
APTR alloc_object_memory(APTR *memory, long size)
{
	// No handle?
	if (!*memory && !(*memory = L_NewMemHandle(0, 0, MEMF_CLEAR)))
		return 0;

	// Allocate memory
	return L_AllocMemH(*memory, size);
}
