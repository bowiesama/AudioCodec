#ifndef _H_timecount_
#define _H_timecount_

#include <intrin.h>

#define cpu  3.0387e+009
typedef __int64 LONGLONG;
unsigned __int64 rdtsc();

extern LONGLONG timeer;

#endif