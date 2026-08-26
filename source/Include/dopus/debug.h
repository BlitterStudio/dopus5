#ifndef DOPUS_DEBUG_H
#define DOPUS_DEBUG_H

#if defined(__amigaos3__) || defined(__MORPHOS__)
	#include <clib/debug_protos.h>
#endif
#if defined(__AROS__)
	#include <aros/debug.h>
	#undef bug
	#undef D
#endif /* __AROS__ */

#if defined(DEBUG) && DEBUG
	#define D(x) x
#else
	#define D(x)
#endif

#if defined(__amigaos4__)
	#define bug(fmt, args...)                                              \
		{                                                                  \
			DebugPrintF("[%s:%ld %s] ", __FILE__, (long)__LINE__, __FUNCTION__); \
			DebugPrintF(fmt, ##args);                                      \
		}
#elif defined(__amigaos3__)
	#define bug(fmt, args...)                                          \
		{                                                              \
			kprintf("[%s:%s:%ld] ", __FILE__, __FUNCTION__, (long)__LINE__); \
			kprintf(fmt, ##args);                                      \
		}
#elif defined(__MORPHOS__) || defined(__AROS__)
	#define bug(fmt, args...)                                          \
		{                                                              \
			kprintf("[%s:%ld %s] ", __FILE__, (long)__LINE__, __FUNCTION__); \
			kprintf(fmt, ##args);                                      \
		}
#endif

#endif /* DOPUS_DEBUG_H */
