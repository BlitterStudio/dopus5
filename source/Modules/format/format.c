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

#include "format.h"

#define FORMAT_LNFS_MIN_FS_VERSION 46
#define FORMAT_DOS_TYPE_MASK 0xffffff00UL
#define FORMAT_FILESYSTEM_RESOURCE "FileSystem.resource"

typedef struct
{
	struct Node fsr_Node;
	struct List fsr_FileSysEntries;
} FormatFileSysResource;

typedef struct
{
	struct Node fse_Node;
	unsigned long fse_DosType;
	unsigned long fse_Version;
} FormatFileSysEntry;

int sys_format(struct Screen *screen, char *name);

static BOOL format_is_dos_filesystem(unsigned long dos_type)
{
	return (BOOL)((dos_type & FORMAT_DOS_TYPE_MASK) == ID_DOS_DISK);
}

static unsigned short format_filesystem_resource_version(unsigned long dos_type)
{
	FormatFileSysResource *resource;
	FormatFileSysEntry *entry;
	unsigned short version = 0;

	if (!(resource = (FormatFileSysResource *)OpenResource(FORMAT_FILESYSTEM_RESOURCE)))
		return 0;

	Forbid();
	for (entry = (FormatFileSysEntry *)resource->fsr_FileSysEntries.lh_Head; entry->fse_Node.ln_Succ;
		 entry = (FormatFileSysEntry *)entry->fse_Node.ln_Succ)
	{
		if (entry->fse_DosType == dos_type)
		{
			version = (unsigned short)(entry->fse_Version >> 16);
			break;
		}
	}
	Permit();

	return version;
}

static void format_handler_name(struct DosList *dl, char *handler_name, short size)
{
	BPTR handler;

	if (!handler_name || size < 1)
		return;
	handler_name[0] = 0;

	if (!dl || !(handler = dl->dol_misc.dol_handler.dol_Handler))
		return;

	if (TypeOfMem(BADDR(handler)) == 0)
		return;

	BtoCStr((BSTR)handler, handler_name, size);
}

static BOOL format_filesystem_supports_lnfs(unsigned long dos_type, char *handler_name)
{
	short version, revision;

	if (!format_is_dos_filesystem(dos_type))
		return FALSE;

	if (((struct Library *)DOSBase)->lib_Version < FORMAT_LNFS_MIN_FS_VERSION)
		return FALSE;

	if (dos_type == ID_FFS7_DISK)
		return TRUE;

	if (handler_name && handler_name[0])
	{
		if (GetFileVersion(handler_name, &version, &revision, 0, 0))
			return (BOOL)(version >= FORMAT_LNFS_MIN_FS_VERSION);
	}

	return (BOOL)(format_filesystem_resource_version(dos_type) >= FORMAT_LNFS_MIN_FS_VERSION);
}

int LIBFUNC L_Module_Entry(REG(a0, struct List *disks),
						   REG(a1, struct Screen *screen),
						   REG(a2, IPCData *ipc),
						   REG(a3, IPCData *main_ipc),
						   REG(d0, IPTR mod_id),
						   REG(d1, ULONG mod_data))
{
	format_data *data;
	short ret = 1;
	BOOL one_flag = 0;
#ifdef __amigaos4__
	char *name = NULL;
	IPCMessage *imsg;
	int result = 0;

	// Get disk name if one is supplied
	if (disks && !(IsListEmpty(disks)))
		name = disks->lh_Head->ln_Name;
	// Launch system format command
	result = sys_format(screen, name);
	// IPC messages?
	if (ipc)
		while ((imsg = (IPCMessage *)GetMsg(ipc->command_port)))
			IPC_Reply(imsg);
	return result;
#endif
	// Allocate data
	if (!(data = AllocVec(sizeof(format_data), MEMF_CLEAR)))
		return 0;

	// Store IPC pointer
	data->ipc = ipc;
	data->screen = screen;

	// Get abort bit
	data->abort_bit = AllocSignal(-1);

	// Initialise defaults
	data->default_ffs = 1;
	data->default_verify = 1;
	strcpy(data->default_name, GetString(locale, MSG_FORMAT_DEFAULT_NAME));

	// Disk name supplied?
	if (disks && !(IsListEmpty(disks)))
	{
		BPTR lock;

		// Build device list
		data->device_list = get_device_list(disks->lh_Head->ln_Name);

		// Couldn't find?
		if (IsListEmpty((struct List *)data->device_list))
		{
			format_free(data);
			return 0;
		}

		// Lock device
		if ((lock = Lock(disks->lh_Head->ln_Name, ACCESS_READ)))
		{
			// Get disk info
			Info(lock, &data->info);
			UnLock(lock);

			// Get default name
			lsprintf(data->default_name, "%b", ((struct DosList *)BADDR(data->info.id_VolumeNode))->dol_Name);

			// FFS?
			if (data->info.id_DiskType == ID_FFS_DISK || data->info.id_DiskType == ID_INTER_FFS_DISK ||
				data->info.id_DiskType == ID_FASTDIR_FFS_DISK || data->info.id_DiskType == ID_FFS7_DISK)
				data->default_ffs = 1;

			// >=39?
			if (((struct Library *)DOSBase)->lib_Version >= 39)
			{
				// International?
				if (data->info.id_DiskType == ID_INTER_DOS_DISK || data->info.id_DiskType == ID_INTER_FFS_DISK)
					data->default_int = 1;

				// Cache?
				if (data->info.id_DiskType == ID_FASTDIR_DOS_DISK || data->info.id_DiskType == ID_FASTDIR_FFS_DISK)
				{
					data->default_int = 1;
					data->default_cache = 1;
				}

				// Long File Names? (FFS-LNFS implies FFS+International)
				if (data->info.id_DiskType == ID_FFS7_DISK)
				{
					data->default_int = 1;
					data->default_lnfs = 1;
				}
			}

			// If previously formatted, assume no verify needed
			data->default_verify = 0;
		}
		else
			data->default_verify = 1;

		// Set flag
		one_flag = 1;
	}

	// Otherwise, build device list
	else
		data->device_list = get_device_list(0);

	// Open window
	if (!(format_open(data, 0)))
	{
		format_free(data);
		return 0;
	}

	// Disk name supplied?
	if (one_flag)
	{
		// Select first node
		SetGadgetValue(data->list, GAD_FORMAT_DEVICES, 0);

		// Enable format buttons
		DisableObject(data->list, GAD_FORMAT_QUICK_FORMAT, FALSE);
		DisableObject(data->list, GAD_FORMAT_FORMAT, FALSE);

		// Show device information
		show_device_info(data);
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

				// Disk change
				case IDCMP_DISKINSERTED:
				case IDCMP_DISKREMOVED:

					// Show device information
					show_device_info(data);
					break;

				// Gadget
				case IDCMP_GADGETUP:
					switch (((struct Gadget *)msg_copy.IAddress)->GadgetID)
					{
					// Cancel
					case GAD_FORMAT_CANCEL:
						quit_flag = 1;
						break;

					// Device selected
					case GAD_FORMAT_DEVICES:

						// Enable format buttons
						DisableObject(data->list, GAD_FORMAT_QUICK_FORMAT, FALSE);
						DisableObject(data->list, GAD_FORMAT_FORMAT, FALSE);

						// Show device information
						show_device_info(data);
						break;

					// Caching implies International, mutually exclusive with Long File Names
					case GAD_FORMAT_CACHING: {
						BOOL state;

						// Get state
						state = GetGadgetValue(data->list, GAD_FORMAT_CACHING);

						// Enable/disable International gadget
						DisableObject(data->list, GAD_FORMAT_INTERNATIONAL, state);

						// If on, check International and clear LNFS
						if (state)
						{
							SetGadgetValue(data->list, GAD_FORMAT_INTERNATIONAL, 1);
							SetGadgetValue(data->list, GAD_FORMAT_LNFS, 0);
						}
					}
					break;

					// Long File Names implies FFS+International, excludes Caching
					case GAD_FORMAT_LNFS: {
						BOOL state;

						// Get state
						state = GetGadgetValue(data->list, GAD_FORMAT_LNFS);

						// If on, force FFS+International, clear Caching
						if (state)
						{
							SetGadgetValue(data->list, GAD_FORMAT_FFS, 1);
							SetGadgetValue(data->list, GAD_FORMAT_INTERNATIONAL, 1);
							SetGadgetValue(data->list, GAD_FORMAT_CACHING, 0);
						}

						// Lock implied gadgets while LNFS is active
						DisableObject(data->list, GAD_FORMAT_FFS, state);
						DisableObject(data->list, GAD_FORMAT_INTERNATIONAL, state);
						DisableObject(data->list, GAD_FORMAT_CACHING, state);
					}
					break;

					// Do format
					case GAD_FORMAT_FORMAT:
					case GAD_FORMAT_QUICK_FORMAT:

						// Run the format routine
						if (!(start_format(data, ((struct Gadget *)msg_copy.IAddress)->GadgetID, !one_flag)))
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
							IPC_Command(main_ipc, IPC_HELP, (1 << 31), "Format", 0, (struct MsgPort *)-1);

							// Clear busy pointer
							ClearWindowBusy(data->window);
						}
					}
					break;
				}

				// Check window is still valid
				if (!data->window)
					break;
			}
		}

		if (quit_flag)
			break;

		Wait(((data->window) ? (1 << data->window->UserPort->mp_SigBit) : 0) |
			 ((data->ipc) ? (1 << data->ipc->command_port->mp_SigBit) : 0));
	}

	// Free abort bit
	if (data->abort_bit != -1)
		FreeSignal(data->abort_bit);

	// Free stuff
	format_free(data);
	return ret;
}

// Open format window
BOOL format_open(format_data *data, BOOL noactive)
{
	long sel;
	BOOL selected = FALSE;

	// Fill out new window
	data->new_win.parent = data->screen;
	data->new_win.dims = &data->win_dims;
	data->new_win.title = GetString(locale, MSG_FORMAT_TITLE);
	data->new_win.locale = locale;
	data->new_win.flags = WINDOW_SCREEN_PARENT | WINDOW_VISITOR | WINDOW_AUTO_KEYS | WINDOW_REQ_FILL;
	if (noactive)
		data->new_win.flags |= WINDOW_NO_ACTIVATE;

	// Default dimensions
	data->win_dims = format_window;

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
		!(data->list = AddObjectList(data->window, format_objects)))
	{
		return 0;
	}

	// Fix IDCMP flags to add disk insertion/removal
	ModifyIDCMP(data->window, data->window->IDCMPFlags | IDCMP_DISKINSERTED | IDCMP_DISKREMOVED);

	// Attach device list
	SetGadgetChoices(data->list, GAD_FORMAT_DEVICES, data->device_list);

	// Current selection?
	if (data->selection[0] && (sel = Att_NodeNumber(data->device_list, data->selection)) != -1)
	{
		// Select this entry
		SetGadgetValue(data->list, GAD_FORMAT_DEVICES, sel);
		selected = TRUE;
	}

	// Otherwise, disable format buttons initially
	else
	{
		DisableObject(data->list, GAD_FORMAT_QUICK_FORMAT, TRUE);
		DisableObject(data->list, GAD_FORMAT_FORMAT, TRUE);
		DisableObject(data->list, GAD_FORMAT_LNFS, TRUE);
	}

	// If <39, disable International, Caching and Long File Names
	// (LNFS implies International compare, so it can't work on
	// pre-KS2 dos.library either)
	if (((struct Library *)DOSBase)->lib_Version < 39)
	{
		DisableObject(data->list, GAD_FORMAT_INTERNATIONAL, TRUE);
		DisableObject(data->list, GAD_FORMAT_CACHING, TRUE);
		DisableObject(data->list, GAD_FORMAT_LNFS, TRUE);
	}

	// Set defaults
	SetGadgetValue(data->list, GAD_FORMAT_NAME, (IPTR)data->default_name);
	SetGadgetValue(data->list, GAD_FORMAT_FFS, data->default_ffs);
	SetGadgetValue(data->list, GAD_FORMAT_INTERNATIONAL, data->default_int);
	SetGadgetValue(data->list, GAD_FORMAT_CACHING, data->default_cache);
	SetGadgetValue(data->list, GAD_FORMAT_LNFS, data->default_lnfs);
	SetGadgetValue(data->list, GAD_FORMAT_TRASHCAN, data->default_trash);
	SetGadgetValue(data->list, GAD_FORMAT_INSTALL, data->default_boot);
	SetGadgetValue(data->list, GAD_FORMAT_VERIFY, data->default_verify);

	// If caching is on, disable international
	if (data->default_cache)
		DisableObject(data->list, GAD_FORMAT_INTERNATIONAL, TRUE);

	// If LNFS is on, FFS/International/Caching are forced and locked
	if (data->default_lnfs)
	{
		DisableObject(data->list, GAD_FORMAT_FFS, TRUE);
		DisableObject(data->list, GAD_FORMAT_INTERNATIONAL, TRUE);
		DisableObject(data->list, GAD_FORMAT_CACHING, TRUE);
	}

	if (selected)
		show_device_info(data);

	return 1;
}

// Close format display
void format_close(format_data *data)
{
	// Window open?
	if (data->window)
	{
		// Store position
		data->window_pos = *((Point *)&data->window->LeftEdge);
		data->pos_valid = 1;

		// Store settings
		strcpy(data->default_name, (char *)GetGadgetValue(data->list, GAD_FORMAT_NAME));
		data->default_ffs = GetGadgetValue(data->list, GAD_FORMAT_FFS);
		data->default_int = GetGadgetValue(data->list, GAD_FORMAT_INTERNATIONAL);
		data->default_cache = GetGadgetValue(data->list, GAD_FORMAT_CACHING);
		data->default_lnfs = GetGadgetValue(data->list, GAD_FORMAT_LNFS);
		data->default_trash = GetGadgetValue(data->list, GAD_FORMAT_TRASHCAN);
		data->default_boot = GetGadgetValue(data->list, GAD_FORMAT_INSTALL);
		data->default_verify = GetGadgetValue(data->list, GAD_FORMAT_VERIFY);

		// Close window
		CloseConfigWindow(data->window);
		data->window = 0;
	}
}

// Free format data
void format_free(format_data *data)
{
	if (data)
	{
		// Close window
		CloseConfigWindow(data->window);
		data->window = 0;

		// Free device list
		Att_RemList(data->device_list, 0);

		// Free data
		FreeVec(data);
	}
}

// Build device list
Att_List *get_device_list(char *only_get)
{
	struct DosList *dl;
	Att_List *list;

	// Create a list
	if (!(list = Att_NewList(0)))
		return 0;

	// Lock dos list
	dl = LockDosList(LDF_DEVICES | LDF_READ);

	// Scan device list
	while ((dl = NextDosEntry(dl, LDF_DEVICES)))
	{
		// Is it a valid device?
		if (dl->dol_Task && (IPTR)dl->dol_misc.dol_handler.dol_Startup > 512)
		{
			char devname[32];

			// Convert name
			lsprintf(devname, "%b:", dl->dol_Name);

			// Only looking for one?
			if (!only_get || stricmp(devname, only_get) == 0)
			{
				// Add to list
				Att_NewNode(list, devname, 0, ADDNODE_SORT);
			}
		}
	}

	// Unlock dos list
	UnLockDosList(LDF_DEVICES | LDF_READ);

	return list;
}

// Show information on a device
void show_device_info(format_data *data)
{
	Att_Node *node;
	struct DosList *dl;
	char name_buf[32], *ptr;
	char handler_name[108] = {0};
	char info_buf[120];
	unsigned long dos_type = ID_DOS_DISK, table_size = 0;
	BOOL device_found = FALSE;
	BOOL lnfs_supported = FALSE;

	// Get selected node
	if (!(node = Att_FindNode(data->device_list, GetGadgetValue(data->list, GAD_FORMAT_DEVICES))))
	{
		SetGadgetValue(data->list, GAD_FORMAT_STATUS, 0);
		return;
	}

	// Store selection
	strcpy(data->selection, node->node.ln_Name);

	// Get name, strip colon
	strcpy(name_buf, node->node.ln_Name);
	if ((ptr = strchr(name_buf, ':')))
		*ptr = 0;

	// Lock dos list
	dl = LockDosList(LDF_DEVICES | LDF_READ);

	// Find entry
	if ((dl = FindDosEntry(dl, name_buf, LDF_DEVICES)))
	{
		struct DosEnvec *geo;
		long tracks, track_size, size;
		char size_buf[20];

		// Get disk geometry
		geo = (struct DosEnvec *)BADDR(
			((struct FileSysStartupMsg *)BADDR(dl->dol_misc.dol_handler.dol_Startup))->fssm_Environ);

		// Get tracks and track size
		tracks = geo->de_HighCyl - geo->de_LowCyl + 1;
		track_size = (geo->de_BlocksPerTrack * geo->de_Surfaces) * (geo->de_SizeBlock * 4);

		// Store dos type and table size
		dos_type = geo->de_DosType;
		table_size = geo->de_TableSize;
		device_found = TRUE;
		format_handler_name(dl, handler_name, sizeof(handler_name));

		// Calculate size of disk
		size = tracks * track_size;
		BytesToString(size, size_buf, 1, 0);

		// Build display string
		lsprintf(info_buf, GetString(locale, MSG_FORMAT_STATUS), tracks, track_size, (IPTR)size_buf);
	}
	else
		info_buf[0] = 0;

	// Unlock dos list
	UnLockDosList(LDF_DEVICES | LDF_READ);

	if (device_found)
		lnfs_supported = format_filesystem_supports_lnfs(dos_type, handler_name);

	// Identify PFS-family volumes in the status text so the user knows
	// they are re-formatting a PFS partition rather than a plain DOS
	// one. The PFS3 filesystem itself can serve any of these DOS types
	// depending on which features (deldir, large file, multiuser) the
	// partition was set up with, so match the whole family rather than
	// just the PFS\3 / PDS\3 / muPF triple.
	if (info_buf[0])
	{
		BOOL is_pfs = FALSE;

		/* PFS\0..PFS\3: PFS variants in DOS\x ID space. */
		if ((dos_type & 0xFFFFFFFCUL) == (ID_PFS_FLOPPY & 0xFFFFFFFCUL))
			is_pfs = TRUE;

		/* PDS\0..PDS\3: Direct-SCSI variants. dopus5.h only declares
		 * PDS\2/PDS\3 (ID_PFS2_SC_DISK / ID_PFS3_SC_DISK), but the
		 * masked compare also covers any PDS\0/PDS\1 a setup might
		 * use; deriving the base from ID_PFS3_SC_DISK keeps this in
		 * step with the header.
		 */
		else if ((dos_type & 0xFFFFFFFCUL) == (ID_PFS3_SC_DISK & 0xFFFFFFFCUL))
			is_pfs = TRUE;

		/* muPF: PFS3 multiuser. */
		else if (dos_type == ID_PFS3_MULTI)
			is_pfs = TRUE;

		if (is_pfs)
		{
			char *fs_name = GetString(locale, MSG_FORMAT_FS_PFS);
			int cur_len = strlen(info_buf);
			int suffix_len = strlen(fs_name) + 4; /* " (" + name + ")" + NUL */

			if ((long)(cur_len + suffix_len) < (long)sizeof(info_buf))
			{
				strcat(info_buf, " (");
				strcat(info_buf, fs_name);
				strcat(info_buf, ")");
			}
		}
	}

	// Display status
	SetGadgetValue(data->list, GAD_FORMAT_STATUS, (IPTR)info_buf);

	// If this isn't a standard dos disk, disable FFS, etc.
	// Also lock FFS/International/Caching whenever LNFS is on, since
	// LNFS implies FFS+International and excludes Directory Caching.
	{
		BOOL not_dos = !format_is_dos_filesystem(dos_type);
		BOOL lnfs_on = GetGadgetValue(data->list, GAD_FORMAT_LNFS);
		BOOL cache_on = GetGadgetValue(data->list, GAD_FORMAT_CACHING);

		if (!lnfs_supported && lnfs_on)
		{
			SetGadgetValue(data->list, GAD_FORMAT_LNFS, 0);
			lnfs_on = FALSE;
		}

		DisableObject(data->list, GAD_FORMAT_LNFS, not_dos || !lnfs_supported);
		DisableObject(data->list, GAD_FORMAT_FFS, not_dos || lnfs_on);
		DisableObject(data->list, GAD_FORMAT_CACHING, not_dos || lnfs_on);
		DisableObject(data->list, GAD_FORMAT_INTERNATIONAL, not_dos || cache_on || lnfs_on);
	}

	// Disable install if no bootblock entry in table
	DisableObject(data->list, GAD_FORMAT_INSTALL, (table_size < DE_BOOTBLOCKS));
}

// Start the format routine
BOOL start_format(format_data *data, unsigned short type, BOOL reopen)
{
	DiskHandle *disk;
	Att_Node *node;
	long msg = 0;
	BPTR lock;
	BOOL blank = 1;
	short noactive;

	// Get selected node
	if (!(node = Att_FindNode(data->device_list, GetGadgetValue(data->list, GAD_FORMAT_DEVICES))))
		return 1;

	// Make window busy
	SetWindowBusy(data->window);

	// Show status text
	SetGadgetValue(data->list, GAD_FORMAT_STATUS, (IPTR)GetString(locale, MSG_OPENING_DEVICE));

	// Open device
	if (!(disk = OpenDisk(node->node.ln_Name, 0)))
	{
		// Failed
		msg = MSG_CANT_OPEN_DEVICE;
	}

	// Got device
	else
	{
		// Check for disk presence
		disk->dh_io->iotd_Req.io_Command = TD_CHANGESTATE;
		DoIO((struct IORequest *)disk->dh_io);

		// No disk present?
		if (disk->dh_io->iotd_Req.io_Actual)
			msg = MSG_NO_DISK_PRESENT;

		// Check for write protect
		else
		{
			// Got disk information?
			if (disk->dh_result)
			{
				// Write-protected?
				if (disk->dh_info.id_DiskState == ID_WRITE_PROTECTED)
					msg = MSG_DISK_WRITE_PROTECTED;
			}

			// Might be a bad disk or something, do it through the device
			else
			{
				disk->dh_io->iotd_Req.io_Command = TD_PROTSTATUS;
				DoIO((struct IORequest *)disk->dh_io);

				// Write protected?
				if (disk->dh_io->iotd_Req.io_Actual)
					msg = MSG_DISK_WRITE_PROTECTED;
			}
		}
	}

	// Failed on something?
	if (msg)
	{
		// Display error text
		SetGadgetValue(data->list, GAD_FORMAT_STATUS, (IPTR)GetString(locale, msg));

		// Cleanup and return
		CloseDisk(disk);
		ClearWindowBusy(data->window);
		return 1;
	}

	// Display status text
	SetGadgetValue(data->list, GAD_FORMAT_STATUS, (IPTR)GetString(locale, MSG_CHECKING_DISK));

	// Try and lock device
	if ((lock = Lock(disk->dh_name, ACCESS_READ)))
	{
		// Get device info
		Info(lock, &data->info);

		// Check disk type
		switch (data->info.id_DiskType)
		{
		// Invalid dos disk
		case ID_UNREADABLE_DISK:
		case ID_NOT_REALLY_DOS:
			break;

		// Valid disk
		default:

			// Get disk name
			Examine(lock, &data->fib);
			strcpy(data->disk_name, data->fib.fib_FileName);

			// If there's any files, disk is not blank
			if (ExNext(lock, &data->fib))
				blank = 0;
			break;
		}

		UnLock(lock);
	}

	// Is disk not blank?
	if (!blank)
	{
		char size_buf[27];

		// Get size string
#ifdef USE_64BIT
		{
			UQUAD tmp = (UQUAD)data->info.id_NumBlocksUsed * (UQUAD)data->info.id_BytesPerBlock;
			BytesToString64(&tmp, size_buf, sizeof(size_buf), 1, 0);
		}
#else
		BytesToString(data->info.id_NumBlocksUsed * data->info.id_BytesPerBlock, size_buf, 1, 0);
#endif

		// Display requester
		if (SimpleRequestTags(data->window,
							  0,
							  GetString(locale, MSG_PROCEED_CANCEL),
							  GetString(locale, MSG_DISK_NOT_BLANK),
							  (IPTR)disk->dh_name,
							  (IPTR)data->disk_name,
							  (IPTR)size_buf))
			blank = 1;
	}

	// Fail?
	if (!blank)
	{
		// Display aborted
		SetGadgetValue(data->list, GAD_FORMAT_STATUS, (IPTR)GetString(locale, MSG_ABORTED));

		// Cleanup and return
		CloseDisk(disk);
		ClearWindowBusy(data->window);
		return 1;
	}

	// Close window
	format_close(data);

	// Non-standard DOS?
	if ((disk->dh_geo->de_DosType & ID_DOS_DISK) != ID_DOS_DISK)
		data->dos_type = disk->dh_geo->de_DosType;

	// Long File Names? (always FFS+International, requires FFS-LNFS aware filesystem)
	else if (data->default_lnfs)
		data->dos_type = ID_FFS7_DISK;

	// Caching?
	else if (data->default_cache)
	{
		if (data->default_ffs)
			data->dos_type = ID_FASTDIR_FFS_DISK;
		else
			data->dos_type = ID_FASTDIR_DOS_DISK;
	}

	// International?
	else if (data->default_int)
	{
		if (data->default_ffs)
			data->dos_type = ID_INTER_FFS_DISK;
		else
			data->dos_type = ID_INTER_DOS_DISK;
	}

	// FFS?
	else if (data->default_ffs)
		data->dos_type = ID_FFS_DISK;

	// Normal
	else
		data->dos_type = ID_DOS_DISK;

	// Do the format
	blank = do_format(data, disk, type, &noactive);

	// Close device
	CloseDisk(disk);

	// Re-open window
	if (!reopen || !(format_open(data, noactive)))
		return 0;

	// Display status text
	SetGadgetValue(
		data->list, GAD_FORMAT_STATUS, (IPTR)GetString(locale, (blank) ? MSG_FORMAT_SUCCESSFUL : MSG_FORMAT_FAILED));

	return 1;
}

// Actually do the format
BOOL do_format(format_data *data, DiskHandle *disk, unsigned short type, short *noactive)
{
	APTR status;
	short suc = 1;
	struct Window *window = 0;

	// Initialise noactive
	*noactive = 0;

	// Build status title window
	lsprintf(data->status_title, "%s %s", (IPTR)GetString(locale, MSG_FORMAT_TITLE), (IPTR)disk->dh_name);

	// Open status window
	if (!(status = OpenProgressWindowTags(PW_Screen,
										  (IPTR)data->screen,
										  PW_Title,
										  (IPTR)data->status_title,
										  PW_SigTask,
										  (IPTR)data->ipc->proc,
										  PW_SigBit,
										  (IPTR)data->abort_bit,
										  PW_Flags,
										  PWF_INFO | PWF_GRAPH,
										  PW_Info,
										  (IPTR)GetString(locale, MSG_FORMAT_SETTING_UP),
										  TAG_END)))
		return 0;

	// Inhibit the drive
	Inhibit(disk->dh_name, DOSTRUE);

	// Turn drive motor on
	disk->dh_io->iotd_Req.io_Command = TD_MOTOR;
	disk->dh_io->iotd_Req.io_Length = 1;
	DoIO((struct IORequest *)disk->dh_io);

	// If not quick formatting, do low-level format
	if (type != GAD_FORMAT_QUICK_FORMAT)
	{
		// Format disk
		suc = do_raw_format(data, disk, status);
	}

	// If we've succeeded thus far, initialise the disk
	if (suc)
	{
		// Display status text
		SetProgressWindowTags(status, PW_Info, (IPTR)GetString(locale, MSG_FORMAT_INITIALISING), TAG_END);

		// Initialise disk
#ifdef __amigaos4__
		if (!(Format(disk->dh_name, data->default_name, data->dos_type, 0)))
#else
		if (!(Format(disk->dh_name, data->default_name, data->dos_type)))
#endif
			suc = 0;

		// Make bootable?
		else if (data->default_boot)
		{
			do_install(data, disk, status);
		}
	}

	// Display status text
	SetProgressWindowTags(status, PW_Info, (IPTR)GetString(locale, MSG_FORMAT_CLEANING_UP), TAG_END);

	// Turn motor off
	disk->dh_io->iotd_Req.io_Command = TD_MOTOR;
	disk->dh_io->iotd_Req.io_Length = 0;
	DoIO((struct IORequest *)disk->dh_io);

	// Uninhibit the drive
	Inhibit(disk->dh_name, FALSE);

	// Successful; do trashcan?
	if (suc && data->default_trash)
	{
		struct DiskObject *trash;
		char trash_name[80];
		BPTR lock;

		// Display status text
		SetProgressWindowTags(status, PW_Info, (IPTR)GetString(locale, MSG_FORMAT_MAKING_TRASH), TAG_END);

		// Build trashcan name
		lsprintf(trash_name, "%sTrashcan", (IPTR)disk->dh_name);

		// Create directory
		if ((lock = CreateDir(trash_name)))
		{
			UnLock(lock);

			// Add icon
			if ((trash = GetDefDiskObject(WBGARBAGE)))
			{
				PutDiskObject(trash_name, trash);
				FreeDiskObject(trash);
			}
		}
	}

	// Get window pointer
	GetProgressWindowTags(status, PW_Window, (IPTR)&window, TAG_END);

	// Is window not active?
	if (!window || !(window->Flags & WFLG_WINDOWACTIVE))
		*noactive = 1;

	// Close status window
	CloseProgressWindow(status);

	return suc;
}

// Low-level format a disk
BOOL do_raw_format(format_data *data, DiskHandle *disk, APTR status)
{
	ULONG *track_buffer, *verify_buffer = 0;  // compared in 32-bit words (compare_size)
	unsigned long track_size, compare_size = 0, track_count;
	unsigned long offset, track;
	struct Window *window;
	BOOL abort = 0;

	// Calculate track size
	track_size = (disk->dh_geo->de_SizeBlock * 4) * disk->dh_geo->de_Surfaces * disk->dh_geo->de_BlocksPerTrack;

	// Allocate write buffer
	if (!(track_buffer = AllocVec(track_size, disk->dh_geo->de_BufMemType | MEMF_CLEAR)))
		return 0;

	// If verify mode is on, allocate read buffer
	if (data->default_verify)
	{
		verify_buffer = AllocVec(track_size, disk->dh_geo->de_BufMemType | MEMF_CLEAR);
		compare_size = track_size >> 2;
	}

	// Starting offset
	offset = disk->dh_geo->de_LowCyl * track_size;

	// Get track count
	track_count = disk->dh_geo->de_HighCyl - disk->dh_geo->de_LowCyl + 1;

	// Get progress window
	GetProgressWindowTags(status, PW_Window, (IPTR)&window, TAG_END);

	// Display status text
	SetProgressWindowTags(
		status, PW_Info, (IPTR)GetString(locale, MSG_FORMAT_FORMATTING), PW_FileCount, track_count, TAG_END);

	// Format a track at a time
	for (track = 0; track < track_count; track++)
	{
		BOOL verify = 0;

		// Loop until it succeeds or is aborted
		FOREVER
		{
			unsigned short error = 0;

			// Check for abort
			if (SetSignal(0, 1 << data->abort_bit) & (1 << data->abort_bit))
			{
				abort = 1;
				break;
			}

			// Verify?
			if (verify)
			{
				// Read track
				disk->dh_io->iotd_Req.io_Command = CMD_READ;
				disk->dh_io->iotd_Req.io_Data = (APTR)verify_buffer;
				disk->dh_io->iotd_Req.io_Offset = offset;
				disk->dh_io->iotd_Req.io_Length = track_size;
			}

			// Otherwise, format this track
			else
			{
				// Format track
				disk->dh_io->iotd_Req.io_Command = TD_FORMAT;
				disk->dh_io->iotd_Req.io_Data = (APTR)track_buffer;
				disk->dh_io->iotd_Req.io_Offset = offset;
				disk->dh_io->iotd_Req.io_Length = track_size;

				// Update bar graph
				SetProgressWindowTags(status, PW_FileNum, track, TAG_END);
			}

			// Send command
			if (DoIO((struct IORequest *)disk->dh_io))
			{
				// Format error
				error = MSG_FORMAT_FORMATERROR;
			}

			// Command successful; was it verify?
			else if (verify)
			{
				long cmp;

				// Check verify buffer
				for (cmp = 0; cmp < compare_size; cmp++)
				{
					if (verify_buffer[cmp] != 0)
					{
						error = MSG_FORMAT_VERIFYERROR;
						break;
					}
				}
			}

			// An error?
			if (error)
			{
				// Display requester
				if (!(SimpleRequestTags(window,
										data->status_title,
										GetString(locale, MSG_RETRY_CANCEL),
										GetString(locale, error),
										track)))
				{
					abort = 1;
					break;
				}

				// Otherwise, need to try again
				else
				{
					// If it was a verify error, write the track again
					if (error == MSG_FORMAT_VERIFYERROR)
						verify = 0;
				}
				continue;
			}

			// Successful. Do we need to do verify?
			if (!verify_buffer || verify)
				break;

			// Send update command to write buffer
			disk->dh_io->iotd_Req.io_Command = CMD_UPDATE;
			DoIO((struct IORequest *)disk->dh_io);

			// Set verify flag and continue
			verify = 1;
		}

		// Check for abort
		if (abort)
			break;

		// Increment offset
		offset += track_size;
	}

	// Update bar graph
	SetProgressWindowTags(status, PW_FileNum, track, TAG_END);

	// Free buffers
	FreeVec(track_buffer);
	FreeVec(verify_buffer);

	// Return success indicator
	return (BOOL)(!abort);
}

// Install bootblock
void do_install(format_data *data, DiskHandle *disk, APTR status)
{
	unsigned long *boot_buffer;
	unsigned long track_size, boot_size;
	long a, sum_size, sum;

	// Disk valid for bootblock?
	if (disk->dh_geo->de_TableSize < DE_BOOTBLOCKS)
		return;

	// Calculate track size
	track_size = (disk->dh_geo->de_SizeBlock * 4) * disk->dh_geo->de_Surfaces * disk->dh_geo->de_BlocksPerTrack;

	// Calculate bootblock size
	boot_size = (disk->dh_geo->de_SizeBlock * 4) * disk->dh_geo->de_BootBlocks;

	// Allocate buffer for track
	if (!(boot_buffer = AllocVec(boot_size, disk->dh_geo->de_BufMemType | MEMF_CLEAR)))
		return;

	// Show status text
	SetProgressWindowTags(status, PW_Info, (IPTR)GetString(locale, MSG_INSTALLING_DISK), TAG_END);

	// Copy standard 2.0 bootblock into buffer
	CopyMem((char *)bootblock_20, (char *)(boot_buffer + 2), sizeof(bootblock_20));

	// Set DOS type
	boot_buffer[0] = data->dos_type;

	// Calculate checksum
	for (a = 0, sum = 0, sum_size = boot_size >> 2; a < sum_size; a++)
	{
		long lastsum;

		lastsum = sum;
		sum += boot_buffer[a];
		if (lastsum > sum)
			++sum;
	}

	// Store checksum
	boot_buffer[1] = (unsigned long)~sum;

	// Write bootblock
	disk->dh_io->iotd_Req.io_Command = CMD_WRITE;
	disk->dh_io->iotd_Req.io_Data = boot_buffer;
	disk->dh_io->iotd_Req.io_Offset = disk->dh_geo->de_LowCyl * track_size;
	disk->dh_io->iotd_Req.io_Length = boot_size;
	if (!(DoIO((struct IORequest *)disk->dh_io)))
	{
		disk->dh_io->iotd_Req.io_Command = CMD_UPDATE;
		DoIO((struct IORequest *)disk->dh_io);
	}

	// Free bootblock buffer
	FreeVec(boot_buffer);
}

#ifdef __amigaos4__
int sys_format(struct Screen *screen, char *name)
{
	char scrname[MAXPUBSCREENNAME + 2] = {0};
	char oldname[MAXPUBSCREENNAME + 2] = {0};
	char *command = "SYS:System/Format";
	char *commandline = NULL;
	char devname[108] = {0};
	UWORD modes = 0;
	LONG result = 0;
	BPTR lock = 0;

	if (!(lock = Lock(command, ACCESS_READ)))
		return 0;
	UnLock(lock);
	if (name && (lock = Lock(name, ACCESS_READ)))
	{
		result = DevNameFromLock(lock, devname, sizeof(devname), DN_DEVICEONLY);
		if (result)
			commandline = ASPrintf("%s DRIVE %s REQ", command, devname);
		UnLock(lock);
	}

	if (!commandline)
		if (!(commandline = ASPrintf("%s REQ", command, devname)))
			return 0;

	result = GetScreenAttr(screen, SA_PubName, &scrname, sizeof(scrname));
	if (result)
	{
		GetDefaultPubScreen(oldname);
		SetDefaultPubScreen(scrname);
		modes = SetPubScreenModes(SHANGHAI | POPPUBSCREEN);
	}

	result = SystemTags(commandline,
						SYS_Input,
						0,
						SYS_Output,
						0,
						SYS_Error,
						0,
						SYS_Asynch,
						TRUE,
						NP_StackSize,
						16384,
						NP_Priority,
						0,
						NP_Cli,
						TRUE,
						TAG_DONE,
						NULL);

	Delay(5);
	if (modes)
		SetPubScreenModes(modes);
	if (strlen(oldname) > 1)
		SetDefaultPubScreen(oldname);
	else
		SetDefaultPubScreen(NULL);

	FreeVec(commandline);
	return 1;
}
#endif
