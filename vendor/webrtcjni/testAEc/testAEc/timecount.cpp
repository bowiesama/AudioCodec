#include "timecounter.h"
unsigned __int64 rdtsc()
{
	return __rdtsc();
}
LONGLONG timeer;