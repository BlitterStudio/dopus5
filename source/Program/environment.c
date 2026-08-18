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

#include "dopus.h"
#include "scripts.h"

#include <libraries/multiuser.h>
#include <proto/configopus.h>
#include <proto/multiuser.h>

static int write_env_string(APTR, char *, ULONG);

// Internal flag for environment_save: filter out paths recorded in
// env_drop_banks instead of writing them back to the layout. Only
// honoured when ENVSAVE_LAYOUT is unset (the dropped-paths filter
// lives inside the non-snapshot bank loop; an ENVSAVE_LAYOUT save
// writes the live GUI state, where missing entries are already
// absent).
#define ENVSAVE_DROP_MISSING_BANKS (1 << 2)

// Module-private bookkeeping for the "remove missing button banks"
// feature, split into two lists:
//
//  - env_missing_banks  filled by environment_open when a BANK/STRT
//                       entry's file is gone. Owned by the most recent
//                       open; cleared at the start of every load and
//                       drained by environment_resolve_missing_banks.
//
//  - env_drop_banks     populated by environment_resolve_missing_banks
//                       from the user's "Remove" answers, then handed
//                       to environment_save with the
//                       ENVSAVE_DROP_MISSING_BANKS flag so the rewritten
//                       env file no longer carries those entries.
//
// Both lists key paths as the raw `ButtonBankNode->ln_Name` from the
// env file (not the resolved path produced by
// environment_fix_open_button_path) so environment_save can match
// the exact string it re-reads from disk. Both lists are cleared at
// the start of every environment_open / environment_resolve_missing_banks
// pair so a save failure for layout A never bleeds into layout B.
// Kind of entry an EnvBankPathNode refers to, so the resolve prompt
// can use the right wording.
#define ENV_KIND_BANK 0	   // BANK chunk - normal button bank
#define ENV_KIND_START 1   // STRT chunk - start menu

struct EnvBankPathNode
{
	struct MinNode node;
	UBYTE kind;
	char path[256];
};

// Both lists are owned by the most recent environment_open / paired
// environment_resolve_missing_banks call. They are first touched on
// the main task during early startup (main.c calls environment_open
// before environment_proc is spawned), so the lazy init below does
// not need a lock.
static struct MinList env_missing_banks;
static struct MinList env_drop_banks;
static BOOL env_bank_lists_inited = FALSE;

static void env_bank_lists_ensure_init(void)
{
	if (!env_bank_lists_inited)
	{
		NewList((struct List *)&env_missing_banks);
		NewList((struct List *)&env_drop_banks);
		env_bank_lists_inited = TRUE;
	}
}

static void env_bank_list_add(struct MinList *list, const char *path, UBYTE kind)
{
	struct EnvBankPathNode *node;

	env_bank_lists_ensure_init();

	if (!path || !path[0])
		return;

	if ((node = AllocVec(sizeof(struct EnvBankPathNode), MEMF_CLEAR)))
	{
		node->kind = kind;
		stccpy(node->path, (char *)path, sizeof(node->path));
		AddTail((struct List *)list, (struct Node *)node);
	}
}

static void env_bank_list_clear(struct MinList *list)
{
	struct EnvBankPathNode *node;

	env_bank_lists_ensure_init();

	while ((node = (struct EnvBankPathNode *)RemHead((struct List *)list)))
		FreeVec(node);
}

static BOOL env_bank_list_empty(struct MinList *list)
{
	env_bank_lists_ensure_init();
	return IsListEmpty((struct List *)list);
}

static BOOL env_drop_banks_contains(const char *path)
{
	struct EnvBankPathNode *node;

	env_bank_lists_ensure_init();

	if (!path)
		return FALSE;

	for (node = (struct EnvBankPathNode *)env_drop_banks.mlh_Head; node->node.mln_Succ;
		 node = (struct EnvBankPathNode *)node->node.mln_Succ)
	{
		if (stricmp(node->path, (char *)path) == 0)
			return TRUE;
	}

	return FALSE;
}

// Allocate a new environment structure
Cfg_Environment *environment_new(void)
{
	Cfg_Environment *env;

	// Allocate new environment structure
	if (!(env = AllocVec(sizeof(Cfg_Environment), MEMF_CLEAR)) || !(env->memory = NewMemHandle(0, 0, MEMF_CLEAR)) ||
		!(env->desktop_memory = NewMemHandle(1024, 512, MEMF_PUBLIC | MEMF_CLEAR)) ||
		!(env->volatile_memory = NewMemHandle(1024, sizeof(ButtonBankNode) + 256, MEMF_PUBLIC | MEMF_CLEAR)))
	{
		environment_free(env);
		return 0;
	}

	// Initialise environment
	InitSemaphore(&env->lock);
	NewList((struct List *)&env->path_formats);
	NewList((struct List *)&env->path_list);
	NewList((struct List *)&env->sound_list);
	strcpy(env->toolbar_path, "PROGDIR:Buttons/Toolbar");
	strcpy(env->menu_path, "PROGDIR:Buttons/Lister Menu");
	strcpy(env->user_menu_path, "PROGDIR:Buttons/User Menu");
	strcpy(env->hotkeys_path, "PROGDIR:Buttons/Hotkeys");
	strcpy(env->scripts_path, "PROGDIR:Buttons/Scripts");
	NewList((struct List *)&env->desktop);
	InitSemaphore(&env->desktop_lock);
	InitSemaphore(&env->sound_lock);

	// Allocate space for settings
	if (!(env->env = AllocMemH(env->memory, sizeof(CFG_ENVR))))
	{
		environment_free(env);
		return 0;
	}

	// Get default settings
	DefaultEnvironment(env->env);
	return env;
}

// Free an environment
void environment_free(Cfg_Environment *env)
{
	if (env)
	{
		env_free_desktop(&env->desktop);
		env_free_desktop(&env->path_list);
		env_free_desktop(&env->sound_list);
		FreeMemHandle(env->memory);
		FreeMemHandle(env->volatile_memory);
		FreeMemHandle(env->desktop_memory);
		FreeVec(env);
	}
}

#define CONFIG_MAGIC 0xFACE
#define NEW_CONFIG_VERSION 10016

static BOOL environment_path_exists(char *path)
{
	BPTR lock;

	if (!path || !path[0])
		return 0;

	if ((lock = Lock(path, ACCESS_READ)))
	{
		UnLock(lock);
		return 1;
	}

	return 0;
}

static char *environment_file_part(char *path)
{
	char *ptr, *part;

	if (!path)
		return 0;

	part = path;
	for (ptr = path; *ptr; ptr++)
	{
		if (*ptr == ':' || *ptr == '/' || *ptr == '\\')
			part = ptr + 1;
	}

	return part;
}

static BOOL environment_try_button_path(char *path, char *drawer, char *name)
{
	char buf[256];

	if (!name || !name[0])
		return FALSE;

	strcpy(buf, drawer);
	if (strlen(buf) + strlen(name) >= sizeof(buf))
		return FALSE;

	strcat(buf, name);
	if (!environment_path_exists(buf))
		return FALSE;

	strcpy(path, buf);
	return TRUE;
}

/* Legacy underscored alias for default names that contain a space, so users
 * upgrading from open-source builds shipped between 2013 and 5.99 (which had
 * `lister_menu` / `user_menu` files in basedata.lha) still get their existing
 * files picked up by default if they haven't extracted the new archive.
 * AmigaDOS is case-insensitive, so `Toolbar` / `Hotkeys` / `Scripts` already
 * match the legacy `toolbar` / `hotkeys` / `scripts` files without help. */
static const char *environment_legacy_alias(const char *default_name)
{
	if (!default_name)
		return NULL;
	if (stricmp(default_name, "Lister Menu") == 0)
		return "lister_menu";
	if (stricmp(default_name, "User Menu") == 0)
		return "user_menu";
	return NULL;
}

static void environment_fix_button_path(char *path, char *default_name)
{
	const char *legacy;
	char *name;

	if (path && strnicmp(path, "DOpus5:Buttons/", 15) == 0)
		environment_try_button_path(path, "PROGDIR:Buttons/", FilePart(path));

	if (environment_path_exists(path))
		return;

	name = (path && path[0]) ? environment_file_part(path) : default_name;
	if (!name || !name[0])
		name = default_name;

	if (environment_try_button_path(path, "PROGDIR:Buttons/", name) ||
		environment_try_button_path(path, "DOpus5:Buttons/", name))
		return;

	if (stricmp(name, default_name) != 0)
	{
		if (environment_try_button_path(path, "PROGDIR:Buttons/", default_name) ||
			environment_try_button_path(path, "DOpus5:Buttons/", default_name))
			return;
	}

	legacy = environment_legacy_alias(default_name);
	if (legacy && stricmp(name, legacy) != 0)
	{
		if (environment_try_button_path(path, "PROGDIR:Buttons/", (char *)legacy) ||
			environment_try_button_path(path, "DOpus5:Buttons/", (char *)legacy))
			return;
	}
}

static void environment_fix_button_paths(Cfg_Environment *env)
{
	environment_fix_button_path(env->toolbar_path, "Toolbar");
	environment_fix_button_path(env->menu_path, "Lister Menu");
	environment_fix_button_path(env->user_menu_path, "User Menu");
	environment_fix_button_path(env->scripts_path, "Scripts");
	environment_fix_button_path(env->hotkeys_path, "Hotkeys");
}

static void environment_fix_open_button_path(char *path)
{
	if (!path || !path[0])
		return;

	if (strnicmp(path, "DOpus5:Buttons/", 15) == 0)
	{
		char *name = FilePart(path);

		if (name && name[0])
			environment_try_button_path(path, "PROGDIR:Buttons/", name);
	}
}

static BOOL environment_minlist_empty(struct MinList *list)
{
	return (BOOL)(list->mlh_Head == (struct MinNode *)&list->mlh_Tail);
}

static ToolBarInfo *environment_open_toolbar(Cfg_Environment *env)
{
	ToolBarInfo *toolbar;
	/* AmigaDOS is case-insensitive, so "Toolbar" also matches the legacy
	 * "toolbar" file. The .default fallback is shipped alongside Toolbar
	 * in basedata.lha so a clean install always recovers if the user
	 * trashes their saved Toolbar. */
	char *fallbacks[] = {"PROGDIR:Buttons/Toolbar",
						 "PROGDIR:Buttons/Toolbar.default",
						 "PROGDIR:Buttons/toolbar_default",
						 "DOpus5:Buttons/Toolbar",
						 "DOpus5:Buttons/Toolbar.default",
						 "DOpus5:Buttons/toolbar_default",
						 0};
	short a;

	if (env->toolbar_path[0] && (toolbar = OpenToolBar(0, env->toolbar_path)))
		return toolbar;

	for (a = 0; fallbacks[a]; a++)
	{
		if (env->toolbar_path[0] && stricmp(env->toolbar_path, fallbacks[a]) == 0)
			continue;

		if ((toolbar = OpenToolBar(0, fallbacks[a])))
		{
			strcpy(env->toolbar_path, fallbacks[a]);
			return toolbar;
		}
	}

	return 0;
}

#ifdef __AROS__
static BOOL environment_is_factory_path(Cfg_Environment *env, char *name, BOOL first)
{
	char *path, *part;

	path = (name && name[0]) ? name : env->path;
	if (!path || !path[0])
		return TRUE;

	part = environment_file_part(path);
	return (BOOL)(stricmp(part, "default") == 0 || stricmp(part, "workbench") == 0);
}

static BOOL environment_has_old_aros_default_look(CFG_ENVR *cfg)
{
	if (!(cfg->display_options & DISPOPTF_NO_BACKDROP) || (cfg->display_options & DISPOPTF_ICON_POS) ||
		(cfg->display_options & DISPOPTF_REALTIME_SCROLL) || (cfg->display_options & DISPOPTF_THIN_BORDERS) ||
		(cfg->env_flags & ENVF_BACKDROP) || (cfg->desktop_flags & DESKTOPF_NO_BORDERS) ||
		(cfg->lister_options & (LISTEROPTF_2XCLICK | LISTEROPTF_TITLES | LISTEROPTF_POPUP | LISTEROPTF_EDIT_BOTH)))
		return FALSE;

	return (BOOL)(cfg->source_col[0] == 2 && cfg->source_col[1] == 3 && cfg->dest_col[0] == 1 &&
				  cfg->dest_col[1] == 0 && cfg->devices_col[0] == 1 && cfg->devices_col[1] == 0 &&
				  cfg->volumes_col[0] == 3 && cfg->volumes_col[1] == 0);
}

static BOOL environment_has_old_aros_workbench_look(CFG_ENVR *cfg)
{
	return (BOOL)((cfg->display_options == 0 || cfg->display_options == DISPOPTF_USE_WBPATTERN) &&
				  cfg->env_flags == 0 && cfg->desktop_flags == 0 && cfg->lister_options == 0);
}

static void environment_harmonise_default_look(Cfg_Environment *env, char *name, BOOL first)
{
	CFG_ENVR *cfg;

	if (!environment_is_factory_path(env, name, first))
		return;

	cfg = env->env;
	if (!environment_has_old_aros_default_look(cfg) && !environment_has_old_aros_workbench_look(cfg) &&
		(cfg->display_options & DISPOPTF_ICON_POS) && (cfg->display_options & DISPOPTF_REALTIME_SCROLL) &&
		(cfg->display_options & DISPOPTF_THIN_BORDERS) && (cfg->env_flags & ENVF_BACKDROP) &&
		(cfg->desktop_flags & DESKTOPF_NO_BORDERS) &&
		((cfg->lister_options & (LISTEROPTF_2XCLICK | LISTEROPTF_TITLES | LISTEROPTF_POPUP | LISTEROPTF_EDIT_BOTH)) ==
		 (LISTEROPTF_2XCLICK | LISTEROPTF_TITLES | LISTEROPTF_POPUP | LISTEROPTF_EDIT_BOTH)))
		return;

	cfg->display_options &= ~DISPOPTF_NO_BACKDROP;
	cfg->display_options |= DISPOPTF_SHOW_APPICONS | DISPOPTF_SHOW_TOOLS | DISPOPTF_ICON_POS |
							DISPOPTF_USE_WBPATTERN | DISPOPTF_REALTIME_SCROLL | DISPOPTF_THIN_BORDERS;
	cfg->env_flags |= ENVF_BACKDROP;
	cfg->desktop_flags |= DESKTOPF_NO_BORDERS;
	cfg->lister_options |= LISTEROPTF_2XCLICK | LISTEROPTF_TITLES | LISTEROPTF_POPUP | LISTEROPTF_EDIT_BOTH;

	cfg->source_col[0] = 1;
	cfg->source_col[1] = 253;
	cfg->dest_col[0] = 2;
	cfg->dest_col[1] = 252;
	cfg->devices_col[0] = 1;
	cfg->devices_col[1] = 2;
	cfg->volumes_col[0] = 3;
	cfg->volumes_col[1] = 2;
}

#endif

// Read an environment
BOOL environment_open(Cfg_Environment *env, char *name, BOOL first, APTR prog)
{
	struct OpenEnvironmentData *opendata;
	BOOL success;
	short progress = 1;

	// Discard any unresolved missing-bank state from a previous load
	// so it can never be applied to a different env file. Both lists
	// are owned by this open / its paired environment_resolve_missing_banks
	// call.
	env_bank_list_clear(&env_missing_banks);
	env_bank_list_clear(&env_drop_banks);

	// Free volatile memory
	ClearMemHandle(env->volatile_memory);

	// Initialise open structure
	if (!(opendata = AllocMemH(env->volatile_memory, sizeof(struct OpenEnvironmentData))))
		return 0;
	opendata->memory = env->desktop_memory;
	opendata->volatile_memory = env->volatile_memory;
	opendata->flags = OEDF_ALL;

	// Initialise progress
	SetProgressWindowTags(prog, PW_FileCount, 14, PW_FileNum, 1, TAG_END);

	// Read environment
	if ((success = OpenEnvironment((name) ? name : env->path, opendata)))
	{
		// Check stack setting and increase if too low
		if (opendata->env.default_stack < STACK_DEFAULT)
			opendata->env.default_stack = STACK_DEFAULT;

		// Copy things back to the environment
		CopyMem((char *)&opendata->env, (char *)env->env, sizeof(CFG_ENVR));
		strcpy(env->toolbar_path, opendata->toolbar_path);
		strcpy(env->menu_path, opendata->menu_path);
		strcpy(env->user_menu_path, opendata->user_menu_path);
		strcpy(env->scripts_path, opendata->scripts_path);
		strcpy(env->hotkeys_path, opendata->hotkeys_path);

#ifdef __AROS__
		environment_harmonise_default_look(env, name, first);
#endif

		// Get maximum filename length
		// we have to do this before the listers are opened
		GUI->def_filename_length = env->env->settings.max_filename;
		if (GUI->def_filename_length < FILENAME_LEN)
			GUI->def_filename_length = FILENAME_LEN;
		else if (GUI->def_filename_length > MAX_FILENAME_LEN)
			GUI->def_filename_length = MAX_FILENAME_LEN;
	}
	// Successful?
	if (success || first)
	{
		ButtonBankNode *button;
		OpenListerNode *lister;

		// Bump progress
		main_bump_progress(prog, progress++, 0);

		// Store new environment path
		if (name)
			strcpy(env->path, name);

		// Use installed defaults if an environment has stale or missing button paths
		environment_fix_button_paths(env);

		// Bump progress
		main_bump_progress(prog, progress++, 0);

		// Get new toolbar
		FreeToolBar(GUI->toolbar);
		GUI->toolbar = environment_open_toolbar(env);

		// Bump progress
		main_bump_progress(prog, progress++, 0);

		// Free lister menu, get new one
		CloseButtonBank(GUI->lister_menu);
		if ((GUI->lister_menu = OpenButtonBank(env->menu_path)))
		{
			// Check if it needs conversion
			if (!(GUI->lister_menu->window.flags & BTNWF_FIX_MENU))
			{
				// Convert it to new format
				ConvertStartMenu(GUI->lister_menu);
				GUI->lister_menu->window.flags |= BTNWF_FIX_MENU;
			}
		}

		// Bump progress
		main_bump_progress(prog, progress++, 0);

		// Free user menu, get new one
		CloseButtonBank(GUI->user_menu);
		GUI->user_menu = OpenButtonBank(env->user_menu_path);

		// Bump progress
		main_bump_progress(prog, progress++, 0);

		// Free scripts, get new set
		CloseButtonBank(GUI->scripts);
		GUI->scripts = OpenButtonBank(env->scripts_path);

		// Bump progress
		main_bump_progress(prog, progress++, 0);

		// Free hotkeys, get new ones
		send_main_reset_cmd(CONFIG_CHANGE_HOTKEYS, 0, 0);

		// Go through buttons to open
		for (button = (ButtonBankNode *)opendata->buttons.mlh_Head; button->node.ln_Succ;)
		{
			ButtonBankNode *next = (ButtonBankNode *)button->node.ln_Succ;
			Buttons *but;
			char button_path[256];

			stccpy(button_path, button->node.ln_Name, sizeof(button_path));
			environment_fix_open_button_path(button_path);

			// File still on disk? Try to open the bank.
			if (environment_path_exists(button_path))
			{
				// Create button bank from this node
				if ((but = buttons_new(button_path, 0, &button->pos, 0, button->flags | BUTTONF_FAIL)))
				{
					// Set icon position
					but->icon_pos_x = button->icon_pos_x;
					but->icon_pos_y = button->icon_pos_y;
				}
			}
			else
			{
				// The bank file is gone. Record the original
				// path; environment_resolve_missing_banks will
				// prompt the user once the display is up - the
				// SimpleRequest at this point in startup would
				// land on Workbench (no DOpus screen yet) and
				// is easy to miss.
				env_bank_list_add(&env_missing_banks, button->node.ln_Name, ENV_KIND_BANK);
			}

			// Free this node, get next
			Remove((struct Node *)button);
			FreeMemH(button->node.ln_Name);
			FreeMemH(button);
			button = next;
		}

		// Bump progress
		main_bump_progress(prog, progress++, 0);

		// Go through StartMenus to open
		for (button = (ButtonBankNode *)opendata->startmenus.mlh_Head; button->node.ln_Succ;)
		{
			ButtonBankNode *next = (ButtonBankNode *)button->node.ln_Succ;
			char button_path[256];

			stccpy(button_path, button->node.ln_Name, sizeof(button_path));
			environment_fix_open_button_path(button_path);

			if (environment_path_exists(button_path))
			{
				// Create new start menu
				start_new(button_path, 0, 0, button->pos.Left, button->pos.Top);
			}
			else
			{
				// Same deferred handling as button banks above.
				env_bank_list_add(&env_missing_banks, button->node.ln_Name, ENV_KIND_START);
			}

			// Free this node, get next
			Remove((struct Node *)button);
			FreeMemH(button->node.ln_Name);
			FreeMemH(button);
			button = next;
		}

		// Bump progress
		main_bump_progress(prog, progress++, 0);

		// Go through listers to open
		for (lister = (OpenListerNode *)opendata->listers.mlh_Head; lister->node.ln_Succ;)
		{
			OpenListerNode *next = (OpenListerNode *)lister->node.ln_Succ;

			// Create lister from this node
			if (lister->lister)
				lister_new((Cfg_Lister *)lister->lister);

			// Free this node, get next
			Remove((struct Node *)lister);
			FreeMemH(lister);
			lister = next;
		}

		// Bump progress
		main_bump_progress(prog, progress++, 0);

		// Free existing desktop list
		env_free_desktop(&env->desktop);

		// Copy new desktop into list
		if (!environment_minlist_empty(&opendata->desktop))
		{
			env->desktop = opendata->desktop;
			env->desktop.mlh_TailPred->mln_Succ = (struct MinNode *)&env->desktop.mlh_Tail;
			env->desktop.mlh_Head->mln_Pred = (struct MinNode *)&env->desktop.mlh_Head;
		}

		// Free existing path list
		env_free_desktop(&env->path_list);

		// Copy new pathlist into list
		if (!environment_minlist_empty(&opendata->pathlist))
		{
			env->path_list = opendata->pathlist;
			env->path_list.mlh_TailPred->mln_Succ = (struct MinNode *)&env->path_list.mlh_Tail;
			env->path_list.mlh_Head->mln_Pred = (struct MinNode *)&env->path_list.mlh_Head;
		}

		// Free existing sound list
		env_free_desktop(&env->sound_list);

		// Copy new sound list into list
		if (!environment_minlist_empty(&opendata->soundlist))
		{
			env->sound_list = opendata->soundlist;
			env->sound_list.mlh_TailPred->mln_Succ = (struct MinNode *)&env->sound_list.mlh_Tail;
			env->sound_list.mlh_Head->mln_Pred = (struct MinNode *)&env->sound_list.mlh_Head;
		}

		// Bump progress
		main_bump_progress(prog, progress++, 0);

		// Update priority
		IPC_Command(
			&main_ipc, IPC_PRIORITY, env->env->settings.pri_main[1], (APTR)(IPTR)env->env->settings.pri_main[0], 0, 0);

		// Fix lister priorities
		lister_fix_priority(0);

		// Bump progress
		main_bump_progress(prog, progress++, 0);
	}

	// Failed, free temp lists
	else
	{
		env_free_desktop(&opendata->desktop);
		env_free_desktop(&opendata->pathlist);
		env_free_desktop(&opendata->soundlist);
		env_free_desktop(&opendata->startmenus);
		env_free_desktop(&opendata->buttons);
		env_free_desktop(&opendata->listers);
	}

	// Free open data structure
	FreeMemH(opendata);

	// Bump progress
	main_bump_progress(prog, progress, 0);

	// Build the user menu (needed even if there is none)
	display_build_user_menu();

	// Initialise progress
	SetProgressWindowTags(prog, PW_FileCount, 1, PW_FileNum, 1, TAG_END);

	// Set library flag for 'Move AppIcons to Tools Menu'
	SetLibraryFlags((env->env->display_options & DISPOPTF_SHIFT_APPICONS) ? LIBDF_REDIRECT_TOOLS : 0,
					LIBDF_REDIRECT_TOOLS);

	// Set library flag for borderless icons
	SetLibraryFlags((env->env->desktop_flags & DESKTOPF_NO_BORDERS) ? LIBDF_BORDERS_OFF : 0, LIBDF_BORDERS_OFF);

	// No icon caching?
	SetLibraryFlags((env->env->desktop_flags & DESKTOPF_NO_CACHE) ? LIBDF_NO_CACHING : 0, LIBDF_NO_CACHING);

	// Set NewIcons flags
	SetNewIconsFlags(env->env->env_NewIconsFlags, env->env->env_NewIconsPrecision);

	// No custom drag?
	SetLibraryFlags((env->env->desktop_flags & DESKTOPF_NO_CUSTOMDRAG) ? LIBDF_NO_CUSTOM_DRAG : 0,
					LIBDF_NO_CUSTOM_DRAG);

	// Thin borders?
	SetLibraryFlags((env->env->display_options & DISPOPTF_THIN_BORDERS) ? LIBDF_THIN_BORDERS : 0, LIBDF_THIN_BORDERS);

	// Update pathlist environment variable
	env_update_pathlist();

	// Set popup delay
	SetPopUpDelay(env->env->settings.popup_delay);

	// Fix MUFS library
	env_fix_mufs();

	// Assign themes path
	env_fix_themes();

	// Initialise sound events
	InitSoundEvents(TRUE);

	// Any missing BANK/STRT entries that were collected above are
	// drained later by environment_resolve_missing_banks once the
	// caller has a usable parent screen for the requesters. We do
	// nothing else here: a startup-time SimpleRequest with no DOpus
	// screen yet was unreliable in earlier builds.
	return success;
}

// Prompt the user about each BANK/STRT entry that environment_open
// found missing on disk, and rewrite the env file without the ones
// they chose to remove. Caller is responsible for invoking this at a
// point where requesters can be displayed (i.e. after the DOpus
// screen is open). Calling it when there is nothing to resolve is
// cheap.
//
// `parent` is the window the requester anchors to. NULL falls back to
// the default public screen, which is fine for the layout-load path
// where the DOpus window is up; for the very first startup load the
// caller passes GUI->window once the display is initialised.
//
// We deliberately don't clear env_missing_banks at entry: the whole
// point of this function is to drain the list environment_open just
// populated. env_drop_banks, on the other hand, is cleared up-front
// so a previous load whose follow-up save failed cannot bleed its
// removals into a different env file.
void environment_resolve_missing_banks(Cfg_Environment *env, struct Window *parent)
{
	struct EnvBankPathNode *node;

	env_bank_lists_ensure_init();

	env_bank_list_clear(&env_drop_banks);

	if (env_bank_list_empty(&env_missing_banks))
		return;

	// Drain the missing list into a Remove/Keep prompt per entry.
	while ((node = (struct EnvBankPathNode *)RemHead((struct List *)&env_missing_banks)))
	{
		const char *fmt = (node->kind == ENV_KIND_START)
							  ? "Start menu '%s'\ncould not be opened.\n"
								"Remove from environment?"
							  : "Button bank '%s'\ncould not be opened.\n"
								"Remove from environment?";

		if (SimpleRequestTags(parent,
							  dopus_name,
							  (char *)"Remove|Keep",
							  (char *)fmt,
							  (IPTR)node->path))
		{
			env_bank_list_add(&env_drop_banks, node->path, node->kind);
		}

		FreeVec(node);
	}

	// Persist the chosen removals. On save failure leave the drop
	// list empty anyway: the env file is unchanged so the next load
	// will detect the same entries again and re-prompt.
	if (!env_bank_list_empty(&env_drop_banks))
		environment_save(env, env->path, ENVSAVE_DROP_MISSING_BANKS, 0);

	env_bank_list_clear(&env_drop_banks);
}

// Save an environment
int environment_save(Cfg_Environment *env, char *name, short snapshot, CFG_ENVR *data)
{
	APTR iff;
	long success = 0;
	struct OpenEnvironmentData *opendata = 0;
	BOOL old_env_valid = FALSE;
#ifdef __AROS__
	CFG_ENVR *env_be;
#endif

	// Invalid name?
	if (!name || !*name)
		return 0;

	// Get data pointer if not supplied
	if (!data)
		data = env->env;

#ifdef __AROS__
	if (!(env_be = AllocVec(sizeof(CFG_ENVR), MEMF_CLEAR)))
		return 0;
#endif

	// Update main window position
	if (snapshot & ENVSAVE_WINDOW)
	{
		GetSemaphore(&GUI->backdrop->window_lock, SEMF_SHARED, 0);
		display_store_pos();
		FreeSemaphore(&GUI->backdrop->window_lock);
	}

	// Not snapshotting listers?
	if (!(snapshot & ENVSAVE_LAYOUT))
	{
		if ((opendata = AllocMemH(env->volatile_memory, sizeof(struct OpenEnvironmentData))))
		{
			opendata->memory = env->desktop_memory;
			opendata->volatile_memory = env->volatile_memory;
			opendata->flags = OEDF_ENVR | OEDF_SETS | OEDF_BANK | OEDF_LSTR;
			old_env_valid = OpenEnvironment(name, opendata);
		}
	}

	// Try to open file to write
	while ((iff = IFFOpen(name, MODE_NEWFILE, ID_EPUS)))
	{
		IPCData *ipc;
		Lister *lister;
		Buttons *buttons;
		Cfg_Desktop *desk;
		struct MinNode *node;
		Cfg_SoundEntry *sound;

		// Write environment settings
#ifdef __AROS__
		CopyMem(data, env_be, sizeof(CFG_ENVR));

		{
			int i;

			env_be->screen_mode = AROS_LONG2BE(env_be->screen_mode);
			env_be->screen_flags = AROS_WORD2BE(env_be->screen_flags);
			env_be->screen_depth = AROS_WORD2BE(env_be->screen_depth);
			env_be->screen_width = AROS_WORD2BE(env_be->screen_width);
			env_be->screen_height = AROS_WORD2BE(env_be->screen_height);

			for (i = 0; i < 50; i++)
				env_be->palette[i] = AROS_LONG2BE(env_be->palette[i]);

			env_be->window_pos.Left = AROS_WORD2BE(env_be->window_pos.Left);
			env_be->window_pos.Top = AROS_WORD2BE(env_be->window_pos.Top);
			env_be->window_pos.Width = AROS_WORD2BE(env_be->window_pos.Width);
			env_be->window_pos.Height = AROS_WORD2BE(env_be->window_pos.Height);
			env_be->general_screen_flags = AROS_LONG2BE(env_be->general_screen_flags);
			env_be->palette_count = AROS_WORD2BE(env_be->palette_count);

			for (i = 0; i < CUST_PENS; i++)
			{
				env_be->env_Colours[i][0][0] = AROS_LONG2BE(env_be->env_Colours[i][0][0]);
				env_be->env_Colours[i][0][1] = AROS_LONG2BE(env_be->env_Colours[i][0][1]);
				env_be->env_Colours[i][0][2] = AROS_LONG2BE(env_be->env_Colours[i][0][2]);
				env_be->env_Colours[i][1][0] = AROS_LONG2BE(env_be->env_Colours[i][1][0]);
				env_be->env_Colours[i][1][1] = AROS_LONG2BE(env_be->env_Colours[i][1][1]);
				env_be->env_Colours[i][1][2] = AROS_LONG2BE(env_be->env_Colours[i][1][2]);
			}

			env_be->env_ColourFlag = AROS_LONG2BE(env_be->env_ColourFlag);
			env_be->env_NewIconsFlags = AROS_LONG2BE(env_be->env_NewIconsFlags);
			env_be->display_options = AROS_WORD2BE(env_be->display_options);
			env_be->main_window_type = AROS_WORD2BE(env_be->main_window_type);
			env_be->hotkey_flags = AROS_LONG2BE(env_be->hotkey_flags);
			env_be->hotkey_code = AROS_WORD2BE(env_be->hotkey_code);
			env_be->hotkey_qual = AROS_WORD2BE(env_be->hotkey_qual);
			env_be->default_stack = AROS_LONG2BE(env_be->default_stack);
			env_be->lister_options = AROS_WORD2BE(env_be->lister_options);
			env_be->flags = AROS_LONG2BE(env_be->flags);
			env_be->lister_popup_code = AROS_WORD2BE(env_be->lister_popup_code);
			env_be->lister_popup_qual = AROS_WORD2BE(env_be->lister_popup_qual);
			env_be->env_flags = AROS_LONG2BE(env_be->env_flags);
			env_be->clock_left = AROS_WORD2BE(env_be->clock_left);
			env_be->clock_top = AROS_WORD2BE(env_be->clock_top);
			env_be->lister_width = AROS_WORD2BE(env_be->lister_width);
			env_be->lister_height = AROS_WORD2BE(env_be->lister_height);
			env_be->version = AROS_WORD2BE(env_be->version);
			env_be->desktop_flags = AROS_LONG2BE(env_be->desktop_flags);

			// Promoted ENV: vars from issue #49 (CONFIG_VERSION_14/15) -- byte-swap so
			// configs are portable between AROS and big-endian targets.  env_pad_15
			// gets the swap proactively: it's currently always zero (byte-swap
			// invariant), but if anyone repurposes the slot for a real UWORD value
			// the swap is already in place rather than waiting for a corruption bug.
			env_be->env_icon_space_x = AROS_WORD2BE(env_be->env_icon_space_x);
			env_be->env_icon_space_y = AROS_WORD2BE(env_be->env_icon_space_y);
			env_be->env_icon_grid_x = AROS_WORD2BE(env_be->env_icon_grid_x);
			env_be->env_icon_grid_y = AROS_WORD2BE(env_be->env_icon_grid_y);
			env_be->env_wheel_scroll_lines = AROS_WORD2BE(env_be->env_wheel_scroll_lines);
			env_be->env_pad_15 = AROS_WORD2BE(env_be->env_pad_15);

			for (i = 0; i < 4; i++)
				env_be->env_BackgroundFlags[i] = AROS_WORD2BE(env_be->env_BackgroundFlags[i]);

			env_be->settings.copy_flags = AROS_LONG2BE(env_be->settings.copy_flags);
			env_be->settings.delete_flags = AROS_LONG2BE(env_be->settings.delete_flags);
			env_be->settings.error_flags = AROS_LONG2BE(env_be->settings.error_flags);
			env_be->settings.general_flags = AROS_LONG2BE(env_be->settings.general_flags);
			env_be->settings.icon_flags = AROS_LONG2BE(env_be->settings.icon_flags);
			env_be->settings.replace_method = AROS_WORD2BE(env_be->settings.replace_method);
			env_be->settings.replace_flags = AROS_WORD2BE(env_be->settings.replace_flags);
			env_be->settings.update_flags = AROS_LONG2BE(env_be->settings.update_flags);
			env_be->settings.dir_flags = AROS_LONG2BE(env_be->settings.dir_flags);
			env_be->settings.view_flags = AROS_LONG2BE(env_be->settings.view_flags);
			env_be->settings.max_buffer_count = AROS_WORD2BE(env_be->settings.max_buffer_count);
			env_be->settings.date_format = AROS_WORD2BE(env_be->settings.date_format);
			env_be->settings.date_flags = AROS_WORD2BE(env_be->settings.date_flags);
			env_be->settings.pop_code = AROS_WORD2BE(env_be->settings.pop_code);
			env_be->settings.pop_qual = AROS_WORD2BE(env_be->settings.pop_qual);
			env_be->settings.pop_qual_mask = AROS_WORD2BE(env_be->settings.pop_qual_mask);
			env_be->settings.pop_qual_same = AROS_WORD2BE(env_be->settings.pop_qual_same);
			env_be->settings.popup_delay = AROS_WORD2BE(env_be->settings.popup_delay);
			env_be->settings.max_openwith = AROS_WORD2BE(env_be->settings.max_openwith);
			env_be->settings.command_line_length = AROS_WORD2BE(env_be->settings.command_line_length);
			env_be->settings.max_filename = AROS_WORD2BE(env_be->settings.max_filename);
			env_be->settings.flags = AROS_LONG2BE(env_be->settings.flags);

			for (i = 0; i < 4; i++)
				env_be->env_BackgroundBorderColour[i] = AROS_LONG2BE(env_be->env_BackgroundBorderColour[i]);
		}

		if (!(IFFWriteChunk(iff, env_be, ID_ENVR, sizeof(CFG_ENVR))))
#else
		if (!(IFFWriteChunk(iff, data, ID_ENVR, sizeof(CFG_ENVR))))
#endif
		{
			success = IoErr();
			break;
		}

		// Write some path names
		if ((success = write_env_string(iff, env->toolbar_path, ID_TBAR)) ||
			(success = write_env_string(iff, env->menu_path, ID_LMEN)) ||
			(success = write_env_string(iff, env->user_menu_path, ID_UMEN)) ||
			(success = write_env_string(iff, env->scripts_path, ID_SCRP)) ||
			(success = write_env_string(iff, env->hotkeys_path, ID_HKEY)))
			break;

		// Not snapshotting?
		if (!(snapshot & ENVSAVE_LAYOUT) && opendata)
		{
			ButtonBankNode *button;
			OpenListerNode *lister;

			// Go through existing listers
			for (lister = (OpenListerNode *)opendata->listers.mlh_Head; lister->node.ln_Succ;)
			{
				OpenListerNode *next = (OpenListerNode *)lister->node.ln_Succ;
				Cfg_Lister *cfg_lister = (Cfg_Lister *)lister->lister;

				if (old_env_valid && CompareListFormat(&cfg_lister->lister.format, &opendata->env.list_format) == 0)
					cfg_lister->lister.format = data->list_format;

				// Write lister data
				if (!(SaveListerDef(iff, cfg_lister)))
					break;

				// Free lister data
				FreeListerDef((Cfg_Lister *)lister->lister);

				// Remove this node, get next
				Remove((struct Node *)lister);
				FreeMemH(lister);
				lister = next;
			}

			// Go through buttons to open
			for (button = (ButtonBankNode *)opendata->buttons.mlh_Head; button->node.ln_Succ;)
			{
				ButtonBankNode *next = (ButtonBankNode *)button->node.ln_Succ;
				ULONG pad[5];
#ifdef __AROS__
				struct IBox pos_be;
#endif

				// Caller asked us to drop banks the user marked as
				// missing during the most recent environment_open.
				if ((snapshot & ENVSAVE_DROP_MISSING_BANKS) &&
					env_drop_banks_contains(button->node.ln_Name))
				{
					Remove((struct Node *)button);
					button = next;
					continue;
				}

				// Write bank header
				if (!(IFFPushChunk(iff, ID_BANK)))
					break;

				// Fill out padding
				pad[0] = 0;
				pad[1] = 0;
				pad[2] = button->icon_pos_x;
				pad[3] = button->icon_pos_y;
				pad[4] = button->flags & BUTTONF_ICONIFIED;

#ifdef __AROS__
				CopyMem(&button->pos, &pos_be, sizeof(struct IBox));

				pos_be.Left = AROS_WORD2BE(pos_be.Left);
				pos_be.Top = AROS_WORD2BE(pos_be.Top);
				pos_be.Width = AROS_WORD2BE(pos_be.Width);
				pos_be.Height = AROS_WORD2BE(pos_be.Height);

				pad[2] = AROS_LONG2BE(pad[2]);
				pad[3] = AROS_LONG2BE(pad[3]);
				pad[4] = AROS_LONG2BE(pad[4]);

				if (!(IFFWriteChunkBytes(iff, pad, sizeof(pad))) ||
					!(IFFWriteChunkBytes(iff, &pos_be, sizeof(struct IBox))) ||
					!(IFFWriteChunkBytes(iff, button->node.ln_Name, strlen(button->node.ln_Name) + 1)) ||
					!(IFFPopChunk(iff)))
					break;
#else
				// Write padding and position and path
				if (!(IFFWriteChunkBytes(iff, pad, sizeof(pad))) ||
					!(IFFWriteChunkBytes(iff, &button->pos, sizeof(struct IBox))) ||
					!(IFFWriteChunkBytes(iff, button->node.ln_Name, strlen(button->node.ln_Name) + 1)) ||
					!(IFFPopChunk(iff)))
					break;
#endif

				// Remove this node, get next
				Remove((struct Node *)button);
				button = next;
			}

			// Go through StartMenus to open
			for (button = (ButtonBankNode *)opendata->startmenus.mlh_Head; button->node.ln_Succ;)
			{
				ButtonBankNode *next = (ButtonBankNode *)button->node.ln_Succ;
				ULONG pad[5];
#ifdef __AROS__
				struct IBox pos_be;
#endif

				// Same skip logic as the BANK loop above.
				if ((snapshot & ENVSAVE_DROP_MISSING_BANKS) &&
					env_drop_banks_contains(button->node.ln_Name))
				{
					Remove((struct Node *)button);
					button = next;
					continue;
				}

				// Write bank header
				if (!(IFFPushChunk(iff, ID_STRT)))
					break;

				// Fill out padding
				pad[0] = 0;
				pad[1] = 0;
				pad[2] = button->icon_pos_x;
				pad[3] = button->icon_pos_y;
				pad[4] = button->flags;

#ifdef __AROS__
				CopyMem(&button->pos, &pos_be, sizeof(struct IBox));

				pos_be.Left = AROS_WORD2BE(pos_be.Left);
				pos_be.Top = AROS_WORD2BE(pos_be.Top);
				pos_be.Width = AROS_WORD2BE(pos_be.Width);
				pos_be.Height = AROS_WORD2BE(pos_be.Height);

				pad[2] = AROS_LONG2BE(pad[2]);
				pad[3] = AROS_LONG2BE(pad[3]);
				pad[4] = AROS_LONG2BE(pad[4]);

				if (!(IFFWriteChunkBytes(iff, pad, sizeof(pad))) ||
					!(IFFWriteChunkBytes(iff, &pos_be, sizeof(struct IBox))) ||
					!(IFFWriteChunkBytes(iff, button->node.ln_Name, strlen(button->node.ln_Name) + 1)) ||
					!(IFFPopChunk(iff)))
					break;
#else
				// Write padding and position and path
				if (!(IFFWriteChunkBytes(iff, pad, sizeof(pad))) ||
					!(IFFWriteChunkBytes(iff, &button->pos, sizeof(struct IBox))) ||
					!(IFFWriteChunkBytes(iff, button->node.ln_Name, strlen(button->node.ln_Name) + 1)) ||
					!(IFFPopChunk(iff)))
					break;
#endif

				// Remove this node, get next
				Remove((struct Node *)button);
				button = next;
			}
		}

		// Otherwise, snapshotting current layout
		else
		{
			// Lock lister list
			lock_listlock(&GUI->lister_list, FALSE);

			// Write lister definitions
			for (ipc = (IPCData *)GUI->lister_list.list.lh_Head; ipc->node.mln_Succ;
				 ipc = (IPCData *)ipc->node.mln_Succ)
			{
				// Get lister
				lister = IPCDATA(ipc);

				// Valid lister definition?
				if (lister->lister)
				{
					// Update lister
					lister_update(lister);

					// Write lister data
					if (!(SaveListerDef(iff, lister->lister)))
						break;
				}
			}

			// Unlock lister list
			unlock_listlock(&GUI->lister_list);

			// Fail?
			if (ipc->node.mln_Succ)
			{
				success = IoErr();
				break;
			}

			// Lock buttons list
			lock_listlock(&GUI->buttons_list, FALSE);

			// Write button file paths
			for (ipc = (IPCData *)GUI->buttons_list.list.lh_Head; ipc->node.mln_Succ;
				 ipc = (IPCData *)ipc->node.mln_Succ)
			{
				// Get buttons pointer
				buttons = IPCDATA(ipc);

				// Valid button configuration?
				if (buttons->bank && buttons->bank->path[0])
				{
					ULONG pad[5];
#ifdef __AROS__
					struct IBox pos_be;
#endif

					// Update buttons
					buttons_update(buttons);

					// Write bank header
					if (!(IFFPushChunk(iff, ID_BANK)))
						break;

					// Fill out padding
					pad[0] = 0;
					pad[1] = 0;
					pad[2] = buttons->icon_pos_x;
					pad[3] = buttons->icon_pos_y;
					pad[4] = buttons->flags & BUTTONF_ICONIFIED;

#ifdef __AROS__
					CopyMem(&buttons->pos, &pos_be, sizeof(struct IBox));

					pos_be.Left = AROS_WORD2BE(pos_be.Left);
					pos_be.Top = AROS_WORD2BE(pos_be.Top);
					pos_be.Width = AROS_WORD2BE(pos_be.Width);
					pos_be.Height = AROS_WORD2BE(pos_be.Height);

					pad[2] = AROS_LONG2BE(pad[2]);
					pad[3] = AROS_LONG2BE(pad[3]);
					pad[4] = AROS_LONG2BE(pad[4]);

					if (!(IFFWriteChunkBytes(iff, pad, sizeof(pad))) ||
						!(IFFWriteChunkBytes(iff, &pos_be, sizeof(struct IBox))) ||
						!(IFFWriteChunkBytes(iff, buttons->bank->path, strlen(buttons->bank->path) + 1)) ||
						!(IFFPopChunk(iff)))
						break;
#else
					// Write padding and position and path
					if (!(IFFWriteChunkBytes(iff, pad, sizeof(pad))) ||
						!(IFFWriteChunkBytes(iff, &buttons->pos, sizeof(struct IBox))) ||
						!(IFFWriteChunkBytes(iff, buttons->bank->path, strlen(buttons->bank->path) + 1)) ||
						!(IFFPopChunk(iff)))
						break;
#endif
				}
			}

			// Unlock buttons list
			unlock_listlock(&GUI->buttons_list);

			// Fail?
			if (ipc->node.mln_Succ)
			{
				success = IoErr();
				break;
			}

			// Lock StartMenu list
			lock_listlock(&GUI->startmenu_list, FALSE);

			// Write StartMenu paths
			for (ipc = (IPCData *)GUI->startmenu_list.list.lh_Head; ipc->node.mln_Succ;
				 ipc = (IPCData *)ipc->node.mln_Succ)
			{
				StartMenu *menu;

				// Get menu pointer
				menu = (StartMenu *)IPCDATA(ipc);

				// Lock the bank
				GetSemaphore(&menu->lock, SEMF_SHARED, 0);

				// Valid configuration?
				if (menu->bank && menu->bank->path[0])
				{
					ULONG pad[5];
					struct IBox pos;

					// Get position
					pos.Left = menu->bank->window.pos.Left;
					pos.Top = menu->bank->window.pos.Top;
#ifdef __AROS__
					pos.Left = AROS_WORD2BE(pos.Left);
					pos.Top = AROS_WORD2BE(pos.Top);
#endif

					// Write menu header
					if (!(IFFPushChunk(iff, ID_STRT)))
						break;

					// Fill out padding
					pad[0] = 0;
					pad[1] = 0;
					pad[2] = 0;
					pad[3] = 0;
					pad[4] = 0;

					// Write padding and position and path
					if (!(IFFWriteChunkBytes(iff, pad, sizeof(pad))) ||
						!(IFFWriteChunkBytes(iff, &pos, sizeof(struct IBox))) ||
						!(IFFWriteChunkBytes(iff, menu->bank->path, strlen(menu->bank->path) + 1)) ||
						!(IFFPopChunk(iff)))
						break;
				}

				// Unlock bank
				FreeSemaphore(&menu->lock);
			}

			// Unlock startmenu list
			unlock_listlock(&GUI->startmenu_list);

			// Fail?
			if (ipc->node.mln_Succ)
				success = IoErr();
		}

		// Failed?
		if (success)
			break;

		// Go through desktop list
		for (desk = (Cfg_Desktop *)env->desktop.mlh_Head; desk->node.mln_Succ;
			 desk = (Cfg_Desktop *)desk->node.mln_Succ)
		{
#ifdef __AROS__
			char buffer[64];  // 12 bytes CFG_DESK + 34 bytes device name + 18 bytes extra
			CFG_DESK *desk_be = (CFG_DESK *)buffer;

			CopyMem(&desk->data, desk_be, desk->data.dt_Size);

			desk_be->dt_Type = AROS_WORD2BE(desk_be->dt_Type);
			desk_be->dt_Size = AROS_WORD2BE(desk_be->dt_Size);
			desk_be->dt_Flags = AROS_LONG2BE(desk_be->dt_Flags);
			if (desk->data.dt_Type != DESKTOP_HIDE_BAD && desk->data.dt_Type != DESKTOP_HIDE)
				desk_be->dt_Data = AROS_LONG2BE(desk_be->dt_Data);

			if (!(IFFWriteChunk(iff, desk_be, ID_DESK, desk->data.dt_Size)))
#else
			// Write chunk
			if (!(IFFWriteChunk(iff, &desk->data, ID_DESK, desk->data.dt_Size)))
#endif
			{
				success = IoErr();
				break;
			}
		}

		// Go through pathlist
		for (node = env->path_list.mlh_Head; node->mln_Succ; node = node->mln_Succ)
		{
			// Write chunk
			if (!(IFFWriteChunk(iff, (char *)(node + 1), ID_PATH, strlen((char *)(node + 1)))))
			{
				success = IoErr();
				break;
			}
		}

		// Go through sound list
		for (sound = (Cfg_SoundEntry *)env->sound_list.mlh_Head; sound->dse_Node.ln_Succ;
			 sound = (Cfg_SoundEntry *)sound->dse_Node.ln_Succ)
		{
#ifdef __AROS__
			Cfg_SoundEntry sound_be;

			CopyMem(sound, &sound_be, sizeof(Cfg_SoundEntry));

			sound_be.dse_Volume = AROS_WORD2BE(sound_be.dse_Volume);
			sound_be.dse_Flags = AROS_WORD2BE(sound_be.dse_Flags);

			if (!(IFFWriteChunk(iff, sound_be.dse_Name, ID_SNDX, 32 + 2 + 2 + strlen(sound->dse_Sound) + 1)))
#else
			// Write chunk
			if (!(IFFWriteChunk(iff, sound->dse_Name, ID_SNDX, 32 + 2 + 2 + strlen(sound->dse_Sound) + 1)))
#endif
			{
				success = IoErr();
				break;
			}
		}
		break;
	}

	// Close file
	if (!iff)
		success = IoErr();
	else
		IFFClose(iff);

#ifdef __AROS__
	FreeVec(env_be);
#endif

	// Write icon if successful (and enabled)
	if ((!success) && (GUI->flags & GUIF_SAVE_ICONS))
	{
		WriteFileIcon("DOpus5:Icons/Environment", name);
	}

	// Free stuff
	FreeMemH(opendata);
	ClearMemHandle(env->volatile_memory);
	return success;
}

static int write_env_string(APTR iff, char *str, ULONG id)
{
	if (*str && !(IFFWriteChunk(iff, str, id, strlen(str) + 1)))
		return IoErr();

	return 0;
}

// Environment handler
IPC_EntryCode(environment_proc)
{
#ifndef __amigaos3__
	struct Library *ConfigOpusBase;
	#ifdef __amigaos4__
	struct ConfigOpusIFace *IConfigOpus;
	#endif
#endif
	IPCData *ipc;
	env_packet *packet = 0;
	char *path;
	struct Window *status = 0;
	ULONG change, change_flags[2] = {0, 0};
	BOOL reopen = 0, old = 0, need_reset = 0;
	short save_layout = 0;
	BPTR file;
	APTR progress = 0;

	// Do startup
	if ((ipc = IPC_ProcStartup((IPTR *)&packet, 0)) && (path = AllocVec(256, 0)))
	{
		// Get name, either from packet or current env name
		strcpy(path, (packet->type == -1) ? packet->name : environment->path);

		// Save layout?
		if (packet->type == MENU_SAVE_LAYOUT)
			save_layout = ENVSAVE_LAYOUT | ENVSAVE_WINDOW;

		// What operation?
		switch (packet->type)
		{
		// Open
		case MENU_OPEN_ENVIRONMENT:

			// Get name
			if (!(request_file(GUI->window,
							   GetString(&locale, MSG_ENVIRONMENT_SELECT_FILE),
							   path,
							   "DOpus5:Environment/",
							   FRF_PRIVATEIDCMP,
							   0)))
				break;

		// Name supplied to load environment
		case -1:

			// Open file
			if ((file = Open(path, MODE_OLDFILE)))
			{
				UWORD test[2];

				// Read first word
				if (Read(file, (char *)test, sizeof(test)) == sizeof(test))
				{
					// Old style configuration?
					if (test[0] >= NEW_CONFIG_VERSION && test[1] == CONFIG_MAGIC)
					{
						old = 1;
					}
				}
				Close(file);
			}

			// Converting old configuration?
			if (old)
			{
				BOOL ok = 0;

				// Open config library
				if ((ConfigOpusBase = OpenModule(config_name)))
				{
#ifdef __amigaos4__
					if ((IConfigOpus = (struct ConfigOpusIFace *)GetInterface(ConfigOpusBase, "main", 1, NULL)))
#endif
						ok = ConvertConfig(path, GUI->screen_pointer, &main_ipc);

#ifdef __amigaos4__
					DropInterface((struct Interface *)IConfigOpus);
#endif
					CloseLibrary(ConfigOpusBase);
				}
				if (!ok)
					break;
			}

			// Close display
			IPC_Command(&main_ipc, MAINCMD_CLOSE_DISPLAY, TRUE, 0, 0, (struct MsgPort *)-1);

			// Open progress indicator
			progress = OpenProgressWindowTags(PW_Title,
											  (IPTR)dopus_name,
											  PW_Info,
											  (IPTR)GetString(&locale, MSG_ENVIRONMENT_LOADING),
											  PW_Flags,
											  PWF_INFO | PWF_GRAPH,
											  TAG_END);

			// Read environment
			environment_open(environment, path, 0, progress);

			// Open display
			reopen = 1;
			break;

		// Edit
		case MENU_EDIT_ENVIRONMENT:

			// Open config library
			if ((ConfigOpusBase = OpenModule(config_name))
#ifdef __amigaos4__
				&& (IConfigOpus = (struct ConfigOpusIFace *)GetInterface(ConfigOpusBase, "main", 1, NULL))
#endif
			)
			{
				char edit_path[256];

				// Configure environment
				strcpy(edit_path, environment->path);
				if ((change = Config_Environment(environment,
												 GUI->screen_pointer,
												 GUI->pens,
												 ipc,
												 &main_ipc,
												 (UWORD)(GUI->pen_alloc & 0xff),
												 change_flags,
												 edit_path,
												 script_list)))
				{
					// Save path
					strcpy(environment->path, edit_path);
					strcpy(path, edit_path);

					// Update priority
					IPC_Command(&main_ipc,
								IPC_PRIORITY,
								environment->env->settings.pri_main[1],
								(APTR)(IPTR)environment->env->settings.pri_main[0],
								0,
								0);

					// Fix lister priorities
					lister_fix_priority(0);
				}
#ifdef __amigaos4__
				DropInterface((struct Interface *)IConfigOpus);
#endif
				CloseLibrary(ConfigOpusBase);

				// Reset needed?
				if (change & CONFIG_NEED_RESET)
					need_reset = 1;

				// Don't save?
				if (!(change & CONFIG_SAVE))
					break;

				// Don't save layout
				save_layout = ENVSAVE_WINDOW;
			}
			else
			{
				CloseLibrary(ConfigOpusBase);  // In case module opens but interface doesn't
				break;
			}

		// Fall through to...
		// Save
		case MENU_SAVE_ENVIRONMENT:
		case MENU_SAVE_LAYOUT:

			// Try to save with current name
			if (path[0])
			{
				short err;

				// Open status
				status = OpenStatusWindow(
					0, GetString(&locale, MSG_ENVIRONMENT_SAVING), GUI->screen_pointer, 0, WINDOW_NO_CLOSE);

				// Save file
				if ((err = environment_save(environment, path, save_layout, 0)))
				{
					error_saving(err, status, MSG_ERROR_SAVING_ENV, 0);
				}
				break;
			}

		// Fall through to...
		// Save as
		case MENU_SAVEAS_ENVIRONMENT:

			// Loop until successful
			while (1)
			{
				short err;

				// Get new name
				if (!(request_file(GUI->window,
								   GetString(&locale, MSG_ENVIRONMENT_ENTER_NAME),
								   path,
								   "DOpus5:Environment/",
								   FRF_DOSAVEMODE | FRF_PRIVATEIDCMP,
								   0)))
					break;

				// Open status
				if (!status)
					status = OpenStatusWindow(
						0, GetString(&locale, MSG_ENVIRONMENT_SAVING), GUI->screen_pointer, 0, WINDOW_NO_CLOSE);

				// Try to save with new name
				if ((err = environment_save(environment, path, save_layout, 0)) == 0)
				{
					strcpy(environment->path, path);
					break;
				}

				// Show error
				if (!(error_saving(err, status, MSG_ERROR_SAVING_ENV, 1)))
					break;
			}
			break;
		}

		// Free path
		FreeVec(path);
	}

	// Close status window
	CloseConfigWindow(status);

	// Close progress indicator
	if (progress)
		CloseProgressWindow(progress);

	// Need a reset?
	if (need_reset)
	{
		// Say that we need to reset
		AsyncRequestTags(ipc,
						 REQTYPE_SIMPLE,
						 0,
						 0,
						 0,
						 AR_Screen,
						 (IPTR)GUI->screen_pointer,
						 AR_Title,
						 (IPTR)dopus_name,
						 AR_Message,
						 (IPTR)GetString(&locale, MSG_CHANGE_NEEDS_RESET),
						 AR_Button,
						 (IPTR)GetString(&locale, MSG_OKAY),
						 TAG_END);
	}

	// Send goodbye
	IPC_Goodbye(ipc, &main_ipc, 0);

	// Send change/reopen status to main process. The OPEN_DISPLAY
	// command is synchronous (matches the CLOSE_DISPLAY at the top
	// of this branch) so a back-to-back layout load cannot start
	// another environment_open while the main task still has this
	// load's missing-bank list to drain in
	// environment_resolve_missing_banks.
	if (reopen)
		IPC_Command(&main_ipc, MAINCMD_OPEN_DISPLAY, DSPOPENF_DESKTOP, 0, 0, (struct MsgPort *)-1);
	else if (change_flags[0] || change_flags[1])
		send_main_reset_cmd(change_flags[0], change_flags[1], 0);

	// Free startup packet
	FreeVec(packet);

	// Exit
	IPC_Free(ipc);
}

// Free a desktop list
void env_free_desktop(struct MinList *list)
{
	struct MinNode *node, *next;

	// Go through list
	for (node = list->mlh_Head; node->mln_Succ; node = next)
	{
		// Get next
		next = node->mln_Succ;

		// Free node
		FreeMemH(node);
	}

	// Initialise list
	NewList((struct List *)list);
}

// Update pathlist
void env_update_pathlist()
{
	// Path list use allowed?
	if (environment->env->env_flags & ENVF_USE_PATHLIST)
	{
		APTR file;
		char *path;
		// short num;

		// Build environment variables
		// for (num=0,path="env:dopus/paths";num<2;num++,path="envarc:dopus/paths")
		path = "env:dopus/paths";
		{
			if ((file = OpenBuf(path, MODE_NEWFILE, 4096)))
			{
				struct MinNode *node;
				if (environment_minlist_empty(&environment->path_list))
					WriteBuf(file, "c:\n", 3);
				else
					for (node = environment->path_list.mlh_Head; node->mln_Succ; node = node->mln_Succ)
					{
						WriteBuf(file, (char *)(node + 1), -1);
						WriteBuf(file, "\n", 1);
					}
				CloseBuf(file);
			}
		}
	}

	// Remove environment variable
	else
	{
		DeleteFile("env:dopus/paths");
		// DeleteFile("envarc:dopus/paths");
	}

	// Update library path list
	UpdatePathList();
}

// Open/close MUFS library
void env_fix_mufs()
{
	if (environment->env->settings.general_flags & GENERALF_ENABLE_MUFS)
	{
		if (!muBase)
		{
#ifdef __amigaos4__
			OpenLibIFace("multiuser.library", (APTR)&muBase, (APTR)&Imu, 39);
#else
			muBase = OpenLibrary("multiuser.library", 39);
#endif
		}
	}
	else if (muBase)
	{
#ifdef __amigaos4__
		DropInterface((struct Interface *)Imu);
#endif
		CloseLibrary(muBase);
		muBase = 0;
	}
}

// Assign the themes directory
void env_fix_themes()
{
	BPTR lock;
	if (!environment->env->themes_location[0])
		strcpy(environment->env->themes_location, "DOpus5:Themes/");
	if ((lock = Lock(environment->env->themes_location, ACCESS_READ)) && !AssignLock("D5THEMES", lock))
		UnLock(lock);
}
