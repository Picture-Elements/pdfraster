// pdfras_writer_tests.cpp : automatic tests for pdfras_writer library
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "PdfStreaming.h"
#include "PdfStrings.h"
#include "PdfString.h"
#include "PdfAtoms.h"
#include "PdfContentsGenerator.h"
#include "PdfHash.h"
#include "PdfArray.h"
#include "PdfDict.h"
#include "PdfXrefTable.h"
#include "PdfStandardAtoms.h"
#include "PdfStandardObjects.h"

///////////////////////////////////////////////////////////////////////
// Constants

///////////////////////////////////////////////////////////////////////
// Macros

_CRTIMP void __cdecl _wassert(_In_z_ const wchar_t * _Message, _In_z_ const wchar_t *_File, _In_ unsigned _Line);

#define assert(_Expression) (void)( (!!(_Expression)) || (_wassert(_CRT_WIDE(#_Expression), _CRT_WIDE(__FILE__), __LINE__), 0) )

///////////////////////////////////////////////////////////////////////
// Types

typedef struct {
	size_t		bufsize;
	pduint8*	buffer;
	unsigned	pos;
} membuf;

///////////////////////////////////////////////////////////////////////
// Globals

// platform functions
t_OS os;
// memory buffer to capture output streams
char output[1024];
membuf buffer = {
	.buffer = (pduint8*)output,
	.bufsize = sizeof output,
	.pos = 0
};

///////////////////////////////////////////////////////////////////////
// Helpers and handlers

static void myMemSet(void *ptr, pduint8 value, size_t count)
{
	memset(ptr, value, count);
}

static int myOutputWriter(const pduint8 * data, pduint32 offset, pduint32 len, void *cookie)
{
	membuf* buff = (membuf *)cookie;
	if (!data) {
		len = 0;
	}
	if (len) {
		data += offset;
		memcpy(buff->buffer + buff->pos, data, len);
		buff->pos += len;
	}
	buff->buffer[buff->pos] = 0;
	return len;
}

static void *mymalloc(size_t bytes)
{
	return malloc(bytes);
}

///////////////////////////////////////////////////////////////////////
// Tests

void test_alloc()
{
	// Number of allocations to do as allocation test
#define ALLOCS 10000

	printf("allocation pool\n");
	// memory allocation
	t_pdmempool* pool = os.allocsys;
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);

	// check that this works on an empty pool:
	pd_pool_clean(pool);
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);

	// check this doesn't crash:
	pd_free(NULL);
	pd_pool_clean(NULL);

	// allocating from a null pool returns NULL
	assert(pd_alloc(NULL, 256) == NULL);

	assert(pd_get_block_size(NULL) == 0);

	// create a second pool
	t_pdmempool* pool2 = pd_alloc_new_pool(&os);
	// check that it looks OK
	assert(pool2);
	assert(pool2 != pool);
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);
	// allocate a block from pool2:
	void *block21 = pd_alloc(pool2, 65536);
	// check that it looks OK
	assert(block21);
	assert(pd_get_block_size(block21) == 65536);
	// and pools change state to match:
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_block_count(pool2) == 1);
	assert(pd_get_bytes_in_use(pool2) == 65536);
	memset(block21, 0x77, 65536);

	// Allocate a bunch of blocks (from the first pool)
	size_t bytes = 0;
	void* ptr[ALLOCS];
	int i;
	for (i = 0; i < ALLOCS; i++) {
		size_t nb = i;
		// note that 0 is a 100% valid size for a block - it just
		// has overhead but no data area.
		char* block = (char*)pd_alloc(pool, nb);
		assert(block != NULL);
		bytes += nb;
		// blocks are always initially zeroed:
		for (size_t j = 0; j < nb; j++) {
			assert(block[j] == 0);
		}
		assert(pd_get_block_size(block) == nb);
		// make sure we can write all over the allocated block:
		memset(block, 0xFF, nb);
		ptr[i] = block;
		assert(pd_get_block_count(pool) == i+1);
		assert(pd_get_bytes_in_use(pool) == bytes);
	}
	assert(pd_get_block_count(pool) == ALLOCS);
	assert(pd_get_bytes_in_use(pool) == bytes);

	int blocks = ALLOCS;
	for (i = 0; i < ALLOCS; i += 2) {
		size_t nb = pd_get_block_size(ptr[i]);
		pd_free(ptr[i]); bytes -= nb; blocks--;
		assert(pd_get_block_count(pool) == blocks);
		assert(pd_get_bytes_in_use(pool) == bytes);
	}

	assert(pd_get_block_size(block21) == 65536);
	assert(pd_get_block_count(pool2) == 1);
	assert(pd_get_bytes_in_use(pool2) == 65536);
	pd_alloc_free_pool(pool2);

	pd_pool_clean(pool);
	// and when we're done, there should be nothing in the pool
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);
}

void test_pd_strcpy()
{
	printf("pd_strcpy\n");
	char five[5];
	pd_strcpy(NULL, 33, "zzzyzzy");

	memset(five, 0, 5);
	pd_strcpy(five, 0, "January");
	assert(five[0] == 0);
	five[0] = '~';
	pd_strcpy(five, 0, "February");
	assert(five[0] == '~');

	memset(five, '@', 5);
	pd_strcpy(five, 1, NULL);
	assert(five[0] == 0);
	assert(five[1] == '@');

	memset(five, '@', 5);
	pd_strcpy(five, 1, "");
	assert(five[0] == 0);
	assert(five[1] == '@');
	assert(five[2] == '@');
	assert(five[3] == '@');
	assert(five[4] == '@');

	memset(five, '@', 5);
	pd_strcpy(five, 5, "");
	assert(five[0] == 0);
	assert(five[1] == '@');
	assert(five[2] == '@');
	assert(five[3] == '@');
	assert(five[4] == '@');

	memset(five, '*', 5);
	pd_strcpy(five, 1, "42");
	assert(five[0] == 0);
	assert(five[1] == '*');
	assert(five[2] == '*');
	assert(five[3] == '*');
	assert(five[4] == '*');

	memset(five, '*', 5);
	pd_strcpy(five, 2, "42");
	assert(five[0] == '4');
	assert(five[1] == 0);
	assert(five[2] == '*');
	assert(five[3] == '*');
	assert(five[4] == '*');

	memset(five, '.', 5);
	pd_strcpy(five, 4, "book");
	assert(five[0] == 'b');
	assert(five[1] == 'o');
	assert(five[2] == 'o');
	assert(five[3] == 0);
	assert(five[4] == '.');

	memset(five, '-', 5);
	pd_strcpy(five, 5, "book");
	assert(five[0] == 'b');
	assert(five[1] == 'o');
	assert(five[2] == 'o');
	assert(five[3] == 'k');
	assert(five[4] == 0);

	memset(five, '#', 5);
	pd_strcpy(five, 5, "books");
	assert(five[0] == 'b');
	assert(five[1] == 'o');
	assert(five[2] == 'o');
	assert(five[3] == 'k');
	assert(five[4] == 0);

	memset(five, '%', 5);
	pd_strcpy(five, 5, "Arbitration");
	assert(five[0] == 'A');
	assert(five[1] == 'r');
	assert(five[2] == 'b');
	assert(five[3] == 'i');
	assert(five[4] == 0);

}

void test_pd_strcmp()
{
	printf("pd_strcmp\n");
	// Note, both arguments to pd_strcmp must be valid 0-terminated C strings.
	assert(pd_strcmp("", "") == 0);
	assert(pd_strcmp("", "a") < 0);
	assert(pd_strcmp(" ", "") > 0);
	assert(pd_strcmp(" ", "!") < 0);
	assert(pd_strcmp("012345678901234567890", "012345678901234567890") == 0);
	assert(pd_strcmp("012345678901234567891", "012345678901234567890") > 0);
	assert(pd_strcmp("012345678901234567890", "012345678901234567891") < 0);
	assert(pd_strcmp("01234567890123456789", "012345678901234567890") < 0);
}

void test_pd_strdup()
{
	printf("pd_strdup\n");
	assert(pd_strdup(os.allocsys, NULL) == NULL);
	const char* fishy = "Hello Fishy";
	// duplicate our small test string:
	char* str1 = pd_strdup(os.allocsys, fishy);
	// it should be equal to the original, but not 'EQ'
	assert(str1 != NULL);
	assert(str1 != fishy);
	assert(str1[0] == 'H');
	assert(pdstrlen(str1) == 11);
	assert(0 == pd_strcmp(fishy, str1));
	// make a copy of the copy of the test string
	char* str2 = pd_strdup(os.allocsys, str1);
	assert(str2 != NULL);
	// all the copies should be equal to the original & each other
	// but not identical
	assert(0 == pd_strcmp(str2, fishy));
	assert(str2 != fishy);
	assert(0 == pd_strcmp(str2, str1));
	assert(str2 != str1);
	assert(0 == pd_strcmp(str1, fishy));
	// free the first copy
	pd_free(str1);
	// check that the 2nd copy
	assert(str2 != NULL);
	assert(0 == pd_strcmp(str2, fishy));
	pd_free(str2);

	char* str3 = pd_strdup(os.allocsys, "");
	assert(str3 != NULL);
	assert(str3[0] == 0);
	assert(0 == pdstrlen(str3));
	pd_free(str3);

	// and when we're done, there should be nothing in the pool
	assert(pd_get_block_count(os.allocsys) == 0);
	assert(pd_get_bytes_in_use(os.allocsys) == 0);
}

// Test pd_putc & pd_putn - byte level output to a stream
void test_byte_output()
{
	printf("stream byte output\n");
	t_pdmempool* pool = os.allocsys;
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);

	t_pdoutstream* out = pd_outstream_new(pool, &os);
	assert(pd_outstream_pos(out) == 0);
	//
	buffer.pos = 0;
	pd_putc(out, ' ');
	assert(0 == strcmp(output, " "));
	assert(pd_outstream_pos(out) == 1);
	pd_putc(out, '\n');
	assert(0 == strcmp(output, " \n"));
	assert(pd_outstream_pos(out) == 2);
	pd_putc(out, 0xFF);
	assert(0 == strcmp(output, " \n\xFF"));
	assert(pd_outstream_pos(out) == 3);
	pd_putc(out, 0);
	assert(0 == strcmp(output, " \n\xFF"));
	assert(pd_outstream_pos(out) == 4);
	pd_putc(out, '%');
	assert(0 == memcmp(output, " \n\xFF\0%", 6));
	assert(pd_outstream_pos(out) == 5);

	buffer.pos = 0;			// reset the output buffer
	pd_puts(out, NULL);
	assert(0 == strcmp(output, ""));			// writes nothing
	pd_puts(out, "");
	assert(0 == strcmp(output, ""));			// writes nothing
	pd_puts(out, " ");
	assert(0 == strcmp(output, " "));
	pd_puts(out, "\n\xFF\0%");					// stops on \0
	assert(0 == strcmp(output, " \n\xFF"));

	const pduint8 rev[] = "\0%\xFF \n";
	buffer.pos = 0;			// reset the output buffer
	pd_putn(out, rev, 0, 0);
	assert(0 == strcmp(output, ""));			// writes nothing
	pd_putn(out, rev, 3, 2);
	assert(0 == strcmp(output, " \n"));
	pd_putn(out, rev, 2, 1);
	assert(0 == strcmp(output, " \n\xFF"));
	pd_putn(out, rev, 0, 2);
	assert(0 == memcmp(output, " \n\xFF\0%", 6));

	pd_outstream_free(out);
	// and when we're done, there should be nothing in the pool
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);
}

// Test pd_putint & puthex - integer output to a stream
void test_int_output()
{
	printf("stream int output\n");
	t_pdmempool* pool = os.allocsys;
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);

	t_pdoutstream* out = pd_outstream_new(pool, &os);
	buffer.pos = 0;
	pd_putint(out, 0);
	assert(0 == strcmp(output, "0"));

	buffer.pos = 0;
	pd_putint(out, -1);
	assert(0 == strcmp(output, "-1"));

	buffer.pos = 0;
	pd_putint(out, 2147483647);					// max pdint32
	assert(0 == strcmp(output, "2147483647"));

	buffer.pos = 0;
	pd_putint(out, 1 << 31);					// min pdint32
	assert(0 == strcmp(output, "-2147483648"));

	buffer.pos = 0;
	pd_puthex(out, 0);
	assert(0 == strcmp(output, "00"));

	buffer.pos = 0;
	pd_puthex(out, 0x11);
	assert(0 == strcmp(output, "11"));

	buffer.pos = 0;
	pd_puthex(out, 0x22);
	assert(0 == strcmp(output, "22"));

	buffer.pos = 0;
	pd_puthex(out, 0x33);
	assert(0 == strcmp(output, "33"));

	buffer.pos = 0;
	pd_puthex(out, 0x44);
	assert(0 == strcmp(output, "44"));

	buffer.pos = 0;
	pd_puthex(out, 0x55);
	assert(0 == strcmp(output, "55"));

	buffer.pos = 0;
	pd_puthex(out, 0x66);
	assert(0 == strcmp(output, "66"));

	buffer.pos = 0;
	pd_puthex(out, 0x77);
	assert(0 == strcmp(output, "77"));

	buffer.pos = 0;
	pd_puthex(out, 0x88);
	assert(0 == strcmp(output, "88"));

	buffer.pos = 0;
	pd_puthex(out, 0x99);
	assert(0 == strcmp(output, "99"));

	buffer.pos = 0;
	pd_puthex(out, 0xAA);
	assert(0 == strcmp(output, "AA"));

	buffer.pos = 0;
	pd_puthex(out, 0xBB);
	assert(0 == strcmp(output, "BB"));

	buffer.pos = 0;
	pd_puthex(out, 0xCC);
	assert(0 == strcmp(output, "CC"));

	buffer.pos = 0;
	pd_puthex(out, 0xDD);
	assert(0 == strcmp(output, "DD"));

	buffer.pos = 0;
	pd_puthex(out, 0xEE);
	assert(0 == strcmp(output, "EE"));

	buffer.pos = 0;
	pd_puthex(out, 0xFF);
	assert(0 == strcmp(output, "FF"));

	pd_outstream_free(out);
	// and when we're done, there should be nothing in the pool
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);
}

void test_floating_point_output()
{
	printf("stream float/double output\n");
	t_pdmempool* pool = os.allocsys;
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);

	t_pdoutstream* out = pd_outstream_new(pool, &os);
	// integral double values
	buffer.pos = 0;
	pd_putfloat(out, 0);
	assert(0 == strcmp(output, "0"));

	buffer.pos = 0;
	pd_putfloat(out, -0.0);
	assert(0 == strcmp(output, "0"));

	buffer.pos = 0;
	pd_putfloat(out, 1);
	assert(0 == strcmp(output, "1"));

	buffer.pos = 0;
	pd_putfloat(out, -1);
	assert(0 == strcmp(output, "-1"));

	buffer.pos = 0;
	pd_putfloat(out, 2147483647.0);
	assert(0 == strcmp(output, "2147483647"));

	buffer.pos = 0;
	pd_putfloat(out, -2147483648.0);
	assert(0 == strcmp(output, "-2147483648"));

	buffer.pos = 0;
	pd_putfloat(out, 1234567890.1);
	assert(0 == strcmp(output, "1234567890.1"));

	buffer.pos = 0;
	pd_putfloat(out, 0.5);
	assert(0 == strcmp(output, "0.5"));

	buffer.pos = 0;
	pd_putfloat(out, 0.25);
	assert(0 == strcmp(output, "0.25"));

	buffer.pos = 0;
	pd_putfloat(out, 0.125);
	assert(0 == strcmp(output, "0.125"));

	buffer.pos = 0;
	pd_putfloat(out, 2.2);
	assert(0 == strcmp(output, "2.2"));

	buffer.pos = 0;
	pd_putfloat(out, 0.1);
	assert(0 == strcmp(output, "0.1"));

	buffer.pos = 0;
	pd_putfloat(out, 0.999999999);
	assert(0 == strcmp(output, "0.999999999"));

	buffer.pos = 0;
	pd_putfloat(out, 0.9999999999);
	assert(0 == strcmp(output, "0.9999999999"));

	buffer.pos = 0;
	pd_putfloat(out, 0.99999999999);
	assert(0 == strcmp(output, "1.0"));

	buffer.pos = 0;
	pd_putfloat(out, 99.999999999);
	assert(0 == strcmp(output, "100.0"));

	buffer.pos = 0;
	pd_putfloat(out, 1.0 / 3);
	assert(0 == strcmp(output, "0.3333333333"));

	buffer.pos = 0;
	pd_putfloat(out, 1.0E-12);
	assert(0 == strcmp(output, "0.000000000001"));

	// test the approximate smallest magnitude recommended for use:
	buffer.pos = 0;
	pd_putfloat(out, 2.0E-38);
	assert(0 == strcmp(output, "0.00000000000000000000000000000000000002"));

	buffer.pos = 0;
	pd_putfloat(out, 1743.98);
	assert(0 == strcmp(output, "1743.98"));

	buffer.pos = 0;
	pd_putfloat(out, 7.0000000009);
	assert(0 == strcmp(output, "7.000000001"));

	buffer.pos = 0;
	pd_putfloat(out, 7.0000000008);				// should round up to 10 digits
	assert(0 == strcmp(output, "7.000000001"));

	buffer.pos = 0;
	pd_putfloat(out, 7.0000000005);				// should round up to 10 digits
	assert(0 == strcmp(output, "7.000000001"));

	buffer.pos = 0;
	pd_putfloat(out, 7.00000039950001);			// should round up
	assert(0 == strcmp(output, "7.0000004"));

	buffer.pos = 0;
	pd_putfloat(out, 602871.0512499);			// should round down
	assert(0 == strcmp(output, "602871.0512"));
	
	buffer.pos = 0;
	pd_putfloat(out, 987654321.5);			// should round down
	assert(0 == strcmp(output, "987654321.5"));

	buffer.pos = 0;
	pd_putfloat(out, 7.00000000049999);			// should round down to 10 digits
	assert(0 == strcmp(output, "7.0"));

	buffer.pos = 0;
	pd_putfloat(out, 7.00000039949999);			// should round down to 10 digits
	assert(0 == strcmp(output, "7.000000399"));

	// test the approximate upper bound recommended for use in PDFs:
	buffer.pos = 0;
	pd_putfloat(out, 3.0E+38);
	assert(39 == strlen(output));
	// truncate the output string to meaningful digits
	output[16] = 0;
	assert(0 == strcmp(output, "3000000000000000"));

	buffer.pos = 0;
	pd_putfloat(out, -3.0E+38);
	assert(40 == strlen(output));
	// truncate the output string to meaningful digits
	output[17] = 0;
	assert(0 == strcmp(output, "-3000000000000000"));

	buffer.pos = 0;
	pd_putfloat(out, 3.141592653589793238E+38);
	assert(39 == strlen(output));
	// truncate the output string to meaningful digits (16)
	// assuming IEEE double-precision:
	output[16] = 0;
	assert(0 == strcmp(output, "3141592653589793"));

#ifdef INFINITY
	double inf = INFINITY;
#else
	double inf = 1.0 / 0;
#endif
	buffer.pos = 0;
	pd_putfloat(out, inf);
	assert(0 == strcmp(output, "inf"));

	buffer.pos = 0;
	pd_putfloat(out, -inf);
	assert(0 == strcmp(output, "-inf"));

	buffer.pos = 0;
	pd_putfloat(out, inf * 0);
	assert(0 == strcmp(output, "nan"));

	pd_outstream_free(out);
	// and when we're done, there should be nothing in the pool
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);
}

static void validate_pdf_time_string(const char* sztime)
{
	int i;
	// D:YYYYMMDDHHMMSSshh'mm   where YYYYMMDD is year-month-day,
	// HHMMSS is time of day in 24-hour clock, s is + or - and hhmm
	// is the offset from UTC to the timezone of the given time.
	// Note: Some sources say that a PDF time string should
	// *end* with an apostrophe, but the ISO 32000:2008 spec does not
	// mention any such trailing apostrophe.
	// So, it's always the same length.
	assert(strlen(sztime) == 22);
	// starts with "D:"
	assert(sztime[0] == 'D');
	assert(sztime[1] == ':');
	assert(sztime[2] == '2');	// valid for quite a while
	assert(sztime[3] == '0');
	assert(isdigit(sztime[4]));	// decade
	assert(isdigit(sztime[5]));	// year in decade
	assert(sztime[6] == '0' || sztime[6] == '1');	// 1st digit of month
	assert(isdigit(sztime[7]));	// 2nd digit of month
	int month = (sztime[6] - '0') * 10 + sztime[7] - '0';
	assert(month >= 1 && month <= 12);
	assert(isdigit(sztime[8]));	// 1st digit of day
	assert(isdigit(sztime[9]));	// 2nd digit of day
	int day = (sztime[8] - '0') * 10 + sztime[9] - '0';
	assert(day >= 1 && day <= 31);
	// HHMMSS
	for (i = 10; i < 16; i++) {
		assert(isdigit(sztime[i]));
	}
	int HH = (sztime[10] - '0') * 10 + sztime[11] - '0';
	int MM = (sztime[12] - '0') * 10 + sztime[13] - '0';
	int SS = (sztime[14] - '0') * 10 + sztime[15] - '0';
	assert(HH >= 0 && HH < 24);
	assert(MM >= 0 && MM < 60);
	assert(SS >= 0 && SS < 60);
	assert(sztime[16] == '+' || sztime[16] == '-');
	assert(isdigit(sztime[17]));
	assert(isdigit(sztime[18]));
	int offh = atoi(sztime + 16);	// timezone offset in hours (Z to local)
	assert(offh >= -12 && offh <= 14);
	// yes, 13 and 14 are apparently technically possible.
	// Do you want to take that call? I don't.
	assert(sztime[19] == '\'');
	assert(isdigit(sztime[20]));
	assert(isdigit(sztime[21]));
	int offm = atoi(sztime + 20);	// timezone offset minutes
	assert(offm == 0 || offm == 15 || offm == 30 || offm == 45);
	// compute total minutes of offset
	int offset = offh * 60;
	offset += (offh >= 0) ? offm : -offm;
	assert(offset >= -12 * 60);
	assert(offset <= 14 * 60);
}

static void test_time_fns()
{
	printf("time functions\n");
	char sztime[200];
	time_t t;
	t_pdmempool* pool = os.allocsys;
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);

	time(&t);
	pd_get_time_string(t, sztime);
	validate_pdf_time_string(sztime);

	struct tm local;
	memset(&local, 0, sizeof local);
	local.tm_hour = 1;
	local.tm_min = 37;
	local.tm_sec = 59;
	local.tm_year = 115;
	local.tm_mon = 2;
	local.tm_mday = 14;
	t = mktime(&local);
	pd_get_time_string(t, sztime);
	validate_pdf_time_string(sztime);
	// date is December 31st 1969
	assert(sztime[2] == '2');
	assert(sztime[3] == '0');
	assert(sztime[4] == '1');
	assert(sztime[5] == '5');
	assert(sztime[6] == '0');
	assert(sztime[7] == '3');
	assert(sztime[8] == '1');
	assert(sztime[9] == '4');
//
	assert(sztime[12] == '3');
	assert(sztime[13] == '7');
	assert(sztime[14] == '5');
	assert(sztime[15] == '9');

	// check the higher-level functions that return string values
	t_pdvalue then = pd_make_time_string(pool, t);
	assert(IS_STRING(then));
	assert(pd_string_length(then.value.stringvalue) == 22);
	memcpy(sztime, pd_string_data(then.value.stringvalue), 22);
	sztime[22] = (char)0;
	validate_pdf_time_string(sztime);

	t_pdvalue now = pd_make_now_string(pool);
	assert(IS_STRING(now));
	assert(pd_string_length(now.value.stringvalue) == 22);
	memcpy(sztime, pd_string_data(now.value.stringvalue), 22);
	sztime[22] = (char)0;
	validate_pdf_time_string(sztime);

	// OK, try futzing with the timezone
	// switch to UTC
#ifdef _WIN32
	_putenv_s("TZ", "UTC");
#else
	setenv("TZ", "UTC", 1);
#endif
	_tzset();
	assert(_timezone == 0);
	pd_get_time_string(t, sztime);
	validate_pdf_time_string(sztime);
	// timezone offset must be +00'00
	assert(sztime[16] == '+');			// should not be negative ;-)
	assert(0 == atoi(sztime + 17));
	assert(0 == atoi(sztime + 20));

	pd_value_free(&now);
	pd_value_free(&then);
	// and when we're done, there should be nothing in the pool
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);
}

static int callbacks;

static pdbool iterCount(t_pdatom atom, t_pdvalue value, void *cookie)
{
	assert(atom != PDA_UNDEFINED_ATOM);
	assert(IS_INT(value));
	assert(cookie == &callbacks);
	callbacks++;
	return PD_TRUE;
}

static pdbool iterStopAfter3(t_pdatom atom, t_pdvalue value, void *cookie)
{
	assert(atom != PDA_UNDEFINED_ATOM);
	assert(IS_INT(value));
	assert(cookie == &callbacks);
	callbacks++;
	return callbacks != 3;
}

static callback_mask;

static pdbool iterCheck5(t_pdatom atom, t_pdvalue value, void *cookie)
{
	assert(cookie == &callbacks);
	assert(IS_INT(value));
	callbacks++;
	switch (value.value.intvalue) {
	case 4:
		assert(atom == PDA_None);
		callback_mask |= 0x01;
		break;
	case 5:
		assert(atom == PDA_Title);
		callback_mask |= 0x02;
		break;
	case 7:
		assert(atom == PDA_Subject);
		callback_mask |= 0x04;
		break;
	case 8:
		assert(atom == PDA_Producer);
		callback_mask |= 0x08;
		break;
	case 42:
		assert(atom == PDA_Creator);
		callback_mask |= 0x10;
		break;
	default:
		assert(!"unexpected intvalue in iterCheck5");
		break;
	} // switch
	return PD_TRUE;
}

void test_hashtables()
{
	printf("hashtables\n");
	t_pdmempool* pool = os.allocsys;
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);

	t_pdhashatomtovalue* table = pd_hashatomtovalue_new(pool, 4);
	assert(table != NULL);
	// check the freshly-minted hashtable in various ways
	// has no entries
	assert(pd_hashatomtovalue_count(table) == 0);
	// has the specified initial capacity
	assert(__pd_hashatomtovalue_capacity(table) == 4);
	// does not contain some randomly chosen keys:
	assert(!pd_hashatomtovalue_contains(table, PDA_Creator));
	assert(!pd_hashatomtovalue_contains(table, PDA_UNDEFINED_ATOM));
	// get fails
	pdbool success = 0xFFFF;
	t_pdvalue val = pd_hashatomtovalue_get(table, PDA_Creator, &success);
	assert(success == PD_FALSE);
	assert(IS_ERR(val));
	assert(val.pdtype == TPDERRVALUE);

	// 'get' doesn't accidentally add the gotten key:
	assert(pd_hashatomtovalue_count(table) == 0);
	// iterating causes no calls to iterator:
	callbacks = 0;
	pd_hashatomtovalue_foreach(table, iterCount, &callbacks);
	assert(callbacks == 0);

	t_pdvalue fortytwo = pdintvalue(42);
	pd_hashatomtovalue_put(table, PDA_Creator, fortytwo);
	assert(pd_hashatomtovalue_count(table) == 1);
	assert(pd_hashatomtovalue_contains(table, PDA_Creator));
	t_pdvalue val2 = pd_hashatomtovalue_get(table, PDA_Creator, &success);
	assert(success == PD_TRUE);
	assert(IS_INT(val2));
	assert(val2.value.intvalue == 42);

	// trying to set the key PDA_UNDEFINED_ATOM has no effect
	pd_hashatomtovalue_put(table, PDA_UNDEFINED_ATOM, fortytwo);
	assert(pd_hashatomtovalue_count(table) == 1);
	assert(pd_hashatomtovalue_contains(table, PDA_Creator));
	assert(!pd_hashatomtovalue_contains(table, PDA_UNDEFINED_ATOM));

	assert(!pd_hashatomtovalue_contains(table, PDA_Producer));
	// insert 4 other entries, forcing table to expand
	pd_hashatomtovalue_put(table, PDA_Producer, pdintvalue(8));
	pd_hashatomtovalue_put(table, PDA_Title, pdintvalue(5));
	pd_hashatomtovalue_put(table, PDA_Subject, pdintvalue(7));
	pd_hashatomtovalue_put(table, PDA_None, pdintvalue(4));
	// table now contains 5 distinct entries
	assert(pd_hashatomtovalue_count(table) == 5);
	assert(pd_hashatomtovalue_contains(table, PDA_None));
	assert(pd_hashatomtovalue_contains(table, PDA_Producer));
	assert(pd_hashatomtovalue_contains(table, PDA_Subject));
	assert(pd_hashatomtovalue_contains(table, PDA_Title));
	assert(pd_hashatomtovalue_contains(table, PDA_Creator)); 
	assert(!pd_hashatomtovalue_contains(table, PDA_Author));
	// capacity should always exceed entry count:
	assert(__pd_hashatomtovalue_capacity(table) >= 5);

	// iterating causes 5 calls to iterator with expected parameters
	callbacks = callback_mask = 0;
	pd_hashatomtovalue_foreach(table, iterCheck5, &callbacks);
	assert(callbacks == 5);
	assert(callback_mask == 0x1F);

	// check that early termination on iter()=>0 works
	callbacks = 0;
	pd_hashatomtovalue_foreach(table, iterStopAfter3, &callbacks);
	assert(callbacks == 3);

	// NULL table means do nothing:
	callbacks = 0;
	pd_hashatomtovalue_foreach(NULL, iterCount, &callbacks);
	assert(callbacks == 0);
	// NULL iterator means do nothing.
	// little hard to test this case, but at least, don't crash.
	pd_hashatomtovalue_foreach(table, NULL, &callbacks);

	// update some entries, which should not change entry count
	pd_hashatomtovalue_put(table, PDA_Producer, pdintvalue(-8));
	pd_hashatomtovalue_put(table, PDA_Title, pdintvalue(-5));
	pd_hashatomtovalue_put(table, PDA_Subject, pdintvalue(-7));
	pd_hashatomtovalue_put(table, PDA_None, pdintvalue(-4));
	assert(pd_hashatomtovalue_count(table) == 5);
	assert(pd_hashatomtovalue_contains(table, PDA_None));
	assert(pd_hashatomtovalue_contains(table, PDA_Producer));
	assert(pd_hashatomtovalue_contains(table, PDA_Subject));
	assert(pd_hashatomtovalue_contains(table, PDA_Title));
	assert(pd_hashatomtovalue_contains(table, PDA_Creator));
	assert(!pd_hashatomtovalue_contains(table, PDA_Author));
	val2 = pd_hashatomtovalue_get(table, PDA_Subject, &success);
	assert(success == PD_TRUE);
	assert(IS_INT(val2));
	assert(val2.value.intvalue == -7);
	val2 = pd_hashatomtovalue_get(table, PDA_None, &success);
	assert(success == PD_TRUE);
	assert(IS_INT(val2));
	assert(val2.value.intvalue == -4);

	pd_hashatomtovalue_free(table);
	// and when we're done, there should be nothing in the pool
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);
}

void test_atoms()
{
	printf("atoms\n");
	t_pdmempool* pool = os.allocsys;
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);

	assert(pd_atom_table_count(NULL) == 0);

	t_pdatomtable* atoms = pd_atom_table_new(pool, 1);
	assert(atoms != NULL);
	assert(pd_atom_table_count(atoms) == 0);
	// check the freshly-minted hashtable in various ways
	t_pdatom foo = pd_atom_intern(atoms, "foo");
	t_pdatom foo2 = pd_atom_intern(atoms, "foo");
	assert(foo == foo2);
	assert(pd_atom_table_count(atoms) == 1);
	// check that atom names are case-sensitive
	// and that table expansion works
	t_pdatom Foo = pd_atom_intern(atoms, "Foo");
	assert(Foo != foo && Foo != foo2);
	t_pdatom foO = pd_atom_intern(atoms, "foO");
	assert(foO != foo && foO != foo2 && foO != Foo);
	t_pdatom bar = pd_atom_intern(atoms, "bar");
	assert(pd_atom_table_count(atoms) == 4);
	// after more additions, and expansion, lookup still works?
	assert(pd_atom_intern(atoms, "foo") == foo);
	assert(pd_atom_intern(atoms, "Foo") == Foo);
	assert(pd_atom_intern(atoms, "foO") == foO);
	assert(pd_atom_intern(atoms, "bar") == bar);
	assert(pd_atom_table_count(atoms) == 4);

	// Simulate a kind of expected load
	for (int stripno = 0; stripno < 1000; stripno++) {
		char stripName[20];
		sprintf_s(stripName, 20, "Strip%d", stripno);
		t_pdatom strip = pd_atom_intern(atoms, stripName);
		assert(strip);
		assert(pd_strcmp(pd_atom_name(strip), stripName) == 0);
		assert(pd_atom_table_count(atoms) == 4+stripno+1);
	}

	pd_atom_table_free(atoms);

	// and when we're done, there should be nothing in the pool
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);
}

static int jenny;		// variable used just for its address.

static void generate_content(t_pdcontents_gen *gen, void *cookie)
{
	// check that we got the expected cookie
	assert(cookie == &jenny);
	// create a transient atom table, co-allocated with the gen:
	t_pdmempool* pool = pd_get_pool(gen);
	assert(pool);
	t_pdatomtable* atoms = pd_atom_table_new(pool, 99);
	assert(atoms);
	// set up some dummy page content parameters
	int strips = 2;
	int W = 1600, H = 2200;
	int SH = H / strips;
	int tx = 0, ty = H;
	// generate simulated page content
	for (int n = 0; n < strips; n++) {
		char stripNname[12] = "Strip";
		pd_strcpy(stripNname + 5, ELEMENTS(stripNname) - 5, pditoa(n));
		t_pdatom stripNatom = pd_atom_intern(atoms, stripNname);
		pd_gen_gsave(gen);
		pd_gen_concatmatrix(gen, W, 0, 0, SH, tx, ty - SH);
		pd_gen_xobject(gen, stripNatom);
		pd_gen_grestore(gen);
		ty -= SH;
	}
	pd_atom_table_free(atoms);
}

static pdbool sink_put(const pduint8 *buffer, pduint32 offset, pduint32 len, void *cookie)
{
	t_pdoutstream *outstm = (t_pdoutstream *)cookie;
	pd_putn(outstm, buffer, offset, len);
	return PD_TRUE;
}
static int sink_free_hit;

static void sink_free(void *cookie)
{
	(void)cookie;
	sink_free_hit++;
}

void test_contents_generator()
{
	printf("content generator\n");
	t_pdmempool* pool = os.allocsys;
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);
	t_pdoutstream* out = pd_outstream_new(pool, &os);

	// create a content generator
	t_pdcontents_gen *gen = pd_contents_gen_new(pool, generate_content, &jenny);
	// <pretend some time passes...>
	// set up a sink for the content - writes content to memory stream
	sink_free_hit = 0;
	t_datasink* sink = pd_datasink_new(pool, sink_put, sink_free, out);
	buffer.pos = 0;
	// generate the content
	pd_contents_generate(sink, gen);
	// check correct output generated
	assert(0 == strcmp(output,
		" q 1600 0 0 1100 0 1100 cm /Strip0 Do Q q 1600 0 0 1100 0 0 cm /Strip1 Do Q"));
	// free the sink
	pd_datasink_free(sink);
	// the sink's sink_free handler should be called exactly once:
	assert(sink_free_hit == 1);

	pd_contents_gen_free(gen);
	pd_outstream_free(out);
	// and when we're done, there should be nothing in the pool
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);
}

void test_dicts()
{
	printf("dicts\n");
	t_pdmempool* pool = os.allocsys;
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);
	t_pdoutstream* out = pd_outstream_new(pool, &os);

	t_pdvalue dict = pd_dict_new(pool, 4);
	// check the freshly-minted dictionary in various ways
	// is a dictionary...
	assert(IS_DICT(dict));
	assert(dict.value.dictvalue != NULL);
	// is not a stream
	assert(!pd_dict_is_stream(dict));
	// has no entries
	assert(pd_dict_count(dict) == 0);
	// has the specified initial capacity
	assert(__pd_dict_capacity(dict) == 4);
	// does not contain some randomly chosen keys:
	assert(!pd_dict_contains(dict, PDA_Creator));
	assert(!pd_dict_contains(dict, PDA_UNDEFINED_ATOM));
	// get fails
	pdbool success = 0xFFFF;
	t_pdvalue val = pd_dict_get(dict, PDA_Creator, &success);
	assert(success == PD_FALSE);
	assert(IS_ERR(val));
	// 'get' doesn't accidentally add the gotten key:
	assert(pd_dict_count(dict) == 0);
	// iterating causes no calls to iterator:
	callbacks = 0;
	pd_dict_foreach(dict, iterCount, &callbacks);
	assert(callbacks == 0);
	// writing an empty dictionary produces the expected result
	buffer.pos = 0;
	pd_write_value(out, dict);
	assert(0 == strcmp(output, "<< >>"));

	t_pdvalue fortytwo = pdintvalue(42);
	pd_dict_put(dict, PDA_Creator, fortytwo);
	assert(pd_dict_count(dict) == 1);
	assert(pd_dict_contains(dict, PDA_Creator));
	t_pdvalue val2 = pd_dict_get(dict, PDA_Creator, &success);
	assert(success == PD_TRUE);
	assert(IS_INT(val2));
	assert(val2.value.intvalue == 42);
	// writing dictionary produces the expected result
	buffer.pos = 0;
	pd_write_value(out, dict);
	assert(0 == strcmp(output, "<< /Creator 42 >>"));

	// trying to set the key PDA_UNDEFINED_ATOM has no effect
	pd_dict_put(dict, PDA_UNDEFINED_ATOM, fortytwo);
	assert(pd_dict_count(dict) == 1);
	assert(pd_dict_contains(dict, PDA_Creator));
	assert(!pd_dict_contains(dict, PDA_UNDEFINED_ATOM));

	assert(!pd_dict_contains(dict, PDA_Producer));
	// insert 4 other entries, forcing dict to expand
	pd_dict_put(dict, PDA_Producer, pdintvalue(8));
	pd_dict_put(dict, PDA_Title, pdintvalue(5));
	pd_dict_put(dict, PDA_Subject, pdintvalue(7));
	pd_dict_put(dict, PDA_None, pdintvalue(4));
	// dict now contains 5 distinct entries
	assert(pd_dict_count(dict) == 5);
	assert(pd_dict_contains(dict, PDA_None));
	assert(pd_dict_contains(dict, PDA_Producer));
	assert(pd_dict_contains(dict, PDA_Subject));
	assert(pd_dict_contains(dict, PDA_Title));
	assert(pd_dict_contains(dict, PDA_Creator));
	assert(!pd_dict_contains(dict, PDA_Author));
	// capacity should always exceed entry count:
	assert(__pd_dict_capacity(dict) >= 5);

	// iterating causes 5 calls to iterator with expected parameters
	callbacks = callback_mask = 0;
	pd_dict_foreach(dict, iterCheck5, &callbacks);
	assert(callbacks == 5);
	assert(callback_mask == 0x1F);

	// check that early termination on iter()=>0 works
	callbacks = 0;
	pd_dict_foreach(dict, iterStopAfter3, &callbacks);
	assert(callbacks == 3);

	//giving pd_dict_foreach a non-dictionary does nothing.
	callbacks = 0;
	pd_dict_foreach(fortytwo, iterCount, &callbacks);
	assert(callbacks == 0);
	// NULL iterator means do nothing.
	// little hard to test this case, but at least, don't crash:
	pd_dict_foreach(dict, NULL, &callbacks);

	// update some entries, which should not change entry count
	pd_dict_put(dict, PDA_Producer, pdintvalue(-8));
	pd_dict_put(dict, PDA_Title, pdintvalue(-5));
	pd_dict_put(dict, PDA_Subject, pdintvalue(-7));
	pd_dict_put(dict, PDA_None, pdintvalue(-4));
	assert(pd_dict_count(dict) == 5);
	assert(pd_dict_contains(dict, PDA_None));
	assert(pd_dict_contains(dict, PDA_Title));
	assert(pd_dict_contains(dict, PDA_Producer));
	assert(pd_dict_contains(dict, PDA_Subject));
	assert(pd_dict_contains(dict, PDA_Creator));
	assert(!pd_dict_contains(dict, PDA_Author));
	val2 = pd_dict_get(dict, PDA_Subject, &success);
	assert(success == PD_TRUE);
	assert(IS_INT(val2));
	assert(val2.value.intvalue == -7);
	val2 = pd_dict_get(dict, PDA_None, &success);
	assert(success == PD_TRUE);
	assert(IS_INT(val2));
	assert(val2.value.intvalue == -4);

	// writing dictionary produces the expected result
	buffer.pos = 0;
	pd_write_value(out, dict);
	assert(output[0] == '<');
	assert(output[1] == '<');
	assert(output[2] == ' ');
	assert(strstr(output + 3, "/None "));
	assert(strstr(output + 3, "/Title "));
	assert(strstr(output + 3, "/Subject "));
	assert(strstr(output + 3, "/Creator "));
	assert(strstr(output + 3, "/Producer "));
	assert(strstr(output + 3, " -4 "));
	assert(strstr(output + 3, " -5 "));
	assert(strstr(output + 3, " -7 "));
	assert(strstr(output + 3, " 42 "));
	assert(strstr(output + 3, " -8 "));
	assert(strlen(output) == 61);
	// output ends with " >>"
	assert(strstr(output, " >>"));
	assert(strlen(strstr(output, " >>")) == 3);

	pd_dict_free(dict);
	pd_outstream_free(out);
	// and when we're done, there should be nothing in the pool
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);
}

static pdbool arrayIterCount(t_pdarray *arr, pduint32 currindex, t_pdvalue value, void *cookie)
{
	assert(arr);
	assert(currindex >= 0);
	assert(currindex < pd_array_count(arr));
	assert(cookie == &callbacks);
	callbacks++;
	return PD_TRUE;
}

static pdbool arrayIterStopAfter3(t_pdarray *arr, pduint32 currindex, t_pdvalue value, void *cookie)
{
	arrayIterCount(arr, currindex, value, cookie);
	return callbacks != 3;
}

void test_arrays()
{
	printf("arrays\n");
	t_pdmempool* pool = os.allocsys;
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);
	t_pdoutstream* out = pd_outstream_new(pool, &os);
	t_pdvalue hello = pdcstrvalue(pool, "hello?");

	t_pdarray *arr = pd_array_new(pool, 3);
	assert(arr);
	t_pdvalue array = pdarrayvalue(arr);
	// check the freshly-minted array in various ways
	// is an array...
	assert(IS_ARRAY(array));
	assert(array.value.arrvalue == arr);
	// has no entries
	assert(pd_array_count(arr) == 0);
	// has the specified initial capacity
	assert(pd_array_capacity(arr) == 3);
	// get fails
	t_pdvalue val = pd_array_get(arr, 0);
	assert(IS_ERR(val));
	// 'get' doesn't accidentally change the array:
	assert(pd_array_count(arr) == 0);
	assert(pd_array_capacity(arr) == 3);

	// iterating causes no calls to iterator:
	callbacks = 0;
	pd_array_foreach(arr, arrayIterCount, &callbacks);
	assert(callbacks == 0);
	// writing an empty aarray produces the expected result
	buffer.pos = 0;
	pd_write_value(out, array);
	assert(0 == strcmp(output, "[ ]"));

	t_pdvalue fortytwo = pdintvalue(42);
	pdbool done = pd_array_add(arr, fortytwo);
	assert(done);
	assert(pd_array_count(arr) == 1);
	assert(pd_array_capacity(arr) == 3);	// unchanged
	val = pd_array_get(arr, 0);
	assert(IS_INT(val) && val.value.intvalue == 42);

	// writing array produces the expected result
	buffer.pos = 0;
	pd_write_value(out, array);
	assert(0 == strcmp(output, "[ 42 ]"));

	// insert 3 more entries, forcing dict to expand
	pd_array_add(arr, pdnullvalue());
	pd_array_add(arr, pdboolvalue(PD_FALSE));
	pd_array_add(arr, hello);
	// array now contains 4 entries
	assert(pd_array_count(arr) == 4);
	// and capacity has necessarily increased
	assert(pd_array_capacity(arr) >= 4);
	// and all entries are where we expect them to be
	assert(pd_value_eq(pd_array_get(arr, 0), fortytwo));
	assert(IS_NULL(pd_array_get(arr, 1)));
	assert(IS_FALSE(pd_array_get(arr, 2)));
	assert(pd_value_eq(pd_array_get(arr, 3), hello));
	// and there is no 5th element
	assert(IS_ERR(pd_array_get(arr, 4)));
	// or zillionth
	assert(IS_ERR(pd_array_get(arr, 0xFFFFFFFF)));

	// writing array produces the expected result
	buffer.pos = 0;
	pd_write_value(out, array);
	assert(0 == strcmp(output, "[ 42 null false (hello?) ]"));

	// iterating causes 4 calls to iterator with expected parameters
	callbacks = callback_mask = 0;
	pd_array_foreach(arr, arrayIterCount, &callbacks);
	assert(callbacks == 4);
	//assert(callback_mask == 0x1F);

	// check that early termination on iter()=>0 works
	callbacks = 0;
	pd_array_foreach(arr, arrayIterStopAfter3, &callbacks);
	assert(callbacks == 3);

	// NULL iterator means do nothing.
	// little hard to test this case, but at least, don't crash:
	pd_array_foreach(arr, NULL, &callbacks);

	// replace entries, which should not change entry count
	pd_array_set(arr, 2, pdintvalue(-2));
	pd_array_set(arr, 0, pdintvalue(-4));
	pd_array_set(arr, 3, pdintvalue(-1));
	pd_array_set(arr, 1, pdintvalue(-3));
	assert(pd_array_count(arr) == 4);
	assert(pd_value_eq(pd_array_get(arr, 0), pdintvalue(-4)));
	assert(pd_value_eq(pd_array_get(arr, 1), pdintvalue(-3)));
	assert(pd_value_eq(pd_array_get(arr, 2), pdintvalue(-2)));
	assert(pd_value_eq(pd_array_get(arr, 3), pdintvalue(-1)));

	pd_array_insert(arr, 4, pdintvalue(4));
	pd_array_insert(arr, 3, pdintvalue(3));
	pd_array_insert(arr, 2, pdintvalue(2));
	pd_array_insert(arr, 1, pdintvalue(1));
	pd_array_insert(arr, 0, pdintvalue(0));
	// writing array produces the expected result
	buffer.pos = 0;
	pd_write_value(out, array);
	assert(0 == strcmp(output, "[ 0 -4 1 -3 2 -2 3 -1 4 ]"));
	assert(pd_array_capacity(arr) >= 9);

	val = pd_array_remove(arr, 1);
	assert(pd_value_eq(val, pdintvalue(-4)));
	val = pd_array_remove(arr, 2);
	assert(pd_value_eq(val, pdintvalue(-3)));
	val = pd_array_remove(arr, 3);
	assert(pd_value_eq(val, pdintvalue(-2)));
	val = pd_array_remove(arr, 4);
	assert(pd_value_eq(val, pdintvalue(-1)));
	assert(pd_array_count(arr) == 5);
	// writing array produces the expected result
	buffer.pos = 0;
	pd_write_value(out, array);
	assert(0 == strcmp(output, "[ 0 1 2 3 4 ]"));
	// capacity may or may not be decreased by pd_array_remove
	assert(pd_array_capacity(arr) >= pd_array_count(arr));
	// make sure we try removing the tail element
	val = pd_array_remove(arr, 4);
	assert(pd_value_eq(val, pdintvalue(4)));
	assert(pd_array_count(arr) == 4);
	// make sure removing beyond the end returns error:
	val = pd_array_remove(arr, 4);
	assert(IS_ERR(val));
	assert(pd_array_count(arr) == 4);
	// writing array produces the expected result
	buffer.pos = 0;
	pd_write_value(out, array);
	assert(0 == strcmp(output, "[ 0 1 2 3 ]"));
	// free the array
	// (it contains only integer values, no need to free them.)
	pd_value_free(&array);

	// create the array fresh from some values
	arr = pd_array_build(pool, 3, hello, pdfloatvalue(1.5), pdnullvalue());
	array = pdarrayvalue(arr);
	buffer.pos = 0;
	pd_write_value(out, array);
	assert(0 == strcmp(output, "[ (hello?) 1.5 null ]"));
	pd_value_free(&array);

	arr = pd_array_buildints(pool, 5, (1 << 31), 0, 771723882, -950158714, 0xDEAD);
	array = pdarrayvalue(arr);
	buffer.pos = 0;
	pd_write_value(out, array);
	assert(0 == strcmp(output, "[ -2147483648 0 771723882 -950158714 57005 ]"));
	pd_value_free(&array);

	arr = pd_array_buildfloats(pool, 4, -1.0, 0.0 * -1, 0.376739502, 987654321.5);
	array = pdarrayvalue(arr);
	buffer.pos = 0;
	pd_write_value(out, array);
	// Note: floats (really doubles) with values that we can represent
	// exactly as integers, print as integers.
	// I'm not even sure why we have 'int' values, when we have 64-bit floats.
	assert(0 == strcmp(output, "[ -1 0 0.376739502 987654321.5 ]"));
	pd_value_free(&array);

	// test void pd_array_destroy(t_pdvalue *arr);
	arr = pd_array_build(pool, 3,
		pdcstrvalue(pool, "In life,"),
		pdcstrvalue(pool, "as in art,"),
		pdcstrvalue(pool, "the beautiful moves in curves."));
	array = pdarrayvalue(arr);
	buffer.pos = 0;
	pd_write_value(out, array);
	assert(0 == strcmp(output, "[ (In life,) (as in art,) (the beautiful moves in curves.) ]"));
	pd_array_destroy(&array);

	pd_outstream_free(out);
	pd_value_free(&hello);

	// and when we're done, there should be nothing in the pool
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);
}


static int cookie;
static const char* hello = "Hello, World!";

static void on_datasink_ready(t_datasink *sink, void *eventcookie)
{
	assert(sink != NULL);
	assert(eventcookie == &cookie);
	pd_datasink_put(sink, (const pduint8*)hello, 0, pdstrlen(hello));
}

void test_streams()
{
	printf("streams\n");
	t_pdmempool* pool = os.allocsys;
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);
	// create an output stream that writes to the 'output' buffer
	t_pdoutstream* out = pd_outstream_new(pool, &os);
	// create an XREF table to register indirect objects:
	t_pdxref* xref = pd_xref_new(pool);

	t_pdvalue streamref = stream_new(pool, xref, 6, on_datasink_ready, &cookie);
	// check the freshly-minted stream in various ways
	// it's both a stream AND a dictionary...
	assert(IS_REFERENCE(streamref));
	t_pdvalue stream = pd_reference_get_value(streamref);
	assert(IS_DICT(stream));
	assert(stream.value.dictvalue != NULL);
	assert(pd_dict_is_stream(stream));
	assert(IS_STREAM(stream));
	// has no entries
	assert(pd_dict_count(stream) == 0);
	// Assume since the stream IS_DICT that all the pd_dict_* functions work correctly.
	pd_dict_put(stream, PDA_Length, pdintvalue(pdstrlen(hello)));
	// now the stream has an entry:
	assert(pd_dict_count(stream) == 1);
	assert(pd_dict_contains(stream, PDA_Length));
	// writing an empty stream produces the expected result
	buffer.pos = 0;
	pd_write_value(out, stream);
	assert(0 == strcmp(output, "<< /Length 13 >>\r\nstream\r\nHello, World!\r\nendstream\r\n"));
	// check that XREF table has changed state appropriately:
	assert(pd_xref_size(xref) == 1);			// just the Stream object

	stream_free(streamref);		// frees the ref and the referenced Stream
	pd_outstream_free(out);
	pd_xref_free(xref);

	// and when we're done, there should be nothing in the pool
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);
}

void test_write_value()
{
	printf("writing t_pdvalue's\n");
	t_pdmempool* pool = os.allocsys;
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);
	// create helper objects
	t_pdoutstream* out = pd_outstream_new(pool, &os);
	assert(out != NULL);
	t_pdvalue splorg = pdcstrvalue(pool, "Splorg");
	assert(IS_STRING(splorg));

	buffer.pos = 0;										// reset the memory buffer
	pd_write_value(out, pdnullvalue());
	assert(0 == strcmp(output, "null"));

	buffer.pos = 0;										// reset the memory buffer
	pd_write_value(out, pderrvalue());
	assert(0 == strcmp(output, "*ERROR*"));

	buffer.pos = 0;										// reset the memory buffer
	pd_write_value(out, pdatomvalue(PDA_DeviceRGB));
	assert(0 == strcmp(output, "/DeviceRGB"));
	// TODO: test the '#' escape in atom names

	buffer.pos = 0;										// reset the memory buffer
	pd_write_value(out, pdintvalue(-123456789));
	assert(0 == strcmp(output, "-123456789"));

	buffer.pos = 0;										// reset the memory buffer
	pd_write_value(out, pdboolvalue(PD_TRUE));
	assert(0 == strcmp(output, "true"));

	buffer.pos = 0;										// reset the memory buffer
	pd_write_value(out, pdboolvalue(PD_FALSE));
	assert(0 == strcmp(output, "false"));

	buffer.pos = 0;										// reset the memory buffer
	pd_write_value(out, pdfloatvalue(-195.125));
	assert(0 == strcmp(output, "-195.125"));

	buffer.pos = 0;										// reset the memory buffer
	t_pdvalue str2 = pdcstrvalue(pool, "In liberty (xyz)\n");
	pd_write_value(out, str2);
	assert(0 == strcmp(output, "(In liberty \\(xyz\\)\\012)"));
	pd_value_free(&str2);

	buffer.pos = 0;										// reset the memory buffer
	pd_write_value(out, splorg);
	assert(0 == strcmp(output, "(Splorg)"));

	//extern t_pdvalue pdarrayvalue(t_pdarray *arr);
	t_pdarray *arr = pd_array_new(pool, 0);
	assert(arr);
	// check empty array
	buffer.pos = 0;
	pd_write_value(out, pdarrayvalue(arr));
	assert(0 == strcmp(output, "[ ]"));
	// 1 element
	pd_array_add(arr, splorg);
	buffer.pos = 0;
	pd_write_value(out, pdarrayvalue(arr));
	assert(0 == strcmp(output, "[ (Splorg) ]"));
	// 2 elements
	pd_array_add(arr, pdnullvalue());
	buffer.pos = 0;
	pd_write_value(out, pdarrayvalue(arr));
	assert(0 == strcmp(output, "[ (Splorg) null ]"));
	pd_array_free(arr);

	// dictionary & stream writing is tested in with those value-types
	// reference writing is tested in the Indirect Object & XREF tables section

	// free everything we created
	pd_outstream_free(out);
	pd_value_free(&splorg);

	// and when we're done, there should be nothing in the pool
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);
}

void test_xref_tables()
{
	printf("Indirect Objects & XREF tables\n");
	t_pdmempool* pool = os.allocsys;
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);
	// create helper objects
	t_pdoutstream* out = pd_outstream_new(pool, &os);
	assert(out != NULL);
	t_pdvalue splorg = pdcstrvalue(pool, "Splorg");
	assert(IS_STRING(splorg));

	// allocate an empty XREF table
	t_pdxref* xref = pd_xref_new(pool);
	assert(xref != NULL);
	// it's empty now
	assert(pd_xref_size(xref) == 0);

	t_pdvalue ref = pd_xref_makereference(NULL, splorg);
	assert(IS_NULL(ref));
	// OK now for realz
	ref = pd_xref_makereference(xref, splorg);
	assert(IS_REFERENCE(ref));
	// The first indirect object in XREF is number 1 (entry 0 is reserved)
	assert(1 == pd_reference_object_number(ref));
	// object's definition has not been written:
	assert(pd_reference_is_written(ref) == PD_FALSE);
	// has file position 0 (which is impossible)
	assert(pd_reference_get_position(ref) == 0);
	t_pdvalue val = pd_reference_get_value(ref);
	assert(IS_STRING(val));
	assert(pd_string_equal(val.value.stringvalue, splorg.value.stringvalue));
	// xref state has now changed
	assert(pd_xref_size(xref) == 1);

	// create a reference to 0
	// start with an integer value of 0
	t_pdvalue zero = pdintvalue(0);
	ref = pd_xref_makereference(xref, zero);
	assert(IS_REFERENCE(ref));
	assert(2 == pd_reference_object_number(ref));
	assert(pd_reference_is_written(ref) == PD_FALSE);
	val = pd_reference_get_value(ref);
	assert(IS_INT(val));
	assert(val.value.intvalue == 0);
	// xref state has now changed
	assert(pd_xref_size(xref) == 2);
	// try making another reference to integer 0
	ref = pd_xref_makereference(xref, pdintvalue(0));
	// should be treated as a new reference (why exactly?)
	assert(pd_xref_size(xref) == 3);
	assert(IS_REFERENCE(ref));
	// it's NOT the same indirect object as the previous 'zero'
	assert(3 == pd_reference_object_number(ref));

	t_pdvalue fwd = pd_xref_create_forward_reference(xref);
	assert(IS_REFERENCE(fwd));
	assert(!pd_reference_is_written(fwd));
	assert(pd_reference_get_position(fwd) == 0);
	// xref state changed again
	assert(pd_xref_size(xref) == 4);
	t_pdvalue unresolved = pd_reference_get_value(fwd);
	assert(IS_NULL(unresolved));
	pd_reference_resolve(fwd, pdintvalue(76));
	unresolved = pd_reference_get_value(fwd);
	assert(IS_INT(unresolved));
	assert(unresolved.value.intvalue == 76);
	assert(!pd_reference_is_written(fwd));
	assert(pd_reference_get_position(fwd) == 0);
	// try writing the forward reference to our test stream
	buffer.pos = 0;										// reset the memory buffer
	pd_write_value(out, fwd);
	assert(0 == strcmp(output, "4 0 R"));
	pd_puts(out, "\n");

	// test writing forward indirect object definition to stream
	int fwdpos = pd_outstream_pos(out);					// capture stream position
	assert(fwdpos == 6);
	pd_write_reference_declaration(out, fwd);			// write out object definition
	assert(pd_reference_is_written(fwd));				// now it's been written
	assert(pd_reference_get_position(fwd) == fwdpos);	// and position updated
	// check stream output
	assert(0 == strcmp(output+fwdpos, "4 0 obj\n76\nendobj\n"));

	// check that trying to write it again has no effect
	pd_write_reference_declaration(out, fwd);			// try to write out object def 2nd time
	assert(pd_reference_get_position(fwd) == fwdpos);	// doesn't change position
	assert(0 == strcmp(output+fwdpos, "4 0 obj\n76\nendobj\n"));	// doesn't add anything to stream

	// Test that null indirect objects aren't converged
	t_pdvalue nullref1 = pd_xref_makereference(xref, pdnullvalue());
	t_pdvalue nullref2 = pd_xref_makereference(xref, pdnullvalue());
	assert(IS_REFERENCE(nullref1));
	assert(IS_REFERENCE(nullref2));
	assert(pd_reference_object_number(nullref1) != pd_reference_object_number(nullref2));
	assert(pd_xref_size(xref) == 6);

	pd_xref_writeallpendingreferences(xref, out);
	assert(0 == strcmp(output,
		"4 0 R\n4 0 obj\n76\nendobj\n1 0 obj\n(Splorg)\nendobj\n2 0 obj\n0\nendobj\n3 0 obj\n0\nendobj\n5 0 obj\nnull\nendobj\n6 0 obj\nnull\nendobj\n"));

	// Write out the actual famous XREF table - containing the recorded offsets (file positions)
	// of the previously written definitions of all indirect objects.
	buffer.pos = 0;
	pd_xref_writetable(xref, out);
	assert(0 == strcmp(output,
		"xref\n0 7\n0000000000 65535 f\r\n0000000024 00000 n\r\n0000000048 00000 n\r\n0000000065 00000 n\r\n0000000006 00000 n\r\n0000000082 00000 n\r\n0000000102 00000 n\r\n"));

	// free everything we created
	pd_xref_free(xref);
	pd_outstream_free(out);
	pd_value_free(&splorg);

	// and when we're done, there should be nothing in the pool
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);
}

void test_file_structure()
{
	printf("file structure\n");
	t_pdmempool* pool = os.allocsys;
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);

	// create helper objects
	t_pdoutstream* out = pd_outstream_new(pool, &os);
	buffer.pos = 0;										// reset the memory buffer

	// Test the function: pd_write_pdf_header
	pd_write_pdf_header(out, "1.4", NULL);
	assert(0 == strcmp(output, "%PDF-1.4\n%‚„œ”\n"));

	buffer.pos = 0;
	pd_write_pdf_header(out, "2.0", "xyzzy");
	assert(0 == strcmp(output, "%PDF-2.0\nxyzzy\n"));
	// doesn't matter if 2nd line ends with \n or not
	buffer.pos = 0;
	pd_write_pdf_header(out, "2.0", "xyzzy\n");
	assert(0 == strcmp(output, "%PDF-2.0\nxyzzy\n"));
	// to have 'no' 2nd line
	buffer.pos = 0;
	pd_write_pdf_header(out, "2.0", "\n");
	assert(0 == strcmp(output, "%PDF-2.0\n\n"));

	// Test pd_write_endofdocument
	buffer.pos = 0;
	pd_write_endofdocument(out, NULL, pdnullvalue(), pdnullvalue());
	assert(0 == strcmp(output, "trailer\n<< /Size 0 /Root null /Info null /ID [ <D41D8CD98F00B204E9800998ECF8427E> <D41D8CD98F00B204E9800998ECF8427E> ] >>\nstartxref\n55\n%%EOF\n"));

	// construct minimal versions of everything pd_write_endofdocument wants.
	t_pdvalue DID = pd_dict_new(pool, 0);
	t_pdvalue catalog = pd_dict_new(pool, 0);
	t_pdxref* xref = pd_xref_new(pool);
	// refresh the output stream
	buffer.pos = 0; pd_outstream_free(out); out = pd_outstream_new(pool, &os);
	// write out a 'classic' PDF header
	pd_write_pdf_header(out, "1.6", "%‚„œ”");
	// then write out all the ending stuff.
	pd_write_endofdocument(out, xref, catalog, DID);
	assert(0 == pd_strcmp(output,
"%PDF-1.6\n"
"%‚„œ”\n"
"xref\n"
"0 1\n"
"0000000000 65535 f\r\n"
"trailer\n"
"<< /Size 0 /Root << >> /Info << >> /ID [ <D41D8CD98F00B204E9800998ECF8427E> <D41D8CD98F00B204E9800998ECF8427E> ] >>\n"
"startxref\n"
"15\n"
"%%EOF\n"
));

	// free everything we created
	pd_dict_free(catalog);
	pd_dict_free(DID);
	pd_xref_free(xref);
	pd_outstream_free(out);

	// and when we're done, there should be nothing in the pool
	assert(pd_get_block_count(pool) == 0);
	assert(pd_get_bytes_in_use(pool) == 0);
}

//////////////////////////////////////////////////////////////////////////////
// main function, top level test driver

int main(int argc, char** argv)
{
	printf("pdfraster-writer Test Suite\n");

	os.alloc = mymalloc;
	os.free = free;
	os.memset = myMemSet;
	os.allocsys = pd_alloc_new_pool(&os);
	os.writeout = myOutputWriter;
	os.writeoutcookie = &buffer;

	test_alloc();

	test_pd_strcpy();
	test_pd_strcmp();
	test_pd_strdup();

	// putting single bytes to output stream
	test_byte_output();
	test_int_output();
	test_floating_point_output();

	test_time_fns();

	test_hashtables();
	test_atoms();
	test_contents_generator();
	test_arrays();
	test_dicts();
	test_streams();
	test_write_value();
	test_xref_tables();

	test_file_structure();

	printf("Hit enter to exit:\n");
	getchar();
	return 0;
}

