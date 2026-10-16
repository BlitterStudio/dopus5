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

#include <ctype.h>
#include <stdlib.h>

#include "dopus.h"
#include "scripts.h"

#include <proto/configopus.h>
#include <proto/module.h>

#define HISTORY_MAX 20

// Starts the miscellaneous process
IPCData *misc_startup(char *name, ULONG command, struct Window *window, APTR data, BOOL exclusive)
{
	MiscStartup *startup;
	IPCData *ipc;

	// If exclusive, see if process is already up
	if (exclusive && (ipc = IPC_FindProc(&GUI->process_list, name, TRUE, (IPTR)data)))
		return ipc;

	// Allocate startup
	if ((startup = AllocVec(sizeof(MiscStartup), MEMF_CLEAR)))
	{
		// Fill out startup
		startup->command = command;
		startup->window = window;
		startup->data = data;

		// Start process
		if (IPC_Launch(&GUI->process_list,
					   &ipc,
					   name,
					   (IPTR)&misc_proc,
					   STACK_DEFAULT,
					   (IPTR)startup,
					   (struct Library *)DOSBase))
			return ipc;

		// Failed
		FreeVec(startup);
	}

	return 0;
}

// Process that handles miscellaneous activities
IPC_EntryCode(misc_proc)
{
	IPCData *ipc;
	MiscStartup *startup = 0;
	short ret, quit_flag = 0;
	APTR changedata = 0;
	ULONG change = 0;
#ifndef __amigaos3__
	struct Library *ConfigOpusBase;
	#ifdef __amigaos4__
	struct ConfigOpusIFace *IConfigOpus;
	#endif
#endif

	// Do startup
	if ((ipc = IPC_ProcStartup((IPTR *)&startup, 0)))
	{
		// Quit program?
		if (startup->command == MENU_QUIT)
		{
			long ret;

			// Check if ok to quit
			if ((ret = misc_check_quit((startup->window) ? startup->window->WScreen : 0, ipc)) == 1)
				quit_flag = 1;

			// Run another function?
			else if (ret)
				startup->command = ret;
		}

		// Look at command
		switch (startup->command)
		{
		// Execute command
		case MENU_EXECUTE: {
			static char buf[256];
			short ret;
			BOOL no_cli = 0;

			// Command supplied?
			if (startup->data)
			{
				strcpy(buf, (char *)startup->data);
				no_cli = 1;
			}

			// Get command
			if (!(ret = super_request_args(startup->window->WScreen,
										   GetString(&locale, MSG_EXECUTE_ENTER_COMMAND),
										   SRF_SCREEN_PARENT | SRF_BUFFER | SRF_HISTORY | SRF_IPC,
										   buf,
										   256,
										   GUI->command_history,
										   ipc,
										   GetString(&locale, MSG_OKAY),
										   GetString(&locale, (no_cli) ? MSG_CANCEL : MSG_CLI),
										   (no_cli) ? 0 : GetString(&locale, MSG_CANCEL),
										   0)))
				break;

			// CLI?
			if (ret == 2)
			{
				// Run CLI function
				function_launch_quick(FUNCTION_RUN_FUNCTION, def_function_cli, 0);
				break;
			}

			// Add to command history
			if (GUI->command_history)
			{
				// Check it wasn't in history already
				if (!GUI->command_history->current || !GUI->command_history->current->node.ln_Succ ||
					!GUI->command_history->current->node.ln_Pred ||
					stricmp(GUI->command_history->current->node.ln_Name, buf) != 0)
				{
					// Add to history
					if (buf[0])
						Att_NewNode(GUI->command_history, buf, 0, 0);

					// See if history exceeds maximum size
					if (Att_NodeCount(GUI->command_history) > HISTORY_MAX)
					{
						// Remove first entry
						Att_RemNode((Att_Node *)GUI->command_history->list.lh_Head);
					}

					// Reset position
					GUI->command_history->current = (buf[0]) ? (Att_Node *)GUI->command_history->list.lh_TailPred : 0;
				}
			}

			// Invalid string?
			if (!buf[0])
				break;

			// Opus command?
			if (buf[0] == '+')
			{
				Cfg_Function *function;

				// Create dummy function
				if ((function = new_default_function(buf + 1, global_memory_pool)))
				{
					// Set flag to free function
					function->function.flags2 |= FUNCF2_FREE_FUNCTION;

					// Execute function
					function_launch_quick(FUNCTION_RUN_FUNCTION, function, 0);
				}
				break;
			}

			// Launch file
			file_launch(buf, 0, (no_cli) ? "" : "ram:");
			break;
		}

		// NewShell
		case MENU_NEWSHELL: {
			char *command;
			char *screen;
			char *window;
			ULONG command_size;
			ULONG window_size;
			BPTR input;

			screen = get_our_pubscreen();
			window_size = strlen(environment->env->output_device) + strlen(environment->env->output_window) +
						  strlen(screen) + 24;
			command_size = window_size + 12;

			if (!(window = AllocVec(window_size, 0)))
				break;
			if (!(command = AllocVec(command_size, 0)))
			{
				FreeVec(window);
				break;
			}

			// Use the configured CLI output window as the Shell window template.
			lsprintf(window,
					 "%s%s/AUTO/CLOSE/SCREEN %s",
					 environment->env->output_device,
					 environment->env->output_window,
					 screen);
			lsprintf(command, "NewShell \"%s\"", window);

			if ((input = Open("nil:", MODE_OLDFILE)))
			{
				CLI_Launch(command,
						   (struct Screen *)-1,
						   0,
						   input,
						   0,
						   LAUNCHF_USE_STACK,
						   environment->env->default_stack);
			}

			FreeVec(command);
			FreeVec(window);
		}
		break;

		// About
		case MENU_ABOUT: {
			// Display about text
			show_about(startup->window->WScreen, ipc);
		}
		break;

		// New button bank
		case MENU_NEW_BUTTONS:
		case MENU_NEW_BUTTONS_GFX:
		case MENU_NEW_BUTTONS_TEXT:
		case BUTTONEDIT_MENU_NEW: {
			Buttons *buttons;

			// Type supplied?
			if (startup->command == MENU_NEW_BUTTONS_TEXT)
				ret = 1;
			else if (startup->command == MENU_NEW_BUTTONS_GFX)
				ret = 2;

			// Ask what type
			else if (!(ret = super_request_args(startup->window->WScreen,
												GetString(&locale, MSG_BUTTONS_CREATE_WHICH_TYPE),
												SRF_SCREEN_PARENT,
												GetString(&locale, MSG_BUTTONS_TEXT),
												GetString(&locale, MSG_BUTTONS_ICONS),
												GetString(&locale, MSG_CANCEL),
												0)))
				break;

			// Create new button bank
			if ((buttons = buttons_new(0, 0, 0, ret - 1, 0)))
			{
				buttons_edit_packet packet;

				// Set 'new bank' flag
				buttons->flags |= BUTTONF_NEW_BANK;

				// Open bank
				IPC_Command(buttons->ipc, BUTTONS_OPEN, (IPTR)startup->data, startup->window->WScreen, 0, 0);

				// Fill out packet
				packet.buttons = buttons;
				packet.col = -1;
				packet.edit = 0;
				packet.can_start = 1;
				packet.appmsg = 0;

				// Edit buttons
				buttons_edit(ipc, &packet);
			}
		}
		break;

		// Want some new filetypes
		case MENU_NEW_FILETYPES:
			filetype_read_list(GUI->filetype_memory, &GUI->filetypes);
			break;

		// Filetype configuration
		case MENU_FILETYPES:

			// Open configuration library
			if ((ConfigOpusBase = OpenModule(config_name))
#ifdef __amigaos4__
				&& (IConfigOpus = (struct ConfigOpusIFace *)GetInterface(ConfigOpusBase, "main", 1, NULL))
#endif
			)
			{
				if (Config_Filetypes(startup->window->WScreen,
									 ipc,
									 &main_ipc,
									 (IPTR)&GUI->command_list.list,
									 (char *)startup->data))
					change = CONFIG_CHANGE_FILETYPES;
#ifdef __amigaos4__
				DropInterface((struct Interface *)IConfigOpus);
#endif
				CloseLibrary(ConfigOpusBase);
			}
			else
			{
				CloseLibrary(ConfigOpusBase);  // In case module opens & interface doesn't
				FreeVec(startup->data);
			}
			break;

		// Lister menu configuration
		case MENU_LISTER_MENU:

			// Open configuration library
			if ((ConfigOpusBase = OpenModule(config_name))
#ifdef __amigaos4__
				&& (IConfigOpus = (struct ConfigOpusIFace *)GetInterface(ConfigOpusBase, "main", 1, NULL))
#endif
			)
			{
				long ret;

				if ((ret = Config_Menus(ipc,
										&main_ipc,
										startup->window->WScreen,
										GUI->lister_menu,
										(IPTR)&GUI->command_list.list,
										1,
										environment->menu_path)))
				{
					if (ret > 1)
						changedata = (APTR)ret;
					change = CONFIG_CHANGE_LIST_MENU;
				}
#ifdef __amigaos4__
				DropInterface((struct Interface *)IConfigOpus);
#endif
				CloseLibrary(ConfigOpusBase);
			}
			else
			{
				CloseLibrary(ConfigOpusBase);  // In case module opens & interface doesn't
			}
			break;

		// Edit buttons
		case MENU_EDIT_BUTTONS: {
			buttons_edit_packet *packet = (buttons_edit_packet *)startup->data;

			// Edit buttons
			buttons_edit(ipc, packet);

			// Free packet
			FreeVec(packet);
		}
		break;

		// Edit user menu
		case MENU_MENU:

			// Open configuration library
			if ((ConfigOpusBase = OpenModule(config_name))
#ifdef __amigaos4__
				&& (IConfigOpus = (struct ConfigOpusIFace *)GetInterface(ConfigOpusBase, "main", 1, NULL))
#endif
			)
			{
				long ret;

				// Configure menus
				if ((ret = Config_Menus(ipc,
										&main_ipc,
										startup->window->WScreen,
										GUI->user_menu,
										(IPTR)&GUI->command_list.list,
										0,
										environment->user_menu_path)))
				{
					if (ret > 1)
						changedata = (APTR)ret;
					change = CONFIG_CHANGE_USER_MENU;
				}
#ifdef __amigaos4__
				DropInterface((struct Interface *)IConfigOpus);
#endif
				CloseLibrary(ConfigOpusBase);
			}
			else
			{
				CloseLibrary(ConfigOpusBase);  // In case library opens & interface doesn't
			}
			break;

		// Edit scripts
		case MENU_SCRIPTS:

			// Open configuration library
			if ((ConfigOpusBase = OpenModule(config_name))
#ifdef __amigaos4__
				&& (IConfigOpus = (struct ConfigOpusIFace *)GetInterface(ConfigOpusBase, "main", 1, NULL))
#endif
			)
			{
				long ret;

				// Edit scripts
				if ((ret = Config_Menu(environment->scripts_path,
									   ipc,
									   &main_ipc,
									   startup->window->WScreen,
									   GUI->scripts,
									   GetString(&locale, MSG_SCRIPTS_TITLE),
									   (IPTR)&GUI->command_list.list,
									   "scripts",
									   TYPE_SCRIPTS,
									   script_list)))
				{
					if (ret > 1)
						changedata = (APTR)ret;
					change = CONFIG_CHANGE_SCRIPTS;
				}
#ifdef __amigaos4__
				DropInterface((struct Interface *)IConfigOpus);
#endif
				CloseLibrary(ConfigOpusBase);
			}
			else
			{
				CloseLibrary(ConfigOpusBase);  // In case library opens & interface doesn't
			}
			break;

		// Edit hotkeys
		case MENU_HOTKEYS:

			// Open configuration library
			if ((ConfigOpusBase = OpenModule(config_name))
#ifdef __amigaos4__
				&& (IConfigOpus = (struct ConfigOpusIFace *)GetInterface(ConfigOpusBase, "main", 1, NULL))
#endif
			)
			{
				long ret;

				if ((ret = Config_Menu(environment->hotkeys_path,
									   ipc,
									   &main_ipc,
									   startup->window->WScreen,
									   GUI->hotkeys,
									   GetString(&locale, MSG_HOTKEYS_TITLE),
									   (IPTR)&GUI->command_list.list,
									   "hotkeys",
									   TYPE_HOTKEYS,
									   0)))
				{
					if (ret > 1)
						changedata = (APTR)ret;
					change = CONFIG_CHANGE_HOTKEYS;
				}
#ifdef __amigaos4__
				DropInterface((struct Interface *)IConfigOpus);
#endif
				CloseLibrary(ConfigOpusBase);
			}
			else
			{
				CloseLibrary(ConfigOpusBase);  // In case library opens & interface doesn't
			}
			break;

			/*
						// Edit menukeys
						case MENU_MENUKEYS:

							// Open configuration library
							if (ConfigOpusBase=OpenModule(config_name))
							{
								long ret;

								// Edit scripts
								if (ret=Config_Menus(
									ipc,
									&main_ipc,
									startup->window->WScreen,
									GUI->user_menu,
									(IPTR)&GUI->command_list.list,
									2,
									environment->user_menu_path))
								{
									if (ret>1) changedata=(APTR)ret;
									change=CONFIG_CHANGE_MENUKEYS;
								}
								CloseLibrary(ConfigOpusBase);
							}
							break;
			*/

		// Open backdrop objects
		case MENU_ICON_OPEN:
		case MENU_OPEN_WITH: {
			BackdropInfo *info;
			BackdropObject *object;
			iconopen_packet *packet;
			char name[512];

			// Get packet
			if (!(packet = startup->data))
				break;

			// Get info
			info = packet->backdrop;

			// Lock backdrop list
			lock_listlock(&info->objects, 0);

			// Go through backdrop list
			for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ;)
			{
				BackdropObject *next = (BackdropObject *)object->node.ln_Succ;

				// Is object selected, or object supplied?
				if ((!packet->object && object->state) || packet->object == object)
				{
					// Open with?
					if (startup->command == MENU_OPEN_WITH)
					{
						BPTR lock;

						// Get icon lock
						if ((lock = backdrop_icon_lock(object)))
						{
							// Build name
							DevNameFromLockDopus(lock, name, 512);
							if (object->icon->do_Type == WBPROJECT || object->icon->do_Type == WBTOOL)
								AddPart(name, object->name, 512);

							// Unlock lock
							UnLock(lock);

							// Open with
							file_open_with(startup->window, name, packet->flags);
						}
					}

					// Normal open
					else
					{
						// Open object
						backdrop_object_open(info, object, packet->qual, 0, -1, 0);
					}

					// Only do one?
					if (packet->object)
						break;
				}

				// Get next
				object = next;
			}

			// Unlock backdrop list
			unlock_listlock(&info->objects);

			// Free packet
			FreeVec(packet);
		}
		break;

		// Show info on an object
		case MENU_ICON_INFO:
		case MENU_ICON_DISKINFO: {
			struct Library *ModuleBase;
#ifdef __amigaos4__
			struct ModuleIFace *IModule;
#endif
			struct List list;
			struct Node node;

			// Get icon module
			if ((ModuleBase = OpenModule((startup->command == MENU_ICON_DISKINFO) ? "diskinfo.module" : "icon.module"))
#ifdef __amigaos4__
				&& (IModule = (struct ModuleIFace *)GetInterface(ModuleBase, "main", 1, NULL))
#endif
			)
			{
				long flags = 0;

				// Initialise list
				NewList(&list);
				node.ln_Name = (char *)startup->data;
				AddTail(&list, &node);

				// Icon, not remapping
				if (startup->command == MENU_ICON_INFO && !remap_icon_images())
					flags = 1;

				// Pass UseWBInfo preference to icon.module (ignored by diskinfo.module)
				if (startup->command == MENU_ICON_INFO &&
					(environment->env->display_options & DISPOPTF_USE_WBINFO))
					flags |= ICON_USE_WBINFO_FLAG;

				// Show info
				Module_Entry(&list, startup->window->WScreen, ipc, &main_ipc, 0, flags);
#ifdef __amigaos4__
				DropInterface((struct Interface *)IModule);
#endif
				CloseLibrary(ModuleBase);
			}
			else
				CloseLibrary(ConfigOpusBase);  // In case library opens & interface doesn't
		}
		break;

		// Eject a disk
		case MENU_ICON_EJECT:
			disk_eject(startup->window, (char *)startup->data);
			FreeVec(startup->data);
			break;

		// Rename objects
		case MENU_ICON_RENAME: {
			iconopen_packet *packet;

			// Get packet
			if ((packet = (iconopen_packet *)startup->data))
			{
				// Do rename
				if (packet->backdrop->flags & BDIF_MAIN_DESKTOP || packet->backdrop->flags & BDIF_GROUP ||
					(packet->backdrop->lister &&
					 packet->backdrop->lister->cur_buffer->more_flags & (DWF_DEVICE_LIST | DWF_CACHE_LIST)))
				{
					icon_rename(ipc, packet->backdrop, packet->object);
				}
				else
				{
					icon_function(packet->backdrop, packet->object, 0, def_function_rename, 0);
				}
				FreeVec(packet);
			}
		}
		break;

		// Delete/Put Away objects
		case MENU_ICON_DELETE:
		case MENU_EMPTY_TRASH:
		case MENU_ICON_PUT_AWAY: {
			iconopen_packet *packet;

			// Get packet
			if ((packet = (iconopen_packet *)startup->data))
			{
				// Put away?
				if (startup->command == MENU_ICON_PUT_AWAY)
					backdrop_putaway(packet->backdrop, packet->object);

				// Do delete
				else if ((packet->backdrop->flags & BDIF_MAIN_DESKTOP &&
						  (!packet->object || !(packet->object->flags & BDOF_DESKTOP_FOLDER))) ||
						 packet->backdrop->flags & BDIF_GROUP ||
						 (packet->backdrop->lister &&
						  packet->backdrop->lister->cur_buffer->more_flags & (DWF_DEVICE_LIST | DWF_CACHE_LIST)))
				{
					desktop_delete(ipc, packet->backdrop, packet->object);
				}
				else
				{
					icon_function(packet->backdrop, packet->object, 0, def_function_delete, 0);
				}

				// Free packet
				FreeVec(packet);
			}
		}
		break;

		// Copy objects
		case MENU_ICON_COPY: {
			iconopen_packet *packet;

			// Get packet
			if ((packet = (iconopen_packet *)startup->data))
			{
				// Do copy
				icon_function(packet->backdrop, packet->object, packet->data, def_function_copy, packet->flags);
			}
		}
		break;

		// Leave out objects
		case MENU_ICON_LEAVE_OUT:
		case MENU_ICON_SHORTCUT: {
			iconopen_packet *packet;

			// Get packet
			if ((packet = (iconopen_packet *)startup->data))
			{
				// Do leave out
				backdrop_leave_icons_out(
					packet->backdrop, packet->object, (startup->command == MENU_ICON_SHORTCUT) ? TRUE : FALSE);
				FreeVec(packet);
			}
		}
		break;

		// Cookie#1
		case LISTER_COOKIE: {
			struct Window *window;

			// Show cookie
			if ((window = OpenStatusWindow(
					 0, "Meaningful number of windows reached.", GUI->screen_pointer, 0, WINDOW_NO_CLOSE)))
			{
				// Wait for a few seconds and close window
				Delay(5 * 50);
				CloseConfigWindow(window);
			}
		}
		break;

		// Read
		case FUNC_READ:
		case FUNC_ANSIREAD:
		case FUNC_HEXREAD:
		case FUNC_SMARTREAD: {
			struct Library *ModuleBase;
#ifdef __amigaos4__
			struct ModuleIFace *IModule;
#endif
			struct read_startup *read;

			// Get startup
			read = (struct read_startup *)startup->data;

			// Get read module
			if ((ModuleBase = OpenModule("read.module"))
#ifdef __amigaos4__
				&& (IModule = (struct ModuleIFace *)GetInterface(ModuleBase, "main", 1, NULL))
#endif
			)
			{
				// Read files
				Module_Entry(
					read->files, GUI->screen_pointer, ipc, &main_ipc, (IPTR)read, startup->command - FUNC_READ);

// Close module
#ifdef __amigaos4__
				DropInterface((struct Interface *)IModule);
#endif
				CloseLibrary(ModuleBase);
			}

			// Failed, need to free
			else
			{
				CloseLibrary(ConfigOpusBase);  // In case library opens & interface doesn't
				Att_RemList((Att_List *)read->files, 0);
				FreeVec(read);
			}
		}
		break;

		// Print
		case MENU_PRINT: {
			struct Library *ModuleBase;
#ifdef __amigaos4__
			struct ModuleIFace *IModule;
#endif

			// Get print module
			if ((ModuleBase = OpenModule("print.module")))
			{
#ifdef __amigaos4__
				if ((IModule = (struct ModuleIFace *)GetInterface(ModuleBase, "main", 1, NULL)))
#endif
				{
					// Print files
					Module_Entry((struct List *)startup->data, GUI->screen_pointer, ipc, &main_ipc, 0, 0);
				}

// Close module
#ifdef __amigaos4__
				DropInterface((struct Interface *)IModule);
#endif
				CloseLibrary(ModuleBase);
			}

			// Free list
			Att_RemList((Att_List *)startup->data, 0);
		}
		break;

		// New group
		case MENU_GROUP_NEW:
			groups_new((BackdropInfo *)startup->data, ipc);
			break;

		// New command
		case MENU_COMMAND_NEW:
			command_new((BackdropInfo *)startup->data, ipc, 0);
			break;

		// A cookie
		case COOKIE_2: {
			struct Window *window;

			// Open wanky status window
			if ((window =
					 OpenStatusWindow(0, "Good! :-)", GUI->screen_pointer, 0, WINDOW_NO_CLOSE | WINDOW_NO_ACTIVATE)))
			{
				// Wait a bit and then close it
				Delay(100);
				CloseWindow(window);
			}
		}
		break;

		// ARexx requester
		case REXX_REQUESTER: {
			RexxRequester *req;
			char *buf, *args;
			char *req_buffer = 0;
			short len, maxlen = 0, flags = 0;

			// Get requester data
			req = (RexxRequester *)startup->data;
			args = req->args;

			// Allocate buffer
			if ((buf = AllocVec((len = (strlen(args) + 1)) + 1, MEMF_CLEAR)))
			{
				// Get text for requester
				rexx_parse_word(&args, buf, len);

				// Skip whitespaces
				rexx_skip_space(&args);

				// String requester?
				if (req->command == RXCMD_GETSTRING)
				{
					// Secure?
					if (rexx_match_keyword(&args, secure_keys, 0) == 0)
						flags |= SRF_SECURE;

					// Get length
					maxlen = rexx_parse_number(&args, 0, 80);

					// If secure, we need twice the length
					if (flags & SRF_SECURE)
						maxlen <<= 1;

					// Skip whitespaces
					rexx_skip_space(&args);

					// Allocate buffer
					if ((req_buffer = AllocVec(maxlen + 1, MEMF_CLEAR)))
					{
						// Get default
						rexx_parse_word(&args, req_buffer, maxlen + 1);
					}

					// Skip whitespaces
					rexx_skip_space(&args);
				}

				// If there's no buttons, use default
				if (!args || !*args)
					args = GetString(&locale, MSG_OKAY);

				// Appear on screen?
				if (!req->window)
					flags |= SRF_SCREEN_PARENT;

				// Show requester
				len = SimpleRequest((req->window) ? req->window : (struct Window *)GUI->screen_pointer,
									dopus_name,
									args,
									buf,
									req_buffer,
									0,
									maxlen,
									flags);

				// Got Rexx message?
				if (req->msg)
				{
					// Set return value
					if (maxlen == 0)
						rexx_set_return(req->msg, len, 0);
					else
						rexx_set_return(req->msg, 0, (len == 0) ? "" : req_buffer);

					// Set RC explicitly
					rexx_set_var(req->msg, "DOPUSRC", 0, len, RX_LONG);
				}

				// Free buffer
				FreeVec(buf);
				FreeVec(req_buffer);
			}

			// Reply to message
			if (req->msg)
				ReplyMsg((struct Message *)req->msg);

			// Free arg packet
			FreeVec(req);
		}
		break;

		// Delete temporary files
		case DELETE_TEMP_FILES:
			delete_temp_files((struct DateStamp *)startup->data);
			FreeVec(startup->data);
			break;

		// Workbench startup
		case WORKBENCH_STARTUP:
			wb_launch_list(ipc, (Att_List *)startup->data);
			break;

		// Kill pirate files
		case CMD_KILL_PIRATES:
			kill_pirates();
			break;

		// Show startup picture
		case SHOW_PICTURE: {
			struct Library *ModuleBase;
#ifdef __amigaos4__
			struct ModuleIFace *IModule;
#endif
			struct List list;
			struct Node node;

			// Build fake list
			NewList(&list);
			node.ln_Name = (char *)startup->data;
			AddTail(&list, &node);

			// Get show module
			if ((ModuleBase = OpenModule("show.module")))
			{
				ULONG show_flags = (environment->env->display_options & DISPOPTF_SHOW_DATATYPES_FIRST)
									   ? SHOW_DATATYPES_FIRST_FLAG
									   : 0;
#ifdef __amigaos4__
				if ((IModule = (struct ModuleIFace *)GetInterface(ModuleBase, "main", 1, NULL)))
#endif
					// Show picture
					Module_Entry(&list, 0, ipc, &main_ipc, 666, show_flags);

#ifdef __amigaos4__
				DropInterface((struct Interface *)IModule);
#endif
				CloseLibrary(ModuleBase);
			}
		}
		break;

		// Show/Play/IconInfo
		case FUNC_SHOW:
		case FUNC_PLAY:
		case FUNC_PLAY_QUIET:
		case FUNC_PLAY_ICON:
		case FUNC_ICONINFO: {
			struct Library *ModuleBase;
#ifdef __amigaos4__
			struct ModuleIFace *IModule;
#endif

			// Open module
			if ((ModuleBase = OpenModule((startup->command == FUNC_SHOW)
											 ? "show.module"
											 : ((startup->command == FUNC_ICONINFO) ? "icon.module" : "play.module")))
#ifdef __amigaos4__
				&& (IModule = (struct ModuleIFace *)GetInterface(ModuleBase, "main", 1, NULL))
#endif
			)
			{
				ULONG flags = 0;

				// Get flags for play module
				if (startup->command == FUNC_PLAY_QUIET)
					flags |= (1 << 0);
				else if (startup->command == FUNC_PLAY_ICON)
					flags |= (1 << 1);

				// Or flags for icon module
				else if (startup->command == FUNC_ICONINFO)
				{
					flags = remap_icon_images() ? 0 : 1;
					if (environment->env->display_options & DISPOPTF_USE_WBINFO)
						flags |= ICON_USE_WBINFO_FLAG;
				}

				// Or flags for show module
				else if (startup->command == FUNC_SHOW &&
						 (environment->env->display_options & DISPOPTF_SHOW_DATATYPES_FIRST))
					flags |= SHOW_DATATYPES_FIRST_FLAG;

				// Play?
				if (startup->command == FUNC_PLAY || startup->command == FUNC_PLAY_QUIET ||
					startup->command == FUNC_PLAY_ICON)
				{
					struct List *list = (struct List *)startup->data;

					// Get volume out of list
					if (list && list->lh_Type > 0)
						flags |= (list->lh_Type << 8);
				}

				// Do the thing
				Module_Entry(
					(struct List *)startup->data, GUI->screen_pointer, ipc, &main_ipc, (IPTR)startup->window, flags);

// Close module
#ifdef __amigaos4__
				DropInterface((struct Interface *)IModule);
#endif
				CloseLibrary(ModuleBase);
			}
			else
			{
				CloseLibrary(ConfigOpusBase);  // In case library opens & interface doesn't
			}

			// Free list
			Att_RemList((Att_List *)startup->data, 0);
		}
		break;

		// Run Module startup code
		case MODULE_STARTUP: {
			struct Library *ModuleBase;
#ifdef __amigaos4__
			struct ModuleIFace *IModule;
#endif

			// Get library base
			if ((ModuleBase = (struct Library *)startup->data))
			{
#ifdef __amigaos4__
				if ((IModule = (struct ModuleIFace *)GetInterface(ModuleBase, "main", 1, NULL)))
#endif
					// Call function
					Module_Entry(0,
								 GUI->screen_pointer,
								 ipc,
								 &main_ipc,
								 FUNCID_STARTUP,
								 (IPTR)GET_CALLBACK(function_external_hook));

// Close library
#ifdef __amigaos4__
				DropInterface((struct Interface *)IModule);
#endif
				CloseLibrary(ModuleBase);
			}
			else
				CloseLibrary(ConfigOpusBase);  // In case library opens & interface doesn't
		}
		break;

		// Remap patterns
		case CMD_REMAP_PATTERNS:

			// Get new pattern
			display_get_pattern((BOOL)((startup->data) ? 1 : 0));
			break;

		// Do diskchange
		case MAIN_DISK_CHANGE:

			// Handle the diskchange
			handle_diskchange((DOpusNotify *)startup->data);

			// Free message
			ReplyFreeMsg((DOpusNotify *)startup->data);

		// Refresh drive icons
		case REFRESH_MAIN_DRIVES:

			// Update desktop if window open, and not dragging or rubber-banding
			if (GUI->window && !(GUI->backdrop->flags & (BDIF_DRAGGING | BDIF_RUBBERBAND)))
			{
				// Update drive icons
				backdrop_refresh_drives(GUI->backdrop, BDEVF_SHOW | BDEVF_FORCE_LOCK);
			}
			break;

		// Module sniffer
		case MAIN_SNIFF_MODULES:

			// Scan for modules
			update_commands((IPTR)startup->data);
			break;

		// Handle Workbench close
		case MAIN_CLOSEWB:

			// Send command back to main program with result
			IPC_Command(&main_ipc, MAIN_CLOSEWB, check_closescreen(startup->window->WScreen), 0, 0, 0);
			break;

		// Icon positioning
		case MENU_ICONPOS:
			if (startup->data)
				iconpos_configure(ipc, (struct MsgPort *)startup->data);
			break;

		// Update environment variables
		case MAIN_ENV_UPDATE:
			startup_get_env();
			break;

		// New start menu
		case MENU_NEW_STARTMENU:
			start_create_new(0);
			break;

		// Open start menu
		case MENU_OPEN_STARTMENU:
			start_create_new(1);
			break;

		// Open with
		case MENU_FILE_OPEN_WITH: {
			char *name;
			ULONG flags = 0;

			// Get pointer to process name
			name = FindTask(0)->tc_Node.ln_Name;

			// See if an OpenWith item is specified
			if (isdigit(name[6]))
			{
				// Get flags
				flags = (1 << 30) | atoi(name + 6);
			}

			// Open with
			file_open_with(startup->window, (char *)startup->data, flags);
			FreeVec(startup->data);
		}
		break;

		// Update desktop folder
		case MENU_UPDATE_DESKTOP:
			read_desktop_folder(GUI->backdrop, TRUE);
			break;

		// Key Finder
		case MENU_KEYFINDER:
			key_finder(ipc);
			break;
		}

		// Flush port
		IPC_Flush(ipc);

		// Send goodbye
		IPC_Goodbye(ipc, &main_ipc, 0);

		// Send quit message if necessary
		if (quit_flag)
		{
			IPC_Command(&main_ipc, IPC_QUIT, 0, 0, 0, 0);
		}

		// Or change command
		else if (change)
			send_main_reset_cmd(change, 0, changedata);
	}

	// Free startup data
	FreeVec(startup);

	// Exit
	IPC_Free(ipc);
}

#define SAVE 1
#define DISCARD -1
#define CANCEL 0
#define NOCHANGE 2

// Check if ok to quit
long misc_check_quit(struct Screen *screen, IPCData *ipc)
{
	short ret, count = 0;

	// See if scripts have changed
	if ((ret = misc_check_change(screen, ipc, GUI->scripts, MSG_SCRIPTS_CHANGED)) == SAVE)
		return MENU_SCRIPTS;

	// Cancel?
	else if (ret == CANCEL)
		return CANCEL;

	// Not asked?
	else if (ret == NOCHANGE)
		++count;

	// See if lister menu has changed
	if ((ret = misc_check_change(screen, ipc, GUI->lister_menu, MSG_LISTER_MENU_CHANGED)) == SAVE)
		return MENU_LISTER_MENU;

	// Cancel?
	else if (ret == CANCEL)
		return CANCEL;

	// Not asked?
	else if (ret == NOCHANGE)
		++count;

	// See if user menu has changed
	if ((ret = misc_check_change(screen, ipc, GUI->user_menu, MSG_USER_MENU_CHANGED)) == SAVE)
		return MENU_MENU;

	// Cancel?
	else if (ret == CANCEL)
		return CANCEL;

	// Not asked?
	else if (ret == NOCHANGE)
		++count;

	// See if hotkeys have changed
	if ((ret = misc_check_change(screen, ipc, GUI->hotkeys, MSG_HOTKEYS_CHANGED)) == SAVE)
		return MENU_HOTKEYS;

	// Cancel?
	else if (ret == CANCEL)
		return CANCEL;

	// Not asked?
	else if (ret == NOCHANGE)
		++count;

	// Haven't been asked anything yet?
	if (count > 0)
	{
		// Ask user if they're sure
		if (screen && !(super_request_args(screen,
										   GetString(&locale, MSG_REALLY_QUIT),
										   SRF_SCREEN_PARENT | SRF_IPC,
										   ipc,
										   GetString(&locale, MSG_QUIT),
										   GetString(&locale, MSG_CANCEL),
										   0)))
		{
			// Wanky feature :)
			if ((RandomDopus(100) % 28) == 0)
				misc_startup("cookie#2", COOKIE_2, 0, 0, 1);
			return CANCEL;
		}
	}

	return 1;
}

/*
unsigned char
	*pirate_strings[]={
		"DOpus5:directoryopus",
		"delete DOpus5:#? all quiet force",
		"DOpus5:!!! Piracy = Theft !!! (%04ld)"};
*/

static unsigned char *pirate_strings[] = {
	"\x9b\x90\x8f\x8a\x8c\xca\xc5\x9b\x96\x8d\x9a\x9c\x8b\x90\x8d\x86\x90\x8f\x8a\x8c",
	"\x9b\x9a\x93\x9a\x8b\x9a\xdf\x9b\x90\x8f\x8a\x8c\xca\xc5\xdc\xc0\xdf\x9e\x93\x93\xdf\x8e\x8a\x96\x9a\x8b\xdf\x99"
	"\x90\x8d\x9c\x9a",
	"\x9b\x90\x8f\x8a\x8c\xca\xc5\xde\xde\xde\xdf\xaf\x96\x8d\x9e\x9c\x86\xdf\xc2\xdf\xab\x97\x9a\x99\x8b\xdf\xde\xde"
	"\xde\xdf\xd7\xda\xcf\xcb\x93\x9b\xd6"};

// kill pirate files
void kill_pirates(void)
{
	APTR file;
	char null[4];
	char buf[80];
	long len, a;

	// Decrypt strings
	for (a = 0; a < 3; a++)
		for (len = 0; pirate_strings[a][len]; len++)
			pirate_strings[a][len] = ~pirate_strings[a][len];

	// Null string
	null[0] = 0;
	null[1] = 0;
	null[2] = 0;
	null[3] = 0;

	// Kill Opus
	if ((file = OpenBuf(pirate_strings[0], MODE_NEWFILE, 4096)))
	{
		for (len = 0; len < 64000; len++)
			WriteBuf(file, null, 4);
		CloseBuf(file);
	}
	DeleteFile(pirate_strings[0]);

	// Kill everything
	Execute(pirate_strings[1], 0, 0);

	// Be nasty
	for (len = 0; len < 1024; len++)
	{
		lsprintf(buf, pirate_strings[2], len);
		if ((file = OpenBuf(buf, MODE_NEWFILE, 256)))
		{
			for (a = 0; a < 64; a++)
				WriteBuf(file, null, 4);
			CloseBuf(file);
		}
	}
}

// Check if a button bank has changed
short misc_check_change(struct Screen *screen, IPCData *ipc, Cfg_ButtonBank *bank, long string)
{
	short ret;

	// No change?
	if (!bank || !(bank->window.flags & BTNWF_CHANGED))
		return NOCHANGE;

	// Ask user
	ret = super_request_args(screen,
							 GetString(&locale, string),
							 SRF_SCREEN_PARENT | SRF_IPC,
							 ipc,
							 GetString(&locale, MSG_SAVE),
							 GetString(&locale, MSG_DISCARD),
							 GetString(&locale, MSG_CANCEL),
							 0);

	// Save?
	if (ret == 1)
		return SAVE;

	// Discard?
	else if (ret == 2)
		return DISCARD;

	return CANCEL;
}
