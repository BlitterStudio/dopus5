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

#ifndef _DOPUS_MISC
#define _DOPUS_MISC

enum {
	TYPE_MENU,
	TYPE_HOTKEYS,
	TYPE_SCRIPTS,
};

#define READFILE_OK 0
#define READFILE_NOT_FOUND -1
#define READFILE_NO_MEMORY -2

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct
{
	ULONG command;
	struct Window *window;
	APTR data;
	IPCData *ipc;
} MiscStartup;

#ifndef __amigaos3__
	#pragma pack()
#endif

IPCData *misc_startup(char *, ULONG, struct Window *, APTR, BOOL);
IPC_EntryProto(misc_proc, extern);
long misc_check_quit(struct Screen *screen, IPCData *ipc);
void kill_pirates(void);

// Prototypes
void getsizestring(char *, ULONG);
BOOL copy_string(char *, char **, struct DOpusRemember **);
void do_title_string(char *, char *, char *);
char *get_our_pubscreen(void);
const char *strstri(const char *, const char *);
struct Node *next_node(struct List *, struct Node *, int);
struct IntuiMessage *get_imsg(struct Window *);
void lock_listlock(struct ListLock *list, BOOL exclusive);
void unlock_listlock(struct ListLock *list);
int request_file(struct Window *parent, char *title, char *buffer, char *def, ULONG flags, char *pattern);
void flush_port(struct MsgPort *port);
BOOL strreplace(char *, char *, char *, BOOL);
void get_screen_data(DOpusScreenData *data);
char *arg_get_word(char *args, char *buffer, int buflen);
BOOL arg_match_keyword(char *args, char *word);
void protect_get_string(ULONG protect, char *buffer);
long prot_from_string(char *buf);
long netprot_from_string(char *buf);
void abort_io(struct IORequest *ioreq);
#ifdef __amigaos4__
	#undef GetDiskInfo
#endif
LONG GetDiskInfo(char *device, struct InfoData *info);
BOOL VolumePresent(DirBuffer *buffer);
BOOL disk_eject_available(char *name);
void disk_eject(struct Window *window, char *names);
char *find_public_screen(struct Screen *screen, BOOL lock);
Cfg_Function *match_function_key(UWORD, UWORD, Cfg_ButtonBank *, Lister *, ULONG, ULONG *);
Cfg_Function *match_function_key_list(UWORD, UWORD, Cfg_ButtonBank *, BOOL);
void fix_literals(char *dest, char *srce);
short error_saving(short err, struct Window *window, long txt, long buttons);
BOOL check_closescreen(struct Screen *screen);
BOOL check_qualifier(UWORD, UWORD, UWORD, UWORD);

#ifndef __amigaos3__
	#pragma pack(2)
#endif

struct read_startup
{
	struct List *files;
	char initial_search[80];
	IPCData *owner;
	struct IBox dims;
	BOOL got_pos;
	UWORD wheel_scroll_lines;
};

typedef struct
{
	struct _BackdropInfo *backdrop;
	struct _BackdropObject *object;
	UWORD qual;
	ULONG flags;
	char data[1];
} iconopen_packet;

#ifndef __amigaos3__
	#pragma pack()
#endif

iconopen_packet *get_icon_packet(struct _BackdropInfo *, struct _BackdropObject *, char *, UWORD);

struct IntuiMessage *check_refresh_msg(struct Window *window, ULONG mask);

#ifndef __amigaos3__
	#pragma pack(2)
#endif

struct Window *WhichWindow(struct Screen *screen, short x, short y, unsigned short flags);

#ifndef __amigaos3__
	#pragma pack()
#endif

#define WWF_LEAVE_LOCKED (1 << 0)
#define WWF_NO_LOCK (1 << 1)

BOOL isdevice(char *path);
void final_path(char *path, char *buf);

char *function_label(Cfg_ButtonFunction *func);
void build_version_string(char *buf, short ver, short rev, long days, short format);

BOOL file_trap_more(char *name, char *tool);

char *sufcmp(char *name, char *suffix);

short misc_check_change(struct Screen *screen, IPCData *ipc, Cfg_ButtonBank *bank, long string);

struct Library *OpenModule(char *);

void ChainTagItems(struct TagItem **list, struct TagItem *tags);

void handle_diskchange(DOpusNotify *);

void send_main_reset_cmd(ULONG, ULONG, APTR);

enum {
	KEYMATCH_INVALID,
	KEYMATCH_NONE,
	KEYMATCH_BUTTONS,
	KEYMATCH_TOOLBAR,
	KEYMATCH_LISTERMENU,
	KEYMATCH_MENU,
	KEYMATCH_START,
	KEYMATCH_HOTKEYS,
	KEYMATCH_SCRIPTS,
};

void key_finder(IPCData *ipc);

BOOL GetFileInfo(char *name, struct FileInfoBlock *fib);

#ifdef __amigaos4__
// Open library and get interface for OS4
BOOL OpenLibIFace(char *name, APTR *libBase, APTR *iface, short version);
#endif

#endif
