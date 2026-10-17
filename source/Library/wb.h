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
#ifndef _WB_H
#define _WB_H

#if defined(__amigaos4__)

	#define LIBCALL_0(ret, func, callbase, callface) ((ret ASM(*)(APTR __local_iface))func)(callface)

	#define LIBCALL_1(ret, func, callbase, callface, r1, n1) \
		((ret ASM(*)(APTR __local_iface, REG(r1, ULONG)))func)(callface, (ULONG)n1)

	#define LIBCALL_2(ret, func, callbase, callface, r1, n1, r2, n2) \
		((ret ASM(*)(APTR __local_iface, REG(r1, ULONG), REG(r2, ULONG)))func)(callface, (ULONG)n1, (ULONG)n2)

	#define LIBCALL_3(ret, func, callbase, callface, r1, n1, r2, n2, r3, n3)                    \
		((ret ASM(*)(APTR __local_iface, REG(r1, ULONG), REG(r2, ULONG), REG(r3, ULONG)))func)( \
			callface, (ULONG)n1, (ULONG)n2, (ULONG)n3)

	#define LIBCALL_5(ret, func, callbase, callface, r1, n1, r2, n2, r3, n3, r4, n4, r5, n5)                           \
		((ret ASM(*)(                                                                                                  \
			APTR __local_iface, REG(r1, ULONG), REG(r2, ULONG), REG(r3, ULONG), REG(r4, ULONG), REG(r5, ULONG)))func)( \
			callface, (ULONG)n1, (ULONG)n2, (ULONG)n3, (ULONG)n4, (ULONG)n5)

	#define LIBCALL_7(ret, func, callbase, callface, r1, n1, r2, n2, r3, n3, r4, n4, r5, n5, r6, n6, r7, n7) \
		((ret ASM(*)(APTR __local_iface,                                                                     \
					 REG(r1, ULONG),                                                                         \
					 REG(r2, ULONG),                                                                         \
					 REG(r3, ULONG),                                                                         \
					 REG(r4, ULONG),                                                                         \
					 REG(r5, ULONG),                                                                         \
					 REG(r6, ULONG),                                                                         \
					 REG(r7, ULONG)))func)(                                                                  \
			callface, (ULONG)n1, (ULONG)n2, (ULONG)n3, (ULONG)n4, (ULONG)n5, (ULONG)n6, (ULONG)n7)

	#define PATCH(jumptable_dummy, offset, func, type) \
		{                                              \
			offset, &func##_stubs, type                \
		}

	#define PATCHED_0(ret, name)                                     \
		ret name##_stubs(APTR __local_iface) { return (ret)name(); } \
		ret name(void)                                               \
		{
	#define PATCHED_1(ret, name, r1, t1, n1)                 \
		ret name##_stubs(APTR __local_iface, REG(r1, t1 n1)) \
		{                                                    \
			extern ret name(REG(r1, t1 n1));                 \
			return (ret)name(n1);                            \
		}                                                    \
		ret name(t1 n1)                                      \
		{
	#define PATCHED_2(ret, name, r1, t1, n1, r2, t2, n2)                     \
		ret name##_stubs(APTR __local_iface, REG(r1, t1 n1), REG(r2, t2 n2)) \
		{                                                                    \
			extern ret name(REG(r1, t1 n1), REG(r2, t2 n2));                 \
			return (ret)name(n1, n2);                                        \
		}                                                                    \
		ret name(t1 n1, t2 n2)                                               \
		{
	#define PATCHED_3(ret, name, r1, t1, n1, r2, t2, n2, r3, t3, n3)                         \
		ret name##_stubs(APTR __local_iface, REG(r1, t1 n1), REG(r2, t2 n2), REG(r3, t3 n3)) \
		{                                                                                    \
			extern ret name(REG(r1, t1 n1), REG(r2, t2 n2), REG(r3, t3 n3));                 \
			return (ret)name(n1, n2, n3);                                                    \
		}                                                                                    \
		ret name(t1 n1, t2 n2, t3 n3)                                                        \
		{
	#define PATCHED_5(ret, name, r1, t1, n1, r2, t2, n2, r3, t3, n3, r4, t4, n4, r5, t5, n5)                    \
		ret name##_stubs(                                                                                       \
			APTR __local_iface, REG(r1, t1 n1), REG(r2, t2 n2), REG(r3, t3 n3), REG(r4, t4 n4), REG(r5, t5 n5)) \
		{                                                                                                       \
			extern ret name(REG(r1, t1 n1), REG(r2, t2 n2), REG(r3, t3 n3), REG(r4, t4 n4), REG(r5, t5 n5));    \
			return (ret)name(n1, n2, n3, n4, n5);                                                               \
		}                                                                                                       \
		ret name(t1 n1, t2 n2, t3 n3, t4 n4, t5 n5)                                                             \
		{
	#define PATCHED_6(ret, name, r1, t1, n1, r2, t2, n2, r3, t3, n3, r4, t4, n4, r5, t5, n5, r6, t6, n6)         \
		ret name##_stubs(APTR __local_iface,                                                                     \
						 REG(r1, t1 n1),                                                                         \
						 REG(r2, t2 n2),                                                                         \
						 REG(r3, t3 n3),                                                                         \
						 REG(r4, t4 n4),                                                                         \
						 REG(r5, t5 n5),                                                                         \
						 REG(r6, t6 n6))                                                                         \
		{                                                                                                        \
			extern ret name(                                                                                     \
				REG(r1, t1 n1), REG(r2, t2 n2), REG(r3, t3 n3), REG(r4, t4 n4), REG(r5, t5 n5), REG(r6, t6 n6)); \
			return (ret)name(n1, n2, n3, n4, n5, n6);                                                            \
		}                                                                                                        \
		ret name(t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6)                                                       \
		{
	#define PATCHED_7(ret, name, r1, t1, n1, r2, t2, n2, r3, t3, n3, r4, t4, n4, r5, t5, n5, r6, t6, n6, r7, t7, n7) \
		ret name##_stubs(APTR __local_iface,                                                                         \
						 REG(r1, t1 n1),                                                                             \
						 REG(r2, t2 n2),                                                                             \
						 REG(r3, t3 n3),                                                                             \
						 REG(r4, t4 n4),                                                                             \
						 REG(r5, t5 n5),                                                                             \
						 REG(r6, t6 n6),                                                                             \
						 REG(r7, t7 n7))                                                                             \
		{                                                                                                            \
			extern ret name(REG(r1, t1 n1),                                                                          \
							REG(r2, t2 n2),                                                                          \
							REG(r3, t3 n3),                                                                          \
							REG(r4, t4 n4),                                                                          \
							REG(r5, t5 n5),                                                                          \
							REG(r6, t6 n6),                                                                          \
							REG(r7, t7 n7));                                                                         \
			return (ret)name(n1, n2, n3, n4, n5, n6, n7);                                                            \
		}                                                                                                            \
		ret name(t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6, t7 n7)                                                    \
		{
#elif defined(__MORPHOS__)

	#define LIBCALL_0(ret, func, callbase, dummy)           \
		({                                                  \
			REG_A6 = (ULONG)callbase;                       \
			(ret) (*MyEmulHandle->EmulCallDirect68k)(func); \
		})

	#define LIBCALL_1(ret, func, callbase, dummy, r1, n1)   \
		({                                                  \
			REG_##r1 = (ULONG)n1;                           \
			REG_A6 = (ULONG)callbase;                       \
			(ret) (*MyEmulHandle->EmulCallDirect68k)(func); \
		})

	#define LIBCALL_2(ret, func, callbase, dummy, r1, n1, r2, n2) \
		({                                                        \
			REG_##r1 = (ULONG)n1;                                 \
			REG_##r2 = (ULONG)n2;                                 \
			REG_A6 = (ULONG)callbase;                             \
			(ret) (*MyEmulHandle->EmulCallDirect68k)(func);       \
		})

	#define LIBCALL_3(ret, func, callbase, dummy, r1, n1, r2, n2, r3, n3) \
		({                                                                \
			REG_##r1 = (ULONG)n1;                                         \
			REG_##r2 = (ULONG)n2;                                         \
			REG_##r3 = (ULONG)n3;                                         \
			REG_A6 = (ULONG)callbase;                                     \
			(ret) (*MyEmulHandle->EmulCallDirect68k)(func);               \
		})

	#define LIBCALL_5(ret, func, callbase, dummy, r1, n1, r2, n2, r3, n3, r4, n4, r5, n5) \
		({                                                                                \
			REG_##r1 = (ULONG)n1;                                                         \
			REG_##r2 = (ULONG)n2;                                                         \
			REG_##r3 = (ULONG)n3;                                                         \
			REG_##r4 = (ULONG)n4;                                                         \
			REG_##r5 = (ULONG)n5;                                                         \
			REG_A6 = (ULONG)callbase;                                                     \
			(ret) (*MyEmulHandle->EmulCallDirect68k)(func);                               \
		})

	#define LIBCALL_7(ret, func, callbase, dummy, r1, n1, r2, n2, r3, n3, r4, n4, r5, n5, r6, n6, r7, n7) \
		({                                                                                                \
			REG_##r1 = (ULONG)n1;                                                                         \
			REG_##r2 = (ULONG)n2;                                                                         \
			REG_##r3 = (ULONG)n3;                                                                         \
			REG_##r4 = (ULONG)n4;                                                                         \
			REG_##r5 = (ULONG)n5;                                                                         \
			REG_##r6 = (ULONG)n6;                                                                         \
			REG_##r7 = (ULONG)n7;                                                                         \
			REG_A6 = (ULONG)callbase;                                                                     \
			(ret) (*MyEmulHandle->EmulCallDirect68k)(func);                                               \
		})

	#define PATCH(offset, iface_dummy, func, type)   \
		{                                            \
			offset, {TRAP_LIB, 0, (APTR)&func}, type \
		}

	#define PATCHED_0(ret, name) \
		ret name(void)           \
		{
	#define PATCHED_1(ret, name, r1, t1, n1) \
		ret name(void)                       \
		{                                    \
			t1 n1 = (t1)REG_##r1;

	#define PATCHED_2(ret, name, r1, t1, n1, r2, t2, n2) \
		ret name(void)                                   \
		{                                                \
			t1 n1 = (t1)REG_##r1;                        \
			t2 n2 = (t2)REG_##r2;

	#define PATCHED_3(ret, name, r1, t1, n1, r2, t2, n2, r3, t3, n3) \
		ret name(void)                                               \
		{                                                            \
			t1 n1 = (t1)REG_##r1;                                    \
			t2 n2 = (t2)REG_##r2;                                    \
			t3 n3 = (t3)REG_##r3;

	#define PATCHED_5(ret, name, r1, t1, n1, r2, t2, n2, r3, t3, n3, r4, t4, n4, r5, t5, n5) \
		ret name(void)                                                                       \
		{                                                                                    \
			t1 n1 = (t1)REG_##r1;                                                            \
			t2 n2 = (t2)REG_##r2;                                                            \
			t3 n3 = (t3)REG_##r3;                                                            \
			t4 n4 = (t4)REG_##r4;                                                            \
			t5 n5 = (t5)REG_##r5;

	#define PATCHED_6(ret, name, r1, t1, n1, r2, t2, n2, r3, t3, n3, r4, t4, n4, r5, t5, n5, r6, t6, n6) \
		ret name(void)                                                                                   \
		{                                                                                                \
			t1 n1 = (t1)REG_##r1;                                                                        \
			t2 n2 = (t2)REG_##r2;                                                                        \
			t3 n3 = (t3)REG_##r3;                                                                        \
			t4 n4 = (t4)REG_##r4;                                                                        \
			t5 n5 = (t5)REG_##r5;                                                                        \
			t6 n6 = (t6)REG_##r6;

	#define PATCHED_7(ret, name, r1, t1, n1, r2, t2, n2, r3, t3, n3, r4, t4, n4, r5, t5, n5, r6, t6, n6, r7, t7, n7) \
		ret name(void)                                                                                               \
		{                                                                                                            \
			t1 n1 = (t1)REG_##r1;                                                                                    \
			t2 n2 = (t2)REG_##r2;                                                                                    \
			t3 n3 = (t3)REG_##r3;                                                                                    \
			t4 n4 = (t4)REG_##r4;                                                                                    \
			t5 n5 = (t5)REG_##r5;                                                                                    \
			t6 n6 = (t6)REG_##r6;                                                                                    \
			t7 n7 = (t7)REG_##r7;

#else

	#define LIBCALL_0(ret, func, callbase, dummy) ((ret ASM(*)(REG(a6, APTR)))func)((APTR)callbase)

	#define LIBCALL_1(ret, func, callbase, dummy, r1, n1) \
		((ret ASM(*)(REG(r1, IPTR), REG(a6, APTR)))func)((IPTR)n1, (APTR)callbase)

	#define LIBCALL_2(ret, func, callbase, dummy, r1, n1, r2, n2) \
		((ret ASM(*)(REG(r1, IPTR), REG(r2, IPTR), REG(a6, APTR)))func)((IPTR)n1, (IPTR)n2, (APTR)callbase)

	#define LIBCALL_3(ret, func, callbase, dummy, r1, n1, r2, n2, r3, n3)                  \
		((ret ASM(*)(REG(r1, IPTR), REG(r2, IPTR), REG(r3, IPTR), REG(a6, APTR)))func)(    \
			(IPTR)n1, (IPTR)n2, (IPTR)n3, (APTR)callbase)

	#define LIBCALL_5(ret, func, callbase, dummy, r1, n1, r2, n2, r3, n3, r4, n4, r5, n5)                         \
		((ret ASM(*)(                                                                                             \
			REG(r1, IPTR), REG(r2, IPTR), REG(r3, IPTR), REG(r4, IPTR), REG(r5, IPTR), REG(a6, APTR)))func)(      \
			(IPTR)n1, (IPTR)n2, (IPTR)n3, (IPTR)n4, (IPTR)n5, (APTR)callbase)

	#define LIBCALL_7(ret, func, callbase, dummy, r1, n1, r2, n2, r3, n3, r4, n4, r5, n5, r6, n6, r7, n7) \
		((ret ASM(*)(REG(r1, IPTR),                                                                       \
					 REG(r2, IPTR),                                                                       \
					 REG(r3, IPTR),                                                                       \
					 REG(r4, IPTR),                                                                       \
					 REG(r5, IPTR),                                                                       \
					 REG(r6, IPTR),                                                                       \
					 REG(r7, IPTR),                                                                       \
					 REG(a6, APTR)))func)(                                                                \
			(IPTR)n1, (IPTR)n2, (IPTR)n3, (IPTR)n4, (IPTR)n5, (IPTR)n6, (IPTR)n7, (APTR)callbase)

	#define PATCH(offset, iface_dummy, func, type) \
		{                                          \
			offset, func, type                     \
		}

	#define PATCHED_0(ret, name) \
		ret name(void)           \
		{
	#define PATCHED_1(ret, name, r1, t1, n1) \
		ret name(REG(r1, t1 n1))             \
		{
	#define PATCHED_2(ret, name, r1, t1, n1, r2, t2, n2) \
		ret name(REG(r1, t1 n1), REG(r2, t2 n2))         \
		{
	#define PATCHED_3(ret, name, r1, t1, n1, r2, t2, n2, r3, t3, n3) \
		ret name(REG(r1, t1 n1), REG(r2, t2 n2), REG(r3, t3 n3))     \
		{
	#define PATCHED_5(ret, name, r1, t1, n1, r2, t2, n2, r3, t3, n3, r4, t4, n4, r5, t5, n5)     \
		ret name(REG(r1, t1 n1), REG(r2, t2 n2), REG(r3, t3 n3), REG(r4, t4 n4), REG(r5, t5 n5)) \
		{
	#define PATCHED_6(ret, name, r1, t1, n1, r2, t2, n2, r3, t3, n3, r4, t4, n4, r5, t5, n5, r6, t6, n6)         \
		ret name(REG(r1, t1 n1), REG(r2, t2 n2), REG(r3, t3 n3), REG(r4, t4 n4), REG(r5, t5 n5), REG(r6, t6 n6)) \
		{
	#define PATCHED_7(ret, name, r1, t1, n1, r2, t2, n2, r3, t3, n3, r4, t4, n4, r5, t5, n5, r6, t6, n6, r7, t7, n7) \
		ret name(REG(r1, t1 n1),                                                                                     \
				 REG(r2, t2 n2),                                                                                     \
				 REG(r3, t3 n3),                                                                                     \
				 REG(r4, t4 n4),                                                                                     \
				 REG(r5, t5 n5),                                                                                     \
				 REG(r6, t6 n6),                                                                                     \
				 REG(r7, t7 n7))                                                                                     \
		{
#endif

#define PATCH_END }

#ifndef LIB_VECTSIZE
	// assume AmigaOS binary compatible
	#define LIB_VECTSIZE 6
#endif

typedef struct
{
	long offset;
#if defined(__MORPHOS__)
	struct EmulLibEntry trap;
#else
	APTR function;
#endif
	short type;
} PatchList;

enum {
	WB_PATCH_ADDAPPWINDOWA,
	WB_PATCH_REMAPPWINDOW,
	WB_PATCH_ADDAPPICONA,
	WB_PATCH_REMAPPICON,
	WB_PATCH_ADDAPPMENUA,
	WB_PATCH_REMAPPMENU,
	WB_PATCH_CLOSEWORKBENCH,
	WB_PATCH_OPENWORKBENCH,
#ifdef __amigaos4__
	WB_PATCH_OPENWORKBENCHOBJECT,
	WB_PATCH_OPENWORKBENCHOBJECTA,
	WB_PATCH_WORKBENCHCONTROL,
#endif
	WB_PATCH_WORKBENCHCONTROLA,
	WB_PATCH_PUTDISKOBJECT,
	WB_PATCH_DELETEDISKOBJECT,
	WB_PATCH_ADDPORT,
	WB_PATCH_CLOSEWINDOW,
	WB_PATCH_CREATEDIR,
	WB_PATCH_DELETEFILE,
	WB_PATCH_SETFILEDATE,
	WB_PATCH_SETCOMMENT,
	WB_PATCH_SETPROTECTION,
	WB_PATCH_RENAME,
	WB_PATCH_OPEN,
	WB_PATCH_CLOSE,
	WB_PATCH_WRITE,
	WB_PATCH_RELABEL,
	WB_PATCH_WBINFO,
	WB_PATCH_ADDTASK,
	WB_PATCH_REMTASK,
	WB_PATCH_FINDTASK,
	WB_PATCH_OPENWINDOWTAGS,
	//	WB_PATCH_ALLOCBITMAP,

	WB_PATCH_COUNT
};

enum {
	WB_PATCH_WORKBENCH,
	WB_PATCH_INTUITION,
	WB_PATCH_ICON,
	WB_PATCH_EXEC,
	WB_PATCH_DOS,
	WB_PATCH_GFX,
	WB_PATCH_DOSFUNC,
	WB_PATCH_DATATYPES,
};

AppEntry *
new_app_entry(ULONG type, IPTR id, IPTR userdata, APTR object, char *text, struct MsgPort *port, WB_Data *wb_data);
APTR rem_app_entry(AppEntry *entry, WB_Data *wb_data, BOOL *);
void free_app_entry(AppEntry *entry, WB_Data *);
AppEntry *find_app_entry(AppEntry *entry, WB_Data *wb_data);

#ifndef __amigaos3__
	#pragma pack(2)
#endif
typedef struct
{
	struct DiskObject doc_DiskObject;
	struct Image doc_GadgetRender;
	struct Image doc_SelectRender;
	APTR doc_Image1;
	APTR doc_Image2;
	APTR doc_Data;
} DiskObjectCopy;
#ifndef __amigaos3__
	#pragma pack()
#endif

void icon_notify(struct LibData *data, char *name, ULONG flags, short);
char *icon_fullname(struct LibData *data, char *name);

#define INF_FULLNAME (1 << 0)

#endif
