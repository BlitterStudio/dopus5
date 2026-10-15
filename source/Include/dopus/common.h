/* AmigaOS includes & definitions */

#ifndef DOPUS_COMMON_H
#define DOPUS_COMMON_H

// Temporary defines
#undef __chip
#define __chip
// End Temporary defines

#define __ARGS(x) x
#define DOPUS_WIN_NAME "dOpUs5.win"

#ifdef __amigaos4__
	#include <amiga_compiler.h>
#endif
#include <SDI/SDI_compiler.h>
#include <SDI/SDI_lib.h>
#include <SDI/SDI_stdarg.h>

#ifdef __amigaos4__
	#undef DEPRECATED
	#define DEPRECATED
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>

#ifdef __amigaos4__
#define MAX_FILENAME_LEN	255
#else
#define MAX_FILENAME_LEN	107
#endif

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/locale.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/wb.h>
#include <proto/icon.h>
#include <proto/iffparse.h>
#include <proto/locale.h>
#include <proto/commodities.h>
#include <proto/asl.h>
#include <proto/gadtools.h>
#include <proto/console.h>
#include <proto/timer.h>
#include <proto/diskfont.h>
#include <proto/input.h>
#include <proto/datatypes.h>
#include <proto/layers.h>
#include <proto/rexxsyslib.h>
#include <proto/Picasso96.h>
/* cybergraphics.library kept as a fallback for systems without Picasso96;
 * 24-bit ILBM write path in read_ilbm.c uses it when P96Base is NULL.
 * MorphOS / AROS need the cybergraphx header explicitly for RECTFMT_*
 * constants; on OS3 / OS4 the proto header pulls them in. */
#if defined(__MORPHOS__) || defined(__AROS__)
	#include <cybergraphx/cybergraphics.h>
#endif
#include <proto/cybergraphics.h>

#ifdef __amigaos4__
	#include <dos/obsolete.h>
#endif
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <dos/dostags.h>
#include <dos/filehandler.h>
#include <devices/trackdisk.h>
#include <graphics/gfxmacros.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/icclass.h>
#include <intuition/cghooks.h>
#include <intuition/sghooks.h>
#include <workbench/startup.h>
#include <workbench/icon.h>
#include <prefs/wbpattern.h>
#include <prefs/prefhdr.h>
#include <prefs/font.h>
#include <datatypes/animationclass.h>
#include <datatypes/textclass.h>

#ifdef __amigaos3__
	#include <clib/alib_protos.h>
	#include <intuition/intuitionbase.h>
	// Declare Utilitybase ourselves as NDK3.9 and NDK 3.2 can't agree on the type.
	#define __NOLIBBASE__
	#include <proto/utility.h>
	extern struct Library *
	# ifdef __CONSTLIBBASEDECL__
	__CONSTLIBBASEDECL__
	# endif /* __CONSTLIBBASEDECL__ */
		UtilityBase;
	#undef __NOLIBBASE__
#else
	#include <proto/utility.h>
#endif

#ifdef __MORPHOS__
	#include <intuition/intuitionbase.h>
	#include <clib/alib_protos.h>
#endif

#ifdef __AROS__
	#include <proto/alib.h>
	#include <graphics/gfxbase.h>
	#include <aros/preprocessor/variadic/cast2iptr.hpp>
#endif

#include <dopus/debug.h>
#include <dopus/version.h>
#include <dopus/stack.h>

#ifdef __amigaos4__
	#include <exec/emulation.h>	 // necessary 68k emul-based parts
	#include <dos/stdio.h>		 // for #define Flush(x) FFlush(x)
	#define REG68K_d0 REG68K_D0	 // make macros work for all builds
	#define REG68K_d1 REG68K_D1
	#define REG68K_a0 REG68K_A0
	#define REG68K_a1 REG68K_A1
	#define REG68K_a2 REG68K_A2
	#define REG68K_a3 REG68K_A3
	#define REG68K_a4 REG68K_A4
#endif

#ifndef RAWKEY_WHEEL_UP
	#define RAWKEY_WHEEL_UP 0x7a
#endif
#ifndef RAWKEY_WHEEL_DOWN
	#define RAWKEY_WHEEL_DOWN 0x7b
#endif

#define DOPUS_NATIVE_WHEEL_MIN_VERSION 47
#define DOPUS_NATIVE_WHEEL_SUPPORTED() \
	((BOOL)(((struct Library *)IntuitionBase)->lib_Version >= DOPUS_NATIVE_WHEEL_MIN_VERSION))

/* Intuition 47.44+ (AmigaOS 3.2) draws a propgclass knob with a BOOPSI image
 * passed as PGA_KnobImage. GadTools 47 gives its scrollers a frameiclass
 * FRAME_PROPKNOB image (vertical orientation = 1) under the same version
 * check, so this knob follows the system look. Intuition sizes the image
 * through IM_DRAWFRAME; the caller disposes it after the gadget.
 * Only for scrollers inside the window: Intuition also honours the image
 * on border scrollers, where it replaces the system border knob. */
#ifdef __amigaos3__
	#define DOPUS_PROP_KNOB_SUPPORTED()                                       \
		((BOOL)(((struct Library *)IntuitionBase)->lib_Version > 47 ||        \
				(((struct Library *)IntuitionBase)->lib_Version == 47 &&      \
				 ((struct Library *)IntuitionBase)->lib_Revision >= 44)))
	#define DOPUS_NEW_PROP_KNOB(vertical)                                     \
		((APTR)(DOPUS_PROP_KNOB_SUPPORTED()                                   \
					? NewObject(0,                                            \
								"frameiclass",                                \
								IA_FrameType,                                 \
								FRAME_PROPKNOB,                               \
								IA_Orientation,                               \
								(vertical) ? 1 : 0,                           \
								TAG_END)                                      \
					: 0))
	#define DOPUS_PROP_KNOB_TAG(knob) ((knob) ? PGA_KnobImage : TAG_IGNORE)
#else
	#define DOPUS_NEW_PROP_KNOB(vertical) ((APTR)0)
	#define DOPUS_PROP_KNOB_TAG(knob) TAG_IGNORE
#endif

/* Long word alignement (mainly used to get
 * FIB or DISK_INFO as auto variables)
 */
#define D_S(type, name)              \
	char a_##name[sizeof(type) + 3]; \
	type *name = (type *)((IPTR)(a_##name + 3) & ~3);

// no need for old functions
#undef UDivMod32
#define UDivMod32(x, y) (((ULONG)x) / ((ULONG)y))
#undef SDivMod32
#define SDivMod32(x, y) (((LONG)x) / ((LONG)y))
#undef UMult32
#define UMult32(x, y) (((ULONG)x) * ((ULONG)y))

#ifdef __AROS__
	#define DOPUS_VARIADIC_IPTR(...) AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__)
#else
	#define DOPUS_VARIADIC_IPTR(...) __VA_ARGS__
#endif

/* IsListEmpty equivalent for struct MinList; using IsListEmpty with a
 * (struct List *) cast triggers -Wstrict-aliasing on GCC even though
 * the layout matches. This macro accesses the MinList fields directly
 * so the strict-aliasing rules stay clean. */
#ifndef IsMinListEmpty
	#define IsMinListEmpty(l) ((l)->mlh_TailPred == (struct MinNode *)(l))
#endif

/* Replacement functions for functions not available in some SDKs/GCCs */
#if !defined(__MORPHOS__) && !defined(__AROS__)
	#undef stccpy
int stccpy(char *p, const char *q, int n);
#endif

#if defined(__amigaos3__)
	#define lsprintf(buf, fmt, ...)                                                \
		({                                                                         \
			static ULONG StuffChar = 0x16c04e75;                                   \
			IPTR vargs[] = {DOPUS_VARIADIC_IPTR(__VA_ARGS__)};                    \
			RawDoFmt((STRPTR)(IPTR)fmt, (APTR)&vargs, (void(*)) & StuffChar, (APTR)buf); \
		})
	#define LSprintf(buffer, string, data)                         \
		({                                                         \
			static ULONG StuffChar = 0x16c04e75;                   \
			RawDoFmt(string, data, (void(*)) & StuffChar, buffer); \
		})
#else
	#define lsprintf(buf, fmt, ...)                               \
		({                                                        \
			IPTR vargs[] = {DOPUS_VARIADIC_IPTR(__VA_ARGS__)};    \
			RawDoFmt((STRPTR)(IPTR)fmt, (APTR)&vargs, NULL, (APTR)buf); \
		})
	#define LSprintf(buffer, string, data) RawDoFmt(string, data, NULL, buffer)
#endif

#endif /* DOPUS_COMMON_H */
