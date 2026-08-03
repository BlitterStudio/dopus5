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

#include "dopus.h"

// Find button function
Cfg_ButtonFunction *button_find_function(Cfg_Button *button, short which, APTR *image)
{
	Cfg_ButtonFunction *func = 0;

	// Standard function?
	if (which == FTYPE_LEFT_BUTTON || which == FTYPE_RIGHT_BUTTON || which == FTYPE_MID_BUTTON)
	{
		func = (Cfg_ButtonFunction *)FindFunctionType((struct List *)&button->function_list, which);
	}

	// Custom function?
	else if (which & FTYPE_SPECIAL_FLAG)
	{
		short num;

		// Clear special flag
		which &= ~FTYPE_SPECIAL_FLAG;

		// Find function
		for (func = (Cfg_ButtonFunction *)button->function_list.mlh_Head, num = 0; func->node.ln_Succ;
			 func = (Cfg_ButtonFunction *)func->node.ln_Succ)
		{
			// Special function?
			if (func->function.func_type == FTYPE_LIST)
			{
				// Match number?
				if (num == which)
					break;

				// Increment count
				++num;
			}
		}

		// Invalid?
		if (!func->node.ln_Succ)
			func = 0;
	}

	// Want image pointer?
	if (func && image)
	{
		// Set image value
		*image = func->image;

		// No image for this function?
		if (!func->image)
		{
			Cfg_ButtonFunction *ifunc;

			// Look for left-button function
			if ((ifunc =
					 (Cfg_ButtonFunction *)FindFunctionType((struct List *)&button->function_list, FTYPE_LEFT_BUTTON)))
			{
				// Use this image
				*image = ifunc->image;
			}
		}
	}

	return func;
}

// Run a function from a button
void buttons_run_button(Buttons *buttons, Cfg_Button *button, short which)
{
	Cfg_Function *function;
	struct ArgArray *array = 0;

	// Get the function to run
	if (!(function = button_valid(button, which)))
		return;

	// Desktop selection fallback: when no lister provides a selection,
	// honour a selected desktop icon (as the icon context menu does)
	if (!lister_has_selection() && (array = desktop_selection_argarray(GUI->backdrop)))
	{
		function_launch(FUNCTION_RUN_FUNCTION_EXTERNAL,
						function,
						0,
						0,
						0,
						0,
						environment->env->desktop_location,
						0,
						array,
						0,
						buttons);
		return;
	}

	// Launch function
	function_launch(FUNCTION_RUN_FUNCTION, function, 0, 0, 0, 0, 0, 0, 0, 0, buttons);
}

// Handle some appstuff
BOOL buttons_app_message(Buttons *buttons, DOpusAppMessage *msg)
{
	short col, row;
	Cfg_Button *button;
	Cfg_Function *function;
	struct ArgArray *arg_array;
	IPCData *ipc;

	// Lock process list
	lock_listlock(&GUI->process_list, FALSE);

	// See if button editor is running
	ipc = IPC_FindProc(&GUI->process_list, NAME_BUTTON_EDITOR_RUN, FALSE, 0);

	// Unlock process list
	unlock_listlock(&GUI->process_list);

	// Editor running?
	if (ipc)
	{
		// Send button
		if (buttons_edit_bank(buttons, -1, -1, 0, (struct AppMessage *)msg, 0))
		{
			// Message was swallowed
			return 1;
		}
	}

	// Lock bank
	GetSemaphore(&buttons->bank->lock, SEMF_SHARED, 0);

	// Get button and function we dropped on
	col = msg->da_Msg.am_MouseX;
	row = msg->da_Msg.am_MouseY;
	if (!(button = button_from_point(buttons, &col, &row)) || !(function = button_valid(button, button->current)))
	{
		FreeSemaphore(&buttons->bank->lock);
		return 0;
	}

	// Get arg array
	if ((arg_array = AppArgArray(msg, AAF_ALLOW_DIRS)))
	{
		BPTR lock;
		char pathname[256];

		// Get pathname of first file
		DevNameFromLockDopus(msg->da_Msg.am_ArgList[0].wa_Lock, pathname, 256);

		// Need source directory; if no name, get parent
		if ((!msg->da_Msg.am_ArgList[0].wa_Name || !*msg->da_Msg.am_ArgList[0].wa_Name) &&
			(lock = ParentDir(msg->da_Msg.am_ArgList[0].wa_Lock)))
		{
			// Get pathname of parent
			DevNameFromLockDopus(lock, pathname, 256);
			UnLock(lock);
		}

		// Launch function
		function_launch(FUNCTION_RUN_FUNCTION_EXTERNAL, function, 0, 0, 0, 0, pathname, 0, arg_array, 0, buttons);
	}

	// Unlock button lock
	FreeSemaphore(&buttons->bank->lock);

	return 0;
}
