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

#ifndef _DOPUS_FUNCTION_LAUNCH
#define _DOPUS_FUNCTION_LAUNCH

#define COMMAND_LINE_LEN 512

enum {
	FUNCTION_READ_DIRECTORY,		 // Read a directory
	FUNCTION_BUFFERLIST,			 // Generate list of buffers
	FUNCTION_DEVICELIST,			 // Generate list of devices
	FUNCTION_TREE,					 // Generate directory tree
	FUNCTION_RUN_FUNCTION,			 // Run a function
	FUNCTION_FILETYPE,				 // Do filetype action on a file
	FUNCTION_RUN_FUNCTION_EXTERNAL,	 // Run with external args
	FUNCTION_GET_ICONS,				 // Get icons
};

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct _FunctionEntry
{
	struct MinNode node;
	char *name;
	DirEntry *entry;
	short type;
	short flags;
#ifdef USE_64BIT
	UQUAD size;
#else
	long size;
#endif
} FunctionEntry;

typedef struct
{
	struct MinNode node;
	short type;
	short flags;
	DirEntry *entry;
	char *name;
	char path[1];
} ExternalEntry;

typedef struct _InstructionParsed
{
	struct MinNode node;
	struct _CommandList *command;
	short type;
	unsigned char *string;
	ULONG flags;
	char inst_data[680];
	ULONG inst_flags;
	FuncArgs *funcargs;
	short new_arg;
	unsigned short count;
} InstructionParsed;

#ifndef __amigaos3__
	#pragma pack()
#endif

#define FUNCENTF_UNSELECT (1 << 0)	 // Deselect this entry
#define FUNCENTF_REMOVE (1 << 1)	 // Remove this entry
#define FUNCENTF_RECURSE (1 << 2)	 // Entry collected recursively
#define FUNCENTF_FAKE (1 << 3)		 // Fake entry
#define FUNCENTF_ENTERED (1 << 4)	 // Entered this entry
#define FUNCENTF_EXITED (1 << 5)	 // Exited this entry
#define FUNCENTF_ICON (1 << 6)		 // File is an icon
#define FUNCENTF_TOP_LEVEL (1 << 7)	 // Top level entry
#define FUNCENTF_RELOAD (1 << 8)	 // Reload this entry
#define FUNCENTF_ICON_ACTION (1 << 9)
#define FUNCENTF_ICON_ONLY (1 << 10)
#define FUNCENTF_LINK (1 << 11)	 // Link
#define FUNCENTF_FAKE_ICON (1 << 12)
#define FUNCENTF_NO_UNSELECT (1 << 13)

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct _PathNode
{
	struct MinNode node;
	char path_buf[512];
	char *path;
	Lister *lister;
	short dual_side;
	ULONG flags;
} PathNode;

#ifndef __amigaos3__
	#pragma pack()
#endif

#define LISTNF_INVALID (1 << 0)		  // List is invalid
#define LISTNF_NO_TITLE (1 << 1)	  // Don't update title at finish
#define LISTNF_LOCKED (1 << 2)		  // List has been locked
#define LISTNF_SAME (1 << 3)		  // Same as the source
#define LISTNF_CHANGED (1 << 4)		  // Use internal buffer
#define LISTNF_UPDATE_STAMP (1 << 5)  // Update datestamp
#define LISTNF_RESCAN (1 << 6)		  // Rescan this list
#define LISTNF_NO_REFRESH (1 << 7)	  // Don't refresh this lister
#define LISTNF_DUAL_SIDE (1 << 8)	  // Lister path is tied to a dual panel
#define LISTNF_SHARED_LOCK (1 << 9)	  // Lister busy lock is owned by another path node

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct
{
	struct MinList list;
	PathNode *current;
} PathList;

/*
 * Dual lister IPC packets carry a 1-based side value when a command needs to
 * target a specific panel but the legacy IPC argument slots are already used.
 * A side of 0 keeps the old active-lister behaviour.
 */
typedef struct
{
	SelectData data;
	PathList *dest_list;
	ULONG side;
} SelectWildData;

typedef struct
{
	ReselectionData *reselect;
	ULONG flags;
	ULONG side;
} ListerReselectData;

typedef struct
{
	char *path;
	struct DateStamp *stamp;
	char *name;
	ULONG side;
} ListerBufferFindData;

typedef struct
{
	struct Node node;
	struct MinList files;
	Lister *lister;
	short count;
	char path[1];
} FileChangeList;

typedef struct
{
	struct Node node;
	short count;
	ULONG flags;
#if 0
	#ifdef USE_64BIT
	UQUAD			fib_Size;
	#else
	long			fib_Size;
	#endif
	short			fib_DirEntryType;
	struct DateStamp	fib_Date;
	unsigned long		fib_Protection;
	char			*fib_Comment;
	NetworkInfo		*fib_Network;
	char			fib_FileName[1];
#else
	struct FileInfoBlock *fib;
	NetworkInfo *network;
#endif
} FileChange;

#ifndef __amigaos3__
	#pragma pack()
#endif

#define FCTYPE_ADD 1
#define FCTYPE_DEL 2
#define FCTYPE_RELOAD 3
#define FCTYPE_MODIFY 4
#define FCTYPE_LOAD 8
#define FCTYPE_RENAME 16

#define FCF_SHOW 1
#define FCF_SELECT 2

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct _FunctionHandle
{
	IPCData *ipc;  // IPC data
	APTR memory;   // Function memory pool

	ULONG command;			 // Function command
	Cfg_Function *function;	 // Function pointer
	ULONG data;				 // Function data
	ULONG flags;			 // Function flags

	char source_path[512];	// Source path
	char dest_path[512];	// Destination path

	int entry_count;			   // Total number of entries
	struct List entry_list;		   // Entries to work on
	FunctionEntry *current_entry;  // Current entry
	FunctionEntry *use_entry;	   // Entry to use

	int dest_entry_count;				 // Total number of destination entries
	struct List dest_entry_list;			 // Destination entries to use as arguments
	FunctionEntry *dest_current_entry;	 // Current destination entry
	PathNode *dest_entry_path;			 // Path used to build destination entries

	int arg_count;			// Argument count
	struct ArgArray *args;	// Arguments
	ULONG result;			// Result code

	ULONG func_flags;				   // Function flags
	struct MinList func_instructions;  // Function instructions (parsed)
	CFG_FUNC func_parameters;		   // Function parameters

	struct List func_arguments;		// Function arguments (user supplied)
	struct Node *func_current_arg;	// Current function argument

	short func_iterations;	// Iteration count
	short func_source_num;	// Source number we are doing

	struct List recurse_list;		 // List of files collected recursively
	FunctionEntry *current_recurse;	 // Current recursively-collected file

	struct AnchorPath *anchor;	   // Anchor for recursing directories
	char *anchor_path;			   // Full pathname for anchor
	int recurse_return;			   // Result code from last return
	FunctionEntry *recurse_entry;  // Recursively collected entry
	FunctionEntry *recurse_entry_data;

	BPTR recurse_cd;  // Current dir for recursing

	ULONG temp_key;	 // Key for temp files

	ULONG special_flags;  // Special flags

	InstructionParsed *cur_instruction;	 // Currently executing instruction

	char recurse_path[512];	 // Pathname of recursed entry
	int recurse_count;		 // Count of recursed files
#ifdef USE_64BIT
	UQUAD recurse_bytes;
#else
	ULONG recurse_bytes;  // Size of recursed files
#endif
	int recurse_depth;	// Recurse depth

	char work_buffer[1024];	 // A buffer to do things in

	PathList source_paths;	// List of source paths
	PathList dest_paths;	// List of destination paths

	struct FileInfoBlock  // Some FileInfoBlocks to play with
		*s_info,
		*d_info;

	ULONG instruction_flags;  // Flags for this instruction
	char final_message[80];	  // Final message to display
	APTR entry_memory;		  // Entry memory pool

	char *inst_data;   // Instruction-specific data pointer
	ULONG inst_flags;  // Instruction-specific flags

	struct FileInfoBlock recurse_info;	// Info on last recursed entry

	char last_filename[512];  // Last filename used
	char dest_last_filename[512];  // Last destination filename used

	APTR script_file;		// External script file
	char script_name[80];	// Script file name
	char temp_buffer[256];	// Temporary buffer
	char temp_name[80];		// Temporary file name

	struct MinList external_list;  // List of external entries

	Lister *source_lister;	// Source lister
	Lister *dest_lister;	// Dest lister

	unsigned short source_block_size;	  // Source block size
	unsigned short dest_block_size;		  // Destination block size
	unsigned short dest_data_block_size;  // Destination data block size
	unsigned long dest_recurse_blocks;	  // Number of blocks for destination

	FuncArgs *funcargs;	 // Parsed function arguments
	struct List entry_list_backup;
	FunctionEntry *current_entry_backup;
	FunctionEntry *use_entry_backup;
	int entry_count_backup;

	Lister *progress_lister;  // Lister with progress req

	struct Message *done_msg;  // Message to send when done

	char file_filter[31];  // File filter
	char got_filter;
	char file_filter_parsed[63];

	char replace_option;

	unsigned short file_count;	// File and dir counts
	unsigned short dir_count;
	unsigned short file_count_backup;
	unsigned short dir_count_backup;

	short instruction_count;  // Number of instructions

	char output_handle[120];  // Output handle

	ULONG result_flags;	 // Flags for result

	struct MsgPort *reply_port;

	Buttons *buttons;  // Button bank launched from

	struct FileRequester *filereq;	// File requester
	struct IBox coords;				// Coordinates

	struct _DOpusAppMessage *app_msg;  // AppMessage from drag&drop

	struct List filechange;	 // File changes

	ULONG flags2;  // More flags

	struct ArgArray *arg_passthru;	// Arguments passed through

	unsigned short link_dir_count;
	unsigned short link_file_count;

	Lister *saved_source_lister;  // Source lister

	short ret_code;

	ULONG func_additional_flags;
} FunctionHandle;

typedef struct _FunctionReturn
{
	int rc;
	char *result;
} FunctionReturn;

#ifndef __amigaos3__
	#pragma pack()
#endif

// Function flags
#define FUNCF_NEED_SOURCE (1 << 0)	// Needs a source directory
#define FUNCF_NEED_DEST (1 << 1)	// Needs a destination directory
#define FUNCF_NEED_FILES (1 << 2)	// Needs some files to work with
#define FUNCF_NEED_DIRS (1 << 3)	// Needs some files to work with
#define FUNCF_NEED_ENTRIES (FUNCF_NEED_FILES | FUNCF_NEED_DIRS)
#define FUNCF_SCRIPT_OVERRIDE (1 << 4)	// Can be overridden by a script
#define FUNCF_NEED_DEST_ENTRIES (1 << 5)  // Needs selected destination entries
#define FUNCF_CAN_DO_ICONS (1 << 6)		// Function can do icons
#define FUNCF_LAST_DEST_FILE_FLAG (1 << 7)  // Still using last destination file
#define FUNCF_SINGLE_SOURCE (1 << 8)	// Only a single source needed
#define FUNCF_SINGLE_DEST (1 << 9)		// Only a single destination needed
#define FUNCF_WANT_DEST_ENTRIES (1 << 10)  // Want destination entries

#define FUNCF_WANT_DEST (1 << 11)		   // Want destinations, don't need them
#define FUNCF_WANT_SOURCE (1 << 12)		   // Want source, don't need it
#define FUNCF_CREATE_SOURCE (1 << 13)	   // Can create our own source
#define FUNCF_EXTERNAL_FUNCTION (1 << 14)  // Function is external
#define FUNCF_ARG_ENTRIES (1 << 15)		   // Entries in arguments
#define FUNCF_ASK_FILTER (1 << 16)		   // Can ask for a file filter
#define FUNCF_ASK_PATH (1 << 17)		   // Allowed to ask for paths
#define FUNCF_SPECIAL_OK (1 << 18)		   // Ok if special buffer
#define FUNCF_WANT_ENTRIES (1 << 19)	   // Want entries
#define FUNCF_NO_SOURCE (1 << 20)		   // Can't use lister sources
#define FUNCF_RESCAN_DESKTOP (1 << 21)	   // Rescan the desktop

#define FUNCF_ASK_DEST (1 << 22)   // Must ask for destination
#define FUNCF_ICONS (1 << 23)	   // Function running on icons
#define FUNCF_SYNC (1 << 24)	   // Function wants to be synchronous
#define FUNCF_DRAG_DROP (1 << 25)  // Called by drag'n'drop
#define FUNCF_PRIVATE (1 << 26)	   // Function is private

#define FUNCF_MSG_QUIT (1 << 27)  // Send message when done

#define FUNCF_RUN_NO_ICONS (1 << 28)  // Don't run using icons (double-click kludge)
#define FUNCF_COPY_NO_MOVE (1 << 28)  // Don't move even if on same volume (drag'n'drop kludge)

#define FUNCF_LAST_FILE_FLAG (1 << 29)	// Still using last file
#define FUNCF_GOT_SOURCE (1 << 30)		// Got a source
#define FUNCF_GOT_DEST (1 << 31)		// Got a destination

#define FUNCF_ACTIVATE_ME (1 << 0)	// Want to be activated

// Result flags
#define FRESULTF_RESORT (1 << 0)	   // Need to resort
#define FRESULTF_MADE_LISTER (1 << 1)  // We made a lister

// Instruction flags
#define INSTF_RECURSE_DIRS (1 << 0)		 // Enter directories recursively
#define INSTF_WANT_DIRS (1 << 1)		 // Want directories as well as the contents
#define INSTF_WANT_DIRS_END (1 << 2)	 // Want directories at the end
#define INSTF_DELETE_UNPROTECT (1 << 3)	 // Unprotect all
#define INSTF_DIR_NO_SIZES (1 << 4)		 // Don't count directory sizes
#define INSTF_DIR_CLEAR_SIZES (1 << 5)	 // Clear directory sizes
#define INSTF_REPEAT (1 << 6)			 // Repeating this instruction
#define INSTF_ABORT_DIR (1 << 7)		 // Abort this directory

// Other flags
#define INSTF_UNPROTECT_ALL (1 << 0)  // Unprotect all
#define INSTF_RECURSE (1 << 1)		  // Recurse directories
#define INSTF_NO_RECURSE (1 << 2)	  // Don't recurse directories
#define INSTF_NO_ASK (1 << 3)		  // Don't need to ask

// Copy
#define COPYF_DELETE_ALL (1 << 0)
#define COPYF_UNPROTECT_ALL (1 << 1)
#define COPYF_SKIP_ALL (1 << 2)
#define COPYF_RENAME (1 << 3)
#define COPYF_ICON_COPY (1 << 4)

#define PARSE_MORE_FILES 1
#define PARSE_END 0
#define PARSE_OK 0
#define PARSE_ABORT -1
#define PARSE_INVALID -2

// protos
BOOL function_launch_quick(ULONG, Cfg_Function *, Lister *);
BOOL function_launch(ULONG,
					 Cfg_Function *,
					 ULONG,
					 ULONG,
					 Lister *,
					 Lister *,
					 char *,
					 char *,
					 struct ArgArray *,
					 struct Message *,
					 Buttons *);
FunctionHandle *function_new_handle(struct MsgPort *, BOOL);
void function_handle_init(FunctionHandle *, BOOL);
void function_free(FunctionHandle *);
BOOL function_check_abort(FunctionHandle *);
Lister *function_valid_lister(FunctionHandle *handle, Lister *lister);
BOOL function_lock_paths(FunctionHandle *handle, PathList *, int);
void function_unlock_paths(FunctionHandle *handle, PathList *, int);

int function_build_list(FunctionHandle *function, PathNode **, InstructionParsed *);
void function_drop_brace_name_arg(InstructionParsed *);
short function_brace_name_arg(InstructionParsed *);
int function_build_dest_list(FunctionHandle *function, PathNode **, InstructionParsed *);
FunctionEntry *function_new_entry(FunctionHandle *, char *, BOOL);
FunctionEntry *function_current_entry(FunctionHandle *handle);
FunctionEntry *function_next_entry(FunctionHandle *handle);
FunctionEntry *function_current_dest_entry(FunctionHandle *handle);
FunctionEntry *function_get_dest_entry(FunctionHandle *handle);
int function_end_dest_entry(FunctionHandle *, FunctionEntry *);
void function_files_from_args(FunctionHandle *handle);
char *function_build_file_string(FunctionHandle *handle, short);

void function_progress_on(FunctionHandle *handle, char *operation, ULONG total, ULONG flags);
BOOL function_progress_update(FunctionHandle *handle, FunctionEntry *entry, ULONG count);
Lister *function_get_paths(FunctionHandle *, PathList *, ULONG, short);
BOOL function_valid_path(PathNode *path);
PathNode *function_add_path(FunctionHandle *, PathList *, Lister *, char *);
void function_init_path_node(PathNode *);
void function_capture_path_side(PathNode *);
void function_apply_path_side(PathNode *);

void function_read_directory(FunctionHandle *handle, Lister *lister, char *);
void buffer_list_buffers(Lister *dest_lister);
void build_device_list(Lister *lister);

void function_run_function(FunctionHandle *);
int function_parse_function(FunctionHandle *);
void function_parse_arguments(FunctionHandle *, InstructionParsed *);
void function_parse_instruction(FunctionHandle *, char *, unsigned char *, ULONG *);
short function_run(FunctionHandle *);
int function_build_instruction(FunctionHandle *, InstructionParsed *, unsigned char *, char *);
int function_add_filename(char *, char *, char *, int, short);
void function_build_default(FunctionHandle *handle, char *def_string, char *buffer);
BOOL function_check_paths(FunctionHandle *handle);
short function_check_single(FunctionHandle *, long, long, short);

int function_internal_command(struct _CommandList *, char *, FunctionHandle *, InstructionParsed *);
struct _CommandList *function_find_internal(char **, short);
ULONG parse_internal_function(char *str, char *template, ULONG orig_flags);
char *parse_find_path(char *str);
REF_CALLBACK_PROTO(IPTR,
				   ASM SAVEDS function_external_hook,
				   d0,
				   ULONG,
				   command,
				   a0,
				   struct _FunctionHandle *,
				   handle,
				   a1,
				   APTR,
				   packet);

int function_external_command(FunctionHandle *, InstructionParsed *);
int function_open_script(FunctionHandle *);
int function_write_script(FunctionHandle *, char *, short);
void function_close_script(FunctionHandle *, int);

short function_check_same_path(char *, char *);
int check_file_destination(FunctionHandle *handle, FunctionEntry *entry, char *destination, short *confirm_each);
int function_error(FunctionHandle *handle, char *name, int action_msg, int error_code);
BOOL function_check_dirs(FunctionHandle *);
FunctionEntry *function_get_entry(FunctionHandle *);
int function_end_entry(FunctionHandle *, FunctionEntry *, int);

void function_abort(FunctionHandle *handle);
void function_error_text(FunctionHandle *handle, int);
void function_text(FunctionHandle *handle, char *text);
#define function_request(handle, message, ...)                         \
	({                                                                 \
		IPTR __args[] = {DOPUS_VARIADIC_IPTR(__VA_ARGS__)};            \
		(int)function_request_args(handle, message, __args);           \
	})
int function_request_args(FunctionHandle *, char *, IPTR *);
void function_build_source(FunctionHandle *handle, FunctionEntry *entry, char *buffer);
void function_build_dest(FunctionHandle *handle, FunctionEntry *entry, char *buffer);
PathNode *function_path_current(PathList *);
Lister *function_lister_current(PathList *);
PathNode *function_path_next(PathList *);
void function_path_end(FunctionHandle *, PathList *, int);
void function_cleanup(FunctionHandle *, PathNode *, BOOL);
void function_do_lister_changes(FunctionHandle *, PathList *);
void function_perform_changes(FunctionHandle *, PathNode *);

void function_filetype(FunctionHandle *handle);

void function_check_ins_path(FunctionHandle *handle,
							 InstructionParsed *instruction,
							 PathList *path_list,
							 char path_type,
							 short locker);

void function_replace_paths(FunctionHandle *handle, PathList *path_list, char *new_path, short locker);

void function_progress_file(FunctionHandle *handle, long size, long count);

void function_progress_file64(FunctionHandle *handle, QUAD *size, QUAD *count);

void function_progress_info(FunctionHandle *handle, char *info);

short func_requester(FunctionHandle *handle,
					 unsigned char *instruction,
					 char *buf,
					 short max_len,
					 short func_len,
					 short *position);

FileChangeList *function_find_filechanges(FunctionHandle *, FileChangeList *, char *, Lister *, short *);
FileChangeList *function_add_filechanges(FunctionHandle *, char *, Lister *);
FileChange *function_filechange_addfile(FunctionHandle *, char *, struct FileInfoBlock *, NetworkInfo *, Lister *);
FileChange *function_filechange_loadfile(FunctionHandle *, char *, char *, short);
void function_filechange_reloadfile(FunctionHandle *, char *, char *, short);
FileChange *function_filechange_delfile(FunctionHandle *, char *, char *, Lister *, BOOL);
void function_filechange_do(FunctionHandle *, BOOL);

FileChange *function_filechange_modify(FunctionHandle *, char *, char *, ...);
FileChange *function_filechange_rename(FunctionHandle *, char *, char *);

#define FM_Size (TAG_USER + 0x1)
#define FM_Date (TAG_USER + 0x2)
#define FM_Protect (TAG_USER + 0x3)
#define FM_Comment (TAG_USER + 0x4)
#define FM_Name (TAG_USER + 0x5)
#define FM_Size64 (TAG_USER + 0x6)

#define FMF_SIZE (1 << 0)
#define FMF_DATE (1 << 1)
#define FMF_PROTECT (1 << 2)
#define FMF_COMMENT (1 << 3)
#define FMF_NAME (1 << 4)

void function_build_info(FunctionHandle *handle, char *, char *, short);

#define FFLF_ICON (1 << 0)
#define FFLF_RELOAD (1 << 1)
#define FFLF_DEFERRED (1 << 2)

void function_iconinfo_update(FunctionHandle *handle, Att_List *list);

void get_trunc_filename(char *source, char *dest);
void get_trunc_path(char *source, char *dest);

#endif
