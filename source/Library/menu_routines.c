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
#include "layout_routines.h"

/****************************************************************************
							  Build a menu strip
 ****************************************************************************/

struct Menu *LIBFUNC L_BuildMenuStrip(REG(a0, const MenuData *menudata), REG(a1, struct DOpusLocale *locale))
{
	short count;
	short level;
	unsigned long exclude_total[2];
	short level_start[2], level_value[2];
	struct NewMenu *new_menu;
	struct Menu *menustrip;
	const MenuData *data;
	APTR memory;

	// Count menus
	for (count = 0, data = menudata; data->type != NM_END;)
	{
		// Next?
		if (data->type == NM_NEXT)
		{
			data = (const MenuData *)data->name;
		}
		else
		{
			++count;
			++data;
		}
	}

	// Allocate memory handle
	if (!(memory = L_NewMemHandle(128, 8, MEMF_CLEAR)))
		return 0;

	// Allocate NewMenu structures
	if (!(new_menu = L_AllocMemH(memory, sizeof(struct NewMenu) * (count + 1))))
	{
		L_FreeMemHandle(memory);
		return 0;
	}

	// Initialise mutual exclude stuff
	level = 0;
	exclude_total[0] = 0;
	exclude_total[1] = 0;
	level_start[0] = 0;
	level_start[1] = 0;

	// Build NewMenu strip
	for (count = 0, data = menudata;;)
	{
		// Next?
		if (data->type == NM_NEXT)
		{
			data = (const MenuData *)data->name;
			continue;
		}

		// Set NewMenu type
		new_menu[count].nm_Type = data->type;

		// Menu ID
		new_menu[count].nm_UserData = (APTR)data->id;

		// Look at menu type
		if (data->type == NM_SUB)
		{
			// Entering new level?
			if (level == 0)
			{
				// Initialise sub-menu level
				level_start[1] = count;
				exclude_total[1] = 0;
				level_value[1] = 0;
				level = 1;
			}
		}

		// End of a level?
		if (data->type == NM_TITLE || data->type == NM_END || (data->type == NM_ITEM && level == 1))
		{
			// Valid level?
			if (level > -1)
			{
				short num, count;

				// Go from start of level, fix mutual exclude values
				for (num = level_start[level], count = 0;; num++, count++)
				{
					// End of level?
					if (new_menu[num].nm_Type != new_menu[level_start[level]].nm_Type)
					{
						// Unless we're only interrupted by a sub-menu, we break
						if (new_menu[num].nm_Type != NM_SUB)
							break;
					}

					// Checkable item?
					if (exclude_total[level] & (1 << count))
					{
						// Store mutual exclude value
						new_menu[num].nm_MutualExclude = exclude_total[level];

						// Clear the menu's own bit
						new_menu[num].nm_MutualExclude &= ~(1 << count);
					}
				}

				// Clear exclude
				exclude_total[level] = 0;
				--level;
			}
		}

		// Break if this was the last
		if (data->type == NM_END)
			break;

		// Normal item?
		if (data->type == NM_ITEM)
		{
			// New item level?
			if (level == -1)
			{
				// Initialise item level
				level_start[0] = count;
				exclude_total[0] = 0;
				level_value[0] = 0;
				level = 0;
			}
		}

		// Checkable item?
		if ((data->type == NM_ITEM || data->type == NM_SUB) && data->flags & MENUFLAG_AUTO_MUTEX)
		{
			// Update exclude value
			exclude_total[level] |= 1 << level_value[level];
		}

		// Increment level value
		if (level > -1)
			++level_value[level];

		// Check label for special values
		if (data->name == NM_BAR_LABEL)
			new_menu[count].nm_Label = NM_BARLABEL;

		// Otherwise, see if it's valid
		else if (data->name)
		{
			// See if it's a real string or a locale string
			if (data->flags & MENUFLAG_TEXT_STRING)
				new_menu[count].nm_Label = (char *)data->name;
			else
				new_menu[count].nm_Label = L_GetString(locale, data->name);
		}

		// Command sequence supplied?
		if (data->flags & MENUFLAG_USE_SEQ)
		{
			char key;

			// Get key sequence
			key = MENUFLAG_GET_SEQ(data->flags);

			// Create sequence string
#ifdef __AROS__
			{
				char *tmp = L_AllocMemH(memory, 2);
				if (tmp)
				{
					tmp[0] = key;
					new_menu[count].nm_CommKey = tmp;
				}
			}
#else
			if ((new_menu[count].nm_CommKey = (char *)L_AllocMemH(memory, 2)))
			{
				/* this stops the compiler complaining about read-only locations */
				STRPTR ck = (STRPTR)new_menu[count].nm_CommKey;
				ck[0] = key;
			}
#endif
		}

		// Automatic command sequence?
		else if (data->flags & MENUFLAG_COMM_SEQ)
		{
			short a, twice, k;
			char key, menkey;

			// Go through label twice
			for (twice = 0; twice < 2; twice++)
			{
				for ((k = 0); (menkey = new_menu[count].nm_Label[k]); k++)
				{
					// First iteration only looks at uppercase letters
					if (twice == 0 && (menkey < 'A' || menkey > 'Z'))
						continue;

					// Get key to try
					key = (menkey >= 'a' && menkey <= 'z') ? menkey - ('a' - 'A') : menkey;

					// Go through all menus up to this one
					for (a = 0; a < count; a++)
					{
						// Is this key used?
						if (new_menu[a].nm_CommKey && new_menu[a].nm_CommKey[0] == key)
							break;
					}

					// Ok to use this key?
					if (a == count)
					{
						// Allocate buffer for key
#ifdef __AROS__
						{
							char *tmp = L_AllocMemH(memory, 2);
							if (tmp)
							{
								tmp[0] = key;
								new_menu[count].nm_CommKey = tmp;
							}
						}
#else
						if ((new_menu[count].nm_CommKey = (char *)L_AllocMemH(memory, 2)))
						{
							/* this stops the compiler complaining about read-only locations */
							STRPTR ck = (STRPTR)new_menu[count].nm_CommKey;
							ck[0] = key;
						}
#endif
						break;
					}
				}

				// Got one?
				if (new_menu[count].nm_CommKey)
					break;
			}
		}

		// Copy flags (lower word)
		new_menu[count].nm_Flags = (UWORD)data->flags;

		// Increment count
		++count;
		++data;
	}

	// Create menu strip
	menustrip = CreateMenusA(new_menu, 0);

	// Free memory
	L_FreeMemHandle(memory);

	return menustrip;
}

// Find a menu by userdata ID
struct MenuItem *LIBFUNC L_FindMenuItem(REG(a0, struct Menu *menu), REG(d0, UWORD id))
{
	if (!menu)
		return 0;

	// Go through all menus
	for (; menu; menu = menu->NextMenu)
	{
		struct MenuItem *item;

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
