#include "PdfArray.h"
#include <stdarg.h>

#define kSomeReasonableDefaultSize 12

typedef struct t_pdarray {
	t_pdallocsys *alloc;
	pduint32 size;
	pduint32 maxsize;
	t_pdvalue *data;
} t_pdarray;

t_pdarray *pd_array_new(t_pdallocsys *alloc, pduint32 initialSize)
{
	t_pdarray *arr;
	if (!alloc) return 0;
	initialSize = initialSize ? initialSize : kSomeReasonableDefaultSize;
	arr = (t_pdarray *)pd_alloc(alloc, sizeof(t_pdarray));
	if (!arr) return 0;
	arr->alloc = alloc;
	arr->size = 0;
	arr->maxsize = initialSize;
	arr->data = (t_pdvalue *)pd_alloc(arr->alloc, initialSize * sizeof(t_pdvalue));
	return arr;
}

void pd_array_free(t_pdarray *arr)
{
	if (!arr) return;
	if (arr->data)
	{
		pd_free(arr->alloc, arr->data);
		arr->data = 0;
	}
	pd_free(arr->alloc, arr);
}

pduint32 pd_array_size(t_pdarray *arr)
{
	if (!arr) return 0;
	return arr->size;
}

pduint32 pd_array_maxsize(t_pdarray *arr)
{
	if (!arr) return 0;
	return arr->maxsize;
}

t_pdvalue pd_array_get(t_pdarray *arr, pduint32 index)
{
	if (!arr || index >= arr->size) return pderrvalue();
	return arr->data[index];
}

void pd_array_set(t_pdarray *arr, pduint32 index, t_pdvalue value)
{
	if (!arr || index >= arr->size) return;
	arr->data[index] = value;
}

static void grow_by_one(t_pdarray *arr)
{
	if (arr->size == arr->maxsize)
	{
		t_pdvalue *olddata = arr->data;
		pduint32 i;
		arr->maxsize = arr->maxsize * 5 / 2; /* reasonable? */
		arr->data = (t_pdvalue *)pd_alloc(arr->alloc, arr->maxsize * sizeof(t_pdvalue));
		if (arr->data)
		{
			/* TODO - FAIL */
			return;
		}
		for (i = 0; i < arr->size; i++)
		{
			arr->data[i] = olddata[i];
		}
		pd_free(arr->alloc, olddata);
	}
	arr->size++;
}

static void copy_elements(t_pdarray *arr, pduint32 from, pduint32 size, pduint32 to)
{
	int i;
	if (to == from) return;
	if (to < from)
	{
		for (i = 0; i < (int)size; i++)
		{
			arr->data[to + i] = arr->data[from + i];
		}
	}
	else {
		for (i = size - 1; i >= 0; i++)
		{
			arr->data[to + i] = arr->data[from + i];
		}
	}
}

void pd_array_insert(t_pdarray *arr, pduint32 index, t_pdvalue value)
{
	if (!arr) return;
	grow_by_one(arr);
	copy_elements(arr, index, arr->size - index - 1, index + 1);
	arr->data[index] = value;
}

void pd_array_add(t_pdarray *arr, t_pdvalue value)
{
	if (!arr) return;
	grow_by_one(arr);
	arr->data[arr->size - 1] = value;
}

t_pdvalue pd_array_remove(t_pdarray *arr, pduint32 index)
{
	t_pdvalue val;
	if (!arr || index >= arr->size) return pderrvalue();
	val = arr->data[index];
	copy_elements(arr, index + 1, arr->size - index - 1, index);
	arr->size--;
	return val;
}

void pd_array_foreach(t_pdarray *arr, f_pdarray_iterator iter, void *cookie)
{
	pduint32 i;
	if (!arr || !iter || arr->size == 0) return;
	for (i = 0; i < arr->size; i++)
	{
		if (!iter(arr, i, arr->data[i], cookie))
			break;
	}
}

t_pdarray *pd_array_build(t_pdallocsys *alloc, pduint32 size, t_pdvalue value, ...)
{
	va_list ap;
	pduint32 i;
	t_pdarray *arr = pd_array_new(alloc, size);
	if (!arr) return 0;
	va_start(ap, size);
	for (i = 0; i < size; i++)
	{
		pd_array_add(arr, va_arg(ap, t_pdvalue));
	}
	va_end(ap);
	return arr;
}

t_pdarray *pd_array_buildints(t_pdallocsys *alloc, pduint32 size, pdint32 value, ...)
{
	va_list ap;
	pduint32 i;
	t_pdarray *arr = pd_array_new(alloc, size);
	if (!arr) return 0;
	va_start(ap, size);
	for (i = 0; i < size; i++)
	{
		pd_array_add(arr, pdintvalue(va_arg(ap, pdint32)));
	}
	va_end(ap);
	return arr;
}

t_pdarray *pd_array_buildfloats(t_pdallocsys *alloc, pduint32 size, double value, ...)
{
	va_list ap;
	pduint32 i;
	t_pdarray *arr = pd_array_new(alloc, size);
	if (!arr) return 0;
	va_start(ap, size);
	for (i = 0; i < size; i++)
	{
		double v = va_arg(ap, double);
		pd_array_add(arr, pdfloatvalue(v));
	}
	va_end(ap);
	return arr;
}
