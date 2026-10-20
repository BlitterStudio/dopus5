#include "config_lib.h"

// Build a sub-option list
Att_List *build_sub_options(SubOptionHandle *list)
{
	Att_List *options;

	if ((options = Att_NewList(0)))
	{
		int a;

		// Add names to list
		for (a = 0; list[a].num > -1; a++)
			Att_NewNode(options, GetString(locale, list[a].name), a, ADDNODE_SORT);
	}
	return options;
}

// Get line from coordinates
short functioned_get_line(struct Window *window,
						  GL_Object *object,
						  unsigned short x,
						  unsigned short y,
						  struct Library *IntuitionBase)
{
	IPTR storage;

	// Get window-relative coordinates
	x -= window->LeftEdge;
	y -= window->TopEdge;

	// Send coordinates
	storage = (x << 16) | y;

	// Get selection
	GetAttr(DLV_GetLine, GADGET(object), &storage);

	return (short)storage;
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

// Check if a path is valid for saving
BOOL config_valid_path(char *path)
{
	char *ptr;

	// Nothing at all?
	if (!path || !*path)
		return 0;

	// Get file part
	if (!(ptr = FilePart(path)))
		return 0;

	// Untitled?
	if (strcmp(ptr, GetString(locale, MSG_UNTITLED)) == 0)
		return 0;

	// Ok
	return 1;
}

// File requester
BOOL config_filereq(struct Window *window, ULONG title, char *path, char *defpath, short flags)
{
	char *ptr = 0;
	char file[40];
	BOOL ok;

	// Invalid path?
	if (!*path)
		stccpy(path, defpath, 256);

	// Get filename
	else if ((ptr = FilePart(path)))
	{
		stccpy(file, ptr, sizeof(file));
		*ptr = 0;
		ptr = file;
	}

	// Display file requester
	if ((ok = AslRequestTags(DATA(window)->request,
							 ASLFR_Window,
							 window,
							 ASLFR_TitleText,
							 GetString(locale, title),
							 ASLFR_InitialFile,
							 (ptr) ? ptr : "",
							 ASLFR_InitialDrawer,
							 path,
							 ASLFR_Flags1,
							 FRF_PRIVATEIDCMP | ((flags & 1) ? FRF_DOSAVEMODE : 0),
							 TAG_END)))
	{
		// Get new path
		stccpy(path, DATA(window)->request->fr_Drawer, 256);
		AddPart(path, DATA(window)->request->fr_File, 256);
	}

	return ok;
}

ULONG LIBFUNC L_Config_ListerButtons(REG(a0, char *bank_name),
									 REG(a1, IPCData *ipc),
									 REG(a2, IPCData *owner_ipc),
									 REG(a3, struct Screen *screen),
									 REG(d0, Cfg_ButtonBank *def_bank),
									 REG(d1, IPTR command_list),
									 REG(d2, LONG initial))
{
	return 0;
}

// Create instruction from dropped file
Cfg_Instruction *instruction_from_wbarg(struct WBArg *arg, APTR memory)
{
	char buf[256], buffer[300];
	short type;
	BOOL quotes = 0;

	// Check argument
	if ((type = funced_appmsg_arg(arg, buf, (struct Library *)DOSBase)) == -1)
		return 0;

	// Clear buffer
	*buffer = 0;

	// Directory?
	if (type == INST_DIR)
	{
		// Scandir command
		strcpy(buffer, "ScanDir NEW ");
		type = INST_COMMAND;
	}

	// Project?
	else if (type == INST_PROJECT)
	{
		struct DiskObject *icon;

		// Try to get icon
		if ((icon = GetCachedDiskObjectNew(buf, 1)))
		{
			// Got default tool?
			if (icon->do_DefaultTool)
			{
				stccpy(buffer, icon->do_DefaultTool, sizeof(buffer) - 1);
				strcat(buffer, " ");
			}

			// Free icon
			FreeCachedDiskObject(icon);

			// Change type
			type = INST_WORKBENCH;
		}

		// If no default tool, use SmartRead
		if (!*buffer)
		{
			// SmartRead it
			strcpy(buffer, "SmartRead ");

			// Change type
			type = INST_COMMAND;
		}
	}

	// Any spaces in name?
	if (strchr(buf, ' '))
	{
		// Add quote
		strcat(buffer, "\"");
		quotes = 1;
	}

	// Add filename to buffer
	strcat(buffer, buf);

	// Terminating quote
	if (quotes)
		strcat(buffer, "\"");

	// Add instruction
	return NewInstruction(memory, type, buffer);
}

void parse_number(char **ptr, unsigned short *val)
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

void store_window_pos(struct Window *window, struct IBox *pos)
{
	pos->Left = window->LeftEdge;
	pos->Top = window->TopEdge;
	pos->Width = window->Width - window->BorderLeft - window->BorderRight;
	pos->Height = window->Height - window->BorderTop - window->BorderBottom;
}
