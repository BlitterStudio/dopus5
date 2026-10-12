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

// List of builtin modules for OpenModule() function
const char *const dopus_modules[] = {"about.module",	"cleanup.module",	 "configopus.module",
									 "diskcopy.module", "diskinfo.module",	 "filetype.module",
									 "fixicons.module", "format.module",	 "ftp.module",
									 "icon.module",		"join.module",		 "listerformat.module",
									 "misc.module",		"pathformat.module", "play.module",
									 "print.module",	"read.module",		 "show.module",
									 "themes.module",	"xadopus.module",	 0};

// Get the name of the screen we are currently on
char *get_our_pubscreen()
{
	char *name;

	// Do we have our own screen?
	if (GUI->screen)
		return GUI->rexx_port_name;

	// If window is open
	if (GUI->window)
	{
		struct PubScreenNode *node;

		// Get pointer to public screen node
		if (!(node = FindPubScreen(GUI->screen_pointer, FALSE)))
		{
			// If not found, use default title
			name = GUI->screen_pointer->DefaultTitle;
		}

		// Get name from node
		else
			name = node->psn_Node.ln_Name;
	}

	// If no window open, use Workbench
	else
		name = "Workbench";

	return name;
}

// Case-insensitive strstr() function
char *strstri(const char *string, const char *substring)
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
			return (char *)string + a;
	}

	// No match
	return NULL;
}

// Get the next or previous node (handles wrap around)
struct Node *next_node(struct List *list, struct Node *node, int direction)
{
	// Move forwards?
	if (direction > 0)
	{
		if (node->ln_Succ->ln_Succ)
			node = node->ln_Succ;
		else
			node = list->lh_Head;
	}

	// Backwards
	else
	{
		if (node->ln_Pred->ln_Pred)
			node = node->ln_Pred;
		else
			node = list->lh_TailPred;
	}
	return node;
}

// Lock a list/lock
void lock_listlock(struct ListLock *list, BOOL exclusive)
{
	GetSemaphore(&list->lock, (exclusive) ? SEMF_EXCLUSIVE : SEMF_SHARED, 0);
}

// Unlock a list/lock
void unlock_listlock(struct ListLock *list)
{
	FreeSemaphore(&list->lock);
	/*
		// Check list integrity
		if (list->list.lh_TailPred->ln_Succ!=(struct Node *)&list->list.lh_Tail ||
			list->list.lh_Head->ln_Pred!=(struct Node *)&list->list.lh_Head)
		{
			D(bug("*** List integrity failure\n"));
		}
	*/
}

// Flush a message port
void flush_port(struct MsgPort *port)
{
	struct Message *msg;

	while ((msg = GetMsg(port)))
		ReplyFreeMsg(msg);
}

// Replace the beginning of a string
BOOL strreplace(char *string, char *old_part, char *new_part, BOOL path)
{
	short old_part_len;
	char *tempbuf = 0;

	// Get length of old part to replace
	old_part_len = strlen(old_part);

	// See if string matches old part at the start
	if (strnicmp(string, old_part, old_part_len) != 0)
		return 0;

	// Check path if necessary
	if (path && string[old_part_len] != 0 && string[old_part_len - 1] != '/' && string[old_part_len - 1] != ':')
		return 0;

	// See if string is longer than the old part
	if (strlen(string) > old_part_len)
	{
		// Allocate temporary buffer
		if ((tempbuf = AllocVec(strlen(string + old_part_len) + 1, 0)))
		{
			// Store remainder of string
			strcpy(tempbuf, string + old_part_len);
		}
	}

	// Copy new part into string buffer
	strcpy(string, new_part);

	// Any remainder?
	if (tempbuf)
	{
		// Add remainder and free buffer
		strcat(string, tempbuf);
		FreeVec(tempbuf);
	}

	return 1;
}

// Get a protection bit value as a string
void protect_get_string(ULONG protect, char *buffer)
{
	int a;
	char *key;

	// Get string of keys
	key = "hspa----";

	// Initialise buffer with default string
	strcpy(buffer, "----rwed");

	// Go through, if bits are set copy from second string into buffer
	for (a = 7; a >= 0; a--)
		if (protect & (1 << a))
			buffer[7 - a] = key[7 - a];
}

// Get protection value from a string
long prot_from_string(char *buf)
{
	short a;
	char ch;
	long protection;

	protection = FIBF_READ | FIBF_WRITE | FIBF_EXECUTE | FIBF_DELETE;
	for (a = 0; buf[a]; a++)
	{
		ch = tolower(buf[a]);
		if (ch == 'h')
			protection |= 1 << 7;
		else if (ch == 's')
			protection |= FIBF_SCRIPT;
		else if (ch == 'p')
			protection |= FIBF_PURE;
		else if (ch == 'a')
			protection |= FIBF_ARCHIVE;
		else if (ch == 'r')
			protection &= ~FIBF_READ;
		else if (ch == 'w')
			protection &= ~FIBF_WRITE;
		else if (ch == 'e')
			protection &= ~FIBF_EXECUTE;
		else if (ch == 'd')
			protection &= ~FIBF_DELETE;
	}

	return protection;
}

// Get network protection value from a string
long netprot_from_string(char *buf)
{
	long protection = 0;

	// Manual conversion
	if (buf[0] == 'r')
		protection |= FIBF_GRP_READ;
	if (buf[1] == 'w')
		protection |= FIBF_GRP_WRITE;
	if (buf[2] == 'e')
		protection |= FIBF_GRP_EXECUTE;
	if (buf[3] == 'd')
		protection |= FIBF_GRP_DELETE;
	if (buf[4] == 'r')
		protection |= FIBF_OTR_READ;
	if (buf[5] == 'w')
		protection |= FIBF_OTR_WRITE;
	if (buf[6] == 'e')
		protection |= FIBF_OTR_EXECUTE;
	if (buf[7] == 'd')
		protection |= FIBF_OTR_DELETE;

	return protection;
}

// Fill out a DOpusScreenData structure
void get_screen_data(DOpusScreenData *data)
{
	if (GUI->screen_pointer)
	{
		data->screen = GUI->screen_pointer;
		data->draw_info = GUI->draw_info;
		data->depth = GUI->screen_pointer->RastPort.BitMap->Depth;
		data->pen_alloc = GUI->pen_alloc;
		memcpy((char *)data->pen_array, (char *)GUI->pens, sizeof(data->pen_array));
		data->pen_count = environment->env->palette_count;
	}
	else
		data->screen = 0;
}

// Locale print-f
static IPTR ASM printf_hook(REG(a0, struct Hook *hook), REG(a2, APTR dummy), REG(a1, IPTR ch))
{
	char *ptr = (char *)hook->h_Data;
	(void)dummy;
	*ptr++ = (char)ch;
	hook->h_Data = (APTR)ptr;
	return 0;
}

#define LocaleBase locale.li_LocaleBase

void STDARGS SAVEDS v_loc_printf(char *buffer, char *string, APTR data)
{
	struct Hook hook;

// Fill out hook
#if defined(__AROS__) || defined(__MORPHOS__)
	hook.h_Entry = (HOOKFUNC)HookEntry;
	hook.h_SubEntry = (HOOKFUNC)printf_hook;
#else
	hook.h_Entry = (ULONG(*)())printf_hook;
#endif
	hook.h_Data = (APTR)buffer;
	FormatString(locale.li_Locale, string, data, &hook);
}

// Get disk info
LONG GetDiskInfo(char *device, struct InfoData *info)
{
	struct DevProc *proc;
	struct DosList *dos;
	LONG res = 0;

	// Get device proc
	if (!(proc = GetDeviceProc(device, 0)))
		return 0;

		// Send packet
#ifdef __AROS__
	if (((struct Library *)DOSBase)->lib_Version < 50)
	{
		// res=Info(proc->dvp_Lock,info);
		BPTR lock;
		if ((lock = Lock(device, SHARED_LOCK)))
		{
			res = Info(lock, info);
			UnLock(lock);
		}
	}
	else
#endif
		res = DoPkt(proc->dvp_Port, ACTION_DISK_INFO, (SIPTR)MKBADDR(info), 0, 0, 0, 0);

	// Clear "in use" flag to indicate formatting by default
	info->id_InUse = 0;

	// Get doslist pointer
	if ((dos = DeviceFromHandler(proc->dvp_Port, NULL)))
	{
		// Invalid device?
		if (!GetDeviceUnit(dos->dol_misc.dol_handler.dol_Startup, NULL, NULL))
		{
			// Disk can't be formatted
			info->id_InUse = 1;
		}
	}

	// Free device proc
	FreeDeviceProc(proc);
	return res;
}

// Check if a volume/path is available
BOOL VolumePresent(DirBuffer *buffer)
{
	D_S(struct InfoData, info)
	struct DosList *dos;
	char volume[32];

	// Valid path?
	if (!buffer->buf_Path[0])
		return 0;

	// Get current disk info
	if (!(GetDiskInfo(buffer->buf_Path, info)) || !(dos = (struct DosList *)BADDR(info->id_VolumeNode)))
		return 0;

	// Valid dos list?
	if (!dos || !dos->dol_Name)
		return 0;

	// Is volume mounted?
	if (info->id_DiskType == ID_NO_DISK_PRESENT || info->id_DiskType == ID_UNREADABLE_DISK)
		return 0;

	// Get current disk name
	if (dos->dol_Name)
		BtoCStr(dos->dol_Name, volume, 32);
	else
	{
		char *ptr;
		stccpy(volume, buffer->buf_ExpandedPath, 32);
		if ((ptr = strchr(volume, ':')))
			*ptr = 0;
	}

	// Compare disk name
	if (strcmp(volume, buffer->buf_VolumeLabel) != 0)
		return 0;

	// Compare date stamp
	if (CompareDates(&dos->dol_misc.dol_volume.dol_VolumeDate, &buffer->buf_VolumeDate) != 0)
		return 0;

	// Volume is available
	return 1;
}

// Copy a string and add ' for literal wildcards
void fix_literals(char *dest, char *srce)
{
	// Go through string
	while (*srce)
	{
		// Check for wildcard character
		if (*srce == '*' || *srce == '#' || *srce == '?' || *srce == '(' || *srce == ')' || *srce == '|' ||
			*srce == '~' || *srce == '[' || *srce == ']' || *srce == '\'')
		{
			// Add ' to make litera
			*dest++ = '\'';
		}

		// Copy character
		*dest++ = *srce++;
	}

	// Null-terminate
	*dest = 0;
}

// Error saving requester
short error_saving(short err, struct Window *window, long txt, long retry)
{
	char buf[80], error_text[140];

	// Build requester text
	Fault(err, "", buf, 80);
	lsprintf(error_text,
			 "%s\n%s %ld%s",
			 (IPTR)GetString(&locale, txt),
			 (IPTR)GetString(&locale, MSG_DOS_ERROR),
			 err,
			 (IPTR)buf);

	// Display requester
	return super_request_args(window,
							  error_text,
							  NULL,
							  (retry) ? GetString(&locale, MSG_RETRY) : GetString(&locale, MSG_OKAY),
							  (retry) ? GetString(&locale, MSG_CANCEL) : 0,
							  0);
}

// Check that a screen can close if Opus disappears
BOOL check_closescreen(struct Screen *screen)
{
	ULONG lock;
	struct Window *window = 0;
	TimerHandle *timer = 0;
	short a;

	// Try this four times
	for (a = 0; a < 4; a++)
	{
		// Lock IntuitionBase
		lock = LockIBase(0);

		// Go through window list
		for (window = screen->FirstWindow; window; window = window->NextWindow)
		{
			// Not a Workbench tool window?
			if (!(window->Flags & WFLG_WBENCHWINDOW))
			{
				// If this is one of Opus', GetWindowID() should know about it
				if (GetWindowID(window) == WINDOW_UNKNOWN)
				{
					BOOL ok = 0;
					struct Task *task = 0;

					// Special case for IPrefs requester - get window's task
					if (window->UserPort)
						task = (struct Task *)window->UserPort->mp_SigTask;

					// Valid name?
					if (task && task->tc_Node.ln_Name)
					{
						// Owned by IPrefs?
						if (strcmp(task->tc_Node.ln_Name, "� IPrefs �") == 0)
							ok = 1;
					}

					// If not ok, fail
					if (!ok)
						break;
				}
			}
		}

		// Unlock IntuitionBase
		UnlockIBase(lock);

		// Ok to shut?
		if (!window)
			break;

		// Allocate timer if don't already have it
		if (!timer && !(timer = AllocTimer(UNIT_VBLANK, 0)))
			break;

		// Start timer and wait for it
		StartTimer(timer, 0, 250000);
		while (!CheckTimer(timer))
			WaitPort(timer->port);
	}

	// Free timer
	FreeTimer(timer);

	// If window==NULL we can shut
	return (BOOL)(!window);
}

// See if qualifier matches
BOOL check_qualifier(UWORD qual, UWORD qual_match, UWORD qual_mask, UWORD qual_same)
{
	UWORD key_qual, match_qual;

	// Get qualifiers to match
	match_qual = qual_match & qual_mask;
	key_qual = qual & qual_mask;

	// Treat shift as the same?
	if (qual_same & (IXSYM_SHIFT | IXSYM_CAPS))
	{
		// If either shift is on, turn them both on
		if (match_qual & IEQUAL_ANYSHIFT)
			match_qual |= IEQUAL_ANYSHIFT;
		if (key_qual & IEQUAL_ANYSHIFT)
			key_qual |= IEQUAL_ANYSHIFT;
	}

	// Treat alt as the same?
	if (qual_same & IXSYM_ALT)
	{
		// If either alt is on, turn them both on
		if (match_qual & IEQUAL_ANYALT)
			match_qual |= IEQUAL_ANYALT;
		if (key_qual & IEQUAL_ANYALT)
			key_qual |= IEQUAL_ANYALT;
	}

	// Does qualifier match?
	return (BOOL)(match_qual == key_qual);
}

iconopen_packet *get_icon_packet(BackdropInfo *info, BackdropObject *object, char *data, UWORD qual)
{
	iconopen_packet *packet;

	// Get packet
	if ((packet = AllocVec(sizeof(iconopen_packet) + (data ? strlen(data) : 0), MEMF_CLEAR)))
	{
		packet->backdrop = info;
		packet->object = object;
		packet->qual = qual;
		if (data)
			strcpy(packet->data, data);
	}

	return packet;
}

// Check for refresh message
struct IntuiMessage *check_refresh_msg(struct Window *window, ULONG mask)
{
	struct IntuiMessage *msg;

	// Valid window?
	if (!window)
		return 0;

	// Go through messages
	for (msg = (struct IntuiMessage *)window->UserPort->mp_MsgList.lh_Head; msg->ExecMessage.mn_Node.ln_Succ;
		 msg = (struct IntuiMessage *)msg->ExecMessage.mn_Node.ln_Succ)
	{
		// Refresh?
		if (msg->Class & mask)
		{
			// Remove it
			Forbid();
			Remove((struct Node *)msg);
			Permit();

			// Return the message
			return msg;
		}
	}

	return 0;
}

// Find window from coordinates
struct Window *WhichWindow(struct Screen *screen, short x, short y, unsigned short flags)
{
	struct Layer *layer;
	struct Window *window = 0;

	// Lock screen layerinfo
	if (!(flags & WWF_NO_LOCK))
		LockLayerInfo(&screen->LayerInfo);

	// Find layer
	if ((layer = WhichLayer(&screen->LayerInfo, x, y)))
	{
		// Get window pointer
		window = layer->Window;
	}

	// Unlock if necessary
	if (!(flags & WWF_NO_LOCK) && (!window || !(flags & WWF_LEAVE_LOCKED)))
		UnlockLayerInfo(&screen->LayerInfo);

	return window;
}

// See if a path is just a device
BOOL isdevice(char *path)
{
	if (path[strlen(path) - 1] == ':')
		return 1;
	return 0;
}

// Get final path from a pathname
void final_path(char *path, char *buf)
{
	char *ptr, *end_ptr;
	short len;

	// Empty or null string?
	if (!path || !*path)
	{
		*buf = 0;
		return;
	}

	// Get end of string
	end_ptr = path + strlen(path) - 1;

	// Go backwards
	for (ptr = end_ptr; ptr > path; --ptr)
	{
		// Directory separator?
		if (*ptr == '/')
		{
			// Last character?
			if (*(ptr + 1) == 0)
			{
				// Save new end pointer
				end_ptr = ptr - 1;
				continue;
			}

			// Break here
			break;
		}

		// Device separator?
		else if (*ptr == ':')
		{
			// If last character, go to start of string
			if (*(ptr + 1) == 0)
				ptr = path;

			// Break here
			break;
		}
	}

	// If not at the start of the string, skip forward one
	if (ptr > path)
		++ptr;

	// Copy into buffer
	len = end_ptr - ptr + 1;
	strncpy(buf, ptr, len);
	buf[len] = 0;
}

// Get pointer to function label
char *function_label(Cfg_ButtonFunction *func)
{
	// Valid function?
	if (!func)
		return 0;

	// Got name?
	if (func->node.ln_Name && *func->node.ln_Name)
		return func->node.ln_Name;

	// Got label?
	if (func->label && *func->label)
		return func->label;

	// No label
	return 0;
}

// Build version string
void build_version_string(char *buf, short ver, short rev, long days, short format)
{
	char datebuf[20];

	// Build version string
	lsprintf(buf, "%ld.%ld", ver, rev);

	// Got date?
	if (days)
	{
		// Get date string
		datebuf[0] = ' ';
		date_string(days, datebuf + 1, format, TRUE);

		// Add string
		strcat(buf, datebuf);
	}
}

// Trap 'more' in default tool
BOOL file_trap_more(char *name, char *tool)
{
	short len;
	BOOL ok = 0;

	if (!(environment->env->settings.icon_flags & ICONFLAG_TRAP_MORE))
		return 0;

	// Look for 'more' in default tool
	len = strlen(tool);
	if (len >= 4 && stricmp(tool + len - 4, "more") == 0 && (len == 4 || tool[len - 5] == '/' || tool[len - 5] == ':'))
	{
		struct read_startup *read;

		// Allocate startup
		if ((read = AllocVec(sizeof(struct read_startup), MEMF_CLEAR)))
		{
			read->wheel_scroll_lines = environment->env->env_wheel_scroll_lines;

			// Create list
			if ((read->files = (struct List *)Att_NewList(0)))
			{
				// Add file to list
				if (Att_NewNode((Att_List *)read->files, name, 0, 0))
				{
					// Substitute internal viewer
					misc_startup("dopus_more_trap", FUNC_SMARTREAD, 0, read, FALSE);
					ok = 1;
				}
				else
					Att_RemList((Att_List *)read->files, 0);
			}
			if (!ok)
				FreeVec(read);
		}
	}

	return ok;
}

// See if file has a suffix
char *sufcmp(char *name, char *suffix)
{
	short a, len;

	// Length of suffix
	len = strlen(suffix);

	// Is string too short?
	if ((a = strlen(name)) < len)
		return 0;

	// Does it have the suffix?
	if (stricmp(name + a - len, suffix) == 0)
		return name + a - len;

	// No
	return 0;
}

// Open a module
struct Library *OpenModule(char *name)
{
	struct Library *lib = NULL;
	char buf[256];
	short a, ver = 0, known_module = 0;

	// See if this is one of our modules
	for (a = 0; dopus_modules[a]; a++)
		if (stricmp(name, dopus_modules[a]) == 0)
		{
			// Need newest version
			ver = LIB_VERSION;
			known_module = 1;
			break;
		}
	// Prefer an already-resident module instance, so modules with global
	// state do not silently split between path aliases.
	if (!known_module || !(lib = OpenLibrary(name, ver)))
	{
		// Build name in program modules directory
		strcpy(buf, "PROGDIR:Modules/");
		strcat(buf, name);

		// Open library
		if (!(lib = OpenLibrary(buf, ver)))
		{
			// Fall back to the DOpus5: assign
			strcpy(buf, "DOpus5:Modules/");
			strcat(buf, name);
			lib = OpenLibrary(buf, ver);
		}
	}

#ifdef __AROS__
	if (!lib && known_module && ver && stricmp(name, "ftp.module") == 0)
	{
		// AROS can fail the versioned open for ftp.module even though the module is usable.
		// The FTP worker keeps itself resident with an unversioned open, so mirror that here.
		if (!(lib = OpenLibrary(name, 0)))
		{
			strcpy(buf, "PROGDIR:Modules/");
			strcat(buf, name);

			if (!(lib = OpenLibrary(buf, 0)))
			{
				strcpy(buf, "DOpus5:Modules/");
				strcat(buf, name);
				lib = OpenLibrary(buf, 0);
			}
		}
	}
#endif

	if (!lib)
	{
		// Show error
		error_request(GUI->window, 1, GetString(&locale, MSG_OPENING), -1, name, 0);
	}

	return lib;
}

// Chain tags
void ChainTagItems(struct TagItem **list_ptr, struct TagItem *tags)
{
	struct TagItem *list;

	// Invalid list?
	if (!list_ptr || !tags)
		return;

	// Get list pointer
	if (!(list = *list_ptr))
	{
		// No list, so install new one
		*list_ptr = tags;
		return;
	}

	// Go through list until we find a TAG_END
	while (list->ti_Tag != TAG_END)
	{
		// More tags?
		if (list->ti_Tag == TAG_MORE)
		{
			// Get new pointer
			list = (struct TagItem *)list->ti_Data;
		}

		// Advance this pointer
		else
			++list;
	}

	// Change to TAG_MORE
	list->ti_Tag = TAG_MORE;
	list->ti_Data = (IPTR)tags;
}

// Handle a diskchange message
void handle_diskchange(DOpusNotify *notify)
{
	// Bad disk?
	if (notify->dn_Flags == (ULONG)-1)
	{
		// Launch "bad" script
		RunScript(SCRIPT_BAD_DISKINSERT, notify->dn_Name);
		return;
	}
	else if (notify->dn_Flags == (ULONG)-2)
		return;

	// Launch insertion script
	RunScript((notify->dn_Flags == 0) ? SCRIPT_DISKREMOVE : SCRIPT_DISKINSERT, notify->dn_Name);
	/*
		short unit;
		char buf[10];
		BPTR lock;

		// Go through units
		for (unit=0;unit<4;unit++)
		{
			// Did this unit have a disk change?
			if (notify->dn_Data&(1<<unit))
			{
				short valid=0;

				// Try DFx: first
				lsprintf(buf,"DF%ld:",unit);

				// Disk in drive?
				if (notify->dn_Flags&(1<<unit))
				{
					D_S(struct InfoData, info)
					struct DevProc *proc;
					long res;

					// Get process
					if (proc=GetDeviceProc(buf,0))
					{
						// Send packet to get info
						res=DoPkt(proc->dvp_Port,ACTION_DISK_INFO,(SIPTR)MKBADDR(info),0,0,0,0);
						FreeDeviceProc(proc);
					}

					// Is disk busy?
					if (res && info->id_DiskType==ID_BUSY)
						continue;

					// Can we lock it?
					if (!(lock=Lock(buf,ACCESS_READ)))
					{
						// Ok, try PCx: now
						buf[0]='P';
						buf[1]='C';

						// Try to lock it
						lock=Lock(buf,ACCESS_READ);
					}

					// Got a lock?
					if (lock)
					{
						UnLock(lock);
						valid=2;
					}
				}

				// If disk was removed, always trigger script
				else valid=1;

				// Ok to run script?
				if (valid)
				{
					// Launch insertion script
					RunScript((valid==1)?SCRIPT_DISKREMOVE:SCRIPT_DISKINSERT,buf);
				}

				// Invalid
				else
				{
					// Use DFx:
					buf[0]='D';
					buf[1]='F';

					// Launch "bad" script
					RunScript(SCRIPT_BAD_DISKINSERT,buf);
				}
			}
		}
	*/
}

// Send reset command to main process
void send_main_reset_cmd(ULONG flags1, ULONG flags2, APTR data)
{
	ULONG *flags;

	// Allocate flags
	if ((flags = AllocVec(sizeof(ULONG) * 2, 0)))
	{
		// Fill out flags
		flags[0] = flags1;
		flags[1] = flags2;

		// Send command
		IPC_Command(&main_ipc, MAINCMD_RESET, 0, data, flags, 0);
	}
}

// Get file information
BOOL GetFileInfo(char *name, struct FileInfoBlock *fib)
{
	BPTR lock;

	if (!(lock = Lock(name, ACCESS_READ)))
		return 0;
	Examine(lock, fib);
	UnLock(lock);
	return 1;
}

#ifdef __amigaos4__
// Open library and get interface for OS4
BOOL OpenLibIFace(char *name, APTR *libBase, APTR *iface, short version)
{
	if ((*libBase = OpenLibrary(name, version)))
	{
		if (!(*iface = GetInterface(*libBase, "main", 1, NULL)))
		{
			CloseLibrary(*libBase);
			*libBase = NULL;
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}
#endif
