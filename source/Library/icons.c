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
#include <proto/newicon.h>

/*
	Modified 3 nov 99 gjp to now call puticontags if V44


*/

#ifdef __amigaos3__
static inline void atomic_inc(ULONG *counter)
{
	asm volatile("addql #1,%0" : "+m"(*counter));
}

static inline void atomic_dec(ULONG *counter)
{
	asm volatile("subql #1,%0" : "+m"(*counter));
}
#elif defined(__AROS__) && defined(__arm__)
	#include <aros/atomic.h>
static inline void atomic_inc(ULONG *counter)
{
	__AROS_ATOMIC_INC_L(*counter);
}

static inline void atomic_dec(ULONG *counter)
{
	__AROS_ATOMIC_DEC_L(*counter);
}
#elif defined(__MORPHOS__)
	#include <hardware/atomic.h>
static inline void atomic_inc(ULONG *counter)
{
	ATOMIC_ADD(counter, 1);
}

static inline void atomic_dec(ULONG *counter)
{
	ATOMIC_SUB(counter, 1);
}
#else
static inline void atomic_inc(ULONG *counter)
{
	__sync_add_and_fetch(counter, 1);
}

static inline void atomic_dec(ULONG *counter)
{
	__sync_sub_and_fetch(counter, 1);
}
#endif

extern ULONG usecount[WB_PATCH_COUNT];

// Patched PutDiskObject()
PATCHED_2(BOOL, LIBFUNC L_WB_PutDiskObject, a0, char *, name, a1, struct DiskObject *, diskobj)
{
	atomic_inc(&usecount[WB_PATCH_PUTDISKOBJECT]);
	{
		struct LibData *data;
		struct MyLibrary *libbase;
		BOOL result, magic = FALSE;

		// Get library
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_PUTDISKOBJECT]);
			return FALSE;
		}

		// Get data pointer
		data = (struct LibData *)libbase->ml_UserData;

		// See if icon type is set to inverse magic
		if (diskobj->do_Magic == (UWORD)~WB_DISKMAGIC)
		{
			// Fix it
			diskobj->do_Magic = WB_DISKMAGIC;

			// Set magic flag, which will stop us sending notification
			magic = TRUE;
		}

		// Write icon
		result = L_WriteIcon(name, diskobj, libbase);

		// Succeeded?
		if (result && !magic)
			icon_notify(data, name, 0, 0);

		atomic_dec(&usecount[WB_PATCH_PUTDISKOBJECT]);
		return result;
	}
}
PATCH_END

// Patched DeleteDiskObject()
PATCHED_1(BOOL, LIBFUNC L_WB_DeleteDiskObject, a0, char *, name)
{
	atomic_inc(&usecount[WB_PATCH_DELETEDISKOBJECT]);
	{
		struct LibData *data;
		struct MyLibrary *libbase;
		BOOL result;
		char *full_name;

		// Get library
		if (!(libbase = GET_DOPUSLIB))
		{
			atomic_dec(&usecount[WB_PATCH_DELETEDISKOBJECT]);
			return FALSE;
		}

		// Get data pointer
		data = (struct LibData *)libbase->ml_UserData;

		// Get full name
		full_name = icon_fullname(data, name);

		// Write icon
		result = L_DeleteIcon(name, libbase);

		//#define DOSBase (data->dos_base)

#ifdef __AROS__
		// try DeleteFile too, in case it was an orphan icon
		if (!result)
			result = DeleteFile(full_name);
#endif

		// Succeeded?
		if ((result || IoErr() == ERROR_OBJECT_NOT_FOUND) && full_name)
			icon_notify(data, full_name, INF_FULLNAME, 1);

		//#undef DOSBase

		// Free full name buffer
		FreeVec(full_name);

		atomic_dec(&usecount[WB_PATCH_DELETEDISKOBJECT]);
		return result;
	}
}
PATCH_END

// Send icon notification
void icon_notify(struct LibData *data, char *name, ULONG flags, short delete)
{
	char *full_name;

	// Given full name?
	if (flags & INF_FULLNAME)
		full_name = name;

	// Get full name
	else if (!(full_name = icon_fullname(data, name)))
		return;

	// Send notify message
	L_SendNotifyMsg(DN_WRITE_ICON, 0, delete, FALSE, full_name, 0, data->dopus_base);

	// Free buffer
	if (!(flags & INF_FULLNAME))
		FreeVec(full_name);
}

// Get icon full name
char *icon_fullname(struct LibData *data, char *name)
{
	char *full_name, *ptr;
	BPTR lock;
	short len;

	// Allocate buffer for full name
	if (!(full_name = AllocVec(300, 0)))
		return 0;

	// Copy name
	strcpy(full_name, name);
	ptr = full_name + strlen(full_name);
	strcat(full_name, ".info");

	//#define DOSBase (data->dos_base)

	// Lock icon
	if (!(lock = Lock(full_name, ACCESS_READ)))
	{
		// Strip .info
		*ptr = 0;
		if ((lock = Lock(full_name, ACCESS_READ)))
		{
			// Add .info back
			strcpy(ptr, ".info");
		}
	}

	// Got lock?
	if (lock)
	{
		// Expand path
		L_DevNameFromLockDopus(lock, full_name, 256, data->dopus_base);

		// Disk?
		if (((ptr = strchr(full_name, ':'))) && stricmp(ptr + 1, "disk.info") == 0)
		{
			// Get real full name
			NameFromLock(lock, full_name, 256);

			// Strip after colon
			if ((ptr = strchr(full_name, ':')))
				*(ptr + 1) = 0;
		}

		// Unlock lock
		UnLock(lock);
	}

	//#undef DOSBase

	// Strip .info from name
	if ((len = strlen(full_name)) > 5 && stricmp(full_name + len - 5, ".info") == 0)
	{
		// Strip it
		*(full_name + len - 5) = 0;
	}

	return full_name;
}

// PutDiskObject without notification
BOOL LIBFUNC L_WriteIcon(REG(a0, char *name), REG(a1, struct DiskObject *icon), REG(a6, struct MyLibrary *libbase))
{
	struct LibData *data;
	BOOL result;

#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif
	// Get data pointer
	data = (struct LibData *)libbase->ml_UserData;

	// No icon?
	if (!icon)
		return 0;

	//#define NewIconBase	(data->new_icon_base)

	// A NewIcon?
	if (L_GetIconType(icon) == ICON_NEWICON && NewIconBase)
	{
		struct NewIconDiskObject *nido;
		struct DiskObject *temp, write_obj;

		// Get NIDO pointer
		nido = (struct NewIconDiskObject *)icon;

		// Save structure DiskObject pointer
		temp = nido->nido_NewDiskObject->ndo_StdObject;

		// Copy DiskObject to write
		CopyMem((char *)icon, (char *)&write_obj, sizeof(struct DiskObject));

		// Get original gadget structure fields
		write_obj.do_Gadget.GadgetRender = temp->do_Gadget.GadgetRender;
		write_obj.do_Gadget.SelectRender = temp->do_Gadget.SelectRender;
		write_obj.do_Gadget.Flags = temp->do_Gadget.Flags;
		write_obj.do_Gadget.Width = temp->do_Gadget.Width;
		write_obj.do_Gadget.Height = temp->do_Gadget.Height;

		// Fix revision
		if ((((IPTR)write_obj.do_Gadget.UserData) & WB_DISKREVISIONMASK) == 0)
			write_obj.do_Gadget.UserData = (APTR)((IPTR)write_obj.do_Gadget.UserData | WB_DISKREVISION);

		// Clear SpecialInfo field so we won't get stuck in a loop
		write_obj.do_Gadget.SpecialInfo = 0;

		// Set new icon pointer
		nido->nido_NewDiskObject->ndo_StdObject = &write_obj;

		// This is heavy magic, which stops us from notifying things twice
		// (PutNewDiskObject calls PutDiskObject..)
		write_obj.do_Magic = (UWORD)~WB_DISKMAGIC;

		// Write the NewIcon
		result = PutNewDiskObject((UBYTE *)name, nido->nido_NewDiskObject);

		// Restore the DiskObject pointer in the NewIcon
		nido->nido_NewDiskObject->ndo_StdObject = temp;
	}

	// Write normally
	else
	{
		// Fix revision
		if ((((IPTR)icon->do_Gadget.UserData) & WB_DISKREVISIONMASK) == 0)
			icon->do_Gadget.UserData = (APTR)((IPTR)icon->do_Gadget.UserData | WB_DISKREVISION);

		//#define IconBase	(data->icon_base)

		if (IconBase->lib_Version >= 44)
			result = PutIconTags(name, icon, TAG_DONE);

		else
			// Write the icon
			result = LIBCALL_2(
				BOOL, data->wb_data.old_function[WB_PATCH_PUTDISKOBJECT], IconBase, IIcon, a0, name, a1, icon);
	}

	//#undef IconBase
	//#undef NewIconBase

	return result;
}

// DeleteDiskObject without notification
BOOL LIBFUNC L_DeleteIcon(REG(a0, char *name), REG(a6, struct MyLibrary *libbase))
{
	struct LibData *data;

#if defined(__amigaos4__) || defined(__AROS__)
	libbase = dopuslibbase_global;
#endif

	// Get data pointer
	data = (struct LibData *)libbase->ml_UserData;

	// Delete it
	return LIBCALL_1(BOOL, data->wb_data.old_function[WB_PATCH_DELETEDISKOBJECT], IconBase, IIcon, a0, name);
}

// Get icon flags
ULONG LIBFUNC L_GetIconFlags(REG(a0, struct DiskObject *icon))
{
	ULONG flags;

	// Valid icon?
	if (!icon)
		return 0;

	// Stored in gadget userdata
	flags = (IPTR)icon->do_Gadget.UserData;

	// Mask off flags we're not interested in
	flags &= ICONF_POSITION_OK | ICONF_ICON_VIEW | ICONF_BORDER_OFF | ICONF_NO_LABEL | ICONF_BORDER_ON;
	return flags;
}

// Get Opus icon position
void LIBFUNC L_GetIconPosition(REG(a0, struct DiskObject *icon), REG(a1, short *x), REG(a2, short *y))
{
	// Clear values
	if (x)
		*x = 0;
	if (y)
		*y = 0;

	// Valid icon?
	if (!icon)
		return;

	// Get positions
	if (x)
		*x = ((ULONG)icon->do_Gadget.MutualExclude) >> 16;
	if (y)
		*y = ((ULONG)icon->do_Gadget.MutualExclude) & 0xffff;
}

// Set icon flags
void LIBFUNC L_SetIconFlags(REG(a0, struct DiskObject *icon), REG(d0, ULONG flags))
{
	ULONG oldflags;

	// Valid icon?
	if (!icon)
		return;

	// Get old flags from gadget
	oldflags = (IPTR)icon->do_Gadget.UserData;

	// Clear flags we're interested in
	oldflags &= ~(ICONF_POSITION_OK | ICONF_ICON_VIEW | ICONF_BORDER_OFF | ICONF_NO_LABEL | ICONF_BORDER_ON);

	// Or new flags in
	oldflags |= flags;

	// Store back in gadget
	icon->do_Gadget.UserData = (APTR)(IPTR)oldflags;
}

// Set Opus icon position
void LIBFUNC L_SetIconPosition(REG(a0, struct DiskObject *icon), REG(d0, short x), REG(d1, short y))
{
	// Valid icon?
	if (!icon)
		return;

	// Set position
	icon->do_Gadget.MutualExclude = (IPTR)(((ULONG)(UWORD)x << 16) | (UWORD)y);
}

// Get the icon type
short LIBFUNC L_GetIconType(REG(a0, struct DiskObject *icon))
{
	if (!icon)
		return 0;

	if (icon->do_Gadget.SpecialInfo == (APTR)icon)
		return ICON_CACHED;
	else if (icon->do_Gadget.SpecialInfo == (APTR)(icon + 1))
		return ICON_NEWICON;

	return ICON_NORMAL;
}
