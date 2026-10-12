/* Replacement functions */

#include <dopus/common.h>

#if !defined(__MORPHOS__) && !defined(__AROS__)
int stccpy(char *p, const char *q, int n)
{
	char *t = p;

	while ((*p++ = *q++) && --n > 0)
	{
	};

	p[-1] = '\0';

	return p - t;
}

char *stpblk(const char *p)
{
	while (isspace((unsigned char)*p))
		++p;

	return (char *)p;
}
#endif

char *stptok(const char *s, char *tok, size_t toklen, char *brk)
{
	char *lim, *b;

	if (!*s)
		return NULL;

	lim = tok + toklen - 1;
	while (*s && tok < lim)
	{
		for (b = brk; *b; b++)
		{
			if (*s == *b)
			{
				*tok = 0;
				return (char *)s;
			}
		}
		*tok++ = *s++;
	}
	*tok = 0;
	return (char *)s;
}
