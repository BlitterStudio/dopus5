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
#ifndef _IMAGES_H
#define _IMAGES_H

#include <libraries/newicon.h>

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct
{
	struct Node node;  // Image path in ln_Name
	long count;		   // Open count

	unsigned short width;	// Width of image
	unsigned short height;	// Height of image
	unsigned short depth;	// Depth of image

	unsigned short **planes[2];	 // Image planes (not necessarily chip memory)
	unsigned short flags;		 // Flags

	ILBMHandle *ilbm;  // ILBM

	unsigned short **remap_planes[2];  // Remapped planes
	unsigned long checksum[2];		   // Image checksum (for icons)
	short remap_depth;				   // Depth remapped to

	struct SignalSemaphore lock;  // Image lock
	short remap_count;			  // Number of times remapped

	struct NewDiskObject *new_diskobj;	// NewIcons disk object
	struct Image *remap_image[2];		// Remapped images

	ImageRemap *remap_info;	 // Image remap info

	/*
		Object			*dt_object;		// Datatypes object
		struct BitMap		*dt_object_bm;		// Datatypes bitmap
	*/
} Image_Data;

#ifndef __amigaos3__
	#pragma pack()
#endif

#define IMAGEF_SEL_IMAGE (1 << 0)	// Selected imagery
#define IMAGEF_ILBM (1 << 1)		// Image from ILBM
#define IMAGEF_CHIP (1 << 2)		// Image already in chip memory
#define IMAGEF_ALT_CHIP (1 << 3)	// Alternate image in chip memory
#define IMAGEF_ICON (1 << 4)		// Cached icon
#define IMAGEF_DEFICON (1 << 5)		// Cached default icon
#define IMAGEF_ICON_IMAGE (1 << 6)	// Image is from an icon
#define IMAGEF_NEWICON (1 << 7)		// Icon is a NewIcon
#define IMAGEF_REMAP_FAST (1 << 8)	// Remapped planes in fast memory (RTG screen)
#define IMAGEF_FAST (1 << 9)		// Image moved from chip to fast memory (RTG screen)
#define IMAGEF_ALT_FAST (1 << 10)	// Alternate image moved from chip to fast memory

Image_Data *read_image(char *, OpenImageInfo *);
Image_Data *new_image(short, short, short);
void free_image(Image_Data *);
BOOL copy_icon_image(struct Image *, Image_Data *, short);
BOOL add_remap_pen(struct Screen *screen, ImageRemap *remap, unsigned short pen);

#endif
