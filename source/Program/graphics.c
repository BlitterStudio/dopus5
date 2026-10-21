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

// Draw a 3D box
void do3dbox(struct RastPort *rp, struct Rectangle *rect, BOOL recessed)
{
	DrawBox(rp, rect, GUI->draw_info, recessed);
}

#define SMALL_ARROW_WIDTH 4
#define SMALL_ARROW_HEIGHT 7
#define BIG_ARROW_WIDTH 6
#define BIG_ARROW_HEIGHT 11

// Show list selector
void show_list_selector(Lister *lister, short line, BOOL erase)
{
	short left, top, width, height, image_width, image_height, fpen = 1, bpen = 0;
	const unsigned short *image;
	DirEntry *entry;

	// Get width and height
	width = KEY_SEL_OFFSET;
	height = lister->text_area.rast.TxHeight;

	// Get position
	left = lister->text_area.rect.MinX - KEY_SEL_OFFSET;
	top = lister->text_area.rect.MinY + line * height;

	// Small font?
	if (height < BIG_ARROW_HEIGHT)
	{
#ifdef __amigaos3__
		image = small_arrow_chip;
#else
		image = small_arrow;
#endif
		image_width = SMALL_ARROW_WIDTH;
		image_height = SMALL_ARROW_HEIGHT;
	}

	// Big font
	else
	{
#ifdef __amigaos3__
		image = big_arrow_chip;
#else
		image = big_arrow;
#endif
		image_width = BIG_ARROW_WIDTH;
		image_height = BIG_ARROW_HEIGHT;
	}

	// Center in area
	left += (width - image_width) >> 1;
	top += (height - image_height) >> 1;

	// Is lister in damage refresh mode?
	if (lister->window->Flags & WFLG_WINDOWREFRESH)
	{
		struct Rectangle rect;

		// Forbid for this
#ifdef LOCKLAYER_OK
		LockLayer(0, lister->window->WLayer);
#else
		Forbid();
#endif

		// End refresh temporarily
		EndRefresh(lister->window, FALSE);

		// Get rectangle for arrow
		rect.MinX = left;
		rect.MinY = top;
		rect.MaxX = left + image_width - 1;
		rect.MaxY = top + image_height - 1;

		// Add to damage list
		OrRectRegion(lister->window->WLayer->DamageList, &rect);

		// Continue with refresh
		BeginRefresh(lister->window);

		// Enable now we've done our dirty work
#ifdef LOCKLAYER_OK
		UnlockLayer(lister->window->WLayer);
#else
		Permit();
#endif
	}

	// Get selector line entry, and set correct colours
	if ((entry = get_entry_ord(&lister->cur_buffer->entry_list, line + lister->cur_buffer->buf_VertOffset)))
		getdispcol(entry, lister, line, &fpen, &bpen);

	// Erase?
	if (erase || lister->cur_buffer->buf_TotalEntries[0] < 1)
	{
		// Get correct background colour
		SetAPen(&lister->list_area.rast, (entry) ? bpen : 0);

		// Erase image
		RectFill(&lister->list_area.rast, left, top, left + image_width - 1, top + image_height - 1);
		return;
	}

	// Draw arrow-like thing
	SetAPen(&lister->list_area.rast,
			(entry && bpen == lister->drawinfo->dri_Pens[TEXTPEN]) ? fpen : lister->drawinfo->dri_Pens[TEXTPEN]);
	SetDrMd(&lister->list_area.rast, JAM1);
	BltTemplate((unsigned char *)image, 0, 2, &lister->list_area.rast, left, top, image_width, image_height);
}

// Erase a window interior
void erase_window(struct Window *window)
{
	EraseRect(window->RPort,
			  window->BorderLeft,
			  window->BorderTop,
			  window->Width - window->BorderRight - 1,
			  window->Height - window->BorderBottom - 1);
}

// Create an iconify gadget for a window
struct Gadget *create_iconify_gadget(struct Screen *screen, struct List *list, BOOL cover_zoom)
{
	return CreateTitleGadget(screen, list, cover_zoom, TBGADGETOFFSET, IM_ICONIFY, GAD_ICONIFY);
}

// Fill out one gadget to cover another
void fill_out_cover_gadget(struct Gadget *gadget, struct Gadget *cover)
{
	// Copy data from sizing gadget
	*cover = *gadget;

	// Fix flags, etc
	cover->NextGadget = 0;
	cover->Flags &= GFLG_RELBOTTOM | GFLG_RELRIGHT;
	cover->Activation = 0;
	cover->GadgetType = GTYP_BOOLGADGET;
	cover->GadgetRender = 0;
	cover->SelectRender = 0;
}

// Lock screen layerinfo
void LockScreenLayer(struct Screen *screen)
{
	// Not already locked?
	if (!(GUI->flags & GUIF_LAYER_LOCKED))
	{
		// Lock layerinfo
		LockLayerInfo(&screen->LayerInfo);
		GUI->flags |= GUIF_LAYER_LOCKED;
	}
}

// Unlock layerinfo
void UnlockScreenLayer(struct Screen *screen)
{
	// Locked?
	if (GUI->flags & GUIF_LAYER_LOCKED)
	{
		// Unlock layerinfo
		UnlockLayerInfo(&screen->LayerInfo);
		GUI->flags &= ~GUIF_LAYER_LOCKED;
	}
}
