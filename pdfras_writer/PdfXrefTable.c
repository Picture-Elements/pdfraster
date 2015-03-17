#include "PdfXrefTable.h"

typedef struct t_pdreference {
	pdint16 isWritten;
	pdint16 objectNumber;
	pduint32 pos;
	t_pdvalue value;
} t_pdreference;

pduint32 pd_reference_object_number(t_pdreference *ref)
{
	if (!ref) return 0;
	return ref->objectNumber;
}

pdbool pd_reference_is_written(t_pdreference *ref)
{
	if (!ref) return PD_FALSE;
	return ref->isWritten ? PD_TRUE : PD_FALSE;
}

void pd_reference_mark_written(t_pdreference *ref)
{
	if (!ref) return; /* TODO FAIL */
	ref->isWritten = 1;
}

t_pdvalue pd_reference_get_value(t_pdreference *ref)
{
	if (!ref) return pderrvalue();
	return ref->value;
}

void pd_reference_set_value(t_pdreference *ref, t_pdvalue value)
{
	if (!ref) return; /* TODO FAIL */
	ref->value = value;
}

pduint32 pd_reference_get_position(t_pdreference *ref)
{
	if (!ref) return 0; /* TODO FAIL */
	return ref->pos;
}

void pd_reference_set_position(t_pdreference *ref, pduint32 pos)
{
	if (!ref) return; /* TODO FAIL */
	ref->pos = pos;
}


typedef struct t_xr {
	t_pdreference *reference;
	struct t_xr *next;
} t_xr;

typedef struct t_pdxref
{
	t_pdallocsys *alloc;
	pduint32 nextObjectNumber;
	t_xr *first, *last;
} t_pdxref;

t_pdxref *pd_xref_new(t_pdallocsys *alloc)
{
	t_pdxref *xref = (t_pdxref *)pd_alloc(alloc, sizeof(t_pdxref));
	if (!xref) return 0;
	xref->alloc = alloc;
	xref->nextObjectNumber = 1;
	return xref;
}

void pd_xref_free(t_pdxref *xref)
{
	t_xr *walker, *next;
	if (!xref) return;
	walker = xref->first;
	while (walker)
	{
		next = walker->next;
		if (walker->reference)
			pd_free(xref->alloc, walker->reference);
		pd_free(xref->alloc, walker);
		walker = next;
	}
	pd_free(xref->alloc, xref);
}

static pdbool match(t_pdreference *ref, t_pdvalue value)
{
	if (ref->value.pdtype != value.pdtype) return PD_FALSE;
	switch (value.pdtype)
	{
	case TPDDICT: return value.value.dictvalue == ref->value.value.dictvalue;
	case TPDARRAY: return value.value.arrvalue == ref->value.value.arrvalue;
	default: break;
	}
	return PD_FALSE;
}

static t_xr *findmatch(t_pdxref *xref, t_pdvalue value)
{
	t_xr *walker;
	for (walker = xref->first; walker; walker = walker->next)
	{
		if (match(walker->reference, value))
			return walker;
	}
	return 0;
}

t_pdvalue pd_xref_makereference(t_pdxref *xref, t_pdvalue value)
{
	t_xr *xr;
	if (!xref) return pdnullvalue();
	if (IS_REFERENCE(value)) return value;
	xr = findmatch(xref, value);
	if (!xr) {
		xr = (t_xr *)pd_alloc(xref->alloc, sizeof(t_xr));
		xr->reference = (t_pdreference *)pd_alloc(xref->alloc, sizeof(t_pdreference));
		xr->reference->value = value;
		xr->reference->objectNumber = (pdint16)(xref->nextObjectNumber++);
		if (!xref->first)
		{
			xref->first = xref->last = xr;
		}
		else {
			xref->last->next = xr;
			xref->last = xr;
		}
	}
	t_pdvalue val = { 0, TPDREFERENCE, { .refvalue = xr->reference } };
	return val;
}

pdbool pd_xref_isreferenced(t_pdxref *xref, t_pdvalue value)
{
	if (IS_REFERENCE(value)) return PD_FALSE;
	return findmatch(xref, value) != 0;
}

static int xref_size(t_pdxref *xref)
{
	t_xr *walker;
	int i = 0;
	if (!xref) return 0;
	for (walker = xref->first; walker; walker = walker->next)
	{
		i++;
	}
	return i;
}

static void write_entry(t_pdoutstream *os, pduint32 pos, char *gen, char status)
{
	char *s = pditoa((pdint32)pos);
	int len = pdstrlen(s);
	int i;

	for (i = 0; i < 10 - len; i++)
		pd_putc(os, '0');
	pd_putn(os, s, 0, len);
	pd_putc(os, ' ');
	pd_puts(os, gen);
	pd_putc(os, ' ');
	pd_putc(os, status);
	pd_putc(os, 13);
	pd_putc(os, 10);
}

void pd_xref_writeallpendingreferences(t_pdxref *xref, t_pdoutstream *os)
{
	t_xr *walker;
	if (!xref || !os) return;
	for (walker = xref->first; walker; walker = walker->next)
	{
		pd_write_pdreference_declaration(os, walker->reference);
	}
}

void pd_xref_writetable(t_pdxref *xref, t_pdoutstream *os)
{
	int size = xref_size(xref);
	t_xr *walker;
	pd_puts(os, "xref\n");
	pd_putint(os, 0);
	pd_putc(os, ' ');
	pd_putint(os, size + 1);
	pd_putc(os, '\n');
	write_entry(os, 0, "65535", 'f');
	for (walker = xref->first; walker; walker = walker->next)
	{
		write_entry(os, walker->reference->pos, "00000", 'n');
	}
}

pdint32 pd_xref_size(t_pdxref *xref)
{
	if (!xref) return 0;
	return (pdint32)xref_size(xref);
}


