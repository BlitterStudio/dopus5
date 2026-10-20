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

#include "misc.h"
#include <devices/audio.h>

int LIBFUNC L_Module_Entry(REG(a0, struct List *files),
						   REG(a1, struct Screen *screen),
						   REG(a2, IPCData *ipc),
						   REG(a3, IPCData *main_ipc),
						   REG(d0, IPTR mod_id),
						   REG(d1, IPTR mod_data))
{
	struct IOAudio audio;
	struct MsgPort *reply_port;

	// Flash only?
	if (mod_id == FLASH)
	{
		DisplayBeep(screen);
		return 1;
	}

	// Create port
	if (!(reply_port = CreateMsgPort()))
		return 0;

	// Fill out audio request
	audio.ioa_Request.io_Message.mn_ReplyPort = reply_port;
	audio.ioa_Request.io_Message.mn_Node.ln_Pri = 90;
	audio.ioa_Data = achannels;
	audio.ioa_Length = sizeof(achannels);

	// Open audio device
	if (!(OpenDevice("audio.device", 0, (struct IORequest *)&audio, 0)))
	{
		// Initialise write command
		audio.ioa_Request.io_Command = CMD_WRITE;
		audio.ioa_Request.io_Flags = ADIOF_PERVOL;
		audio.ioa_Volume = 64;
		audio.ioa_Data = (UBYTE *)beepwave;

		// It's an alarm?
		if (mod_id == ALARM)
		{
			short a;

			// Waveform size
			audio.ioa_Length = 8;

			// Standard length
			audio.ioa_Cycles = 60;

			// Five cycles
			for (a = 0; a < 5; a++)
			{
				// High tone
				audio.ioa_Period = 800;

				// Play sound
				BeginIO((struct IORequest *)&audio);
				WaitIO((struct IORequest *)&audio);

				// Low tone
				audio.ioa_Period = 1200;

				// Play sound
				BeginIO((struct IORequest *)&audio);
				WaitIO((struct IORequest *)&audio);
			}
		}

		// Waveform size
		else
			audio.ioa_Length = sizeof(beepwave);

		// Set note information
		audio.ioa_Period = (mod_id == BEEP) ? 300 : 800;
		audio.ioa_Cycles = (mod_id == BEEP) ? 100 : 60;

		// Play sound
		BeginIO((struct IORequest *)&audio);
		WaitIO((struct IORequest *)&audio);

		// Close audio device
		CloseDevice((struct IORequest *)&audio);
	}

	// Delete message port
	DeleteMsgPort(reply_port);
	return 1;
}
