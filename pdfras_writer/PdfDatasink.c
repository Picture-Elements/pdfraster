#include "PdfDatasink.h"

typedef struct t_datasink {
	t_pdallocsys *alloc;
	f_sink_begin begin;
	f_sink_put put;
	f_sink_end end;
	f_sink_free free;
	void *cookie;
} t_datasink;

t_datasink *pd_datasink_new(t_pdallocsys *alloc, f_sink_begin begin, f_sink_put put, f_sink_end end, f_sink_free free, void *cookie)
{
	t_datasink *sink = 0;
	if (begin && end && put && free) {
		sink = (t_datasink *)pd_alloc(alloc, sizeof(t_datasink));
		if (sink) {
			sink->begin = begin;
			sink->put = put;
			sink->end = end;
			sink->free = free;
			sink->cookie = cookie;
		}
	}
	return sink;
}

void pd_datasink_free(t_datasink *sink)
{
	if (sink) {
		// Let the sink specialist do whatever it needs
		sink->free(sink->cookie);
		// free the sink object
		pd_free(sink);
	}
}

void pd_datasink_begin(t_datasink *sink)
{
	if (!sink) return;
	sink->begin(sink->cookie);
}

pdbool pd_datasink_put(t_datasink *sink, const pduint8 *data, pduint32 offset, pduint32 len)
{
	if (!sink || !data) return PD_FALSE;
	return sink->put(data, offset, len, sink->cookie);
}

void pd_datasink_end(t_datasink *sink)
{
	if (!sink) return;
	sink->end(sink->cookie);
}