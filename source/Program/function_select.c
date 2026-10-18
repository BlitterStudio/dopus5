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

int function_select_simple(SelectData *, FunctionHandle *);
int function_select_complex(IPCData *, SelectData *, struct Window *);
void function_select_disable(ObjectList *objlist, SelectData *data, short);

// SELECT internal function
DOPUS_FUNC(function_select)
{
	Lister *lister;
	SelectData *data;
	SelectWildData *packet;
	short ret = 0;
	ULONG side;

	// Get current lister
	if (!(lister = function_lister_current(&handle->source_paths)))
		return 1;

	side = function_path_side(handle->source_paths.current);

	// Allocate and copy local select data
	if (!(data = AllocVec(sizeof(SelectData), MEMF_CLEAR)))
		return 0;

	// Parsed arguments?
	if (instruction->funcargs)
	{
		long val;

		// Convert from arguments
		data->type = SELECT_COMPLEX;

		// Name match stuff
		data->name_match = SELECT_MATCH_IGNORE;
		if (instruction->funcargs->FA_Arguments[SELECT_VAR_NAME])
		{
			stccpy(data->name, (char *)instruction->funcargs->FA_Arguments[SELECT_VAR_NAME], 59);
			data->name_match = SELECT_MATCH_MATCH;
		}
		if (instruction->funcargs->FA_Arguments[SELECT_VAR_NOMATCHNAME])
			data->name_match = SELECT_MATCH_NO_MATCH;

		// Date match stuff
		data->date_match = SELECT_MATCH_IGNORE;
		if (instruction->funcargs->FA_Arguments[SELECT_VAR_FROM])
		{
			stccpy(data->date_from, (char *)instruction->funcargs->FA_Arguments[SELECT_VAR_FROM], 10);
			data->date_match = SELECT_MATCH_MATCH;
		}
		if (instruction->funcargs->FA_Arguments[SELECT_VAR_TO])
		{
			stccpy(data->date_to, (char *)instruction->funcargs->FA_Arguments[SELECT_VAR_TO], 10);
			data->date_match = SELECT_MATCH_MATCH;
		}
		if (instruction->funcargs->FA_Arguments[SELECT_VAR_NOMATCHDATE])
			data->date_match = SELECT_MATCH_NO_MATCH;

		// Bits match stuff
		data->bits_match = SELECT_MATCH_IGNORE;
		if (instruction->funcargs->FA_Arguments[SELECT_VAR_BITSON])
		{
			val = prot_from_string((char *)instruction->funcargs->FA_Arguments[SELECT_VAR_BITSON]);
			val = ((val & 0xf0) | ((~val) & 0xf)) & 0xff;
			data->bits |= (val << 8);
			data->bits_match = SELECT_MATCH_MATCH;
		}
		if (instruction->funcargs->FA_Arguments[SELECT_VAR_BITSOFF])
		{
			val = prot_from_string((char *)instruction->funcargs->FA_Arguments[SELECT_VAR_BITSOFF]);
			val = ((val & 0xf0) | ((~val) & 0xf)) & 0xff;
			data->bits |= (val & 0xff);
			data->bits_match = SELECT_MATCH_MATCH;
		}
		if (instruction->funcargs->FA_Arguments[SELECT_VAR_NOMATCHBITS])
			data->bits_match = SELECT_MATCH_NO_MATCH;

		// Compare match stuff
		data->compare_match = SELECT_MATCH_IGNORE;
		if (instruction->funcargs->FA_Arguments[SELECT_VAR_COMPARE])
		{
			if (stricmp((char *)instruction->funcargs->FA_Arguments[SELECT_VAR_COMPARE], "newer") == 0)
				data->compare = SELECT_COMPARE_NEWER;
			else if (stricmp((char *)instruction->funcargs->FA_Arguments[SELECT_VAR_COMPARE], "older") == 0)
				data->compare = SELECT_COMPARE_OLDER;
			else
				data->compare = SELECT_COMPARE_DIFFERENT;
			data->compare_match = SELECT_MATCH_MATCH;
		}
		if (instruction->funcargs->FA_Arguments[SELECT_VAR_NOMATCHCOMPARE])
			data->compare_match = SELECT_MATCH_NO_MATCH;

		// Entry type
		if (instruction->funcargs->FA_Arguments[SELECT_VAR_FILESONLY])
			data->entry_type = SELECT_ENTRY_FILES;
		else if (instruction->funcargs->FA_Arguments[SELECT_VAR_DIRSONLY])
			data->entry_type = SELECT_ENTRY_DIRS;
		else
			data->entry_type = SELECT_ENTRY_BOTH;

		// Include type
		if (instruction->funcargs->FA_Arguments[SELECT_VAR_EXCLUDE])
			data->include = SELECT_EXCLUDE;
		else
			data->include = SELECT_INCLUDE;

		ret = 1;
	}

	// Otherwise, get a copy of the default settings
	else
	{
		GetSemaphore(&GUI->select_lock, SEMF_SHARED, 0);
		CopyMem((char *)&GUI->select_data, (char *)data, sizeof(SelectData));
		FreeSemaphore(&GUI->select_lock);

		// If first source, display requesters
		if (handle->func_source_num == 0)
		{
			// Loop selection requests
			FOREVER
			{
				// Simple?
				if (data->type == SELECT_SIMPLE)
					ret = function_select_simple(data, handle);

				// Complex
				else
					ret = function_select_complex(handle->ipc, data, lister->window);

				// Ok/Cancel?
				if (ret != 2)
					break;
			}

			// If ok, copy select data back, otherwise just store type
			GetSemaphore(&GUI->select_lock, SEMF_EXCLUSIVE, 0);
			if (ret)
			{
				CopyMem((char *)data, (char *)&GUI->select_data, sizeof(SelectData));
			}
			else
				GUI->select_data.type = data->type;
			FreeSemaphore(&GUI->select_lock);
		}
	}

	// Do wildcard selection
	if (ret)
	{
		if ((packet = AllocVec(sizeof(SelectWildData), MEMF_CLEAR)))
		{
			CopyMem(data, &packet->data, sizeof(SelectData));
			packet->dest_list = &handle->dest_paths;
			packet->side = side;

			if (!IPC_Command(lister->ipc, LISTER_SELECT_WILD, 0, packet, packet, REPLY_NO_PORT))
				FreeVec(packet);
		}
		else if (!side)
			IPC_Command(lister->ipc, LISTER_SELECT_WILD, (IPTR)&handle->dest_paths, data, 0, REPLY_NO_PORT);

		// Refresh window
		IPC_Command(lister->ipc, LISTER_REFRESH_WINDOW, 0, (APTR)(IPTR)side, 0, REPLY_NO_PORT);
		ret = 1;
	}

	// Free local select data
	FreeVec(data);
	return ret;
}

// Simple select
int function_select_simple(SelectData *data, FunctionHandle *handle)
{
	short ret;

	// Display requester
	ret = function_request(handle,
						   GetString(&locale, MSG_SELECT_ENTER_PATTERN),
						   SRF_BUFFER | SRF_PATH_FILTER,
						   data->name,
						   59,
						   GetString(&locale, MSG_OKAY),
						   GetString(&locale, MSG_SELECT_COMPLEX),
						   GetString(&locale, MSG_CANCEL),
						   0);

	// Complex?
	if (ret == 2)
		data->type = SELECT_COMPLEX;
	return ret;
}

// Complex select
int function_select_complex(IPCData *ipc, SelectData *data, struct Window *parent)
{
	struct Window *window;
	NewConfigWindow new_win;
	ConfigWindow win_dims;
	ObjectList *objlist;
	ObjectDef *prot_objects;
	short a, b, c_x, f_x;
	static char *prot_bits = "H\0S\0P\0A\0R\0W\0E\0D";

	// Fill out new window
	new_win.parent = parent;
	new_win.dims = &win_dims;
	new_win.title = GetString(&locale, MSG_SELECT_TITLE);
	new_win.locale = &locale;
	new_win.port = 0;
	new_win.flags = WINDOW_VISITOR | WINDOW_AUTO_KEYS | WINDOW_REQ_FILL;
	new_win.font = 0;

	// Fill out dimensions
	win_dims.char_dim.Left = POS_CENTER;
	win_dims.char_dim.Top = POS_CENTER;
	win_dims.char_dim.Width = 38;
	win_dims.char_dim.Height = 7;
	win_dims.fine_dim.Left = 0;
	win_dims.fine_dim.Top = 0;
	win_dims.fine_dim.Width = 194;
	win_dims.fine_dim.Height = 64;

	// Open window
	if (!(window = OpenConfigWindow(&new_win)))
		return 0;

	// Add object list
	if (!(objlist = AddObjectList(window, _function_select_objects)) ||
		!(prot_objects = AllocVec(sizeof(ObjectDef) * 17, MEMF_CLEAR)))
	{
		CloseConfigWindow(window);
		return 0;
	}

	// Create protection objects
	for (a = 0, b = 0, c_x = 8, f_x = 6; a < 16; a++, c_x++, f_x += 8)
	{
		// Fill out object
		prot_objects[a].type = OD_GADGET;
		prot_objects[a].object_kind = BUTTON_KIND;
		prot_objects[a].char_dims.Left = c_x;
		prot_objects[a].char_dims.Top = 2;
		prot_objects[a].char_dims.Width = 1;
		prot_objects[a].char_dims.Height = 1;
		prot_objects[a].fine_dims.Left = f_x;
		prot_objects[a].fine_dims.Top = 20;
		prot_objects[a].fine_dims.Width = 8;
		prot_objects[a].fine_dims.Height = 6;
		prot_objects[a].gadget_text = (IPTR)&prot_bits[b * 2];
		prot_objects[a].flags = TEXTFLAG_TEXT_STRING | BUTTONFLAG_TOGGLE_SELECT;
		prot_objects[a].gadgetid = GAD_SELECT_PROTECTION + a;
		prot_objects[a].taglist = _select_layout_tags;

		// Increment button pointer
		if (b == 7)
		{
			b = 0;
			c_x += 4;
			f_x += 8;
		}
		else
			++b;
	}

	// Last object
	prot_objects[a].type = OD_END;

	// Add protection objects, free temporary memory
	AddObjectList(window, prot_objects);
	FreeVec(prot_objects);

	// Initialise gadgets
	SetGadgetValue(objlist, GAD_SELECT_NAME_STRING, (IPTR)data->name);
	SetGadgetValue(objlist, GAD_SELECT_NAME_CYCLE, (ULONG)data->name_match);
	SetGadgetValue(objlist, GAD_SELECT_DATE_FROM, (IPTR)data->date_from);
	SetGadgetValue(objlist, GAD_SELECT_DATE_TO, (IPTR)data->date_to);
	SetGadgetValue(objlist, GAD_SELECT_DATE_CYCLE, (ULONG)data->date_match);
	SetGadgetValue(objlist, GAD_SELECT_PROT_CYCLE, (ULONG)data->bits_match);
	SetGadgetValue(objlist, GAD_SELECT_COMPARE_TYPE_CYCLE, (ULONG)data->compare);
	SetGadgetValue(objlist, GAD_SELECT_COMPARE_CYCLE, (ULONG)data->compare_match);
	for (a = 0; a < 16; a++)
	{
		if (data->bits & (1 << (15 - a)))
			SetGadgetValue(objlist, GAD_SELECT_PROTECTION + a, 1);
	}
	SetGadgetValue(objlist, GAD_SELECT_INCLUDE_EXCLUDE, (ULONG)data->include);
	SetGadgetValue(objlist, GAD_SELECT_ENTRY_TYPE, (ULONG)data->entry_type);

	// Fix gadget disabling
	function_select_disable(objlist, data, 15);

	// Activate name gadget
	ActivateStrGad(GADGET(GetObject(objlist, GAD_SELECT_NAME_STRING)), window);

	// Event loop
	FOREVER
	{
		struct IntuiMessage *msg;
		IPCMessage *imsg;

		// Check for abort
		while ((imsg = (IPCMessage *)GetMsg(ipc->command_port)))
		{
			BOOL quit = 0;

			// Check for abort
			if (imsg->command == IPC_ABORT)
				quit = 1;
			IPC_Reply(imsg);

			// Aborted?
			if (quit)
			{
				CloseConfigWindow(window);
				return 0;
			}
		}

		// Any messages?
		while ((msg = GetWindowMsg(window->UserPort)))
		{
			struct IntuiMessage msg_copy;
			struct Gadget *gadget;
			short ret;

			// Copy message and reply
			msg_copy = *msg;
			ReplyWindowMsg(msg);
			gadget = (struct Gadget *)msg_copy.IAddress;

			// Look at message
			switch (msg_copy.Class)
			{
			// Close window
			case IDCMP_CLOSEWINDOW:
				CloseConfigWindow(window);
				return 0;

			// Key
			case IDCMP_VANILLAKEY: {
				short bit;

				// See if it's a bit key
				for (bit = 0; bit < 8; bit++)
				{
					if (toupper(msg_copy.Code) == prot_bits[bit * 2])
					{
						// Shift is down?
						if (isupper(msg_copy.Code))
							bit += 8;

						// Get gadget
						gadget = GADGET(GetObject(objlist, GAD_SELECT_PROTECTION + bit));

						// Gadget not disabled?
						if (!(gadget->Flags & GFLG_DISABLED))
						{
							// Toggle state
							gadget->Flags ^= GFLG_SELECTED;
							RefreshGList(gadget, window, 0, 1);
							data->bits ^= (1 << (15 - bit));

							// Need to turn opposite bit off if this is on
							if (gadget->Flags & GFLG_SELECTED)
							{
								if (bit < 8)
									bit += 8;
								else
									bit -= 8;
								SetGadgetValue(objlist, GAD_SELECT_PROTECTION + bit, 0);
								data->bits &= ~(1 << (15 - bit));
							}
						}
					}
				}
			}
			break;

			// Gadget
			case IDCMP_GADGETUP:
			case IDCMP_GADGETDOWN:

				// Store gadget value
				StoreGadgetValue(objlist, &msg_copy);

				// Protection bits?
				if (gadget->GadgetID >= GAD_SELECT_PROTECTION)
				{
					short bit;

					// Which bit?
					bit = gadget->GadgetID - GAD_SELECT_PROTECTION;
					if (bit < 0 || bit > 15)
						break;

					// Set bit in select data
					if (gadget->Flags & GFLG_SELECTED)
						data->bits |= 1 << (15 - bit);
					else
						data->bits &= ~(1 << (15 - bit));

					// Need to turn opposite bit off if this is on
					if (gadget->Flags & GFLG_SELECTED)
					{
						if (bit < 8)
							bit += 8;
						else
							bit -= 8;
						SetGadgetValue(objlist, GAD_SELECT_PROTECTION + bit, 0);
						data->bits &= ~(1 << (15 - bit));
					}
					break;
				}

				// Look at gadget code
				switch (gadget->GadgetID)
				{
				// Match types
				case GAD_SELECT_NAME_CYCLE:
					data->name_match = msg_copy.Code;
					function_select_disable(objlist, data, 1);
					ActivateStrGad(GADGET(GetObject(objlist, GAD_SELECT_NAME_STRING)), window);
					break;

				case GAD_SELECT_DATE_CYCLE:
					data->date_match = msg_copy.Code;
					function_select_disable(objlist, data, 2);
					ActivateStrGad(GADGET(GetObject(objlist, GAD_SELECT_DATE_FROM)), window);
					break;

				case GAD_SELECT_PROT_CYCLE:
					data->bits_match = msg_copy.Code;
					function_select_disable(objlist, data, 4);
					break;

				case GAD_SELECT_COMPARE_CYCLE:
					data->compare_match = msg_copy.Code;
					function_select_disable(objlist, data, 8);
					break;

				// Include/Exclude
				case GAD_SELECT_INCLUDE_EXCLUDE:
					data->include = msg_copy.Code;
					break;

				// Entry type
				case GAD_SELECT_ENTRY_TYPE:
					data->entry_type = msg_copy.Code;
					break;

				// From date; activate to field
				case GAD_SELECT_DATE_FROM:
					ActivateStrGad(GADGET(GetObject(objlist, GAD_SELECT_DATE_TO)), window);
					break;

				// Simple
				case GAD_SELECT_SIMPLE:
					data->type = SELECT_SIMPLE;

				// Ok
				case GAD_SELECT_OK:

					// Store data
					strcpy(data->name, (char *)GetGadgetValue(objlist, GAD_SELECT_NAME_STRING));
					strcpy(data->date_from, (char *)GetGadgetValue(objlist, GAD_SELECT_DATE_FROM));
					strcpy(data->date_to, (char *)GetGadgetValue(objlist, GAD_SELECT_DATE_TO));
					data->compare = GetGadgetValue(objlist, GAD_SELECT_COMPARE_TYPE_CYCLE);

				// Cancel
				case GAD_SELECT_CANCEL:
					ret = gadget->GadgetID - GAD_SELECT_CANCEL;
					CloseConfigWindow(window);
					return ret;
				}
				break;
			}
		}

		Wait(1 << window->UserPort->mp_SigBit | 1 << ipc->command_port->mp_SigBit);
	}
}

// Fix gadget disabling
void function_select_disable(ObjectList *list, SelectData *data, short type)
{
	short a;

	if (type & 1)
		DisableObject(list, GAD_SELECT_NAME_STRING, (data->name_match == SELECT_MATCH_IGNORE));

	if (type & 2)
	{
		DisableObject(list, GAD_SELECT_DATE_FROM, (data->date_match == SELECT_MATCH_IGNORE));
		DisableObject(list, GAD_SELECT_DATE_TO, (data->date_match == SELECT_MATCH_IGNORE));
	}

	if (type & 4)
	{
		for (a = 0; a < 16; a++)
			DisableObject(list, GAD_SELECT_PROTECTION + a, (data->bits_match == SELECT_MATCH_IGNORE));
	}

	if (type & 8)
		DisableObject(list, GAD_SELECT_COMPARE_TYPE_CYCLE, (data->compare_match == SELECT_MATCH_IGNORE));
}

// Select a file (called from function process)
void function_select_file(FunctionHandle *handle, Lister *lister, char *filename)
{
	SelectData data;
	SelectWildData *packet;
	ULONG side;

	// Fill in selection data
	strcpy(data.name, filename);
	data.type = SELECT_SIMPLE | SELECTF_MAKE_VISIBLE | SELECTF_EXCLUSIVE;
	data.entry_type = SELECT_ENTRY_FILES;
	data.name_match = SELECT_MATCH_MATCH;
	data.date_match = SELECT_MATCH_IGNORE;
	data.bits_match = SELECT_MATCH_IGNORE;
	data.compare_match = SELECT_MATCH_IGNORE;
	data.include = SELECT_INCLUDE;

	// The file was read into the current source panel
	side = (handle->source_paths.current && handle->source_paths.current->lister == lister)
			   ? function_path_side(handle->source_paths.current)
			   : 0;

	// Select file
	if ((packet = AllocVec(sizeof(SelectWildData), MEMF_CLEAR)))
	{
		CopyMem(&data, &packet->data, sizeof(SelectData));
		packet->dest_list = &handle->dest_paths;
		packet->side = side;

		if (!IPC_Command(lister->ipc, LISTER_SELECT_WILD, 0, packet, packet, (struct MsgPort *)-1))
			FreeVec(packet);
	}
	else if (!side)
		IPC_Command(lister->ipc, LISTER_SELECT_WILD, (IPTR)&handle->dest_paths, &data, 0, (struct MsgPort *)-1);
}
