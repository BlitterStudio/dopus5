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

#include "play.h"
#include <devices/audio.h>
#include <dopus/common.h>
#if defined(__amigaos3__) || defined(__MORPHOS__)
static const char *const modtypes[] = {"SoundTracker", "MED", "Octalyzer", "SoundTracker 15"};
struct Library *MUSICBase = NULL;
#endif

int LIBFUNC L_Module_Entry(REG(a0, struct List *files),
						   REG(a1, struct Screen *screen),
						   REG(a2, IPCData *ipc),
						   REG(a3, IPCData *main_ipc),
						   REG(d0, IPTR mod_id),
						   REG(d1, IPTR mod_data))
{
	play_data *data;
	struct Node *node;
	short ret = 1;

	// Allocate data
	if (!(data = AllocVec(sizeof(play_data), MEMF_CLEAR)))
		return 0;

	// Save datatypes library base
	// data->dt_base = DataTypesBase;

	// Allocate signal number for dt to talk to us with
	data->dt_signal = AllocSignal(-1);
	// Use Ctrl-E signal if no user signals available
	if (data->dt_signal == -1)
		data->dt_signal = SIGBREAKB_CTRL_E;

	// Allocate message port
	data->app_port = CreateMsgPort();

	// Store IPC pointer
	data->ipc = ipc;

	// Get colour clock
	data->clock = (((struct GfxBase *)GfxBase)->DisplayFlags & PAL) ? 3546895 : 3579545;

	// Cache screen and flags
	data->screen = screen;
	data->flags = mod_data;

	// Initialise icon position
	data->icon_x = NO_ICON_POSITION;
	data->icon_y = NO_ICON_POSITION;

#if defined(__amigaos3__) || defined(__MORPHOS__)
	MUSICBase = NULL;
#endif

	// Environment variable set?
	if (GetVar(ENV_PLAY, data->buf, sizeof(data->buf), GVF_GLOBAL_ONLY) > 0)
	{
		char *ptr = data->buf;

		// Get icon position
		read_parse_set(&ptr, (long *)&data->icon_x);
		read_parse_set(&ptr, (long *)&data->icon_y);
	}

	if (files->lh_Head->ln_Succ->ln_Succ)
		data->multiple = TRUE;
	else
		data->multiple = FALSE;

	// Go through files
	for (node = files->lh_Head; node->ln_Succ; node = node->ln_Succ)
	{
		short a;

		// Store file name pointer
		data->file = node->ln_Name;

		// Need to open icon?
		if (!data->app_icon && data->flags & PLAYF_ICON)
		{
			// Open AppIcon
			play_iconify(data);
			data->flags &= ~PLAYF_ICON;
		}

		// Need to open window?
		else if (!data->window && !data->app_icon && !(data->flags & PLAYF_QUIET))
		{
			// Open the window
			play_open_window(data, screen, (struct Window *)mod_id);
			mod_id = 0;

			// Set busy pointer
			if (data->window)
				SetWindowBusy(data->window);
		}

		// Play file
		a = play_file(data);
		play_cleanup(data);

		// Aborted?
		if (!a)
		{
			ret = -1;
			break;
		}
	}

	// Free data
	play_free(data);

	return ret;
}

// Free play data
void play_free(play_data *data)
{
	if (data)
	{
		// Remove AppIcon
		if (data->app_icon)
			RemoveAppIcon(data->app_icon);

		// Free icon
		if (data->icon)
			FreeDiskObject(data->icon);

		// Close window
		CloseConfigWindow(data->window);

		// Free message port
		DeleteMsgPort(data->app_port);

#if defined(__amigaos3__) || defined(__MORPHOS__)
		// Close music library
		if (MUSICBase)
			CloseLibrary(MUSICBase);
#endif

		// Free signal
		if (data->dt_signal != SIGBREAKB_CTRL_E)
			FreeSignal(data->dt_signal);

		// Free data
		FreeVec(data);
	}
}

// Play a sound file
BOOL play_file(play_data *data)
{
	short volume;

	// Initialise
	data->module_type = 0;

	// Get volume - might be needed if we add a MOD player
	volume = (data->flags >> 8) & 0x7f;
	if (volume == 0 || volume > 64)
		volume = 64;

	// Initialise info fields
	play_update_info(data, FilePart(data->file), 0, 0, GetString(locale, MSG_STATUS_LOADING));

	// If we've got datatypes, see if it matches
	if (DataTypesBase && (data->dt_object = NewDTObject(data->file,
														SDTA_SignalTask,
														(IPTR)FindTask(0),
														SDTA_SignalBit,
														1 << data->dt_signal,
														DTA_GroupID,
														GID_SOUND,
														TAG_END)))
	{
		struct dtTrigger trigger;
		struct DataType *dt = 0;

		// Set sound things
		SetDTAttrs(data->dt_object,
				   0,
				   0,
				   SDTA_SignalTask,
				   (IPTR)FindTask(0),
				   SDTA_SignalBit,
				   1 << data->dt_signal,
				   SDTA_Volume,
				   volume,
				   TAG_END);

		// Get datatype pointer
		GetAttr(DTA_DataType, data->dt_object, (APTR)&dt);

		// Update status
		play_update_length(data);
		play_update_info(data,
						 (char *)-1,
						 (char *)-1,
						 (dt) ? (char *)dt->dtn_Header->dth_Name : (char *)-1,
						 GetString(locale, MSG_STATUS_PLAYING));

		// Trigger sound
		trigger.MethodID = DTM_TRIGGER;
		trigger.dtt_GInfo = 0;
		trigger.dtt_Function = STM_PLAY;
		trigger.dtt_Data = 0;

		// Play sound
		SetSignal(0, 1 << data->dt_signal);
		DoDTMethodA(data->dt_object, 0, 0, (Msg)(APTR)&trigger);

		// Clear busy pointer
		if (data->window)
			ClearWindowBusy(data->window);

		// Wait until completed or aborted
		return wait_for_play(data, 1);
	}

#if defined(__amigaos3__) || defined(__MORPHOS__)
	else
	{
		// Open music library
		if (!MUSICBase && !(MUSICBase = OpenLibrary("DOpus5:Libs/inovamusic.library", 0)))
			MUSICBase = OpenLibrary("inovamusic.library", 0);

		// Try for module
		if (MUSICBase && ((data->module_type = IsModule(data->file)) > 0 && data->module_type < 100))
		{
			// Start module playing
			if (PlayModule(data->file, TRUE))
			{
				// Failed
				FlushModule();
				return 1;
			}

			// Playing OK
			else
			{
				BOOL res;
				const char *type;

				// Get module type
				type = (data->module_type >= MOD_STNTPT && data->module_type <= MOD_OLDST)
						   ? modtypes[data->module_type - 1]
						   : "Module";

				// Update info
				play_update_info(data, (char *)-1, "???", type, GetString(locale, MSG_STATUS_PLAYING));

				// Clear busy pointer
				if (data->window)
					ClearWindowBusy(data->window);

				// Wait for play to finish
				res = wait_for_play(data, 0);

				// Flush module
				FlushModule();
				return res;
			}
		}
	}
#endif /* __amigaos3__ &  __MORPHOS__ */

	return 1;
}

// Cleanup after playing a sound
void play_cleanup(play_data *data)
{
	// Free dt object
	if (data->dt_object)
	{
		DisposeDTObject(data->dt_object);
		data->dt_object = 0;
	}

	// Set busy pointer
	if (data->window)
		SetWindowBusy(data->window);
}

// Update length info field
void play_update_length(play_data *data)
{
	ULONG minutes, seconds;	 // DivideU() stores a ULONG remainder
	unsigned long length = 0;
	unsigned long period = 0;
	char length_buf[10];

	GetDTAttrs(data->dt_object, SDTA_SampleLength, &length, SDTA_Period, &period, TAG_DONE);

	// Get time to play
	minutes = DivideU(UDivMod32((length * period), data->clock), 60, &seconds, UtilityBase);
	if (minutes == 0 && seconds == 0)
		seconds = 1;

	// Build length string
	lsprintf(length_buf, "%02ld:%02ld", minutes, seconds);

	// Update display
	play_update_info(data, (char *)-1, length_buf, (char *)-1, (char *)-1);
}

// Wait for play
BOOL wait_for_play(play_data *data, BOOL dt)
{
	// Wait until aborted
	FOREVER
	{
		IPCMessage *imsg;

		// IPC messages?
		if (data->ipc)
		{
			short quit_flag = 0;

			while ((imsg = (IPCMessage *)GetMsg(data->ipc->command_port)))
			{
				// Abort?
				if (imsg->command == IPC_ABORT || imsg->command == IPC_QUIT)
					quit_flag = 1;

				IPC_Reply(imsg);
			}

			// Abort?
			if (quit_flag)
				return 0;
		}

		// AppIcon open?
		if (data->app_icon)
		{
			struct AppSnapshotMsg *msg;
			short ret = 0, done = 0;

			// Get messages
			while ((msg = (struct AppSnapshotMsg *)GetMsg(data->app_port)))
			{
				// Still got AppIcon?
				if (data->app_icon)
				{
					// Normal message?
					if (msg->ap_msg.am_Type == MTYPE_APPICON)
					{
						// Open up
						play_reveal(data, data->screen);
					}

					// Special message?
					else if (msg->ap_msg.am_Type == MTYPE_APPSNAPSHOT)
					{
						// Menu operation?
						if (msg->flags & APPSNAPF_MENU)
						{
							// Next?
							if (msg->id == 0)
							{
								ret = 1;
								done = 1;
							}

							// Abort?
							else if (msg->id == 1)
							{
								done = 1;
							}
						}

						// Snapshot
						else
							play_snapshot(data, msg);
					}
				}

				// Reply to the message
				ReplyFreeMsg((struct Message *)msg);
			}

			// Finished?
			if (done)
				return ret;
		}

		// Window open?
		if (data->window)
		{
			struct IntuiMessage *msg, copy_msg;

			// Any Intuition messages?
			while ((msg = GetWindowMsg(data->window->UserPort)))
			{
				// Copy message and reply
				copy_msg = *msg;
				ReplyWindowMsg(msg);

				// Gadget?
				if (copy_msg.Class == IDCMP_GADGETUP)
				{
					switch (((struct Gadget *)copy_msg.IAddress)->GadgetID)
					{
					// Next
					case GAD_PLAY_NEXT:
						return 1;

					// Abort
					case GAD_PLAY_ABORT:
						return 0;

					// Iconify
					case GAD_ID_ICONIFY:
						play_iconify(data);
						break;
					}
				}

				// Close window
				else if (copy_msg.Class == IDCMP_CLOSEWINDOW)
					return 0;

				// Window not valid any more?
				if (!data->window)
					break;
			}
		}

		// Wait for an event
		if ((Wait(((dt) ? 1 << data->dt_signal : 0) | ((data->ipc) ? (1 << data->ipc->command_port->mp_SigBit) : 0) |
				  ((data->app_icon) ? (1 << data->app_port->mp_SigBit) : 0) |
				  ((data->window) ? (1 << data->window->UserPort->mp_SigBit) : 0))) &
				(1 << data->dt_signal) &&
			dt)
			break;
	}

	// Return
	return 1;
}

// Iconify play requester
void play_iconify(play_data *data)
{
	struct TagItem tags[4];

	// Already iconified?
	if (data->app_icon)
		return;

	// No port?
	if (!data->app_port)
		return;

	// No icon yet?
	if (!data->icon && !(data->icon = GetDiskObject("DOpus5:Icons/play")) &&
		!(data->icon = GetDefDiskObject(WBPROJECT)))
		return;

	// Fill out tags
	tags[0].ti_Tag = DAE_SnapShot;
	tags[0].ti_Data = 1;
	tags[1].ti_Tag = DAE_Menu;
	tags[1].ti_Data = (IPTR)GetString(locale, MSG_PLAY_NEXT);
	tags[2].ti_Tag = DAE_Menu;
	tags[2].ti_Data = (IPTR)GetString(locale, MSG_PLAY_ABORT);
	tags[3].ti_Tag = TAG_END;

	// Set position
	data->icon->do_CurrentX = data->icon_x;
	data->icon->do_CurrentY = data->icon_y;

	// Try to add AppIcon
	if (!(data->app_icon = AddAppIconA(0, 0, data->play_name, data->app_port, 0, data->icon, tags)))
		return;

	// Save window position
	if (data->window)
		data->pos = *((struct IBox *)&data->window->LeftEdge);

	// Close window
	CloseConfigWindow(data->window);
	data->window = 0;
}

// Open play window
void play_open_window(play_data *data, struct Screen *screen, struct Window *parent)
{
	// Fill out new window
	data->new_win.dims = &data->dims;
	data->new_win.title = GetString(locale, MSG_PLAY_TITLE);
	data->new_win.locale = locale;
	data->new_win.flags = WINDOW_AUTO_KEYS | WINDOW_REQ_FILL | WINDOW_VISITOR | WINDOW_ICONIFY;
	data->new_win.port = 0;
	data->new_win.font = 0;

	// Saved position valid?
	if (data->pos.Width)
	{
		// Use this position
		data->dims.char_dim.Left = 0;
		data->dims.char_dim.Top = 0;
		data->dims.fine_dim.Left = data->pos.Left;
		data->dims.fine_dim.Top = data->pos.Top;
	}

	// Otherwise, get default size
	else
		data->dims = play_window;

	// Window as parent?
	if (parent)
		data->new_win.parent = (void *)parent;

	// Screen as parent
	else
	{
		data->new_win.parent = screen;
		data->new_win.flags |= WINDOW_SCREEN_PARENT;
	}

	// Open window
	if ((data->window = OpenConfigWindow(&data->new_win)))
	{
		// Add objects
		if (data->multiple)
			data->objlist = AddObjectList(data->window, play_objects);
		else
			data->objlist = AddObjectList(data->window, play_objects_single);

		if (data->objlist)
		{
			// Initialise fields
			SetGadgetValue(data->objlist, GAD_PLAY_FILENAME, (IPTR)data->play_name);
			SetGadgetValue(data->objlist, GAD_PLAY_LENGTH, (IPTR)data->play_length);
			SetGadgetValue(data->objlist, GAD_PLAY_TYPE, (IPTR)data->play_type);
			SetGadgetValue(data->objlist, GAD_PLAY_STATUS, (IPTR)data->play_status);
		}
	}
}

// Update info fields
void play_update_info(play_data *data, const char *filename, const char *length, const char *type, const char *status)
{
	// Change filename?
	if (filename != (char *)-1)
	{
		// Copy filename
		if (filename)
			stccpy(data->play_name, filename, 40);
		else
			*data->play_name = 0;

		// Update field if window open
		if (data->window)
			SetGadgetValue(data->objlist, GAD_PLAY_FILENAME, (IPTR)data->play_name);

		// Or, if AppIcon is open, change label
		else if (data->app_icon)
			ChangeAppIcon(data->app_icon, 0, 0, data->play_name, CAIF_TITLE);
	}

	// Change length?
	if (length != (char *)-1)
	{
		// Copy length
		if (length)
			stccpy(data->play_length, length, 40);
		else
			*data->play_length = 0;

		// Update field if window open
		if (data->window)
			SetGadgetValue(data->objlist, GAD_PLAY_LENGTH, (IPTR)data->play_length);
	}

	// Change type?
	if (type != (char *)-1)
	{
		// Copy type
		if (type)
			stccpy(data->play_type, type, 40);
		else
			*data->play_type = 0;

		// Update field if window open
		if (data->window)
			SetGadgetValue(data->objlist, GAD_PLAY_TYPE, (IPTR)data->play_type);
	}

	// Change status?
	if (status != (char *)-1)
	{
		// Copy status
		if (status)
			stccpy(data->play_status, status, 40);
		else
			*data->play_status = 0;

		// Update field if window open
		if (data->window)
			SetGadgetValue(data->objlist, GAD_PLAY_STATUS, (IPTR)data->play_status);
	}
}

// Come back from iconified
void play_reveal(play_data *data, struct Screen *scr)
{
	// Remove AppIcon
	if (data->app_icon)
	{
		RemoveAppIcon(data->app_icon);
		data->app_icon = 0;
	}

	// Window already open?
	if (data->window)
		return;

	// Open window
	play_open_window(data, scr, 0);
}

// Parse number out of string, leave pointer at one character after end of number
void read_parse_set(char **ptr, long *val)
{
	// Digit?
	if (isdigit(*(*ptr)))
	{
		*val = atoi(*ptr);
		while (*(*ptr) && isdigit(*(*ptr)))
			++*ptr;
		++*ptr;
	}
}

// Snapshot icon
void play_snapshot(play_data *data, struct AppSnapshotMsg *msg)
{
	// Unsnapshot?
	if (msg->flags & APPSNAPF_UNSNAPSHOT)
	{
		// Reinitialise position
		data->icon_x = NO_ICON_POSITION;
		data->icon_y = NO_ICON_POSITION;
	}

	// Store position
	else
	{
		data->icon_x = msg->position_x;
		data->icon_y = msg->position_y;
	}

	// Build environment variable
	lsprintf(data->buf, "%ld/%ld", data->icon_x, data->icon_y);

	// Set environment variable
	SetEnv(ENV_PLAY, data->buf, TRUE);
}
