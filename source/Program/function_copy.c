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

#include <proto/timer.h>

#include "dopus.h"

#define COPY_INITIAL_BUFFER 8192
#define COPY_MIN_BUFFER 2048
#define COPY_LOW_THRESH 500000
#define COPY_HIGH_THRESH 1000000

enum {
	COPY_NAME,
	COPY_TO,
	COPY_QUIET,
	COPY_UPDATEC,
	COPY_MOVESAME,
	COPY_NEWER,
};

enum {
	COPYAS_NAME,
	COPYAS_NEWNAME,
	COPYAS_TO,
	COPYAS_QUIET,
	COPYAS_MOVESAME,
};

enum {
	ENCRYPT_NAME,
	ENCRYPT_TO,
	ENCRYPT_PASSWORD,
	ENCRYPT_QUIET,
};

enum {
	CLONE_NAME,
	CLONE_NEWNAME,
	CLONE_QUIET,
};

#define COPY_TOP_LEVEL (1 << 14)
#define ENCRYPT_DECRYPT (1 << 15)
#define COPY_UPDATE_COPY (1 << 16)
#define COPY_NEWER_COPY (1 << 17)

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct
{
	short valid;
	short confirm_each;

	union
	{
		struct
		{
			char new_name_edit[256];
			char old_name_edit[256];
			BOOL wild_copy;
		} copy;

		struct
		{
			char password[24];
			short decrypt;
		} encrypt;

		struct
		{
			short hard;
		} makelink;
	} func;
} CopyData;

#ifndef __amigaos3__
	#pragma pack()
#endif

void copy_icon_position(FunctionHandle *handle, char *name, struct DiskObject *icon);

// COPY, etc internal function
DOPUS_FUNC(function_copy)
{
	FunctionEntry *entry;
	short ret = 1, dest_count = 0, function, count;
	PathNode *path, *source;
	char *source_file, *dest_file, *dest_name, *source_no_icon;
	char *old_name = 0, *old_name_edit, *new_name, *new_name_edit;
	char *password = 0, *password_buf;
	BOOL move_flag = 0, rename_flag = 0, link_flag = 0, no_move_rename = 0, source_same = 0, dragdrop = 0;
	BOOL change_info = 0;
	ULONG copy_flags, side;
#ifdef USE_64BIT
	UQUAD rec_size = 0;
#else
	ULONG rec_size = 0;
#endif
	CopyData *data;
	struct DateStamp top_date = {0, 0, 0};
	char top_name[256] = {0};
	char filepath[512] = {0};

	// Get function
	function = command->function;
	copy_flags = environment->env->settings.copy_flags;
	if (function == FUNC_CLONE)
		copy_flags &= ~COPY_DATE;

	// Icon copy?
	if (environment->env->desktop_flags & DESKTOPF_SMART_ICONCOPY)
		copy_flags |= COPYF_ICON_COPY;

	// Set flags to allow recursive directories (except for makelink)
	if (function != FUNC_MAKELINK && function != FUNC_MAKELINKAS)
		handle->instruction_flags = INSTF_RECURSE_DIRS | INSTF_WANT_DIRS | INSTF_WANT_DIRS_END;

	// Making links
	else
	{
		handle->instruction_flags = 0;
		link_flag = 1;
	}

	// If moving, clear sizes
	if (function == FUNC_MOVE || function == FUNC_MOVEAS)
	{
		handle->instruction_flags |= INSTF_DIR_CLEAR_SIZES;
		move_flag = 1;
	}

	// Get source path
	source = function_path_current(&handle->source_paths);

	// From icon drag & drop?
	if (handle->flags & FUNCF_DRAG_DROP && handle->flags & FUNCF_ICONS && !(handle->flags & FUNCF_COPY_NO_MOVE))
	{
		// Set flag
		dragdrop = 1;

		// See if paths are the same device
		if (function_check_same_path(handle->source_path, handle->dest_path) == LOCK_SAME_VOLUME)
		{
			// Change operation to move
			handle->instruction_flags |= INSTF_DIR_CLEAR_SIZES;
			move_flag = 1;
		}

		// Icon copy
		copy_flags |= COPYF_ICON_COPY;
	}

	// Copy or CopyAs, with Move flag set?
	else if ((function == FUNC_COPY && instruction->funcargs && instruction->funcargs->FA_Arguments[COPY_MOVESAME]) ||
			 (function == FUNC_COPYAS && instruction->funcargs && instruction->funcargs->FA_Arguments[COPYAS_MOVESAME]))
	{
		// See if paths are the same device
		if (function_check_same_path(handle->source_path, handle->dest_path) == LOCK_SAME_VOLUME)
		{
			// Change operation to move
			handle->instruction_flags |= INSTF_DIR_CLEAR_SIZES;
			move_flag = 1;
		}
	}

	// Go through destination paths
	while ((path = function_path_next(&handle->dest_paths)))
	{
		short invalid = 0, ret;

		// See if the paths are the same
		if ((ret = function_check_same_path(handle->source_path, path->path)) == LOCK_SAME)
		{
			// Mark as the same
			path->flags |= LISTNF_SAME;

			// If not clone or *as, we can't do this
			if (function != FUNC_CLONE && function != FUNC_MOVEAS && function != FUNC_COPYAS &&
				function != FUNC_MAKELINKAS)
			{
				// Don't show requester for drag & drop
				if (!dragdrop)
				{
					// Put up error requester
					function_request(
						handle,
						GetString(&locale,
								  (function == FUNC_MAKELINK) ? MSG_CANT_OVERLINK_FILES : MSG_CANT_OVERCOPY_FILES),
						0,
						(IPTR)GetString(&locale, MSG_CONTINUE),
						0);
				}

				// Flag this lister as invalid
				invalid = 1;
			}

			// Otherwise, set flag
			else
				source_same = 1;
		}

		// If valid
		if (!invalid)
		{
			// Tell this lister to update it's datestamp at the end
			path->flags |= LISTNF_UPDATE_STAMP;

			// Increment count
			++dest_count;
		}

		// Done with this path
		function_path_end(handle, &handle->dest_paths, invalid);
	}

	// If we don't have any destinations, return
	if (function != FUNC_CLONE && dest_count == 0)
	{
		// DisplayBeep for drag & drop
		if (dragdrop)
			DisplayBeep(GUI->screen_pointer);
		return 0;
	}

	// Turn progress indicator on
	function_progress_on(
		handle,
		GetString(&locale,
				  (function == FUNC_ENCRYPT)
					  ? MSG_PROGRESS_OPERATION_ENCRYPTING
					  : ((link_flag) ? MSG_PROGRESS_OPERATION_LINKING
									 : ((move_flag) ? MSG_PROGRESS_OPERATION_MOVING
													: ((function == FUNC_CLONE) ? MSG_PROGRESS_OPERATION_CLONING
																				: MSG_PROGRESS_OPERATION_COPYING)))),
		handle->entry_count,
		PWF_FILENAME | PWF_FILESIZE | PWF_INFO | PWF_GRAPH);

	// Allocate memory for strings
	if (!(source_file = AllocVec(1280, MEMF_CLEAR)))
		return 0;
	dest_file = source_file + 256;
	dest_name = dest_file + 256;
	new_name = dest_name + 256;
	source_no_icon = new_name + 256;

	// Get data pointers
	data = (CopyData *)handle->inst_data;
	new_name_edit = data->func.copy.new_name_edit;
	old_name_edit = data->func.copy.old_name_edit;
	password_buf = data->func.encrypt.password;

	// First time through?
	if (!data->valid)
	{
		data->confirm_each = 0;
		data->valid = 1;
	}

	// Args?
	if (instruction->funcargs)
	{
		short arg = 0;

		// Check for quiet
		if (function == FUNC_CLONE)
			arg = CLONE_QUIET;
		else if (function == FUNC_COPY || function == FUNC_MOVE || function == FUNC_MAKELINK)
			arg = COPY_QUIET;
		else if (function == FUNC_COPYAS || function == FUNC_MOVEAS || function == FUNC_MAKELINKAS)
			arg = COPYAS_QUIET;
		else if (function == FUNC_ENCRYPT)
			arg = ENCRYPT_QUIET;

		// Quiet?
		if (instruction->funcargs->FA_Arguments[arg])
			data->confirm_each = COPYF_DELETE_ALL | COPYF_UNPROTECT_ALL;

		// Copy?
		if (function == FUNC_COPY)
		{
			// Update?
			if (instruction->funcargs->FA_Arguments[COPY_UPDATEC])
				copy_flags |= COPY_UPDATE_COPY;

			// Newer?
			else if (instruction->funcargs->FA_Arguments[COPY_NEWER])
				copy_flags |= COPY_NEWER_COPY;
		}
	}

	// Source=destination or Move?
	if (move_flag || source_same)
	{
		// Tell source to update its datestamp
		source->flags |= LISTNF_UPDATE_STAMP;
	}

	// Initialise file count
	count = 1;

	// Moving?
	if (move_flag)
	{
		// Initialise progress bar for move
		function_progress_file(handle, 2, 0);
		function_progress_file(handle, 0, 2);

		// See if source is the desktop
		if (source && HookMatchDesktop(source->path_buf))
			handle->flags |= FUNCF_RESCAN_DESKTOP;
	}

	// Build info string
	function_build_info(handle, 0, 0, 3);

	// Go through entries
	while ((entry = function_get_entry(handle)))
	{
		BOOL file_ok = 1;

		// Is source a device (in drag'n'drop operation)?
		if (!(entry->flags & FUNCENTF_EXITED) && entry->name[strlen(entry->name) - 1] == ':' && dragdrop)
		{
			BPTR lock;

			// See if source is the same device
			if (move_flag && function_check_same_path(entry->name, handle->dest_path) == LOCK_SAME_VOLUME)
			{
				// Skip over this entry
				DisplayBeep(GUI->screen_pointer);
				count += function_end_entry(handle, entry, 0);
				continue;
			}

			// Lock and examine source path
			if ((lock = Lock(entry->name, ACCESS_READ)))
			{
				struct DiskObject *icon;

				// Examine
#ifdef USE_64BIT
				ExamineLock64(lock, (FileInfoBlock64 *)handle->d_info);
#else
				Examine(lock, handle->d_info);
#endif

				// Get device name
				DevNameFromLockDopus(lock, handle->work_buffer + 512, 256);
				UnLock(lock);

				// Get disk icon
				lsprintf(handle->work_buffer, "%sDisk", entry->name);
				if (!(icon = GetDiskObject(handle->work_buffer)))
				{
					// Use default drawer icon
					icon = GetDefDiskObject(WBDRAWER);
				}

				// Got icon?
				if (icon)
				{
					// Change icon type
					icon->do_Type = WBDRAWER;

					// Try and get position
					copy_icon_position(handle, handle->work_buffer + 512, icon);
				}

				// Get destination path
				if ((path = function_path_current(&handle->dest_paths)))
				{
					// Build destination path
					strcpy(handle->work_buffer, path->path);
					AddPart(handle->work_buffer, handle->d_info->fib_FileName, 512);

					// Create directory
					if ((lock = OriginalCreateDir(handle->work_buffer)))
					{
						// Examine this directory
#ifdef USE_64BIT
						ExamineLock64(lock, (FileInfoBlock64 *)handle->s_info);
#else
						Examine(lock, handle->s_info);
#endif

						// Unlock directory
						UnLock(lock);

						// Write icon
						if (icon)
						{
							// Write icon
							if (PutDiskObject(handle->work_buffer, icon))
							{
								// Add to listers
								function_filechange_loadfile(
									handle, path->path, handle->d_info->fib_FileName, FFLF_ICON);
							}
						}

						// Add new directory
						function_filechange_addfile(handle, path->path, handle->s_info, 0, 0);
					}

					// Fix path node
					strcpy(path->path_buf, path->path);
					AddPart(path->path_buf, handle->d_info->fib_FileName, 512);
					path->flags |= LISTNF_CHANGED;
				}

				// Free icon
				FreeDiskObject(icon);
			}
		}

		// Update progress indicator
		if (function_progress_update(handle, entry, count))
		{
			function_abort(handle);
			ret = 0;
			break;
		}

		// Build source name
		function_build_source(handle, entry, source_file);

		// Get destination filename
		strcpy(dest_name, entry->name);

		// Check this isn't an exited directory
		if (!(entry->flags & FUNCENTF_EXITED))
		{
			// Copy as/Move as/MakeLink as/Clone?
			if (function == FUNC_COPYAS || function == FUNC_MOVEAS || function == FUNC_MAKELINKAS ||
				function == FUNC_CLONE)
			{
				// Not recurse entry?
				if (!(entry->flags & FUNCENTF_RECURSE))
				{
					// An icon entry?
					if (entry->flags & FUNCENTF_ICON)
					{
						// Get new name, tack a .info to the end
						strcpy(dest_name, new_name);
						strcat(dest_name, ".info");
					}

					// Normal entry
					else
					{
						// Do we have to ask for a name?
						if (!data->func.copy.wild_copy)
						{
							// Name supplied in arguments?
							if (instruction->funcargs && instruction->funcargs->FA_Arguments[COPYAS_NAME] &&
								instruction->funcargs->FA_Arguments[COPYAS_NEWNAME])
							{
								// Get names
								strcpy(old_name_edit, (char *)instruction->funcargs->FA_Arguments[COPYAS_NAME]);
								strcpy(new_name_edit, (char *)instruction->funcargs->FA_Arguments[COPYAS_NEWNAME]);
								ret = 1;
							}

							// Otherwise
							else
							{
								// Ask for a new name
								strcpy(old_name_edit, entry->name);
								strcpy(new_name_edit, entry->name);
								if ((ret = function_request(
										 handle,
										 GetString(&locale, MSG_ENTER_NEW_NAME),
										 SRF_BUFFER | SRF_BUFFER2 | SRF_PATH_FILTER,
										 (IPTR)new_name_edit,
										 GUI->def_filename_length,
										 (IPTR)old_name_edit,
										 GUI->def_filename_length,
										 (IPTR)GetString(&locale,
														  (move_flag)
															  ? MSG_MOVE
															  : ((link_flag) ? MSG_MAKELINK
																			 : ((function == FUNC_CLONE) ? MSG_DUPLICATE
																										 : MSG_COPY))),
										 (IPTR)GetString(&locale, MSG_ABORT),
										 (IPTR)GetString(&locale, MSG_SKIP),
										 0)) == 2)
								{
									function_abort(handle);
									ret = 0;
									break;
								}

								// Turn -1 into 1
								else if (ret == -1)
									ret = 1;
							}

							// If name is the same (for clone), skip
							if (function == FUNC_CLONE && stricmp(old_name_edit, new_name_edit) == 0)
							{
								ret = 0;
							}

							// Skip?
							if (ret == 0)
								file_ok = 0;

							// Otherwise
							else
							{
								// Does the new name contain wildcards?
								if (strchr(new_name_edit, '*'))
								{
									// If the old name doesn't, make it a *
									if (!(strchr(old_name_edit, '*')))
										strcpy(old_name_edit, "*");

									// Set wildcard flag
									data->func.copy.wild_copy = 1;
								}

								// No wildcards
								else
								{
									// Store new name
									strcpy(dest_name, new_name_edit);
								}
							}
						}

						// Wildcard copy?
						if (data->func.copy.wild_copy)
						{
							// Get new name
							file_ok = rename_get_wild(old_name_edit, new_name_edit, entry->name, dest_name);
						}

						// Still ok?
						if (file_ok)
						{
							// Get old and new names
							old_name = entry->name;
							strcpy(new_name, dest_name);
						}
					}
				}

				// Otherwise, make change to first part of destination
				else if (old_name)
					strreplace(dest_name, old_name, new_name, 0);
			}

			// Encrypt?
			else if (function == FUNC_ENCRYPT)
			{
				// Have to ask for a password?
				if (!password)
				{
					// Password supplied in arguments?
					if (instruction->funcargs && instruction->funcargs->FA_Arguments[ENCRYPT_PASSWORD])
					{
						// Copy password
						stccpy(password_buf, (char *)instruction->funcargs->FA_Arguments[ENCRYPT_PASSWORD], 24);
					}

					// Ask for a password
					else if (!(function_request(handle,
												GetString(&locale, MSG_ENTER_PASSWORD),
												SRF_BUFFER | SRF_CHECKMARK,
												(IPTR)password_buf,
												24,
												(IPTR)GetString(&locale, MSG_DECRYPT),
												(IPTR)&data->func.encrypt.decrypt,
												(IPTR)GetString(&locale, MSG_OKAY),
												(IPTR)GetString(&locale, MSG_ABORT),
												0)) ||
							 !password_buf[0])
					{
						function_abort(handle);
						ret = 0;
						break;
					}

					// Get password pointer
					password = password_buf;

					// Decrypt?
					if (data->func.encrypt.decrypt)
						copy_flags |= ENCRYPT_DECRYPT;
					else
						copy_flags &= ~ENCRYPT_DECRYPT;
				}
			}
		}

		// Exited directory
		else
		{
			// Exited a device (in drag'n'drop operation)?
			if (entry->name[strlen(entry->name) - 1] == ':' && dragdrop)
			{
				// Get destination path
				if ((path = function_path_current(&handle->dest_paths)))
				{
					// Clear path flag
					path->flags &= ~LISTNF_CHANGED;
				}
			}
		}

		// Top-level entry?
		if (entry->flags & FUNCENTF_TOP_LEVEL)
		{
			// Exited directory?
			if (entry->flags & FUNCENTF_EXITED)
			{
				if ((copy_flags & COPY_DATE) && (top_date.ds_Days != 0))
				{
					// Get top level destination path
					strcpy(filepath, handle->dest_path);
					// Add destination directory name to path
					AddPart(filepath, top_name, sizeof(filepath));

					// Set the directory date
					SetFileDate((CONST_STRPTR)filepath, &top_date);

					// Update directory date in destination lister
					function_filechange_modify(handle, handle->dest_path, top_name, FM_Date, &top_date, TAG_DONE);

					top_date.ds_Days = 0;
				}

				// Add size update
				function_filechange_modify(handle,
										   source->path,
										   entry->name,
#ifdef USE_64BIT
										   FM_Size64,
										   &rec_size,
#else
										   FM_Size,
										   rec_size,
#endif
										   TAG_END);
				rec_size = 0;
			}
			else if (entry->type > 0)
			{
				BPTR lock = 0;
				D_S(struct FileInfoBlock, fib)
				if ((lock = Lock(source_file, ACCESS_READ)))
				{
					if (Examine(lock, fib))
					{
						top_date.ds_Days = fib->fib_Date.ds_Days;
						top_date.ds_Minute = fib->fib_Date.ds_Minute;
						top_date.ds_Tick = fib->fib_Date.ds_Tick;
					}
					UnLock(lock);
				}
				strcpy(top_name, dest_name);
			}
		}

		// File within a directory?
		else if (entry->type < 0)
		{
			// Increment size count
			rec_size += entry->size;
		}

		// Ok to do this file?
		if (file_ok)
		{
			// If clone, use source path
			if (function == FUNC_CLONE)
				path = source;

			// Otherwise, get destination path
			else
				path = function_path_next(&handle->dest_paths);

			// Go through destination paths
			while (path)
			{
				// Get destination path
				strcpy(handle->dest_path, path->path);
				strcpy(dest_file, handle->dest_path);
				AddPart(dest_file, dest_name, 256);

				// If there's multiple destinations, or flagged, change info
				if (dest_count > 1 || change_info)
				{
					char *file, ch = 0;

					// Clear filename
					if ((file = (STRPTR)FilePart(source_file)))
					{
						ch = *file;
						*file = 0;
					}

					// Build info string
					function_build_info(handle, source_file, handle->dest_path, 3);
					change_info = 0;

					// Restore source filename
					if (file)
						*file = ch;
				}

				// Check this isn't an exited directory
				if (!(entry->flags & FUNCENTF_EXITED))
				{
					BOOL ok = 1;

					// Is this a directory?
					if (entry->type > 0)
					{
						// Set flag to change info
						change_info = 1;

						// Is this a top-level directory?
						if (entry->flags & FUNCENTF_TOP_LEVEL)
						{
							short len;

							// Check we're not trying to copy it into itself
							if (strnicmp(handle->dest_path, source_file, (len = strlen(source_file))) == 0 &&
								(handle->dest_path[len] == 0 || handle->dest_path[len] == '/' ||
								 handle->dest_path[len] == ':'))
							{
								// Put up error requester
								if (!(function_request(handle,
													   GetString(&locale,
																 (link_flag) ? MSG_CANT_LINK_DIR_INTO_ITSELF
																			 : MSG_CANT_COPY_DIR_INTO_ITSELF),
													   0,
													   (IPTR)GetString(&locale, MSG_CONTINUE),
													   (IPTR)GetString(&locale, MSG_ABORT),
													   0)))
									ret = -1;
								else
									ret = 0;
								ok = 0;
							}
						}
					}

					// Update/Newer?
					else if (copy_flags & (COPY_UPDATE_COPY | COPY_NEWER_COPY))
					{
						D_S(struct FileInfoBlock, fib)

						// See if destination exists
						if (GetFileInfo(dest_file, fib))
						{
							// If copying newer, check date
							if (copy_flags & COPY_NEWER_COPY)
							{
								D_S(struct FileInfoBlock, fib2)

								// Get source info
								if (GetFileInfo(source_file, fib2))
								{
									// Compare file dates
									if (CompareDates(&fib2->fib_Date, &fib->fib_Date) >= 0)
									{
										// Don't copy
										ok = 0;
										ret = 0;
									}
								}
							}

							// If copy update, don't copy at all if file exists
							else
							{
								// Clear ok flag so file won't be copied
								ok = 0;
								ret = 0;
							}
						}
					}

					// Check destination is ok to write to
					if (ok && (ret = check_file_destination(handle, entry, dest_file, &data->confirm_each)) == 1)
					{
						long error = 0;

						// If we're moving, try rename, unless this is an icon
						if (move_flag && (!(entry->flags & FUNCENTF_ICON) || !(copy_flags & COPYF_ICON_COPY)))
						{
							// Can only rename if a file, or no recursive filter set
							if (!no_move_rename && (entry->type < 0 || handle->got_filter != 2))
							{
								// Try rename
								if ((error = OriginalRename(source_file, dest_file)))
								{
									BPTR lock;
									if ((lock = Lock(dest_file, ACCESS_READ)))
									{
#ifdef USE_64BIT
										ExamineLock64(lock, (FileInfoBlock64 *)handle->d_info);
#else
										Examine(lock, handle->d_info);
#endif
										UnLock(lock);
									}
								}

								// Different devices?
								else if (IoErr() == ERROR_RENAME_ACROSS_DEVICES)
									no_move_rename = 1;
							}
						}

						// Or linking?
						else if (link_flag)
						{
							BPTR lock = 0;

							// Loop while unsuccessful
							while (1)
							{
								// Clear error code
								error = 0;

								// No source lock?
								if (!lock)
								{
									// Lock source
									if (!(lock = Lock(source_file, ACCESS_READ)))
										error = IoErr();
								}

								// Try to make link
								if (lock && !(MakeLink(dest_file, (SIPTR)lock, FALSE)))
									error = IoErr();

								// Successful?
								if (!error)
								{
									BPTR dest;

									// Lock new destination
									if ((dest = Lock(dest_file, ACCESS_READ)))
									{
										// Examine it
#ifdef USE_64BIT
										ExamineLock64(lock, (FileInfoBlock64 *)handle->d_info);
#else
										Examine(lock, handle->d_info);
#endif
										UnLock(dest);

										// Fix type to indicate link
										if (handle->d_info->fib_DirEntryType < 0)
											handle->d_info->fib_DirEntryType = ST_LINKFILE;
										else
											handle->d_info->fib_DirEntryType = ST_LINKDIR;

										// Use new filename
										strcpy(handle->d_info->fib_FileName, FilePart(dest_file));
										ret = 1;
									}
									break;
								}

								// Failed
								else
								{
									// Show error
									if (!(ret = function_error(handle, entry->name, MSG_LINKING, IoErr())) || ret == -1)
										break;
								}
							}

							// Unlock lock if we had one
							UnLock(lock);
							error = 0;
						}

						// Was rename successful?
						if (error)
						{
							ret = 1;

							// Did we move a whole directory tree?
							if (entry->flags & FUNCENTF_ENTERED)
							{
								entry->flags |= FUNCENTF_REMOVE;
								rename_flag = 1;
							}
						}

						// Try to copy the file
						else if (!link_flag)
						{
							// Top-level entry?
							if (entry->flags & FUNCENTF_TOP_LEVEL)
								copy_flags |= COPY_TOP_LEVEL;
							else
								copy_flags &= ~COPY_TOP_LEVEL;

							// Try to copy file
							while (!(ret = function_copy_file(handle->s_info,
															  handle->d_info,
															  handle,
															  source_file,
															  dest_file,
															  &error,
															  password,
															  copy_flags)))
							{
								// Not found?
								if (error == ERROR_OBJECT_NOT_FOUND)
								{
									// If this is an icon, just ignore it
									if (entry->flags & FUNCENTF_ICON)
									{
										ret = -2;  // 0
										break;
									}

									// Icon operation?
									if (handle->flags & FUNCF_ICONS && !(isicon(source_file)))
									{
										BPTR lock;
										char *ptr;

										// See if file has an icon on disk
										strcat(source_file, ".info");
										if ((lock = Lock(source_file, ACCESS_READ)))
											UnLock(lock);

										// Restore file name
										if ((ptr = isicon(source_file)))
											*ptr = 0;

										// If icon exists, skip over main file
										if (lock)
										{
											ret = -2;
											break;
										}
									}
								}

								// Display error
								if (!(ret = function_error(handle, entry->name, MSG_COPYING, error)) || ret == -1)
									break;
							}
						}
					}

					// Aborted?
					if (ret == -1)
						break;

					// Success?
					if (ret > 0)
					{
						// If a top-level entry, add to destination list
						if (entry->flags & FUNCENTF_TOP_LEVEL)
						{
							// Add file
							function_filechange_addfile(handle, path->path, handle->d_info, 0, 0);
						}

						// Update free space?
						if (copy_flags & COPY_UPDATE)
						{
							// Valid lister?
							if (path->lister && path->lister->window)
							{
								side = (path->flags & LISTNF_DUAL_SIDE) ? path->dual_side + 1 : 0;

								// Send update message
								IPC_Command(path->lister->ipc, LISTER_REFRESH_NAME, side, 0, 0, 0);
							}
						}

						// Update archive bit status in lister entry?
						if (copy_flags & COPY_ARC && !(entry->flags & FUNCENTF_ICON_ACTION) && entry->entry)
						{
							// Get new protection bits
							entry->entry->de_Protection = handle->s_info->fib_Protection;
							protect_get_string(entry->entry->de_Protection, entry->entry->de_ProtBuf);
						}
					}

					// Skipped over file, still want to do icon?
					else if (ret == -2)
						ret = 0;

					// File has failed (as far as deselecting goes)
					else
						file_ok = 0;
				}

				// Exited directory, set flag to change info
				else
					change_info = 1;

				if ((entry->type == ENTRY_DIRECTORY) && (top_date.ds_Days != 0))
				{
					if (entry->flags & FUNCENTF_EXITED)
					{
						if (copy_flags & COPY_DATE)
						{
							char *file = NULL;
							// Get top level destination path
							strcpy(filepath, handle->dest_path);
							// Add top destination directory name to path
							AddPart(filepath, top_name, sizeof(filepath));

							// Add remainder of destination path
							file = strchr(entry->name, '/');
							strcat(filepath, file);

							// Set the directory date
							SetFileDate((CONST_STRPTR)filepath, &handle->recurse_info.fib_Date);
						}
					}
				}

				// If clone, break out
				if (function == FUNC_CLONE)
					break;

				// Done with this path, get next
				function_path_end(handle, &handle->dest_paths, 0);
				path = function_path_next(&handle->dest_paths);
			}

			// Aborted?
			if (ret == -1)
			{
				function_abort(handle);
				ret = 0;
				break;
			}
		}

		// Need to delete source?
		if (move_flag && file_ok && !rename_flag && (entry->type < 0 || entry->flags & FUNCENTF_EXITED))
		{
			BOOL icon = 0;
			char *ptr;
			short suc = 0;

			// Copy name
			strcpy(source_no_icon, source_file);

			// See if file is an icon
			if ((ptr = isicon(source_no_icon)))
			{
				// Strip .info
				*ptr = 0;
				icon = 1;
			}

			// Set result code
			ret = 1;

			// Loop while unsuccessful
			while (!suc)
			{
				short err_code;

				// Delete file (or icon)
				if (icon)
					suc = DeleteDiskObject(source_no_icon);
				else
					suc = DeleteFile(source_file);

				// Successful?
				if (suc)
					break;

				// Get error code
				err_code = IoErr();

				// If object not found, it's already deleted
				if (err_code == ERROR_OBJECT_NOT_FOUND)
					break;

				// Delete protected
				else if (err_code == ERROR_DELETE_PROTECTED)
				{
					// Ask before unprotecting?
					if (!(handle->instruction_flags & INSTF_DELETE_UNPROTECT))
					{
						// Build requester text
						lsprintf(handle->work_buffer, GetString(&locale, MSG_DELETE_PROTECTED), FilePart(source_file));

						// Display request
						if (!(ret = function_request(handle,
													 handle->work_buffer,
													 0,
													 (IPTR)GetString(&locale, MSG_UNPROTECT),
													 (IPTR)GetString(&locale, MSG_UNPROTECT_ALL),
													 (IPTR)GetString(&locale, MSG_ABORT),
													 (IPTR)GetString(&locale, MSG_SKIP),
													 0)))
							break;

						// Abort?
						if (ret == 3)
						{
							ret = -1;
							break;
						}

						// Unprotect all?
						if (ret == 2)
						{
							handle->instruction_flags |= INSTF_DELETE_UNPROTECT;
							ret = 1;
						}
					}

					// Try to unprotect file
					if (!(SetProtection(source_file, 0)))
					{
						ret = 0;
						break;
					}
				}

				// Another error
				else
				{
					// Directory not empty with recursive filter on
					if (err_code == ERROR_DIRECTORY_NOT_EMPTY && handle->got_filter == 2)
					{
						// Skip
						ret = 0;
						break;
					}

					// Otherwise
					else if (!(ret = function_error(handle, entry->name, MSG_DELETING, err_code)) || ret == -1)
						break;
				}
			}

			// Successful?
			if (ret == 1)
				entry->flags |= FUNCENTF_REMOVE;
			else
				file_ok = 0;
		}

		// Aborted?
		if (ret == -1)
		{
			function_abort(handle);
			ret = 0;
			break;
		}

		// Get next entry, increment count
		count += function_end_entry(handle, entry, file_ok);

		// Reset result code
		ret = 1;
		rename_flag = 0;
	}

	// Free data
	FreeVec(source_file);

	return ret;
}

// Copy a file, with optional encryption
int function_copy_file(struct FileInfoBlock *s_info,
					   struct FileInfoBlock *d_info,
					   FunctionHandle *handle,
					   char *source_file,
					   char *dest_file,
					   long *err_code,
					   char *password,
					   ULONG copy_flags)
{
	BPTR lock;
	BPTR in_file, out_file = 0;
	char *file_buffer, *initial_buffer;
	unsigned long buffer_size;
#ifdef USE_64BIT
	UQUAD file_size = 0LL;
	UQUAD total_size = 0LL;
#else
	long file_size = 0;
	long total_size = 0;
#endif
	short ret_code = COPY_FAILED;
	char decrypt_flag = 0;
	char got_dest_info = 0;
	char *source_ptr, *dest_ptr;
	BOOL done = 0;

	// Initialise error code
	*err_code = 0;

	// Lock and examine source
#ifdef USE_64BIT
	if (!(lock = Lock(source_file, ACCESS_READ)) || !(ExamineLock64(lock, (FileInfoBlock64 *)s_info)))
#else
	if (!(lock = Lock(source_file, ACCESS_READ)) || !(Examine(lock, s_info)))
#endif
	{
		*err_code = IoErr();
		UnLock(lock);
		return COPY_FAILED;
	}

	// Unlock lock
	UnLock(lock);

	// Is a password specified?
	if (password)
	{
		int a, seed = 1;

		// Decrypting?
		if (password[0] == '-')
		{
			++password;
			decrypt_flag = 1;
		}
		else if (copy_flags & ENCRYPT_DECRYPT)
			decrypt_flag = 1;

		// Go through password, build seed
		for (a = 0; password[a]; a++)
			seed *= password[a];

		// Seed random number generator
		Seed(seed);
	}

	// Copying an icon?
	else if (copy_flags & COPYF_ICON_COPY && (source_ptr = isicon(source_file)) && (dest_ptr = isicon(dest_file)))
	{
		struct DiskObject *icon;

		// Strip .info suffixes
		*source_ptr = 0;
		*dest_ptr = 0;

		// Set file size
		function_progress_file(handle, 10, 0);

		// Try to get icon
		if ((icon = GetDiskObject(source_file)))
		{
			// Update progress
			function_progress_file(handle, 0, 5);

			// Adjust position for top-level files
			if (copy_flags & COPY_TOP_LEVEL)
				copy_icon_position(handle, (STRPTR)FilePart(source_file), icon);

			// Check abort
			if (function_check_abort(handle))
				ret_code = COPY_ABORTED;

			// Try to write icon
			else if (PutDiskObject(dest_file, icon))
			{
				function_progress_file(handle, 0, 10);
				ret_code = COPY_OK;
			}

			// Get error
			else
				*err_code = IoErr();

			// Free icon
			FreeDiskObject(icon);
		}

		// Restore .info's
		*source_ptr = '.';
		*dest_ptr = '.';

		// Did we have an icon?
		if (icon)
		{
			// Success?
			if (ret_code == COPY_OK)
			{
				// Lock destination file
				if ((lock = Lock(dest_file, ACCESS_READ)))
				{
					// Examine file
#ifdef USE_64BIT
					if (ExamineLock64(lock, (FileInfoBlock64 *)d_info))
#else
					if (Examine(lock, d_info))
#endif
						got_dest_info = 1;
					UnLock(lock);
				}
			}

			// Mark as done
			done = 1;
		}
	}

	// Not copied yet?
	if (!done)
	{
		// Is it a directory?
		if (s_info->fib_DirEntryType > 0)
		{
			// Try to create destination directory
			if ((lock = Lock(dest_file, ACCESS_READ)) || (lock = CreateDir(dest_file)))
			{
				// Get directory information
#ifdef USE_64BIT
				ExamineLock64(lock, (FileInfoBlock64 *)d_info);
#else
				Examine(lock, d_info);
#endif
				UnLock(lock);

				// Is destination a directory?
				if (d_info->fib_DirEntryType > 0)
				{
					// Copy protection?
					if (copy_flags & COPY_PROT)
					{
						ULONG prot = s_info->fib_Protection & ~FIBF_ARCHIVE;
						if (SetProtection(dest_file, prot))
							d_info->fib_Protection = prot;
					}

					// Copy comment
					if (copy_flags & COPY_NOTE)
					{
						if (SetComment(dest_file, s_info->fib_Comment))
							strcpy(d_info->fib_Comment, s_info->fib_Comment);
					}

					// Indicate a directory
					*err_code = -42;

					// Set success code
					return COPY_OK;
				}

				// Wrong type
				else
					*err_code = ERROR_OBJECT_WRONG_TYPE;
			}

			// Couldn't create
			else
				*err_code = IoErr();
			return COPY_FAILED;
		}

		// Check source isn't read-protected
		if (s_info->fib_Protection & FIBF_READ)
		{
			// It is; fail
			*err_code = ERROR_READ_PROTECTED;
			return COPY_FAILED;
		}

		// Open source file
		if (!(in_file = Open(source_file, MODE_OLDFILE)))
		{
			// Store error code
			*err_code = IoErr();
			return COPY_FAILED;
		}

		// Get file size
		file_size = GETFIBSIZE(s_info);

		// Set file size
//		function_progress_file(handle,file_size*2,0);
#ifdef USE_64BIT
		function_progress_file64(handle, (QUAD *)&file_size, 0);
#else
		function_progress_file(handle, file_size, 0);
#endif

		// Start with an 8K buffer
		buffer_size = COPY_INITIAL_BUFFER;

		// Allocate initial buffer
		initial_buffer = AllocVec(COPY_INITIAL_BUFFER, 0);
		file_buffer = initial_buffer;

		// Got buffer?
		if (file_buffer)
		{
			// Open output file; if top level, use original function
			if (copy_flags & COPY_TOP_LEVEL)
				out_file = OriginalOpen(dest_file, MODE_NEWFILE);

			// Otherwise allow patched function to be used
			else
				out_file = Open(dest_file, MODE_NEWFILE);

			// Got file?
			if (out_file)
			{
				ULONG copytime;

				// Initial copy time setting
				copytime = 750000;

				// Loop while data remains to process
				while (file_size > 0)
				{
					LONG read_size, write_size, old_buffersize, size;
					struct timeval start, end;
					BOOL new_buf = 0;

					// Check abort
					if (function_check_abort(handle))
					{
						ret_code = COPY_ABORTED;
						break;
					}

					// Remember old buffer size
					old_buffersize = buffer_size;

					// Get smaller buffer?
					if (copytime > COPY_HIGH_THRESH)
					{
						// Halve buffer size
						buffer_size >>= 1;

						// Minimum 2K
						if (buffer_size < COPY_MIN_BUFFER)
							buffer_size = COPY_MIN_BUFFER;

						// Try for smaller buffer
						if (buffer_size != old_buffersize)
							new_buf = 1;
					}

					// Or a larger one
					else if (copytime < COPY_LOW_THRESH)
					{
						// Double buffer size
						buffer_size <<= 1;

						// Try for larger buffer
						new_buf = 1;
					}

					// Want to try for new buffer?
					if (new_buf)
					{
						// Free existing buffer (unless it's the initial one)
						if (file_buffer != initial_buffer)
							FreeVec(file_buffer);

						// Allocate new buffer
						if (!(file_buffer = AllocVec(buffer_size, 0)))
						{
							// Failed, fall back to initial buffer
							file_buffer = initial_buffer;
							buffer_size = COPY_INITIAL_BUFFER;
						}
					}

					// Get size to read
					read_size = (file_size > buffer_size) ? buffer_size : file_size;

					// Get current time
					GetSysTime((APTR)&start);

					// Read data
					if ((size = Read(in_file, file_buffer, read_size)) < 1)
						break;

					// Did we get back less than we asked for?
					if (size < read_size)
					{
						// Reduce file size by the difference
						file_size -= read_size - size;
					}

					// Save read size
					read_size = size;

					// Add to total
					//					total_size+=size;
					total_size += (size >> 1);

					// Update file progress
#ifdef USE_64BIT
					function_progress_file64(handle, 0, (QUAD *)&total_size);
#else
					function_progress_file(handle, 0, total_size);
#endif
					// Check abort
					if (function_check_abort(handle))
					{
						ret_code = COPY_ABORTED;
						break;
					}

					// Password supplied?
					if (password)
					{
						int a;
						char enbyte;

						// Encrypting?
						if (!decrypt_flag)
						{
							for (a = 0; a < read_size; a++)
							{
								enbyte = RandomDopus(9999);
								file_buffer[a] += enbyte;
							}
						}

						// Decrypting
						else
						{
							for (a = 0; a < read_size; a++)
							{
								enbyte = RandomDopus(9999);
								file_buffer[a] -= enbyte;
							}
						}
					}

					// If top level, write with original function
					if (copy_flags & COPY_TOP_LEVEL)
						write_size = OriginalWrite(out_file, file_buffer, read_size);

					// Otherwise, use patched function
					else
						write_size = Write(out_file, file_buffer, read_size);

					// Failed?
					if (write_size < read_size)
						break;

					// Get current time
					GetSysTime((APTR)&end);

					// Calculate time that portion of the copy took
					SubTime((APTR)&end, (APTR)&start);
					copytime = (end.tv_secs * 1000000) + end.tv_micro;

					// Subtract from remaining size
					file_size -= write_size;

					// Add to total
					//					total_size+=write_size;
					total_size += (write_size >> 1);

					// Update file progress
#ifdef USE_64BIT
					function_progress_file64(handle, 0, (QUAD *)&total_size);
#else
					function_progress_file(handle, 0, total_size);
#endif
				}

				// Successful?
				if (file_size == 0)
					ret_code = COPY_OK;
			}
		}

		// Store error code
		*err_code = IoErr();

		// Close in file
		if (in_file)
			Close(in_file);

		// Get destination file information
		if (out_file)
		{
			// Close out file; if top level, use original function
			if (copy_flags & COPY_TOP_LEVEL)
				OriginalClose(out_file);

			// Otherwise, allow patched function to be used
			else
				Close(out_file);

			// Re-lock out-file
			if ((lock = Lock(dest_file, ACCESS_READ)))
			{
				// Examine file
#ifdef USE_64BIT
				if (ExamineLock64(lock, (FileInfoBlock64 *)d_info))
#else
				if (Examine(lock, d_info))
#endif
					got_dest_info = 1;
				UnLock(lock);
			}
		}

		// Free buffer memory
		if (file_buffer != initial_buffer)
			FreeVec(file_buffer);
		FreeVec(initial_buffer);
	}

	// If successful, copy other things
	if (ret_code == COPY_OK)
	{
		// Copy date?
		if (copy_flags & COPY_DATE)
		{
			short res;

			// Set date; use original function if at top-level
			if (copy_flags & COPY_TOP_LEVEL)
				res = OriginalSetFileDate(dest_file, &s_info->fib_Date);

			// Otherwise, allow patched function to be used
			else
				res = SetFileDate(dest_file, &s_info->fib_Date);

			// Successful?
			if (res && got_dest_info)
				d_info->fib_Date = s_info->fib_Date;
		}

		// Copy protection?
		if (copy_flags & COPY_PROT)
		{
			short res;
			ULONG prot = s_info->fib_Protection & ~FIBF_ARCHIVE;

			// Set protection; use original function if at top-level
			if (copy_flags & COPY_TOP_LEVEL)
				res = OriginalSetProtection(dest_file, prot);

			// Otherwise, allow patched function to be used
			else
				res = SetProtection(dest_file, prot);

			// Successful?
			if (res && got_dest_info)
				d_info->fib_Protection = prot;
		}

		// Copy comment
		if (copy_flags & COPY_NOTE)
		{
			short res;

			// Set comment; use original function if at top-level
			if (copy_flags & COPY_TOP_LEVEL)
				res = OriginalSetComment(dest_file, s_info->fib_Comment);

			// Otherwise, allow patched function to be used
			else
				res = SetComment(dest_file, s_info->fib_Comment);

			// Successful?
			if (res && got_dest_info)
				strcpy(d_info->fib_Comment, s_info->fib_Comment);
		}

		// Did we fail to get destination info from the file handle?
		if (!got_dest_info)
			ret_code = COPY_FAILED;

		// Set source archive?
		if (copy_flags & COPY_ARC)
		{
			if (!(s_info->fib_Protection & FIBF_ARCHIVE))
			{
				// Get new bits
				s_info->fib_Protection |= FIBF_ARCHIVE;

				// Set protection; use original function if at top-level
				if (copy_flags & COPY_TOP_LEVEL)
					OriginalSetProtection(source_file, s_info->fib_Protection);

				// Otherwise, allow patched function to be used
				else
					SetProtection(source_file, s_info->fib_Protection);
			}
		}
	}

	// Otherwise, delete failed file
	else if (out_file)
		DeleteFile(dest_file);

	return ret_code;
}

// Set position for an icon
void copy_icon_position(FunctionHandle *handle, char *name, struct DiskObject *icon)
{
	ULONG flags;

	// Get icon flags
	flags = GetIconFlags(icon);

	// Reset position
	icon->do_CurrentX = NO_ICON_POSITION;
	icon->do_CurrentY = NO_ICON_POSITION;
	flags &= ~ICONF_POSITION_OK;

	if (CheckAppMessage(handle->app_msg))
	{
		short num;

		// Find arg number
		if ((num = FindWBArg(handle->app_msg->da_Msg.am_ArgList, handle->app_msg->da_Msg.am_NumArgs, name)) > -1)
		{
			short x, y;

			// Fix icon position
			x = handle->app_msg->da_DragOffset.x + handle->app_msg->da_Msg.am_MouseX +
				handle->app_msg->da_DropPos[num].x;
			y = handle->app_msg->da_DragOffset.y + handle->app_msg->da_Msg.am_MouseY +
				handle->app_msg->da_DropPos[num].y;

			// Store position, unless removing Opus positions
			if (!(environment->env->display_options & DISPOPTF_REMOPUSPOS))
			{
				SetIconPosition(icon, x, y);
				flags |= ICONF_POSITION_OK;
			}

			// Remove Opus position
			else
			{
				flags &= ~ICONF_POSITION_OK;
			}

			// Using Workbench positions?
			if (environment->env->display_options & DISPOPTF_ICON_POS)
			{
				// Set position
				icon->do_CurrentX = x;
				icon->do_CurrentY = y;
			}
		}
	}

	// Set flags
	SetIconFlags(icon, flags);
}
