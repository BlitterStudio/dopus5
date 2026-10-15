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

#ifndef _DOPUS_FILETYPES
#define _DOPUS_FILETYPES

enum {
	FTTYPE_WANT_NAME = -3,	// Try Datatypes if nothing internal matches
	FTTYPE_ICON,			// Must have icon defined to match
	FTTYPE_ANY,				// Match anything

	FTTYPE_USER_1,			   // UserFunc1
	FTTYPE_USER_2,			   // UserFunc2
	FTTYPE_DOUBLE_CLICK,	   // Double-click
	FTTYPE_DRAG_DROP,		   // Drag 'n' drop
	FTTYPE_CTRL_DOUBLECLICK,   // Control double-click
	FTTYPE_USER_3,			   // UserFunc3
	FTTYPE_ALT_DOUBLECLICK,	   // Alt double-click
	FTTYPE_ALT_DRAGDROP,	   // Alt drag/drop
	FTTYPE_CTRL_DRAGDROP,	   // Control drag/drop
	FTTYPE_USER_4,			   // UserFunc4
	FTTYPE_SHIFT_DOUBLECLICK,  // Shift double-click
	FTTYPE_SHIFT_DRAGDROP,	   // Shift drag/drop
	FTTYPE_USER_5,			   // UserFunc5
	FTTYPE_USER_6,			   // UserFunc6
	FTTYPE_USER_7,			   // UserFunc7
	FTTYPE_USER_8,			   // UserFunc8
	FTTYPE_USER_9,			   // UserFunc9
	FTTYPE_USER_10,			   // UserFunc10
};

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct
{
	char *name;					  // Filename
	BPTR lock;					  // File lock
	APTR file;					  // File handle
	unsigned char buffer[1024];	  // Buffer for matching
	unsigned char wild_buf[260];  // Buffer for wildcards
	struct FileInfoBlock fib;	  // File information
	ULONG iff_form;				  // IFF form
	ULONG last_chunk;			  // Last chunk position
	ULONG chunk_size;			  // Size of last chunk
	struct DataType *datatype;	  // DataType handle
	long flags;
	struct InfoData info;	  // Disk information
	unsigned long disk_type;  // Disk type
} MatchHandle;

#ifndef __amigaos3__
	#pragma pack()
#endif

#define MATCHF_TRIED_DT (1 << 0)	  // Tried to get datatypes
#define MATCHF_TRIED_MODULE (1 << 1)  // Tried to get module
#define MATCHF_IS_MODULE (1 << 2)	  // Is a module
#define MATCHF_EXECUTABLE (1 << 3)	  // File is executable
#define MATCHF_IS_DISK (1 << 4)		  // Disk

// Filetype matching commands
enum {
	FTOP_NOOP,			// No operation
	FTOP_MATCH,			// Match text
	FTOP_MATCHNAME,		// Match filename
	FTOP_MATCHBITS,		// Match protection bits
	FTOP_MATCHCOMMENT,	// Match comment
	FTOP_MATCHSIZE,		// Match size
	FTOP_MATCHDATE,		// Match date
	FTOP_MOVETO,		// Move to absolute location
	FTOP_MOVE,			// Move to relative location
	FTOP_SEARCHFOR,		// Search for text
	FTOP_MATCHFORM,		// Match an IFF FORM
	FTOP_FINDCHUNK,		// Find an IFF chunk
	FTOP_MATCHDTGROUP,	// Match datatypes group
	FTOP_MATCHDTID,		// Match datatypes ID
	FTOP_MATCHNOCASE,	// Match text case insensitive
	FTOP_DIRECTORY,		// Match directory
	FTOP_MODULE,		// Match sound module
	FTOP_DISK,			// Match disk
	FTOP_SEARCHRANGE,	// Search for text (limited range)
	FTOP_MATCHCHUNK,	// Match an IFF chunk

	FTOP_LAST,	// Last valid command

	FTOP_SPECIAL = 252,	 // Start of special instructions

	FTOP_OR,		 // Or
	FTOP_AND,		 // And
	FTOP_ENDSECTION	 // End of a section
};

#define MATCH_LESS -1
#define MATCH_EQUAL 0
#define MATCH_GREATER 1
#define MATCH_INVALID 2

// Filetype Functions

enum {
	FTFUNC_ICON = -2,	   // Match to add icon
	FTFUNC_NOFUNC = -1,	   // No function needed
	FTFUNC_AUTOFUNC1 = 0,  // UserFunc1
	FTFUNC_AUTOFUNC2,	   // UserFunc2
	FTFUNC_DOUBLECLICK,	   // Double-click
	FTFUNC_CLICKMCLICK,	   // Click-m-click
	FTFUNC_ANSIREAD,	   // ANSI read
	FTFUNC_AUTOFUNC3,	   // UserFunc3
	FTFUNC_HEXREAD,		   // Hex read
	FTFUNC_LOOPPLAY,	   // Loop play
	FTFUNC_PLAY,		   // Play
	FTFUNC_AUTOFUNC4,	   // UserFunc4
	FTFUNC_READ,		   // Read
	FTFUNC_SHOW			   // Show
};

#ifndef __amigaos3__
	#pragma pack(2)
#endif

typedef struct
{
	struct MinNode node;
	Lister *lister;
	DirBuffer *buffer;
	char type_name[32];
	Cfg_Filetype *type;
	short flags;
	short ver;
	short rev;
	long days;
	char name[1];
} SniffData;

#ifndef __amigaos3__
	#pragma pack()
#endif

#define SNIFFF_VERSION (1 << 0)
#define SNIFFF_NO_FILETYPES (1 << 1)

#define SNIFFF_FILETYPES (1 << 2)
#define SNIFFF_START (1 << 3)
#define SNIFFF_STOP (1 << 4)

// Prototypes
void filetype_read_list(APTR, struct ListLock *);
void filetype_read_list_async(APTR);
void filetype_default_list(APTR, struct ListLock *);
Cfg_Filetype *filetype_default_new(APTR, Cfg_FiletypeList *, char *, char *, short, char *, char *);
void filetype_default_menu(APTR, Cfg_Filetype *, char *, char *);
MatchHandle *filetype_new_handle(char *);
void filetype_free_handle(MatchHandle *);
Cfg_Filetype *filetype_identify(char *, short, char *, unsigned short);

void parse_prot_string(char *, ULONG *);
void parse_date_string(char *, struct DateStamp *);

BOOL ftype_check_function(char *, int, Lister *, Lister *);
char *ftype_get_description(char *);
void ftype_doubleclick(Lister *, char *, char *, int);
BOOL ftype_icon_drop(struct AppMessage *, int, Lister *);

IPCData *filetype_get_sniffer(void);
void filetype_find_type(Lister *lister, DirBuffer *buffer, char *name, short flags);
void filetype_find_typelist(Lister *lister, short flags);

Cfg_Filetype *filetype_find(char *desc, short type);
BOOL filetype_check(Cfg_Filetype *match_type);

BOOL is_default_filetype(Cfg_Filetype *type);

BOOL filetype_match_type(char *, Cfg_Filetype *);

#endif
