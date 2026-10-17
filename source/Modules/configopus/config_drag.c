#include "config_lib.h"

void config_drag_init(CfgDragInfo *drag);

// Check for drag-deadlocks
BOOL config_drag_check(CfgDragInfo *drag)
{
	BOOL ret = 0;

	// Not dragging anything?
	if (!drag->drag)
		return 0;

	// Check for timer messages
	if (CheckTimer(drag->timer))
	{
		// Check for deadlocks
		if (drag->tick_count == drag->last_tick)
		{
			// Locked!
			ret = 1;
		}

		// If none, restart timer
		else
		{
			StartTimer(drag->timer, 0, 500000);
			drag->last_tick = drag->tick_count;
		}
	}

	return ret;
}

// Handle mouse movement
void config_drag_move(CfgDragInfo *drag)
{
	// Dragging something?
	if (drag->drag)
	{
		// Show drag image
		ShowDragImage(drag->drag,
					  ((!drag->flags) ? drag->window->MouseX : drag->window->WScreen->MouseX) + drag->drag_x,
					  ((!drag->flags) ? drag->window->MouseY : drag->window->WScreen->MouseY) + drag->drag_y);
	}
}

// Start drag from a list
void config_drag_start(CfgDragInfo *drag, Att_List *list, short item, struct TagItem *tags, BOOL global)
{
	short width, height;
	ListViewDraw draw;
	IPTR ptr;

	// Find node
	if (!(drag->drag_node = Att_FindNode(list, item)))
		return;

	// Initialise flags
	drag->flags = global;

	// Get rectangle size
	width = GetTagData(GA_Width, 0, tags);
	height = GetTagData(GA_Height, 0, tags);

	// Allocate DragInfo
	if (!(drag->drag = GetDragInfo(
			  drag->window, (global) ? 0 : drag->window->RPort, width, height, DRAGF_NEED_GELS | DRAGF_CUSTOM)))
		return;

	// Fill out draw packet
	draw.rp = &drag->drag->drag_rp;
	draw.drawinfo = DRAWINFO(drag->window);
	draw.node = (struct Node *)drag->drag_node;
	draw.line = 0;
	draw.box.Left = 0;
	draw.box.Top = 0;
	draw.box.Width = width;
	draw.box.Height = height;

	// Set flag for opaque drawing
	drag->drag->flags |= DRAGF_OPAQUE | ((global) ? 0 : DRAGF_NO_LOCK);

	// Draw image
	ptr = (IPTR)&draw;
	GetAttr(DLV_DrawLine, (Object *)GetTagData(DLV_Object, 0, tags), &ptr);

	// Build drag mask
	GetDragMask(drag->drag);

	// Get drag offsets
	drag->drag_x = -GetTagData(GA_RelRight, width >> 1, tags);
	drag->drag_y = -GetTagData(GA_RelBottom, height >> 1, tags);

	// Initialise drag
	config_drag_init(drag);
}

void config_drag_init(CfgDragInfo *drag)
{
	// Show initial drag image
	config_drag_move(drag);

	// Save old flags
	drag->old_flags = drag->window->Flags;
	drag->old_idcmp = drag->window->IDCMPFlags;

	// Set mousemove reporting and ticks
	drag->window->Flags |= WFLG_REPORTMOUSE | WFLG_RMBTRAP;
	drag->window->Flags &= ~WFLG_WINDOWTICKED;
	ModifyIDCMP(drag->window, drag->window->IDCMPFlags | IDCMP_INTUITICKS);

	// Start deadlock timer
	StartTimer(drag->timer, 0, 500000);

	// Initialise tick count
	drag->tick_count = 1;
	drag->last_tick = 0;
}

// Start drag from a window
void config_drag_start_window(CfgDragInfo *drag,
							  struct Window *window,
							  struct Rectangle *rect,
							  short mousex,
							  short mousey)
{
	// Initialise node and flags
	drag->drag_node = 0;
	drag->flags = 1;

	// Allocate drag info
	if (!(drag->drag = GetDragInfo(
			  window, 0, (rect->MaxX - rect->MinX) + 1, (rect->MaxY - rect->MinY) + 1, DRAGF_NEED_GELS | DRAGF_CUSTOM)))
		return;

	// Set opaque flag
	drag->drag->flags |= DRAGF_OPAQUE;

	// Get image to drag
	GetDragImage(drag->drag, rect->MinX, rect->MinY);

	// Get drag offset
	drag->drag_x = rect->MinX - mousex;
	drag->drag_y = rect->MinY - mousey;

	// Initialise drag
	config_drag_init(drag);
}

// End a drag
struct Window *config_drag_end(CfgDragInfo *drag, short ok)
{
	// Free drag stuff
	FreeDragInfo(drag->drag);
	drag->drag = 0;

	// Restore flags in window
	if (drag->window)
	{
		drag->window->Flags = drag->old_flags;
		ModifyIDCMP(drag->window, drag->old_idcmp);
	}

	// Abort timer
	StopTimer(drag->timer);

	// Ok to look for window?
	if (ok)
	{
		struct Layer *layer;

		// Lock LayerInfo
		LockLayerInfo(&drag->window->WScreen->LayerInfo);

		// Find which layer we dropped on
		if ((layer = WhichLayer(&drag->window->WScreen->LayerInfo, drag->drag_x, drag->drag_y)))
		{
			// Get window pointer
			if (layer->Window)
			{
				// Leave layers locked
				return (struct Window *)layer->Window;
			}
		}
	}

	// Not found a window
	return 0;
}

// Send a button to another process
BOOL config_drag_send_button(CfgDragInfo *drag, IPCData *ipc, Cfg_Button *button, Cfg_ButtonFunction *func)
{
	Point *pos;
	Cfg_Button *send_button;

	// Allocate position field
	if ((pos = AllocVec(sizeof(Point), 0)))
	{
		// Store screen-relative position
		pos->x = drag->drag_x;
		pos->y = drag->drag_y;
	}

	// Get button to send
	if ((send_button = config_drag_get_button(button, func)))
	{
		IPC_Command(ipc, BUTTONEDIT_CLIP_BUTTON, 0, send_button, pos, 0);
		return 1;
	}

	// Failed
	FreeVec(pos);
	return 0;
}

// Get button to send somewhere
Cfg_Button *config_drag_get_button(Cfg_Button *button, Cfg_ButtonFunction *func)
{
	Cfg_Button *send_button;

	// Copy the whole button?
	if (!func)
	{
		// Copy button
		send_button = CopyButton(button, 0, 0);
	}

	// Just the function, make a new button
	else if ((send_button = NewButtonWithFunc(0, 0, FTYPE_LEFT_BUTTON)))
	{
		// Copy function into new one
		CopyButtonFunction(func, 0, (Cfg_ButtonFunction *)send_button->function_list.mlh_Head);

		// Set type
		((Cfg_ButtonFunction *)send_button->function_list.mlh_Head)->function.func_type = FTYPE_LEFT_BUTTON;
	}

	return send_button;
}
