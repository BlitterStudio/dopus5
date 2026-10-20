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
#include "key_finder.h"

STATIC BOOL cx_over_icon(BackdropInfo *info, short x, short y);

// Install commodity
void cx_install(CxData *cx)
{
	CxObj *filter, *cxobj;
	Cfg_Button *button;
	Cfg_ButtonFunction *function;
	short count = 0;
	IX ix;

	// Initialise broker
	cx->nb.nb_Version = NB_VERSION;
	cx->nb.nb_Name = "Directory Opus 5";
	cx->nb.nb_Title = "(c) 1998 Jonathan Potter & GPSoftware";
	cx->nb.nb_Descr = GetString(&locale, MSG_CX_DESC);
	cx->nb.nb_Unique = 0;
	cx->nb.nb_Flags = COF_SHOW_HIDE;
	cx->nb.nb_Pri = GUI->cx_pri;
	cx->nb.nb_Port = GUI->appmsg_port;
	cx->nb.nb_ReservedChannel = 0;

	// Install commodity
	if (!(cx->broker = CxBroker(&cx->nb, 0)))
		return;

	// Install pop-up filter
	cx_install_popup(cx);

	// Valid hotkeys?
	if (GUI->hotkeys)
	{
		// Lock hotkeys list
		GetSemaphore(&GUI->hotkeys_lock, SEMF_SHARED, 0);

		// Go through hotkey buttons
		for (button = (Cfg_Button *)GUI->hotkeys->buttons.lh_Head; button->node.ln_Succ;
			 button = (Cfg_Button *)button->node.ln_Succ)
		{
			// Global?
			if (!(button->button.flags & BUTNF_GLOBAL))
				continue;

			// Get left button function
			if ((function =
					 (Cfg_ButtonFunction *)FindFunctionType((struct List *)&button->function_list, FTYPE_LEFT_BUTTON)))
			{
				// Valid function?
				if (!(IsListEmpty((struct List *)&function->instructions)))
				{
					// Got a keycode?
					if (function->function.code != 0xffff)
					{
						// Initialise IX
						ix.ix_Version = IX_VERSION;
						ix.ix_Class = IECLASS_RAWKEY;
						ix.ix_Code = function->function.code;
						ix.ix_CodeMask = 0xffff;
						ix.ix_Qualifier = function->function.qual;
						ix.ix_QualMask = function->function.qual_mask;
						ix.ix_QualSame = function->function.qual_same;

						// Create a new filter
						if ((filter = CxFilter(0)))
						{
							// Set IX
							SetFilterIX(filter, &ix);

							// Make a sender
							if ((cxobj = CxSender(cx->nb.nb_Port, CXCODE_HOTKEY_BASE + count)))
								AttachCxObj(filter, cxobj);

							// Make a translator
							if ((cxobj = CxTranslate(0)))
								AttachCxObj(filter, cxobj);

							// Add to broker
							AttachCxObj(cx->broker, filter);
						}

						// Increment count
						++count;
					}
				}
			}
		}

		// Release hotkeys list
		FreeSemaphore(&GUI->hotkeys_lock);
	}

	// Enable broker
	ActivateCxObj(cx->broker, 1);
}

// Remove commodity
void cx_remove(CxData *cx)
{
	if (cx)
	{
		// Delete the broker
		DeleteCxObjAll(cx->broker);
		cx->broker = 0;
	}
}

// Execute a hotkey
void cx_hotkey(long id)
{
	Cfg_Button *button;
	Cfg_ButtonFunction *function;
	long count = 0;

	// Lock hotkeys list
	GetSemaphore(&GUI->hotkeys_lock, SEMF_SHARED, 0);

	// Go through hotkey buttons
	for (button = (Cfg_Button *)GUI->hotkeys->buttons.lh_Head; button->node.ln_Succ;
		 button = (Cfg_Button *)button->node.ln_Succ)
	{
		// Global?
		if (!(button->button.flags & BUTNF_GLOBAL))
			continue;

		// Get left button function
		if ((function =
				 (Cfg_ButtonFunction *)FindFunctionType((struct List *)&button->function_list, FTYPE_LEFT_BUTTON)))
		{
			// Valid function?
			if (!(IsListEmpty((struct List *)&function->instructions)))
			{
				// Got a keycode?
				if (function->function.code != 0xffff)
				{
					// Is this the one we want?
					if (count == id)
					{
						BOOL ok = 1;

						// Is Key Finder in the system?
						if (GUI->flags2 & GUIF2_KEY_FINDER)
						{
							ULONG lock;
							struct Window *window;

							// Lock Intuition
							lock = LockIBase(0);

							// Check active window for Key Finder
							if (GetWindowID((window = IntuitionBase->ActiveWindow)) == WINDOW_KEYFINDER)
							{
								IPCData *ipc;

								// Lock the process list
								lock_listlock(&GUI->process_list, FALSE);

								// Find KeyFinder process
								if ((ipc = IPC_FindProc(&GUI->process_list, "dopus_keyfinder", TRUE, 0)))
								{
									// Send key message to KeyFinder
									IPC_Command(
										ipc,
										KFIPC_KEYCODE,
										(function->function.code << 16) | function->function.qual,
										(APTR)(IPTR)((function->function.qual_mask << 16) | function->function.qual_same),
										0,
										0);
								}

								// Unlock process list
								unlock_listlock(&GUI->process_list);

								// Set flag to swallow event
								ok = 0;
							}

							// Unlock Intuition
							UnlockIBase(lock);
						}

						// Ok to launch function?
						if (ok)
						{
							Cfg_Function *func;

							// Copy function
							if ((func = CopyFunction((Cfg_Function *)function, 0, 0)))
							{
								// Set flag to free function
								func->function.flags2 |= FUNCF2_FREE_FUNCTION;

								// Launch function
								function_launch_quick(FUNCTION_RUN_FUNCTION, func, 0);
							}
						}
						break;
					}

					// Increment count
					++count;
				}
			}
		}
	}

	// Unlock hotkeys list
	FreeSemaphore(&GUI->hotkeys_lock);
}

// Right mousebutton filter
STATIC void SAVEDS cx_right_button(register CxMsg *cxm, CxObj *co)
{
	struct InputEvent *ie;
	struct Window *window = 0;
	CxData *data;
	BOOL lock = 0;
	short x = 0, y = 0;

	// Get InputEvent from CxMsg
	ie = (struct InputEvent *)CxMsgData(cxm);

	// Get data pointer (the 32-bit CxMsgID() can't carry a 64-bit pointer)
	data = GUI->cx;

	// Trap mouse events when not iconified; tablets and virtual machines
	// report pointer moves as IECLASS_NEWPOINTERPOS, which a menu opened in
	// an inactive window needs as well
	if ((ie->ie_Class == IECLASS_RAWMOUSE || (ie->ie_Class == IECLASS_NEWPOINTERPOS && data->right_down)) &&
		GUI->screen_pointer)
	{
		// Mouse button push?
		if (!(ie->ie_Code & IECODE_UP_PREFIX) && !data->right_down)
		{
			struct Screen *scr;
			ULONG ilock;
			short which;
			BOOL store = 1;
			short mousex, mousey;

			// Get button number and mouse position
			which = ie->ie_Code - IECODE_LBUTTON;
			mousex = IntuitionBase->MouseX;
			mousey = IntuitionBase->MouseY;

			// Test for double-click
			if (GUI->dc_button == ie->ie_Code &&
#ifdef __amigaos4__
				DoubleClick(GUI->dc_seconds[which],
							GUI->dc_micros[which],
							ie->ie_TimeStamp.Seconds,
							ie->ie_TimeStamp.Microseconds))
#else
				DoubleClick(
					GUI->dc_seconds[which], GUI->dc_micros[which], ie->ie_TimeStamp.tv_secs, ie->ie_TimeStamp.tv_micro))
#endif
			{
				// Close enough to original position?
				if (mousex >= GUI->dc_pos[which].x - 5 && mousex <= GUI->dc_pos[which].x + 5 &&
					mousey >= GUI->dc_pos[which].y - 5 && mousey <= GUI->dc_pos[which].y + 5)
				{
					// Middle button?
					if (ie->ie_Code == IECODE_MBUTTON)
					{
						// Run script
						if (RunScript(SCRIPT_MID_DOUBLECLICK, 0))
						{
							// We ran a script
							GUI->dc_seconds[2] = 0;
							GUI->last_double_click = 0;

							// Eat the event
							DisposeCxMsg(cxm);
							return;
						}
					}

					// Save double-click result
					GUI->last_double_click = ie->ie_Code;
					store = 0;
				}
			}

			// Store doubleclick info
			if (store)
			{
				// Store button and double-click value
				GUI->dc_button = ie->ie_Code;
#ifdef __amigaos4__
				GUI->dc_seconds[which] = ie->ie_TimeStamp.Seconds;
				GUI->dc_micros[which] = ie->ie_TimeStamp.Microseconds;
#else
				GUI->dc_seconds[which] = ie->ie_TimeStamp.tv_secs;
				GUI->dc_micros[which] = ie->ie_TimeStamp.tv_micro;
#endif
				GUI->dc_pos[which].x = mousex;
				GUI->dc_pos[which].y = mousey;
				GUI->last_double_click = 0;
			}

			// Lock Intuition
			ilock = LockIBase(0);

			// Go through screens list
			for (scr = IntuitionBase->FirstScreen; scr; scr = scr->NextScreen)
			{
				// Is screen top <= mouse position (ie click is on this screen)
				if (scr->MouseY >= 0)
					break;
			}

			// Unlock Intuition
			UnlockIBase(ilock);

			// Is our screen active?
			if (GUI->flags & GUIF_OPEN && scr == GUI->screen_pointer)
			{
				// Ignore button if layerinfo is locked, or right button if mouse is over title bar and control not down
				if (GUI->screen_pointer->LayerInfo.LockLayersCount == 0 &&
					(ie->ie_Code != MENUDOWN || ie->ie_Qualifier & IEQUALIFIER_CONTROL ||
					 GUI->screen_pointer->MouseY > GUI->screen_pointer->BarHeight))
				{
					struct Layer *layer;

					// Get mouse position
					x = GUI->screen_pointer->MouseX;
					y = GUI->screen_pointer->MouseY;

					// Lock layers
					LockLayerInfo(&GUI->screen_pointer->LayerInfo);

					// Find layer
					if ((layer = WhichLayer(&GUI->screen_pointer->LayerInfo, x, y)))
					{
						ULONG id;

						// Get ID of layer's window
						id = GetWindowID(layer->Window);

						// Convert coordinates to window-relative
						if (layer->Window)
						{
							x -= ((struct Window *)layer->Window)->LeftEdge;
							y -= ((struct Window *)layer->Window)->TopEdge;
						}

						// Button bank window?
						if (id == WINDOW_BUTTONS)
						{
							IPCData *ipc;
							Buttons *buttons;

							// Get buttons ipc pointer
							if ((ipc = (IPCData *)GetWindowAppPort((struct Window *)layer->Window)) &&
								(buttons = (Buttons *)IPCDATA(ipc)))
							{
								// Get window pointer
								window = buttons->window;

								// See if mouse is outside of internal area
								if (x < buttons->internal.Left || y < buttons->internal.Top ||
									x >= buttons->internal.Left + buttons->internal.Width ||
									y >= buttons->internal.Top + buttons->internal.Height)
								{
									// If it's the left or middle button that was pressed, ignore it
									if (ie->ie_Code == IECODE_LBUTTON || ie->ie_Code == IECODE_MBUTTON)
										window = 0;
								}

								// Ignore left button if editor is up
								else if (ie->ie_Code == IECODE_LBUTTON && GUI->flags & GUIF_BUTTON_EDITOR)
									window = 0;
							}
						}

						// Lister? Only trap right and middle buttons
						else if (id == WINDOW_LISTER && ie->ie_Code != IECODE_LBUTTON)
						{
							IPCData *ipc;

							// Get window pointer
							window = layer->Window;

							// Get IPC port
							if ((ipc = (IPCData *)GetWindowAppPort(window)))
							{
								Lister *lister;

								// Get lister pointer
								if ((lister = (Lister *)IPCDATA(ipc)))
								{
									// If lister is locked ignore
									if (lister->flags & LISTERF_LOCK)
										window = 0;

									// Is lister in text or icon action mode?
									else if (!(lister->flags & LISTERF_VIEW_ICONS) ||
											 lister->flags & LISTERF_ICON_ACTION)
									{
										// Trap all buttons over the toolbar; if not...
										if (!(lister->flags & LISTERF_TOOLBAR) ||
											!(point_in_element(&lister->toolbar_area, x, y)))
										{
											// Middle button?
											if (ie->ie_Code == IECODE_MBUTTON)
											{
												// If editing mode is not enabled, don't trap middle
												if (!(environment->env->lister_options & LISTEROPTF_EDIT_MID) ||
													GUI->flags & LISTERF_VIEW_ICONS ||
													(lister->cur_buffer->buf_CustomHandler[0] &&
													 !(lister->cur_buffer->cust_flags & CUSTF_EDITING)))
													window = 0;

												// Only trap middle button over text area
												else if (!(point_in_element(&lister->list_area, x, y)))
													window = 0;
											}
										}
									}

									// Icon mode; don't trap middle button
									else if (ie->ie_Code == IECODE_MBUTTON)
										window = 0;
								}
							}
						}

						// Group?
						else if (id == WINDOW_GROUP)
						{
							// Only trap right button, if window is inactive
							if (ie->ie_Code == IECODE_RBUTTON &&
								!(((struct Window *)layer->Window)->Flags & WFLG_WINDOWACTIVE))
							{
								IPCData *ipc;

								// Get IPC port
								if ((ipc = (IPCData *)GetWindowAppPort(layer->Window)))
								{
									GroupData *group;

									// Get group pointer
									if ((group = (GroupData *)IPCDATA(ipc)))
									{
										// Get window pointer
										window = group->window;
									}
								}
							}
						}

						// Main window?
						else if (id == WINDOW_BACKDROP)
						{
							// Only trap right button, if window is inactive
							if (ie->ie_Code == IECODE_RBUTTON &&
								!(((struct Window *)layer->Window)->Flags & WFLG_WINDOWACTIVE))
							{
								// Over any objects?
								if (cx_over_icon(GUI->backdrop, x, y))
								{
									// Get window pointer
									window = layer->Window;
								}
							}

							// Check double-click
							if (!window && GUI->last_double_click == MENUDOWN)
							{
								// Run script
								if (RunScript(SCRIPT_RIGHT_DOUBLECLICK, 0))
								{
									// We ran a script
									GUI->dc_seconds[1] = 0;
									GUI->last_double_click = 0;

									// Eat the event
									DisposeCxMsg(cxm);
								}
							}
						}

						// Popup-menu?
						else if (id == WINDOW_POPUP_MENU)
						{
							// Only trap left button
							if (ie->ie_Code == IECODE_LBUTTON)
								window = layer->Window;
						}

						// Icon positioning
						else if (id == WINDOW_ICONPOS)
						{
							// Only trap right button
							if ((ie->ie_Code & ~IECODE_UP_PREFIX) == IECODE_RBUTTON)
								window = layer->Window;
						}

						// Text viewer
						else if (id == WINDOW_TEXT_VIEWER)
						{
							// Only trap right button, if window is inactive
							if (ie->ie_Code == IECODE_RBUTTON &&
								!(((struct Window *)layer->Window)->Flags & WFLG_WINDOWACTIVE))
							{
								// Get window pointer
								window = layer->Window;

								// Ignore if mouse is outside of text area
								if (x <= window->BorderLeft || x > window->Width - window->BorderRight ||
									y <= window->BorderTop || y > window->Height - window->BorderBottom)
									window = 0;
							}
						}

						// Start Menu?
						else if (id == WINDOW_START)
						{
							StartMenu *menu;

							// Get start menu pointer
							if ((menu = (StartMenu *)GetWindowAppPort((struct Window *)layer->Window)))
							{
								// Trap anything over button, unless shift is down
								if (!(ie->ie_Qualifier & IEQUAL_ANYSHIFT) && x >= menu->button.MinX &&
									y >= menu->button.MinY && x <= menu->button.MaxX && y <= menu->button.MaxY)
								{
									// Get window pointer
									window = menu->window;
								}

								// Trap right button over drag gadget
								else if (!(menu->bank->window.flags & BTNWF_NO_BORDER) && x >= menu->drag.LeftEdge &&
										 x < menu->drag.LeftEdge + menu->drag.Width && y >= menu->drag.TopEdge &&
										 y < menu->drag.TopEdge + menu->drag.Height)
								{
									// Get window pointer for right button
									if (ie->ie_Code == IECODE_RBUTTON)
										window = menu->window;
								}
							}
						}
					}

					// Set flag to say we locked
					lock = 1;
				}
			}
		}

		// Mouse move/release and button is down?
		else if (data->right_down)
		{
			ULONG id = 0;

			// Display open?
			if (GUI->flags & GUIF_OPEN)
			{
				short num;

				// Get mouse position
				x = GUI->screen_pointer->MouseX;
				y = GUI->screen_pointer->MouseY;

				// Lock layers
				LockLayerInfo(&GUI->screen_pointer->LayerInfo);

				// If we have a popup window we need to look for both
				num = (data->popup_window) ? 0 : 1;

				// Go through windows to see if we're still valid
				for (window = GUI->screen_pointer->FirstWindow; window; window = window->NextWindow)
				{
					// Match the one we're looking for?
					if (window == data->button_window)
					{
						// Convert coordinates to window-relative
						x -= window->LeftEdge;
						y -= window->TopEdge;
						++num;
					}

					// Match popup window?
					if (window == data->popup_window)
					{
						// Get window ID
						if ((id = GetWindowID(window)) == WINDOW_START)
						{
							StartMenu *start = (StartMenu *)GetWindowAppPort(window);
							if (start)
							{
								// Not popped?
								if (!(start->flags & STARTMENUF_POPPED))
								{
									if (data->start_popped)
										id = 0;
								}
								else
									data->start_popped = 1;
							}
						}
						++num;
					}

					// Found all we're looking for?
					if (num == 2)
						break;
				}

				// Set flag to say we locked
				lock = 1;
			}

			// If no window, clear flag
			if (!window)
			{
				data->right_down = 0;
				data->popup_window = 0;
				data->start_popped = 0;
			}

			// Check qualifiers to make sure buttons are still down (unless it's a popup menu)
			else if (ie->ie_Code == IECODE_NOBUTTON && !(ie->ie_Qualifier & data->qual_down) && id != WINDOW_START &&
					 id != WINDOW_POPUP_MENU)
			{
				data->right_down = 0;
				data->popup_window = 0;
				data->start_popped = 0;
				window = 0;
			}
		}
	}

	// Check for wheel event, cancel middle double-click
	else if (ie->ie_Class == 0x16 && (ie->ie_Code == 0x7a || ie->ie_Code == 0x7b))
	{
		GUI->dc_seconds[2] = 0;
	}

	// Got a window?
	if (window)
	{
		// If window is inactive, we need to send it a message
		if (!(window->Flags & WFLG_WINDOWACTIVE))
		{
			struct IntuiMessage *msg;

			// DisposeCxMsg() can free the event (AROS does), so keep its code
			UWORD code = ie->ie_Code;

			// Allocate message
			if ((msg = AllocVec(sizeof(struct IntuiMessage), MEMF_CLEAR)))
			{
				// Fill out message
				msg->ExecMessage.mn_ReplyPort = GUI->rexx_app_port;
				msg->ExecMessage.mn_Length = sizeof(struct IntuiMessage);
				msg->Class = (ie->ie_Code == IECODE_NOBUTTON) ? IDCMP_MOUSEMOVE : IDCMP_MOUSEBUTTONS;
				msg->Code = ie->ie_Code;
				msg->Qualifier = ie->ie_Qualifier;
				msg->MouseX = x;
				msg->MouseY = y;
#ifdef __amigaos4__
				msg->Seconds = ie->ie_TimeStamp.Seconds;
				msg->Micros = ie->ie_TimeStamp.Microseconds;
#else
				msg->Seconds = ie->ie_TimeStamp.tv_secs;
				msg->Micros = ie->ie_TimeStamp.tv_micro;
#endif
				msg->IDCMPWindow = window;

				// Send message to the window
				PutMsg(window->UserPort, (struct Message *)msg);

				// Dispose of the event if it's a button one
				if (code != IECODE_NOBUTTON)
					DisposeCxMsg(cxm);

				// Button gone down?
				if (!(code & IECODE_UP_PREFIX) && !data->right_down)
				{
					// Store button code
					data->right_down = code | IECODE_UP_PREFIX;

					// Store window
					data->button_window = window;

					// Get qualifier for this button
					if (code == IECODE_LBUTTON)
						data->qual_down = IEQUALIFIER_LEFTBUTTON;
					else if (code == IECODE_RBUTTON)
						data->qual_down = IEQUALIFIER_RBUTTON;
					else if (code == IECODE_MBUTTON)
						data->qual_down = IEQUALIFIER_MIDBUTTON;
				}

				// Or gone up?
				else if (code == data->right_down)
				{
					struct Layer *layer;
					BOOL ok = 1;

					// Get mouse position
					x = GUI->screen_pointer->MouseX;
					y = GUI->screen_pointer->MouseY;

					// Lock layers
					if (!lock)
						LockLayerInfo(&GUI->screen_pointer->LayerInfo);

					// Find layer
					if ((layer = WhichLayer(&GUI->screen_pointer->LayerInfo, x, y)))
					{
						ULONG id;

						// Get ID of layer's window
						id = GetWindowID(layer->Window);

						// Popup menu?
						if (id == WINDOW_POPUP_MENU || id == WINDOW_START)
						{
							// Clear ok flag so we don't release button
							ok = 0;

							// Save pointer to menu window
							data->popup_window = layer->Window;
						}
					}

					// Set flag to say we locked layers
					lock = 1;

					// Ok to release button?
					if (ok)
					{
						data->right_down = 0;
						data->button_window = 0;
						data->popup_window = 0;
						data->start_popped = 0;
					}
				}
			}
		}
	}

	// Unlock layers?
	if (lock)
		UnlockLayerInfo(&GUI->screen_pointer->LayerInfo);
}

#if defined(__MORPHOS__)
static void cx_right_button_stubs(void)
{
	cx_right_button((APTR)REG_A0, (APTR)REG_A1);
}
static struct EmulLibEntry cx_right_button_trap = {TRAP_LIB, 0, (APTR)&cx_right_button_stubs};
	#define GET_CX_FUNC(name) (APTR) & name##_trap
#else
	#define GET_CX_FUNC(name) (APTR) & name
#endif

STATIC BOOL cx_over_icon(BackdropInfo *info, short x, short y)
{
	BOOL ret = 0;

	// Attempt to backdrop list
	if (!(GetSemaphore(&info->objects.lock, SEMF_SHARED | SEMF_ATTEMPT, 0)))
		return 0;

	// Over any objects?
	if (backdrop_get_object(info, x, y, 0))
		ret = 1;

	// Unlock backdrop list
	FreeSemaphore(&info->objects.lock);
	return ret;
}

// See if mouse is outside of window borders
BOOL cx_mouse_outside(struct Window *window, short x, short y)
{
	return (BOOL)(!window || x < window->BorderLeft || y < window->BorderTop ||
				  x >= window->Width - window->BorderRight || y >= window->Height - window->BorderBottom);
}

// Install pop-up filter
void cx_install_popup(CxData *cx)
{
	IX ix;

	// Initialise IX
	ix.ix_Version = IX_VERSION;
	ix.ix_Class = IECLASS_RAWKEY;

	// Valid key?
	if (environment->env->settings.general_flags & GENERALF_VALID_POPKEY)
	{
		// Fill out
		ix.ix_Code = environment->env->settings.pop_code;
		ix.ix_Qualifier = environment->env->settings.pop_qual;
		ix.ix_QualMask = environment->env->settings.pop_qual_mask;
		ix.ix_QualSame = environment->env->settings.pop_qual_same;
		ix.ix_CodeMask = 0xffff;
	}

	// Use default
	else
	{
		ix.ix_Code = 0;
		ix.ix_CodeMask = 0;
		ix.ix_Qualifier = IEQUALIFIER_LSHIFT | IEQUALIFIER_LALT | IEQUALIFIER_CONTROL;
		ix.ix_QualMask = IEQUALIFIER_LSHIFT | IEQUALIFIER_LALT | IEQUALIFIER_CONTROL;
		ix.ix_QualSame = 0;
	}

	// Create filter
	if ((cx->popup_filter = CxFilter(0)))
	{
		CxObj *obj;

		// Set IX
		SetFilterIX(cx->popup_filter, &ix);

		// Make a sender
		if ((obj = CxSender(cx->nb.nb_Port, CXCODE_POPUP)))
			AttachCxObj(cx->popup_filter, obj);

		// Make a translator
		if ((obj = CxTranslate(0)))
			AttachCxObj(cx->popup_filter, obj);

		// Add to broker
		AttachCxObj(cx->broker, cx->popup_filter);
	}

	// Install right-button filter if wanted
	if ((environment->env->settings.general_flags & GENERALF_MOUSE_HOOK) &&
		(cx->rmb_hook = CxCustom(GET_CX_FUNC(cx_right_button), 0)))
	{
		// Activate filter
		AttachCxObj(cx->broker, cx->rmb_hook);
	}
}
