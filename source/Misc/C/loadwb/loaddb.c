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

#include <dopus/common.h>
#include <proto/dopus5.h>

#if defined(__amigaos3__) || defined(__AROS__) || defined(__amigaos4__)
struct Device *InputBase = NULL;
#else
struct Library *InputBase = NULL;
#endif

struct Library *DOpusBase = NULL;

#ifdef __amigaos4__
struct InputIFace *IInput = NULL;
struct DOpusIFace *IDOpus = NULL;
#endif

const char USED_VAR version[] = "\0$VER: LoadDB " CMD_STRING;

enum { ARG_DEBUG, ARG_DELAY, ARG_CLEANUP, ARG_NEWPATH, ARG_STARTUPDRAWER, ARG_COUNT };

#ifdef SYS_Error
	#define DETACH_Error SYS_Error
#elif defined(NP_Error)
	#define DETACH_Error NP_Error
#else
	#define DETACH_Error TAG_IGNORE
#endif

BOOL launch_detached(char *command);
BOOL workbench_running(void);

int main(int argc, char **arg_string)
{
	BOOL ok = 0, run_wb = 0, wb_startup = 1, wb_running;
	BPTR lock;
	struct Process *proc;
	APTR wsave;
	char buf[640];
	struct IOStdReq input_req;
	short a;
	char *arg_ptr = 0;
	char arg_buf[256];
	char startup_drawer[256];
	struct RDArgs *args;
	IPTR arg_array[ARG_COUNT];

	// Initialise arguments
	arg_buf[0] = 0;
	startup_drawer[0] = 0;
	for (a = 0; a < ARG_COUNT; a++)
		arg_array[a] = 0;

	// Parse arguments
	if ((args = ReadArgs("-DEBUG/S,DELAY/S,CLEANUP/S,NEWPATH/S,STARTUPDRAWER/K", (APTR)arg_array, NULL)))
	{
		// Keep the drawer name; ReadArgs owns the string
		if (arg_array[ARG_STARTUPDRAWER])
		{
			strncpy(startup_drawer, (char *)arg_array[ARG_STARTUPDRAWER], sizeof(startup_drawer) - 1);
			startup_drawer[sizeof(startup_drawer) - 1] = 0;
		}

		// Free arguments
		FreeArgs(args);
	}

	// See if Workbench is running
	wb_running = workbench_running();

	// New path?
	if (arg_array[ARG_NEWPATH])
	{
		struct Library *DOpusBase;
		BOOL ok = 0;

		// See if DOpus is running
		if (FindPort("Directory Opus"))
		{
			// Open library
			if ((DOpusBase = OpenLibrary("DOpus5:Libs/dopus5.library", LIB_VERSION)))
			{
#ifdef __amigaos4__
				IDOpus = (struct DOpusIFace *)GetInterface(DOpusBase, "main", 1, NULL);
#endif

				// Update the path list from current process
				UpdatePathList();

// Close library
#ifdef __amigaos4__
				DropInterface((struct Interface *)IDOpus);
#endif
				CloseLibrary(DOpusBase);

				// Set flag to say we got a new path
				ok = 1;
			}
		}

		// Is Workbench also running?
		if (wb_running)
		{
			// Set flag to pass through to Workbench
			run_wb = 1;
		}

		// Otherwise, exit now, unless Opus has to be launched
		else if (ok)
			return (0);
	}

	// Open input device

#ifdef __AROS__
	input_req.io_Message.mn_Length = sizeof(input_req);
#endif
	if (!run_wb && !(OpenDevice("input.device", 0, (struct IORequest *)&input_req, 0)))
	{
		// Get input base
		InputBase = (APTR)input_req.io_Device;
#ifdef __amigaos4__
		IInput = (struct InputIFace *)GetInterface((struct Library *)InputBase, "main", 1, NULL);
#endif

		// See if shift is held down
		if (PeekQualifier() & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
		{
			// Run Workbench instead
			run_wb = 1;
		}

		// Is control held down?
		else if (PeekQualifier() & IEQUALIFIER_CONTROL)
		{
			// No WBStartup
			wb_startup = 0;
		}

// Close input device
#ifdef __amigaos4__
		DropInterface((struct Interface *)IInput);
#endif
		CloseDevice((struct IORequest *)&input_req);
	}

	// Find argument pointer
	for (a = 1; a < argc; a++)
	{
		size_t len = strlen(arg_buf);

		// STARTUPDRAWER goes to Opus quoted, from ReadArgs, so a name with
		// spaces survives; leave it out here, with its value
		if (startup_drawer[0] && strnicmp(arg_string[a], "STARTUPDRAWER", 13) == 0 &&
			(arg_string[a][13] == '=' || arg_string[a][13] == 0))
		{
			if (arg_string[a][13] == 0)
				a++;
			else if (arg_string[a][14] == '"')
			{
				// STARTUPDRAWER="a b" can arrive split at the space; skip up
				// to the argument with the closing quote
				char *end = arg_string[a] + 15;

				while (!*end || end[strlen(end) - 1] != '"')
				{
					if (++a >= argc)
						break;
					end = arg_string[a];
				}
			}
			continue;
		}

		if (len < sizeof(arg_buf) - 1 && arg_buf[0])
		{
			arg_buf[len++] = ' ';
			arg_buf[len] = 0;
		}
		strncat(arg_buf, arg_string[a], sizeof(arg_buf) - strlen(arg_buf) - 1);
	}
	if (arg_buf[0])
		arg_ptr = arg_buf;

	// Try to run DOpus?
	if (!run_wb)
	{
		// See if Workbench is already running; if so, we don't do WBStartup again
		if (wb_running)
			wb_startup = 0;

		// Get this process, turn off requesters
		proc = (struct Process *)FindTask(NULL);
		wsave = proc->pr_WindowPtr;
		proc->pr_WindowPtr = (APTR)-1;

		// See if DOpus is there
		if ((lock = Lock("DOpus5:DirectoryOpus", ACCESS_READ)))
		{
			// Yup
			UnLock(lock);

			// Open library
			// if ((DOpusBase=OpenLibrary("DOpus5:Libs/dopus5.library",0)))
			{
				/*#ifdef __amigaos4__
				IDOpus = (struct DOpusIFace *)GetInterface(DOpusBase, "main", 1, NULL);
				#endif

				#ifdef __amigaos4__
				DropInterface((struct Interface *)IDOpus);
				#endif
				CloseLibrary(DOpusBase);*/

				// Build command string
				strcpy(buf, "DOpus5:DirectoryOpus ENVIRONMENT=workbench ");
				if (wb_startup)
				{
					strcat(buf, " WBSTARTUP=YES ");
					if (startup_drawer[0])
					{
						strcat(buf, "\"STARTUPDRAWER=");
						strcat(buf, startup_drawer);
						strcat(buf, "\" ");
					}
				}
				if (arg_ptr)
					strcat(buf, arg_ptr);

				// Strip LF
				for (arg_ptr = buf; *arg_ptr; arg_ptr++)
					if (*arg_ptr == '\n')
						*arg_ptr = 0;

				// Try launch
				ok = launch_detached(buf);
			}
		}

		// Restore requesters
		proc->pr_WindowPtr = wsave;
	}

	// Did we fail?
	if (!ok)
	{
		// Failed for some reason; start old-fashioned Workbench
		strcpy(buf, "c:LoadWB");

		// Add Workbench arguments
		if (arg_array[ARG_DEBUG])
			strcat(buf, " -DEBUG");
		if (arg_array[ARG_DELAY])
			strcat(buf, " DELAY");
		if (arg_array[ARG_CLEANUP])
			strcat(buf, " CLEANUP");
		if (arg_array[ARG_NEWPATH])
			strcat(buf, " NEWPATH");

		// Run it
		launch_detached(buf);
	}

	return (0);
}

// Launch without keeping the startup shell/script process or its files open.
//
// SystemTags() in async mode (SYS_Asynch=TRUE) takes ownership of the input/
// output/error filehandles and closes them when the launched command exits.
// If we pass 0 for those tags, dos.library clones the *caller's* Input() and
// Output() handles for the new process - which keeps the parent CLI/shell
// pinned open until DOpus exits, even though LoadDB itself has already
// returned (see issue #56). Open explicit NIL: streams instead so the
// launched process holds zero references to our shell.
BOOL launch_detached(char *command)
{
	BPTR nil_in = 0, nil_out = 0, nil_err = 0;
	BOOL ok = FALSE;

	nil_in  = Open("NIL:", MODE_OLDFILE);
	nil_out = Open("NIL:", MODE_NEWFILE);
	nil_err = Open("NIL:", MODE_NEWFILE);

	if (nil_in && nil_out && nil_err)
	{
		ok = (SystemTags(command,
						 SYS_Input,     nil_in,
						 SYS_Output,    nil_out,
						 DETACH_Error,  nil_err,
						 SYS_Asynch,    TRUE,
						 SYS_UserShell, TRUE,
						 NP_Cli,        TRUE,
						 TAG_DONE) != -1);
	}

	if (!ok)
	{
		// SystemTags failed (or NIL: open failed); we still own the handles.
		if (nil_in)  Close(nil_in);
		if (nil_out) Close(nil_out);
		if (nil_err) Close(nil_err);
	}
	// On success, SystemTags() owns the handles and closes them when the
	// async command finishes.

	return ok;
}

// See if Workbench is running
BOOL workbench_running(void)
{
	struct Screen *screen;
	struct List *list;
	BOOL wb = 0;

	// Lock public screen list
	if ((list = LockPubScreenList()))
	{
		struct PubScreenNode *pub;
		BOOL ok = 0;

		// Go through screens
		for (pub = (struct PubScreenNode *)list->lh_Head; pub->psn_Node.ln_Succ;
			 pub = (struct PubScreenNode *)pub->psn_Node.ln_Succ)
		{
			// Workbench screen?
			if (stricmp(pub->psn_Node.ln_Name, "Workbench") == 0)
			{
				ok = 1;
				break;
			}
		}

		// Unlock list
		UnlockPubScreenList();

		// If Workbench screen isn't open, don't try to lock it
		if (!ok)
			return 0;
	}

	// Lock Workbench screen
	if ((screen = LockPubScreen("Workbench")))
	{
		struct Window *window;
		ULONG lock;

		// Lock Intuition
		lock = LockIBase(0);

		// Go through windows on screen
		for (window = screen->FirstWindow; window; window = window->NextWindow)
		{
			// Look for Workbench windows
			if (window->Flags & WFLG_WBENCHWINDOW)
			{
				wb = 1;
				break;
			}
		}

		// Unlock Intuition, and screen
		UnlockIBase(lock);
		UnlockPubScreen(0, screen);
	}

	return wb;
}
