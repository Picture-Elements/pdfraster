#ifndef PORTABILITY_H
#define PORTABILITY_H

#ifdef _MSC_VER
// Microsoft C/C++ compiler

// has sprintf_s
#define TIMEZONE _timezone
#define TZSET _tzset
#define SETENV(var,val) _putenv_s(var, val)

#else
// gcc (Linux) and clang/XCode (OS X)

// sprintf_s is not standard, snprintf seems to be?
#define sprintf_s(buffer, buffer_size, stringbuffer, ...) (sprintf(buffer, stringbuffer, __VA_ARGS__))

#define TIMEZONE timezone
#define TZSET tzset
#define SETENV(var,val) setenv(var, val, 1)
#endif

#endif // PORTABILITY_H

