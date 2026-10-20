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
#define BOOPSI_LIBS
#include "boopsi.h"

static unsigned char FAR palette_layout[8][3][2] = {

	/* 2 */ {{2, 1}, {2, 1}, {1, 2}},
	/* 4 */ {{2, 2}, {4, 1}, {1, 4}},
	/* 8 */ {{4, 2}, {8, 1}, {1, 8}},
	/* 16 */ {{4, 4}, {8, 2}, {2, 8}},
	/* 32 */ {{8, 4}, {8, 4}, {4, 8}},
	/* 64 */ {{8, 8}, {16, 4}, {4, 16}},
	/* 128 */ {{16, 8}, {32, 4}, {4, 32}},
	/* 256 */ {{16, 16}, {32, 8}, {8, 32}}};

// Palette dispatcher
IPTR ASM SAVEDS palette_dispatch(REG(a0, Class *cl), REG(a2, Object *obj), REG(a1, Msg msg))
{
	struct Gadget *gadget = 0;
	PaletteData *data = 0;
	IPTR retval = 0;

	// Get gadget and data pointer
	if (obj)
	{
		gadget = (struct Gadget *)obj;
		data = INST_DATA(cl, obj);
	}

	// Look at method
	switch (msg->MethodID)
	{
	// New/set
	case OM_NEW: {
		struct TextAttr *attr;
		struct TagItem *tags;

		if (!(retval = DoSuperMethodA(cl, obj, msg)))
			break;
		gadget = (struct Gadget *)(IPTR)retval;
		data = INST_DATA(cl, (Object *)(IPTR)retval);
		data->last_row = -1;
		data->last_sel = -1;
		data->pen_count = 2;

		// Get taglist
		tags = ((struct opSet *)msg)->ops_AttrList;

		// Get font to use
		if ((attr = (struct TextAttr *)GetTagData(GTCustom_TextAttr, 0, tags)))
		{
			data->font = OpenFont(attr);
		}

		// Flags
		if (GetTagData(GTCustom_ThinBorders, 0, tags))
			data->flags |= BUTTONF_THIN;

		// Get title
		if (gadget->GadgetText)
		{
			char *ptr = 0;

			// Get title pointer
			if (gadget->Flags & GFLG_LABELSTRING)
				ptr = (char *)gadget->GadgetText;
			else if (!(gadget->Flags & GFLG_LABELIMAGE))
				ptr = (char *)gadget->GadgetText->IText;

			// Got a title?
			if (ptr)
			{
				short pos = 0;

				// Copy title string, look for underscore
				data->title_uscore = -1;
				while (*ptr && pos < 79)
				{
					// Underscore?
					if (*ptr == '_')
					{
						// Remember position, skip over it
						data->title_uscore = pos;
					}

					// Otherwise, store
					else
						data->title[pos++] = *ptr;
					++ptr;
				}
			}
		}
	}

	// Look at tags
	case OM_SET: {
		struct TagItem *tags, *tag;
		BOOL redraw = 0, recalc = 0;
		short bias = 0, colors, which = -1;

		// Get taglist
		tags = ((struct opSet *)msg)->ops_AttrList;

		// Number of pens
		if ((tag = FindTagItem(GTPA_NumColors, tags)))
		{
			if (data->pen_count != tag->ti_Data)
			{
				data->pen_count = tag->ti_Data;
				if (data->pen_sel >= data->pen_count)
					data->pen_sel = data->pen_count - 1;
				if (data->pen_sel < 0)
					data->pen_sel = 0;
				recalc = 1;
			}
		}

		// Depth
		else if ((tag = FindTagItem(GTPA_Depth, tags)))
		{
			if (data->pen_count != 1 << tag->ti_Data)
			{
				data->pen_count = 1 << tag->ti_Data;
				recalc = 1;
			}
		}

		// Pen array
		if ((tag = FindTagItem(GTPA_ColorTable, tags)) && data->pen_array != (short *)tag->ti_Data)
		{
			// Get pointer
			if (!(data->pen_array = (short *)tag->ti_Data))
			{
				short a;

				// Get default array
				if (data->def_pen_array)
					FreeVec(data->def_pen_array);
				if (!(data->def_pen_array = AllocVec(sizeof(short) * data->pen_count, 0)))
				{
					retval = 0;
					break;
				}

				// Initialise array
				for (a = 0; a < data->pen_count; a++)
					data->def_pen_array[a] = a;
				data->pen_array = data->def_pen_array;
			}
			recalc = 1;
		}

		// Need to recalculate?
		if (recalc || msg->MethodID == OM_NEW)
		{
			ULONG rem_h, rem_v;

			// If gadget width is twice the height or more, bias is towards horizontal
			if (gadget->Width >= gadget->Height << 1)
				bias = 1;

			// And vice versa
			else if (gadget->Height >= gadget->Width << 1)
				bias = 2;

			// Next power-of-2 number of colours
			for (colors = 0; colors < 8; colors++)
				if ((2 << colors) >= data->pen_count)
					break;

			// Get columns/rows
			data->pen_cols = palette_layout[colors][bias][0];
			data->pen_rows = palette_layout[colors][bias][1];

			// Look at the bias - horizontal?
			if (bias == 0 || bias == 1)
			{
				short row;

				// Find the number of rows we need
				for (row = 1;; row++)
					if ((data->pen_cols * row) >= data->pen_count)
						break;
				data->pen_rows = row;
			}

			// Vertical?
			else
			{
				short col;

				// Find the number of columns we need
				for (col = 1;; col++)
					if ((data->pen_rows * col) >= data->pen_count)
						break;
				data->pen_cols = col;
			}

			// Get the size of each pen selector
			data->pen_width = L_DivideU(gadget->Width - 8, data->pen_cols, &rem_h, UtilityBase);
			data->pen_height = L_DivideU(gadget->Height - 5, data->pen_rows, &rem_v, UtilityBase);

			// Get internal size of the gadget
			data->box.Left = gadget->LeftEdge + 3 + (rem_h >> 1);
			data->box.Top = gadget->TopEdge + 2 + (rem_v >> 1);
			data->box.Width = data->pen_width * data->pen_cols;
			data->box.Height = data->pen_height * data->pen_rows;
			recalc = 1;
		}

		// Valid pen array?
		if (data->pen_array && data->pen_count > 0)
		{
			// Pen selection
			if ((tag = FindTagItem(DPG_Pen, tags)))
			{
				ULONG rem;
				short old_pen;

				// Get pen
				old_pen = data->pen_sel;
				data->pen_sel = tag->ti_Data;

				// Map to top of palette
				if (data->pen_sel >= data->pen_count && data->pen_count > 0)
				{
					L_DivideU(data->pen_sel, data->pen_count, &rem, UtilityBase);
					data->pen_sel = rem;
				}

				// Check pen isn't disabled
				if (data->pen_array[data->pen_sel] == -1)
					data->pen_sel = old_pen;
				else
					redraw = 2;
			}

			// Color selection
			else if ((tag = FindTagItem(GTPA_Color, tags)))
			{
				short pen, old_pen;

				// Find pen in colour array
				old_pen = data->pen_sel;
				for (pen = 0; pen < data->pen_count; pen++)
				{
					if (data->pen_array[pen] == tag->ti_Data)
					{
						data->pen_sel = pen;
						redraw = 2;
						break;
					}
				}

				// Check pen isn't disabled
				if (data->pen_array[data->pen_sel] == -1)
					data->pen_sel = old_pen;
				else if (pen < data->pen_count)
					redraw = 2;
			}

			// Select next
			else if (FindTagItem(DPG_SelectNext, tags))
			{
				short old;

				// Select next pen
				old = data->pen_sel;
				data->pen_sel++;
				while (data->pen_sel != old)
				{
					// Too big?
					if (data->pen_sel >= data->pen_count)
						data->pen_sel = 0;

					// Disabled pen
					else if (data->pen_array[data->pen_sel] == -1)
						++data->pen_sel;

					// Otherwise it's ok
					else
						break;
				}
				redraw = 2;
			}

			// Select previous
			else if (FindTagItem(DPG_SelectPrevious, tags))
			{
				short old;

				// Select previous pen
				old = data->pen_sel;
				data->pen_sel--;
				while (data->pen_sel != old)
				{
					// Too small?
					if (data->pen_sel < 0)
						data->pen_sel = data->pen_count - 1;

					// Disabled pen
					else if (data->pen_array[data->pen_sel] == -1)
						--data->pen_sel;

					// Otherwise it's ok
					else
						break;
				}
				redraw = 2;
			}
		}

		// Disabled?
		if ((tag = FindTagItem(GA_Disabled, tags)))
		{
			if (tag->ti_Data && !(gadget->Flags & GFLG_DISABLED))
			{
				gadget->Flags |= GFLG_DISABLED;
				redraw = 1;
			}
			else if (!tag->ti_Data && gadget->Flags & GFLG_DISABLED)
			{
				gadget->Flags &= ~GFLG_DISABLED;
				redraw = 1;
			}
		}

		// Redraw one?
		if ((tag = FindTagItem(DPG_Redraw, tags)))
		{
			// Get pen to redraw
			which = tag->ti_Data;
			redraw = 2;
		}

		// Need a redraw?
		if ((msg->MethodID == OM_SET && redraw) || recalc)
		{
			struct RastPort *rp;

			// Get rastport
			if ((rp = ObtainGIRPort(((struct opSet *)msg)->ops_GInfo)))
			{
				// Send redraw
				DoMethod(obj,
						 GM_RENDER,
						 ((struct opSet *)msg)->ops_GInfo,
						 rp,
						 (recalc || redraw == 1) ? GREDRAW_REDRAW
												 : ((which > -1) ? (GREDRAW_EXTRA | which) : GREDRAW_UPDATE));
				ReleaseGIRPort(rp);
			}
		}
	}
	break;

	// Get an attribute
	case OM_GET: {
		struct opGet *get;

		// Get get message
		get = (struct opGet *)msg;

		// Default to ok
		retval = 1;

		// Look at attribute
		switch (get->opg_AttrID)
		{
		// Selected color
		case GTPA_Color:
			if (data->pen_array)
				*get->opg_Storage = (ULONG)data->pen_array[data->pen_sel];
			else
				*get->opg_Storage = (IPTR)-1;
			break;

		// Selected pen
		case DPG_Pen:
			*get->opg_Storage = (ULONG)data->pen_sel;
			break;

		// Other
		default:
			retval = DoSuperMethodA(cl, obj, msg);
			break;
		}
	}
	break;

	// Render gadget
	case GM_RENDER:
		palette_render(cl, gadget, data, (struct gpRender *)msg);
		break;

	// Handle input
	case GM_GOACTIVE:
	case GM_HANDLEINPUT: {
		struct gpInput *input;
		BOOL redraw = 0;

		// Get input pointer
		input = (struct gpInput *)msg;

		// Valid mouse event?
		if (input->gpi_IEvent && input->gpi_IEvent->ie_Class == IECLASS_RAWMOUSE)
		{
			// Default is not to activate
			retval = GMR_NOREUSE;

			// Button active?
			if (msg->MethodID == GM_GOACTIVE ||
				(input->gpi_IEvent->ie_Code != SELECTUP && input->gpi_IEvent->ie_Code != MENUDOWN))
			{
				short x, y;

				// Ok to activate?
				if (!(gadget->Flags & GFLG_DISABLED) && data->pen_array && data->pen_count > 0)
				{
					retval = GMR_MEACTIVE;

					// Going active?
					if (!(gadget->Flags & GFLG_SELECTED))
					{
						// Set active flag
						gadget->Flags |= GFLG_SELECTED;

						// Store last selection
						data->old_sel = data->pen_sel;
					}

					// Get coordinates relative to display box
					x = input->gpi_Mouse.X - (data->box.Left - gadget->LeftEdge);
					y = input->gpi_Mouse.Y - (data->box.Top - gadget->TopEdge);

					// Is mouse over button?
					if (x >= 0 && y >= 0 && x < data->box.Width && y < data->box.Height)
					{
						short row, col, pen;

						// Calculate row and column
						col = UDivMod32(x, data->pen_width);
						row = UDivMod32(y, data->pen_height);

						// Get selection
						pen = UMult32(row, data->pen_cols) + col;

						// Valid pen?
						if (data->pen_array && data->pen_array[pen] != -1 && pen < data->pen_count)
						{
							// Different from previous selection?
							if (data->last_sel != pen)
							{
								// Redraw
								data->pen_sel = pen;
								redraw = 1;

								// Notify
								DoMethod(obj, OM_NOTIFY, 0, input->gpi_GInfo, OPUF_INTERIM);
							}
						}
					}
				}
			}

			// Button is no longer active
			else if (gadget->Flags & GFLG_SELECTED)
			{
				// Clear select flag
				gadget->Flags &= ~GFLG_SELECTED;

				// Cancel?
				if (input->gpi_IEvent->ie_Code == MENUDOWN)
				{
					// Restore to previous value and redraw
					data->pen_sel = data->old_sel;
					redraw = 1;
				}

				// Otherwise
				else
				{
					// Send back new value
					*input->gpi_Termination = data->pen_sel;
					retval |= GMR_VERIFY;
				}

				// Notify
				DoMethod(obj, OM_NOTIFY, 0, input->gpi_GInfo, 0);
			}
		}

		// Otherwise, get default
		else
			retval = (gadget->Flags & GFLG_SELECTED) ? GMR_MEACTIVE : GMR_NOREUSE;

		// Need a redraw?
		if (redraw)
		{
			struct RastPort *rp;

			// Get rastport
			if ((rp = ObtainGIRPort(input->gpi_GInfo)))
			{
				// Send redraw
				DoMethod(obj, GM_RENDER, input->gpi_GInfo, rp, GREDRAW_UPDATE);
				ReleaseGIRPort(rp);
			}
		}
	}
	break;

	// Notify
	case OM_NOTIFY:

		// Fill out tags
		data->notify_tags[0].ti_Tag = DPG_Pen;
		data->notify_tags[0].ti_Data = data->pen_sel;
		data->notify_tags[1].ti_Tag = GA_ID;
		data->notify_tags[1].ti_Data = gadget->GadgetID;
		data->notify_tags[2].ti_Tag = TAG_END;

		// Send message
		DoSuperMethod(cl,
					  obj,
					  OM_NOTIFY,
					  data->notify_tags,
					  ((struct opUpdate *)msg)->opu_GInfo,
					  ((struct opUpdate *)msg)->opu_Flags);
		break;

	// Dispose
	case OM_DISPOSE:

		// Free font if we got one
		if (data->font)
			CloseFont(data->font);

		// Free default array
		FreeVec(data->def_pen_array);

	// Unknown method
	default:
		retval = DoSuperMethodA(cl, obj, msg);
		break;
	}

	return retval;
}

#ifdef __AROS__
AROS_UFH3(IPTR,
		  palette_dispatch_entry,
		  AROS_UFHA(Class *, cl, A0),
		  AROS_UFHA(Object *, obj, A2),
		  AROS_UFHA(Msg, msg, A1))
{
	AROS_USERFUNC_INIT
	return palette_dispatch(cl, obj, msg);
	AROS_USERFUNC_EXIT
}
#endif

// Render palette button
void palette_render(Class *cl, struct Gadget *gadget, PaletteData *data, struct gpRender *render)
{
	struct RastPort *rp;
	UWORD *pens;
	unsigned short ghost[2];
	ULONG row, col;
	short draw_one = -1;

	// Get ghosting pattern
	ghost[0] = 0x8888;
	ghost[1] = 0x2222;

	// Get rastport to use
	rp = render->gpr_RPort;
	SetDrMd(rp, JAM1);

	// Get pen array
	pens = render->gpr_GInfo->gi_DrInfo->dri_Pens;

	// Draw only one?
	if ((render->gpr_Redraw & GREDRAW_EXTRA) == GREDRAW_EXTRA)
	{
		// Get the one to draw
		draw_one = render->gpr_Redraw & ~GREDRAW_EXTRA;

		// Change to redraw
		render->gpr_Redraw = GREDRAW_REDRAW;
	}

	// Redraw the whole button?
	if (!data->done_first || render->gpr_Redraw == GREDRAW_REDRAW)
	{
		short pen, row, col, count;

		// Don't just wanna draw one square?
		if (draw_one == -1)
		{
			// Clear button background
			SetAPen(rp, pens[BACKGROUNDPEN]);
			RectFill(rp,
					 gadget->LeftEdge + ((data->flags & BUTTONF_THIN) ? 1 : 2),
					 gadget->TopEdge + 1,
					 gadget->LeftEdge + gadget->Width - ((data->flags & BUTTONF_THIN) ? 2 : 3),
					 gadget->TopEdge + gadget->Height - 2);

			// Draw border around the button
			listview_border(cl,
							rp,
							pens,
							(struct IBox *)&gadget->LeftEdge,
							IDS_NORMAL,
							(data->flags & BUTTONF_THIN) ? THIN : THICK);

			// Set flag to say done the first draw
			data->done_first = 1;
		}

		// We've cleared, so no last selection
		data->last_row = -1;

		// Gadget title
		if (data->title[0] && draw_one == -1)
		{
			short x, y;

			// Set pen for text
			SetAPen(rp, pens[TEXTPEN]);
			if (data->font)
				SetFont(rp, data->font);

			// Position for text
			x = gadget->LeftEdge + 2;
			y = gadget->TopEdge - (rp->TxHeight - rp->TxBaseline) - 1;
			Move(rp, x, y);
			Text(rp, data->title, strlen(data->title));

			// Underscore?
			if (data->title_uscore > -1)
			{
				short pos, len;

				// Get position and length of underscore
				pos = TextLength(rp, data->title, data->title_uscore);
				len = TextLength(rp, data->title + data->title_uscore, 1);

				// Draw it
				Move(rp, x + pos, y + 1);
				Draw(rp, x + pos + len - 1, y + 1);
			}
		}

		// Valid pens?
		if (data->pen_array && data->pen_count > 0)
		{
			// Draw palette entries
			count = UMult32(data->pen_cols, data->pen_rows);
			for (pen = 0, row = 0, col = 0; pen < count; pen++)
			{
				BOOL ok = 0;

				// Only draw given one if one given
				if (draw_one == -1 || draw_one == pen)
				{
					// Is this a real entry?
					if (data->pen_array[pen] != -1 && pen < data->pen_count)
					{
						// Set pen
						SetAPen(rp, data->pen_array[pen]);
						ok = 1;
					}

					// Otherwise, use shine pen
					else
						SetAPen(rp, pens[SHINEPEN]);

					// Draw this entry
					RectFill(rp,
							 data->box.Left + UMult32(col, data->pen_width) + 2,
							 data->box.Top + UMult32(row, data->pen_height) + 2,
							 data->box.Left + UMult32(col + 1, data->pen_width) - 2,
							 data->box.Top + UMult32(row + 1, data->pen_height) - 2);

					// If not ok, ghost area (unless gadget is disabled)
					if (!ok)
					{
						// Set ghosting pattern
						SetAfPt(rp, ghost, 1);
						SetAPen(rp, pens[SHADOWPEN]);

						// Fill with ghosting pattern
						RectFill(rp,
								 data->box.Left + UMult32(col, data->pen_width) + 2,
								 data->box.Top + UMult32(row, data->pen_height) + 2,
								 data->box.Left + UMult32(col + 1, data->pen_width) - 2,
								 data->box.Top + UMult32(row + 1, data->pen_height) - 2);
						SetAfPt(rp, 0, 0);
					}

					// Only wanted to draw one?
					if (draw_one > -1)
						break;
				}

				// Increment column/row
				if (++col >= data->pen_cols)
				{
					col = 0;
					++row;
				}
			}
		}
	}

	// Valid pens?
	if (data->pen_array && data->pen_count > 0)
	{
		// Calculate column and row of selection
		row = L_DivideU(data->pen_sel, data->pen_cols, &col, UtilityBase);

		// Different to previous one?
		if (row != data->last_row || col != data->last_col)
		{
			// Valid previous one?
			if (data->last_row != -1)
			{
				// Clear old selection box
				SetAPen(rp, pens[BACKGROUNDPEN]);
				Move(rp, data->selection_box.MinX, data->selection_box.MinY);
				Draw(rp, data->selection_box.MaxX, data->selection_box.MinY);
				Draw(rp, data->selection_box.MaxX, data->selection_box.MaxY);
				Draw(rp, data->selection_box.MinX, data->selection_box.MaxY);
				Draw(rp, data->selection_box.MinX, data->selection_box.MinY);
			}
		}

		// Calculate selection box
		data->selection_box.MinX = data->box.Left + UMult32(col, data->pen_width);
		data->selection_box.MinY = data->box.Top + UMult32(row, data->pen_height);
		data->selection_box.MaxX = data->selection_box.MinX + data->pen_width;
		data->selection_box.MaxY = data->selection_box.MinY + data->pen_height;

		// Draw new selection box
		SetAPen(rp, pens[SHADOWPEN]);
		Move(rp, data->selection_box.MinX, data->selection_box.MaxY);
		Draw(rp, data->selection_box.MinX, data->selection_box.MinY);
		Draw(rp, data->selection_box.MaxX - 1, data->selection_box.MinY);
		SetAPen(rp, pens[SHINEPEN]);
		Move(rp, data->selection_box.MaxX, data->selection_box.MinY);
		Draw(rp, data->selection_box.MaxX, data->selection_box.MaxY);
		Draw(rp, data->selection_box.MinX + 1, data->selection_box.MaxY);

		// Save old column/row
		data->last_col = col;
		data->last_row = row;
		data->last_sel = data->pen_sel;
	}

	// Disabled?
	if (gadget->Flags & GFLG_DISABLED || !data->pen_array || data->pen_count < 1)
	{
		// Set ghosting pattern
		SetAfPt(rp, ghost, 1);
		SetAPen(rp, pens[SHADOWPEN]);

		// Fill with ghosting pattern
		RectFill(rp,
				 gadget->LeftEdge,
				 gadget->TopEdge,
				 gadget->LeftEdge + gadget->Width - 1,
				 gadget->TopEdge + gadget->Height - 1);
		SetAfPt(rp, 0, 0);
	}
}
