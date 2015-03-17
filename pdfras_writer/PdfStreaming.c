#include "PdfStreaming.h"
#include "PdfDict.h"
#include "PdfAtoms.h"
#include "PdfString.h"
#include "PdfXrefTable.h"
#include "PdfStandardObjects.h"
#include "PdfArray.h"

typedef struct t_pdoutstream {
	t_pdallocsys *alloc;
	fOutputWriter writer;
	void *writercookie;
	pduint32 pos;
} t_pdoutstream;

t_pdoutstream *pd_outstream_new(t_pdallocsys *allocsys, t_OS *os)
{
	t_pdoutstream *stm = (t_pdoutstream *)pd_alloc(allocsys, sizeof(t_pdoutstream));
	if (stm)
	{
		stm->alloc = allocsys;
		stm->writer = os->writeout;
		stm->writercookie = os->writeoutcookie;
		stm->pos = 0;
	}
	return stm;
}

void pd_outstream_free(t_pdoutstream *stm)
{
	if (!stm) return;
	pd_free(stm->alloc, stm);
}

static char __buf[1];
void pd_putc(t_pdoutstream *stm, char c)
{
	if (!stm) return;
	__buf[0] = c;
	stm->writer(__buf, 0, 1, stm->writercookie);
	stm->pos += 1;
}

void pd_puts(t_pdoutstream *stm, char *s)
{
	pdint32 len;
	if (!stm || !s || !*s) return;
	len = pdstrlen(s);
	pd_putn(stm, s, 0, len);
}

void pd_putn(t_pdoutstream *stm, const pduint8 *s, pduint32 offset, pduint32 len)
{
	if (!stm) return;
	stm->writer(s, offset, len, stm->writercookie);
	stm->pos += len;
}

void pd_putint(t_pdoutstream *stm, pdint32 i)
{
	char *s = pditoa(i);
	pd_puts(stm, s);
}

void pd_putfloat(t_pdoutstream *stm, pddouble d)
{
	char *s = pdftoa(d);
	pd_puts(stm, s);
}

pduint32 pd_outstream_pos(t_pdoutstream *stm)
{
	if (!stm) return 0;
	return stm->pos;
}



static void writeatom(t_pdoutstream *os, t_pdatom atom)
{
	char *str;
	pd_putc(os, '/');
	str = pd_string_from_atom(atom);
	while (*str)
	{
		if (*str < (char)0x21 || *str > '~' || *str == '#' || *str == '%' || *str == '/')
		{
			pd_putc(os, '#');
			pd_putc(os, hexdigit(*str >> 4));
			pd_putc(os, hexdigit(*str));
		}
		else
		{
			pd_putc(os, *str);
		}
		str++;
	}
}
static pdbool itemwriter(t_pdatom key, t_pdvalue value, void *cookie)
{
	t_pdoutstream *os = (t_pdoutstream *)cookie;
	if (!os) return PD_FALSE;
	pd_putc(os, ' ');
	writeatom(os, key);
	pd_putc(os, ' ');
	pd_write_value(os, value);
	return PD_TRUE;
}

static void writedict(t_pdoutstream *os, t_pdvalue dict)
{
	if (!IS_DICT(dict)) return;
	pd_puts(os, "<<");
	dict_foreach(dict, itemwriter, os);
	pd_puts(os, " >>");
	if (dict_is_stream(dict))
	{
		stream_write_contents(dict, os);
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
	t_pdoutstream *stm = (t_pdoutstream *)cookie;
	pd_putc(stm, hexdigit(c >> 4));
	pd_putc(stm, hexdigit(c));
	return PD_TRUE;
}

static void writestring(t_pdoutstream *stm, t_pdstring *str)
{
	if (pd_string_is_binary(str))
	{
		pd_putc(stm, '<');
		pd_string_foreach(str, hexiter, stm);
		pd_putc(stm, '>');
	}
	else
	{
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
	case TPDBOOL: pd_puts(stm, value.value.boolvalue ? "true" : "false"); break;
	case TPDNUMBERINT: pd_putint(stm, value.value.intvalue); break;
	case TPDNUMBERFLOAT: pd_putfloat(stm, value.value.floatvalue); break;
	case TPDDICT: writedict(stm, value); break;
	case TPDNAME: writeatom(stm, value.value.namevalue); break;
	case TPDREFERENCE:
		pd_putint(stm, pd_reference_object_number(value.value.refvalue));
		pd_puts(stm, " 0 R");
		break;
	case TPDSTRING:
		writestring(stm, value.value.stringvalue);
		break;
	}
}

void pd_write_pdreference_declaration(t_pdoutstream *stm, t_pdreference *ref)
{
	if (!stm || !ref || pd_reference_is_written(ref)) return;
	pd_reference_set_position(ref, stm->pos);
	pd_putint(stm, pd_reference_object_number(ref));
	pd_puts(stm, " 0 obj ");
	pd_write_value(stm, pd_reference_get_value(ref));
	pd_puts(stm, "\nendobj\n");
	pd_reference_mark_written(ref);
}

void pd_write_reference_declaration(t_pdoutstream *stm, t_pdvalue value)
{
	if (!IS_REFERENCE(value)) return;
	pd_write_pdreference_declaration(stm, value.value.refvalue);
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

void pd_write_endofdocument(t_pdallocsys *alloc, t_pdoutstream *stm, t_pdxref *xref, t_pdvalue catalog, t_pdvalue info)
{
	t_pdvalue trailer = pd_trailer_new(alloc, xref, catalog, info);
	pd_xref_writeallpendingreferences(xref, stm);
	pduint32 pos = pd_outstream_pos(stm);
	pd_xref_writetable(xref, stm);
	pd_puts(stm, "trailer\n");
	pd_write_value(stm, trailer);
	pd_putc(stm, '\n');
	pd_puts(stm, "startxref\n");
	pd_putint(stm, pos);
	pd_puts(stm, "\n%%EOF\n");
	dict_free(trailer);
}
