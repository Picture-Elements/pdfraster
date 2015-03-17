
#include "PdfAtoms.h"
#include "PdfStandardAtoms.h"
#if _DEBUG
#include <assert.h>
#endif

typedef struct t_atomsys {
	t_OS *os;
} t_atomsys;

typedef struct t_atomelem {
	char *strName;
	t_pdatom atom;
} t_atomelem;

static t_atomelem __atomstrs[] =
{
	/* 0 */
	{ (char *)0, PDA_UNDEFINED_ATOM },
	{ "Type", PDA_TYPE },
	{ "Pages", PDA_PAGES },
	{ "Size", PDA_SIZE },
	{ "Root", PDA_ROOT },
	{ "Info", PDA_INFO },
	{ "ID", PDA_ID },
	{ "Catalog", PDA_CATALOG },
	{ "Parent", PDA_PARENT },
	{ "Kids", PDA_KIDS },
	/* 10 */
	{ "Count", PDA_COUNT },
	{ "Page", PDA_PAGE },
	{ "Resources", PDA_RESOURCES },
	{ "MediaBox", PDA_MEDIABOX },
	{ "CropBox", PDA_CROPBOX },
	{ "Contents", PDA_CONTENTS },
	{ "Rotate", PDA_ROTATE },
	{ "Length", PDA_LENGTH },
	{ "Filter", PDA_FILTER },
	{ "DecodeParms", PDA_DECODEPARMS },
	/* 20 */
	{ "Subtype", PDA_SUBTYPE },
	{ "Width", PDA_WIDTH },
	{ "Height", PDA_HEIGHT },
	{ "BitsPerComponent", PDA_BITSPERCOMPONENT },
	{ "ColorSpace", PDA_COLORSPACE },
	{ "Image", PDA_IMAGE },
	{ "XObject", PDA_XOBJECT },
	{ "Title", PDA_TITLE },
	{ "Subject", PDA_SUBJECT },
	{ "Author", PDA_AUTHOR },
	/* 30 */
	{ "Keywords", PDA_KEYWORDS },
	{ "Creator", PDA_CREATOR },
	{ "Producer", PDA_PRODUCER },
	{ "None", PDA_NONE },
	{ "FlateDecode", PDA_FLATEDECODE },
	{ "CCITTFaxDecode", PDA_CCITTFAXDECODE },
	{ "DCTDecode", PDA_DCTDECODE },
	{ "JBIG2Decode", PDA_JBIG2DECODE },
	{ "JPXDecode", PDA_JPXDECODE },
	{ "K", PDA_K },
	/* 40 */
	{ "Columns", PDA_COLUMNS },
	{ "Rows", PDA_ROWS },
	{ "BlackIs1", PDA_BLACKIS1 },
	{ "DeviceGray", PDA_DEVICEGRAY },
	{ "DeviceRGB", PDA_DEVICERGB },
	{ "DeviceCMYK", PDA_DEVICECMYK },
	{ "Indexed", PDA_INDEXED },
	{ "ICCBased", PDA_ICCBASED },
	{ "PieceInfo", PDA_PIECEINFO },
	{ "LastModified", PDA_LASTMODIFIED },
	/* 50 */
	{ "Private", PDA_PRIVATE },
	{ "PdfRaster", PDA_PDFRASTER },
	{ "PhysicalPageNumber", PDA_PHYSICALPAGENUMBER },
	{ "FrontSide", PDA_FRONTSIDE },
	{ "strip0", PDA_STRIP0 },

};

char *pd_string_from_atom(t_pdatom atom)
{
#if _DEBUG
	{
		int i;
		for (i = 0; i < sizeof(__atomstrs) / sizeof(t_atomelem); i++)
		{
			if ((int)__atomstrs[i].atom != i)
			{
				assert(0);
				// TODO - FAIL
			}
		}
	}
#endif
	return __atomstrs[atom].strName;
}
