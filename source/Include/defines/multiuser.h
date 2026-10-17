/* Automatically generated header! Do not edit! */

#ifndef _INLINE_MULTIUSER_H
#define _INLINE_MULTIUSER_H

#ifndef AROS_LIBCALL_H
	#include <aros/libcall.h>
#endif /* !AROS_LIBCALL_H */

#include <aros/preprocessor/variadic/cast2iptr.hpp>

#ifndef MULTIUSER_BASE_NAME
	#define MULTIUSER_BASE_NAME muBase
#endif /* !MULTIUSER_BASE_NAME */

#define muAddMonitor(___monitor)                                                                                       \
	AROS_LC1(                                                                                                          \
		BOOL, muAddMonitor, AROS_LCA(struct muMonitor *, (___monitor), A0), struct Library *, MULTIUSER_BASE_NAME, 28, \
		/* s */)

#define muAllocGroupInfo() \
	AROS_LC0(struct muGroupInfo *, muAllocGroupInfo, struct Library *, MULTIUSER_BASE_NAME, 25, /* s */)

#define muAllocUserInfo() \
	AROS_LC0(struct muUserInfo *, muAllocUserInfo, struct Library *, MULTIUSER_BASE_NAME, 9, /* s */)

#define muCheckPasswd(___taglist)                                                                                     \
	AROS_LC1(                                                                                                         \
		BOOL, muCheckPasswd, AROS_LCA(struct TagItem *, (___taglist), A0), struct Library *, MULTIUSER_BASE_NAME, 17, \
		/* s */)

#ifndef NO_INLINE_STDARG
	#define muCheckPasswdTags(...)                                    \
		({                                                            \
			IPTR _tags[] = {AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__)}; \
			muCheckPasswd((struct TagItem *)_tags);                   \
		})
#endif /* !NO_INLINE_STDARG */

#define muFreeExtOwner(___info)                                                                                        \
	AROS_LC1NR(void,                                                                                                           \
		 muFreeExtOwner, AROS_LCA(struct muExtOwner *, (___info), A0), struct Library *, MULTIUSER_BASE_NAME, 22, \
		/* s */)

#define muFreeGroupInfo(___info)                            \
	AROS_LC1NR(void,                                           \
			 muFreeGroupInfo,                               \
			 AROS_LCA(struct muGroupInfo *, (___info), A0), \
			 struct Library *,                              \
			 MULTIUSER_BASE_NAME,                           \
			 26,                                            \
			 /* s */)

#define muFreeUserInfo(___info)                                                                                        \
	AROS_LC1NR(void,                                                                                                           \
		 muFreeUserInfo, AROS_LCA(struct muUserInfo *, (___info), A0), struct Library *, MULTIUSER_BASE_NAME, 10, \
		/* s */)

#define muFreeze(___task)                                                                                            \
	AROS_LC1(BOOL, muFreeze, AROS_LCA(struct Task *, (___task), D0), struct Library *, MULTIUSER_BASE_NAME, 31, /* s \
																												 */)

#define muGetConfigDirLock() AROS_LC0(BPTR, muGetConfigDirLock, struct Library *, MULTIUSER_BASE_NAME, 20, /* s */)

#define muGetDefProtection(___task)                                                                                   \
	AROS_LC1(                                                                                                         \
		ULONG, muGetDefProtection, AROS_LCA(struct Task *, (___task), D0), struct Library *, MULTIUSER_BASE_NAME, 14, \
		/* s */)

#define muGetGroupInfo(___info, ___keytype)                 \
	AROS_LC2(struct muGroupInfo *,                          \
			 muGetGroupInfo,                                \
			 AROS_LCA(struct muGroupInfo *, (___info), A0), \
			 AROS_LCA(ULONG, (___keytype), D0),             \
			 struct Library *,                              \
			 MULTIUSER_BASE_NAME,                           \
			 27,                                            \
			 /* s */)

#define muGetPasswdDirLock() AROS_LC0(BPTR, muGetPasswdDirLock, struct Library *, MULTIUSER_BASE_NAME, 19, /* s */)

#define muGetRelationshipA(___user, ___owner, ___taglist)  \
	AROS_LC3(ULONG,                                        \
			 muGetRelationshipA,                           \
			 AROS_LCA(struct muExtOwner *, (___user), D0), \
			 AROS_LCA(ULONG, (___owner), D1),              \
			 AROS_LCA(struct TagItem *, (___taglist), A0), \
			 struct Library *,                             \
			 MULTIUSER_BASE_NAME,                          \
			 23,                                           \
			 /* s */)

#ifndef NO_INLINE_STDARG
	#define muGetRelationship(a0, a1, ...)                            \
		({                                                            \
			IPTR _tags[] = {AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__)}; \
			muGetRelationshipA((a0), (a1), (struct TagItem *)_tags);  \
		})
#endif /* !NO_INLINE_STDARG */

#define muGetTaskExtOwner(___task)                   \
	AROS_LC1(struct muExtOwner *,                    \
			 muGetTaskExtOwner,                      \
			 AROS_LCA(struct Task *, (___task), D0), \
			 struct Library *,                       \
			 MULTIUSER_BASE_NAME,                    \
			 21,                                     \
			 /* s */)

#define muGetTaskOwner(___task)                                                                                       \
	AROS_LC1(ULONG, muGetTaskOwner, AROS_LCA(struct Task *, (___task), D0), struct Library *, MULTIUSER_BASE_NAME, 7, \
			 /* s */)

#define muGetUserInfo(___info, ___keytype)                 \
	AROS_LC2(struct muUserInfo *,                          \
			 muGetUserInfo,                                \
			 AROS_LCA(struct muUserInfo *, (___info), A0), \
			 AROS_LCA(ULONG, (___keytype), D0),            \
			 struct Library *,                             \
			 MULTIUSER_BASE_NAME,                          \
			 11,                                           \
			 /* s */)

#define muKill(___task) \
	AROS_LC1(BOOL, muKill, AROS_LCA(struct Task *, (___task), D0), struct Library *, MULTIUSER_BASE_NAME, 30, /* s */)

#define muLimitDOSSetProtection(___flag)                                                                              \
	AROS_LC1(BOOL, muLimitDOSSetProtection, AROS_LCA(BOOL, (___flag), D0), struct Library *, MULTIUSER_BASE_NAME, 16, \
			 /* s */)

#define muLoginA(___taglist)                                                                                          \
	AROS_LC1(ULONG, muLoginA, AROS_LCA(struct TagItem *, (___taglist), A0), struct Library *, MULTIUSER_BASE_NAME, 6, \
			 /* s */)

#ifndef NO_INLINE_STDARG
	#define muLogin(...)                                              \
		({                                                            \
			IPTR _tags[] = {AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__)}; \
			muLoginA((struct TagItem *)_tags);                        \
		})
#endif /* !NO_INLINE_STDARG */

#define muLogoutA(___taglist)                                                                                          \
	AROS_LC1(ULONG, muLogoutA, AROS_LCA(struct TagItem *, (___taglist), A0), struct Library *, MULTIUSER_BASE_NAME, 5, \
			 /* s */)

#ifndef NO_INLINE_STDARG
	#define muLogout(...)                                             \
		({                                                            \
			IPTR _tags[] = {AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__)}; \
			muLogoutA((struct TagItem *)_tags);                       \
		})
#endif /* !NO_INLINE_STDARG */

#define muPasswd(___oldpwd, ___newpwd)          \
	AROS_LC2(BOOL,                              \
			 muPasswd,                          \
			 AROS_LCA(STRPTR, (___oldpwd), A0), \
			 AROS_LCA(STRPTR, (___newpwd), A1), \
			 struct Library *,                  \
			 MULTIUSER_BASE_NAME,               \
			 8,                                 \
			 /* s */)

#define muRemMonitor(___monitor)                                                                                       \
	AROS_LC1NR(void,                                                                                                           \
		 muRemMonitor, AROS_LCA(struct muMonitor *, (___monitor), A0), struct Library *, MULTIUSER_BASE_NAME, 29, \
		/* s */)

#define muSetDefProtectionA(___taglist)                    \
	AROS_LC1(BOOL,                                         \
			 muSetDefProtectionA,                          \
			 AROS_LCA(struct TagItem *, (___taglist), A0), \
			 struct Library *,                             \
			 MULTIUSER_BASE_NAME,                          \
			 13,                                           \
			 /* s */)

#ifndef NO_INLINE_STDARG
	#define muSetDefProtection(...)                                   \
		({                                                            \
			IPTR _tags[] = {AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__)}; \
			muSetDefProtectionA((struct TagItem *)_tags);             \
		})
#endif /* !NO_INLINE_STDARG */

#define muSetProtection(___name, ___mask)     \
	AROS_LC2(BOOL,                            \
			 muSetProtection,                 \
			 AROS_LCA(STRPTR, (___name), D1), \
			 AROS_LCA(LONG, (___mask), D2),   \
			 struct Library *,                \
			 MULTIUSER_BASE_NAME,             \
			 15,                              \
			 /* s */)

#define muUnfreeze(___task)                                                                                       \
	AROS_LC1(BOOL, muUnfreeze, AROS_LCA(struct Task *, (___task), D0), struct Library *, MULTIUSER_BASE_NAME, 32, \
			 /* s */)

#define muUserInfo2ExtOwner(___info)                       \
	AROS_LC1(struct muExtOwner *,                          \
			 muUserInfo2ExtOwner,                          \
			 AROS_LCA(struct muUserInfo *, (___info), A0), \
			 struct Library *,                             \
			 MULTIUSER_BASE_NAME,                          \
			 24,                                           \
			 /* s */)

#endif /* !_INLINE_MULTIUSER_H */
