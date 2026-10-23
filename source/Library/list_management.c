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

#include <stdlib.h>

#include "dopuslib.h"

/****************************************************************************
						 Returns a new list structure
 ****************************************************************************/

Att_List *LIBFUNC L_Att_NewList(REG(d0, ULONG flags))
{
	Att_List *list;

	// Create new list
	if (!(list = AllocVec(sizeof(Att_List), MEMF_CLEAR)))
		return 0;

	// Initialise list structure
	L_InitListLock((struct ListLock *)list, 0);

	// Want pooling?
	if (flags & LISTF_POOL)
	{
		if (!(list->memory = L_NewMemHandle(1024, 256, MEMF_CLEAR)))
			flags &= ~LISTF_POOL;
	}

	// Store flags
	list->flags = flags;
	return list;
}

/****************************************************************************
						  Add a node to an Att_List
 ****************************************************************************/

Att_Node *LIBFUNC L_Att_NewNode(REG(a0, Att_List *list), REG(a1, const char *name), REG(d0, IPTR data), REG(d1, ULONG flags))
{
	Att_Node *node;
	BOOL added = 0;

	// Valid list?
	if (!list)
		return 0;

	// Lock list
	L_LockAttList(list, TRUE);

	// Allocate node
	if (!(node = L_AllocMemH(list->memory, sizeof(Att_Node))))
	{
		L_UnlockAttList(list);
		return 0;
	}

	// If name supplied, create a copy of it
	if (name)
	{
		if (!(node->node.ln_Name = L_AllocMemH(list->memory, strlen(name) + 1)))
		{
			L_FreeMemH(node);
			L_UnlockAttList(list);
			return 0;
		}
		strcpy(node->node.ln_Name, name);
	}

	// Store data and list pointer
	node->data = data;
	node->list = list;

	// Alphabetical sort?
	if ((flags & ADDNODE_SORT) && name)
	{
		Att_Node *posnode, *lastnode = 0;
		short match;

		// Go through existing nodes
		posnode = (Att_Node *)list->list.lh_Head;
		while (posnode->node.ln_Succ)
		{
			// Compare new node name against existing name
			if ((match = stricmp(name, posnode->node.ln_Name)) <= 0)
			{
				// If exclusive flag is set and we matched exactly, reject this node
				if ((flags & ADDNODE_EXCLUSIVE) && match == 0)
				{
					L_FreeMemH(node->node.ln_Name);
					L_FreeMemH(node);
					L_UnlockAttList(list);
					return 0;
				}

				// Insert into list
				Insert((struct List *)list, (struct Node *)node, (struct Node *)lastnode);
				added = 1;
				break;
			}
			lastnode = posnode;
			posnode = (Att_Node *)posnode->node.ln_Succ;
		}
	}

	// Numerical sort?
	else if ((flags & ADDNODE_NUMSORT) && name)
	{
		Att_Node *posnode, *lastnode = 0;

		// Go through existing nodes
		posnode = (Att_Node *)list->list.lh_Head;
		while (posnode->node.ln_Succ)
		{
			long one, two;

			// Get numerical values of names
			one = atoi(name);
			two = atoi(posnode->node.ln_Name);

			// If exclusive flag is set and we match exactly, reject this node
			if ((flags & ADDNODE_EXCLUSIVE) && one == two)
			{
				L_FreeMemH(node->node.ln_Name);
				L_FreeMemH(node);
				L_UnlockAttList(list);
				return 0;
			}

			// Insert into list
			if (one < two)
			{
				Insert((struct List *)list, (struct Node *)node, (struct Node *)lastnode);
				added = 1;
				break;
			}
			lastnode = posnode;
			posnode = (Att_Node *)posnode->node.ln_Succ;
		}
	}

	// Priority sort?
	else if (flags & ADDNODE_PRI)
	{
		node->node.ln_Pri = data;
		Enqueue((struct List *)list, (struct Node *)node);
		added = 1;
	}

	// No sorting, exclusive?
	else if (flags & ADDNODE_EXCLUSIVE)
	{
		Att_Node *old;

		// Find existing node
		if ((old = (Att_Node *)FindName((struct List *)list, node->node.ln_Name)))
		{
			// Remove old node
			Remove((struct Node *)old);

			// Free old node
			L_FreeMemH(old->node.ln_Name);
			L_FreeMemH(old);
		}
	}

	// If not added by a sort, add to end
	if (!added)
		AddTail((struct List *)list, (struct Node *)node);

	// Unlock the list
	L_UnlockAttList(list);

	return node;
}

/****************************************************************************
						  Remove a node from an Att_List
 ****************************************************************************/

void LIBFUNC L_Att_RemNode(REG(a0, Att_Node *node))
{
	// Valid node?
	if (node)
	{
		Att_List *list;

		// Cache list
		list = node->list;

		// Lock list
		L_LockAttList(list, TRUE);

		// Remove the node
		Remove((struct Node *)node);

		// Free memory allocated for name
		L_FreeMemH(node->node.ln_Name);

		// Free node
		L_FreeMemH(node);

		// Unlock list
		L_UnlockAttList(list);
	}
}

/****************************************************************************
					   Reposition a node in an Att_List
 ****************************************************************************/

void LIBFUNC L_Att_PosNode(REG(a0, Att_List *list), REG(a1, Att_Node *node), REG(a2, Att_Node *before))
{
	// Check valid stuff
	if (!list || !node || !before || !before->node.ln_Pred)
		return;

	// Remove node from list
	Remove((struct Node *)node);

	// Insert node in list
	Insert((struct List *)list, (struct Node *)node, before->node.ln_Pred);

	// Store new list pointer
	node->list = list;
}

/****************************************************************************
						  Free an entire Att_List
 ****************************************************************************/

void LIBFUNC L_Att_RemList(REG(a0, Att_List *list), REG(d0, long flags))
{
	Att_Node *node, *next;

	// Valid list?
	if (!list)
		return;

	// Only need to go through list if freeing data, or not using pools
	if (flags & REMLIST_FREEDATA || !list->memory)
	{
		// Go through list
		for (node = (Att_Node *)list->list.lh_Head; node->node.ln_Succ; node = next)
		{
			// Cache next
			next = (Att_Node *)node->node.ln_Succ;

			// If flag is set, free node data
			if (flags & REMLIST_FREEDATA)
			{
				if (flags & REMLIST_FREEMEMH)
					L_FreeMemH((APTR)node->data);
				else
					FreeVec((APTR)node->data);
			}

			// If not pooling, remove node
			if (!list->memory)
			{
				// Free memory allocated for name
				L_FreeMemH(node->node.ln_Name);

				// Free node
				L_FreeMemH(node);
			}
		}
	}

	// Free entire list?
	if (!(flags & REMLIST_SAVELIST))
	{
		// Free list memory
		L_FreeMemHandle(list->memory);

		// Free list itself
		FreeVec(list);
	}

	// Otherwise, reinitialise it
	else
	{
		// Clear list memory
		L_ClearMemHandle(list->memory);

		// Reinitialise list
		NewList(&list->list);
	}
}

/****************************************************************************
						Find a node by ordinal number
 ****************************************************************************/

Att_Node *LIBFUNC L_Att_FindNode(REG(a0, Att_List *list), REG(d0, long number))
{
	Att_Node *node;
	long a;

	// Valid list and number?
	if (!list || number < 0)
		return 0;

	// Scan list
	node = (Att_Node *)list->list.lh_Head;
	for (a = 0; a < number && node->node.ln_Succ; a++, node = (Att_Node *)node->node.ln_Succ)
		;

	// Return node
	return (node->node.ln_Succ) ? node : 0;
}

/****************************************************************************
				  Find the ordinal number of a node by name
 ****************************************************************************/

long LIBFUNC L_Att_NodeNumber(REG(a0, Att_List *list), REG(a1, char *name))
{
	Att_Node *node;
	long a;

	// Valid list and name?
	if (!list || !name)
		return -1;

	// Scan list
	node = (Att_Node *)list->list.lh_Head;
	for (a = 0; node->node.ln_Succ; a++, node = (Att_Node *)node->node.ln_Succ)

		// If names match, return number
		if (stricmp(name, node->node.ln_Name) == 0)
			return a;

	return -1;
}

/****************************************************************************
						   Find a node by its data
 ****************************************************************************/

Att_Node *LIBFUNC L_Att_FindNodeData(REG(a0, Att_List *list), REG(d0, IPTR data))
{
	Att_Node *node;

	// Valid list?
	if (!list)
		return 0;

	// Scan list
	for (node = (Att_Node *)list->list.lh_Head; node->data != data && node->node.ln_Succ;
		 node = (Att_Node *)node->node.ln_Succ)
		;

	// Return node
	return (node->node.ln_Succ) ? node : 0;
}

/****************************************************************************
				   Find a node's ordinal number by its data
 ****************************************************************************/

long LIBFUNC L_Att_NodeDataNumber(REG(a0, Att_List *list), REG(d0, IPTR data))
{
	Att_Node *node;
	long a;

	// Valid list?
	if (!list)
		return -1;

	// Scan list
	for (a = 0, node = (Att_Node *)list->list.lh_Head; node->data != data && node->node.ln_Succ;
		 node = (Att_Node *)node->node.ln_Succ, a++)
		;

	// Return node
	return (node->node.ln_Succ) ? a : -1;
}

/****************************************************************************
					  Find a node name by ordinal number
 ****************************************************************************/

char *LIBFUNC L_Att_NodeName(REG(a0, Att_List *list), REG(d0, long number))
{
	Att_Node *node;

	// Get node
	if (!(node = L_Att_FindNode(list, number)))
		return 0;

	// Return name
	return node->node.ln_Name;
}

/****************************************************************************
					  Count the nodes in a list
 ****************************************************************************/

long LIBFUNC L_Att_NodeCount(REG(a0, Att_List *list))
{
	Att_Node *node;
	long count;

	// Valid list?
	if (!list)
		return 0;

	// Scan list
	for (count = 0, node = (Att_Node *)list->list.lh_Head; node->node.ln_Succ;
		 node = (Att_Node *)node->node.ln_Succ, count++)
		;

	// Return node count
	return count;
}

/****************************************************************************
							 Change a node's name
 ****************************************************************************/

void LIBFUNC L_Att_ChangeNodeName(REG(a0, Att_Node *node), REG(a1, char *name))
{
	// Valid node?
	if (!node)
		return;

	// Free existing name
	if (node->node.ln_Name)
	{
		L_FreeMemH(node->node.ln_Name);
		node->node.ln_Name = 0;
	}

	// Copy new name
	if (name)
	{
		if ((node->node.ln_Name = L_AllocMemH((node->list) ? node->list->memory : 0, strlen(name) + 1)))
			strcpy(node->node.ln_Name, name);
	}
}

/****************************************************************************
					  Find the ordinal number of a node
 ****************************************************************************/

long LIBFUNC L_Att_FindNodeNumber(REG(a0, Att_List *list), REG(a1, Att_Node *node))
{
	Att_Node *test_node;
	long count;

	// Valid list and node?
	if (!list || !node)
		return -1;

	// Scan list
	for (test_node = (Att_Node *)list->list.lh_Head, count = 0; node != test_node && test_node->node.ln_Succ;
		 test_node = (Att_Node *)test_node->node.ln_Succ, count++)
		;

	// Return node
	return (test_node->node.ln_Succ) ? count : -1;
}

/****************************************************************************
				Add a node to a list with alphabetical sorting
 ****************************************************************************/

void LIBFUNC L_AddSorted(REG(a0, struct List *list), REG(a1, struct Node *node))
{
	struct Node *posnode, *lastnode = 0;
	BOOL added = 0;

	// Go through existing nodes
	for (posnode = list->lh_Head; posnode->ln_Succ; lastnode = posnode, posnode = posnode->ln_Succ)
	{
		// Compare new node name against existing name
		if ((stricmp(node->ln_Name, posnode->ln_Name)) <= 0)
		{
			// Insert into list
			Insert(list, node, lastnode);
			added = 1;
			break;
		}
	}

	// If not added, add to end of list
	if (!added)
		AddTail(list, node);
}

// FindName case-insensitive
struct Node *LIBFUNC L_FindNameI(REG(a0, struct List *list), REG(a1, char *name))
{
	register struct Node *node;
	short len;

	// Get string length
	len = strlen(name);

	// Go through list
	for (node = list->lh_Head; node->ln_Succ; node = node->ln_Succ)
	{
		char *nodename = node->ln_Name;
#ifdef __AROS__
		// ABIv0 AROS Nodes are not binary compatible with AmigaOS
		// so nodes like CommandList has to be treated specially
		if (list->lh_Type == 255)  // don't try this at home
			nodename = *(char **)(((char *)node) + sizeof(struct MinNode) + sizeof(UWORD));

#endif
		// Valid name?
		if (nodename)
		{
			// Compare node name
			if (stricmp(nodename, name) == 0)
				return node;

			// Compare to length supplied
			if (strnicmp(nodename, name, len) == 0)
			{
				// Check following character is a "control code"
				if (nodename[len] == '\t' || nodename[len] == '\a')
					return node;
			}
		}
	}

	// Not found
	return 0;
}

// Lock a list
void LIBFUNC L_LockAttList(REG(a0, Att_List *list), REG(d0, short exclusive))
{
	// List requires locking?
	if (list && list->flags & LISTF_LOCK)
		L_GetSemaphore(&list->lock, (exclusive) ? SEMF_EXCLUSIVE : SEMF_SHARED, 0);
}

// Unlock list
void LIBFUNC L_UnlockAttList(REG(a0, Att_List *list))
{
	// List required locking?
	if (list && list->flags & LISTF_LOCK)
		L_FreeSemaphore(&list->lock);
}

// Swap two nodes in a list
void LIBFUNC L_SwapListNodes(REG(a0, struct List *list), REG(a1, struct Node *swap1), REG(a2, struct Node *swap2))
{
	// Is swap1 immediately before swap2?
	if (swap1->ln_Succ == swap2)
	{
		// Position swap1 after swap2
		Remove(swap1);
		Insert(list, swap1, swap2);
	}

	// Or is swap2 immediately before swap1?
	else if (swap2->ln_Succ == swap1)
	{
		// Position swap2 after swap1
		Remove(swap2);
		Insert(list, swap2, swap1);
	}

	// Swap the two nodes
	else
	{
		struct Node *after;

		// Get node preceding swap2
		after = swap2->ln_Pred;

		// Shift swap2 to after swap1
		Remove(swap2);
		Insert(list, swap2, swap1);

		// Shift swap1 to swap2's old spot
		Remove(swap1);
		Insert(list, swap1, after);
	}
}

// See if a locked list is empty
BOOL LIBFUNC L_IsListLockEmpty(REG(a0, struct ListLock *list))
{
	BOOL empty;

	// Lock list
	L_GetSemaphore(&list->lock, SEMF_SHARED, 0);

	// See if it's empty
	empty = IsListEmpty(&list->list) ? TRUE : FALSE;

	// Unlock list
	L_FreeSemaphore(&list->lock);
	return empty;
}
