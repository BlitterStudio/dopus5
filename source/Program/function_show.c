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

// SHOW/PLAY/READ/PRINT/ICONINFO internal function
DOPUS_FUNC(function_show)
{
	FunctionEntry *entry;
	Att_List *list;
	struct Library *ModuleBase;
#ifdef __amigaos4__
	struct ModuleIFace *IModule;
#endif
	short ret = 0, count = 0, funcid;
	BOOL sync_flag = 0;

	// Got arguments? See if 'wait' flag is set
	if (instruction->funcargs && instruction->funcargs->FA_Arguments[1])
		sync_flag = 1;

	// Or, set in function
	else if (handle->flags & FUNCF_SYNC)
		sync_flag = 1;

	// Called by default filetype?
	if ((funcid = command->function) == FUNC_DEFFTYPE)
	{
		// Flag set for create filetype?
		if (environment->env->settings.general_flags & GENERALF_FILETYPE_SNIFFER)
		{
			Cfg_Filetype *type;

			// Get first entry
			entry = function_get_entry(handle);

			// Build full name
			function_build_source(handle, entry, handle->work_buffer);

			// See if file really can't be identified
			if (!(type = filetype_identify(handle->work_buffer, FTTYPE_ANY, 0, 0)) || is_default_filetype(type))
			{
				// Build requester text
				lsprintf(handle->work_buffer, GetString(&locale, MSG_SNIFF_CONFIRMATION), entry->name);

				// Ask for confirmation
				if ((ret = function_request(handle,
											handle->work_buffer,
											0,
											GetString(&locale, MSG_SNIFF),
											GetString(&locale, MSG_SEARCH_READ),
											GetString(&locale, MSG_CANCEL),
											0)) == 1)
				{
					// Open filetype module
					if ((ModuleBase = OpenModule("filetype.module"))
#ifdef __amigaos4__
						&& (IModule = (struct ModuleIFace *)GetInterface(ModuleBase, "main", 1, NULL))
#endif
					)
					{
						// Call module
						ret = Module_Entry(0,
										   GUI->screen_pointer,
										   handle->ipc,
										   &main_ipc,
										   0,  // FindFileType
										   (IPTR)GET_CALLBACK(function_external_hook));

// Close module
#ifdef __amigaos4__
						DropInterface((struct Interface *)IModule);
#endif
						CloseLibrary(ModuleBase);
						return ret;
					}

					// Fail
					else
					{
						CloseLibrary(ModuleBase);  // In case module opens but interface doesn't
						DisplayBeep(0);
					}
				}

				// Cancel?
				else if (!ret)
					return 0;

				// Restore flag
				ret = 0;
			}
		}

		// Change to smartread
		funcid = FUNC_SMARTREAD;
	}

	// Create source list
	if (!(list = Att_NewList(LISTF_POOL)))
		return 0;

	// Build list of sources
	while ((entry = function_get_entry(handle)))
	{
		// Build full name
		function_build_source(handle, entry, handle->work_buffer);

		// Add entry to list
		Att_NewNode(list, handle->work_buffer, (IPTR)entry, 0);

		// Get next entry
		function_end_entry(handle, entry, 1);
		++count;
	}

	// Read?
	if (funcid == FUNC_READ || funcid == FUNC_HEXREAD || funcid == FUNC_ANSIREAD || funcid == FUNC_SMARTREAD)
	{
		struct read_startup *startup;

		// Allocate startup
		if ((startup = AllocVec(sizeof(struct read_startup), MEMF_CLEAR)))
		{
			// Set files pointer
			startup->files = (struct List *)list;
			startup->wheel_scroll_lines = environment->env->env_wheel_scroll_lines;

			// Synchronous?
			if (sync_flag)
			{
				struct Library *ModuleBase;
#ifdef __amigaos4__
				struct ModuleIFace *IModule;
#endif

				// Get read module
				if ((ModuleBase = OpenModule("read.module"))
#ifdef __amigaos4__
					&& (IModule = (struct ModuleIFace *)GetInterface(ModuleBase, "main", 1, NULL))
#endif
				)
				{
					// Read files
					Module_Entry(startup->files,
								 GUI->screen_pointer,
								 handle->ipc,
								 &main_ipc,
								 (IPTR)startup,
								 funcid - FUNC_READ);

// Close module
#ifdef __amigaos4__
					DropInterface((struct Interface *)IModule);
#endif
					CloseLibrary(ModuleBase);
					return 1;
				}
				else
					CloseLibrary(ModuleBase);  // In case module opens but interface doesn't
			}

			// Otherwise, start async
			else if (misc_startup("dopus_read", funcid, 0, startup, FALSE))
				return 1;

			// Failed; free startup
			FreeVec(startup);
		}
	}

	// Print
	else if (funcid == FUNC_PRINT)
	{
		if (misc_startup("dopus_print", MENU_PRINT, 0, list, FALSE))
			return 1;
	}

	// Play?
	else if (funcid == FUNC_PLAY)
	{
		long func = FUNC_PLAY, volume = 64;
		Lister *lister;
		struct Window *window = 0;

		// Got arguments?
		if (instruction->funcargs)
		{
			// Quiet?
			if (instruction->funcargs->FA_Arguments[2])
				func = FUNC_PLAY_QUIET;

			// Iconified?
			else if (instruction->funcargs->FA_Arguments[3])
				func = FUNC_PLAY_ICON;

			// Volume?
			if (instruction->funcargs->FA_Arguments[4])
				volume = *((ULONG *)instruction->funcargs->FA_Arguments[4]);
		}

		// Current lister?
		if ((lister = function_lister_current(&handle->source_paths)))
			window = lister->window;

		// Synchronous?
		if (sync_flag)
		{
			// Open module
			if ((ModuleBase = OpenModule("play.module"))
#ifdef __amigaos4__
				&& (IModule = (struct ModuleIFace *)GetInterface(ModuleBase, "main", 1, NULL))
#endif
			)
			{
				// Play files
				if (Module_Entry((struct List *)list,
								 GUI->screen_pointer,
								 handle->ipc,
								 &main_ipc,
								 (IPTR)window,
								 (volume << 8) | ((func == FUNC_PLAY_QUIET) ? (1 << 0) : 0)) == 1)
					ret = 1;

// Close module
#ifdef __amigaos4__
				DropInterface((struct Interface *)IModule);
#endif
				CloseLibrary(ModuleBase);
			}
			else
				CloseLibrary(ModuleBase);  // In case module opens & interface doesn't
		}

		// Detach
		else
		{
			// Store volume in list
			list->list.lh_Type = volume;

			// Start background player
			misc_startup("dopus_detached_player", func, window, list, 0);
			list = 0;
			ret = 1;
		}
	}

	// IconInfo
	else if (funcid == FUNC_ICONINFO)
	{
		Lister *lister;
		struct Window *window = 0;

		// Current lister?
		if ((lister = function_lister_current(&handle->source_paths)))
			window = lister->window;

		// Synchronous?
		if (sync_flag)
		{
			// Open module
			if ((ModuleBase = OpenModule("icon.module"))
#ifdef __amigaos4__
				&& (IModule = (struct ModuleIFace *)GetInterface(ModuleBase, "main", 1, NULL))
#endif
			)
			{
				// Show IconInfo
				{
					ULONG flags = remap_icon_images() ? 0 : 1;
					if (environment->env->display_options & DISPOPTF_USE_WBINFO)
						flags |= ICON_USE_WBINFO_FLAG;
					if (Module_Entry((struct List *)list, GUI->screen_pointer, handle->ipc, &main_ipc, 0, flags))
						ret = 1;
				}

// Close module
#ifdef __amigaos4__
				DropInterface((struct Interface *)IModule);
#endif
				CloseLibrary(ModuleBase);

				// Do update
				function_iconinfo_update(handle, list);
			}
			else
				CloseLibrary(ModuleBase);  // In case module opens & interface doesn't
		}

		// Detach
		else
		{
			misc_startup("dopus_detached_iconinfo", FUNC_ICONINFO, window, list, 0);
			list = 0;
			ret = 1;
		}
	}

	// Show
	else
	{
		// Can we detach?
		if (count == 1 && handle->instruction_count == 1 && !sync_flag)
		{
			misc_startup("dopus_detached_function", FUNC_SHOW, 0, list, 0);
			list = 0;
		}

		// Open module
		else if ((ModuleBase = OpenModule("show.module"))
#ifdef __amigaos4__
				 && (IModule = (struct ModuleIFace *)GetInterface(ModuleBase, "main", 1, NULL))
#endif
		)
		{
			Att_Node *node;
			ULONG show_flags =
				(environment->env->display_options & DISPOPTF_SHOW_DATATYPES_FIRST) ? SHOW_DATATYPES_FIRST_FLAG : 0;

			// Show files
			ret = Module_Entry((struct List *)list, GUI->screen_pointer, handle->ipc, &main_ipc, 0, show_flags);

// Close show module
#ifdef __amigaos4__
			DropInterface((struct Interface *)IModule);
#endif
			CloseLibrary(ModuleBase);

			// Clear external list
			NewList((struct List *)&handle->external_list);

			// Go through nodes
			for (node = (Att_Node *)list->list.lh_Head; node->node.ln_Succ; node = (Att_Node *)node->node.ln_Succ)
			{
				// Leave selected?
				if (node->node.lve_Flags & (SHOWF_SELECTED | SHOWF_DELETE))
				{
					// Clear 'unselect' flag
					((FunctionEntry *)node->data)->flags &= ~FUNCENTF_UNSELECT;

					// Delete?
					if (node->node.lve_Flags & SHOWF_DELETE)
					{
						ExternalEntry *entry;

						// Create entry
						if ((entry = new_external_entry(handle, node->node.ln_Name)))
						{
							// Add to external entry list
							AddTail((struct List *)&handle->external_list, (struct Node *)entry);
						}
					}
				}
			}

			// Anything in the external list?
			if (!(IsListEmpty((struct List *)&handle->external_list)))
			{
				// Run function
				function_user_run(handle, def_function_delete);
			}
		}
		else
			CloseLibrary(ModuleBase);  // In case module opens & interface doesn't
	}

	// Free file list
	if (list)
		Att_RemList(list, 0);
	return ret;
}

void function_iconinfo_update(FunctionHandle *handle, Att_List *list)
{
	Att_Node *node;

	// Go through list of files
	for (node = (Att_Node *)list->list.lh_Head; node->node.ln_Succ; node = (Att_Node *)node->node.ln_Succ)
	{
		// Was this icon saved?
		if (node->node.ln_Type == 2)
		{
			Lister *lister;

			// Do we have a lister?
			if ((lister = function_lister_current(&handle->source_paths)))
			{
				FunctionEntry *entry;

				// Get entry pointer
				entry = (FunctionEntry *)node->data;

				// Reload file and icon
				function_filechange_reloadfile(handle, handle->source_path, entry->name, 0);
				function_filechange_reloadfile(handle, handle->source_path, entry->name, FFLF_ICON);
			}
		}
	}
}
