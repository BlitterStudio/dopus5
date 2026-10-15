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

// Build user menu
void display_build_user_menu(void)
{
	Cfg_Button *button;
	AppEntry *appmenu;
	short count = 0, tool_count = 0;

	// Lock menus
	GetSemaphore(&GUI->user_menu_lock, SEMF_SHARED, 0);

	// Free existing menu
	FreeVec(GUI->user_menu_data);

	// Got user menu?
	if (GUI->user_menu)
	{
		// Go through user menu
		for (button = (Cfg_Button *)GUI->user_menu->buttons.lh_Head; button->node.ln_Succ;
			 button = (Cfg_Button *)button->node.ln_Succ)
		{
			// Title?
			if (button->button.flags & BUTNF_TITLE)
				++count;

			// Otherwise
			else
			{
				Cfg_ButtonFunction *func;
				short num;

				// First item, default title?
				if (count == 0)
					count = 1;

				// Add this item
				++count;

				// Go through functions
				for (func = (Cfg_ButtonFunction *)button->function_list.mlh_Head, num = 0; func->node.ln_Succ;
					 func = (Cfg_ButtonFunction *)func->node.ln_Succ)
				{
					// Ignore function if empty
					if ((!function_label(func) || !*function_label(func)) &&
						IsListEmpty((struct List *)&func->instructions))
						continue;

					// Skip first function until we know there's more
					if (num > 0)
					{
						// Increment count
						++count;
						if (num == 1)
							++count;
					}

					// Increment function count
					++num;
				}
			}
		}
	}

	// Showing Tools menu?
	if (environment->env->display_options & DISPOPTF_SHOW_TOOLS && !(GUI->flags & GUIF_NO_TOOLS_MENU))
	{
		// Lock AppList
		appmenu = LockAppList();

		// Count tool (app) menus
		while ((appmenu = NextAppEntry(appmenu, APP_MENU)) && (++tool_count) < 63)
			;

		// Unlock AppList
		UnlockAppList();

		// Add tools title
		if (tool_count > 0)
			++count;
	}

	// Add end node
	++count;

	// Allocate data for user menus
	if ((GUI->user_menu_data = AllocVec(sizeof(MenuData) * (count + tool_count), MEMF_CLEAR)))
	{
		short num = 0;

		// Got a user menu?
		if (GUI->user_menu)
		{
			short item;

			// Go through user menu
			for (button = (Cfg_Button *)GUI->user_menu->buttons.lh_Head, item = 0; button->node.ln_Succ;
				 button = (Cfg_Button *)button->node.ln_Succ, item++)
			{
				Cfg_ButtonFunction *func;
				short title = 0;

				// Title?
				if (button->button.flags & BUTNF_TITLE)
					title = 1;

				// Otherwise
				else
				{
					// First item, default title?
					if (item == 0)
						title = 2;
				}

				// Need to do title?
				if (title)
				{
					// Get button function
					if (title == 1)
						func = (Cfg_ButtonFunction *)FindFunctionType((struct List *)&button->function_list,
																	  FTYPE_LEFT_BUTTON);
					else
						func = 0;

					// Fill in data
					GUI->user_menu_data[num].type = NM_TITLE;
					GUI->user_menu_data[num].id = (IPTR)func;
					if (func)
					{
						if (!(GUI->user_menu_data[num].name = (IPTR)function_label(func)))
							GUI->user_menu_data[num].name = (IPTR)GUI->null_string;
					}
					else
						GUI->user_menu_data[num].name = (IPTR)GetString(&locale, MSG_USER_MENU);
					GUI->user_menu_data[num].flags = MENUFLAG_TEXT_STRING;
					++num;
				}

				// Real function?
				if (title != 1)
				{
					Cfg_ButtonFunction *first = 0;
					short type = NM_ITEM;
					short tot;

					// Go through functions
					for (func = (Cfg_ButtonFunction *)button->function_list.mlh_Head, tot = 0; func->node.ln_Succ;
						 func = (Cfg_ButtonFunction *)func->node.ln_Succ)
					{
						// Ignore function if empty
						if ((!function_label(func) || !*function_label(func)) &&
							IsListEmpty((struct List *)&func->instructions))
							continue;

						// On the second function?
						if (tot == 1)
						{
							// Did the first function have instructions?
							if (!(IsListEmpty((struct List *)&first->instructions)))
							{
								// Use it again
								func = first;
							}
						}

						// Fill in menu data
						GUI->user_menu_data[num].type = type;
						GUI->user_menu_data[num].id = (IPTR)func;

						// Bar label?
						if (function_label(func) && strncmp(function_label(func), "---", 3) == 0)
							GUI->user_menu_data[num].name = NM_BAR_LABEL;

						// Normal function
						else
						{
							// Get name
							if (!(GUI->user_menu_data[num].name = (IPTR)function_label(func)))
								GUI->user_menu_data[num].name = (IPTR)GUI->null_string;
							GUI->user_menu_data[num].flags = MENUFLAG_TEXT_STRING;

							// Does function have a right-amiga hotkey?
							if (func->function.qual & IEQUALIFIER_RCOMMAND)
							{
								char key;

								// Convert from rawkey
								if (ConvertRawKey(func->function.code, 0, &key))
								{
									unsigned short qual;

									// For letters, shift is always down
									if (key >= 'a' && key <= 'z')
										qual = IEQUALIFIER_LSHIFT;

									// Otherwise, it might be down
									else
										qual = func->function.qual & IEQUAL_ANYSHIFT;

									// Convert again
									if (ConvertRawKey(func->function.code, qual, &key))
									{
										// Set flags for hotkey
										GUI->user_menu_data[num].flags |= MENUFLAG_USE_SEQ;
										GUI->user_menu_data[num].flags |= MENUFLAG_MAKE_SEQ(key);
									}
								}
							}
						}

						// Change type to sub-item
						type = NM_SUB;
						++num;

						// Remember first function
						if (!first)
							first = func;

						// Increment count
						++tot;
					}
				}
			}
		}

		// Tool menu?
		if (tool_count > 0)
		{
			short tot = 0;

			// Build tool title
			GUI->user_menu_data[num].type = NM_TITLE;
			GUI->user_menu_data[num].id = MENU_TOOL_MENU;
			GUI->user_menu_data[num++].name = MSG_TOOL_MENU;

			// Lock AppList
			appmenu = LockAppList();

			// Fill in tool menus
			while ((appmenu = NextAppEntry(appmenu, APP_MENU)) && (++tot) < 63)
			{
				// Fill in menu data
				GUI->user_menu_data[num].type = NM_ITEM;
				GUI->user_menu_data[num].id = (IPTR)appmenu;

				// Separator?
				if (strncmp(appmenu->text, "---", 3) == 0)
				{
					GUI->user_menu_data[num].name = NM_BAR_LABEL;
				}

				// Normal string
				else
				{
					GUI->user_menu_data[num].name = (IPTR)appmenu->text;
					GUI->user_menu_data[num].flags = MENUFLAG_TEXT_STRING;
				}

				// Increment count
				++num;
			}

			// Unlock AppList
			UnlockAppList();
		}

		// Last menu
		GUI->user_menu_data[num].type = NM_END;
	}

	// Go through fixed menus, look for NM_NEXT
	for (count = 0; dopus_menus[count].type != NM_NEXT; count++)
		;

	// Point fixed menus on to user menu
	dopus_menus[count].name = (IPTR)GUI->user_menu_data;

	// Unlock menus
	FreeSemaphore(&GUI->user_menu_lock);
}

// Reset window menus
void display_reset_menus(BOOL main_only, BOOL fix_only)
{
	// Do main window
	if (GUI->window)
	{
		// Reset?
		if (!fix_only)
		{
			// Free existing menu
			display_free_menu(GUI->window);

			// Get new menu
			display_get_menu(GUI->window);
		}

		// Fix checkmarks
		display_fix_menu(GUI->window, WINDOW_BACKDROP, 0);
	}

	// Doing main window only?
	if (main_only)
		return;

	// Signal children to reset their menus
	IPC_ListCommand(&GUI->lister_list, IPC_RESET, !fix_only, 0, 0);
	IPC_ListCommand(&GUI->buttons_list, IPC_RESET, !fix_only, 0, 0);
	IPC_ListCommand(&GUI->process_list, IPC_RESET, !fix_only, 0, 0);
	IPC_ListCommand(&GUI->group_list, IPC_RESET, !fix_only, 0, 0);
}

// Find a menu by userdata ID
struct MenuItem *find_menu_item(struct Menu *menu, UWORD id)
{
	// Go through all menus
	for (; menu; menu = menu->NextMenu)
	{
		struct MenuItem *item;

		// Is this what we're looking for?
		if (id == (UWORD)(IPTR)GTMENU_USERDATA(menu))
			return (struct MenuItem *)menu;

		// Go through items
		for (item = menu->FirstItem; item; item = item->NextItem)
		{
			struct MenuItem *sub;

			// Is this what we're looking for?
			if (id == (UWORD)(IPTR)GTMENUITEM_USERDATA(item))
				return item;

			// Go through sub items
			for (sub = item->SubItem; sub; sub = sub->NextItem)
			{
				// Is this what we're looking for?
				if (id == (UWORD)(IPTR)GTMENUITEM_USERDATA(sub))
					return sub;
			}
		}
	}

	// Not found
	return 0;
}

// Get a menu
void display_get_menu(struct Window *window)
{
	struct Menu *menu;
	APTR visual_info;

	// Get visual info
	if ((visual_info = GetVisualInfoA(window->WScreen, 0)))
	{
		// Build new menu strip
		if ((menu = BuildMenuStrip(dopus_menus, &locale)))
		{
			// Layout menus
			LayoutMenus(menu, visual_info, GTMN_NewLookMenus, TRUE, TAG_END);

			// Set menu strip
			SetMenuStrip(window, menu);
		}

		// Free visual info
		FreeVisualInfo(visual_info);
	}
}

// Free a menu
void display_free_menu(struct Window *window)
{
	struct Menu *menu;

	// Free existing menu
	if (window && (menu = window->MenuStrip))
	{
		ClearMenuStrip(window);
		FreeMenus(menu);
	}
}

// Fix menus for a window
void display_fix_menu(struct Window *window, long type, APTR spec)
{
	struct Menu *menu, *menustrip;
	struct MenuItem *item;

	// Get menu strip
	if (!window || !(menustrip = window->MenuStrip))
		return;

	// Clear menu strip
	ClearMenuStrip(window);

	// Create icons
	if ((item = find_menu_item(menustrip, MENU_CREATE_ICONS)))
	{
		if (GUI->flags & GUIF_SAVE_ICONS)
			item->Flags |= CHECKED;
		else
			item->Flags &= ~CHECKED;
	}

	// Clock item
	if ((item = find_menu_item(menustrip, MENU_CLOCK)))
	{
		if (GUI->flags & GUIF_CLOCK)
			item->Flags |= CHECKED;
		else
			item->Flags &= ~CHECKED;
	}

	// Filter
	if ((item = find_menu_item(menustrip, MENU_FILTER)))
	{
		if (GUI->flags & GUIF_FILE_FILTER)
			item->Flags |= CHECKED;
		else
			item->Flags &= ~CHECKED;
	}

	// Def public screen
	if ((item = find_menu_item(menustrip, MENU_DEFPUBSCR)))
	{
		if (GUI->flags & GUIF_DEFPUBSCR)
			item->Flags |= CHECKED;
		else
			item->Flags &= ~CHECKED;
	}

	// Backdrop item
	if ((item = find_menu_item(menustrip, MENU_BACKDROP)))
	{
		if (environment->env->env_flags & ENVF_BACKDROP)
			item->Flags |= CHECKED;
		else
			item->Flags &= ~CHECKED;
	}

	// Main backdrop window?
	if (type == WINDOW_BACKDROP)
	{
		// Show as name
		if ((item = find_menu_item(menustrip, MENU_LISTER_VIEW_NAME)))
		{
			if (GUI->flags & (GUIF_VIEW_ICONS | GUIF_ICON_ACTION))
				item->Flags &= ~CHECKED;
			else
				item->Flags |= CHECKED;
		}

		// Show as icon
		if ((item = find_menu_item(menustrip, MENU_LISTER_VIEW_ICON)))
		{
			if (GUI->flags & GUIF_VIEW_ICONS)
				item->Flags |= CHECKED;
			else
				item->Flags &= ~CHECKED;
		}

		// Icon action?
		if ((item = find_menu_item(menustrip, MENU_LISTER_ICON_ACTION)))
		{
			if (GUI->flags & GUIF_ICON_ACTION)
				item->Flags |= CHECKED;
			else
				item->Flags &= ~CHECKED;
		}

		// Show all
		if ((item = find_menu_item(menustrip, MENU_LISTER_SHOW_ALL)))
		{
			item->Flags &= ~CHECKED;
			if (GUI->flags & (GUIF_VIEW_ICONS | GUIF_ICON_ACTION))
			{
				if (GUI->flags & GUIF_SHOW_ALL)
					item->Flags |= CHECKED;
			}
		}
	}

	// Go through all menus
	for (menu = menustrip; menu; menu = menu->NextMenu)
	{
		struct MenuItem *item;

		// Go through items
		for (item = menu->FirstItem; item; item = item->NextItem)
		{
			struct MenuItem *sub;

			// Check disable
			menu_check_disable(item, type);

			// Go through sub items
			for (sub = item->SubItem; sub; sub = sub->NextItem)
			{
				// Check disable
				menu_check_disable(sub, type);
			}
		}
	}

	// Reset menu strip
	ResetMenuStrip(window, menustrip);
}

#define WINDOW_ALL (WINDOW_BACKDROP | WINDOW_LISTER | WINDOW_LISTER_ICONS | WINDOW_GROUP)

static ULONG menu_disable_keys[] = {WINDOW_BACKDROP | WINDOW_LISTER | WINDOW_LISTER_ICONS,
									MENU_ICON_MAKEDIR,
									WINDOW_BACKDROP | WINDOW_LISTER | WINDOW_LISTER_ICONS,
									MENU_ICON_MAKEFILE,
									WINDOW_BACKDROP | WINDOW_GROUP | WINDOW_LISTER_ICONS,
									MENU_ICON_OPEN,
									WINDOW_ALL,
									MENU_ICON_INFO,
									WINDOW_BACKDROP | WINDOW_GROUP | WINDOW_LISTER_ICONS,
									MENU_ICON_SNAPSHOT,
									WINDOW_GROUP | WINDOW_LISTER_ICONS,
									MENU_ICON_SNAPSHOT_WINDOW,
									WINDOW_BACKDROP | WINDOW_GROUP | WINDOW_LISTER_ICONS,
									MENU_ICON_UNSNAPSHOT,
									WINDOW_ALL,
									MENU_ICON_LEAVE_OUT,
									WINDOW_BACKDROP,
									MENU_ICON_PUT_AWAY,
									WINDOW_ALL,
									MENU_ICON_SELECT_ALL,
									WINDOW_BACKDROP | WINDOW_GROUP | WINDOW_LISTER_ICONS,
									MENU_ICON_ARRANGE,
									WINDOW_BACKDROP | WINDOW_GROUP | WINDOW_LISTER_ICONS,
									MENU_LISTER_ARRANGE_NAME,
									WINDOW_BACKDROP | WINDOW_GROUP | WINDOW_LISTER_ICONS,
									MENU_LISTER_ARRANGE_TYPE,
									WINDOW_BACKDROP | WINDOW_GROUP | WINDOW_LISTER_ICONS,
									MENU_LISTER_ARRANGE_SIZE,
									WINDOW_BACKDROP | WINDOW_GROUP | WINDOW_LISTER_ICONS,
									MENU_LISTER_ARRANGE_DATE,
									WINDOW_BACKDROP | WINDOW_GROUP | WINDOW_LISTER_ICONS,
									MENU_ICON_CLEANUP,
									WINDOW_LISTER_ICONS | WINDOW_GROUP,
									MENU_LISTER_RESIZE_FIT,
									WINDOW_LISTER_ICONS | WINDOW_GROUP,
									MENU_ICON_RESET,
									WINDOW_ALL,
									MENU_ICON_RENAME,
									WINDOW_BACKDROP,
									MENU_ICON_FORMAT,
									WINDOW_BACKDROP | WINDOW_LISTER | WINDOW_LISTER_ICONS,
									MENU_ICON_DISKINFO,
									WINDOW_BACKDROP,
									MENU_ICON_EJECT,
									WINDOW_ALL,
									MENU_ICON_DELETE,

									WINDOW_LISTER | WINDOW_LISTER_ICONS,
									MENU_OPEN_PARENT,
									WINDOW_LISTER | WINDOW_LISTER_ICONS | WINDOW_GROUP,
									MENU_LISTER_CLOSE,
									WINDOW_LISTER | WINDOW_LISTER_ICONS,
									MENU_LISTER_SOURCE,
									WINDOW_LISTER | WINDOW_LISTER_ICONS,
									MENU_LISTER_DEST,
									WINDOW_LISTER | WINDOW_LISTER_ICONS,
									MENU_LISTER_LOCK_SOURCE,
									WINDOW_LISTER | WINDOW_LISTER_ICONS,
									MENU_LISTER_LOCK_DEST,
									WINDOW_LISTER | WINDOW_LISTER_ICONS,
									MENU_LISTER_UNLOCK,
									WINDOW_LISTER | WINDOW_LISTER_ICONS,
									MENU_LISTER_OFF,
									WINDOW_LISTER,
									MENU_EDIT_LISTER,
									WINDOW_LISTER | WINDOW_LISTER_ICONS,
									MENU_LISTER_SNAPSHOT,
									WINDOW_LISTER | WINDOW_LISTER_ICONS,
									MENU_LISTER_UNSNAPSHOT,
									WINDOW_BACKDROP | WINDOW_LISTER_ICONS | WINDOW_LISTER,
									MENU_LISTER_VIEW,
									WINDOW_LISTER,
									MENU_LISTER_FILTER,

									WINDOW_BUTTONS,
									MENU_SAVE_BUTTONS,
									WINDOW_BUTTONS,
									MENU_SAVEAS_BUTTONS,
									WINDOW_BUTTONS,
									MENU_CLOSE_BUTTONS,
									0,
									0};

// Check to disable a menu item
void menu_check_disable(struct MenuItem *item, unsigned long type)
{
	short key;
	IPTR id;

	// Get menu ID
	id = (IPTR)GTMENUITEM_USERDATA(item);

	// Go through keys
	for (key = 0; menu_disable_keys[key + 1]; key += 2)
	{
		// Does ID match?
		if (menu_disable_keys[key + 1] == id)
		{
			// Disable this item?
			if ((menu_disable_keys[key] & type) != type)
			{
				// Disable it
				item->Flags &= ~ITEMENABLED;
			}

			// Enable it
			else
				item->Flags |= ITEMENABLED;
			return;
		}
	}
}
