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

#include "dopuslib.h"
#include "config.h"
#include <Program/dopus_config.h>

long write_string(struct _IFFHandle *iff, char *string);
long write_button_list(struct _IFFHandle *iff, struct List *list, long flags);
short write_function_list(struct _IFFHandle *iff, struct List *list);
extern void do_backup(char *name);

#ifdef __AROS__
#define AROS_CFG_BUTN_DISK_SIZE 14
#endif

int LIBFUNC L_SaveSettings(REG(a0, CFG_SETS *settings), REG(a1, char *name), REG(a6, struct MyLibrary *libbase))
{
	struct _IFFHandle *iff;
	int success = 0;
#ifdef __AROS__
	CFG_SETS *settings_be;
#endif

#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Check valid config and name
	if (!settings || !name || !name[0])
		return -1;

#ifdef __AROS__
	if (!(settings_be = AllocVec(sizeof(CFG_SETS), MEMF_CLEAR)))
		return -1;
#endif

	// Try to open file to write
	if ((iff = L_IFFOpen(name, MODE_NEWFILE, ID_OPUS)))
	{
		char buf[40];

		// Write header chunk
#ifdef __AROS__
		CopyMem(settings, settings_be, sizeof(CFG_SETS));

		settings_be->copy_flags = AROS_LONG2BE(settings_be->copy_flags);
		settings_be->delete_flags = AROS_LONG2BE(settings_be->delete_flags);
		settings_be->error_flags = AROS_LONG2BE(settings_be->error_flags);
		settings_be->general_flags = AROS_LONG2BE(settings_be->general_flags);
		settings_be->icon_flags = AROS_LONG2BE(settings_be->icon_flags);
		settings_be->replace_method = AROS_WORD2BE(settings_be->replace_method);
		settings_be->replace_flags = AROS_WORD2BE(settings_be->replace_flags);
		settings_be->update_flags = AROS_LONG2BE(settings_be->update_flags);
		settings_be->dir_flags = AROS_LONG2BE(settings_be->dir_flags);
		settings_be->view_flags = AROS_LONG2BE(settings_be->view_flags);
		settings_be->max_buffer_count = AROS_WORD2BE(settings_be->max_buffer_count);
		settings_be->date_format = AROS_WORD2BE(settings_be->date_format);
		settings_be->date_flags = AROS_WORD2BE(settings_be->date_flags);
		settings_be->pop_code = AROS_WORD2BE(settings_be->pop_code);
		settings_be->pop_qual = AROS_WORD2BE(settings_be->pop_qual);
		settings_be->pop_qual_mask = AROS_WORD2BE(settings_be->pop_qual_mask);
		settings_be->pop_qual_same = AROS_WORD2BE(settings_be->pop_qual_same);
		settings_be->popup_delay = AROS_WORD2BE(settings_be->popup_delay);
		settings_be->max_openwith = AROS_WORD2BE(settings_be->max_openwith);
		settings_be->command_line_length = AROS_WORD2BE(settings_be->command_line_length);
		settings_be->max_filename = AROS_WORD2BE(settings_be->max_filename);
		settings_be->flags = AROS_LONG2BE(settings_be->flags);

		if (L_IFFWriteChunk(iff, (char *)settings_be, ID_SETS, sizeof(CFG_SETS)))
#else
		if (L_IFFWriteChunk(iff, (char *)settings, ID_SETS, sizeof(CFG_SETS)))
#endif
		{
			// Succeeded
			success = 1;

			// Write icons?
			if ((GetVar("dopus/dopus", buf, sizeof(buf), GVF_GLOBAL_ONLY) > 0) && strstr(buf, "-icons-"))
			{
				// Write icon file
				L_WriteFileIcon("DOpus5:Icons/Settings", name, libbase);
			}
		}
	}

	if (!success)
		success = IoErr();
	else
		success = 0;

	L_IFFClose(iff);

#ifdef __AROS__
	FreeVec(settings_be);
#endif

	return success;
}

long LIBFUNC L_SaveListerDef(REG(a0, struct _IFFHandle *iff), REG(a1, Cfg_Lister *lister))
{
#ifdef __AROS__
	CFG_LSTR *lister_be;
#endif

	if (!iff || !lister)
		return 0;

#ifdef __AROS__
	if (!(lister_be = AllocVec(sizeof(CFG_LSTR), MEMF_CLEAR)))
		return 0;

	CopyMem(&lister->lister, lister_be, sizeof(CFG_LSTR));

	lister_be->icon_x = AROS_LONG2BE(lister_be->icon_x);
	lister_be->icon_y = AROS_LONG2BE(lister_be->icon_y);
	lister_be->pos[0].Left = AROS_WORD2BE(lister_be->pos[0].Left);
	lister_be->pos[0].Top = AROS_WORD2BE(lister_be->pos[0].Top);
	lister_be->pos[0].Width = AROS_WORD2BE(lister_be->pos[0].Width);
	lister_be->pos[0].Height = AROS_WORD2BE(lister_be->pos[0].Height);
	lister_be->pos[1].Left = AROS_WORD2BE(lister_be->pos[1].Left);
	lister_be->pos[1].Top = AROS_WORD2BE(lister_be->pos[1].Top);
	lister_be->pos[1].Width = AROS_WORD2BE(lister_be->pos[1].Width);
	lister_be->pos[1].Height = AROS_WORD2BE(lister_be->pos[1].Height);
	lister_be->flags = AROS_LONG2BE(lister_be->flags);

	// Write lister information
	if (!(L_IFFPushChunk(iff, ID_LSTR)) || !(L_IFFWriteChunkBytes(iff, (char *)lister_be, sizeof(CFG_LSTR))))
	{
		FreeVec(lister_be);
		return 0;
	}

	FreeVec(lister_be);
#else
	// Write lister information
	if (!(L_IFFPushChunk(iff, ID_LSTR)) || !(L_IFFWriteChunkBytes(iff, (char *)&lister->lister, sizeof(CFG_LSTR))))
		return 0;
#endif

	// Write lister path
	if (lister->path && lister->path[0] && !(write_string(iff, lister->path)))
		return 0;

	// Pop chunk
	return L_IFFPopChunk(iff);
}

int LIBFUNC L_SaveButtonBank(REG(a0, Cfg_ButtonBank *bank), REG(a1, char *name), REG(a6, struct MyLibrary *libbase))
{
	struct _IFFHandle *iff;
	BPTR lock = 0;
	short success = 0;
#ifdef __AROS__
	CFG_BTNW *window_be;
	CFG_STRT *startmenu_be = NULL;
#endif

#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Check valid bank and name
	if (!bank || !name || !name[0])
		return -1;

#ifdef __AROS__
	if (!(window_be = AllocVec(sizeof(CFG_BTNW), MEMF_CLEAR)))
		return -1;

	if (bank->startmenu && !(startmenu_be = AllocVec(sizeof(CFG_STRT), MEMF_CLEAR)))
	{
		FreeVec(window_be);
		return -1;
	}
#endif

	if ((lock = Lock(name, SHARED_LOCK)))
	{
		UnLock(lock);
		do_backup(name);
	}

	// Try to open file to write
	while ((iff = L_IFFOpen(name, MODE_NEWFILE, ID_EPUS)))
	{
		char buf[40];

		// Clear change flag
		bank->window.flags &= ~BTNWF_CHANGED;

		// Check columns/rows for zero
		if (bank->window.columns == 0)
			bank->window.columns = 1;
		if (bank->window.rows == 0)
			bank->window.rows = 1;

		// Get bank path name
		if (!(NameFromFH(L_FHFromBuf(L_IFFFileHandle(iff)), bank->path, 256)))
			strcpy(bank->path, name);

			// Write header chunk
#ifdef __AROS__
		CopyMem(&bank->window, window_be, sizeof(CFG_BTNW));

		window_be->pos.Left = AROS_WORD2BE(window_be->pos.Left);
		window_be->pos.Top = AROS_WORD2BE(window_be->pos.Top);
		window_be->pos.Width = AROS_WORD2BE(window_be->pos.Width);
		window_be->pos.Height = AROS_WORD2BE(window_be->pos.Height);
		window_be->columns = AROS_WORD2BE(window_be->columns);
		window_be->rows = AROS_WORD2BE(window_be->rows);
		window_be->flags = AROS_LONG2BE(window_be->flags);

		if (!(L_IFFWriteChunk(iff, (char *)window_be, ID_BTNW, sizeof(CFG_BTNW))))
#else
		if (!(L_IFFWriteChunk(iff, (char *)&bank->window, ID_BTNW, sizeof(CFG_BTNW))))
#endif
			break;

		// Write background picture
		if (bank->backpic[0] && !(L_IFFWriteChunk(iff, bank->backpic, ID_BPIC, strlen(bank->backpic) + 1)))
			break;

		// Start menu?
		if (bank->startmenu)
		{
			// Write STRT chunk
			bank->startmenu->flags |= STRTF_VERSION2;
#ifdef __AROS__
			CopyMem(bank->startmenu, startmenu_be, sizeof(CFG_STRT));

			startmenu_be->flags = AROS_LONG2BE(startmenu_be->flags);
			startmenu_be->fpen = AROS_WORD2BE(startmenu_be->fpen);
			startmenu_be->label_fpen = AROS_WORD2BE(startmenu_be->label_fpen);
			startmenu_be->sel_fpen = AROS_WORD2BE(startmenu_be->sel_fpen);
			startmenu_be->label_fontsize = AROS_WORD2BE(startmenu_be->label_fontsize);

			if (!(L_IFFWriteChunk(iff, (char *)startmenu_be, ID_STRT, sizeof(CFG_STRT))))
#else
			if (!(L_IFFWriteChunk(iff, (char *)bank->startmenu, ID_STRT, sizeof(CFG_STRT))))
#endif
				break;
		}

		// Write button list
		if (!(write_button_list(iff, &bank->buttons, bank->window.flags)))
			break;

		// Succeeded
		success = 1;

		// Write icons?
		if ((GetVar("dopus/dopus", buf, sizeof(buf), GVF_GLOBAL_ONLY) > 0) && strstr(buf, "-icons-"))
		{
			// Write icon file
			L_WriteFileIcon("DOpus5:Icons/Buttons", name, libbase);
		}
		break;
	}

	if (!success)
		success = IoErr();
	else
		success = 0;

	L_IFFClose(iff);

#ifdef __AROS__
	FreeVec(window_be);
	FreeVec(startmenu_be);
#endif

	return success;
}

// Create the missing drawers in the path of a file that is about to be written.
// Returns TRUE if at least one drawer was created.
static BOOL create_parent_drawers(char *name)
{
	char path[256], *start, *ptr;
	BPTR lock;
	BOOL created = FALSE;

	stccpy(path, name, sizeof(path));
	*PathPart(path) = 0;

	// Skip the device or assign name
	if ((start = strchr(path, ':')))
		++start;
	else
		start = path;

	// Lock or create each drawer in turn
	for (ptr = start;; ptr++)
	{
		if (*ptr == '/' || !*ptr)
		{
			char ch = *ptr;

			// Skip empty components (parent references)
			if (ptr > start && ptr[-1] != '/')
			{
				*ptr = 0;
				if ((lock = Lock(path, ACCESS_READ)))
					UnLock(lock);
				else if ((lock = CreateDir(path)))
				{
					UnLock(lock);
					created = TRUE;
				}
				else
					break;
				*ptr = ch;
			}

			if (!ch)
				break;
		}
	}

	return created;
}

int LIBFUNC L_SaveFiletypeList(REG(a0, Cfg_FiletypeList *list), REG(a1, char *name), REG(a6, struct MyLibrary *libbase))
{
	struct _IFFHandle *iff;
	Cfg_Filetype *type;
	int success = 0;
#ifdef __AROS__
	CFG_TYPE type_be;
#endif

#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Check valid list and name
	if (!list || list->flags & FTLISTF_INTERNAL || !name || !name[0])
		return -1;

	// Try to open file to write; if the drawer is missing, create it and try again
	if (!(iff = L_IFFOpen(name, MODE_NEWFILE, ID_OPUS)))
	{
		LONG error = IoErr();

		if (!create_parent_drawers(name))
			return error;
		iff = L_IFFOpen(name, MODE_NEWFILE, ID_OPUS);
	}

	while (iff)
	{
		char buf[40];

		// Go through filetypes in list
		for (type = (Cfg_Filetype *)list->filetype_list.lh_Head; type->node.ln_Succ;
			 type = (Cfg_Filetype *)type->node.ln_Succ)
		{
			short a;

			// Write filetype data
#ifdef __AROS__
			CopyMem(&type->type, &type_be, sizeof(CFG_TYPE));

			type_be.flags = AROS_WORD2BE(type_be.flags);
			type_be.count = AROS_WORD2BE(type_be.count);

			if (!(L_IFFPushChunk(iff, ID_TYPE)) || !(L_IFFWriteChunkBytes(iff, (char *)&type_be, sizeof(CFG_TYPE))))
#else
			if (!(L_IFFPushChunk(iff, ID_TYPE)) || !(L_IFFWriteChunkBytes(iff, (char *)&type->type, sizeof(CFG_TYPE))))
#endif
				break;

			// Write recognition and icon path
			if (!(write_string(iff, (char *)type->recognition)) || !(write_string(iff, type->icon_path)))
				break;

			// Write actions
			for (a = 0; a < 16; a++)
			{
				if (!(write_string(iff, (type->actions) ? type->actions[a] : 0)))
					break;
			}
			if (a < type->type.count || !(L_IFFPopChunk(iff)))
				break;

			// Write function list
			if (!(write_function_list(iff, &type->function_list)))
				break;
		}

		// Failure?
		if (type->node.ln_Succ)
			break;

		// Succeeded
		success = 1;

		// Write icons?
		if ((GetVar("dopus/dopus", buf, sizeof(buf), GVF_GLOBAL_ONLY) > 0) && strstr(buf, "-icons-"))
		{
			// Write icon file
			L_WriteFileIcon("DOpus5:Icons/Filetype", name, libbase);
		}
		break;
	}

	L_IFFClose(iff);

	if (success)
		return 0;
	return IoErr();
}

short LIBFUNC L_SaveButton(REG(a0, struct _IFFHandle *iff), REG(a1, Cfg_Button *button))
{
	Cfg_ButtonFunction *func;
#ifdef __AROS__
	UBYTE button_be[AROS_CFG_BUTN_DISK_SIZE];
#endif

	// Count number of functions
	button->button.count = L_Att_NodeCount((Att_List *)&button->function_list);

	// Set "new style" flag
	button->button.flags |= BUTNF_NEW_FORMAT;

	// Write button data
#ifdef __AROS__
	button_be[0] = button->button.fpen;
	button_be[1] = button->button.bpen;
	button_be[2] = (UBYTE)(button->button.flags >> 24);
	button_be[3] = (UBYTE)(button->button.flags >> 16);
	button_be[4] = (UBYTE)(button->button.flags >> 8);
	button_be[5] = (UBYTE)button->button.flags;
	button_be[6] = (UBYTE)((UWORD)button->button.count >> 8);
	button_be[7] = (UBYTE)button->button.count;
	button_be[8] = (UBYTE)((UWORD)button->button.pad_1 >> 8);
	button_be[9] = (UBYTE)button->button.pad_1;
	button_be[10] = (UBYTE)(button->button.pad_2 >> 24);
	button_be[11] = (UBYTE)(button->button.pad_2 >> 16);
	button_be[12] = (UBYTE)(button->button.pad_2 >> 8);
	button_be[13] = (UBYTE)button->button.pad_2;

	if (!(L_IFFPushChunk(iff, ID_BUTN)) ||
		!(L_IFFWriteChunkBytes(iff, (char *)button_be, sizeof(button_be))))
#else
	if (!(L_IFFPushChunk(iff, ID_BUTN)) || !(L_IFFWriteChunkBytes(iff, (char *)&button->button, sizeof(CFG_BUTN))))
#endif
		return 0;

	// Go through button functions
	for (func = (Cfg_ButtonFunction *)button->function_list.mlh_Head; func->node.ln_Succ;
		 func = (Cfg_ButtonFunction *)func->node.ln_Succ)
	{
		// Write label
		if (!(write_string(iff, func->label)))
			break;

		// Is the name the same as the label?
		if (!func->node.ln_Name || (func->label && strcmp(func->label, func->node.ln_Name) == 0 && *func->label))
		{
			char special[2];

			// Special marker
			special[0] = 1;
			special[1] = 0;
			if (!(write_string(iff, special)))
				break;
		}
		else if (!(write_string(iff, func->node.ln_Name)))
			break;
	}

	// Failed?
	if (func->node.ln_Succ || !(L_IFFPopChunk(iff)))
		return 0;

	// Write functions
	return write_function_list(iff, (struct List *)&button->function_list);
}

long write_string(struct _IFFHandle *iff, char *string)
{
	int len;

	if (!string)
		string = "";
	len = strlen(string) + 1;
	return L_IFFWriteChunkBytes(iff, string, len);
}

long write_button_list(struct _IFFHandle *iff, struct List *list, long flags)
{
	Cfg_Button *button;

	// Go through button list
	for (button = (Cfg_Button *)list->lh_Head; button->node.ln_Succ; button = (Cfg_Button *)button->node.ln_Succ)
	{
		// Write button
		if (!(L_SaveButton(iff, button)))
			break;
	}

	// Fail?
	if (button->node.ln_Succ)
		return 0;
	return 1;
}

short write_function_list(struct _IFFHandle *iff, struct List *list)
{
	Cfg_Function *function;

	// Go through function list
	for (function = (Cfg_Function *)list->lh_Head; function->node.ln_Succ;
		 function = (Cfg_Function *)function->node.ln_Succ)
	{
		// Write function data
		if (!(L_SaveFunction(iff, function)))
			break;
	}

	// Fail?
	if (function->node.ln_Succ)
		return 0;
	return 1;
}

BOOL LIBFUNC L_SaveFunction(REG(a0, struct _IFFHandle *iff), REG(a1, Cfg_Function *function))
{
	Cfg_Instruction *ins;
#ifdef __AROS__
	CFG_FUNC *function_be;

	if (!(function_be = AllocVec(sizeof(CFG_FUNC), MEMF_CLEAR)))
		return 0;

	CopyMem(&function->function, function_be, sizeof(CFG_FUNC));

	function_be->flags = AROS_LONG2BE(function_be->flags);
	function_be->flags2 = AROS_LONG2BE(function_be->flags2);
	function_be->code = AROS_WORD2BE(function_be->code);
	function_be->qual = AROS_WORD2BE(function_be->qual);
	function_be->func_type = AROS_WORD2BE(function_be->func_type);
	function_be->qual_mask = AROS_WORD2BE(function_be->qual_mask);
	function_be->qual_same = AROS_WORD2BE(function_be->qual_same);

	if (!(L_IFFPushChunk(iff, ID_FUNC)) || !(L_IFFWriteChunkBytes(iff, (char *)function_be, sizeof(CFG_FUNC))))
	{
		FreeVec(function_be);
		return 0;
	}
	FreeVec(function_be);
#else
	if (!(L_IFFPushChunk(iff, ID_FUNC)) || !(L_IFFWriteChunkBytes(iff, (char *)&function->function, sizeof(CFG_FUNC))))
		return 0;
#endif

	// Go through instruction list
	for (ins = (Cfg_Instruction *)function->instructions.mlh_Head; ins->node.mln_Succ;
		 ins = (Cfg_Instruction *)ins->node.mln_Succ)
	{
#ifdef __AROS__
		short type_be = AROS_WORD2BE(ins->type);

		if (!(write_string(iff, ins->string)) || !(L_IFFWriteChunkBytes(iff, (char *)&type_be, sizeof(short))))
#else
		// Write function string
		if (!(write_string(iff, ins->string)) || !(L_IFFWriteChunkBytes(iff, (char *)&ins->type, sizeof(short))))
#endif
			return 0;
	}

	// Fail?
	if (ins->node.mln_Succ || !(L_IFFPopChunk(iff)))
		return 0;

	return 1;
}
