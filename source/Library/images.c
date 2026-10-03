/*

Directory Opus 5
Original APL release version 5.82
Copyright 1993-2012 Jonathan Potter & GP Software
Copyright 2012-2013 DOPUS5 Open Source Team
Copyright 2023-2026 Dimitris Panokostas (dopus5 fork)

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

#include <intuition/intuitionbase.h>
#include <proto/newicon.h>

#ifdef __AROS__
static void aros_image_debug_log(char *);
static Image_Data *aros_read_image_fallback(char *, OpenImageInfo *);
#endif

// Open an image
APTR LIBFUNC L_OpenImage(REG(a0, char *name), REG(a1, OpenImageInfo *info))
{
	Image_Data *image;

	// No name, and no info?
	if ((!name || !name[0]) && !info)
		return 0;

	// Lock image list
	L_GetSemaphore(&image_lock, SEMF_EXCLUSIVE, 0);

	// Look for image in image list
#ifdef IMAGE_CACHING
	if (!name || !(image = (Image_Data *)L_FindNameI(&image_list, name)))
	{
#endif
		// Read a new image
		image = read_image(name, info);
#ifdef __AROS__
		if (!image)
			image = aros_read_image_fallback(name, info);
#endif
		if (image)
		{
			// Allocate name
			if (!name || (image->node.ln_Name = L_AllocMemH(image_memory, strlen(name) + 1)))
			{
				// Copy name
				if (name)
					strcpy(image->node.ln_Name, name);

				// Link in to list
				AddTail(&image_list, &image->node);
			}
			else
			{
				free_image(image);
				image = 0;
			}
		}
#ifdef IMAGE_CACHING
	}
#endif

	// Unlock image list
	L_FreeSemaphore(&image_lock);

	// Get an image?
	if (image)
	{
		// Increment open count
		++image->count;
	}

	return image;
}

// Close an image
void LIBFUNC L_CloseImage(REG(a0, APTR ptr))
{
	// Lock image list
	L_GetSemaphore(&image_lock, SEMF_EXCLUSIVE, 0);

	// Valid image?
	if (ptr)
	{
		Image_Data *image;

		// Get image pointer
		image = (Image_Data *)ptr;

		// Decrement open count
		--image->count;

		// Flush image?
		if (image->count == 0)
		{
			// Remove image from the list
			Remove(&image->node);

			// Free image
			free_image(image);
		}
	}

	// Unlock image list
	L_FreeSemaphore(&image_lock);
}

// Copy an image
APTR LIBFUNC L_CopyImage(REG(a0, APTR ptr))
{
	// Lock image list
	L_GetSemaphore(&image_lock, SEMF_EXCLUSIVE, 0);

	// Valid image?
	if (ptr)
	{
		Image_Data *image;

		// Get image pointer
		image = (Image_Data *)ptr;

		// Increment open count
		++image->count;
	}

	// Unlock image list
	L_FreeSemaphore(&image_lock);

	return ptr;
}

// Flush all images
void LIBFUNC L_FlushImages(void)
{
	Image_Data *image;

	// Lock image list
	L_GetSemaphore(&image_lock, SEMF_EXCLUSIVE, 0);

	// Go through image list
	for (image = (Image_Data *)image_list.lh_Head; image->node.ln_Succ;)
	{
		Image_Data *next = (Image_Data *)image->node.ln_Succ;

		// Remove image and free it
		Remove(&image->node);
		free_image(image);

		// Get next
		image = next;
	}

	// Free image memory handle
	L_FreeMemHandle(image_memory);
	image_memory = 0;

	// Initialise image list
	NewList(&image_list);

	// Unlock image list
	L_FreeSemaphore(&image_lock);
}

// Draw an image
short LIBFUNC L_RenderImage(REG(a0, struct RastPort *rp),
							REG(a1, APTR imptr),
							REG(d0, unsigned short left),
							REG(d1, unsigned short top),
							REG(a2, struct TagItem *tags))
{
	short state = 0, mask = 0;
	struct TagItem *tag;
	struct Rectangle *rect = 0;
	short plane;
	unsigned short **planeptrs;
	unsigned long planesize;
	unsigned short *mask_plane = 0;
	UBYTE old_mask;
	short s_x, s_y, s_w, s_h, depth;
	short clip_bound, erase = -1, backfill;
	Image_Data *image = (Image_Data *)imptr;
	struct BitMap bitmap, *scale_bm = 0;
	BOOL in_chip = 0, plane_ok = 0, no_remap = 0, scale = 0;
	short width = 0, height = 0, mask_depth;
	short ret = 1;

	// Invalid image?
	if (!image)
		return 0;

	// Get state
	if ((tag = FindTagItem(IM_State, tags)))
	{
		// Want alternate image?
		if ((state = tag->ti_Data))
		{
			// Haven't got one?
			if (!(image->flags & IMAGEF_SEL_IMAGE))
			{
				// Want to return straight away?
				if (FindTagItem(IM_NoDrawInvalid, tags))
					return 0;

				// Draw primary image
				state = 0;
				ret = 0;
			}

			// Otherwise, is it in chip memory?
			else if (image->flags & IMAGEF_ALT_CHIP)
				in_chip = 1;
		}
	}

	// Erase background?
	if ((tag = FindTagItem(IM_Erase, tags)))
		erase = tag->ti_Data;
	backfill = GetTagData(IM_Backfill, 0, tags);

	// Don't remap icons?
	if (GetTagData(IM_NoIconRemap, 0, tags))
		no_remap = 1;

	// Get rectangle
	if ((tag = FindTagItem(IM_Rectangle, tags)))
	{
		rect = (struct Rectangle *)tag->ti_Data;

		// Fix left and top
		left = rect->MinX + (((rect->MaxX - rect->MinX) + 1 - image->width) >> 1);
		top = rect->MinY + (((rect->MaxY - rect->MinY) + 1 - image->height) >> 1);
	}

	// Clip boundary
	clip_bound = GetTagData(IM_ClipBoundary, 2, tags);

	// Get mask
	if ((tag = FindTagItem(IM_Mask, tags)))
		mask = tag->ti_Data;

	// Scale?
	if (GetTagData(IM_Scale, 0, tags))
		scale = 1;

	// Image in chip memory?
	if (state == 0 && image->flags & IMAGEF_CHIP)
		in_chip = 1;

	// Initialise bitmap
	InitBitMap(&bitmap, rp->BitMap->Depth, image->width, image->height);
	if (bitmap.Depth > 8)
		bitmap.Depth = 8;

	// Get image depth
	depth = image->depth;
	mask_depth = depth;

	// Get plane pointers
	planeptrs = image->planes[state];

	// Calculate plane size
	planesize = ((image->width + 15) >> 4) * image->height;

	// Get image lock
	L_GetSemaphore(&image->lock, SEMF_SHARED, 0);

	// Got remap planes?
	if (image->remap_count)
	{
		// Get depth
		depth = image->remap_depth;
		mask_depth = depth;

		// Got remapped image?
		if (image->remap_image[state])
		{
			long planesize;
			unsigned char *ptr;

			// Get size of image plane, and pointer to data
			planesize = RASSIZE(image->remap_image[state]->Width, image->remap_image[state]->Height);
			ptr = (unsigned char *)image->remap_image[state]->ImageData;

			// Set plane pointers in bitmap
			for (plane = 0; plane < depth; plane++)
			{
				bitmap.Planes[plane] = (PLANEPTR)ptr;
				ptr += planesize;
			}
			for (; plane < bitmap.Depth; plane++)
				bitmap.Planes[plane] = 0;

			// Get plane pointers
			planeptrs = (unsigned short **)bitmap.Planes;
		}

		// Normal remap
		else
		{
			// Set plane pointers in bitmap
			for (plane = 0; plane < bitmap.Depth; plane++)
				bitmap.Planes[plane] = (PLANEPTR)image->remap_planes[state][plane];

			// Get plane pointers
			planeptrs = image->remap_planes[state];
		}

		// Set flags so we won't try to copy to chip memory
		in_chip = 1;
		plane_ok = 1;
	}

	// Is image not in chip memory?
	else if (!in_chip)
	{
		// Allocate fake plane
		if ((bitmap.Planes[0] = AllocVec(planesize << 1, MEMF_CHIP)))
		{
			// Set fake plane pointers
			for (plane = 1; plane < bitmap.Depth; plane++)
				bitmap.Planes[plane] = bitmap.Planes[0];
		}
	}

	// Otherwise
	else
	{
		// Set plane pointers in bitmap
		for (plane = 0; plane < depth; plane++)
			bitmap.Planes[plane] = (PLANEPTR)image->planes[state][plane];
		for (; plane < bitmap.Depth; plane++)
			bitmap.Planes[plane] = 0;
	}

	// Got a valid plane?
	if (bitmap.Planes[0] || plane_ok)
	{
		BOOL remap = 0;

		// Icon image?
		if (image->flags & IMAGEF_ICON_IMAGE && !(image->flags & IMAGEF_NEWICON))
		{
			// Three planes, under 39?
			if (image->depth == 3 && ((struct Library *)GfxBase)->lib_Version >= 39 && !no_remap)
			{
				// Set remap flag
				remap = 1;

				// Remap to top of palette
				for (plane = 3; plane < bitmap.Depth; plane++)
					bitmap.Planes[plane] = bitmap.Planes[2];
			}
		}

		// Get source position and size
		s_x = 0;
		s_y = 0;
		s_w = image->width;
		s_h = image->height;

		// Clipping region?
		if (rect)
		{
			// Get size of clip rectangle
			width = RECTWIDTH(rect);
			height = RECTHEIGHT(rect);

			// Scale image?
			if (scale && in_chip && (s_w > width - clip_bound || s_h > height - clip_bound))
			{
				struct BitScaleArgs args;

				// Fill out arguments
				args.bsa_SrcX = 0;
				args.bsa_SrcY = 0;
				args.bsa_SrcWidth = s_w;
				args.bsa_SrcHeight = s_h;
				args.bsa_DestX = 0;
				args.bsa_DestY = 0;
				args.bsa_DestWidth = (s_w > width - clip_bound) ? width - clip_bound : s_w;
				args.bsa_DestHeight = (s_h > height - clip_bound) ? height - clip_bound : s_h;
				args.bsa_XSrcFactor = args.bsa_SrcWidth;
				args.bsa_XDestFactor = args.bsa_DestWidth;
				args.bsa_YSrcFactor = args.bsa_SrcHeight;
				args.bsa_YDestFactor = args.bsa_DestHeight;
				args.bsa_SrcBitMap = &bitmap;
				args.bsa_Flags = 0;

				// Allocate bitmap for scaling
				if ((scale_bm = L_NewBitMap(args.bsa_DestWidth, args.bsa_DestHeight, bitmap.Depth, 0, 0)))
				{
					short pl;
					PLANEPTR blank_plane = 0;

					// Get plane pointers
					planeptrs = (unsigned short **)scale_bm->Planes;

					// Go through source planes, check for null pointers
					for (pl = 1; pl < bitmap.Depth; pl++)
					{
						// Null plane?
						if (!bitmap.Planes[pl])
						{
							// Allocate null plane?
							if (!blank_plane)
								blank_plane =
									AllocVec(RASSIZE(args.bsa_SrcWidth, args.bsa_SrcHeight), MEMF_CHIP | MEMF_CLEAR);

							// Set plane pointer
							bitmap.Planes[pl] = blank_plane;
						}
					}

					// Set destination bitmap
					args.bsa_DestBitMap = scale_bm;

					// Scale into bitmap
					BitMapScale(&args);

					// Get new size
					s_w = args.bsa_DestWidth;
					s_h = args.bsa_DestHeight;

					// Fix left and top
					left = rect->MinX + (((rect->MaxX - rect->MinX) + 1 - s_w) >> 1);
					top = rect->MinY + (((rect->MaxY - rect->MinY) + 1 - s_h) >> 1);

					// Calculate plane size
					planesize = ((s_w + 15) >> 4) * s_h;
					mask_depth = bitmap.Depth;

					// Wait for blitter
					WaitBlit();

					// Got blank plane?
					if (blank_plane)
					{
						// Free blank plane pointers
						for (pl = 1; pl < bitmap.Depth; pl++)
						{
							// Blank plane?
							if (bitmap.Planes[pl] == blank_plane)
								bitmap.Planes[pl] = 0;
						}

						// Free blank plane
						FreeVec(blank_plane);
					}
				}
			}

			// Don't have a scaled bitmap?
			if (!scale_bm)
			{
				// Does source exceed this size?
				if (s_w > width - clip_bound)
				{
					s_x = (s_w - (width - clip_bound)) >> 1;
					s_w = width - clip_bound;
					left = rect->MinX + (clip_bound >> 1);
				}
				if (s_h > height - clip_bound)
				{
					s_y = (s_h - (height - clip_bound)) >> 1;
					s_h = height - clip_bound;
					top = rect->MinY + (clip_bound >> 1);
				}
			}
		}

		// Need a mask?
		if (mask && (mask_plane = AllocVec(planesize << 1, MEMF_CHIP | MEMF_CLEAR)))
		{
			register long word;
			register short plane;

			// Go through planes of image
			for (plane = 0; plane < mask_depth; plane++)
			{
				// Invalid plane?
				if (!planeptrs[plane])
					continue;

				// Go through image
				for (word = 0; word < planesize; word++)
					mask_plane[word] |= ((UWORD *)planeptrs[plane])[word];
			}
		}

		// Clipping region?
		if (rect)
		{
			// Erase background?
			if (erase != -1)
			{
				// Set background pen
				SetAPen(rp, erase);

				// Erase whole rectangle?
				if (mask_plane)
				{
					// Erase
					if (backfill)
						EraseRect(rp, rect->MinX, rect->MinY, rect->MaxX, rect->MaxY);
					else
						RectFill(rp, rect->MinX, rect->MinY, rect->MaxX, rect->MaxY);
				}

				// Erase around rectangle?
				else if (s_w < width || s_h < height)
				{
					// Erase top and bottom of image
					if (s_h < height)
					{
						if (top > rect->MinY)
							RectFill(rp, rect->MinX, rect->MinY, rect->MaxX, top - 1);
						if (top + s_h <= rect->MaxY)
							RectFill(rp, rect->MinX, top + s_h, rect->MaxX, rect->MaxY);
					}

					// Erase left and right of image
					if (s_w < width)
					{
						if (rect->MinX < left)
							RectFill(rp, rect->MinX, rect->MinY, left - 1, rect->MaxY);
						if (left + s_w <= rect->MaxX)
							RectFill(rp, left + s_w, rect->MinY, rect->MaxX, rect->MaxY);
					}
				}
			}
		}

		// Was image in chip memory?
		if (in_chip)
		{
			// Got a mask?
			if (mask_plane)
			{
				// Draw image
				BltMaskBitMapRastPort(
					(scale_bm) ? scale_bm : &bitmap, s_x, s_y, rp, left, top, s_w, s_h, 0xe2, (PLANEPTR)mask_plane);
			}

			// Otherwise draw without masking
			else
			{
				BltBitMapRastPort((scale_bm) ? scale_bm : &bitmap, s_x, s_y, rp, left, top, s_w, s_h, 0xc0);
			}
		}

		// Nope, need to copy from fast
		else
		{
			// Save old mask
			old_mask = rp->Mask;

			// Draw image
			for (plane = 0; plane < bitmap.Depth; plane++)
			{
				// Valid plane?
				if (plane < depth || remap)
				{
					unsigned short *ptr;

					// Remap plane?
					if (plane >= depth)
						ptr = planeptrs[2];
					else
						ptr = planeptrs[plane];

					// Copy plane data to bitmap
					if (ptr)
						CopyMem((char *)ptr, (char *)bitmap.Planes[0], planesize << 1);
				}

				// Otherwise, clear plane
				else if (plane == depth)
					BltClear(bitmap.Planes[0], planesize << 1, 1);

				// Set write mask to render this plane
				SetWrMsk(rp, 1 << plane);

				// Got a mask?
				if (mask_plane)
				{
					// Draw this plane
					BltMaskBitMapRastPort(&bitmap, s_x, s_y, rp, left, top, s_w, s_h, 0xe2, (PLANEPTR)mask_plane);
				}

				// Otherwise draw without masking
				else
				{
					BltBitMapRastPort(&bitmap, s_x, s_y, rp, left, top, s_w, s_h, 0xc0);
				}
			}

			// Fix write mask
			SetWrMsk(rp, old_mask);
		}

		// Wait for blitter to finish
		WaitBlit();

		// Free mask
		if (mask_plane)
			FreeVec(mask_plane);
		if (!in_chip)
			FreeVec(bitmap.Planes[0]);
		L_DisposeBitMap(scale_bm);
	}

	// Unlock image
	L_FreeSemaphore(&image->lock);
	return ret;
}

// Get some image information
void LIBFUNC L_GetImageAttrs(REG(a0, APTR imptr), REG(a1, struct TagItem *tags))
{
	Image_Data *image = (Image_Data *)imptr;

	// Valid image?
	if (image)
	{
		struct TagItem *tstate;
		struct TagItem *tag;

		// Iterate through tags
		tstate = tags;
		while ((tag = NextTagItem(&tstate)))
		{
			switch (tag->ti_Tag)
			{
			case IM_Width:
				tag->ti_Data = image->width;
				break;

			case IM_Height:
				tag->ti_Data = image->height;
				break;

			case IM_Depth:
				if (image->remap_depth)
					tag->ti_Data = image->remap_depth;
				else
					tag->ti_Data = image->depth;
				break;

			case IM_State:
				if (image->flags & IMAGEF_SEL_IMAGE)
					tag->ti_Data = 1;
				else
					tag->ti_Data = 0;
				break;
			}
		}
	}
}

// Read an image from disk
Image_Data *read_image(char *name, OpenImageInfo *info)
{
	short len = 0;
	Image_Data *data = 0;
	ILBMHandle *ilbm;

	// Is image an icon?
	if (name)
		len = strlen(name);
	if (!info && len > 5 && strcmp(&name[len - 5], ".info") == 0)
	{
		struct DiskObject *obj = NULL;
		struct NewDiskObject *ndo = NULL;
		char *path;

		// Get name without .info
		if ((path = AllocVec(len + 1, 0)))
		{
			strcpy(path, name);
			path[len - 5] = 0;

			// handle OS 3.5 icons
			if (IconBase->lib_Version >= 44)
			{
				struct Screen *screen;
				if ((screen = LockPubScreen(0)))
				{
					if ((obj = GetIconTags(path, ICONGETA_Screen, (IPTR)screen, TAG_DONE)))
					{
						struct Image *image;
						LONG IsPaletteMapped, IsNewIcon;

						IsPaletteMapped = IsNewIcon = FALSE;

						IconControl(obj,
									ICONCTRLA_IsPaletteMapped,
									(IPTR)&IsPaletteMapped,
									ICONCTRLA_IsNewIcon,
									(IPTR)&IsNewIcon,
									TAG_DONE);

						if (!IsPaletteMapped && !IsNewIcon)
						{
							UnlockPubScreen(0, screen);
							goto quinella_jump;
						}

						image = obj->do_Gadget.GadgetRender;

						// Allocate fake image data
						if ((data = new_image(image->Width, image->Height, image->Depth)))
						{
							// Store icon pointer
							data->new_diskobj = (struct NewDiskObject *)obj;

							// Set icon flags
							data->flags |= IMAGEF_ICON_IMAGE | IMAGEF_NEWICON;

							// Alternate image?
							if (obj->do_Gadget.SelectRender)
								data->flags |= IMAGEF_SEL_IMAGE;
						}
					}
					UnlockPubScreen(0, screen);
				}

			}  // end if V44
			else
			{
				// handle old style < OS 3.5 icons

				// Got NewIcons library?
				if (NewIconBase && (ndo = GetNewDiskObject((UBYTE *)path)))
				{
					// Got a newstyle icon?
					if (ndo->ndo_NormalImage)
					{
						// Allocate image data
						if ((data = new_image(ndo->ndo_NormalImage->Width, ndo->ndo_NormalImage->Height, 0)))
						{
							// Store icon pointer
							data->new_diskobj = ndo;

							// Set icon flags
							data->flags |= IMAGEF_ICON_IMAGE | IMAGEF_NEWICON;

							// Do we have a selected image?
							if (ndo->ndo_SelectedImage)
							{
								// Set flag for alternate imagery
								data->flags |= IMAGEF_SEL_IMAGE;
							}
						}
					}

					// Otherwise, fallback to old style
					else
					{
						// Get icon pointer
						obj = ndo->ndo_StdObject;
					}
				}

				// If no NewIcon, read standard icon
				if (!ndo)
					obj = GetDiskObject(path);

				// Got a standard icon?

			quinella_jump:

				if (obj)
				{
					struct Image *image;
					unsigned long planesize;

					// Get image pointer
					image = (struct Image *)obj->do_Gadget.GadgetRender;

					// Allocate image data
					if ((data = new_image(image->Width, image->Height, image->Depth)))
					{
						// Calculate plane size
						planesize = ((image->Width + 15) >> 4) * image->Height;

						// Small enough to store in chip ram?
						if (planesize * image->Depth <= 16384)
							data->flags |= IMAGEF_CHIP;

						// Copy image data
						if (!(copy_icon_image(image, data, 0)))
						{
							// Failed
							free_image(data);
							data = 0;
						}

						// Do we have alternate imagery?
						else if ((image = (struct Image *)obj->do_Gadget.SelectRender))
						{
							// Alternate image MUST be the same size
							if (image->Width == data->width && image->Height == data->height)
							{
								/* ??????????????????? */
								// Store alt image in chip if primary image is
								if (data->flags & IMAGEF_CHIP)
									data->flags |= IMAGEF_ALT_CHIP;

								// Copy image data
								if (copy_icon_image(image, data, 1))
								{
									// Set flag for alternate imagery
									data->flags |= IMAGEF_SEL_IMAGE;
								}
							}
						}

						// Set icon flag
						data->flags |= IMAGEF_ICON_IMAGE;
					}

					// Free icon
					if (ndo)
						FreeNewDiskObject(ndo);
					else
						FreeDiskObject(obj);
				}

			}  // end else handle old style < OS 3.5 icons

			// Free path
			FreeVec(path);
		}

		// Have we got an image?
		if (data)
			return data;
	}

	// From data?
	if (info)
		ilbm = L_FakeILBM(info->oi_ImageData,
						  info->oi_Palette,
						  info->oi_Width,
						  info->oi_Height,
						  info->oi_Depth,
						  ILBMF_GET_PLANES | ILBMF_ANIM_ONE_ONLY);

	// Try to read it as ILBM
	else
		ilbm = L_ReadILBM(name, ILBMF_GET_PLANES | ILBMF_ANIM_ONE_ONLY);

	// Got ILBM handle?
	if (ilbm)
	{
		// Did we get planes successfully?
		if (ilbm->flags & ILBMF_GOT_PLANES)
		{
			// Allocate new image data
			if ((data = new_image(ilbm->header.w, ilbm->header.h, ilbm->header.nPlanes)))
			{
				short plane;

				// Set flag to indicate ILBM, store ILBM pointer
				data->flags |= IMAGEF_ILBM;
				data->ilbm = ilbm;

				// Copy plane pointers over
				for (plane = 0; plane < ilbm->header.nPlanes; plane++)
					data->planes[0][plane] = ilbm->image.planes.planes[plane];

				// Are planes in chip memory?
				if (ilbm->flags & ILBMF_CHIP_PLANES)
				{
					// Set flag
					data->flags |= IMAGEF_CHIP;
				}

				// Do we have an animation frame?
				if (ilbm->flags & ILBMF_IS_ANIM_BRUSH && ilbm->anim->frame_count > 0)
				{
					long planesize;

					// Calculate plane size
					planesize = (((ilbm->header.w + 15) >> 4) << 1) * ilbm->header.h;

					// Start trying for chip?
					if (data->flags & IMAGEF_CHIP)
						data->flags |= IMAGEF_ALT_CHIP;

					// Try and allocate alternate image planes
					for (plane = 0; plane < ilbm->header.nPlanes; plane++)
					{
						// Allocate plane
						if (!(data->flags & IMAGEF_ALT_CHIP) ||
							!(data->planes[1][plane] = AllocVec(planesize, MEMF_CHIP)))
						{
							// Allocate normal memory
							data->planes[1][plane] = AllocVec(planesize, 0);
							data->flags &= ~IMAGEF_ALT_CHIP;
						}

						// Failed?
						if (!data->planes[1][plane])
							break;

						// Copy original image data over
						CopyMem((char *)data->planes[0][plane], (char *)data->planes[1][plane], planesize);
					}

					// Did we fail?
					if (plane < ilbm->header.nPlanes)
					{
						// Free what we got
						for (--plane; plane >= 0; plane--)
						{
							FreeVec(data->planes[1][plane]);
							data->planes[1][plane] = 0;
						}
					}

					// Otherwise
					else
					{
						ANIMFrame *frame = ilbm->anim->current;

						// Set flag for alternate image
						data->flags |= IMAGEF_SEL_IMAGE;

						// Decode type is RLE?
						if (frame->header.operation == 0)
						{
							struct BitMap bitmap;

							// Initialise fake bitmap
							InitBitMap(&bitmap, ilbm->header.nPlanes, ilbm->header.w, ilbm->header.h);

							// Set plane pointers
							for (plane = 0; plane < ilbm->header.nPlanes; plane++)
								bitmap.Planes[plane] = (PLANEPTR)data->planes[1][plane];

							// Decode RLE into second buffer
							L_DecodeILBM((char *)frame->delta,
										 ilbm->header.w,
										 ilbm->header.h,
										 ilbm->header.nPlanes,
										 &bitmap,
										 (ilbm->header.masking == 1) ? 1 : 0,
										 ilbm->header.compression);
						}

						// Or RIFF option 5
						else if (frame->header.operation == 5)
						{
							unsigned long *dptr;
							unsigned char *ddata;
							unsigned short sourcewidth;

							// Calculate source width in bytes
							sourcewidth = ((ilbm->header.w + 15) >> 4) << 1;

							// Get long pointer to deltas
							if ((dptr = (unsigned long *)frame->delta))
							{
								// Go through planes
								for (plane = 0; plane < ilbm->header.nPlanes; plane++)
								{
									// Valid delta?
									if (dptr[plane])
									{
										// Get pointer to plane delta
#ifdef __AROS__
										ddata = (unsigned char *)frame->delta + AROS_BE2LONG(dptr[plane]);
#else
										ddata = (unsigned char *)frame->delta + dptr[plane];
#endif

										// XOR?
										if (frame->header.bits & ANIMF_XOR)
										{
											L_AnimDecodeRIFFXor(
												ddata, (char *)data->planes[1][plane], sourcewidth, sourcewidth);
										}

										// Set
										else
										{
											L_AnimDecodeRIFFSet(
												ddata, (char *)data->planes[1][plane], sourcewidth, sourcewidth);
										}
									}
								}
							}
						}
					}
				}

				// Return image pointer
				return data;
			}
		}

		// Failed
		L_FreeILBM(ilbm);
	}

	return 0;
}

#ifdef __AROS__
static void aros_image_debug_log(char *text)
{
	BPTR file;
	char *paths[] = {"PROGDIR:DOpus5-startup.log", "T:DOpus5-startup.log", 0};
	short a;

	if (!text)
		return;

	for (a = 0; paths[a]; a++)
	{
		if ((file = Open(paths[a], MODE_READWRITE)) || (file = Open(paths[a], MODE_NEWFILE)))
		{
			Seek(file, 0, OFFSET_END);
			FPuts(file, text);
			Close(file);
		}
	}
}

static Image_Data *aros_try_image_path(char *prefix, char *leaf)
{
	char path[256];
	Image_Data *image;
	static short log_count = 0;
	BOOL do_log;

	if (!prefix || !leaf || strlen(prefix) + strlen(leaf) >= sizeof(path))
		return 0;

	strcpy(path, prefix);
	strcat(path, leaf);

	do_log = (log_count < 32);
	if (do_log)
	{
		BPTR lock;

		aros_image_debug_log("OpenImage fallback try ");
		aros_image_debug_log(path);

		if ((lock = Lock(path, ACCESS_READ)))
		{
			UnLock(lock);
			aros_image_debug_log(" lock ok");
		}
		else
			aros_image_debug_log(" lock failed");
	}

	image = read_image(path, 0);

	if (do_log)
	{
		aros_image_debug_log((image) ? " read ok\n" : " read failed\n");
		++log_count;
	}

	return image;
}

static Image_Data *aros_read_image_fallback(char *name, OpenImageInfo *info)
{
	char *leaf = 0;
	Image_Data *image;

	if (info || !name || !name[0])
		return 0;

	if (strnicmp(name, "DOpus5:Images/", 14) == 0)
		leaf = name + 14;
	else if (strnicmp(name, "PROGDIR:Images/", 15) == 0)
		leaf = name + 15;

	if (!leaf || !leaf[0])
		return 0;

	if ((image = aros_try_image_path("DOpus5:Images/", leaf)))
		return image;

	return aros_try_image_path("PROGDIR:Images/", leaf);
}
#endif

// Allocate a new image
Image_Data *new_image(short width, short height, short depth)
{
	Image_Data *image;

	// If there's no memory handle yet, create one
	if (!image_memory)
		image_memory = L_NewMemHandle(2048, 1024, MEMF_CLEAR);

	// Allocate image
	if ((image = L_AllocMemH(image_memory, sizeof(Image_Data))))
	{
		unsigned short **planes;

		// Fill out image
		image->width = width;
		image->height = height;
		image->depth = depth;

		// Initialise signal semaphore
		InitSemaphore(&image->lock);

		// Allocate plane pointers
		if ((planes = L_AllocMemH(image_memory, sizeof(unsigned short *) * 32)))
		{
			// Get plane pointers
			image->planes[0] = planes;
			image->planes[1] = planes + 8;
			image->remap_planes[0] = planes + 16;
			image->remap_planes[1] = planes + 24;

			// Return image pointer
			return image;
		}

		// Failed
		free_image(image);
	}

	return 0;
}

// Free an image from memory
void free_image(Image_Data *image)
{
	// Valid image?
	if (image)
	{
		// handle OS 3.5 icons
		if (IconBase->lib_Version >= 44 && image->new_diskobj)
		{
			// LayoutIcon(icon.NULL not needed since FreeDiskObject does this job

			// Free image path
			L_FreeMemH(image->node.ln_Name);

			FreeDiskObject((struct DiskObject *)image->new_diskobj);

			// Free image structure
			L_FreeMemH(image);
		}
		else
		{
			// handle old style < OS 3.5 icons
			short a, b;

			// Free image planes
			for (b = 0; b < 2; b++)
			{
				// Primary plane?
				if (image->planes[b][0])
				{
					for (a = 0; a < image->depth; a++)
					{
						// For ILBM only free what FreeILBM would leave behind
						if (b == 1 || !(image->flags & IMAGEF_ILBM))
							FreeVec(image->planes[b][a]);
					}
				}
			}

			// Remapped?
			if (image->remap_count)
			{
				struct Screen *screen = 0;
				ULONG lock = 0;

				// Got image remap screen?
				if (image->remap_info && (screen = image->remap_info->ir_Screen))
				{
					struct Screen *test;

					// Lock Intuition
					lock = LockIBase(0);

					// Check that screen still exists
					for (test = ((struct IntuitionBase *)IntuitionBase)->FirstScreen; test && test != screen;
						 test = test->NextScreen)
						;

					// Doesn't exist?
					if (!test)
					{
						// Can't free pens then
						image->remap_info = 0;
					}
				}

				// Set count to free remap
				image->remap_count = 1;

				// Free remap data
				L_FreeRemapImage(image, image->remap_info);

				// Unlock Intuition if it's locked
				if (screen)
					UnlockIBase(lock);
			}

			// Free plane pointers
			L_FreeMemH(image->planes[0]);

			// If image is an ILBM, free ilbm information
			if (image->flags & IMAGEF_ILBM)
				L_FreeILBM(image->ilbm);

			// Free image path
			L_FreeMemH(image->node.ln_Name);

			// Free NewIcon
			if (image->new_diskobj)
				FreeNewDiskObject(image->new_diskobj);

			// Free image structure
			L_FreeMemH(image);

		}  // end else  handle old style < OS 3.5 icons
	}
}

BOOL copy_icon_image(struct Image *image, Image_Data *data, short state)
{
	short plane;
	long planesize;
	BOOL in_chip = 0;

	// Calculate plane size
	planesize = ((image->Width + 15) >> 4) * image->Height;

	// Image in chip memory?
	if (!state && data->flags & IMAGEF_CHIP)
		in_chip = 1;
	else if (state && data->flags & IMAGEF_ALT_CHIP)
		in_chip = 1;

	// Go through planes
	for (plane = 0; plane < image->Depth; plane++)
	{
		unsigned short *ptr;

		// Get plane pointer
		ptr = image->ImageData + planesize * plane;

		// Allocate this plane
		if (!in_chip || !(data->planes[state][plane] = AllocVec(planesize << 1, MEMF_CHIP)))
		{
			data->planes[state][plane] = AllocVec(planesize << 1, 0);
			in_chip = 0;
		}

		// Successful?
		if (data->planes[state][plane])
		{
			CopyMem((char *)ptr, (char *)data->planes[state][plane], planesize << 1);
		}
		else
			break;
	}

	// Fix chip flag
	if (!state && !in_chip)
		data->flags &= ~IMAGEF_CHIP;
	else if (state && !in_chip)
		data->flags &= ~IMAGEF_ALT_CHIP;

	// Success?
	if (plane == image->Depth)
		return TRUE;

	// Free what we got
	for (--plane; plane >= 0; plane--)
	{
		FreeVec(data->planes[state][plane]);
		data->planes[state][plane] = 0;
	}

	return FALSE;
}

// Get image palette
ULONG *LIBFUNC L_GetImagePalette(REG(a0, APTR ptr))
{
	Image_Data *image;

	// Get image pointer
	if ((image = (Image_Data *)ptr))
	{
		// ILBM?
		if (image->ilbm)
			return image->ilbm->palette;
	}

	return 0;
}

// Add a pen to pen array
BOOL add_remap_pen(struct Screen *screen, ImageRemap *remap, unsigned short pen)
{
	short num;

	// Don't have pen array?
	if (!remap->ir_PenArray)
	{
		short depth;

		// The remap code stores planar pen values and only supports up to
		// 8 destination planes. True-colour screens can report 24/32-bit
		// depths, so cap the bookkeeping array to the same range.
		depth = screen->RastPort.BitMap->Depth;
		if (depth > 8)
			depth = 8;

		// Allocate two UWORDs for every possible remapped pen.
		if (!(remap->ir_PenArray = AllocVec((sizeof(UWORD) * 2) << depth, MEMF_CLEAR)))
			return 0;
	}

	// Go through array, see if pen is already allocated
	for (num = 0; num < remap->ir_PenCount; num++)
	{
		// Match our pen?
		if (remap->ir_PenArray[(num << 1)] == pen)
		{
			// Increment count
			++remap->ir_PenArray[(num << 1) + 1];

			// Release this copy of the pen
			ReleasePen(screen->ViewPort.ColorMap, pen);
			return 1;
		}
	}

	// Add a new entry
	remap->ir_PenArray[(num << 1)] = pen;
	remap->ir_PenArray[(num << 1) + 1] = 1;

	// Increment pen count
	++remap->ir_PenCount;
	return 1;
}

// Free pens allocated by image remapping
void LIBFUNC L_FreeImageRemap(REG(a0, ImageRemap *remap))
{
	// Valid pointer?
	if (remap)
	{
		// Pen array?
		if (remap->ir_PenArray)
		{
			short pen;

			// Free pens
			for (pen = 0; pen < remap->ir_PenCount; pen++)
			{
				// Release pen
				ReleasePen(remap->ir_Screen->ViewPort.ColorMap, remap->ir_PenArray[(pen << 1)]);
			}

			// Free pen array
			FreeVec(remap->ir_PenArray);
			remap->ir_PenArray = 0;
		}

		// Clear pen count
		remap->ir_PenCount = 0;
	}
}

// Remap an image to a screen
BOOL LIBFUNC L_RemapImage(REG(a0, Image_Data *image), REG(a1, struct Screen *screen), REG(a2, ImageRemap *remap))
{
	struct BitMap source, dest, *readbm, *writebm = 0;
	struct RastPort srp, drp, readrp, writerp;
	short plane, loop, colours, start;
	long wordcount;
	register short *pen_lookup;
	ULONG *palette;
	UBYTE *buffer = 0;
	long precision = PRECISION_IMAGE;

	// Only works under OS39
	if (!image || ((struct Library *)GfxBase)->lib_Version < 39)
		return 0;

	// Is image a new icon?
	if (image->new_diskobj)
	{
		// Get image lock
		L_GetSemaphore(&image->lock, SEMF_EXCLUSIVE, 0);

		// Already remapped?
		if (image->remap_count > 0)
		{
			// Increment count and return
			++image->remap_count;

			// Free semaphore
			L_FreeSemaphore(&image->lock);
			return 1;
		}

		// handle OS 3.5 icons
		if (IconBase->lib_Version >= 44)
		{
			struct Image *im;

			LayoutIcon((struct DiskObject *)image->new_diskobj, screen, TAG_DONE);

			if ((im = ((struct DiskObject *)image->new_diskobj)->do_Gadget.GadgetRender))
			{
				image->width = im->Width;
				image->height = im->Height;
				image->depth = im->Depth;
				image->remap_image[0] = im;
				image->remap_depth = im->Depth;

				if ((image->remap_image[1] = ((struct DiskObject *)image->new_diskobj)->do_Gadget.SelectRender))
					image->flags |= IMAGEF_SEL_IMAGE;
			}
		}
		else
		{
			// handle old style < OS 3.5 icons

			// Remap normal image if we have it
			if (image->new_diskobj->ndo_NormalImage)
			{
				if ((image->remap_image[0] = RemapChunkyImage(image->new_diskobj->ndo_NormalImage, screen)))
					image->remap_depth = image->remap_image[0]->Depth;
			}

			// Remap selected image if we have it
			if (image->new_diskobj->ndo_SelectedImage)
			{
				if ((image->remap_image[1] = RemapChunkyImage(image->new_diskobj->ndo_SelectedImage, screen)))
				{
					if (image->remap_image[1]->Depth > image->remap_depth)
						image->remap_depth = image->remap_image[1]->Depth;
				}
			}
		}

		// Save screen pointer
		remap->ir_Screen = screen;

		// Increment remap count
		++image->remap_count;

		// Save remap information pointer
		image->remap_info = remap;

		// Unlock image
		L_FreeSemaphore(&image->lock);
		return 1;
	}

	// Must have valid palette
	if (!(palette = L_GetImagePalette(image)))
		return 0;

	// Not allowed to remap?
	if (image->node.ln_Name && (loop = strlen(image->node.ln_Name)) > 8 &&
		stricmp(image->node.ln_Name + loop - 8, ".noremap") == 0)
		return 0;

	// Get precision
	if (remap->ir_Flags & IRF_PRECISION_EXACT)
		precision = PRECISION_EXACT;
	else if (remap->ir_Flags & IRF_PRECISION_ICON)
		precision = PRECISION_ICON;
	else if (remap->ir_Flags & IRF_PRECISION_GUI)
		precision = PRECISION_GUI;

	// Allocate lookup table
	if (!(pen_lookup = AllocVec(2 << image->depth, MEMF_CLEAR)))
		return 0;

	// Get image lock
	L_GetSemaphore(&image->lock, SEMF_EXCLUSIVE, 0);

	// Already remapped?
	if (image->remap_count > 0)
	{
		// Increment count and return
		++image->remap_count;

		// Free semaphore and table
		L_FreeSemaphore(&image->lock);
		FreeVec(pen_lookup);
		return 1;
	}

	// Remapping colour 0?
	start = (remap->ir_Flags & IRF_REMAP_COL0) ? 0 : 1;

	// Initialise table
	colours = 1 << image->depth;
	for (plane = start; plane < colours; plane++)
		pen_lookup[plane] = -1;

	// Go through pens
	for (plane = start; plane < colours; plane++)
	{
		// See if pen colour is different to screen colour
		if (FindColor(screen->ViewPort.ColorMap,
					  palette[1 + (plane * 3)],
					  palette[2 + (plane * 3)],
					  palette[3 + (plane * 3)],
					  colours - 1) != plane)
			break;
	}

	// If pens are the same, we don't need to remap
	if (plane == colours)
	{
		// Unlock image
		L_FreeSemaphore(&image->lock);
		return 1;
	}

	// Save screen pointer
	remap->ir_Screen = screen;

	// Initialise bitmaps
	InitBitMap(&source, image->depth, image->width, image->height);
	InitBitMap(&dest, screen->RastPort.BitMap->Depth, image->width, image->height);
	if (dest.Depth > 8)
		dest.Depth = 8;

	// Temporary bitmaps
	if ((readbm = AllocBitMap(image->width, 1, image->depth, 0, 0)) &&
		(writebm = AllocBitMap(image->width, 1, dest.Depth, 0, 0)))
	{
		// Allocate buffer
		buffer = AllocVec(((image->width + 15) >> 4) << 4, 0);

		// Initialise rastports
		InitRastPort(&readrp);
		InitRastPort(&writerp);
		readrp.BitMap = readbm;
		writerp.BitMap = writebm;
	}

	// Get words per plane
	wordcount = (((image->width + 15) >> 4) * image->height) << 1;

	// Initialise rastports
	InitRastPort(&srp);
	srp.BitMap = &source;
	InitRastPort(&drp);
	drp.BitMap = &dest;

	// Remap two images
	for (loop = 0; loop < 2; loop++)
	{
		register short x, y;
		ULONG mask = 0;

		// Valid image?
		if (!image->planes[loop][0])
			continue;

		// Get source bitmap pointers
		for (plane = 0; plane < image->depth; plane++)
			source.Planes[plane] = (PLANEPTR)image->planes[loop][plane];

		// Allocate destination bitmap pointers
		for (plane = 0; plane < dest.Depth; plane++)
		{
			// Allocate plane
			if ((dest.Planes[plane] = AllocVec(wordcount, MEMF_CHIP | MEMF_CLEAR)))
			{
				// Save pointer
				image->remap_planes[loop][plane] = (unsigned short *)dest.Planes[plane];
			}
		}

		// Failed?
		if (plane < dest.Depth)
		{
			// Free what we got
			for (; plane >= 0; plane--)
				FreeVec(dest.Planes[plane]);
			continue;
		}

		// Go through rows
		for (y = 0; y < image->height; y++)
		{
			// Got buffer?
			if (buffer)
			{
				// Read pixel line
				ReadPixelLine8(&srp, 0, y, image->width, buffer, &readrp);
			}

			// Go through columns
			for (x = 0; x < image->width; x++)
			{
				register short pen;

				// Get pen value
				pen = (buffer) ? buffer[x] : ReadPixel(&srp, x, y);

				// Has this pen not been mapped yet?
				if (pen_lookup[pen] == -1)
				{
					long alloc;

					// Try for pen
					if ((alloc = ObtainBestPen(screen->ViewPort.ColorMap,
											   palette[1 + (pen * 3)],
											   palette[2 + (pen * 3)],
											   palette[3 + (pen * 3)],
											   OBP_Precision,
											   precision,
											   TAG_END)) != -1)
					{
						// Save pen
						pen_lookup[pen] = alloc;

						// Add to pen array
						add_remap_pen(screen, remap, alloc);
					}

					// Couldn't allocate one, just grab nearest
					else
						pen_lookup[pen] = FindColor(screen->ViewPort.ColorMap,
													palette[1 + (pen * 3)],
													palette[2 + (pen * 3)],
													palette[3 + (pen * 3)],
													-1);

					// Add pen to mask of planes
					mask |= pen_lookup[pen];
				}

				// If we have a buffer, set value in buffer
				if (buffer)
					buffer[x] = pen_lookup[pen];

				// Otherwise
				else
				{
					// Write pen value
					SetAPen(&drp, pen_lookup[pen]);
					WritePixel(&drp, x, y);
				}
			}

			// Got buffer?
			if (buffer)
			{
				// Write data
				WritePixelLine8(&drp, 0, y, image->width, buffer, &writerp);
			}
		}

		// Free the planes we don't need
		for (plane = 0; plane < dest.Depth; plane++)
		{
			if (!(mask & (1 << plane)))
			{
				FreeVec(dest.Planes[plane]);
				image->remap_planes[loop][plane] = 0;
			}
		}

		// No alternate image?
		if (!(image->flags & IMAGEF_SEL_IMAGE))
			break;

		// Reinitialise pen table
		if (loop == 0)
			for (plane = start; plane < colours; plane++)
				pen_lookup[plane] = -1;
	}

	// Free pen lookup
	FreeVec(pen_lookup);

	// Store depth
	image->remap_depth = dest.Depth;

	// Increment remap count
	++image->remap_count;

	// Save remap information pointer
	image->remap_info = remap;

	// Free buffer and temporary bitmaps
	FreeVec(buffer);
	FreeBitMap(readbm);
	FreeBitMap(writebm);

	// Unlock image
	L_FreeSemaphore(&image->lock);
	return 1;
}

// Free remapped image data
void LIBFUNC L_FreeRemapImage(REG(a0, Image_Data *image), REG(a1, ImageRemap *remap))
{
	short plane, loop;

	// Got remapped images?
	if (!image || !image->remap_count)
		return;

	// Get image lock
	L_GetSemaphore(&image->lock, SEMF_EXCLUSIVE, 0);

	if (IconBase->lib_Version >= 44 && image->new_diskobj)
	{
		// if 3.5 and is an icon then don't do this since plane data
		//  is private to new V44 diskobject stuff
		// just keep nesting count in sync

		if ((--image->remap_count) == 0)
		{  // Store depth ? Needed?
			image->remap_depth = 0;
		}
	}
	else
	{
		// Decrement remap count
		if ((--image->remap_count) == 0)
		{
			// Go through images
			for (loop = 0; loop < 2; loop++)
			{
				// Go through planes
				for (plane = 0; plane < 8; plane++)
				{
					// Free plane
					FreeVec(image->remap_planes[loop][plane]);
					image->remap_planes[loop][plane] = 0;
				}

				// No alternate image?
				if (!(image->flags & IMAGEF_SEL_IMAGE))
					break;
			}

			// Free Images
			for (loop = 0; loop < 2; loop++)
			{
				// Got remapped image?
				if (image->remap_image[loop])
				{
					// Free it
					FreeRemappedImage(image->remap_image[loop], (remap) ? remap->ir_Screen : 0);
					image->remap_image[loop] = 0;
				}
			}

			// Store depth
			image->remap_depth = 0;
		}
	}
	// Unlock image
	L_FreeSemaphore(&image->lock);
}
