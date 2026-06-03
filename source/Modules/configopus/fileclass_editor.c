#include "config_lib.h"
#include "config_filetypes.h"

/*#define DOpusBase		(data->dopus_base)
#define DOSBase			(data->dos_base)
#define IntuitionBase	(data->int_base)
#define GfxBase			(data->gfx_base)
#define RexxSysBase		(data->rexx_base)
#define AslBase			(data->asl_base)*/

//#ifdef __amigaos4__
// ULONG ASM _fileclassed_init(REG(a0, IPCData *ipc), REG(a2, int skip), REG(a1, fileclass_ed_data *data))
//#else
IPC_StartupCode(_fileclassed_init, fileclass_ed_data *, data)
//#endif
{
	short a;

	// Save IPC pointer
	data->ipc = ipc;

	// Create a list of match strings
	if (!(data->command_list = Att_NewList(0)))
		return 0;
	for (a = 0; data->strings[a]; a += 2)
	{
		char buf[80];

		// Build string
		stccpy(buf, GetString(data->new_win.locale, data->strings[a]), sizeof(buf));
		strcat(buf, "\t");
		strcat(buf, GetString(data->new_win.locale, data->strings[a + 1]));

		// Add node
		Att_NewNode(data->command_list, buf, 0, 0);
	}

	// Open window
	if (!(data->window = OpenConfigWindow(&data->new_win)) ||
		!(data->objlist = AddObjectList(data->window, data->obj_def)))
	{
		CloseConfigWindow(data->window);
		Att_RemList(data->command_list, 0);
		return 0;
	}

	// Build definition list
	_fileclassed_build_list(data);

	// Attach list to lister
	SetGadgetChoices(data->objlist, GAD_CLASSED_DEFINITION, data->match_list);

	// Fill out fields
	SetGadgetValue(data->objlist, GAD_CLASSED_CLASS_NAME, (IPTR)data->type->type.name);
	SetGadgetValue(data->objlist, GAD_CLASSED_CLASS_ID, (IPTR)data->type->type.id);
	SetGadgetValue(data->objlist, GAD_CLASSED_CLASS_PRI, data->type->type.priority);

	// Disable edit fields
	DisableObject(data->objlist, GAD_CLASSED_MATCHTYPE, TRUE);
	DisableObject(data->objlist, GAD_CLASSED_MATCHDATA, TRUE);

	return 1;
}

void SAVEDS FileclassEditor(void)
{
	fileclass_ed_data *data;
	IPCData *ipc;
	short success = 0;
	ReaderNode *node;

	// Do startup
	if (!(ipc = Local_IPC_ProcStartup((IPTR *)&data, (APTR)&_fileclassed_init)))
		return;

	// Open REXX library
	/*if ((data->rexx_base=OpenLibrary("rexxsyslib.library",0)) &&
		!(data->reply_port=CreateMsgPort()))
	{
		CloseLibrary(data->rexx_base);
		data->rexx_base=0;
	}

	// Can't view files if no ARexx
	if (!RexxSysBase) DisableObject(data->objlist,GAD_FILECLASSED_VIEW_FILE,TRUE);*/
	if (!(data->reply_port = CreateMsgPort()))
		DisableObject(data->objlist, GAD_FILECLASSED_VIEW_FILE, TRUE);

	// Message loop
	FOREVER
	{
		IPCMessage *submsg;
		struct IntuiMessage *imsg;
		BOOL break_flag = 0;

		// Task message?
		while ((submsg = (IPCMessage *)GetMsg(ipc->command_port)))
		{
			// Look at message
			switch (submsg->command)
			{
			// Quit immediately
			case IPC_QUIT:
				success = submsg->flags;
				break_flag = 1;
				break;

			// Activate
			case IPC_ACTIVATE:
				if (data->window)
				{
					WindowToFront(data->window);
					ActivateWindow(data->window);
				}
				break;
			}

			// Reply the message
			IPC_Reply(submsg);
		}

		// Intuimessage
		if (data->window)
		{
			while ((imsg = GetWindowMsg(data->window->UserPort)))
			{
				// Copy message and reply
				struct IntuiMessage msg_copy = *imsg;
				ReplyWindowMsg(imsg);

				// Look at message
				switch (msg_copy.Class)
				{
				// Close window
				case IDCMP_CLOSEWINDOW:
					break_flag = 1;
					break;

				// Gadget
				case IDCMP_GADGETUP:
				case IDCMP_GADGETDOWN:

					switch (((struct Gadget *)msg_copy.IAddress)->GadgetID)
					{
					// Use
					case GAD_FILECLASSED_USE:
						success = 1;

					// Cancel
					case GAD_FILECLASSED_CANCEL:
						break_flag = 1;
						break;

					// View file
					case GAD_FILECLASSED_VIEW_FILE:
						fileclassed_view_file(data);
						break;

					// Entry in the definition lister
					case GAD_CLASSED_DEFINITION: {
						Att_Node *node;

						// Get selected node
						if (!(node = Att_FindNode(data->match_list, msg_copy.Code)))
							break;

						// Is it the node we're editing?
						if (node == data->match_edit)
							break;

						// End any current edits
						_fileclassed_end_edit(data, 0);

						// Start editing this node
						_fileclassed_start_edit(data, node);
					}
					break;

					// Match type changed
					case GAD_CLASSED_MATCHTYPE:

						// Were we editing something?
						if (data->match_edit)
						{
							FileclassNode *fcnode;
							short selection;

							// Get selection
							SetWindowBusy(data->window);
							selection = SelectionList(data->command_list,
													  data->window,
													  0,
													  GetString(data->new_win.locale, MSG_FUNCED_SELECT_COMMAND),
													  -1,
													  0,
													  0,
													  GetString(data->new_win.locale, MSG_OKAY),
													  GetString(data->new_win.locale, MSG_CANCEL),
													  0,
													  0);
							ClearWindowBusy(data->window);

							// Nothing selected?
							if (selection < 0)
								break;

							// Get fileclass node
							fcnode = (FileclassNode *)data->match_edit->data;

							// Get match type
							fcnode->match_type = data->lookup[selection];
							data->last_selection = selection;

							// Update command display
							SetGadgetValue(data->objlist,
										   GAD_CLASSED_MATCHTYPE_DISPLAY,
										   (IPTR)GetString(data->new_win.locale, data->strings[selection << 1]));

							// Remove list from listview
							SetGadgetChoices(data->objlist, GAD_CLASSED_DEFINITION, (APTR)~0);

							// Rebuild display node
							_fileclassed_build_display(data, data->match_edit);

							// Attach list to lister
							SetGadgetChoices(data->objlist, GAD_CLASSED_DEFINITION, data->match_list);

							// Or/And/Directory = disable field
							DisableObject(data->objlist,
										  GAD_CLASSED_MATCHDATA,
										  (fcnode->match_type == FTOP_OR || fcnode->match_type == FTOP_AND ||
										   fcnode->match_type == FTOP_DIRECTORY || fcnode->match_type == FTOP_MODULE));

							// Activate field
							ActivateStrGad(GADGET(GetObject(data->objlist, GAD_CLASSED_MATCHDATA)), data->window);
						}
						break;

					// Return pressed on match data field
					case GAD_CLASSED_MATCHDATA:

						// End current edit
						_fileclassed_end_edit(data, 1);
						break;

					// Add an entry
					case GAD_CLASSED_ADD:
					case GAD_CLASSED_INSERT: {
						FileclassNode *fcnode;
						Att_Node *node = 0, *old_edit;

						// End current edit
						old_edit = data->match_edit;
						_fileclassed_end_edit(data, 0);

						// Remove list from listview
						SetGadgetChoices(data->objlist, GAD_CLASSED_DEFINITION, (APTR)~0);

						// Create a new fileclass node
						if ((fcnode = AllocVec(sizeof(FileclassNode), MEMF_CLEAR)))
						{
							// Add a new node
							if ((node = Att_NewNode(data->match_list, 0, (IPTR)fcnode, 0)))
							{
								// Inserting?
								if (((struct Gadget *)msg_copy.IAddress)->GadgetID == GAD_CLASSED_INSERT && old_edit)
								{
									Att_PosNode(data->match_list, node, old_edit);
								}

								// Set initial type
								fcnode->match_type = data->lookup[data->last_selection];

								// Update command display
								SetGadgetValue(
									data->objlist,
									GAD_CLASSED_MATCHTYPE_DISPLAY,
									(IPTR)GetString(data->new_win.locale, data->strings[data->last_selection << 1]));

								// Build display string
								_fileclassed_build_display(data, node);
							}
							else
								FreeVec(fcnode);
						}

						// Attach list to lister
						SetGadgetChoices(data->objlist, GAD_CLASSED_DEFINITION, data->match_list);

						// Make new node the selected on
						SetGadgetValue(
							data->objlist, GAD_CLASSED_DEFINITION, Att_FindNodeNumber(data->match_list, node));

						// Edit this node
						if (node)
							_fileclassed_start_edit(data, node);
					}
					break;

					// Remove an entry
					case GAD_CLASSED_REMOVE:

						// End current edit (with remove flag)
						_fileclassed_end_edit(data, 2);
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
						IPC_Command(data->main_owner, IPC_HELP, (1 << 31), "File Class Editor", 0, REPLY_NO_PORT);

						// Clear busy pointer
						ClearWindowBusy(data->window);
					}
					break;
				}
			}
		}

		// Check break flag
		if (break_flag)
			break;

		// Wait for message
		Wait(1 << ipc->command_port->mp_SigBit | ((data->window) ? (1 << data->window->UserPort->mp_SigBit) : 0));
	}

	// Edit successful?
	if (success)
	{
		// Turn busy pointer on
		SetWindowBusy(data->window);

		// Get new name, id and priority
		stccpy(data->type->type.name,
			   (char *)GetGadgetValue(data->objlist, GAD_CLASSED_CLASS_NAME),
			   sizeof(data->type->type.name));
		stccpy(data->type->type.id,
			   (char *)GetGadgetValue(data->objlist, GAD_CLASSED_CLASS_ID),
			   sizeof(data->type->type.id));
		data->type->type.priority = (char)GetGadgetValue(data->objlist, GAD_CLASSED_CLASS_PRI);

		// Build new recognition string
		_fileclassed_build_recognition(data);

		// Send new function back
		IPC_Command(data->owner_ipc, CLASSEDIT_RETURN, 0, data->type, 0, REPLY_NO_PORT);
	}

	// Close window
	CloseConfigWindow(data->window);

	// Go through readers
	for (node = (ReaderNode *)data->readers.mlh_Head; node->node.mln_Succ;)
	{
		char command[80];
		ReaderNode *next = (ReaderNode *)node->node.mln_Succ;

		// Build quit command
		lsprintf(command, "dopus read %ld quit", node->handle);

		// Send command
		classed_send_rexx(data, command, 0);

		// Free node
		FreeVec(node);

		// Get next
		node = next;
	}

	// Free command list
	Att_RemList(data->command_list, 0);

	// Say goodbye
	IPC_Goodbye(ipc, data->owner_ipc, (ULONG)-1);

	// Close REXX library
	// CloseLibrary(data->rexx_base);
	DeleteMsgPort(data->reply_port);

	// Free data
	IPC_Free(ipc);
	Att_RemList(data->match_list, REMLIST_FREEDATA);
	FreeFiletype(data->type);
	FreeVec(data);
}

// Builds fileclass definition list
void _fileclassed_build_list(fileclass_ed_data *data)
{
	unsigned short pos = 0, bufpos = 0;
	unsigned char *recognition;
	char buf[80];
	short type = -1;

	// Create new list
	if (!(data->match_list = Att_NewList(0)))
		return;

	// Valid recognition string?
	if ((recognition = data->type->recognition))
	{
		// Parse string
		do
		{
			// No type set yet
			if (type == -1)
			{
				// Valid type?
				if (recognition[pos] < FTOP_LAST)
				{
					type = recognition[pos];
				}

				// And/Or
				else if (recognition[pos] == FTOP_OR || recognition[pos] == FTOP_AND)
				{
					// Create entry
					_fileclassed_new_entry(data, recognition[pos], 0);

					// Reset stuff
					bufpos = 0;
					type = -1;
				}
			}

			// Otherwise, completed a section
			else if (bufpos == 79 || !recognition[pos] || recognition[pos] > FTOP_SPECIAL)
			{
				// Null-terminate buffer
				buf[bufpos] = 0;

				// Create new entry
				_fileclassed_new_entry(data, type, buf);

				// Reset stuff
				bufpos = 0;
				type = -1;

				// Handle and/or
				if (recognition[pos] == FTOP_OR || recognition[pos] == FTOP_AND)
					--pos;
			}

			// Else, just add character to buffer
			else
				buf[bufpos++] = recognition[pos];
		} while (recognition[pos++]);
	}
}

// Builds recognition string from fileclass definition list
void _fileclassed_build_recognition(fileclass_ed_data *data)
{
	Att_Node *node;
	long size = 1, bufpos = 0;

	// Free existing recognition string
	FreeMemH(data->type->recognition);
	data->type->recognition = 0;

	// If no valid list, return
	if (!data->match_list)
		return;

	// Go through nodes to get size of buffer needed
	for (node = (Att_Node *)data->match_list->list.lh_Head; node->node.ln_Succ; node = (Att_Node *)node->node.ln_Succ)
	{
		// Get node data
		FileclassNode *fcnode = (FileclassNode *)node->data;

		// Add one for type
		++size;

		// Check valid type for a string
		if (fcnode->match_type < FTOP_LAST)
		{
			// Add length of string (if any)
			if (fcnode->match_data[0])
				size += strlen(fcnode->match_data) + 1;
		}
	}

	// Allocate recognition buffer
	if (size == 0 || !(data->type->recognition = AllocMemH(0, size)))
		return;

	// Go through nodes to build recognition string
	for (node = (Att_Node *)data->match_list->list.lh_Head; node->node.ln_Succ; node = (Att_Node *)node->node.ln_Succ)
	{
		// Get node data
		FileclassNode *fcnode = (FileclassNode *)node->data;

		// Check valid type
		if (fcnode->match_type < FTOP_LAST)
		{
			// Store type
			data->type->recognition[bufpos++] = fcnode->match_type;

			// Add string (if valid)
			if (fcnode->match_data[0])
			{
				// Copy match data
				strcpy(data->type->recognition + bufpos, fcnode->match_data);

				// Get new end of string
				bufpos += strlen(fcnode->match_data);
			}

			// Set ENDSECTION char
			data->type->recognition[bufpos] = FTOP_ENDSECTION;
		}

		// And/Or?
		else if (fcnode->match_type == FTOP_AND || fcnode->match_type == FTOP_OR)
		{
			// See if we need to overwrite ENDSECTION
			if (bufpos > 0 && data->type->recognition[bufpos - 1] == FTOP_ENDSECTION)
				--bufpos;

			// Set type
			data->type->recognition[bufpos] = fcnode->match_type;
		}

		// Increment buffer position
		++bufpos;
	}

	// Null terminate string
	data->type->recognition[size - 1] = 0;
}

// Add a new entry to the list
Att_Node *_fileclassed_new_entry(fileclass_ed_data *data, short type, char *match)
{
	Att_Node *node;
	FileclassNode *classnode;

	// Allocate file class node
	if (!(classnode = AllocVec(sizeof(FileclassNode), MEMF_CLEAR)))
		return 0;

	// Fill out node
	classnode->match_type = type;
	if (match)
		strcpy(classnode->match_data, match);

	// Add a new node to the list
	if (!(node = Att_NewNode(data->match_list, 0, (IPTR)classnode, 0)))
	{
		FreeVec(classnode);
		return 0;
	}

	// Build display string
	_fileclassed_build_display(data, node);

	return node;
}

// Build display string for a node
void _fileclassed_build_display(fileclass_ed_data *data, Att_Node *node)
{
	char buf[120];
	short entry;

	// Find entry in string table
	for (entry = 0; data->lookup[entry]; entry++)
	{
		if (data->lookup[entry] == ((FileclassNode *)node->data)->match_type)
			break;
	}

	// Build new display string
	strcpy(buf, GetString(data->new_win.locale, data->strings[entry << 1]));
	if (((FileclassNode *)node->data)->match_type != FTOP_AND && ((FileclassNode *)node->data)->match_type != FTOP_OR &&
		((FileclassNode *)node->data)->match_type != FTOP_DIRECTORY &&
		((FileclassNode *)node->data)->match_type != FTOP_MODULE && ((FileclassNode *)node->data)->match_data[0])
	{
		strcat(buf, "\a\xf");
		strcat(buf, ((FileclassNode *)node->data)->match_data);
	}

	// Set new name
	Att_ChangeNodeName(node, buf);
}

// Start editing a node
void _fileclassed_start_edit(fileclass_ed_data *data, Att_Node *node)
{
	FileclassNode *fcnode;
	short type;

	// Get fileclass node
	fcnode = (FileclassNode *)node->data;

	// Fill out edit fields
	for (type = 0; data->lookup[type]; type++)
		if (data->lookup[type] == fcnode->match_type)
			break;

	// Set match command
	SetGadgetValue(
		data->objlist, GAD_CLASSED_MATCHTYPE_DISPLAY, (IPTR)GetString(data->new_win.locale, data->strings[type << 1]));
	data->last_selection = type;
	SetGadgetValue(data->objlist, GAD_CLASSED_MATCHDATA, (IPTR)fcnode->match_data);

	// Enable edit fields
	DisableObject(data->objlist, GAD_CLASSED_MATCHTYPE, FALSE);
	DisableObject(data->objlist,
				  GAD_CLASSED_MATCHDATA,
				  (fcnode->match_type == FTOP_AND || fcnode->match_type == FTOP_OR ||
				   fcnode->match_type == FTOP_DIRECTORY || fcnode->match_type == FTOP_MODULE));

	// Store pointer to edit node
	data->match_edit = node;

	// Activate edit field
	ActivateStrGad(GADGET(GetObject(data->objlist, GAD_CLASSED_MATCHDATA)), data->window);
}

// Stop editing an entry
void _fileclassed_end_edit(fileclass_ed_data *data, short disable)
{
	// Were we editing something?
	if (data->match_edit)
	{
		FileclassNode *fcnode;

		// Get fileclass node
		fcnode = (FileclassNode *)data->match_edit->data;

		// Remove list from listview
		SetGadgetChoices(data->objlist, GAD_CLASSED_DEFINITION, (APTR)~0);

		// And/Or/Directory has no data
		if (disable != 2 && (fcnode->match_type == FTOP_AND || fcnode->match_type == FTOP_OR ||
							 fcnode->match_type == FTOP_DIRECTORY || fcnode->match_type == FTOP_MODULE))
		{
			fcnode->match_data[0] = 0;
		}

		// Otherwise, get match data
		else
		{
			// Copy data
			strcpy(fcnode->match_data, (char *)GetGadgetValue(data->objlist, GAD_CLASSED_MATCHDATA));

			// If edit field is empty (except for disk), delete node
			if ((!fcnode->match_data[0] && fcnode->match_type != FTOP_DISK) || disable == 2)
			{
				// Remove node
				Att_RemNode(data->match_edit);

				// Free node data
				FreeVec(fcnode);
				fcnode = 0;
			}
		}

		// Rebuild this display node if necessary
		if (fcnode)
			_fileclassed_build_display(data, data->match_edit);

		// Attach list to lister
		SetGadgetChoices(data->objlist, GAD_CLASSED_DEFINITION, data->match_list);

		// Clear and disable fields?
		if (disable)
		{
			// Clear edit fields
			SetGadgetValue(data->objlist, GAD_CLASSED_MATCHDATA, 0);
			SetGadgetValue(data->objlist, GAD_CLASSED_MATCHTYPE_DISPLAY, 0);

			// Disable edit fields
			DisableObject(data->objlist, GAD_CLASSED_MATCHTYPE, TRUE);
			DisableObject(data->objlist, GAD_CLASSED_MATCHDATA, TRUE);

			// Clear lister selection
			SetGadgetValue(data->objlist, GAD_CLASSED_DEFINITION, (ULONG)-1);
		}

		// Clear edit pointer
		data->match_edit = 0;
	}
}

// View file
void fileclassed_view_file(fileclass_ed_data *data)
{
	struct FileRequester *filereq;
	struct TagItem tags[5];

	// Get requester pointer
	if (!(filereq = WINREQUESTER(data->window)))
	{
		DisplayBeep(data->window->WScreen);
		return;
	}

	// Requester tags
	tags[0].ti_Tag = ASLFR_Window;
	tags[0].ti_Data = (IPTR)data->window;
	tags[1].ti_Tag = ASLFR_TitleText;
	tags[1].ti_Data = (IPTR)GetString(data->new_win.locale, MSG_CLASSED_SELECT_VIEW_FILE);
	tags[2].ti_Tag = ASLFR_Flags1;
	tags[2].ti_Data = FRF_PRIVATEIDCMP;
	tags[3].ti_Tag = ASLFR_InitialDrawer;
	tags[3].ti_Data = (IPTR)"DOpus5:";
	tags[4].ti_Tag = TAG_DONE;

	// Put window to sleep
	SetWindowBusy(data->window);

	// Show requester
	if (AslRequest(filereq, tags))
	{
		char buf[256], command[300];

		// Get filename
		stccpy(buf, filereq->fr_Drawer, sizeof(buf));
		AddPart(buf, filereq->fr_File, 256);

		// Build command
		lsprintf(command, "dopus hexread %s", buf);

		// Send command
		classed_send_rexx(data, command, 1);
	}

	// Wake window up
	ClearWindowBusy(data->window);
}

// Send rexx command
void classed_send_rexx(fileclass_ed_data *data, char *command, short open)
{
	char portname[30];
	struct RexxMsg *msg;
	struct MsgPort *port;

	// Get port name
	portname[0] = 0;
	IPC_Command(data->main_owner, IPC_IDENTIFY, sizeof(portname), portname, 0, REPLY_NO_PORT);

	// No port name?
	if (!portname[0])
		return;

	// Create message
	if (!(msg = CreateRexxMsg(data->reply_port, 0, portname)))
		return;

	// Fill in message
	msg->rm_Action |= RXFF_RESULT;
	msg->rm_Args[0] = (IPTR)command;
	FillRexxMsg(msg, 1, 0);

	// Forbid and look for port
	Forbid();
	if ((port = FindPort(portname)))
	{
		// Send message
		PutMsg(port, (struct Message *)msg);
		msg = 0;
	}

	// Permit
	Permit();

	// Was message sent?
	if (!msg)
	{
		ULONG handle = 0;

		// Wait for reply
		WaitPort(data->reply_port);

		// Get reply
		msg = (struct RexxMsg *)GetMsg(data->reply_port);

		// Opened a new viewer?
		if (open)
		{
			// Get handle
			if (msg->rm_Result2)
				handle = (ULONG)atoi((char *)msg->rm_Result2);

			// Opened viewer?
			if (handle)
			{
				ReaderNode *node;

				// Allocate new node
				if ((node = AllocVec(sizeof(ReaderNode), MEMF_CLEAR)))
				{
					// Store handle
					node->handle = handle;

					// Add to list
					AddTail((struct List *)&data->readers, (struct Node *)node);
				}
			}
		}

		// Free the result
		if (msg->rm_Result1 == 0 && msg->rm_Result2 != 0)
			DeleteArgstring((char *)msg->rm_Result2);
	}

	// Clear message
	ClearRexxMsg(msg, 1);

	// Free message
	DeleteRexxMsg(msg);
}
