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

#include "diskcopy.h"

int LIBFUNC L_Module_Entry(REG(a0, struct List *disks),
						   REG(a1, struct Screen *screen),
						   REG(a2, IPCData *ipc),
						   REG(a3, IPCData *main_ipc),
						   REG(d0, IPTR mod_id),
						   REG(d1, IPTR mod_data))
{
	diskcopy_data *data;
	short ret = 1;
	BOOL auto_sel = 0;

	// Allocate data
	if (!(data = AllocVec(sizeof(diskcopy_data), MEMF_CLEAR)))
		return 0;

	// Store IPC pointer
	data->ipc = ipc;
	data->screen = screen;

	// Create a message port for destinations
	if (!(data->dest_port = CreateMsgPort()))
	{
		FreeVec(data);
		return 0;
	}

	// Get abort bit
	data->abort_bit = AllocSignal(-1);

	// Initialise defaults
	data->default_verify = 1;
	data->default_bump = 0;

	// Disks supplied?
	if (disks && !(IsListEmpty(disks)))
	{
		// Build source list
		data->source_list = get_device_list(disks->lh_Head->ln_Name, 0);
		auto_sel = 1;
	}

	// Otherwise, build source list
	else
		data->source_list = get_device_list(0, 0);

	// No source?
	if (!data->source_list || IsListEmpty((struct List *)data->source_list))
	{
		if (screen)
			DisplayBeep(screen);
		diskcopy_free(data);
		return 0;
	}

	// Source supplied?
	if (auto_sel)
	{
		struct Node *node;
		short count = 0;

		// Show device information
		show_device_info(data, -1);

		// Go through supplied disks
		for (node = disks->lh_Head->ln_Succ; node->ln_Succ; node = node->ln_Succ)
		{
			// Find in destination list
			if (FindName((struct List *)data->dest_list, node->ln_Name))
			{
				count = 1;
				break;
			}
		}

		// Didn't get any destinations?
		if (count == 0 && disks->lh_Head->ln_Succ->ln_Succ)
		{
			// Beep and display error
			DisplayBeep(screen);
			/*SimpleRequestScreenTags(
				screen,
				GetString(locale,MSG_DISKCOPY_TITLE),
				GetString(locale,MSG_OK),
				GetString(locale,MSG_INCOMPATIBLE));*/
			SimpleRequest((struct Window *)screen,
						  GetString(locale, MSG_DISKCOPY_TITLE),
						  GetString(locale, MSG_OK),
						  GetString(locale, MSG_INCOMPATIBLE),
						  NULL,
						  NULL,
						  0,
						  SRF_SCREEN_PARENT);

			// Return
			diskcopy_free(data);
			return 0;
		}
	}

	// Open window
	if (!(diskcopy_open(data, 0)))
	{
		diskcopy_free(data);
		return 0;
	}

	// Source supplied?
	if (auto_sel)
	{
		struct Node *node;

		// Select first source
		SetGadgetValue(data->list, GAD_DISKCOPY_SOURCE, 0);

		// Show device information
		show_device_info(data, -1);

		// Detach destination list
		SetGadgetChoices(data->list, GAD_DISKCOPY_DEST, (APTR)~0);

		// Go through supplied disks
		for (node = disks->lh_Head->ln_Succ; node->ln_Succ; node = node->ln_Succ)
		{
			Att_Node *dest;

			// Find in destination list
			if ((dest = (Att_Node *)FindName((struct List *)data->dest_list, node->ln_Name)))
			{
				// Select this entry
				dest->node.lve_Flags |= LVEF_SELECTED;
			}
		}

		// Attach destination list
		SetGadgetChoices(data->list, GAD_DISKCOPY_DEST, data->dest_list);

		// Enable Diskcopy button
		DisableObject(data->list, GAD_DISKCOPY_DISKCOPY, FALSE);
	}

	// Event loop
	FOREVER
	{
		struct IntuiMessage *msg;
		IPCMessage *imsg;
		BOOL quit_flag = 0;

		// IPC messages?
		if (data->ipc)
		{
			while ((imsg = (IPCMessage *)GetMsg(data->ipc->command_port)))
			{
				// Abort?
				if (imsg->command == IPC_ABORT || imsg->command == IPC_QUIT)
				{
					quit_flag = 1;
					ret = 0;
				}
				IPC_Reply(imsg);
			}
		}

		// Intuition messages
		if (data->window)
		{
			while ((msg = GetWindowMsg(data->window->UserPort)))
			{
				struct IntuiMessage msg_copy;

				// Copy message and reply
				msg_copy = *msg;
				ReplyWindowMsg(msg);

				switch (msg_copy.Class)
				{
				// Close window
				case IDCMP_CLOSEWINDOW:
					quit_flag = 1;
					break;

				// Gadget
				case IDCMP_GADGETUP:
					switch (((struct Gadget *)msg_copy.IAddress)->GadgetID)
					{
					// Cancel
					case GAD_DISKCOPY_CANCEL:
						quit_flag = 1;
						break;

					// Source selected
					case GAD_DISKCOPY_SOURCE:

						// Show device information
						show_device_info(data, 1);
						break;

					// Destination selected
					case GAD_DISKCOPY_DEST: {
						Att_Node *node = 0;

						if (data->dest_list)
						{
							// Go through destination list
							for (node = (Att_Node *)data->dest_list->list.lh_Head; node->node.ln_Succ;
								 node = (Att_Node *)node->node.ln_Succ)
							{
								// Selected?
								if (node->node.lve_Flags & LVEF_SELECTED)
									break;
							}
						}

						// Disable diskcopy button if no destination selected
						DisableObject(data->list, GAD_DISKCOPY_DISKCOPY, (!node || !node->node.ln_Succ));
					}
					break;

					// Do diskcopy
					case GAD_DISKCOPY_DISKCOPY:

						// Update source info
						show_device_info(data, 0);

						// Run the diskcopy routine
						if (!(start_diskcopy(data)))
							quit_flag = 1;
						break;
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
							IPC_Command(main_ipc, IPC_HELP, (1 << 31), "DiskCopy", 0, (struct MsgPort *)-1);

							// Clear busy pointer
							ClearWindowBusy(data->window);
						}
					}
					break;
				}
			}
		}

		if (quit_flag)
			break;

		Wait(((data->window) ? (1 << data->window->UserPort->mp_SigBit) : 0) |
			 ((data->ipc) ? (1 << data->ipc->command_port->mp_SigBit) : 0));
	}

	// Free stuff
	diskcopy_free(data);
	return ret;
}

// Open diskcopy window
BOOL diskcopy_open(diskcopy_data *data, BOOL noactive)
{
	// Fill out new window
	data->new_win.parent = data->screen;
	data->new_win.dims = &data->win_dims;
	data->new_win.title = GetString(locale, MSG_DISKCOPY_TITLE);
	data->new_win.locale = locale;
	data->new_win.flags = WINDOW_SCREEN_PARENT | WINDOW_VISITOR | WINDOW_AUTO_KEYS | WINDOW_REQ_FILL;
	if (noactive)
		data->new_win.flags |= WINDOW_NO_ACTIVATE;

	// Default dimensions
	data->win_dims = diskcopy_window;

	// Stored position valid?
	if (data->pos_valid)
	{
		data->win_dims.char_dim.Left = 0;
		data->win_dims.char_dim.Top = 0;
		data->win_dims.fine_dim.Left = data->window_pos.x;
		data->win_dims.fine_dim.Top = data->window_pos.y;
	}

	// Open window
	if (!(data->window = OpenConfigWindow(&data->new_win)) ||
		!(data->list = AddObjectList(data->window, diskcopy_objects)))
	{
		return 0;
	}

	// Attach source list
	SetGadgetChoices(data->list, GAD_DISKCOPY_SOURCE, data->source_list);

	// Set defaults
	SetGadgetValue(data->list, GAD_DISKCOPY_VERIFY, data->default_verify);
	SetGadgetValue(data->list, GAD_DISKCOPY_BUMP, data->default_bump);

	// Disable Diskcopy button initially
	DisableObject(data->list, GAD_DISKCOPY_DISKCOPY, TRUE);
	return 1;
}

// Close diskcopy display
void diskcopy_close(diskcopy_data *data)
{
	// Window open?
	if (data->window)
	{
		// Store position
		data->window_pos = *((Point *)&data->window->LeftEdge);
		data->pos_valid = 1;

		// Store settings
		data->default_verify = GetGadgetValue(data->list, GAD_DISKCOPY_VERIFY);
		data->default_bump = GetGadgetValue(data->list, GAD_DISKCOPY_BUMP);

		// Close window
		CloseConfigWindow(data->window);
		data->window = 0;
	}
}

// Free diskcopy data
void diskcopy_free(diskcopy_data *data)
{
	if (data)
	{
		// Close window
		CloseConfigWindow(data->window);
		data->window = 0;

		// Free device lists
		Att_RemList(data->source_list, 0);

		// Delete port
		DeleteMsgPort(data->dest_port);

		// Free abort bit
		if (data->abort_bit > -1)
			FreeSignal(data->abort_bit);

		// Free data
		FreeVec(data);
	}
}

// Build device list
Att_List *get_device_list(char *get_only, char *like_device)
{
	struct DosList *dl, *like;
	struct DosEnvec *like_geo = 0;
	Att_List *list;

	// Create a list
	if (!(list = Att_NewList(0)))
		return 0;

	// Lock dos list
	dl = LockDosList(LDF_DEVICES | LDF_READ);

	// "Like-device" to look for?
	if (like_device && (like = FindDosEntry(dl, like_device, LDF_DEVICES)))
	{
		like_geo = (struct DosEnvec *)BADDR(
			((struct FileSysStartupMsg *)BADDR(like->dol_misc.dol_handler.dol_Startup))->fssm_Environ);
	}

	// Scan device list
	while ((dl = NextDosEntry(dl, LDF_DEVICES)))
	{
		// Is it a valid device?
		if (dl->dol_Task && (IPTR)dl->dol_misc.dol_handler.dol_Startup > 512)
		{
			struct DosEnvec *geo;
			char devname[32];

			// Get disk geometry
			geo = (struct DosEnvec *)BADDR(
				((struct FileSysStartupMsg *)BADDR(dl->dol_misc.dol_handler.dol_Startup))->fssm_Environ);

			// Don't have to test?
			if (!like_geo ||

				// Do have to test to see if it's like another
				(geo->de_SizeBlock == like_geo->de_SizeBlock && geo->de_Surfaces == like_geo->de_Surfaces &&
				 geo->de_BlocksPerTrack == like_geo->de_BlocksPerTrack &&
				 geo->de_HighCyl + geo->de_LowCyl == like_geo->de_HighCyl + like_geo->de_LowCyl))
			{
				// Convert name
				lsprintf(devname, "%b:", dl->dol_Name);

				// Is it ok?
				if (!get_only || stricmp(get_only, devname) == 0)
				{
					// Add to list
					Att_NewNode(list, devname, 0, ADDNODE_SORT);
				}
			}
		}
	}

	// Unlock dos list
	UnLockDosList(LDF_DEVICES | LDF_READ);

	return list;
}

// Show information on source device
void show_device_info(diskcopy_data *data, short list)
{
	Att_Node *node;
	BPTR lock;
	char info_buf[80], size_buf[27], *ptr;
	BOOL ok = 1;

	// Window not open?
	if (!data->window)
		ok = 0;

	// Get selected node
	if ((node = Att_FindNode(data->source_list, (ok) ? GetGadgetValue(data->list, GAD_DISKCOPY_SOURCE) : 0)) &&
		(lock = Lock(node->node.ln_Name, ACCESS_READ)))
	{
		// Get disk information
		Info(lock, &data->info);
		UnLock(lock);

		// Get space used
#ifdef USE_64BIT
		{
			UQUAD tmp = (UQUAD)data->info.id_BytesPerBlock * (UQUAD)data->info.id_NumBlocksUsed;
			BytesToString64(&tmp, size_buf, sizeof(size_buf), 1, 0);
		}
#else
		BytesToString(data->info.id_BytesPerBlock * data->info.id_NumBlocksUsed, size_buf, 1, 0);
#endif

		// Get disk name
		lsprintf(data->disk_name, "%b", ((struct DosList *)BADDR(data->info.id_VolumeNode))->dol_Name);

		// Build info string
		lsprintf(info_buf, GetString(locale, MSG_DISKCOPY_STATUS), (IPTR)data->disk_name, (IPTR)size_buf);
	}
	else
		info_buf[0] = 0;

	// Update status, etc?
	if (list)
	{
		BOOL state = 1;

		// Display status
		if (ok)
			SetGadgetValue(data->list, GAD_DISKCOPY_STATUS, (IPTR)info_buf);

		// Free existing destination list
		Att_RemList(data->dest_list, 0);
		data->dest_list = 0;

		// Did we have a valid node?
		if (node)
		{
			// Get device name without a colon
			strcpy(info_buf, node->node.ln_Name);
			if ((ptr = strchr(info_buf, ':')))
				*ptr = 0;

			// Build new destination list
			data->dest_list = get_device_list(0, info_buf);

			// Valid destination list?
			if (list == 1 && !(IsListEmpty((struct List *)data->dest_list)))
			{
				Att_Node *test;

				// Find first destination that's different to the source
				for (test = (Att_Node *)data->dest_list->list.lh_Head; test->node.ln_Succ;
					 test = (Att_Node *)test->node.ln_Succ)
				{
					// Different?
					if (strcmp(node->node.ln_Name, test->node.ln_Name))
					{
						// Select this node
						test->node.lve_Flags |= LVEF_SELECTED;
						break;
					}
				}

				// If we didn't find anything, select first entry
				if (!test->node.ln_Succ)
				{
					data->dest_list->list.lh_Head->lve_Flags |= LVEF_SELECTED;
				}

				// Enable "Diskcopy" button
				state = 0;
			}
		}

		// Window open?
		if (ok)
		{
			// Attach destination list
			SetGadgetChoices(data->list, GAD_DISKCOPY_DEST, data->dest_list);

			// Disable diskcopy button
			DisableObject(data->list, GAD_DISKCOPY_DISKCOPY, state);
		}
	}
}

// Start the diskcopy routine
BOOL start_diskcopy(diskcopy_data *data)
{
	Att_Node *node;
	long msg = 0;
	short dest_total = 0;
	BOOL ok = 1;
	APTR status;
	short noactive = 0;

	// Get selected node
	if (!(node = Att_FindNode(data->source_list, GetGadgetValue(data->list, GAD_DISKCOPY_SOURCE))))
		return 1;

	// Make window busy
	SetWindowBusy(data->window);

	// Show status text
	SetGadgetValue(data->list, GAD_DISKCOPY_STATUS, (IPTR)GetString(locale, MSG_DISKCOPY_OPEN_DEVICES));

	// Open source device
	if (!(data->source = OpenDisk(node->node.ln_Name, 0)))
	{
		msg = MSG_CANT_OPEN_DEVICE;
	}

	// Got device
	else
	{
		// Inhibit drive
		Inhibit(node->node.ln_Name, DOSTRUE);

		// Check for disk presence
		data->source->dh_io->iotd_Req.io_Command = TD_CHANGESTATE;
		DoIO((struct IORequest *)data->source->dh_io);

		// No source present?
		if (data->source->dh_io->iotd_Req.io_Actual)
			msg = MSG_NO_DISK_PRESENT;
	}

	// Failed on something?
	if (msg)
	{
		// Display error text
		SetGadgetValue(data->list, GAD_DISKCOPY_STATUS, (IPTR)GetString(locale, msg));

		// Cleanup and return
		cleanup_diskcopy(data);
		return 1;
	}

	// Initialise status title
	lsprintf(data->status_title,
			 "%s %s %s",
			 (IPTR)GetString(locale, MSG_DISKCOPY_TITLE),
			 (IPTR)node->node.ln_Name,
			 (IPTR)GetString(locale, MSG_DISKCOPY_TO_TITLE));

	// Go through destination list
	for (node = (Att_Node *)data->dest_list->list.lh_Head; node->node.ln_Succ && ok;
		 node = (Att_Node *)node->node.ln_Succ)
	{
		// Selected?
		if (node->node.lve_Flags & LVEF_SELECTED)
		{
			// Inhibit drive
			Inhibit(node->node.ln_Name, DOSTRUE);

			// Try until successful or aborted
			FOREVER
			{
				msg = 0;

				// Try to open device
				if (!node->data && !(node->data = (IPTR)OpenDisk(node->node.ln_Name, data->dest_port)))
				{
					msg = MSG_CANT_OPEN_DESTINATION;
				}

				// Got device
				else
				{
					// Check for disk presence
					((DiskHandle *)node->data)->dh_io->iotd_Req.io_Command = TD_CHANGESTATE;
					DoIO((struct IORequest *)((DiskHandle *)node->data)->dh_io);

					// No source present?
					if (((DiskHandle *)node->data)->dh_io->iotd_Req.io_Actual)
						msg = MSG_NO_DISK_PRESENT_DEST;

					// Check for write protect (unless same as source)
					else if (strcmp(((DiskHandle *)node->data)->dh_name, data->source->dh_name))
					{
						((DiskHandle *)node->data)->dh_io->iotd_Req.io_Command = TD_PROTSTATUS;
						DoIO((struct IORequest *)((DiskHandle *)node->data)->dh_io);

						// Write protected?
						if (((DiskHandle *)node->data)->dh_io->iotd_Req.io_Actual)
							msg = MSG_WRITE_PROTECTED_DEST;
					}
				}

				// Failed on something?
				if (msg)
				{
					// Put up requester
					msg = SimpleRequestTags(data->window,
											NULL,
											GetString(locale, MSG_RETRY_REMOVE_CANCEL),
											GetString(locale, msg),
											(IPTR)node->node.ln_Name);

					// Retry?
					if (msg == 1)
						continue;

					// Remove from list?
					if (msg == 2)
					{
						if (node->data)
						{
							// Turn motor off
							diskcopy_motor((DiskHandle *)node->data, 0);

							// Close disk
							CloseDisk((DiskHandle *)node->data);
							node->data = 0;
						}
						node->node.lve_Flags &= ~LVEF_SELECTED;
					}

					// Cancel?
					else
					{
						dest_total = 0;
						ok = 0;
					}
				}

				// Successful
				else
				{
					// Increment total
					++dest_total;

					// Add name to status title
					strcat(data->status_title, " ");
					strcat(data->status_title, node->node.ln_Name);
				}
				break;
			}
		}
	}

	// No destinations?
	if (dest_total == 0)
	{
		// Display status text
		SetGadgetValue(data->list, GAD_DISKCOPY_STATUS, (IPTR)GetString(locale, MSG_ABORTED));

		// Cleanup and return
		cleanup_diskcopy(data);
		return 1;
	}

	// Close diskcopy window
	diskcopy_close(data);

	// Open status window
	if ((status = OpenProgressWindowTags(PW_Screen,
										 (IPTR)data->screen,
										 PW_Title,
										 (IPTR)data->status_title,
										 PW_SigTask,
										 (IPTR)data->ipc->proc,
										 PW_SigBit,
										 data->abort_bit,
										 PW_Flags,
										 PWF_INFO | PWF_GRAPH,
										 PW_Info,
										 (IPTR)GetString(locale, MSG_DISKCOPY_COPYING),
										 TAG_END)))
	{
		struct Window *window = 0;

		// Do the diskcopy
		ok = do_diskcopy(data, status);

		// Get window pointer
		GetProgressWindowTags(status, PW_Window, (IPTR)&window, TAG_END);

		// Is window not active?
		if (!window || !(window->Flags & WFLG_WINDOWACTIVE))
			noactive = 1;

		// Close status window
		CloseProgressWindow(status);
	}
	else
		ok = 0;

	// Cleanup
	cleanup_diskcopy(data);

	// Re-open window
	if (!(diskcopy_open(data, noactive)))
		return 0;

	// Display status text
	SetGadgetValue(data->list,
				   GAD_DISKCOPY_STATUS,
				   (IPTR)GetString(locale, (ok) ? MSG_DISKCOPY_SUCCESSFUL : MSG_DISKCOPY_FAILED));

	return 1;
}

// Cleanup after diskcopy
void cleanup_diskcopy(diskcopy_data *data)
{
	Att_Node *node;

	// Close source
	if (data->source)
	{
		// Turn motor off
		diskcopy_motor(data->source, 0);

		// Uninhibit drive
		Inhibit(data->source->dh_name, FALSE);

		// Close disk
		CloseDisk(data->source);
		data->source = 0;
	}

	// Go through destination list
	for (node = (Att_Node *)data->dest_list->list.lh_Head; node->node.ln_Succ; node = (Att_Node *)node->node.ln_Succ)
	{
		// Device open for this node?
		if (node->data)
		{
			// Turn motor off
			diskcopy_motor((DiskHandle *)node->data, 0);

			// Close disk
			CloseDisk((DiskHandle *)node->data);
			node->data = 0;
		}

		// Was node selected?
		if (node->node.lve_Flags & LVEF_SELECTED)
		{
			// Uninhibit drive
			Inhibit(node->node.ln_Name, FALSE);
			node->node.lve_Flags &= ~LVEF_SELECTED;
		}
	}

	// Make window unbusy
	if (data->window)
		ClearWindowBusy(data->window);
}

// Actually do a diskcopy
BOOL do_diskcopy(diskcopy_data *data, APTR status)
{
	Att_Node *node;
	ULONG *track_buffer, *verify_buffer = 0;  // compared in 32-bit words (compare_size)
	unsigned long track_size, compare_size = 0, track_count, buffer_size;
	unsigned long offset, buffer_offset, read_track, write_track;
	unsigned long total_track, track_num, track_update = 1;
	unsigned long root_track;
	ULONG root_offset;	// DivideU() stores a ULONG remainder
	BOOL serialise = 0;
	BOOL abort = 0, same_flag = 0;
	short dest_count = 0;
	struct Window *window = 0;

	// Get window pointer
	GetProgressWindowTags(status, PW_Window, (IPTR)&window, TAG_END);

	// Calculate track size
	track_size = (data->source->dh_geo->de_SizeBlock * 4) * data->source->dh_geo->de_Surfaces *
				 data->source->dh_geo->de_BlocksPerTrack;

	// Get track count
	track_count = data->source->dh_geo->de_HighCyl - data->source->dh_geo->de_LowCyl + 1;

	// Get track the root block is on
	root_track = DivideU(data->source->dh_root,
						 data->source->dh_geo->de_BlocksPerTrack * data->source->dh_geo->de_Surfaces,
						 &root_offset,
						 UtilityBase);
	root_offset *= data->source->dh_blocksize;

	// Got Info?
	if (data->source->dh_result)
	{
		ULONG disktype;
		struct DosList *doslist;

		// Get disk type from DOS list if it's set
		if (!(doslist = (struct DosList *)BADDR(data->source->dh_info.id_VolumeNode)) ||
			!(disktype = doslist->dol_misc.dol_volume.dol_DiskType))
		{
			// If not set, use InfoData
			disktype = data->source->dh_info.id_DiskType;
		}

		// FFS or OFS?
		if (disktype == ID_DOS_DISK || disktype == ID_FFS_DISK || disktype == ID_INTER_DOS_DISK ||
			disktype == ID_INTER_FFS_DISK || disktype == ID_FASTDIR_DOS_DISK || disktype == ID_FASTDIR_FFS_DISK)
			serialise = 1;
	}

	// Go through destinations
	for (node = (Att_Node *)data->dest_list->list.lh_Head; node->node.ln_Succ; node = (Att_Node *)node->node.ln_Succ)
	{
		// Clear ln_Pri
		node->node.ln_Pri = 0;

		// Destination?
		if (node->data)
		{
			// Increment count
			++dest_count;

			// Is destination the same as the source?
			if (!same_flag && (strcmp(node->node.ln_Name, data->source->dh_name) == 0))
			{
				same_flag = 1;
			}

			// Give each destination its own DateStamp
			DateStamp(&((DiskHandle *)node->data)->dh_stamp);
		}
	}

	// Same disk copy?
	if (same_flag)
	{
		// Try for largest possible buffer size
		buffer_size = track_size * track_count;
	}

	// Otherwise, buffer size of a single track
	else
		buffer_size = track_size;

	// Allocate read buffer
	while (!(track_buffer = AllocVec(buffer_size, data->source->dh_geo->de_BufMemType | MEMF_CLEAR)))
	{
		// If request was for more than a single track, halve it
		if (buffer_size > track_size)
			buffer_size >>= 1;

		// Otherwise, fail
		else
			return 0;
	}

	// If verify mode is on, allocate verify buffer
	if (data->default_verify)
	{
		verify_buffer = AllocVec(track_size, data->source->dh_geo->de_BufMemType | MEMF_CLEAR);
		compare_size = track_size >> 2;
	}

	// Calculate total number of tracks
	total_track = track_count * dest_count;
	if (verify_buffer)
		total_track <<= 1;
	total_track += track_count;
	track_num = 1;

	// Copy a track at a time
	for (read_track = 0, write_track = 0; read_track < track_count;)
	{
		BOOL verify = 0;

		// Clear buffer offset
		buffer_offset = 0;

		// Get read offset
		offset = (data->source->dh_geo->de_LowCyl + read_track) * track_size;

		// Read tracks; loop until it succeeds or is aborted
		while (buffer_offset < buffer_size && read_track < track_count)
		{
			// Check for abort
			if (SetSignal(0, 1 << data->abort_bit) & (1 << data->abort_bit))
			{
				abort = 1;
				break;
			}

			// Update bar graph
			SetProgressWindowTags(status, PW_FileCount, total_track, PW_FileNum, track_num, TAG_END);

			// Read track
			data->source->dh_io->iotd_Req.io_Command = CMD_READ;
			data->source->dh_io->iotd_Req.io_Data = ((char *)track_buffer) + buffer_offset;
			data->source->dh_io->iotd_Req.io_Offset = offset;
			data->source->dh_io->iotd_Req.io_Length = track_size;

			// Send command
			if (DoIO((struct IORequest *)data->source->dh_io))
			{
				// Display requester
				if (!(diskcopy_error(window,
									 data->source->dh_io->iotd_Req.io_Error,
									 read_track,
									 data->source->dh_name,
									 MSG_RETRY_CANCEL,
									 MSG_READ_ERROR)))
				{
					abort = 1;
					break;
				}
			}

			// Increment track and buffer offset if successful
			else
			{
				buffer_offset += track_size;
				offset += track_size;
				++read_track;
				++track_num;
			}
		}

		// Check abort
		if (abort)
			break;

		// Need to ask for disk change?
		if (same_flag)
		{
			// Turn motor off
			diskcopy_motor(data->source, 0);

			// Display requester
			if (!(SimpleRequestTags(window,
									0,
									GetString(locale, MSG_PROCEED_CANCEL),
									GetString(locale, MSG_INSERT_DESTINATION),
									(IPTR)data->source->dh_name)))
			{
				abort = 1;
				break;
			}
		}

		// Check abort
		if (abort)
			break;

		// Clear buffer offset
		buffer_offset = 0;

		// Get write offset
		offset = (data->source->dh_geo->de_LowCyl + write_track) * track_size;

		// Loop until all destinations successful or aborted
		while (buffer_offset < buffer_size && write_track < track_count)
		{
			// Loop through write and optional verify pass
			FOREVER
			{
				BOOL loop = 0;
				short send_count = 0;
				ULONG *track_buffer_ptr;

				// Get track buffer pointer
				track_buffer_ptr = (ULONG *)(((char *)track_buffer) + buffer_offset);

				// Go through destinations
				for (node = (Att_Node *)data->dest_list->list.lh_Head; node->node.ln_Succ;
					 node = (Att_Node *)node->node.ln_Succ)
				{
					// Check for abort
					if (SetSignal(0, 1 << data->abort_bit) & (1 << data->abort_bit))
					{
						abort = 1;
						break;
					}

					// Device open for this node?
					if (node->data)
					{
						// Verify?
						if (verify)
						{
							// Not done this track yet? (use ln_Pri field)
							if (node->node.ln_Pri == 1)
							{
								// Update track
								((DiskHandle *)node->data)->dh_io->iotd_Req.io_Command = CMD_UPDATE;
								DoIO((struct IORequest *)((DiskHandle *)node->data)->dh_io);

								// Serialising, and this is the right track?
								if (serialise && write_track == root_track)
								{
									// Serialise it
									serialise_disk(
										(ULONG *)track_buffer_ptr, root_offset, &((DiskHandle *)node->data)->dh_stamp);
								}

								// Read track
								((DiskHandle *)node->data)->dh_io->iotd_Req.io_Command = CMD_READ;
								((DiskHandle *)node->data)->dh_io->iotd_Req.io_Data = verify_buffer;
								((DiskHandle *)node->data)->dh_io->iotd_Req.io_Offset = offset;
								((DiskHandle *)node->data)->dh_io->iotd_Req.io_Length = track_size;
								if (!(DoIO((struct IORequest *)((DiskHandle *)node->data)->dh_io)))
								{
									long cmp;

									// Check verify buffer
									for (cmp = 0; cmp < compare_size; cmp++)
									{
										if (verify_buffer[cmp] != track_buffer_ptr[cmp])
										{
											// Verify error
											((DiskHandle *)node->data)->dh_io->iotd_Req.io_Error = -1;
											break;
										}
									}
								}
							}
						}

						// Write
						else
						{
							// Not done this track yet? (use ln_Pri field)
							if (node->node.ln_Pri == 0)
							{
								// Serialising, and this is the right track?
								if (serialise && write_track == root_track)
								{
									// Serialise it
									serialise_disk(
										(ULONG *)track_buffer_ptr, root_offset, &((DiskHandle *)node->data)->dh_stamp);
								}

								// Write track
								((DiskHandle *)node->data)->dh_io->iotd_Req.io_Command = TD_FORMAT;
								((DiskHandle *)node->data)->dh_io->iotd_Req.io_Data = track_buffer_ptr;
								((DiskHandle *)node->data)->dh_io->iotd_Req.io_Offset = offset;
								((DiskHandle *)node->data)->dh_io->iotd_Req.io_Length = track_size;
								SendIO((struct IORequest *)((DiskHandle *)node->data)->dh_io);
								++send_count;
							}
						}
					}
				}

				// Wait for replies
				while (send_count > 0)
				{
					// Check for abort
					if (SetSignal(0, 1 << data->abort_bit) & (1 << data->abort_bit))
						abort = 1;

					// Any replies in?
					while (GetMsg(data->dest_port))
					{
						// Decrement send count
						--send_count;
					}

					// Replies left to get?
					if (send_count > 0)
						Wait(1 << data->dest_port->mp_SigBit | 1 << data->abort_bit);
				}

				// Check abort
				if (abort)
					break;

				// Go through destinations
				for (node = (Att_Node *)data->dest_list->list.lh_Head; node->node.ln_Succ;
					 node = (Att_Node *)node->node.ln_Succ)
				{
					// Device open for this node?
					if (node->data)
					{
						short ret;

						// Check for error
						if (((DiskHandle *)node->data)->dh_io->iotd_Req.io_Error)
						{
							// Put up requester
							ret = diskcopy_error(window,
												 ((DiskHandle *)node->data)->dh_io->iotd_Req.io_Error,
												 write_track,
												 node->node.ln_Name,
												 MSG_RETRY_REMOVE_CANCEL,
												 (verify) ? MSG_VERIFY_ERROR : MSG_WRITE_ERROR);

							// Abort?
							if (ret == 0)
							{
								abort = 1;
								break;
							}

							// Remove
							if (ret == 2)
							{
								if (node->data)
								{
									// Turn motor off
									diskcopy_motor((DiskHandle *)node->data, 0);

									// Close disk
									CloseDisk((DiskHandle *)node->data);
									node->data = 0;

									// Decrement destination count
									if (--dest_count < 1)
										abort = 1;
									else
									{
										// Increment track update
										++track_update;
									}
								}
							}

							// Retry
							else
							{
								loop = 1;
								node->node.ln_Pri = 0;
							}
						}

						// Mark as ok
						else
						{
							++node->node.ln_Pri;

							// Update bar graph
							SetProgressWindowTags(status, PW_FileCount, total_track, PW_FileNum, track_num, TAG_END);

							// Increment track count
							track_num += track_update;
						}
					}
				}

				// Check abort
				if (abort)
					break;

				// Need to do verify?
				if (verify_buffer && !verify && !loop)
				{
					verify = 1;
					continue;
				}

				// Clear verify flag
				verify = 0;

				// Ok to proceed?
				if (!loop)
					break;
			}

			// Check abort
			if (abort)
				break;

			// Increment offsets
			buffer_offset += track_size;
			offset += track_size;
			++write_track;

			// Go through destinations
			for (node = (Att_Node *)data->dest_list->list.lh_Head; node->node.ln_Succ;
				 node = (Att_Node *)node->node.ln_Succ)
			{
				// Clear ln_Pri field
				node->node.ln_Pri = 0;
			}
		}

		// Check abort
		if (abort)
			break;

		// Need to ask for disk change?
		if (same_flag && read_track < track_count - 1)
		{
			// Turn motor off
			diskcopy_motor(data->source, 0);

			// Display requester
			if (!(SimpleRequestTags(window,
									0,
									GetString(locale, MSG_PROCEED_CANCEL),
									GetString(locale, MSG_INSERT_SOURCE),
									(IPTR)data->source->dh_name)))
			{
				abort = 1;
				break;
			}
		}
	}

	// Free buffers
	FreeVec(track_buffer);
	FreeVec(verify_buffer);

	// Successful?
	if (!abort)
	{
		struct DevProc *proc;
		char disk_name[32], disk_buf[32];

		// Do status text
		if (data->default_bump)
		{
			SetProgressWindowTags(status, PW_Info, (IPTR)GetString(locale, MSG_BUMPING_NAMES), TAG_END);
		}

		// Get initial disk name
		strcpy(disk_name, data->disk_name);

		// Close source
		if (data->source)
		{
			// Turn motor off
			diskcopy_motor(data->source, 0);

			// Uninhibit drive
			Inhibit(data->source->dh_name, FALSE);

			// Close disk
			CloseDisk(data->source);
			data->source = 0;
		}

		// Go through destinations
		for (node = (Att_Node *)data->dest_list->list.lh_Head; node->node.ln_Succ;
			 node = (Att_Node *)node->node.ln_Succ)
		{
			// Destination?
			if (node->data)
			{
				// Turn motor off
				diskcopy_motor((DiskHandle *)node->data, 0);

				// Close disk
				CloseDisk((DiskHandle *)node->data);
				node->data = 0;

				// Get device process
				if ((proc = GetDeviceProc(node->node.ln_Name, 0)))
				{
					// Serialise disk
					DoPkt(proc->dvp_Port, ACTION_SERIALIZE_DISK, 0, 0, 0, 0, 0);
					FreeDeviceProc(proc);
				}

				// Uninhibit drive
				Inhibit(node->node.ln_Name, FALSE);

				// Bump names?
				if (data->default_bump)
				{
					// Bump name
					BumpRevision(disk_buf, disk_name);

					// Relabel disk
					Relabel(node->node.ln_Name, disk_buf);

					// Store name
					strcpy(disk_name, disk_buf);
				}

				// Clear selection flag
				node->node.lve_Flags &= ~LVEF_SELECTED;
			}
		}
	}

	// Return success indicator
	return (BOOL)(!abort);
}

short diskcopy_error(struct Window *window, long error, long track, char *device, long buttons, long message)
{
	long errortext = MSG_ERROR_UNKNOWN;

	// Write protected?
	if (error == TDERR_WriteProt)
		errortext = MSG_ERROR_WRITE_PROT;

	// No disk?
	else if (error == TDERR_DiskChanged)
		errortext = MSG_ERROR_NO_DISK;

	// Verify error?
	else if (error == -1)
		errortext = MSG_ERROR_VERIFY;

	// Put up requester
	return (short)SimpleRequestTags(window,
									0,
									GetString(locale, buttons),
									GetString(locale, message),
									(IPTR)device,
									track,
									(IPTR)GetString(locale, errortext));
}

// Control motor
void diskcopy_motor(DiskHandle *handle, short on)
{
	handle->dh_io->iotd_Req.io_Command = TD_MOTOR;
	handle->dh_io->iotd_Req.io_Length = on;
	DoIO((struct IORequest *)handle->dh_io);
}

// Serialise disk buffer
void serialise_disk(ULONG *data, unsigned long offset, struct DateStamp *date)
{
	unsigned long size, checksum, lw;
	struct DateStamp *ds;

	// Get offset as longwords
	offset >>= 2;

	// Get size of root block
	size = data[offset + 3] + 56;

	// Get DateStamp pointer
	ds = (struct DateStamp *)(data + offset + size - 7);

	// Update datestamp
	*ds = *date;

	// Calculate new checksum
	data[offset + 5] = 0;
	for (lw = 0, checksum = 0; lw < size; lw++)
		checksum += data[offset + lw];

	// Install new checksum
	data[offset + 5] = 0xffffffff - checksum + 1;
}
