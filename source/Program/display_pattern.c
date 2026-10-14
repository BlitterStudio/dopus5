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

#define NUM_PATTERNS 3
#define DISPLAY_FIP_KNOWN_MASK \
	(PATF_LOCK | PATF_RANDOM | PATF_STRETCH | PATF_CENTER | PATF_TILE | PATF_FILL | 0x03000000 | 0x00030000)

static ULONG display_prepare_fastiprefs_flags(ULONG flags)
{
#ifdef __AROS__
	ULONG swapped = AROS_BE2LONG(flags);

	if ((swapped & ~DISPLAY_FIP_KNOWN_MASK) < (flags & ~DISPLAY_FIP_KNOWN_MASK))
		return swapped;
#endif

	return flags;
}

static BOOL display_wbpattern_prefs_valid(struct WBPatternPrefs *prefs, ULONG chunk_size)
{
	if (chunk_size < sizeof(struct WBPatternPrefs))
		return FALSE;
	if (prefs->wbp_Which > WBP_SCREEN)
		return FALSE;
	if (prefs->wbp_DataLength > chunk_size - sizeof(struct WBPatternPrefs))
		return FALSE;
	if ((prefs->wbp_Flags & WBPF_PATTERN) && prefs->wbp_Depth > MAXDEPTH)
		return FALSE;
	return TRUE;
}

static BOOL display_prepare_wbpattern_prefs(struct WBPatternPrefs *prefs, ULONG chunk_size, BOOL *swapped)
{
	*swapped = FALSE;
	if (display_wbpattern_prefs_valid(prefs, chunk_size))
		return TRUE;

#ifdef __AROS__
	{
		struct WBPatternPrefs fixed;
		short a;

		fixed = *prefs;
		for (a = 0; a < 4; a++)
			fixed.wbp_Reserved[a] = AROS_BE2LONG(fixed.wbp_Reserved[a]);
		fixed.wbp_Which = AROS_BE2WORD(fixed.wbp_Which);
		fixed.wbp_Flags = AROS_BE2WORD(fixed.wbp_Flags);
		fixed.wbp_DataLength = AROS_BE2WORD(fixed.wbp_DataLength);

		if (display_wbpattern_prefs_valid(&fixed, chunk_size))
		{
			*prefs = fixed;
			*swapped = TRUE;
			return TRUE;
		}
	}
#endif

	return FALSE;
}

static void display_prepare_wbpattern_data(struct WBPatternPrefs *prefs, char *data, BOOL swapped)
{
#ifdef __AROS__
	if (swapped && data && (prefs->wbp_Flags & WBPF_PATTERN))
	{
		UWORD *words = (UWORD *)data;
		ULONG count = prefs->wbp_DataLength / sizeof(UWORD);
		ULONG a;

		for (a = 0; a < count; a++)
			words[a] = AROS_BE2WORD(words[a]);
	}
#else
	(void)prefs;
	(void)data;
	(void)swapped;
#endif
}

// Get backdrop pattern
short display_get_pattern(BOOL use_custom)
{
	APTR iff;
	BPTR file;
	struct WBPatternPrefs pattern[NUM_PATTERNS];
	short a;
	char *data[NUM_PATTERNS], *filename;
	short change = 0;
	ULONG flags = 0;

	// Copy from system fields to custom fields (if custom field is empty, or custom flag is not set)
	for (a = 0; a < 4; a++)
	{
		if (!GUI->env_BackgroundPic[a][0] || !use_custom)
		{
			strcpy(GUI->env_BackgroundPic[a], environment->env->env_BackgroundPic[a]);
			GUI->env_BackgroundFlags[a] = environment->env->env_BackgroundFlags[a];
			GUI->env_BackgroundBorderColour[a] = environment->env->env_BackgroundBorderColour[a];
		}
	}

	// Pattern disabled?
	if (environment->env->display_options & DISPOPTF_NO_BACKDROP)
	{
		// No main backdrop to wait for
		GUI->pattern_main_pending = FALSE;

		// Check both patterns are invalid
		for (a = 0; a < NUM_PATTERNS; a++)
		{
			// Is pattern valid?
			if (GUI->pattern[a].valid)
			{
				// Clear existing pattern
				display_update_pattern(a, 0);

				// Free pattern data
				FreeVec(GUI->pattern[a].data);
				GUI->pattern[a].data = 0;

				// Free pattern
				FreePattern(&GUI->pattern[a]);
			}
		}

		return 0;
	}

	// Use pattern prefs?
	if (environment->env->display_options & DISPOPTF_USE_WBPATTERN)
	{
		// End pattern notification
		stop_file_notify(GUI->pattern_notify);

		// Get filename

		filename = (((struct Library *)IntuitionBase)->lib_Version < 39) ? "env:sys/wb.pat" : "env:sys/wbpattern.prefs";

		// User-specified file?
		if (environment->env->backdrop_prefs[0])
		{
			BPTR lock;

			// Does file exist?
			if ((lock = Lock(environment->env->backdrop_prefs, ACCESS_READ)))
			{
				// Yep
				UnLock(lock);
				filename = environment->env->backdrop_prefs;
			}
		}

		// Initialise pattern and data pointers
		for (a = 0; a < NUM_PATTERNS; a++)
		{
			pattern[a] = GUI->pattern[a].prefs;
			data[a] = 0;
		}

		// Try to open wbpattern prefs file
		if (((struct Library *)IntuitionBase)->lib_Version >= 39 && (iff = IFFOpen(filename, IFF_READ, ID_PREF)))
		{
			ULONG id;

			// Scan for chunks we know and love
			while ((id = IFFNextChunk(iff, 0)))
			{
				// FastIPrefs FIP0 chunk?
				if (id == ID_FIP0)
				{
					// Get flags
					if (IFFReadChunkBytes(iff, &flags, sizeof(flags)) == sizeof(flags))
						flags = display_prepare_fastiprefs_flags(flags);
				}

				// PTRN chunk?
				else if (id == ID_PTRN)
				{
					struct WBPatternPrefs *prefs;
					LONG chunk_size;
					BOOL swapped;

					// Allocate space for chunk
					chunk_size = IFFChunkSize(iff);
					if (chunk_size >= sizeof(struct WBPatternPrefs) && (prefs = AllocVec(chunk_size, MEMF_CLEAR)))
					{
						// Read chunk
						if (IFFReadChunkBytes(iff, prefs, chunk_size) != chunk_size)
						{
							FreeVec(prefs);
							continue;
						}

						// Pattern we can handle?
						if (display_prepare_wbpattern_prefs(prefs, chunk_size, &swapped) &&
							(prefs->wbp_Which == WBP_ROOT || prefs->wbp_Which == WBP_DRAWER))
						{
							// Copy data
							CopyMem((char *)prefs, (char *)&pattern[prefs->wbp_Which], sizeof(struct WBPatternPrefs));

							// Allocate pattern data
							if (prefs->wbp_DataLength > 0 &&
								(data[prefs->wbp_Which] = AllocVec(prefs->wbp_DataLength + 1, MEMF_CLEAR)))
							{
								// Copy pattern data
								CopyMem((char *)(prefs + 1), data[prefs->wbp_Which], prefs->wbp_DataLength);
								display_prepare_wbpattern_data(prefs, data[prefs->wbp_Which], swapped);
							}
						}

						// Free chunk
						FreeVec(prefs);
					}
				}
			}

			// Close iff file
			IFFClose(iff);
		}

		// Couldn't find it, try for wb.pat
		else if (((struct Library *)IntuitionBase)->lib_Version < 39 && (file = Open(filename, MODE_OLDFILE)))
		{
			// Allocate pattern data
			if ((data[PATTERN_MAIN] = AllocVec(96, MEMF_CLEAR)))
			{
				// Skip forwards
				Seek(file, 20, OFFSET_CURRENT);

				// Read pattern
				if ((Read(file, data[PATTERN_MAIN], 96)) < 96)
				{
					FreeVec(data[PATTERN_MAIN]);
					data[PATTERN_MAIN] = 0;
				}

				// Fill out WBPatternPrefs
				else
				{
					pattern[PATTERN_MAIN].wbp_Flags = WBPF_PATTERN;
					pattern[PATTERN_MAIN].wbp_Depth = GUI->screen_pointer->RastPort.BitMap->Depth;
					if (pattern[PATTERN_MAIN].wbp_Depth > MAXDEPTH)
						pattern[PATTERN_MAIN].wbp_Depth = MAXDEPTH;
				}
			}

			// Close file
			Close(file);
		}

		// Start notification of wbpattern prefs
		GUI->pattern_notify = start_file_notify(filename, NOTIFY_PATTERN_CHANGED, GUI->appmsg_port);
	}

	// Otherwise, under 39 user-defined pictures
	else if (((struct Library *)GfxBase)->lib_Version >= 39)
	{
		short pat;

		// Do both patterns
		for (pat = 0; pat < NUM_PATTERNS; pat++)
		{
			// Got valid picture?
			if (GUI->env_BackgroundPic[pat][0])
			{
				// Fill out WBPatternPrefs
				pattern[pat].wbp_Flags = 0;
				pattern[pat].wbp_Which = pat;
				pattern[pat].wbp_DataLength = strlen(GUI->env_BackgroundPic[pat]);
				if ((data[pat] = AllocVec(pattern[pat].wbp_DataLength + 1, 0)))
					strcpy(data[pat], GUI->env_BackgroundPic[pat]);
			}

			// Set to no pattern
			else
			{
				// Fill out WBPatternPrefs
				pattern[pat].wbp_Flags = WBPF_PATTERN;
				pattern[pat].wbp_DataLength = 0;
				data[pat] = 0;
			}
		}
	}

	// Otherwise, default to no pattern
	else
	{
		short pat;

		// Do both patterns
		for (pat = 0; pat < NUM_PATTERNS; pat++)
		{
			// Fill out WBPatternPrefs
			pattern[pat].wbp_Flags = WBPF_PATTERN;
			pattern[pat].wbp_DataLength = 0;
			data[pat] = 0;
		}
	}

	// Go through both patterns
	for (a = 0; a < NUM_PATTERNS; a++)
	{
		BOOL diff = 0;
		short precision = 0;

		// Using Opus settings?
		if (!(environment->env->display_options & DISPOPTF_USE_WBPATTERN))
		{
			// Get precision
			if (GUI->env_BackgroundFlags[a] & ENVBF_PRECISION_NONE)
				precision = -1;
			else if (GUI->env_BackgroundFlags[a] & ENVBF_PRECISION_GUI)
				precision = 2;
			else if (GUI->env_BackgroundFlags[a] & ENVBF_PRECISION_ICON)
				precision = 1;
			else if (GUI->env_BackgroundFlags[a] & ENVBF_PRECISION_EXACT)
				precision = 3;
		}

		// Maybe set in prefs
		else
		{
			// Get precision
			precision = (a == 0) ? PATF_WB_MODE(flags) : PATF_LISTER_MODE(flags);
		}

		// Valid data?
		if (data[a])
		{
			// No last pattern?
			if (!GUI->pattern[a].data)
			{
				diff = 1;
			}

			// Changed from pattern to picture or vice versa?
			else if ((pattern[a].wbp_Flags & WBPF_PATTERN && !(GUI->pattern[a].prefs.wbp_Flags & WBPF_PATTERN)) ||
					 (!(pattern[a].wbp_Flags & WBPF_PATTERN) && GUI->pattern[a].prefs.wbp_Flags & WBPF_PATTERN))
			{
				diff = 1;
			}

			// Pattern pattern?
			else if (pattern[a].wbp_Flags & WBPF_PATTERN)
			{
				// Different depth?
				if (pattern[a].wbp_Depth != GUI->pattern[a].prefs.wbp_Depth)
					diff = 1;

				// Has data changed from the last one?
				else
				{
					short num, d;

					// Get amount of data
					num = PAT_HEIGHT * pattern[a].wbp_Depth;

					// Compare data
					for (d = 0; d < num; d++)
					{
						if (((UWORD *)data[a])[d] != ((UWORD *)GUI->pattern[a].data)[d])
						{
							diff = 1;
							break;
						}
					}
				}
			}

			// Picture?
			else
			{
				// Different name for picture, or it's random?
				if (strcmp(data[a], GUI->pattern[a].data) != 0 || GUI->pattern[a].random)
				{
					diff = 1;
				}

				// Precision changed?
				else if (precision != GUI->pattern[a].precision)
				{
					diff = 1;
				}

				// Stretching changed?
				else if (a == 0 && (((GUI->env_BackgroundFlags[a] & ENVBF_STRETCH_PIC) &&
									 !(GUI->pattern[a].flags & PATF_STRETCH)) ||
									!((GUI->env_BackgroundFlags[a] & ENVBF_STRETCH_PIC) &&
									  (GUI->pattern[a].flags & PATF_STRETCH))))
				{
					diff = 1;
				}

				// Centering changed?
				else if (((GUI->env_BackgroundFlags[a] & ENVBF_CENTER_PIC) && !(GUI->pattern[a].flags & PATF_CENTER)) ||
						 !((GUI->env_BackgroundFlags[a] & ENVBF_CENTER_PIC) && (GUI->pattern[a].flags & PATF_CENTER)))
				{
					diff = 2;
				}
			}
		}

		// Had last data?
		else if (GUI->pattern[a].data)
			diff = 1;

		if (diff != 1)
		{
			// Border pen changed?
			if ((GUI->env_BackgroundFlags[a] & ENVBF_USE_COLOUR && GUI->pattern[a].border_pen == 0) ||
				(!(GUI->env_BackgroundFlags[a] & ENVBF_USE_COLOUR) && GUI->pattern[a].border_pen != 0))
			{
				diff = 3;
			}
			else if (GUI->pattern[a].border_pen != 0 && GUI->env_BackgroundFlags[a] & ENVBF_USE_COLOUR)
			{
				ULONG col[3];
				GetPalette32(&GUI->screen_pointer->ViewPort,
							 col,
							 1,
							 (GUI->pattern[a].border_pen == -1) ? 0 : GUI->pattern[a].border_pen);
				if (ENVBF_COL_R_GET(col[0]) != ENVBF_COL_R_GET(GUI->env_BackgroundBorderColour[a]) ||
					ENVBF_COL_R_GET(col[1]) != ENVBF_COL_G_GET(GUI->env_BackgroundBorderColour[a]) ||
					ENVBF_COL_R_GET(col[2]) != ENVBF_COL_B_GET(GUI->env_BackgroundBorderColour[a]))
				{
					diff = 3;
				}
			}
		}

		// Has it changed?
		if (diff)
		{
			// Existing pattern valid?
			if (GUI->pattern[a].valid)
			{
				// Clear existing patterns
				display_update_pattern(a, 0);
			}

			// Change more than centering?
			if (diff == 1)
			{
				// Free existing pattern
				FreePattern(&GUI->pattern[a]);
				FreeVec(GUI->pattern[a].data);

				// Copy pattern and data pointer
				GUI->pattern[a].prefs = pattern[a];
				GUI->pattern[a].data = data[a];

				// Stretch?
				if (a == 0 && GUI->env_BackgroundFlags[a] & ENVBF_STRETCH_PIC)
				{
					flags |= PATF_STRETCH;
					flags &= ~PATF_CENTER;
				}
				else
				{
					flags &= ~PATF_STRETCH;
					if (!(environment->env->display_options & DISPOPTF_USE_WBPATTERN) &&
						GUI->env_BackgroundFlags[a] & ENVBF_CENTER_PIC)
						flags |= PATF_CENTER;
					else
						flags &= ~PATF_CENTER;
				}

				// Set flags and precision
				GUI->pattern[a].flags = flags;
				GUI->pattern[a].precision = precision;

				// Initialise pattern
				GetPattern(&GUI->pattern[a],
						   GUI->screen_pointer,
						   GUI->env_BackgroundBorderColour[a] | (GUI->env_BackgroundFlags[a] & ENVBF_USE_COLOUR));
			}

			// Change colour
			else if (diff == 3)
			{
				// Initialise pattern colour
				GetPatternBorder(&GUI->pattern[a],
								 GUI->screen_pointer,
								 GUI->env_BackgroundBorderColour[a] | (GUI->env_BackgroundFlags[a] & ENVBF_USE_COLOUR));
			}

			// Centering changed
			else
			{
				// Environment flags set for centering?
				if (!(environment->env->display_options & DISPOPTF_USE_WBPATTERN) &&
					GUI->env_BackgroundFlags[a] & ENVBF_CENTER_PIC)
					GUI->pattern[a].flags |= PATF_CENTER;
				else
					GUI->pattern[a].flags &= ~PATF_CENTER;
			}

			// Install new pattern
			display_update_pattern(a, 1);
		}

		// If not, free data
		else
			FreeVec(data[a]);

		// Main backdrop settled; display_open() can stop waiting for it
		if (a == PATTERN_MAIN)
			GUI->pattern_main_pending = FALSE;
	}

	return change;
}

// Free backdrop pattern
void display_free_pattern(void)
{
	short a;

	// Go through both patterns
	for (a = PATTERN_MAIN; a <= PATTERN_REQ; a++)
	{
		// Free pattern data
		FreeVec(GUI->pattern[a].data);
		GUI->pattern[a].data = 0;

		// Free pattern
		FreePattern(&GUI->pattern[a]);
	}
}

// Signal with pattern update
void display_update_pattern(short which, short valid)
{
	// Main pattern changed?
	if (which == PATTERN_MAIN)
	{
		// Send refresh command
		IPC_Command(&main_ipc, LISTER_BACKFILL_CHANGE, valid, 0, 0, REPLY_NO_PORT);
	}

	// Redraw listers and groups if they need it
	else if (which == PATTERN_LISTER)
	{
		IPC_ListCommand(&GUI->lister_list, LISTER_BACKFILL_CHANGE, valid, 0, TRUE);
		IPC_ListCommand(&GUI->group_list, LISTER_BACKFILL_CHANGE, valid, 0, TRUE);
	}
}
