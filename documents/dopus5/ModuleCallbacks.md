# Directory Opus 5 Module Callback System

This document describes the external command callback system used by modules to communicate with Directory Opus 5.

## Overview

Modules interact with Directory Opus 5 via a callback hook, `function_external_hook`. This hook is passed to the module's entry point and can be called by the module with various `EXTCMD_*` codes and associated data packets.

The implementation is primarily located in:
- `source/Include/libraries/module.h`: Definitions of `EXTCMD_*` codes and packet structures.
- `source/Program/function_internal.c`: The main callback dispatcher (`function_external_hook`).
- `source/Program/callback_lister.c`: Most implementations of the actual hook functions.
- `source/Program/callback_help.c`: Implementation for help-related callbacks.

---

## Callback Command Reference

### `EXTCMD_GET_SOURCE` (0)
- **Description**: Gets the current source path.
- **Handler**: `HookGetSource` (`source/Program/callback_lister.c`)
- **Packet**: `char *pathbuf` (Pointer to a buffer of at least 256 characters)
- **Return Value**: `PathNode *` (Pointer to the current source PathNode, or 0 if none)
- **Synchronous**: Yes

### `EXTCMD_NEXT_SOURCE` (1)
- **Description**: Advances to the next source path and retrieves it.
- **Handler**: `HookNextSource` (`source/Program/callback_lister.c`)
- **Packet**: `char *pathbuf` (Pointer to a buffer of at least 256 characters)
- **Return Value**: `PathNode *` (Pointer to the next source PathNode, or 0 if none)
- **Synchronous**: Yes

### `EXTCMD_UNLOCK_SOURCE` (2)
- **Description**: Unlocks the current source paths.
- **Handler**: `HookUnlockSource` (`source/Program/callback_lister.c`)
- **Packet**: None
- **Return Value**: None
- **Synchronous**: Yes

### `EXTCMD_GET_ENTRY` (3)
- **Description**: Gets the next entry from the current source.
- **Handler**: `HookGetEntry` (`source/Program/callback_lister.c`)
- **Packet**: None
- **Return Value**: `APTR` (Pointer to a `FunctionEntry` or equivalent, or 0 if no more entries)
- **Synchronous**: Yes

### `EXTCMD_END_ENTRY` (4)
- **Description**: Finishes processing a specific entry.
- **Handler**: `HookEndEntry` (`source/Program/callback_lister.c`)
- **Packet**: `struct endentry_packet *`
- **Return Value**: None
- **Synchronous**: Yes

### `EXTCMD_RELOAD_ENTRY` (5)
- **Description**: Marks an entry for reloading in its lister.
- **Handler**: `HookReloadEntry` (`source/Program/callback_lister.c`)
- **Packet**: `FunctionEntry *entry`
- **Return Value**: None
- **Synchronous**: Yes

### `EXTCMD_OPEN_PROGRESS` (6)
- **Description**: Opens a progress indicator for the current operation.
- **Handler**: `HookOpenProgress` (`source/Program/callback_lister.c`)
- **Packet**: `struct progress_packet *`
- **Return Value**: None
- **Synchronous**: Yes (Calls `IPC_Command` internally)

### `EXTCMD_UPDATE_PROGRESS` (7)
- **Description**: Updates an existing progress indicator.
- **Handler**: `HookUpdateProgress` (`source/Program/callback_lister.c`)
- **Packet**: `struct progress_packet *`
- **Return Value**: None
- **Synchronous**: Yes (Calls `lister_command` internally)

### `EXTCMD_CLOSE_PROGRESS` (8)
- **Description**: Closes the progress indicator.
- **Handler**: `HookCloseProgress` (`source/Program/callback_lister.c`)
- **Packet**: `struct progress_packet *` (Uses the `path` member)
- **Return Value**: None
- **Synchronous**: Yes (Calls `lister_command` internally)

### `EXTCMD_CHECK_ABORT` (9)
- **Description**: Checks if the user has requested to abort the current operation.
- **Handler**: `HookCheckAbort` (`source/Program/callback_lister.c`)
- **Packet**: None
- **Return Value**: `BOOL` (True if aborted, False otherwise)
- **Synchronous**: Yes

### `EXTCMD_ENTRY_COUNT` (10)
- **Description**: Returns the total number of entries to be processed.
- **Handler**: `HookEntryCount` (`source/Program/callback_lister.c`)
- **Packet**: None
- **Return Value**: `long` (Number of entries)
- **Synchronous**: Yes

### `EXTCMD_GET_WINDOW` (11)
- **Description**: Gets the window handle associated with a PathNode.
- **Handler**: `HookGetWindow` (`source/Program/callback_lister.c`)
- **Packet**: `PathNode *path`
- **Return Value**: `struct Window *`
- **Synchronous**: Yes

### `EXTCMD_GET_DEST` (12)
- **Description**: Gets the current destination path.
- **Handler**: `HookGetDest` (`source/Program/callback_lister.c`)
- **Packet**: `char *pathbuf`
- **Return Value**: `PathNode *`
- **Synchronous**: Yes
- **Refreshing the destination**: Do not send `ScanDir` for the destination lister from inside the module call. The calling function holds that lister busy, so the second function process cannot lock it and opens a new lister instead. Before calling the module, `function_internal_command()` marks the destination path node for a rescan if the module command was registered with `FUNCF_NEED_DEST` or the calling function has `FUNCF_RESCAN_DEST`; the lister reads its directory again in place when the function finishes and releases it.

### `EXTCMD_END_SOURCE` (13)
- **Description**: Finishes with the current source path.
- **Handler**: `HookEndSource` (`source/Program/callback_lister.c`)
- **Packet**: `long complete` (Status flag)
- **Return Value**: None
- **Synchronous**: Yes

### `EXTCMD_END_DEST` (14)
- **Description**: Finishes with the current destination path.
- **Handler**: `HookEndDest` (`source/Program/callback_lister.c`)
- **Packet**: `long complete` (Status flag)
- **Return Value**: None
- **Synchronous**: Yes

### `EXTCMD_ADD_CHANGE` (15)
- **Description**: (Not implemented in `function_internal.c` handler - possibly deprecated)
- **Handler**: N/A
- **Packet**: `struct addchange_packet *`
- **Return Value**: N/A
- **Synchronous**: N/A

### `EXTCMD_ADD_FILE` (16)
- **Description**: Adds a new file entry to a lister.
- **Handler**: `HookAddFile` (`source/Program/callback_lister.c`)
- **Packet**: `struct addfile_packet *`
- **Return Value**: None
- **Synchronous**: Yes

### `EXTCMD_GET_HELP` (17)
- **Description**: Shows help for a specific topic or file.
- **Handler**: `HookShowHelp` (`source/Program/callback_help.c`)
- **Packet**: `char *topic`
- **Return Value**: None
- **Synchronous**: Yes

### `EXTCMD_GET_PORT` (18)
- **Description**: Gets the name of the DOpus ARexx port.
- **Handler**: `HookGetPort` (`source/Program/callback_lister.c`)
- **Packet**: `char *portname` (Buffer to store name)
- **Return Value**: `struct MsgPort *`
- **Synchronous**: Yes

### `EXTCMD_GET_SCREEN` (19)
- **Description**: Gets the name and handle of the DOpus public screen.
- **Handler**: `HookGetScreen` (`source/Program/callback_lister.c`)
- **Packet**: `char *screenname` (Buffer to store name)
- **Return Value**: `struct Screen *`
- **Synchronous**: Yes

### `EXTCMD_REPLACE_REQ` (20)
- **Description**: Shows the File Exists / Replace requester.
- **Handler**: `HookReplaceReq` (`source/Program/callback_lister.c`)
- **Packet**: `struct replacereq_packet *`
- **Return Value**: `long` (Result of requester)
- **Synchronous**: Yes

### `EXTCMD_REMOVE_ENTRY` (21)
- **Description**: Marks an entry for removal from its list.
- **Handler**: `HookRemoveEntry` (`source/Program/callback_lister.c`)
- **Packet**: `FunctionEntry *entry`
- **Return Value**: None
- **Synchronous**: Yes

### `EXTCMD_GET_SCREENDATA` (22)
- **Description**: Retrieves data about the DOpus screen (colors, depth, etc).
- **Handler**: `HookGetScreenData` (`source/Program/callback_lister.c`)
- **Packet**: None
- **Return Value**: `DOpusScreenData *` (Must be freed with `EXTCMD_FREE_SCREENDATA`)
- **Synchronous**: Yes

### `EXTCMD_FREE_SCREENDATA` (23)
- **Description**: Frees screen data obtained from `EXTCMD_GET_SCREENDATA`.
- **Handler**: `HookFreeScreenData` (`source/Program/callback_lister.c`)
- **Packet**: `DOpusScreenData *data`
- **Return Value**: None
- **Synchronous**: Yes

### `EXTCMD_CREATE_FILE_ENTRY` (24)
- **Description**: (Not implemented in `function_internal.c` handler)
- **Handler**: N/A
- **Packet**: N/A
- **Return Value**: N/A
- **Synchronous**: N/A

### `EXTCMD_FREE_FILE_ENTRY` (25)
- **Description**: (Not implemented in `function_internal.c` handler)
- **Handler**: N/A
- **Packet**: N/A
- **Return Value**: N/A
- **Synchronous**: N/A

### `EXTCMD_SORT_FILELIST` (26)
- **Description**: (Not implemented in `function_internal.c` handler)
- **Handler**: N/A
- **Packet**: `struct sortlist_packet *`
- **Return Value**: N/A
- **Synchronous**: N/A

### `EXTCMD_NEW_LISTER` (27)
- **Description**: (Not implemented in `function_internal.c` handler)
- **Handler**: N/A
- **Packet**: N/A
- **Return Value**: N/A
- **Synchronous**: N/A

### `EXTCMD_GET_POINTER` (28)
- **Description**: Retrieves a pointer to a specific internal DOpus structure.
- **Handler**: `HookGetPointer` (`source/Program/callback_lister.c`)
- **Packet**: `struct pointer_packet *`
- **Return Value**: `ULONG` (Pointer value)
- **Synchronous**: Yes

### `EXTCMD_FREE_POINTER` (29)
- **Description**: Frees a pointer obtained via `EXTCMD_GET_POINTER`.
- **Handler**: `HookFreePointer` (`source/Program/callback_lister.c`)
- **Packet**: `struct pointer_packet *`
- **Return Value**: None
- **Synchronous**: Yes

### `EXTCMD_SEND_COMMAND` (30)
- **Description**: Sends an ARexx command to DOpus.
- **Handler**: `HookSendCommand` (`source/Program/callback_lister.c`)
- **Packet**: `struct command_packet *`
- **Return Value**: `ULONG` (Success flag)
- **Synchronous**: Yes (Waits for ARexx reply if `COMMANDF_RESULT` is set)

### `EXTCMD_DEL_FILE` (31)
- **Description**: Deletes a file and updates the lister.
- **Handler**: `HookDelFile` (`source/Program/callback_lister.c`)
- **Packet**: `struct delfile_packet *`
- **Return Value**: None
- **Synchronous**: Yes

### `EXTCMD_DO_CHANGES` (32)
- **Description**: Flushes any pending lister changes.
- **Handler**: `HookDoChanges` (`source/Program/callback_lister.c`)
- **Packet**: None
- **Return Value**: None
- **Synchronous**: Yes

### `EXTCMD_LOAD_FILE` (33)
- **Description**: Loads a file into a lister or reloads it.
- **Handler**: `HookLoadFile` (`source/Program/callback_lister.c`)
- **Packet**: `struct loadfile_packet *`
- **Return Value**: None
- **Synchronous**: Yes

---

## Packet Structures

### `progress_packet`
```c
struct progress_packet {
    struct _PathNode *path;
    char *name;
    ULONG count;
};
```

### `endentry_packet`
```c
struct endentry_packet {
    struct _FunctionEntry *entry;
    BOOL deselect;
};
```

### `addfile_packet`
```c
struct addfile_packet {
    char *path;
    struct FileInfoBlock *fib;
    struct ListerWindow *lister;
};
```

### `delfile_packet`
```c
struct delfile_packet {
    char *path;
    char *name;
    struct ListerWindow *lister;
};
```

### `loadfile_packet`
```c
struct loadfile_packet {
    char *path;
    char *name;
    short flags;
    short reload;
};
```

### `replacereq_packet`
```c
struct replacereq_packet {
    struct Window *window;
    struct Screen *screen;
    IPCData *ipc;
    struct FileInfoBlock *file1;
    struct FileInfoBlock *file2;
    short default_option;
};
```

### `pointer_packet`
```c
struct pointer_packet {
    ULONG type;
    APTR pointer;
    ULONG flags;
};
```

### `command_packet`
```c
struct command_packet {
    char *command;
    ULONG flags;
    char *result;
    ULONG rc;
};
```
