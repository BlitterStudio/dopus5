/*

Directory Opus 5
Original APL release version 5.82
Copyright 1993-2012 Jonathan Potter & GP Software
Copyright 2012-2013 DOPUS5 Open Source Team
Copyright 2023-2026 Dimitris Panokostas (dopus5 fork)

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

#define DELETE_TIMER 300
#define MENU_TIMER 5
#define FILETYPE_TIMER 5
#define DESKTOP_TIMER 5
#define SEED_TIMER 60

void handle_icon_notify(DOpusNotify *);

TimerHandle *filetype_timer = 0;

// Main event loop
void event_loop()
{
	struct IntuiMessage *int_msg;
	IPCMessage *ipc_msg;
	struct Message *msg;
	TimerHandle *drag_timer;
	TimerHandle *delete_timer;
	TimerHandle *menu_timer = 0;
	TimerHandle *open_timer;
	TimerHandle *desktop_timer = 0;
	TimerHandle *seed_timer;
	struct DateStamp last_janitor;
	struct MinList notify_list;
	ULONG seed = 1;

	// Open timer for drag deadlock checking and menu updating
	if (!(drag_timer = AllocTimer(UNIT_VBLANK, 0)) || !(menu_timer = AllocTimer(UNIT_VBLANK, drag_timer->port)) ||
		!(desktop_timer = AllocTimer(UNIT_VBLANK, drag_timer->port)))
	{
		FreeTimer(drag_timer);
		FreeTimer(menu_timer);
		quit(0);
	}

	// Open timer for t: deleting
	if ((delete_timer = AllocTimer(UNIT_VBLANK, drag_timer->port)))
		StartTimer(delete_timer, DELETE_TIMER, 0);

	// Open timer for filetype updating
	filetype_timer = AllocTimer(UNIT_VBLANK, drag_timer->port);

	// Open timer to check for screen needing to open
	open_timer = AllocTimer(UNIT_VBLANK, drag_timer->port);

	// Start random seed timer
	if ((seed_timer = AllocTimer(UNIT_VBLANK, drag_timer->port)))
		StartTimer(seed_timer, SEED_TIMER, 0);

	// Datestamp for janitor
	DateStamp(&last_janitor);

	// Initialise list
	NewList((struct List *)&notify_list);

	// Loop forever
	FOREVER
	{
		BOOL quit_flag = 0;
		ULONG res;

		// Application messages?
		while ((msg = GetMsg(GUI->appmsg_port)))
		{
			// App message reply?
			if (msg->mn_Node.ln_Type == NT_REPLYMSG && (((DOpusAppMessage *)msg)->da_Msg.am_Type == MTYPE_APPWINDOW ||
														((DOpusAppMessage *)msg)->da_Msg.am_Type == MTYPE_APPICON ||
														((DOpusAppMessage *)msg)->da_Msg.am_Type == MTYPE_APPMENUITEM ||
														((DOpusAppMessage *)msg)->da_Msg.am_Type == MTYPE_APPSNAPSHOT))
			{
				// Free the message
				FreeAppMessage((DOpusAppMessage *)msg);
				msg = 0;
			}

			// File drop on backdrop window?
			else if ((((DOpusAppMessage *)msg)->da_Msg.am_Type == MTYPE_DOPUS ||
					  ((DOpusAppMessage *)msg)->da_Msg.am_Type == MTYPE_APPWINDOW) &&
					 ((DOpusAppMessage *)msg)->da_Msg.am_ID == WINDOW_BACKDROP)
			{
				// Handle drop
				desktop_drop(GUI->backdrop, (DOpusAppMessage *)msg, PeekQualifier());
				msg = 0;
			}

			// Hidden App message
			else if ((((DOpusAppMessage *)msg)->da_Msg.am_Type == MTYPE_APPICON ||
					  ((DOpusAppMessage *)msg)->da_Msg.am_Type == MTYPE_APPWINDOW ||
					  ((DOpusAppMessage *)msg)->da_Msg.am_Type == MTYPE_APPMENUITEM) &&
					 ((DOpusAppMessage *)msg)->da_Msg.am_ID == 0x12345678)
			{
				// Files dropped on AppIcon?
				if ((((DOpusAppMessage *)msg)->da_Msg.am_Type == MTYPE_APPICON ||
					 ((DOpusAppMessage *)msg)->da_Msg.am_Type == MTYPE_APPWINDOW) &&
					((DOpusAppMessage *)msg)->da_Msg.am_NumArgs > 0)
				{
					struct ArgArray *arg_array;
					char *pathname;
					DOpusAppMessage *amsg = (DOpusAppMessage *)msg;
					BPTR lock;

					// Allocate buffer
					if ((pathname = AllocVec(512, 0)))
					{
						// Get arg array
						if ((arg_array = AppArgArray(amsg, 0)))
						{
							// Get pathname of first file
							DevNameFromLockDopus(amsg->da_Msg.am_ArgList[0].wa_Lock, pathname, 512);

							// Need source directory; if no name, get parent
							if ((!amsg->da_Msg.am_ArgList[0].wa_Name || !*amsg->da_Msg.am_ArgList[0].wa_Name) &&
								(lock = ParentDir(amsg->da_Msg.am_ArgList[0].wa_Lock)))
							{
								// Get pathname of parent
								DevNameFromLockDopus(lock, pathname, 512);
								UnLock(lock);
							}

							// Do filetype action
							function_launch(
								FUNCTION_FILETYPE, 0, FTTYPE_DOUBLE_CLICK, 0, 0, 0, pathname, 0, arg_array, 0, 0);
						}

						// Free buffer
						FreeVec(pathname);
					}
				}

				// Otherwise, re-open program
				else
				{
					// Display screen
					display_open(0);
				}

				// Reply message
				ReplyAppMessage((DOpusAppMessage *)msg);
				msg = 0;
			}

			// Notify message?
			else if (((struct NotifyMessage *)msg)->nm_Class == NOTIFY_CLASS)
			{
				struct NotifyMessage *notify;

				// Get pointer to notify message
				notify = (struct NotifyMessage *)msg;

				// Which notify?
				switch (notify->nm_NReq->nr_UserData)
				{
				// Filetypes changed?
				case NOTIFY_FILETYPES_CHANGED:

					// Start filetype timer
					StartTimer(filetype_timer, FILETYPE_TIMER, 0);
					break;

				// Pattern changed?
				case NOTIFY_PATTERN_CHANGED:

					// Ignore if iconified, or pattern disabled
					if (GUI->window && !(environment->env->display_options & DISPOPTF_NO_BACKDROP) &&
						environment->env->display_options & DISPOPTF_USE_WBPATTERN)
					{
						// Launch process to read pattern
						misc_startup(NAME_PATTERNS, CMD_REMAP_PATTERNS, 0, 0, TRUE);
					}
					break;

				// Modules changed?
				case NOTIFY_MODULES_CHANGED:

					// Not disabled?
					if (!(GUI->flags & GUIF_NO_NOTIFY))
					{
						// Launch process to update modules
						misc_startup("dopus_module_sniffer", MAIN_SNIFF_MODULES, GUI->window, (APTR)SNIFF_BOTH, 1);
					}
					break;

				// Environment variables changed?
				case NOTIFY_ENV_CHANGED:

					// Update environment variables
					misc_startup("dopus_env_rooter", MAIN_ENV_UPDATE, GUI->window, 0, 1);
					break;

				// Desktop changed?
				case NOTIFY_DESKTOP_CHANGED:

					// Start desktop timer
					StartTimer(desktop_timer, DESKTOP_TIMER, 0);
					break;

				// User commands changed?
				case NOTIFY_COMMANDS_CHANGED:

					// Launch process to update modules
					misc_startup("dopus_command_sniffer", MAIN_SNIFF_MODULES, GUI->window, (APTR)SNIFF_USER, 1);
					break;
				}
			}

			// Commodities message?
			else if (CxMsgType((CxMsg *)msg) == CXM_COMMAND || CxMsgType((CxMsg *)msg) == CXM_IEVENT)
			{
				CxMsg *cxmsg;
				long id;

				// Get pointer to cx message
				cxmsg = (CxMsg *)msg;

				// Look at the message type
				switch (CxMsgType(cxmsg))
				{
				// Input event
				case CXM_IEVENT:

					// Get id
					id = CxMsgID(cxmsg);

					// Pop-up?
					if (id == CXCODE_POPUP)
						display_popup(POPUP_NORMAL);

					// A global hotkey?
					else if (id >= CXCODE_HOTKEY_BASE)
					{
						cx_hotkey(id - CXCODE_HOTKEY_BASE);
					}

					break;

				// Command
				case CXM_COMMAND:

					// Look at command ID
					switch (CxMsgID(cxmsg))
					{
					// Kill
					case CXCMD_KILL:
						quit_flag = 1;
						break;

						/*
													// Disable
													case CXCMD_DISABLE:
														ActivateCxObj(GUI->cx->broker,0);
														break;

													// Enable
													case CXCMD_ENABLE:
														ActivateCxObj(GUI->cx->broker,1);
														break;
						*/

					// Show
					case CXCMD_APPEAR:

						// Popup display
						display_popup(POPUP_NORMAL);
						break;

					// Hide
					case CXCMD_DISAPPEAR:
						hide_display();
						break;
					}
					break;
				}
			}

			// Reply the message
			if (msg)
				ReplyMsg(msg);
		}

		// IPC messages
		while ((ipc_msg = (IPCMessage *)GetMsg(main_ipc.command_port)))
		{
			// Look at command
			switch (ipc_msg->command)
			{
			// Quit
			case IPC_QUIT:
				quit_flag = 1;
				break;

			// Hide
			case IPC_HIDE:
				hide_display();
				break;

			// Show
			case IPC_SHOW:

				// Do popup
				display_popup(POPUP_NORMAL);
				break;

			// Identify
			case IPC_IDENTIFY:
				stccpy((char *)ipc_msg->data, GUI->rexx_port_name, ipc_msg->flags);
				break;

			// Menu event
			case MAIN_MENU_EVENT:

				// Process menu event
				quit_flag = menu_process_event(((MenuEvent *)ipc_msg->data_free)->id,
											   ((MenuEvent *)ipc_msg->data_free)->menu,
											   ((MenuEvent *)ipc_msg->data_free)->window);
				break;

			// Command
			case MAINCMD_COMMAND:

				// Process command
				quit_flag = menu_process_event(ipc_msg->flags, 0, GUI->window);
				break;

			// Get command list
			case MAINCMD_GET_LIST:

				// Return pointer to command list
				ipc_msg->command = (IPTR)&GUI->command_list.list;
				break;

			// Reset display
			case MAINCMD_RESET:

				// Handle reset
				main_handle_reset((ULONG *)ipc_msg->data_free, ipc_msg->data);
				break;

			// Reset tools menu
			case MAINCMD_RESET_TOOLMENU:

				// No tools menu?
				if (ipc_msg->flags)
					GUI->flags |= GUIF_NO_TOOLS_MENU;
				else
					GUI->flags &= ~GUIF_NO_TOOLS_MENU;

				// Rebuild and reset menus
				display_build_user_menu();
				display_reset_menus(0, 0);
				break;

			// Close display
			case MAINCMD_CLOSE_DISPLAY:
				close_display(CLOSE_ALL | CLOSE_STOREPOS, ipc_msg->flags);
				break;

			// Open display
			case MAINCMD_OPEN_DISPLAY:

				// Open display
				if (!GUI->window)
					display_open(ipc_msg->flags);

				// If the load that triggered this reopen flagged
				// any missing button banks, prompt the user about
				// them now (display is up).
				environment_resolve_missing_banks(environment, GUI->window);
				break;

			// Get screen data
			case MAINCMD_GET_SCREEN_DATA:
				get_screen_data((DOpusScreenData *)ipc_msg->data);
				break;

			// Help
			case IPC_HELP: {
				char *name, *file = 0;

				// Get name
				name = (ipc_msg->flags & HELP_REAL) ? (char *)ipc_msg->data : HELP_MAIN;

				// Get file
				if (ipc_msg->flags & HELP_REAL && ipc_msg->flags & HELP_NEWFILE)
					file = name + strlen(name) + 1;

				// Show help
				help_show_help(name, file);
			}
			break;

			// Change priority
			case IPC_PRIORITY:
				SetTaskPri(FindTask(0), (long)ipc_msg->data);
				break;

			// Backfill pattern has changed
			case LISTER_BACKFILL_CHANGE:

				// Window open?
				if (GUI->window)
				{
					// Install appropriate hook
					InstallLayerHook(GUI->window->WLayer, (ipc_msg->flags) ? &GUI->main_pattern.hook : LAYERS_BACKFILL);

					// Refresh window
					backdrop_show_objects(GUI->backdrop, BDSF_CLEAR);
				}
				break;

			// Save position list
			case MAINCMD_SAVE_POSITIONS:

				// Make window busy
				if (GUI->window)
					SetBusyPointer(GUI->window);

				// Lock list and save it out
				lock_listlock(&GUI->positions, FALSE);
				SavePositions(&GUI->positions.list, GUI->position_name);
				unlock_listlock(&GUI->positions);

				// Clear pointer
				if (GUI->window)
					ClearPointer(GUI->window);
				break;

			// Load position list
			case MAINCMD_LOAD_POSITIONS:

				// Make window busy
				if (GUI->window)
					SetBusyPointer(GUI->window);

				// Lock position list
				lock_listlock(&GUI->positions, TRUE);

				// Free existing positions
				FreePositions(&GUI->positions.list);

				// Load new positions
				GetPositions(&GUI->positions, GUI->position_memory, GUI->position_name);

				// Unlock position list
				unlock_listlock(&GUI->positions);

				// Clear pointer
				if (GUI->window)
					ClearPointer(GUI->window);
				break;

			// Close for Workbench screen to close
			case MAIN_CLOSEWB:

				// Ok to close?
				if (ipc_msg->flags)
				{
					// Close display
					close_display(CLOSE_ALL | CLOSE_STOREPOS, 0);

					// Set flag
					GUI->flags |= GUIF_CLOSED;

					// Close the Workbench screen again (to be sure)
					CloseWorkBench();

					// Start timer to check for re-opening
					StartTimer(open_timer, 1, 0);
				}

				// Not OK; clear flag
				else
					GUI->flags &= ~GUIF_CLOSE_PENDING;
				break;

			// Refresh drives
			case MAINCMD_REFRESH_DRIVES:

				// Update device list
				misc_startup("dopus_refresh_drives", REFRESH_MAIN_DRIVES, 0, 0, 0);
				break;

			// Get callbacks
			case MAINCMD_GET_CALLBACKS: {
				DOpusCallbackInfo *info;

				// Get info pointer
				if ((info = (DOpusCallbackInfo *)ipc_msg->data))
				{
					// Initialise hooks
					ipc_msg->command = HookInitHooks(info);
				}
			}
			break;

			// Get a custom pen
			case MAINCMD_GET_PEN:

				// Get the pen
				ipc_msg->command = GetCustomPen(
					(ipc_msg->flags >> 16) & 0xffff, ipc_msg->flags & 0xffff, (ColourSpec32 *)ipc_msg->data);
				break;

			// Free a custom pen
			case MAINCMD_RELEASE_PEN:

				// Free the pen
				FreeCustomPen((ipc_msg->flags >> 16) & 0xffff, ipc_msg->flags & 0xffff);
				break;

			// Get an icon
			case MAINCMD_GET_ICON:

				// Get the icon
				ipc_msg->command = (IPTR)GetProperIcon((char *)ipc_msg->data, 0, ipc_msg->flags);
				break;

			// Save environment
			case MAINCMD_SAVE_ENV:
				ipc_msg->command =
					(IPTR)environment_save(environment, (char *)ipc_msg->data, 0, (CFG_ENVR *)ipc_msg->flags);
				break;
			}

			// Reply to the message
			IPC_Reply(ipc_msg);
		}

		// Icon notification
		if (GUI->backdrop->notify_req)
		{
			DOpusNotify *notify;

			// Get notify message
			while ((notify = (DOpusNotify *)GetMsg(GUI->backdrop->notify_port)))
				backdrop_check_notify(GUI->backdrop, notify, 0);
		}

		// App notification
		if (GUI->notify_req)
		{
			DOpusNotify *notify;

			// Get notify message
			while ((notify = (DOpusNotify *)GetMsg(GUI->notify_port)))
			{
				// AppMenu?
				if (notify->dn_Type == DN_APP_MENU_LIST && (environment->env->display_options & DISPOPTF_SHOW_TOOLS))
				{
					// Is it an AppMenu change from ToolManger?
					if (notify->dn_Flags)
					{
						// Menu timer not already running?
						if (!(TimerActive(menu_timer)))
						{
							// Start timer
							StartTimer(menu_timer, MENU_TIMER, 0);

							// Signal to remove tool menu
							IPC_Command(&main_ipc, MAINCMD_RESET_TOOLMENU, 1, 0, 0, 0);
						}
					}

					// Signal menu update immediately
					else
						IPC_Command(&main_ipc, MAINCMD_RESET_TOOLMENU, 0, 0, 0, 0);
				}

				// AppIcon?
				else if (notify->dn_Type == DN_APP_ICON_LIST && notify->dn_Data)
				{
					// If we're dragging, process later
					if (GUI->backdrop->flags & (BDIF_DRAGGING | BDIF_RUBBERBAND))
					{
						// Add to notify list
						AddTail((struct List *)&notify_list, (struct Node *)notify);
						notify = 0;
					}

					// Handle it
					else
						handle_icon_notify(notify);
				}

				// Close Workbench?
				else if (notify->dn_Type == DN_CLOSE_WORKBENCH)
				{
					// Is DOpus on the Workbench screen?
					if (!(GUI->flags & GUIF_CLOSE_PENDING) && !GUI->screen && GUI->window &&
						strcmp(get_our_pubscreen(), "Workbench") == 0)
					{
						// Set flag
						GUI->flags |= GUIF_CLOSE_PENDING;

						// Launch process to shut Opus down
						misc_startup("dopus_closewb", MAIN_CLOSEWB, GUI->window, 0, TRUE);
					}
				}

				// Open Workbench?
				else if (notify->dn_Type == DN_OPEN_WORKBENCH)
				{
					// Close pending?
					if (GUI->flags & GUIF_CLOSE_PENDING)
					{
						// Was window actually closed?
						if (GUI->flags & GUIF_CLOSED)
						{
							// Re-open display
							if (!GUI->window)
								display_open(0);

							// Clear flags
							GUI->flags &= ~(GUIF_CLOSED | GUIF_CLOSE_PENDING);
						}
					}
				}

				// Diskchange (ignore if iconified)
				else if (notify->dn_Type == DN_DISKCHANGE && GUI->window)
				{
					// Launch process to handle this
					misc_startup("dopus_disk_change", MAIN_DISK_CHANGE, GUI->window, notify, 0);  // ** not exclusive
					notify = 0;
				}

				// REXX started up
				else if (notify->dn_Type == DN_REXX_UP)
				{
					// Wasn't running before?
					if (!(GUI->flags & GUIF_REXX))
					{
						// Set flag
						GUI->flags |= GUIF_REXX;

						// Launch process to scan for modules
						misc_startup("dopus_module_sniffer", MAIN_SNIFF_MODULES, GUI->window, SNIFF_REXX, 0);
					}
				}

				// Flush memory
				else if (notify->dn_Type == DN_FLUSH_MEM)
				{
					// Flush buffers
					buffer_mem_handler(notify->dn_Flags);
				}

				// Free message
				if (notify)
					ReplyFreeMsg(notify);
			}
		}

		// Check for IntuiMessages from main window
		if (GUI->window)
		{
			while ((int_msg = (struct IntuiMessage *)GetMsg(GUI->window->UserPort)))
			{
				struct IntuiMessage msg_copy;

				// Copy message
				msg_copy = *int_msg;

				// Add message to random seed
				if (seed_timer)
					seed *= msg_copy.Class + msg_copy.Code + msg_copy.MouseX + msg_copy.MouseY + msg_copy.Seconds +
							msg_copy.Micros + 1;

				// Menu verify?
				if (int_msg->Class == IDCMP_MENUVERIFY)
				{
					// See if want to swallow it
					backdrop_test_rmb(GUI->backdrop, int_msg, &msg_copy, BTRM_ICON);
				}

				// Reply to message if not refresh
				if (int_msg->Class != IDCMP_REFRESHWINDOW)
				{
					// Reply to message
					ReplyMsg((struct Message *)int_msg);
					int_msg = 0;
				}

				// Inactive window does menu up
				if (msg_copy.Class == IDCMP_INACTIVEWINDOW)
				{
					msg_copy.Class = IDCMP_MOUSEBUTTONS;
					msg_copy.Code = MENUUP;
				}

				// Can backdrop handle it?
				if (backdrop_idcmp(GUI->backdrop, &msg_copy, BIDCMPF_DOUBLECLICK))
				{
					// Started dragging?
					if (GUI->backdrop->flags & (BDIF_DRAGGING | BDIF_RUBBERBAND))
					{
						// Start drag timer
						if (!drag_timer->active)
							StartTimer(drag_timer, 0, 500000);
					}

					// Stopped dragging?
					else
					{
						// Stop drag timer
						StopTimer(drag_timer);

						// Notify messages to process?
						while (!(IsListEmpty((struct List *)&notify_list)))
						{
							DOpusNotify *notify;

							// Get message
							notify = (DOpusNotify *)notify_list.mlh_Head;

							// Handle it
							handle_icon_notify(notify);

							// Remove and free the message
							Remove((struct Node *)notify);
							ReplyFreeMsg(notify);
						}
					}
				}

				// Look at message class
				else
					switch (msg_copy.Class)
					{
					// Window closed
					case IDCMP_CLOSEWINDOW:
						quit_flag = menu_process_event(MENU_QUIT, 0, GUI->window);
						break;

					// Gadget
					case IDCMP_GADGETUP: {
						struct Gadget *gadget = (struct Gadget *)msg_copy.IAddress;

						// Iconify?
						if (gadget->GadgetID == GAD_ICONIFY)
						{
							hide_display();
						}
					}
					break;

					// Menu picked
					case IDCMP_MENUHELP:
					case IDCMP_MENUPICK: {
						struct MenuItem *item;
						struct Menu *oldstrip = GUI->window->MenuStrip;
						UWORD nextselect;

						// Get item
						nextselect = msg_copy.Code;
						while ((item = ItemAddress(GUI->window->MenuStrip, nextselect)))
						{
							// Get next
							nextselect = item->NextSelect;

							// Help?
							if (msg_copy.Class == IDCMP_MENUHELP)
							{
								help_menu_help((long)GTMENUITEM_USERDATA(item), 0);
								break;
							}

							// Process menu event
							if ((quit_flag = menu_process_event((IPTR)GTMENUITEM_USERDATA(item), item, GUI->window)) ||
								!GUI->window || oldstrip != GUI->window->MenuStrip)
								break;

							// Check next selection
							if (!nextselect)
								break;
						}
					}
					break;

					// Disk change
					case IDCMP_DISKINSERTED:
					case IDCMP_DISKREMOVED:

						// Notify the library of a disk change
						NotifyDiskChange();
						break;

					// Key press
					case IDCMP_RAWKEY: {
						Cfg_Function *function;

						// Help?
						if (msg_copy.Code == 0x5f && !(msg_copy.Qualifier & VALID_QUALIFIERS))
						{
							help_get_help(msg_copy.MouseX + GUI->window->LeftEdge,
										  msg_copy.MouseY + GUI->window->TopEdge,
										  msg_copy.Qualifier);
							break;
						}

						// Can we match a function key?
						if ((function =
								 match_function_key(msg_copy.Code, msg_copy.Qualifier, 0, 0, WINDOW_BACKDROP, 0)))
						{
							// Run function
							function_launch_quick(FUNCTION_RUN_FUNCTION, function, 0);
						}

						// Refresh desktop?
						else if (msg_copy.Code == 0x54)
						{
							// Update the desktop folder
							misc_startup("dopus_desktop_update", MENU_UPDATE_DESKTOP, GUI->window, 0, TRUE);

							// Stop desktop timer
							StopTimer(desktop_timer);
						}
					}
					break;
					}

				// Reply to message if not already replied
				if (int_msg)
					ReplyMsg((struct Message *)int_msg);

				// Check window is still valid
				if (!GUI->window)
					break;
			}
		}

		// Check quit flag
		if (quit_flag)
			break;

		// Check for IntuiMessages from icon positioning windows
		if (GUI->iconpos_port && (GUI->flags2 & GUIF2_ICONPOS))
		{
			while ((int_msg = (struct IntuiMessage *)GetMsg(GUI->iconpos_port)))
			{
				// Pass to handler
				if (iconpos_idcmp(int_msg))
				{
					// Reply the message
					ReplyMsg((struct Message *)int_msg);
				}
			}
		}

		// Wait for an event
		if ((res = Wait(1 << main_ipc.command_port->mp_SigBit | 1 << GUI->appmsg_port->mp_SigBit |
						1 << drag_timer->port->mp_SigBit |
						((GUI->backdrop->notify_req) ? (1 << GUI->backdrop->notify_port->mp_SigBit) : 0) |
						((GUI->notify_port) ? (1 << GUI->notify_port->mp_SigBit) : 0) |
						((GUI->window) ? (1 << GUI->window->UserPort->mp_SigBit) : 0) |
						((GUI->iconpos_port) ? (1 << GUI->iconpos_port->mp_SigBit) : 0) | IPCSIG_SHOW)) &
			IPCSIG_SHOW)
		{
			// Do popup
			display_popup(POPUP_NORMAL);
		}

		else if (res & (1 << drag_timer->port->mp_SigBit))
		{
			// Seed timer?
			if (seed_timer && CheckTimer(seed_timer))
			{
				BPTR file;
				char buf[20];
				struct Process *proc;
				APTR wsave;

				// Save seed value
				lsprintf(buf, "%ld", seed);

				// Get this process, turn off requesters
				proc = (struct Process *)FindTask(NULL);
				wsave = proc->pr_WindowPtr;
				proc->pr_WindowPtr = (APTR)-1;

				if ((file = Open("DOpus5:System/seed", MODE_NEWFILE)))
				{
					Write(file, buf, strlen(buf));
					Close(file);
				}

				// Restore requesters
				proc->pr_WindowPtr = wsave;

				// Free timer
				FreeTimer(seed_timer);
				seed_timer = 0;
			}

			// Open timer?
			if (GUI->flags & GUIF_CLOSED && CheckTimer(open_timer))
			{
				// Re-open display
				if (!GUI->window)
					display_open(0);

				// Clear flags
				GUI->flags &= ~(GUIF_CLOSED | GUIF_CLOSE_PENDING);
			}

			// Menu timer?
			if (CheckTimer(menu_timer))
			{
				// Signal menu update
				IPC_Command(&main_ipc, MAINCMD_RESET_TOOLMENU, 0, 0, 0, 0);
			}

			// Filetype timer?
			if (CheckTimer(filetype_timer))
			{
				struct MsgPort *port;

				// See if locking port is present
				Forbid();
				port = FindPort("FILETYPE_LOCK");
				Permit();

				// Found it?
				if (port)
				{
					// Start timer to try again
					StartTimer(filetype_timer, FILETYPE_TIMER, 0);
				}

				// Otherwise, launch process to update filetypes
				else
					misc_startup("dopus_filetypes", MENU_NEW_FILETYPES, 0, 0, 1);
			}

			// Drag timer?
			if (CheckTimer(drag_timer))
			{
				// Dragging something?
				if (GUI->backdrop->flags & (BDIF_DRAGGING | BDIF_RUBBERBAND))
				{
					// Check for deadlocks
					if (GUI->backdrop->tick_count == GUI->backdrop->last_tick)
					{
						// Stop drag
						backdrop_stop_drag(GUI->backdrop);
					}

					// Restart timer
					StartTimer(drag_timer, 0, 500000);

					// Remember tick count
					GUI->backdrop->last_tick = GUI->backdrop->tick_count;
				}
			}

			// Delete timer?
			if (CheckTimer(delete_timer))
			{
				struct DateStamp *copy;

				// Copy datestamp
				if ((copy = AllocVec(sizeof(struct DateStamp), 0)))
				{
					// Launch deleter
					*copy = last_janitor;
					misc_startup("dopus_janitor", DELETE_TEMP_FILES, 0, copy, 1);
				}

				// Restart timer
				StartTimer(delete_timer, DELETE_TIMER, 0);

				// Datestamp for janitor
				DateStamp(&last_janitor);
			}

			// Desktop timer?
			if (CheckTimer(desktop_timer))
			{
				// Update the desktop folder
				misc_startup("dopus_desktop_update", MENU_UPDATE_DESKTOP, GUI->window, 0, TRUE);
			}
		}
	}

	// Set busy pointer
	if (GUI->window)
		SetBusyPointer(GUI->window);

	// Messages to clean up
	while (!(IsListEmpty((struct List *)&notify_list)))
	{
		struct MinNode *node;

		// Get pointer
		node = notify_list.mlh_Head;

		// Remove and free the message
		Remove((struct Node *)node);
		ReplyFreeMsg((struct Message *)node);
	}

	// Close timers
	FreeTimer(open_timer);
	FreeTimer(filetype_timer);
	FreeTimer(delete_timer);
	FreeTimer(menu_timer);
	FreeTimer(seed_timer);
	FreeTimer(drag_timer);
	FreeTimer(desktop_timer);

	// Stop drag
	backdrop_stop_drag(GUI->backdrop);

	// Exit
	quit(1);
}

// Process menu events
BOOL menu_process_event(IPTR id, struct MenuItem *item, struct Window *window)
{
	Cfg_Button *button;
	Cfg_Function *function = 0;
	static short cookie = 0;
	static ULONG cookie_seconds, cookie_micros;

	// Select all?
	if (id == MENU_ICON_SELECT_ALL)
	{
		// Ok?
		if (cookie == 0)
		{
			cookie = 1;
			CurrentTime(&cookie_seconds, &cookie_micros);
		}
	}

	// Format?
	else if (id == MENU_ICON_FORMAT)
	{
		// Ok?
		if (cookie == 1)
		{
			// Shift down?
			if (PeekQualifier() & IEQUALIFIER_RSHIFT)
			{
				ULONG sec, mic;

				// Get time
				CurrentTime(&sec, &mic);

				// Check time is ok
				if (sec <= cookie_seconds + 2)
				{
					// Show text
					global_requester("I hate wearing short pants, don't you?");

					// No more
					cookie = -1;
					return 0;
				}
				else
					cookie = 0;
			}
			else
				cookie = 0;
		}
	}

	// Other
	else if (cookie == 1)
		cookie = 0;

	// Look at menu ID
	switch (id)
	{
	// Toggle backdrop
	case MENU_BACKDROP:

		// Close main window
		close_display(CLOSE_WINDOW | CLOSE_KEEP_PATTERN | CLOSE_KEEP_REMAP, FALSE);

		// Toggle backdrop flag
		environment->env->env_flags ^= ENVF_BACKDROP;

		// Re-open main window
		display_open(DSPOPENF_NO_REMAP);

		// Fix menus
		display_reset_menus(0, 1);

		// Bring listers to front
		IPC_ListCommand(&GUI->lister_list, IPC_ACTIVATE, 1, 0, 0);
		break;

	// Execute command
	case MENU_EXECUTE: {
		misc_startup("dopus_execute", MENU_EXECUTE, window, 0, 1);
	}
	break;

	// Open a new AmigaDOS Shell
	case MENU_NEWSHELL: {
		misc_startup("dopus_newshell", MENU_NEWSHELL, window, 0, 1);
	}
	break;

	// Hide
	case MENU_HIDE:
		hide_display();
		break;

	// About
	case MENU_ABOUT: {
		misc_startup("dopus_about", MENU_ABOUT, window, 0, 1);
	}
	break;

	// Help
	case MENU_HELP:
		help_show_help(HELP_MAIN, 0);
		break;

	// Key Finder
	case MENU_KEYFINDER:
		misc_startup("dopus_keyfinder", MENU_KEYFINDER, window, 0, 1);
		break;

	// Quit
	case MENU_QUIT: {
		if (environment->env->settings.general_flags & GENERALF_QUICK_QUIT)
			return 1;
		if (!(misc_startup("dopus_quit", MENU_QUIT, window, 0, 1)))
			return 1;
	}
	break;

	// New lister
	case MENU_NEW_LISTER: {
		Lister *lister;
		ULONG flags;

		// Create new lister
		if ((lister = lister_new(0)))
		{
			flags = (environment->env->lister_options & LISTEROPTF_DEVICES) ? 0 : LISTERF_DEVICES;
			if (environment->env->lister_options & LISTEROPTF_DUAL_DEFAULT)
				flags |= LISTERF_DUAL;

			IPC_Command(lister->ipc,
						LISTER_OPEN,
						flags,
						GUI->screen_pointer,
						0,
						0);
		}
	}
	break;

	// Unlock all listers
	case MENU_LISTER_UNLOCK_ALL:
		IPC_ListCommand(&GUI->lister_list, LISTER_UNLOCK, 0, 0, 0);
		break;

	// Close all listers
	case MENU_CLOSE_ALL:

		// Close all windows
		IPC_ListQuit(&GUI->lister_list, &main_ipc, 0, 0);
		break;

	// New button bank
	case MENU_NEW_BUTTONS:
	case MENU_NEW_BUTTONS_GFX:
	case MENU_NEW_BUTTONS_TEXT:
	case BUTTONEDIT_MENU_NEW: {
		misc_startup("dopus_new_button_bank", id, window, (APTR)(IPTR)(id == BUTTONEDIT_MENU_NEW), 0);
	}
	break;

	// New start menu
	case MENU_NEW_STARTMENU:
	case MENU_OPEN_STARTMENU:
		misc_startup("dopus_new_startmenu", id, window, 0, 0);
		break;

	// Open a button bank
	case MENU_OPEN_BUTTONS:
	case BUTTONEDIT_MENU_OPEN: {
		Buttons *buttons;

		// Create new button bank
		if ((buttons = buttons_new((char *)-1, 0, 0, 0, 0)))
		{
			// Open bank
			IPC_Command(buttons->ipc, BUTTONS_OPEN, (id == BUTTONEDIT_MENU_OPEN), GUI->screen_pointer, 0, 0);
		}
	}
	break;

	// Edit button banks
	case MENU_EDIT_BUTTONS: {
		misc_startup(NAME_BUTTON_EDITOR, MENU_EDIT_BUTTONS, window, 0, FALSE);
	}
	break;

	// Create icons
	case MENU_CREATE_ICONS:

		// Toggle flag
		GUI->flags ^= GUIF_SAVE_ICONS;

		// Fix menus
		display_reset_menus(0, 1);

		// Update environment settings
		env_update_settings(1);
		break;

	// File filter
	case MENU_FILTER:

		// Toggle flag
		GUI->flags ^= GUIF_FILE_FILTER;

		// Fix menus
		display_reset_menus(0, 1);

		// Update environment settings
		env_update_settings(1);
		break;

	// Default public screen
	case MENU_DEFPUBSCR:

		// Toggle flag
		GUI->flags ^= GUIF_DEFPUBSCR;

		// Fix screen
		if (GUI->screen)
			display_set_shanghai((BOOL)(GUI->flags & GUIF_DEFPUBSCR));

		// Update environment settings
		env_update_settings(1);
		break;

	// Toggle the clock
	case MENU_CLOCK: {
		unsigned short state;

		// Get desired state of clock
		if ((state = item->Flags & CHECKED))
			GUI->flags |= GUIF_CLOCK;
		else
			GUI->flags &= ~GUIF_CLOCK;

		// Refresh title bar
		if (window)
			SetWindowTitles(window, (char *)-1, (char *)GUI->screen_title);

		// Update environment settings
		env_update_settings(1);
	}
	break;

	// Environment handling
	case MENU_EDIT_ENVIRONMENT:
	case MENU_SAVE_ENVIRONMENT:
	case MENU_OPEN_ENVIRONMENT:
	case MENU_SAVE_LAYOUT: {
		env_packet *packet;

		// Check environment handler isn't already up
		if (IPC_FindProc(&GUI->process_list, "dopus_environment", TRUE, 0))
			break;

		// Allocate packet
		if ((packet = AllocVec(sizeof(env_packet), MEMF_CLEAR)))
		{
			// Fill out type
			packet->type = id;

			// Launch it
			IPC_Launch(&GUI->process_list,
					   0,
					   "dopus_environment",
					   (IPTR)&environment_proc,
					   STACK_LARGE,
					   (IPTR)packet,
					   (struct Library *)DOSBase);
		}
	}
	break;

	// Config filetypes
	case MENU_FILETYPES: {
		// Launch process to configure filetypes
		misc_startup("dopus_config_filetypes", MENU_FILETYPES, window, 0, 1);
	}
	break;

	// Edit toolbar
	case MENU_LISTER_BUTTONS:
		lister_toolbar_edit(-1);
		break;

	// Edit lister menu
	case MENU_LISTER_MENU:
		misc_startup("dopus_config_lister_menu", MENU_LISTER_MENU, window, 0, 1);
		break;

	// Edit user menu
	case MENU_MENU:
		misc_startup("dopus_config_user_menu", MENU_MENU, window, 0, 1);
		break;

	// Edit hotkeys
	case MENU_HOTKEYS:
		misc_startup("dopus_config_hotkeys", MENU_HOTKEYS, window, 0, 1);
		break;

	// Edit scripts
	case MENU_SCRIPTS:
		misc_startup("dopus_config_scripts", MENU_SCRIPTS, window, 0, 1);
		break;

		/*
				// Edit menu keys
				case MENU_MENUKEYS:
					misc_startup("dopus_config_menukeys",MENU_MENUKEYS,window,0,1);
					break;
		*/

	// Open backdrop object
	case MENU_ICON_OPEN: {
		iconopen_packet *packet;

		// Allocate packet
		if ((packet = get_icon_packet(GUI->backdrop, 0, 0, 0)))
		{
			// Start process
			if (!(misc_startup("dopus_icon_open", MENU_ICON_OPEN, window, packet, 1)))
				FreeVec(packet);
		}
	}
	break;

	// Info on backdrop objects
	case MENU_ICON_INFO:
		backdrop_info(GUI->backdrop, 0, 0);
		break;

	// Disk info
	case MENU_ICON_DISKINFO:
		backdrop_info(GUI->backdrop, 1, 0);
		break;

	// Select all
	case MENU_ICON_SELECT_ALL:
		backdrop_select_all(GUI->backdrop, 1);
		break;

	// Snapshot backdrop objects
	case MENU_ICON_SNAPSHOT_ALL:
	case MENU_ICON_SNAPSHOT_ICON:
	case MENU_ICON_UNSNAPSHOT:

		// Set busy pointer
		SetBusyPointer(GUI->window);

		// Snapshot icons
		backdrop_snapshot(GUI->backdrop, (id == MENU_ICON_UNSNAPSHOT), (id == MENU_ICON_SNAPSHOT_ALL), 0);

		// Clear busy pointer
		ClearPointer(GUI->window);
		break;

	// Put away backdrop objects
	case MENU_ICON_PUT_AWAY:

		// Set busy pointer
		SetBusyPointer(GUI->window);

		// Put icons away
		backdrop_putaway(GUI->backdrop, 0);

		// Clear busy pointer
		ClearPointer(GUI->window);
		break;

	// Clean up
	case MENU_ICON_CLEANUP:
		backdrop_cleanup(GUI->backdrop, 0, CLEANUPF_ALIGN_OK);
		break;

	// Arrange icons
	case MENU_LISTER_ARRANGE_NAME:
	case MENU_LISTER_ARRANGE_TYPE:
	case MENU_LISTER_ARRANGE_SIZE:
	case MENU_LISTER_ARRANGE_DATE:
		backdrop_cleanup(GUI->backdrop, BSORT_NAME + (id - MENU_LISTER_ARRANGE_NAME), CLEANUPF_ALIGN_OK);
		break;

	// Line up
	case MENU_ICON_LINEUP:
		backdrop_lineup_objects(GUI->backdrop);
		break;

	// Reset
	case MENU_ICON_RESET:

		// Set busy pointer
		SetBusyPointer(GUI->window);

		// Free objects
		backdrop_free_list(GUI->backdrop);

		// Show objects (clear)
		backdrop_show_objects(GUI->backdrop, BDSF_CLEAR | BDSF_RECALC);

		// Lock position list
		lock_listlock(&GUI->positions, TRUE);

		// Free existing positions
		FreePositions(&GUI->positions.list);

		// Load new positions
		GetPositions(&GUI->positions, GUI->position_memory, GUI->position_name);

		// Unlock position list
		unlock_listlock(&GUI->positions);

		// Read object lists
		backdrop_get_objects(GUI->backdrop, BGOF_ALL | BGOF_SHOW);

		// Clear busy pointer
		ClearPointer(GUI->window);
		break;

	// Format disks
	case MENU_ICON_FORMAT:
		backdrop_format(GUI->backdrop, 0);
		break;

	// Rename/delete icons
	case MENU_ICON_RENAME:
	case MENU_ICON_DELETE: {
		iconopen_packet *packet;

		// Get packet
		if ((packet = get_icon_packet(GUI->backdrop, 0, 0, 0)))
		{
			// Start process
			if (!(misc_startup(
					(id == MENU_ICON_RENAME) ? "dopus_icon_rename" : "dopus_icon_delete", id, window, packet, 1)))
				FreeVec(packet);
		}
	}
	break;

	// New group
	case MENU_GROUP_NEW:
		misc_startup("dopus_new_group", MENU_GROUP_NEW, window, GUI->backdrop, 1);
		break;

	// New command
	case MENU_COMMAND_NEW:
		misc_startup("dopus_new_command", MENU_COMMAND_NEW, window, GUI->backdrop, 1);
		break;

	// Tile/Cascade
	case MENU_TILE_HORIZ:
	case MENU_TILE_VERT:
	case MENU_CASCADE:

		// Tile listers
		lister_tile(id);
		break;

	// Show as name
	case MENU_LISTER_VIEW_NAME:
		GUI->flags &= ~(GUIF_VIEW_ICONS | GUIF_ICON_ACTION);
		display_fix_menu(GUI->window, WINDOW_BACKDROP, 0);
		env_update_settings(1);
		break;

	// Show as icon
	case MENU_LISTER_VIEW_ICON:
		GUI->flags |= GUIF_VIEW_ICONS;
		GUI->flags &= ~GUIF_ICON_ACTION;
		display_fix_menu(GUI->window, WINDOW_BACKDROP, 0);
		env_update_settings(1);
		break;

	// Icon action
	case MENU_LISTER_ICON_ACTION:
		GUI->flags |= GUIF_ICON_ACTION;
		GUI->flags &= ~GUIF_VIEW_ICONS;
		display_fix_menu(GUI->window, WINDOW_BACKDROP, 0);
		env_update_settings(1);
		break;

	// Show all
	case MENU_LISTER_SHOW_ALL:
		GUI->flags ^= GUIF_SHOW_ALL;
		display_fix_menu(GUI->window, WINDOW_BACKDROP, 0);
		env_update_settings(1);
		break;

	// Leave objects out
	case MENU_ICON_LEAVE_OUT:
		SetBusyPointer(GUI->window);
		backdrop_leave_icons_out(GUI->backdrop, 0, 0);
		ClearPointer(GUI->window);
		break;

	// Button editor save commands
	case BUTTONEDIT_MENU_SAVE:
	case BUTTONEDIT_MENU_SAVEAS:
		buttons_edit_save(id);
		break;

	// Button editor default, etc commands
	case BUTTONEDIT_MENU_DEFAULTS:
	case BUTTONEDIT_MENU_LASTSAVED:
	case BUTTONEDIT_MENU_RESTORE:
		buttons_edit_defaults(id);
		break;

	// Icon positioning
	case MENU_ICONPOS:
		misc_startup("dopus_iconpos", MENU_ICONPOS, GUI->window, GUI->iconpos_port, TRUE);
		break;

	// Theme stuff
	case MENU_THEMES_LOAD:
		function_launch_quick(FUNCTION_RUN_FUNCTION, def_function_loadtheme, 0);
		break;

	case MENU_THEMES_SAVE:
		function_launch_quick(FUNCTION_RUN_FUNCTION, def_function_savetheme, 0);
		break;

	case MENU_THEMES_BUILD:
		function_launch_quick(FUNCTION_RUN_FUNCTION, def_function_buildtheme, 0);
		break;

	// Something else; maybe a user menu?
	default:

		// Lock user menu
		GetSemaphore(&GUI->user_menu_lock, SEMF_SHARED, 0);

		// Go through menu, look for this function
		for (button = (Cfg_Button *)GUI->user_menu->buttons.lh_Head; button->node.ln_Succ;
			 button = (Cfg_Button *)button->node.ln_Succ)
		{
			Cfg_ButtonFunction *func;

			// Go through functions
			for (func = (Cfg_ButtonFunction *)button->function_list.mlh_Head; func->node.ln_Succ;
				 func = (Cfg_ButtonFunction *)func->node.ln_Succ)
			{
				// Match function
				if (func == (Cfg_ButtonFunction *)id)
				{
					struct ArgArray *array = 0;

					// Remember function
					function = (Cfg_Function *)func;

					// Desktop selection fallback: when no lister provides a
					// selection, honour a selected desktop icon (as the icon
					// context menu does)
					if (!lister_has_selection() && (array = desktop_selection_argarray(GUI->backdrop)))
					{
						function_launch(FUNCTION_RUN_FUNCTION_EXTERNAL,
										function,
										0,
										0,
										0,
										0,
										environment->env->desktop_location,
										0,
										array,
										0,
										0);
					}

					// Otherwise launch normally
					else
					{
						function_launch_quick(FUNCTION_RUN_FUNCTION, function, 0);
					}
					break;
				}
			}

			// Finished?
			if (function)
				break;
		}

		// Unlock user menu
		FreeSemaphore(&GUI->user_menu_lock);

		// Might be an AppMenuItem
		if (!function)
		{
			APTR appitem;

			// Lock menu item list
			appitem = LockAppList();

			// Search list
			while ((appitem = NextAppEntry(appitem, APP_MENU)))
			{
				// Item we want?
				if (appitem == (APTR)id)
				{
					DOpusAppMessage *msg;
					struct MsgPort *port;

					// Build AppMessage
					if ((msg = backdrop_appmessage(GUI->backdrop, 0)))
					{
						// Complete message
						msg->da_Msg.am_Type = MTYPE_APPMENUITEM;
						port =
							WB_AppWindowData((struct AppWindow *)appitem, &msg->da_Msg.am_ID, &msg->da_Msg.am_UserData);

						// Send the message
						PutMsg(port, (struct Message *)msg);
					}
					break;
				}
			}

			// Unlock list
			UnlockAppList();
		}
		break;

	// Create a directory on the desktop
	case MENU_ICON_MAKEDIR:
		function_launch(FUNCTION_RUN_FUNCTION,
						def_function_makedir,
						0,
						FUNCF_ICONS | FUNCF_RESCAN_DESKTOP,
						0,
						0,
						environment->env->desktop_location,
						0,
						0,
						0,
						0);
		break;

	// Create a file on the desktop
	case MENU_ICON_MAKEFILE:
		function_launch(FUNCTION_RUN_FUNCTION,
						def_function_makefile,
						0,
						FUNCF_ICONS | FUNCF_RESCAN_DESKTOP,
						0,
						0,
						environment->env->desktop_location,
						0,
						0,
						0,
						0);
		break;
	}

	return 0;
}

// Handle an AppIcon change
void handle_icon_notify(DOpusNotify *notify)
{
	// Adding an AppIcon?
	if (!(notify->dn_Flags & DNF_ICON_REMOVED))
	{
		AppEntry *icon = (AppEntry *)notify->dn_Data;

		// Showing AppIcons, or a local icon?
		if (environment->env->display_options & DISPOPTF_SHOW_APPICONS || icon->flags & APPENTF_LOCAL)
		{
			short flags;

			// Get flags
			flags = BDAF_SHOW | BDAF_LOCK;

			// Change?
			if (notify->dn_Flags & DNF_ICON_CHANGED)
			{
				// Change flag
				flags |= BDAF_CHANGE;
				if (notify->dn_Flags & DNF_ICON_IMAGE_CHANGED)
					flags |= BDAF_NEW_IMAGE;
			}

			// Add icon
			backdrop_add_appicon(icon, GUI->backdrop, flags);
		}
	}

	// Removing one
	else
	{
		// Remove icon
		backdrop_rem_appicon(GUI->backdrop, (AppEntry *)notify->dn_Data);
	}
}
