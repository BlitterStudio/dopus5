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

#include <stdlib.h>

#include "dopus.h"

// Array of sort functions
STATIC CONST_APTR sort_functions[] = {buffer_sort_entries_name,
									  buffer_sort_entries_size,
									  buffer_sort_entries_protection,
									  buffer_sort_entries_date,
									  buffer_sort_entries_comment,
									  buffer_sort_entries_description,
									  buffer_sort_entries_owner,
									  buffer_sort_entries_group,
									  buffer_sort_entries_netprot,
									  buffer_sort_entries_version};

// Sort a list of files
void buffer_sort_list(DirBuffer *buffer, struct MinList *file_list, long file_count, long dir_count)
{
	ListFormat *format;
	DirEntry *entry;
	BOOL sort_reverse = 0;
	short sort_method;

	// Get sort format
	format = &buffer->buf_ListFormat;

	// See if reverse sorting is set for this window
	if (format->sort.sort_flags & SORT_REVERSE)
		sort_reverse = 1;

	// Get desired sort method
	sort_method = format->sort.sort;
	if (sort_method < 0 || sort_method >= DISPLAY_LAST)
		sort_method = DISPLAY_NAME;

	// Sort directories and files separately?
	if (format->sort.separation == SEPARATE_DIRSFIRST || format->sort.separation == SEPARATE_FILESFIRST)
	{
		DirEntry **file_array, **dir_array;
		long file_num, dir_num, num;

		// Allocate arrays
		if (!(file_array = AllocVec(sizeof(DirEntry *) * (file_count + 1), 0)) ||
			!(dir_array = AllocVec(sizeof(DirEntry *) * (dir_count + 1), 0)))
		{
			// Failed; fall back to insertion sort
			FreeVec(file_array);
			buffer_insert_sort_list(buffer, file_list);
			return;
		}

		// Build arrays
		for (entry = (DirEntry *)file_list->mlh_Head, file_num = 0, dir_num = 0; entry->de_Node.dn_Succ;)
		{
			DirEntry *next = (DirEntry *)entry->de_Node.dn_Succ;

			// Test for rejection
			if (buffer_test_file(entry, buffer))
			{
				// Add to reject list
				AddHead((struct List *)&buffer->reject_list, (struct Node *)entry);
				entry->de_Flags |= ENTF_REJECTED;
			}

			// Valid entry
			else
			{
				// Increment buffer entry count
				++buffer->buf_TotalEntries[0];

				// File?
				if (ENTRYTYPE(entry->de_Node.dn_Type) == ENTRY_FILE)
				{
					// Increment file count
					++buffer->buf_TotalFiles[0];
					buffer->buf_TotalBytes[0] += entry->de_Size;
					if (entry->de_Flags & ENTF_SELECTED)
					{
						++buffer->buf_SelectedFiles[0];
						buffer->buf_SelectedBytes[0] += entry->de_Size;
					}

					// Store in file array
					file_array[file_num++] = entry;
				}

				// Directory
				else if (ENTRYTYPE(entry->de_Node.dn_Type) == ENTRY_DIRECTORY)
				{
					// Increment directory count
					++buffer->buf_TotalDirs[0];
					if (entry->de_Flags & ENTF_SELECTED)
						++buffer->buf_SelectedDirs[0];
					buffer->buf_TotalBytes[0] += entry->de_Size;
					if (entry->de_Flags & ENTF_SELECTED)
						buffer->buf_SelectedBytes[0] += entry->de_Size;

					// Store in dir array
					dir_array[dir_num++] = entry;
				}
			}

			// Get next
			entry = next;
		}

		// Sort the arrays
		if (dir_num > 0)
			buffer_sort_array(dir_array, dir_num, sort_method);
		if (file_num > 0)
			buffer_sort_array(file_array, file_num, sort_method);

		// Files first?
		if ((format->sort.separation == SEPARATE_FILESFIRST && !sort_reverse) ||
			(format->sort.separation == SEPARATE_DIRSFIRST && sort_reverse))
		{
			// Go through file array
			for (num = 0; num < file_num; num++)
			{
				// Add to entry list
				if (sort_reverse)
				{
					AddHead((struct List *)&buffer->entry_list, (struct Node *)file_array[num]);
				}
				else
				{
					AddTail((struct List *)&buffer->entry_list, (struct Node *)file_array[num]);
				}
			}

			// Go through directory array
			for (num = 0; num < dir_num; num++)
			{
				// Add to entry list
				if (sort_reverse)
				{
					AddHead((struct List *)&buffer->entry_list, (struct Node *)dir_array[num]);
				}
				else
				{
					AddTail((struct List *)&buffer->entry_list, (struct Node *)dir_array[num]);
				}
			}
		}

		// Directories first
		else
		{
			// Go through directory array
			for (num = 0; num < dir_num; num++)
			{
				// Add to entry list
				if (sort_reverse)
				{
					AddHead((struct List *)&buffer->entry_list, (struct Node *)dir_array[num]);
				}
				else
				{
					AddTail((struct List *)&buffer->entry_list, (struct Node *)dir_array[num]);
				}
			}

			// Go through file array
			for (num = 0; num < file_num; num++)
			{
				// Add to entry list
				if (sort_reverse)
				{
					AddHead((struct List *)&buffer->entry_list, (struct Node *)file_array[num]);
				}
				else
				{
					AddTail((struct List *)&buffer->entry_list, (struct Node *)file_array[num]);
				}
			}
		}

		// Get first file pointer
		if (file_num < 1)
			buffer->first_file = 0;
		else if (sort_reverse)
			buffer->first_file = file_array[file_num - 1];
		else
			buffer->first_file = file_array[0];

		// Get first directory pointer
		if (dir_num < 1)
			buffer->first_dir = 0;
		else if (sort_reverse)
			buffer->first_dir = dir_array[dir_num - 1];
		else
			buffer->first_dir = dir_array[0];

		// Free sort arrays
		FreeVec(file_array);
		FreeVec(dir_array);
	}

	// Sort together
	else
	{
		DirEntry **array;
		long total_num, num;

		// Allocate array
		if (!(array = AllocVec(sizeof(DirEntry *) * (file_count + dir_count + 1), 0)))
		{
			// Failed; fall back to insertion sort
			buffer_insert_sort_list(buffer, file_list);
			return;
		}

		// Build array
		for (entry = (DirEntry *)file_list->mlh_Head, total_num = 0; entry->de_Node.dn_Succ;)
		{
			DirEntry *next = (DirEntry *)entry->de_Node.dn_Succ;

			// Test for rejection
			if (buffer_test_file(entry, buffer))
			{
				// Add to reject list
				AddHead((struct List *)&buffer->reject_list, (struct Node *)entry);
				entry->de_Flags |= ENTF_REJECTED;
			}

			// Valid entry
			else
			{
				// Increment buffer entry count
				++buffer->buf_TotalEntries[0];

				// File?
				if (ENTRYTYPE(entry->de_Node.dn_Type) == ENTRY_FILE)
				{
					// Increment buffer count
					++buffer->buf_TotalFiles[0];
					buffer->buf_TotalBytes[0] += entry->de_Size;
					if (entry->de_Flags & ENTF_SELECTED)
					{
						++buffer->buf_SelectedFiles[0];
						buffer->buf_SelectedBytes[0] += entry->de_Size;
					}
				}

				// Directory
				else if (ENTRYTYPE(entry->de_Node.dn_Type) == ENTRY_DIRECTORY)
				{
					++buffer->buf_TotalDirs[0];
					if (entry->de_Flags & ENTF_SELECTED)
						++buffer->buf_SelectedDirs[0];
					buffer->buf_TotalBytes[0] += entry->de_Size;
					if (entry->de_Flags & ENTF_SELECTED)
						buffer->buf_SelectedBytes[0] += entry->de_Size;
				}

				// Store in array
				array[total_num++] = entry;
			}

			// Get next
			entry = next;
		}

		// Sort the array
		if (total_num > 0)
			buffer_sort_array(array, total_num, sort_method);

		// Go through array
		for (num = 0; num < total_num; num++)
		{
			// Add to entry list
			if (sort_reverse)
			{
				AddHead((struct List *)&buffer->entry_list, (struct Node *)array[num]);
			}
			else
			{
				AddTail((struct List *)&buffer->entry_list, (struct Node *)array[num]);
			}
		}

		// Clear first file/directory pointers
		buffer->first_file = 0;
		buffer->first_dir = 0;

		// Free sort array
		FreeVec(array);
	}
}

// Sort an array
void buffer_sort_array(DirEntry **array, long count, short sort_method)
{
	register int j, i, h;
	register DirEntry *v;
	register int (*ASM cmp)(REG(a0, DirEntry *), REG(a1, DirEntry *));

	// Get comparison function
	cmp = (int (*ASM)(REG(a0, DirEntry *), REG(a1, DirEntry *)))sort_functions[sort_method];

	// Shell-sort files
	for (h = 1; h < count / 9; h = 3 * h + 1)
		;
	for (; h > 0; h /= 3)
	{
		for (i = h + 1; i <= count; i++)
		{
			v = array[i - 1];
			j = i;
			while (j > h && ((*cmp)(array[j - h - 1], v)) > 0)
			{
				array[j - 1] = array[j - h - 1];
				j -= h;
			}
			array[j - 1] = v;
		}
	}
}

// Insert sort a list of files
void buffer_insert_sort_list(DirBuffer *buffer, struct MinList *file_list)
{
	DirEntry *entry;

	// Go through list
	for (entry = (DirEntry *)file_list->mlh_Head; entry->de_Node.dn_Succ;)
	{
		DirEntry *next = (DirEntry *)entry->de_Node.dn_Succ;

		// Add file to buffer
		add_file_entry(buffer, entry, 0);

		// Get next
		entry = next;
	}
}

// Compare two entries for sort by name
int ASM buffer_sort_entries_name(REG(a0, DirEntry *entry1), REG(a1, DirEntry *entry2))
{
	return namesort(entry1->de_Node.dn_Name, entry2->de_Node.dn_Name);
}

// Compare two entries for sort by size
int ASM buffer_sort_entries_size(REG(a0, DirEntry *entry1), REG(a1, DirEntry *entry2))
{
	// If sizes are equal, do name sort
	if (entry1->de_Size == entry2->de_Size)
		return namesort(entry1->de_Node.dn_Name, entry2->de_Node.dn_Name);

		// Otherwise, return difference in sizes
#ifdef USE_64BIT
	return (entry1->de_Size < entry2->de_Size) ? -1 : 1;
#else
	return (long)((long)entry1->de_Size - (long)entry2->de_Size);
#endif
}

// Compare two entries for sort by protection
int ASM buffer_sort_entries_protection(REG(a0, DirEntry *entry1), REG(a1, DirEntry *entry2))
{
	register LONG prot, sort_prot;

	// If values are equal, do name sort
	if (entry1->de_Protection == entry2->de_Protection)
		return namesort(entry1->de_Node.dn_Name, entry2->de_Node.dn_Name);

	// Get real protection values
	prot = (entry1->de_Protection & 255) ^ 15;
	sort_prot = (entry2->de_Protection & 255) ^ 15;

	// Return difference in values
	return (prot - sort_prot);
}

// Compare two entries for sort by date
int ASM buffer_sort_entries_date(REG(a0, DirEntry *entry1), REG(a1, DirEntry *entry2))
{
	register int ret;

	// Compare dates
	if ((ret = CompareDates(&entry2->de_Date, &entry1->de_Date)) == 0)
	{
		// If values are equal, do name sort
		return namesort(entry1->de_Node.dn_Name, entry2->de_Node.dn_Name);
	}

	return ret;
}

// Compare two entries for sort by comment
int ASM buffer_sort_entries_comment(REG(a0, DirEntry *entry1), REG(a1, DirEntry *entry2))
{
	char *comment1, *comment2;

	// Get comments
	comment1 = (char *)GetTagData(DE_Comment, 0, entry1->de_Tags);
	comment2 = (char *)GetTagData(DE_Comment, 0, entry2->de_Tags);

	// Compare comments
	if (comment1 && comment2)
	{
		register short ret;

		// If comments are equal, do name sort
		if ((ret = stricmp(comment1, comment2)) == 0)
			return namesort(entry1->de_Node.dn_Name, entry2->de_Node.dn_Name);
		return ret;
	}

	// No comment on one file
	else if (comment1)
		return 1;
	else if (comment2)
		return -1;

	// No comments on either, do name sort
	return namesort(entry1->de_Node.dn_Name, entry2->de_Node.dn_Name);
}

// Compare two entries for sort by description
int ASM buffer_sort_entries_description(REG(a0, DirEntry *entry1), REG(a1, DirEntry *entry2))
{
	char *type1, *type2;

	// Get filetypes
	type1 = (char *)GetTagData(DE_Filetype, 0, entry1->de_Tags);
	type2 = (char *)GetTagData(DE_Filetype, 0, entry2->de_Tags);

	// Compare descriptions
	if (type1 && type2)
	{
		register short ret;

		// If descriptions are equal, do name sort
		if ((ret = stricmp(type1, type2)) == 0)
			return namesort(entry1->de_Node.dn_Name, entry2->de_Node.dn_Name);
		return ret;
	}

	// No description on one file
	else if (type1)
		return 1;
	else if (type2)
		return -1;

	// No descriptions on either, do name sort
	return namesort(entry1->de_Node.dn_Name, entry2->de_Node.dn_Name);
}

// Compare two entries for sort by version
int ASM buffer_sort_entries_version(REG(a0, DirEntry *entry1), REG(a1, DirEntry *entry2))
{
	VersionInfo *info1 = 0, *info2 = 0;

	// Get version info
	if (entry1->de_Flags & ENTF_VERSION)
		info1 = (VersionInfo *)GetTagData(DE_VersionInfo, 0, entry1->de_Tags);
	if (entry2->de_Flags & ENTF_VERSION)
		info2 = (VersionInfo *)GetTagData(DE_VersionInfo, 0, entry2->de_Tags);

	// Do both files have version info?
	if (info1 && info2)
	{
		// Compare version numbers
		if (info1->vi_Version == info2->vi_Version)
		{
			// Same revision?
			if (info1->vi_Revision == info2->vi_Revision)
			{
				// Same version, compare version dates
				if (info1->vi_Days == info2->vi_Days)
				{
					// Same dates too, do name sort
					return namesort(entry1->de_Node.dn_Name, entry2->de_Node.dn_Name);
				}

				// Return difference in dates
				return info1->vi_Days - info2->vi_Days;
			}

			// Return difference in revision
			return info1->vi_Revision - info2->vi_Revision;
		}

		// Return difference in version
		return info1->vi_Version - info2->vi_Version;
	}

	// No version on one file
	else if (info1)
		return 1;
	else if (info2)
		return -1;

	// No versions on either, do name sort
	return namesort(entry1->de_Node.dn_Name, entry2->de_Node.dn_Name);
}

// Compare two entries for sort by owner
int ASM buffer_sort_entries_owner(REG(a0, DirEntry *entry1), REG(a1, DirEntry *entry2))
{
	register NetworkInfo *network1 = 0, *network2 = 0;

	// Get network pointers
	if (entry1->de_Flags & ENTF_NETWORK)
		network1 = (NetworkInfo *)GetTagData(DE_NetworkInfo, 0, entry1->de_Tags);
	if (entry2->de_Flags & ENTF_NETWORK)
		network2 = (NetworkInfo *)GetTagData(DE_NetworkInfo, 0, entry2->de_Tags);

	// Compare owners
	if (network1 && *network1->owner && network2 && *network2->owner)
	{
		register short ret;

		// If owners are equal, do name sort
		if ((ret = stricmp(network1->owner, network2->owner)) == 0)
			return namesort(entry1->de_Node.dn_Name, entry2->de_Node.dn_Name);
		return ret;
	}

	// No owner on one file
	else if (network1 && *network1->owner)
		return 1;
	else if (network2 && *network2->owner)
		return -1;

	// No owners on either, do name sort
	return namesort(entry1->de_Node.dn_Name, entry2->de_Node.dn_Name);
}

// Compare two entries for sort by group
int ASM buffer_sort_entries_group(REG(a0, DirEntry *entry1), REG(a1, DirEntry *entry2))
{
	register NetworkInfo *network1 = 0, *network2 = 0;

	// Get network pointers
	if (entry1->de_Flags & ENTF_NETWORK)
		network1 = (NetworkInfo *)GetTagData(DE_NetworkInfo, 0, entry1->de_Tags);
	if (entry2->de_Flags & ENTF_NETWORK)
		network2 = (NetworkInfo *)GetTagData(DE_NetworkInfo, 0, entry2->de_Tags);

	// Compare groups
	if (network1 && *network1->group && network2 && *network2->group)
	{
		register short ret;

		// If groups are equal, do name sort
		if ((ret = stricmp(network1->group, network2->group)) == 0)
			return namesort(entry1->de_Node.dn_Name, entry2->de_Node.dn_Name);
		return ret;
	}

	// No group on one file
	else if (network1 && *network1->group)
		return 1;
	else if (network2 && *network2->group)
		return -1;

	// No groups on either, do name sort
	return namesort(entry1->de_Node.dn_Name, entry2->de_Node.dn_Name);
}

// Compare two entries for sort by network protection bits
int ASM buffer_sort_entries_netprot(REG(a0, DirEntry *entry1), REG(a1, DirEntry *entry2))
{
	register LONG prot, sort_prot;

	// Get network protection values
	prot = entry1->de_Protection & (~255);
	sort_prot = entry2->de_Protection & (~255);

	// If values are equal, do name sort
	if (prot == sort_prot)
		return namesort(entry1->de_Node.dn_Name, entry2->de_Node.dn_Name);

	// Return difference in values
	return (prot - sort_prot);
}

// Sort two strings taking leading numbers into account
int ASM namesort(REG(a0, char *str1), REG(a1, char *str2))
{
	// Do both names begin with numbers?
	if (*str1 <= '9' && *str1 >= '0' && *str2 <= '9' && *str2 >= '0')
	{
		register long n1, n2;

		// Get value of leading numbers
		n1 = atoi(str1);
		n2 = atoi(str2);

		// If the two numbers are not the same, return the difference between them
		if (n1 != n2)
			return (n1 - n2);

		// Get string following numbers
		while (*str1 >= '0' && *str1 <= '9')
			++str1;
		while (*str2 >= '0' && *str2 <= '9')
			++str2;
	}

	// Return value of the comparison (uses utility.library strcmp routine)
	return Stricmp(str1, str2);
}

// Change the reverse or separation state of a buffer (must be already sorted)
void buffer_sort_reversesep(DirBuffer *buffer, BOOL do_reverse)
{
	ListFormat *format;
	struct MinList temp;
	DirEntry *entry;

	// Get sort format
	format = &buffer->buf_ListFormat;

	// Copy list pointers
	temp = buffer->entry_list;
	temp.mlh_TailPred->mln_Succ = (struct MinNode *)&temp.mlh_Tail;
	temp.mlh_Head->mln_Pred = (struct MinNode *)&temp.mlh_Head;

	// Clear buffer entry pointers
	NewList((struct List *)&buffer->entry_list);
	buffer->first_file = 0;
	buffer->first_dir = 0;

	// Sort directories and files separately?
	if (format->sort.separation == SEPARATE_DIRSFIRST || format->sort.separation == SEPARATE_FILESFIRST)
	{
		struct MinList file_list, dir_list;

		// Initialise temporary lists
		NewList((struct List *)&file_list);
		NewList((struct List *)&dir_list);

		// Go through entries
		for (entry = (DirEntry *)temp.mlh_Head; entry->de_Node.dn_Succ;)
		{
			DirEntry *next = (DirEntry *)entry->de_Node.dn_Succ;

			// File?
			if (ENTRYTYPE(entry->de_Node.dn_Type) == ENTRY_FILE)
			{
				// Add to file list
				if (do_reverse)
					AddHead((struct List *)&file_list, (struct Node *)entry);
				else
					AddTail((struct List *)&file_list, (struct Node *)entry);
			}

			// Directory
			else if (do_reverse)
				AddHead((struct List *)&dir_list, (struct Node *)entry);
			else
				AddTail((struct List *)&dir_list, (struct Node *)entry);

			// Get next
			entry = next;
		}

		// Get first file and directory pointers
		if (!(IsListEmpty((struct List *)&file_list)))
			buffer->first_file = (DirEntry *)file_list.mlh_Head;
		if (!(IsListEmpty((struct List *)&dir_list)))
			buffer->first_dir = (DirEntry *)dir_list.mlh_Head;

		// Directories first (or no files)
		if ((format->sort.separation == SEPARATE_DIRSFIRST && !(IsListEmpty((struct List *)&dir_list))) ||
			IsListEmpty((struct List *)&file_list))
		{
			// Patch directory list in
			buffer->entry_list.mlh_Head = dir_list.mlh_Head;
			dir_list.mlh_Head->mln_Pred = (struct MinNode *)&buffer->entry_list.mlh_Head;

			// Anything in file list?
			if (!(IsListEmpty((struct List *)&file_list)))
			{
				// Patch file list in; link the two lists together
				dir_list.mlh_TailPred->mln_Succ = file_list.mlh_Head;
				file_list.mlh_Head->mln_Pred = dir_list.mlh_TailPred;

				// Set tail pointer
				buffer->entry_list.mlh_TailPred = file_list.mlh_TailPred;
			}

			// Otherwise, set tail
			else
				buffer->entry_list.mlh_TailPred = dir_list.mlh_TailPred;
		}

		// Files first, or no directories
		else if (!(IsListEmpty((struct List *)&file_list)))
		{
			// Patch file list in
			buffer->entry_list.mlh_Head = file_list.mlh_Head;
			file_list.mlh_Head->mln_Pred = (struct MinNode *)&buffer->entry_list.mlh_Head;

			// Anything in directory list?
			if (!(IsListEmpty((struct List *)&dir_list)))
			{
				// Patch dir list in; link the two lists together
				file_list.mlh_TailPred->mln_Succ = dir_list.mlh_Head;
				dir_list.mlh_Head->mln_Pred = file_list.mlh_TailPred;

				// Set tail pointer
				buffer->entry_list.mlh_TailPred = dir_list.mlh_TailPred;
			}

			// Otherwise, set tail
			else
				buffer->entry_list.mlh_TailPred = file_list.mlh_TailPred;
		}

		// Fix tail pointer
		buffer->entry_list.mlh_TailPred->mln_Succ = (struct MinNode *)&buffer->entry_list.mlh_Tail;
	}

	// Mix files and directories
	else
	{
		// Go through entries
		for (entry = (DirEntry *)temp.mlh_Head; entry->de_Node.dn_Succ;)
		{
			DirEntry *next = (DirEntry *)entry->de_Node.dn_Succ;

			// Add to main list
			if (do_reverse)
				AddHead((struct List *)&buffer->entry_list, (struct Node *)entry);
			else
				AddTail((struct List *)&buffer->entry_list, (struct Node *)entry);

			// Get next
			entry = next;
		}
	}
}

// Sort selected entries to the top of the list
void buffer_sort_selected(DirBuffer *buffer)
{
	struct MinList temp;
	DirEntry *entry, *first_file = 0, *first_pos = 0;

	// Lock buffer
	buffer_lock(buffer, TRUE);

	// Initialise temporary list
	NewList((struct List *)&temp);

	// Go through entries
	for (entry = (DirEntry *)buffer->entry_list.mlh_Head; entry->de_Node.dn_Succ;)
	{
		DirEntry *next = (DirEntry *)entry->de_Node.dn_Succ;

		// File?
		if (ENTRYTYPE(entry->de_Node.dn_Type) == ENTRY_FILE)
		{
			// Remove and add to temporary if selected
			if (entry->de_Flags & ENTF_SELECTED)
			{
				// Remember first position
				if (!first_pos)
					first_pos = (DirEntry *)entry->de_Node.dn_Pred;

				// Remove and re-add
				Remove((struct Node *)entry);
				AddTail((struct List *)&temp, (struct Node *)entry);
			}

			// Remember if the first file
			else if (!first_file)
				first_file = entry;
		}

		// Get next
		entry = next;
	}

	// Something in the temporary list?
	if (!(IsListEmpty((struct List *)&temp)))
	{
		// If we have a first file, get its predecessor
		if (first_file)
			first_pos = (DirEntry *)first_file->de_Node.dn_Pred;

		// Go through temporary list, add after first position
		for (entry = (DirEntry *)temp.mlh_Head; entry->de_Node.dn_Succ;)
		{
			DirEntry *next = (DirEntry *)entry->de_Node.dn_Succ;

			// Remove and add after last one
			Remove((struct Node *)entry);
			Insert((struct List *)&buffer->entry_list, (struct Node *)entry, (struct Node *)first_pos);

			// Bump next position
			first_pos = (DirEntry *)entry;

			// Get next
			entry = next;
		}
	}

	// Unlock buffer
	buffer_unlock(buffer);
}
