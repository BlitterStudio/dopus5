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

// Refresh buttons display
void buttons_refresh(Buttons *buttons, ULONG type)
{
	int x, y;
	Cfg_Button *button;

	// Lock bank
	GetSemaphore(&buttons->bank->lock, SEMF_SHARED, 0);

	// Make selector visible
	if (type & BUTREFRESH_SELECTOR)
	{
		if (buttons_visible_select(buttons))
			type |= BUTREFRESH_REFRESH;
	}

	// Font reset
	if (type & BUTREFRESH_FONT)
	{
		// See if font has changed
		if ((strcmp(buttons->window->RPort->Font->tf_Message.mn_Node.ln_Name, buttons->bank->window.font_name)) != 0 ||
			buttons->window->RPort->Font->tf_YSize != buttons->bank->window.font_size)
		{
			// Get new font
			buttons_get_font(buttons);

			// Do full reset
			type = BUTREFRESH_RESIZE | BUTREFRESH_REFRESH;

			// Clear resized flag
			buttons->flags &= ~BUTTONF_RESIZED;
		}
	}

	// Resize event?
	if (type & BUTREFRESH_RESIZE)
	{
		unsigned short min_width, min_height;
		unsigned short max_width, max_height;
		short border_x = 0, border_y = 0;

		// Borderless window?
		if (buttons->flags & BUTTONF_BORDERLESS)
		{
			ULONG flags;
			short old_width, old_height;
			ULONG old_border;

			// Save drag gadget flags and size
			flags = buttons->drag_gadget.Flags;
			old_width = buttons->drag_gadget.Width;
			old_height = buttons->drag_gadget.Height;
			old_border = buttons->border_type;

			// See if drag bar orientation has changed
			if (buttons_fix_drag(buttons))
			{
				short width, height;

				// Get current width/height
				width = buttons->window->Width;
				height = buttons->window->Height;

				// If bar is now horizontal, it used to be vertical
				if (buttons->drag_gadget.Flags & GFLG_RELWIDTH && flags & GFLG_RELHEIGHT)
				{
					// Adjust accordingly
					width -= old_width;
					height += buttons->drag_gadget.Height;
				}

				// And vice versa
				else if (buttons->drag_gadget.Flags & GFLG_RELHEIGHT && flags & GFLG_RELWIDTH)
				{
					// Adjust
					height -= old_height;
					width += buttons->drag_gadget.Width;
				}

				// No drag bar
				else if (!(buttons->drag_gadget.Flags & (GFLG_RELWIDTH | GFLG_RELHEIGHT)))
				{
					// Adjust size
					if (flags & GFLG_RELHEIGHT)
					{
						if (old_border & BTNWF_RIGHT_BELOW)
							border_x = old_width;
						width -= old_width;
					}
					else if (flags & GFLG_RELWIDTH)
					{
						if (old_border & BTNWF_RIGHT_BELOW)
							border_y = old_height;
						height -= old_height;
					}
				}

				// State of right/below flag changed
				else if (!(old_border & BTNWF_RIGHT_BELOW) && (buttons->bank->window.flags & BTNWF_RIGHT_BELOW))
				{
					// Move window up or right depending on border
					if (buttons->bank->window.flags & BTNWF_HORIZ)
						border_y = -((GUI->screen_info & SCRI_LORES) ? DRAG_LO_HEIGHT : DRAG_HI_HEIGHT);
					else if (buttons->bank->window.flags & BTNWF_VERT)
						border_x = -DRAG_WIDTH;
				}

				// Is size not changing?
				if (buttons->window->Width == width && buttons->window->Height == height)
				{
					// Refresh buttons display
					type |= BUTREFRESH_REFRESH;
					RefreshGList(&buttons->drag_gadget, buttons->window, 0, 1);
				}

				// Size is changing so window will be refreshed automatically
				else
				{
					// Resize window
					ChangeWindowBox(buttons->window,
									buttons->window->LeftEdge + border_x,
									buttons->window->TopEdge + border_y,
									width,
									height);
					buttons->flags |= BUTTONF_RESIZED;
				}

				// Recalculate internal size
				buttons_fix_internal(buttons);
			}

			// Refresh drag gadget
			else
			{
				RefreshGList(&buttons->drag_gadget, buttons->window, 0, 1);
			}
		}

		// Get the maximum size of a button
		buttons_get_max_size(buttons, &max_width, &max_height);

		// Graphical buttons?
		if (buttons->bank->window.flags & BTNWF_GFX)
		{
			// Maximum is also minimum
			min_width = max_width;
		}

		// Text buttons
		else
		{
			// Get minimum width
			min_width = TextLength(buttons->window->RPort, "a", 1) * 6;

			// Border?
			if (!(buttons->flags & BUTTONF_BORDERLESS))
			{
				// Calculate button size from window
				max_width = buttons->window->GZZWidth / buttons->bank->window.columns;
			}

			// Is button width less than minimum?
			if (max_width < min_width)
				max_width = min_width;
		}

		// Store button size
		buttons->button_width = max_width;
		buttons->button_height = max_height;

		// No border or toolbar?
		if (buttons->flags & (BUTTONF_BORDERLESS | BUTTONF_TOOLBAR))
		{
			short side_borders, top_borders;

			// Start with maximum columns/rows
			buttons->button_cols = buttons->bank->window.columns;
			buttons->button_rows = buttons->bank->window.rows;

			// Calculate window size
			max_width = buttons->button_width * buttons->button_cols;
			max_height = buttons->button_height * buttons->button_rows;

			// Calculate border sizes
			side_borders = buttons->window->Width - buttons->internal.Width;
			top_borders = buttons->window->Height - buttons->internal.Height;

			// Too big for screen?
			if (max_width + side_borders > buttons->window->WScreen->Width)
			{
				buttons->button_cols = (buttons->window->WScreen->Width - side_borders) / buttons->button_width;
				max_width = buttons->button_cols * buttons->button_width;
			}
			if (max_height + top_borders > buttons->window->WScreen->Height)
			{
				buttons->button_rows = (buttons->window->WScreen->Height - top_borders) / buttons->button_height;
				max_height = buttons->button_rows * buttons->button_height;
			}

			// Add border size
			max_width += side_borders;
			max_height += top_borders;

			// Minimum is the maximum
			min_width = max_width;
			min_height = max_height;
		}

		// Normal borders
		else
		{
			// Get columns/rows
			buttons->button_cols = buttons->window->GZZWidth / buttons->button_width;
			buttons->button_rows = buttons->window->GZZHeight / buttons->button_height;

			// Check columns/rows don't exceed actual number of buttons
			if (buttons->button_cols > buttons->bank->window.columns)
				buttons->button_cols = buttons->bank->window.columns;
			if (buttons->button_rows > buttons->bank->window.rows)
				buttons->button_rows = buttons->bank->window.rows;

			// Get window minimum size
			min_width += buttons->window->BorderLeft + buttons->window->BorderRight;
			min_height = buttons->window->BorderTop + buttons->window->BorderBottom + buttons->button_height;

			// Get window maximum size
			max_width = (!(buttons->bank->window.flags & BTNWF_GFX))
							? buttons->window->WScreen->Width
							: buttons->window->BorderLeft + buttons->window->BorderRight +
								  buttons->bank->window.columns * buttons->button_width;
			max_height = buttons->window->BorderTop + buttons->window->BorderBottom +
						 buttons->button_height * buttons->bank->window.rows;
		}

		// Set window limits
		WindowLimits(buttons->window, 0, 0, (ULONG)-1, (ULONG)-1);

		// See if window size can be changed
		if (!(buttons->flags & BUTTONF_RESIZED))
		{
			short width, height;

			// No border?
			if (buttons->flags & BUTTONF_BORDERLESS)
			{
				// Get desired size
				width = max_width;
				height = max_height;
			}

			// Normal borders
			else
			{
				// Get initial size
				width = buttons->window->Width;
				height = buttons->window->Height;

				// Is window narrower than minimum?
				if (buttons->window->Width < min_width)
					width = min_width;

				// Or wider than maximum?
				else if (buttons->window->Width > max_width)
					width = max_width;

				// Otherwise, size to button boundary
				else
				{
					// Drop back to last whole column
					width -= buttons->window->GZZWidth - (buttons->button_cols * buttons->button_width);

					// Size to make window bigger?
					if (buttons->window->Width > buttons->last_position.Width &&
						buttons->button_cols < buttons->bank->window.columns)
					{
						short new_width;

						// Get new width
						new_width = width + buttons->button_width;

						// Would this be legal?
						if (buttons->window->LeftEdge + new_width <= buttons->window->WScreen->Width)
							width = new_width;
					}
				}

				// Is window shorter than minimum?
				if (buttons->window->Height < min_height)
					height = min_height;

				// Or taller than maximum?
				else if (buttons->window->Height > max_height)
					height = max_height;

				// Otherwise, size to button boundary
				else
				{
					// Drop back to last whole row
					height -= buttons->window->GZZHeight - (buttons->button_rows * buttons->button_height);

					// Size to make window bigger?
					if (buttons->window->Height > buttons->last_position.Height &&
						buttons->button_rows < buttons->bank->window.rows)
					{
						short new_height;

						// Get new height
						new_height = height + buttons->button_height;

						// Would this be legal?
						if (buttons->window->TopEdge + new_height <= buttons->window->WScreen->Height)
							height = new_height;
					}
				}
			}

			// Does window need to be resized?
			if (buttons->window->Width != width || buttons->window->Height != height)
			{
				// Resize window
				ChangeWindowBox(buttons->window,
								buttons->window->LeftEdge + border_x,
								buttons->window->TopEdge + border_y,
								width,
								height);
				buttons->flags |= BUTTONF_RESIZED;

				// Set window limits
				WindowLimits(buttons->window, min_width, min_height, max_width, max_height);

				// Unlock bank
				FreeSemaphore(&buttons->bank->lock);
				return;
			}
		}

		// Returned from a resize; refresh frame
		else
			RefreshWindowFrame(buttons->window);

		// Save window position
		buttons->last_position = *((struct IBox *)&buttons->window->LeftEdge);

		// Set window limits
		WindowLimits(buttons->window, min_width, min_height, max_width, max_height);

		// Fix sliders
		if (buttons->vert_scroll)
		{
			SetGadgetAttrs(buttons->vert_scroll,
						   buttons->window,
						   0,
						   PGA_Total,
						   buttons->bank->window.rows,
						   PGA_Visible,
						   buttons->button_rows,
						   TAG_END);
		}
		if (buttons->horiz_scroll)
		{
			SetGadgetAttrs(buttons->horiz_scroll,
						   buttons->window,
						   0,
						   PGA_Total,
						   buttons->bank->window.columns,
						   PGA_Visible,
						   buttons->button_cols,
						   TAG_END);
		}

		// Make sure selector is visible
		if (buttons->editor)
			buttons_visible_select(buttons);
	}

	// Refresh event
	if (type & BUTREFRESH_REFRESH)
	{
		IPTR pos;

		// Get vertical position (PGA_Top stores an IPTR; button_top is an int)
		pos = 0;
		if (buttons->vert_scroll)
			GetAttr(PGA_Top, buttons->vert_scroll, &pos);
		buttons->button_top = pos;

		// Horizontal
		pos = 0;
		if (buttons->horiz_scroll)
			GetAttr(PGA_Top, buttons->horiz_scroll, &pos);
		buttons->button_left = pos;

		// Draw buttons
		button = (Cfg_Button *)buttons->bank->buttons.lh_Head;
		for (y = 0; y < buttons->bank->window.rows; y++)
		{
			for (x = 0; x < buttons->bank->window.columns; x++)
			{
				// Check button is valid
				if (button && !button->node.ln_Succ)
					button = 0;

				// Is button visible?
				if (y >= buttons->button_top && y < buttons->button_top + buttons->button_rows &&
					x >= buttons->button_left && x < buttons->button_left + buttons->button_cols)
				{
					// Draw button
					buttons_show_button(buttons,
										button,
										x - buttons->button_left,
										y - buttons->button_top,
										(button) ? button->current : 0,
										0);
				}

				// Get next button
				if (button)
					button = (Cfg_Button *)button->node.ln_Succ;
			}
		}
	}

	// Clear resized flag
	buttons->flags &= ~BUTTONF_RESIZED;

	// Unlock bank
	FreeSemaphore(&buttons->bank->lock);
}

// Get the maximum size of a button in the bank
short buttons_get_max_size(Buttons *buttons, unsigned short *max_width, unsigned short *max_height)
{
	Cfg_Button *button;
	struct TagItem dim_tags[3];
	struct RastPort rp;
	short count, image_count;

	// Initialise
	*max_width = 0;
	*max_height = 0;

	// Initialise tags
	dim_tags[0].ti_Tag = IM_Width;
	dim_tags[1].ti_Tag = IM_Height;
	dim_tags[2].ti_Tag = TAG_DONE;

	// Initialise rastport
	if (buttons->window)
		rp = *buttons->window->RPort;
	else
	{
		InitRastPort(&rp);
		if (buttons->font)
			SetFont(&rp, buttons->font);
		else
			SetFont(&rp, GUI->screen_pointer->RastPort.Font);
	}

	// Start with font height for text buttons
	if (!(buttons->bank->window.flags & BTNWF_GFX))
		*max_height = rp.TxHeight;

	// Go through buttons
	for (button = (Cfg_Button *)buttons->bank->buttons.lh_Head, count = 0, image_count = 0; button->node.ln_Succ;
		 button = (Cfg_Button *)button->node.ln_Succ, count++)
	{
		Cfg_ButtonFunction *func;

		// Go through all functions looking for images/text
		for (func = (Cfg_ButtonFunction *)button->function_list.mlh_Head; func->node.ln_Succ;
			 func = (Cfg_ButtonFunction *)func->node.ln_Succ)
		{
			// Graphical buton?
			if (button->button.flags & BUTNF_GRAPHIC)
			{
				// Clear size tags
				dim_tags[0].ti_Data = 0;
				dim_tags[1].ti_Data = 0;

				// Valid image?
				if (func->image)
				{
					// Get size of this image
					GetImageAttrs(func->image, dim_tags);
					++image_count;
				}

				// Otherwise, use unknown image size
				else if (func->function.func_type == FTYPE_LEFT_BUTTON)
				{
					dim_tags[0].ti_Data = 16;
					dim_tags[1].ti_Data = 16;
				}

				// If this image is the biggest so far, store size
				if (dim_tags[0].ti_Data > *max_width)
					*max_width = dim_tags[0].ti_Data;
				if (dim_tags[1].ti_Data > *max_height)
					*max_height = dim_tags[1].ti_Data;
			}

			// Text button
			else if (func->label)
			{
				short width;

				// Get width
				width = TextLength(&rp, func->label, strlen(func->label));

				// Maximum width?
				if (width > *max_width)
					*max_width = width;
			}
		}
	}

	// Want borders?
	if (!(buttons->bank->window.flags & BTNWF_BORDERLESS))
	{
		short xextra, yextra;

		// Graphical?
		if (buttons->bank->window.flags & BTNWF_GFX)
		{
			// Not much space for toolbars
			if (buttons->flags & BUTTONF_TOOLBAR)
			{
				xextra = 2;
				yextra = 2;
			}

			// Normal bank
			else
			{
				xextra = 8;
				yextra = 8;
			}
		}

		// Text
		else
		{
			xextra = 4;	 // <-- was 2
			yextra = 4;
		}

		// Add on space around image
		*max_width += xextra;
		*max_height += yextra;
	}

	// Minimum
	else
	{
		if (*max_width < 4)
			*max_width = 4;
		if (*max_height < 4)
			*max_height = 4;
		/*
				// Take off some size for graphical toolbar
				if (buttons->bank->window.flags&BTNWF_GFX &&
					buttons->flags&BUTTONF_TOOLBAR) *max_height-=4;
		*/
	}

	// Graphical?
	if (buttons->bank->window.flags & BTNWF_GFX)
	{
		// If no images, use big size
		if (image_count == 0)
		{
			*max_width = 96;
			*max_height = 16;
		}
	}

	return count;
}
