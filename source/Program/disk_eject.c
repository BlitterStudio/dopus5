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

typedef struct
{
	char dos_name[32];	// DOS device to inhibit, eg "CD0:"
	char device[128];	// Exec device driving it
	ULONG unit;
	ULONG flags;
} EjectTarget;

// Find the DOS device and exec device behind a disk or volume name
static BOOL disk_eject_target(char *name, EjectTarget *target)
{
	struct DevProc *proc;
	struct DosList *dos;
	BOOL ok = FALSE;

	if (!(proc = GetDeviceProc(name, 0)))
		return FALSE;

	// An assign doesn't own the medium it points into; RAM: and network
	// handlers have no startup message naming an exec device
	if (!(proc->dvp_Flags & DVPF_ASSIGN) && (dos = DeviceFromHandler(proc->dvp_Port, (target) ? target->dos_name : 0)) &&
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
	return ok;
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

	// Make the filesystem write back and release the medium
	if (!Inhibit(target->dos_name, DOSTRUE))
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

	// Let the filesystem see the empty drive
	Inhibit(target->dos_name, DOSFALSE);

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

// Eject a disk, reporting failures (runs in its own process)
void disk_eject(struct Window *window, char *name)
{
	EjectTarget target;
	short ret;

	FOREVER
	{
		if (!disk_eject_target(name, &target))
		{
			ret = EJECT_UNSUPPORTED;
			break;
		}

		if ((ret = disk_eject_media(&target)) != EJECT_FAILED)
			break;

		if (!(error_request(window, ERF_MOUSE, (IPTR)GetString(&locale, MSG_EJECTING), -1, (IPTR)name, 0)))
			break;
	}

	if (ret == EJECT_UNSUPPORTED)
	{
		char buffer[256];

		lsprintf(buffer, GetString(&locale, MSG_EJECT_UNSUPPORTED), (IPTR)name);
		super_request_args(window, buffer, SRF_MOUSE_POS, (IPTR)GetString(&locale, MSG_OKAY), 0);
	}
}

// Start ejecting a disk
static void disk_eject_launch(struct Window *window, char *name)
{
	char *copy;

	if ((copy = AllocVec(strlen(name) + 1, 0)))
	{
		strcpy(copy, name);
		if (!(misc_startup("dopus_disk_eject", MENU_ICON_EJECT, window, copy, 0)))
			FreeVec(copy);
	}
}

// Eject the selected disks, or the one supplied
void backdrop_eject(BackdropInfo *info, BackdropObject *icon)
{
	BackdropObject *object;
	short count = 0;

	lock_listlock(&info->objects, 0);

	for (object = (BackdropObject *)info->objects.list.lh_Head; object->node.ln_Succ;
		 object = (BackdropObject *)object->node.ln_Succ)
	{
		if (object->type != BDO_DISK && object->type != BDO_BAD_DISK)
			continue;

		if ((!icon && object->state) || icon == object)
		{
			if (object->device_name && !(object->flags & (BDOF_ASSIGN | BDOF_CACHE)))
			{
				disk_eject_launch(info->window, object->device_name);
				++count;
			}

			if (icon)
				break;
		}
	}

	unlock_listlock(&info->objects);

	// Nothing to eject
	if (!count)
		DisplayBeep(info->window->WScreen);
}
