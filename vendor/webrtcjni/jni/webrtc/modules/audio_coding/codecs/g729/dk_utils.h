/*
 *  Copyright (c) 2014 Broadsoft. All Rights Reserved.
 *
 *  Authors: 
 *           Danail Kirov
 */

#ifndef dk_utils_h
#define dk_utils_h

#include <stdint.h>

void
trace_d(const char* formatstring, ...);

uint64_t
dk_utils_time_in_miliseconds();

uint64_t
dk_utils_time_in_microseconds();

#endif
