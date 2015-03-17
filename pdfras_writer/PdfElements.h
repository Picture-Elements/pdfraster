#ifndef _H_PdfElements
#define _H_PdfElements
#pragma once

#include "PdfPlatform.h"
#include "PdfAlloc.h"

typedef pduint32 t_pdatom;

/* OPTIONAL|REFERENCE|TYPE */

typedef enum {
	TPDERRVALUE,
	TPDNULL,
	TPDNUMBERINT,
	TPDNUMBERFLOAT,
	TPDNAME,
	TPDSTRING,
	TPDARRAY,
	TPDDICT,
	TPDBOOL,
	TPDREFERENCE,
} t_pdtype;

typedef t_pdatom t_pdname;

typedef union {
	pdint32 intvalue;
	pddouble floatvalue;
	t_pdatom namevalue;
	pdbool boolvalue;
	struct t_pdstring *stringvalue;
	struct t_pdarray *arrvalue;
	struct t_pddict *dictvalue;
	struct t_pdreference *refvalue;
} u_pdvalue;

typedef struct {
	int isOptional : 1;
	t_pdtype pdtype : 7;
	u_pdvalue value;
} t_pdvalue;

typedef struct t_pdarray t_pdarray;
typedef struct t_pdstring t_pdstring;
typedef struct t_pdhashatomtovalue t_pdhashatomtovalue;
typedef struct t_pddict t_pddict;
typedef struct t_pdreference t_pdreference;



extern t_pdvalue pdnullvalue();
extern t_pdvalue pderrvalue();
extern t_pdvalue pdatomvalue(t_pdatom v);
extern t_pdvalue pdintvalue(pdint32 v);
extern t_pdvalue pdboolvalue(pdbool v);
// Create and return a value that holds a floating point number:
extern t_pdvalue pdfloatvalue(double v);
extern t_pdvalue pdarrayvalue(t_pdarray *arr);
extern t_pdvalue pdstringvalue(t_pdstring *str);
extern t_pdvalue pdcstrvalue(t_pdallocsys *alloc, char *s);

#define IS_ERR(v) ((v).pdtype == TPDERRVALUE)
#define IS_NULL(v) ((v).pdtype == TPDNULL)
#define IS_DICT(v) ((v).pdtype == TPDDICT)
#define IS_NUMBER(v) ((v).pdtype == TPDNUMBERINT || (v).pdtype == TPDNUMBERFLOAT)
#define IS_INT(v) ((v).pdtype == TPDNUMBERINT)
#define IS_FLOAT(v) ((v).pdtype == TPDNUMBERFLOAT)
#define IS_STRING(v) ((v).pdtype == TPDSTRING)
#define IS_ARRAY(v) ((v).pdtype == TPDARRAY)
#define IS_BOOL(v) ((v).pdtype == TPDBOOL)
#define IS_NAME(v) ((v).pdtype == TPDNAME)
#define IS_REFERENCE(v) ((v).pdtype == TPDREFERENCE)

#endif