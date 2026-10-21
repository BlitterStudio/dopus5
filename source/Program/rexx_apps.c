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

// dopus addappicon port text id [pos pos] [quotes] [info] [snap] [close] [icon filename] [menu stem] [local] [locked]

enum {
	APPARG_POS,
	APPARG_QUOTES,
	APPARG_INFO,
	APPARG_SNAP,
	APPARG_CLOSE,
	APPARG_ICON,
	APPARG_MENU,
	APPARG_LOCAL,
	APPARG_LOCKED,
};

// Add an AppIcon from rexx
IPTR rexx_add_appicon(char *str, struct RexxMsg *msg)
{
	char iconfile[256], menustem[80];
	RexxAppThing *app;
	struct TagItem *tags;
	short key, menu_count = 0, count;
	long base = 0;
	APTR memory;

	// Allocate AppNode
	if (!(app = AllocVec(sizeof(RexxAppThing), MEMF_CLEAR)))
		return 0;

	// Allocate memory handle
	if (!(memory = NewMemHandle(0, 0, MEMF_CLEAR)))
	{
		FreeVec(app);
		return 0;
	}

	// Set type
	app->node.ln_Type = REXXAPP_ICON;

	// Initialise position
	app->pos_x = NO_ICON_POSITION;
	app->pos_y = NO_ICON_POSITION;

	// Get port name
	rexx_parse_word(&str, app->port_name, sizeof(app->port_name));

	// Get icon name
	rexx_parse_word(&str, app->icon_name, sizeof(app->icon_name));

	// Get ID
	rexx_skip_space(&str);
	app->id = rexx_parse_number(&str, 0, 0);

	// Clear buffers
	iconfile[0] = 0;
	menustem[0] = 0;

	// Position set?
	rexx_skip_space(&str);
	while ((key = rexx_match_keyword(&str, pos_keys, 0)) != -1)
	{
		// Position?
		if (key == APPARG_POS)
		{
			// Get position
			app->pos_x = rexx_parse_number(&str, 1, NO_ICON_POSITION);
			app->pos_y = rexx_parse_number(&str, 0, NO_ICON_POSITION);
		}

		// Quotes?
		else if (key == APPARG_QUOTES)
			app->flags |= RATF_QUOTES;

		// Info?
		else if (key == APPARG_INFO)
			app->flags |= RATF_INFO;

		// Snapshot
		else if (key == APPARG_SNAP)
			app->flags |= RATF_SNAP;

		// Close
		else if (key == APPARG_CLOSE)
			app->flags |= RATF_CLOSE;

		// Local
		else if (key == APPARG_LOCAL)
			app->flags |= RATF_LOCAL;

		// Locked
		else if (key == APPARG_LOCKED)
			app->flags |= RATF_LOCKED;

		// Icon
		else if (key == APPARG_ICON)
		{
			// Get filename
			rexx_parse_word(&str, iconfile, 256);
		}

		// Menu
		else if (key == APPARG_MENU)
		{
			// Get menu stem
			if (rexx_parse_word(&str, menustem, 30))
			{
				// Check stem has a period
				if (menustem[0] && menustem[strlen(menustem) - 1] != '.')
					strcat(menustem, ".");
			}
		}

		// Skip spaces
		rexx_skip_space(&str);
	}

	// Try and get icon
	if (iconfile[0])
		app->icon = GetCachedDiskObject(iconfile, GCDOF_NOCACHE);

	// Failed? Get default
	if (!app->icon && !(app->icon = GetCachedDefDiskObject(WBTOOL | GCDOF_NOCACHE)))
	{
		// Failed completely
		FreeVec(app);
		return 0;
	}

	// Set icon position
	app->icon->do_CurrentX = app->pos_x - WBICONMAGIC_X;
	app->icon->do_CurrentY = app->pos_y - WBICONMAGIC_Y;
	SetIconFlags(app->icon, GetIconFlags(app->icon) & ~ICONF_POSITION_OK);

	// Remap the icon (only if a local one)
	if (app->flags & RATF_LOCAL)
		RemapIcon(app->icon, GUI->screen_pointer, FALSE);

	// Menu items?
	if (menustem[0])
	{
		char buffer[10];

		// Get count
		rexx_get_var(msg, menustem, "COUNT", buffer, 10);
		menu_count = atoi(buffer);

		// Get base
		if (rexx_get_var(msg, menustem, "BASE", buffer, 10))
			base = atoi(buffer);
	}

	// Number of tags needed
	count = 7 + menu_count;

	// Allocate tags
	if ((tags = AllocMemH(memory, sizeof(struct TagItem) * count)))
	{
		// Initialise tags
		tags[0].ti_Tag = DAE_SnapShot;
		tags[0].ti_Data = (app->flags & RATF_SNAP) ? 1 : 0;
		tags[1].ti_Tag = DAE_Info;
		tags[1].ti_Data = (app->flags & RATF_INFO) ? 1 : 0;
		tags[2].ti_Tag = DAE_Close;
		tags[2].ti_Data = (app->flags & RATF_CLOSE) ? 1 : 0;
		tags[3].ti_Tag = (app->flags & RATF_LOCAL) ? DAE_Local : TAG_IGNORE;
		tags[3].ti_Data = 1;
		tags[4].ti_Tag = DAE_Locked;
		tags[4].ti_Data = (app->flags & RATF_LOCKED) ? 1 : 0;
		tags[5].ti_Tag = DAE_MenuBase;
		tags[5].ti_Data = base;

		// Go through menus
		for (count = 0; count < menu_count; count++)
		{
			char buffer[80];

			// Build variable name
			lsprintf(iconfile, "%ld", count);

			// Get variable
			if (rexx_get_var(msg, menustem, iconfile, buffer, 80))
			{
				// Allocate buffer
				if ((tags[count + 6].ti_Data = (IPTR)AllocMemH(memory, strlen(buffer) + 1)))
				{
					// Copy name
					strcpy((char *)tags[count + 6].ti_Data, buffer);

					// Set tag ID
					tags[count + 6].ti_Tag = DAE_Menu;
				}
				else
					tags[count + 6].ti_Tag = TAG_IGNORE;
			}

			// Skip this tag
			else
				tags[count + 6].ti_Tag = TAG_IGNORE;
		}
	}

	// Add AppIcon
	app->app_thing = AddAppIconA(app->id, (IPTR)app, app->icon_name, GUI->rexx_app_port, 0, app->icon, tags);

	// Free memory
	FreeMemHandle(memory);

	// Failed?
	if (!app->app_thing)
	{
		// Failed
		if (app->flags & RATF_LOCAL)
			RemapIcon(app->icon, GUI->screen_pointer, TRUE);
		FreeCachedDiskObject(app->icon);
		FreeVec(app);
		return 0;
	}

	// Add to AppList
	lock_listlock(&GUI->rexx_apps, TRUE);
	AddTail(&GUI->rexx_apps.list, (struct Node *)app);
	unlock_listlock(&GUI->rexx_apps);

	return (IPTR)app;
}

// Remove an AppIcon
void rexx_rem_appthing(char *str, short type)
{
	RexxAppThing *app = 0, *look, *next;

	// Get app handle
	if (type != REXXAPP_ALL)
	{
		// Skip spaces
		rexx_skip_space(&str);

		// Get address
		if (!(app = (RexxAppThing *)rexx_parse_iptr(&str, 0, 0)))
			return;
	}

	// Lock list
	lock_listlock(&GUI->rexx_apps, TRUE);

	// Go through list
	for (look = (RexxAppThing *)GUI->rexx_apps.list.lh_Head; look->node.ln_Succ; look = next)
	{
		// Cache next pointer
		next = (RexxAppThing *)look->node.ln_Succ;

		// Match the one we're looking for?
		if (type == REXXAPP_ALL || look == app)
		{
			// Check type
			if (type == REXXAPP_ALL || look->node.ln_Type == type)
			{
				// Remove it
				Remove((struct Node *)look);

				// Icon?
				if (look->node.ln_Type == REXXAPP_ICON)
				{
					// Remove icon
					RemoveAppIcon(look->app_thing);

					// Free the remap (if local)
					if (look->flags & RATF_LOCAL)
						RemapIcon(look->icon, GUI->screen_pointer, TRUE);

					// Free icon
					FreeCachedDiskObject(look->icon);

					// If we're removing all, send goodbye
					if (type == REXXAPP_ALL)
					{
						if (look->port_name[0])
							rexx_send_appmsg(look, REXXAPPCMD_QUIT, 0);
					}
				}

				// Progress?
				else if (look->node.ln_Type == REXXAPP_PROGRESS)
				{
					// Close window
					CloseProgressWindow(((RexxProgress *)look)->progress);
				}

				// Free data
				FreeVec(look);
			}

			// Unless we're removing all, break
			if (type != REXXAPP_ALL)
				break;
		}
	}

	// Unlock list
	unlock_listlock(&GUI->rexx_apps);
}

// Handle an AppMessage
void rexx_handle_appmsg(struct AppMessage *msg)
{
	RexxAppThing *app;
	struct AppSnapshotMsg *amsg;
	short type;

	// Lock list
	lock_listlock(&GUI->rexx_apps, FALSE);

	// Snapshot?
	if (msg->am_Type == MTYPE_APPSNAPSHOT)
	{
		// Get message pointer
		amsg = (struct AppSnapshotMsg *)msg;

		// Close?
		if (amsg->flags & APPSNAPF_CLOSE)
			type = REXXAPPCMD_CLOSE;

		// Info?
		else if (amsg->flags & APPSNAPF_INFO)
			type = REXXAPPCMD_INFO;

		// Menu?
		else if (amsg->flags & APPSNAPF_MENU)
			type = REXXAPPCMD_MENU;

		// Snapshot
		else
			type = REXXAPPCMD_SNAPSHOT;
	}

	// Double-click?
	else if (msg->am_NumArgs == 0)
		type = REXXAPPCMD_DOUBLECLICK;

	// Drop
	else
		type = REXXAPPCMD_DRAGNDROP;

	// Go through list
	for (app = (RexxAppThing *)GUI->rexx_apps.list.lh_Head; app->node.ln_Succ; app = (RexxAppThing *)app->node.ln_Succ)
	{
		// Match with UserData in AppMessage
		if (app == (RexxAppThing *)msg->am_UserData)
		{
			// Send message
			rexx_send_appmsg(app, type, msg);
			break;
		}
	}

	// Unlock list
	unlock_listlock(&GUI->rexx_apps);
}

// Send a message for an AppThing
BOOL rexx_send_appmsg(RexxAppThing *app, short type, struct AppMessage *msg)
{
	char buf[40];
	char *files = 0, *entry = 0;
	IPTR lister = 0;
	BOOL ret;

	// Snapshot?
	if (type == REXXAPPCMD_SNAPSHOT)
	{
		struct AppSnapshotMsg *sm;

		// Get snapshot pointer
		sm = (struct AppSnapshotMsg *)msg;

		// Unsnapshot?
		if (sm->flags & APPSNAPF_UNSNAPSHOT)
			type = REXXAPPCMD_UNSNAPSHOT;

		// Snapshot
		else
		{
			// Build coordinate string
			lsprintf(buf, "%ld,%ld", sm->position_x, sm->position_y);
			entry = buf;
		}
	}

	// Menu?
	else if (type == REXXAPPCMD_MENU)
	{
		struct AppSnapshotMsg *sm;

		// Get message pointer
		sm = (struct AppSnapshotMsg *)msg;

		// ID string
		lsprintf(buf, "%ld", sm->id);
		entry = buf;

		// Help?
		if (sm->flags & APPSNAPF_HELP)
			type = REXXAPPCMD_MENUHELP;
	}

	// Something with files?
	else if (msg && msg->am_NumArgs > 0)
	{
		// Build string of files
		if ((files = rexx_build_filestring(
				 (DOpusAppMessage *)msg, &lister, (app->flags & RATF_QUOTES) ? CUSTF_WANT_QUOTES : 0)))
			entry = files;
	}

	// Send message
	ret = rexx_handler_msg(app->port_name,
						   0,
						   (type != REXXAPPCMD_QUIT) ? RXMF_WARN : 0,
						   HA_String,
						   0,
						   app_commands[type],
						   HA_Value,
						   1,
						   app->id,
						   HA_String,
						   2,
						   entry,
						   HA_Value,
						   3,
						   lister,
						   HA_String,
						   4,
						   "icon",
						   TAG_END);

	// Free files string
	FreeVec(files);

	return ret;
}

enum {
	SETARG_TEXT,
	SETARG_BUSY,
	SETARG_LOCKED,
};

// Change an AppIcon from rexx
long rexx_set_appicon(char *str, struct RexxMsg *msg)
{
	RexxAppThing *app, *look;
	char name[40];
	short key, busy = -1, locked = -1;
	short name_change = 0;
	short ret = RXERR_INVALID_SET;

	// Get address
	if (!(look = (RexxAppThing *)rexx_parse_iptr(&str, 0, 0)))
		return RXERR_INVALID_HANDLE;

	// Lock list
	lock_listlock(&GUI->rexx_apps, TRUE);

	// Go through list
	for (app = (RexxAppThing *)GUI->rexx_apps.list.lh_Head; app->node.ln_Succ; app = (RexxAppThing *)app->node.ln_Succ)
	{
		// The one we're looking for?
		if (look == app && look->node.ln_Type == REXXAPP_ICON)
		{
			// Ok
			break;
		}
	}

	// Not found?
	if (!app->node.ln_Succ)
		return RXERR_INVALID_HANDLE;

	// Clear buffers
	name[0] = 0;

	// Keywords
	rexx_skip_space(&str);
	while ((key = rexx_match_keyword(&str, seticon_keys, 0)) != -1)
	{
		// Text?
		if (key == SETARG_TEXT)
		{
			// Get name
			rexx_parse_word(&str, name, 40);
			name_change = 1;
		}

		// Busy?
		else if (key == SETARG_BUSY)
		{
			busy = rexx_match_keyword(&str, on_off_strings, 0);
		}

		// Locked?
		else if (key == SETARG_LOCKED)
		{
			locked = rexx_match_keyword(&str, on_off_strings, 0);
		}

		// Skip spaces
		rexx_skip_space(&str);
	}

	// Got AppIcon?
	if (app->app_thing)
	{
		struct Image *image1 = 0, *image2 = 0;
		char *text = 0;
		long flags = 0;

		// Name change?
		if (name_change)
		{
			// Copy into buffer
			strcpy(app->icon_name, name);
			text = app->icon_name;
			flags = CAIF_TITLE;
		}

		// Locked changed?
		if (locked != -1)
		{
			flags |= CAIF_LOCKED;
			if (locked)
				flags |= CAIF_SET;
		}

		// Do change
		if (flags)
		{
			ChangeAppIcon(app->app_thing, image1, image2, text, flags);
			ret = 0;
		}

		// Busy changed?
		if (busy != -1)
		{
			// Set/clear busy flag
			ChangeAppIcon(
				app->app_thing, 0, 0, 0, (busy) ? CAIF_BUSY | CAIF_GHOST | CAIF_SET : CAIF_UNBUSY | CAIF_GHOST);
		}
	}

	return ret;
}

// Remap rexx icons
void rexx_remap_icons(BOOL free)
{
	RexxAppThing *look;

	// Lock list
	lock_listlock(&GUI->rexx_apps, TRUE);

	// Go through list
	for (look = (RexxAppThing *)GUI->rexx_apps.list.lh_Head; look->node.ln_Succ;
		 look = (RexxAppThing *)look->node.ln_Succ)
	{
		// Icon?
		if (look->node.ln_Type == REXXAPP_ICON)
		{
			// Remap it (if local)
			if (look->flags & RATF_LOCAL)
				RemapIcon(look->icon, GUI->screen_pointer, free);
		}
	}

	// Unlock list
	unlock_listlock(&GUI->rexx_apps);
}
