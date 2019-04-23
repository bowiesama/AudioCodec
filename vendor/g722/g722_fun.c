#include "g722.h"
#include "safe_mem_lib.h"

void g722_init_decoder(g722Decoder *pDecoder)
{
    if(pDecoder)
    {
        cisco_memset_s(pDecoder, sizeof(g722Decoder), 0);
        pDecoder->detl = 32;
        pDecoder->deth = 8;
        pDecoder->channels = 1;
    }
}

void g722_init_encoder(g722Encoder *pEncoder, int frameSampleSize)
{
    if(pEncoder)
    {
        cisco_memset_s(pEncoder, sizeof(g722Encoder), 0);
        pEncoder->detl = 32;
        pEncoder->deth = 8;
        pEncoder->frameCnt = 0;
        pEncoder->frameSampleSize = frameSampleSize;
        pEncoder->channels = 1;
    }

}