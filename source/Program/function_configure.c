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

// CONFIGURE a lister
DOPUS_FUNC(function_configure)
{
	Lister *lister;
	struct Library *ModuleBase;
#ifdef __amigaos4__
	struct ModuleIFace *IModule;
#endif
	ListFormat format;
	ListFormat default_format;
	ListFormat *pass_format = 0;
	ULONG ret;

	// Get current lister
	if (!(lister = function_lister_current(&handle->source_paths)))
		return 0;

	// Get current format of the panel the command was started from
	format = function_path_buffer(handle->source_paths.current)->buf_ListFormat;
	default_format = environment->env->list_format;

	// Open lister format module
	if (!(ModuleBase = OpenModule("listerformat.module"))
#ifdef __amigaos4__
		|| !(IModule = (struct ModuleIFace *)GetInterface(ModuleBase, "main", 1, NULL))
#endif
	)
	{
		CloseLibrary(ModuleBase);  // In case module opens but interface doesn't
		return 0;
	}

	// Send message to say hello
	IPC_Command(lister->ipc, LISTER_CONFIGURE, 1, 0, 0, 0);

	// Edit list format
	ret = Module_Entry((struct List *)&format,
					   (struct Screen *)lister->window,
					   handle->ipc,
					   &main_ipc,
					   1,
					   (IPTR)&environment->env->list_format);

// Close library
#ifdef __amigaos4__
	DropInterface((struct Interface *)IModule);
#endif
	CloseLibrary(ModuleBase);

	// Need to refresh the lister?
	if (ret)
	{
		// Persist "Set as defaults" changes made by the lister format editor
		if ((ret & CONFIG_SAVE) && CompareListFormat(&default_format, &environment->env->list_format))
			IPC_Command(&main_ipc, MAINCMD_SAVE_ENV, 0, environment->path, 0, REPLY_NO_PORT);

		// Allocate format
		if ((pass_format = AllocVec(sizeof(ListFormat), 0)))
		{
			// Copy format in
			CopyMem((char *)&format, (char *)pass_format, sizeof(ListFormat));
		}
	}

	// Send message to say goodbye; bits 8+ name the 1-based dual panel
	return IPC_Command(lister->ipc,
					   LISTER_CONFIGURE,
					   function_path_side(handle->source_paths.current) << 8,
					   (APTR)(IPTR)ret,
					   pass_format,
					   0);
}
