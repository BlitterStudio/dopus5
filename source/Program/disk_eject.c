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

#include "dopus.h"
#include <devices/scsidisk.h>
#include <exec/errors.h>

#define EJECT_DONE 0
#define EJECT_UNSUPPORTED 1
#define EJECT_FAILED 2

#define SCSI_INQUIRY 0x12
#define SCSI_START_STOP_UNIT 0x1b
#define SCSI_PREVENT_ALLOW_REMOVAL 0x1e

#define SCSI_INQUIRY_LEN 36
#define SCSI_SENSE_LEN 18

#define EJECT_NAME_LEN 32

typedef struct
{
	char device[128];	// Exec device driving the medium
	ULONG unit;
	ULONG flags;
	char *names;		// Every mounted DOS device on the unit, eg "CD0:"
	short count;
} EjectTarget;

typedef struct
{
	char device[128];
	ULONG unit;
} EjectUnit;

// Collect every mounted DOS device on the target's unit. Ejecting takes
// the medium away from all partitions on it, so they must all be inhibited.
static BOOL disk_eject_partitions(EjectTarget *target)
{
	struct DosList *list, *dos;
	char device[128];
	short pass, count = 0;

	list = LockDosList(LDF_DEVICES | LDF_READ);

	// Count them, then copy their names
	for (pass = 0; pass < 2; pass++)
	{
		for (dos = list; (dos = NextDosEntry(dos, LDF_DEVICES));)
		{
			struct FileSysStartupMsg *fssm;
			char *name;

			// No running handler means nothing to write back
			if (!dos->dol_Task || !GetDeviceUnit(dos->dol_misc.dol_handler.dol_Startup, 0, 0))
				continue;

			fssm = (struct FileSysStartupMsg *)BADDR(dos->dol_misc.dol_handler.dol_Startup);
			if (fssm->fssm_Unit != target->unit)
				continue;
			BtoCStr(fssm->fssm_Device, device, sizeof(device));
			if (stricmp(device, target->device) != 0)
				continue;

			if (pass)
			{
				name = target->names + count * EJECT_NAME_LEN;
				BtoCStr(dos->dol_Name, name, EJECT_NAME_LEN - 1);
				strcat(name, ":");
			}
			++count;
		}

		if (!pass)
		{
			if (!count || !(target->names = AllocVec(count * EJECT_NAME_LEN, MEMF_CLEAR)))
				break;
			count = 0;
		}
		else
			target->count = count;
	}

	UnLockDosList(LDF_DEVICES | LDF_READ);
	return (target->count > 0);
}

// Find the exec device behind a disk or volume name, and with a target,
// every DOS device sharing its unit
static BOOL disk_eject_target(char *name, EjectTarget *target)
{
	struct DevProc *proc;
	struct DosList *dos;
	BOOL ok = FALSE;

	if (target)
	{
		target->names = 0;
		target->count = 0;
	}

	if (!(proc = GetDeviceProc(name, 0)))
		return FALSE;

	// An assign doesn't own the medium it points into; RAM: and network
	// handlers have no startup message naming an exec device
	if (!(proc->dvp_Flags & DVPF_ASSIGN) && (dos = DeviceFromHandler(proc->dvp_Port, 0)) &&
		GetDeviceUnit(dos->dol_misc.dol_handler.dol_Startup, 0, 0))
	{
		if (target)
		{
			struct FileSysStartupMsg *fssm = (struct FileSysStartupMsg *)BADDR(dos->dol_misc.dol_handler.dol_Startup);

			BtoCStr(fssm->fssm_Device, target->device, sizeof(target->device));
			target->unit = fssm->fssm_Unit;
			target->flags = fssm->fssm_Flags;
		}
		ok = TRUE;
	}

	FreeDeviceProc(proc);
	if (ok && target)
		ok = disk_eject_partitions(target);
	return ok;
}

// Inhibit every partition on the unit, or release them again
static BOOL disk_eject_inhibit(EjectTarget *target, BOOL inhibit)
{
	short num;
	LONG error;

	for (num = 0; num < target->count; num++)
	{
		if (!Inhibit(target->names + num * EJECT_NAME_LEN, (inhibit) ? DOSTRUE : DOSFALSE) && inhibit)
		{
			// Release the ones already inhibited, keeping the error
			error = IoErr();
			while (num-- > 0)
				Inhibit(target->names + num * EJECT_NAME_LEN, DOSFALSE);
			SetIoErr(error);
			return FALSE;
		}
	}

	return TRUE;
}

// Can this disk be offered for ejection?
BOOL disk_eject_available(char *name)
{
	return disk_eject_target(name, 0);
}

// Send a six byte SCSI command; TRUE if it completed with good status
static BOOL disk_eject_scsi(struct IOStdReq *io, UBYTE *cdb, UBYTE *data, ULONG length, UBYTE *sense, ULONG *actual)
{
	struct SCSICmd cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.scsi_Data = (UWORD *)data;
	cmd.scsi_Length = length;
	cmd.scsi_Command = cdb;
	cmd.scsi_CmdLength = 6;
	cmd.scsi_Flags = SCSIF_AUTOSENSE | ((data) ? SCSIF_READ : SCSIF_WRITE);
	cmd.scsi_SenseData = sense;
	cmd.scsi_SenseLength = SCSI_SENSE_LEN;

	io->io_Command = HD_SCSICMD;
	io->io_Data = &cmd;
	io->io_Length = sizeof(cmd);

	if (DoIO((struct IORequest *)io) || cmd.scsi_Status)
		return FALSE;

	if (actual)
		*actual = cmd.scsi_Actual;
	return TRUE;
}

// Eject the medium from an open unit
static short disk_eject_unit(EjectTarget *target, struct IOStdReq *io, UBYTE *buffer)
{
	struct DriveGeometry geo;
	UBYTE cdb[6], *sense = buffer + SCSI_INQUIRY_LEN;
	ULONG actual = 0;
	BOOL scsi = FALSE, ok;
	BYTE error;

	// SCSI and ATAPI drivers report removable media in the INQUIRY data.
	// Checking first keeps START STOP UNIT away from fixed disks, which
	// would spin them down.
	memset(cdb, 0, sizeof(cdb));
	cdb[0] = SCSI_INQUIRY;
	cdb[4] = SCSI_INQUIRY_LEN;
	if (disk_eject_scsi(io, cdb, buffer, SCSI_INQUIRY_LEN, sense, &actual) && actual >= 2)
	{
		if (!(buffer[1] & 0x80))
			return EJECT_UNSUPPORTED;
		scsi = TRUE;
	}

	// trackdisk-style drivers report it in the drive geometry
	else
	{
		io->io_Command = TD_GETGEOMETRY;
		io->io_Data = &geo;
		io->io_Length = sizeof(geo);
		if (DoIO((struct IORequest *)io) == 0 && !(geo.dg_Flags & DGF_REMOVABLE))
			return EJECT_UNSUPPORTED;
	}

	// Nothing to do if the medium has gone already; an empty tray stays shut
	io->io_Command = TD_CHANGESTATE;
	io->io_Data = 0;
	io->io_Length = 0;
	if (DoIO((struct IORequest *)io) == 0 && io->io_Actual)
		return EJECT_DONE;

	// Make the filesystems write back and release the medium
	if (!disk_eject_inhibit(target, TRUE))
		return EJECT_FAILED;

	if (scsi)
	{
		// Unlock the tray in case something locked it; this may fail
		memset(cdb, 0, sizeof(cdb));
		cdb[0] = SCSI_PREVENT_ALLOW_REMOVAL;
		disk_eject_scsi(io, cdb, 0, 0, sense, 0);

		// Stop the unit and eject (LoEj)
		cdb[0] = SCSI_START_STOP_UNIT;
		cdb[4] = 0x02;
		ok = disk_eject_scsi(io, cdb, 0, 0, sense, 0);
	}
	else
	{
		io->io_Command = TD_EJECT;
		io->io_Data = 0;
		io->io_Length = 1;
		ok = (DoIO((struct IORequest *)io) == 0);
	}
	error = io->io_Error;

	// Let the filesystems see the empty drive
	disk_eject_inhibit(target, FALSE);

	if (ok)
		return EJECT_DONE;
	if (error == IOERR_NOCMD)
		return EJECT_UNSUPPORTED;

	// A device error has no DOS error text
	SetIoErr(0);
	return EJECT_FAILED;
}

// Open the unit behind a disk and eject its medium
static short disk_eject_media(EjectTarget *target)
{
	struct MsgPort *port;
	struct IOStdReq *io;
	UBYTE *buffer;
	short ret = EJECT_FAILED;

	SetIoErr(ERROR_NO_FREE_STORE);

	// INQUIRY data and sense data may be transferred by DMA
	if (!(buffer = AllocVec(SCSI_INQUIRY_LEN + SCSI_SENSE_LEN, MEMF_PUBLIC | MEMF_CLEAR)))
		return EJECT_FAILED;

	if ((port = CreateMsgPort()))
	{
		if ((io = (struct IOStdReq *)CreateIORequest(port, sizeof(struct IOExtTD))))
		{
			if (OpenDevice(target->device, target->unit, (struct IORequest *)io, target->flags) == 0)
			{
				ret = disk_eject_unit(target, io, buffer);
				CloseDevice((struct IORequest *)io);
			}
			else
				SetIoErr(0);
			DeleteIORequest((struct IORequest *)io);
		}
		DeleteMsgPort(port);
	}

	FreeVec(buffer);
	return ret;
}

// Eject one disk, reporting failures. Units already in done[] are skipped:
// selecting two partitions of one medium ejects it once.
static void disk_eject_one(struct Window *window, char *name, EjectUnit *done, short *done_count)
{
	EjectTarget target;
	BOOL resolved = FALSE;
	short ret, num;

	FOREVER
	{
		if (!disk_eject_target(name, &target))
		{
			FreeVec(target.names);
			ret = EJECT_UNSUPPORTED;
			break;
		}

		// Same unit as an earlier disk in this request?
		for (num = 0; num < *done_count; num++)
		{
			if (done[num].unit == target.unit && stricmp(done[num].device, target.device) == 0)
				break;
		}
		if (num < *done_count)
		{
			FreeVec(target.names);
			return;
		}
		resolved = TRUE;

		ret = disk_eject_media(&target);
		FreeVec(target.names);
		if (ret != EJECT_FAILED)
			break;

		if (!(error_request(window, ERF_MOUSE, (IPTR)GetString(&locale, MSG_EJECTING), -1, (IPTR)name, 0)))
			break;
	}

	// Don't ask about this unit again
	if (resolved)
	{
		strcpy(done[*done_count].device, target.device);
		done[*done_count].unit = target.unit;
		++*done_count;
	}

	if (ret == EJECT_UNSUPPORTED)
	{
		char buffer[256];

		lsprintf(buffer, GetString(&locale, MSG_EJECT_UNSUPPORTED), (IPTR)name);
		super_request_args(window, buffer, SRF_MOUSE_POS, (IPTR)GetString(&locale, MSG_OKAY), 0);
	}
}

// Eject a list of disks (runs in its own process). The names are
// null-terminated strings, ended by an empty one.
void disk_eject(struct Window *window, char *names)
{
	EjectUnit *done;
	short count = 0, done_count = 0;
	char *name;

	// Another eject would release our inhibited partitions, or eject
	// the same medium twice; only one runs at a time
	if (!AttemptSemaphore(&GUI->eject_lock))
	{
		DisplayBeep((window) ? window->WScreen : 0);
		return;
	}

	for (name = names; *name; name += strlen(name) + 1)
		++count;

	if ((done = AllocVec(count * sizeof(EjectUnit), MEMF_CLEAR)))
	{
		for (name = names; *name; name += strlen(name) + 1)
			disk_eject_one(window, name, done, &done_count);
		FreeVec(done);
	}

	ReleaseSemaphore(&GUI->eject_lock);
}

// Eject the selected disks, or the one supplied
void backdrop_eject(BackdropInfo *info, BackdropObject *icon)
{
	BackdropObject *object;
	char *names = 0, *ptr = 0;
	long size = 1;
	short pass;

	lock_listlock(&info->objects, 0);

	// Measure the names, then copy them into one list
	for (pass = 0; pass < 2; pass++)
	{
		for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ;
			 object = (BackdropObject *)object->node.ln_Succ)
		{
			if (object->type != BDO_DISK && object->type != BDO_BAD_DISK)
				continue;

			if ((!icon && object->state) || icon == object)
			{
				if (object->device_name && !(object->flags & (BDOF_ASSIGN | BDOF_CACHE)))
				{
					if (pass)
					{
						strcpy(ptr, object->device_name);
						ptr += strlen(ptr) + 1;
					}
					else
						size += strlen(object->device_name) + 1;
				}

				if (icon)
					break;
			}
		}

		if (!pass && (size == 1 || !(names = ptr = AllocVec(size, MEMF_CLEAR))))
			break;
	}

	unlock_listlock(&info->objects);

	// Nothing to eject
	if (!names)
	{
		DisplayBeep(info->window->WScreen);
		return;
	}

	// One process for the lot, so partitions of one medium are ejected once
	if (!(misc_startup("dopus_disk_eject", MENU_ICON_EJECT, info->window, names, 0)))
		FreeVec(names);
}
