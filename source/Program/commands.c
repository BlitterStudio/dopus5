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

#include <proto/configopus.h>
#include <proto/module.h>

extern const char *const dopus_modules[];
const char *const module_exclusions[] = {"configopus.module",
										 "read.module",
										 "show.module",
										 "play.module",
										 "listerformat.module",
										 "icon.module",
										 "register.module",
										 "print.module",
										 "diskcopy.module",
										 "format.module",
										 "about.module",
										 "update.module",
										 0};

// New default function
Cfg_Function *new_default_function(char *func, APTR memory)
{
	Cfg_Function *function;

	// Create function
	if ((function = NewFunction(memory, 0)))
	{
		Cfg_Instruction *ins;

		// Create instruction
		if ((ins = NewInstruction(memory, INST_COMMAND, func)))
			AddTail((struct List *)&function->instructions, (struct Node *)ins);
	}

	return function;
}

// Module command cache. Identifying a module means opening it, and some modules
// open big libraries when they are opened (ftp.module codesets.library,
// xadopus.module xadmaster.library and its clients), so the first start after a
// reboot spent seconds loading modules only to read their command lists. The
// commands of each identified module are saved here and reused for as long as
// the module file keeps its size and datestamp and its catalog resolves to the
// same language and version. Each record holds the commands the module itself
// reported, including any that another module's command of the same name hides.
#define MODULE_CACHE_FILE "DOpus5:System/module-cache"
#define MODULE_CACHE_ID MAKE_ID('M', 'C', 'A', '3')
#define MODULE_CACHE_MAX 128

typedef struct
{
	char *name;
	ULONG size;
	struct DateStamp date;
	char *catalog;		// Catalog the module opens
	char *catalog_id;	// What it resolved to when the commands were read
	UBYTE *block;		// Command count and records
	ULONG block_size;
	BOOL seen;			// Module file found by this scan
} ModuleCacheEntry;

typedef struct
{
	UBYTE *buffer;
	UBYTE *end;
	short count;
	ModuleCacheEntry entry[MODULE_CACHE_MAX];
} ModuleCache;

typedef struct
{
	UBYTE *ptr;
	UBYTE *end;
	BOOL bad;
} ModuleCacheReader;

// Values are stored big-endian byte by byte, so records need no alignment
static ULONG module_cache_get(ModuleCacheReader *reader, short bytes)
{
	ULONG value = 0;

	if (reader->bad || reader->end - reader->ptr < bytes)
	{
		reader->bad = TRUE;
		return 0;
	}
	while (bytes--)
		value = (value << 8) | *reader->ptr++;
	return value;
}

// Strings are stored with their length including the null, 0 for no string
static char *module_cache_get_string(ModuleCacheReader *reader)
{
	ULONG len;
	char *string;

	if (!(len = module_cache_get(reader, 2)))
		return 0;
	if (reader->bad || reader->end - reader->ptr < len || reader->ptr[len - 1])
	{
		reader->bad = TRUE;
		return 0;
	}
	string = (char *)reader->ptr;
	reader->ptr += len;
	return string;
}

// Command descriptions come from the module's catalog, which modules open with
// OpenCatalogA(NULL, name, NULL). Open it the same way and note which catalog the
// locale preferences resolved to: none ("-", built-in strings) or a language and
// version. An empty result means the module has no catalog.
static void module_catalog_id(char *catalog, char *id, short size)
{
	struct Catalog *cat = 0;

	id[0] = 0;
	if (!catalog || !*catalog)
		return;

#define LocaleBase locale.li_LocaleBase
	if (LocaleBase)
		cat = OpenCatalogA(NULL, catalog, NULL);
	if (cat)
	{
		snprintf(id,
				 size,
				 "%.24s %ld.%ld",
				 (cat->cat_Language) ? (char *)cat->cat_Language : "",
				 (long)cat->cat_Version,
				 (long)cat->cat_Revision);
		CloseCatalog(cat);
	}
	else
		strcpy(id, "-");
#undef LocaleBase
}

// Read one command record
static BOOL module_cache_get_command(ModuleCacheReader *reader,
									 char **name,
									 char **desc,
									 char **template,
									 char **help,
									 ULONG *flags,
									 UWORD *id)
{
	*name = module_cache_get_string(reader);
	*desc = module_cache_get_string(reader);
	*template = module_cache_get_string(reader);
	*help = module_cache_get_string(reader);
	*flags = module_cache_get(reader, 4);
	*id = (UWORD)module_cache_get(reader, 2);
	if (!*name)
		reader->bad = TRUE;
	return (BOOL)!reader->bad;
}

static void module_cache_free(ModuleCache *cache)
{
	if (cache)
	{
		FreeVec(cache->buffer);
		FreeVec(cache);
	}
}

// Load and check the cache; returns 0 if there is none or it does not apply
static ModuleCache *module_cache_load(void)
{
	ModuleCache *cache;
	ModuleCacheReader reader;
	APTR file;
	D_S(struct FileInfoBlock, fib)
	LONG size = 0;

	if (!(cache = AllocVec(sizeof(ModuleCache), MEMF_CLEAR)))
		return 0;

	if (!(file = OpenBuf(MODULE_CACHE_FILE, MODE_OLDFILE, 512)))
	{
		FreeVec(cache);
		return 0;
	}
	if (ExamineBuf(file, fib) && (size = fib->fib_Size) > 0 && size < 0x10000 &&
		(cache->buffer = AllocVec(size, 0)) && ReadBuf(file, (char *)cache->buffer, size) != size)
	{
		FreeVec(cache->buffer);
		cache->buffer = 0;
	}
	CloseBuf(file);
	if (!cache->buffer)
	{
		FreeVec(cache);
		return 0;
	}

	reader.ptr = cache->buffer;
	reader.end = cache->end = cache->buffer + size;
	reader.bad = FALSE;

	// Written by this version?
	if (module_cache_get(&reader, 4) != MODULE_CACHE_ID || module_cache_get(&reader, 2) != PROG_REVISION ||
		module_cache_get(&reader, 2) != LIB_VERSION || module_cache_get(&reader, 2) != LIB_REVISION)
	{
		module_cache_free(cache);
		return 0;
	}

	// Index the module records, checking them as we go
	while (reader.ptr < reader.end && !reader.bad && cache->count < MODULE_CACHE_MAX)
	{
		ModuleCacheEntry *entry = &cache->entry[cache->count++];
		ULONG num, count;
		char *name, *desc, *template, *help;
		ULONG flags;
		UWORD id;

		if (!(entry->name = module_cache_get_string(&reader)))
			reader.bad = TRUE;
		entry->size = module_cache_get(&reader, 4);
		entry->date.ds_Days = module_cache_get(&reader, 4);
		entry->date.ds_Minute = module_cache_get(&reader, 4);
		entry->date.ds_Tick = module_cache_get(&reader, 4);
		entry->catalog = module_cache_get_string(&reader);
		entry->catalog_id = module_cache_get_string(&reader);
		entry->block = reader.ptr;
		count = module_cache_get(&reader, 2);
		for (num = 0; num < count && !reader.bad; num++)
			module_cache_get_command(&reader, &name, &desc, &template, &help, &flags, &id);
		entry->block_size = reader.ptr - entry->block;
	}

	if (reader.bad || reader.ptr != reader.end)
	{
		module_cache_free(cache);
		return 0;
	}

	return cache;
}

// Builds cache data in memory; with no buffer it only counts the bytes
typedef struct
{
	UBYTE *buffer;
	ULONG pos;
} ModuleCacheWriter;

static void module_cache_put_bytes(ModuleCacheWriter *writer, APTR data, ULONG len)
{
	if (writer->buffer && len)
		CopyMem(data, writer->buffer + writer->pos, len);
	writer->pos += len;
}

static void module_cache_put(ModuleCacheWriter *writer, ULONG value, short bytes)
{
	UBYTE buf[4];
	short a;

	for (a = bytes - 1; a >= 0; a--, value >>= 8)
		buf[a] = (UBYTE)value;
	module_cache_put_bytes(writer, buf, bytes);
}

static void module_cache_put_string(ModuleCacheWriter *writer, char *string)
{
	ULONG len = (string) ? strlen(string) + 1 : 0;

	module_cache_put(writer, len, 2);
	module_cache_put_bytes(writer, string, len);
}

static void module_cache_put_command(ModuleCacheWriter *writer,
									 char *name,
									 char *desc,
									 char *template,
									 char *help,
									 ULONG flags,
									 UWORD id)
{
	module_cache_put_string(writer, name);
	module_cache_put_string(writer, desc);
	module_cache_put_string(writer, template);
	module_cache_put_string(writer, help);
	module_cache_put(writer, flags, 4);
	module_cache_put(writer, id, 2);
}

// Add a module command, splitting the template from its key
static void module_add_command(char *name, char *desc, char *template, char *module, ULONG flags, UWORD id, char *help)
{
	CommandList *command;
	char *ptr;

	if ((command = add_command(name, desc, template, module, flags, 0, 0, help, 0)))
	{
		// Set function ID
		command->function = id;

		// Newline marks template key
		if (command->template && (ptr = strchr(command->template, '\n')))
		{
			// Set template key pointer, and clear join
			command->template_key = ptr + 1;
			*ptr = 0;
		}
	}
}

// Find a module's cache record and note that the module still exists
static ModuleCacheEntry *module_cache_find(ModuleCache *cache, char *module)
{
	short a;

	if (cache)
	{
		for (a = 0; a < cache->count; a++)
		{
			if (stricmp(cache->entry[a].name, module) == 0)
			{
				cache->entry[a].seen = TRUE;
				return &cache->entry[a];
			}
		}
	}

	return 0;
}

// Records for modules that have gone should be dropped from the file
static BOOL module_cache_stale(ModuleCache *cache)
{
	short a;

	if (cache)
	{
		for (a = 0; a < cache->count; a++)
		{
			if (!cache->entry[a].seen)
				return TRUE;
		}
	}

	return FALSE;
}

// Keep what a module's commands and catalog were for the cache file; FALSE if they do
// not fit, in which case the module is left out of the cache. The block is copied.
static BOOL module_node_set_cache(ModuleNode *node, char *catalog, char *catalog_id, UBYTE *block, ULONG size)
{
	UBYTE *copy;

	if ((catalog && strlen(catalog) >= sizeof(node->catalog)) ||
		(catalog_id && strlen(catalog_id) >= sizeof(node->catalog_id)) ||
		!(copy = AllocMemH(global_memory_pool, size)))
		return FALSE;
	CopyMem(block, copy, size);

	lock_listlock(&GUI->modules_list, TRUE);
	FreeMemH(node->commands);
	node->commands = copy;
	node->commands_size = size;
	strcpy(node->catalog, (catalog) ? catalog : "");
	strcpy(node->catalog_id, (catalog_id) ? catalog_id : "");
	node->flags |= MNF_CACHE;
	unlock_listlock(&GUI->modules_list);
	return TRUE;
}

// Forget a module's cached commands (modules list must be locked)
static void module_node_clear_cache(ModuleNode *node)
{
	FreeMemH(node->commands);
	node->commands = 0;
	node->commands_size = 0;
	node->flags &= ~MNF_CACHE;
}

// Add a module's commands from the cache; FALSE if the cache does not have this version of it
static BOOL module_cache_add_commands(ModuleCache *cache, ModuleNode *node, char *module, struct FileInfoBlock *fib)
{
	ModuleCacheEntry *entry;
	ModuleCacheReader reader;
	char catalog_id[40];
	ULONG num, count;

	if (!(entry = module_cache_find(cache, module)) || entry->size != (ULONG)fib->fib_Size ||
		CompareDates(&entry->date, &fib->fib_Date) != 0)
		return FALSE;

	// Descriptions were read through a catalog that may resolve differently now
	module_catalog_id(entry->catalog, catalog_id, sizeof(catalog_id));
	if (strcmp(catalog_id, (entry->catalog_id) ? entry->catalog_id : "") != 0)
		return FALSE;

	reader.ptr = entry->block;
	reader.end = cache->end;
	reader.bad = FALSE;
	count = module_cache_get(&reader, 2);
	for (num = 0; num < count; num++)
	{
		char *name, *desc, *template, *help;
		ULONG flags;
		UWORD id;

		if (module_cache_get_command(&reader, &name, &desc, &template, &help, &flags, &id))
			module_add_command(name, desc, template, module, flags, id, help);
	}

	if (node)
		module_node_set_cache(node, entry->catalog, entry->catalog_id, entry->block, entry->block_size);
	return TRUE;
}

// Save the commands of every module whose commands are known
static void module_cache_save(void)
{
	ModuleCacheWriter writer = {0, 0};
	ModuleNode *node;
	BPTR file;
	short pass, count;

	lock_listlock(&GUI->modules_list, FALSE);

	// Size the file, then fill it in
	for (pass = 0; pass < 2; pass++)
	{
		if (pass == 1 && !(writer.buffer = AllocVec(writer.pos, 0)))
			break;
		writer.pos = 0;

		module_cache_put(&writer, MODULE_CACHE_ID, 4);
		module_cache_put(&writer, PROG_REVISION, 2);
		module_cache_put(&writer, LIB_VERSION, 2);
		module_cache_put(&writer, LIB_REVISION, 2);

		for (node = (ModuleNode *)GUI->modules_list.list.lh_Head, count = 0;
			 node->node.ln_Succ && count < MODULE_CACHE_MAX;
			 node = (ModuleNode *)node->node.ln_Succ)
		{
			if (!(node->flags & MNF_CACHE) || !node->commands)
				continue;

			module_cache_put_string(&writer, node->node.ln_Name);
			module_cache_put(&writer, node->size, 4);
			module_cache_put(&writer, node->date.ds_Days, 4);
			module_cache_put(&writer, node->date.ds_Minute, 4);
			module_cache_put(&writer, node->date.ds_Tick, 4);
			module_cache_put_string(&writer, (node->catalog[0]) ? node->catalog : 0);
			module_cache_put_string(&writer, (node->catalog_id[0]) ? node->catalog_id : 0);
			module_cache_put_bytes(&writer, node->commands, node->commands_size);
			++count;
		}
	}

	unlock_listlock(&GUI->modules_list);

	if (writer.buffer)
	{
		if ((file = Open(MODULE_CACHE_FILE, MODE_NEWFILE)))
		{
			Write(file, writer.buffer, writer.pos);
			Close(file);
		}
		FreeVec(writer.buffer);
	}
}

// Update disk-based commands
void update_commands(ULONG flag)
{
	// Just rexx commands?
	if (flag == SNIFF_REXX)
		init_commands_scan(SCAN_REXX);

	// Rexx and modules?
	else if (flag == SNIFF_BOTH)
		init_commands_scan((GUI->flags & GUIF_REXX) ? SCAN_BOTH : SCAN_MODULES);

	// User
	else if (flag == SNIFF_USER)
		init_commands_scan(SCAN_USER);
}

// Initialises the module and command discovery scan.
// See documents/dopus5/ModuleDiscovery.md for details on the discovery workflow.
void init_commands_scan(short type)
{
	struct AnchorPath *anchor;
	ModuleNode *node = 0;
	ModuleCache *cache = 0;
	BOOL cache_changed = FALSE;
	long error;
	short a, pat;
	char *ptr;
	char *patterns[3] = {0};

	// Allocate anchor path
	if (!(anchor = AllocVec(sizeof(struct AnchorPath) + 256, MEMF_CLEAR)))
		return;

	// Initialise anchor path
	anchor->ap_Strlen = 256;
	anchor->ap_Flags = APF_DOWILD;

	// Get pointer to match on
	if (type == SCAN_BOTH)
	{
		patterns[0] = "PROGDIR:Modules/#?.(module|dopus5)";
		patterns[1] = "DOpus5:Modules/#?.(module|dopus5)";
	}
	else if (type == SCAN_MODULES)
	{
		patterns[0] = "PROGDIR:Modules/#?.module";
		patterns[1] = "DOpus5:Modules/#?.module";
	}
	else if (type == SCAN_USER)
		patterns[0] = "DOpus5:Commands/~(#?.info)";
	else
	{
		patterns[0] = "PROGDIR:Modules/#?.dopus5";
		patterns[1] = "DOpus5:Modules/#?.dopus5";
	}

	// Commands of unchanged modules can come from the cache
	if (type & SCAN_MODULES)
		cache = module_cache_load();

	for (pat = 0; patterns[pat]; pat++)
	{
		ptr = patterns[pat];

		// Search for modules
		error = MatchFirst(ptr, anchor);

		// Continue while there's files
		while (!error)
		{
			BOOL ok = 1, real_module = 0;
			char *name_ptr = 0;

			// Skip the basedata.lha placeholder used to keep DOpus5:Commands
			// archived (lha drops empty directories on extraction). We don't
			// want it registered as a phantom user command.
			if (type == SCAN_USER && stricmp(anchor->ap_Info.fib_FileName, ".dummy") == 0)
			{
				error = MatchNext(anchor);
				continue;
			}

			// A user command?
			if (type == SCAN_USER)
			{
				// Use full name
				name_ptr = anchor->ap_Buf;
			}

			// A real module?
			else if (sufcmp(anchor->ap_Info.fib_FileName, ".module"))
			{
				// Set flag
				real_module = 1;

				// See if module is in the exclusion list
				for (a = 0; module_exclusions[a]; a++)
				{
					if (stricmp(anchor->ap_Info.fib_FileName, module_exclusions[a]) == 0)
					{
						ok = 0;
						break;
					}
				}

				// Keep its cache record
				if (ok)
					module_cache_find(cache, anchor->ap_Info.fib_FileName);
			}

			// Not an excluded module?
			if (ok)
			{
				// Get filename if name not already set
				if (!name_ptr)
					name_ptr = anchor->ap_Info.fib_FileName;

				// Lock modules list
				lock_listlock(&GUI->modules_list, TRUE);

				// Module already in list?
				if ((node = (ModuleNode *)FindNameI(&GUI->modules_list.list, name_ptr)))
				{
					// Check datestamp
					if (CompareDates(&node->date, &anchor->ap_Info.fib_Date) != 0)
					{
						// Update datestamp and size, ok to reread
						node->date = anchor->ap_Info.fib_Date;
						node->size = anchor->ap_Info.fib_Size;
						if (node->flags & MNF_CACHE)
							cache_changed = TRUE;
						module_node_clear_cache(node);

						// Expunge functions for this module
						command_expunge(name_ptr);
					}

					// Don't reread
					else
						ok = 0;

					// Set 'temp' flag
					node->flags |= MNF_TEMP;
				}

				// Allocate new module
				else if ((node = AllocMemH(global_memory_pool, sizeof(ModuleNode) + strlen(name_ptr) + 1)))
				{
					// Fill out node, add to list
					node->node.ln_Name = (char *)(node + 1);
					strcpy(node->node.ln_Name, name_ptr);
					node->date = anchor->ap_Info.fib_Date;
					node->size = anchor->ap_Info.fib_Size;
					AddTail(&GUI->modules_list.list, &node->node);

					// Set 'temp' flag
					node->flags |= MNF_TEMP;
				}

				// Unlock modules list
				unlock_listlock(&GUI->modules_list);
			}

			// Not ok to read?
			if (!ok)
			{
			}

			// User command?
			else if (type == SCAN_USER)
			{
				// Add the command
				add_command(anchor->ap_Info.fib_FileName, anchor->ap_Info.fib_Comment, 0, anchor->ap_Buf, 0, 0, 0, 0, 0);
			}

			// ARexx module?
			else if (!real_module)
			{
				// Function to run
				lsprintf(anchor->ap_Buf, "DOpus5:Modules/%s %s init", anchor->ap_Info.fib_FileName, GUI->rexx_port_name);

				// Run rexx thing
				rexx_send_command(anchor->ap_Buf, FALSE);
			}

			// Commands already known from the cache?
			else if (module_cache_add_commands(cache, node, anchor->ap_Info.fib_FileName, &anchor->ap_Info))
			{
				// No need to open it
			}

			// Otherwise
			else
			{
				struct Library *ModuleBase = NULL;
#ifdef __amigaos4__
				struct ModuleIFace *IModule = NULL;
#endif
				short ver = 0;
				// See if this is one of our modules
				for (a = 0; dopus_modules[a]; a++)
					if (stricmp(anchor->ap_Info.fib_FileName, dopus_modules[a]) == 0)
					{
						// Need newest version
						ver = LIB_VERSION;
						break;
					}

					// Try to open library
#ifdef __amigaos4__
				if (OpenLibIFace(anchor->ap_Buf, (APTR)&ModuleBase, (APTR)&IModule, ver))
#else
	#ifdef __AROS__
				if (!(ModuleBase = OpenLibrary(anchor->ap_Buf, ver)) && ver &&
					stricmp(anchor->ap_Info.fib_FileName, "ftp.module") == 0)
				{
					// AROS can fail the versioned open for ftp.module during command discovery.
					// Runtime command dispatch applies the same fallback in OpenModule().
					ModuleBase = OpenLibrary(anchor->ap_Buf, 0);
				}
				if (ModuleBase)
	#else
				if ((ModuleBase = OpenLibrary(anchor->ap_Buf, ver)))
	#endif
#endif
				{
					ModuleInfo *info;

					// Ask module to identify itself.
					// This retrieves module-wide metadata (name, function count, flags).
					if ((info = Module_Identify(-1)))
					{
						short num;
						char helpname[256], *helpptr = 0;

						// Help available?
						if (info->flags & MODULEF_HELP_AVAILABLE && info->name)
						{
							// Copy module name, strip suffix
							strcpy(helpname, info->name);
							helpptr = helpname + strlen(helpname) - 1;
							while (helpptr >= helpname && *helpptr != '.')
								--helpptr;
							if (*helpptr == '.')
								*helpptr = 0;

							// Add '.guide' suffix
							strcat(helpname, ".guide");

							// Get pointer to help name
							helpptr = helpname;
						}

						// Go through module's functions
						for (num = 0; num < info->function_count; num++)
						{
							module_add_command(info->function[num].name,
											   (char *)Module_Identify(num),
											   info->function[num].template,
											   anchor->ap_Info.fib_FileName,
											   info->function[num].flags,
											   info->function[num].id,
											   helpptr);
						}

						// Does module want to be called?
						if (info->flags & MODULEF_CALL_STARTUP)
						{
							// Run synchronously?
							if (info->flags & MODULEF_STARTUP_SYNC)
							{
								// Launch on our context
								Module_Entry(0,
											 0,
											 &main_ipc,
											 &main_ipc,
											 FUNCID_STARTUP,
											 (IPTR)GET_CALLBACK(function_external_hook));
							}

							// Run module function in background
							else if (misc_startup("dopus_module_init", MODULE_STARTUP, 0, ModuleBase, 0))
								ModuleBase = 0;
						}

						// Commands are all we need from it next time
						else if (node)
						{
							ModuleCacheWriter writer = {0, 0};
							char catalog_id[40];
							short pass;

							// Record the commands as the module reports them; a later module's
							// command of the same name may replace one in the command list
							for (pass = 0; pass < 2; pass++)
							{
								if (pass == 1 && !(writer.buffer = AllocVec(writer.pos, 0)))
									break;
								writer.pos = 0;
								module_cache_put(&writer, info->function_count, 2);
								for (num = 0; num < info->function_count; num++)
								{
									module_cache_put_command(&writer,
															 info->function[num].name,
															 (char *)Module_Identify(num),
															 info->function[num].template,
															 helpptr,
															 info->function[num].flags,
															 info->function[num].id);
								}
							}

							module_catalog_id(info->locale_name, catalog_id, sizeof(catalog_id));
							if (writer.buffer &&
								module_node_set_cache(node, info->locale_name, catalog_id, writer.buffer, writer.pos))
								cache_changed = TRUE;
							FreeVec(writer.buffer);
						}
					}
// Close module
#ifdef __amigaos4__
					DropInterface((struct Interface *)IModule);
#endif
					CloseLibrary(ModuleBase);
				}
			}

			// Find next file in directory
			error = MatchNext(anchor);
		}

		// Clean up match stuff
		MatchEnd(anchor);
	}

	FreeVec(anchor);
	if (module_cache_stale(cache))
		cache_changed = TRUE;
	module_cache_free(cache);

	// Lock modules list
	lock_listlock(&GUI->modules_list, TRUE);

	// Go through list
	for (node = (ModuleNode *)GUI->modules_list.list.lh_Head; node->node.ln_Succ;)
	{
		ModuleNode *next = (ModuleNode *)node->node.ln_Succ;

		// Wasn't found in directory?
		if (!(node->flags & MNF_TEMP))
		{
			// Module or rexx as appropriate
			if ((sufcmp(node->node.ln_Name, ".dopus5") && type & SCAN_REXX) ||
				(sufcmp(node->node.ln_Name, ".module") && type & SCAN_MODULES))
			{
				// Remove node
				Remove((struct Node *)node);
				if (node->flags & MNF_CACHE)
					cache_changed = TRUE;
				module_node_clear_cache(node);

				// Expunge the module
				command_expunge(node->node.ln_Name);

				// Free the entry
				FreeMemH(node);
			}
		}

		// Otherwise, clear flag
		else
			node->flags &= ~MNF_TEMP;

		// Get next
		node = next;
	}

	// Unlock modules list
	unlock_listlock(&GUI->modules_list);

	// Save the module commands if they changed
	if (cache_changed)
		module_cache_save();
}

// Expunge functions
void command_expunge(char *name)
{
	CommandList *cmd, *next;
	struct Library *ModuleBase;
#ifdef __amigaos4__
	struct ModuleIFace *IModule;
#endif

	// Lock command list
	lock_listlock(&GUI->command_list, TRUE);

	// Scan command list
	for (cmd = (CommandList *)GUI->command_list.list.lh_Head; cmd->node.mln_Succ; cmd = next)
	{
		// Cache next
		next = (CommandList *)cmd->node.mln_Succ;

		// Does this match the module?
		if (cmd->flags & FUNCF_EXTERNAL_FUNCTION && stricmp(cmd->stuff.module_name, name) == 0)
		{
			// Remove from list
			Remove((struct Node *)cmd);

			// Remove PopUp extensions
			remove_popup_ext(cmd->name);

			// Free entry
			FreeMemH(cmd);
		}
	}

	// Unlock command list
	unlock_listlock(&GUI->command_list);

	// Try to open module
	if (sufcmp(name, ".module") &&
#ifdef __amigaos4__
		(OpenLibIFace(name, (APTR)&ModuleBase, (APTR)&IModule, LIB_VERSION)))
#else
		(ModuleBase = OpenLibrary(name, LIB_VERSION)))
#endif
	{
		// Expunge it
		Module_Expunge();

#ifdef __amigaos4__
		DropInterface((struct Interface *)IModule);
#endif
		CloseLibrary(ModuleBase);
	}
}

// Add a command
CommandList *add_command(char *name,
						 char *desc,
						 char *temp,
						 char *file,
						 ULONG flags,
						 char *menu,
						 Att_List *type_list,
						 char *help_name,
						 ULONG ext_flags)
{
	CommandList *command, *old;
	short size;
	char *ptr;

	// Get size
	size = sizeof(CommandList);
	size += strlen(name) + 1;
	if (desc)
		size += strlen(desc) + 1;
	if (temp)
		size += strlen(temp) + 1;
	if (file)
		size += strlen(file) + 1;

	// Allocate command
	if (!(command = AllocMemH(global_memory_pool, size)))
		return 0;

	// Pointer to buffer
	ptr = (char *)(command + 1);

	// Fill in name
	command->name = ptr;
	strcpy(command->name, name);
	ptr += strlen(name) + 1;

	// Description
	if (desc)
	{
		command->desc = (IPTR)ptr;
		strcpy((char *)command->desc, desc);
		ptr += strlen(desc) + 1;
	}

	// Template
	if (temp)
	{
		command->template = ptr;
		strcpy(command->template, temp);
		ptr += strlen(temp) + 1;
	}

	// Module name
	if (file)
	{
		command->stuff.module_name = ptr;
		strcpy(command->stuff.module_name, file);
	}

	// Set flags
	command->flags = flags | FUNCF_EXTERNAL_FUNCTION;

	// Help available?
	if (help_name && *help_name)
	{
		// Allocate help name copy
		if ((command->help_name = AllocMemH(global_memory_pool, strlen(help_name) + 1)))
			strcpy(command->help_name, help_name);
	}

	// Lock command list
	lock_listlock(&GUI->command_list, TRUE);

	// See if this command exists
	if ((old = (CommandList *)FindNameI((struct List *)&GUI->command_list, command->name)))
	{
		// Remove it
		Remove((struct Node *)old);

		// Was the original command private?
		if (old->flags & FUNCF_PRIVATE)
		{
			// Make the new command private too
			command->flags |= FUNCF_PRIVATE;
		}

		// External module?
		if (old->flags & FUNCF_EXTERNAL_FUNCTION)
		{
			// Free entry
			FreeMemH(old);
		}

		// Otherwise, add it to the secondary list
		else
		{
			// Lock original command list
			lock_listlock(&GUI->original_cmd_list, TRUE);

			// Add to list
			AddTail((struct List *)&GUI->original_cmd_list, (struct Node *)old);

			// Unlock list
			unlock_listlock(&GUI->original_cmd_list);
		}
	}

	// Add to command list
	AddTail((struct List *)&GUI->command_list, (struct Node *)command);

	// Unlock command list
	unlock_listlock(&GUI->command_list);

	// Got a menu entry?
	if (menu && *menu && type_list)
		add_popup_ext(menu, type_list, command->name, ext_flags);
	return command;
}

// Find an internal function
CommandList *function_find_internal(char **name, short original)
{
	CommandList *command;
	char *nameptr;
	short a;

	// Invalid string?
	if (!(nameptr = *name) || !*nameptr)
		return 0;

	// Do this twice
	for (a = 0; a < 2; a++)
	{
		// Original?
		if (original)
		{
			// Lock it and get head
			lock_listlock(&GUI->original_cmd_list, FALSE);
			command = (CommandList *)GUI->original_cmd_list.list.lh_Head;
		}

		// Command list
		else
		{
			// Lock it and get head
			lock_listlock(&GUI->command_list, FALSE);
			command = (CommandList *)GUI->command_list.list.lh_Head;
		}

		// Go through command list
		for (; command->node.mln_Succ; command = (CommandList *)command->node.mln_Succ)
		{
			short len;

			// Valid name?
			if (command->name)
			{
				// Get length of this command name
				len = strlen(command->name);

				// See if command string matches
				if (strnicmp(command->name, nameptr, len) == 0 && (nameptr[len] == ' ' || nameptr[len] == 0))
				{
					// Bump name pointer to start of arguments (if any)
					nameptr += len;
					while (*nameptr == ' ')
						++nameptr;
					if (!*nameptr)
						nameptr = 0;

					// Return pointer to command
					*name = nameptr;

					// Unlock list
					unlock_listlock((original) ? &GUI->original_cmd_list : &GUI->command_list);
					return command;
				}
			}
		}

		// Unlock command list
		unlock_listlock((original) ? &GUI->original_cmd_list : &GUI->command_list);

		// If doing original, loop around for a try at the main list
		if (original)
			original = 0;
		else
			break;
	}

	// Not found
	return 0;
}

// Create a new command
void command_new(BackdropInfo *info, IPCData *ipc, char *filename)
{
	Cfg_Function *func, *edit_func = 0;
	D_S(struct FileInfoBlock, fib)
	char buffer[256];
	APTR iff = 0;
#ifndef __amigaos3__
	struct Library *ConfigOpusBase;
	#ifdef __amigaos4__
	struct ConfigOpusIFace *IConfigOpus;
	#endif
#endif

	// Initialise buffers
	fib->fib_Comment[0] = 0;

	// Given filename?
	if (filename)
	{
		// Can't open?
		if (!(iff = IFFOpen(filename, IFF_READ, ID_OPUS)))
			return;

		// Find function chunk
		if (IFFNextChunk(iff, ID_FUNC) == ID_FUNC)
		{
			// Read function
			edit_func = ReadFunction(iff, global_memory_pool, 0, 0);
		}

		// Close IFF file
		IFFClose(iff);
		iff = 0;

		// Couldn't get function?
		if (!edit_func)
		{
			DisplayBeep(info->window->WScreen);
			return;
		}

		// Get current comment
		GetFileInfo(filename, fib);
	}

	// Open config library
	if (!(ConfigOpusBase = OpenModule(config_name))
#ifdef __amigaos4__
		|| !(IConfigOpus = (struct ConfigOpusIFace *)GetInterface(ConfigOpusBase, "main", 1, NULL))
#endif
	)
	{
		if (ConfigOpusBase)
			CloseLibrary(ConfigOpusBase);
		FreeFunction(edit_func);
		return;
	}

	// Create a new command
	func = Config_EditFunction(
		ipc, &main_ipc, info->window, edit_func, global_memory_pool, (IPTR)&GUI->command_list.list);

	// Free edit function
	FreeFunction(edit_func);

	// Success?
	if (func)
	{
		short leaveout = 1;

		// Initialise file buffer
		if (filename)
			strcpy(buffer, filename);
		else
			*buffer = 0;

		// Ask user for a description
		if (super_request_args(info->window,
							   GetString(&locale, MSG_COMMAND_DESCRIPTION),
							   SRF_BUFFER | SRF_CHECKMARK,
							   fib->fib_Comment,
							   79,
							   GetString(&locale, MSG_COMMAND_LEAVEOUT),
							   &leaveout,
							   GetString(&locale, MSG_OKAY),
							   GetString(&locale, MSG_CANCEL),
							   0))
		{
			// Don't leave out?
			if (!leaveout)
				edit_func = (Cfg_Function *)1;

			// Loop to open file
			while (!iff)
			{
				// Get file to save (unless given filename)
				if (!filename && !(request_file(info->window,
												GetString(&locale, MSG_COMMAND_SAVE),
												buffer,
												"DOpus5:Commands/",
												FRF_DOSAVEMODE,
												0)))
					break;

				// Try to open file
				if (!(iff = IFFOpen(buffer, IFF_WRITE, ID_OPUS)))
				{
					// No longer use supplied filename
					filename = 0;

					// Show error
					if (!(error_request(info->window,
										FALSE,
										GetString(&locale, MSG_SAVING_COMMAND),
										(char *)-1,
										FilePart(buffer),
										"\n",
										0)))
						break;
				}
			}
		}
	}

	// Got file?
	if (iff)
	{
		// Save function
		SaveFunction(iff, func);

		// Close file
		IFFClose(iff);

		// Set the comment
		SetComment(buffer, fib->fib_Comment);

		// Write the default command icon if enabled and there isn't one already
		if (GUI->flags & GUIF_SAVE_ICONS)
		{
			BPTR icon_lock;

			StrCombine(info->buffer, buffer, ".info", sizeof(info->buffer));
			if ((icon_lock = Lock(info->buffer, ACCESS_READ)))
				UnLock(icon_lock);
			else
				icon_write(ICONTYPE_PROJECT, buffer, 0, 0, 0, 0);
		}

		// Only add as a leftout if not editing an existing function
		if (!edit_func)
		{
			// Leave the new function out
			if (backdrop_leave_out(info, buffer, BLOF_PERMANENT | BLOF_REFRESH, -1, -1))
			{
				// Save leftout list
				backdrop_save_leftouts(info);
			}
		}
	}

	// Free function
	FreeFunction(func);

// Close module
#ifdef __amigaos4__
	DropInterface((struct Interface *)IConfigOpus);
#endif
	CloseLibrary(ConfigOpusBase);
}

// Remove a command
void command_remove(char *name)
{
	CommandList *cmd, *next;

	// Lock command list
	lock_listlock(&GUI->command_list, TRUE);

	// Scan command list
	for (cmd = (CommandList *)GUI->command_list.list.lh_Head; cmd->node.mln_Succ; cmd = next)
	{
		// Cache next
		next = (CommandList *)cmd->node.mln_Succ;

		// Does this match the command?
		if (cmd->flags & FUNCF_EXTERNAL_FUNCTION && stricmp(cmd->name, name) == 0)
		{
			// Remove from list
			Remove((struct Node *)cmd);

			// Remove PopUp extensions
			remove_popup_ext(cmd->name);

			// Free entry
			FreeMemH(cmd);
		}
	}

	// Unlock command list
	unlock_listlock(&GUI->command_list);
}

// See if a backdrop object is an Opus command file
BOOL command_filetype_match(BackdropObject *object)
{
	BPTR lock, old, commands_lock;
	BOOL match = 0;

	if (!object || object->type != BDO_LEFT_OUT || object->flags & BDOF_DESKTOP_FOLDER)
		return 0;

	if ((lock = backdrop_icon_lock(object)))
	{
		old = CurrentDir(lock);

		if (command_filetype)
			match = filetype_match_type(object->name, command_filetype);

		if (!match && object->icon && object->icon->do_Type == WBPROJECT &&
			(commands_lock = Lock("DOpus5:Commands", ACCESS_READ)))
		{
			if (SameLock(lock, commands_lock) == LOCK_SAME)
				match = 1;
			UnLock(commands_lock);
		}

		UnLock(CurrentDir(old));
	}

	return match;
}
