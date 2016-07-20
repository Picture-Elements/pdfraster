#include "PdfOS.h"

extern pdint32 pdstrlen(const char *s)
{
	pdint32 i = 0;
	if (!s) return 0;
	while (*s++)
	{
		i++;
	}
	return i;
}

#define kMAX_DIGITS 19 /* 64 bit */

extern char *pditoa(pdint32 i, char* dst)
{
	if (dst) {
		char *s = dst;
		char *p = dst;
		int neg = i < 0;
		if (neg) i = -i;
		if (i < 0) {
			// there is one value that can't be negated...
			*s++ = '8';
			i = 214748364;
		}
		do
		{
			*s++ = (i % 10) + '0';
			i /= 10;
		} while (i > 0);
		if (neg) {
			*s++ = '-';
		}
		*s = '\0';
		while (p < --s) {
			char t = *s;
			*s = *p;
			*p++ = t;
		}
	}
	return dst;
}
