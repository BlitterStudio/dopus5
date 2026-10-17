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

#define CATCOMP_NUMBERS
#include "show.strings"
#include "module_deps.h"

#ifndef __amigaos3__
	#pragma pack(2)
#endif
typedef struct
{
	ILBMHandle *ilbm;
	struct Screen *display_screen;
	struct Window *display_window;
	ULONG error;
	IPCData *ipc;
	char *file;

	TimerHandle *timer;

	struct BitMap *double_bm;
	BOOL anim_ok;

	struct BitMap *anim_bm[2];
	short anim_bitmap;

	unsigned long frame_speed;
	unsigned long original_speed;
	unsigned long frame_secs;
	unsigned long frame_micros;

	BOOL anim_playing;
	unsigned short frame_num;
	unsigned short frame_count;
	ANIMFrame *last_frame;

	struct Preferences prefs;

	// struct Library		*dt_base;
	ULONG pad1;

	short width;
	short height;
	short depth;
	ULONG modeid;

	APTR dt_object;
	struct dtFrameBox framebox;
	struct FrameInfo frameinfo;
	struct gpLayout layout;
	ULONG *cregs;
	struct BitMap *dt_bm;

	short pic_ok;
	char *picture_type;

	BOOL active;

	ULONG numcolours;

	struct BitMap *scr_bm;
} show_data;
#ifndef __amigaos3__
	#pragma pack()
#endif

//#define DataTypesBase (data->dt_base) Don't redefine-opened in libinit.c

/*struct Screen *STDARGS my_OpenScreenTags(Tag tag,...);
struct Window *STDARGS my_OpenWindowTags(Tag tag,...);*/

void show_free(show_data *data);
BOOL show_help(show_data *data);
void show_next_frame(show_data *data);
short show_print(show_data *data, struct Window *help_window, ObjectList *list);

void do_riff(unsigned char *delta,
			 struct BitMap *bitmap,
			 unsigned short xor
			 ,
			 unsigned short sourcewidth,
			 unsigned short size);
void decode_riff_xor(unsigned char *delta, char *plane, unsigned short rowbytes, unsigned short sourcebytes);
void decode_riff_set(unsigned char *delta, char *plane, unsigned short rowbytes, unsigned short sourcebytes);

void do_riff_7(unsigned char *delta, struct BitMap *bitmap, unsigned short sourcewidth, unsigned short size);
void decode_riff7_short(unsigned short *delta,
						unsigned char *opcode,
						unsigned char *plane,
						unsigned short rowbytes,
						unsigned short sourcebytes);
void decode_riff7_long(ULONG *delta,
					   unsigned char *opcode,
					   unsigned char *plane,
					   unsigned short rowbytes,
					   unsigned short sourcebytes);

BOOL show_get_dtpic(show_data *data, struct Node *node);

extern ConfigWindow picture_info_window, anim_info_window;
extern ObjectDef picture_info_objects[], anim_info_objects[], show_info_objects[], print_objects[];

enum {
	GAD_dummy,

	GAD_INFO_AREA,
	GAD_INFO_FILE,
	GAD_INFO_IMAGE_SIZE,
	GAD_INFO_COLOURS,
	GAD_INFO_MODE,
	GAD_INFO_FRAME,
	GAD_INFO_ANIM,
	GAD_OK,

	GAD_PRINT_AREA,
	GAD_PRINT_ASPECT,
	GAD_PRINT_IMAGE,
	GAD_PRINT_SHADE,
	GAD_PRINT_PLACEMENT,
	GAD_PRINT_FORM_FEED,
	GAD_PRINT_PRINT_TITLE,
	GAD_PRINT,

	GAD_INFO_TYPE,
};

#define ILBMF_GET_BODY (1 << 0)		  // Get BODY
#define ILBMF_GET_BITMAP (1 << 1)	  // Get Bitmap
#define ILBMF_GET_PLANES (1 << 2)	  // Get plane data
#define ILBMF_NO_ANIM (1 << 3)		  // Specifically don't want an animation
#define ILBMF_ANIM_ONE_ONLY (1 << 4)  // Get maximum of one animation frame
#define ILBMF_NO_PALETTE (1 << 5)	  // Don't bother about palette
#define ILBMF_NO_24BIT (1 << 6)		  // Don't support 24 bit

#define ILBMF_GOT_BODY (1 << 0)	   // Got BODY
#define ILBMF_GOT_BITMAP (1 << 1)  // Got Bitmap
#define ILBMF_GOT_PLANES (1 << 2)  // Got plane data
#define ILBMF_IS_ANIM (1 << 3)	   // Is an animation

#define ILBMF_GOT_HEADER (1 << 4)	  // Got header
#define ILBMF_GOT_PALETTE (1 << 5)	  // Got palette
#define ILBMF_GOT_MODE (1 << 6)		  // Got mode ID
#define ILBMF_IS_ANIM_BRUSH (1 << 7)  // Is an anim brush
#define ILBMF_CHIP_PLANES (1 << 8)	  // Planes are in chip

#define DIF_MASK (1 << 0)
#define DIF_WRITEPIX (1 << 6)

#define ANIMF_LONG 1
#define ANIMF_XOR 2
