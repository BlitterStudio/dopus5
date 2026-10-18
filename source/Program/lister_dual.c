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

#define DUALF_ONE_WINDOW (1 << 0)
#define DUALF_SCROLLBARS_REMOVED (1 << 1)
#define DUALF_PATHFIELD_OWNED (1 << 2)
#define DUALF_FRAME_REFRESH (1 << 4)

#define DUAL_PATH_REL_FLAGS (GFLG_RELRIGHT | GFLG_RELBOTTOM | GFLG_RELWIDTH | GFLG_RELHEIGHT)
#define DUAL_TITLE_BUTTON_WIDTH 18

typedef struct ListerDualState
{
	ULONG flags;
	short active;		  // Panel the user selected (source)
	short view;			  // Panel whose buffer and areas are loaded into the Lister
	short drop_side;	  // Destination panel of a drop being launched, or -1
	short highlight_side; // Panel holding the drag-over highlight
	BOOL locked;
	UBYTE rescan_sides; // Panels with a rescan queued while the lister was busy
	ListFormat format[LISTER_DUAL_SIDES]; // Working format of each panel (lister->format for the shown one)
	DirBuffer *old_buffer[LISTER_DUAL_SIDES]; // Buffer each panel returns to from a device list
	DirBuffer *buffer[LISTER_DUAL_SIDES];
	DirBuffer *offset_buffer[LISTER_DUAL_SIDES];
	long vert_offset[LISTER_DUAL_SIDES];
	long horiz_offset[LISTER_DUAL_SIDES];
	GUI_Element name_area[LISTER_DUAL_SIDES];
	GUI_Element close_area[LISTER_DUAL_SIDES];
	GUI_Element command_area[LISTER_DUAL_SIDES];
	GUI_Element parent_area[LISTER_DUAL_SIDES];
	GUI_Element title_area[LISTER_DUAL_SIDES];
	TitleBox title_boxes[LISTER_DUAL_SIDES][16];
	GUI_Element list_area[LISTER_DUAL_SIDES];
	GUI_Element text_area[LISTER_DUAL_SIDES];
	UWORD text_width[LISTER_DUAL_SIDES];
	UWORD text_height[LISTER_DUAL_SIDES];
	struct Rectangle header_rect[LISTER_DUAL_SIDES];
	struct Rectangle scroll_border[LISTER_DUAL_SIDES];
	struct Gadget *path_field[LISTER_DUAL_SIDES];
	char path_buffer[LISTER_DUAL_SIDES][512];
	UWORD path_rel_flags;
	BOOL path_rel_valid;
	struct Rectangle path_border[LISTER_DUAL_SIDES];
	struct Gadget *vert_scroller[LISTER_DUAL_SIDES];
	struct Gadget *horiz_scroller[LISTER_DUAL_SIDES];
	struct Gadget *vert_arrow_up[LISTER_DUAL_SIDES];
	struct Gadget *vert_arrow_down[LISTER_DUAL_SIDES];
	struct Gadget *horiz_arrow_left[LISTER_DUAL_SIDES];
	struct Gadget *horiz_arrow_right[LISTER_DUAL_SIDES];
	APTR scroll_image[LISTER_DUAL_SIDES][4];
	struct Rectangle scroll_rect[LISTER_DUAL_SIDES][3]; // Last scroller layout: vertical, horizontal, right arrow
	APTR scroll_knob[LISTER_DUAL_SIDES];
	struct Gadget *system_scroller;
	struct Gadget *system_horiz_scroller;
	ULONG frame_screen_flags;
	short frame_border_left;
	short frame_border_top;
	short frame_border_right;
	short frame_border_bottom;
	APTR frame_draw_info;
	ULONG scroll_screen_flags;
	short scroll_border_right;
	short scroll_border_bottom;
	APTR scroll_draw_info;
	struct Rectangle separator;
} ListerDualState;

typedef char lister_dual_pad2_holds_state_pointer[(sizeof(((Lister *)0)->pad2) >= sizeof(APTR)) ? 1 : -1];

static void lister_dual_restore_pathfield(Lister *lister, ListerDualState *state);
static void lister_dual_remove_scrollbars(Lister *lister, ListerDualState *state);
static void lister_dual_close_pathfields(Lister *lister, ListerDualState *state);
static void lister_dual_close_scrollbars(Lister *lister, ListerDualState *state);

static ListerDualState *lister_dual_get_state(Lister *lister)
{
	ListerDualState *state = 0;

	if (lister)
		CopyMem(lister->pad2, &state, sizeof(state));

	return state;
}

static void lister_dual_set_state(Lister *lister, ListerDualState *state)
{
	if (lister)
		CopyMem(&state, lister->pad2, sizeof(state));
}

static void lister_dual_clear_title_boxes(TitleBox *boxes)
{
	short item;

	if (!boxes)
		return;

	for (item = 0; item < 16; item++)
	{
		boxes[item].tb_Left = -1;
		boxes[item].tb_Right = -1;
		boxes[item].tb_Item = -1;
		boxes[item].tb_Position = 0;
		boxes[item].tb_Width = 0;
		boxes[item].tb_ItemNum = item;
	}
}

static void lister_dual_capture_title_boxes(Lister *lister, ListerDualState *state, short side)
{
	if (!lister || !state || side < 0 || side >= LISTER_DUAL_SIDES)
		return;

	CopyMem(lister->title_boxes, state->title_boxes[side], sizeof(lister->title_boxes));
}

static void lister_dual_apply_title_boxes(Lister *lister, ListerDualState *state, short side)
{
	if (!lister || !state || side < 0 || side >= LISTER_DUAL_SIDES)
		return;

	CopyMem(state->title_boxes[side], lister->title_boxes, sizeof(lister->title_boxes));
}

static ListerDualState *lister_dual_ensure_state(Lister *lister)
{
	ListerDualState *state;

	if (!(state = lister_dual_get_state(lister)))
	{
		if ((state = AllocVec(sizeof(ListerDualState), MEMF_CLEAR)))
		{
			state->active = LISTER_DUAL_LEFT;
			state->view = LISTER_DUAL_LEFT;
			state->drop_side = -1;
			state->highlight_side = -1;
			lister_dual_clear_title_boxes(state->title_boxes[LISTER_DUAL_LEFT]);
			lister_dual_clear_title_boxes(state->title_boxes[LISTER_DUAL_RIGHT]);
			lister_dual_set_state(lister, state);
		}
	}

	return state;
}

static void lister_dual_free_state(Lister *lister)
{
	ListerDualState *state;

	if ((state = lister_dual_get_state(lister)))
	{
		lister_dual_restore_pathfield(lister, state);
		lister_dual_remove_scrollbars(lister, state);

		// Unpublish the state before freeing it; other processes look it up
		lister_dual_set_state(lister, 0);
		FreeVec(state);
	}
}

static short lister_dual_other_side(short side)
{
	return (side == LISTER_DUAL_RIGHT) ? LISTER_DUAL_LEFT : LISTER_DUAL_RIGHT;
}

static void lister_dual_role_flags(Lister *lister, ListerDualState *state, short side)
{
	lister->flags &= ~(LISTERF_SOURCE | LISTERF_DEST | LISTERF_STORED_SOURCE | LISTERF_STORED_DEST |
					   LISTERF_SOURCEDEST_LOCK);

	if (side == state->active)
		lister->flags |= LISTERF_SOURCE;
	else
		lister->flags |= LISTERF_DEST;

	if (state->locked)
		lister->flags |= LISTERF_SOURCEDEST_LOCK;
}

static void lister_dual_init_side_offsets(ListerDualState *state, short side)
{
	DirBuffer *buffer;

	if (!state || side < 0 || side >= LISTER_DUAL_SIDES || !(buffer = state->buffer[side]))
		return;

	if (state->offset_buffer[side] != buffer)
	{
		state->offset_buffer[side] = buffer;
		state->vert_offset[side] = buffer->buf_VertOffset;
		state->horiz_offset[side] = buffer->buf_HorizOffset;
	}
}

static void lister_dual_apply_side_offsets(ListerDualState *state, short side)
{
	DirBuffer *buffer;

	if (!state || side < 0 || side >= LISTER_DUAL_SIDES || !(buffer = state->buffer[side]))
		return;

	buffer->buf_VertOffset = state->vert_offset[side];
	buffer->buf_HorizOffset = state->horiz_offset[side];
}

static void lister_dual_capture_side_offsets(Lister *lister, ListerDualState *state, short side)
{
	if (!lister || !state || side < 0 || side >= LISTER_DUAL_SIDES || !lister->cur_buffer)
		return;

	state->offset_buffer[side] = lister->cur_buffer;
	state->vert_offset[side] = lister->cur_buffer->buf_VertOffset;
	state->horiz_offset[side] = lister->cur_buffer->buf_HorizOffset;
}

// Load one panel's buffer and display areas into the Lister structure.
// Lister process only: the display code reads these fields directly.
static void lister_dual_apply_panel(Lister *lister, short side)
{
	ListerDualState *state;

	if (!(state = lister_dual_get_state(lister)) || !(state->flags & DUALF_ONE_WINDOW) || side < 0 ||
		side >= LISTER_DUAL_SIDES)
		return;

	if (!state->buffer[side])
		state->buffer[side] = state->buffer[LISTER_DUAL_LEFT];

	lister_dual_init_side_offsets(state, side);
	lister_dual_apply_side_offsets(state, side);

	// A buffer shown in both panels has one set of last-drawn offsets; after
	// a panel change they describe the other panel, so scrolling must redraw
	if (side != state->view && state->buffer[side] == state->buffer[lister_dual_other_side(side)])
	{
		state->buffer[side]->buf_OldVertOffset = -1;
		state->buffer[side]->buf_OldHorizOffset = -1;
	}

	// Other processes pair view with cur_buffer (lister_dual_side_buffer)
	// and write the shown panel's working format (lister_dual_set_format)
	Forbid();
	if (side != state->view)
	{
		state->format[state->view] = lister->format;
		lister->format = state->format[side];
		state->old_buffer[state->view] = lister->old_buffer;
		lister->old_buffer = state->old_buffer[side];
	}
	state->view = side;
	lister->cur_buffer = state->buffer[side];
	Permit();
	lister->name_area = state->name_area[side];
	lister->status_area = state->close_area[side];
	lister->command_area = state->command_area[side];
	lister->parent_area = state->parent_area[side];
	lister->title_area = state->title_area[side];
	lister->list_area = state->list_area[side];
	lister->text_area = state->text_area[side];
	lister->text_width = state->text_width[side];
	lister->text_height = state->text_height[side];
	lister->scroll_border = state->scroll_border[side];
	lister_dual_apply_title_boxes(lister, state, side);
	lister->backdrop_info->size = lister->list_area.rect;
	if (state->vert_scroller[side])
		lister->backdrop_info->vert_scroller = state->vert_scroller[side];
	if (state->horiz_scroller[side])
		lister->backdrop_info->horiz_scroller = state->horiz_scroller[side];
	lister_dual_role_flags(lister, state, side);
}

// Store the buffer and scroll offsets the Lister currently shows into the
// panel they belong to. Lister code changes cur_buffer (reading a directory,
// device lists, history) without knowing about panels, so this runs before
// any other panel is loaded.
static void lister_dual_capture_view(Lister *lister, ListerDualState *state)
{
	short other;

	if (!state || !(state->flags & DUALF_ONE_WINDOW) || !lister->cur_buffer)
		return;

	state->buffer[state->view] = lister->cur_buffer;
	lister_dual_capture_side_offsets(lister, state, state->view);
	if (lister->cur_buffer == lister->special_buffer)
	{
		other = lister_dual_other_side(state->view);
		if (!state->buffer[other] || (!state->buffer[other]->buf_Path[0] && !(state->buffer[other]->flags & DWF_VALID)))
			state->buffer[other] = lister->cur_buffer;
	}
}

// Capture the current panel, then load another one
static void lister_dual_switch_view(Lister *lister, ListerDualState *state, short side)
{
	lister_dual_capture_view(lister, state);
	lister_dual_apply_panel(lister, side);
}

static void lister_dual_apply_active(Lister *lister)
{
	ListerDualState *state;

	if ((state = lister_dual_get_state(lister)) && state->flags & DUALF_ONE_WINDOW)
		lister_dual_switch_view(lister, state, state->active);
}

static void lister_dual_update_title_mode(Lister *lister, ListerDualState *state)
{
	short side;
	BOOL show_titles = FALSE;

	if (!lister || !state || !(environment->env->lister_options & LISTEROPTF_TITLES) ||
		(lister->flags & LISTERF_VIEW_ICONS))
	{
		lister->more_flags &= ~(LISTERF_TITLE | LISTERF_HIDE_TITLE);
		return;
	}

	for (side = 0; side < LISTER_DUAL_SIDES; side++)
	{
		if (state->buffer[side] && state->buffer[side] != lister->special_buffer &&
			!(state->buffer[side]->more_flags & DWF_HIDE_TITLE))
		{
			show_titles = TRUE;
			break;
		}
	}

	if (show_titles)
	{
		lister->more_flags |= LISTERF_TITLE;
		lister->more_flags &= ~LISTERF_HIDE_TITLE;
	}
	else
	{
		lister->more_flags &= ~LISTERF_TITLE;
		lister->more_flags |= LISTERF_HIDE_TITLE;
	}
}

static short lister_dual_rect_width(struct Rectangle *rect)
{
	return rect->MaxX - rect->MinX + 1;
}

static short lister_dual_rect_height(struct Rectangle *rect)
{
	return rect->MaxY - rect->MinY + 1;
}

static short lister_dual_bottom_scroll_height(Lister *lister)
{
	short height = 8;

	if (lister_valid_window(lister))
		height = lister->window->BorderBottom - 4;
	if (height < 1)
		height = 1;

	return height;
}

static void lister_dual_resize_gadget(Lister *lister, struct Gadget *gadget, struct Rectangle *rect)
{
	if (!lister_valid_window(lister) || !gadget || !rect)
		return;

	gadget->Flags &= ~(GFLG_RELRIGHT | GFLG_RELBOTTOM | GFLG_RELWIDTH | GFLG_RELHEIGHT);
	gadget->LeftEdge = rect->MinX;
	gadget->TopEdge = rect->MinY;
	gadget->Width = lister_dual_rect_width(rect);
	gadget->Height = lister_dual_rect_height(rect);
}

static void lister_dual_resize_bottom_gadget(Lister *lister, struct Gadget *gadget, struct Rectangle *rect)
{
	if (!lister_valid_window(lister) || !gadget || !rect)
		return;

	gadget->LeftEdge = rect->MinX;
	gadget->Flags &= ~(GFLG_RELRIGHT | GFLG_RELWIDTH | GFLG_RELHEIGHT);
	gadget->Flags |= GFLG_RELBOTTOM;
	gadget->TopEdge = rect->MinY - lister->window->Height;
	gadget->Width = lister_dual_rect_width(rect);
	gadget->Height = lister_dual_rect_height(rect);
}

static void lister_dual_resize_path_gadget(Lister *lister, struct Gadget *gadget, struct Rectangle *rect)
{
	struct gpResize resize;

	if (!lister_valid_window(lister) || !gadget || !rect)
		return;

	resize.MethodID = GM_RESIZE;
	resize.gpr_GInfo = 0;
	resize.gpr_RPort = 0;
	resize.gpr_Size.Left = rect->MinX;
	resize.gpr_Size.Top = rect->MinY;
	resize.gpr_Size.Width = lister_dual_rect_width(rect);
	resize.gpr_Size.Height = lister_dual_rect_height(rect);
	resize.gpr_Redraw = 0;
	resize.gpr_Window = lister->window;
	resize.gpr_Requester = 0;
	DoMethodA((Object *)gadget, (Msg)&resize);
}

static BOOL lister_dual_gadget_rect(Lister *lister, struct Gadget *gadget, struct Rectangle *rect)
{
	LONG left, top, width, height;

	if (!lister_valid_window(lister) || !gadget || !rect)
		return FALSE;

	left = gadget->LeftEdge;
	top = gadget->TopEdge;
	width = gadget->Width;
	height = gadget->Height;

	if (gadget->Flags & GFLG_RELRIGHT)
		left += lister->window->Width;
	if (gadget->Flags & GFLG_RELBOTTOM)
		top += lister->window->Height;
	if (gadget->Flags & GFLG_RELWIDTH)
		width += lister->window->Width;
	if (gadget->Flags & GFLG_RELHEIGHT)
		height += lister->window->Height;

	if (width < 1 || height < 1)
		return FALSE;

	rect->MinX = left;
	rect->MinY = top;
	rect->MaxX = left + width - 1;
	rect->MaxY = top + height - 1;

	if (rect->MinX < 0)
		rect->MinX = 0;
	if (rect->MinY < 0)
		rect->MinY = 0;
	if (rect->MaxX >= lister->window->Width)
		rect->MaxX = lister->window->Width - 1;
	if (rect->MaxY >= lister->window->Height)
		rect->MaxY = lister->window->Height - 1;

	return (rect->MaxX >= rect->MinX && rect->MaxY >= rect->MinY);
}

static void lister_dual_clear_gadget_box(Lister *lister, struct Gadget *gadget)
{
	struct Rectangle rect;
	short clear_pen;

	if (!lister_dual_gadget_rect(lister, gadget, &rect))
		return;

	clear_pen = (gadget->Flags & GFLG_RELBOTTOM) ? FILLPEN : BACKGROUNDPEN;
	SetAPen(lister->window->RPort, GUI->draw_info->dri_Pens[clear_pen]);
	RectFill(lister->window->RPort,
			 rect.MinX,
			 rect.MinY,
			 rect.MaxX,
			 rect.MaxY);
}

static void lister_dual_clear_gadget_chain(Lister *lister, struct Gadget *gadget, short count)
{
	struct Gadget *next;

	while (gadget && count)
	{
		next = gadget->NextGadget;
		lister_dual_clear_gadget_box(lister, gadget);
		gadget = next;
		--count;
	}
}

static char *lister_dual_path_buffer(Lister *lister, ListerDualState *state, short side)
{
	if (side == LISTER_DUAL_LEFT)
	{
		stccpy(state->path_buffer[LISTER_DUAL_LEFT], lister->path_buffer, sizeof(state->path_buffer[0]));
		return state->path_buffer[LISTER_DUAL_LEFT];
	}

	return state->path_buffer[LISTER_DUAL_RIGHT];
}

static BOOL lister_dual_add_pathfield(Lister *lister, ListerDualState *state)
{
	if (!lister_valid_window(lister) || !state || !lister->path_field)
		return FALSE;

	lister->path_field->NextGadget = 0;
	AddGList(lister->window, lister->path_field, -1, 1, 0);
	lister->flags |= LISTERF_PATH_FIELD;
	state->flags |= DUALF_PATHFIELD_OWNED;
	return TRUE;
}

static void lister_dual_restore_pathfield(Lister *lister, ListerDualState *state)
{
	struct gpResize resize;

	if (!lister_valid_window(lister) || !state || !lister->path_field)
		return;

	if (state->path_field[LISTER_DUAL_RIGHT])
	{
		RemoveGList(lister->window, state->path_field[LISTER_DUAL_RIGHT], 1);
		DisposeObject((Object *)state->path_field[LISTER_DUAL_RIGHT]);
		state->path_field[LISTER_DUAL_RIGHT] = 0;
	}

	if (state->path_rel_valid)
		lister->path_field->Flags = (lister->path_field->Flags & ~DUAL_PATH_REL_FLAGS) | state->path_rel_flags;

	resize.MethodID = GM_RESIZE;
	resize.gpr_GInfo = 0;
	resize.gpr_RPort = 0;
	resize.gpr_Size.Left = lister->window->BorderLeft;
	resize.gpr_Size.Top = -(lister->window->BorderBottom + FIELD_FONT->tf_YSize + 4);
	resize.gpr_Size.Width = -(lister->window->BorderLeft + lister->window->BorderRight);
	resize.gpr_Size.Height = FIELD_FONT->tf_YSize + 4;
	resize.gpr_Redraw = 0;
	resize.gpr_Window = lister->window;
	resize.gpr_Requester = 0;
	DoMethodA((Object *)lister->path_field, (Msg)&resize);

	if (state->flags & DUALF_PATHFIELD_OWNED)
	{
		RemoveGList(lister->window, lister->path_field, 1);
		lister->path_field->NextGadget = 0;
		lister->parent_button.NextGadget = lister->path_field;
		AddGList(lister->window, &lister->parent_button, -1, 2, 0);
		RefreshGList(&lister->parent_button, lister->window, 0, 2);
		RefreshWindowFrame(lister->window);
		state->flags &= ~DUALF_PATHFIELD_OWNED;
	}
	state->path_field[LISTER_DUAL_LEFT] = 0;
}

static void lister_dual_close_pathfields(Lister *lister, ListerDualState *state)
{
	if (!lister_valid_window(lister) || !state)
		return;

	if (state->path_field[LISTER_DUAL_RIGHT])
	{
		RemoveGList(lister->window, state->path_field[LISTER_DUAL_RIGHT], 1);
		DisposeObject((Object *)state->path_field[LISTER_DUAL_RIGHT]);
		state->path_field[LISTER_DUAL_RIGHT] = 0;
	}

	if (lister->path_field)
	{
		if (state->path_rel_valid)
			lister->path_field->Flags = (lister->path_field->Flags & ~DUAL_PATH_REL_FLAGS) | state->path_rel_flags;

		if (state->flags & DUALF_PATHFIELD_OWNED)
		{
			RemoveGList(lister->window, lister->path_field, 1);
			lister->path_field->NextGadget = 0;
			state->flags &= ~DUALF_PATHFIELD_OWNED;
		}
	}

	state->path_field[LISTER_DUAL_LEFT] = 0;
}

static BOOL lister_dual_ensure_pathfields(Lister *lister, ListerDualState *state)
{
	if (!lister_valid_window(lister) || !state || !lister->path_field)
		return FALSE;

	state->path_field[LISTER_DUAL_LEFT] = lister->path_field;

	if (!state->path_rel_valid)
	{
		state->path_rel_flags = lister->path_field->Flags & DUAL_PATH_REL_FLAGS;
		state->path_rel_valid = TRUE;
	}
	if (!(state->flags & DUALF_PATHFIELD_OWNED))
	{
		RemoveGList(lister->window, &lister->parent_button, 2);
		RefreshWindowFrame(lister->window);
		lister->path_field->NextGadget = 0;
		AddGList(lister->window, lister->path_field, -1, 1, 0);
		state->flags |= DUALF_PATHFIELD_OWNED;
	}
	lister->path_field->Flags &= ~DUAL_PATH_REL_FLAGS;
	lister_dual_resize_path_gadget(lister, state->path_field[LISTER_DUAL_LEFT], &state->path_border[LISTER_DUAL_LEFT]);

	if (!state->path_field[LISTER_DUAL_RIGHT])
	{
		if (!(state->path_field[LISTER_DUAL_RIGHT] =
				  (struct Gadget *)NewObject(0,
											 "dopusstrgclass",
											 GA_ID,
											 GAD_PATH,
											 GA_Left,
											 state->path_border[LISTER_DUAL_RIGHT].MinX,
											 GA_Top,
											 state->path_border[LISTER_DUAL_RIGHT].MinY,
											 GA_Width,
											 lister_dual_rect_width(&state->path_border[LISTER_DUAL_RIGHT]),
											 GA_Height,
											 lister_dual_rect_height(&state->path_border[LISTER_DUAL_RIGHT]),
											 GA_RelVerify,
											 TRUE,
											 GA_Immediate,
											 TRUE,
											 GTCustom_ThinBorders,
											 TRUE,
											 GTCustom_NoGhost,
											 TRUE,
											 STRINGA_TextVal,
											 (IPTR)((state->buffer[LISTER_DUAL_RIGHT])
														? state->buffer[LISTER_DUAL_RIGHT]->buf_ExpandedPath
														: ""),
											 STRINGA_MaxChars,
											 511,
											 STRINGA_Buffer,
											 (IPTR)state->path_buffer[LISTER_DUAL_RIGHT],
											 STRINGA_UndoBuffer,
											 (IPTR)GUI->global_undo_buffer,
											 STRINGA_WorkBuffer,
											 (IPTR)(GUI->global_undo_buffer + 512),
											 STRINGA_Font,
											 (IPTR)FIELD_FONT,
											 STRINGA_EditHook,
											 (IPTR)lister->path_edit_hook,
											 TAG_END)))
		{
			lister_dual_restore_pathfield(lister, state);
			return FALSE;
		}

		AddGList(lister->window, state->path_field[LISTER_DUAL_RIGHT], -1, 1, 0);
	}
	else
		lister_dual_resize_path_gadget(
			lister, state->path_field[LISTER_DUAL_RIGHT], &state->path_border[LISTER_DUAL_RIGHT]);

	return TRUE;
}

static void lister_dual_refresh_pathfields(Lister *lister, ListerDualState *state)
{
	if (!lister_valid_window(lister) || !state || !(lister->flags & LISTERF_PATH_FIELD))
		return;

	if (state->path_field[LISTER_DUAL_LEFT])
		RefreshGList(state->path_field[LISTER_DUAL_LEFT], lister->window, 0, 1);
	if (state->path_field[LISTER_DUAL_RIGHT])
		RefreshGList(state->path_field[LISTER_DUAL_RIGHT], lister->window, 0, 1);
}

static void lister_dual_update_parent_button_metrics(Lister *lister)
{
	if (!lister_valid_window(lister))
		return;

	lister->parent_button.TopEdge = lister->window->BorderTop;
	lister->parent_button.Width = lister->window->BorderLeft;
	lister->parent_button.Height = -(lister->window->BorderTop + lister->window->BorderBottom);
}

static BOOL lister_dual_frame_metrics_changed(Lister *lister, ListerDualState *state)
{
	ULONG screen_flags = 0;

	if (!lister_valid_window(lister) || !state)
		return FALSE;

	if (lister->window->WScreen)
		screen_flags = lister->window->WScreen->Flags & SCREENHIRES;

	return (state->frame_screen_flags != screen_flags || state->frame_border_left != lister->window->BorderLeft ||
			state->frame_border_top != lister->window->BorderTop ||
			state->frame_border_right != lister->window->BorderRight ||
			state->frame_border_bottom != lister->window->BorderBottom ||
			state->frame_draw_info != (APTR)GUI->draw_info);
}

static void lister_dual_store_frame_metrics(Lister *lister, ListerDualState *state)
{
	if (!lister_valid_window(lister) || !state)
		return;

	state->frame_screen_flags = (lister->window->WScreen) ? lister->window->WScreen->Flags & SCREENHIRES : 0;
	state->frame_border_left = lister->window->BorderLeft;
	state->frame_border_top = lister->window->BorderTop;
	state->frame_border_right = lister->window->BorderRight;
	state->frame_border_bottom = lister->window->BorderBottom;
	state->frame_draw_info = (APTR)GUI->draw_info;
}

static void lister_dual_prepare_frame_refresh(Lister *lister, ListerDualState *state)
{
	if (!lister_valid_window(lister) || !state)
		return;

	if (lister_dual_frame_metrics_changed(lister, state))
	{
		lister_dual_update_parent_button_metrics(lister);
		state->flags |= DUALF_FRAME_REFRESH;
	}
	lister_dual_store_frame_metrics(lister, state);
}

void lister_dual_finish_frame_refresh(Lister *lister)
{
	ListerDualState *state;

	if (!lister_valid_window(lister) || !(state = lister_dual_get_state(lister)) ||
		!(state->flags & DUALF_FRAME_REFRESH))
		return;
	if (lister->window->WLayer && (lister->window->WLayer->Flags & LAYERREFRESH))
		return;

	state->flags &= ~DUALF_FRAME_REFRESH;
	RefreshWindowFrame(lister->window);
}

static void lister_dual_prepare_resize_refresh(Lister *lister)
{
	struct Rectangle rect;

	if (!lister_valid_window(lister) || !(lister->flags & LISTERF_RESIZED) || !lister->refresh_extra)
		return;

	rect.MinX = lister->window->BorderLeft;
	rect.MinY = lister->window->BorderTop;
	rect.MaxX = lister->window->Width - lister->window->BorderRight - 1;
	rect.MaxY = lister->window->Height - lister->window->BorderBottom - 1;
	if (rect.MinX > rect.MaxX || rect.MinY > rect.MaxY)
		return;

	EraseRect(lister->window->RPort, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);
	OrRectRegion(lister->refresh_extra, &rect);
	RefreshWindowFrame(lister->window);
}

static void lister_dual_scroll_rects(Lister *lister,
									 ListerDualState *state,
									 short side,
									 struct Rectangle *vert,
									 struct Rectangle *up,
									 struct Rectangle *down,
									 struct Rectangle *horiz,
									 struct Rectangle *left,
									 struct Rectangle *right)
{
	short arrow_h, left_arrow_w, right_arrow_w, bar_w, bar_h, panel_left, panel_right, bottom_top;
	short horiz_arrow_top, horiz_arrow_height, bottom_strip_height, bottom_strip_top, bottom_strip_bottom;

	bottom_strip_height = lister_dual_bottom_scroll_height(lister);
	arrow_h = lister->window->BorderRight;
	left_arrow_w = lister->window->BorderRight;
	right_arrow_w = lister->window->BorderRight;
	horiz_arrow_height = bottom_strip_height;
	if (arrow_h < 1)
		arrow_h = 1;
	if (left_arrow_w < 1)
		left_arrow_w = 1;
	if (right_arrow_w < 1)
		right_arrow_w = 1;

	if (state->scroll_image[side][0])
	{
		left_arrow_w = ((struct Image *)state->scroll_image[side][0])->Width;
	}
	if (state->scroll_image[side][2])
		right_arrow_w = ((struct Image *)state->scroll_image[side][2])->Width;
	if (state->scroll_image[side][1])
		arrow_h = ((struct Image *)state->scroll_image[side][1])->Height;

	bar_w = lister->window->BorderRight;
	if (bar_w < left_arrow_w)
		bar_w = left_arrow_w;
	if (bar_w < right_arrow_w)
		bar_w = right_arrow_w;

	panel_left = state->path_border[side].MinX;
	panel_right = state->path_border[side].MaxX;
	bottom_strip_top = lister->window->Height - lister->window->BorderBottom + 3;
	bottom_strip_bottom = bottom_strip_top + bottom_strip_height - 1;
	bar_h = bottom_strip_height;
	bottom_top = bottom_strip_top;
	if (bottom_top + bar_h > bottom_strip_bottom + 1)
		bar_h = bottom_strip_bottom - bottom_top + 1;
	if (bar_h < 1)
		bar_h = 1;

	vert->MinX = panel_right - bar_w + 1;
	vert->MaxX = panel_right;
	vert->MinY = state->list_area[side].rect.MinY;
	vert->MaxY = state->path_border[side].MinY - (arrow_h << 1) - 2;
	if (vert->MaxY < vert->MinY)
		vert->MaxY = vert->MinY;

	*up = *vert;
	up->MinY = vert->MaxY + 1;
	up->MaxY = up->MinY + arrow_h - 1;
	*down = *up;
	down->MinY = up->MaxY + 1;
	down->MaxY = down->MinY + arrow_h - 1;

	right->MaxX = panel_right;
	right->MinX = right->MaxX - right_arrow_w + 1;
	horiz_arrow_top = lister->window->Height - lister->window->BorderBottom + 1;
	horiz_arrow_height = lister->window->BorderBottom;
	if (horiz_arrow_height < 1)
		horiz_arrow_height = 1;
	right->MinY = horiz_arrow_top;
	right->MaxY = horiz_arrow_top + horiz_arrow_height - 1;
	left->MaxX = right->MinX - 1;
	left->MinX = left->MaxX - left_arrow_w + 1;
	left->MinY = right->MinY;
	left->MaxY = right->MaxY;

	horiz->MinX = panel_left;
	horiz->MaxX = left->MinX - 4;
	horiz->MinY = bottom_top;
	horiz->MaxY = bottom_top + bar_h - 1;
	if (horiz->MaxX < horiz->MinX)
		horiz->MaxX = horiz->MinX;
}

static BOOL lister_dual_same_rect(struct Rectangle *a, struct Rectangle *b)
{
	return (a->MinX == b->MinX && a->MinY == b->MinY && a->MaxX == b->MaxX && a->MaxY == b->MaxY) ? TRUE : FALSE;
}

// Move a panel's scrollers to the current layout; returns FALSE if attached
// gadgets were already in place
static BOOL lister_dual_position_scrollbars(Lister *lister, ListerDualState *state, short side, BOOL attached)
{
	struct Rectangle vert, up, down, horiz, left, right;
	long pos = -1;

	if (!state->vert_scroller[side])
		return FALSE;

	lister_dual_scroll_rects(lister, state, side, &vert, &up, &down, &horiz, &left, &right);
	if (attached && lister_dual_same_rect(&vert, &state->scroll_rect[side][0]) &&
		lister_dual_same_rect(&horiz, &state->scroll_rect[side][1]) &&
		lister_dual_same_rect(&right, &state->scroll_rect[side][2]))
		return FALSE;

	if (attached && lister_valid_window(lister))
		pos = RemoveGList(lister->window, state->vert_scroller[side], 6);

	lister_dual_clear_gadget_box(lister, state->vert_scroller[side]);
	lister_dual_clear_gadget_box(lister, state->vert_arrow_up[side]);
	lister_dual_clear_gadget_box(lister, state->vert_arrow_down[side]);
	lister_dual_clear_gadget_box(lister, state->horiz_scroller[side]);
	lister_dual_clear_gadget_box(lister, state->horiz_arrow_left[side]);
	lister_dual_clear_gadget_box(lister, state->horiz_arrow_right[side]);

	lister_dual_resize_gadget(lister, state->vert_scroller[side], &vert);
	lister_dual_resize_gadget(lister, state->vert_arrow_up[side], &up);
	lister_dual_resize_gadget(lister, state->vert_arrow_down[side], &down);
	lister_dual_resize_bottom_gadget(lister, state->horiz_scroller[side], &horiz);
	lister_dual_resize_bottom_gadget(lister, state->horiz_arrow_left[side], &left);
	lister_dual_resize_bottom_gadget(lister, state->horiz_arrow_right[side], &right);
	state->scroll_rect[side][0] = vert;
	state->scroll_rect[side][1] = horiz;
	state->scroll_rect[side][2] = right;

	if (attached && lister_valid_window(lister))
		AddGList(lister->window, state->vert_scroller[side], pos, 6, 0);
	return TRUE;
}

static BOOL lister_dual_scroll_metrics_changed(Lister *lister, ListerDualState *state)
{
	ULONG screen_flags = 0;

	if (!lister_valid_window(lister) || !state)
		return FALSE;

	if (!state->vert_scroller[LISTER_DUAL_LEFT] && !state->vert_scroller[LISTER_DUAL_RIGHT])
		return FALSE;

	if (lister->window->WScreen)
		screen_flags = lister->window->WScreen->Flags & SCREENHIRES;

	return (state->scroll_screen_flags != screen_flags || state->scroll_border_right != lister->window->BorderRight ||
			state->scroll_border_bottom != lister->window->BorderBottom || state->scroll_draw_info != (APTR)GUI->draw_info);
}

static void lister_dual_store_scroll_metrics(Lister *lister, ListerDualState *state)
{
	if (!lister_valid_window(lister) || !state)
		return;

	state->scroll_screen_flags = (lister->window->WScreen) ? lister->window->WScreen->Flags & SCREENHIRES : 0;
	state->scroll_border_right = lister->window->BorderRight;
	state->scroll_border_bottom = lister->window->BorderBottom;
	state->scroll_draw_info = (APTR)GUI->draw_info;
}

static void lister_dual_clear_scroll_metrics(ListerDualState *state)
{
	if (!state)
		return;

	state->scroll_screen_flags = 0;
	state->scroll_border_right = 0;
	state->scroll_border_bottom = 0;
	state->scroll_draw_info = 0;
}

static void lister_dual_dispose_scrollbar_side(Lister *lister, ListerDualState *state, short side, BOOL attached)
{
	short a;

	if (!state || side < 0 || side >= LISTER_DUAL_SIDES)
		return;

	if (attached && lister_valid_window(lister) && state->vert_scroller[side])
		RemoveGList(lister->window, state->vert_scroller[side], 6);

	if (state->vert_scroller[side])
		DisposeObject((Object *)state->vert_scroller[side]);
	if (state->vert_arrow_up[side])
		DisposeObject((Object *)state->vert_arrow_up[side]);
	if (state->vert_arrow_down[side])
		DisposeObject((Object *)state->vert_arrow_down[side]);
	if (state->horiz_scroller[side])
		DisposeObject((Object *)state->horiz_scroller[side]);
	if (state->horiz_arrow_left[side])
		DisposeObject((Object *)state->horiz_arrow_left[side]);
	if (state->horiz_arrow_right[side])
		DisposeObject((Object *)state->horiz_arrow_right[side]);

	state->vert_scroller[side] = 0;
	state->vert_arrow_up[side] = 0;
	state->vert_arrow_down[side] = 0;
	state->horiz_scroller[side] = 0;
	state->horiz_arrow_left[side] = 0;
	state->horiz_arrow_right[side] = 0;

	for (a = 0; a < 4; a++)
	{
		if (state->scroll_image[side][a])
			DisposeObject((Object *)state->scroll_image[side][a]);
		state->scroll_image[side][a] = 0;
	}

	if (state->scroll_knob[side])
		DisposeObject((Object *)state->scroll_knob[side]);
	state->scroll_knob[side] = 0;
}

static BOOL lister_dual_create_scrollbar_side(Lister *lister, ListerDualState *state, short side)
{
	short a;
	static struct TagItem map_tags[] = {{PGA_Top, ICSPECIAL_CODE}, {TAG_END}};

	if (state->vert_scroller[side])
	{
		lister_dual_position_scrollbars(lister, state, side, TRUE);
		RefreshGList(state->vert_scroller[side], lister->window, 0, 6);
		return TRUE;
	}

	for (a = 0; a < 4; a++)
	{
		if (!(state->scroll_image[side][a] = NewObject(0,
													   "sysiclass",
													   SYSIA_Size,
													   (lister->window->WScreen->Flags & SCREENHIRES) ? SYSISIZE_MEDRES
																									  : SYSISIZE_LOWRES,
													   SYSIA_Which,
													   LEFTIMAGE + a,
													   SYSIA_DrawInfo,
													   (IPTR)GUI->draw_info,
													   TAG_END)))
			goto fail;
	}

	state->scroll_knob[side] = DOPUS_NEW_PROP_KNOB(TRUE);

	if (!(state->vert_scroller[side] = (struct Gadget *)NewObject(0,
																 "propgclass",
																 GA_ID,
																 GAD_VERT_SCROLLER,
																 GA_Immediate,
																 TRUE,
																 GA_FollowMouse,
																 TRUE,
																 GA_RelVerify,
																 TRUE,
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
																 DOPUS_PROP_KNOB_TAG(state->scroll_knob[side]),
																 (IPTR)state->scroll_knob[side],
																 ICA_TARGET,
																 ICTARGET_IDCMP,
																 ICA_MAP,
																 map_tags,
																 TAG_END)) ||
		!(state->vert_arrow_up[side] = (struct Gadget *)NewObject(0,
																  "buttongclass",
																  GA_ID,
																  GAD_VERT_ARROW_UP,
																  GA_Image,
																  state->scroll_image[side][1],
																  GA_Immediate,
																  TRUE,
																  GA_RelVerify,
																  TRUE,
																  ICA_TARGET,
																  ICTARGET_IDCMP,
																  TAG_END)) ||
		!(state->vert_arrow_down[side] = (struct Gadget *)NewObject(0,
																	"buttongclass",
																	GA_ID,
																	GAD_VERT_ARROW_DOWN,
																	GA_Image,
																	state->scroll_image[side][3],
																	GA_Immediate,
																	TRUE,
																	GA_RelVerify,
																	TRUE,
																	ICA_TARGET,
																	ICTARGET_IDCMP,
																	TAG_END)) ||
		!(state->horiz_scroller[side] = (struct Gadget *)NewObject(0,
																  "propgclass",
																  GA_ID,
																  GAD_HORIZ_SCROLLER,
																  GA_Immediate,
																  TRUE,
																  GA_FollowMouse,
																  TRUE,
																  GA_RelVerify,
																  TRUE,
																  GA_BottomBorder,
																  TRUE,
																  PGA_Freedom,
																  FREEHORIZ,
																  PGA_NewLook,
																  TRUE,
																  PGA_Borderless,
																  TRUE,
																  PGA_Visible,
																  1,
																  PGA_Total,
																  1,
																  ICA_TARGET,
																  ICTARGET_IDCMP,
																  ICA_MAP,
																  map_tags,
																  TAG_END)) ||
		!(state->horiz_arrow_left[side] = (struct Gadget *)NewObject(0,
																	"buttongclass",
																	GA_ID,
																	GAD_HORIZ_ARROW_LEFT,
																	GA_Image,
																	state->scroll_image[side][0],
																	GA_Immediate,
																	TRUE,
																	GA_RelVerify,
																	TRUE,
																	GA_BottomBorder,
																	TRUE,
																	ICA_TARGET,
																	ICTARGET_IDCMP,
																	TAG_END)) ||
		!(state->horiz_arrow_right[side] = (struct Gadget *)NewObject(0,
																	 "buttongclass",
																	 GA_ID,
																	 GAD_HORIZ_ARROW_RIGHT,
																	 GA_Image,
																	 state->scroll_image[side][2],
																	 GA_Immediate,
																	 TRUE,
																	 GA_RelVerify,
																	 TRUE,
																	 GA_BottomBorder,
																	 TRUE,
																	 ICA_TARGET,
																	 ICTARGET_IDCMP,
																	 TAG_END)))
		goto fail;

	state->vert_scroller[side]->NextGadget = state->vert_arrow_up[side];
	state->vert_arrow_up[side]->NextGadget = state->vert_arrow_down[side];
	state->vert_arrow_down[side]->NextGadget = state->horiz_scroller[side];
	state->horiz_scroller[side]->NextGadget = state->horiz_arrow_left[side];
	state->horiz_arrow_left[side]->NextGadget = state->horiz_arrow_right[side];
	state->horiz_arrow_right[side]->NextGadget = 0;

	lister_dual_position_scrollbars(lister, state, side, FALSE);
	AddGList(lister->window, state->vert_scroller[side], -1, 6, 0);
	RefreshGList(state->vert_scroller[side], lister->window, 0, 6);
	return TRUE;

fail:
	lister_dual_dispose_scrollbar_side(lister, state, side, FALSE);
	return FALSE;
}

static void lister_dual_dispose_dual_scrollbars(Lister *lister, ListerDualState *state)
{
	short side;

	if (!state)
		return;

	for (side = 0; side < LISTER_DUAL_SIDES; side++)
		lister_dual_dispose_scrollbar_side(lister, state, side, TRUE);
}

static void lister_dual_remove_scrollbars(Lister *lister, ListerDualState *state)
{
	if (!state)
		return;

	lister_dual_dispose_dual_scrollbars(lister, state);

	if (lister_valid_window(lister) && state->flags & DUALF_SCROLLBARS_REMOVED && state->system_scroller)
	{
		AddGList(lister->window, state->system_scroller, -1, 6, 0);
		RefreshGList(state->system_scroller, lister->window, 0, 6);
		RefreshWindowFrame(lister->window);
		lister->backdrop_info->vert_scroller = state->system_scroller;
		lister->backdrop_info->horiz_scroller = state->system_horiz_scroller;
	}
	state->flags &= ~DUALF_SCROLLBARS_REMOVED;
	state->system_scroller = 0;
	state->system_horiz_scroller = 0;
	lister_dual_clear_scroll_metrics(state);
}

static void lister_dual_close_scrollbars(Lister *lister, ListerDualState *state)
{
	if (!state)
		return;

	lister_dual_dispose_dual_scrollbars(lister, state);
	state->flags &= ~DUALF_SCROLLBARS_REMOVED;
	state->system_scroller = 0;
	state->system_horiz_scroller = 0;
	lister_dual_clear_scroll_metrics(state);
}

static BOOL lister_dual_ensure_scrollbars(Lister *lister, ListerDualState *state)
{
	short side;

	if (!lister_valid_window(lister) || !state)
		return FALSE;

	if (lister_dual_scroll_metrics_changed(lister, state))
	{
		lister_dual_dispose_dual_scrollbars(lister, state);
		lister->backdrop_info->vert_scroller = state->system_scroller;
		lister->backdrop_info->horiz_scroller = state->system_horiz_scroller;
	}

	if (!(state->flags & DUALF_SCROLLBARS_REMOVED) && lister->backdrop_info->vert_scroller)
	{
		state->system_scroller = lister->backdrop_info->vert_scroller;
		state->system_horiz_scroller = lister->backdrop_info->horiz_scroller;
		RemoveGList(lister->window, state->system_scroller, 6);
		lister_dual_clear_gadget_chain(lister, state->system_scroller, 6);
		RefreshWindowFrame(lister->window);
		state->flags |= DUALF_SCROLLBARS_REMOVED;
	}

	for (side = 0; side < LISTER_DUAL_SIDES; side++)
	{
		if (!lister_dual_create_scrollbar_side(lister, state, side))
		{
			lister_dual_remove_scrollbars(lister, state);
			return FALSE;
		}
	}

	lister->backdrop_info->vert_scroller = state->vert_scroller[state->view];
	lister->backdrop_info->horiz_scroller = state->horiz_scroller[state->view];
	lister_dual_store_scroll_metrics(lister, state);
	return TRUE;
}

static void lister_dual_layout(Lister *lister, ListerDualState *state)
{
	short a, panel;
	short gap = 3;
	short scroll_width, content_width;
	short panel_left[LISTER_DUAL_SIDES], panel_width[LISTER_DUAL_SIDES];
	short panel_top, list_top, width, path_top, path_bottom;

	if (!state->buffer[LISTER_DUAL_LEFT])
		state->buffer[LISTER_DUAL_LEFT] = lister->cur_buffer;
	if (!state->buffer[LISTER_DUAL_RIGHT])
		state->buffer[LISTER_DUAL_RIGHT] = state->buffer[LISTER_DUAL_LEFT];

	// Common toolbar at the top of the single dual window
	if (lister_valid_toolbar(lister))
	{
		lister->toolbar_area.box.Left = lister->window->BorderLeft;
		lister->toolbar_area.box.Top = lister->window->BorderTop + 1;
		lister->toolbar_area.box.Width = lister->window->GZZWidth;
		lister->toolbar_area.box.Height = lister->toolbar->height;
		lister->flags |= LISTERF_TOOLBAR;
	}
	else
	{
		lister->toolbar_area.box.Left = lister->window->BorderLeft;
		lister->toolbar_area.box.Top = lister->window->BorderTop;
		lister->toolbar_area.box.Width = lister->window->GZZWidth;
		lister->toolbar_area.box.Height = 0;
		lister->flags &= ~LISTERF_TOOLBAR;
	}
	lister->toolbar_area.font = lister->window->WScreen->RastPort.Font;
	display_gui_complete(&lister->toolbar_area, lister->window->RPort);

	panel_top = (lister->flags & LISTERF_TOOLBAR) ? lister->toolbar_area.rect.MaxY + 2 : lister->window->BorderTop + 1;
	path_bottom = lister->window->Height - lister->window->BorderBottom - 1;
	path_top = path_bottom - FIELD_FONT->tf_YSize - 4;
	panel_width[LISTER_DUAL_LEFT] = (lister->window->GZZWidth - gap) / 2;
	panel_width[LISTER_DUAL_RIGHT] = lister->window->GZZWidth - panel_width[LISTER_DUAL_LEFT] - gap;
	panel_left[LISTER_DUAL_LEFT] = lister->window->BorderLeft;
	panel_left[LISTER_DUAL_RIGHT] = panel_left[LISTER_DUAL_LEFT] + panel_width[LISTER_DUAL_LEFT] + gap;
	state->separator.MinX = panel_left[LISTER_DUAL_LEFT] + panel_width[LISTER_DUAL_LEFT];
	state->separator.MaxX = panel_left[LISTER_DUAL_RIGHT] - 1;
	state->separator.MinY = panel_top - 1;
	scroll_width = lister->window->BorderRight;
	if (scroll_width < 8)
		scroll_width = 8;

	for (panel = 0; panel < LISTER_DUAL_SIDES; panel++)
	{
		content_width = panel_width[panel] - scroll_width;
		if (content_width < 8)
			content_width = 8;

		state->header_rect[panel].MinX = panel_left[panel];
		state->header_rect[panel].MinY = panel_top - 1;
		state->header_rect[panel].MaxX = panel_left[panel] + content_width - 1;
		state->header_rect[panel].MaxY = panel_top + lister->window->WScreen->RastPort.Font->tf_YSize;

		state->close_area[panel].box.Left = panel_left[panel] + content_width;
		state->close_area[panel].box.Top = state->header_rect[panel].MinY;
		state->close_area[panel].box.Width = scroll_width;
		state->close_area[panel].box.Height = lister_dual_rect_height(&state->header_rect[panel]);
		state->close_area[panel].font = lister->window->WScreen->RastPort.Font;
		display_gui_complete(&state->close_area[panel], lister->window->RPort);

		state->name_area[panel].box.Left = state->header_rect[panel].MinX + 1;
		state->name_area[panel].box.Top = panel_top;
		state->name_area[panel].box.Height = lister->window->WScreen->RastPort.Font->tf_YSize;

		state->command_area[panel].box.Width = DUAL_TITLE_BUTTON_WIDTH;
		state->command_area[panel].box.Left = state->close_area[panel].box.Left - state->command_area[panel].box.Width - 2;
		if (state->command_area[panel].box.Left <= state->name_area[panel].box.Left + 8)
		{
			state->command_area[panel].box.Width = 0;
			state->command_area[panel].box.Left = state->close_area[panel].box.Left;
		}
		state->command_area[panel].box.Top = state->name_area[panel].box.Top;
		state->command_area[panel].box.Height = state->name_area[panel].box.Height;
		state->command_area[panel].font = 0;
		display_gui_complete(&state->command_area[panel], lister->window->RPort);

		state->name_area[panel].box.Width = state->command_area[panel].box.Left - state->name_area[panel].box.Left - 2;
		if (state->name_area[panel].box.Width < 8)
			state->name_area[panel].box.Width = 8;
		state->name_area[panel].font = lister->window->WScreen->RastPort.Font;
		display_gui_complete(&state->name_area[panel], lister->window->RPort);

		state->parent_area[panel].box.Left = state->command_area[panel].rect.MaxX + 1;
		state->parent_area[panel].box.Top = state->name_area[panel].rect.MinY;
		state->parent_area[panel].box.Width = 0;
		state->parent_area[panel].box.Height = state->name_area[panel].box.Height;
		state->parent_area[panel].font = 0;
		display_gui_complete(&state->parent_area[panel], lister->window->RPort);

		state->title_area[panel].box.Left = panel_left[panel];
		state->title_area[panel].box.Top = state->name_area[panel].rect.MaxY + 2;
		state->title_area[panel].box.Width = content_width;
		state->title_area[panel].box.Height = lister->window->WScreen->RastPort.Font->tf_YSize + 2;
		state->title_area[panel].font = lister->window->WScreen->RastPort.Font;
		display_gui_complete(&state->title_area[panel], lister->window->RPort);

		list_top = (lister->more_flags & LISTERF_TITLE) ? state->title_area[panel].rect.MaxY + 1
														: state->name_area[panel].rect.MaxY + 2;

		state->list_area[panel].box.Left = panel_left[panel];
		state->list_area[panel].box.Top = list_top;
		state->list_area[panel].box.Width = content_width;
		if (state->list_area[panel].box.Width < 8)
			state->list_area[panel].box.Width = 8;
		state->list_area[panel].box.Height = path_top - list_top + 1;
		if (state->list_area[panel].box.Height < 8)
			state->list_area[panel].box.Height = 8;
		state->list_area[panel].font = 0;
		display_gui_complete(&state->list_area[panel], lister->window->RPort);

		width = state->list_area[panel].box.Width - 2;
		if (lister->flags & LISTERF_KEY_SELECTION)
			width -= KEY_SEL_OFFSET;
		state->text_width[panel] = (lister->more_flags & LISTERF_PROP_FONT)
									   ? width
									   : ((width / lister->window->RPort->TxWidth) * lister->window->RPort->TxWidth);
		state->text_height[panel] = (state->list_area[panel].box.Height - 2) / lister->window->RPort->TxHeight;

		state->text_area[panel].box.Width = state->text_width[panel];
		state->text_area[panel].box.Height = state->text_height[panel] * lister->window->RPort->TxHeight;
		state->text_area[panel].box.Left = state->list_area[panel].box.Left + 1;
		if (lister->flags & LISTERF_KEY_SELECTION)
			state->text_area[panel].box.Left += KEY_SEL_OFFSET;
		state->text_area[panel].box.Top = state->list_area[panel].box.Top + 1;
		state->text_area[panel].font = 0;
		display_gui_complete(&state->text_area[panel], lister->window->RPort);

		a = state->text_width[panel] >> 1;
		if (a > 6)
			a = 6;
		else if (a < 6)
			a = 2;
		state->scroll_border[panel].MinX = state->text_area[panel].rect.MinX + a;
		state->scroll_border[panel].MaxX = state->text_area[panel].rect.MaxX - a;
		state->scroll_border[panel].MinY =
			state->text_area[panel].rect.MinY + ((state->text_height[panel] / 3) * lister->window->RPort->TxHeight);
		state->scroll_border[panel].MaxY =
			state->text_area[panel].rect.MaxY - ((state->text_height[panel] / 3) * lister->window->RPort->TxHeight);
	}

	lister->path_border.MinX = lister->window->BorderLeft;
	lister->path_border.MinY = path_top;
	lister->path_border.MaxX = lister->window->Width - lister->window->BorderRight - 1;
	lister->path_border.MaxY = path_bottom;
	state->path_border[LISTER_DUAL_LEFT] = lister->path_border;
	state->path_border[LISTER_DUAL_LEFT].MaxX = state->separator.MinX - 1;
	state->path_border[LISTER_DUAL_RIGHT] = lister->path_border;
	state->path_border[LISTER_DUAL_RIGHT].MinX = panel_left[LISTER_DUAL_RIGHT];
	state->separator.MaxY = lister->path_border.MaxY;
	if (state->separator.MaxY < state->separator.MinY)
		state->separator.MaxY = state->separator.MinY;

	lister_dual_apply_active(lister);
}

static void lister_dual_begin_panel_clip(Lister *lister, ListerDualState *state, short side)
{
	struct Rectangle clip;

	if (!lister_valid_window(lister) || !state || !lister->title_region || side < 0 || side >= LISTER_DUAL_SIDES)
		return;

	clip.MinX = state->title_area[side].rect.MinX;
	clip.MinY = state->title_area[side].rect.MinY;
	clip.MaxX = state->list_area[side].rect.MaxX;
	clip.MaxY = state->list_area[side].rect.MaxY;
	if (clip.MaxX < clip.MinX || clip.MaxY < clip.MinY)
		return;

	ClearRegion(lister->title_region);
	OrRectRegion(lister->title_region, &clip);
	InstallClipRegion(lister->window->WLayer, lister->title_region);
}

static void lister_dual_end_panel_clip(Lister *lister)
{
	if (lister_valid_window(lister))
		InstallClipRegion(lister->window->WLayer, 0);
}

static void lister_dual_draw_separator(Lister *lister, ListerDualState *state)
{
	struct RastPort *rp;
	short x;

	if (!lister_valid_window(lister) || !state || state->separator.MaxX < state->separator.MinX ||
		state->separator.MaxY < state->separator.MinY)
		return;

	rp = lister->window->RPort;
	SetAPen(rp, GUI->draw_info->dri_Pens[BACKGROUNDPEN]);
	RectFill(rp, state->separator.MinX, state->separator.MinY, state->separator.MaxX, state->separator.MaxY);

	x = (state->separator.MinX + state->separator.MaxX) >> 1;
	SetAPen(rp, GUI->draw_info->dri_Pens[SHADOWPEN]);
	Move(rp, x, state->separator.MinY);
	Draw(rp, x, state->separator.MaxY);

	if (x < state->separator.MaxX)
	{
		SetAPen(rp, GUI->draw_info->dri_Pens[SHINEPEN]);
		Move(rp, x + 1, state->separator.MinY);
		Draw(rp, x + 1, state->separator.MaxY);
	}
}

static void lister_dual_clear_toolbar_area(Lister *lister)
{
	if (!lister_valid_window(lister) || !(lister->flags & LISTERF_TOOLBAR))
		return;

	SetAPen(&lister->toolbar_area.rast, GUI->draw_info->dri_Pens[BACKGROUNDPEN]);
	RectFill(&lister->toolbar_area.rast,
			 lister->toolbar_area.rect.MinX,
			 lister->toolbar_area.rect.MinY,
			 lister->toolbar_area.rect.MaxX,
			 lister->toolbar_area.rect.MaxY);
}

static void lister_dual_draw_command_button(Lister *lister, ListerDualState *state, short side)
{
	GUI_Element *area;

	if (!lister_valid_window(lister) || !state || side < 0 || side >= LISTER_DUAL_SIDES)
		return;

	area = &state->command_area[side];
	if (area->box.Width < 1)
		return;

	lister_title_pens(&area->rast, lister, 1);
	lister_clear_title_area(area);
	SetAPen(&area->rast, area->rast.BgPen);
	SetDrMd(&area->rast, JAM1);
#ifdef __amigaos3__
	BltTemplate((char *)command_arrow_chip,
				0,
				2,
				&area->rast,
				area->rect.MinX + (area->box.Width - 8) / 2,
				area->rect.MinY + (area->box.Height - 7) / 2,
				8,
				7);
#else
	BltTemplate((char *)command_arrow,
				0,
				2,
				&area->rast,
				area->rect.MinX + (area->box.Width - 8) / 2,
				area->rect.MinY + (area->box.Height - 7) / 2,
				8,
				7);
#endif
	display_gui_border(area);
}

static void lister_dual_draw_close_button(Lister *lister, ListerDualState *state, short side)
{
	GUI_Element *area;
	struct Rectangle rect;
	short text_width;

	if (!lister_valid_window(lister) || !state || side < 0 || side >= LISTER_DUAL_SIDES)
		return;

	area = &state->close_area[side];
	rect = area->rect;
	SetAPen(&area->rast, GUI->draw_info->dri_Pens[BACKGROUNDPEN]);
	RectFill(&area->rast, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);
	do3dbox(&area->rast, &rect, 0);

	SetAPen(&area->rast, GUI->draw_info->dri_Pens[TEXTPEN]);
	SetBPen(&area->rast, GUI->draw_info->dri_Pens[BACKGROUNDPEN]);
	SetDrMd(&area->rast, JAM1);
	text_width = TextLength(&area->rast, "x", 1);
	Move(&area->rast,
		 area->box.Left + ((area->box.Width - text_width) >> 1),
		 area->box.Top + area->rast.Font->tf_Baseline + ((area->box.Height - area->rast.Font->tf_YSize) >> 1));
	Text(&area->rast, "x", 1);
	SetDrMd(&area->rast, JAM2);
}

static void lister_dual_clear_top_gutter(Lister *lister, ListerDualState *state, short side)
{
	struct Rectangle rect;

	if (!lister_valid_window(lister) || !state || side < 0 || side >= LISTER_DUAL_SIDES)
		return;

	rect.MinX = state->close_area[side].rect.MinX;
	rect.MaxX = state->path_border[side].MaxX;
	rect.MinY = state->header_rect[side].MinY;
	rect.MaxY = state->list_area[side].rect.MinY - 1;
	if (rect.MaxX < rect.MinX || rect.MaxY < rect.MinY)
		return;

	SetAPen(lister->window->RPort, GUI->draw_info->dri_Pens[BACKGROUNDPEN]);
	RectFill(lister->window->RPort, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);
}

static void lister_dual_draw_header(Lister *lister, ListerDualState *state, short side)
{
	if (!lister_valid_window(lister) || !state || side < 0 || side >= LISTER_DUAL_SIDES)
		return;

	lister_dual_clear_top_gutter(lister, state, side);
	SetAPen(&lister->name_area.rast, GUI->draw_info->dri_Pens[BACKGROUNDPEN]);
	if (state->list_area[side].rect.MinX < state->header_rect[side].MinX)
		RectFill(&lister->name_area.rast,
				 state->list_area[side].rect.MinX,
				 state->header_rect[side].MinY,
				 state->header_rect[side].MinX - 1,
				 state->header_rect[side].MaxY);
	lister_title_pens(&lister->name_area.rast, lister, 0);
	SetAPen(&lister->name_area.rast, lister->name_area.rast.BgPen);
	RectFill(&lister->name_area.rast,
			 state->header_rect[side].MinX,
			 state->header_rect[side].MinY,
			 state->header_rect[side].MaxX,
			 state->header_rect[side].MaxY);
	select_show_info(lister, 0);
	do3dbox(&lister->name_area.rast, &state->header_rect[side], 0);
	lister_dual_draw_command_button(lister, state, side);
	lister_dual_draw_close_button(lister, state, side);
}

// Draw one panel. The Lister is left showing the panel it showed before.
static void lister_dual_draw_panel(Lister *lister, short side)
{
	ListerDualState *state;
	short old_view;

	if (!(state = lister_dual_get_state(lister)) || !(state->flags & DUALF_ONE_WINDOW))
		return;

	old_view = state->view;
	lister_dual_switch_view(lister, state, side);
	lister_dual_draw_header(lister, state, side);
	lister_dual_clear_title_boxes(lister->title_boxes);
	if (lister->more_flags & LISTERF_TITLE &&
		(!lister->cur_buffer || lister->cur_buffer == lister->special_buffer ||
		 lister->cur_buffer->more_flags & DWF_HIDE_TITLE))
		lister_clear_title(lister);
	lister_init_lister_area(lister);
	lister_dual_begin_panel_clip(lister, state, side);
	if (lister->cur_buffer)
	{
		lister->cur_buffer->buf_OldVertOffset = -1;
		lister->cur_buffer->buf_OldHorizOffset = -1;
		lister_fix_horiz_len(lister);
		lister_display_dir(lister);
		lister_dual_capture_title_boxes(lister, state, side);
	}
	else
		lister_dual_clear_title_boxes(state->title_boxes[side]);
	lister_dual_end_panel_clip(lister);
	lister_dual_switch_view(lister, state, old_view);
}

// Redraw both panel headers (source/destination/busy colours and counts),
// then set the window title and fuel gauge from the active panel
static void lister_dual_draw_headers(Lister *lister, ListerDualState *state)
{
	short side;

	if (!state)
		return;
	if (!lister_valid_window(lister))
	{
		lister_dual_switch_view(lister, state, state->active);
		return;
	}

	for (side = 0; side < LISTER_DUAL_SIDES; side++)
	{
		lister_dual_switch_view(lister, state, side);
		lister_dual_draw_header(lister, state, side);
	}
	lister_dual_switch_view(lister, state, state->active);
	lister_show_name(lister);
}

// Make a panel the active (source) panel. Switching panels only changes the
// header colours and the window title, so the file lists are not redrawn.
static void lister_dual_set_active(Lister *lister, ListerDualState *state, short side)
{
	if (side < 0 || side >= LISTER_DUAL_SIDES)
		return;

	if (side == state->active)
	{
		lister_dual_switch_view(lister, state, side);
		return;
	}

	lister_dual_capture_view(lister, state);
	state->active = side;
	lister_dual_draw_headers(lister, state);
}

// Lay the panels out again after the field titles were shown or hidden
// (lister_init_filelist() would give one panel the whole window)
BOOL lister_dual_init_filelist(Lister *lister)
{
	ListerDualState *state;
	ULONG old_title;
	short view;

	if (!lister_valid_window(lister) || !(state = lister_dual_get_state(lister)) || !(state->flags & DUALF_ONE_WINDOW))
		return FALSE;

	view = state->view;
	lister_dual_capture_view(lister, state);
	old_title = lister->more_flags & LISTERF_TITLE;
	lister_dual_update_title_mode(lister, state);
	lister_dual_layout(lister, state);

	// Both panels move when the titles come or go
	if ((lister->more_flags & LISTERF_TITLE) != old_title)
		lister_dual_refresh(lister, LREFRESH_FULL);

	lister_dual_switch_view(lister, state, view);
	return TRUE;
}

BOOL lister_dual_update_pathfields(Lister *lister)
{
	ListerDualState *state;
	short side;

	if (!lister_valid_window(lister) || !(lister->flags & LISTERF_PATH_FIELD) ||
		!(state = lister_dual_get_state(lister)) || !(state->flags & DUALF_ONE_WINDOW))
		return FALSE;

	// The shown panel's slot is only updated when the Lister changes panel
	lister_dual_capture_view(lister, state);
	state->path_field[LISTER_DUAL_LEFT] = lister->path_field;
	for (side = 0; side < LISTER_DUAL_SIDES; side++)
	{
		if (state->path_field[side] && state->buffer[side])
			SetGadgetAttrs(state->path_field[side],
						   lister->window,
						   0,
						   STRINGA_TextVal,
						   (IPTR)state->buffer[side]->buf_ExpandedPath,
						   TAG_END);
	}

	return TRUE;
}

BOOL lister_dual_disable_pathfields(Lister *lister, short disable)
{
	ListerDualState *state;
	short side;

	if (!lister_valid_window(lister) || !(state = lister_dual_get_state(lister)) ||
		!(state->flags & DUALF_ONE_WINDOW))
		return FALSE;

	state->path_field[LISTER_DUAL_LEFT] = lister->path_field;
	for (side = 0; side < LISTER_DUAL_SIDES; side++)
	{
		if (state->path_field[side])
			SetGadgetAttrs(state->path_field[side], lister->window, 0, GA_Disabled, disable, TAG_END);
	}
	return TRUE;
}

BOOL lister_dual_handle_path_gadget(Lister *lister, struct Gadget *gadget)
{
	ListerDualState *state;
	short side;
	char *path;

	if (!lister_valid_window(lister) || !(state = lister_dual_get_state(lister)) ||
		!(state->flags & DUALF_ONE_WINDOW) || !gadget)
		return FALSE;

	for (side = 0; side < LISTER_DUAL_SIDES; side++)
		if (gadget == state->path_field[side])
			break;
	if (side >= LISTER_DUAL_SIDES)
		return FALSE;

	lister_dual_set_active(lister, state, side);
	path = lister_dual_path_buffer(lister, state, side);

	if (lister->hot_name_req)
		lister_rem_hotname(lister);
	else if (lister->filter_name_req)
		lister_rem_filter(lister);
	else if (!(lister->flags & LISTERF_LOCK))
	{
		if (lister->cur_buffer->buf_CustomHandler[0])
		{
			rexx_handler_msg(0,
							 lister->cur_buffer,
							 RXMF_WARN,
							 HA_String,
							 0,
							 "path",
							 HA_Value,
							 1,
							 lister,
							 HA_String,
							 2,
							 path,
							 TAG_END);
			return TRUE;
		}

		if (!(lister->flags & LISTERF_SOURCEDEST_LOCK))
		{
			lister_do_function(lister, MENU_LISTER_SOURCE);
			if (!(lister->flags & LISTERF_SOURCE))
				lister->flags |= LISTERF_STORED_SOURCE;
		}

		if (!(lister_check_ftp(lister, path)))
		{
			if (stricmp(path, lister->cur_buffer->buf_ExpandedPath))
				read_directory(lister, path, GETDIRF_CANMOVEEMPTY | GETDIRF_CANCHECKBUFS);
			else
				read_directory(lister, path, 0);
		}
	}

	return TRUE;
}

BOOL lister_dual_activate_pathfield(Lister *lister)
{
	ListerDualState *state;
	struct Gadget *gadget;

	if (!lister_valid_window(lister) || !(state = lister_dual_get_state(lister)) ||
		!(state->flags & DUALF_ONE_WINDOW))
		return FALSE;

	state->path_field[LISTER_DUAL_LEFT] = lister->path_field;
	gadget = state->path_field[state->active];
	if (!gadget)
		return FALSE;

	lister_dual_switch_view(lister, state, state->active);
	ActivateStrGad(gadget, lister->window);
	return TRUE;
}

BOOL lister_dual_update_sliders(Lister *lister, int which)
{
	ListerDualState *state;
	DirBuffer *buffer;
	short side;

	if (!lister_valid_window(lister) || lister->flags & LISTERF_VIEW_ICONS || lister->more_flags & LISTERF_TITLEBARRED ||
		!(state = lister_dual_get_state(lister)) || !(state->flags & DUALF_ONE_WINDOW))
		return FALSE;

	lister_dual_capture_side_offsets(lister, state, state->view);

	for (side = 0; side < LISTER_DUAL_SIDES; side++)
	{
		if (!(buffer = state->buffer[side]))
			continue;

		lister_dual_init_side_offsets(state, side);

		if ((which & SLIDER_VERT) && state->vert_scroller[side])
		{
			if (state->vert_offset[side] + state->text_height[side] > buffer->buf_TotalEntries[0])
				state->vert_offset[side] = buffer->buf_TotalEntries[0] - state->text_height[side];
			if (state->vert_offset[side] < 0)
				state->vert_offset[side] = 0;
			SetGadgetAttrs(state->vert_scroller[side],
						   lister->window,
						   0,
						   PGA_Top,
						   state->vert_offset[side],
						   (which & UPDATE_POS_ONLY) ? TAG_IGNORE : PGA_Total,
						   buffer->buf_TotalEntries[0],
						   (which & UPDATE_POS_ONLY) ? TAG_IGNORE : PGA_Visible,
						   state->text_height[side],
						   TAG_END);
		}

		if ((which & SLIDER_HORZ) && state->horiz_scroller[side])
		{
			if (!(lister->more_flags & LISTERF_PROP_FONT))
				state->horiz_offset[side] =
					(state->horiz_offset[side] / lister->window->RPort->TxWidth) * lister->window->RPort->TxWidth;
			if (state->horiz_offset[side] >= (buffer->buf_HorizLength - state->text_width[side]))
				state->horiz_offset[side] = buffer->buf_HorizLength - state->text_width[side];
			if (state->horiz_offset[side] < 0)
				state->horiz_offset[side] = 0;
			SetGadgetAttrs(state->horiz_scroller[side],
						   lister->window,
						   0,
						   PGA_Top,
						   state->horiz_offset[side],
						   (which & UPDATE_POS_ONLY) ? TAG_IGNORE : PGA_Total,
						   buffer->buf_HorizLength,
						   (which & UPDATE_POS_ONLY) ? TAG_IGNORE : PGA_Visible,
						   state->text_width[side],
						   TAG_END);
		}
	}

	lister_dual_apply_side_offsets(state, state->view);
	return TRUE;
}

BOOL lister_dual_handle_scroll_gadget(Lister *lister, struct Gadget *gadget, short which)
{
	ListerDualState *state;
	short side, x_dir = 0, y_dir = 0;
	BOOL found = FALSE;

	if (!lister_valid_window(lister) || !(state = lister_dual_get_state(lister)) ||
		!(state->flags & DUALF_ONE_WINDOW) || !gadget)
		return FALSE;

	for (side = 0; side < LISTER_DUAL_SIDES; side++)
	{
		if (gadget == state->vert_scroller[side] || gadget == state->horiz_scroller[side])
		{
			found = TRUE;
			break;
		}
		if (gadget == state->vert_arrow_up[side])
		{
			y_dir = -1;
			found = TRUE;
			break;
		}
		if (gadget == state->vert_arrow_down[side])
		{
			y_dir = 1;
			found = TRUE;
			break;
		}
		if (gadget == state->horiz_arrow_left[side])
		{
			x_dir = -1;
			found = TRUE;
			break;
		}
		if (gadget == state->horiz_arrow_right[side])
		{
			x_dir = 1;
			found = TRUE;
			break;
		}
	}

	if (!found)
		return FALSE;

	// Scroll the panel without changing which panel is the source
	lister_dual_switch_view(lister, state, side);

	if (x_dir || y_dir)
		lister_scroll(lister, x_dir, y_dir);
	else
		lister_pos_slider(lister, which);

	lister_dual_switch_view(lister, state, state->active);
	return TRUE;
}

// Is this lister part of a dual lister group?
BOOL lister_dual_is_side(Lister *lister)
{
	return (lister && lister->flags2 & LISTERF2_DUAL) ? TRUE : FALSE;
}

// Is this lister using the one-window dual panel model?
BOOL lister_dual_one_window(Lister *lister)
{
	ListerDualState *state;

	return ((state = lister_dual_get_state(lister)) && state->flags & DUALF_ONE_WINDOW) ? TRUE : FALSE;
}

// Initialise the single-window dual panel layout
BOOL lister_dual_init_display(Lister *lister)
{
	ListerDualState *state;

	if (!lister_valid_window(lister) || !lister_dual_one_window(lister) || lister->flags & LISTERF_VIEW_ICONS)
		return FALSE;
	if (!(state = lister_dual_get_state(lister)))
		return FALSE;

	lister_dual_prepare_frame_refresh(lister, state);
	lister_dual_prepare_resize_refresh(lister);
	lister_dual_update_title_mode(lister, state);
	lister_dual_layout(lister, state);

	if (!(lister->flags & LISTERF_PATH_FIELD))
	{
		if (!lister_dual_add_pathfield(lister, state))
			return FALSE;
	}
	if (!lister_dual_ensure_pathfields(lister, state) || !lister_dual_ensure_scrollbars(lister, state))
		return FALSE;
	lister_update_pathfield(lister);
	if (lister->flags & LISTERF_LOCK)
		lister_dual_disable_pathfields(lister, TRUE);

	lister_set_limits(lister);
	return TRUE;
}

// Refresh the single-window dual panel display
void lister_dual_refresh(Lister *lister, unsigned short mode)
{
	ListerDualState *state;
	short side;

	if (!lister_valid_window(lister) || !(state = lister_dual_get_state(lister)) || !(state->flags & DUALF_ONE_WINDOW))
		return;

	lister_dual_capture_view(lister, state);

	if (mode & LREFRESH_REFRESH)
		BeginRefresh(lister->window);

	lister_dual_init_display(lister);

	if (lister->flags & LISTERF_TOOLBAR)
	{
		lister_dual_clear_toolbar_area(lister);
		lister_show_toolbar(lister);
	}

	for (side = 0; side < LISTER_DUAL_SIDES; side++)
		lister_dual_draw_panel(lister, side);
	lister_dual_draw_separator(lister, state);

	lister_dual_switch_view(lister, state, state->active);
	lister_fix_horiz_len(lister);
	lister_update_slider(lister, SLIDER_VERT | SLIDER_HORZ);
	if (lister->flags & LISTERF_PATH_FIELD)
	{
		lister_update_pathfield(lister);
		if (mode & LREFRESH_FULL)
			lister_dual_refresh_pathfields(lister, state);
	}

	// Window title and fuel gauge follow the active panel
	lister_show_name(lister);

	if (mode & LREFRESH_REFRESH)
		EndRefresh(lister->window, TRUE);
	lister_dual_finish_frame_refresh(lister);
}

// Resort the buffer the Lister currently shows, as lister_refresh_display does
static void lister_dual_resort_view(Lister *lister, ULONG flags)
{
	lister_resort(lister, 0);
	if (flags & REFRESHF_SORTSEL && lister->cur_buffer)
		buffer_sort_selected(lister->cur_buffer);
}

// Refresh the panel the Lister currently shows, and the other panel if it
// shows the same buffer
static void lister_dual_refresh_view_panel(Lister *lister, ListerDualState *state, ULONG flags)
{
	short side;

	side = state->view;
	lister_dual_capture_view(lister, state);

	if (flags & REFRESHF_RESORT)
		lister_dual_resort_view(lister, flags);

	if (!lister_valid_window(lister))
		return;

	if (flags & REFRESHF_UPDATE_NAME)
		lister_refresh_name(lister);

	if (flags & REFRESHF_SLIDERS || lister->more_flags & LISTERF_NEED_RECALC)
		lister_fix_horiz_len(lister);

	lister_dual_draw_panel(lister, side);
	if (state->buffer[lister_dual_other_side(side)] == state->buffer[side])
		lister_dual_draw_panel(lister, lister_dual_other_side(side));

	if (flags & REFRESHF_SLIDERS)
		lister_update_slider(lister, SLIDER_VERT | SLIDER_HORZ);
	if (flags & REFRESHF_PATH)
		lister_update_pathfield(lister);

	// The title shows the active panel; a destination refresh must not take it
	if (flags & (REFRESHF_UPDATE_NAME | REFRESHF_NAME))
	{
		lister_dual_switch_view(lister, state, state->active);
		lister_show_name(lister);
		lister_dual_switch_view(lister, state, side);
	}
}

// Refresh the dual display from lower-level refresh paths. Partial refreshes
// apply to the panel the Lister currently shows (the lister process selects
// the target panel before it refreshes).
void lister_dual_refresh_display(Lister *lister, ULONG flags)
{
	ListerDualState *state;

	if (!(state = lister_dual_get_state(lister)) || !(state->flags & DUALF_ONE_WINDOW))
		return;

	if (flags & (REFRESHF_FULL | REFRESHF_REFRESH | REFRESHF_COLOUR))
	{
		if (flags & REFRESHF_RESORT)
		{
			short side, view = state->view;

			for (side = 0; side < LISTER_DUAL_SIDES; side++)
			{
				if (side == LISTER_DUAL_RIGHT && state->buffer[side] == state->buffer[LISTER_DUAL_LEFT])
					break;
				lister_dual_switch_view(lister, state, side);
				lister_dual_resort_view(lister, flags);
			}
			lister_dual_switch_view(lister, state, view);
		}

		lister_dual_refresh(lister, (flags & REFRESHF_REFRESH) ? LREFRESH_REFRESH : LREFRESH_FULL);
		return;
	}

	lister_dual_refresh_view_panel(lister, state, flags);
}

// Refresh both panels for a request that names no panel (ARexx "lister
// refresh", module callbacks)
BOOL lister_dual_refresh_all(Lister *lister, ULONG flags)
{
	ListerDualState *state;
	short side;

	if (!(state = lister_dual_get_state(lister)) || !(state->flags & DUALF_ONE_WINDOW))
		return FALSE;

	if (flags & (REFRESHF_FULL | REFRESHF_REFRESH | REFRESHF_COLOUR))
	{
		lister_refresh_display(lister, flags);
		return TRUE;
	}

	for (side = 0; side < LISTER_DUAL_SIDES; side++)
	{
		if (side == LISTER_DUAL_RIGHT && state->buffer[side] == state->buffer[LISTER_DUAL_LEFT])
			break;
		lister_dual_switch_view(lister, state, side);
		lister_refresh_display(lister, flags);
	}
	lister_dual_switch_view(lister, state, state->active);
	return TRUE;
}

void lister_dual_close_display(Lister *lister)
{
	ListerDualState *state;

	if (!(state = lister_dual_get_state(lister)) || !(state->flags & DUALF_ONE_WINDOW))
		return;

	lister_dual_close_pathfields(lister, state);
	lister_dual_close_scrollbars(lister, state);
}

static void lister_dual_close_side(Lister *lister, short side)
{
	ListerDualState *state;
	short keep_side;

	if (!(state = lister_dual_get_state(lister)) || !(state->flags & DUALF_ONE_WINDOW))
		return;
	if (side < 0 || side >= LISTER_DUAL_SIDES)
		return;

	keep_side = lister_dual_other_side(side);
	state->active = keep_side;
	lister_dual_leave(lister);
}

BOOL lister_dual_handle_close_button(Lister *lister, UWORD x, UWORD y)
{
	ListerDualState *state;
	short side;

	if (!(state = lister_dual_get_state(lister)) || !(state->flags & DUALF_ONE_WINDOW))
		return FALSE;

	for (side = 0; side < LISTER_DUAL_SIDES; side++)
	{
		if (point_in_element(&state->close_area[side], x, y))
		{
			lister_dual_close_side(lister, side);
			return TRUE;
		}
	}

	return FALSE;
}

// Show help for a dual-panel coordinate
BOOL lister_dual_help_at(Lister *lister, UWORD x, UWORD y)
{
	ListerDualState *state;
	short side;

	if (!(state = lister_dual_get_state(lister)) || !(state->flags & DUALF_ONE_WINDOW))
		return FALSE;

	for (side = 0; side < LISTER_DUAL_SIDES; side++)
	{
		if (point_in_element(&state->command_area[side], x, y))
		{
			help_show_help(HELP_LISTER_MENU_POPUP, 0);
			return TRUE;
		}
		if (point_in_element(&state->name_area[side], x, y))
		{
			help_show_help(HELP_LISTER_FILE_COUNT, 0);
			return TRUE;
		}
	}

	return FALSE;
}

// Activate the panel under a mouse coordinate
void lister_dual_activate_at(Lister *lister, UWORD x, UWORD y)
{
	ListerDualState *state;
	short side;

	if (!(state = lister_dual_get_state(lister)) || !(state->flags & DUALF_ONE_WINDOW))
		return;

	for (side = 0; side < LISTER_DUAL_SIDES; side++)
	{
		if (point_in_element(&state->list_area[side], x, y) || point_in_element(&state->name_area[side], x, y) ||
			point_in_element(&state->command_area[side], x, y) ||
			point_in_element(&state->title_area[side], x, y))
		{
			lister_dual_set_active(lister, state, side);
			break;
		}
	}
}

// Activate the inactive panel
void lister_dual_activate_other(Lister *lister)
{
	ListerDualState *state;

	if (!(state = lister_dual_get_state(lister)) || !(state->flags & DUALF_ONE_WINDOW))
		return;

	lister_dual_set_active(lister, state, lister_dual_other_side(state->active));
}

// Return the inactive panel index for dual command destinations
short lister_dual_dest_index(Lister *lister)
{
	ListerDualState *state;

	if ((state = lister_dual_get_state(lister)) && state->flags & DUALF_ONE_WINDOW)
		return lister_dual_other_side(state->active);

	return -1;
}

// Load the active panel into the Lister (lister process)
Lister *lister_dual_active_side(Lister *lister)
{
	ListerDualState *state;

	if ((state = lister_dual_get_state(lister)) && state->flags & DUALF_ONE_WINDOW)
		lister_dual_switch_view(lister, state, state->active);

	return lister;
}

// Return the active panel index, or -1 for a normal lister
short lister_dual_active_index(Lister *lister)
{
	ListerDualState *state;

	if ((state = lister_dual_get_state(lister)) && state->flags & DUALF_ONE_WINDOW)
		return state->active;

	return -1;
}

// Panel a function launched now works on: the panel the lister process has
// loaded when it launches the function itself (a rescan of the destination,
// say), otherwise the active panel
short lister_dual_launch_side(Lister *lister)
{
	ListerDualState *state;

	if (!(state = lister_dual_get_state(lister)) || !(state->flags & DUALF_ONE_WINDOW))
		return -1;

	if (lister->ipc && (struct Task *)lister->ipc->proc == FindTask(0))
		return state->view;

	return state->active;
}

// Load the panel that shows a buffer; FALSE if no panel shows it. For a
// normal lister this only tests whether it shows the buffer.
BOOL lister_dual_view_buffer(Lister *lister, DirBuffer *buffer)
{
	ListerDualState *state;

	if (lister->cur_buffer == buffer)
		return TRUE;

	if ((state = lister_dual_get_state(lister)) && state->flags & DUALF_ONE_WINDOW &&
		state->buffer[lister_dual_other_side(state->view)] == buffer)
	{
		lister_dual_switch_view(lister, state, lister_dual_other_side(state->view));
		return TRUE;
	}

	return FALSE;
}

// Load a panel for one lister-process operation without changing the active
// panel; lister_dual_active_side() returns to the active panel afterwards
void lister_dual_view_side(Lister *lister, short side)
{
	ListerDualState *state;

	if ((state = lister_dual_get_state(lister)) && state->flags & DUALF_ONE_WINDOW && side >= 0 &&
		side < LISTER_DUAL_SIDES)
		lister_dual_switch_view(lister, state, side);
}

// Buffer shown in a panel. Function and ARexx processes use this instead of
// lister->cur_buffer, which only the lister process may switch between panels.
DirBuffer *lister_dual_side_buffer(Lister *lister, short side)
{
	ListerDualState *state;
	DirBuffer *buffer;

	if (side < 0 || side >= LISTER_DUAL_SIDES)
		return lister->cur_buffer;

	Forbid();
	buffer = lister->cur_buffer;
	if ((state = lister_dual_get_state(lister)) && state->flags & DUALF_ONE_WINDOW && side != state->view &&
		state->buffer[side])
		buffer = state->buffer[side];
	Permit();

	// The panel the Lister shows may have a newer buffer than its slot, so
	// that panel is read from cur_buffer
	return buffer;
}

// Panel under a window-relative coordinate, or -1
short lister_dual_side_at(Lister *lister, short x, short y)
{
	ListerDualState *state;

	if (!(state = lister_dual_get_state(lister)) || !(state->flags & DUALF_ONE_WINDOW))
		return -1;

	if (y < state->header_rect[LISTER_DUAL_LEFT].MinY || y > state->separator.MaxY)
		return -1;
	if (x < state->separator.MinX)
		return LISTER_DUAL_LEFT;
	if (x > state->separator.MaxX)
		return LISTER_DUAL_RIGHT;
	return -1;
}

// Does the panel the Lister does not show display this buffer? A panel that
// shows a device list still holds the buffer it will return to.
BOOL lister_dual_other_side_shows(Lister *lister, DirBuffer *buffer)
{
	ListerDualState *state;
	DirBuffer *other;

	if (!buffer || !(state = lister_dual_get_state(lister)) || !(state->flags & DUALF_ONE_WINDOW))
		return FALSE;

	other = state->buffer[lister_dual_other_side(state->view)];
	return (other == buffer ||
			(other == lister->special_buffer && state->old_buffer[lister_dual_other_side(state->view)] == buffer))
			   ? TRUE
			   : FALSE;
}

// Mark the panel a drop is aimed at; function_launch() takes it for the
// destination path of the function it starts
void lister_dual_set_drop_side(Lister *lister, short side)
{
	ListerDualState *state;

	if ((state = lister_dual_get_state(lister)) && state->flags & DUALF_ONE_WINDOW)
		state->drop_side = side;
}

short lister_dual_take_drop_side(Lister *lister)
{
	ListerDualState *state;
	short side;

	if (!(state = lister_dual_get_state(lister)) || !(state->flags & DUALF_ONE_WINDOW))
		return -1;

	side = state->drop_side;
	state->drop_side = -1;
	return side;
}

// Panel that holds the drag-over highlight
short lister_dual_highlight_side(Lister *lister)
{
	ListerDualState *state;

	if (!(state = lister_dual_get_state(lister)) || !(state->flags & DUALF_ONE_WINDOW))
		return -1;

	return (state->highlight_side >= 0) ? state->highlight_side : state->active;
}

void lister_dual_set_highlight_side(Lister *lister, short side)
{
	ListerDualState *state;

	if ((state = lister_dual_get_state(lister)) && state->flags & DUALF_ONE_WINDOW)
		state->highlight_side = side;
}

// Remember that the panel the Lister shows needs a rescan once it is not busy
void lister_dual_queue_rescan(Lister *lister)
{
	ListerDualState *state;

	if ((state = lister_dual_get_state(lister)) && state->flags & DUALF_ONE_WINDOW)
		state->rescan_sides |= 1 << state->view;
}

// Rescan a panel queued by lister_dual_queue_rescan(). Each rescan is a
// function that locks the lister, so one panel is read at a time and the
// lister is flagged to rescan again when it is free.
BOOL lister_dual_do_rescans(Lister *lister)
{
	ListerDualState *state;
	short side;

	if (!(state = lister_dual_get_state(lister)) || !(state->flags & DUALF_ONE_WINDOW) || !state->rescan_sides)
		return FALSE;

	for (side = 0; side < LISTER_DUAL_SIDES; side++)
		if (state->rescan_sides & (1 << side))
			break;

	state->rescan_sides &= ~(1 << side);
	if (state->rescan_sides)
		lister->flags |= LISTERF_RESCAN;

	lister_dual_switch_view(lister, state, side);
	read_directory(lister, lister->cur_buffer->buf_Path, GETDIRF_RESELECT);
	lister_dual_switch_view(lister, state, state->active);
	return TRUE;
}

// Working format of a panel (side -1 or a normal lister: lister->format).
// Function processes use these while the lister process shows either panel.
void lister_dual_get_format(Lister *lister, short side, ListFormat *format)
{
	ListerDualState *state;

	Forbid();
	if (side >= 0 && side < LISTER_DUAL_SIDES && (state = lister_dual_get_state(lister)) &&
		state->flags & DUALF_ONE_WINDOW && side != state->view)
		*format = state->format[side];
	else
		*format = lister->format;
	Permit();
}

void lister_dual_set_format(Lister *lister, short side, ListFormat *format)
{
	ListerDualState *state;

	Forbid();
	if (side >= 0 && side < LISTER_DUAL_SIDES && (state = lister_dual_get_state(lister)) &&
		state->flags & DUALF_ONE_WINDOW && side != state->view)
		state->format[side] = *format;
	else
		lister->format = *format;
	Permit();
}

// The shown panel is about to show the special buffer (device or buffer
// list). There is one special buffer per lister, so if the other panel shows
// it, that panel returns to the directory it came from first.
void lister_dual_release_special(Lister *lister)
{
	ListerDualState *state;
	short other;

	if (!(state = lister_dual_get_state(lister)) || !(state->flags & DUALF_ONE_WINDOW) ||
		lister->cur_buffer == lister->special_buffer)
		return;

	other = lister_dual_other_side(state->view);
	if (state->buffer[other] != lister->special_buffer || !state->old_buffer[other])
		return;

	state->buffer[other] = state->old_buffer[other];
	state->old_buffer[other] = 0;
	if (lister_valid_window(lister))
		lister_dual_draw_panel(lister, other);
}

// Show the busy/source/destination state of both panels
void lister_dual_show_status(Lister *lister)
{
	ListerDualState *state;

	if ((state = lister_dual_get_state(lister)) && state->flags & DUALF_ONE_WINDOW)
	{
		lister_dual_capture_view(lister, state);
		lister_dual_draw_headers(lister, state);
	}
}

// Detach a lister from dual mode without closing it
static void lister_dual_detach(Lister *lister)
{
	if (lister)
	{
		lister->flags2 &= ~LISTERF2_DUAL;
		lister_dual_free_state(lister);
	}
}

// Normal buffer a panel the Lister does not show holds: its own, or the one it
// returns to from a device list
static DirBuffer *lister_dual_held_buffer(Lister *lister, ListerDualState *state, short side)
{
	return (state->buffer[side] == lister->special_buffer) ? state->old_buffer[side] : state->buffer[side];
}

// Release the lock a panel holds on a buffer the Lister no longer shows
static void lister_dual_release_buffer(Lister *lister, DirBuffer *buffer, short arg)
{
	if (!buffer || buffer == lister->cur_buffer || buffer == lister->special_buffer ||
		(buffer == lister->old_buffer && lister->cur_buffer == lister->special_buffer))
		return;

	lock_listlock(&GUI->buffer_list, TRUE);
	if (buffer->buf_CurrentLister == lister)
		buffer_clear_lock(buffer, arg);
	unlock_listlock(&GUI->buffer_list);
}

// Detach a closing dual lister; lister_cleanup() unlocks the active buffer
void lister_dual_detach_closing(Lister *lister)
{
	ListerDualState *state;
	DirBuffer *other = 0;

	if ((state = lister_dual_get_state(lister)) && state->flags & DUALF_ONE_WINDOW)
	{
		lister_dual_capture_view(lister, state);
		other = lister_dual_held_buffer(lister, state, lister_dual_other_side(state->view));
	}
	lister_dual_detach(lister);
	lister_dual_release_buffer(lister, other, 1);
}

// Remove external source/destination state while a dual lister owns local roles
static void lister_dual_clear_external_roles(Lister *dual)
{
	Lister *lister;
	IPCData *ipc;

	if (!dual)
		return;

	lock_listlock(&GUI->lister_list, FALSE);
	for (ipc = (IPCData *)GUI->lister_list.list.lh_Head; ipc->node.mln_Succ; ipc = (IPCData *)ipc->node.mln_Succ)
	{
		lister = IPCDATA(ipc);
		if (lister == dual || lister_dual_is_side(lister))
			continue;
		if (lister->flags & (LISTERF_SOURCE | LISTERF_DEST | LISTERF_STORED_SOURCE | LISTERF_STORED_DEST | LISTERF_SOURCEDEST_LOCK))
			IPC_Command(lister->ipc, LISTER_OFF, 0, 0, 0, 0);
	}
	unlock_listlock(&GUI->lister_list);
}

// Set the local source/destination roles for both sides of a dual lister
static void lister_dual_set_roles(Lister *source, BOOL locked)
{
	ListerDualState *state;

	if ((state = lister_dual_get_state(source)) && state->flags & DUALF_ONE_WINDOW)
	{
		state->locked = locked;
		lister_dual_clear_external_roles(source);
		lister_dual_switch_view(source, state, state->active);
		lister_dual_draw_headers(source, state);
		return;
	}
}

// Make one side active/source and the other side destination
void lister_dual_activate(Lister *lister)
{
	ListerDualState *state;

	if ((state = lister_dual_get_state(lister)) && state->flags & DUALF_ONE_WINDOW)
		lister_dual_switch_view(lister, state, state->active);
}

// Latch one side as source and the other side as destination; this is not a task lock.
void lister_dual_lock_source(Lister *lister)
{
	lister_dual_set_roles(lister, TRUE);
}

// Release the role latch without breaking the local source/destination roles.
void lister_dual_unlock(Lister *lister)
{
	ListerDualState *state;

	if ((state = lister_dual_get_state(lister)) && state->flags & DUALF_ONE_WINDOW)
	{
		state->locked = FALSE;
		lister_dual_switch_view(lister, state, state->active);
		lister_dual_draw_headers(lister, state);
		return;
	}
}

// Enter dual lister mode
BOOL lister_dual_enter(Lister *lister)
{
	ListerDualState *state;

	if (!lister)
		return FALSE;

	// Dual listers are Name-mode only, and need the full window
	if (lister->flags & LISTERF_VIEW_ICONS || lister->more_flags & LISTERF_TITLEBARRED)
		return FALSE;

	if (lister_dual_is_side(lister))
	{
		lister_dual_activate(lister);
		return TRUE;
	}

	if (!(state = lister_dual_ensure_state(lister)))
		return FALSE;

	state->flags |= DUALF_ONE_WINDOW;
	state->active = LISTER_DUAL_LEFT;
	state->view = LISTER_DUAL_LEFT;
	state->locked = FALSE;
	state->buffer[LISTER_DUAL_LEFT] = lister->cur_buffer;
	state->buffer[LISTER_DUAL_RIGHT] = lister->cur_buffer;
	state->format[LISTER_DUAL_LEFT] = state->format[LISTER_DUAL_RIGHT] = lister->format;
	state->old_buffer[LISTER_DUAL_LEFT] = state->old_buffer[LISTER_DUAL_RIGHT] = lister->old_buffer;

	lister->flags2 |= LISTERF2_DUAL;
	lister_dual_apply_panel(lister, state->active);
	lister_dual_clear_external_roles(lister);
	lister_check_source(lister);
	if (lister_valid_window(lister))
	{
		lister_init_display(lister);
		lister_refresh(lister, LREFRESH_FULL);
	}

	return TRUE;
}

// Leave dual lister mode, keeping the lister that requested the toggle
void lister_dual_leave(Lister *lister)
{
	ListerDualState *state;

	if ((state = lister_dual_get_state(lister)) && state->flags & DUALF_ONE_WINDOW)
	{
		DirBuffer *dropped;

		lister_dual_switch_view(lister, state, state->active);
		dropped = lister_dual_held_buffer(lister, state, lister_dual_other_side(state->active));
		lister->flags2 &= ~LISTERF2_DUAL;
		lister_dual_free_state(lister);
		lister_dual_release_buffer(lister, dropped, 0);

		lister->flags |= LISTERF_SOURCE;
		lister->flags &= ~(LISTERF_DEST | LISTERF_STORED_SOURCE | LISTERF_STORED_DEST | LISTERF_SOURCEDEST_LOCK);
		lister_check_source(lister);
		if (lister_valid_window(lister))
		{
			lister_init_display(lister);
			lister_refresh(lister, LREFRESH_FULL);
			lister_fix_menus(lister, 0);
		}
		return;
	}

	if (lister)
	{
		lister->flags2 &= ~LISTERF2_DUAL;
		lister_dual_free_state(lister);
	}
}

// Toggle dual lister mode
BOOL lister_dual_toggle(Lister *lister, short mode)
{
	if (!lister)
		return FALSE;

	if (mode == LISTER_DUAL_TOGGLE)
		mode = (lister_dual_is_side(lister)) ? LISTER_DUAL_OFF : LISTER_DUAL_ON;

	if (mode == LISTER_DUAL_OFF)
	{
		if (lister_dual_is_side(lister))
			lister_dual_leave(lister);
		return TRUE;
	}

	if (mode == LISTER_DUAL_ON)
		return lister_dual_enter(lister);

	return FALSE;
}
