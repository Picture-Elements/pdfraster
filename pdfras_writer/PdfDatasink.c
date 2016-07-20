#include "PdfDatasink.h"

typedef struct t_datasink {
	f_sink_put put;
	f_sink_free free;
	void *cookie;
} t_datasink;

t_datasink *pd_datasink_new(t_pdmempool *pool, f_sink_put put, f_sink_free free, void *cookie)
{
	t_datasink *sink = 0;
	if (put && free) {
		sink = (t_datasink *)pd_alloc(pool, sizeof(t_datasink));
		if (sink) {
			sink->put = put;
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
		if (sink->free) {
			sink->free(sink->cookie);
		}
		// free the sink object
		pd_free(sink);
	}
}

pdbool pd_datasink_put(t_datasink *sink, const void* data, pduint32 offset, size_t len)
{
	if (!sink || !data) return PD_FALSE;
	return sink->put((const pduint8*)data, offset, len, sink->cookie);
}
