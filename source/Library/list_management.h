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
#ifndef _LIST_MANAGMENT_H
#define _LIST_MANAGMENT_H

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct _Att_List
{
	struct List list;
} Att_List;

typedef struct _Att_Node
{
	struct Node node;
	Att_List *list;
	IPTR data;
} Att_Node;

#ifndef __amigaos3__
	#pragma pack()
#endif

#define ADDNODE_SORT 1		 // Sort names
#define ADDNODE_EXCLUSIVE 2	 // Exclusive entry
#define ADDNODE_NUMSORT 4	 // Numerical name sort

#define REMLIST_FREEDATA 1	// FreeVec data when freeing list

// prototypes
Att_List *Att_NewList(void);
Att_Node *Att_NewNode(Att_List *list, const char *name, IPTR data, ULONG flags);
void Att_RemNode(Att_Node *node);
void Att_PosNode(Att_List *, Att_Node *, Att_Node *);
void Att_RemList(Att_List *list, int);
Att_Node *Att_FindNode(Att_List *list, int number);
Att_NodeNumber(Att_List *list, char *name);
Att_Node *Att_FindNodeData(Att_List *list, IPTR data);
Att_NodeDataNumber(Att_List *list, IPTR data);

#define VALIDLIST(l) ((l) && (l)->list.lh_Head->ln_Succ)

#endif
