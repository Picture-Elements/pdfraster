#include "PdfString.h"
#include "PdfStrings.h"

typedef struct t_pdstring {
	pdbool isBinary;
	pduint32 length;
	pduint8 *strData;
} t_pdstring;

t_pdstring *pd_string_new(t_pdallocsys *pool, const char *string, pduint32 len, pdbool isbinary)
{
	t_pdstring *str = NULL;
	if (pool) {
		str = (t_pdstring *)pd_alloc(pool, sizeof(t_pdstring));
		if (str) {
			str->strData = (pduint8 *)pd_alloc(pool, len);
			if (str->strData)
			{
				str->length = len;
				if (string) {
					// attach data to string and set the isBinary field:
					pd_string_set(str, string, len, isbinary);
				}
			}
			else {
				pd_free(str);
				str = NULL;
			}
		}
	}
	return str;
}

void pd_string_free(t_pdstring *str)
{
	if (!str) return;
	if (str->strData)
	{
		pd_free(str->strData);
	}
	pd_free(str);
}

pduint32 pd_string_length(t_pdstring *str)
{
	if (!str) return 0;
	return str->length;
}

pduint8* pd_string_data(t_pdstring *str)
{
	if (!str) return 0;
	return str->strData;
}

pdbool pd_string_set_length(t_pdstring *str, pduint32 len)
{
	if (!str) {
		return PD_FALSE;
	}
	if (len != str->length)
	{
		// allocate the new data block in same pool as string header
		pduint8 *newData = (pduint8 *)pd_alloc_same_pool(str, len);
		if (!newData) {
			return PD_FALSE;
		}
		// free the old data block if any:
		pd_free(str->strData);
		// and swap in the new
		str->strData = newData;
		str->length = len;
	}
	return PD_TRUE;
}

void pd_string_set(t_pdstring *str, const char *string, pduint32 len, pdbool isbinary)
{
	if (str && pd_string_set_length(str, len)) {
		str->isBinary = isbinary;
		if (string) {
			pduint32 i;
			for (i = 0; i < len; i++)
			{
				str->strData[i] = string[i];
			}
		}
	}
}

pdbool pd_string_is_binary(t_pdstring *str)
{
	if (!str) return PD_FALSE;
	return str->isBinary;
}

pdbool pd_string_equal(t_pdstring *s1, t_pdstring *s2)
{
	return pd_string_compare(s1, s2) == 0 ? PD_TRUE : PD_FALSE;
}

int pd_string_compare(t_pdstring *s1, t_pdstring *s2)
{
	if (s1 && s2) {
		if (s1 == s2) {
			return 0;		// identical => equal
		}
		pduint32 i, len = s1->length;
		const pduint8* p1 = s1->strData;
		const pduint8* p2 = s2->strData;
		if (len != s2->length) {
			// shorter string is 'less'
			return (len < s2->length) ? -1 : 1;
		}
		for (i = 0; i < len; i++) {
			if (p1[i] != p2[i]) {
				return (p1[i] < p2[i]) ? -1 : 1;
			}
		}
		return 0;			// equal
	}
	else {
		return pd_strcmp(NULL, NULL);
	}
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
