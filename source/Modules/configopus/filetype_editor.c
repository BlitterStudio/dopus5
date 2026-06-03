#include "config_lib.h"
#include "config_filetypes.h"
#include "config_buttons.h"

/*#define DOpusBase		(data->func_startup.dopus_base)*/
#define Locale (data->func_startup.locale)

/*
#define DOSBase			(data->func_startup.dos_base)
#define IntuitionBase	(data->func_startup.int_base)
#define GfxBase			(data->func_startup.gfx_base)
#define AslBase			(data->func_startup.asl_base)
#define WorkbenchBase	(data->func_startup.wb_base)
#define UtilityBase		(data->func_startup.util_base)
#define LayersBase		(data->func_startup.layers_base)
*/

// Editor init
//#ifdef __amigaos4__
// ULONG ASM _filetypeed_init(REG(a0, IPCData *ipc), REG(a2, int skip), REG(a1, filetype_ed_data *data))
//#else
IPC_StartupCode(_filetypeed_init, filetype_ed_data *, data)
//#endif
{
	short a;

	// Store IPC pointer
	data->ipc = ipc;

	// Fill in new window
	data->new_win.title = data->type->type.name;
	data->new_win.flags = WINDOW_VISITOR | WINDOW_REQ_FILL | WINDOW_AUTO_KEYS;

	// Create timer
	if (!(data->drag.timer = AllocTimer(UNIT_VBLANK, 0)))
		return 0;

	// Open window, create action list
	if (!(data->window = OpenConfigWindow(&data->new_win)) ||
		!(data->objlist = AddObjectList(data->window, data->obj_def)) || !(data->action_list = Att_NewList(0)))
	{
		CloseConfigWindow(data->window);
		FreeTimer(data->drag.timer);
		return 0;
	}

	// Store window for drag
	data->drag.window = data->window;

	// Set window ID
	SetWindowID(data->window, 0, WINDOW_BUTTON_CONFIG, (struct MsgPort *)data->ipc);

	// Build action list
	for (a = 0; data->action_lookup[a]; a += 2)
	{
		Att_NewNode(data->action_list, GetString(Locale, data->action_lookup[a]), data->action_lookup[a + 1], 0);
	}

	// Disable edit and delete buttons
	DisableObject(data->objlist, GAD_FILETYPES_EDIT_ACTION, TRUE);

	// Update action and icon list
	filetypeed_update_actions(data);
	filetypeed_update_iconmenu(data);

	// Initialise process list
	InitListLock(&data->proc_list, 0);
	return 1;
}

void FiletypeEditor(void)
{
	filetype_ed_data *data = 0;
	IPCData *ipc;
	short success = 0, pending_quit = 0;
	BOOL change_flag = 0;

	// Do startup
	if (!(ipc = Local_IPC_ProcStartup((IPTR *)&data, (APTR)&_filetypeed_init)))
		return;

	// Create App stuff
	if ((data->app_port = CreateMsgPort()))
	{
		data->app_window = AddAppWindowA(0, 0, data->window, data->app_port, 0);
	}

	// Get icon image
	if (data->type->icon_path)
	{
#ifdef USE_DRAWICONSTATE
		{
			char *path_copy;
			data->icon_image = NULL;
			if ((path_copy = AllocMemH(0, strlen(data->type->icon_path) + 1)))
			{
				// icon_path is guaranteed to have a .info at the end
				stccpy(path_copy, data->type->icon_path, strlen(data->type->icon_path) - 4);
				data->icon_image = GetCachedDiskObject(path_copy, 0);
				FreeMemH(path_copy);
			}
		}
#else
		data->icon_image = OpenImage(data->type->icon_path, 0);
#endif
		// Show icon image
		filetypeed_show_icon(data);
	}

	// Launch class editor immediately?
	if (data->edit_flag)
		filetypeed_edit_definition(data);

	// Message loop
	FOREVER
	{
		IPCMessage *msg;
		struct IntuiMessage *imsg;
		short break_flag = 0;

		// Check drag
		if (config_drag_check(&data->drag))
		{
			// End drag
			filetypeed_end_drag(data, 0);
		}

		// Task message?
		while ((msg = (IPCMessage *)GetMsg(ipc->command_port)))
		{
			switch (msg->command)
			{
			// Close message?
			case IPC_QUIT:
				if (!pending_quit)
				{
					success = msg->flags;
					break_flag = 1;
				}
				break;

			// Activate
			case IPC_ACTIVATE:
				if (data->window)
				{
					WindowToFront(data->window);
					ActivateWindow(data->window);
				}
				break;

			// Editor saying goodbye
			case IPC_GOODBYE: {
				IPTR which;

				// What's just gone?
				which = IPC_GetGoodbye(msg);

				// Class editor?
				if (which == (IPTR)-1)
					data->class_editor = 0;

				// Icon menu editor?
				else if (which > 15)
				{
					Att_Node *node;

					// Go through icon list
					for (node = (Att_Node *)data->icon_list->list.lh_Head; node->node.ln_Succ;
						 node = (Att_Node *)node->node.ln_Succ)
					{
						// Match function
						if (((func_node *)node->data)->func == (Cfg_Function *)which)
						{
							// Clear editor pointer
							((func_node *)node->data)->editor = 0;

							// Check for invalid function
							if (filetypeed_check_iconmenu(data, node, FALSE))
								change_flag = 1;
							break;
						}
					}
				}

				// Normal filetype editor
				else
					data->editor[which] = 0;
			}
			break;

			// Editor returning a function
			case FUNCTIONEDIT_RETURN: {
				short ret;

				if ((ret = filetypeed_receive_edit(data, (FunctionReturn *)msg->data)))
				{
					change_flag = 1;
					filetypeed_update_actions(data);
					if (ret == 2)
						filetypeed_update_iconmenu(data);
				}
			}
			break;

			// Class editor returning
			case CLASSEDIT_RETURN:
				filetypeed_receive_class(data, (Cfg_Filetype *)msg->data);
				change_flag = 1;
				break;

			// Get a copy of a button
			case BUTTONEDIT_CLIP_BUTTON:

				// Handle this button
				if (filetypeed_get_button(data, (Cfg_Button *)msg->data, (Point *)msg->data_free))
					change_flag = 1;
				break;
			}

			// Reply the message
			IPC_Reply(msg);
		}

		// Intuimessage
		if (data->window)
		{
			while ((imsg = GetWindowMsg(data->window->UserPort)))
			{
				struct IntuiMessage msg_copy;
				struct Gadget *gadget;
				struct TagItem *tags;

				// Copy message
				msg_copy = *imsg;

				// Don't reply to IDCMPUPDATE messages just yet
				if (imsg->Class != IDCMP_IDCMPUPDATE)
				{
					ReplyWindowMsg(imsg);
					imsg = 0;
				}

				// Get gadget and tag pointers
				gadget = (struct Gadget *)msg_copy.IAddress;
				tags = (struct TagItem *)gadget;

				// Look at message
				switch (msg_copy.Class)
				{
				// Close window
				case IDCMP_CLOSEWINDOW:
					if (!pending_quit)
						break_flag = 1;
					break;

				// Gadget
				case IDCMP_GADGETUP:
					switch (gadget->GadgetID)
					{
					// Use
					case GAD_FILETYPEED_USE:
						success = 1;

					// Cancel
					case GAD_FILETYPEED_CANCEL:
						if (!pending_quit)
							break_flag = 1;
						break;

					// Select a function
					case GAD_FILETYPEED_ACTION_LIST: {
						Att_Node *node;

						// Get selected node
						if (!(node = Att_FindNode(data->action_list, msg_copy.Code)))
							break;

						// Enable edit action button
						DisableObject(data->objlist, GAD_FILETYPES_EDIT_ACTION, FALSE);

						// Double-click?
						if (!(DoubleClick(data->last_sec, data->last_mic, msg_copy.Seconds, msg_copy.Micros)) ||
							node != data->last_sel)
						{
							data->last_sec = msg_copy.Seconds;
							data->last_mic = msg_copy.Micros;
							data->last_sel = node;
							data->last_icon = 0;
							break;
						}
					}

						// Fall through

					case GAD_FILETYPES_EDIT_ACTION:

						// No current selection?
						if (!data->last_sel)
							break;

						// Is editor already up for this action?
						if (data->editor[data->last_sel->data])
							IPC_Command(data->editor[data->last_sel->data], IPC_ACTIVATE, 0, 0, 0, 0);

						// Need to launch editor
						else
							filetypeed_edit_action(data, data->last_sel->data, data->last_sel->node.ln_Name);
						break;

					// Delete action
					case GAD_FILETYPES_DEL_ACTION:

						// No current selection?
						if (!data->last_sel)
							break;

						// Is editor up for this action?
						if (data->editor[data->last_sel->data])
							IPC_Command(data->editor[data->last_sel->data], IPC_QUIT, 0, 0, 0, 0);

						// Delete it
						if (filetypeed_del_action(data, data->last_sel->data))
							change_flag = 1;
						break;

					// Edit filetype definition
					case GAD_FILETYPEED_EDIT_CLASS:

						// Is class editor already up for this action?
						if (data->class_editor)
							IPC_Command(data->class_editor, IPC_ACTIVATE, 0, 0, 0, 0);

						// Need to launch editor
						else
							filetypeed_edit_definition(data);
						break;

					// Select icon
					case GAD_FILETYPEED_SELECT_ICON:
						if (filetypeed_pick_icon(data))
							change_flag = 1;
						break;

					// Add to icon menu
					case GAD_FILETYPES_ADD_ICON_MENU:
						filetypeed_add_iconmenu(data);
						break;

					// Select an icon menu
					case GAD_FILETYPES_ICON_MENU: {
						Att_Node *last = data->last_icon;

						// Handle selection
						if (!(filetypeed_sel_icon(data, msg_copy.Code)))
							break;

						// Double-click?
						if (data->last_icon != last ||
							!(DoubleClick(data->last_sec, data->last_mic, msg_copy.Seconds, msg_copy.Micros)))
						{
							data->last_sec = msg_copy.Seconds;
							data->last_mic = msg_copy.Micros;
							data->last_sel = 0;
							break;
						}
					}

						// Fall through

					case GAD_FILETYPES_EDIT_ICON_MENU:

						// No current selection?
						if (!data->last_icon)
							break;

						// Edit it
						filetypeed_edit_iconmenu(data, data->last_icon);
						break;

					// Delete from icon menu
					case GAD_FILETYPES_DEL_ICON_MENU:

						// No current selection?
						if (!data->last_icon)
							break;

						// Delete function
						if (filetypeed_check_iconmenu(data, data->last_icon, TRUE))
							change_flag = 1;
						break;

					// Move icon menu entry up or down
					case GAD_FILETYPES_UP_ICON_MENU:
					case GAD_FILETYPES_DOWN_ICON_MENU:

						// Need a selection
						if (!data->last_icon)
							break;

						// Move it
						if (filetypeed_move_iconmenu(data, gadget->GadgetID == GAD_FILETYPES_UP_ICON_MENU))
							change_flag = 1;
						break;
					}
					break;

				// BOOPSI message
				case IDCMP_IDCMPUPDATE: {
					short item;

					// Icon list?
					if (GetTagData(GA_ID, 0, tags) != GAD_FILETYPES_ICON_MENU)
						break;

					// Get item
					if ((item = GetTagData(DLV_DragNotify, -1, tags)) != -1)
					{
						// Handle selection
						filetypeed_sel_icon(data, item);

						// Start the drag
						config_drag_start(&data->drag, data->icon_list, item, tags, TRUE);
					}
				}
				break;

				// Ticks
				case IDCMP_INTUITICKS:
					++data->drag.tick_count;
					break;

				// Mouse move
				case IDCMP_MOUSEMOVE:

					// Handle drag move
					config_drag_move(&data->drag);
					break;

				// Mouse buttons
				case IDCMP_MOUSEBUTTONS:

					// Valid drag info?
					if (data->drag.drag)
					{
						short ok = -1;

						// Dropped ok?
						if (msg_copy.Code == SELECTUP)
						{
							// Remember last position
							data->drag.drag_x = data->window->WScreen->MouseX;
							data->drag.drag_y = data->window->WScreen->MouseY;
							ok = 1;
						}

						// Aborted
						else if (msg_copy.Code == MENUDOWN)
							ok = 0;

						// End drag?
						if (ok != -1 && filetypeed_end_drag(data, ok))
							change_flag = 1;
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
						IPC_Command(
							data->func_startup.main_owner, IPC_HELP, (1 << 31), "File Type Editor", 0, REPLY_NO_PORT);

						// Clear busy pointer
						ClearWindowBusy(data->window);
					}
					break;
				}

				// Reply to outstanding messages
				if (imsg)
					ReplyWindowMsg(imsg);
			}

			// Check break flag
			if (break_flag || pending_quit)
			{
				// See if all the editors are gone
				if (IsListEmpty(&data->proc_list.list))
					break;

				// Send quit?
				if (break_flag)
				{
					IPC_ListQuit(&data->proc_list, 0, success, FALSE);
					SetWindowBusy(data->window);
				}
				pending_quit = 1;
			}
		}

		// AppMessage
		if (data->app_window)
		{
			struct AppMessage *msg;

			while ((msg = (struct AppMessage *)GetMsg(data->app_port)))
			{
				// Got an argument?
				if (msg->am_NumArgs > 0)
				{
					char name[256];
					short len;
					APTR image;

					// Get full name
					NameFromLock(msg->am_ArgList[0].wa_Lock, name, 256);
					if (msg->am_ArgList[0].wa_Name && *msg->am_ArgList[0].wa_Name)
						AddPart(name, (STRPTR)msg->am_ArgList[0].wa_Name, 256);

					// Add .info
					if ((len = strlen(name)) < 6 || stricmp(name + len - 5, ".info") != 0)
						strcat(name, ".info");

						// Try to get image
#ifdef USE_DRAWICONSTATE
					{
						char *path_copy;
						image = NULL;
						if ((path_copy = AllocMemH(0, strlen(name) + 1)))
						{
							// icon_path is guaranteed to have a .info at the end
							stccpy(path_copy, name, strlen(name) - 4);
							image = GetCachedDiskObject(path_copy, 0);
							FreeMemH(path_copy);
						}
					}
#else
					image = OpenImage(name, 0);
#endif

					if (image)
					{
						// Store path
						FreeMemH(data->type->icon_path);
						if ((data->type->icon_path = AllocMemH(0, strlen(name) + 1)))
							strcpy(data->type->icon_path, name);

							// Free existing image
#ifdef USE_DRAWICONSTATE
						FreeCachedDiskObject(data->icon_image);
#else
						CloseImage(data->icon_image);
#endif
						data->icon_image = image;

						// Show new image
						filetypeed_show_icon(data);
						change_flag = 1;
					}
				}

				// Reply message
				ReplyMsg((struct Message *)msg);
			}
		}

		Wait(1 << ipc->command_port->mp_SigBit | 1 << data->drag.timer->port->mp_SigBit |
			 ((data->window) ? (1 << data->window->UserPort->mp_SigBit) : 0) |
			 ((data->app_window) ? (1 << data->app_port->mp_SigBit) : 0));
	}

	// End any drag in progress
	filetypeed_end_drag(data, 0);

	// Need to send button back?
	if (success == 1 && change_flag)
	{
		if (IPC_Command(data->owner_ipc, FILETYPEEDIT_RETURN, (IPTR)data->type, data->node, 0, REPLY_NO_PORT))
		{
			data->node = 0;
		}
	}

	// Free edit filetype
	FreeFiletype(data->type);

	// Remove AppWindow
	RemoveAppWindow(data->app_window);

	// Close window
	CloseConfigWindow(data->window);

	// Close app port
	if (data->app_port)
	{
		struct Message *msg;
		while ((msg = GetMsg(data->app_port)))
			ReplyMsg(msg);
		DeleteMsgPort(data->app_port);
	}

	// Say goodbye
	IPC_Goodbye(ipc, data->owner_ipc, (success == -1) ? 0 : (IPTR)data->node);

	// Free icon image
#ifdef USE_DRAWICONSTATE
	FreeCachedDiskObject(data->icon_image);
#else
	CloseImage(data->icon_image);
#endif

	// Close timer
	FreeTimer(data->drag.timer);

	// Free data
	IPC_Free(ipc);
	Att_RemList(data->action_list, 0);
	Att_RemList(data->icon_list, REMLIST_FREEDATA);
	FreeVec(data);
}

// Update checkmarks in action list
void filetypeed_update_actions(filetype_ed_data *data)
{
	Att_Node *node;

	// Detach list from listview
	SetGadgetChoices(data->objlist, GAD_FILETYPEED_ACTION_LIST, (APTR)~0);

	// Go through action list
	for (node = (Att_Node *)data->action_list->list.lh_Head; node->node.ln_Succ; node = (Att_Node *)node->node.ln_Succ)
	{
		Cfg_Function *func;

		// Initially not selected
		node->node.lve_Flags &= ~LVEF_SELECTED;

		// Go through function list
		for (func = (Cfg_Function *)data->type->function_list.lh_Head; func->node.ln_Succ;
			 func = (Cfg_Function *)func->node.ln_Succ)
		{
			// Right type?
			if (func->function.func_type == node->data && !(func->function.flags2 & FUNCF2_LABEL_FUNC))
			{
				node->node.lve_Flags |= LVEF_SELECTED;
				break;
			}
		}
	}

	// Reattach list
	SetGadgetChoices(data->objlist, GAD_FILETYPEED_ACTION_LIST, data->action_list);
}

// Edit a filetype action
void filetypeed_edit_action(filetype_ed_data *data, short action, char *name)
{
	Cfg_Function *function;
	FunctionStartup *startup;
	BOOL success = 0;

	// Allocate startup data
	if (!(startup = AllocVec(sizeof(FunctionStartup), MEMF_CLEAR)))
		return;

	// Find the function we want to edit
	if (!(function = FindFunctionType(&data->type->function_list, action)))
	{
		// Allocate a new function
		function = NewFunction(0, action);
	}

	// Otherwise, copy the function
	else
		function = CopyFunction(function, 0, 0);

	// Valid function?
	if (function)
	{
		// Copy startup data
		*startup = data->func_startup;
		startup->window = data->window;
		startup->function = function;
		startup->owner_ipc = data->ipc;
		startup->object = data->type;
		startup->object_flags = action;

		// Build title
		lsprintf(startup->title, "%s : %s", (IPTR)data->type->type.name, (IPTR)name);

		// Launch editor
		if ((IPC_Launch(&data->proc_list,
						&data->editor[action],
						"dopus_function_editor",
						IPC_NATIVE(FunctionEditor),
						STACK_DEFAULT,
						(IPTR)startup,
						(struct Library *)DOSBase)) &&
			data->editor[action])
			success = 1;
	}

	// Free data if not successful
	if (!success)
	{
		FreeFunction(function);
		FreeVec(startup);
	}
}

// Receive edit from function editor
short filetypeed_receive_edit(filetype_ed_data *data, FunctionReturn *ret)
{
	Cfg_Function *function = 0;
	Cfg_Function *func_copy = 0;
	BOOL no_func = 0, icon = 0;
	short success = 0;

	// Icon menu? (this is a bit kludgy)
	if (ret->object_flags > 15)
		icon = 1;

	// Is the new function empty?
	if (IsMinListEmpty(&ret->function->instructions))
		no_func = 1;

	// Icon menu entries are identified by their label; if the user
	// cleared it, the entry would be invisible in the listview but
	// still saved to disk. Treat such an entry as empty so the old
	// function gets removed instead. Match filetypeed_update_iconmenu's
	// "first INST_LABEL wins" semantics so the predicate stays in
	// sync with how the listview is populated.
	else if (icon)
	{
		Cfg_Instruction *ins;
		BOOL has_label = FALSE;

		for (ins = (Cfg_Instruction *)ret->function->instructions.mlh_Head;
			 ins->node.mln_Succ;
			 ins = (Cfg_Instruction *)ins->node.mln_Succ)
		{
			if (ins->type == INST_LABEL)
			{
				has_label = (BOOL)(ins->string && ins->string[0]);
				break;
			}
		}

		if (!has_label)
			no_func = 1;
	}

	// Try to copy the new function
	if (no_func || (func_copy = CopyFunction(ret->function, 0, 0)))
	{
		// Look up the function we are replacing
		if (icon)
		{
			Att_Node *node;

			// Go through icon list
			for (node = (Att_Node *)data->icon_list->list.lh_Head; node->node.ln_Succ;
				 node = (Att_Node *)node->node.ln_Succ)
			{
				// Match function
				if (((func_node *)node->data)->func == (Cfg_Function *)(IPTR)ret->object_flags)
				{
					// Get function pointer
					function = (Cfg_Function *)(IPTR)ret->object_flags;
					break;
				}
			}
		}

		// Otherwise, find the function we want to replace
		else
			function = FindFunctionType(&data->type->function_list, ret->object_flags);

		// Add new function to the list
		if (func_copy)
		{
			// Fix type
			if (!icon)
				func_copy->function.func_type = ret->object_flags;

			// The user accepted this entry via Use, so it is no
			// longer a placeholder. CopyFunction propagates flags2
			// verbatim, so the bit was inherited from the original;
			// clear it now so check_iconmenu / a later save path
			// never see a "needs cleanup" marker on a real entry.
			func_copy->function.flags2 &= ~FUNCF2_PLACEHOLDER;

			// Add to list, in same position if applicable
			if (function)
				Insert(&data->type->function_list, &func_copy->node, &function->node);
			else
				AddTail(&data->type->function_list, &func_copy->node);
		}

		// Free old function
		if (function)
		{
			// Remove function and free it
			Remove(&function->node);
			FreeFunction(function);
		}

		success = (icon) ? 2 : 1;
	}

	return success;
}

// Edit a filetype definition
void filetypeed_edit_definition(filetype_ed_data *data)
{
	fileclass_ed_data *startup;
	BOOL success = 0;

	// Allocate startup data
	if (!(startup = AllocVec(sizeof(fileclass_ed_data), MEMF_CLEAR)) || !(startup->type = CopyFiletype(data->type, 0)))
	{
		FreeVec(startup);
		return;
	}

	// Supply IPC ports
	startup->owner_ipc = data->ipc;
	startup->main_owner = data->func_startup.main_owner;

	// Supply some library pointers
	/*startup->dopus_base=DOpusBase;
	startup->dos_base=(struct Library *)DOSBase;
	startup->int_base=(struct Library *)IntuitionBase;
	startup->gfx_base=(struct Library *)GfxBase;
	startup->asl_base=AslBase;*/

	// Fill out new window
	startup->new_win.parent = data->window;
	startup->new_win.dims = data->class_win;
	startup->new_win.title = data->type->type.name;
	startup->new_win.locale = Locale;
	startup->new_win.flags = WINDOW_VISITOR | WINDOW_REQ_FILL | WINDOW_AUTO_KEYS;

	// Supply object pointers
	startup->obj_def = data->class_obj;
	startup->lookup = data->class_lookup;
	startup->strings = data->class_strings;

	// Initialise reader list
	NewList((struct List *)&startup->readers);

	// Launch editor
	if ((IPC_Launch(&data->proc_list,
					&data->class_editor,
					"dopus_class_editor",
					IPC_NATIVE(FileclassEditor),
					STACK_DEFAULT,
					(IPTR)startup,
					(struct Library *)DOSBase)) &&
		data->class_editor)
		success = 1;

	// Free data if not successful
	if (!success)
	{
		FreeFiletype(startup->type);
		FreeVec(startup);
	}
}

// Receive an edited class definition
void filetypeed_receive_class(filetype_ed_data *data, Cfg_Filetype *type)
{
	// Store name, id and priority
	strcpy(data->type->type.name, type->type.name);
	strcpy(data->type->type.id, type->type.id);
	data->type->type.priority = type->type.priority;

	// Free recognition string
	FreeMemH(data->type->recognition);
	data->type->recognition = 0;

	// Allocate new recognition
	if (type->recognition &&
		(data->type->recognition = AllocMemH(0, strlen((char *)type->recognition) + 1)))
		strcpy((char *)data->type->recognition, (char *)type->recognition);

	// Set new window title
	if (data->window)
	{
		SetWindowTitles(data->window, data->type->type.name, (char *)-1);
	}
}

// Show icon image
void filetypeed_show_icon(filetype_ed_data *data)
{
	struct Rectangle bounds;
#ifndef USE_DRAWICONSTATE
	struct TagItem tags[2];
	ImageRemap remap;
#endif

	// Clear icon area
	SetGadgetValue(data->objlist, GAD_FILETYPEED_ICON_AREA, 0);

	// Valid icon?
	if (!data->icon_image)
	{
		// An icon path is configured but the image could not be
		// loaded - draw a "?" placeholder so the user knows the
		// referenced file is missing rather than no icon being set.
		if (data->type->icon_path && data->type->icon_path[0] &&
			GetObjectRect(data->objlist, GAD_FILETYPEED_ICON_AREA, &bounds))
		{
			short text_width;

			// Match the inset used by the icon-image draw below
			// so the "?" stays clear of the recessed border.
			bounds.MinX += 2;
			bounds.MinY += 1;
			bounds.MaxX -= 2;
			bounds.MaxY -= 1;

			text_width = TextLength(data->window->RPort, (char *)"?", 1);

			SetAPen(data->window->RPort, DRAWINFO(data->window)->dri_Pens[TEXTPEN]);
			SetDrMd(data->window->RPort, JAM1);
			Move(data->window->RPort,
				 bounds.MinX + (((bounds.MaxX - bounds.MinX) + 1 - text_width) >> 1),
				 bounds.MinY +
					 (((bounds.MaxY - bounds.MinY) + 1 - data->window->RPort->TxHeight) >> 1) +
					 data->window->RPort->TxBaseline);
			Text(data->window->RPort, (char *)"?", 1);
		}
		return;
	}

	// Get icon area
	if (!(GetObjectRect(data->objlist, GAD_FILETYPEED_ICON_AREA, &bounds)))
		return;

	// Move bounds inwards
	bounds.MinX += 2;
	bounds.MinY += 1;
	bounds.MaxX -= 2;
	bounds.MaxY -= 1;

#ifdef USE_DRAWICONSTATE
	{
		struct Rectangle rect;
		int x = 0, y = 0;

		if (GetIconRectangle(data->window->RPort, data->icon_image, NULL, &rect, ICONDRAWA_Borderless, TRUE, TAG_DONE))
		{
			x = (bounds.MaxX - bounds.MinX - (rect.MaxX - rect.MinX)) >> 1;
			y = (bounds.MaxY - bounds.MinY - (rect.MaxY - rect.MinY)) >> 1;
		}

		DrawIconState(data->window->RPort,
					  data->icon_image,
					  NULL,
					  x + bounds.MinX,
					  y + bounds.MinY,
					  /*data->image_num ? IDS_SELECTED :*/ IDS_NORMAL,
					  ICONDRAWA_Frameless,
					  TRUE,
					  ICONDRAWA_Borderless,
					  TRUE,
					  ICONDRAWA_EraseBackground,
					  TRUE,
					  TAG_DONE);
	}
#else
	// Fill out render tags
	tags[0].ti_Tag = IM_Rectangle;
	tags[0].ti_Data = (ULONG)&bounds;
	tags[1].ti_Tag = TAG_DONE;

	// Remap the icon
	remap.ir_PenArray = 0;
	RemapImage(data->icon_image, data->window->WScreen, &remap);

	// Display icon
	RenderImage(data->window->RPort, data->icon_image, 0, 0, tags);

	// Free icon remap
	FreeRemapImage(data->icon_image, &remap);
	FreeImageRemap(&remap);
#endif
}

// Pick icon image
BOOL filetypeed_pick_icon(filetype_ed_data *data)
{
	char path[256], file[32], *ptr, pattern[18];
	BOOL ret = 0;

	// Make window busy
	SetWindowBusy(data->window);

	// Get current path
	path[0] = 0;
	file[0] = 0;
	if (data->type->icon_path && data->type->icon_path[0])
	{
		if ((ptr = strstr(data->type->icon_path, ":")))
		{
			strcpy(path, data->type->icon_path);
			/* FilePart returns CONST_STRPTR on AOS4; we know `path`
			 * is non-const (it's our local buffer) so the cast is
			 * safe and silences the discarded-qualifier warning. */
			if ((ptr = (char *)FilePart(path)))
			{
				strcpy(file, ptr);
				*ptr = 0;
			}
		}
	}

	if (!path[0])
	{
		BPTR lock = Lock("ENVARC:Sys", ACCESS_READ);
		if (lock)
		{
			NameFromLock(lock, path, sizeof(path));
			UnLock(lock);
		}
		else
		{
			path[0] = 0;
		}
	}

	// Build pattern. ParsePatternNoCase's buffer arg is STRPTR (char*)
	// on AOS3/AOS4 but UBYTE* on MorphOS, so use the matching cast on
	// each target to keep all toolchains warning-clean.
#ifdef __MORPHOS__
	ParsePatternNoCase("#?.info", (UBYTE *)pattern, 18);
#else
	ParsePatternNoCase("#?.info", (STRPTR)pattern, 18);
#endif

	// Display requester
	if (AslRequestTags(DATA(data->window)->request,
					   ASLFR_Flags1,
					   FRF_PRIVATEIDCMP,
					   ASLFR_Window,
					   data->window,
					   ASLFR_TitleText,
					   GetString(Locale, MSG_FILETYPE_PICK_ICON),
					   ASLFR_InitialDrawer,
					   path,
					   ASLFR_InitialFile,
					   file,
					   ASLFR_InitialPattern,
					   "#?.info",
					   ASLFR_AcceptPattern,
					   pattern,
					   TAG_END))
	{
		char *reqpath = DATA(data->window)->request->fr_Drawer;
		BPTR lock = 0;

		// Free existing path
		FreeMemH(data->type->icon_path);
		data->type->icon_path = 0;

		// Build new path
		if (reqpath[0] && (lock = Lock(reqpath, ACCESS_READ)))
		{
			// Copy drawer path if it's in DOpus5: tree (case-insensitive,
			// since the assign is registered as "DOPUS5")
			if (strnicmp(reqpath, "DOpus5:", 7) == 0 && !strstr(reqpath, ":/"))
				strcpy(path, reqpath);
			// Get absolute path to avoid relative paths
			else if (!(NameFromLock(lock, path, sizeof(path))))
				path[0] = 0;
			UnLock(lock);
		}
		else
		{
			path[0] = 0;
		}

		// Add filename if path is valid
		if (path[0])
			AddPart(path, DATA(data->window)->request->fr_File, 256);

		// Throw it away if it's not an icon
		if (!(ptr = strstr(path, ".info")))
			path[0] = 0;

		// Store new icon path
		if (path[0] && (data->type->icon_path = AllocMemH(0, strlen(path) + 1)))
			strcpy(data->type->icon_path, path);

			// Free existing image
#ifdef USE_DRAWICONSTATE
		FreeCachedDiskObject(data->icon_image);
#else
		CloseImage(data->icon_image);
#endif

		// Get new image
		if (path[0])
#ifdef USE_DRAWICONSTATE
		{
			char *path_copy;
			data->icon_image = NULL;
			if ((path_copy = AllocMemH(0, strlen(path) + 1)))
			{
				// icon_path is guaranteed to have a .info at the end
				stccpy(path_copy, path, strlen(path) - 4);
				data->icon_image = GetCachedDiskObject(path_copy, 0);
				FreeMemH(path_copy);
			}
		}
#else
			data->icon_image = OpenImage(path, 0);
#endif
		else
			data->icon_image = NULL;

		// Show new image
		filetypeed_show_icon(data);
		ret = 1;
	}

	// Make window unbusy
	ClearWindowBusy(data->window);

	return ret;
}

// Update icon menu list
void filetypeed_update_iconmenu(filetype_ed_data *data)
{
	Cfg_Function *func;

	// Detach list from listview
	SetGadgetChoices(data->objlist, GAD_FILETYPES_ICON_MENU, (APTR)~0);

	// Delete icon list
	Att_RemList(data->icon_list, REMLIST_FREEDATA);

	// Create new list
	if ((data->icon_list = Att_NewList(0)))
	{
		// Go through filetype functions
		for (func = (Cfg_Function *)data->type->function_list.lh_Head; func->node.ln_Succ;
			 func = (Cfg_Function *)func->node.ln_Succ)
		{
			// Label function?
			if (func->function.flags2 & FUNCF2_LABEL_FUNC)
			{
				Cfg_Instruction *ins;

				// Go through instruction list
				for (ins = (Cfg_Instruction *)func->instructions.mlh_Head; ins->node.mln_Succ;
					 ins = (Cfg_Instruction *)ins->node.mln_Succ)
				{
					// Label?
					if (ins->type == INST_LABEL)
					{
						func_node *node;

						// Allocate node data
						if ((node = AllocVec(sizeof(func_node), MEMF_CLEAR)))
						{
							// Fill out data
							node->func = func;

							// Create node
							if (!(Att_NewNode(data->icon_list, ins->string, (IPTR)node, 0)))
								FreeVec(node);
						}
						break;
					}
				}
			}
		}
	}

	// Attach list
	SetGadgetChoices(data->objlist, GAD_FILETYPES_ICON_MENU, data->icon_list);

	// No selection
	filetypeed_no_iconsel(data);
}

// Add to icon menu
void filetypeed_add_iconmenu(filetype_ed_data *data)
{
	Cfg_Function *func;
	Att_Node *node;
	func_node *fndata;
	Cfg_Instruction *label_ins;
	CONST_STRPTR default_label;

	// Allocate a new function and data
	if (!(func = NewFunction(0, FTYPE_LIST)) || !(fndata = AllocVec(sizeof(func_node), MEMF_CLEAR)))
	{
		FreeFunction(func);
		return;
	}

	// Detach list from listview
	SetGadgetChoices(data->objlist, GAD_FILETYPES_ICON_MENU, (APTR)~0);

	// Fill out data, marking the function as a placeholder so the
	// IPC_GOODBYE handler (via filetypeed_check_iconmenu) can drop
	// the row if the user cancels the editor instead of clicking
	// Use. See FUNCF2_PLACEHOLDER's comment in config_filetypes.h
	// for why the flag lives on the Cfg_Function rather than on
	// func_node. filetypeed_receive_edit clears the bit when the
	// user accepts the entry via Use.
	fndata->func = func;
	func->function.flags2 |= FUNCF2_LABEL_FUNC | FUNCF2_PLACEHOLDER;

	// Pre-populate with a default label so the new entry has a name
	// even if the user clicks Use without touching the Label gadget.
	// filetypeed_receive_edit would otherwise drop the entry on save
	// (it requires a non-empty INST_LABEL, matching update_iconmenu's
	// listview rule). The editor reads this same INST_LABEL into its
	// Label gadget, so the user can keep "Untitled", rename it, or
	// clear it as they like.
	default_label = GetString(Locale, MSG_UNTITLED);
	if ((label_ins = NewInstruction(0, INST_LABEL, (char *)default_label)))
		AddHead((struct List *)&func->instructions, (struct Node *)label_ins);

	// Add to lister
	if (!(node = Att_NewNode(data->icon_list, (char *)default_label, (IPTR)fndata, 0)))
	{
		// Failed
		FreeVec(fndata);
		FreeFunction(func);
	}

	// Add function to list
	else
		AddTail(&data->type->function_list, &func->node);

	// Attach list
	SetGadgetChoices(data->objlist, GAD_FILETYPES_ICON_MENU, data->icon_list);

	// Edit node
	if (node)
		filetypeed_edit_iconmenu(data, node);
}

// Edit icon menu node
void filetypeed_edit_iconmenu(filetype_ed_data *data, Att_Node *node)
{
	Cfg_Function *function;
	FunctionStartup *startup;
	func_node *fndata;
	BOOL success = 0;

	// Get the function to edit
	if (!node || !(fndata = (func_node *)node->data))
		return;

	// Editor already open?
	if (fndata->editor)
	{
		// Bring editor to the front
		IPC_Command(fndata->editor, IPC_ACTIVATE, 0, 0, 0, 0);
		return;
	}

	// Allocate startup data
	if (!(startup = AllocVec(sizeof(FunctionStartup), MEMF_CLEAR)))
		return;

	// Copy the function
	if ((function = CopyFunction(fndata->func, 0, 0)))
	{
		// Copy startup data
		*startup = data->func_startup;
		startup->window = data->window;
		startup->function = function;
		startup->owner_ipc = data->ipc;
		startup->object = data->type;
		startup->object_flags = (IPTR)fndata->func;
		startup->flags |= FUNCEDF_LABEL;

		// Build title
		lsprintf(startup->title,
				 "%s : %s",
				 (IPTR)data->type->type.name,
				 (IPTR)GetString(Locale, MSG_ICON_MENU));

		// Launch editor
		if ((IPC_Launch(&data->proc_list,
						&fndata->editor,
						"dopus_function_editor",
						IPC_NATIVE(FunctionEditor),
						STACK_DEFAULT,
						(IPTR)startup,
						(struct Library *)DOSBase)) &&
			fndata->editor)
			success = 1;
	}

	// Free data if not successful
	if (!success)
	{
		FreeFunction(function);
		FreeVec(startup);
	}
}

// Check for empty function
BOOL filetypeed_check_iconmenu(filetype_ed_data *data, Att_Node *node, BOOL del)
{
	func_node *fndata;

	// Get data pointer
	fndata = (func_node *)node->data;

	// Invalid / placeholder function?
	// - del: explicit delete request
	// - !fndata->func / empty instructions: no real content
	// - FUNCF2_PLACEHOLDER: a fresh row created by Add that the user
	//   cancelled out of. Its instructions list still contains the
	//   seeded INST_LABEL, so the IsMinListEmpty test alone would
	//   miss it. The bit is cleared in receive_edit when Use is
	//   clicked, so accepted rows are never treated as placeholders.
	if (del || !fndata->func || IsMinListEmpty(&fndata->func->instructions) ||
		(fndata->func->function.flags2 & FUNCF2_PLACEHOLDER))
	{
		// Detach list from listview
		SetGadgetChoices(data->objlist, GAD_FILETYPES_ICON_MENU, (APTR)~0);

		// Free function
		Remove(&fndata->func->node);
		FreeFunction(fndata->func);

		// Remove node
		Att_RemNode(node);

		// Is there an editor for this?
		if (fndata->editor)
		{
			// Tell editor to shutdown
			IPC_Command(fndata->editor, IPC_QUIT, 0, 0, 0, 0);
		}

		// Free data
		FreeVec(fndata);

		// Attach list
		SetGadgetChoices(data->objlist, GAD_FILETYPES_ICON_MENU, data->icon_list);

		// No selection
		filetypeed_no_iconsel(data);
		return 1;
	}

	return 0;
}

// Move the selected icon menu entry up or down by one position
BOOL filetypeed_move_iconmenu(filetype_ed_data *data, BOOL up)
{
	Att_Node *neighbour, *anchor;
	Cfg_Function *cur_func, *neighbour_func;
	short pos;

	// Need a selection
	if (!data->last_icon)
		return 0;

	// Find the neighbour we're swapping past
	if (up)
		neighbour = (Att_Node *)data->last_icon->node.ln_Pred;
	else
		neighbour = (Att_Node *)data->last_icon->node.ln_Succ;

	// Already at the boundary? (head/tail sentinel reached)
	if (!neighbour || (up ? !neighbour->node.ln_Pred : !neighbour->node.ln_Succ))
		return 0;

	cur_func = ((func_node *)data->last_icon->data)->func;
	neighbour_func = ((func_node *)neighbour->data)->func;

	// Detach list from listview
	SetGadgetChoices(data->objlist, GAD_FILETYPES_ICON_MENU, (APTR)~0);

	// Reposition icon node next to its neighbour using insert-before
	// semantics. Att_PosNode places the node before "anchor".
	anchor = up ? neighbour : (Att_Node *)neighbour->node.ln_Succ;
	Att_PosNode(data->icon_list, data->last_icon, anchor);

	// Move the corresponding label function the same way in the
	// filetype's full function list (which also contains action
	// functions; only the relative order of label functions matters
	// for the icon menu).
	Remove((struct Node *)cur_func);
	if (up)
		Insert(&data->type->function_list, (struct Node *)cur_func, neighbour_func->node.ln_Pred);
	else
		Insert(&data->type->function_list, (struct Node *)cur_func, (struct Node *)neighbour_func);

	// Reattach list and keep focus on the moved entry. Cache the
	// node-number lookup so we don't pay for two O(n) walks here.
	SetGadgetChoices(data->objlist, GAD_FILETYPES_ICON_MENU, data->icon_list);
	pos = Att_FindNodeNumber(data->icon_list, data->last_icon);
	SetGadgetValue(data->objlist, GAD_FILETYPES_ICON_MENU, pos);

	// Refresh enable/disable state for the new selection position
	filetypeed_sel_icon(data, pos);
	return 1;
}

// End drag
BOOL filetypeed_end_drag(filetype_ed_data *data, BOOL ok)
{
	struct Window *window;
	ULONG id = 0;
	IPCData *ipc = 0;
	BOOL ret = 0;
	short drag_item;

	// Valid drag?
	if (!data->drag.drag)
		return 0;

	// End drag
	if (!(window = config_drag_end(&data->drag, ok)))
		return 0;

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

	// Get drag item
	drag_item = Att_FindNodeNumber(data->icon_list, data->drag.drag_node);

	// Drop on our window?
	if (window == data->window)
	{
		long item;
		IPTR top = 0;
		GL_Object *lister;

		// Convert to window coordinates
		data->drag.drag_x -= data->window->LeftEdge;
		data->drag.drag_y -= data->window->TopEdge;

		// Dropped over list?
		lister = GetObject(data->objlist, GAD_FILETYPES_ICON_MENU);
		if (!(CheckObjectArea(lister, data->drag.drag_x, data->drag.drag_y)))
			return 0;

		// Get top item (GetAttr writes through an IPTR* on AOS4/AROS)
		GetAttr(DLV_Top, (Object *)GADGET(lister), &top);

		// Get item we dropped over
		item = UDivMod32((data->drag.drag_y - lister->dims.Top), data->window->RPort->TxHeight) + (long)top;

		// Valid item?
		if (item >= 0 && item < Att_NodeCount(data->icon_list) && item != drag_item)
		{
			Att_Node *target, *dragged;

			// Get the two nodes
			if ((target = Att_FindNode(data->icon_list, item)) &&
				(dragged = Att_FindNode(data->icon_list, drag_item)))
			{
				Cfg_Function *target_func = ((func_node *)target->data)->func;
				Cfg_Function *dragged_func = ((func_node *)dragged->data)->func;
				BOOL drag_down = (BOOL)(drag_item < item);
				Att_Node *anchor;

				// Detach list from listview
				SetGadgetChoices(data->objlist, GAD_FILETYPES_ICON_MENU, (APTR)~0);

				// Insert-at-target semantics: dragging downward drops the
				// node just after the target; dragging upward drops it
				// just before the target. So the dropped item ends up
				// visually where the user released it.
				anchor = drag_down ? (Att_Node *)target->node.ln_Succ : target;
				Att_PosNode(data->icon_list, dragged, anchor);

				// Mirror the move in the filetype's full function list
				// (which contains action functions interleaved with
				// label functions; only the relative order of label
				// functions matters for the icon menu).
				Remove((struct Node *)dragged_func);
				if (drag_down)
					Insert(&data->type->function_list,
						   (struct Node *)dragged_func,
						   (struct Node *)target_func);
				else
					Insert(&data->type->function_list,
						   (struct Node *)dragged_func,
						   target_func->node.ln_Pred);

				// Attach list and keep the focus on the moved entry
				// (one node-number lookup, used twice).
				SetGadgetChoices(data->objlist, GAD_FILETYPES_ICON_MENU, data->icon_list);
				{
					short pos = Att_FindNodeNumber(data->icon_list, dragged);
					SetGadgetValue(data->objlist, GAD_FILETYPES_ICON_MENU, pos);
					filetypeed_sel_icon(data, pos);
				}
				ret = 1;
			}
		}

		return ret;
	}

	// Another window
	else if (ipc)
	{
		// Valid function?
		if (drag_item != -1)
		{
			Att_Node *node;

			// Get node
			if ((node = Att_FindNode(data->icon_list, drag_item)))
			{
				func_node *fndata;
				Cfg_Function *func = 0;

				// Get function pointer
				if ((fndata = (func_node *)node->data))
					func = fndata->func;

				// Drop on function editor?
				if (id == WINDOW_FUNCTION_EDITOR)
				{
					Point *pos;
					Cfg_Instruction *ins;
					FunctionEntry *entry, *first = 0, *last = 0;

					// Store position (screen relative)
					if ((pos = AllocVec(sizeof(Point), 0)))
					{
						pos->x = data->drag.drag_x;
						pos->y = data->drag.drag_y;
					}

					// Go through functions
					for (ins = (Cfg_Instruction *)func->instructions.mlh_Head; ins->node.mln_Succ;
						 ins = (Cfg_Instruction *)ins->node.mln_Succ)
					{
						// Skip label
						if (ins->type != INST_LABEL)
						{
							// Create a function entry
							if ((entry = AllocVec(sizeof(FunctionEntry), MEMF_CLEAR)))
							{
								// Copy function data
								entry->type = ins->type;
								strcpy(entry->buffer, ins->string);

								// First?
								if (!first)
									first = entry;
								else
									last->node = (Att_Node *)entry;
								last = entry;
							}
						}
					}

					// Send function
					IPC_Command(ipc, FUNCTIONEDIT_COPY_LINE, 0, first, pos, 0);
					ok = 1;
				}

				// Or on a button editor
				else if (id == WINDOW_BUTTON_CONFIG || id == WINDOW_BUTTONS || id == WINDOW_FUNCTION_EDITOR)
				{
					Point *pos;
					Cfg_Button *button;

					// Store position (screen relative)
					if ((pos = AllocVec(sizeof(Point), 0)))
					{
						pos->x = data->drag.drag_x;
						pos->y = data->drag.drag_y;
					}

					// Create a new button
					if ((button = NewButton(0)))
					{
						Cfg_ButtonFunction *function;

						// Create function
						if ((function = NewButtonFunction(0, FTYPE_LEFT_BUTTON)))
						{
							Cfg_Instruction *ins;

							// Copy function data
							CopyFunction(func, 0, (Cfg_Function *)function);

							// Fix type
							function->function.func_type = FTYPE_LEFT_BUTTON;
							function->function.flags2 &= ~FUNCF2_LABEL_FUNC;

							// Go through instructions in function copy
							for (ins = (Cfg_Instruction *)function->instructions.mlh_Head; ins->node.mln_Succ;
								 ins = (Cfg_Instruction *)ins->node.mln_Succ)
							{
								// Label?
								if (ins->type == INST_LABEL)
								{
									// Remove from function
									Remove((struct Node *)ins);

									// Store label
									if ((function->node.ln_Name = AllocMemH(0, strlen(ins->string) + 1)))
										strcpy(function->node.ln_Name, ins->string);
									if ((function->label = AllocMemH(0, strlen(ins->string) + 1)))
										strcpy(function->label, ins->string);

									// Free instruction
									FreeInstruction(ins);
									break;
								}
							}

							// Add function to button
							AddTail((struct List *)&button->function_list, (struct Node *)function);
						}

						// Send button
						IPC_Command(ipc, BUTTONEDIT_CLIP_BUTTON, 0, button, pos, 0);
						ok = 1;
					}

					// Failed
					else
						FreeVec(pos);
				}
			}
		}

		// Permit now the message has been sent
		Permit();
		if (ok)
			return ret;
	}

	// Illegal drop
	DisplayBeep(data->window->WScreen);
	return ret;
}

// Select nothing
void filetypeed_no_iconsel(filetype_ed_data *data)
{
	// No selection
	SetGadgetValue(data->objlist, GAD_FILETYPES_ICON_MENU, (ULONG)-1);
	data->last_icon = 0;

	// Disable edit/delete/move buttons
	DisableObject(data->objlist, GAD_FILETYPES_EDIT_ICON_MENU, TRUE);
	DisableObject(data->objlist, GAD_FILETYPES_DEL_ICON_MENU, TRUE);
	DisableObject(data->objlist, GAD_FILETYPES_UP_ICON_MENU, TRUE);
	DisableObject(data->objlist, GAD_FILETYPES_DOWN_ICON_MENU, TRUE);
}

// Given a button
BOOL filetypeed_get_button(filetype_ed_data *data, Cfg_Button *button, Point *pos)
{
	short x = 0, y = 0;
	BOOL ret = 0;

	// Get window-relative position
	if (pos)
	{
		x = pos->x - data->window->LeftEdge;
		y = pos->y - data->window->TopEdge;
	}

	// Dropped over the icon list?
	if (CheckObjectArea(GetObject(data->objlist, GAD_FILETYPES_ICON_MENU), x, y))
	{
		Cfg_ButtonFunction *func;

		// Find left-button function
		if ((func = (Cfg_ButtonFunction *)FindFunctionType((struct List *)&button->function_list, FTYPE_LEFT_BUTTON)))
		{
			Cfg_Function *newfunc;
			func_node *fndata;

			// Copy function, allocate node data
			if ((newfunc = CopyFunction((Cfg_Function *)func, 0, 0)) &&
				(fndata = AllocVec(sizeof(func_node), MEMF_CLEAR)))
			{
				char *name;
				Att_Node *node;

				// Get name pointer
				if ((name = func->label) || (name = func->node.ln_Name))
				{
					Cfg_Instruction *ins;

					// Create label instruction, add to function
					if ((ins = NewInstruction(0, INST_LABEL, name)))
						AddHead((struct List *)&newfunc->instructions, (struct Node *)ins);
				}

				// Set "label" flag
				newfunc->function.flags2 |= FUNCF2_LABEL_FUNC;

				// Detach list from listview
				SetGadgetChoices(data->objlist, GAD_FILETYPES_ICON_MENU, (APTR)~0);

				// Fill out node entry
				fndata->func = newfunc;

				// Add to node
				if (!(node = Att_NewNode(data->icon_list, name, (IPTR)fndata, 0)))
				{
					// Failed
					FreeVec(fndata);
					FreeFunction(newfunc);
				}

				// Add function to list
				else
				{
					AddTail(&data->type->function_list, &newfunc->node);
					ret = 1;
				}

				// Attach list
				SetGadgetChoices(data->objlist, GAD_FILETYPES_ICON_MENU, data->icon_list);

				// New node?
				if (node)
				{
					// Select this node and refresh edit/delete/move
					// buttons (one node-number lookup, used twice).
					short pos = Att_FindNodeNumber(data->icon_list, node);
					SetGadgetValue(data->objlist, GAD_FILETYPES_ICON_MENU, pos);
					filetypeed_sel_icon(data, pos);
				}
			}

			// Failed
			else if (newfunc)
				FreeFunction(newfunc);
		}
	}

	// Illegal drop
	else
		DisplayBeep(data->window->WScreen);

	// Free new button
	FreeButton(button);
	return ret;
}

// Delete action
BOOL filetypeed_del_action(filetype_ed_data *data, short action)
{
	Cfg_Function *func, *next;
	short count = 0;

	// Go through list
	for (func = (Cfg_Function *)data->type->function_list.lh_Head; func->node.ln_Succ; func = next)
	{
		// Cache next
		next = (Cfg_Function *)func->node.ln_Succ;

		// Right type?
		if (func->function.func_type == action && !(func->function.flags2 & FUNCF2_LABEL_FUNC))
		{
			// Remove function and free it
			Remove((struct Node *)func);
			FreeFunction(func);
			++count;
		}
	}

	// No change?
	if (!count)
		return 0;

	// Update actions list
	filetypeed_update_actions(data);
	return 1;
}

// Select an icon item
BOOL filetypeed_sel_icon(filetype_ed_data *data, short item)
{
	Att_Node *node;
	BOOL at_top, at_bottom;

	// Get selected node
	if (!(node = Att_FindNode(data->icon_list, item)))
		return 0;

	// Enable edit/delete buttons
	DisableObject(data->objlist, GAD_FILETYPES_EDIT_ICON_MENU, FALSE);
	DisableObject(data->objlist, GAD_FILETYPES_DEL_ICON_MENU, FALSE);

	// Up is meaningful only when there's a node before us; Down only
	// when there's a node after us. Sentinel ln_Pred / ln_Succ point at
	// the list head/tail anchor (themselves valid Node structs whose
	// own ln_Pred / ln_Succ are NULL), so test that to detect ends.
	at_top = !node->node.ln_Pred || !node->node.ln_Pred->ln_Pred;
	at_bottom = !node->node.ln_Succ || !node->node.ln_Succ->ln_Succ;
	DisableObject(data->objlist, GAD_FILETYPES_UP_ICON_MENU, at_top);
	DisableObject(data->objlist, GAD_FILETYPES_DOWN_ICON_MENU, at_bottom);

	// Store pointer
	data->last_icon = node;

	return 1;
}
