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

#include <proto/diskfont.h>

#include "dopus.h"

#define SNIFFF_SHOW (1 << 0)
#define SNIFFF_SLIDERS (1 << 1)

short lister_handle_sniff(Lister *, SniffData *);

static void lister_launch_special_pair(Lister *lister, Cfg_Function *func)
{
	if (!func)
		return;

	function_launch_quick(FUNCTION_RUN_FUNCTION, func, lister);
}

static short lister_proc_apply_side(Lister **lister, ULONG side, BOOL preserve_active)
{
	short old_side = -1;

	if (!lister || !*lister)
		return -1;

	if (side > 0 && side <= LISTER_DUAL_SIDES)
	{
		old_side = lister_dual_active_index(*lister);
		if (preserve_active && lister_dual_apply_side_temporary(*lister, side - 1))
			return old_side;
		lister_dual_apply_side(*lister, side - 1);
	}
	else
		*lister = lister_dual_active_side(*lister);

	return -1;
}

static void lister_proc_restore_side(Lister *lister, short side)
{
	if (side >= 0)
		lister_dual_restore_active(lister, side);
}

// Initialise a new lister
IPC_StartupCode(lister_init, Lister *, lister, static)
{
	// Store IPC and lister pointers
	lister->ipc = ipc;
	ipc->userdata = lister;

	// Store IPC pointer in backdrop info
	lister->backdrop_info->ipc = ipc;

	// Path history list
	lister->path_history = Att_NewList(LISTF_LOCK);

	// Initialise reselection
	InitReselect(&lister->reselect);
	lister->abort_signal = -1;
	lister->filter_name_bit = -1;

	// Create message ports and signals
	if (!(lister->app_port = CreateMsgPort()) || !(lister->timer_port = CreateMsgPort()) ||
		(lister->hot_name_bit = AllocSignal(-1)) == -1 || (lister->abort_signal = AllocSignal(-1)) == -1 ||
		(lister->filter_name_bit = AllocSignal(-1)) == -1)
		return 0;

	// Allocate some timers
	if (!(lister->busy_timer = AllocTimer(UNIT_VBLANK, lister->timer_port)) ||
		!(lister->scroll_timer = AllocTimer(UNIT_VBLANK, lister->timer_port)) ||
		!(lister->edit_timer = AllocTimer(UNIT_VBLANK, lister->timer_port)) ||
		!(lister->foo_timer = AllocTimer(UNIT_VBLANK, lister->timer_port)))
		return 0;
	StartTimer(lister->foo_timer, 5, 0);

	// Create regions
	if (!(lister->title_region = NewRegion()) || !(lister->refresh_extra = NewRegion()))
		return 0;

	// Lock buffer list
	lock_listlock(&GUI->buffer_list, TRUE);

	// Allocate initial buffer
	if (!(lister->cur_buffer = lister_get_empty_buffer()) && !(lister->cur_buffer = lister_new_buffer(lister)))
		return 0;
	lister->cur_buffer->buf_CurrentLister = lister;

	// Unlock buffer list
	unlock_listlock(&GUI->buffer_list);

	// Allocate "special" buffer
	if (!(lister->special_buffer = buffer_new()))
		return 0;

	// Build popup menu
	lister_build_menu(lister);

	// Initialise flags
	lister->flags |= LISTERF_FIRST_TIME;
	lister->flags2 |= LISTERF2_UNAVAILABLE;
	lister->tool_sel = -1;
	lister->tool_hover_but = -1;
	lister->toolbar_offset = 0;

	// Fix priority
	lister->normal_pri = environment->env->settings.pri_lister[0];
	lister->busy_pri = environment->env->settings.pri_lister[1];
	SetTaskPri((struct Task *)lister->ipc->proc, lister->normal_pri);

	// Get font to use
	lister->lister_font.ta_Name = lister->font_name;
	lister->lister_font.ta_YSize = lister->font_size;
	lister->lister_font.ta_Flags = 0;
	lister->lister_font.ta_Style = 0;

	// Open font
	if ((lister->font = OpenDiskFont(&lister->lister_font)))
	{
		// Proportional	font?
		if (lister->font->tf_Flags & FPF_PROPORTIONAL)
			lister->more_flags |= LISTERF_PROP_FONT;

		// Set font in text area
		InitRastPort(&lister->text_area.rast);
		SetFont(&lister->text_area.rast, lister->font);
	}

	return 1;
}

// The code that actually runs a lister
IPC_EntryCode(lister_code)
{
	Lister *lister;
	IPCMessage *lmsg;
	struct IntuiMessage *imsg;
	DOpusAppMessage *amsg;
	short pending_quit = 0;
	struct MinList sniff_list;

	// Do startup
	if (!(IPC_ProcStartup((IPTR *)&lister, (ULONG (*)(IPCData *, APTR))&lister_init)))
	{
		lister_cleanup(lister, 0);
		return;
	}

	// Increment lister count
	++main_lister_count;

	// Time for cookie?
	if (!(GUI->flags & GUIF_LISTER_COOKIE) && main_lister_count == 42)
	{
		misc_startup("6*9", LISTER_COOKIE, 0, 0, 0);
		GUI->flags |= GUIF_LISTER_COOKIE;
	}

	// Initialise list for temporary sniffs
	NewList((struct List *)&sniff_list);

	// Event loop
	FOREVER
	{
		BOOL wait_flag = 1;
		short quit_flag = 0;
		char *progress_filename = 0;
		unsigned long progress_count = 0;
#ifdef USE_64BIT
		UQUAD file_progress_count = 0LL;
#else
		unsigned long file_progress_count = 0;
#endif
		short progress_flag = 0;
		DOpusNotify *notify;

		// Got an icon drag timer?
		if (lister->icon_drag_timer)
		{
			// Has timer returned?
			if (CheckTimer(lister->icon_drag_timer))
			{
				// Check for deadlocks
				if (lister->backdrop_info->tick_count == lister->backdrop_info->last_tick)
				{
					// Stop drag
					backdrop_stop_drag(lister->backdrop_info);
				}

				// Restart timer
				StartTimer(lister->icon_drag_timer, 0, 500000);

				// Remember tick count
				lister->backdrop_info->last_tick = lister->backdrop_info->tick_count;
			}
		}

		// Busy request returned?
		if ((lister->flags & LISTERF_BUSY) && !(lister->flags & LISTERF_BUSY_VISUAL) &&
			(CheckTimer(lister->busy_timer)))
		{
			// We've been busy for long enough; update status bar
			lister->flags |= LISTERF_BUSY_VISUAL;
			if (lister_valid_window(lister))
				lister_show_status(lister);
		}

		// Periodic timer return
		if (CheckTimer(lister->foo_timer))
		{
			// If lister isn't busy, check wait list
			if (!(lister->flags & LISTERF_BUSY) && !(lister->flags & LISTERF_SNIFFING))
			{
				// Anyone waiting to find out when lister finishes?
				if (!(IsListEmpty((struct List *)&lister->wait_list)))
					lister_relieve_waiters(lister, LISTERWAIT_UNBUSY);
			}

			// Anything in the sniff list?
			if (!IsListEmpty((struct List *)&sniff_list))
			{
				// Check layers aren't locked
				if (lister->window && lister->window->WScreen->LayerInfo.LockLayersCount == 0)
				{
					SniffData *sniff;
					DirBuffer *last_buffer = 0;
					short show = 0;

					// Go through sniff list
					while ((sniff = (SniffData *)sniff_list.mlh_Head)->node.mln_Succ)
					{
						// Different buffer?
						if (sniff->buffer != last_buffer)
						{
							// Unlock previous buffer, lock this one
							if (last_buffer)
								buffer_unlock(last_buffer);
							buffer_lock(sniff->buffer, FALSE);
							last_buffer = sniff->buffer;
						}

						// Remove first entry
						Remove((struct Node *)sniff);

						// Process the sniff
						show |= lister_handle_sniff(lister, sniff);

						// Free sniff entry
						FreeVec(sniff);
					}

					// Unlock buffer
					if (last_buffer)
						buffer_unlock(last_buffer);

					// Need to redraw?
					if (show)
					{
						// Update slider and title?
						if (show & SNIFFF_SLIDERS)
						{
							lister_update_slider(lister, SLIDER_HORZ);
							lister_show_title(lister, TRUE);
						}

						// Redraw display?
						if (show & SNIFFF_SHOW)
						{
							lister->cur_buffer->buf_OldVertOffset = -1;
							lister_display_dir(lister);
						}
					}
				}

				// Restart timer, quicker to allow more frequent updates for sniffing
				StartTimer(lister->foo_timer, 1, 0);
			}

			// Sniff list is empty, so set timer for a longer delay
			else
			{
				// Restart timer
				StartTimer(lister->foo_timer, 5, 0);
			}
		}

		// Intuition messages
		if (lister->window)
		{
			while ((imsg = (struct IntuiMessage *)GetMsg(lister->window->UserPort)))
			{
				struct IntuiMessage msg_copy;
				BOOL drag_flag = 0;

				// Copy message
				msg_copy = *imsg;

				// Menu verify?
				if (imsg->Class == IDCMP_MENUVERIFY)
				{
					// Ignore if not right button, or window is inactive
					if ((imsg->Qualifier & IEQUALIFIER_RBUTTON) && (lister->window->Flags & WFLG_WINDOWACTIVE))
					{
						// Did event happen over the window?
						if (imsg->MouseX >= 0 && imsg->MouseY >= 0 && imsg->MouseX < lister->window->Width &&
							imsg->MouseY < lister->window->Height)
						{
							// Cancel menu event
							imsg->Code = MENUCANCEL;

							// Change our copy to MOUSEBUTTONS
							msg_copy.Class = IDCMP_MOUSEBUTTONS;
							msg_copy.Code = MENUDOWN;

							// Kludge for MagicMenu
							if (msg_copy.Seconds == 0)
								CurrentTime(&msg_copy.Seconds, &msg_copy.Micros);
						}
					}
				}

				// Reply to message (unless its a refresh or change message)
				if (imsg->Class != IDCMP_REFRESHWINDOW && imsg->Class != IDCMP_CHANGEWINDOW &&
					imsg->Class != IDCMP_NEWSIZE)
				{
					ReplyMsg((struct Message *)imsg);
					imsg = 0;
				}

				// Store timestamp
				lister->seconds = msg_copy.Seconds;
				lister->micros = msg_copy.Micros;

				// Are we dragging an icon?
				if ((lister->flags & LISTERF_VIEW_ICONS) && (lister->backdrop_info->flags & BDIF_DRAGGING))
					drag_flag = 1;

				// Clear 'no active' flag on button click, a bit kludgy
				if (msg_copy.Class == IDCMP_MOUSEBUTTONS)
					lister->more_flags &= ~LISTERF_NO_ACTIVE;

				// Refresh/resize messages get handled differently
				if (msg_copy.Class == IDCMP_REFRESHWINDOW || msg_copy.Class == IDCMP_CHANGEWINDOW ||
					msg_copy.Class == IDCMP_NEWSIZE)
				{
					// Pass to callback refresh function
					lister_refresh_callback(msg_copy.Class, 0, lister);
				}

				// If in icon view, see if backdrop is interested in message, otherwise handle it ourselves
				else if (!(lister->flags & LISTERF_VIEW_ICONS) ||
						 !(backdrop_idcmp(lister->backdrop_info,
										  &msg_copy,
										  (lister->flags & LISTERF_ICON_ACTION) ? BIDCMPF_LISTER : 0)))
				{
					// Handle lister message
					lister_process_msg(lister, &msg_copy);
				}

				// Reply to message if not already replied
				if (imsg)
					ReplyMsg((struct Message *)imsg);

				// Dragging something now?
				if ((lister->flags & LISTERF_VIEW_ICONS) && (lister->backdrop_info->flags & BDIF_DRAGGING))
				{
					// If we weren't before, start timer
					if (!drag_flag)
					{
						lister->icon_drag_timer = AllocTimer(UNIT_VBLANK, lister->timer_port);
						StartTimer(lister->icon_drag_timer, 1, 0);
					}
				}

				// Otherwise, were we?
				else if (drag_flag)
				{
					// Abort timer
					FreeTimer(lister->icon_drag_timer);
					lister->icon_drag_timer = 0;
				}

				// Window closed?
				if (!lister->window)
					break;

				// Deferred title update?
				if (lister->flags & LISTERF_TITLE_DEFERRED)
					lister_set_title(lister);
			}
		}

		// Application messages
		if ((amsg = (DOpusAppMessage *)GetMsg(lister->app_port)))
		{
			wait_flag = 0;

			// Files dropped?
			if ((lister->window || lister->appicon) && amsg->da_Msg.am_NumArgs > 0 && !(lister->flags & LISTERF_LOCK))
			{
				// Receive the drop
				lister_receive_drop(lister, amsg);
			}

			// Menu operation?
			else if (amsg->da_Msg.am_Type == MTYPE_APPSNAPSHOT)
			{
				struct AppSnapshotMsg *asma;

				// Get SnapShot message pointer
				asma = (struct AppSnapshotMsg *)amsg;

				// Menu operation?
				if (asma->flags & APPSNAPF_MENU)
				{
					// Close lister?
					if (asma->id == LISTERPOPUP_CLOSE)
					{
						// Help?
						if (asma->flags & APPSNAPF_HELP)
						{
							// Send help
							help_menu_help(MENU_LISTER_CLOSE, 0);
						}

						// Send ourselves a quit command
						else
							IPC_Command(lister->ipc, IPC_QUIT, 0, 0, 0, 0);
					}
				}
			}

			// Or, appicon action?
			else if (amsg->da_Msg.am_Type == MTYPE_APPICON)
			{
				// Double-click?
				if (amsg->da_Msg.am_NumArgs == 0)
				{
					// Clear iconified flag
					lister->flags &= ~LISTERF_ICONIFIED;

					// Open lister
					lister_open(lister, GUI->screen_pointer);

					// Check source/dest
					lister_smart_source(lister);
				}
			}

			// Reply the message
			ReplyAppMessage(amsg);
		}

		// Icon notification
		if (lister->backdrop_info->notify_req)
		{
			// Get notify message
			if ((notify = (DOpusNotify *)GetMsg(lister->backdrop_info->notify_port)))
			{
				// If in icon view mode, see if it was for us
				if (lister->flags & LISTERF_VIEW_ICONS)
				{
					// Was it for us?
					if (backdrop_check_notify(lister->backdrop_info, notify, lister))
					{
						// It was; update our datestamp
						update_buffer_stamp(lister);
					}
				}

				// Otherwise just free it
				else
					ReplyFreeMsg(notify);
			}
		}

		// Commands
		if (!lister->down_gadget)
		{
			while ((lmsg = (IPCMessage *)GetMsg(lister->ipc->command_port)) && !quit_flag)
			{
				IPTR flags;
				APTR data;

				// Cache stuff
				flags = lmsg->flags;
				data = lmsg->data;
				wait_flag = 0;

				// Look at command
				switch (lmsg->command)
				{
				// Set priority
				case IPC_PRIORITY:

					// Store priorities
					lister->normal_pri = (short)(IPTR)lmsg->data;
					lister->busy_pri = (short)lmsg->flags;

					// Fix priority
					lister_fix_priority(lister);
					break;

				// Initialise lister
				case LISTER_INIT:

					// Is lister already open?
					if (lister->window || lister->flags & LISTERF_ICONIFIED)
					{
						// Move in front of main window (unless main window is backdrop)
						if (lister->window && GUI->window && !GUI->backdrop)
							MoveWindowInFrontOf(lister->window, GUI->window);
						break;
					}

					// Lister not busy?
					if (!(lister->flags & LISTERF_BUSY))
					{
						Cfg_Lister *cfg = 0;

						// Is this our first initialisation?
						if (lister->flags & LISTERF_FIRST_TIME)
						{
							// Configuration lister?
							if (lister->lister)
								cfg = lister->lister;
							else if (lmsg->data_free)
								cfg = (Cfg_Lister *)lmsg->data_free;
						}

						// Valid configuration?
						if (cfg)
						{
							BOOL open = 1;

							// Get position and list format
							lister->dimensions.wd_Normal = cfg->lister.pos[0];
							lister->format = cfg->lister.format;

							// Is lister locked?
							if (cfg->lister.flags & DLSTF_LOCK_POS)
								lister->flags |= LISTERF_LOCK_POS;

							// Lister is iconified?
							if (cfg->lister.flags & DLSTF_ICONIFIED)
							{
								// Try to iconify
								if (lister_iconify(lister))
									open = 0;
							}

							// Or invisible?
							else if (cfg->lister.flags & DLSTF_INVISIBLE)
							{
								// Don't open
								open = 0;
							}

							// View as icons?
							if (cfg->lister.flags & DLSTF_ICON)
							{
								lister->flags |= LISTERF_VIEW_ICONS;
								if (cfg->lister.flags & DLSTF_ICON_ACTION)
									lister->flags |= LISTERF_ICON_ACTION;
								if (cfg->lister.flags & DLSTF_SHOW_ALL)
									lister->flags |= LISTERF_SHOW_ALL;
							}

							// No activate?
							if (cfg->lister.flags & DLSTF_NOACTIVE)
								lister->more_flags |= LISTERF_NO_ACTIVE;

							// Need to open?
							if (open && data)
							{
								// Dual lister default?
								if (flags & LISTERF_DUAL)
									lister_dual_enter(lister);

								// Open window
								lister_open(lister, (struct Screen *)data);

								// Activate if first open
								if (lister->flags & LISTERF_FIRST_TIME && lister->window &&
									!(lister->more_flags & LISTERF_NO_ACTIVE))
									ActivateWindow(lister->window);
							}

							// Make source?
							if (cfg->lister.flags & DLSTF_SOURCE)
							{
								lister_do_function(
									lister,
									(cfg->lister.flags & DLSTF_LOCKED) ? MENU_LISTER_LOCK_SOURCE : MENU_LISTER_SOURCE);
							}
							else if (flags & LISTERF_MAKE_SOURCE)
								lister_do_function(lister, MENU_LISTER_SOURCE);

							// Make dest?
							else if (cfg->lister.flags & DLSTF_DEST)
							{
								lister_do_function(
									lister,
									(cfg->lister.flags & DLSTF_LOCKED) ? MENU_LISTER_LOCK_DEST : MENU_LISTER_DEST);
							}

							// If this was the first time, do source/dest check
							else if (lister->flags & LISTERF_FIRST_TIME)
								lister_smart_source(lister);

							// Activate?
							if (flags & LISTERF_ACTIVATE && lister->window && !(lister->more_flags & LISTERF_NO_ACTIVE))
								ActivateWindow(lister->window);

							// Do we need to read a path?
							if (cfg->path && !(cfg->lister.flags & (DLSTF_DEVICE_LIST | DLSTF_CACHE_LIST)))
							{
								read_directory(lister, cfg->path, GETDIRF_CANCHECKBUFS);
							}

							// Not reading a directory, so we can clear the 'unavailable' flag
							else
							{
								lister->flags2 &= ~LISTERF2_UNAVAILABLE;
							}

							// Device list?
							if (cfg->lister.flags & DLSTF_DEVICE_LIST)
							{
								Cfg_Function *func;

								// Get function pointer
								if (cfg->lister.flags & DLSTF_DEV_FULL)
									func = def_function_devicelist_full;
								else if (cfg->lister.flags & DLSTF_DEV_BRIEF)
									func = def_function_devicelist_brief;
								else
									func = def_function_devicelist;

								// Dual lister default?
								if (flags & LISTERF_DUAL)
									lister_dual_enter(lister);

								// Launch function
								lister_launch_special_pair(lister, func);
							}

							// Cache list?
							else if (cfg->lister.flags & DLSTF_CACHE_LIST)
							{
								// Dual lister default?
								if (flags & LISTERF_DUAL)
									lister_dual_enter(lister);

								lister_launch_special_pair(lister, def_function_cachelist);
							}
							else
							{
								// Dual lister default?
								if (flags & LISTERF_DUAL)
									lister_dual_enter(lister);
							}

							// Clear the 'first time' flag
							lister->flags &= ~LISTERF_FIRST_TIME;
							break;
						}
					}

					// Fall through to open

				// Open lister
				case IPC_SHOW:
				case LISTER_OPEN:

					// Dual lister default?
					if (flags & LISTERF_DUAL)
						lister_dual_enter(lister);

					// Open lister
					lister_open(lister, (struct Screen *)data);

					// If open for the first time, activate window
					if (lister->flags & LISTERF_FIRST_TIME)
					{
						if (lister->window && !(lister->more_flags & LISTERF_NO_ACTIVE))
							ActivateWindow(lister->window);
						lister->flags &= ~LISTERF_FIRST_TIME;
						lister->flags2 &= ~LISTERF2_UNAVAILABLE;

						// Read device list?
						if (flags & LISTERF_DEVICES)
						{
							// Dual lister default?
							if (flags & LISTERF_DUAL)
								lister_dual_enter(lister);

							lister_launch_special_pair(lister, def_function_devicelist);
						}

						// Otherwise, check source/dest
						else
						{
							lister_smart_source(lister);

							// Dual lister default?
							if (flags & LISTERF_DUAL)
								lister_dual_enter(lister);
						}
					}
					break;

				// Close lister
				case IPC_HIDE:
				case LISTER_CLOSE:

					// Is window open?
					if (lister->window)
					{
						// Close window
						lister_close(lister, 0);
					}
					break;

				// Iconify lister
				case LISTER_ICONIFY:
					lister_do_function(lister, MENU_LISTER_ICONIFY);
					break;

				// Uniconify lister
				case LISTER_UNICONIFY:

					// Clear iconified flag
					lister->flags &= ~LISTERF_ICONIFIED;

					// Open lister
					lister_open(lister, GUI->screen_pointer);

					// Check source/dest
					lister_smart_source(lister);
					break;

				// Reset (menus)
				case IPC_RESET:

					// Gotta window?
					if (lister->window)
					{
						// Reset menus?
						if (flags)
						{
							display_free_menu(lister->window);
							display_get_menu(lister->window);
						}

						// Fix menus
						display_fix_menu(lister->window,
										 (lister->flags & LISTERF_VIEW_ICONS) ? WINDOW_LISTER_ICONS : WINDOW_LISTER,
										 lister);
						lister_fix_menus(lister, 0);
					}
					break;

				// Quit
				case IPC_QUIT:

					// Can we quit immediately?
					if (!(lister->flags & LISTERF_BUSY))
					{
						lister->flags |= LISTERF_BUSY;
						quit_flag = 1;
					}

					// Have to defer quit
					else
					{
						lister_send_abort(lister);
						pending_quit = 1;
					}
					break;

				// Activate
				case IPC_ACTIVATE:

					// Window open?
					if (lister->window)
					{
						// Move in front of?
						if (flags)
						{
							if (GUI->window && !GUI->backdrop)
								MoveWindowInFrontOf(lister->window, GUI->window);
							break;
						}

						// Activate window
						ActivateWindow(lister->window);
						lister_dual_to_front(lister);
					}

					// Uniconify/Make visible?
					else if (data)
					{
						// Clear iconified flag
						lister->flags &= ~LISTERF_ICONIFIED;

						// Open lister
						lister_open(lister, GUI->screen_pointer);
					}
					break;

				// Make a lister the source
				case LISTER_MAKE_SOURCE:
					lister_do_function(lister, (flags) ? MENU_LISTER_LOCK_SOURCE : MENU_LISTER_SOURCE);
					if (lister->window && !(lister->more_flags & LISTERF_NO_ACTIVE))
						ActivateWindow(lister->window);
					break;

				// Make a lister the destination
				case LISTER_MAKE_DEST:
					lister_do_function(lister, (flags) ? MENU_LISTER_LOCK_DEST : MENU_LISTER_DEST);
					break;

				// Turn a lister off
				case LISTER_OFF:
					lister_do_function(lister, MENU_LISTER_OFF);
					break;

				// Unlock a lister
				case LISTER_UNLOCK:
					if (!(lister->flags & LISTERF_BUSY))
						lister_do_function(lister, MENU_LISTER_UNLOCK);
					break;

				// Wait for busy status to change
				case LISTER_WAIT_BUSY:

					// Add message to waiting list
					if (lister_new_waiter(
							lister, lmsg, (struct Message *)flags, (data) ? LISTERWAIT_BUSY : LISTERWAIT_UNBUSY))
						lmsg = 0;
					flags = 0;

				// Change busy status
				case LISTER_BUSY:

					// End a direct edit
					lister_end_edit(lister, 0);

					// Make busy
					if (data)
					{
						// Is lister already busy?
						if (lister->flags & LISTERF_BUSY)
						{
							if (lmsg)
								lmsg->command = IPC_ABORT;
						}
						else
							lister_busy(lister, flags);

						// Anyone waiting to find out about this?
						if (!(IsListEmpty((struct List *)&lister->wait_list)))
						{
							lister_relieve_waiters(lister, LISTERWAIT_BUSY);
						}
					}

					// Make unbusy
					else
					{
						// Does lister have a locker?
						if (lister->locker_ipc)
						{
							// Send safe command
							IPC_SafeCommand(lister->locker_ipc, IPC_REMOVE, 0, lister, 0, 0, &GUI->function_list);
						}

						// Make un-busy
						lister_unbusy(lister, flags);

						// Pending flags?
						if (!(lister->flags & LISTERF_BUSY))
						{
							// Pending quit
							if (pending_quit)
							{
								quit_flag = 1;
								lister->flags |= LISTERF_BUSY;
							}

							// Pending rescan
							else if (lister->flags & LISTERF_RESCAN)
							{
								lister->flags &= ~LISTERF_RESCAN;
								read_directory(lister, lister->cur_buffer->buf_Path, GETDIRF_RESELECT);
							}
						}

						// Anyone waiting to find out about this?
						if (!(IsListEmpty((struct List *)&lister->wait_list)) && !(lister->flags & LISTERF_SNIFFING))
						{
							lister_relieve_waiters(lister, LISTERWAIT_UNBUSY);
						}
					}
					break;

				// Check if currently showing a special buffer
				case LISTER_CHECK_SPECIAL_BUFFER:
					if (flags > 0 && flags <= LISTER_DUAL_SIDES)
						lister_dual_apply_side(lister, flags - 1);
					else
						lister = lister_dual_active_side(lister);
					lmsg->command = check_special_buffer(lister, 1);
					lister_dual_store_active_buffer(lister);
					break;

				// Show special buffer
				case LISTER_SHOW_SPECIAL_BUFFER: {
					char *title = (char *)data;

					lister = lister_dual_active_side(lister);
					if (!title && flags)
						title = GetString(&locale, flags);
					buffer_show_special(lister, title);
					lister_dual_store_active_buffer(lister);
					break;
				}

				// Get current path
				case LISTER_GET_PATH:
					lister = lister_dual_active_side(lister);
					strcpy((char *)data, lister->cur_buffer->buf_Path);
					break;

				// Searches for a named buffer
				case LISTER_BUFFER_FIND: {
					ListerBufferFindData *find = (ListerBufferFindData *)data;

					if (find && lmsg->data_free == find)
					{
						if (find->side > 0 && find->side <= LISTER_DUAL_SIDES)
							lister_dual_apply_side(lister, find->side - 1);
						else
							lister = lister_dual_active_side(lister);
						lmsg->command = (IPTR)lister_find_buffer(lister,
																 0,
																 find->path,
																 find->stamp,
																 find->name,
																 LISTER_BFPF_DONT_MOVE | LISTER_BFPF_DONT_UNLOCK);
					}
					else
					{
						lister = lister_dual_active_side(lister);
						lmsg->command = (IPTR)lister_find_buffer(lister,
																 0,
																 data,
																 (struct DateStamp *)flags,
																 (char *)lmsg->data_free,
																 LISTER_BFPF_DONT_MOVE | LISTER_BFPF_DONT_UNLOCK);
					}
					break;
				}

				// Show a newly found buffer
				case LISTER_SHOW_BUFFER: {
					ULONG side;

					side = LISTER_SHOW_BUFFER_SIDE(flags);
					flags = LISTER_SHOW_BUFFER_BASE_FLAGS(flags);
					if (side > 0 && side <= LISTER_DUAL_SIDES)
						lister_dual_apply_side(lister, side - 1);
					else
						lister = lister_dual_active_side(lister);

					// If in icon mode, clear window
					if (lister->flags & LISTERF_VIEW_ICONS)
					{
						// Redraw window
						backdrop_show_objects(lister->backdrop_info, BDSF_CLEAR | BDSF_CLEAR_ONLY);
					}

					// Show buffer and check for reread
					lister_show_buffer(lister, (DirBuffer *)data, 1, 1);
					lister_dual_store_active_buffer(lister);
					lister_check_old_buffer(lister, 0);

					// Unlock buffer list?
					if (flags)
						unlock_listlock(&GUI->buffer_list);

					// Change directory
					lister_fix_cd(lister);

					// Check fuel gauge
					lister_set_gauge(lister, TRUE);
					break;
				}

				// Finds an empty buffer
				case LISTER_BUFFER_FIND_EMPTY: {
					ListerBufferFindData *find = (ListerBufferFindData *)data;

					if (find && lmsg->data_free == find)
					{
						if (find->side > 0 && find->side <= LISTER_DUAL_SIDES)
							lister_dual_apply_side(lister, find->side - 1);
						else
							lister = lister_dual_active_side(lister);
						lmsg->command = (IPTR)lister_buffer_find_empty(lister, find->path, find->stamp);
					}
					else
					{
						lister = lister_dual_active_side(lister);
						lmsg->command = (IPTR)lister_buffer_find_empty(lister, data, (struct DateStamp *)flags);
					}
					lister_dual_store_active_buffer(lister);
					break;
				}

				// Find cached buffer
				case LISTER_FIND_CACHED_BUFFER:
					lister = lister_dual_active_side(lister);
					lmsg->command = (IPTR)lister_find_cached_buffer(lister, (char *)data, (char *)flags);
					break;

				// Refresh path field
				case LISTER_REFRESH_PATH: {
					short restore_side;

					restore_side = lister_proc_apply_side(&lister, flags, TRUE);

					// Refresh path field
					lister_update_pathfield(lister);

					// Update title bar
					lister_show_name(lister);
					lister_proc_restore_side(lister, restore_side);
					break;
				}

				// Refresh free space
				case LISTER_REFRESH_FREE:
				case LISTER_REFRESH_NAME: {
					short restore_side;

					restore_side = lister_proc_apply_side(&lister, flags, TRUE);
					lister_update_name(lister);
					lister_proc_restore_side(lister, restore_side);
					break;
				}

				// Refresh lister display
				case LISTER_REFRESH_WINDOW: {
					short restore_side;

					restore_side = lister_proc_apply_side(&lister, (IPTR)data, TRUE);

					// Datestamp?
					if (flags & REFRESHF_DATESTAMP)
					{
						// Update our datestamp
						update_buffer_stamp(lister);

						// Only refresh date?
						if (flags == REFRESHF_DATESTAMP)
						{
							lister_proc_restore_side(lister, restore_side);
							break;
						}
					}

					// Window open?
					if (lister->window)
					{
						lister->flags &= ~LISTERF_NO_REFRESH;
						lister_refresh_display(lister, flags);
					}

					// Change directory?
					if (flags & REFRESHF_CD)
						lister_fix_cd(lister);
					lister_proc_restore_side(lister, restore_side);
					break;
				}

				// Refresh lister title
				case LISTER_REFRESH_TITLE:
					lister = lister_dual_active_side(lister);
					if (lister->window)
						lister_refresh_name(lister);
					break;

				// Refresh lister sliders
				case LISTER_REFRESH_SLIDERS:
					lister = lister_dual_active_side(lister);
					if (lister->window)
					{
						lister_update_slider(lister, (int)(IPTR)data);
						if (((IPTR)data) & SLIDER_VERT_DISPLAY)
							lister_pos_slider(lister, SLIDER_VERT);
						if (((IPTR)data) & SLIDER_HORZ_DISPLAY)
							lister_pos_slider(lister, SLIDER_HORZ);
					}
					break;

				// Make reselection list
				case LISTER_MAKE_RESELECT: {
					ListerReselectData *select = (ListerReselectData *)data;

					if (select && lmsg->data_free == select)
					{
						if (select->side > 0 && select->side <= LISTER_DUAL_SIDES)
							lister_dual_apply_side(lister, select->side - 1);
						else
							lister = lister_dual_active_side(lister);
						MakeReselect(select->reselect, lister->cur_buffer, select->flags);
					}
					else
					{
						lister = lister_dual_active_side(lister);
						MakeReselect((ReselectionData *)data, lister->cur_buffer, flags);
					}
					break;
				}

				// Do reselection list
				case LISTER_DO_RESELECT: {
					ListerReselectData *select = (ListerReselectData *)data;

					if (select && lmsg->data_free == select)
					{
						if (select->side > 0 && select->side <= LISTER_DUAL_SIDES)
							lister_dual_apply_side(lister, select->side - 1);
						else
							lister = lister_dual_active_side(lister);
						buffer_lock(lister->cur_buffer, FALSE);
						DoReselect(select->reselect, lister, select->flags);
						buffer_unlock(lister->cur_buffer);
					}
					else
					{
						lister = lister_dual_active_side(lister);
						buffer_lock(lister->cur_buffer, FALSE);
						DoReselect((ReselectionData *)data, lister, flags);
						buffer_unlock(lister->cur_buffer);
					}
					break;
				}

				// Scroll to show first selected entry
				case LISTER_FIND_FIRST_SEL:
					lister = lister_dual_active_side(lister);
					lister_show_selected(lister, (int)(IPTR)data);
					break;

				// Scroll to show an entry
				case LISTER_FIND_ENTRY:
					lister = lister_dual_active_side(lister);
					lister_show_entry(lister, (DirEntry *)data);
					break;

				// Do parent/root
				case LISTER_DO_PARENT_ROOT:
					lister = lister_dual_active_side(lister);
					do_parent_root(lister, (char *)data);
					break;

				// Show selection information
				case LISTER_SHOW_INFO: {
					short restore_side;

					restore_side = lister_proc_apply_side(&lister, flags, TRUE);
					select_show_info(lister, 0);
					lister_proc_restore_side(lister, restore_side);
					break;
				}

				// Remove directory sizes
				case LISTER_REMOVE_SIZES:
					lister = lister_dual_active_side(lister);
					remove_dir_sizes(lister);
					break;

				// Set locker port
				case LISTER_SET_LOCKER:
					lister->locker_ipc = data;

					// If we have a locker, make its priority match ours
					if (lister->locker_ipc)
						SetTaskPri((struct Task *)lister->locker_ipc->proc, lister->ipc->proc->pr_Task.tc_Node.ln_Pri);
					break;

				// Select global state
				case LISTER_SELECT_GLOBAL_STATE:
					if ((IPTR)data > 0 && (IPTR)data <= LISTER_DUAL_SIDES)
						lister_dual_apply_side(lister, (IPTR)data - 1);
					else
						lister = lister_dual_active_side(lister);
					select_global_state(lister, flags);
					break;

				// Toggle global state
				case LISTER_SELECT_GLOBAL_TOGGLE:
					if (flags > 0 && flags <= LISTER_DUAL_SIDES)
						lister_dual_apply_side(lister, flags - 1);
					else
						lister = lister_dual_active_side(lister);
					select_global_toggle(lister);
					break;

				// Wildcard selection
				case LISTER_SELECT_WILD: {
					SelectWildData *select = (SelectWildData *)data;

					if (select && lmsg->data_free == select)
					{
						if (select->side > 0 && select->side <= LISTER_DUAL_SIDES)
							lister_dual_apply_side(lister, select->side - 1);
						else
							lister = lister_dual_active_side(lister);
						select_global_wild(lister, &select->data, select->dest_list);
					}
					else
					{
						lister = lister_dual_active_side(lister);
						select_global_wild(lister, (SelectData *)data, (PathList *)flags);
					}
					break;
				}

				// Show status text
				case LISTER_STATUS:
					if (lmsg->data_free)
						lister_status(lister, (char *)lmsg->data_free);
					else
						lister_status(lister, (char *)data);
					break;

				// Progress indicator on
				case LISTER_PROGRESS_ON:

					// Open progress requester
					lister_progress_on(lister, (ProgressPacket *)lmsg->data_free);
					break;

				// Progress indicator off
				case LISTER_PROGRESS_OFF:
					lister_progress_off(lister);
					if (flags)
						lister->flags |= LISTERF_NO_REFRESH;
					break;

				// Progress count
				case LISTER_PROGRESS_COUNT:

					// Remember count
					progress_count = flags;
					progress_flag |= PWF_GRAPH;
					break;

				// Progress indicator update
				case LISTER_PROGRESS_UPDATE:

					// Have we already had some update data?
					if (progress_flag & PWF_FILENAME)
					{
						// Is there a new name supplied?
						if (data)
						{
							// Throw old name away
							FreeMemH(progress_filename);
							progress_filename = 0;
						}
					}

					// Remember name
					if (data && !progress_filename)
					{
						progress_filename = data;
						progress_flag |= PWF_FILENAME;
					}

					// File count given?
					if (flags > 0)
					{
						progress_count = flags;
						progress_flag |= PWF_GRAPH;
					}
					break;

				// Set total progress
				case LISTER_PROGRESS_TOTAL:
					lister_progress_total(lister, flags, (long)data);
					break;

				// Update file total
				case LISTER_FILE_PROGRESS_TOTAL:
					lister_progress_filetotal(lister, (long)data);
					break;

				case LISTER_FILE_PROGRESS_TOTAL64:
					lister_progress_filetotal64(lister, (QUAD *)data);
					break;

				// Set file progress
				case LISTER_FILE_PROGRESS_SET:
					lister_progress_file(lister, flags, (long)data);
					break;

				// Update file progress
				case LISTER_FILE_PROGRESS_UPDATE:

					// Remember count
					file_progress_count = (unsigned long)data;
					progress_flag |= PWF_FILESIZE;
					break;

				// Update 64bit file progress
				case LISTER_FILE_PROGRESS_UPDATE64:

					// Remember count
					file_progress_count = *(UQUAD *)data;
					progress_flag |= PWF_FILESIZE;
					break;

				// Update file progress info
				case LISTER_FILE_PROGRESS_INFO:

					// Set direct
					SetProgressWindowTags(
						lister->progress_window,
						(flags == RXPROG_INFO2) ? PW_Info2 : (flags == RXPROG_INFO3) ? PW_Info3 : PW_Info,
						(IPTR)data,
						TAG_END);

					/*
											lister_progress_info(
												lister,
												(char *)data);
					*/
					break;

				// Update progress title
				case LISTER_PROGRESS_TITLE:
					lister_progress_title(lister, (char *)data);
					break;

				// Update buffer stamp
				case LISTER_UPDATE_STAMP: {
					short restore_side;

					restore_side = lister_proc_apply_side(&lister, flags, TRUE);

					// If there's no pending rescan
					if (!(lister->flags & LISTERF_RESCAN))
						update_buffer_stamp(lister);
					lister_proc_restore_side(lister, restore_side);
					break;
				}

				// Copy a buffer
				case LISTER_COPY_BUFFER:
					lister = lister_dual_active_side(lister);

					// Only if window's not busy
					if (!(lister->flags & LISTERF_LOCK))
					{
						// Lock lister
						lister_busy(lister, 1);

						// Copy buffer
						buffer_copy((DirBuffer *)data, lister->cur_buffer, lister);

						// Unlock lister
						lister_unbusy(lister, 1);
					}
					break;

				// Rescan directory
				case LISTER_RESCAN:
					if (flags > 0 && flags <= LISTER_DUAL_SIDES)
						lister_dual_apply_side(lister, flags - 1);
					else
						lister = lister_dual_active_side(lister);

					// If not busy, reread immediately
					if (!(lister->flags & LISTERF_LOCK))
						read_directory(lister, lister->cur_buffer->buf_Path, GETDIRF_RESELECT);

					// Otherwise, set flag to queue rescan
					else
						lister->flags |= LISTERF_RESCAN;
					break;

				// Split display with another lister
				case LISTER_SPLIT:
					lister_split_display(lister, (Lister *)data);
					break;

				// Wait for lister to finish
				case LISTER_WAIT:

					// Quick wait, and lister isn't busy?
					if (!(lister->flags & LISTERF_BUSY) && flags)
					{
						// Signal failure
						lmsg->command = 0;
						break;
					}

					// Add message to waiting list
					lister_new_waiter(lister, 0, (struct Message *)data, LISTERWAIT_UNBUSY);

					// Restart timer for 2 seconds
					StartTimer(lister->foo_timer, 2, 0);
					lmsg->command = 2;
					break;

				// Filetype sniff
				case LISTER_FILETYPE_SNIFF:

					// Add message to sniff list
					AddTail((struct List *)&sniff_list, (struct Node *)lmsg->data_free);
					lmsg->data_free = 0;
					break;

				// Get icons
				case LISTER_GET_ICONS:

					// Viewing icons?
					if (lister->flags & LISTERF_VIEW_ICONS)
					{
						// Clear display if requested
						if (flags)
							backdrop_show_objects(lister->backdrop_info, BDSF_CLEAR_ONLY | BDSF_RECALC);

						// Read icons
						function_launch_quick(FUNCTION_GET_ICONS, 0, lister);
					}
					break;

				// Get a specific icon
				case LISTER_GET_ICON:

					// Get icon
					lister_get_icons(0, lister, (char *)data, 0);
					break;

				// Get lister handle
				case LISTER_GET_HANDLE:
					lmsg->command = (IPTR)lister;
					break;

				// Show help on something
				case IPC_HELP: {
					short x, y;

					// Get coordinates
					x = ((IPTR)data) >> 16;
					y = ((IPTR)data) & 0xffff;

					// Is point over toolbar?
					if (lister->flags & LISTERF_TOOLBAR && point_in_element(&lister->toolbar_area, x, y))
					{
						Cfg_Button *button;

						// Get button
						if ((button = lister_get_toolbar_button(lister, x, y, 0)) == (Cfg_Button *)-1)
						{
							// Arrow button
							help_show_help(HELP_ARROW_BUTTON, 0);
						}

						// Show help for button
						else if (button)
							help_button_help(button, 0, flags, 0, GENERIC_TOOLBAR_BUTTON);
					}

					// Or over a dual-panel header?
					else if (lister_dual_help_at(lister, x, y))
					{
					}

					// Or over the menu popup?
					else if (point_in_element(&lister->command_area, x, y))
					{
						help_show_help(HELP_LISTER_MENU_POPUP, 0);
					}

					// Or over the path popup?
					else if (point_in_element(&lister->parent_area, x, y))
					{
						help_show_help(HELP_LISTER_PATH_POPUP, 0);
					}

					// Or over the status popup
					else if (point_in_element(&lister->status_area, x, y))
					{
						help_show_help(HELP_LISTER_STATUS_POPUP, 0);
					}

					// Or over the file count
					else if (point_in_element(&lister->name_area, x, y))
					{
						help_show_help(HELP_LISTER_FILE_COUNT, 0);
					}

					// Generic lister help
					else
						help_show_help(HELP_LISTER_HELP, 0);
				}
				break;

				// Backfill pattern has changed
				case LISTER_BACKFILL_CHANGE:

					// No window?
					if (!lister->window)
						break;

					// Install appropriate hook
					InstallLayerHook(lister->window->WLayer, (flags) ? &lister->pattern.hook : LAYERS_BACKFILL);

					// Are we using a pattern?
					if (!lister->pattern.disabled)
					{
						// Redraw window
						erase_window(lister->window);

						// Refresh window
						lister_refresh(lister, LREFRESH_FULL);
					}
					break;

				// Change mode
				case LISTER_MODE:

					// Is lister busy?
					if (lister->flags & LISTERF_BUSY)
					{
						// Add message to waiters
						lister_new_waiter(lister, lmsg, 0, LISTERWAIT_AUTO_CMD);
						lmsg = 0;
						break;
					}

					// Icon mode?
					if (flags & LISTERF_VIEW_ICONS)
					{
						// Get flags
						lister->flags &= ~LISTERF_SHOW_ALL;
						lister->flags |= flags & ~(LISTERF_ICON_ACTION | LISTERF_VIEW_ICONS);

						// Switch to icon mode
						lister_do_function(
							lister, (flags & LISTERF_ICON_ACTION) ? MENU_LISTER_ICON_ACTION : MENU_LISTER_VIEW_ICON);
					}

					// Name mode
					else
						lister_do_function(lister, MENU_LISTER_VIEW_NAME);
					break;

				// New toolbar
				case LISTER_TOOLBAR:
					lister_new_toolbar(lister, (char *)flags, (ToolBarInfo *)data);
					break;

				// Toggle dual lister mode
				case LISTER_DUAL:
					lister_dual_toggle(lister, flags);
					break;

				// Do a function
				case LISTER_DO_FUNCTION:
					lister_do_function(lister, (IPTR)data);
					break;

				// Check for refresh
				case LISTER_CHECK_REFRESH:
					if ((imsg = check_refresh_msg(lister->window, IDCMP_REFRESHWINDOW)))
					{
						lister_process_msg(lister, imsg);
						ReplyMsg((struct Message *)imsg);
					}
					break;

				// Change a device name
				case LISTER_UPDATE_DEVICE:
					backdrop_update_disk(lister->backdrop_info,
										 (devname_change *)lmsg->data_free,
										 (lister->flags & LISTERF_VIEW_ICONS) ? TRUE : FALSE);
					break;

				// Show an icon
				case LISTER_SHOW_ICON:
					lister_show_icon(lister, (BackdropObject *)lmsg->data);
					break;

				// New font for icons
				case GROUP_NEW_FONT:

					// In icon mode?
					if (lister->window && lister->flags & LISTERF_VIEW_ICONS)
					{
						// Get new font
						backdrop_get_font(lister->backdrop_info);

						// Redraw icons
						backdrop_show_objects(lister->backdrop_info, BDSF_CLEAR | BDSF_RESET);
					}
					break;

				// Control sniffing flag
				case LISTER_SET_SNIFF:

					// Sniffing on?
					if (lmsg->flags)
						lister->flags |= LISTERF_SNIFFING;

					// Turn off
					else
					{
						// Clear flag
						lister->flags &= ~LISTERF_SNIFFING;

						// Anyone waiting to find out about this?
						if (!(lister->flags & LISTERF_BUSY) && !(IsListEmpty((struct List *)&lister->wait_list)))
						{
							lister_relieve_waiters(lister, LISTERWAIT_UNBUSY);
						}
					}
					break;

				// Change sort format
				case LISTER_CONFIGURE:

					// Saying hello?
					if (lmsg->flags)
						lister->more_flags |= LISTERF_CONFIGURE;

					// Saying goodbye
					else
					{
						// New format?
						if (lmsg->data_free)
							lister_change_format(lister, (ListFormat *)lmsg->data_free);

						// Clear flag
						lister->more_flags &= ~LISTERF_CONFIGURE;

						// Save?
						if (((IPTR)lmsg->data) & CONFIG_SAVE && lmsg->data_free)
						{
							short flags = POSUPF_SAVE | POSUPF_FORMAT;

							// Default format?
							if ((CompareListFormat(&environment->env->list_format, (ListFormat *)lmsg->data_free)) == 0)
								flags |= POSUPF_DEFAULT;

							// Snapshot lister
							PositionUpdate(lister, flags);
						}
					}
					break;

				// Select a file
				case LISTER_SELSHOW:

					// Is flag set?
					if (lmsg->flags)
					{
						// Clear flag
						lmsg->flags = 0;

						// Add message to waiters
						lister_new_waiter(lister, lmsg, 0, LISTERWAIT_AUTO_CMD);
						lmsg = 0;
						break;
					}

					// Select entry
					lister_sel_show(lister, (char *)lmsg->data_free);
					break;

				// Set gauge state
				case LISTER_SET_GAUGE: {
					short restore_side;

					restore_side = lister_proc_apply_side(&lister, flags, TRUE);
					lister_set_gauge(lister, TRUE);
					lister_proc_restore_side(lister, restore_side);
					break;
				}

				// Highlight an entry
				case LISTER_HIGHLIGHT:
					lmsg->command = (IPTR)lister_highlight(
						lister, (lmsg->flags) & 0xffff, (lmsg->flags >> 16) & 0xffff, (DragInfo *)lmsg->data);
					break;
				}

				// Reply the message
				IPC_Reply(lmsg);
			}
		}

		// Check quit flag
		if (quit_flag)
			break;

		// Update progress bar?
		if (progress_flag)
		{
			// Update things
#ifdef USE_64BIT
			SetProgressWindowTags(lister->progress_window,
								  (progress_flag & PWF_FILENAME) ? PW_FileName : TAG_IGNORE,
								  (IPTR)progress_filename,
								  (progress_flag & PWF_GRAPH) ? PW_FileNum : TAG_IGNORE,
								  progress_count,
								  (progress_flag & PWF_FILESIZE) ? PW_FileDone64 : TAG_IGNORE,
								  (IPTR)&file_progress_count,
								  TAG_END);
#else
			SetProgressWindowTags(lister->progress_window,
								  (progress_flag & PWF_FILENAME) ? PW_FileName : TAG_IGNORE,
								  progress_filename,
								  (progress_flag & PWF_GRAPH) ? PW_FileNum : TAG_IGNORE,
								  progress_count,
								  (progress_flag & PWF_FILESIZE) ? PW_FileDone : TAG_IGNORE,
								  file_progress_count,
								  TAG_END);
#endif
			// Free filename
			FreeMemH(progress_filename);
		}

		// Wait for a message
		if (wait_flag)
		{
			ULONG res =
				Wait(1 << lister->ipc->command_port->mp_SigBit | 1 << lister->app_port->mp_SigBit |
					 1 << lister->timer_port->mp_SigBit | 1 << lister->abort_signal | 1 << lister->hot_name_bit |
					 1 << lister->filter_name_bit |
					 ((lister->window) ? 1 << lister->window->UserPort->mp_SigBit : 0) |
					 ((lister->backdrop_info->notify_req) ? 1 << lister->backdrop_info->notify_port->mp_SigBit : 0));

			// Abort?
			if (res & (1 << lister->abort_signal))
				lister_send_abort(lister);

			// Hot name?
			if (res & (1 << lister->hot_name_bit))
				lister_handle_hotname(lister);

			// Live filter?
			if (res & (1 << lister->filter_name_bit))
				lister_handle_filter(lister);
		}
	}

	// Set flag to indicate closing
	lister->flags |= LISTERF_CLOSING;

	// Close display
	lister_close(lister, 1);

	// Anyone waiting to find out when lister finishes?
	if (!(IsListEmpty((struct List *)&lister->wait_list)))
		lister_relieve_waiters(lister, LISTERWAIT_ALL);

	// Free sniff list
	if (!IsListEmpty((struct List *)&sniff_list))
	{
		SniffData *sniff;

		while ((sniff = (SniffData *)sniff_list.mlh_Head)->node.mln_Succ)
		{
			Remove((struct Node *)sniff);
			FreeVec(sniff);
		}
	}

	// Cleanup lister
	lister_cleanup(lister, 1);
}

// Cleanup a lister
void lister_cleanup(Lister *lister, BOOL bye)
{
	struct Node *node;

	// Detach from any dual lister state
	lister_dual_detach_closing(lister);

	// Send goodbye message?
	if (bye)
	{
		// Update our position
		if (!(environment->env->lister_options & LISTEROPTF_SNAPSHOT))
			PositionUpdate(lister, 0);

		// Send goodbye message
		IPC_Goodbye(lister->ipc, &main_ipc, WINDOW_LISTER);
	}

	// Is lister iconified?
	if (lister->appicon)
	{
		// Remove AppIcon
		RemoveAppIcon(lister->appicon);

		// Free icon
		FreeDiskObjectCopy(lister->appicon_icon);
	}

	// If we still have a progress window, close it
	lister_progress_off(lister);

	// Showing a special buffer?
	if (lister->cur_buffer == lister->special_buffer)
	{
		// Return to normal
		lister->cur_buffer = lister->old_buffer;
	}

	// Remove buffer lock
	if (lister->cur_buffer)
	{
		// Get buffer list lock
		lock_listlock(&GUI->buffer_list, TRUE);

		// Unlock buffer
		buffer_clear_lock(lister->cur_buffer, 1);

		// If buffer is empty or buffer count exceeds maximum, free it
		if (lister->cur_buffer->buf_TotalEntries[0] == 0 ||
			GUI->buffer_count > environment->env->settings.max_buffer_count ||
			(environment->env->settings.dir_flags & DIRFLAGS_DISABLE_CACHING))
		{
			// Free buffer
			lister_free_buffer(lister->cur_buffer);
		}

		// Unlock buffer list
		unlock_listlock(&GUI->buffer_list);
	}

	// Free user data
	while ((node = lister->user_data_list.list.lh_Head)->ln_Succ)
	{
		Remove(node);
		FreeMemH(node);
	}

	// Free ports
	if (lister->app_port)
	{
		DOpusAppMessage *msg;
		while ((msg = (DOpusAppMessage *)GetMsg(lister->app_port)))
			ReplyAppMessage(msg);
		DeleteMsgPort(lister->app_port);
	}

	// Close font
	if (lister->font)
		CloseFont(lister->font);

	// Free signals
	if (lister->abort_signal != -1)
		FreeSignal(lister->abort_signal);
	if (lister->hot_name_bit != -1)
		FreeSignal(lister->hot_name_bit);
	if (lister->filter_name_bit != -1)
		FreeSignal(lister->filter_name_bit);

	// Free timer stuff
	if (lister->timer_port)
	{
		// Close timers
		FreeTimer(lister->foo_timer);
		FreeTimer(lister->icon_drag_timer);
		FreeTimer(lister->busy_timer);
		FreeTimer(lister->edit_timer);
		FreeTimer(lister->scroll_timer);

		// Delete timer port
		DeleteMsgPort(lister->timer_port);
	}

	// Free regions
	if (lister->title_region)
		DisposeRegion(lister->title_region);
	if (lister->refresh_extra)
		DisposeRegion(lister->refresh_extra);

	// Free special buffer
	if (lister->special_buffer)
		buffer_free(lister->special_buffer);

	// Free reselection
	FreeReselect(&lister->reselect);

	// Free history
	Att_RemList(lister->path_history, 0);

	// Free popup menu
	PopUpFreeHandle(lister->lister_tool_menu);

	// Free toolbar
	FreeToolBar(lister->toolbar_alloc);

	// Free IPC data
	IPC_Free(lister->ipc);

	// Free lister data
	lister_free(lister);

	// Decrement count
	--main_lister_count;
}

// Sends abort from lister to locking process
void lister_send_abort(Lister *lister)
{
	// Do we have a known locker?
	if (lister->locker_ipc)
	{
		// Send abort command
		if ((IPC_SafeCommand(lister->locker_ipc, IPC_ABORT, 0, lister, 0, 0, &GUI->function_list)) != (ULONG)-1)
		{
			// Also signal control C to break MatchFirst/MatchNext
			if (lister->locker_ipc->proc)
			{
				Signal((struct Task *)lister->locker_ipc->proc, (SIGBREAKF_CTRL_C | IPCSIG_QUIT));
			}
		}
	}

	// Set abort flag
	lister->flags |= LISTERF_ABORTED;

	// Custom lister handler?
	if (lister->cur_buffer->buf_CustomHandler[0])
	{
		char port_name[40];

		// Abort trap installed?
		if (FindFunctionTrap("abort", lister->cur_buffer->buf_CustomHandler, port_name))
		{
			// Send abort message
			rexx_handler_msg(port_name, lister->cur_buffer, 0, HA_String, 0, "abort", HA_Value, 1, lister, TAG_END);
		}
	}
}

// Handle sniff message
short lister_handle_sniff(Lister *lister, SniffData *data)
{
	DirEntry *entry;
	long off;
	short show = 0;

	// Find entry
	if ((entry = find_entry(&data->buffer->entry_list, data->name, &off, data->buffer->more_flags & DWF_CASE)))
	{
		char *version = 0, *type = 0;
		char buf[40];

		// Got a filetype?
		if (data->type || *data->type_name)
		{
			// Get name pointer
			if ((type = (data->type) ? data->type->type.name : data->type_name) && *type)
			{
				// Set filetype description
				direntry_add_string(data->buffer, entry, DE_Filetype, type);

				// Directory sorted by filetype?
				if (data->buffer->buf_ListFormat.sort.sort == DISPLAY_FILETYPE)
				{
					// Remove entry
					remove_file_entry(data->buffer, entry);

					// Add it again
					add_file_entry(data->buffer, entry, 0);
					show |= SNIFFF_SHOW;
				}
			}
		}

		// Got a version?
		if (data->flags & SNIFFF_VERSION)
		{
			// Set version
			direntry_add_version(data->buffer, entry, data->ver, data->rev, data->days);

			// Build version string
			build_version_string(buf, data->ver, data->rev, data->days, -1);
			version = buf;

			// Directory sorted by version?
			if (data->buffer->buf_ListFormat.sort.sort == DISPLAY_VERSION)
			{
				// Remove entry
				remove_file_entry(data->buffer, entry);

				// Add it again
				add_file_entry(data->buffer, entry, 0);
				show |= SNIFFF_SHOW;
			}
		}

		// Is buffer currently visible?
		if (lister->cur_buffer == data->buffer && !(lister->flags & LISTERF_VIEW_ICONS))
		{
			short len;

			// Type field changed, and don't have a custom width for filetype field?
			if (type && *type && !(data->buffer->buf_CustomWidthFlags & (1 << DISPLAY_FILETYPE)))
			{
				short old;

				// Get new field size
				old = data->buffer->buf_FieldWidth[DISPLAY_FILETYPE];
				lister_check_max_length(lister, type, &len, DISPLAY_FILETYPE);
				old -= data->buffer->buf_FieldWidth[DISPLAY_FILETYPE];

				// Field needs to be bigger?
				if (old < 0)
				{
					// Add to width
					data->buffer->buf_HorizLength -= old;
					if (data->buffer->type_length < len)
						data->buffer->type_length = len;
					show |= SNIFFF_SLIDERS;
				}
			}

			// Same for version field
			if (version && !(data->buffer->buf_CustomWidthFlags & (1 << DISPLAY_VERSION)))
			{
				short old;

				// Get new field size
				old = data->buffer->buf_FieldWidth[DISPLAY_VERSION];
				lister_check_max_length(lister, version, &len, DISPLAY_VERSION);
				old -= data->buffer->buf_FieldWidth[DISPLAY_VERSION];

				// Field needs to be bigger?
				if (old < 0)
				{
					// Add to width
					data->buffer->buf_HorizLength -= old;
					if (data->buffer->version_length < len)
						data->buffer->version_length = len;
					show |= SNIFFF_SLIDERS;
				}
			}

			// Update slider?
			if (show & SNIFFF_SLIDERS)
				show |= SNIFFF_SHOW;

			// Don't need to redraw?
			else if (!(show & SNIFFF_SHOW))
			{
				// Is entry visible?
				if (off >= lister->cur_buffer->buf_VertOffset &&
					off < lister->cur_buffer->buf_VertOffset + lister->text_height)
				{
					// Show entry
					display_entry(entry, lister, off - lister->cur_buffer->buf_VertOffset);
				}
			}
		}
		else
			show = 0;
	}

	return show;
}
