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
#include "misc.h"

#define PEXTRA 8

// Display a directory list
// Called from LISTER PROCESS
void lister_display_dir(Lister *lister)
{
	short delta = 0, top_line, bottom_line;
	short count;
	short y_pos = 0;
	short draw_width, draw_x_pos, draw_offset;
	short line;
	DirBuffer *buffer;
	DirEntry *entry;
	short scroll_x_amount = 0, scroll_y_amount;
	BOOL refresh = 0;

	// If invalid window, return
	if (!lister || !lister_valid_window(lister))
		return;

	// Icon mode or no refresh?
	if (lister->flags & (LISTERF_VIEW_ICONS | LISTERF_NO_REFRESH))
		return;

	// Get current buffer
	buffer = lister->cur_buffer;

	// Lock current buffer
	buffer_lock(buffer, FALSE);

	// Redraw the whole display?
	if (buffer->buf_OldVertOffset == -1)
	{
		top_line = 0;
		bottom_line = lister->text_height;

		// Update title
		if (lister->more_flags & LISTERF_TITLE)
			lister_show_title(lister, 1);
	}

	// Otherwise, do we need to scroll?
	else
	{
		// Calculate scroll distance
		if (!(delta = buffer->buf_VertOffset - buffer->buf_OldVertOffset))
		{
			buffer_unlock(buffer);
			return;
		}

		// Scroll up?
		if (delta < 0)
		{
			top_line = lister->text_height + delta;
			bottom_line = lister->text_height;
		}

		// Scroll down
		else
		{
			top_line = 0;
			bottom_line = delta;
		}

		// Scroll more than full page or not at all?
		if (ABS(delta) >= lister->text_height)
		{
			top_line = 0;
			bottom_line = lister->text_height;
			delta = 0;
		}
	}

	// Initialise horizontal drawing width and position
	draw_width = lister->text_width;
	draw_x_pos = lister->text_area.rect.MinX;
	draw_offset = 0;

	// Find entry at the top of the window; quicker to start from the bottom?
	if (buffer->buf_VertOffset > (buffer->buf_TotalEntries[0] >> 1))
	{
		// Go from bottom
		for (entry = (DirEntry *)buffer->entry_list.mlh_TailPred, count = buffer->buf_TotalEntries[0] - 1;
			 entry->de_Node.dn_Pred && count > buffer->buf_VertOffset;
			 entry = (DirEntry *)entry->de_Node.dn_Pred, count--)
			;
	}
	else
	{
		// Go from top
		for (entry = (DirEntry *)buffer->entry_list.mlh_Head, count = 0;
			 entry->de_Node.dn_Succ && count < buffer->buf_VertOffset;
			 entry = (DirEntry *)entry->de_Node.dn_Succ, count++)
			;
	}

	// Scroll horizontally?
	if (buffer->buf_OldHorizOffset != -1)
	{
		short hdelta;

		// Get horizontal delta
		hdelta = buffer->buf_HorizOffset - buffer->buf_OldHorizOffset;

		// See if we actually need to scroll
		if (hdelta != 0 && ABS(hdelta) < lister->text_width)
		{
			// Get amount to scroll
			scroll_x_amount = hdelta;

			// Need to render on the right side of the window?
			if (hdelta > 0)
			{
				draw_x_pos = lister->text_area.rect.MaxX + 1 - scroll_x_amount;
				draw_offset = lister->text_width - hdelta;
			}

			// Store horizontal draw width
			draw_width = ABS(hdelta);

			// Update title
			if (lister->more_flags & LISTERF_TITLE)
				lister_show_title(lister, 1);
		}
	}

	// Get amount to scroll vertically
	scroll_y_amount = delta * lister->text_area.font->tf_YSize;

	// Any scrolling to do?
	if (scroll_x_amount || scroll_y_amount)
	{
		GUI_Element *element;

		// Use full lister display if scrolling vertically
		element = (!scroll_x_amount) ? &lister->list_area : &lister->text_area;

		// Is the window smart refresh?
		if (!(lister->window->Flags & WFLG_SIMPLE_REFRESH))
		{
			// Scroll display
			ClipBlit(&lister->text_area.rast,
					 element->rect.MinX + ((scroll_x_amount > 0) ? scroll_x_amount : 0),
					 lister->text_area.rect.MinY + ((scroll_y_amount > 0) ? scroll_y_amount : 0),
					 &lister->text_area.rast,
					 element->rect.MinX - ((scroll_x_amount < 0) ? scroll_x_amount : 0),
					 lister->text_area.rect.MinY - ((scroll_y_amount < 0) ? scroll_y_amount : 0),
					 element->box.Width - (ABS(scroll_x_amount)),
					 lister->text_area.box.Height - (ABS(scroll_y_amount)),
					 0xc0);
		}

		// Simple
		else
		{
			// Set correct pen for background fill
			SetBPen(&lister->text_area.rast, lister_colour_pen(lister, ENVCOL_FILES_UNSEL, 1));

			// Scroll display
			ScrollRaster(&lister->text_area.rast,
						 scroll_x_amount,
						 scroll_y_amount,
						 element->rect.MinX,
						 lister->text_area.rect.MinY,
						 element->rect.MaxX,
						 lister->text_area.rect.MaxY);

			// Any damage?
			if (lister->text_area.rast.Layer->Flags & LAYERREFRESH)
			{
				struct Rectangle rect;

				// Get rectangle to show new areas
				rect = element->rect;
				if (scroll_x_amount < 0)
					rect.MaxX = lister->text_area.rect.MinX - (scroll_x_amount + 1);
				else if (scroll_x_amount > 0)
					rect.MinX = lister->text_area.rect.MaxX - (scroll_x_amount - 1);
				else if (scroll_y_amount < 0)
					rect.MaxY = lister->text_area.rect.MinY - (scroll_y_amount + 1);
				else if (scroll_y_amount > 0)
					rect.MinY = lister->text_area.rect.MaxY - (scroll_y_amount - 1);

					// Modify damage list
#ifdef LOCKLAYER_OK
				LockLayer(0, lister->window->WLayer);
#else
				Forbid();
#endif
				OrRectRegion(lister->window->WLayer->DamageList, &rect);
#ifdef LOCKLAYER_OK
				UnlockLayer(lister->window->WLayer);
#else
				Permit();
#endif

				// Do system refresh
				BeginRefresh(lister->window);
				refresh = 1;

				// Redraw the whole display
				top_line = 0;
				bottom_line = lister->text_height;
				delta = 0;
				draw_width = lister->text_width;
				draw_x_pos = lister->text_area.rect.MinX;
				draw_offset = 0;
			}
		}
	}

	// If we're scrolling down, move to the first entry we actually have to draw
	if (delta > 0)
	{
		for (count = 0; count < lister->text_height - delta && entry->de_Node.dn_Succ;
			 count++, entry = (DirEntry *)entry->de_Node.dn_Succ)
			;

		// Start printing at the bottom
		y_pos = lister->text_area.box.Height - (lister->text_area.font->tf_YSize * delta);
	}

	// Add window top offset on
	y_pos += lister->text_area.rect.MinY + lister->text_area.font->tf_Baseline;

	// Non-proportional font?
	if (!(lister->more_flags & LISTERF_PROP_FONT))
	{
		// Convert x coordinates to characters
		draw_offset = (draw_offset + buffer->buf_HorizOffset) / lister->text_area.rast.TxWidth;
		draw_width = draw_width / lister->text_area.rast.TxWidth;
	}

	// Go through the lines we need to render
	for (line = top_line; line < bottom_line; line++)
	{
		// Display entry
		lister_draw_entry(lister, entry, draw_x_pos, y_pos, draw_offset, draw_width, line);

		// Is entry valid?
		if (entry->de_Node.dn_Succ)
		{
			// Get next entry
			entry = (DirEntry *)entry->de_Node.dn_Succ;
		}

		// Increment y position
		y_pos += lister->text_area.font->tf_YSize;

		// Update keyboard selector
		if (lister->flags & LISTERF_KEY_SELECTION && lister->selector_pos == line)
			show_list_selector(lister, line, 0);
	}

	// Finish system refresh
	if (refresh)
	{
		EndRefresh(lister->window, TRUE);
	}

	// Remember old offsets
	buffer->buf_OldVertOffset = buffer->buf_VertOffset;
	buffer->buf_OldHorizOffset = buffer->buf_HorizOffset;

	// Unlock buffer
	buffer_unlock(buffer);
}

// Display an entry at a given position
// Called from LISTER PROCESS
void display_entry(DirEntry *entry, Lister *lister, int position)
{
	short width = lister->text_width, offset = 0;

	// If iconified or in icon view, return
	if (!lister_valid_window(lister) || (lister->flags & LISTERF_VIEW_ICONS))
		return;

	// Non-proportional font?
	if (!(lister->more_flags & LISTERF_PROP_FONT))
	{
		// Convert x coordinates to characters
		offset = lister->cur_buffer->buf_HorizOffset / lister->text_area.rast.TxWidth;
		width = width / lister->text_area.rast.TxWidth;
	}

	// Display entry
	lister_draw_entry(lister,
					  entry,
					  lister->text_area.rect.MinX,
					  lister->text_area.rect.MinY + lister->text_area.font->tf_Baseline +
						  (lister->text_area.font->tf_YSize * position),
					  offset,
					  width,
					  position);

	// Update keyboard selector
	if (lister->flags & LISTERF_KEY_SELECTION && lister->selector_pos == position)
		show_list_selector(lister, position, 0);
}

// Draw the text for an entry
void lister_draw_entry(Lister *lister, DirEntry *entry, short x, short y, short offset, short width, short line)
{
	// Set pen colours
	setdispcol(entry, lister);

	// Proportional font?
	if (lister->more_flags & LISTERF_PROP_FONT)
	{
		// Build string to display
		if (builddisplaystring_prop(entry, lister->display_buf, lister, line))
		{
			// Blit the display and free bitmap
			ClipBlit(&lister->render_rast,
					 lister->cur_buffer->buf_HorizOffset + offset + PEXTRA,
					 0,
					 &lister->text_area.rast,
					 x,
					 y - lister->text_area.rast.TxBaseline,
					 width,
					 lister->text_area.rast.TxHeight,
					 0xc0);
		}
	}

	// Non-proportional
	else
	{
		// Build string to display
		builddisplaystring(entry, lister->display_buf, lister);

		// Display entry
		Move(&lister->text_area.rast, x, y);
		Text(&lister->text_area.rast, lister->display_buf + offset, width);
	}

	// Erase edges of lister
	SetAPen(&lister->text_area.rast, lister->text_area.rast.BgPen);
	if (lister->text_area.rect.MinX > lister->list_area.rect.MinX)
		RectFill(&lister->text_area.rast,
				 lister->list_area.rect.MinX,
				 y - lister->text_area.rast.TxBaseline,
				 lister->text_area.rect.MinX - 1,
				 y + lister->text_area.rast.TxHeight - lister->text_area.rast.TxBaseline - 1);
	if (lister->text_area.rect.MaxX < lister->list_area.rect.MaxX)
		RectFill(&lister->text_area.rast,
				 lister->text_area.rect.MaxX + 1,
				 y - lister->text_area.rast.TxBaseline,
				 lister->list_area.rect.MaxX,
				 y + lister->text_area.rast.TxHeight - lister->text_area.rast.TxBaseline - 1);

	// Valid entry?
	if (entry->de_Node.dn_Succ)
	{
		// Entry highlight?
		if (entry->de_Flags & ENTF_HIGHLIGHT)
			entry_highlight(lister, y);

		// Editing line?
		if (line == lister->cursor_line)
		{
			short w;

			// Get position of cursor
			x = lister->text_area.rect.MinX + lister->cursor_pos - lister->cur_buffer->buf_HorizOffset;
			y -= lister->text_area.font->tf_Baseline;
			if (lister->edit_ptr && *(lister->edit_ptr + lister->edit_pos))
				w = TextLength(&lister->text_area.rast, lister->edit_ptr + lister->edit_pos, 1);
			else
				w = lister->text_area.rast.TxWidth;

			// Within display?
			if (x + lister->text_area.rast.TxWidth - 1 <= lister->text_area.rect.MaxX)
			{
				// Show cursor
#ifdef __AROS__
				struct BitMap *bm;
				struct RastPort rp;

				// Blitting overlapping regions doesn't always work, so we use a dummy RastPort
				if ((bm = AllocBitMap(w, lister->text_area.rast.TxHeight, 1, 0, lister->text_area.rast.BitMap)))
				{
					InitRastPort(&rp);
					rp.BitMap = bm;
					ClipBlit(&rp, 0, 0, &lister->text_area.rast, x, y, w, lister->text_area.rast.TxHeight, 0x50);
					FreeBitMap(bm);
				}
#else
				ClipBlit(&lister->text_area.rast,
						 x,
						 y,
						 &lister->text_area.rast,
						 x,
						 y,
						 w,
						 lister->text_area.rast.TxHeight,
						 0x50);
#endif
			}
		}
	}
}

// Change pens to the colours needed to display an entry
// Called from the LISTER PROCESS
void setdispcol(DirEntry *entry, Lister *lister)
{
	short fpen, bpen;

	// Get pen colours
	getdispcol(entry, lister, &fpen, &bpen);

	/*
		// Highlighted entry?
		if (lister->last_highlight==entry)
		{
			short temp;

			// Swap the pens
			temp=fpen;
			fpen=bpen;
			bpen=temp;
		}
	*/

	// Under 39, set pens using the fast call
	if (((struct Library *)GfxBase)->lib_Version >= 39)
	{
		SetABPenDrMd(&lister->text_area.rast, fpen, bpen, JAM2);
	}

	// Under 37
	else
	{
		// Have pens changed?
		if (lister->text_area.rast.FgPen != fpen)
			SetAPen(&lister->text_area.rast, fpen);
		if (lister->text_area.rast.BgPen != bpen)
			SetBPen(&lister->text_area.rast, bpen);
	}

	// Bold set?
	if (lister->text_area.rast.AlgoStyle & FSF_BOLD)
	{
		// Don't need it any more?
		if (!(entry->de_Flags & ENTF_LINK))
			SetSoftStyle(&lister->text_area.rast, 0, FSF_BOLD);
	}

	// Need it?
	else if (entry->de_Flags & ENTF_LINK)
		SetSoftStyle(&lister->text_area.rast, FSF_BOLD, FSF_BOLD);
}

void getdispcol(DirEntry *entry, Lister *lister, short *fpen, short *bpen)
{
	short col = ENVCOL_FILES_UNSEL;

	// Valid entry?
	if (entry->de_Node.dn_Succ)
	{
		// Look at entry type
		switch (ENTRYTYPE(entry->de_Node.dn_Type))
		{
		// File
		case ENTRY_FILE:

			// Is file selected?
			if (entry->de_Flags & ENTF_SELECTED)
				col = ENVCOL_FILES_SEL;

			// Device colour?
			else if (entry->de_Flags & ENTF_COLOUR_DEVICE)
				col = ENVCOL_DEVICES;

			// Not selected
			else
				col = ENVCOL_FILES_UNSEL;
			break;

		// Device
		case ENTRY_DEVICE:

			// Selected?
			if (entry->de_Flags & ENTF_SELECTED)
				col = ENVCOL_FILES_SEL;

			// Is entry a device or a volume (ie not an assign)
			else if (entry->de_Size == DLT_DEVICE)
				col = ENVCOL_DEVICES;

			// Assign
			else
				col = ENVCOL_VOLUMES;
			break;

		// Directory
		case ENTRY_DIRECTORY:

			// Selected?
			if (entry->de_Flags & ENTF_SELECTED)
				col = ENVCOL_DIRS_SEL;

			// Assign colour?
			else if (entry->de_Flags & ENTF_COLOUR_ASSIGN)
				col = ENVCOL_VOLUMES;

			// Not selected
			else
				col = ENVCOL_DIRS_UNSEL;
			break;

		// Unknown type
		default:
			*fpen = 1;
			*bpen = 0;
			return;
		}
	}

	// Get pens, custom or configured
	*fpen = lister_colour_pen(lister, col, 0);
	*bpen = lister_colour_pen(lister, col, 1);
}

// Build the string to display an entry
// Called from the LISTER process
void builddisplaystring(DirEntry *entry, char *display_buf, Lister *lister)
{
	NetworkInfo *network = 0;
	char size_buf[27];
	register char *buf_ptr, *src_ptr;
	char *end_ptr;
	register short len;
	register short item;
	DirBuffer *buffer = lister->cur_buffer;

	// If entry is invalid, fill string with spaces
	if (!entry->de_Node.dn_Succ)
	{
		CopyMem(str_space_string, display_buf, MAXDISPLAYLENGTH);
		return;
	}

	// Does entry have a custom display string?
	if ((buf_ptr = (char *)GetTagData(DE_DisplayString, 0, entry->de_Tags)))
	{
		char *ptr;
		short pos = 0;

		// Fill with spaces
		CopyMem(str_space_string, display_buf, MAXDISPLAYLENGTH);

		// Copy display string
		for (ptr = buf_ptr; *ptr; ptr++)
		{
			// Tab?
			if (*ptr == '\t')
			{
				// Column position?
				if (*(ptr + 1) == '\b')
				{
					short col;

					// Get column
					col = atoi(ptr + 2);
					for (ptr += 2; *ptr && *ptr != '\b'; ptr++)
						;

					// Advance to column position
					col /= lister->text_area.rast.TxWidth;
					pos = col;
				}

				// Normal tab
				else
				{
					// Advance to next tab
					pos = ((pos + LISTER_TABSIZE) / LISTER_TABSIZE) * LISTER_TABSIZE;
				}
			}

			// Normal character
			else
				display_buf[pos++] = *ptr;
		}

		return;
	}

	// See if file has network information
	if (entry->de_Flags & ENTF_NETWORK)
		network = (NetworkInfo *)GetTagData(DE_NetworkInfo, 0, entry->de_Tags);

	// If entry has a known size
	if (entry->de_Node.dn_Type <= ENTRY_FILE || (entry->de_Node.dn_Type >= ENTRY_DIRECTORY && entry->de_Size > 0))
	{
		// Zero-byte file?
		if (entry->de_Size == 0)
			strcpy(size_buf, string_empty);

		// Build size string
		else
		{
#ifdef USE_64BIT
			ItoaU64(&entry->de_Size,
					size_buf,
					sizeof(size_buf),
					(environment->env->settings.date_flags & DATE_1000SEP)
						? ((entry->de_Flags & ENTF_FAKE) ? ',' : GUI->decimal_sep)
						: 0);
#else
			ItoaU(entry->de_Size,
				  size_buf,
				  (environment->env->settings.date_flags & DATE_1000SEP)
					  ? ((entry->de_Flags & ENTF_FAKE) ? ',' : GUI->decimal_sep)
					  : 0);
#endif
		}
	}

	// Otherwise, clear size string
	else
		size_buf[0] = 0;

	// Get pointer to start and end of display buffer
	buf_ptr = display_buf;
	end_ptr = display_buf + MAXDISPLAYLENGTH - 1;

	// Go through display items
	for (item = 0; item < DISPLAY_LAST; item++)
	{
		short display_item;

		display_item = buffer->buf_ListFormat.display_pos[item];
		if (display_item < 0 || display_item >= DISPLAY_LAST)
			break;

		// Look at item type
		switch (display_item)
		{
		// Name
		case DISPLAY_NAME:

			// Get length and pointer to name
			len = buffer->buf_FieldWidth[DISPLAY_NAME] / lister->text_area.rast.TxWidth;
			if ((src_ptr = entry->de_Node.dn_Name))
			{
				// Copy name into buffer
				while (*src_ptr && len-- > 0)
					*buf_ptr++ = *src_ptr++;
			}

			// Pad name with spaces
			while (len-- > 0)
				*buf_ptr++ = ' ';
			break;

		// Size
		case DISPLAY_SIZE:

			// Get pointer to size buffer
			if (buffer->size_length > 0)
			{
				short size_len = strlen(size_buf);
				len = buffer->buf_FieldWidth[DISPLAY_SIZE] / lister->text_area.rast.TxWidth;
				src_ptr = size_buf;

				// Pad with spaces
				while (len > size_len + 1)
				{
					*buf_ptr++ = ' ';
					--len;
				}

				// Copy size into buffer
				while (*src_ptr && len-- > 0)
					*buf_ptr++ = *src_ptr++;

				// Add an extra space
				*buf_ptr++ = ' ';
			}
			break;

		// Protection
		case DISPLAY_PROTECT:

			// Get pointer to protection buffer
			src_ptr = entry->de_ProtBuf;
			len = buffer->buf_FieldWidth[DISPLAY_PROTECT] / lister->text_area.rast.TxWidth;

			// Copy protection into buffer. Add a space on the end
			while (*src_ptr && len-- > 1)
				*buf_ptr++ = *src_ptr++;
			while (len-- > 0)
				*buf_ptr++ = ' ';
			break;

		// Date
		case DISPLAY_DATE:

			// Get length and pointer to date buffer
			len = buffer->buf_FieldWidth[DISPLAY_DATE] / lister->text_area.rast.TxWidth;
			src_ptr = entry->de_DateBuf;

			// Copy date into buffer
			while (*src_ptr && len-- > 1)
				*buf_ptr++ = *src_ptr++;

			// Pad date with spaces (including extra space)
			while (len-- > 0)
				*buf_ptr++ = ' ';
			break;

		// Comment
		case DISPLAY_COMMENT:

			// Get length of comment to display
			if (buffer->comment_length > 0)
			{
				len = buffer->buf_FieldWidth[DISPLAY_COMMENT] / lister->text_area.rast.TxWidth;

				// Check valid comment pointer
				if ((src_ptr = (char *)GetTagData(DE_Comment, 0, entry->de_Tags)))
				{
					// Copy comment into buffer
					while (*src_ptr && len-- > 1)
						*buf_ptr++ = *src_ptr++;
				}

				// Pad with spaces
				while (len-- > 0)
					*buf_ptr++ = ' ';
			}
			break;

		// Version
		case DISPLAY_VERSION:

			// Get length of version to display
			if (buffer->version_length > 0)
			{
				VersionInfo *info;

				// Get length
				len = buffer->buf_FieldWidth[DISPLAY_VERSION] / lister->text_area.rast.TxWidth;

				// Fake entry?
				if (entry->de_Flags & ENTF_FAKE)
				{
					// Fake string
					*buf_ptr++ = '1';
					--len;
				}

				// Check we have version info
				else if (entry->de_Flags & ENTF_VERSION &&
						 (info = (VersionInfo *)GetTagData(DE_VersionInfo, 0, entry->de_Tags)))
				{
					char version_buf[40];
					short version_len;

					// Build string
					build_version_string(version_buf, info->vi_Version, info->vi_Revision, info->vi_Days, -1);

					// Get length
					version_len = strlen((src_ptr = version_buf));

					// Pad with spaces on left
					while (len > version_len + 1)
					{
						*buf_ptr++ = ' ';
						--len;
					}

					// Copy version into buffer
					while (*src_ptr && len-- > 1)
						*buf_ptr++ = *src_ptr++;
				}

				// Pad with spaces
				while (len-- > 0)
					*buf_ptr++ = ' ';
			}
			break;

		// Filetype
		case DISPLAY_FILETYPE:

			// Get length of filetype to display
			if (buffer->type_length > 0)
			{
				len = buffer->buf_FieldWidth[DISPLAY_FILETYPE] / lister->text_area.rast.TxWidth;

				// Check valid filetype pointer
				if ((src_ptr = (char *)GetTagData(DE_Filetype, 0, entry->de_Tags)))
				{
					// Copy filetype into buffer
					while (*src_ptr && len-- > 1)
						*buf_ptr++ = *src_ptr++;
				}

				// Pad with spaces
				while (len-- > 0)
					*buf_ptr++ = ' ';
			}
			break;

		// Owner
		case DISPLAY_OWNER:

			// Get length of owner string to display
			len = buffer->buf_FieldWidth[DISPLAY_OWNER] / lister->text_area.rast.TxWidth;

			// Get owner
			if (network && network->owner[0])
				src_ptr = network->owner;
			else
				src_ptr = string_no_owner;

			// Copy into buffer
			while (*src_ptr && len-- > 0)
				*buf_ptr++ = *src_ptr++;

			// Pad with spaces
			while (len-- > 0)
				*buf_ptr++ = ' ';
			break;

		// Group
		case DISPLAY_GROUP:

			// Get length of group string to display
			len = buffer->buf_FieldWidth[DISPLAY_GROUP] / lister->text_area.rast.TxWidth;

			// Get group
			if (network && network->group[0])
				src_ptr = network->group;
			else
				src_ptr = string_no_group;

			// Copy into buffer
			while (*src_ptr && len-- > 0)
				*buf_ptr++ = *src_ptr++;

			// Pad with spaces
			while (len-- > 0)
				*buf_ptr++ = ' ';
			break;

		// Network protection bits
		case DISPLAY_NETPROT:

			// Length to display
			len = buffer->buf_FieldWidth[DISPLAY_NETPROT] / lister->text_area.rast.TxWidth;

			// Get protection string
			if (network)
				src_ptr = network->net_protbuf;
			else
				src_ptr = "--------";

			// Copy into buffer
			while (*src_ptr && len-- > 0)
				*buf_ptr++ = *src_ptr++;

			// Pad with spaces
			while (len-- > 0)
				*buf_ptr++ = ' ';
			break;
		}
	}

	// Pad rest of display buffer with spaces
	while (buf_ptr < end_ptr)
		*buf_ptr++ = ' ';
}

// Highlight an entry
void entry_highlight(Lister *lister, short y)
{
	short x, x1, y1, sides = 0;
	struct RastPort *rp = &lister->text_area.rast;

	// Switch to COMPLEMENT
	SetDrMd(rp, COMPLEMENT);

	// Set line pattern
	SetDrPt(rp, 0xaaaa);

	// Get coordinates
	x = lister->text_area.rect.MinX;
	y -= lister->text_area.font->tf_Baseline;
	x1 = lister->text_area.rect.MaxX;
	y1 = y + lister->text_area.font->tf_YSize - 1;

	// Do sides?
	if (lister->cur_buffer->buf_HorizOffset == 0)
		sides |= 1;
	if (lister->cur_buffer->buf_HorizLength + lister->cur_buffer->buf_HorizOffset <= lister->text_width)
		sides |= 2;

	// Draw highlight box
	Move(rp, x, y);
	Draw(rp, x1, y);
	if (sides & 2)
		Draw(rp, x1, y1);
	else
		Move(rp, x1, y1);
	Draw(rp, x, y1);
	if (sides & 1)
		Draw(rp, x, y);

	// Restore rastport
	SetDrMd(rp, JAM2);
	SetDrPt(rp, 0xffff);
}

// Read disk name and size and update name display
// Called from the LISTER PROCESS
void lister_update_name(Lister *lister)
{
	DirBuffer *buffer;

	// If iconified, return
	if (!lister_valid_window(lister))
		return;

	// Get buffer
	buffer = lister->cur_buffer;

	// Lock buffer
	buffer_lock(buffer, FALSE);

	// If window is not a device list
	if (IsListEmpty((struct List *)&buffer->entry_list) ||
		((DirEntry *)buffer->entry_list.mlh_Head)->de_Node.dn_Type != ENTRY_DEVICE)
	{
		D_S(struct InfoData, info)

		// If no path, return immediately
		if (!buffer->buf_Path[0])
		{
			lister_show_name(lister);
			buffer_unlock(buffer);
			return;
		}

		// Get disk info
		GetDiskInfo(buffer->buf_Path, info);

		// If the filesystem reports id_NumBlocksUsed > id_NumBlocks
		// (seen on amiberry directory-mounts with inconsistent host
		// stats), clamp used to total so the title bar doesn't display
		// a wrap-around tens-of-terabytes "free" figure.
		{
			ULONG num_blocks = (ULONG)info->id_NumBlocks;
			ULONG used_blocks = (ULONG)info->id_NumBlocksUsed;
			ULONG bytes_per_block = (ULONG)info->id_BytesPerBlock;
			if (used_blocks > num_blocks)
				used_blocks = num_blocks;

#ifdef USE_64BIT
			buffer->buf_TotalDiskSpace = num_blocks;
			buffer->buf_TotalDiskSpace64 = (UQUAD)bytes_per_block * (UQUAD)num_blocks;
			buffer->buf_FreeDiskSpace = num_blocks - used_blocks;
			buffer->buf_FreeDiskSpace64 =
				buffer->buf_TotalDiskSpace64 - ((UQUAD)bytes_per_block * (UQUAD)used_blocks);
#else
			buffer->buf_TotalDiskSpace = UMult32(bytes_per_block, num_blocks);
			buffer->buf_FreeDiskSpace =
				buffer->buf_TotalDiskSpace - UMult32(bytes_per_block, used_blocks);
#endif
		}

		// Is this the RAM disk?
		if (buffer->buf_FreeDiskSpace == 0 && strncmp(buffer->buf_Path, "RAM:", 4) == 0)
		{
			// Free space equals available memory
			buffer->buf_FreeDiskSpace = AvailMem(MEMF_ANY);
			buffer->buf_TotalDiskSpace += buffer->buf_FreeDiskSpace;
#ifdef USE_64BIT
	#warning this is probably not necessary anymore
			buffer->buf_FreeDiskSpace64 = (UQUAD)buffer->buf_FreeDiskSpace;
			buffer->buf_TotalDiskSpace64 += buffer->buf_FreeDiskSpace64;
#endif
		}

		// If disk is write protected, set READONLY flag in directory
		if (info->id_DiskState == ID_WRITE_PROTECTED)
			buffer->flags |= DWF_READONLY;
		else
			buffer->flags &= ~DWF_READONLY;
	}

	// Unlock buffer
	buffer_unlock(buffer);

	// Display name
	lister_show_name(lister);
}

// Refresh the display of a window
// Called from LISTER PROCESS
void lister_refresh_display(Lister *lister, ULONG flags)
{
	DirBuffer *buffer;

	// Colours changed; get the custom pens again
	if (flags & REFRESHF_COLOUR && lister->window)
		lister_init_display_colours(lister);

	if (lister_dual_one_window(lister))
	{
		lister_dual_refresh_display(lister, flags);
		return;
	}

	// Get current buffer
	buffer = lister->cur_buffer;

	// Resort?
	if (flags & REFRESHF_RESORT)
	{
		lister_resort(lister, 0);
		if (flags & REFRESHF_SORTSEL)
			buffer_sort_selected(lister->cur_buffer);
	}

	// Iconified?
	if (!lister_valid_window(lister))
		return;

	// Field titles enabled? (can't do this on a idcmp_refresh)
	if (environment->env->lister_options & LISTEROPTF_TITLES && !(flags & REFRESHF_REFRESH))
	{
		// User-changed state of field titles?
		if ((lister->cur_buffer->more_flags & DWF_HIDE_TITLE && lister->more_flags & LISTERF_TITLE) ||
			!(lister->cur_buffer->more_flags & DWF_HIDE_TITLE || lister->more_flags & LISTERF_TITLE))
		{
			// Hide title if needed
			if (lister->cur_buffer->more_flags & DWF_HIDE_TITLE)
			{
				lister->more_flags &= ~LISTERF_TITLE;
				flags |= REFRESHF_FULL;
			}

			// Show title
			else if (!(lister->more_flags & LISTERF_HIDE_TITLE))
			{
				lister->more_flags |= LISTERF_TITLE;
				flags |= REFRESHF_FULL;
			}
		}
	}

	// Full refresh?
	if (flags & REFRESHF_FULL)
	{
		// Initialise lister display
		lister_init_display(lister);

		// Erase area
		lister_init_lister_area(lister);
	}

	// Is key selection on?
	if (lister->flags & LISTERF_KEY_SELECTION && !(flags & REFRESHF_REFRESH))
	{
		// Is selector past the bottom of the list?
		if (buffer->buf_VertOffset + lister->selector_pos >= buffer->buf_TotalEntries[0])
		{
			// Set the new selector position
			set_list_selector(lister, 0xffff);
		}
	}

	// Colour?
	if (flags & REFRESHF_COLOUR)
	{
		// Got gauge?
		if (lister->gauge_gadget)
		{
			// Get new existing pens
			lister_init_colour(lister, ENVCOL_GAUGE, FALSE);

			// Update gauge gadget
			SetGadgetAttrs(lister->gauge_gadget,
						   lister->window,
						   0,
						   DGG_FillPen,
						   lister->lst_Colours[ENVCOL_GAUGE].cr_Pen[0],
						   DGG_FillPenAlert,
						   lister->lst_Colours[ENVCOL_GAUGE].cr_Pen[1],
						   TAG_END);
		}
	}

	// Sliders
	if (flags & REFRESHF_SLIDERS)
	{
		lister_fix_horiz_len(lister);
		lister_update_slider(lister, SLIDER_VERT | SLIDER_HORZ);
	}
	else if (lister->more_flags & LISTERF_NEED_RECALC)
		lister_fix_horiz_len(lister);

	// Not refreshing the sliders
	if (!(flags & REFRESHF_SLIDERS))
	{
		// Check vertical offset
		if (buffer->buf_VertOffset + lister->text_height > buffer->buf_TotalEntries[0])
			buffer->buf_VertOffset = buffer->buf_TotalEntries[0] - lister->text_height;
		if (buffer->buf_VertOffset < 0)
			buffer->buf_VertOffset = 0;

		// Check horizontal offset
		if (buffer->buf_HorizOffset > buffer->buf_HorizLength - lister->text_width)
			buffer->buf_HorizOffset = buffer->buf_HorizLength - lister->text_width;
		if (buffer->buf_HorizOffset < 0)
			buffer->buf_HorizOffset = 0;
	}

	// Disk name
	if (flags & REFRESHF_UPDATE_NAME)
		lister_refresh_name(lister);
	else if (flags & REFRESHF_NAME)
		lister_show_name(lister);

	// Path field?
	if (flags & REFRESHF_PATH)
		lister_update_pathfield(lister);

	// Status
	if (flags & REFRESHF_STATUS)
		lister_show_status(lister);

	// Icon mode?
	if (lister->flags & LISTERF_VIEW_ICONS)
	{
		// Allowed to do icons?
		if (!(flags & REFRESHF_NO_ICONS))
		{
			// Clear icon display?
			if (flags & REFRESHF_CLEAR_ICONS)
				backdrop_show_objects(lister->backdrop_info, BDSF_CLEAR_ONLY);

			// Refresh icons?
			else if (flags & REFRESHF_ICONS)
				backdrop_show_objects(lister->backdrop_info, BDSF_RECALC);
		}
	}

	// Refresh files
	else
	{
		buffer->buf_OldVertOffset = -1;
		buffer->buf_OldHorizOffset = -1;
		lister_display_dir(lister);
	}
}

// Update a disk name and size
// Called from the LISTER PROCESS
void lister_refresh_name(Lister *lister)
{
	// Is volume available?
	if (VolumePresent(lister->cur_buffer))
	{
		// Update size information
		lister_update_name(lister);
		return;
	}

	// Otherwise, just redraw stored name
	lister_show_name(lister);
}

// Build the string to display an entry (supports proportional fonts)
struct BitMap *builddisplaystring_prop(DirEntry *entry, char *display_buf, Lister *lister, short line)
{
	NetworkInfo *network = 0;
	VersionInfo *info;
	char size_buf[20], version_buf[40], *src_ptr;
	register short pos;
	register short item;
	DirBuffer *buffer = lister->cur_buffer;
	struct BitMap *bm;
	struct RastPort *rp;
	short width;

	// Get bitmap width
	width = (lister->cur_buffer->buf_HorizLength > lister->window->Width) ? lister->cur_buffer->buf_HorizLength
																		  : lister->window->Width;

	// No existing bitmap, or too small?
	if (!lister->render_bitmap || lister->render_bitmap_width < width)
	{
		// Free existing bitmap
		WaitBlit();
		DisposeBitMap(lister->render_bitmap);

		// Create bitmap
		if (!(lister->render_bitmap = NewBitMap(width + 16,
												lister->window->RPort->TxHeight + 2,
												lister->window->RPort->BitMap->Depth,
												0,
												lister->window->RPort->BitMap)))
			return 0;

		// Save width
		lister->render_bitmap_width = width;

		// Initialise rastport
		CopyMem((char *)&lister->text_area.rast, (char *)&lister->render_rast, sizeof(struct RastPort));
		lister->render_rast.BitMap = lister->render_bitmap;
		lister->render_rast.Layer = 0;
		SetDrMd(&lister->render_rast, JAM2);
	}

	// Get pointer to bitmap and rastport
	bm = lister->render_bitmap;
	rp = &lister->render_rast;

	// Erase rectangle to background colour
	SetAPen(rp, lister->text_area.rast.BgPen);
	RectFill(rp, PEXTRA, 0, PEXTRA + width - 1, rp->TxHeight - 1);

	// Copy colour settings
	if (((struct Library *)GfxBase)->lib_Version >= 39)
	{
		SetABPenDrMd(rp, lister->text_area.rast.FgPen, lister->text_area.rast.BgPen, JAM2);
	}
	else
	{
		SetAPen(rp, lister->text_area.rast.FgPen);
		SetBPen(rp, lister->text_area.rast.BgPen);
	}
	SetSoftStyle(rp, lister->text_area.rast.AlgoStyle & FSF_BOLD, FSF_BOLD);

	// If entry is invalid, return empty bitmap
	if (!entry->de_Node.dn_Succ)
		return bm;

	// Does entry have a custom display string?
	if ((src_ptr = (char *)GetTagData(DE_DisplayString, 0, entry->de_Tags)))
	{
		// Draw display string
		Move(rp, PEXTRA, rp->TxBaseline);
		TabbedTextOut(rp, src_ptr, strlen(src_ptr), LISTER_TABSIZE);
		return bm;
	}

	// See if file has network information
	if (entry->de_Flags & ENTF_NETWORK)
		network = (NetworkInfo *)GetTagData(DE_NetworkInfo, 0, entry->de_Tags);

	// If entry has a known size
	if (entry->de_Node.dn_Type <= ENTRY_FILE || (entry->de_Node.dn_Type >= ENTRY_DIRECTORY && entry->de_Size > 0))
	{
		// Zero-byte file?
		if (entry->de_Size == 0)
			strcpy(size_buf, string_empty);

		// Build size string
		else
		{
#ifdef USE_64BIT
			ItoaU64(&entry->de_Size,
					size_buf,
					sizeof(size_buf),
					(environment->env->settings.date_flags & DATE_1000SEP) ? GUI->decimal_sep : 0);
#else
			ItoaU(entry->de_Size,
				  size_buf,
				  (environment->env->settings.date_flags & DATE_1000SEP) ? GUI->decimal_sep : 0);
#endif
		}
	}

	// Otherwise, clear size string
	else
		size_buf[0] = 0;

	// Get pointer to start position
	pos = PEXTRA;

	// Go through display items
	for (item = 0; item < DISPLAY_LAST; item++)
	{
		short right = 0, split = 0, num;

		// Clear pointer
		src_ptr = 0;

		// Cache item type
		num = buffer->buf_ListFormat.display_pos[item];
		if (num < 0 || num >= DISPLAY_LAST)
			break;

		// Look at item type
		switch (num)
		{
		// Name
		case DISPLAY_NAME:
			src_ptr = entry->de_Node.dn_Name;
			break;

		// Size
		case DISPLAY_SIZE:
			src_ptr = size_buf;
			right = 1;
			break;

		// Protection
		case DISPLAY_PROTECT:
			src_ptr = entry->de_ProtBuf;
			right = 1;
			break;

		// Date
		case DISPLAY_DATE:
			src_ptr = entry->de_DateBuf;
			split = 1;
			break;

		// Comment
		case DISPLAY_COMMENT:
			src_ptr = (char *)GetTagData(DE_Comment, 0, entry->de_Tags);
			break;

		// Version
		case DISPLAY_VERSION:
			if (entry->de_Flags & ENTF_VERSION && (info = (VersionInfo *)GetTagData(DE_VersionInfo, 0, entry->de_Tags)))
			{
				// Build string
				build_version_string(version_buf, info->vi_Version, info->vi_Revision, info->vi_Days, -1);
				src_ptr = version_buf;
				right = 1;
			}
			break;

		// Filetype
		case DISPLAY_FILETYPE:
			src_ptr = (char *)GetTagData(DE_Filetype, 0, entry->de_Tags);
			break;

		// Owner
		case DISPLAY_OWNER:
			src_ptr = (network && network->owner[0]) ? network->owner : string_no_owner;
			break;

		// Group
		case DISPLAY_GROUP:
			src_ptr = (network && network->group[0]) ? network->group : string_no_group;
			break;

		// Network protection bits
		case DISPLAY_NETPROT:
			src_ptr = (network) ? network->net_protbuf : "--------";
			right = 1;
			break;
		}

		// Display string
		if (src_ptr && *src_ptr)
		{
			short tlen, used = 0;
			struct TextExtent ex;

			// Editing line?
			if (line == lister->cursor_line)
			{
				// Is this item being edited?
				if ((buffer->buf_ListFormat.display_pos[item]) == lister->edit_type)
				{
					// No right justification or splitting
					split = 0;
					right = 0;
				}
			}

			// Split into columns
			if (split)
			{
				short len, tlen;

				// Get length of first column
				for (len = 0; src_ptr[len]; len++)
					if (src_ptr[len] == ' ')
						break;

				// Get length we can display
				tlen = len;
				if (buffer->buf_CustomWidthFlags & (1 << num))
				{
					tlen = TextFit(rp, src_ptr, tlen, &ex, 0, 1, buffer->buf_FieldWidth[num], rp->TxHeight);
					used = ex.te_Width;
				}

				// Draw left column
				Move(rp, pos, rp->TxBaseline);
				Text(rp, src_ptr, tlen);

				// Bump pointer
				src_ptr += len;
				while (*src_ptr == ' ')
					++src_ptr;

				// Right-justify second column
				right = 1;
			}

			// Get string length
			tlen = strlen(src_ptr);
			if (buffer->buf_CustomWidthFlags & (1 << num))
			{
				if (right)
					used += rp->TxWidth;
				tlen = TextFit(rp, src_ptr, tlen, &ex, 0, 1, buffer->buf_FieldWidth[num] - used, rp->TxHeight);
			}

			// Text to display?
			if (tlen > 0)
			{
				// Right justified?
				if (right)
				{
					short x =
						pos + lister->cur_buffer->buf_FieldWidth[num] - rp->TxWidth - TextLength(rp, src_ptr, tlen);

					// Right justify position
					Move(rp, (x < 0) ? 0 : x, rp->TxBaseline);
				}

				// Left justified
				else
				{
					// Set position
					Move(rp, pos, rp->TxBaseline);
				}

				// Draw text
				Text(rp, src_ptr, tlen);
			}
		}

		// Increment position
		if ((pos += lister->cur_buffer->buf_FieldWidth[num]) >= width + PEXTRA)
			break;
	}

	return bm;
}
