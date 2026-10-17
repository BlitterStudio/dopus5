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

// Handle backdrop icon idcmp stuff
BOOL backdrop_idcmp(BackdropInfo *info, struct IntuiMessage *msg, unsigned short flags)
{
	BOOL handled = 1;
	BOOL busy = 0;

	// Lister locked?
	if (info->lister && info->lister->flags & LISTERF_LOCK)
		busy = 1;

	// Message class?
	switch (msg->Class)
	{
	// Size change
	case IDCMP_NEWSIZE:
		backdrop_update_size(info);
		break;

	// Refresh window
	case IDCMP_REFRESHWINDOW:

		// If we haven't already refreshed
		if (!(info->flags & BDIF_REFRESHED))
		{
			UWORD ref_flags;

			// Get flags for refreshing
			ref_flags = BDSF_REFRESH;
			if (!(flags & BIDCMPF_LISTER))
				ref_flags |= BDSF_REFRESH_DONE;

			// From a resize?
			if (info->flags & BDIF_NEW_SIZE)
				ref_flags |= BDSF_RECALC;

			// Otherwise, check that size is ok
			else
				backdrop_check_size(info);

			// Refresh objects
			backdrop_show_objects(info, ref_flags);
		}

		// Clear flags
		info->flags &= ~(BDIF_NEW_SIZE | BDIF_REFRESHED);

		// Got a lister?
		if (info->lister)
		{
			// Clear 'sized' flag in lister
			info->lister->flags &= ~LISTERF_RESIZED;

			// Save event for lister too
			if (flags & BIDCMPF_LISTER)
				handled = 0;
		}
		break;

	// Mouse moved
	case IDCMP_MOUSEMOVE:

		// Rubber banding?
		if (info->flags & BDIF_RUBBERBAND)
		{
			short x, y;

			// Get coordinates, clip to borders
			x = info->window->MouseX;
			y = info->window->MouseY;
			if (x < info->size.MinX)
				x = info->size.MinX;
			if (x > info->size.MaxX)
				x = info->size.MaxX;
			if (y < info->size.MinY)
				y = info->size.MinY;
			if (y > info->size.MaxY)
				y = info->size.MaxY;

			// Have coordinates changed?
			if (info->select.MaxX != x || info->select.MaxY != y)
			{
				// Erase old box
				backdrop_rubber_band(info, 0);

				// New coordinates
				info->select.MaxX = x;
				info->select.MaxY = y;

				// Show new box
				backdrop_rubber_band(info, 1);
			}
			break;
		}

		// Are we dragging something?
		else if (info->flags & BDIF_DRAGGING && info->last_sel_object)
		{
			// Show drag objects
			backdrop_show_drag(
				info, info->last_sel_object, info->window->WScreen->MouseX, info->window->WScreen->MouseY);
			break;
		}

		// No gadget down, or no real-time scrolling?
		if (!info->down_gadget || !(environment->env->display_options & DISPOPTF_REALTIME_SCROLL))
		{
			// Not handled
			handled = 0;
			break;
		}

	// Tick
	case IDCMP_INTUITICKS:

		// Tick?
		if (msg->Class == IDCMP_INTUITICKS)
		{
			// Increment tick count
			++info->tick_count;

			// No gadget down?
			if (!info->down_gadget)
			{
				// IDCMP_INTUITICKS must stay subscribed: the lister's toolbar
				// tooltip timer runs off it. Stripping it here (backdrop is
				// the primary handler in icon view) permanently killed the
				// tooltip once the user returned to name mode.
				//
				// Also let the dispatcher fall through to lister_process_msg
				// so the tooltip tick handler runs in icon/icon-action mode
				// too - otherwise hovering the toolbar in Icon Action never
				// fires tooltip_tick because backdrop_idcmp returned "handled".
				handled = 0;
				break;
			}

			// Ignore this gadget?
			if (info->flags & BDIF_IGNORE_GADGET)
			{
				info->flags &= ~BDIF_IGNORE_GADGET;
				break;
			}
		}

		// Fake gadget down
		msg->IAddress = info->down_gadget;

	// Gadget down
	case IDCMP_GADGETDOWN:

		// Store gadget
		if (info->down_gadget != (struct Gadget *)msg->IAddress)
		{
			info->down_gadget = (struct Gadget *)msg->IAddress;
			info->flags |= BDIF_IGNORE_GADGET;
			ModifyIDCMP(info->window, info->window->IDCMPFlags | IDCMP_INTUITICKS);
		}

		// Check gadget is selected
		if (!(info->down_gadget->Flags & GFLG_SELECTED))
			break;

		// Look at gadget ID
		switch (info->down_gadget->GadgetID)
		{
		// Up arrow
		case GAD_VERT_ARROW_UP:
			backdrop_scroll(info, 0, -1);
			break;

		// Down arrow
		case GAD_VERT_ARROW_DOWN:
			backdrop_scroll(info, 0, 1);
			break;

		// Left arrow
		case GAD_HORIZ_ARROW_LEFT:
			backdrop_scroll(info, -1, 0);
			break;

		// Right arrow
		case GAD_HORIZ_ARROW_RIGHT:
			backdrop_scroll(info, 1, 0);
			break;

		// Vertical slider
		case GAD_VERT_SCROLLER:
			if (environment->env->display_options & DISPOPTF_REALTIME_SCROLL)
				backdrop_pos_slider(info, SLIDER_VERT);
			break;

		// Horizontal slider
		case GAD_HORIZ_SCROLLER:
			if (environment->env->display_options & DISPOPTF_REALTIME_SCROLL)
				backdrop_pos_slider(info, SLIDER_HORZ);
			break;
		}
		break;

	// Gadget up
	case IDCMP_GADGETUP:
		// IDCMP_INTUITICKS stays subscribed for lister tooltip hover tracking.
		info->down_gadget = 0;
		info->flags &= ~BDIF_IGNORE_GADGET;

		// Look at gadget ID
		switch (((struct Gadget *)msg->IAddress)->GadgetID)
		{
		// Vertical slider
		case GAD_VERT_SCROLLER:
			backdrop_pos_slider(info, SLIDER_VERT);
			break;

		// Horizontal slider
		case GAD_HORIZ_SCROLLER:
			backdrop_pos_slider(info, SLIDER_HORZ);
			break;

		// Another gadget
		default:
			handled = 0;
			break;
		}
		break;

	// Mouse buttons
	case IDCMP_MOUSEBUTTONS:

		// Ignore if busy
		if (busy)
			break;

		// Clear down gadget (IDCMP_INTUITICKS stays on for hover tracking)
		info->down_gadget = 0;
		info->flags &= ~BDIF_IGNORE_GADGET;

		// Handle button
		handled = backdrop_handle_button(info, msg, flags);

		// Remember the last button pushed
		info->last_button = msg->Code & ~IECODE_UP_PREFIX;
		break;

	// Key press
	case IDCMP_RAWKEY:

		// Dragging something?
		if (info->flags & (BDIF_RUBBERBAND | BDIF_DRAGGING))
		{
			// If escape is pressed, cancel drag
			if (msg->Code == 0x45)
				backdrop_stop_drag(info);
			break;
		}

		// Title-barred lister?
		if (info->lister && info->lister->more_flags & LISTERF_TITLEBARRED)
			break;

		// Not handled by default
		handled = 0;

		// Look at key
		switch (msg->Code)
		{
		// Space toggles icon selection
		case 0x40:

			// Lister not busy?
			if (!busy)
			{
				BackdropObject *object;

				// Lock backdrop list
				lock_listlock(&info->objects, 1);

				// Turn off?
				if (info->flags & BDIF_KEY_SELECTION)
				{
					// See if current selection is still valid
					if ((object = find_backdrop_object(info, info->current_sel_object)))
					{
						// Clear selection flag and redraw
						object->flags &= ~BDOF_SELECTED;
						backdrop_render_object(info, object, BRENDERF_CLIP);
					}

					// Clear selection pointer
					info->current_sel_object = 0;
					info->flags &= ~BDIF_KEY_SELECTION;
				}

				// Turn on
				else
				{
					// Sort objects into position order
					backdrop_sort_objects(info, 0, 0);

					// Get first object
					object = (BackdropObject *)info->objects.list.lh_Head;
					if (object->node.ln_Succ)
					{
						// Set selection flag in new object and render
						object->flags |= BDOF_SELECTED;
						backdrop_render_object(info, object, BRENDERF_CLIP);

						// Store object pointer
						info->current_sel_object = object;
						info->flags |= BDIF_KEY_SELECTION;
					}
				}

				// Unlock backdrop list
				unlock_listlock(&info->objects);
				handled = 1;
			}
			break;

		// Cursor keys changes selection
		case CURSORUP:
		case CURSORDOWN:
		case CURSORLEFT:
		case CURSORRIGHT:

			// In selection mode?
			if (!busy && info->flags & BDIF_KEY_SELECTION)
			{
				BackdropObject *object = 0, *lastobject;
				short horiz = 0;

				// Lock backdrop list
				lock_listlock(&info->objects, 1);

				// Horizontal movement?
				if (msg->Code == CURSORLEFT || msg->Code == CURSORRIGHT)
					horiz = BSORT_HORIZ;

				// Need to resort?
				if ((horiz && !(info->flags & BDIF_LAST_SORT_HORIZ)) || (!horiz && info->flags & BDIF_LAST_SORT_HORIZ))
				{
					// Sort objects into position order
					backdrop_sort_objects(info, horiz, 0);
				}

				// See if current selection is still valid
				if ((lastobject = find_backdrop_object(info, info->current_sel_object)))
				{
					// Down or right, get next object
					if (msg->Code == CURSORDOWN || msg->Code == CURSORRIGHT)
						object = (BackdropObject *)lastobject->node.ln_Succ;

					// Up or left, get previous object
					else
						object = (BackdropObject *)lastobject->node.ln_Pred;
				}

				// Invalid object?
				if (!object || !object->node.ln_Succ || !object->node.ln_Pred)
				{
					// Get first (or last) selection
					if (object && !object->node.ln_Pred)
						object = (BackdropObject *)info->objects.list.lh_TailPred;
					else
						object = (BackdropObject *)info->objects.list.lh_Head;
				}

				// New object different from last?
				if (object != info->current_sel_object)
				{
					// Was there an old selection?
					if (lastobject)
					{
						// Clear selection flag and redraw
						lastobject->flags &= ~BDOF_SELECTED;
						backdrop_render_object(info, lastobject, BRENDERF_CLIP);
					}

					// New valid object?
					if (object)
					{
						// Make sure object is visible
						backdrop_make_visible(info, object);

						// Set selection flag in new object and render
						object->flags |= BDOF_SELECTED;
						backdrop_render_object(info, object, BRENDERF_CLIP);
					}

					// Store object pointer
					info->current_sel_object = object;
				}

				// Unlock backdrop list
				unlock_listlock(&info->objects);
				handled = 1;
			}
			break;

		// Return opens an icon
		case 0x44:

			// In selection mode?
			if (!busy && info->flags & BDIF_KEY_SELECTION)
			{
				BackdropObject *object;

				// Lock backdrop list
				lock_listlock(&info->objects, 0);

				// See if current selection is still valid
				if ((object = find_backdrop_object(info, info->current_sel_object)))
				{
					// Open object
					info->last_sel_object = object;
					backdrop_object_open(info, object, msg->Qualifier, 1, 0, 0);

					// Clear selection pointers
					info->current_sel_object = 0;
					info->last_sel_object = 0;
					info->flags &= ~BDIF_KEY_SELECTION;

					// Deselect object
					object->flags &= ~BDOF_SELECTED;
					backdrop_render_object(info, object, BRENDERF_CLIP);
				}

				// Unlock backdrop list
				unlock_listlock(&info->objects);
				handled = 1;
			}
			break;

		// Page up/Page down
		case PAGEUP:
		case PAGEDOWN: {
			IPTR oldpos, pos;
			long newpos, diff;

			// Get old position
			GetAttr(PGA_Top, info->vert_scroller, &oldpos);

			// Get new position
			newpos = info->offset_y - info->area.MinY;
			if (msg->Code == PAGEUP)
				newpos -= (info->size.MaxY - info->size.MinY) + 1;
			else
				newpos += (info->size.MaxY - info->size.MinY) + 1;
			if (newpos < 0)
				newpos = 0;

			// Update gadget
			SetGadgetAttrs(info->vert_scroller, info->window, 0, PGA_Top, newpos, TAG_END);

			// Get new position
			GetAttr(PGA_Top, info->vert_scroller, &pos);

			// Change?
			diff = oldpos - pos;
			if (diff < 0)
				diff = -diff;
			if (diff > 2)
			{
				// Update screen
				backdrop_pos_slider(info, SLIDER_VERT);
			}
			handled = 1;
		}
		break;

		// Home/End
		case HOME:
		case END: {
			IPTR oldpos, pos;
			long diff;

			// Get old position
			GetAttr(PGA_Top, info->horiz_scroller, &oldpos);

			// Update gadget
			SetGadgetAttrs(
				info->horiz_scroller, info->window, 0, PGA_Top, (msg->Code == HOME) ? 0 : 0xfffffff, TAG_END);

			// Get new position
			GetAttr(PGA_Top, info->horiz_scroller, &pos);

			// Change?
			diff = oldpos - pos;
			if (diff < 0)
				diff = -diff;
			if (diff > 2)
			{
				// Update screen
				backdrop_pos_slider(info, SLIDER_HORZ);
			}
			handled = 1;
		}
		break;
		}
		break;

	// Not handled
	default:
		handled = 0;
		break;
	}

	return handled;
}

// Show box for rubber-banding
void backdrop_rubber_band(BackdropInfo *info, BOOL sel)
{
	UBYTE mode;
	struct RastPort *rp;

	// Do selections
	if (sel)
		backdrop_select_area(info, 2);

	// Get rastport
	rp = info->window->RPort;

	// Save draw mode
	mode = rp->DrawMode;

	// Set complement and dash pattern
	SetDrMd(rp, COMPLEMENT);
	SetDrPt(rp, 0xf0f0);

	// Draw rectangle
	Move(rp, info->select.MinX, info->select.MinY);
	Draw(rp, info->select.MaxX, info->select.MinY);
	Draw(rp, info->select.MaxX, info->select.MaxY);
	Draw(rp, info->select.MinX, info->select.MaxY);
	Draw(rp, info->select.MinX, info->select.MinY + ((info->select.MinY < info->select.MaxY) ? 1 : -1));

	// Restore old mode and pattern
	SetDrMd(rp, mode);
	SetDrPt(rp, 0xffff);
}

// Update size for a backdrop
void backdrop_update_size(BackdropInfo *info)
{
	// Check new size
	backdrop_check_size(info);

	// Set flag to say we have a new size
	info->flags |= BDIF_NEW_SIZE;

	// Centered pattern?
	if (info->window->WLayer->BackFill != LAYERS_BACKFILL && GUI->pattern &&
		GUI->pattern[(info->flags & BDIF_MAIN_DESKTOP) ? WBP_ROOT : WBP_DRAWER].flags & PATF_CENTER)
	{
		// Signal full refresh
		IPC_Command(info->ipc, LISTER_BACKFILL_CHANGE, 1, 0, 0, 0);
	}

	// Smart-refresh?
	else if (info->window->WLayer->Flags & LAYERSMART)
	{
		// Refresh objects immediately
		backdrop_show_objects(info, BDSF_REFRESH_DONE | BDSF_RECALC);
		info->flags &= ~BDIF_NEW_SIZE;
	}
}

// Check backdrop size
void backdrop_check_size(BackdropInfo *info)
{
	// Check we have the right clipping size
	if (info->size.MaxX != info->window->Width - info->window->BorderRight - info->right_border - 1 ||
		info->size.MaxY != info->window->Height - info->window->BorderBottom - info->bottom_border - 1)
	{
		// Save new size
		info->size.MinX = info->window->BorderLeft + info->left_border;
		info->size.MinY = info->window->BorderTop + info->top_border;
		info->size.MaxX = info->window->Width - info->window->BorderRight - info->right_border - 1;
		info->size.MaxY = info->window->Height - info->window->BorderBottom - info->bottom_border - 1;

		// Update clip region
		backdrop_install_clip(info);
	}
}
