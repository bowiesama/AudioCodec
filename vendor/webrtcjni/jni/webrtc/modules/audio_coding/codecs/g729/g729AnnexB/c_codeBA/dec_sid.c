/* ITU-T G.729 Software Package Release 2 (November 2006) */
/*
   ITU-T G.729A Annex B     ANSI-C Source Code
   Version 1.4    Last modified: November 2000

   Copyright (c) 1996, France Telecom, Rockwell International,
                       Universite de Sherbrooke.
   All rights reserved.
*/

/*
**
** File:            "dec_cng.c"
**
** Description:     Comfort noise generation
**                  performed at the decoder part
**
*/
/**** Fixed point version ***/

#include <stdio.h>
#include <stdlib.h>
#include "typedef.h"
#include "ld8a.h"
#include "tab_ld8a.h"
#include "basic_op.h"
#include "vad.h"
#include "dtx.h"
#include "sid.h"
#include "tab_dtx.h"


/*
**
** Function:        Init_Dec_cng()
**
** Description:     Initialize dec_cng static variables
**
**
*/
void Init_Dec_cng(DecState *decoder)
{
  
  decoder->sid_gain = tab_Sidgain[0];

  return;
}

/*-----------------------------------------------------------*
 * procedure Dec_cng:                                        *
 *           ~~~~~~~~                                        *
 *                     Receives frame type                   *
 *                     0  :  for untransmitted frames        *
 *                     2  :  for SID frames                  *
 *                     Decodes SID frames                    *
 *                     Computes current frame excitation     *
 *                     Computes current frame LSPs
 *-----------------------------------------------------------*/
void Dec_cng(
  DecState *decoder,
  Word16 *parm,         /* (i)   : coded SID parameters                 */
  Word16 *A_t,          /* (o)   : set of interpolated LPC coefficients */
  Word16 freq_prev[MA_NP][M]
                        /* (i/o) : previous LPS for quantization        */
)
{
  Word16 temp, ind;
  Word16 dif;
  Flag   Overflow = 0;

  dif = sub(decoder->past_ftyp, 1, &Overflow);
  
  /* SID Frame */
  /*************/
  if(parm[0] != 0) {

    decoder->sid_gain = tab_Sidgain[(int)parm[4]];           
    
    /* Inverse quantization of the LSP */
    sid_lsfq_decode(&parm[1], decoder->lspSid, freq_prev);
    
  }

  /* non SID Frame */
  /*****************/
  else {
    
    /* Case of 1st SID frame erased : quantize-decode   */
    /* energy estimate stored in sid_gain         */
    if(dif == 0) {
      Qua_Sidgain(&decoder->sid_sav, &decoder->sh_sid_sav, 0, &temp, &ind);
      decoder->sid_gain = tab_Sidgain[(int)ind];
    }
    
  }
  
  if(dif == 0) {
    decoder->cur_gain = decoder->sid_gain;
  }
  else {
    decoder->cur_gain = mult_r(decoder->cur_gain, A_GAIN0);
    decoder->cur_gain = add(decoder->cur_gain, mult_r(decoder->sid_gain, A_GAIN1), &Overflow);
  }
 
  Calc_exc_rand(decoder->cur_gain, decoder->exc, &decoder->seed, NULL);

  /* Interpolate the Lsp vectors */
  Int_qlpc(decoder->lsp_old, decoder->lspSid, A_t);
  Copy(decoder->lspSid, decoder->lsp_old, M);
  
  return;
}



/*---------------------------------------------------------------------------*
 * Function  Init_lsfq_noise                                                 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~                                                 *
 *                                                                           *
 * -> Initialization of variables for the lsf quantization in the SID        *
 *                                                                           *
 *---------------------------------------------------------------------------*/
void Init_lsfq_noise(DecState *decoder)
{
  Word16 i, j;
  Word32 acc0;
  Flag   Overflow = 0;

  /* initialize the noise_fg */
  for (i=0; i<4; i++)
    Copy(fg[0][i], noise_fg[0][i], M);
  
  for (i=0; i<4; i++)
    for (j=0; j<M; j++){
      acc0 = L_mult(fg[0][i][j], 19660, &Overflow);
      acc0 = L_mac(acc0, fg[1][i][j], 13107, &Overflow);
      noise_fg[1][i][j] = extract_h(acc0);
    }
}


void sid_lsfq_decode(Word16 *index,             /* (i) : quantized indices    */
                     Word16 *lspq,              /* (o) : quantized lsp vector */
                     Word16 freq_prev[MA_NP][M] /* (i) : memory of predictor  */
                     )
{
  Word32 acc0;
  Word16 i, j, k, lsfq[M], tmpbuf[M];
  Flag   Overflow = 0;

  /* get the lsf error vector */
  Copy(lspcb1[PtrTab_1[index[1]]], tmpbuf, M);
  for (i=0; i<M/2; i++)
    tmpbuf[i] = add(tmpbuf[i], lspcb2[PtrTab_2[0][index[2]]][i], &Overflow);
  for (i=M/2; i<M; i++)
    tmpbuf[i] = add(tmpbuf[i], lspcb2[PtrTab_2[1][index[2]]][i], &Overflow);

  /* guarantee minimum distance of 0.0012 (~10 in Q13) between tmpbuf[j] 
     and tmpbuf[j+1] */
  for (j=1; j<M; j++){
    acc0 = L_mult(tmpbuf[j-1], 16384, &Overflow);
    acc0 = L_mac(acc0, tmpbuf[j], -16384, &Overflow);
    acc0 = L_mac(acc0, 10, 16384, &Overflow);
    k = extract_h(acc0);

    if (k > 0){
      tmpbuf[j-1] = sub(tmpbuf[j-1], k, &Overflow);
      tmpbuf[j] = add(tmpbuf[j], k, &Overflow);
    }
  }
  
  /* compute the quantized lsf vector */
  Lsp_prev_compose(tmpbuf, lsfq, noise_fg[index[0]], freq_prev, 
                   noise_fg_sum[index[0]]);
  
  /* update the prediction memory */
  Lsp_prev_update(tmpbuf, freq_prev);
  
  /* lsf stability check */
  Lsp_stability(lsfq);

  /* convert lsf to lsp */
  Lsf_lsp2(lsfq, lspq, M);

}





