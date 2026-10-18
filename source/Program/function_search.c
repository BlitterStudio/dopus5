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

typedef struct
{
	char search_text[80];
	unsigned long search_flags;
	long search_result;
} SearchData;

static int search_get_data(FunctionHandle *handle, SearchData *data);

// SEARCH internal command
DOPUS_FUNC(function_search)
{
	PathNode *path;
	FunctionEntry *entry;
	long ret = 0;
	long count = 1;
	SearchData *data;
	APTR output = 0;

	// Get data
	data = (SearchData *)handle->inst_data;

	// Already got search text?
	if (data->search_text[0])
	{
		// Need output file?
		if (data->search_result == RESULT_OUTPUT)
		{
			if ((output = OpenBuf("t:DOpus-SearchResults", MODE_READWRITE, 512)))
			{
				SeekBuf(output, 0, OFFSET_END);
			}
		}
		else
			data->search_result = RESULT_LEAVE;
	}

	// Otherwise, set initial flags
	else
		data->search_flags = SEARCH_NOCASE;

	// Initialise flags
	handle->instruction_flags = 0;

	// Any directories selected?
	if (function_check_dirs(handle))
	{
		// Recurse?
		if (handle->inst_flags & INSTF_RECURSE)
			handle->instruction_flags = INSTF_RECURSE_DIRS;

		// Ask?
		else if (!(handle->inst_flags & INSTF_NO_RECURSE))
		{
			// Ask user
			if (!(ret = function_request(handle,
										 GetString(&locale, MSG_SEARCH_DIRECTORIES),
										 0,
										 GetString(&locale, MSG_YES),
										 GetString(&locale, MSG_NO),
										 GetString(&locale, MSG_CANCEL),
										 0)))
			{
				function_abort(handle);
				return 0;
			}

			// Recurse?
			if (ret == 1)
			{
				handle->inst_flags |= INSTF_RECURSE;
				handle->instruction_flags = INSTF_RECURSE_DIRS;
			}

			// No recurse
			else
				handle->inst_flags |= INSTF_NO_RECURSE;
		}
	}

	// Get current path
	if (!(path = function_path_current(&handle->source_paths)))
		return 0;

	// Turn progress indicator on
	function_progress_on(
		handle, GetString(&locale, MSG_PROGRESS_SEARCHING), handle->entry_count, PWF_FILENAME | PWF_GRAPH);

	// Go through entries
	while ((entry = function_get_entry(handle)))
	{
		BOOL file_ok = 1, break_flag = 0;

		// Update progress indicator
		if (function_progress_update(handle, entry, count))
		{
			function_abort(handle);
			ret = 0;
			break;
		}

		// Skip directories
		if (entry->type < 0)
		{
			APTR file;

			// Ask for search text?
			if (!data->search_text[0])
			{
				// Ask for search text
				if (!(search_get_data(handle, data)))
				{
					function_abort(handle);
					ret = 0;
					break;
				}

				// Need output file?
				if (data->search_result == RESULT_OUTPUT)
				{
					if ((output = OpenBuf("t:DOpus-SearchResults", MODE_NEWFILE, 512)))
					{
						lsprintf(handle->work_buffer, GetString(&locale, MSG_SEARCH_OUTPUT_HEADER), data->search_text);
						WriteBuf(output, handle->work_buffer, -1);
					}
					else
						data->search_result = RESULT_LEAVE;
				}
			}

			// Build source name
			function_build_source(handle, entry, handle->work_buffer);

			// By default no match
			ret = -1;

			// Open file
			if ((file = OpenBuf(handle->work_buffer, MODE_OLDFILE, 0)))
			{
				ret = SearchFile(file, data->search_text, data->search_flags, 0, 0);
				CloseBuf(file);
			}

			// Match?
			if (ret != -1)
			{
				// Got a match; what do we do with it?
				switch (data->search_result)
				{
				// Leave selected
				case RESULT_LEAVE:

					// If entry is recursive, abort this directory
					if (entry->flags & FUNCENTF_RECURSE)
					{
						handle->instruction_flags |= INSTF_ABORT_DIR;
					}

					// Otherwise leave file highlighted
					else
						file_ok = 0;
					break;

				// Prompt
				case RESULT_PROMPT: {
					Att_List *list;
					char buf[256];

					// Build requester text
					get_trunc_path(handle->work_buffer, buf);
					lsprintf(handle->work_buffer + 512, GetString(&locale, MSG_FOUND_A_MATCH), buf);

					// If not recursive, leave file selected
					if (!(entry->flags & FUNCENTF_RECURSE))
						file_ok = 0;

					// Ask what to do
					if (!(ret = function_request(handle,
												 handle->work_buffer + 512,
												 0,
												 GetString(&locale, MSG_SEARCH_READ),
												 GetString(&locale, MSG_SKIP),
												 GetString(&locale, MSG_SKIP_ALL),
												 GetString(&locale, MSG_ABORT),
												 0)))
					{
						// Is file in a directory?
						if (entry->flags & FUNCENTF_RECURSE)
						{
							char *ptr;
							PathNode *path;
							char *filename;

							// Get current path
							if ((filename = AllocVec(256, 0)) &&
								(path = function_path_current(&handle->source_paths)) && path->lister)
							{
								// Copy filename
								if ((ptr = FilePart(handle->work_buffer)))
								{
									strcpy(filename, ptr);
									*ptr = 0;
								}

								// Read directory
								handle->flags = GETDIRF_CANCHECKBUFS | GETDIRF_CANMOVEEMPTY;
								function_read_directory(
									handle, path->lister, function_path_side(path), handle->work_buffer);

								// Select file
								function_select_file(handle, path->lister, filename);
							}
							FreeVec(filename);
						}

						// Abort
						ret = 0;
						break_flag = 1;
						break;
					}

					// Skip?
					if (ret == 2 || ret == 3)
					{
						// Skip all?
						if (ret == 3)
						{
							data->search_result = RESULT_LEAVE;

							// If entry is recursive, abort this directory
							if (entry->flags & FUNCENTF_RECURSE)
								handle->instruction_flags |= INSTF_ABORT_DIR;
						}
						break;
					}

					// Create list for filename to read file
					if ((list = Att_NewList(0)))
					{
						struct read_startup *startup;

						// Add filename node
						Att_NewNode(list, handle->work_buffer, 0, 0);

						// Allocate startup
						if ((startup = AllocVec(sizeof(struct read_startup), MEMF_CLEAR)))
						{
							// Copy search string
							strcpy(startup->initial_search, data->search_text);
							startup->files = (struct List *)list;
							startup->wheel_scroll_lines = environment->env->env_wheel_scroll_lines;

							// Read file
							if (misc_startup("dopus_read", FUNC_READ, 0, startup, 0))
								break;
							FreeVec(startup);
						}
						Att_RemList(list, 0);
					}
					break;
				}

				// Output
				case RESULT_OUTPUT:

					if (output)
					{
						WriteBuf(output, handle->work_buffer, -1);
						WriteBuf(output, "\n", 1);
					}
					break;
				}
			}
		}

		// Break flag?
		if (break_flag)
			break;

		// Get next entry, increment count
		if (entry->entry)
			++count;
		function_end_entry(handle, entry, file_ok);

		// Reset result code
		ret = 1;
	}

	// Close output file
	CloseBuf(output);

	// Launch read on output file
	if (data->search_result == RESULT_OUTPUT)
	{
		Att_List *list;

		// Create list
		if ((list = Att_NewList(0)))
		{
			Att_Node *node;

			// Add node with filename
			if ((node = Att_NewNode(list, "t:DOpus-SearchResults", 0, 0)))
			{
				struct read_startup *startup;

				// Allocate startup
				if ((startup = AllocVec(sizeof(struct read_startup), MEMF_CLEAR)))
				{
					// Set flag to indicate delete file
					node->node.ln_Pri = 1;
					startup->files = (struct List *)list;
					startup->wheel_scroll_lines = environment->env->env_wheel_scroll_lines;

					// Read file
					if (misc_startup("dopus_read", FUNC_READ, 0, startup, FALSE))
						return ret;
					FreeVec(startup);
				}
			}
			Att_RemList(list, 0);
		}
	}

	return ret;
}

// Get search data
static int search_get_data(FunctionHandle *handle, SearchData *data)
{
	NewConfigWindow new_win;
	struct Window *window;
	ObjectList *objlist;
	Lister *lister;
	short cancel = 1;

	// Fill out new window
	if ((lister = function_lister_current(&handle->source_paths)))
	{
		new_win.parent = lister->window;
		new_win.flags = 0;
	}
	else
	{
		new_win.parent = GUI->screen_pointer;
		new_win.flags = WINDOW_SCREEN_PARENT;
	}
	new_win.dims = &search_window;
	new_win.title = GetString(&locale, MSG_ENTER_SEARCH_STRING);
	new_win.locale = &locale;
	new_win.port = 0;
	new_win.flags |= WINDOW_NO_CLOSE | WINDOW_VISITOR | WINDOW_AUTO_KEYS | WINDOW_REQ_FILL;
	new_win.font = 0;

	// Open window
	if (!(window = OpenConfigWindow(&new_win)) || !(objlist = AddObjectList(window, search_objects)))
	{
		CloseConfigWindow(window);
		return 0;
	}

	// Initial settings
	SetGadgetValue(objlist, GAD_SEARCH_TEXT, (IPTR)data->search_text);
	SetGadgetValue(objlist, GAD_SEARCH_CASE, data->search_flags & SEARCH_NOCASE);
	SetGadgetValue(objlist, GAD_SEARCH_WILD, data->search_flags & SEARCH_WILDCARD);
	SetGadgetValue(objlist, GAD_SEARCH_ONLYWORD, data->search_flags & SEARCH_ONLYWORDS);
	SetGadgetValue(objlist, GAD_SEARCH_RESULT, data->search_result);

	// Activate text field
	ActivateStrGad(GADGET(GetObject(objlist, GAD_SEARCH_TEXT)), window);

	// Event loop
	FOREVER
	{
		struct IntuiMessage *msg;
		BOOL break_flag = 0;

		// Check for abort
		if (function_check_abort(handle))
			break;

		// Activate?
		if (handle->flags2 & FUNCF_ACTIVATE_ME)
		{
			ActivateWindow(window);
			WindowToFront(window);
			handle->flags2 &= ~FUNCF_ACTIVATE_ME;
		}

		// Any Intuition messages?
		while ((msg = GetWindowMsg(window->UserPort)))
		{
			struct IntuiMessage copy_msg;

			// Copy message and reply
			copy_msg = *msg;
			ReplyWindowMsg(msg);

			// Gadget?
			if (copy_msg.Class == IDCMP_GADGETUP)
			{
				UWORD gadgetid;

				// Get gadget id
				gadgetid = ((struct Gadget *)copy_msg.IAddress)->GadgetID;

				// Look at gadget ID
				switch (gadgetid)
				{
				// Okay
				case GAD_SEARCH_TEXT:
				case GAD_SEARCH_OKAY:

					// Store flags
					data->search_flags = 0;
					if (GetGadgetValue(objlist, GAD_SEARCH_CASE))
						data->search_flags |= SEARCH_NOCASE;
					if (GetGadgetValue(objlist, GAD_SEARCH_WILD))
						data->search_flags |= SEARCH_WILDCARD;
					if (GetGadgetValue(objlist, GAD_SEARCH_ONLYWORD))
						data->search_flags |= SEARCH_ONLYWORDS;
					data->search_result = GetGadgetValue(objlist, GAD_SEARCH_RESULT);

					// Get search text
					strcpy(data->search_text, (char *)GetGadgetValue(objlist, GAD_SEARCH_TEXT));
					cancel = 0;

				// Cancel
				case GAD_SEARCH_CANCEL:
					break_flag = 1;
					break;
				}
			}
		}

		// Check break flag
		if (break_flag)
			break;

		// Wait for an event
		Wait(1 << window->UserPort->mp_SigBit | 1 << handle->ipc->command_port->mp_SigBit);
	}

	// Close window
	CloseConfigWindow(window);

	// Return
	return !cancel;
}
