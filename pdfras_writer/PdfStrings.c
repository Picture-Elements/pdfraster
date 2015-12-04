#include "PdfStrings.h"
#include <string.h>

void pd_strcpy(char *dst, size_t destlen, const char *src)
{
	if (dst && destlen != 0) {
		if (src) {
			while (*src && destlen > 1)
			{
				*dst++ = *src++;
				--destlen;
			}
		}
		*dst = 0;
	}
}


char * pd_strdup(t_pdmempool *alloc, const char *str)
{
	char* dup = 0;
	if (str) {
		int len = pdstrlen(str) + 1;
		dup = pd_alloc(alloc, len);
		if (dup) {
			pd_strcpy(dup, len, str);
		}
	}
	return dup;
}

pdint32 pd_strcmp(const char *s1, const char *s2)
{
	return strcmp(s1, s2);
}
