#include "PdfStreaming.h"
#include "PdfDatasink.h"
#include "PdfDict.h"
#include "PdfAtoms.h"
#include "PdfStandardAtoms.h"
#include "PdfString.h"
#include "PdfXrefTable.h"
#include "PdfStandardObjects.h"
#include "PdfArray.h"

typedef struct t_pdoutstream {
	fOutputWriter writer;
	void *writercookie;
	pduint32 pos;
} t_pdoutstream;

t_pdoutstream *pd_outstream_new(t_pdallocsys *pool, t_OS *os)
{
	t_pdoutstream *stm = (t_pdoutstream *)pd_alloc(pool, sizeof(t_pdoutstream));
	if (stm)
	{
		stm->writer = os->writeout;
		stm->writercookie = os->writeoutcookie;
		stm->pos = 0;
	}
	return stm;
}

void pd_outstream_free(t_pdoutstream *stm)
{
	pd_free(stm);			// doesn't mind NULLs
}

void pd_putc(t_pdoutstream *stm, char c)
{
	if (stm) {
		char __buf[1] = { c };
		stm->pos += stm->writer(__buf, 0, 1, stm->writercookie);	// data, offset, length, cookie
	}
}

void pd_putn(t_pdoutstream *stm, const pduint8 *s, pduint32 offset, pduint32 len)
{
	if (stm) {
		stm->pos += stm->writer(s, offset, len, stm->writercookie);
	}
}

void pd_puts(t_pdoutstream *stm, char *s)
{
	if (stm) {
		// Treat s==NULL same as empty string
		pdint32 len = s ? pdstrlen(s) : 0;
		pd_putn(stm, s, 0, len);
	}
}

void pd_puthex(t_pdoutstream *stm, pduint8 b)
{
	const char* hexdigit = "0123456789ABCDEF";
	pd_putc(stm, hexdigit[(b >> 4) & 0xF]);
	pd_putc(stm, hexdigit[b & 0xF]);
}

void pd_putint(t_pdoutstream *stm, pdint32 i)
{
	char *s = pditoa(i);
	pd_puts(stm, s);
}

// Write a double to the stream, formatted as a PDF real number.
// NaNs are output as "nan"
// Infinities are output as "inf" or "-inf"
// 0.0 is treated as always positive (IEEE 754 has signed zeroes)
// There is no exponential notation.
// Integral values are output as integers, without decimal point.
// Numbers with fractional part are output with only (approximately)
// REAL_PRECISION digits of precision.
void pd_putfloat(t_pdoutstream *stm, pddouble n)
{
	// one weakness: with numbers > 10^16, you get noise digits
	// in the lower digits of the integer part.
	if (pdisnan(n)) {
		pd_puts(stm, "nan");
	}
	else if (n == 0.0) {
		pd_putc(stm, '0');
	}
	else if (pdisinf(n)) {
		if (n < 0) { pd_putc(stm, '-'); }
		pd_puts(stm, "inf");
	}
	else {
		if (n < 0) {
			pd_putc(stm, '-');
			n = -n;
		}
		// the least integer that has REAL_PRECISION digits:
		double nprec = pow(10, REAL_PRECISION-1);
		int decPt = 1, digits = 1;
		// compute w, the place-value of the leading digit
		double w = 1.0;
		while (w * 10 <= n) { w *= 10; decPt++; digits++; }
		// if n is fractional, shift it up until we find first non-zero digit:
		while (n < 1.0) { n *= 10; w *= 10; digits++; }
		// shift up more digits  from fractional part (if any) to bring
		// integer part of n to REAL_PRECISION digits:
		while (n < nprec && floor(n) != n) { n *= 10; w *= 10; digits++; }
		if (floor(n) != n && decPt == digits) {
			// would print as integer but isn't an integer,
			// make sure we print a fractional part to 'suggest' this:
			n *= 10; w *= 10; digits++;
		}
		// round that last digit to nearest.
		// Note, no effect if n is exact at this point
		n = floor(n + 0.5);
		if (w * 10 <= n) {
			// whoops, rare case where rounding adds a leading digit 1
			w *= 10; decPt++; digits++;
		}
		// now, finally, render n as a sequence of digits possibly
		// including a decimal point
		// (one weakness: with numbers > 10^16, you may get noise digits
		// in the lower digits of the integer part.)
		do {
			if (decPt-- == 0) pd_putc(stm, '.');
			int d = (int)floor(n / w);
			d = (d < 0) ? 0 : (d > 9) ? 9 : d;
			pd_putc(stm, '0' + d);
			n -= d * w;
			if (n == 0 && decPt < 0) break;
			w /= 10;
		} while (--digits);
	}
}

pduint32 pd_outstream_pos(t_pdoutstream *stm)
{
	return stm ? stm->pos : 0;
}



static void writeatom(t_pdoutstream *os, t_pdatom atom)
{
	const char *str = pd_atom_name(atom);
	pd_putc(os, '/');
	while (*str)
	{
		char c = *str++;
		if (c <= (char)0x20 || c > '~' || c == '#' || c == '%' || c == '/')
		{
			pd_putc(os, '#');
			pd_puthex(os, (pduint8)c);
		}
		else
		{
			pd_putc(os, c);
		}
	}
}

// iterator for writing dictionary key-value pairs to a stream.
static pdbool itemwriter(t_pdatom key, t_pdvalue value, void *cookie)
{
	t_pdoutstream *os = (t_pdoutstream *)cookie;
	if (os) {
		pd_putc(os, ' ');
		writeatom(os, key);
		pd_putc(os, ' ');
		pd_write_value(os, value);
		return PD_TRUE;
	}
	else {
		return PD_FALSE;
	}
}

///////////////////////////////////////////////////////////////////////
// stream datasink

static void stm_sink_begin(void *cookie)
{
	t_pdoutstream *outstm = (t_pdoutstream *)cookie;
}

static pdbool stm_sink_put(const pduint8 *buffer, pduint32 offset, pduint32 len, void *cookie)
{
	t_pdoutstream *outstm = (t_pdoutstream *)cookie;
	pd_putn(outstm, buffer, offset, len);
	return PD_TRUE;
}

void stm_sink_end(void *cookie)
{
	t_pdoutstream *outstm = (t_pdoutstream *)cookie;
}

void stm_sink_free(void *cookie)
{
	t_pdoutstream *outstm = (t_pdoutstream *)cookie;
}

static t_datasink *stream_datasink_new(t_pdvalue stream, t_pdoutstream *outstm)
{
	t_pdallocsys* pool = __pd_get_pool(outstm);
	return pd_datasink_new(pool, stm_sink_begin, stm_sink_put, stm_sink_end, stm_sink_free, outstm);
}

static void stream_resolve_length(t_pdvalue stream, pduint32 len)
{
	pdbool succ;
	t_pdvalue lengthref = pd_dict_get(stream, PDA_Length, &succ);
	if (IS_REFERENCE(lengthref)) {
		pd_reference_resolve(lengthref, pdintvalue(len));
	}
}

static void writestreambody(t_pdoutstream *os, t_pdvalue dict)
{
	// create a datasink wrapper around the Stream and the outstream
	t_datasink *sink = stream_datasink_new(dict, os);
	if (sink) {
		pduint32 startpos = pd_outstream_pos(os);
		pd_puts(os, "\r\nstream\r\n");
		// Call the Stream's content generator to write its contents
		// to the sink (which writes it to the outstream):
		stream_write_data(dict, sink);
		// If there's an indirect /Length entry in the Stream dictionary, resolve it
		pduint32 finalpos = pd_outstream_pos(os);
		stream_resolve_length(dict, finalpos - startpos);
		// write the ending keyword after the stream data.
		pd_puts(os, "\r\nendstream\r\n");
		pd_datasink_free(sink);
	}
}

static void writedict(t_pdoutstream *os, t_pdvalue dict)
{
	if (IS_DICT(dict)) {
		pd_puts(os, "<<");
		pd_dict_foreach(dict, itemwriter, os);
		pd_puts(os, " >>");
		// If it's also a stream, append the stream<data>endstream
		if (pd_dict_is_stream(dict))
		{
			writestreambody(os, dict);
		}
	}
}

static pdbool arritemwriter(t_pdarray *arr, pduint32 currindex, t_pdvalue value, void *cookie)
{
	t_pdoutstream *os = (t_pdoutstream *)cookie;
	if (!os) return PD_FALSE;
	pd_putc(os, ' ');
	pd_write_value(os, value);
	return PD_TRUE;
}

static void writearray(t_pdoutstream *os, t_pdvalue arr)
{
	if (!IS_ARRAY(arr)) return;
	pd_puts(os, "[");
	pd_array_foreach(arr.value.arrvalue, arritemwriter, os);
	pd_puts(os, " ]");
}

static void writeesc(t_pdoutstream *stm, pduint8 c)
{
	pd_putc(stm, '\\');
	if (c < ' ')
	{
		pd_putc(stm, '0');
		pd_putc(stm, '0' + ((c >> 3) & 7));
		pd_putc(stm, '0' + (c & 7));
	}
	else {
		pd_putc(stm, c);
	}
}

static pdbool asciter(pduint32 index, pduint8 c, void *cookie)
{
	t_pdoutstream *stm = (t_pdoutstream *)cookie;
	if (c < ' ' || c == '(' || c == ')' || c == '\\')
		writeesc(stm, c);
	else
		pd_putc(stm, c);
	return PD_TRUE;
}

static pdbool hexiter(pduint32 index, pduint8 c, void *cookie)
{
	pd_puthex((t_pdoutstream *)cookie, c);
	return PD_TRUE;
}

static void writestring(t_pdoutstream *stm, t_pdstring *str)
{
	if (pd_string_is_binary(str))
	{	// write using the hex string notation
		pd_putc(stm, '<');
		pd_string_foreach(str, hexiter, stm);
		pd_putc(stm, '>');
	}
	else
	{
		// write using the ASCII
		pd_putc(stm, '(');
		pd_string_foreach(str, asciter, stm);
		pd_putc(stm, ')');
	}
}

void pd_write_value(t_pdoutstream *stm, t_pdvalue value)
{
	if (!stm) return;
	switch (value.pdtype)
	{
	case TPDARRAY: writearray(stm, value); break;
	case TPDNULL: pd_puts(stm, "null"); break;
	case TPDERRVALUE: pd_puts(stm, "*ERROR*"); break;
	case TPDBOOL: pd_puts(stm, value.value.boolvalue ? "true" : "false"); break;
	case TPDNUMBERINT: pd_putint(stm, value.value.intvalue); break;
	case TPDNUMBERFLOAT: pd_putfloat(stm, value.value.floatvalue); break;
	case TPDDICT: writedict(stm, value); break;		// note streams are a subcase
	case TPDNAME: writeatom(stm, value.value.namevalue); break;
	case TPDSTRING: writestring(stm, value.value.stringvalue); break;
	case TPDREFERENCE:
		pd_putint(stm, pd_reference_object_number(value));
		pd_puts(stm, " 0 R");
		break;
	}
}

void pd_write_reference_declaration(t_pdoutstream *stm, t_pdvalue ref)
{
	if (stm && IS_REFERENCE(ref)) {
		// if this indirect object has not already been written
		if (!pd_reference_is_written(ref)) {
			pd_reference_set_position(ref, pd_outstream_pos(stm));
			pd_putint(stm, pd_reference_object_number(ref));
			pd_puts(stm, " 0 obj\n");
			pd_write_value(stm, pd_reference_get_value(ref));
			pd_puts(stm, "\nendobj\n");
			pd_reference_mark_written(ref);
		}
	}
}

void pd_write_pdf_header(t_pdoutstream *stm, char *version, char *line2)
{
	if (!line2) {
		// default is the conventional 2nd line comment that marks the
		// file as not being 7-bit ASCII:
		line2 = "%\xE2\xE3\xCF\xD3\n";
	}
	pd_puts(stm, "%PDF-");
	pd_puts(stm, version);
	pd_putc(stm, '\n');
	pd_puts(stm, line2);
	if (line2[pdstrlen(line2) - 1] != '\n') {
		pd_putc(stm, '\n');
	}
}

static pdbool freeTrailerEntry(t_pdatom atom, t_pdvalue value, void *cookie)
{
	pd_value_free(&value);
	return PD_TRUE;
}

void pd_write_endofdocument(t_pdallocsys *alloc, t_pdoutstream *stm, t_pdxref *xref, t_pdvalue catalog, t_pdvalue info)
{
	t_pdvalue trailer = pd_trailer_new(alloc, xref, catalog, info);
	// drop the File ID into the trailer dictionary:

	t_pdvalue file_id = pd_generate_file_id(alloc, info);
	// finally, stuff that 'ID' entry into the trailer
	pd_dict_put(trailer, PDA_ID, file_id);

	pd_xref_writeallpendingreferences(xref, stm);
	pduint32 pos = pd_outstream_pos(stm);
	pd_xref_writetable(xref, stm);
	pd_puts(stm, "trailer\n");
	pd_write_value(stm, trailer);
	pd_putc(stm, '\n');
	pd_puts(stm, "startxref\n");
	pd_putint(stm, pos);
	pd_puts(stm, "\n%%EOF\n");
	// free the stuff that only we know about
	// namely the file-id array in the trailer dict
	pd_array_destroy(&file_id);
	pd_dict_free(trailer);
}
