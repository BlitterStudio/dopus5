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

#if defined(__MORPHOS__)
	/* MOS-only alpha blit / tint API lives in cybergraphics.library; P96 has no equivalent. */
	#include <proto/cybergraphics.h>
	#include <cybergraphx/cybergraphics.h>
	/*
	 * This is from Ambient source code.
	 */
	#define OWN_MAGIC 0x466f4164

struct OwnDiskObject
{
	struct DiskObject diskobj;
	struct FreeList *fl; /* this is safe for WB 3.1 */
	/* start of additions */
	ULONG ownmagic;
	APTR ownptr;
	APTR png_context;
	ULONG ttnum;
	ULONG ttcur;
	STRPTR path;
	APTR glowchunk;
	ULONG glowsize;
	struct BitMap *pngimage;
	ULONG pngimage_width;
	ULONG pngimage_height;
	struct BitMap *pngimage2;
};

	#define ISOWN(x) (((struct OwnDiskObject *)x)->ownmagic == OWN_MAGIC && ((struct OwnDiskObject *)x)->ownptr == x)
#endif

// Can this mask be used to draw into this bitmap?
static BOOL backdrop_mask_blittable(struct BitMap *bm, APTR mask)
{
	return (BOOL)(display_rtg_bitmap(bm) || (TypeOfMem(mask) & MEMF_CHIP));
}

// Memory type for icon masks on the current screen
static ULONG backdrop_mask_memory(void)
{
#ifdef USE_DRAWICONSTATE
	// Icons are drawn by DrawIconState(); masks are only used for hit-testing
	return MEMF_ANY;
#else
	// No screen yet; stay with chip, backdrop_init_info() checks again
	if (!GUI->screen_pointer)
		return MEMF_CHIP;
	return display_rtg_bitmap(GUI->screen_pointer->RastPort.BitMap) ? MEMF_ANY : MEMF_CHIP;
#endif
}

#ifndef USE_DRAWICONSTATE
// Draw an icon image through its mask. Returns FALSE if the mask could not
// be used, so the caller draws the image without it.
static BOOL backdrop_blit_icon_mask(struct BitMap *bitmap,
									struct RastPort *rp,
									short left,
									short top,
									struct Image *image,
									UWORD *mask)
{
	UWORD *chip_mask = 0;
	long size;

	// Mask is in fast memory (RTG screen) but the blitter draws into this
	// native bitmap (eg a BOB drag image); give it a chip copy
	if (!backdrop_mask_blittable(rp->BitMap, mask))
	{
		size = RASSIZE(image->Width, image->Height);
		if (!(chip_mask = AllocVec(size, MEMF_CHIP)))
			return FALSE;
		CopyMem((char *)mask, (char *)chip_mask, size);
		mask = chip_mask;
	}

	// Draw using the mask
	BltMaskBitMapRastPort(bitmap, 0, 0, rp, left, top, image->Width, image->Height, 0xe0, (PLANEPTR)mask);

	// Free temporary copy once the blitter is done with it
	if (chip_mask)
	{
		WaitBlit();
		FreeVec(chip_mask);
	}
	return TRUE;
}
#endif

// Scroll the icons
void backdrop_scroll_objects(BackdropInfo *info, short off_x, short off_y)
{
	// Lock window
	GetSemaphore(&info->window_lock, SEMF_EXCLUSIVE, 0);

	// Window open?
	if (info->window)
	{
		short damage = 0, clear = 0;

		// Bounds-check the deltas
		if (off_x < 0 && off_x < -RECTWIDTH(&info->size))
			clear = 1;
		else if (off_x > 0 && off_x > RECTWIDTH(&info->size))
			clear = 1;
		else if (off_y < 0 && off_y < -RECTHEIGHT(&info->size))
			clear = 1;
		else if (off_y > 0 && off_y > RECTHEIGHT(&info->size))
			clear = 1;

		// Clear instead of scrolling?
		if (clear)
		{
			// Clear the whole window
			EraseRect(info->window->RPort, info->size.MinX, info->size.MinY, info->size.MaxX, info->size.MaxY);
		}

		// Scroll
		else
		{
			// Check for 39
			if (((struct Library *)GfxBase)->lib_Version >= 39)
			{
				// Scroll backdrop window
				ScrollRasterBF(info->window->RPort,
							   off_x,
							   off_y,
							   info->size.MinX,
							   info->size.MinY,
							   info->size.MaxX,
							   info->size.MaxY);
			}

			// No backfills
			else
			{
				// Scroll backdrop window
				ScrollRaster(info->window->RPort,
							 off_x,
							 off_y,
							 info->size.MinX,
							 info->size.MinY,
							 info->size.MaxX,
							 info->size.MaxY);
			}

			// Damaged simple-refresh?
			if (info->window->Flags & WFLG_SIMPLE_REFRESH && info->window->WLayer->Flags & LAYERREFRESH)
			{
				// Forbid
#ifdef LOCKLAYER_OK
				LockScreenLayer(info->window->WScreen);
#else
				Forbid();
#endif

				// Begin refreshing
				BeginRefresh(info->window);

				// Clear the new bits
				EraseRect(info->window->RPort, info->size.MinX, info->size.MinY, info->size.MaxX, info->size.MaxY);

				// End refreshing for the moment
				EndRefresh(info->window, FALSE);
				damage = 1;
			}
		}

		// Got temporary region?
		if (info->temp_region)
		{
			struct Rectangle rect;

			// Get refresh region
			rect.MinX = (off_x == 0) ? info->size.MinX : ((off_x > 0) ? info->size.MaxX - off_x : info->size.MinX);
			rect.MaxX = (off_x == 0) ? info->size.MaxX : ((off_x > 0) ? info->size.MaxX : info->size.MinX - off_x);
			rect.MinY = (off_y == 0) ? info->size.MinY : ((off_y > 0) ? info->size.MaxY - off_y : info->size.MinY);
			rect.MaxY = (off_y == 0) ? info->size.MaxY : ((off_y > 0) ? info->size.MaxY : info->size.MinY - off_y);

			// Bounds check region
			if (rect.MinX < info->size.MinX)
				rect.MinX = info->size.MinX;
			if (rect.MinY < info->size.MinY)
				rect.MinY = info->size.MinY;
			if (rect.MaxX > info->size.MaxX)
				rect.MaxX = info->size.MaxX;
			if (rect.MaxY > info->size.MaxY)
				rect.MaxY = info->size.MaxY;

			// Add to damage list?
			if (damage)
			{
				// Or rectangle in
				OrRectRegion(info->window->WLayer->DamageList, &rect);
			}

			// Manually refresh
			else
			{
				// Set refresh region
				ClearRegion(info->temp_region);
				OrRectRegion(info->temp_region, &rect);

				// Install region
				InstallClipRegion(info->window->WLayer, info->temp_region);
			}
		}

		// Manual refresh?
		if (!damage)
		{
			// Refresh
			backdrop_show_objects(info, BDSF_NO_CLIP);

			// Remove clip region
			InstallClipRegion(info->window->WLayer, 0);
		}

		// Automatic refresh
		else
		{
			// Lister?
			if (info->lister)
				lister_refresh_callback(IDCMP_REFRESHWINDOW, info->window, info->lister);

			// Other type
			else
			{
				struct IntuiMessage msg;

				// Fake IntuiMessage
				msg.Class = IDCMP_REFRESHWINDOW;

				// Handle refresh
				backdrop_idcmp(info, &msg, 0);
			}

			// Enable multi-tasking
#ifdef LOCKLAYER_OK
			UnlockScreenLayer(info->window->WScreen);
#else
			Permit();
#endif
		}
	}

	// Unlock window
	FreeSemaphore(&info->window_lock);
}

// Show the backdrop objects
void backdrop_show_objects(BackdropInfo *info, UWORD flags)
{
	BackdropObject *object;

	// Lock backdrop list
	lock_listlock(&info->objects, 0);

	// Lock window
	GetSemaphore(&info->window_lock, SEMF_EXCLUSIVE, 0);

	// Window open?
	if (info->window)
	{
		// Are we in a refresh?
		if (flags & BDSF_IN_REFRESH)
		{
			// Lock layers
#ifdef LOCKLAYER_OK
			LockScreenLayer(info->window->WScreen);
#else
			Forbid();
#endif

			// End refresh temporarily
			EndRefresh(info->window, FALSE);

			// Install new clip region if we have it
			if (info->clip_region)
				InstallClipRegion(info->window->WLayer, info->clip_region);

			// Continue refresh
			BeginRefresh(info->window);
		}

		// Or, are we meant to be refreshing?
		else if (flags & BDSF_REFRESH)
		{
			// Start refresh here?
			if ((flags & BDSF_REFRESH_DONE) == BDSF_REFRESH_DONE)
			{
				// Lock layers
#ifdef LOCKLAYER_OK
				LockScreenLayer(info->window->WScreen);
#else
				Forbid();
#endif
			}

			// And our region with damagelist
			if (info->clip_region)
				AndRegionRegion(info->clip_region, info->window->WLayer->DamageList);

			// Begin the refresh
			BeginRefresh(info->window);
		}

		// Install clip region if we have it
		else if (!(flags & BDSF_NO_CLIP) && info->clip_region)
			InstallClipRegion(info->window->WLayer, info->clip_region);

		// Clear backdrop window
		if (flags & BDSF_CLEAR)
		{
			EraseRect(&info->rp, info->size.MinX, info->size.MinY, info->size.MaxX, info->size.MaxY);
		}

		// Not just clearing?
		if ((flags & BDSF_CLEAR_ONLY) != BDSF_CLEAR_ONLY)
		{
			// Go through backdrop list (backwards)
			for (object = (BackdropObject *)info->objects.list.lh_TailPred; object->node.ln_Pred;
				 object = (BackdropObject *)object->node.ln_Pred)
			{
				// Reset?
				if (flags & BDSF_RESET)
				{
					// Need to get masks?
					if (!backdrop_icon_border(object) && !object->image_mask[0])
					{
						// Get masks for this icon
						backdrop_get_masks(object);
					}
				}

				// Valid position?
				if (!(object->flags & BDOF_NO_POSITION))
				{
					// Render this object
					backdrop_draw_object(info, object, BRENDERF_REAL, &info->rp, object->pos.Left, object->pos.Top);
				}
			}
		}

		// Refresh?
		if (flags & BDSF_REFRESH)
		{
			EndRefresh(info->window, ((flags & BDSF_REFRESH_DONE) == BDSF_REFRESH_DONE) ? TRUE : FALSE);

			// End refresh here?
			if ((flags & BDSF_REFRESH_DONE) == BDSF_REFRESH_DONE)
			{
				// Unlock layers
#ifdef LOCKLAYER_OK
				UnlockScreenLayer(info->window->WScreen);
#else
				Permit();
#endif
			}
		}

		// In refresh?
		else if (flags & BDSF_IN_REFRESH)
		{
			// End refresh temporarily
			EndRefresh(info->window, FALSE);

			// Remove clip region
			if (info->clip_region)
				InstallClipRegion(info->window->WLayer, 0);

			// Continue refresh
			BeginRefresh(info->window);

			// Unlock layers
#ifdef LOCKLAYER_OK
			UnlockScreenLayer(info->window->WScreen);
#else
			Permit();
#endif
		}

		// Remove clip region
		else if (!(flags & BDSF_NO_CLIP) && info->clip_region)
			InstallClipRegion(info->window->WLayer, 0);

		// Update virtual size
		if (flags & BDSF_RECALC)
			backdrop_calc_virtual(info);
	}

	// Unlock window
	FreeSemaphore(&info->window_lock);

	// Unlock backdrop list
	unlock_listlock(&info->objects);
}

// Render an object to the main window
void backdrop_render_object(BackdropInfo *info, BackdropObject *object, UWORD flags)
{
	// Lock window
	GetSemaphore(&info->window_lock, SEMF_EXCLUSIVE, 0);

	// Window open?
	if (info->window)
	{
		// Install clip?
		if (flags & BRENDERF_CLIP && info->clip_region)
			InstallClipRegion(info->window->WLayer, info->clip_region);

		// Draw object
		backdrop_draw_object(info, object, flags | BRENDERF_REAL, &info->rp, object->pos.Left, object->pos.Top);

		// Remove clip region
		if (flags & BRENDERF_CLIP && info->clip_region)
			InstallClipRegion(info->window->WLayer, 0);
	}

	// Unlock window
	FreeSemaphore(&info->window_lock);
}

// Call an AppIcon's Workbench render hook (WBAPPICONA_RenderHook).
// Returns TRUE if the hook wants the regular image drawn as well.
static BOOL backdrop_call_render_hook(struct Hook *hook,
									  BackdropObject *object,
									  struct RastPort *rp,
									  short left,
									  short top)
{
	struct AppIconRenderMsg msg;
	struct RastPort hook_rp;

	// The hook may change pens, draw mode or font; give it a copy
	hook_rp = *rp;

	msg.arm_RastPort = &hook_rp;
	msg.arm_Icon = object->icon;
	msg.arm_Label = object->name;
	msg.arm_Tags = NULL;
	msg.arm_Left = left;
	msg.arm_Top = top;
	msg.arm_Width = object->pos.Width;
	msg.arm_Height = object->pos.Height;
	msg.arm_State = (object->state) ? IDS_SELECTED : IDS_NORMAL;

	return (BOOL)(CallHookPkt(hook, NULL, &msg) != 0);
}

// Draw an object
void backdrop_draw_object(BackdropInfo *info,
						  BackdropObject *object,
						  UWORD flags,
						  struct RastPort *rp,
						  short left,
						  short top)
{
	UBYTE fpen = 1, bpen = 0, drawmode = JAM2, opus_drawmode = JAM2;
	UWORD *imagedata = 0;
	struct Image *image = 0;
	struct Rectangle rect;
	short len;
	BOOL comp = 0, draw = 1, state = 0, draw_image = 1, draw_label = 1;
	short has_border;
	struct Hook *render_hook = NULL;

	// No icon?
	if (!object->icon)
		return;

	// See if icon has no border
	has_border = backdrop_icon_border(object);

	// AppIcon that renders itself? Drag images always use the plain image
	if (flags & BRENDERF_REAL && object->type == BDO_APP_ICON)
		render_hook = ((AppEntry *)object->misc_data)->render_hook;

	// Not just clearing image?
	if (!(flags & BRENDERF_CLEAR))
	{
		// Get image to render
		if ((image = (struct Image *)object->icon->do_Gadget.GadgetRender))
			imagedata = image->ImageData;

		// Is icon selected?
		if (object->state)
		{
			// Is there a select image?
			if (object->icon->do_Gadget.SelectRender && (object->icon->do_Gadget.Flags & GFLG_GADGHIMAGE))
			{
				if ((image = (struct Image *)object->icon->do_Gadget.SelectRender))
					imagedata = image->ImageData;
				state = 1;
			}
			else
				comp = 1;
		}
	}

#ifdef USE_DRAWICONSTATE
	if (GetIconRectangle(rp, object->icon, NULL, &rect, ICONDRAWA_Borderless, TRUE, TAG_DONE))
	{
		object->pos.Width = rect.MaxX - rect.MinX + 1;
		object->pos.Height = rect.MaxY - rect.MinY + 1;
	}
#elif defined(__MORPHOS__)
	if (ISOWN(object->icon))
	{
		struct OwnDiskObject *o = (APTR)object->icon;
		object->pos.Width = o->pngimage_width;
		object->pos.Height = o->pngimage_height;
	}
#endif

	// Get object position
	if (flags & BRENDERF_REAL)
	{
		short border_x = 0, border_y_top = 0, border_y_bottom = 0;

		// Border?
		if (has_border)
		{
			// Use default border
			border_x = ICON_BORDER_X;
			border_y_top = ICON_BORDER_Y_TOP;
			border_y_bottom = ICON_BORDER_Y_BOTTOM;
		}

		// Calculate full size
		object->full_size.MinX = left - border_x;
		object->full_size.MinY = top - border_y_top;
		object->full_size.MaxX = left + object->pos.Width + border_x - 1;
		object->full_size.MaxY = top + object->pos.Height + border_y_bottom - 1;

		// Get image rectangle
		rect.MinX = object->full_size.MinX + (info->size.MinX - info->offset_x);
		rect.MinY = object->full_size.MinY + (info->size.MinY - info->offset_y);
		rect.MaxX = object->full_size.MaxX + (info->size.MinX - info->offset_x);
		rect.MaxY = object->full_size.MaxY + (info->size.MinY - info->offset_y);

		// Store position
		object->show_rect = rect;
		object->image_rect = rect;

		// Is object offscreen?
		if (rect.MaxX < info->size.MinX || rect.MaxY < info->size.MinY || rect.MinX > info->size.MaxX ||
			rect.MinY > info->size.MaxY)
			draw = 0;

		// Offset by coordinates
		left += info->size.MinX - info->offset_x;
		top += info->size.MinY - info->offset_y;
	}
	else
	{
		rect.MinX = left;
		rect.MinY = top;
		rect.MaxX = left + object->pos.Width - 1;
		rect.MaxY = top + object->pos.Height - 1;
	}

	// Clear area?
	if (flags & BRENDERF_CLEAR)
	{
		// Ok to draw?
		if (draw)
		{
			// Clear whole icon?
			if (!(flags & BRENDERF_LABEL))
			{
				EraseRect(rp, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);
			}
		}

		// Can't draw
		else
			flags &= ~BRENDERF_CLEAR;
	}

	// Object state changed?
	else if (object->flags & BDOF_STATE_CHANGE)
	{
#ifndef USE_DRAWICONSTATE
	// Need to clear if transparent
	#if !defined(__MORPHOS__)
		if (!has_border)
	#endif
			EraseRect(rp, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);
#endif
		object->flags &= ~BDOF_STATE_CHANGE;
	}

	// Draw as normal
	if (!(flags & BRENDERF_CLEAR))
	{
		// Not drawing if no image
		if (!imagedata && !render_hook)
			draw = 0;

		// Ok to draw?
		if (draw)
		{
			struct BitMap bitmap;
			BOOL use_mask = 0;

			// Full icon?
			if (flags & BRENDERF_REAL)
			{
				// Want border?
				if (has_border)
				{
					// Draw border around icon
					DrawBox(rp, &rect, GUI->draw_info, object->state);

					// Get background colour
#ifdef USE_64BIT
					SetAPen(rp,
							(object->flags & BDOF_BACKGROUND) ? object->pen : GUI->draw_info->dri_Pens[BACKGROUNDPEN]);
#else
					SetAPen(rp,
							(object->flags & BDOF_BACKGROUND) ? object->size : GUI->draw_info->dri_Pens[BACKGROUNDPEN]);
#endif

					// Clear boundary around image
#ifdef USE_DRAWICONSTATE
					RectFill(rp, rect.MinX + 1, rect.MinY + 1, rect.MaxX - 1, rect.MaxY - 1);
#else
					RectFill(rp, rect.MinX + 1, rect.MinY + 1, rect.MaxX - 1, rect.MinY + 2);
					RectFill(rp, rect.MinX + 1, rect.MinY + 1, rect.MinX + 3, rect.MaxY - 1);
					RectFill(rp, rect.MinX + 4, rect.MaxY - 3, rect.MaxX - 1, rect.MaxY - 1);
					RectFill(rp, rect.MaxX - 3, rect.MinY + 3, rect.MaxX - 1, rect.MaxY - 4);
#endif
				}
#ifdef USE_DRAWICONSTATE
				else
				{
					EraseRect(rp, rect.MinX, rect.MinY, rect.MaxX, rect.MaxY);
				}
#endif
			}

			// Let the AppIcon draw itself. If the hook says it drew everything, Workbench
			// draws neither the image nor the label, so we don't either
			if (render_hook)
			{
				if (backdrop_call_render_hook(render_hook, object, rp, left, top))
					draw_image = (imagedata != 0);
				else
				{
					draw_image = 0;
					draw_label = 0;
				}
			}

			if (draw_image)
			{
#ifdef USE_DRAWICONSTATE
				DrawIconState(rp,
							  object->icon,
							  NULL,
							  left,
							  top,
							  object->state ? IDS_SELECTED : IDS_NORMAL,
							  ICONDRAWA_Frameless,
							  TRUE,
							  ICONDRAWA_Borderless,
							  TRUE,
							  ICONDRAWA_EraseBackground,
							  FALSE,
							  TAG_DONE);
#else
	#if defined(__MORPHOS__)
				if (ISOWN(object->icon))
				{
					IPTR tags[] = {BLTBMA_USESOURCEALPHA, TRUE, TAG_DONE};
					struct OwnDiskObject *o = (APTR)object->icon;
					BltBitMapRastPortAlpha(o->pngimage,
										   0,
										   0,
										   rp,
										   left,
										   top,
										   o->pngimage_width,
										   o->pngimage_height,
										   (struct TagItem *)&tags);

					if (object->state)
					{
						ProcessPixelArray(
							rp, left, top, o->pngimage_width, o->pngimage_height, POP_TINT, 0x5082ff, NULL);
					}
				}
				else
				{
	#endif
					// Get image as a bitmap
					backdrop_image_bitmap(info, image, imagedata, &bitmap);

					// Using a mask (no border)?
					if (!has_border && object->image_mask[state] &&
						backdrop_blit_icon_mask(&bitmap, rp, left, top, image, object->image_mask[state]))
					{
						use_mask = 1;
					}

					// Draw normally
					else
					{
						// Draw image
						BltBitMapRastPort(&bitmap, 0, 0, rp, left, top, image->Width, image->Height, 0xc0);
					}

					// Complement?
					if (comp)
					{
						DragInfo *drag_info;
						UBYTE mask;

						// Only complement to image depth
						mask = rp->Mask;
						SetWrMsk(rp, image->PlanePick);

						// Need backfill?
						if ((object->icon->do_Gadget.Flags & GFLG_GADGBACKFILL) &&
							(drag_info = GetDragInfo(info->window, 0, -object->pos.Width, -object->pos.Height, 0)))
						{
							// Draw icon into drag buffer
							BltBitMapRastPort(
								&bitmap, 0, 0, &drag_info->drag_rp, 0, 0, image->Width, image->Height, 0xc0);

							// Build mask
							GetDragMask(drag_info);

							// Complement area with mask
							BltMaskBitMapRastPort(&drag_info->drag_bm,
												  0,
												  0,
												  rp,
												  left,
												  top,
												  image->Width,
												  image->Height,
												  0x20,
												  (PLANEPTR)drag_info->bob.ImageShadow);

							// Free temporary drag info
							FreeDragInfo(drag_info);
						}

						// Just complement image
						else
						{
							ClipBlit(rp, left, top, rp, left, top, image->Width, image->Height, 0x50);
						}

						// Restore mask
						SetWrMsk(rp, mask);
					}
	#if defined(__MORPHOS__)
				}
	#endif
#endif
			}

			// Left out (on desktop), or a link?
			if (!(environment->env->desktop_flags & DESKTOPF_NO_ARROW) &&
				((object->type == BDO_LEFT_OUT && !(object->flags & BDOF_DESKTOP_FOLDER) &&
				  info->flags & BDIF_MAIN_DESKTOP) ||
				 object->flags & BDOF_LINK_ICON))
			{
				struct Image *image;

				// Get correct image for this resolution
				image = (GUI->screen_info & SCRI_LORES) ? &arrow_image[1] : &arrow_image[0];

				// Is object big enough for the 'shortcut arrow'?
				if (object->pos.Width > (image->Width << 1) && object->pos.Height > image->Height + 4)
				{
					// Draw arrow in bottom-left corner
					DrawImage(rp, image, rect.MinX, rect.MaxY - image->Height + 1);
				}
			}

			// Ghosted icon?
			if (object->flags & BDOF_GHOSTED)
			{
				// Draw ghosting over the icon
				backdrop_draw_icon_ghost(rp, &rect, (use_mask) ? (PLANEPTR)object->image_mask[state] : 0);
			}
		}

		// Get pens from configuration
		fpen = (info->flags & BDIF_MAIN_DESKTOP) ? environment->env->icon_fpen : environment->env->iconw_fpen;
		bpen = (info->flags & BDIF_MAIN_DESKTOP) ? environment->env->icon_bpen : environment->env->iconw_bpen;
		drawmode = (info->flags & BDIF_MAIN_DESKTOP) ? environment->env->icon_style : environment->env->iconw_style;
		opus_drawmode = drawmode;

		// Shadow/Outline?
		if (drawmode == MODE_SHADOW || drawmode == MODE_OUTLINE)
		{
			UBYTE temp;

			// Set back to JAM1
			drawmode = JAM1;

			// Swap the pens
			temp = fpen;
			fpen = bpen;
			bpen = temp;
		}

		// Fix pens for user colours
		if (fpen >= 4 && fpen < 252)
			fpen = GUI->pens[fpen - 4];
		if (bpen >= 4 && bpen < 252)
			bpen = GUI->pens[bpen - 4];

		// Use fast call under 39
		if (((struct Library *)GfxBase)->lib_Version >= 39)
		{
			SetABPenDrMd(rp, fpen, bpen, drawmode);
		}
		else
		{
			SetAPen(rp, fpen);
			SetBPen(rp, bpen);
			SetDrMd(rp, drawmode);
		}
	}

	// Full icon?
	if (flags & BRENDERF_REAL && draw_label && !(object->flags & BDOF_NO_LABEL))
	{
		char *name, namebuf[40];

		// Bad disk?
		if (object->type == BDO_BAD_DISK)
		{
			// Get bad disk name
			backdrop_bad_disk_name(object, namebuf);
			name = namebuf;
		}

		// Custom label?
		else if (object->flags & BDOF_CUSTOM_LABEL)
			name = object->device_name;

		// Get name pointer
		else
			name = object->name;

		// Get text length
		if ((len = strlen(name)))
		{
			struct TextExtent extent;
			short max_width = 0, loop, textx, texty;

			// Splitting long labels?
			if (environment->env->desktop_flags & DESKTOPF_SPLIT_LABELS)
			{
				// Calculate maximum text width (minimum 6 chars)
				max_width = RECTWIDTH(&rect) + (RECTWIDTH(&rect) >> 1);
				if (max_width < rp->Font->tf_XSize * 6)
					max_width = rp->Font->tf_XSize * 6;
			}

			// Position for text
			textx = rect.MinX + (RECTWIDTH(&rect) >> 1);
			texty = rect.MaxY + ICON_LABEL_SPACE + rp->TxBaseline + ((opus_drawmode == MODE_OUTLINE) ? 1 : 0);
			Move(rp, textx, texty);

			// Get text pixel size
			TextMultiLine(rp, name, len, max_width, TMLF_CENTER | TMLF_EXTENT, &extent);

			// Fix extent for shadow/outline
			if (opus_drawmode == MODE_SHADOW)
			{
				extent.te_Extent.MaxX++;
				extent.te_Width++;
				extent.te_Height++;
			}
			else if (opus_drawmode == MODE_OUTLINE)
			{
				extent.te_Extent.MinX--;
				extent.te_Extent.MaxX++;
				extent.te_Width += 2;
				extent.te_Height += 2;
			}

			// Stretch out rectangle sides if necessary
			if (extent.te_Extent.MinX < rect.MinX)
				object->full_size.MinX -= rect.MinX - extent.te_Extent.MinX;
			if (extent.te_Extent.MaxX > rect.MaxX)
				object->full_size.MaxX += extent.te_Extent.MaxX - rect.MaxX;
			if (object->show_rect.MinX > extent.te_Extent.MinX)
				object->show_rect.MinX = extent.te_Extent.MinX;
			if (object->show_rect.MaxX < extent.te_Extent.MinX + extent.te_Width - 1)
				object->show_rect.MaxX = extent.te_Extent.MinX + extent.te_Width - 1;

			// Save bottom of text
			object->show_rect.MaxY += ICON_LABEL_SPACE + extent.te_Height;	// +((opus_drawmode==MODE_OUTLINE)?1:0);
			object->full_size.MaxY += ICON_LABEL_SPACE + extent.te_Height;	// +((opus_drawmode==MODE_OUTLINE)?1:0);

			// Clear?
			if (flags & BRENDERF_CLEAR)
			{
				EraseRect(rp,
						  extent.te_Extent.MinX,
						  rect.MaxY + ICON_LABEL_SPACE + ((opus_drawmode == MODE_OUTLINE) ? 1 : 0),
						  extent.te_Extent.MinX + extent.te_Width - 1,
						  rect.MaxY + ICON_LABEL_SPACE + extent.te_Height - 1);
			}

			// Draw text
			else
			{
				// Is object selected?
				if (object->flags & BDOF_SELECTED)
				{
					if (drawmode == JAM2)
						SetDrMd(rp, INVERSVID | JAM2);
					else if (fpen != GUI->draw_info->dri_Pens[HIGHLIGHTTEXTPEN])
						SetAPen(rp, GUI->draw_info->dri_Pens[HIGHLIGHTTEXTPEN]);
					else
						SetAPen(rp, GUI->draw_info->dri_Pens[BACKGROUNDPEN]);
				}

				// Shadow
				if (opus_drawmode == MODE_SHADOW)
				{
					// Draw shadow text first
					Move(rp, textx + 1, texty + 1);
					TextMultiLine(rp, name, len, max_width, TMLF_CENTER, 0);

					// Draw text
					SetAPen(rp, bpen);
					Move(rp, textx, texty);
					TextMultiLine(rp, name, len, max_width, TMLF_CENTER, 0);
				}

				// Normal text
				else if (opus_drawmode != MODE_OUTLINE)
				{
					// Render text
					TextMultiLine(rp, name, len, max_width, TMLF_CENTER, 0);
				}

				// Outline
				else
				{
					static const BYTE outline_offset[8][2] = {
						{-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}};

					// Draw the outline at all eight neighbouring positions; diagonals alone
					// leave gaps around thin (1 pixel) strokes
					for (loop = 0; loop < 8; loop++)
					{
						Move(rp, textx + outline_offset[loop][0], texty + outline_offset[loop][1]);
						TextMultiLine(rp, name, len, max_width, TMLF_CENTER, 0);
					}

					// Draw text on top
					SetAPen(rp, bpen);
					Move(rp, textx, texty);
					TextMultiLine(rp, name, len, max_width, TMLF_CENTER, 0);
				}

				// Reset draw mode if necessary
				if (object->flags & BDOF_SELECTED)
					SetDrMd(rp, drawmode);
			}
		}
	}
}

// Convert an image to a bitmap
void backdrop_image_bitmap(BackdropInfo *info, struct Image *image, UWORD *imagedata, struct BitMap *bitmap)
{
	short depth, plane = 1;
	long planesize;

	// Get depth
	depth = info->window->WScreen->RastPort.BitMap->Depth;

	// Initialise bitmap
	InitBitMap(bitmap, depth, image->Width, image->Height);

	// Get plane size
	planesize = ((image->Width + 15) >> 4) * image->Height;

	// First plane pointer
	bitmap->Planes[0] = (PLANEPTR)imagedata;

	// More than 1 plane?
	if (image->Depth > 1)
	{
		// Get second plane pointer
		bitmap->Planes[plane++] = (PLANEPTR)(imagedata + planesize);

		// If it's 3 planes, under 39, we can remap it
		if (image->Depth == 3 && ((struct Library *)GfxBase)->lib_Version >= 39 && remap_icon_images())
		{
			for (plane = 2; plane < depth; plane++)
				bitmap->Planes[plane] = (PLANEPTR)(imagedata + (planesize << 1));
		}

		// More than 2 planes?
		else if (image->Depth > 2)
		{
			for (plane = 2; plane < image->Depth; plane++)
				bitmap->Planes[plane] = (PLANEPTR)(imagedata + planesize * plane);
		}
	}

	// Clear extra planes
	for (; plane < depth; plane++)
		bitmap->Planes[plane] = 0;
}

// Are the icon's masks in the wrong memory type for the current screen?
BOOL backdrop_masks_misplaced(BackdropObject *object)
{
#ifdef USE_DRAWICONSTATE
	// Masks are never blitted; any memory will do
	return FALSE;
#else
	BOOL in_chip, want_chip;

	if (!object->image_mask[0])
		return FALSE;
	in_chip = (TypeOfMem(object->image_mask[0]) & MEMF_CHIP) ? TRUE : FALSE;
	want_chip = (backdrop_mask_memory() & MEMF_CHIP) ? TRUE : FALSE;
	return (BOOL)(in_chip != want_chip);
#endif
}

// Get image mask for an icon
void backdrop_get_masks(BackdropObject *object)
{
	short a;
	struct Image *image;

	// Free existing masks
	for (a = 0; a < 2; a++)
	{
		FreeVec(object->image_mask[a]);
		object->image_mask[a] = 0;
	}

	// See if icon has border - no mask if so
	if (!backdrop_icon_border(object))
	{
		// Get first image data
		image = (struct Image *)object->icon->do_Gadget.GadgetRender;

		// Do two images
		for (a = 0; a < 2; a++)
		{
			// Allocate mask (chip only if the blitter will read it)
			if (!(object->image_mask[a] = AllocVec(((image->Width + 15) >> 4) * image->Height * sizeof(UWORD),
												   backdrop_mask_memory() | MEMF_CLEAR)))
				break;

			// Full transparency? (quicker)
			if (environment->env->desktop_flags & DESKTOPF_TRANSPARENCY)
			{
				short plane, row, col, word, width, off;

				// Get width in words
				width = (image->Width + 15) >> 4;

				// Build shadow mask
				for (plane = 0, off = 0; plane < image->Depth; plane++)
				{
					for (row = 0, word = 0; row < image->Height; row++)
					{
						for (col = 0; col < width; col++, word++, off++)
						{
							// Build mask
							object->image_mask[a][word] |= image->ImageData[off];
						}
					}
				}
			}

			// Only transparent around the edge
			else
			{
				// Build mask
				BuildTransDragMask(
					object->image_mask[a], image->ImageData, image->Width, image->Height, image->Depth, 0);
			}

			// Get second image
			if (!(image = (struct Image *)object->icon->do_Gadget.SelectRender))
				break;
		}
	}
}

// Install clip region
void backdrop_install_clip(BackdropInfo *info)
{
	// Or clip rectangle in to region
	if (info->clip_region)
	{
		ClearRegion(info->clip_region);
		OrRectRegion(info->clip_region, &info->size);
	}
}

// Clear clip region
void backdrop_clear_region(BackdropInfo *info)
{
	// Clear clip region
	if (info->clip_region)
		ClearRegion(info->clip_region);
}

// Add rectangle to region
void backdrop_add_region(BackdropInfo *info, struct Rectangle *rect)
{
	// Or into region
	if (info->clip_region)
		OrRectRegion(info->clip_region, rect);
}

// Check region is within bounds
void backdrop_region_bounds(BackdropInfo *info)
{
	// And with full rectangle
	if (info->clip_region)
		AndRectRegion(info->clip_region, &info->size);
}

// Erase an icon
void backdrop_erase_icon(BackdropInfo *info, BackdropObject *object, UWORD flags)
{
	BackdropObject *icon;

	// Lock icon list
	lock_listlock(&info->objects, FALSE);

	// Lock window
	GetSemaphore(&info->window_lock, SEMF_EXCLUSIVE, 0);

	// Window open?
	if (info->window)
	{
		// Install clip?
		if (info->clip_region)
			InstallClipRegion(info->window->WLayer, info->clip_region);

		// Erase icon area
		EraseRect(
			&info->rp, object->show_rect.MinX, object->show_rect.MinY, object->show_rect.MaxX, object->show_rect.MaxY);

		// Got through icons
		for (icon = (BackdropObject *)info->objects.list.lh_Head; icon->node.ln_Succ;
			 icon = (BackdropObject *)icon->node.ln_Succ)
		{
			// Valid position?
			if (icon->flags & BDOF_NO_POSITION)
				continue;

			// Not the one we just removed?
			if (icon == object)
				continue;

			// Did its box intersect the one we removed?
			if (geo_box_intersect(&object->show_rect, &icon->show_rect))
			{
				// ReDraw object
				backdrop_draw_object(info, icon, BRENDERF_REAL, &info->rp, icon->pos.Left, icon->pos.Top);
			}
		}

		// Remove clip region
		InstallClipRegion(info->window->WLayer, 0);

		// Update virtual size
		if (flags & BDSF_RECALC)
			backdrop_calc_virtual(info);
	}

	// Unlock window
	FreeSemaphore(&info->window_lock);

	// Unlock icon list
	unlock_listlock(&info->objects);
}

// Draw ghosted image over icon
void backdrop_draw_icon_ghost(struct RastPort *rp, struct Rectangle *rect, PLANEPTR mask)
{
	struct BitMap *ghost_bm;
	struct RastPort ghost_rp;
	UWORD *new_mask = 0;
	unsigned short width, height, words;

	// Get image size
	width = RECTWIDTH(rect);
	height = RECTHEIGHT(rect);

	// Get image size in words
	words = ((width + 15) >> 4) * height;

	// Got a mask?
	if (mask)
	{
		// Allocate a copy of the mask (the blitter needs chip memory for a native bitmap)
		if ((new_mask = AllocVec(words * sizeof(UWORD), display_rtg_bitmap(rp->BitMap) ? MEMF_ANY : MEMF_CHIP)))
		{
			short word, line, count;
			unsigned short mask_word = 0xaaaa;

			// Copy mask
			CopyMem((char *)mask, (char *)new_mask, words * sizeof(UWORD));

			// Words per line
			line = (width + 15) >> 4;

			// Stipple mask
			for (word = 0, count = 0; word < words; word++)
			{
				// Mask off
				new_mask[word] &= ~mask_word;

				// New line?
				if (++count == line)
				{
					// Switch mask value to create alternating dotted lines
					mask_word = (mask_word == 0xaaaa) ? 0x5555 : 0xaaaa;
					count = 0;
				}
			}

			// Use new mask as mask
			mask = (PLANEPTR)new_mask;
		}

		// Original mask is in fast memory and the blitter would read it
		else if (!backdrop_mask_blittable(rp->BitMap, mask))
			mask = 0;
	}

	// No mask?
	if (!mask)
	{
		static UWORD stipple[2] = {0xaaaa, 0x5555};

		// Set stipple fill
		SetAfPt(rp, stipple, 1);

		// Fill in black
		SetAPen(rp, 1);
		SetDrMd(rp, JAM1);

		// Draw over the image
		RectFill(rp, rect->MinX, rect->MinY, rect->MaxX, rect->MaxY);

		// Clear stipple fill
		SetAfPt(rp, 0, 0);
		return;
	}

	// Create ghost bitmap
	if (!(ghost_bm = NewBitMap(width, height, 1, 0, rp->BitMap)))
	{
		FreeVec(new_mask);
		return;
	}

	// Initialise ghost rastport
	InitRastPort(&ghost_rp);
	ghost_rp.BitMap = ghost_bm;

	// Set rastport to black
	SetRast(&ghost_rp, 1);

	// Blit over icon (mask will create the stipple effect)
	BltMaskBitMapRastPort(ghost_bm, 0, 0, rp, rect->MinX, rect->MinY, width, height, 0xe0, mask);

	// Wait for blitter
	WaitBlit();

	// Free ghost bitmap and mask
	DisposeBitMap(ghost_bm);
	FreeVec(new_mask);
}

// See if an icon has a border
BOOL backdrop_icon_border(BackdropObject *icon)
{
	ULONG iflags;

	// See if icon has no border
	if (((iflags = GetIconFlags(icon->icon)) & ICONF_BORDER_OFF) ||
		(environment->env->desktop_flags & DESKTOPF_NO_BORDERS && !(iflags & ICONF_BORDER_ON)))
		return 0;
	return 1;
}

// Get the name of a bad disk
void backdrop_bad_disk_name(BackdropObject *object, char *namebuf)
{
	ULONG pad[2];
	char *ptr;
	short num;

	// Build name
	strcpy(namebuf, object->name);

	// Get dos type; quick null-padding using ULONGs
	pad[0] = object->misc_data;
	pad[1] = 0;
	ptr = (char *)pad;
#ifdef __AROS__
	pad[0] = AROS_BE2LONG(pad[0]);
#endif

	// Check for printable characters
	for (num = 0; num < 4; num++, ptr++)
	{
		// Lowercase?
		if (*ptr >= 'a' && *ptr <= 'Z')
			*ptr -= ('a' - 'A');

		// Number?
		else if (*ptr < 10)
			*ptr += '0';

		// Non-letter?
		else if (*ptr < 'A' || *ptr > 'Z')
		{
			// Non-printable?
			if (*ptr < '0' || *ptr > '9')
				*ptr = '?';
		}
	}

	// Kill a trailing zero
	ptr = (char *)pad;
	if (ptr[3] == '0')
		ptr[3] = 0;

	// Get name pointer
	strcat(namebuf, (char *)pad);
}
