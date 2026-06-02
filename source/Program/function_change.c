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
#include "function_change.h"

static int function_change_get_comment(FunctionHandle *handle, char *file, char *buffer);
static int function_change_get_protect(FunctionHandle *handle, char *file, ULONG old_prot, unsigned char *masks);
static int function_change_get_date(FunctionHandle *handle, char *file, struct DateStamp *date);
static BOOL function_change_arg_empty(char *arg);

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef union
{
	char comment[80];
	unsigned char prot[2];
	struct DateStamp date;
} ChangeData;

#ifndef __amigaos3__
	#pragma pack()
#endif

// COMMENT/DATESTAMP/PROTECT internal function
DOPUS_FUNC(function_change)
{
	FunctionEntry *entry;
	char *source_file;
	PathNode *path;
	short ret = 0;
	int count = 1, action, error_action;
	ChangeData *data;
	BOOL progress = 0;

	// Get data
	data = (ChangeData *)handle->inst_data;

	// Initialise flags
	handle->instruction_flags = 0;

	// Got arguments?
	if (instruction->funcargs)
	{
		// Default to no recursing
		handle->inst_flags |= INSTF_NO_RECURSE;
	}

	// Get action strings
	if (command->function == FUNC_COMMENT)
	{
		action = MSG_PROGRESS_OPERATION_COMMENT;
		error_action = MSG_COMMENTING;

		// Arguments?
		if (instruction->funcargs)
		{
			// Comment supplied?
			if (instruction->funcargs->FA_Arguments[1])
			{
				// An empty (or quoted-empty) comment clears the filenote
				if (function_change_arg_empty((char *)instruction->funcargs->FA_Arguments[1]))
					data->comment[0] = 0;

				// Otherwise copy the supplied comment
				else
					stccpy(data->comment, (char *)instruction->funcargs->FA_Arguments[1], 80);

				handle->inst_flags |= INSTF_NO_ASK;
			}

			// Recursing?
			if (instruction->funcargs->FA_Arguments[2])
				handle->inst_flags |= INSTF_RECURSE;
		}
	}
	else if (command->function == FUNC_PROTECT)
	{
		action = MSG_PROGRESS_OPERATION_PROTECT;
		error_action = MSG_PROTECTING;

		// Arguments?
		if (instruction->funcargs)
		{
			// Protection supplied?
			if ((instruction->funcargs->FA_Arguments[2] || instruction->funcargs->FA_Arguments[3]))
			{
				short a;

				// Initialise protection masks
				data->prot[0] = 0;
				data->prot[1] = 0;

				// Two sets of bits
				for (a = 0; a < 2; a++)
				{
					// Bits supplied?
					if (instruction->funcargs->FA_Arguments[a + 2])
					{
						// Get bit value
						data->prot[a] = prot_from_string((char *)instruction->funcargs->FA_Arguments[a + 2]);

						// Toggle lower bits
						data->prot[a] = (data->prot[a] & 0xf0) | ((~data->prot[a]) & 0xf);
					}
				}
				handle->inst_flags |= INSTF_NO_ASK;
			}

			// Recursing?
			if (instruction->funcargs->FA_Arguments[1])
				handle->inst_flags |= INSTF_RECURSE;
		}
	}
	else
	{
		action = MSG_PROGRESS_OPERATION_DATESTAMP;
		error_action = MSG_DATESTAMPING;

		// Arguments supplied?
		if (instruction->funcargs)
		{
			// Date supplied?
			if (instruction->funcargs->FA_Arguments[2])
			{
				// Null date?
				if (function_change_arg_empty((char *)instruction->funcargs->FA_Arguments[2]))
					DateStamp(&data->date);

				// Otherwise
				else
				{
					// Convert to separate strings
					ParseDateStrings((char *)instruction->funcargs->FA_Arguments[2],
									 handle->work_buffer + 768,
									 handle->work_buffer + 896,
									 0);

					// Convert to datestamp
					if (!(DateFromStringsNew(handle->work_buffer + 768,
											 handle->work_buffer + 896,
											 &data->date,
											 environment->env->settings.date_format)))
						return 0;
				}
				handle->inst_flags |= INSTF_NO_ASK;
			}

			// Recursing?
			if (instruction->funcargs->FA_Arguments[1])
				handle->inst_flags |= INSTF_RECURSE;
		}
	}

	// Any directories selected?
	if (function_check_dirs(handle))
	{
		// Recurse?
		if (handle->inst_flags & INSTF_RECURSE)
			handle->instruction_flags = INSTF_RECURSE_DIRS | INSTF_WANT_DIRS | INSTF_WANT_DIRS_END;

		// Ask?
		else if (!(handle->inst_flags & INSTF_NO_RECURSE))
		{
			// Ask user
			if (!(ret = function_request(handle,
										 GetString(&locale, MSG_ACT_RECURSIVELY),
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
				handle->instruction_flags = INSTF_RECURSE_DIRS | INSTF_WANT_DIRS | INSTF_WANT_DIRS_END;
			}

			// No recurse
			else
				handle->inst_flags |= INSTF_NO_RECURSE;
		}
	}

	// Get current path
	if (!(path = function_path_current(&handle->source_paths)))
		return 0;

	// Tell this path to update it's datestamp at the end
	path->flags |= LISTNF_UPDATE_STAMP;

	// Allocate memory for source path
	if (!(source_file = AllocVec(256, 0)))
		return 0;

	// Go through entries
	while ((entry = function_get_entry(handle)))
	{
		BOOL file_ok = 1;

		// Update progress indicator
		if (function_progress_update(handle, entry, count))
		{
			function_abort(handle);
			ret = 0;
			break;
		}

		// Build source name
		function_build_source(handle, entry, source_file);

		// Do we have to ask?
		if (!(handle->inst_flags & INSTF_NO_ASK) && !(entry->flags & (FUNCENTF_RECURSE | FUNCENTF_EXITED)))
		{
			// Call appropriate "ask" routine
			if (command->function == FUNC_COMMENT)
			{
				char *ptr;
				D_S(struct FileInfoBlock, fib)

				// Clear data buffer
				data->comment[0] = 0;

				// Is there an entry attached?
				if (entry->entry && !(entry->flags & FUNCENTF_ICON_ACTION) &&
					(ptr = (char *)GetTagData(DE_Comment, 0, entry->entry->de_Tags)))
				{
					// Get existing comment
					strcpy(data->comment, ptr);
				}

				// Otherwise, try to read existing comment from disk
				else if (GetFileInfo(source_file, fib))
				{
					// Get existing comment
					strcpy(data->comment, fib->fib_Comment);
				}

				// Ask for comment
				ret = function_change_get_comment(handle, source_file, data->comment);
			}

			else if (command->function == FUNC_PROTECT)
			{
				ULONG old_prot = 0;

				// Get old protection
				if (entry->entry && !(entry->flags & FUNCENTF_ICON_ACTION))
					old_prot = entry->entry->de_Protection;

				// Ask for protection masks
				ret = function_change_get_protect(handle, source_file, old_prot, data->prot);
			}

			else
			{
				// Get old datestamp
				if (entry->entry && !(entry->flags & FUNCENTF_ICON_ACTION))
					data->date = entry->entry->de_Date;
				else
					DateStamp(&data->date);

				// Ask for date
				ret = function_change_get_date(handle, source_file, &data->date);
			}

			// Check abort
			if (ret == -1)
			{
				function_abort(handle);
				ret = 0;
				break;
			}

			// Do all?
			if (ret == 2)
				handle->inst_flags |= INSTF_NO_ASK;
		}

		else
			ret = 1;

		// Check if we need to open the progress indicator
		if (!progress && handle->inst_flags & INSTF_NO_ASK)
		{
			// Turn progress indicator on
			function_progress_on(handle, GetString(&locale, action), handle->entry_count, PWF_FILENAME | PWF_GRAPH);
			progress = 1;
		}

		// Skip directory "entered" entries
		if (entry->type < 0 || handle->inst_flags & INSTF_NO_RECURSE || entry->flags & FUNCENTF_EXITED)
		{
			// Ok to do this entry?
			while (ret)
			{
				BOOL change = 0;
				char *comment = 0;
				ULONG protection = 0;
				struct DateStamp date = {0};

				// Get existing settings
				if (entry->entry && !(entry->flags & FUNCENTF_ICON_ACTION))
				{
					comment = (char *)GetTagData(DE_Comment, 0, entry->entry->de_Tags);
					protection = entry->entry->de_Protection;
					date = entry->entry->de_Date;
				}

				// Comment?
				if (command->function == FUNC_COMMENT)
				{
					// Got a lister entry?
					if (entry->entry)
					{
						// Use original function
						ret = OriginalSetComment(source_file, data->comment);
					}

					// Otherwise, allow patched function to be used
					else
						ret = SetComment(source_file, data->comment);

					// Successful?
					if (ret)
					{
						ret = 1;

						// Is the new comment different?
						if (entry->entry && !(entry->flags & FUNCENTF_ICON_ACTION) &&
							((!comment && *data->comment) || (comment && strcmp(comment, data->comment))))
						{
							// Need to change entry
							change = 1;
							comment = data->comment;
						}
					}
				}

				// Protect?
				else if (command->function == FUNC_PROTECT)
				{
					ULONG set_bits, clear_bits, new_prot;

					// Get bits to set
					set_bits = (data->prot[0] & 0xf0) | (data->prot[1] & 0xf);
					clear_bits = (data->prot[1] & 0xf0) | (data->prot[0] & 0xf);

					// Get current bits
					if (entry->entry && !(entry->flags & FUNCENTF_ICON_ACTION))
						new_prot = entry->entry->de_Protection;
					else
						new_prot = handle->recurse_info.fib_Protection;

					// Apply changes
					new_prot |= set_bits;
					new_prot &= ~clear_bits;

					// Got a lister entry?
					if (entry->entry)
					{
						// Use original function
						ret = OriginalSetProtection(source_file, new_prot);
					}

					// Otherwise, allow patched function to be used
					else
						ret = SetProtection(source_file, new_prot);

					// Successful?
					if (ret)
					{
						ret = 1;

						// Are the new bits different?
						if (entry->entry && !(entry->flags & FUNCENTF_ICON_ACTION) &&
							entry->entry->de_Protection != new_prot)
						{
							// Need to change entry
							change = 1;
							protection = new_prot;
						}
					}
				}

				// Datestamp
				else
				{
					// Got a lister entry?
					if (entry->entry)
					{
						// Use original function
						ret = OriginalSetFileDate(source_file, &data->date);
					}

					// Otherwise, allow patched function to be used
					else
						ret = SetFileDate(source_file, &data->date);

					// Successful?
					if (ret)
					{
						ret = 1;

						// Is the new date different?
						if (entry->entry && !(entry->flags & FUNCENTF_ICON_ACTION) &&
							CompareDates(&entry->entry->de_Date, &data->date))
						{
							// Need to change entry
							change = 1;
							date = data->date;
						}
					}
				}

				// Need to change entry?
				if (change && path->lister)
				{
					struct FileInfoBlock fib;

					// Fill out dummy fileinfoblock
					strcpy(fib.fib_FileName, entry->name);
					if (comment)
						strcpy(fib.fib_Comment, comment);
					else
						fib.fib_Comment[0] = 0;
					GETFIBSIZE(&fib) = entry->entry->de_Size;
					fib.fib_DirEntryType = entry->entry->de_Node.dn_Type;
					fib.fib_Date = date;
					fib.fib_Protection = protection;

					// Add new file
					if (function_filechange_addfile(handle,
													path->path,
													&fib,
													(NetworkInfo *)GetTagData(DE_NetworkInfo, 0, entry->entry->de_Tags),
													0))
					{
						// Mark old entry for removal
						entry->flags |= FUNCENTF_REMOVE;
					}
				}

				// Successful?
				if (ret)
					break;

				// Display error requester
				if ((ret = function_error(handle, entry->name, error_action, IoErr())) == -1)
					break;

				// Skip?
				if (ret == 0)
					file_ok = 0;
			}
		}

		// Aborted?
		if (ret == -1)
		{
			function_abort(handle);
			ret = 0;
			break;
		}

		// Get next entry, increment count
		count += function_end_entry(handle, entry, file_ok);

		// Reset result code
		ret = 1;
	}

	// Free data
	FreeVec(source_file);

	return ret;
}

// Get comment
static int function_change_get_comment(FunctionHandle *handle, char *file, char *buffer)
{
	short ret;
	char name[35];

	// Ask user for a comment
	get_trunc_filename(FilePart(file), name);
	lsprintf(handle->work_buffer, GetString(&locale, MSG_ENTER_COMMENT), name);
	ret = function_request(handle,
						   handle->work_buffer,
						   SRF_BUFFER,
						   buffer,
						   79,
						   GetString(&locale, MSG_OKAY),
						   GetString(&locale, MSG_ALL),
						   GetString(&locale, MSG_SKIP),
						   GetString(&locale, MSG_ABORT),
						   0);

	// Return appropriate value
	if (ret == 0)
		return -1;
	if (ret == 3)
		return 0;
	if (ret != 2)
		ret = 1;
	return ret;
}

// Get protection
static int function_change_get_protect(FunctionHandle *handle, char *file, ULONG old_prot, unsigned char *masks)
{
	NewConfigWindow new_win;
	struct Window *window;
	ObjectList *objlist;
	Lister *lister;
	short break_flag = 0, x, y, obj;
	static char *prot_bits = "H\0S\0P\0A\0R\0W\0E\0D";
	ObjectDef *objects;
	unsigned char *set_mask, *clear_mask;
	char name[35];

	// Fix old protection bits
	old_prot = (old_prot & 0xf0) | ((~old_prot) & 0xf);

	// Get pointer to set/clear mask
	set_mask = masks;
	clear_mask = masks + 1;

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
	new_win.dims = &_protect_window;
	new_win.title = GetString(&locale, MSG_SELECT_PROTECTION_BITS);
	new_win.locale = &locale;
	new_win.port = 0;
	new_win.flags |= WINDOW_NO_CLOSE | WINDOW_VISITOR | WINDOW_AUTO_KEYS | WINDOW_REQ_FILL;
	new_win.font = 0;

	// Allocate objects
	if (!(objects = AllocVec(sizeof(ObjectDef) * 8 * 3 + 1, MEMF_CLEAR)))
		return 0;

	// Build objects
	for (y = 0, obj = 0; y < 3; y++)
	{
		for (x = 0; x < 8; x++, obj++)
		{
			// Copy template
			CopyMem((char *)&_protect_button_template, (char *)(objects + obj), sizeof(ObjectDef));

			// Fix coordinates
			objects[obj].char_dims.Left = 12 + x * 2;
			objects[obj].char_dims.Top = y + 1;
			objects[obj].fine_dims.Left = 5 + x * 4;
			objects[obj].fine_dims.Top = 14 + y * 8;

			// Type and flags for old display
			if (y == 0)
			{
				objects[obj].type = OD_AREA;
				objects[obj].object_kind = TEXTPEN;
				objects[obj].flags = AREAFLAG_RECESSED | TEXTFLAG_CENTER;

				// Only has text if bit is set
				if (old_prot & (1 << (7 - x)))
					objects[obj].gadget_text = (IPTR)&prot_bits[x * 2];
			}

			// Gadget text
			else
				objects[obj].gadget_text = (IPTR)&prot_bits[x * 2];

			// Flags
			objects[obj].flags |= TEXTFLAG_TEXT_STRING;

			// Gadget id
			objects[obj].gadgetid = GAD_PROTECT_OLD + (y * 10) + x;
		}
	}

	// Last object
	objects[obj].type = OD_END;

	// Open window
	if (!(window = OpenConfigWindow(&new_win)) || !(objlist = AddObjectList(window, _protect_objects)) ||
		!(AddObjectList(window, objects)))
	{
		CloseConfigWindow(window);
		FreeVec(objects);
		return 0;
	}

	// Free object memory
	FreeVec(objects);

	// Show filename
	get_trunc_filename(FilePart(file), name);
	DisplayObject(window, GetObject(objlist, GAD_PROTECT_FILENAME), -1, -1, name);

	// Set initial gadget states
	for (obj = 0; obj < 8; obj++)
	{
		if ((*set_mask) & (1 << obj))
			SetGadgetValue(objlist, GAD_PROTECT_SET + 7 - obj, 1);
		else if ((*clear_mask) & (1 << obj))
			SetGadgetValue(objlist, GAD_PROTECT_CLEAR + 7 - obj, 1);
	}

	// Event loop
	FOREVER
	{
		struct IntuiMessage *msg;

		// Check for abort
		if (function_check_abort(handle))
		{
			break_flag = GAD_PROTECT_ABORT;
			break;
		}

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
			UWORD gadgetid = 0;

			// Copy message and reply
			copy_msg = *msg;
			ReplyWindowMsg(msg);
			if (copy_msg.Class == IDCMP_GADGETUP)
				gadgetid = ((struct Gadget *)copy_msg.IAddress)->GadgetID;

			// Look at message
			switch (copy_msg.Class)
			{
			// Key press
			case IDCMP_VANILLAKEY:

				// See if this matches a protection bit
				for (obj = 0; obj < 8; obj++)
				{
					if (toupper(copy_msg.Code) == prot_bits[obj * 2])
					{
						short state;

						// Shift is clear
						if (isupper(copy_msg.Code))
							gadgetid = GAD_PROTECT_CLEAR + obj;
						else
							gadgetid = GAD_PROTECT_SET + obj;

						// Toggle state of gadget
						state = 1 - GetGadgetValue(objlist, gadgetid);
						SetGadgetValue(objlist, gadgetid, state);
						break;
					}
				}

				// Not recognised
				if (obj == 8)
					break;

			// Gadget
			case IDCMP_GADGETUP:

				// Clear bit?
				if (gadgetid >= GAD_PROTECT_CLEAR)
				{
					// Make sure corresponding set bit is off
					SetGadgetValue(objlist, GAD_PROTECT_SET + gadgetid - GAD_PROTECT_CLEAR, 0);
					break;
				}

				// Set bit?
				else if (gadgetid >= GAD_PROTECT_SET)
				{
					// Make sure corresponding clear bit is off
					SetGadgetValue(objlist, GAD_PROTECT_CLEAR + gadgetid - GAD_PROTECT_SET, 0);
					break;
				}

				// Another button
				switch (gadgetid)
				{
				case GAD_PROTECT_OK:
				case GAD_PROTECT_ALL:
				case GAD_PROTECT_SKIP:
				case GAD_PROTECT_ABORT:
					break_flag = gadgetid;
					break;
				}
				break;
			}
		}

		if (break_flag)
			break;

		Wait(1 << window->UserPort->mp_SigBit | 1 << handle->ipc->command_port->mp_SigBit);
	}

	// Get gadget states
	*set_mask = 0;
	*clear_mask = 0;
	for (obj = 0; obj < 8; obj++)
	{
		if (GetGadgetValue(objlist, GAD_PROTECT_SET + 7 - obj))
			*set_mask |= 1 << obj;
		else if (GetGadgetValue(objlist, GAD_PROTECT_CLEAR + 7 - obj))
			*clear_mask |= 1 << obj;
	}

	// Close window
	CloseConfigWindow(window);

	// Return correct code
	if (break_flag == GAD_PROTECT_ABORT)
		return -1;
	if (break_flag == GAD_PROTECT_SKIP)
		return 0;
	if (break_flag == GAD_PROTECT_ALL)
		return 2;
	return 1;
}

// Check for an empty argument, including ARG="" if quotes survive parsing
static BOOL function_change_arg_empty(char *arg)
{
	char *end;

	if (!arg)
		return 0;

	while (*arg && isspace(*arg))
		++arg;

	end = arg + strlen(arg);
	while (end > arg && isspace(*(end - 1)))
		--end;

	if (end == arg)
		return 1;

	if ((*arg == '"' || *arg == '\'') && end > arg && *(end - 1) == *arg)
	{
		++arg;
		--end;

		while (arg < end && isspace(*arg))
			++arg;
		while (end > arg && isspace(*(end - 1)))
			--end;

		return (end == arg);
	}

	return 0;
}

// Get date
static int function_change_get_date(FunctionHandle *handle, char *file, struct DateStamp *date)
{
	short ret = 0;
	char *ptr, name[35];

	// Get current datestamp string
	date_to_strings(date, handle->work_buffer + 512, handle->work_buffer + 540, 0);

	// Strip excess spaces at the end of the date string
	ptr = handle->work_buffer + 512 + strlen(handle->work_buffer + 512) - 1;
	while (*ptr == ' ' && ptr > handle->work_buffer + 512)
		--ptr;
	if (*(ptr + 1) == ' ')
		*(ptr + 1) = 0;

	// Add time string
	strcat(handle->work_buffer + 512, " ");
	strcat(handle->work_buffer + 512, handle->work_buffer + 540);

	// Ask user for date
	get_trunc_filename(FilePart(file), name);
	lsprintf(handle->work_buffer, GetString(&locale, MSG_ENTER_DATE_AND_TIME), name);

	// Loop until successful
	FOREVER
	{
		// Ask for date
		if ((ret = function_request(handle,
									handle->work_buffer,
									SRF_BUFFER,
									handle->work_buffer + 512,
									20,
									GetString(&locale, MSG_OKAY),
									GetString(&locale, MSG_ALL),
									GetString(&locale, MSG_SKIP),
									GetString(&locale, MSG_ABORT),
									0)) == 0 ||
			ret == 3)
			break;

		// If string is empty, get current datestamp
		if (handle->work_buffer[512] == 0)
		{
			DateStamp(date);
			break;
		}

		else
		{
			// Convert to separate strings
			ParseDateStrings(handle->work_buffer + 512, handle->work_buffer + 768, handle->work_buffer + 896, 0);

			// Convert to datestamp
			if (!(DateFromStringsNew(handle->work_buffer + 768,
									 handle->work_buffer + 896,
									 date,
									 environment->env->settings.date_format)))
			{
				// Display error
				if (!(ret = function_request(handle,
											 GetString(&locale, MSG_ERROR_INVALID_DATE),
											 0,
											 GetString(&locale, MSG_TRY_AGAIN),
											 GetString(&locale, MSG_CANCEL),
											 0)))
					break;
			}
			else
				break;
		}
	}

	// Return appropriate value
	if (ret == 0)
		return -1;
	if (ret == 3)
		return 0;
	return ((ret != 2) ? 1 : 2);
}
