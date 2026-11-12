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
#ifndef _LAYOUT_ROUTINES_H
#define _LAYOUT_ROUTINES_H

//#define BACKFILL

#ifndef __amigaos3__
	#pragma pack(2)
#endif

struct backfill_hook
{
	struct Hook hook;
	// struct Library		*GfxBase;
	ULONG pad1;
	UWORD dither[2];
	struct RastPort rp;
	short shinepen;
	short shadowpen;
};

#ifndef __amigaos3__
	#pragma pack()
#endif

#ifdef BACKFILL
void ASM _req_backfill(REG(a0, struct backfill_hook *hook),
					   REG(a1, struct BackFillInfo *info),
					   REG(a2, struct RastPort *rp));
#else
void _req_backfill(struct Window *window);
#endif

void _layout_add_gad_image(APTR *memory, struct Gadget *, struct Image *);
BOOL _layout_slider_has_level(struct TagItem *tags);
void _layout_slider_level(struct Window *window, GL_Object *object, long level);
void _layout_slider_message(struct IntuiMessage *msg);

APTR alloc_object_memory(APTR *memory, long size);

void layout_resize(struct Window *window);

#include "boopsi.h"

#endif
