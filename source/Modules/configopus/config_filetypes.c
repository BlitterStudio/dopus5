#include "config_lib.h"
#include "config_filetypes.h"

static BOOL filetypes_saved_size_matches_font(struct Screen *screen, short fontsize)
{
	struct Screen *pub = 0;
	short current_size = 0;

	if (!screen)
	{
		if (!(pub = LockPubScreen(0)))
			return FALSE;
		screen = pub;
	}

	current_size = screen->RastPort.TxHeight;

	if (pub)
		UnlockPubScreen(0, pub);

	return (current_size == fontsize);
}

short LIBFUNC L_Config_Filetypes(REG(a0, struct Screen *screen),
								 REG(a1, IPCData *ipc),
								 REG(a2, IPCData *owner_ipc),
								 REG(d0, IPTR command_list),
								 REG(a3, char *name))
{
	config_filetypes_data *data;
	IPCMessage *quit_msg = 0;
	short undo_flag = 0, pending_quit = 0;
	short ret = 0, fontsize;
	struct IBox pos;
	ConfigWindow dims;

	// Allocate data and memory handle
	if (!(data = AllocVec(sizeof(config_filetypes_data), MEMF_CLEAR)) ||
		!(data->memory = NewMemHandle(4096, 256, MEMF_CLEAR)))
		return 0;

	// Save pointers
	data->ipc = ipc;
	data->owner_ipc = owner_ipc;
	data->command_list = command_list;

	// Initialise data
	NewList(&data->list_list);
	InitListLock(&data->proc_list, 0);

	// Fill in new window
	data->newwin.parent = screen;
	data->newwin.dims = &dims;
	data->newwin.title = GetString(locale, MSG_FILETYPES_TITLE);
	data->newwin.locale = locale;
	data->newwin.flags =
		WINDOW_SCREEN_PARENT | WINDOW_VISITOR | WINDOW_REQ_FILL | WINDOW_AUTO_KEYS | WINDOW_SIZE_BOTTOM;

	// Get default size
	dims = _config_filetypes_window;

	// Get saved position
	if (LoadPos("dopus/windows/filetypes", &pos, &fontsize) && filetypes_saved_size_matches_font(screen, fontsize))
	{
		dims.char_dim.Width = 0;
		dims.char_dim.Height = 0;
		dims.fine_dim.Width = pos.Width;
		dims.fine_dim.Height = pos.Height;
	}

	// Open window and add objects
	if (!(data->window = OpenConfigWindow(&data->newwin)) ||
		!(data->objlist = AddObjectList(data->window, _config_filetypes_objects)))
	{
		CloseConfigWindow(data->window);
		FreeMemHandle(data->memory);
		FreeVec(data);
		return 0;
	}

	// Set minimum size
	SetConfigWindowLimits(data->window, &_config_filetypes_window, 0);

	// Read filetype list
	SetWindowBusy(data->window);
	filetype_read_list(data->memory, &data->list_list);

	// Build display list
	filetype_build_list(data);
	ClearWindowBusy(data->window);

	// Name to edit?
	if (name)
		filetype_edit_name(data, name);

	// Event loop
	FOREVER
	{
		struct IntuiMessage *msg;
		IPCMessage *imsg;
		int quit_flag = 0;

		// Any IPC messages?
		while ((imsg = (IPCMessage *)GetMsg(ipc->command_port)))
		{
			// Look at command
			switch (imsg->command)
			{
			// Quit
			case IPC_QUIT:
				quit_flag = 1;
				quit_msg = imsg;
				imsg = 0;
				data->change = 0;
				break;

			// Activate
			case IPC_ACTIVATE:
				if (data->window)
				{
					// Bring window to front
					WindowToFront(data->window);
					ActivateWindow(data->window);

					// Edit name supplied?
					if (imsg->data)
						filetype_edit_name(data, (char *)imsg->data);
				}
				break;

			// Process saying goodbye
			case IPC_GOODBYE: {
				FiletypeNode *node;

				// Handle goodbye
				if ((node = (FiletypeNode *)IPC_GetGoodbye(imsg)))
				{
					// Node no longer has an editor
					node->editor = 0;
				}

				// All process gone and pending quit?
				if (pending_quit && (IsListEmpty(&data->proc_list.list)))
				{
					quit_flag = 1;
					pending_quit = 2;
				}
			}
			break;

			// Got a filetype back from the editor
			case FILETYPEEDIT_RETURN:
				if (filetype_receive_edit(data, (Cfg_Filetype *)imsg->flags, (FiletypeNode *)imsg->data))
				{
					data->change = 1;
					imsg->command = 1;
				}
				else
					imsg->command = 0;
				break;
			}

			// Reply message
			IPC_Reply(imsg);

			// Check quit flag
			if (quit_flag)
				break;
		}

		// Intuimessages
		if (data->window)
		{
			while ((msg = GetWindowMsg(data->window->UserPort)))
			{
				struct IntuiMessage msg_copy;
				UWORD id;

				// Copy message and reply
				msg_copy = *msg;
				ReplyWindowMsg(msg);

				if (pending_quit)
					continue;

				// Look at message
				switch (msg_copy.Class)
				{
				// Close window
				case IDCMP_CLOSEWINDOW:
					quit_flag = 1;
					undo_flag = 1;
					break;

				// Gadget
				case IDCMP_GADGETUP:

					id = ((struct Gadget *)msg_copy.IAddress)->GadgetID;
					switch (id)
					{
					// Cancel
					case GAD_FILETYPES_CANCEL:
						undo_flag = 1;

					// Use
					case GAD_FILETYPES_USE:
						quit_flag = 1;
						break;

					// Filetype selected
					case GAD_FILETYPES_LIST: {
						Att_Node *node;

						// Enable buttons
						filetype_able_buttons(data, FALSE);

						// Get selection
						if (!(node = Att_FindNode(data->filetype_list, msg_copy.Code)))
							break;

						// Double-click?
						if (node == data->sel_filetype &&
							DoubleClick(data->seconds, data->micros, msg_copy.Seconds, msg_copy.Micros))
						{
							// Launch editor for this filetype
							filetype_edit(data, (FiletypeNode *)data->sel_filetype->data, 0);
						}

						// New selection
						else
						{
							data->sel_filetype = node;
							data->seconds = msg_copy.Seconds;
							data->micros = msg_copy.Micros;
						}
					}
					break;

					// Add a new filetype
					case GAD_FILETYPES_DUPLICATE:
						if (!data->sel_filetype)
							break;
					case GAD_FILETYPES_ADD: {
						Cfg_FiletypeList *list;
						Cfg_Filetype *type = 0;

						// Allocate a new filetype list
						if ((list = AllocMemH(data->memory, sizeof(Cfg_FiletypeList))))
						{
							// Initialise list
							NewList(&list->filetype_list);

							// Copy existing filetype?
							if ((((struct Gadget *)msg_copy.IAddress)->GadgetID == GAD_FILETYPES_DUPLICATE))
							{
								// Copy filetype
								type = CopyFiletype(((FiletypeNode *)data->sel_filetype->data)->type, data->memory);
							}

							// Allocate a new filetype
							else if ((type = NewFiletype(data->memory)))
							{
								// Initialise name
								strcpy(type->type.name, GetString(locale, MSG_UNTITLED));
							}

							// Get a filetype?
							if (type)
							{
								// Add filetype list to main list
								AddTail(&data->list_list, &list->node);

								// Add filetype to list
								AddTail(&list->filetype_list, &type->node);

								// Set list pointer
								type->list = list;
								list->flags = FTLISTF_CHANGED;
							}

							// Failed
							else
								FreeMemH(list);
						}

						// Got new filetype?
						if (type)
						{
							Att_Node *node;

							// Remove existing list
							SetGadgetChoices(data->objlist, GAD_FILETYPES_LIST, (APTR)~0);

							// Add entry for this filetype
							node = filetype_add_entry(data, type);

							// Handle new nodes
							filetype_new_node(data, node);
						}
					}
					break;

					// Edit filetype
					case GAD_FILETYPES_EDIT:

						// Valid selection?
						if (data->sel_filetype)
						{
							// Launch editor for this filetype
							filetype_edit(data, (FiletypeNode *)data->sel_filetype->data, 0);
						}
						break;

					// Remove/Store
					case GAD_FILETYPES_REMOVE:
					case GAD_FILETYPES_STORE:

						// Valid selection?
						if (data->sel_filetype)
						{
							short ret;

							// Remove filetype
							if ((ret = filetype_remove(data, data->sel_filetype, (id == GAD_FILETYPES_STORE))) == 1)
							{
								data->change = 1;
								data->sel_filetype = 0;
							}

							// Quit?
							else if (ret == -1)
							{
								quit_flag = 1;
							}
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
						IPC_Command(data->owner_ipc, IPC_HELP, (1 << 31), "File Types", 0, REPLY_NO_PORT);

						// Clear busy pointer
						ClearWindowBusy(data->window);
					}
					break;
				}

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
				SetWindowBusy(data->window);
				if (!(IPC_ListQuit(&data->proc_list, 0, !undo_flag, FALSE)))
					pending_quit = 2;
				else
					pending_quit = 1;
			}
		}

		// Set to break?
		if (pending_quit == 2)
		{
			// Save filetypes?
			if (data->change && !undo_flag)
			{
				if (!(ret = filetype_save(data)))
				{
					// Abort save/quit
					ClearWindowBusy(data->window);
					pending_quit = 0;
					continue;
				}
			}
			break;
		}

		// Wait for an event
		Wait(1 << ipc->command_port->mp_SigBit | ((data->window) ? 1 << data->window->UserPort->mp_SigBit : 0));
	}

	// Save window position
	if (data->window)
	{
		struct IBox pos;
		pos.Left = data->window->LeftEdge;
		pos.Top = data->window->TopEdge;
		pos.Width = data->window->Width - data->window->BorderLeft - data->window->BorderRight;
		pos.Height = data->window->Height - data->window->BorderTop - data->window->BorderBottom;
		SavePos("dopus/windows/filetypes", (struct IBox *)&pos, data->window->RPort->TxHeight);
	}

	// Close up
	CloseConfigWindow(data->window);

	// Free data
	Att_RemList(data->filetype_list, REMLIST_FREEDATA);
	FreeMemHandle(data->memory);
	FreeVec(data);

	// Reply quit message
	IPC_Reply(quit_msg);
	return ret;
}

// Read filetype list
void filetype_read_list(APTR memory, struct List *main_list)
{
	Cfg_FiletypeList *list;
#ifdef __amigaos4__
	struct AnchorPathOld *anchor;
#else
	struct AnchorPath *anchor;
#endif
	long error;

	// Allocate anchor path
#ifdef __amigaos4__
	if (!(anchor = AllocMemH(memory, sizeof(struct AnchorPathOld) + 256)))
		return;
#else
	if (!(anchor = AllocMemH(memory, sizeof(struct AnchorPath) + 256)))
		return;
#endif

	// Initialise anchor path
	anchor->ap_Strlen = 256;
	anchor->ap_BreakBits = IPCSIG_QUIT;
	anchor->ap_Flags = APF_DOWILD;

	// Search for filetypes
	error = MatchFirst("DOpus5:Filetypes/~(#?.info)", anchor);

	// Continue while there's files
	while (!error)
	{
		// Read new filetype list
		if ((list = ReadFiletypes(anchor->ap_Buf, memory)))
		{
			// Add to list
			AddTail(main_list, &list->node);
		}

		// Find next file in directory
		error = MatchNext(anchor);
	}

	// Clean up match stuff
	MatchEnd(anchor);
	FreeMemH(anchor);
}

// Build list of available filetypes
void filetype_build_list(config_filetypes_data *data)
{
	Cfg_FiletypeList *list;
	Cfg_Filetype *type;

	// Remove existing list
	SetGadgetChoices(data->objlist, GAD_FILETYPES_LIST, (APTR)~0);

	// Free list
	Att_RemList(data->filetype_list, REMLIST_FREEDATA);

	// Create new list
	if ((data->filetype_list = Att_NewList(0)))
	{
		// Go through filetype lists
		for (list = (Cfg_FiletypeList *)data->list_list.lh_Head; list->node.ln_Succ;
			 list = (Cfg_FiletypeList *)list->node.ln_Succ)
		{
			// Go through filetypes in this list
			for (type = (Cfg_Filetype *)list->filetype_list.lh_Head; type->node.ln_Succ;
				 type = (Cfg_Filetype *)type->node.ln_Succ)
			{
				// Add an entry for this filetype
				filetype_add_entry(data, type);
			}
		}
	}

	// Add list to gadget
	SetGadgetChoices(data->objlist, GAD_FILETYPES_LIST, data->filetype_list);

	// Clear current selection
	SetGadgetValue(data->objlist, GAD_FILETYPES_LIST, (ULONG)-1);
	data->sel_filetype = 0;

	// Disable buttons
	filetype_able_buttons(data, TRUE);
}

// Add an entry to the filetype list
Att_Node *filetype_add_entry(config_filetypes_data *data, Cfg_Filetype *type)
{
	char buf[44];
	FiletypeNode *node;
	Att_Node *new_node;

	// Allocate new node
	if ((node = AllocVec(sizeof(FiletypeNode), MEMF_CLEAR)))
	{
		// Fill out node
		node->type = type;

		// Build display entry
		lsprintf(buf, "%s\t%s", type->type.name, type->type.id);

		// Add entry to list for this filetype
		if ((new_node = Att_NewNode(data->filetype_list, buf, (IPTR)node, ADDNODE_SORT)))
			return new_node;
		FreeVec(node);
	}

	return 0;
}

// Edit a filetype
void filetype_edit(config_filetypes_data *data, FiletypeNode *node, short edit_flag)
{
	filetype_ed_data *eddata;

	// Valid node?
	if (!node)
		return;

	// Check editor is not already up for this node
	if (node->editor)
	{
		// Send activate message
		IPC_Command(node->editor, IPC_ACTIVATE, 0, 0, 0, 0);
		return;
	}

	// Allocate data for the editor
	if (!(eddata = AllocVec(sizeof(filetype_ed_data), MEMF_CLEAR)))
		return;

	// Try to copy filetype
	if (!(eddata->type = CopyFiletype(node->type, 0)))
	{
		FreeVec(eddata);
		return;
	}

	// Fill out data
	eddata->owner_ipc = data->ipc;
	eddata->node = node;
	eddata->action_lookup = filetype_action_lookup;

	// Supply libraries
	/*eddata->func_startup.dopus_base=DOpusBase;
	eddata->func_startup.dos_base=(struct Library *)DOSBase;
	eddata->func_startup.int_base=(struct Library *)IntuitionBase;
	eddata->func_startup.util_base=(struct Library *)UtilityBase;
	eddata->func_startup.cx_base=CxBase;
	eddata->func_startup.wb_base=WorkbenchBase;
	eddata->func_startup.gfx_base=(struct Library *)GfxBase;
	eddata->func_startup.asl_base=AslBase;
	eddata->func_startup.layers_base=LayersBase;*/

	// Supply locale
	eddata->func_startup.locale = locale;

	// Supply gui data pointers
	eddata->new_win.parent = data->window;
	eddata->new_win.dims = &_filetype_editor_window;
	eddata->new_win.locale = locale;
	eddata->obj_def = _filetype_editor_objects;

	// Supply data pointers for function editor
	eddata->func_startup.win_def = &_function_editor_window;
	eddata->func_startup.obj_def = _function_editor_objects;
	eddata->func_startup.func_labels = _function_type_labels;
	eddata->func_startup.flag_list = _funced_flaglist;
	eddata->func_startup.func_list = data->command_list;
	eddata->func_startup.flags = FUNCEDF_NO_KEY;
	eddata->func_startup.main_owner = data->owner_ipc;

	// Supply data pointers for the fileclass editor
	eddata->class_win = &_fileclass_editor_window;
	eddata->class_obj = _fileclass_editor_objects;
	eddata->class_lookup = fileclass_match_lookup;
	eddata->class_strings = matchtype_labels;
	eddata->edit_flag = edit_flag;

	// Launch editor
	if (!(IPC_Launch(&data->proc_list,
					 &node->editor,
					 "dopus_filetype_editor",
					 IPC_NATIVE(FiletypeEditor),
					 STACK_DEFAULT,
					 (IPTR)eddata,
					 (struct Library *)DOSBase)) ||
		!node->editor)
	{
		// Failed; free data
		FreeFiletype(eddata->type);
		FreeVec(eddata);
	}
}

// Receive an edited filetype
BOOL filetype_receive_edit(config_filetypes_data *data, Cfg_Filetype *type, FiletypeNode *node)
{
	BOOL success = 0;
	Cfg_Filetype *type_copy;
	Att_Node *display_node;

	// Try to copy the new filetype
	if ((type_copy = CopyFiletype(type, data->memory)))
	{
		// Get list pointer from existing filetype
		type_copy->list = node->type->list;
		type_copy->list->flags |= FTLISTF_CHANGED;

		// Free existing filetype
		FreeFiletype(node->type);

		// Add new filetype to list
		AddTail(&type_copy->list->filetype_list, &type_copy->node);

		// Remove existing display list
		SetGadgetChoices(data->objlist, GAD_FILETYPES_LIST, (APTR)~0);

		// Free display node
		if ((display_node = Att_FindNodeData(data->filetype_list, (IPTR)node)))
			Att_RemNode(display_node);
		FreeVec(node);

		// Add new filetype entry
		filetype_add_entry(data, type_copy);

		// Reattach list to gadget
		SetGadgetChoices(data->objlist, GAD_FILETYPES_LIST, data->filetype_list);

		// Set success flag
		success = 1;
	}

	return success;
}

// Remove a filetype
short filetype_remove(config_filetypes_data *data, Att_Node *node, short store)
{
	FiletypeNode *ftnode;
	struct DOpusSimpleRequest simple;
	char buf[100], *gadgets[3];
	int ret_vals[2];
	short change = 0;

	// Get filetype node
	ftnode = (FiletypeNode *)node->data;

	// Fill out requester structure
	simple.title = GetString(locale, MSG_FILETYPE_EDITOR_TITLE);
	simple.message = buf;
	simple.gadgets = gadgets;
	simple.return_values = ret_vals;
	simple.string_buffer = 0;
	simple.flags = SRF_IPC;
	simple.font = 0;
	simple.ipc = data->ipc;

	// Build message
	lsprintf(buf, GetString(locale, MSG_FILETYPE_REMOVE_REQ), ftnode->type->type.name);

	// Fill in gadgets
	gadgets[0] = GetString(locale, MSG_FILETYPES_REMOVE);
	gadgets[1] = GetString(locale, MSG_CANCEL);
	gadgets[2] = 0;
	ret_vals[0] = 1;
	ret_vals[1] = 0;

	// Make window busy
	SetWindowBusy(data->window);

	// Display requester
	if (store || (change = DoSimpleRequest(data->window, &simple)) == 1)
	{
		// Is the editor open for this filetype?
		if (ftnode->editor)
		{
			// Tell it to quit
			IPC_Quit(ftnode->editor, -1, 0);
		}

		// Remove filetype list from lister
		SetGadgetChoices(data->objlist, GAD_FILETYPES_LIST, (APTR)~0);

		// Mark this list as changed
		ftnode->type->list->flags |= FTLISTF_CHANGED;

		// Stored?
		if (store)
		{
			ftnode->type->list->flags |= FTLISTF_STORE;
			change = 1;
		}

		// Removed
		else
		{
			// Free filetype
			FreeFiletype(ftnode->type);
		}

		// Free node data and node
		FreeVec(ftnode);
		Att_RemNode(node);

		// Reattach list with no selection
		SetGadgetValue(data->objlist, GAD_FILETYPES_LIST, (ULONG)-1);
		SetGadgetChoices(data->objlist, GAD_FILETYPES_LIST, data->filetype_list);

		// Disable buttons
		filetype_able_buttons(data, TRUE);
	}

	// Make window unbusy
	ClearWindowBusy(data->window);

	return change;
}

// Lock or create a drawer
static void filetype_make_drawer(char *path)
{
	BPTR lock;

	if ((lock = Lock(path, ACCESS_READ)) || (lock = CreateDir(path)))
		UnLock(lock);
}

// Save filetypes
short filetype_save(config_filetypes_data *data)
{
	Cfg_FiletypeList *list;
	Cfg_Filetype *type;
	short ret = 1;
	struct MsgPort *port;

	// Open a port to stop dopus re-reading
	if ((port = CreateMsgPort()))
	{
		port->mp_Node.ln_Name = "FILETYPE LOCK";
		AddPort(port);
	}

	// Go through filetype lists
	for (list = (Cfg_FiletypeList *)data->list_list.lh_Head; list->node.ln_Succ;
		 list = (Cfg_FiletypeList *)list->node.ln_Succ)
	{
		// Has this list been modified?
		if (list->flags & FTLISTF_CHANGED)
		{
			// Clear flag
			list->flags &= ~FTLISTF_CHANGED;

			// Is the list empty?
			if (IsListEmpty(&list->filetype_list))
			{
				// Valid list path?
				if (list->path[0])
				{
					// Try to delete list
					if (!DeleteFile(list->path))
						ret = 0;
				}
			}

			// Otherwise
			else
			{
				short err;

				// Valid list path?
				if (!list->path[0])
				{
					char *ptr;

					// Build path from name of first filetype
					type = (Cfg_Filetype *)list->filetype_list.lh_Head;

					// Storage or normal?
					if (list->flags & FTLISTF_STORE)
						strcpy(list->path, "DOpus5:Storage/Filetypes/");
					else
						strcpy(list->path, "DOpus5:Filetypes/");

					// Check name for / characters
					for (ptr = type->type.name; *ptr; ptr++)
						if (*ptr == '/')
							*ptr = '\\';

					// Add name
					strcat(list->path, type->type.name);

					// Clear flag
					list->flags &= ~FTLISTF_STORE;
				}

				// Store?
				else if (list->flags & FTLISTF_STORE)
				{
					char buf[256];

					// Make sure the storage drawer exists
					filetype_make_drawer("DOpus5:Storage");
					filetype_make_drawer("DOpus5:Storage/Filetypes");

					// Build storage filename
					lsprintf(buf, "DOpus5:Storage/Filetypes/%s", FilePart(list->path));

					// Delete existing stored filetype
					DeleteFile(buf);

					// Move file to storage
					if (Rename(list->path, buf))
					{
						// Get names for icon
						strcat(list->path, ".info");
						strcat(buf, ".info");

						// Delete existing stored filetype
						DeleteFile(buf);

						// Move icon
						Rename(list->path, buf);

						// Clear .info suffix
						buf[strlen(buf) - 5] = 0;

						// Use path to save
						strcpy(list->path, buf);
					}

					// Clear flag
					list->flags &= ~FTLISTF_STORE;
				}

				// Success by default
				ret = 1;

				// Save filetype list
				while ((err = SaveFiletypeList(list, list->path)) > 0)
				{
					// Show error
					if (!(ret = error_saving(err, data->window)))
						break;
				}

				// Failed?
				if (!ret)
				{
					// Set change flag again
					list->flags |= FTLISTF_CHANGED;
					break;
				}
			}
		}
	}

	// Remove lock port
	if (port)
	{
		RemPort(port);
		DeleteMsgPort(port);
	}

	return ret;
}

// Edit a specific filetype
void filetype_edit_name(config_filetypes_data *data, char *name)
{
	Att_Node *node;
	char *path;

	// Store path pointer
	path = name;

	// Full path specified?
	if (strchr(name, ':') || strchr(name, '/'))
	{
		// Get pointer to filename only
		name = FilePart(name);
	}

	// Find node
	if ((node = (Att_Node *)FindNameI((struct List *)data->filetype_list, name)))
	{
		// Select the new entry
		data->sel_filetype = node;
		SetGadgetValue(data->objlist, GAD_FILETYPES_LIST, Att_FindNodeNumber(data->filetype_list, node));

		// Launch editor
		filetype_edit(data, (FiletypeNode *)node->data, 0);
	}

	// Load from disk?
	else
	{
		Cfg_FiletypeList *list;

		// Try to load filetype from disk
		if ((list = ReadFiletypes(path, data->memory)))
		{
			Cfg_Filetype *type;
			Att_Node *node = 0;

			// Add filetype list to main list
			AddTail(&data->list_list, &list->node);

			// Fix list path and flags
			strcpy(list->path, "DOpus5:Filetypes/");
			strcat(list->path, name);
			list->flags = FTLISTF_CHANGED;

			// Remove list from gadget
			SetGadgetChoices(data->objlist, GAD_FILETYPES_LIST, (APTR)~0);

			// Go through filetypes in list
			for (type = (Cfg_Filetype *)list->filetype_list.lh_Head; type->node.ln_Succ;
				 type = (Cfg_Filetype *)type->node.ln_Succ)
			{
				// Add entry for this filetype
				node = filetype_add_entry(data, type);
			}

			// Handle new node
			filetype_new_node(data, node);
		}
	}

	// Free name
	FreeVec(path);
}

// Handle new nodes
void filetype_new_node(config_filetypes_data *data, Att_Node *node)
{
	// Reattach list
	SetGadgetChoices(data->objlist, GAD_FILETYPES_LIST, data->filetype_list);

	// Select the new entry
	data->sel_filetype = node;
	SetGadgetValue(data->objlist, GAD_FILETYPES_LIST, Att_FindNodeNumber(data->filetype_list, node));

	// Enable buttons
	filetype_able_buttons(data, FALSE);

	// Launch editor for this filetype
	if (node)
	{
		filetype_edit(data, (FiletypeNode *)node->data, 1);
		data->change = 1;
	}
}

// Enable/disable buttons
void filetype_able_buttons(config_filetypes_data *data, short state)
{
	DisableObject(data->objlist, GAD_FILETYPES_DUPLICATE, state);
	DisableObject(data->objlist, GAD_FILETYPES_REMOVE, state);
	DisableObject(data->objlist, GAD_FILETYPES_EDIT, state);
	DisableObject(data->objlist, GAD_FILETYPES_STORE, state);
}

// Error saving requester
short error_saving(short err, struct Window *window)
{
	char buf[80], error_text[140];

	// Build requester text
	Fault(err, "", buf, 80);
	lsprintf(
		error_text, "%s\n%s %ld%s", GetString(locale, MSG_ERROR_SAVING), GetString(locale, MSG_DOS_ERROR), err, buf);

	// Display requester
	return (short)SimpleRequestTags(window, 0, GetString(locale, MSG_RETRY_CANCEL), error_text);
}
