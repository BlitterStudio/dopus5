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
#include "string_hook.h"
#include "clipboard.h"

HookData *LIBFUNC L_GetEditHook(REG(d0, ULONG type), REG(d1, ULONG flags), REG(a0, struct TagItem *tags))
{
	HookData *hook;

	// Allocate hook
	if ((hook = AllocVec(sizeof(HookData), MEMF_ANY)))
	{
// Fill out hook
#if defined(__AROS__) || defined(__MORPHOS__)
		hook->hook.h_Entry = (HOOKFUNC)HookEntry;
		hook->hook.h_SubEntry = (HOOKFUNC)string_edit_hook;
#else
		hook->hook.h_Entry = (ULONG(*)())string_edit_hook;
#endif
		hook->type = type;
		hook->flags = flags;

		// Get data from tags
		hook->history = (Att_List *)GetTagData(GTCustom_History, 0, tags);
		hook->change_task = (struct Task *)GetTagData(GTCustom_ChangeSigTask, 0, tags);
		hook->change_bit = GetTagData(GTCustom_ChangeSigBit, 0, tags);
	}

	return hook;
}

void LIBFUNC L_FreeEditHook(REG(a0, APTR hook))
{
	FreeVec(hook);
}

char *LIBFUNC L_GetSecureString(REG(a0, struct Gadget *gadget))
{
	struct StringInfo *info;
	char *ptr;

	// Get StringInfo pointer
	info = (struct StringInfo *)gadget->SpecialInfo;

	// Get pointer to real string
	ptr = info->Buffer + (info->MaxChars >> 1);

	// Make sure string is null-terminated
	ptr[info->NumChars] = 0;

	// Return pointer to 'real' string
	return ptr;
}

ULONG ASM string_edit_hook(REG(a0, HookData *hook), REG(a2, struct SGWork *work), REG(a1, ULONG *msg))
{
	ULONG ret = 1;
	short buffer_pos;
	BOOL signal = 0;

	// Cache buffer position
	buffer_pos = work->StringInfo->BufferPos;

	// Key press?
	while (*msg == SGH_KEY)
	{
		// If this is a hotkey field, we grab stuff UNLESS capslock is down or return pressed
		if (hook->flags & OBJECTF_HOTKEY && !(work->IEvent->ie_Qualifier & IEQUALIFIER_CAPSLOCK) &&
			work->IEvent->ie_Code != 0x44)
		{
			// Ignore key up and repeats
			if (!(work->IEvent->ie_Code & IECODE_UP_PREFIX) && !(work->IEvent->ie_Qualifier & IEQUALIFIER_REPEAT))
			{
				// Ignore qualifier keys
				if (work->IEvent->ie_Code != 0x63 && work->IEvent->ie_Code != 0x60 && work->IEvent->ie_Code != 0x64 &&
					work->IEvent->ie_Code != 0x66 && work->IEvent->ie_Code != 0x67 && work->IEvent->ie_Code != 0x65 &&
					work->IEvent->ie_Code != 0x61)
				{
					// Convert string
					L_IPC_Command(launcher_ipc,
								  STRING_CONVERT_KEY,
								  (work->IEvent->ie_Code << 16) | work->IEvent->ie_Qualifier,
								  work->WorkBuffer,
								  0,
								  REPLY_NO_PORT_IPC);

					// Same as last time?
					if (strcmp(work->WorkBuffer, work->PrevBuffer) == 0)
					{
						// Was this backspace or delete?
						if (work->IEvent->ie_Code == 0x41 || work->IEvent->ie_Code == 0x46)
						{
							// Clear buffer
							*work->WorkBuffer = 0;
						}
					}

					// Fix settings
					work->NumChars = strlen(work->WorkBuffer);
					work->BufferPos = strlen(work->WorkBuffer);
					work->Actions |= SGA_REDISPLAY;

					// Set flag to signal
					signal = 1;
					break;
				}
			}

			// Don't use
			work->Actions &= ~SGA_USE;
			break;
		}

		// Hotkey field with capslock?
		if (hook->flags & OBJECTF_HOTKEY)
		{
			char *ptr;

			// Old character position
			ptr = work->WorkBuffer + work->StringInfo->BufferPos;

			// Change to lowercase
			if (*ptr >= 'A' && *ptr <= 'Z')
				*ptr += ('a' - 'A');
		}

		// Uppercase string
		else if (hook->flags & OBJECTF_UPPERCASE)
		{
			char *ptr;

			// Old character position
			ptr = work->WorkBuffer + work->StringInfo->BufferPos;

			// Change to uppercase
			if (*ptr >= 'a' && *ptr <= 'z')
				*ptr -= ('a' - 'A');
		}

		// What happened?
		switch (work->EditOp)
		{
		// Return pressed?
		case EO_ENTER:

			// Shift pressed?
			if (work->IEvent->ie_Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
			{
				// Activate previous gadget
				work->Actions &= ~SGA_USE;
				work->Actions |= SGA_END | SGA_PREVACTIVE;
			}

			// Otherwise
			else
			{
				// Deactivate
				work->Actions &= ~SGA_USE;
				work->Actions |= SGA_END;

				// Not read only?
				if (!(hook->flags & OBJECTF_READ_ONLY))
				{
					// Want next gadget?
					if (!(hook->flags & OBJECTF_NO_SELECT_NEXT))
					{
						struct Gadget *gadget;

						// Don't want to wrap around, so see if there are any others
						for (gadget = work->Gadget->NextGadget; gadget; gadget = gadget->NextGadget)
						{
							// Not disabled?
							if (!(gadget->Flags & GFLG_DISABLED))
							{
								// Tab cycle set?
								if (gadget->Flags & GFLG_TABCYCLE)
								{
									// Activate next gadget
									work->Actions |= SGA_NEXTACTIVE;
									break;
								}
							}
						}
					}
				}
			}
			break;

		// Clear/undo
		case EO_CLEAR:
		case EO_UNDO:

			// Read-only?
			if (hook->flags & OBJECTF_READ_ONLY)
				work->Actions &= ~SGA_USE;

			// Set flag for signal
			else
				signal = 1;
			break;

		// Real character typed
		case EO_INSERTCHAR:
		case EO_REPLACECHAR:

			// Right-Amiga held down?
			if (work->IEvent->ie_Qualifier & IEQUALIFIER_RCOMMAND)
			{
				// What key was pressed?
				switch (work->Code)
				{
				// Copy to clipboard
				case 'c':
				case 'C': {
					ClipData data;

					// Secure field?
					if (hook->flags & OBJECTF_SECURE)
					{
						short len;

						// Get maximum length of string, and string pointer
						len = work->StringInfo->MaxChars >> 1;
						data.string = work->PrevBuffer + len;
					}

					// Normal field
					else
						data.string = work->PrevBuffer;

					// Get length
					data.length = work->StringInfo->NumChars;

					// Clip string
					L_IPC_Command(launcher_ipc, CLIP_PUTSTRING, 0, &data, 0, REPLY_NO_PORT_IPC);

					// Don't use
					work->Actions &= ~SGA_USE;
				}
				break;

				// Paste from clipboard
				case 'v':
				case 'V':

					// Not read only?
					if (!(hook->flags & OBJECTF_READ_ONLY))
					{
						ClipData data;
						char *buffer = 0;

						// Fill out packet
						data.string = work->WorkBuffer;
						data.length = work->StringInfo->MaxChars;

						// Try to allocate buffer for insert
						if (work->Code == 'V' && (buffer = AllocVec((work->StringInfo->MaxChars + 1) * 2, MEMF_CLEAR)))
						{
							// Fill out packet
							data.string = buffer;
						}

						// Paste string
						L_IPC_Command(launcher_ipc, CLIP_GETSTRING, 0, &data, 0, REPLY_NO_PORT_IPC);

						// Got a valid string?
						if (data.result)
						{
							char *ptr;

							// Integer?
							if (hook->flags & OBJECTF_INTEGER)
							{
								// Go through string, strip after a non-digit
								for (ptr = data.string; *ptr; ptr++)
								{
									// Not a digit?
									if ((*ptr < '0' || *ptr > '9') && *ptr != '-' && *ptr != '+')
									{
										// Strip from here
										*ptr = 0;
										break;
									}
								}
							}

							// Path filter?
							if (hook->flags & OBJECTF_PATH_FILTER)
							{
								// Go through string, strip after a path character
								for (ptr = data.string; *ptr; ptr++)
								{
									// Path character?
									if (*ptr == '/' || *ptr == ':')
									{
										// Strip from here
										*ptr = 0;
										break;
									}
								}
							}

							// Insert?
							if (buffer)
							{
								char *temp;
								short a, len;

								// Get temp buffer pointer
								temp = buffer + work->StringInfo->MaxChars + 1;

								// Copy head of old string
								stccpy(temp, work->StringInfo->Buffer, work->StringInfo->BufferPos + 1);

								// Tack on insertion string
								a = work->StringInfo->MaxChars - work->StringInfo->NumChars;
								if ((len = strlen(buffer)) > a)
									len = a;
								if (len > 0)
									stccpy(temp + strlen(temp), buffer, len + 1);

								// Tack on end of old string
								strcat(temp, work->StringInfo->Buffer + work->StringInfo->BufferPos);

								// Copy new string to work buffer
								strcpy(work->WorkBuffer, temp);
							}

							// Fix length
							work->NumChars = strlen(work->WorkBuffer);
							work->BufferPos = (buffer) ? work->StringInfo->BufferPos + strlen(buffer) : work->NumChars;
							work->Actions |= SGA_REDISPLAY;

							// Set flag to signal
							signal = 1;

							// Secure field?
							if (hook->flags & OBJECTF_SECURE)
							{
								short len;

								// Get maximum length of string, and string pointer
								len = work->StringInfo->MaxChars >> 1;
								ptr = work->PrevBuffer + len;

								// Copy string to real buffer
								strcpy(ptr, work->WorkBuffer);

								// Replace work buffer with asterisks
								for (len = work->NumChars - 1; len >= 0; len--)
									work->WorkBuffer[len] = '*';
							}
						}
						else
						{
							// Don't use
							work->Actions &= ~SGA_USE;
							work->Actions |= SGA_BEEP;
						}

						// Free buffer
						FreeVec(buffer);
					}

					// Don't use
					else
						work->Actions &= ~SGA_USE;
					break;

				// Something else
				default:

					// Read-only?
					if (hook->flags & OBJECTF_READ_ONLY)
						work->Actions &= ~SGA_USE;

					// Set flag to signal
					signal = 1;
					break;
				}
				break;
			}

			// Read-only?
			if (hook->flags & OBJECTF_READ_ONLY && work->Code != 0x1b)
			{
				work->Actions &= ~SGA_USE;
				break;
			}

			// Look at key
			switch (work->Code)
			{
			// Control U or Q (undo)
			case 0x11:
			case 0x15:

				// Copy undo string back
				CopyMem(work->StringInfo->UndoBuffer, work->PrevBuffer, work->StringInfo->MaxChars);

				// Copy to work buffer
				strcpy(work->WorkBuffer, work->PrevBuffer);

				// Fix settings
				work->NumChars = strlen(work->WorkBuffer);
				work->BufferPos = work->NumChars;
				work->Actions |= SGA_REDISPLAY;

				// Set flag to signal
				signal = 1;
				break;

			// Control X (clear)
			case 0x18:

				// Copy to undo buffer
				CopyMem(work->PrevBuffer, work->StringInfo->UndoBuffer, work->StringInfo->MaxChars);

				// Clear work buffer
				*work->WorkBuffer = 0;
				work->NumChars = 0;
				work->BufferPos = 0;
				work->Actions |= SGA_REDISPLAY;

				// Set flag to signal
				signal = 1;
				break;

			// Escape deactivates gadget
			case 0x1b:
				work->Actions &= ~SGA_USE;
				work->Actions |= SGA_END;
				work->Code = 0x45;
				break;

			// Filter path characters
			case '/':
			case ':':

				// Is filter set?
				if (hook->flags & OBJECTF_PATH_FILTER)
				{
					work->Actions &= ~SGA_USE;
					work->Actions |= SGA_BEEP;
					break;
				}

			// Some key
			default:

				// Integer gadget?
				if (hook->flags & OBJECTF_INTEGER)
				{
					// If this isn't a number or a sign, fail
					if ((work->Code < '0' || work->Code > '9') && work->Code != '-' && work->Code != '+')
					{
						// Fail
						work->Actions &= ~SGA_USE;
						work->Actions |= SGA_BEEP;
						break;
					}
				}

				// Hiding string?
				if (hook->flags & OBJECTF_SECURE)
				{
					short len;
					char *ptr;

					// Get maximum length of string, and string pointer
					len = work->StringInfo->MaxChars >> 1;
					ptr = work->PrevBuffer + len;

					// Check length
					if (work->NumChars >= len)
					{
						// Buffer full
						work->Actions &= ~SGA_USE;
						work->Actions |= SGA_BEEP;
						break;
					}

					// Add character to 'real' string
					ptr[work->StringInfo->BufferPos] = work->Code;

					// Null-terminate real string
					ptr[work->NumChars] = 0;

					// Insert asterisk into work buffer
					work->WorkBuffer[work->StringInfo->BufferPos] = '*';
				}

				// Set flag for signal
				signal = 1;
				break;
			}
			break;

		// Character deleted backwards
		case EO_DELBACKWARD:

			// Read-only?
			if (hook->flags & OBJECTF_READ_ONLY)
			{
				work->Actions &= ~SGA_USE;
			}

			// Otherwise
			else
			{
				short len;
				char *ptr;

				// Set flag for signal
				signal = 1;

				// Ignore if not in secure mode
				if (!(hook->flags & OBJECTF_SECURE))
					break;

				// Ignore if cursor at beginning of string
				if (work->StringInfo->BufferPos == 0)
					break;

				// Get maximum length of string, and string pointer
				len = work->StringInfo->MaxChars >> 1;
				ptr = work->PrevBuffer + len;

				// Perform delete in real string
				strcpy(ptr + work->BufferPos, ptr + work->StringInfo->BufferPos);
			}
			break;

		// Character deleted forwards
		case EO_DELFORWARD:

			// Read-only?
			if (hook->flags & OBJECTF_READ_ONLY)
			{
				work->Actions &= ~SGA_USE;
			}

			// Otherwise
			else
			{
				short len;
				char *ptr;

				// Set flag for signal
				signal = 1;

				// Ignore if not in secure mode
				if (!(hook->flags & OBJECTF_SECURE))
					break;

				// Get maximum length of string, and string pointer
				len = work->StringInfo->MaxChars >> 1;
				ptr = work->PrevBuffer + len;

				// Perform delete in real string
				strcpy(ptr + work->BufferPos, ptr + work->BufferPos + 1);
			}
			break;

		// Raw key press
		default:

			// Handle cursor keys
			switch (work->IEvent->ie_Code)
			{
			// Left
			case CURSORLEFT:

				// Not at start of line?
				if (buffer_pos > 0)
				{
					// Control = page left
					if (work->IEvent->ie_Qualifier & IEQUALIFIER_CONTROL)
					{
						if (buffer_pos == work->StringInfo->NumChars &&
							work->StringInfo->NumChars > work->StringInfo->DispCount)
							buffer_pos -= work->StringInfo->DispCount - 1;
						else if (buffer_pos == work->StringInfo->DispPos)
							buffer_pos -= work->StringInfo->DispCount;
						else
							buffer_pos = work->StringInfo->DispPos;
					}

					// Alt = word left
					else if (work->IEvent->ie_Qualifier & (IEQUALIFIER_LALT | IEQUALIFIER_RALT))
					{
						short a;

						// Find next word break
						for (a = buffer_pos - 2; a >= 0; a--)
							if (work->PrevBuffer[a] == ' ')
								break;

						// Store new position
						buffer_pos = a + 1;
					}

					// Shift = full left
					else if (work->IEvent->ie_Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
						buffer_pos = 0;

					// Normal one character left
					else
						--buffer_pos;

					// Store new buffer position
					if (buffer_pos < 0)
						work->BufferPos = 0;
					else
						work->BufferPos = buffer_pos;
				}
				break;

			// Right
			case CURSORRIGHT:

				// Not at end of line?
				if (buffer_pos < work->StringInfo->NumChars)
				{
					// Control = page right
					if (work->IEvent->ie_Qualifier & IEQUALIFIER_CONTROL)
					{
						if (work->StringInfo->NumChars <= work->StringInfo->DispCount)
							buffer_pos = work->StringInfo->NumChars;
						else if (buffer_pos == work->StringInfo->DispPos + work->StringInfo->DispCount - 1)
							buffer_pos += work->StringInfo->DispCount;
						else
							buffer_pos = work->StringInfo->DispPos + work->StringInfo->DispCount - 1;
					}

					// Alt = word right
					else if (work->IEvent->ie_Qualifier & (IEQUALIFIER_LALT | IEQUALIFIER_RALT))
					{
						short a, flag = 0;

						// Find next word break
						for (a = buffer_pos; a < work->StringInfo->NumChars; a++)
							if (work->PrevBuffer[a] == ' ')
								flag = 1;
							else if (flag)
								break;

						// Store new position
						buffer_pos = a;
					}

					// Shift = full right
					else if (work->IEvent->ie_Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
						buffer_pos = work->StringInfo->NumChars;

					// Normal one character right
					else
						++buffer_pos;

					// Store new buffer position
					if (buffer_pos > work->StringInfo->NumChars)
						work->BufferPos = work->StringInfo->NumChars;
					else
						work->BufferPos = buffer_pos;
				}
				break;

			// History
			case CURSORUP:
			case CURSORDOWN: {
				Att_Node *node = 0;

				// No history?
				if (!hook->history)
					break;

				// Lock history list
				L_LockAttList(hook->history, FALSE);

				// Move up?
				if (work->IEvent->ie_Code == CURSORUP)
				{
					// If no current node, get last
					if (!hook->history->current)
						node = (Att_Node *)hook->history->list.lh_TailPred;

					// Otherwise, get previous
					else if (hook->history->current->node.ln_Pred && hook->history->current->node.ln_Pred->ln_Pred)
						node = (Att_Node *)hook->history->current->node.ln_Pred;
				}

				// Move down
				else
				{
					// If no current node, get first
					if (!hook->history->current)
						node = (Att_Node *)hook->history->list.lh_Head;

					// Otherwise, get next
					else if (hook->history->current->node.ln_Succ && hook->history->current->node.ln_Succ->ln_Succ)
						node = (Att_Node *)hook->history->current->node.ln_Succ;
				}

				// Valid node?
				if (node && node->node.ln_Succ && node->node.ln_Pred)
				{
					// Copy string
					strcpy(work->WorkBuffer, node->node.ln_Name);
					work->NumChars = strlen(work->WorkBuffer);
					work->BufferPos = work->NumChars;
					work->Actions |= SGA_REDISPLAY;

					// Store node
					hook->history->current = node;

					// Set flag to signal
					signal = 1;
				}

				// Unlock history list
				L_UnlockAttList(hook->history);
			}
			break;
			}
			break;
		}

		break;
	}

	// Unknown?
	if (*msg != SGH_CLICK)
		ret = 0;

	// Task to signal?
	if (signal && hook->change_task)
		Signal(hook->change_task, 1 << hook->change_bit);

	return ret;
}
