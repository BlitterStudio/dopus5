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

static BOOL button_has_custom_backfill(struct gpRender *render)
{
	struct Window *window;

	if (!render || !render->gpr_GInfo || !(window = render->gpr_GInfo->gi_Window) || !window->WLayer)
		return FALSE;

	return (window->WLayer->BackFill && window->WLayer->BackFill != LAYERS_BACKFILL);
}

// Button dispatcher
IPTR LIBFUNC button_dispatch(REG(a0, Class *cl), REG(a2, Object *obj), REG(a1, Msg msg))
{
	struct Gadget *gadget = 0;
	ButtonData *data = 0;
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
		struct TagItem *tags, string_tags[6];
		char *buffer = 0;
		short place;
		long style;

		// Get tag list
		tags = ((struct opSet *)msg)->ops_AttrList;

		// String gadget?
		if (cl->cl_UserData == CLASS_STRINGGAD)
		{
			short bufsize = 42;
			struct TagItem *tag;
			short got_buffers = 0;

			// Find what buffers we have
			if (FindTagItem(STRINGA_Buffer, tags))
				got_buffers |= GOTBUF_MAIN;
			if (FindTagItem(STRINGA_UndoBuffer, tags))
				got_buffers |= GOTBUF_UNDO;
			if (FindTagItem(STRINGA_WorkBuffer, tags))
				got_buffers |= GOTBUF_WORK;

			// Not all buffers supplied?
			if (got_buffers != GOTBUF_ALL)
			{
				// Use supplied buffer size exactly?
				if (got_buffers && (tag = FindTagItem(STRINGA_MaxChars, tags)))
					bufsize = tag->ti_Data;

				// Find buffer size needed
				else if ((tag = FindTagItem(GTST_MaxChars, tags)) || (tag = FindTagItem(GTIN_MaxChars, tags)) ||
						 (tag = FindTagItem(STRINGA_MaxChars, tags)))
					bufsize = tag->ti_Data + 1;

				// Allocate buffer
				if (!(buffer = AllocVec((bufsize + 1) * 3, MEMF_CLEAR)))
					break;

				// Fill out tags to set buffer and stuff
				string_tags[0].ti_Tag = STRINGA_MaxChars;
				string_tags[0].ti_Data = bufsize;
				string_tags[1].ti_Tag = (got_buffers & GOTBUF_MAIN) ? TAG_IGNORE : STRINGA_Buffer;
				string_tags[1].ti_Data = (IPTR)buffer;
				string_tags[2].ti_Tag = (got_buffers & GOTBUF_UNDO) ? TAG_IGNORE : STRINGA_UndoBuffer;
				string_tags[2].ti_Data = (IPTR)buffer + bufsize + 1;
				string_tags[3].ti_Tag = (got_buffers & GOTBUF_WORK) ? TAG_IGNORE : STRINGA_WorkBuffer;
				string_tags[3].ti_Data = (IPTR)buffer + bufsize + bufsize + 2;
				string_tags[4].ti_Tag = GA_TabCycle;
				string_tags[4].ti_Data = TRUE;
				string_tags[5].ti_Tag = TAG_MORE;
				string_tags[5].ti_Data = (IPTR)tags;

				// Set using string tags
				tags = string_tags;
			}
		}

		// Create super gadget
		if (!(retval = DoSuperMethod(cl, obj, OM_NEW, tags, 0)))
		{
			// Free buffer if we had one
			FreeVec(buffer);
			break;
		}
		gadget = (struct Gadget *)(IPTR)retval;
		data = INST_DATA(cl, (Object *)(IPTR)retval);

		// Get taglist
		tags = ((struct opSet *)msg)->ops_AttrList;

		// String gadget?
		if (cl->cl_UserData == CLASS_STRINGGAD)
		{
			// Set flag
			data->flags |= BUTTONF_STRING;

			// Store border dimensions
			((StringData *)data)->border = *((struct IBox *)&gadget->LeftEdge);

			// Save buffer pointer
			((StringData *)data)->buffer = buffer;

			// Get font if supplied
			((StringData *)data)->string_font = (struct TextFont *)GetTagData(STRINGA_Font, 0, tags);

			// Get change task and bit
			((StringData *)data)->change_task = (struct Task *)GetTagData(GTCustom_ChangeSigTask, 0, tags);
			((StringData *)data)->change_bit = GetTagData(GTCustom_ChangeSigBit, 0, tags);

			// Position cursor at end of string
			((struct StringInfo *)gadget->SpecialInfo)->BufferPos =
				strlen(((struct StringInfo *)gadget->SpecialInfo)->Buffer);
		}

		// Check gadget?
		else if (cl->cl_UserData == CLASS_CHECKGAD)
		{
			// Set flag
			data->flags |= BUTTONF_CHECK;

			// Get image
			((CheckData *)data)->check_image =
				(struct Image *)NewObject(0,
										  "dopusiclass",
										  DIA_Type,
										  IM_CHECK,
										  IA_Width,
										  13,  // (gadget->Height>13)?gadget->Height-6:gadget->Height-4,
										  IA_Height,
										  gadget->Height - 5,  // (gadget->Height>13)?gadget->Height-6:gadget->Height-4,
										  TAG_END);

			// Set toggle-select flag
			gadget->Activation |= GACT_TOGGLESELECT;
		}

		// Number view?
		else if (cl->cl_UserData == CLASS_VIEW)
		{
			// Set flag
			data->flags |= BUTTONF_VIEW;
		}

		// Frame?
		else if (cl->cl_UserData == CLASS_FRAME)
		{
			// Set flag
			data->flags |= BUTTONF_FRAME;

			// Get flags
			data->value = GetTagData(GTCustom_FrameFlags, 0, tags);
		}

		// Gauge?
		else if (cl->cl_UserData == CLASS_GAUGE)
		{
			// Set flag
			data->flags |= BUTTONF_GAUGE;

			// Get flags
			data->value = GetTagData(DGG_Flags, 0, tags);

			// Initial pens
			((GaugeData *)data)->fillpen = -1;
			((GaugeData *)data)->fillpen_alert = -1;
		}

		// Get font to use
		if ((attr = (struct TextAttr *)GetTagData(GTCustom_TextAttr, 0, tags)))
		{
			data->font = OpenFont(attr);
		}

		// Flags
		if (GetTagData(GTCustom_Borderless, 0, tags))
			data->flags |= BUTTONF_NOBORDER;
		else if (GetTagData(GTCustom_ThinBorders, 0, tags))
			data->flags |= BUTTONF_THIN;
		if (GetTagData(GTCustom_Bold, 0, tags))
			data->flags |= BUTTONF_BOLD;
		if (GetTagData(GTCustom_NoGhost, 0, tags))
			data->flags |= BUTTONF_NO_GHOST;
		if (GetTagData(GTCustom_Recessed, 0, tags))
			data->flags |= BUTTONF_RECESSED;

		// Style
		if ((style = GetTagData(GTCustom_Style, 0, tags)))
		{
			// Bold?
			if (style & FSF_BOLD)
				data->flags |= BUTTONF_BOLD;

			// Italics?
			if (style & FSF_ITALIC)
				data->flags |= BUTTONF_ITALICS;
		}

		// Text placement
		if ((place = GetTagData(GTCustom_TextPlacement, 0, tags)))
			data->place = place;

		// Check box?
		else if (data->flags & BUTTONF_CHECK)
			data->place = PLACETEXT_RIGHT;

		// String?
		else if (data->flags & BUTTONF_STRING)
			data->place = PLACETEXT_LEFT;

		// View?
		else if (data->flags & BUTTONF_VIEW)
			data->place = PLACETEXT_LEFT;

		// Default
		else
			data->place = PLACETEXT_IN;

		// Justification
		data->justify = GetTagData(GTCustom_Justify, JUSTIFY_CENTER, tags);

		// Get title
		if (gadget->GadgetText && gadget->Flags & GFLG_LABELSTRING)
		{
			char *ptr;
			short pos = 0;

			// Get pointer to title
			ptr = (char *)gadget->GadgetText;
			data->title_uscore = -1;

			// Copy title string, look for underscore
			while (*ptr && pos < 79)
			{
				// Underscore?
				if (!(data->flags & BUTTONF_FRAME) && *ptr == '_')
				{
					// Remember position, skip over it
					data->title_uscore = pos;
				}

				// Otherwise, store
				else
					data->title[pos++] = *ptr;
				++ptr;
			}

			// Clear gadget text pointer
			gadget->GadgetText = 0;
		}
	}

	// Look at tags
	case OM_SET:

		// String gadget?
		if (data && data->flags & (BUTTONF_STRING | BUTTONF_VIEW))
		{
			struct TagItem *tag;
			BOOL redraw = 0, handle = 1;

			// Disabled?
			if ((tag = FindTagItem(GA_Disabled, ((struct opSet *)msg)->ops_AttrList)))
			{
				// Check state has changed
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

				// Handled ourselves
				handle = 0;

				// No ghosting?
				if (data->flags & BUTTONF_NO_GHOST)
					redraw = 0;
			}

			// Set string?
			if ((tag = FindTagItem(STRINGA_TextVal, ((struct opSet *)msg)->ops_AttrList)) ||
				(tag = FindTagItem(GTST_String, ((struct opSet *)msg)->ops_AttrList)) ||
				(tag = FindTagItem(GTTX_Text, ((struct opSet *)msg)->ops_AttrList)) ||
				(tag = FindTagItem(GTIN_Number, ((struct opSet *)msg)->ops_AttrList)) ||
				(tag = FindTagItem(GTNM_Number, ((struct opSet *)msg)->ops_AttrList)))
			{
				// Number?
				if (tag->ti_Tag == GTIN_Number)
				{
					// Build number string
					lsprintf(((struct StringInfo *)gadget->SpecialInfo)->Buffer, "%ld", tag->ti_Data);
				}

				// View number?
				else if (tag->ti_Tag == GTNM_Number)
				{
					// Store value
					data->value = tag->ti_Data;
					data->flags |= BUTTONF_VIEW_NUM;
				}

				// String?
				else if (tag->ti_Tag == GTTX_Text)
				{
					// Free existing text
					FreeVec(data->data);

					// Allocate new text
					if ((data->data = AllocVec(strlen((char *)tag->ti_Data) + 1, 0)))
						strcpy(data->data, (char *)tag->ti_Data);
				}

				// Store string value
				else if (tag->ti_Data)
					stccpy(((struct StringInfo *)gadget->SpecialInfo)->Buffer,
						   (char *)tag->ti_Data,
						   ((struct StringInfo *)gadget->SpecialInfo)->MaxChars);

				// Clear buffer
				else
					*((struct StringInfo *)gadget->SpecialInfo)->Buffer = 0;

				// Trigger redraw
				redraw = 1;
				handle = 0;
			}

			// Let it handle its own sets?
			if (msg->MethodID == OM_SET && handle)
				retval = DoSuperMethodA(cl, obj, msg);

			// Redraw needed?
			if (redraw)
			{
				struct RastPort *rp;

				// Get rastport
				if ((rp = ObtainGIRPort(((struct opSet *)msg)->ops_GInfo)))
				{
					// Send redraw
					DoMethod(obj, GM_RENDER, ((struct opSet *)msg)->ops_GInfo, rp, GREDRAW_REDRAW);
					ReleaseGIRPort(rp);
				}
			}
		}

		// Gauge gadget
		else if (data && data->flags & BUTTONF_GAUGE)
		{
			struct TagItem *tag;
			BOOL redraw = 0;

			// New total
			if ((tag = FindTagItem(DGG_Total, ((struct opSet *)msg)->ops_AttrList)))
			{
				// Set new total, trigger redraw
				((GaugeData *)data)->total = tag->ti_Data;
				redraw = 1;
			}

			// New free
			if ((tag = FindTagItem(DGG_Free, ((struct opSet *)msg)->ops_AttrList)))
			{
				// Set new free, trigger redraw
				((GaugeData *)data)->free = tag->ti_Data;
				redraw = 1;
			}

			// New fill pen
			if ((tag = FindTagItem(DGG_FillPen, ((struct opSet *)msg)->ops_AttrList)))
			{
				// Set new pen, trigger redraw
				if (((GaugeData *)data)->fillpen != tag->ti_Data)
				{
					((GaugeData *)data)->fillpen = tag->ti_Data;
					redraw = 1;
				}
			}

			// New fill alert pen
			if ((tag = FindTagItem(DGG_FillPenAlert, ((struct opSet *)msg)->ops_AttrList)))
			{
				// Set new pen, trigger redraw
				if (((GaugeData *)data)->fillpen_alert != tag->ti_Data)
				{
					((GaugeData *)data)->fillpen_alert = tag->ti_Data;
					redraw = 1;
				}
			}

			// Need a redraw?
			if (msg->MethodID == OM_SET && redraw)
			{
				struct RastPort *rp;

				// Get rastport
				if ((rp = ObtainGIRPort(((struct opSet *)msg)->ops_GInfo)))
				{
					// Send redraw
					DoMethod(obj, GM_RENDER, ((struct opSet *)msg)->ops_GInfo, rp, GREDRAW_REDRAW);
					ReleaseGIRPort(rp);
				}
			}
		}

		// Button gadget
		else
		{
			struct TagItem *tags, *tag;
			BOOL redraw = 0;

			// Get taglist
			tags = ((struct opSet *)msg)->ops_AttrList;

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

				// No ghosting?
				if (data->flags & BUTTONF_NO_GHOST)
					redraw = 0;
			}

			// Checkbox
			if (data->flags & BUTTONF_CHECK)
			{
				// Set state?
				if ((tag = FindTagItem(GTCB_Checked, tags)))
				{
					// Check state has changed
					if (tag->ti_Data && !(gadget->Flags & GFLG_SELECTED))
					{
						gadget->Flags |= GFLG_SELECTED;
						redraw = 1;
					}
					else if (!tag->ti_Data && gadget->Flags & GFLG_SELECTED)
					{
						gadget->Flags &= ~GFLG_SELECTED;
						redraw = 1;
					}
				}
			}

			// Need a redraw?
			if (msg->MethodID == OM_SET && redraw)
			{
				struct RastPort *rp;

				// Get rastport
				if ((rp = ObtainGIRPort(((struct opSet *)msg)->ops_GInfo)))
				{
					// Send redraw
					DoMethod(obj, GM_RENDER, ((struct opSet *)msg)->ops_GInfo, rp, GREDRAW_REDRAW);
					ReleaseGIRPort(rp);
				}
			}
		}
		break;

	// Render gadget
	case GM_RENDER:

		// String gadget?
		if (data->flags & BUTTONF_STRING)
		{
			// First render?
			if (!(data->flags & BUTTONF_SHRUNK))
			{
				// Shrink gadget
				button_field_shrink((StringData *)data,
									gadget,
									((struct gpRender *)msg)->gpr_GInfo->gi_Window,
									((struct gpRender *)msg)->gpr_GInfo->gi_Requester);
				data->flags |= BUTTONF_SHRUNK;
			}
		}

		// Render border (and button for button gadgets)
		button_render(cl, gadget, data, (struct gpRender *)msg);

		// Let string gadget handle itself
		if (data->flags & BUTTONF_STRING)
		{
			unsigned short flags;

			// Save gadget flags
			flags = gadget->Flags;

			// Disabled, but no ghosting?
			if (gadget->Flags & GFLG_DISABLED && data->flags & BUTTONF_NO_GHOST)
				gadget->Flags &= ~GFLG_DISABLED;

			// Do gadget rendering
			retval = DoSuperMethodA(cl, obj, msg);

			// Restore flags
			gadget->Flags = flags;
		}
		break;

	// Handle input
	case GM_GOACTIVE:

		// Can't activate view, frame or gauge gadgets
		if (data->flags & (BUTTONF_VIEW | BUTTONF_FRAME | BUTTONF_GAUGE))
		{
			retval = GMR_NOREUSE;
			break;
		}

	case GM_HANDLEINPUT:

		// Let string gadget handle itself
		if (data->flags & BUTTONF_STRING)
		{
#ifdef __amigaos4__
			// OS4 strgclass swallows Escape — intercept before super
			struct gpInput *input = (struct gpInput *)msg;
			if (input->gpi_IEvent &&
				input->gpi_IEvent->ie_Class == IECLASS_RAWKEY &&
				input->gpi_IEvent->ie_Code == 0x45)
			{
				*input->gpi_Termination = 0x45;
				retval = GMR_NOREUSE | GMR_VERIFY;
				break;
			}
#endif
			retval = DoSuperMethodA(cl, obj, msg);
		}

		// Button
		else
		{
			struct gpInput *input;
			BOOL redraw = 0;

			// Get input pointer
			input = (struct gpInput *)msg;

			// Default is not to activate
			retval = GMR_NOREUSE;

			// Valid input event?
			if (input->gpi_IEvent)
			{
				// Button active?
				if (msg->MethodID == GM_GOACTIVE || input->gpi_IEvent->ie_Code != SELECTUP)
				{
					// Ok to activate
					retval = GMR_MEACTIVE;

					// Just gone active?
					if (msg->MethodID == GM_GOACTIVE)
					{
						// Save old selection state
						if (gadget->Flags & GFLG_SELECTED)
							data->flags |= BUTTONF_SEL;
						else
							data->flags &= ~BUTTONF_SEL;

						// Start action
						data->flags |= BUTTONF_INSIDE;
						gadget->Flags ^= GFLG_SELECTED;
						redraw = 1;
					}

					// Is mouse over button?
					if (input->gpi_Mouse.X >= 0 && input->gpi_Mouse.Y >= 0 && input->gpi_Mouse.X < gadget->Width &&
						input->gpi_Mouse.Y < gadget->Height)
					{
						// If not inside before, toggle selection
						if (!(data->flags & BUTTONF_INSIDE))
						{
							data->flags |= BUTTONF_INSIDE;
							gadget->Flags ^= GFLG_SELECTED;
							redraw = 1;
						}
					}

					// If inside before, toggle selection
					else if (data->flags & BUTTONF_INSIDE)
					{
						data->flags &= ~BUTTONF_INSIDE;
						gadget->Flags ^= GFLG_SELECTED;
						redraw = 1;
					}

					// Immediate response?
					if (gadget->Activation & GACT_IMMEDIATE)
						retval |= GMR_VERIFY;
				}

				// Button is no longer active
				else
				{
					// Has selection state changed?
					if ((gadget->Flags & GFLG_SELECTED && !(data->flags & BUTTONF_SEL)) ||
						(!(gadget->Flags & GFLG_SELECTED) && data->flags & BUTTONF_SEL))
					{
						// Button selected!
						retval |= GMR_VERIFY;
					}

					// Deselect non-toggleselect buttons
					if (gadget->Flags & GFLG_SELECTED && !(gadget->Activation & GACT_TOGGLESELECT))
					{
						gadget->Flags &= ~GFLG_SELECTED;
						redraw = 1;
					}
				}
			}

			// Need a redraw?
			if (redraw)
			{
				struct RastPort *rp;

				// Get rastport
				if ((rp = ObtainGIRPort(input->gpi_GInfo)))
				{
					// Send redraw
					DoMethod(obj, GM_RENDER, input->gpi_GInfo, rp, GREDRAW_REDRAW);
					ReleaseGIRPort(rp);
				}
			}
		}
		break;

	// Go in-active
	case GM_GOINACTIVE:

		// Pass it to super class
		retval = DoSuperMethodA(cl, obj, msg);

		// String gadget?
		if (data->flags & BUTTONF_STRING)
		{
			// Did Intuition abort this?
			if (((struct gpGoInactive *)msg)->gpgi_Abort)
			{
				// Task to signal?
				if (((StringData *)data)->change_task)
				{
					// Signal it
					Signal(((StringData *)data)->change_task, 1 << ((StringData *)data)->change_bit);
				}
			}
		}
		break;

	// Resize
	case GM_RESIZE:

		// Handle the resize
		button_resize(cl, gadget, data, (struct gpResize *)msg);
		break;

	// Dispose
	case OM_DISPOSE:

		// Free stuff
		if (data->font)
			CloseFont(data->font);

		// String gadget?
		if (data->flags & BUTTONF_STRING)
		{
			// Free our buffer
			FreeVec(((StringData *)data)->buffer);
		}

		// Check gadget?
		else if (data->flags & BUTTONF_CHECK)
		{
			// Free image
			DisposeObject((Object *)((CheckData *)data)->check_image);
		}

		// View?
		else if (data->flags & BUTTONF_VIEW)
		{
			// Free data
			FreeVec(data->data);
		}

	// Unknown method
	default:
		retval = DoSuperMethodA(cl, obj, msg);
		break;
	}

	return retval;
}

#ifdef __AROS__
AROS_UFH3(IPTR,
		  button_dispatch_entry,
		  AROS_UFHA(Class *, cl, A0),
		  AROS_UFHA(Object *, obj, A2),
		  AROS_UFHA(Msg, msg, A1))
{
	AROS_USERFUNC_INIT
	return button_dispatch(cl, obj, msg);
	AROS_USERFUNC_EXIT
}
#endif

// Render button
void button_render(Class *cl, struct Gadget *gadget, ButtonData *data, struct gpRender *render)
{
	struct RastPort *rp;
	struct Image *image = 0;
	struct IBox box, gadget_box;
	UWORD *pens;

	// Get gadget position
	if (data->flags & BUTTONF_STRING)
		box = ((StringData *)data)->border;
	else
		box = *((struct IBox *)&gadget->LeftEdge);

	// Get real gadget position
	gadget_box = *((struct IBox *)&gadget->LeftEdge);

	// Adjust for relative position and size
	if (gadget->Flags & GFLG_RELRIGHT)
	{
		// Requester?
		if (render->gpr_GInfo->gi_Requester)
		{
			box.Left += render->gpr_GInfo->gi_Requester->Width;
			gadget_box.Left += render->gpr_GInfo->gi_Requester->Width;
		}
		else
		{
			box.Left += render->gpr_GInfo->gi_Window->Width;
			gadget_box.Left += render->gpr_GInfo->gi_Window->Width;
		}
	}
	if (gadget->Flags & GFLG_RELBOTTOM)
	{
		// Requester?
		if (render->gpr_GInfo->gi_Requester)
		{
			box.Top += render->gpr_GInfo->gi_Requester->Height;
			gadget_box.Top += render->gpr_GInfo->gi_Requester->Height;
		}
		else
		{
			box.Top += render->gpr_GInfo->gi_Window->Height;
			gadget_box.Top += render->gpr_GInfo->gi_Window->Height;
		}
	}
	if (gadget->Flags & GFLG_RELWIDTH)
	{
		// Requester?
		if (render->gpr_GInfo->gi_Requester)
		{
			box.Width += render->gpr_GInfo->gi_Requester->Width;
			gadget_box.Width += render->gpr_GInfo->gi_Requester->Width;
		}
		else
		{
			box.Width += render->gpr_GInfo->gi_Window->Width;
			gadget_box.Width += render->gpr_GInfo->gi_Window->Width;
		}
	}
	if (gadget->Flags & GFLG_RELHEIGHT)
	{
		// Requester?
		if (render->gpr_GInfo->gi_Requester)
		{
			box.Height += render->gpr_GInfo->gi_Requester->Height;
			gadget_box.Height += render->gpr_GInfo->gi_Requester->Height;
		}
		else
		{
			box.Height += render->gpr_GInfo->gi_Window->Height;
			gadget_box.Height += render->gpr_GInfo->gi_Window->Height;
		}
	}

	// Get rastport to use
	rp = render->gpr_RPort;
	SetDrMd(rp, JAM1);

	// Image in the border?
	if (gadget->Flags & GFLG_GADGIMAGE && gadget->Activation & GACT_TOPBORDER)
	{
		long state;

		// Get state; is window active?
		if (render->gpr_GInfo->gi_Window->Flags & WFLG_WINDOWACTIVE)
		{
			// Selected, busy or normal?
			if (gadget->Flags & GFLG_SELECTED)
				state = IDS_SELECTED;
			else if (gadget->Flags & GFLG_DISABLED)
				state = IDS_DISABLED;
			else
				state = IDS_NORMAL;
		}

		// Inactive
		else
		{
			// Selected, busy or normal?
			if (gadget->Flags & GFLG_SELECTED)
				state = IDS_INACTIVESELECTED;
			else if (gadget->Flags & GFLG_DISABLED)
				state = IDS_INACTIVEDISABLED;
			else
				state = IDS_INACTIVENORMAL;
		}

		// Draw image
		DrawImageState(rp,
					   (struct Image *)gadget->GadgetRender,
					   gadget_box.Left,
					   gadget_box.Top,
					   state,
					   render->gpr_GInfo->gi_DrInfo);
		return;
	}

	// Get pen array
	pens = render->gpr_GInfo->gi_DrInfo->dri_Pens;

	// String gadgets have the border around them
	if (data->flags & BUTTONF_STRING)
	{
		struct Rectangle rect;
		struct Rectangle interior;

		// Convert box to rectangle
		rect.MinX = box.Left;
		rect.MinY = box.Top;
		rect.MaxX = box.Left + box.Width - 1;
		rect.MaxY = box.Top + box.Height - 1;

		// Copy to interior
		interior = rect;

		// Thin border?
		if (data->flags & BUTTONF_THIN)
		{
			// Normal string box
			draw_field_box(rp, &rect, render->gpr_GInfo->gi_DrInfo, (struct Library *)GfxBase);

			// Calculate interior
			interior.MinX += 2;
			interior.MinY += 2;
			interior.MaxX -= 2;
			interior.MaxY -= 2;
		}

		// Thick
		else if (!(data->flags & BUTTONF_NOBORDER))
		{
			// Do shine lines
			SetAPen(rp, pens[SHINEPEN]);
			Move(rp, rect.MinX, rect.MaxY);
			Draw(rp, rect.MinX, rect.MinY);
			Move(rp, rect.MinX + 1, rect.MaxY - 1);
			Draw(rp, rect.MinX + 1, rect.MinY);
			Draw(rp, rect.MaxX - 1, rect.MinY);
			Move(rp, rect.MaxX - 2, rect.MinY);
			Draw(rp, rect.MaxX - 2, rect.MaxY - 1);
			Move(rp, rect.MaxX - 3, rect.MinY + 2);
			Draw(rp, rect.MaxX - 3, rect.MaxY - 1);
			Draw(rp, rect.MinX + 3, rect.MaxY - 1);

			// Do shadow lines
			SetAPen(rp, pens[SHADOWPEN]);
			Move(rp, rect.MaxX, rect.MinY);
			Draw(rp, rect.MaxX, rect.MaxY);
			Move(rp, rect.MaxX - 1, rect.MinY + 1);
			Draw(rp, rect.MaxX - 1, rect.MaxY);
			Draw(rp, rect.MinX + 1, rect.MaxY);
			Move(rp, rect.MinX + 2, rect.MaxY);
			Draw(rp, rect.MinX + 2, rect.MinY + 1);
			Move(rp, rect.MinX + 3, rect.MaxY - 2);
			Draw(rp, rect.MinX + 3, rect.MinY + 1);
			Draw(rp, rect.MaxX - 3, rect.MinY + 1);

			// Calculate interior
			interior.MinX += 4;
			interior.MinY += 2;
			interior.MaxX -= 4;
			interior.MaxY -= 2;
		}

		// Set pen to fill background
		SetAPen(rp,
				(gadget->Flags & GFLG_SELECTED) ? ((struct StringInfo *)gadget->SpecialInfo)->Extension->ActivePens[1]
												: ((struct StringInfo *)gadget->SpecialInfo)->Extension->Pens[1]);

		// Fill left-side?
		if (interior.MinX < gadget_box.Left)
			RectFill(rp, interior.MinX, interior.MinY, gadget_box.Left - 1, interior.MaxY);

		// Fill right-side?
		if (interior.MaxX >= gadget_box.Left + gadget_box.Width)
			RectFill(rp, gadget_box.Left + gadget_box.Width, interior.MinY, interior.MaxX, interior.MaxY);

		// Fill top?
		if (interior.MinY < gadget_box.Top)
			RectFill(rp, interior.MinX, interior.MinY, interior.MaxX, gadget_box.Top - 1);

		// Fill bottom?
		if (interior.MaxY >= gadget_box.Top + gadget_box.Height)
			RectFill(rp, interior.MinX, gadget_box.Top + gadget_box.Height, interior.MaxX, interior.MaxY);
	}

	// Frame
	else if (data->flags & BUTTONF_FRAME)
	{
		// Want a border?
		if (!(data->flags & BUTTONF_NOBORDER))
		{
			// Simple frame to start with
			listview_border(cl,
							rp,
							pens,
							&box,
							(data->value & AREAFLAG_RECESSED) ? IDS_SELECTED : IDS_NORMAL,
							(data->flags & BUTTONF_THIN) ? THIN : THICK);
		}

		// Shift box in
		box.Left += (data->flags & BUTTONF_THIN) ? 1 : 2;
		box.Top++;
		box.Width -= (data->flags & BUTTONF_THIN) ? 2 : 4;
		box.Height -= 2;

		// Fill with background colour?
		if (!(data->flags & BUTTONF_NOBORDER))
		{
			// Fill interior of frame
			SetAPen(rp, pens[BACKGROUNDPEN]);
			RectFill(rp, box.Left, box.Top, box.Left + box.Width - 1, box.Top + box.Height - 1);
		}

		// If borderless then fill with hook
		else
		{
			// Fill interior of frame
			EraseRect(rp, box.Left, box.Top, box.Left + box.Width - 1, box.Top + box.Height - 1);
		}
	}

	// Gauge
	else if (data->flags & BUTTONF_GAUGE)
	{
		GaugeData *gauge;
		short bgpen, height, pcent;

		// Get background pen depending on window activity
		bgpen = (render->gpr_GInfo->gi_Window->Flags & WFLG_WINDOWACTIVE) ? pens[FILLPEN] : pens[BACKGROUNDPEN];

		// Get pointer to gauge data
		gauge = (GaugeData *)data;

		// Draw lines down the two sides
		SetAPen(rp, pens[SHINEPEN]);
		Move(rp, box.Left, box.Top);
		Draw(rp, box.Left, box.Top + box.Height - 1);
		SetAPen(rp, pens[SHADOWPEN]);
		Move(rp, box.Left + box.Width - 1, box.Top);
		Draw(rp, box.Left + box.Width - 1, box.Top + box.Height - 2);

		// Fill interior
		SetAPen(rp, bgpen);
		RectFill(rp, box.Left + 1, box.Top, box.Left + 3, box.Top + box.Height - 2);
		RectFill(rp, box.Left + box.Width - 4, box.Top, box.Left + box.Width - 2, box.Top + box.Height - 2);
		RectFill(rp, box.Left + 4, box.Top, box.Width - 5, box.Top + 1);
		RectFill(rp, box.Left + 4, box.Top + box.Height - 3, box.Width - 5, box.Top + box.Height - 2);

		// Shift box in to get interior
		box.Left += 4;
		box.Top += 2;
		box.Width -= 8;
		box.Height -= 4;

		// Draw interior box
		listview_border(cl, rp, pens, &box, IDS_SELECTED, THIN);

		// Shift box in again
		box.Left++;
		box.Top++;
		box.Width -= 2;
		box.Height -= 2;

		// Get percentage free
		pcent = L_CalcPercent(gauge->free, gauge->total, UtilityBase);

		// Get height of selected area
		height = UDivMod32(box.Height * pcent, 100);

		// Bounds-check height
		if (height > box.Height)
			height = box.Height;

		// Set fill pen again
		SetAPen(rp, bgpen);

		// Fill to marker box
		if (height < box.Height)
			RectFill(rp, box.Left, box.Top, box.Left + box.Width - 1, box.Top + box.Height - height - 1);

		// Any marker box?
		if (height > 0)
		{
			// Get size of marker box
			box.Top += box.Height - height;
			box.Height = height;

			// Fill interior of marker box
			if (height > 2)
			{
				// Get background pen if active and set
				if (render->gpr_GInfo->gi_Window->Flags & WFLG_WINDOWACTIVE)
				{
					// Over 90%?
					if (pcent >= 90 && gauge->fillpen_alert > -1)
						SetAPen(rp, gauge->fillpen_alert);

					// Standard pen set?
					else if (gauge->fillpen > -1)
						SetAPen(rp, gauge->fillpen);
				}

				// Fill box
				RectFill(rp, box.Left + 1, box.Top + 1, box.Left + box.Width - 2, box.Top + box.Height - 2);
			}

			// Draw border of marker box
			listview_border(cl, rp, pens, &box, IDS_NORMAL, THIN);
		}
	}

	// Button gadget
	else
	{
		long state = IDS_NORMAL;

		// Selected border?
		if (gadget->Flags & GFLG_SELECTED && !(data->flags & BUTTONF_CHECK))
			state = IDS_SELECTED;
		else if (data->flags & BUTTONF_VIEW)
			state = IDS_SELECTED;

		// Recessed?
		if (data->flags & BUTTONF_RECESSED)
		{
			// Toggle state
			state = (state == IDS_NORMAL) ? IDS_SELECTED : IDS_NORMAL;
		}

		// Want a border?
		if (!(data->flags & BUTTONF_NOBORDER))
		{
			// Draw border around the button
			listview_border(cl, rp, pens, &box, state, (data->flags & BUTTONF_THIN) ? THIN : THICK);

			// Fill button interior
			SetAPen(rp,
					pens[(gadget->Flags & GFLG_SELECTED && !(data->flags & BUTTONF_CHECK)) ? FILLPEN : BACKGROUNDPEN]);
			RectFill(rp,
					 box.Left + ((data->flags & BUTTONF_THIN) ? 1 : 2),
					 box.Top + 1,
					 box.Left + box.Width - ((data->flags & BUTTONF_THIN) ? 2 : 3),
					 box.Top + box.Height - 2);
		}

		// Borderless means we fill with EraseRect() rather than colour 0
		else
		{
			EraseRect(rp,
					  box.Left + ((data->flags & BUTTONF_THIN) ? 1 : 2),
					  box.Top + 1,
					  box.Left + box.Width - ((data->flags & BUTTONF_THIN) ? 2 : 3),
					  box.Top + box.Height - 2);
		}

		// Selected check gadget?
		if (data->flags & BUTTONF_CHECK && gadget->Flags & GFLG_SELECTED)
		{
			struct Image *image;

			// Got an image?
			if ((image = ((CheckData *)data)->check_image))
			{
				short x, y;

				// Get image position
				x = box.Left + ((box.Width - image->Width) >> 1);
				y = box.Top + ((box.Height - image->Height) >> 1);

				// Draw the image
				DrawImageState(rp, image, x, y, IDS_NORMAL, render->gpr_GInfo->gi_DrInfo);
			}
		}

		// View gadget?
		else if (data->flags & BUTTONF_VIEW)
		{
			char buf[20], *ptr;

			// Number to view?
			if (data->flags & BUTTONF_VIEW_NUM)
			{
				// Build string
				lsprintf(buf, "%ld", data->value);
				ptr = buf;
			}

			// Or supplied text?
			else if (data->data)
				ptr = data->data;

			// Supplied string
			else
				ptr = (char *)data->value;

			// Valid string?
			if (ptr && *ptr)
			{
				struct TextExtent extent;
				short len;

				// Set font if we have it
				if (data->font)
					SetFont(rp, data->font);

				// Get length of string
				if ((len = TextFit(rp, ptr, strlen(ptr), &extent, 0, 1, box.Width - 4, rp->TxHeight)))
				{
					short x;

					// Left?
					if (data->justify == JUSTIFY_LEFT)
						x = box.Left + 2;

					// Right?
					else if (data->justify == JUSTIFY_RIGHT)
						x = box.Left + box.Width - extent.te_Width - 3;

					// Center
					else
						x = box.Left + ((box.Width - extent.te_Width) >> 1);

					// Draw text
					SetAPen(rp, pens[TEXTPEN]);
					Move(rp, x, box.Top + ((box.Height - extent.te_Height) >> 1) + rp->TxBaseline);
					Text(rp, ptr, len);
				}
			}
		}
	}

	// Gadget text
	if (data->title[0])
	{
		short len, str_len;
		short x, y;
		UBYTE old_style;
		BOOL custom_backfill_text;

		// Remember old style
		old_style = rp->AlgoStyle;
		// Outside labels should not paint BACKGROUNDPEN over requester backfills.
		custom_backfill_text = (data->place != PLACETEXT_IN && button_has_custom_backfill(render));

		// Set pen for text
		if (data->place == PLACETEXT_IN)
		{
			SetAPen(rp, pens[(gadget->Flags & GFLG_SELECTED) ? FILLTEXTPEN : TEXTPEN]);
			SetBPen(rp, pens[(gadget->Flags & GFLG_SELECTED) ? FILLPEN : BACKGROUNDPEN]);
		}
		else
		{
			SetAPen(rp, pens[TEXTPEN]);
			SetBPen(rp, pens[BACKGROUNDPEN]);
		}
		SetDrMd(rp, (custom_backfill_text) ? JAM1 : JAM2);
		if (data->font)
			SetFont(rp, data->font);
		if (data->flags & BUTTONF_BOLD)
			SetSoftStyle(rp, FSF_BOLD, FSF_BOLD);
		if (data->flags & BUTTONF_ITALICS)
			SetSoftStyle(rp, FSF_ITALIC, FSF_ITALIC);

		// Get text length
		str_len = strlen(data->title);
		len = TextLength(rp, data->title, str_len);

		// Default y coordinate
		y = box.Top + ((box.Height - rp->TxHeight) >> 1) + rp->TxBaseline;

		// Look at placement
		switch (data->place)
		{
		// Inside
		case PLACETEXT_IN:
			x = box.Left + ((box.Width - len) >> 1);
			break;

		// Right
		case PLACETEXT_RIGHT:
			x = box.Left + box.Width + 7;
			break;

		// Above
		case PLACETEXT_ABOVE:
			x = box.Left;
			y = box.Top - 2 - rp->TxHeight + rp->TxBaseline;
			break;

		// Left (default)
		default:
			x = box.Left - len - 8;
			break;
		}

		// Draw text
		if (custom_backfill_text)
			EraseRect(rp, x, y - rp->TxBaseline, x + len - 1, y - rp->TxBaseline + rp->TxHeight - 1);
		Move(rp, x, y);
		Text(rp, data->title, str_len);

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

		// Restore JAM1 draw mode
		SetDrMd(rp, JAM1);

		// Restore style
		if (old_style != rp->AlgoStyle)
			SetSoftStyle(rp, old_style, FSF_BOLD | FSF_ITALIC);
	}

	// Stuff for buttons only
	if (!(data->flags & BUTTONF_STRING))
	{
		// Get button image if there is one
		if (gadget->Flags & GFLG_SELECTED && gadget->SelectRender && gadget->Flags & GFLG_GADGHIMAGE)
			image = (struct Image *)gadget->SelectRender;
		else if ((!(gadget->Flags & GFLG_SELECTED) || !(gadget->Flags & GFLG_GADGHIMAGE)) && gadget->GadgetRender &&
				 gadget->Flags & GFLG_GADGIMAGE)
			image = (struct Image *)gadget->GadgetRender;

		// Draw image
		if (image)
		{
			DrawImageState(rp,
						   image,
						   box.Left,
						   box.Top,
						   (gadget->Flags & GFLG_SELECTED) ? IDS_SELECTED : IDS_NORMAL,
						   render->gpr_GInfo->gi_DrInfo);
		}
	}

	// Disabled?
	if (gadget->Flags & GFLG_DISABLED && !(data->flags & BUTTONF_NO_GHOST))
	{
		unsigned short ghost[2];

		// Set ghosting pattern
		ghost[0] = 0x8888;
		ghost[1] = 0x2222;
		SetAfPt(rp, ghost, 1);
		SetAPen(rp, pens[SHADOWPEN]);

		// Fill with ghosting pattern
		RectFill(rp,
				 gadget_box.Left,
				 gadget_box.Top,
				 gadget_box.Left + gadget_box.Width - 1,
				 gadget_box.Top + gadget_box.Height - 1);
		SetAfPt(rp, 0, 0);
	}
}

// Resize the button
void button_resize(Class *cl, struct Gadget *gadget, ButtonData *data, struct gpResize *size)
{
	// Get new position
	*((struct IBox *)&gadget->LeftEdge) = size->gpr_Size;

	// String?
	if (data->flags & BUTTONF_STRING)
	{
		// Save border position
		((StringData *)data)->border = size->gpr_Size;

		// Shrink gadget
		button_field_shrink((StringData *)data, gadget, size->gpr_Window, size->gpr_Requester);
	}
}

// Shrink string gadget
void button_field_shrink(StringData *sdata, struct Gadget *gadget, struct Window *window, struct Requester *req)
{
	short shrink = 0;

	// Not got font yet?
	if (!sdata->string_font)
	{
		// Get string font
		if (!(sdata->string_font = ((struct StringInfo *)gadget->SpecialInfo)->Extension->Font) && window)
			sdata->string_font = window->RPort->Font;
	}

	// Get amount of horizontal space
	if (sdata->string_font)
	{
		short height;

		// Get gadget height
		height = gadget->Height;

		// Adjust for relative height
		if (gadget->Flags & GFLG_RELHEIGHT)
		{
			// Requester?
			if (req)
				height += req->Height;
			else
				height += window->Height;
		}

		// Get shrink amount
		shrink = height - sdata->string_font->tf_YSize;
		if (shrink > 1)
			shrink >>= 1;
		else
			shrink = 0;
	}

	// Thin borders?
	if (sdata->data.flags & BUTTONF_THIN)
	{
		// Shrink horizontally for thin borders
		gadget->LeftEdge += 4;
		gadget->Width -= 8;
	}

	// Shrink for thick borders
	else if (!(sdata->data.flags & BUTTONF_NOBORDER))
	{
		gadget->LeftEdge += 6;
		gadget->Width -= 12;
	}

	// Shrink gadget vertically
	gadget->TopEdge += shrink;
	gadget->Height -= shrink << 1;

	// Rel bottom?
	if (gadget->Flags & GFLG_RELBOTTOM)
		gadget->TopEdge++;
}
