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
#include "music.h"

#define is_digit(c) ((c) >= '0' && (c) <= '9')

// Read the filetype files into the list; the list must be locked exclusively
static void filetype_read_files(APTR memory, struct ListLock *main_list)
{
	Cfg_FiletypeList *list;
	struct AnchorPath *anchor;
	long error;

	// Allocate anchor path
	if (!(anchor = AllocMemH(memory, sizeof(struct AnchorPath) + 256)))
		return;

	// Flush the filetype cache
	ClearFiletypeCache();

	// Initially mark all existing filetype lists as "not found"
	for (list = (Cfg_FiletypeList *)main_list->list.lh_Head; list->node.ln_Succ;
		 list = (Cfg_FiletypeList *)list->node.ln_Succ)
	{
		if (!(list->flags & FTLISTF_INTERNAL))
			list->flags |= FTLISTF_NOT_FOUND;
	}

	// Initialise anchor path
	anchor->ap_Strlen = 256;
	anchor->ap_BreakBits = IPCSIG_QUIT;

	// Search for filetypes
	error = MatchFirst("DOpus5:Filetypes/~(#?.info)", anchor);

	// Continue while there's files
	while (!error)
	{
		// See if this file is already in the list
		for (list = (Cfg_FiletypeList *)main_list->list.lh_Head; list->node.ln_Succ;
			 list = (Cfg_FiletypeList *)list->node.ln_Succ)
		{
			// Compare name and datestamp
			if (!(list->flags & FTLISTF_INTERNAL) && stricmp(list->path, anchor->ap_Buf) == 0 &&
				CompareDates(&anchor->ap_Info.fib_Date, &list->date) == 0)
			{
				list->flags &= ~FTLISTF_NOT_FOUND;
				break;
			}
		}

		// Not in the list?
		if (!list->node.ln_Succ)
		{
			// Read new filetype list
			if ((list = ReadFiletypes(anchor->ap_Buf, memory)))
			{
				// Add to list
				Enqueue(&main_list->list, &list->node);
			}
		}

		// Find next file in directory
		error = MatchNext(anchor);
	}

	// Clean up match stuff
	MatchEnd(anchor);

	// Go through list again
	for (list = (Cfg_FiletypeList *)main_list->list.lh_Head; list->node.ln_Succ;)
	{
		Cfg_FiletypeList *next = (Cfg_FiletypeList *)list->node.ln_Succ;

		// If not found bit is set, free this entry
		if (!(list->flags & FTLISTF_INTERNAL) && list->flags & FTLISTF_NOT_FOUND)
		{
			Remove(&list->node);
			FreeFiletypeList(list);
		}

		// Get next entry
		list = next;
	}

	// Free anchor
	FreeMemH(anchor);
}

// Read filetype list
void filetype_read_list(APTR memory, struct ListLock *main_list)
{
	lock_listlock(main_list, TRUE);
	filetype_read_files(memory, main_list);
	unlock_listlock(main_list);
}

static BOOL filetype_reader_locked;

// Lock the list before IPC_Launch() returns, so nothing sees it half read
IPC_StartupCode(filetype_reader_init, APTR, memory, static)
{
	lock_listlock(&GUI->filetypes, TRUE);
	filetype_reader_locked = TRUE;
	return 1;
}

// Reads the filetype files while startup carries on
IPC_EntryCode(filetype_reader)
{
	IPCData *ipc;
	APTR memory = 0;

	ipc = IPC_ProcStartup((IPTR *)&memory, (ULONG (*)(IPCData *, APTR))&filetype_reader_init);

	// The list is locked even if the rest of the startup failed
	if (filetype_reader_locked)
	{
		if (ipc)
			filetype_read_files(memory, &GUI->filetypes);
		filetype_reader_locked = FALSE;
		unlock_listlock(&GUI->filetypes);
	}

	if (ipc)
	{
		IPC_Flush(ipc);
		IPC_Goodbye(ipc, &main_ipc, 0);
	}
	IPC_Free(ipc);
}

// Read the filetype list in the background; anything that locks the list waits for it.
// Opening 70-odd small files is mostly disk seeks, which the rest of startup can overlap.
// The name differs from the "dopus_filetypes" rescan process that file notification
// starts: misc_startup() would find this reader under that name and skip the rescan.
void filetype_read_list_async(APTR memory)
{
	if (!IPC_Launch(&GUI->process_list,
					0,
					"dopus_filetype_reader",
					(IPTR)&filetype_reader,
					STACK_DEFAULT,
					(IPTR)memory,
					(struct Library *)DOSBase))
		filetype_read_list(memory, &GUI->filetypes);
}

// Create default filetype list
void filetype_default_list(APTR memory, struct ListLock *main_list)
{
	Cfg_FiletypeList *list;
	Cfg_Filetype *type;

	// Lock list
	lock_listlock(main_list, TRUE);

	// Create a new filetype list
	if ((list = AllocMemH(memory, sizeof(Cfg_FiletypeList))))
	{
		// Initialise list
		list->node.ln_Pri = -125;
		NewList(&list->filetype_list);
		list->flags = FTLISTF_INTERNAL;

		// Create default icon filetype
		if ((type = filetype_default_new(memory, list, "Icon", "ICON", 11, "iconinfo", 0)))
		{
			// Recognition string for executable
			type->recognition[0] = FTOP_MATCH;
			strcpy(type->recognition + 1, "$e3100001");
		}

		// Create default ILBM filetype
		if ((type = filetype_default_new(memory, list, "Picture, ILBM", "ILBM", 6, "show", 0)))
		{
			// Recognition string for ILBM
			type->recognition[0] = FTOP_MATCHFORM;
			strcpy(type->recognition + 1, "ILBM");
		}

		// Create default picture filetype (if we have datatypes)
		if (DataTypesBase)
		{
			if ((type = filetype_default_new(memory, list, "Picture", "PICTURE", 6, "show", 0)))
			{
				// Recognition string for Picture
				type->recognition[0] = FTOP_MATCHDTGROUP;
				strcpy(type->recognition + 1, "pict");
			}
		}

		// Create default ANIM filetype
		if ((type = filetype_default_new(memory, list, "Animation, ANIM", "ANIM", 6, "show", 0)))
		{
			// Recognition string for ANIM
			type->recognition[0] = FTOP_MATCHFORM;
			strcpy(type->recognition + 1, "ANIM");
		}

		// Create default 8SVX filetype
		if ((type = filetype_default_new(memory, list, "Sound, 8SVX", "8SVX", 6, "play", 0)))
		{
			// Recognition string for 8SVX
			type->recognition[0] = FTOP_MATCHFORM;
			strcpy(type->recognition + 1, "8SVX");
		}

		// Create default sound filetype (if we have datatypes)
		if (DataTypesBase)
		{
			if ((type = filetype_default_new(memory, list, "Sound", "SOUND", 6, "play", 0)))
			{
				// Recognition string for Sound
				type->recognition[0] = FTOP_MATCHDTGROUP;
				strcpy(type->recognition + 1, "soun");
			}
		}

#ifndef __AROS__
		// Create default 68k executable filetype
		if ((type = filetype_default_new(memory, list, "Program", "EXEC", 11, "run", 0)))
		{
			// Recognition string for executable
			type->recognition[0] = FTOP_MATCH;
			strcpy(type->recognition + 1, "$000003f3");
		}
#endif

		// Create default ELF executable filetype
		if ((type = filetype_default_new(memory, list, "ELF_Program", "EXEC", 11, "run", 0)))
		{
			// Recognition string for executable
			type->recognition[0] = FTOP_MATCH;
			strcpy(type->recognition + 1, "$7f454c46");
		}

		// Add filetype list to main list
		Enqueue(&main_list->list, &list->node);
	}

	// Create a new filetype list
	if ((list = AllocMemH(memory, sizeof(Cfg_FiletypeList))))
	{
		// Initialise list
		list->node.ln_Pri = 125;
		NewList(&list->filetype_list);
		list->flags = FTLISTF_INTERNAL;

		// Create default startmenu filetype
		if ((type = filetype_default_new(
				 memory, list, "Opus 5 Start Menu", "STRTMEN", 30, "loadbuttons start", "DOpus5:Icons/startmenu.info")))
		{
			// Recognition string for buttons
			lsprintf(type->recognition,
					 "%lcOPUS%lc%lcBTNW%lc%lcSTRT%lc%lcBPIC%lc%lcSTRT",
					 FTOP_MATCHFORM,
					 FTOP_AND,
					 FTOP_MATCHCHUNK,
					 FTOP_AND,
					 FTOP_MATCHCHUNK,
					 FTOP_OR,
					 FTOP_MATCHCHUNK,
					 FTOP_AND,
					 FTOP_MATCHCHUNK);
		}

		// Create default button filetype
		if ((type = filetype_default_new(
				 memory, list, "Opus 5 Buttons", "BUTTONS", 12, "loadbuttons", "DOpus5:Icons/buttons.info")))
		{
			// Recognition string for buttons
			lsprintf(type->recognition, "%lcOPUS%lc%lcBTNW", FTOP_MATCHFORM, FTOP_AND, FTOP_MATCHCHUNK);
		}

		// Create default environment filetype
		if ((type = filetype_default_new(memory,
										 list,
										 "Opus 5 Environment",
										 "ENVIRON",
										 12,
										 "loadenvironment",
										 "DOpus5:Icons/environment.info")))
		{
			// Recognition string for environment
			lsprintf(type->recognition, "%lcOPUS%lc%lcENVR", FTOP_MATCHFORM, FTOP_AND, FTOP_MATCHCHUNK);
		}

		// Create default filetype filetype
		if ((type = filetype_default_new(
				 memory, list, "Opus 5 Filetype", "FTYPE", 12, "editfiletype", "DOpus5:Icons/filetype.info")))
		{
			// Recognition string for filetypes
			lsprintf(type->recognition, "%lcOPUS%lc%lcTYPE", FTOP_MATCHFORM, FTOP_AND, FTOP_MATCHCHUNK);
		}

		// Create default command filetype
		if ((type = filetype_default_new(
				 memory, list, "Opus 5 Command", "CMD", 16, "runcommand", "DOpus5:Icons/command.info")))
		{
			// Recognition string for command
			lsprintf(type->recognition,
					 "%lcOPUS%lc%lc12%lc%lcFUNC",
					 FTOP_MATCHFORM,
					 FTOP_AND,
					 FTOP_MOVE,
					 FTOP_ENDSECTION,
					 FTOP_MATCH);

			// Add Edit menus
			filetype_default_menu(memory, type, GetString(&locale, MSG_EDIT_COMMAND), "editcommand");

			// Save pointer to filetype
			command_filetype = type;
		}

#ifndef __AROS__
		// Create default module filetype (no function, recognition only)
		if ((type =
				 filetype_default_new(memory, list, "Opus 5 Module 68k", "MODULE", 21, 0, "DOpus5:Icons/module.info")))
		{
			// Recognition string for modules
			lsprintf(type->recognition, "%lc*.module%lc%lc$000003f3", FTOP_MATCHNAME, FTOP_AND, FTOP_MATCH);
		}
#endif

		if ((type =
				 filetype_default_new(memory, list, "Opus 5 Module ELF", "MODULE", 21, 0, "DOpus5:Icons/module.info")))
		{
			// Recognition string for modules
			lsprintf(type->recognition, "%lc*.module%lc%lc$7f454c46", FTOP_MATCHNAME, FTOP_AND, FTOP_MATCH);
		}

		// Create default rexx module filetype (no function, recognition only)
		if ((type = filetype_default_new(memory, list, "Opus 5 Module", "MODULE", 14, 0, "DOpus5:Icons/module.info")))
		{
			// Recognition string for modules
			lsprintf(type->recognition, "%lc*.dopus5%lc%lc/*", FTOP_MATCHNAME, FTOP_AND, FTOP_MATCH);
		}

		// Font prefs
		if ((type = filetype_default_new(memory, list, "Font Prefs", "FONTS", 18, "loadfonts", 0)))
		{
			// Recognition string for fonts
			lsprintf(type->recognition,
					 "%lcPREF%lc%lcPRHD%lc%lcFONT",
					 FTOP_MATCHFORM,
					 FTOP_AND,
					 FTOP_MATCHCHUNK,
					 FTOP_AND,
					 FTOP_MATCHCHUNK);
		}

		// Add filetype list to main list
		Enqueue(&main_list->list, &list->node);
	}

	// Create a new filetype list
	if ((list = AllocMemH(memory, sizeof(Cfg_FiletypeList))))
	{
		// Initialise list
		list->node.ln_Pri = -127;
		NewList(&list->filetype_list);
		list->flags = FTLISTF_INTERNAL;

		// Final, default filetype
		if ((type =
				 filetype_default_new(memory, list, GetString(&locale, MSG_UNKNOWN_TYPE), "DEFAULT", 4, "defftype", 0)))
		{
			Cfg_Function *func;

			// Recognition string for default
			type->recognition[0] = FTOP_MATCHNAME;
			strcpy(type->recognition + 1, "#?");

			// Create a second function
			if ((func = NewFunction(memory, FTTYPE_DRAG_DROP)))
			{
				Cfg_Instruction *ins;

				AddTail(&type->function_list, &func->node);
				if ((ins = NewInstruction(memory, INST_COMMAND, "copy")))
					AddTail((struct List *)&func->instructions, (struct Node *)ins);
			}

			// Global pointer to default filetype (ick!)
			default_filetype = type;
		}

		// Default filetype to run things
		if ((type = filetype_default_new(memory, list, "DEFAULT", "DEFAULT", 4, "run", 0)))
		{
			// Recognition string for default
			type->recognition[0] = FTOP_MATCHNAME;
			strcpy(type->recognition + 1, "#?");

			// Global pointer to default filetype (ick!)
			run_filetype = type;
		}

		// Add filetype list to main list
		Enqueue(&main_list->list, &list->node);
	}

	// Unlock list
	unlock_listlock(main_list);
}

// Create a new default filetype
Cfg_Filetype *filetype_default_new(APTR memory,
								   Cfg_FiletypeList *list,
								   char *name,
								   char *id,
								   short recog_size,
								   char *command,
								   char *icon)
{
	Cfg_Filetype *type;
	Cfg_Function *func;
	Cfg_Instruction *ins;

	// Allocate type memory
	if ((type = NewFiletype(memory)) && (type->recognition = AllocMemH(memory, recog_size)))
	{
		// Initialise type
		strcpy(type->type.name, name);
		strcpy(type->type.id, id);

		// Default instruction
		if (command)
		{
			type->type.count = 1;
			if ((func = NewFunction(memory, FTTYPE_DOUBLE_CLICK)))
			{
				AddTail(&type->function_list, &func->node);
				if ((ins = NewInstruction(memory, INST_COMMAND, command)))
					AddTail((struct List *)&func->instructions, (struct Node *)ins);
			}
		}

		// Icon?
		if (icon && (type->icon_path = AllocMemH(memory, strlen(icon) + 1)))
			strcpy(type->icon_path, icon);

		// Add to list
		AddTail(&list->filetype_list, &type->node);
		type->list = list;
	}

	return type;
}

// Add a filetype menu item
void filetype_default_menu(APTR memory, Cfg_Filetype *type, char *name, char *command)
{
	Cfg_Function *func;
	Cfg_Instruction *ins;

	// Allocate function
	if ((func = NewFunction(memory, 0)))
	{
		// Set 'menu' flag
		func->function.flags2 |= FUNCF2_LABEL_FUNC;

		// Add to list
		AddTail(&type->function_list, &func->node);

		// Increment function count
		type->type.count++;

		// Create label instruction
		if ((ins = NewInstruction(memory, INST_LABEL, name)))
			AddTail((struct List *)&func->instructions, (struct Node *)ins);

		// Create command instruction
		if ((ins = NewInstruction(memory, INST_COMMAND, command)))
			AddTail((struct List *)&func->instructions, (struct Node *)ins);
	}
}

// Try and match a file to a filetype
Cfg_Filetype *filetype_identify(char *file, short match_type, char *name, unsigned short qual)
{
	Cfg_FiletypeList *list;
	Cfg_Filetype *type = 0;
	MatchHandle *handle;

	// Clear name if supplied
	if (name)
		*name = 0;

	// Get match handle for the file
	if (!(handle = GetMatchHandle(file)))
		return 0;

	// Lock filetype lists
	lock_listlock(&GUI->filetypes, FALSE);

	// Go through filetype lists
	for (list = (Cfg_FiletypeList *)GUI->filetypes.list.lh_Head; list->node.ln_Succ;
		 list = (Cfg_FiletypeList *)list->node.ln_Succ)
	{
		// Go through filetypes in this list
		for (type = (Cfg_Filetype *)list->filetype_list.lh_Head; type->node.ln_Succ;
			 type = (Cfg_Filetype *)type->node.ln_Succ)
		{
			BOOL try_match = 0;

			// Match anything?
			if (match_type == FTTYPE_ANY || match_type == FTTYPE_WANT_NAME)
				try_match = 1;

			// Need an icon to match?
			else if (match_type == FTTYPE_ICON)
			{
				if (type->icon_path && type->icon_path[0])
					try_match = 1;
			}

			// Need a function to match
			else if (FindFunctionType(&type->function_list, match_type))
				try_match = 1;

			// Try to match if we can
			if (try_match && MatchFiletype(handle, type))
				break;
		}

		// Did something match?
		if (type->node.ln_Succ)
			break;
		type = 0;
	}

	// If file is a directory and we didn't match, return default type
	if (handle->fib.fib_DirEntryType > 0 && !type)
	{
		// Free things
		FreeMatchHandle(handle);

		// Unlock filetype lists
		unlock_listlock(&GUI->filetypes);

		// If we're after a name or an icon, return failure
		if (match_type == FTTYPE_WANT_NAME || match_type == FTTYPE_ICON)
			return 0;

		// Return default filetype
		return default_filetype;
	}

	// Did we match a file on a double-click?
	if (handle->fib.fib_DirEntryType < 0 && match_type == FTTYPE_DOUBLE_CLICK)
	{
		struct DiskObject *icon;

		// Shift held down?
		if (qual & IEQUAL_ANYSHIFT)
		{
			// If file isn't an icon, try and get icon for it
			if (!(isicon(file)))
			{
        		icon = GetDiskObject(file);
        		if (!icon)
        		{
        			//if file has no icon attempt to get the deficons default icon
          			if (IconBase->lib_Version >= 44)
            		{
               			icon = GetIconTags(file, ICONGETA_FailIfUnavailable, FALSE, TAG_DONE);
            		}
        		}
        		if (icon)
        		{
          			// Is icon a project, with a default tool?
          			if (icon->do_Type == WBPROJECT && icon->do_DefaultTool && icon->do_DefaultTool[0])
          			{
	            		// Use 'run' filetype
            			type = run_filetype;
          			}

	          		// Free icon
        		  	FreeDiskObject(icon);
		        }
			}
		}
	}

	// Got a buffer for the type?
	if (name)
	{
		// Do we have datatypes.library?
		if (DataTypesBase)
		{
			// No match (or default match)
			if (!type || strcmp(type->type.id, "DEFAULT") == 0)
			{
				// Want name?
				if (match_type == FTTYPE_WANT_NAME)
				{
					// If we didn't try for a datatype before, we can try now
					if (!(handle->flags & MATCHF_TRIED_DT))
						handle->datatype = ObtainDataTypeA(DTST_FILE, (APTR)handle->lock, 0);
				}

				// Do we have a valid datatype?
				if (handle->datatype)
				{
					// Copy datatype name
					stccpy(name, handle->datatype->dtn_Header->dth_Name, 31);
					type = 0;
				}
			}
		}

		// Name from filetype?
		if (type)
			strcpy(name, type->type.name);
	}

	// Unlock filetype lists
	unlock_listlock(&GUI->filetypes);

	// Free handle and return type
	FreeMatchHandle(handle);
	return type;
}

// Find a filetype by description
Cfg_Filetype *filetype_find(char *desc, short mtype)
{
	Cfg_FiletypeList *list;
	Cfg_Filetype *type, *match_type = 0;

	// Lock filetype lists
	lock_listlock(&GUI->filetypes, FALSE);

	// Go through filetype lists
	for (list = (Cfg_FiletypeList *)GUI->filetypes.list.lh_Head; list->node.ln_Succ;
		 list = (Cfg_FiletypeList *)list->node.ln_Succ)
	{
		// Go through filetypes in this list
		for (type = (Cfg_Filetype *)list->filetype_list.lh_Head; type->node.ln_Succ;
			 type = (Cfg_Filetype *)type->node.ln_Succ)
		{
			char *ptr;

			// Get pointer depending on match type
			ptr = (mtype == 1) ? type->type.id : type->type.name;

			// Compare description
			if (stricmp(ptr, desc) == 0)
			{
				match_type = type;
				break;
			}
		}

		// Did something match?
		if (type->node.ln_Succ)
			break;
	}

	// Unlock filetype lists
	unlock_listlock(&GUI->filetypes);

	return match_type;
}

// Check a filetype is still in the list
BOOL filetype_check(Cfg_Filetype *match_type)
{
	Cfg_FiletypeList *list;
	Cfg_Filetype *type;
	BOOL ok = 0;

	// Lock filetype lists
	lock_listlock(&GUI->filetypes, FALSE);

	// Go through filetype lists
	for (list = (Cfg_FiletypeList *)GUI->filetypes.list.lh_Head; list->node.ln_Succ;
		 list = (Cfg_FiletypeList *)list->node.ln_Succ)
	{
		// Go through filetypes in this list
		for (type = (Cfg_Filetype *)list->filetype_list.lh_Head; type->node.ln_Succ;
			 type = (Cfg_Filetype *)type->node.ln_Succ)
		{
			// Match pointer
			if (type == match_type)
			{
				ok = 1;
				break;
			}
		}

		// Did something match?
		if (type->node.ln_Succ)
			break;
	}

	// Unlock filetype lists
	unlock_listlock(&GUI->filetypes);

	return ok;
}

// See if a filetype is the 'default'
BOOL is_default_filetype(Cfg_Filetype *type)
{
	unsigned char *ptr;

	// Valid?
	if (!type)
		return 0;

	// Match pointer?
	if (type == default_filetype)
		return 1;

	// Get recognition string
	if (!(ptr = type->recognition))
		return 0;

	// Match name?
	if (ptr[0] != FTOP_MATCHNAME)
		return 0;

	// #?
	if (ptr[1] == '#' && ptr[2] == '?')
	{
		// Bump pointer
		ptr += 3;
	}

	// *
	else if (ptr[1] == '*')
	{
		// Bump pointer
		ptr += 2;
	}

	// Fail
	else
		return 0;

	// That's all?
	if (ptr[0] != 0 && ptr[0] <= FTOP_SPECIAL)
		return 0;

	return 1;
}

// See if a file matches a given filetype
BOOL filetype_match_type(char *filename, Cfg_Filetype *match_type)
{
	Cfg_FiletypeList *list;
	Cfg_Filetype *type;
	MatchHandle *handle;
	BOOL ok = 0, done = 0;

	// Get match handle for the file
	if (!(handle = GetMatchHandle(filename)))
		return 0;

	// Lock filetype lists
	lock_listlock(&GUI->filetypes, FALSE);

	// Go through filetype lists
	for (list = (Cfg_FiletypeList *)GUI->filetypes.list.lh_Head; list->node.ln_Succ;
		 list = (Cfg_FiletypeList *)list->node.ln_Succ)
	{
		// Go through filetypes in this list
		for (type = (Cfg_Filetype *)list->filetype_list.lh_Head; type->node.ln_Succ;
			 type = (Cfg_Filetype *)type->node.ln_Succ)
		{
			// Found filetype?
			if (type == match_type)
			{
				// Test it
				if (MatchFiletype(handle, type))
				{
					// Matched
					ok = 1;
				}

				// Done with the search
				done = 1;
				break;
			}
		}

		// Done?
		if (done)
			break;
	}

	// Unlock filetype lists
	unlock_listlock(&GUI->filetypes);

	// Free handle
	FreeMatchHandle(handle);

	return ok;
}
