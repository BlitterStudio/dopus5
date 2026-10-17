/* Automatically generated header! Do not edit! */

#ifndef _INLINE_SYSINFO_H
#define _INLINE_SYSINFO_H

#ifndef AROS_LIBCALL_H
	#include <aros/libcall.h>
#endif /* !AROS_LIBCALL_H */

#ifndef SYSINFO_BASE_NAME
	#define SYSINFO_BASE_NAME SysInfoBase
#endif /* !SYSINFO_BASE_NAME */

#define AddNotify(___si, ___flags, ___safety_limit)   \
	AROS_LC3(struct SI_Notify *,                      \
			 AddNotify,                               \
			 AROS_LCA(struct SysInfo *, (___si), A0), \
			 AROS_LCA(WORD, (___flags), D0),          \
			 AROS_LCA(LONG, (___safety_limit), D1),   \
			 struct Library *,                        \
			 SYSINFO_BASE_NAME,                       \
			 13,                                      \
			 /* s */)

#define FreeSysInfo(___si)                                                                                       \
	AROS_LC1NR(void,  FreeSysInfo, AROS_LCA(struct SysInfo *, (___si), A0), struct Library *, SYSINFO_BASE_NAME, 6, \
			 /* s */)

#define GetCpuUsage(___si, ___usage)                         \
	AROS_LC2NR(void,                                            \
			 GetCpuUsage,                                    \
			 AROS_LCA(struct SysInfo *, (___si), A0),        \
			 AROS_LCA(struct SI_CpuUsage *, (___usage), A1), \
			 struct Library *,                               \
			 SYSINFO_BASE_NAME,                              \
			 15,                                             \
			 /* s */)

#define GetLoadAverage(___si, ___la)                         \
	AROS_LC2NR(void,                                            \
			 GetLoadAverage,                                 \
			 AROS_LCA(struct SysInfo *, (___si), A0),        \
			 AROS_LCA(struct SI_LoadAverage *, (___la), A1), \
			 struct Library *,                               \
			 SYSINFO_BASE_NAME,                              \
			 7,                                              \
			 /* s */)

#define GetNice(___si, ___which, ___who)              \
	AROS_LC3(LONG,                                    \
			 GetNice,                                 \
			 AROS_LCA(struct SysInfo *, (___si), A0), \
			 AROS_LCA(LONG, (___which), D0),          \
			 AROS_LCA(LONG, (___who), D1),            \
			 struct Library *,                        \
			 SYSINFO_BASE_NAME,                       \
			 11,                                      \
			 /* s */)

#define GetPgrp(___si) \
	AROS_LC1(LONG, GetPgrp, AROS_LCA(struct SysInfo *, (___si), A0), struct Library *, SYSINFO_BASE_NAME, 10, /* s */)

#define GetPid(___si) \
	AROS_LC1(LONG, GetPid, AROS_LCA(struct SysInfo *, (___si), A0), struct Library *, SYSINFO_BASE_NAME, 8, /* s */)

#define GetPpid(___si) \
	AROS_LC1(LONG, GetPpid, AROS_LCA(struct SysInfo *, (___si), A0), struct Library *, SYSINFO_BASE_NAME, 9, /* s */)

#define GetTaskCpuUsage(___si, ___usage, ___task)                \
	AROS_LC3(LONG,                                               \
			 GetTaskCpuUsage,                                    \
			 AROS_LCA(struct SysInfo *, (___si), A0),            \
			 AROS_LCA(struct SI_TaskCpuUsage *, (___usage), A1), \
			 AROS_LCA(struct Task *, (___task), A2),             \
			 struct Library *,                                   \
			 SYSINFO_BASE_NAME,                                  \
			 16,                                                 \
			 /* s */)

#define InitSysInfo() AROS_LC0(struct SysInfo *, InitSysInfo, struct Library *, SYSINFO_BASE_NAME, 5, /* s */)

#define RemoveNotify(___si, ___notify)                      \
	AROS_LC2NR(void,                                           \
			 RemoveNotify,                                  \
			 AROS_LCA(struct SysInfo *, (___si), A0),       \
			 AROS_LCA(struct SI_Notify *, (___notify), A1), \
			 struct Library *,                              \
			 SYSINFO_BASE_NAME,                             \
			 14,                                            \
			 /* s */)

#define SetNice(___si, ___which, ___who, ___nice)     \
	AROS_LC4(LONG,                                    \
			 SetNice,                                 \
			 AROS_LCA(struct SysInfo *, (___si), A0), \
			 AROS_LCA(LONG, (___which), D0),          \
			 AROS_LCA(LONG, (___who), D1),            \
			 AROS_LCA(LONG, (___nice), D2),           \
			 struct Library *,                        \
			 SYSINFO_BASE_NAME,                       \
			 12,                                      \
			 /* s */)

#endif /* !_INLINE_SYSINFO_H */
