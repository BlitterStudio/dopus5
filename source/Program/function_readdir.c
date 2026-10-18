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
#include <libraries/multiuser.h>
#include <proto/multiuser.h>

int read_dir(DirBuffer *, Lister *, FunctionHandle *, BPTR, struct InfoData *, ReselectionData *);
void readdir_check_format(Lister *lister, char *path, ListFormat *, BOOL);

#define PATH_FULL_DEVICE handle->work_buffer
#define PATH_FULL_NAME handle->work_buffer + 768

// Buffer of the panel being read (side is 1-based, 0 = normal lister)
static DirBuffer *function_read_buffer(Lister *lister, ULONG side)
{
	return (side) ? lister_dual_side_buffer(lister, side - 1) : lister->cur_buffer;
}

// Sets up and reads a new directory. side is the 1-based dual panel to read
// into, 0 for a normal lister or the active panel.
// Called from FUNCTION PROCESS
void function_read_directory(FunctionHandle *handle, Lister *lister, ULONG side, char *source_path)
{
	ReselectionData reselect;
	BPTR lock;
	short ret;
	BOOL same = 0, noread = 0;
	struct DateStamp stamp, *stamp_ptr = 0;
	D_S(struct InfoData, info)
	ListFormat oldformat;
	char *ptr;
	DirBuffer *buffer;
	ULONG disktype = 0;
	ListerReselectData *packet;
	ListerBufferFindData *find_packet;

	// Resolve the panel now; the active panel may change while we read
	if (!side && lister_dual_active_index(lister) >= 0)
		side = lister_dual_active_index(lister) + 1;

	// If we're currently displaying a special buffer, return to a normal one
	IPC_Command(lister->ipc, LISTER_CHECK_SPECIAL_BUFFER, side, 0, 0, REPLY_NO_PORT);

	// Get last character of path, strip if it's a /
	ptr = source_path + strlen(source_path) - 1;
	if (*ptr == '/')
		*ptr = 0;

	// Lock path to read
	if ((lock = Lock(source_path, ACCESS_READ)))
	{
		struct DosList *doslist;

		// Get full paths
		NameFromLock(lock, PATH_FULL_NAME, 256);
		DevNameFromLockDopus(lock, PATH_FULL_DEVICE, 256);

		// Get disk information
		Info(lock, info);

		// Get dos list pointer and hence disk datestamp
		if ((doslist = (struct DosList *)BADDR(info->id_VolumeNode)))
		{
			// Get date stamp
			stamp = doslist->dol_misc.dol_volume.dol_VolumeDate;
			stamp_ptr = &stamp;

			// Get disk type
			disktype = doslist->dol_misc.dol_volume.dol_DiskType;
		}

		// No disktype?
		if (!disktype)
			disktype = info->id_DiskType;
	}

	// Couldn't lock
	else
	{
		strcpy(PATH_FULL_NAME, source_path);
		strcpy(PATH_FULL_DEVICE, source_path);
	}

	// Make sure paths are terminated
	AddPart(PATH_FULL_DEVICE, "", 256);
	AddPart(PATH_FULL_NAME, "", 256);

	// Store current list format
	oldformat = function_read_buffer(lister, side)->buf_ListFormat;

	// Search for this path in another buffer?
	if (handle->flags & GETDIRF_CANCHECKBUFS)
	{
		DirBuffer *buffer;
		/*
				char *ptr;

				// Get volume name
				if (ptr=strchr(PATH_FULL_NAME,':'))
				{
					char *volume;

					// Clear colon temporarily
					*ptr=0;

					// Allocate memory for volume name
					if (volume=AllocVec(strlen(PATH_FULL_NAME)+1,MEMF_CLEAR))
					{
						// Copy the volume name
						strcpy(volume,PATH_FULL_NAME);
					}

					// Restore the colon
					*ptr=':';

					// Save pointer to name
					ptr=volume;
				}
		*/

		// Look for path
		buffer = 0;
		if ((find_packet = AllocVec(sizeof(ListerBufferFindData), MEMF_CLEAR)))
		{
			find_packet->path = PATH_FULL_DEVICE;
			find_packet->stamp = stamp_ptr;
			find_packet->side = side;

			buffer = (DirBuffer *)IPC_Command(
				lister->ipc, LISTER_BUFFER_FIND, 0, find_packet, find_packet, REPLY_NO_PORT);
		}
		else
			buffer = (DirBuffer *)IPC_Command(
				lister->ipc, LISTER_BUFFER_FIND, (IPTR)stamp_ptr, (APTR)(IPTR)PATH_FULL_DEVICE, 0, REPLY_NO_PORT);

		if (buffer)
		{
			// Found it
			UnLock(lock);

			// Check for special sort format
			readdir_check_format(lister, PATH_FULL_NAME, &oldformat, same);

			// Store format
			lister->format = oldformat;

			// Reset some flags in the buffer
			buffer->more_flags &= ~DWF_LOCK_STATE;

			// Show buffer in lister
			IPC_Command(lister->ipc, LISTER_SHOW_BUFFER, LISTER_SHOW_BUFFER_PACK_FLAGS(TRUE, side), buffer, 0, REPLY_NO_PORT);
			noread = 1;
		}
	}

	// Still reading?
	if (!noread)
	{
		// Empty buffer? A panel never re-uses a buffer the other panel shows
		// for another directory
		buffer = function_read_buffer(lister, side);
		if (handle->flags & GETDIRF_CANMOVEEMPTY ||
			(side && buffer == function_read_buffer(lister, (side == 1) ? 2 : 1) &&
			 stricmp(buffer->buf_Path, PATH_FULL_DEVICE)))
		{
			if ((find_packet = AllocVec(sizeof(ListerBufferFindData), MEMF_CLEAR)))
			{
				find_packet->path = PATH_FULL_DEVICE;
				find_packet->stamp = stamp_ptr;
				find_packet->side = side;

				IPC_Command(lister->ipc, LISTER_BUFFER_FIND_EMPTY, 0, find_packet, find_packet, REPLY_NO_PORT);
			}
			else
				IPC_Command(lister->ipc,
							LISTER_BUFFER_FIND_EMPTY,
							(IPTR)stamp_ptr,
							(APTR)(IPTR)PATH_FULL_DEVICE,
							0,
							REPLY_NO_PORT);
		}

		// Get buffer
		buffer = function_read_buffer(lister, side);

		// Same path?
		if (stricmp(buffer->buf_Path, PATH_FULL_DEVICE) == 0)
			same = 1;

		// Store paths
		strcpy(buffer->buf_Path, PATH_FULL_DEVICE);
		strcpy(buffer->buf_ExpandedPath, PATH_FULL_NAME);

		// Refresh path field
		IPC_Command(lister->ipc, LISTER_REFRESH_PATH, side, 0, 0, 0);

		// Display text
		status_text(lister, GetString(&locale, MSG_READING_DIRECTORY));

		// Lock buffer
		buffer_lock(buffer, TRUE);

		// Need to do reslection?
		if (handle->flags & GETDIRF_RESELECT)
		{
			InitReselect(&reselect);
			if ((packet = AllocVec(sizeof(ListerReselectData), MEMF_CLEAR)))
			{
				packet->reselect = &reselect;
				packet->flags = RESELF_SAVE_FILETYPES;
				packet->side = side;

				if (!IPC_Command(lister->ipc, LISTER_MAKE_RESELECT, 0, packet, packet, REPLY_NO_PORT))
					FreeVec(packet);
			}
			else
				IPC_Command(lister->ipc, LISTER_MAKE_RESELECT, RESELF_SAVE_FILETYPES, (APTR)&reselect, 0, REPLY_NO_PORT);
		}

		// Free buffer
		buffer_freedir(buffer, 0); /******* was 1 ******/

		// Default to previous format
		buffer->buf_ListFormat = oldformat;

		// Unlock buffer
		buffer_unlock(buffer);

		// Refresh window display
		IPC_Command(lister->ipc,
					LISTER_REFRESH_WINDOW,
					REFRESHF_UPDATE_NAME | REFRESHF_STATUS | REFRESHF_SLIDERS | REFRESHF_CLEAR_ICONS,
					(APTR)(IPTR)side,
					0,
					REPLY_NO_PORT);

		// Invalid directory?
		if (!lock)
		{
			// Display error message
			function_error_text(handle, -1);
			ret = 0;
		}

		// Otherwise
		else
		{
			// Lock buffer
			buffer_lock(buffer, TRUE);

			// Check format
			readdir_check_format(lister, PATH_FULL_NAME, &oldformat, same);

			// Store format
			buffer->buf_ListFormat = oldformat;

			/**************/
			lister->format = oldformat;
			/**************/

			// Save disk type
			buffer->buf_DiskType = disktype;

			// Get volume name
			stccpy(buffer->buf_VolumeLabel, buffer->buf_ExpandedPath, 32);
			if ((ptr = strchr(buffer->buf_VolumeLabel, ':')))
				*ptr = 0;

			// Unlock buffer
			buffer_unlock(buffer);

			// Read directory
			ret = read_dir(buffer, lister, handle, lock, info, (handle->flags & GETDIRF_RESELECT) ? &reselect : 0);
		}

		// Unlock directory
		UnLock(lock);

		// Store disk stamp
		if (stamp_ptr)
			buffer->buf_VolumeDate = *stamp_ptr;

		// Do reselection
		if (handle->flags & GETDIRF_RESELECT)
		{
			if (ret)
			{
				if ((packet = AllocVec(sizeof(ListerReselectData), MEMF_CLEAR)))
				{
					packet->reselect = &reselect;
					packet->flags = 0;
					packet->side = side;

					if (!IPC_Command(lister->ipc, LISTER_DO_RESELECT, 0, packet, packet, REPLY_NO_PORT))
						FreeVec(packet);
				}
				else
					IPC_Command(lister->ipc, LISTER_DO_RESELECT, 0, (APTR)&reselect, 0, REPLY_NO_PORT);
			}
			FreeReselect(&reselect);
		}

		// Add to history list
		lister_add_history(lister, buffer);
	}

	// Refresh window display if read was successful
	if (lock)
	{
		short mode;

		// Refresh
		IPC_Command(lister->ipc,
					LISTER_REFRESH_WINDOW,
					REFRESHF_UPDATE_NAME | REFRESHF_STATUS | REFRESHF_SLIDERS | REFRESHF_CD,
					(APTR)(IPTR)side,
					0,
					0);

		// Check space gauge
		IPC_Command(lister->ipc, LISTER_SET_GAUGE, side, 0, 0, 0);

		// Get icons if necessary
		if (lister->flags & (LISTERF_VIEW_ICONS | LISTERF_ICON_ACTION))
			lister_get_icons(handle, lister, 0, GETICONSF_SHOW);

		// Initial mode
		mode = (lister->flags & LISTERF_VIEW_ICONS) ? 0 : LISTERMODE_ICON;

		// Get position entry
		if (!(GetListerPosition(PATH_FULL_NAME, 0, 0, &lister->other_dims, &mode, 0, 0, 0, GLPF_USE_MODE)))
		{
			// No entry; use same dimensions as current
			if (lister_valid_window(lister))
				lister->other_dims = *((struct IBox *)&lister->window->LeftEdge);
		}
	}
}

// Read a directory into a buffer
// Called from FUNCTION PROCESS
int read_dir(DirBuffer *buffer,
			 Lister *lister,
			 FunctionHandle *handle,
			 BPTR lock,
			 struct InfoData *info,
			 ReselectionData *reselect)
{
	long file_count = 0, dir_count = 0;
	D_S(struct FileInfoBlock, fileinfo)
	BPTR parent;
	DirEntry *entry;
	struct MinList file_list;
	struct DevProc *proc;
	NetworkInfo network, *network_ptr = 0;
	short sniff = SNIFFF_NO_FILETYPES;
	short item;

	// Go through display items
	for (item = 0; item < DISPLAY_LAST; item++)
	{
		// Do we want to show network stuff?
		if (buffer->buf_ListFormat.display_pos[item] == DISPLAY_OWNER ||
			buffer->buf_ListFormat.display_pos[item] == DISPLAY_GROUP ||
			buffer->buf_ListFormat.display_pos[item] == DISPLAY_NETPROT)
		{
			// Yes
			network_ptr = &network;
		}

		// Or show filetypes?
		else if (buffer->buf_ListFormat.display_pos[item] == DISPLAY_FILETYPE)
			sniff &= ~SNIFFF_NO_FILETYPES;

		// Or show versions?
		else if (buffer->buf_ListFormat.display_pos[item] == DISPLAY_VERSION)
			sniff |= SNIFFF_VERSION;
	}

	// Examine this object
#ifdef USE_64BIT
	if (!ExamineLock64(lock, (FileInfoBlock64 *)fileinfo))
#else
	if (!Examine(lock, fileinfo))
#endif
		return 0;

	// If object is a file, fail
	if (fileinfo->fib_DirEntryType < 0)
	{
		function_error_text(handle, ERROR_OBJECT_WRONG_TYPE);
		return 0;
	}

	// Get device process
	proc = GetDeviceProc(buffer->buf_Path, 0);

	// Store directory datestamp
	buffer->buf_DirectoryDate = fileinfo->fib_Date;

	// Set reading flag
	buffer->flags |= DWF_READING;

	// Is this a root?
	if (!(parent = ParentDir(lock)))
		buffer->flags |= DWF_ROOT;
	else
		UnLock(parent);

	// Save object name
	if (fileinfo->fib_FileName[0])
		strcpy(buffer->buf_ObjectName, fileinfo->fib_FileName);
	else
	{
		if (buffer->flags & DWF_ROOT)
		{
			char *ptr;

			stccpy(buffer->buf_ObjectName, buffer->buf_ExpandedPath, GUI->def_filename_length - 1);
			if ((ptr = strchr(buffer->buf_ObjectName, ':')))
				*ptr = 0;
		}
		else
			stccpy(buffer->buf_ObjectName, FilePart(buffer->buf_ExpandedPath), GUI->def_filename_length - 1);
	}

	// Initialise list
	NewList((struct List *)&file_list);

	// Loop until directory is empty
#ifdef USE_64BIT
	while (ExamineNext64(lock, (FileInfoBlock64 *)fileinfo))
#else
	while (ExNext(lock, fileinfo))
#endif
	{
		BOOL ok = 1;

		// Check abort
		if (function_check_abort(handle))
		{
			buffer->flags |= DWF_ABORTED;
			break;
		}

		// Ok to add?
		if (ok)
		{
			// Get network info if needed
			if (network_ptr)
			{
				network_get_info(network_ptr,
								 buffer,
								 proc,
								 fileinfo->fib_OwnerUID,
								 fileinfo->fib_OwnerGID,
								 fileinfo->fib_Protection);
			}

			// Directories are meant to have no size, apparently
			if (fileinfo->fib_DirEntryType > 0)
				GETFIBSIZE(fileinfo) = 0;

			// Create entry
			if ((entry = create_file_entry_fib(buffer, lock, fileinfo, 0, 0, 0, network_ptr)))
			{
				// Add to list
				AddTail((struct List *)&file_list, (struct Node *)entry);

				// Increment counts
				if (ENTRYTYPE(entry->de_Node.dn_Type) == ENTRY_DIRECTORY)
					++dir_count;
				else
					++file_count;
			}

			// Failed
			else
			{
				function_error_text(handle, -1);
				break;
			}
		}
	}

	// Free device process
	FreeDeviceProc(proc);

	// Reselection list?
	if (reselect && reselect->flags & RESELF_SAVE_FILETYPES)
		GetReselectFiletypes(reselect, &file_list);

	// Lock buffer
	buffer_lock(buffer, TRUE);

	// Sort directory
	buffer_sort_list(buffer, &file_list, file_count, dir_count);

	// Unlock buffer
	buffer_unlock(buffer);

	// Get things?
	if (sniff != SNIFFF_NO_FILETYPES)
		filetype_find_typelist(lister, buffer, sniff);

	// Clear reading flag, set valid flag
	buffer->flags &= ~DWF_READING;
	buffer->flags |= DWF_VALID;

	return 1;
}

// Get network information for a file
void network_get_info(NetworkInfo *network,
					  DirBuffer *buffer,
					  struct DevProc *proc,
					  UWORD owner,
					  UWORD group,
					  ULONG protection)
{
	char *ptr;
	short a, b;

	// Initialise network info
	network->owner_id = owner;
	network->group_id = group;
	network->owner[0] = 0;
	network->group[0] = 0;

	// Valid owner?
	if (owner)
	{
		// If owner ID matches the last one, copy that name
		if (buffer->last_owner == owner)
			stccpy(network->owner, buffer->owner_name, sizeof(network->owner));

		// Try sending packet
		else if (buffer->user_info &&
				 (DoPkt(proc->dvp_Port, ACTION_UID_TO_USERINFO, owner, (IPTR)buffer->user_info, 0, 0, 0)))
		{
			// Store owner for next time
			buffer->last_owner = owner;
			strcpy(buffer->owner_name, buffer->user_info->ui_UserName);

			// Copy name
			stccpy(network->owner, buffer->user_info->ui_UserName, sizeof(network->owner));
		}

		// MUFS maybe?
		else if (buffer->mu_user_info && buffer->buf_DiskType != ID_ENVOY_DISK)
		{
			// Owner by someone?
			if (owner != muNOBODY_UID)
			{
				// Try for info
				buffer->mu_user_info->uid = owner;
				if (muGetUserInfo(buffer->mu_user_info, muKeyType_uid))
				{
					// Store owner for next time
					buffer->last_owner = owner;
					strcpy(buffer->owner_name, buffer->mu_user_info->UserID);

					// Copy name
					stccpy(network->owner, buffer->mu_user_info->UserID, sizeof(network->owner));
				}
			}
			else
				owner = 0;
		}

		// No owner yet?
		if (!*network->owner)
		{
			// Build number string
			lsprintf(network->owner, "%ld", owner);
		}
	}

	// Valid group?
	if (group)
	{
		// If group ID matches the last one, copy that name
		if (buffer->last_group == group)
			stccpy(network->group, buffer->group_name, sizeof(network->group));

		// Try sending packet
		else if (buffer->group_info &&
				 (DoPkt(proc->dvp_Port, ACTION_GID_TO_GROUPINFO, group, (IPTR)buffer->group_info, 0, 0, 0)))
		{
			// Store owner for next time
			buffer->last_group = group;
			strcpy(buffer->group_name, buffer->group_info->gi_GroupName);

			// Copy name
			stccpy(network->group, buffer->group_info->gi_GroupName, sizeof(network->group));
		}

		// MUFS maybe?
		else if (buffer->mu_group_info && buffer->buf_DiskType != ID_ENVOY_DISK)
		{
			// Try for info
			buffer->mu_group_info->gid = group;
			if (muGetGroupInfo(buffer->mu_group_info, muKeyType_gid))
			{
				// Store group for next time
				buffer->last_group = group;
				strcpy(buffer->group_name, buffer->mu_group_info->GroupID);

				// Copy name
				stccpy(network->group, buffer->mu_group_info->GroupID, sizeof(network->group));
			}
		}

		// No group yet?
		if (!*network->group)
		{
			// Build number string
			lsprintf(network->group, "%ld", group);
		}
	}

	// Get pointer to protection bits buffer
	ptr = network->net_protbuf;

	// Build group protection bits
	for (a = FIBB_GRP_READ, b = 0; a >= FIBB_GRP_DELETE; a--, b++)
	{
		if (!(protection & (1 << a)))
			*ptr++ = '-';
		else
			*ptr++ = ((char *)"rwed")[b];
	}

	// Build other protection bits
	for (a = FIBB_OTR_READ, b = 0; a >= FIBB_OTR_DELETE; a--, b++)
	{
		if (!(protection & (1 << a)))
			*ptr++ = '-';
		else
			*ptr++ = ((char *)"rwed")[b];
	}

	// Null-terminate
	*ptr = 0;
}

// Check sort format
void readdir_check_format(Lister *lister, char *path, ListFormat *format, BOOL same)
{
	position_rec *pos;

	// Is format locked, or this is a rescan?
	if (lister->more_flags & LISTERF_LOCK_FORMAT || same)
	{
		// Use current format
		*format = lister->format;
	}

	// See if position is supplied
	else if (!(pos = GetListerPosition(path, 0, 0, 0, 0, format, 0, 0, 0)) || !(pos->flags & POSITIONF_FORMAT))
	{
		// Use current format from lister
		*format = lister->format;

		// Gauge flag is preserved from lister->format as-is (already copied above)
	}

	// Is the lister in icon mode?
	if (lister->flags & LISTERF_VIEW_ICONS)
	{
		// Filter icons on?
		if (format->flags & LFORMATF_REJECT_ICONS)
		{
			// Turn it off temporarily
			format->flags &= ~LFORMATF_REJECT_ICONS;
			format->flags |= LFORMATF_TEMP_ICONS;
		}
	}
}
