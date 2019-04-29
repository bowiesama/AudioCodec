#include "g722.h"

void g722_init_decoder(g722Decoder *pDecoder)
{
    if(pDecoder)
    {
        memset(pDecoder, 0, sizeof(g722Decoder));
        pDecoder->detl = 32;
        pDecoder->deth = 8;
        pDecoder->channels = 1;
    }
}

void g722_init_encoder(g722Encoder *pEncoder, int frameSampleSize)
{
    if(pEncoder)
    {
        memset(pEncoder, 0, sizeof(g722Encoder));
        pEncoder->detl = 32;
        pEncoder->deth = 8;
        pEncoder->frameCnt = 0;
        pEncoder->frameSampleSize = frameSampleSize;
        pEncoder->channels = 1;
    }

}
