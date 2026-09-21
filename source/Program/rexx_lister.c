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

#define RETURN_RESULT 0
#define RETURN_VAR 1
#define RETURN_STEM 2

enum {
	NL_TOOLBAR,
	NL_PARENT,
	NL_NOACTIVE,
	NL_NOVISIBLE,
	NL_ICONIFIED,
	NL_FROMICON,
	NL_MODE,
};

// Create a new lister
void rexx_lister_new(struct RexxMsg *msg, char *args)
{
	struct IBox dims;
	char *path = 0;
	Cfg_Lister *cfg;
	Lister *new_lister = 0, *parent = 0;
	char toolbar_path[256];
	char buf[32];
	short key, fromicon = 0, nomode = 0;
	ULONG flags = 0;
	position_rec *pos = 0;

	// Get dimensions
	rexx_skip_space(&args);
	dims.Left = rexx_parse_number(&args, 1, -1);
	dims.Top = rexx_parse_number(&args, 1, -1);
	dims.Width = rexx_parse_number(&args, 1, 0);
	dims.Height = rexx_parse_number(&args, 0, 0);

	// Clear toolbar path
	toolbar_path[0] = 0;

	// Check keywords
	while ((key = rexx_match_keyword(&args, new_lister_keys, 0)) != -1)
	{
		// Toolbar supplied?
		if (key == NL_TOOLBAR)
		{
			// Get toolbar path
			rexx_parse_word(&args, toolbar_path, 256);
		}

		// Parent
		else if (key == NL_PARENT)
		{
			// Get parent lister
			parent = (Lister *)rexx_parse_iptr(&args, 0, 0);
		}

		// Invisible
		else if (key == NL_NOVISIBLE)
			flags |= DLSTF_INVISIBLE;

		// Iconfied
		else if (key == NL_ICONIFIED)
			flags |= DLSTF_ICONIFIED;

		// No activate
		else if (key == NL_NOACTIVE)
			flags |= DLSTF_NOACTIVE;

		// From icon
		else if (key == NL_FROMICON)
			fromicon = 1;

		// Mode
		else if (key == NL_MODE)
		{
			// Match mode keys
			nomode = 1;
			while ((key = rexx_match_keyword(&args, mode_keys, 0)) != -1)
			{
				// Icon?
				if (key == 1)
					flags |= DLSTF_ICON;

				// Icon Action?
				else if (key == 3)
					flags |= DLSTF_ICON | DLSTF_ICON_ACTION;

				// Show All
				else if (key == 2)
					flags |= DLSTF_SHOW_ALL;

				// Skip spaces
				rexx_skip_space(&args);
			}
		}

		// Skip spaces
		rexx_skip_space(&args);
	}

	// Get path
	rexx_skip_space(&args);
	if (*args)
		path = args;

	// Get a new lister definition
	if ((cfg = NewLister(path)))
	{
		// Lock lister list
		lock_listlock(&GUI->lister_list, FALSE);

		// Got a parent list?
		if (parent)
		{
			// Check if it's valid
			if (!(rexx_lister_valid(parent)))
				parent = 0;
		}

		// Fill out lister
		lister_init_new(cfg, parent);

		// Unlock lister list
		unlock_listlock(&GUI->lister_list);

		// Store dimensions (if set)
		if (dims.Left != -1)
			cfg->lister.pos[0].Left = dims.Left;
		if (dims.Top != -1)
			cfg->lister.pos[0].Top = dims.Top;
		if (dims.Width)
			cfg->lister.pos[0].Width = dims.Width;
		if (dims.Height)
			cfg->lister.pos[0].Height = dims.Height;

		// Store additional flags
		cfg->lister.flags |= flags;

		// Position from icon?
		if (fromicon && path)
		{
			char fullpath[512];
			BPTR lock;

			// Get full path
			if ((lock = Lock(path, ACCESS_READ)))
			{
				struct DiskObject *icon;
				short mode = 0;

				NameFromLock(lock, fullpath, 512);
				UnLock(lock);

				// Try and load icon
				icon = GetDiskObject(fullpath);

				// Get position
				pos = GetListerPosition(fullpath,
										0,
										icon,
										&cfg->lister.pos[0],
										(nomode) ? 0 : &mode,
										&cfg->lister.format,
										(parent) ? parent->window : 0,
										parent,
										0);

				// Free icon
				FreeDiskObject(icon);

				// Mode set?
				if (!nomode && mode)
				{
					if (mode & LISTERMODE_ICON)
						flags |= DLSTF_ICON;
					if (mode & LISTERMODE_ICON_ACTION)
						flags |= DLSTF_ICON_ACTION;
					if (mode & LISTERMODE_SHOW_ALL)
						flags |= DLSTF_SHOW_ALL;

					// Store additional flags
					cfg->lister.flags |= flags;
				}
			}
		}

		// Create a new lister
		if ((new_lister = lister_new(cfg)))
		{
			// Store position record
			new_lister->pos_rec = pos;

			// Custom toolbar?
			if (toolbar_path[0])
			{
				// Get new toolbar
				lister_new_toolbar(new_lister, toolbar_path, 0);
			}

			// Initialise lister
			IPC_Command(new_lister->ipc, LISTER_INIT, 0, GUI->screen_pointer, 0, 0);
		}
	}

	// Got a message?
	if (msg)
	{
		// Build return codes
		if (new_lister)
		{
			rexx_format_iptr(buf, (IPTR)new_lister);
			rexx_set_return(msg, 0, buf);
		}
		else
			rexx_set_return(msg, RXERR_NO_LISTER, 0);
	}
}

// Rexx lister command
BOOL rexx_lister_cmd(struct RexxMsg *msg, short command, char *args)
{
	Lister *lister = 0;
	long rc;
	char result[768];
	char varname[40];
	short id, val;
	short var_flag = RETURN_RESULT;
	short all_listers = 0;
	BOOL ret = 0;

	// Zero returns
	rc = 0;
	result[0] = 0;
	varname[0] = 0;

	// Get lister handle
	rexx_skip_space(&args);

	// All listers?
	if ((command == RXCMD_CLOSE || command == RXCMD_REFRESH || command == RXCMD_ICONIFY) &&
		(rexx_match_keyword(&args, all_keys, 0) == 0))
		all_listers = 1;

	// Get handle
	else
		lister = (Lister *)rexx_parse_iptr(&args, 0, 0);

	// See if variable or stem is specified
	for (id = strlen(args), val = 0; id > 0 && val < 2; id--)
		if (isspace(args[id]))
			++val;

	// Get two spaces?
	if (val == 2)
	{
		char *ptr = args + id + 2;

		// Var or stem?
		if (strnicmp(ptr, "var ", 4) == 0)
			var_flag = RETURN_VAR;
		else if (strnicmp(ptr, "stem ", 5) == 0)
			var_flag = RETURN_STEM;

		// Get variable name
		if (var_flag)
		{
			// Get name
			ptr += 3 + var_flag;
			rexx_skip_space(&ptr);
			rexx_parse_word(&ptr, varname, 40);

			// Stem?
			if (var_flag == RETURN_STEM)
			{
				// Check stem has a period
				if (varname[0] && varname[strlen(varname) - 1] != '.')
					strcat(varname, ".");
			}
		}
	}

	// Lister handle null?
	if (!lister && !all_listers)
	{
		// Sub-command supplied (that doesn't require a lister)?
		id = rexx_get_command(&args);

		// Query something?
		if (command == RXCMD_QUERY)
		{
			// Look at query type
			switch (id)
			{
			// Return lister handles?
			case RXCMD_SOURCE:
			case RXCMD_DEST:
			case RXCMD_ALL:
			case RXCMD_ACTIVE:
				rexx_lister_get_current(msg, id, var_flag, varname);
				return 0;

			// List format information
			case RXCMD_SORT:
			case RXCMD_SEPARATE:
			case RXCMD_DISPLAY:
			case RXCMD_FLAGS:
			case RXCMD_HIDE:
			case RXCMD_SHOW:
				rexx_query_format(id, &environment->env->list_format, result);
				rexx_set_return(msg, 0, result);
				return 0;
			}
		}

		// Invalid handle
		rexx_set_return(msg, RXERR_INVALID_HANDLE, 0);
		return 0;
	}

	// Doing all listers?
	if (all_listers)
	{
		short a;

		// See if command is in this list
		for (a = 0; rexx_commands_can_do_all[a]; a++)
			if (rexx_commands_can_do_all[a] == command)
				break;

		// Not found?
		if (!rexx_commands_can_do_all[a])
		{
			// Invalid handle
			rexx_set_return(msg, RXERR_INVALID_HANDLE, 0);
			return 0;
		}
	}

	// Get list lock
	lock_listlock(&GUI->lister_list, FALSE);

	// Is lister invalid (except for freecache command)
	if (lister && command != RXCMD_FREECACHES && !(rexx_lister_valid(lister)))
		rc = RXERR_INVALID_HANDLE;

	// No, it's okay
	else
	{
		// Look at command
		switch (command)
		{
		// Close lister
		case RXCMD_CLOSE:

			// Send quit to lister
			if (all_listers)
				IPC_ListCommand(&GUI->lister_list, IPC_QUIT, 0, 0, 0);
			else
				IPC_Command(lister->ipc, IPC_QUIT, 0, 0, 0, 0);
			break;

		// Read a path
		case RXCMD_READ: {
			long flags;
			// Paths are stored in DirBuffer.buf_Path[512]; keep the parse
			// capacity consistent so long paths are not silently truncated
			char path[512];

			// Get path to read
			rexx_parse_word(&args, path, 512);

			// Force re-read?
			rexx_skip_space(&args);
			if ((rexx_match_keyword(&args, force_string, 0)) == 0)
				flags = 0;
			else
				flags = GETDIRF_CANMOVEEMPTY | GETDIRF_CANCHECKBUFS;

			// Return old path
			strcpy(result, lister->cur_buffer->buf_Path);

			// Read this path
			function_launch(FUNCTION_READ_DIRECTORY, 0, 0, flags, lister, 0, path, 0, 0, 0, 0);
		}
		break;

		// Wait for lister to be non-busy
		case RXCMD_WAIT: {
			short flags = 0;

			// Quick?
			rexx_skip_space(&args);
			if ((rexx_match_keyword(&args, quick_string, 0)) == 0)
				flags = 1;

			// Send to lister
			flags = IPC_Command(lister->ipc, LISTER_WAIT, flags, msg, 0, REPLY_NO_PORT);

			// Not returned immediately?
			if (flags)
			{
				// Script will wait for reply
				msg = 0;
				ret = 1;
			}
		}
		break;

		// Refresh lister
		case RXCMD_REFRESH: {
			long refresh;
			short ret;

			// Default refresh
			refresh = REFRESHF_SLIDERS | REFRESHF_RESORT;

			// Refresh fully?
			rexx_skip_space(&args);
			if ((ret = rexx_match_keyword(&args, full_string, 0)) == 0)
				refresh |= REFRESHF_UPDATE_NAME | REFRESHF_STATUS;

			// Datestamp only?
			else if (ret == 1)
				refresh = REFRESHF_DATESTAMP;

			// Send refresh command
			if (all_listers)
			{
				IPC_ListCommand(&GUI->lister_list, LISTER_REFRESH_WINDOW, refresh, 0, 0);
			}
			else
			{
				IPC_Command(lister->ipc, LISTER_REFRESH_WINDOW, refresh, 0, 0, 0);
			}
		}
		break;

		// Query lister
		case RXCMD_QUERY:

			// Lock current buffer
			buffer_lock(lister->cur_buffer, FALSE);

			// Get query item
			id = rexx_get_command(&args);
			switch (id)
			{
			// Path
			case RXCMD_PATH:
				strcpy(result, lister->cur_buffer->buf_Path);
				break;

			// Label
			case RXCMD_LABEL:
				strcpy(result, lister->icon_name);
				break;

			// Position
			case RXCMD_POSITION: {
				struct IBox *box;

				// If window is open, get position from that
				if (lister_valid_window(lister))
					box = (struct IBox *)&lister->window->LeftEdge;

				// Otherwise, get stored position
				else
					box = &lister->dimensions.wd_Normal;

				// Build string
				lsprintf(result,
						 "%ld/%ld/%ld/%ld%s %ld/%ld/%ld/%ld",
						 box->Left,
						 box->Top,
						 box->Width,
						 box->Height,
						 (lister->flags & LISTERF_LOCK_POS) ? " locked" : 0,
						 lister->cur_buffer->buf_TotalEntries[0],
						 lister->cur_buffer->buf_HorizLength,
						 lister->text_width,
						 lister->text_height);
			}
			break;

			// Busy
			case RXCMD_BUSY:
				lsprintf(result, "%ld", (lister->flags & LISTERF_BUSY) ? 1 : 0);
				break;

			// Handler
			case RXCMD_HANDLER:
				strcpy(result, lister->cur_buffer->buf_CustomHandler);
				break;

			// Visible
			case RXCMD_VISIBLE:
				lsprintf(result, "%ld", (lister_valid_window(lister)) ? 1 : 0);
				break;

			// Entries
			case RXCMD_FILES:
			case RXCMD_DIRS:
			case RXCMD_ENTRIES:
			case RXCMD_SELFILES:
			case RXCMD_SELDIRS:
			case RXCMD_SELENTRIES:
			case RXCMD_FIRSTSEL:
				rexx_skip_space(&args);
				rexx_lister_file_return(msg, lister->cur_buffer, id, args[0], var_flag, varname);
				rc = -1;
				break;

			// File count
			case RXCMD_NUMFILES:
				lsprintf(result, "%ld", lister->cur_buffer->buf_TotalFiles[0]);
				break;

			// Dir count
			case RXCMD_NUMDIRS:
				lsprintf(result, "%ld", lister->cur_buffer->buf_TotalDirs[0]);
				break;

			// Entry count
			case RXCMD_NUMENTRIES:
				lsprintf(result, "%ld", lister->cur_buffer->buf_TotalEntries[0]);
				break;

			// Byte count
			case RXCMD_NUMBYTES:
#ifdef USE_64BIT
				ItoaU64(&lister->cur_buffer->buf_TotalBytes[0], result, sizeof(result), 0);
#else
				lsprintf(result, "%ld", lister->cur_buffer->buf_TotalBytes[0]);
#endif
				break;

			// Selected file count
			case RXCMD_NUMSELFILES:
				lsprintf(result, "%ld", lister->cur_buffer->buf_SelectedFiles[0]);
				break;

			// Selected dir count
			case RXCMD_NUMSELDIRS:
				lsprintf(result, "%ld", lister->cur_buffer->buf_SelectedDirs[0]);
				break;

			// Selected entry count
			case RXCMD_NUMSELENTRIES:
				lsprintf(
					result, "%ld", lister->cur_buffer->buf_SelectedFiles[0] + lister->cur_buffer->buf_SelectedDirs[0]);
				break;

			// Selected byte count
			case RXCMD_NUMSELBYTES:
#ifdef USE_64BIT
				ItoaU64(&lister->cur_buffer->buf_SelectedBytes[0], result, sizeof(result), 0);
#else
				lsprintf(result, "%ld", lister->cur_buffer->buf_SelectedBytes[0]);
#endif
				break;

			// Entry information
			case RXCMD_ENTRY:
				rexx_skip_space(&args);
				rexx_lister_entry_info(msg, lister->cur_buffer, args, var_flag, varname);
				rc = -1;
				break;

			// List format information
			case RXCMD_SORT:
			case RXCMD_SEPARATE:
			case RXCMD_DISPLAY:
			case RXCMD_FLAGS:
			case RXCMD_HIDE:
			case RXCMD_SHOW:
				rexx_query_format(id, &lister->cur_buffer->buf_ListFormat, result);
				break;

			// Abort
			case RXCMD_ABORT:

				// Abort flag set?
				if (lister->flags & LISTERF_ABORTED)
					result[0] = '1';
				else
					result[0] = '0';
				result[1] = 0;
				break;

			// Toolbar
			case RXCMD_TOOLBAR:

				// Valid toolbar?
				if (lister_valid_toolbar(lister))
					strcpy(result, lister->toolbar_path);
				else
					rc = RXERR_NO_TOOLBAR;
				break;

			// Mode
			case RXCMD_MODE:

				// Icon mode?
				if (lister->flags & LISTERF_VIEW_ICONS)
				{
					// "icon" string
					strcpy(result, mode_keys[1]);

					// Action?
					if (lister->flags & LISTERF_ICON_ACTION)
					{
						strcat(result, " ");
						strcat(result, mode_keys[3]);
					}

					// Show all?
					if (lister->flags & LISTERF_SHOW_ALL)
					{
						strcat(result, " ");
						strcat(result, mode_keys[2]);
					}
				}

				// Name mode
				else
					strcpy(result, mode_keys[0]);
				break;

			// Locked state
			case RXCMD_LOCK:

				// Get keyword
				rexx_skip_space(&args);
				if ((val = rexx_match_keyword(&args, lock_keys, 0)) != -1)
				{
					// Lock state?
					if (val == 0)
						val = (lister->cur_buffer->more_flags & DWF_LOCK_STATE) ? 1 : 0;

					// Lock format
					else
						val = (lister->cur_buffer->more_flags & DWF_LOCK_FORMAT) ? 1 : 0;

					// Build string
					lsprintf(result, "%ld", val);
				}
				else
					rc = RXERR_INVALID_QUERY;
				break;

			// Case sensitivity
			case RXCMD_CASE:
				lsprintf(result, "%ld", (lister->cur_buffer->more_flags & DWF_CASE) ? 1 : 0);
				break;

			// Name length
			case RXCMD_NAMELENGTH:
				lsprintf(result, "%ld", lister->cur_buffer->name_field_size);
				break;

			// Comment length
			case RXCMD_COMMENTLENGTH:
				lsprintf(result, "%ld", lister->cur_buffer->comment_field_size);
				break;

			// Movement
			case RXCMD_MOVEMENT:
				lsprintf(result, "%ld %ld", lister->cur_buffer->buf_VertOffset, lister->cur_buffer->buf_HorizOffset);
				break;

			// Window pointer
			case RXCMD_WINDOW:
				rexx_format_iptr(result, (IPTR)lister->window);
				break;

			// Process pointer
			case RXCMD_PROC:
				rexx_format_iptr(result, (IPTR)lister->ipc->proc);
				break;

			// Custom title/header
			case RXCMD_TITLE:
			case RXCMD_HEADER:

				// Lock current buffer
				buffer_lock(lister->cur_buffer, FALSE);

				// Return title as the result
				strcpy(
					result,
					(id == RXCMD_TITLE) ? lister->cur_buffer->buf_CustomTitle : lister->cur_buffer->buf_CustomHeader);

				// Unlock buffer
				buffer_unlock(lister->cur_buffer);
				break;

			// Query value
			case RXCMD_VALUE: {
				char name[40];
				struct Node *find;

				// Get name
				rexx_skip_space(&args);
				rexx_parse_word(&args, name, 40);

				// See if value exists
				lock_listlock(&lister->user_data_list, FALSE);
				if ((find = FindNameI(&lister->user_data_list.list, name)))
				{
					// Save value
					stccpy(result, (char *)(find + 1), sizeof(result) - 1);
				}
				else
					rc = RXERR_INVALID_NAME;

				// Unlock value list
				unlock_listlock(&lister->user_data_list);
			}
			break;

			// Unknown
			default:
				rc = RXERR_INVALID_QUERY;
				break;
			}

			// Unlock buffer
			buffer_unlock(lister->cur_buffer);
			break;

		// Clear lister
		case RXCMD_CLEAR:

			// Get set item
			id = rexx_get_command(&args);

			// Look at id
			switch (id)
			{
			// Clear whole lister
			case 0:

				// Check we're not showing a special buffer
				check_special_buffer(lister, 1);

				// Lock current buffer
				buffer_lock(lister->cur_buffer, TRUE);

				// Free contents of buffer
				buffer_freedir(lister->cur_buffer, 0);

				// Unlock buffer
				buffer_unlock(lister->cur_buffer);
				break;

			// Clear progress indicator
			case RXCMD_PROGRESS:

				// Turn progress indicator off
				IPC_Command(lister->ipc, LISTER_PROGRESS_OFF, 0, 0, 0, 0);
				break;

			// Abort flag
			case RXCMD_ABORT:

				// Clear abort flag
				lister->flags &= ~LISTERF_ABORTED;
				break;

			// Flags
			case RXCMD_FLAGS:
				rexx_skip_space(&args);
				if (!(rc = rexx_set_format(command, id, &lister->format, args)))
					lister->cur_buffer->flags &= ~DWF_SPECIAL_SORT;
				break;

			// Clear value
			case RXCMD_VALUE: {
				char name[40];
				struct Node *find;

				// Get name
				rexx_skip_space(&args);
				rexx_parse_word(&args, name, 40);

				// See if value exists
				lock_listlock(&lister->user_data_list, TRUE);
				if ((find = FindNameI(&lister->user_data_list.list, name)))
				{
					// Save old value and free it
					stccpy(result, (char *)(find + 1), sizeof(result) - 1);
					Remove(find);
					FreeMemH(find);
				}
				else
					rc = RXERR_INVALID_NAME;

				// Unlock value list
				unlock_listlock(&lister->user_data_list);
			}
			break;

			// Invalid
			default:
				rc = RXERR_INVALID_SET;
				break;
			}
			break;

		// Set lister
		case RXCMD_SET:

			// Get set item
			id = rexx_get_command(&args);

			// Look at id
			switch (id)
			{
			// Path
			case RXCMD_PATH:

				// Get pointer to path
				rexx_skip_space(&args);

				// Lock current buffer
				buffer_lock(lister->cur_buffer, TRUE);

				// Copy path string
				strcpy(lister->cur_buffer->buf_Path, args);
				strcpy(lister->cur_buffer->buf_ExpandedPath, args);
				strcpy(lister->cur_buffer->buf_ObjectName, FilePart(args));

				// Update path field
				lister_update_pathfield(lister);

				// Unlock buffer
				buffer_unlock(lister->cur_buffer);
				break;

			// Label
			case RXCMD_LABEL:

				// Get pointer to label
				rexx_skip_space(&args);

				// Copy label
				stccpy(lister->cur_buffer->buf_CustomLabel, args, 31);

				// Do we have an appicon?
				if (lister->appicon)
				{
					// Update icon label
					lister_build_icon_name(lister);

					// Update app icon
					SendNotifyMsg(DN_APP_ICON_LIST, (IPTR)lister->appicon, DNF_ICON_CHANGED, FALSE, 0, 0);
				}
				break;

			// Position
			case RXCMD_POSITION:

				// Is lister locked in position?
				if (lister->flags & LISTERF_LOCK_POS)
					rc = RXERR_INVALID_SET;

				// Ok to position
				else
				{
					struct IBox dims;

					// Get new dimensions
					rexx_skip_space(&args);
					dims.Left = rexx_parse_number(&args, 1, 0);
					dims.Top = rexx_parse_number(&args, 1, 0);
					dims.Width = rexx_parse_number(&args, 1, 0);
					dims.Height = rexx_parse_number(&args, 0, 0);

					// If window is open, change immediately
					if (lister_valid_window(lister))
					{
						if (dims.Width == 0)
							dims.Width = lister->window->Width;
						if (dims.Height == 0)
							dims.Height = lister->window->Height;
						ChangeWindowBox(lister->window, dims.Left, dims.Top, dims.Width, dims.Height);
					}

					// Otherwise, store for future use
					else
					{
						if (dims.Width == 0)
							dims.Width = lister->dimensions.wd_Normal.Width;
						if (dims.Height == 0)
							dims.Height = lister->dimensions.wd_Normal.Height;
						lister->dimensions.wd_Normal = dims;
					}
				}
				break;

			// Handler
			case RXCMD_HANDLER: {
				// Lock current buffer
				buffer_lock(lister->cur_buffer, TRUE);

				// Store handler name
				rexx_skip_space(&args);
				rexx_parse_word(&args, lister->cur_buffer->buf_CustomHandler, 32);

				// Set lister pointer in buffer
				lister->cur_buffer->buf_OwnerLister = lister;

				// Get flags
				lister->cur_buffer->cust_flags = 0;
				rexx_skip_space(&args);

				// Want quotes?
				while ((val = rexx_match_keyword(&args, custhandler_keys, 0)) != -1)
				{
					// Quotes
					if (val == SETHF_QUOTES)
						lister->cur_buffer->cust_flags |= CUSTF_WANT_QUOTES;

					// Full paths
					else if (val == SETHF_FULLPATH)
						lister->cur_buffer->cust_flags |= CUSTF_WANT_FULLPATH;

					// Support editing
					else if (val == SETHF_EDITING)
						lister->cur_buffer->cust_flags |= CUSTF_EDITING;

					// No popups
					else if (val == SETHF_NOPOPUPS)
						lister->cur_buffer->cust_flags |= CUSTF_NOPOPUPS;

					// Gauge
					else if (val == SETHF_GAUGE)
						lister->cur_buffer->cust_flags |= CUSTF_GAUGE;

					// No sub-drops
					else if (val == SETHF_SUBDROP)
						lister->cur_buffer->cust_flags |= CUSTF_SUBDROP;

					// Support inherit
					else if (val == SETHF_INHERIT)
						lister->cur_buffer->cust_flags |= CUSTF_INHERIT;

					// Don't remove the fuel gauge
					else if (val == SETHF_LEAVEGAUGE)
						lister->cur_buffer->cust_flags |= CUSTF_LEAVEGAUGE;

					// Sync trap messages
					else if (val == SETHF_SYNCTRAPS)
						lister->cur_buffer->cust_flags |= CUSTF_SYNCTRAPS;
				}

				// Unlock buffer
				buffer_unlock(lister->cur_buffer);

				// Check that the gauge display is ok
				IPC_Command(lister->ipc, LISTER_SET_GAUGE, 0, 0, 0, 0);
			}
			break;

			// Busy
			case RXCMD_BUSY: {
				short wait;

				// Get state
				rexx_skip_space(&args);
				val = rexx_match_keyword(&args, on_off_strings, 0);

				// Wait?
				wait = (rexx_match_keyword(&args, command_keys, 0) == 0);

				// Set lister busy state
				if (val != -1)
				{
					// Turn busy on or off
					IPC_Command(lister->ipc,
								LISTER_WAIT_BUSY,
								(IPTR)msg,
								(APTR)(IPTR)((val & 1) ? 1 : 0),
								0,
								(wait) ? REPLY_NO_PORT : 0);

					// If turning it off, turn progress bar off too
					if (!(val & 1))
						IPC_Command(lister->ipc, LISTER_PROGRESS_OFF, 0, 0, 0, 0);

					msg = 0;
					ret = 1;
				}
			}
			break;

			// Visible
			case RXCMD_VISIBLE:

				// Get state
				rexx_skip_space(&args);
				val = rexx_match_keyword(&args, on_off_strings, 0);

				// Make invisible?
				rexx_skip_space(&args);
				if (val != -1 && !(val & 1))
					IPC_Command(lister->ipc, IPC_HIDE, 0, 0, 0, 0);

				// Make visible
				else
					IPC_Command(lister->ipc, IPC_SHOW, 0, GUI->screen_pointer, 0, 0);
				break;

			// List format information
			case RXCMD_SORT:
			case RXCMD_SEPARATE:
			case RXCMD_DISPLAY:
			case RXCMD_FLAGS:
			case RXCMD_HIDE:
			case RXCMD_SHOW:
			case RXCMD_DEFAULT:
				rexx_skip_space(&args);
				if (!(rc = rexx_set_format(command, id, &lister->format, args)))
					lister->cur_buffer->flags &= ~DWF_SPECIAL_SORT;
				break;

			// Custom title/header
			case RXCMD_TITLE:
			case RXCMD_HEADER:

				// Lock current buffer
				buffer_lock(lister->cur_buffer, TRUE);

				// Return old title as the result
				strcpy(
					result,
					(id == RXCMD_TITLE) ? lister->cur_buffer->buf_CustomTitle : lister->cur_buffer->buf_CustomHeader);

				// Copy title
				rexx_skip_space(&args);
				stccpy((id == RXCMD_TITLE) ? lister->cur_buffer->buf_CustomTitle : lister->cur_buffer->buf_CustomHeader,
					   args,
					   (id == RXCMD_TITLE) ? 31 : 80);

				// Unlock buffer
				buffer_unlock(lister->cur_buffer);
				break;

			// Field title
			case RXCMD_FIELD:

				// Skip whitespaces
				rexx_skip_space(&args);

				// On or off?
				if ((val = rexx_match_keyword(&args, on_off_strings2, 0)) > -1)
				{
					// Turn on?
					if (val % 2)
						lister->cur_buffer->more_flags &= ~DWF_HIDE_TITLE;

					// Turn off
					else
						lister->cur_buffer->more_flags |= DWF_HIDE_TITLE;
					break;
				}

				// Get title number
				while ((val = rexx_parse_number(&args, 0, -1)) > -1 && val < DISPLAY_LAST)
				{
					char label[40];

					// Get label
					rexx_skip_space(&args);
					rexx_parse_word(&args, label, 40);
					rexx_skip_space(&args);

					// Free existing label
					FreeMemH(lister->cur_buffer->buf_TitleFields[val]);
					lister->cur_buffer->buf_TitleFields[val] = 0;

					// Valid label?
					if (label[0])
					{
						// Allocate label copy
						if ((lister->cur_buffer->buf_TitleFields[val] =
								 AllocMemH(lister->cur_buffer->memory, strlen(label) + 1)))
						{
							// Copy label
							strcpy(lister->cur_buffer->buf_TitleFields[val], label);
						}
					}
				}
				break;

			// Progress indicator
			case RXCMD_PROGRESS: {
				long value;
				char *copy;

				// Get progress item
				id = rexx_get_command(&args);

				// Skip whitespaces
				rexx_skip_space(&args);

				// Look at progress item
				switch (id)
				{
				// Open progress requester
				default: {
					ProgressPacket *packet;

					// Allocate packet
					if ((packet = AllocVec(sizeof(ProgressPacket), MEMF_CLEAR)))
					{
						// Get total
						packet->total = rexx_parse_number(&args, 0, 0);

						// Skip whitespaces to get text
						rexx_skip_space(&args);

						// Get operation
						packet->operation = args;

						// Set flags
						if (packet->total >= 0)
							packet->flags = PWF_FILENAME | PWF_GRAPH;

						// Filename, no graph?
						else if (packet->total == -2)
						{
							packet->flags = PWF_FILENAME;
							packet->total = 0;
						}

						// Something else
						else
							packet->total = 0;

						// If window isn't open, don't open progress window
						if (!lister->window)
							packet->flags |= PWF_INVISIBLE;

						// Display progress indicator
						IPC_Command(lister->ipc, LISTER_PROGRESS_ON, 0, 0, packet, REPLY_NO_PORT);
					}
				}
				break;

				// Set progress count
				case RXCMD_COUNT:

					// Get count value
					value = rexx_parse_number(&args, 0, 0);

					// Set update message
					IPC_Command(lister->ipc, LISTER_PROGRESS_COUNT, value, 0, 0, 0);
					break;

				// Set progress filename
				case RXCMD_NAME:

					// Allocate copy of name
					if ((copy = AllocMemH(0, strlen(args) + 1)))
					{
						// Copy name
						strcpy(copy, args);

						// Send update message
						IPC_Command(lister->ipc, LISTER_PROGRESS_UPDATE, 0, copy, 0, 0);
					}
					break;
				}
			}
			break;

			// New progress indicator
			case RXCMD_NEWPROGRESS:
				if (!(rexx_lister_newprogress(lister, args, 0)))
					rc = RXERR_INVALID_SET;
				break;

			// Source
			case RXCMD_SOURCE:

				// Make lister the source
				IPC_Command(lister->ipc, LISTER_MAKE_SOURCE, (rexx_get_command(&args) == RXCMD_LOCK), 0, 0, 0);
				break;

			// Destination
			case RXCMD_DEST:

				// Make lister the destination
				IPC_Command(lister->ipc, LISTER_MAKE_DEST, (rexx_get_command(&args) == RXCMD_LOCK), 0, 0, 0);
				break;

			// Turn off
			case RXCMD_OFF:

				// Turn lister off
				IPC_Command(lister->ipc, LISTER_OFF, 0, 0, 0, 0);
				break;

			// Lock mode
			case RXCMD_LOCK:

				// Get keywords
				rexx_skip_space(&args);
				while ((val = rexx_match_keyword(&args, lock_keys, 0)) != -1)
				{
					short state;

					// Get state
					rexx_skip_space(&args);
					if ((state = rexx_match_keyword(&args, on_off_strings, 0)) != -1)
					{
						// Lock?
						if (val == 0)
						{
							// Turn on?
							if (state % 2)
								lister->cur_buffer->more_flags |= DWF_LOCK_STATE;

							// Turn off
							else
								lister->cur_buffer->more_flags &= ~DWF_LOCK_STATE;
						}

						// Format
						else
						{
							// Turn on?
							if (state % 2)
								lister->cur_buffer->more_flags |= DWF_LOCK_FORMAT;

							// Turn off
							else
								lister->cur_buffer->more_flags &= ~DWF_LOCK_FORMAT;
						}
					}
				}
				break;

			// Toolbar
			case RXCMD_TOOLBAR:

				// Get new toolbar
				rexx_skip_space(&args);

				// Tell lister to get toolbar
				IPC_Command(lister->ipc, LISTER_TOOLBAR, (IPTR)args, 0, 0, REPLY_NO_PORT);
				break;

			// Mode
			case RXCMD_MODE:
				rexx_set_lister_mode(lister, args);
				break;

			// Case sensitivity
			case RXCMD_CASE:

				// Get state
				rexx_skip_space(&args);
				if ((val = rexx_match_keyword(&args, on_off_strings, 0)) != -1)
				{
					// Turn on?
					if (val % 2)
						lister->cur_buffer->more_flags |= DWF_CASE;

					// Turn off
					else
						lister->cur_buffer->more_flags &= ~DWF_CASE;
				}
				break;

			// Name length
			case RXCMD_NAMELENGTH:

				// Get length
				rexx_skip_space(&args);
				val = rexx_parse_number(&args, 0, 32);

				// Minimum is 32, maximum is 254, and it must be even
				if (val < 32)
					val = 32;
				else if (val > 254)
					val = 254;
				else if (val & 1)
					++val;

				// Store value in buffer
				lister->cur_buffer->name_field_size = val;
				break;

			// Comment length
			case RXCMD_COMMENTLENGTH:

				// Get length
				rexx_skip_space(&args);
				val = rexx_parse_number(&args, 0, 80);

				// Minimum is 80, maximum is 254, and it must be even
				if (val < 80)
					val = 80;
				else if (val > 254)
					val = 254;
				else if (val & 1)
					++val;

				// Store value in buffer
				lister->cur_buffer->comment_field_size = val;
				break;

			// Set value
			case RXCMD_VALUE: {
				char name[40];
				struct Node *node, *find;

				// Get name
				rexx_skip_space(&args);
				rexx_parse_word(&args, name, 40);

				// Get value pointer
				rexx_skip_space(&args);

				// See if old value exists
				lock_listlock(&lister->user_data_list, TRUE);
				if ((find = FindNameI(&lister->user_data_list.list, name)))
				{
					// Save old value and free it
					stccpy(result, (char *)(find + 1), sizeof(result) - 1);
					Remove(find);
					FreeMemH(find);
				}

				// Allocate new value tag
				if ((node = AllocMemH(lister->memory, sizeof(struct Node) + strlen(args) + 1 + strlen(name) + 1)))
				{
					strcpy((char *)(node + 1), args);
					node->ln_Name = ((char *)(node + 1)) + strlen(args) + 1;
					strcpy(node->ln_Name, name);
					AddTail(&lister->user_data_list.list, node);
				}
				else
					rc = RXERR_NO_MEMORY;

				// Unlock value list
				unlock_listlock(&lister->user_data_list);
			}
			break;

			// Unknown
			default:
				rc = RXERR_INVALID_SET;
				break;
			}
			break;

		// Add
		case RXCMD_ADD:

			// Add file
			rc = rexx_lister_add_file(lister, args, 0);
			break;

		// Reload
		case RXCMD_RELOAD:

			// Reload file
			rc = rexx_lister_reload_file(lister, args);
			break;

		// Add from stem
		case RXCMD_ADD_STEM:

			// Add file
			rc = rexx_lister_add_file(lister, args, msg);
			break;

		// Remove
		case RXCMD_REMOVE:

			// Remove file
			rc = rexx_lister_remove_file(lister, args);
			break;

		// Select
		case RXCMD_SELECT:

			// Select file
			rc = rexx_lister_select_file(lister, args, result);
			break;

		// Movement
		case RXCMD_MOVEMENT:

			// Handle movement
			rexx_skip_space(&args);
			if (rexx_lister_movement(lister, args))
			{
				// Return current position
				lsprintf(result, "%ld %ld", lister->cur_buffer->buf_VertOffset, lister->cur_buffer->buf_HorizOffset);
			}
			else
				rc = RXERR_INVALID_SET;
			break;

		// Empty lister
		case RXCMD_EMPTY: {
			char handler[32];
			long flags;

			// Save handler and flags
			strcpy(handler, lister->cur_buffer->buf_CustomHandler);
			flags = lister->cur_buffer->cust_flags;

			// Find an empty buffer for this lister
			IPC_Command(lister->ipc, LISTER_BUFFER_FIND_EMPTY, 0, 0, 0, REPLY_NO_PORT);

			// Set handler and flags
			strcpy(lister->cur_buffer->buf_CustomHandler, handler);
			lister->cur_buffer->cust_flags = flags;

			// Set lister pointer
			lister->cur_buffer->buf_OwnerLister = lister;
		}
		break;

		// Find a cached buffer
		case RXCMD_FINDCACHE: {
			// Find path
			rexx_skip_space(&args);

			// Find the cached path
			lsprintf(result,
					 "%ld",
					 IPC_Command(lister->ipc,
								 LISTER_FIND_CACHED_BUFFER,
								 (IPTR)lister->cur_buffer->buf_CustomHandler,
								 args,
								 0,
								 REPLY_NO_PORT));
		}
		break;

		// Copy to another lister
		case RXCMD_COPY: {
			Lister *dest_lister;

			// Get destination lister handle
			rexx_skip_space(&args);
			dest_lister = (Lister *)rexx_parse_iptr(&args, 0, 0);

			// Check it's valid
			if (rexx_lister_valid(dest_lister))
			{
				// Copy buffer
				IPC_Command(dest_lister->ipc, LISTER_COPY_BUFFER, 0, lister->cur_buffer, 0, 0);
			}

			else
				rc = RXERR_INVALID_HANDLE;
		}
		break;

		// Iconify
		case RXCMD_ICONIFY:

			// Get state
			rexx_skip_space(&args);
			val = rexx_match_keyword(&args, on_off_strings, 0);

			// Iconify?
			if (val == -1 || val & 1)
			{
				if (all_listers)
					IPC_ListCommand(&GUI->lister_list, LISTER_ICONIFY, 0, 0, 0);
				else
					IPC_Command(lister->ipc, LISTER_ICONIFY, 0, 0, 0, 0);
			}

			// Uniconify
			else
			{
				if (all_listers)
					IPC_ListCommand(&GUI->lister_list, LISTER_UNICONIFY, 0, 0, 0);
				else
					IPC_Command(lister->ipc, LISTER_UNICONIFY, 0, 0, 0, 0);
			}
			break;

		// Get a string/request a choice
		case RXCMD_GETSTRING:
		case RXCMD_REQUEST: {
			RexxRequester *req;

			// Skip any spaces
			rexx_skip_space(&args);

			// Allocate requester argument packet
			if ((req = AllocVec(sizeof(RexxRequester) + strlen(args), MEMF_CLEAR)))
			{
				// Store message
				req->msg = msg;
				req->command = command;
				req->window = lister->window;

				// Copy arguments
				strcpy(req->args, args);

				// Startup requester process
				if (!(misc_startup("dopus_rexx_request", REXX_REQUESTER, GUI->window, req, 0)))
					FreeVec(req);
				else
				{
					ret = 1;
					msg = 0;
				}
			}
		}
		break;

		// Free caches
		case RXCMD_FREECACHES: {
			char handler[40];

			// Skip any spaces, get handler
			rexx_skip_space(&args);
			rexx_parse_word(&args, handler, 40);

			// Free caches
			lister_free_caches(lister, handler);
		}
		break;
		}
	}

	// Unlock lister list
	unlock_listlock(&GUI->lister_list);

	// Set return
	if (rc != -1 && msg)
		rexx_set_return(msg, rc, result);
	return ret;
}

// Build rexx file return
void rexx_lister_file_return(struct RexxMsg *msg, DirBuffer *buffer, short id, char sep, short var_flag, char *varname)
{
	short type, num = 0;
	BOOL selected = 0;
	DirEntry *entry;
	long size = 0, count = 0;
	char *string = 0, sep_string[3];

	// If setting stem, set count to 0 initially
	if (var_flag == RETURN_STEM)
		rexx_set_var(msg, varname, "COUNT", 0, RX_LONG);

	// If we haven't been asked for a result code, return
	if (!(msg->rm_Action & (1 << RXFB_RESULT)) && var_flag == RETURN_RESULT)
		return;

	// Get type we want
	if (id == RXCMD_FILES || id == RXCMD_SELFILES)
		type = ENTRY_FILE;
	else if (id == RXCMD_DIRS || id == RXCMD_SELDIRS)
		type = ENTRY_DIRECTORY;
	else
		type = ENTRY_ANYTHING;

	// Selected?
	if (id == RXCMD_SELFILES || id == RXCMD_SELDIRS || id == RXCMD_SELENTRIES || id == RXCMD_FIRSTSEL)
		selected = 1;

	// Need a result string?
	if (var_flag != RETURN_STEM)
	{
		// Go through entries, calculate size of string needed
		entry = (DirEntry *)&buffer->entry_list;
		while ((entry = get_entry((struct MinList *)entry, selected, type)))
		{
			size += strlen(entry->de_Node.dn_Name) + 3;
			if (id == RXCMD_FIRSTSEL)
				break;
		}

		// Nothing there?
		if (size == 0)
		{
			// Variable?
			if (var_flag == RETURN_VAR)
				rexx_set_var(msg, varname, 0, (IPTR) "", RX_STRING);

			else
				// Didn't ask for a result?
				if (!(msg->rm_Action & RXFF_RESULT))
			{
				msg->rm_Result2 = 0;
				return;
			}

			else
				// Result string
				if (!(msg->rm_Result2 = (IPTR)CreateArgstring(0, 0)))
				msg->rm_Result1 = RXERR_NO_MEMORY;
			return;
		}

		// Allocate result string
		if (!(string = AllocVec(size, MEMF_CLEAR)))
		{
			msg->rm_Result1 = RXERR_NO_MEMORY;
			return;
		}

		// String that will separate entries
		sep_string[0] = (sep && var_flag == RETURN_RESULT) ? sep : ' ';
		sep_string[1] = '\"';
		sep_string[2] = 0;
	}

	// Go through entries again and build string
	entry = (DirEntry *)&buffer->entry_list;
	while ((entry = get_entry((struct MinList *)entry, selected, type)))
	{
		// Setting stem?
		if (var_flag == RETURN_STEM)
		{
			char buf[12];

			// Build variable name
			lsprintf(buf, "%ld", count++);

			// Set rexx variable
			rexx_set_var(msg, varname, buf, (IPTR)entry->de_Node.dn_Name, RX_STRING);
		}

		// Otherwise, building string
		else
		{
			// Add name, in quotes
			strcat(string, ((num++) == 0) ? "\"" : sep_string);
			strcat(string, entry->de_Node.dn_Name);
			strcat(string, "\"");
		}

		// Break if we only want the first
		if (id == RXCMD_FIRSTSEL)
			break;
	}

	// Stem?
	if (var_flag == RETURN_STEM)
		rexx_set_var(msg, varname, "COUNT", count, RX_LONG);

	// Variable?
	else if (var_flag == RETURN_VAR)
		rexx_set_var(msg, varname, 0, (IPTR)string, RX_STRING);

	else
		// Didn't ask for a result?
		if (!(msg->rm_Action & RXFF_RESULT))
		msg->rm_Result2 = 0;

	else
		// Result string
		if (!(msg->rm_Result2 = (IPTR)CreateArgstring(string, strlen(string))))
		msg->rm_Result1 = RXERR_NO_MEMORY;

	// Free temporary string
	FreeVec(string);
}

// Return information about an entry
void rexx_lister_entry_info(struct RexxMsg *msg, DirBuffer *buffer, char *args, short var_flag, char *varname)
{
	char name[364], sep;
	long num;
	DirEntry *entry;
	char *string, *ptr;
	ULONG date;
	short type;
#ifdef USE_64BIT
	char sizebuf[32];
#endif

	// If we haven't been asked for a result code, return
	if (!(msg->rm_Action & (1 << RXFB_RESULT)) && var_flag == RETURN_RESULT)
		return;

	// Get file
	if (!(entry = rexx_lister_get_file(buffer, &args)))
	{
		msg->rm_Result1 = RXERR_INVALID_NAME;
		return;
	}

	// Get date value
	date = (entry->de_Date.ds_Days * 86400) + (entry->de_Date.ds_Minute * 60) + (entry->de_Date.ds_Tick / 50);
#ifdef USE_64BIT
	ItoaU64(&entry->de_Size, sizebuf, sizeof(sizebuf), 0);
#endif

	// Get entry type
	if (entry->de_Node.dn_Type == 0)
		type = 0;

	// File?
	else if (entry->de_Node.dn_Type < 0)
	{
		// Colour link?
		if ((entry->de_Flags & (ENTF_LINK | ENTF_COLOUR_DEVICE)) == (ENTF_LINK | ENTF_COLOUR_DEVICE))
			type = RXENTRY_FILE_COLOUR_LINK;

		// Link?
		else if (entry->de_Flags & ENTF_LINK)
			type = RXENTRY_FILE_LINK;

		// Colour?
		else if (entry->de_Flags & ENTF_COLOUR_DEVICE)
			type = RXENTRY_FILE_COLOUR;

		// Plain file
		else
			type = RXENTRY_FILE;
	}

	// Directory
	else
	{
		// Colour link?
		if ((entry->de_Flags & (ENTF_LINK | ENTF_COLOUR_DEVICE)) == (ENTF_LINK | ENTF_COLOUR_DEVICE))
			type = RXENTRY_DIR_COLOUR_LINK;

		// Link?
		else if (entry->de_Flags & ENTF_LINK)
			type = RXENTRY_DIR_LINK;

		// Colour?
		else if (entry->de_Flags & ENTF_COLOUR_DEVICE)
			type = RXENTRY_DIR_COLOUR;

		// Plain directory
		else
			type = RXENTRY_DIR;
	}

	// Stem?
	if (var_flag == RETURN_STEM)
	{
		char datebuf[20];
		VersionInfo *ver;

		// Return information
		rexx_set_var(msg, varname, "NAME", (IPTR)entry->de_Node.dn_Name, RX_STRING);
#ifdef USE_64BIT
		rexx_set_var(msg, varname, "SIZE", (IPTR)sizebuf, RX_STRING);
#else
		rexx_set_var(msg, varname, "SIZE", entry->de_Size, RX_LONG);
#endif
		rexx_set_var(msg, varname, "TYPE", type, RX_LONG);
		rexx_set_var(msg, varname, "SELECTED", (entry->de_Flags & ENTF_SELECTED) ? 1 : 0, RX_LONG);
		rexx_set_var(msg, varname, "DATE", date, RX_LONG);
		rexx_set_var(msg, varname, "PROTECT", entry->de_Protection, RX_LONG);
		rexx_set_var(msg, varname, "PROTSTRING", (IPTR)entry->de_ProtBuf, RX_STRING);
		rexx_set_var(msg, varname, "COMMENT", GetTagData(DE_Comment, 0, entry->de_Tags), RX_STRING);
		rexx_set_var(msg, varname, "DATESTRING", (IPTR)entry->de_DateBuf, RX_STRING);
		rexx_set_var(msg, varname, "FILETYPE", GetTagData(DE_Filetype, 0, entry->de_Tags), RX_STRING);
		rexx_set_var(msg, varname, "USERDATA", GetTagData(DE_UserData, 0, entry->de_Tags), RX_LONG);
		rexx_set_var(msg, varname, "DISPLAY", GetTagData(DE_DisplayString, 0, entry->de_Tags), RX_STRING);

		// Get date string
		date_string(entry->de_Date.ds_Days, datebuf, FORMAT_CDN, 0);
		rexx_set_var(msg, varname, "DATENUM", (IPTR)datebuf, RX_STRING);

		// Get time string
		time_string(entry->de_Date.ds_Minute, entry->de_Date.ds_Tick, datebuf);
		rexx_set_var(msg, varname, "TIME", (IPTR)datebuf, RX_STRING);

		// Got version info?
		if (entry->de_Flags & ENTF_VERSION && (ver = (VersionInfo *)GetTagData(DE_VersionInfo, 0, entry->de_Tags)))
		{
			// Set version & revision
			rexx_set_var(msg, varname, "VERSION", (IPTR)ver->vi_Version, RX_LONG);
			rexx_set_var(msg, varname, "REVISION", (IPTR)ver->vi_Revision, RX_LONG);

			// Got date?
			if (ver->vi_Days > 0)
			{
				// Build date string
				date_string(ver->vi_Days, datebuf, FORMAT_CDN, 0);
				rexx_set_var(msg, varname, "VERDATE", (IPTR)datebuf, RX_STRING);
			}
		}

		// Clear version
		else
		{
			rexx_set_var(msg, varname, "VERSION", 0, RX_STRING);
			rexx_set_var(msg, varname, "REVISION", 0, RX_STRING);
			rexx_set_var(msg, varname, "VERDATE", 0, RX_STRING);
		}
		return;
	}

	// Get separator
	rexx_skip_space(&args);
	if (var_flag == RETURN_RESULT && *args)
		sep = *args;
	else
		sep = ' ';

#ifdef USE_64BIT
	lsprintf(name,
			 "%s%lc%s%lc%ld%lc%ld%lc%ld%lc%s%lc",
			 (IPTR)entry->de_Node.dn_Name,
			 sep,
			 (IPTR)sizebuf,
			 sep,
			 type,
			 sep,
			 (entry->de_Flags & ENTF_SELECTED) ? 1 : 0,
			 sep,
			 (entry->de_Date.ds_Days * 86400) + (entry->de_Date.ds_Minute * 60) + (entry->de_Date.ds_Tick / 50),
			 sep,
			 (IPTR)entry->de_ProtBuf,
			 sep);
#else
	lsprintf(name,
			 "%s%lc%lu%lc%ld%lc%ld%lc%ld%lc%s%lc",
			 (IPTR)entry->de_Node.dn_Name,
			 sep,
			 entry->de_Size,
			 sep,
			 type,
			 sep,
			 (entry->de_Flags & ENTF_SELECTED) ? 1 : 0,
			 sep,
			 (entry->de_Date.ds_Days * 86400) + (entry->de_Date.ds_Minute * 60) + (entry->de_Date.ds_Tick / 50),
			 sep,
			 (IPTR)entry->de_ProtBuf,
			 sep);
#endif

	// Get comment pointer
	ptr = (char *)GetTagData(DE_Comment, 0, entry->de_Tags);

	// Calculate length of string needed
	num = strlen(name) + 1 + ((ptr) ? strlen(ptr) : 0);

	// Allocate string
	if (!(string = AllocVec(num, MEMF_CLEAR)))
	{
		msg->rm_Result1 = RXERR_NO_MEMORY;
		return;
	}

	// Build string
	strcpy(string, name);
	if (ptr)
		strcat(string, ptr);

	// Set variable?
	if (var_flag == RETURN_VAR)
		rexx_set_var(msg, varname, 0, (IPTR)string, RX_STRING);

	else
		// Didn't ask for a result?
		if (!(msg->rm_Action & RXFF_RESULT))
		msg->rm_Result2 = 0;

	else
		// Result string
		if (!(msg->rm_Result2 = (IPTR)CreateArgstring(string, strlen(string))))
		msg->rm_Result1 = RXERR_NO_MEMORY;

	// Free temp string
	FreeVec(string);
}

// Get current lister handles
void rexx_lister_get_current(struct RexxMsg *msg, short type, short var_flag, char *varname)
{
	Lister *lister;
	IPCData *ipc;
	short num, count = 0;
	char *string = 0, buf[32];

	// If setting stem, set count to 0 initially
	if (var_flag == RETURN_STEM)
		rexx_set_var(msg, varname, "COUNT", 0, RX_LONG);

	// Get list lock
	lock_listlock(&GUI->lister_list, FALSE);

	// Go through listers
	for (ipc = (IPCData *)GUI->lister_list.list.lh_Head, num = 0; ipc->node.mln_Succ;
		 ipc = (IPCData *)ipc->node.mln_Succ)
	{
		// Get lister pointer
		lister = (Lister *)IPCDATA(ipc);

		// Is lister not busy and not closing, and not an icon view?
		if (!(lister->flags & (LISTERF_BUSY | LISTERF_CLOSING)) &&
			(!(lister->flags & LISTERF_VIEW_ICONS) || lister->flags & LISTERF_ICON_ACTION))
		{
			// Is lister the right type?
			if (type == RXCMD_ALL ||
				(type == RXCMD_ACTIVE && lister_valid_window(lister) && lister->window->Flags & WFLG_WINDOWACTIVE) ||
				(type == RXCMD_SOURCE && lister->flags & LISTERF_SOURCE) ||
				(type == RXCMD_DEST && lister->flags & LISTERF_DEST))
			{
				// Increment count
				++num;
				lister->flags |= LISTERF_TEMP_FLAG;
			}
		}

		/* ACTION_CHANGE */
		// Or, is it a source icon mode and we're after a source?
		else if (GUI->current_lister == lister && lister->flags & LISTERF_VIEW_ICONS &&
				 !(lister->flags & LISTERF_ICON_ACTION) &&
				 (type == RXCMD_ALL || type == RXCMD_SOURCE || type == RXCMD_ACTIVE))
		{
			// Increment count
			++num;
			lister->flags |= LISTERF_TEMP_FLAG;
		}
	}

	// Didn't find any listers?
	if (num == 0)
	{
		// Free list lock
		unlock_listlock(&GUI->lister_list);
		msg->rm_Result1 = RXERR_NO_LISTER;
		return;
	}

	// Allocate result string
	if (var_flag != RETURN_STEM && !(string = AllocVec(num * (sizeof(buf) - 1) + 1, MEMF_CLEAR)))
	{
		msg->rm_Result1 = RXERR_NO_MEMORY;
	}
	else
	{
		// Go through listers again
		for (ipc = (IPCData *)GUI->lister_list.list.lh_Head; ipc->node.mln_Succ; ipc = (IPCData *)ipc->node.mln_Succ)
		{
			// Get lister pointer
			lister = (Lister *)IPCDATA(ipc);

			// One of ours?
			if (lister->flags & LISTERF_TEMP_FLAG)
			{
				// Setting stem?
				if (var_flag == RETURN_STEM)
				{
					char name[12];
					char handle[32];

					// Build variable
					lsprintf(name, "%ld", count++);
					rexx_format_iptr(handle, (IPTR)lister);

					// Set rexx variable
					rexx_set_var(msg, varname, name, (IPTR)handle, RX_STRING);
				}

				// Otherwise, add handle to string
				else
				{
					rexx_format_iptr(buf, (IPTR)lister);
					strcat(buf, " ");
					strcat(string, buf);
				}

				// Clear temp flag
				lister->flags &= ~LISTERF_TEMP_FLAG;
			}
		}

		// Setting stem?
		if (var_flag == RETURN_STEM)
			rexx_set_var(msg, varname, "COUNT", count, RX_LONG);

		// Setting variable?
		else if (var_flag == RETURN_VAR)
			rexx_set_var(msg, varname, 0, (IPTR)string, RX_STRING);

		else
			// Didn't ask for a result?
			if (!(msg->rm_Action & RXFF_RESULT))
			msg->rm_Result2 = 0;

		// Result string
		else if (!(msg->rm_Result2 = (IPTR)CreateArgstring(string, strlen(string))))
			msg->rm_Result1 = RXERR_NO_MEMORY;

		// Free temporary string
		FreeVec(string);
	}

	// Free list lock
	unlock_listlock(&GUI->lister_list);
}

// Query format information
void rexx_query_format(short id, ListFormat *format, char *result)
{
	// What sort of information?
	switch (id)
	{
	// Sort method
	case RXCMD_SORT:
		strcpy(result, sort_method_strings[format->sort.sort]);
		break;

	// Separation method
	case RXCMD_SEPARATE:
		strcpy(result, separate_method_strings[format->sort.separation]);
		break;

	// Display items
	case RXCMD_DISPLAY: {
		short item;

		// Go through display items
		for (item = 0; item < 16; item++)
		{
			// Invalid?
			if (format->display_pos[item] == -1)
				break;

			// Add to string
			if (item > 0)
				strcat(result, " ");
			strcat(result, sort_method_strings[format->display_pos[item]]);
		}
	}
	break;

	// Flags
	case RXCMD_FLAGS:

		// Clear buffer
		result[0] = 0;

		// Reverse?
		if (format->sort.sort_flags & SORT_REVERSE)
			strcat(result, sort_flags_strings[RXSORTF_REVERSE]);

		// Filter icons
		if (format->flags & LFORMATF_REJECT_ICONS)
		{
			if (result[0])
				strcat(result, " ");
			strcat(result, sort_flags_strings[RXSORTF_NOICONS]);
		}

		// Hidden bit
		if (format->flags & LFORMATF_HIDDEN_BIT)
		{
			if (result[0])
				strcat(result, " ");
			strcat(result, sort_flags_strings[RXSORTF_HIDDEN]);
		}
		break;

	// Show pattern
	case RXCMD_SHOW:
		strcpy(result, format->show_pattern);
		break;

	// Hide pattern
	case RXCMD_HIDE:
		strcpy(result, format->hide_pattern);
		break;
	}
}

// Set lister format
long rexx_set_format(short command, short id, ListFormat *format, char *args)
{
	short key;

	// Set what?
	switch (id)
	{
	// Default format
	case RXCMD_DEFAULT:

		// Copy default format
		*format = environment->env->list_format;
		break;

	// Sort method
	case RXCMD_SORT:

		// Match key word
		if ((key = rexx_match_keyword(&args, sort_method_strings, 0)) == -1)
			return RXERR_INVALID_KEYWORD;

		// Store new sort method
		format->sort.sort = key;
		break;

	// Separate method
	case RXCMD_SEPARATE:

		// Match key word
		if ((key = rexx_match_keyword(&args, separate_method_strings, 0)) == -1)
			return RXERR_INVALID_KEYWORD;

		// Store new separation method
		format->sort.separation = key;
		break;

	// Display items
	case RXCMD_DISPLAY: {
		short item;
		BYTE display[16];

		// Go through display items
		for (item = 0; item < 16 && *args; item++)
		{
			// Match key word
			if ((key = rexx_match_keyword(&args, sort_method_strings, 0)) == -1)
				return RXERR_INVALID_KEYWORD;

			// Store new display item
			display[item] = key;
		}

		// Fill in remainder of array
		for (; item < 16; item++)
			display[item] = -1;

		// Store in configuration
		for (item = 0; item < 16; item++)
			format->display_pos[item] = display[item];
	}
	break;

	// Flags
	case RXCMD_FLAGS: {
		long type;

		// Go through keywords
		while ((key = rexx_match_keyword(&args, sort_flags_strings, &type)) != -1)
		{
			// Look at key
			switch (key)
			{
			// Reverse
			case RXSORTF_REVERSE:
				if (command == RXCMD_CLEAR || type == KEY_SUB)
					format->sort.sort_flags &= ~SORT_REVERSE;
				else if (type == KEY_TOGGLE)
					format->sort.sort_flags ^= SORT_REVERSE;
				else
					format->sort.sort_flags |= SORT_REVERSE;
				break;

			// Filter icons
			case RXSORTF_NOICONS:
				if (command == RXCMD_CLEAR || type == KEY_SUB)
					format->flags &= ~LFORMATF_REJECT_ICONS;
				else if (type == KEY_TOGGLE)
					format->flags ^= LFORMATF_REJECT_ICONS;
				else
					format->flags |= LFORMATF_REJECT_ICONS;
				break;

			// Hidden bit
			case RXSORTF_HIDDEN:
				if (command == RXCMD_CLEAR || type == KEY_SUB)
					format->flags &= ~LFORMATF_HIDDEN_BIT;
				else if (type == KEY_TOGGLE)
					format->flags ^= LFORMATF_HIDDEN_BIT;
				else
					format->flags |= LFORMATF_HIDDEN_BIT;
				break;
			}
		}
	}
	break;

	// Show pattern
	case RXCMD_SHOW:
		stccpy(format->show_pattern, args, 39);
		ParsePatternNoCase(format->show_pattern, format->show_pattern_p, 80);
		break;

	// Hide pattern
	case RXCMD_HIDE:
		stccpy(format->hide_pattern, args, 39);
		ParsePatternNoCase(format->hide_pattern, format->hide_pattern_p, 80);
		break;
	}

	return 0;
}

// New progress indicator
long rexx_lister_newprogress(Lister *lister, char *args, IPTR *retval)
{
	short key;
	APTR handle = 0;
	char *argptr;

	// Backup arg pointer
	argptr = args;

	// Skip spaces
	rexx_skip_space(&args);

	// No lister?
	if (!lister)
	{
		IPTR value;

		// Skip spaces
		rexx_skip_space(&args);

		// See if number is given
		if ((value = rexx_parse_iptr(&args, 0, (IPTR)-1)) != (IPTR)-1)
		{
			RexxProgress *look;

			// Skip spaces
			rexx_skip_space(&args);

			// Lock app list
			lock_listlock(&GUI->rexx_apps, FALSE);

			// Go through list
			for (look = (RexxProgress *)GUI->rexx_apps.list.lh_Head; look->node.ln_Succ;
				 look = (RexxProgress *)look->node.ln_Succ)
			{
				// Does it match?
				if (look == (RexxProgress *)value)
				{
					handle = look->progress;
					break;
				}
			}

			// Unlock app list
			unlock_listlock(&GUI->rexx_apps);

			// Not found?
			if (!handle)
				return 0;
		}
	}

	// Is progress indicator not open?
	if ((lister && !lister->progress_window) || (!lister && !handle))
	{
		ULONG flags = PWF_NOABORT;
		ProgressPacket *prog;

		// Match keys
		while ((key = rexx_match_keyword(&args, progress_keys, 0)) > -1)
		{
			// Get flag from key
			if (key == RXPROG_NAME)
				flags |= PWF_FILENAME;
			else if (key == RXPROG_FILE)
				flags |= PWF_FILESIZE;
			else if (key == RXPROG_INFO)
				flags |= PWF_INFO;
			else if (key == RXPROG_INFO2)
				flags |= PWF_INFO2;
			else if (key == RXPROG_INFO3)
				flags |= PWF_INFO3;
			else if (key == RXPROG_BAR)
				flags |= PWF_GRAPH;
			else if (key == RXPROG_ABORT)
				flags &= ~PWF_NOABORT;
		}

		// Got a lister?
		if (lister)
		{
			// Allocate progress packet
			if ((prog = AllocVec(sizeof(ProgressPacket), MEMF_CLEAR)))
			{
				// Set flags
				prog->flags = flags;

				// Display progress indicator
				IPC_Command(lister->ipc, LISTER_PROGRESS_ON, 0, 0, prog, REPLY_NO_PORT);
			}
		}

		// Global bar
		else
		{
			// Want abort?
			if (!(flags & PWF_NOABORT))
				flags |= PWF_ABORT;

			// Open indicator
			if ((handle = OpenProgressWindowTags(PW_Screen, GUI->screen_pointer, PW_Flags, flags, TAG_END)))
			{
				RexxProgress *prog;

				// Allocate node
				if ((prog = AllocVec(sizeof(RexxProgress), MEMF_CLEAR)))
				{
					// Set type and pointer
					prog->node.ln_Type = REXXAPP_PROGRESS;
					prog->progress = handle;

					// Add to AppList
					lock_listlock(&GUI->rexx_apps, TRUE);
					AddTail(&GUI->rexx_apps.list, (struct Node *)prog);
					unlock_listlock(&GUI->rexx_apps);

					// Send back handle
					if (retval)
						*retval = (IPTR)prog;
					return 1;
				}

				// Failed
				CloseProgressWindow(handle);
			}

			return 0;
		}

		return 1;
	}

	// Match key
	key = rexx_match_keyword(&args, progress_keys, 0);

	// Look at key
	switch (key)
	{
	// Filename
	case RXPROG_NAME:

		// Global bar?
		if (handle)
		{
			// Update filename
			SetProgressWindowTags(handle, PW_FileName, args, TAG_END);
		}

		// Got a lister
		else
		{
			char *name;

			// Allocate copy of name
			if ((name = AllocMemH(0, strlen(args) + 1)))
			{
				// Copy name
				strcpy(name, args);

				// Send update message
				IPC_Command(lister->ipc, LISTER_PROGRESS_UPDATE, 0, name, 0, 0);
			}
		}
		break;

	// Info/title
	case RXPROG_INFO:
	case RXPROG_INFO2:
	case RXPROG_INFO3:
	case RXPROG_TITLE:

		// Global bar?
		if (handle)
		{
			// Update info or title
			SetProgressWindowTags(handle,
								  (key == RXPROG_INFO)
									  ? PW_Info
									  : (key == RXPROG_INFO2) ? PW_Info2 : (key == RXPROG_INFO3) ? PW_Info3 : PW_Title,
								  args,
								  TAG_END);
		}

		// Got a lister
		else
		{
			// Send update command
			IPC_Command(lister->ipc,
						(key == RXPROG_TITLE) ? LISTER_PROGRESS_TITLE : LISTER_FILE_PROGRESS_INFO,
						(key == RXPROG_TITLE) ? 0 : key,
						args,
						0,
						REPLY_NO_PORT);
		}
		break;

	// File/bar info
	case RXPROG_FILE:
	case RXPROG_BAR: {
		long total, count;

		// Get values
		total = rexx_parse_number(&args, 0, 0);
		rexx_skip_space(&args);
		count = rexx_parse_number(&args, 0, 0);

		// Global bar?
		if (handle)
		{
			// Update file or progress info
			SetProgressWindowTags(handle,
								  (key == RXPROG_BAR) ? PW_FileCount : PW_FileSize,
								  total,
								  (key == RXPROG_BAR) ? PW_FileNum : PW_FileDone,
								  count,
								  TAG_END);
		}

		// Got a lister
		else
		{
			// Send update message
			IPC_Command(lister->ipc,
						(key == RXPROG_BAR) ? LISTER_PROGRESS_TOTAL : LISTER_FILE_PROGRESS_SET,
						total,
						(APTR)count,
						0,
						0);
		}
	}
	break;

	// Turn off
	case RXPROG_OFF:

		// Global bar?
		if (handle)
		{
			// Remove indicator
			rexx_rem_appthing(argptr, REXXAPP_PROGRESS);
		}

		// Got a lister
		else
		{
			// Turn it off
			IPC_Command(lister->ipc, LISTER_PROGRESS_OFF, 0, 0, 0, 0);
		}
		break;

	// Check abort
	case RXPROG_ABORT:

		// Global bar?
		if (handle)
		{
			// Return abort value
			if (retval)
			{
				BOOL abort = CheckProgressAbort(handle);

				// Return -1 for no abort
				if (!abort)
					*retval = (IPTR)-1;
				else
					*retval = 1;
			}
			return 1;
		}
		break;

	// Unknown
	default:

		// Return value
		if (retval)
			*retval = (IPTR)handle;
		return 0;
	}

	return 1;
}

// Set mode
void rexx_set_lister_mode(Lister *lister, char *args)
{
	ULONG flags = 0;
	short val;

	// Check mode keys
	rexx_skip_space(&args);
	while ((val = rexx_match_keyword(&args, mode_keys, 0)) != -1)
	{
		// Icon?
		if (val == 1)
			flags |= LISTERF_VIEW_ICONS;

		// Action?
		else if (val == 3)
			flags |= LISTERF_ICON_ACTION;

		// Show all?
		if (val == 2)
			flags |= LISTERF_SHOW_ALL;
	}

	// Send command to lister
	IPC_Command(lister->ipc, LISTER_MODE, flags, 0, 0, 0);
}

enum {
	RXMOVE_TO,
	RXMOVE_UP,
	RXMOVE_DOWN,
	RXMOVE_PGUP,
	RXMOVE_PGDN,
	RXMOVE_TOP,
	RXMOVE_BOTTOM,
	RXMOVE_LEFT,
	RXMOVE_RIGHT,
	RXMOVE_PGLT,
	RXMOVE_PGRT,
	RXMOVE_HOME,
	RXMOVE_END,
	RXMOVE_SHOW,
};

// Handle movement
BOOL rexx_lister_movement(Lister *lister, char *args)
{
	short val, refresh = 0;
	long vert, horiz, delta;
	DirBuffer *buffer = lister->cur_buffer;

	// Get current offsets
	vert = buffer->buf_VertOffset;
	horiz = buffer->buf_HorizOffset;

	// Check movement keys
	while ((val = rexx_match_keyword(&args, move_keys, 0)) != -1)
	{
		// Look at movement
		switch (val)
		{
		// Absolute location
		case RXMOVE_TO:
			if ((delta = rexx_parse_number(&args, 1, -1)) > -1)
				vert = delta;
			if ((delta = rexx_parse_number(&args, 0, -1)) > -1)
				horiz = delta;
			rexx_skip_space(&args);
			break;

		// Line up
		case RXMOVE_UP:
			if ((delta = rexx_parse_number(&args, 0, -1)) > 0)
			{
				vert -= delta;
				rexx_skip_space(&args);
			}
			else
				--vert;
			break;

		// Line down
		case RXMOVE_DOWN:
			if ((delta = rexx_parse_number(&args, 0, -1)) > 0)
			{
				vert += delta;
				rexx_skip_space(&args);
			}
			else
				++vert;
			break;

		// Page up
		case RXMOVE_PGUP:
			vert -= lister->text_height - 1;
			break;

		// Page down
		case RXMOVE_PGDN:
			vert += lister->text_height - 1;
			break;

		// Top
		case RXMOVE_TOP:
			vert = 0;
			break;

		// Bottom
		case RXMOVE_BOTTOM:
			vert = buffer->buf_TotalEntries[0] - lister->text_height;
			break;

		// Left
		case RXMOVE_LEFT:
			if ((delta = rexx_parse_number(&args, 0, -1)) > 0)
			{
				horiz -= delta;
				rexx_skip_space(&args);
			}
			else
				--horiz;
			break;

		// Right
		case RXMOVE_RIGHT:
			if ((delta = rexx_parse_number(&args, 0, -1)) > 0)
			{
				horiz += delta;
				rexx_skip_space(&args);
			}
			else
				++horiz;
			break;

		// Page left
		case RXMOVE_PGLT:
			horiz -= lister->text_width;
			break;

		// Page right
		case RXMOVE_PGRT:
			horiz += lister->text_width;
			break;

		// Home
		case RXMOVE_HOME:
			horiz = 0;
			break;

		// End
		case RXMOVE_END:
			horiz = 0xffff;
			break;

		// Show an entry
		case RXMOVE_SHOW: {
			DirEntry *entry;

			// Lock buffer
			buffer_lock(buffer, FALSE);

			// Get entry
			if ((entry = rexx_lister_get_file(buffer, &args)))
			{
				// Get offset
				if ((delta = find_entry_offset(&buffer->entry_list, entry)) > -1)
				{
					// Scroll to this one
					vert = delta;
				}
			}

			// Unlock buffer
			buffer_unlock(buffer);

			// Skip spaces
			rexx_skip_space(&args);
		}
		break;
		}
	}

	// Bounds check
	if (vert + lister->text_height > buffer->buf_TotalEntries[0])
		vert = buffer->buf_TotalEntries[0] - lister->text_height;
	if (vert < 0)
		vert = 0;
	if (horiz >= (buffer->buf_HorizLength - lister->text_width))
		horiz = buffer->buf_HorizLength - lister->text_width;
	if (horiz < 0)
		horiz = 0;

	// Have the offsets changed?
	if (vert != buffer->buf_VertOffset)
		refresh |= SLIDER_VERT;
	if (horiz != buffer->buf_HorizOffset)
		refresh |= SLIDER_HORZ;

	// Need to refresh?
	if (refresh)
	{
		// Set new position
		buffer->buf_VertOffset = vert;
		buffer->buf_HorizOffset = horiz;

		// If horizontal has changed, we need to redraw the whole display
		if (refresh & SLIDER_HORZ)
			buffer->buf_OldVertOffset = -1;

		// Refresh scroller and window
		lister_update_slider(lister, refresh);
		lister_display_dir(lister);
		return 1;
	}

	// Didn't move
	return 0;
}
