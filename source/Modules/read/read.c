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

#include "read.h"
#include <stdio.h>
#include <proto/module.h>
#include <proto/diskfont.h>
#include <proto/input.h>
#include <dopus/common.h>
#include <dopus/lib_macros.h>

#if defined(__amigaos3__) || defined(__AROS__) || defined(__amigaos4__)
struct Device *InputBase = NULL;
#else
struct Library *InputBase = NULL;
#endif

#ifdef __amigaos4__
struct InputIFace *IInput;
#endif

#define H_EXTRA 1

static long read_wheel_scroll_lines(read_data *data)
{
	if (data && data->startup && data->startup->wheel_scroll_lines > 0)
		return data->startup->wheel_scroll_lines;

	return 1;
}

#define IEQUALIFIER_BAD \
	(IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT | IEQUALIFIER_LALT | IEQUALIFIER_RALT | IEQUALIFIER_CONTROL)

static struct IBox dims;
static short tab_size = 8;
static short search_flags = SEARCHF_NOCASE;
static char *mode_string[] = {"normal", "ansi", "hex"};
static char run_once;
static short use_screen;
static ULONG modeid;
static char fontname[40];
static char editor[3][80];
static short fontsize;

int LIBFUNC L_Module_Entry(REG(a0, struct List *files),
						   REG(a1, struct Screen *screen),
						   REG(a2, IPCData *ipc),
						   REG(a3, IPCData *main_ipc),
						   REG(d0, IPTR mod_id),
						   REG(d1, IPTR mod_data))
{
	read_data *data;
	struct Node *node, *next;

	// Allocate data and memory pool
	if (!(data = AllocVec(sizeof(read_data), MEMF_CLEAR)) || !(data->memory = NewMemHandle(0, 0, 0)) ||
		!(data->scroll_timer = AllocTimer(UNIT_VBLANK, 0)))
	{
		read_free(data, (struct read_startup *)mod_id, ipc);
		return 0;
	}

	// Store IPC pointer
	data->ipc = ipc;
	data->main_ipc = main_ipc;
	data->screen = screen;
	data->slider_div = 1;
	data->startup = (struct read_startup *)mod_id;
	data->abort_bit = -1;

#ifdef __AROS__
	data->input_req.io_Message.mn_Length = sizeof(data->input_req);
#endif

	// Get input.device base
	if (!OpenDevice("input.device", 0, (struct IORequest *)&data->input_req, 0))
		InputBase = (APTR)(/*data->input_base =*/(APTR)data->input_req.io_Device);
#ifdef __amigaos4__
	IInput = (struct InputIFace *)GetInterface((struct Library *)InputBase, "main", 1, NULL);
#endif

	// Initial search?
	if (data->startup && data->startup->initial_search[0])
		data->initial_search = data->startup->initial_search;

	// Initialise tab size and flags
	data->tab_size = tab_size;
	data->mode = 0;
	data->search_flags = search_flags;
	data->flags_ok = 1;

	// Initialise dimensions
	if (dims.Width == 0)
	{
		if (screen)
		{
			dims.Top = screen->BarHeight + 1;
			dims.Width = screen->Width;
			dims.Height = screen->Height - screen->BarHeight - 1;
		}
		else
		{
			dims.Width = 640;
			dims.Height = 200;
		}
	}

	// Try for environment variable if first run
	if (!run_once || run_once == 2)
	{
		unsigned short temp = 0;

		if (GetVar("dopus/Text Viewer", data->line_buffer, sizeof(data->line_buffer), GVF_GLOBAL_ONLY) > 0)
		{
			char *ptr = data->line_buffer;

			// Parse settings
			read_parse_set(&ptr, (UWORD *)&dims.Left);
			read_parse_set(&ptr, (UWORD *)&dims.Top);
			read_parse_set(&ptr, (UWORD *)&dims.Width);
			read_parse_set(&ptr, (UWORD *)&dims.Height);
			if (!run_once)
			{
				read_parse_set(&ptr, (UWORD *)&data->tab_size);
				read_parse_set(&ptr, (UWORD *)&data->search_flags);
				read_parse_set(&ptr, (UWORD *)&use_screen);
				read_parse_set(&ptr, &temp);
				modeid = temp << 16;
				read_parse_set(&ptr, &temp);
				modeid |= temp;
				read_parse_set(&ptr, (UWORD *)&fontsize);

				// No font set?
				if (*ptr == '/')
					fontname[0] = 0;

				// Get font name
				else
				{
					stccpy(fontname, ptr, 38);
					while (*ptr && *ptr != '/')
						++ptr;
				}

				// Get editors
				if (*ptr == '/')
					++ptr;
				for (temp = 0; temp < 3; temp++)
					read_parse_string(&ptr, editor[temp], 80);

				// Null-terminate font
				ptr = fontname;
				while (*ptr)
				{
					if (*ptr == '/')
					{
						*ptr = 0;
						break;
					}
					++ptr;
				}
			}
		}
		run_once = 1;

		// Default editor if string is blank
		for (temp = 0; temp < 3; temp++)
			if (!editor[temp][0])
				strcpy(editor[temp], "ed \"%s\"");
	}

	// Called with position in startup message?
	if (data->startup && data->startup->got_pos)
	{
		// Get position from startup
		dims = data->startup->dims;

		// Set run_once flag so position will be reset next time
		run_once = 2;
	}

	// Create app message port
	data->appport = CreateMsgPort();

	// Allocate filerequester
	data->filereq = AllocAslRequestTags(ASL_FileRequest,
										ASLFR_SleepWindow,
										TRUE,
										ASLFR_TitleText,
										GetString(locale, MSG_ENTER_FILENAME),
										ASLFR_Flags1,
										FRF_DOSAVEMODE | FRF_PRIVATEIDCMP,
										ASLFR_Flags2,
										FRF_REJECTICONS,
										TAG_END);

	// Save files pointer
	data->files = files;

	// Open view window
	if (read_open_window(data))
	{
		// Go through files
		for (node = files->lh_Head; node->ln_Succ; node = node->ln_Succ)
		{
			short ret;

			// Store current node
			data->current = node;

			// Store file name pointer
			data->file = node->ln_Name;

			// Show title
			snprintf(data->title, sizeof(data->title), GetString(locale, MSG_READING_FILE), FilePart(data->file));
			SetWindowTitles(data->window, data->title, (char *)-1);

			// Set busy pointer
			SetBusyPointer(data->window);

			// If no more files, disable Next item
			if (!node->ln_Succ->ln_Succ)
			{
				OffMenu(data->window, FULLMENUNUM(0, 0, 0));
				data->no_next = 1;
			}

			// Otherwise, enable Next item if it's disabled
			else if (data->no_next)
			{
				OnMenu(data->window, FULLMENUNUM(0, 0, 0));
				data->no_next = 0;
			}

			// Read file
			if ((ret = read_file(data, mod_data)))
			{
				// Display file
				ret = read_view(data);

				// Delete file?
				if (node->ln_Pri)
					DeleteFile(node->ln_Name);
			}

			// Aborted?
			if (!ret)
				break;
		}

		// Go through files and free any custom entries
		for (node = files->lh_Head; node->ln_Succ; node = next)
		{
			// Cache next
			next = node->ln_Succ;

			// Custom?
			if (node->ln_Type == 0x7f)
			{
				Remove(node);
				FreeVec(node);
			}
		}
	}

	// Free data
	read_free(data, data->startup, ipc);

	return 1;
}

// Free read data
void read_free(read_data *data, struct read_startup *startup, IPCData *ipc)
{
	// Startup?
	if (startup)
	{
		// Say goodbye to an owner
		if (startup->owner)
			IPC_Goodbye(ipc, startup->owner, 0);

		// Free startup data
		Att_RemList((Att_List *)startup->files, 0);
		FreeVec(startup);
	}

	// Valid data?
	if (data)
	{
		// Close display
		read_close_window(data);

		// Remove AppIcon
		if (data->app_icon)
			RemoveAppIcon(data->app_icon);

		// Free icon
		if (data->app_diskobj)
			FreeDiskObject(data->app_diskobj);

		// Save flags
		if (data->flags_ok)
		{
			tab_size = data->tab_size;
			search_flags = data->search_flags;
		}

		// Close input device
		// if (data->input_base)
		if (InputBase)
		{
#ifdef __amigaos4__
			DropInterface((struct Interface *)IInput);
#endif
			CloseDevice((struct IORequest *)&data->input_req);
		}

		// Free file requester
		FreeAslRequest(data->filereq);

		// Free timer
		FreeTimer(data->scroll_timer);

		// Free app message port
		DeleteMsgPort(data->appport);

		// Free stuff
		FreeMemHandle(data->memory);
		FreeVec(data);
	}
}

// Open display
struct Window *read_open_window(read_data *data)
{
	struct Screen *screen;
	struct TextAttr font;
	struct Gadget *gadget;
	short a;

	// Initialise pen array
	for (a = 0; a < PEN_COUNT; a++)
		data->ansi_pen_array[a] = a;

	// Use supplied screen by default
	screen = data->screen;

	// Open our own screen?
	if (use_screen)
	{
		UWORD pens[1];

		// Get mode ID to use
		if (!modeid || ModeNotAvailable(modeid))
			modeid = GetVPModeID(&screen->ViewPort);

		// Public screen name
		snprintf(data->screen_name, sizeof(data->screen_name), "dopus text viewer - %lx", (unsigned long)(IPTR)data);

		// Open screen
		pens[0] = (UWORD)~0;
		if ((data->my_screen = OpenScreenTags(0,
											  SA_Depth,
											  (data->mode == MODE_ANSI) ? 4 : 2,
											  SA_DisplayID,
											  modeid,
											  SA_AutoScroll,
											  TRUE,
											  SA_Pens,
											  pens,
											  SA_SysFont,
											  1,
											  SA_Title,
											  GetString(locale, MSG_SCREEN_TITLE),
											  SA_Interleaved,
											  (data->mode == MODE_ANSI) ? TRUE : FALSE,
											  SA_SharePens,
											  TRUE,
											  SA_PubName,
											  data->screen_name,
											  TAG_END)))
		{
			screen = data->my_screen;

			// Default dimensions
			dims.Left = 0;
			dims.Top = screen->BarHeight + 1;
			dims.Width = screen->Width;
			dims.Height = screen->Height - dims.Top;
		}
	}

	// Initialise BOOPSI list
	NewList(&data->boopsi_list);

	// Create iconify gadget
	gadget = CreateTitleGadget(screen, &data->boopsi_list, FALSE, 0, IM_ICONIFY, GAD_ID_ICONIFY);

	// Open window
	if (!(data->window = OpenWindowTags(0,
										WA_Top,
										dims.Top,
										WA_Left,
										dims.Left,
										WA_Width,
										dims.Width,
										WA_Height,
										dims.Height,
										WA_SizeGadget,
										TRUE,
										WA_DragBar,
										TRUE,
										WA_DepthGadget,
										TRUE,
										WA_CloseGadget,
										TRUE,
										WA_Activate,
										TRUE,
										WA_SizeBRight,
										TRUE,
										WA_SizeBBottom,
										TRUE,
										WA_NewLookMenus,
										TRUE,
										WA_IDCMP,
										IDCMP_CLOSEWINDOW | IDCMP_NEWSIZE | IDCMP_MENUPICK | IDCMP_IDCMPUPDATE |
											IDCMP_INACTIVEWINDOW | IDCMP_GADGETUP | IDCMP_RAWKEY | IDCMP_MENUVERIFY |
											IDCMP_MOUSEBUTTONS |
#ifdef IDCMP_EXTENDEDMOUSE
											(DOPUS_NATIVE_WHEEL_SUPPORTED() ? IDCMP_EXTENDEDMOUSE : 0) |
#endif
											IDCMP_MOUSEMOVE,
										WA_PubScreen,
										screen,
										WA_PubScreenFallBack,
										TRUE,
										WA_WindowName,
										DOPUS_WIN_NAME,
										WA_MinWidth,
										128,
										WA_MinHeight,
										64,
										WA_MaxWidth,
										-1,
										WA_MaxHeight,
										-1,
										(gadget) ? WA_Gadgets : TAG_IGNORE,
										gadget,
										TAG_END)))
		return 0;

	// Fix title gadgets
	FixTitleGadgets(data->window);

	// Make window busy
	SetBusyPointer(data->window);

	// Set window ID
	SetWindowID(data->window, &data->window_id, WINDOW_TEXT_VIEWER, 0);

	// Add AppWindow
	if (data->appport)
		data->appwindow = AddAppWindowA(0, 0, data->window, data->appport, 0);

	// Want a custom font?
	if (fontname[0])
	{
		font.ta_Name = fontname + 1;
		font.ta_YSize = fontsize;
		font.ta_Style = 0;
		font.ta_Flags = 0;
		if ((data->font = OpenDiskFont(&font)))
			SetFont(data->window->RPort, data->font);
	}

	// Get DrawInfo and visual info
	data->drawinfo = GetScreenDrawInfo(data->window->WScreen);
	data->vi = GetVisualInfoA(data->window->WScreen, 0);

	// Initialise BOOPSI list and scrollers
	AddScrollBars(data->window, &data->boopsi_list, data->drawinfo, SCROLL_VERT | SCROLL_HORIZ);

	// Create menus
	if ((data->menus = BuildMenuStrip(read_menus, locale)))
	{
		struct MenuItem *item;

		// Lay menus out
		LayoutMenus(data->menus, data->vi, GTMN_NewLookMenus, TRUE, TAG_END);

		// If not using ASL 38+, disable Pick screen mode
		if (AslBase->lib_Version < 38)
		{
			if ((item = FindMenuItem(data->menus, MENU_SCREEN_MODE)))
				item->Flags &= ~ITEMENABLED;
		}

		// Attach menus
		SetMenuStrip(data->window, data->menus);

		// No next file?
		if (data->no_next)
		{
			OffMenu(data->window, FULLMENUNUM(0, 0, 0));
		}
	}

	// Get scrollers
	if ((data->vert_scroller = FindBOOPSIGadget(&data->boopsi_list, GAD_VERT_SCROLLER)))
	{
		// Get pointer to horizontal scroller
		data->horiz_scroller = FindBOOPSIGadget(&data->boopsi_list, GAD_HORIZ_SCROLLER);

		// Add scrollers
		AddGList(data->window, data->vert_scroller, -1, -1, 0);
		RefreshGList(data->vert_scroller, data->window, 0, -1);

		// Clear window busy
		ClearPointer(data->window);

		return data->window;
	}

	return 0;
}

// Close display
void read_close_window(read_data *data)
{
	// Close window
	if (data->window)
	{
		// Any pens allocated?
		if (((struct Library *)GfxBase)->lib_Version >= 39 && data->pen_alloc)
		{
			short a;

			// Free pens
			for (a = 0; a < PEN_COUNT; a++)
			{
				// Allocated?
				if (data->pen_alloc & (1 << a))
				{
					// Free pen
					ReleasePen(data->window->WScreen->ViewPort.ColorMap, data->ansi_pen_array[a]);
				}
			}
			data->pen_alloc = 0;
		}

		// Store window dimensions
		dims = *((struct IBox *)&data->window->LeftEdge);

		// Free draw info and visual info
		if (data->drawinfo)
		{
			FreeScreenDrawInfo(data->window->WScreen, data->drawinfo);
			data->drawinfo = 0;
		}
		if (data->vi)
		{
			FreeVisualInfo(data->vi);
			data->vi = 0;
		}

		// Close window, free stuff
		ClearMenuStrip(data->window);
		RemoveAppWindow(data->appwindow);
		data->appwindow = 0;
		CloseWindow(data->window);
		data->window = 0;
		BOOPSIFree(&data->boopsi_list);

		// Close font
		if (data->font)
		{
			CloseFont(data->font);
			data->font = 0;
		}

		// Free menus
		FreeMenus(data->menus);
		data->menus = 0;
	}

	// Close screen
	if (data->my_screen)
	{
		CloseScreen(data->my_screen);
		data->my_screen = 0;
	}
}

// Read text file
BOOL read_file(read_data *data, ULONG type)
{
	BPTR file;
	D_S(struct FileInfoBlock, fib)
	struct DateTime dt;
	short ret = 1;

	memset(fib, 0, sizeof(*fib));

	// Clear any existing file
	ClearMemHandle(data->memory);
	NewList((struct List *)&data->text);
	NewList((struct List *)&data->text_data);
	data->current_block = 0;
	data->current_line = 0;
	data->text_buffer_size = 0;
	data->text_lines = 0;
	data->lines = 0;
	data->top = 0;
	data->left = 0;
	data->top_line = 0;
	data->top_position = 0;
	data->search_line = 0;
	data->search_last_line = 0;
	data->flags = 0;
	data->sel_top_pos = -1;

	// Initialise scrollers
	SetGadgetAttrs(data->vert_scroller, data->window, 0, PGA_Total, 0, PGA_Top, 0, TAG_END);
	SetGadgetAttrs(data->horiz_scroller, data->window, 0, PGA_Total, 0, PGA_Top, 0, TAG_END);

	// Clear window interior
	SetAPen(data->window->RPort, data->drawinfo->dri_Pens[BACKGROUNDPEN]);
	RectFill(data->window->RPort,
			 data->window->BorderLeft,
			 data->window->BorderTop,
			 data->window->Width - data->window->BorderRight - 1,
			 data->window->Height - data->window->BorderBottom - 1);

	// Lock file
	if (!(file = Lock(data->file, ACCESS_READ)) || !(Examine(file, fib)))
		ret = 0;

	// Unlock file
	UnLock(file);

	// Ok to read?
	if (ret)
	{
		// Store date and size
		dt.dat_Stamp = fib->fib_Date;
		dt.dat_Flags = 0;
		dt.dat_StrDay = 0;
		dt.dat_StrDate = data->date;
		dt.dat_StrTime = 0;
		DateToStr(&dt);
		data->size = fib->fib_Size;

		// Open file
		if (!(file = Open(data->file, MODE_OLDFILE)))
			ret = 0;
		else
		{
			char extra[256];
			short extra_size = 0;
			long read_count = 0;

			// Allocate abort bit
			data->abort_bit = AllocSignal(-1);
			SetSignal(0, 1 << data->abort_bit);

			// If file is bigger than the chunk size, we'll show a progress window
			if (fib->fib_Size > READCHUNK_SIZE)
				data->progress = OpenProgressWindowTags(PW_Window,
														(IPTR)data->window,
														PW_Title,
														(IPTR)GetString(locale, MSG_READING_FILE_TITLE),
														PW_SigTask,
														(IPTR)FindTask(0),
														PW_SigBit,
														data->abort_bit,
														PW_FileCount,
														fib->fib_Size,
														PW_Flags,
														PWF_GRAPH,
														TAG_END);

			// Loop while data remains
			while (fib->fib_Size > 0)
			{
				long chunk_size, read_size, read_amount, read_total;
				text_chunk *chunk = 0;
				char *read_ptr;

				// Get initial chunk size
				chunk_size = (fib->fib_Size > READCHUNK_SIZE) ? READCHUNK_SIZE : fib->fib_Size;

				// Loop until we allocate it
				while (chunk_size > 0)
				{
					// Allocate chunk
					if ((chunk = AllocMemH(data->memory, sizeof(text_chunk) + chunk_size)))
						break;

					// Halve the chunk size
					chunk_size >>= 1;
				}

				// Failed?
				if (!chunk)
				{
					ret = 0;
					break;
				}

				// Initially read full chunk size
				read_size = chunk_size;
				read_ptr = chunk->buffer;

				// Any extra data?
				if (extra_size > 0)
				{
					// Copy to buffer
					CopyMem(extra, chunk->buffer, extra_size);

					// Advance read pointer
					read_ptr += extra_size;
					read_size -= extra_size;
				}

				// Get amount to read
				read_amount = read_size;
				read_total = 0;

				// Read data into chunk
				while (read_amount > 0)
				{
					long count;

					// Get size to read
					read_size = (read_amount > READ_READ_SIZE) ? READ_READ_SIZE : read_amount;

					// Read data
					if ((count = Read(file, read_ptr, read_size)) < 1)
					{
						ret = 0;
						break;
					}

					// Decrement read amount
					read_amount -= count;

					// Increment pointer
					read_ptr += count;
					read_total += count;

					// Got progress window?
					if (data->progress)
					{
						// Increment total
						read_count += count;

						// Update progress window
						SetProgressWindowTags(data->progress, PW_FileNum, read_count, TAG_END);

						// Check abort
						if (SetSignal(0, 0) & (1 << data->abort_bit))
						{
							// Cancel
							ret = 0;
							break;
						}
					}
				}

				// Failed
				if (!ret)
					break;

				// Store chunk size
				chunk->size = read_total + extra_size;
				extra_size = 0;

				// Decrement total size
				fib->fib_Size -= chunk->size;

				// Increment total size
				data->text_buffer_size += chunk->size;

				// Add chunk to list
				AddTail((struct List *)&data->text_data, (struct Node *)chunk);

				// Not the end of the file?
				if (fib->fib_Size > 0)
				{
					long pos, num;

					// Seek backwards for last newline
					for (pos = chunk->size - 1, num = 0; pos > 0 && num < 256; pos--, ++num)
						if (chunk->buffer[pos] == '\n')
							break;

					// Got new-line?
					if (pos > 0 && pos < chunk->size - 1 && num < 256)
					{
						short size;

						// Bump position
						++pos;

						// Calculate size of extraneous data
						size = chunk->size - pos;

						// Copy to extra
						CopyMem(chunk->buffer + pos, extra, size);
						extra[size] = 0;
						extra_size = size;

						// Subtract from chunk size
						chunk->size -= size;
						data->text_buffer_size -= size;

						// Add back to file size to read
						fib->fib_Size += size;
					}
				}
			}

			// Close file
			Close(file);
		}

		// Count lines
		if (ret)
			ret = read_set_mode(data, type, FALSE);

		// Close progress window
		CloseProgressWindow(data->progress);
		data->progress = 0;

		// Free signal bit
		if (data->abort_bit != -1)
			FreeSignal(data->abort_bit);
		data->abort_bit = -1;
	}

	return ret;
}

// Set up for a display mode
BOOL read_set_mode(read_data *data, short mode, BOOL keep_pos)
{
	BOOL ansi_flag = 0;
	BOOL ret = 1;

	// Clear some stuff
	if (!keep_pos)
	{
		data->top = 0;
		data->left = 0;
	}
	data->top_line = 0;
	data->top_position = 0;
	data->search_line = 0;
	data->search_last_line = 0;

	// Which mode?
	switch (mode)
	{
	case MODE_ANSI:
	case MODE_NORMAL:
	case MODE_SMART:

		// Haven't counted lines yet?
		if (IsListEmpty((struct List *)&data->text))
		{
			register unsigned long bufpos = 0, chunkpos = 0;
			register short length = 0;
			register unsigned char ch;
			long buf_count = 0;
			short nonprint_count = 0, nonprint_tri = 0, smart_count = 0;
			char *start_ptr;
			text_chunk *chunk;

			// Get first chunk
			chunk = (text_chunk *)data->text_data.mlh_Head;
			start_ptr = chunk->buffer;

			// Got progress window
			if (data->progress)
			{
				// Update
				SetProgressWindowTags(data->progress,
									  PW_Title,
									  (IPTR)GetString(locale, MSG_COUNTING_LINES),
									  PW_FileCount,
									  data->text_buffer_size,
									  PW_FileNum,
									  0,
									  TAG_END);
			}

			// Search for lines
			for (; bufpos < data->text_buffer_size; bufpos++, length++)
			{
				// Get character
				ch = chunk->buffer[chunkpos];

				// End of line?
				if (ch == '\n' || length == 255 || bufpos == data->text_buffer_size - 1)
				{
					register read_line *line;

					// Allocate line node
					if (!(line = read_alloc_node(data)))
					{
						break;
					}

					// If valid character, increment length
					if (length < 255 && ch != '\n')
						++length;

					// Store length and text pointer
					line->length = length;
					line->text = start_ptr;

					// Add line to list and increment line count
					AddTail((struct List *)&data->text, (struct Node *)line);
					++data->text_lines;

					// Update progress window every so often
					if (data->progress && buf_count > READ_READ_SIZE)
					{
						// Update progress window
						SetProgressWindowTags(data->progress, PW_FileNum, bufpos, TAG_END);
						buf_count = 0;

						// Check abort
						if (SetSignal(0, 0) & (1 << data->abort_bit))
						{
							// Cancel
							ret = 0;
							break;
						}
					}
					else
						buf_count += length;

					// Store position for start of next line
					start_ptr += (length < 255) ? length + 1 : 256;
					length = -1;
				}

				// Escape sequence?
				else if (ch == 0x1b || ch == 0x9b)
					ansi_flag = 1;

				// Otherwise, smart mode?
				else if (mode == MODE_SMART && smart_count < 64)
				{
					// Non-printable?
					if (ch < 0x8 || (ch > 0xd && ch < 0x20) || (ch > 0x9d && ch < 0xc0))
					{
						// Increment non-print count
						++nonprint_count;

						// Three together?
						if (nonprint_count == 3)
						{
							++nonprint_tri;
							nonprint_count = 0;
						}
					}

					// Reset non-print count
					else
						nonprint_count = 0;

					// Increment smart count
					++smart_count;
				}

				// End of this chunk?
				if (++chunkpos == chunk->size)
				{
					// Get next chunk
					chunk = (text_chunk *)chunk->node.mln_Succ;
					chunkpos = 0;
					start_ptr = chunk->buffer;
				}
			}

			// Smart mode?
			if (mode == MODE_SMART)
			{
				// If there's two or more non-printable tri's, use hex mode
				if (nonprint_tri >= 2 || (nonprint_tri && smart_count < 32))
				{
					// Use hex mode
					mode = MODE_HEX;
				}
			}
		}

		// Store normal line count
		data->lines = data->text_lines;

		// If still smart mode, use normal
		if (mode == MODE_SMART)
		{
			if (ansi_flag)
				mode = MODE_ANSI;
			else
				mode = MODE_NORMAL;
		}

		// Unless in hex mode now, break out
		if (mode != MODE_HEX)
			break;

	case MODE_HEX:

		// Calculate line count
		data->lines = UDivMod32(data->text_buffer_size + (HEX_DISPLAY_WIDTH - 1), HEX_DISPLAY_WIDTH);
		break;
	}

	// Store mode
	data->mode = mode;

	// ANSI mode?
	if (mode == MODE_ANSI)
	{
		// Try to allocate pens?
		if (((struct Library *)GfxBase)->lib_Version >= 39 && !data->pen_alloc)
		{
			// Don't try unless screen is deeper than 8 colours
			if (data->window->WScreen->RastPort.BitMap->Depth > 3)
			{
				short a;
				long pen;

				// Allocate pens if possible
				for (a = 1; a < PEN_COUNT; a++)
				{
					// Try to allocate a pen
					if ((pen = ObtainPen(data->window->WScreen->ViewPort.ColorMap,
										 -1,
										 (ansi_palette[a] & 0xff0000) << 8,
										 (ansi_palette[a] & 0x00ff00) << 16,
										 (ansi_palette[a] & 0x0000ff) << 24,
										 0)) == -1)
					{
						// Try to share a pen
						pen = ObtainBestPenA(data->window->WScreen->ViewPort.ColorMap,
											 (ansi_palette[a] & 0xff0000) << 8,
											 (ansi_palette[a] & 0x00ff00) << 16,
											 (ansi_palette[a] & 0x0000ff) << 24,
											 0);
					}

					// Get one?
					if (pen != -1)
					{
						// Store pen, set alloc bit
						data->ansi_pen_array[a] = pen;
						data->pen_alloc |= 1 << a;
					}
				}
			}
		}

		// 2.0 version
		else if (data->my_screen && data->my_screen->RastPort.BitMap->Depth > 3)
		{
			short a;

			// Use pens 4-12, set colours
			for (a = 0; a < PEN_COUNT; a++)
			{
				data->ansi_pen_array[a] = a + 4;
				SetRGB4(&data->window->WScreen->ViewPort,
						a + 4,
						(ansi_palette[a] & 0xff0000) >> 16,
						(ansi_palette[a] & 0x00ff00) >> 8,
						(ansi_palette[a] & 0x0000ff));
			}
		}
	}

	return ret;
}

// Display file
BOOL read_view(read_data *data)
{
	short quit_flag = 0;

	// Initialise display
	read_init(data);

	// Clear pointer
	ClearPointer(data->window);

	// Initial search?
	if (data->initial_search)
	{
		// Copy search string
		strcpy(data->search_text, data->initial_search);
		data->initial_search = 0;

		// Initiate search
		read_search(data, 1);
	}

	// Event loop
	FOREVER
	{
		struct IntuiMessage *msg;
		IPCMessage *imsg;
		struct AppMessage *amsg;

		// AppMessages?
		if (data->appwindow || data->app_icon)
		{
			BOOL remove = 0;

			while ((amsg = (struct AppMessage *)GetMsg(data->appport)))
			{
				short arg;
				struct Node *insert = data->current;

				// DOpus message?
				if (amsg->am_Type == MTYPE_APPSNAPSHOT)
				{
					struct AppSnapshotMsg *asma;

					// Get SnapShot message pointer
					asma = (struct AppSnapshotMsg *)amsg;

					// Menu operation?
					if (asma->flags & APPSNAPF_MENU)
					{
						// Close?
						if (asma->id == 0)
						{
							// Set quit flag
							quit_flag = 2;
							remove = 1;
						}
					}
				}

				// AppIcon?
				else if (amsg->am_Type == MTYPE_APPICON)
					remove = 1;

				// Go through arguments
				for (arg = 0; arg < amsg->am_NumArgs; arg++)
				{
					// Is argument a file?
					if (amsg->am_ArgList[arg].wa_Name && *amsg->am_ArgList[arg].wa_Name)
					{
						struct Node *node;

						// Build full filename
						GetWBArgPath(&amsg->am_ArgList[arg], data->line_buffer, 256);

						// Allocate new node
						if ((node = AllocVec(sizeof(struct Node) + strlen(data->line_buffer) + 1, MEMF_CLEAR)))
						{
							// Fill out node
							node->ln_Name = (char *)(node + 1);
							strcpy(node->ln_Name, data->line_buffer);
							node->ln_Type = 0x7f;

							// Add to list of files after current node
							Insert(data->files, node, insert);
							insert = node;

							// Set quit flag
							quit_flag = 1;
						}
					}
				}

				// Error
				if (!quit_flag && data->window)
					DisplayBeep(data->window->WScreen);

				// Reply to the message
				ReplyMsg((struct Message *)amsg);
			}

			// Remove AppIcon?
			if (remove)
			{
				// Remove the app icon
				RemoveAppIcon(data->app_icon);
				data->app_icon = 0;

				// Don't want to quit?
				if (quit_flag != 2)
				{
					// Open the viewer
					read_open_window(data);

					// Initialise mode
					read_set_mode(data, data->mode, TRUE);

					// Display text
					read_init(data);
				}
			}
		}

		// IPC messages?
		if (data->ipc)
		{
			while ((imsg = (IPCMessage *)GetMsg(data->ipc->command_port)))
			{
				// Abort?
				if (imsg->command == IPC_ABORT || imsg->command == IPC_QUIT)
				{
					quit_flag = 2;
				}

				// Read a new file?
				else if (imsg->command == READCOM_READ)
				{
					struct Node *node;

					// Allocate node
					if ((node = AllocVec(sizeof(struct Node) + strlen((char *)imsg->data) + 1, MEMF_CLEAR)))
					{
						// Fill out node
						node->ln_Name = (char *)(node + 1);
						strcpy(node->ln_Name, (char *)imsg->data);
						node->ln_Type = 0x7f;

						// Add to list of files after current node
						Insert(data->files, node, data->current);

						// Delete after reading?
						if (imsg->flags)
							node->ln_Pri = 1;

						// Set quit flag
						quit_flag = 1;
					}
				}

				// Reply to message
				IPC_Reply(imsg);
			}
		}

		// Window messages
		if (data->window)
		{
			while ((msg = (struct IntuiMessage *)GetMsg(data->window->UserPort)))
			{
				struct IntuiMessage msg_copy;
#ifdef IDCMP_EXTENDEDMOUSE
				struct IntuiWheelData wheel_data;
#endif
				short sel_dx = 0, sel_dy = 0;

				// Copy message
				msg_copy = *msg;
#ifdef IDCMP_EXTENDEDMOUSE
				if (msg_copy.Class == IDCMP_EXTENDEDMOUSE && msg_copy.Code == IMSGCODE_INTUIWHEELDATA &&
					msg_copy.IAddress)
				{
					wheel_data = *(struct IntuiWheelData *)msg_copy.IAddress;
					msg_copy.IAddress = &wheel_data;
				}
#endif

				// Menu verify?
				if (msg->Class == IDCMP_MENUVERIFY)
				{
					// Check right button is down and window is active
					if (msg->Qualifier & IEQUALIFIER_RBUTTON && data->window->Flags & WFLG_WINDOWACTIVE)
					{
						// See if mouse is within window boundaries
						if (msg->MouseX > data->window->BorderLeft &&
							msg->MouseX < data->window->Width - data->window->BorderRight &&
							msg->MouseY > data->window->BorderTop &&
							msg->MouseY < data->window->Height - data->window->BorderBottom)
						{
							// Cancel menu event
							msg->Code = MENUCANCEL;

							// Change copy to MOUSEBUTTONS
							msg_copy.Class = IDCMP_MOUSEBUTTONS;
							msg_copy.Code = MENUDOWN;
						}
					}
				}

				// Reply to it
				ReplyMsg((struct Message *)msg);

				// Look at message
				switch (msg_copy.Class)
				{
				// Gadget
				case IDCMP_GADGETUP:

					// Iconify?
					if (((struct Gadget *)msg_copy.IAddress)->GadgetID == GAD_ID_ICONIFY && data->appport)
					{
						struct TagItem tags[3];

						// No icon yet?
						if (!data->app_diskobj && !(data->app_diskobj = GetDiskObject("DOpus5:Icons/read")) &&
							!(data->app_diskobj = GetDefDiskObject(WBPROJECT)))
							break;

						// Fill out tags
						tags[0].ti_Tag = DAE_SnapShot;
						tags[0].ti_Data = 0;
						tags[1].ti_Tag = DAE_Menu;
						tags[1].ti_Data = (IPTR)GetString(locale, MSG_CLOSE);
						tags[2].ti_Tag = TAG_END;

						// Try to add AppIcon
						if (!(data->app_icon =
								  AddAppIconA(0, 0, FilePart(data->file), data->appport, 0, data->app_diskobj, tags)))
							break;

						// Close display
						read_close_window(data);
					}
					break;

				// Close window
				case IDCMP_CLOSEWINDOW:
					quit_flag = 1;
					break;

				// Resized
				case IDCMP_NEWSIZE:
					read_calc_size(data);
					read_update_text(data, 0, 0, 0);
					break;

				// Menu
				case IDCMP_MENUPICK: {
					struct MenuItem *item;
					short itemid;

					// Loop while valid items
					while ((item = ItemAddress(data->window->MenuStrip, (UWORD)msg_copy.Code)))
					{
						// Get real ID
						itemid = (UWORD)(IPTR)(GTMENUITEM_USERDATA(item));

						switch (itemid)
						{
						// Next file
						case MENU_NEXT:
							quit_flag = 1;
							break;

						// Quit
						case MENU_QUIT:
							quit_flag = 2;
							break;

						// Tab size
						case MENU_TAB_1:
						case MENU_TAB_2:
						case MENU_TAB_4:
						case MENU_TAB_8: {
							short tab;

							// Get new tab size
							tab = itemid - MENU_TAB_1;
							tab = 1 << tab;

							// Change?
							if (tab != data->tab_size)
							{
								// Set busy pointer
								SetBusyPointer(data->window);

								// Update tab size
								data->tab_size = tab;

								// Get text width
								read_get_width(data);

								// Update scrollers
								read_calc_size(data);

								// Display text
								read_show_text(data, 0, 0, 0);

								// Display title
								read_build_title(data);

								// Clear pointer
								ClearPointer(data->window);
							}
						}
						break;

						// Search
						case MENU_SEARCH:
							read_search(data, 1);
							break;

						// Repeat search
						case MENU_REPEAT:
							read_search(data, 0);
							break;

						// Save settings
						case MENU_SAVE: {
							// Set busy pointer
							SetBusyPointer(data->window);

							// Build variable string
							snprintf(data->line_buffer,
									 sizeof(data->line_buffer),
									 "%ld/%ld/%ld/%ld/%ld/%ld/%ld/%ld/%ld/%ld/%s/%s|%s|%s\n",
									 (long)data->window->LeftEdge,
									 (long)data->window->TopEdge,
									 (long)data->window->Width,
									 (long)data->window->Height,
									 (long)data->tab_size,
									 (long)data->search_flags,
									 (long)use_screen,
									 (long)((modeid >> 16) & 0xffff),
									 (long)(modeid & 0xffff),
									 (long)fontsize,
									 fontname,
									 editor[0],
									 editor[1],
									 editor[2]);

							// Set variable
							SetEnv("dopus/Text Viewer", data->line_buffer, TRUE);

							// Clear busy pointer
							ClearPointer(data->window);
							break;
						}

						// Print
						case MENU_PRINT:
							read_print(data);
							break;

						// Save As
						case MENU_SAVE_AS:
							read_save(data);
							break;

						// Mode
						case MENU_MODE_NORMAL:
						case MENU_MODE_ANSI:

							// Set busy pointer
							SetBusyPointer(data->window);

							// Change mode
							read_set_mode(data, (itemid == MENU_MODE_NORMAL) ? MODE_NORMAL : MODE_ANSI, FALSE);

							// Initialise display
							read_init(data);

							// Clear busy pointer
							ClearPointer(data->window);
							break;

						case MENU_MODE_HEX:

							// Already in hex mode?
							if (data->mode == MODE_HEX)
								break;

							// Change mode
							read_set_mode(data, MODE_HEX, FALSE);

							// Initialise display
							read_init(data);
							break;

						// Use screen
						case MENU_USE_SCREEN:

							// Get state
							use_screen = (item->Flags & CHECKED) ? 1 : 0;
							item = 0;

							// Close and re-open
							read_close_window(data);
							if (!(read_open_window(data)))
								break;
							read_set_mode(data, data->mode, TRUE);
							read_init(data);
							break;

						// Pick screen mode
						case MENU_SCREEN_MODE:
							item = 0;
							read_pick_screen_mode(data);
							break;

						// Pick font
						case MENU_SELECT_FONT:
							item = 0;
							read_pick_font(data);
							break;

						// Send to editor
						case MENU_TO_EDITOR:

							// Valid editor string?
							if (editor[data->mode][0])
							{
								// Build command string
								snprintf(data->line_buffer, sizeof(data->line_buffer), editor[data->mode], data->file);

								// Launch editor
								CLI_Launch(
									data->line_buffer, data->window->WScreen, 0, 0, Open("nil:", MODE_OLDFILE), 0, 0);
								break;
							}

						// Pick editor
						case MENU_SELECT_EDITOR_NORMAL:
						case MENU_SELECT_EDITOR_ANSI:
						case MENU_SELECT_EDITOR_HEX:
							item = 0;
							read_pick_editor(data, itemid - MENU_SELECT_EDITOR_NORMAL);
							break;
						}

						// Get next item
						if (item)
							msg_copy.Code = item->NextSelect;
						else
							break;

						// Check quit flag
						if (quit_flag)
							break;
					}
					break;
				}

				// BOOPSI gadget
				case IDCMP_IDCMPUPDATE:

					switch (GetTagData(GA_ID, 0, (struct TagItem *)msg_copy.IAddress))
					{
					// Vertical slider
					case GAD_VERT_SCROLLER: {
						IPTR top;

						// Get new top
						GetAttr(PGA_Top, (Object *)data->vert_scroller, &top);
						if (data->slider_div > 1)
						{
							top = UMult32(top, data->slider_div);
							if (top + data->v_visible > data->lines)
								top = data->lines - data->v_visible;
						}

						// Scroll display
						if (top != data->top)
							read_show_text(data, 0, top - data->top, 0);
					}
					break;

					// Scroll up
					case GAD_VERT_ARROW_UP:
						if (data->top > 0)
							read_update_text(data, 0, -1, 0);
						break;

					// Scroll down
					case GAD_VERT_ARROW_DOWN:
						if (data->top + data->v_visible < data->lines)
							read_update_text(data, 0, 1, 0);
						break;

					// Horizontal slider
					case GAD_HORIZ_SCROLLER: {
						IPTR top;

						// Get new top
						GetAttr(PGA_Top, (Object *)data->horiz_scroller, &top);

						// Scroll display
						if (top != data->left)
							read_show_text(data, top - data->left, 0, 0);
					}
					break;

					// Scroll left
					case GAD_HORIZ_ARROW_LEFT:
						if (data->left > 0)
							read_update_text(data, -1, 0, 0);
						break;

					// Scroll right
					case GAD_HORIZ_ARROW_RIGHT:
						if (data->left + data->h_visible < data->columns)
							read_update_text(data, 1, 0, 0);
						break;
					}
					break;
#ifdef IDCMP_EXTENDEDMOUSE
				// native mouse wheel support
				case IDCMP_EXTENDEDMOUSE:
					if (DOPUS_NATIVE_WHEEL_SUPPORTED() && msg_copy.Code == IMSGCODE_INTUIWHEELDATA &&
						msg_copy.IAddress)
					{
						struct IntuiWheelData *iwd = (struct IntuiWheelData *)msg_copy.IAddress;

						if (iwd->WheelY < 0)
						{
							if (data->top <= 0)
								break;
							if (msg_copy.Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
								read_update_text(data, 0, -(data->v_visible - 1), 0);
							else if (msg_copy.Qualifier & IEQUALIFIER_CONTROL)
								read_update_text(data, 0, -data->top, 0);
							else
							{
								long scroll_line = read_wheel_scroll_lines(data);

								read_update_text(data, 0, scroll_line * iwd->WheelY, 0);
							}
						}

						else if (iwd->WheelY > 0)
						{
							if (data->top + data->v_visible >= data->lines)
								break;
							if (msg_copy.Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
								read_update_text(data, 0, data->v_visible - 1, 0);
							else if (msg_copy.Qualifier & IEQUALIFIER_CONTROL)
								read_update_text(data, 0, (data->lines - data->top) - data->v_visible, 0);
							else
							{
								long scroll_line = read_wheel_scroll_lines(data);

								read_update_text(data, 0, scroll_line * iwd->WheelY, 0);
							}
						}
					}
					break;
#endif

				// Key press
				case IDCMP_RAWKEY:

					switch (msg_copy.Code)
					{
					// Help?
					case 0x5f:

						// Valid main IPC?
						if (data->main_ipc && !(msg_copy.Qualifier & VALID_QUALIFIERS))
						{
							// Set busy pointer
							SetBusyPointer(data->window);

							// Send help request
							IPC_Command(data->main_ipc, IPC_HELP, (1 << 31), "Read", 0, (struct MsgPort *)-1);

							// Clear busy pointer
							ClearPointer(data->window);
						}
						break;

					// Quit
					case 0x10:
						if (msg_copy.Qualifier & IEQUALIFIER_BAD)
							break;
					case 0x45:
						quit_flag = 1;
						break;

					// Scroll up
					case CURSORUP:
					case RAWKEY_WHEEL_UP:
					case 0x3e: /* KP 8 */

						// Ok to scroll up?
						if (data->top <= 0)
							break;

						// Page up?
						if (msg_copy.Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
							read_update_text(data, 0, -(data->v_visible - 1), 0);

						// Top
						else if (msg_copy.Qualifier & IEQUALIFIER_CONTROL)
						{
							if (data->top > 0)
								read_update_text(data, 0, -data->top, 0);
						}

						// Line up
						else
						{
							long scroll_line =
								(msg_copy.Code == RAWKEY_WHEEL_UP) ? read_wheel_scroll_lines(data) : 1;

							read_update_text(data, 0, -scroll_line, 0);
						}
						break;

					// Page up
					case 0x16:
						if (msg_copy.Qualifier & IEQUALIFIER_BAD)
							break;
					case 0x3f:
					case 0x41:

						// Ok to scroll up?
						if (data->top <= 0)
							break;
						read_update_text(data, 0, -(data->v_visible - 1), 0);
						break;

					// Home
					case 0x14:
						if (msg_copy.Qualifier & IEQUALIFIER_BAD)
							break;
					case 0x3d:
						if (data->top > 0)
							read_update_text(data, 0, -data->top, 0);
						break;

					// Scroll down
					case CURSORDOWN:
					case RAWKEY_WHEEL_DOWN:
					case 0x1e: /* KP 2 */

						// Ok to scroll down?
						if (data->top + data->v_visible >= data->lines)
							break;

						// Page down?
						if (msg_copy.Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
							read_update_text(data, 0, data->v_visible - 1, 0);

						// Bottom
						else if (msg_copy.Qualifier & IEQUALIFIER_CONTROL)
						{
							if (data->top < data->lines - data->v_visible)
								read_update_text(data, 0, (data->lines - data->top) - data->v_visible, 0);
						}

						// Line down
						else
						{
							long scroll_line =
								(msg_copy.Code == RAWKEY_WHEEL_DOWN) ? read_wheel_scroll_lines(data) : 1;

							read_update_text(data, 0, scroll_line, 0);
						}
						break;

					// Page down
					case 0x22:
						if (msg_copy.Qualifier & IEQUALIFIER_BAD)
							break;
					case 0x1f:
					case 0x40:

						// Ok to scroll down?
						if (data->top + data->v_visible >= data->lines)
							break;
						read_update_text(data, 0, data->v_visible - 1, 0);
						break;

					// End
					case 0x1d:
					case 0x35:
						if (data->top < data->lines - data->v_visible)
							read_update_text(data, 0, (data->lines - data->top) - data->v_visible, 0);
						break;

					// Scroll left
					case CURSORLEFT:
					case 0x2d:

						// Ok to scroll left?
						if (data->left <= 0)
							break;

						// Page left?
						if (msg_copy.Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
							read_update_text(data, -data->h_visible, 0, 0);

						// Full left
						else if (msg_copy.Qualifier & IEQUALIFIER_CONTROL)
							read_update_text(data, -data->left, 0, 0);

						// Line up
						else
							read_update_text(data, -1, 0, 0);
						break;

					// Scroll right
					case CURSORRIGHT:
					case 0x2f:

						// Ok to scroll right?
						if (data->left + data->h_visible >= data->columns)
							break;

						// Page right?
						if (msg_copy.Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
							read_update_text(data, data->h_visible, 0, 0);

						// Full right
						else if (msg_copy.Qualifier & IEQUALIFIER_CONTROL)
							read_update_text(data, (data->columns - data->left) - data->h_visible, 0, 0);

						// Line right
						else
							read_update_text(data, 1, 0, 0);
						break;

					// Function keys
					case 0x50:
					case 0x51:
					case 0x52:
					case 0x53:
					case 0x54:
					case 0x55:
					case 0x56:
					case 0x57:
					case 0x58:
					case 0x59: {
						short percent;
						long line;

						// Can move?
						if (data->lines <= data->v_visible)
							break;

						// Get percentage
						percent = (msg_copy.Code - 0x4f) * 10;

						// Get line to jump to
						line = (data->lines * percent) / 100;

						// Bounds check
						if (line + data->v_visible > data->lines)
							line = data->lines - data->v_visible;
						if (line < 0)
							line = 0;

						// Jump to mark
						if (line != data->top)
							read_update_text(data, 0, line - data->top, 0);
					}
					break;

					// Search
					case 0x21:
					case 0x23:
						if (msg_copy.Qualifier & IEQUALIFIER_BAD)
							break;
						if (!(IsListEmpty((struct List *)&data->text_data)))
							read_search(data, 1);
						break;

					// Next
					case 0x36:
						if (!(IsListEmpty((struct List *)&data->text_data)))
							read_search(data, 0);
						break;

					// Print
					case 0x19:
						if (msg_copy.Qualifier & IEQUALIFIER_BAD)
							break;
						if (!(IsListEmpty((struct List *)&data->text_data)))
							read_print(data);
						break;

					// C for copy
					case 0x33:

						// Right amiga must be down
						if (!(msg_copy.Qualifier & IEQUALIFIER_RCOMMAND))
							break;

						// Copy to clipboard
						read_clipboard_copy(data);
						break;
					}
					break;

				// Inactive window
				case IDCMP_INACTIVEWINDOW:

					// Turn into button up and fall through
					msg_copy.Code = (data->scroll) ? MENUUP : SELECTUP;

				// Mouse button press
				case IDCMP_MOUSEBUTTONS:

					// Are we scrolling?
					if (data->scroll)
					{
						// Right button up stops scroll
						if (msg_copy.Code == MENUUP)
						{
							// Abort timer
							StopTimer(data->scroll_timer);
							data->scroll = 0;
						}
						break;
					}

					// Select down?
					if (msg_copy.Code == SELECTDOWN)
					{
						// Within text area and not in hex mode?
						if (msg_copy.MouseX >= data->text_box.MinX && msg_copy.MouseY >= data->text_box.MinY &&
							msg_copy.MouseX <= data->text_box.MaxX && msg_copy.MouseY <= data->text_box.MaxY &&
							data->mode != MODE_HEX)
						{
							// Clear any old highlight
							if (data->sel_top_pos != -1)
							{
								data->sel_top_pos = -1;
								read_update_text(data, 1, 1, 0);
							}

							// Get new start position
							data->sel_top_pos = (msg_copy.MouseX - data->text_box.MinX) / data->window->RPort->TxWidth;
							data->sel_top_line =
								(msg_copy.MouseY - data->text_box.MinY) / data->window->RPort->TxHeight;

							// Add top position
							data->sel_top_pos += data->left;
							data->sel_top_line += data->top;

							// Copy to bottom position
							data->sel_bottom_pos = data->sel_top_pos;
							data->sel_bottom_line = data->sel_top_line;

							// Turn on mouse reporting and ticks and disable menus
							data->window->Flags |= WFLG_REPORTMOUSE | WFLG_RMBTRAP;
							ModifyIDCMP(data->window, data->window->IDCMPFlags | IDCMP_INTUITICKS);

							// Show highlighted text
							read_show_text(data, 1, 1, 0);

							// Set flag for ok
							data->flags |= READF_HIGH_OK;
						}
					}

					// Select up?
					else if (msg_copy.Code == SELECTUP)
					{
						// Are we highlighting?
						if (data->window->Flags & WFLG_REPORTMOUSE)
						{
							// Turn off mouse reporting and ticks and enable menus
							data->window->Flags &= ~(WFLG_REPORTMOUSE | WFLG_RMBTRAP);
							ModifyIDCMP(data->window, data->window->IDCMPFlags & ~IDCMP_INTUITICKS);

							// Was the highlight ok?
							if (data->flags & READF_HIGH_OK)
							{
								// Check if end is before start
								if (data->sel_bottom_line < data->sel_top_line)
								{
									short pos, line;

									// Swap top and bottom
									pos = data->sel_top_pos;
									line = data->sel_top_line;
									data->sel_top_pos = data->sel_bottom_pos;
									data->sel_top_line = data->sel_bottom_line;
									data->sel_bottom_pos = pos;
									data->sel_bottom_line = line;
								}

								// Or on the same line
								else if (data->sel_bottom_line == data->sel_top_line &&
										 data->sel_bottom_pos < data->sel_top_pos)
								{
									short pos;

									// Swap top and bottom
									pos = data->sel_top_pos;
									data->sel_top_pos = data->sel_bottom_pos;
									data->sel_bottom_pos = pos;
								}
							}

							// Otherwise, no highlight
							else
								data->sel_top_pos = -1;
						}
					}

					// Middle-down does copy
					else if (msg_copy.Code == MIDDLEDOWN)
					{
						// Copy to clipboard
						read_clipboard_copy(data);
					}

					// Right-button starts scroll timer
					else if (msg_copy.Code == MENUDOWN)
					{
						// Start timer
						StartTimer(data->scroll_timer, 0, 2);
						data->scroll = 1;
					}
					break;

				// Ticks
				case IDCMP_INTUITICKS:

					// Is mouse button down?
					if (!(msg_copy.Qualifier & IEQUALIFIER_LEFTBUTTON) || !(data->window->Flags & WFLG_REPORTMOUSE))
						break;

					// Get scroll direction
					if (data->window->MouseY < data->text_box.MinY)
					{
						if (data->top == 0)
							break;
						sel_dy = -1;
					}
					else if (data->window->MouseY > data->text_box.MaxY)
					{
						if (data->top + data->v_visible >= data->lines)
							break;
						sel_dy = 1;
					}
					else if (data->window->MouseX < data->text_box.MinX)
					{
						if (data->left == 0)
							break;
						sel_dx = -1;
					}
					else if (data->window->MouseX > data->text_box.MaxX)
					{
						if (data->left + data->h_visible >= data->columns)
							break;
						sel_dx = 1;
					}
					else
						break;

					// Fall through

				// Mouse move
				case IDCMP_MOUSEMOVE: {
					short old_pos, old_line;

					// Is mouse button down?
					if (!(msg_copy.Qualifier & IEQUALIFIER_LEFTBUTTON) || !(data->window->Flags & WFLG_REPORTMOUSE))
						break;

					// Save old end
					old_pos = data->sel_bottom_pos;
					old_line = data->sel_bottom_line;

					// Get end position
					data->sel_bottom_pos = (data->window->MouseX - data->text_box.MinX) / data->window->RPort->TxWidth;
					data->sel_bottom_line =
						(data->window->MouseY - data->text_box.MinY) / data->window->RPort->TxHeight;

					// Bounds check
					if (data->sel_bottom_pos < 0)
						data->sel_bottom_pos = 0;
					else if (data->sel_bottom_pos >= data->h_visible)
						data->sel_bottom_pos = data->h_visible - 1;
					if (data->sel_bottom_line < 0)
						data->sel_bottom_line = 0;
					else if (data->sel_bottom_line >= data->v_visible)
						data->sel_bottom_line = data->v_visible - 1;

					// Add top position
					data->sel_bottom_pos += data->left;
					data->sel_bottom_line += data->top;

					// Scrolling down?
					if (sel_dy == 1)
						++data->sel_bottom_line;

					// Scrolling up?
					else if (sel_dy == -1)
						--data->sel_bottom_line;

					// Scrolling right?
					else if (sel_dx == 1)
						++data->sel_bottom_pos;

					// Scrolling left?
					else if (sel_dx == -1)
						--data->sel_bottom_pos;

					// Has it changed?
					if (old_pos != data->sel_bottom_pos || old_line != data->sel_bottom_line)
					{
						// No scrolling?
						if (!sel_dx && !sel_dy)
						{
							// Redraw the whole screen
							read_show_text(data, 1, 1, 0);
						}
						else
						{
							// Display text
							read_update_text(data, sel_dx, sel_dy, 1);
						}

						// Set flag for ok
						data->flags |= READF_HIGH_OK;
					}
				}
				break;
				}

				// If window is gone, quit
				if (!data->window)
				{
					if (!data->app_icon)
						quit_flag = 1;
					break;
				}
			}
		}

		// Check quit flag
		if (quit_flag)
			break;

		// Wait for a message
		if ((Wait(((data->window) ? (1 << data->window->UserPort->mp_SigBit) : 0) |
				  ((data->appwindow || data->app_icon) ? (1 << data->appport->mp_SigBit) : 0) |
				  ((data->ipc) ? (1 << data->ipc->command_port->mp_SigBit) : 0) |
				  1 << data->scroll_timer->port->mp_SigBit)) &
			(1 << data->scroll_timer->port->mp_SigBit))
		{
			// Timer returned?
			if (CheckTimer(data->scroll_timer))
			{
				short dir = 0, y, h;
				long speed = 10000;

				// Check right button is still down
				if (InputBase && !(PeekQualifier() & IEQUALIFIER_RBUTTON))
				{
					// Stop scrolling
					data->scroll = 0;
					continue;
				}

				// Get mouse position and half window height
				y = data->window->MouseY;
				h = data->window->GZZHeight >> 1;

				// Get scroll direction and speed
				if (y < h - (h >> 1))
				{
					dir = -1;
				}
				else if (y < h - 20)
				{
					dir = -1;
					speed = 40000;
				}
				else if (y > h + (h >> 1))
				{
					dir = 1;
				}
				else if (y > h + 20)
				{
					dir = 1;
					speed = 40000;
				}

				// Scrolling?
				if (dir)
				{
					// Check ok to scroll
					if ((dir < 0 && data->top > 0) || (dir > 0 && data->top + data->v_visible < data->lines))
					{
						// Scroll it
						read_update_text(data, 0, dir, 0);
					}
				}

				// Restart timer
				StartTimer(data->scroll_timer, 0, speed);
			}
		}
	}

	return (BOOL)((quit_flag == 2) ? 0 : 1);
}

// Calculate window size and update scrollers
void read_calc_size(read_data *data)
{
	// Get text box size
	data->text_box.MinX = data->window->BorderLeft + H_EXTRA;
	data->text_box.MinY = data->window->BorderTop;
	data->text_box.MaxX = data->window->Width - data->window->BorderRight - 1 - H_EXTRA;
	data->text_box.MaxY = data->window->Height - data->window->BorderBottom - 1;

	// Get visible lines and columns
	data->h_visible = RECTWIDTH(&data->text_box) / data->window->RPort->TxWidth;
	data->v_visible = RECTHEIGHT(&data->text_box) / data->window->RPort->TxHeight;

	// Get division factor for slider
	data->slider_div = UDivMod32(data->lines, 65535) + 1;

	// Bounds-check top and left
	read_check_bounds(data);

	// Initialise scrollers
	SetGadgetAttrs(data->vert_scroller,
				   data->window,
				   0,
				   PGA_Top,
				   (data->slider_div > 1) ? UDivMod32(data->top, data->slider_div) : data->top,
				   PGA_Total,
				   (data->slider_div > 1) ? UDivMod32(data->lines, data->slider_div) : data->lines,
				   PGA_Visible,
				   (data->slider_div > 1) ? UDivMod32(data->v_visible, data->slider_div) : data->v_visible,
				   TAG_END);
	SetGadgetAttrs(data->horiz_scroller,
				   data->window,
				   0,
				   PGA_Top,
				   data->left,
				   PGA_Total,
				   data->columns,
				   PGA_Visible,
				   data->h_visible,
				   TAG_END);

	// Get display rectangle
	data->disprect.MinX = data->text_box.MinX;
	data->disprect.MinY = data->text_box.MinY;
	data->disprect.MaxX = (data->disprect.MinX + data->h_visible * data->window->RPort->TxWidth) - 1;
	data->disprect.MaxY = (data->disprect.MinY + data->v_visible * data->window->RPort->TxHeight) - 1;

	// Erase area outside of display
	if (data->disprect.MaxX < data->window->Width - data->window->BorderRight - 1)
	{
		EraseRect(data->window->RPort,
				  data->disprect.MaxX + 1,
				  data->disprect.MinY,
				  data->window->Width - data->window->BorderRight - 1,
				  data->window->Height - data->window->BorderBottom - 1);
	}
	if (data->disprect.MaxY < data->window->Height - data->window->BorderBottom - 1)
	{
		EraseRect(data->window->RPort,
				  data->disprect.MinX,
				  data->disprect.MaxY + 1,
				  data->window->Width - data->window->BorderRight - 1,
				  data->window->Height - data->window->BorderBottom - 1);
	}
}

// Initialise display
void read_init(read_data *data)
{
	// Get pen to use
	SetAPen(data->window->RPort, data->drawinfo->dri_Pens[TEXTPEN]);
	SetBPen(data->window->RPort, data->drawinfo->dri_Pens[BACKGROUNDPEN]);
	SetDrMd(data->window->RPort, JAM2);

	// Write mask
	if (data->mode == MODE_ANSI)
	{
		if (((struct Library *)GfxBase)->lib_Version >= 39)
			SetWriteMask(data->window->RPort, 0xffffffff);
		else
			SetWrMsk(data->window->RPort, 0xff);
	}
	else
	{
		if (((struct Library *)GfxBase)->lib_Version >= 39)
			SetMaxPen(data->window->RPort, data->drawinfo->dri_Pens[TEXTPEN]);
		else
			SetWrMsk(data->window->RPort, data->drawinfo->dri_Pens[TEXTPEN]);
	}

	// Get text width
	read_get_width(data);

	// Update scrollers
	read_calc_size(data);

	// Display text
	read_show_text(data, 0, 0, 0);

	// Display title
	read_build_title(data);
}

// Build title string
void read_build_title(read_data *data)
{
	struct MenuItem *item;
	short num;

	// If no text, no title
	if (!(IsListEmpty((struct List *)&data->text_data)))
	{
		// Display title
		snprintf(data->title,
				 sizeof(data->title),
				 GetString(locale, MSG_FILE_TITLE),
				 FilePart(data->file),
				 data->date,
				 (long)data->lines,
				 (long)data->size,
				 mode_string[data->mode],
				 (long)data->tab_size);
		SetWindowTitles(data->window, data->title, (char *)-1);
	}

	// Remove menu
	ClearMenuStrip(data->window);

	// Fix tab checkmarks
	for (num = 0; num < 4; num++)
	{
		// Get item
		if ((item = FindMenuItem(data->menus, MENU_TAB_1 + num)))
		{
			// Check or uncheck as appropriate
			if (data->tab_size == (1 << num))
				item->Flags |= CHECKED;
			else
				item->Flags &= ~CHECKED;
		}
	}

	// Fix mode checkmarks
	for (num = 0; num < 3; num++)
	{
		// Get item
		if ((item = FindMenuItem(data->menus, MENU_MODE_NORMAL + num)))
		{
			// Check or uncheck as appropriate
			if (data->mode == num)
				item->Flags |= CHECKED;
			else
				item->Flags &= ~CHECKED;
		}
	}

	// Fix use screen
	if ((item = FindMenuItem(data->menus, MENU_USE_SCREEN)))
	{
		// Check, etc
		if (data->my_screen)
			item->Flags |= CHECKED;
		else
			item->Flags &= ~CHECKED;
	}

	// If using ASL 38+, fix Pick screen mode
	if (AslBase->lib_Version >= 38)
	{
		if ((item = FindMenuItem(data->menus, MENU_SCREEN_MODE)))
		{
			if (data->my_screen)
				item->Flags |= ITEMENABLED;
			else
				item->Flags &= ~ITEMENABLED;
		}
	}

	// Enable/disable menus if there's no buffer
	if ((item = FindMenuItem(data->menus, MENU_SEARCH)))
	{
		if (!(IsListEmpty((struct List *)&data->text_data)))
			item->Flags |= ITEMENABLED;
		else
			item->Flags &= ~ITEMENABLED;
	}
	if ((item = FindMenuItem(data->menus, MENU_REPEAT)))
	{
		if (!(IsListEmpty((struct List *)&data->text_data)))
			item->Flags |= ITEMENABLED;
		else
			item->Flags &= ~ITEMENABLED;
	}
	if ((item = FindMenuItem(data->menus, MENU_PRINT)))
	{
		if (!(IsListEmpty((struct List *)&data->text_data)))
			item->Flags |= ITEMENABLED;
		else
			item->Flags &= ~ITEMENABLED;
	}
	if ((item = FindMenuItem(data->menus, MENU_SAVE_AS)))
	{
		if (!(IsListEmpty((struct List *)&data->text_data)))
			item->Flags |= ITEMENABLED;
		else
			item->Flags &= ~ITEMENABLED;
	}
	if ((item = FindMenuItem(data->menus, MENU_TO_EDITOR)))
	{
		if (!(IsListEmpty((struct List *)&data->text_data)))
			item->Flags |= ITEMENABLED;
		else
			item->Flags &= ~ITEMENABLED;
	}

	// Reattach menus
	ResetMenuStrip(data->window, data->menus);
}

// Update text and scroller display
void read_update_text(read_data *data, long dx, long dy, short show_two)
{
	// Display text
	read_show_text(data, dx, dy, show_two);

	// Update scrollers
	SetGadgetAttrs(data->vert_scroller,
				   data->window,
				   0,
				   PGA_Top,
				   (data->slider_div > 1) ? UDivMod32(data->top, data->slider_div) : data->top,
				   TAG_END);
	SetGadgetAttrs(data->horiz_scroller, data->window, 0, PGA_Top, data->left, TAG_END);
}

// Display text
void read_show_text(read_data *data, long dx, long dy, short show_two)
{
	short topline, botline;
	long old_left, old_top;
	long line;
	read_line *text_line = 0;
	BOOL redisplay = 0;
	text_chunk *chunk = 0;
	long chunk_offset = 0;

	// No lines?
	if (data->lines < 1)
		return;

	// Display all lines by default
	topline = 0;
	botline = data->v_visible - 1;

	// Save old positions
	old_left = data->left;
	old_top = data->top;

	// If dx and dy are both set, just redisplay without clearing
	if (dx && dy)
	{
		dx = dy = 0;
		redisplay = 1;
	}

	// Store new positions
	data->left += dx;
	data->top += dy;

	// Bounds-check
	read_check_bounds(data);

	// Get checked deltas
	dx = data->left - old_left;
	dy = data->top - old_top;

	// Scroll horizontally?
	if (dx && ABS(dx) < data->h_visible)
	{
		// Scroll
		ScrollRaster(data->window->RPort,
					 dx * data->window->RPort->TxWidth,
					 0,
					 data->disprect.MinX - H_EXTRA,
					 data->disprect.MinY,
					 data->disprect.MaxX + H_EXTRA,
					 data->disprect.MaxY);
	}

	// Scroll vertically
	else if (dy && ABS(dy) < data->v_visible)
	{
		// Scroll
		ScrollRaster(data->window->RPort,
					 0,
					 dy * data->window->RPort->TxHeight,
					 data->disprect.MinX - H_EXTRA,
					 data->disprect.MinY,
					 data->disprect.MaxX + H_EXTRA,
					 data->disprect.MaxY);

		// Scroll down?
		if (dy > 0)
		{
			// Display bottom of view
			topline = data->v_visible - dy - show_two;
			botline = data->v_visible - 1;

			// Get new top line
			if (data->top_line && data->mode != MODE_HEX)
			{
				while (dy > 0 && data->top_line->node.mln_Succ->mln_Succ)
				{
					data->top_line = (read_line *)data->top_line->node.mln_Succ;
					--dy;
				}
			}
		}

		// Scroll up
		else
		{
			// Display top of view
			topline = 0;
			botline = (-dy - 1) + show_two;

			// Get new top line
			if (data->top_line && data->mode != MODE_HEX)
			{
				while (dy < 0 && data->top_line->node.mln_Pred->mln_Pred)
				{
					data->top_line = (read_line *)data->top_line->node.mln_Pred;
					++dy;
				}
			}
		}
	}

	// Redraw full screen
	else if (!redisplay)
	{
		data->top_line = 0;
		EraseRect(data->window->RPort,
				  data->disprect.MinX - H_EXTRA,
				  data->disprect.MinY,
				  data->disprect.MaxX + H_EXTRA,
				  data->disprect.MaxY);
	}

	// In hex mode, get top position
	if (data->mode == MODE_HEX)
	{
		data->top_position = data->top * HEX_DISPLAY_WIDTH;
		chunk_offset = data->top_position;
		chunk = read_hex_pointer(data, &chunk_offset);
		if (chunk)
			text_line = (read_line *)(chunk->buffer + chunk_offset);
	}

	// Normal text mode
	else
	{
		// No valid top line?
		if (!data->top_line)
		{
			for (line = 0, text_line = (read_line *)data->text.mlh_Head; line < data->top && text_line->node.mln_Succ;
				 line++, text_line = (read_line *)text_line->node.mln_Succ)
				;
			data->top_line = text_line;
		}
		text_line = data->top_line;
	}

	// Display text
	for (line = 0; line < data->v_visible; line++)
	{
		// Ok to display?
		if (line >= topline && line <= botline)
		{
			// Display text
			read_display_text(data, text_line, line, chunk);

			// Search line?
			if (data->search_line)
			{
				// Hex?
				if (data->mode == MODE_HEX)
				{
					// Match line
					if (line + data->top == ((long)data->search_line) - 1)
					{
						// Show search text
						read_show_search(data, line);
					}
				}

				// Normal, match line
				else if (data->search_line == text_line)
				{
					// Show search text
					read_show_search(data, line);
				}
			}
		}

		// Get next line
		if (data->mode == MODE_HEX)
		{
			// Finished?
			if (line + data->top >= data->lines - 1)
				text_line = 0;

			// Next line
			else if (chunk)
			{
				// Increment offset
				chunk_offset += HEX_DISPLAY_WIDTH;

				// Exceeded chunk?
				if (chunk_offset >= chunk->size)
				{
					short overrun;

					// Get the amount of overrun
					overrun = chunk->size - 1 - (chunk_offset - HEX_DISPLAY_WIDTH);

					// Get next chunk
					chunk = (text_chunk *)chunk->node.mln_Succ;
					chunk_offset = HEX_DISPLAY_WIDTH - overrun - 1;
					if (chunk_offset < 0)
						chunk_offset = 0;

					// Invalid chunk?
					if (!chunk->node.mln_Succ)
						chunk = 0;
					else
						text_line = (read_line *)(chunk->buffer + chunk_offset);
				}
				else
					text_line = (read_line *)(((char *)text_line) + HEX_DISPLAY_WIDTH);
			}
		}

		// Next next line
		else if (text_line)
			text_line = (read_line *)text_line->node.mln_Succ;
	}
}

// Display a line of text
void read_display_text(read_data *data, read_line *text, long line, text_chunk *chunk)
{
	short c_x, cp, start, len, length;
	short y_pos, vispos = -1;
	short highlight = 0, highlight_start_pos = -1, highlight_end_pos = -1;
	short real_line = 0;
	char *textptr;
	unsigned char fpen = 1, bpen = 0, pen = 0;
	struct Rectangle high_box = {0};
	char text_buf[HEX_DISPLAY_WIDTH + 1];

	// Valid text?
	if (!text || (data->mode != MODE_HEX && (!text->node.mln_Succ || text->length < 1)))
		return;

	// Get y position
	y_pos = data->disprect.MinY + line * data->window->RPort->TxHeight;

	// Position for text
	Move(data->window->RPort, data->disprect.MinX, y_pos + data->window->RPort->TxBaseline);

	// Reset colour and style (if in ANSI mode)
	if (data->mode == MODE_ANSI)
	{
		// Get pens
		fpen = data->drawinfo->dri_Pens[TEXTPEN];
		bpen = data->drawinfo->dri_Pens[BACKGROUNDPEN];

		// Reset colour
		if (((struct Library *)GfxBase)->lib_Version >= 39)
		{
			SetABPenDrMd(data->window->RPort, fpen, bpen, JAM2);
		}
		else
		{
			SetAPen(data->window->RPort, fpen);
			SetBPen(data->window->RPort, bpen);
		}

		// Style (if necessary)
		if (data->style)
		{
			SetSoftStyle(data->window->RPort, 0, FSF_BOLD | FSF_ITALIC | FSF_UNDERLINED);
			data->style = 0;
		}
	}

	// Hex display?
	if (data->mode == MODE_HEX)
	{
		short cp;
		char *ptr;

		// Get text pointer and length
		textptr = (char *)text;
		if (textptr + HEX_DISPLAY_WIDTH > chunk->buffer + chunk->size)
		{
			// Get remaining length
			length = chunk->buffer + chunk->size - textptr;

			// Copy the remainder of this chunk to the buffer
			CopyMem(textptr, text_buf, length);

			// Get next chunk
			chunk = (text_chunk *)chunk->node.mln_Succ;
			if (chunk->node.mln_Succ)
			{
				short size;

				// Get extra data needed
				size = HEX_DISPLAY_WIDTH - length;
				if (size > chunk->size)
					size = chunk->size;

				// Copy data out of this chunk to fill buffer
				CopyMem(chunk->buffer, text_buf + length, size);
				length += size;
			}

			// Use pointer to temporary buffer
			textptr = text_buf;
		}
		else
			length = HEX_DISPLAY_WIDTH;

		// Start hex display
		snprintf(data->hex_display, sizeof(data->hex_display), "%08lx:", (unsigned long)(data->top + line) * HEX_DISPLAY_WIDTH);

		// Hex values
		ptr = data->hex_display + 9;
		for (cp = 0; cp < length; cp++)
		{
			// Space every four
			if (cp % 4 == 0)
				*ptr++ = ' ';
			snprintf(ptr, sizeof(data->hex_display) - (ptr - data->hex_display), "%02lx", (unsigned long)((unsigned char *)textptr)[cp]);
			ptr += 2;
		}

		// Fill with spaces
		for (; cp < HEX_DISPLAY_WIDTH; cp++)
		{
			// Extra space every four
			if (cp % 4 == 0)
				*ptr++ = ' ';
			*ptr++ = ' ';
			*ptr++ = ' ';
		}

		// Space
		*ptr++ = ' ';

		// ASCII characters
		for (cp = 0; cp < length; cp++)
		{
			unsigned char ch;

			// Non-printable?
			ch = textptr[cp];
			if (ch < 0x20 || (ch > 0x7e && ch < 0xc0))
				*ptr++ = '.';
			else
				*ptr++ = ch;
		}

		// Get length and real text pointer
		length = ptr - data->hex_display;
		textptr = data->hex_display;
	}
	else
	{
		// Normal text display
		length = text->length;
		textptr = text->text;

		// Highlight?
		if (data->sel_top_pos != -1)
		{
			// Get highlight box
			if (data->sel_bottom_line < data->sel_top_line)
			{
				high_box.MinX = data->sel_bottom_pos;
				high_box.MinY = data->sel_bottom_line;
				high_box.MaxX = data->sel_top_pos;
				high_box.MaxY = data->sel_top_line;
			}
			else if (data->sel_top_line < data->sel_bottom_line || data->sel_top_pos < data->sel_bottom_pos)
			{
				high_box.MinX = data->sel_top_pos;
				high_box.MinY = data->sel_top_line;
				high_box.MaxX = data->sel_bottom_pos;
				high_box.MaxY = data->sel_bottom_line;
			}
			else
			{
				high_box.MinX = data->sel_bottom_pos;
				high_box.MinY = data->sel_top_line;
				high_box.MaxX = data->sel_top_pos;
				high_box.MaxY = data->sel_bottom_line;
			}

			// Is this within the highlight area?
			real_line = line + data->top;
			if (real_line >= high_box.MinY && real_line <= high_box.MaxY)
			{
				// Flag highlight on
				highlight = 1;

				// First highlight line?
				if (real_line == high_box.MinY)
				{
					// Get position to highlight from
					highlight_start_pos = high_box.MinX;
				}

				// Last highlight line?
				if (real_line == high_box.MaxY)
				{
					// Get position to highlight to
					highlight_end_pos = high_box.MaxX + 1;
				}

				// Highlight from start of line?
				if (real_line != high_box.MinY)
				{
					// Set inverse mode
					SetDrMd(data->window->RPort, INVERSVID | JAM2);
					highlight = 2;
				}
			}
		}
	}

	// Go through text
	for (cp = 0, c_x = 0, start = 0, len = 0; cp <= length; cp++)
	{
		// Need to output existing text?
		if (textptr[cp] == '\t' || cp == length || textptr[cp] == '\x1b' || textptr[cp] == '\x9b' ||
			textptr[cp] == '\r' ||
			(highlight && ((real_line == high_box.MinY && c_x + len == highlight_start_pos) ||
						   (real_line == high_box.MaxY && c_x + len == highlight_end_pos))))
		{
			struct TextExtent extent;
			BOOL ansi = 0;

			// Make sure text is visible at left
			while (c_x < data->left && start < length)
			{
				// Tab?
				if (textptr[start] == '\t')
				{
					// Move text position to next tab stop
					c_x = ((c_x + data->tab_size) / data->tab_size) * data->tab_size;

					// Now in view?
					if (c_x > data->left)
					{
						// Move across to tab stop
						Move(data->window->RPort,
							 data->disprect.MinX + (c_x - data->left) * data->window->RPort->TxWidth,
							 data->window->RPort->cp_y);
					}
				}

				// Escape sequence?
				else if (textptr[start] == '\x1b' || textptr[start] == '\x9b')
					ansi = 1;

				// End of escape sequence?
				else if (ansi)
				{
					if ((textptr[start] >= 'A' && textptr[start] <= 'Z') ||
						(textptr[start] >= 'a' && textptr[start] <= 'z'))
						ansi = 0;
				}

				// Normal character
				else
				{
					++c_x;
					--len;
				}

				++start;
			}

			// First visible character?
			if (vispos == -1)
				vispos = start;

			// Any text?
			if (len > 0)
			{
				// Get length that will fit
				len = TextFit(data->window->RPort,
							  textptr + start,
							  len,
							  &extent,
							  0,
							  1,
							  (data->disprect.MaxX - data->window->RPort->cp_x) + 1,
							  data->window->RPort->TxHeight);

				// Output text
				Text(data->window->RPort, textptr + start, len);
				c_x += len;
			}

			// Restart
			len = 0;

			// Highlight?
			if (highlight)
			{
				// First line?
				if (real_line == high_box.MinY && highlight == 1 && c_x >= highlight_start_pos)
				{
					// ANSI mode?
					if (data->mode == MODE_ANSI)
					{
						// Save pens
						fpen = data->window->RPort->FgPen;
						bpen = data->window->RPort->BgPen;
						pen = 1;

						// Set pens
						SetAPen(data->window->RPort, data->drawinfo->dri_Pens[TEXTPEN]);
						SetBPen(data->window->RPort, data->drawinfo->dri_Pens[BACKGROUNDPEN]);

						// Clear styles
						SetSoftStyle(data->window->RPort, 0, FSF_BOLD | FSF_ITALIC | FSF_UNDERLINED);
					}

					// Turn highlighting on
					SetDrMd(data->window->RPort, INVERSVID | JAM2);
					highlight = 2;
				}

				// Last line?
				else if (real_line == high_box.MaxY && highlight == 2 && c_x >= highlight_end_pos)
				{
					// Turn it off
					SetDrMd(data->window->RPort, JAM2);
					highlight = -1;

					// Fix pens and style
					if (pen)
					{
						SetAPen(data->window->RPort, fpen);
						SetBPen(data->window->RPort, bpen);

						SetSoftStyle(data->window->RPort, data->style, FSF_BOLD | FSF_ITALIC | FSF_UNDERLINED);
					}
				}
			}
		}

		// Escape sequence?
		if (textptr[cp] == '\x1b' || textptr[cp] == '\x9b')
		{
			// In normal mode, skip to end of sequence
			if (data->mode == MODE_NORMAL)
			{
				// Look for end of sequence (or line)
				while (cp <= length &&
					   (textptr[cp] < 'A' || (textptr[cp] > 'Z' && textptr[cp] < 'a') || textptr[cp] > 'z'))
				{
					++cp;
				}
			}

			// In ansi mode, parse to end of sequence
			else if (data->mode == MODE_ANSI)
			{
				char num_buf[8];
				short num_pos = 0, num_vals[12], num_count = 0;

				// Look for end of sequence (or line)
				while (cp <= length)
				{
					// Numeric?
					if (textptr[cp] >= '0' && textptr[cp] <= '9')
					{
						num_buf[num_pos++] = textptr[cp];
					}

					// End of a number?
					if (textptr[cp] < '0' || textptr[cp] > '9' || num_pos == 7)
					{
						// Get value
						if (num_pos > 0)
						{
							num_buf[num_pos] = 0;
							if (num_count < 12)
								num_vals[num_count++] = atoi(num_buf);
							num_pos = 0;
						}
					}

					// End of sequence?
					if ((textptr[cp] >= 'A' && textptr[cp] <= 'Z') || (textptr[cp] >= 'a' && textptr[cp] <= 'z'))
						break;
					++cp;
				}

				// Only understand 'm' (SGR)
				if (textptr[cp] == 'm' && num_count > 0)
				{
					short a;
					long style = data->style;

					// Look at values
					for (a = 0; a < num_count; a++)
					{
						// Foreground colour?
						if (num_vals[a] >= 30 && num_vals[a] <= 37)
						{
							fpen = data->ansi_pen_array[num_vals[a] - 30];
							if (highlight == 2)
								pen = 1;
							else
								SetAPen(data->window->RPort, data->ansi_pen_array[num_vals[a] - 30]);
						}

						// Background colour?
						else if (num_vals[a] >= 40 && num_vals[a] <= 47)
						{
							bpen = data->ansi_pen_array[num_vals[a] - 40];
							if (highlight == 2)
								pen = 1;
							else
								SetBPen(data->window->RPort, data->ansi_pen_array[num_vals[a] - 40]);
						}

						// Style
						if (num_vals[a] == 0)
						{
							fpen = data->drawinfo->dri_Pens[TEXTPEN];
							bpen = data->drawinfo->dri_Pens[BACKGROUNDPEN];
							if (highlight == 2)
								pen = 1;
							else
							{
								SetAPen(data->window->RPort, data->drawinfo->dri_Pens[TEXTPEN]);
								SetBPen(data->window->RPort, data->drawinfo->dri_Pens[BACKGROUNDPEN]);
							}
							style = 0;
						}
						else if (num_vals[a] == 1)
							style |= FSF_BOLD;
						else if (num_vals[a] == 3)
							style |= FSF_ITALIC;
						else if (num_vals[a] == 4)
							style |= FSF_UNDERLINED;
						else if (num_vals[a] == 22)
							style &= ~FSF_BOLD;
						else if (num_vals[a] == 23)
							style &= ~FSF_ITALIC;
						else if (num_vals[a] == 24)
							style &= ~FSF_UNDERLINED;

						// Need to change style?
						if (style != data->style)
						{
							if (highlight == 2)
								pen = 1;
							else
								SetSoftStyle(data->window->RPort, style, FSF_BOLD | FSF_ITALIC | FSF_UNDERLINED);
							data->style = style;
						}
					}
				}
			}
		}

		// Visible?
		else if (textptr[cp] != '\r' && cp >= vispos)
		{
			// Tab?
			if (textptr[cp] == '\t')
			{
				short x;

				// Get next tab stop
				c_x = ((c_x + data->tab_size) / data->tab_size) * data->tab_size;

				// Get new position
				x = data->disprect.MinX + (c_x - data->left) * data->window->RPort->TxWidth;
				if (x >= data->disprect.MaxX)
					x = data->disprect.MaxX;

				// Move across to tab stop
				Move(data->window->RPort, x, data->window->RPort->cp_y);
			}

			// Normal character
			else
			{
				// First in a run?
				if (len == 0)
					start = cp;
				++len;
			}
		}

		// Reached end of display?
		if (c_x >= data->left + data->h_visible)
			break;
	}

	// Fix draw mode if we were highlighting
	if (highlight)
		SetDrMd(data->window->RPort, JAM2);
}

// Get maximum text width
void read_get_width(read_data *data)
{
	short max = 0;
	read_line *text;

	// Hex mode?
	if (data->mode == MODE_HEX)
	{
		max = 10 + (HEX_DISPLAY_WIDTH << 1) + (HEX_DISPLAY_WIDTH >> 2) + HEX_DISPLAY_WIDTH;
	}

	// Normal text
	else
	{
		// Go through text
		for (text = (read_line *)data->text.mlh_Head; text->node.mln_Succ; text = (read_line *)text->node.mln_Succ)
		{
			short c_x, len;
			char *ptr;

			// Go through line
			for (len = 0, c_x = 0, ptr = text->text; len < text->length; len++, ptr++)
			{
				// Tab?
				if (*ptr == '\t')
				{
					// Move to next tab stop
					c_x = ((c_x + data->tab_size) / data->tab_size) * data->tab_size;
				}

				// Ansi sequence?
				else if (*ptr == '\x1b' || *ptr == '\x9b')
				{
					// Look for end of sequence (or line)
					while (len <= text->length && (*ptr < 'A' || (*ptr > 'Z' && *ptr < 'a') || *ptr > 'z'))
					{
						++len;
						++ptr;
					}
				}

				// Normal character
				else
					++c_x;
			}

			// Longest line yet?
			if (c_x > max)
				max = c_x;
		}
	}

	// Store text width
	data->columns = max;
}

// Search
void read_search(read_data *data, BOOL prompt)
{
	read_line *line = 0;
	long linepos;
	BOOL found = 0;

	// Set busy pointer in parent
	SetBusyPointer(data->window);

	// Ask for search string?
	if (prompt || !data->search_text[0])
	{
		NewConfigWindow newwin;
		struct Window *window;
		ObjectList *objlist;
		BOOL cancel = 1, reset = 1;

		// Fill out new window
		newwin.parent = data->window;
		newwin.dims = &search_window;
		newwin.title = GetString(locale, MSG_MENU_SEARCH);
		newwin.locale = locale;
		newwin.port = 0;
		newwin.flags = WINDOW_VISITOR | WINDOW_AUTO_KEYS | WINDOW_REQ_FILL;
		newwin.font = 0;

		// Open window
		if (!(window = OpenConfigWindow(&newwin)) || !(objlist = AddObjectList(window, search_objects)))
		{
			CloseConfigWindow(window);
			ClearPointer(data->window);
			return;
		}

		// Initial settings
		SetGadgetValue(objlist, GAD_SEARCH_TEXT, (IPTR)data->search_text);
		SetGadgetValue(objlist, GAD_SEARCH_CASE, data->search_flags & SEARCHF_NOCASE);
		SetGadgetValue(objlist, GAD_SEARCH_WILD, data->search_flags & SEARCHF_WILDCARD);
		SetGadgetValue(objlist, GAD_SEARCH_ONLYWORD, data->search_flags & SEARCHF_ONLYWORDS);
		SetGadgetValue(objlist, GAD_SEARCH_REVERSE, data->search_flags & SEARCHF_REVERSE);

		// Activate text field
		ActivateStrGad(GADGET(GetObject(objlist, GAD_SEARCH_TEXT)), window);

		// Event loop
		FOREVER
		{
			struct IntuiMessage *msg;
			BOOL break_flag = 0;

			// Any Intuition messages?
			while ((msg = GetWindowMsg(window->UserPort)))
			{
				struct IntuiMessage copy_msg;

				// Copy message and reply
				copy_msg = *msg;
				ReplyWindowMsg(msg);

				// Gadget?
				if (copy_msg.Class == IDCMP_GADGETUP)
				{
					// Look at gadget ID
					switch (((struct Gadget *)copy_msg.IAddress)->GadgetID)
					{
					// Okay
					case GAD_SEARCH_TEXT:
					case GAD_SEARCH_OKAY:

						// Store flags
						data->search_flags = 0;
						if (GetGadgetValue(objlist, GAD_SEARCH_CASE))
							data->search_flags |= SEARCHF_NOCASE;
						if (GetGadgetValue(objlist, GAD_SEARCH_WILD))
							data->search_flags |= SEARCHF_WILDCARD;
						if (GetGadgetValue(objlist, GAD_SEARCH_ONLYWORD))
							data->search_flags |= SEARCHF_ONLYWORDS;
						if (GetGadgetValue(objlist, GAD_SEARCH_REVERSE))
							data->search_flags |= SEARCHF_REVERSE;

						// Get search text
						if (strcmp(data->search_text, (char *)GetGadgetValue(objlist, GAD_SEARCH_TEXT)))
							strcpy(data->search_text, (char *)GetGadgetValue(objlist, GAD_SEARCH_TEXT));
						else
							reset = 0;
						cancel = 0;

					// Cancel
					case GAD_SEARCH_CANCEL:
						break_flag = 1;
						break;
					}
				}

				// Close window?
				else if (copy_msg.Class == IDCMP_CLOSEWINDOW)
					break_flag = 1;
			}

			// Check break flag
			if (break_flag)
				break;

			// Wait for an event
			Wait(1 << window->UserPort->mp_SigBit);
		}

		// Close window
		CloseConfigWindow(window);

		// Cancelled?
		if (cancel || !data->search_text[0])
		{
			// Clear busy pointer
			ClearPointer(data->window);
			return;
		}

		// No current selection if text changed
		if (reset)
		{
			data->search_line = 0;
			data->search_char = 0;
		}
	}

	// Get line to search from
	if (data->search_line)
	{
		read_line *test;

		// Old search line
		line = data->search_line;

		// Hex
		if (data->mode == MODE_HEX)
		{
			// See if it's visible
			if ((long)line < data->top_position &&
				(long)line >= data->top_position + (data->v_visible * HEX_DISPLAY_WIDTH))
				line = 0;
		}

		// Normal text
		else
		{
			// See if it's visible
			for (test = data->top_line, linepos = 0; test != line && linepos < data->v_visible - 1;
				 test = (read_line *)test->node.mln_Succ, linepos++)
				;

			// Not visible?
			if (test != line)
				line = 0;
		}
	}

	// Start from top/bottom
	if (!line)
	{
		// Reverse; search from bottom
		if (data->search_flags & SEARCHF_REVERSE)
		{
			// Hex?
			if (data->mode == MODE_HEX)
				line = (read_line *)((data->lines - 1) * HEX_DISPLAY_WIDTH);

			// Normal text
			else
			{
				for (line = data->top_line, linepos = 0;
					 line->node.mln_Succ && line->node.mln_Succ->mln_Succ && linepos < data->v_visible - 1;
					 line = (read_line *)line->node.mln_Succ, linepos++)
					;
			}
		}

		// Search from top
		else if (data->mode != MODE_HEX)
			line = data->top_line;
	}

	// Hex search?
	if (data->mode == MODE_HEX)
	{
		char text_buf[HEX_DISPLAY_WIDTH + 1];
		text_chunk *chunk;
		long offset, line_num;

		// Get starting position
		line_num = (long)line;
		offset = line_num * HEX_DISPLAY_WIDTH;
		chunk = read_hex_pointer(data, &offset);

		// Search through file
		while (line_num >= 0 && line_num < data->lines && chunk)
		{
			long size;

			// Get size to copy
			size = chunk->size - offset;
			if (size > HEX_DISPLAY_WIDTH)
				size = HEX_DISPLAY_WIDTH;

			// Copy to buffer
			CopyMem(chunk->buffer + offset, text_buf, size);
			text_buf[size] = 0;

			// End of chunk going backwards?
			if (data->search_flags & SEARCHF_REVERSE)
			{
				if (offset < 1)
				{
					// Get previous chunk
					chunk = (text_chunk *)chunk->node.mln_Pred;

					// Valid?
					if (chunk->node.mln_Pred)
					{
						// New offset
						offset = chunk->size - HEX_DISPLAY_WIDTH;
					}
					else
						chunk = 0;
				}
			}

			// End of chunk going forwards?
			else if (chunk->size - offset <= HEX_DISPLAY_WIDTH)
			{
				// Get next chunk
				chunk = (text_chunk *)chunk->node.mln_Succ;

				// Valid?
				if (chunk->node.mln_Succ)
				{
					short more;

					// New offset
					offset = 0;

					// More to copy?
					more = HEX_DISPLAY_WIDTH - size;
					if (more > 0)
					{
						// Copy it
						CopyMem(chunk->buffer, text_buf + size, more);
						text_buf[size + more] = 0;
						offset = more;
					}
				}
				else
					chunk = 0;
			}

			// Still more
			else
			{
				// Increment offset
				if (data->search_flags & SEARCHF_REVERSE)
					offset -= HEX_DISPLAY_WIDTH;
				else
					offset += HEX_DISPLAY_WIDTH;
			}

			// Search this line
			if (read_search_line(
					data, (read_line *)text_buf, (line_num == (long)data->search_line) ? data->search_char + 1 : 0))
			{
				// Found
				data->search_line = (read_line *)(line_num + 1);
				found = 1;
				break;
			}

			// Increment line
			if (data->search_flags & SEARCHF_REVERSE)
				--line_num;
			else
				++line_num;
		}
	}

	// Normal text
	else
	{
		// Go through lines
		while (line->node.mln_Succ && line->node.mln_Pred)
		{
			// Search this line
			if (read_search_line(data, line, (line == data->search_line) ? data->search_char + 1 : 0))
			{
				// Found
				found = 1;
				break;
			}

			// Next line
			if (data->search_flags & SEARCHF_REVERSE)
				line = (read_line *)line->node.mln_Pred;
			else
				line = (read_line *)line->node.mln_Succ;
		}
	}

	// Remove current highlight
	read_remove_highlight(data);

	// Not found?
	if (!found)
	{
		// Display requester
		SimpleRequest(data->window,
					  GetString(locale, MSG_MENU_SEARCH),
					  GetString(locale, MSG_OK_BUTTON),
					  GetString(locale, MSG_STRING_NOT_FOUND),
					  0,
					  0,
					  0,
					  0);

		// No current selection
		data->search_line = 0;
		data->search_char = 0;
	}

	// Found
	else
	{
		// Get line match is on
		if (data->mode == MODE_HEX)
		{
			linepos = ((long)data->search_line) - 1;
		}
		else
		{
			for (linepos = 0, line = (read_line *)data->text.mlh_Head; line != data->search_line && line->node.mln_Succ;
				 ++linepos, line = (read_line *)line->node.mln_Succ)
				;
		}

		// Is line off-screen?
		if (linepos < data->top)
			read_update_text(data, 0, linepos - data->top, 0);
		else if (linepos >= data->top + data->v_visible)
			read_update_text(data, 0, linepos + 1 - (data->top + data->v_visible), 0);

		// No, highlight text
		else
			read_show_search(data, linepos - data->top);
	}

	// Clear busy pointer
	ClearPointer(data->window);
}

// Search a line of text
BOOL read_search_line(read_data *data, read_line *line, short start)
{
	short matchstart = -1, matchpos = 0;
	short length;
	char *textptr;

	// Get length and text pointer
	if (data->mode == MODE_HEX)
	{
		textptr = (char *)line;
		length = strlen(textptr);
	}
	else
	{
		textptr = line->text;
		length = line->length;
	}

	// Search line
	for (; start < length; start++)
	{
		BOOL match = 0;

		// Match?
		if (data->search_flags & SEARCHF_WILDCARD && data->search_text[matchpos] == '?')
			match = 1;
		else if (data->search_flags & SEARCHF_NOCASE)
		{
			if (toupper(textptr[start]) == toupper(data->search_text[matchpos]))
				match = 1;
		}
		else if (textptr[start] == data->search_text[matchpos])
			match = 1;

		if (match)
		{
			// Start of match?
			if (matchstart == -1)
				matchstart = start;

			// Increment match position
			++matchpos;

			// End of match?
			if (data->search_text[matchpos] == 0)
			{
				// Only words?
				if (data->search_flags & SEARCHF_ONLYWORDS)
				{
					match = 0;

					// Character before match string must be valid (or start of line)
					if (matchstart == 0 || isspace(textptr[matchstart - 1]) || ispunct(textptr[matchstart - 1]))
					{
						// Character after match string must be likewise
						if (matchstart + matchpos > length || isspace(textptr[matchstart + matchpos]) ||
							ispunct(textptr[matchstart + matchpos]))
							match = 1;
					}
				}

				// Still valid match?
				if (match)
				{
					// Store match location
					data->search_line = line;
					data->search_char = matchstart;
					data->search_len = matchpos;
					return 1;
				}

				// Failed only-words test, reset
				matchstart = -1;
				matchpos = 0;
			}
		}

		// No match
		else if (matchpos)
		{
			// Start match again on current character
			matchstart = -1;
			matchpos = 0;
			--start;
		}
	}

	// No match
	return 0;
}

// Show search text
void read_show_search(read_data *data, long line)
{
	short c_x, c_xend, cp;
	short length, xoff;
	char *textptr;
	APTR line_ptr;
	text_chunk *chunk = 0;

	// Get length and text pointer
	if (data->mode == MODE_HEX)
	{
		long offset;

		// Get chunk
		offset = (((long)data->search_line) - 1) * HEX_DISPLAY_WIDTH;
		if ((chunk = read_hex_pointer(data, &offset)))
		{
			// Get text pointer
			textptr = chunk->buffer + offset;
			length = HEX_DISPLAY_WIDTH;
		}
		else
			return;

		// Get line pointer
		line_ptr = textptr;
	}
	else
	{
		textptr = data->search_line->text;
		length = data->search_line->length;
		line_ptr = data->search_line;
	}

	// Find horiztonal position of search text
	for (cp = 0, c_x = 0; cp < data->search_char && cp < length; cp++)
	{
		// Not hex mode?
		if (data->mode != MODE_HEX)
		{
			// Tab?
			if (textptr[cp] == '\t')
			{
				// Move to next tab stop
				c_x = ((c_x + data->tab_size) / data->tab_size) * data->tab_size;
			}

			// Ansi sequence
			else if (textptr[cp] == '\x1b' || textptr[cp] == '\x9b')
			{
				// Look for end of sequence (or line)
				while (cp <= length &&
					   (textptr[cp] < 'A' || (textptr[cp] > 'Z' && textptr[cp] < 'a') || textptr[cp] > 'z'))
				{
					++cp;
				}
			}

			// Normal character
			else
				++c_x;
		}

		// Normal character
		else
			++c_x;
	}

	// Get end of search text
	for (c_xend = c_x; cp < data->search_char + data->search_len - 1 && cp < length; cp++)
	{
		// Not hex mode?
		if (data->mode != MODE_HEX)
		{
			// Tab?
			if (textptr[cp] == '\t')
			{
				// Move to next tab stop
				c_xend = ((c_xend + data->tab_size) / data->tab_size) * data->tab_size;
			}

			// Ansi sequence
			else if (textptr[cp] == '\x1b' || textptr[cp] == '\x9b')
			{
				// Look for end of sequence (or line)
				while (cp <= length &&
					   (textptr[cp] < 'A' || (textptr[cp] > 'Z' && textptr[cp] < 'a') || textptr[cp] > 'z'))
				{
					++cp;
				}
			}

			// Normal character
			else
				++c_xend;
		}

		// Normal character
		else
			++c_xend;
	}

	// Save for posterity's sake
	data->search_last_line = line_ptr;
	data->search_last_linepos = data->top + line;
	data->search_last_chunk = chunk;
	data->search_last_c_x = c_x;
	data->search_last_c_xend = c_xend;

	// Off screen completely?
	if (c_x > data->left + data->h_visible || c_xend < data->left)
		return;

	// Adjust position for left scroll
	c_x -= data->left;
	c_xend -= data->left;

	// Bounds check
	if (c_x < 0)
		c_x = 0;
	if (c_xend >= data->h_visible)
		c_xend = data->h_visible - 1;

	// Get x-offset for hex
	if (data->mode == MODE_HEX)
	{
		xoff = (46 - data->left) * data->window->RPort->TxWidth;
	}
	else
		xoff = 0;

	// Highlight text
	ClipBlit(data->window->RPort,
			 0,
			 0,
			 data->window->RPort,
			 data->disprect.MinX + c_x * data->window->RPort->TxWidth + xoff,
			 data->disprect.MinY + line * data->window->RPort->TxHeight,
			 ((c_xend - c_x) + 1) * data->window->RPort->TxWidth,
			 data->window->RPort->TxHeight,
			 0x50);
}

// Remove search highlight
void read_remove_highlight(read_data *data)
{
	// Valid line?
	if (data->search_last_line)
	{
		// Is line visible?
		if (data->search_last_linepos >= data->top && data->search_last_linepos < data->top + data->v_visible)
		{
			// Show line
			read_display_text(
				data, data->search_last_line, data->search_last_linepos - data->top, data->search_last_chunk);
		}
		data->search_last_line = 0;
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

// Parse string out of string
void read_parse_string(char **ptr, char *buffer, short size)
{
	// Skip joiner
	if (**ptr == '|')
		++*ptr;

	// Parse string
	while (**ptr && **ptr != '|')
	{
		// Copy to buffer
		if (size > 1)
		{
			*buffer++ = **ptr;
			--size;
		}

		// Increment pointer
		++*ptr;
	}

	// Null-terminate buffer
	*buffer = 0;
}

// Pick a screen mode to use
BOOL read_pick_screen_mode(read_data *data)
{
	APTR request;

	// Allocate an ASL request
	if (!(request = AllocAslRequestTags(ASL_ScreenModeRequest,
										ASLSM_Window,
										data->window,
										ASLSM_SleepWindow,
										TRUE,
										ASLSM_InitialDisplayID,
										GetVPModeID(&data->window->WScreen->ViewPort),
										ASLSM_MinWidth,
										640,
										ASLSM_MinDepth,
										2,
										TAG_END)))
		return 1;

	// Show requester
	if (!(AslRequest(request, 0)))
	{
		FreeAslRequest(request);
		return 1;
	}

	// Get mode ID
	modeid = ((struct ScreenModeRequester *)request)->sm_DisplayID;

	// Free requester
	FreeAslRequest(request);

	// Close window
	read_close_window(data);

	// Re-open window
	if (read_open_window(data))
	{
		// Redraw text
		read_set_mode(data, data->mode, TRUE);
		read_init(data);
		return 1;
	}

	// Failed
	return 0;
}

// Pick a font to use
void read_pick_font(read_data *data)
{
	APTR request;

	// Allocate an ASL request
	if (!(request = AllocAslRequestTags(ASL_FontRequest,
										ASLFO_Window,
										data->window,
										ASLFO_SleepWindow,
										TRUE,
										ASLFO_InitialName,
										(fontname[0]) ? fontname + 1 : "",
										ASLFO_InitialSize,
										(fontname[0]) ? fontsize : 8,
										ASLFO_FixedWidthOnly,
										TRUE,
										TAG_END)))
		return;

	// Show requester
	if (!(AslRequest(request, 0)))
	{
		FreeAslRequest(request);
		return;
	}

	// Free existing font
	SetBusyPointer(data->window);
	if (data->font)
	{
		SetFont(data->window->RPort, ((struct GfxBase *)GfxBase)->DefaultFont);
		CloseFont(data->font);
	}

	// Get new font
	if ((data->font = OpenDiskFont(&((struct FontRequester *)request)->fo_Attr)))
		SetFont(data->window->RPort, data->font);

	// Store font information
	fontname[0] = '0';
	strcpy(fontname + 1, ((struct FontRequester *)request)->fo_Attr.ta_Name);
	fontsize = ((struct FontRequester *)request)->fo_Attr.ta_YSize;

	// Free requester
	FreeAslRequest(request);

	// Redraw text
	read_init(data);
	ClearPointer(data->window);
}

// Allocate a line node
read_line ASM *read_alloc_node(REG(a0, read_data *data))
{
	register read_line *line;

	// Current line block?
	if (data->current_block)
	{
		// Get current line
		line = &data->current_block->lines[data->current_line++];

		// Time to get a new block?
		if (data->current_line >= LINES_PER_BLOCK)
			data->current_block = 0;
	}

	// No block
	else
	{
		// Try and allocate a block
		if ((data->current_block = AllocMemH(data->memory, sizeof(read_block))))
		{
			// Send back first line
			line = &data->current_block->lines[0];
			data->current_line = 1;
		}

		// Block alloc failed; allocate a single line entry
		else
			line = AllocMemH(data->memory, sizeof(read_line));
	}

	// Return line pointer
	return line;
}

// Print
void read_print(read_data *data)
{
	struct Library *ModuleBase = NULL;
#ifdef __amigaos4__
	struct ModuleIFace *IModule = NULL;
#endif

	// Set busy pointer
	SetBusyPointer(data->window);

	// Get print module
	if ((ModuleBase = OpenLibrary("DOpus5:Modules/print.module", LIB_VERSION)) && GETINTERFACE(IModule, ModuleBase))
	{
		struct List list;
		struct Node node;

		// Build list for this file
		NewList(&list);
		node.ln_Name = data->file;
		AddTail(&list, &node);

		// Print file
		Module_Entry(&list, data->window->WScreen, data->ipc, data->main_ipc, 0, 0);

// Close library
#ifdef __amigaos4__
		DropInterface((struct Interface *)IModule);
#endif
		CloseLibrary(ModuleBase);
	}
	else
		CloseLibrary(ModuleBase);

	// Clear busy pointer
	ClearPointer(data->window);
}

// Copy highlighted area to clipboard
void read_clipboard_copy(read_data *data)
{
	read_line *line;
	short cp, pos;
	long l;
	APTR iff;

	// Got a selection?
	if (data->sel_top_pos == -1)
		return;

	// Open clipboard for IFF
	if (!(iff = IFFOpen(0, IFF_CLIP_WRITE, ID_FTXT)))
		return;

	// Set busy pointer
	SetBusyPointer(data->window);

	// Push text chunk
	if (IFFPushChunk(iff, ID_CHRS))
	{
		// Get top selection line
		for (l = 0, line = (read_line *)data->text.mlh_Head; l < data->sel_top_line && line->node.mln_Succ;
			 l++, line = (read_line *)line->node.mln_Succ)
			;

		// Go through lines
		for (; l <= data->sel_bottom_line && line->node.mln_Succ; l++, line = (read_line *)line->node.mln_Succ)
		{
			char *ptr;

			// Build text line
			for (cp = 0, pos = 0; cp <= line->length; cp++)
			{
				// Escape sequence?
				if (line->text[cp] == '\x1b' || line->text[cp] == '\x9b')
				{
					// Skip to end of sequence
					while (cp <= line->length &&
						   (line->text[cp] < 'A' || (line->text[cp] > 'Z' && line->text[cp] < 'a') ||
							line->text[cp] > 'z'))
						++cp;
				}

				// Normal character
				else if (line->text[cp] != '\r')
				{
					// Add to buffer
					data->line_buffer[pos++] = line->text[cp];
				}
			}

			// Get string pointer
			ptr = data->line_buffer;

			// Last line?
			if (l == data->sel_bottom_line)
			{
				short bottom, bot;

				// Calculate real bottom position including tabs
				for (bot = 0, bottom = 0; bot < data->sel_bottom_pos; bottom++)
				{
					// Increment fake position for loop
					if (line->text[bottom] == '\t')
					{
						// Increment for tab
						bot = ((bot + data->tab_size) / data->tab_size) * data->tab_size;
					}
					else
						bot++;
				}

				// Adjust for bottom?
				if (pos > bottom)
					pos -= pos - bottom - 1;
			}

			// First line?
			if (l == data->sel_top_line)
			{
				short top, t;

				// Calculate real top position including tabs
				for (t = 0, top = 0; t < data->sel_top_pos; top++)
				{
					// Increment fake position for loop
					if (line->text[top] == '\t')
					{
						// Increment for tab
						t = ((t + data->tab_size) / data->tab_size) * data->tab_size;
					}
					else
						t++;
				}

				// Adjust for top
				pos -= top;
				ptr += top;
			}

			// Write data
			if (pos > 0)
			{
				if (!(IFFWriteChunkBytes(iff, ptr, pos)))
					break;
			}
		}

		// Pop the chunk
		IFFPopChunk(iff);
	}

	// Close clipboard file
	IFFClose(iff);

	// Clear busy pointer
	ClearPointer(data->window);
}

// Save As
void read_save(read_data *data)
{
	struct TagItem tags[4];
	char *ptr;
	APTR file;
	long error = 0;
	struct Message *msg;

	// Fill out tags
	tags[0].ti_Tag = ASLFR_Window;
	tags[0].ti_Data = (IPTR)data->window;
	tags[1].ti_Tag = ASLFR_InitialDrawer;
	tags[1].ti_Data = (IPTR)data->line_buffer;
	tags[2].ti_Tag = ASLFR_InitialFile;
	tags[2].ti_Data = (IPTR)FilePart(data->file);
	tags[3].ti_Tag = TAG_DONE;

	// Get initial drawer
	strcpy(data->line_buffer, data->file);
	if ((ptr = FilePart(data->line_buffer)))
		*ptr = 0;

	// Show filerequester
	if (!data->filereq || !(AslRequest(data->filereq, tags)))
		return;

	// Display busy pointer
	SetBusyPointer(data->window);

	// Build filename
	strcpy(data->line_buffer, data->filereq->fr_Drawer);
	AddPart(data->line_buffer, data->filereq->fr_File, 256);

	// Open file
	if ((file = OpenBuf(data->line_buffer, MODE_NEWFILE, 4096)))
	{
		read_line *line;

		// Go through text
		for (line = (read_line *)data->text.mlh_Head; line->node.mln_Succ; line = (read_line *)line->node.mln_Succ)
		{
			// Output line contents
			if ((WriteBuf(file, line->text, line->length)) != line->length || (WriteBuf(file, "\n", 1)) != 1)
			{
				error = IoErr();
				break;
			}
		}

		// Close file
		CloseBuf(file);
	}
	else
		error = IoErr();

	// Error?
	if (error)
	{
		// Build error text
		Fault(error, GetString(locale, MSG_ERROR_CODE), data->line_buffer, 256);

		// Show requester
		SimpleRequest(data->window,
					  GetString(locale, MSG_IO_ERROR),
					  GetString(locale, MSG_OK_BUTTON),
					  data->line_buffer,
					  0,
					  (APTR)((ULONG *)&error),
					  0,
					  0);
	}

	// Clear pointer
	ClearPointer(data->window);

	// Flush any queued messages
	while ((msg = GetMsg(data->window->UserPort)))
		ReplyMsg(msg);
}

// Pick editor
void read_pick_editor(read_data *data, short which)
{
	struct Message *msg;

	// Display busy pointer
	SetBusyPointer(data->window);

	// Show requester
	SimpleRequest(data->window,
				  GetString(locale, MSG_MENU_SELECT_EDITOR),
				  GetString(locale, MSG_OK_CANCEL_BUTTON),
				  GetString(locale, MSG_SELECT_EDITOR),
				  editor[which],
				  0,
				  79,
				  0);

	// Clear pointer
	ClearPointer(data->window);

	// Flush any queued messages
	while ((msg = GetMsg(data->window->UserPort)))
		ReplyMsg(msg);
}

// Bounds-checking
void read_check_bounds(read_data *data)
{
	// Bounds check horizontal
	if (data->left + data->h_visible > data->columns)
		data->left = data->columns - data->h_visible;
	if (data->left < 0)
		data->left = 0;

	// Bounds check vertical
	if (data->top + data->v_visible > data->lines)
	{
		data->top = data->lines - data->v_visible;
		data->top_line = 0;
	}
	if (data->top < 0)
	{
		data->top = 0;
		data->top_line = 0;
	}
}

// Return a pointer to the chunk containing a byte offset
text_chunk *read_hex_pointer(read_data *data, long *offset)
{
	text_chunk *chunk;

	// Go through text chunks
	for (chunk = (text_chunk *)data->text_data.mlh_Head; chunk->node.mln_Succ;
		 chunk = (text_chunk *)chunk->node.mln_Succ)
	{
		// Is offset within this chunk?
		if (*offset < chunk->size)
			return chunk;

		// Subtract size from offset
		*offset -= chunk->size;
	}

	// Not found
	return 0;
}
