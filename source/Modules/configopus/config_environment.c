#include "config_lib.h"
#include "config_environment.h"
#include <proto/module.h>

//#define FUNKOFF

#define NUM_CHANGE_FLAGS 2

void funced_edit_insertstring(ObjectList *, ULONG, char *string, struct Library *, struct Library *);
BOOL config_env_popkey(config_env_data *);
void config_env_load(config_env_data *, UWORD);
BOOL config_env_save(config_env_data *, short);
char *strstri(char *string, char *substring);

unsigned long LIBFUNC L_Config_Environment(REG(a0, Cfg_Environment *env),
										   REG(a1, struct Screen *screen),
										   REG(a2, UWORD *pen_table),
										   REG(a3, IPCData *ipc),
                                           REG(a4, IPCData *main_ipc),
                                           REG(d0, UWORD pen_alloc),
                                           REG(d1, ULONG *change_flags),
                                           REG(d2, char *settings_name),
                                           REG(d3, Att_List *script_list))
{
	config_env_data *data;
	IPCMessage *quit_msg = 0;
	unsigned long success = CONFIG_OK;
	short a;

	// Clear change flags
	change_flags[0] = 0;
	change_flags[1] = 0;

	// Valid config?
	if (!env)
		return 0;

	// Allocate data and backup configuration
	if (!(data = AllocVec(sizeof(config_env_data), MEMF_CLEAR)) ||
		!(data->config = AllocVec(sizeof(CFG_ENVR), MEMF_CLEAR)))
	{
		FreeVec(data);
		return 0;
	}

	// Initialise
	data->custom_pen[0] = -1;
	data->custom_pen[1] = -1;
	NewList((struct List *)&data->sound_list);

	// Create application port
	data->app_port = CreateMsgPort();
	data->ipc = ipc;
	data->main_ipc = main_ipc;
	data->settings_name = settings_name;
	data->script_list = script_list;

	// Store pathname
	strcpy(data->last_saved, settings_name);

	// Copy configuration
	CopyMem((char *)env->env, (char *)data->config, sizeof(CFG_ENVR));
	data->env = env;

	// Snapshot the dopus/DOSPatch ENVARC: var so the Lister Display
	// checkbox reflects the current state.  The variable is read by
	// libinit.c at dopus5.library open time (see LIBDF_DOS_PATCH); the
	// GUI commits any change back to ENV: + ENVARC: in the success block
	// below.  initial+state both start at the loaded value; modified is
	// flipped by _config_env_store when the new state diverges from
	// initial (so toggle-on-then-off doesn't dirty the var pointlessly).
	{
		char buf[1];
		BOOL set = (GetVar("dopus/DOSPatch", buf, sizeof(buf), GVF_GLOBAL_ONLY) > -1);
		data->dos_patch_initial = set;
		data->dos_patch_state = set;
		data->dos_patch_modified = FALSE;
	}

	// Initial palette table
	data->palette_table = pen_table;
	data->initial_pen_alloc = pen_alloc;
	data->option = -1;

	// Fix font pen table
	_config_env_fix_font_pens(data);

	// Store graphics library
	// data->GfxBase=(struct Library *)GfxBase;

	// Build sub-option list, open display
	if (!(data->options = build_sub_options(_environment_options)) || !(_config_env_open(data, screen)))
	{
		_config_env_cleanup(data);
		return 0;
	}

#ifndef FUNKOFF
	// Allocate font requesters
	data->font_req = AllocAslRequestTags(ASL_FontRequest,
										 ASLFO_TitleText,
										 GetString(locale, MSG_LISTER_SELECT_FONT),
										 ASLFO_Flags,
										 FOF_PRIVATEIDCMP | FOF_FIXEDWIDTHONLY,
										 ASLFO_MaxHeight,
										 96,
										 TAG_END);
	data->icon_font_req = AllocAslRequestTags(ASL_FontRequest,
											  (screen) ? ASLFO_InitialHeight : TAG_IGNORE,
											  (screen) ? (screen->Height >> 3) + (screen->Height >> 1) : 0,
											  ASLFO_MaxHeight,
											  96,
											  TAG_END);

	// Build screen mode list
	if ((data->mode_list = Att_NewList(LISTF_POOL)))
	{
		ULONG mode_id;
		struct NameInfo mode_name;
		struct DimensionInfo mode_dims;
		struct List *pubscreen_list;
		struct PubScreenNode *pubscreen;

		// Go through available screen modes
		mode_id = INVALID_ID;
		while ((mode_id = NextDisplayInfo(mode_id)) != INVALID_ID)
		{
			// Check that mode is available
			if (!(ModeNotAvailable(mode_id)) &&
				(GetDisplayInfoData(0, (char *)&mode_name, sizeof(mode_name), DTAG_NAME, mode_id)) &&
				(GetDisplayInfoData(0, (char *)&mode_dims, sizeof(mode_dims), DTAG_DIMS, mode_id)))
			{
				// Check that this is a "hires" mode
				if (mode_dims.Nominal.MaxX >= 639 && mode_dims.Nominal.MaxY >= 199)
				{
					// Add to list
					Att_NewNode(data->mode_list, mode_name.Name, mode_id, ADDNODE_SORT | ADDNODE_EXCLUSIVE);
				}
			}
		}

		// Get public screen list
		if ((pubscreen_list = LockPubScreenList()))
		{
			// Go through public screens
			for (pubscreen = (struct PubScreenNode *)pubscreen_list->lh_Head; pubscreen->psn_Node.ln_Succ;
				 pubscreen = (struct PubScreenNode *)pubscreen->psn_Node.ln_Succ)
			{
				// Check that this isn't Workbench or DOpus, and is public
				if (strcmp(pubscreen->psn_Node.ln_Name, "Workbench") != 0 &&
					strncmp(pubscreen->psn_Node.ln_Name, "DOPUS", 5) != 0 && !(pubscreen->psn_Flags & PSNF_PRIVATE))
				{
					// Build entry name
					char name_buf[128];
					size_t len;

					stccpy(name_buf, pubscreen->psn_Node.ln_Name, sizeof(name_buf));
					len = strlen(name_buf);
					if (len < sizeof(name_buf) - 1)
					{
						name_buf[len++] = ':';
						name_buf[len] = 0;
						stccpy(name_buf + len, GetString(locale, MSG_ENVIRONMENT_USE), sizeof(name_buf) - len);
					}

					// Add node
					Att_NewNode(data->mode_list, name_buf, MODE_PUBLICSCREEN, ADDNODE_SORT | ADDNODE_EXCLUSIVE);
				}
			}

			// Free public screen list
			UnlockPubScreenList();
		}

		// Add Workbench Use/Clone
		Att_NewNode(
			data->mode_list, GetString(locale, MSG_ENVIRONMENT_SCREENMODE_WB_USE), MODE_WORKBENCHUSE, ADDNODE_SORT);

		Att_NewNode(
			data->mode_list, GetString(locale, MSG_ENVIRONMENT_SCREENMODE_WB_CLONE), MODE_WORKBENCHCLONE, ADDNODE_SORT);
	}

	// Build lister display items list
	if ((data->lister_items = Att_NewList(0)))
	{
		short a;

		for (a = MSG_ENVIRONMENT_LISTER_FILES; a <= MSG_ENVIRONMENT_LISTER_VOLUMES; a++)
		{
			Att_NewNode(data->lister_items, GetString(locale, a), a, 0);
		}
		Att_NewNode(data->lister_items, GetString(locale, MSG_ENVIRONMENT_GAUGE_COLOUR), a, 0);
		Att_NewNode(data->lister_items, GetString(locale, MSG_ENVIRONMENT_TOOLTIP_COLOUR), a + 1, 0);
	}

	// Build icon settings list
	if ((data->icon_settings = Att_NewList(0)))
	{
		short a;
		for (a = 0; _environment_iconsettings_labels[a]; a++)
		{
			char buf[40], *ptr;
			strcpy(buf, GetString(locale, _environment_iconsettings_labels[a]));
			for (ptr = buf; *ptr; ptr++)
			{
				if (*ptr == '_')
				{
					strcpy(ptr, ptr + 1);
					break;
				}
			}
			Att_NewNode(data->icon_settings, buf, a, ADDNODE_SORT);
		}
	}

	// Build path list
	if ((data->path_list = Att_NewList(0)))
	{
		struct MinNode *node;
		for (node = env->path_list.mlh_Head; node->mln_Succ; node = node->mln_Succ)
			Att_NewNode(data->path_list, (char *)(node + 1), 0, 0);
	}

	// Build sound list
	env_init_sounds(data);

#endif

	// Event loop
	FOREVER
	{
		struct IntuiMessage *msg;
		IPCMessage *imsg;
		int quit_flag = 0;

		// Any IPC messages?
		while ((imsg = (IPCMessage *)GetMsg(ipc->command_port)))
		{
			// Look at command
			switch (imsg->command)
			{
			// Quit immediately
			case IPC_QUIT:
				quit_flag = 1;
				quit_msg = imsg;
				imsg = 0;
				success = CONFIG_NO_CHANGE;
				break;

			// Hide
			case IPC_HIDE:

				// Close display
				_config_env_close(data);
				break;

			// Show
			case IPC_SHOW:

				// Show display
				if (!data->window)
				{
					if (!(_config_env_open(data, (struct Screen *)imsg->data)))
					{
						quit_flag = 1;
						success = CONFIG_NO_CHANGE;
					}
				}
				break;

			// Activate
			case IPC_ACTIVATE:
				if (data->window)
				{
					WindowToFront(data->window);
					ActivateWindow(data->window);
				}
				break;
			}

			// Reply message
			IPC_Reply(imsg);
		}

		// Appwindow?
		if (data->appwindow)
		{
			struct AppMessage *appmsg;
			BOOL ok = 0, gotmsg = 0;

			// Get messages
			while ((appmsg = (struct AppMessage *)GetMsg(data->app_port)))
			{
				short num;

				gotmsg = 1;

				// In palette mode?
				if (data->option == ENVIRONMENT_PALETTE)
				{
					// Go through arguments
					for (num = 0; num < appmsg->am_NumArgs; num++)
					{
						// Valid name?
						if (appmsg->am_ArgList[num].wa_Name && *appmsg->am_ArgList[num].wa_Name)
						{
							char name[256];

							// Get full name
							GetWBArgPath(&appmsg->am_ArgList[num], name, 256);

							// Try and get palette
							if (get_palette_file(data, name))
							{
								// Update sliders
								_config_env_palette_update_sliders(data);
								ok = 1;
								break;
							}
						}
					}
				}

				// In backgrounds mode
				else if (data->option == ENVIRONMENT_PICTURES)
				{
					GL_Object *obj;
					if (((obj = GetObject(data->option_list, GAD_ENVIRONMENT_PICTURE_DESK_FIELD)) &&
						 CheckObjectArea(obj, appmsg->am_MouseX, appmsg->am_MouseY)) ||
						((obj = GetObject(data->option_list, GAD_ENVIRONMENT_PICTURE_LISTER_FIELD)) &&
						 CheckObjectArea(obj, appmsg->am_MouseX, appmsg->am_MouseY)) ||
						((obj = GetObject(data->option_list, GAD_ENVIRONMENT_PICTURE_REQ_FIELD)) &&
						 CheckObjectArea(obj, appmsg->am_MouseX, appmsg->am_MouseY)))
					{
						char name[256];

						// Get full name and store in field
						GetWBArgPath(appmsg->am_ArgList, name, 256);
						SetGadgetValue(data->option_list, obj->id, (IPTR)name);
						ok = 1;
					}
				}

				// Sound mode
				else if (data->option == ENVIRONMENT_SOUNDLIST)
				{
					if (GetGadgetValue(data->option_list, GAD_SETTINGS_SOUNDLIST) > -1)
					{
						char name[256];

						// Get full name and store in field
						GetWBArgPath(appmsg->am_ArgList, name, 256);
						SetGadgetValue(data->option_list, GAD_SETTINGS_SOUNDLIST_PATH, (IPTR)name);
						ok = 1;
					}
				}

				// Reply the message
				ReplyMsg((struct Message *)appmsg);
			}

			// Beep if error
			if (gotmsg && !ok)
				DisplayBeep(data->window->WScreen);
		}

		// Window open?
		if (data->window)
		{
			// Intuition messages
			while ((msg = GetWindowMsg(data->window->UserPort)))
			{
				struct IntuiMessage msg_copy;
				struct Gadget *gadget;
				UWORD gadgetid = 0;

				// Copy message and reply
				msg_copy = *msg;
				if (msg->Class != IDCMP_IDCMPUPDATE)
				{
					ReplyWindowMsg(msg);
					msg = 0;
				}

				// Look at message
				switch (msg_copy.Class)
				{
				// Close window
				case IDCMP_CLOSEWINDOW:
					success = CONFIG_NO_CHANGE;
					quit_flag = 1;
					break;

				// BOOPSI message
				case IDCMP_IDCMPUPDATE: {
					struct TagItem *tags = (struct TagItem *)msg_copy.IAddress;

					// Path lister?
					if (GetTagData(GA_ID, 0, tags) == GAD_SETTINGS_PATHLIST)
					{
						short item;

						// Get item
						if ((item = GetTagData(DLV_DragNotify, -1, tags)) != -1)
						{
							// Start the drag
							config_drag_start(&data->drag, data->path_list, item, tags, FALSE);
						}
					}

					// Sound listview
					else if (GetTagData(GA_ID, 0, tags) == GAD_SETTINGS_SOUNDLIST)
					{
						// Double-click on sound listview edits entry
						if (GetTagData(DLV_DoubleClick, 0, tags))
						{
							struct IntuiMessage msg;
							msg.Class = IDCMP_GADGETUP;
							msg.IAddress = GADGET(GetObject(data->option_list, GAD_SETTINGS_SOUNDLIST_POPUP));
							StoreGadgetValue(data->option_list, &msg);
							config_env_store_sound(data);
						}
					}
				}
				break;

				// Menu
				case IDCMP_MENUPICK: {
					struct MenuItem *item;

					// Get item
					if (!(item = ItemAddress(data->window->MenuStrip, msg_copy.Code)))
						break;

					// Treat as a gadget
					gadgetid = MENUID(item);
				}

				// Gadget
				case IDCMP_GADGETUP:
				case IDCMP_GADGETDOWN:

					// Get GadgetID
					if (msg_copy.Class != IDCMP_MENUPICK)
					{
						gadget = (struct Gadget *)msg_copy.IAddress;
						gadgetid = gadget->GadgetID;
					}

					// Look at gadget
					switch (gadgetid)
					{
					// New option
					case GAD_ENVIRONMENT_ITEM_LISTER:

						// Get selected entry
						data->option_node = Att_FindNode(data->options, msg_copy.Code);

						// Same as last selection?
						if (data->option == _environment_options[data->option_node->data].num)
							break;

						// See if we have an option already
						if (data->option_list)
						{
#ifndef FUNKOFF
							// Store gadget values
							_config_env_store(data, data->option);
#endif

							// Free objects
							FreeObjectList(data->option_list);
							data->option_list = 0;

							// Erase display
							SetGadgetValue(data->objlist, GAD_ENVIRONMENT_EDIT_AREA, 0);
						}

						// Create option list
						data->option = _environment_options[data->option_node->data].num;
						data->option_list =
							AddObjectList(data->window, _environment_options[data->option_node->data].objects);

#ifndef FUNKOFF
						// Initialise gadgets
						if (data->option_list)
							_config_env_set(data, data->option);
#endif
						break;

#ifndef FUNKOFF
					// New screen mode selected
					case GAD_ENVIRONMENT_SCREENMODE_MODE: {
						Att_Node *node;

						// Get selected node
						if ((node = Att_FindNode(data->mode_list, msg_copy.Code)))
						{
							// Set mode
							_config_env_screenmode_set_mode(data, node);
						}
					}
					break;

					// Default width/height checkmarks
					case GAD_ENVIRONMENT_SCREENMODE_DEFWIDTH:
					case GAD_ENVIRONMENT_SCREENMODE_DEFHEIGHT:
						_config_env_screenmode_fix_gadgets(data);
						break;

					// New width/height entered
					case GAD_ENVIRONMENT_SCREENMODE_WIDTH:
					case GAD_ENVIRONMENT_SCREENMODE_HEIGHT:
						_config_env_screenmode_check_values(data);
						break;

					// Number of colours changed
					case GAD_ENVIRONMENT_SCREENMODE_COLORS:

						// Store new depth
						data->config->screen_depth = msg_copy.Code;

						// Show depth
						_config_env_screenmode_show_depth(data);

						// Remember this one
						data->slide_gun = (msg_copy.Class == IDCMP_GADGETDOWN) ? 3 : -1;
						break;

					// New colour selected
					case GAD_ENVIRONMENT_PALETTE_PALETTE:

						// Valid pen, gadget up only?
						if (msg_copy.Class == IDCMP_GADGETUP)
						{
							// Calculate palette entry
							if (msg_copy.Code < 4)
							{
								if (data->own_screen)
									data->sel_colour = msg_copy.Code;
								else
									data->sel_colour = msg_copy.Code + 8;
							}
							else if (msg_copy.Code < 8)
							{
								if (data->own_screen > 4)
								{
									// Fix top four mapping under 39
									if (((struct Library *)GfxBase)->lib_Version >= 39)
										data->sel_colour = 7 - (msg_copy.Code - 4);
									else
										data->sel_colour = msg_copy.Code;
								}
								else
									data->sel_colour = msg_copy.Code + 8;
							}
							else
								data->sel_colour = msg_copy.Code;

							// Store original selection number
							data->orig_colour_sel = msg_copy.Code;

							// Update sliders
							_config_env_palette_update_sliders(data);
						}
						break;

					// New lister element
					case GAD_ENVIRONMENT_LISTER_COLOUR_ITEMS: {
						UBYTE a, *ptr;
						short old;

						// Get current lister selection
						a = GetGadgetValue(data->option_list, GAD_ENVIRONMENT_LISTER_COLOUR_ITEMS);

						// Get pointer
						if (a == 0)
							ptr = data->config->list_format.files_unsel;
						else if (a == 1)
							ptr = data->config->list_format.dirs_unsel;
						else if (a == 2)
							ptr = data->config->list_format.files_sel;
						else if (a == 3)
							ptr = data->config->list_format.dirs_sel;
						else if (a == 4)
							ptr = data->config->source_col;
						else if (a == 5)
							ptr = data->config->dest_col;
						else if (a == 6)
							ptr = data->config->devices_col;
						else if (a == 7)
							ptr = data->config->volumes_col;
						else if (a == 8)
							ptr = data->config->gauge_col;
						else if (a == ENVCOL_TOOLTIP)
							ptr = data->config->tooltip_col;
						else
							break;

						// Save old pen
						old = data->colour_number;

						// Store selected item
						data->colour_selection = ptr;
						data->colour_number = a;

						// Free existing custom pens, get new ones
						for (a = 0; a < 2; a++)
						{
							// Free old pen
							if (data->custom_pen[a] > -1)
								IPC_Command(data->main_ipc, MAINCMD_RELEASE_PEN, (old << 16) | a, 0, 0, 0);

							// Get new custom pen
							data->custom_pen[a] =
								(short)IPC_Command(data->main_ipc,
												   MAINCMD_GET_PEN,
												   (data->colour_number << 16) | a,
												   (APTR)&data->config->env_Colours[data->colour_number][a],
												   0,
												   REPLY_NO_PORT);
						}

						// Initialise "Example" area
						_config_env_update_example(data, ptr);
					}
					break;

					// Modify an element colour
					case GAD_ENVIRONMENT_COLOURS_MODIFY: {
						DOpusScreenData scrdata;
						short a, fgpen, bgpen, flags;

						// Make window busy
						SetWindowBusy(data->window);

						// Fake up screendata
						scrdata.pen_count =
							data->palette_count - ((((struct Library *)IntuitionBase)->lib_Version >= 39) ? 8 : 4);
						for (a = 0; a < 16; a++)
							scrdata.pen_array[a] = data->palette_array[a];

						// Get current pens
						fgpen = data->colour_selection[0];
						bgpen = data->colour_selection[1];

						// Using custom colours?
						if (data->config->env_ColourFlag & (1 << data->colour_number))
							fgpen = data->palette_count - 4;
						if (data->config->env_ColourFlag & (1 << (data->colour_number + CUST_PENS)))
							bgpen = data->palette_count - 4;

                        // Bring up palette box
						if ((flags = L_ShowPaletteBox(
								 data->window,
								 &scrdata,
								 &fgpen,
								 &bgpen,
								 0,
								 (ColourSpec32 *)data->config->env_Colours[data->colour_number],
								 data->custom_pen)) > -1)
						{
							// Save colour selection
							data->colour_selection[0] = (UBYTE)fgpen;
							data->colour_selection[1] = (UBYTE)bgpen;

							// Using custom colours?
							if (flags & (1 << 0))
								data->config->env_ColourFlag |= 1 << data->colour_number;
							else
								data->config->env_ColourFlag &= ~(1 << data->colour_number);

							if (flags & (1 << 1))
								data->config->env_ColourFlag |= 1 << (data->colour_number + CUST_PENS);
							else
								data->config->env_ColourFlag &= ~(1 << (data->colour_number + CUST_PENS));
						}

						// Update "Example" area
						_config_env_update_example(data, data->colour_selection);

						// Make window unbusy
						ClearWindowBusy(data->window);
					}
					break;

					// Red/Green/Blue sliders
					case GAD_ENVIRONMENT_PALETTE_RED:
					case GAD_ENVIRONMENT_PALETTE_GREEN:
					case GAD_ENVIRONMENT_PALETTE_BLUE:

						// Change colour
						_config_env_palette_change_colour(data, gadgetid - GAD_ENVIRONMENT_PALETTE_RED, msg_copy.Code);

						// Remember this one
						data->slide_gun =
							(msg_copy.Class == IDCMP_GADGETDOWN) ? (gadgetid - GAD_ENVIRONMENT_PALETTE_RED) : -1;
						break;

					// Edit list format
					case GAD_ENVIRONMENT_LISTER_EDIT_FORMAT: {
						struct Library *ModuleBase = NULL;
	#ifdef __amigaos4__
						struct ModuleIFace *IModule = NULL;
	#endif

						// Make window busy
						SetWindowBusy(data->window);

						// Get lister format module
						if ((ModuleBase = OpenLibrary("DOpus5:Modules/listerformat.module", LIB_VERSION)) &&
							GETINTERFACE(IModule, ModuleBase))
						{
							// Edit format
							Module_Entry((struct List *)&data->config->list_format,
										 (struct Screen *)data->window,
										 ipc,
										 data->main_ipc,
										 0,
										 0);

	#ifdef __amigaos4__
							DropInterface((struct Interface *)IModule);
	#endif
							CloseLibrary(ModuleBase);
						}
						else
							CloseLibrary(ModuleBase);

						// Make window unbusy
						ClearWindowBusy(data->window);
					}
					break;

					// Reset palette
					case GAD_ENVIRONMENT_PALETTE_RESET: {
						// Copy original palette
						CopyMem(
							(char *)env->env->palette, (char *)data->config->palette, sizeof(data->config->palette));

						// Update sliders
						_config_env_palette_update_sliders(data);

						// Reset palette
						_config_env_reset_palette(data);
					}
					break;

					// Grab Workbench palette
					case GAD_ENVIRONMENT_PALETTE_GRABWB:

						// Do it
						_config_env_grabwb(data);

						// Update sliders
						_config_env_palette_update_sliders(data);
						break;

					// Number of colours changed
					case GAD_ENVIRONMENT_PALETTE_COUNT:

						// Get new number of colours
						data->config->palette_count = msg_copy.Code;

						// Update palette gadget
						_config_env_palette_init(data);

						// Fix pens for font requesters
						_config_env_fix_font_pens(data);
						break;

					// Backdrop pattern
					case GAD_ENVIRONMENT_OPTIONS_DISPLAY_BACKDROP:
					case GAD_ENVIRONMENT_PICTURE_USE_WBPATTERN:

						// Disable gadgets appropriately
						config_env_fix_picture_gads(data);
						break;

					// Background picture prefs
					case GAD_ENVIRONMENT_PICTURE_DESK_PREFS:
					case GAD_ENVIRONMENT_PICTURE_LISTER_PREFS:
					case GAD_ENVIRONMENT_PICTURE_REQ_PREFS:

						// Only do this on gadget down
						if (msg_copy.Class == IDCMP_GADGETDOWN)
						{
							// Do popup menu
							config_env_picture_menu(data, gadgetid - GAD_ENVIRONMENT_PICTURE_DESK_PREFS);

							// Make sure gadget is deselected
							gadget->Flags &= ~GFLG_SELECTED;
							RefreshGList(gadget, data->window, 0, 1);
						}
						break;

					// Set output window size
					case GAD_ENVIRONMENT_OUTPUT_SET:
					case GAD_ENVIRONMENT_OUTPUT_SETSIZE:

						// Make window busy
						SetWindowBusy(data->window);

						// Configure size
						quit_msg = _config_env_output_window_set(data, gadgetid);

						// Clear busy
						ClearWindowBusy(data->window);

						// Do we have to quit?
						if (quit_msg)
						{
							quit_flag = 1;
							success = CONFIG_NO_CHANGE;
						}
						break;

					// Output device
					case GAD_ENVIRONMENT_OUTPUT_DEVICE:

						// Check valid device
						config_env_check_device(data);
						break;

					// Pop-up list for status bar text
					case GAD_ENVIRONMENT_LISTER_STATUS_LIST:

						// Display list
						_config_env_status_list(data->objlist,
												GAD_ENVIRONMENT_LISTER_STATUS,
												MSG_STATUSBAR_CODE_FIRST,
												MSG_STATUSBAR_CODE_LAST);
						break;

					// Pop-up list for screen title text
					case GAD_ENVIRONMENT_SCREEN_TITLE_LIST:

						// Display list
						_config_env_status_list(
							data->objlist, GAD_ENVIRONMENT_SCREEN_TITLE, MSG_SCREENTITLE_CODE_FIRST, 0);
						break;

					// Pop-up list for clock format text
					case GAD_ENVIRONMENT_CLOCK_FORMAT_LIST:

						// Display list
						_config_env_clock_format_list(data->objlist,
													  GAD_ENVIRONMENT_CLOCK_FORMAT,
													  MSG_CLOCKFORMAT_CODE_FIRST,
													  MSG_CLOCKFORMAT_CODE_LAST);
						break;

					// Edit pattern
					case GAD_ENVIRONMENT_EDIT_PATTERN: {
						char func[256];
						char *ptr;

						// Build function string
						strcpy(func, "sys:prefs/WBPattern");

						// Get filename
						if ((ptr = (char *)GetGadgetValue(data->option_list, GAD_ENVIRONMENT_MAIN_WINDOW_FIELD)) &&
							*ptr)
						{
							size_t len;

							// Tack it on
							strcat(func, " \"");
							len = strlen(func);
							if (len < sizeof(func) - 2)
							{
								stccpy(func + len, ptr, sizeof(func) - len - 1);
								len = strlen(func);
								func[len++] = '"';
								func[len] = 0;
							}
						}

						// Launch function
						CLI_Launch(func, data->window->WScreen, 0, 0, 0, 0, 0);
					}
					break;

					// Select desktop icon font
					case GAD_ENVIRONMENT_ICON_FONT_DESKTOP:
						_env_select_icon_font(data,
											  data->config->font_name[FONT_ICONS],
											  &data->config->font_size[FONT_ICONS],
											  &data->config->icon_fpen,
											  &data->config->icon_bpen,
											  &data->config->icon_style,
											  sizeof(data->config->font_name[FONT_ICONS]));
						break;

					// Select windows icon font
					case GAD_ENVIRONMENT_ICON_FONT_WINDOWS:
						_env_select_icon_font(data,
											  data->config->font_name[FONT_WINDOW_ICONS],
											  &data->config->font_size[FONT_WINDOW_ICONS],
											  &data->config->iconw_fpen,
											  &data->config->iconw_bpen,
											  &data->config->iconw_style,
											  sizeof(data->config->font_name[FONT_WINDOW_ICONS]));
						break;

					// Stack size changed
					case GAD_ENVIRONMENT_STACK:
						BoundsCheckGadget(data->option_list, GAD_ENVIRONMENT_STACK, STACK_DEFAULT, 0x7FFFFFFF);
						break;

					// Command line length
					case GAD_ENVIRONMENT_CLL:
						BoundsCheckGadget(data->option_list, GAD_ENVIRONMENT_CLL, 256, 8192);
						break;

					// Popup delay
					case GAD_SETTINGS_POPUP_DELAY:
						BoundsCheckGadget(data->option_list, GAD_SETTINGS_POPUP_DELAY, 1, 50);
						break;

					// Max. open with
					case GAD_SETTINGS_MAX_OPENWITH:
						BoundsCheckGadget(data->option_list, GAD_SETTINGS_MAX_OPENWITH, 0, 100);
						break;

					// Max. filename
					case GAD_SETTINGS_MAX_FILENAME:
						BoundsCheckGadget(data->option_list, GAD_SETTINGS_MAX_FILENAME, 30, MAX_FILENAME_LEN);
						break;

					// NewIcons precision
					case GAD_ENVIRONMENT_NEWICONS_PRECISION:
						BoundsCheckGadget(data->option_list, GAD_ENVIRONMENT_NEWICONS_PRECISION, -1, 16);
						break;

					// Hidden drives cycle switched
					case GAD_ENVIRONMENT_HIDDEN_DRIVES_CYCLE:

						// Attach new drive list
						SetGadgetChoices(
							data->option_list, GAD_ENVIRONMENT_HIDDEN_DRIVES, data->desktop_drives[msg_copy.Code]);
						break;

					// Desktop popup enable/disable
					case GAD_ENVIRONMENT_DESKTOP_ENABLE:

						// Disable/enable cycle gadget
						DisableObject(data->option_list, GAD_ENVIRONMENT_DESKTOP_DEFAULT, !msg_copy.Code);

						// Store state
						if (msg_copy.Code)
							data->config->env_flags |= ENVF_DESKTOP_FOLDER;
						else
							data->config->env_flags &= ~ENVF_DESKTOP_FOLDER;
						break;

					// Icon position gadget disable
					case GAD_ENVIRONMENT_OPTIONS_ICONPOS:

						// Disable gadget
						DisableObject(data->option_list, GAD_ENVIRONMENT_OPTIONS_REMOVE_OPUS, !msg_copy.Code);
						break;

					// Use pathlist
					case GAD_SETTINGS_USE_PATHLIST:

						// Disable/enable gadgets
						for (a = GAD_SETTINGS_PATHLIST; a <= GAD_SETTINGS_PATHLIST_PATH; a++)
							if (a != GAD_SETTINGS_PATHLIST_DELETE || !msg_copy.Code)
								DisableObject(data->option_list, a, !msg_copy.Code);

						// Enabled, and pathlist is empty?
						if (msg_copy.Code && data->path_list && IsListEmpty((struct List *)data->path_list))
							config_env_build_pathlist(data, 0);
						break;

					// Reset pathlist
					case GAD_SETTINGS_PATHLIST_RESET:
						config_env_build_pathlist(data, 1);
						break;

					// Add pathlist
					case GAD_SETTINGS_PATHLIST_ADD:
						config_env_path_add(data);
						break;

					// Delete pathlist
					case GAD_SETTINGS_PATHLIST_DELETE:
						config_env_path_del(data);
						break;

					// Path entry selected
					case GAD_SETTINGS_PATHLIST:
						config_env_path_sel(data);
						break;

					// Path entry edited
					case GAD_SETTINGS_PATHLIST_PATH:
						config_env_path_edit(data);
						break;

					// Sound entry selected
					case GAD_SETTINGS_SOUNDLIST:
						config_env_show_sound(data);
						break;

					// Clear sound entry
					case GAD_SETTINGS_SOUNDLIST_RESET:
						SetGadgetValue(data->option_list, GAD_SETTINGS_SOUNDLIST_PATH, 0);
						SetGadgetValue(data->option_list, GAD_SETTINGS_VOLUME, 64);

					// Sound entry edited
					case GAD_SETTINGS_SOUNDLIST_PATH:
					case GAD_SETTINGS_VOLUME:
						config_env_store_sound(data);
						break;

					// Volume slider
					case GAD_SETTINGS_VOLUME_SLIDER:
						SetGadgetValue(data->objlist, GAD_SETTINGS_VOLUME, msg_copy.Code);
						data->slide_gun = (msg_copy.Class == IDCMP_GADGETDOWN) ? -2 : -1;
						config_env_store_sound(data);
						break;

					// Test sound
					case GAD_SETTINGS_TEST_SOUND:
						config_env_store_sound(data);
						config_env_test_sound(data);
						break;

#endif

					// Open, etc
					case BUTTONEDIT_MENU_OPEN:
					case BUTTONEDIT_MENU_NEW:
					case BUTTONEDIT_MENU_DEFAULTS:
					case BUTTONEDIT_MENU_LASTSAVED:
					case BUTTONEDIT_MENU_RESTORE:

						// Do open
						config_env_load(data, gadgetid);
						break;

					// Save/Save As
					case BUTTONEDIT_MENU_SAVE:
					case BUTTONEDIT_MENU_SAVEAS:

						// Save settings
						_config_env_store(data, data->option);

						// Do save
						config_env_save(data, gadgetid - BUTTONEDIT_MENU_SAVE);
						break;

					// Cancel
					case GAD_ENVIRONMENT_CANCEL:
					case BUTTONEDIT_MENU_CLOSE:
						success = CONFIG_NO_CHANGE;
						quit_flag = 1;
						break;

					// Save
					case GAD_ENVIRONMENT_SAVE:
						success |= CONFIG_SAVE;

					// Use
					case GAD_ENVIRONMENT_USE:
						quit_flag = 1;
						break;
					}
					break;

#ifndef FUNKOFF
				// Mouse move
				case IDCMP_MOUSEMOVE:

					// See if sliding a slider
					if (data->slide_gun > -1)
					{
						// Colours slider?
						if (data->slide_gun > 2)
						{
							// Store new depth
							data->config->screen_depth = msg_copy.Code;

							// Show depth
							_config_env_screenmode_show_depth(data);
						}

						// Palette slider
						else
						{
							// Change colour
							_config_env_palette_change_colour(data, data->slide_gun, msg_copy.Code);
						}
					}

					// Sliding volume control
					else if (data->slide_gun == -2)
					{
						SetGadgetValue(data->objlist, GAD_SETTINGS_VOLUME, msg_copy.Code);
					}

					// Handle drag move
					else
					{
						config_drag_move(&data->drag);
					}
					break;

				// Inactive window does menu down
				case IDCMP_INACTIVEWINDOW:
					msg_copy.Code = MENUDOWN;

				// Mouse buttons
				case IDCMP_MOUSEBUTTONS:

					// Valid drag info?
					if (data->drag.drag)
					{
						short ok = -1;

						// Dropped ok?
						if (msg_copy.Code == SELECTUP)
						{
							// Remember last position
							data->drag.drag_x = data->window->WScreen->MouseX;
							data->drag.drag_y = data->window->WScreen->MouseY;
							ok = 1;
						}

						// Aborted
						else if (msg_copy.Code == MENUDOWN)
						{
							// Set abort flag
							ok = 0;
						}

						// Want to end drag?
						if (ok != -1)
						{
							config_env_paths_end_drag(data, ok);
						}
					}
					break;

				// Key press
				case IDCMP_RAWKEY:

					// Help?
					if (msg_copy.Code == 0x5f && !(msg_copy.Qualifier & VALID_QUALIFIERS))
					{
						char buf[80];
						struct DOpusLocale temp;

						// Base environment help string
						strcpy(buf, "Environment");

						// Copy locale and clear catalog pointer
						temp = *locale;
						temp.li_Catalog = 0;

						// Selected option?
						if (data->option_node)
						{
							// Add to help string
							strcat(buf, " - ");
							strcat(buf, GetString(&temp, _environment_options[data->option_node->data].name));
						}

						// Set busy pointer
						SetWindowBusy(data->window);

						// Send help command
						IPC_Command(data->main_ipc, IPC_HELP, (1 << 31), buf, 0, REPLY_NO_PORT);

						// Clear busy pointer
						ClearWindowBusy(data->window);
					}
					break;
#endif
				}

				// Reply to any outstanding message
				if (msg)
					ReplyWindowMsg(msg);
			}
		}

		// Check quit flag
		if (quit_flag)
			break;

		// Wait for input
		Wait(((data->window) ? 1 << data->window->UserPort->mp_SigBit : 0) |
			 ((data->app_port) ? 1 << data->app_port->mp_SigBit : 0) | 1 << ipc->command_port->mp_SigBit);
	}

#ifndef FUNKOFF
	// Close.. see if we have an option open
	if (data->option_list)
		_config_env_store(data, data->option);

	// If not successful, restore palette
	if (!success)
	{
		if (data->window)
		{
			// Reset palette
			_config_env_reset_palette(data);
		}
	}

	// If successful
	else
	{
		// Lock configuration
		GetSemaphore(&env->lock, SEMF_EXCLUSIVE, 0);

		// Different screen mode or size?
		if (data->config->screen_mode != env->env->screen_mode ||
			data->config->screen_depth != env->env->screen_depth ||
			data->config->screen_width != env->env->screen_width ||
			data->config->screen_height != env->env->screen_height)
		{
			// Reset window position
			if (data->config->screen_mode != env->env->screen_mode)
			{
				data->config->window_pos.Width = -1;
				data->config->window_pos.Height = -1;
			}
			change_flags[0] |= CONFIG_CHANGE_DISPLAY;
		}

		// New public screen?
		else if (data->config->screen_mode == MODE_PUBLICSCREEN &&
				 strcmp(data->config->pubscreen_name, env->env->pubscreen_name) != 0)
		{
			// Reset window position
			data->config->window_pos.Width = -1;
			data->config->window_pos.Height = -1;
			change_flags[0] |= CONFIG_CHANGE_DISPLAY;
		}

		// New font?
		else if (data->config->screen_mode != MODE_WORKBENCHUSE && data->config->screen_mode != MODE_PUBLICSCREEN &&
				 (strcmp(data->config->font_name[FONT_SCREEN], env->env->font_name[FONT_SCREEN]) != 0 ||
				  data->config->font_size[FONT_SCREEN] != env->env->font_size[FONT_SCREEN]))
		{
			// Reset full display
			change_flags[0] |= CONFIG_CHANGE_DISPLAY;
		}

		// Has the number of user colours changed
		if (data->config->palette_count != env->env->palette_count)
			change_flags[0] |= CONFIG_CHANGE_PALETTE;

		// Lister font changed?
		if (stricmp(data->config->font_name[FONT_DIRS], env->env->font_name[FONT_DIRS]) != 0 ||
			data->config->font_size[FONT_DIRS] != env->env->font_size[FONT_DIRS])
		{
			// Will close and re-open all listers
			change_flags[0] |= CONFIG_CHANGE_LIST_DISPLAY | CONFIG_CHANGE_LIST_FONT;
		}

		// Lister display changed?
		else
		{
			short a;

			for (a = 0; a < 2; a++)
			{
				if (data->config->list_format.files_unsel[a] != env->env->list_format.files_unsel[a] ||
					data->config->list_format.files_sel[a] != env->env->list_format.files_sel[a] ||
					data->config->list_format.dirs_unsel[a] != env->env->list_format.dirs_unsel[a] ||
					data->config->list_format.dirs_sel[a] != env->env->list_format.dirs_sel[a] ||
					data->config->source_col[a] != env->env->source_col[a] ||
					data->config->dest_col[a] != env->env->dest_col[a] ||
					data->config->devices_col[a] != env->env->devices_col[a] ||
					data->config->volumes_col[a] != env->env->volumes_col[a] ||
					data->config->gauge_col[a] != env->env->gauge_col[a] ||
					data->config->tooltip_col[a] != env->env->tooltip_col[a])
				{
					change_flags[0] |= CONFIG_CHANGE_LIST_DISPLAY;
					break;
				}
			}
		}

		// Lister title changed?
		if (stricmp(data->config->status_text, env->env->status_text) != 0 ||
			stricmp(data->config->scr_title_text, env->env->scr_title_text) != 0 ||
			stricmp(data->config->clock_format, env->env->clock_format) != 0)
			change_flags[0] |= CONFIG_CHANGE_LOCALE;

		// Field titles?
		if ((data->config->lister_options & LISTEROPTF_TITLES) != (env->env->lister_options & LISTEROPTF_TITLES))
			change_flags[0] |= CONFIG_CHANGE_LISTER_TITLES;

		// Background pictures
		if (((data->config->display_options & DISPOPTF_NO_BACKDROP) !=
			 (env->env->display_options & DISPOPTF_NO_BACKDROP)) ||
			((data->config->display_options & DISPOPTF_USE_WBPATTERN) !=
			 (env->env->display_options & DISPOPTF_USE_WBPATTERN)) ||
			(!(data->config->display_options & DISPOPTF_NO_BACKDROP) &&
			 data->config->display_options & DISPOPTF_USE_WBPATTERN &&
			 stricmp(data->config->backdrop_prefs, env->env->backdrop_prefs) != 0) ||
			(!(data->config->display_options & (DISPOPTF_NO_BACKDROP | DISPOPTF_USE_WBPATTERN)) &&
			 (stricmp(data->config->env_BackgroundPic[0], env->env->env_BackgroundPic[0]) != 0 ||
			  stricmp(data->config->env_BackgroundPic[1], env->env->env_BackgroundPic[1]) != 0 ||
			  stricmp(data->config->env_BackgroundPic[2], env->env->env_BackgroundPic[2]) != 0 ||
			  data->config->env_BackgroundFlags[0] != env->env->env_BackgroundFlags[0] ||
			  data->config->env_BackgroundFlags[1] != env->env->env_BackgroundFlags[1] ||
			  data->config->env_BackgroundFlags[2] != env->env->env_BackgroundFlags[2] ||
			  (data->config->env_BackgroundFlags[0] & ENVBF_USE_COLOUR &&
			   data->config->env_BackgroundBorderColour[0] != env->env->env_BackgroundBorderColour[0]) ||
			  (data->config->env_BackgroundFlags[1] & ENVBF_USE_COLOUR &&
			   data->config->env_BackgroundBorderColour[1] != env->env->env_BackgroundBorderColour[1]) ||
			  (data->config->env_BackgroundFlags[2] & ENVBF_USE_COLOUR &&
			   data->config->env_BackgroundBorderColour[2] != env->env->env_BackgroundBorderColour[2]))))
		{
			change_flags[0] |= CONFIG_CHANGE_BACKDROP;
		}

		// Changed AppIcons status?
		else
		{
			if ((data->config->display_options & DISPOPTF_SHOW_APPICONS) !=
				(env->env->display_options & DISPOPTF_SHOW_APPICONS))
				change_flags[0] |= CONFIG_CHANGE_APP_ICONS;

			if ((data->config->display_options & DISPOPTF_HIDE_BAD) != (env->env->display_options & DISPOPTF_HIDE_BAD))
				change_flags[0] |= CONFIG_CHANGE_BAD_DISKS;

			if ((data->config->display_options & DISPOPTF_SHIFT_APPICONS) !=
				(env->env->display_options & DISPOPTF_SHIFT_APPICONS))
				change_flags[1] |= CONFIG_CHANGE_APP_TOOLS;
		}

		// Changed Tools menu status?
		if ((data->config->display_options & DISPOPTF_SHOW_TOOLS) != (env->env->display_options & DISPOPTF_SHOW_TOOLS))
			change_flags[0] |= CONFIG_CHANGE_TOOLS_MENU;

		// Changed thin borders setting?
		if ((data->config->display_options & DISPOPTF_THIN_BORDERS) !=
			(env->env->display_options & DISPOPTF_THIN_BORDERS))
			change_flags[1] |= CONFIG_CHANGE_THIN_BORDERS;

		// Changed icon font for desktop?
		if (stricmp(data->config->font_name[FONT_ICONS], env->env->font_name[FONT_ICONS]) ||
			data->config->font_size[FONT_ICONS] != env->env->font_size[FONT_ICONS] ||
			data->config->icon_fpen != env->env->icon_fpen || data->config->icon_bpen != env->env->icon_bpen ||
			data->config->icon_style != env->env->icon_style)
			change_flags[0] |= CONFIG_CHANGE_ICON_FONT;

		// Changed icon font for windows?
		if (stricmp(data->config->font_name[FONT_WINDOW_ICONS], env->env->font_name[FONT_WINDOW_ICONS]) ||
			data->config->font_size[FONT_WINDOW_ICONS] != env->env->font_size[FONT_WINDOW_ICONS] ||
			data->config->iconw_fpen != env->env->iconw_fpen || data->config->iconw_bpen != env->env->iconw_bpen ||
			data->config->iconw_style != env->env->iconw_style)
			change_flags[0] |= CONFIG_CHANGE_ICON_FONT_WINDOWS;

		// The 'remap icons', 'icon borders' and 'toolbar arrow' flags all cause icon windows to refresh (fake font
		// change)
		if ((data->config->desktop_flags & DESKTOPF_NO_REMAP) != (env->env->desktop_flags & DESKTOPF_NO_REMAP) ||
			(data->config->desktop_flags & DESKTOPF_NO_BORDERS) != (env->env->desktop_flags & DESKTOPF_NO_BORDERS) ||
			(data->config->desktop_flags & DESKTOPF_NO_NOLABELS) != (env->env->desktop_flags & DESKTOPF_NO_NOLABELS) ||
			(data->config->desktop_flags & DESKTOPF_TRANSPARENCY) !=
				(env->env->desktop_flags & DESKTOPF_TRANSPARENCY) ||
			(data->config->desktop_flags & DESKTOPF_NO_ARROW) != (env->env->desktop_flags & DESKTOPF_NO_ARROW) ||
			(data->config->desktop_flags & DESKTOPF_SPLIT_LABELS) !=
				(env->env->desktop_flags & DESKTOPF_SPLIT_LABELS) ||
			(data->config->desktop_flags & DESKTOPF_FORCE_SPLIT) != (env->env->desktop_flags & DESKTOPF_FORCE_SPLIT))
			change_flags[0] |= CONFIG_CHANGE_ICON_FONT | CONFIG_CHANGE_ICON_FONT_WINDOWS;

		// Hidden drives?
		if (_config_env_update_drives_list(data, DESKTOP_HIDE))
			change_flags[0] |= CONFIG_CHANGE_HIDDEN_DRIVES;
		if (_config_env_update_drives_list(data, DESKTOP_HIDE_BAD))
			change_flags[0] |= CONFIG_CHANGE_HIDDEN_DRIVES;

		// No cache needs a reboot
		if ((data->config->desktop_flags & DESKTOPF_NO_CACHE) != (env->env->desktop_flags & DESKTOPF_NO_CACHE))
			success |= CONFIG_NEED_RESET;

		// Max. filename length needs a reboot
		if (data->config->settings.max_filename != env->env->settings.max_filename)
			success |= CONFIG_NEED_RESET;

		// Changed folder location
		if (stricmp(data->config->desktop_location, env->env->desktop_location) != 0)
			change_flags[1] |= CONFIG_CHANGE_DESKTOP_FOLDER;

		// Changed NewIcons settings
		if ((data->config->env_NewIconsFlags != env->env->env_NewIconsFlags) ||
			(data->config->env_NewIconsPrecision != env->env->env_NewIconsPrecision))
		{
			change_flags[1] |= CONFIG_CHANGE_NEWICONS;
			success |= CONFIG_NEED_RESET;
		}

		// Maximum buffers changed?
		if (data->config->settings.max_buffer_count != env->env->settings.max_buffer_count ||
			(data->config->settings.dir_flags & DIRFLAGS_DISABLE_CACHING) !=
				(env->env->settings.dir_flags & DIRFLAGS_DISABLE_CACHING))
			change_flags[0] |= CONFIG_CHANGE_MAX_BUFFERS;

		// Locale options changed?
		if (data->config->settings.date_format != env->env->settings.date_format ||
			data->config->settings.date_flags != env->env->settings.date_flags)
			change_flags[0] |= CONFIG_CHANGE_LOCALE;

		// Mouse button hook changed?
		if ((data->config->settings.general_flags & (GENERALF_MOUSE_HOOK | GENERALF_VALID_POPKEY)) !=
			(env->env->settings.general_flags & (GENERALF_MOUSE_HOOK | GENERALF_VALID_POPKEY)))
			change_flags[0] |= CONFIG_CHANGE_MOUSEHOOK;

		// Or popkey changed?
		else if (data->config->settings.pop_code != env->env->settings.pop_code ||
				 data->config->settings.pop_qual != env->env->settings.pop_qual ||
				 data->config->settings.pop_qual_mask != env->env->settings.pop_qual_mask ||
				 data->config->settings.pop_qual_same != env->env->settings.pop_qual_same)
			change_flags[0] |= CONFIG_CHANGE_MOUSEHOOK;

		// Changed 'custom drag' flag
		if ((data->config->desktop_flags & DESKTOPF_NO_CUSTOMDRAG) !=
			(env->env->desktop_flags & DESKTOPF_NO_CUSTOMDRAG))
			change_flags[1] |= CONFIG_CHANGE_CUSTOM_DRAG;

		// Changed 'enable mufs' flag
		if ((data->config->settings.general_flags & GENERALF_ENABLE_MUFS) !=
			(env->env->settings.general_flags & GENERALF_ENABLE_MUFS))
			change_flags[1] |= CONFIG_CHANGE_MUFS;

		// Changed theme path
		if (stricmp(data->config->themes_location, env->env->themes_location) != 0)
			change_flags[1] |= CONFIG_CHANGE_THEMES;

		// Changed path list
		if ((data->config->env_flags & ENVF_USE_PATHLIST) != (env->env->env_flags & ENVF_USE_PATHLIST) ||
	#ifdef __MORPHOS__
			(data->path_list && data->path_list->list.lh_pad))
	#else
			(data->path_list && data->path_list->list.l_pad))
	#endif
		{
			struct MinNode *node, *next;
			Att_Node *path;

			// Set flag
			change_flags[1] |= CONFIG_CHANGE_PATHLIST;

	// Path list changed?
	#ifdef __MORPHOS__
			if (data->path_list && data->path_list->list.lh_pad)
	#else
			if (data->path_list && data->path_list->list.l_pad)
	#endif
			{
				// Free old path list
				for (node = env->path_list.mlh_Head; node->mln_Succ; node = next)
				{
					next = node->mln_Succ;
					FreeMemH(node);
				}
				NewList((struct List *)&env->path_list);

				// Copy new list back
				for (path = (Att_Node *)data->path_list->list.lh_Head; path->node.ln_Succ;
					 path = (Att_Node *)path->node.ln_Succ)
				{
					// Add node to list
					if ((node =
							 AllocMemH(env->desktop_memory, strlen(path->node.ln_Name) + 1 + sizeof(struct MinNode))))
					{
						strcpy((char *)(node + 1), path->node.ln_Name);
						AddTail((struct List *)&env->path_list, (struct Node *)node);
					}
				}
			}
		}

		// Popup menu delay changed?
		if (data->config->settings.popup_delay != env->env->settings.popup_delay)
			change_flags[1] |= CONFIG_CHANGE_POPUP_DELAY;

		// See if sounds changed
		{
			Cfg_SoundEntry *sound;
			GetSemaphore(&env->sound_lock, SEMF_EXCLUSIVE, 0);
			for (sound = (Cfg_SoundEntry *)data->sound_list.mlh_Head; sound->dse_Node.ln_Succ;
				 sound = (Cfg_SoundEntry *)sound->dse_Node.ln_Succ)
			{
				// Did this sound change?
				if (sound->dse_Node.lve_Flags & LVEF_TEMP)
				{
					Cfg_SoundEntry *orig;

					// Find original sound
					if ((orig = (Cfg_SoundEntry *)FindNameI((struct List *)&env->sound_list, sound->dse_Name)))
					{
						// New sound empty?
						if (!sound->dse_Sound[0])
						{
							// Remove from list
							Remove((struct Node *)orig);
							FreeMemH(orig);
							orig = 0;
						}

						// Store new settings
						else
						{
							strcpy(orig->dse_Sound, sound->dse_Sound);
							orig->dse_Volume = sound->dse_Volume;
							orig->dse_Flags = sound->dse_Flags;
						}
					}

					// Need to create entry, but only if sound is set
					else if (sound->dse_Sound)
					{
						// Create entry
						if ((orig = AllocMemH(env->desktop_memory, sizeof(Cfg_SoundEntry))))
						{
							strcpy(orig->dse_Name, sound->dse_Name);
							strcpy(orig->dse_Sound, sound->dse_Sound);
							orig->dse_Volume = sound->dse_Volume;
							orig->dse_Flags = sound->dse_Flags;
							orig->dse_Node.ln_Name = orig->dse_Name;
							AddTail((struct List *)&env->sound_list, (struct Node *)orig);
						}
					}

					// Set change flag
					if (orig)
					{
						orig->dse_Flags |= CFGSEF_CHANGED;
						change_flags[1] |= CONFIG_CHANGE_SOUNDS;
					}
				}
			}
			FreeSemaphore(&env->sound_lock);
		}

		// Copy configuration back
		CopyMem((char *)data->config, (char *)env->env, sizeof(CFG_ENVR));

		// Unlock configuration
		FreeSemaphore(&env->lock);

		// Live folder updates / DOS patches: persist the checkbox state to
		// the dopus/DOSPatch ENVARC: variable, but only if the user actually
		// changed it - otherwise we'd mass-write the var on every Use even
		// when nothing about it had been touched.  Cancel skips this whole
		// block (the Cancel branch above doesn't fall through here), which
		// preserves the user's original ENV/ENVARC: state on rollback.
		// Note: the var is read by libinit.c at dopus5.library open time;
		// the patches won't install/uninstall live, so this requires a DOpus
		// restart to take full effect.
		if (data->dos_patch_modified)
		{
			if (data->dos_patch_state)
				SetVar("dopus/DOSPatch", "1", -1, GVF_GLOBAL_ONLY | GVF_SAVE_VAR);
			else
				DeleteVar("dopus/DOSPatch", GVF_GLOBAL_ONLY | GVF_SAVE_VAR);
		}
	}
#endif

	// Free data
	_config_env_cleanup(data);

	// Reply quit message
	IPC_Reply(quit_msg);

	return success;
}

// Open display
BOOL _config_env_open(config_env_data *data, struct Screen *screen)
{
	// Initialise palette stuff
	data->slide_gun = -1;
	data->alloc_pen_count = data->config->palette_count;

	// Open window
	data->newwin.parent = screen;
	data->newwin.dims = &_config_environment_window;
	data->newwin.title = GetString(locale, MSG_ENVIRONMENT_TITLE);
	data->newwin.locale = locale;
	data->newwin.flags = WINDOW_SCREEN_PARENT | WINDOW_VISITOR | WINDOW_AUTO_KEYS | WINDOW_REQ_FILL;
	if (!(data->window = OpenConfigWindow(&data->newwin)))
		return 0;

	/*
		// Under 39?
		if (GfxBase->LibNode.lib_Version>=39)
		{
			// See if we're on a cyber screen
			if (!(GetBitMapAttr(data->window->WScreen->RastPort.BitMap,BMA_FLAGS)&BMF_STANDARD))
				data->cyber_flag=1;
		}
	*/

	// Add menus
	AddWindowMenus(data->window, button_toolbar_menu);

	// Store data pointer in window
	DATA(data->window)->data = (IPTR)data;

	// Are we open on our own screen?
	if (data->env->env->screen_mode != MODE_WORKBENCHUSE && data->env->env->screen_mode != MODE_PUBLICSCREEN)
	{
		data->own_screen = 1 << data->window->WScreen->RastPort.BitMap->Depth;
	}

	// Make this an AppWindow
	if (data->app_port)
		data->appwindow = AddAppWindowA(0, 0, data->window, data->app_port, 0);

	// Store window pointer for dragging
	data->drag.window = data->window;

	// Create objects
	if (!(data->objlist = AddObjectList(data->window, _config_environment_objects)))
		return 0;

	// Set sub-option list
	SetGadgetChoices(data->objlist, GAD_ENVIRONMENT_ITEM_LISTER, data->options);

	// Get pens for palette editor
	if (data->palette_table)
	{
		short base, a;

		// Initialise table
		for (base = 0; base < 16; base++)
			data->palette_edit_pens[base] = -1;
		base = 0;

		// OS pens (if on custom screen)
		if (data->own_screen)
		{
			// Bottom four
			for (; base < 4; base++)
			{
				data->palette_edit_pens[base] = base;
				data->palette_edit_penalloc |= 1 << base;
			}

			// Top four if 39?
			if (((struct Library *)GfxBase)->lib_Version >= 39 && data->own_screen > 4)
			{
				for (; base < 8; base++)
				{
					data->palette_edit_pens[base] = (1 << data->window->WScreen->RastPort.BitMap->Depth) - 8 + base;
					data->palette_edit_penalloc |= 1 << base;
				}
			}
		}

		// Get pens we're given initially
		for (a = 0; a < 8; a++)
		{
			// Pen allocated?
			if (data->initial_pen_alloc & (1 << a))
			{
				data->palette_edit_pens[a + base] = data->palette_table[a];
				data->palette_edit_penalloc |= 1 << (a + base);
			}
			else
				data->palette_edit_pens[a + base] = -1;
		}

		// Try and allocate our own pens to play with
		if (((struct Library *)GfxBase)->lib_Version >= 39)
		{
			for (a = 0; a < 8; a++)
			{
				// Nothing already allocated?
				if (data->palette_edit_pens[a + base] == -1)
				{
					short pen;

					// Allocate a pen
					if ((pen = ObtainPen(data->window->WScreen->ViewPort.ColorMap,
										 -1,
										 data->config->palette[25 + a * 3],
										 data->config->palette[26 + a * 3],
										 data->config->palette[27 + a * 3],
										 PEN_EXCLUSIVE)) != -1)
					{
						// Got a pen!
						data->palette_edit_pens[a + base] = pen;
						data->palette_edit_penalloc |= 1 << (a + base);
						data->palette_edit_freepen |= 1 << (a + base);
					}
				}
			}
		}
	}

	// Initialise colour information
	_config_env_colours_init(data);

	// If no option selected, default to first entry
	if (data->option == -1 && !data->option_node)
	{
		data->option_node = Att_FindNode(data->options, 0);
	}

	// Got an option?
	if (data->option_node)
	{
		// Select option in list
		SetGadgetValue(
			data->objlist, GAD_ENVIRONMENT_ITEM_LISTER, Att_FindNodeNumber(data->options, data->option_node));

		// Set the option number
		data->option = _environment_options[data->option_node->data].num;

		// Create option list
		if ((data->option_list = AddObjectList(data->window, _environment_options[data->option_node->data].objects)))
		{

#ifndef FUNKOFF
			// Initialise gadgets
			_config_env_set(data, data->option);
#endif
		}
	}

	return 1;
}

// Close display
void _config_env_close(config_env_data *data)
{
	// Close.. see if we have an option open
#ifdef FUNKOFF
	if (data->option_list)
	{
		_config_env_store(data, data->option);
		data->option_list = 0;
	}
#endif

	// Got a window?
	if (data->window)
	{
		// Remove AppWindow
		RemoveAppWindow(data->appwindow);
		data->appwindow = 0;

		// End drag
		config_env_paths_end_drag(data, 0);

		// Free any pens we allocated
		if (((struct Library *)GfxBase)->lib_Version >= 39 && data->palette_edit_penalloc)
		{
			short a;

			for (a = 0; a < 16; a++)
			{
				// Got a pen?
				if (data->palette_edit_freepen & (1 << a))
				{
					// Free pen
					ReleasePen(data->window->WScreen->ViewPort.ColorMap, data->palette_edit_pens[a]);
				}
			}
		}

		// Close window
		CloseConfigWindow(data->window);
		data->window = 0;
	}
}

void _config_env_cleanup(config_env_data *data)
{
	if (data)
	{
		Cfg_SoundEntry *sound;

		// Close display
		_config_env_close(data);

		// Free stuff
		if (data->app_port)
		{
			struct Message *msg;
			while ((msg = GetMsg(data->app_port)))
				ReplyMsg(msg);
			DeleteMsgPort(data->app_port);
		}
		Att_RemList(data->options, 0);
		Att_RemList(data->path_list, 0);
		Att_RemList(data->mode_list, 0);
		Att_RemList(data->lister_items, 0);
		Att_RemList(data->icon_settings, 0);
		Att_RemList(data->desktop_drives[0], 0);
		Att_RemList(data->desktop_drives[1], 0);
		while ((sound = (Cfg_SoundEntry *)data->sound_list.mlh_Head) && sound->dse_Node.ln_Succ)
		{
			Remove((struct Node *)sound);
			FreeVec(sound);
		}
		FreeAslRequest(data->font_req);
		FreeAslRequest(data->icon_font_req);
		FreeVec(data->config);
		FreeVec(data);
	}
}

// Set initial gadget values
void _config_env_set(config_env_data *data, short option)
{
	short a;

	switch (option)
	{
	// Screen mode
	case ENVIRONMENT_DISPLAY:

		// Attach list of modes
		SetGadgetChoices(data->option_list, GAD_ENVIRONMENT_SCREENMODE_MODE, data->mode_list);

		// Screen width/height
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_SCREENMODE_WIDTH, data->config->screen_width);
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_SCREENMODE_HEIGHT, data->config->screen_height);
		SetGadgetValue(
			data->option_list, GAD_ENVIRONMENT_SCREENMODE_DEFWIDTH, (data->config->screen_flags & SCRFLAGS_DEFWIDTH));
		SetGadgetValue(
			data->option_list, GAD_ENVIRONMENT_SCREENMODE_DEFHEIGHT, (data->config->screen_flags & SCRFLAGS_DEFHEIGHT));

		// Font
		SetGadgetValue(
			data->option_list, GAD_ENVIRONMENT_SCREENMODE_FONTNAME, (IPTR)data->config->font_name[FONT_SCREEN]);
		SetGadgetValue(
			data->option_list, GAD_ENVIRONMENT_SCREENMODE_FONTSIZE, (ULONG)data->config->font_size[FONT_SCREEN]);

		// Initialise screen mode
		_config_env_screenmode_init(data, 1);
		break;

	// Palette
	case ENVIRONMENT_PALETTE:

		// Get palette box
		data->palette_gadget = GetObject(data->option_list, GAD_ENVIRONMENT_PALETTE_PALETTE);

		// Select colour 0
		data->sel_colour = (data->own_screen) ? 0 : 8;
		data->orig_colour_sel = 0;
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_PALETTE_PALETTE, data->sel_colour);

		// Initialise palette gadget
		_config_env_palette_init(data);

		// Number of allocated colours
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_PALETTE_COUNT, data->palette_colours);

		// If not on own screen, can't grab workbench
		if (!data->own_screen)
			DisableObject(data->option_list, GAD_ENVIRONMENT_PALETTE_GRABWB, 1);
		break;

	// Lister display
	case ENVIRONMENT_LISTER_DISPLAY:

		// Status bar text
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_LISTER_STATUS, (IPTR)data->config->status_text);

		// Font
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_LISTER_FONTNAME, (IPTR)data->config->font_name[FONT_DIRS]);
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_LISTER_FONTSIZE, (ULONG)data->config->font_size[FONT_DIRS]);

		// Field titles
		SetGadgetValue(
			data->option_list, GAD_ENVIRONMENT_FIELD_TITLES, data->config->lister_options & LISTEROPTF_TITLES);

		// Hide padlock gadget
		SetGadgetValue(
			data->option_list, GAD_ENVIRONMENT_HIDE_PADLOCK, data->config->lister_options & LISTEROPTF_NO_PADLOCK);

		// Open new listers in dual mode
		SetGadgetValue(
			data->option_list, GAD_ENVIRONMENT_DUAL_DEFAULT, data->config->lister_options & LISTEROPTF_DUAL_DEFAULT);

		// Live folder updates / DOS patches (dopus/DOSPatch ENVARC: var)
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_DOS_PATCH, data->dos_patch_state);
		break;

	// Lister colours
	case ENVIRONMENT_LISTER_COLOURS:

		// Attach list of items
		SetGadgetChoices(data->option_list, GAD_ENVIRONMENT_LISTER_COLOUR_ITEMS, data->lister_items);

		// First selection
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_LISTER_COLOUR_ITEMS, 0);

		// Set initial colour number
		data->colour_number = 0;

		// Get custom pens
		for (a = 0; a < 2; a++)
		{
			// Get custom pen
			data->custom_pen[a] = (short)IPC_Command(data->main_ipc,
													 MAINCMD_GET_PEN,
													 (data->colour_number << 16) | a,
													 (APTR)&data->config->env_Colours[data->colour_number][a],
													 0,
													 REPLY_NO_PORT);
		}

		// Initialise "Example" area
		data->example = GetObject(data->option_list, GAD_ENVIRONMENT_LISTER_EXAMPLE);
		_config_env_update_example(data, data->config->list_format.files_unsel);
		data->colour_selection = data->config->list_format.files_unsel;
		break;

	// Display options
	case ENVIRONMENT_DISPLAY_OPTIONS:

		// Workbench patches
		SetGadgetValue(data->option_list,
					   GAD_ENVIRONMENT_OPTIONS_APPICONS,
					   data->config->display_options & DISPOPTF_SHOW_APPICONS);
		SetGadgetValue(
			data->option_list, GAD_ENVIRONMENT_OPTIONS_APPMENU, data->config->display_options & DISPOPTF_SHOW_TOOLS);
		SetGadgetValue(data->option_list,
					   GAD_ENVIRONMENT_OPTIONS_APPTOOLS,
					   data->config->display_options & DISPOPTF_SHIFT_APPICONS);
		SetGadgetValue(
			data->option_list, GAD_ENVIRONMENT_OPTIONS_HIDEBAD, data->config->display_options & DISPOPTF_HIDE_BAD);
		SetGadgetValue(data->option_list,
					   GAD_ENVIRONMENT_OPTIONS_SHOW_WBLEFTOUTS,
					   data->config->display_options & DISPOPTF_SHOW_WBLEFTOUTS);
		SetGadgetValue(data->option_list,
					   GAD_ENVIRONMENT_OPTIONS_WB_TITLE,
					   data->config->display_options & DISPOPTF_WB_TITLE);
		SetGadgetValue(data->option_list,
					   GAD_ENVIRONMENT_OPTIONS_USE_WBINFO,
					   data->config->display_options & DISPOPTF_USE_WBINFO);
		SetGadgetValue(data->option_list,
					   GAD_ENVIRONMENT_OPTIONS_ENABLE_SHORTCUTS,
					   data->config->env_flags & ENVF_ENABLE_SHORTCUTS);
		SetGadgetValue(data->option_list,
					   GAD_ENVIRONMENT_OPTIONS_SHOW_DT_FIRST,
					   data->config->display_options & DISPOPTF_SHOW_DATATYPES_FIRST);
		break;

	// Pictures
	case ENVIRONMENT_PICTURES:

		// Backdrop pattern
		SetGadgetValue(data->option_list,
					   GAD_ENVIRONMENT_OPTIONS_DISPLAY_BACKDROP,
					   !(data->config->display_options & DISPOPTF_NO_BACKDROP));
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_MAIN_WINDOW_FIELD, (IPTR)data->config->backdrop_prefs);
		SetGadgetValue(data->option_list,
					   GAD_ENVIRONMENT_PICTURE_USE_WBPATTERN,
					   data->config->display_options & DISPOPTF_USE_WBPATTERN);
		for (a = 0; a < 3; a++)
			SetGadgetValue(
				data->option_list, GAD_ENVIRONMENT_PICTURE_DESK_FIELD + a, (IPTR)data->config->env_BackgroundPic[a]);

		// Disable gadgets appropriately
		config_env_fix_picture_gads(data);

		// Themes path
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_THEMES_FIELD, (IPTR)data->config->themes_location);
		break;

	// Output window
	case ENVIRONMENT_OUTPUT_WINDOW:
		_config_env_update_output(data);
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_OUTPUT_DEVICE, (IPTR)data->config->output_device);
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_STACK, data->config->default_stack);
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_CLL, data->config->settings.command_line_length);
		break;

	// Lister options
	case ENVIRONMENT_LISTER_OPTIONS:

		// Device list
		SetGadgetValue(
			data->option_list, GAD_ENVIRONMENT_NEW_DEVICELIST, !(data->config->lister_options & LISTEROPTF_DEVICES));

		// Double-click editing
		SetGadgetValue(
			data->option_list, GAD_ENVIRONMENT_2XCLICK_EDITING, !(data->config->lister_options & LISTEROPTF_2XCLICK));

		// Simple refresh
		SetGadgetValue(
			data->option_list, GAD_ENVIRONMENT_SIMPLE_REFRESH, data->config->lister_options & LISTEROPTF_SIMPLE);

		// Use Snapshot position
		SetGadgetValue(
			data->option_list, GAD_ENVIRONMENT_USE_SNAPSHOT, data->config->lister_options & LISTEROPTF_SNAPSHOT);

		// Name mode popup
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_NAME_POPUP, data->config->lister_options & LISTEROPTF_POPUP);

		// Drop into subdirs
		SetGadgetValue(
			data->option_list, GAD_ENVIRONMENT_DROP_SUBDIR, data->config->lister_options & LISTEROPTF_SUBDROP);

		// No file select on window activation
		SetGadgetValue(data->option_list,
					   GAD_ENVIRONMENT_NO_ACTIVE_SELECT,
					   !(data->config->lister_options & LISTEROPTF_NOACTIVESELECT));

		// Ben Vost zoom mode
		SetGadgetValue(
			data->option_list, GAD_ENVIRONMENT_VOSTY_ZOOM, data->config->lister_options & LISTEROPTF_VOSTY_ZOOM);

		// Full path in lister title
		SetGadgetValue(
			data->option_list, GAD_ENVIRONMENT_FULL_PATH, data->config->lister_options & LISTEROPTF_FULL_PATH);

		// Lister editing
		SetGadgetValue(data->option_list,
					   GAD_ENVIRONMENT_LISTER_EDITING,
					   ((data->config->lister_options & LISTEROPTF_EDIT_BOTH) == LISTEROPTF_EDIT_BOTH)
						   ? 3
						   : ((data->config->lister_options & LISTEROPTF_EDIT_LEFT)
								  ? 1
								  : ((data->config->lister_options & LISTEROPTF_EDIT_MID) ? 2 : 0)));
		break;

	// Lister size
	case ENVIRONMENT_LISTER_SIZE:

		// Update lister size settings
		_config_env_update_listersize(data);
		break;

	// Desktop
	case ENVIRONMENT_DESKTOP: {
		short a;

		// Make busy
		SetWindowBusy(data->window);

		// Two drive lists
		for (a = 0; a < 2; a++)
		{
			// No drives yet?
			if (!data->desktop_drives[a])
			{
				// Build drive list
				_config_env_build_drive_list(data, a);
				_config_env_fix_drive_list(data, (a == 0) ? DESKTOP_HIDE : DESKTOP_HIDE_BAD);
			}
		}

		// Make unbusy
		ClearWindowBusy(data->window);

		// Attach drive list
		SetGadgetChoices(data->option_list, GAD_ENVIRONMENT_HIDDEN_DRIVES, data->desktop_drives[0]);

		// Folder location
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_DESKTOP_LOCATION, (IPTR)data->config->desktop_location);

		// Set default action
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_DESKTOP_DEFAULT, data->config->desktop_popup_default);

		// Popup enabled?
		SetGadgetValue(data->option_list,
					   GAD_ENVIRONMENT_DESKTOP_ENABLE,
					   (data->config->env_flags & ENVF_DESKTOP_FOLDER) ? TRUE : FALSE);
		DisableObject(data->option_list,
					  GAD_ENVIRONMENT_DESKTOP_DEFAULT,
					  (data->config->env_flags & ENVF_DESKTOP_FOLDER) ? FALSE : TRUE);
	}
	break;

	// Icons
	case ENVIRONMENT_ICONS:

		// Need to go through flag list
		if (data->icon_settings)
		{
			Att_Node *node;

			// Remove list of modes
			SetGadgetChoices(data->option_list, GAD_ENVIRONMENT_ICON_SETTINGS, (APTR)~0);

			for (node = (Att_Node *)data->icon_settings->list.lh_Head; node->node.ln_Succ;
				 node = (Att_Node *)node->node.ln_Succ)
			{
				short a = node->data;
				if ((a == 0 && !(data->config->desktop_flags & DESKTOPF_NO_CACHE)) ||
					(a == 1 && !(data->config->desktop_flags & DESKTOPF_NO_REMAP)) ||
					(a == 2 && !(data->config->desktop_flags & DESKTOPF_NO_BORDERS)) ||
					(a == 3 && data->config->desktop_flags & DESKTOPF_TRANSPARENCY) ||
					(a == 4 && !(data->config->desktop_flags & DESKTOPF_NO_ARROW)) ||
					(a == 5 && data->config->desktop_flags & DESKTOPF_SPLIT_LABELS) ||
					(a == 6 && data->config->display_options & DISPOPTF_ICON_POS) ||
					(a == 7 && data->config->display_options & DISPOPTF_REMOPUSPOS) ||
					(a == 8 && data->config->display_options & DISPOPTF_REALTIME_SCROLL) ||
					(a == 9 && data->config->settings.icon_flags & ICONFLAG_DOUNTOICONS) ||
					(a == 10 && data->config->settings.icon_flags & ICONFLAG_AUTOSELECT) ||
					(a == 11 && data->config->settings.icon_flags & ICONFLAG_TRAP_MORE) ||
					(a == 12 && !(data->config->desktop_flags & DESKTOPF_NO_NOLABELS)) ||
					(a == 13 && data->config->desktop_flags & DESKTOPF_SMART_ICONCOPY) ||
					(a == 14 && data->config->desktop_flags & DESKTOPF_FORCE_SPLIT) ||
					(a == 15 && data->config->desktop_flags & DESKTOPF_QUICK_DRAG) ||
					(a == 16 && !(data->config->desktop_flags & DESKTOPF_NO_CUSTOMDRAG)))
				{
					node->node.lve_Flags |= LVEF_SELECTED;
				}
				else
				{
					node->node.lve_Flags &= ~LVEF_SELECTED;
				}
			}

			// Attach list
			SetGadgetChoices(data->option_list, GAD_ENVIRONMENT_ICON_SETTINGS, data->icon_settings);
		}
		break;

	// Icon layout (was dopus/IconSpace*, dopus/IconGrid*, dopus/ReturnOfBenify env vars)
	case ENVIRONMENT_ICON_LAYOUT:
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_ICON_SPACE_X, data->config->env_icon_space_x);
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_ICON_SPACE_Y, data->config->env_icon_space_y);
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_ICON_GRID_X, data->config->env_icon_grid_x);
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_ICON_GRID_Y, data->config->env_icon_grid_y);
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_BENIFY, data->config->env_flags & ENVF_BENIFY);
		break;

	// Icon colours
	case ENVIRONMENT_ICON_COLOURS:

		// Enable NewIcons
		SetGadgetValue(
			data->option_list, GAD_ENVIRONMENT_NEWICONS_ENABLE, data->config->env_NewIconsFlags & ENVNIF_ENABLE);

		// Discourage NewIcons
		SetGadgetValue(data->option_list,
					   GAD_ENVIRONMENT_NEWICONS_DISCOURAGE,
					   data->config->env_NewIconsFlags & ENVNIF_DISCOURAGE);

		// Dithering
		SetGadgetValue(
			data->option_list, GAD_ENVIRONMENT_NEWICONS_DITHERING, data->config->env_NewIconsFlags & ENVNIF_DITHERING);

		// RTG
		SetGadgetValue(
			data->option_list, GAD_ENVIRONMENT_NEWICONS_RTGMODE, data->config->env_NewIconsFlags & ENVNIF_RTG);

		// Precision
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_NEWICONS_PRECISION, data->config->env_NewIconsPrecision);

		if (IconBase->lib_Version >= 44)
		{
			DisableObject(data->option_list, GAD_ENVIRONMENT_NEWICONS_ENABLE, TRUE);
			DisableObject(data->option_list, GAD_ENVIRONMENT_NEWICONS_DISCOURAGE, TRUE);
			DisableObject(data->option_list, GAD_ENVIRONMENT_NEWICONS_DITHERING, TRUE);
			DisableObject(data->option_list, GAD_ENVIRONMENT_NEWICONS_RTGMODE, TRUE);
			DisableObject(data->option_list, GAD_ENVIRONMENT_NEWICONS_PRECISION, TRUE);
		}

		break;

	// Copy
	case ENVIRONMENT_COPY:
		SetGadgetValue(data->option_list, GAD_SETTINGS_COPY_UPDATE, data->config->settings.copy_flags & COPY_UPDATE);
		SetGadgetValue(data->option_list, GAD_SETTINGS_COPY_SETARCHIVE, data->config->settings.copy_flags & COPY_ARC);
		SetGadgetValue(data->option_list, GAD_SETTINGS_COPY_DATESTAMP, data->config->settings.copy_flags & COPY_DATE);
		SetGadgetValue(data->option_list, GAD_SETTINGS_COPY_PROTECTION, data->config->settings.copy_flags & COPY_PROT);
		SetGadgetValue(data->option_list, GAD_SETTINGS_COPY_COMMENT, data->config->settings.copy_flags & COPY_NOTE);
		SetGadgetValue(data->option_list,
					   GAD_SETTINGS_VERBOSE_REPLACE,
					   data->config->settings.replace_flags & REPLACEF_VERBOSE_REPLACE);
		SetGadgetValue(data->option_list,
					   GAD_SETTINGS_CHECK_VERSION,
					   data->config->settings.replace_flags & REPLACEF_CHECK_VERSION);
		break;

	// Delete
	case ENVIRONMENT_DELETE:
		SetGadgetValue(
			data->option_list, GAD_SETTINGS_ASKDELETE_BEFORE, data->config->settings.delete_flags & DELETE_ASK);
		SetGadgetValue(
			data->option_list, GAD_SETTINGS_ASKDELETE_FILES, data->config->settings.delete_flags & DELETE_FILES);
		SetGadgetValue(
			data->option_list, GAD_SETTINGS_ASKDELETE_DIRS, data->config->settings.delete_flags & DELETE_DIRS);
		break;

	// Caching
	case ENVIRONMENT_CACHING:
		SetGadgetValue(data->option_list, GAD_SETTINGS_CACHING_MAX_BUFFERS, data->config->settings.max_buffer_count);
		SetGadgetValue(data->option_list,
					   GAD_SETTINGS_DISABLE_CACHING,
					   data->config->settings.dir_flags & DIRFLAGS_DISABLE_CACHING);
		SetGadgetValue(data->option_list,
					   GAD_SETTINGS_REREAD_CHANGED_BUFFERS,
					   data->config->settings.dir_flags & DIRFLAGS_REREAD_CHANGED);
		//			SetGadgetValue(data->option_list,GAD_SETTINGS_EXPAND_PATHS,data->config->settings.dir_flags&DIRFLAGS_EXPANDPATHS);
		SetGadgetValue(
			data->option_list, GAD_SETTINGS_ENABLE_MUFS, data->config->settings.general_flags & GENERALF_ENABLE_MUFS);
		SetGadgetValue(data->option_list, GAD_SETTINGS_MAX_FILENAME, (ULONG)data->config->settings.max_filename);
		break;

	// Date
	case ENVIRONMENT_DATE:
		SetGadgetValue(data->option_list, GAD_SETTINGS_DATE_FORMAT, data->config->settings.date_format);
		SetGadgetValue(data->option_list, GAD_SETTINGS_DATE_NAMESUB, data->config->settings.date_flags & DATE_SUBST);
		SetGadgetValue(data->option_list, GAD_SETTINGS_DATE_12HOUR, data->config->settings.date_flags & DATE_12HOUR);
		SetGadgetValue(
			data->option_list, GAD_SETTINGS_THOUSANDS_SEPS, data->config->settings.date_flags & DATE_1000SEP);
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_CLOCK_FORMAT, (IPTR)data->config->clock_format);
		break;

	// Hide method
	case ENVIRONMENT_HIDE:

		// Hide method
		SetGadgetValue(data->option_list, GAD_SETTINGS_HIDE_METHOD, data->config->settings.hide_method);

		// Valid fields?
		if (data->config->settings.general_flags & GENERALF_VALID_POPKEY)
		{
			char buffer[128];

			// Default hotkey
			if (data->config->settings.pop_code == 0 &&
				data->config->settings.pop_qual == (IEQUALIFIER_LALT | IEQUALIFIER_LSHIFT | IEQUALIFIER_CONTROL))
				buffer[0] = 0;

			// Otherwise
			else
			{
				// Build key string
				BuildKeyString(data->config->settings.pop_code,
							   data->config->settings.pop_qual,
							   data->config->settings.pop_qual_mask,
							   data->config->settings.pop_qual_same,
							   buffer);
			}

			// Fill out gadget
			SetGadgetValue(data->objlist, GAD_SETTINGS_POPKEY, (IPTR)buffer);
		}

		// Clear string
		else
			SetGadgetValue(data->objlist, GAD_SETTINGS_POPKEY, 0);
		break;

	// Misc
	case ENVIRONMENT_MISC:
		SetGadgetValue(
			data->option_list, GAD_SETTINGS_MOUSE_HOOK, data->config->settings.general_flags & GENERALF_MOUSE_HOOK);
		SetGadgetValue(
			data->option_list, GAD_SETTINGS_QUICK_QUIT, data->config->settings.general_flags & GENERALF_QUICK_QUIT);
		SetGadgetValue(
			data->option_list, GAD_SETTINGS_EXTENDED_KEY, data->config->settings.general_flags & GENERALF_EXTENDED_KEY);
		SetGadgetValue(data->option_list,
					   GAD_SETTINGS_FILETYPE_SNIFFER,
					   data->config->settings.general_flags & GENERALF_FILETYPE_SNIFFER);
		SetGadgetValue(
			data->option_list, GAD_SETTINGS_THIN_BORDERS, data->config->display_options & DISPOPTF_THIN_BORDERS);
		SetGadgetValue(data->option_list, GAD_ENVIRONMENT_SCREEN_TITLE, (IPTR)data->config->scr_title_text);
		SetGadgetValue(data->option_list, GAD_SETTINGS_POPUP_DELAY, (ULONG)data->config->settings.popup_delay);
		SetGadgetValue(data->option_list, GAD_SETTINGS_MAX_OPENWITH, (ULONG)data->config->settings.max_openwith);
		SetGadgetValue(data->option_list, GAD_SETTINGS_WHEEL_SCROLL_LINES, (ULONG)data->config->env_wheel_scroll_lines);
		break;

	// Priority
	case ENVIRONMENT_PRI:
		SetGadgetValue(data->option_list, GAD_SETTINGS_PRI_MAIN, data->config->settings.pri_main[0]);
		SetGadgetValue(data->option_list, GAD_SETTINGS_PRI_MAIN_BUSY, data->config->settings.pri_main[1]);
		SetGadgetValue(data->option_list, GAD_SETTINGS_PRI_LISTER, data->config->settings.pri_lister[0]);
		SetGadgetValue(data->option_list, GAD_SETTINGS_PRI_LISTER_BUSY, data->config->settings.pri_lister[1]);
		break;

	// Path list
	case ENVIRONMENT_PATHLIST:

		// Attach path list
		SetGadgetChoices(data->option_list, GAD_SETTINGS_PATHLIST, data->path_list);

		// Paths enabled?
		SetGadgetValue(
			data->option_list, GAD_SETTINGS_USE_PATHLIST, (data->config->env_flags & ENVF_USE_PATHLIST) ? TRUE : FALSE);
		for (a = GAD_SETTINGS_PATHLIST; a <= GAD_SETTINGS_PATHLIST_PATH; a++)
			if (a != GAD_SETTINGS_PATHLIST_DELETE || !((data->config->env_flags & ENVF_USE_PATHLIST)))
				DisableObject(data->option_list, a, (data->config->env_flags & ENVF_USE_PATHLIST) ? FALSE : TRUE);
		break;

	// Sound list
	case ENVIRONMENT_SOUNDLIST:

		// Attach sound list
		SetGadgetChoices(data->option_list, GAD_SETTINGS_SOUNDLIST, &data->sound_list);
		SetGadgetValue(data->option_list, GAD_SETTINGS_SOUNDLIST, 0);

		// Display soundlist data
		config_env_show_sound(data);

		// Exclusive sounds
		SetGadgetValue(data->option_list,
					   GAD_SETTINGS_EXCLUSIVE_SOUND,
					   data->config->settings.general_flags & GENERALF_EXCLUSIVE_STARTUP_SND);
		break;
	}
}

// Store gadget values
void _config_env_store(config_env_data *data, short option)
{
	short a;

	switch (option)
	{
	// Screen mode
	case ENVIRONMENT_DISPLAY:

		// Store width and height
		data->config->screen_width = GetGadgetValue(data->option_list, GAD_ENVIRONMENT_SCREENMODE_WIDTH);
		data->config->screen_height = GetGadgetValue(data->option_list, GAD_ENVIRONMENT_SCREENMODE_HEIGHT);

		// Default width/height
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_SCREENMODE_DEFWIDTH))
			data->config->screen_flags |= SCRFLAGS_DEFWIDTH;
		else
			data->config->screen_flags &= ~SCRFLAGS_DEFWIDTH;
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_SCREENMODE_DEFHEIGHT))
			data->config->screen_flags |= SCRFLAGS_DEFHEIGHT;
		else
			data->config->screen_flags &= ~SCRFLAGS_DEFHEIGHT;

		// Font
		stccpy(data->config->font_name[FONT_SCREEN],
			   (char *)GetGadgetValue(data->option_list, GAD_ENVIRONMENT_SCREENMODE_FONTNAME),
			   sizeof(data->config->font_name[FONT_SCREEN]));
		if (data->config->font_name[FONT_SCREEN][0] &&
			strlen(data->config->font_name[FONT_SCREEN]) < sizeof(data->config->font_name[FONT_SCREEN]) - 5 &&
			!strstri(data->config->font_name[FONT_SCREEN], ".font"))
			strcat(data->config->font_name[FONT_SCREEN], ".font");
		data->config->font_size[FONT_SCREEN] = GetGadgetValue(data->option_list, GAD_ENVIRONMENT_SCREENMODE_FONTSIZE);
		break;

	// Lister display
	case ENVIRONMENT_LISTER_DISPLAY:

		// Status bar text
		stccpy(data->config->status_text,
			   (char *)GetGadgetValue(data->option_list, GAD_ENVIRONMENT_LISTER_STATUS),
			   sizeof(data->config->status_text));

		// Font
		stccpy(data->config->font_name[FONT_DIRS],
			   (char *)GetGadgetValue(data->option_list, GAD_ENVIRONMENT_LISTER_FONTNAME),
			   sizeof(data->config->font_name[FONT_DIRS]));
		if (data->config->font_name[FONT_DIRS][0] &&
			strlen(data->config->font_name[FONT_DIRS]) < sizeof(data->config->font_name[FONT_DIRS]) - 5 &&
			!strstri(data->config->font_name[FONT_DIRS], ".font"))
			strcat(data->config->font_name[FONT_DIRS], ".font");
		data->config->font_size[FONT_DIRS] = GetGadgetValue(data->option_list, GAD_ENVIRONMENT_LISTER_FONTSIZE);

		// Field titles
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_FIELD_TITLES))
			data->config->lister_options |= LISTEROPTF_TITLES;
		else
			data->config->lister_options &= ~LISTEROPTF_TITLES;

		// Hide padlock gadget
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_HIDE_PADLOCK))
			data->config->lister_options |= LISTEROPTF_NO_PADLOCK;
		else
			data->config->lister_options &= ~LISTEROPTF_NO_PADLOCK;

		// Open new listers in dual mode
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_DUAL_DEFAULT))
			data->config->lister_options |= LISTEROPTF_DUAL_DEFAULT;
		else
			data->config->lister_options &= ~LISTEROPTF_DUAL_DEFAULT;

		// Live folder updates / DOS patches: track current state and only
		// flag for write-back if the *net* change vs. the dialog-open
		// snapshot is non-zero.  Comparing against dos_patch_initial
		// (rather than the previous dos_patch_state) avoids a redundant
		// ENVARC: write when the user toggles ON then OFF, or vice versa.
		// See the success block at the bottom of L_Config_Environment for
		// the SetVar/DeleteVar call.
		{
			BOOL want = (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_DOS_PATCH) != 0);
			data->dos_patch_state = want;
			data->dos_patch_modified = (want != data->dos_patch_initial);
		}
		break;

	// Lister colours
	case ENVIRONMENT_LISTER_COLOURS:

		// Free custom pens
		for (a = 0; a < 2; a++)
		{
			// Free pen
			if (data->custom_pen[a] > -1)
			{
				IPC_Command(data->main_ipc, MAINCMD_RELEASE_PEN, (data->colour_number << 16) | a, 0, 0, 0);
				data->custom_pen[a] = -1;
			}
		}
		break;

	// Display options
	case ENVIRONMENT_DISPLAY_OPTIONS:

		// Reset flags
		data->config->display_options &= ~(DISPOPTF_SHOW_APPICONS | DISPOPTF_SHIFT_APPICONS | DISPOPTF_SHOW_TOOLS |
										   DISPOPTF_HIDE_BAD | DISPOPTF_SHOW_WBLEFTOUTS | DISPOPTF_WB_TITLE |
										   DISPOPTF_USE_WBINFO | DISPOPTF_SHOW_DATATYPES_FIRST);
		data->config->env_flags &= ~ENVF_ENABLE_SHORTCUTS;

		// Workbench patches
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_OPTIONS_APPICONS))
			data->config->display_options |= DISPOPTF_SHOW_APPICONS;
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_OPTIONS_APPTOOLS))
			data->config->display_options |= DISPOPTF_SHIFT_APPICONS;
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_OPTIONS_APPMENU))
			data->config->display_options |= DISPOPTF_SHOW_TOOLS;
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_OPTIONS_HIDEBAD))
			data->config->display_options |= DISPOPTF_HIDE_BAD;
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_OPTIONS_SHOW_WBLEFTOUTS))
			data->config->display_options |= DISPOPTF_SHOW_WBLEFTOUTS;
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_OPTIONS_WB_TITLE))
			data->config->display_options |= DISPOPTF_WB_TITLE;
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_OPTIONS_USE_WBINFO))
			data->config->display_options |= DISPOPTF_USE_WBINFO;
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_OPTIONS_ENABLE_SHORTCUTS))
			data->config->env_flags |= ENVF_ENABLE_SHORTCUTS;
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_OPTIONS_SHOW_DT_FIRST))
			data->config->display_options |= DISPOPTF_SHOW_DATATYPES_FIRST;
		break;

	// Pictures
	case ENVIRONMENT_PICTURES:

		// Reset flags
		data->config->display_options &= ~(DISPOPTF_NO_BACKDROP | DISPOPTF_USE_WBPATTERN);

		// Picture flags
		if (!(GetGadgetValue(data->option_list, GAD_ENVIRONMENT_OPTIONS_DISPLAY_BACKDROP)))
			data->config->display_options |= DISPOPTF_NO_BACKDROP;
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_PICTURE_USE_WBPATTERN))
			data->config->display_options |= DISPOPTF_USE_WBPATTERN;

		// Get prefs fields
		stccpy(data->config->backdrop_prefs,
			   (char *)GetGadgetValue(data->option_list, GAD_ENVIRONMENT_MAIN_WINDOW_FIELD),
			   sizeof(data->config->backdrop_prefs));
		for (a = 0; a < 3; a++)
			stccpy(data->config->env_BackgroundPic[a],
				   (char *)GetGadgetValue(data->option_list, GAD_ENVIRONMENT_PICTURE_DESK_FIELD + a),
				   sizeof(data->config->env_BackgroundPic[a]));

		// Themes path
		stccpy(data->config->themes_location,
			   (char *)GetGadgetValue(data->option_list, GAD_ENVIRONMENT_THEMES_FIELD),
			   sizeof(data->config->themes_location));
		break;

	// Output window
	case ENVIRONMENT_OUTPUT_WINDOW:
	{
		char *name;
		size_t len;

		// Build output string
		lsprintf(data->config->output_window,
				 "%ld/%ld/%ld/%ld/",
				 data->output_dims[0],
				 data->output_dims[1],
				 data->output_dims[2],
				 data->output_dims[3]);

		// Append name without passing a string pointer through RawDoFmt.
		name = (char *)GetGadgetValue(data->option_list, GAD_ENVIRONMENT_OUTPUT_NAME);
		len = strlen(data->config->output_window);
		if (name && len < sizeof(data->config->output_window))
			stccpy(data->config->output_window + len, name, sizeof(data->config->output_window) - len);

		// Get device
		stccpy(data->config->output_device,
			   (char *)GetGadgetValue(data->option_list, GAD_ENVIRONMENT_OUTPUT_DEVICE),
			   sizeof(data->config->output_device));

		// Get default stack
		data->config->default_stack = GetGadgetValue(data->option_list, GAD_ENVIRONMENT_STACK);
		if (data->config->default_stack < STACK_DEFAULT)
			data->config->default_stack = STACK_DEFAULT;

		// Command line length
		data->config->settings.command_line_length = GetGadgetValue(data->option_list, GAD_ENVIRONMENT_CLL);
		if (data->config->settings.command_line_length < 256)
			data->config->settings.command_line_length = 256;
		break;
	}

	// Lister options
	case ENVIRONMENT_LISTER_OPTIONS:

		// Device list
		if (!(GetGadgetValue(data->option_list, GAD_ENVIRONMENT_NEW_DEVICELIST)))
			data->config->lister_options |= LISTEROPTF_DEVICES;
		else
			data->config->lister_options &= ~LISTEROPTF_DEVICES;

		// Double-click editing
		if (!(GetGadgetValue(data->option_list, GAD_ENVIRONMENT_2XCLICK_EDITING)))
			data->config->lister_options |= LISTEROPTF_2XCLICK;
		else
			data->config->lister_options &= ~LISTEROPTF_2XCLICK;

		// Simple refresh
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_SIMPLE_REFRESH))
			data->config->lister_options |= LISTEROPTF_SIMPLE;
		else
			data->config->lister_options &= ~LISTEROPTF_SIMPLE;

		// Use Snapshot
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_USE_SNAPSHOT))
			data->config->lister_options |= LISTEROPTF_SNAPSHOT;
		else
			data->config->lister_options &= ~LISTEROPTF_SNAPSHOT;

		// Name popup
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_NAME_POPUP))
			data->config->lister_options |= LISTEROPTF_POPUP;
		else
			data->config->lister_options &= ~LISTEROPTF_POPUP;

		// Drop into subdirs
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_DROP_SUBDIR))
			data->config->lister_options |= LISTEROPTF_SUBDROP;
		else
			data->config->lister_options &= ~LISTEROPTF_SUBDROP;

		// No file select, etc
		if (!(GetGadgetValue(data->option_list, GAD_ENVIRONMENT_NO_ACTIVE_SELECT)))
			data->config->lister_options |= LISTEROPTF_NOACTIVESELECT;
		else
			data->config->lister_options &= ~LISTEROPTF_NOACTIVESELECT;

		// Ben Vost zoom mode
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_VOSTY_ZOOM))
			data->config->lister_options |= LISTEROPTF_VOSTY_ZOOM;
		else
			data->config->lister_options &= ~LISTEROPTF_VOSTY_ZOOM;

		// Full path in lister title
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_FULL_PATH))
			data->config->lister_options |= LISTEROPTF_FULL_PATH;
		else
			data->config->lister_options &= ~LISTEROPTF_FULL_PATH;

		// Lister editing
		option = GetGadgetValue(data->option_list, GAD_ENVIRONMENT_LISTER_EDITING);
		data->config->lister_options &= ~LISTEROPTF_EDIT_BOTH;
		if (option == 1)
			data->config->lister_options |= LISTEROPTF_EDIT_LEFT;
		else if (option == 2)
			data->config->lister_options |= LISTEROPTF_EDIT_MID;
		else if (option == 3)
			data->config->lister_options |= LISTEROPTF_EDIT_BOTH;
		break;

	// Lister size
	case ENVIRONMENT_LISTER_SIZE:
		break;

	// Desktop
	case ENVIRONMENT_DESKTOP:

		// Folder location
		stccpy(data->config->desktop_location,
			   (char *)GetGadgetValue(data->option_list, GAD_ENVIRONMENT_DESKTOP_LOCATION),
			   sizeof(data->config->desktop_location));

		// Get default action
		data->config->desktop_popup_default = GetGadgetValue(data->option_list, GAD_ENVIRONMENT_DESKTOP_DEFAULT);
		break;

	// Icons
	case ENVIRONMENT_ICONS:

		// Need to go through flag list
		if (data->icon_settings)
		{
			Att_Node *node;

			// Initialise flags
			data->config->desktop_flags = 0;
			data->config->display_options &= ~(DISPOPTF_ICON_POS | DISPOPTF_REMOPUSPOS | DISPOPTF_REALTIME_SCROLL);
			data->config->settings.icon_flags = 0;

			for (node = (Att_Node *)data->icon_settings->list.lh_Head; node->node.ln_Succ;
				 node = (Att_Node *)node->node.ln_Succ)
			{
				short a = node->data;
				if (a == 0 && !(node->node.lve_Flags & LVEF_SELECTED))
					data->config->desktop_flags |= DESKTOPF_NO_CACHE;
				else if (a == 1 && !(node->node.lve_Flags & LVEF_SELECTED))
					data->config->desktop_flags |= DESKTOPF_NO_REMAP;
				else if (a == 2 && !(node->node.lve_Flags & LVEF_SELECTED))
					data->config->desktop_flags |= DESKTOPF_NO_BORDERS;
				else if (a == 3 && node->node.lve_Flags & LVEF_SELECTED)
					data->config->desktop_flags |= DESKTOPF_TRANSPARENCY;
				else if (a == 4 && !(node->node.lve_Flags & LVEF_SELECTED))
					data->config->desktop_flags |= DESKTOPF_NO_ARROW;
				else if (a == 5 && node->node.lve_Flags & LVEF_SELECTED)
					data->config->desktop_flags |= DESKTOPF_SPLIT_LABELS;
				else if (a == 6 && node->node.lve_Flags & LVEF_SELECTED)
					data->config->display_options |= DISPOPTF_ICON_POS;
				else if (a == 7 && node->node.lve_Flags & LVEF_SELECTED)
					data->config->display_options |= DISPOPTF_REMOPUSPOS;
				else if (a == 8 && node->node.lve_Flags & LVEF_SELECTED)
					data->config->display_options |= DISPOPTF_REALTIME_SCROLL;
				else if (a == 9 && node->node.lve_Flags & LVEF_SELECTED)
					data->config->settings.icon_flags |= ICONFLAG_DOUNTOICONS;
				else if (a == 10 && node->node.lve_Flags & LVEF_SELECTED)
					data->config->settings.icon_flags |= ICONFLAG_AUTOSELECT;
				else if (a == 11 && node->node.lve_Flags & LVEF_SELECTED)
					data->config->settings.icon_flags |= ICONFLAG_TRAP_MORE;
				else if (a == 12 && !(node->node.lve_Flags & LVEF_SELECTED))
					data->config->desktop_flags |= DESKTOPF_NO_NOLABELS;
				else if (a == 13 && node->node.lve_Flags & LVEF_SELECTED)
					data->config->desktop_flags |= DESKTOPF_SMART_ICONCOPY;
				else if (a == 14 && node->node.lve_Flags & LVEF_SELECTED)
					data->config->desktop_flags |= DESKTOPF_FORCE_SPLIT;
				else if (a == 15 && node->node.lve_Flags & LVEF_SELECTED)
					data->config->desktop_flags |= DESKTOPF_QUICK_DRAG;
				else if (a == 16 && !(node->node.lve_Flags & LVEF_SELECTED))
					data->config->desktop_flags |= DESKTOPF_NO_CUSTOMDRAG;
			}
		}
		break;

	// Icon layout
	case ENVIRONMENT_ICON_LAYOUT: {
		// Read into signed LONG and clamp BEFORE casting to UWORD: an
		// INTEGER_KIND gadget without GTCustom_MinMax accepts negative input,
		// and (val < 1) on an unsigned val would silently keep the wrap.
		// Note: the (LONG) truncation of GetGadgetValue's IPTR is safe because
		// GTIN_MaxChars=4 in _environment_icon_int_tags constrains values to
		// [-9999, 9999] -- well within int32 range.  If max-chars ever grows
		// beyond 9, switch to (IPTR)/intptr_t to avoid losing the sign bit on
		// x86_64-aros where IPTR is 64-bit.
		LONG val;

		val = (LONG)GetGadgetValue(data->option_list, GAD_ENVIRONMENT_ICON_SPACE_X);
		if (val < 1)
			val = 1;
		data->config->env_icon_space_x = (UWORD)val;

		val = (LONG)GetGadgetValue(data->option_list, GAD_ENVIRONMENT_ICON_SPACE_Y);
		if (val < 1)
			val = 1;
		data->config->env_icon_space_y = (UWORD)val;

		val = (LONG)GetGadgetValue(data->option_list, GAD_ENVIRONMENT_ICON_GRID_X);
		if (val < 1)
			val = 1;
		data->config->env_icon_grid_x = (UWORD)val;

		val = (LONG)GetGadgetValue(data->option_list, GAD_ENVIRONMENT_ICON_GRID_Y);
		if (val < 1)
			val = 1;
		data->config->env_icon_grid_y = (UWORD)val;

		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_BENIFY))
			data->config->env_flags |= ENVF_BENIFY;
		else
			data->config->env_flags &= ~ENVF_BENIFY;
		break;
	}

	// Icon colours
	case ENVIRONMENT_ICON_COLOURS:

		// Flags
		data->config->env_NewIconsFlags = 0;
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_NEWICONS_ENABLE))
			data->config->env_NewIconsFlags |= ENVNIF_ENABLE;
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_NEWICONS_DISCOURAGE))
			data->config->env_NewIconsFlags |= ENVNIF_DISCOURAGE;
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_NEWICONS_DITHERING))
			data->config->env_NewIconsFlags |= ENVNIF_DITHERING;
		if (GetGadgetValue(data->option_list, GAD_ENVIRONMENT_NEWICONS_RTGMODE))
			data->config->env_NewIconsFlags |= ENVNIF_RTG;

		// Precision
		data->config->env_NewIconsPrecision = GetGadgetValue(data->option_list, GAD_ENVIRONMENT_NEWICONS_PRECISION);
		if (data->config->env_NewIconsPrecision < -1)
			data->config->env_NewIconsPrecision = -1;
		else if (data->config->env_NewIconsPrecision > 16)
			data->config->env_NewIconsPrecision = 16;
		break;

	// Copy
	case ENVIRONMENT_COPY:
		data->config->settings.copy_flags = 0;
		data->config->settings.replace_flags = 0;
		if (GetGadgetValue(data->option_list, GAD_SETTINGS_COPY_UPDATE))
			data->config->settings.copy_flags |= COPY_UPDATE;
		if (GetGadgetValue(data->option_list, GAD_SETTINGS_COPY_SETARCHIVE))
			data->config->settings.copy_flags |= COPY_ARC;
		if (GetGadgetValue(data->option_list, GAD_SETTINGS_COPY_DATESTAMP))
			data->config->settings.copy_flags |= COPY_DATE;
		if (GetGadgetValue(data->option_list, GAD_SETTINGS_COPY_PROTECTION))
			data->config->settings.copy_flags |= COPY_PROT;
		if (GetGadgetValue(data->option_list, GAD_SETTINGS_COPY_COMMENT))
			data->config->settings.copy_flags |= COPY_NOTE;
		if (GetGadgetValue(data->option_list, GAD_SETTINGS_VERBOSE_REPLACE))
			data->config->settings.replace_flags |= REPLACEF_VERBOSE_REPLACE;
		if (GetGadgetValue(data->option_list, GAD_SETTINGS_CHECK_VERSION))
			data->config->settings.replace_flags |= REPLACEF_CHECK_VERSION;
		break;

	// Delete
	case ENVIRONMENT_DELETE:
		data->config->settings.delete_flags = 0;
		if (GetGadgetValue(data->option_list, GAD_SETTINGS_ASKDELETE_BEFORE))
			data->config->settings.delete_flags |= DELETE_ASK;
		if (GetGadgetValue(data->option_list, GAD_SETTINGS_ASKDELETE_FILES))
			data->config->settings.delete_flags |= DELETE_FILES;
		if (GetGadgetValue(data->option_list, GAD_SETTINGS_ASKDELETE_DIRS))
			data->config->settings.delete_flags |= DELETE_DIRS;
		break;

	// Caching
	case ENVIRONMENT_CACHING:
		data->config->settings.max_buffer_count = GetGadgetValue(data->option_list, GAD_SETTINGS_CACHING_MAX_BUFFERS);
		data->config->settings.dir_flags &=
			~(DIRFLAGS_DISABLE_CACHING | DIRFLAGS_REREAD_CHANGED | DIRFLAGS_EXPANDPATHS);
		data->config->settings.general_flags &= ~GENERALF_ENABLE_MUFS;
		if (GetGadgetValue(data->option_list, GAD_SETTINGS_DISABLE_CACHING))
			data->config->settings.dir_flags |= DIRFLAGS_DISABLE_CACHING;
		if (GetGadgetValue(data->option_list, GAD_SETTINGS_REREAD_CHANGED_BUFFERS))
			data->config->settings.dir_flags |= DIRFLAGS_REREAD_CHANGED;
		/*
					if (GetGadgetValue(data->option_list,GAD_SETTINGS_EXPAND_PATHS))
						data->config->settings.dir_flags|=DIRFLAGS_EXPANDPATHS;
		*/
		if (GetGadgetValue(data->option_list, GAD_SETTINGS_ENABLE_MUFS))
			data->config->settings.general_flags |= GENERALF_ENABLE_MUFS;
		data->config->settings.max_filename = GetGadgetValue(data->option_list, GAD_SETTINGS_MAX_FILENAME);
		if (data->config->settings.max_filename < 30)
			data->config->settings.max_filename = 30;
		else if (data->config->settings.max_filename > MAX_FILENAME_LEN)
			data->config->settings.max_filename = MAX_FILENAME_LEN;
		break;

	// Date
	case ENVIRONMENT_DATE:
		data->config->settings.date_format = GetGadgetValue(data->option_list, GAD_SETTINGS_DATE_FORMAT);
		data->config->settings.date_flags = 0;
		if (GetGadgetValue(data->option_list, GAD_SETTINGS_DATE_NAMESUB))
			data->config->settings.date_flags |= DATE_SUBST;
		if (GetGadgetValue(data->option_list, GAD_SETTINGS_DATE_12HOUR))
			data->config->settings.date_flags |= DATE_12HOUR;
		if (GetGadgetValue(data->option_list, GAD_SETTINGS_THOUSANDS_SEPS))
			data->config->settings.date_flags |= DATE_1000SEP;
		stccpy(data->config->clock_format,
			   (char *)GetGadgetValue(data->option_list, GAD_ENVIRONMENT_CLOCK_FORMAT),
			   sizeof(data->config->clock_format));
		break;

	// Hide method
	case ENVIRONMENT_HIDE:

		// Get hide method
		data->config->settings.hide_method = GetGadgetValue(data->option_list, GAD_SETTINGS_HIDE_METHOD);

		// Get popkey
		config_env_popkey(data);
		break;

	// Misc
	case ENVIRONMENT_MISC:
		data->config->settings.general_flags &=
			~(GENERALF_MOUSE_HOOK | GENERALF_QUICK_QUIT | GENERALF_EXTENDED_KEY | GENERALF_FILETYPE_SNIFFER);
		data->config->display_options &= ~DISPOPTF_THIN_BORDERS;
		if (GetGadgetValue(data->option_list, GAD_SETTINGS_MOUSE_HOOK))
			data->config->settings.general_flags |= GENERALF_MOUSE_HOOK;
		if (GetGadgetValue(data->option_list, GAD_SETTINGS_QUICK_QUIT))
			data->config->settings.general_flags |= GENERALF_QUICK_QUIT;
		if (GetGadgetValue(data->option_list, GAD_SETTINGS_EXTENDED_KEY))
			data->config->settings.general_flags |= GENERALF_EXTENDED_KEY;
		if (GetGadgetValue(data->option_list, GAD_SETTINGS_FILETYPE_SNIFFER))
			data->config->settings.general_flags |= GENERALF_FILETYPE_SNIFFER;
		if (GetGadgetValue(data->option_list, GAD_SETTINGS_THIN_BORDERS))
			data->config->display_options |= DISPOPTF_THIN_BORDERS;
		stccpy(data->config->scr_title_text,
			   (char *)GetGadgetValue(data->option_list, GAD_ENVIRONMENT_SCREEN_TITLE),
			   sizeof(data->config->scr_title_text));
		data->config->settings.popup_delay = GetGadgetValue(data->option_list, GAD_SETTINGS_POPUP_DELAY);
		data->config->settings.max_openwith = GetGadgetValue(data->option_list, GAD_SETTINGS_MAX_OPENWITH);
		{
			// Signed clamp before UWORD cast (see Icon Layout above).
			LONG val = (LONG)GetGadgetValue(data->option_list, GAD_SETTINGS_WHEEL_SCROLL_LINES);
			if (val < 1)
				val = 1;
			data->config->env_wheel_scroll_lines = (UWORD)val;
		}
		break;

	// Priority
	case ENVIRONMENT_PRI:
		data->config->settings.pri_main[0] = GetGadgetValue(data->option_list, GAD_SETTINGS_PRI_MAIN);
		data->config->settings.pri_main[1] = GetGadgetValue(data->option_list, GAD_SETTINGS_PRI_MAIN_BUSY);
		data->config->settings.pri_lister[0] = GetGadgetValue(data->option_list, GAD_SETTINGS_PRI_LISTER);
		data->config->settings.pri_lister[1] = GetGadgetValue(data->option_list, GAD_SETTINGS_PRI_LISTER_BUSY);
		break;

	// Path list
	case ENVIRONMENT_PATHLIST:
		if (GetGadgetValue(data->option_list, GAD_SETTINGS_USE_PATHLIST))
			data->config->env_flags |= ENVF_USE_PATHLIST;
		else
			data->config->env_flags &= ~ENVF_USE_PATHLIST;
		break;

	// Sound list
	case ENVIRONMENT_SOUNDLIST:

		// Exclusive sounds
		if (GetGadgetValue(data->option_list, GAD_SETTINGS_EXCLUSIVE_SOUND))
			data->config->settings.general_flags |= GENERALF_EXCLUSIVE_STARTUP_SND;
		else
			data->config->settings.general_flags &= ~GENERALF_EXCLUSIVE_STARTUP_SND;
		break;
	}
}

// Check output device is valid
void config_env_check_device(config_env_data *data)
{
	char *device;
	char name[80];

	// Get device
	device = (char *)GetGadgetValue(data->option_list, GAD_ENVIRONMENT_OUTPUT_DEVICE);

	// Valid string?
	if (device && *device)
	{
		char *ptr;
		struct DosList *dos;

		// Copy name
		stccpy(name, device, sizeof(name));

		// Clear colon
		if ((ptr = strchr(name, ':')))
			*ptr = 0;

		// Lock the dos list
		dos = LockDosList(LDF_DEVICES | LDF_READ);

		// See if entry is there.
		//
		// This used to also reject any entry with a running handler process
		// (dol_Task), or with dol_Startup > 511, as heuristics for "this is a
		// filesystem rather than a console handler". Both misclassify modern
		// console handlers. CCON: creates its handler process on first
		// reference, so dol_Task is set as soon as it has been used, and
		// handlers mounted with a string Startup (CRAW:, RAW:) have a
		// dol_Startup BSTR pointer well above 511. Stock CON: trips the
		// dol_Task test too - that went unnoticed only because the fallback
		// value is "CON:" itself, so the reset was invisible.
		//
		// The DOS list offers no reliable way to tell a handler from a
		// filesystem, so just check the device exists, which is what catches
		// typos.
		if (!FindDosEntry(dos, name, LDF_DEVICES))
		{
			// Error
			DisplayBeep(data->window->WScreen);

			// Reset to con:
			stccpy(name, "CON", sizeof(name));
		}

		// Unlock dos list
		UnLockDosList(LDF_DEVICES | LDF_READ);

		// Add colon
		strcat(name, ":");
	}

	// Default to con:
	else
		strcpy(name, "CON:");

	// Refresh gadget
	SetGadgetValue(data->option_list, GAD_ENVIRONMENT_OUTPUT_DEVICE, (IPTR)name);
}

// Get popkey string
BOOL config_env_popkey(config_env_data *data)
{
	IX ix;
	char *ptr;

	// Get key, see if it's invalid
	if ((ptr = (char *)GetGadgetValue(data->objlist, GAD_SETTINGS_POPKEY)) && *ptr)
	{
		// Try to parse
		if (ParseIX(ptr, &ix))
			return 0;

		// Get values
		data->config->settings.pop_code = ix.ix_Code;
		data->config->settings.pop_qual = QualValid(ix.ix_Qualifier);
		data->config->settings.pop_qual_mask = ix.ix_QualMask;
		data->config->settings.pop_qual_same = ix.ix_QualSame;

		// Set 'ok' flag
		data->config->settings.general_flags |= GENERALF_VALID_POPKEY;
	}

	// Set 'invalid' flag
	else
		data->config->settings.general_flags &= ~GENERALF_VALID_POPKEY;

	return 1;
}

// Load environment
void config_env_load(config_env_data *data, UWORD id)
{
	char path[256];
	short ret = 0, ask = 1;
	CFG_ENVR *work;

	// Allocate work buffer
	if (!(work = AllocVec(sizeof(CFG_ENVR), MEMF_CLEAR)))
		return;

	// Set busy pointer
	SetWindowBusy(data->window);

	// Last saved?
	if (id == BUTTONEDIT_MENU_LASTSAVED)
	{
		// Get last saved path
		strcpy(path, data->last_saved);
		ask = 0;
	}

	// Copy path
	else
		strcpy(path, data->settings_name);

	// Default or new?
	if (id == BUTTONEDIT_MENU_NEW || id == BUTTONEDIT_MENU_DEFAULTS)
	{
		Cfg_SoundEntry *sound;

		// Get default environment
		DefaultEnvironment(work);

		// If new, clear filename
		if (id == BUTTONEDIT_MENU_NEW)
		{
			strcpy(path, "DOpus5:Environment/");
			strcat(path, GetString(locale, MSG_UNTITLED));
		}
		ret = READCFG_OK;

		// Initialise sounds
		for (sound = (Cfg_SoundEntry *)data->sound_list.mlh_Head; sound->dse_Node.ln_Succ;
			 sound = (Cfg_SoundEntry *)sound->dse_Node.ln_Succ)
		{
			if (sound->dse_Sound[0])
				sound->dse_Node.lve_Flags |= LVEF_TEMP;
			sound->dse_Sound[0] = 0;
			sound->dse_Volume = 64;
			sound->dse_Flags = 0;
			sound->dse_Node.lve_Flags &= ~LVEF_USE_PEN;
		}
	}

	// Restore?
	else if (id == BUTTONEDIT_MENU_RESTORE)
	{
		Cfg_SoundEntry *sound;

		// Copy from original settings
		CopyMem((char *)data->env->env, (char *)work, sizeof(CFG_ENVR));

		// Free sound list and re-initialise
		while ((sound = (Cfg_SoundEntry *)data->sound_list.mlh_Head) && sound->dse_Node.ln_Succ)
		{
			Remove((struct Node *)sound);
			FreeVec(sound);
		}
		env_init_sounds(data);
		ret = READCFG_OK;
	}

	// Need to load; loop while unsuccessful
	else
		while (!ret)
		{
			struct OpenEnvironmentData *opendata;

			// Get filename
			if (ask && !(config_filereq(data->window, MSG_ENVIRONMENT_OPEN, path, "DOpus5:Environment/", 0)))
				break;

			// Initialise open structure
			if ((opendata = AllocVec(sizeof(struct OpenEnvironmentData), MEMF_CLEAR)))
			{
				opendata->memory = data->env->desktop_memory;
				opendata->volatile_memory = data->env->volatile_memory;
				opendata->flags = OEDF_ENVR | OEDF_SETS | OEDF_SNDX;

				// Read environment
				if (OpenEnvironment(path, opendata))
				{
					Cfg_SoundEntry *sound;
					ret = READCFG_OK;

					// Copy to environment
					CopyMem((char *)&opendata->env, (char *)work, sizeof(CFG_ENVR));

					// Initialise sounds
					for (sound = (Cfg_SoundEntry *)data->sound_list.mlh_Head; sound->dse_Node.ln_Succ;
						 sound = (Cfg_SoundEntry *)sound->dse_Node.ln_Succ)
					{
						Cfg_SoundEntry *newsnd;

						// Is this sound present in the new environment?
						if ((newsnd = (Cfg_SoundEntry *)FindNameI((struct List *)&opendata->soundlist,
																  sound->dse_Node.ln_Name)))
						{
							// Sound has changed?
							if (stricmp(sound->dse_Sound, newsnd->dse_Sound) != 0)
							{
								// Copy new sound details
								strcpy(sound->dse_Sound, newsnd->dse_Sound);
								sound->dse_Volume = newsnd->dse_Volume;
								sound->dse_Flags = newsnd->dse_Flags;

								// Set change flag
								sound->dse_Node.lve_Flags |= LVEF_TEMP;
							}

							// Remove sound from list
							Remove((struct Node *)newsnd);
							FreeMemH(newsnd);
						}

						// Sound must be blank, so clear entry
						else if (sound->dse_Sound[0])
						{
							sound->dse_Sound[0] = 0;
							sound->dse_Volume = 64;
							sound->dse_Flags = 0;
							sound->dse_Node.lve_Flags |= LVEF_TEMP;
						}

						// Is sound valid?
						if (sound->dse_Sound[0])
							sound->dse_Node.lve_Flags |= LVEF_USE_PEN;
						else
							sound->dse_Node.lve_Flags &= ~LVEF_USE_PEN;
					}

					// Any sounds left in newly-loaded environment?
					while ((sound = (Cfg_SoundEntry *)opendata->soundlist.mlh_Head) && sound->dse_Node.ln_Succ)
					{
						Cfg_SoundEntry *copy;

						// Add to our copy of the sound list
						if ((copy = AllocVec(sizeof(Cfg_SoundEntry), MEMF_CLEAR)))
						{
							strcpy(copy->dse_Name, sound->dse_Name);
							copy->dse_Node.ln_Name = copy->dse_Name;
							copy->dse_Node.lve_Pen = DRAWINFO(data->window)->dri_Pens[HIGHLIGHTTEXTPEN];
							strcpy(copy->dse_Sound, sound->dse_Sound);
							copy->dse_Volume = sound->dse_Volume;
							copy->dse_Flags = sound->dse_Flags;
							if (copy->dse_Sound[0])
								copy->dse_Node.lve_Flags |= LVEF_USE_PEN;
							sound->dse_Node.lve_Flags |= LVEF_TEMP;
							AddSorted((struct List *)&data->sound_list, (struct Node *)copy);
						}

						// Remove and free entry
						Remove((struct Node *)sound);
						FreeMemH(sound);
					}
				}
				else
					ret = READCFG_WRONG;
				FreeVec(opendata);
			}

			// Failed to open
			if (ret != READCFG_OK)
			{
				char buf[80];

				// Get error string
				Fault(IoErr(), "", buf, 80);

				// Show error
				if (!(SimpleRequestTags(data->window,
										0,
										GetString(locale, MSG_RETRY_CANCEL),
										GetString(locale, MSG_ERROR_LOADING_BUILD),
										ret,
										buf)))
					break;
				ret = 0;
			}
		}

	// Successful?
	if (ret == READCFG_OK)
	{
		// Bring environment up to date
		UpdateEnvironment(work);

		// Copy new settings
		CopyMem((char *)work, (char *)data->config, sizeof(CFG_ENVR));

		// Store new path
		strcpy(data->settings_name, path);

		// Got a section open?
		if (data->option_list)
		{
			// Refresh display
			_config_env_set(data, data->option);
		}
	}

	// Free work buffer
	FreeVec(work);

	// Clear pointer
	ClearWindowBusy(data->window);
}

// Save
BOOL config_env_save(config_env_data *data, short saveas)
{
	char path[256];
	long ret = 1;
	BOOL retry = 0, res = 0;

	// Set busy pointer
	SetWindowBusy(data->window);

	// Copy path
	strcpy(path, data->settings_name);

	// Loop while unsuccessful
	while (ret)
	{
		// SaveAs?
		if (retry || saveas || !config_valid_path(path))
		{
			// Get filename
			if (!config_filereq(data->window, MSG_ENVIRONMENT_SAVE, path, "DOpus5:Environment/", 1))
				break;
		}

		// Try to save
		if ((ret = IPC_Command(data->main_ipc, MAINCMD_SAVE_ENV, (IPTR)data->config, path, 0, REPLY_NO_PORT)))
		{
			char buf[80];

			// Get error string
			Fault(ret, "", buf, 80);

			// Show error
			if (!(SimpleRequestTags(data->window,
									0,
									GetString(locale, MSG_RETRY_CANCEL),
									GetString(locale, MSG_ERROR_SAVING_BUILD),
									ret,
									buf)))
				break;
		}
	}

	// Successful?
	if (!ret)
	{
		// Store new path
		strcpy(data->settings_name, path);

		// Save pathname for last saved
		strcpy(data->last_saved, path);
		res = 1;
	}

	// Clear pointer
	ClearWindowBusy(data->window);
	return res;
}

// Initialise sound list
void env_init_sounds(config_env_data *data)
{
	Cfg_SoundEntry *sound;
	if (data->script_list)
	{
		Att_Node *node;
		for (node = (Att_Node *)data->script_list->list.lh_Head; node->node.ln_Succ;
			 node = (Att_Node *)node->node.ln_Succ)
		{
			Cfg_SoundEntry *copy;
			if (node->data & SCRIPTF_NO_SOUND)
				continue;
			if ((copy = AllocVec(sizeof(Cfg_SoundEntry), MEMF_CLEAR)))
			{
				strcpy(copy->dse_Name, node->node.ln_Name);
				copy->dse_Node.ln_Name = copy->dse_Name;
				copy->dse_Node.lve_Pen = DRAWINFO(data->window)->dri_Pens[HIGHLIGHTTEXTPEN];
				AddSorted((struct List *)&data->sound_list, (struct Node *)copy);
			}
		}
	}
	GetSemaphore(&data->env->sound_lock, SEMF_SHARED, 0);
	for (sound = (Cfg_SoundEntry *)data->env->sound_list.mlh_Head; sound->dse_Node.ln_Succ;
		 sound = (Cfg_SoundEntry *)sound->dse_Node.ln_Succ)
	{
		Cfg_SoundEntry *copy;
		if ((copy = (Cfg_SoundEntry *)FindNameI((struct List *)&data->sound_list, sound->dse_Name)) ||
			(copy = AllocVec(sizeof(Cfg_SoundEntry), MEMF_CLEAR)))
		{
			if (!copy->dse_Name[0])
			{
				strcpy(copy->dse_Name, sound->dse_Name);
				copy->dse_Node.ln_Name = copy->dse_Name;
				copy->dse_Node.lve_Pen = DRAWINFO(data->window)->dri_Pens[HIGHLIGHTTEXTPEN];
				AddSorted((struct List *)&data->sound_list, (struct Node *)copy);
			}
			strcpy(copy->dse_Sound, sound->dse_Sound);
			copy->dse_Volume = sound->dse_Volume;
			copy->dse_Flags = sound->dse_Flags;
			if (copy->dse_Sound[0])
				copy->dse_Node.lve_Flags |= LVEF_USE_PEN;
		}
	}
	FreeSemaphore(&data->env->sound_lock);
}

// Case-insensitive strstr() function
char *strstri(char *string, char *substring)
{
	int a, len, sublen;

	// See if substring is longer than main string
	len = (strlen(string) - (sublen = strlen(substring))) + 1;
	if (len < 1)
		return 0;

	// Go through string (for the difference in length between the two)
	for (a = 0; a < len; a++)
	{
		// See if substring matches at this point
		if (strnicmp(&string[a], substring, sublen) == 0)
			return string + a;
	}

	// No match
	return 0;
}
