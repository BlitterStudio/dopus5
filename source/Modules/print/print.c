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

#include "print.h"

static size_t print_bounded_strlen(char *str, size_t max)
{
	size_t len;

	for (len = 0; len < max && str[len]; len++)
		;

	return len;
}

static void print_copy_string(char *dest, const char *src, size_t dest_size)
{
	size_t pos;

	if (!dest || dest_size == 0)
		return;
	if (!src)
		src = "";

	for (pos = 0; pos < dest_size - 1 && src[pos]; pos++)
		dest[pos] = src[pos];
	dest[pos] = 0;
}

static void print_append_string(char *dest, const char *src, size_t dest_size)
{
	size_t pos;

	if (!dest || dest_size == 0)
		return;

	pos = print_bounded_strlen(dest, dest_size);
	if (pos >= dest_size)
	{
		dest[dest_size - 1] = 0;
		return;
	}

	print_copy_string(dest + pos, src, dest_size - pos);
}

int LIBFUNC L_Module_Entry(REG(a0, struct List *files),
						   REG(a1, struct Screen *screen),
						   REG(a2, IPCData *ipc),
						   REG(a3, IPCData *main_ipc),
						   REG(d0, IPTR mod_id),
						   REG(d1, IPTR mod_data))
{
	print_data *data;
	short break_flag = 0;

	// Allocate data
	if (!(data = AllocVec(sizeof(print_data), MEMF_CLEAR)))
		return 0;

	// Store IPC, files and screen pointer
	data->files = files;
	data->ipc = ipc;
	data->main_ipc = main_ipc;
	data->screen = screen;

	// Allocate signal bit
	data->abort_bit = AllocSignal(-1);

	// Allocate file requester
	data->filereq = AllocAslRequest(ASL_FileRequest, 0);

	// Get preferences
	GetPrefs(&data->prefs, sizeof(struct Preferences));

	// Initialise settings
	data->print_quality = (data->prefs.PrintQuality == LETTER) ? PRINT_QUALITY_LETTER : PRINT_QUALITY_DRAFT;
	data->print_spacing = (data->prefs.PrintSpacing == SIX_LPI) ? PRINT_SPACING_6 : PRINT_SPACING_8;
	data->print_pitch = (data->prefs.PrintPitch == PICA)
							? PRINT_PITCH_PICA
							: ((data->prefs.PrintPitch == ELITE) ? PRINT_PITCH_ELITE : PRINT_PITCH_FINE);
	data->left_margin = data->prefs.PrintLeftMargin;
	data->right_margin = data->prefs.PrintRightMargin;
	data->page_length = data->prefs.PaperLength;
	data->tab_size = 8;
	data->output_type = 0;

	// See if environment variable is set
	GetVar("dopus/Print", (char *)&data->print_quality, DATA_SIZE, GVF_BINARY_VAR | GVF_DONT_NULL_TERM);

	// Open window
	if (!(print_open(data)))
	{
		print_free(data);
		return 0;
	}

	// Event loop
	FOREVER
	{
		struct IntuiMessage *msg;
		IPCMessage *imsg;

		// IPC messages?
		if (ipc)
		{
			while ((imsg = (IPCMessage *)GetMsg(ipc->command_port)))
			{
				// Abort?
				if (imsg->command == IPC_ABORT || imsg->command == IPC_QUIT)
					break_flag = 1;
				IPC_Reply(imsg);
			}
		}

		// Any Intuition messages?
		if (data->window)
		{
			while ((msg = GetWindowMsg(data->window->UserPort)))
			{
				struct IntuiMessage copy_msg;

				// Copy message and reply
				copy_msg = *msg;
				ReplyWindowMsg(msg);

				// Look at message
				switch (copy_msg.Class)
				{
				// Close window
				case IDCMP_CLOSEWINDOW:
					break_flag = 1;
					break;

				// Gadget
				case IDCMP_GADGETUP:
					switch (((struct Gadget *)copy_msg.IAddress)->GadgetID)
					{
					// Header/footer
					case GAD_PRINT_HEADER_FOOTER:

						// Get old gadget values, update with new values
						print_get_header(data, 1 - copy_msg.Code);
						print_fix_header(data);
						break;

					// Title
					case GAD_PRINT_TITLE:

						// En/disable title string
						DisableObject(data->objlist, GAD_PRINT_TITLE_STRING, 1 - copy_msg.Code);
						break;

					// Cancel
					case GAD_PRINT_CANCEL:
						break_flag = 1;
						break;

					// Print
					case GAD_PRINT_OKAY:
						break_flag = 2;
						break;
					}
					break;

				// Key press
				case IDCMP_RAWKEY:

					// Help?
					if (copy_msg.Code == 0x5f && !(copy_msg.Qualifier & VALID_QUALIFIERS))
					{
						// Valid main IPC?
						if (data->main_ipc)
						{
							// Set busy pointer
							SetWindowBusy(data->window);

							// Send help request
							IPC_Command(data->main_ipc, IPC_HELP, (1 << 31), "Print", 0, (struct MsgPort *)-1);

							// Clear busy pointer
							ClearWindowBusy(data->window);
						}
					}
					break;
				}
			}
		}

		// Check break flag
		if (break_flag)
			break;

		// Wait for an event
		Wait(((data->window) ? 1 << data->window->UserPort->mp_SigBit : 0) |
			 ((ipc) ? 1 << ipc->command_port->mp_SigBit : 0));
	}

	// Print?
	if (break_flag == 2)
	{
		// Get gadget values
		data->print_quality = GetGadgetValue(data->objlist, GAD_PRINT_TEXT_QUALITY);
		data->print_spacing = GetGadgetValue(data->objlist, GAD_PRINT_TEXT_SPACING);
		data->print_pitch = GetGadgetValue(data->objlist, GAD_PRINT_TEXT_PITCH);
		data->left_margin = GetGadgetValue(data->objlist, GAD_PRINT_LEFT_MARGIN);
		data->right_margin = GetGadgetValue(data->objlist, GAD_PRINT_RIGHT_MARGIN);
		data->page_length = GetGadgetValue(data->objlist, GAD_PRINT_PAGE_LENGTH);
		data->tab_size = GetGadgetValue(data->objlist, GAD_PRINT_TAB_SIZE);
		data->output_type = GetGadgetValue(data->objlist, GAD_PRINT_OUTPUT);
		print_get_header(data, GetGadgetValue(data->objlist, GAD_PRINT_HEADER_FOOTER));

		// Close window
		CloseConfigWindow(data->window);
		data->window = 0;

		// Print the stuff
		print_print(data);

		// Set environment variable
		SetVar("dopus/Print", (char *)&data->print_quality, DATA_SIZE, GVF_GLOBAL_ONLY);
	}

	// Free data
	print_free(data);
	return 1;
}

// Free print stuff
void print_free(print_data *data)
{
	if (data)
	{
		// Close progress window
		CloseProgressWindow(data->progress);

		// Close window
		CloseConfigWindow(data->window);

		// Free signal bit
		if (data->abort_bit > -1)
			FreeSignal(data->abort_bit);

		// Free file requester
		if (data->filereq)
			FreeAslRequest(data->filereq);

		// Free data
		FreeVec(data);
	}
}

// Update header gadgets
void print_fix_header(print_data *data)
{
	short h;

	// Get header/footer value
	h = GetGadgetValue(data->objlist, GAD_PRINT_HEADER_FOOTER);

	// Update gadget values
	SetGadgetValue(data->objlist, GAD_PRINT_TITLE, data->header_flags[h] & PRINT_HEADERF_TITLE);
	SetGadgetValue(data->objlist, GAD_PRINT_DATE, data->header_flags[h] & PRINT_HEADERF_DATE);
	SetGadgetValue(data->objlist, GAD_PRINT_PAGE, data->header_flags[h] & PRINT_HEADERF_PAGE);
	SetGadgetValue(data->objlist, GAD_PRINT_TITLE_STRING, (IPTR)data->header_title[h]);
	SetGadgetValue(data->objlist, GAD_PRINT_STYLE, data->header_style[h]);

	// En/disable title string
	DisableObject(data->objlist, GAD_PRINT_TITLE_STRING, !(data->header_flags[h] & PRINT_HEADERF_TITLE));
}

// Get header gadget values
void print_get_header(print_data *data, short h)
{
	// Get gadget values
	data->header_flags[h] = 0;
	if (GetGadgetValue(data->objlist, GAD_PRINT_TITLE))
		data->header_flags[h] |= PRINT_HEADERF_TITLE;
	if (GetGadgetValue(data->objlist, GAD_PRINT_DATE))
		data->header_flags[h] |= PRINT_HEADERF_DATE;
	if (GetGadgetValue(data->objlist, GAD_PRINT_PAGE))
		data->header_flags[h] |= PRINT_HEADERF_PAGE;
	print_copy_string(data->header_title[h], (char *)GetGadgetValue(data->objlist, GAD_PRINT_TITLE_STRING), sizeof(data->header_title[h]));
	data->header_style[h] = GetGadgetValue(data->objlist, GAD_PRINT_STYLE);
}

// Open print window
BOOL print_open(print_data *data)
{
	// Fill out new window
	data->newwin.parent = data->screen;
	data->newwin.dims = &print_window;
	data->newwin.title = GetString(locale, MSG_PRINTREQ_TITLE);
	data->newwin.locale = locale;
	data->newwin.port = 0;
	data->newwin.flags = WINDOW_SCREEN_PARENT | WINDOW_VISITOR | WINDOW_AUTO_KEYS | WINDOW_REQ_FILL;
	data->newwin.font = 0;

	// Open window
	if (!(data->window = OpenConfigWindow(&data->newwin)) ||
		!(data->objlist = AddObjectList(data->window, print_objects)))
		return 0;

	// Initialise gadgets
	SetGadgetValue(data->objlist, GAD_PRINT_TEXT_QUALITY, data->print_quality);
	SetGadgetValue(data->objlist, GAD_PRINT_TEXT_SPACING, data->print_spacing);
	SetGadgetValue(data->objlist, GAD_PRINT_TEXT_PITCH, data->print_pitch);
	SetGadgetValue(data->objlist, GAD_PRINT_LEFT_MARGIN, data->left_margin);
	SetGadgetValue(data->objlist, GAD_PRINT_RIGHT_MARGIN, data->right_margin);
	SetGadgetValue(data->objlist, GAD_PRINT_PAGE_LENGTH, data->page_length);
	SetGadgetValue(data->objlist, GAD_PRINT_TAB_SIZE, data->tab_size);
	SetGadgetValue(data->objlist, GAD_PRINT_OUTPUT, data->output_type);
	print_fix_header(data);

	return 1;
}

// Do print
void print_print(print_data *data)
{
	struct Node *node;
	BOOL ok = 0;

	// Open progress window
	if (!(data->progress = OpenProgressWindowTags(PW_Screen,
												  (IPTR)data->screen,
												  PW_Title,
												  (IPTR)GetString(locale, MSG_PRINTING_TITLE),
												  PW_SigTask,
												  (IPTR)data->ipc->proc,
												  PW_SigBit,
												  data->abort_bit,
												  PW_Flags,
												  PWF_INFO | PWF_GRAPH,
												  PW_Info,
												  (IPTR)GetString(locale, MSG_OPENING_PRINTER),
												  TAG_END)))
		return;

	// Get progress window pointer
	data->prog_win = 0;
	GetProgressWindowTags(data->progress, PW_Window, (IPTR)&data->prog_win, 0);

	// Get line width
	data->line_width = (data->right_margin - data->left_margin) + 1;

	// Output to file?
	if (data->output_type == 1)
	{
		char *ptr;

		// Set busy pointer
		SetBusyPointer(data->prog_win);

		// Strip filename
		if ((ptr = FilePart(data->output_name)) > data->output_name)
			*ptr = 0;

		// Get filename
		if (!(AslRequestTags(data->filereq,
							 ASLFR_Window,
							 data->prog_win,
							 ASLFR_TitleText,
							 GetString(locale, MSG_SELECT_OUTPUT_FILE),
							 ASLFR_InitialDrawer,
							 data->output_name,
							 ASLFR_Flags1,
							 FRF_DOSAVEMODE | FRF_PRIVATEIDCMP,
							 ASLFR_Flags2,
							 FRF_REJECTICONS,
							 TAG_END)))
			return;

		// Clear busy pointer
		ClearPointer(data->prog_win);

		// Get filename
		print_copy_string(data->output_name, data->filereq->fr_Drawer, sizeof(data->output_name));
		AddPart(data->output_name, data->filereq->fr_File, 256);

		// Open file
		if (!(data->output_file = OpenBuf(data->output_name, MODE_NEWFILE, 256)))
			return;

		// Ok
		ok = 1;
	}

	// Open printer
	else
	{
		// Create printer port, open printer
		if (!(data->printer_port = CreateMsgPort()))
			return;
		if (!(data->printer_io = (union printerIO *)CreateIORequest(data->printer_port, sizeof(union printerIO))))
		{
			DeleteMsgPort(data->printer_port);
			return;
		}
		if (OpenDevice("printer.device", 0, (struct IORequest *)data->printer_io, 0))
		{
			// Show error
			SimpleRequest(data->prog_win,
						  GetString(locale, MSG_PRINTREQ_TITLE),
						  GetString(locale, MSG_OK),
						  GetString(locale, MSG_CANT_OPEN_PRINTER),
						  0,
						  0,
						  0,
						  0);
			DeleteIORequest((struct IORequest *)data->printer_io);
			DeleteMsgPort(data->printer_port);
			return;
		}

		// Open successfully
		else
		{
			struct PrinterData *pd;
			struct Preferences *prefs;

			// Get pointer to printer data and from there preferences
			pd = (struct PrinterData *)data->printer_io->ios.io_Device;
			prefs = &pd->pd_Preferences;

			// Set our configuration
			prefs->PrintLeftMargin = data->left_margin;
			prefs->PrintRightMargin = data->right_margin + 1;
			prefs->PaperLength = data->page_length + 2;
			prefs->PrintQuality = (data->print_quality == PRINT_QUALITY_LETTER) ? LETTER : DRAFT;
			prefs->PrintSpacing = (data->print_spacing == PRINT_SPACING_6) ? SIX_LPI : EIGHT_LPI;
			prefs->PrintPitch = (data->print_pitch == PRINT_PITCH_ELITE)
									? ELITE
									: ((data->print_pitch == PRINT_PITCH_FINE) ? FINE : PICA);

			// Send init string
			if (print_print_string(data, "\x1b#1"))
				ok = 1;
		}
	}

	// Open ok?
	if (ok)
	{
		// Go through files
		for (node = data->files->lh_Head; node->ln_Succ; node = node->ln_Succ)
		{
			BPTR file;
			BOOL ret;

			// Show filename, reset progress bar
			SetProgressWindowTags(
				data->progress, PW_Info, (IPTR)FilePart(node->ln_Name), PW_FileCount, 1, PW_FileNum, 0, TAG_END);

			// Lock file
			if (!(file = Lock(node->ln_Name, ACCESS_READ)) || !(Examine(file, &data->fib)))
			{
				UnLock(file);
				break;
			}
			UnLock(file);

			// Check it is a file
			if (data->fib.fib_DirEntryType < 0 && data->fib.fib_Size > 0)
			{
				// Open file
				if (!(file = Open(node->ln_Name, MODE_OLDFILE)))
					break;

				// Print file
				ret = print_print_file(data, file);

				// Close file
				Close(file);
				if (!ret)
					break;
			}
		}
	}

	// File open?
	if (data->output_file)
	{
		// Close file
		CloseBuf(data->output_file);
		data->output_file = 0;
	}

	// Otherwise, close printer
	else
	{
		// Close printer
		CloseDevice((struct IORequest *)data->printer_io);

		// Cleanup
		DeleteIORequest((struct IORequest *)data->printer_io);
		DeleteMsgPort(data->printer_port);
	}
}

// Check for abort
BOOL print_check_abort(print_data *data)
{
	BOOL abort = 0;

	// Check for abort
	if (SetSignal(0, 1 << data->abort_bit) & (1 << data->abort_bit))
		abort = 1;

	return abort;
}

// Print a file
BOOL print_print_file(print_data *data, BPTR file)
{
	short buffer_size, buffer_pos;
	short line_pos;
	short last_space;
	short line;
	long total;

	// Convert date to string
	data->file_date.dat_Stamp = data->fib.fib_Date;
	data->file_date.dat_StrDate = data->date_buf;
	data->file_date.dat_StrTime = data->time_buf;
	DateToStr(&data->file_date);

	// Initialise things
	buffer_pos = 0;
	line_pos = 0;
	last_space = 0;
	line = 0;
	total = 0;
	data->page = 1;

	// Read first chunk of file
	buffer_size = Read(file, data->buffer, 8192);

	// Loop while we've got data
	while (buffer_size > 0)
	{
		char ch;
		short print_line = 0;

		// Get current character
		ch = data->buffer[buffer_pos++];

		// End of buffer?
		if (buffer_pos == buffer_size)
		{
			// Update total count
			total += buffer_size;

			// Read more data
			buffer_size = Read(file, data->buffer, 8192);
			buffer_pos = 0;

			// End of file?
			if (buffer_size < 1)
				print_line = 2;
		}

		// Look at character
		switch (ch)
		{
		// Tab
		case '\t': {
			short pos;

			// Get new position
			pos = ((line_pos + data->tab_size) / data->tab_size) * data->tab_size;

			// Not past line length?
			if (pos < data->line_width)
			{
				// Add spaces to next tab position
				while (line_pos < pos)
				{
					data->line_buffer[line_pos++] = ' ';
				}
				data->line_buffer[line_pos] = 0;
				break;
			}
		}

		// Carriage return; strip
		case '\r':
			break;

		// New line
		case '\n':

			// Set flag to print line
			if (!print_line)
				print_line = 1;
			break;

		// Form feed
		case '\f':

			// Set flag for form feed
			print_line = 2;
			break;

		// Other character
		default:

			// Store character
			data->line_buffer[line_pos++] = ch;
			data->line_buffer[line_pos] = 0;

			// Past end of line?
			if (line_pos > data->line_width)
			{
				// Last space for word-wrap?
				if (last_space > 0)
				{
					// Copy to wrap buffer
					print_copy_string(data->wrap_buffer, data->line_buffer + last_space + 1 + ((ch == ' ' || ch == '\t') ? 1 : 0), sizeof(data->wrap_buffer));
					data->line_buffer[last_space] = 0;
					line_pos = last_space;
					last_space = 0;
				}

				// Set flag to print line
				if (!print_line)
					print_line = 1;
			}

			// Space?
			else if (ch == ' ' || ch == '\t')
				last_space = line_pos - 1;
			break;
		}

		// Print line?
		if (print_line)
		{
			// Need to print header?
			if (line == 0 && data->header_flags[PRINT_HEADER])
			{
				// Print header
				if (!(print_header_footer(data, PRINT_HEADER)))
					return 0;

				// Now on line 2
				line = 2;
			}

			// Print line
			if (!(print_print_string(data, data->line_buffer)))
				return 0;

			// Increment line count
			++line;

			// Need new-line?
			if (!(print_print_string(data, "\n")))
				return 0;

			// Any word-wrap data?
			if (data->wrap_buffer[0])
			{
				// Copy word-wrap stuff
				print_copy_string(data->line_buffer, data->wrap_buffer, sizeof(data->line_buffer));
				line_pos = strlen(data->line_buffer);

				// Clear word-wrap buffer
				data->wrap_buffer[0] = 0;
			}

			// Otherwise, clear buffer
			else
			{
				data->line_buffer[0] = 0;
				line_pos = 0;
			}

			// Clear last space
			last_space = 0;

			// Form feed and need a footer?
			if (print_line == 2 && data->header_flags[PRINT_FOOTER])
			{
				// New-line down to footer
				while (line < data->page_length - 2)
				{
					if (!(print_print_string(data, "\n")))
						return 0;
					++line;
				}
			}

			// Need to print footer?
			if (data->header_flags[PRINT_FOOTER] && line == data->page_length - 2)
			{
				// Print footer
				if (!(print_header_footer(data, PRINT_FOOTER)))
					return 0;

				// On end of page
				line = data->page_length;
			}

			// End of page?
			if (line == data->page_length)
			{
				// Send form-feed
				if (!(print_print_string(data, "\f")))
					return 0;

				// Increment page number
				++data->page;

				// Reset line number
				line = 0;
			}

			// Update progress bar
			SetProgressWindowTags(
				data->progress, PW_FileCount, data->fib.fib_Size, PW_FileNum, total + buffer_pos, TAG_END);
		}
	}

	// Final form-feed
	if (line > 0)
		print_print_string(data, "\f");
	return 1;
}

// Print current line buffer
BOOL print_print_string(print_data *data, char *buf)
{
	short len;

	// Check for abort
	if (print_check_abort(data))
	{
		// Verify abort
		if (SimpleRequest(data->prog_win,
						  GetString(locale, MSG_PRINTREQ_TITLE),
						  GetString(locale, MSG_ABORT_RESUME),
						  GetString(locale, MSG_REALLY_ABORT),
						  0,
						  0,
						  0,
						  0))
			return 0;
	}

	// Get length
	len = strlen(buf);

	// Got a file?
	if (data->output_file)
	{
		// Write data to file
		while ((WriteBuf(data->output_file, buf, len)) < len)
		{
			long error = IoErr();

			// Show error
			if (!(SimpleRequest(data->prog_win,
								GetString(locale, MSG_PRINTREQ_TITLE),
								GetString(locale, MSG_RETRY_CANCEL),
								GetString(locale, MSG_PRINTER_ERROR),
								0,
								&error,
								0,
								0)))
				return 0;
		}

		// Ok
		return 1;
	}

	// Send command to print buffer
	data->printer_io->ios.io_Length = len;
	data->printer_io->ios.io_Data = buf;
	data->printer_io->ios.io_Command = CMD_WRITE;
	SendIO((struct IORequest *)data->printer_io);

	// Wait for reply or abort
	FOREVER
	{
		// Reply?
		if (CheckIO((struct IORequest *)data->printer_io))
		{
			// Wait for reply
			WaitIO((struct IORequest *)data->printer_io);

			// Any errors?
			if (data->printer_io->ios.io_Error)
			{
				long error = data->printer_io->ios.io_Error;

				// Show error
				if (!(SimpleRequest(data->prog_win,
									GetString(locale, MSG_PRINTREQ_TITLE),
									GetString(locale, MSG_RETRY_CANCEL),
									GetString(locale, MSG_PRINTER_ERROR),
									0,
									&error,
									0,
									0)))
					return 0;

				// Re-send request
				data->printer_io->ios.io_Length = len;
				data->printer_io->ios.io_Data = buf;
				data->printer_io->ios.io_Command = CMD_WRITE;
				SendIO((struct IORequest *)data->printer_io);
			}

			// Success
			else
				return 1;
		}

		// Check for abort
		if (print_check_abort(data))
		{
			// Verify abort
			if (SimpleRequest(data->prog_win,
							  GetString(locale, MSG_PRINTREQ_TITLE),
							  GetString(locale, MSG_ABORT_RESUME),
							  GetString(locale, MSG_REALLY_ABORT),
							  0,
							  0,
							  0,
							  0))
			{
				// Abort request
				AbortIO((struct IORequest *)data->printer_io);
				WaitIO((struct IORequest *)data->printer_io);
				return 0;
			}
		}

		// Wait for signal
		Wait(1 << data->abort_bit | 1 << data->printer_port->mp_SigBit);
	}
}

// Print header or footer
BOOL print_header_footer(print_data *data, short which)
{
	short a, off = 0;

	// Initial newline for footer
	if (which == PRINT_FOOTER)
	{
		data->header_buffer[0] = '\n';
		off = 1;
	}

	// Style
	if (data->header_style[which])
	{
		print_copy_string(data->header_buffer + off, print_styles[data->header_style[which] - 1], sizeof(data->header_buffer) - off);
		off = strlen(data->header_buffer);
	}

	// Fill header full of spaces
	for (a = off; a < data->line_width + off; a++)
		data->header_buffer[a] = ' ';
	data->header_buffer[a] = 0;

	// Date?
	if (data->header_flags[which] & PRINT_HEADERF_DATE)
		CopyMem(data->date_buf, data->header_buffer + off + 1, strlen(data->date_buf));

	// Title?
	if (data->header_flags[which] & PRINT_HEADERF_TITLE)
	{
		char title[40];
		short len, x;

		// Supplied title?
		if (data->header_title[which][0])
			print_copy_string(title, data->header_title[which], sizeof(title));

		// Otherwise use filename
		else
			print_copy_string(title, data->fib.fib_FileName, sizeof(title));

		// Check length
		if ((len = strlen(title)) > data->line_width - 4)
		{
			len = data->line_width - 4;
			x = 2;
		}

		// Get offset
		else
			x = (data->line_width - len) >> 1;

		// Copy into title
		if (len > 0)
			CopyMem(title, data->header_buffer + off + x, len);
	}

	// Page number?
	if (data->header_flags[which] & PRINT_HEADERF_PAGE)
	{
		char page[20];
		short len;

		// Build page string
		lsprintf(page, "%s%ld", (IPTR)GetString(locale, MSG_PRINT_PAGENUM), data->page);
		if ((len = strlen(page)) > data->line_width - 1)
			len = data->line_width - 1;

		// Right-justify
		if (len > 0)
			CopyMem(page, data->header_buffer + off + data->line_width - len - 1, len);
	}

	// Restore normal
	if (data->header_style[which])
	{
		print_append_string(data->header_buffer, "\x1b[0m", sizeof(data->header_buffer));
	}

	// Print string
	if (!(print_print_string(data, data->header_buffer)))
		return 0;

	// Trailing newline (two for header)
	if (!(print_print_string(data, (which == PRINT_HEADER) ? "\n\n" : "\n")))
		return 0;

	return 1;
}
