#include "config_lib.h"
#include <Program/function_data.h>
#include "config_buttons.h"

#define FUNCF_PRIVATE (1 << 26)	 // Function is private

/*#define DOpusBase		(data->startup->dopus_base)*/

//#ifdef __amigaos4__
// ULONG ASM funced_init(REG(a0, IPCData *ipc), REG(a2, int skip), REG(a1, FunctionStartup *startup))
//#else
IPC_StartupCode(funced_init, FunctionStartup *, startup)
//#endif
{
	FuncEdData *data;

	// Allocate data
	if (!(data = AllocVec(sizeof(FuncEdData), MEMF_CLEAR)))
		return 0;

	// Store data
	startup->data = data;

	// Initialise some pointers
	data->startup = startup;
	data->function = startup->function;
	data->locale = startup->locale;

	// Create timer
	if (!(data->drag.timer = AllocTimer(UNIT_VBLANK, 0)))
		return 0;

	// Create lists
	if (!(data->func_display_list = Att_NewList(0)) || !(data->flag_list = Att_NewList(0)) ||
		!(data->function_list = Att_NewList(0)))
		return 0;

	// Create app port
	data->appport = CreateMsgPort();
	return 1;
}

void FunctionEditor(void)
{
	struct IntuiMessage *gmsg, msg;
	int break_flag = 0;
	UWORD gadgetid = 0;
	ULONG waitbits;
	FuncEdData *data;
	IPCData *ipc;
	NewConfigWindow newwin;
	FunctionStartup *startup = 0;
	IPCMessage *submsg;
	int success = 0;
	BOOL open_window = 1;
	BPTR lock;
	ConfigWindow windims;

	// Do startup
	if (!(ipc = Local_IPC_ProcStartup((IPTR *)&startup, (APTR)&funced_init)))
	{
		funced_cleanup(startup->data);
		return;
	}

	// Get data pointer
	data = startup->data;

	// Lock RAM: and CD to it
	if ((lock = Lock("ram:", ACCESS_READ)))
		lock = CurrentDir(lock);

	// Copy dimensions
	if (startup->flags & FUNCEDF_CENTER)
		windims = _function_editor_window_center;
	else if (startup->flags & FUNCEDF_LABEL)
		windims = _function_editor_label_window;
	else
		windims = _function_editor_window;

	// Fill in new window
	newwin.parent = startup->window;
	newwin.dims = &windims;
	newwin.title = (startup->title[0]) ? startup->title : (char *)GetString(startup->locale, MSG_FUNCED_TITLE);
	newwin.locale = startup->locale;
	newwin.port = 0;
	newwin.flags = WINDOW_VISITOR | WINDOW_AUTO_KEYS | WINDOW_REQ_FILL;	 //|WINDOW_SIZE_BOTTOM;
	newwin.font = 0;

	// Build flag list
	funced_build_flaglist(data);
	waitbits = 1 << ipc->command_port->mp_SigBit | 1 << data->drag.timer->port->mp_SigBit;

	// Event loop
	FOREVER
	{
		// Check drag
		if (config_drag_check(&data->drag))
		{
			// End drag
			functioned_end_drag(data, 0);
		}

		// Task message?
		while ((submsg = (IPCMessage *)GetMsg(ipc->command_port)))
		{
			// Close message?
			if (submsg->command == IPC_QUIT)
			{
				// Keep changes?
				success = submsg->flags;

				// Set break flag
				break_flag = 1;
			}

			// Activate
			else if (submsg->command == IPC_ACTIVATE && data->window)
			{
				WindowToFront(data->window);
				ActivateWindow(data->window);
			}

			// Hide
			else if (submsg->command == IPC_HIDE && data->window)
			{
				// Store changes in function
				funced_store_edits(data);

				// End any drag in progress
				functioned_end_drag(data, 0);

				// Remove AppWindow
				RemoveAppWindow(data->appwindow);
				data->appwindow = 0;

				// Close window
				CloseConfigWindow(data->window);

				// Zero stuff
				data->window = 0;
				data->objlist = 0;
				waitbits = 1 << ipc->command_port->mp_SigBit;
			}

			// Show
			else if (submsg->command == IPC_SHOW && !data->window)
			{
				// Set flag to open window
				open_window = 1;
				newwin.parent = (struct Window *)submsg->data;
			}

			// Passed-along app message
			else if (submsg->command == CFG_APPMESSAGE_PASS)
			{
				funced_appmsg(data, submsg->data);
				ReplyMsg((struct Message *)submsg->data);
			}

			// Update our identity
			else if (submsg->command == CFG_NEW_ID)
			{
				data->function->function.func_type = submsg->flags;
				startup->object_flags = (IPTR)submsg->data;
			}

			// Copy a function
			else if (submsg->command == FUNCTIONEDIT_COPY_LINE)
			{
				FunctionEntry *entry, *next;

				// Copy function
				functioned_copy_line(data, (FunctionEntry *)submsg->data, 0, (Point *)submsg->data_free);

				// Free function
				entry = (FunctionEntry *)submsg->data;
				while (entry)
				{
					// Get next
					next = (FunctionEntry *)entry->node;

					// Free entry
					FreeVec(entry);
					entry = next;
				}
			}

			// Clip button
			else if (submsg->command == BUTTONEDIT_CLIP_BUTTON)
			{
				Cfg_ButtonFunction *func;

				// Show busy pointer
				SetWindowBusy(data->window);

				// Stop editing
				funced_end_edit(data, data->edit_node, 0, 0);
				data->edit_node = 0;

				// Get first function
				if ((func = (Cfg_ButtonFunction *)FindFunctionType(
						 (struct List *)&((Cfg_Button *)submsg->data)->function_list, FTYPE_LEFT_BUTTON)))
				{
					FunctionEntry dummy;
					Cfg_Instruction *ins;

					// Go through instructions
					for (ins = (Cfg_Instruction *)func->instructions.mlh_Head; ins->node.mln_Succ;
						 ins = (Cfg_Instruction *)ins->node.mln_Succ)
					{
						// Fill out dummy entry
						dummy.type = ins->type;
						if (ins->string)
							strcpy(dummy.buffer, ins->string);
						else
							dummy.buffer[0] = 0;

						// Copy function line
						data->edit_node = funced_new_entry(data, 0, &dummy);
					}

					// Start editing last line
					funced_start_edit(data);

					// Get flags
					data->function->function.flags = func->function.flags;
					data->function->function.code = func->function.code;
					data->function->function.qual = func->function.qual;
					data->function->function.qual_mask = func->function.qual_mask;
					data->function->function.qual_same = func->function.qual_same;

					// Update flag list
					funced_update_flaglist(data);

					// Update key
					funced_show_key(data);
				}

				// Free button
				FreeButton((Cfg_Button *)submsg->data);

				// Clear busy pointer
				ClearWindowBusy(data->window);
			}

			// Reply the message
			IPC_Reply(submsg);
		}

		// Open window?
		if (open_window)
		{
			// Open window
			if (!(data->window = OpenConfigWindow(&newwin)) ||
				!(data->p_objlist =
					  AddObjectList(data->window,
									(startup->flags & FUNCEDF_IMAGE)
										? _function_editor_image_objects
										: ((startup->flags & FUNCEDF_LABEL) ? _function_editor_label_objects
																			: _function_editor_normal_objects))) ||
				!(data->objlist = AddObjectList(data->window, _function_editor_objects)))
				break;

			// Add use or save button
			AddObjectList(
				data->window,
				(startup->flags & FUNCEDF_SAVE) ? _function_editor_objects_save : _function_editor_objects_use);

			// Store window for drag
			data->drag.window = data->window;

			// Set window ID
			SetWindowID(data->window, 0, WINDOW_FUNCTION_EDITOR, (struct MsgPort *)ipc);

			// Add menus
			AddWindowMenus(data->window, _funced_menus);

			// Make this an AppWindow
			data->appwindow = AddAppWindowA(0, 0, data->window, data->appport, 0);

			// Get waitbits
			waitbits |= 1 << data->window->UserPort->mp_SigBit;
			if (data->appwindow)
				waitbits |= 1 << data->appport->mp_SigBit;

			// Disable key?
			if (startup->flags & FUNCEDF_NO_KEY)
			{
				DisableObject(data->objlist, GAD_FUNCED_KEY, TRUE);
			}

			// Initialise gadgets with function data.
			funced_init_gads(data);
			open_window = 0;
		}

		// Intuimessage
		if (data->window)
		{
			while ((gmsg = GetWindowMsg(data->window->UserPort)))
			{
				Att_Node *node;

				// Copy message and reply (unless IDCMPUPDATE)
				msg = *gmsg;
				if (gmsg->Class != IDCMP_IDCMPUPDATE)
				{
					ReplyWindowMsg(gmsg);
					gmsg = 0;
				}

				// Get GadgetID
				if (msg.Class == IDCMP_GADGETDOWN || msg.Class == IDCMP_GADGETUP)
					gadgetid = ((struct Gadget *)msg.IAddress)->GadgetID;

				// Look at message
				switch (msg.Class)
				{
				// Key press
				case IDCMP_VANILLAKEY:

					// If editing something, activate string gadget
					if (data->edit_node)
						ActivateStrGad(GADGET(GetObject(data->objlist, GAD_FUNCED_EDIT)), data->window);
					break;

				// Close window
				case IDCMP_CLOSEWINDOW:

					// Set break flag
					break_flag = 1;
					break;

				// Mouse move
				case IDCMP_MOUSEMOVE:

					// Handle drag move
					config_drag_move(&data->drag);
					break;

				// Inactive window does menu down
				case IDCMP_INACTIVEWINDOW:
					msg.Code = MENUDOWN;

				// Mouse buttons
				case IDCMP_MOUSEBUTTONS:

					// Valid drag info?
					if (data->drag.drag)
					{
						short ok = -1;

						// Dropped ok?
						if (msg.Code == SELECTUP)
						{
							// Is shift down?
							if (msg.Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
								data->drag_shift = 1;
							else
								data->drag_shift = 0;

							// Remember last position
							data->drag.drag_x = data->window->WScreen->MouseX;
							data->drag.drag_y = data->window->WScreen->MouseY;
							ok = 1;
						}

						// Aborted
						else if (msg.Code == MENUDOWN)
						{
							// Set abort
							ok = 0;
						}

						// End drag?
						if (ok != -1)
							functioned_end_drag(data, ok);
					}
					break;

				// Menu
				case IDCMP_MENUPICK:

				{
					struct MenuItem *item;

					// Get item
					if (!(item = ItemAddress(data->window->MenuStrip, msg.Code)))
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
					// Hotkey
					case GAD_FUNCED_KEY: {
						IX ix;
						char *ptr;

						// Get key, see if it's invalid
						if ((ptr = (char *)GetGadgetValue(data->objlist, GAD_FUNCED_KEY)) && *ptr)
						{
							// Try to parse
							if (ParseIX(ptr, &ix))
							{
								// Flash screen
								DisplayBeep(data->window->WScreen);

								// Activate key field again
								ActivateStrGad(GADGET(GetObject(data->objlist, GAD_FUNCED_KEY)), data->window);
							}
							else
							{
								data->function->function.code = ix.ix_Code;
								data->function->function.qual = QualValid(ix.ix_Qualifier);
								data->function->function.qual_mask = ix.ix_QualMask;
								data->function->function.qual_same = ix.ix_QualSame;
							}
						}
					}
					break;

					// Flag selected
					case GAD_FUNCED_FLAGS:

						// Get selected node
						node = Att_FindNode(data->flag_list, msg.Code);

						// Set flag
						data->function->function.flags ^= node->data;

						// Check change
						funced_check_flag(&data->function->function.flags, node->data);

						// Update flag list
						funced_update_flaglist(data);
						break;

					// Edit a line
					case GAD_FUNCED_LISTER:

						// Get selected node
						node = Att_FindNode(data->func_display_list, msg.Code);

						// Is this the node we're already editing?
						if (node == data->edit_node)
							break;

						// If we're editing another one, store its contents
						if (data->edit_node)
							funced_end_edit(data, data->edit_node, 0, 0);

						// Start editing this entry
						data->edit_node = node;
						funced_start_edit(data);
						break;

					// Return pressed in edit line
					case GAD_FUNCED_EDIT:

						// See if we were editing something
						if (data->edit_node)
						{
							// Stop editing
							funced_end_edit(data, data->edit_node, END_DISABLE, msg.Qualifier);
							data->edit_node = 0;
						}
						break;

					// New entry
					case GAD_FUNCED_NEW_ENTRY:
					case GAD_FUNCED_INSERT_ENTRY:

						// Create new entry
						functioned_copy_line(data, 0, gadgetid, 0);
						break;

					// Delete entry
					case GAD_FUNCED_DELETE_ENTRY:
						// Check we were editing something
						if (!data->edit_node)
							break;

						// Stop editing (signal delete)
						funced_end_edit(data, data->edit_node, END_DISABLE | END_DELETE, 0);
						data->edit_node = 0;
						break;

					// Function type
					case GAD_FUNCED_FUNCTION_TYPE:
						// Check we were editing something
						if (!data->edit_node)
							break;

						// Store type
						((FunctionEntry *)data->edit_node->data)->type = msg.Code;

						// Disable popup button if no functions
						DisableObject(data->objlist,
									  GAD_FUNCED_EDIT_GLASS,
									  (msg.Code == INST_COMMAND && !data->startup->func_list));
						break;

					// Glass gadget
					case GAD_FUNCED_EDIT_GLASS:

						// Check we were editing something
						if (data->edit_node)
						{
							char buffer[256];

							// Put up requester
							if (!(funced_command_req(data, buffer, ((FunctionEntry *)data->edit_node->data)->type)))
								break;

							// Insert string in edit line
							funced_edit_insertstring(
								data->objlist, GAD_FUNCED_EDIT, buffer, DOpusBase, (struct Library *)IntuitionBase);
						}
						break;

					// Argument list
					case GAD_FUNCED_EDIT_ARGUMENT:

						// Check we were editing something
						if (data->edit_node)
						{
							char buffer[80];

							// Put up requester
							if (!(funced_command_req(data, buffer, -1)))
								break;

							// Insert string in edit line
							funced_edit_insertstring(
								data->objlist, GAD_FUNCED_EDIT, buffer, DOpusBase, (struct Library *)IntuitionBase);
						}
						break;

					// Export
					case MENU_FUNCED_EXPORT_ASCII:
					case MENU_FUNCED_EXPORT_CMD:

						// Got file requester?
						if (WINREQUESTER(data->window))
						{
							struct TagItem tags[6];
							struct FileRequester *req = WINREQUESTER(data->window);

							// Show busy pointer
							SetWindowBusy(data->window);

							// File requester tags
							tags[0].ti_Tag = ASLFR_Window;
							tags[0].ti_Data = (IPTR)data->window;
							tags[1].ti_Tag = ASLFR_TitleText;
							tags[1].ti_Data = (IPTR)GetString(startup->locale, MSG_FUNCED_SELECT_FILE);
							tags[2].ti_Tag = ASLFR_Flags1;
							tags[2].ti_Data = FRF_DOSAVEMODE | FRF_PRIVATEIDCMP;
							tags[3].ti_Tag = ASLFR_Flags2;
							tags[3].ti_Data = FRF_REJECTICONS;
							char path1[256];
							BPTR lock1 = Lock("DOpus5:Commands", ACCESS_READ);
							if (lock1)
							{
								NameFromLock(lock1, path1, sizeof(path1));
								UnLock(lock1);
							}
							else
							{
								path1[0] = 0;
							}
							tags[4].ti_Tag = (gadgetid == MENU_FUNCED_EXPORT_CMD) ? ASLFR_InitialDrawer : TAG_DONE;
							tags[4].ti_Data = (IPTR)path1;
							tags[5].ti_Tag = TAG_DONE;

							// Show filerequester
							if (AslRequest(req, tags))
							{
								// Build filename
								strcpy(data->buffer, req->fr_Drawer);
								AddPart(data->buffer, req->fr_File, 256);

								// Store changes in function
								funced_store_edits(data);

								// Do export
								if (gadgetid == MENU_FUNCED_EXPORT_CMD)
									function_export_cmd(data->buffer, 0, data->function);
								else
									L_FunctionExportASCII(data->buffer, 0, data->function, startup->a4);
							}

							// Remove busy pointer
							ClearWindowBusy(data->window);
						}
						break;

					// Cut/Copy
					case MENU_FUNCED_CUT:
					case MENU_FUNCED_COPY: {
						APTR iff;

						// Set busy pointer
						SetWindowBusy(data->window);

						// Open clipboard
						if ((iff = IFFOpen((char *)0, IFF_CLIP_WRITE, ID_OPUS)))
						{
							// Stop editing
							funced_end_edit(data, data->edit_node, END_DISABLE, 0);
							data->edit_node = 0;

							// Store changes in function
							funced_store_edits(data);

							// Save function
							SaveFunction(iff, data->function);

							// Close clipboard
							IFFClose(iff);
						}

						// Just copy, not cut?
						if (gadgetid == MENU_FUNCED_COPY)
						{
							// Clear busy pointer
							ClearWindowBusy(data->window);
							break;
						}

						// Fall through, paste null function
					}

					// Paste
					case MENU_FUNCED_PASTE: {
						Cfg_Function *func = 0;

						// Set busy pointer
						SetWindowBusy(data->window);

						// End any edit
						funced_end_edit(data, data->edit_node, END_DISABLE, 0);
						data->edit_node = 0;

						// Paste?
						if (gadgetid == MENU_FUNCED_PASTE)
						{
							APTR iff;

							// Open clipboard
							if ((iff = IFFOpen((char *)0, IFF_CLIP_READ, ID_OPUS)))
							{
								// Find function
								if (IFFNextChunk(iff, ID_FUNC))
								{
									// Read function
									func = ReadFunction(iff, 0, 0, 0);
								}

								// Close clipboard
								IFFClose(iff);
							}

							// No valid function?
							if (!func)
							{
								// Flash error
								DisplayBeep(data->window->WScreen);
								ClearWindowBusy(data->window);
								break;
							}
						}

						// Detach existing list
						SetGadgetChoices(data->objlist, GAD_FUNCED_FLAGS, (APTR)~0);

						// Clear function list
						Att_RemList(data->function_list, REMLIST_SAVELIST | REMLIST_FREEDATA);

						// Free existing instructions
						FreeInstructionList(data->function);

						// Rebuild display list
						funced_build_display(data);

						// Clear flags
						data->function->function.flags = 0;
						funced_update_flaglist(data);

						// Clear key
						data->function->function.code = 0xffff;
						SetGadgetValue(data->objlist, GAD_FUNCED_KEY, 0);

						// Clear label
						if (startup->flags & FUNCEDF_LABEL)
						{
							data->label[0] = 0;
							SetGadgetValue(data->p_objlist, GAD_FUNCED_LABEL, 0);
						}

						// Paste function in?
						if (func)
						{
							short type;

							// Save type
							type = data->function->function.func_type;

							// Copy function in
							CopyFunction(func, 0, data->function);
							data->function->function.func_type = type;

							// Free copied function
							FreeFunction(func);

							// Initialise gadgets
							funced_init_gads(data);
						}

						// Clear busy pointer
						ClearWindowBusy(data->window);
					}
					break;

					// Use
					case GAD_FUNCED_USE:
						success = 1;

					// Cancel
					case GAD_FUNCED_CANCEL:
						break_flag = 1;
						break;
					}
					break;

				// Key press
				case IDCMP_RAWKEY:

					// Help?
					if (msg.Code == 0x5f && !(msg.Qualifier & VALID_QUALIFIERS))
					{
						// Set busy pointer
						SetWindowBusy(data->window);

						// Send help command
						IPC_Command(startup->main_owner, IPC_HELP, (1 << 31), "Function Editor", 0, REPLY_NO_PORT);

						// Clear busy pointer
						ClearWindowBusy(data->window);
					}
					break;

				// BOOPSI message
				case IDCMP_IDCMPUPDATE: {
					struct TagItem *tags = (struct TagItem *)msg.IAddress;
					short item;

					// Check ID
					if (GetTagData(GA_ID, 0, tags) != GAD_FUNCED_LISTER)
						break;

					// Get item
					if ((item = GetTagData(DLV_DragNotify, -1, tags)) != -1)
					{
						// Start the drag
						config_drag_start(&data->drag, data->func_display_list, item, tags, TRUE);
					}
				}
				break;

				// Ticks
				case IDCMP_INTUITICKS:
					++data->drag.tick_count;
					break;
				}

				// Reply to any outstanding message
				if (gmsg)
					ReplyWindowMsg(gmsg);
			}
		}

		// AppMessage
		if (data->appwindow)
		{
			struct AppMessage *msg;

			while ((msg = (struct AppMessage *)GetMsg(data->appport)))
			{
				// Make sure window is active
				ActivateWindow(data->window);

				// Handle message
				funced_appmsg(data, msg);
				ReplyMsg((struct Message *)msg);
			}
		}

		// Check break flag
		if (break_flag)
			break;

		// Wait for message
		Wait(waitbits);
	}

	// Restore CD
	if (lock)
		UnLock(CurrentDir(lock));

	// Edit successful?
	if (success)
	{
		FunctionReturn ret;

		// Store changes in function
		funced_store_edits(data);

		// Fill out return data
		ret.object = startup->object;
		ret.object_flags = startup->object_flags;
		ret.function = data->function;

		// Send new function back
		IPC_Command(startup->owner_ipc, FUNCTIONEDIT_RETURN, 0, &ret, 0, REPLY_NO_PORT);
	}

	// End any drag in progress
	functioned_end_drag(data, 0);

	// Free edit function
	FreeFunction(data->function);

	// Close window
	RemoveAppWindow(data->appwindow);
	CloseConfigWindow(data->window);

	// Close application port
	if (data->appport)
	{
		struct Message *msg;
		while ((msg = GetMsg(data->appport)))
			ReplyMsg(msg);
		DeleteMsgPort(data->appport);
	}

	// Say goodbye
	IPC_Goodbye(ipc, startup->owner_ipc, startup->object_flags);

	// Delete IPC data
	Forbid();
	IPC_Free(ipc);

	// Free data
	funced_cleanup(data);
	FreeVec(startup);
}

void funced_cleanup(FuncEdData *data)
{
	if (data)
	{
		// Free timer
		FreeTimer(data->drag.timer);

		// Free lists
		Att_RemList(data->flag_list, 0);
		Att_RemList(data->func_display_list, 0);
		Att_RemList(data->function_list, REMLIST_FREEDATA);

		// Free data
		FreeVec(data);
	}
}

// Build list of flags
void funced_build_flaglist(FuncEdData *data)
{
	short flag;

	// Go through flag list
	for (flag = 0; data->startup->flag_list[flag]; flag += 2)
	{
		// Add flag to the list
		Att_NewNode(data->flag_list,
					GetString(data->locale, data->startup->flag_list[flag + 1]),
					data->startup->flag_list[flag],
					ADDNODE_SORT);
	}
}

// Update flag selections
void funced_update_flaglist(FuncEdData *data)
{
	Att_Node *node;

	// Detach existing list
	SetGadgetChoices(data->objlist, GAD_FUNCED_FLAGS, (APTR)~0);

	// Go through flag list
	for (node = (Att_Node *)data->flag_list->list.lh_Head; node->node.ln_Succ; node = (Att_Node *)node->node.ln_Succ)
	{
		// Is flag set?
		if (data->function->function.flags & node->data)
			node->node.lve_Flags |= LVEF_SELECTED;
		else
			node->node.lve_Flags &= ~LVEF_SELECTED;
	}

	// Attach list to gadget
	SetGadgetChoices(data->objlist, GAD_FUNCED_FLAGS, data->flag_list);
}

// Decompile function string into list of functions
void funced_decompile(FuncEdData *data)
{
	FunctionEntry *entry;
	Cfg_Instruction *ins;

	// Clear list
	Att_RemList(data->function_list, REMLIST_SAVELIST | REMLIST_FREEDATA);

	// Copy to function entry list
	for (ins = (Cfg_Instruction *)data->function->instructions.mlh_Head; ins->node.mln_Succ;
		 ins = (Cfg_Instruction *)ins->node.mln_Succ)
	{
		// Label?
		if (ins->type == INST_LABEL)
		{
			// Store label
			stccpy(data->label, ins->string, 79);
			continue;
		}

		// Allocate function entry
		if ((entry = AllocVec(sizeof(FunctionEntry), MEMF_CLEAR)))
		{
			// Store function data
			entry->type = ins->type;
			if (ins->string)
				strcpy(entry->buffer, ins->string);

			// Add entry to list
			if (!(entry->node = Att_NewNode(data->function_list, 0, (IPTR)entry, 0)))
				FreeVec(entry);
		}
	}
}

// Compile function list into function
void funced_compile(FuncEdData *data)
{
	FunctionEntry *entry;
	Cfg_Instruction *ins;
	Att_Node *node;

	// Free existing instructions
	FreeInstructionList(data->function);

	// Got a label?
	if (data->label[0])
	{
		// Create label instruction
		if ((ins = NewInstruction(0, INST_LABEL, data->label)))
			AddHead((struct List *)&data->function->instructions, (struct Node *)ins);
	}

	// Go through instructions
	for (node = (Att_Node *)data->function_list->list.lh_Head; node->node.ln_Succ;
		 node = (Att_Node *)node->node.ln_Succ)
	{
		// Get function entry
		entry = (FunctionEntry *)node->data;

		// Create a new instruction
		if ((ins = NewInstruction(0, entry->type, entry->buffer)))
		{
			// Add to function list
			AddTail((struct List *)&data->function->instructions, (struct Node *)ins);
		}
	}
}

// Build display list from function list
void funced_build_display(FuncEdData *data)
{
	Att_Node *node;

	// Detach existing list
	SetGadgetChoices(data->objlist, GAD_FUNCED_LISTER, (APTR)~0);

	// Clear list
	Att_RemList(data->func_display_list, REMLIST_SAVELIST);

	// Go through function list
	for (node = (Att_Node *)data->function_list->list.lh_Head; node->node.ln_Succ;
		 node = (Att_Node *)node->node.ln_Succ)
	{
		// Build display string for this node
		funced_build_entrydisplay(data, 0, (FunctionEntry *)node->data);
	}

	// Attach list to gadget
	SetGadgetChoices(data->objlist, GAD_FUNCED_LISTER, data->func_display_list);
}

// Build display entry for a function
void funced_build_entrydisplay(FuncEdData *data, Att_Node *node, FunctionEntry *entry)
{
	// If node is 0, create a new one
	if (!node)
		node = Att_NewNode(data->func_display_list, 0, (IPTR)entry, 0);

	// Check valid node and data
	if (!node || !node->node.ln_Succ || !entry)
		return;

	// If node has a name, free it
	FreeMemH(node->node.ln_Name);

	// Get new name
	if ((node->node.ln_Name = AllocMemH(data->func_display_list->memory, strlen(entry->buffer) + 32)))
	{
		// Build new name
		strcpy(node->node.ln_Name, GetString(data->locale, data->startup->func_labels[entry->type]));
		if (entry->buffer[0])
		{
			strcat(node->node.ln_Name, "\a\xc");
			strcat(node->node.ln_Name, entry->buffer);
		}
	}
}

// Start editing a line
void funced_start_edit(FuncEdData *data)
{
	// Set lister selection
	SetGadgetValue(
		data->objlist, GAD_FUNCED_LISTER, Att_NodeDataNumber(data->func_display_list, data->edit_node->data));

	// Copy string
	SetGadgetValue(data->objlist, GAD_FUNCED_EDIT, (IPTR)((FunctionEntry *)data->edit_node->data)->buffer);
	DisableObject(data->objlist, GAD_FUNCED_EDIT, FALSE);

	// Set function type
	SetGadgetValue(data->objlist, GAD_FUNCED_FUNCTION_TYPE, ((FunctionEntry *)data->edit_node->data)->type);
	DisableObject(data->objlist, GAD_FUNCED_FUNCTION_TYPE, FALSE);

	// Enable some gadgets
	DisableObject(data->objlist, GAD_FUNCED_INSERT_ENTRY, FALSE);
	DisableObject(data->objlist, GAD_FUNCED_DELETE_ENTRY, FALSE);

	// Enable popup list if not a command, or if command & we have function list
	DisableObject(data->objlist,
				  GAD_FUNCED_EDIT_GLASS,
				  (((FunctionEntry *)data->edit_node->data)->type == INST_COMMAND && !data->startup->func_list));
	DisableObject(data->objlist,
				  GAD_FUNCED_EDIT_ARGUMENT,
				  (((FunctionEntry *)data->edit_node->data)->type == INST_COMMAND && !data->startup->func_list));

	// Activate gadget
	ActivateStrGad(GADGET(GetObject(data->objlist, GAD_FUNCED_EDIT)), data->window);
}

// Stop editing a line
BOOL funced_end_edit(FuncEdData *data, Att_Node *node, int endflag, UWORD qual)
{
	BOOL delete = 0;

	// Valid node?
	if (node)
	{
		// Get entry data
		FunctionEntry *entry = (FunctionEntry *)node->data;

		// Detach existing list
		SetGadgetValue(data->objlist, GAD_FUNCED_LISTER, (ULONG)~0);
		SetGadgetChoices(data->objlist, GAD_FUNCED_LISTER, (APTR)~0);

		// Clear string if delete flag is set
		if (endflag & END_DELETE)
			entry->buffer[0] = 0;

		// Otherwise, use string
		else
		{
			// Copy string
			strcpy(entry->buffer, (char *)GetGadgetValue(data->objlist, GAD_FUNCED_EDIT));

			// Is type set to command?
			if (entry->type == INST_COMMAND && data->startup->func_list &&
				!(qual & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)))
			{
				char *ptr;
				struct MinList *list;
				CommandList *command;
				BOOL ok = 0;

				// Skip any leading spaces
				ptr = entry->buffer;
				while (*ptr && *ptr == ' ')
					++ptr;

				// Go through list of commands
				list = (struct MinList *)data->startup->func_list;
				for (command = (CommandList *)list->mlh_Head; command->node.mln_Succ;
					 command = (CommandList *)command->node.mln_Succ)
				{
					short len;

					// Check command length
					len = strlen(command->name);

					// Try and match command
					if ((ptr[len] == ' ' || !ptr[len]) && strnicmp(ptr, command->name, len) == 0)
					{
						// It's valid
						ok = 1;
						break;
					}
				}

				// If it's not a valid command, change type to AmigaDOS
				if (!ok)
				{
					entry->type = INST_AMIGADOS;
					SetGadgetValue(data->objlist, GAD_FUNCED_FUNCTION_TYPE, INST_AMIGADOS);
				}
			}

			// New function?
			if (node->node.lve_Flags)
			{
				// Clear flag
				node->node.lve_Flags = 0;

				// ARexx?
				if (entry->type == INST_AREXX)
				{
					char buf[80];

					// Get arguments
					if (funced_rexx_args(entry->buffer, buf))
					{
						// Add to buffer
						strcat(entry->buffer, buf);
					}
				}
			}
		}

		// If buffer is empty, delete node
		if (!entry->buffer[0])
		{
			// Remove display node
			Att_RemNode(node);

			// Remove node from function list
			Att_RemNode(entry->node);

			// Free entry data
			FreeVec(entry);
			delete = 1;
		}

		// Rebuild display string
		else
			funced_build_entrydisplay(data, node, entry);

		// Reattach list
		SetGadgetChoices(data->objlist, GAD_FUNCED_LISTER, data->func_display_list);
	}

	// Clear string gadget
	SetGadgetValue(data->objlist, GAD_FUNCED_EDIT, 0);
	DisableObject(data->objlist, GAD_FUNCED_EDIT, (endflag & END_DISABLE));

	// Disable some gadgets (if disable flag set)
	if (endflag & END_DISABLE)
	{
		DisableObject(data->objlist, GAD_FUNCED_INSERT_ENTRY, TRUE);
		DisableObject(data->objlist, GAD_FUNCED_DELETE_ENTRY, TRUE);
		DisableObject(data->objlist, GAD_FUNCED_FUNCTION_TYPE, TRUE);
		DisableObject(data->objlist, GAD_FUNCED_EDIT_GLASS, TRUE);
		DisableObject(data->objlist, GAD_FUNCED_EDIT_ARGUMENT, TRUE);
	}

	return delete;
}

// Create a new entry
Att_Node *funced_new_entry(FuncEdData *data, Att_Node *insert, FunctionEntry *copy)
{
	Att_Node *node = 0;
	FunctionEntry *entry;

	// Allocate FunctionEntry
	if ((entry = AllocVec(sizeof(FunctionEntry), MEMF_CLEAR)))
	{
		// Copy existing node if it exists
		if (copy)
		{
			entry->type = copy->type;
			strcpy(entry->buffer, copy->buffer);
		}

		// Create new function node
		entry->node = Att_NewNode(data->function_list, 0, (IPTR)entry, 0);

		// Detach existing list
		SetGadgetChoices(data->objlist, GAD_FUNCED_LISTER, (APTR)~0);

		// Create new display node
		node = Att_NewNode(data->func_display_list, 0, (IPTR)entry, 0);

		// If insert, position nodes
		if (insert)
		{
			Att_PosNode(data->function_list, entry->node, ((FunctionEntry *)insert->data)->node);
			Att_PosNode(data->func_display_list, node, insert);
		}

		// Get preceeding node
		insert = (Att_Node *)node->node.ln_Pred;

		// If it's valid, get the type of that entry
		if (!copy && insert && insert->node.ln_Pred)
		{
			((FunctionEntry *)node->data)->type = ((FunctionEntry *)insert->data)->type;
		}

		// Build display node
		funced_build_entrydisplay(data, node, entry);

		// Reattach list to gadget
		SetGadgetChoices(data->objlist, GAD_FUNCED_LISTER, data->func_display_list);

		// Reset type cycle gadget
		SetGadgetValue(data->objlist, GAD_FUNCED_FUNCTION_TYPE, ((FunctionEntry *)node->data)->type);
	}

	// Return node
	return node;
}

// Check mutual-exclusion of flags
void funced_check_flag(ULONG *flags, ULONG change)
{
	// See which flag changed
	switch (change)
	{
	// Output window
	case FUNCF_OUTPUT_WINDOW:

		// Output window on?
		if (*flags & FUNCF_OUTPUT_WINDOW)
		{
			// Output file off
			*flags &= ~FUNCF_OUTPUT_FILE;
		}
		break;

	// Output to file
	case FUNCF_OUTPUT_FILE:

		// Output to file on?
		if (*flags & FUNCF_OUTPUT_FILE)
		{
			// Output window/Workbench output/WaitClose/Run Async off
			*flags &= ~(FUNCF_OUTPUT_WINDOW | FUNCF_WAIT_CLOSE | FUNCF_WORKBENCH_OUTPUT | FUNCF_RUN_ASYNC);
		}
		break;

	// Workbench output
	case FUNCF_WORKBENCH_OUTPUT:

		// Workbench output on?
		if (*flags & FUNCF_WORKBENCH_OUTPUT)
		{
			// Output window on
			*flags |= FUNCF_OUTPUT_WINDOW;

			// Output file off
			*flags &= ~FUNCF_OUTPUT_FILE;
		}
		break;

	// Run Async
	case FUNCF_RUN_ASYNC:

		// Reload/Rescan off
		*flags &= ~(FUNCF_RELOAD_FILES | FUNCF_RESCAN_SOURCE | FUNCF_RESCAN_DEST);

		// If output to file is on, change to window
		if (*flags & FUNCF_OUTPUT_FILE)
		{
			*flags &= ~FUNCF_OUTPUT_FILE;
			*flags |= FUNCF_OUTPUT_WINDOW;
		}
		break;

	// CD source
	case FUNCF_CD_SOURCE:
		*flags &= ~FUNCF_CD_DESTINATION;
		break;

	// CD dest
	case FUNCF_CD_DESTINATION:
		*flags &= ~FUNCF_CD_SOURCE;
		break;

	// Reload/Rescan
	case FUNCF_RELOAD_FILES:
	case FUNCF_RESCAN_SOURCE:
	case FUNCF_RESCAN_DEST:

		// Run Async off
		*flags &= ~FUNCF_RUN_ASYNC;
		break;
	}
}

// Store function edits
void funced_store_edits(FuncEdData *data)
{
	IX ix;
	char *ptr;

	// Get key
	ptr = (char *)GetGadgetValue(data->objlist, GAD_FUNCED_KEY);
	if (ptr && !(ParseIX(ptr, &ix)))
	{
		data->function->function.code = ix.ix_Code;
		data->function->function.qual = QualValid(ix.ix_Qualifier);
		data->function->function.qual_mask = ix.ix_QualMask;
		data->function->function.qual_same = ix.ix_QualSame;
	}
	else
	{
		data->function->function.code = 0xffff;
		data->function->function.qual = 0;
		data->function->function.qual_mask = 0;
		data->function->function.qual_same = 0;
	}

	// Valid IX expression
	data->function->function.flags2 |= FUNCF2_VALID_IX;

	// If we're editing a line, store its contents
	if (data->edit_node)
		funced_end_edit(data, data->edit_node, 0, 0);

	// Want a label?
	if (data->startup->flags & FUNCEDF_LABEL)
	{
		strcpy(data->label, (char *)GetGadgetValue(data->p_objlist, GAD_FUNCED_LABEL));
		data->function->function.flags2 |= FUNCF2_LABEL_FUNC;
	}
	else
	{
		// This editor has no label gadget; ensure no stale label
		// (e.g. from a paste of a labelled function) leaks through
		// to the compiled instructions.
		data->label[0] = 0;
		data->function->function.flags2 &= ~FUNCF2_LABEL_FUNC;
	}

	// Compile function list into function
	funced_compile(data);
}

// Initialise gadgets with function data
void funced_init_gads(FuncEdData *data)
{
	// Show key
	funced_show_key(data);

	// Attach flag list to lister
	funced_update_flaglist(data);

	// Decompile function into function list
	funced_decompile(data);

	// Build function list
	funced_build_display(data);

	// Disable gadgets initially
	funced_end_edit(data, 0, END_DISABLE, 0);

	// Want a label?
	if (data->startup->flags & FUNCEDF_LABEL)
		SetGadgetValue(data->p_objlist, GAD_FUNCED_LABEL, (IPTR)data->label);
}

// Show key
void funced_show_key(FuncEdData *data)
{
	// Key equivalent?
	if (data->function->function.flags2 & FUNCF2_FILETYPE_FUNC)
		DisableObject(data->objlist, GAD_FUNCED_KEY, TRUE);

	// Need to build key string
	else
	{
		char buffer[128];

		// Build key string
		BuildKeyString(data->function->function.code,
					   data->function->function.qual,
					   data->function->function.qual_mask,
					   data->function->function.qual_same,
					   buffer);

		// Fill out gadget
		SetGadgetValue(data->objlist, GAD_FUNCED_KEY, (IPTR)buffer);
	}
}

// List of commands
BOOL funced_command_req(FuncEdData *data, char *buffer, short type)
{
	BOOL ret = 0;

	// Make window busy
	SetWindowBusy(data->window);

	// Internal command?
	while (type == INST_COMMAND || type < 0)
	{
		struct MinList *list;
		Att_List *command_list;
		CommandList *command;
		short selection;
		BOOL cmdarg = 0;

		// Create a new list
		if (!(command_list = Att_NewList(0)))
			break;

		// Internal commands?
		if (type == INST_COMMAND)
		{
			// Got list?
			if (data->startup->func_list)
			{
				// Go through list of commands
				list = (struct MinList *)data->startup->func_list;
				for (command = (CommandList *)list->mlh_Head; command->node.mln_Succ;
					 command = (CommandList *)command->node.mln_Succ)
				{
					char buf[80];

					// Not private?
					if (!(command->flags & FUNCF_PRIVATE))
					{
						// Build entry string
						lsprintf(buf, "%s\t%s", command->name, command->desc);

						// Add to requester list (sorted)
						Att_NewNode(command_list, buf, (IPTR)command, ADDNODE_SORT);
					}
				}
			}
		}

		// Command arguments?
		else if (((FunctionEntry *)data->edit_node->data)->type == INST_COMMAND)
		{
			char *cmd;

			// Get command pointer
			cmd = (char *)GetGadgetValue(data->objlist, GAD_FUNCED_EDIT);
			while (*cmd == ' ')
				++cmd;

			// Go through list of commands
			list = (struct MinList *)data->startup->func_list;
			for (command = (CommandList *)list->mlh_Head; command->node.mln_Succ;
				 command = (CommandList *)command->node.mln_Succ)
			{
				short len;

				// Get command length
				len = strlen(command->name);

				// Match command
				if (strnicmp(cmd, command->name, len) == 0 && (cmd[len] == ' ' || !cmd[len]))
				{
					// Matched!
					break;
				}
			}

			// Got one, with a template?
			if (command->node.mln_Succ && command->template && *command->template)
			{
				char *ptr, buf[80];
				short len = 0;

				// Go through template string
				for (ptr = command->template;; ptr++)
				{
					// End of string or keyword?
					if (*ptr == ',' || !*ptr)
					{
						// Terminate buffer
						buf[len] = 0;

						// Add to requester list
						Att_NewNode(command_list, buf, 0, 0);

						// Clear buffer
						len = 0;

						// End of string?
						if (!*ptr)
							break;
					}

					// Store in buffer
					else if (len < 79)
						buf[len++] = *ptr;
				}

				// Set flag
				cmdarg = 1;
			}
		}

		// Arguments
		else
		{
			short num;
			char *str;

			// Go through strings
			for (num = MSG_FUNCED_ARGUMENT_1; *(str = GetString(data->locale, num)) != '-'; num++)
			{
				// Add to requester list
				Att_NewNode(command_list, str, 0, 0);
			}
		}

		// Empty list?
		if (IsListEmpty((struct List *)command_list))
		{
			// Error
			DisplayBeep(data->window->WScreen);
			selection = -1;
		}

		// Ok to show
		else
		{
			// Put up selection list
			selection = SelectionList(
				command_list,
				data->window,
				0,
				GetString(data->startup->locale, (type == -1) ? MSG_FUNCED_SELECT_ARG : MSG_FUNCED_SELECT_COMMAND),
				-1,
				0,
				0,
				GetString(data->startup->locale, MSG_OKAY),
				GetString(data->startup->locale, MSG_CANCEL),
				0,
				0);
		}

		// Valid selection?
		if (selection > -1)
		{
			Att_Node *node;

			// Get selection
			if ((node = Att_FindNode(command_list, selection)))
			{
				// Argument?
				if (type == -1)
				{
					char *ptr;

					// Command argument?
					if (cmdarg)
					{
						// Copy up to = or /
						if (!(ptr = strchr(node->node.ln_Name, '=')))
							ptr = strchr(node->node.ln_Name, '/');
					}

					// Normal arg, copy up to tab
					else
						ptr = strchr(node->node.ln_Name, '\t');

					// Copy to buffer
					if (ptr)
						stccpy(buffer, node->node.ln_Name, (ptr - node->node.ln_Name) + 1);
					else
						strcpy(buffer, node->node.ln_Name);
				}

				// Command
				else
				{
					// Copy command into buffer
					strcpy(buffer, ((CommandList *)node->data)->name);
				}
				ret = 1;
			}
		}

		// Free list
		Att_RemList(command_list, 0);
		break;
	}

	// Other command
	if (type > INST_COMMAND)
	{
		struct TagItem tags[6];

		// Fill out requester tags
		tags[0].ti_Tag = ASLFR_Window;
		tags[0].ti_Data = (IPTR)data->window;
		tags[1].ti_Tag = ASLFR_TitleText;
		tags[1].ti_Data = (IPTR)GetString(data->locale, MSG_SELECT_FILE);
		tags[2].ti_Tag = ASLFR_InitialFile;
		tags[2].ti_Data = (IPTR) "";
		char path2[256];
		BPTR lock2;
		path2[0] = 0;
		if (type == INST_SCRIPT)
		{
			lock2 = Lock("s:", ACCESS_READ);
			if (lock2)
			{
				NameFromLock(lock2, path2, sizeof(path2));
				UnLock(lock2);
			}
		}
		else if (type == INST_AREXX)
		{
			lock2 = Lock("DOpus5:ARexx/", ACCESS_READ);
			if (lock2)
			{
				NameFromLock(lock2, path2, sizeof(path2));
				UnLock(lock2);
			}
		}
		tags[3].ti_Tag = (data->last_type != type) ? ASLFR_InitialDrawer : TAG_IGNORE;
		tags[3].ti_Data = (IPTR)path2;
		tags[4].ti_Tag = ASLFR_Flags1;
		tags[4].ti_Data = FRF_PRIVATEIDCMP;
		tags[5].ti_Tag = TAG_END;

		// Put up file requester
		if (AslRequest(DATA(data->window)->request, tags))
		{
			// Copy into buffer
			strcpy(buffer, DATA(data->window)->request->fr_Drawer);
			AddPart(buffer, DATA(data->window)->request->fr_File, 256);
			ret = 1;
		}

		// Remember type
		data->last_type = type;
	}

	// Clear window busy
	ClearWindowBusy(data->window);

	return ret;
}

// Handle app message
void funced_appmsg(FuncEdData *data, struct AppMessage *msg)
{
	short num;

	// Is the drop over the image gadget
	if (data->startup->flags & FUNCEDF_IMAGE &&
		CheckObjectArea(GetObject(data->p_objlist, GAD_FUNCED_LABEL), msg->am_MouseX, msg->am_MouseY))
	{
		char buf[262];
		if (GetWBArgPath(msg->am_ArgList, buf, sizeof(buf)))
		{
			APTR iff;

			// Device?
			if (buf[0] && buf[strlen(buf) - 1] == ':')
				AddPart(buf, "Disk.info", 256);

			// See if file is ILBM
			else if ((iff = IFFOpen(buf, IFF_READ, ID_ILBM)))
			{
				IFFClose(iff);
			}
			else
			{
				// See if it's an icon
				BPTR lock;
				strcat(buf, ".info");
				if ((lock = Lock(buf, ACCESS_READ)))
					UnLock(lock);
				else
					buf[strlen(buf) - 5] = 0;
			}

			// Fill out field
			SetGadgetValue(data->p_objlist, GAD_FUNCED_LABEL, (IPTR)buf);
			return;
		}
	}

	// See if we were editing something
	if (data->edit_node)
	{
		// Stop editing
		funced_end_edit(data, data->edit_node, 0, 0);
		data->edit_node = 0;
	}

	// Detach existing list
	SetGadgetChoices(data->objlist, GAD_FUNCED_LISTER, (APTR)~0);

	// Go through arguments
	for (num = 0; num < msg->am_NumArgs; num++)
	{
		Cfg_Instruction *ins;

		// Create instruction
		if ((ins = instruction_from_wbarg(&msg->am_ArgList[num], 0)))
		{
			FunctionEntry *entry;
			Att_Node *node;

			// Allocate FunctionEntry
			if ((entry = AllocVec(sizeof(FunctionEntry), MEMF_CLEAR)))
			{
				// Create new function node
				entry->node = Att_NewNode(data->function_list, 0, (IPTR)entry, 0);

				// Fill out entry
				strcpy(entry->buffer, ins->string);
				entry->type = ins->type;

				// Create display node
				node = Att_NewNode(data->func_display_list, 0, (IPTR)entry, 0);

				// Build display node
				funced_build_entrydisplay(data, node, entry);
				data->edit_node = node;
			}

			// Free instruction
			FreeInstruction(ins);
		}
	}

	// Reattach list to gadget
	SetGadgetChoices(data->objlist, GAD_FUNCED_LISTER, data->func_display_list);

	// Start editing last entry
	if (data->edit_node)
	{
		funced_start_edit(data);
		ActivateStrGad(GADGET(GetObject(data->objlist, GAD_FUNCED_EDIT)), data->window);
	}
}

// See if an app argument is ok
short funced_appmsg_arg(struct WBArg *arg, char *buffer, struct Library *DOSBase)
{
	BPTR old, file;
	long test = 0;
	D_S(struct FileInfoBlock, fib)

	char buf[40];
	short type = INST_PROJECT;

	// Directory?
	if (!arg->wa_Name || !*arg->wa_Name)
		type = INST_DIR;

	// File
	else
	{
		// Change directory
		old = CurrentDir(arg->wa_Lock);

		// Try to open file
		if ((file = Lock(arg->wa_Name, ACCESS_READ)))
		{
			Examine(file, fib);
			UnLock(file);

			if ((file = Open(arg->wa_Name, MODE_OLDFILE)))
			{
				// Read long
				Read(file, (char *)&test, sizeof(long));
				Close(file);
#ifdef __AROS__
				test = AROS_BE2LONG(test);
#endif

				// Executable?
#ifdef __AROS__
				if (test == 0x7f454c46)
#else
				if ((test == 0x000003f3) || (test == 0x7f454c46))
#endif
				{
					// See if file has an icon
					lsprintf(buf, "%s.info", arg->wa_Name);
					if ((file = Lock(buf, ACCESS_READ)))
					{
						// Workbench program
						type = INST_WORKBENCH;
						UnLock(file);
					}

					// AmigaDOS
					else
						type = INST_AMIGADOS;
				}

				// Else is Script bit set?
				else if (fib->fib_Protection & FIBF_SCRIPT)
				{
					// Script file
					type = INST_SCRIPT;
				}
			}
		}

		// Restore directory
		CurrentDir(old);
	}

	// Got buffer?
	if (buffer)
	{
		// Get full pathname
		NameFromLock(arg->wa_Lock, buffer, 256);
		if (type != INST_DIR)
			AddPart(buffer, arg->wa_Name, 256);
	}

	return type;
}

// End drag
void functioned_end_drag(FuncEdData *data, short ok)
{
	struct Window *window;
	ULONG id = 0;
	IPCData *ipc = 0;
	short drag_item;

	// Not dragging something?
	if (!data->drag.drag)
		return;

	// End drag
	if (!(window = config_drag_end(&data->drag, ok)))
		return;

	// Not our own window?
	if (window != data->window)
	{
		// Get window ID
		id = GetWindowID(window);

		// Forbid to get IPC
		Forbid();
		if (!(ipc = (IPCData *)GetWindowAppPort(window)))
			Permit();
	}

	// Unlock layer
	UnlockLayerInfo(&data->window->WScreen->LayerInfo);
	ok = 0;

	// Get drag item
	drag_item = Att_FindNodeNumber(data->func_display_list, data->drag.drag_node);

	// Drop on our window?
	if (window == data->window)
	{
		short sel;

		// Get selection
		sel = functioned_get_line(data->window,
								  GetObject(data->objlist, GAD_FUNCED_LISTER),
								  data->drag.drag_x,
								  data->drag.drag_y,
								  (struct Library *)IntuitionBase);

		// Valid selection?
		if (sel != drag_item)
		{
			Att_Node *node, *before, *drop_node;
			short a = 0;
			Att_List *list;

			// Get node we dropped on
			drop_node = Att_FindNode(data->func_display_list, sel);

			// Stop editing
			if (data->edit_node)
			{
				// End edit, see if line was deleted
				if (funced_end_edit(data, data->edit_node, END_DISABLE, 0))
				{
					// Was drop onto the deleted line?
					if (drop_node == data->edit_node)
						a = 1;
				}
				data->edit_node = 0;
			}

			// Ok to proceed?
			if (!a)
			{
				// Detach existing list
				SetGadgetChoices(data->objlist, GAD_FUNCED_LISTER, (APTR)~0);

				// Do twice for both lists
				for (a = 0, list = data->function_list; a < 2; a++, list = data->func_display_list)
				{
					// Get selected entry
					if ((node = Att_FindNode(list, drag_item)))
					{
						// Remove it
						Remove((struct Node *)node);

						// Get "before" node
						if ((before = Att_FindNode(list, sel)))
						{
							// Is node the first in the list?
							if (sel == 0)
								AddHead((struct List *)list, (struct Node *)node);

							// Position node
							else
								Insert((struct List *)list, (struct Node *)node, before->node.ln_Pred);
						}

						// Add after
						else
							AddTail((struct List *)list, (struct Node *)node);
					}
				}

				// Attach list to gadget
				SetGadgetChoices(data->objlist, GAD_FUNCED_LISTER, data->func_display_list);
			}

			// No selection
			SetGadgetValue(data->objlist, GAD_FUNCED_LISTER, (ULONG)-1);
		}
		ok = 1;
	}

	// Got an IPC port?
	else if (ipc)
	{
		// Another function editor?
		if (id == WINDOW_FUNCTION_EDITOR)
		{
			Att_Node *node;
			FunctionEntry *entry, *copy, *first = 0, *last = 0;
			Point *pos;

			// Store position (screen relative)
			if ((pos = AllocVec(sizeof(Point), 0)))
			{
				pos->x = data->drag.drag_x;
				pos->y = data->drag.drag_y;
			}

			// Copy all?
			if (data->drag_shift)
				node = (Att_Node *)data->function_list->list.lh_Head;

			// Get selection
			else
				node = Att_FindNode(data->function_list, drag_item);

			// Loop until done
			while (node && node->node.ln_Succ)
			{
				// Copy function entry
				copy = (FunctionEntry *)node->data;
				if ((entry = AllocVec(sizeof(FunctionEntry), MEMF_CLEAR)))
				{
					// Copy function data
					entry->type = copy->type;
					strcpy(entry->buffer, copy->buffer);

					// First?
					if (!first)
						first = entry;
					else
						last->node = (Att_Node *)entry;
					last = entry;
				}

				// Only doing one?
				if (!data->drag_shift)
					break;

				// Get next
				node = (Att_Node *)node->node.ln_Succ;
			}

			// Got function?
			if (first)
			{
				// Send entry
				IPC_Command(ipc, FUNCTIONEDIT_COPY_LINE, 0, first, pos, 0);
				ok = 1;
			}
			else
				FreeVec(pos);
		}

		// Permit now we've sent the message
		Permit();
	}

	// Failed?
	if (!ok)
		DisplayBeep(data->window->WScreen);
}

// Copy a function
void functioned_copy_line(FuncEdData *data, FunctionEntry *entry, unsigned short type, Point *pos)
{
	Att_Node *before;

	// See if we were editing something
	if (data->edit_node)
	{
		// Stop editing
		funced_end_edit(data, data->edit_node, 0, 0);

		// Clear edit node if not inserting
		if (type != GAD_FUNCED_INSERT_ENTRY)
			data->edit_node = 0;
	}

	// Got a position?
	if (pos)
	{
		short sel;

		// Get selection
		sel = functioned_get_line(
			data->window, GetObject(data->objlist, GAD_FUNCED_LISTER), pos->x, pos->y, (struct Library *)IntuitionBase);

		// Get node to insert before
		data->edit_node = Att_FindNode(data->func_display_list, sel);
	}

	// Before entry
	before = data->edit_node;

	// Do all entries
	do
	{
		// Create new node
		data->edit_node = funced_new_entry(data, before, entry);

		// Get next
		if (entry)
			entry = (FunctionEntry *)entry->node;
	} while (entry);

	// Valid note?
	if (data->edit_node)
	{
		// Mark this one as new
		data->edit_node->node.lve_Flags = 1;

		// Start editing this entry
		funced_start_edit(data);
	}
}

#undef DOpusBase

// Insert a string into a string gadget
void funced_edit_insertstring(ObjectList *list,
							  ULONG id,
							  char *string,
							  struct Library *DOpusBase,
							  struct Library *IntuitionBase)
{
	char buffer[256], tempbuf[256];
	struct Gadget *gadget;
	short len, addlen = 0;

	// Get gadget
	gadget = GADGET(GetObject(list, id));

	// Get current string
	strcpy(buffer, ((struct StringInfo *)gadget->SpecialInfo)->Buffer);

	// Copy from current position into second buffer
	strcpy(tempbuf, buffer + ((struct StringInfo *)gadget->SpecialInfo)->BufferPos);
	buffer[(len = ((struct StringInfo *)gadget->SpecialInfo)->BufferPos)] = 0;

	// Do we need to add a space?
	if (len > 0 && buffer[len - 1] != ' ' && buffer[len - 1] != '}')
	{
		buffer[len++] = ' ';
		buffer[len] = 0;
		++addlen;
	}

	// Tack on new string
	StrConcat(buffer, string, 255);
	len += strlen(string);

	// Do we need to add a space?
	if (tempbuf[0] && tempbuf[0] != ' ' && tempbuf[0] != '{')
	{
		buffer[len++] = ' ';
		buffer[len] = 0;
		++addlen;
	}

	// Add previous suffix
	StrConcat(buffer, tempbuf, 255);

	// Set new string in gadget
	SetGadgetValue(list, id, (IPTR)buffer);

	// Bump buffer position
	((struct StringInfo *)gadget->SpecialInfo)->BufferPos += addlen + strlen(string);

	// Activate gadget
	ActivateGadget(gadget, list->window, 0);
}

// Get rexx arguments from a script
BOOL funced_rexx_args(char *name, char *buffer)
{
	char *buf, *ptr;
	BPTR file;
	short size, len = 0;
	short got_rexx = 0;

	// Allocate buffer
	if (!(buf = AllocVec(4096, MEMF_CLEAR)))
		return 0;

	// Open file
	if (!(file = Open(name, MODE_OLDFILE)))
	{
		FreeVec(buf);
		return 0;
	}

	// Read data
	size = Read(file, buf, 4096);
	Close(file);

	// Got data?
	if (size > 0)
	{
		// Go through buffer
		for (ptr = buf; size > 0; size--, ptr++)
		{
			// Start of comment?
			if (*ptr == '/' && *(ptr + 1) == '*')
				++got_rexx;

			// End of comment
			else if (*ptr == '*' && *(ptr + 1) == '/')
				--got_rexx;

			// Inside a comment?
			else if (got_rexx)
			{
				// Template introducer?
				if (strnicmp(ptr, "$ARG:", 5) == 0)
				{
					// Start buffer with a space
					*(buffer++) = ' ';

					// Skip spaces
					ptr += 5;
					size -= 5;
					while (*ptr == ' ' && size > 0)
					{
						++ptr;
						--size;
					}

					// Copy to buffer
					while (len < 78 && size > 0 && *ptr != '\n')
					{
						*(buffer++) = *(ptr++);
						--size;
						++len;
					}

					// Null-terminate buffer
					*buffer = 0;

					// Free data
					FreeVec(buf);
					return 1;
				}
			}
		}
	}

	// Free buffer, nothing found
	FreeVec(buf);
	return 0;
}
