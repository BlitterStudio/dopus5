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
#include <proto/module.h>

// PRINTDIR internal function
DOPUS_FUNC(function_printdir)
{
	PathNode *path;
	Lister *lister;
	BPTR lock;
	APTR outfile;
	struct Library *ModuleBase;
#ifdef __amigaos4__
	struct ModuleIFace *IModule;
#endif

	// Get source path
	if (!(path = function_path_current(&handle->source_paths)))
		return 0;

	// Open temporary output file
	lsprintf(handle->work_buffer + 800, "t:dopus-tmp.%lx", handle);
	if (!(outfile = OpenBuf(handle->work_buffer + 800, MODE_NEWFILE, 1024)))
		return 0;

	// Output path
	WriteBuf(outfile, path->path, -1);
	WriteBuf(outfile, "\n\n", 2);

	// Valid lister?
	if ((lister = path->lister))
	{
		DirBuffer *buffer;
		DirEntry *entry;

		// Lock lister buffer
		buffer_lock((buffer = function_path_buffer(path)), FALSE);

		// Go through buffer
		for (entry = (DirEntry *)buffer->entry_list.mlh_Head; entry->de_Node.dn_Succ;
			 entry = (DirEntry *)entry->de_Node.dn_Succ)
		{
			short len;

			// Build display string for this entry
			builddisplaystring(entry, handle->work_buffer, lister);

			// Find last character in string
			for (len = strlen(handle->work_buffer) - 1; len >= 0; len--)
			{
				if (handle->work_buffer[len] != ' ')
				{
					handle->work_buffer[len + 1] = 0;
					break;
				}
			}

			// Output to temporary file
			WriteBuf(outfile, handle->work_buffer, -1);
			WriteBuf(outfile, "\n", 1);
		}

		// Unlock buffer
		buffer_unlock(buffer);
	}

	// Otherwise, lock directory
	else if ((lock = Lock(path->path, ACCESS_READ)))
	{
		// Examine directory
		Examine(lock, handle->s_info);

		// Go through directory
		while (ExNext(lock, handle->s_info))
		{
			// Get size
			if (handle->s_info->fib_DirEntryType < 0)
			{
				if (handle->s_info->fib_Size > 0)
					lsprintf(handle->temp_buffer, "%ld", handle->s_info->fib_Size);
				else
					strcpy(handle->temp_buffer, GetString(&locale, MSG_EMPTY));
			}
			else
				strcpy(handle->temp_buffer, "<dir>");

			// Get protection and date strings
			protect_get_string(handle->s_info->fib_Protection, handle->temp_buffer + 20);
			date_build_string(&handle->s_info->fib_Date, handle->temp_buffer + 40, 1);

			// Build output string for this file
			lsprintf(handle->work_buffer,
					 "%-24s%8s %8s %s\n",
					 handle->s_info->fib_FileName,
					 handle->temp_buffer,
					 handle->temp_buffer + 20,
					 handle->temp_buffer + 40);

			// Write output
			WriteBuf(outfile, handle->work_buffer, -1);

			// Comment?
			if (handle->s_info->fib_Comment[0])
			{
				WriteBuf(outfile, ": ", 2);
				WriteBuf(outfile, handle->s_info->fib_Comment, -1);
				WriteBuf(outfile, "\n", 1);
			}
		}

		// Unlock directory
		UnLock(lock);
	}

	// Close output file
	CloseBuf(outfile);

	// Open print module
	if ((ModuleBase = OpenModule("print.module"))
#ifdef __amigaos4__
		&& (IModule = (struct ModuleIFace *)GetInterface(ModuleBase, "main", 1, NULL))
#endif
	)
	{
		struct List list;
		struct Node node;

		// Initialise fake list
		NewList(&list);
		node.ln_Name = handle->work_buffer + 800;
		AddTail(&list, &node);

		// Print file
		Module_Entry(&list, GUI->screen_pointer, handle->ipc, &main_ipc, 0, 0);

// Close print module
#ifdef __amigaos4__
		DropInterface((struct Interface *)IModule);
#endif
		CloseLibrary(ModuleBase);
	}
	else
		CloseLibrary(ModuleBase);  // In case libray opens and interface doesn't

	// Delete temporary file
	DeleteFile(handle->work_buffer + 800);
	return 1;
}
