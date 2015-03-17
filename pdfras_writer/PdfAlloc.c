#include "PdfAlloc.h"
#include <assert.h>

typedef struct t_heapelem
{
	struct t_heapelem *prev;
	struct t_heapelem *next;
	size_t size;
	pduint32 allocnumber;
#if PDDEBUG
	char *allocator;
#endif
	pduint8 data[0];
} t_heapelem;

typedef struct t_pdallocsys {
	t_OS *os;
	t_heapelem *heap;
	pduint32 allocnumber;
} t_pdallocsys;


t_pdallocsys *pd_alloc_sys_new(t_OS *os)
{
	t_pdallocsys *allocsys = 0; 
	if (!os) return 0;

	allocsys = os->alloc(sizeof(t_pdallocsys));
	if (!allocsys) return 0;
	allocsys->os = os;
	allocsys->allocnumber = 0;
	allocsys->heap = 0;
	return allocsys;
}


void *__pd_alloc(t_pdallocsys *allocsys, size_t bytes, char *allocatedBy)
{
	t_heapelem *elem = 0;
	size_t totalBytes = 0;

	if (!allocsys) return 0;
	totalBytes = bytes + sizeof(t_heapelem);
	elem = allocsys->os->alloc(totalBytes);
	if (!elem) return 0;

	elem->prev = allocsys->heap;
	elem->next = 0;
	allocsys->heap = elem;
	if (elem->prev)
		elem->prev->next = elem;

#if PDDEBUG
	elem->allocator = allocatedBy;
#endif
	allocsys->os->memclear(elem->data, bytes);
	elem->size = bytes;
	return elem->data;
}

void __pd_free(t_pdallocsys *allocsys, void *ptr, pdbool validate)
{
	t_heapelem *elem;
	int offset = offsetof(t_heapelem, data);
	if (!ptr) return;
	elem = (t_heapelem *)(((pduint8 *)ptr) - offset);
#if PDDEBUG
	if (validate)
	{
		t_heapelem *walker = allocsys->heap;
		while (walker)
		{
			if (walker->data == ptr) break;
			walker = walker->prev;
		}
		assert(walker);
		assert(elem == walker);
	}
#endif
	if (elem == allocsys->heap)
	{
		allocsys->heap = elem->prev;
		if (elem->prev) elem->prev->next = 0;
	}
	else
	{
		elem->next->prev = elem->prev;
		if (elem->prev) elem->prev->next = elem->next;
		allocsys->os->memclear(elem, sizeof(t_heapelem) + elem->size);
	}
	allocsys->os->free(elem);
}

void pd_alloc_sys_free(t_pdallocsys *sys)
{
	t_OS *os;
	if (!sys) return;
	os = sys->os;
	os->memclear(sys, sizeof(t_pdallocsys));
	os->free(sys);
}
