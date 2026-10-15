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

#ifndef _DOPUS_REXX
#define _DOPUS_REXX

#define RXERRORIMGONE 100  // Error to indicate shutting down
#define RXERRORNOCMD 30	   // Error to indicate invalid command

#define RX_STRING 0
#define RX_LONG 1
#define RX_CHAR 2

#define RXENTRY_FILE -1
#define RXENTRY_FILE_COLOUR -2
#define RXENTRY_FILE_LINK -3
#define RXENTRY_FILE_COLOUR_LINK -4

#define RXENTRY_DIR 1
#define RXENTRY_DIR_COLOUR 2
#define RXENTRY_DIR_LINK 3
#define RXENTRY_DIR_COLOUR_LINK 4

#define KEY_NONE 0
#define KEY_ADD 1
#define KEY_SUB -1
#define KEY_TOGGLE -2

#define RXMF_WARN (1 << 0)
#define RXMF_NO_INC (1 << 1)
#define RXMF_SYNC (1 << 2)

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct
{
	struct Message msg;
	struct RexxMsg *rmsg;
} RexxSyncMessage;

typedef struct
{
	char *command;
	short id;
} RexxCommandList;

typedef struct
{
	struct RexxMsg *msg;
	long command;
	struct Window *window;
	char args[1];
} RexxRequester;

#ifndef __amigaos3__
	#pragma pack()
#endif

extern RexxCommandList rexx_commands[];
extern short rexx_commands_can_do_all[];
extern char *sort_method_strings[], *separate_method_strings[], *sort_flags_strings[], *full_string[], *quick_string[],
	*on_off_strings[], *on_off_strings2[], *force_string[], *getfiletype_keys[], *custhandler_keys[], *port_keys[],
	*all_keys[], *new_lister_keys[], *mode_keys[], *quit_keys[], *reader_keys[], *command_keys[], *pos_keys[],
	*seticon_keys[], *app_commands[], *progress_keys[], *secure_keys[], *excommand_keys[], *lock_keys[], *wait_keys[],
	*move_keys[], *update_key[], *background_key[], *custom_key[], *precision_key[], *pens_key[], *font_key[];

extern unsigned short rexx_error_table[];

enum {
	RXCMD_BASE,

	RXCMD_LISTER,
	RXCMD_NEW,
	RXCMD_CLOSE,
	RXCMD_QUERY,
	RXCMD_SET,
	RXCMD_ADD,
	RXCMD_REMOVE,
	RXCMD_SELECT,
	RXCMD_REFRESH,
	RXCMD_CLEAR,
	RXCMD_EMPTY,
	RXCMD_WAIT,
	RXCMD_PATH,
	RXCMD_POSITION,
	RXCMD_BUSY,
	RXCMD_HANDLER,
	RXCMD_VISIBLE,
	RXCMD_FILES,
	RXCMD_DIRS,
	RXCMD_ENTRIES,
	RXCMD_SELFILES,
	RXCMD_SELDIRS,
	RXCMD_SELENTRIES,
	RXCMD_ENTRY,
	RXCMD_NUMFILES,
	RXCMD_NUMDIRS,
	RXCMD_NUMENTRIES,
	RXCMD_NUMBYTES,
	RXCMD_NUMSELFILES,
	RXCMD_NUMSELDIRS,
	RXCMD_NUMSELENTRIES,
	RXCMD_NUMSELBYTES,
	RXCMD_SORT,
	RXCMD_SEPARATE,
	RXCMD_DISPLAY,
	RXCMD_FLAGS,
	RXCMD_SOURCE,
	RXCMD_DEST,
	RXCMD_ALL,
	RXCMD_SHOW,
	RXCMD_HIDE,
	RXCMD_TITLE,
	RXCMD_READ,
	RXCMD_COPY,
	RXCMD_DOPUS,
	RXCMD_BACK,
	RXCMD_FRONT,
	RXCMD_PROGRESS,
	RXCMD_COUNT,
	RXCMD_NAME,
	RXCMD_ABORT,
	RXCMD_FIRSTSEL,
	RXCMD_GETSTRING,
	RXCMD_REQUEST,
	RXCMD_LOCK,
	RXCMD_OFF,
	RXCMD_GETFILETYPE,
	RXCMD_COMMAND,
	RXCMD_ADDTRAP,
	RXCMD_REMTRAP,
	RXCMD_ICONIFY,
	RXCMD_VERSION,
	RXCMD_TOOLBAR,
	RXCMD_MODE,
	RXCMD_SCREEN,
	RXCMD_CASE,
	RXCMD_NAMELENGTH,
	RXCMD_ERROR,
	RXCMD_ADDAPPICON,
	RXCMD_REMAPPICON,
	RXCMD_HEXREAD,
	RXCMD_ANSIREAD,
	RXCMD_SMARTREAD,
	RXCMD_NEWPROGRESS,
	RXCMD_LABEL,
	RXCMD_SETAPPICON,
	RXCMD_ADD_STEM,
	RXCMD_HEADER,
	RXCMD_FIELD,
	RXCMD_SEND,
	RXCMD_RX,
	RXCMD_MOVEMENT,
	RXCMD_RELOAD,
	RXCMD_WINDOW,
	RXCMD_FINDCACHE,
	RXCMD_FREECACHES,
	RXCMD_CHECKDESKTOP,
	RXCMD_GETDESKTOP,
	RXCMD_DEFAULT,
	RXCMD_PROC,
	RXCMD_ACTIVE,
	RXCMD_SCRIPT,
	RXCMD_DESKTOPPOPUP,
	RXCMD_BACKGROUND,
	RXCMD_SOUND,
	RXCMD_PENS,
	RXCMD_PALETTE,
	RXCMD_FONT,
	RXCMD_ICONS,
	RXCMD_MATCHDESKTOP,
	RXCMD_VALUE,
	RXCMD_COMMENTLENGTH,
};

enum {
	RXSORTF_REVERSE,
	RXSORTF_NOICONS,
	RXSORTF_HIDDEN,
};

enum {
	RXCOM_WAIT,
	RXCOM_SOURCE,
	RXCOM_DEST,
	RXCOM_ORIGINAL,
};

enum {
	RXPROG_NAME,
	RXPROG_FILE,
	RXPROG_INFO,
	RXPROG_BAR,
	RXPROG_ABORT,
	RXPROG_TITLE,
	RXPROG_OFF,
	RXPROG_INFO2,
	RXPROG_INFO3,
};

// Error codes (RC)
#define RXERR_OK 0
#define RXERR_FILE_REJECTED 1	 // File was rejected
#define RXERR_INVALID_QUERY 5	 // Query item invalid
#define RXERR_INVALID_SET 5		 // Set item invalid
#define RXERR_INVALID_NAME 6	 // Name invalid
#define RXERR_INVALID_KEYWORD 6	 // Keyword invalid
#define RXERR_INVALID_TRAP 8	 // Trap invalid
#define RXERR_INVALID_HANDLE 10	 // Lister handle invalid
#define RXERR_NO_TOOLBAR 12		 // No toolbar
#define RXERR_INVALID_PATH 13	 // Bad path
#define RXERR_INVALID_FILE 14	 // Bad file
#define RXERR_NO_MEMORY 15		 // No memory to do something
#define RXERR_NO_LISTER 20		 // Lister couldn't open

#ifndef __amigaos3__
	#pragma pack(2)
#endif

// Function trap
typedef struct _FunctionTrap
{
	struct MinNode node;
	char *command;	   // Command name
	char handler[32];  // Custom handler
	ULONG flags;
	ULONG pad[4];
} FunctionTrap;

#ifndef __amigaos3__
	#pragma pack()
#endif

#define FTRAPF_SIGNAL (1 << 0)

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct _TrapSignal
{
	char port_name[40];
	short signal;
} TrapSignal;

typedef struct
{
	struct MinNode node;
	IPCData *ipc;
} RexxReader;

typedef struct
{
	struct Node node;
	APTR app_thing;
	struct DiskObject *icon;
	ULONG id;
	char icon_name[40];
	char port_name[80];
	long pos_x;
	long pos_y;
	unsigned long flags;
	struct DiskObject *busy_icon;
} RexxAppThing;

typedef struct
{
	struct Node node;
	APTR progress;
} RexxProgress;

#ifndef __amigaos3__
	#pragma pack()
#endif

enum {
	REXXAPP_ICON,
	REXXAPP_PROGRESS,

	REXXAPP_ALL = 100
};

#define RATF_QUOTES (1 << 0)  // Quoted filenames
#define RATF_INFO (1 << 1)	  // Supports Information
#define RATF_SNAP (1 << 2)	  // Supports Snapshot
#define RATF_CLOSE (1 << 3)	  // Close instead of Open
#define RATF_LOCAL (1 << 4)	  // Local only
#define RATF_LOCKED (1 << 5)  // Locked

enum {
	REXXAPPCMD_QUIT,
	REXXAPPCMD_DOUBLECLICK,
	REXXAPPCMD_DRAGNDROP,
	REXXAPPCMD_SNAPSHOT,
	REXXAPPCMD_UNSNAPSHOT,
	REXXAPPCMD_CLOSE,
	REXXAPPCMD_INFO,
	REXXAPPCMD_MENU,
	REXXAPPCMD_MENUHELP,
};

enum {
	READCOM_BASE,
	READCOM_READ,
};

IPC_EntryProto(rexx_proc, extern);
BOOL rexx_process_msg(struct RexxMsg *, struct MsgPort *, long *);
void rexx_set_return(struct RexxMsg *msg, long rc, char *result);
RexxReader *rexx_read_file(short, char *, struct RexxMsg *);
void rexx_goodbye_reader(IPCData *ipc);
void rexx_reply_msg(struct RexxMsg *rmsg);

void rexx_lister_new(struct RexxMsg *msg, char *args);
BOOL rexx_lister_cmd(struct RexxMsg *msg, short command, char *args);
void rexx_lister_file_return(struct RexxMsg *msg, DirBuffer *, short id, char, short, char *);
void rexx_lister_entry_info(struct RexxMsg *msg, DirBuffer *buffer, char *args, short, char *);
void rexx_lister_get_current(struct RexxMsg *, short, short, char *);
void rexx_query_format(short id, ListFormat *format, char *result);
long rexx_set_format(short command, short id, ListFormat *format, char *args);

BOOL rexx_dopus_cmd(struct RexxMsg *, short, char *);

void rexx_custom_app_msg(struct _DOpusAppMessage *msg,
						 DirBuffer *buffer,
						 char *action,
						 Lister *,
						 char *,
						 unsigned short);
char *rexx_build_filestring(struct _DOpusAppMessage *, IPTR *, ULONG);
short rexx_lister_add_file(Lister *lister, char *args, struct RexxMsg *);
DirEntry *rexx_lister_get_file(DirBuffer *buffer, char **args);
short rexx_lister_remove_file(Lister *lister, char *args);
short rexx_lister_select_file(Lister *lister, char *args, char *result);

short rexx_get_command(char **commandptr);
void rexx_skip_space(char **command);
void rexx_skip_space_reverse(char **command, char *);
long rexx_parse_number(char **ptr, BOOL, long);
IPTR rexx_parse_iptr(char **ptr, BOOL, IPTR);
void rexx_format_iptr(char *, IPTR);
BOOL rexx_parse_number_byte(char **, UBYTE *);
short rexx_parse_word(char **ptr, char *buffer, short bufferlen);
short rexx_match_keyword(char **ptr, char **keys, long *);
BOOL rexx_lister_valid(Lister *lister);
void rexx_set_var(struct RexxMsg *, char *, char *, IPTR, short);
BOOL rexx_get_var(struct RexxMsg *, char *, char *, char *, short);

short AddFunctionTrap(char *, char *, char *);
short RemFunctionTrap(char *, char *);
BOOL FindFunctionTrap(char *, char *, char *);
APTR LockTrapList(void);
void UnlockTrapList(void);
APTR FindTrapEntry(APTR, char *, char *);

IPTR rexx_add_appicon(char *, struct RexxMsg *);
void rexx_rem_appthing(char *, short);
void rexx_handle_appmsg(struct AppMessage *msg);
BOOL rexx_send_appmsg(RexxAppThing *, short, struct AppMessage *);

long rexx_lister_newprogress(Lister *lister, char *args, IPTR *);

void rexx_set_lister_mode(Lister *lister, char *args);
void rexx_send_command(char *command, BOOL);

#define HA_String 0x01
#define HA_Value 0x02
#define HA_Qualifier 0x03

#define rexx_handler_msg(handler, buffer, flags, ...)                                \
	({                                                                               \
		IPTR __args[] = {DOPUS_VARIADIC_IPTR(__VA_ARGS__)};                         \
		(short)rexx_handler_msg_args(handler, buffer, flags, __args);               \
	})
short STDARGS rexx_handler_msg_args(char *handler, DirBuffer *buffer, short flags, IPTR *args);

long rexx_set_appicon(char *str, struct RexxMsg *msg);

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct
{
	struct MinNode node;
	struct RexxMsg *rx_msg;
	IPCMessage *ipc_msg;
} RexxMsgTracker;

#ifndef __amigaos3__
	#pragma pack()
#endif

BOOL rexx_send_msg(struct MinList *list, IPCMessage **imsg, struct MsgPort *port);

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct
{
	char handler[40];
	struct RexxMsg *rx_msg;
} RexxDespatch;

#ifndef __amigaos3__
	#pragma pack()
#endif

BOOL rexx_send_rxmsg(IPCMessage *imsg, struct MsgPort *rxport);
BOOL rexx_send_rxmsg_args(RexxDespatch *desp, ULONG flags, struct MsgPort *rxport);

short rexx_lister_reload_file(Lister *, char *);

// dos notify
void handle_dos_notify(DOpusNotify *, FunctionHandle *);

void rexx_add_cmd(char *args);
BOOL rexx_lister_movement(Lister *lister, char *args);
void rexx_add_cmd_type(Att_List *list, char *type);

void rexx_remap_icons(BOOL);

enum {
	SETHF_QUOTES,
	SETHF_FULLPATH,
	SETHF_EDITING,
	SETHF_NOPOPUPS,
	SETHF_GAUGE,
	SETHF_SUBDROP,
	SETHF_INHERIT,
	SETHF_LEAVEGAUGE,
	SETHF_SYNCTRAPS,
};

#define IEQUALIFIER_SUBDROP 0x400

BOOL rexx_dopus_get_pen(char **, UBYTE *);
void rexx_dopus_get_colour(char **, UBYTE *, short);
short rexx_dopus_map_pen(short);
void rexx_send_reset(ULONG, ULONG);

#endif
