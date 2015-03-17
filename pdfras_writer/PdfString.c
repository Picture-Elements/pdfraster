#include "PdfString.h"

typedef struct t_pdstring {
	t_pdallocsys *alloc;
	pdbool isBinary;
	pduint32 length;
	pduint8 *strData;
} t_pdstring;

t_pdstring *pd_string_new(t_pdallocsys *alloc, char *string, pduint32 len, pdbool isbinary)
{
	t_pdstring *str;
	if (!alloc || !string) return 0;
	str = (t_pdstring *)pd_alloc(alloc, sizeof(t_pdstring));
	if (!str) return 0;
	str->alloc = alloc;
	str->strData = (pduint8 *)pd_alloc(alloc, len);
	if (!str->strData)
	{
		pd_free(alloc, str);
		return 0;
	}
	str->length = len;
	pd_string_set(str, string, len, isbinary);
	return str;
}

void pd_string_free(t_pdstring *str)
{
	if (!str) return;
	if (str->strData)
	{
		pd_free(str->alloc, str->strData);
	}
	pd_free(str->alloc, str);
}

pduint32 pd_string_length(t_pdstring *str)
{
	if (!str) return 0;
	return str->length;
}

void pd_string_set(t_pdstring *str, char *string, pduint32 len, pdbool isbinary)
{
	pduint32 i;

	if (!str || !string) return;
	if (len != str->length)
	{
		pd_free(str->alloc, str->strData);
		str->strData = (pduint8 *)pd_alloc(str->alloc, len);
		str->length = len;
	}

	for (i = 0; i < len; i++)
	{
		str->strData[i] = string[i];
	}
}

pdbool pd_string_is_binary(t_pdstring *str)
{
	if (!str) return PD_FALSE;
	return str->isBinary;
}

pduint8 pdstring_char_at(t_pdstring *str, pduint32 index)
{
	if (!str || index >= str->length) return (pduint8)0;
	return str->strData[index];
}

void pd_string_foreach(t_pdstring *str, f_pdstring_foreach iter, void *cookie)
{
	pduint32 i;
	if (!str || !iter) return;
	for (i = 0; i < str->length; i++)
	{
		if (!iter(i, str->strData[i], cookie))
			break;
	}
}
