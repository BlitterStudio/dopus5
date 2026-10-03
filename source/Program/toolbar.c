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

// Get a toolbar
ToolBarInfo *OpenToolBar(Cfg_ButtonBank *buttons, char *pathname)
{
	ToolBarInfo *toolbar;

	// Allocate toolbar structure
	if ((toolbar = AllocVec(sizeof(ToolBarInfo), MEMF_CLEAR)))
	{
		BOOL cache_ok = FALSE;

		// Bank provided?
		if (buttons)
			toolbar->buttons = buttons;

		// Load disk file
		else if (pathname && pathname[0])
		{
			toolbar->buttons = OpenButtonBank(pathname);
		}

		if (toolbar->buttons)
			cache_ok = GetToolBarCache(toolbar, FALSE);

		// Invalid?
		if (!toolbar->buttons || !cache_ok)
		{
			FreeToolBar(toolbar);
			return 0;
		}
	}

	// Bank provided? Copy pathname
	if (buttons && pathname)
		strcpy(pathname, buttons->path);

	return toolbar;
}

// Free a toolbar
void FreeToolBar(ToolBarInfo *toolbar)
{
	// Valid toolbar?
	if (toolbar)
	{
		// Free cache
		FreeToolBarCache(toolbar);

		// Free buttons
		CloseButtonBank(toolbar->buttons);

		// Free position array
		FreeVec(toolbar->button_array);

		// Free data
		FreeVec(toolbar);
	}
}

// Update toolbar cache
BOOL GetToolBarCache(ToolBarInfo *toolbar, BOOL real)
{
	short depth, width, height, num, x, y;
	short last_width = 0, last_height = 0;
	Cfg_Button *button;
	struct TagItem tags[4];
	struct Rectangle rect;

	// Invalid toolbar?
	if (!toolbar)
		return 0;

	// Free existing cache
	FreeToolBarCache(toolbar);

	// Free existing position array
	if (toolbar->button_array)
	{
		FreeVec(toolbar->button_array);
		toolbar->button_array = 0;
	}

	// Remap toolbar if this is for real
	if (real && !toolbar->done_remap)
	{
		// Do remap
		RemapToolBar(toolbar);
		toolbar->done_remap = 1;
	}

	// Initialise tags
	tags[0].ti_Tag = IM_Depth;
	tags[0].ti_Data = 1;
	tags[1].ti_Tag = IM_Width;
	tags[1].ti_Data = 0;
	tags[2].ti_Tag = IM_Height;
	tags[2].ti_Data = 0;
	tags[3].ti_Tag = TAG_DONE;

	// Minimum depth
	depth = 1;

	// Count items in toolbar
	for (button = (Cfg_Button *)toolbar->buttons->buttons.lh_Head, toolbar->count = 0; button->node.ln_Succ;
		 button = (Cfg_Button *)button->node.ln_Succ, toolbar->count++)
		;

	// Allocate position array
	if (!(toolbar->button_array = AllocVec(sizeof(struct Rectangle) * (toolbar->count + 2), MEMF_CLEAR)))
	{
		FreeToolBarCache(toolbar);
		return 0;
	}

	// Go through buttons again
	for (button = (Cfg_Button *)toolbar->buttons->buttons.lh_Head, num = 0, toolbar->max_width = 0;
		 button->node.ln_Succ;
		 button = (Cfg_Button *)button->node.ln_Succ, num++)
	{
		Cfg_ButtonFunction *func;
		short width, height, x;

		// Get left button image
		if ((func = (Cfg_ButtonFunction *)FindFunctionType((struct List *)&button->function_list, FTYPE_LEFT_BUTTON)) &&
			func->image)
		{
			// Get depth and other info
			GetImageAttrs(func->image, tags);

			// Biggest depth so far?
			if (tags[0].ti_Data > depth)
				depth = tags[0].ti_Data;

			// Get size
			width = tags[1].ti_Data;
			height = tags[2].ti_Data;
		}

		// Or textual button?
		else if (!(button->button.flags & BUTNF_GRAPHIC) && func)
		{
			struct TextExtent extent;

			// Get length of label
			TextExtent(&GUI->screen_pointer->RastPort, func->label, strlen(func->label), &extent);

			// Get size
			width = extent.te_Width;
			height = extent.te_Height;
		}

		// Use last size
		else
		{
			width = last_width;
			height = last_height;
		}

		// Minimum size
		if (width < 2)
			width = 8;
		if (height < 2)
			height = 8;

		// Save size
		last_width = width;
		last_height = height;

		// Increase size for border
		if (!(toolbar->buttons->window.flags & BTNWF_BORDERLESS))
		{
			width += 2;
			height += 2;
		}

		// Biggest width so far?
		if (width > toolbar->max_width)
			toolbar->max_width = width;

		// Get last position
		x = (num > 0) ? toolbar->button_array[num - 1].MaxX + 1 : 0;

		// Store position in array
		toolbar->button_array[num].MinX = x;
		toolbar->button_array[num].MinY = 0;
		toolbar->button_array[num].MaxX = x + width - 1;
		toolbar->button_array[num].MaxY = height - 1;
	}

	// No actual buttons?
	if (toolbar->count < 1 || depth < 1)
	{
		FreeToolBarCache(toolbar);
		return 0;
	}

	// Store rows/cols
	toolbar->cols = toolbar->count;
	toolbar->rows = 1;

	// Valid toolbar arrow?
	if (GUI->toolbar_arrow_image)
	{
		short width, height;

		// Get size of the arrow button
		GetImageAttrs(GUI->toolbar_arrow_image, tags);

		// Biggest depth so far?
		if (tags[0].ti_Data > depth)
			depth = tags[0].ti_Data;

		// Get size
		width = tags[1].ti_Data;
		height = tags[2].ti_Data;

		// Increase size for border
		if (!(toolbar->buttons->window.flags & BTNWF_BORDERLESS))
		{
			width += 2;
			height += 2;
		}

		// Store size in array
		toolbar->button_array[toolbar->count].MaxX = width - 1;
		toolbar->button_array[toolbar->count].MaxY = height - 1;

		// Add to maximum width
		toolbar->max_width += width;
	}

	// Get height of toolbar from the buttons alone; the scroll arrow is only
	// shown on overflow and must not force a taller row than the buttons need
	for (num = 0, height = 0; num < toolbar->count; num++)
	{
		if ((width = RECTHEIGHT(&toolbar->button_array[num])) > height)
			height = width;
	}

	// Fit the arrow to the button row; RenderImage crops it to the rectangle
	if (toolbar->button_array[toolbar->count].MaxY > height - 1)
		toolbar->button_array[toolbar->count].MaxY = height - 1;

	// Store toolbar height
	toolbar->button_height = height;

	// Get cache size
	toolbar->width = toolbar->button_array[toolbar->count - 1].MaxX + 1;
	toolbar->height = toolbar->button_height;

	// Don't want cache?
	if (!real)
		return 1;

	// Allocate cache bitmap
	if (!(toolbar->bitmap =
			  NewBitMap(toolbar->width, toolbar->height, depth, BMF_CLEAR, GUI->screen_pointer->RastPort.BitMap)))
	//			0)))
	{
		// Failed
		FreeToolBarCache(toolbar);
		return 0;
	}

	// Initialise RastPort
	InitRastPort(&toolbar->rp);
	toolbar->rp.BitMap = toolbar->bitmap;

	// Got cache successfully
	toolbar->cache = 1;

#ifdef __AROS__
	// AROS can have pen 0 as black; initialise the off-screen cache
	// with the screen background pen before images and button frames are drawn.
	if (GUI->draw_info && toolbar->width > 0 && toolbar->height > 0)
	{
		SetAPen(&toolbar->rp, GUI->draw_info->dri_Pens[BACKGROUNDPEN]);
		RectFill(&toolbar->rp, 0, 0, toolbar->width - 1, toolbar->height - 1);
	}
#endif

	// Set pens and font
#ifdef __AROS__
	if (GUI->draw_info)
	{
		SetAPen(&toolbar->rp, GUI->draw_info->dri_Pens[TEXTPEN]);
		SetBPen(&toolbar->rp, GUI->draw_info->dri_Pens[BACKGROUNDPEN]);
	}
	else
	{
		SetAPen(&toolbar->rp, 1);
		SetBPen(&toolbar->rp, 0);
	}
#else
	SetAPen(&toolbar->rp, 1);
	SetBPen(&toolbar->rp, 0);
#endif
	SetFont(&toolbar->rp, GUI->screen_pointer->RastPort.Font);

	// Initialise draw tags
	tags[0].ti_Tag = IM_Rectangle;
	tags[0].ti_Data = (IPTR)&rect;
	tags[1].ti_Tag = IM_ClipBoundary;
	tags[1].ti_Data = (toolbar->buttons->window.flags & BTNWF_BORDERLESS) ? 0 : 2;
	tags[2].ti_Tag = IM_NoIconRemap;
	tags[2].ti_Data = (environment->env->desktop_flags & DESKTOPF_NO_REMAP) ? TRUE : FALSE;
	tags[3].ti_Tag = TAG_DONE;

	// Go through buttons
	for (button = (Cfg_Button *)toolbar->buttons->buttons.lh_Head, x = 0, y = 0, num = 0; button->node.ln_Succ;
		 button = (Cfg_Button *)button->node.ln_Succ, num++)
	{
		Cfg_ButtonFunction *func;
		BOOL ok = 0;

		// Get button rectangle
		rect = toolbar->button_array[num];

		// Get left button image
		if ((func = (Cfg_ButtonFunction *)FindFunctionType((struct List *)&button->function_list, FTYPE_LEFT_BUTTON)) &&
			func->image)
		{
			// Draw button image
			RenderImage(&toolbar->rp, func->image, 0, 0, tags);
			ok = 1;
		}

		// Or textual button?
		else if (!(button->button.flags & BUTNF_GRAPHIC) && func && func->label && *func->label)
		{
			// Draw label
			Move(&toolbar->rp, x, y + toolbar->rp.TxBaseline);
			Text(&toolbar->rp, func->label, strlen(func->label));
			ok = 1;
		}

		// Draw button border
		if (!ok || !(toolbar->buttons->window.flags & BTNWF_BORDERLESS))
			DrawBox(&toolbar->rp, &rect, GUI->draw_info, 0);
	}

	return 1;
}

// Free cache stuff
void FreeToolBarCache(ToolBarInfo *toolbar)
{
	// Valid toolbar?
	if (toolbar)
	{
		// Free bitmap
		DisposeBitMap(toolbar->bitmap);
		toolbar->bitmap = 0;

		// Clear cache flag
		toolbar->cache = 0;

		// Free remapped images
		if (toolbar->done_remap)
		{
			FreeToolBarRemap(toolbar);
			toolbar->done_remap = 0;
		}

		// Clear height
		//		toolbar->height=0;
	}
}

// Remap toolbar images
void RemapToolBar(ToolBarInfo *toolbar)
{
	Cfg_Button *button;
	Cfg_ButtonFunction *func;

	// Valid buttons?
	if (!toolbar || !toolbar->buttons)
		return;

	// Go through buttons
	for (button = (Cfg_Button *)toolbar->buttons->buttons.lh_Head; button->node.ln_Succ;
		 button = (Cfg_Button *)button->node.ln_Succ)
	{
		// Go through functions
		for (func = (Cfg_ButtonFunction *)button->function_list.mlh_Head; func->node.ln_Succ;
			 func = (Cfg_ButtonFunction *)func->node.ln_Succ)
		{
			// Got an image?
			if (func->image)
			{
				// Remap the image
				RemapImage(func->image, GUI->screen_pointer, &toolbar->remap);
			}
		}
	}
}

// Free remapped toolbar images
void FreeToolBarRemap(ToolBarInfo *toolbar)
{
	Cfg_Button *button;
	Cfg_ButtonFunction *func;

	// Valid buttons?
	if (!toolbar || !toolbar->buttons)
		return;

	// Go through buttons
	for (button = (Cfg_Button *)toolbar->buttons->buttons.lh_Head; button->node.ln_Succ;
		 button = (Cfg_Button *)button->node.ln_Succ)
	{
		// Go through functions
		for (func = (Cfg_ButtonFunction *)button->function_list.mlh_Head; func->node.ln_Succ;
			 func = (Cfg_ButtonFunction *)func->node.ln_Succ)
		{
			// Got an image?
			if (func->image)
			{
				// Free image remapping
				FreeRemapImage(func->image, &toolbar->remap);
			}
		}
	}

	// Free pens
	FreeImageRemap(&toolbar->remap);
}
