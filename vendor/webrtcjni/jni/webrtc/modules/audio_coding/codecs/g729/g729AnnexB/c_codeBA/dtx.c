/* ITU-T G.729 Software Package Release 2 (November 2006) */
/*
   ITU-T G.729A Annex B     ANSI-C Source Code
   Version 1.3    Last modified: August 1997
   Copyright (c) 1996, France Telecom, Rockwell International,
                       Universite de Sherbrooke.
   All rights reserved.
*/

/* DTX and Comfort Noise Generator - Encoder part */

#include <stdio.h>
#include <stdlib.h>

#include "typedef.h"
#include "basic_op.h"
#include "ld8a.h"
#include "oper_32b.h"
#include "tab_ld8a.h"
#include "vad.h"
#include "dtx.h"
#include "tab_dtx.h"
#include "sid.h"


/* Local functions */
static void Calc_pastfilt(CodState *coder, Word16 *Coeff);
static void Calc_RCoeff(Word16 *Coeff, Word16 *RCoeff, Word16 *sh_RCoeff);
static Word16 Cmp_filt(Word16 *RCoeff, Word16 sh_RCoeff, Word16 *acf,
                                        Word16 alpha, Word16 Fracthresh);
static void Calc_sum_acf(Word16 *acf, Word16 *sh_acf,
                    Word16 *sum, Word16 *sh_sum, Word16 nb);
static void Update_sumAcf(CodState *coder);

/*-----------------------------------------------------------*
 * procedure Init_Cod_cng:                                   *
 *           ~~~~~~~~~~~~                                    *
 *   Initialize variables used for dtx at the encoder        *
 *-----------------------------------------------------------*/
void Init_Cod_cng(CodState *coder)
{
  Word16 i;

  for(i=0; i<SIZ_SUMACF; i++) coder->sumAcf[i] = 0;
  for(i=0; i<NB_SUMACF; i++) coder->sh_sumAcf[i] = 40;

  for(i=0; i<SIZ_ACF; i++) coder->Acf[i] = 0;
  for(i=0; i<NB_CURACF; i++) coder->sh_Acf[i] = 40;

  for(i=0; i<NB_GAIN; i++) coder->sh_ener[i] = 40;
  for(i=0; i<NB_GAIN; i++) coder->ener[i] = 0;

  coder->cur_gain = 0;
  coder->fr_cur = 0;
  coder->flag_chang = 0;

  return;
}


/*-----------------------------------------------------------*
 * procedure Cod_cng:                                        *
 *           ~~~~~~~~                                        *
 *   computes DTX decision                                   *
 *   encodes SID frames                                      *
 *   computes CNG excitation for encoder update              *
 *-----------------------------------------------------------*/
void Cod_cng(
  CodState *coder,
  Word16 *Aq,           /* (o)   : set of interpolated LPC coefficients */
  Word16 *ana,          /* (o)   : coded SID parameters                 */
  Word16 freq_prev[MA_NP][M]
                        /* (i/o) : previous LPS for quantization        */
)
{

  Word16 i;

  Word16 curAcf[MP1];
  Word16 bid[M], zero[MP1];
  Word16 curCoeff[MP1];
  Word16 lsp_new[M];
  Word16 *lpcCoeff;
  Word16 cur_igain;
  Word16 energyq, temp;
  Flag   Overflow = 0;

  /* Update Ener and sh_ener */
  for(i = NB_GAIN-1; i>=1; i--) {
    coder->ener[i] = coder->ener[i-1];
    coder->sh_ener[i] = coder->sh_ener[i-1];
  }

  /* Compute current Acfs */
  Calc_sum_acf(coder->Acf, coder->sh_Acf, curAcf, &coder->sh_ener[0], NB_CURACF);

  /* Compute LPC coefficients and residual energy */
  if(curAcf[0] == 0) {
    coder->ener[0] = 0;                /* should not happen */
  }
  else {
    Set_zero(zero, MP1);
    Levinson(coder, curAcf, zero, curCoeff, bid, &coder->ener[0]);
  }

  /* if first frame of silence => SID frame */
  if(coder->pastVad != 0) {
    ana[0] = 2;
    coder->count_fr0 = 0;
    coder->nb_ener = 1;
    Qua_Sidgain(coder->ener, coder->sh_ener, coder->nb_ener, &energyq, &cur_igain);

  }
  else {
    coder->nb_ener = add(coder->nb_ener, 1, &Overflow);
    if(sub(coder->nb_ener, NB_GAIN, &Overflow) > 0) coder->nb_ener = NB_GAIN;
    Qua_Sidgain(coder->ener, coder->sh_ener, coder->nb_ener, &energyq, &cur_igain);
      
    /* Compute stationarity of current filter   */
    /* versus reference filter                  */
    if(Cmp_filt(coder->RCoeff, coder->sh_RCoeff, curAcf, coder->ener[0], FRAC_THRESH1) != 0) {
      coder->flag_chang = 1;
    }
      
    /* compare energy difference between current frame and last frame */
    temp = abs_s(sub(coder->dtx_prev_energy, energyq, &Overflow));
    temp = sub(temp, 2, &Overflow);
    if (temp > 0) coder->flag_chang = 1;
      
    coder->count_fr0 = add(coder->count_fr0, 1, &Overflow);
    if(sub(coder->count_fr0, FR_SID_MIN, &Overflow) < 0) {
      ana[0] = 0;               /* no transmission */
    }
    else {
      if(coder->flag_chang != 0) {
        ana[0] = 2;             /* transmit SID frame */
      }
      else{
        ana[0] = 0;
      }
        
      coder->count_fr0 = FR_SID_MIN;   /* to avoid overflow */
    }
  }


  if(sub(ana[0], 2, &Overflow) == 0) {
      
    /* Reset frame count and change flag */
    coder->count_fr0 = 0;
    coder->flag_chang = 0;
      
    /* Compute past average filter */
    Calc_pastfilt(coder, coder->pastCoeff);
    Calc_RCoeff(coder->pastCoeff, coder->RCoeff, &coder->sh_RCoeff);

    /* Compute stationarity of current filter   */
    /* versus past average filter               */


    /* if stationary */
    /* transmit average filter => new ref. filter */
    if(Cmp_filt(coder->RCoeff, coder->sh_RCoeff, curAcf, coder->ener[0], FRAC_THRESH2) == 0) {
      lpcCoeff = coder->pastCoeff;
    }

    /* else */
    /* transmit current filter => new ref. filter */
    else {
      lpcCoeff = curCoeff;
      Calc_RCoeff(curCoeff, coder->RCoeff, &coder->sh_RCoeff);
    }

    /* Compute SID frame codes */

    Az_lsp(lpcCoeff, lsp_new, coder->lsp_old_q); /* From A(z) to lsp */

    /* LSP quantization */
    lsfq_noise(lsp_new, coder->lspSid_q, freq_prev, &ana[1]);

    coder->dtx_prev_energy = energyq;
    ana[4] = cur_igain;
    coder->sid_gain = tab_Sidgain[cur_igain];


  } /* end of SID frame case */

  /* Compute new excitation */
  if(coder->pastVad != 0) {
    coder->cur_gain = coder->sid_gain;
  }
  else {
    coder->cur_gain = mult_r(coder->cur_gain, A_GAIN0);
    coder->cur_gain = add(coder->cur_gain, mult_r(coder->sid_gain, A_GAIN1), &Overflow);
  }

  Calc_exc_rand(coder->cur_gain, coder->exc, &coder->seed, coder->L_exc_err);

  Int_qlpc(coder->lsp_old_q, coder->lspSid_q, Aq);
  for(i=0; i<M; i++) {
    coder->lsp_old_q[i]   = coder->lspSid_q[i];
  }

  /* Update sumAcf if fr_cur = 0 */
  if(coder->fr_cur == 0) {
    Update_sumAcf(coder);
  }

  return;
}

/*-----------------------------------------------------------*
 * procedure Update_cng:                                     *
 *           ~~~~~~~~~~                                      *
 *   Updates autocorrelation arrays                          *
 *   used for DTX/CNG                                        *
 *   If Vad=1 : updating of array sumAcf                     *
 *-----------------------------------------------------------*/
void Update_cng(
  CodState *coder,
  Word16 *r_h,      /* (i) :   MSB of frame autocorrelation        */
  Word16 exp_r,     /* (i) :   scaling factor associated           */
  Word16 Vad        /* (i) :   current Vad decision                */
)
{
  Word16 i;
  Word16 *ptr1, *ptr2;
  Flag   Overflow = 0;

  /* Update Acf and shAcf */
  ptr1 = coder->Acf + SIZ_ACF - 1;
  ptr2 = ptr1 - MP1;
  for(i=0; i<(SIZ_ACF-MP1); i++) {
    *ptr1-- = *ptr2--;
  }
  for(i=NB_CURACF-1; i>=1; i--) {
    coder->sh_Acf[i] = coder->sh_Acf[i-1];
  }

  /* Save current Acf */
  coder->sh_Acf[0] = negate(add(16, exp_r, &Overflow));
  for(i=0; i<MP1; i++) {
    coder->Acf[i] = r_h[i];
  }

  coder->fr_cur = add(coder->fr_cur, 1, &Overflow);
  if(sub(coder->fr_cur, NB_CURACF, &Overflow) == 0) {
    coder->fr_cur = 0;
    if(Vad != 0) {
      Update_sumAcf(coder);
    }
  }

  return;
}


/*-----------------------------------------------------------*
 *         Local procedures                                  *
 *         ~~~~~~~~~~~~~~~~                                  *
 *-----------------------------------------------------------*/

/* Compute scaled autocorr of LPC coefficients used for Itakura distance */
/*************************************************************************/
static void Calc_RCoeff(Word16 *Coeff, Word16 *RCoeff, Word16 *sh_RCoeff)
{
  Word16 i, j;
  Word16 sh1;
  Word32 L_acc;
  Flag   Overflow = 0;
  
  /* RCoeff[0] = SUM(j=0->M) Coeff[j] ** 2 */
  L_acc = 0L;
  for(j=0; j <= M; j++) {
    L_acc = L_mac(L_acc, Coeff[j], Coeff[j], &Overflow);
  }
  
  /* Compute exponent RCoeff */
  sh1 = norm_l(L_acc);
  L_acc = L_shl(L_acc, sh1, &Overflow);
  RCoeff[0] = W_round(L_acc, &Overflow);
  
  /* RCoeff[i] = SUM(j=0->M-i) Coeff[j] * Coeff[j+i] */
  for(i=1; i<=M; i++) {
    L_acc = 0L;
    for(j=0; j<=M-i; j++) {
      L_acc = L_mac(L_acc, Coeff[j], Coeff[j+i], &Overflow);
    }
    L_acc = L_shl(L_acc, sh1, &Overflow);
    RCoeff[i] = W_round(L_acc, &Overflow);
  }
  *sh_RCoeff = sh1;
  return;
}

/* Compute Itakura distance and compare to threshold */
/*****************************************************/
static Word16 Cmp_filt(Word16 *RCoeff, Word16 sh_RCoeff, Word16 *acf,
                                        Word16 alpha, Word16 FracThresh)
{
  Word32 L_temp0, L_temp1;
  Word16 temp1, temp2, sh[2], ind;
  Word16 i;
  Word16 diff, flag;
  Flag Overflow;

  sh[0] = 0;
  sh[1] = 0;
  ind = 1;
  flag = 0;
  do {
    Overflow = 0;
    temp1 = shr(RCoeff[0], sh[0], &Overflow);
    temp2 = shr(acf[0], sh[1], &Overflow);
    L_temp0 = L_shr(L_mult(temp1, temp2, &Overflow),1, &Overflow);
    for(i=1; i <= M; i++) {
      temp1 = shr(RCoeff[i], sh[0], &Overflow);
      temp2 = shr(acf[i], sh[1], &Overflow);
      L_temp0 = L_mac(L_temp0, temp1, temp2, &Overflow);
    }
    if(Overflow != 0) {
      sh[(int)ind] = add(sh[(int)ind], 1, &Overflow);
      ind = sub(1, ind, &Overflow);
    }
    else flag = 1;
  } while (flag == 0);
  
  
  temp1 = mult_r(alpha, FracThresh);
  L_temp1 = L_add(L_deposit_l(temp1), L_deposit_l(alpha), &Overflow);
  temp1 = add(sh_RCoeff, 9, &Overflow);  /* 9 = Lpc_justif. * 2 - 16 + 1 */
  temp2 = add(sh[0], sh[1], &Overflow);
  temp1 = sub(temp1, temp2, &Overflow);
  L_temp1 = L_shl(L_temp1, temp1, &Overflow);
  
  L_temp0 = L_sub(L_temp0, L_temp1, &Overflow);
  if(L_temp0 > 0L) diff = 1;
  else diff = 0;

  return(diff);
}

/* Compute past average filter */
/*******************************/
static void Calc_pastfilt(CodState *coder, Word16 *Coeff)
{
  Word16 i;
  Word16 s_sumAcf[MP1];
  Word16 bid[M], zero[MP1];
  Word16 temp;
  
  Calc_sum_acf(coder->sumAcf, coder->sh_sumAcf, s_sumAcf, &temp, NB_SUMACF);
  
  if(s_sumAcf[0] == 0L) {
    Coeff[0] = 4096;
    for(i=1; i<=M; i++) Coeff[i] = 0;
    return;
  }

  Set_zero(zero, MP1);
  Levinson(coder, s_sumAcf, zero, Coeff, bid, &temp);
  return;
}

/* Update sumAcf */
/*****************/
static void Update_sumAcf(CodState *coder)
{
  Word16 *ptr1, *ptr2;
  Word16 i;

  /*** Move sumAcf ***/
  ptr1 = coder->sumAcf + SIZ_SUMACF - 1;
  ptr2 = ptr1 - MP1;
  for(i=0; i<(SIZ_SUMACF-MP1); i++) {
    *ptr1-- = *ptr2--;
  }
  for(i=NB_SUMACF-1; i>=1; i--) {
    coder->sh_sumAcf[i] = coder->sh_sumAcf[i-1];
  }

  /* Compute new sumAcf */
  Calc_sum_acf(coder->Acf, coder->sh_Acf, coder->sumAcf, coder->sh_sumAcf, NB_CURACF);
  return;
}

/* Compute sum of acfs (curAcf, sumAcf or s_sumAcf) */
/****************************************************/
static void Calc_sum_acf(Word16 *acf, Word16 *sh_acf,
                         Word16 *sum, Word16 *sh_sum, Word16 nb)
{

  Word16 *ptr1;
  Word32 L_temp, L_tab[MP1];
  Word16 sh0, temp;
  Word16 i, j;
  Flag   Overflow = 0;
  
  /* Compute sum = sum of nb acfs */
  /* Find sh_acf minimum */
  sh0 = sh_acf[0];
  for(i=1; i<nb; i++) {
    if(sub(sh_acf[i], sh0, &Overflow) < 0) sh0 = sh_acf[i];
  }
  sh0 = add(sh0, 14, &Overflow);           /* 2 bits of margin */

  for(j=0; j<MP1; j++) {
    L_tab[j] = 0L;
  }
  ptr1 = acf;
  for(i=0; i<nb; i++) {
    temp = sub(sh0, sh_acf[i], &Overflow);
    for(j=0; j<MP1; j++) {
      L_temp = L_deposit_l(*ptr1++);
      L_temp = L_shl(L_temp, temp, &Overflow); /* shift right if temp<0 */
      L_tab[j] = L_add(L_tab[j], L_temp, &Overflow);
    }
  } 
  temp = norm_l(L_tab[0]);
  for(i=0; i<=M; i++) {
    sum[i] = extract_h(L_shl(L_tab[i], temp, &Overflow));
  }
  temp = sub(temp, 16, &Overflow);
  *sh_sum = add(sh0, temp, &Overflow);
  return;
}
