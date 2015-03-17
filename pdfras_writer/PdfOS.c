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
	do
	{
		*--s = (i % 10) + '0';
		i /= 10;
	} while (i > 0);
	if (neg)
		*--s = '-';
	return s;
}

//#define PRECISION 0.00000000000001
#define MAX_FLOAT_STRING_SIZE 80

/* see http://stackoverflow.com/a/7097567/20481 */
/**
* Double to ASCII
*/
static char __fbuf[MAX_FLOAT_STRING_SIZE];

char * pdftoa(pddouble n) {
	return pdftoaprecision(n, 0.00000000000001);
}


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
		int digit, m, m1;
		char *c = __fbuf;
		int neg = (n < 0);
		if (neg)
			n = -n;
		// calculate magnitude
		m = (int)log10(n);
		int useExp = (m >= 14 || (neg && m >= 9) || m <= -9);
		if (neg)
			*(c++) = '-';
		// set up for scientific notation
		if (useExp) {
			if (m < 0)
				m -= 1;
			n = (pdfloat32)(n / pow(10.0, m));
			m1 = m;
			m = 0;
		}
		if (m < 1.0) {
			m = 0;
		}
		// convert the number
		while (n > precision || m >= 0) {
			double weight = pow(10.0, m);
			if (weight > 0 && !pdisinf(weight)) {
				digit = (int)floor(n / weight);
				n -= (pdfloat32)(digit * weight);
				*(c++) = '0' + digit;
			}
			if (m == 0 && n > 0)
				*(c++) = '.';
			m--;
		}
		if (useExp) {
			// convert the exponent
			int i, j;
			*(c++) = 'e';
			if (m1 > 0) {
				*(c++) = '+';
			}
			else {
				*(c++) = '-';
				m1 = -m1;
			}
			m = 0;
			while (m1 > 0) {
				*(c++) = '0' + m1 % 10;
				m1 /= 10;
				m++;
			}
			c -= m;
			for (i = 0, j = m - 1; i<j; i++, j--) {
				// swap without temporary
				c[i] ^= c[j];
				c[j] ^= c[i];
				c[i] ^= c[j];
			}
			c += m;
		}
		*(c) = '\0';
	}
	return __fbuf;
}

char hexdigit(int c)
{
	static char *digits = "0123456789ABCDEF";
	return digits[c & 0xf];
}