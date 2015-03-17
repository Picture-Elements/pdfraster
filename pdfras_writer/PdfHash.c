#include "PdfHash.h"
#include "PdfStandardAtoms.h"
#include "PdfStrings.h"

#define kSomeReasonableInitialSize 10

typedef struct {
	t_pdvalue value;
	t_pdatom key;
} t_bucket;

typedef struct t_pdhashatomtovalue {
	t_pdallocsys *alloc;
	pduint32 limit;
	pduint32 elements;
	t_bucket *buckets;
} t_pdhashatomtovalue;

static void init_table(t_pdhashatomtovalue *hash, pduint32 size)
{
	pduint32 i;
	hash->buckets = pd_alloc(hash->alloc, sizeof(t_bucket)* size);
	if (!hash->buckets) return;
	hash->limit = size;
	hash->elements = 0;
	for (i = 0; i < size; i++)
	{
		hash->buckets[i].key = PDA_UNDEFINED_ATOM;
		hash->buckets[i].value.value.intvalue = 0;
	}
}


t_pdhashatomtovalue *pd_hashatomtovalue_new(t_pdallocsys *alloc, pduint32 initialsize)
{
	t_pdhashatomtovalue *hash;
	if (!alloc) return 0;
	hash = pd_alloc(alloc, sizeof(t_pdhashatomtovalue));
	if (!hash) return 0;
	hash->alloc = alloc;
	hash->elements = 0;
	init_table(hash, initialsize == 0 ? kSomeReasonableInitialSize : initialsize);
	return hash;
}

void pd_hashatomtovalue_free(t_pdhashatomtovalue *table)
{
	t_pdallocsys *alloc;
	if (!table) return;
	alloc = table->alloc;

	pd_free(alloc, table->buckets);
	pd_free(alloc, table);
}

static pdint32 hash(t_pdhashatomtovalue *table, t_pdatom key)
{
	pduint32 count;
	pdint32 i = (pdint32)key % table->limit;
	for (count = 0; count < table->limit; count++)
	{
		if (table->buckets[i].key == key)
			return i;
		if (table->buckets[i].key == PDA_UNDEFINED_ATOM)
		{
			return i;
		}
		i = (i + 1) % table->limit;
	}
	return i; // won't happen
}

static void rehash_table(t_bucket *buckets, int n, t_pdhashatomtovalue *table)
{
	while (n-- > 0)
	{
		if (buckets->key != PDA_UNDEFINED_ATOM)
			pd_hashatomtovalue_put(table, buckets->key, buckets->value);
		buckets++;
	}
}

void pd_hashatomtovalue_put(t_pdhashatomtovalue *table, t_pdatom key, t_pdvalue value)
{
	int index;
	if (!table) return;
	if (key == PDA_UNDEFINED_ATOM) return;

	index = hash(table, key);
	if (table->buckets[index].key != key && table->elements > (table->limit * 3) / 4)
	{
		int oldlimit = table->limit;
		t_bucket *oldbuckets = table->buckets;
		init_table(table, (oldlimit * 5) / 2); /* reasonable ? */
		rehash_table(oldbuckets, oldlimit, table);
		pd_free(table->alloc, oldbuckets);
		/* try again */
		pd_hashatomtovalue_put(table, key, value);
	}
	else {
		table->buckets[index].key = key;
		table->buckets[index].value = value;
	}
}

t_pdvalue pd_hashatomtovalue_get(t_pdhashatomtovalue *table, t_pdatom key, pdbool *success)
{
	int index;
	if (!table) return pderrvalue();
	if (key == PDA_UNDEFINED_ATOM) {
		*success = 0;
		return pderrvalue();
	}

	index = hash(table, key);
	*success = table->buckets[index].key == key;
	return (*success) ? table->buckets[index].value : pderrvalue();
}

int pd_hashatomtovalue_contains(t_pdhashatomtovalue *table, t_pdatom key)
{
	pdbool success = 0;
	pd_hashatomtovalue_get(table, key, &success);
	return success;
}

t_pdallocsys *pd_hashatomtovalue_getallocsys(t_pdhashatomtovalue *table)
{
	return table->alloc;
}

void pd_hashatomtovalue_foreach(t_pdhashatomtovalue *table, f_pdhashatomtovalue_iterator iter, void *cookie)
{
	pduint32 i;
	if (!iter || !table) return;
	for (i = 0; i < table->limit; i++)
	{
		if (table->buckets[i].key != PDA_UNDEFINED_ATOM)
		{
			if (!iter(table->buckets[i].key, table->buckets[i].value, cookie))
				break;
		}
	}
}
