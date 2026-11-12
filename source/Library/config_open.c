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
#include "config.h"
#include <Program/dopus_config.h>
#include "config_open.h"
#include <stddef.h>

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct
{
	char *buffer;
	char *pointer;
	int size;
	short error;
} string_handle;

#ifndef __amigaos3__
	#pragma pack()
#endif

static ULONG iff_file_id = 0;

void init_config_string(struct _IFFHandle *iff, string_handle *handle);
char *read_config_string(APTR memory, string_handle *handle);
short read_config_short(string_handle *handle);
void free_config_string(string_handle *handle);
char *copy_string(APTR, char *);
static APTR open_button_image(char *);
#ifdef __AROS__
#define AROS_CFG_BUTN_DISK_SIZE 14
static void aros_config_debug_log(char *);
static UWORD aros_raw_word(UBYTE *);
static ULONG aros_raw_long(UBYTE *);
static Cfg_ButtonBank *aros_open_button_bank_raw(char *, char *);
#endif
void dump_button_info(Cfg_ButtonBank *);
void env_read_open_bank(APTR, struct OpenEnvironmentData *, ULONG);
void env_read_open_lister(APTR, struct OpenEnvironmentData *, ULONG);
int convert_env(struct _IFFHandle *iff, CFG_ENVR *env);
void convert_list_format(OldListFormat *old, ListFormat *format);
int convert_open_lister(struct _IFFHandle *iff, CFG_LSTR *lister);
int convert_button_window(struct _IFFHandle *iff, CFG_BTNW *butwin);
void do_backup(char *name);

short LIBFUNC L_ReadSettings(REG(a0, CFG_SETS *settings), REG(a1, char *name))
{
	struct _IFFHandle *iff;
	short success = READCFG_FAIL;

	// Try to open file to read
	if ((iff = L_IFFOpen(name, IFF_READ, ID_OPUS)))
	{
		ULONG chunk;
		BOOL ok = 0;

		// Got file
		success = READCFG_OK;

		// Parse file
		while ((chunk = L_IFFNextChunk(iff, 0)))
		{
			// Look at chunk type
			switch (chunk)
			{
			// Settings
			case ID_SETS: {
				short size;

				// Right type of file at least
				ok = 1;

				// Get size to read
				size = L_IFFChunkSize(iff);
				if (size > sizeof(CFG_SETS))
					size = sizeof(CFG_SETS);

				// Read info
				if ((L_IFFReadChunkBytes(iff, settings, size)) != size)
					success = READCFG_FAIL;
#ifdef __AROS__
				else
				{
					settings->copy_flags = AROS_BE2LONG(settings->copy_flags);
					settings->delete_flags = AROS_BE2LONG(settings->delete_flags);
					settings->error_flags = AROS_BE2LONG(settings->error_flags);
					settings->general_flags = AROS_BE2LONG(settings->general_flags);
					settings->icon_flags = AROS_BE2LONG(settings->icon_flags);
					settings->replace_method = AROS_BE2WORD(settings->replace_method);
					settings->replace_flags = AROS_BE2WORD(settings->replace_flags);
					settings->update_flags = AROS_BE2LONG(settings->update_flags);
					settings->dir_flags = AROS_BE2LONG(settings->dir_flags);
					settings->view_flags = AROS_BE2LONG(settings->view_flags);
					settings->max_buffer_count = AROS_BE2WORD(settings->max_buffer_count);
					settings->date_format = AROS_BE2WORD(settings->date_format);
					settings->date_flags = AROS_BE2WORD(settings->date_flags);
					settings->pop_code = AROS_BE2WORD(settings->pop_code);
					settings->pop_qual = AROS_BE2WORD(settings->pop_qual);
					settings->pop_qual_mask = AROS_BE2WORD(settings->pop_qual_mask);
					settings->pop_qual_same = AROS_BE2WORD(settings->pop_qual_same);
					settings->popup_delay = AROS_BE2WORD(settings->popup_delay);
					settings->max_openwith = AROS_BE2WORD(settings->max_openwith);
					settings->command_line_length = AROS_BE2WORD(settings->command_line_length);
					settings->max_filename = AROS_BE2WORD(settings->max_filename);
					settings->flags = AROS_BE2LONG(settings->flags);
				}
#endif
			}
			break;
			}
		}

		// Close file
		L_IFFClose(iff);

		// Wrong type of file?
		if (success == READCFG_OK && !ok)
			success = READCFG_WRONG;
	}

	// Wrong type?
	else if (IoErr() == ERROR_OBJECT_WRONG_TYPE)
		success = READCFG_WRONG;

	return success;
}

// Read a lister definition
Cfg_Lister *LIBFUNC L_ReadListerDef(REG(a0, struct _IFFHandle *iff), REG(d0, ULONG id))
{
	Cfg_Lister *lister = NULL;

	// Allocate lister structure
	if ((lister = L_NewLister(0)))
	{
		string_handle handle;
		CFG_DLST old;

		// Old style?
		if (id == ID_DLST)
		{
			// Read lister data
			L_IFFReadChunkBytes(iff, &old, sizeof(CFG_DLST));

			// Convert to new
			lister->lister.pos[0] = old.pos;
			lister->lister.pos[1] = old.pos;
			lister->lister.format = old.format;
			lister->lister.flags = old.flags;
			lister->lister.icon_x = NO_ICON_POSITION;
			lister->lister.icon_y = NO_ICON_POSITION;
		}

		// Read lister data
		else
		{
			if (iff_file_id == ID_OPUS)
			{
				if (!convert_open_lister(iff, &lister->lister))
				{
					L_FreeListerDef(lister);
					return NULL;
				}
			}
			else
				L_IFFReadChunkBytes(iff, &lister->lister, sizeof(CFG_LSTR));

#ifdef __AROS__
			lister->lister.icon_x = AROS_BE2LONG(lister->lister.icon_x);
			lister->lister.icon_y = AROS_BE2LONG(lister->lister.icon_y);
#endif
		}

#ifdef __AROS__
		lister->lister.pos[0].Left = AROS_BE2WORD(lister->lister.pos[0].Left);
		lister->lister.pos[0].Top = AROS_BE2WORD(lister->lister.pos[0].Top);
		lister->lister.pos[0].Width = AROS_BE2WORD(lister->lister.pos[0].Width);
		lister->lister.pos[0].Height = AROS_BE2WORD(lister->lister.pos[0].Height);
		lister->lister.pos[1].Left = AROS_BE2WORD(lister->lister.pos[1].Left);
		lister->lister.pos[1].Top = AROS_BE2WORD(lister->lister.pos[1].Top);
		lister->lister.pos[1].Width = AROS_BE2WORD(lister->lister.pos[1].Width);
		lister->lister.pos[1].Height = AROS_BE2WORD(lister->lister.pos[1].Height);
		lister->lister.flags = AROS_BE2LONG(lister->lister.flags);
#endif
		// Read lister path
		init_config_string(iff, &handle);
		lister->path = read_config_string(0, &handle);
		free_config_string(&handle);
	}

	return lister;
}

// Read a button bank
Cfg_ButtonBank *LIBFUNC L_OpenButtonBank(REG(a0, char *name))
{
	Cfg_ButtonBank *current_bank = 0;
	struct _IFFHandle *iff = NULL;
	BPTR lock, old = 0;
	char *name_ptr = 0;
	BOOL ok = 0;
	ULONG iff_button_id = 0;

	// See if file exists as is
	if ((lock = Lock(name, ACCESS_READ)))
	{
		D_S(struct FileInfoBlock, fib)

		// Examine it
		Examine(lock, fib);
		UnLock(lock);

		// Check it's a file
		if (fib->fib_DirEntryType < 0)
		{
			// It does
			name_ptr = name;
			ok = 1;
		}
		lock = 0;
	}

	// It doesn't
	if (!ok)
	{
		char *drawers[] = {"PROGDIR:Buttons", "DOpus5:Buttons", 0};
		short a;

		// Get filename only
		name_ptr = FilePart(name);

		// See if it's in the buttons drawer
		for (a = 0; drawers[a]; a++)
		{
			BPTR test;

			if ((lock = Lock(drawers[a], ACCESS_READ)))
			{
				// Change directory
				old = CurrentDir(lock);

				// See if file is in there
				if ((test = Lock(name_ptr, ACCESS_READ)))
				{
					// It is
					UnLock(test);
					break;
				}

				// It's not in there
				CurrentDir(old);
				UnLock(lock);
				lock = 0;
			}
		}

		if (!lock)
			name_ptr = 0;
	}

	// Try to open file to read
	if (name_ptr)
	{
		if ((iff = L_IFFOpen(name_ptr, IFF_READ, ID_OPUS)))
			iff_button_id = ID_OPUS;
		else
			iff = L_IFFOpen(name_ptr, IFF_READ, ID_EPUS);
	}

	if (iff)
	{
		APTR file;
		Cfg_Button *current_button;
		Cfg_ButtonFunction *current_func = 0;
		ULONG chunk;

		// Get file handle
		file = L_IFFFileHandle(iff);

		// Parse file
		while ((chunk = L_IFFNextChunk(iff, 0)))
		{
			// Look at chunk type
			switch (chunk)
			{
			// Window definition
			case ID_BTNW:

				// Allocate button bank
				if ((current_bank = L_NewButtonBank(0, 0)))
				{
					// Get bank path name
					if (!(NameFromFH(L_FHFromBuf(file), current_bank->path, 256)))
						strcpy(current_bank->path, name);

					// Read bank data
					if (iff_button_id == ID_OPUS)
					{
						if (!convert_button_window(iff, &current_bank->window))
						{
							L_CloseButtonBank(current_bank);
							current_bank = 0;
							goto bank_cleanup;
						}
					}
					else
						L_IFFReadChunkBytes(iff, &current_bank->window, sizeof(CFG_BTNW));
#ifdef __AROS__
					current_bank->window.pos.Left = AROS_BE2WORD(current_bank->window.pos.Left);
					current_bank->window.pos.Top = AROS_BE2WORD(current_bank->window.pos.Top);
					current_bank->window.pos.Width = AROS_BE2WORD(current_bank->window.pos.Width);
					current_bank->window.pos.Height = AROS_BE2WORD(current_bank->window.pos.Height);
					current_bank->window.columns = AROS_BE2WORD(current_bank->window.columns);
					current_bank->window.rows = AROS_BE2WORD(current_bank->window.rows);
					current_bank->window.flags = AROS_BE2LONG(current_bank->window.flags);
#endif

					// Clear 'toolbar' flag
					current_bank->window.flags &= ~BTNWF_TOOLBAR;
				}
				break;

			// Background picture
			case ID_BPIC:
				if (current_bank)
					L_IFFReadChunkBytes(iff, current_bank->backpic, sizeof(current_bank->backpic));
				break;

			// Button definition
			case ID_BUTN:

				// Do we have a current bank?
				if (!current_bank)
					break;

				// Read button
				if ((current_button = L_ReadButton(iff, current_bank->memory)))
				{
					// Add to button list
					AddTail(&current_bank->buttons, &current_button->node);

					// Get first function pointer
					current_func = (Cfg_ButtonFunction *)current_button->function_list.mlh_Head;
				}
				break;

			// Function definition
			case ID_FUNC:

				// Do we have something to read this in to?
				if (!current_func || !current_func->node.ln_Succ)
					break;

				// Read function
				L_ReadFunction(iff, current_bank->memory, 0, (Cfg_Function *)current_func);

				// Get next function
				current_func = (Cfg_ButtonFunction *)current_func->node.ln_Succ;
				break;

			// Start menu info
			case ID_STRT:

				// Allocate information
				if ((current_bank->startmenu = L_AllocMemH(current_bank->memory, sizeof(CFG_STRT))))
				{
					// Read data
					L_IFFReadChunkBytes(iff, current_bank->startmenu, sizeof(CFG_STRT));

#ifdef __AROS__
					current_bank->startmenu->flags = AROS_BE2LONG(current_bank->startmenu->flags);
					current_bank->startmenu->fpen = AROS_BE2WORD(current_bank->startmenu->fpen);
					current_bank->startmenu->label_fpen = AROS_BE2WORD(current_bank->startmenu->label_fpen);
					current_bank->startmenu->sel_fpen = AROS_BE2WORD(current_bank->startmenu->sel_fpen);
					current_bank->startmenu->label_fontsize = AROS_BE2WORD(current_bank->startmenu->label_fontsize);
#endif
				}
				break;
			}
		}

bank_cleanup:
		L_IFFClose(iff);
	}

#ifdef __AROS__
	if (name_ptr && (!current_bank || IsListEmpty(&current_bank->buttons)))
	{
		Cfg_ButtonBank *raw_bank;

		aros_config_debug_log("OpenButtonBank raw fallback path=");
		aros_config_debug_log(name_ptr);
		aros_config_debug_log(" original=");
		aros_config_debug_log(name);
		aros_config_debug_log("\n");

		if ((raw_bank = aros_open_button_bank_raw(name_ptr, name)))
		{
			if (!IsListEmpty(&raw_bank->buttons))
			{
				aros_config_debug_log("OpenButtonBank raw fallback replaced empty bank\n");
				L_CloseButtonBank(current_bank);
				current_bank = raw_bank;
			}
			else
				L_CloseButtonBank(raw_bank);
		}
	}
#endif

	// Directory to unlock?
	if (lock)
	{
		// Restore cd
		CurrentDir(old);
		UnLock(lock);
	}

	// A version 1 startmenu that needs to be converted?
	if (current_bank && current_bank->startmenu && !(current_bank->startmenu->flags & STRTF_VERSION2))
	{
		L_ConvertStartMenu(current_bank);
		current_bank->startmenu->fpen = 1;
		current_bank->startmenu->label_fpen = 1;
		current_bank->startmenu->sel_fpen = 2;
	}
	return current_bank;
}

// Read a button
Cfg_Button *LIBFUNC L_ReadButton(REG(a0, struct _IFFHandle *iff), REG(a1, APTR memory))
{
	Cfg_Button *button;

	// Allocate a new button
	if ((button = L_NewButton(memory)))
	{
		string_handle handle;
		short count, a;
		char *name, *label;

		// Read button data
#ifdef __AROS__
		{
			UBYTE raw_button[AROS_CFG_BUTN_DISK_SIZE];

			L_IFFReadChunkBytes(iff, raw_button, sizeof(raw_button));
			button->button.fpen = raw_button[0];
			button->button.bpen = raw_button[1];
			button->button.flags = aros_raw_long(raw_button + 2);
			button->button.count = (short)aros_raw_word(raw_button + 6);
			button->button.pad_1 = (short)aros_raw_word(raw_button + 8);
			button->button.pad_2 = aros_raw_long(raw_button + 10);
		}
#else
		L_IFFReadChunkBytes(iff, &button->button, sizeof(CFG_BUTN));
#endif

		// Old style button?
		if (!(button->button.flags & BUTNF_NEW_FORMAT))
			count = 3;
		else
			count = button->button.count;

		// Read button names
		init_config_string(iff, &handle);
		for (a = 0; a < count; a++)
		{
			Cfg_ButtonFunction *func;

			// Read name
			if (!(name = read_config_string(memory, &handle)))
				break;

			// New style button?
			if (button->button.flags & BUTNF_NEW_FORMAT)
			{
				// Read label
				label = read_config_string(memory, &handle);

				// Special marker?
				if (label && label[0] == 1 && label[1] == 0)
				{
					// Free marker
					L_FreeMemH(label);

					// Copy the name
					label = copy_string(memory, name);
				}
			}

			// Otherwise, is button graphical?
			else if (button->button.flags & BUTNF_GRAPHIC)
			{
				char filename[40];
				char *ptr;

				// Use filename as label
				stccpy(filename, FilePart(name), 39);

				// Strip any suffix (.small for instance)
				if ((ptr = strchr(filename, '.')))
					*ptr = 0;
				label = copy_string(memory, filename);
			}

			// Otherwise, copy name
			else
				label = copy_string(memory, name);

			// No label?
			if (!label)
			{
				L_FreeMemH(name);
				break;
			}

			// Allocate button function
			if ((func = L_NewButtonFunction(memory, 0)))
			{
				// Store name as label
				func->label = name;

				// And store label as name (for some reason :)
				func->node.ln_Name = label;

				// If a graphic button, get image
				if (button->button.flags & BUTNF_GRAPHIC)
					func->image = open_button_image(name);

				// Add to button list
				AddTail((struct List *)&button->function_list, &func->node);
			}

			// Failed
			else
			{
				L_FreeMemH(name);
				L_FreeMemH(label);
				break;
			}
		}

		// Free string stuff
		free_config_string(&handle);
	}

	return button;
}

static APTR open_button_image(char *name)
{
	APTR image;
	char image_name[256];
	char *file;
	int len;

	image = L_OpenImage(name, 0);
	if (image || !name || !name[0])
		return image;

	file = FilePart(name);
	if (file && strchr(file, '.'))
		return 0;

	len = strlen(name);
	if (len + 6 >= sizeof(image_name))
		return 0;

	strcpy(image_name, name);
	strcat(image_name, ".small");

	return L_OpenImage(image_name, 0);
}

#ifdef __AROS__
static void aros_config_debug_log(char *text)
{
	BPTR file;
	char *paths[] = {"PROGDIR:DOpus5-startup.log", "T:DOpus5-startup.log", 0};
	short a;

	if (!text)
		return;

	for (a = 0; paths[a]; a++)
	{
		if ((file = Open(paths[a], MODE_READWRITE)) || (file = Open(paths[a], MODE_NEWFILE)))
		{
			Seek(file, 0, OFFSET_END);
			FPuts(file, text);
			Close(file);
		}
	}
}

static BOOL aros_raw_id(UBYTE *data, char *id)
{
	return (data[0] == id[0] && data[1] == id[1] && data[2] == id[2] && data[3] == id[3]);
}

static UWORD aros_raw_word(UBYTE *data)
{
	return ((UWORD)data[0] << 8) | data[1];
}

static ULONG aros_raw_long(UBYTE *data)
{
	return ((ULONG)data[0] << 24) | ((ULONG)data[1] << 16) | ((ULONG)data[2] << 8) | data[3];
}

static char *aros_raw_copy_string(APTR memory, UBYTE **data, ULONG *remain)
{
	ULONG len;
	char *string;

	for (len = 0; len < *remain && (*data)[len]; len++)
		;
	if (len >= *remain)
		return 0;

	if ((string = L_AllocMemH(memory, len + 1)))
	{
		CopyMem(*data, string, len);
		string[len] = 0;
	}

	*data += len + 1;
	*remain -= len + 1;
	return string;
}

static void aros_raw_read_button_window(CFG_BTNW *window, UBYTE *data, ULONG size)
{
	memset(window, 0, sizeof(CFG_BTNW));

	if (size >= sizeof(CFG_BTNW))
	{
		CopyMem(data, window->name, 32);
		window->name[31] = 0;
		window->pos.Left = (WORD)aros_raw_word(data + 32);
		window->pos.Top = (WORD)aros_raw_word(data + 34);
		window->pos.Width = (WORD)aros_raw_word(data + 36);
		window->pos.Height = (WORD)aros_raw_word(data + 38);
		CopyMem(data + 40, window->font_name, 80);
		window->font_name[79] = 0;
		window->font_size = data[123];
		window->columns = aros_raw_word(data + 124);
		window->rows = aros_raw_word(data + 126);
		window->flags = aros_raw_long(data + 128);
	}
	else if (size >= sizeof(OLD_CFG_BTNW))
	{
		CopyMem(data, window->name, 32);
		window->name[31] = 0;
		window->pos.Left = (WORD)aros_raw_word(data + 32);
		window->pos.Top = (WORD)aros_raw_word(data + 34);
		window->pos.Width = (WORD)aros_raw_word(data + 36);
		window->pos.Height = (WORD)aros_raw_word(data + 38);
		CopyMem(data + 40, window->font_name, 31);
		window->font_name[31] = 0;
		window->font_size = data[71];
		window->columns = aros_raw_word(data + 72);
		window->rows = aros_raw_word(data + 74);
		window->flags = aros_raw_long(data + 76);
	}

	if (window->columns == 0)
		window->columns = 1;
	if (window->rows == 0)
		window->rows = 1;
	window->flags &= ~BTNWF_TOOLBAR;
}

static void aros_raw_fix_function_ix(Cfg_Function *func)
{
	if (!(func->function.flags2 & FUNCF2_VALID_IX))
	{
		func->function.qual_mask = 0xffff;
		func->function.qual_same = 0;

		if (func->function.qual & (IEQUALIFIER_LALT | IEQUALIFIER_RALT))
			func->function.qual_same |= IXSYM_ALT;
		if (func->function.qual & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
			func->function.qual_same |= IXSYM_SHIFT;

		func->function.flags2 |= FUNCF2_VALID_IX;
	}
}

static void aros_raw_read_function(Cfg_ButtonFunction *button_func, UBYTE *data, ULONG size, APTR memory)
{
	Cfg_Function *func = (Cfg_Function *)button_func;
	UBYTE *ptr;
	ULONG remain;
	char *string;

	if (size < sizeof(CFG_FUNC))
		return;

	func->function.flags = aros_raw_long(data);
	func->function.flags2 = aros_raw_long(data + 4);
	func->function.pad_1 = aros_raw_long(data + 8);
	func->function.code = aros_raw_word(data + 12);
	func->function.qual = aros_raw_word(data + 14);
	func->function.func_type = aros_raw_word(data + 16);
	func->function.qual_mask = aros_raw_word(data + 18);
	func->function.qual_same = aros_raw_word(data + 20);
	aros_raw_fix_function_ix(func);

	ptr = data + sizeof(CFG_FUNC);
	remain = size - sizeof(CFG_FUNC);

	while ((string = aros_raw_copy_string(memory, &ptr, &remain)))
	{
		Cfg_Instruction *ins;
		short type;

		if (remain < 2)
		{
			L_FreeMemH(string);
			break;
		}

		type = (short)aros_raw_word(ptr);
		ptr += 2;
		remain -= 2;

		if ((ins = L_NewInstruction(memory, type, 0)))
		{
			ins->string = string;
			AddTail((struct List *)&func->instructions, (struct Node *)ins);
		}
		else
		{
			L_FreeMemH(string);
			break;
		}
	}
}

static Cfg_Button *aros_raw_read_button(Cfg_ButtonBank *bank, UBYTE *data, ULONG size)
{
	Cfg_Button *button;
	UBYTE *ptr;
	ULONG remain;
	short count, a;
	static short image_log_count = 0;

	if (size < AROS_CFG_BUTN_DISK_SIZE || !(button = L_NewButton(bank->memory)))
		return 0;

	button->button.fpen = data[0];
	button->button.bpen = data[1];
	button->button.flags = aros_raw_long(data + 2);
	button->button.count = (short)aros_raw_word(data + 6);
	button->button.pad_1 = (short)aros_raw_word(data + 8);
	button->button.pad_2 = aros_raw_long(data + 10);

	if (!(button->button.flags & BUTNF_NEW_FORMAT))
		count = 3;
	else
		count = button->button.count;

	if (count < 1 || count > 64)
		return button;

	ptr = data + AROS_CFG_BUTN_DISK_SIZE;
	remain = size - AROS_CFG_BUTN_DISK_SIZE;

	for (a = 0; a < count; a++)
	{
		Cfg_ButtonFunction *func;
		char *name, *label = 0;

		if (!(name = aros_raw_copy_string(bank->memory, &ptr, &remain)))
			break;

		if (button->button.flags & BUTNF_NEW_FORMAT)
		{
			label = aros_raw_copy_string(bank->memory, &ptr, &remain);
			if (label && label[0] == 1 && label[1] == 0)
			{
				L_FreeMemH(label);
				label = copy_string(bank->memory, name);
			}
		}
		else if (button->button.flags & BUTNF_GRAPHIC)
		{
			char filename[40];
			char *dot;

			stccpy(filename, FilePart(name), 39);
			if ((dot = strchr(filename, '.')))
				*dot = 0;
			label = copy_string(bank->memory, filename);
		}
		else
			label = copy_string(bank->memory, name);

		if (!label)
		{
			L_FreeMemH(name);
			break;
		}

		if ((func = L_NewButtonFunction(bank->memory, 0)))
		{
			func->label = name;
			func->node.ln_Name = label;
			if (button->button.flags & BUTNF_GRAPHIC)
			{
				func->image = open_button_image(name);
				if (image_log_count < 24)
				{
					aros_config_debug_log("OpenButtonBank raw image ");
					aros_config_debug_log(name);
					aros_config_debug_log((func->image) ? " ok\n" : " failed\n");
					++image_log_count;
				}
			}
			AddTail((struct List *)&button->function_list, &func->node);
		}
		else
		{
			L_FreeMemH(name);
			L_FreeMemH(label);
			break;
		}
	}

	return button;
}

static void aros_raw_read_startmenu(Cfg_ButtonBank *bank, UBYTE *data, ULONG size)
{
	if (size < sizeof(CFG_STRT))
		return;

	if ((bank->startmenu = L_AllocMemH(bank->memory, sizeof(CFG_STRT))))
	{
		CopyMem(data, bank->startmenu, sizeof(CFG_STRT));
		bank->startmenu->label[39] = 0;
		bank->startmenu->image[255] = 0;
		bank->startmenu->flags = aros_raw_long(data + 296);
		bank->startmenu->fpen = (short)aros_raw_word(data + 300);
		bank->startmenu->label_fpen = (short)aros_raw_word(data + 302);
		bank->startmenu->sel_fpen = (short)aros_raw_word(data + 304);
		bank->startmenu->label_fontname[79] = 0;
		bank->startmenu->label_fontsize = (short)aros_raw_word(data + 420);
	}
}

static Cfg_ButtonBank *aros_open_button_bank_raw(char *raw_name, char *path_name)
{
	Cfg_ButtonBank *bank = 0;
	BPTR file;
	UBYTE *buffer = 0;
	LONG file_size = 0;
	LONG read_size;
	ULONG button_chunks = 0, func_chunks = 0, buttons_read = 0;
	char logbuf[180];

	if (!raw_name)
		return 0;

	aros_config_debug_log("OpenButtonBank raw parser start path=");
	aros_config_debug_log(raw_name);
	aros_config_debug_log("\n");

	if (!(file = Open(raw_name, MODE_OLDFILE)))
	{
		aros_config_debug_log("OpenButtonBank raw parser open failed\n");
		return 0;
	}

	while (file)
	{
		D_S(struct FileInfoBlock, fib)
		ULONG offset;
		Cfg_ButtonFunction *current_func = 0;

		if (!ExamineFH(file, fib) || fib->fib_Size < 12)
		{
			aros_config_debug_log("OpenButtonBank raw parser examine failed\n");
			break;
		}
		file_size = fib->fib_Size;
		lsprintf(logbuf, "OpenButtonBank raw parser size=%ld\n", file_size);
		aros_config_debug_log(logbuf);

		if (!(buffer = AllocVec(file_size, MEMF_CLEAR)))
		{
			aros_config_debug_log("OpenButtonBank raw parser alloc failed\n");
			break;
		}

		Seek(file, 0, OFFSET_BEGINNING);
		read_size = Read(file, buffer, file_size);
		if (read_size != file_size)
		{
			lsprintf(logbuf, "OpenButtonBank raw parser read failed got=%ld\n", read_size);
			aros_config_debug_log(logbuf);
			break;
		}

		if (!aros_raw_id(buffer, "FORM") ||
			(!aros_raw_id(buffer + 8, "OPUS") && !aros_raw_id(buffer + 8, "EPUS")))
		{
			aros_config_debug_log("OpenButtonBank raw parser header failed\n");
			break;
		}

		for (offset = 12; offset + 8 <= (ULONG)file_size;)
		{
			UBYTE *chunk = buffer + offset;
			ULONG chunk_size = aros_raw_long(chunk + 4);
			UBYTE *chunk_data = chunk + 8;

			offset += 8;
			if (offset + chunk_size > (ULONG)file_size)
				break;

			if (aros_raw_id(chunk, "BTNW"))
			{
				current_func = 0;
				if (!bank && (bank = L_NewButtonBank(0, 0)))
				{
					stccpy(bank->path, (path_name && path_name[0]) ? path_name : raw_name, 256);
					aros_raw_read_button_window(&bank->window, chunk_data, chunk_size);
				}
			}
			else if (bank && aros_raw_id(chunk, "BPIC"))
			{
				ULONG copy_size = (chunk_size < sizeof(bank->backpic) - 1) ? chunk_size : sizeof(bank->backpic) - 1;

				CopyMem(chunk_data, bank->backpic, copy_size);
				bank->backpic[copy_size] = 0;
			}
			else if (bank && aros_raw_id(chunk, "BUTN"))
			{
				Cfg_Button *button;

				++button_chunks;
				current_func = 0;
				if ((button = aros_raw_read_button(bank, chunk_data, chunk_size)))
				{
					AddTail(&bank->buttons, &button->node);
					++buttons_read;
					current_func = (Cfg_ButtonFunction *)button->function_list.mlh_Head;
				}
			}
			else if (bank && aros_raw_id(chunk, "FUNC"))
			{
				++func_chunks;
				if (current_func && current_func->node.ln_Succ)
				{
					aros_raw_read_function(current_func, chunk_data, chunk_size, bank->memory);
					current_func = (Cfg_ButtonFunction *)current_func->node.ln_Succ;
				}
			}
			else if (bank && aros_raw_id(chunk, "STRT"))
				aros_raw_read_startmenu(bank, chunk_data, chunk_size);

			offset += chunk_size + (chunk_size & 1);
		}
		break;
	}

	lsprintf(logbuf,
			 "OpenButtonBank raw parser done buttons=%ld button_chunks=%ld func_chunks=%ld bank=%ld\n",
			 buttons_read,
			 button_chunks,
			 func_chunks,
			 (bank) ? 1 : 0);
	aros_config_debug_log(logbuf);

	FreeVec(buffer);
	Close(file);

	if (bank && IsListEmpty(&bank->buttons))
	{
		L_CloseButtonBank(bank);
		bank = 0;
	}

	return bank;
}
#endif

// Read some filetype definitions
Cfg_FiletypeList *LIBFUNC L_ReadFiletypes(REG(a0, char *name), REG(a1, APTR memory))
{
	struct _IFFHandle *iff;
	Cfg_FiletypeList *list = 0;
	Cfg_Filetype *type = 0;
	short count = 0;

	// Try to open file to read
	if ((iff = L_IFFOpen(name, IFF_READ, ID_OPUS)))
	{
		ULONG chunk;

		// Parse file
		while ((chunk = L_IFFNextChunk(iff, 0)))
		{
			// Look at chunk type
			switch (chunk)
			{
			// Filetype definition
			case ID_TYPE:

				// If we don't have a list, create one
				if (!list)
				{
					D_S(struct FileInfoBlock, fib)

					// Create list entry
					if (!(list = L_AllocMemH(memory, sizeof(Cfg_FiletypeList))))
						break;

					// Initialise filetype list
					NewList(&list->filetype_list);

					// Copy path
					strcpy(list->path, name);

					// Get file date
					if (L_ExamineBuf(L_IFFFileHandle(iff), fib))
						list->date = fib->fib_Date;
				}

				// Allocate a new filetype
				if ((type = L_NewFiletype(memory)))
				{
					string_handle handle;
					short a;

					// Read filetype data
					L_IFFReadChunkBytes(iff, &type->type, sizeof(CFG_TYPE));

#ifdef __AROS__
					type->type.flags = AROS_BE2WORD(type->type.flags);
					type->type.count = AROS_BE2WORD(type->type.count);
#endif

					// Initialise strings
					init_config_string(iff, &handle);

					// Read recognition string
					type->recognition = (unsigned char *)read_config_string(memory, &handle);

					// Read icon path
					type->icon_path = read_config_string(memory, &handle);

					// Read actions
					for (a = 0; a < 16; a++)
					{
						type->actions[a] = read_config_string(memory, &handle);
					}
					free_config_string(&handle);

					// Add to list
					type->node.ln_Pri = type->type.priority;
					Enqueue(&list->filetype_list, &type->node);

					// Store priority in list
					if (count == 0 || type->type.priority < list->node.ln_Pri)
						list->node.ln_Pri = type->type.priority;

					// Store pointer to list
					type->list = list;
					++count;
				}
				break;

			// Function definition
			case ID_FUNC:

				// Read function
				if (type)
					L_ReadFunction(iff, memory, &type->function_list, 0);
				break;
			}
		}

		L_IFFClose(iff);
	}

	return list;
}

// Read a function
Cfg_Function *LIBFUNC L_ReadFunction(REG(a0, struct _IFFHandle *iff),
									 REG(a1, APTR memory),
									 REG(a2, struct List *func_list),
									 REG(a3, Cfg_Function *function))
{
	Cfg_Function *func;
	string_handle handle;
	char *string;
	short type;

	// Function supplied?
	if (function)
		func = function;

	// Allocate a new function
	else if (!(func = L_NewFunction(memory, 0)))
		return 0;

	// Add to function list
	else if (func_list)
		AddTail(func_list, &func->node);

	// Read function data
	L_IFFReadChunkBytes(iff, &func->function, sizeof(CFG_FUNC));

#ifdef __AROS__
	func->function.flags = AROS_BE2LONG(func->function.flags);
	func->function.flags2 = AROS_BE2LONG(func->function.flags2);
	func->function.code = AROS_BE2WORD(func->function.code);
	func->function.qual = AROS_BE2WORD(func->function.qual);
	func->function.func_type = AROS_BE2WORD(func->function.func_type);
	func->function.qual_mask = AROS_BE2WORD(func->function.qual_mask);
	func->function.qual_same = AROS_BE2WORD(func->function.qual_same);
#endif

	// Old-style function?
	if (!(func->function.flags2 & FUNCF2_VALID_IX))
	{
		// Fix IX
		func->function.qual_mask = 0xffff;
		func->function.qual_same = 0;

		// Fix alt
		if (func->function.qual & (IEQUALIFIER_LALT | IEQUALIFIER_RALT))
			func->function.qual_same |= IXSYM_ALT;

		// Fix shift
		if (func->function.qual & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
			func->function.qual_same |= IXSYM_SHIFT;

		// Mark as a valid new function
		func->function.flags2 |= FUNCF2_VALID_IX;
	}

	// Read function instructions
	init_config_string(iff, &handle);
	while ((string = read_config_string(memory, &handle)))
	{
		Cfg_Instruction *ins;

		// Read instruction type
		type = read_config_short(&handle);

		// Allocate new instruction
		if ((ins = L_NewInstruction(memory, type, 0)))
		{
			// Initialise instruction
			ins->string = string;

			// Add to instruction list
			AddTail((struct List *)&func->instructions, (struct Node *)ins);
		}

		// Failed
		else
		{
			L_FreeMemH(string);
			break;
		}
	}
	free_config_string(&handle);

	return func;
}

void init_config_string(struct _IFFHandle *iff, string_handle *handle)
{
	long size;

	handle->buffer = 0;
	handle->pointer = 0;
	handle->size = 0;
	handle->error = 0;

	// See how much is left to read
	if ((handle->size = L_IFFChunkRemain(iff)) < 1)
	{
		handle->size = 0;
		return;
	}

	// Allocate buffer
	if (!(handle->buffer = AllocVec(handle->size + 1, MEMF_CLEAR)))
		return;

	// Read data
	if ((size = L_IFFReadChunkBytes(iff, handle->buffer, handle->size)) < 0)
	{
		handle->error = 1;
		return;
	}
	handle->size = size;

	// Initialise pointer
	handle->pointer = handle->buffer;
}

char *read_config_string(APTR memory, string_handle *handle)
{
	int len;
	char *string;

	if (!handle->pointer || handle->size < 1 || handle->error)
		return 0;

	// Find next null
	for (len = 0; len < handle->size && handle->pointer[len]; len++)
		;

	// Allocate data for string
	if ((string = L_AllocMemH(memory, len + 1)))

		// Copy string
		strcpy(string, handle->pointer);

	// Bump pointer, decrement size
	handle->pointer += len + 1;
	handle->size -= len + 1;

	// Return string pointer
	return string;
}

short read_config_short(string_handle *handle)
{
	short value;

	// Valid buffer?
	if (!handle->pointer || handle->size < 2)
		return 0;

	// Get value
	value = (handle->pointer[0] << 16) | handle->pointer[1];

	// Bump pointer, decrement size
	handle->pointer += 2;
	handle->size -= 2;

	// Return value
	return value;
}

void free_config_string(string_handle *handle)
{
	FreeVec(handle->buffer);
}

// Convert start menu to version 2
void LIBFUNC L_ConvertStartMenu(REG(a0, Cfg_ButtonBank *bank))
{
	Cfg_Button *button, *prev = 0, *next_but = 0;

	// Find first button and remove it - no longer needed
	if ((button = (Cfg_Button *)RemHead(&bank->buttons)))
	{
		// Free button
		L_FreeButton(button);
	}

	// Go through remaining buttons
	for (button = (Cfg_Button *)bank->buttons.lh_Head; button->node.ln_Succ; button = next_but)
	{
		Cfg_ButtonFunction *func, *next = 0;
		BOOL first = 1;

		// Cache next button
		next_but = (Cfg_Button *)button->node.ln_Succ;

		// Go through all functions in the button
		for (func = (Cfg_ButtonFunction *)button->function_list.mlh_Head; func->node.ln_Succ; func = next)
		{
			Cfg_Button *newbut;

			// Cache next function
			next = (Cfg_ButtonFunction *)func->node.ln_Succ;

			// Set the type back to 0
			func->function.func_type = FTYPE_LEFT_BUTTON;

			// Remove the function
			Remove((struct Node *)func);

			// Empty function?
			if (IsListEmpty((struct List *)&func->instructions) && (!func->node.ln_Name || !*func->node.ln_Name) &&
				(!func->label || !*func->label))
			{
				// Free function, don't add it
				L_FreeButtonFunction(func);
			}

			// Make a new button for the menu title
			else if ((newbut = L_NewButton(bank->memory)))
			{
				// First function in the button means it's a title
				if (first)
				{
					// Set flag in that button to say it's a menu title
					newbut->button.flags |= BUTNF_TITLE | BUTNF_NEW_FORMAT;
					first = 0;
				}
				else
					newbut->button.flags |= BUTNF_NEW_FORMAT;

				// Add the function we removed to the new button
				AddHead((struct List *)&newbut->function_list, &func->node);

				// Add the new button to the button list before the current button
				Insert(&bank->buttons, &newbut->node, (struct Node *)prev);

				// Save button pointer for next time
				prev = newbut;
			}

			// Failed
			else
				L_FreeButtonFunction(func);
		}

		// Free the old button
		Remove((struct Node *)button);
		L_FreeButton(button);
	}

	// Set flag to say bank has been converted
	if (bank->startmenu)
		bank->startmenu->flags |= STRTF_VERSION2;
}

/*
void dump_button_info(Cfg_ButtonBank *bank)
{
	Cfg_Button *button;
	for (button=(Cfg_Button *)bank->buttons.lh_Head;button->node.ln_Succ;button=(Cfg_Button *)button->node.ln_Succ)
	{
		Cfg_ButtonFunction *func;
		D(bug("button flags %lx (title %lx item %lx sub %lx)\n",button->button.flags,
			button->button.flags&BUTNF_TITLE,button->button.flags&BUTNF_ITEM,button->button.flags&BUTNF_SUB));
		for (func=(Cfg_ButtonFunction *)button->function_list.mlh_Head;func->node.ln_Succ;func=(Cfg_ButtonFunction
*)func->node.ln_Succ)
		{
			Cfg_Instruction *ins;

			D(bug("name %s label %s flags %lx type
%ld\n",func->node.ln_Name,func->label,func->function.flags2,func->function.func_type)); for (ins=(Cfg_Instruction
*)func->instructions.mlh_Head;ins->node.mln_Succ;ins=(Cfg_Instruction *)ins->node.mln_Succ) D(bug("\t%ld
%s\n",ins->type,ins->string));
		}
		D(bug("------------\n"));
	}
}
*/

BOOL LIBFUNC L_OpenEnvironment(REG(a0, char *name), REG(a1, struct OpenEnvironmentData *data))
{
	APTR iff;
	ULONG chunk;
	char buf[256];

	// Initialise
	NewList((struct List *)&data->desktop);
	NewList((struct List *)&data->pathlist);
	NewList((struct List *)&data->soundlist);
	NewList((struct List *)&data->startmenus);
	NewList((struct List *)&data->buttons);

	NewList((struct List *)&data->listers);
	data->toolbar_path[0] = 0;
	data->menu_path[0] = 0;
	data->user_menu_path[0] = 0;
	data->scripts_path[0] = 0;
	data->hotkeys_path[0] = 0;

	// Try to open file to read
	if ((iff = L_IFFOpen(name, IFF_READ, ID_OPUS)))
		iff_file_id = ID_OPUS;
	else if ((iff = L_IFFOpen(name, IFF_READ, ID_EPUS)))
		iff_file_id = ID_EPUS;
	else
		return 0;

	// Parse file
	while ((chunk = L_IFFNextChunk(iff, 0)))
	{
		// Look at chunk type
		switch (chunk)
		{
		// Environment settings
		case ID_ENVR:
			if (!(data->flags & OEDF_ENVR))
				break;
			if (iff_file_id == ID_OPUS)
			{
				if (!convert_env(iff, &data->env))
				{
					L_IFFClose(iff);
					iff_file_id = 0;
					return 0;
				}
			}
			else
			{
				memset(&data->env, 0, sizeof(data->env));
				L_IFFReadChunkBytes(iff, &data->env, sizeof(CFG_ENVR));
			}
#ifdef __AROS__
			{
				int i;

				data->env.screen_mode = AROS_BE2LONG(data->env.screen_mode);
				data->env.screen_flags = AROS_BE2WORD(data->env.screen_flags);
				data->env.screen_depth = AROS_BE2WORD(data->env.screen_depth);
				data->env.screen_width = AROS_BE2WORD(data->env.screen_width);
				data->env.screen_height = AROS_BE2WORD(data->env.screen_height);

				for (i = 0; i < 50; i++)
					data->env.palette[i] = AROS_BE2LONG(data->env.palette[i]);

				// User pen colours 9-16 (CONFIG_VERSION_19)
				for (i = 0; i < 24; i++)
					data->env.palette2[i] = AROS_BE2LONG(data->env.palette2[i]);

				data->env.window_pos.Left = AROS_BE2WORD(data->env.window_pos.Left);
				data->env.window_pos.Top = AROS_BE2WORD(data->env.window_pos.Top);
				data->env.window_pos.Width = AROS_BE2WORD(data->env.window_pos.Width);
				data->env.window_pos.Height = AROS_BE2WORD(data->env.window_pos.Height);
				data->env.general_screen_flags = AROS_BE2LONG(data->env.general_screen_flags);
				data->env.palette_count = AROS_BE2WORD(data->env.palette_count);

				for (i = 0; i < CUST_PENS; i++)
				{
					data->env.env_Colours[i][0][0] = AROS_BE2LONG(data->env.env_Colours[i][0][0]);
					data->env.env_Colours[i][0][1] = AROS_BE2LONG(data->env.env_Colours[i][0][1]);
					data->env.env_Colours[i][0][2] = AROS_BE2LONG(data->env.env_Colours[i][0][2]);
					data->env.env_Colours[i][1][0] = AROS_BE2LONG(data->env.env_Colours[i][1][0]);
					data->env.env_Colours[i][1][1] = AROS_BE2LONG(data->env.env_Colours[i][1][1]);
					data->env.env_Colours[i][1][2] = AROS_BE2LONG(data->env.env_Colours[i][1][2]);
				}

				data->env.env_ColourFlag = AROS_BE2LONG(data->env.env_ColourFlag);
				data->env.env_NewIconsFlags = AROS_BE2LONG(data->env.env_NewIconsFlags);
				data->env.display_options = AROS_BE2WORD(data->env.display_options);
				data->env.main_window_type = AROS_BE2WORD(data->env.main_window_type);
				data->env.hotkey_flags = AROS_BE2LONG(data->env.hotkey_flags);
				data->env.hotkey_code = AROS_BE2WORD(data->env.hotkey_code);
				data->env.hotkey_qual = AROS_BE2WORD(data->env.hotkey_qual);
				data->env.default_stack = AROS_BE2LONG(data->env.default_stack);
				data->env.lister_options = AROS_BE2WORD(data->env.lister_options);
				data->env.flags = AROS_BE2LONG(data->env.flags);
				data->env.lister_popup_code = AROS_BE2WORD(data->env.lister_popup_code);
				data->env.lister_popup_qual = AROS_BE2WORD(data->env.lister_popup_qual);
				data->env.env_flags = AROS_BE2LONG(data->env.env_flags);
				data->env.clock_left = AROS_BE2WORD(data->env.clock_left);
				data->env.clock_top = AROS_BE2WORD(data->env.clock_top);
				data->env.lister_width = AROS_BE2WORD(data->env.lister_width);
				data->env.lister_height = AROS_BE2WORD(data->env.lister_height);
				data->env.version = AROS_BE2WORD(data->env.version);
				data->env.desktop_flags = AROS_BE2LONG(data->env.desktop_flags);

				// Promoted ENV: vars from issue #49 (CONFIG_VERSION_14/15) -- byte-swap so
				// configs are portable between AROS and big-endian targets.  env_pad_15
				// gets the swap proactively: it's currently always zero (byte-swap
				// invariant), but if anyone repurposes the slot for a real UWORD value
				// the swap is already in place rather than waiting for a corruption bug.
				data->env.env_icon_space_x = AROS_BE2WORD(data->env.env_icon_space_x);
				data->env.env_icon_space_y = AROS_BE2WORD(data->env.env_icon_space_y);
				data->env.env_icon_grid_x = AROS_BE2WORD(data->env.env_icon_grid_x);
				data->env.env_icon_grid_y = AROS_BE2WORD(data->env.env_icon_grid_y);
				data->env.env_wheel_scroll_lines = AROS_BE2WORD(data->env.env_wheel_scroll_lines);
				data->env.env_pad_15 = AROS_BE2WORD(data->env.env_pad_15);

				for (i = 0; i < 4; i++)
					data->env.env_BackgroundFlags[i] = AROS_BE2WORD(data->env.env_BackgroundFlags[i]);

				data->env.settings.copy_flags = AROS_BE2LONG(data->env.settings.copy_flags);
				data->env.settings.delete_flags = AROS_BE2LONG(data->env.settings.delete_flags);
				data->env.settings.error_flags = AROS_BE2LONG(data->env.settings.error_flags);
				data->env.settings.general_flags = AROS_BE2LONG(data->env.settings.general_flags);
				data->env.settings.icon_flags = AROS_BE2LONG(data->env.settings.icon_flags);
				data->env.settings.replace_method = AROS_BE2WORD(data->env.settings.replace_method);
				data->env.settings.replace_flags = AROS_BE2WORD(data->env.settings.replace_flags);
				data->env.settings.update_flags = AROS_BE2LONG(data->env.settings.update_flags);
				data->env.settings.dir_flags = AROS_BE2LONG(data->env.settings.dir_flags);
				data->env.settings.view_flags = AROS_BE2LONG(data->env.settings.view_flags);
				data->env.settings.max_buffer_count = AROS_BE2WORD(data->env.settings.max_buffer_count);
				data->env.settings.date_format = AROS_BE2WORD(data->env.settings.date_format);
				data->env.settings.date_flags = AROS_BE2WORD(data->env.settings.date_flags);
				data->env.settings.pop_code = AROS_BE2WORD(data->env.settings.pop_code);
				data->env.settings.pop_qual = AROS_BE2WORD(data->env.settings.pop_qual);
				data->env.settings.pop_qual_mask = AROS_BE2WORD(data->env.settings.pop_qual_mask);
				data->env.settings.pop_qual_same = AROS_BE2WORD(data->env.settings.pop_qual_same);
				data->env.settings.popup_delay = AROS_BE2WORD(data->env.settings.popup_delay);
				data->env.settings.max_openwith = AROS_BE2WORD(data->env.settings.max_openwith);
				data->env.settings.command_line_length = AROS_BE2WORD(data->env.settings.command_line_length);
				data->env.settings.max_filename = AROS_BE2WORD(data->env.settings.max_filename);
				data->env.settings.flags = AROS_BE2LONG(data->env.settings.flags);

				for (i = 0; i < 4; i++)
					data->env.env_BackgroundBorderColour[i] = AROS_BE2LONG(data->env.env_BackgroundBorderColour[i]);
			}
#endif
			break;

		// Settings file name
		case ID_SETS:
			if (!(data->flags & OEDF_SETS))
				break;
			L_IFFReadChunkBytes(iff, buf, 256);
			L_ReadSettings(&data->env.settings, buf);
			break;

		// Toolbar file name
		case ID_TBAR:
			if (!(data->flags & OEDF_TBAR))
				break;
			L_IFFReadChunkBytes(iff, data->toolbar_path, 256);
			break;

		// Lister menu file name
		case ID_LMEN:
			if (!(data->flags & OEDF_LMEN))
				break;
			L_IFFReadChunkBytes(iff, data->menu_path, 256);
			break;

		// Menu file name
		case ID_UMEN:
			if (!(data->flags & OEDF_UMEN))
				break;
			L_IFFReadChunkBytes(iff, data->user_menu_path, 256);
			break;

		// Scripts file name
		case ID_SCRP:
			if (!(data->flags & OEDF_SCRP))
				break;
			L_IFFReadChunkBytes(iff, data->scripts_path, 256);
			break;

		// Hotkeys name
		case ID_HKEY:
			if (!(data->flags & OEDF_HKEY))
				break;
			L_IFFReadChunkBytes(iff, data->hotkeys_path, 256);
			break;

		// Button bank to open
		case ID_BTBK:
		case ID_BANK:
		case ID_STRT:
			if (!(data->flags & OEDF_BANK))
				break;
			env_read_open_bank(iff, data, chunk);
			break;

		// Lister to open
		case ID_LSTR:
		case ID_DLST:
			if (!(data->flags & OEDF_LSTR))
				break;
			env_read_open_lister(iff, data, chunk);
			break;

		// Desktop information
		case ID_DESK:
			if (data->flags & OEDF_DESK)
			{
				Cfg_Desktop *desk;

				// Allocate desktop entry
				if ((desk = L_AllocMemH(data->memory, L_IFFChunkSize(iff) + sizeof(struct MinNode))))
				{
					// Read data
					L_IFFReadChunkBytes(iff, &desk->data, L_IFFChunkSize(iff));

#ifdef __AROS__
					desk->data.dt_Type = AROS_BE2WORD(desk->data.dt_Type);
					desk->data.dt_Size = AROS_BE2WORD(desk->data.dt_Size);
					desk->data.dt_Flags = AROS_BE2LONG(desk->data.dt_Flags);
					if (desk->data.dt_Type != DESKTOP_HIDE_BAD && desk->data.dt_Type != DESKTOP_HIDE)
						desk->data.dt_Data = AROS_BE2LONG(desk->data.dt_Data);
#endif

					AddTail((struct List *)&data->desktop, (struct Node *)desk);
				}
			}
			break;

		// Path list entry
		case ID_PATH:
			if (data->flags & OEDF_PATH)
			{
				struct MinNode *node;

				// Allocate entry
				if ((node = L_AllocMemH(data->memory, L_IFFChunkSize(iff) + sizeof(struct MinNode) + 1)))
				{
					// Read data, add to list
					L_IFFReadChunkBytes(iff, (char *)(node + 1), L_IFFChunkSize(iff));
					AddTail((struct List *)&data->pathlist, (struct Node *)node);
				}
			}
			break;

		// Sound list entry
		case ID_SNDX:
			if (data->flags & OEDF_SNDX)
			{
				Cfg_SoundEntry *sound;

				// Allocate entry
				if ((sound = L_AllocMemH(data->memory, sizeof(Cfg_SoundEntry))))
				{
					long sndbytes = L_IFFChunkSize(iff);
					long sndmax = (long)(offsetof(Cfg_SoundEntry, dse_Random) - offsetof(Cfg_SoundEntry, dse_Name));

					// Read data, add to list
					memset(sound, 0, sizeof(Cfg_SoundEntry));
					if (sndbytes < 0)
						sndbytes = 0;
					else if (sndbytes > sndmax)
						sndbytes = sndmax;

					L_IFFReadChunkBytes(iff, (char *)sound->dse_Name, sndbytes);
					sound->dse_Name[sizeof(sound->dse_Name) - 1] = 0;
					sound->dse_Sound[sizeof(sound->dse_Sound) - 1] = 0;

#ifdef __AROS__
					sound->dse_Volume = AROS_BE2WORD(sound->dse_Volume);
					sound->dse_Flags = AROS_BE2WORD(sound->dse_Flags);
					// sound->dse_RandomVolume = AROS_BE2WORD(sound->dse_RandomVolume);
#endif

					sound->dse_Node.ln_Name = sound->dse_Name;
					sound->dse_Flags &= ~CFGSEF_CHANGED;
					AddTail((struct List *)&data->soundlist, (struct Node *)sound);
				}
			}
			break;
		}
	}

	// Close file
	L_IFFClose(iff);
	if (iff_file_id == ID_OPUS)
		do_backup(name);
	iff_file_id = 0;

	// Bring environment file up to date
	if ((data->flags & OEDF_ENVR) && (data->flags & OEDF_SETS))
		L_UpdateEnvironment(&data->env);
	return 1;
}

// Read info on a button bank to open
void env_read_open_bank(APTR iff, struct OpenEnvironmentData *data, ULONG id)
{
	ButtonBankNode *node;
	char path[256];

	// Allocate new node
	if ((node = L_AllocMemH(data->volatile_memory, sizeof(ButtonBankNode))))
	{
		ULONG pad[5];

		// Read padding if new version
		if (id == ID_BANK || id == ID_STRT)
		{
			L_IFFReadChunkBytes(iff, pad, sizeof(pad));
			node->icon_pos_x = pad[2];
			node->icon_pos_y = pad[3];
			node->flags = pad[4];
#ifdef __AROS__
			node->icon_pos_x = AROS_BE2LONG(node->icon_pos_x);
			node->icon_pos_y = AROS_BE2LONG(node->icon_pos_y);
			node->flags = AROS_BE2LONG(node->flags);
#endif
		}
		else
			node->icon_pos_x = NO_ICON_POSITION;

		// Read position
		L_IFFReadChunkBytes(iff, &node->pos, sizeof(struct IBox));

#ifdef __AROS__
		node->pos.Left = AROS_BE2WORD(node->pos.Left);
		node->pos.Top = AROS_BE2WORD(node->pos.Top);
		node->pos.Width = AROS_BE2WORD(node->pos.Width);
		node->pos.Height = AROS_BE2WORD(node->pos.Height);
#endif

		// Get path of button bank
		L_IFFReadChunkBytes(iff, path, 256);

		// Store in node
		if ((node->node.ln_Name = L_AllocMemH(data->volatile_memory, strlen(path) + 1)))
			strcpy(node->node.ln_Name, path);

		// Link to list
		if (id == ID_STRT)
			AddTail((struct List *)&data->startmenus, (struct Node *)node);
		else
			AddTail((struct List *)&data->buttons, (struct Node *)node);
	}
}

// Read info on a lister to open
void env_read_open_lister(APTR iff, struct OpenEnvironmentData *data, ULONG id)
{
	OpenListerNode *node;

	// Allocate new node
	if ((node = L_AllocMemH(data->volatile_memory, sizeof(OpenListerNode))))
	{
		// Read definition
		node->lister = (APTR)L_ReadListerDef(iff, id);

		// Link to list
		AddTail((struct List *)&data->listers, (struct Node *)node);
	}
}

// Adjust old CFG_ENVR to new CFG_ENVR
int convert_env(struct _IFFHandle *iff, CFG_ENVR *env)
{
	OLD_CFG_ENVR *oldenv = NULL;
	int i = 0;

	oldenv = AllocVec(sizeof(OLD_CFG_ENVR), MEMF_CLEAR);
	if (!oldenv)
		return 0;
	L_IFFReadChunkBytes(iff, oldenv, (sizeof(OLD_CFG_ENVR)));

	env->screen_mode = oldenv->screen_mode;
	env->screen_flags = oldenv->screen_flags;
	env->screen_depth = oldenv->screen_depth;
	env->screen_width = oldenv->screen_width;
	env->screen_height = oldenv->screen_height;
	for (i = 0; i < 50; i++)
		env->palette[i] = oldenv->palette[i];
	for (i = 0; i < 80; i++)
		env->pubscreen_name[i] = oldenv->pubscreen_name[i];
	env->window_pos.Left = oldenv->window_pos.Left;
	env->window_pos.Top = oldenv->window_pos.Top;
	env->window_pos.Width = oldenv->window_pos.Width;
	env->window_pos.Height = oldenv->window_pos.Height;
	env->general_screen_flags = oldenv->general_screen_flags;
	env->source_col[0] = oldenv->source_col[0];
	env->source_col[1] = oldenv->source_col[1];
	env->dest_col[0] = oldenv->dest_col[0];
	env->dest_col[1] = oldenv->dest_col[1];
	env->palette_count = oldenv->palette_count;
	for (i = 0; i < 40; i++)
	{
		env->font_name[0][i] = oldenv->font_name[0][i];
		env->font_name[1][i] = oldenv->font_name[1][i];
		env->font_name[2][i] = oldenv->font_name[2][i];
		env->font_name[3][i] = oldenv->font_name[3][i];
	}
	for (i = 0; i < CUST_PENS; i++)
	{
		env->env_Colours[i][0][0] = oldenv->env_Colours[i][0][0];
		env->env_Colours[i][0][1] = oldenv->env_Colours[i][0][1];
		env->env_Colours[i][0][2] = oldenv->env_Colours[i][0][2];
		env->env_Colours[i][1][0] = oldenv->env_Colours[i][1][0];
		env->env_Colours[i][1][1] = oldenv->env_Colours[i][1][1];
		env->env_Colours[i][1][2] = oldenv->env_Colours[i][1][2];
	}
	env->env_ColourFlag = oldenv->env_ColourFlag;
	env->env_NewIconsFlags = oldenv->env_NewIconsFlags;
	for (i = 0; i < 80; i++)
		env->status_text[i] = oldenv->status_text[i];
	env->font_size[0] = oldenv->font_size[0];
	env->font_size[1] = oldenv->font_size[1];
	env->font_size[2] = oldenv->font_size[2];
	env->font_size[3] = oldenv->font_size[3];
	env->display_options = oldenv->display_options;
	env->main_window_type = oldenv->main_window_type;
	env->hotkey_flags = oldenv->hotkey_flags;
	env->hotkey_code = oldenv->hotkey_code;
	env->hotkey_qual = oldenv->hotkey_qual;
	convert_list_format(&oldenv->list_format, &env->list_format);
	for (i = 0; i < 80; i++)
		env->backdrop_prefs[i] = oldenv->backdrop_prefs[i];
	for (i = 0; i < 240; i++)
		env->desktop_location[i] = oldenv->desktop_location[i];
	for (i = 0; i < 80; i++)
		env->output_window[i] = oldenv->output_window[i];
	for (i = 0; i < 80; i++)
		env->output_device[i] = oldenv->output_device[i];
	env->default_stack = oldenv->default_stack;
	for (i = 0; i < 188; i++)
		env->scr_title_text[i] = oldenv->scr_title_text[i];
	env->lister_options = oldenv->lister_options;
	env->flags = oldenv->flags;
	env->lister_popup_code = oldenv->lister_popup_code;
	env->lister_popup_qual = oldenv->lister_popup_qual;
	env->env_flags = oldenv->env_flags;
	env->clock_left = oldenv->clock_left;
	env->clock_top = oldenv->clock_top;
	env->devices_col[0] = oldenv->devices_col[0];
	env->devices_col[1] = oldenv->devices_col[1];
	env->volumes_col[0] = oldenv->volumes_col[0];
	env->volumes_col[1] = oldenv->volumes_col[1];
	env->lister_width = oldenv->lister_width;
	env->lister_height = oldenv->lister_height;
	env->version = oldenv->version;
	env->gauge_col[0] = oldenv->gauge_col[0];
	env->gauge_col[1] = oldenv->gauge_col[1];
	env->tooltip_col[0] = 1;
	env->tooltip_col[1] = 0;
	env->icon_fpen = oldenv->icon_fpen;
	env->icon_bpen = oldenv->icon_bpen;
	env->icon_style = oldenv->icon_style;
	env->env_NewIconsPrecision = oldenv->env_NewIconsPrecision;
	env->desktop_flags = oldenv->desktop_flags;
	env->iconw_fpen = oldenv->iconw_fpen;
	env->iconw_bpen = oldenv->iconw_bpen;
	env->iconw_style = oldenv->iconw_style;
	env->desktop_popup_default = oldenv->desktop_popup_default;
	for (i = 0; i < 256; i++)
	{
		env->env_BackgroundPic[0][i] = oldenv->env_BackgroundPic[0][i];
		env->env_BackgroundPic[1][i] = oldenv->env_BackgroundPic[1][i];
		env->env_BackgroundPic[2][i] = oldenv->env_BackgroundPic[2][i];
		env->env_BackgroundPic[3][i] = oldenv->env_BackgroundPic[3][i];
	}
	for (i = 0; i < 4; i++)
		env->env_BackgroundFlags[i] = oldenv->env_BackgroundFlags[i];
	// ***** CFG_SETS
	env->settings.copy_flags = oldenv->settings.copy_flags;
	env->settings.delete_flags = oldenv->settings.delete_flags;
	env->settings.error_flags = oldenv->settings.error_flags;
	env->settings.general_flags = oldenv->settings.general_flags;
	env->settings.icon_flags = oldenv->settings.icon_flags;
	env->settings.replace_method = oldenv->settings.replace_method;
	env->settings.replace_flags = oldenv->settings.replace_flags;
	env->settings.update_flags = oldenv->settings.update_flags;
	env->settings.dir_flags = oldenv->settings.dir_flags;
	env->settings.view_flags = oldenv->settings.view_flags;
	env->settings.hide_method = oldenv->settings.hide_method;
	env->settings.max_buffer_count = oldenv->settings.max_buffer_count;
	env->settings.date_format = oldenv->settings.date_format;
	env->settings.date_flags = oldenv->settings.date_flags;
	env->settings.pri_main[0] = oldenv->settings.pri_main[0];
	env->settings.pri_main[1] = oldenv->settings.pri_main[1];
	env->settings.pri_lister[0] = oldenv->settings.pri_lister[0];
	env->settings.pri_lister[1] = oldenv->settings.pri_lister[1];
	env->settings.flags = oldenv->settings.flags;
	env->settings.pop_code = oldenv->settings.pop_code;
	env->settings.pop_qual = oldenv->settings.pop_qual;
	env->settings.pop_qual_mask = oldenv->settings.pop_qual_mask;
	env->settings.pop_qual_same = oldenv->settings.pop_qual_same;
	env->settings.popup_delay = oldenv->settings.popup_delay;
	env->settings.max_openwith = oldenv->settings.max_openwith;
	env->settings.command_line_length = oldenv->settings.command_line_length;
	env->settings.max_filename = oldenv->settings.max_filename;
	// ***** CFG_SETS end
	for (i = 0; i < 4; i++)
		env->env_BackgroundBorderColour[i] = oldenv->env_BackgroundBorderColour[i];
	for (i = 0; i < 256; i++)
		env->themes_location[i] = oldenv->themes_location[i];
	FreeVec(oldenv);
	return 1;
}

void convert_list_format(OldListFormat *old, ListFormat *format)
{
	int i;

	format->files_unsel[0] = old->files_unsel[0];
	format->files_unsel[1] = old->files_unsel[1];
	format->files_sel[0] = old->files_sel[0];
	format->files_sel[1] = old->files_sel[1];
	format->dirs_unsel[0] = old->dirs_unsel[0];
	format->dirs_unsel[1] = old->dirs_unsel[1];
	format->dirs_sel[0] = old->dirs_sel[0];
	format->dirs_sel[1] = old->dirs_sel[1];

	format->sort.sort = old->sort.sort;
	format->sort.sort_flags = old->sort.sort_flags;
	format->sort.separation = old->sort.separation;

	for (i = 0; i < 16; i++)
		format->display_pos[i] = old->display_pos[i];
	for (i = 0; i < 15; i++)
		format->display_len[i] = old->display_len[i];

	format->flags = old->flags;
	format->show_free = old->show_free;
	for (i = 0; i < 40; i++)
	{
		format->show_pattern[i] = old->show_pattern[i];
		format->hide_pattern[i] = old->hide_pattern[i];
	}
	for (i = 0; i < 82; i++)
	{
		format->show_pattern_p[i] = 0;
		format->hide_pattern_p[i] = 0;
	}
	for (i = 0; i < 40; i++)
	{
		format->show_pattern_p[i] = old->show_pattern_p[i];
		format->hide_pattern_p[i] = old->hide_pattern_p[i];
	}
}

int convert_open_lister(struct _IFFHandle *iff, CFG_LSTR *lister)
{
	OLD_CFG_LSTR *oldlister = NULL;

	oldlister = AllocVec(sizeof(OLD_CFG_LSTR), MEMF_CLEAR);
	if (!oldlister)
		return 0;

	L_IFFReadChunkBytes(iff, oldlister, sizeof(OLD_CFG_LSTR));
	lister->pos[0] = oldlister->pos[0];
	lister->pos[1] = oldlister->pos[1];
	lister->icon_x = oldlister->icon_x;
	lister->icon_y = oldlister->icon_y;
	convert_list_format(&oldlister->format, &lister->format);
	lister->flags = oldlister->flags;
	CopyMem(oldlister->pad, lister->pad, sizeof(lister->pad));

	if (lister->format.show_pattern[0] != '\0')
		ParsePatternNoCase(lister->format.show_pattern, (UBYTE *)lister->format.show_pattern_p, 80);
	else
		lister->format.show_pattern_p[0] = '\0';

	if (lister->format.hide_pattern[0] != '\0')
		ParsePatternNoCase(lister->format.hide_pattern, (UBYTE *)lister->format.hide_pattern_p, 80);
	else
		lister->format.hide_pattern_p[0] = '\0';

	FreeVec(oldlister);
	return 1;
}

int convert_button_window(struct _IFFHandle *iff, CFG_BTNW *butwin)
{
	OLD_CFG_BTNW *oldbutwin = NULL;
	int i = 0;

	oldbutwin = AllocVec(sizeof(OLD_CFG_BTNW), MEMF_CLEAR);
	if (!oldbutwin)
		return 0;

	L_IFFReadChunkBytes(iff, oldbutwin, sizeof(OLD_CFG_BTNW));

	for (i = 0; i < 32; i++)
		butwin->name[i] = oldbutwin->name[i];

	butwin->font_size = oldbutwin->font_size;
	butwin->pos.Left = oldbutwin->pos.Left;
	butwin->pos.Top = oldbutwin->pos.Top;
	butwin->pos.Width = oldbutwin->pos.Width;
	butwin->pos.Height = oldbutwin->pos.Height;

	for (i = 0; i < 31; i++)
		butwin->font_name[i] = oldbutwin->font_name[i];

	butwin->columns = oldbutwin->columns;
	butwin->rows = oldbutwin->rows;
	butwin->flags = oldbutwin->flags;

	FreeVec(oldbutwin);
	return 1;
}

void do_backup(char *name)
{
	BPTR in = 0;
	BPTR out = 0;
	BPTR lock = 0;
	char *extension = ".bak";
	char *legacy_extension = ".bac_5_90_cfg";
	char outname[256] = {'\0'};
	char legacy_outname[256] = {'\0'};
	char buffer[4096] = {'\0'};
	int buflen = 4094;
	int insize = 0, outsize = 0;

	if (!name)
		return;
	// 238 leaves room for the longer legacy_extension (13 chars) plus
	// nul terminator inside both 256-byte buffers; the new .bak fits
	// comfortably in the same budget.
	if (strlen(name) > 238)
		return;
	strcpy(outname, name);
	strcat(outname, extension);

	// One-shot migration: pre-5.101 builds wrote backups with the
	// .bac_5_90_cfg extension.  If such a file is still sitting next
	// to `name` from an earlier install, fold it into the new .bak
	// scheme so the user does not end up with two stale snapshots.
	// If a current .bak already exists, just delete the legacy file;
	// otherwise rename it so the original one-shot snapshot is kept.
	strcpy(legacy_outname, name);
	strcat(legacy_outname, legacy_extension);
	if ((lock = Lock(legacy_outname, SHARED_LOCK)))
	{
		UnLock(lock);
		if ((lock = Lock(outname, SHARED_LOCK)))
		{
			UnLock(lock);
			DeleteFile(legacy_outname);
		}
		else
		{
			Rename(legacy_outname, outname);
		}
	}

	if ((lock = Lock(outname, SHARED_LOCK)))
	{
		UnLock(lock);
		return;
	}

	if (!(in = Open(name, MODE_OLDFILE)))
		return;
	if (!(out = Open(outname, MODE_NEWFILE)))
	{
		Close(in);
		return;
	}

	do
	{
		insize = Read(in, buffer, buflen);
		if (insize < 1)
			break;
		outsize = Write(out, buffer, insize);
	} while (outsize == insize);

	Close(in);
	Close(out);
	if ((insize < 0) || (outsize < insize))
		DeleteFile(outname);

	return;
}
