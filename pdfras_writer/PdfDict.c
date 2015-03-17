#include "PdfDict.h"
#include "PdfHash.h"
#include "PdfElements.h"
#include "PdfAtoms.h"
#include "PdfDatasink.h"
#include "PdfXrefTable.h"
#include "PdfStandardAtoms.h"

typedef struct t_pddict
{
	t_pdallocsys *alloc;
	pdbool isStream;
	t_pdhashatomtovalue *elems;
} t_pddict;

typedef struct t_pdstream
{
	t_pddict dict; /* must be first */
	t_datasink *sink;
	f_on_datasink_ready ready;
	void *readycookie;
} t_pdstream;



t_pdvalue dict_new(t_pdallocsys *allocsys, pdint32 initialsize)
{
	t_pdhashatomtovalue *hash;
	t_pddict *dict;
	t_pdvalue dictvalue;
	hash = pd_hashatomtovalue_new(allocsys, initialsize);
	if (!hash) return pdnullvalue();

	dict = (t_pddict *)pd_alloc(allocsys, sizeof(t_pddict));
	if (!dict) return pdnullvalue();
	dict->alloc = allocsys;
	dict->elems = hash;
	dict->isStream = PD_FALSE;
	dictvalue.pdtype = TPDDICT;
	dictvalue.value.dictvalue = dict;
	return dictvalue;
}

extern void dict_free(t_pdvalue dict)
{
	t_pdallocsys *alloc;
	if (!IS_DICT(dict)) return;
	if (!dict.value.dictvalue) return;
	alloc = pd_hashatomtovalue_getallocsys(dict.value.dictvalue->elems);
	/* this does not free the elements individually.
	 * yes, this will leak memory.
	 * no, this won't ultimately be an issue.
	 * the object may be referenced in multiple places so
	 * this is not our job.
	 */
	pd_hashatomtovalue_free(dict.value.dictvalue->elems);
	pd_free(alloc, dict.value.dictvalue);
	dict.value.dictvalue = 0;
}

t_pdvalue dict_get(t_pdvalue dict, t_pdatom key, pdbool *success)
{
	if (IS_REFERENCE(dict)) dict = pd_reference_get_value(dict.value.refvalue);
	if (!IS_DICT(dict) || !dict.value.dictvalue) { *success = PD_FALSE; return pdnullvalue(); }
	return pd_hashatomtovalue_get(dict.value.dictvalue->elems, key, success);
}

t_pdvalue dict_put(t_pdvalue dict, t_pdatom key, t_pdvalue value)
{
	if (IS_REFERENCE(dict)) dict = pd_reference_get_value(dict.value.refvalue);
	if (!IS_DICT(dict) || !dict.value.dictvalue) return pderrvalue();
	pd_hashatomtovalue_put(dict.value.dictvalue->elems, key, value);
	return dict;
}

pdbool dict_is_stream(t_pdvalue dict)
{
	if (IS_REFERENCE(dict)) dict = pd_reference_get_value(dict.value.refvalue);
	if (!IS_DICT(dict)) return PD_FALSE;
	return dict.value.dictvalue->isStream;
}


void dict_foreach(t_pdvalue dict, f_pdhashatomtovalue_iterator iter, void *cookie)
{
	if (IS_REFERENCE(dict)) dict = pd_reference_get_value(dict.value.refvalue);
	if (!IS_DICT(dict) || !dict.value.dictvalue) return;
	pd_hashatomtovalue_foreach(dict.value.dictvalue->elems, iter, cookie);
}

typedef struct t_pdstm_sink
{
	t_pdallocsys *alloc;
	t_pdstream *stm;
	t_pdreference *lengthref;
	t_pdoutstream *outstm;
	pduint32 startpos;
} t_pdstm_sink;

static t_pdstm_sink *pd_stm_sink_new(t_pdallocsys *alloc, t_pdstream *stm, t_pdoutstream *outstm)
{
	pdbool succ;
	t_pdstm_sink *sink = (t_pdstm_sink *)pd_alloc(alloc, sizeof(t_pdstm_sink));	
	t_pdreference *lengthref = pd_hashatomtovalue_get(stm->dict.elems, PDA_LENGTH, &succ).value.refvalue;
	if (!sink) return 0;
	sink->alloc = alloc;
	sink->stm = stm;
	sink->lengthref = lengthref;
	sink->outstm = outstm;
	return sink;
}

static void stm_sink_begin(void *cookie)
{
	t_pdstm_sink *sink = (t_pdstm_sink *)cookie;
	pd_puts(sink->outstm, "stream\r\n");
	sink->startpos = pd_outstream_pos(sink->outstm);
}

static pdbool stm_sink_put(const pduint8 *buffer, pduint32 offset, pduint32 len, void *cookie)
{
	t_pdstm_sink *sink = (t_pdstm_sink *)cookie;
	pd_putn(sink->outstm, buffer, offset, len);
	return PD_TRUE;
}

void stm_sink_end(void *cookie)
{
	t_pdstm_sink *sink = (t_pdstm_sink *)cookie;
	pduint32 finalpos = pd_outstream_pos(sink->outstm);
	pd_puts(sink->outstm, "\r\nendstream\r\n");
	pd_reference_set_value(sink->lengthref, pdintvalue(finalpos - sink->startpos));
}

void stm_sink_free(void *cookie)
{
	t_pdstm_sink *sink = (t_pdstm_sink *)cookie;
	pd_free(sink->alloc, sink);
}


static t_datasink *stream_datasink_new(t_pdallocsys *allocsys, t_pdstream *stm, t_pdoutstream *outstm)
{
	t_pdstm_sink *sink = pd_stm_sink_new(allocsys, stm, outstm);
	
	return pd_datasink_new(allocsys, stm_sink_begin, stm_sink_put, stm_sink_end, stm_sink_free, sink);
}


t_pdvalue stream_new(t_pdallocsys *allocsys, t_pdxref *xref, pdint32 initialsize, f_on_datasink_ready ready, void *eventcookie)
{
	t_pdhashatomtovalue *hash;
	t_pdstream *stream;
	t_pdvalue dictvalue;
	hash = pd_hashatomtovalue_new(allocsys, initialsize);
	if (!hash) return pdnullvalue();

	stream = (t_pdstream *)pd_alloc(allocsys, sizeof(t_pdstream));
	if (!stream) return pdnullvalue();
	stream->dict.elems = hash;
	stream->dict.isStream = PD_TRUE;
	stream->dict.alloc = allocsys;
	stream->ready = ready;
	stream->readycookie = eventcookie;
	dictvalue.pdtype = TPDDICT;
	dictvalue.value.dictvalue = &(stream->dict);
	return pd_xref_makereference(xref, dictvalue);
}

void stream_free(t_pdvalue stream)
{
	t_pdstream *stmp;
	if (!IS_REFERENCE(stream)) return;
	stream = pd_reference_get_value(stream.value.refvalue);
	if (!dict_is_stream(stream)) return;
	stmp = (t_pdstream *)stream.value.dictvalue;
	if (!stmp) return;

	if (stmp->sink)
		pd_datasink_free(stmp->sink);
	dict_free(stream);
}

void stream_set_on_datasink_ready(t_pdvalue stream, f_on_datasink_ready ready, void *eventcookie)
{
	t_pdstream *stmp;
	if (!IS_REFERENCE(stream)) return;
	stream = pd_reference_get_value(stream.value.refvalue);
	if (!dict_is_stream(stream)) return;
	stmp = (t_pdstream *)stream.value.dictvalue;
	if (stmp->ready)
		stmp->ready(stmp->sink, stmp->readycookie);
}

void stream_write_contents(t_pdvalue stream, t_pdoutstream *outstm)
{
	t_datasink *sink;
	t_pdstream *stm;
	if (!dict_is_stream(stream)) return;
	stm = (t_pdstream *)stream.value.dictvalue;
	sink = stream_datasink_new(stm->dict.alloc, stm, outstm);
	stm->ready(sink, stm->readycookie);
}

