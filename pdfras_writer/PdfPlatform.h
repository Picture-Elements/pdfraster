#ifndef _H_Platform
#define _H_Platform
#pragma once

#include <stddef.h>
#include <math.h>

#ifdef WIN32
#define PDPLATFORM "WIN32"
typedef char pdint8;
typedef unsigned char pduint8;
typedef short pdint16;
typedef unsigned short pduint16;
typedef long pdint32;
typedef unsigned long pduint32;
typedef float pdfloat32;
typedef double pddouble;
typedef pduint32 pdbool;

#define PDDEBUG _DEBUG
#define PD_FALSE ((pdbool)0)
#define PD_TRUE ((pdbool)!0)
#endif

#include <float.h>

#define pdisinf(x) (!_finite(x))
#define pdisnan(x) (_isnan(x))

#ifndef PDPLATFORM
#error Target platform not defined.
#endif
#endif