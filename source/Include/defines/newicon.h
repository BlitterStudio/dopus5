/* Automatically generated header! Do not edit! */

#ifndef _INLINE_NEWICON_H
#define _INLINE_NEWICON_H

#ifndef AROS_LIBCALL_H
	#include <aros/libcall.h>
#endif /* !AROS_LIBCALL_H */

#ifndef NEWICON_BASE_NAME
	#define NEWICON_BASE_NAME NewIconBase
#endif /* !NEWICON_BASE_NAME */

#define FreeNewDiskObject(___newdiskobj)                            \
	AROS_LC1NR(void,                                                   \
			 FreeNewDiskObject,                                     \
			 AROS_LCA(struct NewDiskObject *, (___newdiskobj), A0), \
			 struct Library *,                                      \
			 NEWICON_BASE_NAME,                                     \
			 7,                                                     \
			 /* s */)

#define FreeRemappedImage(___image, ___screen)           \
	AROS_LC2NR(void,                                        \
			 FreeRemappedImage,                          \
			 AROS_LCA(struct Image *, (___image), A0),   \
			 AROS_LCA(struct Screen *, (___screen), A1), \
			 struct Library *,                           \
			 NEWICON_BASE_NAME,                          \
			 12,                                         \
			 /* s */)

#define GetDefNewDiskObject(___def_type)        \
	AROS_LC1(struct NewDiskObject *,            \
			 GetDefNewDiskObject,               \
			 AROS_LCA(LONG, (___def_type), D0), \
			 struct Library *,                  \
			 NEWICON_BASE_NAME,                 \
			 13,                                \
			 /* s */)

#define GetNewDiskObject(___name)              \
	AROS_LC1(struct NewDiskObject *,           \
			 GetNewDiskObject,                 \
			 AROS_LCA(UBYTE *, (___name), A0), \
			 struct Library *,                 \
			 NEWICON_BASE_NAME,                \
			 5,                                \
			 /* s */)

#define PutNewDiskObject(___name, ___newdiskobj)                    \
	AROS_LC2(BOOL,                                                  \
			 PutNewDiskObject,                                      \
			 AROS_LCA(UBYTE *, (___name), A0),                      \
			 AROS_LCA(struct NewDiskObject *, (___newdiskobj), A1), \
			 struct Library *,                                      \
			 NEWICON_BASE_NAME,                                     \
			 6,                                                     \
			 /* s */)

#define RemapChunkyImage(___chunkyimage, ___screen)                \
	AROS_LC2(struct Image *,                                       \
			 RemapChunkyImage,                                     \
			 AROS_LCA(struct ChunkyImage *, (___chunkyimage), A0), \
			 AROS_LCA(struct Screen *, (___screen), A1),           \
			 struct Library *,                                     \
			 NEWICON_BASE_NAME,                                    \
			 11,                                                   \
			 /* s */)

#endif /* !_INLINE_NEWICON_H */
