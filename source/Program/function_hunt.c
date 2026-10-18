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

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct
{
	char pattern[40];
	short comment;
} HuntData;

#ifndef __amigaos3__
	#pragma pack()
#endif

// HUNT
DOPUS_FUNC(function_hunt)
{
	FunctionEntry *entry;
	long count = 1, ret = 1;
	HuntData *data;

	// Get data pointer
	data = (HuntData *)handle->inst_data;

	// Need to ask for pattern?
	if (!data->pattern[0])
	{
		// Get existing pattern
		GetSemaphore(&GUI->findfile_lock, SEMF_SHARED, 0);
		strcpy(data->pattern, GUI->findfile_string);
		FreeSemaphore(&GUI->findfile_lock);

		// Ask for hunt pattern
		if (!(function_request(handle,
							   GetString(&locale, MSG_HUNT_ENTER_FILENAME),
							   SRF_BUFFER | SRF_PATH_FILTER | SRF_CHECKMARK,
							   data->pattern,
							   40,
							   GetString(&locale, MSG_SEARCH_COMMENTS),
							   &data->comment,
							   GetString(&locale, MSG_OKAY),
							   GetString(&locale, MSG_CANCEL),
							   0)))
		{
			function_abort(handle);
			return 0;
		}

		// Store pattern
		GetSemaphore(&GUI->findfile_lock, SEMF_EXCLUSIVE, 0);
		strcpy(GUI->findfile_string, data->pattern);
		FreeSemaphore(&GUI->findfile_lock);

		// Parse pattern
		ParsePatternNoCase(data->pattern, handle->inst_data + 64, 128);
	}

	// Allow recursive directories
	handle->instruction_flags = INSTF_RECURSE_DIRS | INSTF_WANT_DIRS;

	// Display progress requester
	function_progress_on(
		handle, GetString(&locale, MSG_SCANNING_DIRECTORIES), handle->entry_count, PWF_FILENAME | PWF_GRAPH);

	// Go through files
	while ((entry = function_get_entry(handle)))
	{
		// Top-level directory?
		if (entry->flags & FUNCENTF_TOP_LEVEL && entry->type > 0)
		{
			// Update progress indicator
			if (function_progress_update(handle, entry, count++))
			{
				function_abort(handle);
				ret = 0;
				break;
			}
		}

		// Otherwise
		else
		{
			// Check abort
			if (function_check_abort(handle))
			{
				ret = 0;
				break;
			}

			// File?
			if (entry->type < 0)
			{
				BOOL match = 0;

				// Build full path
				function_build_source(handle, entry, handle->work_buffer + 512);

				// Match comments?
				if (data->comment)
				{
					D_S(struct FileInfoBlock, fib)

					// Get file information
					if (GetFileInfo(handle->work_buffer + 512, fib))
					{
						// Match comment
						match = MatchPatternNoCase(handle->inst_data + 64, fib->fib_Comment);
					}
				}

				// Match filename
				else
				{
					// Match name
					match = MatchPatternNoCase(handle->inst_data + 64, FilePart(entry->name));
				}

				// Does file match?
				if (match)
				{
					char *ptr, name[35];

					// Strip filename
					if ((ptr = FilePart(handle->work_buffer + 512)))
						*ptr = 0;
					get_trunc_filename(FilePart(entry->name), name);

					// Build requester text
					lsprintf(
						handle->work_buffer, GetString(&locale, MSG_HUNT_FOUND_FILE), name, handle->work_buffer + 512);

					// Display requester
					if (!(ret = function_request(handle,
												 handle->work_buffer,
												 0,
												 GetString(&locale, MSG_YES),
												 GetString(&locale, MSG_NEW_LISTER),
												 GetString(&locale, MSG_NO),
												 GetString(&locale, MSG_ABORT),
												 0)))
					{
						function_abort(handle);
						ret = 0;
						break;
					}

					// Go to that directory?
					if (ret == 1)
					{
						PathNode *path;

						// Get current lister
						if ((path = function_path_current(&handle->source_paths)) && path->lister)
						{
							// Read directory
							handle->flags = GETDIRF_CANCHECKBUFS | GETDIRF_CANMOVEEMPTY;
							function_read_directory(
								handle, path->lister, function_path_side(path), handle->work_buffer + 512);

							// Do wildcard selection
							function_select_file(
								handle, path->lister, (data->comment) ? (char *)FilePart(entry->name) : data->pattern);

							// Break out
							ret = 0;
							break;
						}
					}

					// New lister?
					else if (ret == 2)
					{
						Lister *lister;

						// Open new lister
						if ((lister = lister_open_new(
								 handle->work_buffer + 512, 0, 0, function_lister_current(&handle->source_paths))))
						{
							// Initialise lister
							IPC_Command(lister->ipc, LISTER_INIT, 0, GUI->screen_pointer, 0, 0);

							// Do wildcard selection
							function_select_file(
								handle, lister, (data->comment) ? (char *)FilePart(entry->name) : data->pattern);
						}
					}
				}
			}
		}

		// Get next entry
		function_end_entry(handle, entry, 1);
	}

	return ret;
}
