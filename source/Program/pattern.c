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

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

void safe_RectFill(struct RastPort *, short, short, short, short);

#define PDTA_DestMode (DTA_Dummy + 251)
#define PDTA_UseFriendBitMap (DTA_Dummy + 255)

#define MODE_V43 1

// Load a backfill pattern; pattern must be locked exclusively
void GetPattern(PatternData *pattern, struct Screen *screen, ULONG border_col)
{
	// Initially make invalid
	pattern->valid = FALSE;
	pattern->border_pen = 0;

	// Set screen pointer
	pattern->screen = screen;

	// Check for invalid data
	if (!pattern->data)
	{
		// Update border pen
		GetPatternBorder(pattern, screen, border_col);
		return;
	}

	// Bitmap pattern?
	if (pattern->prefs.wbp_Flags & WBPF_PATTERN)
	{
		UWORD *ptr, *planes[MAXDEPTH];
		short num, row, col, pen;

		// Get plane pointers
		for (num = 0; num < pattern->prefs.wbp_Depth; num++)
		{
			planes[num] = ((UWORD *)pattern->data) + (PAT_HEIGHT * num);
		}
		pattern->fill_plane_key = 0;

		// Clear fill initially
		for (num = 0, ptr = pattern->fill[0]; num < 128; num++, ptr++)
			*ptr = 0;

		// Under 37 use fill pattern as supplied
		if (((struct Library *)GfxBase)->lib_Version < 39)
		{
			for (num = 0; num < pattern->prefs.wbp_Depth; num++)
			{
				CopyMem((char *)planes[num], (char *)pattern->fill[num], PAT_HEIGHT * sizeof(UWORD));
			}
			pattern->fill_plane_key = pattern->prefs.wbp_Depth;
		}

		// Otherwise, we need to remap
		else
		{
			// Go through pattern rows
			for (row = 0; row < PAT_HEIGHT; row++)
			{
				// Go through bits
				for (col = 0; col < PAT_WIDTH; col++)
				{
					// Clear pen
					pen = 0;

					// Calculate pixel colour
					for (num = 0; num < pattern->prefs.wbp_Depth; num++)
					{
						// Is bit set in this bitplane?
						if (planes[num][row] & (1 << col))
							pen |= (1 << num);
					}

					// Pixel set?
					if (pen)
					{
						// Map pens 4-7 to top four colours
						if (pen >= 4 && screen->RastPort.BitMap->Depth <= 8)
						{
							pen = (1 << screen->RastPort.BitMap->Depth) - (8 - pen);
						}

						// Remap
						for (num = 0; num < 8; num++)
						{
							// Want this bit?
							if (pen & (1 << num))
							{
								// Set bit
								pattern->fill[num][row] |= 1 << col;
								if (num >= pattern->fill_plane_key)
									pattern->fill_plane_key = num + 1;
							}
						}
					}
				}
			}
		}

		// Any data?
		if (pattern->fill_plane_key > 0)
		{
			short depth;

			// Get depth for bitmap
			if (((struct Library *)GfxBase)->lib_Version >= 39)
				depth = GetBitMapAttr(screen->RastPort.BitMap, BMA_DEPTH);
			else
				depth = screen->RastPort.BitMap->Depth;

			// Check its not too deep
			if (depth > 8)
				depth = 8;

			// Allocate bitmap for pattern
			if ((pattern->pattern_bitmap = NewBitMap(PATTILE_SIZE, PATTILE_SIZE, depth, BMF_CLEAR, 0)))
			{
				struct RastPort rp;
				struct BitMap *bitmap;

				// Initialise dummy rastport
				InitRastPort(&rp);
				rp.BitMap = pattern->pattern_bitmap;

				// Set fill pattern
				SetAfPt(&rp, pattern->fill[0], -4);

				// Fill bitmap
				SetRast(&rp, 0);
				RectFill(&rp, 0, 0, PATTILE_SIZE - 1, PATTILE_SIZE - 1);

				// Is the screen a non-standard bitmap?
				if (((struct Library *)GfxBase)->lib_Version >= 39 &&
					!(GetBitMapAttr(screen->RastPort.BitMap, BMA_FLAGS) & BMF_STANDARD))
				{
					// Try and allocate friend bitmap
					if ((bitmap = NewBitMap(PATTILE_SIZE, PATTILE_SIZE, depth, BMF_CLEAR, screen->RastPort.BitMap)))
					{
						// Copy to friend bitmap
						BltBitMap(
							pattern->pattern_bitmap, 0, 0, bitmap, 0, 0, PATTILE_SIZE, PATTILE_SIZE, 0xc0, 0xff, 0);

						// Free original bitmap
						DisposeBitMap(pattern->pattern_bitmap);

						// Use new bitmap pointer
						pattern->pattern_bitmap = bitmap;
					}
				}

				// Get bitmap pointer
				pattern->bitmap = pattern->pattern_bitmap;

				// Get pattern size
				pattern->width = PATTILE_SIZE;
				pattern->height = PATTILE_SIZE;
			}
		}
	}

	// Picture; must have datatypes
	else if (DataTypesBase)
	{
		short len;
		BOOL remap = 1;
		long precision = PRECISION_IMAGE;
		char name[256];
		long ver = 0;
		struct Library *pdt;
#ifdef PATCH_OK
		APTR patchhandle = 0;
#endif

		// Check picture datatype version; use the copy already in memory if there is one,
		// since opening by path can mean a disk access for every pattern
		Forbid();
		if ((pdt = (struct Library *)FindName(&SysBase->LibList, "picture.datatype")))
			ver = pdt->lib_Version;
		Permit();
		if (!pdt && ((pdt = OpenLibrary("sys:classes/datatypes/picture.datatype", 0)) ||
					 (pdt = OpenLibrary("picture.datatype", 0))))
		{
			// Get version
			ver = pdt->lib_Version;
			CloseLibrary(pdt);
		}

		// No remap?
		if ((len = strlen(pattern->data)) > 8 && stricmp(pattern->data + len - 8, ".noremap") == 0)
			remap = 0;

		// Precision?
		if (len > 6 && stricmp(pattern->data + len - 6, ".exact") == 0)
			precision = PRECISION_EXACT;

		// Specified in prefs?
		else if (pattern->precision)
		{
			if (pattern->precision == -1)
				remap = 0;
			else if (pattern->precision == 1)
				precision = PRECISION_ICON;
			else if (pattern->precision == 2)
				precision = PRECISION_GUI;
			else if (pattern->precision == 3)
				precision = PRECISION_EXACT;
		}

#ifdef PATCH_OK
		// If we've got v43 of the datatype, we don't need the fastram patch
		if (ver < 43)
		{
			// Add this task to the patchlist for allocbitmap
			patchhandle = AddAllocBitmapPatch(FindTask(0), screen);
		}
#endif

		// Turn back on requesters
		((struct Process *)FindTask(0))->pr_WindowPtr = GUI->window;

		// Check for random pictures, and then load picture
		if (pattern_check_random(pattern, name) &&
			(pattern->object = NewDTObject(name,
										   DTA_GroupID,
										   GID_PICTURE,
										   PDTA_Screen,
										   screen,
										   PDTA_FreeSourceBitMap,
										   TRUE,
										   (ver > 42) ? PDTA_DestMode : TAG_IGNORE,
										   MODE_V43,
										   (ver > 42) ? PDTA_UseFriendBitMap : TAG_IGNORE,
										   TRUE,
										   TAG_END)))
		{
			struct BitMapHeader *header;
			struct BitMap *bitmap;
			D_S(struct FileInfoBlock, fib)

			// Get file information
			if (GetFileInfo(name, fib) && strnicmp(fib->fib_Comment, "dopus ", 6) == 0)
			{
				char *args = fib->fib_Comment + 6;
				short val;
				rexx_skip_space(&args);
				while ((val = rexx_match_keyword(&args, background_key, 0)) != -1)
				{
					// Tile/center/stretch
					if (val == 3)
						pattern->flags &= ~(PATF_CENTER | PATF_STRETCH);
					else if (val == 4)
					{
						pattern->flags |= PATF_CENTER;
						pattern->flags &= ~PATF_STRETCH;
					}
					else if (val == 5 && pattern->prefs.wbp_Which == 0)
					{
						pattern->flags |= PATF_STRETCH;
						pattern->flags &= ~PATF_CENTER;
					}

					// Precision
					else if (val == 6)
					{
						// Get precision
						rexx_skip_space(&args);
						if ((val = rexx_match_keyword(&args, precision_key, 0)) > -1)
						{
							if (val == 0)
								remap = 0;
							else if (val == 1)
								precision = PRECISION_GUI;
							else if (val == 2)
								precision = PRECISION_ICON;
							else if (val == 3)
								precision = PRECISION_IMAGE;
							else if (val == 4)
								precision = PRECISION_EXACT;
						}
					}

					// Border
					else if (val == 8)
					{
						// Off?
						rexx_skip_space(&args);
						if (rexx_match_keyword(&args, on_off_strings2, 0) == 0)
							border_col &= ~ENVBF_USE_COLOUR;
						else
						{
							border_col = ENVBF_USE_COLOUR;
							border_col |= Atoh(args, -1) << 8;
						}
					}
				}
			}

			// Layout picture (should check for success)
			SetDTAttrs(pattern->object, 0, 0, OBP_Precision, precision, PDTA_Remap, remap, TAG_END);
			DoMethod(pattern->object, DTM_PROCLAYOUT, 0, 1);

			// Get bitmap pointer and header
			GetDTAttrs(pattern->object,
					   PDTA_BitMap,
					   &bitmap,
					   PDTA_DestBitMap,
					   &pattern->bitmap,
					   PDTA_BitMapHeader,
					   &header,
					   TAG_END);

			// No dest bitmap?
			if (!pattern->bitmap)
				pattern->bitmap = bitmap;

			// Valid bitmap?
			if (pattern->bitmap)
			{
				// Get width and height
				pattern->width = header->bmh_Width;
				pattern->height = header->bmh_Height;
			}
		}

		// Turn off requesters
		((struct Process *)FindTask(0))->pr_WindowPtr = (APTR)-1;

#ifdef PATCH_OK
		// Remove this task from the patchlist for allocbitmap
		if (patchhandle)
			RemAllocBitmapPatch(patchhandle);
#endif

		// Got bitmap?
		if (pattern->bitmap)
		{
			// Stretch?
			if (pattern->flags & PATF_STRETCH)
			{
				// Not the size of the screen?
				if (pattern->width != screen->Width || pattern->height != screen->Height)
				{
					// Allocate a bitmap for the stretched picture
					if ((pattern->pattern_bitmap = NewBitMap(screen->Width,
															 screen->Height,
															 GetBitMapAttr(pattern->bitmap, BMA_DEPTH),
															 BMF_CLEAR,
															 screen->RastPort.BitMap)))
					{
						struct BitScaleArgs scale;

						// Scale bitmap
						scale.bsa_SrcX = 0;
						scale.bsa_SrcY = 0;
						scale.bsa_SrcWidth = pattern->width;
						scale.bsa_SrcHeight = pattern->height;
						scale.bsa_DestX = 0;
						scale.bsa_DestY = 0;
						scale.bsa_DestWidth = screen->Width;
						scale.bsa_DestHeight = screen->Height;
						scale.bsa_XSrcFactor = scale.bsa_SrcWidth;
						scale.bsa_XDestFactor = scale.bsa_DestWidth;
						scale.bsa_YSrcFactor = scale.bsa_SrcHeight;
						scale.bsa_YDestFactor = scale.bsa_DestHeight;
						scale.bsa_SrcBitMap = pattern->bitmap;
						scale.bsa_DestBitMap = pattern->pattern_bitmap;
						scale.bsa_Flags = 0;
						BitMapScale(&scale);

						// Get pointer to scaled bitmap
						pattern->bitmap = pattern->pattern_bitmap;
						pattern->width = screen->Width;
						pattern->height = screen->Height;
					}
				}
			}
		}

		// Update border pen
		GetPatternBorder(pattern, screen, border_col);
	}

	// Pattern ok now
	pattern->valid = TRUE;
}

// Get border pen
void GetPatternBorder(PatternData *pattern, struct Screen *screen, ULONG border_col)
{
	// Do nothing under 37
	if (((struct Library *)GfxBase)->lib_Version < 39)
		return;

	// Free existing border pen
	if (pattern->border_pen != 0)
	{
		if (pattern->border_pen == -1)
			pattern->border_pen = 0;
		ReleasePen(screen->ViewPort.ColorMap, pattern->border_pen);
		pattern->border_pen = 0;
	}

	// Want a border pen?
	if (border_col & ENVBF_USE_COLOUR)
	{
		// Try to allocate pen
		if ((pattern->border_pen = ObtainBestPen(screen->ViewPort.ColorMap,
												 ENVBF_COL_R_GET(border_col) << 24,
												 ENVBF_COL_G_GET(border_col) << 24,
												 ENVBF_COL_B_GET(border_col) << 24,
												 OBP_Precision,
												 PRECISION_IMAGE,
												 TAG_END)) == 0)
			pattern->border_pen = -1;
		else if (pattern->border_pen == -1)
			pattern->border_pen = 0;
	}
}

// Free backfill pattern; pattern must be locked exclusively
void FreePattern(PatternData *pattern)
{
	// Mark as invalid
	pattern->valid = FALSE;

	// Free object
	if (pattern->object)
	{
		DisposeDTObject(pattern->object);
		pattern->object = 0;
	}

	// Got a border pen?
	if (pattern->border_pen != 0)
	{
		if (pattern->border_pen == -1)
			pattern->border_pen = 0;
		ReleasePen(pattern->screen->ViewPort.ColorMap, pattern->border_pen);
		pattern->border_pen = 0;
	}

	// Free friend bitmap
	if (pattern->pattern_bitmap)
	{
		DisposeBitMap(pattern->pattern_bitmap);
		pattern->pattern_bitmap = 0;
	}

	// Clear bitmap pointer
	pattern->bitmap = 0;
}

// Window backfill
void ASM SAVEDS PatternBackfill(REG(a0, PatternInstance *instance),
								REG(a2, struct RastPort *rport),
								REG(a1, struct BackFillInfo *info))
{
	PatternData *pattern;
	struct RastPort rp;
	short border_pen = 0;

	// Copy rastport and clear any layer clipping
	rp = *rport;
	rp.Layer = 0;

	// Get pattern
	pattern = instance->pattern;

	// Get border pen colour
	if (pattern->border_pen != 0)
	{
		border_pen = (pattern->border_pen == -1) ? 0 : pattern->border_pen;
	}

	// Valid picture?
	if (pattern->valid && !instance->disabled && pattern->bitmap)
	{
		short offsetx, offsety;
		short SrcOffsetX, SrcOffsetY;
		short FirstSizeX, FirstSizeY;
		short SecondMinX, SecondMinY;
		short SecondSizeX, SecondSizeY;
		short Pos, Size;
		BOOL center = 0;

		// Get offset
#ifdef __AROS__
		offsetx = info->bounds.MinX;
		offsety = info->bounds.MinY;
#else
		offsetx = info->offsetx;
		offsety = info->offsety;
#endif

		// Get centering flag
		if (pattern->flags & PATF_CENTER)
			center = 1;

		// Icon mode, real-time scrolling?
		if (instance->hook.h_Data && environment->env->display_options & DISPOPTF_REALTIME_SCROLL)
		{
			BackdropInfo *info;

			// Get info pointer
			info = (BackdropInfo *)instance->hook.h_Data;

			// Adjust offset
			offsetx += info->offset_x;	//%pattern->width;
			offsety += info->offset_y;	//%pattern->height;
		}

		// Center pattern?
		if (center && info->layer)
		{
			short x, y, x0, y0, x1, y1;
			BOOL fail = 0;

			// Get size of window
			FirstSizeX = (info->layer->bounds.MaxX - info->layer->bounds.MinX) + 1;
			FirstSizeY = (info->layer->bounds.MaxY - info->layer->bounds.MinY) + 1;

			// Get position of tile within window
			x = info->layer->bounds.MinX + ((FirstSizeX - pattern->width) >> 1);
			y = info->layer->bounds.MinY + ((FirstSizeY - pattern->height) >> 1);

			// Get position of tile
			x0 = x;
			y0 = y;
			x1 = x + pattern->width - 1;
			y1 = y + pattern->height - 1;

			// Check drawing does not go outside bounds
			if (x0 < info->bounds.MinX)
				x0 = info->bounds.MinX;
			else if (x0 > info->bounds.MaxX)
				fail = 1;
			if (y0 < info->bounds.MinY)
				y0 = info->bounds.MinY;
			else if (y0 > info->bounds.MaxY)
				fail = 1;

			if (x1 > info->bounds.MaxX)
				x1 = info->bounds.MaxX;
			else if (x1 < info->bounds.MinX)
				fail = 1;
			if (y1 > info->bounds.MaxY)
				y1 = info->bounds.MaxY;
			else if (y1 < info->bounds.MinY)
				fail = 1;

			// Ok to draw?
			if (!fail)
			{
				// Get position in source image
				SrcOffsetX = x0 - x;
				SrcOffsetY = y0 - y;

				// Blit the tile
				BltBitMap(pattern->bitmap,
						  SrcOffsetX,
						  SrcOffsetY,
						  rp.BitMap,
						  x0,
						  y0,
						  (x1 - x0) + 1,
						  (y1 - y0) + 1,
						  0xc0,
						  0xff,
						  0);
			}

			// If we failed, just get the full center position to clear around it
			else
			{
				x0 = x;
				y0 = y;
				x1 = x + pattern->width - 1;
				y1 = y + pattern->height - 1;
			}

			// Set pen to clear other areas
			SetAPen(&rp, border_pen);
			SetDrMd(&rp, JAM1);

			// To the left of the image?
			if (info->bounds.MinX < x0)
				safe_RectFill(&rp,
							  info->bounds.MinX,
							  info->bounds.MinY,
							  (info->bounds.MaxX < x0 - 1) ? info->bounds.MaxX : (x0 - 1),
							  info->bounds.MaxY);

			// To the right of the image?
			if (info->bounds.MaxX > x1)
				safe_RectFill(&rp,
							  (info->bounds.MinX > x1) ? info->bounds.MinX : (x1 + 1),
							  info->bounds.MinY,
							  info->bounds.MaxX,
							  info->bounds.MaxY);

			// Above the image?
			if (info->bounds.MinY < y0)
				safe_RectFill(&rp,
							  (info->bounds.MinX < x0) ? x0 : info->bounds.MinX,
							  info->bounds.MinY,
							  (info->bounds.MaxX > x1) ? x1 : info->bounds.MaxX,
							  (info->bounds.MaxY < y0 - 1) ? info->bounds.MaxY : (y0 - 1));

			// Below the image?
			if (info->bounds.MaxY > y1)
				safe_RectFill(&rp,
							  (info->bounds.MinX < x0) ? x0 : info->bounds.MinX,
							  (info->bounds.MinY > y1) ? info->bounds.MinY : (y1 + 1),
							  (info->bounds.MaxX > x1) ? x1 : info->bounds.MaxX,
							  info->bounds.MaxY);
			return;
		}

		// Calculate offsets
		SrcOffsetX = info->bounds.MinX - (info->bounds.MinX - offsetx);
		SrcOffsetX = MOD(SrcOffsetX, pattern->width);
		SrcOffsetY = info->bounds.MinY - (info->bounds.MinY - offsety);
		SrcOffsetY = MOD(SrcOffsetY, pattern->height);

		// The width of the first tile, this is either the rest of the tile right to SrcOffsetX
		// or the width of the dest rect, if the rect is narrow
		FirstSizeX = MIN((pattern->width - SrcOffsetX), (RECTWIDTH(&info->bounds)));

		// The start for the second tile (if used)
		SecondMinX = info->bounds.MinX + FirstSizeX;

		// The width of the second tile (we want the whole tile to be pattern->width pixels wide,
		// if we use pattern->width-SrcOffsetX pixels for the left part we'll use SrcOffsetX for the right part)
		SecondSizeX = MIN(SrcOffsetX, (info->bounds.MaxX - SecondMinX + 1));

		// The same values are calculated for y direction
		FirstSizeY = MIN((pattern->height - SrcOffsetY), (RECTHEIGHT(&info->bounds)));
		SecondMinY = info->bounds.MinY + FirstSizeY;
		SecondSizeY = MIN(SrcOffsetY, (info->bounds.MaxY - SecondMinY + 1));

		// Wait for the blitter, for some reason...
		WaitBlit();

		// Blit the first piece of the tile
		BltBitMap(pattern->bitmap,
				  SrcOffsetX,
				  SrcOffsetY,
				  rp.BitMap,
				  info->bounds.MinX,
				  info->bounds.MinY,
				  FirstSizeX,
				  FirstSizeY,
				  0xc0,
				  0xff,
				  0);

		// If SrcOffset was 0 or the dest rect was too narrow, we won't need a second column
		if (SecondSizeX > 0)
			BltBitMap(pattern->bitmap,
					  0,
					  SrcOffsetY,
					  rp.BitMap,
					  SecondMinX,
					  info->bounds.MinY,
					  SecondSizeX,
					  FirstSizeY,
					  0xc0,
					  0xff,
					  0);

		// Is a second row necessary?
		if (SecondSizeY > 0)
		{
			// Blit second row
			BltBitMap(pattern->bitmap,
					  SrcOffsetX,
					  0,
					  rp.BitMap,
					  info->bounds.MinX,
					  SecondMinY,
					  FirstSizeX,
					  SecondSizeY,
					  0xc0,
					  0xff,
					  0);

			// Second column in second row
			if (SecondSizeX > 0)
				BltBitMap(
					pattern->bitmap, 0, 0, rp.BitMap, SecondMinX, SecondMinY, SecondSizeX, SecondSizeY, 0xc0, 0xff, 0);
		}

		// This loop generates the first row of the tiles
		for (Pos = info->bounds.MinX + pattern->width, Size = MIN(pattern->width, (info->bounds.MaxX - Pos + 1));
			 Pos <= info->bounds.MaxX;)
		{
			// Blit across row
			BltBitMap(rport->BitMap,
					  info->bounds.MinX,
					  info->bounds.MinY,
					  rport->BitMap,
					  Pos,
					  info->bounds.MinY,
					  Size,
					  MIN(pattern->height, RECTHEIGHT(&info->bounds)),
					  0xc0,
					  0xff,
					  0);

			// Increment position, decrement size
			Pos += Size;
			Size = MIN((Size << 1), (info->bounds.MaxX - Pos + 1));
		}

		// This loop blit the first row down several times to fill the whole dest rect
		for (Pos = info->bounds.MinY + pattern->height, Size = MIN(pattern->height, (info->bounds.MaxY - Pos + 1));
			 Pos <= info->bounds.MaxY;)
		{
			// Blit down display
			BltBitMap(rport->BitMap,
					  info->bounds.MinX,
					  info->bounds.MinY,
					  rport->BitMap,
					  info->bounds.MinX,
					  Pos,
					  RECTWIDTH(&info->bounds),
					  Size,
					  0xc0,
					  0xff,
					  0);

			// Increment position, decrement size
			Pos += Size;
			Size = MIN((Size << 1), (info->bounds.MaxY - Pos + 1));
		}
	}

	// Need to clear
	else
	{
		// No valid pattern, just clear area
		SetAPen(&rp, border_pen);
		SetDrMd(&rp, JAM1);

		// Erase area
		RectFill(&rp, info->bounds.MinX, info->bounds.MinY, info->bounds.MaxX, info->bounds.MaxY);
	}
}

void safe_RectFill(struct RastPort *rp, short x0, short y0, short x1, short y1)
{
	if (x0 <= x1 && y0 <= y1)
		RectFill(rp, x0, y0, x1, y1);
}

// Check for random pictures
BOOL pattern_check_random(PatternData *data, char *name)
{
	D_S(struct FileInfoBlock, fib)
	D_S(struct AnchorPath, anchor)
	Att_List *list;
	Att_Node *node = 0;
	short count = 0, num, wild = 0, err;
	char buf[320], *ptr;
	APTR file;

	// Check for wildcard characters
	if ((ptr = FilePart(data->data)) && (strchr(ptr, '*') || strchr(ptr, '?') || strchr(ptr, '#')))
		wild = 1;

	// If supplied picture is a file (or doesn't exist), use the name as given
	else if (!GetFileInfo(data->data, fib) || fib->fib_DirEntryType < 0)
	{
		strcpy(name, data->data);
		data->random = 0;
		return 1;
	}

	// Set 'random' flag
	data->random = 1;

	// Build name to search directory
	strcpy(buf, data->data);
	if (!wild)
		AddPart(buf, "*", 300);

	// Build list of matching files
	anchor->ap_BreakBits = 0;
	anchor->ap_Flags = 0;
	anchor->ap_Strlen = 0;
	if ((err = MatchFirst(buf, anchor)) || !(list = Att_NewList(LISTF_POOL)))
	{
		if (!err)
			MatchEnd(anchor);
		return 0;
	}
	while (!err)
	{
		if (anchor->ap_Info.fib_DirEntryType < 0)
		{
			if (Att_NewNode(list, anchor->ap_Info.fib_FileName, 0, 0))
				++count;
		}
		else
			anchor->ap_Flags &= ~APF_DODIR;
		err = MatchNext(anchor);
	}
	MatchEnd(anchor);

	// No pictures?
	if (count < 1)
	{
		Att_RemList(list, 0);
		return 0;
	}

	// See what the last random picture was
	lsprintf(buf, "DOpus5:System/rnd.%ld", data->prefs.wbp_Which);
	if ((file = OpenBuf(buf, MODE_OLDFILE, 512)))
	{
		ReadBufLine(file, buf + 64, 256);
		CloseBuf(file);
	}
	else
		buf[64] = 0;

	// Pick random picture
	while (!node)
	{
		num = RandomDopus(8193);
		num %= count;
		if (!(node = Att_FindNode(list, num)))
			break;

		// Check it's not the one used last time
		if (stricmp(node->node.ln_Name, FilePart(buf + 64)) != 0 || count == 1)
			break;
		node = 0;
	}

	// Got picture?
	if (node)
	{
		// Save name of picture so it won't be used next time
		strcpy(buf + 64, data->data);
		if (wild && (ptr = FilePart(buf + 64)))
			*ptr = 0;
		AddPart(buf + 64, node->node.ln_Name, 256);
		if ((file = OpenBuf(buf, MODE_NEWFILE, 512)))
		{
			WriteBuf(file, buf + 64, strlen(buf + 64) + 1);
			CloseBuf(file);
		}

		// Return name
		strcpy(name, buf + 64);
	}

	Att_RemList(list, 0);
	return (BOOL)((node) ? 1 : 0);
}
