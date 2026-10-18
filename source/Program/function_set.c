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

enum {
	SET_OUTPUT,
	SET_SORT,
	SET_DISPLAY,
	SET_SEPARATE,
	SET_HIDE,
	SET_SHOW,
	SET_FLAGS,
	SET_TOOLBAR,
	SET_MODE,
	SET_SOURCE,
	SET_DEST,
	SET_OFF,
	SET_LOCK,
	SET_NOTIFY,
	SET_LABEL_COLOUR,
	SET_DUAL,
};

enum {
	MODE_NAME,
	MODE_ICON,
};

char *set_keys[] = {"output",
					"sort",
					"display",
					"separate",
					"hide",
					"show",
					"flags",
					"toolbar",
					"mode",
					"source",
					"dest",
					"off",
					"lock",
					"notify",
					"labelcolour",
					"dual",
					0};

extern char *labelcol_keys[];
extern char *mode_keys[];

char *dual_mode_keys[] = {"off", "on", "toggle", "0", "1", 0};

// SET function
DOPUS_FUNC(function_set)
{
	Lister *lister;
	short set, refresh = 0;
	char *set_args = args;

	// Get key word
	rexx_skip_space(&set_args);
	if (strnicmp(set_args, "dual=", 5) == 0)
	{
		set = SET_DUAL;
		args = set_args + 5;
	}
	else if ((set = rexx_match_keyword(&args, set_keys, 0)) == -1)
		return 0;

	// Get current lister
	lister = function_lister_current(&handle->source_paths);

	// Set what?
	switch (set)
	{
	case SET_OUTPUT:

		// New output handle
		stccpy(handle->output_handle, args, 79);
		break;

	case SET_SORT:

		// Must have a lister
		if (!lister)
			break;

		// Set sort format
		refresh = !(rexx_set_format(RXCMD_SET, RXCMD_SORT, &lister->format, args));
		break;

	case SET_DISPLAY:

		// Must have a lister
		if (!lister)
			break;

		// Set display format
		refresh = !(rexx_set_format(RXCMD_SET, RXCMD_DISPLAY, &lister->format, args));
		break;

	case SET_SEPARATE:

		// Must have a lister
		if (!lister)
			break;

		// Set separation
		refresh = !(rexx_set_format(RXCMD_SET, RXCMD_SEPARATE, &lister->format, args));
		break;

	case SET_SHOW:

		// Must have a lister
		if (!lister)
			break;

		// Set show pattern
		refresh = !(rexx_set_format(RXCMD_SET, RXCMD_SHOW, &lister->format, args));
		break;

	case SET_HIDE:

		// Must have a lister
		if (!lister)
			break;

		// Set hide pattern
		refresh = !(rexx_set_format(RXCMD_SET, RXCMD_HIDE, &lister->format, args));
		break;

	case SET_FLAGS: {
		char *ptr;

		// Must have a lister
		if (!lister)
			break;

		// Go through arg string, see if it's a + or a - operation
		for (ptr = args; *ptr && *ptr != '+' && *ptr != '-' && *ptr != '/'; ptr++)
			;

		// Explicit set?
		if (!*ptr)
		{
			// Clear flags initially
			lister->format.sort.sort_flags &= ~SORT_REVERSE;
			lister->format.flags &= ~(LFORMATF_REJECT_ICONS | LFORMATF_HIDDEN_BIT);
		}

		// Set flags
		refresh = !(rexx_set_format(RXCMD_SET, RXCMD_FLAGS, &lister->format, args));
	}
	break;

	case SET_TOOLBAR:

		// Must have a lister
		if (!lister)
			break;

		// Set new toolbar
		rexx_skip_space(&args);
		lister_new_toolbar(lister, args, 0);
		break;

	case SET_MODE:

		// Must have a lister
		if (lister)
		{
			// Set mode
			rexx_set_lister_mode(lister, args);
		}
		break;

	case SET_SOURCE:

		// Got lister?
		if (lister)
		{
			// Set "stored source" flag
			lister->flags |= LISTERF_STORED_SOURCE;
			lister->flags &= ~LISTERF_STORED_DEST;
		}
		break;

	case SET_DEST:

		// Got lister?
		if (lister)
		{
			// Set "stored dest" flag
			lister->flags |= LISTERF_STORED_DEST;
			lister->flags &= ~LISTERF_STORED_SOURCE;
		}
		break;

	case SET_OFF:

		// Got lister?
		if (lister)
		{
			// Clear "stored" flags
			lister->flags &= ~(LISTERF_STORED_SOURCE | LISTERF_STORED_DEST);
		}
		break;

	case SET_LOCK:

		// Got lister?
		if (lister)
		{
			short on;

			// On or off?
			on = rexx_match_keyword(&args, on_off_strings, 0);

			// Turn off?
			if (on == 0 || on == 2)
				lister->flags &= ~LISTERF_SOURCEDEST_LOCK;

			// Turn on
			else
				lister->flags |= LISTERF_SOURCEDEST_LOCK;
		}
		break;

	case SET_NOTIFY: {
		short on;

		// On or off?
		on = rexx_match_keyword(&args, on_off_strings, 0);

		// Turn off?
		if (on == 0 || on == 2)
			GUI->flags |= GUIF_NO_NOTIFY;

		// Turn on
		else
			GUI->flags &= ~GUIF_NO_NOTIFY;
	}
	break;

	case SET_LABEL_COLOUR: {
		short which, fg, bg, mode;

		// Get which one
		if ((which = rexx_match_keyword(&args, labelcol_keys, 0)) < 0)
			break;

		// Get foreground and background
		fg = rexx_parse_number(&args, 1, (which == 0) ? environment->env->icon_fpen : environment->env->iconw_fpen);
		bg = rexx_parse_number(&args, 0, (which == 0) ? environment->env->icon_bpen : environment->env->iconw_bpen);
		rexx_skip_space(&args);

		// Map colours
		if (GUI->screen_pointer->RastPort.BitMap->Depth > 2)
		{
			if (fg >= 4 && fg < 8)
				fg = 252 + (fg - 4);
			else if (fg >= 8)
				fg -= 4;
			if (bg >= 4 && bg < 8)
				bg = 252 + (bg - 4);
			else if (bg >= 8)
				bg -= 4;
		}

		// Get draw mode
		if ((mode = rexx_match_keyword(&args, labelcol_keys, 0)) < 2)
			mode = (which == 0) ? environment->env->icon_style : environment->env->iconw_style;
		else
			mode -= 2;

		// Set new values
		if (which == 0)
		{
			environment->env->icon_fpen = fg;
			environment->env->icon_bpen = bg;
			environment->env->icon_style = mode;
		}
		else
		{
			environment->env->iconw_fpen = fg;
			environment->env->iconw_bpen = bg;
			environment->env->iconw_style = mode;
		}

		// Reset the icons
		send_main_reset_cmd((which == 0) ? CONFIG_CHANGE_ICON_FONT : CONFIG_CHANGE_ICON_FONT_WINDOWS, 0, 0);
	}
	break;

	case SET_DUAL:

		// Got lister?
		if (lister)
		{
			short val, mode = LISTER_DUAL_TOGGLE;

			// Accept "Set DUAL=toggle" and "Set DUAL toggle"
			rexx_skip_space(&args);
			if (*args == '=')
			{
				++args;
				rexx_skip_space(&args);
			}
			val = rexx_match_keyword(&args, dual_mode_keys, 0);

			if (val == 0 || val == 3)
				mode = LISTER_DUAL_OFF;
			else if (val == 1 || val == 4)
				mode = LISTER_DUAL_ON;

			IPC_Command(lister->ipc, LISTER_DUAL, mode, 0, 0, 0);
		}
		break;
	}

	// Resort?
	if (refresh)
	{
		// Save format
		lister->user_format = lister->format;

		// Set flag to resort the panel the command was started from
		function_path_buffer(handle->source_paths.current)->flags &= ~DWF_SPECIAL_SORT;
		handle->result_flags |= FRESULTF_RESORT;
	}

	return 1;
}
