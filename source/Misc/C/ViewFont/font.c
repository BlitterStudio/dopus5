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

#include "font.h"

#define FONT_SETTINGF_EXTENDED (1 << 0)

static void font_build_text(font_data *data);
static void font_append_text(font_data *data, short *pos, short first, short last);

#ifdef __AROS__
static IPTR font_refresh_entry(struct Hook *, APTR, APTR);
#endif

int main(int argc, char **argv)
{
	font_data *data;

	// Need dopus library
	if (!(DOpusBase = OpenLibrary("dopus5.library", LIB_VERSION)))
		if (!(DOpusBase = OpenLibrary("DOpus5:Libs/dopus5.library", 55)))
			return (10);
#ifdef __amigaos4__
	if (!(IDOpus = (struct DOpusIFace *)GetInterface(DOpusBase, "main", 1, NULL)))
	{
		CloseLibrary(DOpusBase);
		return (10);
	}
#endif

	// Allocate data
	if (!(data = AllocVec(sizeof(font_data), MEMF_CLEAR)))
	{
		font_free(0);
		return (5);
	}

	// Parse arguments
	data->args = ReadArgs("FONT,SIZE/N,B=BOLD/S,I=ITALIC/S,U=ULINE/S,PUBSCREEN/K", (APTR)data->arg_array, NULL);

	// Default to topaz 8
#ifdef __AROS__
	strcpy(data->font_name, "ttcourier.font");
#else
	strcpy(data->font_name, "topaz.font");
#endif
	data->font_size = 8;

	// Got font name?
	if (data->arg_array[ARG_FONT])
		font_get_name(data, (char *)data->arg_array[ARG_FONT]);

	// Got a font size?
	if (data->arg_array[ARG_SIZE] && *((LONG *)data->arg_array[ARG_SIZE]))
	{
		data->font_size = *((LONG *)data->arg_array[ARG_SIZE]);
		data->first = TRUE;
	}

	// Initialise locale
	init_locale_data(&data->locale);

	// Got locale library?
	if (LocaleBase)
	{
		BPTR lock;

		// Change PROGDIR: to DOpus5:, save old PROGDIR for restore on exit
		if ((lock = Lock("DOpus5:", ACCESS_READ)))
			data->lock = SetProgramDir(lock);

		// Initialise
		data->locale.li_LocaleBase = LocaleBase;
		data->locale.li_Catalog = OpenCatalogA(0, "viewfont.catalog", 0);
		data->locale.li_Locale = OpenLocale(0);
	}

	// Create message port
	data->appport = CreateMsgPort();

	// Open window
	if (!(font_open(data)))
	{
		font_free(data);
		return (5);
	}

	// Initial gadget settings
	if (data->arg_array[ARG_BOLD])
		SetGadgetValue(data->list, GAD_FONT_BOLD, 1);
	if (data->arg_array[ARG_ITALIC])
		SetGadgetValue(data->list, GAD_FONT_ITALIC, 1);
	if (data->arg_array[ARG_ULINE])
		SetGadgetValue(data->list, GAD_FONT_ULINE, 1);

	// Show font
	font_show_font(data, FALSE);

	// Initialise refresh hook
#ifdef __AROS__
	data->refresh_hook.h_Entry = (HOOKFUNC)font_refresh_entry;
#else
	data->refresh_hook.h_Entry = (ULONG(*)())font_refresh;
#endif
	data->refresh_hook.h_Data = data;

	// Event loop
	FOREVER
	{
		struct IntuiMessage *msg;
		BOOL quit_flag = FALSE;

		// AppWindow?
		if (data->appwindow)
		{
			struct AppMessage *amsg;

			// Get messages
			while ((amsg = (struct AppMessage *)GetMsg(data->appport)))
			{
				// Got file?
				if (amsg->am_NumArgs > 0)
				{
					char buf[256];

					// Get name
					DevNameFromLockDopus(amsg->am_ArgList[0].wa_Lock, buf, 256);
					if (amsg->am_ArgList[0].wa_Name && *amsg->am_ArgList[0].wa_Name)
						AddPart(buf, amsg->am_ArgList[0].wa_Name, 256);

					// Get font name
					font_get_name(data, buf);

					// Get new font
					font_get_font(data);
					font_show_font(data, FALSE);
				}

				// Reply to message
				ReplyMsg((struct Message *)amsg);
			}
		}

		// Requester?
		if (data->about)
		{
			// Handle requester
			if (SysReqHandler(data->about, 0, 0) >= 0)
			{
				// Close requester
				FreeSysRequest(data->about);
				data->about = NULL;
			}
		}

		// Intuition messages
		if (data->window)
		{
			while ((msg = GetWindowMsg(data->window->UserPort)))
			{
				struct IntuiMessage msg_copy;
				UWORD id = 0;

				// Copy message and reply
				msg_copy = *msg;
				ReplyWindowMsg(msg);

				// Get gadget ID
				if (msg_copy.Class == IDCMP_GADGETUP)
					id = ((struct Gadget *)msg_copy.IAddress)->GadgetID;

				// Look at message
				switch (msg_copy.Class)
				{
				// Close window
				case IDCMP_CLOSEWINDOW:
					quit_flag = TRUE;
					break;

				// New size
				case IDCMP_NEWSIZE:

					// Redraw font
					font_show_font(data, FALSE);

					// Set flag to say we resized
					data->resized = TRUE;
					break;

				// Menu
				case IDCMP_MENUPICK: {
					struct MenuItem *item;

					// Get item
					if (!(item = ItemAddress(data->window->MenuStrip, msg_copy.Code)))
						break;

					// Get ID
					id = (UWORD)(IPTR)GTMENUITEM_USERDATA(item);

					// Fall through
				}

				// Gadget
				case IDCMP_GADGETUP:
					switch (id)
					{
					// Show requester
					case GAD_FONT_FONT_POPUP:
					case MENU_OPEN_FONT:

						// Ask for name
						font_ask_name(data);
						break;

					// Font name typed
					case GAD_FONT_FONT:

						// Get name
						font_get_name(data, (char *)GetGadgetValue(data->list, GAD_FONT_FONT));

						// Get new font
						font_get_font(data);
						font_show_font(data, FALSE);
						break;

					// Font size up/down
					case GAD_FONT_UP:
					case GAD_FONT_DOWN: {
						short size;

						// Get next size
						size = font_get_size(data, (id == GAD_FONT_UP) ? 1 : -1);

						// No change?
						if (size == data->font_size)
							break;

						// Use this size
						data->font_size = size;
						SetGadgetValue(data->list, GAD_FONT_SIZE, size);
					}

					// Font size given
					case GAD_FONT_CYCLE:

						// Check id
						if (id == GAD_FONT_CYCLE)
						{
							short size;

							// Get size
							if (!(size = atoi(data->size_labels[msg_copy.Code])))
								break;

							// Refresh gadget
							data->font_size = size;
							SetGadgetValue(data->list, GAD_FONT_SIZE, data->font_size);
						}

					// Font size
					case GAD_FONT_SIZE:

						// Entered size
						if (id == GAD_FONT_SIZE)
						{
							// Bounds check gadget
							data->font_size = BoundsCheckGadget(data->list, GAD_FONT_SIZE, 4, 255);
						}

						// Get new font
						font_get_font(data);
						font_show_font(data, FALSE);
						break;

					// Styles changed
					case GAD_FONT_BOLD:
					case GAD_FONT_ITALIC:
					case GAD_FONT_ULINE:

						// Redraw font
						font_show_font(data, FALSE);
						break;

					// Extended characters changed
					case GAD_FONT_EXTENDED:

						// Rebuild preview and redraw font
						data->extended = (BOOL)GetGadgetValue(data->list, GAD_FONT_EXTENDED);
						font_build_text(data);
						font_show_font(data, FALSE);
						break;

					// Save settings
					case MENU_SAVE_SETTINGS:
						font_save_settings(data);
						break;

					// Quit
					case MENU_QUIT:
						quit_flag = TRUE;
						break;

					// About
					case MENU_ABOUT:
						font_show_about(data);
						break;
					}
					break;

				// Rawkey
				case IDCMP_RAWKEY:

					// Help?
					if (msg_copy.Code == 0x5f)
						font_show_about(data);

				// Key
				case IDCMP_VANILLAKEY:

					// Escape does quit
					if (msg_copy.Code == 0x1b)
						quit_flag = TRUE;
					break;

				// Refresh
				case IDCMP_REFRESHWINDOW:

					// Have we just resized?
					if (data->resized)
					{
						// Don't need to refresh
						BeginRefresh(data->window);
						EndRefresh(data->window, TRUE);
						data->resized = FALSE;
						break;
					}

					// Refresh font display
					font_show_font(data, TRUE);
					break;
				}

				// Check window is still valid
				if (!data->window)
					break;
			}
		}

		if (quit_flag)
			break;

		// Wait for signal (ctrl-c breaks)
		if ((Wait(SIGBREAKF_CTRL_C | ((data->window) ? (1 << data->window->UserPort->mp_SigBit) : 0) |
				  ((data->about) ? (1 << data->about->UserPort->mp_SigBit) : 0) |
				  ((data->appwindow) ? (1 << data->appport->mp_SigBit) : 0))) &
			SIGBREAKF_CTRL_C)
			break;
	}

	// Free stuff
	font_free(data);
	return (0);
}

// Open font window
BOOL font_open(font_data *data)
{
	struct Screen *screen = NULL;
	short min_width, min_height;

	// Screen supplied?
	if (data->arg_array[ARG_SCREEN])
		screen = LockPubScreen((char *)data->arg_array[ARG_SCREEN]);

	// Fill out new window
	data->new_win.parent = screen;
	data->new_win.dims = &data->win_dims;
	data->new_win.title = GetString(&data->locale, MSG_FONT_TITLE);
	data->new_win.locale = &data->locale;
	data->new_win.flags = WINDOW_SCREEN_PARENT | WINDOW_VISITOR | WINDOW_AUTO_KEYS | WINDOW_SIZE_BOTTOM | WINDOW_SIMPLE;

	// Default dimensions
	data->win_dims = font_window;

	// Read settings
	font_read_settings(data);

	// Open window
	if ((data->window = OpenConfigWindow(&data->new_win)))
		data->list = AddObjectList(data->window, font_objects);

	// Unlock screen
	UnlockPubScreen(0, screen);

	// Failed to open?
	if (!data->list)
		return FALSE;

	// Restore saved settings
	SetGadgetValue(data->list, GAD_FONT_EXTENDED, data->extended);

	// Fix sizing limits
	min_width = (font_window.char_dim.Width * data->window->RPort->TxWidth) + font_window.fine_dim.Width;
	min_height = (font_window.char_dim.Height * data->window->RPort->TxHeight) + font_window.fine_dim.Height;
	WindowLimits(data->window, min_width, min_height, ~0, ~0);

	// Expand older saved settings that are now below the minimum
	if (data->window->Width < min_width || data->window->Height < min_height)
	{
		ChangeWindowBox(data->window,
						data->window->LeftEdge,
						data->window->TopEdge,
						(data->window->Width < min_width) ? min_width : data->window->Width,
						(data->window->Height < min_height) ? min_height : data->window->Height);
		LayoutResize(data->window);
	}

	// Add menus
	AddWindowMenus(data->window, font_menus);

	// Add AppWindow
	if (data->appport)
		data->appwindow = AddAppWindowA(0, 0, data->window, data->appport, 0);

	// Get the font
	font_get_font(data);
	return TRUE;
}

// Close font display
void font_close(font_data *data)
{
	// Window open?
	if (data->window)
	{
		// Remove AppWindow
		RemoveAppWindow(data->appwindow);
		data->appwindow = NULL;

		// Close requester
		FreeSysRequest(data->about);
		data->about = NULL;

		// Close window
		CloseConfigWindow(data->window);
		data->window = NULL;
	}
}

// Free font data
void font_free(font_data *data)
{
	if (data)
	{
		// Close stuff
		font_close(data);

		// Free font
		if (data->font)
			CloseFont(data->font);

		// Free port
		DeleteMsgPort(data->appport);

		// Close locale stuff
		if (data->locale.li_Locale)
		{
			CloseLocale(data->locale.li_Locale);
			CloseCatalog(data->locale.li_Catalog);
		}

		// Restore PROGDIR: and unlock our DOpus5: lock
		if (data->lock)
			UnLock(SetProgramDir(data->lock));

		// Free args
		FreeArgs(data->args);

		// Free labels
		FreeVec(data->size_labels);

		// Free data
		FreeVec(data);
	}

	// Close library
#ifdef __amigaos4__
	DropInterface((struct Interface *)IDOpus);
#endif
	CloseLibrary(DOpusBase);
}

// Get font
void font_get_font(font_data *data)
{
	// Make window busy
	SetWindowBusy(data->window);

	// Existing font?
	if (data->font)
	{
		// Free font
		CloseFont(data->font);
		data->font = NULL;
	}

	// Has the font name changed?
	if (stricmp(data->font_name, data->last_font_name))
	{
		struct FontContentsHeader *fch;
		BPTR lock;

		// Empty labels
		font_build_labels(data, 0);

		// Is the name a full path?
		if (strchr(data->font_name, '/') || strchr(data->font_name, ':'))
		{
			// Lock font
			if ((lock = Lock(data->font_name, ACCESS_READ)))
			{
				BPTR parent;

				// Get lock on parent
				parent = ParentDir(lock);

				// Unlock lock on file
				UnLock(lock);
				lock = parent;
			}
		}

		// Lock fonts: directory
		else
			lock = Lock("fonts:", ACCESS_READ);

		// Got lock?
		if (lock)
		{
			// Get font contents
			if ((fch = NewFontContents(lock, FilePart(data->font_name))))
			{
				// Build labels
				font_build_labels(data, fch);

				// Free contents
				DisposeFontContents(fch);
			}

			// Unlock drawer
			UnLock(lock);
		}
	}

	// Fill out TextAttr
	data->attr.ta_Name = data->font_name;
	data->attr.ta_YSize = data->font_size;
	data->attr.ta_Style = 0;
	data->attr.ta_Flags = 0;

	// Open font
	data->font = OpenDiskFont(&data->attr);

	// Show font name and size
	SetGadgetValue(data->list, GAD_FONT_FONT, (IPTR)FilePart(data->font_name));
	SetGadgetValue(data->list, GAD_FONT_SIZE, data->font_size);

	// Got font?
	if (data->font)
	{
		short pos;

		// Build display text
		font_build_text(data);

		// Got labels?
		if (data->size_labels)
		{
			// Find label corresponding to this size
			for (pos = 0; data->size_labels[pos]; pos++)
			{
				if (atoi(data->size_labels[pos]) == data->font_size)
				{
					// Set cycle gadget
					SetGadgetValue(data->list, GAD_FONT_CYCLE, pos);
					break;
				}
			}
		}
	}

	// Empty labels
	else
		font_build_labels(data, 0);

	// Copy name
	strcpy(data->last_font_name, data->font_name);

	// Clear 'first' flag
	data->first = FALSE;

	// Clear window busy
	ClearWindowBusy(data->window);
}

// Build display text
static void font_build_text(font_data *data)
{
	short pos = 0, ch, hi;

	data->font_text[0] = 0;

	// Got font?
	if (!data->font)
		return;

	// Extended range?
	if (data->extended)
	{
		// Keep the normal printable ASCII sample, and add high printable characters
		font_append_text(data, &pos, 33, 126);
		font_append_text(data, &pos, 160, 255);
	}

	// Normal range
	else
	{
		// First character
		ch = data->font->tf_LoChar;
		if (ch < 33)
			ch = 33;

		// Hi character
		hi = data->font->tf_HiChar;
		if (hi > 126 && ch < 127)
			hi = 127;

		// Add existing preview range
		font_append_text(data, &pos, ch, hi - 1);
	}

	data->font_text[pos] = 0;
}

// Append a character range to the display text
static void font_append_text(font_data *data, short *pos, short first, short last)
{
	short ch;

	// Clamp to font range
	if (first < data->font->tf_LoChar)
		first = data->font->tf_LoChar;
	if (last > data->font->tf_HiChar)
		last = data->font->tf_HiChar;
	if (last > 255)
		last = 255;

	// Valid range?
	if (first > last)
		return;

	// Append characters
	for (ch = first; ch <= last && *pos < (short)sizeof(data->font_text) - 1; ch++, (*pos)++)
		data->font_text[*pos] = ch;
}

// Show font example
void font_show_font(font_data *data, BOOL refresh)
{
	struct Rectangle rect;
	struct Region *region;
	struct RastPort rp;

	// Get display rectangle
	GetObjectRect(data->list, GAD_FONT_DISPLAY, &rect);

	// Move rectangle in
	rect.MinX += 3;
	rect.MinY += 3;
	rect.MaxX -= 3;
	rect.MaxY -= 3;

	// Copy rastport
	rp = *data->window->RPort;

	// Refresh?
	if (refresh)
	{
		LockLayerInfo(&data->window->WScreen->LayerInfo);
		BeginRefresh(data->window);
	}

	// Clear background
	SetAPen(&rp, DRAWINFO(data->window)->dri_Pens[SHINEPEN]);
	RectFill(&rp, rect.MinX - 1, rect.MinY - 1, rect.MaxX + 1, rect.MaxY + 1);

	// Refreshing?
	if (refresh)
		EndRefresh(data->window, FALSE);

	// Create region
	if ((region = NewRegion()))
	{
		// Set rectangle
		OrRectRegion(region, &rect);

		// Install region
		InstallClipRegion(data->window->WLayer, region);
	}

	// Refreshing?
	if (refresh)
		BeginRefresh(data->window);

	// Got a font?
	if (data->font)
	{
		ULONG flags;
		short y;
		struct TextExtent extent;
		char *ptr, *end;

		// Set pen and font
		SetAPen(&rp, DRAWINFO(data->window)->dri_Pens[TEXTPEN]);
		SetDrMd(&rp, JAM1);
		SetFont(&rp, data->font);

		// Get style flags
		flags = 0;
		if (GetGadgetValue(data->list, GAD_FONT_BOLD))
			flags |= FSF_BOLD;
		if (GetGadgetValue(data->list, GAD_FONT_ITALIC))
			flags |= FSF_ITALIC;
		if (GetGadgetValue(data->list, GAD_FONT_ULINE))
			flags |= FSF_UNDERLINED;

		// Set styles
		SetSoftStyle(&rp, flags, FSF_BOLD | FSF_ITALIC | FSF_UNDERLINED);

		// Valid font to draw?
		if (data->font_text[0])
		{
			// Get end of the string
			end = data->font_text + strlen(data->font_text);

			// Initial coordinates
			y = rect.MinY;

			// Initialise position
			if (!(ptr = strchr(data->font_text, 'A')))
				ptr = data->font_text;
			Move(&rp, rect.MinX, y + rp.TxBaseline);

			// Draw until we reach the bottom
			while (y < rect.MaxY)
			{
				// New line
				if (rp.cp_x > rect.MaxX)
				{
					// Bump position
					y += rp.TxHeight + 1;
					Move(&rp, rect.MinX, y + rp.TxBaseline);
				}

				// Otherwise
				else
				{
					short len, maxlen;

					// Get text that will fit
					len =
						TextFit(&rp, ptr, (maxlen = strlen(ptr)), &extent, 0, 1, rect.MaxX - rp.cp_x + 1, rp.TxHeight);

					// Check against length, add extra character if ok
					if (len < maxlen)
						++len;

					// Draw text
					Text(&rp, ptr, len);

					// Bump text position
					ptr += len;

					// End of the string?
					if (ptr >= end)
						ptr = data->font_text;
				}
			}
		}
	}

	// Finished refreshing?
	if (refresh)
		EndRefresh(data->window, TRUE);

	// Remove region
	if (region)
	{
		InstallClipRegion(data->window->WLayer, 0);
		DisposeRegion(region);
	}

	// Unlock layers if we refreshed
	if (refresh)
		UnlockLayerInfo(&data->window->WScreen->LayerInfo);
}

// Get font name from string
void font_get_name(font_data *data, char *name)
{
	short len;

	// .font suffix?
	if ((len = strlen(name)) > 5 && stricmp(name + len - 5, ".font") == 0)
	{
		// Use name as given
		strcpy(data->font_name, name);
	}

	// Numeric name?
	else if ((len = atoi(FilePart(name))))
	{
		// Strip filename
		*(PathPart(name)) = 0;

		// Store name
		strcpy(data->font_name, name);
		strcat(data->font_name, ".font");

		// Store size
		data->font_size = len;
		data->first = TRUE;
	}

	// Name specified without .font
	else
	{
		// Store name
		strcpy(data->font_name, name);
		strcat(data->font_name, ".font");
	}
}

// Ask for font name
void font_ask_name(font_data *data)
{
	char file[40], path[256], *fileptr;

	// Copy path
	strcpy(path, data->font_name);

	// Split filename
	if ((fileptr = FilePart(path)))
	{
		strcpy(file, fileptr);
		*fileptr = 0;
	}
	else
		file[0] = 0;

	// Empty path?
	if (!path[0])
		strcpy(path, "fonts:");

	// Busy window
	SetWindowBusy(data->window);

	// Display requester
	if (AslRequestTags(((WindowData *)data->window->UserData)->request,
					   ASLFR_Window,
					   data->window,
					   ASLFR_TitleText,
					   GetString(&data->locale, MSG_FONT_SELECT),
					   ASLFR_InitialFile,
					   file,
					   ASLFR_InitialDrawer,
					   path,
					   ASLFR_IntuiMsgFunc,
					   &data->refresh_hook,
					   TAG_END))
	{
		// Get pathname
		strcpy(path, ((WindowData *)data->window->UserData)->request->fr_Drawer);
		AddPart(path, ((WindowData *)data->window->UserData)->request->fr_File, 256);

		// Get font name
		font_get_name(data, path);

		// Get new font
		font_get_font(data);
		font_show_font(data, FALSE);
	}

	// Unbusy window
	else
		ClearWindowBusy(data->window);
}

// Refresh hook
ULONG ASM SAVEDS font_refresh(REG(a0, struct Hook *hook), REG(a1, struct IntuiMessage *msg))
{
	font_data *data = (font_data *)hook->h_Data;

	// Refresh message?
	if (msg->Class == IDCMP_REFRESHWINDOW)
	{
		// Do the refresh
		font_show_font(data, TRUE);
	}

	// New size?
	else if (msg->Class == IDCMP_NEWSIZE)
	{
		// Handle resize
		LayoutResize(data->window);

		// Redraw font
		font_show_font(data, FALSE);

		// Set flag to say we resized
		data->resized = TRUE;
	}

	return (IPTR)msg;
}

#ifdef __AROS__
AROS_UFH3S(IPTR,
		  font_refresh_entry,
		  AROS_UFHA(struct Hook *, hook, A0),
		  AROS_UFHA(APTR, object, A2),
		  AROS_UFHA(APTR, message, A1))
{
	AROS_USERFUNC_INIT
	(void)object;
	return font_refresh(hook, (struct IntuiMessage *)message);
	AROS_USERFUNC_EXIT
}
#endif

// Get next font size
short font_get_size(font_data *data, short dir)
{
	short size;

	// Current size
	size = data->font_size;

	// Increment it
	size += dir;

	// Bounds check
	if (size < 2)
		size = 2;
	else if (size > 254)
		size = 254;

	return size;
}

// Build size labels
void font_build_labels(font_data *data, struct FontContentsHeader *fch)
{
	char **labels, *ptr;
	short num = 0, a, smallest = 0;
	struct FontContents *fc = NULL;
	BOOL topaz = FALSE;

	// Get number
	if (fch)
		num = fch->fch_NumEntries;
	if (num < 1)
	{
		num = 1;
		fch = NULL;
	}

	// Kludge for topaz
	if (fch)
	{
		// Get first contents pointer
		fc = (struct FontContents *)(fch + 1);

		// Topaz?
		if (strnicmp(fc->fc_FileName, "topaz/", 6) == 0)
		{
			// Add 8 and 9
			num += 2;
			topaz = TRUE;
		}
	}

	// Allocate array and buffer
	if (!(ptr = AllocVec((sizeof(char *) + 8) * (num + 1), MEMF_CLEAR)))
		return;

	// Get label array, bump pointer
	labels = (char **)ptr;
	ptr += sizeof(char *) * (num + 1);

	// No font contents?
	if (!fc)
	{
		// Null label
		labels[0] = ptr;
		labels[1] = 0;
	}

	// Get label pointers, build labels
	else
		for (a = 0; a < num; a++, ptr += 8)
		{
			// Get pointer
			labels[a] = ptr;

			// Topaz kludge?
			if (topaz && a < 2)
			{
				short val = 8 + a;

				// Add 8 or 9
				labels[a][0] = '0' + val;
				labels[a][1] = 0;

				// Is this the smallest?
				if (!smallest || val < smallest)
					smallest = val;
			}

			// Normal font
			else
			{
				// Add size
				lsprintf(labels[a], "%ld", fc->fc_YSize);

				// Is this the smallest?
				if (!smallest || fc->fc_YSize < smallest)
					smallest = fc->fc_YSize;

				// Bump contents pointer
				fc++;
			}
		}

	// Use smallest as first size to show
	if (!data->first && smallest)
		data->font_size = smallest;

	// Use new labels
	SetGadgetChoices(data->list, GAD_FONT_CYCLE, labels);

	// Disable if no labels
	DisableObject(data->list, GAD_FONT_CYCLE, (BOOL)!labels[1]);

	// Free old labels, store pointer
	FreeVec(data->size_labels);
	data->size_labels = labels;
}

// Show about requester
void font_show_about(font_data *data)
{
	struct EasyStruct easy;

	// Window open?
	if (data->about)
	{
		WindowToFront(data->about);
		ActivateWindow(data->about);
		return;
	}

	// Fill out easy struct
	easy.es_StructSize = sizeof(easy);
	easy.es_Flags = 0;
	easy.es_Title = 0;
	easy.es_TextFormat = "Directory Opus 5 Font Viewer\nv55.0 (01.08.96)\n\n� 1996 Jonathan Potter";
	easy.es_GadgetFormat = GetString(&data->locale, MSG_OK);

	// Open requester
	data->about = BuildEasyRequest(data->window, &easy, 0, 0);
}

// Save settings
void font_save_settings(font_data *data)
{
	char buf[80];
	ULONG flags = 0;

	// Set busy pointer
	SetWindowBusy(data->window);

	// Get settings
	data->extended = (BOOL)GetGadgetValue(data->list, GAD_FONT_EXTENDED);
	if (data->extended)
		flags |= FONT_SETTINGF_EXTENDED;

	// Build settings string
	lsprintf(buf,
			 "%ld/%ld/%ld/%ld/%ld\n",
			 data->window->LeftEdge,
			 data->window->TopEdge,
			 data->window->GZZWidth,
			 data->window->Height - data->window->BorderTop - 2,
			 flags);

	// Set variable
	if (SetVar("dopus/Font Viewer", buf, -1, GVF_GLOBAL_ONLY))
	{
		// Copy to ENVARC:
		SystemTagList("copy \"env:dopus/Font Viewer\" envarc:dopus", 0);
	}

	// Clear busy pointer
	ClearWindowBusy(data->window);
}

// Read settings
void font_read_settings(font_data *data)
{
	char buf[80], *ptr;
	struct IBox dims;
	UWORD flags = 0;

	// Get environment variable
	if (GetVar("dopus/Font Viewer", buf, sizeof(buf), GVF_GLOBAL_ONLY) <= 0)
		return;

	// Get pointer to buffer
	ptr = buf;

	// Parse settings
	read_parse_set(&ptr, (UWORD *)&dims.Left);
	read_parse_set(&ptr, (UWORD *)&dims.Top);
	read_parse_set(&ptr, (UWORD *)&dims.Width);
	read_parse_set(&ptr, (UWORD *)&dims.Height);
	read_parse_set(&ptr, &flags);
	data->extended = (BOOL)(flags & FONT_SETTINGF_EXTENDED);

	// Got valid size?
	if (dims.Height > 0)
	{
		// Clear character coordinates
		data->win_dims.char_dim.Top = 0;
		data->win_dims.char_dim.Left = 0;
		data->win_dims.char_dim.Width = 0;
		data->win_dims.char_dim.Height = 0;

		// Set absolute coordinates
		data->win_dims.fine_dim = dims;
	}
}

// Parse number out of string, leave pointer at one character after end of number
void read_parse_set(char **ptr, unsigned short *val)
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
