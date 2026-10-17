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

/*********************************************************************
 *
 * 	13.8.98	Modified gjp to provide extra functions for FTP module
 *
 *	mod_id	0 normal lister format
 *		1 extended
 *		2 ftp type with no reset
 *		3 ftp type with reset to ftp defaults button
 */
#include "listerformat.h"

#define CONFIG_SAVE (1 << 30)  // Save it

struct formats
{
	ListFormat *format_opus;
	ListFormat *format_ftp;
};

#define FlagIsSet(v, f) (((v) & (f)) != 0)
#define ClearFlag(v, f) ((v) &= ~(f))

ULONG LIBFUNC L_Module_Entry(REG(a0, struct List *files),
							 REG(a1, struct Screen *screen),
							 REG(a2, IPCData *ipc),
							 REG(a3, IPCData *main_ipc),
							 REG(d0, IPTR mod_id),
							 REG(d1, IPTR mod_data))
{
	config_lister_data *data;
	ListFormat *format;
	struct Window *window;
	ULONG success = 1;
	ObjectDef *obj1, *obj2;
	ConfigWindow *dims;
	struct formats *ftp_formats;

	// Get data
	format = (ListFormat *)files;
	window = (struct Window *)screen;

	// invalid call
	if (mod_id > 3)
		return 0;

	// Valid config?
	if (!format)
		return 0;

	// Allocate data
	if (!(data = AllocVec(sizeof(config_lister_data), MEMF_CLEAR)))
		return 0;

	// Copy the list format
	CopyMem((char *)format, (char *)&data->format, sizeof(ListFormat));

	// Store default pointer
	if ((data->def_format = (ListFormat *)mod_data))
		CopyMem((char *)data->def_format, (char *)&data->def_format_copy, sizeof(ListFormat));

	// save call type
	data->mod_id = mod_id;

	if (mod_id == 0)
	{
		dims = &listformat_window;
		obj1 = listformat_objects;
		obj2 = listformat_nosave_objects;
	}
	else if (mod_id == 1)
	{
		dims = &listformat_window_big;
		obj1 = listformat_objects;
		obj2 = listformat_save_objects;
	}
	else if (mod_id == 2)
	{  // ftp format lister no reset ftp button
		dims = &listformat_window_ftp;
		obj1 = listformat_ftp_objects;
		obj2 = listformat_ftp_use_objects;
		ftp_formats = (struct formats *)mod_data;

		// Store default pointers
		if ((data->def_format = (ListFormat *)ftp_formats->format_opus))
			CopyMem((char *)data->def_format, (char *)&data->def_format_copy, sizeof(ListFormatStorage));
		data->def_ftp_format = (ListFormatStorage *)ftp_formats->format_ftp;
	}
	else
	// ftp format lister large
	{
		dims = &listformat_window_ftp_big;
		obj1 = listformat_ftp_objects;
		obj2 = listformat_ftp_use_objects_big;
		ftp_formats = (struct formats *)mod_data;

		// Store default pointers
		if ((data->def_format = (ListFormat *)ftp_formats->format_opus))
			CopyMem((char *)data->def_format, (char *)&data->def_format_copy, sizeof(ListFormatStorage));
		data->def_ftp_format = (ListFormatStorage *)ftp_formats->format_ftp;
	}

	// Open window
	data->newwin.parent = window;
	data->newwin.dims = dims;
	data->newwin.title = GetString(locale, MSG_LISTER_FORMAT_TITLE);
	data->newwin.locale = locale;
	data->newwin.flags = WINDOW_VISITOR | WINDOW_AUTO_KEYS | WINDOW_REQ_FILL;
	if (!(data->window = OpenConfigWindow(&data->newwin)) || !(data->objlist = AddObjectList(data->window, obj1)))
	{
		_config_lister_cleanup(data);
		return 0;
	}

	// Save/No save gadgets
	AddObjectList(data->window, obj2);

	// Store data pointer in window
	DATA(data->window)->data = (IPTR)data;
	data->sel_lister = GetObject(data->objlist, GAD_LISTER_FORMAT_SELITEMS);

	// Initialise
	_config_lister_init_format(data);

	// Event loop
	FOREVER
	{
		struct IntuiMessage *msg;
		int quit_flag = 0;

		// IPC?
		if (ipc)
		{
			IPCMessage *msg;

			// Any IPC messages?
			while ((msg = (IPCMessage *)GetMsg(ipc->command_port)))
			{
				// Activate?
				if (msg->command == IPC_ACTIVATE)
				{
					MoveWindowInFrontOf(data->window, window);
					ActivateWindow(data->window);
				}

				// Quit
				else if (msg->command == IPC_QUIT)
				{
					quit_flag = 1;
					success = 0;
				}

				// Reply message
				IPC_Reply(msg);
			}
		}

		// Any messages to the window?
		while ((msg = GetWindowMsg(data->window->UserPort)))
		{
			struct IntuiMessage msg_copy;
			UWORD gadgetid;

			// Copy message and reply
			msg_copy = *msg;
			ReplyWindowMsg(msg);

			// Look at message
			switch (msg_copy.Class)
			{
			// Close window
			case IDCMP_CLOSEWINDOW:
				quit_flag = 1;
				success = 0;
				break;

			// BOOPSI message
			case IDCMP_IDCMPUPDATE: {
				struct TagItem *tags = (struct TagItem *)msg_copy.IAddress;
				ULONG id;

				// Lister?
				id = GetTagData(GA_ID, 0, tags);
				if (id == GAD_LISTER_FORMAT_SELITEMS || id == GAD_LISTER_FORMAT_ITEMS)
				{
					short item;
					Att_List *list;

					// Get list
					list = (id == GAD_LISTER_FORMAT_SELITEMS) ? data->item_sel_list : data->item_list;
					data->drag_from = id;

					// Get item
					if ((item = GetTagData(DLV_DragNotify, -1, tags)) != -1 &&
						(data->drag_node = Att_FindNode(list, item)))
					{
						// Allocate drag info
						if ((data->drag = GetDragInfo(
								 data->window,
								 data->window->RPort,
								 GetTagData(GA_Width, 0, tags) - ((id == GAD_LISTER_FORMAT_SELITEMS) ? 21 : 0),
								 GetTagData(GA_Height, 0, tags),
								 DRAGF_NEED_GELS | DRAGF_CUSTOM)))
						{
							short len;
							struct TextExtent extent;

							// Set pens and font
							SetAPen(&data->drag->drag_rp, DRAWINFO(data->window)->dri_Pens[FILLTEXTPEN]);
							SetDrMd(&data->drag->drag_rp, JAM1);
							SetRast(&data->drag->drag_rp, DRAWINFO(data->window)->dri_Pens[FILLPEN]);
							SetFont(&data->drag->drag_rp, data->window->RPort->Font);

							// Get length that will fit
							len = TextFit(data->window->RPort,
										  data->drag_node->node.ln_Name,
										  strlen(data->drag_node->node.ln_Name),
										  &extent,
										  0,
										  1,
										  data->drag->width - 2,
										  data->drag->height);

							// Draw text
							Move(&data->drag->drag_rp, 2, data->drag->drag_rp.TxBaseline);
							Text(&data->drag->drag_rp, data->drag_node->node.ln_Name, len);

							// Build the drag shadow mask
							data->drag->flags |= DRAGF_OPAQUE | DRAGF_NO_LOCK;
							GetDragMask(data->drag);

							// Get drag offsets
							data->drag_x = -GetTagData(GA_RelRight, data->drag->width >> 1, tags);
							if (id == GAD_LISTER_FORMAT_SELITEMS)
								data->drag_x += 21;
							data->drag_y = -GetTagData(GA_RelBottom, data->drag->height >> 1, tags);

							// Show drag image
							ShowDragImage(data->drag, msg_copy.MouseX + data->drag_x, msg_copy.MouseY + data->drag_y);

							// Set mouse move reporting
							data->window->Flags |= WFLG_REPORTMOUSE;
						}
					}
				}
			}
			break;

			// Mouse move
			case IDCMP_MOUSEMOVE:

				// Valid drag info?
				if (data->drag)
				{
					// Show drag image
					ShowDragImage(data->drag, data->window->MouseX + data->drag_x, data->window->MouseY + data->drag_y);
				}
				break;

			// Gadget
			case IDCMP_GADGETUP:
			case IDCMP_GADGETDOWN:

				// Get GadgetID
				gadgetid = ((struct Gadget *)msg_copy.IAddress)->GadgetID;

				// Look at gadget
				switch (gadgetid)
				{
				// Cancel
				case GAD_LISTER_CANCEL:
					success = 0;

				// Use
				case GAD_LISTER_USE:
					quit_flag = 1;
					break;

				// Save
				case GAD_LISTER_SAVE:
					quit_flag = 1;
					success |= CONFIG_SAVE;
					break;

				// Default
				case GAD_DEFAULTS:
					listerformat_defaults(data);
					break;

				// Set as default
				case GAD_SET_AS_DEFAULTS:
					listerformat_setas_defaults(data);
					break;

				// ftp defaults
				case GAD_FTP_DEFAULTS:
					listerformat_ftp_defaults(data);
					break;

				// Available items
				case GAD_LISTER_FORMAT_ITEMS: {
					Att_Node *node, *newnode;

					// Get selected item
					if ((node = Att_FindNode(data->item_list, msg_copy.Code)))
					{
						struct TagItem tags[2];

						// Remove lists
						SetGadgetChoices(data->objlist, GAD_LISTER_FORMAT_ITEMS, (APTR)~0);
						SetGadgetChoices(data->objlist, GAD_LISTER_FORMAT_SELITEMS, (APTR)~0);

						// Add entry to selected list
						if ((newnode = Att_NewNode(data->item_sel_list, node->node.ln_Name, node->data, 0)))
						{
							// Remove node from available list
							Att_RemNode(node);
						}

						// Reattach lists
						SetGadgetChoices(data->objlist, GAD_LISTER_FORMAT_ITEMS, data->item_list);
						SetGadgetChoices(data->objlist, GAD_LISTER_FORMAT_SELITEMS, data->item_sel_list);

						// Move selected list to the end
						tags[0].ti_Tag = DLV_Top;
						tags[0].ti_Data = 100;
						tags[1].ti_Tag = TAG_END;
						SetGadgetAttrsA(GADGET(data->sel_lister), data->window, 0, tags);

						// Check 'defaults' gadget
						listerformat_check_default(data);
					}
				}
				break;

				// Something else that would change 'default' state
				case GAD_LISTER_FORMAT_SELITEMS:
				case GAD_LISTER_FORMAT_SEPARATION:
				case GAD_LISTER_FORMAT_REVERSE:
				case GAD_LISTER_REJECT_ICONS:
				case GAD_LISTER_HIDDEN_BIT:
				case GAD_LISTER_FUEL_GAUGE:
				case GAD_LISTER_SHOW:
				case GAD_LISTER_HIDE:

					// Check 'defaults' gadget
					listerformat_check_default(data);
					break;
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
						data->drag_x = data->window->MouseX;
						data->drag_y = data->window->MouseY;
						listerformat_end_drag(data, 1);

						// Check 'defaults' gadget
						listerformat_check_default(data);
					}

					// Aborted
					else if (msg_copy.Code == MENUDOWN)
					{
						// End drag
						listerformat_end_drag(data, 0);
					}
				}
				break;

			// Key press
			case IDCMP_RAWKEY:

				// Help?
				if (msg_copy.Code == 0x5f && !(msg_copy.Qualifier & VALID_QUALIFIERS))
				{
					// Valid main IPC?
					if (main_ipc)
					{
						// Set busy pointer
						SetWindowBusy(data->window);

						// Send help request
						IPC_Command(main_ipc, IPC_HELP, (1 << 31), "Lister Format", 0, (struct MsgPort *)-1);

						// Clear busy pointer
						ClearWindowBusy(data->window);
					}
				}
				break;
			}

			// Check quit flag
			if (quit_flag)
				break;
		}

		// Check quit flag
		if (quit_flag)
			break;

		// Wait for input
		if ((Wait(1 << data->window->UserPort->mp_SigBit | ((ipc) ? 1 << ipc->command_port->mp_SigBit : 0) |
				  IPCSIG_QUIT)) &
			IPCSIG_QUIT)
			break;
	}

	// End any drag in progress
	listerformat_end_drag(data, 0);

	// Copy format back if successful
	if (success)
	{
		// Get final values
		_config_lister_get_values(data);

		// See what's changed
		success |= CompareListFormat(&data->format, format);

		// Copy format back
		CopyMem((char *)&data->format, (char *)format, sizeof(ListFormat));

		// Copy default format back
		if (data->def_format)
			CopyMem((char *)&data->def_format_copy, (char *)data->def_format, sizeof(ListFormat));
	}

	// Free stuff
	_config_lister_cleanup(data);
	return success;
}

static char item_lookup[DISPLAY_LAST] = {DISPLAY_NETPROT,
										 DISPLAY_GROUP,
										 DISPLAY_OWNER,
										 DISPLAY_VERSION,
										 DISPLAY_FILETYPE,
										 DISPLAY_COMMENT,
										 DISPLAY_PROTECT,
										 DISPLAY_DATE,
										 DISPLAY_SIZE,
										 DISPLAY_NAME};

// Initialise list format gadgets
void _config_lister_init_format(config_lister_data *data)
{
	short a;
	char *sort_ptr = 0;
	static short items[DISPLAY_LAST] = {MSG_LISTER_LIST_FILENAME,
										MSG_LISTER_LIST_FILESIZE,
										MSG_LISTER_LIST_DATE,
										MSG_LISTER_LIST_PROTECTION,
										MSG_LISTER_LIST_COMMENT,
										MSG_LISTER_LIST_FILETYPE,
										MSG_LISTER_LIST_VERSION,
										MSG_LISTER_LIST_OWNER,
										MSG_LISTER_LIST_GROUP,
										MSG_LISTER_LIST_NETPROTECTION};

	// Build available item list
	if (data->item_list || (data->item_list = Att_NewList(0)))
	{
		for (a = 0; a < DISPLAY_LAST; a++)
			Att_NewNode(data->item_list, GetString(locale, items[a]), 9 - a, ADDNODE_PRI);
	}

	// Build selected item list
	if (!data->item_sel_list)
		data->item_sel_list = Att_NewList(0);
	for (a = 0; a < 16 && data->format.display_pos[a] != -1; a++)
	{
		short b;
		Att_Node *node;

		// Find selected item
		for (b = 0; b < DISPLAY_LAST; b++)
			if (item_lookup[b] == data->format.display_pos[a])
				break;

		// Find node in source list
		if ((node = Att_FindNodeData(data->item_list, b)))
		{
			Att_Node *new_node;

			// Add to selected list
			if ((new_node = Att_NewNode(data->item_sel_list, node->node.ln_Name, node->data, 0)))
			{
				// See if this is the sort method
				if (data->format.sort.sort == data->format.display_pos[a])
					sort_ptr = new_node->node.ln_Name;

				// Remove from available item list
				Att_RemNode(node);
			}
		}
	}

	// Attach lists
	SetGadgetChoices(data->objlist, GAD_LISTER_FORMAT_ITEMS, data->item_list);
	SetGadgetChoices(data->objlist, GAD_LISTER_FORMAT_SELITEMS, data->item_sel_list);
	SetGadgetValue(data->objlist, GAD_LISTER_FORMAT_SELITEMS, Att_NodeNumber(data->item_sel_list, sort_ptr));

	// Set separate method
	SetGadgetValue(data->objlist, GAD_LISTER_FORMAT_SEPARATION, data->format.sort.separation);

	// Reverse sort flag
	SetGadgetValue(data->objlist, GAD_LISTER_FORMAT_REVERSE, data->format.sort.sort_flags & SORT_REVERSE);

	// Filter icons/hidden bit
	SetGadgetValue(data->objlist, GAD_LISTER_REJECT_ICONS, data->format.flags & LFORMATF_REJECT_ICONS);
	SetGadgetValue(data->objlist, GAD_LISTER_HIDDEN_BIT, data->format.flags & LFORMATF_HIDDEN_BIT);

	if (data->mod_id < 2)
	{
		// Fuel gauge/inherit
		SetGadgetValue(data->objlist, GAD_LISTER_FUEL_GAUGE, data->format.flags & LFORMATF_GAUGE);
		SetGadgetValue(data->objlist, GAD_LISTER_INHERIT, data->format.flags & LFORMATF_INHERIT);
	}
	// Show/hide pattern
	SetGadgetValue(data->objlist, GAD_LISTER_SHOW, (IPTR)data->format.show_pattern);
	SetGadgetValue(data->objlist, GAD_LISTER_HIDE, (IPTR)data->format.hide_pattern);

	// Check 'defaults' gadget
	listerformat_check_default(data);
}

void _config_lister_cleanup(config_lister_data *data)
{
	if (data)
	{
		// Close window
		CloseConfigWindow(data->window);

		// Free lists
		Att_RemList(data->item_list, 0);
		Att_RemList(data->item_sel_list, 0);

		// Free data
		FreeVec(data);
	}
}

void _config_lister_get_values(config_lister_data *data)
{
	listformat_get_values(data, &data->format);
}

void listformat_get_values(config_lister_data *data, ListFormat *format)
{
	Att_Node *node;
	short num;

	// Get list format
	for (node = (Att_Node *)data->item_sel_list->list.lh_Head, num = 0; num < 16; num++)
	{
		// Valid node?
		if (node->node.ln_Succ)
		{
			format->display_pos[num] = item_lookup[node->data];
			node = (Att_Node *)node->node.ln_Succ;
		}
		else
			format->display_pos[num] = -1;
	}

	// Sort method
	if ((node = Att_FindNode(data->item_sel_list, GetGadgetValue(data->objlist, GAD_LISTER_FORMAT_SELITEMS))))
		format->sort.sort = item_lookup[node->data];

	// Separation
	format->sort.separation = GetGadgetValue(data->objlist, GAD_LISTER_FORMAT_SEPARATION);

	// Reverse sort
	format->sort.sort_flags = 0;
	if (GetGadgetValue(data->objlist, GAD_LISTER_FORMAT_REVERSE))
		format->sort.sort_flags |= SORT_REVERSE;
	else
		format->sort.sort_flags &= ~SORT_REVERSE;

	// Flags
	format->flags = 0;
	if (GetGadgetValue(data->objlist, GAD_LISTER_REJECT_ICONS))
		format->flags |= LFORMATF_REJECT_ICONS;
	if (GetGadgetValue(data->objlist, GAD_LISTER_HIDDEN_BIT))
		format->flags |= LFORMATF_HIDDEN_BIT;

	if (data->mod_id < 2)
	{
		if (GetGadgetValue(data->objlist, GAD_LISTER_FUEL_GAUGE))
			format->flags |= LFORMATF_GAUGE;
		if (GetGadgetValue(data->objlist, GAD_LISTER_INHERIT))
			format->flags |= LFORMATF_INHERIT;
	}

	// Show/hide
	strcpy(format->show_pattern, (char *)GetGadgetValue(data->objlist, GAD_LISTER_SHOW));
	strcpy(format->hide_pattern, (char *)GetGadgetValue(data->objlist, GAD_LISTER_HIDE));
	ParsePatternNoCase(format->show_pattern, format->show_pattern_p, 80);
	ParsePatternNoCase(format->hide_pattern, format->hide_pattern_p, 80);
}

// End drag
void listerformat_end_drag(config_lister_data *data, BOOL ok)
{
	// Valid drag?
	if (data->drag)
	{
		// Free drag stuff
		FreeDragInfo(data->drag);
		data->drag = 0;

		// Clear flags
		if (data->window)
			data->window->Flags &= ~WFLG_REPORTMOUSE;

		// Valid drop?
		if (ok)
		{
			short id;
			GL_Object *object;
			Att_Node *old_sort;
			short new_sort;

			// Get old sort method
			old_sort = Att_FindNode(data->item_sel_list, GetGadgetValue(data->objlist, GAD_LISTER_FORMAT_SELITEMS));

			// Get id of opposite (destination) lister
			id = (data->drag_from == GAD_LISTER_FORMAT_SELITEMS) ? GAD_LISTER_FORMAT_ITEMS : GAD_LISTER_FORMAT_SELITEMS;

			// See if we dropped within it
			if ((object = GetObject(data->objlist, id)) && CheckObjectArea(object, data->drag_x, data->drag_y))
			{
				// Remove lists
				SetGadgetChoices(data->objlist, GAD_LISTER_FORMAT_ITEMS, (APTR)~0);
				SetGadgetChoices(data->objlist, GAD_LISTER_FORMAT_SELITEMS, (APTR)~0);

				// Adding to selected list?
				if (id == GAD_LISTER_FORMAT_SELITEMS)
				{
					Att_Node *node;

					// Add entry to selected list
					if ((node =
							 Att_NewNode(data->item_sel_list, data->drag_node->node.ln_Name, data->drag_node->data, 0)))
					{
						// Position in list
						listerformat_position(data, node);

						// Remove node from available list
						Att_RemNode(data->drag_node);
					}
				}

				// Removing from selected list
				else
				{
					// Add back to available list
					Att_NewNode(data->item_list, data->drag_node->node.ln_Name, data->drag_node->data, ADDNODE_PRI);

					// Clear selected item
					Att_RemNode(data->drag_node);
				}

				// Reattach lists
				SetGadgetChoices(data->objlist, GAD_LISTER_FORMAT_ITEMS, data->item_list);
				SetGadgetChoices(data->objlist, GAD_LISTER_FORMAT_SELITEMS, data->item_sel_list);
			}

			// Or, see if we're repositioning in the selection list
			else if (data->drag_from == GAD_LISTER_FORMAT_SELITEMS &&
					 (CheckObjectArea(data->sel_lister, data->drag_x, data->drag_y)))
			{
				// Remove list
				SetGadgetChoices(data->objlist, GAD_LISTER_FORMAT_SELITEMS, (APTR)~0);

				// Position in list
				listerformat_position(data, data->drag_node);

				// Reattach lists
				SetGadgetChoices(data->objlist, GAD_LISTER_FORMAT_SELITEMS, data->item_sel_list);
			}

			// Nothing happening
			else
				return;

			// Get new sort position
			new_sort = Att_FindNodeNumber(data->item_sel_list, old_sort);

			// Set new sort selection
			SetGadgetValue(data->objlist, GAD_LISTER_FORMAT_SELITEMS, (new_sort == -1) ? 0 : new_sort);
		}
	}
}

// Position an entry in the selected list
void listerformat_position(config_lister_data *data, Att_Node *node)
{
	IPTR top;
	ULONG item;
	Att_Node *pos;

	// Get top item (DLV_Top stores an IPTR)
	GetAttr(DLV_Top, (Object *)GADGET(data->sel_lister), &top);

	// Get item we dropped over
	item = (data->drag_y - data->sel_lister->dims.Top) / data->window->RPort->TxHeight;

	// Get node to position in front of
	pos = Att_FindNode(data->item_sel_list, item + top);

	// Dropped over same item?
	if (pos == node)
		return;

	// Dropped over next item?
	if (pos == (Att_Node *)node->node.ln_Succ)
	{
		if (node->node.ln_Succ)
			pos = (Att_Node *)node->node.ln_Succ->ln_Succ;
	}

	// Valid node?
	if (pos)
		Att_PosNode(data->item_sel_list, node, pos);

	// Add to end of list
	else
	{
		Remove((struct Node *)node);
		AddTail((struct List *)data->item_sel_list, (struct Node *)node);
	}
}

// Do defaults
void listerformat_defaults(config_lister_data *data)
{
	// No defaults?
	if (data->def_format)
	{
		// Copy into settings
		CopyMem((char *)&data->def_format_copy, (char *)&data->format, sizeof(ListFormat));

		// Clear lists
		SetGadgetChoices(data->objlist, GAD_LISTER_FORMAT_ITEMS, (APTR)~0);
		SetGadgetChoices(data->objlist, GAD_LISTER_FORMAT_SELITEMS, (APTR)~0);

		// Clear out lists
		Att_RemList(data->item_list, REMLIST_SAVELIST);
		Att_RemList(data->item_sel_list, REMLIST_SAVELIST);

		// Initialise gadgets
		_config_lister_init_format(data);
	}
}

// Do defaults
void listerformat_ftp_defaults(config_lister_data *data)
{
	// No defaults?
	if (data->def_ftp_format && data->mod_id == 3)
	{
		// Copy into settings
		CopyMem((char *)data->def_ftp_format, (char *)&data->format, sizeof(ListFormatStorage));

		// Clear lists
		SetGadgetChoices(data->objlist, GAD_LISTER_FORMAT_ITEMS, (APTR)~0);
		SetGadgetChoices(data->objlist, GAD_LISTER_FORMAT_SELITEMS, (APTR)~0);

		// Clear out lists
		Att_RemList(data->item_list, REMLIST_SAVELIST);
		Att_RemList(data->item_sel_list, REMLIST_SAVELIST);

		// Initialise gadgets
		_config_lister_init_format(data);
	}
}

// Set as defaults
void listerformat_setas_defaults(config_lister_data *data)
{
	// No defaults?
	if (data->def_format)
	{
		ListFormat format;

		// listerformat_get_values() doesn't set file/dir colors
		// Copy file/dir colors into format so we don't lose them
		format.files_unsel[0] = data->def_format_copy.files_unsel[0];
		format.files_unsel[1] = data->def_format_copy.files_unsel[1];
		format.files_sel[0] = data->def_format_copy.files_sel[0];
		format.files_sel[1] = data->def_format_copy.files_sel[1];
		format.dirs_unsel[0] = data->def_format_copy.dirs_unsel[0];
		format.dirs_unsel[1] = data->def_format_copy.dirs_unsel[1];
		format.dirs_sel[0] = data->def_format_copy.dirs_sel[0];
		format.dirs_sel[1] = data->def_format_copy.dirs_sel[1];

		// Get format
		listformat_get_values(data, &format);

		// Copy settings into default format
		CopyMem((char *)&format, (char *)&data->def_format_copy, sizeof(ListFormat));

		// Check 'defaults' gadget
		listerformat_check_default(data);
	}
}

// Check state of defaults gadget
void listerformat_check_default(config_lister_data *data)
{
	ListFormat format;
	BOOL state = 1;

	// Get format
	listformat_get_values(data, &format);

	// Got default && Different?
	if (data->def_format)
	{
		if (data->mod_id == 3)
		{
			// adjust for FTP not having these gadgets

			if (FlagIsSet(data->def_format_copy.flags, LFORMATF_GAUGE))
				format.flags |= LFORMATF_GAUGE;

			if (FlagIsSet(data->def_format_copy.flags, LFORMATF_INHERIT))
				format.flags |= LFORMATF_INHERIT;
		}

		if (CompareListFormat((ListFormat *)&data->def_format_copy, &format))
			state = 0;
	}

	// Set gadget state
	DisableObject(data->objlist, GAD_DEFAULTS, state);
	DisableObject(data->objlist, GAD_SET_AS_DEFAULTS, state);

	// FTP ?
	if (data->mod_id == 3)
	{
		state = 1;

		// Got ftp default and different
		if (data->def_ftp_format)
		{
			// adjust for FTP not having these gadgets

			ClearFlag(format.flags, (LFORMATF_GAUGE | LFORMATF_INHERIT));

			if (FlagIsSet(data->def_ftp_format->flags, LFORMATF_GAUGE))
				format.flags |= LFORMATF_GAUGE;

			if (FlagIsSet(data->def_ftp_format->flags, LFORMATF_INHERIT))
				format.flags |= LFORMATF_INHERIT;

			if (CompareListFormat((ListFormat *)data->def_ftp_format, &format))
				state = 0;
		}

		// Set gadget state
		DisableObject(data->objlist, GAD_FTP_DEFAULTS, state);
	}
}
