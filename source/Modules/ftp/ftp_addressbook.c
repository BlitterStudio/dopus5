/*

Directory Opus 5
Original APL release version 5.82
Copyright 1993-2012 Jonathan Potter & GP Software
Copyright 2012-2013 DOPUS5 Open Source Team
Copyright 2023-2026 Dimitris Panokostas

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
 *	 1-04-96	Addressbook requester now appears even if config file doesn't exist
 *	13-05-96	ReadArgs() template changed to make the 'HOST' and 'DIR' keywords optional
 *			'PATH' has been added as a synonym for 'DIR'
 *			The 'ANON' and 'ACCT' switches are no longer compulsory
 *	17-05-96	No longer requires screen name to open requester
 *	31-05-96	Now uses Opus ParseArgs() instead of ReadArgs()
 *
 *	jan/feb 97	GJP rewrote - added GUI and stuff
 *
 *	20.02.97	Fixed problem of saving config - it was saved BEFORE it was udated
 *
 *	Jan/Feb 98	GJP Re-wrote gui
 *
 *
 *	4/3/99		Fixed problem with swaplistnodes using
 *			woops wrong list of dg->dg_addrwp->wp_objlist
 *			instead of dg->dg_og->og_SiteList
 *			Also fixed problem with bad list handling (Remove) in same function
 *
 *
 *	11.3.99		Fixed crash when HELP on Options windows
 *			(was incorrectly setting busy on wrong window)
 *
 *	Changes reqd  change windowtofront to progessively activate children
 *
 */

#include <proto/dopus5.h>
#include <proto/configopus.h>
#include <proto/module.h>

#include "ftp.h"
#include "ftp_addressbook.h"
#include "ftp_addrsupp.h"
#include "ftp_addrformat.h"
#include "ftp_ipc.h"
#include "ftp_protocol.h"
#include "ftp_recursive.h"
#include "ftp_util.h"

#define FTP_ADDRBOOK_MIN_SAVED_WIDTH 320
#define FTP_ADDRBOOK_MIN_SAVED_HEIGHT 140

#define SetFlag(v, f) ((v) |= (f))
#define ClearFlag(v, f) ((v) &= ~(f))
#define ToggleFlag(v, f) ((v) ^= (f))
#define FlagIsSet(v, f) (((v) & (f)) != 0)
#define FlagIsClear(v, f) (((v) & (f)) == 0)

static int site_protocol(struct site_entry *entry)
{
	int protocol = entry && entry->se_protocol ? entry->se_protocol : FTP_PROTOCOL_FTP;

	if (entry)
		protocol = ftp_protocol_infer_from_connection(protocol, entry->se_port, entry->se_anon);

	return protocol;
}

enum {
	FTP_CONNECTION_STANDARD,
	FTP_CONNECTION_FTPS_EXPLICIT,
	FTP_CONNECTION_SFTP,
};

static struct ftp_environment *site_env(struct display_globals *dg, struct site_entry *entry)
{
	if (!entry)
		return NULL;

	if (!entry->se_env && dg && dg->dg_og)
		entry->se_env = &dg->dg_og->og_oc.oc_env;

	return entry->se_env;
}

static void site_make_custom_env(struct display_globals *dg, struct site_entry *entry)
{
	struct ftp_environment *env;

	if (!entry)
		return;

	if (entry->se_has_custom_env)
	{
		entry->se_env = &entry->se_env_private;
		return;
	}

	env = site_env(dg, entry);
	if (env)
		entry->se_env_private = *env;

	entry->se_env = &entry->se_env_private;
	entry->se_has_custom_env = TRUE;
}

static void site_set_tls_mode(struct display_globals *dg, struct site_entry *entry, int tls_mode)
{
	struct ftp_environment *env;

	if (!entry)
		return;

	env = site_env(dg, entry);
	if (!entry->se_has_custom_env && env && env->e_tls_mode == tls_mode)
		return;

	site_make_custom_env(dg, entry);
	entry->se_env_private.e_tls_mode = tls_mode;
}

static int site_connection(struct site_entry *entry)
{
	if (site_protocol(entry) == FTP_PROTOCOL_SFTP)
		return FTP_CONNECTION_SFTP;

	if (entry && entry->se_env && ftp_tls_mode_uses_control_tls(entry->se_env->e_tls_mode))
		return FTP_CONNECTION_FTPS_EXPLICIT;

	return FTP_CONNECTION_STANDARD;
}

static const char *site_connection_short_name(struct site_entry *entry)
{
	switch (site_connection(entry))
	{
	case FTP_CONNECTION_FTPS_EXPLICIT:
		return "FTPS";
	case FTP_CONNECTION_SFTP:
		return "SFTP";
	}

	return "FTP";
}

static int connection_cycle_value(int connection)
{
	switch (connection)
	{
	case FTP_CONNECTION_FTPS_EXPLICIT:
		return 1;
	case FTP_CONNECTION_SFTP:
		return 2;
	}

	return 0;
}

static int connection_from_cycle_value(LONG value)
{
	switch (value)
	{
	case 1:
		return FTP_CONNECTION_FTPS_EXPLICIT;
	case 2:
		return FTP_CONNECTION_SFTP;
	}

	return FTP_CONNECTION_STANDARD;
}

static void set_site_protocol(struct site_entry *entry, int protocol, LONG current_port)
{
	int old_protocol;

	if (!entry)
		return;

	old_protocol = site_protocol(entry);
	if (protocol != FTP_PROTOCOL_SFTP)
		protocol = FTP_PROTOCOL_FTP;

	entry->se_protocol = protocol;
	if (current_port <= 0 || current_port == ftp_protocol_default_port(old_protocol))
		entry->se_port = ftp_protocol_default_port(protocol);
	else
		entry->se_port = current_port;

	if (protocol == FTP_PROTOCOL_SFTP)
		entry->se_anon = FALSE;
}

static void set_site_connection(struct display_globals *dg, struct site_entry *entry, int connection, LONG current_port)
{
	if (!entry)
		return;

	if (connection == FTP_CONNECTION_SFTP)
	{
		set_site_protocol(entry, FTP_PROTOCOL_SFTP, current_port);
		entry->se_anon = FALSE;
	}
	else
	{
		set_site_protocol(entry, FTP_PROTOCOL_FTP, current_port);
		site_set_tls_mode(dg,
						  entry,
						  connection == FTP_CONNECTION_FTPS_EXPLICIT ? FTP_TLS_MODE_EXPLICIT : FTP_TLS_MODE_OFF);
	}
}

static void set_site_host_from_text(struct display_globals *dg,
									struct site_entry *entry,
									const char *host_text,
									LONG current_port)
{
	const char *host = host_text ? host_text : "";
	const char *without_scheme;
	int protocol;

	if (!entry)
		return;

	if (!entry->se_protocol)
		entry->se_protocol = FTP_PROTOCOL_FTP;

	if (ftp_protocol_from_url_scheme(host, &without_scheme, &protocol))
	{
		int connection = protocol == FTP_PROTOCOL_SFTP ? FTP_CONNECTION_SFTP : FTP_CONNECTION_STANDARD;
		const char *without_tls_scheme;
		int tls_mode;

		if (ftp_tls_mode_from_url_scheme(host, &without_tls_scheme, &tls_mode))
		{
			connection =
				ftp_tls_mode_uses_control_tls(tls_mode) ? FTP_CONNECTION_FTPS_EXPLICIT : FTP_CONNECTION_STANDARD;
			without_scheme = without_tls_scheme;
		}

		set_site_connection(dg, entry, connection, current_port);
		host = without_scheme;
	}

	stccpy(entry->se_host, (char *)host, HOSTNAMELEN + 1);
}

static void update_connection_gadgets(ObjectList *objlist,
									  struct display_globals *dg,
									  struct site_entry *entry,
									  UWORD protocol_gadget,
									  UWORD port_gadget,
									  UWORD anon_gadget,
									  UWORD user_gadget,
									  UWORD password_gadget)
{
	int protocol;

	if (!objlist || !entry)
		return;

	protocol = site_protocol(entry);
	site_env(dg, entry);
	SetGadgetValue(objlist, protocol_gadget, connection_cycle_value(site_connection(entry)));
	SetGadgetValue(objlist, port_gadget, entry->se_port);
	SetGadgetValue(objlist, anon_gadget, protocol == FTP_PROTOCOL_SFTP ? FALSE : entry->se_anon);
	DisableObject(objlist, anon_gadget, protocol == FTP_PROTOCOL_SFTP);
	DisableObject(objlist, user_gadget, protocol != FTP_PROTOCOL_SFTP && entry->se_anon);
	DisableObject(objlist, password_gadget, protocol != FTP_PROTOCOL_SFTP && entry->se_anon);
}

static void update_connection_from_protocol_gadget(ObjectList *objlist,
												   struct display_globals *dg,
												   struct site_entry *entry,
												   UWORD protocol_gadget,
												   UWORD port_gadget,
												   UWORD anon_gadget,
												   UWORD user_gadget,
												   UWORD password_gadget)
{
	LONG port;
	int connection;

	if (!objlist || !entry)
		return;

	port = GetGadgetValue(objlist, port_gadget);
	connection = connection_from_cycle_value(GetGadgetValue(objlist, protocol_gadget));
	set_site_connection(dg, entry, connection, port > 65535 ? 0 : port);
	update_connection_gadgets(
		objlist, dg, entry, protocol_gadget, port_gadget, anon_gadget, user_gadget, password_gadget);
}

static void update_connection_from_host_gadget(ObjectList *objlist,
											   struct display_globals *dg,
											   struct site_entry *entry,
											   UWORD host_gadget,
											   UWORD protocol_gadget,
											   UWORD port_gadget,
											   UWORD anon_gadget,
											   UWORD user_gadget,
											   UWORD password_gadget)
{
	char *host;
	LONG port;

	if (!objlist || !entry)
		return;

	host = (char *)GetGadgetValue(objlist, host_gadget);
	port = GetGadgetValue(objlist, port_gadget);
	entry->se_port = port > 65535 ? 0 : port;
	set_site_host_from_text(dg, entry, host, entry->se_port);
	SetGadgetValue(objlist, host_gadget, (IPTR)entry->se_host);
	update_connection_gadgets(
		objlist, dg, entry, protocol_gadget, port_gadget, anon_gadget, user_gadget, password_gadget);
}

#if defined(__amigaos3__) || defined(__AROS__) || defined(__amigaos4__)
struct Device *InputBase = NULL;
#else
struct Library *InputBase = NULL;
#endif

extern ConfigWindow ftp_environment_window, ftp_connect_window, ftp_list_window;
extern ObjectDef ftp_options_objects[], ftp_connect_objects[], ftp_opt_lister[], ftp_list_objects[];
extern SubOptionHandle ftp_custom_suboptions[], ftp_default_suboptions[];
extern MenuData options_menus[], site_menus[];

VOID free_address_book(struct opusftp_globals *);  // ftp_main.c
static void close_addrbook(struct window_params *, BOOL);
static BOOL end_edit(struct window_params *, BOOL);
static void end_connect(struct window_params *, BOOL);
static void end_options(struct window_params *, BOOL);
static void end_small_site_list(struct window_params *, BOOL);
static void display_edit_gadgets(struct display_globals *dg, struct window_params *wp);

/*********************** code ***********************************/

static BOOL get_qualified(VOID)
{
// struct Library *InputBase;
#ifdef __amigaos4__
	struct InputIFace *IInput;
#endif
	struct IOStdReq input_req;
	BOOL qualified = FALSE;

#ifdef __AROS__
	input_req.io_Message.mn_Length = sizeof(input_req);
#endif

	if (!(OpenDevice("input.device", 0, (struct IORequest *)&input_req, 0)))
	{
		// Get input base
#ifdef __amigaos4__
		InputBase = input_req.io_Device;
		IInput = (struct InputIFace *)GetInterface((struct Library *)InputBase, "main", 1, NULL);
#else
		InputBase = input_req.io_Device;
#endif

		// See if shift is held down
		if (PeekQualifier() & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
			qualified = TRUE;

#ifdef __amigaos4__
		DropInterface((struct Interface *)IInput);
#endif

		// Close input device
		CloseDevice((struct IORequest *)&input_req);
		InputBase = NULL;
	}

	return qualified;
}

static VOID store_window_pos(struct Window *window)
{
	struct IBox pos;

	// Save window position
	pos.Left = window->LeftEdge;
	pos.Top = window->TopEdge;
	pos.Width = window->Width - window->BorderLeft - window->BorderRight;
	pos.Height = window->Height - window->BorderTop - window->BorderBottom;

	if (pos.Width < FTP_ADDRBOOK_MIN_SAVED_WIDTH || pos.Height < FTP_ADDRBOOK_MIN_SAVED_HEIGHT)
		return;

	SavePos("dopus/windows/ftp", (struct IBox *)&pos, window->RPort->TxHeight);
}

static void sitename_from_host(char *name, const char *host)
{
	if (!name)
		return;

	if (host && *host && !strnicmp(host, "ftp.", 4) &&
		(strstr(host + 4, ".com") == host + strlen(host) - 4 || strstr(host + 4, ".com.") || strstr(host + 4, ".co.")))
	{
		stccpy(name, host + 4, strchr(host + 4, '.') - host - 4 + 1);
		*name = toupper(*name);
	}
	else
		stccpy(name, host, HOSTNAMELEN + 1);
}

// check if a window is one of my windows

static struct window_params *my_window(struct display_globals *dg, struct Window *win)
{
	Att_Node *node;
	struct window_params *wp;

	for (node = (Att_Node *)dg->dg_wp_list->list.lh_Head; node->node.ln_Succ; node = (Att_Node *)node->node.ln_Succ)
	{
		wp = (struct window_params *)node->data;

		if (wp->wp_win == win)
			return (wp);
	}

	return (NULL);
}

/****************************************************************
 *
 *	Bring window to front and
 *	 progressively activate a window -> child stack if any
 *
 *	WARNING: RECURSIVE FUNCTION
 */
static VOID myWindowToFront(struct window_params *wp, BOOL activate)
{
	// bring designated window to front
	WindowToFront(wp->wp_win);

	// activate it if required
	if (activate)
		ActivateWindow(wp->wp_win);

	// if sub task then activate this one
	if (wp->wp_extask_ipc)
	{
		// then bring it to front
		IPC_Command(wp->wp_extask_ipc, IPC_ACTIVATE, 0, 0, 0, 0);
	}

	// then work on children
	if (wp->wp_children)
	{
		struct List *list;

		list = &wp->wp_children->list;

		if (list && !IsListEmpty(list))
		{
			struct Node *node;
			for (node = list->lh_Head; node->ln_Succ; node = node->ln_Succ)
				myWindowToFront((struct window_params *)((Att_Node *)node)->data, activate);
		}
	}
}

/*********************************************************
 *
 *	Create a dummy copy of the default options
 *	copy defaults to our structure and set ptr
 *	and clear format marker
 */

static VOID create_dummy_env(struct display_globals *dg, struct site_entry *e)
{
	*(&e->se_env_private) = *(&dg->dg_og->og_oc.oc_env);
	e->se_env = &e->se_env_private;

	// Clear format marker
	e->se_env->e_custom_format = FALSE;

	// mark as no custom env yet
	e->se_has_custom_env = FALSE;
}

/*************************** mardi gras routines ********************/

static DragInfo *
address_drag_start(struct display_globals *dg, int item, int left, int top, int width, int height, int xoff, int yoff)
{
	DragInfo *draginf;
	short len;
	struct TextExtent extent;
	Att_Node *drag_node;

	if (width == 0 || height == 0)
		return 0;

	dg->dg_drag_item = item;

	// Find drag node

	if (!(drag_node = Att_FindNode(dg->dg_og->og_SiteList, item)))
		return 0;

	// We want to drag outside the window.
	// Docs say pass NULL for window, RastPort * for rastport.
	// In fact, we still pass our window, and NULL for rastport.

	if ((draginf = GetDragInfo(dg->dg_addrwp->wp_win,  // window,
							   NULL,				   // rastport
							   width,
							   height,
							   DRAGF_NEED_GELS | DRAGF_CUSTOM)))  // need_gels
	{
		// Set pens and font
		SetAPen(&draginf->drag_rp, DRAWINFO(dg->dg_addrwp->wp_win)->dri_Pens[FILLTEXTPEN]);
		SetDrMd(&draginf->drag_rp, JAM1);
		SetRast(&draginf->drag_rp, DRAWINFO(dg->dg_addrwp->wp_win)->dri_Pens[FILLPEN]);
		SetFont(&draginf->drag_rp, dg->dg_addrwp->wp_win->RPort->Font);

		// Get length that will fit
		len = TextFit(dg->dg_addrwp->wp_win->RPort,
					  drag_node->node.ln_Name,
					  strlen(drag_node->node.ln_Name),
					  &extent,
					  0,
					  1,
					  draginf->width - 2,
					  draginf->height);

		// Draw text
		Move(&draginf->drag_rp, 2, draginf->drag_rp.TxBaseline);
		Text(&draginf->drag_rp, drag_node->node.ln_Name, len);

		// Build the drag shadow mask
		// Don't just overwrite the flags.  They already contain DRAGF_TRANSPARENT.
		// And the system will barf without it.

		draginf->flags |= DRAGF_OPAQUE;

		// Create mask
		GetDragMask(draginf);

		// Get drag offset
		dg->dg_drag_x = xoff;
		dg->dg_drag_y = yoff;

		// Set pointer to draginfo so we know we're dragging
		dg->dg_draginfo = draginf;

		// Show initial drag image
		ShowDragImage(
			draginf, dg->dg_addrwp->wp_win->WScreen->MouseX + xoff, dg->dg_addrwp->wp_win->WScreen->MouseY + yoff);

		// Save old flags
		dg->dg_old_winflags = dg->dg_addrwp->wp_win->Flags;
		dg->dg_old_idcmp = dg->dg_addrwp->wp_win->IDCMPFlags;

		// Set mousemove reporting and ticks
		dg->dg_addrwp->wp_win->Flags |= WFLG_REPORTMOUSE | WFLG_RMBTRAP;
		dg->dg_addrwp->wp_win->Flags &= ~WFLG_WINDOWTICKED;
		ModifyIDCMP(dg->dg_addrwp->wp_win, dg->dg_addrwp->wp_win->IDCMPFlags | IDCMP_INTUITICKS | IDCMP_MOUSEBUTTONS);

		// Start deadlock timer
		StartTimer(dg->dg_timer, 0, 500000);

		// Initialise tick count
		dg->dg_tick_count = 1;
		dg->dg_last_tick = 0;
	}

	return draginf;
}

static void address_drag_copy(struct display_globals *dg, struct window_params *wp)
{
	Att_Node *drag_node;

	// Find drag node
	drag_node = Att_FindNode(dg->dg_og->og_SiteList, dg->dg_drag_item);

	if (drag_node)
	{
		struct site_entry *newe;
		newe = (struct site_entry *)drag_node->data;

		copy_site_entry(dg->dg_og, wp->wp_se_copy, newe);

		display_edit_gadgets(dg, wp);
	}
}

/*********************************************************
 *
 *
 *	Arrange the items in the listview via drag and drop
 *
 *	swap != 0, swap items rather than inserting in new pos
 */

static void address_drag_arrange(struct display_globals *dg, int swap)
{
	GL_Object *obj;
	int x, y;
	ULONG drop_item;
	Att_Node *drag_node, *drop_node;

	if ((obj = GetObject(dg->dg_addrwp->wp_objlist, GAD_FTP_SITES)))
	{
		x = dg->dg_og->og_screen->MouseX - dg->dg_addrwp->wp_win->LeftEdge;
		y = dg->dg_og->og_screen->MouseY - dg->dg_addrwp->wp_win->TopEdge;

		// Dropped on listview?
		if (CheckObjectArea(obj, x, y))
		{
			drop_item = (x << 16) | y;

			D(bug("x %ld y %ld di %ld\n  ", (long)x, (long)y, (long)drop_item));

			GetAttr(DLV_GetLine, GADGET(obj), &drop_item);

			D(bug("di %ld\n  ", (long)drop_item));

			// Item dragged onto itself?
			if (dg->dg_drag_item == drop_item)
				return;

			// Find drag node
			drag_node = Att_FindNode(dg->dg_og->og_SiteList, dg->dg_drag_item);

			if (drag_node)
			{
				// Detach item list using SetGadgetChoices
				SetGadgetChoices(dg->dg_addrwp->wp_objlist, GAD_FTP_SITES, (APTR)~0);

				// Find drop node
				if (drop_item != -1)
					drop_node = Att_FindNode(dg->dg_og->og_SiteList, drop_item);
				else
					drop_node = 0;

				if (drop_node)
				{
					if (swap)
					{
						// Swap two entries?
						SwapListNodes((struct List *)dg->dg_og->og_SiteList, (struct Node *)drag_node, (struct Node *)drop_node);
					}
					else
					{
						Remove((struct Node *)drag_node);
						Insert(
							dg->dg_og->og_SiteList, (struct Node *)drag_node, (struct Node *)drop_node->node.ln_Pred);
					}
				}
				else
				{
					// Otherwise drag entry to end of list
					Remove((struct Node *)drag_node);
					AddTail((struct List *)dg->dg_og->og_SiteList, (struct Node *)drag_node);
				}

				// Reattach item list using SetGadgetChoices
				SetGadgetChoices(dg->dg_addrwp->wp_objlist, GAD_FTP_SITES, dg->dg_og->og_SiteList);
			}
		}
	}
}

static int address_drag_to_backdrop(struct display_globals *dg)
{
	char path[PATHLEN + 1];
	char command[256 + 1] = "";
	Att_Node *node;
	struct site_entry *entry;
	char *name;
	int i;
	int ok = 0;
	DOpusCallbackInfo *infoptr = &dg->dg_og->og_hooks;

	// Find entry in phonebook
	if ((node = Att_FindNode(dg->dg_og->og_SiteList, dg->dg_drag_item)))
	{
		entry = (struct site_entry *)node->data;

		if ((name = AllocVec(strlen(entry->se_name) + 1, MEMF_ANY)))
		{
			for (i = 0;; ++i)
			{
				name[i] = entry->se_name[i];

				if (name[i] == 0)
					break;

				else if (strchr(":/", name[i]))
					name[i] = '_';
			}

			// Get desktop path
			DC_CALL1(infoptr, dc_GetDesktop, DC_REGA0, path);
			// dg->dg_og->og_hooks.dc_GetDesktop( path );

			// Get site name
			AddPart(path, name, PATHLEN + 1);

			// Build command
			sprintf(command, "FTPConnect SITE \"%s\"", entry->se_name);

			// Create the function
			if (CreateFunctionFile(path, INST_COMMAND, command, "DOpus5:Icons/FTPSite"))
			{
				ok = 1;

				// Strip filename from path again
				*PathPart(path) = 0;

				// Make icon appear
				DC_CALL1(infoptr, dc_CheckDesktop, DC_REGA0, path);
				// dg->dg_og->og_hooks.dc_CheckDesktop( path );
			}
			FreeVec(name);
		}
	}

	return ok;
}

/******************************************************************
 *
 *	Drag an entry onto a function editor, button, button editor,
 *	menu editor, hotkey editor, or filetype editor.
 */

static int address_drag_to_editor(struct display_globals *dg, IPCData *ipc, int x, int y)
{
	char buffer[256 + 1];
	Att_Node *node;
	struct site_entry *entry;
	Point *pos;
	Cfg_Instruction *ins;
	Cfg_ButtonFunction *func;
	Cfg_Button *button;
	int ok = 0;

	if ((pos = AllocVec(sizeof(Point), MEMF_CLEAR)))
	{
		pos->x = x;
		pos->y = y;

		if ((func = NewButtonFunction(0, FTYPE_LEFT_BUTTON)))
		{
			if ((node = Att_FindNode(dg->dg_og->og_SiteList, dg->dg_drag_item)))
			{
				char label[80];

				entry = (struct site_entry *)node->data;

				/************* New Stuff here *****************/

				// Build label for function
				lsprintf(label, "%s %s", site_connection_short_name(entry), entry->se_name);

				// Allocate label
				if ((func->label = AllocMemH(0, strlen(label) + 1)))
					strcpy(func->label, label);

				// Allocate name
				if ((func->node.ln_Name = AllocMemH(0, strlen(label) + 1)))
					strcpy(func->node.ln_Name, label);

				/************* New Stuff here *****************/

				lsprintf(buffer, "FTPConnect SITE \"%s\"", entry->se_name);

				if ((ins = NewInstruction(0, INST_COMMAND, buffer)))
				{
					AddTail((struct List *)&func->instructions, (struct Node *)ins);

					if ((button = NewButton(0)))
					{
						AddTail((struct List *)&button->function_list, (struct Node *)func);

						IPC_Command(ipc, BUTTONEDIT_CLIP_BUTTON, 0, button, pos, 0);
						ok = 1;
					}
				}
			}

			if (!ok)
				FreeButtonFunction(func);
		}

		if (!ok)
			FreeVec(pos);
	}

	return ok;
}

static int address_drag_to_lister(struct display_globals *dg, IPCData *ipc)
{
	Att_Node *node;
	struct site_entry *e;
	int ok = 0;
	struct connect_msg *cm;

	if ((node = Att_FindNode(dg->dg_og->og_SiteList, dg->dg_drag_item)))
	{
		e = (struct site_entry *)node->data;

		// no need to get_blank_connectmsg() since we overwrite anyway
		if ((cm = AllocVec(sizeof(struct connect_msg), MEMF_CLEAR)))
		{
			copy_site_entry(dg->dg_og, &cm->cm_site, e);
			cm->cm_protocol = site_protocol(e);
			cm->cm_site.se_protocol = cm->cm_protocol;

			// if anon the blank user and password
			if (site_protocol(e) != FTP_PROTOCOL_SFTP && e->se_anon)
			{
				*cm->cm_site.se_user = 0;
				*cm->cm_site.se_pass = 0;
			}

			stccpy(cm->cm_opus, dg->dg_opusport, PORTNAMELEN);

			cm->cm_handle = (IPTR)IPCDATA(ipc);

			// send IPC_CONNECT to MAIN proccess so it can open new connection
			// in curent lister we dropped onto.

			IPC_Command(dg->dg_og->og_main_ipc, IPC_CONNECT, CONN_OPT_PROTOCOL, 0, cm, 0);

			ok = 1;
		}
	}

	return (ok);
}

/****************************************************************
 *
 *	option = 0, cancel drag
 *	option = 1, drop
 *	option = 2, shift drop
 */

static void address_drag_end(struct display_globals *dg, int option)
{
	int x, y;
	int ok = 0;

	x = dg->dg_addrwp->wp_win->WScreen->MouseX;
	y = dg->dg_addrwp->wp_win->WScreen->MouseY;

	// Free drag stuff
	FreeDragInfo(dg->dg_draginfo);
	dg->dg_draginfo = 0;

	// Restore window flags & IDCMP
	dg->dg_addrwp->wp_win->Flags = dg->dg_old_winflags;
	ModifyIDCMP(dg->dg_addrwp->wp_win, dg->dg_old_idcmp);

	// Abort timer
	StopTimer(dg->dg_timer);

	// Drag dropped?
	if (option)
	{
		struct Window *win = 0;
		struct window_params *mywp;
		struct Layer *layer;

		LockLayerInfo(&dg->dg_og->og_screen->LayerInfo);

		if ((layer = WhichLayer(&dg->dg_og->og_screen->LayerInfo, x, y)))
			win = layer->Window;

		UnlockLayerInfo(&dg->dg_og->og_screen->LayerInfo);

		if (win)
		{
			// Dropped on one of my windows
			if ((mywp = my_window(dg, win)))
			{
				switch (mywp->wp_type)
				{
				// Address window? but not during edit!
				case WT_ADR:
					if (!dg->dg_edit_count)
					{
						address_drag_arrange(dg, option == 2 ? 1 : 0);
						ok = 1;
					}
					break;

				case WT_CON:
				case WT_NEW:
					address_drag_copy(dg, mywp);
					ok = 1;
					break;

				case WT_EDIT:
				case WT_ADD:
				case WT_OPT:
				case WT_DEFOPT:
				case WT_ADRLIST:

				default:
					break;
				}
			}
			else
			{
				IPCData *win_ipc = 0;
				ULONG win_id = 0;

				// Get window ID
				win_id = GetWindowID(win);

				Forbid();  // while getting IPC

				// Got IPC?
				if ((win_ipc = (IPCData *)GetWindowAppPort(win)))
				{
					switch (win_id)
					{
					case WINDOW_BACKDROP:
						ok = address_drag_to_backdrop(dg);
						break;

					case WINDOW_FUNCTION_EDITOR:
					case WINDOW_BUTTON_CONFIG:
					case WINDOW_BUTTONS:
						ok = address_drag_to_editor(dg, win_ipc, x, y);
						break;

					case WINDOW_LISTER:
						ok = address_drag_to_lister(dg, win_ipc);
						break;

					default:
						D(bug("** dropped on registered window\n"));
						break;
					}
				}
				Permit();  // Okay to multitask once the IPC has been used
			}
		}  // if win
	}

	// Drag aborted?
	else
	{
		ok = 1;
	}

	if (!ok)
		DisplayBeep(dg->dg_og->og_screen);
}

/***************** gui addressbook ****************************/

/*****************************************************************
 *
 *	Check if site list is available and in memory,
 *	 If not then load it
 */

static BOOL make_sitelist_available(struct display_globals *dg)
{
	if (!dg->dg_og->og_SiteList)
		read_build_addressbook(dg->dg_og, dg->dg_ipc);

	// still not there?

	if (!dg->dg_og->og_SiteList)
	{
		DisplayBeep(dg->dg_og->og_screen);
		return (FALSE);
	}

	return (TRUE);
}

/***************************************************
 *	Set menu states for GUI
 *
 */

static void set_menu_state(struct MenuItem *item, BOOL state)
{
	if (state)
		SetFlag(item->Flags, ITEMENABLED);
	else
		ClearFlag(item->Flags, ITEMENABLED);
}

// set main addressbook menus while child window is up
// to disable things we don't want to happen

static void set_ftp_menu(struct window_params *wp, BOOL state)
{
	struct MenuItem *item;

	if (wp)
	{
		if ((item = FindMenuItem(wp->wp_win->MenuStrip, MENU_FTP_SAVEAS)))
			set_menu_state(item, state);

		if ((item = FindMenuItem(wp->wp_win->MenuStrip, MENU_FTP_SAVE)))
			set_menu_state(item, state);

		if ((item = FindMenuItem(wp->wp_win->MenuStrip, MENU_FTP_IMPORT)))
			set_menu_state(item, state);

		if ((item = FindMenuItem(wp->wp_win->MenuStrip, MENU_FTP_IMPORT_AMFTP)))
			set_menu_state(item, state);

		if ((item = FindMenuItem(wp->wp_win->MenuStrip, MENU_FTP_OPEN)))
			set_menu_state(item, state);

		DisableObject(wp->wp_objlist, GAD_FTP_SAVE, !state);
	}
}

/**************************************************************
 *
 *	Compare two configurations
 *	adjust for ListFormat differences
 */

static int config_different(struct ftp_config *new, struct ftp_config *def)
{
	int result;

	result = memcmp(new, def, offsetof(struct ftp_config, oc_env.e_listformat));

	// if there is a custom format then check if we changed it
	if (new->oc_env.e_custom_format)
		result |= CompareListFormat(&new->oc_env.e_listformat, &def->oc_env.e_listformat);

	return (result);
}

/**************************************************************
 *
 *	Compare two environments including ListFormat differences
 */

static int env_different(struct ftp_environment *new, struct ftp_environment *def)
{
	int result;

	result = memcmp(new, def, offsetof(struct ftp_environment, e_listformat));

	// if there is a custom format then check if we changed it
	if (new->e_custom_format)
		result |= CompareListFormat(&new->e_listformat, &def->e_listformat);

	return (result);
}

/**************************************************************
 *
 *	Compare an environment against the ftp default
 *	adjust for ListFormat differences
 */

static int env_not_default(struct display_globals *dg, struct ftp_environment *new)
{
	int format;
	int result;

	// adjust for formats! Grrr
	format = dg->dg_og->og_oc.oc_env.e_custom_format;
	dg->dg_og->og_oc.oc_env.e_custom_format = 0;

	result = env_different(new, &dg->dg_og->og_oc.oc_env);

	dg->dg_og->og_oc.oc_env.e_custom_format = format;

	return (result);
}

VOID set_reset_button(struct display_globals *dg, struct window_params *wp)
{
	BOOL disabled = FALSE;
	struct MenuItem *item;

	// compare current version with actual global default values
	if (wp->wp_type == WT_DEFOPT)
		disabled = config_different(&dg->dg_oc, &dg->dg_default_oc);  // !0 are different?
	else
	{
		disabled = env_not_default(dg, &wp->wp_se_copy->se_env_private);
	}

	// this gadget in main objects not subobjects
	DisableObject(wp->wp_objlist, GAD_ENV_RESET_DEFAULT, !disabled);

	// update menu items

	if ((item = FindMenuItem(wp->wp_win->MenuStrip, MENU_ENV_RESET_DEFAULTS)))
		set_menu_state(item, disabled);

	if ((item = FindMenuItem(wp->wp_win->MenuStrip, MENU_ENV_LAST_SAVED)))
		set_menu_state(item, disabled);

	if ((item = FindMenuItem(wp->wp_win->MenuStrip, MENU_ENV_LAST_SAVED)))
		set_menu_state(item, disabled);

	if (wp->wp_type == WT_OPT)
	{
		disabled = FALSE;

		if (wp->wp_se_real && wp->wp_se_real->se_has_custom_env)
			disabled = env_different(&wp->wp_se_copy->se_env_private, &wp->wp_se_real->se_env_private);

		if ((item = FindMenuItem(wp->wp_win->MenuStrip, MENU_ENV_RESTORE)))
			set_menu_state(item, disabled);
	}
}

static void reset_default_options(struct window_params *wp)
{
	struct display_globals *dg;

	dg = wp->wp_dg;

	if (wp->wp_type == WT_DEFOPT)
	{
		// reset current copy to real system defaults
		set_config_to_default(&dg->dg_oc);
	}
	else
	{
		// overcopy current version being edited with current default
		*(&wp->wp_se_copy->se_env_private) = *(&dg->dg_og->og_oc.oc_env);

		wp->wp_se_copy->se_env = &dg->dg_og->og_oc.oc_env;
		wp->wp_se_copy->se_has_custom_env = FALSE;

		// adjust for global settings being different for format meaning ?
		wp->wp_se_copy->se_env_private.e_custom_format = FALSE;
	}
}

static void restore_options(struct window_params *wp)
{
	struct display_globals *dg;

	dg = wp->wp_dg;

	if (wp->wp_type == WT_OPT)
	{
		if (wp->wp_se_real->se_has_custom_env)
		{
			// overcopy current version being edited with last used
			*(&wp->wp_se_copy->se_env_private) = *(&wp->wp_se_real->se_env_private);
			wp->wp_se_copy->se_env = &wp->wp_se_copy->se_env_private;
		}
		else
		{
			reset_default_options(wp);
		}

		wp->wp_se_copy->se_has_custom_env = wp->wp_se_real->se_has_custom_env;
	}
	else
	{
		// restore to last used default set

		*(&dg->dg_oc) = *(&dg->dg_og->og_oc);
	}
}

/***************************************************************
 *
 * Check limits for gadgets and redisplay if exceeded
 *
 */

static void check_bounds(struct display_globals *dg, struct IntuiMessage *msg, int max, int def)
{
	struct window_params *wp;
	int val;
	ObjectList *objlist;

	wp = FINDWP(msg->IDCMPWindow);

	// is an options window?
	if (wp->wp_SubOptionsHandle)
		objlist = wp->wp_sub_objlist;
	else
		objlist = wp->wp_objlist;

	val = GetGadgetValue(objlist, ((struct Gadget *)msg->IAddress)->GadgetID);

	if (val > max)
		SetGadgetValue(objlist, ((struct Gadget *)msg->IAddress)->GadgetID, def);
}

/************************************************************
 *
 *	Check if a specific addressbook entry is currently open and displayed
 *	Returns the window if so otherwise NULL.
 */

static struct window_params *entry_in_use(struct display_globals *dg, struct site_entry *entry)
{
	Att_Node *node;
	struct window_params *wp;

	for (node = (Att_Node *)dg->dg_wp_list->list.lh_Head; node->node.ln_Succ; node = (Att_Node *)node->node.ln_Succ)
	{
		wp = (struct window_params *)node->data;

		if (wp->wp_se_real == entry)
			return (wp);
	}

	return (NULL);
}

/************************************************************
 *
 *	Check if a site is currentin the site list
 * 	Match by BOTH name AND host
 */

Att_Node *check_duplicate_entry(struct display_globals *dg, struct site_entry *e)
{
	struct List *list;
	struct Node *node, *next;
	Att_Node *found = NULL;

	list = &dg->dg_og->og_SiteList->list;

	if (list && !IsListEmpty(list))
	{
		if ((node = list->lh_Head))
		{
			while ((next = node->ln_Succ))
			{
				struct site_entry *site;

				site = (struct site_entry *)((Att_Node *)node)->data;

				if (!stricmp(e->se_name, site->se_name) && !stricmp(e->se_host, site->se_host))
				{
					found = (Att_Node *)node;
					break;
				}
				node = next;
			}
		}
	}

	return (found);
}

static void set_new_config(struct display_globals *dg, struct window_params *wp)
{
	struct opusftp_globals *ogp;

	ogp = dg->dg_og;

	if (config_different(&dg->dg_oc, &ogp->og_oc))
	{
		LONG ipcflags = 0;

		if (ogp->og_log_open != dg->dg_log_open)
		{
			ogp->og_log_open = dg->dg_log_open;
			ipcflags |= ID_LOGON_F;
		}

		if (stricmp(ogp->og_oc.oc_logname, dg->dg_oc.oc_logname))
			ipcflags |= ID_LOGNAME_F;

		// copy new one over orig
		*(&ogp->og_oc) = *(&dg->dg_oc);

		// if log details have changed then signal mainprocess to check them
		// window can be default options ONLY then imsg is valid
		// or addressbook wnbe it ois child so not valid

		if (ipcflags)
			IPC_Command(dg->dg_og->og_main_ipc, IPC_UPDATECONFIG, ipcflags, 0, 0, REPLY_NO_PORT);
	}
}

static void send_connect(struct display_globals *dg, int selection, struct window_params *wp)
{
	struct site_entry *e;
	struct connect_msg *cm;
	Att_Node *node;

	if (dg->dg_og->og_main_ipc)
	{
		// Should be passed the screen pointer (multiple Opuses)
		if (dg->dg_og->og_screen)
		{
			if ((node = Att_FindNode(dg->dg_og->og_SiteList, selection)))
			{
				e = (struct site_entry *)node->data;

				/*
				 *	Send connect msg to main
				 *	In the future the requester will stay open and can send connect messages
				 *	at any time
				 */

				// no need to get_blank_connectmsg() since we overwrite anyway

				if ((cm = AllocVec(sizeof(struct connect_msg), MEMF_CLEAR)))
				{
					copy_site_entry(dg->dg_og, &cm->cm_site, e);
					cm->cm_protocol = site_protocol(e);
					cm->cm_site.se_protocol = cm->cm_protocol;

					// if anon the blank user and password
					if (site_protocol(e) != FTP_PROTOCOL_SFTP && e->se_anon)
					{
						*cm->cm_site.se_user = 0;
						*cm->cm_site.se_pass = 0;
					}

					stccpy(cm->cm_opus, dg->dg_opusport, PORTNAMELEN);

					IPC_Command(dg->dg_og->og_main_ipc, IPC_CONNECT, CONN_OPT_PROTOCOL, 0, cm, 0);
				}
			}
		}
	}
}

/*********************************************************
 *
 * Build a sub-option list for visual display for rhs menu style display
 *
 */

static Att_List *build_sub_options(SubOptionHandle *list)
{
	Att_List *options;
	if ((options = Att_NewList(0)))
	{
		int a;
		// Add names to list
		for (a = 0; list[a].num > -1; a++)
			Att_NewNode(options, GetString(locale, list[a].name), a, ADDNODE_SORT);
	}
	return (options);
}

static void change_menus(MenuData *menu, int id, IPTR textval)
{
	int a;

	for (a = 0; menu[a].type != NM_END; a++)
	{
		if (menu[a].id == id)
		{
			menu[a].name = textval;
			break;
		}
	}
}

/*********************************************************
 *
 * search the objectdef list (ftp_data.c)
 * to find a specific object by gadgetid
 *
 */

static ObjectDef *find_ObjectDef(ObjectDef *obj, UWORD id)
{
	while (obj->type)
	{
		if (obj->gadgetid == id)
			return (obj);

		obj++;
	}

	return (NULL);
}

/********************************************************
 *
 *	ALL code to close or end a config window comes here
 *	and is then handled by window type and associated function
 *
 *	EXCEPT close_addressbook which is handled directly unless IPC_QUIT msg
 *
 */
static void close_conwin_by_type(struct window_params *wp, BOOL success)
{
	// does window have a spawned process or requester?
	if (wp->wp_extask_ipc)
	{
		// then send it a quit message
		IPC_Command(wp->wp_extask_ipc, IPC_QUIT, 0, 0, 0, REPLY_NO_PORT);

		// wait until subtask has completed
		ObtainSemaphore(&wp->wp_extask_semaphore);

		ReleaseSemaphore(&wp->wp_extask_semaphore);
	}

	switch (wp->wp_type)
	{
	case WT_ADR:  // Can only be one! Should be last closed
		close_addrbook(wp, success);
		break;

	case WT_EDIT:
	case WT_NEW:
	case WT_ADD:
		end_edit(wp, success);
		break;

	case WT_OPT:
	case WT_DEFOPT:
		end_options(wp, success);
		break;

	case WT_CON:
		end_connect(wp, success);
		break;

	case WT_ADRLIST:
		end_small_site_list(wp, success);
		break;

	default:
		break;
	}
}

/**************************************************************
 *
 *	Perform the actual close and tidy up of any config window
 *	Free options list
 *	Free Child list - children must have gone buy now
 *	CloseConfigWindow
 *	Remove busy pointer from parent if any?
 *	Remove node from main window list
 *	Free allocated wp structure
 *
 */

static void close_configwin(struct window_params *wp)
{
	// free the options list from any env display

	if (wp->wp_options_list)
	{
		Att_RemList(wp->wp_options_list, 0);
		wp->wp_options_list = NULL;
	}

	if (wp->wp_sub_objlist)
		wp->wp_sub_objlist = 0;

	// any children should have left home by now
	if (wp->wp_children)
	{
		Att_RemList(wp->wp_children, 0);
		wp->wp_children = NULL;
	}

	// Close sub-window
	CloseConfigWindow(wp->wp_win);

	// remove us from the main window dg list
	Att_RemNode(wp->wp_node);

	// free data
	FreeVec(wp);
}

/**************************************************************
 *
 *	Close a child window ONLY
 *	children must have gone buy now
 *	Must have a parent
 *	Remove busy pointer from parent
 *	Remove this window from parents child list
 *
 */

static void close_a_child(struct window_params *wp)
{
	Att_Node *node;
	struct window_params *parentwp;

	if (!(parentwp = wp->wp_parentwp))
	{
		DisplayBeep(wp->wp_dg->dg_og->og_screen);
		return;
	}

	// remove us from parent child list
	if ((node = Att_FindNodeData(parentwp->wp_children, (IPTR)wp)))
		Att_RemNode(node);

	// Remove busy pointer from parent
	if (parentwp->wp_type != WT_ADR)
		ClearWindowBusy(parentwp->wp_win);

	// close actual config window itself
	close_configwin(wp);
}

/***********************************************************
 *
 * Close a child window but check if has children
 * if so then recursively find children and close them
 *
 * starting from last child close all children
 * handling close window with cleanup related window type
 *
 * WARNING: RECURSIVE FUNCTION
 *
 * nodes are removed from the various window lists by sub funcs()
 *
 */

static void close_child(struct window_params *wp)
{
	Att_Node *node;
	struct List *list;

	// has children ? Then close them first!

	if (wp->wp_children)
	{
		list = &wp->wp_children->list;

		while (list->lh_TailPred != (struct Node *)list)  // IsListEmpty ?
		{
			node = (Att_Node *)list->lh_TailPred;
			close_conwin_by_type((struct window_params *)node->data, FALSE);
		}
	}

	close_a_child(wp);
}

/***********************************************************
 *
 * starting from last entry close all open windows
 * handling close window with cleanup related window type
 *
 * This routine really only for EMERGENCY shutdown?
 */

static void close_all_windows(struct display_globals *dg)
{
	Att_Node *node;
	struct List *list;

	list = &dg->dg_wp_list->list;

	while (list->lh_TailPred != (struct Node *)list)  // IsListEmpty ?
	{
		node = (Att_Node *)list->lh_TailPred;
		close_conwin_by_type((struct window_params *)node->data, FALSE);
	}
}

/*************************************************************************
 *
 * Alloc wp and set wp data details
 * open a config window, add objects and add to internal list of windows
 * Initialise the child list
 * Set back pointer Window->UserData->userdata == wp
 *
 */

static struct window_params *open_configwin(struct display_globals *dg,
											NewConfigWindow *newwin,
											ObjectDef *objects,
											WORD type)
{
	struct window_params *wp;
	int catalog_no;

	switch (type)
	{
	case WT_EDIT:
		catalog_no = MSG_FTP_EDIT_TITLE;
		break;
	case WT_NEW:
		catalog_no = MSG_FTP_NEW_TITLE;
		break;

	case WT_OPT:
		catalog_no = MSG_FTP_CUSTOM_OPTIONS_TITLE;
		break;

	case WT_DEFOPT:
		catalog_no = MSG_FTP_DEFAULT_OPTIONS_TITLE;
		break;

	case WT_CON:
		catalog_no = MSG_FTP_CONNECT_TITLE;
		break;

	case WT_ADD:
		catalog_no = MSG_FTP_ADD_TITLE;
		break;

	case WT_ADRLIST:
	default:
		catalog_no = MSG_FTP_TITLE;
		break;
	}

	newwin->title = GetString(locale, catalog_no);

	if ((wp = AllocVec(sizeof(struct window_params), MEMF_CLEAR)))
	{
		// init child list
		if ((wp->wp_children = Att_NewList(LISTF_POOL)))
		{
			// open the window
			if ((wp->wp_win = OpenConfigWindow(newwin)))
			{
				// add the gadget data
				if ((wp->wp_objlist = AddObjectList(wp->wp_win, objects)))
				{
					// add this window to main list
					if ((wp->wp_node = Att_NewNode(dg->dg_wp_list, NULL, (IPTR)wp, 0)))
					{
						wp->wp_dg = dg;
						wp->wp_type = type;

						// semaphore for extra stuff attached to window
						// such as format requester task. Stops closure of
						// window until external proceses completed.

						InitSemaphore(&wp->wp_extask_semaphore);

						// set back pointer in userdata structure
						// cannot use win->UserData so use next available

						DATA(wp->wp_win)->userdata = (IPTR)wp;

						return (wp);
					}
				}
				// Failed to add objects
				CloseConfigWindow(wp->wp_win);
			}
			Att_RemList(wp->wp_children, 0);
		}
		FreeVec(wp);
	}

	// failed

	DisplayBeep(dg->dg_og->og_screen);
	return (NULL);
}

/************************************
 *
 * open a CHILD configwindow ONLY
 * add new window to parent's child list
 * Make parent busy if required
 *
 */

static struct window_params *open_childwin(struct display_globals *dg,
										   ConfigWindow *dims,
										   ObjectDef *objects,
										   struct window_params *parentwp,
										   WORD type,
										   ULONG flags)
{
	NewConfigWindow newwin;
	struct window_params *wp;
	Att_Node *node;

	newwin.locale = locale;
	newwin.font = 0;
	newwin.parent = parentwp->wp_win;
	newwin.dims = dims;
	newwin.flags = WINDOW_AUTO_KEYS | WINDOW_REQ_FILL | WINDOW_VISITOR | flags;
	newwin.port = dg->idcmp_port;

	if ((wp = open_configwin(dg, &newwin, objects, type)))
	{
		wp->wp_parentwp = parentwp;

		// add this window to parents children list
		if ((node = Att_NewNode(parentwp->wp_children, NULL, (IPTR)wp, 0)))
		{
			// Opened ok?
			// Make parent busy unless main addresbook
			if (wp->wp_parentwp && wp->wp_parentwp->wp_type != WT_ADR)
				SetWindowBusy(wp->wp_parentwp->wp_win);

			return (wp);
		}

		// failed then just close config window
		close_configwin(wp);
	}
	return (NULL);
}

// disable save menu if no change to site list

static void adjust_save_menu(struct display_globals *dg)
{
	struct MenuItem *item;

	if ((item = FindMenuItem(dg->dg_addrwp->wp_win->MenuStrip, MENU_FTP_SAVE)))
		set_menu_state(item, (FlagIsSet(dg->dg_config_changed, CHANGED_ADR)));
}
/*************************************************************
 *
 *	update and redisplay text gadgets in Addressbook window
 *
 */

static void display_main_gadgets(struct display_globals *dg, int number)
{
	Att_Node *node;

	if ((node = Att_FindNode(dg->dg_og->og_SiteList, number)))
	{
		// if there is a node then display it

		struct site_entry *e;

		e = (struct site_entry *)node->data;
		SetGadgetValue(dg->dg_addrwp->wp_objlist, GAD_FTP_SITE_NAME, (IPTR)e->se_name);
		SetGadgetValue(dg->dg_addrwp->wp_objlist, GAD_FTP_HOST_NAME, (IPTR)e->se_host);
		SetGadgetValue(dg->dg_addrwp->wp_objlist, GAD_FTP_PORT, e->se_port);
		SetGadgetValue(dg->dg_addrwp->wp_objlist, GAD_FTP_ANON, e->se_anon);
		dg->dg_selected = number;

		SetGadgetValue(dg->dg_addrwp->wp_objlist, GAD_FTP_CUSTOM_OPTIONS, e->se_has_custom_env);
	}
	else
	{
		// else clear the display
		SetGadgetValue(dg->dg_addrwp->wp_objlist, GAD_FTP_SITE_NAME, (IPTR) "");
		SetGadgetValue(dg->dg_addrwp->wp_objlist, GAD_FTP_HOST_NAME, (IPTR) "");
		SetGadgetValue(dg->dg_addrwp->wp_objlist, GAD_FTP_PORT, 21);
		SetGadgetValue(dg->dg_addrwp->wp_objlist, GAD_FTP_ANON, TRUE);
		dg->dg_selected = 0;

		SetGadgetValue(dg->dg_addrwp->wp_objlist, GAD_FTP_CUSTOM_OPTIONS, FALSE);
	}

	adjust_save_menu(dg);
}

static void display_connect_gadgets(struct display_globals *dg, struct window_params *wp)
{
	ObjectList *objlist;
	struct site_entry *e;

	objlist = wp->wp_objlist;
	e = wp->wp_se_copy;

	SetGadgetValue(objlist, GAD_CONNECT_NAME, (IPTR)e->se_name);
	SetGadgetValue(objlist, GAD_CONNECT_PROTOCOL, connection_cycle_value(site_connection(e)));
	SetGadgetValue(objlist, GAD_CONNECT_HOST, (IPTR)e->se_host);
	SetGadgetValue(objlist, GAD_CONNECT_PORT, e->se_port);
	SetGadgetValue(objlist, GAD_CONNECT_ANON, site_protocol(e) == FTP_PROTOCOL_SFTP ? FALSE : e->se_anon);

	SetGadgetValue(objlist, GAD_CONNECT_USER, (IPTR)e->se_user);
	SetGadgetValue(objlist, GAD_CONNECT_PASSWORD, (IPTR)e->se_pass);

	SetGadgetValue(objlist, GAD_CONNECT_DIR, (IPTR)e->se_path);

	update_connection_gadgets(objlist,
							  dg,
							  e,
							  GAD_CONNECT_PROTOCOL,
							  GAD_CONNECT_PORT,
							  GAD_CONNECT_ANON,
							  GAD_CONNECT_USER,
							  GAD_CONNECT_PASSWORD);
}

static void display_edit_gadgets(struct display_globals *dg, struct window_params *wp)
{
	ObjectList *objlist;
	struct site_entry *e;

	objlist = wp->wp_objlist;
	e = wp->wp_se_copy;

	SetGadgetValue(objlist, GAD_EDIT_NAME, (IPTR)e->se_name);
	SetGadgetValue(objlist, GAD_EDIT_PROTOCOL, connection_cycle_value(site_connection(e)));
	SetGadgetValue(objlist, GAD_EDIT_HOST, (IPTR)e->se_host);
	SetGadgetValue(objlist, GAD_EDIT_PORT, e->se_port);
	SetGadgetValue(objlist, GAD_EDIT_ANON, site_protocol(e) == FTP_PROTOCOL_SFTP ? FALSE : e->se_anon);

	SetGadgetValue(objlist, GAD_EDIT_USER, (IPTR)e->se_user);
	SetGadgetValue(objlist, GAD_EDIT_PASSWORD, (IPTR)e->se_pass);

	SetGadgetValue(objlist, GAD_EDIT_DIR, (IPTR)e->se_path);

	update_connection_gadgets(objlist,
							  dg,
							  e,
							  GAD_EDIT_PROTOCOL,
							  GAD_EDIT_PORT,
							  GAD_EDIT_ANON,
							  GAD_EDIT_USER,
							  GAD_EDIT_PASSWORD);

	SetGadgetValue(objlist, GAD_EDIT_CUSTOM_OPTIONS, e->se_has_custom_env);
	DisableObject(objlist, GAD_EDIT_SET_CUSTOM_OPTIONS, !e->se_has_custom_env);

	// activate first gadget
	ActivateStrGad(GADGET(GetObject(wp->wp_objlist, GAD_EDIT_NAME)), wp->wp_win);
}

/************************************
 *
 *	fill out options gadgets from our temp copy
 *
 */

static void display_options_gadgets(struct window_params *wp)
{
	ObjectList *objlist;
	struct ftp_environment *env;
	struct display_globals *dg;

	dg = wp->wp_dg;

	objlist = wp->wp_sub_objlist;

	if (wp->wp_type == WT_DEFOPT)
	{
		struct ftp_config *tmp_oc;
		tmp_oc = &dg->dg_oc;
		env = &tmp_oc->oc_env;
	}
	else
	{
		if (wp && wp->wp_se_copy)
		{
			// set for copy in site data
			env = &wp->wp_se_copy->se_env_private;
		}
		else
			return;
	}

	switch (wp->wp_option)
	{
	case ENV_SUB_GLOBAL: {
		struct ftp_config *tmp_oc;
		tmp_oc = &dg->dg_oc;

		if (tmp_oc->oc_user_password)
			strcpy(dg->dg_anonpass, tmp_oc->oc_anonpass);
		else
		{
			if (((struct globals *)dg->dg_ipc->userdata)->g_socketbase)
				getuseraddress(dg->dg_anonpass);
			else
				stccpy(dg->dg_anonpass, "Unknown - No Socket", PASSWORDLEN);
		}

		DisableObject(objlist, GAD_ENV_ANON_PASSWORD, !(((struct globals *)dg->dg_ipc->userdata)->g_socketbase != NULL));
		SetGadgetValue(objlist, GAD_ENV_ANON_PASSWORD, (IPTR)dg->dg_anonpass);

		SetGadgetValue(objlist, GAD_ENV_LOG_FILE, (IPTR)tmp_oc->oc_logname);
		SetGadgetValue(objlist, GAD_ENV_LOG_ENABLE, tmp_oc->oc_enable_log);
		SetGadgetValue(objlist, GAD_ENV_DEBUG, tmp_oc->oc_log_debug);
		SetGadgetValue(objlist, GAD_ENV_NO_KEEP_ALIVE, tmp_oc->oc_no_keep_alive);

		SetGadgetValue(objlist, GAD_ENV_ADDR_AUTO, tmp_oc->oc_addr_auto);
		SetGadgetValue(objlist, GAD_ENV_ADDR_DC, tmp_oc->oc_addr_dc);
	}

	break;

	case ENV_SUB_COPYFLAGS:
		SetGadgetValue(objlist, GAD_ENV_COPY_TYPE, env->e_copy_type);

		SetGadgetValue(objlist, GAD_ENV_COPY_SET_ARCHIVE, env->e_copy_set_archive);
		SetGadgetValue(objlist, GAD_ENV_COPY_URL_COMMENT, env->e_url_comment);
		SetGadgetValue(objlist, GAD_ENV_REPLACE, env->e_copy_replace);

		SetGadgetValue(objlist, GAD_ENV_COPY_RESCAN, env->e_rescan);

		SetGadgetValue(objlist, GAD_ENV_COPY_RECURSIVE_SPECIAL, env->e_recursive_special);

		break;

	case ENV_SUB_COPYATTR:

		SetGadgetValue(objlist, GAD_ENV_COPY_OPUS_DEFAULT, env->e_copy_opus_default);
		SetGadgetValue(objlist, GAD_ENV_COPY_DATESTAMP, env->e_copy_datestamp);
		SetGadgetValue(objlist, GAD_ENV_COPY_PROTECTION, env->e_copy_protection);
		SetGadgetValue(objlist, GAD_ENV_COPY_COMMENT, env->e_copy_comment);

		DisableObject(objlist, GAD_ENV_COPY_DATESTAMP, env->e_copy_opus_default);
		DisableObject(objlist, GAD_ENV_COPY_PROTECTION, env->e_copy_opus_default);
		DisableObject(objlist, GAD_ENV_COPY_COMMENT, env->e_copy_opus_default);

		break;

	case ENV_SUB_LISTER:
		SetGadgetValue(objlist, GAD_ENV_TOOLBAR, (IPTR)env->e_toolbar);
		SetGadgetValue(objlist, GAD_ENV_CUST_FORMAT, env->e_custom_format);
		break;

	case ENV_SUB_MISC:
		SetGadgetValue(objlist, GAD_ENV_KEEP_LAST_DIR, env->e_keep_last_dir);
		SetGadgetValue(objlist, GAD_ENV_PASSIVE, env->e_passive);
		SetGadgetValue(objlist, GAD_ENV_SPECIAL_DIR, env->e_special_dir);

		SetGadgetValue(objlist, GAD_ENV_SAFE_LINKS, env->e_safe_links);
		SetGadgetValue(objlist, GAD_ENV_UNK_LINKS, env->e_unk_links_file);
		SetGadgetValue(objlist, GAD_ENV_TIMEOUT, (ULONG)env->e_timeout);
		SetGadgetValue(objlist, GAD_ENV_LIST_UPDATE, (ULONG)env->e_list_update);
		break;

	case ENV_SUB_DISPLAY:
		SetGadgetValue(objlist, GAD_ENV_SHOW_STARTUP_MSG, env->e_show_startup);
		SetGadgetValue(objlist, GAD_ENV_SHOW_DIR_MSG, env->e_show_dir);
		SetGadgetValue(objlist, GAD_ENV_PROGRESS_WINDOW, env->e_progress_window);
		SetGadgetValue(objlist, GAD_ENV_TRANSFER_DETAILS, !env->e_transfer_details);
		break;

	case ENV_SUB_SCRIPTS:
		SetGadgetValue(objlist, GAD_ENV_SCRIPT_CONNECT_SUCCESS, env->e_script_connect_ok);
		SetGadgetValue(objlist, GAD_ENV_SCRIPT_CONNECT_FAIL, env->e_script_connect_fail);
		SetGadgetValue(objlist, GAD_ENV_SCRIPT_COPY_SUCCESS, env->e_script_copy_ok);
		SetGadgetValue(objlist, GAD_ENV_SCRIPT_COPY_FAIL, env->e_script_copy_fail);
		SetGadgetValue(objlist, GAD_ENV_SCRIPT_ERROR, env->e_script_error);
		SetGadgetValue(objlist, GAD_ENV_SCRIPT_CLOSE, env->e_script_close);
		SetGadgetValue(objlist, GAD_ENV_SCRIPT_TIME, (LONG)env->e_script_time);
		break;

	case ENV_SUB_INDEX:
		SetGadgetValue(objlist, GAD_ENV_INDEX_ENABLE, env->e_index_enable);
		SetGadgetValue(objlist, GAD_ENV_AUTO_DOWNLOAD, env->e_index_auto);
		SetGadgetValue(objlist, GAD_ENV_AUTO_DOWNLOAD_SIZE, env->e_indexsize);

		DisableObject(objlist, GAD_ENV_AUTO_DOWNLOAD_SIZE, !env->e_index_enable);
		DisableObject(objlist, GAD_ENV_AUTO_DOWNLOAD, !env->e_index_enable);

		break;

	case ENV_SUB_CONNECTION:

		SetGadgetValue(objlist, GAD_ENV_ENABLE_RETRY, env->e_retry);
		SetGadgetValue(objlist, GAD_ENV_RETRY_COUNT, env->e_retry_count);
		SetGadgetValue(objlist, GAD_ENV_RETRY_DELAY, env->e_retry_delay);
		SetGadgetValue(objlist, GAD_ENV_ENABLE_RETRY_LOST, env->e_retry_lost);
		SetGadgetValue(objlist, GAD_ENV_NOOPS, env->e_noops);
		SetGadgetValue(objlist,
					   GAD_ENV_TLS_MODE,
					   ftp_tls_mode_uses_control_tls(env->e_tls_mode) ? FTP_TLS_MODE_EXPLICIT : FTP_TLS_MODE_OFF);
		SetGadgetValue(objlist, GAD_ENV_TLS_VERIFY, env->e_tls_verify_peer);

		DisableObject(objlist, GAD_ENV_RETRY_COUNT, !env->e_retry);
		DisableObject(objlist, GAD_ENV_RETRY_DELAY, !env->e_retry);
		DisableObject(objlist, GAD_ENV_TLS_VERIFY, !ftp_tls_mode_uses_control_tls(env->e_tls_mode));

		break;

	default:
		break;
	}

	set_reset_button(dg, wp);
}

/***********************************
 *
 * Store Option/Environment gadget values
 *
 */

static void store_options_gadgets(struct window_params *wp)
{
	struct display_globals *dg;
	ObjectList *objlist;
	struct ftp_environment *env;
	struct ftp_config *tmp_oc;

	dg = wp->wp_dg;
	objlist = wp->wp_sub_objlist;

	if (wp->wp_type == WT_DEFOPT)
	{
		tmp_oc = &dg->dg_oc;
		env = &tmp_oc->oc_env;
	}
	else
	{
		if (wp && wp->wp_se_copy)
		{
			// set for copy in site data
			env = &wp->wp_se_copy->se_env_private;
		}
		else
			return;
	}

	switch (wp->wp_option)
	{
	case ENV_SUB_GLOBAL: {
		char oldlog[LOGNAMELEN + 1];
		char *anonpass;
		int logopen;
		tmp_oc = &dg->dg_oc;

		env = &tmp_oc->oc_env;

		strcpy(oldlog, tmp_oc->oc_logname);
		logopen = tmp_oc->oc_enable_log;

		anonpass = (char *)GetGadgetValue(objlist, GAD_ENV_ANON_PASSWORD);

		// is anon pass empty then clear old and reset to defaults

		if (!*anonpass)
		{
			*tmp_oc->oc_anonpass = 0;
			tmp_oc->oc_user_password = FALSE;
		}
		else
		{
			// otherwise is anon pass different to the one we fed user
			// then copy to global and mark as used

			if (stricmp(dg->dg_anonpass, anonpass))
			{
				strcpy(tmp_oc->oc_anonpass, anonpass);
				tmp_oc->oc_user_password = TRUE;
			}
		}

		strcpy(tmp_oc->oc_logname, (char *)GetGadgetValue(objlist, GAD_ENV_LOG_FILE));
		tmp_oc->oc_enable_log = GetGadgetValue(objlist, GAD_ENV_LOG_ENABLE);
		tmp_oc->oc_log_debug = GetGadgetValue(objlist, GAD_ENV_DEBUG);
		tmp_oc->oc_no_keep_alive = GetGadgetValue(objlist, GAD_ENV_NO_KEEP_ALIVE);

		tmp_oc->oc_addr_auto = GetGadgetValue(objlist, GAD_ENV_ADDR_AUTO);
		tmp_oc->oc_addr_dc = GetGadgetValue(objlist, GAD_ENV_ADDR_DC);

		// save what we want for log file
		dg->dg_log_open = tmp_oc->oc_enable_log;
	}

	break;

	case ENV_SUB_COPYFLAGS:

		env->e_copy_type = GetGadgetValue(objlist, GAD_ENV_COPY_TYPE);

		env->e_copy_set_archive = GetGadgetValue(objlist, GAD_ENV_COPY_SET_ARCHIVE);
		env->e_url_comment = GetGadgetValue(objlist, GAD_ENV_COPY_URL_COMMENT);

		env->e_rescan = GetGadgetValue(objlist, GAD_ENV_COPY_RESCAN);

		env->e_recursive_special = GetGadgetValue(objlist, GAD_ENV_COPY_RECURSIVE_SPECIAL);

		env->e_copy_replace = GetGadgetValue(objlist, GAD_ENV_REPLACE);

		break;

	case ENV_SUB_COPYATTR:

		env->e_copy_opus_default = GetGadgetValue(objlist, GAD_ENV_COPY_OPUS_DEFAULT);
		env->e_copy_datestamp = GetGadgetValue(objlist, GAD_ENV_COPY_DATESTAMP);
		env->e_copy_protection = GetGadgetValue(objlist, GAD_ENV_COPY_PROTECTION);
		env->e_copy_comment = GetGadgetValue(objlist, GAD_ENV_COPY_COMMENT);
		break;

	case ENV_SUB_LISTER:

		strcpy(env->e_toolbar, (char *)GetGadgetValue(objlist, GAD_ENV_TOOLBAR));
		// env->e_custom_format is set elsewhere

		break;

	case ENV_SUB_MISC:
		env->e_keep_last_dir = GetGadgetValue(objlist, GAD_ENV_KEEP_LAST_DIR);
		env->e_safe_links = GetGadgetValue(objlist, GAD_ENV_SAFE_LINKS);
		env->e_passive = GetGadgetValue(objlist, GAD_ENV_PASSIVE);
		env->e_special_dir = GetGadgetValue(objlist, GAD_ENV_SPECIAL_DIR);
		env->e_unk_links_file = GetGadgetValue(objlist, GAD_ENV_UNK_LINKS);
		env->e_timeout = GetGadgetValue(objlist, GAD_ENV_TIMEOUT);
		env->e_list_update = GetGadgetValue(objlist, GAD_ENV_LIST_UPDATE);
		break;

	case ENV_SUB_DISPLAY:
		env->e_show_startup = GetGadgetValue(objlist, GAD_ENV_SHOW_STARTUP_MSG);
		env->e_show_dir = GetGadgetValue(objlist, GAD_ENV_SHOW_DIR_MSG);
		env->e_progress_window = GetGadgetValue(objlist, GAD_ENV_PROGRESS_WINDOW);
		env->e_transfer_details = !GetGadgetValue(objlist, GAD_ENV_TRANSFER_DETAILS);
		break;

	case ENV_SUB_SCRIPTS:
		env->e_script_connect_ok = GetGadgetValue(objlist, GAD_ENV_SCRIPT_CONNECT_SUCCESS);
		env->e_script_connect_fail = GetGadgetValue(objlist, GAD_ENV_SCRIPT_CONNECT_FAIL);
		env->e_script_copy_ok = GetGadgetValue(objlist, GAD_ENV_SCRIPT_COPY_SUCCESS);
		env->e_script_copy_fail = GetGadgetValue(objlist, GAD_ENV_SCRIPT_COPY_FAIL);
		env->e_script_error = GetGadgetValue(objlist, GAD_ENV_SCRIPT_ERROR);
		env->e_script_close = GetGadgetValue(objlist, GAD_ENV_SCRIPT_CLOSE);
		env->e_script_time = GetGadgetValue(objlist, GAD_ENV_SCRIPT_TIME);
		break;

	case ENV_SUB_INDEX:
		env->e_index_enable = GetGadgetValue(objlist, GAD_ENV_INDEX_ENABLE);
		env->e_index_auto = GetGadgetValue(objlist, GAD_ENV_AUTO_DOWNLOAD);
		env->e_indexsize = GetGadgetValue(objlist, GAD_ENV_AUTO_DOWNLOAD_SIZE);
		break;

	case ENV_SUB_CONNECTION:
		env->e_retry_count = GetGadgetValue(objlist, GAD_ENV_RETRY_COUNT);
		env->e_retry = GetGadgetValue(objlist, GAD_ENV_ENABLE_RETRY);
		env->e_retry_delay = GetGadgetValue(objlist, GAD_ENV_RETRY_DELAY);
		env->e_retry_lost = GetGadgetValue(objlist, GAD_ENV_ENABLE_RETRY_LOST);
		env->e_noops = GetGadgetValue(objlist, GAD_ENV_NOOPS);
		env->e_tls_mode = GetGadgetValue(objlist, GAD_ENV_TLS_MODE);
		env->e_tls_verify_peer = GetGadgetValue(objlist, GAD_ENV_TLS_VERIFY);
		break;

	default:
		break;
	}
}

static void change_options_texts(int type)
{
	ObjectDef *x, *y;

	if (!(x = find_ObjectDef(ftp_options_objects, GAD_ENV_SAVE)))
		return;

	if (!(y = find_ObjectDef(ftp_options_objects, GAD_ENV_RESET_DEFAULT)))
		return;

	if (type == WT_DEFOPT)
	{
		x->gadget_text = MSG_FTP_SAVE;
		y->gadget_text = MSG_RESET_TO_SYSTEM;

		change_menus(options_menus, MENU_ENV_RESET_DEFAULTS, MSG_RESET_TO_SYSTEM);
	}
	else
	{
		x->gadget_text = MSG_FTP_USE;
		y->gadget_text = MSG_RESET_TO_DEFAULT;

		change_menus(options_menus, MENU_ENV_RESET_DEFAULTS, MSG_RESET_TO_DEFAULT);
	}
}

/*****************************************************************
 *
 *	Edit options from addressbook only.
 *	Alloc a new site entry and use env componet for editing
 *	then copy this back to original if ok selected
 *
 */

static void edit_options(struct display_globals *dg, struct Window *win, WORD type)
{
	Att_List *opts;
	SubOptionHandle *subopts;

	if (type == WT_DEFOPT)
	{
		if (dg->dg_defoptions)
		{
			myWindowToFront(dg->dg_defoptions, TRUE);
			return;
		}

		subopts = ftp_default_suboptions;

		// make tmp copy of current config
		*(&dg->dg_oc) = *(&dg->dg_og->og_oc);

		// get current start of log file
		dg->dg_log_open = dg->dg_og->og_log_open;
	}
	else
	{
		subopts = ftp_custom_suboptions;
	}

	// set specific gadget and menu texts for different modes
	change_options_texts(type);

	if ((opts = build_sub_options(subopts)))
	{
		// alloc a site entry and use env for editing
		struct site_entry *e;

		if ((e = AllocVec(sizeof(struct site_entry), MEMF_CLEAR)))
		{
			struct window_params *wp, *parentwp;
			parentwp = FINDWP(win);

			if ((wp = open_childwin(dg, &ftp_environment_window, ftp_options_objects, parentwp, type, 0)))
			{
				Att_Node *option_node;

				// save ptr to allocated site entry
				wp->wp_se_copy = e;

				wp->wp_options_list = opts;
				wp->wp_option = 0;
				wp->wp_SubOptionsHandle = subopts;

				// Add menus **NOTE these are freed automatically with closeconfigwindow
				AddWindowMenus(wp->wp_win, options_menus);

				// Set sub-option list
				SetGadgetChoices(wp->wp_objlist, GAD_ENV_ITEM_LISTER, wp->wp_options_list);

				SetGadgetValue(wp->wp_objlist, GAD_ENV_ITEM_LISTER, wp->wp_option);

				option_node = Att_FindNode(wp->wp_options_list, wp->wp_option);

				wp->wp_sub_objlist = AddObjectList(wp->wp_win, wp->wp_SubOptionsHandle[option_node->data].objects);

				if (type == WT_DEFOPT)
				{
					// this uses default config and copy in dg so no need to alloc site entry
					dg->dg_defoptions = wp;
				}
				else
				{
					// get environment/options settings from calling window
					// make a copy for this wp node.

					copy_site_entry(dg->dg_og, e, parentwp->wp_se_copy);

					// update private env copy with global so we can edit it
					if (!e->se_has_custom_env)
						create_dummy_env(dg, e);

					// set real to point to original site_entry
					wp->wp_se_real = parentwp->wp_se_real;
				}

				display_options_gadgets(wp);
				return;
			}
			FreeVec(e);
		}

		// failed
		Att_RemList(opts, 0);
	}
}

/******************************************************************
 *
 *	Called from main ftp process to display a stand alone options req
 *	If cm->cm_site == NULL then doing default config
 *	otherwise is a custom set attached to a lister
 */

static struct window_params *show_ftp_options(struct display_globals *dg, struct subproc_data *data, IPCMessage *imsg)
{
	NewConfigWindow newwin;
	struct connect_msg *cm;
	Att_List *opts;
	SubOptionHandle *subopts;
	WORD type;

	cm = (struct connect_msg *)imsg->data;

	// Is this a valid message ?
	if (!cm)
		return (NULL);

	// Are custom or default settings wanted
	// imsg->flags==1 for custom options

	if (imsg->flags == 0)  // 0 means default options
	{
		type = WT_DEFOPT;
		subopts = ftp_default_suboptions;

		// make tmp copy of current config
		*(&dg->dg_oc) = *(&dg->dg_og->og_oc);

		// get current start of log file
		dg->dg_log_open = dg->dg_og->og_log_open;
	}
	else
	{
		type = WT_OPT;
		subopts = ftp_custom_suboptions;
	}

	// set specific gadget and menu texts for different modes
	change_options_texts(type);

	if ((opts = build_sub_options(subopts)))
	{
		struct site_entry *e;

		if ((e = AllocVec(sizeof(struct site_entry), MEMF_CLEAR)))
		{
			struct window_params *wp;

			// Fill out new window structure
			newwin.parent = dg->dg_og->og_screen;
			newwin.dims = &ftp_environment_window;
			newwin.locale = locale;
			newwin.port = dg->idcmp_port;
			newwin.flags = WINDOW_SCREEN_PARENT | WINDOW_AUTO_KEYS | WINDOW_REQ_FILL | WINDOW_VISITOR;
			newwin.font = 0;

			// Open window, add objects
			if ((wp = open_configwin(dg, &newwin, ftp_options_objects, type)))
			{
				Att_Node *option_node;

				// store the ptr to copy entry. Only used for site/lister options
				wp->wp_se_copy = e;

				wp->wp_options_list = opts;
				wp->wp_option = 0;
				wp->wp_SubOptionsHandle = subopts;

				if (type == WT_DEFOPT)
					dg->dg_defoptions = wp;
				else
				{
					// get environment/options settings from calling connect msg
					// make a copy of the old entry

					copy_site_entry(dg->dg_og, e, &cm->cm_site);

					// if no custom options then create a dummy copy of the default options
					if (!e->se_has_custom_env)
						create_dummy_env(dg, e);

					// set real to point to original site_entry on connect message
					wp->wp_se_real = &cm->cm_site;
				}

				// Add menus **NOTE these are freed automatically with closeconfigwindow
				AddWindowMenus(wp->wp_win, options_menus);

				// Set sub-option list
				SetGadgetChoices(wp->wp_objlist, GAD_ENV_ITEM_LISTER, wp->wp_options_list);

				SetGadgetValue(wp->wp_objlist, GAD_ENV_ITEM_LISTER, wp->wp_option);

				option_node = Att_FindNode(wp->wp_options_list, wp->wp_option);

				wp->wp_sub_objlist = AddObjectList(wp->wp_win, wp->wp_SubOptionsHandle[option_node->data].objects);

				display_options_gadgets(wp);

				return (wp);
			}

			FreeVec(e);
		}
		// failed
		Att_RemList(opts, 0);
	}

	return (NULL);
}

static struct window_params *show_connect(struct display_globals *dg, struct subproc_data *data, IPCMessage *imsg)
{
	NewConfigWindow newwin;
	struct connect_msg *cm;
	cm = (struct connect_msg *)imsg->data;

	if (cm && cm->cm_opus)	// Is this a valid message ?
	{
		struct site_entry *e;
		struct window_params *wp;

		if ((e = AllocVec(sizeof(struct site_entry), MEMF_CLEAR)))
		{
			// Fill out new window structure
			newwin.parent = dg->dg_og->og_screen;
			newwin.dims = &ftp_connect_window;
			newwin.locale = locale;
			newwin.port = dg->idcmp_port;
			newwin.flags = WINDOW_SCREEN_PARENT | WINDOW_AUTO_KEYS | WINDOW_REQ_FILL | WINDOW_VISITOR;
			newwin.font = 0;

			// Open window, add objects
			if ((wp = open_configwin(dg, &newwin, ftp_connect_objects, WT_CON)))
			{
				// IMPORTANT - Set this to null to indicate we are not editing entry
				wp->wp_se_real = NULL;

				// set ptr to allocaged copy of site
				wp->wp_se_copy = e;

				// get site_entry from connect message
				copy_site_entry(dg->dg_og, e, &cm->cm_site);

				if (site_protocol(e) == FTP_PROTOCOL_SFTP)
					e->se_anon = FALSE;
				else if (!*e->se_user)
					e->se_anon = TRUE;

				if (e->se_port <= 0)
					e->se_port = ftp_protocol_default_port(site_protocol(e));

				display_connect_gadgets(dg, wp);

				if (e->se_active_gadget)
				{
					// safety incase bad id and gadget not found
					GL_Object *obj;

					obj = GetObject(wp->wp_objlist, e->se_active_gadget);

					// now now just activate the first gadget
					if (obj)
						ActivateStrGad(GADGET(obj), wp->wp_win);
				}

				return (wp);
			}
			FreeVec(e);
		}
	}

	return (NULL);
}

/********************************************************
 *
 *	Add an entry sent to us from main lister process
 *	We are sent a connect message which contains a copy of the site
 *	so we must allocate a new one, copy the site entry and use this
 *	before returning.
 *	The Caller will free the connect_msg
 */

static struct window_params *add_entry(struct display_globals *dg, struct subproc_data *data, IPCMessage *imsg)
{
	NewConfigWindow newwin;
	struct connect_msg *cm;

	cm = (struct connect_msg *)imsg->data;

	if (cm && cm->cm_opus)	// Is this a valid message ?
	{
		struct site_entry *e;

		if ((e = AllocVec(sizeof(struct site_entry), MEMF_CLEAR)))
		{
			struct window_params *wp;

			newwin.parent = dg->dg_og->og_screen;
			newwin.dims = &ftp_edit_window;
			newwin.locale = locale;
			newwin.port = dg->idcmp_port;
			newwin.flags = WINDOW_SCREEN_PARENT | WINDOW_AUTO_KEYS | WINDOW_REQ_FILL | WINDOW_VISITOR;
			newwin.font = 0;

			// Open window, add objects
			if ((wp = open_configwin(dg, &newwin, ftp_edit_objects, WT_ADD)))
			{
				// IMPORTANT - Set this to null to indicate we are NOT editing entry
				wp->wp_se_real = NULL;

				// set ptr to allocaged copy of site
				wp->wp_se_copy = e;

				// copy site_entry from connect message
				copy_site_entry(dg->dg_og, e, &cm->cm_site);

				if (site_protocol(e) == FTP_PROTOCOL_SFTP)
					e->se_anon = FALSE;
				else if (!*e->se_user)
					e->se_anon = TRUE;

				// if anon the blank user and password
				if (site_protocol(e) != FTP_PROTOCOL_SFTP && e->se_anon)
				{
					*e->se_user = 0;
					*e->se_pass = 0;
				}

				if (e->se_port <= 0)
					e->se_port = ftp_protocol_default_port(site_protocol(e));

				display_edit_gadgets(dg, wp);

				if (++dg->dg_edit_count == 1)
					set_ftp_menu(dg->dg_addrwp, FALSE);

				return (wp);
			}
			FreeVec(e);
		}
	}

	return (NULL);
}

/***************************************
 *
 *	create a blank entry for new entry and display
 *	NOTE: wp->wp_se_real is used as marker for New or Old entry
 *
 */

static void new_entry(struct display_globals *dg, struct Window *win)
{
	struct window_params *wp;
	struct window_params *parentwp;

	parentwp = FINDWP(win);

	if ((wp = open_childwin(dg, &ftp_edit_window, ftp_edit_objects, parentwp, WT_NEW, 0)))
	{
		struct site_entry *e;

		if ((e = AllocVec(sizeof(struct site_entry), MEMF_CLEAR)))
		{
			e->se_anon = TRUE;
			e->se_protocol = FTP_PROTOCOL_FTP;
			e->se_port = ftp_protocol_default_port(e->se_protocol);
			e->se_env = &dg->dg_og->og_oc.oc_env;

			// IMPORTANT - Set this to null to indicate we are adding entry
			wp->wp_se_real = NULL;

			// set ptr to allocaged copy of site
			wp->wp_se_copy = e;

			display_edit_gadgets(dg, wp);

			if (++dg->dg_edit_count == 1)
				set_ftp_menu(dg->dg_addrwp, FALSE);
		}
		else  // failed alloc
			close_child(wp);
	}
}

/***************************************
 *
 * 	create a copy entry for edit and display
 *	make a copy of the real entry and compare when end edit
 *	If different then copy and trigger save option, othewise
 *	do not bother to save if entries are the same.
 *	FreeVec(copy of entry ) at end edit
 *	NOTE:wp->wp_se_real is used as marker for New or Old entry
 *
 *
 *
 */

static void edit_entry(struct display_globals *dg, struct Window *win)
{
	Att_Node *node;

	if ((node = Att_FindNode(dg->dg_og->og_SiteList, dg->dg_selected)))
	{
		struct window_params *wp, *parentwp, *oldwp;

		parentwp = FINDWP(win);

		if ((oldwp = entry_in_use(dg, (struct site_entry *)node->data)))
		{
			myWindowToFront(oldwp, TRUE);
			return;
		}

		if ((wp = open_childwin(dg, &ftp_edit_window, ftp_edit_objects, parentwp, WT_EDIT, 0)))
		{
			struct site_entry *new;

			wp->wp_se_real = (struct site_entry *)node->data;

			wp->wp_sitenode = node;

			wp->wp_itemnumber = dg->dg_selected;

			if ((new = AllocVec(sizeof(struct site_entry), MEMF_CLEAR)))
			{
				// set ptr to allocaged copy of site
				wp->wp_se_copy = new;

				// make a copy of the old entry
				copy_site_entry(dg->dg_og, new, wp->wp_se_real);
				if (site_protocol(new) == FTP_PROTOCOL_SFTP)
					new->se_anon = FALSE;

				display_edit_gadgets(dg, wp);

				if (++dg->dg_edit_count == 1)
					set_ftp_menu(dg->dg_addrwp, FALSE);
			}

			else  // failed alloc
				close_child(wp);
		}
	}
}

/*******************************************************************
 *
 *	Display site list in small listview
 *
 *	Note NO ALLOCATED SITE LIST uses one from Parent window
 *
 */

static void small_site_list(struct display_globals *dg, struct window_params *parentwp)
{
	struct window_params *wp;
	Att_List *list;
	char *name;

	if (!make_sitelist_available(dg))
		return;

	// get the current name/host from the connect gadgets

	// check that we have not got a ? for name so we don't overwrite the real name

	name = (char *)GetGadgetValue(parentwp->wp_objlist, GAD_CONNECT_NAME);
	if (*name != '?')
		strcpy(parentwp->wp_se_copy->se_name, name);

	// get host
	strcpy(parentwp->wp_se_copy->se_host, (char *)GetGadgetValue(parentwp->wp_objlist, GAD_CONNECT_HOST));

	if ((wp = open_childwin(
			 dg, &ftp_list_window, ftp_list_objects, parentwp, WT_ADRLIST, WINDOW_SIZE_BOTTOM | IDCMP_IDCMPUPDATE)))
	{
		struct site_entry *e;
		int pos;

		// get calling details
		e = parentwp->wp_se_copy;

		// find in site list if there
		// and adjust to ordinal value from get_site

		if ((pos = get_site_entry(dg->dg_og, e, dg->dg_ipc)))
			--pos;

		list = dg->dg_og->og_SiteList;

		if (list && !IsListEmpty((struct List *)list))
		{
			// Add list
			SetGadgetChoices(wp->wp_objlist, GAD_LIST_SITES, (APTR)list);

			// highlight name
			SetGadgetValue(wp->wp_objlist, GAD_LIST_SITES, pos);
		}
		else
			// or just disable it
			DisableObject(wp->wp_objlist, GAD_LIST_SITES, TRUE);
	}
}

/***********************************************************
 *
 *	Update the main site list with new or changed entry
 *
 *	found==NULL means that this is a NEW entry
 *	Otherwise update old entry
 */

static VOID change_site_list_entry(struct display_globals *dg, struct site_entry *e, Att_Node *found)
{
	Att_Node *node;
	Att_Node *nextnode = NULL;

	// update env ptrs
	if (e->se_has_custom_env)
		e->se_env = &e->se_env_private;
	else
		e->se_env = &dg->dg_og->og_oc.oc_env;

	// if addressbook i visible then detach list
	if (dg->dg_addrwp)
		SetGadgetChoices(dg->dg_addrwp->wp_objlist, GAD_FTP_SITES, (APTR)~0);

	// found previous entry then remember where to replace node
	if (found)
	{
		nextnode = (Att_Node *)found->node.ln_Succ;
		Att_RemNode(found);
	}

	// add the new entry
	if ((node = Att_NewNode(dg->dg_og->og_SiteList, e->se_name, (IPTR)e, ADDNODE_SORT)))
	{
		// Specific place to put this node?
		if (nextnode)
			Att_PosNode(dg->dg_og->og_SiteList, node, nextnode);

		dg->dg_selected = Att_FindNodeNumber(dg->dg_og->og_SiteList, node);

		// mark as changed site details
		SetFlag(dg->dg_config_changed, CHANGED_ADR);
	}

	// reattach the list if visible and show it
	if (dg->dg_addrwp)
	{
		SetGadgetChoices(dg->dg_addrwp->wp_objlist, GAD_FTP_SITES, dg->dg_og->og_SiteList);
		SetGadgetValue(dg->dg_addrwp->wp_objlist, GAD_FTP_SITES, dg->dg_selected);
		display_main_gadgets(dg, dg->dg_selected);
	}
}

/***********************************************************
 *
 * UPDATE an existing entry in the site list
 *
 *	Check if the entry exists.
 *	if exists then check if name and host match
 *	if so then replace old one
 *	for  ADD_ENTRY replace
 *       for SAVE_DIR just update current and save
 *
 *
 *	If ADD and site name and site host exist then replace old entry
 *
 *	Supplied site_entry is only be partial for SAVE_DIR
 *
 * Maybe change this to just update disk entry not re write whole file?
 *
 *
 */

enum {
	ADD_ENTRY,
	SAVE_DIR,
};

static VOID update_sitelist(struct display_globals *dg, struct site_entry *copy, struct Window *win, int type)
{
	Att_Node *found;

	// check site list is in memory!
	if (!make_sitelist_available(dg))
	{
		FreeVec(copy);
		return;
	}

	if ((found = check_duplicate_entry(dg, copy)))	// Exists then update it
	{
		struct window_params *oldwp;
		struct site_entry *olde;

		// get entry from site list or..
		olde = (struct site_entry *)found->data;

		// if is currently being edited or displayed get
		// site_entry being edited from its wp

		if ((oldwp = entry_in_use(dg, olde)))
			olde = oldwp->wp_se_copy;

		if (type == ADD_ENTRY)	// overwrite old site
		{
			// if same as before then skip
			if (memcmp(olde, copy, sizeof(struct site_entry)) == 0)
			{
				FreeVec(copy);
				return;
			}

			// if the window is displayed and has children then
			// we cannot change underlying structures

			if (oldwp && oldwp->wp_children)
			{
				struct List *list;
				list = &oldwp->wp_children->list;
				if (list && !IsListEmpty(list))
				{
					myWindowToFront(oldwp, FALSE);
					DisplayBeep(dg->dg_og->og_screen);

					FreeVec(copy);
					return;
				}
			}

			// otherwise we can update the old entry and free copy

			copy_site_entry(dg->dg_og, olde, copy);

			FreeVec(copy);
		}
		else if (type == SAVE_DIR)	// overwrite old site directory
		{
			/// if save dir and no change then don't bother
			if (stricmp(olde->se_path, copy->se_path) == 0)
				return;

			stccpy(olde->se_path, copy->se_path, PATHLEN);
		}

		// is it currently being displayed?
		if (oldwp)
		{
			WindowToFront(oldwp->wp_win);
			display_edit_gadgets(dg, oldwp);
			DisplayBeep(dg->dg_og->og_screen);
		}
		else
		{
			// update the old entry
			change_site_list_entry(dg, olde, found);
		}
	}
	else
	{
		// if site not found then if ADD, add this site_entry to the list
		//	do not free allocated data since is added to list
		if (type == ADD_ENTRY)
			change_site_list_entry(dg, copy, NULL);
	}

	// if not visible and changed then save address book if changed
	if (!dg->dg_addrwp && FlagIsSet(dg->dg_config_changed, CHANGED_ADR))
	{
		save_sites(dg, win, GAD_FTP_SAVE);
		ClearFlag(dg->dg_config_changed, CHANGED_ADR);
	}
}

/***********************************************************
 *
 * end small listview with site dispplay and copy selected if there is one
 *
 */

static void end_small_site_list(struct window_params *wp, BOOL flag)
{
	struct display_globals *dg;
	struct site_entry *e = NULL;
	struct window_params *parentwp;

	dg = wp->wp_dg;

	parentwp = wp->wp_parentwp;

	if (flag)
	{
		Att_Node *node;
		int hit;

		hit = GetGadgetValue(wp->wp_objlist, GAD_LIST_SITES);

		if ((node = Att_FindNode(dg->dg_og->og_SiteList, hit)))
			e = (struct site_entry *)node->data;
	}

	close_child(wp);

	if (e && parentwp)
	{
		// update caller site entry and refresh
		copy_site_entry(dg->dg_og, parentwp->wp_se_copy, e);
		display_connect_gadgets(dg, parentwp);
	}
}

/***********************************************************
 *
 *	Done connect requester
 *	return to calling message with TRUE/FALSE and
 *	details in connect message site_entry data*
 *
 */

static void end_connect(struct window_params *wp, BOOL flag)
{
	struct display_globals *dg;
	struct site_entry *copy;
	IPCMessage *imsg;

	dg = wp->wp_dg;

	copy = wp->wp_se_copy;

	// save ptr for we free wp before using this ptr
	imsg = wp->wp_imsg;

	if (flag)
	{
		ObjectList *objlist = wp->wp_objlist;
		int connection;
		LONG port;

		// used selected OK
		// get gadget values
		// add allocated entry to the list

		strcpy(copy->se_name, (char *)GetGadgetValue(objlist, GAD_CONNECT_NAME));
		connection = connection_from_cycle_value(GetGadgetValue(objlist, GAD_CONNECT_PROTOCOL));
		port = GetGadgetValue(objlist, GAD_CONNECT_PORT);
		copy->se_port = port > 65535 ? 0 : port;
		set_site_connection(dg, copy, connection, copy->se_port);
		set_site_host_from_text(dg, copy, (char *)GetGadgetValue(objlist, GAD_CONNECT_HOST), copy->se_port);

		// if no host or defaulted then erro msg

		if (!*copy->se_host || !strcmp(copy->se_host, "Shrubbery"))
		{
			display_msg(dg->dg_og, dg->dg_ipc, wp->wp_win, 0, GetString(locale, MSG_BADSITE));
			SetGadgetValue(objlist, GAD_CONNECT_HOST, (IPTR) "Shrubbery");
			return;
		}

		// if not name then make name == host
		if (!*copy->se_name)
			// strcpy(copy->se_name,copy->se_host);
			sitename_from_host(copy->se_name, copy->se_host);

		if (copy->se_port <= 0)
			copy->se_port = ftp_protocol_default_port(site_protocol(copy));

		strcpy(copy->se_user, (char *)GetGadgetValue(objlist, GAD_CONNECT_USER));
		strcpy(copy->se_pass, (char *)GetGadgetValue(objlist, GAD_CONNECT_PASSWORD));
		strcpy(copy->se_path, (char *)GetGadgetValue(objlist, GAD_CONNECT_DIR));

		copy->se_anon = site_protocol(copy) == FTP_PROTOCOL_SFTP ? FALSE : GetGadgetValue(objlist, GAD_CONNECT_ANON);

		// if no user name then make anon entry

		if (!*copy->se_user && site_protocol(copy) != FTP_PROTOCOL_SFTP)
			copy->se_anon = TRUE;

		if (imsg)
		{
			struct connect_msg *cm;

			cm = (struct connect_msg *)imsg->data;

			// copy site back to connect message
			copy_site_entry(dg->dg_og, &cm->cm_site, copy);
			cm->cm_protocol = site_protocol(copy);
			cm->cm_site.se_protocol = cm->cm_protocol;
		}
	}

	// free allocated site entry

	FreeVec(copy);

	if (wp->wp_parentwp)
		close_child(wp);
	else
		close_configwin(wp);

	// reply to intial calling message

	if (imsg)
	{
		// Set reply to true/false according to flag
		// set reply msg for ok

		imsg->command = flag;
		IPC_Reply(imsg);
	}
}

/**************************************************
 *
 *	End an NEW, EDIT or ADD entry sequence
 *
 *	Get OK/Cancel from editing an entry
 *	update the main list and refresh gadgets
 *
 */

static BOOL end_edit(struct window_params *wp, BOOL flag)
{
	struct display_globals *dg;
	struct site_entry *copy;
	dg = wp->wp_dg;

	copy = wp->wp_se_copy;

	// if TRUE then user selected OK. Get data from gadgets

	if (flag)
	{
		ObjectList *objlist = wp->wp_objlist;
		int connection;
		int custom_options;
		LONG port;

		// used selected OK
		// get gadget values
		// add allocated entry to the list

		strcpy(copy->se_name, (char *)GetGadgetValue(objlist, GAD_EDIT_NAME));
		connection = connection_from_cycle_value(GetGadgetValue(objlist, GAD_EDIT_PROTOCOL));
		custom_options = GetGadgetValue(objlist, GAD_EDIT_CUSTOM_OPTIONS);
		if (custom_options)
			site_make_custom_env(dg, copy);
		else
		{
			copy->se_has_custom_env = FALSE;
			copy->se_env = &dg->dg_og->og_oc.oc_env;
		}
		port = GetGadgetValue(objlist, GAD_EDIT_PORT);
		copy->se_port = port > 65535 ? 0 : port;
		set_site_connection(dg, copy, connection, copy->se_port);
		set_site_host_from_text(dg, copy, (char *)GetGadgetValue(objlist, GAD_EDIT_HOST), copy->se_port);

		// if no host or defaulted then error msg

		if (!*copy->se_host || !strcmp(copy->se_host, "Shrubbery"))
		{
			display_msg(dg->dg_og, dg->dg_ipc, wp->wp_win, 0, GetString(locale, MSG_BADSITE));
			SetGadgetValue(objlist, GAD_EDIT_HOST, (IPTR) "Shrubbery");
			return (FALSE);
		}

		// if not name then make name == host
		if (!*copy->se_name)
			sitename_from_host(copy->se_name, copy->se_host);

		if (copy->se_port <= 0)
			copy->se_port = ftp_protocol_default_port(site_protocol(copy));

		strcpy(copy->se_user, (char *)GetGadgetValue(objlist, GAD_EDIT_USER));
		strcpy(copy->se_pass, (char *)GetGadgetValue(objlist, GAD_EDIT_PASSWORD));
		strcpy(copy->se_path, (char *)GetGadgetValue(objlist, GAD_EDIT_DIR));

		copy->se_anon = site_protocol(copy) == FTP_PROTOCOL_SFTP ? FALSE : GetGadgetValue(objlist, GAD_EDIT_ANON);

		// if no user name then make anon entry

		if (!*copy->se_user && site_protocol(copy) != FTP_PROTOCOL_SFTP)
			copy->se_anon = TRUE;

		// check if we actually changed the entry
		// if edit entry and unchanged then skip and just discard copy
		// otherwise detach and replace.
		// if new then add it.

		if (wp->wp_type == WT_NEW)
		{
			// Allocated copy used for node->data in list so do not free
			change_site_list_entry(dg, copy, NULL);
			copy = NULL;
		}
		else if (wp->wp_type == WT_EDIT)
		{
			// if changed
			if (memcmp(copy, wp->wp_se_real, sizeof(struct site_entry)))
			{
				// copy back over old
				copy_site_entry(dg->dg_og, wp->wp_se_real, copy);

				// update old new entry
				change_site_list_entry(dg, wp->wp_se_real, wp->wp_sitenode);
			}
		}
		else if (wp->wp_type == WT_ADD)
		{
			// copy is either copied over old entry or used for list data
			// do not free it
			update_sitelist(dg, copy, wp->wp_win, ADD_ENTRY);
			copy = NULL;
		}
	}

	// if not saved or user selected CANCEL just discard the allocated site_entry copy

	if (copy)
		FreeVec(copy);

	if (wp->wp_type == WT_ADD && wp->wp_imsg)
	{
		// set reply true/false according to flag
		wp->wp_imsg->command = flag;
		IPC_Reply(wp->wp_imsg);
	}

	if (wp->wp_parentwp)
		close_child(wp);
	else
		close_configwin(wp);

	if (--dg->dg_edit_count == 0)
		set_ftp_menu(dg->dg_addrwp, TRUE);

	return (TRUE);
}

/*****************************
 *
 *	End edit of options
 *	compare copy and real config
 *	If different then copy and trigger save option
 *
 *	same routine handles both default and custom options
 */

static void end_options(struct window_params *wp, BOOL flag)
{
	BOOL default_opt = FALSE;
	struct display_globals *dg;
	struct opusftp_globals *ogp;

	dg = wp->wp_dg;
	ogp = dg->dg_og;

	// check if we are using the default options

	if (wp->wp_type == WT_DEFOPT)
		default_opt = TRUE;

	// get any which have been set in last display

	if (flag)  // user selected OK
	{
		store_options_gadgets(wp);

		if (default_opt)
		{
			//	compare REAL config with our copy to see if user changed anything
			if (config_different(&dg->dg_oc, &ogp->og_oc))
			{
				// update old config with new one
				set_new_config(dg, wp);

				// save new ones
				save_options(wp, wp->wp_type, 0);
			}
		}
		else
		{
			// compare copy env with default
			if (env_not_default(dg, &wp->wp_se_copy->se_env_private))
			{
				// if different then update flag and ptr
				wp->wp_se_copy->se_has_custom_env = TRUE;
				wp->wp_se_copy->se_env = &wp->wp_se_copy->se_env_private;
			}
			else
			{
				// if the same then reset ptr to point to global values
				wp->wp_se_copy->se_has_custom_env = FALSE;
				wp->wp_se_copy->se_env = &ogp->og_oc.oc_env;
			}

			// if options window from adressbook site entry then
			// update original site entry with new version

			if (wp->wp_parentwp)
			{
				// if changed options then copy back just env
				*(&wp->wp_parentwp->wp_se_copy->se_env_private) = *(&wp->wp_se_copy->se_env_private);
			}
		}
	}

	if (default_opt && wp == dg->dg_defoptions)
		dg->dg_defoptions = NULL;

	// is a child window from main addressbook
	if (wp->wp_parentwp)
	{
		if (!default_opt)
		{
			SetGadgetValue(wp->wp_parentwp->wp_objlist, GAD_EDIT_CUSTOM_OPTIONS, wp->wp_se_copy->se_has_custom_env);
			DisableObject(wp->wp_parentwp->wp_objlist, GAD_EDIT_SET_CUSTOM_OPTIONS, !wp->wp_se_copy->se_has_custom_env);
		}

		FreeVec(wp->wp_se_copy);
		close_child(wp);
	}
	else
	{
		// otherwise has been called by main process as independant window
		// to display

		if (wp->wp_imsg)
		{
			if (flag)
			{
				// if custom options for a lister then update it.
				// and do send msg back.

				if (wp->wp_type == WT_OPT)
				{
					struct connect_msg *cm;

					cm = (struct connect_msg *)wp->wp_imsg->data;

					copy_site_entry(dg->dg_og, &cm->cm_site, wp->wp_se_copy);
					cm->cm_protocol = site_protocol(wp->wp_se_copy);
					cm->cm_site.se_protocol = cm->cm_protocol;
				}
			}

			wp->wp_imsg->command = flag;
			IPC_Reply(wp->wp_imsg);
		}

		FreeVec(wp->wp_se_copy);
		close_configwin(wp);
	}
}

static void delete_entry(struct display_globals *dg)
{
	Att_Node *node;
	int count;

	if ((node = Att_FindNode(dg->dg_og->og_SiteList, dg->dg_selected)))
	{
		struct window_params *oldwp;

		if ((oldwp = entry_in_use(dg, (struct site_entry *)node->data)))
		{
			myWindowToFront(oldwp, TRUE);
			return;
		}

		// detach list
		SetGadgetChoices(dg->dg_addrwp->wp_objlist, GAD_FTP_SITES, (APTR)~0);

		// free the node data then remove the node
		FreeVec((APTR)node->data);
		Att_RemNode(node);

		// add list
		SetGadgetChoices(dg->dg_addrwp->wp_objlist, GAD_FTP_SITES, dg->dg_og->og_SiteList);

		// check if we have run off the end of the list ?
		if (dg->dg_selected >= (count = Att_NodeCount(dg->dg_og->og_SiteList)))
			dg->dg_selected = count - 1;

		SetGadgetValue(dg->dg_addrwp->wp_objlist, GAD_FTP_SITES, dg->dg_selected);

		// mark as changed site details
		SetFlag(dg->dg_config_changed, CHANGED_ADR);

		// redisplay the gadgets
		display_main_gadgets(dg, dg->dg_selected);
	}
}

/***************************************************
 *
 *	Close main addressbook window
 *	and reply to originating message
 *	Other stuff such as saving config etc done elsewhere
 */

static void close_addrbook(struct window_params *wp, BOOL flag)
{
	Att_Node *node;
	IPCMessage *imsg;

	if (wp)
	{
		struct display_globals *dg;
		dg = wp->wp_dg;

		// save ptr for we free wp before using this ptr
		imsg = wp->wp_imsg;

		if (wp->wp_children)
		{
			struct List *list;
			// has children ?
			list = &wp->wp_children->list;
			while (list->lh_TailPred != (struct Node *)list)  // IsListEmpty ?
			{
				// nodes are progressively removed from list by sub funcs
				node = (Att_Node *)list->lh_TailPred;
				close_conwin_by_type((struct window_params *)node->data, FALSE);
			}
		}

		// Store window position
		store_window_pos(wp->wp_win);

		close_configwin(wp);

		ReleaseSemaphore(&dg->dg_og->og_SiteList_semaphore);
		dg->dg_og->og_addrbook_open = FALSE;

		// reply to intial calling message

		if (imsg)
		{
			imsg->command = flag;
			IPC_Reply(imsg);
		}

		dg->dg_addrwp = NULL;
	}
}

/************************************************
 *
 *	Display main ftp addressbook
 *
 */

static struct window_params *show_addrbook(struct display_globals *dg, struct subproc_data *data, IPCMessage *imsg)
{
	NewConfigWindow newwin;
	ConfigWindow ftp_win;

	struct addrbook_msg *am;

	am = (struct addrbook_msg *)imsg->data;

	if (am && am->am_opus)
	{
		struct IBox pos;
		short old_font_size = 0;

		// Fill out opus port - needed for sending connect messages
		// This implementation will break if running multiple Opuses
		stccpy(dg->dg_opusport, am->am_opus, PORTNAMELEN);

		// make tmp copy of current config
		*(&dg->dg_oc) = *(&dg->dg_og->og_oc);

		// get current start of log file
		dg->dg_log_open = dg->dg_og->og_log_open;

		// copy configwindow structure to preserve real for next call

		*(&ftp_win) = *(&ftp_main_window);

		// Fill out new window structure
		newwin.parent = dg->dg_og->og_screen;
		newwin.dims = &ftp_win;
		newwin.locale = locale;
		newwin.port = dg->idcmp_port;
		newwin.flags = WINDOW_SCREEN_PARENT | WINDOW_AUTO_KEYS | WINDOW_REQ_FILL | WINDOW_VISITOR | WINDOW_SIZE_BOTTOM |
					   IDCMP_IDCMPUPDATE;
		newwin.font = 0;

		// Try to load size
		if (LoadPos("dopus/windows/ftp", &pos, &old_font_size))
		{
			if (pos.Width < FTP_ADDRBOOK_MIN_SAVED_WIDTH || pos.Height < FTP_ADDRBOOK_MIN_SAVED_HEIGHT)
				old_font_size = 0;

			D(bug("font size %ld , old_font_size %ld\n", (long)dg->dg_og->og_screen->RastPort.TxHeight, (long)old_font_size));

			// Is font size the same?
			if (dg->dg_og->og_screen->RastPort.TxHeight == old_font_size)
			{
				newwin.dims->char_dim.Width = 0;
				newwin.dims->char_dim.Height = 0;
				newwin.dims->fine_dim.Width = pos.Width;
				newwin.dims->fine_dim.Height = pos.Height;
			}
		}

		// Reopening?
		if (old_font_size)
		{
			// Get position
			newwin.dims->char_dim.Left = 0;
			newwin.dims->char_dim.Top = 0;
			newwin.dims->fine_dim.Left = pos.Left;
			newwin.dims->fine_dim.Top = pos.Top;

			// calc / adjust dimension
			newwin.dims->char_dim.Height = pos.Height / old_font_size;
			newwin.dims->fine_dim.Height = pos.Height - newwin.dims->char_dim.Height * old_font_size;
		}

		// Open window, add objects
		if ((dg->dg_addrwp = open_configwin(dg, &newwin, ftp_main_objects, WT_ADR)))
		{
			SetConfigWindowLimits(dg->dg_addrwp->wp_win, &ftp_main_window, 0);

			// limit to vertical sizing only
			WindowLimits(dg->dg_addrwp->wp_win, dg->dg_addrwp->wp_win->Width, 0, dg->dg_addrwp->wp_win->Width, -1);

			// Add menus **NOTE these are freed automatically with closeconfigwindow
			AddWindowMenus(dg->dg_addrwp->wp_win, site_menus);

			// disable connect if no socket
			DisableObject(dg->dg_addrwp->wp_objlist,
						  GAD_FTP_CONNECT,
						  !(((struct globals *)(data->spd_ipc->userdata))->g_socketbase != NULL));

			// read config and display
			// config is read in setup_config
			// now keep site list in memory under semaphore
			// and free when addressbook shut down or low memory

			ObtainSemaphoreShared(&dg->dg_og->og_SiteList_semaphore);

			if (!dg->dg_og->og_SiteList)
				read_build_addressbook(dg->dg_og, dg->dg_ipc);

			// initialise the list and set to 1st entry
			SetGadgetChoices(dg->dg_addrwp->wp_objlist, GAD_FTP_SITES, dg->dg_og->og_SiteList);

			SetGadgetValue(dg->dg_addrwp->wp_objlist, GAD_FTP_SITES, 0);
			display_main_gadgets(dg, 0);

			// mark global pointer to say addressbook on screen
			dg->dg_og->og_addrbook_open = TRUE;

			return (dg->dg_addrwp);
		}
	}

	return (NULL);
}

/*******************************************************
 *
 *	If addressbook invoked again then check if the
 *	socket library has become available this time
 */

static void recheck_for_socketlib(struct opusftp_globals *ogp, IPCData *ipc)
{
	struct globals *g = ipc->userdata;

	if (!g->g_socketbase)
	{
		g->g_socketbase = OpenLibrary("bsdsocket.library", 3);
#ifdef __amigaos4__
		if (g->g_socketbase)
		{
			if (!(g->g_isocket = (struct SocketIFace *)GetInterface(g->g_socketbase, "main", 1, NULL)))
			{
				CloseLibrary(g->g_socketbase);
				g->g_socketbase = NULL;
			}
		}
#endif
	}
}

/***************************************************
 *
 * Add a list of sites to the main site list
 *
 */

static void add_to_main_list(struct display_globals *dg, Att_List *atlist)
{
	Att_Node *node;
	struct site_entry *e;
	struct List *list;

	list = &atlist->list;

	while (list->lh_TailPred != (struct Node *)list)  // IsListEmpty ?
	{
		node = (Att_Node *)list->lh_TailPred;

		// save data pointer
		e = (struct site_entry *)node->data;

		// remove from new list , data not freed
		Att_RemNode(node);

		// add to current list

		if (!Att_NewNode(dg->dg_og->og_SiteList, e->se_name, (IPTR)e, ADDNODE_SORT))
			break;
	}
	// discard any remaining list stuff

	Att_RemList(atlist, 0);
}

/***************************************************
 *
 * User gets a new toolbar file
 * remove DOpus5:System if selected path
 * defaults to "toolbar_ftp"
 *
 */

static BOOL get_toolbar(struct window_params *wp, char *filename)
{
	BOOL ok;
	char path[256], file[80];
	char *ptr = 0;

	*path = 0;

	stccpy(path, filename, 256);

	// Get filename
	if ((ptr = FilePart(path)))
	{
		strcpy(file, ptr);
		*ptr = 0;
		ptr = file;
	}

	if (!*ptr)
		ptr = "toolbar_ftp";

	if (!*path)
		strcpy(path, "DOpus5:Buttons");

	// Make window busy
	SetWindowBusy(wp->wp_win);

	// Display file requester
	if ((ok = AslRequestTags(DATA(wp->wp_win)->request,
							 ASLFR_Window,
							 wp->wp_win,
							 ASLFR_TitleText,
							 GetString(locale, MSG_SELECT_TOOLBAR),
							 ASLFR_InitialFile,
							 ptr,
							 ASLFR_InitialDrawer,
							 path,
							 ASLFR_Flags1,
							 FRF_PRIVATEIDCMP,
							 TAG_END)))
	{
		// Get new path

		if (stricmp("DOpus5:Buttons", DATA(wp->wp_win)->request->fr_Drawer))
		{
			strcpy(filename, DATA(wp->wp_win)->request->fr_Drawer);
			AddPart(filename, DATA(wp->wp_win)->request->fr_File, 256);
		}
		else
			stccpy(filename, DATA(wp->wp_win)->request->fr_File, 256);
	}

	// Ok to read?
	ClearWindowBusy(wp->wp_win);

	return (ok);
}

/***************************************************
 *
 *	 Show and hide routines
 */

static void hide(struct display_globals *dg, struct subproc_data *data, IPCMessage *imsg)
{
	close_all_windows(dg);
}

static void show(struct display_globals *dg, struct subproc_data *data, IPCMessage *imsg) {}

/***************************************************
 *
 *	Store a new directory for a specific site
 *
 */

static BOOL store_dir(struct display_globals *dg, struct subproc_data *data, IPCMessage *imsg)
{
	struct connect_msg *cm;
	struct site_entry *e;

	// get site_entry from connect message

	if ((cm = (struct connect_msg *)imsg->data))
	{
		if ((e = &cm->cm_site))
			update_sitelist(dg, e, NULL, SAVE_DIR);
	}

	return (TRUE);
}

/*********************************************
 *
 *	Idle and Event loop for Addressbook GUI windows
 *	looks for quit and SIGBREAKF_CTRL_C signals from AmiTCP
 *
 *	Also handles
 * 		IPC_ADDRBOOK:
 * 		IPC_CONNECTVISUAL:
 *		IPC_OPTIONS:
 *		IPC_ADD:
 *		IPC_HURRYUP: // dummy for synchronisation just ignore reply
 *		IPC_HIDE:
 *		IPC_CONFIGURE:
 *		IPC_SHOW:
 *		IPC_REMEMBERPATH:
 *		IPC_GOODBYE: 	// from format module
 *
 *
 */

#define GOT_NO_SELECTION (-1)
#define GOT_QUIT_EVENT (-2)

static void idle_loop(struct display_globals *dg, struct subproc_data *data, IPCMessage **loop_quitmsg)
{
	int selected = GOT_NO_SELECTION;
	LONG mask, signals;
	IPCMessage *imsg;
	struct IntuiMessage *msg;
	struct opusftp_globals *ogp;

	ogp = dg->dg_og;

	mask = SIGBREAKF_CTRL_C | dg->win_sig | (1 << data->spd_ipc->command_port->mp_SigBit);

	D(bug("Idle\n"));

	// Event loop
	while (TRUE)
	{
		BOOL quit_flag = 0;

		// Wait for event
		signals = Wait(mask);

		// Check for drag and drop deadlock
		if (dg->dg_draginfo && CheckTimer(dg->dg_timer))
		{
			// Deadlocked?
			if (dg->dg_tick_count == dg->dg_last_tick)
			{
				D(bug("** drag and drop deadlock!\n"));
				address_drag_end(dg, 0);
			}

			// Not deadlock, restart timer
			else
			{
				StartTimer(dg->dg_timer, 0, 500000);
				dg->dg_last_tick = dg->dg_tick_count;
			}
		}

		// Get messages	IPC messages
		if (signals & 1 << data->spd_ipc->command_port->mp_SigBit)
		{
			while ((imsg = (IPCMessage *)GetMsg(data->spd_ipc->command_port)))
			{
				struct window_params *tmpwp;
				int command_result = FALSE;

				D(bug("IPC event %lx \n", imsg->command));
				switch (imsg->command)
				{
				case IPC_QUIT:
					// save quit msg so we can replay after we cleanup
					*loop_quitmsg = imsg;
					imsg = NULL;
					quit_flag = GOT_QUIT_EVENT;
					break;

				case IPC_ADDRBOOK:
					// if get an IPC_ADDRBOOK then display main adr window(s)
					// save calling message and only reply when ADR closes
					// to maintain synchronicity of messages

					if (ogp->og_addrbook_open)
					{
						WindowToFront(dg->dg_addrwp->wp_win);
						ActivateWindow(dg->dg_addrwp->wp_win);
						command_result = TRUE;
					}
					else
					{
						recheck_for_socketlib(data->spd_ogp, data->spd_ipc);

						if ((tmpwp = show_addrbook(dg, data, imsg)))
						{
							tmpwp->wp_imsg = imsg;
							imsg = NULL;
						}
					}
					break;

				case IPC_CONNECTVISUAL:

					// if get an IPC_CONNECTVISUAL then show connect req window
					// save calling message and only reply when closed
					// to maintain synchronicity of messages

					if ((tmpwp = show_connect(dg, data, imsg)))	 // if ok
					{
						tmpwp->wp_imsg = imsg;
						imsg = NULL;
					}
					break;

				case IPC_OPTIONS:

					// if get an IPC_OPTIONS then show options req window
					// save calling message and only reply when closed
					// to maintain synchronicity of messages

					// default already open?
					if (imsg->flags == 0 && dg->dg_defoptions)
					{
						myWindowToFront(dg->dg_defoptions, TRUE);
						command_result = TRUE;
					}
					else
					{
						if ((tmpwp = show_ftp_options(dg, data, imsg)))	 // if ok
						{
							tmpwp->wp_imsg = imsg;
							imsg = NULL;
						}
					}
					break;

				case IPC_ADD:

					// if get an IPC_ADD then add the entry to the adressbook
					// save calling message and only reply when closed
					// to maintain synchronicity of messages

					if ((tmpwp = add_entry(dg, data, imsg)))  // if ok
					{
						tmpwp->wp_imsg = imsg;
						imsg = NULL;
					}
					break;

				case IPC_HURRYUP:  // dummy for synchronisation just ignore reply
					command_result = TRUE;
					break;

				case IPC_HIDE:
					hide(dg, data, imsg);
					command_result = TRUE;
					break;

				case IPC_CONFIGURE:
					// now asynchronous so we need to block with
					// holding the msg until we quit the format req
					// IPC_Reply is done by format process

					if (configure_format(dg, data, imsg))
						imsg = NULL;

					break;

				case IPC_SHOW:
					show(dg, data, imsg);
					command_result = TRUE;
					break;

				case IPC_REMEMBERPATH:
					if (store_dir(dg, data, imsg))
						command_result = TRUE;
					break;

				case IPC_GOODBYE:  // from format module
					if ((tmpwp = (struct window_params *)imsg->data))
						display_options_gadgets(tmpwp);
					break;

				default:
					command_result = FALSE;

					D(bug("** addrbook unknown IPC event (0x%lx %ld)\n", imsg->command, imsg->command));
					break;
				}

				if (imsg)
				{
					imsg->command = command_result;
					IPC_Reply(imsg);
				}

				if (quit_flag)
					break;
			}
		}

		// signal from AmiTCP to quit?
		if (signals & SIGBREAKF_CTRL_C)
			quit_flag = GOT_QUIT_EVENT;

		// Get window messages
		if (signals & dg->win_sig)
		{
			while ((msg = GetWindowMsg(dg->idcmp_port)))
			{
				struct window_params *wp;
				struct IntuiMessage msg_copy;
				UWORD gadgetid = 0;

				// Copy message and reply
				msg_copy = *msg;

				if (msg->Class != IDCMP_IDCMPUPDATE)  // keep for tags
					ReplyWindowMsg(msg);

				if (msg_copy.Class == IDCMP_GADGETUP || msg_copy.Class == IDCMP_GADGETDOWN)
					gadgetid = ((struct Gadget *)msg_copy.IAddress)->GadgetID;

				// get back ptr to our data

				// ****************************************
				// VIP NOTE: wp ONLY valid within while loop!
				// ****************************************

				wp = FINDWP(msg_copy.IDCMPWindow);

				// Examine message
				switch (msg_copy.Class)
				{
				case IDCMP_INTUITICKS:
					++dg->dg_tick_count;
					break;

				case IDCMP_VANILLAKEY:	// small site list
				{
					Att_Node *node;
					char key;
					short offset;
					Att_List *list = ogp->og_SiteList;

					// Get key press
					key = tolower(msg_copy.Code);

					// Search list for something starting with this key
					for (node = (Att_Node *)list->list.lh_Head, offset = 0; node->node.ln_Succ;
						 node = (Att_Node *)node->node.ln_Succ, offset++)
					{
						// Does first letter match?
						if (tolower(node->node.ln_Name[0]) == key)
						{
							// Position at this node
							SetGadgetValue(wp->wp_objlist, GAD_LIST_SITES, offset);
							break;
						}
					}
				}
				break;

				case IDCMP_MOUSEMOVE:
					if (dg->dg_draginfo)
					{
						ShowDragImage(dg->dg_draginfo,
									  dg->dg_addrwp->wp_win->WScreen->MouseX + dg->dg_drag_x,
									  dg->dg_addrwp->wp_win->WScreen->MouseY + dg->dg_drag_y);
					}
					break;

				case IDCMP_MOUSEBUTTONS:
					if (dg->dg_draginfo)
					{
						// Dropped?
						if (msg_copy.Code == SELECTUP)
						{
							SetFlag(dg->dg_config_changed, CHANGED_ADR);

							if (msg_copy.Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
								address_drag_end(dg, 2);
							else
								address_drag_end(dg, 1);
						}

						// Aborted?
						else if (msg_copy.Code == MENUDOWN)
							address_drag_end(dg, 0);
					}
					break;

				case IDCMP_IDCMPUPDATE: {
					// BOOPSI message
					struct TagItem *tags = (struct TagItem *)msg_copy.IAddress;
					int item;

					if (wp->wp_type == WT_ADRLIST)
					{
						if (GetTagData(GA_ID, 0, tags) == GAD_LIST_SITES)
						{
							if (GetTagData(DLV_DoubleClick, 0, tags))
							{
								// reply to the msg since we are about to close the winwow
								ReplyWindowMsg(msg);
								msg = NULL;
								close_conwin_by_type(wp, TRUE);
							}
						}
					}
					else
					{
						// no connect while editing
						{
							// Get ID
							if (GetTagData(GA_ID, 0, tags) == GAD_FTP_SITES)
							{
								// Double-click?
								if (GetTagData(DLV_DoubleClick, 0, tags))
								{
									if (!dg->dg_edit_count)
									{
										selected = dg->dg_selected;

										if (selected > GOT_NO_SELECTION)
										{
											// set to edit or connect?
											if (ogp->og_oc.oc_addr_dc == 1)	 // edit
												edit_entry(dg, msg_copy.IDCMPWindow);
											else
											{
												// is shift down?
												BOOL qual = get_qualified();

												send_connect(dg, selected, wp);

												if (!qual && ogp->og_oc.oc_addr_auto)
													quit_flag = GAD_FTP_SAVE;
												else
													quit_flag = GAD_FTP_CONNECT;
											}
										}
									}
								}
								// Drag?
								else if ((item = GetTagData(DLV_DragNotify, -1, tags)) != -1)
								{
									address_drag_start(dg,
													   item,
													   GetTagData(GA_Left, 0, tags),
													   GetTagData(GA_Top, 0, tags),
													   GetTagData(GA_Width, 0, tags),
													   GetTagData(GA_Height, 0, tags),
													   -GetTagData(GA_RelRight, 0, tags),
													   -GetTagData(GA_RelBottom, 0, tags));
								}
							}
						}
					}
				}

					if (msg)
						;
					ReplyWindowMsg(msg);
					break;

				case IDCMP_RAWKEY:
					// Help?
					if (msg_copy.Code == 0x5f && !(msg_copy.Qualifier & VALID_QUALIFIERS))
					{
						// Set busy pointer
						SetWindowBusy(wp->wp_win);

						// Send help request
						IPC_Command(ogp->og_opus_ipc, IPC_HELP, (1 << 31), "FTPAddressbook", 0, 0);

						// Clear busy pointer
						ClearWindowBusy(wp->wp_win);
					}
					break;

				// Shut down
				case IDCMP_CLOSEWINDOW:

					switch (wp->wp_type)
					{
					case WT_ADR:
						gadgetid = GAD_FTP_CANCEL;
						break;

					case WT_ADD:
					case WT_EDIT:
					case WT_NEW:
						gadgetid = GAD_EDIT_CANCEL;
						break;

					case WT_OPT:
					case WT_DEFOPT:
						gadgetid = GAD_ENV_CANCEL;
						break;

					case WT_CON:
						gadgetid = GAD_CONNECT_CANCEL;
						break;

					case WT_ADRLIST:
						gadgetid = GAD_LIST_CANCEL;

					default:
						break;
					}

					if (!gadgetid)
						break;

					// else drop through

				// Menu
				case IDCMP_MENUPICK:
					if (msg_copy.Class == IDCMP_MENUPICK)
					{
						struct MenuItem *item;

						// Get item
						if (!(item = ItemAddress(msg_copy.IDCMPWindow->MenuStrip, msg_copy.Code)))
							break;

						// Treat as gadget
						gadgetid = MENUID(item);
					}

					// drop through

				// Gadget event
				case IDCMP_GADGETDOWN:
				case IDCMP_GADGETUP:

					// Look at gadget ID
					switch (gadgetid)
					{
					/********************** main addressbook ftp gadgets ****/
					case GAD_FTP_CONNECT:

						selected = dg->dg_selected;

						if (selected > GOT_NO_SELECTION)
						{
							// is shift down?
							BOOL qual = get_qualified();

							send_connect(dg, selected, wp);

							if (!qual && ogp->og_oc.oc_addr_auto)
								quit_flag = GAD_FTP_SAVE;
							else
								quit_flag = GAD_FTP_CONNECT;  // will not close adr win
						}

						break;

					// Quit program check for changed?
					case GAD_FTP_CANCEL:
					case MENU_FTP_CANCEL:
						quit_flag = GAD_FTP_CANCEL;

						// if user selects cancel and has changed then reload old!

						if (FlagIsSet(dg->dg_config_changed, CHANGED_ADR))
						{
							ReleaseSemaphore(&ogp->og_SiteList_semaphore);
							free_address_book(ogp);
							ObtainSemaphoreShared(&ogp->og_SiteList_semaphore);

							if (!ogp->og_SiteList)
								read_build_addressbook(ogp, dg->dg_ipc);

							ClearFlag(dg->dg_config_changed, CHANGED_ADR);
						}

						break;

					// New entry
					case GAD_FTP_NEW:
						new_entry(dg, msg_copy.IDCMPWindow);
						break;

					// Delete entry
					case GAD_FTP_DELETE:
						delete_entry(dg);
						break;

					case GAD_FTP_SORT:
						// detach list
						SetGadgetChoices(wp->wp_objlist, GAD_FTP_SITES, (APTR)~0);

						sort_list(ogp->og_SiteList);

						// add list
						SetGadgetChoices(wp->wp_objlist, GAD_FTP_SITES, ogp->og_SiteList);

						dg->dg_selected = 0;

						SetGadgetValue(wp->wp_objlist, GAD_FTP_SITES, 0);

						// mark as changed site details
						SetFlag(dg->dg_config_changed, CHANGED_ADR);

						// redisplay the gadgets
						display_main_gadgets(dg, 0);

						break;

					// Edit entry
					case GAD_FTP_EDIT:
						edit_entry(dg, msg_copy.IDCMPWindow);
						break;

					case GAD_FTP_SITES:
						display_main_gadgets(dg, msg_copy.Code);
						break;

					case GAD_FTP_SET_DEFAULT_OPTIONS:
						edit_options(dg, msg_copy.IDCMPWindow, WT_DEFOPT);
						break;

					case GAD_FTP_SAVE:
						// auto save at exit of switch statement
						quit_flag = GAD_FTP_SAVE;
						break;

					case MENU_FTP_SAVEAS:
					case MENU_FTP_SAVE:

						save_sites(dg, wp->wp_win, gadgetid);

						if (gadgetid == MENU_FTP_SAVE)
							ClearFlag(dg->dg_config_changed, CHANGED_ADR);

						adjust_save_menu(dg);

						break;

					case MENU_FTP_OPEN: {
						Att_List *list;
						if ((list = read_sites(wp, gadgetid)))
						{
							// detach list
							SetGadgetChoices(wp->wp_objlist, GAD_FTP_SITES, (APTR)~0);

							Att_RemList(ogp->og_SiteList, REMLIST_FREEDATA);

							ogp->og_SiteList = list;

							// add list
							SetGadgetChoices(wp->wp_objlist, GAD_FTP_SITES, dg->dg_og->og_SiteList);

							dg->dg_selected = 0;

							SetGadgetValue(wp->wp_objlist, GAD_FTP_SITES, 0);

							// mark as changed site details
							ClearFlag(dg->dg_config_changed, CHANGED_ADR);

							// redisplay the gadgets
							display_main_gadgets(dg, 0);
						}
					}
					break;

					case MENU_FTP_IMPORT:
					case MENU_FTP_IMPORT_AMFTP: {
						Att_List *list;
						if ((list = import_sites(wp, gadgetid)))
						{
							// detach list
							SetGadgetChoices(wp->wp_objlist, GAD_FTP_SITES, (APTR)~0);

							add_to_main_list(dg, list);

							// add list
							SetGadgetChoices(wp->wp_objlist, GAD_FTP_SITES, dg->dg_og->og_SiteList);

							dg->dg_selected = 0;

							SetGadgetValue(wp->wp_objlist, GAD_FTP_SITES, 0);

							// mark as changed site details
							SetFlag(dg->dg_config_changed, CHANGED_ADR);

							// redisplay the gadgets
							display_main_gadgets(dg, 0);
						}
					}
					break;

					case MENU_FTP_EXPORT:
						export_sites(dg, msg_copy.IDCMPWindow);
						break;

						/********************* edit gadgets *********************/

					case GAD_EDIT_CUSTOM_OPTIONS: {
						int mx_value;

						mx_value = GetGadgetValue(wp->wp_objlist, GAD_EDIT_CUSTOM_OPTIONS);
						DisableObject(wp->wp_objlist, GAD_EDIT_SET_CUSTOM_OPTIONS, !mx_value);

						if (mx_value)
							site_make_custom_env(dg, wp->wp_se_copy);
						else
						{
							wp->wp_se_copy->se_has_custom_env = FALSE;
							wp->wp_se_copy->se_env = &dg->dg_og->og_oc.oc_env;
						}

						// user selected custom?
						if (mx_value)
							edit_options(dg, msg_copy.IDCMPWindow, WT_OPT);
					}

					break;

					// Cancel in editor
					case GAD_EDIT_CANCEL:
						close_conwin_by_type(wp, FALSE);
						break;
					// Ok in editor
					case GAD_EDIT_OK:
						close_conwin_by_type(wp, TRUE);
						break;

					case GAD_EDIT_HOST: {
						char *host, *name;
						char newname[HOSTNAMELEN + 1];

						update_connection_from_host_gadget(wp->wp_objlist,
														   dg,
														   wp->wp_se_copy,
														   GAD_EDIT_HOST,
														   GAD_EDIT_PROTOCOL,
														   GAD_EDIT_PORT,
														   GAD_EDIT_ANON,
														   GAD_EDIT_USER,
														   GAD_EDIT_PASSWORD);
						SetGadgetValue(wp->wp_objlist, GAD_EDIT_CUSTOM_OPTIONS, wp->wp_se_copy->se_has_custom_env);
						DisableObject(
							wp->wp_objlist, GAD_EDIT_SET_CUSTOM_OPTIONS, !wp->wp_se_copy->se_has_custom_env);

						name = (char *)GetGadgetValue(wp->wp_objlist, GAD_EDIT_NAME);

						if (!name || !*name)
						{
							host = (char *)GetGadgetValue(wp->wp_objlist, GAD_EDIT_HOST);

							sitename_from_host(newname, host);
							SetGadgetValue(wp->wp_objlist, GAD_EDIT_NAME, (IPTR)newname);
						}
					}
					break;

					case GAD_EDIT_PORT:
						check_bounds(dg, &msg_copy, 65535, ftp_protocol_default_port(site_protocol(wp->wp_se_copy)));
						break;

					case GAD_EDIT_PROTOCOL:
						update_connection_from_protocol_gadget(wp->wp_objlist,
															   dg,
															   wp->wp_se_copy,
															   GAD_EDIT_PROTOCOL,
															   GAD_EDIT_PORT,
															   GAD_EDIT_ANON,
															   GAD_EDIT_USER,
															   GAD_EDIT_PASSWORD);
						SetGadgetValue(wp->wp_objlist, GAD_EDIT_CUSTOM_OPTIONS, wp->wp_se_copy->se_has_custom_env);
						DisableObject(
							wp->wp_objlist, GAD_EDIT_SET_CUSTOM_OPTIONS, !wp->wp_se_copy->se_has_custom_env);
						break;

					case GAD_EDIT_ANON:
						if (site_protocol(wp->wp_se_copy) == FTP_PROTOCOL_SFTP)
							wp->wp_se_copy->se_anon = FALSE;
						else
							wp->wp_se_copy->se_anon = GetGadgetValue(wp->wp_objlist, GAD_EDIT_ANON);
						update_connection_gadgets(wp->wp_objlist,
												  dg,
												  wp->wp_se_copy,
												  GAD_EDIT_PROTOCOL,
												  GAD_EDIT_PORT,
												  GAD_EDIT_ANON,
												  GAD_EDIT_USER,
												  GAD_EDIT_PASSWORD);
						break;

					case GAD_EDIT_SET_CUSTOM_OPTIONS:
						edit_options(dg, msg_copy.IDCMPWindow, WT_OPT);
						break;

					/********************* options gadgets *******************/

					// Cancel in options
					case GAD_ENV_CANCEL:
					case MENU_ENV_CANCEL:
						close_conwin_by_type(wp, FALSE);
						break;

					case GAD_ENV_SAVE:
						close_conwin_by_type(wp, TRUE);
						break;

					case GAD_ENV_ITEM_LISTER: {
						Att_Node *option_node;

						// Get selected entry
						if (!(option_node = Att_FindNode(wp->wp_options_list, msg_copy.Code)))
							break;

						// Same as last selection?
						if (wp->wp_option == wp->wp_SubOptionsHandle[option_node->data].num)
							break;

						// See if we have an option already
						if (wp->wp_sub_objlist)
						{
							// Store gadget values
							store_options_gadgets(wp);

							// Free objects
							FreeObjectList(wp->wp_sub_objlist);

							// Erase display
							SetGadgetValue(wp->wp_objlist, GAD_ENV_EDIT_AREA, 0);
						}
						// Create option list
						wp->wp_option = wp->wp_SubOptionsHandle[option_node->data].num;
						wp->wp_sub_objlist =
							AddObjectList(wp->wp_win, wp->wp_SubOptionsHandle[option_node->data].objects);

						// Initialise gadgets
						display_options_gadgets(wp);
					}
					break;

					// these require bounds checking
					case GAD_ENV_RETRY_COUNT:
						check_bounds(dg, &msg_copy, 99, 5);
						break;

					case GAD_ENV_RETRY_DELAY:
						check_bounds(dg, &msg_copy, 600, 30);
						break;

					case GAD_ENV_TIMEOUT:
						check_bounds(dg, &msg_copy, 999, 60);
						break;

					case GAD_ENV_LIST_UPDATE:
						check_bounds(dg, &msg_copy, 99, 10);
						break;

					case GAD_ENV_AUTO_DOWNLOAD_SIZE:
						check_bounds(dg, &msg_copy, 9999, 30);
						break;

					case GAD_ENV_SCRIPT_TIME:
						check_bounds(dg, &msg_copy, 999, 120);
						break;

					case GAD_ENV_RESET_DEFAULT:
						reset_default_options(wp);
						display_options_gadgets(wp);
						break;

					case GAD_ENV_SET_FORMAT:
						get_listformat(wp);
						break;

					case GAD_ENV_TOOLBAR_GLASS: {
						char new[256];

						stccpy(new, (char *)GetGadgetValue(wp->wp_sub_objlist, GAD_ENV_TOOLBAR), 256);

						if (get_toolbar(wp, new))
							SetGadgetValue(wp->wp_sub_objlist, GAD_ENV_TOOLBAR, (IPTR) new);
					}
					break;

					case GAD_ENV_TOOLBAR:
						break;

					case GAD_ENV_LOG_ENABLE:
						break;

						/*
								case GAD_ENV_ENABLE_RETRY:
								case GAD_ENV_ENABLE_RETRY_LOST:
								case GAD_ENV_INDEX_ENABLE:
								case GAD_ENV_AUTO_DOWNLOAD:
								case GAD_ENV_DEBUG:
								case GAD_ENV_LOG_FILE:
								case GAD_ENV_ANON_PASSWORD:
								case GAD_ENV_REPLACE:
								case GAD_ENV_COPY_TYPE:
								case GAD_ENV_COPY_DATESTAMP:
								case GAD_ENV_COPY_PROTECTION:
								case GAD_ENV_COPY_COMMENT:
									break;
						*/

					case GAD_ENV_COPY_OPUS_DEFAULT: {
						int val = GetGadgetValue(wp->wp_sub_objlist, GAD_ENV_COPY_OPUS_DEFAULT);
						DisableObject(wp->wp_sub_objlist, GAD_ENV_COPY_DATESTAMP, val);
						DisableObject(wp->wp_sub_objlist, GAD_ENV_COPY_PROTECTION, val);
						DisableObject(wp->wp_sub_objlist, GAD_ENV_COPY_COMMENT, val);
					}
					break;

					case MENU_ENV_SAVEAS:
						save_options(wp, wp->wp_type, gadgetid);
						break;

					case MENU_ENV_OPEN: {
						struct ftp_config *conf;

						if ((conf = read_options(wp, wp->wp_type)))
						{
							if (wp->wp_type == WT_OPT)
							{
								*(&wp->wp_se_copy->se_env_private) = *(&conf->oc_env);
								wp->wp_se_copy->se_env = &wp->wp_se_copy->se_env_private;
								wp->wp_se_copy->se_has_custom_env = TRUE;
							}
							else
								*(&dg->dg_oc) = *conf;

							// free old one
							FreeVec(conf);
							display_options_gadgets(wp);
						}
					}
					break;

					case MENU_ENV_RESET_DEFAULTS:
						reset_default_options(wp);
						display_options_gadgets(wp);
						break;

					case MENU_ENV_LAST_SAVED:
						break;

					case MENU_ENV_RESTORE:
						// restore to value before any changes
						restore_options(wp);
						display_options_gadgets(wp);
						break;

						/********************** connect gadgets ***************/

					case GAD_CONNECT_GLASS:
						small_site_list(dg, wp);
						break;

					case GAD_CONNECT_NAME: {
						char *name;

						// see if user enter ? as name to access site list
						// or if name entered and host field blank
						if ((name = (char *)GetGadgetValue(wp->wp_objlist, GAD_CONNECT_NAME)))
						{
							if (*name)
							{
								if (*name == '?')
									small_site_list(dg, wp);

								else
								{
									// name entered but NO site? then lookup
									name = (char *)GetGadgetValue(wp->wp_objlist, GAD_CONNECT_HOST);
									if (!*name)
										small_site_list(dg, wp);
								}
							}
						}
					}
					break;

					case GAD_CONNECT_PORT:
						check_bounds(dg, &msg_copy, 65535, ftp_protocol_default_port(site_protocol(wp->wp_se_copy)));
						break;

					case GAD_CONNECT_HOST:
						update_connection_from_host_gadget(wp->wp_objlist,
														   dg,
														   wp->wp_se_copy,
														   GAD_CONNECT_HOST,
														   GAD_CONNECT_PROTOCOL,
														   GAD_CONNECT_PORT,
														   GAD_CONNECT_ANON,
														   GAD_CONNECT_USER,
														   GAD_CONNECT_PASSWORD);
						break;

					case GAD_CONNECT_PROTOCOL:
						update_connection_from_protocol_gadget(wp->wp_objlist,
															   dg,
															   wp->wp_se_copy,
															   GAD_CONNECT_PROTOCOL,
															   GAD_CONNECT_PORT,
															   GAD_CONNECT_ANON,
															   GAD_CONNECT_USER,
															   GAD_CONNECT_PASSWORD);
						break;

					case GAD_CONNECT_USER:
					case GAD_CONNECT_PASSWORD:
					case GAD_CONNECT_DIR:
						break;

					case GAD_CONNECT_ANON:
						if (site_protocol(wp->wp_se_copy) == FTP_PROTOCOL_SFTP)
							wp->wp_se_copy->se_anon = FALSE;
						else
							wp->wp_se_copy->se_anon = GetGadgetValue(wp->wp_objlist, GAD_CONNECT_ANON);
						update_connection_gadgets(wp->wp_objlist,
												  dg,
												  wp->wp_se_copy,
												  GAD_CONNECT_PROTOCOL,
												  GAD_CONNECT_PORT,
												  GAD_CONNECT_ANON,
												  GAD_CONNECT_USER,
												  GAD_CONNECT_PASSWORD);
						break;

					case GAD_CONNECT_LAST:

					{
						WORD active;

						active = wp->wp_se_copy->se_active_gadget;

						// Put last site in GUI
						if (dg->dg_og->og_valid_site)
						{
							copy_site_entry(dg->dg_og, wp->wp_se_copy, &dg->dg_og->og_last_site);
							wp->wp_se_copy->se_active_gadget = active;

							display_connect_gadgets(dg, wp);

							if (active)
							{
								// safety incase bad id and gadget not found
								GL_Object *obj;

								obj = GetObject(wp->wp_objlist, active);

								// now now just activate the first gadget
								if (obj)
									ActivateStrGad(GADGET(obj), wp->wp_win);
							}
						}
					}

					break;

					case GAD_CONNECT_CONNECT:
						close_conwin_by_type(wp, TRUE);
						break;

					case GAD_CONNECT_CANCEL:
						close_conwin_by_type(wp, FALSE);
						break;

					case GAD_LIST_SITES:
						break;

					case GAD_LIST_OK:
						close_conwin_by_type(wp, TRUE);
						break;

					case GAD_LIST_CANCEL:
						close_conwin_by_type(wp, FALSE);
						break;

					default:

						break;

					}  // end switch gadgetid

					// update reset to defaults
					if (gadgetid >= GAD_ENV_START_UPDATE && gadgetid <= GAD_ENV_END_UPDATE)
					{
						if (gadgetid != GAD_ENV_SET_FORMAT)
						{
							store_options_gadgets(wp);
							display_options_gadgets(wp);
						}
					}

					break;

				default:  // default case

					break;

				}  // end switch case
			}	   // end while getmsg
		}		   // end if sig userport

		// if addressbook changed and quit then save it automatically

		if (quit_flag == GAD_FTP_SAVE)
		{
			if (FlagIsSet(dg->dg_config_changed, CHANGED_ADR))
			{
				save_sites(dg, dg->dg_addrwp->wp_win, GAD_FTP_SAVE);
				ClearFlag(dg->dg_config_changed, CHANGED_ADR);
			}
			close_addrbook(dg->dg_addrwp, TRUE);
		}

		if (quit_flag == GAD_FTP_CANCEL)
			close_addrbook(dg->dg_addrwp, FALSE);

		if (quit_flag == GOT_QUIT_EVENT)
			break;

	}  // while true

	// finished so close any child windows

	close_all_windows(dg);

	D(bug("end idle\n"));
}

static void dump_IDCMP_messages(struct display_globals *dg)
{
	struct IntuiMessage *msg;

	while ((msg = GetWindowMsg(dg->idcmp_port)))
		ReplyWindowMsg(msg);
}

/******************************************************************
 *
 *	Cleanup custom data for gui support structures
 */

static void cleanup_globals(struct display_globals *dg)

{
	if (dg)
	{
		if (dg->idcmp_port)
		{
			dump_IDCMP_messages(dg);
			DeletePort(dg->idcmp_port);
		}

		// free the last node for dg window
		if (dg->dg_wp_list)
			Att_RemList(dg->dg_wp_list, 0);

		// free my data
		FreeVec(dg);
	}
}

/******************************************************************
 *
 *	Allocate and init custom data for gui support structures
 */

static struct display_globals *init_globals(struct subproc_data *data)
{
	struct display_globals *dg;

	if ((dg = AllocVec(sizeof(struct display_globals), MEMF_CLEAR)))
	{
		dg->dg_og = data->spd_ogp;	// get global data ptr
		dg->dg_ipc = data->spd_ipc;

		if ((dg->dg_wp_list = Att_NewList(LISTF_POOL)))
		{
			// make tmp copy of current global ftp_config
			*(&dg->dg_oc) = *(&dg->dg_og->og_oc);

			// initialise a default config for comparisons
			set_config_to_default(&dg->dg_default_oc);

			// get current start of log file
			dg->dg_log_open = dg->dg_og->og_log_open;

			// read addresbook into memory for later use.
			// Not under semaphore so can be freed by low mem handler

			if (!dg->dg_og->og_SiteList)
				read_build_addressbook(dg->dg_og, dg->dg_ipc);

			if ((dg->idcmp_port = (struct MsgPort *)CreatePort(NULL, 0)))
			{
				dg->win_sig = 1L << dg->idcmp_port->mp_SigBit;
				return (dg);
			}
		}

		// failed
		cleanup_globals(dg);
	}

	return (NULL);
}

static void addressbook_cleanup(struct opusftp_globals *ogp, IPCData *ipc)
{
	struct globals *g = ipc->userdata;

	if (g)
	{
#ifdef __amigaos4__
		if (g->g_isocket)
			DropInterface((struct Interface *)g->g_isocket);
#endif
		if (g->g_socketbase)
			CloseLibrary(g->g_socketbase);
		FreeVec(g);
	}

	free_address_book(ogp);
}

/*****************************************************************
 *	Init code for process
 *	 Since we use the util function getuseraddress() which calls the socket library, we need to
 * 	 open it and store it in the 'userdata' field of our IPCData
 *
 */
//#if defined(__amigaos4__)
// ULONG ASM addressbook_init( REG(a0, IPCData *ipc), REG(a2, int skip), REG(a1, struct subproc_data *data ))
//#else
IPC_StartupCode(addressbook_init, struct subproc_data *, data)
//#endif
{
	struct globals *g;
	struct opusftp_globals *ogp;

	if (data)
	{
		ogp = data->spd_ogp;

		data->spd_ipc = ipc; /* 'ipc' points to this task's tc_UserData field */

		if ((g = ipc->userdata = AllocVec(sizeof(struct globals), MEMF_CLEAR)))
		{
			g->g_socketbase = OpenLibrary("bsdsocket.library", 3);
#ifdef __amigaos4__
			if (g->g_socketbase)
			{
				if (!(g->g_isocket = (struct SocketIFace *)GetInterface(g->g_socketbase, "main", 1, NULL)))
				{
					CloseLibrary(g->g_socketbase);
					g->g_socketbase = NULL;
				}
			}
#endif
		}
		else
			addressbook_cleanup(ogp, ipc);
	}
	return (1);
}

/*****************************************************************
 *
 *	Addressbook process started by main opusftp
 *	Initialises the system then sits in idle_loop waiting for messages
 */

void addressbook(void)
{
	// struct Library	*DOpusBase;
	struct subproc_data *data = 0;
	IPCMessage *loop_quitmsg = 0;
	struct display_globals *dg;

	/*if	((DOpusBase = OpenLibrary( "dopus5.library", VERSION_DOPUSLIB )))
		{
		#ifdef __amigaos4__
		IDOpus = (struct DOpusIFace *)GetInterface(DOpusBase, "main", 1, NULL);
		#endif*/

	/* returns true if 'data' is filled in correctly */
	if (IPC_ProcStartup((IPTR *)&data, (APTR)&addressbook_init))
	{
		if ((dg = init_globals(data)))
		{
			idle_loop(dg, data, &loop_quitmsg);
			cleanup_globals(dg);
		}

		// if there is one then
		// Reply to the quit message (might want to do some cleanup first)
		if (loop_quitmsg)
			IPC_Reply(loop_quitmsg);

		IPC_Flush(data->spd_ipc);
		IPC_Goodbye(data->spd_ipc, data->spd_owner_ipc, 0);
		addressbook_cleanup(data->spd_ogp, data->spd_ipc);

		IPC_Free(data->spd_ipc);
		FreeVec(data);
	}

	/*#ifdef __amigaos4__
	DropInterface((struct Interface *)IDOpus);
	#endif
	CloseLibrary(DOpusBase);
	}*/
}
