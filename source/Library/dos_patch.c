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

//#define DOSBase (data->dos_base)

/********************************** CreateDir **********************************/

// Patched CreateDir()
PATCHED_1(BPTR, LIBFUNC L_PatchedCreateDir, d1, char *, name)
{
	atomic_inc(&usecount[WB_PATCH_CREATEDIR]);
	if (!WB_PATCH_ACTIVE(WB_PATCH_CREATEDIR))
	{
		BPTR res = WB_CALL_OLD_1(BPTR, WB_PATCH_CREATEDIR, d1, name);
		atomic_dec(&usecount[WB_PATCH_CREATEDIR]);
		return res;
	}
	{
		struct MyLibrary *libbase;
		struct FileInfoBlock *fib;
		BPTR lock;

		// Get library
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_CREATEDIR]);
			return 0;
		}

		// Call original function
		lock = WB_CALL_OLD_1(BPTR, WB_PATCH_CREATEDIR, d1, name);

		// Failed?
		if (!lock)
		{
			atomic_dec(&usecount[WB_PATCH_CREATEDIR]);
			return 0;
		}

		// Get directory information
		if ((fib = dospatch_fib(lock, libbase, 1)))
		{
			// Send notification
			L_SendNotifyMsg(DN_DOS_ACTION, 0, DNF_DOS_CREATEDIR, 0, (char *)(fib + 1), fib, libbase);

			// Free info block
			L_FreeMemH(fib);
		}

		atomic_dec(&usecount[WB_PATCH_CREATEDIR]);
		return lock;
	}
}
PATCH_END

// Calls original CreateDir directly
BPTR LIBFUNC L_OriginalCreateDir(REG(d1, char *name), REG(a6, struct MyLibrary *libbase))
{
	// Without our patch running there is nothing to bypass
	if (!WB_PATCH_ACTIVE(WB_PATCH_CREATEDIR))
		return CreateDir(name);

	// Create directory
	return WB_CALL_OLD_1(BPTR, WB_PATCH_CREATEDIR, d1, name);
}

/********************************** DeleteFile **********************************/

// Patched DeleteFile()
PATCHED_1(long, LIBFUNC L_PatchedDeleteFile, d1, char *, name)
{
	atomic_inc(&usecount[WB_PATCH_DELETEFILE]);
	if (!WB_PATCH_ACTIVE(WB_PATCH_DELETEFILE))
	{
		long res = WB_CALL_OLD_1(long, WB_PATCH_DELETEFILE, d1, name);
		atomic_dec(&usecount[WB_PATCH_DELETEFILE]);
		return res;
	}
	{
		struct MyLibrary *libbase;
		char *buf = NULL;
		BPTR lock;
		long res;
		APTR wsave = (APTR)-1;
		struct Process *task;

		// Get library
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_DELETEFILE]);
			return 0;
		}

		// Find process
		if ((task = (struct Process *)FindTask(NULL)))
		{
			// Is it a process?
			if (task->pr_Task.tc_Node.ln_Type == NT_PROCESS)
			{
				// Save pointer and turn requesters off
				wsave = task->pr_WindowPtr;
				task->pr_WindowPtr = (APTR)-1;
			}
			else
				task = NULL;
		}

		// Try to lock file to delete
		if ((lock = Lock(name, ACCESS_READ)))
		{
			// Allocate buffer
			if ((buf = AllocVec(512, MEMF_CLEAR)))
			{
				// Get full path
				L_DevNameFromLockDopus(lock, buf, 512, libbase);
			}

			// Unlock file
			UnLock(lock);
		}

		// Fix requesters
		if (task)
			task->pr_WindowPtr = wsave;

		// Call original function
		res = WB_CALL_OLD_1(long, WB_PATCH_DELETEFILE, d1, name);

		// Failed?
		if (!res)
		{
			atomic_dec(&usecount[WB_PATCH_DELETEFILE]);
			return 0;
		}

		// Got name?
		if (buf && *buf)
		{
			// Send notification
			L_SendNotifyMsg(DN_DOS_ACTION, 0, DNF_DOS_DELETEFILE, 0, buf, 0, libbase);
		}

		// Free buffer
		FreeVec(buf);

		atomic_dec(&usecount[WB_PATCH_DELETEFILE]);
		return res;
	}
}
PATCH_END

// Original DeleteFile
long LIBFUNC L_OriginalDeleteFile(REG(d1, char *name), REG(a6, struct MyLibrary *libbase))
{
	// Without our patch running there is nothing to bypass
	if (!WB_PATCH_ACTIVE(WB_PATCH_DELETEFILE))
		return DeleteFile(name);

	// Delete file
	return WB_CALL_OLD_1(long, WB_PATCH_DELETEFILE, d1, name);
}

/********************************** SetFileDate **********************************/

// Patched SetFileDate()
PATCHED_2(BOOL, LIBFUNC L_PatchedSetFileDate, d1, char *, name, d2, struct DateStamp *, date)
{
	atomic_inc(&usecount[WB_PATCH_SETFILEDATE]);
	if (!WB_PATCH_ACTIVE(WB_PATCH_SETFILEDATE))
	{
		BOOL res = WB_CALL_OLD_2(BOOL, WB_PATCH_SETFILEDATE, d1, name, d2, date);
		atomic_dec(&usecount[WB_PATCH_SETFILEDATE]);
		return res;
	}
	{
		struct MyLibrary *libbase;
		BOOL res;
		BPTR lock;
		APTR wsave = (APTR)-1;
		struct Process *task;

		// Get library
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_SETFILEDATE]);
			return FALSE;
		}

		// Call original function
		res = WB_CALL_OLD_2(BOOL, WB_PATCH_SETFILEDATE, d1, name, d2, date);

		// Failed?
		if (!res)
		{
			atomic_dec(&usecount[WB_PATCH_SETFILEDATE]);
			return FALSE;
		}

		// Find process
		if ((task = (struct Process *)FindTask(NULL)))
		{
			// Is it a process?
			if (task->pr_Task.tc_Node.ln_Type == NT_PROCESS)
			{
				// Save pointer and turn requesters off
				wsave = task->pr_WindowPtr;
				task->pr_WindowPtr = (APTR)-1;
			}
			else
				task = NULL;
		}

		// Try to lock file
		if ((lock = Lock(name, ACCESS_READ)))
		{
			struct FileInfoBlock *fib;

			// Get directory information
			if ((fib = dospatch_fib(lock, libbase, 0)))
			{
				// Send notification
				L_SendNotifyMsg(DN_DOS_ACTION, 0, DNF_DOS_SETFILEDATE, 0, (char *)(fib + 1), fib, libbase);

				// Free info block
				L_FreeMemH(fib);
			}

			// Unlock file
			UnLock(lock);
		}

		// Fix requesters
		if (task)
			task->pr_WindowPtr = wsave;

		atomic_dec(&usecount[WB_PATCH_SETFILEDATE]);
		return res;
	}
}
PATCH_END

// Calls original SetFileDate directly
BOOL LIBFUNC L_OriginalSetFileDate(REG(d1, char *name),
								   REG(d2, struct DateStamp *date),
								   REG(a6, struct MyLibrary *libbase))
{
	// Without our patch running there is nothing to bypass
	if (!WB_PATCH_ACTIVE(WB_PATCH_SETFILEDATE))
		return (BOOL)SetFileDate(name, date);

	// Set date
	return WB_CALL_OLD_2(BOOL, WB_PATCH_SETFILEDATE, d1, name, d2, date);
}

/********************************** SetComment **********************************/

// Patched SetComment()
PATCHED_2(BOOL, LIBFUNC L_PatchedSetComment, d1, char *, name, d2, char *, comment)
{
	atomic_inc(&usecount[WB_PATCH_SETCOMMENT]);
	if (!WB_PATCH_ACTIVE(WB_PATCH_SETCOMMENT))
	{
		BOOL res = WB_CALL_OLD_2(BOOL, WB_PATCH_SETCOMMENT, d1, name, d2, comment);
		atomic_dec(&usecount[WB_PATCH_SETCOMMENT]);
		return res;
	}
	{
		struct MyLibrary *libbase;
		BOOL res;
		BPTR lock;
		APTR wsave = (APTR)-1;
		struct Process *task;

		// Get library
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_SETCOMMENT]);
			return FALSE;
		}

		// Call original function
		res = WB_CALL_OLD_2(BOOL, WB_PATCH_SETCOMMENT, d1, name, d2, comment);

		// Failed?
		if (!res)
		{
			atomic_dec(&usecount[WB_PATCH_SETCOMMENT]);
			return FALSE;
		}

		// Find process
		if ((task = (struct Process *)FindTask(NULL)))
		{
			// Is it a process?
			if (task->pr_Task.tc_Node.ln_Type == NT_PROCESS)
			{
				// Save pointer and turn requesters off
				wsave = task->pr_WindowPtr;
				task->pr_WindowPtr = (APTR)-1;
			}
			else
				task = NULL;
		}

		// Lock file
		if ((lock = Lock(name, ACCESS_READ)))
		{
			struct FileInfoBlock *fib;

			// Get directory information
			if ((fib = dospatch_fib(lock, libbase, 0)))
			{
				// Send notification
				L_SendNotifyMsg(DN_DOS_ACTION, 0, DNF_DOS_SETCOMMENT, 0, (char *)(fib + 1), fib, libbase);

				// Free info block
				L_FreeMemH(fib);
			}

			// Unlock file
			UnLock(lock);
		}

		// Fix requesters
		if (task)
			task->pr_WindowPtr = wsave;

		atomic_dec(&usecount[WB_PATCH_SETCOMMENT]);
		return res;
	}
}
PATCH_END

// Calls original SetComment directly
BOOL LIBFUNC L_OriginalSetComment(REG(d1, char *name), REG(d2, char *comment), REG(a6, struct MyLibrary *libbase))
{
	// Without our patch running there is nothing to bypass
	if (!WB_PATCH_ACTIVE(WB_PATCH_SETCOMMENT))
		return (BOOL)SetComment(name, comment);

	// Set comment
	return WB_CALL_OLD_2(BOOL, WB_PATCH_SETCOMMENT, d1, name, d2, comment);
}

/********************************** SetProtection **********************************/

// Patched SetProtection()
PATCHED_2(BOOL, LIBFUNC L_PatchedSetProtection, d1, char *, name, d2, ULONG, mask)
{
	atomic_inc(&usecount[WB_PATCH_SETPROTECTION]);
	if (!WB_PATCH_ACTIVE(WB_PATCH_SETPROTECTION))
	{
		BOOL res = WB_CALL_OLD_2(BOOL, WB_PATCH_SETPROTECTION, d1, name, d2, mask);
		atomic_dec(&usecount[WB_PATCH_SETPROTECTION]);
		return res;
	}
	{
		struct MyLibrary *libbase;
		BOOL res;
		BPTR lock;
		APTR wsave = (APTR)-1;
		struct Process *task;

		// Get library
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_SETPROTECTION]);
			return FALSE;
		}

		// Call original function
		res = WB_CALL_OLD_2(BOOL, WB_PATCH_SETPROTECTION, d1, name, d2, mask);

		// Failed?
		if (!res)
		{
			atomic_dec(&usecount[WB_PATCH_SETPROTECTION]);
			return FALSE;
		}

		// Find process
		if ((task = (struct Process *)FindTask(NULL)))
		{
			// Is it a process?
			if (task->pr_Task.tc_Node.ln_Type == NT_PROCESS)
			{
				// Save pointer and turn requesters off
				wsave = task->pr_WindowPtr;
				task->pr_WindowPtr = (APTR)-1;
			}
			else
				task = NULL;
		}

		// Lock file
		if ((lock = Lock(name, ACCESS_READ)))
		{
			struct FileInfoBlock *fib;

			// Get directory information
			if ((fib = dospatch_fib(lock, libbase, 0)))
			{
				// Send notification
				L_SendNotifyMsg(DN_DOS_ACTION, 0, DNF_DOS_SETPROTECTION, 0, (char *)(fib + 1), fib, libbase);

				// Free info block
				L_FreeMemH(fib);
			}

			// Unlock file
			UnLock(lock);
		}

		// Fix requesters
		if (task)
			task->pr_WindowPtr = wsave;

		atomic_dec(&usecount[WB_PATCH_SETPROTECTION]);
		return res;
	}
}
PATCH_END

// Calls original SetProtection directly
BOOL LIBFUNC L_OriginalSetProtection(REG(d1, char *name), REG(d2, ULONG mask), REG(a6, struct MyLibrary *libbase))
{
	// Without our patch running there is nothing to bypass
	if (!WB_PATCH_ACTIVE(WB_PATCH_SETPROTECTION))
		return (BOOL)SetProtection(name, mask);

	// Set protection
	return WB_CALL_OLD_2(BOOL, WB_PATCH_SETPROTECTION, d1, name, d2, mask);
}

/********************************** Rename **********************************/

// Patched Rename()
PATCHED_2(BOOL, LIBFUNC L_PatchedRename, d1, char *, oldname, d2, char *, newname)
{
	atomic_inc(&usecount[WB_PATCH_RENAME]);
	if (!WB_PATCH_ACTIVE(WB_PATCH_RENAME))
	{
		BOOL res = WB_CALL_OLD_2(BOOL, WB_PATCH_RENAME, d1, oldname, d2, newname);
		atomic_dec(&usecount[WB_PATCH_RENAME]);
		return res;
	}
	{
		struct FileInfoBlock *fib = NULL;
		struct MyLibrary *libbase;
		BOOL res;
		BPTR lock;
		char old_name[108];
		APTR wsave = (APTR)-1;
		struct Process *task;

		// Get library
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_RENAME]);
			return FALSE;
		}

		// Find process
		if ((task = (struct Process *)FindTask(NULL)))
		{
			// Is it a process?
			if (task->pr_Task.tc_Node.ln_Type == NT_PROCESS)
			{
				// Save pointer and turn requesters off
				wsave = task->pr_WindowPtr;
				task->pr_WindowPtr = (APTR)-1;
			}
			else
				task = NULL;
		}

		// Get lock on old file?
		if ((lock = Lock(oldname, ACCESS_READ)))
		{
			// Get file information
			if (!(fib = dospatch_fib(lock, libbase, 0)))
			{
				// Failed to get info
				UnLock(lock);
				lock = 0;
			}
		}

		// Fix requesters
		if (task)
			task->pr_WindowPtr = wsave;

		// Call original function
		res = WB_CALL_OLD_2(BOOL, WB_PATCH_RENAME, d1, oldname, d2, newname);

		// Failure?
		if (!res)
		{
			// Unlock and free fib
			if (fib)
				L_FreeMemH(fib);
			if (lock)
				UnLock(lock);
			atomic_dec(&usecount[WB_PATCH_RENAME]);
			return res;
		}

		// Got no Fib?
		if (!fib)
		{
			atomic_dec(&usecount[WB_PATCH_RENAME]);
			return res;
		}

		// Store old name
		strcpy(old_name, fib->fib_FileName);

		// Turn requesters off
		if (task)
			task->pr_WindowPtr = (APTR)-1;

			// Get new information
#ifdef USE_64BIT
		if (L_ExamineLock64(lock, (FileInfoBlock64 *)fib))
#else
		if (Examine(lock, fib))
#endif
		{
			// Copy new name to comment
			stccpy(fib->fib_Comment, fib->fib_FileName, 79);

			// Restore old name
			strcpy(fib->fib_FileName, old_name);

			// Send notification
			L_SendNotifyMsg(DN_DOS_ACTION, 0, DNF_DOS_RENAME, 0, (char *)(fib + 1), fib, libbase);
		}

		// Free info block and lock
		L_FreeMemH(fib);
		UnLock(lock);

		// Fix requesters
		if (task)
			task->pr_WindowPtr = wsave;

		atomic_dec(&usecount[WB_PATCH_RENAME]);
		return res;
	}
}
PATCH_END

// Calls original Rename directly
BOOL LIBFUNC L_OriginalRename(REG(d1, char *oldname), REG(d2, char *newname), REG(a6, struct MyLibrary *libbase))
{
	// Without our patch running there is nothing to bypass
	if (!WB_PATCH_ACTIVE(WB_PATCH_RENAME))
		return (BOOL)Rename(oldname, newname);

	// Rename file
	return WB_CALL_OLD_2(BOOL, WB_PATCH_RENAME, d1, oldname, d2, newname);
}

/********************************** Relabel **********************************/

// Patched Relabel()
PATCHED_2(BOOL, LIBFUNC L_PatchedRelabel, d1, char *, volumename, d2, char *, name)
{
	atomic_inc(&usecount[WB_PATCH_RELABEL]);
	if (!WB_PATCH_ACTIVE(WB_PATCH_RELABEL))
	{
		BOOL res = WB_CALL_OLD_2(BOOL, WB_PATCH_RELABEL, d1, volumename, d2, name);
		atomic_dec(&usecount[WB_PATCH_RELABEL]);
		return res;
	}
	{
		struct MyLibrary *libbase;
		struct FileInfoBlock fib;
		BOOL res;

		// Get library
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_RELABEL]);
			return FALSE;
		}

		// Call original function
		res = WB_CALL_OLD_2(BOOL, WB_PATCH_RELABEL, d1, volumename, d2, name);

		// Failure?
		if (!res)
		{
			atomic_dec(&usecount[WB_PATCH_RELABEL]);
			return res;
		}

		// Fake fib with new name
		strcpy(fib.fib_FileName, name);

		// Send notification
		L_SendNotifyMsg(DN_DOS_ACTION, 0, DNF_DOS_RELABEL, 0, volumename, &fib, libbase);

		atomic_dec(&usecount[WB_PATCH_RELABEL]);
		return res;
	}
}
PATCH_END

// Calls original Relabel directly
BOOL LIBFUNC L_OriginalRelabel(REG(d1, char *volumename), REG(d2, char *name), REG(a6, struct MyLibrary *libbase))
{
	// Without our patch running there is nothing to bypass
	if (!WB_PATCH_ACTIVE(WB_PATCH_RELABEL))
		return (BOOL)Relabel(volumename, name);

	// Relabel disk
	return WB_CALL_OLD_2(BOOL, WB_PATCH_RELABEL, d1, volumename, d2, name);
}

/********************************** Open **********************************/

// Patched Open()
PATCHED_2(BPTR, LIBFUNC L_PatchedOpen, d1, char *, name, d2, LONG, accessMode)
{
	atomic_inc(&usecount[WB_PATCH_OPEN]);
	if (!WB_PATCH_ACTIVE(WB_PATCH_OPEN))
	{
		BPTR res = WB_CALL_OLD_2(BPTR, WB_PATCH_OPEN, d1, name, d2, accessMode);
		atomic_dec(&usecount[WB_PATCH_OPEN]);
		return res;
	}
	{
		struct LibData *data;
		struct MyLibrary *libbase;
		BPTR file, lock;
		BOOL create = FALSE;
		APTR wsave = (APTR)-1;
		struct Process *task;
		struct FileHandleWrapper *handle;

		// Get library
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_OPEN]);
			return 0;
		}

		// Get data pointer
		data = (struct LibData *)libbase->ml_UserData;

		// Find process
		if ((task = (struct Process *)FindTask(NULL)))
		{
			// Is it a process?
			if (task->pr_Task.tc_Node.ln_Type == NT_PROCESS)
			{
				// Save pointer and turn requesters off
				wsave = task->pr_WindowPtr;
				task->pr_WindowPtr = (APTR)-1;
			}
			else
				task = NULL;
		}

		// Check mode
		switch (accessMode)
		{
		// Create?
		case MODE_NEWFILE:
			create = TRUE;
			break;

		// Read/write
		case MODE_READWRITE:

			// If file is going to be created, we use it
			if (!(lock = Lock(name, ACCESS_READ)))
				create = TRUE;
			else
				UnLock(lock);
			break;
		}

		// Fix requesters
		if (task)
			task->pr_WindowPtr = wsave;

		// Call original function
		file = WB_CALL_OLD_2(BPTR, WB_PATCH_OPEN, d1, name, d2, accessMode);

		// Failure?
		if (!file)
		{
			atomic_dec(&usecount[WB_PATCH_OPEN]);
			return file;
		}

		// If file is Interactive, assume it's not a filesystem
		if (IsInteractive(file))
		{
			atomic_dec(&usecount[WB_PATCH_OPEN]);
			return file;
		}

		// Disable requesters
		if (task)
			task->pr_WindowPtr = (APTR)-1;

		// Get lock on parent directory
		if ((lock = ParentOfFH(file)))
		{
			// Allocate wrapper handle
			if ((handle = AllocVec(sizeof(struct FileHandleWrapper), 0)))
			{
				// Fill out handle
				handle->fhw_FileHandle = file;
				handle->fhw_Flags = 0;
				handle->fhw_Parent = lock;

				// Lock file list
				ObtainSemaphore(&data->file_list.lock);

				// Add to head of list
				AddHead(&data->file_list.list, (struct Node *)handle);

				// Unlock file list
				ReleaseSemaphore(&data->file_list.lock);
			}

			// If this was a create operation, we need to send a message now.
			// The FIB+512 path scratch buffer is only allocated on this hot
			// branch - opening for read (the common case) skips the alloc
			// entirely.  Comes from the shared dos_patch_memory pool so all
			// the dos_patch.c FIB scratch traffic flows through one arena.
			if (create)
			{
				struct FileInfoBlock *fib;

				if ((fib = L_AllocMemH(data->dos_patch_memory, sizeof(struct FileInfoBlock) + 512)))
				{
					// Get buffer pointer
					char *path = (char *)(fib + 1);

					// Get full path of parent, and terminate it
					L_DevNameFromLockDopus(lock, path, 512, libbase);
					AddPart(path, "", 512);

					// Got path?
					if (*path)
					{
						// Examine the new file
#ifdef USE_64BIT
						if (L_ExamineHandle64(file, (FileInfoBlock64 *)fib))
#else
						if (ExamineFH(file, fib))
#endif
						{
							// Send notification
							L_SendNotifyMsg(DN_DOS_ACTION, 0, DNF_DOS_CREATE, 0, path, fib, libbase);
						}
					}

					// Free fib
					L_FreeMemH(fib);
				}
			}

			// Unlock parent, unless handle has grabbed it
			if (!handle)
				UnLock(lock);
		}

		// Fix requesters
		if (task)
			task->pr_WindowPtr = wsave;

		atomic_dec(&usecount[WB_PATCH_OPEN]);
		return file;
	}
}
PATCH_END

// Calls original Open directly
BPTR LIBFUNC L_OriginalOpen(REG(d1, char *name), REG(d2, LONG accessMode), REG(a6, struct MyLibrary *libbase))
{
	// Without our patch running there is nothing to bypass
	if (!WB_PATCH_ACTIVE(WB_PATCH_OPEN))
		return Open(name, accessMode);

	// Open file
	return WB_CALL_OLD_2(BPTR, WB_PATCH_OPEN, d1, name, d2, accessMode);
}

/********************************** Close **********************************/

// Patched Close()
PATCHED_1(BOOL, LIBFUNC L_PatchedClose, d1, BPTR, file)
{
	atomic_inc(&usecount[WB_PATCH_CLOSE]);
	if (!WB_PATCH_ACTIVE(WB_PATCH_CLOSE))
	{
		BOOL res = WB_CALL_OLD_1(BOOL, WB_PATCH_CLOSE, d1, file);
		atomic_dec(&usecount[WB_PATCH_CLOSE]);
		return res;
	}
	{
		struct LibData *data;
		struct MyLibrary *libbase;
		BOOL res;

		// Get library
		if (!file || !(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_CLOSE]);
			return FALSE;
		}

		// Get data pointer
		data = (struct LibData *)libbase->ml_UserData;

		// If this is the 'last' file, clear pointer
		if (data->last_file == file)
			data->last_file = 0;

		// Is file not interactive (ie a filesystem)?
		if (!IsInteractive(file))
		{
			struct FileHandleWrapper *handle;

			// See if we have a wrapper
			if ((handle = find_filehandle(file, data)))
			{
				// Remove it from the list
				Remove((struct Node *)handle);

				// Unlock the list
				ReleaseSemaphore(&data->file_list.lock);

				// Did the file get written to?
				if (handle->fhw_Flags & FHWF_WRITTEN)
				{
					struct FileInfoBlock *fib;
					APTR wsave = (APTR)-1;
					struct Process *task;

					// Find process
					if ((task = (struct Process *)FindTask(NULL)))
					{
						// Is it a process?
						if (task->pr_Task.tc_Node.ln_Type == NT_PROCESS)
						{
							// Save pointer and turn requesters off
							wsave = task->pr_WindowPtr;
							task->pr_WindowPtr = (APTR)-1;
						}
						else
							task = NULL;
					}

					// Allocate FileInfoBlock and buffer from the shared pool
					if ((fib = L_AllocMemH(data->dos_patch_memory, sizeof(struct FileInfoBlock) + 512)))
					{
						char *path = (char *)(fib + 1);

						// Get full path of parent and terminate it
						L_DevNameFromLockDopus(handle->fhw_Parent, path, 512, libbase);
						AddPart(path, "", 512);

						// Got path?
						if (*path)
						{
							// Examine the file
#ifdef USE_64BIT
							if (L_ExamineHandle64(file, (FileInfoBlock64 *)fib))
#else
							if (ExamineFH(file, fib))
#endif
							{
								// Send notification
								L_SendNotifyMsg(DN_DOS_ACTION, 0, DNF_DOS_CLOSE, 0, path, fib, libbase);
							}
						}

						// Free fib
						L_FreeMemH(fib);
					}

					// Fix requesters
					if (task)
						task->pr_WindowPtr = wsave;
				}

				// Unlock parent lock
				UnLock(handle->fhw_Parent);

				// Free wrapper handle
				FreeVec(handle);
			}
		}

		// Close file
		res = WB_CALL_OLD_1(BOOL, WB_PATCH_CLOSE, d1, file);

		atomic_dec(&usecount[WB_PATCH_CLOSE]);
		return res;
	}
}
PATCH_END

// Calls original Close directly
BOOL LIBFUNC L_OriginalClose(REG(d1, BPTR file), REG(a6, struct MyLibrary *libbase))
{
	// Without our patch running there is nothing to bypass
	if (!WB_PATCH_ACTIVE(WB_PATCH_CLOSE))
		return (BOOL)Close(file);

	// Close file
	return WB_CALL_OLD_1(BOOL, WB_PATCH_CLOSE, d1, file);
}

/********************************** Write **********************************/

// Patched Write()
PATCHED_3(LONG, LIBFUNC L_PatchedWrite, d1, BPTR, file, d2, void *, wdata, d3, LONG, length)
{
	atomic_inc(&usecount[WB_PATCH_WRITE]);
	if (!WB_PATCH_ACTIVE(WB_PATCH_WRITE))
	{
		LONG res = WB_CALL_OLD_3(LONG, WB_PATCH_WRITE, d1, file, d2, wdata, d3, length);
		atomic_dec(&usecount[WB_PATCH_WRITE]);
		return res;
	}
	{
		struct LibData *data;
		struct MyLibrary *libbase;
		struct FileHandleWrapper *handle;
		LONG res;

		// Get library
		if (!file || !(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_WRITE]);
			return 0;
		}

		// Get data pointer
		data = (struct LibData *)libbase->ml_UserData;

		// Different to the last file we saw?
		if (data->last_file != file)
		{
			// See if we have a wrapper
			if ((handle = find_filehandle(file, data)))
			{
				// Set 'written' flag
				handle->fhw_Flags |= FHWF_WRITTEN;

				// Unlock the list
				ReleaseSemaphore(&data->file_list.lock);
			}

			// Remember this file
			data->last_file = file;
		}

		// Write data
		res = WB_CALL_OLD_3(LONG, WB_PATCH_WRITE, d1, file, d2, wdata, d3, length);

		atomic_dec(&usecount[WB_PATCH_WRITE]);
		return res;
	}
}
PATCH_END

// Calls original Write directly
LONG LIBFUNC L_OriginalWrite(REG(d1, BPTR file),
							 REG(d2, void *wdata),
							 REG(d3, LONG length),
							 REG(a6, struct MyLibrary *libbase))
{
	// Without our patch running there is nothing to bypass
	if (!WB_PATCH_ACTIVE(WB_PATCH_WRITE))
		return Write(file, wdata, length);

	// Write data
	return WB_CALL_OLD_3(LONG, WB_PATCH_WRITE, d1, file, d2, wdata, d3, length);
}

/******************************************************************************/

// Get information on a filelock.
//
// Allocates a FileInfoBlock + 512-byte path scratch buffer from the
// dos_patch_memory pool (LibData.dos_patch_memory).  Caller frees with
// L_FreeMemH() once it's done sending notifications.  L_AllocMemH falls back
// to a plain AllocMem if the pool wasn't created (e.g., low-memory libinit).
struct FileInfoBlock *dospatch_fib(BPTR lock, struct MyLibrary *libbase, BOOL req)
{
	struct FileInfoBlock *fib = NULL;
	BPTR parent;
	BOOL ok = FALSE;
	APTR wsave = (APTR)-1;
	struct Process *task = NULL;

	// Find process
	if (req && (task = (struct Process *)FindTask(NULL)))
	{
		// Is it a process?
		if (task->pr_Task.tc_Node.ln_Type == NT_PROCESS)
		{
			// Save pointer and turn requesters off
			wsave = task->pr_WindowPtr;
			task->pr_WindowPtr = (APTR)-1;
		}
		else
			task = NULL;
	}

	// Get parent directory
	if ((parent = ParentDir(lock)))
	{
		// Get FileInfoBlock and a buffer
		if ((fib = L_AllocMemH(data->dos_patch_memory, sizeof(struct FileInfoBlock) + 512)))
		{
			char *buf;

			// Get buffer pointer
			buf = (char *)(fib + 1);

			// Get full path of parent, and terminate it
			L_DevNameFromLockDopus(parent, buf, 512, libbase);
			AddPart(buf, "", 512);

			// Got path?
			if (*buf)
			{
				// Examine the lock
#ifdef USE_64BIT
				if (L_ExamineLock64(lock, (FileInfoBlock64 *)fib))
#else
				if (Examine(lock, fib))
#endif
					ok = TRUE;
			}
		}

		// Not ok?
		if (!ok)
		{
			L_FreeMemH(fib);
			fib = NULL;
		}

		// Unlock parent
		UnLock(parent);
	}

	// Fix requesters
	if (task)
		task->pr_WindowPtr = wsave;
	return fib;
}

// Find FileHandle in tracking list
struct FileHandleWrapper *find_filehandle(BPTR file, struct LibData *data)
{
	struct FileHandleWrapper *handle;

	// Lock list
	ObtainSemaphore(&data->file_list.lock);

	// Go through list
	for (handle = (struct FileHandleWrapper *)data->file_list.list.lh_Head; handle->fhw_Node.mln_Succ;
		 handle = (struct FileHandleWrapper *)handle->fhw_Node.mln_Succ)
	{
		// Match handle
		if (handle->fhw_FileHandle == file)
		{
			// Not the top one?
			if (handle != (struct FileHandleWrapper *)data->file_list.list.lh_Head)
			{
				// Shift it to the top as the most recently accessed
				Remove((struct Node *)handle);
				AddHead(&data->file_list.list, (struct Node *)handle);
			}

			// Return the handle (leave the list locked)
			return handle;
		}
	}

	// Not found
	ReleaseSemaphore(&data->file_list.lock);
	return 0;
}
