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

// LoadDOpusLib: open dopus5.library and leave it open.
//
// The library installs its Workbench patches when it is first opened and
// keeps them while it stays open. Run from S:User-Startup, before LoadWB,
// it catches the AppIcons that programs in SYS:WBStartup add, so a
// Directory Opus started later shows them on its desktop. The library is
// never closed, so it stays in memory until the next reboot.

#include <dopus/common.h>
#include <dopus/version.h>

const char USED_VAR version[] = "\0$VER: LoadDOpusLib " CMD_STRING;

int main(void)
{
	if (!OpenLibrary("DOpus5:Libs/dopus5.library", LIB_VERSION))
	{
		PutStr("LoadDOpusLib: cannot open DOpus5:Libs/dopus5.library\n");
		return RETURN_WARN;
	}

	return RETURN_OK;
}
