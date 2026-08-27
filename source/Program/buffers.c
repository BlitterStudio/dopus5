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

// Allocates a new buffer
DirBuffer *buffer_new(void)
{
	DirBuffer *buffer;

	// Allocate buffer and memory pool
	if (!(buffer = AllocVec(sizeof(DirBuffer), MEMF_CLEAR)) ||
		!(buffer->memory = NewMemHandle(4096, 1024, MEMF_CLEAR | MEMF_PUBLIC)))
	{
		FreeVec(buffer);
		return 0;
	}

	// Initialise buffer
	NewList((struct List *)&buffer->entry_list);
	NewList((struct List *)&buffer->reject_list);
	InitSemaphore(&buffer->semaphore);
	buffer->name_field_size = GUI->def_filename_length;
	buffer->comment_field_size = 80;
	buffer->buf_ListFormat = environment->env->list_format;
	if (!(buffer->buf_ObjectName = AllocVec(256, MEMF_CLEAR)))
	{
		buffer_free(buffer);
		return 0;
	}

	// Allocate networking things
	buffer->user_info = AllocVec(sizeof(struct UserInfo) * 2, MEMF_CLEAR);
	buffer->group_info = AllocVec(sizeof(struct GroupInfo) * 2, MEMF_CLEAR);
	if (muBase && environment->env->settings.general_flags & GENERALF_ENABLE_MUFS)
	{
		buffer->mu_user_info = muAllocUserInfo();
		buffer->mu_group_info = muAllocGroupInfo();
	}
	return buffer;
}

// Free a buffer
void buffer_free(DirBuffer *buffer)
{
	// Valid buffer?
	if (buffer)
	{
		// Free networking things
		FreeVec(buffer->user_info);
		FreeVec(buffer->group_info);
		if (muBase)
		{
			if (buffer->mu_user_info)
				muFreeUserInfo(buffer->mu_user_info);
			if (buffer->mu_group_info)
				muFreeGroupInfo(buffer->mu_group_info);
		}

		// Free buffer memory
		FreeMemHandle(buffer->memory);
		FreeVec(buffer->buf_ObjectName);
		FreeVec(buffer);
	}
}

// Free the contents of a buffer
BOOL buffer_freedir(DirBuffer *buffer, BOOL clear_handler)
{
	short a;

	// Valid buffer?
	if (!buffer)
		return 1;

	// Clear buffer memory
	ClearMemHandle(buffer->memory);

	// Reset buffer
	NewList((struct List *)&buffer->entry_list);
	NewList((struct List *)&buffer->reject_list);
	buffer->first_file = 0;
	buffer->first_dir = 0;
	buffer->buf_OldVertOffset = -1;
	buffer->buf_OldHorizOffset = -1;
	buffer->buf_VertOffset = 0;
	buffer->buf_TotalEntries[0] = 0;
	buffer->buf_SelectedFiles[0] = 0;
	buffer->buf_SelectedDirs[0] = 0;
	buffer->buf_SelectedBytes[0] = 0;
	buffer->buf_TotalBytes[0] = 0;
	buffer->buf_TotalDirs[0] = 0;
	buffer->buf_TotalFiles[0] = 0;
	buffer->buf_HorizOffset = 0;
	buffer->buf_FreeDiskSpace = 0;
	buffer->buf_TotalDiskSpace = 0;
	buffer->buf_VolumeLabel[0] = 0;
#ifdef USE_64BIT
	buffer->buf_FreeDiskSpace64 = 0;
	buffer->buf_TotalDiskSpace64 = 0;
#endif
	buffer->flags = 0;
	buffer->name_length = 0;
	buffer->date_length = 0;
	buffer->comment_length = 0;
	buffer->version_length = 0;
	buffer->type_length = 0;
	buffer->owner_length = 0;
	buffer->group_length = 0;
	buffer->buf_VolumeDate.ds_Days = 0;
	buffer->buf_VolumeDate.ds_Minute = 0;
	buffer->buf_VolumeDate.ds_Tick = 0;
	buffer->buf_DiskType = 0;
	buffer->last_owner = 0;
	buffer->last_group = 0;
	buffer->last_status[0] = 0;

	// Clear custom field titles
	for (a = 0; a < 10; a++)
		buffer->buf_TitleFields[a] = 0;

	// Reset field widths
	for (a = 0; a < DISPLAY_LAST; a++)
		if (!(buffer->buf_CustomWidthFlags & (1 << a)))
			buffer->buf_FieldWidth[a] = 0;

	// Clear handler?
	if (clear_handler)
	{
		// Clear custom title and label
		buffer->buf_CustomTitle[0] = 0;
		buffer->buf_CustomLabel[0] = 0;
		buffer->buf_CustomHeader[0] = 0;

		// Send inactive message
		if (buffer->buf_CustomHandler[0])
			buffer_inactive(buffer, 0);

		// Clear handler pointer
		buffer->buf_CustomHandler[0] = 0;
	}

	// Is there a lister showing this buffer?
	if (buffer->buf_CurrentLister)
	{
		// Get format from lister
		buffer->buf_ListFormat = buffer->buf_CurrentLister->format;
	}

	// Otherwise get default format
	else
	{
		buffer->buf_ListFormat = environment->env->list_format;
	}

	return 1;
}

// Create a new entry for a directory buffer
DirEntry *create_file_entry(DirBuffer *buffer,
							BPTR lock,
							char *entry_name,
#ifdef USE_64BIT
							UQUAD entry_size,
#else
							unsigned long entry_size,
#endif
							short entry_type,
							struct DateStamp *entry_date,
							char *entry_comment,
							ULONG entry_prot,
							short entry_subtype,
							char *display_string,
							char *description,
							NetworkInfo *network_info)
{
	DirEntry *newentry;
	unsigned short size;
	D_S(SoftLinkInfo, sinfo)
	BOOL softlink = 0;

	// Valid buffer?
	if (!buffer)
		return 0;

	// Is entry a soft link?
	if (entry_type == ST_SOFTLINK)
	{
		// Get soft-link info
		if (ReadSoftLinkDopus(lock, buffer->buf_Path, entry_name, sinfo))
		{
			// Get info from soft link
			entry_type = sinfo->sli_Fib.fib_DirEntryType;
			entry_date = &sinfo->sli_Fib.fib_Date;
			entry_comment = (char *)sinfo->sli_Fib.fib_Comment;
			if (entry_type == ST_FILE)
			{
				entry_size = GETFIBSIZE(&sinfo->sli_Fib);
			}
		}
		else
		{
			entry_size = 0;
			entry_type = ST_LINKFILE;
			entry_comment = "UNRESOLVED LINK";
		}
		softlink = 1;
	}

	// Calculate size of entry allocation
	size = sizeof(DirEntry) + buffer->name_field_size;

	// Allocate entry memory
	if (!(newentry = AllocMemH(buffer->memory, size)))
		return 0;

	// Name field
	newentry->de_NameLen = buffer->name_field_size;
	newentry->de_Node.dn_Name = (char *)(newentry + 1);

	// Copy name
	if (entry_name)
		stccpy(newentry->de_Node.dn_Name, entry_name, newentry->de_NameLen);

	// Store size, type and owner/group IDs
	newentry->de_Size = entry_size;
	newentry->de_Node.dn_Type = entry_type;
	newentry->de_SubType = entry_subtype;

	// If entry is a directory with no size, set 'no size' flag
	if (entry_type >= ENTRY_DIRECTORY && entry_size == 0)
		newentry->de_Flags |= ENTF_NO_SIZE;

	// Add comment
	if (entry_comment && *entry_comment)
		direntry_add_string(buffer, newentry, DE_Comment, entry_comment);

	// Add display string
	if (display_string && *display_string)
		direntry_add_string(buffer, newentry, DE_DisplayString, display_string);

	// Add description
	if (description && *description)
		direntry_add_string(buffer, newentry, DE_Filetype, description);

	// Don't need this stuff for devices
	if (entry_type != ENTRY_DEVICE)
	{
		// Store date (if supplied)
		if (entry_date)
			newentry->de_Date = *entry_date;

		// Otherwise get current date
		else
			DateStamp(&newentry->de_Date);

		// Get date string
		date_build_string(&newentry->de_Date, newentry->de_DateBuf, 1);

		// Store protection
		newentry->de_Protection = entry_prot;
		protect_get_string(entry_prot, newentry->de_ProtBuf);

		// Soft link?
		if (softlink)
			newentry->de_Flags |= ENTF_LINK | ENTF_SOFTLINK;

		// Hard link?
		else if (entry_type == ST_LINKDIR || entry_type == ST_LINKFILE)
			newentry->de_Flags |= ENTF_LINK;
	}

	// Network info?
	if (network_info)
	{
		// Add network tag
		direntry_add_network(buffer, newentry, network_info);
	}

	// Entry starts out as not selected
	newentry->de_Flags &= ~ENTF_SELECTED;

	// Return the new entry
	return newentry;
}

// Create a new entry for a directory buffer from a FileInfoBlock
DirEntry *create_file_entry_fib(DirBuffer *buffer,
								BPTR lock,
								struct FileInfoBlock *fib,
								short entry_subtype,
								char *display_string,
								char *description,
								NetworkInfo *network_info)
{
	return create_file_entry(buffer,
							 lock,
							 fib->fib_FileName,
							 GETFIBSIZE(fib),
							 fib->fib_DirEntryType,
							 &fib->fib_Date,
							 fib->fib_Comment,
							 fib->fib_Protection,
							 entry_subtype,
							 display_string,
							 description,
							 network_info);
}

// Copy a file entry
DirEntry *copy_file_entry(DirBuffer *buffer, DirEntry *entry)
{
	DirEntry *newentry;
	unsigned short size;
	VersionInfo *version;

	// Calculate size of entry allocation
	size = sizeof(DirEntry) + buffer->name_field_size;

	// Allocate entry memory
	if (!(newentry = AllocMemH(buffer->memory, size)))
		return 0;

	// Fix name pointer
	newentry->de_NameLen = buffer->name_field_size;
	newentry->de_Node.dn_Name = (char *)(newentry + 1);

	// Copy entry data
	strcpy(newentry->de_Node.dn_Name, entry->de_Node.dn_Name);
	newentry->de_Node.dn_Type = entry->de_Node.dn_Type;
	newentry->de_SubType = entry->de_SubType;
	newentry->de_Size = entry->de_Size;
	newentry->de_Protection = entry->de_Protection;
	newentry->de_Flags = entry->de_Flags;
	newentry->de_Date = entry->de_Date;
	strcpy(newentry->de_ProtBuf, entry->de_ProtBuf);
	strcpy(newentry->de_DateBuf, entry->de_DateBuf);

	// Clear some important flags
	newentry->de_Flags &= ~(ENTF_SELECTED | ENTF_SKIPPED | ENTF_SKIP | ENTF_REJECTED | ENTF_NETWORK | ENTF_VERSION);

	// Copy version info
	if ((version = (VersionInfo *)GetTagData(DE_VersionInfo, 0, entry->de_Tags)))
		direntry_add_version(buffer, newentry, version->vi_Version, version->vi_Revision, version->vi_Days);

	// Copy comment, display string and filetype
	direntry_add_string(buffer, newentry, DE_Comment, (char *)GetTagData(DE_Comment, 0, entry->de_Tags));
	direntry_add_string(buffer, newentry, DE_DisplayString, (char *)GetTagData(DE_DisplayString, 0, entry->de_Tags));
	direntry_add_string(buffer, newentry, DE_Filetype, (char *)GetTagData(DE_Filetype, 0, entry->de_Tags));

	// Network info?
	if (entry->de_Flags & ENTF_NETWORK)
	{
		NetworkInfo *network;

		// Find NetworkInfo
		if ((network = (NetworkInfo *)GetTagData(DE_NetworkInfo, 0, entry->de_Tags)))
		{
			// Add network tag
			direntry_add_network(buffer, newentry, network);
		}
	}

	// Return the new entry
	return newentry;
}

// Add an entry to a list
DirEntry *add_file_entry(DirBuffer *buffer, DirEntry *newentry, DirEntry *add_position)
{
	DirEntry *position;	 // Gives position to add new entry
	DirEntry *first;	 // First entry to work from
	NetworkInfo *network = 0;
	ListFormat *format;
	char sort_reverse = 0;
	BYTE sort_method, desired_sort_method;
	char check_sep = 0;
	short a;

	// Valid file?
	if (!newentry)
		return 0;

	// Not a device entry
	if (newentry->de_Node.dn_Type != ENTRY_DEVICE)
	{
		// See if file is already in list
		if ((first = find_entry(&buffer->entry_list, newentry->de_Node.dn_Name, 0, buffer->more_flags & DWF_CASE)) ||
			(first = find_entry(&buffer->reject_list, newentry->de_Node.dn_Name, 0, buffer->more_flags & DWF_CASE)))
		{
			// Remove entry
			removefile(buffer, first);
		}

		// See if file is rejected
		if (buffer_test_file(newentry, buffer))
		{
			// Add to reject list
			AddHead((struct List *)&buffer->reject_list, (struct Node *)newentry);
			newentry->de_Flags |= ENTF_REJECTED;
			return newentry;
		}
	}

	// Get sort format
	format = &buffer->buf_ListFormat;

	// See if reverse sorting is set for this window
	if (format->sort.sort_flags & SORT_REVERSE)
		sort_reverse = 1;

	// Get network information if any
	if (newentry->de_Flags & ENTF_NETWORK)
		network = (NetworkInfo *)GetTagData(DE_NetworkInfo, 0, newentry->de_Tags);

	// Get desired sort method
	desired_sort_method = format->sort.sort;
	if (desired_sort_method < 0 || desired_sort_method >= DISPLAY_LAST)
		desired_sort_method = DISPLAY_NAME;

	// Sort method for device entries
	if (newentry->de_Node.dn_Type == ENTRY_DEVICE)
	{
		// Buffers sorted by display string
		if (newentry->de_SubType == SUBENTRY_BUFFER)
			sort_method = -2;

		// Volumes also sorted by display string
		else if (newentry->de_Size == DLT_DEVICE)
			sort_method = -2;

		// Otherwise sort by name
		else
			sort_method = DISPLAY_NAME;
	}

	// Directories
	else if (newentry->de_Node.dn_Type >= ENTRY_DIRECTORY)
	{
		// Look at desired method
		switch (desired_sort_method)
		{
		// Size/Filetype are invalid
		case DISPLAY_SIZE:
		case DISPLAY_FILETYPE:
			sort_method = DISPLAY_NAME;
			break;

		// Network stuff, only if Envoy is present
		case DISPLAY_OWNER:
		case DISPLAY_GROUP:
		case DISPLAY_NETPROT:
			if (!network)
			{
				sort_method = DISPLAY_NAME;
				break;
			}

		// Use requested method
		default:
			sort_method = desired_sort_method;
			break;
		}
	}

	// Files
	else
	{
		// Network stuff, only if Envoy is present
		if (!network && (desired_sort_method == DISPLAY_OWNER || desired_sort_method == DISPLAY_GROUP ||
						 desired_sort_method == DISPLAY_NETPROT))
			sort_method = DISPLAY_NAME;

		// Otherwise, use requested method
		else
			sort_method = desired_sort_method;
	}

	// Initialise pointers
	first = 0;
	position = 0;

	// Is a position supplied?
	if (add_position)
	{
		// Start after the supplied position
		first = (DirEntry *)add_position->de_Node.dn_Succ;

		// If that's invalid, start with the supplied position
		if (!first->de_Node.dn_Succ)
		{
			first = add_position;
			position = add_position;
		}
	}

	// No supplied position
	else
	{
		// Look at the separation method for this window
		switch (format->sort.separation)
		{
		// Directories listed first
		case SEPARATE_DIRSFIRST:

			// Make sure buffer isn't empty
			if (!(IsListEmpty((struct List *)&buffer->entry_list)))
			{
				// Are we adding a file?
				if (newentry->de_Node.dn_Type <= ENTRY_FILE)
				{
					// If so, start at first file
					first = buffer->first_file;

					// If that's invalid, start at end of buffer
					if (!first || !first->de_Node.dn_Succ)
					{
						first = (DirEntry *)buffer->entry_list.mlh_TailPred;
						position = first;
					}
				}

				// Adding a directory, start at top of buffer
				else
					first = (DirEntry *)buffer->entry_list.mlh_Head;
			}
			check_sep = 1;
			break;

		// Files listed first
		case SEPARATE_FILESFIRST:

			// Make sure buffer isn't empty
			if (!(IsListEmpty((struct List *)&buffer->entry_list)))
			{
				// Are we adding a directory?
				if (newentry->de_Node.dn_Type >= ENTRY_DEVICE)
				{
					// If so, start at first directory
					first = buffer->first_dir;

					// If that's invalid, start at end of buffer
					if (!first || !first->de_Node.dn_Succ)
					{
						first = (DirEntry *)buffer->entry_list.mlh_TailPred;
						position = first;
					}
				}

				// Adding a file, start at top of buffer
				else
					first = (DirEntry *)buffer->entry_list.mlh_Head;
			}
			check_sep = 1;
			break;

		// Mixed, start at top of buffer
		default:
			first = (DirEntry *)buffer->entry_list.mlh_Head;
			break;
		}
	}

	// If we have a starting entry and no actual position, we need to sort
	if (first && first->de_Node.dn_Succ && !position)
	{
		DirEntry *entry;
		int break_flag = 0;

		// Go through entries
		for (entry = first; entry->de_Node.dn_Succ; entry = (DirEntry *)entry->de_Node.dn_Succ)
		{
			long cmp_val = 0;

			// Check for multi-path assigns
			if (entry->de_Node.dn_Type == ENTRY_DEVICE)
			{
				// Is this entry a multi-path assign?
				if (entry->de_SubType == SUBENTRY_BUFFER && newentry->de_SubType != SUBENTRY_BUFFER)
				{
					// Skip this entry for sorting
					continue;
				}
			}

			// Check separation method?
			if (check_sep)
			{
				// Look at method
				switch (format->sort.separation)
				{
				// Directories first
				case SEPARATE_DIRSFIRST:

					// If adding a directory, and this entry is a file
					if (entry->de_Node.dn_Type <= ENTRY_FILE && newentry->de_Node.dn_Type >= ENTRY_DEVICE)
					{
						// Position before this entry
						if (entry->de_Node.dn_Pred->dn_Pred)
							position = (DirEntry *)entry->de_Node.dn_Pred;
						else
							position = (DirEntry *)-1;

						// Set to break out
						break_flag = 1;
					}
					break;

				// Files first
				case SEPARATE_FILESFIRST:

					// If adding a file, and this entry is a directory
					if (newentry->de_Node.dn_Type <= ENTRY_FILE && entry->de_Node.dn_Type >= ENTRY_DEVICE)
					{
						// Position before this entry
						if (entry->de_Node.dn_Pred->dn_Pred)
							position = (DirEntry *)entry->de_Node.dn_Pred;
						else
							position = (DirEntry *)-1;

						// Set to break out
						break_flag = 1;
					}
					break;
				}
			}

			// Look at sort method
			switch (sort_method)
			{
			// Add to end of buffer
			case -1:
				if (!entry->de_Node.dn_Succ->dn_Succ)
				{
					position = entry;
					break_flag = 2;
				}
				break;

			// Sort by display string
			case -2: {
				char *ptr1, *ptr2;

				// Get pointers
				ptr1 = (char *)GetTagData(DE_DisplayString, 0, entry->de_Tags);
				ptr2 = (char *)GetTagData(DE_DisplayString, 0, newentry->de_Tags);

				// Do comparison
				if (!ptr1 || !ptr2 || (namesort(ptr2, ptr1) <= 0))
					break_flag = 1;
			}
			break;

			// Sort by name
			case DISPLAY_NAME:
				cmp_val = namesort(newentry->de_Node.dn_Name, entry->de_Node.dn_Name);
				break;

			// Sort by size
			case DISPLAY_SIZE:
				cmp_val = buffer_sort_entries_size(newentry, entry);
				break;

			// Sort by protection
			case DISPLAY_PROTECT:
				cmp_val = buffer_sort_entries_protection(newentry, entry);
				break;

			// Sort by date
			case DISPLAY_DATE:
				cmp_val = buffer_sort_entries_date(newentry, entry);
				break;

			// Comment
			case DISPLAY_COMMENT:
				cmp_val = buffer_sort_entries_comment(newentry, entry);
				break;

			// Filetype
			case DISPLAY_FILETYPE:
				cmp_val = buffer_sort_entries_description(newentry, entry);
				break;

			// Version
			case DISPLAY_VERSION:
				cmp_val = buffer_sort_entries_version(newentry, entry);
				break;

			// Owner
			case DISPLAY_OWNER:
				cmp_val = buffer_sort_entries_owner(newentry, entry);
				break;

			// Group
			case DISPLAY_GROUP:
				cmp_val = buffer_sort_entries_group(newentry, entry);
				break;

			case DISPLAY_NETPROT:
				cmp_val = buffer_sort_entries_netprot(newentry, entry);
				break;
			}

			// Got comparison?
			if (cmp_val)
			{
				// Position here?
				if ((!sort_reverse && cmp_val < 0) || (sort_reverse && cmp_val > 0))
					break_flag = 1;
			}

			// Break flag set?
			if (break_flag)
			{
				// Actually position before the current entry
				if (break_flag == 1)
				{
					if (entry->de_Node.dn_Pred->dn_Pred)
						position = (DirEntry *)entry->de_Node.dn_Pred;
					else
						position = (DirEntry *)-1;
				}
				break;
			}
		}
	}

	// Add to head of buffer?
	if (position == (DirEntry *)-1)
	{
		// Fix "first" pointers for separation method
		switch (format->sort.separation)
		{
		// Directories first
		case SEPARATE_DIRSFIRST:
			if (newentry->de_Node.dn_Type <= ENTRY_FILE)
				buffer->first_file = newentry;
			break;

		// Files first
		case SEPARATE_FILESFIRST:
			if (newentry->de_Node.dn_Type >= ENTRY_DEVICE)
				buffer->first_dir = newentry;
			break;
		}

		// Store new "first" pointer
		AddHead((struct List *)&buffer->entry_list, (struct Node *)newentry);
	}

	// Or, add after an existing entry
	else if (position)
	{
		// Chain in
		Insert((struct List *)&buffer->entry_list, (struct Node *)newentry, (struct Node *)position);

		// See if this is the new first file
		if (position->de_Node.dn_Type >= ENTRY_DEVICE && newentry->de_Node.dn_Type <= ENTRY_FILE)
			buffer->first_file = newentry;

		// Or the new first directory
		else if (position->de_Node.dn_Type <= ENTRY_FILE && newentry->de_Node.dn_Type >= ENTRY_DEVICE)
			buffer->first_dir = newentry;
	}

	// Or, add to end of buffer
	else
	{
		// Starting a new list?
		if (IsListEmpty((struct List *)&buffer->entry_list))
		{
			if (newentry->de_Node.dn_Type <= ENTRY_FILE)
				buffer->first_file = newentry;
			else
				buffer->first_dir = newentry;
		}

		// Add to end of list
		AddTail((struct List *)&buffer->entry_list, (struct Node *)newentry);
	}

	// Look at entry type to increment counts
	switch (ENTRYTYPE(newentry->de_Node.dn_Type))
	{
	// File
	case ENTRY_FILE:
		++buffer->buf_TotalFiles[0];
		if (newentry->de_Flags & ENTF_SELECTED)
			++buffer->buf_SelectedFiles[0];
		buffer->buf_TotalBytes[0] += newentry->de_Size;
		if (newentry->de_Flags & ENTF_SELECTED)
			buffer->buf_SelectedBytes[0] += newentry->de_Size;
		break;

	// Directory/Device
	case ENTRY_DIRECTORY:
		buffer->buf_TotalBytes[0] += newentry->de_Size;
		if (newentry->de_Flags & ENTF_SELECTED)
			buffer->buf_SelectedBytes[0] += newentry->de_Size;
	case ENTRY_DEVICE:
		++buffer->buf_TotalDirs[0];
		if (newentry->de_Flags & ENTF_SELECTED)
			++buffer->buf_SelectedDirs[0];
		break;
	}

	// Increment buffer entry count
	++buffer->buf_TotalEntries[0];

	// A file?
	if (newentry->de_Node.dn_Type < 0)
	{
		short flags = SNIFFF_NO_FILETYPES;

		// Go through display entries
		for (a = 0; a < DISPLAY_LAST; a++)
		{
			// Something tricky to show?
			if (buffer->buf_ListFormat.display_pos[a] == DISPLAY_FILETYPE)
				flags &= ~SNIFFF_NO_FILETYPES;
			else if (buffer->buf_ListFormat.display_pos[a] == DISPLAY_VERSION)
				flags |= SNIFFF_VERSION;
		}

		// Show them?
		if (flags != SNIFFF_NO_FILETYPES)
		{
			// Already got things?
			if (GetTagData(DE_Filetype, 0, newentry->de_Tags))
				flags |= SNIFFF_NO_FILETYPES;
			if (newentry->de_Flags & ENTF_VERSION)
				flags &= ~SNIFFF_VERSION;

			// Get them if we still want them
			if (flags != SNIFFF_NO_FILETYPES)
			{
				// Get filetype for this entry
				filetype_find_type(buffer->buf_CurrentLister, buffer, newentry->de_Node.dn_Name, flags);
			}
		}
	}

	// Return entry pointer
	return newentry;
}

// Remove an entry from a buffer
void remove_file_entry(DirBuffer *buffer, DirEntry *entry)
{
	// Invalid buffer or entry?
	if (!buffer || !entry)
		return;

	// Check that entry isn't a reject
	if (!(entry->de_Flags & ENTF_REJECTED))
	{
		// Decrement list total count of entries
		--buffer->buf_TotalEntries[0];

		// Is entry a file?
		if (entry->de_Node.dn_Type <= ENTRY_FILE)
		{
			// Decrement file and byte count
			--buffer->buf_TotalFiles[0];
			buffer->buf_TotalBytes[0] -= entry->de_Size;

			// If entry is selected, decrement selection counts
			if (entry->de_Flags & ENTF_SELECTED)
			{
				--buffer->buf_SelectedFiles[0];
				buffer->buf_SelectedBytes[0] -= entry->de_Size;
			}
		}

		// Other type
		else
		{
			// Decrement directory count
			--buffer->buf_TotalDirs[0];

			// Decrement byte count for directories
			if (entry->de_Node.dn_Type >= ENTRY_DIRECTORY)
				buffer->buf_TotalBytes[0] -= entry->de_Size;

			// If entry is selected, decrement selection counts
			if (entry->de_Flags & ENTF_SELECTED)
			{
				--buffer->buf_SelectedDirs[0];

				// Decrement byte count for directories
				if (entry->de_Node.dn_Type >= ENTRY_DIRECTORY)
					buffer->buf_SelectedBytes[0] -= entry->de_Size;
			}
		}

		// Is entry the first file in the buffer?
		if (entry == buffer->first_file)
		{
			// If there's a following file, make that the new "first file"
			if (entry->de_Node.dn_Succ->dn_Succ)
				buffer->first_file = (DirEntry *)entry->de_Node.dn_Succ;
			else
				buffer->first_file = 0;
		}

		// Or the first directory in the buffer?
		else if (entry == buffer->first_dir)
		{
			// If there's a following entry, make that the new "first directory"
			if (entry->de_Node.dn_Succ->dn_Succ)
				buffer->first_dir = (DirEntry *)entry->de_Node.dn_Succ;
			else
				buffer->first_dir = 0;
		}
	}

	// Remove entry from list
	Remove((struct Node *)entry);
}

// Remove a file from a buffer and free it
void removefile(DirBuffer *buffer, DirEntry *entry)
{
	// Remove entry
	remove_file_entry(buffer, entry);

	// Free memory associated with this entry
	free_file_memory(entry);
}

// Free all memory associated with an entry
void free_file_memory(DirEntry *entry)
{
	// Valid entry?
	if (entry)
	{
		// Got tags?
		if (entry->de_Tags)
		{
			struct TagItem *tags, *first;

			// Get first tag
			tags = entry->de_Tags;
			first = entry->de_Tags;
			while (1)
			{
				// End of this tag?
				if (tags->ti_Tag == TAG_MORE || tags->ti_Tag == TAG_END)
				{
					long tag = tags->ti_Tag;

					// Get next tag pointer
					tags = (struct TagItem *)tags->ti_Data;

					// Free this section
					FreeMemH(first);

					// End?
					if (tag == TAG_END)
						break;

					// Get pointer to next section
					first = tags;
				}

				// Otherwise
				else
				{
					// NetworkInfo?
					if (tags->ti_Tag == DE_NetworkInfo)
					{
						// Free info
						FreeMemH((void *)tags->ti_Data);
					}

					// Menu
					else if (tags->ti_Tag == DE_PopupMenu)
					{
						struct List *list;
						struct Node *node, *next;

						// Get list pointer
						if ((list = (struct List *)tags->ti_Data))
						{
							// Go through list
							for (node = list->lh_Head; node->ln_Succ; node = next)
							{
								// Cache next
								next = node->ln_Succ;

								// Free this entry
								FreeMemH(node);
							}

							// Free list
							FreeMemH(list);
						}
					}

					// Skip to next
					++tags;
				}
			}
		}

		// Free entry
		FreeMemH(entry);
	}
}

// See if a file is rejected by the buffer format
BOOL buffer_test_file(DirEntry *entry, DirBuffer *buffer)
{
	// Hidden bit?
	if (buffer->buf_ListFormat.flags & LFORMATF_HIDDEN_BIT)
	{
		// Does this file have the H bit set?
		if (entry->de_Protection & 128)
			return 1;
	}

	// If this is a directory, return ok
	if (entry->de_Node.dn_Type >= ENTRY_DIRECTORY)
		return 0;

	// Reject icons?
	if (buffer->buf_ListFormat.flags & LFORMATF_REJECT_ICONS)
	{
		// Is this file an icon?
		if (isicon(entry->de_Node.dn_Name))
			return 1;
	}

	// Test show pattern
	if (buffer->buf_ListFormat.show_pattern[0])
	{
		// If the file doesn't match the pattern we reject it
		if (!(MatchPatternNoCase(buffer->buf_ListFormat.show_pattern_p, entry->de_Node.dn_Name)))
			return 1;
	}

	// Test hide pattern
	if (buffer->buf_ListFormat.hide_pattern[0])
	{
		// If the file matches the pattern we reject it
		if (MatchPatternNoCase(buffer->buf_ListFormat.hide_pattern_p, entry->de_Node.dn_Name))
			return 1;
	}

	return 0;
}

// Reject a file from a list
void buffer_reject_file(DirEntry *entry, DirBuffer *buffer)
{
	// Remove file from list, add to reject list
	remove_file_entry(buffer, entry);
	AddHead((struct List *)&buffer->reject_list, (struct Node *)entry);
	entry->de_Flags |= ENTF_REJECTED;
}

// Do list rejections
void buffer_do_rejects(DirBuffer *buffer)
{
	DirEntry *entry;

	// Valid buffer?
	if (!buffer)
		return;

	// Go through entry list
	for (entry = (DirEntry *)buffer->entry_list.mlh_Head; entry->de_Node.dn_Succ;)
	{
		DirEntry *next;

		// Store next
		next = (DirEntry *)entry->de_Node.dn_Succ;

		// See if file is rejected
		if (buffer_test_file(entry, buffer))
		{
			// Reject file
			buffer_reject_file(entry, buffer);
		}

		// Get next
		entry = next;
	}
}

// Replace list rejections
void buffer_replace_rejects(DirBuffer *buffer, BOOL sort)
{
	DirEntry *entry;

	// Go through reject list
	for (entry = (DirEntry *)buffer->reject_list.mlh_Head; entry->de_Node.dn_Succ;)
	{
		DirEntry *next;

		// Save next
		next = (DirEntry *)entry->de_Node.dn_Succ;

		// Remove from reject list
		Remove((struct Node *)entry);

		// Sort into list?
		if (sort)
			add_file_entry(buffer, entry, 0);

		// Add to list for later sorting
		else
		{
			// Add to list
			AddTail((struct List *)&buffer->entry_list, (struct Node *)entry);

			// Increment file or directory count
			if (entry->de_Node.dn_Type <= ENTRY_FILE)
				++buffer->buf_TotalFiles[0];
			else
				++buffer->buf_TotalDirs[0];
		}

		// Clear reject flag
		entry->de_Flags &= ~ENTF_REJECTED;

		// Get next
		entry = next;
	}
}

// Locks a buffer
void buffer_lock(DirBuffer *buffer, BOOL exclusive)
{
	GetSemaphore(&buffer->semaphore, (exclusive) ? SEMF_EXCLUSIVE : SEMF_SHARED, 0);
}

// Unlocks a buffer
void buffer_unlock(DirBuffer *buffer)
{
	FreeSemaphore(&buffer->semaphore);
}

// Clear a buffer lock
void buffer_clear_lock(DirBuffer *buffer, short arg)
{
	// Was buffer locked?
	if (buffer->buf_CurrentLister)
	{
		// Does current buffer have a custom handler?
		if (buffer->buf_CustomHandler[0])
		{
			// Send inactive message
			buffer_inactive(buffer, arg);
		}

		// Get position from lister
		if (lister_valid_window(buffer->buf_CurrentLister))
		{
			buffer->dimensions = *((struct IBox *)&buffer->buf_CurrentLister->window->LeftEdge);
			buffer->dim_valid = 1;
		}

		// Clear lock
		buffer->buf_CurrentLister = 0;
	}
}

// Send inactive message
void buffer_inactive(DirBuffer *buffer, short arg)
{
	rexx_handler_msg(0,
					 buffer,
					 0,
					 HA_String,
					 0,
					 "inactive",
					 HA_Value,
					 1,
					 buffer->buf_CurrentLister,
					 HA_String,
					 2,
					 (buffer->buf_CustomTitle[0]) ? buffer->buf_CustomTitle : buffer->buf_Path,
					 HA_Value,
					 3,
					 arg,
					 HA_String,
					 4,
					 buffer->buf_Path,
					 TAG_END);
}

// Send active message
void buffer_active(DirBuffer *buffer, short arg)
{
	rexx_handler_msg(0,
					 buffer,
					 0,
					 HA_String,
					 0,
					 "active",
					 HA_Value,
					 1,
					 (buffer->buf_CurrentLister) ? buffer->buf_CurrentLister : buffer->buf_OwnerLister,
					 HA_String,
					 2,
					 (buffer->buf_CustomTitle[0]) ? buffer->buf_CustomTitle : buffer->buf_Path,
					 HA_Value,
					 3,
					 arg,
					 HA_String,
					 4,
					 buffer->buf_Path,
					 TAG_END);
}

// Add NetWorkInfo to a DirEntry
void direntry_add_network(DirBuffer *buffer, DirEntry *entry, NetworkInfo *network_info)
{
	struct TagItem *tags;

	// Allocate tags and NetworkInfo copy
	if ((tags = AllocMemH(buffer->memory, sizeof(struct TagItem) * 2)) &&
		(tags[0].ti_Data = (IPTR)AllocMemH(buffer->memory, sizeof(NetworkInfo))))
	{
		// Fill out tags
		tags[0].ti_Tag = DE_NetworkInfo;
		tags[1].ti_Tag = TAG_END;

		// Copy NetworkInfo
		CopyMem((char *)network_info, (char *)tags[0].ti_Data, sizeof(NetworkInfo));

		// Chain tags
		ChainTagItems(&entry->de_Tags, tags);

		// Set flag
		entry->de_Flags |= ENTF_NETWORK;
	}
}

// Add a string
BOOL direntry_add_string(DirBuffer *buffer, DirEntry *entry, ULONG type, char *string)
{
	struct TagItem *tags;
	BOOL ret = 0;

	// See if tag exists
	if ((tags = FindTagItem(type, entry->de_Tags)))
	{
		// Free existing string
		FreeMemH((void *)tags->ti_Data);
		tags->ti_Data = 0;
	}

	// Null string?
	if (!string || !*string)
		return 0;

	// Allocate tags if needed
	if (!tags)
	{
		// Allocate tags
		if (!(tags = AllocMemH(buffer->memory, sizeof(struct TagItem) * 2)))
			return 0;

		// Set tag type and end tag
		tags[0].ti_Tag = type;
		tags[1].ti_Tag = TAG_END;

		// Chain tags
		ChainTagItems(&entry->de_Tags, tags);
	}

	// Allocate string copy
	if ((tags->ti_Data = (IPTR)AllocMemH(buffer->memory, strlen(string) + 1)))
	{
		// Copy string
		strcpy((char *)tags->ti_Data, string);
		ret = 1;
	}

	return ret;
}

// Add VersionInfo to a DirEntry
void direntry_add_version(DirBuffer *buffer, DirEntry *entry, UWORD ver, UWORD rev, UWORD days)
{
	struct TagItem *tags;
	VersionInfo *info;

	// Not already got VersionInfo?
	if (!(entry->de_Flags & ENTF_VERSION) || !(info = (VersionInfo *)GetTagData(DE_VersionInfo, 0, entry->de_Tags)))
	{
		// Allocate tags and VersionInfo
		if (!(tags = AllocMemH(buffer->memory, sizeof(struct TagItem) * 2)) ||
			!(info = AllocMemH(buffer->memory, sizeof(VersionInfo))))
		{
			// Failed
			FreeMemH(tags);
			return;
		}

		// Fill out tags
		tags[0].ti_Tag = DE_VersionInfo;
		tags[0].ti_Data = (IPTR)info;
		tags[1].ti_Tag = TAG_END;

		// Chain tags
		ChainTagItems(&entry->de_Tags, tags);

		// Set flag
		entry->de_Flags |= ENTF_VERSION;
	}

	// Fill out version info
	info->vi_Version = ver;
	info->vi_Revision = rev;
	info->vi_Days = days;
}
