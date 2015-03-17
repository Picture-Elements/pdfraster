#include <assert.h>
#include <time.h>
#include <sys\timeb.h>

#include "PdfStandardObjects.h"
#include "PdfStrings.h"
#include "PdfStandardAtoms.h"
#include "PdfDict.h"
#include "PdfArray.h"
#include "PdfContentsGenerator.h"

// format unsigned integer n into w characters with leading 0's
// place at p with trailing NUL and return pointer to that NUL.
char* pdatoulz(char* p, pduint32 n, int w)
{
	char* digits = pditoa(n);
	// figure out how many digits to represent n
	int d = pdstrlen(digits);
	// lay down those leading 0's
	while (w > d) {
		*p++ = '0'; w--;
	}
	// append the digits
	pd_strcpy(p, w+1, digits);
	// step over them
	p += d;
	*p = (char)0;
	return p;
}

t_pdvalue pd_make_time_string(t_pdallocsys *alloc, time_t t)
{
	char szText[200];
	struct tm *tmp = localtime(&t);
	// As a side effect, localtime sets global 'timezone'
	// to offset from localtime to UTC in seconds
	// We want the offset from UTC to local, and in minutes:
	long UTCoff = -timezone / 60;
	char chSign = '+';
	if (UTCoff < 0) {
		chSign = '-'; UTCoff = -UTCoff;
	}
	else if (UTCoff == 0) chSign = 'Z';
	strftime(szText, sizeof(szText), "(D:%Y%m%d%H%M%S", tmp);
	char* p = szText + pdstrlen(szText);
	*p++ = chSign;
	p = pdatoulz(p, UTCoff / 60, 2);
	*p++ = '\'';
	p = pdatoulz(p, UTCoff % 60, 2);
	pd_strcpy(p, 3, "')");
	return pdcstrvalue(alloc, szText);
}

t_pdvalue pd_make_now_string(t_pdallocsys *alloc)
{
	time_t t;
	time(&t);
	return pd_make_time_string(alloc, t);
}

t_pdvalue pd_catalog_new(t_pdallocsys *alloc, t_pdxref *xref)
{
	t_pdvalue catdict = dict_new(alloc, 20);
	t_pdvalue pagesdict = dict_new(alloc, 3);
	dict_put(catdict, PDA_TYPE, pdatomvalue(PDA_CATALOG));
	dict_put(catdict, PDA_PAGES, pd_xref_makereference(xref, pagesdict));

	dict_put(pagesdict, PDA_TYPE, pdatomvalue(PDA_PAGES));
	dict_put(pagesdict, PDA_KIDS, pdarrayvalue(pd_array_new(alloc, 10)));
	dict_put(pagesdict, PDA_COUNT, pdintvalue(0));

	return pd_xref_makereference(xref, catdict);
}

t_pdvalue pd_info_new(t_pdallocsys *alloc, t_pdxref *xref, char *title, char *author, char *subject, char *keywords, char *creator, char *producer)
{
	t_pdvalue infodict = dict_new(alloc, 8);
	dict_put(infodict, PDA_TYPE, pdatomvalue(PDA_INFO));
	if (title)
		dict_put(infodict, PDA_TITLE, pdcstrvalue(alloc, title));
	if (author)
		dict_put(infodict, PDA_AUTHOR, pdcstrvalue(alloc, author));
	if (subject)
		dict_put(infodict, PDA_SUBJECT, pdcstrvalue(alloc, subject));
	if (keywords)
		dict_put(infodict, PDA_KEYWORDS, pdcstrvalue(alloc, keywords));
	if (creator)
		dict_put(infodict, PDA_CREATOR, pdcstrvalue(alloc, creator));
	if (producer)
		dict_put(infodict, PDA_PRODUCER, pdcstrvalue(alloc, producer));
	return pd_xref_makereference(xref, infodict);
}

t_pdvalue pd_trailer_new(t_pdallocsys *alloc, t_pdxref *xref, t_pdvalue catalog, t_pdvalue info)
{
	t_pdvalue trailer = dict_new(alloc, 4);
	dict_put(trailer, PDA_SIZE, pdintvalue(pd_xref_size(xref)));
	dict_put(trailer, PDA_ROOT, catalog);
	if (!IS_ERR(info))
		dict_put(trailer, PDA_INFO, info);
	return trailer;
}

static t_pdatom ToCompressionAtom(e_ImageCompression comp)
{
	switch (comp) {
	default:
	case kCompNone: return PDA_NONE;
	case kCompFlate: return PDA_FLATEDECODE;
	case kCompCCITT: return PDA_CCITTFAXDECODE;
	case kCompDCT: return PDA_DCTDECODE;
	case kCompJBIG2: return PDA_JBIG2DECODE;
	case kCompJPX: return PDA_JPXDECODE;
	}
}

t_pdvalue pd_image_new(t_pdallocsys *alloc, t_pdxref *xref, f_on_datasink_ready ready, void *eventcookie,
	t_pdvalue width, t_pdvalue height, t_pdvalue bitspercomponent, e_ImageCompression comp, t_pdvalue compParms, t_pdvalue colorspace)
{
	t_pdvalue image = stream_new(alloc, xref, 10, ready, eventcookie);
	t_pdarray *filter, *filterparms;
	if (IS_ERR(image)) return image;
	dict_put(image, PDA_TYPE, pdatomvalue(PDA_XOBJECT));
	dict_put(image, PDA_SUBTYPE, pdatomvalue(PDA_IMAGE));
	dict_put(image, PDA_WIDTH, width);
	dict_put(image, PDA_HEIGHT, height);
	dict_put(image, PDA_BITSPERCOMPONENT, bitspercomponent);
	filter = pd_array_new(alloc, 1);
	if (comp != kCompNone)
	{
		pd_array_add(filter, pdatomvalue(ToCompressionAtom(comp)));
		dict_put(image, PDA_FILTER, pdarrayvalue(filter));
		filterparms = pd_array_new(alloc, 1);
		if (!IS_NULL(compParms))
			pd_array_add(filterparms, compParms);
		dict_put(image, PDA_DECODEPARMS, pdarrayvalue(filterparms));
	}
	dict_put(image, PDA_COLORSPACE, colorspace);
	dict_put(image, PDA_LENGTH, pd_xref_makereference(xref, pdintvalue(0)));

	return image;
}

static pdint32 ToK(e_CCITTKind kind)
{
	switch (kind)
	{
	default:
	case kCCIITTG4: return -1;
	case kCCITTG31D: return 0;
	case kCCITTG32D: return 1;
	}
}

static t_pdvalue MakeCCITTParms(t_pdallocsys *alloc, pduint32 width, pduint32 height, e_CCITTKind kind, pdbool ccittBlackIs1)
{
	t_pdvalue parms = dict_new(alloc, 4);
	dict_put(parms, PDA_K, pdintvalue(ToK(kind)));
	dict_put(parms, PDA_COLUMNS, pdintvalue(width));
	dict_put(parms, PDA_ROWS, pdintvalue(height));
	dict_put(parms, PDA_BLACKIS1, pdboolvalue(ccittBlackIs1));
	return parms;
}

static t_pdatom ToColorSpaceAtom(e_ColorSpace cs)
{
	switch (cs)
	{
	default: /* TODO FAIL */
	case kDeviceGray: return PDA_DEVICEGRAY;
	case kDeviceRgb: return PDA_DEVICERGB;
	case kDeviceCmyk: return PDA_DEVICECMYK;
	}
}

t_pdvalue pd_image_new_simple(t_pdallocsys *alloc, t_pdxref *xref, f_on_datasink_ready ready, void *eventcookie,
	pduint32 width, pduint32 height, pduint32 bitspercomponent, e_ImageCompression comp, e_CCITTKind kind, pdbool ccittBlackIs1, e_ColorSpace colorspace)
{
	t_pdvalue cs = pdatomvalue(ToColorSpaceAtom(colorspace));
	t_pdvalue comparms = comp == kCompCCITT ? MakeCCITTParms(alloc, width, height, kind, ccittBlackIs1) : pdnullvalue();
	return pd_image_new(alloc, xref, ready, eventcookie, pdintvalue(width), pdintvalue(height), pdintvalue(bitspercomponent),
		comp, comparms, cs);
}

t_pdvalue pd_page_new_simple(t_pdallocsys *alloc, t_pdxref *xref, t_pdvalue catalog, double width, double height)
{
	pdbool succ;
	t_pdvalue pagedict = dict_new(alloc, 20);
	t_pdvalue pagesdict = dict_get(catalog, PDA_PAGES, &succ); /* this is a reference */
	t_pdvalue resources = dict_new(alloc, 1);
	assert(IS_REFERENCE(pagesdict));

	dict_put(pagedict, PDA_TYPE, pdatomvalue(PDA_PAGE));
	dict_put(pagedict, PDA_PARENT, pagesdict);
	dict_put(pagedict, PDA_MEDIABOX, pdarrayvalue(pd_array_buildfloats(alloc, 4, 0.0, 0.0, width, height)));
	dict_put(pagedict, PDA_RESOURCES, resources);
	dict_put(resources, PDA_XOBJECT, dict_new(alloc, 20));
	return pd_xref_makereference(xref, pagedict);
}

void pd_catalog_add_page(t_pdvalue catalog, t_pdvalue page)
{
	pdbool succ;
	t_pdvalue pagesdict = dict_get(catalog, PDA_PAGES, &succ); /* this is a reference */
	t_pdvalue kidsarr = dict_get(pagesdict, PDA_KIDS, &succ);
	t_pdvalue count = dict_get(pagesdict, PDA_COUNT, &succ);
	assert(IS_REFERENCE(pagesdict));
	assert(IS_DICT(pd_reference_get_value(pagesdict.value.refvalue)));
	assert(IS_ARRAY(kidsarr));
	assert(IS_INT(count));
	// append the new page to the /Kids array
	pd_array_add(kidsarr.value.arrvalue, page);
	// increment the total page count
	dict_put(pagesdict, PDA_COUNT, pdintvalue(count.value.intvalue + 1));
}

t_pdvalue pd_contents_new(t_pdallocsys *alloc, t_pdxref *xref, t_pdcontents_gen *gen)
{
	t_pdvalue contents = stream_new(alloc, xref, 0, pd_contents_generate, gen);
	dict_put(contents, PDA_LENGTH, pd_xref_makereference(xref, pdintvalue(0)));
	return contents;
}

void pd_page_add_image(t_pdvalue page, t_pdatom imageatom, t_pdvalue image)
{
	pdbool succ;
	dict_put(dict_get(dict_get(page, PDA_RESOURCES, &succ), PDA_XOBJECT, &succ), imageatom, image);
}
