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
#include "layout_routines.h"

/****************************************************************************
					   Strip IntuiMessages for a window
 ****************************************************************************/

void LIBFUNC L_StripIntuiMessagesDopus(REG(a0, struct Window *window))
{
	struct IntuiMessage *msg;
	struct Node *node;

	// Get first message
	msg = (struct IntuiMessage *)window->UserPort->mp_MsgList.lh_Head;

	// Go through messages
	while ((node = msg->ExecMessage.mn_Node.ln_Succ))
	{
		// See if this message belongs to this window
		if (msg->IDCMPWindow == window)
		{
			// Remove and reply it
			Remove((struct Node *)msg);
			ReplyMsg((struct Message *)msg);
		}

		// Get next message
		msg = (struct IntuiMessage *)node;
	}
}

void LIBFUNC L_StripWindowMessages(REG(a0, struct MsgPort *port), REG(a1, struct IntuiMessage *except))
{
	struct IntuiMessage *msg;
	struct Node *node;

	// Forbid to prevent changes
	Forbid();

	// Get first message
	msg = (struct IntuiMessage *)port->mp_MsgList.lh_Head;

	// Go through messages
	while ((node = msg->ExecMessage.mn_Node.ln_Succ))
	{
		// See if this message is the same type
		if (msg->Class == except->Class && msg != except)
		{
			// Remove and reply it
			Remove((struct Node *)msg);
			ReplyMsg((struct Message *)msg);
		}

		// Get next message
		msg = (struct IntuiMessage *)node;
	}

	// Permit now we're done
	Permit();
}

/****************************************************************************
				   Close a window using a shared IDCMP port
 ****************************************************************************/

void LIBFUNC L_CloseWindowSafely(REG(a0, struct Window *window))
{
	// Forbid
	Forbid();

	// Strip any outstanding messages
	L_StripIntuiMessagesDopus(window);

	// Clear UserPort
	window->UserPort = 0;

	// Tell Intuition to stop sending messages
	ModifyIDCMP(window, 0);

	// Permit
	Permit();

	// Close the window
	CloseWindow(window);
}

/****************************************************************************
					 Store the value of a GadTools gadget
 ****************************************************************************/

void LIBFUNC L_StoreGadgetValue(REG(a0, ObjectList *list),
								REG(a1, struct IntuiMessage *msg),
								REG(a6, struct MyLibrary *libbase))
{
#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Check for gadget message
	if (msg->Class != IDCMP_GADGETUP && msg->Class != IDCMP_GADGETDOWN && msg->Class != IDCMP_MOUSEMOVE)
		return;

	// Update value
	L_UpdateGadgetValue(list, msg, ((struct Gadget *)msg->IAddress)->GadgetID, libbase);
}

void LIBFUNC L_UpdateGadgetValue(REG(a0, ObjectList *list),
								 REG(a1, struct IntuiMessage *msg),
								 REG(d0, UWORD id),
								 REG(a6, struct MyLibrary *libbase))
{
	GL_Object *object;
	struct Gadget *gadget;

#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Look for gadget in list
	if (!(object = L_GetObject(list, id)))
		return;

	// Not a gadget?
	if (object->type != OD_GADGET)
		return;

	// Get gadget pointer
	gadget = GADGET(object);

	// Switch on gadget kind
	switch (object->object_kind)
	{
	// Button (toggle only)
	case BUTTON_KIND:
		if (gadget->Activation & GACT_TOGGLESELECT)
			object->gl_info.gl_gadget.data = (gadget->Flags & GFLG_SELECTED) ? 1 : 0;
		if (msg)
			msg->Code = object->gl_info.gl_gadget.data;
		break;

	// Radio, cycle, slider, scroller, palette and listview
	case MX_KIND:
	case CYCLE_KIND:
	case SLIDER_KIND:
	case SCROLLER_KIND:
	case PALETTE_KIND:
	case LISTVIEW_KIND:
	case MY_LISTVIEW_KIND:

		// Store current value
		if (msg)
			object->gl_info.gl_gadget.data = msg->Code;
		break;

	// Checkbox
	case CHECKBOX_KIND:

		// Get value
		if (gadget->Flags & GFLG_SELECTED)
			object->gl_info.gl_gadget.data = 1;
		else
			object->gl_info.gl_gadget.data = 0;
		if (msg)
			msg->Code = object->gl_info.gl_gadget.data;

		// Does this control something else?
		if (object->control_id != (unsigned short)-1 && msg)
		{
			// Set the disable state
			L_DisableObject(list, object->control_id, object->gl_info.gl_gadget.data);
		}
		break;

	// Integer
	case INTEGER_KIND: {
		long data, old;

		// Get value
		data = atoi(((struct StringInfo *)gadget->SpecialInfo)->Buffer);

		// Got min/max?
		if (object->gl_info.gl_gadget.choice_min != 0 || object->gl_info.gl_gadget.choice_max != 0)
		{
			// Bounds check
			old = data;
			if (data < object->gl_info.gl_gadget.choice_min)
				data = object->gl_info.gl_gadget.choice_min;
			if (data > object->gl_info.gl_gadget.choice_max)
				data = object->gl_info.gl_gadget.choice_max;

			// Update gadget if it was out of bounds
			if (data != old && msg)
			{
				// Fix value
				L_SetGadgetValue(list, object->id, data);

				// Beep error
				DisplayBeep(0);
			}
		}

		// Store data
		object->gl_info.gl_gadget.data = data;
	}
	break;

	// Glass gadget
	case FILE_BUTTON_KIND:
	case DIR_BUTTON_KIND:
	case FILE_GLASS_KIND:
	case DIR_GLASS_KIND:

		// Only work on a message
		if (msg)
		{
			char path[256];
			char file[32], *fileptr;
			GL_Object *refer;

			// Get the gadget we refer to
			if ((refer = L_GetObject(list, object->control_id)))
			{
				// Get path
				stccpy(path, ((struct StringInfo *)(GADGET(refer)->SpecialInfo))->Buffer, sizeof(path));

				// Empty path?
				if (!path[0])
				{
					// Default path specified?
					if (object->data_ptr)
						stccpy(path, (char *)object->data_ptr, sizeof(path));
				}

				// See if this is a file glass gadget
				if (object->object_kind != DIR_GLASS_KIND && object->object_kind != DIR_BUTTON_KIND)
				{
					// Split filename
					if ((fileptr = FilePart(path)))
					{
						stccpy(file, fileptr, sizeof(file));
						*fileptr = 0;
					}
				}
				else
					file[0] = 0;

				// Display file requester
				if (AslRequestTags(
						((WindowData *)list->window->UserData)->request,
						ASLFR_Window,
						list->window,
						ASLFR_TitleText,
						L_GetString(&((struct LibData *)libbase->ml_UserData)->locale,
									(object->object_kind != DIR_GLASS_KIND && object->object_kind != DIR_BUTTON_KIND)
										? MSG_SELECT_FILE
										: MSG_SELECT_DIRECTORY),
						ASLFR_InitialFile,
						(IPTR)file,
						ASLFR_InitialDrawer,
						(IPTR)path,
						ASLFR_Flags1,
						(object->flags & FILEBUTFLAG_SAVE) ? FRF_DOSAVEMODE : 0,
						ASLFR_Flags2,
						(object->object_kind != DIR_GLASS_KIND && object->object_kind != DIR_BUTTON_KIND)
							? 0
							: FRF_DRAWERSONLY,
						ASLFR_SleepWindow,
						TRUE,
						TAG_END))
				{
					// Get pathname
					stccpy(path, ((WindowData *)list->window->UserData)->request->fr_Drawer, sizeof(path));

					// Add filename
					AddPart(path, ((WindowData *)list->window->UserData)->request->fr_File, 256);

					// Pass filename to gadget
					L_SetGadgetValue(list, object->control_id, (IPTR)path);

					// Change to gadget up on this gadget
					msg->Class = IDCMP_GADGETUP;
					msg->IAddress = GADGET(refer);
				}
			}
		}
		break;

	// Font gadget
	case FONT_BUTTON_KIND:

		// Only work on message
		if (msg)
		{
			char path[256];
			short size;
			GL_Object *fontname_gad, *fontsize_gad;
			WindowData *data;

			// Get WindowData
			data = (WindowData *)list->window->UserData;

			// Get the gadgets we refer to
			if ((fontname_gad = L_GetObject(list, object->control_id)) &&
				(fontsize_gad = L_GetObject(list, object->control_id + 1)))
			{
				ULONG *pen_storage = 0;
				ULONG pen_table = 0, pen_count = 0;
				BOOL fixed_only = 0;
				char *mode_list[4];

				// Get font from gadgets
				stccpy(path, ((struct StringInfo *)(GADGET(fontname_gad)->SpecialInfo))->Buffer, sizeof(path));
				size = atoi(((struct StringInfo *)(GADGET(fontsize_gad)->SpecialInfo))->Buffer);

				// Want to do pens?
				if (object->tags)
				{
					// Get storage
					pen_storage = (ULONG *)(IPTR)GetTagData(GTCustom_FontPens, 0, object->tags);

					// Pen count and table
					pen_count = GetTagData(GTCustom_FontPenCount, 8, object->tags);
					pen_table = GetTagData(GTCustom_FontPenTable, 0, object->tags);

					// Fixed width only?
					fixed_only = GetTagData(GTCustom_FixedWidthOnly, 0, object->tags);
				}

				// Fill out mode list array
				mode_list[0] = L_GetString(&((struct LibData *)libbase->ml_UserData)->locale, MSG_FONT_DRAWMODE);
				mode_list[1] = L_GetString(&((struct LibData *)libbase->ml_UserData)->locale, MSG_FONT_TEXT);
				mode_list[2] = L_GetString(&((struct LibData *)libbase->ml_UserData)->locale, MSG_FONT_FIELD_TEXT);
				mode_list[3] = 0;

				// Empty font name?
				if (!path[0])
				{
					// Default font specified?
					if (object->data_ptr)
					{
						char *ptr;

						// Copy font path
						stccpy(path, (char *)object->data_ptr, sizeof(path));

						// Find last component
						if ((ptr = PathPart(path)))
						{
							// Get size, strip from path
							size = atoi(ptr + 1);
							*ptr = 0;
						}
					}
				}

				// Allocate font requester if needed
				if (!data->font_request &&
					!(data->font_request = AllocAslRequestTags(
						  ASL_FontRequest,
						  ASLFO_InitialHeight,
						  ((list->window->WScreen->Height) >> 3) + ((list->window->WScreen->Height) >> 1),
						  TAG_END)))
					break;

				// Display font requester
				if (AslRequestTags(data->font_request,
								   ASLFO_Window,
								   list->window,
								   ASLFO_TitleText,
								   L_GetString(&((struct LibData *)libbase->ml_UserData)->locale, MSG_SELECT_FONT),
								   ASLFO_InitialName,
								   (IPTR)path,
								   ASLFO_InitialSize,
								   size,
								   ASLFO_SleepWindow,
								   TRUE,
								   ASLFO_InitialFrontPen,
								   (pen_storage) ? ((*pen_storage) & 0xff) : 1,
								   ASLFO_InitialBackPen,
								   (pen_storage) ? (((*pen_storage) >> 8) & 0xff) : 0,
								   ASLFO_InitialDrawMode,
								   (pen_storage) ? (((*pen_storage) >> 16) & 0xff) : JAM1,
								   ASLFO_Flags,
								   (pen_storage) ? (FOF_PRIVATEIDCMP | FOF_DOFRONTPEN | FOF_DOBACKPEN | FOF_DODRAWMODE)
												 : FOF_PRIVATEIDCMP,
								   ASLFO_MaxFrontPen,
								   pen_count,
								   ASLFO_MaxBackPen,
								   pen_count,
								   ASLFO_FrontPens,
								   pen_table,
								   ASLFO_BackPens,
								   pen_table,
								   ASLFO_ModeList,
								   mode_list,
								   ASLFO_FixedWidthOnly,
								   fixed_only,
								   TAG_END))
				{
					// Set font name
					L_SetGadgetValue(list, object->control_id, (IPTR)data->font_request->fo_Attr.ta_Name);

					// Set font size
					L_SetGadgetValue(list, object->control_id + 1, data->font_request->fo_Attr.ta_YSize);

					// Want colours?
					if (pen_storage)
					{
						// Store colours
						*pen_storage = (data->font_request->fo_DrawMode << 16) | (data->font_request->fo_BackPen << 8) |
									   data->font_request->fo_FrontPen;
					}

					// Change to gadget up on this gadget
					msg->Class = IDCMP_GADGETUP;
					msg->IAddress = GADGET(fontname_gad);
				}
			}
		}
		break;
	}
}

void LIBFUNC L_UpdateGadgetList(REG(a0, ObjectList *list), REG(a6, struct MyLibrary *libbase))
{
	GL_Object *object;

#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Go through linked lists
	for (; list->node.mln_Succ; list = (ObjectList *)list->node.mln_Succ)
	{
		// Get first
		object = list->firstobject;

		// Go through objects
		while (object)
		{
			// Gadget?
			if (object->type == OD_GADGET)
			{
				// Update it
				L_UpdateGadgetValue(list, 0, object->id, libbase);
			}

			// Get next
			object = object->next;
		}
	}
}

/****************************************************************************
					  Set the value of a GadTools gadget
 ****************************************************************************/

void LIBFUNC L_SetGadgetValue(REG(a0, ObjectList *list), REG(d0, UWORD id), REG(d1, IPTR value))
{
	GL_Object *object;
	struct Gadget *gadget;

	// Get object
	if (!(object = L_GetObject(list, id)))
		return;

	// Not a gadget?
	if (object->type != OD_GADGET)
	{
		// Text/Area?
		if (object->type == OD_TEXT || object->type == OD_AREA)
		{
			// Display new text
			L_DisplayObject(list->window, object, -1, -1, (char *)value);
		}
		return;
	}

	// Get gadget
	gadget = GADGET(object);

	// Switch on type
	switch (object->object_kind)
	{
	// Button (toggle-select only)
	case BUTTON_KIND:
		if (gadget->Activation & GACT_TOGGLESELECT)
		{
			// Store number
			object->gl_info.gl_gadget.data = value;

			// Toggle display
			if (value)
				gadget->Flags |= GFLG_SELECTED;
			else
				gadget->Flags &= ~GFLG_SELECTED;
			RefreshGList(gadget, list->window, 0, 1);
		}
		break;

	// String
	case STRING_KIND:

		// Secure field?
		if (object->flags2 & OBJECTF_SECURE && value)
		{
			short a;
			char *ptr;
			struct StringInfo *info;

			// Get stringinfo pointer
			info = (struct StringInfo *)gadget->SpecialInfo;

			// Copy string to undo buffer
			stccpy(info->UndoBuffer, (char *)value, info->MaxChars);

			// Turn copy into asterisks
			for (a = 0; info->UndoBuffer[a]; a++)
				info->UndoBuffer[a] = '*';

			// Set gadget with asterisks
			SetGadgetAttrs(gadget, list->window, 0, STRINGA_TextVal, info->UndoBuffer, TAG_END);

			// Get pointer to secure field
			ptr = L_GetSecureString(gadget);

			// Copy string to real buffer
			strcpy(ptr, (char *)value);
			break;
		}

		// Set string
		SetGadgetAttrs(gadget, list->window, 0, STRINGA_TextVal, (char *)value, TAG_END);
		break;

	// Number
	case INTEGER_KIND:

		// Store number
		object->gl_info.gl_gadget.data = value;

		// Set number
		SetGadgetAttrs(gadget, list->window, 0, GTIN_Number, value, TAG_END);
		break;

	// Listview
	case LISTVIEW_KIND:
		// Store selection
		object->gl_info.gl_gadget.data = value;

		// Set selection
		GT_SetGadgetAttrs(gadget,
						  list->window,
						  0,
						  GTLV_Selected,
						  value,
						  (value != (IPTR)~0) ? GTLV_MakeVisible : TAG_IGNORE,
						  value,
						  TAG_END);
		break;

	// Custom listview
	case MY_LISTVIEW_KIND:
		// Store selection
		object->gl_info.gl_gadget.data = value;

		// Set selection
		SetGadgetAttrs(gadget,
					   list->window,
					   0,
					   DLV_Selected,
					   value,
					   (value != (IPTR)~0) ? DLV_MakeVisible : TAG_IGNORE,
					   value,
					   TAG_END);
		break;

	// Flag
	case CHECKBOX_KIND:
		// Make sure it's 0 or 1
		if (value)
			value = 1;

		// Store flag value
		object->gl_info.gl_gadget.data = value;

		// Set check state
		SetGadgetAttrs(gadget, list->window, 0, GTCB_Checked, value, TAG_END);

		// Does this control something else?
		if (object->control_id != (unsigned short)-1)
		{
			// Set the disable state
			L_DisableObject(list, object->control_id, value);
		}
		break;

	// MX
	case MX_KIND:
		// Store value
		if (value > 127)
			value = 0;
		object->gl_info.gl_gadget.data = value;

		// Set number
		GT_SetGadgetAttrs(gadget, list->window, 0, GTMX_Active, value, TAG_END);
		break;

	// Cycle
	case CYCLE_KIND:
		// Store value
		object->gl_info.gl_gadget.data = value;

		// Set number
		GT_SetGadgetAttrs(gadget, list->window, 0, GTCY_Active, value, TAG_END);
		break;

	// Slider
	case SLIDER_KIND:
		// Store value
		object->gl_info.gl_gadget.data = value;

		// Set number
		GT_SetGadgetAttrs(gadget, list->window, 0, GTSL_Level, value, TAG_END);
		break;

	// Scroller
	case SCROLLER_KIND:
		// Store value
		object->gl_info.gl_gadget.data = value;

		// Set number
		GT_SetGadgetAttrs(gadget, list->window, 0, GTSC_Top, value, TAG_END);
		break;

	// Palette
	case PALETTE_KIND:

		// Store value
		object->gl_info.gl_gadget.data = value;

		// Set selection
		SetGadgetAttrs(gadget, list->window, 0, DPG_Pen, value, TAG_END);
		break;

	// Number kind
	case NUMBER_KIND:

		// Store value
		object->gl_info.gl_gadget.data = value;

		// Change value
		SetGadgetAttrs(gadget, list->window, 0, GTNM_Number, value, TAG_END);
		break;

	// Text kind
	case TEXT_KIND:

		// Change text
		SetGadgetAttrs(gadget, list->window, 0, GTTX_Text, (value) ? value : (IPTR) "", TAG_END);
		break;

	// Default
	default:
		// Store value
		object->gl_info.gl_gadget.data = value;
		break;
	}
}

/****************************************************************************
					  Get the value of a GadTools gadget
 ****************************************************************************/

IPTR LIBFUNC L_GetGadgetValue(REG(a0, ObjectList *list), REG(a1, UWORD id), REG(a6, struct MyLibrary *libbase))
{
	GL_Object *object;

#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Look for gadget in list
	if (!(object = L_GetObject(list, id)))
		return 0;

	// Special case for string gadget
	if (object->type == OD_GADGET && object->object_kind == STRING_KIND)
	{
		// No string?
		if (!object->gl_info.gl_gadget.data)
		{
			return (IPTR)((struct LibData *)libbase->ml_UserData)->null_string;
		}

		// Secure string?
		if (object->flags2 & OBJECTF_SECURE)
			return (IPTR)L_GetSecureString(GADGET(object));
	}

	// Return data
	return object->gl_info.gl_gadget.data;
}

/****************************************************************************
				   Check a point is within an object's area
 ****************************************************************************/

BOOL LIBFUNC L_CheckObjectArea(REG(a0, GL_Object *object), REG(d0, int x), REG(d1, int y))
{
	struct IBox *pos;

	// Get pointer to position depending on object type
	if (object->type == OD_GADGET)
		pos = &object->dims;
	else if (object->type == OD_TEXT)
		pos = &object->gl_info.gl_text.text_pos;
	else if (object->type == OD_AREA)
		pos = &object->gl_info.gl_area.area_pos;
	else
		return 0;

	// Check if point is within area
	if (x >= pos->Left && x < pos->Left + pos->Width && y >= pos->Top && y < pos->Top + pos->Height)
		return 1;
	return 0;
}

/****************************************************************************
							  Make a window busy
 ****************************************************************************/

void LIBFUNC L_SetWindowBusy(REG(a0, struct Window *window))
{
	WindowData *data;

	// Valid window?
	if (!window)
		return;

	// Set busy pointer
	L_SetBusyPointer(window);
	data = (WindowData *)window->UserData;

	// Create requester if needed
	if (!(data->busy_req))
	{
		// Create requester
		if ((data->busy_req = L_AllocMemH(data->memory, sizeof(struct Requester))))
		{
			Request(data->busy_req, window);
		}
	}
}

/****************************************************************************
							  Make a window unbusy
 ****************************************************************************/

void LIBFUNC L_ClearWindowBusy(REG(a0, struct Window *window))
{
	// Valid window?
	if (!window)
		return;

	// Clear busy pointer
	ClearPointer(window);

	// Free requester if needed
	if (((WindowData *)window->UserData)->busy_req)
	{
		// End requester
		EndRequest(((WindowData *)window->UserData)->busy_req, window);
		L_FreeMemH(((WindowData *)window->UserData)->busy_req);
		((WindowData *)window->UserData)->busy_req = 0;
	}
}

/****************************************************************************
							 Get a Locale string
 ****************************************************************************/

static LONG locale_read_be_long(const UBYTE *ptr)
{
	return (LONG)(((ULONG)ptr[0] << 24) | ((ULONG)ptr[1] << 16) | ((ULONG)ptr[2] << 8) | (ULONG)ptr[3]);
}

static UWORD locale_read_be_word(const UBYTE *ptr)
{
	return (UWORD)(((UWORD)ptr[0] << 8) | (UWORD)ptr[1]);
}

STRPTR LIBFUNC L_GetString(REG(a0, struct DOpusLocale *li), REG(d0, LONG stringNum))
{
	const UBYTE *entry;
	register STRPTR builtIn;

	if (!li)
		return "";

	if (!li->li_BuiltIn)
		return "";

#define LocaleBase li->li_LocaleBase
	// Get string from locale
	if (li->li_Catalog && LocaleBase && (builtIn = (STRPTR)GetCatalogStr(li->li_Catalog, stringNum, 0)))
		return builtIn;
#undef LocaleBase

	// Get pointer to table
	entry = (const UBYTE *)li->li_BuiltIn;

	// Look for string
	while (locale_read_be_long(entry) != stringNum)
	{
		entry += locale_read_be_word(entry + sizeof(ULONG)) + sizeof(ULONG) + sizeof(UWORD);
	}

	// Return string
	builtIn = (STRPTR)(entry + sizeof(ULONG) + sizeof(UWORD));
	return builtIn;
}

/****************************************************************************
		  Scan object list looking for a key equivalent for a gadget
 ****************************************************************************/

struct Gadget *LIBFUNC L_FindKeyEquivalent(REG(a0, ObjectList *first_list),
										   REG(a1, struct IntuiMessage *msg),
										   REG(d0, int process))
{
	GL_Object *object;
	ObjectList *list;
	struct Gadget *gadget;
	int no_case;
	UWORD key;

	// Plain vanilla key?
	if (msg->Class == IDCMP_VANILLAKEY)
	{
		// Get key
		key = (msg->Code >= 'A' && msg->Code <= 'Z') ? msg->Code + ('a' - 'A') : msg->Code;
	}

	// Rawkey code
	else
	{
		// Ignore key up
		if (msg->Code & IECODE_UP_PREFIX)
			return 0;

		// Get key
		key = msg->Code;
	}

	// Do this twice, first time is case sensitive
	for (no_case = 0; no_case < 2; no_case++)
	{
		// Go through linked lists
		for (list = first_list; list->node.mln_Succ; list = (ObjectList *)list->node.mln_Succ)
		{
			// Go through the object list, looking for gadgets
			for (object = list->firstobject; object; object = object->next)
			{
				GL_Object *use_object;

				// Store object pointer
				use_object = object;

				// Null gadget pointer
				gadget = 0;

				// Gadget?
				if (use_object->type == OD_GADGET)
				{
					// Convert numeric pad keys to rawkey codes
					if (msg->Qualifier & IEQUALIFIER_NUMERICPAD)
					{
						// Change to rawkey
						msg->Class = IDCMP_RAWKEY;

						// Change code
						if (key == '7')
							key = HOME;
						else if (key == '1')
							key = END;
						else if (key == '9')
							key = PAGEUP;
						else if (key == '3')
							key = PAGEDOWN;
						else if (key == '8')
							key = CURSORUP;
						else if (key == '2')
							key = CURSORDOWN;

						// Not one of those so change back to vanilla
						else
							msg->Class = IDCMP_VANILLAKEY;
					}

					// Rawkey message?
					if (msg->Class == IDCMP_RAWKEY)
					{
						// Convert shift/cursor into page up/down
						if (key == CURSORUP && msg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
							key = PAGEUP;
						else if (key == CURSORDOWN && msg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
							key = PAGEDOWN;

						// Convert control/cursor into home/end
						else if (key == CURSORUP && msg->Qualifier & IEQUALIFIER_CONTROL)
							key = HOME;
						else if (key == CURSORDOWN && msg->Qualifier & IEQUALIFIER_CONTROL)
							key = END;

						// Up/down cursor, page up/down or home/end
						if (key == CURSORUP || key == CURSORDOWN || key == PAGEUP || key == PAGEDOWN || key == HOME ||
							key == END)
						{
							// Listview?
							if (use_object->object_kind == MY_LISTVIEW_KIND)
							{
								// Cursor key flag set?
								if (use_object->flags & LISTVIEWFLAG_CURSOR_KEYS)
								{
									// Get gadget
									gadget = GADGET(use_object);

									// Store the key we use
									msg->Code = key;
								}
						}
					}

					// Escape rawkey — look for cancel button
					else if (key == 0x45)
					{
						if (use_object->flags & BUTTONFLAG_CANCEL_BUTTON)
							gadget = GADGET(use_object);
					}
				}

				// Escape/lcommand b?
					else if (key == 0x1b || (key == 'b' && L_QualValid(msg->Qualifier) == IEQUALIFIER_LCOMMAND))
					{
						// See if gadget is a "cancel" button
						if (use_object->flags & BUTTONFLAG_CANCEL_BUTTON)
							gadget = GADGET(use_object);
					}

					// Return/space/lcommand v?
					else if (key == 0xd || key == ' ' ||
							 (key == 'v' && L_QualValid(msg->Qualifier) == IEQUALIFIER_LCOMMAND))
					{
						// See if gadget is an "ok" button
						if (use_object->flags & BUTTONFLAG_OKAY_BUTTON)
							gadget = GADGET(use_object);
					}

					// Does key match?
					else if (use_object->key == key)
					{
						// Get gadget pointer
						gadget = GADGET(use_object);
					}

					// Otherwise, valid label?
					else if (use_object->text)
					{
						int ch;

						// Go through gadget label, look for an underscore
						for (ch = 0; use_object->text[ch]; ch++)
						{
							// Underscore?
							if (use_object->text[ch] == '_')
							{
								unsigned char test = use_object->text[ch + 1];
								if (test >= 'A' && test <= 'Z')
									test += ('a' - 'A');

								// Does the next character match?
								if (key == use_object->text[ch + 1] || (no_case && key == test))
								{
									// Got a match. Get gadget pointer
									gadget = GADGET(use_object);
									break;
								}
							}
						}
					}

					// Did we get a match?
					if (gadget)
					{
						ULONG state;
						short change = 0;

						// See if gadget is disabled
						do
						{
							// Before 39, read disabled flag
							if (!(state = gadget->Flags & GFLG_DISABLED))

								// Break out if enabled
								break;

							// Is gadget a glass kind?
							if (use_object->object_kind == FILE_GLASS_KIND ||
								use_object->object_kind == FILE_BUTTON_KIND ||
								use_object->object_kind == FONT_BUTTON_KIND ||
								use_object->object_kind == DIR_BUTTON_KIND || use_object->object_kind == DIR_GLASS_KIND)
							{
								// Get the gadget we refer to
								if ((use_object = L_GetObject(list, use_object->control_id)))
									gadget = GADGET(use_object);
								else
									break;
							}

							// Otherwise fail
							else
								break;
						} while (1);

						// Disabled?
						if (state)
							continue;

						// See if we need to process this ourselves
						if (process)
						{
							// Switch on gadget kind
							switch (use_object->object_kind)
							{
							// Button
							case BUTTON_KIND:

								// Ignore repeat
								if (msg->Qualifier & IEQUALIFIER_REPEAT)
									gadget = 0;
								else
								{
									// Toggle-select?
									if (gadget->Activation & GACT_TOGGLESELECT)
									{
										// Toggle select
										gadget->Flags ^= GFLG_SELECTED;
										RefreshGList(gadget, list->window, 0, 1);

										// Store value
										msg->Code = (gadget->Flags & GFLG_SELECTED) ? 1 : 0;
										object->gl_info.gl_gadget.data = msg->Code;
									}

									// Highlight gadget
									else
									{
										gadget->Flags |= GFLG_SELECTED;
										RefreshGList(gadget, list->window, 0, 1);
										Delay(5);
										gadget->Flags &= ~GFLG_SELECTED;
										RefreshGList(gadget, list->window, 0, 1);
									}
								}
								break;

							// Glass
							case FILE_GLASS_KIND:
							case FILE_BUTTON_KIND:
							case FONT_BUTTON_KIND:
							case DIR_BUTTON_KIND:
							case DIR_GLASS_KIND:

								// Ignore repeat
								if (msg->Qualifier & IEQUALIFIER_REPEAT)
									gadget = 0;
								else
								{
									gadget->Flags |= GFLG_SELECTED;
									RefreshGList(gadget, list->window, 0, 1);
									Delay(5);
									gadget->Flags &= ~GFLG_SELECTED;
									RefreshGList(gadget, list->window, 0, 1);
								}
								break;

							// String and integer
							case STRING_KIND:
							case INTEGER_KIND:

								// Activate gadget
								L_ActivateStrGad(gadget, list->window);
								change = 1;
								break;

							// Checkbox
							case CHECKBOX_KIND:
								// Ignore repeat
								if (msg->Qualifier & IEQUALIFIER_REPEAT)
									gadget = 0;
								else
								{
									short status;

									// Get status
									status = !(gadget->Flags & GFLG_SELECTED);

									// Toggle gadget status
									SetGadgetAttrs(gadget, list->window, 0, GTCB_Checked, status, TAG_END);

									// Store status in code
									msg->Code = status;
								}
								break;

							// Palette, cycle, scroller, listview, mx
							case CYCLE_KIND:
								// Ignore repeat
								if (msg->Qualifier & IEQUALIFIER_REPEAT)
								{
									gadget = 0;
									break;
								}
							case LISTVIEW_KIND:
							case SCROLLER_KIND:
							case SLIDER_KIND:
							case MX_KIND:

								// Is the number of choices known?
								if (use_object->gl_info.gl_gadget.choice_max > 0)
								{
									short min, max;
									long sel, old;
									Tag tag = TAG_IGNORE, tag2 = TAG_IGNORE;

									// Get old selection number
									old = sel = use_object->gl_info.gl_gadget.data;

									// Get new selection number
									if (msg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
										--sel;
									else
										++sel;

									// Minimum/Maximum
									min = use_object->gl_info.gl_gadget.choice_min;
									max = use_object->gl_info.gl_gadget.choice_max - 1;

									// Handle wrap for use_object kind
									if (sel < min)
									{
										sel = (use_object->object_kind == CYCLE_KIND) ? max : min;
									}
									else if (sel > max)
									{
										sel = (use_object->object_kind == CYCLE_KIND) ? min : max;
									}

									// Has value changed?
									if (sel != old)
									{
										// Get tag to use
										if (use_object->object_kind == CYCLE_KIND)
											tag = GTCY_Active;
										else if (use_object->object_kind == LISTVIEW_KIND)
										{
											tag = GTLV_Selected;
											if (GadToolsBase->lib_Version >= 39)
												tag2 = GTLV_MakeVisible;
											else
												tag2 = GTLV_Top;
										}
										else if (use_object->object_kind == MX_KIND)
											tag = GTMX_Active;
										else if (use_object->object_kind == SCROLLER_KIND)
											tag = GTSC_Top;
										else if (use_object->object_kind == SLIDER_KIND)
											tag = GTSL_Level;

										// Change selection
										GT_SetGadgetAttrs(gadget, list->window, 0, tag, sel, tag2, sel, TAG_END);

										// Store selection in message code
										msg->Code = sel;
									}
									else
										gadget = 0;
								}
								break;

							// Custom listview
							case MY_LISTVIEW_KIND: {
								Tag tag = DLV_SelectNext;
								IPTR sel, old;

								// Pressed key equivalent?
								if (msg->Class == IDCMP_VANILLAKEY)
								{
									// With shift means previous
									if (msg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
										tag = DLV_SelectPrevious;
								}

								// Pressed cursor key?
								else if (msg->Class == IDCMP_RAWKEY)
								{
									// Page up?
									if (msg->Code == PAGEUP)
										tag = DLV_PageUp;

									// Home?
									else if (msg->Code == HOME)
										tag = DLV_Home;

									// Page down
									else if (msg->Code == PAGEDOWN)
										tag = DLV_PageDown;

									// End
									else if (msg->Code == END)
										tag = DLV_End;

									// Line up
									else if (msg->Code == CURSORUP)
										tag = DLV_SelectPrevious;
								}

								// Get old selection
								GetAttr(DLV_Selected, (Object *)gadget, &old);

								// Change selection
								SetGadgetAttrs(gadget, list->window, 0, tag, 1, TAG_END);

								// Get current selection
								GetAttr(DLV_Selected, (Object *)gadget, &sel);

								// Has it changed?
								if (sel != old)
								{
									// Make it visible
									SetGadgetAttrs(gadget, list->window, 0, DLV_MakeVisible, sel, TAG_END);

									// Store selection in message code
									msg->Code = (UWORD)sel;
								}
								else
									gadget = 0;
							}
							break;

							// Palette
							case PALETTE_KIND: {
								Tag tag;
								IPTR sel;

								// Select previous?
								if ((msg->Class == IDCMP_RAWKEY && msg->Code == CURSORUP) ||
									(msg->Class == IDCMP_VANILLAKEY &&
									 msg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)))
									tag = DPG_SelectPrevious;
								else
									tag = DPG_SelectNext;

								// Change selection
								SetGadgetAttrs(gadget, list->window, 0, tag, 1, TAG_END);

								// Get selection
								GetAttr(DPG_Pen, (Object *)gadget, &sel);
								msg->Code = (UWORD)sel;
							}
							break;
							}
						}

						// Still valid?
						if (gadget)
						{
							// Turn IntuiMessage into a gadget message
							if (change == 0)
							{
								if (use_object->object_kind == MX_KIND)
									msg->Class = IDCMP_GADGETDOWN;
								else
									msg->Class = IDCMP_GADGETUP;
								msg->IAddress = gadget;
							}
						}

						// Return the gadget pointer
						return gadget;
					}
				}
			}
		}
	}

	// Not found
	return 0;
}

/****************************************************************************
					 Do a progress bar in an area object
 ****************************************************************************/

void LIBFUNC L_ShowProgressBar(REG(a0, struct Window *window),
							   REG(a1, GL_Object *object),
							   REG(d0, ULONG total),
							   REG(d1, ULONG count))
{
}

/****************************************************************************
						 Change the kind of an object
 ****************************************************************************/

void LIBFUNC L_SetObjectKind(REG(a0, ObjectList *list), REG(d0, ULONG id), REG(d1, UWORD kind))
{
	GL_Object *object;

	// Get object
	if (!(object = L_GetObject(list, id)))
		return;

	// Change kind
	object->object_kind = kind;
}

/****************************************************************************
						 Disable or enable an object
 ****************************************************************************/

void LIBFUNC L_DisableObject(REG(a0, ObjectList *list), REG(d0, ULONG id), REG(d1, BOOL state))
{
	GL_Object *object;

	// Get object
	if (!(object = L_GetObject(list, id)))
		return;

	// Must be a gadget (for now)
	if (object->type == OD_GADGET)
	{
		// GadTools?
		if (IS_GADTOOLS(object))
		{
			GT_SetGadgetAttrs(GADGET(object), list->window, 0, GA_Disabled, state, TAG_END);
		}

		// Boopsi
		else
		{
			SetGadgetAttrs(GADGET(object), list->window, 0, GA_Disabled, state, TAG_END);
		}
	}
}

/****************************************************************************
						Bounds check an integer gadget
 ****************************************************************************/

int LIBFUNC L_BoundsCheckGadget(REG(a0, ObjectList *list), REG(d0, ULONG id), REG(d1, int min), REG(d2, int max))
{
	GL_Object *object;
	int val = 0;

	// Get object
	if (!(object = L_GetObject(list, id)))
		return 0;

	// Must be an integer gadget
	if (object->type == OD_GADGET && object->object_kind == INTEGER_KIND)
	{
		// Get current value of gadget
		val = object->gl_info.gl_gadget.data;

		// Bounds check
		if (val < min || val > max)
		{
			// Fix value
			if (val < min)
				val = min;
			else
				val = max;

			// Refresh gadget
			L_SetGadgetValue(list, id, val);
		}
	}

	// Return value
	return val;
}

/****************************************************************************
						Get a message from the window
 ****************************************************************************/

struct IntuiMessage *LIBFUNC L_GetWindowMsg(REG(a0, struct MsgPort *port), REG(a6, struct MyLibrary *lib))
{
	struct IntuiMessage *msg;

	// Get message
	while ((msg = GT_GetIMsg(port)))
	{
		// Is it a refresh message?
		if (msg->Class == IDCMP_REFRESHWINDOW)
		{
			// Strip other refresh messages
			L_StripWindowMessages(port, msg);

			// See if window is auto refresh
			if (WINFLAG(msg->IDCMPWindow) & WINDOW_AUTO_REFRESH)
			{
				// Refresh the window
				L_StartRefreshConfigWindow(msg->IDCMPWindow, TRUE);

				// Reply message
				GT_ReplyIMsg(msg);

				// Loop back for another message
				continue;
			}
		}

		// A resize message?
		else if (msg->Class == IDCMP_NEWSIZE)
		{
			// Strip other resize messages
			L_StripWindowMessages(port, msg);

			// Handle re-size
			L_LayoutResize(msg->IDCMPWindow);
		}

		// Otherwise
		else
		{
			// Is it a key message?
			if (msg->Class == IDCMP_VANILLAKEY || msg->Class == IDCMP_RAWKEY)
			{
				// See if window is auto key
				if (WINFLAG(msg->IDCMPWindow) & WINDOW_AUTO_KEYS)
				{
					// See if we can match a gadget
					L_FindKeyEquivalent(OBJLIST(msg->IDCMPWindow), msg, TRUE);
				}
			}

			// A gadget message?
			if (msg->Class == IDCMP_GADGETUP || msg->Class == IDCMP_GADGETDOWN)
			{
				// Update gadget values
				L_StoreGadgetValue(OBJLIST(msg->IDCMPWindow), msg, lib);
			}
		}

		// Break out; we only loop if we swallow a message
		break;
	}

	// Return message
	return msg;
}

/****************************************************************************
						  Reply to a window message
 ****************************************************************************/

void LIBFUNC L_ReplyWindowMsg(REG(a0, struct IntuiMessage *msg))
{
	GT_ReplyIMsg(msg);
}

/****************************************************************************
						Set the "choices" for a gadget
 ****************************************************************************/

void LIBFUNC L_SetGadgetChoices(REG(a0, ObjectList *list), REG(d0, ULONG id), REG(a1, APTR choices))
{
	GL_Object *object;
	struct Gadget *gadget;

	// Find object
	if (!(object = L_GetObject(list, id)))
		return;

	// Get gadget pointer
	if (object->type != OD_GADGET || !(gadget = GADGET(object)))
		return;

	// Look at gadget kind
	switch (object->object_kind)
	{
	// Scroller
	case SCROLLER_KIND: {
		unsigned long val;

		// Get value as a long
		val = (unsigned long)choices;

		// Store maximum value
		object->gl_info.gl_gadget.choice_max = (unsigned short)(val & 0xffff);

		// Check current value
		if (object->gl_info.gl_gadget.data >= object->gl_info.gl_gadget.choice_max)
			object->gl_info.gl_gadget.data = object->gl_info.gl_gadget.choice_max - 1;

		// Change gadget settings
		GT_SetGadgetAttrs(gadget,
						  list->window,
						  0,
						  GTSC_Total,
						  (unsigned long)choices,
						  (val & 0xffff0000) ? GTSC_Visible : TAG_IGNORE,
						  (unsigned long)((val >> 16) & 0xffff),
						  TAG_END);
	}
	break;

	// Slider
	case SLIDER_KIND: {
		unsigned short max, min;

		// Get max/min values
		max = ((IPTR)choices) >> 16;
		min = ((IPTR)choices) & 0xffff;

		// Store values
		object->gl_info.gl_gadget.choice_min = min;
		object->gl_info.gl_gadget.choice_max = max + 1;

		// Check current value
		if (object->gl_info.gl_gadget.data >= object->gl_info.gl_gadget.choice_max)
			object->gl_info.gl_gadget.data = object->gl_info.gl_gadget.choice_max - 1;
		if (object->gl_info.gl_gadget.data < object->gl_info.gl_gadget.choice_min)
			object->gl_info.gl_gadget.data = object->gl_info.gl_gadget.choice_min;

		// Change gadget settings
		GT_SetGadgetAttrs(gadget, list->window, 0, GTSL_Min, min, GTSL_Max, max, TAG_END);
	}
	break;

	// Listview
	case LISTVIEW_KIND:

		// ~0/0 means remove list
		if (choices == 0)
		{
			object->gl_info.gl_gadget.choice_max = 0;
			object->gl_info.gl_gadget.data = -1;
		}
		else if (choices == (APTR)~0)
			object->gl_info.gl_gadget.choice_max = 0;

		// Otherwise, count items in list
		else
		{
			// Count items
			object->gl_info.gl_gadget.choice_max = L_Att_NodeCount((Att_List *)choices);

			// Check current value
			if (object->gl_info.gl_gadget.data >= object->gl_info.gl_gadget.choice_max)
				object->gl_info.gl_gadget.data = object->gl_info.gl_gadget.choice_max - 1;
		}

		// Attach labels to listview
		GT_SetGadgetAttrs(gadget, list->window, 0, GTLV_Labels, (IPTR)choices, TAG_END);
		break;

	// Custom listview
	case MY_LISTVIEW_KIND:
		SetGadgetAttrs(gadget, list->window, 0, DLV_Labels, (IPTR)choices, TAG_END);
		break;

	// Cycle gadget
	case CYCLE_KIND:

		// 0 means remove list
		if (choices == 0)
		{
			object->gl_info.gl_gadget.choice_max = 0;
			object->gl_info.gl_gadget.data = -1;
		}

		// Otherwise, count items in array
		else
		{
			char **array = (char **)choices;

			// Count items
			for (object->gl_info.gl_gadget.choice_max = 0; array[object->gl_info.gl_gadget.choice_max];
				 object->gl_info.gl_gadget.choice_max++)
				;

			// Check current value
			if (object->gl_info.gl_gadget.data >= object->gl_info.gl_gadget.choice_max)
				object->gl_info.gl_gadget.data = object->gl_info.gl_gadget.choice_max - 1;
		}

		// Attach labels to gadget
		GT_SetGadgetAttrs(gadget, list->window, 0, GTCY_Labels, (IPTR)choices, TAG_END);
		break;
	}
}

/****************************************************************************
						   Initialise a window's ID
 ****************************************************************************/

void LIBFUNC L_SetWindowID(REG(a0, struct Window *window),
						   REG(a1, WindowID *id),
						   REG(d0, ULONG window_id),
						   REG(a2, struct MsgPort *port))
{
	// Valid window?
	if (!window)
		return;

	// If no ID pointer supplied, assume it's in UserData
	if (!id)
		id = (WindowID *)window->UserData;

	// Otherwise store it in UserData
	else
		window->UserData = (APTR)id;

	// Valid ID pointer?
	if (!id)
		return;

	// Fill out fields
	id->magic = WINDOW_MAGIC;
	id->window = window;
	id->window_id = window_id;
	id->app_port = port;
}

/****************************************************************************
							  Get a window's ID
 ****************************************************************************/

ULONG LIBFUNC L_GetWindowID(REG(a0, struct Window *window))
{
	WindowID *id;
	STRPTR name = NULL;

	// Valid window?
	if (!window)
		return WINDOW_UNKNOWN;

#ifdef amigaos4
	if (!GetWindowAttr(window, WA_WindowName, &name, sizeof(name)))
		return WINDOW_UNKNOWN;
	if (name == NULL)
		return WINDOW_UNKNOWN;
	if (name[0] == '\0')
		return WINDOW_UNKNOWN;
	if (strcmp(DOPUS_WIN_NAME, name) != 0)
		return WINDOW_UNKNOWN;
#endif

	// Get ID pointer
	if (!(id = (WindowID *)window->UserData) || id < (WindowID *)0x1000 || TypeOfMem(id) == 0)
		return WINDOW_UNKNOWN;

	// Check ID is valid
	if (id->magic != WINDOW_MAGIC || id->window != window)
		return WINDOW_UNKNOWN;

	// Return ID code
	return id->window_id;
}

/****************************************************************************
					   Get a window's application port
 ****************************************************************************/

struct MsgPort *LIBFUNC L_GetWindowAppPort(REG(a0, struct Window *window))
{
	WindowID *id;
	STRPTR name = NULL;

	// Valid window?
	if (!window)
		return 0;

#ifdef amigaos4
	if (!GetWindowAttr(window, WA_WindowName, &name, sizeof(name)))
		return 0;
	if (name == NULL)
		return 0;
	if (name[0] == '\0')
		return 0;
	if (strcmp(DOPUS_WIN_NAME, name) != 0)
		return 0;
#endif

	// Get ID pointer
	if (!(id = (WindowID *)window->UserData))
		return 0;

	// Check ID is valid
	if (id->magic != WINDOW_MAGIC || id->window != window)
		return 0;

	// Return app port
	return id->app_port;
}

/****************************************************************************
						  Get an objects's rectangle
 ****************************************************************************/

BOOL LIBFUNC L_GetObjectRect(REG(a0, ObjectList *list), REG(d0, ULONG id), REG(a1, struct Rectangle *rect))
{
	GL_Object *object;
	struct IBox box;

	// Get object
	if (!(object = L_GetObject(list, id)))
		return 0;

	// Look at object type
	switch (object->type)
	{
	// Gadget
	case OD_GADGET:
		box = *((struct IBox *)&object->gl_info.gl_gadget.gadget->LeftEdge);
		break;

	// Text
	case OD_TEXT:
		box = object->gl_info.gl_text.text_pos;
		break;

	// Area
	case OD_AREA:
		box = object->gl_info.gl_area.area_pos;
		break;

	default:
		return 0;
	}

	// Convert to rectangle
	rect->MinX = box.Left;
	rect->MinY = box.Top;
	rect->MaxX = box.Left + box.Width - 1;
	rect->MaxY = box.Top + box.Height - 1;
	return 1;
}

// Refresh a window
void LIBFUNC L_StartRefreshConfigWindow(REG(a0, struct Window *window), REG(d0, long final_state))
{
#ifndef BACKFILL
	// Two-pass refresh for back-fill
	if (WINFLAG(window) & WINDOW_REQ_FILL)
	{
		// Lock layers
		LockLayerInfo(&window->WScreen->LayerInfo);

		// Begin refresh pass 1
		GT_BeginRefresh(window);

		// Back-fill window
		_req_backfill(window);

		// End pass 1
		GT_EndRefresh(window, FALSE);
	}
#endif

	// Begin second pass
	GT_BeginRefresh(window);

	// Refresh objects
	L_RefreshObjectList(window, 0);

	// End refresh?
	if (final_state)
	{
		// End it
		L_EndRefreshConfigWindow(window);
	}
}

void LIBFUNC L_EndRefreshConfigWindow(REG(a0, struct Window *window))
{
#ifndef BACKFILL
	// Extra stuff for back fill
	if (WINFLAG(window) & WINDOW_REQ_FILL)
	{
		// Refresh gadgets
		RefreshGadgets(window->FirstGadget, window, 0);
		GT_RefreshWindow(window, 0);

		// Lock layers
		UnlockLayerInfo(&window->WScreen->LayerInfo);
	}
#endif

	// End refresh
	GT_EndRefresh(window, TRUE);
}
