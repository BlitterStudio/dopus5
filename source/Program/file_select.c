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

#define EDIT_TIMEOUT 750000

// Select files with the mouse
// Called from LISTER PROCESS
short select_select_files(Lister *lister, UWORD qual, short mouse_x, short mouse_y)
{
	char drag_ok = DRAG_OK, drag_status = DRAG_OK;
	short old_mouse_x, old_mouse_y, drag_immediate = 0;
	short first_mouse_x, first_mouse_y;
	short line, old_line, first_line;
	short drag_offset_x = 0, drag_offset_y = 0;
	char state = 0;
	char break_flag = 0;
	signed char scroll_dir = SCROLL_NO;
	char multi_drag = 0, drag_type = 0;
	struct IntuiMessage *msg;
	DirBuffer *buffer;
	DirEntry *entry;
	DragInfo *drag_info = 0;
	unsigned long timer_micro = 500000;
	unsigned char device_list = 0, is_inside = 1;
	unsigned long count = 1;
	unsigned long tick_count = 1, last_tick = 0;
	unsigned long oldidcmp;
	BOOL edit_active = 0;
	short retval = 0;
	Lister *over_lister = 0;
	DirEntry *over_entry = 0;

	// If window was not already a source, activate it
	if (!(lister->flags & (LISTERF_SOURCE | LISTERF_SOURCEDEST_LOCK)))
	{
		lister_do_function(lister, MENU_LISTER_SOURCE);
		if (lister->flags & LISTERF_SOURCE && environment->env->lister_options & LISTEROPTF_NOACTIVESELECT)
		{
			short count;

			// If there's more than one lister, don't select the file
			lock_listlock(&GUI->lister_list, FALSE);
			count = Att_NodeCount((Att_List *)&GUI->lister_list.list);
			unlock_listlock(&GUI->lister_list);
			if (count > 1)
				return 0;
		}
	}

	// Get buffer pointer
	buffer = lister->cur_buffer;

	// Update selection information
	select_show_info(lister, 1);

	// If window is empty, return
	if (buffer->buf_TotalEntries[0] == 0)
		return 0;

	// Lock buffer
	buffer_lock(buffer, FALSE);

	// Store mouse position, and initialise selected entry
	old_mouse_x = mouse_x;
	old_mouse_y = mouse_y;
	first_mouse_x = mouse_x;
	first_mouse_y = mouse_y;
	lister->selected_entry = 0;

	// Get line clicked on
	line = (mouse_y - lister->text_area.rect.MinY) / lister->text_area.font->tf_YSize;
	old_line = line;
	first_line = line;

	// Drag immediately?
	if (qual & IEQUALIFIER_LALT && !(qual & IEQUAL_ANYSHIFT) &&
		(buffer->buf_SelectedFiles[0] + buffer->buf_SelectedDirs[0]) > 0)
	{
		if ((lister->selected_entry = get_entry(&buffer->entry_list, 1, ENTRY_ANYTHING)))
		{
			state = 1;
			drag_immediate = 1;
			if ((buffer->buf_SelectedFiles[0] + buffer->buf_SelectedDirs[0]) > 1)
				multi_drag = 1;
		}
	}

	// Select this entry
	if (!state && (state = select_toggle_entry(lister, line, qual, 1)) == SELECT_SPECIAL)
	{
		buffer_unlock(buffer);
		return 0;
	}

	// If entry was unselected, we won't be dragging
	if (!state || !lister->selected_entry)
		drag_status = DRAG_NO;

	// Otherwise, see if this is a device list
	else if (lister->selected_entry->de_Node.dn_Type == ENTRY_DEVICE)
		device_list = 1;

	// Drag immediately?
	if (drag_immediate)
		drag_status = DRAG_START;

	// If we have an entry, start timer
	else if (lister->selected_entry && !device_list)
	{
		// Allowed to have editing?
		if (environment->env->lister_options & LISTEROPTF_EDIT_LEFT)
		{
			edit_active = 1;
			StartTimer(lister->edit_timer, 0, (qual & IEQUALIFIER_RALT) ? 2 : EDIT_TIMEOUT);
		}
	}

	// Send initial time request
	StartTimer(lister->scroll_timer, 0, timer_micro);

	// Turn mouse move reporting on
	lister->window->Flags |= WFLG_REPORTMOUSE;

	// Start ticks
	oldidcmp = lister->window->IDCMPFlags;
	ModifyIDCMP(lister->window, oldidcmp | IDCMP_INTUITICKS);

	// Loop forever
	FOREVER
	{
		// Timer?
		if (CheckTimer(lister->scroll_timer))
		{
			// Dragging?
			if (drag_info)
			{
				// Check for drag deadlocks
				if (tick_count == last_tick)
				{
					// Abort drag
					drag_ok = DRAG_NO;
					break;
				}

				// Remember last tick count
				last_tick = tick_count;
			}

			// Send new timer request
			StartTimer(lister->scroll_timer, 0, timer_micro);
		}

		// Edit timer come up?
		else if (edit_active && CheckTimer(lister->edit_timer))
		{
			BPTR lock;
			D_S(struct InfoData, info)

			// Clear flag
			info->id_DiskState = ID_VALIDATED;

			// Lock current path
			if ((lock = Lock("", ACCESS_READ)))
			{
				// Get information
				Info(lock, info);
				UnLock(lock);
			}

			// Disk writeable?
			if (info->id_DiskState == ID_VALIDATED)
			{
				// Get edit position
				if (!(lister_start_edit(lister, first_mouse_x, first_mouse_y, -1)))
				{
					// Failed to start
					edit_active = 0;
				}

				// Ok
				else
				{
					// Set return code
					retval = 1;
					break;
				}
			}
			else
				edit_active = 0;
		}

		// Intuition?
		while ((msg = (struct IntuiMessage *)GetMsg(lister->window->UserPort)))
		{
			struct IntuiMessage msg_copy;

			// Get message copy
			msg_copy = *msg;
			qual = msg_copy.Qualifier;

			// Cancel any menu operations
			if (msg->Class == IDCMP_MENUVERIFY)
			{
				msg->Code = MENUCANCEL;
				msg_copy.Class = IDCMP_MOUSEBUTTONS;
				msg_copy.Code = MENUDOWN;
			}

			// Reply message
			ReplyMsg((struct Message *)msg);

			// Get mouse position
			mouse_x = lister->window->MouseX;
			mouse_y = lister->window->MouseY;

			// Look at message
			switch (msg_copy.Class)
			{
			// Refresh
			case IDCMP_REFRESHWINDOW:
				lister_refresh(lister, LREFRESH_REFRESH);
				break;

			// Intutick
			case IDCMP_INTUITICKS:
				++tick_count;

				// Check that left button is still down
				if (msg_copy.Qualifier & IEQUALIFIER_LEFTBUTTON)
				{
					// Are we dragging a bob?
					if (drag_status == DRAG_DOING)
					{
						// Not allowed to drop over a sub-dir?
						if (!(environment->env->lister_options & LISTEROPTF_SUBDROP) &&
							!(msg->Qualifier & IEQUAL_ANYSHIFT))
						{
							// If we had an entry highlighted, remove it
							if (over_entry)
								over_entry = lister_test_drag(lister, drag_info, -1, -1, &over_lister);
						}

						// Otherwise, we can drop over a sub-dir
						else
						{
							// Check lister drop position
							over_entry = lister_test_drag(lister,
														  drag_info,
														  mouse_x + lister->window->LeftEdge,
														  mouse_y + lister->window->TopEdge,
														  &over_lister);
						}
					}
				}
				break;

			// Mouse moved
			case IDCMP_MOUSEMOVE:

				// Check that left button is still down
				if (msg_copy.Qualifier & IEQUALIFIER_LEFTBUTTON)
				{
					// Are we dragging a bob?
					if (drag_status == DRAG_DOING)
					{
						// Check that mouse has moved
						if (mouse_x == old_mouse_x && mouse_y == old_mouse_y)
							break;

						// Show drag image in the new position
						ShowDragImage(drag_info, mouse_x - drag_offset_x, mouse_y - drag_offset_y);

						// Not allowed to drop over a sub-dir?
						if (!(environment->env->lister_options & LISTEROPTF_SUBDROP) &&
							!(msg->Qualifier & IEQUAL_ANYSHIFT))
						{
							// If we had an entry highlighted, remove it
							if (over_entry)
								over_entry = lister_test_drag(lister, drag_info, -1, -1, &over_lister);
						}

						// Otherwise, we can drop over a sub-dir
						else
						{
							// Check lister drop position
							over_entry = lister_test_drag(lister,
														  drag_info,
														  mouse_x + lister->window->LeftEdge,
														  mouse_y + lister->window->TopEdge,
														  &over_lister);
						}
					}

					// Or, not dragging?
					else
					{
						int line;

						// Get line we're over
						line = (mouse_y - lister->text_area.rect.MinY) / lister->text_area.font->tf_YSize;
						if (line > lister->text_height - 1)
							line = lister->text_height - 1;
						if (line < 0)
							line = 0;

						// Edit timer active?
						if (edit_active)
						{
							// Has line changed, or we've moved too far?
							if (line != first_line ||
								(mouse_x < first_mouse_x - lister->text_area.font->tf_XSize ||
								 mouse_x > first_mouse_x + lister->text_area.font->tf_XSize ||
								 mouse_x < lister->text_area.rect.MinX || mouse_x > lister->text_area.rect.MaxX))
							{
								// Abort timer
								edit_active = 0;
							}
						}

						// Device list?
						if (device_list)
						{
							// Different lines?
							if (old_line != line)
							{
								// Deselect the old line
								select_default_state(lister, old_line, 0);

								// Select the new line
								select_default_state(lister, line, 1);

								// Store new line
								old_line = line;
							}
						}

						// Normal entries
						else
						{
							// If line changed, select the new lines
							if (line != old_line)
							{
								short l;

								// Moved below old line?
								if (old_line < line)
								{
									// Select lines
									for (l = old_line; l <= line; l++)
										count += select_default_state(lister, l, state);
								}

								// Moved above old line
								else if (old_line > line)
								{
									// Select lines
									for (l = old_line; l >= line; l--)
										count += select_default_state(lister, l, state);
								}

								// Update info display
								select_show_info(lister, 1);
							}

							// Store new line
							old_line = line;
						}

						// See if mouse was moved horizontally outside of window
						if ((mouse_x < lister->text_area.rect.MinX || mouse_x > lister->text_area.rect.MaxX) &&
							mouse_y >= lister->text_area.rect.MinY && mouse_y <= lister->text_area.rect.MaxY)
						{
							// Ok to start dragging?
							if (is_inside && drag_status != DRAG_NO)
							{
								// If we have a valid entry, we can start dragging
								if (lister->selected_entry)
								{
									// Start dragging
									drag_status = DRAG_START;

									// Multi-drag?
									if ((lister->selected_entry->de_Node.dn_Type < 0 ||
										 !(msg_copy.Qualifier & IEQUAL_ANYSHIFT)) &&
										count > 1 && (buffer->buf_SelectedFiles[0] + buffer->buf_SelectedDirs[0]) > 1)
										multi_drag = 1;
								}
							}
						}
					}

					// See if mouse is inside lister area
					if (mouse_x >= lister->text_area.rect.MinX && mouse_x <= lister->text_area.rect.MaxX &&
						mouse_y >= lister->text_area.rect.MinY && mouse_y <= lister->text_area.rect.MaxY)
					{
						is_inside = 1;
					}
					else
					{
						is_inside = 0;
					}
					break;
				}

			// Inactive window means button up
			case IDCMP_INACTIVEWINDOW:

				// Are we dragging a bob?
				if (drag_ok && lister->selected_entry && drag_status == DRAG_DOING)
					msg_copy.Code = MENUDOWN;

				// Otherwise, left button up
				else
					msg_copy.Code = SELECTUP;

			// Mouse button
			case IDCMP_MOUSEBUTTONS:

				// Left button up?
				if (msg_copy.Code == SELECTUP)
				{
					// Set break flag
					break_flag = 1;
					break;
				}

				// Middle button?
				if (msg_copy.Code == MIDDLEDOWN && edit_active)
				{
					// Send off quick time request to be triggered immediately
					StartTimer(lister->edit_timer, 0, 2);
					break;
				}

				// Right button down?
				if (msg_copy.Code != MENUDOWN)
					break;

				// If we can't drag, or we don't have a valid entry, break
				if (!drag_ok || !lister->selected_entry)
					break;

				// If already dragging, stop it
				if (drag_status == DRAG_DOING)
				{
					drag_ok = DRAG_NO;
					break_flag = 1;
				}

				// Otherwise start dragging
				else if (is_inside)
				{
					drag_status = DRAG_START;

					// If shift is down and more than 1 file is selected, multi drag
					if ((lister->selected_entry->de_Node.dn_Type < 0 || !(msg_copy.Qualifier & IEQUAL_ANYSHIFT)) &&
						(buffer->buf_SelectedFiles[0] + buffer->buf_SelectedDirs[0]) > 1)
						multi_drag = 1;
				}
				break;
			}

			// See if we're about to start dragging
			if (drag_status == DRAG_START)
			{
				char *name_ptr = 0, *o_name_ptr;
				char drag_buffer[50];
				short height = 1;
				short str_len = 0;
				DirEntry *first_entry = 0;
				short first_sel = -1;

				// Abort timer
				edit_active = 0;

				// Multi-drag?
				if (multi_drag)
				{
					short last_sel = -1, num = 0;

					// Go through list
					for (entry = (DirEntry *)buffer->entry_list.mlh_Head; entry->de_Node.dn_Succ;
						 entry = (DirEntry *)entry->de_Node.dn_Succ, num++)
					{
						// Selected?
						if (entry->de_Flags & ENTF_SELECTED)
						{
							short len;

							// First selection?
							if (first_sel == -1)
							{
								first_sel = num;
								first_entry = entry;
								name_ptr = entry->de_Node.dn_Name;
							}

							// Store last selection
							last_sel = num;

							// Get name length; proportional font?
							if (lister->more_flags & LISTERF_PROP_FONT)
								len = TextLength(
									lister->window->RPort, entry->de_Node.dn_Name, strlen(entry->de_Node.dn_Name));
							else
								len = strlen(entry->de_Node.dn_Name) * lister->text_area.rast.TxWidth;

							// Check for longest name
							if (len > str_len)
								str_len = len;
						}
					}

					// Get height of drag buffer
					height = (last_sel - first_sel) + 1;

					// Too big to drag?
					if (height > lister->text_height)
						height = 1;

					// Dragging files
					drag_type = ENTRY_FILE;
				}

				// Otherwise, only dragging one thing
				else if (lister->selected_entry)
				{
					// Get entry
					first_entry = lister->selected_entry;

					// Use name by default
					name_ptr = first_entry->de_Node.dn_Name;

					// Device entry?
					if (first_entry->de_Node.dn_Type == ENTRY_DEVICE)
					{
						char *ptr;

						// Get comment pointer
						ptr = (char *)GetTagData(DE_Comment, 0, first_entry->de_Tags);

						// Buffer list entry?
						if (first_entry->de_SubType == SUBENTRY_BUFFER)
						{
							if (!(name_ptr = ptr))
								name_ptr = (char *)GetTagData(DE_DisplayString, 0, first_entry->de_Tags);
						}

						// Device?
						else if (first_entry->de_Size == DLT_DEVICE && ptr)
							name_ptr = ptr;
					}

					// Get type of entry
					drag_type = first_entry->de_Node.dn_Type;

					// Get name length
					if (name_ptr)
					{
						// Proportional font?
						if (lister->more_flags & LISTERF_PROP_FONT)
							str_len = TextLength(lister->window->RPort, name_ptr, strlen(name_ptr));
						else
							str_len = strlen(name_ptr) * lister->text_area.rast.TxWidth;
					}
				}

				// Valid drag?
				if (str_len)
				{
					// Create DragInfo
					if (!multi_drag || height > 1)
					{
						drag_info = GetDragInfo(lister->window,
												0,
												str_len + 2,
												lister->text_area.rast.TxHeight * height,
												DRAGF_NEED_GELS | DRAGF_CUSTOM);
					}

					// Failed?
					if (!drag_info)
					{
						// If multi-drag, try again with height=1
						if (multi_drag)
						{
							// Back to one line
							height = 1;

							// Build "multi-drag" string
							lsprintf(drag_buffer,
									 GetString(&locale, MSG_MULTI_DRAG),
									 buffer->buf_SelectedFiles[0],
									 buffer->buf_SelectedDirs[0]);
							name_ptr = drag_buffer;

							// Proportional font?
							if (lister->more_flags & LISTERF_PROP_FONT)
								str_len = TextLength(lister->window->RPort, name_ptr, strlen(name_ptr));
							else
								str_len = strlen(name_ptr) * lister->text_area.rast.TxWidth;

							// Get drag buffer
							drag_info = GetDragInfo(lister->window,
													0,
													str_len + 2,
													lister->text_area.rast.TxHeight,
													DRAGF_NEED_GELS | DRAGF_CUSTOM);
						}
					}

					// Fail?
					if (!drag_info)
					{
						// Can't drag'n'drop
						drag_ok = DRAG_NO;
						drag_status = DRAG_NO;
					}

					// Successful
					else
					{
						short l, y, a;

						// Get x offset
						drag_offset_x = mouse_x - lister->text_area.rect.MinX;
						if (drag_offset_x < 0)
							drag_offset_x = 0;
						else if (drag_offset_x > drag_info->width)
							drag_offset_x = drag_info->width;
						drag_offset_x -= lister->window->LeftEdge;

						// Get y offset
						drag_offset_y =
							mouse_y - (lister->text_area.rect.MinY + line * lister->text_area.font->tf_YSize);
						if (multi_drag)
						{
							drag_offset_y +=
								(buffer->buf_VertOffset + line - first_sel) * lister->text_area.font->tf_YSize;
						}
						if (drag_offset_y < 0)
							drag_offset_y = 0;
						else if (drag_offset_y > drag_info->height)
							drag_offset_y = drag_info->height;
						drag_offset_y -= lister->window->TopEdge;

						// Set font in drag rastport
						SetFont(&drag_info->drag_rp, lister->text_area.rast.Font);
						SetDrMd(&drag_info->drag_rp, JAM1);

						// Go through twice, once for mask, once for real image
						o_name_ptr = name_ptr;
						for (a = 0; a < 2; a++)
						{
							// Go through lines
							for (l = 0, entry = first_entry, y = drag_info->drag_rp.TxBaseline, name_ptr = o_name_ptr;
								 l < height;
								 l++, y += drag_info->drag_rp.TxHeight)
							{
								short type;

								// Get type
								if (entry)
									type = entry->de_Node.dn_Type;
								else
									type = drag_type;

								// Doing mask?
								if (a == 0)
								{
									// If valid name, fill in mask bit
									if (name_ptr)
									{
										// Clear background
										SetAPen(&drag_info->drag_rp, 1);
										RectFill(&drag_info->drag_rp,
												 0,
												 y - drag_info->drag_rp.TxBaseline,
												 drag_info->width - 1,
												 (y - drag_info->drag_rp.TxBaseline) + drag_info->drag_rp.TxHeight - 1);
									}
								}

								// Otherwise, real image
								else
								{
									// Set pen for background
									if (name_ptr)
									{
										// Get background of drag image
										SetAPen(&drag_info->drag_rp,
												lister_colour_pen(lister,
																  (type <= ENTRY_DEVICE) ? ENVCOL_FILES_SEL : ENVCOL_DIRS_SEL,
																  1));
									}

									// No name, clear area
									else
										SetAPen(&drag_info->drag_rp, GUI->draw_info->dri_Pens[BACKGROUNDPEN]);

									// Clear background
									RectFill(&drag_info->drag_rp,
											 0,
											 y - drag_info->drag_rp.TxBaseline,
											 drag_info->width - 1,
											 (y - drag_info->drag_rp.TxBaseline) + drag_info->drag_rp.TxHeight - 1);

									// Draw text
									if (name_ptr)
									{
										// Get pen to display file with
										SetAPen(&drag_info->drag_rp,
												lister_colour_pen(lister,
																  (type <= ENTRY_DEVICE) ? ENVCOL_FILES_SEL : ENVCOL_DIRS_SEL,
																  0));

										// Display text
										Move(&drag_info->drag_rp, 1, y);
										Text(&drag_info->drag_rp, name_ptr, strlen(name_ptr));
									}
								}

								// No more to do?
								if (!multi_drag || l >= height - 1)
									break;

								// Get next entry
								if (!(entry = (DirEntry *)entry->de_Node.dn_Succ) || !entry->de_Node.dn_Succ)
									break;

								// Selected?
								if (entry->de_Flags & ENTF_SELECTED)
									name_ptr = entry->de_Node.dn_Name;
								else
									name_ptr = 0;
							}

							// Build the drag shadow mask
							if (a == 0)
								GetDragMask(drag_info);
						}

						// Display the drag image
						ShowDragImage(drag_info, mouse_x - drag_offset_x, mouse_y - drag_offset_y);

						// Set drag status
						drag_status = DRAG_DOING;

						// Set new timeout value
						timer_micro = 500000;
					}
				}
			}

			// Check break flag
			if (break_flag)
				break;
		}

		// Check break flag
		if (break_flag)
			break;

		// Get mouse position
		mouse_x = lister->window->MouseX;
		mouse_y = lister->window->MouseY;

		// Check we're not dragging
		if (drag_status != DRAG_DOING)
		{
			// Mouse moved above window?
			if (mouse_y < lister->text_area.rect.MinY)
			{
				// Already at top of list?
				if (buffer->buf_VertOffset == 0)
					scroll_dir = SCROLL_NO;

				// Otherwise, scroll up
				else
				{
					// Not previously scrolling up?
					if (scroll_dir != SCROLL_UP)
					{
						short l;

						// If a device list, select top line
						if (device_list)
						{
							if (old_line != 0)
							{
								// Deselect old line
								select_default_state(lister, old_line, 0);

								// Select top line
								select_default_state(lister, 0, 1);
							}
						}

						// Normal files; select entries from old line to top
						else
						{
							for (l = old_line; l >= 0; l--)
								select_default_state(lister, l, state);

							// Update info display
							select_show_info(lister, 1);
						}

						// Store new line
						old_line = 0;

						// Set scroll direction
						scroll_dir = SCROLL_UP;

						// Set new timer value
						timer_micro = 20000;
					}

					// Were previously scrolling
					else
					{
						// If a device list, deselect the old line
						if (device_list)
							select_default_state(lister, 0, 0);

						// Select top line
						select_default_state(lister, -1, state);

						// Scroll window
						lister_scroll(lister, 0, -1);

						// Update info display
						if (!device_list)
							select_show_info(lister, 1);
					}
				}
			}

			// Otherwise, mouse moved below window?
			else if (mouse_y >= lister->text_area.rect.MaxY)
			{
				// Already at bottom of list?
				if (buffer->buf_VertOffset + lister->text_height >= buffer->buf_TotalEntries[0])
					scroll_dir = SCROLL_NO;

				// Otherwise, scroll down
				else
				{
					// Not previously scrolling down?
					if (scroll_dir != SCROLL_DOWN)
					{
						short l;

						// If a device list, select bottom line
						if (device_list)
						{
							if (old_line != lister->text_height - 1)
							{
								// Deselect old line
								select_default_state(lister, old_line, 0);

								// Select bottom line
								select_default_state(lister, lister->text_height - 1, 1);
							}
						}

						// Normal files; select entries from old line to bottom
						else
						{
							for (l = old_line; l < lister->text_height; l++)
								select_default_state(lister, l, state);

							// Update info display
							select_show_info(lister, 1);
						}

						// Store new line
						old_line = lister->text_height - 1;

						// Set scroll direction
						scroll_dir = SCROLL_DOWN;

						// Set new timer value
						timer_micro = 20000;
					}

					// Were previously scrolling
					else
					{
						// If a device list, deselect the old line
						if (device_list)
							select_default_state(lister, lister->text_height - 1, 0);

						// Select bottom line
						select_default_state(lister, lister->text_height, state);

						// Scroll window
						lister_scroll(lister, 0, 1);

						// Update info display
						if (!device_list)
							select_show_info(lister, 1);
					}
				}
			}

			// Otherwise, reset timer value
			else
				timer_micro = 500000;

			// Save old mouse position
			old_mouse_x = mouse_x;
			old_mouse_y = mouse_y;
		}

		// Wait for a message
		Wait(1 << lister->window->UserPort->mp_SigBit | 1 << lister->timer_port->mp_SigBit);
	}

	// Free drag image
	if (drag_info)
	{
		// Free image
		FreeDragInfo(drag_info);

		// Clear any highlights
		lister_test_drag(lister, 0, -1, -1, &over_lister);
	}

	// Abort timers
	StopTimer(lister->scroll_timer);
	StopTimer(lister->edit_timer);

	// Turn mouse move reporting off
	lister->window->Flags &= ~WFLG_REPORTMOUSE;

	// Turn ticks off
	ModifyIDCMP(lister->window, oldidcmp);

	// Save selected entry
	entry = lister->selected_entry;

	// Was this a device list?
	if (device_list)
	{
		// Deselect device entry
		select_default_state(lister, old_line, 0);
	}

	// Were we dragging?
	if (drag_status == DRAG_DOING)
	{
		// Check drag wasn't aborted
		if (drag_ok == DRAG_OK)
		{
			struct Window *window = 0;
			struct Layer *layer;
			DOpusAppMessage *msg;
			struct AppWindow *app_window = 0;
			BOOL ok = 1, refresh = 0;
			unsigned long over_parent = 0;

			// Get current mouse position
			mouse_x = lister->window->WScreen->MouseX;
			mouse_y = lister->window->WScreen->MouseY;

			// Forbid while we do this
			Forbid();

			// Find which layer we dropped it on
			if ((layer = WhichLayer(&lister->window->WScreen->LayerInfo, mouse_x, mouse_y)))
			{
				// Does layer have a window?
				if ((window = layer->Window))
				{
					// Is it over the 'parent' area of this lister?
					if (window == lister->window && mouse_x - lister->window->LeftEdge >= 0 &&
						mouse_x - lister->window->LeftEdge < lister->window->BorderLeft &&
						mouse_y - lister->window->TopEdge >= lister->window->BorderTop &&
						mouse_y - lister->window->TopEdge <= lister->window->Height - lister->window->BorderBottom)
					{
						// Set flag to say we dropped in 'parent' area
						over_parent = DROPF_PARENT;
					}

					// Check it's not ours, unless we dropped onto a directory or
					// onto the other panel of a dual lister
					if (window != lister->window || over_entry || over_parent ||
						(lister_dual_one_window(lister) &&
						 lister_dual_side_at(lister, mouse_x - window->LeftEdge, mouse_y - window->TopEdge) ==
							 lister_dual_dest_index(lister)))
					{
						// See if window is in list of AppWindows
						if (!(app_window = WB_FindAppWindow(window)))
							ok = 0;
					}
				}
				else
					ok = 0;
			}
			else
				ok = 0;

			// Can't drop here?
			if (!ok)
				DisplayBeep(lister->window->WScreen);

			// Do we have an appwindow?
			else if (app_window)
			{
				struct MsgPort *port;

				// Allocate AppMessage
				if ((msg = alloc_appmsg_files(entry, lister->cur_buffer, multi_drag)))
				{
					// Get window information
					port = WB_AppWindowData(app_window, &msg->da_Msg.am_ID, &msg->da_Msg.am_UserData);

					// Fill out AppMessage info
					msg->da_Msg.am_Type = MTYPE_APPWINDOW;
					msg->da_Msg.am_MouseX = mouse_x - ((window) ? window->LeftEdge : 0);
					msg->da_Msg.am_MouseY = mouse_y - ((window) ? window->TopEdge : 0);

					// Set source lister pointer
					set_appmsg_data(msg, (IPTR)lister, over_parent, (IPTR)over_entry);

					// Custom handler assigned?
					if (lister->cur_buffer->buf_CustomHandler[0])
					{
						BOOL put = 0;
						Lister *dest = 0;
						char *dest_path = 0, *pathname;
						BackdropInfo *info = 0;
						ULONG id;

						// Allocate path name
						if ((pathname = AllocVec(512, MEMF_CLEAR)))
						{
							// Is window a lister?
							if ((id = GetWindowID(window)) == WINDOW_LISTER)
							{
								// Get destination lister's handle
								if ((dest = (Lister *)IPCDATA(((IPCData *)GetWindowAppPort(window)))))
								{
									// If destination lister also has a custom handler
									if (dest->cur_buffer->buf_CustomHandler[0])
									{
										// Send message to destination
										put = 1;
									}

									// Get path
									strcpy(pathname, dest->cur_buffer->buf_Path);
									dest_path = pathname;

									// Add on sub-directory, or do parent
									if (over_entry)
										AddPart(pathname, over_entry->de_Node.dn_Name, 512);
									else if (over_parent)
										path_parent(pathname);

									// Is lister in icon mode?
									if (dest->flags & LISTERF_VIEW_ICONS)
									{
										// Get backdrop info
										info = dest->backdrop_info;
									}
								}
							}

							// Or the desktop?
							else if (id == WINDOW_BACKDROP)
							{
								// Set destination path
								dest_path = "desktop";

								// Get info pointer
								info = GUI->backdrop;
							}

							// Got icon backdrop?
							if (info)
							{
								BackdropObject *icon;

								// Lock backdrop list
								lock_listlock(&info->objects, 0);

								// See if we dropped on an object
								if ((icon = backdrop_get_object(info, msg->da_Msg.am_MouseX, msg->da_Msg.am_MouseY, 0)))
								{
									// Get path of icon
									if (desktop_icon_path(icon, pathname, 512, 0))
									{
										// Use this path
										dest_path = pathname;
									}
								}

								// Unlock backdrop list
								unlock_listlock(&info->objects);
							}

							// Send to rexx handler
							if (over_entry || over_parent)
								qual |= IEQUALIFIER_SUBDROP;
							rexx_custom_app_msg(msg, lister->cur_buffer, "dropfrom", dest, dest_path, qual);

							// Free message if not sending it
							if (!put)
							{
								ReplyAppMessage(msg);
								msg = 0;
							}

							// Free path
							FreeVec(pathname);
						}
					}

					// Send message
					if (msg)
						PutMsg(port, (struct Message *)msg);

					// Refresh the lister
					refresh = 1;
				}

				// FindAppWindow left us in (nested) Forbid()
				Permit();
			}

			// Unbusy
			Permit();

			// Need to refresh?
			if (refresh)
			{
				lister_refresh_display(lister, 0);
				select_show_info(lister, 1);
			}
		}
	}

	// Otherwise, if this was a device list
	else if (device_list)
	{
		// Fake double-click
		lister->seconds = 1;
		lister->micros = 0;
		lister->old_seconds = 1;
		lister->old_micros = 1;
		lister->last_selected_entry = entry;

		// Toggle entry
		select_toggle_entry(lister, old_line, qual, 0);
	}

	// Unlock buffer
	buffer_unlock(buffer);
	return retval;
}

#define POPUP_DELAY 100000

// Scroll with the right mouse button
// Called from the LISTER PROCESS
void select_rmb_scroll(Lister *lister, short x, short y)
{
	struct IntuiMessage *msg;
	char break_flag = 0;
	ULONG wait;
	short line, show_line = 0;
	long total_timer = 1000;
	DirEntry *entry = 0;

	// Initial time request
	StartTimer(lister->scroll_timer, 0, 1000);

	// Get wait bits
	wait = 1 << lister->window->UserPort->mp_SigBit | 1 << lister->timer_port->mp_SigBit;

	// Allowed to do popup?
	if (environment->env->lister_options & LISTEROPTF_POPUP && !(lister->flags & LISTERF_BUSY))
	{
		// Get line clicked on
		show_line = UDivMod32(y - lister->text_area.rect.MinY, lister->text_area.font->tf_YSize);

		// Not out of display?
		if (show_line < 0 || show_line >= lister->text_height)
			line = -1;

		// Add top offset
		else
			line = show_line + lister->cur_buffer->buf_VertOffset;

		// Valid line?
		if (line >= 0 && line < lister->cur_buffer->buf_TotalEntries[0])
		{
			// Find entry
			entry = get_entry_ord(&lister->cur_buffer->entry_list, line);
		}

		// Got an entry?
		if (entry)
		{
			/*
						// Don't do assigns or caches
						if (entry->de_Node.dn_Type==ENTRY_DEVICE && entry->de_Size!=DLT_DEVICE)
							entry=0;
			*/

			// Don't do devices
			if (entry->de_Node.dn_Type == ENTRY_DEVICE)
				entry = 0;

			// Ok to use
			else
			{
				// Set highlight flag
				entry->de_Flags |= ENTF_HIGHLIGHT;

				// Show entry
				display_entry(entry, lister, show_line);
			}
		}
	}

	// Loop forever
	FOREVER
	{
		// Any intuition messages?
		while ((msg = (struct IntuiMessage *)GetMsg(lister->window->UserPort)))
		{
			// Right button up?
			if ((msg->Class == IDCMP_MOUSEBUTTONS && msg->Code == MENUUP) || msg->Class == IDCMP_INACTIVEWINDOW)
				break_flag = 1;

			// Refresh?
			else if (msg->Class == IDCMP_REFRESHWINDOW)
				lister_refresh(lister, LREFRESH_REFRESH);

			/*
						// Left button down triggers popup
						if (msg->Class==IDCMP_MOUSEBUTTONS && msg->Code==SELECTDOWN)
							total_timer=POPUP_DELAY+1;
			*/

			// Reply message
			ReplyMsg((struct Message *)msg);
		}

		// Check break flag
		if (break_flag)
			break;

		// Timer returned?
		if (CheckTimer(lister->scroll_timer))
		{
			short mouse_x, mouse_y;
			unsigned short delay = 100;

			// Get current mouse position
			mouse_x = lister->window->MouseX;
			mouse_y = lister->window->MouseY;

			// Got an entry?
			if (entry)
			{
				// Have we moved too far?
				if (mouse_x < x - 1 || mouse_x > x + 1 || mouse_y < y - 1 || mouse_y > y + 1)
				{
					// Clear highlight flag
					entry->de_Flags &= ~ENTF_HIGHLIGHT;

					// Show entry
					display_entry(entry, lister, show_line);
					entry = 0;
				}

				// Have we waited long enough?
				else if (total_timer > POPUP_DELAY)
				{
					DirEntry *pass_entry = 0;

					// Initialise buffer for popup kludge
					lister->work_buffer[0] = 0;
					lister->work_buffer[1] = 0;

					// Does entry have its own menus?
					if (entry->de_Flags & ENTF_USER_MENU)
					{
						// Get entry to pass
						pass_entry = entry;
					}

					// Otherwise
					else
					{
						// Device?
						if (entry->de_Node.dn_Type == ENTRY_DEVICE)
						{
							// Get pathname
							strcpy(lister->work_buffer, entry->de_Node.dn_Name);
						}

						// File
						else
						{
							// Build full filename
							strcpy(lister->work_buffer, lister->cur_buffer->buf_Path);
							AddPart(lister->work_buffer, entry->de_Node.dn_Name, 512);
						}
					}

					// Do the popup
					if (backdrop_popup(
							lister->backdrop_info, mouse_x, mouse_y, 0, BPOPF_ICONS, lister->work_buffer, pass_entry))
					{
						UWORD id = 0;

						// Got a code?
						if (!lister->work_buffer[0])
						{
							// Get code
							id = *((UWORD *)(lister->work_buffer + 2));

							// Custom function?
							if (lister->work_buffer[1])
							{
								// Custom handler assigned?
								if (lister->cur_buffer->buf_CustomHandler[0])
								{
									// Send message
									rexx_handler_msg(0,
													 lister->cur_buffer,
													 RXMF_WARN,
													 HA_String,
													 0,
													 app_commands[REXXAPPCMD_MENU],
													 HA_Value,
													 1,
													 lister,
													 HA_String,
													 2,
													 entry->de_Node.dn_Name,
													 HA_Value,
													 3,
													 id,
													 HA_String,
													 4,
													 "file",
													 HA_Value,
													 5,
													 GetTagData(DE_UserData, 0, entry->de_Tags),
													 TAG_END);
								}
								break;
							}
						}

						// Open with existing item?
						if (id >= MENU_OPEN_WITH_BASE && id <= MENU_OPEN_WITH_MAX)
						{
							char *name;

							// Allocate name
							if ((name = AllocVec(512, 0)))
							{
								char task_name[30];

								// Build name
								strcpy(name, lister->cur_buffer->buf_Path);
								AddPart(name, entry->de_Node.dn_Name, 512);

								// Build task name (kludgy!)
								lsprintf(task_name, "dopus_%ld_open_with", id - MENU_OPEN_WITH_BASE);

								// Do 'open with' on this entry
								if (!(misc_startup(task_name, MENU_FILE_OPEN_WITH, lister->window, name, 0)))
									FreeVec(name);
							}
						}

						// Look at ID
						else
							switch (id)
							{
							// Open
							case MENU_ICON_OPEN:
							case MENU_OPEN_NEW_WINDOW:

								// Do double-click on this entry
								file_doubleclick(lister,
												 entry,
												 (entry->de_Node.dn_Type == ENTRY_DEVICE || id == MENU_OPEN_NEW_WINDOW)
													 ? IEQUALIFIER_LSHIFT
													 : 0);
								break;

							// Open with
							case MENU_OPEN_WITH: {
								char *name;

								// Allocate name
								if ((name = AllocVec(512, 0)))
								{
									// Build name
									strcpy(name, lister->cur_buffer->buf_Path);
									AddPart(name, entry->de_Node.dn_Name, 512);

									// Do 'open with' on this entry
									if (!(misc_startup(
											"dopus_open_with", MENU_FILE_OPEN_WITH, lister->window, name, 0)))
										FreeVec(name);
								}
							}
							break;

							// Info
							case MENU_ICON_INFO:

								// Do information
								file_run_function(lister, entry, def_function_iconinfo, 0, 0);
								break;

							// Rename
							case MENU_ICON_RENAME:

								// Do rename
								file_run_function(lister, entry, def_function_rename, 0, 0);
								break;

							// Delete
							case MENU_ICON_DELETE:

								// Do delete
								file_run_function(lister, entry, def_function_delete, 0, 0);
								break;

							// Leave Out
							case MENU_ICON_LEAVE_OUT:

								// Do leave out
								file_run_function(lister, entry, def_function_leaveout, 0, 0);
								break;

							// Copy
							case MENU_ICON_COPY_TO:
							case MENU_ICON_COPY_DESKTOP:
							case MENU_ICON_COPY_OTHER:

								// Copy dest to other part of buffer
								strcpy(lister->work_buffer + 512, lister->work_buffer + 4);

								// Do copy
								file_run_function(lister,
												  entry,
												  def_function_copy,
												  lister->work_buffer + 512,
												  (id == MENU_ICON_COPY_DESKTOP) ? FUNCF_RESCAN_DESKTOP : 0);
								break;

							// Copy to script
							case MENU_ICON_COPY_TO_SCRIPT:

								// Get rexx function to run
								lsprintf(lister->work_buffer + 512,
										 "%s \"%s",
										 lister->work_buffer + 4,
										 lister->cur_buffer->buf_Path);
								AddPart(lister->work_buffer + 512, entry->de_Node.dn_Name, 512);
								lsprintf(lister->work_buffer + 512 + strlen(lister->work_buffer + 512),
										 "\" %s %ld",
										 GUI->rexx_port_name,
										 lister);

								// Run rexx thing
								rexx_send_command(lister->work_buffer + 512, FALSE);
								break;

							// Disk copy
							case MENU_ICON_COPY:

								// Launch diskcopy
								function_launch(FUNCTION_RUN_FUNCTION,
												def_function_diskcopy,
												0,
												0,
												0,
												0,
												0,
												0,
												BuildArgArray(entry->de_Node.dn_Name, 0),
												0,
												0);
								break;
							}
					}

					// Break out
					break;
				}

				// Long delay
				else
					delay = 20000;

				// Increment timer count
				total_timer += delay;
			}

			// We can scroll
			else
			{
				// Scroll left?
				if (mouse_x < lister->scroll_border.MinX)
					lister_scroll(lister, -1, 0);

				// Scroll right?
				else if (mouse_x > lister->scroll_border.MaxX)
					lister_scroll(lister, 1, 0);

				// Scroll up?
				else if (mouse_y < lister->scroll_border.MinY)
				{
					lister_scroll(lister, 0, -1);

					// Slow?
					if (mouse_y > lister->scroll_border.MinY - (lister->text_area.font->tf_YSize * 4))
						delay = 20000;
				}

				// Scroll down?
				else if (mouse_y > lister->scroll_border.MaxY)
				{
					lister_scroll(lister, 0, 1);

					// Slow?
					if (mouse_y < lister->scroll_border.MaxY + (lister->text_area.font->tf_YSize * 4))
						delay = 20000;
				}
			}

			// Send off new timer request
			StartTimer(lister->scroll_timer, 0, delay);
			continue;
		}

		// Wait for a message
		Wait(wait);
	}

	// Abort timer
	StopTimer(lister->scroll_timer);

	// Still got an entry?
	if (entry)
	{
		// Clear highlight flag
		entry->de_Flags &= ~ENTF_HIGHLIGHT;

		// Show entry
		display_entry(entry, lister, show_line);
	}
}

// Toggle selection of an entry (specified by position in visible display)
// Returns new selection status
// Called from the LISTER PROCESS
int select_toggle_entry(Lister *lister, int position, UWORD qual, BOOL show)
{
	short num;
	BOOL double_click;
	DirEntry *entry;

	if (!lister)
		return 0;

	// Get entry number
	num = position + lister->cur_buffer->buf_VertOffset;

	// Check valid entry
	if (position >= lister->text_height || num >= lister->cur_buffer->buf_TotalEntries[0])
		return SELECT_OFF;

	// Find entry
	if (!(entry = get_entry_ord(&lister->cur_buffer->entry_list, num)))
	{
		// Invalid
		lister->selected_entry = 0;
		return 0;
	}

	// Test for double-click
	if (lister->last_selected_entry == entry)
	{
		double_click = DoubleClick(lister->old_seconds, lister->old_micros, lister->seconds, lister->micros);
	}
	else
		double_click = 0;

	// Store double-click time
	lister->old_seconds = lister->seconds;
	lister->old_micros = lister->micros;

	// Remember this entry
	lister->selected_entry = entry;

	// If we double-clicked, and entry was not selected, leave it that way
	// otherwise, change selection status
	if (!double_click || (entry->de_Flags & ENTF_SELECTED))
	{
		// Toggle state
		entry->de_Flags ^= ENTF_SELECTED;

		// Update lister to reflect new state
		if (show)
			entry_change_state(lister, entry, position);
	}

	// If not double-clicked, store entry
	if (!double_click)
		lister->last_selected_entry = entry;

	// Otherwise, double-clicked on?
	else if (double_click && lister->last_selected_entry == entry)
	{
		// Do double-click on this entry
		file_doubleclick(lister, entry, qual);

		// Reset double-click, return "special" code
		lister->old_seconds = 0;
		return SELECT_SPECIAL;
	}

	// Return the new state
	return (entry->de_Flags & ENTF_SELECTED);
}

// Do double-click on a file
void file_doubleclick(Lister *lister, DirEntry *entry, UWORD qual)
{
	// Look at type of entry
	switch (ENTRYTYPE(entry->de_Node.dn_Type))
	{
	// Device or directory
	case ENTRY_DEVICE:
	case ENTRY_DIRECTORY:

		// Custom handler assigned?
		if (lister->cur_buffer->buf_CustomHandler[0])
		{
			// Send double-click message
			rexx_handler_msg(0,
							 lister->cur_buffer,
							 RXMF_WARN,
							 HA_String,
							 0,
							 "doubleclick",
							 HA_Value,
							 1,
							 lister,
							 HA_String,
							 2,
							 entry->de_Node.dn_Name,
							 HA_Value,
							 5,
							 GetTagData(DE_UserData, 0, entry->de_Tags),
							 HA_Qualifier,
							 6,
							 qual,
							 TAG_END);
		}

		// Otherwise
		else
		{
			Cfg_Filetype *type;
			char *dir;
			char *pathname;
			Lister *new_lister = 0;
			short action;

			if (!(pathname = AllocVec(512, 0)))
				break;

			// Get current directory
			dir = lister->cur_buffer->buf_Path;

			// If this is a device, replace pathname
			if (entry->de_Node.dn_Type == ENTRY_DEVICE)
			{
				if (entry->de_SubType == SUBENTRY_BUFFER)
				{
					char *ptr;

					if (!(ptr = (char *)GetTagData(DE_Comment, 0, entry->de_Tags)))
						ptr = (char *)GetTagData(DE_DisplayString, 0, entry->de_Tags);
					if (ptr)
						strcpy(pathname, ptr);
					else
						*pathname = 0;

					// Check for custom buffer
					if (lister_select_cache(lister, (DirBuffer *)entry->de_UserData))
					{
						// Was a custom buffer so we don't do anything
						*pathname = 0;
					}
				}
				else
					strcpy(pathname, entry->de_Node.dn_Name);
			}

			// Otherwise, tack directory name onto pathname
			else
			{
				strcpy(pathname, dir);
				AddPart(pathname, entry->de_Node.dn_Name, 512);
				AddPart(pathname, "", 512);
			}

			// Break if no path
			if (!*pathname)
			{
				FreeVec(pathname);
				break;
			}

			// Get filetype action
			if (qual & IEQUALIFIER_CONTROL)
				action = FTTYPE_CTRL_DOUBLECLICK;
			else if (qual & (IEQUALIFIER_LALT | IEQUALIFIER_RALT))
				action = FTTYPE_ALT_DOUBLECLICK;
			else
				action = FTTYPE_DOUBLE_CLICK;

			// Try to match filetype
			if (entry->de_Node.dn_Type != ENTRY_DEVICE && (type = filetype_identify(pathname, action, 0, 0)) &&
				type != default_filetype)
			{
				// Do filetype action on directory
				function_launch(FUNCTION_FILETYPE,
								0,
								action,
								0,
								lister,
								0,
								lister->cur_buffer->buf_Path,
								0,
								BuildArgArray(pathname, 0),
								0,
								0);
				FreeVec(pathname);
				break;
			}

			// Open a new window?
			if (qual & IEQUAL_ANYSHIFT)
			{
				// Read into new lister
				new_lister = read_new_lister(pathname, lister, qual);
			}

			// Need to read in current window?
			if (!new_lister)
			{
				/*
									// Move to the next buffer
									lister_buffer_find_empty(
										lister,
										pathname,
										&lister->cur_buffer->buf_VolumeDate);
				*/

				// Read directory
				read_directory(lister, pathname, GETDIRF_CANMOVEEMPTY | GETDIRF_CANCHECKBUFS);
			}

			FreeVec(pathname);
		}
		break;

	// Ordinary file
	case ENTRY_FILE:

		// Custom handler assigned?
		if (lister->cur_buffer->buf_CustomHandler[0])
		{
			// Send double-click message
			rexx_handler_msg(0,
							 lister->cur_buffer,
							 RXMF_WARN,
							 HA_String,
							 0,
							 "doubleclick",
							 HA_Value,
							 1,
							 lister,
							 HA_String,
							 2,
							 entry->de_Node.dn_Name,
							 HA_Value,
							 5,
							 GetTagData(DE_UserData, 0, entry->de_Tags),
							 HA_Qualifier,
							 6,
							 qual,
							 TAG_END);
		}
		else
		{
			char *pathname;
			ULONG flags = 0;
			short action;

			if (!(pathname = AllocVec(512, 0)))
				break;

			// Build full pathname
			strcpy(pathname, lister->cur_buffer->buf_Path);
			AddPart(pathname, entry->de_Node.dn_Name, 512);

			// Is the lister in name mode?
			if (!(lister->flags & LISTERF_VIEW_ICONS))
			{
				// Unless shift is down, run without looking at icons
				if (!(qual & IEQUAL_ANYSHIFT))
					flags = FUNCF_RUN_NO_ICONS;
			}

			// Get filetype action
			if (qual & IEQUALIFIER_CONTROL)
				action = FTTYPE_CTRL_DOUBLECLICK;
			else if (qual & (IEQUALIFIER_LALT | IEQUALIFIER_RALT))
				action = FTTYPE_ALT_DOUBLECLICK;
			else
				action = FTTYPE_DOUBLE_CLICK;

			// Do filetype action on a file
			function_launch(FUNCTION_FILETYPE,
							0,
							(qual << 16) | action,
							flags,
							lister,
							0,
							lister->cur_buffer->buf_Path,
							0,
							BuildArgArray(pathname, 0),
							0,
							0);
			FreeVec(pathname);
		}
		break;
	}
}

// An entry has just changed state
void entry_change_state(Lister *lister, DirEntry *entry, short position)
{
	// Update status information
	select_update_info(entry, lister->cur_buffer);

	// If we're not iconified, update display
	if (lister_valid_window(lister))
	{
		// Display entry
		display_entry(entry, lister, position);

		// If auto icon select is on, select icon
		select_auto_icon_select(entry, lister);

		// Update selection info
		select_show_info(lister, 1);
	}
}

// Unselect an entry
// Returns 1 is display was updated
// Called from the LISTER PROCESS
int select_off_entry(Lister *lister, DirEntry *entry)
{
	int position;
	DirEntry *temp;
	DirBuffer *buffer;

	// Check valid window, and that entry is actually selected
	if (!lister || !(entry->de_Flags & ENTF_SELECTED))
		return 0;

	// Make sure entry really is in this window
	buffer = lister->cur_buffer;
	for (temp = (DirEntry *)buffer->entry_list.mlh_Head, position = 0; temp != entry && temp->de_Node.dn_Succ;
		 ++position, temp = (DirEntry *)temp->de_Node.dn_Succ)
		;

	// Was entry found?
	if (!temp)
		return 0;

	// Deselect the entry
	deselect_entry(buffer, entry);

	// If entry is not visible in window, return
	if (position < buffer->buf_VertOffset || position >= (buffer->buf_VertOffset + lister->text_height))
		return 0;

	// If not iconified, update display
	if (lister_valid_window(lister))
		display_entry(entry, lister, position - buffer->buf_VertOffset);

	return 1;
}

// Deselect an entry
void deselect_entry(DirBuffer *buffer, DirEntry *entry)
{
	// Make sure entry is selected
	if (!(entry->de_Flags & ENTF_SELECTED))
		return;

	// Look at entry type
	switch (ENTRYTYPE(entry->de_Node.dn_Type))
	{
	// Directory
	case ENTRY_DIRECTORY:

		// Decrement selected directory and byte count
		--buffer->buf_SelectedDirs[0];
		buffer->buf_SelectedBytes[0] -= entry->de_Size;
		break;

	// File
	case ENTRY_FILE:
		// Decrement selected file and byte count
		--buffer->buf_SelectedFiles[0];
		buffer->buf_SelectedBytes[0] -= entry->de_Size;
		break;
	}

	// Clear selection flag
	entry->de_Flags &= ~ENTF_SELECTED;
}

// Change the selection of an entry (specified by offset in window) to a certain state
// Called from the LISTER PROCESS
short select_default_state(Lister *lister, int offset, int sel_state)
{
	register short position;
	register DirEntry *entry;

	// Check valid offset
	if (!lister || offset < -1 || offset > lister->text_height)
		return 0;

	// Get position in list
	position = offset + lister->cur_buffer->buf_VertOffset;

	// Valid position?
	if (position < 0 || position >= lister->cur_buffer->buf_TotalEntries[0])
		return 0;

	// Find entry in list
	if (!(entry = get_entry_ord(&lister->cur_buffer->entry_list, position)))
		return 0;

	// Valid entry?
	if (!entry->de_Node.dn_Succ)
		return 0;

	// If we're selecting this entry, remember it
	if (sel_state)
		lister->selected_entry = entry;
	else
		lister->selected_entry = 0;

	// If entry is not already in the desired state
	if ((entry->de_Flags & ENTF_SELECTED) != (sel_state & ENTF_SELECTED))
	{
		// Set new state and update selection information
		if (sel_state)
			entry->de_Flags |= ENTF_SELECTED;
		else
			entry->de_Flags &= ~ENTF_SELECTED;
		select_update_info(entry, lister->cur_buffer);

		// If a valid offset, display entry
		if (offset >= 0 && offset < lister->text_height)
			display_entry(entry, lister, offset);

		// Select icon (if necessary)
		if (environment->env->settings.icon_flags & ICONFLAG_AUTOSELECT)
			select_auto_icon_select(entry, lister);
		return 1;
	}

	return 0;
}

// Select an icon whose owner was selected
// Called from the LISTER PROCESS
void select_auto_icon_select(DirEntry *entry, Lister *lister)
{
	// If auto-icon select flag is on
	if (environment->env->settings.icon_flags & ICONFLAG_AUTOSELECT)
	{
		DirEntry *icon;
		long icon_pos;

		// Look for icon in list
		if ((icon = find_entry(&lister->cur_buffer->entry_list,
							   entry->de_Node.dn_Name,
							   &icon_pos,
							   (lister->cur_buffer->more_flags & DWF_CASE) | FINDENTRY_ICON)))
		{
			// If icon is not already the same state
			if ((icon->de_Flags & ENTF_SELECTED) != (entry->de_Flags & ENTF_SELECTED))
			{
				// Change icon state
				icon->de_Flags ^= ENTF_SELECTED;

				// Update selection information
				select_update_info(icon, lister->cur_buffer);

				// See if icon is currently visible
				if (icon_pos >= lister->cur_buffer->buf_VertOffset &&
					icon_pos < lister->cur_buffer->buf_VertOffset + lister->text_height)

					// Display icon
					display_entry(icon, lister, icon_pos - lister->cur_buffer->buf_VertOffset);
			}
		}
	}
}

// Set the state of all entries in a window
// Called from the LISTER PROCESS
void select_global_state(Lister *lister, int sel_state)
{
	DirEntry *entry;
	DirBuffer *buffer;

	// Get buffer
	if (!lister)
		return;
	buffer = lister->cur_buffer;

	// Lock buffer
	buffer_lock(buffer, FALSE);

	// Check list isn't custom
	if (!(list_is_custom(buffer)))
	{
		// Icon action mode?
		if (lister->flags & (LISTERF_VIEW_ICONS | LISTERF_ICON_ACTION))
		{
			BackdropObject *object;

			// Lock backdrop list
			lock_listlock(&lister->backdrop_info->objects, 0);

			// Go through objects
			for (object = (BackdropObject *)lister->backdrop_info->objects.list.lh_Head; object->node.ln_Succ;
				 object = (BackdropObject *)object->node.ln_Succ)
			{
				// Set state
				if (object->state != sel_state)
				{
					object->state = sel_state;
					object->flags |= BDOF_STATE_CHANGE;
				}
			}

			// Unlock backdrop list
			unlock_listlock(&lister->backdrop_info->objects);

			// Fix selection count
			backdrop_fix_count(lister->backdrop_info, 1);
		}

		// Normal file mode
		else
		{
			// Go through entries
			for (entry = (DirEntry *)buffer->entry_list.mlh_Head; entry->de_Node.dn_Succ;
				 entry = (DirEntry *)entry->de_Node.dn_Succ)
			{
				// Set selection state
				if (sel_state)
					entry->de_Flags |= ENTF_SELECTED;
				else
					entry->de_Flags &= ~ENTF_SELECTED;
			}

			// Set selected counts
			if (sel_state)
			{
				buffer->buf_SelectedFiles[0] = buffer->buf_TotalFiles[0];
				buffer->buf_SelectedDirs[0] = buffer->buf_TotalDirs[0];
				buffer->buf_SelectedBytes[0] = buffer->buf_TotalBytes[0];
			}
			else
			{
				buffer->buf_SelectedFiles[0] = 0;
				buffer->buf_SelectedDirs[0] = 0;
				buffer->buf_SelectedBytes[0] = 0;
			}
		}

		// Show selection information
		select_show_info(lister, 1);
	}

	// Unlock buffer
	buffer_unlock(buffer);
}

// Toggle the state of all entries in a window
// Called from the LISTER PROCESS
void select_global_toggle(Lister *lister)
{
	DirEntry *entry;
	DirBuffer *buffer;

	// Get buffer
	if (!lister)
		return;
	buffer = lister->cur_buffer;

	/* ACTION_CHANGE */
	// Icon action mode?
	if (lister->flags & (LISTERF_VIEW_ICONS | LISTERF_ICON_ACTION))
	{
		BackdropObject *object;

		// Lock backdrop list
		lock_listlock(&lister->backdrop_info->objects, 0);

		// Go through objects
		for (object = (BackdropObject *)lister->backdrop_info->objects.list.lh_Head; object->node.ln_Succ;
			 object = (BackdropObject *)object->node.ln_Succ)
		{
			// Toggle state
			object->state = (object->state) ? 0 : 1;
			object->flags |= BDOF_STATE_CHANGE;
		}

		// Unlock backdrop list
		unlock_listlock(&lister->backdrop_info->objects);

		// Fix selection count
		backdrop_fix_count(lister->backdrop_info, 1);
	}

	// Normal files
	else
	{
		// Go through entries
		for (entry = (DirEntry *)buffer->entry_list.mlh_Head; entry->de_Node.dn_Succ;
			 entry = (DirEntry *)entry->de_Node.dn_Succ)
		{
			// Look at entry type
			switch (ENTRYTYPE(entry->de_Node.dn_Type))
			{
			// File
			case ENTRY_FILE:
				if (entry->de_Flags & ENTF_SELECTED)
				{
					buffer->buf_SelectedBytes[0] -= entry->de_Size;
					--buffer->buf_SelectedFiles[0];
					entry->de_Flags &= ~ENTF_SELECTED;
				}
				else
				{
					buffer->buf_SelectedBytes[0] += entry->de_Size;
					++buffer->buf_SelectedFiles[0];
					entry->de_Flags |= ENTF_SELECTED;
				}
				break;

			// Directory
			case ENTRY_DIRECTORY:
				if (entry->de_Flags & ENTF_SELECTED)
				{
					buffer->buf_SelectedBytes[0] -= entry->de_Size;
					--buffer->buf_SelectedDirs[0];
					entry->de_Flags &= ~ENTF_SELECTED;
				}
				else
				{
					buffer->buf_SelectedBytes[0] += entry->de_Size;
					++buffer->buf_SelectedDirs[0];
					entry->de_Flags |= ENTF_SELECTED;
				}
				break;
			}
		}
	}

	// Show selection information
	select_show_info(lister, 1);
}

// Update selection counts when selecting an entry
// Called from the LISTER PROCESS
void select_update_info(DirEntry *entry, DirBuffer *buffer)
{
	// Look at entry type
	switch (ENTRYTYPE(entry->de_Node.dn_Type))
	{
	// Directory
	case ENTRY_DIRECTORY:

		// If entry is selected, increment counts
		if (entry->de_Flags & ENTF_SELECTED)
		{
			buffer->buf_SelectedBytes[0] += entry->de_Size;
			++buffer->buf_SelectedDirs[0];
		}

		// Otherwise, decrement counts
		else
		{
			buffer->buf_SelectedBytes[0] -= entry->de_Size;
			--buffer->buf_SelectedDirs[0];
		}
		break;

	// File
	case ENTRY_FILE:

		// If entry is selected, increment counts
		if (entry->de_Flags & ENTF_SELECTED)
		{
			++buffer->buf_SelectedFiles[0];
			buffer->buf_SelectedBytes[0] += entry->de_Size;
		}

		// Otherwise, decrement counts
		else
		{
			--buffer->buf_SelectedFiles[0];
			buffer->buf_SelectedBytes[0] -= entry->de_Size;
		}
		break;
	}
}

// Make sure that a file is deselected
// Called from the LISTER PROCESS
void select_check_off(DirEntry *entry, Lister *lister)
{
	DirEntry *temp;

	// Zero file pointers
	lister->last_selected_entry = 0;

	// Look for file in current list
	for (temp = (DirEntry *)lister->cur_buffer->entry_list.mlh_Head; temp->de_Node.dn_Succ && temp != entry;
		 temp = (DirEntry *)temp->de_Node.dn_Succ)
		;

	// If found, and selected, deselect it
	if (temp && temp->de_Flags & ENTF_SELECTED)
		select_off_entry(lister, temp);
}

// Wildcard selection
// Called from the LISTER PROCESS
void select_global_wild(Lister *lister, SelectData *data, PathList *dest_list)
{
	BackdropObject *object = 0;
	DirEntry *entry = 0, *first_entry = 0;
	DirBuffer *buffer;
	char name_match[120];
	struct DateStamp date_match[2];
	unsigned short prot_match[2];
	short include = ENTF_SELECTED, type = SELECT_ENTRY_BOTH;
	BOOL get_next = 0;

	// Get buffer
	if (!lister)
		return;
	buffer = lister->cur_buffer;

	// Parse name pattern
	ParsePatternNoCase(data->name, name_match, 120);

	// Parse date strings
	date_match[0].ds_Days = 0;
	date_match[0].ds_Minute = 0;
	date_match[0].ds_Tick = 0;
	if (data->date_from[0])
		DateFromStringsNew(data->date_from, "00:00:00", &date_match[0], environment->env->settings.date_format);
	DateStamp(&date_match[1]);
	if (data->date_to[0])
		DateFromStringsNew(data->date_to, "23:59:59", &date_match[1], environment->env->settings.date_format);
	else if (data->date_from[0])
		DateFromStringsNew(data->date_from, "23:59:59", &date_match[1], environment->env->settings.date_format);

	// Get protection matches
	prot_match[0] = (data->bits >> 8) & 0xff;
	prot_match[1] = data->bits & 0xff;

	// Complex?
	if (data->type == SELECT_COMPLEX)
	{
		if (data->include == SELECT_EXCLUDE)
			include = 0;
		type = data->entry_type;
	}

	/* ACTION_CHANGE */
	// Icon action mode?
	if (lister->flags & (LISTERF_VIEW_ICONS | LISTERF_ICON_ACTION))
	{
		// Lock backdrop list
		lock_listlock(&lister->backdrop_info->objects, 0);

		// Get first
		object = (BackdropObject *)lister->backdrop_info->objects.list.lh_Head;
	}

	// Get first file
	else
		entry = (DirEntry *)buffer->entry_list.mlh_Head;

	// Go through entries
	while ((entry && entry->de_Node.dn_Succ) || (object && object->node.ln_Succ))
	{
		BOOL match;
		short entry_type, sel;
		char *entry_name;
		struct DateStamp *entry_date;

		// Want to get the next one?
		if (get_next)
		{
			// Get next entry
			if (entry)
				entry = (DirEntry *)entry->de_Node.dn_Succ;
			else
				object = (BackdropObject *)object->node.ln_Succ;

			// Clear flag and loop
			get_next = 0;
			continue;
		}
		else
			get_next = 1;

		// Selected?
		if (entry)
			sel = (entry->de_Flags & ENTF_SELECTED) ? 1 : 0;
		else
			sel = object->state;

		// Get type
		if (entry)
			entry_type = ENTRYTYPE(entry->de_Node.dn_Type);
		else
			entry_type = (object->icon->do_Type == WBDRAWER || object->icon->do_Type == WBGARBAGE) ? ENTRY_DIRECTORY
																								   : ENTRY_FILE;

		// Get name and date
		entry_name = (entry) ? entry->de_Node.dn_Name : object->name;
		entry_date = (entry) ? &entry->de_Date : &object->date;

		// If exclusive, deselect this to start with
		if (data->type & SELECTF_EXCLUSIVE && sel)
		{
			// Icon?
			if (object && object->state)
			{
				object->state = 0;
				object->flags |= BDOF_STATE_CHANGE;
			}

			// Normal file
			else
			{
				// Look at entry type
				switch (entry_type)
				{
				// File
				case ENTRY_FILE:
					buffer->buf_SelectedBytes[0] -= entry->de_Size;
					--buffer->buf_SelectedFiles[0];
					entry->de_Flags ^= ENTF_SELECTED;
					break;

				// Directory
				case ENTRY_DIRECTORY:
					buffer->buf_SelectedBytes[0] -= entry->de_Size;
					--buffer->buf_SelectedDirs[0];
					entry->de_Flags ^= ENTF_SELECTED;
					break;
				}
			}
		}

		// Is this entry the right type?
		if ((type == SELECT_ENTRY_FILES && entry_type == ENTRY_DIRECTORY) ||
			(type == SELECT_ENTRY_DIRS && entry_type == ENTRY_FILE))
			continue;

		// Name match
		match = MatchPatternNoCase(name_match, entry_name);
		if ((data->type & SELECT_TYPE_MASK) == SELECT_SIMPLE)
		{
			if (!match)
				continue;
		}

		// Complex matching
		else
		{
			if ((data->name_match == SELECT_MATCH_MATCH && !match) ||
				(data->name_match == SELECT_MATCH_NO_MATCH && match))
				continue;

			// Date match
			if (data->date_match != SELECT_MATCH_IGNORE)
			{
				match =
					(CompareDates(entry_date, &date_match[0]) <= 0 && CompareDates(&date_match[1], entry_date) <= 0);
				if ((data->date_match == SELECT_MATCH_MATCH && !match) ||
					(data->date_match == SELECT_MATCH_NO_MATCH && match))
					continue;
			}

			// Protection match
			if (data->bits_match != SELECT_MATCH_IGNORE && entry)
			{
				unsigned short prot;

				prot = entry->de_Protection ^ 0xf;
				match = ((prot & prot_match[0]) == prot_match[0] && (prot & prot_match[1]) == 0);
				if ((data->bits_match == SELECT_MATCH_MATCH && !match) ||
					(data->bits_match == SELECT_MATCH_NO_MATCH && match))
					continue;
			}

			// Compare match
			if (data->compare_match != SELECT_MATCH_IGNORE && entry)
			{
				DirEntry *dest_entry;
				PathNode *node;
				BOOL ok = 0;

				// Valid list?
				if (dest_list)
				{
					// Go through destination lists
					for (node = (PathNode *)dest_list->list.mlh_Head; node->node.mln_Succ;
						 node = (PathNode *)node->node.mln_Succ)
					{
						// Valid lister?
						if (!node->lister || node->flags & LISTNF_INVALID)
							continue;
						// See if lister contains an entry by this name
						if ((dest_entry = find_entry(&node->lister->cur_buffer->entry_list,
													 entry->de_Node.dn_Name,
													 0,
													 node->lister->cur_buffer->more_flags & DWF_CASE)) &&
							dest_entry->de_Node.dn_Type == entry->de_Node.dn_Type)
						{
							if (data->compare == SELECT_COMPARE_NEWER)
								match = CompareDates(&entry->de_Date, &dest_entry->de_Date) < 0;
							else if (data->compare == SELECT_COMPARE_OLDER)
								match = CompareDates(&entry->de_Date, &dest_entry->de_Date) > 0;
							else
								match = (entry->de_Size != dest_entry->de_Size);

							// Check match
							if ((data->compare_match == SELECT_MATCH_MATCH && match) ||
								(data->compare_match == SELECT_MATCH_NO_MATCH && !match))
							{
								ok = 1;
								break;
							}
						}

						// Not found
						else if (data->compare == SELECT_COMPARE_DIFFERENT)
						{
							ok = 1;
							break;
						}
					}
				}

				// If not ok, continue
				if (!ok)
					continue;
			}
		}

		// First match?
		if (!first_entry)
			first_entry = entry;

		// See if entry state needs to be changed
		if (sel != include)
		{
			// Icon?
			if (object)
			{
				object->state = (object->state) ? 0 : 1;
				object->flags |= BDOF_STATE_CHANGE;
			}

			// Normal file
			else
			{
				// Look at entry type
				switch (entry_type)
				{
				// File
				case ENTRY_FILE:
					if (entry->de_Flags & ENTF_SELECTED)
					{
						buffer->buf_SelectedBytes[0] -= entry->de_Size;
						--buffer->buf_SelectedFiles[0];
					}
					else
					{
						buffer->buf_SelectedBytes[0] += entry->de_Size;
						++buffer->buf_SelectedFiles[0];
					}
					entry->de_Flags ^= ENTF_SELECTED;
					break;

				// Directory
				case ENTRY_DIRECTORY:
					if (entry->de_Flags & ENTF_SELECTED)
					{
						buffer->buf_SelectedBytes[0] -= entry->de_Size;
						--buffer->buf_SelectedDirs[0];
					}
					else
					{
						buffer->buf_SelectedBytes[0] += entry->de_Size;
						++buffer->buf_SelectedDirs[0];
					}
					entry->de_Flags ^= ENTF_SELECTED;
					break;
				}
			}
		}

		// Fix selection count for icon
		if (object)
			backdrop_fix_count(lister->backdrop_info, 1);
	}

	/* ACTION_CHANGE */
	// Icon action mode?
	if (lister->flags & (LISTERF_VIEW_ICONS | LISTERF_ICON_ACTION))
	{
		// Unlock backdrop list
		unlock_listlock(&lister->backdrop_info->objects);
	}

	// Show selection information
	select_show_info(lister, 1);

	// Make visible?
	if (data->type & SELECTF_MAKE_VISIBLE && first_entry)
	{
		// Scroll to make first selection visible
		lister_show_entry(lister, first_entry);
	}
}

// Select an item by the keyboard
short select_key_select(Lister *lister, BOOL doubleclick)
{
	// Clear stored seconds so we won't double-click accidentally
	lister->old_seconds = 0;

	// Toggle entry
	select_toggle_entry(lister, lister->selector_pos, 0, 1);

	// Doing double-click?
	if (doubleclick || ((DirEntry *)lister->cur_buffer->entry_list.mlh_Head)->de_Node.dn_Type == ENTRY_DEVICE)
	{
		// Toggle it again
		select_toggle_entry(lister, lister->selector_pos, 0, 1);
		return 0;
	}

	return 1;
}

// Run a function on a single file
void file_run_function(Lister *lister, DirEntry *entry, Cfg_Function *func, char *dest, ULONG flags)
{
	lister = lister_dual_active_side(lister);

	// Build filename
	strcpy(lister->work_buffer, lister->cur_buffer->buf_Path);
	AddPart(lister->work_buffer, entry->de_Node.dn_Name, 512);

	// Add a / if a directory
	if (entry->de_Node.dn_Type >= 0)
		AddPart(lister->work_buffer, "", 512);

	// Launch function
	function_launch(FUNCTION_RUN_FUNCTION_EXTERNAL,
					func,
					0,
					flags,
					lister,
					0,
					0,
					dest,
					BuildArgArray(lister->work_buffer, 0),
					0,
					0);
}
