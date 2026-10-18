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

#define STATUS_BUTTON_WIDTH 20

// Initialise lister display for a new size
void lister_init_display(Lister *lister)
{
	BOOL icons = 0;

	// Viewing icons?
	if (lister->flags & LISTERF_VIEW_ICONS && !(lister->flags & LISTERF_ICON_ACTION))
		icons = 1;

	if (lister_dual_init_display(lister))
		return;

	// Want a title?
	if (environment->env->lister_options & LISTEROPTF_TITLES)
	{
		// Title hidden?
		if (lister->more_flags & LISTERF_HIDE_TITLE || lister->cur_buffer->more_flags & DWF_HIDE_TITLE)
			lister->more_flags &= ~LISTERF_TITLE;

		// No title in icon mode, or when showing a special buffer
		else if (lister->flags & LISTERF_VIEW_ICONS || lister->cur_buffer == lister->special_buffer)
		{
			// Hidden title
			lister->more_flags &= ~LISTERF_TITLE;
			lister->more_flags |= LISTERF_HIDE_TITLE;
		}

		// Otherwise
		else
		{
			// Want a title
			lister->more_flags |= LISTERF_TITLE;
			lister->more_flags &= ~LISTERF_HIDE_TITLE;
		}
	}

	// No title!
	else
		lister->more_flags &= ~(LISTERF_TITLE | LISTERF_HIDE_TITLE);

	// Viewing icons?
	if (icons)
	{
		// No disk name box
		lister->name_area.box.Left = 0;
		lister->name_area.box.Top = 0;
		lister->name_area.box.Width = 0;
		lister->name_area.box.Height = 0;
		display_gui_complete(&lister->name_area, lister->window->RPort);
	}

	// Otherwise
	else
	{
		short status_size = 0, size, a;

		// Get size of SRCE/DEST string
		for (a = MSG_LISTER_STATUS_SOURCE; a < MSG_LISTER_STATUS_OFF; a++)
		{
			char *ptr;

			// Get length of this string, see if it's the longest
			ptr = GetString(&locale, a);
			if (!ptr)
				ptr = "";
			if ((size = TextLength(&lister->window->WScreen->RastPort, ptr, strlen(ptr))) > status_size)
				status_size = size;
		}

		// Add size of exclamation mark
		status_size += TextLength(&lister->window->WScreen->RastPort, "!", 1);

		// Initialise disk name area
		lister->name_area.box.Left = lister->window->BorderLeft + 1;
		lister->name_area.box.Top = lister->window->BorderTop + 1;
		lister->name_area.box.Width = lister->window->GZZWidth - status_size - 10 - STATUS_BUTTON_WIDTH * 2;
		lister->name_area.box.Height = lister->window->WScreen->RastPort.Font->tf_YSize;
		lister->name_area.font = lister->window->WScreen->RastPort.Font;
		display_gui_complete(&lister->name_area, lister->window->RPort);

		// Command area
		lister->command_area.box.Left = lister->name_area.rect.MaxX + 3;
		lister->command_area.box.Top = lister->name_area.rect.MinY;
		lister->command_area.box.Width = STATUS_BUTTON_WIDTH - 2;
		lister->command_area.box.Height = lister->name_area.box.Height;
		lister->command_area.font = 0;
		display_gui_complete(&lister->command_area, lister->window->RPort);

		// Parent area
		lister->parent_area.box.Left = lister->command_area.rect.MaxX + 3;
		lister->parent_area.box.Top = lister->name_area.rect.MinY;
		lister->parent_area.box.Width = STATUS_BUTTON_WIDTH - 2;
		lister->parent_area.box.Height = lister->name_area.box.Height;
		lister->parent_area.font = 0;
		display_gui_complete(&lister->parent_area, lister->window->RPort);

		// Lister status area
		lister->status_area.box.Width = status_size + 6;
		lister->status_area.box.Left =
			lister->window->Width - lister->window->BorderRight - lister->status_area.box.Width - 1;
		lister->status_area.box.Top = lister->window->BorderTop + 1;
		lister->status_area.box.Height = lister->window->WScreen->RastPort.Font->tf_YSize;
		lister->status_area.font = lister->window->WScreen->RastPort.Font;
		display_gui_complete(&lister->status_area, lister->window->RPort);
	}

	// Does the lister have a toolbar?
	if (lister_valid_toolbar(lister))
	{
		// Position toolbar below title bar
		lister->toolbar_area.box.Left = lister->window->BorderLeft;
		lister->toolbar_area.box.Top = lister->name_area.rect.MaxY + 2;
		lister->toolbar_area.box.Width = lister->window->GZZWidth;
		lister->toolbar_area.box.Height = lister->toolbar->height;
		lister->flags |= LISTERF_TOOLBAR;
	}

	// No toolbar, use name area dimensions
	else
	{
		lister->toolbar_area.box = lister->name_area.box;
		++lister->toolbar_area.box.Height;
		lister->flags &= ~LISTERF_TOOLBAR;
	}

	// Initialise toolbar area
	lister->toolbar_area.font = lister->window->WScreen->RastPort.Font;
	display_gui_complete(&lister->toolbar_area, lister->window->RPort);

	// Position title below toolbar
	lister->title_area.box.Left = lister->window->BorderLeft;
	lister->title_area.box.Top = lister->toolbar_area.rect.MaxY + 1;
	lister->title_area.box.Width = lister->window->GZZWidth;
	lister->title_area.box.Height = lister->window->WScreen->RastPort.Font->tf_YSize + 2;

	// Initialise title area
	lister->title_area.font = lister->window->WScreen->RastPort.Font;
	display_gui_complete(&lister->title_area, lister->window->RPort);

	// Path field size
	lister->path_border.MinX = lister->window->BorderLeft;

	lister->path_border.MinY = lister->window->Height - lister->window->BorderBottom - FIELD_FONT->tf_YSize - 5;

	lister->path_border.MaxX = lister->window->Width - lister->window->BorderRight - 1;

	lister->path_border.MaxY = lister->window->Height - lister->window->BorderBottom - 1;

	// Icon action mode?
	if (lister->flags & LISTERF_VIEW_ICONS && lister->flags & LISTERF_ICON_ACTION)
	{
		// Get icon display borders
		lister->backdrop_info->top_border = (lister->toolbar_area.rect.MaxY - lister->name_area.rect.MinY) + 2;
		lister->backdrop_info->bottom_border = FIELD_FONT->tf_YSize + 4;
	}

	// Otherwise, clear borders
	else
	{
		lister->backdrop_info->top_border = 0;
		lister->backdrop_info->bottom_border = 0;
	}

	// Initialise backdrop size
	backdrop_init_info(lister->backdrop_info, lister->window, -1);

	// Not viewing icons?
	if (!icons)
	{
		short a;

		// Initialise file list size
		lister_init_filelist(lister);

		// Calculate scroll borders
		a = lister->text_width >> 1;
		if (a > 6)
			a = 6;
		else if (a < 6)
			a = 2;
		lister->scroll_border.MinX = lister->text_area.rect.MinX + a;
		lister->scroll_border.MaxX = lister->text_area.rect.MaxX - a;
		lister->scroll_border.MinY =
			lister->text_area.rect.MinY + ((lister->text_height / 3) * lister->window->RPort->TxHeight);
		lister->scroll_border.MaxY =
			lister->text_area.rect.MaxY - ((lister->text_height / 3) * lister->window->RPort->TxHeight);

		// Need to add path field?
		if (!(lister->flags & LISTERF_PATH_FIELD))
		{
			// Add field
			lister_add_pathfield(lister);

			// Lister is locked?
			if (lister->flags & LISTERF_LOCK)
				lister_disable_pathfield(lister, TRUE);
		}

		// Just refresh it
		else
			lister_update_pathfield(lister);
	}

	// Otherwise
	else
	{
		// Update virtual size
		backdrop_calc_virtual(lister->backdrop_info);

		// Need to remove path field?
		if (lister->flags & LISTERF_PATH_FIELD)
			lister_remove_pathfield(lister, 1);
	}

	// Calculate window limits
	lister_set_limits(lister);
}

// Refresh lister display
void lister_refresh(Lister *lister, unsigned short mode)
{
	ULONG flags;

	if (lister_dual_one_window(lister))
	{
		lister_dual_refresh(lister, mode);
		return;
	}

	// Refresh?
	if (mode & LREFRESH_REFRESH)
	{
		// Start refresh
		BeginRefresh(lister->window);

		// Have we just resized?
		if (lister->flags & LISTERF_RESIZED)
		{
			// Skip this refresh
			lister->flags &= ~LISTERF_RESIZED;
			EndRefresh(lister->window, TRUE);
			return;
		}
	}

	// Want toolbar, etc?
	if (!(lister->flags & LISTERF_VIEW_ICONS) || lister->flags & LISTERF_ICON_ACTION)
	{
		// Draw borders
		display_gui_border(&lister->name_area);
		display_gui_border(&lister->command_area);
		display_gui_border(&lister->parent_area);
		display_gui_border(&lister->status_area);

		// Initialise and redraw toolbar area
		if (lister->flags & LISTERF_TOOLBAR)
			lister_show_toolbar(lister);

		// Icon mode?
		if (lister->flags & LISTERF_VIEW_ICONS)
		{
			// Full refresh?
			if (mode & LREFRESH_FULL_ICON)
			{
				// Clear icon display
				backdrop_show_objects(lister->backdrop_info, BDSF_CLEAR | BDSF_CLEAR_ONLY);
			}
		}

		// Clear file list area
		else if (mode & LREFRESH_FULL)
			lister_init_lister_area(lister);

		// Refresh path field
		if (lister->flags & LISTERF_PATH_FIELD && mode & LREFRESH_FULL)
			RefreshGList(lister->path_field, lister->window, 0, 1);

		// Check key selector
		if (lister->flags & LISTERF_KEY_SELECTION && (mode & LREFRESH_FULL))
			set_list_selector(lister, lister->selector_pos);
	}

	// Viewing icons?
	if (lister->flags & LISTERF_VIEW_ICONS)
	{
		// Full refresh with layout change (e.g. gauge added/removed)?
		// Clear old icon positions before recalculating to avoid ghost images
		if (mode & LREFRESH_FULL_ICON)
			backdrop_show_objects(lister->backdrop_info, BDSF_CLEAR | BDSF_CLEAR_ONLY);

		// Full refresh?
		if (mode & LREFRESH_FULL)
			backdrop_show_objects(lister->backdrop_info, BDSF_RECALC);

		// Partial
		else if (!(mode & LREFRESH_NOICON))
			backdrop_show_objects(lister->backdrop_info,
								  BDSF_CLEAR | ((mode & LREFRESH_REFRESH) ? BDSF_IN_REFRESH : BDSF_REFRESH_DONE));
	}

	// Refresh sliders and status bar, and file list if in text mode
	flags = REFRESHF_STATUS | REFRESHF_NO_ICONS;
	if (mode & LREFRESH_FULL)
		flags |= REFRESHF_SLIDERS;
	if (mode & LREFRESH_REFRESH)
		flags |= REFRESHF_REFRESH;
	lister_refresh_display(lister, flags);

	// Refresh?
	if (mode & (LREFRESH_REFRESH | LREFRESH_SIMPLEREFRESH))
	{
		// Title, with a button selected?
		if (lister->more_flags & LISTERF_TITLE && lister->title_click != -1)
		{
			// If button is selected, highlight it
			if (lister->title_sel)
				lister_title_highlight(lister, lister->title_click, lister->title_sel);
		}
	}

	// End refresh
	if (mode & LREFRESH_REFRESH)
	{
		EndRefresh(lister->window, TRUE);
	}
}

// Initialise file list area size
void lister_init_filelist(Lister *lister)
{
	short width;
	struct Rectangle *rect;

	if (lister_dual_init_filelist(lister))
		return;

	// Rectangle at top
	if (lister->more_flags & LISTERF_TITLE)
		rect = &lister->title_area.rect;
	else
		rect = &lister->toolbar_area.rect;

	// Initialise display area
	lister->list_area.box.Left = lister->window->BorderLeft;
	lister->list_area.box.Top = rect->MaxY + 1;
	lister->list_area.box.Width = lister->window->GZZWidth;
	lister->list_area.box.Height =
		lister->window->Height - lister->window->BorderBottom - lister->list_area.box.Top - FIELD_FONT->tf_YSize - 4;
	lister->list_area.font = 0;
	display_gui_complete(&lister->list_area, lister->window->RPort);

	// Store file list area for backdrop
	lister->backdrop_info->size = lister->list_area.rect;

	// Get width of available area
	width = lister->list_area.box.Width - 2;
	if (lister->flags & LISTERF_KEY_SELECTION)
		width -= KEY_SEL_OFFSET;

	// Calculate text dimensions
	lister->text_width = (lister->more_flags & LISTERF_PROP_FONT)
							 ? width
							 : ((width / lister->window->RPort->TxWidth) * lister->window->RPort->TxWidth);
	lister->text_height = (lister->list_area.box.Height - 2) / lister->window->RPort->TxHeight;

	// Initialise lister text area
	lister->text_area.box.Width = lister->text_width;
	lister->text_area.box.Height = lister->text_height * lister->window->RPort->TxHeight;
	lister->text_area.box.Left = lister->list_area.box.Left + 1;
	if (lister->flags & LISTERF_KEY_SELECTION)
		lister->text_area.box.Left += KEY_SEL_OFFSET;
	lister->text_area.box.Top = lister->list_area.box.Top + 1;
	lister->text_area.font = 0;
	display_gui_complete(&lister->text_area, lister->window->RPort);
}

// Initialise file list area
void lister_init_lister_area(Lister *lister)
{
	// In icon mode?
	if (lister->flags & LISTERF_VIEW_ICONS)
		return;

	// Clear region
	ClearRegion(lister->title_region);

	// Install clip rectangle for list box
	OrRectRegion(lister->title_region, &lister->list_area.rect);

	// Remove text area box
	XorRectRegion(lister->title_region, &lister->text_area.rect);

	// Install region
	InstallClipRegion(lister->window->WLayer, lister->title_region);

	// Erase area
	EraseRect(lister->window->RPort,
			  lister->list_area.rect.MinX,
			  lister->list_area.rect.MinY,
			  lister->list_area.rect.MaxX,
			  lister->list_area.rect.MaxY);

	// Remove region
	InstallClipRegion(lister->window->WLayer, 0);
}

// Add the path field to a lister
void lister_add_pathfield(Lister *lister)
{
	short pos = -1;

	// Don't add if shrunk down
	if (lister->more_flags & LISTERF_TITLEBARRED)
		return;

	// Got gauge gadget?
	if (lister->gauge_gadget)
	{
		struct Gadget *gadget;

		// Find gauge gadget
		for (gadget = lister->window->FirstGadget, pos = 0; gadget; gadget = gadget->NextGadget, pos++)
		{
			if (gadget->GadgetID == GAD_GAUGE)
				break;
		}

		// Not found?
		if (!gadget)
			pos = -1;
	}

	// Add gadgets
	AddGList(lister->window, &lister->parent_button, pos, 2, 0);
	RefreshGList(&lister->parent_button, lister->window, 0, 2);
	lister->flags |= LISTERF_PATH_FIELD;

	// Update the path field
	lister_update_pathfield(lister);
}

// Update path field
void lister_update_pathfield(Lister *lister)
{
	if (lister_dual_update_pathfields(lister))
		return;

	// Is window open with path field?
	if (lister->window && lister->flags & LISTERF_PATH_FIELD)
	{
		// Update path field
		SetGadgetAttrs(
			lister->path_field,
			lister->window,
			0,
			STRINGA_TextVal,
			(IPTR)lister->cur_buffer->buf_ExpandedPath,
			TAG_END);
	}

	// Is lister iconified?
	else if (lister->appicon)
	{
		// Build new icon label
		lister_build_icon_name(lister);

		// Update app icon
		SendNotifyMsg(DN_APP_ICON_LIST, (IPTR)lister->appicon, DNF_ICON_CHANGED, FALSE, 0, 0);
	}
}

// Remove path field
void lister_remove_pathfield(Lister *lister, BOOL clear)
{
	// Remove gadget
	RemoveGList(lister->window, &lister->parent_button, 2);
	lister->flags &= ~LISTERF_PATH_FIELD;

	// Erase border area
	if (clear)
	{
		EraseRect(lister->window->RPort,
				  lister->path_border.MinX,
				  lister->path_border.MinY,
				  lister->path_border.MaxX,
				  lister->path_border.MaxY);
	}
}

// Dis/enable path field
void lister_disable_pathfield(Lister *lister, short disable)
{
	if (lister_dual_disable_pathfields(lister, disable))
		return;

	// Disable it
	if (lister->flags & LISTERF_PATH_FIELD)
		SetGadgetAttrs(lister->path_field, lister->window, 0, GA_Disabled, disable, TAG_END);
}

// Changed mode from icon/text
void lister_mode_change(Lister *lister, struct IBox *dims)
{
	BOOL need_refresh = 1;

	// If we change size, we'll refresh automatically
	if (dims->Width != lister->window->Width || dims->Height != lister->window->Height)
		need_refresh = 0;

	// Just gone to icon mode?
	if (lister->flags & LISTERF_VIEW_ICONS)
	{
		// Need to remove path field?
		if (lister->flags & LISTERF_PATH_FIELD && !(lister->flags & LISTERF_ICON_ACTION))
			lister_remove_pathfield(lister, 0);

		// Enable backdrop pattern
		lister->pattern.disabled = FALSE;
	}

	// Gone to text mode
	else
	{
		// Disable backdrop pattern for text mode
		lister->pattern.disabled = TRUE;

		// Hidden title?
		if (lister->more_flags & LISTERF_HIDE_TITLE && !(lister->cur_buffer->more_flags & DWF_HIDE_TITLE))
		{
			// Fix flag
			lister->more_flags |= LISTERF_TITLE;
			lister->more_flags &= ~LISTERF_HIDE_TITLE;
		}
	}

	// Clear window area
	erase_window(lister->window);

	// Set to infinite limits
	WindowLimits(lister->window, 1, 1, (UWORD)~0, (UWORD)~0);

	// Resize window
	ChangeWindowBox(lister->window, lister->window->LeftEdge, lister->window->TopEdge, dims->Width, dims->Height);

	// Need to refresh?
	if (need_refresh)
	{
		// Initialise display
		lister_init_display(lister);

		// Refresh display
		lister_refresh(lister, LREFRESH_FULL);
	}

	// Otherwise, set flag to indicate mode change
	else
	{
		lister->more_flags |= LISTERF_MODE_CHANGED;
	}
}

// Check if a refresh is needed
void lister_check_refresh(Lister *lister)
{
	// See if refresh bit is set
	if (lister->window->WLayer->Flags & LAYERREFRESH)
	{
		// Do refresh
		lister_refresh(lister, LREFRESH_REFRESH);
	}
}

// Refresh callback
REF_CALLBACK_BEGIN(void, lister_refresh_callback, d0, ULONG, type, a0, struct Window *, window, a1, Lister *, lister)
{
	// Do refresh
	if (lister)
	{
		// Change window/resize
		if (type == IDCMP_CHANGEWINDOW || type == IDCMP_NEWSIZE)
		{
			// Is window not titlebarred?
			if (!(lister->flags & LISTERF_TITLEBARRED))
			{
				// Reposition only
				if (type == IDCMP_CHANGEWINDOW)
				{
					// If size has not changed, store new position
					if (!CheckWindowDims(lister->window, &lister->dimensions))
					{
						StoreWindowDims(lister->window, &lister->dimensions);
					}
				}

				// Has size changed?
				else if (CheckWindowDims(lister->window, &lister->dimensions))
				{
					// Store new size
					StoreWindowDims(lister->window, &lister->dimensions);

					// Set flag for resize
					lister->flags |= LISTERF_RESIZED;

					// Turn on size verify
					ModifyIDCMP(lister->window, lister->window->IDCMPFlags | IDCMP_SIZEVERIFY);

					// Path field?
					if (lister->flags & LISTERF_PATH_FIELD)
					{
						struct Rectangle rect;

						// Get old path area
						rect = lister->path_border;

						// Erase to end of newly uncovered area
						rect.MaxX = lister->window->Width - lister->window->BorderRight - 1;
						rect.MaxY = lister->window->Height - lister->window->BorderBottom - FIELD_FONT->tf_YSize - 6;

						// Need to erase path field?
						if (rect.MinX <= rect.MaxX && rect.MinY <= rect.MaxY)
						{
							// Erase path area
							EraseRect(lister->window->RPort, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);

							// Add to refresh region
							OrRectRegion(lister->refresh_extra, &rect);
						}
					}

					// Title bar?
					if (!(lister->flags & LISTERF_VIEW_ICONS) || (lister->flags & LISTERF_ICON_ACTION))
					{
						struct Rectangle rect;

						// Build rectangle for title bar area
						rect.MinX = lister->window->BorderLeft;
						rect.MinY = lister->window->BorderTop;
						rect.MaxX =
							lister->window->Width - lister->window->BorderRight - 1;  // lister->title_area.rect.MaxX;
						rect.MaxY = lister->title_area.rect.MaxY;

						// Add title bar area to refresh region
						OrRectRegion(lister->refresh_extra, &rect);
					}

					// Initialise lister for new size
					lister_init_display(lister);

					// Refresh sliders
					lister_refresh_display(lister, REFRESHF_SLIDERS);

					// In text mode?
					if (!(lister->flags & LISTERF_VIEW_ICONS))
					{
						// Erase text area border
						lister_init_lister_area(lister);
					}

					// Mode changed to icon mode?
					else if (lister->more_flags & LISTERF_MODE_CHANGED)
					{
						// Update selection count
						backdrop_fix_count(lister->backdrop_info, 1);

						// Get icons
						IPC_Command(lister->ipc, LISTER_GET_ICONS, 0, 0, 0, 0);
					}
				}

				// In icon mode?
				if (lister->flags & LISTERF_VIEW_ICONS)
				{
					struct IntuiMessage msg;

					// Fake IntuiMessage
					msg.Class = type;

					// Pass message on to icons
					backdrop_idcmp(lister->backdrop_info, &msg, 0);
				}
			}
		}

		// Refresh window
		else if (type == IDCMP_REFRESHWINDOW)
		{
			// Mode change, needs a full reset
			if (lister->flags & LISTERF_RESIZED && lister->more_flags & LISTERF_MODE_CHANGED)
			{
				// Do refresh stuff
				BeginRefresh(lister->window);
				EndRefresh(lister->window, TRUE);
				lister_dual_finish_frame_refresh(lister);

				// Do full refresh
				lister_refresh(lister, LREFRESH_FULL);

				// Clear flags
				lister->flags &= ~LISTERF_RESIZED;
				lister->more_flags &= ~LISTERF_MODE_CHANGED;
			}

			// Otherwise
			else
			{
				// Lock layers
#ifdef LOCKLAYER_OK
				LockScreenLayer(lister->window->WScreen);
#else
				Forbid();
#endif

				// Resized?
				if (lister->flags & LISTERF_RESIZED)
				{
					// Or in extra refresh areas
					OrRegionRegion(lister->refresh_extra, lister->window->WLayer->DamageList);

					// Clear extra region
					ClearRegion(lister->refresh_extra);
				}

				// Start refresh
				BeginRefresh(lister->window);

				// Do lister refresh
				lister_refresh(lister, LREFRESH_NOICON | LREFRESH_SIMPLEREFRESH);

				// Icons?
				if (lister->flags & LISTERF_VIEW_ICONS)
				{
					struct IntuiMessage msg;

					// Fake IntuiMessage
					msg.Class = IDCMP_REFRESHWINDOW;

					// End refresh temporarily
					EndRefresh(lister->window, FALSE);

					// Redraw icons
					backdrop_idcmp(lister->backdrop_info, &msg, BIDCMPF_LISTER);

					// Start refresh again
					BeginRefresh(lister->window);
				}

				// End refresh for good
				EndRefresh(lister->window, TRUE);
				lister_dual_finish_frame_refresh(lister);

				// Unlock layers
#ifdef LOCKLAYER_OK
				UnlockScreenLayer(lister->window->WScreen);
#else
				Permit();
#endif

				// Resized?
				if (lister->flags & LISTERF_RESIZED)
				{
					// Clear resize flag
					lister->flags &= ~LISTERF_RESIZED;

					// Turn off size verify
					ModifyIDCMP(lister->window, lister->window->IDCMPFlags & ~IDCMP_SIZEVERIFY);
				}
			}
		}
	}
}
