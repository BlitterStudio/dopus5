/*
  XADOpus.module 1.22 - DOpus Magellan plugin to browse through XAD archives
  Copyright (C) 1999,2000 Mladen Milinkovic <mladen.milinkovic@ri.tel.hr>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#define XADopus_NUMBERS
#include "XADopus.strings"
#include "module_deps.h"

#define DOpusGetString GetString

#define ADDNODEF_SORT 1	 // Sort names

#ifdef __amigaos3__
	#define xadERROR LONG
#endif

#ifndef __amigaos3__
	#pragma pack(2)
#endif

struct function_entry
{
	struct MinNode node;
	char *name;	  // File name
	APTR entry;	  // Entry pointer (don't touch!)
	short type;	  // Type of file
	short flags;  // File flags
};

struct path_node
{
	struct MinNode node;
	char buffer[512];  // Contains path string
	char *path;		   // Points to path string
	APTR lister;	   // Lister pointer
	ULONG flags;	   // Flags
};
//----

struct Tree
{
	struct FileInfoBlock fib;
	struct xadFileInfo *xfi;
	APTR entry;
	struct Tree *Next, *Child;
};

struct xoData
{
	ULONG ArcMode;
	IPTR listh;
	char lists[24], listpath[512], rootpath[108];
	struct xadArchiveInfo *ArcInf;
	struct MsgPort *mp;
	char mp_name[20];
	IPCData *ipc;
	struct Screen *screen;
	struct Tree *root, *cur;
	APTR rhand;
	char *arcname, *buf;
	struct Window *listw;
	struct DOpusLocale *locale;
	/*struct Library *DOpusBase;
	struct DosLibrary *DOSBase;
	struct UtilityBase *UtilityBase;*/
	ULONG pad1[3];
	struct path_node *listp2, *destp;
	APTR ptr;
	DOpusCallbackInfo hook;
	struct MinList Temp;
	struct path_node listp;
	BOOL All;
	char password[512];
	BOOL same_lister;
	char origpath[512];
};

struct TempFile
{
	struct MinNode node;
	char FileName[110];
};

struct MyPacket
{
	struct Message sp_Msg;
	struct DosPacket sp_Pkt;
	char *a7, *a8;
};

#ifndef __amigaos3__
	#pragma pack()
#endif
