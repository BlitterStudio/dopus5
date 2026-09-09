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

#define BUTTONS_FLASH_RATE 500000

// Initialise a new button bank
IPC_StartupCode(buttons_init, Buttons *, buttons, static)
{
	// Store IPC and buttons pointers
	buttons->ipc = ipc;
	ipc->userdata = buttons;

	// Initialise boopsi list
	NewList(&buttons->boopsi_list);

	// Memory handle
	if (!(buttons->memory = NewMemHandle(1024, 256, MEMF_CLEAR)))
		return 0;

	// Get a file requester
	if (!(buttons->filereq = alloc_filereq()))
		return 0;

	// Create app message port
	if (!(buttons->app_port = CreateMsgPort()))
		return 0;

	// Open timer
	if (!(buttons->timer = AllocTimer(UNIT_VBLANK, 0)))
		return 0;

	// Start timer
	StartTimer(buttons->timer, 5, 0);

	// Load button bank information
	if (buttons->buttons_file[0] && !buttons->bank)
	{
		// Open bank
		if (!(buttons->bank = OpenButtonBank(buttons->buttons_file)))
		{
			// Failed, try to create a new one if we're allowed to
			if (buttons->flags & BUTTONF_FAIL || !(buttons->bank = NewButtonBank(1, 0)))
				return 0;
		}
	}

	// Valid bank?
	if (buttons->bank)
	{
		// Coordinates not supplied?
		if (!(buttons->flags & BUTTONF_COORDS_SUPPLIED))
			buttons->pos = buttons->bank->window.pos;
	}

	return 1;
}

// The code that actually runs a button bank
IPC_EntryCode(buttons_code)
{
	Buttons *buttons;
	IPCMessage *lmsg;
	struct IntuiMessage *imsg;
	DOpusAppMessage *msg;
	IPCData *ipc;

	// Do startup
	if (!(IPC_ProcStartup((IPTR *)&buttons, (ULONG (*)(IPCData *, APTR))&buttons_init)))
	{
		buttons_cleanup(buttons, 0);
		return;
	}

	// Event loop
	FOREVER
	{
		short quit_flag = 0;
		short wait_flag = 1;

		// See if timer has finished
		if (CheckTimer(buttons->timer))
		{
			// Is editor up?
			if (buttons->editor && buttons->window)
			{
				// Are we dragging?
				if (buttons->drag_info)
				{
					// Check for deadlocks
					if (buttons->tick_count == buttons->last_tick)
					{
						// Abort drag
						buttons_stop_drag(buttons, -1, -1);
					}

					// Remember last tick count
					buttons->last_tick = buttons->tick_count;
				}

				// Show highlight
				else if (!(buttons->flags & BUTTONF_NO_FLASH))
					buttons_show_highlight(buttons);

				// Flash quickly
				StartTimer(buttons->timer, 0, BUTTONS_FLASH_RATE);
			}

			// Otherwise
			else
			{
				// Is there a pending edit request?
				if (buttons->flags & BUTTONF_EDIT_REQUEST)
				{
					// If the editor isn't there any more, cancel request
					if (!(IPC_FindProc(&GUI->process_list, NAME_BUTTON_EDITOR_RUN, 0, 0)))
						buttons->flags &= ~BUTTONF_EDIT_REQUEST;
				}

				// Button held down?
				if (buttons->flags & BUTTONF_BUTTON_HELD && buttons->window)
				{
					short x, y;

					// Get coordinates
					x = buttons->window->MouseX;
					y = buttons->window->MouseY;

					// Check mouse hasn't moved too far
					if (x > buttons->click_x - 3 && x < buttons->click_x + 3 && y > buttons->click_y - 3 &&
						y < buttons->click_y + 3)
					{
						// Check button is still held down
						if (buttons->button_sel_button && buttons->button_sel_flag)
						{
							// Do popup button list
							buttons_do_popup(buttons, SELECTDOWN);
						}
					}
				}

				// Slow timer
				StartTimer(buttons->timer, 8, 0);
			}

			// Clear held flag
			buttons->flags &= ~BUTTONF_BUTTON_HELD;
		}

		// AppMessages?
		while ((msg = (DOpusAppMessage *)GetMsg(buttons->app_port)))
		{
			// Menu operation?
			if (msg->da_Msg.am_Type == MTYPE_APPSNAPSHOT)
			{
				struct AppSnapshotMsg *asma;

				// Get SnapShot message pointer
				asma = (struct AppSnapshotMsg *)msg;

				// Menu operation?
				if (asma->flags & APPSNAPF_MENU)
				{
					// Close buttons?
					if (asma->id == LISTERPOPUP_CLOSE)
					{
						// Help?
						if (asma->flags & APPSNAPF_HELP)
						{
							// Send help
							help_menu_help(MENU_CLOSE_BUTTONS, 0);
						}

						// Send ourselves a quit command
						else
							IPC_Command(buttons->ipc, IPC_QUIT, 0, 0, 0, 0);
					}
				}
			}

			// AppIcon?
			else if (msg->da_Msg.am_Type == MTYPE_APPICON)
			{
				// De-iconify?
				if (msg->da_Msg.am_NumArgs == 0)
				{
					// Update icon position
					buttons_update_icon(buttons);

					// Remove AppIcon
					RemoveAppIcon(buttons->appicon);
					buttons->appicon = 0;
					buttons->flags &= ~BUTTONF_ICONIFIED;

					// Free icon
					if (buttons->appicon_icon)
					{
						FreeDiskObjectCopy(buttons->appicon_icon);
						buttons->appicon_icon = 0;
					}

					// Open window
					buttons_open(buttons, GUI->screen_pointer, BUTOPENF_REMAP);
				}
			}

			// Otherwise, window message
			else if (buttons->window)
			{
				// Handle the message
				if (buttons_app_message(buttons, msg))
					msg = 0;
			}

			// Reply message
			ReplyAppMessage(msg);
		}

		// Commands
		if ((lmsg = (IPCMessage *)GetMsg(buttons->ipc->command_port)))
		{
			wait_flag = 0;

			// Look at command
			switch (lmsg->command)
			{
			// Quit
			case IPC_QUIT:

				// Ok to quit?
				if (buttons_check_change(buttons, 0))
				{
					// Set quit flag
					quit_flag = 1;
				}

				// Abort quit
				else
					lmsg->command = IPC_ABORT;
				break;

			// Reset (menus)
			case IPC_RESET:

				// Gotta window?
				if (buttons->window && !(buttons->flags & BUTTONF_TOOLBAR))
				{
					// Reset menus?
					if (lmsg->flags)
					{
						display_free_menu(buttons->window);
						display_get_menu(buttons->window);
					}

					// Fix menus
					display_fix_menu(buttons->window, WINDOW_BUTTONS, 0);
				}
				break;

			// Close button bank
			case BUTTONS_CLOSE:
			case IPC_HIDE:
				buttons_close(buttons, 0);
				break;

			// Open button bank
			case BUTTONS_OPEN:
			case IPC_SHOW:
				// Don't open if iconified
				if (!buttons->appicon)
				{
					// Try to open
					if (!(buttons_open(buttons, (struct Screen *)lmsg->data, BUTOPENF_REMAP)))
					{
						lmsg->command = IPC_ABORT;
						quit_flag = 1;
						break;
					}

					// Activate?
					if (lmsg->flags && buttons->window)
						ActivateWindow(buttons->window);
				}
				break;

			// Iconify
			case BUTTONS_ICONIFY:
				buttons_do_function(buttons, MENU_ICONIFY_BUTTONS);
				break;

			// Editor saying hello
			case BUTTONEDIT_HELLO:

				// Clear request flag
				buttons->flags &= ~BUTTONF_EDIT_REQUEST;

				// Is window open?
				if (buttons->window)
				{
					// Initialise edits
					buttons->editor_sel_row = -1;
					buttons->editor_sel_col = -1;
					//						buttons->button_sel_button=0;
					buttons->flags &= ~BUTTONF_HIGH_SHOWN;

					// Store editor pointer
					buttons->editor = (IPCData *)lmsg->data;

					// Activate window
					ActivateWindow(buttons->window);

					// Send quick time request
					StartTimer(buttons->timer, 0, BUTTONS_FLASH_RATE);
				}

				// Otherwise, tell editor to piss off
				else
				{
					IPC_Command((IPCData *)lmsg->data, BUTTONEDIT_PISS_OFF, 0, buttons->bank, 0, 0);
				}
				break;

			// Editor saying goodbye
			case BUTTONEDIT_GOODBYE: {
				BOOL reopen = 0;
				ULONG flags = BUTREFRESH_REFRESH;

				// Clear editor pointer and flags
				buttons->editor = 0;
				if (lmsg->flags)
					buttons->flags |= BUTTONF_CHANGED;

				// Break if no window
				if (!buttons->window)
					break;

				// Border flag changed?
				if (buttons->bank->window.flags & BTNWF_NO_BORDER && buttons->vert_scroll)
					reopen = 1;

				else if (!(buttons->bank->window.flags & BTNWF_NO_BORDER) && !buttons->vert_scroll)
					reopen = 1;

				// Flag not changed, no border?
				else if (buttons->bank->window.flags & BTNWF_NO_BORDER)
				{
					// Border position changed?
					if ((buttons->bank->window.flags & (BTNWF_HORIZ | BTNWF_VERT | BTNWF_NONE | BTNWF_RIGHT_BELOW)) !=
						buttons->border_type)
						flags |= BUTREFRESH_RESIZE;
				}

				// Refresh changed?
				if (buttons->bank->window.flags & BTNWF_SMART_REFRESH && buttons->window->Flags & WFLG_SIMPLE_REFRESH)
					reopen = 1;

				else if (!(buttons->bank->window.flags & BTNWF_SMART_REFRESH) &&
						 !(buttons->window->Flags & WFLG_SIMPLE_REFRESH))
					reopen = 1;

				// Don't reopen?
				if (!reopen || buttons->flags & BUTTONF_TOOLBAR)
				{
					// Refresh display
					buttons_refresh(buttons, flags);
					break;
				}

				// Fall through
			}

			// Re-open window
			case BUTTONEDIT_REOPEN: {
				BOOL edit = 0;

				// Were we being edited?
				if (buttons->editor)
					edit = 1;
				buttons->editor = 0;

				// Close and re-open
				buttons_close(buttons, BUTCLOSEF_NO_REMAP);
				if (buttons_open(buttons, GUI->screen_pointer, 0))
				{
					// Re-edit?
					if (edit)
						buttons_edit_bank(buttons, -2, -2, 0, 0, TRUE);
				}
			}
			break;

			// Redraw the bank
			case BUTTONEDIT_REDRAW:

				// Refresh display
				if (buttons->window)
					buttons_refresh(buttons, BUTREFRESH_REFRESH);
				break;

			// New bank pointer supplied
			case BUTTONEDIT_NEW_BANK:

				// Store new bank pointer
				buttons->bank = (Cfg_ButtonBank *)lmsg->data;

				// New bank?
				if (buttons->flags & BUTTONF_NEW_BANK)
				{
					// Bank was new, and cancelled, so we can close it
					quit_flag = 1;
					break;
				}

				// Refresh if window is open
				if (buttons->window)
				{
					if (!(buttons->window->Flags & WFLG_BORDERLESS))
						SetWindowTitles(buttons->window, buttons->bank->window.name, (char *)-1);
					buttons_refresh(buttons, BUTREFRESH_FONT | BUTREFRESH_RESIZE | BUTREFRESH_REFRESH);
				}

				// Clear change flag if necessary
				if (lmsg->flags)
					buttons->flags &= ~BUTTONF_CHANGED;
				break;

			// Asked to supply bank pointer
			case BUTTONEDIT_GIMME_BANK:
				lmsg->command = (IPTR)buttons->bank;
				if (lmsg->data && buttons->window)
				{
					((Point *)lmsg->data)->x = buttons->window->LeftEdge;
					((Point *)lmsg->data)->y = buttons->window->TopEdge;
				}
				break;

			// Refresh window
			case BUTTONEDIT_REFRESH:
				buttons_refresh(buttons, lmsg->flags);

				// Refresh window title (kludgy)
				if (!(lmsg->flags & BUTREFRESH_SELECTOR) && !(buttons->window->Flags & WFLG_BORDERLESS))
					SetWindowTitles(buttons->window, buttons->bank->window.name, (char *)-1);
				break;

			// Get current selection
			case BUTTONEDIT_GET_SELECTION: {
				struct colrow_data *data;

				data = (struct colrow_data *)lmsg->data;
				data->col = buttons->editor_sel_col;
				data->row = buttons->editor_sel_row;

				if (buttons->editor_sel_col != -1 && buttons->editor_sel_row != -1)
					lmsg->command = 1;
				else
					lmsg->command = 0;
			}
			break;

			// Set selection
			case BUTTONEDIT_SET_SELECTION:
				buttons->editor_sel_col = (short)lmsg->flags;
				buttons->editor_sel_row = (short)(IPTR)lmsg->data;
				lmsg->command = buttons_visible_select(buttons);
				break;

			// Get button
			case BUTTONEDIT_GET_BUTTON:
				lmsg->command = (IPTR)button_from_pos(buttons, (short)lmsg->flags, (short)(IPTR)lmsg->data);
				break;

			// Get button from a point
			case BUTTONEDIT_GET_BUTTON_POINT:
				lmsg->command = (IPTR)button_from_point(buttons, (short *)lmsg->flags, (short *)lmsg->data);
				break;

			// Control flash
			case BUTTONEDIT_FLASH:

				// Stop flash?
				if (lmsg->flags != 1)
				{
					// Stop flashing
					buttons_stop_highlight(buttons);

					// Clear selection
					if (lmsg->flags == (ULONG)-1)
					{
						buttons->editor_sel_col = -1;
						buttons->editor_sel_row = -1;
						buttons->button_sel_button = 0;
					}
				}

				// Start flash
				else
				{
					buttons->flags &= ~BUTTONF_NO_FLASH;
				}
				break;

			// Redraw button
			case BUTTONEDIT_REDRAW_BUTTON:
				GetSemaphore(&buttons->bank->lock, SEMF_SHARED, 0);
				buttons_redraw_button(buttons, (Cfg_Button *)lmsg->data);
				FreeSemaphore(&buttons->bank->lock);
				break;

			// Sent a button from another button bank
			case BUTTONEDIT_CLIP_BUTTON: {
				short x = -1, y = -1;

				// Get position
				if (lmsg->data_free)
				{
					x = ((Point *)lmsg->data_free)->x;
					y = ((Point *)lmsg->data_free)->y;
				}

				// Send message to edit us and give us this button
				buttons_edit_bank(buttons, x, y, (Cfg_Button *)lmsg->data, 0, TRUE);

				// Set change flag
				buttons->flags |= BUTTONF_CHANGED;
			}
			break;

			// Tell button to save itself
			case BUTTONS_SAVE:
			case BUTTONS_SAVEAS:
				buttons_do_function(buttons, MENU_SAVE_BUTTONS + (lmsg->command - BUTTONS_SAVE));
				break;

			// Show help on something
			case IPC_HELP: {
				short x, y;
				Cfg_Button *button;

				// Get coordinates
				x = ((IPTR)lmsg->data) >> 16;
				y = ((IPTR)lmsg->data) & 0xffff;

				// Check point is within button area
				if (x < buttons->internal.Left || y < buttons->internal.Top ||
					x >= buttons->internal.Left + buttons->internal.Width ||
					y >= buttons->internal.Top + buttons->internal.Height)
					break;

				// Lock bank
				GetSemaphore(&buttons->bank->lock, SEMF_SHARED, 0);

				// Get button from this point
				if ((button = button_from_point(buttons, &x, &y)))
				{
					// Show help for this button
					help_button_help(button, 0, lmsg->flags, 0, GENERIC_BUTTON);
				}

				// Unlock bank
				FreeSemaphore(&buttons->bank->lock);
			}
			break;

			// Defaults (toolbar)
			case BUTTONEDIT_MENU_DEFAULTS:
				buttons_do_function(buttons, MENU_TOOLBAR_RESET_DEFAULTS);
				break;

			// Last saved (toolbar)
			case BUTTONEDIT_MENU_LASTSAVED:
				buttons_do_function(buttons, MENU_TOOLBAR_LAST_SAVED);
				break;

			// Restore (toolbar)
			case BUTTONEDIT_MENU_RESTORE:
				buttons_do_function(buttons, MENU_TOOLBAR_RESTORE);
				break;
			}

			// Reply the message
			IPC_Reply(lmsg);
		}

		// Intuition messages
		if (buttons->window && !quit_flag)
		{
			if ((imsg = (struct IntuiMessage *)GetMsg(buttons->window->UserPort)))
			{
				struct IntuiMessage msg_copy;

				// Copy message
				msg_copy = *imsg;

				// Menu verify?
				if (imsg->Class == IDCMP_MENUVERIFY)
				{
					// Was it a real right-button push?
					if (imsg->Qualifier & IEQUALIFIER_RBUTTON)
					{
						// Is the window active?
						if (buttons->window->Flags & WFLG_WINDOWACTIVE)
						{
							BOOL cancel = 0;

							// Cancel drag
							if (buttons->drag_info)
								cancel = 1;

							// Did event happen over the window?
							else if (imsg->MouseX >= 0 && imsg->MouseX < buttons->window->Width && imsg->MouseY >= 0 &&
									 imsg->MouseY < buttons->window->Height)
							{
								// Over border?
								if (imsg->MouseX < buttons->internal.Left ||
									imsg->MouseX >= buttons->internal.Left + buttons->internal.Width ||
									imsg->MouseY < buttons->internal.Top ||
									imsg->MouseY >= buttons->internal.Top + buttons->internal.Height)
								{
									// Only cancel if not a toolbar
									if (!(buttons->flags & BUTTONF_TOOLBAR))
										cancel = 1;
								}

								// Cancel always
								else
									cancel = 1;
							}

							// Cancel?
							if (cancel)
							{
								// Cancel menu event
								imsg->Code = MENUCANCEL;

								// Change our copy to MOUSEBUTTONS
								msg_copy.Class = IDCMP_MOUSEBUTTONS;
								msg_copy.Code = MENUDOWN;
							}
						}
					}
				}

				// Reply to message
				ReplyFreeMsg(imsg);
				wait_flag = 0;

				// Handle buttons message
				if (!(buttons_process_msg(buttons, &msg_copy)))
					quit_flag = 1;
			}
		}

		// Check quit flag
		if (quit_flag)
		{
			// Can only quit if there's no pending edit request
			if (!(buttons->flags & BUTTONF_EDIT_REQUEST) ||
				!(IPC_FindProc(&GUI->process_list, NAME_BUTTON_EDITOR_RUN, 0, 0)))
				break;
		}

		// Wait for a message
		if (wait_flag)
			Wait(1 << buttons->ipc->command_port->mp_SigBit | 1 << buttons->app_port->mp_SigBit |
				 1 << buttons->timer->port->mp_SigBit |
				 ((buttons->window) ? 1 << buttons->window->UserPort->mp_SigBit : 0));
	}

	// Lock process list
	lock_listlock(&GUI->process_list, FALSE);

	// If editor is open, send it a goodbye message
	if ((ipc = IPC_FindProc(&GUI->process_list, NAME_BUTTON_EDITOR_RUN, 0, 0)))
	{
		// Send goodbye
		IPC_Command(ipc, BUTTONEDIT_GOODBYE, 0, buttons->bank, 0, 0);
	}

	// Clear editor pointer
	buttons->editor = 0;

	// Unlock list
	unlock_listlock(&GUI->process_list);

	// Close display
	buttons_close(buttons, BUTCLOSEF_SCRIPT);

	// Get any outstanding messages
	while ((lmsg = (IPCMessage *)GetMsg(buttons->ipc->command_port)))
	{
		// New bank?
		if (lmsg->command == BUTTONEDIT_NEW_BANK)
			buttons->bank = (Cfg_ButtonBank *)lmsg->data;

		// Button?
		else if (lmsg->command == BUTTONEDIT_CLIP_BUTTON)
			FreeButton((Cfg_Button *)lmsg->data);

		// Reply
		IPC_Reply(lmsg);
	}

	// Cleanup buttons
	buttons_cleanup(buttons, 1);
}

// Cleanup a button bank
void buttons_cleanup(Buttons *buttons, BOOL bye)
{
	IPCData *ipc = buttons->ipc;

	// Send goodbye message
	if (bye)
		IPC_Goodbye(ipc, &main_ipc, 0);

	// AppIcon?
	if (buttons->appicon)
	{
		// Remove AppIcon
		RemoveAppIcon(buttons->appicon);

		// Free icon
		FreeDiskObjectCopy(buttons->appicon_icon);
	}

	// Free ports
	if (buttons->app_port)
	{
		flush_port(buttons->app_port);
		DeleteMsgPort(buttons->app_port);
	}

	// Timer?
	FreeTimer(buttons->timer);

	// Free file requester
	FreeAslRequest(buttons->filereq);

	// Free memory
	FreeMemHandle(buttons->memory);

	// Free bank information
	CloseButtonBank(buttons->bank);
	CloseButtonBank(buttons->backup);
	FreeVec(buttons);

	// Free IPC data
	IPC_Free(ipc);
}

// Make button window unbusy
void buttons_busy(Buttons *buttons)
{
	if (buttons->window && !(buttons->flags & BUTTONF_BUSY))
	{
		InitRequester(&buttons->busy_req);
		Request(&buttons->busy_req, buttons->window);
		SetBusyPointer(buttons->window);
		buttons->flags |= BUTTONF_BUSY;
	}
}

// Make button window busy
void buttons_unbusy(Buttons *buttons)
{
	if (buttons->window && buttons->flags & BUTTONF_BUSY)
	{
		EndRequest(&buttons->busy_req, buttons->window);
		ClearPointer(buttons->window);
		buttons->flags &= ~BUTTONF_BUSY;
	}
}
