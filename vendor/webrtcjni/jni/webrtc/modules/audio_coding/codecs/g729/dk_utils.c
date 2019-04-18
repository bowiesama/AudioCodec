/*
 *  Copyright (c) 2014 Broadsoft. All Rights Reserved.
 *
 *  Authors: 
 *           Danail Kirov
 */

#include "dk_utils.h"

#ifdef _MSC_VER
#include <windows.h>
#include <stdio.h>

LARGE_INTEGER StartingTime = { 0 };
LARGE_INTEGER Frequency = { 0 };
#endif

void dk_utils_init()
{
#ifdef _MSC_VER
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&StartingTime);
#endif
}

uint64_t
dk_utils_delta_time_in_miliseconds()
{
#ifdef _MSC_VER
	LARGE_INTEGER EndingTime = { 0 }, Miliseconds = { 0 };

	QueryPerformanceCounter(&EndingTime);
	Miliseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;

	//
	// We now have the elapsed number of ticks, along with the
	// number of ticks-per-second. We use these values
	// to convert to the number of elapsed miliseconds.
	// To guard against loss-of-precision, we convert
	// to miliseconds *before* dividing by ticks-per-second.
	//

	Miliseconds.QuadPart *= 1000;
	Miliseconds.QuadPart /= Frequency.QuadPart;
	
	return Miliseconds.QuadPart;
#else
	return 0;
#endif
}

uint64_t
dk_utils_time_in_miliseconds()
{
#ifdef _MSC_VER
	LARGE_INTEGER Miliseconds;
	
	if (Frequency.QuadPart == 0)
	{
		dk_utils_init();
	}

	QueryPerformanceCounter(&Miliseconds);
	//
	// We now have the elapsed number of ticks, along with the
	// number of ticks-per-second. We use these values
	// to convert to the number of elapsed miliseconds.
	// To guard against loss-of-precision, we convert
	// to miliseconds *before* dividing by ticks-per-second.
	//

	Miliseconds.QuadPart *= 1000;
	Miliseconds.QuadPart /= Frequency.QuadPart;
	return Miliseconds.QuadPart;
#else
	return 0;
#endif
}

uint64_t
dk_utils_time_in_microseconds()
{
#ifdef _MSC_VER
	LARGE_INTEGER Microseconds;

	if (Frequency.QuadPart == 0)
	{
		dk_utils_init();
	}

	QueryPerformanceCounter(&Microseconds);
	//
	// We now have the elapsed number of ticks, along with the
	// number of ticks-per-second. We use these values
	// to convert to the number of elapsed microseconds.
	// To guard against loss-of-precision, we convert
	// to microseconds *before* dividing by ticks-per-second.
	//

	Microseconds.QuadPart *= 1000000;
	Microseconds.QuadPart /= Frequency.QuadPart;
	return Microseconds.QuadPart;
#else
	return 0;
#endif
}

void
trace_d(const char* formatstring, ...)
{
#ifdef _MSC_VER
	char		buff[512];
	int			BytesWritten;    
	va_list		args;
	SYSTEMTIME	lt;

	GetLocalTime(&lt);

	BytesWritten =
	sprintf_s(buff, 512, "%2.2u.%2.2u.%2.2u:%3.3u ",
		lt.wHour,
		lt.wMinute,
		lt.wSecond,
		lt.wMilliseconds);

	va_start(args, formatstring);
	vsnprintf_s(&buff[BytesWritten], sizeof(buff)-BytesWritten, _TRUNCATE, formatstring, args);
	va_end(args);

	OutputDebugStringA(buff);
#else
	// implement here
#endif
}

