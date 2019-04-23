
/*
The program is available by ftp from CMU, and may be used and distributed
freely, provided the copyright notices are maintained.

The Carnegie Mellon ADPCM program is Copyright (c) 1993
by Carnegie Mellon University. Use of this program, for any research or
commercial purpose, is completely unrestricted.  If you make use of or
redistribute this material, we would appreciate acknowlegement of its origin.
*/

/*
 * It's based on source code from  ftp://ftp.cs.cmu.edu/project/fgdata/speech-compression/CCITT-ADPCM/64kbps/adpcm64_g722/,
 * The source code is in public domain, no copyright is claimed.
 */


#include "g722.h"
#include "memory.h"
#ifdef ANDROID
#include "string.h"
#endif

void g722_reset_encoder(g722Encoder *pEncoder)
{
    if(pEncoder)
    {
        memset(pEncoder, 0, sizeof(g722Encoder));
        pEncoder->detl = 32;
        pEncoder->deth = 8;
        pEncoder->frameCnt = 0;
        pEncoder->frameSampleSize = 320; // 16kHz sampled, 20ms frame
        pEncoder->channels = 1;
    }

}


static void sb_encoder_tx_qmf(g722Encoder *enc, short x1, short x0, int  *xl, int *xh)
{

    // QMF filter coefficients
    static int qmf_tx_h[24] =  {3,	-11,	-11,	53,	12,	-156,
                                32,	362,	-210,	-805,	951,	3876,
                                3876,	951,	-805,	-210,	362,	32,
                                -156,	12,	53,	-11,	-11,	3
                               } ;

    int so = 0;
    int se = 0;
    int i;
    register int *x;

    if(!enc)
    {
        return;
    }

    x = &enc->xin[0];

    for(i = 23; i > 1; i--)
        x[i] = x[i - 2];

    x[1] = x1;
    x[0] = x0;

    for(i = 1;  i < 24; i = i + 2)
        so = so + x[i] * qmf_tx_h[i];

    for(i = 0;  i < 24; i = i + 2)
        se = se + x[i] * qmf_tx_h[i];

    *xl = (se + so) >> 14;
    *xh = (se - so) >> 14;

    *xl = (*xl > 16383) ?  16383 : *xl;
    *xl = (*xl < -16384) ? -16384 : *xl;
    *xh = (*xh > 16383) ?  16383 : *xh;
    *xh = (*xh < -16383) ? -16384 : *xh;

    return;
}


// block1L
static void sb_encoder_quant_lowband(g722Encoder *enc, int xl, int *il)
{
    int  i, el, sil, mil, wd, wd1, hdu ;

    static int q6[32] = {0, 35, 72, 110, 150, 190, 233, 276, 323,
                         370, 422, 473, 530, 587, 650, 714, 786,
                         858, 940, 1023, 1121, 1219, 1339, 1458,
                         1612, 1765, 1980, 2195, 2557, 2919, 0, 0
                        } ;

    static int iln[32] = {0, 63, 62, 31, 30, 29, 28, 27, 26, 25,
                          24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14,
                          13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 0
                         } ;

    static int ilp[32] = {0, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52,
                          51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41,
                          40, 39, 38, 37, 36, 35, 34, 33, 32, 0
                         } ;


    el = xl - enc->sl ;
    if(el > 32767) el = 32767;
    if(el < -32768) el = -32768;


    sil = el >> 15 ;
    if(sil == 0)  wd = el ;
    else wd = 32767 - (el & 32767);

    mil = 1 ;

    for(i = 1; i < 30; i++)
    {
        hdu = (q6[i] << 3) * enc->detl;
        wd1 = (hdu >> 15) ;
        if(wd >= wd1)  mil = (i + 1) ;
        else break ;
    }

    if(sil == -1) *il = iln[mil] ;
    else *il = ilp[mil] ;

}

// block2L
static void sb_encoder_inquant_lowband(g722Encoder *enc, int il, int *dlt)
{
    //int dlt ;
    int ril, wd2 ;
    static int qm4[16] =
    {
        0,	-20456,	-12896,	-8968,
        -6288,	-4240,	-2584,	-1200,
        20456,	12896,	8968,	6288,
        4240,	2584,	1200,	0
    } ;

    ril = il >> 2 ;
    wd2 = qm4[ril] ;
    *dlt = (enc->detl * wd2) >> 15 ;

}

// block3L
static void sb_encoder_quantadapt_lowband(g722Encoder *enc, int il)
{

    int ril, il4, wd, wd1, wd2, wd3, nbpl, depl ;
    static int wl[8] = { -60, -30, 58, 172, 334, 538, 1198, 3042 } ;
    static int rl42[16] = {0, 7, 6, 5, 4, 3, 2, 1, 7, 6, 5, 4, 3, 2,
                           1, 0
                          } ;
    static int ilb[32] = {2048, 2093, 2139, 2186, 2233, 2282, 2332,
                          2383, 2435, 2489, 2543, 2599, 2656, 2714,
                          2774, 2834, 2896, 2960, 3025, 3091, 3158,
                          3228, 3298, 3371, 3444, 3520, 3597, 3676,
                          3756, 3838, 3922, 4008
                         } ;

    ril = il >> 2 ;
    il4 = rl42[ril] ;
    wd = (enc->nbl * 32512) >> 15 ;
    nbpl = wd + wl[il4] ;

    if(nbpl <     0) nbpl = 0 ;
    if(nbpl > 18432) nbpl = 18432 ;

    wd1 = (nbpl >> 6) & 31 ;
    wd2 = nbpl >> 11 ;
    if((8 - wd2) < 0)    wd3 = ilb[wd1] << (wd2 - 8) ;
    else   wd3 = ilb[wd1] >> (8 - wd2) ;
    depl = wd3 << 2 ;
    enc->nbl = nbpl ;
    enc->detl = depl ;

}

// block4L
static void sb_encoder_adaptpred_lowband(g722Encoder *enc, int dl)
{
    int i ;
    int wd, wd1, wd2, wd3, wd4, wd5 ;

    enc->dlt[0] = dl;

    enc->rlt[0] = enc->sl + enc->dlt[0] ;
    if(enc->rlt[0] > 32767) enc->rlt[0] = 32767;
    if(enc->rlt[0] < -32768) enc->rlt[0] = -32768;


    enc->plt[0] = enc->dlt[0] + enc->szl ;
    if(enc->plt[0] > 32767) enc->plt[0] = 32767;
    if(enc->plt[0] < -32768) enc->plt[0] = -32768;


    enc->sgl[0] = enc->plt[0] >> 15 ;
    enc->sgl[1] = enc->plt[1] >> 15 ;
    enc->sgl[2] = enc->plt[2] >> 15 ;

    wd1 = enc->al[1] + enc->al[1] ;
    wd1 = wd1 + wd1 ;
    if(wd1 > 32767) wd1 = 32767;
    if(wd1 < -32768) wd1 = -32768;

    if(enc->sgl[0] == enc->sgl[1])  wd2 = - wd1 ;
    else  wd2 = wd1 ;
    if(wd2 > 32767) wd2 = 32767;

    wd2 = wd2 >> 7 ;

    if(enc->sgl[0] == enc->sgl[2])  wd3 = 128 ;
    else  wd3 = - 128 ;

    wd4 = wd2 + wd3 ;
    wd5 = (enc->al[2] * 32512) >> 15 ;

    enc->apl[2] = wd4 + wd5 ;
    if(enc->apl[2] >  12288)  enc->apl[2] =  12288 ;
    if(enc->apl[2] < -12288)  enc->apl[2] = -12288 ;

    enc->sgl[0] = enc->plt[0] >> 15 ;
    enc->sgl[1] = enc->plt[1] >> 15 ;

    if(enc->sgl[0] == enc->sgl[1])  wd1 = 192 ;
    if(enc->sgl[0] != enc->sgl[1])  wd1 = - 192 ;

    wd2 = (enc->al[1] * 32640) >> 15 ;

    enc->apl[1] = wd1 + wd2 ;
    if(enc->apl[1] > 32767) enc->apl[1] = 32767;
    if(enc->apl[1] < -32768) enc->apl[1] = -32768;

    wd3 = (15360 - enc->apl[2]) ;
    if(wd3 > 32767) wd3 = 32767;
    if(wd3 < -32768) wd3 = -32768;
    if(enc->apl[1] >  wd3)  enc->apl[1] =  wd3 ;
    if(enc->apl[1] < -wd3)  enc->apl[1] = -wd3 ;


    if(enc->dlt[0] == 0)  wd1 = 0 ;
    if(enc->dlt[0] != 0)  wd1 = 128 ;

    enc->sgl[0] = enc->dlt[0] >> 15 ;

    for(i = 1; i < 7; i++)
    {
        enc->sgl[i] = enc->dlt[i] >> 15 ;
        if(enc->sgl[i] == enc->sgl[0])  wd2 = wd1 ;
        if(enc->sgl[i] != enc->sgl[0])  wd2 = - wd1 ;
        wd3 = (enc->bl[i] * 32640) >> 15 ;
        enc->bpl[i] = wd2 + wd3 ;
        if(enc->bpl[i] > 32767) enc->bpl[i] = 32767;
        if(enc->bpl[i] < -32768) enc->bpl[i] = -32768;
    }
    for(i = 6; i > 0; i--)
    {
        enc->dlt[i] = enc->dlt[i - 1] ;
        enc->bl[i]  = enc->bpl[i] ;
    }

    for(i = 2; i > 0; i--)
    {
        enc->rlt[i] = enc->rlt[i - 1] ;
        enc->plt[i] = enc->plt[i - 1] ;
        enc->al[i] = enc->apl[i] ;
    }

    wd1 = (enc->rlt[1] + enc->rlt[1]) ;
    if(wd1 > 32767) wd1 = 32767;
    if(wd1 < -32768) wd1 = -32768;
    wd1 = (enc->al[1] * wd1) >> 15 ;

    wd2 = (enc->rlt[2] + enc->rlt[2]) ;
    if(wd2 > 32767) wd2 = 32767;
    if(wd2 < -32768) wd2 = -32768;
    wd2 = (enc->al[2] * wd2) >> 15 ;

    enc->spl = wd1 + wd2 ;
    if(enc->spl > 32767) enc->spl = 32767;
    if(enc->spl < -32768) enc->spl = -32768;


    enc->szl = 0 ;
    for(i = 6; i > 0; i--)
    {
        wd = (enc->dlt[i] + enc->dlt[i]) ;
        if(wd > 32767) wd = 32767;
        if(wd < -32768) wd = -32768;
        enc->szl += (enc->bl[i] * wd) >> 15 ;
        if(enc->szl > 32767) enc->szl = 32767;
        if(enc->szl < -32768) enc->szl = -32768;
    }


    enc->sl = enc->spl + enc->szl ;
    if(enc->sl > 32767) enc->sl = 32767;
    if(enc->sl < -32768) enc->sl = -32768;

}

static void sb_encoder_adpcmEncodeLowband(g722Encoder *enc, int xlow, int *ilow)
{
    int dlowt;
    int il;

    sb_encoder_quant_lowband(enc, xlow, ilow);

    il = *ilow;

    sb_encoder_inquant_lowband(enc, il, &dlowt);

    sb_encoder_quantadapt_lowband(enc,  il);

    sb_encoder_adaptpred_lowband(enc, dlowt);

    return;
}


// block1H
static void sb_encoder_quant_highband(g722Encoder *enc, int xhigh, int *ih)
{
    int wd, wd1, hdu;
    int eh, mih;
    //int sih;


    static int ihn[3] = { 0, 1, 0 } ;
    static int ihp[3] = { 0, 3, 2 } ;


    eh = xhigh - enc->sh ;
    /*
    if ( eh > 32767 ) eh = 32767;
    else if ( eh < -32768 ) eh = -32768;
    */

    wd = (eh >= 0) ? eh : -(eh + 1) ;

    hdu = 564 * enc->deth;
    wd1 = hdu >> 12 ;
    mih = (wd >= wd1) ? 2 : 1 ;
    *ih = (eh < 0) ? ihn[mih] : ihp[mih] ;
    return;

}
// block2H
static void sb_encoder_inquant_highband(g722Encoder *enc, int ih, int *dh)
{
    int wd2;

    static int qm2[4] =
    { -7408, -1616,  7408,   1616} ;


    wd2 = qm2[ih] ;
    *dh = (enc->deth * wd2) >> 15 ;

}


// block3H
static void sb_encoder_quantadapt_highband(g722Encoder *enc, int ih)
{
    int ih2, wd, wd1, wd2, wd3;
    static int wh[3] = {0, -214, 798} ;
    static int rh2[4] = {2, 1, 2, 1} ;
    static int ilb[32] = {2048, 2093, 2139, 2186, 2233, 2282, 2332,
                          2383, 2435, 2489, 2543, 2599, 2656, 2714,
                          2774, 2834, 2896, 2960, 3025, 3091, 3158,
                          3228, 3298, 3371, 3444, 3520, 3597, 3676,
                          3756, 3838, 3922, 4008
                         } ;

    //int fd;
    //short  *datap;

    ih2 = rh2[ih] ;
    wd = (enc->nbh * 127) >> 7 ;
    enc->nbh = wd + wh[ih2] ;

    if(enc->nbh <     0) enc->nbh = 0 ;
    else if(enc->nbh > 22528) enc->nbh = 22528 ;

    wd1 = (enc->nbh >> 6) & 31 ;
    wd2 = enc->nbh >> 11 ;
    wd3 = ((10 - wd2) < 0) ? ilb[wd1] << (wd2 - 10) : ilb[wd1] >> (10 - wd2) ;
    enc->deth = wd3 << 2 ;

}

// block4H
static void sb_encoder_adaptpred_highband(g722Encoder *enc, int dhigh)
{
    int i ;
    int wd, wd1, wd2, wd3, wd4, wd5;
    //int wd6, wd7 ;

    enc->dh[0] = dhigh;

    enc->rh[0] = enc->sh + enc->dh[0] ;
    if(enc->rh[0] > 32767) enc->rh[0] = 32767;
    if(enc->rh[0] < -32768) enc->rh[0] = -32768;


    enc->ph[0] = enc->dh[0] + enc->szh ;
    if(enc->ph[0] > 32767) enc->ph[0] = 32767;
    if(enc->ph[0] < -32768) enc->ph[0] = -32768;


    enc->sg[0] = enc->ph[0] >> 15 ;
    enc->sg[1] = enc->ph[1] >> 15 ;
    enc->sg[2] = enc->ph[2] >> 15 ;

    wd1 = enc->ah[1] + enc->ah[1] ;
    wd1 = wd1 + wd1 ;
    if(wd1 > 32767) wd1 = 32767;
    if(wd1 < -32768) wd1 = -32768;

    if(enc->sg[0] == enc->sg[1])  wd2 = - wd1 ;
    else  wd2 = wd1 ;
    if(wd2 > 32767) wd2 = 32767;

    wd2 = wd2 >> 7 ;

    if(enc->sg[0] == enc->sg[2])  wd3 = 128 ;
    else  wd3 = - 128 ;

    wd4 = wd2 + wd3 ;
    wd5 = (enc->ah[2] * 32512) >> 15 ;

    enc->aph[2] = wd4 + wd5 ;
    if(enc->aph[2] >  12288)  enc->aph[2] =  12288 ;
    if(enc->aph[2] < -12288)  enc->aph[2] = -12288 ;

    enc->sg[0] = enc->ph[0] >> 15 ;
    enc->sg[1] = enc->ph[1] >> 15 ;

    if(enc->sg[0] == enc->sg[1])  wd1 = 192 ;
    if(enc->sg[0] != enc->sg[1])  wd1 = - 192 ;

    wd2 = (enc->ah[1] * 32640) >> 15 ;

    enc->aph[1] = wd1 + wd2 ;
    if(enc->aph[1] > 32767) enc->aph[1] = 32767;
    if(enc->aph[1] < -32768) enc->aph[1] = -32768;

    wd3 = (15360 - enc->aph[2]) ;
    if(wd3 > 32767) wd3 = 32767;
    if(wd3 < -32768) wd3 = -32768;
    if(enc-> aph[1] >  wd3)  enc->aph[1] =  wd3 ;
    if(enc->aph[1] < -wd3) enc-> aph[1] = -wd3 ;


    if(enc->dh[0] == 0)  wd1 = 0 ;
    if(enc->dh[0] != 0)  wd1 = 128 ;

    enc->sg[0] = enc->dh[0] >> 15 ;

    for(i = 1; i < 7; i++)
    {
        enc->sg[i] = enc->dh[i] >> 15 ;
        if(enc->sg[i] == enc->sg[0])  wd2 = wd1 ;
        if(enc->sg[i] != enc->sg[0])  wd2 = - wd1 ;
        wd3 = (enc->bh[i] * 32640) >> 15 ;
        enc->bph[i] = wd2 + wd3 ;
        if(enc->bph[i] > 32767) enc->bph[i] = 32767;
        if(enc->bph[i] < -32768) enc->bph[i] = -32768;

    }
    for(i = 6; i > 0; i--)
    {
        enc->dh[i] = enc->dh[i - 1] ;
        enc->bh[i]  = enc->bph[i] ;
    }

    for(i = 2; i > 0; i--)
    {
        enc->rh[i] = enc->rh[i - 1] ;
        enc->ph[i] = enc->ph[i - 1] ;
        enc->ah[i] = enc->aph[i] ;
    }

    wd1 = (enc->rh[1] + enc->rh[1]) ;
    if(wd1 > 32767) wd1 = 32767;
    if(wd1 < -32768) wd1 = -32768;
    wd1 = (enc->ah[1] * wd1) >> 15 ;

    wd2 = (enc->rh[2] + enc->rh[2]) ;
    if(wd2 > 32767) wd2 = 32767;
    if(wd2 < -32768) wd2 = -32768;
    wd2 = (enc->ah[2] * wd2) >> 15 ;

    enc->sph = wd1 + wd2 ;
    if(enc->sph > 32767) enc->sph = 32767;
    if(enc->sph < -32768) enc->sph = -32768;


    enc->szh = 0 ;
    for(i = 6; i > 0; i--)
    {
        wd = (enc->dh[i] + enc->dh[i]) ;
        if(wd > 32767) wd = 32767;
        if(wd < -32768) wd = -32768;
        enc->szh += (enc->bh[i] * wd) >> 15 ;
        if(enc->szh > 32767) enc->szh = 32767;
        if(enc->szh < -32768) enc->szh = -32768;
    }


    enc->sh = enc->sph + enc->szh ;
    if(enc->sh > 32767) enc->sh = 32767;
    if(enc->sh < -32768) enc->sh = -32768;



}

static void sb_encoder_adpcmEncodeHighband(g722Encoder *enc, int xhigh, int *ihigh)
{
    int dhigh, ih;

    sb_encoder_quant_highband(enc, xhigh, ihigh);

    ih = *ihigh;

    sb_encoder_inquant_highband(enc, ih, &dhigh);

    sb_encoder_quantadapt_highband(enc, ih);

    sb_encoder_adaptpred_highband(enc, dhigh);

}

int g722_encode(const short *pcm, unsigned char *data, int frame_size, g722Encoder *encoder)
{

    int i;
    short x0, x1;
    int xlow, xhigh, ilow, ihigh;
    int            iter = 0;

    if(!encoder) return G722_ENCODER_ALLOC_ERR ;

    if(!pcm || !data) return G722_ENCODER_BUFFER_ERR;

    frame_size >>= 1;
    for(i = 0; i < frame_size; i += encoder->channels)
    {
        x1 = *pcm;
        pcm += encoder->channels;
        x0 = *pcm;
        pcm += encoder->channels;


        sb_encoder_tx_qmf(encoder, x1, x0, &xlow, &xhigh);

        sb_encoder_adpcmEncodeLowband(encoder, xlow, &ilow);

        sb_encoder_adpcmEncodeHighband(encoder, xhigh, &ihigh);

        data[i] = (ihigh & 0x3);
        data[i] = (data[i] << 6) | (ilow & 0x3f);
        iter++;
    }

    encoder->frameCnt++;
    return iter;

}

















void g722_reset_decoder(g722Decoder *pDecoder)
{
    if(pDecoder)
    {
        memset(pDecoder, 0, sizeof(g722Decoder));
        pDecoder->detl = 32;
        pDecoder->deth = 8;
        pDecoder->channels = 1;
    }
}



static void sb_decoder_codeword_unpacking(char x, int *ihigh, int *ilow)
{
    *ilow =  x & 63;
    *ihigh = (x >> 6) & 3;
    return;
}

//block5L
static void sb_decoder_recon_lowband(g722Decoder *dec, int ilow, int *rl)
{
    int ril, dl, wd2, yl ;
    static int qm4[16] =
    {
        0,	-20456,	-12896,	-8968,
        -6288,	-4240,	-2584,	-1200,
        20456,	12896,	8968,	6288,
        4240,	2584,	1200,	0
    } ;
    static int qm5[32] =
    {
        -280,	-280,	-23352,	-17560,
        -14120,	-11664,	-9752,	-8184,
        -6864,	-5712,	-4696,	-3784,
        -2960,	-2208,	-1520,	-880,
        23352,	17560,	14120,	11664,
        9752,	8184,	6864,	5712,
        4696,	3784,	2960,	2208,
        1520,	880,	280,	-280
    } ;
    static int qm6[64] =
    {
        -136,	-136,	-136,	-136,
        -24808,	-21904,	-19008,	-16704,
        -14984,	-13512,	-12280,	-11192,
        -10232,	-9360,	-8576,	-7856,
        -7192,	-6576,	-6000,	-5456,
        -4944,	-4464,	-4008,	-3576,
        -3168,	-2776,	-2400,	-2032,
        -1688,	-1360,	-1040,	-728,
        24808,	21904,	19008,	16704,
        14984,	13512,	12280,	11192,
        10232,	9360,	8576,	7856,
        7192,	6576,	6000,	5456,
        4944,	4464,	4008,	3576,
        3168,	2776,	2400,	2032,
        1688,	1360,	1040,	728,
        432,	136,	-432,	-136
    } ;

    if(dec->mode == G722_DECODER_64K)
    {
        ril = ilow ;
        wd2 = qm6[ril] ;
    }


    if(dec->mode == G722_DECODER_56K)
    {
        ril = ilow >> 1 ;
        wd2 = qm5[ril] ;
    }


    if(dec->mode == G722_DECODER_48K)
    {
        ril = ilow >> 2 ;
        wd2 = qm4[ril] ;
    }

    dl = (dec->detl * wd2) >> 15 ;


    yl = dec->sl + dl ;


    if(yl > 16383) yl = 16383;
    if(yl < -16384) yl = -16384;

    *rl = yl;

}


//block2L
static void sb_decoder_inquant_lowband(g722Decoder *dec, int ilow, int *dlt)
{
    int ril, wd2 ;
    static int qm4[16] =
    {
        0,	-20456,	-12896,	-8968,
        -6288,	-4240,	-2584,	-1200,
        20456,	12896,	8968,	6288,
        4240,	2584,	1200,	0
    } ;

    ril = ilow >> 2 ;
    wd2 = qm4[ril] ;
    *dlt = (dec->detl * wd2) >> 15 ;

    return;
}

// block3L
static void sb_decoder_quantadapt_lowband(g722Decoder *dec, int il)
{

    int ril, il4, wd, wd1, wd2, wd3, nbpl, depl ;
    static int wl[8] = { -60, -30, 58, 172, 334, 538, 1198, 3042 } ;
    static int rl42[16] = {0, 7, 6, 5, 4, 3, 2, 1, 7, 6, 5, 4, 3, 2,
                           1, 0
                          } ;
    static int ilb[32] = {2048, 2093, 2139, 2186, 2233, 2282, 2332,
                          2383, 2435, 2489, 2543, 2599, 2656, 2714,
                          2774, 2834, 2896, 2960, 3025, 3091, 3158,
                          3228, 3298, 3371, 3444, 3520, 3597, 3676,
                          3756, 3838, 3922, 4008
                         } ;

    ril = il >> 2 ;
    il4 = rl42[ril] ;
    wd = (dec->nbl * 32512) >> 15 ;
    nbpl = wd + wl[il4] ;

    if(nbpl <     0) nbpl = 0 ;
    if(nbpl > 18432) nbpl = 18432 ;

    wd1 = (nbpl >> 6) & 31 ;
    wd2 = nbpl >> 11 ;
    if((8 - wd2) < 0)    wd3 = ilb[wd1] << (wd2 - 8) ;
    else   wd3 = ilb[wd1] >> (8 - wd2) ;
    depl = wd3 << 2 ;
    dec->nbl = nbpl ;
    dec->detl = depl ;

    return;
}

// block4L
static void sb_decoder_adaptpred_lowband(g722Decoder *dec, int dl)
{
    int i ;
    int wd, wd1, wd2, wd3, wd4, wd5 ;

    dec->dlt[0] = dl;

    dec->rlt[0] = dec->sl + dec->dlt[0] ;
    if(dec->rlt[0] > 32767) dec->rlt[0] = 32767;
    if(dec->rlt[0] < -32768) dec->rlt[0] = -32768;


    dec->plt[0] = dec->dlt[0] + dec->szl ;
    if(dec->plt[0] > 32767) dec->plt[0] = 32767;
    if(dec->plt[0] < -32768) dec->plt[0] = -32768;


    dec->sg[0] = dec->plt[0] >> 15 ;
    dec->sg[1] = dec->plt[1] >> 15 ;
    dec->sg[2] = dec->plt[2] >> 15 ;

    wd1 = dec->al[1] + dec->al[1] ;
    wd1 = wd1 + wd1 ;
    if(wd1 > 32767) wd1 = 32767;
    if(wd1 < -32768) wd1 = -32768;

    if(dec->sg[0] == dec->sg[1])  wd2 = - wd1 ;
    else  wd2 = wd1 ;
    if(wd2 > 32767) wd2 = 32767;

    wd2 = wd2 >> 7 ;

    if(dec->sg[0] == dec->sg[2])  wd3 = 128 ;
    else  wd3 = - 128 ;

    wd4 = wd2 + wd3 ;
    wd5 = (dec->al[2] * 32512) >> 15 ;

    dec->apl[2] = wd4 + wd5 ;
    if(dec->apl[2] >  12288)  dec->apl[2] =  12288 ;
    if(dec->apl[2] < -12288)  dec->apl[2] = -12288 ;

    dec->sg[0] = dec->plt[0] >> 15 ;
    dec->sg[1] = dec->plt[1] >> 15 ;

    if(dec->sg[0] == dec->sg[1])  wd1 = 192 ;
    if(dec->sg[0] != dec->sg[1])  wd1 = - 192 ;

    wd2 = (dec->al[1] * 32640) >> 15 ;

    dec->apl[1] = wd1 + wd2 ;
    if(dec->apl[1] > 32767) dec->apl[1] = 32767;
    if(dec->apl[1] < -32768) dec->apl[1] = -32768;

    wd3 = (15360 - dec->apl[2]) ;
    if(wd3 > 32767) wd3 = 32767;
    if(wd3 < -32768) wd3 = -32768;
    if(dec->apl[1] >  wd3)  dec->apl[1] =  wd3 ;
    if(dec->apl[1] < -wd3)  dec->apl[1] = -wd3 ;


    if(dec->dlt[0] == 0)  wd1 = 0 ;
    if(dec->dlt[0] != 0)  wd1 = 128 ;

    dec->sg[0] = dec->dlt[0] >> 15 ;

    for(i = 1; i < 7; i++)
    {
        dec->sg[i] = dec->dlt[i] >> 15 ;
        if(dec->sg[i] == dec->sg[0])  wd2 = wd1 ;
        if(dec->sg[i] != dec->sg[0])  wd2 = - wd1 ;
        wd3 = (dec->bl[i] * 32640) >> 15 ;
        dec->bpl[i] = wd2 + wd3 ;
        if(dec->bpl[i] > 32767) dec->bpl[i] = 32767;
        if(dec->bpl[i] < -32768) dec->bpl[i] = -32768;
    }
    for(i = 6; i > 0; i--)
    {
        dec->dlt[i] = dec->dlt[i - 1] ;
        dec->bl[i]  = dec->bpl[i] ;
    }

    for(i = 2; i > 0; i--)
    {
        dec->rlt[i] = dec->rlt[i - 1] ;
        dec->plt[i] = dec->plt[i - 1] ;
        dec->al[i] = dec->apl[i] ;
    }

    wd1 = (dec->rlt[1] + dec->rlt[1]) ;
    if(wd1 > 32767) wd1 = 32767;
    if(wd1 < -32768) wd1 = -32768;
    wd1 = (dec->al[1] * wd1) >> 15 ;

    wd2 = (dec->rlt[2] + dec->rlt[2]) ;
    if(wd2 > 32767) wd2 = 32767;
    if(wd2 < -32768) wd2 = -32768;

    wd2 = (dec->al[2] * wd2) >> 15 ;

    dec->spl = wd1 + wd2 ;
    if(dec->spl > 32767) dec->spl = 32767;
    if(dec->spl < -32768) dec->spl = -32768;


    dec->szl = 0 ;
    for(i = 6; i > 0; i--)
    {
        wd = (dec->dlt[i] + dec->dlt[i]) ;
        if(wd > 32767) wd = 32767;
        if(wd < -32768) wd = -32768;
        dec->szl += (dec->bl[i] * wd) >> 15 ;

    }
    if(dec->szl > 32767)dec->szl = 32767;
    if(dec->szl < -32768) dec->szl = -32768;

    dec->sl = dec->spl + dec->szl ;
    if(dec->sl > 32767) dec->sl = 32767;
    if(dec->sl < -32768) dec->sl = -32768;

}
static void sb_decoder_adpcmDecodeLowband(g722Decoder *dec, int ilowr, int *rl)
{

    int rlow, dlowt;

    sb_decoder_recon_lowband(dec, ilowr, &rlow);

    *rl = rlow;

    sb_decoder_inquant_lowband(dec, ilowr, &dlowt);

    sb_decoder_quantadapt_lowband(dec, ilowr);

    sb_decoder_adaptpred_lowband(dec, dlowt);

    return;
}

// block2H
static void sb_decoder_inquant_highband(g722Decoder *dec, int ih, int *dh)
{

    static int qm2[4] =
    { -7408, -1616,  7408,   1616} ;

    *dh = (dec->deth * qm2[ih]) >> 15 ;

}

// block3H
static void sb_decoder_quantadapt_highband(g722Decoder *dec, int ih)
{
    int ih2, wd, wd1, wd2, wd3;
    static int wh[3] = {0, -214, 798} ;
    static int rh2[4] = {2, 1, 2, 1} ;
    static int ilb[32] = {2048, 2093, 2139, 2186, 2233, 2282, 2332,
                          2383, 2435, 2489, 2543, 2599, 2656, 2714,
                          2774, 2834, 2896, 2960, 3025, 3091, 3158,
                          3228, 3298, 3371, 3444, 3520, 3597, 3676,
                          3756, 3838, 3922, 4008
                         } ;

    //int fd;
    //short  *datap;



    ih2 = rh2[ih] ;
    wd = (dec->nbh * 127) >> 7 ;
    dec->nbh = wd + wh[ih2] ;

    if(dec->nbh <     0) dec->nbh = 0 ;
    else if(dec->nbh > 22528) dec->nbh = 22528 ;

    wd1 = (dec->nbh >> 6) & 31 ;
    wd2 = dec->nbh >> 11 ;
    wd3 = ((10 - wd2) < 0) ? ilb[wd1] << (wd2 - 10) : ilb[wd1] >> (10 - wd2) ;
    dec->deth = wd3 << 2 ;

}

// block4H
static void sb_decoder_adaptpred_highband(g722Decoder *dec, int dhigh)
{
    int i ;
    int wd, wd1, wd2, wd3, wd4, wd5;
    //int wd6, wd7;

    dec->dh[0] = dhigh;

    dec->rh[0] = dec->sh + dec->dh[0] ;
    if(dec->rh[0] > 32767) dec->rh[0] = 32767;
    if(dec->rh[0] < -32768) dec->rh[0] = -32768;


    dec->ph[0] = dec->dh[0] + dec->szh ;
    if(dec->ph[0] > 32767) dec->ph[0] = 32767;
    if(dec->ph[0] < -32768) dec->ph[0] = -32768;


    dec->sgh[0] = dec->ph[0] >> 15 ;
    dec->sgh[1] = dec->ph[1] >> 15 ;
    dec->sgh[2] = dec->ph[2] >> 15 ;

    wd1 = dec->ah[1] + dec->ah[1] ;
    wd1 = wd1 + wd1 ;
    if(wd1 > 32767) wd1 = 32767;
    if(wd1 < -32768) wd1 = -32768;

    if(dec->sgh[0] == dec->sgh[1])  wd2 = - wd1 ;
    else  wd2 = wd1 ;
    if(wd2 > 32767) wd2 = 32767;

    wd2 = wd2 >> 7 ;

    if(dec->sgh[0] == dec->sgh[2])  wd3 = 128 ;
    else  wd3 = - 128 ;

    wd4 = wd2 + wd3 ;
    wd5 = (dec->ah[2] * 32512) >> 15 ;

    dec->aph[2] = wd4 + wd5 ;
    if(dec->aph[2] >  12288)  dec->aph[2] =  12288 ;
    if(dec->aph[2] < -12288)  dec->aph[2] = -12288 ;

    dec->sgh[0] = dec->ph[0] >> 15 ;
    dec->sgh[1] = dec->ph[1] >> 15 ;

    if(dec->sgh[0] == dec->sgh[1])  wd1 = 192 ;
    if(dec->sgh[0] != dec->sgh[1])  wd1 = - 192 ;

    wd2 = (dec->ah[1] * 32640) >> 15 ;

    dec->aph[1] = wd1 + wd2 ;
    if(dec->aph[1] > 32767) dec->aph[1] = 32767;
    if(dec->aph[1] < -32768) dec->aph[1] = -32768;

    wd3 = (15360 - dec->aph[2]) ;
    if(wd3 > 32767) wd3 = 32767;
    if(wd3 < -32768) wd3 = -32768;
    if(dec->aph[1] >  wd3)  dec->aph[1] =  wd3 ;
    if(dec->aph[1] < -wd3)  dec->aph[1] = -wd3 ;


    if(dec->dh[0] == 0)  wd1 = 0 ;
    if(dec->dh[0] != 0)  wd1 = 128 ;

    dec->sgh[0] = dec->dh[0] >> 15 ;

    for(i = 1; i < 7; i++)
    {
        dec->sgh[i] = dec->dh[i] >> 15 ;
        if(dec->sgh[i] == dec->sgh[0])  wd2 = wd1 ;
        if(dec->sgh[i] != dec->sgh[0])  wd2 = - wd1 ;
        wd3 = (dec->bh[i] * 32640) >> 15 ;
        dec->bph[i] = wd2 + wd3 ;
        if(dec->bph[i] > 32767) dec->bph[i] = 32767;
        if(dec->bph[i] < -32768) dec->bph[i] = -32768;

    }
    for(i = 6; i > 0; i--)
    {
        dec->dh[i] = dec->dh[i - 1] ;
        dec->bh[i]  = dec->bph[i] ;
    }

    for(i = 2; i > 0; i--)
    {
        dec->rh[i] = dec->rh[i - 1] ;
        dec->ph[i] = dec->ph[i - 1] ;
        dec->ah[i] = dec->aph[i] ;
    }

    wd1 = (dec->rh[1] + dec->rh[1]) ;
    if(wd1 > 32767) wd1 = 32767;
    if(wd1 < -32768) wd1 = -32768;
    wd1 = (dec->ah[1] * wd1) >> 15 ;

    wd2 = (dec->rh[2] + dec->rh[2]) ;
    if(wd2 > 32767) wd2 = 32767;
    if(wd2 < -32768) wd2 = -32768;
    wd2 = (dec->ah[2] * wd2) >> 15 ;

    dec->sph = wd1 + wd2 ;
    if(dec->sph > 32767) dec->sph = 32767;
    if(dec->sph < -32768) dec->sph = -32768;

    dec->szh = 0 ;
    for(i = 6; i > 0; i--)
    {
        wd = (dec->dh[i] + dec->dh[i]) ;
        if(wd > 32767) wd = 32767;
        if(wd < -32768) wd = -32768;
        dec->szh += (dec->bh[i] * wd) >> 15 ;

    }
    if(dec->szh > 32767) dec->szh = 32767;
    if(dec->szh < -32768) dec->szh = -32768;

    dec->sh = dec->sph + dec->szh ;
    if(dec->sh > 32767) dec->sh = 32767;
    if(dec->sh < -32768) dec->sh = -32768;


}

static void sb_decoder_recon_highband(g722Decoder *dec, int dhigh, int *rhigh)
{

    *rhigh = dhigh + dec->sh;
    if(*rhigh >  16383)  *rhigh =  16383 ;
    if(*rhigh < -16384)  *rhigh = -16384 ;

    return;
}

static void sb_decoder_adpcmDecodeHighband(g722Decoder *dec, int ihigh, int *rhigh)
{

    int rh, dhigh;

    sb_decoder_inquant_highband(dec, ihigh, &dhigh);

    sb_decoder_recon_highband(dec, dhigh, &rh);

    *rhigh = rh;
    sb_decoder_quantadapt_highband(dec, ihigh);

    sb_decoder_adaptpred_highband(dec, dhigh);

}

static void sb_decoder_rx_qmf(g722Decoder *dec, int rl, int rh, short *x0, short *x1)
{

    int i;
    int *xd;
    int *xs;

    int sum1, sum2;

    static int h[24] =  {3,	-11,	-11,	53,	12,	-156,
                         32,	362,	-210,	-805,	951,	3876,
                         3876,	951,	-805,	-210,	362,	32,
                         -156,	12,	53,	-11,	-11,	3
                        } ;

    xd = dec->xd;
    xs = dec->xs;

    for(i = 11; i > 0; i--)
    {
        xd[i] = xd[i - 1];
        xs[i] = xs[i - 1];
    }
    xd[0] = rl - rh ;
    // 	if (xd[0] > 16383) xd[0] = 16383;
    // 	if (xd[0] < -16384) xd[0] = -16384;
    xs[0] = rl + rh ;
    // 	if (xs[0] > 16383) xs[0] = 16383;
    // 	if (xs[0] < -16384) xs[0] = -16384;

    sum1 = 0;

    int tmpx0 = 0;
    int tmpx1 = 0;

    for(i = 0; i < 12; i += 1) sum1 += xd[i] * h[2 * i];
    tmpx0 = sum1 >> 11 ;
    *x0 = tmpx0;
    if(tmpx0 >  32767)  *x0 =  32767 ;
    if(tmpx0 < -32768)  *x0 = -32768 ;

    sum2 = 0;
    for(i = 0; i < 12; i += 1) sum2 += xs[i] * h[2 * i + 1];
    tmpx1 = sum2 >> 11 ;
    *x1 = tmpx1;
    if(tmpx1 >  32767)  *x1 =  32767 ;
    if(tmpx1 < -32768)  *x1 = -32768 ;

    return;
}


int g722_decode(const unsigned char *data, short *pcm, G722_DECODER_MODE mode, int enc_size, g722Decoder *decoder)
{
    int i, j;
    int ihigh, ilowr, rlow, rhigh;
    short x0, x1;
    unsigned char decin;

    if(!decoder) return G722_DECODER_ALLOC_ERR;

    if(!data || !pcm) return G722_DECODER_BUFFER_ERR;

    for(i = 0, j = 0; i < enc_size; i += decoder->channels)
    {
        decin = data[i];
        sb_decoder_codeword_unpacking(decin, &ihigh, &ilowr);

        sb_decoder_adpcmDecodeLowband(decoder, ilowr, &rlow);

        sb_decoder_adpcmDecodeHighband(decoder, ihigh, &rhigh);

        sb_decoder_rx_qmf(decoder, rlow, rhigh, &x0, &x1);

        pcm[j] = x0;
        j += decoder->channels;
        pcm[j] = x1;
        j += decoder->channels;
    }
    return j;

}

/* .................... end of g722_decode() .......................... */

