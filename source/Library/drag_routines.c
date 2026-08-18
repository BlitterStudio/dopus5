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

#ifdef __AROS__
	#include <proto/commodities.h>
#endif

#if defined(__amigaos4__) && !defined(ABNC)
	#define ABC MINTERM_ABC
	#define ABNC MINTERM_ABNC
	#define ANBC MINTERM_ANBC
#endif

void lock_layers(DragInfo *drag, BOOL lock);

// Allocate and initialise a DragInfo structure
DragInfo *LIBFUNC L_GetDragInfo(REG(a0, struct Window *window),
								REG(a1, struct RastPort *rast),
								REG(d0, long width),
								REG(d1, long height),
								REG(d2, long flags),
								REG(a6, struct MyLibrary *libbase))
{
	DragInfo *drag;
	int plane;
	BOOL real = 1;
	short word_width;

#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Not a real drag?
	if (width < 0 || height < 0)
	{
		real = 0;
		if (width < 0)
			width = -width;
		if (height < 0)
			height = -height;
	}

	// Allocate structure
	if (!(drag = AllocVec(sizeof(DragInfo) + ((flags & DRAGF_NEED_GELS) ? sizeof(DragInfoExtra) : 0), MEMF_CLEAR)))
		return 0;

	// Store stuff
	if (real)
	{
		if (rast)
			drag->rastport = rast;
		else
			drag->rastport = &window->WScreen->RastPort;
		drag->viewport = &window->WScreen->ViewPort;
		drag->window = window;
	}
	drag->width = width;
	drag->height = height;

	// Get width in words
	word_width = (drag->width + 15) >> 4;

	// Allowed custom dragging?
	if (flags & DRAGF_CUSTOM)
	{
		// Check we can do it
		if (window && L_DragCustomOk(window->WScreen->RastPort.BitMap, libbase))
		{
			// Set flag
			drag->flags |= DRAGF_CUSTOM;

			// No mask?
			if (flags & DRAGF_NO_MASK)
				drag->flags |= DRAGF_NO_MASK;
		}
	}

	// Initialise sprite structure
	drag->sprite.Flags = SAVEBACK | OVERLAY;
	drag->sprite.Width = (drag->flags & DRAGF_CUSTOM) ? drag->width : word_width;
	drag->sprite.Height = drag->height;
	drag->sprite.Depth = window->WScreen->RastPort.BitMap->Depth;
	drag->sprite.PlanePick = (1 << drag->sprite.Depth) - 1;

	// Custom rendering?
	if (drag->flags & DRAGF_CUSTOM)
	{
		// Use AllocBitMap (not p96AllocBitMap) here on purpose. The drag
		// sprite is composited via BltMaskBitMapRastPort + a 1-bit shadow
		// mask, and on this target the RTG-native p96 bitmap layout breaks
		// that legacy mask-blit path - the sprite becomes invisible during
		// motion and only shows when the mouse stops. graphics.library's
		// AllocBitMap(..., BMF_MINPLANES, friend) yields a layout the mask
		// blit still renders correctly on OS3 + P96.
		if (!(drag->sprite.ImageData = (WORD *)AllocBitMap(
				  drag->width, drag->height, drag->sprite.Depth, BMF_MINPLANES, window->WScreen->RastPort.BitMap)))
		{
			FreeVec(drag);
			return 0;
		}

		// Allocate backup buffer (same layout; save/restore paired with the
		// mask blit above).
		if (real &&
			!(drag->bob.SaveBuffer = (WORD *)AllocBitMap(
				  drag->width, drag->height, drag->sprite.Depth, BMF_MINPLANES, window->WScreen->RastPort.BitMap)))
		{
			L_FreeDragInfo(drag);
			return 0;
		}

		// Initialise dummy rastport
		InitRastPort(&drag->drag_rp);
		drag->drag_rp.BitMap = (struct BitMap *)drag->sprite.ImageData;

		// Clear image bitmap
		SetRast(&drag->drag_rp, 0);
	}

	// Normal BOBs
	else
	{
		// Allocate image buffer
		if (!(drag->sprite.ImageData = (WORD *)AllocVec(RASSIZE((word_width << 4), drag->height) * drag->sprite.Depth,
														MEMF_CHIP | MEMF_CLEAR)))
		{
			FreeVec(drag);
			return 0;
		}

		// Allocate backup buffer
		if (real && !(drag->bob.SaveBuffer =
						  (WORD *)AllocVec(RASSIZE((word_width << 4), drag->height) * drag->sprite.Depth, MEMF_CHIP)))
		{
			L_FreeDragInfo(drag);
			return 0;
		}

		// Initialise dummy bitmap
		InitBitMap(&drag->drag_bm, drag->sprite.Depth, word_width << 4, drag->height);

		// Get bitmap planes
		for (plane = 0; plane < drag->sprite.Depth; plane++)
			drag->drag_bm.Planes[plane] = (PLANEPTR)drag->sprite.ImageData + ((word_width * drag->height * plane) << 1);
		for (; plane < 8; plane++)
			drag->drag_bm.Planes[plane] = 0;

		// Initialise dummy rastport
		InitRastPort(&drag->drag_rp);
		drag->drag_rp.BitMap = &drag->drag_bm;
	}

	// Allocate shadow mask
	if (!(drag->flags & DRAGF_NO_MASK) &&
		!(drag->bob.ImageShadow = (WORD *)AllocVec(RASSIZE(drag->width, drag->height), MEMF_CHIP | MEMF_CLEAR)))
	{
		L_FreeDragInfo(drag);
		return 0;
	}

	// Point sprite and bob at each other
	drag->sprite.VSBob = &drag->bob;
	drag->bob.BobVSprite = &drag->sprite;

	// Set font in rastport
	if (rast)
		SetFont(&drag->drag_rp, rast->Font);
	else if (window)
		SetFont(&drag->drag_rp, window->RPort->Font);

	// Initialise GEL list if necessary
	if ((flags & DRAGF_NEED_GELS) && !drag->rastport->GelsInfo)
	{
		DragInfoExtra *extra = (DragInfoExtra *)(drag + 1);
		InitGels(&extra->head, &extra->tail, &extra->info);
		drag->rastport->GelsInfo = &extra->info;
		drag->flags |= DRAGF_DONE_GELS;
	}

	// Return DragInfo
	return drag;
}

// Free a DragInfo structure
void LIBFUNC L_FreeDragInfo(REG(a0, DragInfo *drag))
{
	// Check valid
	if (!drag)
		return;

	// Hide bob
	L_HideDragImage(drag);

	// Used custom rendering?
	if (drag->flags & DRAGF_CUSTOM)
	{
		// Free bitmaps (must match AllocBitMap allocator above)
		FreeBitMap((struct BitMap *)drag->sprite.ImageData);
		FreeBitMap((struct BitMap *)drag->bob.SaveBuffer);
	}

	// Standard BOBs
	else
	{
		// Free image buffers
		FreeVec(drag->bob.SaveBuffer);
		FreeVec(drag->sprite.ImageData);
	}

	// Free image shadow
	FreeVec(drag->bob.ImageShadow);

	// Double-buffering info?
	if (drag->flags & DRAGF_DBUF && drag->dbuf)
	{
		// Free buffer
		FreeVec(drag->dbuf->BufBuffer);
		FreeVec(drag->dbuf);
	}

	// Remove GEL info if using ours
	if (drag->rastport && drag->flags & DRAGF_DONE_GELS)
		drag->rastport->GelsInfo = 0;

	// Free DragInfo
	FreeVec(drag);
}

// Hide bob
void LIBFUNC L_HideDragImage(REG(a0, DragInfo *drag))
{
	// Remove bob from list
	if (drag->flags & DRAGF_VALID)
	{
		L_RemDragImage(drag);
		if (!(drag->flags & DRAGF_NO_LOCK))
			lock_layers(drag, 0);
		if (!(drag->flags & DRAGF_NOWAIT))
			WaitTOF();
	}

	// Clear position
	drag->sprite.X = -1;
	drag->sprite.Y = -1;
}

// Get image data for bob
void LIBFUNC L_GetDragImage(REG(a0, DragInfo *drag), REG(d0, ULONG x), REG(d1, ULONG y))
{
	BOOL ok = 1;

	// Hide bob if visible
	L_HideDragImage(drag);

	// Under 3.0?
	if (((struct Library *)GfxBase)->lib_Version >= 39)
	{
		// See if the source bitmap is non-standard
		if (!(GetBitMapAttr(drag->window->WScreen->RastPort.BitMap, BMA_FLAGS) & BMF_STANDARD))
		{
			// If not custom rendering, a clip won't work for some reason
			if (!(drag->flags & DRAGF_CUSTOM))
			{
				// We'll just drag a dashed line around the area
				SetRast(&drag->drag_rp, 0);
				SetAPen(&drag->drag_rp, 1);
				SetDrPt(&drag->drag_rp, 0xaaaa);

				// Draw line
				Move(&drag->drag_rp, 0, 0);
				Draw(&drag->drag_rp, drag->width - 1, 0);
				Draw(&drag->drag_rp, drag->width - 1, drag->height - 1);
				Draw(&drag->drag_rp, 0, drag->height - 1);
				Draw(&drag->drag_rp, 0, 1);

				// Reset, and clear flag so image won't be copied
				SetDrPt(&drag->drag_rp, 0xffff);
				ok = 0;
			}
		}
	}

	//.Ok to copy image?
	if (ok)
	{
		// Get image
		ClipBlit(drag->window->RPort, x, y, &drag->drag_rp, 0, 0, drag->width, drag->height, 0xc0);
	}

	// Wait for blitter to be done
	WaitBlit();

	// Build shadow mask
	L_GetDragMask(drag);
}

// Build a shadow mask
void LIBFUNC L_GetDragMask(REG(a0, DragInfo *drag))
{
	short row, col, plane, word, off = 0;
	unsigned short remainder;

	// No shadow?
	if (!drag->bob.ImageShadow)
		return;

	// Custom rendering
	if (drag->flags & DRAGF_CUSTOM && (!(drag->flags & DRAGF_OPAQUE) || drag->flags & DRAGF_TRANSPARENT))
	{
		short depth;
		struct BitMap *imagebm, *tempbm;
		UWORD *image;
		BOOL ok = 0;

		// Get image depth
		imagebm = (struct BitMap *)drag->sprite.ImageData;
		depth = GetBitMapAttr(imagebm, BMA_DEPTH);

		// Do we have Picasso96 and an RTG bitmap with >256 colours?
		// AROS has no Picasso96API.library; compile the P96 branch out
		// there so the p96* symbols never reach the AROS linker. The
		// CGX branch below covers AROS (and any other CGX-only install).
		//
		// Sequential fallback: if P96 is eligible but its pixel read
		// fails (lock failure, P96BMA_MEMORY NULL), leave `ok` at 0 so
		// the next capability check (CGX) can still attempt the capture
		// before we give up and fill the mask with 0xffff.
#if !defined(__AROS__)
		if (!ok && P96Base && depth > 8 && p96GetBitMapAttr(imagebm, P96BMA_ISP96))
		{
			UBYTE *image_array;

			// Use drag->width / drag->height (NOT sprite.Width/Height) for
			// every mask-build dimension below. ImageShadow itself was
			// allocated as RASSIZE(drag->width, drag->height) (see
			// L_GetDragInfo); for DRAGF_CUSTOM (the only path that reaches
			// the RTG branches) sprite.Width/Height happen to equal
			// drag->width/height, but normalising on the alloc-side pair
			// removes the lurking allocation/write inconsistency and bounds
			// every ImageShadow word write by the same dimension used to
			// size it.

			// Allocate image array
			if ((image_array = AllocVec(drag->width * drag->height * 3, MEMF_CLEAR)))
			{
				short mask[2] = {0xff, 0xff}, bit_pos, x_count;
				long pixfmt, count, num, array_pos, word_pos;
				ULONG colour0[3];
				UWORD word_data;
				LONG lock;
				BOOL got_pixels = FALSE;

				// Get the palette value of colour 0
				GetRGB32(drag->viewport->ColorMap, 0, 1, colour0);

				// Calculate mask for pixel format
				pixfmt = p96GetBitMapAttr(imagebm, P96BMA_RGBFORMAT);
				if (pixfmt == RGBFB_R5G5B5 || pixfmt == RGBFB_R5G5B5PC || pixfmt == RGBFB_B5G5R5PC)
				{
					mask[0] = 0xf8;
					mask[1] = 0xf8;
				}
				else if (pixfmt == RGBFB_R5G6B5 || pixfmt == RGBFB_R5G6B5PC || pixfmt == RGBFB_B5G6R5PC)
				{
					mask[0] = 0xf8;
					mask[1] = 0xfc;
				}

				// Convert to 8 bit, masked colour
				colour0[0] &= mask[0];
				colour0[1] &= mask[1];
				colour0[2] &= mask[0];

				// Read the image via direct bitmap memory access. p96ReadPixelArray
				// has proven unreliable on layerless rastports on some OS3 P96
				// installations, so we do the format conversion ourselves.
				//
				// Pass a struct RenderInfo to p96LockBitMap and read Memory /
				// BytesPerRow from that buffer. The P96 autodoc forbids calling
				// any P96 API (including p96GetBitMapAttr) on the same bitmap
				// while a lock is held - touched bitmaps are locked internally,
				// so a re-entrant query risks dead-lock or blocking the screen
				// switcher mid-drag.
				{
					struct RenderInfo ri;
					ri.Memory = NULL;
					ri.BytesPerRow = 0;
					ri.pad = 0;
					ri.RGBFormat = RGBFB_NONE;

					if ((lock = p96LockBitMap(imagebm, (UBYTE *)&ri, sizeof(ri))))
					{
						UBYTE *srcmem = (UBYTE *)ri.Memory;
						LONG srcbpr = ri.BytesPerRow;
						LONG yy, xx;
						UWORD w = drag->width;
						UWORD h = drag->height;
						BOOL unknown_fmt = FALSE;
						LONG bpp = 0;

						// Bytes-per-pixel for the selected RGB format.
						// We need this up front to validate that the P96
						// driver's reported row stride is large enough to
						// cover width*bpp, before the per-row walk reads
						// past the row. If the format is unknown or the
						// stride is too tight, skip straight to the
						// outer CGX/tempbm fallback via unknown_fmt.
						switch (pixfmt)
						{
							case RGBFB_R8G8B8:
							case RGBFB_B8G8R8:
								bpp = 3; break;
							case RGBFB_A8R8G8B8:
							case RGBFB_R8G8B8A8:
							case RGBFB_A8B8G8R8:
							case RGBFB_B8G8R8A8:
								bpp = 4; break;
							case RGBFB_R5G6B5:
							case RGBFB_R5G6B5PC:
							case RGBFB_B5G6R5PC:
							case RGBFB_R5G5B5:
							case RGBFB_R5G5B5PC:
							case RGBFB_B5G5R5PC:
								bpp = 2; break;
							default:
								unknown_fmt = TRUE;
								break;
						}

					if (!unknown_fmt && srcmem && srcbpr > 0 && srcbpr >= (LONG)w * bpp)
					{
						for (yy = 0; yy < h; yy++)
						{
							UBYTE *sr = srcmem + yy * srcbpr;
							UBYTE *dr = image_array + yy * w * 3;

							switch (pixfmt)
							{
								// Byte order for the 24/32-bit cases follows the Picasso96
								// ABI as documented in <libraries/Picasso96.h>:
								//   RGBFB_R8G8B8   = "TrueColor RGB (8 bit each)"
								//   RGBFB_B8G8R8   = "TrueColor BGR (8 bit each)"
								//   RGBFB_A8R8G8B8 = "4 Byte TrueColor ARGB"
								//   RGBFB_R8G8B8A8 = "4 Byte TrueColor RGBA"
								//   RGBFB_A8B8G8R8 = "4 Byte TrueColor ABGR"
								//   RGBFB_B8G8R8A8 = "4 Byte TrueColor BGRA"
								// Output buffer (image_array) is canonical R,G,B
								// triples - what GetRGB32 + the colour0 compare expect.
								case RGBFB_R8G8B8:
									for (xx = 0; xx < w; xx++)
									{
										dr[0] = sr[0]; dr[1] = sr[1]; dr[2] = sr[2];
										sr += 3; dr += 3;
									}
									break;

								case RGBFB_B8G8R8:
									for (xx = 0; xx < w; xx++)
									{
										dr[0] = sr[2]; dr[1] = sr[1]; dr[2] = sr[0];
										sr += 3; dr += 3;
									}
									break;

								case RGBFB_A8R8G8B8:
									for (xx = 0; xx < w; xx++)
									{
										dr[0] = sr[1]; dr[1] = sr[2]; dr[2] = sr[3];
										sr += 4; dr += 3;
									}
									break;

								case RGBFB_R8G8B8A8:
									for (xx = 0; xx < w; xx++)
									{
										dr[0] = sr[0]; dr[1] = sr[1]; dr[2] = sr[2];
										sr += 4; dr += 3;
									}
									break;

								case RGBFB_A8B8G8R8:
									for (xx = 0; xx < w; xx++)
									{
										dr[0] = sr[3]; dr[1] = sr[2]; dr[2] = sr[1];
										sr += 4; dr += 3;
									}
									break;

								case RGBFB_B8G8R8A8:
									for (xx = 0; xx < w; xx++)
									{
										dr[0] = sr[2]; dr[1] = sr[1]; dr[2] = sr[0];
										sr += 4; dr += 3;
									}
									break;

								// Hicolor cases: store each channel with only the top
								// 5 (or 6) bits and the low bits zero, so the byte we
								// stored matches the masked colour0 below
								// (colour0[n] &= 0xf8 / 0xfc). Bit-replicating the
								// low bits here would expand e.g. 5-bit 16 to 132
								// while the pen side stays 128 after & 0xf8, and the
								// compare would wrongly flag transparent pixels as
								// foreground on non-black backgrounds.
								case RGBFB_R5G6B5:
									for (xx = 0; xx < w; xx++)
									{
										UWORD pix = (sr[0] << 8) | sr[1];
										UBYTE rr = (pix >> 11) & 0x1f;
										UBYTE gg = (pix >> 5) & 0x3f;
										UBYTE bb = pix & 0x1f;
										dr[0] = rr << 3;
										dr[1] = gg << 2;
										dr[2] = bb << 3;
										sr += 2; dr += 3;
									}
									break;

								case RGBFB_R5G6B5PC:
									for (xx = 0; xx < w; xx++)
									{
										UWORD pix = sr[0] | (sr[1] << 8);
										UBYTE rr = (pix >> 11) & 0x1f;
										UBYTE gg = (pix >> 5) & 0x3f;
										UBYTE bb = pix & 0x1f;
										dr[0] = rr << 3;
										dr[1] = gg << 2;
										dr[2] = bb << 3;
										sr += 2; dr += 3;
									}
									break;

								case RGBFB_B5G6R5PC:
									for (xx = 0; xx < w; xx++)
									{
										UWORD pix = sr[0] | (sr[1] << 8);
										UBYTE bb = (pix >> 11) & 0x1f;
										UBYTE gg = (pix >> 5) & 0x3f;
										UBYTE rr = pix & 0x1f;
										dr[0] = rr << 3;
										dr[1] = gg << 2;
										dr[2] = bb << 3;
										sr += 2; dr += 3;
									}
									break;

								case RGBFB_R5G5B5:
									for (xx = 0; xx < w; xx++)
									{
										UWORD pix = (sr[0] << 8) | sr[1];
										UBYTE rr = (pix >> 10) & 0x1f;
										UBYTE gg = (pix >> 5) & 0x1f;
										UBYTE bb = pix & 0x1f;
										dr[0] = rr << 3;
										dr[1] = gg << 3;
										dr[2] = bb << 3;
										sr += 2; dr += 3;
									}
									break;

								case RGBFB_R5G5B5PC:
									for (xx = 0; xx < w; xx++)
									{
										UWORD pix = sr[0] | (sr[1] << 8);
										UBYTE rr = (pix >> 10) & 0x1f;
										UBYTE gg = (pix >> 5) & 0x1f;
										UBYTE bb = pix & 0x1f;
										dr[0] = rr << 3;
										dr[1] = gg << 3;
										dr[2] = bb << 3;
										sr += 2; dr += 3;
									}
									break;

								case RGBFB_B5G5R5PC:
									for (xx = 0; xx < w; xx++)
									{
										UWORD pix = sr[0] | (sr[1] << 8);
										UBYTE bb = (pix >> 10) & 0x1f;
										UBYTE gg = (pix >> 5) & 0x1f;
										UBYTE rr = pix & 0x1f;
										dr[0] = rr << 3;
										dr[1] = gg << 3;
										dr[2] = bb << 3;
										sr += 2; dr += 3;
									}
									break;

								default:
									// Unknown format - do NOT call back into
									// the P96 API while the bitmap is locked
									// (P96 autodoc: touched bitmaps are
									// locked internally; re-entering risks
									// dead-locking or blocked screen
									// switches). Exit the loop without
									// setting got_pixels so ok stays 0 and
									// the outer sequential fallback chain
									// tries CGX / tempbm / opaque silhouette
									// next.
									yy = h;
									unknown_fmt = TRUE;
									break;
							}

							if (unknown_fmt)
								break;
						}

						// Only trust the capture when every row went through
						// a known-format branch. p96ReadPixelArray is void
						// return so even a "successful" call can't be
						// verified, and a zero-filled image_array would
						// build a mask that treats the whole sprite as
						// either fully transparent or fully opaque depending
						// on colour0.
						if (!unknown_fmt)
							got_pixels = TRUE;
					}

						p96UnlockBitMap(imagebm, lock);
					}
				}

				// Only build the mask if we actually captured pixel data.
				// Otherwise image_array is still zero-fill from MEMF_CLEAR
				// and the colour0 compare below would produce a bogus mask
				// (invisible or unmasked drag depending on colour0).
				if (got_pixels)
				{
					// Get number of pixels (drag->width/height: matches the
					// image_array allocation and the ImageShadow sizing -
					// see comment at the start of this RTG branch)
					count = drag->width * drag->height;

					// Go through image array
					for (num = 0, array_pos = 0, bit_pos = 15, word_data = 0, word_pos = 0, x_count = 0; num < count;
						 num++, array_pos += 3, --bit_pos)
					{
						// See if colour is not the background colour
						if (image_array[array_pos + 0] != colour0[0] || image_array[array_pos + 1] != colour0[1] ||
							image_array[array_pos + 2] != colour0[2])
						{
							// Set bit in word data
							word_data |= 1 << bit_pos;
						}

						// Reached the end of a word?
						if ((++x_count) == drag->width)
						{
							// Reset count
							x_count = 0;
							bit_pos = 0;
						}

						// Reached end of a mask word?
						if (bit_pos == 0)
						{
							// Set word in mask
#ifdef __AROS__
							((UWORD *)drag->bob.ImageShadow)[word_pos++] = AROS_BE2WORD(word_data);
#else
							((UWORD *)drag->bob.ImageShadow)[word_pos++] = word_data;
#endif

							// Reset word
							word_data = 0;
							bit_pos = 16;
						}
					}

					// Get mask size
					count = RASSIZE((((drag->width + 15) >> 4) << 4), drag->height) >> 1;

					// Fill rest of the mask
					while (word_pos < count)
					{
						((UWORD *)drag->bob.ImageShadow)[word_pos++] = word_data;
						word_data = 0;
					}

					// Set ok flag
					ok = 1;
				}

				// Free image array (released either way - mask built or not)
				if (image_array)
					FreeVec(image_array);
			}
		}
#endif /* !__AROS__ */

		// CyberGraphX fallback. Runs when either the P96 branch was not
		// eligible (no P96Base, or bitmap is not a P96 surface) or the
		// P96 pixel read failed (ok still 0). On AROS this is the only
		// RTG branch.
		if (!ok && CyberGfxBase && depth > 8 && GetCyberMapAttr(imagebm, CYBRMATTR_ISCYBERGFX))
		{
			UBYTE *image_array;

			// Same dimension-normalisation rule as the P96 branch above:
			// use drag->width / drag->height for everything (image_array
			// alloc, CGX read, iteration, mask sizing) so every write into
			// ImageShadow stays bounded by its RASSIZE(drag->width,
			// drag->height) allocation.

			// Allocate image array
			if ((image_array = AllocVec(drag->width * drag->height * 3, MEMF_CLEAR)))
			{
				short mask[2] = {0xff, 0xff}, bit_pos, x_count;
				long pixfmt, count, num, array_pos, word_pos;
				ULONG colour0[3];
				UWORD word_data;

				// Get the palette value of colour 0
				GetRGB32(drag->viewport->ColorMap, 0, 1, colour0);

				// Calculate mask for pixel format
				pixfmt = GetCyberMapAttr(imagebm, CYBRMATTR_PIXFMT);
				if (pixfmt >= PIXFMT_RGB15 && pixfmt <= PIXFMT_BGR15PC)
				{
					mask[0] = 0xf8;
					mask[1] = 0xf8;
				}
				else if (pixfmt >= PIXFMT_RGB16 && pixfmt <= PIXFMT_BGR16PC)
				{
					mask[0] = 0xf8;
					mask[1] = 0xfc;
				}

				// Convert to 8 bit, masked colour
				colour0[0] &= mask[0];
				colour0[1] &= mask[1];
				colour0[2] &= mask[0];

				// Read the image via CGX (unlike the P96 path we trust this
				// call, mirroring master's pre-migration behaviour)
				ReadPixelArray(image_array,
							   0,
							   0,
							   drag->width * 3,
							   &drag->drag_rp,
							   0,
							   0,
							   drag->width,
							   drag->height,
							   RECTFMT_RGB);

				// Get number of pixels
				count = drag->width * drag->height;

				// Go through image array (identical to the P96 branch)
				for (num = 0, array_pos = 0, bit_pos = 15, word_data = 0, word_pos = 0, x_count = 0; num < count;
					 num++, array_pos += 3, --bit_pos)
				{
					if (image_array[array_pos + 0] != colour0[0] || image_array[array_pos + 1] != colour0[1] ||
						image_array[array_pos + 2] != colour0[2])
					{
						word_data |= 1 << bit_pos;
					}

					if ((++x_count) == drag->width)
					{
						x_count = 0;
						bit_pos = 0;
					}

					if (bit_pos == 0)
					{
#ifdef __AROS__
						((UWORD *)drag->bob.ImageShadow)[word_pos++] = AROS_BE2WORD(word_data);
#else
						((UWORD *)drag->bob.ImageShadow)[word_pos++] = word_data;
#endif
						word_data = 0;
						bit_pos = 16;
					}
				}

				// Get mask size
				count = RASSIZE((((drag->width + 15) >> 4) << 4), drag->height) >> 1;

				// Fill rest of the mask
				while (word_pos < count)
				{
					((UWORD *)drag->bob.ImageShadow)[word_pos++] = word_data;
					word_data = 0;
				}

				FreeVec(image_array);
				ok = 1;
			}
		}

		// (Semi)-normal mode; Allocate temporary bitmap.
		// Standalone if (not else-if) so we still reach it when the P96 or
		// CGX branches were eligible but their pixel read failed.
		if (!ok && (tempbm = AllocBitMap(drag->sprite.Width, drag->sprite.Height, depth, BMF_CLEAR, 0)))
		{
			short columns;

			// Get plane size
			columns = (drag->sprite.Width + 15) >> 4;

			// Blit custom image to temporary bitmap
			BltBitMap(imagebm, 0, 0, tempbm, 0, 0, drag->sprite.Width, drag->sprite.Height, 0xc0, 0xff, 0);
			WaitBlit();

			// Build mask from image
			for (plane = 0; plane < depth; plane++)
			{
				// Get image pointer
				image = (UWORD *)tempbm->Planes[plane];

				// Invalid?
				if (!image)
					continue;

				// Build image
				for (row = 0, off = 0, word = 0; row < drag->sprite.Height; row++)
					for (col = 0; col < columns; col++)
						((UWORD *)drag->bob.ImageShadow)[word++] |= image[off++];
			}

			// Free temporary bitmap
			FreeBitMap(tempbm);

			// Set ok flag
			ok = 1;
		}

		// Failed?
		if (!ok)
		{
			long planesize, x;

			// Get planesize
			planesize = ((drag->width + 15) >> 4) * drag->height;

			// Fill mask with ones
			for (x = 0; x < planesize; x++)
				((UWORD *)drag->bob.ImageShadow)[x] = 0xffff;
		}

		// Opaque/transparent?
		else if (drag->flags & DRAGF_OPAQUE && drag->flags & DRAGF_TRANSPARENT)
		{
			L_BuildTransDragMask((UWORD *)drag->bob.ImageShadow,
								 (UWORD *)drag->sprite.ImageData,
								 drag->sprite.Width,
								 drag->sprite.Height,
								 drag->sprite.Depth,
								 DRAGF_CUSTOM);
		}
	}

	// Normal BOBs
	else if (!(drag->flags & DRAGF_CUSTOM))
	{
		// Calculate remainder mask
		col = (drag->sprite.Width << 4) - drag->width;
		remainder = ~((1 << col) - 1);

		// Opaque/transparent?
		if (drag->flags & DRAGF_OPAQUE && drag->flags & DRAGF_TRANSPARENT)
		{
			if (L_BuildTransDragMask((UWORD *)drag->bob.ImageShadow,
									 (UWORD *)drag->sprite.ImageData,
									 drag->sprite.Width << 4,
									 drag->sprite.Height,
									 drag->sprite.Depth,
									 0))
				return;
		}

		// Build shadow mask
		if (drag->flags & DRAGF_OPAQUE)
			plane = 0;
		else
			plane = -1;
		for (; plane < drag->sprite.Depth; plane++)
		{
			for (row = 0, word = 0; row < drag->sprite.Height; row++)
			{
				for (col = 0; col < drag->sprite.Width; col++, word++)
				{
					if (drag->flags & DRAGF_OPAQUE)
						((unsigned short *)drag->bob.ImageShadow)[word] = 0xffff;
					else if (plane == -1)
						drag->bob.ImageShadow[word] = 0;
					else
						drag->bob.ImageShadow[word] |= drag->sprite.ImageData[off++];
				}

				// Mask off remainder
				drag->bob.ImageShadow[word - 1] &= remainder;
			}
		}
	}

	// Otherwise, no mask
	else
	{
		// Free shadow mask
		FreeVec(drag->bob.ImageShadow);
		drag->bob.ImageShadow = 0;
	}
}

// Show bob
void LIBFUNC L_ShowDragImage(REG(a0, DragInfo *drag), REG(d0, ULONG x), REG(d1, ULONG y))
{
	// If bob not visible, add to list
	if (!(drag->flags & DRAGF_VALID))
	{
		L_AddDragImage(drag);
		if (!(drag->flags & DRAGF_NO_LOCK))
			lock_layers(drag, 1);
	}

	// Check change
	if (drag->sprite.X != x || drag->sprite.Y != y)
	{
		// Position bob
		drag->sprite.X = x;
		drag->sprite.Y = y;

		// Sort GELs list if needed
		if (!(drag->flags & DRAGF_NOSORT))
			SortGList(drag->rastport);

		// Draw BOBs
		L_DrawDragList(drag->rastport, drag->viewport, drag->flags & DRAGF_CUSTOM);

		// Wait for frame if needed
		if (!(drag->flags & DRAGF_NOWAIT))
			WaitTOF();
	}
}

// Add bob to list
void LIBFUNC L_AddDragImage(REG(a0, DragInfo *drag))
{
	// If bob not visible, add to list
	if (!(drag->flags & DRAGF_VALID))
	{
		if (!(drag->flags&DRAGF_CUSTOM))
        	{
        		AddBob(&drag->bob,drag->rastport);
        	}
        	else
		{
			AddVSprite(&drag->sprite, drag->rastport);
		}  
		drag->flags |= DRAGF_VALID;
	}
}

// Remove bob from list
void LIBFUNC L_RemDragImage(REG(a0, DragInfo *drag))
{
	// If bob visible, remove it
	if (drag->flags & DRAGF_VALID)
	{
		// Custom rendering?
		if (drag->flags & DRAGF_CUSTOM)
		{
			// Remove image
			L_RemoveDragImage(drag);
		}

		// Double-buffering?
		else if (drag->flags & DRAGF_DBUF)
		{
			RemBob(&drag->bob);
		}

		// Normal BOBs
		else
		{
			RemIBob(&drag->bob, drag->rastport, drag->viewport);
		}
		drag->flags &= ~DRAGF_VALID;
	}
}

// Stamp bob image down
void LIBFUNC L_StampDragImage(REG(a0, DragInfo *drag), REG(d0, ULONG x), REG(d1, ULONG y))
{
	// Clear SAVEBACK flag
	drag->sprite.Flags &= ~SAVEBACK;

	// Show image
	L_ShowDragImage(drag, x, y);

	// Hide it
	L_HideDragImage(drag);

	// Set SAVEBACK flag again
	drag->sprite.Flags |= SAVEBACK;
}

// Check that a drag hasn't caused a deadlock
BOOL LIBFUNC L_CheckDragDeadlock(REG(a0, DragInfo *drag))
{
	return 0;
}

#ifdef __AROS__
// The commodity broker of AmiStart boldly tries to lock the layers without
// checking the lock count. This leads to a deadlock with the dragging code,
// because the input.device task gets blocked on the AmiStart broker.
static void message_amistart(ULONG command)
{
	struct Library *CxBase;

	if ((CxBase = OpenLibrary("commodities.library", 50)))
	{
		BrokerCommand("AmiStart", command);
		CloseLibrary(CxBase);
	}
}
#endif

void lock_layers(DragInfo *drag, BOOL lock)
{
	struct Layer_Info *info;

	// Get layer info pointer
	info = &drag->window->WScreen->LayerInfo;

	// Lock layer info
	if (lock)
	{
#ifdef __AROS__
		message_amistart(CXCMD_DISABLE);
#endif
		LockLayers(info);
	}
	else
	{
		UnlockLayers(info);
#ifdef __AROS__
		message_amistart(CXCMD_ENABLE);
#endif
	}
}

// Build a transparent/opaque drag mask
BOOL LIBFUNC L_BuildTransDragMask(REG(a0, UWORD *mask),
								  REG(a1, UWORD *image),
								  REG(d0, short width),
								  REG(d1, short height),
								  REG(d2, short depth),
								  REG(d3, long flags))
{
	long x, word, off;
	short plane, row, col, columns, tmpsize;
	struct BitMap bm;
	struct BitMap *maskbm;
	struct RastPort rp;
	struct TmpRas tmpras;
	PLANEPTR tmprasbuf, temp;

	// Get plane size
	columns = (width + 15) >> 4;
	tmpsize = RASSIZE((width + 2), (height + 2));

	// Allocate a bitmap to build the mask, and a TmpRas buffer
	if (!(maskbm = L_NewBitMap(width + 2, height + 2, 2, BMF_CLEAR, 0)) ||
		!(tmprasbuf = (PLANEPTR)AllocVec(tmpsize, MEMF_CHIP)))
	{
		// Free what we got
		L_DisposeBitMap(maskbm);

		// Failed, fill the mask with ones (for non-custom)
		if (!(flags & DRAGF_CUSTOM))
		{
			long planesize = columns * height;
			for (x = 0; x < planesize; x++)
				mask[x] = 0xffff;
		}
		return 0;
	}

	// Initialise RastPort and TmpRas for mask bitmap
	InitRastPort(&rp);
	rp.BitMap = maskbm;
	InitTmpRas(&tmpras, tmprasbuf, tmpsize);
	rp.TmpRas = &tmpras;

	// Initialise dummy bitmap to point to mask
	InitBitMap(&bm, 1, width, height);
	bm.Planes[0] = (PLANEPTR)mask;

	// Non-custom image
	if (!(flags & DRAGF_CUSTOM))
	{
		// Build mask from image
		for (plane = 0, off = 0; plane < depth; plane++)
			for (row = 0, word = 0; row < height; row++)
				for (col = 0; col < columns; col++)
					mask[word++] |= image[off++];
	}

	// Blit mask into slightly bigger bitmap, leaving a one-pixel gap around the edge
	BltBitMap(&bm, 0, 0, maskbm, 1, 1, width, height, 0xc0, 0x1, 0);

	// Fill into secondary plane only of mask bitmap
	SetAPen(&rp, 2);

	// Flood fill the outside of the mask
	Flood(&rp, 1, 0, 0);

	// Switch plane pointers in mask bitmap
	temp = maskbm->Planes[0];
	maskbm->Planes[0] = maskbm->Planes[1];
	maskbm->Planes[1] = temp;

	// Copy from flooded mask plane and invert to get real mask
	BltBitMap(maskbm, 1, 1, &bm, 0, 0, width, height, 0x30, 0x1, tmprasbuf);

	// Wait for blitter
	WaitBlit();

	// Free work memory
	L_DisposeBitMap(maskbm);
	FreeVec(tmprasbuf);

	return 1;
}

// Allocate and initialise double-buffering info
BOOL LIBFUNC L_InitDragDBuf(REG(a0, DragInfo *drag))
{
	// Already got it?
	if (drag->dbuf)
		return 1;

	// Allocate structure
	if (!(drag->dbuf = AllocVec(sizeof(struct DBufPacket), MEMF_CLEAR)))
		return 0;

	// Allocate dbuf buffer
	if (!(drag->dbuf->BufBuffer =
			  (WORD *)AllocVec(RASSIZE((drag->sprite.Width << 4), drag->height) * drag->sprite.Depth, MEMF_CHIP)))
	{
		// Failed
		FreeVec(drag->dbuf);
		return 0;
	}

	// Set flag and pointer in bob
	drag->flags |= DRAGF_DBUF;
	drag->bob.DBuffer = drag->dbuf;

	return 1;
}

// Drag BOB list
void LIBFUNC L_DrawDragList(REG(a0, struct RastPort *rp), REG(a1, struct ViewPort *vp), REG(d0, long flags))
{
	struct VSprite *sprite;
	short dest_width, dest_height;
	struct RastPort saverp;

	// Non-custom rendering?
	if (!(flags & DRAGF_CUSTOM))
	{
		// Draw using graphics
		DrawGList(rp, vp);
		return;
	}

	// Get size of destination bitmap
	dest_width = GetBitMapAttr(rp->BitMap, BMA_WIDTH);
	dest_height = GetBitMapAttr(rp->BitMap, BMA_HEIGHT);

	// Initialise RastPort for SaveBuffer
	InitRastPort(&saverp);

	// Go backwards through sprite list to remove old images
	for (sprite = rp->GelsInfo->gelTail->PrevVSprite; sprite && sprite != rp->GelsInfo->gelHead;
		 sprite = sprite->PrevVSprite)
	{
		// Skip over non-BOBs
		if (!sprite->VSBob)
			continue;

		// Background has been saved?
		if (sprite->Flags & BACKSAVED)
		{
			short x, y, w, h, dx, dy;

			// Get destination coordinates
			dx = sprite->OldX;
			dy = sprite->OldY;

			// Is there a layer?
			if (rp->Layer)
			{
				// Don't need to clip
				x = 0;
				y = 0;
				w = sprite->Width;
				h = sprite->Height;
			}

			// Get clipped-coordinates
			else
			{
				if (dx < 0)
				{
					x = -dx;
					dx = 0;
				}
				else
					x = 0;
				if (dy < 0)
				{
					y = -dy;
					dy = 0;
				}
				else
					y = 0;
				w = sprite->Width - x;
				h = sprite->Height - y;
				if (dx + w > dest_width)
					w = dest_width - dx;
				if (dy + h > dest_height)
					h = dest_height - dy;
			}

			// Anything to draw?
			if (w > 0 && h > 0)
			{
				// Restore background
				BltBitMapRastPort((struct BitMap *)sprite->VSBob->SaveBuffer, x, y, rp, dx, dy, w, h, 0xc0);
			}

			// Clear save flag
			sprite->Flags &= ~BACKSAVED;
		}
	}

	// Remove only?
	if (flags & DRAGF_REMOVE)
		return;

	// Go through sprite list to draw new images
	for (sprite = rp->GelsInfo->gelHead->NextVSprite; sprite && sprite != rp->GelsInfo->gelTail;
		 sprite = sprite->NextVSprite)
	{
		short x = 0, y = 0, w, h, dx, dy;

		// Skip over non-BOBs
		if (!sprite->VSBob)
			continue;

		// Get destination coordinates
		dx = sprite->X;
		dy = sprite->Y;

		// Clip coordines to left/top borders
		if (dx < 0)
		{
			x = -dx;
			dx = 0;
		}
		if (dy < 0)
		{
			y = -dy;
			dy = 0;
		}

		// Get width to draw
		w = sprite->Width - x;
		h = sprite->Height - y;

		// Clip width to right/bottom borders
		if (dx + w > dest_width)
			w = dest_width - dx;
		if (dy + h > dest_height)
			h = dest_height - dy;

		// Anything to draw?
		if (w > 0 && h > 0)
		{
			// Save background?
			if (sprite->Flags & SAVEBACK)
			{
				// Set RastPort bitmap pointer
				saverp.BitMap = (struct BitMap *)sprite->VSBob->SaveBuffer;

				// Backup background
				ClipBlit(rp, dx, dy, &saverp, x, y, w, h, 0xc0);

				// Set save flag
				sprite->Flags |= BACKSAVED;
			}

			// Shadow mask?
			if (sprite->VSBob->ImageShadow)
			{
				// Draw image using image shadow
				BltMaskBitMapRastPort((struct BitMap *)sprite->ImageData,
									  x,
									  y,
									  rp,
									  dx,
									  dy,
									  w,
									  h,
									  (ABC | ABNC | ANBC),
									  (PLANEPTR)sprite->VSBob->ImageShadow);
			}

			// No mask
			else
			{
				// Draw image
				BltBitMapRastPort((struct BitMap *)sprite->ImageData, x, y, rp, dx, dy, w, h, 0xc0);
			}
		}

		// Save old coordinates
		sprite->OldX = sprite->X;
		sprite->OldY = sprite->Y;
	}
}

// Remove a BOB from the GELs list
void LIBFUNC L_RemoveDragImage(REG(a0, DragInfo *drag))
{
	struct RastPort *rp;

	// Get RastPort
	rp = drag->rastport;

	// Need to restore background?
	if (drag->sprite.Flags & BACKSAVED)
	{
		short x, y, w, h, dx, dy;
		short dest_width, dest_height;

		// Get size of destination bitmap
		dest_width = GetBitMapAttr(rp->BitMap, BMA_WIDTH);
		dest_height = GetBitMapAttr(rp->BitMap, BMA_HEIGHT);

		// Get destination coordinates
		dx = drag->sprite.OldX;
		dy = drag->sprite.OldY;

		// Is there a layer?
		if (rp->Layer)
		{
			// Don't need to clip
			x = 0;
			y = 0;
			w = drag->sprite.Width;
			h = drag->sprite.Height;
		}

		// Get clipped-coordinates
		else
		{
			if (dx < 0)
			{
				x = -dx;
				dx = 0;
			}
			else
				x = 0;
			if (dy < 0)
			{
				y = -dy;
				dy = 0;
			}
			else
				y = 0;
			w = drag->sprite.Width - x;
			h = drag->sprite.Height - y;
			if (dx + w > dest_width)
				w = dest_width - dx;
			if (dy + h > dest_height)
				h = dest_height - dy;
		}

		// Anything to draw?
		if (w > 0 && h > 0)
		{
			// Restore background
			BltBitMapRastPort((struct BitMap *)drag->sprite.VSBob->SaveBuffer, x, y, rp, dx, dy, w, h, 0xc0);
		}
	}

	// Remove sprite from list
	drag->sprite.PrevVSprite->NextVSprite = drag->sprite.NextVSprite;
	drag->sprite.NextVSprite->PrevVSprite = drag->sprite.PrevVSprite;
}

// See if custom dragging is OK
BOOL LIBFUNC L_DragCustomOk(REG(a0, struct BitMap *bm), REG(a6, struct MyLibrary *libbase))
{
	BOOL ok = 0;

	struct LibData *data;

#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Get data pointer
	data = (struct LibData *)libbase->ml_UserData;

	// Ok to custom drag?
	// Prefer P96's P96BMA_ISP96 probe - it's the reliable RTG check on
	// OS3.1+ (BMF_STANDARD is a graphics.library V45+ flag and missing
	// from OS3.1). Fall back to CyberGraphX's ISCYBERGFX probe on
	// installs that only have CGX resident, so custom drag still gets
	// enabled there.
	if (!(data->flags & LIBDF_NO_CUSTOM_DRAG) && ((struct Library *)GfxBase)->lib_Version >= 39)
	{
#if !defined(__AROS__)
		if (P96Base && p96GetBitMapAttr(bm, P96BMA_ISP96))
			ok = 1;
		else
#endif
		if (CyberGfxBase && GetCyberMapAttr(bm, CYBRMATTR_ISCYBERGFX))
			ok = 1;
	}

	return ok;
}
