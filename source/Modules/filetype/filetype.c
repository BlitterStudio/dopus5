/*

Directory Opus 5
Original APL release version 5.82
Copyright 1993-2012 Jonathan Potter & GP Software
Copyright 2012-2013 DOPUS5 Open Source Team
Copyright 2023-2026 Dimitris Panokostas (dopus5 fork)

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

/*
 *	04-04-96	Removed checking for filetype named 'Default'
 *	10-05-96	Finder now hilites the best match - this is the one you can install or edit
 *	05-06-96	Now uses datatypes GroupID itself rather than localized string
 *	21-06-96	Now uses Opus' internal filetype list
 *			Finder now hilites the best match even if it isn't installed
 */

#include "filetype.h"

#include <dopus/common.h>
#include <proto/dopus5.h>
#include <proto/configopus.h>
#include <proto/module.h>
#include <dopus/lib_macros.h>

#include <math.h>

/*#ifdef __amigaos3__
extern struct ExecBase *SysBase;
#endif*/

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
//#define GET_DOPUSLIB	(struct Library *)FindName(&((struct ExecBase *)SysBase)->LibList,"dopus5.library");

// replacements for the C library memory functions, which can't be used in shared libraries
void free(void *ptr)
{
	return FreeVec(ptr);
}
char *strdup(const char *orig)
{
	char *copy = AllocVec(strlen(orig) + 1, MEMF_ANY);
	if (copy)
		CopyMem((APTR)orig, copy, strlen(orig) + 1);
	return copy;
}

#warning Should not be here - Disabled - Remove eventually.
/* Commented out - If no problems arise then remove.
typedef struct _PathNode
{
	struct MinNode		node;
	char			path_buf[512];
	char			*path;
	struct _Lister		*lister;
	struct MinList		change_list;
	UWORD			flags;
} PathNode;
*/

static Cfg_FiletypeList *creator_generic(char *args,
										 struct Screen *screen,
										 IPCData *ipc,
										 IPCData *main_ipc,
										 IPCData *opus_ipc,
										 EXT_FUNC(func_callback),
										 Att_List *list,
										 char *path);

#define FTLISTF_INSTALLED (1 << 15)

enum { GOODBYE_CONTINUE, GOODBYE_USE, GOODBYE_INSTALL, GOODBYE_CREATE, GOODBYE_EDIT, GOODBYE_SAVE, GOODBYE_ABORT };

char *version = "$VER: filetype.module 68.0 (3.10.98)";

/********************************/

char *id_to_str(LONG id, char *buf)
{
	buf[0] = (id >> 24) & 0xff;
	buf[1] = (id >> 16) & 0xff;
	buf[2] = (id >> 8) & 0xff;
	buf[3] = id & 0xff;
	buf[4] = 0;

	return buf;
}

/********************************/
#warning this should be replaced with SimpleRequest
static int STDARGS my_SimpleRequest(struct Window *window, char *title, char *buttons, char *message, char *text)
{
	char buffer[1024] = {'0'};
	lsprintf(buffer, message, (IPTR)text);

	return (int)SimpleRequest(window, title, buttons, buffer, 0, 0, 0, 0);
	// return SimpleRequest( window, title, buttons, message, 0, (APTR)(((ULONG *)&message) + 1), 0, 0);
}

/********************************/

#define EXISTF_FILE (1 << 0)

static int file_exists(BPTR pathlock, char *path, char *file, ULONG flags)
{
	int ok = FALSE;
	BPTR olddirlock, filelock;

	D_S(struct FileInfoBlock, fib)

	if (path)
		pathlock = Lock(path, ACCESS_READ);

	if (file && *file)
	{
		if ((olddirlock = CurrentDir(pathlock)))
		{
			if ((filelock = Lock(file, ACCESS_READ)))
			{
				if (flags & EXISTF_FILE)
				{
					if (Examine(filelock, fib) && fib->fib_DirEntryType < 0)
						ok = TRUE;
				}
				else
					ok = TRUE;

				UnLock(filelock);
			}

			CurrentDir(olddirlock);
		}
	}

	if (path)
		UnLock(pathlock);

	return ok;
}

/********************************/

static int exists_req(struct Window *win, char *title, char *path, char *name)
{
	int ok = FALSE;

	if (file_exists((BPTR)NULL, path, name, 0))
	{
		ok = !my_SimpleRequest(
			win, title, GetString(locale, MSG_REPLACE_CANCEL), GetString(locale, MSG_FILETYPE_EXISTS), name);
	}

	return ok;
}

/********************************/

static char *getname(char *name)
{
	char *p = name;

	while (1)
	{
		if (*p == 0)
			return 0;
		if (*p == '\a')
		{
			p += 2;
			while (1)
			{
				if (*p == 0)
					return 0;
				if (*p == '\a')
					return p + 2;
				p++;
			}
		}
		p++;
	}
}

/********************************/

#define ADDNODE_INSTALLED (1 << 31)

static Att_Node *finder_Att_NewNode(Att_List *list, char *name, ULONG data, ULONG flags)
{
	Att_Node *node;
	char added = 0;

	// Valid list?
	if (!list)
		return 0;

	// Lock list
	LockAttList(list, TRUE);

	// Allocate node
	if (!(node = AllocMemH(list->memory, sizeof(Att_Node))))
	{
		UnlockAttList(list);
		return 0;
	}

	// If name supplied, create a copy of it
	if (name)
	{
		if (!(node->node.ln_Name = AllocMemH(list->memory, strlen(name) + 1)))
		{
			FreeMemH(node);
			UnlockAttList(list);
			return 0;
		}
		strcpy(node->node.ln_Name, name);
	}

	// Store data and list pointer
	node->data = data;
	node->list = list;

	// Filetype sort? (of course!)
	if (name)
	{
		Att_Node *posnode, *lastnode = 0;
		char *namep, *posnamep;
		int nodeinstalled = flags & ADDNODE_INSTALLED ? 1 : 0;
		int posnodeinstalled;
		BYTE posnodedata;

		if ((namep = getname(name)))
		{
			if (flags & ADDNODE_INSTALLED)
				node->data |= ADDNODE_INSTALLED;

			// Go through existing nodes
			posnode = (Att_Node *)list->list.lh_Head;
			while (posnode->node.ln_Succ)
			{
				posnodeinstalled = posnode->data & ADDNODE_INSTALLED ? 1 : 0;
				posnodedata = posnode->data;
				posnamep = getname(posnode->node.ln_Name);

				if (!posnamep)
					break;

// Compare new node
#if 1

				// Pri, Installed, Name
				if (((BYTE)data) > posnodedata)
					added = 1;
				else if (((BYTE)data) == posnodedata)
				{
					if (nodeinstalled && !posnodeinstalled)
						added = 1;
					else if (nodeinstalled == posnodeinstalled && stricmp(namep, posnamep) <= 0)
						added = 1;
				}

#else

				// Installed, Pri, Name
				if (nodeinstalled && !posnodeinstalled)
					added = 1;
				else if (nodeinstalled == posnodeinstalled)
				{
					if (((BYTE)data) > posnodedata)
						added = 1;
					else if (((BYTE)data) == posnodedata && stricmp(namep, posnamep) <= 0)
						added = 1;
				}

#endif

				if (added)
				{
					// Insert into list
					Insert((struct List *)list, (struct Node *)node, (struct Node *)lastnode);
					break;
				}
				lastnode = posnode;
				posnode = (Att_Node *)posnode->node.ln_Succ;
			}
		}
	}

	// If not added by a sort, add to end
	if (!added)
		AddTail((struct List *)list, (struct Node *)node);

	UnlockAttList(list);

	return node;
}

/********************************/

static int Att_FindBestNodeNumber(Att_List *list)
{
	Att_Node *test_node;
	long count;

	// Valid list?
	if (!list)
		return -1;

	// Find best installed
	for (test_node = (Att_Node *)list->list.lh_Head, count = 0;
		 !(test_node->data & ADDNODE_INSTALLED) && test_node->node.ln_Succ;
		 test_node = (Att_Node *)test_node->node.ln_Succ, count++)
		;

	// If none found, find best non-installed
	if (!test_node->node.ln_Succ)
		test_node = (Att_Node *)list->list.lh_Head, count = 0;

	// Return node
	return test_node->node.ln_Succ ? count : -1;
}

/********************************/

static int finder_openwindow(finder_data *data)
{
	int ok = TRUE;

	// Fill out new window
	data->new_win.parent = data->screen;
	data->new_win.dims = &_finder_window;
	data->new_win.title = GetString(locale, MSG_FIND_TITLE);
	data->new_win.locale = locale;
	data->new_win.port = 0;
	data->new_win.flags = WINDOW_SCREEN_PARENT | WINDOW_VISITOR | WINDOW_AUTO_KEYS | WINDOW_REQ_FILL;
	data->new_win.font = 0;

	if (!(data->window = OpenConfigWindow(&data->new_win)) ||
		!(data->list = AddObjectList(data->window, _finder_objects)))
	{
		CloseConfigWindow(data->window);
		ok = FALSE;
	}
	else
	{
		// Add AppWindow
		if (data->app_port)
			data->app_window = AddAppWindowA(0, 0, data->window, data->app_port, 0);
	}

	return ok;
}

/********************************/

static void finder_closewindow(finder_data *data)
{
	// Remove AppWindow
	if (data->app_window)
	{
		RemoveAppWindow(data->app_window);
		data->app_window = NULL;
	}

	// Close window
	CloseConfigWindow(data->window);
	data->window = NULL;
}

/********************************/

#define NODENAMELEN (PRI_MAXLEN + 1 + ID_MAXLEN + 1 + FILETYPE_MAXLEN)

static int finder_add_match_to_list(finder_data *data, Cfg_Filetype *ft, int installed)
{
	Att_Node *node;
	char buffer[NODENAMELEN + 2 + 2 + 1];
	int ok = FALSE;
	int number;

	lsprintf(buffer, "%4ld\a\x5%s\a\xa%s", ft->type.priority, (IPTR)ft->type.id, (IPTR)ft->type.name);

	SetGadgetChoices(data->list, GAD_FIND_LISTVIEW, (APTR)~0);

	if ((node = finder_Att_NewNode(data->listview_list, buffer, ft->type.priority, installed ? ADDNODE_INSTALLED : 0)))
	{
		if (installed)
			node->node.lve_Flags |= LVEF_SELECTED;

		ok = TRUE;
	}

	SetGadgetChoices(data->list, GAD_FIND_LISTVIEW, (APTR)data->listview_list);

	// Hilite best match so far
	if (node && (number = Att_FindBestNodeNumber(data->listview_list)) != -1)
		SetGadgetValue(data->list, GAD_FIND_LISTVIEW, number);

	return ok;
}

/********************************/

/*
 *	Check all Opus filetype files in dir for filetype matching file.
 */

static int finder_filetypes_in_cache(finder_data *data)
{
	Att_Node *node;
	Cfg_FiletypeList *ftl; /* Filetype list (only one entry each) */
	Cfg_Filetype *ft;	   /* Filetype */
	APTR matchh;		   /* Match handle */
	int *count;
	int *best_pri;
	int ok = FALSE;

	SetGadgetValue(data->list, GAD_FIND_TEXT2, (IPTR)GetString(locale, MSG_FIND_MATCHING));

	data->count_filetypes = 0;
	data->count_storage = 0;

	data->best_filetypes = -129;
	data->best_storage = -129;

	data->best_installed_ft = NULL;
	data->best_stored_ft = NULL;

	if (data->filetype_cache)
	{
		SetWindowBusy(data->window);

		for (node = (Att_Node *)data->filetype_cache->list.lh_Head; node->node.ln_Succ;
			 node = (Att_Node *)node->node.ln_Succ)
		{
			ftl = (Cfg_FiletypeList *)node->data;

			if (ftl->flags & FTLISTF_INSTALLED)
			{
				count = &data->count_filetypes;
				best_pri = &data->best_filetypes;
			}
			else
			{
				count = &data->count_storage;
				best_pri = &data->best_storage;
			}

			for (ft = (Cfg_Filetype *)ftl->filetype_list.lh_Head; ft->node.ln_Succ;
				 ft = (Cfg_Filetype *)ft->node.ln_Succ)
			{
				if ((matchh = GetMatchHandle(data->current_entry_path)))
				{
					if (MatchFiletype(matchh, ft))
					{
						(*count)++;

						if (ft->type.priority > *best_pri)
							*best_pri = ft->type.priority;

						if (ftl->flags & FTLISTF_INSTALLED)
						{
							if (!data->best_installed_ft || ft->type.priority > data->best_installed_ft->type.priority)
								data->best_installed_ft = ft;
						}
						else
						{
							if (!data->best_stored_ft || ft->type.priority > data->best_stored_ft->type.priority)
								data->best_stored_ft = ft;
						}

						finder_add_match_to_list(data, ft, (ftl->flags & FTLISTF_INSTALLED));
					}

					FreeMatchHandle(matchh);
				}
			}
		}
		ClearWindowBusy(data->window);

		ok = TRUE;
	}

	return ok;
}

/********************************/

static int finder_init_listview(finder_data *data)
{
	int ok = FALSE;

	SetGadgetChoices(data->list, GAD_FIND_LISTVIEW, (APTR)~0);

	if ((data->listview_list = Att_NewList(LISTF_POOL)))
		ok = TRUE;

	SetGadgetChoices(data->list, GAD_FIND_LISTVIEW, (APTR)data->listview_list);

	return ok;
}

/********************************/

static int finder_free_listview(finder_data *data)
{
	int ok = FALSE;

	Att_RemList(data->listview_list, 0);

	return ok;
}

/********************************/

static void finder_print_blurb(finder_data *data, LONG s)
{
	int i;

	for (i = 0; i < 7; i++)
		SetGadgetValue(data->list, GAD_FIND_TEXT2 + i, (IPTR)(s ? (char *)GetString(locale, s + i) : ""));

	DisableObject(data->list, GAD_FIND_INSTALL, s == MSG_NONE_FOUND_1 || s == MSG_INSTALLED_FOUND_1);
	DisableObject(data->list, GAD_FIND_EDIT, s == MSG_NONE_FOUND_1 || s == MSG_STORED_FOUND_1);
}

/********************************/

static int finder_rescan(finder_data *data)
{
	int ok = FALSE;

	finder_free_listview(data);

	if (finder_init_listview(data))
	{
		finder_print_blurb(data, 0);

		// Find filetype
		finder_filetypes_in_cache(data);

		if (data->count_filetypes < 1)
		{
			if (data->count_storage < 1)
				finder_print_blurb(data, MSG_NONE_FOUND_1);
			else
				finder_print_blurb(data, MSG_STORED_FOUND_1);
		}
		else
		{
			if (data->best_storage > data->best_filetypes)
				finder_print_blurb(data, MSG_BETTER_FOUND_1);
			else
				finder_print_blurb(data, MSG_INSTALLED_FOUND_1);
		}

		ok = TRUE;
	}
	return ok;
}

/********************************/

static int finder_install_filetype(finder_data *data)
{
	BPTR src, dst;
	UBYTE buffer[1024];
	LONG len;
	int ok = FALSE;

	if (data->best_stored_ft)
	{
		if (!exists_req(
				data->window, GetString(locale, MSG_FIND_TITLE), "DOpus5:Filetypes", data->best_stored_ft->type.name))
		{
			strcpy((char *)buffer, "DOpus5:Storage/Filetypes");
			AddPart((STRPTR)buffer, data->best_stored_ft->list->path, 1024);
			if ((src = Open((CONST_STRPTR)buffer, MODE_OLDFILE)))
			{
				strcpy((char *)buffer, "DOpus5:Filetypes");
				AddPart((STRPTR)buffer, data->best_stored_ft->type.name, 1024);
				if ((dst = Open((CONST_STRPTR)buffer, MODE_NEWFILE)))
				{
					ok = TRUE;

					while ((len = Read(src, buffer, 1024)) > 0)
						if ((len = Write(dst, buffer, len) < 0))
							break;

					if (len >= 0)
					{
						data->best_stored_ft->list->flags |= FTLISTF_INSTALLED;
						finder_rescan(data);
					}
					else
						ok = FALSE;

					Close(dst);
				}

				Close(src);
			}
		}
	}

	return ok;
}

/********************************/

// Init code for process
#ifdef __amigaos4__
ULONG SAVEDS ASM finder_creator_proc_init(REG(a0, IPCData *ipc), REG(a1, finder_data *data))
#else
IPC_StartupCode(finder_creator_proc_init, finder_data *, data)
#endif
{
	ULONG ok = TRUE;

	data->creator_ipc = ipc;

	return ok;
}

/********************************/

// Code for the sub-tasks
void SAVEDS finder_creator_proc_code(void)
{
	finder_data *data;
	// struct Library *DOpusBase;
	Att_List *list;
	char path[256 + 1];
	char *name;
	struct filetype_info *fti;
	Att_Node *node;
	Cfg_FiletypeList *ftl, *ftl2;
	int ok = FALSE;

	// DOpusBase = GET_DOPUSLIB;

	IPC_ProcStartup((IPTR *)&data, (APTR)&finder_creator_proc_init);

	strcpy(path, data->current_entry_path);
	name = FilePart(path);
	*name = 0;
	name = FilePart(data->current_entry_path);

	if ((list = Att_NewList(LISTF_POOL)))
	{
		if (file_exists((BPTR)NULL, path, name, EXISTF_FILE))
		{
			if ((fti = AllocMemH(list->memory, sizeof(struct filetype_info))))
			{
				if ((node = Att_NewNode(list, name, (IPTR)fti, 0)))
				{
					strcpy(fti->fti_filename, name);
					strncpy(fti->fti_path, path, 256);

					ftl = creator_generic("",				  //	char *args,
										  data->screen,		  //	struct Screen *screen,
										  data->creator_ipc,  //	IPCData *ipc,
										  data->ipc,		  //	IPCData *main_ipc,
										  data->main_ipc,
										  data->func_callback,	//	EXT_FUNC(func_callback)
										  list,
										  path);

					if (ftl)
					{
						if ((ftl2 = ReadFiletypes(ftl->path, data->filetype_cache->memory)))
						{
							// Add created filetype
							ftl2->flags |= FTLISTF_INSTALLED;
							if (Att_NewNode(data->filetype_cache, NULL, (IPTR)ftl2, 0))
								finder_rescan(data);
						}

						FreeFiletypeList(ftl);
					}
				}
				else
					FreeMemH(fti);
			}
		}
		Att_RemList(list, 0);
	}

	// Flush IPC port
	IPC_Flush(data->creator_ipc);

	// Send goodbye message to owner
	IPC_Goodbye(data->creator_ipc, data->ipc, ok);

	// Free IPC data
	IPC_Free(data->editor_ipc);
	data->creator_ipc = NULL;

	return;
}

/********************************/

static int finder_create_filetype(finder_data *data)
{
	int ok = FALSE;

	// Launch a new task
	IPC_Launch(0,								 // List to add task to (optional, but useful)
			   &data->creator_ipc,				 // IPCData ** to store task IPC pointer in (optional)
			   "filetype_creator",				 // Name
			   (IPTR)finder_creator_proc_code,	 // Code
			   STACK_DEFAULT,					 // Stack size
			   (IPTR)data,						 // Data passed to task
			   (struct Library *)DOSBase);		 // Needs pointer to dos.library

	return ok;
}

/********************************/

// Init code for process
#if defined(__amigaos4__)
ULONG SAVEDS ASM finder_editor_proc_init(REG(a0, IPCData *ipc), REG(a1, finder_data *data))
#else
IPC_StartupCode(finder_editor_proc_init, finder_data *, data)
#endif
{
	ULONG ok = TRUE;

	data->editor_ipc = ipc;

	return ok;
}

/********************************/

static int finder_save_edited_filetype(finder_data *data)
{
	Cfg_FiletypeList *ftl;
	Att_Node *oldnode;
	int ok = FALSE;

	// Do we now have a filetype to save?
	if (data->edited_filetype)
	{
		// Make a list for the filetype
		if ((ftl = AllocMemH(0, sizeof(Cfg_FiletypeList))))
		{
			DateStamp(&ftl->date);
			NewList(&ftl->filetype_list);
			ftl->flags = 0;

			// If we have changed the name, check for existing filetype with new name
			if (stricmp(data->best_installed_ft->list->path, data->edited_filetype->type.name) == 0 ||
				!exists_req(data->window,
							GetString(locale, MSG_FIND_TITLE),
							"DOpus5:Filetypes",
							data->edited_filetype->type.name))
			{
				/* 5/8/97 (JP) : Added path part */
				strcpy(ftl->path, "DOpus5:Filetypes/");
				strcat(ftl->path, data->edited_filetype->type.name);

				data->edited_filetype->list = ftl;
				AddHead(&ftl->filetype_list, (struct Node *)data->edited_filetype);

				// Save edited filetype
				if (!SaveFiletypeList(ftl, ftl->path))
				{
					// Delete file of pre-edited version
					if (stricmp(ftl->path, data->best_installed_ft->list->path))
						DeleteFile(data->best_installed_ft->list->path);

					// Remove pre-edited version
					if ((oldnode = Att_FindNodeData(data->filetype_cache, (IPTR)data->best_installed_ft->list)))
					{
						/* 24/7/97 (JP) : Next two lines swapped around */
						FreeFiletypeList((Cfg_FiletypeList *)oldnode->data);
						Att_RemNode(oldnode);

						/* 4/8/97 (JP) : Was ..._ft->list=NULL; */
						data->best_installed_ft = NULL;
					}

					// Add edited filetype to cache
					ftl->flags |= FTLISTF_INSTALLED;
					if (Att_NewNode(data->filetype_cache, NULL, (IPTR)ftl, 0))
						finder_rescan(data);

					ok = TRUE;
				}
			}
			if (!ok)
				FreeFiletypeList(ftl);
		}
	}

	if (!ok)
		DisplayBeep(data->window->WScreen);

	return ok;
}

/********************************/

// Code for the sub-tasks
void SAVEDS finder_editor_proc_code(void)
{
	finder_data *data;
	// struct Library *DOpusBase;
	struct Library *ConfigOpusBase = NULL;
#ifdef __amigaos4__
	struct ConfigOpusIFace *IConfigOpus = NULL;
#endif
	Cfg_Filetype *edited_filetype;

	// DOpusBase = GET_DOPUSLIB;

	IPC_ProcStartup((IPTR *)&data, (APTR)&finder_editor_proc_init);

	if (data->best_installed_ft && (ConfigOpusBase = OpenLibrary("DOpus5:Modules/configopus.module", LIB_VERSION)) &&
		GETINTERFACE(IConfigOpus, ConfigOpusBase))
	{
		if ((edited_filetype =
				 EditFiletype(data->best_installed_ft, data->window, data->editor_ipc, data->main_ipc, 0)))
		{
			data->edited_filetype = edited_filetype;

			if (data->edited_filetype)
				finder_save_edited_filetype(data);
		}
#ifdef __amigaos4__
		DropInterface((struct Interface *)IConfigOpus);
#endif
		CloseLibrary(ConfigOpusBase);
	}
	else
		CloseLibrary(ConfigOpusBase);
	DisplayBeep(data->window->WScreen);

	// Flush IPC port
	IPC_Flush(data->editor_ipc);

	// Send goodbye message to owner
	IPC_Goodbye(data->editor_ipc, data->ipc, 0);

	// Free IPC data
	IPC_Free(data->editor_ipc);
	data->editor_ipc = NULL;

	return;
}

/********************************/

int finder_edit_filetype(finder_data *data)
{
	int ok = FALSE;

	// Launch a new task
	IPC_Launch(0,								// List to add task to (optional, but useful)
			   &data->editor_ipc,				// IPCData ** to store task IPC pointer in (optional)
			   "filetype_editor",				// Name
			   (IPTR)finder_editor_proc_code,	// Code
			   STACK_DEFAULT,					// Stack size
			   (IPTR)data,						// Data passed to task
			   (struct Library *)DOSBase);		// Needs pointer to dos.library

	return ok;
}

/********************************/

static int finder_event_loop(finder_data *data)
{
	int cont = 1;
	struct IntuiMessage *msg;
	struct AppMessage *amsg;
	IPCMessage *imsg;
	int break_flag = 0;
	int goodbye_action = GOODBYE_CONTINUE;

	// Event loop
	FOREVER
	{
		// AppMessages?
		if (data->app_window)
		{
			// Get messages
			while ((amsg = (struct AppMessage *)GetMsg(data->app_port)))
			{
				DisplayBeep(data->window->WScreen);

				// Valid argument?
				if (amsg->am_NumArgs > 0)
				{
				}

				// Reply message
				ReplyMsg((struct Message *)amsg);
			}
		}

		// IPC messages?
		if (data->ipc)
		{
			while ((imsg = (IPCMessage *)GetMsg(data->ipc->command_port)))
			{
				if (imsg->command == IPC_GOODBYE)
				{
					IPC_GetGoodbye(imsg);

					if (!data->editor_ipc && !data->creator_ipc)
					{
						if (goodbye_action == GOODBYE_USE)
							break_flag = 1;
						else if (goodbye_action == GOODBYE_INSTALL)
							break_flag = finder_install_filetype(data);
						else if (goodbye_action == GOODBYE_CREATE)
							break_flag = finder_create_filetype(data);
						else if (goodbye_action == GOODBYE_EDIT)
							break_flag = finder_edit_filetype(data);
						else if (goodbye_action == GOODBYE_ABORT)
						{
							break_flag = 1;
							cont = 0;
						}
					}
					goodbye_action = GOODBYE_CONTINUE;
				}

				// Activate?
				if (imsg->command == IPC_ACTIVATE)
				{
					WindowToFront(data->window);
					ActivateWindow(data->window);
				}

				// Quit?
				else if (imsg->command == IPC_ABORT || imsg->command == IPC_HIDE || imsg->command == IPC_QUIT)
				{
					if (data->editor_ipc)
						IPC_Quit(data->editor_ipc, FALSE, 0);
					if (data->creator_ipc)
						IPC_Quit(data->creator_ipc, FALSE, 0);
					if (data->editor_ipc || data->creator_ipc)
						goodbye_action = GOODBYE_ABORT;
					else
						break_flag = 1;
				}

				ReplyMsg((struct Message *)imsg);
			}
		}

		// Any Intuition messages?
		while ((msg = GetWindowMsg(data->window->UserPort)))
		{
			struct IntuiMessage copy_msg;

			// Copy message and reply
			copy_msg = *msg;
			ReplyWindowMsg(msg);

			// Gadget?
			if (copy_msg.Class == IDCMP_GADGETUP)
			{
				switch (((struct Gadget *)copy_msg.IAddress)->GadgetID)
				{
				// Use
				case GAD_FIND_USE:
					if (data->editor_ipc || data->creator_ipc)
						goodbye_action = GOODBYE_USE;

					if (data->editor_ipc)
						IPC_Quit(data->editor_ipc, FALSE, 0);

					if (data->creator_ipc)
						IPC_Quit(data->creator_ipc, FALSE, 0);
					else
						break_flag = 1;
					break;

				// Install
				case GAD_FIND_INSTALL:
					if (data->editor_ipc || data->creator_ipc)
						goodbye_action = GOODBYE_INSTALL;
					else
						finder_install_filetype(data);

					if (data->editor_ipc)
						IPC_Quit(data->editor_ipc, FALSE, 0);

					if (data->creator_ipc)
						IPC_Quit(data->creator_ipc, FALSE, 0);
					break;

				// Create
				case GAD_FIND_CREATE:
					if (data->creator_ipc)
						IPC_Command(data->creator_ipc, IPC_ACTIVATE, 0, 0, 0, (struct MsgPort *)-1);

					else if (data->editor_ipc)
					{
						IPC_Quit(data->editor_ipc, FALSE, 0);
						goodbye_action = GOODBYE_CREATE;
					}
					else
						finder_create_filetype(data);
					break;

				// Edit
				case GAD_FIND_EDIT:
					if (data->editor_ipc)
						IPC_Command(data->editor_ipc, IPC_ACTIVATE, 0, 0, 0, (struct MsgPort *)-1);

					else if (data->creator_ipc)
					{
						IPC_Quit(data->creator_ipc, FALSE, 0);
						goodbye_action = GOODBYE_EDIT;
					}
					else
						finder_edit_filetype(data);
					break;

				// Cancel
				case GAD_FIND_CANCEL:
					if (data->editor_ipc || data->creator_ipc)
						goodbye_action = GOODBYE_ABORT;
					else
					{
						break_flag = 1;
						cont = 0;
					}

					if (data->editor_ipc)
						IPC_Quit(data->editor_ipc, FALSE, 0);

					if (data->creator_ipc)
						IPC_Quit(data->creator_ipc, FALSE, 0);
					break;
				}
			}

			// Close window?
			else if (copy_msg.Class == IDCMP_CLOSEWINDOW)
			{
				if (data->editor_ipc || data->creator_ipc)
					goodbye_action = GOODBYE_ABORT;
				else
				{
					break_flag = 1;
					cont = 0;
					break;
				}

				if (data->editor_ipc)
					IPC_Quit(data->editor_ipc, FALSE, 0);

				if (data->creator_ipc)
					IPC_Quit(data->creator_ipc, FALSE, 0);
			}

			// Key press
			else if (copy_msg.Class == IDCMP_RAWKEY)
			{
				// Help?
				if (copy_msg.Code == 0x5f && !(copy_msg.Qualifier & VALID_QUALIFIERS))
				{
					// Valid main IPC?
					if (data->main_ipc)
					{
						// Set busy pointer
						SetWindowBusy(data->window);

						// Send help request
						IPC_Command(data->main_ipc, IPC_HELP, (1 << 31), "Filetype Finder", 0, (struct MsgPort *)-1);

						// Clear busy pointer
						ClearWindowBusy(data->window);
					}
				}
			}
		}

		// Check break flag
		if (break_flag)
			break;

		// Wait for an event
		Wait(1 << data->window->UserPort->mp_SigBit | (data->app_port ? 1 << data->app_port->mp_SigBit : 0) |
			 (data->ipc ? 1 << data->ipc->command_port->mp_SigBit : 0));
	}

	return cont;
}

/********************************/

static int finder_build_cache(finder_data *data)
{
	BPTR lock, oldlock; /* Dos locks */

	D_S(struct FileInfoBlock, fib)

	Cfg_FiletypeList *ftl; /* Filetype list (only one entry each) */
	int ok = TRUE;

	SetWindowBusy(data->window);

	SetGadgetValue(data->list, GAD_FIND_TEXT2, (IPTR)GetString(locale, MSG_FIND_SCANNING));

	if ((data->filetype_cache = Att_NewList(LISTF_POOL)))
	{
		/* Get filetypes from Opus itself */

		data->pointer_packet.type = MODPTR_FILETYPES;
		REFCALL(data->func_callback, EXTCMD_GET_POINTER, IPCDATA(data->ipc), &data->pointer_packet);

		if (data->pointer_packet.pointer)
		{
			for (ftl = (Cfg_FiletypeList *)((struct ListLock *)data->pointer_packet.pointer)->list.lh_Head;
				 ftl->node.ln_Succ;
				 ftl = (Cfg_FiletypeList *)ftl->node.ln_Succ)
			{
				Cfg_FiletypeList *copy;

				if (ftl->flags & FTLISTF_INTERNAL)
					continue;

				/* Make a copy : JP 1/8/97 */
				if ((copy = AllocMemH(data->filetype_cache->memory, sizeof(Cfg_FiletypeList))))
				{
					Cfg_Filetype *type, *typecopy;

					/* Initialise list */
					NewList(&copy->filetype_list);

					/* Copy list information */
					strcpy(copy->path, ftl->path);
					copy->date = ftl->date;

					/* Copy filetypes */
					for (type = (Cfg_Filetype *)ftl->filetype_list.lh_Head; type->node.ln_Succ;
						 type = (Cfg_Filetype *)type->node.ln_Succ)
					{
						/* Get copy */
						if ((typecopy = CopyFiletype(type, data->filetype_cache->memory)))
						{
							AddTail(&copy->filetype_list, &typecopy->node);
							typecopy->list = copy;
						}
					}

					/* Set installed flag */
					copy->flags |= FTLISTF_INSTALLED;

					/* Create node */
					if (!Att_NewNode(data->filetype_cache, NULL, (IPTR)copy, 0))
					{
						ok = FALSE;
						FreeFiletypeList(copy);
					}
				}
			}

			if (data->pointer_packet.flags & POINTERF_LOCKED)
				REFCALL(data->func_callback, EXTCMD_FREE_POINTER, IPCDATA(data->ipc), &data->pointer_packet);
		}

		/* Get filetypes from disk */
		if (ok)
		{
			if ((lock = Lock("DOpus5:Storage/Filetypes", ACCESS_READ)))
			{
				oldlock = CurrentDir(lock);

				if (Examine(lock, fib))
				{
					while (ExNext(lock, fib) && fib->fib_DirEntryType < 0)
					{
						if ((ftl = ReadFiletypes(fib->fib_FileName, data->filetype_cache->memory)))
						{
							if (!Att_NewNode(data->filetype_cache, NULL, (IPTR)ftl, 0))
							{
								ok = FALSE;
								FreeFiletypeList(ftl);
							}
						}
					}
				}
				CurrentDir(oldlock);

				UnLock(lock);
			}
		}
	}
	ClearWindowBusy(data->window);

	return ok;
}

/********************************/

static void finder_free_cache(finder_data *data)
{
	Att_Node *node;

	if (data->filetype_cache)
	{
		for (node = (Att_Node *)data->filetype_cache->list.lh_Head; node->node.ln_Succ;
			 node = (Att_Node *)node->node.ln_Succ)
		{
			FreeFiletypeList((Cfg_FiletypeList *)node->data);
			node->data = 0L;
		}
	}

	Att_RemList(data->filetype_cache, 0);
}

/********************************/

static int finder(struct Screen *screen, IPCData *ipc, IPCData *main_ipc, EXT_FUNC(func_callback))
{
	finder_data *data;
	FunctionEntry *entry;
	struct endentry_packet end_pkt = {0};
	int retval = 0;

	// Allocate data
	if ((data = AllocVec(sizeof(finder_data), MEMF_CLEAR)))
	{
		// Store IPC pointer
		data->screen = screen;
		data->ipc = ipc;
		data->main_ipc = main_ipc;
		data->func_callback = func_callback;

		// Create message port
		data->app_port = CreateMsgPort();

		if (finder_openwindow(data))
		{
			// Get path
			REFCALL(func_callback, EXTCMD_GET_SOURCE, IPCDATA(ipc), data->path);

			// Get first entry
			while ((entry = (FunctionEntry *)REFCALL(func_callback, EXTCMD_GET_ENTRY, IPCDATA(ipc), 0)))
			{
				strcpy(data->current_entry_path, data->path);
				AddPart(data->current_entry_path, entry->name, 256);

				// Filename
				SetGadgetValue(data->list,
							   GAD_FIND_TEXT1,
							   (IPTR)(strlen(data->current_entry_path) > FILENAME_MAXLEN
										   ? (char *)FilePart(data->current_entry_path)
										   : data->current_entry_path));

				if (!data->filetype_cache)
					if (!finder_build_cache(data))
						break;

				if (finder_init_listview(data))
				{
					finder_print_blurb(data, 0);

					// Find initial filetype
					finder_filetypes_in_cache(data);

					if (data->count_filetypes < 1)
					{
						if (data->count_storage < 1)
							finder_print_blurb(data, MSG_NONE_FOUND_1);
						else
							finder_print_blurb(data, MSG_STORED_FOUND_1);
					}
					else
					{
						if (data->best_storage > data->best_filetypes)
							finder_print_blurb(data, MSG_BETTER_FOUND_1);
						else
							finder_print_blurb(data, MSG_INSTALLED_FOUND_1);
					}

					if (!finder_event_loop(data))
						break;

					finder_free_listview(data);
				}

				// Fill out packet with results
				end_pkt.entry = entry;
				end_pkt.deselect = TRUE;

				// End this entry
				REFCALL(func_callback, EXTCMD_END_ENTRY, IPCDATA(ipc), &end_pkt);
			}

			finder_free_cache(data);

			finder_closewindow(data);
		}

		// Free message port
		{
			struct Message *msg;
			while ((msg = GetMsg(data->app_port)))
				ReplyMsg(msg);
		}
		DeleteMsgPort(data->app_port);

		// Free data
		FreeVec(data);
	}

	return retval;
}

/********************************/ /********************************/ /********************************/
/********************************/ /********************************/ /********************************/
/********************************/ /********************************/ /********************************/

static int creator_openwindow(creator_data *data)
{
	int ok = TRUE;

	// Fill out new window
	data->new_win.parent = data->screen;
	data->new_win.dims = &_creator_window;
	data->new_win.title = GetString(locale, MSG_CREATE_TITLE);
	data->new_win.locale = locale;
	data->new_win.port = 0;
	data->new_win.flags = WINDOW_VISITOR | WINDOW_AUTO_KEYS | WINDOW_REQ_FILL | WINDOW_SCREEN_PARENT;
	data->new_win.font = 0;

	if (!(data->window = OpenConfigWindow(&data->new_win)) ||
		!(data->list = AddObjectList(data->window, _creator_objects)))
	{
		CloseConfigWindow(data->window);
		ok = FALSE;
	}
	else
	{
		// Add AppWindow
		if (data->app_port)
			data->app_window = AddAppWindowA(0, 0, data->window, data->app_port, 0);
	}

	return ok;
}

/********************************/

static void creator_closewindow(creator_data *data)
{
	// Remove AppWindow
	if (data->app_window)
	{
		RemoveAppWindow(data->app_window);
		data->app_window = NULL;
	}

	// Close window
	CloseConfigWindow(data->window);
	data->window = NULL;
}

/********************************/

static int creator_make_recognition(creator_data *data)
{
	int len = 0;
	unsigned char *p;

	if (data->match_name)
		len += 2 + strlen(data->match_name);
	if (data->match_iff)
		len += 2 + strlen(data->match_iff);
	if (data->match_group)
		len += 2 + strlen(data->match_group);
	if (data->match_id)
		len += 2 + strlen(data->match_id);
	if (data->match_bytes)
		len += 2 + strlen(data->match_bytes);
	if (data->match_bytesc)
		len += 2 + strlen(data->match_bytesc);

	if (len && (data->filetype->recognition = AllocMemH(0, len + 2)))
	{
		p = data->filetype->recognition;

		if (GetGadgetValue(data->list, GAD_CREATE_NAME) && data->match_name)
		{
			*p++ = FTOP_MATCHNAME;
			strcpy((char *)p, data->match_name);
			p += strlen(data->match_name);
			*p++ = FTOP_AND;
		}
		if (GetGadgetValue(data->list, GAD_CREATE_IFF) && data->match_iff)
		{
			*p++ = FTOP_MATCHFORM;
			strcpy((char *)p, data->match_iff);
			p += strlen(data->match_iff);
			*p++ = FTOP_AND;
		}
		if (GetGadgetValue(data->list, GAD_CREATE_GROUP) && data->match_group)
		{
			*p++ = FTOP_MATCHDTGROUP;
			strcpy((char *)p, data->match_group);
			p += strlen(data->match_group);
			*p++ = FTOP_AND;
		}
		if (GetGadgetValue(data->list, GAD_CREATE_ID) && data->match_id)
		{
			*p++ = FTOP_MATCHDTID;
			strcpy((char *)p, data->match_id);
			p += strlen(data->match_id);
			*p++ = FTOP_AND;
		}
		if (GetGadgetValue(data->list, GAD_CREATE_BYTES))
		{
			char *match;
			UBYTE ftop;

			if (data->caseflag == 0)
			{
				match = data->match_bytes;
				ftop = FTOP_MATCH;
			}
			else
			{
				match = data->match_bytesc;
				ftop = FTOP_MATCHNOCASE;
			}

			if (match)
			{
				*p++ = ftop;
				strcpy((char *)p, match);
				p += strlen(match);
				*p++ = FTOP_AND;
			}
		}

		*--p = FTOP_ENDSECTION;
		*++p = 0;
	}

	return TRUE;
}

/********************************/

static int creator_create_filetype(creator_data *data)
{
	int ok = FALSE;

	// Exisiting filetype?
	if (data->filetype)
	{
		FreeFiletype(data->filetype);
		data->filetype = NULL;
	}

	// Fill out the filetype structure
	if ((data->filetype = NewFiletype(0)))
	{
		strcpy(data->filetype->type.name, data->filetype_name);
		strcpy(data->filetype->type.id, "");
		data->filetype->type.flags = 0;
		data->filetype->type.priority = 0;
		data->filetype->type.count = 0;

		creator_make_recognition(data);

		data->filetype->icon_path = NULL;
		data->filetype->actions = NULL;
		NewList(&data->filetype->function_list);
		data->filetype->list = NULL;

		ok = TRUE;
	}
	return ok;
}

/********************************/

#if 1

/*
 *	This is the quick way to find the type of an IFF file.
 *	Only works with simple FORMs.
 */

static int creator_get_iff(struct filetype_info *fti, BPTR lock)
{
	int ok = FALSE;
	BPTR newlock, fh;
	LONG buf[3];

	if ((newlock = DupLock(lock)))
	{
		if (!(fh = OpenFromLock(newlock)))
			UnLock(newlock);
		else
		{
			if (Read(fh, buf, 12) == 12)
			{
				if (buf[0] == ID_FORM)
				{
					fti->fti_flags |= FTIF_IFF;
					fti->fti_iff_type = buf[2];
					ok = TRUE;
				}
			}
			Close(fh);
		}
	}

	return ok;
}

#else

/*
 *	This is the official way to find the type of an IFF file.
 *	Works with FORMs, CATs, LISTs, including nested files (I think).
 */

static int creator_get_iff(struct filetype_info *fti, BPTR lock)
{
	int ok = FALSE;
	struct IFFHandle *iff = NULL;
	BPTR newlock;
	long error;
	struct ContextNode *top;

	if (iff = AllocIFF())
	{
		if (newlock = DupLock(lock))
		{
			if (!(iff->iff_Stream = OpenFromLock(newlock)))
				UnLock(newlock);
			else
			{
				InitIFFasDOS(iff);
				if (!(error = OpenIFF(iff, IFFF_READ)))
				{
					if (!(error = ParseIFF(iff, IFFPARSE_RAWSTEP)))
					{
						if (top = CurrentChunk(iff))
						{
							ok = TRUE;

							fti->fti_flags |= FTIF_IFF;
							fti->fti_iff_type = top->cn_Type;
						}
					}
					CloseIFF(iff);
				}
				Close(iff->iff_Stream);
			}
		}
		FreeIFF(iff);
	}

	return ok;
}

#endif

/********************************/

static int creator_get_datatype(struct filetype_info *fti, BPTR lock)
{
	int ok = FALSE;
	// struct Library *DataTypesBase;
	struct DataType *dt;

	/*if	((DataTypesBase = OpenLibrary ("datatypes.library", 39)))
		{
		#ifdef __amigaos4__
		IDataTypes=(struct DataTypesIFace *)GetInterface(DataTypesBase,"main",1,NULL);
		#endif*/

	if ((dt = ObtainDataTypeA(DTST_FILE, (APTR)lock, NULL)))
	{
		fti->fti_datatype_group = dt->dtn_Header->dth_GroupID;
		fti->fti_flags |= FTIF_GROUP;

		fti->fti_datatype_ID = dt->dtn_Header->dth_ID;
		fti->fti_flags |= FTIF_ID;

		ok = TRUE;

		ReleaseDataType(dt);
	}

	/*#ifdef __amigaos4__
	DropInterface((struct Interface *)IDataTypes);
	#endif
	CloseLibrary( DataTypesBase );
	}*/

	return ok;
}

/********************************/

static int creator_get_bytes(struct filetype_info *fti, BPTR lock)
{
	int ok = FALSE;
	BPTR newlock, fh;

	if ((newlock = DupLock(lock)))
	{
		if ((fh = OpenFromLock(newlock)))
		{
			ok = TRUE;

			if ((fti->fti_bytecountc = fti->fti_bytecount = Read(fh, fti->fti_filebytes, BYTECOUNT)) > 0)
			{
				int i;

				for (i = 0; i < fti->fti_bytecountc; i++)
					fti->fti_filebytesc[i] = toupper(fti->fti_filebytes[i]);

				fti->fti_flags |= FTIF_BYTES;
				fti->fti_flags |= FTIF_BYTESC;
			}

			Close(fh);
		}
		else
			UnLock(newlock);
	}

	return ok;
}

/********************************/

/*
 *	Get various filetype relevant info for a file.
 */

static int creator_sniff_filetype(creator_data *data, struct filetype_info *fti)
{
	char fullpath[256];
	BPTR lock;
	D_S(struct FileInfoBlock, fib)

	strcpy(fullpath, fti->fti_path);
	AddPart(fullpath, fti->fti_filename, 256);

	if ((lock = Lock(fullpath, ACCESS_READ)))
	{
		if (Examine(lock, fib))
		{
			fti->fti_flags |= FTIF_FILENAME;

			if (fib->fib_DirEntryType < 0)
			{
				creator_get_iff(fti, lock);
				creator_get_datatype(fti, lock);
				creator_get_bytes(fti, lock);
			}
		}
		UnLock(lock);
	}

	return 0;
}

/********************************/

/*
 *	Return the number of characters that match at the ends of two strings.
 */

static int strcmpr(char *s1, char *s2)
{
	int same = 0;
	int l1, l2;

	l1 = strlen(s1);
	l2 = strlen(s2);

	while (l1-- > 0 && l2-- > 0)
	{
		if (toupper(s1[l1]) == toupper(s2[l2]))
			same++;
		else
			break;
	}

	return same;
}

/********************************/

static void creator_add_filename(struct filetype_info *master, struct filetype_info *ft)
{
	int oldlen, r;

	if (master->fti_flags & FTIF_FILENAME)
	{
		oldlen = strlen(master->fti_filename);
		r = strcmpr(master->fti_filename, ft->fti_filename);

		if (r > 0)
		{
			memmove(master->fti_filename, master->fti_filename + oldlen - r, r + 1);

			if (r < oldlen || r < strlen(ft->fti_filename))
				master->fti_flags |= FTIF_FILENAME_END;
		}
		else
			master->fti_flags &= ~FTIF_FILENAME;
	}
}

/********************************/

static void creator_add_bytes(struct filetype_info *master, struct filetype_info *ft)
{
	int l, i; /* Length, Index */
	int ok = FALSE;

	if (master->fti_flags & FTIF_BYTES)
	{
		master->fti_bytecount = l = min(master->fti_bytecount, ft->fti_bytecount);

		if (l > 0)
		{
			for (i = 0; i < l; i++)
			{
				if (master->fti_bytemask[i] && master->fti_filebytes[i] == ft->fti_filebytes[i])
					ok = TRUE;
				else
					master->fti_bytemask[i] = 0;
			}

			/* Adjust length to count non-masked bytes only */
			for (i = l - 1; i >= 0; i--)
			{
				if (master->fti_bytemask[i])
				{
					master->fti_bytecount = i + 1;
					break;
				}
			}
		}
	}

	if (!ok)
		master->fti_flags &= ~FTIF_BYTES;
}

/********************************/

static void creator_add_bytesc(struct filetype_info *master, struct filetype_info *ft)
{
	int l, i; /* Lenght, Index */
	int ok = FALSE;

	if (master->fti_flags & FTIF_BYTESC)
	{
		master->fti_bytecountc = l = min(master->fti_bytecountc, ft->fti_bytecountc);

		if (l > 0)
		{
			for (i = 0; i < l; i++)
			{
				if (master->fti_bytemaskc[i] && master->fti_filebytesc[i] == ft->fti_filebytesc[i])
					ok = TRUE;
				else
					master->fti_bytemaskc[i] = 0;
			}

			/* Adjust length to count non-masked bytes only */
			for (i = l - 1; i >= 0; i--)
			{
				if (master->fti_bytemaskc[i])
				{
					master->fti_bytecountc = i + 1;
					break;
				}
			}
		}
	}

	if (!ok)
		master->fti_flags &= ~FTIF_BYTESC;
}

/********************************/

static void creator_addnode(creator_data *data, struct filetype_info *fti)
{
	ULONG flags;

	/* Combine this filetype info into the master filetype info */
	if (!(data->master_fti.fti_flags & FTIF_USED))
	{
		*&data->master_fti = *fti;
		memset(data->master_fti.fti_bytemask, 255, BYTECOUNT);
		memset(data->master_fti.fti_bytemaskc, 255, BYTECOUNT);
		data->master_fti.fti_flags |= FTIF_USED;
	}
	else
	{
		flags = data->master_fti.fti_flags & fti->fti_flags;

		/* Add filename */
		creator_add_filename(&data->master_fti, fti);

		/* Add datatype group */
		if (!(flags & FTIF_GROUP) || (data->master_fti.fti_datatype_group != fti->fti_datatype_group))
			data->master_fti.fti_flags &= ~FTIF_GROUP;

		/* Add datatype ID */
		if (!(flags & FTIF_ID) || (data->master_fti.fti_datatype_ID != fti->fti_datatype_ID))
			data->master_fti.fti_flags &= ~FTIF_ID;

		/* Add iff type */
		if (!(flags & FTIF_IFF) || (data->master_fti.fti_iff_type != fti->fti_iff_type))
			data->master_fti.fti_flags &= ~FTIF_IFF;

		/* Add bytes */
		creator_add_bytes(&data->master_fti, fti);

		/* Add nocase bytes */
		creator_add_bytesc(&data->master_fti, fti);
	}
}

/********************************/

static void fti_bytes_print_text(char *buf, struct filetype_info *fti, int nocase)
{
	int i;
	int c;
	char *p = buf;
	UBYTE *mask;
	UBYTE *bytes;
	int count;

	if (nocase)
	{
		mask = fti->fti_bytemaskc;
		bytes = fti->fti_filebytesc;
		count = fti->fti_bytecountc;
	}
	else
	{
		mask = fti->fti_bytemask;
		bytes = fti->fti_filebytes;
		count = fti->fti_bytecount;
	}

	for (i = 0; i < count; i++)
	{
		if (mask[i])
		{
			c = bytes[i];

			if (isprint(c) && c != '?' && c != '\\')
				*p++ = c;
			else
			{
				*p++ = '\\';
				*p++ = c / 100 + '0';
				*p++ = c % 100 / 10 + '0';
				*p++ = c % 10 + '0';
			}
		}
		else
			*p++ = '?';
	}
	*p = 0;
}

/********************************/

static void fti_bytes_print_hex(char *buf, struct filetype_info *fti)
{
	int i;
	char *p = buf;
	UBYTE *mask;
	UBYTE *bytes;
	int count;

	mask = fti->fti_bytemask;
	bytes = fti->fti_filebytes;
	count = fti->fti_bytecount;

	*p++ = '$';

	for (i = 0; i < count; i++)
	{
		if (mask[i])
		{
			lsprintf(p, "%02lX", bytes[i] & 255);
			p += 2;
		}
		else
		{
			*p++ = '?';
			*p++ = '?';
		}
	}
	*p = 0;
}

/********************************/

static int fti_bytes_isprint(struct filetype_info *fti)
{
	int i;
	int retval = 0;
	UBYTE *mask;
	UBYTE *bytes;
	int count;

	mask = fti->fti_bytemask;
	bytes = fti->fti_filebytes;
	count = fti->fti_bytecount;

	for (i = 0; i < count; i++)
	{
		if (mask[i])
		{
			retval = isprint(bytes[i]);
			break;
		}
	}

	return retval;
}

/********************************/

static void fti_bytes_print(char *buf, struct filetype_info *fti, int nocase)
{
	if (nocase || fti_bytes_isprint(fti))
		fti_bytes_print_text(buf, fti, nocase);
	else
		fti_bytes_print_hex(buf, fti);
}

/********************************/

static void creator_display_info(creator_data *data)
{
	char buf[4 * BYTECOUNT + 1]; /* Room for longest possible string */
	char *match_bytes;
	ULONG name = FALSE;
	ULONG iff = FALSE;
	ULONG group = FALSE;
	ULONG id = FALSE;
	ULONG bytes = FALSE;

	if (data->match_name)
		free(data->match_name);
	if (data->match_iff)
		free(data->match_iff);
	if (data->match_group)
		free(data->match_group);
	if (data->match_id)
		free(data->match_id);
	if (data->match_bytes)
		free(data->match_bytes);
	if (data->match_bytesc)
		free(data->match_bytesc);

	data->match_name = NULL;
	data->match_iff = NULL;
	data->match_group = NULL;
	data->match_id = NULL;
	data->match_bytes = NULL;
	data->match_bytesc = NULL;

	if (data->master_fti.fti_flags & FTIF_FILENAME)
	{
		if (data->master_fti.fti_flags & FTIF_FILENAME_END)
		{
			lsprintf(buf, "#?%s", (IPTR)data->master_fti.fti_filename);
			data->match_name = strdup(buf);
		}
		else
			data->match_name = strdup(data->master_fti.fti_filename);
	}

	if (data->master_fti.fti_flags & FTIF_IFF)
		data->match_iff = strdup(id_to_str(data->master_fti.fti_iff_type, buf));

	if (data->master_fti.fti_flags & FTIF_GROUP)
		data->match_group = strdup(id_to_str(data->master_fti.fti_datatype_group, buf));

	if (data->master_fti.fti_flags & FTIF_ID)
		data->match_id = strdup(id_to_str(data->master_fti.fti_datatype_ID, buf));

	if (data->master_fti.fti_flags & FTIF_BYTES)
	{
		fti_bytes_print(buf, &data->master_fti, FALSE);
		data->match_bytes = strdup(buf);
	}
	if (data->master_fti.fti_flags & FTIF_BYTESC)
	{
		fti_bytes_print(buf, &data->master_fti, TRUE);
		data->match_bytesc = strdup(buf);
	}

	if (data->match_bytes && !data->match_bytesc)
		data->caseflag = 0;
	else if (data->match_bytesc && !data->match_bytes)
		data->caseflag = 1;

	if (data->caseflag == 0)
		match_bytes = data->match_bytes;
	else
		match_bytes = data->match_bytesc;

	if (data->match_name && !data->match_iff)
		name = TRUE;
	if (data->match_iff)
		iff = TRUE;
	if (data->match_group && !data->match_iff)
		group = TRUE;
	if (data->match_id && !data->match_iff)
		id = TRUE;
	if (match_bytes && !data->match_iff)
		bytes = TRUE;

	SetGadgetValue(data->list, GAD_CREATE_NAME, name);
	DisableObject(data->list, GAD_CREATE_NAME, (ULONG)!data->match_name);
	SetGadgetValue(data->list, GAD_CREATE_NAME_FIELD, (IPTR)(data->match_name ? data->match_name : ""));

	SetGadgetValue(data->list, GAD_CREATE_IFF, iff);
	DisableObject(data->list, GAD_CREATE_IFF, (ULONG)!data->match_iff);
	SetGadgetValue(data->list, GAD_CREATE_IFF_FIELD, (IPTR)(data->match_iff ? data->match_iff : ""));

	SetGadgetValue(data->list, GAD_CREATE_GROUP, group);
	DisableObject(data->list, GAD_CREATE_GROUP, (ULONG)!data->match_group);
	SetGadgetValue(data->list, GAD_CREATE_GROUP_FIELD, (IPTR)(data->match_group ? data->match_group : ""));

	SetGadgetValue(data->list, GAD_CREATE_ID, id);
	DisableObject(data->list, GAD_CREATE_ID, (ULONG)!data->match_id);
	SetGadgetValue(data->list, GAD_CREATE_ID_FIELD, (IPTR)(data->match_id ? data->match_id : ""));

	SetGadgetValue(data->list, GAD_CREATE_BYTES, bytes);
	DisableObject(data->list, GAD_CREATE_BYTES, (ULONG)!match_bytes);
	SetGadgetValue(data->list, GAD_CREATE_BYTES_FIELD, (IPTR)(match_bytes ? match_bytes : ""));

	DisableObject(data->list, GAD_CREATE_CYCLE, !(data->match_bytes && data->match_bytesc));
}

/********************************/

static int creator_warn_edited(creator_data *data)
{
	int ok = FALSE;

	if (data->edited)
		ok = my_SimpleRequest(data->window,
							  GetString(locale, MSG_CREATE_TITLE),
							  GetString(locale, MSG_CREATE_CONTINUE_CANCEL),
							  GetString(locale, MSG_CREATE_WARN_EDITED),
							  NULL);

	if (ok)
		data->edited = FALSE;

	return ok;
}

/********************************/

static int creator_add_file(creator_data *data)
{
	struct filetype_info *fti;
	Att_Node *node;
	struct TagItem tags[3];
	int exists = FALSE;
	int ok = FALSE;

	creator_warn_edited(data);

	if (data->file_list && !data->edited)
	{
		// Fill out tags
		tags[0].ti_Tag = ASLFR_Window;
		tags[0].ti_Data = (IPTR)data->window;
		tags[1].ti_Tag = ASLFR_InitialDrawer;
		tags[1].ti_Data = (IPTR)data->req_dir;
		tags[2].ti_Tag = TAG_DONE;

		// Show filerequester
		if (data->filereq && AslRequest(data->filereq, tags))
		{
			if (file_exists((BPTR)NULL, data->filereq->fr_Drawer, data->filereq->fr_File, EXISTF_FILE))
			{
				// Check for file already in list...
				if ((node = (Att_Node *)FindNameI((struct List *)data->file_list, data->filereq->fr_File)))
				{
					if (stricmp(((struct filetype_info *)node->data)->fti_path, data->filereq->fr_Drawer) == 0)
						exists = TRUE;
				}

				if (!exists && (fti = AllocMemH(data->file_list->memory, sizeof(struct filetype_info))))
				{
					strcpy(fti->fti_filename, data->filereq->fr_File);
					strncpy(fti->fti_path, data->filereq->fr_Drawer, 256);

					strcpy(data->req_dir, data->filereq->fr_Drawer);

					if ((node = Att_NewNode(data->file_list, fti->fti_filename, (IPTR)fti, 0)))
					{
						// Detach list
						SetGadgetChoices(data->list, GAD_CREATE_LISTVIEW, (APTR)~0);

						// Find relevant filetype info
						creator_sniff_filetype(data, (struct filetype_info *)node->data);
						creator_addnode(data, (struct filetype_info *)node->data);

						// Reattach list
						SetGadgetChoices(data->list, GAD_CREATE_LISTVIEW, data->file_list);

						ok = TRUE;
					}
				}
			}
		}
		if (ok)
			creator_display_info(data);
	}

	return ok;
}

/********************************/

static int creator_delete_file(creator_data *data)
{
	int v;
	Att_Node *node;
	int ok = FALSE;

	creator_warn_edited(data);

	if (data->file_list && !data->edited)
	{
		// Detach list
		SetGadgetChoices(data->list, GAD_CREATE_LISTVIEW, (APTR)~0);

		v = GetGadgetValue(data->list, GAD_CREATE_LISTVIEW);

		// Get selected item
		if ((node = Att_FindNode(data->file_list, v)))
		{
			ok = TRUE;

			Att_RemNode(node);

			// Clear current master filetype info
			data->master_fti.fti_flags = 0;

			// Rebuild filetype from remaining list
			for (node = (Att_Node *)data->file_list->list.lh_Head; node->node.ln_Succ;
				 node = (Att_Node *)node->node.ln_Succ)
				creator_addnode(data, (struct filetype_info *)node->data);
		}

		// Reattach list
		SetGadgetChoices(data->list, GAD_CREATE_LISTVIEW, data->file_list);
	}

	if (ok)
		creator_display_info(data);

	return ok;
}

/********************************/

static int creator_clear_files(creator_data *data)
{
	Att_Node *node, *nextnode;
	int ok = FALSE;

	creator_warn_edited(data);

	if (data->file_list && !data->edited)
	{
		ok = TRUE;

		// Detach list
		SetGadgetChoices(data->list, GAD_CREATE_LISTVIEW, (APTR)~0);

		for (node = (Att_Node *)data->file_list->list.lh_Head; node->node.ln_Succ; node = nextnode)
		{
			nextnode = (Att_Node *)node->node.ln_Succ;
			Att_RemNode(node);
		}

		// Clear current master filetype info
		data->master_fti.fti_flags = 0;

		// Reattach list
		SetGadgetChoices(data->list, GAD_CREATE_LISTVIEW, data->file_list);
	}

	if (ok)
		creator_display_info(data);

	return ok;
}

/********************************/

// Init code for process
#ifdef __amigaos4__
ULONG SAVEDS ASM creator_editor_proc_init(REG(a0, IPCData *ipc), REG(a1, creator_data *data))
#else
IPC_StartupCode(creator_editor_proc_init, creator_data *, data)
#endif
{
	ULONG ok = TRUE;

	data->editor_ipc = ipc;

	return ok;
}

/********************************/

// Code for the sub-tasks
void SAVEDS creator_editor_proc_code(void)
{
	creator_data *data;
	// struct Library *DOpusBase;
	struct Library *ConfigOpusBase = NULL;
#ifdef __amigaos4__
	struct ConfigOpusIFace *IConfigOpus = NULL;
#endif
	Cfg_Filetype *edited_filetype;
	int ok = FALSE;

	// DOpusBase = GET_DOPUSLIB;

	IPC_ProcStartup((IPTR *)&data, (APTR)&creator_editor_proc_init);

	// Make a filetype from our current information if filetype has been changed
	if (!data->filetype || !data->edited)
		creator_create_filetype(data);

	if (data->filetype && (ConfigOpusBase = OpenLibrary("DOpus5:Modules/configopus.module", LIB_VERSION)) &&
		GETINTERFACE(IConfigOpus, ConfigOpusBase))
	{
		DisableObject(data->list, GAD_CREATE_LISTVIEW, TRUE);
		DisableObject(data->list, GAD_CREATE_ADD, TRUE);
		DisableObject(data->list, GAD_CREATE_DELETE, TRUE);
		DisableObject(data->list, GAD_CREATE_CLEAR, TRUE);
		DisableObject(data->list, GAD_CREATE_FILETYPE, TRUE);
		DisableObject(data->list, GAD_CREATE_NAME, TRUE);
		DisableObject(data->list, GAD_CREATE_IFF, TRUE);
		DisableObject(data->list, GAD_CREATE_GROUP, TRUE);
		DisableObject(data->list, GAD_CREATE_ID, TRUE);
		DisableObject(data->list, GAD_CREATE_BYTES, TRUE);

		if (*data->filetype_name)
			strcpy(data->filetype->type.name, data->filetype_name);

		if ((edited_filetype = EditFiletype(data->filetype,
											data->window,
											data->editor_ipc,
											// data->main_ipc,
											data->opus_ipc,
											EFTF_EDIT_CLASS)))
		{
			data->edited = TRUE;

			// Replace old filetype with edited one
			FreeFiletype(data->filetype);
			data->filetype = edited_filetype;

			ok = TRUE;
		}

		DisableObject(data->list, GAD_CREATE_LISTVIEW, FALSE);
		DisableObject(data->list, GAD_CREATE_ADD, FALSE);
		DisableObject(data->list, GAD_CREATE_DELETE, FALSE);
		DisableObject(data->list, GAD_CREATE_CLEAR, FALSE);
		DisableObject(data->list, GAD_CREATE_FILETYPE, FALSE);
		DisableObject(data->list, GAD_CREATE_NAME, (ULONG)!data->match_name);
		DisableObject(data->list, GAD_CREATE_IFF, (ULONG)!data->match_iff);
		DisableObject(data->list, GAD_CREATE_GROUP, (ULONG)!data->match_group);
		DisableObject(data->list, GAD_CREATE_ID, (ULONG)!data->match_id);
		DisableObject(data->list, GAD_CREATE_BYTES, (ULONG)!data->match_bytes);

		if (ok)
		{
			strcpy(data->filetype_name, data->filetype->type.name);
			SetGadgetValue(data->list, GAD_CREATE_FILETYPE, (IPTR)data->filetype_name);
		}

#ifdef __amigaos4__
		DropInterface((struct Interface *)IConfigOpus);
#endif
		CloseLibrary(ConfigOpusBase);
	}
	else
		CloseLibrary(ConfigOpusBase);
	DisplayBeep(data->window->WScreen);

	// Flush IPC port
	IPC_Flush(data->editor_ipc);

	// Send goodbye message to owner
	IPC_Goodbye(data->editor_ipc, data->ipc, 0);

	// Free IPC data
	IPC_Free(data->editor_ipc);
	data->editor_ipc = NULL;

	return;
}

/********************************/

int creator_edit_filetype(creator_data *data)
{
	int ok = FALSE;

	if (data->editor_ipc)
		IPC_Command(data->editor_ipc, IPC_ACTIVATE, 0, 0, 0, (struct MsgPort *)-1);
	else
	{
		// Launch a new task
		IPC_Launch(0,								 // List to add task to (optional, but useful)
				   &data->editor_ipc,				 // IPCData ** to store task IPC pointer in (optional)
				   "filetype_editor",				 // Name
				   (IPTR)creator_editor_proc_code,	 // Code
				   STACK_DEFAULT,					 // Stack size
				   (IPTR)data,						 // Data passed to task
				   (struct Library *)DOSBase);		 // Needs pointer to dos.library
	}

	return ok;
}

/********************************/

static int creator_save_filetype(creator_data *data)
{
	int ok = FALSE;
	int mem = FALSE;
	// If its not already made, make it now
	if (!data->filetype)
		creator_create_filetype(data);

	// Do we now have a filetype to save?
	if (data->filetype)
	{
		if (!data->filetype_list)
		{
			data->filetype_list = (APTR)AllocMemH(0, sizeof(Cfg_FiletypeList));
			mem = TRUE;
		}

		if (data->filetype_list)
		{
			if (!exists_req(
					data->window, GetString(locale, MSG_CREATE_TITLE), "DOpus5:Filetypes", data->filetype->type.name))
			{
				strcpy(data->filetype_list->path, data->filetype->type.name);

				DateStamp(&data->filetype_list->date);
				NewList(&data->filetype_list->filetype_list);
				data->filetype->list = data->filetype_list;
				AddHead(&data->filetype_list->filetype_list, (struct Node *)data->filetype);
				data->filetype_list->flags = 0;

				if (!SaveFiletypeList(data->filetype_list, data->filetype_list->path))
					ok = TRUE;
			}
			else if (mem)
			{
				FreeMemH(data->filetype_list);
				data->filetype_list = NULL;
			}
		}
	}

	if (!ok)
		DisplayBeep(data->window->WScreen);

	return ok;
}

/********************************/

static int creator_handle_appwindow(creator_data *data)
{
	struct AppMessage *amsg;
	int a;
	char buffer[256 + 1];
	struct filetype_info *fti;
	Att_Node *node;
	int exists = FALSE;
	int ok = FALSE;

	// Get messages
	while ((amsg = (struct AppMessage *)GetMsg(data->app_port)))
	{
		creator_warn_edited(data);

		// Valid argument?
		if (amsg->am_NumArgs > 0 && !data->edited)
		{
			// Detach list
			SetGadgetChoices(data->list, GAD_CREATE_LISTVIEW, (APTR)~0);

			for (a = 0; a < amsg->am_NumArgs; a++)
			{
				if (file_exists(amsg->am_ArgList[a].wa_Lock, NULL, amsg->am_ArgList[a].wa_Name, EXISTF_FILE))
				{
					NameFromLock(amsg->am_ArgList[a].wa_Lock, buffer, 256);

					// Check for file already in list...
					if ((node = (Att_Node *)FindNameI((struct List *)data->file_list, amsg->am_ArgList[a].wa_Name)))
					{
						if (stricmp(((struct filetype_info *)node->data)->fti_path, buffer) == 0)
							exists = TRUE;
					}

					if (!exists && (fti = AllocMemH(data->file_list->memory, sizeof(struct filetype_info))))
					{
						strcpy(fti->fti_filename, amsg->am_ArgList[a].wa_Name);
						strncpy(fti->fti_path, buffer, 256);

						if ((node = Att_NewNode(data->file_list, fti->fti_filename, (IPTR)fti, 0)))
						{
							// Find relevant filetype info
							creator_sniff_filetype(data, (struct filetype_info *)node->data);
							creator_addnode(data, (struct filetype_info *)node->data);

							ok = TRUE;
						}
					}
				}
			}
			// Reattach list
			SetGadgetChoices(data->list, GAD_CREATE_LISTVIEW, data->file_list);
		}
		// Reply message
		ReplyMsg((struct Message *)amsg);
	}

	if (ok)
		creator_display_info(data);

	return ok;
}

/********************************/

static int creator_event_loop(creator_data *data)
{
	struct IntuiMessage *msg;
	IPCMessage *imsg;
	int cont = 1;
	int break_flag = 0;
	int goodbye_action = GOODBYE_CONTINUE;

	// Event loop
	FOREVER
	{
		// AppMessages?
		if (data->app_window)
			creator_handle_appwindow(data);

		// IPC messages?
		while ((imsg = (IPCMessage *)GetMsg(data->ipc->command_port)))
		{
			if (imsg->command == IPC_GOODBYE)
			{
				// Get goodbye command (removes task from the list)
				IPC_GetGoodbye(imsg);

				if (goodbye_action == GOODBYE_SAVE)
				{
					if (creator_save_filetype(data))
						break_flag = 1;
				}
				else if (goodbye_action == GOODBYE_ABORT)
				{
					break_flag = 1;
					cont = 0;
				}
			}

			// Activate?
			else if (imsg->command == IPC_ACTIVATE)
			{
				WindowToFront(data->window);
				ActivateWindow(data->window);
			}

			// Quit?
			else if (imsg->command == IPC_ABORT || imsg->command == IPC_HIDE || imsg->command == IPC_QUIT)
			{
				if (data->editor_ipc)
				{
					IPC_Quit(data->editor_ipc, FALSE, 0);
					goodbye_action = GOODBYE_ABORT;
				}
				else
				{
					break_flag = 1;
					cont = 0;
				}
			}

			ReplyMsg((struct Message *)imsg);
		}

		// Any Intuition messages?
		while ((msg = GetWindowMsg(data->window->UserPort)))
		{
			struct IntuiMessage copy_msg;

			// Copy message and reply
			copy_msg = *msg;
			ReplyWindowMsg(msg);

			// Gadget?
			if (copy_msg.Class == IDCMP_GADGETUP)
			{
				switch (((struct Gadget *)copy_msg.IAddress)->GadgetID)
				{
				case GAD_CREATE_ADD:
					creator_add_file(data);
					break;

				case GAD_CREATE_DELETE:
					creator_delete_file(data);
					break;

				case GAD_CREATE_CLEAR:
					creator_clear_files(data);
					break;

				case GAD_CREATE_FILETYPE:
					strcpy(data->filetype_name, (char *)GetGadgetValue(data->list, GAD_CREATE_FILETYPE));
					break;

				case GAD_CREATE_CYCLE:
					data->caseflag = !data->caseflag;

					if (data->caseflag == 0)
					{
						DisableObject(data->list, GAD_CREATE_BYTES, (ULONG)!data->match_bytes);
						SetGadgetValue(
							data->list, GAD_CREATE_BYTES_FIELD, (IPTR)(data->match_bytes ? data->match_bytes : ""));
					}
					else
					{
						DisableObject(data->list, GAD_CREATE_BYTES, (ULONG)!data->match_bytesc);
						SetGadgetValue(
							data->list, GAD_CREATE_BYTES_FIELD, (IPTR)(data->match_bytesc ? data->match_bytesc : ""));
					}

					break;

				case GAD_CREATE_EDIT:
					creator_edit_filetype(data);
					break;

				case GAD_CREATE_SAVE:
					if (data->editor_ipc)
					{
						IPC_Quit(data->editor_ipc, TRUE, 0);
						goodbye_action = GOODBYE_SAVE;
					}
					else
					{
						if (creator_save_filetype(data))
							break_flag = 1;
					}
					break;

				case GAD_CREATE_CANCEL:
					if (data->editor_ipc)
					{
						IPC_Quit(data->editor_ipc, FALSE, 0);
						goodbye_action = GOODBYE_ABORT;
					}
					else
					{
						break_flag = 1;
						cont = 0;
					}
					break;
				}
			}

			// Close window?
			else if (copy_msg.Class == IDCMP_CLOSEWINDOW)
			{
				if (data->editor_ipc)
				{
					IPC_Quit(data->editor_ipc, 0, 0);
					goodbye_action = GOODBYE_ABORT;
				}
				else
				{
					break_flag = 1;
					cont = 0;
				}
				break;
			}

			// Key press
			else if (copy_msg.Class == IDCMP_RAWKEY)
			{
				// Help?
				if (copy_msg.Code == 0x5f && !(copy_msg.Qualifier & VALID_QUALIFIERS))
				{
					// Valid main IPC?
					if (data->main_ipc)
					{
						// Set busy pointer
						SetWindowBusy(data->window);

						// Send help request
						IPC_Command(data->main_ipc, IPC_HELP, (1 << 31), "Filetype Creator", 0, (struct MsgPort *)-1);

						// Clear busy pointer
						ClearWindowBusy(data->window);
					}
				}
			}
		}

		// Check break flag
		if (break_flag)
			break;

		// Wait for an event
		Wait(1 << data->window->UserPort->mp_SigBit | (data->app_port ? 1 << data->app_port->mp_SigBit : 0) |
			 (data->ipc ? 1 << data->ipc->command_port->mp_SigBit : 0));
	}

	return cont;
}

/********************************/

static Cfg_FiletypeList *creator_generic(char *args,
										 struct Screen *screen,
										 IPCData *ipc,
										 IPCData *main_ipc,
										 IPCData *opus_ipc,
										 EXT_FUNC(func_callback),
										 Att_List *list,
										 char *path)
{
	creator_data *data;
	Cfg_FiletypeList *ftl = NULL;

	// Allocate data
	if ((data = AllocVec(sizeof(creator_data), MEMF_CLEAR)))
	{
		// Store IPC pointer
		data->screen = screen;
		data->ipc = ipc;
		data->main_ipc = main_ipc;
		data->opus_ipc = opus_ipc;
		data->func_callback = func_callback;

		// Create message port
		data->app_port = CreateMsgPort();

		// Get directory
		strcpy(data->req_dir, path);

		// Default filetype name
		strcpy(data->filetype_name, "Untitled");

		// Allocate filerequester
		data->filereq = AllocAslRequestTags(ASL_FileRequest,
											ASLFR_SleepWindow,
											TRUE,
											ASLFR_TitleText,
											GetString(locale, MSG_CREATE_ADD_FILE),
											ASLFR_Flags1,
											FRF_PRIVATEIDCMP,
											TAG_END);
		if (creator_openwindow(data))
		{
			Att_Node *node;

			SetWindowBusy(data->window);

			// Add selected files to listview
			data->file_list = list;
			SetGadgetChoices(data->list, GAD_CREATE_LISTVIEW, data->file_list);

			for (node = (Att_Node *)data->file_list->list.lh_Head; node->node.ln_Succ;
				 node = (Att_Node *)node->node.ln_Succ)
			{
				// Find relevant filetype info
				creator_sniff_filetype(data, (struct filetype_info *)node->data);
				creator_addnode(data, (struct filetype_info *)node->data);
			}

			creator_display_info(data);

			ClearWindowBusy(data->window);

			creator_event_loop(data);

			ftl = data->filetype_list;

			// Free recognition components
			if (data->match_name)
				free(data->match_name);
			if (data->match_iff)
				free(data->match_iff);
			if (data->match_group)
				free(data->match_group);
			if (data->match_id)
				free(data->match_id);
			if (data->match_bytes)
				free(data->match_bytes);
			if (data->match_bytesc)
				free(data->match_bytesc);

			// Close window
			creator_closewindow(data);
		}
		// Free file requester
		FreeAslRequest(data->filereq);

		// Free message port
		{
			struct Message *msg;
			while ((msg = GetMsg(data->app_port)))
				ReplyMsg(msg);
		}
		DeleteMsgPort(data->app_port);

		FreeVec(data);
	}

	return ftl;
}

/********************************/

static int creator(char *args, struct Screen *screen, IPCData *ipc, IPCData *main_ipc, EXT_FUNC(func_callback))
{
	Att_List *list;
	FunctionEntry *entry;
	Att_Node *node;
	struct endentry_packet end_pkt = {0};
	char path[256];
	struct filetype_info *fti;
	Cfg_FiletypeList *ftl;
	int ok = FALSE;

	if (REFCALL(func_callback, EXTCMD_GET_SOURCE, IPCDATA(ipc), path))
	{
		if ((list = Att_NewList(LISTF_POOL)))
		{
			// Go through files
			while ((entry = (FunctionEntry *)REFCALL(func_callback, EXTCMD_GET_ENTRY, IPCDATA(ipc), 0)))
			{
				if (file_exists((BPTR)NULL, path, entry->name, EXISTF_FILE))
				{
					if ((fti = AllocMemH(list->memory, sizeof(struct filetype_info))))
					{
						if ((node = Att_NewNode(list, entry->name, (IPTR)fti, 0)))
						{
							strcpy(fti->fti_filename, entry->name);
							strncpy(fti->fti_path, path, 256);
							ok = TRUE;
						}
						else
						{
							FreeMemH(fti);
							break;
						}
					}
				}

				// Fill out packet with results
				end_pkt.entry = entry;
				end_pkt.deselect = TRUE;

				// End this entry
				REFCALL(func_callback, EXTCMD_END_ENTRY, IPCDATA(ipc), &end_pkt);
			}

			if ((ftl = creator_generic(args, screen, ipc, main_ipc, main_ipc, func_callback, list, path)))
			{
				FreeMemH(ftl);
				ok = TRUE;
			}

			Att_RemList(list, 0);
		}
	}

	return ok;
}

/********************************/
/********************************/
/********************************/

int LIBFUNC L_Module_Entry(REG(a0, char *args),
						   REG(a1, struct Screen *screen),
						   REG(a2, IPCData *ipc),
						   REG(a3, IPCData *main_ipc),
						   REG(d0, IPTR mod_id),
						   REG(d1, EXT_FUNC(func_callback)))
{
	BPTR olddir = 0, newdir;
	int ret = 0;

	ipc->proc->pr_Task.tc_Node.ln_Name = "filetype finder";

	// Installed filetypes are saved here; create the drawer if it is missing
	if (!(newdir = Lock("DOpus5:Filetypes", ACCESS_READ)) && (newdir = CreateDir("DOpus5:Filetypes")))
	{
		UnLock(newdir);
		newdir = Lock("DOpus5:Filetypes", ACCESS_READ);
	}
	if (newdir)
		olddir = CurrentDir(newdir);

	if (mod_id == MODID_FIND_FT)
		ret = finder(screen, ipc, main_ipc, func_callback);
	else if (mod_id == MODID_CREATE_FT)
		ret = creator(args, screen, ipc, main_ipc, func_callback);

	if (newdir)
		UnLock(CurrentDir(olddir));

	return ret;
}
