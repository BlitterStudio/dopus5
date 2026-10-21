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
#include "scripts.h"

void theme_expand_path(char *buf);

// Rexx DOpus command
BOOL rexx_dopus_cmd(struct RexxMsg *msg, short command, char *args)
{
	BOOL ret = 0, clear = 0;
	short id, val;
	char result[768];
	long rc;

	// Zero returns
	rc = 0;
	result[0] = 0;
	rexx_skip_space(&args);

	// Look at sub-command
	switch (command)
	{
	// Screen to back
	case RXCMD_BACK:

		// Got a screen?
		if (GUI->screen)
			ScreenToBack(GUI->screen);

		// Else a window
		else if (GUI->window)
			WindowToBack(GUI->window);
		break;

	// Screen to front
	case RXCMD_FRONT:

		// Got a screen?
		if (GUI->screen)
		{
			if (IntuitionBase->FirstScreen != GUI->screen)
				ScreenToFront(GUI->screen);
		}

		// Got a window?
		else if (GUI->window)
		{
			WindowToFront(GUI->window);
			ActivateWindow(GUI->window);
			if (IntuitionBase->FirstScreen != GUI->window->WScreen)
				ScreenToFront(GUI->window->WScreen);
		}
		break;

	// Get a string/request a choice
	case RXCMD_GETSTRING:
	case RXCMD_REQUEST: {
		RexxRequester *req;

		// Allocate requester argument packet
		if ((req = AllocVec(sizeof(RexxRequester) + strlen(args), MEMF_CLEAR)))
		{
			// Store message
			req->msg = msg;
			req->command = command;

			// Copy arguments
			strcpy(req->args, args);

			// Startup requester process
			if (!(misc_startup("dopus_rexx_request", REXX_REQUESTER, GUI->window, req, 0)))
				FreeVec(req);
			else
				ret = 1;
		}
	}
	break;

	// Query a file's type
	case RXCMD_GETFILETYPE: {
		char name[256], typename[60];
		short id;
		Cfg_Filetype *type;

		// Get filename
		rexx_parse_word(&args, name, 256);
		rexx_skip_space(&args);

		// Match ID keyword
		id = rexx_match_keyword(&args, getfiletype_keys, 0);

		// Try to match filetype
		if ((type = filetype_identify(name, FTTYPE_WANT_NAME, typename, 0)))
		{
			// Want ID?
			if (id == 0 && type->type.id[0])
				strcpy(typename, type->type.id);
		}

		// Set result string
		strcpy(result, typename);
	}
	break;

	// Run a script
	case RXCMD_SCRIPT: {
		char name[80], data[256];

		// Get script name and data
		rexx_parse_word(&args, name, 80);
		rexx_skip_space(&args);
		rexx_parse_word(&args, data, 256);

		// Run the script
		lsprintf(result, "%ld", RunScript_Name(name, data));
	}
	break;

	// Add/Remove a command trap
	case RXCMD_ADDTRAP:
	case RXCMD_REMTRAP: {
		char buf[40], handler[40], *comptr;
		short ret = RXERR_INVALID_NAME;

		// Get command
		rexx_parse_word(&args, buf, 40);
		rexx_skip_space(&args);

		// Get handler
		rexx_parse_word(&args, handler, 40);

		// Try and function command
		comptr = buf;
		if (*buf == '*')
			ret = 0;
		else if (function_find_internal(&comptr, 0))
			ret = 0;

		// Valid command?
		if (ret == 0)
		{
			// Add trap
			if (command == RXCMD_ADDTRAP)
			{
				char portname[40];

				// Unique port?
				rexx_skip_space(&args);
				if (rexx_match_keyword(&args, port_keys, 0) == 0)
				{
					rexx_skip_space(&args);
					rexx_parse_word(&args, portname, 40);
				}
				else
					portname[0] = 0;

				ret = AddFunctionTrap(buf, handler, portname);
			}

			// Remove trap
			else
			{
				ret = RemFunctionTrap(buf, handler);
			}
		}

		// Set result
		rc = ret;
	}
	break;

	// Version
	case RXCMD_VERSION: {
		extern short version_num;

		// Build version string.
		//
		// The documented format is "<version> <revision>". That worked while
		// the revision stayed <= 99, but once we started shipping 5.100 the
		// shipped ARexx scripts (and the old Nudel CDOpus/DOpusFuncs tools)
		// broke, because they do:
		//
		//     dopus version
		//     translate(result, '.', ' ') < 5.1218
		//
		// ARexx parses "5.100" as the decimal 5.1 (trailing-zero drop), so
		// 5.100 wrongly failed the "requires 5.5 or greater" gate. For
		// revisions >= 100 we clamp the reported revision at 99 and expose
		// the real revision as a third component, e.g. "5 99 100". That:
		//   - makes translate(result, '.', ' ') produce "5.99.100", which
		//     ARexx treats as a non-numeric string, falls back to string
		//     comparison, and "5.99.100" > "5.1218" so the gate passes.
		//   - still makes the left-padded decimal form that CDOpus-style
		//     tools build ("99...") compare greater than the required
		//     "1200000000" threshold, so those also pass.
		//   - keeps the real revision visible as the third token.
		if (version_num >= 100)
			lsprintf(result, "5 99 %ld", version_num);
		else
			lsprintf(result, "5 %ld", version_num);
	}
	break;

	// Screen information
	case RXCMD_SCREEN:

		// Valid screen?
		if (GUI->screen_pointer)
		{
			// Build screen info string
			lsprintf(result,
					 "%s %ld %ld %ld %ld %ld %ld",
					 get_our_pubscreen(),
					 GUI->screen_pointer->Width,
					 GUI->screen_pointer->Height,
					 GUI->screen_pointer->RastPort.BitMap->Depth,
					 GUI->screen_pointer->BarHeight,
					 environment->env->lister_width,
					 environment->env->lister_height);
		}

		// If not, set error
		else
			rc = RXERR_INVALID_QUERY;
		break;

	// Text reader
	case RXCMD_READ:
	case RXCMD_HEXREAD:
	case RXCMD_SMARTREAD:
	case RXCMD_ANSIREAD: {
		IPTR res;

		// Start reader
		res = (IPTR)rexx_read_file(command, args, msg);

		// Return reader IPC
		rexx_format_iptr(result, res);
	}
	break;

	// Error
	case RXCMD_ERROR: {
		short err, code;

		// Get error code
		code = rexx_parse_number(&args, 0, 0);

		// Go through table
		for (err = 0; rexx_error_table[err]; err += 2)
			if (rexx_error_table[err] == code)
				break;

		// Return error string
		strcpy(result, GetString(&locale, rexx_error_table[err + 1]));
	}
	break;

	// Add icon
	case RXCMD_ADDAPPICON:

		// Add AppIcon
		rexx_format_iptr(result, rexx_add_appicon(args, msg));
		break;

	// Remove icon
	case RXCMD_REMAPPICON:

		// Remove AppIcon
		rexx_rem_appthing(args, REXXAPP_ICON);
		break;

	// Change icon
	case RXCMD_SETAPPICON:

		// Set AppIcon
		rc = rexx_set_appicon(args, msg);
		break;

	// Progress indicator
	case RXCMD_PROGRESS: {
		IPTR value = 0;

		// Handle it
		if (rexx_lister_newprogress(0, args, &value))
		{
			// Value to return?
			if (value)
			{
				// Build return string
				rexx_format_iptr(result, (value == (IPTR)-1) ? 0 : value);
			}
		}

		// Set error
		else if (value)
			rc = RXERR_INVALID_SET;

		// Invalid handle
		else
			rc = RXERR_INVALID_HANDLE;
	}
	break;

	// Command
	case RXCMD_COMMAND:

		// Add a new command
		rexx_add_cmd(args);
		break;

	// Send rexx message to another port
	case RXCMD_SEND: {
		char port[40];

		// Get port name
		rexx_parse_word(&args, port, 40);
		rexx_skip_space(&args);

		// Valid port?
		if (port[0])
		{
			// Send message
			if (!(rexx_handler_msg(port, 0, 0, HA_String, 0, args, TAG_END)))
			{
				// Failed
				rc = RXERR_NO_MEMORY;
			}
		}
	}
	break;

	// Launch an ARexx script
	case RXCMD_RX:

		// Send command
		rexx_send_command(args, 0);
		break;

	// Check the desktop
	case RXCMD_CHECKDESKTOP:

		// Check desktop
		HookCheckDesktop(args);
		break;

	// Get desktop path
	case RXCMD_GETDESKTOP: {
		UWORD res;

		// Get desktop path
		res = HookGetDesktop(result);

		// Set RC explicitly
		rexx_set_var(msg, "DOPUSRC", 0, res, RX_LONG);
	}
	break;

	// Match desktop path
	case RXCMD_MATCHDESKTOP:
		lsprintf(result, "%ld", HookMatchDesktop(args));
		break;

	// Do the desktop popup
	case RXCMD_DESKTOPPOPUP: {
		short res;

		// Do the popup
		res = HookDesktopPopup(atoi(args));

		// Set return
		lsprintf(result, "%ld", res);
	}
	break;

	// Set
	case RXCMD_CLEAR:
		clear = 1;
	case RXCMD_SET:

		// Get set item
		id = rexx_get_command(&args);
		switch (id)
		{
		// Background
		case RXCMD_BACKGROUND: {
			char buf[256];
			short which = 0, custom = 0;
			UWORD flags;
			ULONG colour;

			// Get name
			if (clear)
				buf[0] = 0;
			else
				rexx_parse_word(&args, buf, 256);
			flags = environment->env->env_BackgroundFlags[0];
			colour = environment->env->env_BackgroundBorderColour[0];

			// Turn backgrounds on?
			if (stricmp(buf, "on") == 0)
			{
				environment->env->display_options &= ~DISPOPTF_NO_BACKDROP;
				environment->env->display_options &= ~DISPOPTF_USE_WBPATTERN;
				break;
			}

			// Or off
			else if (stricmp(buf, "off") == 0)
			{
				environment->env->display_options |= DISPOPTF_NO_BACKDROP;
				environment->env->display_options &= ~DISPOPTF_USE_WBPATTERN;
				break;
			}

			// Get keys
			rexx_skip_space(&args);
			while ((val = rexx_match_keyword(&args, background_key, 0)) != -1)
			{
				// Desktop/lister/requester
				if (val >= 0 && val <= 2)
				{
					which = val;
					flags = environment->env->env_BackgroundFlags[which];
					colour = environment->env->env_BackgroundBorderColour[which];
				}

				// Tile/center/stretch
				else if (val == 3)
					flags &= ~(ENVBF_CENTER_PIC | ENVBF_STRETCH_PIC);
				else if (val == 4)
				{
					flags |= ENVBF_CENTER_PIC;
					flags &= ~ENVBF_STRETCH_PIC;
				}
				else if (val == 5 && which == 0)
				{
					flags |= ENVBF_STRETCH_PIC;
					flags &= ~ENVBF_CENTER_PIC;
				}

				// Precision
				else if (val == 6)
				{
					// Get precision
					rexx_skip_space(&args);
					if ((val = rexx_match_keyword(&args, precision_key, 0)) > -1)
					{
						flags &= ~(ENVBF_PRECISION_NONE | ENVBF_PRECISION_GUI | ENVBF_PRECISION_ICON |
								   ENVBF_PRECISION_IMAGE | ENVBF_PRECISION_EXACT);
						flags |= 1 << (val + 1);
					}
				}

				// Custom
				else if (val == 7)
					custom = 1;

				// Border
				else if (val == 8)
				{
					// Off?
					rexx_skip_space(&args);
					if (rexx_match_keyword(&args, on_off_strings2, 0) == 0)
						flags &= ~ENVBF_USE_COLOUR;
					else
					{
						flags |= ENVBF_USE_COLOUR;
						colour = Atoh(args, -1) << 8;
					}
				}
			}

			// Expand assignment path for themes
			theme_expand_path(buf);

			// Store picture and flags
			stccpy((custom) ? GUI->env_BackgroundPic[which] : environment->env->env_BackgroundPic[which],
				   buf,
				   sizeof(environment->env->env_BackgroundPic[which]) - 1);
			if (custom)
			{
				GUI->env_BackgroundFlags[which] = flags;
				GUI->env_BackgroundBorderColour[which] = colour;
			}
			else
			{
				environment->env->env_BackgroundFlags[which] = flags;
				environment->env->env_BackgroundBorderColour[which] = colour;
			}
		}
		break;

		// Sound
		case RXCMD_SOUND: {
			Cfg_SoundEntry temp, *sound;
			Att_Node *node;
			char buf[20];

			// Get information
			rexx_parse_word(&args, temp.dse_Name, 32);
			if (clear)
				temp.dse_Sound[0] = 0;
			else
				rexx_parse_word(&args, temp.dse_Sound, 256);
			if (rexx_parse_word(&args, buf, 20) && buf[0])
				temp.dse_Volume = atoi(buf);
			else
				temp.dse_Volume = 64;

			// Expand assignment path for themes
			theme_expand_path(temp.dse_Sound);

			// Lock sound list
			GetSemaphore(&environment->sound_lock, SEMF_EXCLUSIVE, 0);

			// Find sound event
			if ((sound = (Cfg_SoundEntry *)FindNameI((struct List *)&environment->sound_list, temp.dse_Name)))
			{
				// Clear?
				if (clear)
				{
					// Remove and free
					Remove((struct Node *)sound);
					FreeMemH(sound);
				}

				// Store
				else
				{
					// Store new values
					strcpy(sound->dse_Sound, temp.dse_Sound);
					sound->dse_Volume = temp.dse_Volume;
					strcpy(result, "1");
				}
			}

			// Not found
			else if (!(node = (Att_Node *)FindNameI((struct List *)script_list, temp.dse_Name)) ||
					 node->data & SCRIPTF_NO_SOUND)
				rc = RXERR_INVALID_NAME;

			// Not in sound list currently; try to allocate
			else if (!clear && (sound = AllocMemH(environment->desktop_memory, sizeof(Cfg_SoundEntry))))
			{
				// Initialise and add
				strcpy(sound->dse_Name, temp.dse_Name);
				strcpy(sound->dse_Sound, temp.dse_Sound);
				sound->dse_Volume = temp.dse_Volume;
				sound->dse_Node.ln_Name = sound->dse_Name;
				AddTail((struct List *)&environment->sound_list, (struct Node *)sound);
				strcpy(result, "1");
			}

			// No memory
			else if (!clear)
				rc = RXERR_NO_MEMORY;

			// Unlock sound list
			FreeSemaphore(&environment->sound_lock);
		}
		break;

		// Palette
		case RXCMD_PALETTE:
			if (clear)
			{
				rc = RXERR_INVALID_SET;
				break;
			}
			args = strstri(args, "0x");
			for (val = 1; val < 49 && args; args = strstri(args, "0x"))
			{
				short a;
				if (*(args + 2) == ' ')
				{
					val += 3;
					++args;
					continue;
				}
				for (a = 0; a < 3; a++, val++)
				{
					ULONG col;
					args += 2;
					col = Atoh(args, 2);
					col |= (col << 24) | (col << 16) | (col << 8);
					environment->env->palette[val] = col;
				}
			}
			lsprintf(result, "%ld", (val - 1) / 3);
			break;

		// Pens
		case RXCMD_PENS:
			if (clear)
			{
				rc = RXERR_INVALID_SET;
				break;
			}
			rexx_skip_space(&args);
			while ((val = rexx_match_keyword(&args, pens_key, 0)) != -1)
			{
				// Icons?
				if (val == 0)
				{
					rexx_dopus_get_pen(&args, &environment->env->icon_fpen);
					rexx_dopus_get_pen(&args, &environment->env->icon_bpen);
					rexx_dopus_get_pen(&args, &environment->env->icon_style);
					rexx_dopus_get_pen(&args, &environment->env->iconw_fpen);
					rexx_dopus_get_pen(&args, &environment->env->iconw_bpen);
					rexx_dopus_get_pen(&args, &environment->env->iconw_style);
				}

				// Files/dirs
				else if (val == 1)
					rexx_dopus_get_colour(&args, environment->env->list_format.files_unsel, ENVCOL_FILES_UNSEL);
				else if (val == 2)
					rexx_dopus_get_colour(&args, environment->env->list_format.dirs_unsel, ENVCOL_DIRS_UNSEL);
				else if (val == 3)
					rexx_dopus_get_colour(&args, environment->env->list_format.files_sel, ENVCOL_FILES_SEL);
				else if (val == 4)
					rexx_dopus_get_colour(&args, environment->env->list_format.dirs_sel, ENVCOL_DIRS_SEL);

				// Devices/assigns
				else if (val == 5)
					rexx_dopus_get_colour(&args, environment->env->devices_col, ENVCOL_DEVICES);
				else if (val == 6)
					rexx_dopus_get_colour(&args, environment->env->volumes_col, ENVCOL_VOLUMES);

				// Source/dest
				else if (val == 7)
					rexx_dopus_get_colour(&args, environment->env->source_col, ENVCOL_SOURCE);
				else if (val == 8)
					rexx_dopus_get_colour(&args, environment->env->dest_col, ENVCOL_DEST);

				// Gauge
				else if (val == 9)
					rexx_dopus_get_colour(&args, environment->env->gauge_col, ENVCOL_GAUGE);

				// User pens
				else if (val == 10)
				{
					environment->env->palette_count = rexx_parse_number(&args, 0, environment->env->palette_count);
					if (environment->env->palette_count < 0)
						environment->env->palette_count = 0;
					else if (environment->env->palette_count > 8)
						environment->env->palette_count = 8;
				}

				// Tooltips/alternate rows
				else if (val == 11)
					rexx_dopus_get_colour(&args, environment->env->tooltip_col, ENVCOL_TOOLTIP);
				else if (val == 12)
					rexx_dopus_get_colour(&args, environment->env->stripe_col, ENVCOL_STRIPE);

				// Invalid
				else
				{
					rc = RXERR_INVALID_QUERY;
					break;
				}
			}
			break;

		// Fonts
		case RXCMD_FONT:
			if (clear)
			{
				rc = RXERR_INVALID_SET;
				break;
			}
			rexx_skip_space(&args);
			if ((val = rexx_match_keyword(&args, font_key, 0)) != -1)
			{
				rexx_parse_word(&args, environment->env->font_name[val], 40);
				rexx_skip_space(&args);
				environment->env->font_size[val] = rexx_parse_number(&args, 0, environment->env->font_size[val]);
			}
			else
				rc = RXERR_INVALID_SET;
			break;

		default:
			rc = RXERR_INVALID_SET;
			break;
		}
		break;

	// Query
	case RXCMD_QUERY:

		// Get get item
		id = rexx_get_command(&args);
		switch (id)
		{
		// Version
		case RXCMD_VERSION: {
			extern short version_num;

			// See the main RXCMD_VERSION handler above for why revisions
			// >= 100 report as "5 99 <rev>" instead of "5 <rev>".
			if (version_num >= 100)
				lsprintf(result, "5 99 %ld", version_num);
			else
				lsprintf(result, "5 %ld", version_num);
		}
		break;

		// Background
		case RXCMD_BACKGROUND: {
			// Get keys
			rc = RXERR_INVALID_QUERY;
			rexx_skip_space(&args);
			while ((val = rexx_match_keyword(&args, background_key, 0)) != -1)
			{
				// Desktop/lister/requester
				if (val >= 0 && val <= 2)
				{
					char col_val[20];
					lsprintf(col_val, "%06lx", environment->env->env_BackgroundBorderColour[val] >> 8);
					lsprintf(
						result,
						"\"%s\" %s %s precision %s border %s",
						environment->env->env_BackgroundPic[val],
						background_key[val],
						(environment->env->env_BackgroundFlags[val] & ENVBF_CENTER_PIC)
							? "center"
							: (environment->env->env_BackgroundFlags[val] & ENVBF_STRETCH_PIC) ? "stretch" : "tile",
						(environment->env->env_BackgroundFlags[val] & ENVBF_PRECISION_NONE)
							? "none"
							: (environment->env->env_BackgroundFlags[val] & ENVBF_PRECISION_GUI)
								  ? "gui"
								  : (environment->env->env_BackgroundFlags[val] & ENVBF_PRECISION_ICON)
										? "icon"
										: (environment->env->env_BackgroundFlags[val] & ENVBF_PRECISION_IMAGE)
											  ? "image"
											  : "exact",
						(environment->env->env_BackgroundFlags[val] & ENVBF_USE_COLOUR) ? col_val : "off");
					rc = 0;
					break;
				}
			}
		}
		break;

		// Sound
		case RXCMD_SOUND: {
			char buf[32];
			Cfg_SoundEntry *sound;
			Att_Node *node;

			// Get name
			rexx_parse_word(&args, buf, 32);

			// Lock sound list
			GetSemaphore(&environment->sound_lock, SEMF_SHARED, 0);

			// Find sound event
			if ((sound = (Cfg_SoundEntry *)FindNameI((struct List *)&environment->sound_list, buf)))
			{
				// Return the information
				lsprintf(result, "\"%s\" %ld", sound->dse_Sound, sound->dse_Volume);
			}

			// Not found
			else if (!(node = (Att_Node *)FindNameI((struct List *)script_list, buf)) || node->data & SCRIPTF_NO_SOUND)
				rc = RXERR_INVALID_NAME;

			// Unlock sound list
			FreeSemaphore(&environment->sound_lock);
		}
		break;

		// Pens
		case RXCMD_PENS:

			rexx_skip_space(&args);
			while ((val = rexx_match_keyword(&args, pens_key, 0)) != -1)
			{
				char buf[20];

				// Icons?
				if (val == 0)
					lsprintf(buf,
							 "%ld %ld %ld %ld %ld %ld ",
							 rexx_dopus_map_pen(environment->env->icon_fpen),
							 rexx_dopus_map_pen(environment->env->icon_bpen),
							 rexx_dopus_map_pen(environment->env->icon_style),
							 rexx_dopus_map_pen(environment->env->iconw_fpen),
							 rexx_dopus_map_pen(environment->env->iconw_bpen),
							 rexx_dopus_map_pen(environment->env->iconw_style));

				// Files/dirs
				else if (val == 1)
					lsprintf(buf,
							 "%ld %ld ",
							 rexx_dopus_map_pen(environment->env->list_format.files_unsel[0]),
							 rexx_dopus_map_pen(environment->env->list_format.files_unsel[1]));
				else if (val == 2)
					lsprintf(buf,
							 "%ld %ld ",
							 rexx_dopus_map_pen(environment->env->list_format.dirs_unsel[0]),
							 rexx_dopus_map_pen(environment->env->list_format.dirs_unsel[1]));
				else if (val == 3)
					lsprintf(buf,
							 "%ld %ld ",
							 rexx_dopus_map_pen(environment->env->list_format.files_sel[0]),
							 rexx_dopus_map_pen(environment->env->list_format.files_sel[1]));
				else if (val == 4)
					lsprintf(buf,
							 "%ld %ld ",
							 rexx_dopus_map_pen(environment->env->list_format.dirs_sel[0]),
							 rexx_dopus_map_pen(environment->env->list_format.dirs_sel[1]));

				// Devices/assigns
				else if (val == 5)
					lsprintf(buf,
							 "%ld %ld ",
							 rexx_dopus_map_pen(environment->env->devices_col[0]),
							 rexx_dopus_map_pen(environment->env->devices_col[1]));
				else if (val == 6)
					lsprintf(buf,
							 "%ld %ld ",
							 rexx_dopus_map_pen(environment->env->volumes_col[0]),
							 rexx_dopus_map_pen(environment->env->volumes_col[1]));

				// Source/dest
				else if (val == 7)
					lsprintf(buf,
							 "%ld %ld ",
							 rexx_dopus_map_pen(environment->env->source_col[0]),
							 rexx_dopus_map_pen(environment->env->source_col[1]));
				else if (val == 8)
					lsprintf(buf,
							 "%ld %ld ",
							 rexx_dopus_map_pen(environment->env->dest_col[0]),
							 rexx_dopus_map_pen(environment->env->dest_col[1]));

				// Gauge
				else if (val == 9)
					lsprintf(buf,
							 "%ld %ld ",
							 rexx_dopus_map_pen(environment->env->gauge_col[0]),
							 rexx_dopus_map_pen(environment->env->gauge_col[1]));

				// User pens
				else if (val == 10)
					lsprintf(buf, "%ld ", environment->env->palette_count);

				// Tooltips/alternate rows
				else if (val == 11)
					lsprintf(buf,
							 "%ld %ld ",
							 rexx_dopus_map_pen(environment->env->tooltip_col[0]),
							 rexx_dopus_map_pen(environment->env->tooltip_col[1]));
				else if (val == 12)
					lsprintf(buf,
							 "%ld %ld ",
							 rexx_dopus_map_pen(environment->env->stripe_col[0]),
							 rexx_dopus_map_pen(environment->env->stripe_col[1]));

				// Invalid
				else
				{
					rc = RXERR_INVALID_QUERY;
					break;
				}
				strcat(result, buf);
			}
			if (!result[0])
				rc = RXERR_INVALID_QUERY;
			break;

		// Palette
		case RXCMD_PALETTE: {
			short num;
			ULONG palette[50];
			CopyMem((char *)environment->env->palette, (char *)palette, sizeof(palette));
			if (GUI->screen_pointer)
			{
				GetPalette32(&GUI->screen_pointer->ViewPort, &palette[1], 4, 0);
				if (GUI->screen_pointer->RastPort.BitMap->Depth > 2)
					GetPalette32(&GUI->screen_pointer->ViewPort,
								 &palette[13],
								 4,
								 (1 << GUI->screen_pointer->RastPort.BitMap->Depth) - 4);
			}
			for (num = 1; num < 49; num += 3)
			{
				char buf[40];
				lsprintf(buf,
						 "0x%02lx%02lx%02lx ",
						 (palette[num + 0] & 0xff000000) >> 24,
						 (palette[num + 1] & 0xff000000) >> 24,
						 (palette[num + 2] & 0xff000000) >> 24);
				strcat(result, buf);
			}
		}
		break;

		// Fonts
		case RXCMD_FONT:
			rexx_skip_space(&args);
			if ((val = rexx_match_keyword(&args, font_key, 0)) != -1)
			{
				lsprintf(result, "\"%s\" %ld", environment->env->font_name[val], environment->env->font_size[val]);
			}
			else
				rc = RXERR_INVALID_QUERY;
			break;

		default:
			rc = RXERR_INVALID_QUERY;
			break;
		}
		break;

	// Refresh
	case RXCMD_REFRESH:

		// Get set item
		id = rexx_get_command(&args);
		switch (id)
		{
		// All
		case RXCMD_ALL:
			rexx_send_reset(CONFIG_CHANGE_DISPLAY, 0);
			break;

		// Background
		case RXCMD_BACKGROUND: {
			BOOL custom = 0;

			// Enable pattern if it's disabled
			environment->env->display_options &= ~DISPOPTF_NO_BACKDROP;
			environment->env->display_options &= ~DISPOPTF_USE_WBPATTERN;

			// Custom flag set?
			rexx_skip_space(&args);
			if ((val = rexx_match_keyword(&args, custom_key, 0)) == 0)
				custom = 1;

			// Launch process to read pattern
			misc_startup(NAME_PATTERNS, CMD_REMAP_PATTERNS, 0, (APTR)(IPTR)custom, TRUE);
		}
		break;

		// Icons
		case RXCMD_ICONS:
			rexx_send_reset(CONFIG_CHANGE_ICON_FONT | CONFIG_CHANGE_ICON_FONT_WINDOWS, 0);
			break;

		// Listers
		case RXCMD_LISTER:
			rexx_skip_space(&args);
			if (rexx_match_keyword(&args, full_string, 0) == 0)
				rexx_send_reset(CONFIG_CHANGE_LIST_DISPLAY | CONFIG_CHANGE_LIST_FONT, 0);
			else
				rexx_send_reset(CONFIG_CHANGE_LIST_DISPLAY, 0);
			break;
		}
		break;
	}

	// Set return
	if (rc != -1 && msg)
		rexx_set_return(msg, rc, result);
	return ret;
}

// Get a pen colour
BOOL rexx_dopus_get_pen(char **args, UBYTE *ptr)
{
	// Get pen number
	if (!rexx_parse_number_byte(args, ptr))
		return FALSE;

	// Map pen
	if (*ptr >= 4 && *ptr < 8)
		*ptr = 252 + (*ptr - 4);
	else if (*ptr >= 8)
		*ptr -= 4;
	return TRUE;
}

// Get the two pens of a lister colour element; a pen given replaces its custom colour
void rexx_dopus_get_colour(char **args, UBYTE *pens, short col)
{
	short num;

	for (num = 0; num < 2; num++)
		if (rexx_dopus_get_pen(args, &pens[num]))
			environment->env->env_ColourFlag &= ~(1 << (col + num * CUST_PENS));
}

// Map pen colour back
short rexx_dopus_map_pen(short pen)
{
	if (pen >= 252)
		return (short)(pen - 252 + 4);
	if (pen >= 4)
		return (short)(pen + 4);
	return pen;
}

// Send refresh to Opus
void rexx_send_reset(ULONG flag1, ULONG flag2)
{
	ULONG *flags;
	if ((flags = AllocVec(sizeof(ULONG) * 2, MEMF_ANY)))
	{
		flags[0] = flag1;
		flags[1] = flag2;
		IPC_Command(&main_ipc, MAINCMD_RESET, 0, 0, flags, 0);
	}
}

// Expand theme path
void theme_expand_path(char *buf)
{
	// Uses assignment?
	if (strnicmp(buf, "D5THEMES:", 9) == 0)
	{
		char temp[256];
		BPTR lock;

		// Copy remainder
		strcpy(temp, buf + 9);

		// Get full path
		if ((lock = Lock("D5THEMES:", ACCESS_READ)))
		{
			DevNameFromLockDopus(lock, buf, 256);
			UnLock(lock);
			AddPart(buf, "", 256);
			strcat(buf, temp);
		}
	}
}
