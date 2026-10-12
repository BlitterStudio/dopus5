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

/*
	04 Nov 99  Changed diskobjectcopy to use DupDiskObject for V44


*/

#include "dopuslib.h"

#include <proto/module.h>
#include <proto/newicon.h>

//#define UtilityBase	(wb_data->utility_base)
//#define WorkbenchBase	(wb_data->wb_base)
//#define IntuitionBase	(wb_data->int_base)

//#define NewIconBase	(data->new_icon_base)
//#define IconBase	(data->icon_base)
//#define DOSBase	(data->dos_base)

#ifdef __amigaos3__
static inline void atomic_inc(ULONG *counter)
{
	asm volatile("addql #1,%0" : "+m"(*counter));
}

static inline void atomic_dec(ULONG *counter)
{
	asm volatile("subql #1,%0" : "+m"(*counter));
}

// Calls the original AddAppIconA. A C call through a function pointer cannot
// load d0/d1/a0-a4/a6 and still keep an address register for the target, so
// the stub loads the target into a5 (saved here).
APTR wb_call_addappicon(APTR func,
						struct Library *base,
						IPTR id,
						IPTR userdata,
						char *text,
						struct MsgPort *port,
						BPTR lock,
						struct DiskObject *icon,
						struct TagItem *tags);

asm(".text                       \n\
     .even                       \n\
     .globl _wb_call_addappicon  \n\
   _wb_call_addappicon:          \n\
      moveml a2-a6,sp@-          \n\
      movel sp@(24),a5           \n\
      movel sp@(28),a6           \n\
      movel sp@(32),d0           \n\
      movel sp@(36),d1           \n\
      movel sp@(40),a0           \n\
      movel sp@(44),a1           \n\
      movel sp@(48),a2           \n\
      movel sp@(52),a3           \n\
      movel sp@(56),a4           \n\
      jsr a5@                    \n\
      moveml sp@+,a2-a6          \n\
      rts");
#elif defined(__AROS__) && defined(__arm__)
	#include <aros/atomic.h>
static inline void atomic_inc(ULONG *counter)
{
	__AROS_ATOMIC_INC_L(*counter);
}

static inline void atomic_dec(ULONG *counter)
{
	__AROS_ATOMIC_DEC_L(*counter);
}
#elif defined(__MORPHOS__)
	#include <hardware/atomic.h>
static inline void atomic_inc(ULONG *counter)
{
	ATOMIC_ADD(counter, 1);
}

static inline void atomic_dec(ULONG *counter)
{
	ATOMIC_SUB(counter, 1);
}
#else
static inline void atomic_inc(ULONG *counter)
{
	__sync_add_and_fetch(counter, 1);
}

static inline void atomic_dec(ULONG *counter)
{
	__sync_sub_and_fetch(counter, 1);
}
#endif

// the use count has to be increased as soon as we enter a function,
// so this can't be stored in wb_data, unless we introduce a global
// pointer to it
ULONG usecount[WB_PATCH_COUNT];
static APTR old_findtask_function;

//// AddAppWindow patch
PATCHED_5(struct AppWindow *,
		  LIBFUNC L_WB_AddAppWindow,
		  d0,
		  IPTR,
		  id,
		  d1,
		  IPTR,
		  userdata,
		  a0,
		  struct Window *,
		  window,
		  a1,
		  struct MsgPort *,
		  port,
		  a2,
		  struct TagItem *,
		  tags)
{
	atomic_inc(&usecount[WB_PATCH_ADDAPPWINDOWA]);
	{
		struct MyLibrary *libbase;
		AppEntry *app_entry;
		WB_Data *wb_data;

		// Check valid data
		if (!window || !port)
		{
			atomic_dec(&usecount[WB_PATCH_ADDAPPWINDOWA]);
			return NULL;
		}

		// Open library
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_ADDAPPWINDOWA]);
			return NULL;
		}

		// Get Workbench data pointer
		wb_data = &((struct LibData *)libbase->ml_UserData)->wb_data;

		// Get new AppEntry
		if ((app_entry = new_app_entry(APP_WINDOW, id, userdata, window, 0, port, wb_data)))
		{
			// Local AppThing?
			if (tags && GetTagData(DAE_Local, 0, tags))
			{
				// Set local flag
				app_entry->flags |= APPENTF_LOCAL;
			}

			// Otherwise pass to OS
			else
			{
				app_entry->os_object = LIBCALL_5(struct AppWindow *,
												 wb_data->old_function[WB_PATCH_ADDAPPWINDOWA],
												 WorkbenchBase,
												 IWorkbench,
												 d0,
												 id,
												 d1,
												 userdata,
												 a0,
												 window,
												 a1,
												 port,
												 a2,
												 tags);
			}

			// Send notification
			L_SendNotifyMsg(DN_APP_WINDOW_LIST, (IPTR)app_entry, 0, FALSE, 0, 0, libbase);
		}

		// Return object
		atomic_dec(&usecount[WB_PATCH_ADDAPPWINDOWA]);
		return (struct AppWindow *)app_entry;
	}
}
PATCH_END

//// AddAppMenuItem patch
PATCHED_5(struct AppMenuItem *,
		  LIBFUNC L_WB_AddAppMenuItem,
		  d0,
		  IPTR,
		  id,
		  d1,
		  IPTR,
		  userdata,
		  a0,
		  char *,
		  text,
		  a1,
		  struct MsgPort *,
		  port,
		  a2,
		  struct TagItem *,
		  tags)
{
	atomic_inc(&usecount[WB_PATCH_ADDAPPMENUA]);
	{
		struct MyLibrary *libbase;
		AppEntry *app_entry = NULL;
		WB_Data *wb_data;
		BOOL osonly = FALSE;
		APTR object = NULL;

		// Open library
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_ADDAPPMENUA]);
			return NULL;
		}

		// Get Workbench data pointer
		wb_data = &((struct LibData *)libbase->ml_UserData)->wb_data;

		// Pass straight through? Workbench's own Tools menu items (its ARexx
		// MENU ADD, used by MenuTools and AutoArrangeIcons on OS 3.2) have no
		// message port, so Opus could not run them; Workbench does.
		if (!port || (tags && !(GetTagData(DAE_Local, 1, tags))))
			osonly = TRUE;

		// Get new AppEntry
		if (osonly || (app_entry = new_app_entry(APP_MENU, id, userdata, 0, text, port, wb_data)))
		{
			// Local AppThing?
			if (!osonly && tags && GetTagData(DAE_Local, 0, tags))
			{
				// Set local flag
				app_entry->flags |= APPENTF_LOCAL;
			}

			// Otherwise pass to OS
			else
			{
				object = LIBCALL_5(APTR,
								   wb_data->old_function[WB_PATCH_ADDAPPMENUA],
								   WorkbenchBase,
								   IWorkbench,
								   d0,
								   id,
								   d1,
								   userdata,
								   a0,
								   text,
								   a1,
								   port,
								   a2,
								   tags);

				if (app_entry)
					app_entry->os_object = object;
			}

			// Send notification
			if (app_entry)
			{
				struct Task *thistask;
				short toolmanger = 0;

				// Get current task
				thistask = FindTask(NULL);

				// Is this from ToolManager?
				if (thistask->tc_Node.ln_Name && strcmp(thistask->tc_Node.ln_Name, "ToolManager Handler") == 0)
					toolmanger = 1;

				// Send notify message
				L_SendNotifyMsg(DN_APP_MENU_LIST, (IPTR)app_entry, toolmanger, FALSE, 0, 0, libbase);
			}
		}

		// Return object
		atomic_dec(&usecount[WB_PATCH_ADDAPPMENUA]);
		return (struct AppMenuItem *)((app_entry) ? (APTR)app_entry : object);
	}
}
PATCH_END

//// AddAppIcon patch
PATCHED_7(struct AppIcon *,
		  LIBFUNC L_WB_AddAppIcon,
		  d0,
		  IPTR,
		  id,
		  d1,
		  IPTR,
		  userdata,
		  a0,
		  char *,
		  text,
		  a1,
		  struct MsgPort *,
		  port,
		  a2,
		  BPTR,
		  lock,
		  a3,
		  struct DiskObject *,
		  icon,
		  a4,
		  struct TagItem *,
		  tags)
{
	atomic_inc(&usecount[WB_PATCH_ADDAPPICONA]);
	{
		struct MyLibrary *libbase;
		AppEntry *app_entry = NULL;
		struct LibData *data;
		WB_Data *wb_data;
		BOOL osonly = FALSE;
		APTR object = NULL;
		short local = 2;

		// Check valid data
		if (!icon || !port)
		{
			atomic_dec(&usecount[WB_PATCH_ADDAPPICONA]);
			return NULL;
		}

		// Open library
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_ADDAPPICONA]);
			return NULL;
		}

		// Get Workbench data pointer
		data = (struct LibData *)libbase->ml_UserData;
		wb_data = &data->wb_data;

		// Pass straight through to OS?
		if (tags && (local = GetTagData(DAE_Local, 2, tags)) == 0)
			osonly = TRUE;

		// Otherwise, if it's not a local icon and the redirection flag is set, send to Tools menu
		else if (local == 2 && data->flags & LIBDF_REDIRECT_TOOLS)
		{
			struct AppMenuItem *item;

// Add as menu item
#if defined(__MORPHOS__)
			REG_D0 = id;
			REG_D1 = userdata;
			REG_A0 = (ULONG)text;
			REG_A1 = (ULONG)port;
			REG_A2 = (ULONG)tags;
			item = L_WB_AddAppMenuItem();
#else
			item = L_WB_AddAppMenuItem(id, userdata, text, port, tags);
#endif

			atomic_dec(&usecount[WB_PATCH_ADDAPPICONA]);
			return (struct AppIcon *)item;
		}

		// Get new AppEntry
		if (osonly || (app_entry = new_app_entry(APP_ICON, id, userdata, icon, text, port, wb_data)))
		{
			short a;
			struct TagItem *tag;

			// Any tags?
			if (tags && app_entry)
			{
				struct TagItem *tstate;

				// Support snapshot?
				if (GetTagData(DAE_SnapShot, 0, tags))
				{
					// Set flag
					app_entry->flags |= APPENTF_SNAPSHOT;
				}

				// Support info?
				if (GetTagData(DAE_Info, 0, tags))
				{
					// Set flag
					app_entry->flags |= APPENTF_INFO;
				}

				// Locked?
				if (GetTagData(DAE_Locked, 0, tags))
				{
					// Set flag
					app_entry->flags |= APPENTF_LOCKED;
				}

				// Close item?
				if ((a = GetTagData(DAE_Close, 0, tags)))
				{
					// Set flag
					app_entry->flags |= APPENTF_CLOSE;

					// No open?
					if (a == 2)
						app_entry->flags |= APPENTF_NO_OPEN;
				}

				// Background colour?
				if ((tag = FindTagItem(DAE_Background, tags)))
				{
					// Set flag
					app_entry->flags |= APPENTF_BACKGROUND;

					// Store colour;
					app_entry->data = tag->ti_Data;
				}

				// Special?
				if (GetTagData(DAE_Special, 0, tags))
				{
					// Set flag
					app_entry->flags |= APPENTF_SPECIAL;
				}

				// Set menu base
				app_entry->menu_id_base = GetTagData(DAE_MenuBase, 0, tags);

				// Workbench render hook; the desktop calls it to draw the icon
				app_entry->render_hook = (struct Hook *)GetTagData(WBAPPICONA_RenderHook, 0, tags);

				// Go through tags
				tstate = tags;
				while ((tag = NextTagItem(&tstate)))
				{
					// Menu item?
					if ((tag->ti_Tag == DAE_Menu || tag->ti_Tag == DAE_ToggleMenu ||
						 tag->ti_Tag == DAE_ToggleMenuSel) &&
						tag->ti_Data)
					{
						struct Node *node;

						// Allocate space for node and name
						if ((node = AllocVec(sizeof(struct Node) + strlen((char *)tag->ti_Data) + 1, MEMF_CLEAR)))
						{
							// Copy name
							node->ln_Name = (char *)(node + 1);
							strcpy(node->ln_Name, (char *)tag->ti_Data);

							// Set flags
							if (tag->ti_Tag == DAE_ToggleMenu)
								node->ln_MenuFlags = MNF_TOGGLE;
							else if (tag->ti_Tag == DAE_ToggleMenuSel)
								node->ln_MenuFlags = MNF_TOGGLE | MNF_SEL;

							// Add to list
							AddTail((struct List *)&app_entry->menu, (struct Node *)node);
						}
					}
				}
			}

			// Local AppThing?
			if (local == 1 && !osonly)
			{
				// Set local flag
				app_entry->flags |= APPENTF_LOCAL;
			}

			// Otherwise, pass to OS
			else
			{
#ifdef __amigaos3__
				object = wb_call_addappicon(wb_data->old_function[WB_PATCH_ADDAPPICONA],
											WorkbenchBase,
											id,
											userdata,
											text,
											port,
											lock,
											icon,
											tags);
#else
				object = LIBCALL_7(APTR,
								   wb_data->old_function[WB_PATCH_ADDAPPICONA],
								   WorkbenchBase,
								   IWorkbench,
								   d0,
								   id,
								   d1,
								   userdata,
								   a0,
								   text,
								   a1,
								   port,
								   a2,
								   lock,
								   a3,
								   icon,
								   a4,
								   tags);
#endif
				if (app_entry)
					app_entry->os_object = object;
			}

			// Send notification
			if (app_entry)
				L_SendNotifyMsg(DN_APP_ICON_LIST, (IPTR)app_entry, 0, FALSE, 0, 0, libbase);
		}

		// Return object
		atomic_dec(&usecount[WB_PATCH_ADDAPPICONA]);
		return (struct AppIcon *)((app_entry) ? (APTR)app_entry : object);
	}
}
PATCH_END

//// RemoveAppWindow patch
PATCHED_1(BOOL, LIBFUNC L_WB_RemoveAppWindow, a0, struct AppWindow *, window)
{
	atomic_inc(&usecount[WB_PATCH_REMAPPWINDOW]);
	{
		AppEntry *entry;
		struct MyLibrary *libbase;
		WB_Data *wb_data;
		APTR os_object;

		// Valid pointer?
		if (!(entry = (AppEntry *)window))
		{
			atomic_dec(&usecount[WB_PATCH_REMAPPWINDOW]);
			return TRUE;
		}

		// Open library
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_REMAPPWINDOW]);
			return FALSE;
		}

		// Get Workbench data pointer
		wb_data = &((struct LibData *)libbase->ml_UserData)->wb_data;

		// Free entry
		os_object = rem_app_entry(entry, wb_data, 0);

		// Workbench object?
		if (os_object)
		{
			// Remove workbench object
			LIBCALL_1(BOOL, wb_data->old_function[WB_PATCH_REMAPPWINDOW], WorkbenchBase, IWorkbench, a0, os_object);
		}

		// Send notification
		L_SendNotifyMsg(DN_APP_WINDOW_LIST, (IPTR)window, DNF_WINDOW_REMOVED, FALSE, 0, 0, libbase);

		atomic_dec(&usecount[WB_PATCH_REMAPPWINDOW]);
		return TRUE;
	}
}
PATCH_END

//// RemoveAppMenuItem patch
PATCHED_1(BOOL, LIBFUNC L_WB_RemoveAppMenuItem, a0, struct AppMenuItem *, item)
{
	atomic_inc(&usecount[WB_PATCH_REMAPPMENU]);
	{
		AppEntry *entry;
		struct MyLibrary *libbase;
		WB_Data *wb_data;
		APTR os_object;
		BOOL local;

		// Valid pointer?
		if (!(entry = (AppEntry *)item))
		{
			atomic_dec(&usecount[WB_PATCH_REMAPPMENU]);
			return TRUE;
		}

		// Open library
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_REMAPPMENU]);
			return FALSE;
		}

		// Get Workbench data pointer
		wb_data = &((struct LibData *)libbase->ml_UserData)->wb_data;

		// Free entry
		os_object = rem_app_entry(entry, wb_data, &local);

		// Workbench object?
		if (os_object)
		{
			// Remove workbench object
			LIBCALL_1(BOOL, wb_data->old_function[WB_PATCH_REMAPPMENU], WorkbenchBase, IWorkbench, a0, os_object);
		}

		// Send notification
		if (os_object != (APTR)entry)
			L_SendNotifyMsg(DN_APP_MENU_LIST, (IPTR)entry, 1, !local, 0, 0, libbase);

		atomic_dec(&usecount[WB_PATCH_REMAPPMENU]);
		return TRUE;
	}
}
PATCH_END

//// RemoveAppIcon patch
PATCHED_1(BOOL, LIBFUNC L_WB_RemoveAppIcon, a0, struct AppIcon *, icon)
{
	atomic_inc(&usecount[WB_PATCH_REMAPPICON]);
	{
		AppEntry *entry;
		struct MyLibrary *libbase;
		WB_Data *wb_data;
		APTR os_object;
		BOOL local;

		// Valid pointer?
		if (!(entry = (AppEntry *)icon))
		{
			atomic_dec(&usecount[WB_PATCH_REMAPPICON]);
			return TRUE;
		}

		// Open library
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_REMAPPICON]);
			return FALSE;
		}

		// Get Workbench data pointer
		wb_data = &((struct LibData *)libbase->ml_UserData)->wb_data;

		// Check valid object
		if (find_app_entry(entry, wb_data))
		{
			// Is it a menu (must have been redirected)?
			if (entry->type == APP_MENU)
			{
				BOOL result;
				// Unlock list
				L_FreeSemaphore(&wb_data->patch_lock);

// Remove as menu item
#if defined(__MORPHOS__)
				REG_A0 = (ULONG)icon;
				result = L_WB_RemoveAppMenuItem();
#else
				result = L_WB_RemoveAppMenuItem((struct AppMenuItem *)icon);
#endif

				atomic_dec(&usecount[WB_PATCH_REMAPPICON]);
				return result;
			}

			// Send notification
			L_SendNotifyMsg(DN_APP_ICON_LIST, (IPTR)entry, DNF_ICON_REMOVED, FALSE, 0, 0, libbase);

			//*********************************************************************TRUE ???

			// Unlock list
			L_FreeSemaphore(&wb_data->patch_lock);
		}

		// Free entry
		if ((os_object = rem_app_entry(entry, wb_data, &local)))
		{
			// Remove workbench object
			LIBCALL_1(BOOL, wb_data->old_function[WB_PATCH_REMAPPICON], WorkbenchBase, IWorkbench, a0, os_object);
		}

		atomic_dec(&usecount[WB_PATCH_REMAPPICON]);
		return TRUE;
	}
}
PATCH_END

// Find AppWindow
struct AppWindow *LIBFUNC L_WB_FindAppWindow(REG(a0, struct Window *window), REG(a6, struct MyLibrary *libbase))
{
	AppEntry *app_entry, *app_window = NULL;
	WB_Data *wb_data;

#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Get Workbench data pointer
	wb_data = &((struct LibData *)libbase->ml_UserData)->wb_data;

	// Lock list
	L_GetSemaphore(&wb_data->patch_lock, SEMF_SHARED, 0);

	// Go through app objects
	for (app_entry = (AppEntry *)wb_data->app_list.mlh_Head; app_entry->node.mln_Succ;
		 app_entry = (AppEntry *)app_entry->node.mln_Succ)
	{
		// Is this the window?
		if (app_entry->type == APP_WINDOW && app_entry->object == window)
		{
			// Forbid so window can't go
			Forbid();

			// Save pointer
			app_window = app_entry;
			break;
		}
	}

	// Unlock list
	L_FreeSemaphore(&wb_data->patch_lock);
	return (struct AppWindow *)app_window;
}

// Get AppWindow data
struct MsgPort *LIBFUNC L_WB_AppWindowData(REG(a0, struct AppWindow *window),
										   REG(a1, IPTR *id),
										   REG(a2, IPTR *userdata))
{
	AppEntry *entry;

	// Valid entry?
	if ((entry = (AppEntry *)window))
	{
		if (id)
			*id = entry->id;
		if (userdata)
			*userdata = entry->userdata;
		return entry->port;
	}
	return NULL;
}

// Is an AppWindow "local"?
BOOL LIBFUNC L_WB_AppWindowLocal(REG(a0, struct AppWindow *window))
{
	return (BOOL)((((AppEntry *)window)->flags & APPENTF_LOCAL) ? 1 : 0);
}

// Get AppWindow window pointer
struct Window *LIBFUNC L_WB_AppWindowWindow(REG(a0, struct AppWindow *window))
{
	return (struct Window *)(((AppEntry *)window)->object);
}

// Get AppIcon flags
unsigned long LIBFUNC L_WB_AppIconFlags(REG(a0, struct AppIcon *icon))
{
	return ((AppEntry *)icon)->flags;
}

// Add a new AppEntry
AppEntry *
new_app_entry(ULONG type, IPTR id, IPTR userdata, APTR object, char *text, struct MsgPort *port, WB_Data *wb_data)
{
	AppEntry *entry;

	// Allocate new entry
	if (!(entry = AllocVec(sizeof(AppEntry), MEMF_CLEAR)))
		return NULL;

	// Fill out AppEntry
	entry->type = type;
	entry->id = id;
	entry->userdata = userdata;
	entry->text = text;
	entry->port = port;
	NewList((struct List *)&entry->menu);

	// AppIcon?
	if (type == APP_ICON)
	{
		struct DiskObject *icon_copy;

		// Copy icon
		if ((icon_copy = L_CopyDiskObject((struct DiskObject *)object, 0, wb_data->dopus_base)))
		{
			// Use copy
			entry->object = icon_copy;
			entry->flags |= APPENTF_ICON_COPY;
		}

		// Couldn't copy
		else
		{
			FreeVec(entry);
			return NULL;
		}
	}

	// Otherwise, save object pointer
	else
		entry->object = object;

	// Lock patch list
	L_GetSemaphore(&wb_data->patch_lock, SEMF_EXCLUSIVE, 0);

	// Is this the first entry?
	if (IsListEmpty((struct List *)&wb_data->app_list) && IsListEmpty((struct List *)&wb_data->rem_app_list))
	{
		// Bump library open count so we won't get expunged
		++wb_data->dopus_base->libBase.lib_OpenCnt;
#warning D(bug) printf there cause crash on os3 build (and on real, and on emulation on os3/mos). TODO: invistigate and make a proper one
#ifndef __amigaos3__
		D(bug("lib_OpenCnt bumped to %d by task '%s'\n",
			  wb_data->dopus_base->libBase.lib_OpenCnt,
			  FindTask(NULL)->tc_Node.ln_Name));
#endif
	}

	// Add to list
	AddTail((struct List *)&wb_data->app_list, (struct Node *)entry);

	// Unlock list
	L_FreeSemaphore(&wb_data->patch_lock);
	return entry;
}

// Remove an AppEntry
APTR rem_app_entry(AppEntry *entry, WB_Data *wb_data, BOOL *local)
{
	AppEntry *app_entry;
	APTR retval;

	// Save pointer
	retval = entry;

	// Clear local flag
	if (local)
		*local = FALSE;

	// Lock AppEntry list
	L_GetSemaphore(&wb_data->patch_lock, SEMF_EXCLUSIVE, 0);

	// Look for entry in list
	for (app_entry = (AppEntry *)wb_data->app_list.mlh_Head; app_entry != entry && app_entry->node.mln_Succ;
		 app_entry = (AppEntry *)app_entry->node.mln_Succ)
		;

	// Found match?
	if (app_entry == entry)
	{
		// Remove from list
		Remove((struct Node *)entry);

		// Return OS object pointer
		retval = entry->os_object;

		// Local?
		if (local && entry->flags & APPENTF_LOCAL)
			*local = TRUE;

		// Add to removal list
		AddTail((struct List *)&wb_data->rem_app_list, (struct Node *)entry);

		// Zero the count
		entry->menu_id_base = 0;
	}

	// Unlock list
	L_FreeSemaphore(&wb_data->patch_lock);

	// Return pointer to OS object
	return retval;
}

// Free an entry from the removal list
void free_app_entry(AppEntry *entry, WB_Data *wb_data)
{
	struct MinNode *node;

	// Remove entry from list
	Remove((struct Node *)entry);

	// Free menu nodes
	for (node = entry->menu.mlh_Head; node->mln_Succ;)
	{
		// Cache next pointer
		struct MinNode *next = node->mln_Succ;

		// Free node
		FreeVec(node);

		// Get next
		node = next;
	}

	// Free icon
	if (entry->flags & APPENTF_ICON_COPY)
		L_FreeDiskObjectCopy((struct DiskObject *)entry->object, wb_data->dopus_base);

	// Free data structure
	FreeVec(entry);
}

// Find an AppEntry
AppEntry *find_app_entry(AppEntry *entry, WB_Data *wb_data)
{
	AppEntry *app_entry;

	// Lock AppEntry list
	L_GetSemaphore(&wb_data->patch_lock, SEMF_SHARED, 0);

	// Look for entry in list
	for (app_entry = (AppEntry *)wb_data->app_list.mlh_Head; app_entry != entry && app_entry->node.mln_Succ;
		 app_entry = (AppEntry *)app_entry->node.mln_Succ)
		;

	// Found match?
	if (app_entry == entry)
		return entry;

	// Unlock list
	L_FreeSemaphore(&wb_data->patch_lock);

	// Not found
	return NULL;
}

// Lock the AppList
APTR LIBFUNC L_LockAppList(REG(a6, struct MyLibrary *libbase))
{
	WB_Data *wb_data;

#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Get Workbench data pointer
	wb_data = &((struct LibData *)libbase->ml_UserData)->wb_data;

	// Get a lock on the list
	L_GetSemaphore(&wb_data->patch_lock, SEMF_SHARED, 0);

	// Increment lock count
	++wb_data->lock_count;

	// Return indicates start of list
	return wb_data;
}

// Get the next app entry of a certain type
APTR LIBFUNC L_NextAppEntry(REG(a0, APTR last), REG(d0, ULONG type), REG(a6, struct MyLibrary *libbase))
{
	WB_Data *wb_data;
	AppEntry *entry;

#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Get Workbench data pointer
	wb_data = &((struct LibData *)libbase->ml_UserData)->wb_data;

	// Invalid "last" pointer?
	if (!last)
		return NULL;

	// Start of list?
	if (last == wb_data)
		entry = (AppEntry *)wb_data->app_list.mlh_Head;

	// Otherwise, go from next entry
	else
		entry = (AppEntry *)((AppEntry *)last)->node.mln_Succ;

	// Scan list
	while (entry->node.mln_Succ)
	{
		// Correct type?
		if (entry->type == type)
			return entry;

		// Get next entry
		entry = (AppEntry *)entry->node.mln_Succ;
	}

	// Not found
	return NULL;
}

// Unlock the AppList
void LIBFUNC L_UnlockAppList(REG(a6, struct MyLibrary *libbase))
{
	WB_Data *wb_data;

#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Get Workbench data pointer
	wb_data = &((struct LibData *)libbase->ml_UserData)->wb_data;

	// Check it's locked
	if (wb_data->lock_count > 0)
	{
		// Unlock the list
		L_FreeSemaphore(&wb_data->patch_lock);

		// Decrement lock count
		--wb_data->lock_count;
	}
}

//// CloseWorkbench patch
PATCHED_0(LONG, LIBFUNC L_WB_CloseWorkBench)
{
	atomic_inc(&usecount[WB_PATCH_CLOSEWORKBENCH]);
	{
		WB_Data *wb_data;
		struct MyLibrary *libbase;
		LONG result;

		// Open library
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_CLOSEWORKBENCH]);
			return 0;
		}

		// Get Workbench data pointer
		wb_data = &((struct LibData *)libbase->ml_UserData)->wb_data;

		// Send notification
		L_SendNotifyMsg(DN_CLOSE_WORKBENCH, 0, 0, FALSE, 0, 0, libbase);

		// Close workbench screen
		result = LIBCALL_0(LONG, wb_data->old_function[WB_PATCH_CLOSEWORKBENCH], IntuitionBase, IIntuition);

		atomic_dec(&usecount[WB_PATCH_CLOSEWORKBENCH]);
		return result;
	}
}
PATCH_END

//// OpenWorkbench patch
PATCHED_0(ULONG, LIBFUNC L_WB_OpenWorkBench)
{
	atomic_inc(&usecount[WB_PATCH_OPENWORKBENCH]);
	{
		WB_Data *wb_data;
		struct MyLibrary *libbase;
		ULONG result;

		// Open library
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_OPENWORKBENCH]);
			return 0;
		}

		// Get Workbench data pointer
		wb_data = &((struct LibData *)libbase->ml_UserData)->wb_data;

		// Send notification
		L_SendNotifyMsg(DN_OPEN_WORKBENCH, 0, 0, FALSE, 0, 0, libbase);

		// Open workbench screen
		result = LIBCALL_0(ULONG, wb_data->old_function[WB_PATCH_OPENWORKBENCH], IntuitionBase, IIntuition);

		atomic_dec(&usecount[WB_PATCH_OPENWORKBENCH]);
		return result;
	}
}
PATCH_END

#ifdef __amigaos4__
BOOL internalOpenWBObject(struct MyLibrary *libbase, CONST_STRPTR name, const struct TagItem *tags)
{
	BOOL result = FALSE;
	struct LibData *libdata = (struct LibData *)libbase->ml_UserData;

	enum { viaWB, viaCLI, viaREXX };

	short via = viaWB;

	if (name)
	{
		/* an object was named to be opened */
		struct DiskObject *icon =
			GetIconTags(name, ICONGETA_FailIfUnavailable, TRUE, ICONGETA_Screen, *libdata->backfill_screen, TAG_DONE);

		// D(bug("Icon: %p  type: %ld\n", icon, icon?icon->do_Type:0 ));

		if (icon)
		{
			if (FindToolType(icon->do_ToolTypes, "CLI"))
				via = viaCLI;
			else if (FindToolType(icon->do_ToolTypes, "AREXX"))
				via = viaREXX;
		}

		if (via == viaWB)
		{
			D(bug("Launching via WB\n"));
			result = L_WB_Launch((char *)name, NULL, FALSE);
		}
		else if (via == viaCLI)
		{
			BPTR cd, dupcd = 0, input;

			if (!(input = Open("CON:20/10/600/180/Directory Opus Output/AUTO/WAIT/CLOSE", MODE_OLDFILE)))
				input = Open("nil:", MODE_OLDFILE);

			if ((cd = GetCurrentDir()))
				dupcd = DupLock(cd);

			D(bug("Launching %s via CLI\n", name));
			result = L_CLI_Launch((char *)name, *libdata->backfill_screen, dupcd, input, 0, LAUNCHF_USE_STACK, 65535);
		}
		else if (via == viaREXX)
		{
			D(bug("Running via ARexx not yet supported\n"));
		}

		if (icon)
			FreeDiskObject(icon);
	}
	else
	{
		/* no object named, error */
	}

	return result;
}

//// OpenWorkbenchObject patch
VARARGS68K BOOL L_WB_OpenWorkbenchObject_stubs(struct WorkbenchIFace *IWorkbench, CONST_STRPTR name, ...)
{
	atomic_inc(&usecount[WB_PATCH_OPENWORKBENCHOBJECT]);
	{
		WB_Data *wb_data;
		struct MyLibrary *libbase;
		va_list ap;
		struct TagItem *tags;
		BOOL result = FALSE;

		va_startlinear(ap, name);

		tags = va_getlinearva(ap, struct TagItem *);

		D(bug("called by %s\n", FindTask(NULL)->tc_Node.ln_Name));

		// Open library
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_OPENWORKBENCHOBJECT]);
			return FALSE;
		}

		// Get Workbench data pointer
		wb_data = &((struct LibData *)libbase->ml_UserData)->wb_data;

		if (FindPort("WORKBENCH"))
		{
			// D(bug("Workbench available, calling original vector\n"));

			// call original, but check examples of how all of this can be handled in another PATCHED functions there.
			result = LIBCALL_2(BOOL,
							   wb_data->old_function[WB_PATCH_OPENWORKBENCHOBJECTA],
							   WorkbenchBase,
							   IWorkbench,
							   a0,
							   name,
							   a1,
							   tags);
		}
		else
		{
			// D(bug("Workbench not found, launching %s ourselves\n", name));
			result = internalOpenWBObject(libbase, name, tags);
		}

		atomic_dec(&usecount[WB_PATCH_OPENWORKBENCHOBJECT]);
		return FALSE;
	}
}

//// OpenWorkbenchObjectA patch
PATCHED_2(BOOL, LIBFUNC L_WB_OpenWorkbenchObjectA, a0, CONST_STRPTR, name, a1, const struct TagItem *, tags)
{
	atomic_inc(&usecount[WB_PATCH_OPENWORKBENCHOBJECTA]);
	{
		WB_Data *wb_data;
		struct MyLibrary *libbase;
		BOOL result = FALSE;

		D(bug("called by %s\n", FindTask(NULL)->tc_Node.ln_Name));

		// Open library
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_OPENWORKBENCHOBJECTA]);
			return FALSE;
		}

		// Get Workbench data pointer
		wb_data = &((struct LibData *)libbase->ml_UserData)->wb_data;

		if (FindPort("WORKBENCH"))
		{
			// D(bug("Workbench available, calling original vector\n"));

			// call original, but check examples of how all of this can be handled in another PATCHED functions there.
			result = LIBCALL_2(BOOL,
							   wb_data->old_function[WB_PATCH_OPENWORKBENCHOBJECTA],
							   WorkbenchBase,
							   IWorkbench,
							   a0,
							   name,
							   a1,
							   tags);
		}
		else
		{
			// D(bug("Workbench not found, launching %s ourselves\n", name ));
			result = internalOpenWBObject(libbase, name, tags);
		}

		atomic_dec(&usecount[WB_PATCH_OPENWORKBENCHOBJECTA]);
		return result;
	}
}
PATCH_END

/* this function is called by the WorkbenchControl patches.
** Currently only supported tag is WBCTRLA_DuplicateSearchPath,
** but we may be able to expand this later for better compatibility.
*/
BOOL internalWBCtrl(struct MyLibrary *libbase, CONST_STRPTR name, const struct TagItem *tags)
{
	struct TagItem *tlist = (struct TagItem *)tags, *tag;
	BOOL result = FALSE;

	while ((tag = NextTagItem(&tlist)))
	{
		switch (tag->ti_Tag)
		{
		case WBCTRLA_IsOpen:
			D(bug("WBCTRLA_IsOpen unsupported\n"));
			break;

		case WBCTRLA_DuplicateSearchPath:
			/* duplicate search path requested, result should hold the new path nodes. */
			D(bug("WBCTRLA_DuplicateSearchPath found, storage: %p\n", tag->ti_Data));

			*((BPTR *)tag->ti_Data) = L_GetOpusPathList(libbase);
			if (*((BPTR *)tag->ti_Data))
				result = TRUE;
			break;

		case WBCTRLA_FreeSearchPath:
			D(bug("WBCTRLA_FreeSearchPath found, path: %p\n", tag->ti_Data));
			L_FreeDosPathList(tag->ti_Data);
			result = TRUE;
			break;

		case WBCTRLA_GetDefaultStackSize:
			tag->ti_Data = 65535;
			result = TRUE;
			break;

		case WBCTRLA_SetDefaultStackSize:
			D(bug("Setting the stack size is currently not supported\n"));
			break;

		case WBCTRLA_RedrawAppIcon:
			/* wb_workbench_control() has already redrawn Opus's copy */
			break;

		case WBCTRLA_GetProgramList:
			/* if we have a list of running programs, we could support this */
			break;

		case WBCTRLA_FreeProgramList:
			/* as above */
			break;

		case WBCTRLA_AllowLoneIcons:
			/* not sure about this */
			break;

		case WBCTRLA_GetSelectedIconList:
			/* we could support this if we have a list of selected icons */
			break;

		case WBCTRLA_FreeSelectedIconList:
			/* as above */
			break;

		case WBCTRLA_GetAppIconList:
			/* if we have a list of appicons, we could support this */
			break;

		case WBCTRLA_FreeAppIconList:
			/* as above */
			break;

		case WBCTRLA_GetOpenDrawerList:
			/* we could support this if we have a list of open drawers */
			break;

		case WBCTRLA_FreeOpenDrawerList:
			/* as above */
			break;

		case WBCTRLA_AddHiddenDeviceName:
		case WBCTRLA_RemoveHiddenDeviceName:
		case WBCTRLA_GetHiddenDeviceList:
		case WBCTRLA_FreeHiddenDeviceList:
			D(bug("Workbenches hidden devices are not supported\n"));
			break;

		case WBCTRLA_GetTypeRestartTime:
		case WBCTRLA_SetTypeRestartTime:
			D(bug("Setting or getting the TypeRestartTime is unsupported\n"));
			break;

		case WBCTRLA_GetCopyHook:
		case WBCTRLA_SetCopyHook:
			D(bug("The Workbench CopyHook is unsupported\n"));
			break;

		case WBCTRLA_GetDeleteHook:
		case WBCTRLA_SetDeleteHook:
			D(bug("The Workbench DeleteHook is unsupported\n"));
			break;

		case WBCTRLA_GetTextInputHook:
		case WBCTRLA_SetTextInputHook:
			D(bug("The Workbench TextInputHook is unsupported\n"));
			break;

		case WBCTRLA_AddSetupCleanupHook:
		case WBCTRLA_RemSetupCleanupHook:
			D(bug("The Workbench CleanupHook is unsupported\n"));
			break;
		}
	}

	return result;
}
#endif

// Find an AppIcon by the handle our AddAppIconA patch returned (list must be locked)
static AppEntry *wb_find_appicon(IPTR handle, WB_Data *wb_data)
{
	AppEntry *entry;

	for (entry = (AppEntry *)wb_data->app_list.mlh_Head; entry->node.mln_Succ; entry = (AppEntry *)entry->node.mln_Succ)
	{
		if ((IPTR)entry == handle && entry->type == APP_ICON)
			return entry;
	}

	return NULL;
}

// WBCTRLA_RedrawAppIcon names the AppEntry that our AddAppIconA patch handed
// out. Redraw Opus's copy of the icon, and pass the OS its own AppIcon. Local
// AppIcons have no OS AppIcon, and without Workbench only Opus shows icons;
// those redraws are Opus's alone and do not reach the OS.
static BOOL wb_workbench_control(struct MyLibrary *libbase, CONST_STRPTR name, const struct TagItem *tags)
{
	WB_Data *wb_data = &((struct LibData *)libbase->ml_UserData)->wb_data;
	struct TagItem *os_tags = NULL, *tstate, *tag;
	AppEntry *entry;
	BOOL ours = FALSE, wb_running, result = FALSE;

	// Is Workbench running?
	Forbid();
	wb_running = (FindPort("WORKBENCH") != NULL);
	Permit();

	// Lock AppEntry list
	L_GetSemaphore(&wb_data->patch_lock, SEMF_SHARED, 0);

	// Redraw our copies
	tstate = (struct TagItem *)tags;
	while ((tag = NextTagItem(&tstate)))
	{
		if (tag->ti_Tag == WBCTRLA_RedrawAppIcon && (entry = wb_find_appicon(tag->ti_Data, wb_data)))
		{
			L_SendNotifyMsg(DN_APP_ICON_LIST, (IPTR)entry, DNF_ICON_CHANGED, FALSE, 0, 0, libbase);
			ours = TRUE;
		}
	}

	// Swap in the OS AppIcons
	if (ours && (os_tags = CloneTagItems(tags)))
	{
		tstate = os_tags;
		while ((tag = NextTagItem(&tstate)))
		{
			if (tag->ti_Tag == WBCTRLA_RedrawAppIcon && (entry = wb_find_appicon(tag->ti_Data, wb_data)))
			{
				if (entry->os_object && wb_running)
					tag->ti_Data = (IPTR)entry->os_object;
				else
					tag->ti_Tag = TAG_IGNORE;
			}
		}
	}

	// Unlock list
	L_FreeSemaphore(&wb_data->patch_lock);

	// Out of memory for the copy; never pass our AppEntry to the OS
	if (ours && !os_tags)
		return FALSE;

	if (os_tags)
	{
		// Nothing left for the OS: Opus did all the redraws
		tstate = os_tags;
		if (!NextTagItem(&tstate))
		{
			FreeTagItems(os_tags);
			return TRUE;
		}
		tags = os_tags;
	}

#ifdef __amigaos4__
	if (!wb_running)
		result = internalWBCtrl(libbase, name, tags);
	else
#endif
		result = LIBCALL_2(
			BOOL, wb_data->old_function[WB_PATCH_WORKBENCHCONTROLA], WorkbenchBase, IWorkbench, a0, name, a1, tags);

	FreeTagItems(os_tags);
	return result;
}

#ifdef __amigaos4__
//// WorkbenchControl patch
VARARGS68K BOOL L_WB_WorkbenchControl_stubs(struct WorkbenchIFace *IWorkbench, CONST_STRPTR name, ...)
{
	atomic_inc(&usecount[WB_PATCH_WORKBENCHCONTROL]);
	{
		struct MyLibrary *libbase;
		va_list ap;
		struct TagItem *tags;
		BOOL result;

		va_startlinear(ap, name);

		tags = va_getlinearva(ap, struct TagItem *);

		D(bug("called by %s\n", FindTask(NULL)->tc_Node.ln_Name));

		// Open library
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_WORKBENCHCONTROL]);
			return FALSE;
		}

		result = wb_workbench_control(libbase, name, tags);

		atomic_dec(&usecount[WB_PATCH_WORKBENCHCONTROL]);
		return result;
	}
}
#endif

//// WorkbenchControlA patch
PATCHED_2(BOOL, LIBFUNC L_WB_WorkbenchControlA, a0, CONST_STRPTR, name, a1, const struct TagItem *, tags)
{
	atomic_inc(&usecount[WB_PATCH_WORKBENCHCONTROLA]);
	{
		struct MyLibrary *libbase;
		BOOL result;

		// Open library
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_WORKBENCHCONTROLA]);
			return FALSE;
		}

		result = wb_workbench_control(libbase, name, tags);

		atomic_dec(&usecount[WB_PATCH_WORKBENCHCONTROLA]);
		return result;
	}
}
PATCH_END

// Change an AppIcon's image
void LIBFUNC L_ChangeAppIcon(REG(a0, APTR appicon),
							 REG(a1, struct Image *render),
							 REG(a2, struct Image *select),
							 REG(a3, char *title),
							 REG(d0, ULONG flags),
							 REG(a6, struct MyLibrary *libbase))
{
	AppEntry *app_entry;
	WB_Data *wb_data;

#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Get Workbench data pointer
	wb_data = &((struct LibData *)libbase->ml_UserData)->wb_data;

	// Lock AppEntry list
	L_GetSemaphore(&wb_data->patch_lock, SEMF_SHARED, 0);

	// Look for entry in list
	for (app_entry = (AppEntry *)wb_data->app_list.mlh_Head;
		 app_entry != (AppEntry *)appicon && app_entry->node.mln_Succ;
		 app_entry = (AppEntry *)app_entry->node.mln_Succ)
		;

	// Found it?
	if (app_entry == (AppEntry *)appicon)
	{
		struct DiskObject *icon;

		// Get icon pointer
		if ((icon = (struct DiskObject *)app_entry->object))
		{
			ULONG notify_flags = DNF_ICON_CHANGED;

			// Set image and title pointers
			if (flags & CAIF_RENDER)
			{
				icon->do_Gadget.GadgetRender = render;
				notify_flags |= DNF_ICON_IMAGE_CHANGED;
			}
			if (flags & CAIF_SELECT)
			{
				icon->do_Gadget.SelectRender = select;
				notify_flags |= DNF_ICON_IMAGE_CHANGED;
			}
			if (flags & CAIF_TITLE)
				app_entry->text = title;

			// Change locked state
			if (flags & CAIF_LOCKED)
			{
				// Set?
				if (flags & CAIF_SET)
					app_entry->flags |= APPENTF_LOCKED;

				// Clear
				else
					app_entry->flags &= ~APPENTF_LOCKED;
			}

			// Change ghosted state
			if (flags & CAIF_GHOST)
			{
				// Set?
				if (flags & CAIF_SET)
					app_entry->flags |= APPENTF_GHOSTED;

				// Clear
				else
					app_entry->flags &= ~APPENTF_GHOSTED;
			}

			// Change busy state?
			if (flags & CAIF_BUSY)
				app_entry->flags |= APPENTF_BUSY;
			else if (flags & CAIF_UNBUSY)
				app_entry->flags &= ~APPENTF_BUSY;

			// Send notification
			L_SendNotifyMsg(DN_APP_ICON_LIST, (IPTR)app_entry, notify_flags, FALSE, 0, 0, libbase);
		}
	}

	// Unlock list
	L_FreeSemaphore(&wb_data->patch_lock);
}

// Set the state of a toggle appicon menu
long LIBFUNC L_SetAppIconMenuState(REG(a0, APTR appicon), REG(d0, long item), REG(d1, long state))
{
	long res = -1, num;
	struct Node *node;
	AppEntry *app_entry = (AppEntry *)appicon;

	// Go through menus
	for (node = (struct Node *)app_entry->menu.mlh_Head, num = 0; node->ln_Succ && num < item;
		 node = node->ln_Succ, num++)
		;

	// Got item?
	if (node->ln_Succ)
	{
		// Get old value
		res = (node->ln_MenuFlags & MNF_SEL) ? 1 : 0;

		// Set new value
		if (state)
			node->ln_MenuFlags |= MNF_SEL;
		else
			node->ln_MenuFlags &= ~MNF_SEL;
	}

	return res;
}

// Copy a DiskObject
struct DiskObject *LIBFUNC L_CopyDiskObject(REG(a0, struct DiskObject *icon),
											REG(d0, ULONG flags),
											REG(a6, struct MyLibrary *libbase))
{
	DiskObjectCopy *copy;
	long planesize;
	struct LibData *data;

#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Valid icon?
	if (!icon)
		return 0;

	// Get data pointer
	data = (struct LibData *)libbase->ml_UserData;

	//#define IconBase	(data->icon_base)

	if (IconBase->lib_Version >= 44)
	{
		struct DiskObject *icon_copy;

		icon_copy = DupDiskObject(icon, TAG_DONE);

		return icon_copy;
	}

	//#undef IconBase

	// Allocate copy structure
	if (!(copy = L_AllocMemH(data->memory, sizeof(DiskObjectCopy))))
		return 0;

	// Copy DiskObject structure
	CopyMem((char *)icon, (char *)&copy->doc_DiskObject, sizeof(struct DiskObject));

	// Null-out pointers
	copy->doc_DiskObject.do_DefaultTool = 0;
	copy->doc_DiskObject.do_ToolTypes = 0;
	copy->doc_DiskObject.do_DrawerData = 0;
	copy->doc_DiskObject.do_ToolWindow = 0;

	// Copying things?
	if (flags & DOCF_COPYALL)
	{
		// Copy default tool
		if (icon->do_DefaultTool &&
			(copy->doc_DiskObject.do_DefaultTool = L_AllocMemH(data->memory, strlen(icon->do_DefaultTool) + 1)))
			strcpy(copy->doc_DiskObject.do_DefaultTool, icon->do_DefaultTool);

		// Any tooltypes?
		if (icon->do_ToolTypes)
		{
			short count;

			// Count tooltypes
			for (count = 0; icon->do_ToolTypes[count]; count++)
				;

			// Allocate array
			if ((copy->doc_DiskObject.do_ToolTypes = L_AllocMemH(data->memory, (count + 1) << 2)))
			{
				// Copy tooltypes
				for (count = 0; icon->do_ToolTypes[count]; count++)
				{
					// Copy string
					if ((copy->doc_DiskObject.do_ToolTypes[count] =
							 L_AllocMemH(data->memory, strlen(icon->do_ToolTypes[count]) + 1)))
						strcpy(copy->doc_DiskObject.do_ToolTypes[count], icon->do_ToolTypes[count]);
					else
						break;
				}
			}
		}

		// Drawer data?
		if (icon->do_DrawerData &&
			(copy->doc_DiskObject.do_DrawerData = L_AllocMemH(data->memory, sizeof(struct DrawerData))))
			CopyMem((char *)icon->do_DrawerData, (char *)copy->doc_DiskObject.do_DrawerData, sizeof(struct DrawerData));

		// Tool window?
		if (icon->do_ToolWindow &&
			(copy->doc_DiskObject.do_ToolWindow = L_AllocMemH(data->memory, strlen(icon->do_ToolWindow) + 1)))
			strcpy(copy->doc_DiskObject.do_ToolWindow, icon->do_ToolWindow);
	}

	// Allowed to copy images?
	if (!(flags & DOCF_NOIMAGE))
	{
		// Primary image?
		if (icon->do_Gadget.GadgetRender)
		{
			// Set image pointer
			copy->doc_DiskObject.do_Gadget.GadgetRender = &copy->doc_GadgetRender;

			// Copy image structure
			copy->doc_GadgetRender = *((struct Image *)icon->do_Gadget.GadgetRender);

			// Calculate plane size
			planesize =
				UMult32(UMult32(((copy->doc_GadgetRender.Width + 15) >> 3 & 0xfffe), copy->doc_GadgetRender.Height),
						copy->doc_GadgetRender.Depth);

			// Allocate image data and copy
			if ((copy->doc_Image1 = AllocVec(planesize, MEMF_CHIP | MEMF_CLEAR)))
			{
				copy->doc_GadgetRender.ImageData = (UWORD *)copy->doc_Image1;
				CopyMem((char *)(((struct Image *)icon->do_Gadget.GadgetRender)->ImageData),
						(char *)copy->doc_GadgetRender.ImageData,
						planesize);
			}

			// Failed
			else
			{
				L_FreeMemH(copy);
				return 0;
			}
		}

		// Secondary image?
		if (icon->do_Gadget.SelectRender)
		{
			// Set image pointer
			copy->doc_DiskObject.do_Gadget.SelectRender = &copy->doc_SelectRender;

			// Copy image structure
			copy->doc_SelectRender = *((struct Image *)icon->do_Gadget.SelectRender);

			// Calculate plane size
			planesize =
				UMult32(UMult32(((copy->doc_SelectRender.Width + 15) >> 3 & 0xfffe), copy->doc_SelectRender.Height),
						copy->doc_SelectRender.Depth);

			// Allocate image data and copy
			if ((copy->doc_Image2 = AllocVec(planesize, MEMF_CHIP | MEMF_CLEAR)))
			{
				copy->doc_SelectRender.ImageData = (UWORD *)copy->doc_Image2;
				CopyMem((char *)(((struct Image *)icon->do_Gadget.SelectRender)->ImageData),
						(char *)copy->doc_SelectRender.ImageData,
						planesize);
			}

			// Failed
			else
			{
				copy->doc_DiskObject.do_Gadget.SelectRender = 0;
				copy->doc_DiskObject.do_Gadget.Flags &= ~GFLG_GADGHIMAGE;
			}
		}
	}

	// To signify that this is a copy, we point SpecialInfo at ourselves
	copy->doc_DiskObject.do_Gadget.SpecialInfo = (APTR)copy;

	return (struct DiskObject *)copy;
}

// Free a DiskObject copy
void LIBFUNC L_FreeDiskObjectCopy(REG(a0, struct DiskObject *icon), REG(a6, struct MyLibrary *libbase))
{
	short type;

#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Valid icon?
	if (!icon)
		return;

	//#define IconBase	(data->icon_base)
	if (IconBase->lib_Version >= 44)
	{
		FreeDiskObject(icon);
		return;
	}
	//#undef IconBase

	// Is this a copy?
	if ((type = L_GetIconType(icon)) == ICON_CACHED)
	{
		short count;
		DiskObjectCopy *copy = (DiskObjectCopy *)icon;

		// Free images
		FreeVec(copy->doc_Image1);
		FreeVec(copy->doc_Image2);

		// Free strings and drawerdata
		L_FreeMemH(copy->doc_DiskObject.do_DefaultTool);
		L_FreeMemH(copy->doc_DiskObject.do_DrawerData);
		L_FreeMemH(copy->doc_DiskObject.do_ToolWindow);

		// Tooltypes?
		if (copy->doc_DiskObject.do_ToolTypes)
		{
			// Free tooltypes
			for (count = 0; copy->doc_DiskObject.do_ToolTypes[count]; count++)
				L_FreeMemH(copy->doc_DiskObject.do_ToolTypes[count]);
			L_FreeMemH(copy->doc_DiskObject.do_ToolTypes);
		}

		// Free copy structure
		L_FreeMemH(copy);
	}

	// Or is it a NewIcon?
	else if (type == ICON_NEWICON)
	{
#ifdef DEBUG_ICON
		if (((struct NewIconDiskObject *)icon)->nido_Flags & NIDOF_REMAPPED)
			D(bug("error! icon freed while still remapped\n"));
#endif

		//#define NewIconBase	(data->new_icon_base)
		// Free the NewDiskObject part
		FreeNewDiskObject(((struct NewIconDiskObject *)icon)->nido_NewDiskObject);
		//#undef NewIconBase

		// Free our part
		FreeVec(icon);
	}

	// Assume it's a real icon
	else
	{
		//#define IconBase	(data->icon_base)
		FreeDiskObject(icon);
		//#undef IconBase
	}
}

//// AddPort patch
PATCHED_1(void, LIBFUNC L_WB_AddPort, a1, struct MsgPort *, port)
{
	atomic_inc(&usecount[WB_PATCH_ADDPORT]);
	{
		struct MyLibrary *libbase;
		WB_Data *wb_data;

		// Invalid port?
		if (!port)
		{
			atomic_dec(&usecount[WB_PATCH_ADDPORT]);
			return;
		}

		// Get library pointer
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_ADDPORT]);
			return;
		}

		// Get Workbench data pointer
		wb_data = &((struct LibData *)libbase->ml_UserData)->wb_data;

		// ARexx?
		if (port->mp_Node.ln_Name && strcmp(port->mp_Node.ln_Name, "REXX") == 0)
		{
			// Send notification
			L_SendNotifyMsg(DN_REXX_UP, 0, 0, 0, 0, 0, libbase);
		}

		// Pass through
		LIBCALL_1NR(wb_data->old_function[WB_PATCH_ADDPORT], SysBase, IExec, a1, port);

		atomic_dec(&usecount[WB_PATCH_ADDPORT]);
	}
}
PATCH_END

//// CloseWindow patch
PATCHED_1(void, LIBFUNC L_WB_CloseWindow, a0, struct Window *, window)
{
	atomic_inc(&usecount[WB_PATCH_CLOSEWINDOW]);
	{
		struct PubScreenNode *node;
		struct MyLibrary *DOpusBase;
		WB_Data *wb_data;
		struct Task *sigtask = NULL;
		UBYTE sigbit = 0;

		// Invalid window?
		if (!window)
		{
			atomic_dec(&usecount[WB_PATCH_CLOSEWINDOW]);
			return;
		}

		// Get library pointer
		if (!(DOpusBase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_CLOSEWINDOW]);
			return;
		}

		// Get Workbench data pointer
		wb_data = &((struct LibData *)DOpusBase->ml_UserData)->wb_data;

		// See if the screen is public
		if ((node = FindPubScreen(window->WScreen, 0)))
		{
			// Is it the Opus screen?
			if (strncmp(node->psn_Node.ln_Name, "DOPUS.", 6) == 0)
			{
				// Get task to signal
				sigtask = node->psn_SigTask;
				sigbit = node->psn_SigBit;
			}
		}

		// Close window
		LIBCALL_1NR(wb_data->old_function[WB_PATCH_CLOSEWINDOW], IntuitionBase, IIntuition, a0, window);

		// Task to signal?
		if (sigtask)
			Signal(sigtask, 1 << sigbit);

		atomic_dec(&usecount[WB_PATCH_CLOSEWINDOW]);
	}
}
PATCH_END

// Get library base to patch
static APTR wb_get_patchbase(short type, struct LibData *data)
{
	APTR lib = NULL;

	// Look at type
	switch (type)
	{
	// Workbench
	case WB_PATCH_WORKBENCH:
#ifdef __amigaos4__
		lib = (APTR)IWorkbench;
#else
		lib = (APTR)WorkbenchBase;
#endif
		break;

	// DOS patched function
	case WB_PATCH_DOSFUNC:

		// Does flag allow these patches?
		if (data->flags & LIBDF_DOS_PATCH)
		{
#ifdef __amigaos4__
			lib = (APTR)IDOS;
#else
			lib = (APTR)DOSBase;
#endif
		}
		break;

	// DOS
	case WB_PATCH_DOS:
#ifdef __amigaos4__
		lib = (APTR)IDOS;
#else
		lib = (APTR)DOSBase;
#endif
		break;

	// Intuition
	case WB_PATCH_INTUITION:
#ifdef __amigaos4__
		lib = (APTR)IIntuition;
#else
		lib = (APTR)IntuitionBase;
#endif
		break;

	// Graphics
	case WB_PATCH_GFX:
#ifdef __amigaos4__
		lib = (APTR)IGraphics;
#else
		lib = (APTR)GfxBase;
#endif
		break;

	// Exec
	case WB_PATCH_EXEC:
#ifdef __amigaos4__
		lib = (APTR)IExec;
#else
		lib = (APTR)SysBase;
#endif
		break;

	// Icon
	case WB_PATCH_ICON:
#ifdef __amigaos4__
		lib = (APTR)IIcon;
#else
		lib = (APTR)IconBase;
#endif
		break;
	}

	return lib;
}

//// WBInfo patch
PATCHED_3(ULONG, LIBFUNC L_PatchedWBInfo, a0, BPTR, lock, a1, char *, name, a2, struct Screen *, screen)
{
	atomic_inc(&usecount[WB_PATCH_WBINFO]);
	{
		struct MyLibrary *libbase;
		struct DOpusSemaphore *sem;
		IPCData *main_ipc = NULL;
		struct LibData *data;
		char buf[10];
		ULONG res = 0;

		// Get library pointer
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_WBINFO]);
			return 0;
		}

		// Get data pointer
		data = (struct LibData *)libbase->ml_UserData;

		// Get DOpus semaphore?
		if ((sem = (struct DOpusSemaphore *)FindSemaphore("DOpus Public Semaphore")))
			main_ipc = (IPCData *)sem->main_ipc;

		//#define DOSBase	(data->dos_base)
		// Patch WBInfo() ?
		if (main_ipc && GetVar("dopus/PatchWBInfo", buf, 2, GVF_GLOBAL_ONLY) > 0 && buf[0] == '1')
		{
			struct Library *ModuleBase = NULL;
#ifdef __amigaos4__
			struct ModuleIFace *IModule = NULL;

			// Open the icon.module
			if ((ModuleBase = OpenLibrary("DOpus5:Modules/icon.module", LIB_VERSION)) &&
				((IModule = (APTR)GetInterface((struct Library *)ModuleBase, "main", 1L, NULL))))
#else
			if ((ModuleBase = OpenLibrary("DOpus5:Modules/icon.module", LIB_VERSION)))
#endif
			{
				BPTR old;
				struct List files;
				struct Node node;

				// Switch to this directory
				old = CurrentDir(lock);

				// Build fake file list
				NewList(&files);
				AddTail(&files, &node);
				node.ln_Name = name;

				// Call the module
				res = Module_Entry(&files, screen, 0, main_ipc, 0, 0x96604497);

				// Restore current directory
				CurrentDir(old);

				// Close the module
				CloseLibrary(ModuleBase);
#ifdef __amigaos4__
				DropInterface(IModule);
#endif
				atomic_dec(&usecount[WB_PATCH_WBINFO]);
				return res;
			}
			if (ModuleBase)
				CloseLibrary(ModuleBase);
		}
		//#undef DOSBase

		// Call original function
		res = LIBCALL_3(ULONG,
						data->wb_data.old_function[WB_PATCH_WBINFO],
						WorkbenchBase,
						IWorkbench,
						a0,
						lock,
						a1,
						name,
						a2,
						screen);

		atomic_dec(&usecount[WB_PATCH_WBINFO]);
		return res;
	}
}
PATCH_END

//// AddTask patch (for statistics)
PATCHED_3(APTR, LIBFUNC L_PatchedAddTask, a1, struct Task *, task, a2, APTR, initialPC, a3, APTR, finalPC)
{
	atomic_inc(&usecount[WB_PATCH_ADDTASK]);
	{
		struct MyLibrary *libbase;
		struct LibData *data;
		APTR ret;

		// Get library pointer
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_ADDTASK]);
			return NULL;
		}

		// Get data pointer
		data = (struct LibData *)libbase->ml_UserData;

		// Increment task count
		++data->task_count;

		// Call original function
		ret = LIBCALL_3(
			APTR, data->wb_data.old_function[WB_PATCH_ADDTASK], SysBase, IExec, a1, task, a2, initialPC, a3, finalPC);

		atomic_dec(&usecount[WB_PATCH_ADDTASK]);
		return ret;
	}
}
PATCH_END

//// RemTask patch (for statistics)
PATCHED_1(void, LIBFUNC L_PatchedRemTask, a1, struct Task *, task)
{
	atomic_inc(&usecount[WB_PATCH_REMTASK]);
	{
		struct MyLibrary *libbase;
		struct LibData *data;

		// Get library pointer
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_REMTASK]);
			return;
		}

		// Get data pointer
		data = (struct LibData *)libbase->ml_UserData;

		// Decrement task count
		--data->task_count;

		// the usage count has to be decreased before the task kicks the bucket
		atomic_dec(&usecount[WB_PATCH_REMTASK]);

		// Call original function
		LIBCALL_1NR(data->wb_data.old_function[WB_PATCH_REMTASK], SysBase, IExec, a1, task);
	}
}
PATCH_END

//// FindTask patch
PATCHED_1(struct Task *, LIBFUNC L_PatchedFindTask, a1, char *, name)
{
	atomic_inc(&usecount[WB_PATCH_FINDTASK]);
	{
		struct MyLibrary *libbase;
		struct LibData *data;
		struct Task *task;

		// Preserve the native FindTask(NULL) path even when dopus5.library
		// is unavailable during patch installation/removal.
		if (!name)
		{
			if (old_findtask_function)
			{
				task = LIBCALL_1(struct Task *, old_findtask_function, SysBase, IExec, a1, NULL);
				atomic_dec(&usecount[WB_PATCH_FINDTASK]);
				return task;
			}
#ifndef __AROS__
			atomic_dec(&usecount[WB_PATCH_FINDTASK]);
			return ((struct ExecBase *)SysBase)->ThisTask;
#else
			atomic_dec(&usecount[WB_PATCH_FINDTASK]);
			return NULL;
#endif
		}

		// Get library pointer
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_FINDTASK]);
			return NULL;
		}

		// Get data pointer
		data = (struct LibData *)libbase->ml_UserData;

		// Call original function
		if ((task = LIBCALL_1(struct Task *, data->wb_data.old_function[WB_PATCH_FINDTASK], SysBase, IExec, a1, name)))
		{
			atomic_dec(&usecount[WB_PATCH_FINDTASK]);
			return task;
		}

		// Task not found, were they looking for workbench?
		if (strcmp(name, "Workbench") == 0)
		{
			// Return the Launcher task (hopefully all they want is the path list)
			if (data->launcher)
			{
				atomic_dec(&usecount[WB_PATCH_FINDTASK]);
				return (struct Task *)data->launcher->proc;
			}
		}

		// Not found
		atomic_dec(&usecount[WB_PATCH_FINDTASK]);
		return NULL;
	}
}
PATCH_END

//// OpenWindowTags patch
PATCHED_2(struct Window *, LIBFUNC L_PatchedOpenWindowTags, a0, struct NewWindow *, newwin, a1, struct TagItem *, tags)
{
	atomic_inc(&usecount[WB_PATCH_OPENWINDOWTAGS]);
	{
		struct MyLibrary *libbase;
		struct LibData *data;
		WB_Data *wb_data;
		struct Window *ret;

		// Get library pointer
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_OPENWINDOWTAGS]);
			return NULL;
		}

		// Get data pointers
		data = (struct LibData *)libbase->ml_UserData;
		wb_data = &data->wb_data;

		// Is window being opened by the kludge task?
		if (data->open_window_kludge == (struct Process *)FindTask(NULL))

		{
			short x, y, w, h;
			struct Screen *scr;
			struct TagItem *tag;
			BOOL lock = FALSE;

			//#define IntuitionBase	(data->int_base)

			// Get window size
			w = GetTagData(WA_Width, (newwin) ? newwin->Width : 0, tags);
			h = GetTagData(WA_Height, (newwin) ? newwin->Height : 0, tags);

			// Get screen
			if (!(scr = (struct Screen *)GetTagData(WA_CustomScreen, (newwin) ? (IPTR)newwin->Screen : 0, tags)))
			{
				scr = LockPubScreen(0);
				lock = TRUE;
			}

			// Calculate position from mouse pointer
			x = scr->MouseX - (w >> 1);
			y = scr->MouseY - (h >> 1);

			// Unlock screen if locked
			if (lock)
				UnlockPubScreen(0, scr);

			// Set new window position
			if (newwin)
			{
				newwin->LeftEdge = x;
				newwin->TopEdge = y;
			}
			if ((tag = FindTagItem(WA_Left, tags)))
				tag->ti_Data = x;
			if ((tag = FindTagItem(WA_Top, tags)))
				tag->ti_Data = y;

			//#undef IntuitionBase

			// Kludge is only used once
			data->open_window_kludge = 0;
		}

		// Open window
		ret = LIBCALL_2(struct Window *,
						wb_data->old_function[WB_PATCH_OPENWINDOWTAGS],
						IntuitionBase,
						IIntuition,
						a0,
						newwin,
						a1,
						tags);

		atomic_dec(&usecount[WB_PATCH_OPENWINDOWTAGS]);
		return ret;
	}
}
PATCH_END

/*
// Patched AllocBitmap to allocate proper bitmap
struct BitMap *LIBFUNC L_PatchedAllocBitmap(
	REG(d0, ULONG sizex),
	REG(d1, ULONG sizey),
	REG(d2, ULONG depth),
	REG(d3, ULONG flags),
	REG(a0, struct BitMap *friend),
	REG(a6, struct Library *GfxBase))
{
	struct AllocBitmapPatchNode *node;
	struct BitMap *bitmap;
	struct MyLibrary *libbase;
	struct LibData *data;

	// Get library pointer
	if (!(libbase=GET_DOPUSLIB))
		return 0;

	// Get data pointer
	data=(struct LibData *)libbase->ml_UserData;

	// Lock patch list
	ObtainSemaphoreShared((struct SignalSemaphore *)&data->allocbitmap_patch.lock);

	// See if this task is on the list
	for (node=(struct AllocBitmapPatchNode *)data->allocbitmap_patch.list.lh_Head;
		node->abp_Node.mln_Succ;
		node=(struct AllocBitmapPatchNode *)node->abp_Node.mln_Succ)
	{
		// Match the task pointer
		if (node->abp_Task==FindTask(0))
		{
			// No friend supplied?
			if (!friend) //  && flags==0)
			{
				// See if the screen has a custom bitmap
				if (!(GetBitMapAttr(node->abp_Screen->RastPort.BitMap,BMA_FLAGS)&BMF_STANDARD))
				{
					// We now want to allocate a friend bitmap
					flags|=BMF_MINPLANES;
					friend=node->abp_Screen->RastPort.BitMap;
				}
			}
			break;
		}
	}

	// Release the semaphore
	ReleaseSemaphore((struct SignalSemaphore *)&data->allocbitmap_patch.lock);

	// Allocate the bitmap
	bitmap=((struct BitMap *ASM (*)
				(REG(d0, ULONG),
				 REG(d1, ULONG),
				 REG(d2, ULONG),
				 REG(d3, ULONG),
				 REG(a0, struct BitMap *),
				 REG(a6, struct Library *)))data->wb_data.old_function[WB_PATCH_ALLOCBITMAP])
				 (sizex,sizey,depth,flags,friend,GfxBase);

	return bitmap;
}
*/

// Add task to patch list
APTR LIBFUNC L_AddAllocBitmapPatch(REG(a0, struct Task *task),
								   REG(a1, struct Screen *screen),
								   REG(a6, struct MyLibrary *libbase))
{
	/*
		struct LibData *data;
		struct AllocBitmapPatchNode *node;

		// Get data pointer
		data=(struct LibData *)libbase->ml_UserData;

		// Lock patch list
		ObtainSemaphore((struct SignalSemaphore *)&data->allocbitmap_patch.lock);

		// Allocate node
		if (node=AllocVec(sizeof(struct AllocBitmapPatchNode),MEMF_CLEAR))
		{
			// Add to list
			node->abp_Task=task;
			node->abp_Screen=screen;
			AddTail(&data->allocbitmap_patch.list,(struct Node *)node);
		}

		// Release the semaphore
		ReleaseSemaphore((struct SignalSemaphore *)&data->allocbitmap_patch.lock);
		return node;
	*/
	return 0;
}

// Remove task from patch list
void LIBFUNC L_RemAllocBitmapPatch(REG(a0, APTR handle), REG(a6, struct MyLibrary *libbase))
{
	/*
		struct LibData *data;
		struct AllocBitmapPatchNode *node;

		// Get data pointer
		data=(struct LibData *)libbase->ml_UserData;

		// Lock patch list
		ObtainSemaphore((struct SignalSemaphore *)&data->allocbitmap_patch.lock);

		// Find node
		for (node=(struct AllocBitmapPatchNode *)data->allocbitmap_patch.list.lh_Head;
			node->abp_Node.mln_Succ;
			node=(struct AllocBitmapPatchNode *)node->abp_Node.mln_Succ)
		{
			// Match the handle
			if (handle==(APTR)node)
			{
				// Remove this node
				Remove((struct Node *)node);
				FreeVec(node);
				break;
			}
		}

		// Release the semaphore
		ReleaseSemaphore((struct SignalSemaphore *)&data->allocbitmap_patch.lock);
	*/
}

/**********************************************************************
	Install/Remove patches
**********************************************************************/

#if defined(__amigaos4__) || defined(__MORPHOS__)
extern void L_WB_PutDiskObject_stubs();
extern void L_WB_DeleteDiskObject_stubs();
extern void L_PatchedCreateDir_stubs();
extern void L_PatchedDeleteFile_stubs();
extern void L_PatchedSetFileDate_stubs();
extern void L_PatchedSetComment_stubs();
extern void L_PatchedSetProtection_stubs();
extern void L_PatchedRename_stubs();
extern void L_PatchedOpen_stubs();
extern void L_PatchedClose_stubs();
extern void L_PatchedWrite_stubs();
extern void L_PatchedRelabel_stubs();
#endif

//// Patch functions
static PatchList wb_patches[WB_PATCH_COUNT] = {

	// PATCH macro (wb.h) have 4 offsets args:
	// 1. offsets in jump table (for os3 and morphos builds, when SetFunction for patching is used)
	// 2. offsets in interface vectors (for os4 build only, when SetMethod for patching is used)
	// reasons is to have one single pathlist for all builds and to avoid alot of ifdefs
	// 3. our patch-function
	// 4. type of function (to make scan by wb_get_patchbase easy).

	// name of functions to patch for os4 interfaces taken from os4 SDK, in pragmas directory, i.e:
	// pragmas/wb_pragmas.h, pragmas/intuition_pragmas.h
	// pragmas/icon_pragmas.h, pragmas/exec_pragmas.h and pragmas/dos_pragmas.h

	PATCH(-8 * LIB_VECTSIZE, offsetof(struct WorkbenchIFace, AddAppWindowA), L_WB_AddAppWindow, WB_PATCH_WORKBENCH),
	PATCH(-9 * LIB_VECTSIZE,
		  offsetof(struct WorkbenchIFace, RemoveAppWindow),
		  L_WB_RemoveAppWindow,
		  WB_PATCH_WORKBENCH),
	PATCH(-10 * LIB_VECTSIZE, offsetof(struct WorkbenchIFace, AddAppIconA), L_WB_AddAppIcon, WB_PATCH_WORKBENCH),
	PATCH(-11 * LIB_VECTSIZE, offsetof(struct WorkbenchIFace, RemoveAppIcon), L_WB_RemoveAppIcon, WB_PATCH_WORKBENCH),
	PATCH(-12 * LIB_VECTSIZE,
		  offsetof(struct WorkbenchIFace, AddAppMenuItemA),
		  L_WB_AddAppMenuItem,
		  WB_PATCH_WORKBENCH),
	PATCH(-13 * LIB_VECTSIZE,
		  offsetof(struct WorkbenchIFace, RemoveAppMenuItem),
		  L_WB_RemoveAppMenuItem,
		  WB_PATCH_WORKBENCH),
	PATCH(-13 * LIB_VECTSIZE, offsetof(struct IntuitionIFace, CloseWorkBench), L_WB_CloseWorkBench, WB_PATCH_INTUITION),
	PATCH(-35 * LIB_VECTSIZE, offsetof(struct IntuitionIFace, OpenWorkBench), L_WB_OpenWorkBench, WB_PATCH_INTUITION),
#ifdef __amigaos4__
	// On OS4 we patch some more functions in workbench.library:
	//
	// First two functions:  OpenWorkbenchObject(OWO) and OpenWorkbenchObjectA (OWOA), as it uses a lot now for running
	// of programms
	// (like Amidock and co). While it may sounds strange why to path OWO if we patch already OWOA (which is called from
	// OWO), it still
	// proved that after patching by SetMethod() only OWOA call, OWO still didn't point directly on patched OWOA. So we
	// done it 2 times.
	//
	// Third and fourth functions are: WorkbenchControl and WorkbenchControlA (the latter is patched on all platforms,
	// below).
	// We should pacth it as well, as without it "duplicate search patches" thing will not works (which used a lot by
	// the system itself,
	// and for example by CodeBench).
	//
	PATCH(-16 * LIB_VECTSIZE,
		  offsetof(struct WorkbenchIFace, OpenWorkbenchObject),
		  L_WB_OpenWorkbenchObject,
		  WB_PATCH_WORKBENCH),
	PATCH(-16 * LIB_VECTSIZE,
		  offsetof(struct WorkbenchIFace, OpenWorkbenchObjectA),
		  L_WB_OpenWorkbenchObjectA,
		  WB_PATCH_WORKBENCH),
	PATCH(-18 * LIB_VECTSIZE,
		  offsetof(struct WorkbenchIFace, WorkbenchControl),
		  L_WB_WorkbenchControl,
		  WB_PATCH_WORKBENCH),
#endif
	// WorkbenchControlA (V44): WBCTRLA_RedrawAppIcon gets the AppEntry from our AddAppIconA, not the OS AppIcon
	PATCH(-18 * LIB_VECTSIZE,
		  offsetof(struct WorkbenchIFace, WorkbenchControlA),
		  L_WB_WorkbenchControlA,
		  WB_PATCH_WORKBENCH),
	PATCH(-14 * LIB_VECTSIZE, offsetof(struct IconIFace, PutDiskObject), L_WB_PutDiskObject, WB_PATCH_ICON),
	PATCH(-23 * LIB_VECTSIZE, offsetof(struct IconIFace, DeleteDiskObject), L_WB_DeleteDiskObject, WB_PATCH_ICON),
	PATCH(-59 * LIB_VECTSIZE, offsetof(struct ExecIFace, AddPort), L_WB_AddPort, WB_PATCH_EXEC),
	PATCH(-12 * LIB_VECTSIZE, offsetof(struct IntuitionIFace, CloseWindow), L_WB_CloseWindow, WB_PATCH_INTUITION),
	PATCH(-20 * LIB_VECTSIZE, offsetof(struct DOSIFace, CreateDir), L_PatchedCreateDir, WB_PATCH_DOSFUNC),
#if defined(__amigaos4__) && defined(SetDate)  // in case we on new OS4's DOS-SDK, do some ifdef which allow builds src
											   // on all sdk versions. another builds should be not affected at all.
	PATCH(-12 * LIB_VECTSIZE, offsetof(struct DOSIFace, Delete), L_PatchedDeleteFile, WB_PATCH_DOSFUNC),
	PATCH(-66 * LIB_VECTSIZE, offsetof(struct DOSIFace, SetDate), L_PatchedSetFileDate, WB_PATCH_DOSFUNC),
#else
	PATCH(-12 * LIB_VECTSIZE, offsetof(struct DOSIFace, DeleteFile), L_PatchedDeleteFile, WB_PATCH_DOSFUNC),
	PATCH(-66 * LIB_VECTSIZE, offsetof(struct DOSIFace, SetFileDate), L_PatchedSetFileDate, WB_PATCH_DOSFUNC),
#endif
	PATCH(-30 * LIB_VECTSIZE, offsetof(struct DOSIFace, SetComment), L_PatchedSetComment, WB_PATCH_DOSFUNC),
	PATCH(-31 * LIB_VECTSIZE, offsetof(struct DOSIFace, SetProtection), L_PatchedSetProtection, WB_PATCH_DOSFUNC),
	PATCH(-13 * LIB_VECTSIZE, offsetof(struct DOSIFace, Rename), L_PatchedRename, WB_PATCH_DOSFUNC),
	PATCH(-5 * LIB_VECTSIZE, offsetof(struct DOSIFace, Open), L_PatchedOpen, WB_PATCH_DOSFUNC),
	PATCH(-6 * LIB_VECTSIZE, offsetof(struct DOSIFace, Close), L_PatchedClose, WB_PATCH_DOSFUNC),
	PATCH(-8 * LIB_VECTSIZE, offsetof(struct DOSIFace, Write), L_PatchedWrite, WB_PATCH_DOSFUNC),
	PATCH(-120 * LIB_VECTSIZE, offsetof(struct DOSIFace, Relabel), L_PatchedRelabel, WB_PATCH_DOS),
	PATCH(-15 * LIB_VECTSIZE, offsetof(struct WorkbenchIFace, WBInfo), L_PatchedWBInfo, WB_PATCH_WORKBENCH),
	PATCH(-47 * LIB_VECTSIZE, offsetof(struct ExecIFace, AddTask), L_PatchedAddTask, WB_PATCH_EXEC),
	PATCH(-48 * LIB_VECTSIZE, offsetof(struct ExecIFace, RemTask), L_PatchedRemTask, WB_PATCH_EXEC),
	PATCH(-49 * LIB_VECTSIZE, offsetof(struct ExecIFace, FindTask), L_PatchedFindTask, WB_PATCH_EXEC),
	PATCH(-101 * LIB_VECTSIZE,
		  offsetof(struct IntuitionIFace, OpenWindowTagList),
		  L_PatchedOpenWindowTags,
		  WB_PATCH_INTUITION),

};

// Install workbench emulation patches
void LIBFUNC L_WB_Install_Patch(REG(a6, struct MyLibrary *libbase))
{
	WB_Data *wb_data;

#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Get Workbench data pointer
	wb_data = &((struct LibData *)libbase->ml_UserData)->wb_data;

	// Lock patch information
	L_GetSemaphore(&wb_data->patch_lock, SEMF_EXCLUSIVE, 0);

	// Are patches not installed?
	if (wb_data->patch_count == 0)
	{
		short patch;

		// Allocate patch table
		if ((wb_data->old_function = AllocVec(sizeof(APTR) * WB_PATCH_COUNT, MEMF_CLEAR)))
		{
			memset(usecount, 0, sizeof(usecount));

			// Forbid while we install the patches
			Forbid();

			// Install patches
			for (patch = 0; patch < WB_PATCH_COUNT; patch++)
			{
				APTR libptr;

#ifndef __amigaos4__
				// WorkbenchControlA() is V44; older workbench.library has no such vector
				if (patch == WB_PATCH_WORKBENCHCONTROLA && WorkbenchBase->lib_Version < 44)
					continue;
#endif

				// Get library
				if ((libptr = wb_get_patchbase(wb_patches[patch].type, (struct LibData *)libbase->ml_UserData)))
				{
// Patch this function
#if defined(__amigaos4__)
					wb_data->old_function[patch] =
						SetMethod(libptr, wb_patches[patch].offset, wb_patches[patch].function);
#endif
#if defined(__MORPHOS__)
					wb_data->old_function[patch] =
						SetFunction(libptr, wb_patches[patch].offset, (APTR)&wb_patches[patch].trap);
#endif
#if defined(__amigaos3__) || defined(__AROS__)
					wb_data->old_function[patch] =
						SetFunction(libptr, wb_patches[patch].offset, wb_patches[patch].function);
#endif
					if (patch == WB_PATCH_FINDTASK)
						old_findtask_function = wb_data->old_function[patch];
				}
			}

// Clear the cache and Permit
#if !defined(__MORPHOS__)
			CacheClearU();
#endif
			Permit();

			// Increment patch count
			++wb_data->patch_count;
		}
	}

	// Unlock patch information
	L_FreeSemaphore(&wb_data->patch_lock);
}

// Remove workbench patches
BOOL LIBFUNC L_WB_Remove_Patch(REG(a6, struct MyLibrary *libbase))
{
	short patch;
	BOOL fail = FALSE;
	APTR old_patch_val[WB_PATCH_COUNT];
	WB_Data *wb_data;

#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Get Workbench data pointer
	wb_data = &((struct LibData *)libbase->ml_UserData)->wb_data;

	// Lock patch information
	L_GetSemaphore(&wb_data->patch_lock, SEMF_EXCLUSIVE, 0);

	// Really remove patches?
	if (wb_data->patch_count == 1)
	{
#if 1
		// Try to remove patches
		for (patch = 0; patch < WB_PATCH_COUNT; patch++)
		{
			// Patch installed for this function?
			if (wb_data->old_function[patch])
			{
				APTR libptr;

				// Get library
				if ((libptr = wb_get_patchbase(wb_patches[patch].type, (struct LibData *)libbase->ml_UserData)))
				{
					D(bug("waiting for the usecount (%d) to be zero (patch type %d at LVO %ld)\n",
						  usecount[patch],
						  wb_patches[patch].type,
						  (long)wb_patches[patch].offset));
					while (usecount[patch] != 0)
						Delay(1);
					Forbid();
					D(bug("ok, trying to remove the patch\n"));

					// Restore old value
					old_patch_val[patch] =
	#ifdef __amigaos4__
						SetMethod(libptr, wb_patches[patch].offset, wb_data->old_function[patch]);
	#else
						SetFunction(libptr, wb_patches[patch].offset, wb_data->old_function[patch]);
	#endif

	// The value returned should have been our patch
	#if defined(__MORPHOS__)
					if ((ULONG)old_patch_val[patch] != (ULONG)&wb_patches[patch].trap)
	#else
					if (old_patch_val[patch] != wb_patches[patch].function)
	#endif
					{
						// There's no good way to handle this without waiting for an indefinite amount of time
						D(bug("something went wrong, the function have been repatched by someone else\n"));
					}

					CacheClearU();
					Permit();
				}
			}
		}
		FreeVec(wb_data->old_function);
		wb_data->old_function = 0;
		old_findtask_function = NULL;
#else
		// Try to remove patches
		Forbid();
		for (patch = 0; patch < WB_PATCH_COUNT; patch++)
		{
			// Patch installed for this function?
			if (wb_data->old_function[patch])
			{
				APTR libptr;

				// Get library
				if ((libptr = wb_get_patchbase(wb_patches[patch].type, (struct LibData *)libbase->ml_UserData)))
				{
					// Restore old value
					old_patch_val[patch] =
	#ifdef __amigaos4__
						SetMethod(libptr, wb_patches[patch].offset, wb_data->old_function[patch]);
	#else
						SetFunction(libptr, wb_patches[patch].offset, wb_data->old_function[patch]);
	#endif

	// The value returned should have been our patch
	#if defined(__MORPHOS__)
					if ((ULONG)old_patch_val[patch] != (ULONG)&wb_patches[patch].trap)
	#else
					if (old_patch_val[patch] != wb_patches[patch].function)
	#endif
					{
						// Something's in there
						fail = TRUE;
						break;
					}
				}
			}
		}

		// Did remove fail?
		if (fail)
		{
			// Restore the vectors that we've changed
			for (; patch >= 0; patch--)
			{
				// Patch installed for this function?
				if (wb_data->old_function[patch])
				{
					APTR libptr;

					// Get library
					if ((libptr = wb_get_patchbase(wb_patches[patch].type, (struct LibData *)libbase->ml_UserData)))
					{
	// Reinstall patch
	#ifdef __amigaos4__
						SetMethod(libptr, wb_patches[patch].offset, wb_data->old_function[patch]);
	#else
						SetFunction(libptr, wb_patches[patch].offset, wb_data->old_function[patch]);
	#endif
					}
				}
			}
		}

		// Otherwise, clear our patch pointers
		else
		{
			FreeVec(wb_data->old_function);
			wb_data->old_function = 0;
			old_findtask_function = NULL;
		}

		CacheClearU();
		Permit();
#endif
	}

	// If we didn't fail, decrement patch count
	if (!fail && wb_data->patch_count > 0)
		--wb_data->patch_count;

	// Unlock patch information
	L_FreeSemaphore(&wb_data->patch_lock);

	return (BOOL)(!fail);
}
