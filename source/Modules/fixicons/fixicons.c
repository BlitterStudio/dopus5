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

#include "fixicons.h"

char *version = "$VER: fixicons.module 68.0 (3.10.98)";

/************************************************************************
 *
 * Module designed to fix icon problems. Adds FixIcons command with the following template:
 *
 *	FILE/M/A,ALLOWNOBORDERS=ANB/S,ALLOWNOLABELS=ANL/S,NOFIXOFFSET=NFO/S,SYNCWBTOOPUS=SWO/S,SYNCOPUSTOWB=SOW/S
 *
 *	Function will act on all selected files and directories recursively, making the
 *	following changes to icons:
 *
 *	- Clearing the ICONF_BORDER_ON and ICONF_BORDER_OFF flags
 *	  (unless ALLOWNOBORDERS switch is set)
 *
 *	- Clearing the ICONF_NO_LABEL flag
 *	  (unless ALLOWNOLABELS switch is set)
 *
 *	- Resetting the dd_CurrentX and dd_CurrentY values of drawer/disk icons to
 *	  0,0, AND offsetting all icons within that drawer by the values of these
 *	  two fields (eg, if dd_CurrentX==5 and dd_CurrentY==3, all icons within
 *	  the drawer would be shifted -5,-3, and the two dd_Current fields would
 *	  be set to 0)
 *	  (unless NOFIXOFFSET switch is set)
 *
 *	- If the SYNCWBTOOPUS switch is set, the Workbench position field within
 *	  the icon will be set to the same coordinates as the Opus position field
 *
 *	- If the SYNCOPUSTOWB switch is set, the Opus position field within the icon
 *	  will be set to the same coordinates as the Workbench position field.
 *	  Additionally, the ICONF_POSITION_OK flag will be cleared.
 *
 *	If an icon file itself is selected, function will operate on that file - if another
 *	file or directory is selected, function will try to obtain an icon for that file
 *	or directory. Additionally, if a directory is selected function will enter
 *	directory recursively and perform actions on contents (use MatchFirst()/MatchNext())
 *
 *	Main module code is already written and working, all that needs to be done is the
 *	code to actually scan and fix icons.
 *
 *	See the functions GetIconFlags(), SetIconFlags(), GetIconPosition() and
 *	SetIconPosition() in icon.doc, and the ICONF_xxx defines in dopusbase.h
 *
 *	--
 *
 *	An addition to the FixIcons spec that I forgot about - it needs to do the
 *	dd_Current shifting in the root directory of a disk as well as for
 *	sub-directories.  Basically, if the NFO flag is not set and the Disk.info
 *	file is one of the selected files, FixIcons should read the disk.info file
 *	first and use the offsets within to adjust the positions of other selected
 *	icons in the root directory.
 *
 *	Also thinking about it, I think the NOFIXOFFSETS flag should be changed to
 *	a positive (FIXOFFSETS) and be off by default.
 *
 *
 *	--
 *	15.4.98 Modified to fix multiple setting of file and file.info.
 *		Now only updates .info files.
 *
 *	15.5.98	Added new switch REPORT to output a report on the results.
 *
 *	26.5.98 Fixed SYNCOPUSTOWB option to save icons only if they have changed.
 *
 ****************************************************************************/

// Local variables kept in this structure to conserve recursive stack space
struct locals
{
	struct FileInfoBlock fib __attribute__((aligned(4)));

	struct DiskObject *obj;
	ULONG iconflags;
	short opus_x, opus_y;
	int put, set;
	int isicon;
	BPTR lock;
	char newpath[256];
	int newxoff, newyoff;
	ULONG flags;
};

// Routine copied from Jon's code
BPTR open_temp_file(char *filename, IPCData *ipc)
{
	unsigned short a;
	ULONG micros, secs;
	IPTR temp_key;
	BPTR file = 0;

	// Get temporary key
	temp_key = (IPTR)ipc;
	CurrentTime(&secs, &micros);
	if (micros)
		temp_key *= micros;
	temp_key += secs;

	// Create a temporary script file; try up to 99 times
	for (a = 0; a < 99; a++)
	{
		// Build script name
		lsprintf(filename, "T:dopus-%lx-tmp", temp_key + a);

		// Try to open file
		if ((file = Open(filename, MODE_NEWFILE)))
			break;
	}

	return file;
}

// Print the result of a fix to the output file
void print_result(BPTR outfile, char *path, BOOL result, ULONG flags)
{
	int i, f = 0;
	char buf[512] = "";

	strcpy(buf, GetString(locale, result ? MSG_FIXED : MSG_COULDNT_FIX));
	lsprintf(buf + strlen(buf), " %s.info", (IPTR)path);

	if (flags)
	{
		strcat(buf, "\t(");

		for (i = 1; i <= 5; ++i)
		{
			if (flags & (1 << i))
			{
				lsprintf(buf + strlen(buf), "%s%s", (IPTR)(f ? ", " : ""), (IPTR)GetString(locale, MSG_ANB + i - 1));
				f = 1;
			}
		}

		strcat(buf, ")");
	}

	strcat(buf, "\n");

	FPuts(outfile, buf);
}

// The main recursive routine
static int fix_icon(APTR progress, char *path, long *args, int xoff, int yoff, BPTR outfile, int *changes)
{
	struct locals *l;

	// Check abort
	if (CheckProgressAbort(progress))
		return 0;

	if (!(l = AllocVec(sizeof(struct locals), MEMF_CLEAR)))
		return 0;

	// Trim .info if needed
	if (strlen(path) >= 5 && !stricmp(path + strlen(path) - 5, ".info"))
	{
		path[strlen(path) - 5] = 0;
		l->isicon = 1;
	}

	if ((l->obj = GetDiskObject(path)))
	{
		// Get flags
		l->iconflags = GetIconFlags(l->obj);

		// Get position
		GetIconPosition(l->obj, &l->opus_x, &l->opus_y);

		// Reset border flags?
		if (!args || (args && !args[FI_ANB]))
		{
			if (l->iconflags & (ICONF_BORDER_ON | ICONF_BORDER_OFF))
			{
				l->iconflags &= ~(ICONF_BORDER_ON | ICONF_BORDER_OFF);
				l->set = 1;
			}
		}

		// Reset label flag?
		if (!args || (args && !args[FI_ANL]))
		{
			if (l->iconflags & ICONF_NO_LABEL)
			{
				l->iconflags &= ~ICONF_NO_LABEL;
				l->set = 1;
			}
		}

		if ((!args || (args && !args[FI_NFO])))
		{
			if (l->obj->do_DrawerData && (l->obj->do_Type == WBDISK || l->obj->do_Type == WBDRAWER ||
										  l->obj->do_Type == WBGARBAGE || l->obj->do_Type == WBDEVICE))
			{
				// Need to adjust x?
				if (xoff && l->obj->do_CurrentX && l->obj->do_CurrentX != NO_ICON_POSITION)
				{
					l->obj->do_CurrentX -= xoff;
					l->put = 1;
					l->flags |= (1 << FI_NFO);
				}

				// Need to adjust y?
				if (yoff && l->obj->do_CurrentY && l->obj->do_CurrentY != NO_ICON_POSITION)
				{
					l->obj->do_CurrentY -= yoff;
					l->put = 1;
					l->flags |= (1 << FI_NFO);
				}

				// Remember offsets for next recursion level
				l->newxoff = l->obj->do_DrawerData->dd_CurrentX;
				l->newyoff = l->obj->do_DrawerData->dd_CurrentY;

				// Need to reset offsets?
				if (l->obj->do_DrawerData->dd_CurrentX || l->obj->do_DrawerData->dd_CurrentY)
				{
					l->obj->do_DrawerData->dd_CurrentX = 0;
					l->obj->do_DrawerData->dd_CurrentY = 0;

					l->put = 1;
					l->flags |= (1 << FI_NFO);
				}
			}
		}

		// Sync WB to Opus?  (Only if there is an Opus pos)
		if (args && args[FI_SWO] && (l->iconflags & ICONF_POSITION_OK))
		{
			// Need to change?
			if (l->obj->do_CurrentX != l->opus_x || l->obj->do_CurrentY != l->opus_y)
			{
				l->obj->do_CurrentX = l->opus_x;
				l->obj->do_CurrentY = l->opus_y;
				l->put = 1;
				l->flags |= (1 << FI_SWO);
			}
		}

		// Sync Opus to WB?  (Syncs position and resets pos flag)
		else if (args && args[FI_SOW])
		{
			// Need to change position?
			if (l->obj->do_CurrentX != l->opus_x || l->obj->do_CurrentY != l->opus_y)
			{
				SetIconPosition(l->obj, l->obj->do_CurrentX, l->obj->do_CurrentY);
				l->put = 1;
				l->flags |= (1 << FI_SOW);
			}

			// Need to reset flag?
			if (l->iconflags & ICONF_POSITION_OK)
			{
				l->iconflags &= ~ICONF_POSITION_OK;
				l->set = 1;
				l->flags |= (1 << FI_SOW);
			}
		}

		// Need to set flags?
		if (l->set)
		{
			SetIconFlags(l->obj, l->iconflags);
			l->put = 1;
		}

		// only write if it was an icon to start with
		// skip if just a plain file

		if (l->isicon && l->put)
		{
			BOOL r;

			if ((r = PutDiskObject(path, l->obj)))
				++*changes;

			if (outfile)
				print_result(outfile, path, r, l->flags);
		}

		FreeDiskObject(l->obj);
	}

	// if not an icon then lock it and check if it is a directory

	if (!l->isicon && (l->lock = Lock(path, ACCESS_READ)))
	{
		// is a directory?

		if (Examine(l->lock, &l->fib) && l->fib.fib_DirEntryType >= 0)
		{
			// yes is a directory
			if (ExNext(l->lock, &l->fib))
			{
				int more_files;

				strcpy(l->newpath, path);
				AddPart(l->newpath, l->fib.fib_FileName, 256);

				more_files = ExNext(l->lock, &l->fib);

				// Update progress window
				SetProgressWindowTags(progress, PW_FileName, (IPTR)FilePart(path), TAG_END);

				fix_icon(progress, l->newpath, args, l->newxoff, l->newyoff, outfile, changes);

				if (more_files)
				{
					do
					{
						// Check abort
						if (CheckProgressAbort(progress))
							break;

						strcpy(l->newpath, path);
						AddPart(l->newpath, l->fib.fib_FileName, 256);

						if (more_files)
							more_files = ExNext(l->lock, &l->fib);

						// Update progress window
						SetProgressWindowTags(progress, PW_FileName, (IPTR)FilePart(path), TAG_END);

						fix_icon(progress, l->newpath, args, l->newxoff, l->newyoff, outfile, changes);
					} while (more_files);
				}
			}
		}
		UnLock(l->lock);
	}

	FreeVec(l);

	return 0;
}

// Fix icons
int LIBFUNC L_Module_Entry(REG(a0, char *argstring),
						   REG(a1, struct Screen *screen),
						   REG(a2, IPCData *ipc),
						   REG(a3, IPCData *main_ipc),
						   REG(d0, IPTR mod_id),
						   REG(d1, EXT_FUNC(func_callback)))
{
	Att_List *files;
	Att_Node *node;
	char source[256], path[300];
	APTR progress;
	short count = 1;
	FuncArgs *args;
	struct DiskObject *obj;
	int xoff = 0;
	int yoff = 0;
	BPTR outfile = 0;
	char temp_filename[256];
	int changes = 0;

	// Create list
	if (!(files = Att_NewList(LISTF_POOL)))
		return 0;

	// Get source path
	func_callback(EXTCMD_GET_SOURCE, IPCDATA(ipc), source);

	// Parse arguments
	args = ParseArgs(arg_template, argstring);

	// Files supplied?
	if (args && args->FA_Arguments[0])
	{
		char **names;
		short a;

		// Get array pointer
		names = (char **)args->FA_Arguments[0];

		// Go through names
		for (a = 0; names[a]; a++)
		{
			// Add name to list
			Att_NewNode(files, names[a], 0, 0);
		}
	}

	// Otherwise
	else
	{
		FunctionEntry *entry;

		// Get entries
		while ((entry = (FunctionEntry *)func_callback(EXTCMD_GET_ENTRY, IPCDATA(ipc), 0)))
		{
			struct endentry_packet packet;

			// Add name to list
			Att_NewNode(files, entry->name, 0, 0);

			// Fill out packet to end entry
			packet.entry = entry;
			packet.deselect = 1;

			// End entry
			func_callback(EXTCMD_END_ENTRY, IPCDATA(ipc), &packet);
		}
	}

	// List empty?
	if (IsListEmpty((struct List *)files))
	{
		// Free and return
		Att_RemList(files, 0);
		DisposeArgs(args);
		return 1;
	}

	// Need output file?
	if (args && args->FA_Arguments[FI_REPORT])
	{
		if ((outfile = open_temp_file(temp_filename, ipc)))
		{
			FPuts(outfile, GetString(locale, MSG_RESULTS));
			FPuts(outfile, ":\n\n");
		}
	}

	// Need to process Disk.info first?
	if (!args || !args->FA_Arguments[FI_NFO])
	{
		for (node = (Att_Node *)files->list.lh_Head; node->node.ln_Succ; node = (Att_Node *)node->node.ln_Succ)
		{
			if (!stricmp("Disk.info", FilePart(node->node.ln_Name)))
			{
				// Not a full path?
				if (!strchr(node->node.ln_Name, ':') && !strchr(node->node.ln_Name, '/'))
				{
					// Build full path
					strcpy(path, source);
					AddPart(path, node->node.ln_Name, 300);
				}

				// Full path supplied
				else
					strcpy(path, node->node.ln_Name);

				// Strip .info
				path[strlen(path) - 5] = 0;

				if ((obj = GetDiskObject(path)))
				{
					if (obj->do_DrawerData && obj->do_Type == WBDISK)
					{
						xoff = obj->do_DrawerData->dd_CurrentX;
						yoff = obj->do_DrawerData->dd_CurrentY;

						if (xoff || yoff)
						{
							BOOL result;

							obj->do_DrawerData->dd_CurrentX = 0;
							obj->do_DrawerData->dd_CurrentY = 0;

							if ((result = PutDiskObject(path, obj)))
								++changes;

							if (outfile)
								print_result(outfile, path, result, 1 << FI_NFO);
						}
					}

					FreeDiskObject(obj);
				}

				Att_RemNode(node);
				break;
			}
		}
	}

	// Open progress window
	progress = OpenProgressWindowTags(PW_Screen,
									  (IPTR)screen,
									  PW_Title,
									  (IPTR)GetString(locale, MSG_FIXING_ICONS),
									  PW_Flags,
									  PWF_FILENAME | PWF_GRAPH | PWF_ABORT,
									  PW_FileCount,
									  Att_NodeCount(files),
									  TAG_END);

	// Go through files list
	for (node = (Att_Node *)files->list.lh_Head; node->node.ln_Succ; node = (Att_Node *)node->node.ln_Succ, count++)
	{
		// Check abort
		if (CheckProgressAbort(progress))
			break;

		// Not a full path?
		if (!strchr(node->node.ln_Name, ':') && !strchr(node->node.ln_Name, '/'))
		{
			// Build full path
			strcpy(path, source);
			AddPart(path, node->node.ln_Name, 300);
		}

		// Full path supplied
		else
			strcpy(path, node->node.ln_Name);

		// Update progress window
		SetProgressWindowTags(progress, PW_FileName, (IPTR)FilePart(path), PW_FileNum, count, TAG_END);

		/*********************** MAIN CODE BODY GOES HERE ****************************/

		fix_icon(progress, path, args ? args->FA_Arguments : NULL, xoff, yoff, outfile, &changes);

		/*********************** MAIN CODE BODY GOES HERE ****************************/
	}

	// Free stuff
	CloseProgressWindow(progress);

	// Need to show results?
	if (outfile)
	{
		struct command_packet packet = {0};
		char buf[256];

		// No files changed?
		if (changes)
			lsprintf(buf, "\n%ld %s\n", changes, (IPTR)GetString(locale, MSG_CHANGES_MADE));
		else
			lsprintf(buf, "%s %s\n", (IPTR)GetString(locale, MSG_NO), (IPTR)GetString(locale, MSG_CHANGES_MADE));

		FPuts(outfile, (STRPTR)buf);

		Close(outfile);

		lsprintf(buf, "dopus read delete %s", (IPTR)temp_filename);

		packet.command = buf;

		func_callback(EXTCMD_SEND_COMMAND, IPCDATA(ipc), &packet);
	}

	Att_RemList(files, 0);
	DisposeArgs(args);
	return 1;
}
