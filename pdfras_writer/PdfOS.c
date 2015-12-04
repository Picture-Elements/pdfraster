#include "PdfOS.h"

pdint32 pdstrlen(const char *s)
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
static char itoabuf[kMAX_DIGITS + 2]; /* null and sign */
extern char *pditoa(pdint32 i)
{
	int neg = i < 0;
	char *s = itoabuf + kMAX_DIGITS + 1;
	*s = '\0';
	if (neg) i = -i;
	if (i < 0) {
		// there is one value that can't be negated...
		*--s = '8';
		i = 214748364;
	}
	do
	{
		*--s = (i % 10) + '0';
		i /= 10;
	} while (i > 0);
	if (neg)
		*--s = '-';
	return s;
}

char * pdftoa(pddouble n) {
	return pdftoaprecision(n, 0.00000000000001);
}

static char __fbuf[1076];

char * pdftoaprecision(pddouble n, pddouble precision) {
	/* handle special cases */
	if (pdisnan(n)) {
		__fbuf[0] = __fbuf[2] = 'n';
		__fbuf[1] = 'a';
		__fbuf[3] = '\0';
		/* strcpy(__fbuf, "nan"); */
	}
	else if (pdisinf(n)) {
		__fbuf[0] = 'i';
		__fbuf[1] = 'n';
		__fbuf[2] = 'f';
		__fbuf[3] = '\0';
		/* strcpy(__fbuf, "inf"); */
	}
	else if (n == 0.0) {
		__fbuf[0] = '0';
		__fbuf[1] = '\0';
	}
	else {
		char intPart_reversed[311];
		int i = 0, charCount = 0;
		double fp_int, fp_frac;

		if (n < 0) {
			__fbuf[charCount++] = '-';
			n = -n;
		}

		fp_frac = modf(n, &fp_int); // Separate integer/fractional parts

		while (fp_int > 0) // Convert integer part, if any
		{
			intPart_reversed[i++] = (char)('0' + (int)fmod(fp_int, 10));
			fp_int = floor(fp_int / 10);
		}

		for (i = 0; i < charCount; i++) {
			//Reverse the integer part, if any
			__fbuf[charCount++] = intPart_reversed[--i];
		}

		if (fp_int != 0) {
			__fbuf[charCount++] = '.'; //Decimal point

			while (fp_frac > 0 && precision < 1.0) // Convert fractional part, if any
			{
				fp_frac *= 10;
				fp_frac = modf(fp_frac, &fp_int);
				__fbuf[charCount++] = (char)('0' + (int)fp_int);
				precision *= 10;
			}
		}

		__fbuf[charCount] = 0; //String terminator
	}
	return __fbuf;
}
