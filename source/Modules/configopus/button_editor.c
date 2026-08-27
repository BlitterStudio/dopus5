#include "config_lib.h"
#include "config_buttons.h"

void buttoned_copy_string(ButtonEdData *data, char *from, char **to);

/*#define DOpusBase		(data->DOpusBase)
#define DOSBase			(data->DOSBase)
#define IntuitionBase	(data->IntuitionBase)
#define CxBase			(data->CxBase)
#define WorkbenchBase	(data->WorkbenchBase)
#define GfxBase			(data->GfxBase)
#define UtilityBase		(data->UtilityBase)
#define LayersBase		(data->LayersBase)*/

//#ifdef __amigaos4__
// ULONG ASM _buttoned_init(REG(a0, IPCData *ipc), REG(a2, int skip), REG(a1, ButtonEdStartup *startup))
//#else
IPC_StartupCode(_buttoned_init, ButtonEdStartup *, startup)
//#endif
{
	ButtonEdData *data;

	// Allocate data
	if (!(data = AllocVec(sizeof(ButtonEdData), MEMF_CLEAR)))
		return 0;

	// Store data
	startup->data = data;

	// Initialise some pointers
	data->ipc = ipc;
	data->startup = startup;
	data->button = startup->button;
	data->locale = startup->func_startup.locale;

	// Initialise library bases
	/*DOpusBase=startup->func_startup.dopus_base;
	DOSBase=startup->func_startup.dos_base;
	IntuitionBase=startup->func_startup.int_base;
	CxBase=startup->func_startup.cx_base;
	WorkbenchBase=startup->func_startup.wb_base;
	GfxBase=startup->func_startup.gfx_base;
	UtilityBase=startup->func_startup.util_base;
	LayersBase=startup->func_startup.layers_base;*/

	// Initialise process list
	InitListLock(&data->proc_list, 0);

	// Create function list and timer, and allocate signal bit
	if (!(data->func_list = Att_NewList(LISTF_POOL)) || !(data->drag.timer = AllocTimer(UNIT_VBLANK, 0)) ||
		(data->change_bit = AllocSignal(-1)) == -1)
	{
		FreeTimer(data->drag.timer);
		Att_RemList(data->func_list, 0);
		FreeVec(data);
		return 0;
	}

	return 1;
}

void ButtonEditor(void)
{
	ButtonEdStartup *startup = 0;
	ButtonEdData *data;
	IPCData *ipc;
	short success = 0, pending_quit = 0;
	BOOL open_window = 1;

	// Do startup
	if (!(ipc = Local_IPC_ProcStartup((IPTR *)&startup, (APTR)&_buttoned_init)))
		return;

	// Get data pointer
	data = startup->data;
	NewList((struct List *)&data->editors);

	// Fill in new window
	data->newwin.parent = startup->window;
	data->newwin.dims = startup->win_text_def;
	data->newwin.locale = data->locale;
	data->newwin.title = GetString(data->locale, MSG_BUTTONED_TITLE);
	data->newwin.flags = WINDOW_VISITOR | WINDOW_REQ_FILL | WINDOW_AUTO_KEYS;

	// Build function list
	buttoned_build_function_list(data);

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
			buttoned_end_drag(data, 0);
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

			// Hide
			case IPC_HIDE:

				// Tell children to hide
				IPC_ListCommand(&data->proc_list, IPC_HIDE, 0, 0, 0);

				// Close window
				_buttoned_close_window(data);
				break;

			// Show
			case IPC_SHOW:
				data->newwin.parent = (void *)msg->data;
				open_window = 1;
				break;

			// Editor saying goodbye
			case IPC_GOODBYE: {
				ButtonEditorNode *editor;

				// Go through editor list
				for (editor = (ButtonEditorNode *)data->editors.mlh_Head; editor->node.mln_Succ;
					 editor = (ButtonEditorNode *)editor->node.mln_Succ)
				{
					// This the one?
					if ((IPCData *)msg->data == editor->editor)
					{
						// Remove node
						Remove((struct Node *)editor);
						FreeVec(editor);
						break;
					}
				}
			}
			break;

			// Editor returning a function
			case FUNCTIONEDIT_RETURN:
				if (_button_editor_receive_edit(data, (FunctionReturn *)msg->data))
					data->change = 1;
				break;

			// Get a copy of a button
			case BUTTONEDIT_CLIP_BUTTON:

				// Handle this button
				buttoned_get_button(data, (Cfg_Button *)msg->data, (Point *)msg->data_free);
				break;
			}

			// Reply the message
			IPC_Reply(msg);
		}

		// Need to open window?
		if (open_window)
		{
			short a;
			Cfg_ButtonFunction *func;

			// Open window
			if (!(data->window = OpenConfigWindow(&data->newwin)) ||
				!(data->objlist = AddObjectList(data->window, startup->obj_def)))
				break;

			// Store data pointer in window
			DATA(data->window)->data = (IPTR)data;

			// Specific objects for graphical buttons
			if (data->button->button.flags & BUTNF_GRAPHIC)
				AddObjectList(data->window, startup->obj_image);

			// Specific objects for text buttons
			else
				AddObjectList(data->window, startup->obj_name);

			// Store window for dragging
			data->drag.window = data->window;

			// Get function list object
			data->lister = GetObject(data->objlist, GAD_BUTTONED_FUNCTIONS);

			// Fix function list
			buttoned_fix_functionlist(data);

			// Select current function
			SetGadgetValue(data->objlist, GAD_BUTTONED_FUNCTIONS, data->which);

			// Disable Add if necessary
			if (startup->flags & BUTTONEDF_NO_ADD)
				DisableObject(data->objlist, GAD_BUTTONED_ADD, TRUE);

			// Fix palette gadgets
			for (a = GAD_PALETTE_FOREGROUND; a <= GAD_PALETTE_BACKGROUND; a++)
			{
				struct Gadget *gadget;
				struct TagItem tags[3];
				short pen;

				// Get gadget
				gadget = GADGET(GetObject(data->objlist, a));

				// Fill out tags
				tags[0].ti_Tag = GTPA_NumColors;
				tags[0].ti_Data = data->startup->palette_data.screen_data.pen_count +
								  ((((struct Library *)GfxBase)->lib_Version >= 39) ? 8 : 4);
				tags[1].ti_Tag = GTPA_ColorTable;
				tags[1].ti_Data = (IPTR)data->startup->palette_data.pen_array;
				tags[2].ti_Tag = TAG_END;

				// Check number of pens
				if (tags[0].ti_Data > (1 << data->window->RPort->BitMap->Depth))
					tags[0].ti_Data = 1 << data->window->RPort->BitMap->Depth;

				// Get pen
				pen = (a == GAD_PALETTE_FOREGROUND) ? data->button->button.fpen : data->button->button.bpen;

				// Map top colours
				if (((struct Library *)GfxBase)->lib_Version >= 39)
				{
					if (pen >= 252)
						pen -= 248;
					else if (pen >= 4)
						pen += 4;
				}

				// Initialise gadget
				SetGadgetAttrsA(gadget, data->window, 0, tags);

				// Set current selection
				SetGadgetValue(data->objlist, a, pen);
			}

			// No palette editing?
			if (startup->palette_data.flags & PBF_NO_EDIT)
			{
				DisableObject(data->objlist, GAD_PALETTE_FOREGROUND, TRUE);
				DisableObject(data->objlist, GAD_PALETTE_BACKGROUND, TRUE);
			}

			// Disable foreground for graphical button
			else if (data->button->button.flags & BUTNF_GRAPHIC)
				DisableObject(data->objlist, GAD_PALETTE_FOREGROUND, TRUE);

			// Set window ID
			SetWindowID(data->window, 0, WINDOW_BUTTON_CONFIG, (struct MsgPort *)data->ipc);

			// Create application port
			data->app_port = CreateMsgPort();

			// Make this an AppWindow
			data->appwindow = AddAppWindowA(0, 0, data->window, data->app_port, 0);

			// Get display area
			data->display = GetObject(data->objlist, GAD_BUTTONED_BUTTON);

			// Get display box
			data->display_box = AREA(data->display).area_pos;
			data->display_box.Top += (data->window->RPort->TxHeight >> 1) + 4;
			data->display_box.Height -= (data->window->RPort->TxHeight >> 1) + 6;
			data->display_box.Left += 2;
			data->display_box.Width -= 4;

			// Go through button functions
			for (func = (Cfg_ButtonFunction *)data->button->function_list.mlh_Head; func->node.ln_Succ;
				 func = (Cfg_ButtonFunction *)func->node.ln_Succ)
			{
				// Got image?
				if (func->image)
				{
					// Remap it
					RemapImage(func->image, data->window->WScreen, &data->remap);
				}
			}

			// Show the button
			_buttoned_show_button(data);

			// Tell children to appear
			IPC_ListCommand(&data->proc_list, IPC_SHOW, 0, (IPTR)data->window, 0);
			open_window = 0;
		}

		// Intuimessage
		while (data->window && (imsg = GetWindowMsg(data->window->UserPort)))
		{
			struct IntuiMessage msg_copy;
			struct Gadget *gadget;
			struct TagItem *tags;

			// Copy message and reply
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

			// BOOPSI message
			case IDCMP_IDCMPUPDATE: {
				short item;

				// Check this is the function list
				if (GetTagData(GA_ID, 0, tags) != GAD_BUTTONED_FUNCTIONS)
					break;

				// Get item
				if ((item = GetTagData(DLV_DragNotify, -1, tags)) != -1)
				{
					// Start the drag
					config_drag_start(&data->drag, data->func_list, item, tags, TRUE);
				}
			}
			break;

			// Ticks
			case IDCMP_INTUITICKS:
				++data->drag.tick_count;
				break;

			// Mouse move
			case IDCMP_MOUSEMOVE:

				// Handle drag
				config_drag_move(&data->drag);
				break;

			// Inactive window does menu down
			case IDCMP_INACTIVEWINDOW:
				msg_copy.Code = MENUDOWN;

			// Mouse buttons
			case IDCMP_MOUSEBUTTONS:

				// Valid drag info?
				if (data->drag.drag)
				{
					short ok = -1;

					// Dropped ok?
					if (msg_copy.Code == SELECTUP)
					{
						// End drag
						data->drag.drag_x = data->window->WScreen->MouseX;
						data->drag.drag_y = data->window->WScreen->MouseY;
						ok = 1;
					}

					// Aborted
					else if (msg_copy.Code == MENUDOWN)
					{
						// Set abort flag
						ok = 0;
					}

					// Want to end drag?
					if (ok != -1)
						buttoned_end_drag(data, ok);
				}

				// Or click on the displayed button?
				else if (msg_copy.Code == SELECTDOWN && msg_copy.MouseX >= data->button_rect.MinX &&
						 msg_copy.MouseY >= data->button_rect.MinY && msg_copy.MouseX <= data->button_rect.MaxX &&
						 msg_copy.MouseY <= data->button_rect.MaxY)
				{
					// Start drag
					config_drag_start_window(
						&data->drag, data->window, &data->button_rect, msg_copy.MouseX, msg_copy.MouseY);
				}
				break;

			// Gadget
			case IDCMP_GADGETUP:
				switch (gadget->GadgetID)
				{
				// Function type
				case GAD_BUTTONED_FUNCTIONS: {
					BOOL edit = 0;

					// New selection?
					if (data->which != msg_copy.Code)
					{
						data->which = msg_copy.Code;
						_buttoned_show_button(data);
					}

					// Check for double-click
					else if (DoubleClick(data->last_sec, data->last_mic, msg_copy.Seconds, msg_copy.Micros))
						edit = 1;

					// Not editing?
					if (!edit)
					{
						// Store double-click time
						data->last_sec = msg_copy.Seconds;
						data->last_mic = msg_copy.Micros;
						break;
					}
				}

					// Fall through to edit function

				// Edit function
				case GAD_BUTTONED_EDIT_FUNCTION:

					// Launch editor
					_button_editor_edit_function(data);
					break;

				// Label/Image or Name changed
				case GAD_BUTTONED_NAME:
				case GAD_BUTTONED_LABEL:

					// Change string
					_button_editor_change_label(data, gadget->GadgetID, 1);
					break;

				// New colours
				case GAD_PALETTE_FOREGROUND:
				case GAD_PALETTE_BACKGROUND: {
					short fg, bg;

					// Get both colours
					fg = GetGadgetValue(data->objlist, GAD_PALETTE_FOREGROUND);
					bg = GetGadgetValue(data->objlist, GAD_PALETTE_BACKGROUND);

					// Map colours under 39
					if (((struct Library *)GfxBase)->lib_Version >= 39)
					{
						if (fg >= 4 && fg < 8)
							fg += 248;
						else if (fg > 7)
							fg -= 4;
						if (bg >= 4 && bg < 8)
							bg += 248;
						else if (bg > 7)
							bg -= 4;
					}

					// Store new colours
					data->button->button.fpen = fg;
					data->button->button.bpen = bg;

					// Show button
					_buttoned_show_button(data);

					// Set change flag
					data->change = 1;
				}
				break;

				// Add function
				case GAD_BUTTONED_ADD:
					buttoned_add_function(data);
					break;

				// Delete function
				case GAD_BUTTONED_DEL:
					buttoned_del_function(data);
					break;

				// Use
				case GAD_BUTTONED_USE:
					success = 1;

				// Cancel
				case GAD_BUTTONED_CANCEL:
					if (!pending_quit)
						break_flag = 1;
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
					IPC_Command(data->startup->main_owner, IPC_HELP, (1 << 31), "Button Editor", 0, REPLY_NO_PORT);

					// Clear busy pointer
					ClearWindowBusy(data->window);
				}
				break;
			}

			// Reply to outstanding messages
			if (imsg)
				ReplyWindowMsg(imsg);
		}

		// App message?
		if (data->app_port)
		{
			struct AppMessage *appmsg;

			while ((appmsg = (struct AppMessage *)GetMsg(data->app_port)))
			{
				short num;
				Cfg_ButtonFunction *func;
				ButtonEditorNode *editor;

				// Get function
				if ((func = buttoned_get_function(data, data->which)))
				{
					BOOL noimage = 0, nofunc = 0, inarea;

					// No image currently?
					if (!func->label || !*func->label)
						noimage = 1;

					// No function currently?
					if (IsListEmpty((struct List *)&func->instructions))
						nofunc = 1;

					// Dropped into button area?
					inarea = CheckObjectArea(data->display, appmsg->am_MouseX, appmsg->am_MouseY);

					// Ok to set label?
					if (noimage || inarea)
					{
						// Get first file for name/icon
						for (num = 0; num < appmsg->am_NumArgs; num++)
						{
							char name[256], *ptr;

							// Is button graphical?
							if (data->button->button.flags & BUTNF_GRAPHIC)
							{
								APTR image;

								// Get full name
								GetWBArgPath(&appmsg->am_ArgList[num], name, 256);

								// Try and get image
								if (!(image = OpenImage(name, 0)))
								{
									// Add .info and try again
									strcat(name, ".info");
									if (!(image = OpenImage(name, 0)))
										break;
								}

								// Free existing image
								CloseImage(func->image);

								// Save new image pointer
								func->image = image;

								// Remap image
								RemapImage(image, data->window->WScreen, &data->remap);

								// Set change flag
								data->change = 1;
							}

							// Otherwise, get basename of object
							else
								strcpy(name, appmsg->am_ArgList[num].wa_Name);

							// Free existing label, copy new one
							FreeMemH(func->label);
							if ((func->label = AllocMemH(0, strlen(name) + 1)))
								strcpy(func->label, name);

							// Strip any .info, get pointer to name
							if ((ptr = strstr(name, ".info")))
								*ptr = 0;
							ptr = FilePart(name);

							// Free existing name, copy new one
							FreeMemH(func->node.ln_Name);
							if ((func->node.ln_Name = AllocMemH(0, strlen(ptr) + 1)))
								strcpy(func->node.ln_Name, ptr);

							// Refresh display
							_buttoned_show_button(data);

							// Fix list
							buttoned_fix_functionlist(data);
							break;
						}
					}

					// Ok to set function?
					if (!inarea || nofunc)
					{
						// If editor is open, pass message on
						if ((editor = buttoned_find_editor(data, func)))
						{
							IPC_Command(editor->editor, CFG_APPMESSAGE_PASS, 0, appmsg, 0, 0);
							appmsg = 0;
						}

						// Otherwise, handle it ourselves
						else
							for (num = 0; num < appmsg->am_NumArgs; num++)
							{
								Cfg_Instruction *ins;

								// Create instruction
								if ((ins = instruction_from_wbarg(&appmsg->am_ArgList[num], 0)))
								{
									// Add to function list
									AddTail((struct List *)&func->instructions, (struct Node *)ins);
									data->change = 1;
								}
							}
					}
				}

				// Reply the message
				if (appmsg)
					ReplyMsg((struct Message *)appmsg);
			}
		}

		// Check break flag
		if (break_flag || pending_quit)
		{
			// See if all the editors are gone
			if (IsListLockEmpty(&data->proc_list))
				break;

			// Send quit?
			if (break_flag)
			{
				if (data->window)
					SetWindowBusy(data->window);
				IPC_ListQuit(&data->proc_list, 0, success, FALSE);
				pending_quit = 1;
			}
		}

		// Wait for signal
		if (Wait(((data->window) ? 1 << data->window->UserPort->mp_SigBit : 0) |
				 ((data->app_port) ? 1 << data->app_port->mp_SigBit : 0) | 1 << data->change_bit |
				 1 << data->drag.timer->port->mp_SigBit | 1 << ipc->command_port->mp_SigBit) &
			(1 << data->change_bit))
		{
			// Update label
			_button_editor_change_label(data, GAD_BUTTONED_LABEL, 0);
		}
	}

	// End any drag in progress
	buttoned_end_drag(data, 0);

	// Need to send button back?
	if (success && data->change)
	{
		Cfg_ButtonFunction *function;

		// Go through button functions
		for (function = (Cfg_ButtonFunction *)data->button->function_list.mlh_Head; function->node.ln_Succ;)
		{
			Cfg_ButtonFunction *next = (Cfg_ButtonFunction *)function->node.ln_Succ;

			// Free function if empty
			if (buttoned_function_empty(function))
				buttoned_free_function(data, function, TRUE);

			// Get next function in list
			function = next;
		}

		// Store button address
		startup->retdata.button = data->button;

		// Send button back
		IPC_Command(startup->owner_ipc, BUTTONEDIT_RETURN, 0, &startup->retdata, 0, REPLY_NO_PORT);
	}
	else
		success = 0;

	// Close display
	_buttoned_close_window(data);

	// Free edit button
	FreeButton(data->button);

	// Close application port
	if (data->app_port)
	{
		struct Message *msg;
		while ((msg = GetMsg(data->app_port)))
			ReplyMsg(msg);
		DeleteMsgPort(data->app_port);
	}

	// Say goodbye
	IPC_Goodbye(ipc, startup->owner_ipc, success);

	// Delete IPC data
	IPC_Free(ipc);

	// Free signal bit
	FreeSignal(data->change_bit);

	// Close timer
	FreeTimer(data->drag.timer);

	// Free data
	Att_RemList(data->func_list, 0);
	FreeVec(data);
	FreeVec(startup);
}

// Show the button being edited
void _buttoned_show_button(ButtonEdData *data)
{
	Cfg_ButtonFunction *func;

	// Get function
	func = buttoned_find_function(data, data->which);

	// Draw button
	button_draw(data->button,
				&data->display_box,
				&data->button_rect,
				data->window,
				data->which,
				func,
				(short *)data->startup->palette_data.screen_data.pen_array,
				DRAW_MASK,
				0,
				0,
				-1,
				DOpusBase,
				(struct Library *)GfxBase);

	// Fill out fields
	SetGadgetValue(data->objlist, GAD_BUTTONED_NAME, (func) ? (IPTR)func->node.ln_Name : 0);
	SetGadgetValue(data->objlist, GAD_BUTTONED_LABEL, (func) ? (IPTR)func->label : 0);
}

// Edit a button
void _button_editor_edit_function(ButtonEdData *data)
{
	Cfg_ButtonFunction *func;
	ButtonEditorNode *editor;
	Cfg_Function *function = 0;
	FunctionStartup *startup = 0;
	IPCData *ipc;
	BOOL success = 0;

	// Find the function we want to edit
	if (!(func = buttoned_find_function(data, data->which)))
	{
		// Create new function
		if ((func = NewButtonFunction(0, data->which)))
		{
			// Add to list
			AddTail((struct List *)&data->button->function_list, (struct Node *)func);

			// Create function to edit
			function = NewFunction(0, data->which);
		}
	}

	// Otherwise, copy the function
	else
		function = CopyFunction((Cfg_Function *)func, 0, 0);

	// Is editor already up?
	if ((editor = buttoned_find_editor(data, func)))
	{
		// Activate editor
		IPC_Command(editor->editor, IPC_ACTIVATE, 0, 0, 0, 0);
		FreeFunction(function);
		return;
	}

	// Allocate startup data
	if (function && (startup = AllocVec(sizeof(FunctionStartup), MEMF_CLEAR)))
	{
		// Copy startup data
		*startup = data->startup->func_startup;
		startup->window = data->window;
		startup->function = function;
		startup->owner_ipc = data->ipc;
		startup->main_owner = data->startup->main_owner;
		startup->object = data->button;
		startup->object_flags = (IPTR)func;

		// Launch editor
		if ((IPC_Launch(&data->proc_list,
						&ipc,
						"dopus_function_editor",
						IPC_NATIVE(FunctionEditor),
						STACK_DEFAULT,
						(IPTR)startup,
						(struct Library *)DOSBase)) &&
			ipc)
		{
			// Allocate edit node
			if ((editor = AllocVec(sizeof(ButtonEditorNode), MEMF_CLEAR)))
			{
				// Fill out edit node
				editor->editor = ipc;
				editor->func = func;
				AddTail((struct List *)&data->editors, (struct Node *)editor);
			}
			success = 1;
		}
	}

	// Free data if not successful
	if (!success)
	{
		FreeFunction(function);
		FreeVec(startup);
	}
}

// Receive an edited function
BOOL _button_editor_receive_edit(ButtonEdData *data, FunctionReturn *ret)
{
	Cfg_ButtonFunction *func;

	// Check function is in list
	for (func = (Cfg_ButtonFunction *)data->button->function_list.mlh_Head; func->node.ln_Succ;
		 func = (Cfg_ButtonFunction *)func->node.ln_Succ)
	{
		// Does it match?
		if (func == (Cfg_ButtonFunction *)(IPTR)ret->object_flags)
		{
			// Free existing function
			FreeInstructionList((Cfg_Function *)func);

			// Copy function into old function
			CopyFunction(ret->function, 0, (Cfg_Function *)func);

			// Fix function list
			buttoned_fix_functionlist(data);
			return 1;
		}
	}

	// Not found
	return 0;
}

// Copy a string
void buttoned_copy_string(ButtonEdData *data, char *from, char **to)
{
	// Free existing string
	if (*to)
		FreeMemH(*to);

	// Allocate and copy
	if (from && *from && (*to = AllocMemH(0, strlen(from) + 1)))
		strcpy(*to, from);
	else
		*to = 0;
}

// Find function entry
Cfg_ButtonFunction *buttoned_find_function(ButtonEdData *data, short which)
{
	Att_Node *node;

	// Find node in function list
	if ((node = Att_FindNode(data->func_list, which)))
	{
		// Normal function?
		if (node->data == FTYPE_LEFT_BUTTON || node->data == FTYPE_MID_BUTTON || node->data == FTYPE_RIGHT_BUTTON)
		{
			return (Cfg_ButtonFunction *)FindFunctionType((struct List *)&data->button->function_list, node->data);
		}

		// Function pointer itself
		return (Cfg_ButtonFunction *)node->data;
	}

	return 0;
}

// Get function entry, or create if it doesn't exist
Cfg_ButtonFunction *buttoned_get_function(ButtonEdData *data, short which)
{
	Cfg_ButtonFunction *func;

	// Look for function
	if ((func = buttoned_find_function(data, which)))
		return func;

	// Create a new function
	if (!(func = NewButtonFunction(0, which)))
		return 0;

	// Add to list
	AddTail((struct List *)&data->button->function_list, (struct Node *)func);
	return func;
}

// End drag
void buttoned_end_drag(ButtonEdData *data, BOOL ok)
{
	short item, drag_item;
	struct Window *window;
	ULONG id = 0;
	IPCData *ipc = 0;

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

	// Get item
	drag_item = Att_FindNodeNumber(data->func_list, data->drag.drag_node);

	// Drop on our window?
	if (window == data->window)
	{
		// Convert to window coordinates
		data->drag.drag_x -= data->window->LeftEdge;
		data->drag.drag_y -= data->window->TopEdge;

		// Dropped over list?
		if (drag_item == -1 || (item = buttoned_get_drop(data, data->drag.drag_x, data->drag.drag_y)) == -1)
			return;

		// Dropped over different item?
		if (item != drag_item)
		{
			// Swap the two functions
			if (buttoned_swap_functions(data, drag_item, item))
			{
				// Select the new entry
				data->which = item;
				SetGadgetValue(data->objlist, GAD_BUTTONED_FUNCTIONS, item);

				// Set change flag
				data->change = 1;
			}
		}

		return;
	}

	// Another window
	else if (ipc)
	{
		Cfg_ButtonFunction *func = 0;
		BOOL ok = 0;

		// Get function we're dragging
		if (drag_item != -1)
			func = buttoned_get_function(data, drag_item);

		// Drop on function editor?
		if (id == WINDOW_FUNCTION_EDITOR && func)
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

			// Send function
			IPC_Command(ipc, FUNCTIONEDIT_COPY_LINE, 0, first, pos, 0);
			ok = 1;
		}

		// Or on a button editor
		else if (id == WINDOW_BUTTON_CONFIG || id == WINDOW_BUTTONS)
		{
			Point *pos;
			Cfg_Button *button;
			Cfg_ButtonFunction *newfunc = 0;

			// Store position (screen relative)
			if ((pos = AllocVec(sizeof(Point), 0)))
			{
				pos->x = data->drag.drag_x;
				pos->y = data->drag.drag_y;
			}

			// Copy the whole button?
			if (drag_item == -1)
			{
				if ((button = CopyButton(data->button, 0, -1)))
					newfunc = (Cfg_ButtonFunction *)1;
			}

			// Create a new button
			else if ((button = NewButton(0)))
			{
				// Copy function
				newfunc = buttoned_copy_function(data, button, func, 0, FTYPE_LEFT_BUTTON);
			}

			// Something to send?
			if (newfunc)
			{
				// Send button
				IPC_Command(ipc, BUTTONEDIT_CLIP_BUTTON, 0, button, pos, 0);
				ok = 1;
			}

			// Failed?
			else
			{
				FreeButton(button);
				FreeVec(pos);
			}
		}

		// Permit now the message has been sent
		Permit();
		if (ok)
			return;
	}

	// Illegal drop
	DisplayBeep(data->window->WScreen);
}

// Get drop position
short buttoned_get_drop(ButtonEdData *data, short x, short y)
{
	long top, item;

	// Valid drop within the list?
	if (!(CheckObjectArea(data->lister, x, y)))
		return -1;

	// Get top item
	GetAttr(DLV_Top, (Object *)GADGET(data->lister), (ULONG *)&top);

	// Get item we dropped over
	item = UDivMod32((y - data->lister->dims.Top), data->window->RPort->TxHeight) + top;

	// Valid item?
	if (item >= 0 && item < Att_NodeCount(data->func_list))
		return (short)item;
	return -1;
}

// Given a new button
void buttoned_get_button(ButtonEdData *data, Cfg_Button *button, Point *pos)
{
	short item;
	short x = 0, y = 0;

	// Get window-relative position
	if (pos)
	{
		x = pos->x - data->window->LeftEdge;
		y = pos->y - data->window->TopEdge;
	}

	// Dropped over an item in the function list?
	if ((item = buttoned_get_drop(data, x, y)) != -1)
	{
		Cfg_ButtonFunction *oldfunc, *func;
		ButtonEditorNode *editor;
		short which;

		// Find existing function
		if ((oldfunc = buttoned_find_function(data, item)))
		{
			// Free function
			buttoned_free_function(data, oldfunc, FALSE);
			which = oldfunc->function.func_type;
		}
		else
			which = item;

		// Ok to take drop?
		if (oldfunc || which <= FTYPE_MID_BUTTON)
		{
			// Get function from button
			if ((func =
					 (Cfg_ButtonFunction *)FindFunctionType((struct List *)&button->function_list, FTYPE_LEFT_BUTTON)))
			{
				// Copy new function into button
				buttoned_copy_function(data, data->button, func, oldfunc, which);
			}

			// Kill editor if it's open
			if (oldfunc && (editor = buttoned_find_editor(data, oldfunc)))
				IPC_Quit(editor->editor, 0, FALSE);

			// Select new item
			data->which = item;
			SetGadgetValue(data->objlist, GAD_BUTTONED_FUNCTIONS, data->which);
		}

		// Free new button
		FreeButton(button);
	}

	// Replace whole button
	else if (CheckObjectArea(data->display, x, y))
	{
		short gfx = 0;

		// Graphical button?
		if (data->button->button.flags & BUTNF_GRAPHIC)
			gfx = 1;

		// Free existing button
		FreeButton(data->button);

		// Store pointer to new button
		data->button = button;

		// Fix type
		if (gfx)
			button->button.flags |= BUTNF_GRAPHIC;
		else
			button->button.flags &= ~BUTNF_GRAPHIC;

		// Tell editors to quit
		IPC_ListQuit(&data->proc_list, 0, 0, FALSE);

		// Select first item
		data->which = 0;
		SetGadgetValue(data->objlist, GAD_BUTTONED_FUNCTIONS, 0);
	}

	// Invalid drop
	else
	{
		DisplayBeep(data->window->WScreen);
		FreeButton(button);
		return;
	}

	// Show the button
	_buttoned_show_button(data);

	// Fix list
	buttoned_refresh_functionlist(data);

	// Set change flag
	data->change = 1;
}

// Free a function
void buttoned_free_function(ButtonEdData *data, Cfg_ButtonFunction *function, BOOL free_all)
{
	// Free function info
	FreeMemH(function->node.ln_Name);
	function->node.ln_Name = 0;
	FreeMemH(function->label);
	function->label = 0;
	CloseImage(function->image);
	function->image = 0;
	FreeInstructionList((Cfg_Function *)function);

	// Free function itself?
	if (free_all)
	{
		// Remove and free the function
		Remove((struct Node *)function);
		FreeFunction((Cfg_Function *)function);
	}
}

// Copy a function
Cfg_ButtonFunction *buttoned_copy_function(ButtonEdData *data,
										   Cfg_Button *button,
										   Cfg_ButtonFunction *func,
										   Cfg_ButtonFunction *newfunc,
										   short which)
{
	BOOL add = 0;

	// Need to create?
	if (!newfunc)
		add = 1;

	// Create a new function
	if (newfunc || (newfunc = NewButtonFunction(0, which)))
	{
		// Copy function data
		CopyFunction((Cfg_Function *)func, 0, (Cfg_Function *)newfunc);

		// Fix type
		newfunc->function.func_type = which;

		// Copy name, label and image
		buttoned_copy_string(data, func->node.ln_Name, &newfunc->node.ln_Name);
		buttoned_copy_string(data, func->label, &newfunc->label);
		newfunc->image = CopyImage(func->image);

		// Add function to new button
		if (add)
			AddTail((struct List *)&button->function_list, (struct Node *)newfunc);
	}

	return newfunc;
}

// See if a function is empty
BOOL buttoned_function_empty(Cfg_ButtonFunction *function)
{
	// No instructions?
	if (IsListEmpty((struct List *)&function->instructions))
	{
		// If a custom function, return true now
		if (function->function.func_type == FTYPE_LIST)
			return 1;

		// Empty if name field is blank
		if (!function->label || !*function->label)
			return 1;
	}
	return 0;
}

// Fix colours in function list
void buttoned_fix_functionlist(ButtonEdData *data)
{
	Att_Node *node;

	// Detach function list
	SetGadgetChoices(data->objlist, GAD_BUTTONED_FUNCTIONS, (APTR)~0);

	// Go through function list
	for (node = (Att_Node *)data->func_list->list.lh_Head; node->node.ln_Succ; node = (Att_Node *)node->node.ln_Succ)
	{
		Cfg_ButtonFunction *func;

		// Custom function?
		if (node->data > 10)
		{
			// Get function pointer from node
			func = (Cfg_ButtonFunction *)node->data;
		}

		// Normal function
		else
		{
			func = (Cfg_ButtonFunction *)FindFunctionType((struct List *)&data->button->function_list, node->data);
		}

		// Non-empty function?
		if (func && !(buttoned_function_empty(func)))
		{
			/*
						// Set colour
						node->node.lve_Flags|=LVEF_USE_PEN;
						node->node.lve_Pen=DRAWINFO(data->window)->dri_Pens[HIGHLIGHTTEXTPEN];
			*/
			node->node.lve_Flags |= LVEF_SELECTED;
		}
		//		else node->node.lve_Flags&=~LVEF_USE_PEN;
		else
			node->node.lve_Flags &= ~LVEF_SELECTED;
	}

	// Attach function list
	SetGadgetChoices(data->objlist, GAD_BUTTONED_FUNCTIONS, data->func_list);
}

// Find an editor
ButtonEditorNode *buttoned_find_editor(ButtonEdData *data, Cfg_ButtonFunction *func)
{
	ButtonEditorNode *editor;

	// Go through editors
	for (editor = (ButtonEditorNode *)data->editors.mlh_Head; editor->node.mln_Succ;
		 editor = (ButtonEditorNode *)editor->node.mln_Succ)
	{
		// Is editor editing this function?
		if (editor->func == func)
			return editor;
	}

	return 0;
}

// Delete function
void buttoned_del_function(ButtonEdData *data)
{
	Cfg_ButtonFunction *func;

	// Find function
	if ((func = buttoned_find_function(data, data->which)))
	{
		// Delete the function
		buttoned_delete_function(data, func, 1);

		// Fix function list
		buttoned_fix_functionlist(data);

		// Set change flag
		data->change = 1;

		// Update display
		_buttoned_show_button(data);
	}
}

void buttoned_delete_function(ButtonEdData *data, Cfg_ButtonFunction *func, BOOL show)
{
	ButtonEditorNode *editor;

	// See if editor is open
	if ((editor = buttoned_find_editor(data, func)))
	{
		// Close editor
		IPC_Command(editor->editor, IPC_QUIT, 0, 0, 0, 0);
	}

	// Custom function?
	if (func->function.func_type == FTYPE_LIST)
	{
		Att_Node *node;

		// Find node in list
		if ((node = Att_FindNodeData(data->func_list, (IPTR)func)))
		{
			// Detach function list
			if (show)
				SetGadgetChoices(data->objlist, GAD_BUTTONED_FUNCTIONS, (APTR)~0);

			// If this is the last node, select the previous one
			if (!node->node.ln_Succ->ln_Succ)
			{
				--data->which;
				if (show)
					SetGadgetValue(data->objlist, GAD_BUTTONED_FUNCTIONS, data->which);
			}

			// Remove the node
			Att_RemNode(node);

			// Attach function list
			if (show)
				SetGadgetChoices(data->objlist, GAD_BUTTONED_FUNCTIONS, data->func_list);
		}
	}

	// Free function
	buttoned_free_function(data, func, TRUE);
}

// Add function
void buttoned_add_function(ButtonEdData *data)
{
	Cfg_ButtonFunction *func;
	char *name;
	Att_Node *node;

	// Create a new function
	if (!(func = NewButtonFunction(0, FTYPE_LIST)))
		return;

	// Initialise name
	name = GetString(locale, MSG_BUTTONED_NEW_FUNC);
	if ((func->node.ln_Name = AllocMemH(0, strlen(name) + 1)))
		strcpy(func->node.ln_Name, name);

	// Add to list
	AddTail((struct List *)&data->button->function_list, (struct Node *)func);

	// Detach function list
	SetGadgetChoices(data->objlist, GAD_BUTTONED_FUNCTIONS, (APTR)~0);

	// Add to function list
	node = Att_NewNode(data->func_list, name, (IPTR)func, 0);

	// Attach function list
	SetGadgetChoices(data->objlist, GAD_BUTTONED_FUNCTIONS, data->func_list);

	// Got node?
	if (node)
	{
		// Get node number
		data->which = Att_FindNodeNumber(data->func_list, node);

		// Select that node
		SetGadgetValue(data->objlist, GAD_BUTTONED_FUNCTIONS, data->which);

		// Show selection
		_buttoned_show_button(data);
	}

	// Set change flag
	data->change = 1;
}

// Build function list
void buttoned_build_function_list(ButtonEdData *data)
{
	short a;
	Cfg_ButtonFunction *func;

	// Clear list contents
	Att_RemList(data->func_list, REMLIST_SAVELIST);

	// Add nodes
	for (a = 0; a < 3; a++)
		Att_NewNode(data->func_list, GetString(data->locale, MSG_BUTTONED_LEFT_FUNCTION + a), a, 0);

	// Go through button functions
	for (func = (Cfg_ButtonFunction *)data->button->function_list.mlh_Head; func->node.ln_Succ;
		 func = (Cfg_ButtonFunction *)func->node.ln_Succ)
	{
		// Additional function?
		if (func->function.func_type == FTYPE_LIST)
		{
			// Add to list
			Att_NewNode(data->func_list, (func->node.ln_Name) ? func->node.ln_Name : func->label, (IPTR)func, 0);
		}
	}
}

// Swap functions
BOOL buttoned_swap_functions(ButtonEdData *data, short item1, short item2)
{
	Cfg_ButtonFunction *swap1, *swap2;
	ButtonEditorNode *editor;
	long temp;

	// Find the two functions
	if (!(swap1 = buttoned_get_function(data, item1)) || !(swap2 = buttoned_get_function(data, item2)))
		return 0;

	// Swap the two entries
	SwapListNodes((struct List *)&data->button->function_list, (struct Node *)swap1, (struct Node *)swap2);

	// Swap function types
	temp = swap1->function.func_type;
	swap1->function.func_type = swap2->function.func_type;
	swap2->function.func_type = temp;

	// Check first function for empty, fix editor otherwise
	if (buttoned_check_empty(swap1))
		buttoned_delete_function(data, swap1, 0);
	else if ((editor = buttoned_find_editor(data, swap1)))
	{
		IPC_Command(editor->editor, CFG_NEW_ID, swap1->function.func_type, swap1, 0, 0);
	}

	// Check second function for empty, fix editor otherwise
	if (buttoned_check_empty(swap2))
		buttoned_delete_function(data, swap2, 0);
	else if ((editor = buttoned_find_editor(data, swap2)))
	{
		IPC_Command(editor->editor, CFG_NEW_ID, swap2->function.func_type, swap2, 0, 0);
	}

	// Refresh function list
	buttoned_refresh_functionlist(data);
	return 1;
}

void buttoned_refresh_functionlist(ButtonEdData *data)
{
	// Detach function list
	SetGadgetChoices(data->objlist, GAD_BUTTONED_FUNCTIONS, (APTR)~0);

	// Rebuild function list
	buttoned_build_function_list(data);

	// Fix colours
	buttoned_fix_functionlist(data);
}

// Change name or label
void _button_editor_change_label(ButtonEdData *data, UWORD id, BOOL refresh)
{
	char *str;
	Cfg_ButtonFunction *func;
	BOOL show = 0, create = 0;

	// Get function
	if (!(func = buttoned_find_function(data, data->which)))
	{
		// Create a new function
		if (!(func = NewButtonFunction(0, data->which)))
			return;

		// Add to list
		AddTail((struct List *)&data->button->function_list, (struct Node *)func);
		create = 1;
	}

	// Copy new name
	str = (char *)GetGadgetValue(data->objlist, id);
	buttoned_copy_string(data, str, (id == GAD_BUTTONED_NAME) ? &func->node.ln_Name : &func->label);

	// Changing image/label?
	if (id == GAD_BUTTONED_LABEL)
	{
		char *ptr;

		// If name is blank, copy label to name
		if (refresh && (!(ptr = (char *)GetGadgetValue(data->objlist, GAD_BUTTONED_NAME)) || !*ptr))
		{
			SetGadgetValue(data->objlist, GAD_BUTTONED_NAME, (IPTR)str);
			buttoned_copy_string(data, str, &func->node.ln_Name);
		}

		// If button is graphical, get new image
		if (data->button->button.flags & BUTNF_GRAPHIC)
		{
			// Close old image
			CloseImage(func->image);

			// Read new image
			if ((func->image = OpenImage(func->label, 0)))
			{
				// Remap image
				RemapImage(func->image, data->window->WScreen, &data->remap);
			}
		}

		// Need to redisplay
		show = 1;
	}

	// Changing name
	else
	{
		Att_Node *node;

		// Get selected node
		if ((node = Att_FindNode(data->func_list, data->which)))
		{
			// If this is a custom function, change node name
			if (func->function.func_type == FTYPE_LIST)
			{
				// Detach function list
				SetGadgetChoices(data->objlist, GAD_BUTTONED_FUNCTIONS, (APTR)~0);

				// Change node name
				Att_ChangeNodeName(node, str);

				// Attach function list
				SetGadgetChoices(data->objlist, GAD_BUTTONED_FUNCTIONS, data->func_list);
			}
		}

		// Not a graphical button?
		if (!(data->button->button.flags & BUTNF_GRAPHIC))
		{
			char *ptr;

			// If label is blank, copy name to label
			if (!(ptr = (char *)GetGadgetValue(data->objlist, GAD_BUTTONED_LABEL)) || !*ptr)
			{
				SetGadgetValue(data->objlist, GAD_BUTTONED_LABEL, (IPTR)str);
				buttoned_copy_string(data, str, &func->label);
				show = 1;
			}
		}
	}

	// Show button
	if (show)
	{
		// Draw button
		button_draw(data->button,
					&data->display_box,
					&data->button_rect,
					data->window,
					data->which,
					func,
					(short *)data->startup->palette_data.screen_data.pen_array,
					(refresh) ? DRAW_MASK : 0,
					0,
					0,
					-1,
					DOpusBase,
					(struct Library *)GfxBase);

		// Fill out fields on a real refresh
		if (refresh)
		{
			SetGadgetValue(data->objlist, GAD_BUTTONED_NAME, (func) ? (IPTR)func->node.ln_Name : 0);
			SetGadgetValue(data->objlist, GAD_BUTTONED_LABEL, (func) ? (IPTR)func->label : 0);
		}
	}

	// Fix list
	if (create)
		buttoned_fix_functionlist(data);

	// Set change flag
	data->change = 1;
}

// Close display
void _buttoned_close_window(ButtonEdData *data)
{
	// Free image remapping
	FreeImageRemap(&data->remap);

	// Remove AppWindow
	RemoveAppWindow(data->appwindow);
	data->appwindow = 0;

	// Close window
	CloseConfigWindow(data->window);
	data->window = 0;
	data->objlist = 0;
}

// Check a function for empty
BOOL buttoned_check_empty(Cfg_ButtonFunction *func)
{
	// If not a custom function, return false now
	if (func->function.func_type != FTYPE_LIST)
		return 0;

	// No instructions?
	if (IsListEmpty((struct List *)&func->instructions))
		return 1;

	return 0;
}
