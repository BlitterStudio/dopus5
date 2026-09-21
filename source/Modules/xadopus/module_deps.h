#ifndef _MODULE_DEPS_H
#define _MODULE_DEPS_H

#include <dopus/common.h>
#include <proto/dopus5.h>
#include <proto/module.h>
#include <proto/xadmaster.h>

extern struct DOpusLocale *locale;
#ifndef __amigaos3__
	#pragma pack(2)
#endif
typedef struct
{
	ULONG ver;					 // Module version
	char *name;					 // Module name
	char *locale_name;			 // Catalog name
	ULONG flags;				 // Module flags
	ULONG function_count;		 // Number of functions in module
	ModuleFunction function[3];	 // All the module functions
} ModuleInfo_2;
#ifndef __amigaos3__
	#pragma pack()
#endif
extern ModuleInfo_2 module_info;

#define VALID_QUALIFIERS                                                                                           \
	(IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND | IEQUALIFIER_CONTROL | IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT | \
	 IEQUALIFIER_LALT | IEQUALIFIER_RALT)

void init_locale_data(struct DOpusLocale *);

#define userlibname "xadopus.module"
#define textsegmentname "\0$VER: xadopus.module " LIB_STRING "Written by Mladen Milinkovic, 2002 by Amis"

#endif
