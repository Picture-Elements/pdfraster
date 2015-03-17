#include "PdfElements.h"
#include "PdfString.h"
#include "PdfStrings.h"

static t_pdvalue __pdnull = { 0, TPDNULL, { 0 } };
static t_pdvalue __pderr = { 0, TPDERRVALUE, { 0 } };

t_pdvalue pdatomvalue(t_pdatom v)
{
	t_pdvalue val = { 0, TPDNAME, { .namevalue = v } };
	return val;
}

t_pdvalue pderrvalue()
{
	return __pderr;
}

t_pdvalue pdnullvalue()
{
	return __pdnull;
}

t_pdvalue pdintvalue(pdint32 v)
{
	t_pdvalue val = { 0, TPDNUMBERINT, { v } };
	return val;
}

t_pdvalue pdfloatvalue(double v)
{
	t_pdvalue val = { 0, TPDNUMBERFLOAT, { .floatvalue = (pdfloat32)v } };
	return val;
}

t_pdvalue pdboolvalue(pdbool v)
{
	t_pdvalue val = { 0, TPDBOOL, { .boolvalue = v } };
	return val;
}

t_pdvalue pdarrayvalue(t_pdarray *arr)
{
	t_pdvalue val = { 0, TPDARRAY, { .arrvalue = arr } };
	return val;
}

t_pdvalue pdstringvalue(t_pdstring *str)
{
	t_pdvalue val = { 0, TPDSTRING, { .stringvalue = str } };
	return val;
}

t_pdvalue pdcstrvalue(t_pdallocsys *alloc, char *s)
{
	t_pdstring *str = pd_string_new(alloc, s, pdstrlen(s), PD_FALSE);
	if (!str) return pderrvalue();
	return pdstringvalue(str);
}
