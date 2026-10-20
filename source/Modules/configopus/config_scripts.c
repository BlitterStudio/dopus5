#include "config_lib.h"

ULONG LIBFUNC L_Config_Scripts(
	REG(a0, IPCData *ipc),
	REG(a1, IPCData *owner_ipc),
	REG(a2, struct Screen *screen),
	REG(a3, Cfg_ButtonBank *bank0,
	REG(d0, IPTR command_list))
{
	config_scripts_data *data;
	IPCMessage *quit_msg = 0;
	short undo_flag = 0, pending_quit = 0, save_flag = 0;

	// Allocate data
	if (!(data = AllocVec(sizeof(config_scripts_data), MEMF_CLEAR)))
		return 0;

	// Initialise data
	data->ipc = ipc;
	data->owner_ipc = owner_ipc;
	data->bank = bank;
	InitListLock(&data->proc_list, 0);

	// Open window
	if (!(config_scripts_open(data, screen)))
		return 0;

	// Backup bank
	data->bank_backup = CopyButtonBank(data->bank);

	// Event loop
	FOREVER
	{
		struct IntuiMessage *msg;
		IPCMessage *imsg;
		BOOL quit_flag = 0;

		// Any IPC messages?
		while (imsg = (IPCMessage *)GetMsg(ipc->command_port))
		{
			// Look at command
			switch (imsg->command)
			{
			// Quit immediately
			case IPC_QUIT:
				quit_flag = 1;
				quit_msg = imsg;
				imsg = 0;
				break;

			// Hide
			case IPC_HIDE:

				// Is window open?
				if (data->window)
				{
					// Store window position
					data->window_pos = *((Point *)&data->window->LeftEdge);
					data->pos_valid = 1;

					// Tell children to hide
					IPC_ListCommand(&data->proc_list, IPC_HIDE, 0, 0, 0);

					// Close window
					CloseConfigWindow(data->window);
					data->window = 0;
				}
				break;

			// Show
			case IPC_SHOW:

				// Re-open window
				if (!(config_scripts_open(data, (struct Screen *)imsg->data)))
				{
					quit_flag = 1;
				}

				// Tell children to appear
				IPC_ListCommand(&data->proc_list, IPC_SHOW, 0, (IPTR)data->window, 0);
				break;

			// Activate
			case IPC_ACTIVATE:
				if (data->window)
				{
					WindowToFront(data->window);
					ActivateWindow(data->window);
				}
				break;

			// Editor returning a function
			case FUNCTIONEDIT_RETURN:
				if (lister_menu_receive_edit(data, (FunctionReturn *)imsg->data))
					data->change = 1;
				break;

			// Process saying goodbye
			case IPC_GOODBYE: {
				IPCData *bye;
				edit_node *node;

				// Get process
				bye = (IPCData *)imsg->data;

				// See if it's a button editor
				for (node = (edit_node *)data->edit_list.lh_Head; node->node.ln_Succ;
					 node = (edit_node *)node->node.ln_Succ)
				{
					// Was it an editor?
					if (bye == node->editor)
					{
						// Remove edit node
						Remove(&node->node);
						FreeVec(node);
						break;
					}
				}

				// All process gone and pending quit?
				if (pending_quit && (IsListEmpty(&data->proc_list.list)))
				{
					quit_flag = 1;
					pending_quit = 2;
				}
			}
			break;

			// Get a copy of a button
			case BUTTONEDIT_CLIP_BUTTON:

				// Copy button
				if (button = CopyButton((Cfg_Button *)imsg->data, data->bank->memory, 0))
				{
					Cfg_ButtonFunction *func;

					// End any edit
					lister_menu_end_edit(data, 0, 0);

					// No selection
					SetGadgetValue(data->objlist, GAD_LISTER_MENU_ITEMS, (ULONG)-1);

					// Detach list
					SetGadgetChoices(data->objlist, GAD_LISTER_MENU_ITEMS, (APTR)~0);

					// Get first function
					func = (Cfg_ButtonFunction *)FindFunctionType((struct List *)&button->function_list,
																  FTYPE_LEFT_BUTTON);

					// Add a new node
					if (data->sel_item = lister_menu_new_item(data, func, button))
					{
						// Add button to bank list
						AddTail(&data->bank->buttons, &button->node);
					}
					else
						FreeButton(button);

					// Attach list
					SetGadgetChoices(data->objlist, GAD_LISTER_MENU_ITEMS, (APTR)data->menu_list);

					// Select new node
					SetGadgetValue(
						data->objlist, GAD_LISTER_MENU_ITEMS, Att_FindNodeNumber(data->menu_list, data->sel_item));

					// Fix gadgets
					lister_menu_fix_gadgets(data);

					// Set change flag
					data->change = 1;
				}

				// Free incoming button
				FreeButton((Cfg_Button *)imsg->data);
				break;
			}

			// Reply message
			IPC_Reply(imsg);

			// Check quit flag
			if (quit_flag)
				break;
		}

		if (pending_quit == 2)
			break;

		// Intuimessages
		if (data->window)
		{
			while (msg = GetWindowMsg(data->window->UserPort))
			{
				struct IntuiMessage msg_copy;
				UWORD gadgetid = 0;

				// Copy message and reply (unless IDCMPUPDATE)
				msg_copy = *msg;
				if (msg->Class != IDCMP_IDCMPUPDATE)
				{
					ReplyWindowMsg(msg);
					msg = 0;
				}

				// Ignore messages if a quit is pending
				if (pending_quit)
					continue;

				// Get gadget ID
				if (msg_copy.Class == IDCMP_GADGETUP || msg_copy.Class == IDCMP_GADGETDOWN)
					gadgetid = ((struct Gadget *)msg_copy.IAddress)->GadgetID;

				// Look at message
				switch (msg_copy.Class)
				{
				// Close window
				case IDCMP_CLOSEWINDOW:
					quit_flag = 1;
					undo_flag = 1;
					break;

				// BOOPSI message
				case IDCMP_IDCMPUPDATE: {
					struct TagItem *tags = (struct TagItem *)msg_copy.IAddress;

					// Lister?
					if (GetTagData(GA_ID, 0, tags) == GAD_LISTER_MENU_ITEMS)
					{
						short item;

						// Get item
						if ((item = GetTagData(DLV_DragNotify, -1, tags)) != -1)
						{
							short width, height;

							// Store item
							data->drag_item = item;

							// Get rectangle size
							width = GetTagData(GA_Width, 0, tags);
							height = GetTagData(GA_Height, 0, tags);

							// Try to allocate Drag info
							if (data->drag = GetDragInfo(data->window, 0, width, height, 1))
							{
								ListViewDraw draw;
								IPTR ptr;

								// Fill out draw packet
								draw.rp = &data->drag->drag_rp;
								draw.drawinfo = DRAWINFO(data->window);
								draw.node = (struct Node *)Att_FindNode(data->menu_list, item);
								draw.line = 0;
								draw.box.Left = 0;
								draw.box.Top = 0;
								draw.box.Width = width;
								draw.box.Height = height;

								// We want opaqueness
								data->drag->flags |= DRAGF_OPAQUE;

								// Draw image
								ptr = (IPTR)&draw;
								GetAttr(DLV_DrawLine, (Object *)GetTagData(DLV_Object, 0, tags), &ptr);
								GetDragMask(data->drag);

								// Get drag offsets
								data->drag_x = -GetTagData(GA_RelRight, width >> 1, tags);
								data->drag_y = -GetTagData(GA_RelBottom, height >> 1, tags);

								// Show drag image
								ShowDragImage(data->drag,
											  data->window->LeftEdge + msg_copy.MouseX + data->drag_x,
											  data->window->TopEdge + msg_copy.MouseY + data->drag_y);

								// Set mouse move reporting and ticks
								data->window->Flags |= WFLG_REPORTMOUSE | WFLG_RMBTRAP;
								ModifyIDCMP(data->window, data->window->IDCMPFlags | IDCMP_INTUITICKS);

								// Start timer
								StartTimer(data->timer, 0, 500000);

								// Initialise tick count
								data->tick_count = 1;
								data->last_tick = 0;
							}
						}
					}
				}
				break;

				// Intuitick
				case IDCMP_INTUITICKS:
					++data->tick_count;
					break;

				// Mouse move
				case IDCMP_MOUSEMOVE:

					// Valid drag info?
					if (data->drag)
					{
						// Show drag image
						ShowDragImage(data->drag,
									  data->window->WScreen->MouseX + data->drag_x,
									  data->window->WScreen->MouseY + data->drag_y);
					}
					break;

				// Inactive window does menu down
				case IDCMP_INACTIVEWINDOW:
					msg_copy.Code = MENUDOWN;

				// Mouse buttons
				case IDCMP_MOUSEBUTTONS:

					// Valid drag info?
					if (data->drag)
					{
						// Dropped ok?
						if (msg_copy.Code == SELECTUP)
						{
							// End drag
							data->drag_x = data->window->WScreen->MouseX;
							data->drag_y = data->window->WScreen->MouseY;
							lister_menu_end_drag(data, 1);
						}

						// Aborted
						else if (msg_copy.Code == MENUDOWN)
						{
							// End drag
							lister_menu_end_drag(data, 0);
						}
					}
					break;

				// Menu pick
				case IDCMP_MENUPICK: {
					struct MenuItem *item;

					// Get item
					if (!(item = ItemAddress(data->window->MenuStrip, msg_copy.Code)))
						break;

					// Treat as gadget
					gadgetid = MENUID(item);
				}

				// Gadget
				case IDCMP_GADGETUP:
				case IDCMP_GADGETDOWN:

					// Look at gadget ID
					switch (gadgetid)
					{
					// Cancel
					case GAD_LISTER_MENU_CANCEL:
						undo_flag = 1;

					// Save
					case GAD_LISTER_MENU_SAVE:
						save_flag = 1;

					// Use
					case GAD_LISTER_MENU_USE:
						quit_flag = 1;
						break;

					// Menu item selected
					case GAD_LISTER_MENU_ITEMS: {
						Att_Node *node;

						// Get selection
						if (!(node = Att_FindNode(data->menu_list, msg_copy.Code)))
							break;

						// Same as last selection?
						if (node == data->sel_item)
						{
							// Double-click?
							if (DoubleClick(data->seconds, data->micros, msg_copy.Seconds, msg_copy.Micros))
							{
								// Edit item
								lister_menu_edit_item(data);
								data->seconds = 0;
								break;
							}
						}

						// Store double-click time
						data->seconds = msg_copy.Seconds;
						data->micros = msg_copy.Micros;

						// Edit this item
						if (data->sel_item != node)
						{
							lister_menu_end_edit(data, 0, 0);
							data->sel_item = node;
							lister_menu_fix_gadgets(data);
						}
					}
					break;

					// Item name
					case GAD_LISTER_MENU_NAME:
						lister_menu_end_edit(data, 1, 0);
						break;

					// New item
					case GAD_LISTER_MENU_ADD:
						lister_menu_add_item(data, 0);
						break;

					// Insert item
					case GAD_LISTER_MENU_INSERT:
						lister_menu_add_item(data, data->sel_item);
						break;

					// Duplicate item
					case GAD_LISTER_MENU_DUPLICATE:
						lister_menu_add_item(data, (Att_Node *)-1);
						break;

					// Delete item
					case GAD_LISTER_MENU_REMOVE:

						// Valid selection?
						if (data->sel_item)
						{
							Cfg_Button *button;

							// Detach list
							SetGadgetChoices(data->objlist, GAD_LISTER_MENU_ITEMS, (APTR)~0);

							// Get button from selection
							button = (Cfg_Button *)data->sel_item->data;

							// Remove selection
							Att_RemNode(data->sel_item);

							// Free button
							if (button->node.ln_Succ)
								Remove(&button->node);
							FreeButton(button);

							// Attach list
							SetGadgetChoices(data->objlist, GAD_LISTER_MENU_ITEMS, (APTR)data->menu_list);

							// No lister selection
							SetGadgetValue(data->objlist, GAD_LISTER_MENU_ITEMS, (ULONG)-1);

							// Fix gadgets
							data->sel_item = 0;
							lister_menu_fix_gadgets(data);

							// Set change flag
							data->change = 1;
						}
						break;

					// Move up/down
					case GAD_LISTER_MENU_UP:
					case GAD_LISTER_MENU_DOWN:

						// Valid selection?
						if (data->sel_item)
						{
							Att_Node *before;

							// Detach list
							SetGadgetChoices(data->objlist, GAD_LISTER_MENU_ITEMS, (APTR)~0);

							// Move up?
							if (gadgetid == GAD_LISTER_MENU_UP)
								before = (Att_Node *)data->sel_item->node.ln_Pred;

							// Move down
							else
								before = (Att_Node *)data->sel_item->node.ln_Succ->ln_Succ;

							// Reposition
							Att_PosNode(data->menu_list, data->sel_item, before);

							// Attach list
							SetGadgetChoices(data->objlist, GAD_LISTER_MENU_ITEMS, (APTR)data->menu_list);

							// Select new node
							SetGadgetValue(data->objlist,
										   GAD_LISTER_MENU_ITEMS,
										   Att_FindNodeNumber(data->menu_list, data->sel_item));

							// Set change flag
							data->change = 1;
						}
						break;

					// Edit item
					case GAD_LISTER_MENU_EDIT:
						lister_menu_edit_item(data);
						break;

					// Save
					case MENU_LISTER_MENU_SAVE:
					case MENU_LISTER_MENU_SAVEAS:
						lister_menu_save(data, gadgetid);
						break;

					// New/Open
					case MENU_LISTER_MENU_NEW:
					case MENU_LISTER_MENU_OPEN:
						if (!(lister_menu_load(data, (gadgetid == MENU_LISTER_MENU_NEW))))
						{
							quit_flag = 1;
						}
						break;

					// Restore
					case MENU_LISTER_RESTORE:

						// Got backed up bank?
						if (data->restore_bank)
						{
							// Check change
							if (!(lister_menu_check_change(data, 0)))
								break;

							// Use it
							lister_menu_load_menu(data, data->restore_bank, 0);
							lister_menu_update_bank(data);
						}
						break;

					// Last saved
					case MENU_LISTER_LAST_SAVED:
						lister_menu_load(data, -1);
						break;

					// Defaults
					case MENU_LISTER_RESET_DEFAULTS:
						lister_menu_load(data, -2);
						break;

					// System-global
					case GAD_HOTKEYS_GLOBAL:

						if (data->sel_item)
						{
							Cfg_Button *button;

							// Get button
							button = (Cfg_Button *)data->sel_item->data;

							// Set or clear flag
							if (msg_copy.Code)
								button->button.flags |= BUTNF_GLOBAL;
							else
								button->button.flags &= ~BUTNF_GLOBAL;
						}
						break;
					}
					break;

				// Key press
				case IDCMP_RAWKEY:

					// Help?
					if (msg_copy.Code == 0x5f && !(msg_copy.Qualifier & VALID_QUALIFIERS))
					{
						// Set busy pointer
						SetWindowBusy(data->window);

						// Send help command
						IPC_Command(data->owner_ipc, IPC_HELP, (1 << 31), "Menu Editor", 0, REPLY_NO_PORT);

						// Clear busy pointer
						ClearWindowBusy(data->window);
					}
					break;
				}

				// Reply to any outstanding message
				if (msg)
					ReplyWindowMsg(msg);

				// Check quit flag
				if (quit_flag)
					break;
			}
		}

		// Check quit flag
		if (quit_flag)
		{
			if (!pending_quit)
			{
				// Make window busy
				if (data->window)
					SetWindowBusy(data->window);

				// Cancel?
				if (undo_flag)
				{
					save_flag = 0;

					// Check changed
					if (!(lister_menu_check_change(data, 0)))
					{
						// Cancelled
						ClearWindowBusy(data->window);
						undo_flag = 0;
						continue;
					}
				}

				// Kill all processes
				if (!(IPC_ListQuit(&data->proc_list, 0, !undo_flag, FALSE)))
					break;
				pending_quit = 1;
			}
		}

		// Wait for an event
		Wait(1 << ipc->command_port->mp_SigBit | 1 << data->timer->port->mp_SigBit |
			 ((data->app_window) ? (1 << data->app_port->mp_SigBit) : 0) |
			 ((data->window) ? (1 << data->window->UserPort->mp_SigBit) : 0));
	}
