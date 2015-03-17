#include "PdfStrings.h"
#include <string.h>

char *pd_strcpy(char *dst, size_t destlen, char *src)
{
	char *retval = dst;
	if (!dst) return 0;
	if (destlen == 0) return src;
	if (!src)
	{
		*dst = 0; return dst;
	}

	while (1)
	{
		if (!(*dst++ = *src++))
			break;
		if (--destlen == 0)
		{
			*dst = 0;
			break;
		}
	}
	return retval;
}


char * pd_strdup(t_pdallocsys *alloc, char *str)
{
	int len;
	if (!str) return 0;
	len = pdstrlen(str) + 1;
	char *dup = pd_alloc(alloc, len);
	if (!dup) return dup;
	pd_strcpy(dup, len, str);
	return dup;
}

pdint32 pd_strcmp(char *s1, char *s2)
{
	return strcmp(s1, s2);
}
