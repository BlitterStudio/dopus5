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

// LOADFONTS internal function
DOPUS_FUNC(function_loadfonts)
{
	FunctionEntry *entry;
	ULONG change = 0;
	BOOL run_prefs = 0;

	// Get first entry
	if ((entry = function_get_entry(handle)))
	{
		APTR iff;

		// Build full name
		function_build_source(handle, entry, handle->work_buffer);

		// Open IFF file
		if ((iff = IFFOpen(handle->work_buffer, IFF_READ, ID_PREF)))
		{
			ULONG chunk;
			while ((chunk = IFFNextChunk(iff, ID_FONT)) == ID_FONT)
			{
				struct FontPrefs fonts;
				BOOL got_font = FALSE;

#ifdef __AROS__
				// FONT chunks contain the 32-bit, 156-byte FontPrefs layout.
				UBYTE font_data[156];

				if (IFFChunkSize(iff) >= sizeof(font_data) &&
					IFFReadChunkBytes(iff, font_data, sizeof(font_data)) == sizeof(font_data))
				{
					memset(&fonts, 0, sizeof(fonts));
					fonts.fp_Type = ((UWORD)font_data[14] << 8) | font_data[15];
					fonts.fp_FrontPen = font_data[16];
					fonts.fp_BackPen = font_data[17];
					fonts.fp_DrawMode = font_data[18];
					fonts.fp_TextAttr.ta_YSize = ((UWORD)font_data[24] << 8) | font_data[25];
					CopyMem(font_data + 28, fonts.fp_Name, sizeof(fonts.fp_Name));
					got_font = TRUE;
				}
#else
				if (IFFReadChunkBytes(iff, &fonts, sizeof(struct FontPrefs)))
					got_font = TRUE;
#endif
				if (got_font)
				{
					// Icon label font?
					if (fonts.fp_Type == FP_WBFONT)
					{
						// Store new font
						stccpy(environment->env->font_name[FONT_ICONS], fonts.fp_Name, 40);
						stccpy(environment->env->font_name[FONT_WINDOW_ICONS], fonts.fp_Name, 40);
						environment->env->font_size[FONT_ICONS] = fonts.fp_TextAttr.ta_YSize;
						environment->env->font_size[FONT_WINDOW_ICONS] = fonts.fp_TextAttr.ta_YSize;

						// Store colours/style
						environment->env->icon_fpen = fonts.fp_FrontPen;
						environment->env->icon_bpen = fonts.fp_BackPen;
						environment->env->icon_style = DOPUS_FONTPREFS_ICON_STYLE(&fonts);
						environment->env->iconw_fpen = fonts.fp_FrontPen;
						environment->env->iconw_bpen = fonts.fp_BackPen;
						environment->env->iconw_style = DOPUS_FONTPREFS_ICON_STYLE(&fonts);
						change |= CONFIG_CHANGE_ICON_FONT | CONFIG_CHANGE_ICON_FONT_WINDOWS;
					}

					// Screen font (only on own screen)
					else if (fonts.fp_Type == FP_SCREENFONT)
					{
						// On own screen?
						if (GUI->screen)
						{
							// See if font is different
							if (stricmp(environment->env->font_name[FONT_SCREEN], fonts.fp_Name) != 0 ||
								environment->env->font_size[FONT_SCREEN] != fonts.fp_TextAttr.ta_YSize)
							{
								// Store new font
								stccpy(environment->env->font_name[FONT_SCREEN], fonts.fp_Name, 40);
								environment->env->font_size[FONT_SCREEN] = fonts.fp_TextAttr.ta_YSize;
								change |= CONFIG_CHANGE_DISPLAY;
							}
						}

						// On Workbench screen
						else if (GUI->screen_pointer)
						{
							// See if font is different
							if (stricmp(GUI->screen_pointer->RastPort.Font->tf_Message.mn_Node.ln_Name,
										fonts.fp_Name) != 0 ||
								GUI->screen_pointer->RastPort.Font->tf_YSize != fonts.fp_TextAttr.ta_YSize)
							{
								run_prefs = 1;
							}
						}
					}
				}
			}
			IFFClose(iff);
		}

		// End this entry
		function_end_entry(handle, entry, 1);
	}

	// Change font?
	if (change)
	{
		// Run Font Prefs program?
		if (run_prefs)
		{
			// Build command
			lsprintf(handle->work_buffer + 256, "sys:prefs/font from %s use", handle->work_buffer);
			CLI_Launch(handle->work_buffer + 256,
					   GUI->screen_pointer,
					   Lock(handle->source_path, ACCESS_READ),
					   Open("nil:", MODE_OLDFILE),
					   0,
					   0,
					   0);
		}

		// Send change command
		else
			rexx_send_reset(change, 0);
	}
	return 0;
}
