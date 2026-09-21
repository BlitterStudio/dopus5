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

#include <proto/module.h>

int function_internal_command(CommandList *command, char *args, FunctionHandle *handle, InstructionParsed *instruction)
{
	short ret = 1;
	Lister *lister;
	DirBuffer *cust_buffer = 0;
	Lister *source_lister = 0, *dest_lister = 0;
	char custom_port[40];
	PathNode *source_n = 0, *dest_n = 0;

	// Clear instruction flags
	if (handle)
		handle->instruction_flags = 0;

	// Valid command?
	if (!command || !command->name)
		return 1;

	// Initialise custom port
	custom_port[0] = 0;

	// Got a handle?
	if (handle)
	{
		// Get source and dest nodes
		source_n = function_path_current(&handle->source_paths);
		dest_n = function_path_current(&handle->dest_paths);

		// Get current source lister
		if ((lister = function_lister_current(&handle->source_paths)))
		{
			// Custom handler installed?
			if (lister->cur_buffer->buf_CustomHandler[0])
			{
				// If there's a NEW flag, we won't be using this lister
				if (instruction && !instruction->new_arg)
				{
					// Look for trap handler
					if (FindFunctionTrap(command->name, lister->cur_buffer->buf_CustomHandler, custom_port))
					{
						source_lister = lister;
						dest_lister = function_lister_current(&handle->dest_paths);
						cust_buffer = lister->cur_buffer;
					}
				}
			}
		}

		// If no custom handler, try destination
		if (!cust_buffer && (command->flags & FUNCF_NEED_DEST) &&
			(lister = function_lister_current(&handle->dest_paths)))
		{
			// Custom handler installed?
			if (lister->cur_buffer->buf_CustomHandler[0])
			{
				// Look for trap handler
				if (FindFunctionTrap(command->name, lister->cur_buffer->buf_CustomHandler, custom_port))
				{
					source_lister = function_lister_current(&handle->source_paths);
					dest_lister = lister;
					cust_buffer = lister->cur_buffer;
				}
			}
		}
	}

	// Custom handler?
	if (cust_buffer)
	{
		char *files = 0;
		ULONG flags;

		// Need files?
		if (command->flags & FUNCF_NEED_ENTRIES)
		{
			// Build file string
			files = function_build_file_string(handle, cust_buffer->cust_flags & CUSTF_WANT_QUOTES);
		}

		// Get flags
		flags = RXMF_WARN;
		if (cust_buffer->cust_flags & CUSTF_SYNCTRAPS)
			flags |= RXMF_SYNC;

		// Send command message
		ret = rexx_handler_msg(custom_port,
							   cust_buffer,
							   flags,
							   HA_String,
							   0,
							   command->name,
							   HA_Value,
							   1,
							   source_lister,
							   HA_String,
							   2,
							   files,
							   HA_Value,
							   3,
							   dest_lister,
							   HA_String,
							   4,
							   (source_n) ? source_n->path : 0,
							   HA_String,
							   5,
							   args,
							   HA_String,
							   7,
							   (dest_n) ? dest_n->path : 0,
							   HA_Value,
							   8,
							   (flags & RXMF_SYNC) ? handle : 0,
							   TAG_END);

		// Free file string
		FreeVec(files);
		return ret;
	}

	// External command?
	if (command->flags & FUNCF_EXTERNAL_FUNCTION)
	{
		struct Library *ModuleBase;
#ifdef __amigaos4__
		struct ModuleIFace *IModule;
#endif
		char *work_buf = 0, *buffer;

		// Need to allocate buffer?
		if (!handle)
		{
			work_buf = AllocVec(512, MEMF_CLEAR);
			buffer = work_buf;
		}
		else
			buffer = handle->work_buffer;

		// No buffer?
		if (!buffer || strcmp(command->stuff.module_name, "!") == 0)
		{
		}

		// User command?
		else if (strnicmp(command->stuff.module_name, "DOpus5:Commands/", 16) == 0)
		{
			// Shouldn't be able to get here!
		}

		// Rexx script?
		else if (sufcmp(command->stuff.module_name, ".dopus5"))
		{
			// Get rexx function to run
			lsprintf(buffer,
					 "DOpus5:Modules/%s %s %s %ld %ld %s",
					 command->stuff.module_name,
					 GUI->rexx_port_name,
					 command->name,
					 (source_n) ? source_n->lister : 0,
					 (dest_n) ? dest_n->lister : 0,
					 args);

			// Run rexx thing
			rexx_send_command(buffer, TRUE);
		}

		// Open module
		else if ((ModuleBase = OpenModule(command->stuff.module_name))
#ifdef __amigaos4__
				 && (IModule = (struct ModuleIFace *)GetInterface(ModuleBase, "main", 1, NULL))
#endif
		)
		{
			// Copy arguments
			strcpy(buffer, args);

			// Add a newline
			if (*buffer)
				strcat(buffer, "\n");

			// Rescan the destination when the function finishes?  Function
			// scripts get this in function_close_script(); inline module
			// commands change the destination outside DOpus's knowledge
			// (e.g. XADOpus extracting a dropped archive), so flag the
			// destination here, before the module's dc_EndDest advances the
			// current path (issue #174)
			if (handle && dest_n && handle->func_parameters.flags & FUNCF_RESCAN_DEST)
				dest_n->flags |= LISTNF_RESCAN;

			// Call module
			ret = Module_Entry((command->flags & FUNCF_NO_ARGS) ? 0 : (struct List *)buffer,
							   GUI->screen_pointer,
							   (handle) ? handle->ipc : 0,
							   &main_ipc,
							   command->function,
							   (IPTR)GET_CALLBACK(function_external_hook));

// Close module
#ifdef __amigaos4__
			DropInterface((struct Interface *)IModule);
#endif
			CloseLibrary(ModuleBase);
		}
		else
			CloseLibrary(ModuleBase);  // In case module opens but interface doesn't

		// Free buffer
		FreeVec(work_buf);
	}

	// Valid code?
	else if (command->stuff.code)
		ret = command->stuff.code(command, handle, args, instruction);

	return ret;
}

// Hook for external commands
REF_CALLBACK_BEGIN(IPTR,
				   ASM SAVEDS function_external_hook,
				   d0,
				   ULONG,
				   command,
				   a0,
				   FunctionHandle *,
				   handle,
				   a1,
				   APTR,
				   packet)
{
	// Look at command
	switch (command)
	{
	// Get current source path
	case EXTCMD_GET_SOURCE:
		return (IPTR)HookGetSource(handle, (char *)packet);

	// Get next source path
	case EXTCMD_NEXT_SOURCE:
		return (IPTR)HookNextSource(handle, (char *)packet);

	// Unlock source paths
	case EXTCMD_UNLOCK_SOURCE:
		HookUnlockSource(handle);
		break;

	// Get destination path
	case EXTCMD_GET_DEST:
		return (IPTR)HookGetDest(handle, (char *)packet);

	// Finished with this source
	case EXTCMD_END_SOURCE:
		HookEndSource(handle, (long)packet);
		break;

	// Finished with this destination
	case EXTCMD_END_DEST:
		HookEndDest(handle, (long)packet);
		break;

	// Get next entry
	case EXTCMD_GET_ENTRY:
		return (IPTR)HookGetEntry(handle);

	// End this entry
	case EXTCMD_END_ENTRY:
		HookEndEntry(handle, ((struct endentry_packet *)packet)->entry, ((struct endentry_packet *)packet)->deselect);
		break;

	// Remove a file
	case EXTCMD_REMOVE_ENTRY:
		HookRemoveEntry((FunctionEntry *)packet);
		break;

	// Get entry count
	case EXTCMD_ENTRY_COUNT:
		return (ULONG)HookEntryCount(handle);

	// Reload a file
	case EXTCMD_RELOAD_ENTRY:
		HookReloadEntry(handle, (FunctionEntry *)packet);
		break;

	// Add a file to a path
	case EXTCMD_ADD_FILE:
		HookAddFile(handle,
					((struct addfile_packet *)packet)->path,
					((struct addfile_packet *)packet)->fib,
					((struct addfile_packet *)packet)->lister);
		break;

	// Delete a file in a path
	case EXTCMD_DEL_FILE:
		HookDelFile(handle,
					((struct delfile_packet *)packet)->path,
					((struct delfile_packet *)packet)->name,
					((struct delfile_packet *)packet)->lister);
		break;

	// Load a file in a path
	case EXTCMD_LOAD_FILE:
		HookLoadFile(handle,
					 ((struct loadfile_packet *)packet)->path,
					 ((struct loadfile_packet *)packet)->name,
					 ((struct loadfile_packet *)packet)->flags,
					 ((struct loadfile_packet *)packet)->reload);
		break;

	// Perform changes
	case EXTCMD_DO_CHANGES:
		HookDoChanges(handle);
		break;

	// Check abort status
	case EXTCMD_CHECK_ABORT:
		return (ULONG)HookCheckAbort(handle);

	// Get a window handle
	case EXTCMD_GET_WINDOW:
		return (IPTR)HookGetWindow((PathNode *)packet);
		break;

	// Get help on a topic
	case EXTCMD_GET_HELP:
		HookShowHelp(0, (char *)packet);
		break;

	// Get rexx portname
	case EXTCMD_GET_PORT:
		return (IPTR)HookGetPort((char *)packet);

	// Get screen name
	case EXTCMD_GET_SCREEN:
		return (IPTR)HookGetScreen((char *)packet);

	// Get screen data
	case EXTCMD_GET_SCREENDATA:
		return (IPTR)HookGetScreenData();

	// Free screen data
	case EXTCMD_FREE_SCREENDATA:
		HookFreeScreenData(packet);
		break;

	// Open progress indicator
	case EXTCMD_OPEN_PROGRESS:
		HookOpenProgress(((struct progress_packet *)packet)->path,
						 ((struct progress_packet *)packet)->name,
						 ((struct progress_packet *)packet)->count);
		break;

	// Update progress indicator
	case EXTCMD_UPDATE_PROGRESS:
		HookUpdateProgress(((struct progress_packet *)packet)->path,
						   ((struct progress_packet *)packet)->name,
						   ((struct progress_packet *)packet)->count);
		break;

	// Close progress indicator
	case EXTCMD_CLOSE_PROGRESS:
		HookCloseProgress(((struct progress_packet *)packet)->path);
		break;

	// Show exists/replace? requester
	case EXTCMD_REPLACE_REQ:
		return (ULONG)HookReplaceReq(((struct replacereq_packet *)packet)->window,
									 ((struct replacereq_packet *)packet)->screen,
									 ((struct replacereq_packet *)packet)->ipc,
									 ((struct replacereq_packet *)packet)->file1,
									 ((struct replacereq_packet *)packet)->file2,
									 ((struct replacereq_packet *)packet)->default_option);
		break;

	// Get a pointer
	case EXTCMD_GET_POINTER:
		return HookGetPointer((struct pointer_packet *)packet);

	// Free a pointer
	case EXTCMD_FREE_POINTER:
		HookFreePointer((struct pointer_packet *)packet);
		break;

	// Send an ARexx command to DOpus
	case EXTCMD_SEND_COMMAND:
		((struct command_packet *)packet)->rc = HookSendCommand(handle,
																((struct command_packet *)packet)->command,
																&((struct command_packet *)packet)->result,
																((struct command_packet *)packet)->flags);
		return 1;
	}

	// Unknown command
	return 0;
}
