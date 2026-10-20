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
#define BOOPSI_LIBS
#include "boopsi.h"

// Image dispatcher
IPTR LIBFUNC image_dispatch(REG(a0, Class *cl), REG(a2, Object *obj), REG(a1, Msg msg))
{
	BoopsiImageData *data = 0;
	struct Image *image = 0;
	IPTR retval = 0;

	// Get gadget and data pointers
	if (obj)
	{
		image = (struct Image *)obj;
		data = INST_DATA(cl, obj);
	}

	// Look at method
	switch (msg->MethodID)
	{
	// Create a new instance
	case OM_NEW:

		// Create superclass instance
		if ((retval = DoSuperMethodA(cl, obj, msg)))
		{
			struct TagItem *tags;

			// Get pointer to our instance data
			data = INST_DATA(cl, (APTR)(IPTR)retval);

			// Get taglist
			tags = ((struct opSet *)msg)->ops_AttrList;

			// Initialise data
			data->type = GetTagData(DIA_Type, 0, tags);
			data->fpen = GetTagData(DIA_FrontPen, -1, tags);
			data->data = ((BoopsiLibs *)cl->cl_Dispatcher.h_Data)->data;
			data->flags = 0;
			if (GetTagData(DIA_ThinBorders, 0, tags))
				data->flags |= BIF_THIN_BORDERS;
		}
		break;

	// Render image
	case IM_DRAW:
	case IM_DRAWFRAME:
		image_draw(cl, image, data, (struct impDraw *)msg);
		break;

	// Unknown method
	default:
		retval = DoSuperMethodA(cl, obj, msg);
		break;
	}

	return retval;
}

// Draw image
void image_draw(Class *cl, struct Image *image, BoopsiImageData *data, struct impDraw *draw)
{
	struct RastPort *rp;
	UWORD *pens;
	struct IBox box;
	Point point[4];
	short p;

	// Get rastport to use
	rp = draw->imp_RPort;

	// Get pen array
	pens = draw->imp_DrInfo->dri_Pens;

	// Get image box
	box.Left = image->LeftEdge + draw->imp_Offset.X;
	box.Top = image->TopEdge + draw->imp_Offset.Y;
	box.Width = image->Width;
	box.Height = image->Height;

	// Look at type
	switch (data->type)
	{
	// Arrow
	case IM_ARROW_UP:
	case IM_ARROW_DOWN:

		// Draw border around the box
		listview_border(cl, rp, pens, &box, draw->imp_State, (data->flags & BIF_THIN_BORDERS) ? THIN : THICK);

		// Fill background
		SetAPen(rp, pens[(draw->imp_State == IDS_SELECTED) ? FILLPEN : BACKGROUNDPEN]);
		RectFill(rp,
				 box.Left + ((data->flags & BIF_THIN_BORDERS) ? 1 : 2),
				 box.Top + 1,
				 box.Left + box.Width - ((data->flags & BIF_THIN_BORDERS) ? 2 : 3),
				 box.Top + box.Height - 2);

		// Calculate image points
		point[0].x = 4;
		point[0].y = (data->type == IM_ARROW_UP) ? image->Height - 3 : 2;
		point[1].x = image->Width >> 1;
		if ((image->Width % 2) == 0)
		{
			point[2].x = point[1].x;
			--point[1].x;
		}
		else
		{
			point[2].x = point[1].x;
		}
		point[1].y = (data->type == IM_ARROW_UP) ? 2 : image->Height - 3;
		point[2].y = point[1].y;
		point[3].x = image->Width - 5;
		point[3].y = point[0].y;

		// Draw arrow
		SetAPen(rp, pens[(draw->imp_State == IDS_SELECTED) ? FILLTEXTPEN : TEXTPEN]);
		Move(rp, point[0].x + box.Left, point[0].y + box.Top);
		for (p = 1; p < 3; p++)
			Draw(rp, point[p].x + box.Left, point[p].y + box.Top);
		Move(rp, point[3].x + box.Left, point[3].y + box.Top);
		Draw(rp, point[2].x + box.Left, point[2].y + box.Top);

		Move(rp, point[0].x + box.Left + 1, point[0].y + box.Top);
		Draw(rp, point[1].x + box.Left + 1, point[1].y + box.Top);
		Draw(rp, point[2].x + box.Left - 1, point[2].y + box.Top);
		Move(rp, point[3].x + box.Left - 1, point[3].y + box.Top);
		Draw(rp, point[2].x + box.Left - 1, point[2].y + box.Top);
		break;

	// Checkmark
	case IM_CHECK:
	case IM_CROSS:

		// Clear background first (prevents progressive bolding with anti-aliased rendering)
		SetAPen(rp, pens[(draw->imp_State == IDS_SELECTED) ? FILLPEN : BACKGROUNDPEN]);
		RectFill(rp, box.Left, box.Top, box.Left + box.Width - 1, box.Top + box.Height - 1);

		// Draw checkmark
		SetAPen(rp, (data->fpen == -1) ? pens[TEXTPEN] : data->fpen);
		SetDrMd(rp, JAM1);

		Move(rp, box.Left, box.Top + (box.Height >> 1));
		Draw(rp, box.Left + (box.Width >> 2), box.Top + box.Height - 1);
		Draw(rp, box.Left + (box.Width >> 1) - 1, box.Top + box.Height - 1);
		Draw(rp, box.Left + box.Width - 2, box.Top);
		Draw(rp, box.Left + box.Width - 1, box.Top);

		Move(rp, box.Left + 1, box.Top + (box.Height >> 1));
		Draw(rp, box.Left + (box.Width >> 2), box.Top + box.Height - 2);
		Draw(rp, box.Left + (box.Width >> 1) - 1, box.Top + box.Height - 2);
		Draw(rp, box.Left + box.Width - 3, box.Top);

		Move(rp, box.Left + 2, box.Top + (box.Height >> 1));
		Draw(rp, box.Left + (box.Width >> 2), box.Top + box.Height - 3);
		break;

		/*
				// Cross
				case IM_CROSS:
					{
						struct Rectangle rect;

						// Convert box to rectangle
						rect.MinX=box.Left;
						rect.MinY=box.Top;
						rect.MaxX=box.Left+box.Width-1;
						rect.MaxY=box.Top+box.Height-1;

						// Set pen
						SetAPen(rp,(data->fpen==-1)?pens[TEXTPEN]:data->fpen);
						SetDrMd(rp,JAM1);

						// Draw top-left to bottom-right
						Move(rp,rect.MinX,rect.MinY);
						Draw(rp,rect.MaxX,rect.MaxY);
						Move(rp,rect.MinX,rect.MinY+1);
						Draw(rp,rect.MaxX-1,rect.MaxY);

						// Draw top-right to bottom-left
						Move(rp,rect.MaxX,rect.MinY);
						Draw(rp,rect.MinX,rect.MaxY);
						Move(rp,rect.MaxX,rect.MinY+1);
						Draw(rp,rect.MinX+1,rect.MaxY);
					}
					break;
		*/

	// Drawer image
	case IM_DRAWER: {
		Point drawer_points[12];
		unsigned short x, y, w, h, a;

		// Get size and x/y offset
		w = box.Width - 12;
		h = box.Height - 9;
		x = box.Left + 6;
		y = box.Top + box.Height - 4;

		// Fill in point array
		drawer_points[0].x = w - 1;
		drawer_points[0].y = 0;
		drawer_points[1].x = w - 1;
		drawer_points[1].y = -h;
		drawer_points[2].x = w - 3;
		drawer_points[2].y = -h - 2;
		drawer_points[3].x = w - 6;	 //
		drawer_points[3].y = -h - 2;
		drawer_points[4].x = 4;
		drawer_points[4].y = -h;
		drawer_points[5].x = 0;
		drawer_points[5].y = -h;
		drawer_points[6].x = 0;
		drawer_points[6].y = -1;
		drawer_points[7].x = 1;
		drawer_points[7].y = -1;
		drawer_points[8].x = 1;
		drawer_points[8].y = -h + 1;
		drawer_points[9].x = 5;
		drawer_points[9].y = -h + 1;
		drawer_points[10].x = w - 6;  //
		drawer_points[10].y = -h + 2;
		drawer_points[11].x = w - 2;
		drawer_points[11].y = -h + 2;

		// Draw image
		SetAPen(rp, pens[TEXTPEN]);
		SetDrMd(rp, JAM1);
		Move(rp, x, y);
		for (a = 0; a < 12; a++)
			Draw(rp, x + drawer_points[a].x, y + drawer_points[a].y);
	}
	break;

	// Box with a border
	case IM_BBOX:

		// If this is a DRAWFRAME, fix size
		if (draw->MethodID == IM_DRAWFRAME)
		{
			box.Width = draw->imp_Dimensions.Width;
			box.Height = draw->imp_Dimensions.Height;
		}

		// Draw shine border
		SetAPen(rp, pens[SHINEPEN]);
		Move(rp, box.Left, box.Top + box.Height - 1);
		Draw(rp, box.Left, box.Top);
		Draw(rp, box.Left + box.Width - 2, box.Top);

		// Draw shadow border
		SetAPen(rp, pens[SHADOWPEN]);
		Move(rp, box.Left + box.Width - 1, box.Top);
		Draw(rp, box.Left + box.Width - 1, box.Top + box.Height - 1);
		Draw(rp, box.Left + 1, box.Top + box.Height - 1);

		// Decrease fill area for the border
		box.Left++;
		box.Top++;
		box.Width -= 2;
		box.Height -= 2;

		// Background colour fill?
		if (draw->imp_State == IDS_INACTIVENORMAL)
		{
			// Fill with background colour
			EraseRect(rp, box.Left, box.Top, box.Left + box.Width - 1, box.Top + box.Height - 1);
			break;
		}

	// Filled box
	case IM_BORDER_BOX:

		// Set pen for state
		SetAPen(rp, pens[(draw->imp_State == IDS_INACTIVENORMAL) ? BACKGROUNDPEN : FILLPEN]);

		// Fill background
		RectFill(rp, box.Left, box.Top, box.Left + box.Width - 1, box.Top + box.Height - 1);
		break;

	// Iconify gadget
	case IM_ICONIFY: {
		short x, y, maxx, maxy;
		BOOL normal = 1, selected = 0;

		// Calculate maxx and maxy
		maxx = box.Left + box.Width - 1;
		maxy = box.Top + box.Height - 1;

		// Set pen for background
		SetAPen(rp,
				pens[(draw->imp_State == IDS_INACTIVENORMAL || draw->imp_State == IDS_INACTIVESELECTED) ? BACKGROUNDPEN
																										: FILLPEN]);

		// Fill background
		RectFill(rp, box.Left + 1, box.Top + 1, maxx - 1, maxy - 1);

		// Inner square colour
		SetAPen(rp, pens[SHADOWPEN]);

		// Get coordinates for top-right corner of inner square
		x = box.Left + (box.Width >> 1) - 1;
		y = maxy - (box.Height >> 1) + 2;
		if (y > maxy - 6)
			y = maxy - 6;

		// SysIHack/3D appearance?
		if (data->data->flags & LIBDF_3DLOOK && box.Height > 9)
			normal = 0;

		// Is gadget selected?
		if (draw->imp_State == IDS_SELECTED || draw->imp_State == IDS_INACTIVESELECTED)
			selected = 1;

		// Draw inner square - old-style
		if (normal)
		{
			Move(rp, x, y);
			Draw(rp, x, y + 2);
			Draw(rp, x - 3, y + 2);
			Draw(rp, x - 3, y);
			Draw(rp, x, y);
		}

		// 3D style
		else
		{
			// Shadow pen or shine if selected
			if (selected)
				SetAPen(rp, pens[SHINEPEN]);
			Move(rp, x + 2, y);
			Draw(rp, x + 2, y + 2);
			Draw(rp, x - 2, y + 2);

			// Shine pen or shadow if selected
			SetAPen(rp, pens[(selected) ? SHADOWPEN : SHINEPEN]);
			Move(rp, x - 3, y + 1);
			Draw(rp, x - 3, y - 1);
			Draw(rp, x + 1, y - 1);

			// Back to shadow pen if not selected
			if (!selected)
				SetAPen(rp, pens[SHADOWPEN]);
		}

		// Is gadget selected?
		if (selected)
		{
			struct Rectangle rect;

			// If gadget is too small outer square doesn't change vertically
			if (box.Height < 13)
			{
				// Get outer-square coordinates
				rect.MinX = box.Left + 5;
				rect.MinY = box.Top + 2;
				rect.MaxX = maxx - ((normal) ? 9 : 7);
				rect.MaxY = maxy - 2;
			}

			// Otherwise, it does
			else
			{
				// Get outer-square coordinates
				rect.MinX = x - 5;
				rect.MinY = y - 3;
				rect.MaxX = x + 3;
				rect.MaxY = y + 4;
			}

			// Draw outer square - old-style
			if (normal)
			{
				// Draw outer square
				Move(rp, rect.MinX, rect.MinY);
				Draw(rp, rect.MaxX, rect.MinY);
				Draw(rp, rect.MaxX, rect.MaxY);
				Draw(rp, rect.MinX, rect.MaxY);
				Draw(rp, rect.MinX, rect.MinY);
			}

			// 3D style
			else
			{
				// Shine pen
				SetAPen(rp, pens[SHINEPEN]);
				Move(rp, rect.MinX, rect.MaxY - 1);
				Draw(rp, rect.MinX, rect.MinY);
				Draw(rp, rect.MaxX - 1, rect.MinY);

				// Shadow pen
				SetAPen(rp, pens[SHADOWPEN]);
				Move(rp, rect.MaxX, rect.MinY + 1);
				Draw(rp, rect.MaxX, rect.MaxY);
				Draw(rp, rect.MinX + 1, rect.MaxY);
			}
		}

		// Unselected
		else
		{
			short s;

			// Gap
			s = (box.Height < 13) ? 2 : 3;

			// Draw outer square - old style
			if (normal)
			{
				Move(rp, box.Left + 5, maxy - s);
				Draw(rp, box.Left + 5, box.Top + s);
				Draw(rp, maxx - 5, box.Top + s);
				Draw(rp, maxx - 5, maxy - s);
				Draw(rp, box.Left + 5, maxy - s);
			}

			// 3D style
			else
			{
				// Shadow pen
				Move(rp, box.Left + 5, maxy - s - 1);
				Draw(rp, box.Left + 5, box.Top + s);
				Draw(rp, maxx - 6, box.Top + s);

				// Shine pen
				SetAPen(rp, pens[SHINEPEN]);
				Move(rp, maxx - 5, box.Top + s + 1);
				Draw(rp, maxx - 5, maxy - s);
				Draw(rp, box.Left + 6, maxy - s);

				// Back to shadow pen
				SetAPen(rp, pens[SHADOWPEN]);
			}
		}

		// Need a shadow line at the far left of the gadget to make the drag bar look ok
		Move(rp, box.Left - 1, box.Top + 1);
		Draw(rp, box.Left - 1, maxy);

		// Back to shine pen
		SetAPen(rp, pens[SHINEPEN]);

		// Fill inner square if active - old style only
		if (normal)
		{
			if (draw->imp_State == IDS_NORMAL || draw->imp_State == IDS_SELECTED)
			{
				Move(rp, x - 2, y + 1);
				Draw(rp, x - 1, y + 1);
			}
		}

		// If selected, shine is in shadow
		if (selected)
			SetAPen(rp, pens[SHADOWPEN]);

		// Draw shine border
		Move(rp, maxx, box.Top);
		Draw(rp, box.Left, box.Top);
		Draw(rp, box.Left, maxy - 1);

		// If selected, shadow is shine
		SetAPen(rp, pens[(selected) ? SHINEPEN : SHADOWPEN]);

		// Draw shadow border
		Move(rp, maxx, box.Top + 1);
		Draw(rp, maxx, maxy);
		Draw(rp, box.Left, maxy);
	}
	break;

	// A lock
	case IM_LOCK: {
		short size, x, y;

		// Small size if box < 15 high
		if (box.Height < 15)
			size = 8;
		else
			size = 10;

		// Get bottom-left coords
		x = box.Left + ((box.Width - size) >> 1);
		y = box.Top + box.Height - 1 - ((box.Height - size) >> 1);

		// Set pen for background
		SetAPen(rp,
				pens[(draw->imp_State == IDS_INACTIVENORMAL || draw->imp_State == IDS_INACTIVESELECTED) ? BACKGROUNDPEN
																										: FILLPEN]);

		// Fill background
		RectFill(rp, box.Left + 1, box.Top + 1, box.Left + box.Width - 2, box.Top + box.Height - 2);

		// Set pen for lock outline
		SetAPen(rp, pens[TEXTPEN]);

		// Draw base
		Move(rp, x, y);
		Draw(rp, x + size - 1, y);
		Draw(rp, x + size - 1, y - (size >> 1) + 1);
		Draw(rp, x, y - (size >> 1) + 1);
		Draw(rp, x, y);

		// Draw thing going up
		Move(rp, x + 1, y - (size >> 1));
		Draw(rp, x + 1, y - size + 2);

		// Draw thing going across
		Move(rp, x + 2, y - size + ((size == 8) ? 1 : 2));
		if (size == 10)
			Draw(rp, x + 2, y - size + 1);
		Draw(rp, x + size - 3, y - size + 1);
		if (size == 10)
			Draw(rp, x + size - 3, y - size + 2);

		// Draw thing going down
		Move(rp, x + size - 2, y - size + 2);

		// Selected?
		if (draw->imp_State == IDS_SELECTED || draw->imp_State == IDS_INACTIVESELECTED)
		{
			// All the way down
			Draw(rp, x + size - 2, y - (size >> 1));
		}

		// Just a little way down
		else
			Draw(rp, x + size - 2, y - size + 2);

		// If active, fill inside of base (shine pen, like the iconify gadget;
		// HIGHLIGHTTEXTPEN is a text pen and may be any colour)
		if (draw->imp_State == IDS_NORMAL || draw->imp_State == IDS_SELECTED)
		{
			// Fill inside of base
			SetAPen(rp, pens[SHINEPEN]);
			RectFill(rp, x + 1, y - (size >> 1) + 2, x + size - 2, y - 1);
		}

		// Need a shadow line at the far left of the gadget to make the drag bar look ok
		SetAPen(rp, pens[SHADOWPEN]);
		Move(rp, box.Left - 1, box.Top + 1);
		Draw(rp, box.Left - 1, box.Top + box.Height - 1);

		// And a shine line
		SetAPen(rp, pens[SHINEPEN]);
		Move(rp, box.Left, box.Top + 1);
		Draw(rp, box.Left, box.Top + box.Height - 2);
	}
	break;
	}
}

#ifdef __AROS__
AROS_UFH3(IPTR,
		  image_dispatch_entry,
		  AROS_UFHA(Class *, cl, A0),
		  AROS_UFHA(Object *, obj, A2),
		  AROS_UFHA(Msg, msg, A1))
{
	AROS_USERFUNC_INIT
	return image_dispatch(cl, obj, msg);
	AROS_USERFUNC_EXIT
}
#endif
