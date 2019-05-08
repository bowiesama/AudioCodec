//
//  CodecAllInOneUtil.h
//  CodecAllInOne
//
//  Created by bowchen on 2019/5/8.
//

#ifndef CODEC_ALL_IN_ONE_UTIL_H
#define CODEC_ALL_IN_ONE_UTIL_H

extern "C"
{
    
    //convert short to float, numSamples will be protected by caller
    void ShortToFloat(short* src, float* dst, int numSamples);
    //convert float to short
    void FloatToShort(float* src, short* dst, int numSamples);
}

#endif /* CodecAllInOneUtil_h */
