#ifndef _H_PdfValues
#define _H_PdfValues
#pragma once

#include "PdfPlatform.h"
#include "PdfAlloc.h"

typedef void* t_pdatom;

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

//typedef t_pdatom t_pdname;

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
extern t_pdvalue pdcstrvalue(t_pdallocsys *alloc, const char *s);

#define IS_ERR(v) ((v).pdtype == TPDERRVALUE)
#define IS_NULL(v) ((v).pdtype == TPDNULL)
#define IS_DICT(v) ((v).pdtype == TPDDICT)
#define IS_STREAM(v) ((v).pdtype == TPDDICT && pd_dict_is_stream(v)==PD_TRUE)
#define IS_NUMBER(v) ((v).pdtype == TPDNUMBERINT || (v).pdtype == TPDNUMBERFLOAT)
#define IS_INT(v) ((v).pdtype == TPDNUMBERINT)
#define IS_FLOAT(v) ((v).pdtype == TPDNUMBERFLOAT)
#define IS_STRING(v) ((v).pdtype == TPDSTRING)
#define IS_ARRAY(v) ((v).pdtype == TPDARRAY)
#define IS_BOOL(v) ((v).pdtype == TPDBOOL)
#define IS_FALSE(v) ((v).pdtype == TPDBOOL && !(v).value.boolvalue)
#define IS_TRUE(v) ((v).pdtype == TPDBOOL && (v).value.boolvalue)
#define IS_NAME(v) ((v).pdtype == TPDNAME)
#define IS_REFERENCE(v) ((v).pdtype == TPDREFERENCE)

#define DEREFERENCE(v) 	{ if (IS_REFERENCE(v)) (v) = pd_reference_get_value(v); }

// Return PD_TRUE if a and b are 'the same' value.
// For streams, dictionaries, arrays and references this means
// 'are aliases for the same underlying object'.
// For the other types, it means 'print the same'.
// Note, strings are compared by value like integers.
extern int pd_value_eq(t_pdvalue a, t_pdvalue b);

// Free a dict, stream, array or string associated with a value.
// Sets the value *v to pdnullvalue().
// Only safe to use if you KNOW there are no other pointers
// to the same underlying dict, stream, array or string.
extern void pd_value_free(t_pdvalue *v);

#endif
