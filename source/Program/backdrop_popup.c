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

#define BPF_SNAPSHOT (1 << 0)
#define BPF_CLOSE (1 << 1)
#define BPF_TRASH (1 << 2)
#define BPF_CANINFO (1 << 3)
#define BPF_DISK (1 << 4)
#define BPF_FTYPE (1 << 5)
#define BPF_FTLOCKED (1 << 6)
#define BPF_OLOCKED (1 << 7)
#define BPF_WRITEPROT (1 << 8)
#define BPF_NOFORMAT (1 << 9)
#define BPF_DIRECTORY (1 << 10)
#define BPF_NOEJECT (1 << 11)

BOOL backdrop_test_rmb(BackdropInfo *info, struct IntuiMessage *msg, struct IntuiMessage *msg_copy, ULONG mode)
{
	short x, y;
	BOOL ret = 0;

	// If not right button or window is inactive, ignore
	if (!(msg->Qualifier & IEQUALIFIER_RBUTTON) || !(info->window->Flags & WFLG_WINDOWACTIVE))
		return 0;

	// Lock screen layers
	LockLayerInfo(&info->window->WScreen->LayerInfo);

	// Convert to screen coordinates
	x = msg->MouseX + info->window->LeftEdge;
	y = msg->MouseY + info->window->TopEdge;

	// See if our window is under the mouse
	if (WhichLayer(&info->window->WScreen->LayerInfo, x, y) == info->window->WLayer)
	{
		// Lock backdrop list
		lock_listlock(&info->objects, 0);

		// Check if we can swallow this
		if (!mode ||
			(mode & BTRM_ICON && backdrop_get_object(info, msg->MouseX, msg->MouseY, 0)) ||
			(mode & BTRM_NOBORDER && !cx_mouse_outside(info->window, msg->MouseX, msg->MouseY)))
		{
			// Cancel menu event
			msg->Code = MENUCANCEL;

			// Change copy to MOUSEBUTTONS
			msg_copy->Class = IDCMP_MOUSEBUTTONS;
			msg_copy->Code = MENUDOWN;

			// Kludge for MagicMenu
			if (msg_copy->Seconds == 0)
				CurrentTime(&msg_copy->Seconds, &msg_copy->Micros);

			ret = 1;
		}

		// Lock backdrop list
		unlock_listlock(&info->objects);
	}

	// Unlock layers
	UnlockLayerInfo(&info->window->WScreen->LayerInfo);

	return ret;
}

BOOL backdrop_popup(BackdropInfo *info, short x, short y, UWORD qual, long bpflags, char *filename, DirEntry *entry)
{
	BackdropObject *object = 0;
	PopUpHandle *menu;
	PopUpItem *item;
	unsigned short res;
	ULONG flags = 0;
	D_S(struct InfoData, infodata)
	long diskinfo = 0;
	short extnum = 0;
	MatchHandle *handle = 0;

	// Clear info
	infodata->id_DiskState = 0;
	infodata->id_InUse = 0;
	infodata->id_DiskType = 0;

	// Entry in a lister?
	if (info->lister && (entry || filename))
	{
		// Popups disabled?
		if (info->lister->cur_buffer->cust_flags & CUSTF_NOPOPUPS)
		{
			// Got an entry?
			if (entry)
			{
				// If there's custom menus, we'll show them
				if (!GetTagData(DE_PopupMenu, 0, entry->de_Tags))
				{
					// No
					return 0;
				}
			}
			else
				return 0;
		}
	}

	// Need to look for icon?
	if (bpflags & BPOPF_ICONS)
	{
		// Lock backdrop list
		lock_listlock(&info->objects, 0);
		flags |= BPF_OLOCKED;

		// Not given a filename?
		if (!filename)
		{
			// Look for icon
			object = backdrop_get_object(info, x, y, 0);

			// If this is the main window, we must have an icon
			if (!object && info->flags & BDIF_MAIN_DESKTOP)
			{
				// Fail
				unlock_listlock(&info->objects);
				return 0;
			}
		}

		// Otherwise
		else
		{
			// Is it a disk?
			if (!entry && filename[strlen(filename) - 1] == ':')
			{
				// Get disk information
				diskinfo = GetDiskInfo(filename, infodata);
				flags |= BPF_DISK;
			}
		}
	}

	// No object?
	if (!object)
	{
		// Don't do popup for groups
		if (info->flags & BDIF_GROUP)
		{
			// Unlock and return
			if (flags & BPF_OLOCKED)
				unlock_listlock(&info->objects);
			return 0;
		}
	}

	// AppIcon?
	else if (object->type == BDO_APP_ICON)
	{
		// Is icon busy?
		if (object->flags & BDOF_BUSY)
		{
			// No popups if busy
			if (flags & BPF_OLOCKED)
				unlock_listlock(&info->objects);
			return 0;
		}

		// Does it support snapshot?
		if (((WB_AppIconFlags((struct AppIcon *)object->misc_data)) & APPENTF_SNAPSHOT) || *object->name)
			flags |= BPF_SNAPSHOT;

		// Does it support information?
		if ((WB_AppIconFlags((struct AppIcon *)object->misc_data)) & APPENTF_INFO)
			flags |= BPF_CANINFO;

		// Close item?
		if (((AppEntry *)object->misc_data)->flags & APPENTF_CLOSE)
			flags |= BPF_CLOSE;
	}

	// Disk?
	else if (object->type == BDO_DISK || object->type == BDO_BAD_DISK)
	{
		// Good disk?
		if (object->type == BDO_DISK)
			flags |= BPF_DISK;

		// Was icon selected, and is shift held down?
		if (object->state && (qual & IEQUAL_ANYSHIFT))
		{
			// Add to hidden list
			if (desktop_add_hidden(object->device_name))
			{
				// Refresh drives
				send_main_reset_cmd(CONFIG_CHANGE_HIDDEN_DRIVES, 0, 0);

				// Unlock lists and return
				unlock_listlock(&info->objects);
				return 1;
			}
		}

		// Get disk information
		if (!(object->flags & (BDOF_ASSIGN | BDOF_CACHE)))
		{
			diskinfo = GetDiskInfo(object->device_name, infodata);

			// Can it be ejected?
			if (!disk_eject_available(object->device_name))
				flags |= BPF_NOEJECT;
		}
	}

	// Trashcan?
	else if (object->icon->do_Type == WBGARBAGE)
		flags |= BPF_TRASH;

	// Got disk info?
	if (diskinfo)
	{
		// Write protected?
		if (infodata->id_DiskState == ID_WRITE_PROTECTED)
			flags |= BPF_WRITEPROT;

		// Validating, and not a bad disk?
		else if (infodata->id_DiskState == ID_VALIDATING && infodata->id_DiskType != ID_UNREADABLE_DISK)
			flags |= BPF_WRITEPROT;

		// Can't format?
		if (infodata->id_InUse)
			flags |= BPF_NOFORMAT;
	}

	// Allocate menu handle
	if (!(menu = PopUpNewHandle((IPTR)info->lister, info->callback, &locale)))
	{
		// Unlock if needed
		if (flags & BPF_OLOCKED)
			unlock_listlock(&info->objects);
		return 0;
	}

	// No object?
	if (!object && !filename)
	{
		// Build default menu
		popup_default_menu(info, menu, &extnum);
	}

	// Got an entry with custom menus?
	else if (entry)
	{
		PopUpMenu *user;
		long num;

		// Get menu list pointer
		if ((user = (PopUpMenu *)GetTagData(DE_PopupMenu, 0, entry->de_Tags)))
		{
			struct Node *node;

			// Get base ID
			num = user->userdata;

			// Go through list
			for (node = (struct Node *)user->item_list.mlh_Head; node->ln_Succ; node = node->ln_Succ, num++)
			{
				// Want a separator?
				if (strcmp(node->ln_Name, "---") == 0)
				{
					// Add separator
					PopUpSeparator(menu);
				}

				// Normal item
				else if ((item = PopUpNewItem(menu, (IPTR)node->ln_Name, MENU_CUSTOM + num, POPUPF_STRING)))
				{
					// Toggle?
					if (node->ln_MenuFlags & MNF_TOGGLE)
					{
						// Set flag
						item->flags |= POPUPF_CHECKIT;

						// Selected?
						if (node->ln_MenuFlags & MNF_SEL)
							item->flags |= POPUPF_CHECKED;
					}
				}
			}
		}
	}

	// Got an object
	else
	{
		// Build icon menu
		handle = popup_build_icon_menu(info, filename, object, flags, &extnum, menu);
	}

	// Filetype menu?
	if (handle ||
		(object && (object->type == BDO_LEFT_OUT || object->type == BDO_DISK || object->type == BDO_BAD_DISK)))
	{
		// Get popups for this file
		if (popup_get_filetype(menu, info, handle, object, filename, extnum, &flags))
			flags |= BPF_FTYPE;
		flags |= BPF_FTLOCKED;
	}

	// AppIcon?
	else if (object && object->type == BDO_APP_ICON)
	{
		// Get AppIcon menus
		popup_get_appicon(menu, (AppEntry *)object->misc_data);
	}

	// Check for and remove trailing separators
	for (item = (PopUpItem *)menu->ph_Menu.item_list.mlh_TailPred; item->node.mln_Pred;)
	{
		PopUpItem *next = (PopUpItem *)item->node.mln_Pred;

		// Not a separator?
		if (item->item_name != POPUP_BARLABEL)
			break;

		// Remove it and free it
		Remove((struct Node *)item);
		FreeMemH(item);

		// Get next
		item = next;
	}

	// Do pop-up menu
	if ((res = DoPopUpMenu(info->window, &menu->ph_Menu, &item, MENUDOWN)) != (UWORD)-1)
	{
		// Set busy pointer
		SetBusyPointer(info->window);

		// Help?
		if (res & POPUP_HELPFLAG)
		{
			// Get help id
			res &= ~POPUP_HELPFLAG;

			// PopUpExtension?
			if (res >= MENU_EXTENSION)
			{
			}

			// Custom item for AppIcon?
			if (res >= MENU_CUSTOM && object && object->type == BDO_APP_ICON)
			{
				unsigned short flags;

				// Get item id
				flags = res - MENU_CUSTOM;
				flags |= BAPPF_MENU | BAPPF_HELP;

				// Send help message
				backdrop_appicon_message(object, flags);
			}

			// Show help
			else
			{
				if (res > MENU_CUSTOM)
					res = MENU_CUSTOM;
				help_menu_help((long)res, 0);
			}
		}

		// PopUpExtension?
		else if (res >= MENU_EXTENSION)
		{
			// Got valid pointer?
			if (item && item->data)
			{
				// Call popup function
				popup_run_func(item->data, object, filename, info->lister);
			}
		}

		// Custom function?
		else if (res >= MENU_CUSTOM)
		{
			// Got filetype menu?
			if (flags & BPF_FTYPE && item && item->data)
			{
				BPTR lock = 0;
				char *devptr = 0;

				// Get object lock
				if (object)
				{
					// Pass device name if it's a disk
					if (object->type == BDO_DISK || object->type == BDO_BAD_DISK)
						devptr = object->device_name;

					// Else get a lock on the object's parent
					else
						lock = backdrop_icon_lock(object);
				}
				else if (filename)
				{
					BPTR temp;

					if ((temp = Lock(filename, ACCESS_READ)))
						lock = ParentDir(temp);
					UnLock(temp);
				}

				// Got lock?
				if (lock || devptr)
				{
					struct ArgArray *array;

					// Get pathname
					if (lock)
						DevNameFromLockDopus(lock, info->buffer, 256);

					// Build argument array
					if ((array = NewArgArray()))
					{
						struct ArgArrayEntry *entry;

						// Get entry
						if ((entry = NewArgArrayEntry(
								 array, (devptr) ? devptr : ((filename) ? (char *)FilePart(filename) : object->name))))
						{
							// Directory?
							if (flags & BPF_DIRECTORY)
								entry->ae_Flags |= AEF_DIR;
							else if (object && object->icon->do_Type == WBDRAWER)
								entry->ae_Flags |= AEF_DIR;
						}
					}

					// Launch function
					function_launch(FUNCTION_RUN_FUNCTION_EXTERNAL,
									(Cfg_Function *)item->data,
									0,
									0,
									info->lister,
									0,
									info->buffer,
									0,
									array,
									0,
									0);

					// Free lock
					if (lock)
						UnLock(lock);
				}
			}

			// AppIcon?
			else if (object && object->type == BDO_APP_ICON)
			{
				// Send message
				backdrop_appicon_message(object, BAPPF_MENU | (res - MENU_CUSTOM));
			}

			// Got an entry?
			else if (entry)
			{
				// Send back code; this is kludgy, but it works
				filename[0] = 0;
				filename[1] = 1;
				*((UWORD *)(filename + 2)) = (UWORD)(res - MENU_CUSTOM);
			}
		}

		// Got filename?
		else if (filename)
		{
			// Send back code; this is kludgy, but it works
			filename[0] = 0;
			filename[1] = 0;
			*((UWORD *)(filename + 2)) = res;

			// Copy to?
			if (res == MENU_ICON_COPY_TO && item)
			{
				// Get path
				if (backdrop_get_copy_path(item, filename + 4))
				{
					// Copy to script
					*((UWORD *)(filename + 2)) = MENU_ICON_COPY_TO_SCRIPT;
				}
			}

			// Copy to desktop
			else if (res == MENU_ICON_COPY_DESKTOP)
			{
				// Get desktop path
				strcpy(filename + 4, environment->env->desktop_location);
			}

			// Copy to other
			else
				strcpy(filename + 4, "::");
		}

		// Open with something done before
		else if (res >= MENU_OPEN_WITH_BASE && res <= MENU_OPEN_WITH_MAX)
		{
			iconopen_packet *packet;

			// Allocate packet
			if ((packet = get_icon_packet(info, object, 0, 0)))
			{
				// Set flags
				packet->flags = (1 << 30) | (res - MENU_OPEN_WITH_BASE);

				// Start process
				if (!(misc_startup("dopus_icon_open", MENU_OPEN_WITH, info->window, packet, 1)))
					FreeVec(packet);
			}
		}

		// Otherwise
		else
			switch (res)
			{
			// Open
			case MENU_ICON_OPEN:
			case MENU_OPEN_NEW_WINDOW:
			case MENU_OPEN_WITH: {
				iconopen_packet *packet;

				// Allocate packet
				if ((packet = get_icon_packet(info, object, 0, (res == MENU_ICON_OPEN) ? 0 : IEQUALIFIER_LSHIFT)))
				{
					// Start process
					if (!(misc_startup("dopus_icon_open",
									   (res == MENU_OPEN_WITH) ? MENU_OPEN_WITH : MENU_ICON_OPEN,
									   info->window,
									   packet,
									   1)))
						FreeVec(packet);
				}
			}
			break;

			// Close (AppIcons only)
			case MENU_ICON_CLOSE:

				// Send close message
				backdrop_appicon_message(object, BAPPF_CLOSE);
				break;

			// Information
			case MENU_ICON_INFO:

				// Show information
				backdrop_info(info, 0, object);
				break;

			// Snapshot
			case MENU_ICON_SNAPSHOT_ICON:
			case MENU_ICON_UNSNAPSHOT:

				// Snapshot icons
				backdrop_snapshot(info, (res == MENU_ICON_UNSNAPSHOT), FALSE, object);
				break;

			// Rename
			case MENU_ICON_RENAME: {
				iconopen_packet *packet;

				// Get packet
				if ((packet = get_icon_packet(info, object, 0, 0)))
				{
					// Start process
					if (!(misc_startup("dopus_icon_rename", MENU_ICON_RENAME, info->window, packet, 1)))
						FreeVec(packet);
				}
			}
			break;

			// Format
			case MENU_ICON_FORMAT:
				backdrop_format(info, object);
				break;

			// DiskInfo
			case MENU_ICON_DISKINFO:
				backdrop_info(info, 1, object);
				break;

			// Eject
			case MENU_ICON_EJECT:
				backdrop_eject(info, object);
				break;

			// Delete
			case MENU_ICON_DELETE:
			case MENU_EMPTY_TRASH:
			case MENU_ICON_PUT_AWAY: {
				iconopen_packet *packet;

				// Get packet
				if ((packet = get_icon_packet(info, object, 0, 0)))
				{
					// Start process
					if (!(misc_startup("dopus_icon_delete", res, info->window, packet, 1)))
						FreeVec(packet);
				}
			}
			break;

			// Copy a file
			case MENU_ICON_COPY_TO:
			case MENU_ICON_COPY_OTHER:
			case MENU_ICON_COPY_DESKTOP: {
				iconopen_packet *packet;
				char *path = 0, buffer[256];
				short type = 0;

				// Copy to?
				if (res == MENU_ICON_COPY_TO && item)
				{
					// Get path
					type = backdrop_get_copy_path(item, buffer);
					path = buffer;
				}

				// Copy to desktop?
				else if (res == MENU_ICON_COPY_DESKTOP)
				{
					// Get path
					path = environment->env->desktop_location;
				}

				// Run a rexx command?
				if (type)
				{
					char rexxcmd[512], ipath[256];

					// Get icon path
					if (desktop_icon_path(object, ipath, 256, 0))
					{
						// Get rexx function to run
						lsprintf(rexxcmd,
								 "%s \"%s\" %s %lu",
								 path,
								 ipath,
								 GUI->rexx_port_name,
								 (unsigned long)(IPTR)info->lister);

						// Run rexx thing
						rexx_send_command(rexxcmd, FALSE);
					}
				}

				// Get packet
				else if ((packet = get_icon_packet(info, object, path, 0)))
				{
					// Update desktop?
					if (res == MENU_ICON_COPY_DESKTOP)
					{
						// Set flag to update desktop
						packet->flags = FUNCF_RESCAN_DESKTOP;
					}

					// Start process
					if (!(misc_startup("dopus_icon_copy", MENU_ICON_COPY, info->window, packet, 1)))
						FreeVec(packet);
				}
			}
			break;

			// Leave out
			case MENU_ICON_LEAVE_OUT:
			case MENU_ICON_SHORTCUT: {
				iconopen_packet *packet;

				// Get packet
				if ((packet = get_icon_packet(info, object, 0, 0)))
				{
					// Start process
					if (!(misc_startup("dopus_icon_leaveout", res, info->window, packet, 1)))
						FreeVec(packet);
				}
			}
			break;

			// Copy a disk
			case MENU_ICON_COPY:

				// This only works on disks
				if (object->type == BDO_DISK)
				{
					// Launch diskcopy
					function_launch(FUNCTION_RUN_FUNCTION,
									def_function_diskcopy,
									0,
									0,
									0,
									0,
									0,
									0,
									BuildArgArray(object->device_name, 0),
									0,
									0);
				}
				break;

			// Lister functions
			case MENU_LISTER_VIEW_NAME:
			case MENU_LISTER_VIEW_ICON:
			case MENU_LISTER_ICON_ACTION:
			case MENU_LISTER_SHOW_ALL:
			case MENU_LISTER_ARRANGE_NAME:
			case MENU_LISTER_ARRANGE_TYPE:
			case MENU_LISTER_ARRANGE_SIZE:
			case MENU_LISTER_ARRANGE_DATE:
			case MENU_ICON_MAKEDIR:
			case MENU_ICON_MAKEFILE:
			case MENU_LISTER_ICONIFY:
			case MENU_LISTER_CLOSE:
			case MENU_LISTER_RESTORE:
			case MENU_LISTER_SNAPSHOT:
			case MENU_ICON_CLEANUP:
			case MENU_LISTER_RESIZE_FIT:
			case MENU_ICON_SNAPSHOT_ALL:

				// Do function
				backdrop_command(info, res, 0);
				break;
			}

		// Clear busy pointer
		ClearPointer(info->window);
	}

	// Free data
	PopUpFreeHandle(menu);

	// Unlock lists
	if (flags & BPF_OLOCKED)
		unlock_listlock(&info->objects);
	if (flags & BPF_FTLOCKED)
		unlock_listlock(&GUI->filetypes);

	return 1;
}

// Send a command for a backdrop window
void backdrop_command(BackdropInfo *info, ULONG cmd, ULONG flags)
{
	IPC_Command(info->ipc, LISTER_DO_FUNCTION, flags, (APTR)(IPTR)cmd, 0, 0);
}

// Get path for 'Copy To'
short backdrop_get_copy_path(PopUpItem *item, char *path)
{
	BPTR lock;
	short ret = 0;

	// Does item have data?
	if (item->data)
	{
		// Script?
		if (*((char *)item->data) == 0)
		{
			// Get real name
			strcpy(path, ((char *)item->data) + 1);
			ret = 1;
		}

		// Get path
		else
			strcpy(path, (char *)item->data);
	}

	// Otherwise
	else
	{
		char *ptr, *to_str;
		short len;

		// Get pointer to string
		ptr = (char *)item->item_name;

		// Get 'to' string
		to_str = GetString(&locale, MSG_ICON_COPY_TO);
		len = strlen(to_str);

		// Does string start with it?
		if (strnicmp(ptr, to_str, len) == 0)
		{
			// Bump the length of the 'to' string
			ptr += len;
		}

		// Skip until we find a space
		while (*ptr && *ptr != ' ')
			++ptr;

		// Skip spaces
		rexx_skip_space(&ptr);

		// Send string back
		strcpy(path, ptr);
	}

	// Normal path?
	if (!ret)
	{
		// Try to lock path
		if ((lock = Lock(path, ACCESS_READ)))
		{
			// Get device path
			DevNameFromLockDopus(lock, path, 256);
			UnLock(lock);
		}

		// Make sure path is terminated
		AddPart(path, "", 256);
	}

	return ret;
}

// Get PopUp menus for an AppIcon
void popup_get_appicon(PopUpHandle *menu, AppEntry *app)
{
	struct Node *node;
	long num;

	// Valid entry?
	if (!app)
		return;

	// Reset separator flag
	menu->ph_Flags &= ~POPHF_SEP;

	// Get menu base
	num = app->menu_id_base;

	// Go through icon menu items
	for (node = (struct Node *)app->menu.mlh_Head; node->ln_Succ; node = node->ln_Succ)
	{
		// Want a separator?
		if (strcmp(node->ln_Name, "---") == 0)
		{
			// Add separator
			PopUpSeparator(menu);
			++num;
		}

		// Normal item
		else
		{
			PopUpItem *item;

			// Add a separator if needed
			if (!(menu->ph_Flags & POPHF_SEP))
				PopUpSeparator(menu);

			// Add item
			if ((item = PopUpNewItem(menu, (IPTR)node->ln_Name, MENU_CUSTOM + num, POPUPF_STRING)))
			{
				// Toggle?
				if (node->ln_MenuFlags & MNF_TOGGLE)
				{
					// Set flag
					item->flags |= POPUPF_CHECKIT;

					// Selected?
					if (node->ln_MenuFlags & MNF_SEL)
						item->flags |= POPUPF_CHECKED;
				}
				++num;
			}
		}
	}
}

// Get PopUp menus for a file
BOOL popup_get_filetype(PopUpHandle *menu,
						BackdropInfo *info,
						MatchHandle *handle,
						BackdropObject *object,
						char *filename,
						short extnum,
						ULONG *flags)
{
	Cfg_FiletypeList *list;
	Cfg_Filetype *type;
	BPTR lock, old = 0;
	char *name;
	BOOL ftype = 0;
	PopUpItem *item;
	short count = 0;

	// Lock filetype list
	lock_listlock(&GUI->filetypes, FALSE);

	// Got icon?
	if (!(name = filename))
	{
		// Disk?
		if (object->type == BDO_DISK || object->type == BDO_BAD_DISK)
		{
			// Use device name
			name = object->device_name;
		}

		// Get icon lock
		else if ((lock = backdrop_icon_lock(object)))
		{
			// Change current directory
			old = CurrentDir(lock);

			// Get name pointer
			name = object->name;
		}
	}

	// Get match handle for file
	if (handle || (handle = GetMatchHandle(name)))
	{
		short num = 0;

		// Directory?
		if (handle->fib.fib_DirEntryType > 0)
			*flags |= BPF_DIRECTORY;

		// Clear 'separator' flag
		menu->ph_Flags &= ~POPHF_SEP;

		// Set busy pointer for this
		SetBusyPointer(info->window);

		// Go through filetype lists
		for (list = (Cfg_FiletypeList *)GUI->filetypes.list.lh_Head; list->node.ln_Succ;
			 list = (Cfg_FiletypeList *)list->node.ln_Succ)
		{
			// Go through filetypes in this list
			for (type = (Cfg_Filetype *)list->filetype_list.lh_Head; type->node.ln_Succ;
				 type = (Cfg_Filetype *)type->node.ln_Succ)
			{
				// Does filetype have menus?
				if ((IsListEmpty(&type->function_list)) && !(popup_ext_check(type->type.name)))
					continue;

				// Try to match
				if (MatchFiletype(handle, type))
				{
					Cfg_Function *func;
					PopUpExt *ext;

					// Set flag
					ftype = 1;

					// Lock extension list
					lock_listlock(&GUI->popupext_list, FALSE);

					// Go through list
					for (ext = (PopUpExt *)GUI->popupext_list.list.lh_Head; ext->pe_Node.ln_Succ;
						 ext = (PopUpExt *)ext->pe_Node.ln_Succ)
					{
						// Match filetype
						if (stricmp(ext->pe_FileType, type->type.name) == 0)
						{
							BOOL sep = 0;

							// Add a separator if needed
							if (!(menu->ph_Flags & POPHF_SEP))
							{
								PopUpSeparator(menu);
								sep = 1;
							}

							// Want separator?
							if (strcmp(ext->pe_Menu, "---") == 0)
							{
								// Add separator if we haven't just added one
								if (!sep)
									PopUpSeparator(menu);
							}

							// Allocate normal item
							else if ((item = PopUpNewItem(
										  menu, (IPTR)ext->pe_Menu, MENU_EXTENSION + extnum, POPUPF_STRING)))
							{
								// Set data pointer
								item->data = ext;
								++extnum;
							}
						}
					}

					// Unlock extension list
					unlock_listlock(&GUI->popupext_list);

					// If this is a -124 priority, only do filetypes menu if no filetypes matched so far
					if (type->node.ln_Pri == -124 && count > 0)
						continue;

					// Go through functions
					for (func = (Cfg_Function *)type->function_list.lh_Head; func->node.ln_Succ;
						 func = (Cfg_Function *)func->node.ln_Succ)
					{
						// Label function?
						if (func->function.flags2 & FUNCF2_LABEL_FUNC)
						{
							Cfg_Instruction *ins;

							// Go through instructions
							for (ins = (Cfg_Instruction *)func->instructions.mlh_Head; ins->node.mln_Succ;
								 ins = (Cfg_Instruction *)ins->node.mln_Succ)
							{
								// Label?
								if (ins->type == INST_LABEL)
								{
									BOOL sep = 0;

									// Add a separator if needed
									if (!(menu->ph_Flags & POPHF_SEP))
									{
										PopUpSeparator(menu);
										sep = 1;
									}

									// Want separator?
									if (strcmp(ins->string, "---") == 0)
									{
										// Add separator if we haven't just added one
										if (!sep)
											PopUpSeparator(menu);
									}

									// Allocate normal item
									else if ((item = PopUpNewItem(
												  menu, (IPTR)ins->string, MENU_CUSTOM + num, POPUPF_STRING)))
									{
										// Set data pointer
										item->data = func;
										++num;
										++count;
									}
									break;
								}
							}
						}
					}
				}
			}
		}

		// Clear busy pointer
		ClearPointer(info->window);

		// Free match handle
		FreeMatchHandle(handle);
	}

	// Restore directory
	if (old)
		UnLock(CurrentDir(old));

	return ftype;
}

// Build 'CopyTo' menu
void popup_build_copyto(PopUpHandle *menu, PopUpItem *item)
{
	BPTR lock;
	BOOL ok = 1;
	struct List *list;

	// Create a list for submenus
	if (!(item->data = AllocMemH(menu->ph_Memory, sizeof(struct MinList))))
		return;

	// Get handle list pointer
	list = (struct List *)item->data;

	// Initialise list
	NewList(list);

	// Get list pointer
	menu->ph_List = list;

	// Try and lock CopyTo directory
	if ((lock = Lock("DOpus5:System/CopyTo", ACCESS_READ)))
	{
		D_S(struct FileInfoBlock, fib)

		// Examine it
		Examine(lock, fib);

		// Directory?
		if (fib->fib_DirEntryType > 0)
		{
			char *to_str;
			short len;

			// Set flag so we don't use default entries
			ok = 0;

			// Get pointer to 'to' string
			to_str = GetString(&locale, MSG_ICON_COPY_TO);
			len = strlen(to_str);

			// Scan directory
			while (ExNext(lock, fib))
			{
				char *name;
				PopUpItem *new;

				// Is this a script?
				if (fib->fib_Protection & FIBF_SCRIPT)
				{
					char *nameptr;

					// Use comment as name if present, otherwise filename
					nameptr = (fib->fib_Comment[0]) ? fib->fib_Comment : fib->fib_FileName;

					// Allocate name
					if ((name = AllocMemH(menu->ph_Memory, strlen(nameptr) + len + 2)))
					{
						// Build name
						lsprintf(name, "%s %s", to_str, nameptr);

						// Add item
						if ((new = PopUpNewItem(menu, (IPTR)name, MENU_ICON_COPY_TO, POPUPF_STRING)))
						{
							// Allocate space for name
							if ((name = AllocMemH(menu->ph_Memory, strlen(fib->fib_FileName) + 23)))
							{
								// Build full name of script
								strcpy(name + 1, "DOpus5:System/CopyTo/");
								strcat(name + 1, fib->fib_FileName);
								new->data = name;
							}
						}
					}
				}

				// Otherwise, treat it as a normal 'pointer'
				else if ((name = AllocMemH(menu->ph_Memory, strlen(fib->fib_FileName) + len + 2)))
				{
					char *ptr;

					// Fix illegal characters
					for (ptr = fib->fib_FileName; *ptr; ptr++)
					{
						if (*ptr == ';')
							*ptr = ':';
						else if (*ptr == '\\')
							*ptr = '/';
					}

					// Build name
					lsprintf(name, "%s %s", to_str, fib->fib_FileName);

					// Add item
					if ((new = PopUpNewItem(menu, (IPTR)name, MENU_ICON_COPY_TO, POPUPF_STRING)))
					{
						// Does entry have a comment?
						if (fib->fib_Comment[0])
						{
							// Allocate copy
							if ((name = AllocMemH(menu->ph_Memory, strlen(fib->fib_Comment) + 1)))
							{
								// Copy it and set it as data
								strcpy(name, fib->fib_Comment);
								new->data = name;
							}
						}
					}
				}
			}
		}

		// Unlock directory
		UnLock(lock);
	}

	// Need default?
	if (ok)
	{
		// Default copy items
		PopUpNewItem(menu, MSG_ICON_COPY_RAM, MENU_ICON_COPY_TO, POPUPF_LOCALE);
		PopUpNewItem(menu, MSG_ICON_COPY_DF0, MENU_ICON_COPY_TO, POPUPF_LOCALE);
	}

	// Otherwise, add a separator
	else
	{
		PopUpSeparator(menu);
	}

	// Copy to desktop
	PopUpNewItem(menu, MSG_ICON_COPY_DESKTOP, MENU_ICON_COPY_DESKTOP, POPUPF_LOCALE);

	// Copy to other
	PopUpNewItem(menu, MSG_ICON_COPY_OTHER, MENU_ICON_COPY_OTHER, POPUPF_LOCALE);

	// Set submenu flag
	item->flags |= POPUPF_SUB;

	// Get list pointer
	menu->ph_List = 0;
}

// Build the OpenWith menu
void popup_build_openwith(PopUpHandle *menu)
{
	PopUpItem *item;

	// Lock the OpenWith list
	lock_listlock(&GUI->open_with_list, 0);

	// Is the list empty?
	if (IsListEmpty(&GUI->open_with_list.list))
	{
		// Add single item
		PopUpNewItem(menu, MSG_OPEN_WITH_MENU, MENU_OPEN_WITH, 0);
	}

	// Stuff in list
	else
	{
		// Add sub-menu item, create sub-menu list
		if ((item = PopUpNewItem(menu, MSG_OPEN_WITH_MENU_SUB, MENU_OPEN_WITH, 0)) && (PopUpItemSub(menu, item)))
		{
			struct Node *node;
			short count = 0;

			// Set submenu flag
			item->flags |= POPUPF_SUB;

			// Go through OpenWith list
			for (node = GUI->open_with_list.list.lh_Head; node->ln_Succ; node = node->ln_Succ, count++)
			{
				// Add item
				PopUpNewItem(menu, (IPTR)FilePart(node->ln_Name), MENU_OPEN_WITH_BASE + count, POPUPF_STRING);
			}

			// Add separator
			PopUpSeparator(menu);

			// Add 'other' menu
			PopUpNewItem(menu, MSG_OPEN_WITH_MENU_OTHER, MENU_OPEN_WITH, 0);

			// End the sub-menu
			PopUpEndSub(menu);
		}
	}

	// Unlock list and return
	unlock_listlock(&GUI->open_with_list);
}

// Add the View sub-menu (name, icon, icon action, show all) for a lister
BOOL popup_lister_view_menu(Lister *lister, PopUpHandle *menu)
{
	PopUpItem *item, *sub;

	// View item
	if (!(item = PopUpNewItem(menu, MSG_LISTER_VIEW, 0, POPUPF_SUB | POPUPF_LOCALE)))
		return 0;

	// Create list for submenus
	if (!(PopUpItemSub(menu, item)))
	{
		item->flags |= POPUPF_DISABLED;
		return 1;
	}

	// View by name
	if ((sub = PopUpNewItem(menu, MSG_LISTER_VIEW_NAME, MENU_LISTER_VIEW_NAME, POPUPF_CHECKIT | POPUPF_LOCALE)))
	{
		// Selected?
		if (!(lister->flags & (LISTERF_VIEW_ICONS | LISTERF_ICON_ACTION)))
			sub->flags |= POPUPF_CHECKED;
	}

	// View by icon
	if ((sub = PopUpNewItem(menu, MSG_LISTER_VIEW_ICON, MENU_LISTER_VIEW_ICON, POPUPF_CHECKIT | POPUPF_LOCALE)))
	{
		// Selected?
		if ((lister->flags & (LISTERF_VIEW_ICONS | LISTERF_ICON_ACTION)) == LISTERF_VIEW_ICONS)
			sub->flags |= POPUPF_CHECKED;
	}

	// Icon action mode
	if ((sub = PopUpNewItem(menu, MSG_LISTER_ICON_ACTION, MENU_LISTER_ICON_ACTION, POPUPF_CHECKIT | POPUPF_LOCALE)))
	{
		// Selected?
		if (lister->flags & LISTERF_ICON_ACTION)
			sub->flags |= POPUPF_CHECKED;
	}

	// Separator
	PopUpSeparator(menu);

	// Show all
	if ((sub = PopUpNewItem(menu, MSG_LISTER_SHOW_ALL, MENU_LISTER_SHOW_ALL, POPUPF_CHECKIT | POPUPF_LOCALE)))
	{
		// Selected?
		if (lister->flags & LISTERF_SHOW_ALL && lister->flags & LISTERF_VIEW_ICONS)
			sub->flags |= POPUPF_CHECKED;
	}

	// End sub menu
	PopUpEndSub(menu);
	return 1;
}

// Default lister popup menu
void popup_default_menu(BackdropInfo *info, PopUpHandle *menu, short *extnum)
{
	short flags = 0;
	PopUpItem *item;
	PopUpExt *ext;

	// Lister in title bar mode?
	if (info->lister && info->lister->more_flags & LISTERF_TITLEBARRED)
	{
		// Restore, iconify, close
		PopUpNewItem(menu, MSG_LISTER_RESTORE, MENU_LISTER_RESTORE, 0);
		PopUpNewItem(menu, MSG_LISTER_ICONIFY, MENU_LISTER_ICONIFY, 0);
		PopUpNewItem(menu, MSG_CLOSE_LISTER_MENU, MENU_LISTER_CLOSE, 0);
		return;
	}

	// Iconify
	PopUpNewItem(menu, MSG_LISTER_ICONIFY, MENU_LISTER_ICONIFY, 0);

	// Snapshot disabled?
	if ((!info->lister->cur_buffer->buf_ExpandedPath[0] || !(info->lister->cur_buffer->flags & DWF_VALID)) &&
		!info->lister->cur_buffer->buf_CustomHandler[0])
		flags = POPUPF_DISABLED;

	// Snapshot sub-item
	if ((item = PopUpNewItem(menu, MSG_ICON_SNAPSHOT_MENU, 0, POPUPF_SUB | POPUPF_LOCALE | flags)))
	{
		// Create list for submenus
		if (PopUpItemSub(menu, item))
		{
			short icons = POPUPF_DISABLED, selicons = POPUPF_DISABLED;

			// Lock backdrop list
			lock_listlock(&info->objects, 0);

			// Got icons?
			if (!IsListEmpty(&info->objects.list))
			{
				BackdropObject *object;
				icons = 0;

				// Go through backdrop list
				for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ;
					 object = (BackdropObject *)object->node.ln_Succ)
				{
					// Selected icon?
					if (object->state)
					{
						selicons = 0;
						break;
					}
				}
			}

			// Unlock backdrop list
			unlock_listlock(&info->objects);

			// Snapshot items
			PopUpNewItem(menu, MSG_ICON_SNAPSHOT_ICONS, MENU_ICON_SNAPSHOT_ICON, selicons);
			PopUpNewItem(menu, MSG_ICON_SNAPSHOT_LISTER, MENU_LISTER_SNAPSHOT, 0);
			PopUpSeparator(menu);
			PopUpNewItem(menu, MSG_ICON_SNAPSHOT_ALL, MENU_ICON_SNAPSHOT_ALL, icons);

			// End sub menu
			PopUpEndSub(menu);
		}
	}

	// Separator
	PopUpSeparator(menu);

	// View item
	if (popup_lister_view_menu(info->lister, menu))
	{
		// Separator
		PopUpSeparator(menu);
	}

	// Lister in file mode?
	if (info->lister && !(info->lister->flags & LISTERF_VIEW_ICONS))
	{
		// Disable arrange and cleanup menu
		flags = POPUPF_DISABLED;
	}
	else
		flags = 0;

	// Arrange icons item
	if ((item = PopUpNewItem(menu, MSG_LISTER_ARRANGE_ICONS, 0, POPUPF_SUB | POPUPF_LOCALE)))
	{
		// Lister in file mode?
		if (flags)
		{
			// Disable arrange menu
			item->flags |= flags;
		}

		// Create list for submenus
		else if ((item->data = AllocMemH(menu->ph_Memory, sizeof(struct MinList))))
		{
			// Initialise list
			NewList((struct List *)item->data);

			// Get list pointer
			menu->ph_List = (struct List *)item->data;

			// Add items
			PopUpNewItem(menu, MSG_LISTER_ARRANGE_NAME, MENU_LISTER_ARRANGE_NAME, POPUPF_LOCALE);
			PopUpNewItem(menu, MSG_LISTER_ARRANGE_TYPE, MENU_LISTER_ARRANGE_TYPE, POPUPF_LOCALE);
			PopUpNewItem(menu, MSG_LISTER_ARRANGE_SIZE, MENU_LISTER_ARRANGE_SIZE, POPUPF_LOCALE);
			PopUpNewItem(menu, MSG_LISTER_ARRANGE_DATE, MENU_LISTER_ARRANGE_DATE, POPUPF_LOCALE);

			// Clear list pointer
			menu->ph_List = 0;
		}

		// Failed
		else
			item->id = MENU_ICON_CLEANUP;
	}

	// CleanUp
	PopUpNewItem(menu, MSG_ICON_CLEANUP, MENU_ICON_CLEANUP, flags);

	// Resize to fit (lister + group windows only - the desktop spans the
	// whole screen so it makes no sense there). We piggyback the existing
	// `flags` so it auto-disables in file-view listers, then add our own
	// disable bit for backdrop windows / no-icon situations.
	{
		ULONG fit_flags = flags;
		if (info->window && (info->window->Flags & WFLG_BACKDROP))
			fit_flags |= POPUPF_DISABLED;
		else if (IsListEmpty(&info->objects.list))
			fit_flags |= POPUPF_DISABLED;
		PopUpNewItem(menu, MSG_LISTER_RESIZE_FIT, MENU_LISTER_RESIZE_FIT, fit_flags);
	}

	// Separator
	PopUpSeparator(menu);

	// New drawer
	PopUpNewItem(menu, MSG_LISTER_NEW_DRAWER, MENU_ICON_MAKEDIR, flags & ~POPUPF_DISABLED);

	// New file
	PopUpNewItem(menu, MSG_LISTER_NEW_FILE, MENU_ICON_MAKEFILE, flags & ~POPUPF_DISABLED);

	// Clear 'separator' flag
	menu->ph_Flags &= ~POPHF_SEP;

	// Lock extension list
	lock_listlock(&GUI->popupext_list, FALSE);

	// Go through list
	for (ext = (PopUpExt *)GUI->popupext_list.list.lh_Head; ext->pe_Node.ln_Succ;
		 ext = (PopUpExt *)ext->pe_Node.ln_Succ)
	{
		// Lister?
		if (ext->pe_Type == POPUP_LISTER)
		{
			// Add a separator if needed
			if (!(menu->ph_Flags & POPHF_SEP))
				PopUpSeparator(menu);

			// Allocate item
			if ((item = PopUpNewItem(menu, (IPTR)ext->pe_Menu, MENU_EXTENSION + *extnum, POPUPF_STRING)))
			{
				// Set data pointer
				item->data = ext;
				++*extnum;
			}
		}
	}

	// Unlock extension list
	unlock_listlock(&GUI->popupext_list);
}

// Build icon menu
MatchHandle *popup_build_icon_menu(BackdropInfo *info,
								   char *filename,
								   BackdropObject *object,
								   ULONG flags,
								   short *extnum,
								   PopUpHandle *menu)
{
	unsigned short bad, app, group, leftout, assign, nosnap, noinfo, templeft, realleft, nodev;
	BOOL command = 0;
	long object_type = 0;
	PopUpExt *ext;
	MatchHandle *handle = 0;
	PopUpItem *item;

	// Given a file?
	if (filename)
	{
		// Set flags for a file supplied
		bad = 0;
		app = 0;
		group = 0;
		leftout = 0;
		realleft = 0;
		templeft = POPUPF_DISABLED;
		assign = 0;
		nosnap = 0;
		noinfo = 0;
		nodev = 0;

		// Disk?
		if (flags & BPF_DISK)
			object_type = WBDISK;

		// Otherwise, get handle
		else if ((handle = GetMatchHandle(filename)))
		{
			// Directory?
			if (handle->fib.fib_DirEntryType > 0)
			{
				// Trashcan?
				if (stricmp(handle->fib.fib_FileName, "trashcan") == 0)
					object_type = WBGARBAGE;
				else
					object_type = WBDRAWER;
			}

			// Tool?
			else if (handle->flags & MATCHF_EXECUTABLE)
				object_type = WBTOOL;

			// Project
			else
				object_type = WBPROJECT;
		}
	}

	// Got an icon
	else
	{
		// Flags for various types
		bad = (object->type == BDO_BAD_DISK) ? POPUPF_DISABLED : 0;
		app = (object->type == BDO_APP_ICON) ? POPUPF_DISABLED : 0;
		group = (object->type == BDO_GROUP) ? POPUPF_DISABLED : 0;
		leftout = (object->type == BDO_LEFT_OUT && !(object->flags & BDOF_DESKTOP_FOLDER) &&
				   info->flags & (BDIF_MAIN_DESKTOP | BDIF_GROUP))
					  ? POPUPF_DISABLED
					  : 0;
		realleft =
			(object->type == BDO_LEFT_OUT && !(object->flags & BDOF_DESKTOP_FOLDER) && info->flags & BDIF_MAIN_DESKTOP)
				? POPUPF_DISABLED
				: 0;
		templeft = (realleft && object->flags & BDOF_TEMP_LEFTOUT) ? POPUPF_DISABLED : 0;
		command = (realleft && !templeft && command_filetype_match(object));
		assign = (object->flags & BDOF_ASSIGN) ? POPUPF_DISABLED : 0;
		nosnap = (flags & BPF_DISK && info->lister) ? POPUPF_DISABLED : 0;
		noinfo = (object->type == BDO_APP_ICON && !(flags & BPF_CANINFO)) ? POPUPF_DISABLED : 0;
		nodev = (object->type == BDO_DISK && !object->device_name) ? POPUPF_DISABLED : 0;

		// Write-protected disk?
		if (flags & BPF_DISK && flags & BPF_WRITEPROT)
			nosnap = POPUPF_DISABLED;

		// Get icon type
		object_type = object->icon->do_Type;
	}

	// Fix type for bad disks
	if (object && object->type == BDO_BAD_DISK)
		object_type = WBKICK;

	// Open - ghosted for bad disks
	PopUpNewItem(menu,
				 (flags & BPF_CLOSE) ? MSG_CLOSE : MSG_ICON_OPEN_MENU,
				 (flags & BPF_CLOSE) ? MENU_ICON_CLOSE : MENU_ICON_OPEN,
				 bad);

	// Open with for projects
	if (object_type == WBPROJECT && (!object || object->type != BDO_APP_ICON))
	{
		// Build OpenWith menu
		popup_build_openwith(menu);
	}

	// Open in New Window for drawers
	if (!object && object_type == WBDRAWER)
		PopUpNewItem(menu, MSG_OPEN_NEW_WINDOW, MENU_OPEN_NEW_WINDOW, 0);

	// If the object is a cache drawer, can't do anything else
	if (object && object->flags & BDOF_CACHE)
		bad = POPUPF_DISABLED;

	// Information - ghosted for appicons & bad disks, and assigns
	PopUpNewItem(menu, MSG_ICON_INFO_MENU, MENU_ICON_INFO, bad | assign | noinfo);

	// Add separator
	PopUpSeparator(menu);

	// Snapshot - ghosted for bad, and maybe appicon, and assigns, and temp leftouts
	PopUpNewItem(menu,
				 MSG_ICON_SNAPSHOT_MENU,
				 MENU_ICON_SNAPSHOT_ICON,
				 bad | ((flags & BPF_SNAPSHOT) ? 0 : app) | assign | nosnap | templeft);

	// Unsnapshot - same
	PopUpNewItem(menu,
				 MSG_ICON_UNSNAPSHOT_MENU,
				 MENU_ICON_UNSNAPSHOT,
				 bad | ((flags & BPF_SNAPSHOT) ? 0 : app) | assign | nosnap | templeft);

	// Not an appicon?
	if (!object || object->type != BDO_APP_ICON)
	{
		// Add separator
		PopUpSeparator(menu);

		// Trashcan?
		if (flags & BPF_TRASH)
		{
			// Add 'empty' item
			PopUpNewItem(menu, MSG_EMPTY_TRASH, MENU_EMPTY_TRASH, 0);
		}

		// Not trash
		else
		{
			// Icon in a lister, or temp leftout?
			if ((filename && !(flags & BPF_DISK)) ||
				(object && object->type == BDO_LEFT_OUT && !(object->flags & BDOF_DESKTOP_FOLDER) &&
				 !(info->flags & BDIF_MAIN_DESKTOP)) ||
				templeft)
			{
				// Leave out
				PopUpNewItem(menu, MSG_ICON_LEAVE_OUT_MENU, MENU_ICON_LEAVE_OUT, 0);

				// Short cut
				if (environment->env->env_flags & ENVF_ENABLE_SHORTCUTS && (filename || !templeft))
				{
					PopUpNewItem(menu, MSG_ICONS_SHORTCUT_MENU, MENU_ICON_SHORTCUT, 0);
					PopUpSeparator(menu);
				}
			}

			// Copy
			if ((item = PopUpNewItem(menu,
									 (flags & BPF_DISK) ? MSG_ICON_COPY2 : MSG_ICON_COPY,
									 MENU_ICON_COPY,
									 bad | group | leftout | assign | POPUPF_LOCALE)))
			{
				// Icon in a lister?
				if ((filename && !(flags & BPF_DISK)) ||
					(object && object->type == BDO_LEFT_OUT &&
					 (!(info->flags & (BDIF_MAIN_DESKTOP | BDIF_GROUP)) || object->flags & BDOF_DESKTOP_FOLDER)))
				{
					// Build 'CopyTo' menu
					popup_build_copyto(menu, item);
				}

				// Disk?
				else if (flags & BPF_DISK)
				{
					// Disable if disk is unavailable
					if (flags & BPF_NOFORMAT)
						item->flags |= POPUPF_DISABLED;
				}
			}

			// Rename
			PopUpNewItem(menu, MSG_ICON_RENAME, MENU_ICON_RENAME, bad | nodev);
		}
	}

	// Reset separator flag
	menu->ph_Flags &= ~POPHF_SEP;

	// Leftout/group or assign?
	if ((filename && !(flags & BPF_DISK)) ||
		(object && (object->type == BDO_LEFT_OUT || object->type == BDO_GROUP || object->flags & BDOF_ASSIGN)))
	{
		// Delete
		if (!(flags & BPF_TRASH))
		{
			if (realleft)
			{
				PopUpNewItem(menu, MSG_ICON_PUT_AWAY_MENU, MENU_ICON_PUT_AWAY, 0);

				if (command)
					PopUpNewItem(menu, MSG_DELETE, MENU_ICON_DELETE, 0);
			}
			else
				PopUpNewItem(menu, MSG_DELETE, MENU_ICON_DELETE, 0);
		}
	}

	// Disk?
	else if ((flags & BPF_DISK || object->type == BDO_BAD_DISK) && (!object || !(object->flags & BDOF_CACHE)))
	{
		// Format
		PopUpNewItem(menu,
					 MSG_ICON_FORMAT,
					 MENU_ICON_FORMAT,
					 (flags & (BPF_NOFORMAT | BPF_WRITEPROT)) ? POPUPF_DISABLED : assign);

		// Disk information
		PopUpNewItem(menu, MSG_ICON_DISKINFO, MENU_ICON_DISKINFO, bad);

		// Eject
		PopUpNewItem(menu, MSG_ICON_EJECT, MENU_ICON_EJECT, (flags & BPF_NOEJECT) ? POPUPF_DISABLED : assign);
	}

	// Not an AppIcon or a Group?
	if (!object || (object->type != BDO_APP_ICON && object->type != BDO_GROUP))
	{
		// Lock extension list
		lock_listlock(&GUI->popupext_list, FALSE);

		// Go through list
		for (ext = (PopUpExt *)GUI->popupext_list.list.lh_Head; ext->pe_Node.ln_Succ;
			 ext = (PopUpExt *)ext->pe_Node.ln_Succ)
		{
			BOOL match = 0;

			// All?
			if (ext->pe_Type == POPUP_ALL)
				match = 1;

			// Left-out?
			else if (ext->pe_Type == POPUP_LEFTOUT)
			{
				// Is this left-out?
				if (object && object->type == BDO_LEFT_OUT && !(object->flags & BDOF_DESKTOP_FOLDER) &&
					info->flags & BDIF_MAIN_DESKTOP)
					match = 1;
			}

			// Match this icon type?
			else if (!*ext->pe_FileType && ext->pe_Type == object_type)
				match = 1;

			// Matched?
			if (match)
			{
				// Add a separator if needed
				if (!(menu->ph_Flags & POPHF_SEP))
					PopUpSeparator(menu);

				// Allocate item
				if ((item = PopUpNewItem(menu, (IPTR)ext->pe_Menu, MENU_EXTENSION + *extnum, POPUPF_STRING)))
				{
					// Set data pointer
					item->data = ext;
					++*extnum;
				}
			}
		}

		// Unlock extension list
		unlock_listlock(&GUI->popupext_list);
	}

	return handle;
}
