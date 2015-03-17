#ifndef H_pdfras_platform
#define H_pdfras_platform
#pragma once

#include <stddef.h>
#include <math.h>

#ifdef WIN32
#define PDFPLATFORM "WIN32"
#ifndef __cplusplus
typedef enum { false, true } bool;
#endif
typedef char pdint8;
typedef unsigned char pduint8;
typedef short pdint16;
typedef unsigned short pduint16;
typedef long pdint32;
typedef unsigned long pduint32;
typedef float pdfloat32;
typedef double pddouble;
typedef pduint32 pdbool;

#define PD_FALSE ((pdbool)0)
#define PD_TRUE ((pdbool)!0)
#endif

#include <float.h>

#define pdisinf(x) (!_finite(x))
#define pdisnan(x) (_isnan(x))

#ifndef PDFPLATFORM
#error Target platform not defined.
#endif
#endif // H_pdfras_platform
