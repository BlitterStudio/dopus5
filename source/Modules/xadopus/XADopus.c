/*
  XADOpus.module 1.22 - DOpus Magellan plugin to browse through XAD archives
  Copyright (C) 1999,2000 Mladen Milinkovic <mladen.milinkovic@ri.tel.hr>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "XADopus.h"

char *viewcmds = "Read Play Show HexRead AnsiRead IconInfo";

LIBFUNC ULONG ASM SAVEDS L_ProgressHook(REG(a0, struct Hook *), REG(a2, int skip), REG(a1, struct xadProgressInfo *));

static IPTR xad_parse_handle(const char *text)
{
	UQUAD value = 0;

	if (!text)
		return 0;

	while (*text >= '0' && *text <= '9')
	{
		value = (value * 10) + (*text - '0');
		++text;
	}

	return (IPTR)value;
}

static void xad_format_handle(char *buf, IPTR handle)
{
	char tmp[24];
	int pos = sizeof(tmp) - 1;
	UQUAD value = (UQUAD)handle;

	tmp[pos] = 0;
	do
	{
		tmp[--pos] = '0' + (value % 10);
		value /= 10;
	} while (value && pos > 0);

	strcpy(buf, tmp + pos);
}

/// Error Requester
void ErrorReq(struct xoData *data, char *Mess)
{
	AsyncRequestTags(data->ipc,
					 REQTYPE_SIMPLE,
					 NULL,
					 NULL,
					 NULL,
					 AR_Screen,
					 (IPTR)data->screen,
					 AR_Message,
					 (IPTR)Mess,
					 AR_Title,
					 (IPTR)DOpusGetString(locale, MSG_ERROR),
					 AR_Button,
					 (IPTR)DOpusGetString(locale, MSG_OK),
					 (data->listw) ? AR_Window : TAG_END,
					 (IPTR)data->listw,
					 TAG_END);
}
///

BOOL PasswordReq(struct xoData *data)
{
	int retval;

	D(bug("password requester for %s\n", data->arcname));
	data->password[0] = 0;	// clear the password string
#warning Put 'Enter password' into the catalog!
	retval = AsyncRequestTags(data->ipc,
							  REQTYPE_SIMPLE,
							  NULL,
							  NULL,
							  NULL,
							  AR_Screen,
							  (IPTR)data->screen,
							  (data->listw) ? AR_Window : TAG_END,
							  (IPTR)data->listw,
							  AR_Message,
							  (IPTR) "Enter password",
							  AR_Buffer,
							  (IPTR)data->password,
							  AR_BufLen,
							  sizeof(data->password),
							  AR_Button,
							  (IPTR)DOpusGetString(locale, MSG_OK),
							  AR_Button,
							  (IPTR)DOpusGetString(locale, MSG_ABORT),
							  AR_Flags,
							  SRF_SECURE,
							  TAG_END);

	if (!retval)
		data->password[0] = 0;

	D(bug("retval %d password '%s'\n", retval, data->password));

	return (retval != 0);
}

/// BuildTree
void ChangeDir(struct xoData *data, struct Tree *cur)
{
	struct Tree *tmp;
	struct List list;
	UWORD dc = 0, fc = 0;
	DOpusCallbackInfo *infoptr = &data->hook;

	DC_CALL2(infoptr, dc_LockFileList, DC_REGA0, data->listh, DC_REGD0, TRUE);
	// data->hook.dc_LockFileList(data->listh,TRUE);

	if (data->cur)
	{
		tmp = data->cur->Child;
		while (tmp)
		{
			DC_CALL2(infoptr, dc_RemoveFileEntry, DC_REGA0, data->listh, DC_REGA1, tmp->entry);
			// data->hook.dc_RemoveFileEntry(data->listh,tmp->entry);
			tmp = tmp->Next;
		}
	}

	data->cur = cur;
	cur = cur->Child;

	NewList(&list);

	while (cur)
	{
		cur->entry = (APTR)DC_CALL3(
			infoptr, dc_CreateFileEntry, DC_REGA0, data->listh, DC_REGA1, &cur->fib, DC_REGD0, (IPTR)NULL);
		// cur->entry=data->hook.dc_CreateFileEntry(data->listh, &cur->fib, NULL);
		AddTail(&list, cur->entry);

		if (cur->fib.fib_DirEntryType < 0)
			fc++;
		else
			dc++;

		cur = cur->Next;
	}

	DC_CALL4(infoptr, dc_SortFileList, DC_REGA0, data->listh, DC_REGA1, &list, DC_REGD0, fc, DC_REGD1, dc);
	DC_CALL1(infoptr, dc_UnlockFileList, DC_REGA0, data->listh);
	DC_CALL2(infoptr, dc_RefreshLister, DC_REGA0, data->listh, DC_REGD0, HOOKREFRESH_DATE | HOOKREFRESH_FULL);
	/*data->hook.dc_SortFileList(data->listh, &list, fc, dc);
	data->hook.dc_UnlockFileList(data->listh);
	data->hook.dc_RefreshLister(data->listh, HOOKREFRESH_DATE|HOOKREFRESH_FULL);*/
}

struct Tree *FindDrw(struct xoData *data, UBYTE *path)
{
	struct Tree *cur = data->root;
	UBYTE x, buf[50];

	while (1)
	{
		for (x = 0; (buf[x] = *path) && (*path++ != '/'); x++)
			;
		if (!buf[x])
			return (cur);
		buf[x] = 0;

		if (!cur->Child)
		{
			cur->Child = AllocMemH(data->rhand, sizeof(struct Tree));
			cur = cur->Child;
			strcpy(cur->fib.fib_FileName, (STRPTR)buf);
			cur->fib.fib_DirEntryType = 1;
			*cur->fib.fib_Comment = cur->fib.fib_Size = cur->fib.fib_Protection = 0;
			DateStamp(&cur->fib.fib_Date);
		}
		else
		{
			cur = cur->Child;
			while (strcmp(cur->fib.fib_FileName, (STRPTR)buf))
			{
				if (cur->Next)
					cur = cur->Next;
				else
				{
					cur->Next = AllocMemH(data->rhand, sizeof(struct Tree));
					cur = cur->Next;
					strcpy(cur->fib.fib_FileName, (STRPTR)buf);
					cur->fib.fib_DirEntryType = 1;
					*cur->fib.fib_Comment = cur->fib.fib_Size = cur->fib.fib_Protection = 0;
					DateStamp(&cur->fib.fib_Date);
				}
			}
		}
	}
}

static BOOL XADFileInfoIsDirectory(struct xadFileInfo *xfi)
{
	ULONG namelen;

	if (!xfi || !xfi->xfi_FileName)
		return FALSE;

	if (xfi->xfi_Flags & XADFIF_DIRECTORY)
		return TRUE;

	namelen = strlen(xfi->xfi_FileName);
	return (BOOL)(namelen > 0 && xfi->xfi_FileName[namelen - 1] == '/');
}

static BOOL XADFileInfoSkipListerEntry(struct xadFileInfo *xfi)
{
	return (BOOL)(!xfi || (xfi->xfi_Flags & (XADFIF_INFOTEXT | XADFIF_NOFILENAME)) ||
				  !xfi->xfi_FileName || !xfi->xfi_FileName[0]);
}

static void UpdateTreeFromXADFileInfo(struct Tree *tree,
									  struct xadFileInfo *xfi,
									  char *name,
									  BOOL directory)
{
	if (name)
		strcpy(tree->fib.fib_FileName, name);
	if (xfi->xfi_Comment)
		strcpy(tree->fib.fib_Comment, xfi->xfi_Comment);
	else
		*tree->fib.fib_Comment = 0;
	xadConvertDates(
		XAD_DATEXADDATE, (IPTR)&xfi->xfi_Date, XAD_GETDATEDATESTAMP, (IPTR)&tree->fib.fib_Date, TAG_DONE);
	tree->fib.fib_Protection = xfi->xfi_Protection;
	tree->fib.fib_DirEntryType = (directory) ? 1 : -1;
	tree->fib.fib_Size = xfi->xfi_Size;
	tree->fib.fib_OwnerUID = xfi->xfi_OwnerUID;
	tree->fib.fib_OwnerGID = xfi->xfi_OwnerGID;
	tree->fib.fib_NumBlocks = tree->fib.fib_DiskKey = 0;
#ifndef __amigaos4__
	tree->fib.fib_EntryType = 0;
#endif
	tree->xfi = xfi;
}

void BuildTree(struct xoData *data)
{
	struct xadFileInfo *xfi;
	struct Tree *tree, *tmp, *dup_tree;
	char *name;
	BOOL dup, directory;

	xfi = data->ArcInf->xai_FileInfo;

	while (xfi)
	{
		if (XADFileInfoSkipListerEntry(xfi))
		{
			xfi = xfi->xfi_Next;
			continue;
		}

		directory = XADFileInfoIsDirectory(xfi);
		tree = FindDrw(data, (UBYTE *)xfi->xfi_FileName);
		name = FilePart(xfi->xfi_FileName);

		/* FindDrw() already materialised a trailing-slash directory path.
		   Do not add FilePart("dir/") as an empty child entry. */
		if (directory && !*name)
		{
			UpdateTreeFromXADFileInfo(tree, xfi, NULL, TRUE);
			xfi = xfi->xfi_Next;
			continue;
		}

		dup = FALSE;
		dup_tree = NULL;

		if (tree->Child)
		{
			tmp = tree->Child;
			while (tmp)
			{
				if (!strcmp(name, tmp->fib.fib_FileName))
				{
					dup = TRUE;
					dup_tree = tmp;
					break;
				}
				tmp = tmp->Next;
			}
		}

		if (!dup)
		{
			if (tree->Child == NULL)
			{
				tree->Child = AllocMemH(data->rhand, sizeof(struct Tree));
				tree = tree->Child;
			}
			else
			{
				tree = tree->Child;
				while (tree->Next != NULL)
					tree = tree->Next;
				tree->Next = AllocMemH(data->rhand, sizeof(struct Tree));
				tree = tree->Next;
			}

			UpdateTreeFromXADFileInfo(tree, xfi, name, directory);
		}
		else if (directory && dup_tree && dup_tree->fib.fib_DirEntryType > 0)
		{
			UpdateTreeFromXADFileInfo(dup_tree, xfi, NULL, TRUE);
		}

		xfi = xfi->xfi_Next;
	}
}
///

/// MsgPort
BOOL AllocPort(struct xoData *data)
{
	// UWORD c=0;
	DOpusCallbackInfo *infoptr = &data->hook;

	if ((data->mp = CreateMsgPort()))
	{
		data->mp->mp_Node.ln_Pri = 2;
		data->mp->mp_Node.ln_Name = data->mp_name;

#if 0
		Forbid();
		do {
			sprintf(data->mp_name,"XADopus_%d",c);
			c++;
		} while(FindPort(data->mp_name));

		AddPort(data->mp);
		Permit();
#else
		sprintf(data->mp_name, "XADopus_%p", data->mp);	 // this is guaranteed to be unique
		AddPort(data->mp);
#endif

		sprintf(data->buf, "lister set %s handler %s quotes", data->lists, data->mp_name);
		DC_CALL4(
			infoptr, dc_SendCommand, DC_REGA0, IPCDATA(data->ipc), DC_REGA1, data->buf, DC_REGA2, NULL, DC_REGD0, 0);
		// data->hook.dc_SendCommand(IPCDATA(data->ipc),data->buf,NULL,NULL);
		sprintf(data->buf, "dopus addtrap * %s", data->mp_name);
		DC_CALL4(
			infoptr, dc_SendCommand, DC_REGA0, IPCDATA(data->ipc), DC_REGA1, data->buf, DC_REGA2, NULL, DC_REGD0, 0);
		// data->hook.dc_SendCommand(IPCDATA(data->ipc),data->buf,NULL,NULL);
		sprintf(data->buf, "dopus remtrap All %s", data->mp_name);
		DC_CALL4(
			infoptr, dc_SendCommand, DC_REGA0, IPCDATA(data->ipc), DC_REGA1, data->buf, DC_REGA2, NULL, DC_REGD0, 0);
		// data->hook.dc_SendCommand(IPCDATA(data->ipc),data->buf,NULL,NULL);
		sprintf(data->buf, "dopus remtrap None %s", data->mp_name);
		DC_CALL4(
			infoptr, dc_SendCommand, DC_REGA0, IPCDATA(data->ipc), DC_REGA1, data->buf, DC_REGA2, NULL, DC_REGD0, 0);
		// data->hook.dc_SendCommand(IPCDATA(data->ipc),data->buf,NULL,NULL);
		sprintf(data->buf, "dopus remtrap Select %s", data->mp_name);
		DC_CALL4(
			infoptr, dc_SendCommand, DC_REGA0, IPCDATA(data->ipc), DC_REGA1, data->buf, DC_REGA2, NULL, DC_REGD0, 0);
		// data->hook.dc_SendCommand(IPCDATA(data->ipc),data->buf,NULL,NULL);
		sprintf(data->buf, "dopus remtrap Toggle %s", data->mp_name);
		DC_CALL4(
			infoptr, dc_SendCommand, DC_REGA0, IPCDATA(data->ipc), DC_REGA1, data->buf, DC_REGA2, NULL, DC_REGD0, 0);
		// data->hook.dc_SendCommand(IPCDATA(data->ipc),data->buf,NULL,NULL);
		sprintf(data->buf, "dopus remtrap CacheList %s", data->mp_name);
		DC_CALL4(
			infoptr, dc_SendCommand, DC_REGA0, IPCDATA(data->ipc), DC_REGA1, data->buf, DC_REGA2, NULL, DC_REGD0, 0);
		// data->hook.dc_SendCommand(IPCDATA(data->ipc),data->buf,NULL,NULL);
		sprintf(data->buf, "dopus remtrap DeviceList %s", data->mp_name);
		DC_CALL4(
			infoptr, dc_SendCommand, DC_REGA0, IPCDATA(data->ipc), DC_REGA1, data->buf, DC_REGA2, NULL, DC_REGD0, 0);
		// data->hook.dc_SendCommand(IPCDATA(data->ipc),data->buf,NULL,NULL);

		return (TRUE);
	}

	return (FALSE);
}

void FreePort(struct xoData *data)
{
	struct Message *msg;

	RemPort(data->mp);

	while ((msg = GetMsg(data->mp)))
		ReplyMsg(msg);

	DeleteMsgPort(data->mp);
}
///

/// Temp Files
void RemoveTemp(struct xoData *data)
{
	struct TempFile *tf;

	while ((tf = (struct TempFile *)RemHead((struct List *)&data->Temp)))
	{
		DeleteFile(tf->FileName);
		FreeVec(tf);
	}
}
///

/// Events
void LaunchCommand(struct xoData *data, char *cmd, char *name, char *qual)
{
	DOpusCallbackInfo *infoptr = &data->hook;

	/* Dispatch the command asynchronously.
	 *
	 * We used to send "command wait ..." here, which blocked this task until
	 * the launched command completed. For view commands (Read/Play/Show/...)
	 * and DoubleClick that frequently means blocking until the user closes a
	 * viewer, leaving the lister stuck in the "busy" state (see issue #27).
	 *
	 * A later attempt wrapped an async "command ..." call in a Wait()/Delay()
	 * loop on data->mp, but that port only receives trap messages from DOpus -
	 * the command itself never signals completion there - so Wait() blocked
	 * forever (never reaching the decrement path) and any trap events that did
	 * arrive were silently consumed instead of being handled by the main event
	 * loop, leaving the lister permanently busy.
	 *
	 * The temp trap removal and the brief busy toggle are still useful to
	 * avoid re-entering the same handler while DOpus dispatches the command,
	 * so keep those, but drop the broken wait entirely and let DOpus run the
	 * command in the background. */
	sprintf(data->buf, "lister set %s busy on wait", data->lists);
	DC_CALL4(infoptr, dc_SendCommand, DC_REGA0, IPCDATA(data->ipc), DC_REGA1, data->buf, DC_REGA2, NULL, DC_REGD0, 0);

	sprintf(data->buf, "dopus remtrap %s %s", cmd, data->mp_name);
	DC_CALL4(infoptr, dc_SendCommand, DC_REGA0, IPCDATA(data->ipc), DC_REGA1, data->buf, DC_REGA2, NULL, DC_REGD0, 0);

	sprintf(data->buf, "command %s %s %s", cmd, qual, name);
	DC_CALL4(infoptr, dc_SendCommand, DC_REGA0, IPCDATA(data->ipc), DC_REGA1, data->buf, DC_REGA2, NULL, DC_REGD0, 0);

	sprintf(data->buf, "dopus addtrap %s %s", cmd, data->mp_name);
	DC_CALL4(infoptr, dc_SendCommand, DC_REGA0, IPCDATA(data->ipc), DC_REGA1, data->buf, DC_REGA2, NULL, DC_REGD0, 0);

	sprintf(data->buf, "lister set %s busy off wait", data->lists);
	DC_CALL4(infoptr, dc_SendCommand, DC_REGA0, IPCDATA(data->ipc), DC_REGA1, data->buf, DC_REGA2, NULL, DC_REGD0, 0);
}

void _scandir(struct xoData *data, char *listh, char *path)
{
	DOpusCallbackInfo *infoptr = &data->hook;

	sprintf(data->buf, "command source %s ScanDir %s", listh, path);
	DC_CALL4(infoptr, dc_SendCommand, DC_REGA0, IPCDATA(data->ipc), DC_REGA1, data->buf, DC_REGA2, NULL, DC_REGD0, 0);
	// data->hook.dc_SendCommand(IPCDATA(data->ipc),data->buf,NULL,NULL);
}

void _cd(struct xoData *data, struct Tree *cur)
{
	DOpusCallbackInfo *infoptr = &data->hook;

	sprintf(data->buf, "lister set %s path %s", data->lists, data->listpath);
	DC_CALL4(infoptr, dc_SendCommand, DC_REGA0, IPCDATA(data->ipc), DC_REGA1, data->buf, DC_REGA2, NULL, DC_REGD0, 0);
	// data->hook.dc_SendCommand(IPCDATA(data->ipc),data->buf,NULL,NULL);

	ChangeDir(data, cur);
}

void _doubleclick(struct xoData *data, char *name, char *qual)
{
	struct Tree *tmp = data->cur->Child;
	struct TempFile *tf;
	xadERROR err;
	BOOL retry;

	while (tmp && strcmp(tmp->fib.fib_FileName, name))
		tmp = tmp->Next;

	// File not found in current directory
	if (!tmp)
		return;

	if (tmp->fib.fib_DirEntryType > 0)
	{
		AddPart(data->listpath, tmp->fib.fib_FileName, 512);
		_cd(data, tmp);
	}
	else if ((tf = AllocVec(sizeof(struct TempFile), MEMF_ANY)))
	{
		AddTail((struct List *)&data->Temp, (struct Node *)tf);
		sprintf(tf->FileName, "T:%s", name);
		do
		{
			retry = FALSE;
#if !defined(__AROS__) && !defined(__MORPHOS__)
			if (data->ArcMode == XADCF_DISKARCHIVER)
			{
				err = xadDiskFileUnArc(data->ArcInf,
									   XAD_ENTRYNUMBER,
									   tmp->xfi->xfi_EntryNumber,
									   XAD_OUTFILENAME,
									   (IPTR)tf->FileName,
									   XAD_OVERWRITE,
									   TRUE,
									   (*data->password) ? XAD_PASSWORD : TAG_IGNORE,
									   (IPTR)data->password,
									   TAG_DONE);
			}
			else
#endif
			{
				err = xadFileUnArc(data->ArcInf,
								   XAD_ENTRYNUMBER,
								   tmp->xfi->xfi_EntryNumber,
								   XAD_OUTFILENAME,
								   (IPTR)tf->FileName,
								   XAD_OVERWRITE,
								   TRUE,
								   (*data->password) ? XAD_PASSWORD : TAG_IGNORE,
								   (IPTR)data->password,
								   TAG_DONE);
			}
			if (err == XADERR_PASSWORD)
				retry = PasswordReq(data);
		} while (retry);
		LaunchCommand(data, "DoubleClick", tf->FileName, qual);
	}
}

void _parent(struct xoData *data)
{
	struct Tree *tmp;
	char *path = data->listpath;

	while (*path++ != ':')
		;

	if (*path)
	{
		tmp = FindDrw(data, (UBYTE *)path);
		*((char *)PathPart(path)) = 0;
		_cd(data, tmp);
	}

	// At the archive root of a lister we took over: leave the archive
	// and read the real directory that contains it.  The read resets
	// the buffer, clears our handler and delivers the "inactive" trap
	// that ends our event loop
	else if (data->same_lister)
	{
		DOpusCallbackInfo *infoptr = &data->hook;

		sprintf(data->buf, "lister read %s \"%s\"", data->lists, data->origpath);
		DC_CALL4(
			infoptr, dc_SendCommand, DC_REGA0, IPCDATA(data->ipc), DC_REGA1, data->buf, DC_REGA2, NULL, DC_REGD0, 0);
	}
}

void _root(struct xoData *data)
{
	DOpusCallbackInfo *infoptr = &data->hook;

	strcpy(data->listpath, data->rootpath);
	sprintf(data->buf, "lister set %s path %s", data->lists, data->listpath);
	DC_CALL4(infoptr, dc_SendCommand, DC_REGA0, IPCDATA(data->ipc), DC_REGA1, data->buf, DC_REGA2, NULL, DC_REGD0, 0);
	// data->hook.dc_SendCommand(IPCDATA(data->ipc),data->buf,NULL,NULL);

	ChangeDir(data, data->root);
}

void _viewcommand(struct xoData *data, char *com, char *name)
{
	struct Tree *tmp = data->cur->Child;
	struct TempFile *tf;
	DOpusCallbackInfo *infoptr = &data->hook;
	xadERROR err;
	BOOL retry;

	while (tmp)
	{
		while (tmp && (sprintf(data->buf, "\"%s\"", tmp->fib.fib_FileName)) && (!strstr(name, data->buf)))
			tmp = tmp->Next;
		if (!tmp)
			break;
		{
			if ((tf = AllocVec(sizeof(struct TempFile), MEMF_ANY)))
			{
				AddTail((struct List *)&data->Temp, (struct Node *)tf);
				sprintf(tf->FileName, "T:%s", tmp->fib.fib_FileName);
				do
				{
					retry = FALSE;
#if !defined(__AROS__) && !defined(__MORPHOS__)
					if (data->ArcMode == XADCF_DISKARCHIVER)
					{
						err = xadDiskFileUnArc(data->ArcInf,
											   XAD_ENTRYNUMBER,
											   tmp->xfi->xfi_EntryNumber,
											   XAD_OUTFILENAME,
											   (IPTR)tf->FileName,
											   XAD_OVERWRITE,
											   TRUE,
											   (*data->password) ? XAD_PASSWORD : TAG_IGNORE,
											   (IPTR)data->password,
											   TAG_DONE);
					}
					else
#endif
					{
						err = xadFileUnArc(data->ArcInf,
										   XAD_ENTRYNUMBER,
										   tmp->xfi->xfi_EntryNumber,
										   XAD_OUTFILENAME,
										   (IPTR)tf->FileName,
										   XAD_OVERWRITE,
										   TRUE,
										   (*data->password) ? XAD_PASSWORD : TAG_IGNORE,
										   (IPTR)data->password,
										   TAG_DONE);
					}
					if (err == XADERR_PASSWORD)
						retry = PasswordReq(data);
				} while (retry);
				sprintf(data->buf, "lister select %s \"%s\" off", data->lists, tmp->fib.fib_FileName);
				DC_CALL4(infoptr,
						 dc_SendCommand,
						 DC_REGA0,
						 IPCDATA(data->ipc),
						 DC_REGA1,
						 data->buf,
						 DC_REGA2,
						 NULL,
						 DC_REGD0,
						 0);
				DC_CALL2(infoptr, dc_RefreshLister, DC_REGA0, data->listh, DC_REGD0, 0);
				/*data->hook.dc_SendCommand(IPCDATA(data->ipc),data->buf,NULL,NULL);
				data->hook.dc_RefreshLister(data->listh, NULL);*/
				LaunchCommand(data, com, tf->FileName, "");
			}
			tmp = tmp->Next;
		}
	}
}

ULONG countchar(char *buf)
{
	ULONG c = 0;

	while (*buf)
		if (*buf++ == '"')
			c++;
	return (c);
}

void _copy(struct xoData *data, char *name, char *Dest, BOOL CopyAs)
{
	struct Tree *tmp = data->cur->Child;
	struct xadFileInfo *xfi;
	char *FileName, *TreeName, *Drawer;
	struct Hook prhk;
	LONG count = 0, total;
	BOOL over = FALSE, skip;
	BPTR dir;
	DOpusCallbackInfo *infoptr = &data->hook;
	xadERROR err;
	BOOL retry;

	FileName = AllocVec(1024, 0);
	TreeName = AllocVec(1024, 0);
	Drawer = AllocVec(1024, 0);

	prhk.h_Entry = (ULONG(*)())L_ProgressHook;
	prhk.h_Data = data;

	total = countchar(name) / 2;

	data->All = 5;

	data->ptr = OpenProgressWindowTags(PW_Window,
									   (IPTR)data->listw,
									   PW_Title,
									   (IPTR)DOpusGetString(locale, MSG_EXTRACTING),
									   PW_FileCount,
									   total,
									   PW_FileSize,
									   1,
									   PW_Flags,
									   PWF_FILENAME | PWF_FILESIZE | PWF_GRAPH | PWF_ABORT | PWF_INFO,
									   TAG_DONE);

	while (tmp && (!over))
	{
		while (tmp && (sprintf(data->buf, "\"%s\"", tmp->fib.fib_FileName)) && (!strstr(name, data->buf)))
			tmp = tmp->Next;
		if (!tmp)
			break;
		{
			strcpy(FileName, Dest);
			skip = FALSE;
			if (CopyAs)
			{
				sprintf(TreeName, DOpusGetString(locale, MSG_RENAME_FORM), tmp->fib.fib_FileName);
				strcpy(Drawer, tmp->fib.fib_FileName);
				switch (AsyncRequestTags(data->ipc,
										 REQTYPE_SIMPLE,
										 NULL,
										 NULL,
										 NULL,
										 AR_Window,
										 (IPTR)data->listw,
										 AR_Message,
										 (IPTR)TreeName,
										 AR_Buffer,
										 (IPTR)Drawer,
										 AR_BufLen,
										 200,
										 AR_Button,
										 (IPTR)DOpusGetString(locale, MSG_RENAME),
										 AR_Button,
										 (IPTR)DOpusGetString(locale, MSG_SKIP),
										 AR_Button,
										 (IPTR)DOpusGetString(locale, MSG_ABORT),
										 AR_Flags,
										 SRF_PATH_FILTER,
										 TAG_END))
				{
				case 0:
					over = TRUE;
				case 2:
					skip = TRUE;
					break;
				default:
					AddPart(FileName, Drawer, 1024);
					break;
				}
			}
			else
				AddPart(FileName, tmp->fib.fib_FileName, 1024);

			if (!skip)
			{
				if (tmp->fib.fib_DirEntryType < 0)
				{
					SetProgressWindowTags(data->ptr,
										  PW_FileName,
										  (IPTR)tmp->fib.fib_FileName,
										  PW_FileNum,
										  ++count,
										  PW_FileDone,
										  0,
										  PW_FileSize,
										  tmp->xfi->xfi_Size,
										  PW_Info,
										  (IPTR) "",
										  TAG_DONE);
					do
					{
						retry = FALSE;
#if !defined(__AROS__) && !defined(__MORPHOS__)
						if (data->ArcMode == XADCF_DISKARCHIVER)
						{
							err = xadDiskFileUnArc(data->ArcInf,
												   XAD_ENTRYNUMBER,
												   tmp->xfi->xfi_EntryNumber,
												   XAD_OUTFILENAME,
												   (IPTR)FileName,
												   XAD_MAKEDIRECTORY,
												   TRUE,
												   XAD_PROGRESSHOOK,
												   (IPTR)&prhk,
												   XAD_OVERWRITE,
												   FALSE,
												   (*data->password) ? XAD_PASSWORD : TAG_IGNORE,
												   (IPTR)data->password,
												   TAG_DONE);
						}
						else
#endif
							err = xadFileUnArc(data->ArcInf,
											   XAD_ENTRYNUMBER,
											   tmp->xfi->xfi_EntryNumber,
											   XAD_OUTFILENAME,
											   (IPTR)FileName,
											   XAD_MAKEDIRECTORY,
											   TRUE,
											   XAD_PROGRESSHOOK,
											   (IPTR)&prhk,
											   XAD_OVERWRITE,
											   FALSE,
											   (*data->password) ? XAD_PASSWORD : TAG_IGNORE,
											   (IPTR)data->password,
											   TAG_DONE);

						if (err == XADERR_BREAK || CheckProgressAbort(data->ptr))
							over = TRUE;
						else if (err == XADERR_PASSWORD)
							retry = PasswordReq(data);
					} while (retry && !over);
				}
				else
				{
					xfi = data->ArcInf->xai_FileInfo;
					strcpy(Drawer, &data->listpath[strlen(data->rootpath)]);
					AddPart(Drawer, tmp->fib.fib_FileName, 1024);
					total = strlen(Drawer);
					SetProgressWindowTags(
						data->ptr, PW_FileName, (IPTR)tmp->fib.fib_FileName, PW_FileNum, ++count, TAG_DONE);
					while (xfi && (!over))
					{
						if ((!strncmp(Drawer, xfi->xfi_FileName, total)) && (xfi->xfi_FileName[total] == '/'))
						{
							strcpy(TreeName, FileName);
							AddPart(TreeName, &xfi->xfi_FileName[total + 1], 1024);
							SetProgressWindowTags(data->ptr,
												  PW_Info,
												  (IPTR)&xfi->xfi_FileName[total],
												  PW_FileDone,
												  0,
												  PW_FileSize,
												  xfi->xfi_Size,
												  TAG_DONE);
							if (xfi->xfi_Flags & (XADFIF_INFOTEXT | XADFIF_NOFILENAME))
							{
							}
							else if (XADFileInfoIsDirectory(xfi))
							{
								if ((dir = CreateDir(TreeName)))
									UnLock(dir);
							}
							else
								do
								{
									retry = FALSE;
#if !defined(__AROS__) && !defined(__MORPHOS__)
									if (data->ArcMode == XADCF_DISKARCHIVER)
									{
										err = xadDiskFileUnArc(data->ArcInf,
															   XAD_ENTRYNUMBER,
															   xfi->xfi_EntryNumber,
															   XAD_OUTFILENAME,
															   (IPTR)TreeName,
															   XAD_MAKEDIRECTORY,
															   TRUE,
															   XAD_PROGRESSHOOK,
															   (IPTR)&prhk,
															   XAD_OVERWRITE,
															   FALSE,
															   (*data->password) ? XAD_PASSWORD : TAG_IGNORE,
															   (IPTR)data->password,
															   TAG_DONE);
									}
									else
#endif
										err = xadFileUnArc(data->ArcInf,
														   XAD_ENTRYNUMBER,
														   xfi->xfi_EntryNumber,
														   XAD_OUTFILENAME,
														   (IPTR)TreeName,
														   XAD_MAKEDIRECTORY,
														   TRUE,
														   XAD_PROGRESSHOOK,
														   (IPTR)&prhk,
														   XAD_OVERWRITE,
														   FALSE,
														   (*data->password) ? XAD_PASSWORD : TAG_IGNORE,
														   (IPTR)data->password,
														   TAG_DONE);

									if (err == XADERR_BREAK || CheckProgressAbort(data->ptr))
										over = TRUE;
									else if (err == XADERR_PASSWORD)
										retry = PasswordReq(data);
								} while (retry && !over);
						}

						xfi = xfi->xfi_Next;
					}
				}
				sprintf(data->buf, "lister select %s \"%s\" off", data->lists, FilePart(tmp->fib.fib_FileName));
				DC_CALL4(infoptr,
						 dc_SendCommand,
						 DC_REGA0,
						 IPCDATA(data->ipc),
						 DC_REGA1,
						 data->buf,
						 DC_REGA2,
						 NULL,
						 DC_REGD0,
						 0);
				DC_CALL2(infoptr, dc_RefreshLister, DC_REGA0, data->listh, DC_REGD0, 0);
				/*data->hook.dc_SendCommand(IPCDATA(data->ipc),data->buf,NULL,NULL);
				data->hook.dc_RefreshLister(data->listh, NULL);*/
			}
			tmp = tmp->Next;
		}
	}

	if (data->ptr)
		CloseProgressWindow(data->ptr);

	if (FileName)
		FreeVec(FileName);
	if (TreeName)
		FreeVec(TreeName);
	if (Drawer)
		FreeVec(Drawer);
}

BOOL _path(struct xoData *data, char *path)
{
	char *old = path, *listerpath = data->listpath;
	struct Tree *tmp, *cur = data->root;
	char buf[50];
	int x;
	DOpusCallbackInfo *infoptr = &data->hook;

	while ((*path != ':') && (*listerpath == *path))
	{
		path++;
		listerpath++;
	}
	if (*path != ':')
	{
		sprintf(data->buf, "lister read %s %s", data->lists, old);
		DC_CALL4(
			infoptr, dc_SendCommand, DC_REGA0, IPCDATA(data->ipc), DC_REGA1, data->buf, DC_REGA2, NULL, DC_REGD0, 0);
		// data->hook.dc_SendCommand(IPCDATA(data->ipc),data->buf,NULL,NULL);
		return (TRUE);
	}

	*(++listerpath) = 0;
	x = strlen(path);
	if (path[x - 1] == '/')
		path[x - 1] = 0;

	while (*path)
	{
		tmp = cur;
		path++;
		for (x = 0; (buf[x] = *path) && (*path != '/'); x++, path++)
			;
		buf[x] = 0;

		if (!cur->Child)
		{
			ErrorReq(data, DOpusGetString(locale, MSG_NO_PATH_ERR));
			_cd(data, tmp);
			return (FALSE);
		}
		else
		{
			cur = cur->Child;
			while ((cur->fib.fib_DirEntryType < 0) || stricmp(cur->fib.fib_FileName, buf))
			{
				if (cur->Next)
					cur = cur->Next;
				else
				{
					ErrorReq(data, DOpusGetString(locale, MSG_NO_PATH_ERR));
					_cd(data, tmp);
					return (FALSE);
				}
			}
		}
		AddPart(data->listpath, cur->fib.fib_FileName, 512);
	}

	_cd(data, cur);

	return (FALSE);
}
///

/// Extract Archive
BOOL ExtractD(struct xoData *data)
{
	struct DosList *dl;
	struct DosEnvec *de;
	struct xadDiskInfo *xdi = data->ArcInf->xai_DiskInfo;
	struct xadDeviceInfo *dev;
	struct Hook prhk;
	WORD sel;
	Att_List *list;

	prhk.h_Entry = (ULONG(*)())L_ProgressHook;
	prhk.h_Data = data;

	if ((list = Att_NewList(0)))
	{
		if ((dl = LockDosList(LDF_DEVICES | LDF_READ)))
		{
			while ((dl = NextDosEntry(dl, LDF_DEVICES)))
			{
				de = BADDR(((struct FileSysStartupMsg *)BADDR(dl->dol_misc.dol_handler.dol_Startup))->fssm_Environ);
				if ((xdi->xdi_Heads == de->de_Surfaces) &&
					(xdi->xdi_Cylinders == (de->de_HighCyl - de->de_LowCyl + 1)) &&
					(xdi->xdi_TrackSectors == (de->de_SectorPerBlock * de->de_BlocksPerTrack)))
					Att_NewNode(list, (((char *)BADDR(dl->dol_Name)) + 1), 0, ADDNODEF_SORT);
			}
			UnLockDosList(LDF_DEVICES | LDF_READ);

			sel = SelectionList(list,
								data->listw,
								NULL,
								DOpusGetString(locale, MSG_SELECT_DEST),
								-1,
								0,
								NULL,
								DOpusGetString(locale, MSG_OK),
								DOpusGetString(locale, MSG_ABORT),
								NULL,
								NULL);

			if ((sel >= 0) && (dev = xadAllocObject(XADOBJ_DEVICEINFO, (IPTR)NULL)))
			{
				SetProgressWindowTags(data->ptr,
									  PW_FileName,
									  (IPTR)FilePart(data->arcname),
									  PW_Info,
									  (IPTR) "",
									  PW_FileDone,
									  0,
									  PW_FileSize,
									  (xdi->xdi_TotalSectors * xdi->xdi_SectorSize),
									  TAG_DONE);
				dev->xdi_DOSName = Att_NodeName(list, sel);
				xadDiskUnArc(data->ArcInf,
							 XAD_ENTRYNUMBER,
							 1,
							 XAD_OUTDEVICE,
							 (IPTR)dev,
							 XAD_VERIFY,
							 TRUE,
							 XAD_PROGRESSHOOK,
							 (IPTR)&prhk,
							 TAG_DONE);
				xadFreeObject(dev, (IPTR)NULL);
			}
		}
		Att_RemList(list, 0);
	}

	return (FALSE);
}

BOOL ExtractF(struct xoData *data)
{
	struct xadFileInfo *xfi;
	struct Hook prhk;
	char FileName[1024];
	BPTR dir;
	ULONG total = 1;
	BOOL over = FALSE;
	xadERROR err;
	BOOL retry;

	prhk.h_Entry = (ULONG(*)())L_ProgressHook;
	prhk.h_Data = data;

	data->All = 5;

	xfi = data->ArcInf->xai_FileInfo;
	while (xfi->xfi_Next)
	{
		total++;
		xfi = xfi->xfi_Next;
	}
	xfi = data->ArcInf->xai_FileInfo;

	SetProgressWindowTags(data->ptr, PW_FileCount, total, PW_FileSize, 0, TAG_DONE);

	total = 0;

	while (xfi && (!over))
	{
		strcpy(FileName, xfi->xfi_FileName);
		*((char *)FilePart(FileName)) = 0;

		SetProgressWindowTags(data->ptr,
							  PW_FileName,
							  (IPTR)FilePart(xfi->xfi_FileName),
							  PW_FileNum,
							  ++total,
							  PW_Info,
							  (IPTR)FileName,
							  PW_FileDone,
							  0,
							  PW_FileSize,
							  xfi->xfi_Size,
							  TAG_DONE);

		strcpy(FileName, data->listpath);
		AddPart(FileName, xfi->xfi_FileName, 1024);

		if (xfi->xfi_Flags & (XADFIF_INFOTEXT | XADFIF_NOFILENAME))
			;
		else if (XADFileInfoIsDirectory(xfi))
		{
			if ((dir = CreateDir(FileName)))
				UnLock(dir);
		}
		else
			do
			{
				retry = FALSE;
#if !defined(__AROS__) && !defined(__MORPHOS__)
				if (data->ArcMode == XADCF_DISKARCHIVER)
				{
					err = xadDiskFileUnArc(data->ArcInf,
										   XAD_ENTRYNUMBER,
										   xfi->xfi_EntryNumber,
										   XAD_OUTFILENAME,
										   (IPTR)FileName,
										   XAD_MAKEDIRECTORY,
										   TRUE,
										   XAD_PROGRESSHOOK,
										   (IPTR)&prhk,
										   XAD_OVERWRITE,
										   FALSE,
										   (*data->password) ? XAD_PASSWORD : TAG_IGNORE,
										   (IPTR)data->password,
										   TAG_DONE);
				}
				else
#endif
					err = xadFileUnArc(data->ArcInf,
									   XAD_ENTRYNUMBER,
									   xfi->xfi_EntryNumber,
									   XAD_OUTFILENAME,
									   (IPTR)FileName,
									   XAD_MAKEDIRECTORY,
									   TRUE,
									   XAD_PROGRESSHOOK,
									   (IPTR)&prhk,
									   XAD_OVERWRITE,
									   FALSE,
									   (*data->password) ? XAD_PASSWORD : TAG_IGNORE,
									   (IPTR)data->password,
									   TAG_DONE);

				if (err == XADERR_BREAK || CheckProgressAbort(data->ptr))
					over = TRUE;
				else if (err == XADERR_PASSWORD)
					retry = PasswordReq(data);
			} while (retry && !over);
		xfi = xfi->xfi_Next;
	}

	return (over);
}

/// Main
int LIBFUNC L_Module_Entry(REG(a0, char *args),
						   REG(a1, struct Screen *screen),
						   REG(a2, IPCData *ipc),
						   REG(a3, IPCData *main_ipc),
						   REG(d0, IPTR mod_id),
						   REG(d1, EXT_FUNC(func_callback)))
{
	char arcname[512];
	char buf[1088];
	STRPTR result;
	struct xoData data;
	struct Tree root;
	DOpusCallbackInfo *infoptr = &data.hook;

	struct function_entry *Entry;

	ULONG err, total;
	STRPTR filename;

	struct MyPacket *pkt;
	BOOL over = FALSE;
	char *a0, *a2, *a3, *a5, *a6, *a7;

	root.xfi = root.entry = root.Next = root.Child = NULL;
	data.ipc = ipc;
	data.root = &root;
	data.arcname = arcname;
	data.buf = buf;
	data.listw = NULL;
	data.cur = NULL;
	data.same_lister = FALSE;
	data.locale = locale;
	/*data.DOpusBase = DOpusBase;
	data.DOSBase = (APTR)DOSBase;
	data.UtilityBase = UtilityBase;*/
	data.password[0] = 0;

	NewList((struct List *)&data.Temp);

	data.hook.dc_Count = DOPUS_HOOK_COUNT;

	if (IPC_Command(main_ipc, HOOKCMD_GET_CALLBACKS, 0, &data.hook, 0, REPLY_NO_PORT) != DOPUS_HOOK_COUNT)
	{
		ErrorReq(&data, DOpusGetString(locale, MSG_NO_HOOKS_ERR));
		return (0);
	}

	if (!(data.rhand = NewMemHandle(0, 0, MEMF_CLEAR)))
		return (0);

	if (!(data.listp2 = (APTR)DC_CALL2(infoptr, dc_GetSource, DC_REGA0, IPCDATA(ipc), DC_REGA1, arcname)))
		return 0;

	DC_CALL1(infoptr, dc_FirstEntry, DC_REGA0, IPCDATA(ipc));

	if (!(Entry = (APTR)DC_CALL1(infoptr, dc_GetEntry, DC_REGA0, IPCDATA(ipc))))
		return 0;

	filename = (STRPTR)DC_CALL2(infoptr, dc_ExamineEntry, DC_REGA0, Entry, DC_REGD0, EE_NAME);

	/*if( !(data.listp2 = data.hook.dc_GetSource(IPCDATA(ipc), arcname)) ) return 0;

	data.hook.dc_FirstEntry(IPCDATA(ipc));

	if( !(Entry=data.hook.dc_GetEntry(IPCDATA(ipc))) ) return 0;

	filename = (STRPTR)data.hook.dc_ExamineEntry(Entry, EE_NAME);*/

	// note: this one was commented out by the author
	//	if( (!(data.listp2 = data.hook.dc_GetSource(IPCDATA(ipc), arcname))) ||
	//(!(Entry=data.hook.dc_GetEntry(IPCDATA(ipc)))) )

	// Remember the real directory the archive lives in (before the
	// filename is appended); XADOpenInLister needs it to leave the
	// archive again from the root
	strcpy(data.origpath, arcname);

	AddPart(arcname, filename, 512);

	strcpy(data.rootpath, filename);

	strcat(data.rootpath, ":");

	if (mod_id == 1)
	{
		if (!(data.destp = (APTR)DC_CALL2(infoptr, dc_GetDest, DC_REGA0, IPCDATA(ipc), DC_REGA1, data.listpath)))
			return 0;

		DC_CALL2(infoptr, dc_EndDest, DC_REGA0, IPCDATA(ipc), DC_REGD0, 0);
		/*if(!(data.destp = data.hook.dc_GetDest(IPCDATA(ipc), data.listpath))) return(0);
		data.hook.dc_EndDest(IPCDATA(ipc), 0);*/

		data.listh = (IPTR)data.listp2->lister;
		data.listw = (APTR)DC_CALL1(infoptr, dc_GetWindow, DC_REGA0, data.listp2);
		// data.listw = data.hook.dc_GetWindow(data.listp2);
		xad_format_handle(data.lists, data.listh);

		sprintf(buf, "lister query %s numselentries", data.lists);
		total = DC_CALL4(infoptr, dc_SendCommand, DC_REGA0, IPCDATA(ipc), DC_REGA1, buf, DC_REGA2, NULL, DC_REGD0, 0);
		// total=data.hook.dc_SendCommand(IPCDATA(ipc),buf,NULL,NULL);

		data.ptr = OpenProgressWindowTags(PW_Window,
										  (IPTR)data.listw,
										  PW_Title,
										  (IPTR)DOpusGetString(locale, MSG_EXTRACTING),
										  PW_FileCount,
										  total,
										  PW_Flags,
										  PWF_FILENAME | PWF_FILESIZE | PWF_GRAPH | PWF_ABORT | PWF_INFO,
										  TAG_DONE);

		if ((data.ArcInf = xadAllocObject(XADOBJ_ARCHIVEINFO, (IPTR)NULL)))
		{
			ULONG count = 0;
			while (Entry && (!over))
			{
				SetProgressWindowTags(data.ptr,
									  PW_Info,
									  (IPTR)FilePart(arcname),
									  PW_FileName,
									  (IPTR)DOpusGetString(locale, MSG_OPENING_ARC),
									  PW_FileNum,
									  ++count,
									  PW_FileCount,
									  total,
									  TAG_DONE);

				if (!(err = xadGetInfo(data.ArcInf, XAD_INFILENAME, (IPTR)arcname, TAG_DONE)))
					data.ArcMode = data.ArcInf->xai_Client->xc_Flags & XADCF_DISKARCHIVER;

				if (err == XADERR_FILETYPE)
				{
					// *** DISKIMAGE
					if (!(err = xadGetDiskInfo(data.ArcInf, XAD_INFILENAME, (IPTR)arcname, TAG_DONE)))
					{
						data.ArcMode = XADCF_DISKARCHIVER;
						over = ExtractF(&data);
						xadFreeInfo(data.ArcInf);
					}
				}
				else if ((!err) && (data.ArcMode == XADCF_DISKARCHIVER))
				{
					// *** DISKARCHIVE
					struct TagItem ti[2];
					switch (AsyncRequestTags(ipc,
											 REQTYPE_SIMPLE,
											 NULL,
											 NULL,
											 NULL,
											 AR_Window,
											 (IPTR)data.listw,
											 AR_Message,
											 (IPTR)DOpusGetString(locale, MSG_DISKARC_EXTRACT),
											 AR_Button,
											 (IPTR)DOpusGetString(locale, MSG_FILES),
											 AR_Button,
											 (IPTR)DOpusGetString(locale, MSG_DISK),
											 AR_Button,
											 (IPTR)DOpusGetString(locale, MSG_ABORT),
											 AR_Flags,
											 SRF_PATH_FILTER,
											 TAG_END))
					{
					case 0:
						break;
					case 2:
						over = ExtractD(&data);
						break;
					default:
						xadFreeInfo(data.ArcInf);
						ti[0].ti_Tag = XAD_INFILENAME;
						ti[0].ti_Data = (IPTR)arcname;
						ti[1].ti_Tag = TAG_DONE;
						if (!(err = xadGetDiskInfo(data.ArcInf, XAD_INDISKARCHIVE, (IPTR)ti, TAG_DONE)))
							over = ExtractF(&data);
						break;
					}
				}
				else if (!err)
					// *** FILEARCHIVE
					over = ExtractF(&data);

				if (!err)
					xadFreeInfo(data.ArcInf);
				else
					ErrorReq(&data, xadGetErrorText(err));
				DC_CALL3(infoptr, dc_EndEntry, DC_REGA0, IPCDATA(ipc), DC_REGA1, Entry, DC_REGD0, TRUE);
				Entry = (APTR)DC_CALL1(infoptr, dc_GetEntry, DC_REGA0, IPCDATA(ipc));
				/*data.hook.dc_EndEntry(IPCDATA(ipc),Entry,TRUE);
				Entry=data.hook.dc_GetEntry(IPCDATA(ipc));*/
				if (Entry)
				{
					*((char *)PathPart(arcname)) = 0;
					AddPart(arcname, Entry->name, 512);
				}
			}
			// No explicit destination refresh here.  This code runs inside the
			// function process that holds the destination lister's busy lock, so
			// a "command source <lister> ScanDir <path>" round-trip launches a
			// second function process which cannot re-lock the busy lister
			// (LISTER_BUSY answers IPC_ABORT), drops the lister handle and ends
			// up opening a duplicate lister for the destination instead of
			// refreshing it (issue #174).  The XADExtract filetype function
			// carries the "rescan destination" flag, which now refreshes the
			// destination lister when the function unwinds.

			xadFreeObject(data.ArcInf, (IPTR)NULL);
		}

		if (data.ptr)
			CloseProgressWindow(data.ptr);
		return (1);
	}

	DC_CALL2(infoptr, dc_EndSource, DC_REGA0, IPCDATA(ipc), DC_REGD0, 0);
	DC_CALL3(infoptr, dc_EndEntry, DC_REGA0, IPCDATA(ipc), DC_REGA1, Entry, DC_REGD0, TRUE);
	DC_CALL1(infoptr, dc_UnlockSource, DC_REGA0, IPCDATA(ipc));
	/*data.hook.dc_EndSource(IPCDATA(ipc), 0);
	data.hook.dc_EndEntry(IPCDATA(ipc), Entry, TRUE);
	data.hook.dc_UnlockSource(IPCDATA(ipc));*/

	//		data.listh = (IPTR)data.listp2->lister;
	//		sprintf(data.lists, "%d", data.listh);

	// XADOpenInLister: take over the source lister instead of opening
	// a fresh one.  Fall back to a new lister when there is no source
	// lister (desktop and AppMessage double-clicks pass none) or its
	// buffer already belongs to another custom handler (another
	// archive, an FTP session, ...)
	if (mod_id == 2 && data.listp2->lister)
	{
		data.listh = (IPTR)data.listp2->lister;
		xad_format_handle(data.lists, data.listh);

		sprintf(buf, "lister query %s handler", data.lists);
		if (!DC_CALL4(
				infoptr, dc_SendCommand, DC_REGA0, IPCDATA(ipc), DC_REGA1, buf, DC_REGA2, (APTR)&result, DC_REGD0, COMMANDF_RESULT))
		{
			if (result)
			{
				data.same_lister = (result[0] == 0);
				FreeVec(result);
			}
		}

		if (!data.same_lister)
			data.listh = 0;
	}

	if (!data.same_lister)
	{
		if (!DC_CALL4(infoptr,
					  dc_SendCommand,
					  DC_REGA0,
					  IPCDATA(ipc),
					  DC_REGA1,
					  "lister new",
					  DC_REGA2,
					  (APTR)&result,
					  DC_REGD0,
					  COMMANDF_RESULT))
		// if(!data.hook.dc_SendCommand(IPCDATA(ipc), "lister new", &result, COMMANDF_RESULT))
		{
			if (!result)
			{
				FreeMemHandle(data.rhand);
				RemoveTemp(&data);
				return 0;
			}

			data.listh = xad_parse_handle(result);
			FreeVec(result);

			if (!data.listh)
			{
				FreeMemHandle(data.rhand);
				RemoveTemp(&data);
				return 0;
			}

			xad_format_handle(data.lists, data.listh);
		}

		// Could not get a lister at all
		else
		{
			FreeMemHandle(data.rhand);
			RemoveTemp(&data);
			return 0;
		}
	}

	// Validate the archive before touching the lister: the takeover
	// below empties the buffer and installs the handler, so on failure
	// the source lister must still be untouched (and no fresh lister
	// should have been opened either)
	if (!(data.ArcInf = xadAllocObject(XADOBJ_ARCHIVEINFO, (IPTR)NULL)))
	{
		FreeMemHandle(data.rhand);
		RemoveTemp(&data);
		return 0;
	}

	err = xadGetInfo(data.ArcInf, XAD_INFILENAME, (IPTR)arcname, TAG_DONE);
	if (!err)
	{
		data.ArcMode = data.ArcInf->xai_Client->xc_Flags & XADCF_DISKARCHIVER;
	}
	else if (err == XADERR_FILETYPE)
	{
		if (!(err = xadGetDiskInfo(data.ArcInf, XAD_INFILENAME, (IPTR)arcname, TAG_DONE)))
		{
			data.ArcMode = XADCF_DISKARCHIVER;
		}
	}
	else if ((!err) && data.ArcMode)
	{
		struct TagItem ti[2];

		xadFreeInfo(data.ArcInf);
		ti[0].ti_Tag = XAD_INFILENAME;
		ti[0].ti_Data = (IPTR)arcname;
		ti[1].ti_Tag = TAG_DONE;
		err = xadGetDiskInfo(data.ArcInf, XAD_INDISKARCHIVE, (IPTR)ti, TAG_DONE);
	}

	if (err)
	{
		ErrorReq(&data, xadGetErrorText(err));
		xadFreeObject(data.ArcInf, (IPTR)NULL);
		FreeMemHandle(data.rhand);
		RemoveTemp(&data);
		return 0;
	}
	if (AllocPort(&data))
	{
		if (data.same_lister)
		{
			// The filetype function that invoked us holds this lister
			// busy for the whole module call; clear the visual busy
			// state so the lister stays usable while browsing
			sprintf(buf, "lister set %s busy off", data.lists);
			DC_CALL4(
				infoptr, dc_SendCommand, DC_REGA0, IPCDATA(ipc), DC_REGA1, buf, DC_REGA2, NULL, DC_REGD0, 0);

			// The taken-over lister still shows the real directory it was
			// reading; swap it to a fresh buffer so those entries don't
			// linger beneath the archive contents.  Deferred until the
			// port exists so an AllocPort failure leaves the lister
			// untouched; the handler just installed by AllocPort is
			// carried across the swap.  The old buffer goes to the cache
			sprintf(buf, "lister empty %s", data.lists);
			DC_CALL4(
				infoptr, dc_SendCommand, DC_REGA0, IPCDATA(ipc), DC_REGA1, buf, DC_REGA2, NULL, DC_REGD0, 0);
		}
		else
		{
			sprintf(buf, "lister wait %s quick", data.lists);
			DC_CALL4(
				infoptr, dc_SendCommand, DC_REGA0, IPCDATA(ipc), DC_REGA1, buf, DC_REGA2, NULL, DC_REGD0, 0);
			// data.hook.dc_SendCommand(IPCDATA(ipc), buf, NULL, NULL);
		}
			strcpy(data.listpath, data.rootpath);
			sprintf(buf, "lister set %s path %s", data.lists, data.listpath);
			DC_CALL4(infoptr, dc_SendCommand, DC_REGA0, IPCDATA(ipc), DC_REGA1, buf, DC_REGA2, NULL, DC_REGD0, 0);
			// data.hook.dc_SendCommand(IPCDATA(ipc), buf, NULL, NULL);

			sprintf(buf, "lister set %s source", data.lists);
			DC_CALL4(infoptr, dc_SendCommand, DC_REGA0, IPCDATA(ipc), DC_REGA1, buf, DC_REGA2, NULL, DC_REGD0, 0);
			// data.hook.dc_SendCommand(IPCDATA(ipc), buf, NULL, NULL);

			sprintf(buf, "lister set %s title %s", data.lists, DOpusGetString(locale, MSG_READING_ARCHIVE));
			DC_CALL4(infoptr, dc_SendCommand, DC_REGA0, IPCDATA(ipc), DC_REGA1, buf, DC_REGA2, NULL, DC_REGD0, 0);
			DC_CALL2(infoptr, dc_RefreshLister, DC_REGA0, data.listh, DC_REGD0, HOOKREFRESH_FULL);
			/*data.hook.dc_SendCommand(IPCDATA(ipc),buf,NULL,NULL);
			data.hook.dc_RefreshLister(data.listh, HOOKREFRESH_FULL);*/

			// The taken-over lister stays function-locked until we return;
			// a "lister wait" on it would never come back
			if (!data.same_lister)
			{
				sprintf(buf, "lister wait %s quick", data.lists);
				DC_CALL4(
					infoptr, dc_SendCommand, DC_REGA0, IPCDATA(ipc), DC_REGA1, buf, DC_REGA2, NULL, DC_REGD0, 0);
				// data.hook.dc_SendCommand(IPCDATA(ipc), buf, NULL, NULL);
			}


			data.listp.lister = (APTR)data.listh;
			*data.listp.buffer = data.listp.flags = 0;
			data.listp.path = data.listp.buffer;
			data.listw = (APTR)DC_CALL1(infoptr, dc_GetWindow, DC_REGA0, &data.listp);
			// data.listw = data.hook.dc_GetWindow(&data.listp);

					BuildTree(&data);
					ChangeDir(&data, &root);

					sprintf(buf, "lister set %s title XADopus: %s", data.lists, FilePart(arcname));
					DC_CALL4(
						infoptr, dc_SendCommand, DC_REGA0, IPCDATA(ipc), DC_REGA1, buf, DC_REGA2, NULL, DC_REGD0, 0);
					DC_CALL2(infoptr, dc_RefreshLister, DC_REGA0, data.listh, DC_REGD0, HOOKREFRESH_FULL);
					/*data.hook.dc_SendCommand(IPCDATA(ipc), buf, NULL, NULL);
					data.hook.dc_RefreshLister(data.listh, HOOKREFRESH_FULL);*/

					while (!over)
					{
						Wait(1L << data.mp->mp_SigBit);
						while ((!over) && (pkt = (struct MyPacket *)GetMsg(data.mp)))
						{
							a0 = (char *)pkt->sp_Pkt.dp_Arg1;
							a2 = (char *)pkt->sp_Pkt.dp_Arg3;
							a3 = (char *)pkt->sp_Pkt.dp_Arg4;
							a5 = (char *)pkt->sp_Pkt.dp_Arg6;
							a6 = (char *)pkt->sp_Pkt.dp_Arg7;
							a7 = pkt->a7;

							if (!strcmp(a0, "inactive"))
							{
								sprintf(buf, "dopus remtrap * %s", data.mp_name);
								DC_CALL4(infoptr,
										 dc_SendCommand,
										 DC_REGA0,
										 IPCDATA(ipc),
										 DC_REGA1,
										 buf,
										 DC_REGA2,
										 NULL,
										 DC_REGD0,
										 0);
								// data.hook.dc_SendCommand(IPCDATA(ipc), buf, NULL, NULL);
								over = TRUE;
							}
							else if (!stricmp(a0, "doubleclick"))
							{
								_doubleclick(&data, a2, a6);
							}
							else if (!stricmp(a0, "parent"))
							{
								_parent(&data);
							}
							else if (!stricmp(a0, "root"))
							{
								_root(&data);
							}
							else if (!strcmp(a0, "path"))
							{
								over = _path(&data, a2);
							}
							else if (strstr(viewcmds, a0))
							{
								_viewcommand(&data, a0, a2);
							}
							else if (!strcmp(a0, "dropfrom"))
							{
								_copy(&data, a2, a5, FALSE);
								_scandir(&data, a3, a5);
							}
							else if ((!stricmp(a0, "copy")) || (!stricmp(a0, "move")))
							{
								_copy(&data, a2, a7, FALSE);
								_scandir(&data, a3, a7);
							}
							else if ((!stricmp(a0, "copyas")) || (!stricmp(a0, "moveas")))
							{
								_copy(&data, a2, a7, TRUE);
								_scandir(&data, a3, a7);
							}
							ReplyMsg((struct Message *)pkt);
						}
					}
					xadFreeInfo(data.ArcInf);
			FreePort(&data);
	}

	xadFreeObject(data.ArcInf, (IPTR)NULL);

	FreeMemHandle(data.rhand);
	RemoveTemp(&data);

	return (1);
}
///
