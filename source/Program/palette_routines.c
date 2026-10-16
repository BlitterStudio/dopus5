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
#ifdef __amigaos3__
	#include <stddef.h>
	#include <prefs/workbench.h>
#endif

#define PEN_MAX 8

#ifdef __amigaos3__
// Colours Workbench puts in pens 4-7 for "Set MagicWB colours"
static const ULONG magicwb_colours[4][3] = {{0x79797979, 0x79797979, 0x79797979},
											{0xaeaeaeae, 0xaeaeaeae, 0xaeaeaeae},
											{0xaaaaaaaa, 0x92929292, 0x7d7d7d7d},
											{0xffffffff, 0xaaaaaaaa, 0x96969696}};

// Is "Set MagicWB colours" on in the Workbench preferences?
static BOOL magicwb_pens_wanted(void)
{
	struct WorkbenchExtendedPrefs prefs;
	APTR iff;
	long size = 0;

	if (!(iff = IFFOpen("ENV:Sys/Workbench.prefs", IFF_READ, ID_PREF)))
		return FALSE;
	if (IFFNextChunk(iff, ID_WBNC) == ID_WBNC)
		size = IFFReadChunkBytes(iff, &prefs, sizeof(prefs));
	IFFClose(iff);

	// Older prefs files stop before wbe_LockPens
	return (BOOL)(size >= (long)(offsetof(struct WorkbenchExtendedPrefs, wbe_LockPens) + sizeof(prefs.wbe_LockPens)) &&
				  prefs.wbe_LockPens);
}
#endif

// Load screen palette
void load_screen_palette(void)
{
	short num, pen;

	// Get number of colours in screen
	num = 1 << GUI->screen->RastPort.BitMap->Depth;

	// Load base 4 colours
	for (pen = 0; pen < 4; pen++)
	{
		if (((struct Library *)GfxBase)->lib_Version >= 39)
		{
			SetRGB32(&GUI->screen->ViewPort,
					 pen,
					 environment->env->palette[pen * 3 + 1],
					 environment->env->palette[pen * 3 + 2],
					 environment->env->palette[pen * 3 + 3]);
		}
		else
		{
			SetRGB4(&GUI->screen->ViewPort,
					pen,
					(environment->env->palette[pen * 3 + 1] >> 28) & 0xf,
					(environment->env->palette[pen * 3 + 2] >> 28) & 0xf,
					(environment->env->palette[pen * 3 + 3] >> 28) & 0xf);
		}
	}

	// Load top 4 colours (under 39)
	if (((struct Library *)GfxBase)->lib_Version >= 39 && num > 4)
	{
		for (pen = 0; pen < 4; pen++)
		{
			SetRGB32(&GUI->screen->ViewPort,
					 num - pen - 1,
					 environment->env->palette[(pen + 4) * 3 + 1],
					 environment->env->palette[(pen + 4) * 3 + 2],
					 environment->env->palette[(pen + 4) * 3 + 3]);
		}
	}

	// Under 37, load custom colours
	if (((struct Library *)GfxBase)->lib_Version < 39 && num > 4)
	{
		// Only maximum of configured colours
		num -= 4;
		if (num > environment->env->palette_count)
			num = environment->env->palette_count;
		if (num > PEN_MAX)
			num = PEN_MAX;

		// Load colours
		for (pen = 0; pen < num; pen++)
		{
			SetRGB4(&GUI->screen->ViewPort,
					pen + 4,
					(environment->env->palette[(pen + 8) * 3 + 1] >> 28) & 0xf,
					(environment->env->palette[(pen + 8) * 3 + 2] >> 28) & 0xf,
					(environment->env->palette[(pen + 8) * 3 + 3] >> 28) & 0xf);
		}
	}
}

// Allocate palette
void get_colour_table()
{
	short pen;
	int newpen = 0;

	// Already got palette?
	if (GUI->flags & GUIF_GOT_PALETTE)
		return;
	GUI->flags |= GUIF_GOT_PALETTE;

	// Initialise pens
	for (pen = 0; pen < 16; pen++)
		GUI->pens[pen] = pen;
	GUI->pen_alloc = 0;

	// Using 37?
	if (((struct Library *)GfxBase)->lib_Version < 39)
	{
		// On our own screen, pens go from 4 onwards
		if (GUI->screen)
		{
			// Pens go from 4 onwards
			for (pen = 0; pen < 16; pen++)
				GUI->pens[pen] = pen + 4;
			GUI->pen_alloc = 0xff;
		}
		return;
	}

	// On our own screen?
	if (GUI->screen)
	{
		short top;

		// Get number of colours in display
		top = (1 << GUI->screen->RastPort.BitMap->Depth) - 1;

		// More than 4 colours?
		if (top > 4)
		{
			// Allocate OS colours
			for (pen = 0; pen < 4; pen++)
			{
				// Get top four palette colours so they won't be used below
				GUI->pens[12 + pen] = newpen = ObtainPen(GUI->screen_pointer->ViewPort.ColorMap,
														 top - pen,
														 environment->env->palette[(pen + 4) * 3 + 1],
														 environment->env->palette[(pen + 4) * 3 + 2],
														 environment->env->palette[(pen + 4) * 3 + 3],
														 0);
				if (newpen != -1)
				{
					GUI->pen_alloc |= 1 << (12 + pen);
				}
			}
		}
	}

#ifdef __amigaos3__
	// Workbench locks pens 4-7 to the MagicWB colours when its prefs ask for it.
	// Do the same on its screen when we replace Workbench, before our own pens
	// are allocated. While Workbench runs it holds these pens and ObtainPen() fails.
	if (!GUI->screen && (GUI->screen_pointer->Flags & SCREENTYPE) == WBENCHSCREEN &&
		GUI->screen_pointer->ViewPort.ColorMap->Count >= 8 && magicwb_pens_wanted())
	{
		// Whether we or Workbench hold them, pens 4-7 have the MagicWB colours; draw icons in them
		GUI->flags2 |= GUIF2_MAGICWB_PENS;

		for (pen = 0; pen < 4; pen++)
		{
			if (ObtainPen(GUI->screen_pointer->ViewPort.ColorMap,
						  pen + 4,
						  magicwb_colours[pen][0],
						  magicwb_colours[pen][1],
						  magicwb_colours[pen][2],
						  0) != -1)
				GUI->magicwb_pen_alloc |= 1 << pen;
		}
	}
#endif

	// Go through user pens
	for (pen = 0; pen < environment->env->palette_count; pen++)
	{
		// Not already allocated?
		if (!(GUI->pen_alloc & (1 << pen)))
		{
			// Try to allocate a pen
			GUI->pens[pen] = newpen = ObtainBestPen(GUI->screen_pointer->ViewPort.ColorMap,
													environment->env->palette[(pen + 8) * 3 + 1],
													environment->env->palette[(pen + 8) * 3 + 2],
													environment->env->palette[(pen + 8) * 3 + 3],
													OBP_Precision,
													PRECISION_EXACT,
													OBP_FailIfBad,
													TRUE,
													TAG_END);
			if (newpen == -1)
			{
				// Failed to allocate, find closest match
				GUI->pens[pen] = FindColor(GUI->screen_pointer->ViewPort.ColorMap,
										   environment->env->palette[(pen + 8) * 3 + 1],
										   environment->env->palette[(pen + 8) * 3 + 2],
										   environment->env->palette[(pen + 8) * 3 + 3],
										   -1);
			}

			// Otherwise, we have an exclusive pen
			else
			{
				// Set flag to say we allocated it
				GUI->pen_alloc |= 1 << pen;
			}
		}
	}
}

// Free allocated colours
void free_colour_table()
{
	// Haven't got palette?
	if (!(GUI->flags & GUIF_GOT_PALETTE))
		return;
	GUI->flags &= ~GUIF_GOT_PALETTE;

	// Only 39+
	if (((struct Library *)GfxBase)->lib_Version >= 39)
	{
		short pen;

		// Go through pens
		for (pen = 0; pen < 16; pen++)
		{
			// Was this pen allocated?
			if (GUI->pen_alloc & (1 << pen))
			{
				// Free it
				ReleasePen(GUI->screen_pointer->ViewPort.ColorMap, GUI->pens[pen]);
			}
		}

#ifdef __amigaos3__
		// Free the MagicWB pens
		for (pen = 0; pen < 4; pen++)
		{
			if (GUI->magicwb_pen_alloc & (1 << pen))
				ReleasePen(GUI->screen_pointer->ViewPort.ColorMap, pen + 4);
		}
		GUI->magicwb_pen_alloc = 0;
#endif
	}

	// Clear allocation flags
	GUI->pen_alloc = 0;
	GUI->flags2 &= ~GUIF2_MAGICWB_PENS;
}

// Should 8 colour icons draw colours 4-7 in the last four pens? Not when the
// Workbench screen keeps the MagicWB colours in pens 4-7, as Workbench draws them there
BOOL remap_icon_images(void)
{
	return (BOOL)(!(environment->env->desktop_flags & DESKTOPF_NO_REMAP) && !(GUI->flags2 & GUIF2_MAGICWB_PENS));
}

// Retrieve custom pen; returns -1 if no exclusive pen is available
short GetCustomPen(short pen, short num, ColourSpec32 *spec)
{
	short flag, bestpen;

	// Doesn't work under 37
	if (((struct Library *)GfxBase)->lib_Version < 39)
		return -1;

	// Get the flag
	flag = pen + (num * CUST_PENS);

	// See if pen is defined, unless forcing the issue
	if (!spec && !(environment->env->env_ColourFlag & (1 << flag)))
		return -1;

	// Lock the custom pens
	GetSemaphore(&GUI->custom_pen_lock, SEMF_EXCLUSIVE, 0);

	// See if pen is already allocated
	if (GUI->custom_pen_alloc & (1 << flag))
	{
		// Increment count
		++GUI->custom_pen_count[pen][num];

		// Get pen
		bestpen = GUI->custom_pens[pen][num];
	}

	// Not allocated yet; try to allocate pen, in exclusive mode
	else if ((bestpen = ObtainPen(GUI->screen_pointer->ViewPort.ColorMap,
								  -1,
								  (spec) ? spec->cs_Red : environment->env->env_Colours[pen][num][0],
								  (spec) ? spec->cs_Green : environment->env->env_Colours[pen][num][1],
								  (spec) ? spec->cs_Blue : environment->env->env_Colours[pen][num][2],
								  PEN_EXCLUSIVE)) > -1)
	{
		// Mark pen as allocated
		GUI->custom_pen_alloc |= (1 << flag);

		// Set count to one
		GUI->custom_pen_count[pen][num] = 1;

		// Store pen
		GUI->custom_pens[pen][num] = bestpen;
	}

	// Unlock the custom pens
	FreeSemaphore(&GUI->custom_pen_lock);

	// Return pen
	return bestpen;
}

// Free a custom pen; restore sets the configured colour again if listers still use the pen
void FreeCustomPen(short pen, short num, BOOL restore)
{
	short flag;

	// Doesn't work under 37
	if (((struct Library *)GfxBase)->lib_Version < 39)
		return;

	// Get the flag
	flag = pen + (num * CUST_PENS);

	// Lock the custom pens
	GetSemaphore(&GUI->custom_pen_lock, SEMF_EXCLUSIVE, 0);

	// See if pen is allocated
	if (GUI->custom_pen_alloc & (1 << flag))
	{
		// Decrement count
		if ((--GUI->custom_pen_count[pen][num]) == 0)
		{
			// Count has reached zero, we can free the pen
			if (GUI->screen_pointer)
			{
				// Free the pen
				ReleasePen(GUI->screen_pointer->ViewPort.ColorMap, GUI->custom_pens[pen][num]);
			}

			// Clear allocation bit
			GUI->custom_pen_alloc &= ~(1 << flag);
		}

		// The editor may have changed the colour; put back the configured one
		else if (restore && GUI->screen_pointer)
		{
			SetRGB32(&GUI->screen_pointer->ViewPort,
					 GUI->custom_pens[pen][num],
					 environment->env->env_Colours[pen][num][0],
					 environment->env->env_Colours[pen][num][1],
					 environment->env->env_Colours[pen][num][2]);
		}
	}

	// Unlock the custom pens
	FreeSemaphore(&GUI->custom_pen_lock);
}

// Initialise custom pens
void InitCustomPens(void)
{
	short pen, num;
	short flag;

	// Doesn't work under 37
	if (((struct Library *)GfxBase)->lib_Version < 39)
		return;

	// Lock the custom pens
	GetSemaphore(&GUI->custom_pen_lock, SEMF_EXCLUSIVE, 0);

	// Go through pens, 2 for each
	for (pen = 0; pen < CUST_PENS; pen++)
		for (num = 0; num < 2; num++)
		{
			// Get the flag
			flag = pen + (num * CUST_PENS);

			// Is this pen allocated?
			if (GUI->custom_pen_alloc & (1 << flag))
			{
				// Set count to zero
				GUI->custom_pen_count[pen][num] = 0;

				// Is screen still open?
				if (GUI->screen_pointer)
				{
					// Free the pen
					ReleasePen(GUI->screen_pointer->ViewPort.ColorMap, GUI->custom_pens[pen][num]);
				}
			}
		}

	// Clear allocation flags
	GUI->custom_pen_alloc = 0;

	// Unlock the custom pens
	FreeSemaphore(&GUI->custom_pen_lock);
}

// Set the configured colours of the allocated custom pens
void UpdateCustomPens(void)
{
	short pen, num;

	// Doesn't work under 37
	if (((struct Library *)GfxBase)->lib_Version < 39 || !GUI->screen_pointer)
		return;

	// Lock the custom pens
	GetSemaphore(&GUI->custom_pen_lock, SEMF_EXCLUSIVE, 0);

	// Go through pens, 2 for each
	for (pen = 0; pen < CUST_PENS; pen++)
		for (num = 0; num < 2; num++)
		{
			// Is this pen allocated?
			if (GUI->custom_pen_alloc & (1 << (pen + (num * CUST_PENS))))
			{
				SetRGB32(&GUI->screen_pointer->ViewPort,
						 GUI->custom_pens[pen][num],
						 environment->env->env_Colours[pen][num][0],
						 environment->env->env_Colours[pen][num][1],
						 environment->env->env_Colours[pen][num][2]);
			}
		}

	// Unlock the custom pens
	FreeSemaphore(&GUI->custom_pen_lock);
}
