//
//  CodecAllInOneUtil.cpp
//  CodecAllInOne
//
//  Created by wme on 2019/5/8.
//

#include "CodecAllInOneUtil.h"

void ShortToFloat(short* src, float* dst, int numSamples)
{
    int i = 0;
    for( i = 0; i < numSamples; i++)
    {
        if (*src > 32767)
        {
            *src = 32767;
        }
        else if (*src < -32768)
        {
            *src = -32768;
        }
        *dst = (*src) / 32768.0f;
        dst++;
        src++;
    }
}

void FloatToShort(float* src, short*dst, int numSamples)
{
    int i = 0;
    for ( i = 0; i < numSamples; i++)
    {
        if (*src > 1)
        {
            *src = 1;
        }
        else if (*src < -1)
        {
            *src = -1;
        }
        *dst = (*src) * 32767.0;
        dst++;
        src++;
        
    }
}
