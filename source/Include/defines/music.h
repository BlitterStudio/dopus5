/* Automatically generated header! Do not edit! */

#ifndef _INLINE_MUSIC_H
#define _INLINE_MUSIC_H

#ifndef AROS_LIBCALL_H
	#include <aros/libcall.h>
#endif /* !AROS_LIBCALL_H */

#ifndef MUSIC_BASE_NAME
	#define MUSIC_BASE_NAME MUSICBase
#endif /* !MUSIC_BASE_NAME */

#define ContModule() AROS_LC0NR(void,  ContModule, struct Library *, MUSIC_BASE_NAME, 9, /* s */)

#define FlushModule() AROS_LC0NR(void,  FlushModule, struct Library *, MUSIC_BASE_NAME, 8, /* s */)

#define IsModule(___name) \
	AROS_LC1(WORD, IsModule, AROS_LCA(char *, (___name), A0), struct Library *, MUSIC_BASE_NAME, 7, /* s */)

#define PlayFaster() AROS_LC0NR(void,  PlayFaster, struct Library *, MUSIC_BASE_NAME, 11, /* s */)

#define PlayModule(___name, ___foob)          \
	AROS_LC2(WORD,                            \
			 PlayModule,                      \
			 AROS_LCA(char *, (___name), A0), \
			 AROS_LCA(BOOL, (___foob), D0),   \
			 struct Library *,                \
			 MUSIC_BASE_NAME,                 \
			 5,                               \
			 /* s */)

#define PlaySlower() AROS_LC0NR(void,  PlaySlower, struct Library *, MUSIC_BASE_NAME, 12, /* s */)

#define SetVolume(___volume) \
	AROS_LC1NR(void,  SetVolume, AROS_LCA(WORD, (___volume), D0), struct Library *, MUSIC_BASE_NAME, 10, /* s */)

#define StopModule() AROS_LC0NR(void,  StopModule, struct Library *, MUSIC_BASE_NAME, 6, /* s */)

#define TempoReset() AROS_LC0NR(void,  TempoReset, struct Library *, MUSIC_BASE_NAME, 13, /* s */)

#endif /* !_INLINE_MUSIC_H */
