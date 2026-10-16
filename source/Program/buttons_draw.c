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

void buttons_EraseRect(struct RastPort *, struct Rectangle *, short);

// Draw button border
short buttons_draw_border(Buttons *buttons,
						  Cfg_Button *button,
						  short x,
						  short y,
						  short selected,
						  short do_corners,
						  struct RastPort *rp)
{
	struct Rectangle rect;
	short corners = 0;
	Cfg_Function *function;
	short shine, shadow;
	short draw[22], d;

	// No borders?
	if (buttons->bank->window.flags & BTNWF_BORDERLESS)
		return 0;

	// No corners?
	if (buttons->bank->window.flags & BTNWF_NO_DOGEARS)
		do_corners = 0;

	// Got rastport?
	if (rp)
	{
		// Get button dimensions
		rect.MinX = 0;
		rect.MinY = 0;
		rect.MaxX = buttons->button_width - 1;
		rect.MaxY = buttons->button_height - 1;
	}

	// Draw to button window
	else
	{
		// Get rastport
		rp = buttons->window->RPort;

		// Calculate rectangle
		buttons_get_rect(buttons, x, y, &rect);
	}

	// Allowed to do corners?
	if (do_corners && !(buttons->flags & BUTTONF_TOOLBAR) && button)
	{
		// Go through buttons functions
		for (function = (Cfg_Function *)button->function_list.mlh_Head; function->node.ln_Succ;
			 function = (Cfg_Function *)function->node.ln_Succ)
		{
			// Valid function?
			if (!(IsListEmpty((struct List *)&function->instructions)))
			{
				// Right button function?
				if (function->function.func_type == FTYPE_RIGHT_BUTTON)
					corners |= 1;

				// Middle button function
				else if (function->function.func_type == FTYPE_MID_BUTTON)
					corners |= 2;

				// Special function
				else if (function->function.func_type == FTYPE_LIST)
					corners |= 1;
			}
		}
	}

	// Get shine and shadow pens
	if (selected)
	{
		shine = GUI->draw_info->dri_Pens[SHADOWPEN];
		shadow = GUI->draw_info->dri_Pens[SHINEPEN];
	}
	else
	{
		shine = GUI->draw_info->dri_Pens[SHINEPEN];
		shadow = GUI->draw_info->dri_Pens[SHADOWPEN];
	}

	// Draw left and top line in shine pen
	SetAPen(rp, shine);
	Move(rp, rect.MinX, rect.MaxY - ((corners & 2) ? 5 : 0));
	draw[0] = rect.MinX;
	draw[1] = rect.MinY;
	draw[2] = rect.MaxX - ((corners & 1) ? 4 : 1);
	draw[3] = rect.MinY;
	PolyDraw(rp, 2, draw);

	// Top-right corner?
	if (corners & 1)
	{
		// Draw internal goo
		Move(rp, rect.MaxX - 3, rect.MinY + 1);
		draw[0] = rect.MaxX - 3;
		draw[1] = rect.MinY + 3;
		draw[2] = rect.MaxX - 1;
		draw[3] = rect.MinY + 3;
		draw[4] = rect.MaxX - 2;
		draw[5] = rect.MinY + 2;
		PolyDraw(rp, 3, draw);
	}

	// Draw right and bottom in shadow pen
	SetAPen(rp, shadow);

	// Top-right corner?
	if (corners & 1)
	{
		// Draw more corner goo
		draw[0] = rect.MaxX;
		draw[1] = rect.MinY + 3;
		draw[2] = rect.MaxX - 3;
		draw[3] = rect.MinY;
		draw[4] = rect.MaxX - 4;
		draw[5] = rect.MinY + 1;
		draw[6] = rect.MaxX - 4;
		draw[7] = rect.MinY + 4;
		draw[8] = rect.MaxX;
		draw[9] = rect.MinY + 4;
		d = 10;
	}
	else
	{
		draw[0] = rect.MaxX;
		draw[1] = rect.MinY;
		d = 2;
	}

	// Draw right and bottom side
	draw[d++] = rect.MaxX;
	draw[d++] = rect.MaxY;
	draw[d++] = rect.MinX + ((corners & 2) ? 3 : 1);
	draw[d++] = rect.MaxY;

	// Bottom-left corner?
	if (corners & 2)
	{
		// Draw corner goo
		draw[d++] = rect.MinX;
		draw[d++] = rect.MaxY - 3;
		draw[d++] = rect.MinX;
		draw[d++] = rect.MaxY - 4;
		draw[d++] = rect.MinX + 4;
		draw[d++] = rect.MaxY - 4;
		draw[d++] = rect.MinX + 4;
		draw[d++] = rect.MaxY;
	}

	// Draw all that (phew)
	Move(rp, draw[0], draw[1]);
	PolyDraw(rp, (d >> 1) - 1, &draw[2]);

	// More bottom-left corner
	if (corners & 2)
	{
		// Draw internal goo
		SetAPen(rp, shine);
		Move(rp, rect.MinX + 1, rect.MaxY - 3);
		draw[0] = rect.MinX + 3;
		draw[1] = rect.MaxY - 3;
		draw[2] = rect.MinX + 3;
		draw[3] = rect.MaxY - 1;
		draw[4] = rect.MinX + 2;
		draw[5] = rect.MaxY - 2;
		PolyDraw(rp, 3, draw);
	}

	// Clear under corners
	if (corners)
	{
		SetAPen(rp, GUI->draw_info->dri_Pens[BACKGROUNDPEN]);

		// Top-right
		if (corners & 1)
		{
			Move(rp, rect.MaxX - 2, rect.MinY);
			draw[0] = rect.MaxX;
			draw[1] = rect.MinY;
			draw[2] = rect.MaxX;
			draw[3] = rect.MinY + 2;
			draw[4] = rect.MaxX - 1;
			draw[5] = rect.MinY + 1;
			PolyDraw(rp, 3, draw);
		}

		// Bottom-left
		if (corners & 2)
		{
			Move(rp, rect.MinX, rect.MaxY - 2);
			draw[0] = rect.MinX + 2;
			draw[1] = rect.MaxY;
			draw[2] = rect.MinX + 1;
			draw[3] = rect.MaxY;
			draw[4] = rect.MinX + 1;
			draw[5] = rect.MaxY - 1;
			PolyDraw(rp, 3, draw);
		}
	}

	return 1;
}

// Calculate a button rectangle
void buttons_get_rect(Buttons *buttons, int x, int y, struct Rectangle *rect)
{
	rect->MinX = buttons->internal.Left + x * buttons->button_width;
	rect->MinY = buttons->internal.Top + y * buttons->button_height;
	rect->MaxX = rect->MinX + buttons->button_width - 1;
	rect->MaxY = rect->MinY + buttons->button_height - 1;
}

// Redraw a single button
void buttons_redraw_button(Buttons *buttons, Cfg_Button *button)
{
	short x, y;

	// Check button is valid
	if (!button || !button->node.ln_Succ)
		return;

	// Get button position
	if (!(buttons_pos(buttons, button, &x, &y)))
		return;

	// Subtract offsets
	x -= buttons->button_left;
	y -= buttons->button_top;

	// Check it's visible
	if (x >= 0 && x < buttons->button_cols && y >= 0 && y < buttons->button_rows)
	{
		// Show button
		buttons_show_button(buttons, button, x, y, button->current, 0);
	}
}

// Show a button
void buttons_show_button(Buttons *buttons, Cfg_Button *button, short x, short y, short which, struct RastPort *rp)
{
	// Draw button
	buttons_draw_button(buttons, button, x, y, which, 0, rp);

	// Draw border
	buttons_draw_border(buttons, button, x, y, 0, 1, rp);
}

// Draw a button
short buttons_draw_button(Buttons *buttons,
						  Cfg_Button *button,
						  short x,
						  short y,
						  short image,
						  short state,
						  struct RastPort *rp)
{
	UBYTE fg, bg;
	BOOL valid = 0;
	struct Rectangle rect;
	WORD width, height;
	APTR image_ptr = 0;
	Cfg_ButtonFunction *func = 0;

	// Check if button is valid
	if (button && button->node.ln_Succ)
		valid = 1;

	// Get function
	if (valid)
		func = button_find_function(button, image, &image_ptr);

	// Get pens we need
	if (valid)
	{
		fg = button->button.fpen;
		bg = button->button.bpen;
		if (fg >= 4 && fg < 252)
			fg = GUI->pens[fg - 4];
		if (bg >= 4 && bg < 252)
			bg = GUI->pens[bg - 4];
	}
	else
	{
		fg = GUI->draw_info->dri_Pens[TEXTPEN];
		bg = GUI->draw_info->dri_Pens[BACKGROUNDPEN];
	}

	// Rastport supplied?
	if (rp)
	{
		// Get button dimensions
		rect.MinX = 0;
		rect.MinY = 0;
		rect.MaxX = buttons->button_width - 1;
		rect.MaxY = buttons->button_height - 1;
	}

	// Draw to button window
	else
	{
		// Get rastport
		rp = buttons->window->RPort;

		// Calculate rectangle
		buttons_get_rect(buttons, x, y, &rect);
	}

	// Need space for border?
	if (!(buttons->bank->window.flags & BTNWF_BORDERLESS) && !(buttons->flags & BUTTONF_TOOLBAR))
	{
		// Move rectangle in
		rect.MinX++;
		rect.MinY++;
		rect.MaxX--;
		rect.MaxY--;
	}

	// Get rectangle size
	width = RECTWIDTH(&rect);
	height = RECTHEIGHT(&rect);

	// Return if not valid
	if (!valid || !func)
	{
		// Erase background
		buttons_EraseRect(rp, &rect, bg);
		return 0;
	}

	// Graphical button?
	if (button->button.flags & BUTNF_GRAPHIC)
	{
		// Valid image?
		if (image_ptr)
		{
			struct TagItem draw_tags[8];

			// Fill out draw tags
			draw_tags[0].ti_Tag = IM_Rectangle;
			draw_tags[0].ti_Data = (IPTR)&rect;

			// Space to clip inside border
			draw_tags[1].ti_Tag = IM_ClipBoundary;
			if (buttons->bank->window.flags & BTNWF_BORDERLESS)
				draw_tags[1].ti_Data = 0;
			else if (buttons->flags & BUTTONF_TOOLBAR)
				draw_tags[1].ti_Data = 2;
			else
				draw_tags[1].ti_Data = 4;

			// More tags...
			draw_tags[2].ti_Tag = IM_Mask;
			draw_tags[2].ti_Data = (buttons->pattern_data.valid || bg != GUI->draw_info->dri_Pens[BACKGROUNDPEN]);
			draw_tags[3].ti_Tag = IM_State;
			draw_tags[3].ti_Data = state;
			draw_tags[4].ti_Tag = IM_Erase;
			draw_tags[4].ti_Data = bg;
			draw_tags[5].ti_Tag = IM_NoIconRemap;
			draw_tags[5].ti_Data = !remap_icon_images();
			draw_tags[6].ti_Tag = IM_Backfill;
			draw_tags[6].ti_Data = (buttons->pattern_data.valid && bg == GUI->draw_info->dri_Pens[BACKGROUNDPEN]);
			draw_tags[7].ti_Tag = TAG_END;

			// Draw image
			return RenderImage(rp, image_ptr, 0, 0, draw_tags);
		}

		// Draw unknown image
		else
		{
			// Erase background
			buttons_EraseRect(rp, &rect, bg);

			return 0;
		}
	}

	// Text button
	else if (func->label)
	{
		char *text;
		struct TextExtent extent;
		int len;

		// Get text pointer
		text = func->label;

		// Calculate length that will fit in button
		len = TextFit(rp, text, strlen(text), &extent, 0, 1, width, height);

		// Erase background
		buttons_EraseRect(rp, &rect, bg);

		// Render text
		if (len > 0)
		{
			SetAPen(rp, fg);
			Move(rp,
				 rect.MinX + ((width - extent.te_Width) >> 1),
				 rect.MinY + ((height - extent.te_Height) >> 1) + rp->Font->tf_Baseline);
			Text(rp, text, len);
		}
	}

	return (short)!state;
}

// Find a button from coordinates
Cfg_Button *button_from_point(Buttons *buttons, WORD *x, WORD *y)
{
	short row, col;

	// Calculate button column/row
	col = (*x - buttons->internal.Left) / buttons->button_width;
	row = (*y - buttons->internal.Top) / buttons->button_height;

	// Store column/row in coordinates
	*x = col;
	*y = row;

	// Offset by bank top/left positions
	col += buttons->button_left;
	row += buttons->button_top;

	return button_from_pos(buttons, col, row);
}

// Find a button from column/row
Cfg_Button *button_from_pos(Buttons *buttons, short col, short row)
{
	Cfg_Button *button;
	short num;

	// Valid coords?
	if (col < 0 || row < 0 || col >= buttons->bank->window.columns || row >= buttons->bank->window.rows)
		return 0;

	// Get button this refers to
	col += row * buttons->bank->window.columns;
	for (num = 0, button = (Cfg_Button *)buttons->bank->buttons.lh_Head; num < col && button->node.ln_Succ;
		 num++, button = (Cfg_Button *)button->node.ln_Succ)
		;

	// Valid button?
	if (button->node.ln_Succ)
		return button;
	return 0;
}

// Find the position of a button
BOOL buttons_pos(Buttons *buttons, Cfg_Button *button, short *col, short *row)
{
	short x, y;
	Cfg_Button *first;

	// Get first button
	first = (Cfg_Button *)buttons->bank->buttons.lh_Head;

	// Go through rows and columns
	for (y = 0; y < buttons->bank->window.rows; y++)
	{
		for (x = 0; x < buttons->bank->window.columns; x++)
		{
			// Is this the button we want?
			if (button == first)
			{
				*col = x;
				*row = y;
				return 1;
			}

			// Get next button
			if (!first->node.ln_Succ)
				return 0;
			first = (Cfg_Button *)first->node.ln_Succ;
		}
	}

	return 0;
}

// See if a button has a valid function
Cfg_Function *button_valid(Cfg_Button *button, short which)
{
	Cfg_Function *function;

	// Valid button?
	if (!button)
		return 0;

	// Get function
	if (!(function = (Cfg_Function *)button_find_function(button, which, 0)))
		return 0;

	// Return function if it's valid
	if (!(IsListEmpty((struct List *)&function->instructions)))
		return function;
	return 0;
}

// Show button highlights
void buttons_show_highlight(Buttons *buttons)
{
	short x, y;
	Cfg_Button *button;

	// Check valid coords
	if (buttons->editor_sel_row == -1 || buttons->editor_sel_col == -1)
		return;

	// Get button
	if (!(button = button_from_pos(buttons, buttons->editor_sel_col, buttons->editor_sel_row)))
		return;

	// Calculate column/row to highlight
	x = buttons->editor_sel_col - buttons->button_left;
	y = buttons->editor_sel_row - buttons->button_top;

	// See if button is visible
	if (x >= 0 && x < buttons->button_cols && y >= 0 && y < buttons->button_rows)
	{
		// Show normal button?
		if (buttons->flags & BUTTONF_HIGH_SHOWN)
		{
			buttons_show_button(buttons, button, x, y, button->current, 0);
		}

		// Show highlighted button
		else
		{
			unsigned short dither[2];
			short pen;

			// Initialise dither pattern
			dither[0] = 0xaaaa;
			dither[1] = 0x5555;

			// Default to shadow pen
			pen = buttons->drawinfo->dri_Pens[SHADOWPEN];

			// Text gadget?
			if (!(button->button.flags & BUTNF_GRAPHIC))
			{
				short bg;

				// Get button background colour
				bg = button->button.bpen;
				if (bg >= 4 && bg < 252)
					bg = GUI->pens[bg - 4];

				// Is it black?
				if (GetRGB4(buttons->window->WScreen->ViewPort.ColorMap, bg) == 0)
				{
					// Use shine pen
					pen = buttons->drawinfo->dri_Pens[SHINEPEN];
				}
			}

			// Set fill pattern and pen
			SetAfPt(buttons->window->RPort, dither, 1);
			SetAPen(buttons->window->RPort, pen);
			SetDrMd(buttons->window->RPort, JAM1);

			// Get button coordinates
			x = buttons->internal.Left + x * buttons->button_width;
			y = buttons->internal.Top + y * buttons->button_height;

			// Shade button
			RectFill(buttons->window->RPort, x, y, x + buttons->button_width - 1, y + buttons->button_height - 1);

			// Clear fill pattern
			SetAfPt(buttons->window->RPort, 0, 0);
		}

		// Toggle highlight flag
		buttons->flags ^= BUTTONF_HIGH_SHOWN;
	}
}

// Stop highlight flashing
void buttons_stop_highlight(Buttons *buttons)
{
	// Turn highlight off
	if (buttons->flags & BUTTONF_HIGH_SHOWN)
	{
		buttons_show_highlight(buttons);
	}

	// Disable flashing
	buttons->flags |= BUTTONF_NO_FLASH;
}

// Rectangle
void buttons_EraseRect(struct RastPort *rp, struct Rectangle *rect, short bg)
{
	if (bg == GUI->draw_info->dri_Pens[BACKGROUNDPEN])
		EraseRect(rp, rect->MinX, rect->MinY, rect->MaxX, rect->MaxY);
	else
	{
		SetAPen(rp, bg);
		RectFill(rp, rect->MinX, rect->MinY, rect->MaxX, rect->MaxY);
	}
}
