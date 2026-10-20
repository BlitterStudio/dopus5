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

char *lv_FilePart(char *name);

static BOOL listview_has_custom_backfill(struct gpRender *render)
{
	struct Window *window;

	if (!render || !render->gpr_GInfo || !(window = render->gpr_GInfo->gi_Window) || !window->WLayer)
		return FALSE;

	return (window->WLayer->BackFill && window->WLayer->BackFill != LAYERS_BACKFILL);
}

// Listview dispatcher
IPTR LIBFUNC listview_dispatch(REG(a0, Class *cl), REG(a2, Object *obj), REG(a1, Msg msg))
{
	ListViewData *data = 0;
	struct Gadget *gadget = 0;
	IPTR retval = 0;

	// Get gadget and data pointers
	if (obj)
	{
		gadget = (struct Gadget *)obj;
		data = INST_DATA(cl, obj);
	}

	// Look at method
	switch (msg->MethodID)
	{
	// Create a new instance
	case OM_NEW:

		// Create superclass instance
		if ((retval = DoSuperMethodA(cl, obj, msg)))
		{
			struct TagItem *tags, *tag;
			short a;
			struct Image *image;
			struct TextAttr *attr;

			// Get pointer to our gadget and instance data
			gadget = (struct Gadget *)(IPTR)retval;
			data = INST_DATA(cl, (APTR)(IPTR)retval);

			// Initialise data
			data->dims = *(struct IBox *)&gadget->LeftEdge;
			NewList(&data->boopsi_list);
			data->sel = -1;
			data->last_selection = -1;

			// Initialise mapping tags
			data->scroll_map[0].ti_Tag = PGA_Top;
			data->scroll_map[0].ti_Data = DLV_Top;
			data->scroll_map[1].ti_Tag = TAG_END;
			data->arrow_up_map[0].ti_Tag = GA_ID;
			data->arrow_up_map[0].ti_Data = DLV_ScrollUp;
			data->arrow_up_map[1].ti_Tag = TAG_END;
			data->arrow_down_map[0].ti_Tag = GA_ID;
			data->arrow_down_map[0].ti_Data = DLV_ScrollDown;
			data->arrow_down_map[1].ti_Tag = TAG_END;

			// Get taglist
			tags = ((struct opSet *)msg)->ops_AttrList;

			// Get width of scroller
			data->scroller_width = GetTagData(DLV_ScrollWidth, 16, tags);

			// Flags
			if (FindTagItem(DLV_ShowSelected, tags))
				data->flags |= LVF_SHOW_SELECTED;
			if (GetTagData(DLV_ThinBorder, 0, tags))
				data->flags |= LVF_THIN_BORDER;
			if (GetTagData(DLV_MultiSelect, 0, tags))
				data->flags |= LVF_MULTI_SELECT;
			else if (GetTagData(DLV_Check, 0, tags))
				data->flags |= LVF_SELECTED_CHECK;
			else if ((a = GetTagData(DLV_ShowChecks, 0, tags)))
			{
				data->flags |= LVF_SHOW_CHECKS;
				if (a == 2)
					data->flags |= LVF_NO_HIGHLIGHT;
			}
			if (GetTagData(DLV_Highlight, 0, tags))
				data->flags |= LVF_SELECTED_HIGH;
			if (GetTagData(DLV_ReadOnly, 0, tags))
				data->flags |= LVF_READ_ONLY;
			if (GetTagData(DLV_NoScroller, 0, tags))
			{
				data->flags |= LVF_NO_SCROLLER;
				data->scroller_width = 0;
			}
			else if (GetTagData(DLV_ScrollLeft, 0, tags))
				data->flags |= LVF_SCROLLER_LEFT;
			if (GetTagData(DLV_TopJustify, 0, tags))
				data->flags |= LVF_TOP_JUSTIFY;
			if (GetTagData(DLV_RightJustify, 0, tags))
				data->flags |= LVF_RIGHT_JUSTIFY;
			if ((a = GetTagData(DLV_DragNotify, 0, tags)))
			{
				data->flags |= LVF_DRAG_NOTIFY;
				if (a == 2)
					data->flags |= LVF_NO_VERT_SCROLL;
			}
			data->layout_flags = GetTagData(DLV_Flags, PLACETEXT_ABOVE, tags);
			if (GetTagData(DLV_ShowFilenames, 0, tags))
				data->flags |= LVF_SHOW_FILENAMES;
			if (GetTagData(DLV_ShowSeparators, 0, tags))
				data->flags |= LVF_SHOW_SEPARATORS;

			// Get title
			if ((tag = FindTagItem(GA_Text, tags)))
			{
				char *ptr;
				short pos = 0;

				// Get pointer to title
				if ((ptr = (char *)tag->ti_Data))
				{
					data->title_uscore = -1;

					// Copy title string, look for underscore
					while (*ptr && pos < 78)
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
					data->title[pos] = 0;
				}
			}

			// Get font to use
			if ((attr = (struct TextAttr *)GetTagData(GTCustom_TextAttr, 0, tags)))
			{
				if ((data->list_font = OpenFont(attr)))
					data->flags |= LVF_GOT_FONT;
			}
			if (!data->list_font)
				data->list_font = ((struct GfxBase *)GfxBase)->DefaultFont;

			// Arrow height
			data->arrow_height = data->list_font->tf_YSize - 2;
			if (data->arrow_height < 10)
				data->arrow_height = 10;

			// Fix dimensions
			listview_get_dims(cl, data);
			data->text_height = data->list_font->tf_YSize;

			// Need check mark?
			if (data->flags & (LVF_MULTI_SELECT | LVF_SHOW_CHECKS) ||
				(data->flags & (LVF_SHOW_SELECTED | LVF_SELECTED_CHECK)) == (LVF_SHOW_SELECTED | LVF_SELECTED_CHECK))
			{
				// Create check image
				if ((data->check = (struct Image *)NewObject(0,
															 "dopusiclass",
															 DIA_Type,
															 IM_CHECK,
															 IA_Width,
															 13,
															 IA_Height,
															 data->text_height - 1,
															 TAG_END)))
				{
					// Add to boopsi list
					DoMethod((Object *)data->check, OM_ADDTAIL, &data->boopsi_list);
				}
				data->text_offset = 23;
			}
			else
				data->text_offset = 2;

			// Scroller needed?
			if (!(data->flags & LVF_NO_SCROLLER))
			{
				APTR knob;

				// System knob imagery, if Intuition provides it
				if ((knob = DOPUS_NEW_PROP_KNOB(TRUE)))
				{
					// Add to boopsi list
					DoMethod((Object *)knob, OM_ADDTAIL, &data->boopsi_list);
				}

				// Create scroller
				if ((data->scroller =
						 (struct Gadget *)NewObject(0,
													"propgclass",
													GA_Previous,
													gadget,
													GA_ID,
													LVGID_SCROLLER,
													GA_Left,
													(data->flags & LVF_SCROLLER_LEFT)
														? data->dims.Left + 4
														: data->dims.Left + data->dims.Width - data->scroller_width + 4,
													GA_Top,
													data->dims.Top + 2,
													GA_Width,
													data->scroller_width - 8,
													GA_Height,
													data->dims.Height - 4 - (data->arrow_height << 1),
													GA_Disabled,
													(gadget->Flags & GFLG_DISABLED) ? TRUE : FALSE,
													PGA_Freedom,
													FREEVERT,
													PGA_NewLook,
													TRUE,
													PGA_Borderless,
													TRUE,
													PGA_Visible,
													1,
													PGA_Total,
													1,
													DOPUS_PROP_KNOB_TAG(knob),
													(IPTR)knob,
													ICA_TARGET,
													gadget,
													ICA_MAP,
													data->scroll_map,
													TAG_END)))
				{
					// Add to boopsi list
					DoMethod((Object *)data->scroller, OM_ADDTAIL, &data->boopsi_list);
				}

				// Create up arrow image
				if ((image = (struct Image *)NewObject(0,
													   "dopusiclass",
													   IA_Width,
													   data->scroller_dims.Width,
													   IA_Height,
													   data->arrow_height,
													   DIA_Type,
													   IM_ARROW_UP,
													   DIA_ThinBorders,
													   (data->flags & LVF_THIN_BORDER) ? TRUE : FALSE,
													   TAG_END)))
				{
					// Add to boopsi list
					DoMethod((Object *)image, OM_ADDTAIL, &data->boopsi_list);
				}

				// Create up arrow button
				if ((data->arrows[0] = (struct Gadget *)NewObject(0,
																  "buttongclass",
																  GA_ID,
																  LVGID_ARROW,
																  GA_Previous,
																  data->scroller,
																  GA_Left,
																  data->scroller_dims.Left,
																  GA_Top,
																  data->scroller_dims.Top + data->scroller_dims.Height,
																  GA_Width,
																  data->scroller_dims.Width,
																  GA_Height,
																  data->arrow_height,
																  GA_Image,
																  image,
																  GA_Disabled,
																  (gadget->Flags & GFLG_DISABLED) ? TRUE : FALSE,
																  ICA_TARGET,
																  gadget,
																  ICA_MAP,
																  data->arrow_up_map,
																  TAG_END)))
				{
					// Add to boopsi list
					DoMethod((Object *)data->arrows[0], OM_ADDTAIL, &data->boopsi_list);
				}

				// Create down arrow image
				if ((image = (struct Image *)NewObject(0,
													   "dopusiclass",
													   IA_Width,
													   data->scroller_dims.Width,
													   IA_Height,
													   data->arrow_height,
													   DIA_Type,
													   IM_ARROW_DOWN,
													   DIA_ThinBorders,
													   (data->flags & LVF_THIN_BORDER) ? TRUE : FALSE,
													   TAG_END)))
				{
					// Add to boopsi list
					DoMethod((Object *)image, OM_ADDTAIL, &data->boopsi_list);
				}

				// Create down arrow button
				if ((data->arrows[1] = (struct Gadget *)NewObject(
						 0,
						 "buttongclass",
						 GA_ID,
						 LVGID_ARROW,
						 GA_Previous,
						 data->arrows[0],
						 GA_Left,
						 data->scroller_dims.Left,
						 GA_Top,
						 data->scroller_dims.Top + data->scroller_dims.Height + data->arrow_height,
						 GA_Width,
						 data->scroller_dims.Width,
						 GA_Height,
						 data->arrow_height,
						 GA_Image,
						 image,
						 GA_Disabled,
						 (gadget->Flags & GFLG_DISABLED) ? TRUE : FALSE,
						 ICA_TARGET,
						 gadget,
						 ICA_MAP,
						 data->arrow_down_map,
						 TAG_END)))
				{
					// Add to boopsi list
					DoMethod((Object *)data->arrows[1], OM_ADDTAIL, &data->boopsi_list);
				}
			}
		}
		else
			break;

	// Fall through to OM_SET

	// Update/Set
	case OM_UPDATE:
	case OM_SET: {
		struct TagItem *tag, *tags;
		unsigned short old;
		struct Node *node;
		short item;
		short redraw = 0;

		// Get taglist
		tags = ((struct opSet *)msg)->ops_AttrList;

		// Make visible?
		if ((tag = FindTagItem(DLV_MakeVisible, tags)))
		{
			short scroll;

			// Remember old top
			old = data->top;

			// Get scroll amount
			scroll = tag->ti_Data - old;

			// Need to scroll?
			if (scroll < 0 || scroll >= data->lines)
			{
				// Get scroll amount
				if (scroll >= data->lines)
					scroll -= data->lines - 1;

				// Get new top
				data->top += scroll;

				// Bounds check
				if (data->top > data->count - data->lines)
					data->top = data->count - data->lines;
				if (data->top < 0)
					data->top = 0;

				// Redraw?
				if (old != data->top)
				{
					data->last_sel = -1;
					redraw = 2;
				}
			}
		}

		// New top?
		else if ((tag = FindTagItem(DLV_Top, tags)))
		{
			// Remember old top
			old = data->top;

			// Get new top
			data->top = tag->ti_Data;

			// Bounds check
			if (data->top > data->count - data->lines)
				data->top = data->count - data->lines;
			if (data->top < 0)
				data->top = 0;

			// Redraw?
			if (old != data->top)
			{
				data->last_sel = -1;
				redraw = 2;
			}
		}

		// Scroll up
		else if ((tag = FindTagItem(DLV_ScrollUp, tags)))
		{
			// Is this the gadgetup?
			if (msg->MethodID == OM_UPDATE && !(((struct opUpdate *)msg)->opu_Flags & OPUF_INTERIM))
			{
				// Clear scroll count
				data->scroll_count = 0;
			}

			// Or is this the second/third scroll (first/second intuitick)?
			else if (msg->MethodID == OM_UPDATE && (data->scroll_count == 1 || data->scroll_count == 2))
			{
				// Throw this away
				++data->scroll_count;
			}

			// Otherwise, check it's valid
			else if (tag->ti_Data != (ULONG)-LVGID_ARROW)
			{
				// Bounds check
				if (data->top > 0)
				{
					--data->top;
					data->last_sel = -1;
					redraw = 2;
				}

				// Increment scroll count if necessary
				if (msg->MethodID == OM_UPDATE)
					++data->scroll_count;
			}
		}

		// Scroll down
		else if ((tag = FindTagItem(DLV_ScrollDown, tags)))
		{
			// Is this the gadgetup?
			if (msg->MethodID == OM_UPDATE && !(((struct opUpdate *)msg)->opu_Flags & OPUF_INTERIM))
			{
				// Clear scroll count
				data->scroll_count = 0;
			}

			// Or is this the second/third scroll (first/second intuitick)?
			else if (msg->MethodID == OM_UPDATE && (data->scroll_count == 1 || data->scroll_count == 2))
			{
				// Throw this away
				++data->scroll_count;
			}

			// Otherwise, check it's valid
			else if (tag->ti_Data != (ULONG)-LVGID_ARROW)
			{
				// Bounds check
				if (data->top < data->count - data->lines)
				{
					++data->top;
					data->last_sel = -1;
					redraw = 2;
				}

				// Increment scroll count if necessary
				if (msg->MethodID == OM_UPDATE)
					++data->scroll_count;
			}
		}

		// New labels
		if ((tag = FindTagItem(DLV_Labels, tags)))
		{
			// clear top item
			data->top_item = 0;

			// Store new label pointer
			data->labels = (struct List *)tag->ti_Data;

			// Detached list?
			if (data->labels == (struct List *)~0)
				data->flags |= LVF_DETACHED;

			// No list
			else if (data->labels == 0)
			{
				// Clear detach flag
				data->count = 0;
				data->flags &= ~LVF_DETACHED;
				redraw = 1;
			}

			// Valid list
			else
			{
				// Count items
				for (data->count = 0, node = data->labels->lh_Head; node->ln_Succ; data->count++, node = node->ln_Succ)
					;

				// Bounds check top
				if (data->top + data->lines > data->count)
					data->top = data->count - data->lines;
				if (data->top < 0)
					data->top = 0;

				// Check selection for out-of-bounds
				if (data->sel > data->count - 1)
					data->sel = -1;

				// Clear detach flag
				data->flags &= ~LVF_DETACHED;
				redraw = 1;
			}
		}

		// Selected
		if ((tag = FindTagItem(DLV_Selected, tags)) || (tag = FindTagItem(DLV_SelectPrevious, tags)) ||
			(tag = FindTagItem(DLV_SelectNext, tags)) || (tag = FindTagItem(DLV_PageUp, tags)) ||
			(tag = FindTagItem(DLV_PageDown, tags)) || (tag = FindTagItem(DLV_Home, tags)) ||
			(tag = FindTagItem(DLV_End, tags)))
		{
			// Store old
			old = data->sel;

			// Get new selected
			if (tag->ti_Tag == DLV_Selected)
			{
				data->sel = tag->ti_Data;
			}
			else if (tag->ti_Tag == DLV_SelectPrevious)
			{
				if (data->sel > 0)
					--data->sel;
			}
			else if (tag->ti_Tag == DLV_SelectNext)
			{
				++data->sel;
			}
			else if (tag->ti_Tag == DLV_PageUp)
			{
				if (data->sel == data->top)
					data->sel -= data->lines - 1;
				else
					data->sel = data->top;
				if (data->sel < 0)
					data->sel = 0;
			}
			else if (tag->ti_Tag == DLV_PageDown)
			{
				if (data->sel == data->top + data->lines - 1)
					data->sel += data->lines - 1;
				else
					data->sel = data->top + data->lines - 1;
			}
			else if (tag->ti_Tag == DLV_Home)
			{
				data->sel = 0;
			}
			else if (tag->ti_Tag == DLV_End)
			{
				data->sel = data->count - 1;
			}

			// Bounds check
			if (data->sel > data->count - 1)
				data->sel = data->count - 1;
			if (data->sel < 0)
				data->sel = -1;

			// Changed?
			if (data->sel != old && !(gadget->Flags & GFLG_DISABLED))
			{
				// Save selection
				if (!redraw)
				{
					data->last_sel = data->last_selection;
					redraw = 2;
				}
				data->last_selection = data->sel;

				// Send notify
				DoMethod(obj, OM_NOTIFY, 0, ((struct opSet *)msg)->ops_GInfo, 0);
			}
		}

		// Disabled (set only)
		if (msg->MethodID == OM_SET && (tag = FindTagItem(GA_Disabled, tags)))
		{
			// Different to current state?
			if ((tag->ti_Data && !(gadget->Flags & GFLG_DISABLED)) || (!tag->ti_Data && gadget->Flags & GFLG_DISABLED))
			{
				struct TagItem distags[2];
				short a;

				// Fill out disable tags
				distags[0].ti_Tag = GA_Disabled;
				distags[0].ti_Data = tag->ti_Data;
				distags[1].ti_Tag = TAG_DONE;

				// Set state of slider
				if (data->scroller)
				{
					DoMethod((Object *)data->scroller, OM_SET, distags, ((struct opSet *)msg)->ops_GInfo);

					// And arrows
					for (a = 0; a < 2; a++)
						DoMethod((Object *)data->arrows[a], OM_SET, distags, ((struct opSet *)msg)->ops_GInfo);

					// Set flag to say disable state changed
					if (!tag->ti_Data)
						data->flags |= LVF_DISABLE_CHANGE;
				}

				// Change disable state and redraw
				gadget->Flags ^= GFLG_DISABLED;
				redraw = 1;
			}
		}

		// Get top item
		if (!data->labels)
			data->top_item = 0;
		else if (!(data->flags & LVF_DETACHED) && data->labels)
		{
			for (node = data->labels->lh_Head, item = 0; node->ln_Succ && item < data->top;
				 node = node->ln_Succ, item++)
				;
			data->top_item = node;
		}

		// Do we need to redraw the list?
		if (msg->MethodID == OM_SET || msg->MethodID == OM_UPDATE)
		{
			if (redraw && !(data->flags & LVF_DETACHED))
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
							 (redraw == 2) ? GREDRAW_UPDATE : GREDRAW_REDRAW);
					ReleaseGIRPort(rp);
				}
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
		// Selected
		case DLV_Selected:
			*get->opg_Storage = (ULONG)data->last_selection;
			break;

		// Top
		case DLV_Top:
			*get->opg_Storage = (ULONG)data->top;
			break;

		// Lines
		case DLV_Lines:
			*get->opg_Storage = (ULONG)data->lines;
			break;

		// Labels
		case DLV_Labels:
			*get->opg_Storage = (IPTR)data->labels;
			break;

		// Get line from coordinates
		case DLV_GetLine: {
			unsigned short x, y;
			struct gpInput fake;

			// Get coordinates
			x = ((*get->opg_Storage) >> 16) & 0xffff;
			y = (*get->opg_Storage) & 0xffff;

			// Fake gpInput
			fake.gpi_Mouse.X = x - gadget->LeftEdge;
			fake.gpi_Mouse.Y = y - gadget->TopEdge;

			// Get selection
			*get->opg_Storage = (ULONG)listview_get_sel(cl, gadget, data, &fake, 0);
		}
		break;

		// Draw a line from the listview
		case DLV_DrawLine: {
			ListViewDraw *draw = (ListViewDraw *)*get->opg_Storage;

			// Draw line
			listview_draw_item(cl, gadget, draw->rp, draw->drawinfo, data, draw->node, draw->line, &draw->box);
		}
		break;

		// Other
		default:
			retval = DoSuperMethodA(cl, obj, msg);
			break;
		}
	}
	break;

	// Kill an instance
	case OM_DISPOSE: {
		APTR object_ptr;
		Object *object;

		// Free BOOPSI objects
		object_ptr = data->boopsi_list.lh_Head;
		while ((object = NextObject((APTR)&object_ptr)))
			DisposeObject(object);

		// Free font
		if (data->flags & LVF_GOT_FONT)
			CloseFont(data->list_font);

		retval = DoSuperMethodA(cl, obj, msg);
	}
	break;

	// Render gadget
	case GM_RENDER:

		// Cache DrawInfo
		data->draw_info = ((struct gpRender *)msg)->gpr_GInfo->gi_DrInfo;

		// If a full redraw, clear double-click time
		if (((struct gpRender *)msg)->gpr_Redraw == GREDRAW_REDRAW)
			data->seconds = 0;

		// Show gadget
		listview_render(cl, gadget, data, (struct gpRender *)msg);
		break;

	// Hit test
	case GM_HITTEST: {
		struct gpHitTest *test;

		// Get test pointer
		test = (struct gpHitTest *)msg;

		// See if we're hit
		if (test->gpht_Mouse.X >= data->text_dims.Left - gadget->LeftEdge &&
			test->gpht_Mouse.Y >= data->text_dims.Top - gadget->TopEdge &&
			test->gpht_Mouse.X < data->text_dims.Left + data->text_dims.Width - gadget->LeftEdge &&
			test->gpht_Mouse.Y < data->text_dims.Top + data->text_dims.Height - gadget->TopEdge)
		{
			retval = GMR_GADGETHIT;
		}
	}
	break;

	// Notify
	case OM_NOTIFY:

		// Drag notify?
		if (data->flags & LVF_DRAG_FLAG)
		{
			// Fill out tags
			data->notify_tags[0].ti_Tag = DLV_DragNotify;
			data->notify_tags[0].ti_Data = data->drag_sel;
			data->notify_tags[1].ti_Tag = GA_ID;
			data->notify_tags[1].ti_Data = gadget->GadgetID;
			data->notify_tags[2].ti_Tag = GA_Left;
			data->notify_tags[2].ti_Data = data->text_dims.Left;
			data->notify_tags[3].ti_Tag = GA_Top;
			data->notify_tags[3].ti_Data = data->text_dims.Top + UMult32(data->drag_sel - data->top, data->text_height);
			data->notify_tags[4].ti_Tag = GA_Width;
			data->notify_tags[4].ti_Data = data->text_dims.Width;
			data->notify_tags[5].ti_Tag = GA_Height;
			data->notify_tags[5].ti_Data = data->text_height;
			data->notify_tags[6].ti_Tag = GA_RelRight;
			data->notify_tags[6].ti_Data = data->drag_x_position;
			data->notify_tags[7].ti_Tag = GA_RelBottom;
			data->notify_tags[7].ti_Data = data->drag_y_position;
			data->notify_tags[8].ti_Tag = DLV_Top;
			data->notify_tags[8].ti_Data = data->top;
			data->notify_tags[9].ti_Tag = DLV_Object;
			data->notify_tags[9].ti_Data = (IPTR)gadget;
			data->notify_tags[10].ti_Tag = TAG_END;

			// Clear flag
			data->flags &= ~LVF_DRAG_FLAG;
		}

		// Otherwise normal notify
		else
		{
			ULONG seconds, micros;

			// Fill out tags
			data->notify_tags[0].ti_Tag = DLV_Selected;
			data->notify_tags[0].ti_Data = data->last_selection;
			data->notify_tags[1].ti_Tag = GA_ID;
			data->notify_tags[1].ti_Data = gadget->GadgetID;
			data->notify_tags[2].ti_Tag = TAG_END;

			// Get current time
			CurrentTime(&seconds, &micros);

			// Double-click?
			if (data->click_sel == data->last_selection && DoubleClick(data->seconds, data->micros, seconds, micros))
			{
				// Set double-click flag
				data->notify_tags[2].ti_Tag = DLV_DoubleClick;
				data->notify_tags[2].ti_Data = TRUE;
				data->notify_tags[3].ti_Tag = TAG_END;

				// Clear double-click
				data->seconds = 0;
			}

			// Otherwise
			else
			{
				// Remember double-click
				data->seconds = seconds;
				data->micros = micros;
				data->click_sel = data->last_selection;
			}
		}

		// Send message
		DoSuperMethod(cl, obj, OM_NOTIFY, data->notify_tags, ((struct opUpdate *)msg)->opu_GInfo, 0);
		break;

	// Go (in)active/Handle input
	case GM_GOACTIVE:
	case GM_GOINACTIVE:
	case GM_HANDLEINPUT: {
		struct gpInput *input;
		short redraw = 0;

		// Get input pointer
		input = (struct gpInput *)msg;

		// Go active?
		if (input->MethodID == GM_GOACTIVE)
		{
			// Default is not to activate
			retval = GMR_NOREUSE;

			// Ok to go active?
			if (!(gadget->Flags & GFLG_DISABLED) && input->gpi_IEvent)
			{
				// Left button down and valid list?
				if (input->gpi_IEvent->ie_Code == SELECTDOWN && data->labels &&
					!(data->flags & (LVF_READ_ONLY | LVF_DETACHED)))
				{
					short sel;

					// Get selection
					if ((sel = listview_get_sel(cl, gadget, data, input, 0)) > -1)
					{
						// Ok to activate
						retval = GMR_MEACTIVE;

						// Store current top
						data->last_top = data->top;

						// Remember x and y click-offset
						data->drag_x_position = input->gpi_Mouse.X - (data->text_dims.Left - data->dims.Left);
						data->drag_y_position = input->gpi_Mouse.Y - (data->text_dims.Top - data->dims.Top) -
												UMult32(sel - data->top, data->text_height);

						// Change?
						if (data->sel != sel || !(data->flags & LVF_SHOW_SELECTED) ||
							(data->flags & LVF_SELECTED_CHECK))
						{
							// New selection
							data->last_sel = data->last_selection;
							data->sel = sel;
							redraw = 1;
						}

						// Set selected flag
						gadget->Flags |= GFLG_SELECTED;
					}
				}
			}
		}

		// Go inactive?
		else if (msg->MethodID == GM_GOINACTIVE)
		{
			// Clear selected flag
			gadget->Flags &= ~GFLG_SELECTED;

			// If we're showing selection with checks, we need to redraw last selection
			if ((data->flags & (LVF_SHOW_SELECTED | LVF_SELECTED_CHECK)) == (LVF_SHOW_SELECTED | LVF_SELECTED_CHECK))
				data->last_sel = data->last_selection;
			redraw = 1;

			// Save this selection
			data->last_selection = data->sel;

			// Send notify
			DoMethod(obj, OM_NOTIFY, 0, ((struct gpInput *)msg)->gpi_GInfo, 0);

			// Clear cancel flag
			data->flags &= ~LVF_CANCEL;
		}

		// Handle input
		else
		{
			// Default return value
			retval = GMR_MEACTIVE;

			// Is it a mouse event?
			if (input->gpi_IEvent->ie_Class == IECLASS_RAWMOUSE)
			{
				// Look at mouse code
				switch (input->gpi_IEvent->ie_Code)
				{
				// Left button up
				case SELECTUP: {
					// Set return code
					*input->gpi_Termination = data->sel;

					// Gadget processing finished
					retval = GMR_NOREUSE | GMR_VERIFY;
					data->flags |= LVF_CANCEL;

					// Multi-selection?
					if (data->flags & LVF_MULTI_SELECT)
					{
						struct Node *node;
						short count;

						// Get this node
						for (node = data->labels->lh_Head, count = 0; node->ln_Succ && count < data->sel;
							 node = node->ln_Succ, count++)
							;

						// Did we get it?
						if (node && node->ln_Succ)
						{
							// Save old selection
							data->last_sel = data->sel;

							// Toggle selection
							node->lve_Flags ^= LVEF_SELECTED;
							redraw = 1;
						}
					}
				}
				break;

				// No button; mouse moved
				case IECODE_NOBUTTON: {
					short sel;

					// Is mouse outside of lister?
					if (input->gpi_Mouse.X < 0 || input->gpi_Mouse.X >= data->list_dims.Width)
					{
						// Drag notify?
						if (data->flags & LVF_DRAG_NOTIFY)
						{
							// Valid selection?
							if (data->sel > -1)
							{
								// Notify drag
								data->flags |= LVF_DRAG_FLAG;
								data->drag_sel = data->sel;

								// Will fall through to MENUDOWN (if NO_VERT_SCROLL)
								if (!(data->flags & LVF_NO_VERT_SCROLL))
								{
									retval = GMR_NOREUSE;
									data->flags |= LVF_CANCEL;
									break;
								}
							}
							else
								break;
						}
						else
							break;
					}

					// Get selection
					else if ((sel = listview_get_sel(cl, gadget, data, input, 1)) > -1 ||
							 (sel == -1 && (data->flags & LVF_NO_VERT_SCROLL)))
					{
						// Change?
						if (data->sel != sel || sel == -1)
						{
							// Clear double-click
							data->seconds = 0;

							// No vertical scroll?
							if (data->flags & LVF_NO_VERT_SCROLL)
							{
								// Drag notify?
								if (data->flags & LVF_DRAG_NOTIFY)
								{
									// Notify drag
									data->flags |= LVF_DRAG_FLAG;
									data->drag_sel = data->sel;

									// Will fall through to MENUDOWN
								}
								else
									break;
							}

							// Otherwise
							else
							{
								// Save old selection
								data->last_sel = data->sel;

								// New selection
								data->sel = sel;
								redraw = 1;
								break;
							}
						}
						else
							break;
					}

					else
						break;
				}

				// Right button does cancel
				case MENUDOWN: {
					// Gadget processing finished
					retval = GMR_NOREUSE;
					data->flags |= LVF_CANCEL;

					// Show selected?
					if (data->flags & LVF_SHOW_SELECTED)
					{
						// Restore last selection
						data->last_sel = data->sel;
						data->sel = data->last_selection;

						// Does top need to change?
						if (data->top != data->last_top)
						{
							short item;

							data->top = data->last_top;
							data->last_sel = -1;

							for (data->top_item = data->labels->lh_Head, item = 0;
								 data->top_item->ln_Succ && item < data->top;
								 data->top_item = data->top_item->ln_Succ, item++)
								;
						}

						redraw = 1;
					}
				}
				break;
				}
			}

			// Or a timer event
			else if (input->gpi_IEvent->ie_Class == IECLASS_TIMER)
			{
				short sel;

				// Get selection
				if ((sel = listview_get_sel(cl, gadget, data, input, 2)) > -1)
				{
					// Change?
					if (data->sel != sel)
					{
						// Save old selection
						data->last_sel = data->sel;

						// New selection
						data->sel = sel;

						// Do we have to scroll?
						if (sel < data->top)
						{
							if (data->top > 0)
							{
								// Scroll up
								--data->top;
								data->top_item = data->top_item->ln_Pred;

								// Redraw the list
								data->last_sel = -1;
								redraw = 1;
							}
						}
						else if (sel >= data->top + data->lines)
						{
							if (data->top + data->lines < data->count)
							{
								// Scroll down
								++data->top;
								data->top_item = data->top_item->ln_Succ;

								// Redraw the list
								data->last_sel = -1;
								redraw = 1;
							}
						}
						else
							redraw = 1;
					}
				}
			}
		}

		// Redraw needed?
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

	// Resize
	case GM_RESIZE:

		// Handle the resize
		listview_resize(cl, gadget, data, (struct gpResize *)msg);
		break;

	// Unknown method
	default:
		retval = DoSuperMethodA(cl, obj, msg);
		break;
	}

	return retval;
}

#ifdef __AROS__
AROS_UFH3(IPTR,
		  listview_dispatch_entry,
		  AROS_UFHA(Class *, cl, A0),
		  AROS_UFHA(Object *, obj, A2),
		  AROS_UFHA(Msg, msg, A1))
{
	AROS_USERFUNC_INIT
	return listview_dispatch(cl, obj, msg);
	AROS_USERFUNC_EXIT
}
#endif

// Render listview
void listview_render(Class *cl, struct Gadget *gadget, ListViewData *data, struct gpRender *render)
{
	struct RastPort *rp;
	UWORD *pens;

	// Get rastport to use
	rp = (render->MethodID == GM_RENDER) ? render->gpr_RPort : ObtainGIRPort(render->gpr_GInfo);

	// Get pen array
	pens = render->gpr_GInfo->gi_DrInfo->dri_Pens;

	// Set font
	SetFont(rp, data->list_font);

	// Look at render type
	switch (render->gpr_Redraw)
	{
	// Draw the whole gadget?
	case GREDRAW_REDRAW: {
		struct TagItem tags[4];
		short a;

		// Valid title?
		if (data->title[0])
		{
			BOOL custom_backfill_title;
			short y, x, len, str_len;

			// Get title length
			str_len = strlen(data->title);
			len = TextLength(rp, data->title, str_len);
			custom_backfill_title = listview_has_custom_backfill(render);

			// Get x and y position for title
			if (data->layout_flags & PLACETEXT_LEFT)
			{
				x = gadget->LeftEdge - len - 8;
				y = gadget->TopEdge + 2 + rp->TxBaseline;
			}
			else
			{
				x = gadget->LeftEdge;
				y = (gadget->TopEdge - 1 - rp->TxHeight) + rp->TxBaseline;
			}

			// Draw title
			SetAPen(rp, pens[TEXTPEN]);
			SetBPen(rp, pens[BACKGROUNDPEN]);
			SetDrMd(rp, (custom_backfill_title) ? JAM1 : JAM2);
			if (custom_backfill_title)
				EraseRect(rp, x, y - rp->TxBaseline, x + len - 1, y - rp->TxBaseline + rp->TxHeight - 1);
			Move(rp, x, y);
			Text(rp, data->title, str_len);

			// Underscore?
			if (data->title_uscore > -1)
			{
				int pos, len;

				// Get position and length of underscore
				pos = TextLength(rp, data->title, data->title_uscore);
				len = TextLength(rp, data->title + data->title_uscore, 1);

				// Draw it
				Move(rp, x + pos, y + 1);
				Draw(rp, x + pos + len - 1, y + 1);
			}

			// Reset draw mode
			SetDrMd(rp, JAM2);
		}

		// Draw border around the list
		listview_border(cl,
						rp,
						pens,
						&data->list_dims,
						(data->flags & LVF_READ_ONLY) ? IDS_SELECTED : IDS_NORMAL,
						(data->flags & LVF_THIN_BORDER) ? THIN : THICK);

		// Draw scroller border
		if (data->scroller)
		{
			listview_border(
				cl, rp, pens, &data->scroller_dims, IDS_NORMAL, (data->flags & LVF_THIN_BORDER) ? THIN : THICK);

			// Draw blank area inside scroller border
			SetAPen(rp, pens[BACKGROUNDPEN]);
			if (0 && data->flags & LVF_DISABLE_CHANGE)
			{
				RectFill(
					rp,
					data->scroller_dims.Left + ((data->flags & LVF_THIN_BORDER) ? 1 : 2),
					data->scroller_dims.Top + 1,
					data->scroller_dims.Left + data->scroller_dims.Width - ((data->flags & LVF_THIN_BORDER) ? 2 : 3),
					data->scroller_dims.Top + data->scroller_dims.Height - 2);
				data->flags &= ~LVF_DISABLE_CHANGE;
			}
			else
			{
				if (data->flags & LVF_THIN_BORDER)
				{
					Move(rp, data->scroller_dims.Left + 1, data->scroller_dims.Top + 1);
					Draw(rp, data->scroller_dims.Left + 1, data->scroller_dims.Top + data->scroller_dims.Height - 2);
					Move(rp, data->scroller_dims.Left + data->scroller_dims.Width - 2, data->scroller_dims.Top + 1);
					Draw(rp,
						 data->scroller_dims.Left + data->scroller_dims.Width - 2,
						 data->scroller_dims.Top + data->scroller_dims.Height - 2);
				}

				Move(rp, data->scroller_dims.Left + 2, data->scroller_dims.Top + 1);
				Draw(rp, data->scroller_dims.Left + data->scroller_dims.Width - 3, data->scroller_dims.Top + 1);
				Draw(rp,
					 data->scroller_dims.Left + data->scroller_dims.Width - 3,
					 data->scroller_dims.Top + data->scroller_dims.Height - 2);
				Draw(rp, data->scroller_dims.Left + 2, data->scroller_dims.Top + data->scroller_dims.Height - 2);
				Draw(rp, data->scroller_dims.Left + 2, data->scroller_dims.Top + 1);
				Move(rp, data->scroller_dims.Left + 3, data->scroller_dims.Top + 1);
				Draw(rp, data->scroller_dims.Left + 3, data->scroller_dims.Top + data->scroller_dims.Height - 2);
				Move(rp, data->scroller_dims.Left + data->scroller_dims.Width - 4, data->scroller_dims.Top + 1);
				Draw(rp,
					 data->scroller_dims.Left + data->scroller_dims.Width - 4,
					 data->scroller_dims.Top + data->scroller_dims.Height - 2);
			}
		}

		// Draw list
		listview_draw_items(cl, gadget, rp, render->gpr_GInfo->gi_DrInfo, data, -1);

		// Fill out scroller tags
		if (data->scroller)
		{
			tags[0].ti_Tag = PGA_Total;
			tags[0].ti_Data = data->count;
			tags[1].ti_Tag = PGA_Visible;
			tags[1].ti_Data = data->lines;
			tags[2].ti_Tag = PGA_Top;
			tags[2].ti_Data = data->top;
			tags[3].ti_Tag = TAG_END;

			// Update scroller
			DoMethod((Object *)data->scroller, OM_SET, tags, render->gpr_GInfo);

			// Refresh arrows
			for (a = 0; a < 2; a++)
				DoMethod((Object *)data->arrows[a], GM_RENDER, render->gpr_GInfo, rp, GREDRAW_REDRAW);

			// Refresh scroller
			DoMethod((Object *)data->scroller, GM_RENDER, render->gpr_GInfo, rp, GREDRAW_REDRAW);
		}

		// Disabled?
		if (gadget->Flags & GFLG_DISABLED)
		{
			unsigned short ghost[2];

			// Set ghosting pattern
			ghost[0] = 0x8888;
			ghost[1] = 0x2222;
			SetAfPt(rp, ghost, 1);
			SetAPen(rp, pens[SHADOWPEN]);
			SetDrMd(rp, JAM1);

			// Fill with ghosting pattern
			RectFill(rp,
					 data->dims.Left,
					 data->dims.Top,
					 data->dims.Left + data->dims.Width - 1,
					 data->dims.Top + data->dims.Height - 1);

			// Clear ghosting pattern
			SetAfPt(rp, 0, 0);
		}

		// Otherwise, clear any gap between text area and gadget
		else
		{
			SetAPen(rp, pens[BACKGROUNDPEN]);
			if (data->text_dims.Top > data->dims.Top + 1)
			{
				RectFill(rp,
						 data->text_dims.Left,
						 data->dims.Top + 1,
						 data->text_dims.Left + data->text_dims.Width - 1,
						 data->text_dims.Top - 1);
			}
			if (data->text_dims.Top + data->text_dims.Height - 1 < data->dims.Top + data->dims.Height - 2)
			{
				RectFill(rp,
						 data->text_dims.Left,
						 data->text_dims.Top + data->text_dims.Height,
						 data->text_dims.Left + data->text_dims.Width - 1,
						 data->dims.Top + data->dims.Height - 2);
			}
		}
	}
	break;

	// Just refresh items
	case GREDRAW_UPDATE: {
		// Draw list
		listview_draw_items(cl, gadget, rp, render->gpr_GInfo->gi_DrInfo, data, data->last_sel);

		// Refresh scroller?
		if (data->last_sel == -1 && data->scroller)
		{
			struct TagItem tags[2];

			// Fill out scroller tags
			tags[0].ti_Tag = PGA_Top;
			tags[0].ti_Data = data->top;
			tags[1].ti_Tag = TAG_END;

			// Update scroller
			DoMethod((Object *)data->scroller, OM_SET, tags, render->gpr_GInfo);
		}
	}
	break;
	}

	// Do we need to free the rastport?
	if (render->MethodID != GM_RENDER)
		ReleaseGIRPort(rp);
}

// Draw a border
void listview_border(Class *cl, struct RastPort *rp, UWORD *pens, struct IBox *box, ULONG state, short single)
{
	SetAPen(rp, pens[(state == IDS_SELECTED) ? SHADOWPEN : SHINEPEN]);
	Move(rp, box->Left + box->Width - ((single) ? 1 : 2), box->Top);
	Draw(rp, box->Left, box->Top);
	Draw(rp, box->Left, box->Top + box->Height - 1);
	if (!single)
	{
		Move(rp, box->Left + 1, box->Top + box->Height - 2);
		Draw(rp, box->Left + 1, box->Top + 1);
	}

	SetAPen(rp, pens[(state == IDS_SELECTED) ? SHINEPEN : SHADOWPEN]);
	Move(rp, box->Left + 1, box->Top + box->Height - 1);
	Draw(rp, box->Left + box->Width - 1, box->Top + box->Height - 1);
	Draw(rp, box->Left + box->Width - 1, box->Top);
	if (!single)
	{
		Move(rp, box->Left + box->Width - 2, box->Top + box->Height - 2);
		Draw(rp, box->Left + box->Width - 2, box->Top + 1);
	}
}

// Draw list items
void listview_draw_items(Class *cl,
						 struct Gadget *gadget,
						 struct RastPort *rp,
						 struct DrawInfo *drawinfo,
						 ListViewData *data,
						 short old_sel)
{
	struct Node *node;
	unsigned short line;

	// Is list detached?
	if (data->flags & LVF_DETACHED)
		return;

	// Clear list?
	if (!data->labels || !data->labels->lh_Head->ln_Succ)
	{
		SetAPen(rp, drawinfo->dri_Pens[BACKGROUNDPEN]);
		RectFill(rp,
				 data->list_dims.Left + ((data->flags & LVF_THIN_BORDER) ? 1 : 2),
				 data->list_dims.Top + 1,
				 data->list_dims.Left + data->list_dims.Width - ((data->flags & LVF_THIN_BORDER) ? 2 : 3),
				 data->list_dims.Top + data->list_dims.Height - 2);
		return;
	}

	// Draw items
	for (node = data->top_item, line = 0; line < data->lines; node = (node) ? node->ln_Succ : 0, line++)
	{
		// Only drawing current and last?
		if (old_sel > -1)
		{
			// Ok to draw this?
			if (line == old_sel - data->top || line == data->sel - data->top)
			{
				// Draw this line
				listview_draw_item(cl, gadget, rp, drawinfo, data, node, line, 0);

				// Are they the same?
				if (old_sel == data->sel)
					break;
			}
		}

		// Drawing all
		else
			listview_draw_item(cl, gadget, rp, drawinfo, data, node, line, 0);
	}
}

// Draw an item in the listview
void listview_draw_item(Class *cl,
						struct Gadget *gadget,
						struct RastPort *rp,
						struct DrawInfo *drawinfo,
						ListViewData *data,
						struct Node *node,
						unsigned short line,
						struct IBox *user_box)
{
	short selected = 0, tab_flag = 0, high;
	unsigned short x, y, len = 0, textlen, pos = 0, tabpos = 0, width, last_x = 0;
	short fpen;
	struct TextExtent extent;
	struct IBox *box;
	char *name;

	// User-supplied box?
	if (user_box)
	{
		box = user_box;
		high = 1;
	}
	else
	{
		box = &data->text_dims;
		high = (BOOL)(data->sel == data->top + line);
	}

	// Get y position
	y = ((unsigned short)box->Top) + line * data->text_height;

	// Invalid item?
	if (!node || !node->ln_Succ)
	{
		// Fill item
		SetAPen(rp, drawinfo->dri_Pens[(high) ? FILLPEN : BACKGROUNDPEN]);
		RectFill(rp, box->Left, y, box->Left + box->Width - 1, y + data->text_height - 1);
		return;
	}

	// Does this item need a highlight select?
	if (high)
	{
		if ((!(data->flags & LVF_CANCEL) && gadget->Flags & GFLG_SELECTED) ||
			(data->flags & LVF_SHOW_SELECTED && !(data->flags & LVF_SELECTED_CHECK)))
			selected = 1;
	}

	// Highlight selection?
	if (high && !(gadget->Flags & GFLG_SELECTED) && data->flags & LVF_SELECTED_HIGH)
	{
		fpen = drawinfo->dri_Pens[HIGHLIGHTTEXTPEN];
	}

	// Highlight multi-select?
	else if (!(data->flags & LVF_NO_HIGHLIGHT) && data->flags & LVF_MULTI_SELECT && data->flags & LVF_SELECTED_HIGH &&
			 node->lve_Flags & LVEF_SELECTED)
	{
		fpen = drawinfo->dri_Pens[HIGHLIGHTTEXTPEN];
	}

	// Custom pen supplied?
	else if (node->lve_Flags & LVEF_USE_PEN)
	{
		// If selected, check pen is different to background
		if (selected && node->lve_Pen == drawinfo->dri_Pens[FILLPEN])
			fpen = drawinfo->dri_Pens[FILLTEXTPEN];

		// Ok to use custom pen
		else
			fpen = node->lve_Pen;
	}

	// Normal entry
	else
	{
		fpen = drawinfo->dri_Pens[(selected) ? FILLTEXTPEN : TEXTPEN];
	}

	SetAPen(rp, fpen);
	SetBPen(rp, drawinfo->dri_Pens[(selected) ? FILLPEN : BACKGROUNDPEN]);
	SetDrMd(rp, JAM2);

	// Valid name?
	if (node->ln_Name)
	{
		// Get name pointer
		name = node->ln_Name;

		// Only show filenames?
		if (data->flags & LVF_SHOW_FILENAMES)
		{
			// Get filename pointer
			name = lv_FilePart(name);
		}

		// Get text length
		textlen = strlen(name);

		// Right-justify?
		if (data->flags & LVF_RIGHT_JUSTIFY)
		{
			char *dots = 0;
			short dotlen = 0;

			// Get length that will fit (from end backwards)
			len = TextFit(rp, name + textlen - 1, textlen, &extent, 0, -1, data->text_width, data->text_height);

			// Won't all fit?
			if (len < textlen)
			{
				// Get length of dots
				dots = "...";
				dotlen = TextLength(rp, dots, 3);

				// Get new length of string that will fit
				len = TextFit(
					rp, name + textlen - 1, textlen, &extent, 0, -1, data->text_width - dotlen, data->text_height);
			}

			// Draw text
			Move(rp, box->Left + data->text_offset, y + rp->TxBaseline);
			if (dots)
				Text(rp, dots, 3);
			Text(rp, name + textlen - len, len);

			// Save x position
			last_x = data->text_offset + dotlen + extent.te_Width;
		}

		// Normal
		else
		{
			BOOL sep = 0;

			// Get x position
			x = data->text_offset;

			// Show separators?
			if (data->flags & LVF_SHOW_SEPARATORS && strcmp(name, "---") == 0)
				sep = 1;

			// Go through string
			while (pos < textlen)
			{
				// Separator?
				if (sep)
				{
					// Get full size
					width = data->text_width - x;
				}

				// Need to parse
				else
				{
					// Look for tab
					for (tabpos = 0; name[pos + tabpos]; tabpos++)
						if (name[pos + tabpos] == '\t' || name[pos + tabpos] == '\a' || name[pos + tabpos] == '\v')
							break;

					// Get text size
					if (tabpos > 0)
					{
						len = TextFit(rp, name + pos, tabpos, &extent, 0, 1, data->text_width - x, data->text_height);
						width = extent.te_Width;
					}
					else
					{
						len = 0;
						width = 0;
					}

					// Tab across if needed
					if (tab_flag == 1)
						x = data->text_width - width;
					else if (tab_flag == 2)
						x = (data->text_width - width) >> 1;
				}

				// Fill background if needed
				if (last_x && last_x < x)
				{
					SetAPen(rp, drawinfo->dri_Pens[(selected) ? FILLPEN : BACKGROUNDPEN]);
					RectFill(rp, box->Left + last_x, y, box->Left + x - 1, y + data->text_height - 1);
					SetAPen(rp, fpen);
				}

				// Draw separator?
				if (sep)
				{
					short yp, xp;

					// Clear area under line
					SetAPen(rp, drawinfo->dri_Pens[(selected) ? FILLPEN : BACKGROUNDPEN]);
					RectFill(rp, box->Left + x, y, (xp = box->Left + x + width - 1), y + data->text_height - 1);
					SetAPen(rp, fpen);

					// Get y position
					yp = y + (data->text_height >> 1);

					// Draw line
					SetDrPt(rp, 0xbbbb);
					Move(rp, box->Left + x, yp);
					Draw(rp, xp, yp);
					SetDrPt(rp, 0xeeee);
					Move(rp, box->Left + x, yp + 1);
					Draw(rp, xp, yp + 1);
					SetDrPt(rp, 0xffff);
				}

				// Draw text
				else if (len > 0)
				{
					Move(rp, box->Left + x, y + rp->TxBaseline);
					Text(rp, name + pos, len);
				}

				// Save x position
				last_x = x + width;

				// Break if just did separator
				if (sep)
					break;

				// Bump buffer pointer
				pos += tabpos + 1;

				// Set tab flag
				if (pos > 0)
				{
					if (name[pos - 1] == '\t')
						tab_flag = 1;
					else if (name[pos - 1] == '\v')
						tab_flag = 2;
					else if (name[pos - 1] == '\a')
					{
						unsigned short tab;

						// Move text position across
						tab = (unsigned short)name[pos];
						x += tab * rp->TxWidth;

						// Bump position
						++pos;
					}

					// Tab flag not set, bump position
					else
						x += width;
				}
			}
		}
	}

	// Set colour to fill background
	SetAPen(rp, drawinfo->dri_Pens[(selected) ? FILLPEN : BACKGROUNDPEN]);

	// Fill beginning of line
	RectFill(rp, box->Left, y, box->Left + data->text_offset - 1, y + data->text_height - 1);

	// Fill end of line?
	if (last_x < box->Width)
	{
		RectFill(rp, box->Left + last_x, y, box->Left + box->Width - 1, y + data->text_height - 1);
	}

	// Check this item?
	if ((data->flags & (LVF_MULTI_SELECT | LVF_SHOW_CHECKS) && node->lve_Flags & LVEF_SELECTED) ||
		(data->flags & LVF_SELECTED_CHECK && data->sel == data->top + line && !selected) ||
		(data->flags & LVF_SELECTED_CHECK && data->last_selection == data->top + line))
	{
		// Draw checkmark
		if (data->check)
		{
			DrawImageState(rp, data->check, box->Left + 6, y, IDS_NORMAL, drawinfo);
		}
	}
}

// Find which line the mouse is over
short listview_get_sel(Class *cl, struct Gadget *gadget, ListViewData *data, struct gpInput *input, short outside_ok)
{
	short line, x, y;

	// No entries?
	if (data->count < 1)
		return -1;

	// Get absolute mouse coordinates
	x = input->gpi_Mouse.X + gadget->LeftEdge;
	y = input->gpi_Mouse.Y + gadget->TopEdge;

	// Check mouse is over text area
	if (y < data->text_dims.Top || y >= data->text_dims.Top + data->text_dims.Height)
	{
		// Not allowed that?
		if (outside_ok != 2)
			return -1;
	}
	if (x < data->text_dims.Left || x >= data->text_dims.Left + data->text_dims.Width)
	{
		// Not allowed that?
		if (!outside_ok)
			return -1;
	}

	// Get the line we're over
	if (y < data->text_dims.Top)
		line = -1;
	else
	{
		line = SDivMod32(y - data->text_dims.Top, data->text_height);
		if (line > data->lines)
			line = data->lines;
	}
	line += data->top;

	// If no vertical scrolling and selection is over empty space, fail
	if (data->flags & LVF_NO_VERT_SCROLL && line >= data->count)
		return -1;

	// Bounds check
	if (line >= data->count)
		line = data->count - 1;
	if (line < 0)
		line = 0;

	// Return line number
	return (short)line;
}

// Get dimensions
void listview_get_dims(Class *cl, ListViewData *data)
{
	// Calculate list dimensions
	data->list_dims.Left = (data->flags & LVF_SCROLLER_LEFT) ? data->dims.Left + data->scroller_width : data->dims.Left;
	data->list_dims.Top = data->dims.Top;
	data->list_dims.Width = data->dims.Width - data->scroller_width;
	data->list_dims.Height = data->dims.Height;

	// Scroller dimensions
	data->scroller_dims.Left =
		(data->flags & LVF_SCROLLER_LEFT) ? data->dims.Left : data->list_dims.Left + data->list_dims.Width;
	data->scroller_dims.Top = data->dims.Top;
	data->scroller_dims.Width = data->scroller_width;
	data->scroller_dims.Height = data->dims.Height - (data->arrow_height << 1);

	// Get number of lines in lister
	data->lines = UDivMod32((data->list_dims.Height - 2), data->list_font->tf_YSize);

	// Calculate text area
	data->text_dims.Width = data->list_dims.Width - ((data->flags & LVF_THIN_BORDER) ? 2 : 4);
	data->text_dims.Height = (unsigned short)(data->lines) * data->list_font->tf_YSize;
	data->text_dims.Left = data->list_dims.Left + ((data->flags & LVF_THIN_BORDER) ? 1 : 2);
	if (data->flags & LVF_TOP_JUSTIFY)
	{
		data->text_dims.Top = data->list_dims.Top + 1;
	}
	else
	{
		data->text_dims.Top = data->list_dims.Top + ((data->list_dims.Height - data->text_dims.Height) >> 1);
	}

	// Get item width
	data->text_width = data->text_dims.Width - 4;
}

// Resize the listview
void listview_resize(Class *cl, struct Gadget *gadget, ListViewData *data, struct gpResize *size)
{
	struct Node *node;
	short item;

	// Get new position
	data->dims = size->gpr_Size;
	*((struct IBox *)&gadget->LeftEdge) = size->gpr_Size;

	// Get new dimensions
	listview_get_dims(cl, data);

	// Bounds check top
	if (data->top > data->count - data->lines)
		data->top = data->count - data->lines;
	if (data->top < 0)
		data->top = 0;

	// List empty?
	if (!data->labels || IsListEmpty(data->labels))
	{
		// Clear top item
		data->top_item = 0;
	}

	// Not empty
	else
	{
		// Find top item
		for (node = data->labels->lh_Head, item = 0; node->ln_Succ && item < data->top; node = node->ln_Succ, item++)
			;
		data->top_item = node;
	}

	// Reposition scroller
	if (data->scroller)
	{
		// Reposition
		data->scroller->LeftEdge = (data->flags & LVF_SCROLLER_LEFT)
									   ? data->dims.Left + 4
									   : data->dims.Left + data->dims.Width - data->scroller_width + 4;
		data->scroller->TopEdge = data->dims.Top + 2;
		data->scroller->Height = data->dims.Height - 4 - (data->arrow_height << 1);
	}

	// Reposition up arrow
	if (data->arrows[0])
	{
		data->arrows[0]->LeftEdge = data->scroller_dims.Left;
		data->arrows[0]->TopEdge = data->scroller_dims.Top + data->scroller_dims.Height;
	}

	// Reposition down arrow
	if (data->arrows[1])
	{
		data->arrows[1]->LeftEdge = data->scroller_dims.Left;
		data->arrows[1]->TopEdge = data->scroller_dims.Top + data->scroller_dims.Height + data->arrow_height;
	}
}

// local FilePart
char *lv_FilePart(char *name)
{
	char *ptr;

	// Get pointer to end of string
	ptr = name + strlen(name) - 1;

	// String too short?
	if (ptr == name)
		return ptr;

	// Skip over trailing path character
	if (*ptr == ':' || *ptr == '/')
		--ptr;

	// Go back until a : or / or start of string
	while (ptr >= name && *ptr != ':' && *ptr != '/')
		--ptr;

	// Gone too far?
	if (ptr < name)
		return name;

	// Found path character?
	if (*ptr == ':' || *ptr == '/')
		++ptr;

	return ptr;
}
