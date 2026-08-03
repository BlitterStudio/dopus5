/*

Directory Opus 5
Original APL release version 5.82
Copyright 1993-2012 Jonathan Potter & GP Software
Copyright 2012-2013 DOPUS5 Open Source Team
Copyright 2023-2026 Dimitris Panokostas

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

//	ftplister.h

#ifndef _LISTER_H
#define _LISTER_H

#include <errno.h>
#include "ftp_opusftp.h"
#include "ftp.h"
#include "ftp_listcmd.h"
#include "ftp_protocol.h"
#include "ftp_sftp.h"

// Flags for entry_info_from_XXX function
enum {
	ENTRYFROMF_DEFAULT = 1 << 0,  // Fill in default values if entry not found
	ENTRYFROMF_DEFDIR = 1 << 1,	  // Default values for directory rather than file
};

// Flags for lister_cwd
enum {
	CWD_RELATIVE = 1 << 0,		// CD to relative path rather than absolute
	CWD_ALWAYS_SYNCH = 1 << 1,	// Always synchronize path with PWD command
	CWD_NO_MSG = 1 << 2,		// Don't show message even if option is on
};

// Flags for lister_long_msg
enum {
	LONGMSG_MULTI = 1 << 0,	 // Only show multi-line messages
};

//	The default FTP command to get a directory listing
#define LSCMD FTP_LISTCMD_DEFAULT

//	Lister flags
enum {
	LST_OPENED = 1 << 0,		   // Lister is open
	LST_CONNECTED = 1 << 1,		   // Lister is connected to a server
	LST_LOGGEDIN = 1 << 2,		   // Lister is logged in to a server
	LST_LEAVEOPEN = 1 << 3,		   // Don't close this lister when we disconnect
	LST_ABORT = 1 << 4,			   // Set to stop lister processing entries
	LST_LEAVEOPENIFFAIL = 1 << 5,  // Don't close lister if we can't connect
	LST_CONNECTFAILED = 1 << 6,	   // Tried to connect but failed
	LST_RECONNECTING = 1 << 7,	   // Don't close lister unless we can't connect
	LST_INVISIBLE = 1 << 8,		   // Lister is purposefully invisible
	LST_NOREQ = 1 << 9,			   // Suppress *all* requesters on this lister
	LST_LOCAL = 1 << 10,		   // Lister is local, NOT FTP!
	LST_NOOP_QUIT = 1 << 11,	   // Quit caused by NOOP stuff
};

//	Option types
enum { OPTION_COPY, OPTION_DELETE };

//	Replace actions
enum { RA_SKIP, RA_REPLACE, RA_RESUME };

#ifndef __amigaos3__
	#pragma pack(2)
#endif

//
//	Like a FileInfoBlock for a file or directory entry
//	Has a node field to enable linked lists
//	Name field is big enough for Unix file names
//	Enables more information about links
//

struct entry_info
{
	struct Node ei_node;			// Used by recursive routines
	char ei_name[FILENAMELEN + 1];	// Big enough for 256 char filenames
	ULONG ei_size;
	int ei_type;  // Compatible with what Opus uses
	ULONG ei_seconds;
	LONG ei_prot;					  // Amiga-style protection bits
	char ei_comment[COMMENTLEN + 1];  // Or real path for Unix links
	ULONG ei_unixprot;				  // Unix-style protection bits
	int ei_linkinfo;				  // Extra information about links
};

// ftp lister structure
// ln_Name used to hold the lister handle in ascii - no more!

struct ftp_node
{
	struct Node fn_node;			// For linking
	struct opusftp_globals *fn_og;	// Points back to global info
	IPTR fn_handle;					// Lister handle
	IPCData *fn_ipc;				// IPC of this lister process
	struct ftp_info fn_ftp;			// Socket & site details + FTP reply code
	struct ftp_sftp_session fn_sftp;	// SSH/SFTP session details
	ULONG fn_flags;					// See below
	char fn_opus[PORTNAMELEN + 1];	// DOPUS.1 etc
	/*
	char			fn_path[PATHLEN+1];	// Current directory
	*/
	char fn_pasvport[ADDRESSLEN + 1];
	int fn_systype;				  // Unix, Amiga, unknown etc
	char fn_lscmd[LSCMDLEN + 1];  // The command sent to get the list of files
	int (*fn_ls_to_entryinfo)(struct entry_info *, const char *line, ULONG flags);
	struct Task *fn_signaltask;	 // Task to signal on abort
	IPTR fn_proghandle;
	struct site_entry fn_site;	// As used in Address book etc
	int fn_protocol;			// FTP_PROTOCOL_*
	IPTR fn_read_handle;		// Handle of our Opus text viewer
};

#ifndef __amigaos3__
	#pragma pack()
#endif

// Unknown values for some fields
#define EI_SIZE_UNKNOWN (0xffffffff)
#define EI_SECONDS_UNKNOWN (0xffffffff)

// Either of these mean it looks like a dir but could be a file!
// Link is an AmigaDos softlink
#define EI_LINK_ADOS_SOFTLINK (1)
// Link is a Unix symbolic link
#define EI_LINK_UNIX_SYMLINK (2)

//
//	Error requester flags
//
#define ERRORREQ_RETRYABORT (1 << 0)

/*
 *	Used by the message loop in lister.c
 */
#ifndef __amigaos3__
	#pragma pack(2)
#endif
struct msg_loop_data
{
	IPCData *mld_ipc;
	struct ftp_node *mld_node;
	int *mld_ftpreply;
	IPCMessage *mld_quitmsg;
	char *mld_quit_command;
	int mld_done;
	int mld_reconnecting;
};
#ifndef __amigaos3__
	#pragma pack()
#endif
//
//	Used when updating the display during a file transfer and directory listing
//	NOTE: when self-aborted because of a Write error, ui_abort == 2
//
#ifndef __amigaos3__
	#pragma pack(2)
#endif
struct update_info
{
	struct opusftp_globals *ui_og;
	struct SignalSemaphore ui_sem;
	ULONG ui_flags;	 // See below...
	ULONG ui_raw_lines;
	ULONG ui_entries;
	struct ftp_node *ui_ftpnode;
	IPTR ui_handle;  // Lister
	char *ui_opus;	  // Opus Arexx port name
	BOOL *ui_abort;	  // TRUE when transfer is aborted - This is the only part the FTP code needs
	char *ui_filename;
	unsigned int ui_total_bytes;	// Total size of file
	unsigned int ui_bytes_so_far;	// Bytes transferred so far
	unsigned int ui_resumed_bytes;	// resume point for peed calc
	struct timeval ui_start, ui_last, ui_curr;
	int ui_info_type;  // extended display
	char ui_infotext_path[256];
};
#ifndef __amigaos3__
	#pragma pack()
#endif
// Flags for update_info
enum {
	UI_NO_LINK_FIELD = 1 << 0,	  // LIST - skip links field for MACOS
	UI_DOT_HIDDEN = 1 << 1,		  // LIST - entries starting with '.' are hidden
	UI_STORING = 1 << 2,		  // XFER - storing if set, otherwise retrieving
	UI_REMOTE = 1 << 3,			  // XFER - FTP to FTP if set
	UI_FIRSTDONE = 1 << 4,		  // First callback done already
	UI_LINKS_ARE_FILES = 1 << 5,  // Treat unknown links as files rather than dirs
	UI_SPECIAL_DIR = 1 << 6,	  // Fred Hack - for dirs starting with spaces

};

/*

// Info type for update_info
enum
{
UI_UPDATE_SWAP,			// Swap between bytes and path
UI_UPDATE_BYTES,		// "nnn/nnn n/s"
UI_UPDATE_PATH,			// "From 'xxx' to 'yyy'"
UI_UPDATE_BOTH,			// "nnn/nnn n/s From 'xxx' to 'yyy'"
};

*/

// Flags for Info type for update_info

#define UI_UPDATE_PATH_LINE (1 << 0)
#define UI_UPDATE_BYTE_LINE (1 << 1)
#define UI_UPDATE_TIME_LINE (1 << 2)

// Flags for request_user() and request_pass()
enum {
	REQUSERF_LAST_USER = 1 << 0,  // Show last used username in requester
	REQUSERF_SAME_USER = 1 << 1,  // Show same username as in connect message
};

#ifndef __amigaos3__
	#pragma pack(2)
#endif
struct message_update_info
{
	struct ftp_node *mu_node;
	Att_List *mu_msg;
};
#ifndef __amigaos3__
	#pragma pack()
#endif

// Matches the int (*)(void *, int, char *) callback type used by ftp.c
int message_update(void *update_info, int num, char *text);

/***********************

format for index files is as follows

AmiNet INDEX

***************
| Contents of the directory text/tex : TeX formatting package
|------------------------------------------------
AmiWeb2C-No20.lha  text/tex   594K  90+A new TeX/Metafont/BibTeX/MetaPost syste
AmiWeb2C.lha       text/tex   1.5M  93+A new TeX/Metafont/BibTeX/MetaPost syste
**************

so get 1st field then skip to +

for Files.BBS
************
AmiWeb2C-No20.lha        607932 950516 A new TeX/Metafont/BibTeX/MetaPost system.
AmiWeb2C.lha            1546465 950426 A new TeX/Metafont/BibTeX/MetaPost system.
**********************

get 1st field then skip two sets of number fields and get rest

**********************************/

void STDARGS logprintf(char *fmt, ...);

// Functions that once used ARexx but now use callbacks
void lister_add(struct ftp_node *, char *name, int size, int type, ULONG seconds, LONG prot, char *comment);
void ftplister_refresh(struct ftp_node *, int date);

struct ftp_node *find_ftpnode(struct opusftp_globals *, IPTR handle);

int lister_synch_path(struct ftp_info *, char *result);
BOOL entry_info_from_lister(struct ftp_node *, char *name, struct entry_info *, ULONG flags);
int entry_info_from_remote(struct ftp_node *, char *name, struct entry_info *, ULONG flags);
ULONG get_file_mdtm(struct ftp_node *, char *name);
int get_file_size(struct ftp_node *, char *name);

int lister_list(struct opusftp_globals *, struct ftp_node *, BOOL redo_cache);
BOOL lister_fallback_list_command(struct ftp_node *);
void lister_doubleclick(struct ftp_node *, IPCMessage *);
void lister_xfer(struct ftp_node *, IPCMessage *);
void lister_getput(struct ftp_node *srcnode, IPCMessage *);

struct ftp_node *lister_new_connection(struct opusftp_globals *, struct msg_loop_data *, IPCMessage *);
void lister_disconnect(struct opusftp_globals *, struct msg_loop_data *);
void lister_reconnect(struct opusftp_globals *, struct msg_loop_data *);

unsigned long lister_options(struct ftp_node *, int type);
void lister_remove_node(struct opusftp_globals *, struct ftp_node *);
void lister_traptemp(struct ftp_node *, IPCMessage *);

int lister_cwd(struct ftp_node *, char *path, ULONG flags);

void lst_addabort(struct ftp_node *, ULONG signals, struct Task *task);
void lst_remabort(struct ftp_node *);
int lst_dos_err(struct opusftp_globals *, struct ftp_node *, ULONG flags, int err);
int lst_server_err(struct opusftp_globals *,
				   struct ftp_node *displaynode,
				   struct ftp_node *errnode,
				   ULONG flags,
				   int default_string);
void lst_server_reply(struct opusftp_globals *,
					  struct ftp_node *displaynode,
					  struct ftp_node *errnode,
					  int default_string);
int parent_buffer(struct ftp_info *, char *buf, int get_current);
int pathcmp(ULONG ftpflags, char *path1, char *path2);
int casematchpattern(ULONG ftpflags, char *pat, char *str);
char *sockerr(void);

BOOL lister_xferindex(struct ftp_node *ftpnode, char *localname, char *remotename, int size);

int getput(struct ftp_node *src, struct ftp_node *dest, char *file, struct update_info *ui, BOOL src_flag);

char *getput_pasv(struct ftp_node *destnode);

void init_xfer_time(struct update_info *ui);

void lister_prog_bar(struct ftp_node *, int total, int count);
void lister_prog_bytes(struct ftp_node *, int total, int count);
void lister_prog_clear(struct ftp_node *);
void lister_prog_info(struct ftp_node *, char *info);
void lister_prog_info2(struct ftp_node *, char *info);
void lister_prog_info3(struct ftp_node *, char *info);
void lister_prog_init(struct ftp_node *, char *title, char *info, char *name, int file, int bar);
void lister_prog_init_multi(struct ftp_node *, char *title, BOOL short_display, char *name, int file, int bar);
void lister_prog_name(struct ftp_node *, char *name);
// int             lister_request         ( struct ftp_node *, Tag, ... );
int lister_request(struct ftp_node *, struct TagItem *tags);
#define lister_request_tags(node, ...)                  \
	({                                                  \
		IPTR _tags[] = {DOPUS_VARIADIC_IPTR(__VA_ARGS__)}; \
		lister_request(node, (struct TagItem *)&_tags); \
	})
int handle_has_handler(const char *opus, IPTR handle);
int lister_get_path(struct ftp_node *, char *buffer);
int lister_long_message(struct ftp_node *, Att_List *msg, ULONG flags);

int ftplister_write_listformat(struct ftp_node *, int ok);
void ftplister_read_listformat(struct ftp_node *);

#endif
