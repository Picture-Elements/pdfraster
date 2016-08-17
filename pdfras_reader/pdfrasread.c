#include "pdfrasread.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <limits.h>

#define MIN(a,b) ((a)<(b) ? (a) : (b))
#define MAX(a,b) ((a)>(b) ? (a) : (b))

///////////////////////////////////////////////////////////////////////
// Internal Constants

#define PDFRASREAD_VERSION "0.6.0.0"
// 0.6.0.0  spike   2016.08.14  moved this history inside the library
//                              went to a.b.c.d version
//                              added: pdfrasread_lib_version()
// 0.5  spike   2016.08.13  pdfrasread_recognize_source reports PDF/raster version
//                          new - error handling API.
// 0.4  spike	2016.07.21	first formal reporting of compliance failures
//							reader test fails down to 27.
// 0.3  spike	2016.07.19	minor API and internal improvements
// 0.2  spike	2016.07.13	revised PDF-raster marker in trailer dict.
// 0.1  spike	2015.02.11	1st version

// The highest major version of PDF/raster we can handle
#define RASREAD_MAX_MAJOR   1
// the highest minor version of the highest major version, that we can handle
#define RASREAD_MAX_MINOR   0

#define READER_SIGNATURE 0xD00D

#ifdef _DEBUG
#define CONFIGURATION "DEBUG"
#else
#define CONFIGURATION "RELEASE"
#endif

///////////////////////////////////////////////////////////////////////
// Data Structures & Types

// Cross-reference table entry
// This is the exact byte-aligned layout present in PDF files,
// a fact that the code relies on.
typedef struct t_xref_entry {
	char		offset[10];					// 10-digit byte offset
	char		gen[6];						// space + 5-digit generation number
	char		status[2];					// " n" (in use) or " f" (free)
	char		eol[2];                     // either <space>LF or CR,LF
} t_xref_entry;

// All the information about a single page and the image it contains
typedef struct {
	pduint32			off;				// offset of page object in file
	unsigned long		BitsPerComponent;
	double				MediaBox[4];
	RasterPixelFormat	format;
	double				whitePoint[3];
	double				blackPoint[3];
	unsigned long		width;
	unsigned long		height;
	unsigned long		rotation;
	double				xdpi, ydpi;
	int					strip_count;		// number of strips in this page
	size_t				max_strip_size;		// largest (raw) strip size
} t_pdfpageinfo;

// Structure that represents a PDF/raster byte-stream that is open for reading
typedef struct t_pdfrasreader {
    int                 sig;                // safety/validity signature
	int					apiLevel;			// caller's specified API level.
	pdfras_freader		fread;				// function to read from source
    pdfras_fsizer       fsize;              // function to get size of source
	pdfras_fcloser		fclose;				// function to close source
    pdfras_err_handler  error_handler;      // external error-reporting callback
	pdbool				bOpen;				// whether this reader is open
	void*				source;				// cookie/handle to caller-defined source
	pduint32			filesize;			// source size, in bytes
    int                 major, minor;       // level of PDF/raster claimed by source
	struct {
		char			data[1024];
		pduint32		off;
		size_t			len;
	}					buffer;
	// cross-reference table
	t_xref_entry*		xrefs;				// xref table (initially NULL)
	unsigned long		numxrefs;			// number of entries in xref table
	// page table
	long				page_count;			// actual page count, or -1 for 'unknown'
	pduint32*			page_table;			// table of page positions
} t_pdfrasreader;

///////////////////////////////////////////////////////////////////////
// Global (gasp!) variables

static pdfras_err_handler global_error_handler = pdfrasread_default_error_handler;

///////////////////////////////////////////////////////////////////////
// Functions

///////////////////////////////////////////////////////////////////////
// Utility Functions

#define VALID(p) ((p)!=NULL && (p)->sig==READER_SIGNATURE)

// Find last occurrence of string in string
static const char * strrstr(const char * haystack, const char * needle)
{
	const char *temp = haystack, *before = NULL;
	while ((temp = strstr(temp, needle))) before = temp++;
	return before;
}

static unsigned long ulmax(unsigned long a, unsigned long b)
{
	return (a >= b) ? a : b;
}

// Utility functions, do not require a reader object
//

static pduint32 pdfras_find_file_size(t_pdfrasreader* reader)
{
    if (!VALID(reader)) {
        global_error_handler(NULL, REPORTING_API, READ_API_BAD_READER, __LINE__);
        return 0;
    }
    return reader->fsize(reader->source);
} // pdfras_find_file_size

  // read the last len bytes, or as many as there are, from the reader->source.
  // Append a trailing NUL (so tail buffer's capacity must be at least len+1)
  // Returns the actual number of bytes read into the tail buffer.
static size_t pdfras_read_tail(t_pdfrasreader* reader, char* tail, size_t len)
{
	pduint32 off = pdfras_find_file_size(reader);
	off = (off < len) ? 0 : off - len;
	size_t step = reader->fread(reader->source, off, len, tail);
	// make sure it's NUL-terminated but remember it could contain embedded NULs.
	tail[step] = 0;
	return step;
}

int pdfras_parse_pdfr_tag(const char* tag, int* pmajor, int* pminor)
{
    assert(tag);
    if (pmajor) *pmajor = 0;
    if (pminor) *pminor = 0;
	if (tag[-1] != 0x0D && tag[-1] != 0x0A) {
		return FALSE;
	}
	tag += 12;
    {
        int major = 0, minor = 0;
        if (!isdigit(*tag)) return FALSE;
        while (isdigit(*tag)) {
            major = major * 10 + (*tag - '0');
            tag++;
        }
        if (*tag != '.') return FALSE;
        tag++;
        if (!isdigit(*tag)) return FALSE;
        while (isdigit(*tag)) {
            minor = minor * 10 + (*tag - '0');
            tag++;
        }
        if (*tag == 0x0D) {
            tag++;
            if (*tag == 0x0A) tag++;
        }
        else if (*tag == 0x0A) {
            tag++;
        }
        else {
            return FALSE;
        }
        if (pmajor) *pmajor = major;
        if (pminor) *pminor = minor;
    }
	return TRUE;
}

int pdfras_recognize_pdf_header(const void* sig)
{
	if (!sig) {
		return FALSE;
	}
	const char* p = (const char*)sig;
	if (0 != strncmp(p, "%PDF-1.", 7)) {
		return FALSE;
	}
	p += 7;
	if (!isdigit(*p)) return FALSE;
	while (isdigit(*p)) p++;
	if (*p == 0x0D) {
		p++;
		if (*p == 0x0A) p++;
	}
	else if (*p == 0x0A) {
		p++;
	}
	else {
		return FALSE;
	}
	return TRUE;
}

int pdfrasread_recognize_source(t_pdfrasreader* reader, void* source, int* pmajor, int* pminor)
{
	char buffer[1024];
    if (pmajor) *pmajor = -1;
    if (pminor) *pminor = -1;
    if (!VALID(reader)) {
        global_error_handler(NULL, REPORTING_API, READ_API_BAD_READER, __LINE__);
        return FALSE;
    }
    if (pdfrasread_is_open(reader)) {
        reader->error_handler(reader, REPORTING_API, READ_API_ALREADY_OPEN, 0);
        return FALSE;
    }
    reader->source = source;
	// check the header, it's quick & easy and we do need
	// to know that it's some kind of PDF, not just any file
	if (reader->fread(source, 0, 32, buffer) != 32) {
		return FALSE;
	}
	if (!pdfras_recognize_pdf_header(buffer)) {
        // not PDF
		return FALSE;
	}
    size_t tailsize = pdfras_read_tail(reader, buffer, sizeof buffer - 1);
    const char* tag = strrstr(buffer, "%PDF-raster-");
    if (!tag || tag == buffer) {
        return FALSE;
    }
    {
        int major = 0, minor = 0;
        if (!pdfras_parse_pdfr_tag(tag, &major, &minor)) {
            return FALSE;
        }
        if (pmajor) *pmajor = major;
        if (pminor) *pminor = minor;
        if (major < 1 || major > RASREAD_MAX_MAJOR) {
            return FALSE;
        }
    }
    return TRUE;
}

static void informational(t_pdfrasreader* reader, int code, pduint32 offset)
{
    if (VALID(reader)) {
        reader->error_handler(reader, REPORTING_INFO, code, offset);
    }
    else {
        global_error_handler(NULL, REPORTING_INFO, code, offset);
    }
}

static void warning(t_pdfrasreader* reader, int code, pduint32 offset)
{
    if (VALID(reader)) {
        reader->error_handler(reader, REPORTING_WARNING, code, offset);
    }
    else {
        global_error_handler(NULL, REPORTING_WARNING, code, offset);
    }
}

// report a failure to comply with the PDF/raster spec, at offset in file
static void compliance_error(t_pdfrasreader* reader, int code, pduint32 offset)
{
    if (VALID(reader)) {
        reader->error_handler(reader, REPORTING_COMPLIANCE, code, offset);
    }
    else {
        global_error_handler(NULL, REPORTING_COMPLIANCE, code, offset);
    }
}

// Low-level I/O functions

// Read the next buffer-full into the buffer, or up to EOF.
// Append a NUL.
// Set *poff to the offset in the file of the first byte in the buffer.
// If nothing read (at EOF) return FALSE, otherwise return TRUE.
static int advance_buffer(t_pdfrasreader* reader, pduint32* poff)
{
    // Compute file position of next byte after current buffer:
    *poff = reader->buffer.off + reader->buffer.len;
    // Read into buffer as much as will fit (with trailing NUL) or up to EOF:
    reader->buffer.len = reader->fread(reader->source, *poff, sizeof reader->buffer.data - 1, reader->buffer.data);
    // NUL-terminate the buffer
    reader->buffer.data[reader->buffer.len] = 0;
    // TRUE if something was read, FALSE if nothing read (presumably EOF)
    return reader->buffer.len != 0;
}

static int seek_to(t_pdfrasreader* reader, pduint32 off)
{
    if (off < reader->buffer.off || off >= reader->buffer.off + reader->buffer.len) {
        reader->buffer.off = off;
        reader->buffer.len = 0;
        if (!advance_buffer(reader, &off)) {
            assert(off = reader->buffer.off + reader->buffer.len);
            return FALSE;
        }
    }
    assert(off >= reader->buffer.off);
    assert(off < reader->buffer.off + reader->buffer.len);
    return TRUE;
}


///////////////////////////////////////////////////////////////////////
// Single-character scanning methods

// Return the character at the current file position.
// Return -1 if at EOF.
// Does not move the file position.
static int peekch(t_pdfrasreader* reader, pduint32 off)
{
	if (!seek_to(reader, off)) {
		return -1;
	}
	assert(off >= reader->buffer.off);
	assert(off < reader->buffer.off + reader->buffer.len);
	return reader->buffer.data[off - reader->buffer.off];
}

// Get the next character in the file.
// Return -1 if at EOF, otherwise
// increments the file position and returns the char at the new position.
static int nextch(t_pdfrasreader* reader, pduint32* poff)
{
	if (!seek_to(reader, *poff + 1)) {
		return -1;
	}
	++*poff;
	return peekch(reader, *poff);
}

// Advance *poff over any whitespace characters.
// Return FALSE if we end up at EOF (or have a read error)
// otherwise return TRUE.
// In EITHER CASE *poff is updated to skip over any whitespace chars.
static int skip_whitespace(t_pdfrasreader* reader, pduint32* poff)
{
	if (!seek_to(reader, *poff)) {
		return FALSE;
	}
	unsigned i = (*poff - reader->buffer.off);
	assert(i <= reader->buffer.len);
	while (TRUE) {
		if (i == reader->buffer.len) {
			if (!advance_buffer(reader, poff)) {
				// end of file
				return FALSE;
			}
			i = 0;
		}
		assert(i < reader->buffer.len);
		if (!isspace(reader->buffer.data[i])) {
			break;
		}
		// advance over whitespace character:
		i++; *poff += 1;
	}
	assert(i + reader->buffer.off == *poff);
	return TRUE;
}

///////////////////////////////////////////////////////////////////////
// Single token parsing methods

// TRUE if ch is a delimiter character per PDF, FALSE otherwise
static int isdelim(int ch)
{
	switch (ch) {
	case '(':
	case ')':
	case '<':
	case '>':
	case '[':
	case ']':
	case '{':
	case '}':
	case '/':
	case '%':
		return TRUE;
	default:
		return FALSE;
	}
}

static int token_skip(t_pdfrasreader* reader, pduint32* poff)
{
	// skip over whitespace
	if (!skip_whitespace(reader, poff)) {
		// EOF hit
		return FALSE;
	}
	// skip over non-whitespace stuff (roughly, 'a token')
	unsigned i = (*poff - reader->buffer.off);
	// skip_whitespace always leaves us looking at a valid (non-whitespace) character
	// If it can't, it returns FALSE which normally indicates EOF.
	assert(i < reader->buffer.len);
	// capture the starting char of the token
	char ch0 = reader->buffer.data[i];
	while (TRUE) {
		// accept current char, look at next
		i++;
		if (i == reader->buffer.len) {
			if (!advance_buffer(reader, poff)) {
				// end of file, end of token
				break;
			}
			i = 0;
		}
		assert(i < reader->buffer.len);
		// check for a 'token break'
		char ch = reader->buffer.data[i];
		if ('/' == ch0) {
			// A Name is a solidus followed by 'regular characters'
			// terminated by delimiter or whitespace
			if (isspace(ch) || isdelim(ch)) {
				break;
			}
		}
		else if (ch0 == ch && ('<' == ch0 || '>' == ch0)) {
			// we treat << and >> as the only double-delimiter token
			i++;
			break;
		}
		// for our purposes, we consider the delimiters to
		// be tokens, even though '(' for example actually starts a string token
		else if (isdelim(ch0)) {
			break;
		}
		// token started with a regular character
		else if (isspace(ch) || isdelim(ch)) {
			// so delim or whitespace ends it
			break;
		}
		// Not a token break, accept this char and continue.
	}
	*poff = i + reader->buffer.off;;
	// position offset at start of next token
	skip_whitespace(reader, poff);
	return TRUE;
}

// If the next token is the given literal string, skip over it (and following whitespace)
// and return TRUE.  Otherwise leave the offset at the start of the next token and
// return FALSE.  
static int token_match(t_pdfrasreader* reader, pduint32* poff, const char* lit)
{
	// TODO: doesn't handle comments
	char ch0 = *lit;
	// skip over whitespace
	if (!skip_whitespace(reader, poff)) {
		// EOF hit
		return FALSE;
	}
	unsigned i = (*poff - reader->buffer.off);
	assert(i <= reader->buffer.len);
	while (TRUE) {
		if (i == reader->buffer.len) {
			if (!advance_buffer(reader, poff)) {
				// end of file
				return FALSE;
			}
			i = 0;
		}
		assert(i < reader->buffer.len);
		// get the current char from the stream:
		char ch = reader->buffer.data[i];
		if (0 == *lit) {
			// end of string-to-match, check for a 'token break'
			if ('/' == ch0) {
				// Name: solidus followed by 'regular characters'
				// terminated by delimiter or whitespace
				if (isspace(ch) || isdelim(ch)) {
					break;
				}
			}
			else if ('<' == ch0 || '>' == ch0) {
				// assume the match was either to '<<' or '>>'
				// in which case it doesn't matter what follows.
				break;
			}
			// for our purposes, we consider the delimiters to
			// be tokens, even though '(' for example actually starts a string token
			else if (isdelim(ch0)) {
				break;
			}
			// token started with a regular character
			else if (isspace(ch) || isdelim(ch)) {
				// so delim or whitespace ends it
				break;
			}
			return FALSE;
		}
		if (*lit++ != ch) {
			return FALSE;
		}
		i++;
	}
	*poff = i + reader->buffer.off;
	skip_whitespace(reader, poff);
	return TRUE;
}

static int token_eol(t_pdfrasreader* reader, pduint32 *poff)
{
	int ch = peekch(reader, *poff);
	while (isspace(ch)) { ch = nextch(reader, poff); }
	// EOL is CR LF, CR or LF
	if (ch == 0x0D) {
		ch = nextch(reader, poff);
		if (ch == 0x0A) {
			nextch(reader, poff);
		}
	}
	else if (ch == 0x0A) {
		nextch(reader, poff);
	}
	else {
		// Not an EOL
		return FALSE;
	}
	return TRUE;
}

// Parse an unsigned long integer.
// Skips leading and trailing whitespace
static int token_ulong(t_pdfrasreader* reader, pduint32* poff, unsigned long *pvalue)
{
	int ch = peekch(reader, *poff);
	while (isspace(ch)) { ch = nextch(reader, poff); }
	*pvalue = 0;
	if (!isdigit(ch)) {
		return FALSE;
	}
	do {
		*pvalue = *pvalue * 10 + (ch - '0');
		ch = nextch(reader, poff);
	} while (isdigit(ch));
	while (isspace(ch)) { ch = nextch(reader, poff); }
	return TRUE;
}

// Try to parse a number token (inline)
// If successful, put the numeric value in *pdvalue, advance *poff and return TRUE.
// Ignores leading whitespace, and if successful skips over trailing whitespace.
// Otherwise leave *poff unchanged, set *pdvalue to 0 and return FALSE.
static int token_number(t_pdfrasreader* reader, pduint32 *poff, double* pdvalue)
{
	// ISO says: "...one or more decimal digits with an optional sign and a leading,
	// trailing, or embedded PERIOD (2Eh) (decimal point)."
	//
	pduint32 off = *poff;
	skip_whitespace(reader, &off);
	*pdvalue = 0.0;
	double intpart = 0.0, fraction = 0.0;
	int digits = 0, precision = 0;
	// parse leading sign, if any
	char sign = '+';
	char ch = peekch(reader, off);
	if (ch == '-' || ch == '+') { sign = ch; ch = nextch(reader, &off); }
	while (isdigit(ch)) {
		digits++;
		intpart = intpart * 10 + (ch - '0');
		ch = nextch(reader, &off);
	}
	if (ch == '.') {
		ch = nextch(reader, &off);
		while (isdigit(ch)) {
			fraction = fraction * 10 + (ch - '0');
			precision++;
			ch = nextch(reader, &off);
		}
	}
	if (digits + precision == 0) {
		// no digits, not a valid number
		assert(0.0 == *pdvalue);
		return FALSE;
	}
	*pdvalue = intpart + fraction * pow(10, -precision);
	if (sign == '-') {
		*pdvalue = -*pdvalue;
	}
	skip_whitespace(reader, &off);
	*poff = off;
	return TRUE;
}

// A literal string shall be written as an arbitrary number of characters enclosed in parentheses.
// Any characters may appear in a string except unbalanced parentheses (LEFT PARENHESIS (28h) and RIGHT PARENTHESIS (29h))
// and the backslash (REVERSE SOLIDUS (5Ch)), which shall be treated specially as described in this sub-clause.
// Balanced pairs of parentheses within a string require no special treatment.

static int token_literal_string(t_pdfrasreader* reader, pduint32* poff)
{
	pduint32 off = *poff;
	int ch = peekch(reader, off);
	if ('(' != ch) {
		return FALSE;
	}
	int nesting = 0;
	do {
		switch (ch) {
		case ')':
			--nesting;
			break;
		case '(':
			++nesting;
			break;
		case '\\':
			ch = peekch(reader, off + 1);
			if ('n' == ch || 'r' == ch || 't' == ch || 'b' == ch || 'f' == ch ||
				'(' == ch || ')' == ch || '\\' == ch) {
				// designated escapes, accept 2nd char as-is
				off++;
			}
			else if (ch >= '0' && ch <= '7') {
				// octal digit - accept it and up to 2 more
				off++; ch = peekch(reader, off + 1);
				if (ch >= '0' && ch <= '7') {
					// octal digit - accept it and up to 1 more
					off++; ch = peekch(reader, off + 1);
					if (ch >= '0' && ch <= '7') {
						// octal digit - accept it
						off++;
					}
				}
			}
			else {
				// not an escape sequence - ignore solidus
			}
			break;
		case -1:
			// invalid PDF: unexpected EOF in literal string
			return FALSE;
		default:
			break;
		} // switch
		ch = nextch(reader, &off);
	} while (nesting > 0);
	skip_whitespace(reader, &off);
	*poff = off;
	return TRUE;
}

static int token_hex_string(t_pdfrasreader* reader, pduint32* poff)
{
	pduint32 off = *poff;
	int ch = peekch(reader, off);
	if ('<' != ch) {
		return FALSE;
	}
	do {
		ch = nextch(reader, &off);
	} while ((ch >= '0' && ch <= '9') ||
		 (ch >= 'A' && ch <= 'F') ||
		 (ch >= 'a' && ch <= 'f') ||
		isspace(ch));
	if (ch != '>') {
		// unexpected character in hexadecimal string
		return FALSE;
	}
	nextch(reader, &off);
	skip_whitespace(reader, &off);
	*poff = off;
	return TRUE;
}

///////////////////////////////////////////////////////////////////////
// Xref table access

// Look up the indirect object (num,gen) in the cross-ref table and return its file position *pobjpos.
// Returns TRUE if successful, FALSE if not.
static int xref_lookup(t_pdfrasreader* reader, unsigned num, unsigned gen, pduint32 *pobjpos)
{
	if (gen != 0) {
		// not in PDF/raster
		return FALSE;
	}
	if (!reader->xrefs) {
		// internal error: no xref table loaded
		return FALSE;
	}
	if (num >= reader->numxrefs) {
		// invalid PDF: indirect object number is outside xref table
		return FALSE;
	}
	// parse the offset out of the indicated xref entry
	pduint32 off = strtoul(reader->xrefs[num].offset, NULL, 10);
	// parse & verify the start of the object definition, which should be <num> <gen> obj:
	unsigned long num2, gen2;
	if (!token_ulong(reader, &off, &num2) ||
		!token_ulong(reader, &off, &gen2) ||
		!token_match(reader, &off, "obj") ||
		num2 != num ||
		gen2 != gen) {
		// invalid PDF: xref table entry doesn't point to object definition
		return FALSE;
	}
	// got it, return the position of the stuff inside the object definition:
	*pobjpos = off;
	return TRUE;
}

///////////////////////////////////////////////////////////////////////
// object parsing methods

static int object_skip(t_pdfrasreader* reader, pduint32 *poff);
static int dictionary_lookup(t_pdfrasreader* reader, pduint32 off, const char* key, pduint32 *pvalpos);

// Parse an indirect reference and return the resolved file offset in *pobjpos.
// If successful returns TRUE (and advances *poff to point past the reference)
// If not, returns FALSE, *poff is not changed and *pobjpos is undefined.
static int parse_indirect_reference(t_pdfrasreader* reader, pduint32* poff, pduint32 *pobjpos)
{
	pduint32 off = *poff;
	unsigned long num, gen;
	if (token_ulong(reader, &off, &num) && token_ulong(reader, &off, &gen) && token_match(reader, &off, "R")) {
		// indirect object!
		// and we already parsed it.
		if (xref_lookup(reader, num, gen, pobjpos)) {
			*poff = off;
			return TRUE;
		}
		// invalid PDF - referenced object is not in cross-reference table
	}
	// indirect reference not found
	return FALSE;
}

// parse a direct OR indirect numeric object
// if successful place it's value in *pdvalue, update *poff and return TRUE.
// Otherwise set *pdvalue to 0, don't touch *poff and return FALSE.
static int parse_number_value(t_pdfrasreader* reader, pduint32 *poff, double* pdvalue)
{
	pduint32 off;
	// if indirect reference, skip over it
	if (parse_indirect_reference(reader, poff, &off)) {
		// and return the referenced numeric value (if any)
		return token_number(reader, &off, pdvalue);
	}
	else {
		// If it's not indirect, parse a numeric value in-line
		return token_number(reader, poff, pdvalue);
	}
}

// parse a direct OR indirect numeric value and round it to a long.
// if successful place it's value in *pvalue, update *poff and return TRUE.
// Otherwise set *pvalue to 0, don't touch *poff and return FALSE.
static int parse_long_value(t_pdfrasreader* reader, pduint32 *poff, long* pvalue)
{
	double dvalue;
	if (!parse_number_value(reader, poff, &dvalue)) {
		return FALSE;
	}
	*pvalue = (long)(dvalue + 0.5);
	return TRUE;
}

// TRUE if successful, FALSE otherwise
static int parse_dictionary(t_pdfrasreader* reader, pduint32 *poff)
{
	pduint32 off = *poff;
	if (!token_match(reader, &off, "<<")) {
		// dictionary malformed
		return FALSE;
	}
	while (!token_match(reader, &off, ">>")) {
		if ('/' != peekch(reader, off)) {
			// invalid PDF: dictionary key is not a Name
			return FALSE;
		}
		// step over key:
		if (!token_skip(reader, &off)) {
			// only fails at EOF
			return FALSE;
		}
		// skip over value:
		if (!object_skip(reader, &off)) {
			// invalid PDF: dictionary value is missing or malformed
			return FALSE;
		}
	}
	*poff = off;
	return TRUE;
}

// Parse a dictionary or stream.
// If successful, return TRUE and advance *poff over the object to the next token.
// Otherwise return FALSE and leave *poff unmoved.
// If a stream is found, set *pstream to the position of the stream data, and *plen to its length in bytes.
// Values are only returned through pstream or plen if they are non-NULL.
static int parse_dictionary_or_stream(t_pdfrasreader* reader, pduint32 *poff, pduint32 *pstream, long* plen)
{
	pduint32 off = *poff;
	if (!parse_dictionary(reader, &off)) {
		return FALSE;
	}
	if (peekch(reader, off + 0) != 's' ||
		peekch(reader, off + 1) != 't' ||
		peekch(reader, off + 2) != 'r' ||
		peekch(reader, off + 3) != 'e' ||
		peekch(reader, off + 4) != 'a' ||
		peekch(reader, off + 5) != 'm' ||
		!isspace(peekch(reader, off + 6))) {
		// Valid dictionary, but not a stream
		*poff = off;
		if (pstream) *pstream = 0;
		if (plen) *plen = 0;
		return TRUE;
	}
	off += 6;
	// must be followed by exactly CRLF or LF
	char ch = peekch(reader, off);
	if (ch == 0x0D) {
		// CR must be followed by LF
		if (nextch(reader, &off) != 0x0A) {
			return FALSE;
		}
	} else if (ch != 0x0A) {
		// Alternative to CRLF is just LF
		return FALSE;
	}
	// we're positioned at the LF, step over it.
	off++;
	pduint32 lenpos;
	if (!dictionary_lookup(reader, *poff, "/Length", &lenpos)) {
		// invalid stream: no /Length key in stream dictionary
		return FALSE;
	}
	long length;
	if (!parse_long_value(reader, &lenpos, &length)) {
		return FALSE;
	}
	// we've found position and length of stream data:
	if (pstream) *pstream = off;
	if (plen) *plen = length;
	// step over stream data
	off += length;
	// Step over 'endstream' keyword.
	if (!token_match(reader, &off, "endstream")) {
		// invalid stream: 'endstream' not found where expected
		return FALSE;
	}
	*poff = off;
	return TRUE;
}

static int parse_array(t_pdfrasreader* reader, pduint32 *poff)
{
	skip_whitespace(reader, poff);
	if (peekch(reader, *poff) != '[') {
		// not a valid array
		return FALSE;
	}
	// skip over the opening '['
	if (nextch(reader, poff) < 0) {
		return FALSE;
	}
	while (!token_match(reader, poff, "]")) {
		if (!object_skip(reader, poff)) {
			// invalid array - unparseable element
			return FALSE;
		}
	}
	return TRUE;
}

static int parse_color_space(t_pdfrasreader* reader, pduint32 *poff, unsigned long stripno, t_pdfpageinfo *info)
{
	// TODO: If stripno == 0, colorspace info should be undefined, ...
	// If stripno != 0, colorspace info must match what's already set in info
	if (token_match(reader, poff, "/DeviceGray")) {
		switch (info->BitsPerComponent) {
		case 1:
			info->format = RASREAD_BITONAL;
			break;
		case 8:
			info->format = RASREAD_GRAY8;
			break;
		case 16:
			info->format = RASREAD_GRAY16;
			break;
		default:
			// PDF/raster: /DeviceGray image must be 1, 8 or 16 BitsPerComponent
			return FALSE;
		}
	}
	else if (token_match(reader, poff, "/DeviceRGB")) {
		switch (info->BitsPerComponent) {
		case 8:
			info->format = RASREAD_RGB24;
			break;
		case 16:
			info->format = RASREAD_RGB48;
			break;
		default:
			// PDF/raster: /DeviceRGB image must be 8 or 16 BitsPerComponent
			return FALSE;
		}
	}
	else if (token_match(reader, poff, "[")) {
		if (token_match(reader, poff, "/CalGray")) {
			// calibrated grayscale
			switch (info->BitsPerComponent) {
			case 1:
				info->format = RASREAD_BITONAL;
				break;
			case 8:
				info->format = RASREAD_GRAY8;
				break;
			case 16:
				info->format = RASREAD_GRAY16;
				break;
			default:
				// PDF/raster: /DeviceGray image must be 1, 8 or 16 BitsPerComponent
				return FALSE;
			}
		}
		else if (token_match(reader, poff, "/ICCBased")) {
			switch (info->BitsPerComponent) {
			case 8:
				info->format = RASREAD_RGB24;
				break;
			case 16:
				info->format = RASREAD_RGB48;
				break;
			default:
				// PDF/raster: /ICCBased image must be 8 or 16 BitsPerComponent
				return FALSE;
			}
		}
		else {
			// missing or unrecognized colorspace type
			return FALSE;
		}
	}
	else {
		// PDF/raster: ColorSpace must be /DeviceGray or /DeviceRGB
		return FALSE;
	}
	return TRUE;
}

static int parse_media_box(t_pdfrasreader* reader, pduint32 *poff, double mediabox[4])
{
	skip_whitespace(reader, poff);
	pduint32 off = *poff;
	if (peekch(reader, off) != '[') {
        // invalid PDF: bad MediaBox
        compliance_error(reader, READ_MEDIABOX_ARRAY, off);
        return FALSE;
	}
	// skip over the opening '['
	if (nextch(reader, &off) < 0) {
        // invalid PDF: bad MediaBox
        compliance_error(reader, READ_MEDIABOX_ARRAY, off);
        return FALSE;
	}
	int i;
	for (i = 0; i < 4; i++) {
		if (!parse_number_value(reader, &off, &mediabox[i])) {
			// invalid PDF: bad MediaBox element
            compliance_error(reader, READ_MEDIABOX_ELEMENTS, off);
            return FALSE;
		}
	}
	if (!token_match(reader, &off, "]")) {
		// invalid PDF: MediaBox array has more than 4 elements
        compliance_error(reader, READ_MEDIABOX_ARRAY, off);
        return FALSE;
	}
    if (mediabox[0] != 0 || mediabox[1] != 0) {
        compliance_error(reader, READ_MEDIABOX_ELEMENTS, off);
        return FALSE;
    }
    if (mediabox[3] < 0 || mediabox[4] < 0) {
        compliance_error(reader, READ_MEDIABOX_ELEMENTS, off);
        return FALSE;
    }
    // normalize rectangle so lower-left corner is actually lower and left etc.
	if (mediabox[0] > mediabox[2]) {
		double t = mediabox[0]; mediabox[0] = mediabox[2]; mediabox[2] = t;
	}
	if (mediabox[1] > mediabox[3]) {
		double t = mediabox[1]; mediabox[1] = mediabox[3]; mediabox[3] = t;
	}
	*poff = off;
	return TRUE;
}


static int object_skip(t_pdfrasreader* reader, pduint32 *poff)
{
	pduint32 off = *poff;
	int ch = peekch(reader, off);
	if (-1 == ch) {
		// at EOF
		return FALSE;
	}
	if (isalpha(ch) ||
		'/' == ch ||
		'-' == ch || '+' == ch) {
		// keyword or Name or signed number
		return token_skip(reader, poff);
	}
	if ('(' == ch) {
		// string object
		return token_literal_string(reader, poff);
	}
	if ('<' == ch) {
		if ('<' == nextch(reader, &off)) {
			return parse_dictionary_or_stream(reader, poff, NULL, NULL);
		}
		else {
			return token_hex_string(reader, poff);
		}
	}
	if ('[' == ch) {
		return parse_array(reader, poff);
	}
	if (isdigit(ch)) {
		unsigned long num, gen;
		if (token_ulong(reader, &off, &num) && token_ulong(reader, &off, &gen) && token_match(reader, &off, "R")) {
			// indirect object!
			// and we already parsed it.
			*poff = off;
			return TRUE;
		}
		else {
			// jus' a plain ol' number, skip over it.
			return token_skip(reader, poff);
		}
	}
	// Don't know what the h-e-double-hockey-sticks it is.
	return FALSE;
}

// Given a dictionary inline at pos, look up the specified key and return the file position of its value element.
static int dictionary_lookup(t_pdfrasreader* reader, pduint32 off, const char* key, pduint32 *pvalpos)
{
	*pvalpos = 0;
	if (!token_match(reader, &off, "<<")) {
		// invalid dictionary
		return FALSE;
	}
	while (!token_match(reader, &off, ">>")) {
		// does the key element match the key we're looking for?
		if (token_match(reader, &off, key)) {
			// yes, bingo.
			// check for indirect reference
			unsigned long num, gen;
			pduint32 p = off;
			if (token_ulong(reader, &p, &num) && token_ulong(reader, &p, &gen) && token_match(reader, &p, "R")) {
				// indirect object!
				// and we already parsed it.
				if (!xref_lookup(reader, num, gen, &off)) {
					// invalid PDF - referenced object is not in cross-reference table
					return FALSE;
				}
			}
			*pvalpos = off;
			return TRUE;
		} // otherwise skip over and ignore key
		else if (!token_skip(reader, &off)) {
			// only fails at EOF
			return FALSE;
		}
		// skip over value element
		if (!object_skip(reader, &off)) {
			// invalid dictionary (invalid value)
			return FALSE;
		}
	}
	// key not found in dictionary
	return FALSE;
}

// Parse the trailer dictionary.
// TRUE if successful, FALSE otherwise
static int read_trailer_dict(t_pdfrasreader* reader, pduint32 *poff)
{
	if (!token_match(reader, poff, "trailer")) {
		// PDF/raster restriction: trailer dictionary does not follow xref table.
		return FALSE;
	}
	return parse_dictionary(reader, poff);
}

// check an xref table for anything invalid
// return TRUE if valid, FALSE otherwise.
static int validate_xref_table(t_xref_entry* xrefs, unsigned long numxrefs)
{
	unsigned long e;
	// Sweep the xref table, validate entries.
	for (e = 0; e < numxrefs; e++) {
		char *offend, *genend;
		pduint32 offset = strtoul(xrefs[e].offset, &offend, 10);
		unsigned long gen = strtoul(xrefs[e].gen, &genend, 10);
		// Note, we don't check for leading 0's on offset or gen.
		if (offend != xrefs[e].gen ||
			genend != xrefs[e].status ||
			(xrefs[e].eol[0] != ' ' && xrefs[e].eol[0] != 0x0D) ||
			(xrefs[e].eol[0] != 0x0D && xrefs[e].eol[1] != 0x0A) ||
			xrefs[e].gen[0] != ' ' ||
			xrefs[e].status[0] != ' ' ||
			(xrefs[e].status[1] != 'n' && xrefs[e].status[1] != 'f')) {
			// invalid xref table entry
			return FALSE;
		}
		if (e == 0) {
			if (xrefs[e].status[1] != 'f' || gen != 65535) {
				// object 0 must be free with gen=65535
				return FALSE;
			}
		}
		else {
			if (gen != 0 && xrefs[e].status[1] != 'f') {
				// PDF/raster restriction: in-use object generation must be 0
				// (free entries can have gen != 0)
				return FALSE;
			}
		}
	}
	return TRUE;
}

// Parse and load the xref table from given offset within the file.
// Returns TRUE if successful, FALSE for any error.
static int read_xref_table(t_pdfrasreader* reader, pduint32* poff)
{
	pduint32 off = *poff;
	unsigned long firstnum, numxrefs;
	t_xref_entry* xrefs = NULL;
	if (!token_match(reader, &off, "xref")) {
		// invalid xref table
		return FALSE;
	}
	// NB: token_match skips over the whitespace (eol) after "xref"
	if (!token_ulong(reader, &off, &firstnum) || !token_ulong(reader, &off, &numxrefs)) {
		// invalid xref table
		return FALSE;
	}
	// And token_ulong skips over trailing whitespace (eol) after 2nd header line
	if (firstnum != 0) {
		// invalid PDF/raster: xref table does not start with object 0.
		return FALSE;
	}
	if (numxrefs < 1 || numxrefs > 8388607) {
		// looks invalid, at least per PDF 32000-1:2008
		return FALSE;
	}
	size_t xref_size = 20 * numxrefs;
	xrefs = (t_xref_entry*)malloc(xref_size);
	if (!xrefs) {
		// allocation failed
		return FALSE;
	}
	// Read all the xref entries straight into memory structure
	// (PDF specifically designed for this)
	if (reader->fread(reader->source, off, xref_size, (char*)xrefs) != xref_size) {
		// invalid PDF, the xref table is cut off
		free(xrefs);
		return FALSE;
	}
	off += xref_size;
	if (!validate_xref_table(xrefs, numxrefs)) {
		free(xrefs);
		return FALSE;
	}
	// OK, attach xref table to reader object:
	reader->xrefs = xrefs;
	reader->numxrefs = numxrefs;
	// update caller's file position
	*poff = off;
	return TRUE;
}

// Find all the pages in the page tree rooted at off, ppn points to next page index value.
// Store each page's file position (indexed by page#) in the page table,
// increment *ppn by the number of pages found.
static int recursive_page_finder(t_pdfrasreader* reader, pduint32 off, pduint32* table, int *ppn)
{
	pduint32 p;
	assert(reader);
	assert(table);
	assert(ppn);
	assert(*ppn >= 0);

	// look for the Type key
	if (!dictionary_lookup(reader, off, "/Type", &p)) {
		// invalid PDF: page tree node is not a dictionary or lacks a /Type entry
		return FALSE;
	}
	// is it a page (leaf) node?
	if (token_match(reader, &p, "/Page")) {
		// Found a page object!
		if (*ppn >= reader->page_count) {
			// invalid PDF: more page objects than expected in page tree
			return FALSE;
		}
		// record page object's position, in the page table:
		table[*ppn] = off;
		*ppn += 1;					// increment the page counter
		return TRUE;
	}
	// is the Type value right for a page tree node?
	if (!token_match(reader, &p, "/Pages")) {
		// invalid PDF: page tree node /Type is not /Pages
		return FALSE;
	}
	pduint32 kids;
	if (!dictionary_lookup(reader, off, "/Kids", &kids)) {
		// invalid PDF: page tree node lacks a /Kids entry
		return FALSE;
	}
	// Enumerate the kids adding their counts to *pcount
	if (!token_match(reader, &kids, "[")) {
		return FALSE;
	}
	pduint32 kid;
	while (parse_indirect_reference(reader, &kids, &kid)) {
		if (!recursive_page_finder(reader, kid, table, ppn)) {
			// invalid PDF, bad 'kid' entry in page tree
			return FALSE;
		}
	}
	if (!token_match(reader, &kids, "]")) {
		// invalid PDF, expected ']' at end of 'kids' array
		return FALSE;
	}
	return TRUE;
}

// Build the page table by walking the page tree from root.
// If successful, return TRUE: page table contains offset of each page object.
// Otherwise return FALSE;
static int build_page_table(t_pdfrasreader* reader, pduint32 root)
{
	assert(reader);
	assert(NULL==reader->page_table);
	assert(root > 0);
	assert(reader->page_count >= 0);

	// allocate a page table
	pduint32* pages;
	size_t ptsize = reader->page_count * sizeof *pages;
	pages = (pduint32*)malloc(ptsize);
	if (!pages) {
		// internal failure, mmemory allocation
		return FALSE;
	}
	memset(pages, 0, ptsize);

	int pageno = 0;
	if (!recursive_page_finder(reader, root, pages, &pageno)) {
		// error
		free(pages);				// free the page table
		return FALSE;
	}
	if (pageno != reader->page_count) {
		// invalid PDF: /Count in root page node is not correct
		free(pages);				// free the page table
		return FALSE;
	}
	// keep the filled-in page table
	reader->page_table = pages;
	return TRUE;
}

// Return TRUE if all OK, FALSE if some problem.
static int parse_trailer(t_pdfrasreader* reader)
{
	reader->filesize = pdfras_find_file_size(reader);
	char tail[1024+1];
	size_t tailsize = pdfras_read_tail(reader, tail, sizeof tail - 1);
    pduint32 off = reader->filesize - tailsize;
    const char* eof = strrstr(tail, "%%EOF");
    if (!eof) {
        // invalid PDF - %%EOF not found in tail of file.
        compliance_error(reader, READ_FILE_EOF_MARKER, off);
        return FALSE;
    }
    // we need to find the startxref anyway, let's check now,
    // it's a good check for a valid PDF.
    const char* startxref = strrstr(tail, "startxref");
    if (!startxref) {
        // invalid PDF - startxref not found in tail of file.
        compliance_error(reader, READ_FILE_STARTXREF, off);
        return FALSE;
    }
    // find the PDF/raster 'tag'
    const char* tag = strrstr(tail, "%PDF-raster-");
    if (!tag || tag == tail) {
        // PDF/raster marker not found in tail of file
        compliance_error(reader, READ_FILE_PDFRASTER_TAG, off);
        return FALSE;
    }
    // found the %PDF-raster tag
    off += tag - tail;
    if (!pdfras_parse_pdfr_tag(tag, &reader->major, &reader->minor) ||
        reader->major < 1 ||
        reader->minor < 0) {
        compliance_error(reader, READ_FILE_BAD_TAG, off);
		return FALSE;
	}
    // point specifically to the x.y part of the tag
    off += 12;
    if (reader->major > RASREAD_MAX_MAJOR) {
        // beyond us, we can't handle it.
        compliance_error(reader, READ_FILE_TOO_MAJOR, off);
        return FALSE;
    }
    if (reader->major == RASREAD_MAX_MAJOR && reader->minor > RASREAD_MAX_MINOR) {
        // minor version is above what we understand - supposedly that
        // means some nonessential new features may not work.
        warning(reader, READ_FILE_TOO_MINOR, off);
        return FALSE;
    }
    // go back to the whole tail thing for a sec...
    off = reader->filesize - tailsize;
	// Calculate the file position of the "startxref" keyword
	// and make a note of it for a bit later.
	pduint32 startxref_off = off += (startxref - tail);
	unsigned long xref_off;
	if (!token_match(reader, &off, "startxref") || !token_ulong(reader, &off, &xref_off)) {
		// startxref not followed by unsigned int
        compliance_error(reader, READ_FILE_BAD_STARTXREF, off);
		return FALSE;
	}
	if (xref_off < 16 || xref_off >= reader->filesize) {
		// invalid PDF - offset to xref table is bogus
        compliance_error(reader, READ_FILE_BAD_STARTXREF, off);
        return FALSE;
	}
	// go there and read the xref table
	off = xref_off;
	if (!read_xref_table(reader, &off)) {
		// xref table not found or not valid
		return FALSE;
	}
	if (!token_match(reader, &off, "trailer")) {
		// PDF/raster restriction: trailer dictionary does not follow xref table.
		return FALSE;
	}
	// find the address of the Catalog
	pduint32 catpos;
	if (!dictionary_lookup(reader, off, "/Root", &catpos)) {
		// invalid PDF: trailer dictionary must contain /Root entry
		return FALSE;
	}
	// check the Catalog
	off = catpos;
	pduint32 p;
	if (!dictionary_lookup(reader, off, "/Type", &p)) {
		// invalid PDF: catalog must have /Type /Catalog
		return FALSE;
	}
	if (!token_match(reader, &p, "/Catalog")) {
		// invalid PDF: catalog must have /Type /Catalog
		return FALSE;
	}
	// Find the root node of the page tree
	pduint32 pages;
	if (!dictionary_lookup(reader, off, "/Pages", &pages)) {
		// invalid PDF: catalog must have a /Pages entry
		return FALSE;
	}
	// pages points to the root Page Tree Node
	off = pages;
	if (!dictionary_lookup(reader, pages, "/Count", &off) || !parse_long_value(reader, &off, &reader->page_count)) {
		// invalid PDF: root page node does not have valid /Count value
		return FALSE;
	}
	// walk the page tree locating all the pages
	if (!build_page_table(reader, pages)) {
		// oops - something went wrong
		return FALSE;
	}

	return TRUE;
}

static pduint32 get_page_pos(t_pdfrasreader* reader, int n)
{
	assert(reader);
	if (n < 0 || n >= pdfrasread_page_count(reader)) {
		// invalid page number
		return 0;
	}
	assert(reader->page_table);
	return reader->page_table[n];
}

// round a dpi value to an exact integer, if it's already 'really close'
static double tweak_dpi(double dpi)
{
	// I don't care what happens if dpi is negative. Don't do that!
	if (dpi+0.5 < LONG_MAX) {
		long ndpi = (long)(0.5 + dpi);
		double err = fabs(dpi - ndpi);
		if (err * 100000 < dpi) {
			dpi = ndpi;
		}
	}
	return dpi;
}

static int decode_strip_format(t_pdfrasreader* reader, pduint32* poff, unsigned long bpc, RasterPixelFormat* pformat)
{
	return TRUE;
}

static int get_page_info(t_pdfrasreader* reader, int p, t_pdfpageinfo* pinfo)
{
	if (!reader || !pinfo) {
		// TODO: internal error
		return FALSE;
	}
	// clear info to all 0's
	memset(pinfo, 0, sizeof *pinfo);
	// If we haven't 'opened' the file, do the initial stuff now
	if (!reader->xrefs && !parse_trailer(reader)) {
		return FALSE;
	}
	// look up the file position of the nth page object:
	pduint32 page = get_page_pos(reader, p);
	if (!page) {
		// TODO: internal error
		return FALSE;
	}
	pinfo->off = page;
	pduint32 val;
	if (!dictionary_lookup(reader, page, "/Type", &val) || !token_match(reader, &val, "/Page")) {
		// bad page object, not marked /Type /Page
		compliance_error(reader, READ_PAGE_TYPE, page);
		return FALSE;
	}
	// a page may be rotated for rendering, by a non-negative
	// multiple of 90 degrees (clockwise).
	// note: if not present defaults to 0.
	if (dictionary_lookup(reader, page, "/Rotate", &val)) {
		unsigned long angle;
		if (!token_ulong(reader, &val, &angle) ||
			angle % 90 != 0) {
			compliance_error(reader, READ_PAGE_ROTATION, val);
			return FALSE;
		}
		pinfo->rotation = angle % 360;
	}
	// similarly for mediabox
	if (!dictionary_lookup(reader, page, "/MediaBox", &val)) {
		compliance_error(reader, READ_PAGE_MEDIABOX, page);
		return FALSE;
	}
    if (!parse_media_box(reader, &val, pinfo->MediaBox)) {
        return FALSE;
    }
    pduint32 resdict;
	if (!dictionary_lookup(reader, page, "/Resources", &resdict)) {
		// bad page object, no /Resources entry
		compliance_error(reader, READ_RESOURCES, page);
		return FALSE;
	}
	// In the Resources dictionary find the XObject dictionary
	pduint32 xobjects;
	if (!dictionary_lookup(reader, resdict, "/XObject", &xobjects)) {
		// bad resource dictionary, no /XObject entry
		compliance_error(reader, READ_XOBJECT, resdict);
		return FALSE;
	}
	// Traverse the XObject dictionary collecting strip info
	if (!token_match(reader, &xobjects, "<<")) {
		// invalid PDF: XObject dictionary doesn't start with '<<'
		compliance_error(reader, READ_XOBJECT_DICT, xobjects);
		return FALSE;
	}
	int n;				// strip no
	for (n = 0; !token_match(reader, &xobjects, ">>"); n++) {
		pduint32 xobj_entry = xobjects;
		if (peekch(reader, xobjects) != '/' ||
			nextch(reader, &xobjects) != 's' ||
			nextch(reader, &xobjects) != 't' ||
			nextch(reader, &xobjects) != 'r' ||
			nextch(reader, &xobjects) != 'i' ||
			nextch(reader, &xobjects) != 'p' ||
			!isdigit(nextch(reader, &xobjects))
			) {
			// illegal entry in xobjects dictionary - only /strip<n> allowed
			compliance_error(reader, READ_XOBJECT_ENTRY, xobj_entry);
			return FALSE;
		}
		unsigned long stripno;
		if (!token_ulong(reader, &xobjects, &stripno)) {
			// PDF/raster: strips must be named /strip0, /strip1, /strip2, etc.
			compliance_error(reader, READ_XOBJECT_ENTRY, xobjects);
			return FALSE;
		}
		pduint32 strip;
		if (!parse_indirect_reference(reader, &xobjects, &strip)) {
			// invalid PDF: strip entry in XObject dict doesn't point to strip stream
			compliance_error(reader, READ_NOT_STRIP_REF, xobj_entry);
			return FALSE;
		}
		if (!dictionary_lookup(reader, strip, "/Subtype", &val) || !token_match(reader, &val, "/Image")) {
			// strip isn't an image?
			compliance_error(reader, READ_STRIP_SUBTYPE, strip);
			return FALSE;
		}
		if (!dictionary_lookup(reader, strip, "/BitsPerComponent", &val) || !token_ulong(reader, &val, &pinfo->BitsPerComponent)) {
			// strip doesn't have BitsPerComponent?
			compliance_error(reader, READ_BITSPERCOMPONENT, strip);
			return FALSE;
		}
		unsigned long strip_height;
		if (!dictionary_lookup(reader, strip, "/Height", &val) || !token_ulong(reader, &val, &strip_height)) {
			// strip doesn't have /Height with non-negative integer value
			compliance_error(reader, READ_STRIP_HEIGHT, strip);
			return FALSE;
		}
		// page height is sum of strip heights
		pinfo->height += strip_height;
		// get/check strip width
		unsigned long width;
		if (!dictionary_lookup(reader, strip, "/Width", &val) || !token_ulong(reader, &val, &width)) {
			// strip doesn't have Width?
			compliance_error(reader, READ_STRIP_WIDTH, strip);
			return FALSE;
		}
		if (stripno == 0) {
			pinfo->width = width;
		}
		else {
			// all strips on a page must have the same width
			// TODO: report error instead of assert
			assert(pinfo->width == width);
		}
		if (!dictionary_lookup(reader, strip, "/ColorSpace", &val)) {
			// PDF/raster: image object, each strip must have a named ColorSpace
			compliance_error(reader, READ_STRIP_COLORSPACE, strip);
			return FALSE;
		}
		if (!parse_color_space(reader, &val, stripno, pinfo)) {
			// PDF/raster: invalid color space in strip
			compliance_error(reader, READ_INVALID_COLORSPACE, val);
			return FALSE;
		}
		// max_strip_size is (surprise) the maximum of the strip sizes (in bytes)
		long strip_size;
		if (!dictionary_lookup(reader, strip, "/Length", &val) || !parse_long_value(reader, &val, &strip_size)) {
			// invalid PDF: strip image must have /Length
			compliance_error(reader, READ_STRIP_LENGTH, strip);
			return FALSE;
		}
		pinfo->max_strip_size = ulmax(pinfo->max_strip_size, (unsigned long)strip_size);
		// found a valid strip, count it
		pinfo->strip_count++;
	} // for each strip
	// we have MediaBox and pixel dimensions, we can calculate DPI
	pinfo->xdpi = tweak_dpi(pinfo->width * 72.0 / (pinfo->MediaBox[2] - pinfo->MediaBox[0]));
	pinfo->ydpi = tweak_dpi(pinfo->height * 72.0 / (pinfo->MediaBox[3] - pinfo->MediaBox[1]));
	return TRUE;
}

static int find_strip(t_pdfrasreader* reader, int p, int s, pduint32* pstrip)
{
    char stripname[32];

    sprintf(stripname, "/strip%d", s);

    // look up the file position of the nth page object:
    pduint32 page = get_page_pos(reader, p);
    if (!page) {
        return FALSE;
    }
    pduint32 resdict;
    if (!dictionary_lookup(reader, page, "/Resources", &resdict)) {
        // bad page object, no /Resources entry
        return FALSE;
    }
    // In the Resources dictionary find the XObject dictionary
    pduint32 xobjects;
    if (!dictionary_lookup(reader, resdict, "/XObject", &xobjects)) {
        // bad resource dictionary, no /XObject entry
        return FALSE;
    }
    if (!dictionary_lookup(reader, xobjects, stripname, pstrip)) {
        // strip not found in XObject dictionary
        return FALSE;
    }
    return TRUE;
}

// internal function that just passes an error through the (settable) global error handler.
static int call_global_error_handler(t_pdfrasreader* reader, int level, int code, pduint32 offset)
{
    return global_error_handler(reader, level, code, offset);
}

///////////////////////////////////////////////////////////////////////
// Top-Level Public Functions

// Create a PDF/raster reader
t_pdfrasreader* pdfrasread_create(int apiLevel, pdfras_freader readfn, pdfras_fsizer sizefn, pdfras_fcloser closefn)
{
	if (apiLevel < 1) {
		// error, invalid parameter value
        global_error_handler(NULL, REPORTING_API, READ_API_APILEVEL, (pduint32)apiLevel);
        return NULL;
	}
	if (apiLevel > RASREAD_API_LEVEL) {
		// error, caller expects a future version of this API
        global_error_handler(NULL, REPORTING_API, READ_API_APILEVEL, (pduint32)apiLevel);
        return NULL;
	}
	// some internal consistency checks
	if (20 != sizeof(t_xref_entry)) {
		// compilation/build error: xref entry is not exactly 20 bytes.
        global_error_handler(NULL, REPORTING_INTERNAL, READ_INTERNAL_XREF_SIZE, sizeof(t_xref_entry));
        return NULL;
	}
	t_pdfrasreader* reader = (t_pdfrasreader*)malloc(sizeof(t_pdfrasreader));
    if (!reader) {
        global_error_handler(NULL, REPORTING_MEMORY, READ_MEMORY_MALLOC, sizeof(t_xref_entry));
        return NULL;
    }
    memset(reader, 0, sizeof *reader);
    reader->sig = READER_SIGNATURE;
    reader->apiLevel = apiLevel;
    reader->fread = readfn;
    reader->fsize = sizefn;
    reader->fclose = closefn;
    reader->error_handler = call_global_error_handler;
    reader->page_count = -1;		// Unknown
    assert(VALID(reader));
	return reader;
}

void pdfrasread_destroy(t_pdfrasreader* reader)
{
    if (!VALID(reader)) {
        global_error_handler(NULL, REPORTING_API, READ_API_BAD_READER, __LINE__);
    } else {
		if (reader->page_table) {
			free(reader->page_table);
		}
		if (reader->xrefs) {
			free(reader->xrefs);
		}
        reader->sig = 0xDEAD;
		free(reader);
	}
}

const char* pdfrasread_lib_version(void)
{
    return PDFRASREAD_VERSION " (" CONFIGURATION ") ";
}

// Return the number of pages in the associated PDF/raster file
// -1 in case of error.
int pdfrasread_page_count(t_pdfrasreader* reader)
{
    if (!VALID(reader)) {
        global_error_handler(NULL, REPORTING_API, READ_API_BAD_READER, __LINE__);
        return -1;
    }
    if (!reader->xrefs) {
        if (!parse_trailer(reader)) {
            return -1;
        }
    }
    return reader->page_count;
}

// Return the pixel format of the raster image of page n (indexed from 0)
RasterPixelFormat pdfrasread_page_format(t_pdfrasreader* reader, int n)
{
    t_pdfpageinfo info;
    if (!get_page_info(reader, n, &info)) {
        return RASREAD_FORMAT_NULL;
    }
    return info.format;
}

// Return the pixel width of the raster image of page n
int pdfrasread_page_width(t_pdfrasreader* reader, int n)
{
    t_pdfpageinfo info;
    if (!get_page_info(reader, n, &info)) {
        return 0;
    }
    return info.width;
}

// Return the pixel height of the raster image of page n
int pdfrasread_page_height(t_pdfrasreader* reader, int n)
{
    t_pdfpageinfo info;
    if (!get_page_info(reader, n, &info)) {
        return 0;
    }
    return info.height;
}

// Return the clockwise rotation in degrees to be applied to page n
int pdfrasread_page_rotation(t_pdfrasreader* reader, int n)
{
    t_pdfpageinfo info;
    if (!get_page_info(reader, n, &info)) {
        return 0;
    }
    return info.rotation;
}

// Get the resolution in dpi of the raster image of page n
double pdfrasread_page_horizontal_dpi(t_pdfrasreader* reader, int n)
{
    t_pdfpageinfo info;
    if (!get_page_info(reader, n, &info)) {
        return 0.0;
    }
    return info.xdpi;
}

double pdfrasread_page_vertical_dpi(t_pdfrasreader* reader, int n)
{
    t_pdfpageinfo info;
    if (!get_page_info(reader, n, &info)) {
        return 0.0;
    }
    return info.ydpi;
}

// Strip reading functions
// Return the number of strips in page p
int pdfrasread_strip_count(t_pdfrasreader* reader, int p)
{
    t_pdfpageinfo info;
    if (!get_page_info(reader, p, &info)) {
        return 0;
    }
    return info.strip_count;
}

// Return the maximum raw (compressed) strip size on page p
size_t pdfrasread_max_strip_size(t_pdfrasreader* reader, int p)
{
    t_pdfpageinfo info;
    if (!get_page_info(reader, p, &info)) {
        return 0;
    }
    return info.max_strip_size;
}

// Read the raw (compressed) data of strip s on page p into buffer, not more than bufsize bytes.
// Returns the actual number of bytes read.
// A return value of 0 indicates an error.
size_t pdfrasread_read_raw_strip(t_pdfrasreader* reader, int p, int s, void* buffer, size_t bufsize)
{
    if (!VALID(reader)) {
        global_error_handler(NULL, REPORTING_API, READ_API_BAD_READER, __LINE__);
        return 0;
    }
    // find the strip
    pduint32 strip;
    if (!find_strip(reader, p, s, &strip)) {
        // invalid strip request
        return 0;
    }
    // Parse the strip stream and find the position & length of its data
    pduint32 pos;
    long length;
    if (!parse_dictionary_or_stream(reader, &strip, &pos, &length) || pos == 0 || length == 0) {
        // strip stream not found or invalid
        return 0;
    }
    if ((unsigned)length > bufsize) {
        // invalid strip request, strip does not fit in buffer
        return 0;
    }
    if (reader->fread(reader->source, pos, length, buffer) != length) {
        // read error, unable to read all of strip data
        return 0;
    }
    return length;
}

int pdfrasread_default_error_handler(t_pdfrasreader* reader, int level, int code, pduint32 offset)
{
    const char* levelName[] = {
        "INFORMATIONAL",        // useful to know but not bad news.
        "WARNING",              // a potential problem - but execution can continue.
        "COMPLIANCE",           // a violation of the PDF/raster specification was detected.
        "INV API CALL",		    // an invalid request was made to this API.
        "MEMORY ALLOC",		    // memory allocation failed.
        "I/O ERROR",		    // low-level read or write failed unexpectedly.
        "INTERNAL LIMIT",		// a built-in limitation of this library was exceeded.
        "INTERNAL ERROR",	    // an 'impossible' internal state has been detected.
        "OTHER FATAL"		    // none of the above, current API call fails.
    };
    char marker = '*';      // for errors
    if (level == REPORTING_INFO) {
        marker = '-';
    }
    else if (level == REPORTING_WARNING) {
        marker = '?';
    }
    assert(level >= REPORTING_INFO);
    assert(level <= REPORTING_OTHER);
    level = MAX(REPORTING_INFO, MIN(REPORTING_OTHER, level));
    fprintf(stderr, "%c %13s  offset +%06u, code %d\n", marker, levelName[level], offset, code);
    return 0;
}


void pdfrasread_set_error_handler(t_pdfrasreader* reader, pdfras_err_handler errhandler)
{
    if (!VALID(reader)) {
        global_error_handler(NULL, REPORTING_API, READ_API_BAD_READER, __LINE__);
    } else {
        if (!errhandler) {
            errhandler = call_global_error_handler;
        }
        reader->error_handler = errhandler;
    }
}

void pdfrasread_set_global_error_handler(pdfras_err_handler errhandler)
{
    if (!errhandler) {
        errhandler = pdfrasread_default_error_handler;
    }
    global_error_handler = errhandler;
}


void pdfrasread_get_highest_pdfr_version(t_pdfrasreader* reader, int* pmajor, int* pminor)
{
    if (pmajor) *pmajor = RASREAD_MAX_MAJOR;
    if (pminor) *pminor = RASREAD_MAX_MINOR;
}

int pdfrasread_open(t_pdfrasreader* reader, void* source)
{
    if (!VALID(reader)) {
        global_error_handler(NULL, REPORTING_API, READ_API_BAD_READER, __LINE__);
        return FALSE;
    }
	if (reader->bOpen) {
        // already open, can't open it.
        global_error_handler(NULL, REPORTING_API, READ_API_ALREADY_OPEN, __LINE__);
        return FALSE;
	}
	reader->source = source;
	if (!parse_trailer(reader)) {
        // not a valid PDF/raster file
		reader->source = NULL;
	}
	else {
		reader->bOpen = PD_TRUE;
	}
	return reader->bOpen;
}

void* pdfrasread_source(t_pdfrasreader* reader)
{
    if (!VALID(reader)) {
        global_error_handler(NULL, REPORTING_API, READ_API_BAD_READER, __LINE__);
        return NULL;
    }
    return reader->source;
}

int pdfrasread_is_open(t_pdfrasreader* reader)
{
    if (!VALID(reader)) {
        global_error_handler(NULL, REPORTING_API, READ_API_BAD_READER, __LINE__);
        return FALSE;
    }
	return reader->bOpen;
}

int pdfrasread_close(t_pdfrasreader* reader)
{
    if (!VALID(reader)) {
        global_error_handler(NULL, REPORTING_API, READ_API_BAD_READER, __LINE__);
        return FALSE;
    }
    if (!reader->bOpen) {
        return FALSE;
    }
    if (reader->fclose) {
        reader->fclose(reader->source);
    }
    reader->bOpen = PD_FALSE;
    return TRUE;
}
