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

static int function_run_instruction(FunctionHandle *handle, InstructionParsed *instruction);
static Cfg_Function *function_dual_special_function(FunctionHandle *handle, ULONG *open_flags);

enum { DUAL_DEVLIST_ARG_NEW, DUAL_DEVLIST_ARG_FULL, DUAL_DEVLIST_ARG_BRIEF };

// Run a function
void function_run_function(FunctionHandle *handle)
{
	InstructionParsed *instruction;

	// Got a valid function?
	if (!handle || !handle->function)
		return;

	// Parse function
	if (function_parse_function(handle))
	{
		// Run the function
		handle->ret_code = function_run(handle);

		// Clean up recurse data
		if (handle->recurse_return != -1)
		{
			// End recurse stuff
			MatchEnd(handle->anchor);

			// Fix CD
			if (handle->recurse_cd)
			{
				BPTR lock = CurrentDir(handle->recurse_cd);
				UnLock(lock);
				handle->recurse_cd = 0;
			}
		}

		// Do final changes to lists
		function_do_lister_changes(handle, &handle->source_paths);
		function_do_lister_changes(handle, &handle->dest_paths);

		// Display final message (if appropriate)
		if (handle->final_message[0] && handle->source_paths.list.mlh_Head->mln_Succ)
		{
			status_text(((PathNode *)handle->source_paths.list.mlh_Head)->lister, handle->final_message);
		}
	}

	// Go through instruction list
	for (instruction = (InstructionParsed *)handle->func_instructions.mlh_Head; instruction->node.mln_Succ;
		 instruction = (InstructionParsed *)instruction->node.mln_Succ)
	{
		// Free ReadArgs stuff?
		DisposeArgs(instruction->funcargs);
	}
}

// Run a function
short function_run(FunctionHandle *handle)
{
	InstructionParsed *instruction;
	PathNode *path;
	BOOL done_paths = 0;
	short run_okay;

	// Parameters supplied?
	if (handle->function)
	{
		// Copy function parameters
		handle->func_parameters = handle->function->function;
	}

	// Add additional flags
	handle->func_parameters.flags |= handle->func_additional_flags;

	// Initialise output handle
	strcpy(handle->output_handle, environment->env->output_device);
	StrConcat(handle->output_handle, environment->env->output_window, sizeof(handle->output_handle) - 1);
	StrConcat(handle->output_handle, "/AUTO/SCREEN ", sizeof(handle->output_handle) - 1);
	StrConcat(handle->output_handle,
			  (handle->func_parameters.flags & FUNCF_WORKBENCH_OUTPUT) ? "Workbench" : get_our_pubscreen(),
			  sizeof(handle->output_handle) - 1);

	// Reset source count
	handle->func_source_num = 0;

	// Check source and destination
	if (!(function_check_paths(handle)))
		return 0;

	// Get first path
	path = function_path_next(&handle->source_paths);

	// Count the instructions
	handle->instruction_count = Att_NodeCount((Att_List *)&handle->func_instructions);

	// Go through source lists
	do
	{
		BOOL first = 1;

		// Reset ok flag
		run_okay = 1;

		// Get source path
		if (path)
		{
			strcpy(handle->source_path, path->path);
			AddPart(handle->source_path, "", 512);
		}
		else
			handle->source_path[0] = 0;

		// Reset iteration count
		handle->func_iterations = 0;

		// If we're reloading files, update stamp in source
		if (handle->func_parameters.flags & FUNCF_RELOAD_FILES && path)
			path->flags |= LISTNF_UPDATE_STAMP;

		// Loop until finished
		FOREVER
		{
			// Go through instruction list
			for (instruction = (InstructionParsed *)handle->func_instructions.mlh_Head; instruction->node.mln_Succ;
				 instruction = (InstructionParsed *)instruction->node.mln_Succ)
			{
				// Once only?
				if (instruction->string && *instruction->string == '#')
				{
					// Check count; skip if already used
					if (instruction->count > 0)
						continue;

					// Increment count
					++instruction->count;
				}

				// Set instruction data pointer
				handle->inst_data = instruction->inst_data;
				handle->inst_flags = instruction->inst_flags;

				// Are we out of files?
				if (!function_current_entry(handle))
				{
					// A {fu}/{ou} NAME argument on an internal command means "use the selection":
					// drop it so the entry list below is built from the live selection
					function_drop_brace_name_arg(instruction);

					// Build new entry list
					if ((function_build_list(handle, &path, instruction)) == -1)
					{
						run_okay = 0;
						break;
					}

					// Check entries
					if (instruction->flags & FUNCF_NEED_ENTRIES &&
						(!first || !(instruction->flags & FUNCF_WANT_ENTRIES)) && !(function_current_entry(handle)))
					{
						run_okay = 0;
						break;
					}
				}

				// Check instruction path
				function_check_ins_path(handle, instruction, &handle->dest_paths, 'd', 0);

				// Haven't done path check yet?
				if (!done_paths)
				{
					// Check valid destination if needed
					if (handle->func_flags & FUNCF_NEED_DEST && handle->func_flags & FUNCF_ASK_PATH &&
						(IsListEmpty((struct List *)&handle->dest_paths)))
					{
						// If we fail on source, don't ask for dest                  *****
						if ((handle->func_flags & FUNCF_NEED_SOURCE && !(handle->func_flags & FUNCF_GOT_SOURCE)) ||
							!(function_check_single(handle, LISTERF_DEST, 0, 0)))
						{
							run_okay = 0;
							break;
						}

						// Got destination
						handle->func_flags |= FUNCF_GOT_DEST;
					}

					// Set flag
					done_paths = 1;
				}

				// Run this instruction
				run_okay = function_run_instruction(handle, instruction);

				// Pending quit or abort?
				if (GUI->flags & GUIF_PENDING_QUIT || function_check_abort(handle))
					run_okay = 0;

				// Failed?
				if (run_okay != 1)
					break;

				// Store flags in case they've changed
				instruction->inst_flags = handle->inst_flags;

				// Do cleanup for this instruction
				function_cleanup(handle, path, 0);
			}

			// Aborted?
			if (run_okay != 1 || function_check_abort(handle))
				break;

			// Do all files not set, or no more files?
			if (!(handle->func_parameters.flags & FUNCF_DO_ALL_FILES) || !function_current_entry(handle))
				break;

			// Start again at first argument
			handle->func_current_arg = handle->func_arguments.lh_Head;

			// Increment iteration count
			++handle->func_iterations;
			first = 0;
		}

		// Close the script file (will run function if successful)
		function_close_script(handle, run_okay);

		// Done with this path
		function_path_end(handle, &handle->source_paths, 1);

		// Check abort
		if (!run_okay || function_check_abort(handle))
			break;

		// Increment source count
		++handle->func_source_num;

		// Get next path
		path = function_path_next(&handle->source_paths);
	} while (path);

	return run_okay;
}

// Run an instruction in a function
static int function_run_instruction(FunctionHandle *handle, InstructionParsed *instruction)
{
	short ret = 1;

	// Save instruction
	handle->cur_instruction = instruction;

	// Destination selections are snapshotted per instruction.
	handle->dest_entry_path = 0;

	// Is the instruction an internal function?
	if (instruction->type == INST_COMMAND)
	{
		// Invalid command?
		if (!instruction->command)
			return 1;

		// Is this the end of a section?
		if (instruction->command->function == FUNC_ENDFUNCTION)
		{
			function_close_script(handle, 1);
		}

		// Otherwise, normal internal command
		else
		{
			char *args;
			short limit, parse_ret;

			// Get cll limit
			if ((limit = environment->env->settings.command_line_length) < 256)
				limit = 256;

			// Allocate space for arguments
			if ((args = AllocMemH(handle->memory, limit + 1)))
			{
				ret = 0;

				// Build function string
				if (instruction->string && instruction->string[0])
					parse_ret = function_build_instruction(handle, instruction, 0, args);
				else
				{
					args[0] = 0;
					parse_ret = PARSE_OK;
				}

				if (parse_ret >= PARSE_OK)
				{
					handle->dest_entry_path = 0;

					// Check arguments
					function_parse_arguments(handle, instruction);

					// For an internal command with a {fu}/{ou} NAME argument, the entry list holds
					// the whole selection (see function_drop_brace_name_arg). Expanding the {fu}/{ou}
					// code while building the arguments above consumed one or more of those entries;
					// rewind so the command processes the entire selection, and mark every entry
					// no-unselect so it stays selected afterwards.
					if (function_brace_name_arg(instruction) >= 0)
					{
						FunctionEntry *bent;

						handle->current_entry = (FunctionEntry *)handle->entry_list.lh_Head;
						for (bent = (FunctionEntry *)handle->entry_list.lh_Head; bent->node.mln_Succ;
							 bent = (FunctionEntry *)bent->node.mln_Succ)
							bent->flags |= FUNCENTF_NO_UNSELECT;
					}

					// If we needs files, check there are some
					if (!(instruction->flags & FUNCF_NEED_ENTRIES) || instruction->flags & FUNCF_WANT_ENTRIES ||
						(function_current_entry(handle)))
					{
						// Don't need a source path, or have one already?
						if (!(handle->func_flags & FUNCF_NEED_SOURCE) || (handle->func_flags & FUNCF_GOT_SOURCE))
						{
							// Run the command
							ret = function_internal_command(instruction->command, args, handle, instruction);
						}
					}
				}

				// Free args
				FreeMemH(args);
			}
		}
	}

	// Otherwise, it's an external function
	else
	{
		return function_external_command(handle, instruction);
	}

	return ret;
}

static Cfg_Function *function_dual_special_function(FunctionHandle *handle, ULONG *open_flags)
{
	InstructionParsed *instruction;

	if (!handle)
		return 0;

	for (instruction = (InstructionParsed *)handle->func_instructions.mlh_Head; instruction->node.mln_Succ;
		 instruction = (InstructionParsed *)instruction->node.mln_Succ)
	{
		if (instruction->type != INST_COMMAND || !instruction->command)
			continue;

		if (instruction->command->function == FUNC_DEVICELIST)
		{
			if (instruction->funcargs)
			{
				if (instruction->funcargs->FA_Arguments[DUAL_DEVLIST_ARG_FULL])
					return def_function_devicelist_full;
				if (instruction->funcargs->FA_Arguments[DUAL_DEVLIST_ARG_BRIEF])
					return def_function_devicelist_brief;
			}
			if (open_flags)
				*open_flags |= LISTERF_DEVICES;
			return def_function_devicelist;
		}

		if (instruction->command->function == FUNC_BUFFERLIST)
			return def_function_cachelist;

		break;
	}

	return 0;
}

// Check a function has the right paths
BOOL function_check_paths(FunctionHandle *handle)
{
	// Do we need a source?
	if (handle->func_flags & (FUNCF_NEED_SOURCE | FUNCF_WANT_SOURCE | FUNCF_NEED_ENTRIES | FUNCF_WANT_ENTRIES))
	{
		// Don't already have source?
		if (!(handle->func_flags & FUNCF_GOT_SOURCE))
		{
			// Build list of sources
			if (function_get_paths(
					handle, &handle->source_paths, LISTERF_SOURCE, handle->func_flags & FUNCF_SINGLE_SOURCE))
			{
				handle->func_flags |= FUNCF_GOT_SOURCE;
			}

			// No sources; can we make our own?
			else if (handle->func_flags & FUNCF_CREATE_SOURCE)
			{
				Lister *lister;
				ULONG open_flags = 0;
				ULONG special_open_flags = 0;

				// Create a new lister
				if ((lister = lister_new(0)))
				{
					function_dual_special_function(handle, &special_open_flags);
					if (environment->env->lister_options & LISTEROPTF_DUAL_DEFAULT)
					{
						open_flags |= special_open_flags;
						open_flags |= LISTERF_DUAL;
					}

					// We want lister to be available for functions
					lister->flags2 &= ~LISTERF2_UNAVAILABLE;

					// Open lister
					IPC_Command(lister->ipc, LISTER_OPEN, open_flags, GUI->screen_pointer, 0, 0);

					// Add this lister
					function_add_path(handle, &handle->source_paths, lister, 0);

					// Set flag to say we made our own
					handle->result_flags |= FRESULTF_MADE_LISTER;
				}
			}
		}

		// If we got source, lock it
		if (handle->func_flags & FUNCF_GOT_SOURCE && !(function_lock_paths(handle, &handle->source_paths, 1)))
			handle->func_flags &= ~FUNCF_GOT_SOURCE;
	}

	// Do we need a destination?
	if (handle->func_flags & (FUNCF_NEED_DEST | FUNCF_WANT_DEST))
	{
		// Don't already have destination?
		if (!(handle->func_flags & FUNCF_GOT_DEST))
		{
			// Build list of destinations and lock it
			if (!(handle->flags & FUNCF_ASK_DEST) &&
				function_get_paths(handle, &handle->dest_paths, LISTERF_DEST, handle->func_flags & FUNCF_SINGLE_DEST))
			{
				handle->func_flags |= FUNCF_GOT_DEST;
			}
		}

		// If we got destination, lock it
		if (handle->func_flags & FUNCF_GOT_DEST && !(function_lock_paths(handle, &handle->dest_paths, 0)))
			handle->func_flags &= ~FUNCF_GOT_DEST;

		// Do single destination check
		else if (handle->func_flags & FUNCF_GOT_DEST && handle->func_flags & FUNCF_SINGLE_DEST)
		{
			if (!(function_check_single(handle, LISTERF_DEST, LISTERF_DEST, 0)))
			{
				handle->func_flags &= ~FUNCF_GOT_DEST;
				return 0;
			}
		}
	}

	return 1;
}

// Do checking for single path
short function_check_single(FunctionHandle *handle, long type, long usetype, short locker)
{
	IPCData *ipc;
	Lister *lister;
	Att_List *list;
	Att_Node *node = 0;
	short sel = -1, ret = 1;
	PathList *path_list;
	PathNode *current = 0;

	// Get pathlist pointer
	if (type == LISTERF_SOURCE)
		path_list = &handle->source_paths;
	else
		path_list = &handle->dest_paths;

	// Got a current lister?
	if (usetype && (!(current = function_path_current(path_list)) || !current->lister))
		return 1;

	// Create list
	if (!(list = Att_NewList(0)))
		return 1;

	// Want to build list, or go straight to filereq?
	if (type == LISTERF_SOURCE || !(handle->flags & FUNCF_ASK_DEST))
	{
		// Lock lister list
		lock_listlock(&GUI->lister_list, FALSE);

		// Go through lister list
		for (ipc = (IPCData *)GUI->lister_list.list.lh_Head; ipc->node.mln_Succ; ipc = (IPCData *)ipc->node.mln_Succ)
		{
			BOOL ok = 0;

			// Get lister
			lister = IPCDATA(ipc);

			// Dual lister sides are resolved locally by their launch path.
			if (lister_dual_is_side(lister))
				continue;

			// Is this lister the right type and not busy, or an icon view?
			if ((!usetype || lister->flags & usetype) && lister->cur_buffer->buf_Path[0] &&
				!(lister->flags & LISTERF_BUSY) &&
				(!(lister->flags & LISTERF_VIEW_ICONS) || lister->flags & LISTERF_ICON_ACTION))
				ok = 1;

			/* ACTION_CHANGE */
			// Or, is it the default icon view?
			else if (lister == GUI->current_lister && lister->flags & LISTERF_VIEW_ICONS &&
					 !(lister->flags & LISTERF_ICON_ACTION))
				ok = 1;

			// Ok to use?
			if (ok)
			{
				// Add to list
				Att_NewNode(list, lister->cur_buffer->buf_Path, (IPTR)lister, ADDNODE_SORT | ADDNODE_EXCLUSIVE);
			}
		}

		// Unlock lister list
		unlock_listlock(&GUI->lister_list);
	}

	// Only put up list if too many listers, or none
	if (!usetype || !(IsListEmpty((struct List *)list)))
	{
		// Any other possible paths?
		if (!(IsListEmpty((struct List *)list)) && current)
		{
			// Add the path we do have to lister
			if ((node = Att_NewNode(list, current->lister->cur_buffer->buf_Path, (IPTR)current, ADDNODE_SORT)))
				sel = Att_FindNodeNumber(list, node);
		}

		// Clear buffer
		handle->work_buffer[0] = 0;

		// Are there no paths in the list?
		if (IsListEmpty((struct List *)list))
		{
			// Set requester coordinates
			*((struct IBox *)&handle->filereq->fr_LeftEdge) = handle->coords;

			// Put up requester
			if (AslRequestTags(
					handle->filereq,
					ASLFR_Screen,
					GUI->screen_pointer,
					ASLFR_TitleText,
					GetString(&locale, (type == LISTERF_SOURCE) ? MSG_SELECT_SOURCE : MSG_SELECT_DESTINATION),
					ASLFR_Flags1,
					FRF_PRIVATEIDCMP,
					ASLFR_Flags2,
					FRF_DRAWERSONLY,
					TAG_END))
			{
				// Build path name
				strcpy(handle->work_buffer, handle->filereq->fr_Drawer);
				AddPart(handle->work_buffer, handle->filereq->fr_File, 256);
				sel = -1;
			}

			// Cancelled
			else
				sel = -2;

			// Save coordinates
			handle->coords = *((struct IBox *)&handle->filereq->fr_LeftEdge);
		}

		// Otherwise, show selection list
		else
		{
			// Show selection list
			sel =
				SelectionList(list,
							  0,
							  GUI->screen_pointer,
							  GetString(&locale, (type == LISTERF_SOURCE) ? MSG_SELECT_SOURCE : MSG_SELECT_DESTINATION),
							  sel,
							  SLF_DIR_FIELD,
							  handle->work_buffer,
							  GetString(&locale, MSG_OKAY),
							  GetString(&locale, MSG_ABORT),
							  0,
							  0);
		}

		// Cancelled?
		if (sel == -2 || (sel == -1 && !handle->work_buffer[0]))
		{
			// Cancelled
			ret = 0;

			// Already got a path?
			if (current)
			{
				// Unlock current path
				function_unlock_paths(handle, path_list, locker);

				// Remove current path
				Remove((struct Node *)current);
				FreeMemH(current);
				path_list->current = 0;
			}
		}

		// New path?
		else if (handle->work_buffer[0] ||
				 ((node = Att_FindNode(list, sel)) && (!current || node->data != (IPTR)current)))
		{
			// Already got a path?
			if (current)
			{
				// Unlock current path
				function_unlock_paths(handle, path_list, locker);

				// Free current path
				FreeMemH(current);
			}

			// Get new current path
			if ((current = AllocMemH(handle->memory, sizeof(PathNode))))
			{
				function_init_path_node(current);

				// Typed-in path name?
				if (handle->work_buffer[0])
				{
					strcpy(current->path_buf, handle->work_buffer);
					current->path = current->path_buf;
				}

				// Lister
				else
				{
					/********* THIS SHOULD BE PROTECTED ************/
					// Got a node?
					if (node)
					{
						// Lock lister list
						lock_listlock(&GUI->lister_list, FALSE);

						// Go through lister list
						for (ipc = (IPCData *)GUI->lister_list.list.lh_Head; ipc->node.mln_Succ;
							 ipc = (IPCData *)ipc->node.mln_Succ)
						{
							// Is this the lister we asked for?
							if (IPCDATA(ipc) == (APTR)node->data)
							{
								// Grab it
								current->lister = (Lister *)node->data;
								function_capture_path_side(current);
								break;
							}
						}

						// Unlock lister list
						unlock_listlock(&GUI->lister_list);
					}

					// Haven't got a lister?
					if (!current->lister)
					{
						// Get path
						if (node)
							strcpy(current->path_buf, node->node.ln_Name);
						else
							current->path_buf[0] = 0;
						current->path = current->path_buf;
					}
				}

				// Add to list
				NewList((struct List *)path_list);
				AddHead((struct List *)path_list, (struct Node *)current);

				// Fix current pointer
				path_list->current = current;

				// Lock this path (if a lister)
				if (current->lister)
				{
					ret = function_lock_paths(handle, path_list, locker);
				}
			}

			// Failed
			else
			{
				path_list->current = 0;
				ret = 0;
			}
		}
	}

	// Free list
	Att_RemList(list, 0);

	return ret;
}

// Check for a path supplied in instruction
void function_check_ins_path(FunctionHandle *handle,
							 InstructionParsed *instruction,
							 PathList *path_list,
							 char path_type,
							 short locker)
{
	char *ptr;

	// Path supplied in instruction?
	if (instruction && instruction->command && instruction->command->template_key &&
		(ptr = strchr(instruction->command->template_key, path_type)))
	{
		// Parsed arguments?
		if (instruction->funcargs)
		{
			// Get number of path argument
			short num = atoi(ptr + 1);

			// Clear work buffer
			*handle->work_buffer = 0;

			// Copy to buffer
			if (instruction->funcargs->FA_Arguments[num])
				strcpy(handle->work_buffer, (char *)instruction->funcargs->FA_Arguments[num]);

			// Valid path?
			if (handle->work_buffer[0])
			{
				// Replace existing paths with new one
				function_replace_paths(handle, path_list, handle->work_buffer, locker);
				strcpy((path_type == 'd') ? handle->dest_path : handle->source_path, handle->work_buffer);
			}
		}
	}
}

// Replace paths
void function_replace_paths(FunctionHandle *handle, PathList *path_list, char *new_path, short locker)
{
	PathNode *current;

	// Get current path
	if ((current = function_path_current(path_list)))
	{
		// Unlock current path
		function_unlock_paths(handle, path_list, locker);
	}

	// Initialise path list
	NewList((struct List *)path_list);
	path_list->current = 0;

	// Get new current path
	if ((current = AllocMemH(handle->memory, sizeof(PathNode))))
	{
		function_init_path_node(current);

		// Copy path
		strcpy(current->path_buf, new_path);
		current->path = current->path_buf;

		// Add to list
		AddHead((struct List *)path_list, (struct Node *)current);

		// Fix current pointer
		path_list->current = current;
	}
}
