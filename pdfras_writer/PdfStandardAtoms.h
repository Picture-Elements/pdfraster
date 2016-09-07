#ifndef _H_PdfStandardAtoms
#define _H_PdfStandardAtoms
#pragma once

#include "PdfValues.h"

#ifdef __cplusplus
extern "C" {
#endif

#define __ATOM(n) ((t_pdatom)n)

#define PDA_UNDEFINED_ATOM ((t_pdatom)__ATOM_UNDEFINED_ATOM)
#define PDA_Type ((t_pdatom)__ATOM_Type)
#define PDA_Pages ((t_pdatom)__ATOM_Pages)
#define PDA_Size ((t_pdatom)__ATOM_Size)
#define PDA_Root ((t_pdatom)__ATOM_Root)
#define PDA_Info ((t_pdatom)__ATOM_Info)
#define PDA_ID ((t_pdatom)__ATOM_ID)
#define PDA_Catalog ((t_pdatom)__ATOM_Catalog)
#define PDA_Parent ((t_pdatom)__ATOM_Parent)
#define PDA_Kids ((t_pdatom)__ATOM_Kids)
#define PDA_Count ((t_pdatom)__ATOM_Count)
#define PDA_Page ((t_pdatom)__ATOM_Page)
#define PDA_Resources ((t_pdatom)__ATOM_Resources)
#define PDA_MediaBox ((t_pdatom)__ATOM_MediaBox)
#define PDA_CropBox ((t_pdatom)__ATOM_CropBox)
#define PDA_Contents ((t_pdatom)__ATOM_Contents)
#define PDA_Rotate ((t_pdatom)__ATOM_Rotate)
#define PDA_Length ((t_pdatom)__ATOM_Length)
#define PDA_Filter ((t_pdatom)__ATOM_Filter)
#define PDA_DecodeParms ((t_pdatom)__ATOM_DecodeParms)
#define PDA_Subtype ((t_pdatom)__ATOM_Subtype)
#define PDA_Width ((t_pdatom)__ATOM_Width)
#define PDA_Height ((t_pdatom)__ATOM_Height)
#define PDA_BitsPerComponent ((t_pdatom)__ATOM_BitsPerComponent)
#define PDA_ColorSpace ((t_pdatom)__ATOM_ColorSpace)
#define PDA_Image ((t_pdatom)__ATOM_Image)
#define PDA_XObject ((t_pdatom)__ATOM_XObject)
#define PDA_Title ((t_pdatom)__ATOM_Title)
#define PDA_Subject ((t_pdatom)__ATOM_Subject)
#define PDA_Author ((t_pdatom)__ATOM_Author)
#define PDA_Keywords ((t_pdatom)__ATOM_Keywords)
#define PDA_Creator ((t_pdatom)__ATOM_Creator)
#define PDA_Producer ((t_pdatom)__ATOM_Producer)
#define PDA_None ((t_pdatom)__ATOM_None)
#define PDA_FlateDecode ((t_pdatom)__ATOM_FlateDecode)
#define PDA_CCITTFaxDecode ((t_pdatom)__ATOM_CCITTFaxDecode)
#define PDA_DCTDecode ((t_pdatom)__ATOM_DCTDecode)
#define PDA_JBIG2Decode ((t_pdatom)__ATOM_JBIG2Decode)
#define PDA_JPXDecode ((t_pdatom)__ATOM_JPXDecode)
#define PDA_K ((t_pdatom)__ATOM_K)
#define PDA_Columns ((t_pdatom)__ATOM_Columns)
#define PDA_Rows ((t_pdatom)__ATOM_Rows)
#define PDA_BlackIs1 ((t_pdatom)__ATOM_BlackIs1)
#define PDA_DeviceGray ((t_pdatom)__ATOM_DeviceGray)
#define PDA_DeviceRGB ((t_pdatom)__ATOM_DeviceRGB)
#define PDA_DeviceCMYK ((t_pdatom)__ATOM_DeviceCMYK)
#define PDA_Indexed ((t_pdatom)__ATOM_Indexed)
#define PDA_ICCBased ((t_pdatom)__ATOM_ICCBased)
#define PDA_PieceInfo ((t_pdatom)__ATOM_PieceInfo)
#define PDA_LastModified ((t_pdatom)__ATOM_LastModified)
#define PDA_Private ((t_pdatom)__ATOM_Private)
#define PDA_PDFRaster ((t_pdatom)__ATOM_PDFRaster)
#define PDA_PhysicalPageNumber ((t_pdatom)__ATOM_PhysicalPageNumber)
#define PDA_FrontSide ((t_pdatom)__ATOM_FrontSide)
#define PDA_CreationDate ((t_pdatom)__ATOM_CreationDate)
#define PDA_ModDate ((t_pdatom)__ATOM_ModDate)
#define PDA_Metadata ((t_pdatom)__ATOM_Metadata)
#define PDA_XML ((t_pdatom)__ATOM_XML)
#define PDA_CalGray ((t_pdatom)__ATOM_CalGray)
#define PDA_BlackPoint ((t_pdatom)__ATOM_BlackPoint)
#define PDA_WhitePoint ((t_pdatom)__ATOM_WhitePoint)
#define PDA_Gamma ((t_pdatom)__ATOM_Gamma)
#define PDA_N		((t_pdatom)__ATOM_N)
#define PDA_ASCIIHexDecode	((t_pdatom)__ATOM_ASCIIHexDecode)
#define PDA_CalRGB ((t_pdatom)__ATOM_CalRGB)
#define PDA_Matrix ((t_pdatom)__ATOM_Matrix)

extern char* __ATOM_UNDEFINED_ATOM;
extern char* __ATOM_Type;
extern char* __ATOM_Pages;
extern char* __ATOM_Size;
extern char* __ATOM_Root;
extern char* __ATOM_Info;
extern char* __ATOM_ID;
extern char* __ATOM_Catalog;
extern char* __ATOM_Parent;
extern char* __ATOM_Kids;
extern char* __ATOM_Count;
extern char* __ATOM_Page;
extern char* __ATOM_Resources;
extern char* __ATOM_MediaBox;
extern char* __ATOM_CropBox;
extern char* __ATOM_Contents;
extern char* __ATOM_Rotate;
extern char* __ATOM_Length;
extern char* __ATOM_Filter;
extern char* __ATOM_DecodeParms;
extern char* __ATOM_Subtype;
extern char* __ATOM_Width;
extern char* __ATOM_Height;
extern char* __ATOM_BitsPerComponent;
extern char* __ATOM_ColorSpace;
extern char* __ATOM_Image;
extern char* __ATOM_XObject;
extern char* __ATOM_Title;
extern char* __ATOM_Subject;
extern char* __ATOM_Author;
extern char* __ATOM_Keywords;
extern char* __ATOM_Creator;
extern char* __ATOM_Producer;
extern char* __ATOM_None;
extern char* __ATOM_FlateDecode;
extern char* __ATOM_CCITTFaxDecode;
extern char* __ATOM_DCTDecode;
extern char* __ATOM_JBIG2Decode;
extern char* __ATOM_JPXDecode;
extern char* __ATOM_K;
extern char* __ATOM_Columns;
extern char* __ATOM_Rows;
extern char* __ATOM_BlackIs1;
extern char* __ATOM_DeviceGray;
extern char* __ATOM_DeviceRGB;
extern char* __ATOM_DeviceCMYK;
extern char* __ATOM_Indexed;
extern char* __ATOM_ICCBased;
extern char* __ATOM_PieceInfo;
extern char* __ATOM_LastModified;
extern char* __ATOM_Private;
extern char* __ATOM_PDFRaster;
extern char* __ATOM_PhysicalPageNumber;
extern char* __ATOM_FrontSide;
extern char* __ATOM_CreationDate;
extern char* __ATOM_ModDate;
extern char* __ATOM_Metadata;
extern char* __ATOM_XML;
extern char* __ATOM_CalGray;
extern char* __ATOM_BlackPoint;
extern char* __ATOM_WhitePoint;
extern char* __ATOM_Gamma;
extern char* __ATOM_N;
extern char* __ATOM_ASCIIHexDecode;
extern char* __ATOM_CalRGB;
extern char* __ATOM_Matrix;

#ifdef __cplusplus
}
#endif
#endif
