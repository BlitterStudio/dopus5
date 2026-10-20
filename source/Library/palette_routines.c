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
#include "config.h"
#include <Program/dopus_config.h>

// Load a 32 bit palette, even under <39
void LIBFUNC L_LoadPalette32(REG(a0, struct ViewPort *vp), REG(a1, ULONG *palette))
{
	// Under 39 pass to Gfx library
	if (((struct Library *)GfxBase)->lib_Version >= 39)
	{
		LoadRGB32(vp, palette);
	}

	// Otherwise, convert to 4 bit
	else
	{
		unsigned short *backup_palette;
		short pen, numcols;

		// Get count
		numcols = *(unsigned short *)palette;
		++palette;

		// Allocate backup palette
		if ((backup_palette = AllocVec(numcols * sizeof(unsigned short), 0)))
		{
			// Convert to 4 bit colour
			for (pen = 0; pen < numcols; pen++)
			{
				backup_palette[pen] = ((*palette++) & 0xf0000000) >> 20;
				backup_palette[pen] |= ((*palette++) & 0xf0000000) >> 24;
				backup_palette[pen] |= ((*palette++) & 0xf0000000) >> 28;
			}

			// Load palette
			LoadRGB4(vp, backup_palette, numcols);

			// Free backup
			FreeVec(backup_palette);
		}
	}
}

// Get a 32 bit palette, even under <39
void LIBFUNC L_GetPalette32(REG(a0, struct ViewPort *vp),
							REG(a1, ULONG *palette),
							REG(d0, unsigned short count),
							REG(d1, short first))
{
	// If under 39+ we get it directly
	if (((struct Library *)GfxBase)->lib_Version >= 39)
		GetRGB32(vp->ColorMap, first, count, palette);

	// Otherwise, we have to convert from 4 bit colour
	else
	{
		unsigned short pen;
		ULONG colour;

		// Go through each pen
		for (pen = 0; pen < count; pen++)
		{
			// Get colour value of this pen
			colour = GetRGB4(vp->ColorMap, pen + first);

			// Convert from 4 to 32 bit colour
			*palette++ = (((colour >> 8) & 0xf) << 28) | 0x0fffffff;
			*palette++ = (((colour >> 4) & 0xf) << 28) | 0x0fffffff;
			*palette++ = ((colour & 0xf) << 28) | 0x0fffffff;
		}
	}
}
