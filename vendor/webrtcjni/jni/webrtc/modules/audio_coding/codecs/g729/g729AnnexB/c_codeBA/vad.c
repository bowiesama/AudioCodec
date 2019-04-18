/* ITU-T G.729 Software Package Release 2 (November 2006) */
/*
   ITU-T G.729A Annex B     ANSI-C Source Code
   Version 1.3    Last modified: August 1997
   Copyright (c) 1996, France Telecom, Rockwell International,
                       Universite de Sherbrooke.
   All rights reserved.
*/

#include <stdio.h>
#include "typedef.h"
#include "ld8a.h"
#include "basic_op.h"
#include "oper_32b.h"
#include "tab_ld8a.h"
#include "vad.h"
#include "dtx.h"
#include "tab_dtx.h"

/* local function */
static Word16 MakeDec(
               Word16 dSLE,    /* (i)  : differential low band energy */
               Word16 dSE,     /* (i)  : differential full band energy */
               Word16 SD,      /* (i)  : differential spectral distortion */
               Word16 dSZC     /* (i)  : differential zero crossing rate */
);

/*---------------------------------------------------------------------------*
 * Function  vad_init                                                        *
 * ~~~~~~~~~~~~~~~~~~                                                        *
 *                                                                           *
 * -> Initialization of variables for voice activity detection               *
 *                                                                           *
 *---------------------------------------------------------------------------*/
void vad_init(CodState *coder)
{
  /* Static vectors to zero */
  Set_zero(coder->MeanLSF, M);

  /* Initialize VAD parameters */
  coder->MeanSE = 0;
  coder->MeanSLE = 0;
  coder->MeanE = 0;
  coder->MeanSZC = 0;
  coder->count_sil = 0;
  coder->count_update = 0;
  coder->count_ext = 0;
  coder->less_count = 0;
  coder->flag = 1;
  coder->Min = MAX_16;
}


/*-----------------------------------------------------------------*
 * Functions vad                                                   *
 *           ~~~                                                   *
 * Input:                                                          *
 *   rc            : reflection coefficient                        *
 *   lsf[]         : unquantized lsf vector                        *
 *   r_h[]         : upper 16-bits of the autocorrelation vector   *
 *   r_l[]         : lower 16-bits of the autocorrelation vector   *
 *   exp_R0        : exponent of the autocorrelation vector        *
 *   sigpp[]       : preprocessed input signal                     *
 *   frm_count     : frame counter                                 *
 *   prev_marker   : VAD decision of the last frame                *
 *   pprev_marker  : VAD decision of the frame before last frame   *
 *                                                                 *
 * Output:                                                         *
 *                                                                 *
 *   marker        : VAD decision of the current frame             * 
 *                                                                 *
 *-----------------------------------------------------------------*/
void vad(
         CodState *coder,
         Word16 rc,
         Word16 *lsf, 
         Word16 *r_h,
         Word16 *r_l, 
         Word16 exp_R0,
         Word16 *sigpp,
         Word16 frm_count,
         Word16 prev_marker,
         Word16 pprev_marker,
         Word16 *marker)
{
 /* scalar */
  Word32 acc0;
  Word16 i, j, exp, frac;
  Word16 ENERGY, ENERGY_low, SD, ZC, dSE, dSLE, dSZC;
  Word16 COEF, C_COEF, COEFZC, C_COEFZC, COEFSD, C_COEFSD;
  Flag   Overflow = 0;

  /* compute the frame energy */
  acc0 = L_Comp(r_h[0], r_l[0]);
  Log2(acc0, &exp, &frac);
  acc0 = Mpy_32_16(exp, frac, 9864);
  i = sub(exp_R0, 1, &Overflow);  
  i = sub(i, 1, &Overflow);
  acc0 = L_mac(acc0, 9864, i, &Overflow);
  acc0 = L_shl(acc0, 11, &Overflow);
  ENERGY = extract_h(acc0);
  ENERGY = sub(ENERGY, 4875, &Overflow);

  /* compute the low band energy */
  acc0 = 0;
  for (i=1; i<=NP; i++)
    acc0 = L_mac(acc0, r_h[i], lbf_corr[i], &Overflow);
  acc0 = L_shl(acc0, 1, &Overflow);
  acc0 = L_mac(acc0, r_h[0], lbf_corr[0], &Overflow);
  Log2(acc0, &exp, &frac);
  acc0 = Mpy_32_16(exp, frac, 9864);
  i = sub(exp_R0, 1, &Overflow);  
  i = sub(i, 1, &Overflow);
  acc0 = L_mac(acc0, 9864, i, &Overflow);
  acc0 = L_shl(acc0, 11, &Overflow);
  ENERGY_low = extract_h(acc0);
  ENERGY_low = sub(ENERGY_low, 4875, &Overflow);
  
  /* compute SD */
  acc0 = 0;
  for (i=0; i<M; i++){
    j = sub(lsf[i], coder->MeanLSF[i], &Overflow);
    acc0 = L_mac(acc0, j, j, &Overflow);
  }
  SD = extract_h(acc0);      /* Q15 */
  
  /* compute # zero crossing */
  ZC = 0;
  for (i=ZC_START+1; i<=ZC_END; i++)
    if (mult(sigpp[i-1], sigpp[i], &Overflow) < 0)
      ZC = add(ZC, 410, &Overflow);     /* Q15 */

  /* Initialize and update Mins */
  if(sub(frm_count, 129, &Overflow) < 0){
    if (sub(ENERGY, coder->Min, &Overflow) < 0){
      coder->Min = ENERGY;
      coder->Prev_Min = ENERGY;
    }
    
    if((frm_count & 0x0007) == 0){
      i = sub(shr(frm_count,3, &Overflow),1, &Overflow);
      coder->Min_buffer[i] = coder->Min;
      coder->Min = MAX_16;
    }
  }

  if((frm_count & 0x0007) == 0){
    coder->Prev_Min = coder->Min_buffer[0];
    for (i=1; i<16; i++){
      if (sub(coder->Min_buffer[i], coder->Prev_Min, &Overflow) < 0)
        coder->Prev_Min = coder->Min_buffer[i];
    }
  }
  
  if(sub(frm_count, 129, &Overflow) >= 0){
    if(((frm_count & 0x0007) ^ (0x0001)) == 0){
      coder->Min = coder->Prev_Min;
      coder->Next_Min = MAX_16;
    }
    if (sub(ENERGY, coder->Min, &Overflow) < 0)
      coder->Min = ENERGY;
    if (sub(ENERGY, coder->Next_Min, &Overflow) < 0)
      coder->Next_Min = ENERGY;
    
    if((frm_count & 0x0007) == 0){
      for (i=0; i<15; i++)
        coder->Min_buffer[i] = coder->Min_buffer[i+1]; 
      coder->Min_buffer[15] = coder->Next_Min; 
      coder->Prev_Min = coder->Min_buffer[0];
      for (i=1; i<16; i++) 
        if (sub(coder->Min_buffer[i], coder->Prev_Min, &Overflow) < 0)
          coder->Prev_Min = coder->Min_buffer[i];
    }
    
  }

  if (sub(frm_count, INIT_FRAME, &Overflow) <= 0){
    if(sub(ENERGY, 3072, &Overflow) < 0){
      *marker = NOISE;
      coder->less_count++;
    }
    else{
      *marker = VOICE;
      acc0 = L_deposit_h(coder->MeanE);
      acc0 = L_mac(acc0, ENERGY, 1024, &Overflow);
      coder->MeanE = extract_h(acc0);
      acc0 = L_deposit_h(coder->MeanSZC);
      acc0 = L_mac(acc0, ZC, 1024, &Overflow);
      coder->MeanSZC = extract_h(acc0);
      for (i=0; i<M; i++){
        acc0 = L_deposit_h(coder->MeanLSF[i]);
        acc0 = L_mac(acc0, lsf[i], 1024, &Overflow);
        coder->MeanLSF[i] = extract_h(acc0);
      }
    }
  }
    
  if (sub(frm_count, INIT_FRAME, &Overflow) >= 0){
    if (sub(frm_count, INIT_FRAME, &Overflow) == 0){
      acc0 = L_mult(coder->MeanE, factor_fx[coder->less_count], &Overflow);
      acc0 = L_shl(acc0, shift_fx[coder->less_count], &Overflow);
      coder->MeanE = extract_h(acc0);

      acc0 = L_mult(coder->MeanSZC, factor_fx[coder->less_count], &Overflow);
      acc0 = L_shl(acc0, shift_fx[coder->less_count], &Overflow);
      coder->MeanSZC = extract_h(acc0);

      for (i=0; i<M; i++){
        acc0 = L_mult(coder->MeanLSF[i], factor_fx[coder->less_count], &Overflow);
        acc0 = L_shl(acc0, shift_fx[coder->less_count], &Overflow);
        coder->MeanLSF[i] = extract_h(acc0);
      }

      coder->MeanSE = sub(coder->MeanE, 2048, &Overflow);   /* Q11 */
      coder->MeanSLE = sub(coder->MeanE, 2458, &Overflow);  /* Q11 */
    }

    dSE = sub(coder->MeanSE, ENERGY, &Overflow);
    dSLE = sub(coder->MeanSLE, ENERGY_low, &Overflow);
    dSZC = sub(coder->MeanSZC, ZC, &Overflow);

    if(sub(ENERGY, 3072, &Overflow) < 0)
      *marker = NOISE;
    else 
      *marker = MakeDec(dSLE, dSE, SD, dSZC);

    coder->v_flag = 0;
    if((prev_marker==VOICE) && (*marker==NOISE) && (add(dSE,410, &Overflow) < 0) 
       && (sub(ENERGY, 3072, &Overflow)>0)){
      *marker = VOICE;
      coder->v_flag = 1;
    }

    if(coder->flag == 1){
      if((pprev_marker == VOICE) && 
         (prev_marker == VOICE) && 
         (*marker == NOISE) && 
         (sub(abs_s(sub(coder->vad_prev_energy,ENERGY, &Overflow)), 614, &Overflow) <= 0)){
        coder->count_ext++;
        *marker = VOICE;
        coder->v_flag = 1;
        if(sub(coder->count_ext, 4, &Overflow) <= 0)
          coder->flag=1;
        else{
          coder->count_ext=0;
          coder->flag=0;
        }
      }
    }
    else
      coder->flag=1;
    
    if(*marker == NOISE)
      coder->count_sil++;

    if((*marker == VOICE) && (sub(coder->count_sil, 10, &Overflow) > 0) && 
       (sub(sub(ENERGY,coder->vad_prev_energy, &Overflow), 614, &Overflow) <= 0)){
      *marker = NOISE;
      coder->count_sil=0;
    }

    if(*marker == VOICE)
      coder->count_sil=0;

    if ((sub(sub(ENERGY, 614, &Overflow), coder->MeanSE, &Overflow) < 0) && (sub(frm_count, 128, &Overflow) > 0)
        && (!coder->v_flag) && (sub(rc, 19661, &Overflow) < 0))
      *marker = NOISE;

    if ((sub(sub(ENERGY,614, &Overflow),coder->MeanSE, &Overflow) < 0) && (sub(rc, 24576, &Overflow) < 0)
        && (sub(SD, 83, &Overflow) < 0)){ 
      coder->count_update++;
      if (sub(coder->count_update, INIT_COUNT, &Overflow) < 0){
        COEF = 24576;
        C_COEF = 8192;
        COEFZC = 26214;
        C_COEFZC = 6554;
        COEFSD = 19661;
        C_COEFSD = 13017;
      } 
      else
        if (sub(coder->count_update, INIT_COUNT+10, &Overflow) < 0){
          COEF = 31130;
          C_COEF = 1638;
          COEFZC = 30147;
          C_COEFZC = 2621;
          COEFSD = 21299;
          C_COEFSD = 11469;
        }
        else
          if (sub(coder->count_update, INIT_COUNT+20, &Overflow) < 0){
            COEF = 31785;
            C_COEF = 983;
            COEFZC = 30802;
            C_COEFZC = 1966;
            COEFSD = 22938;
            C_COEFSD = 9830;
          }
          else
            if (sub(coder->count_update, INIT_COUNT+30, &Overflow) < 0){
              COEF = 32440;
              C_COEF = 328;
              COEFZC = 31457;
              C_COEFZC = 1311;
              COEFSD = 24576;
              C_COEFSD = 8192;
            }
            else
              if (sub(coder->count_update, INIT_COUNT+40, &Overflow) < 0){
                COEF = 32604;
                C_COEF = 164;
                COEFZC = 32440;
                C_COEFZC = 328;
                COEFSD = 24576;
                C_COEFSD = 8192;
              }
              else{
                COEF = 32604;
                C_COEF = 164;
                COEFZC = 32702;
                C_COEFZC = 66;
                COEFSD = 24576;
                C_COEFSD = 8192;
              }
      

      /* compute MeanSE */
      acc0 = L_mult(COEF, coder->MeanSE, &Overflow);
      acc0 = L_mac(acc0, C_COEF, ENERGY, &Overflow);
      coder->MeanSE = extract_h(acc0);

      /* compute MeanSLE */
      acc0 = L_mult(COEF, coder->MeanSLE, &Overflow);
      acc0 = L_mac(acc0, C_COEF, ENERGY_low, &Overflow);
      coder->MeanSLE = extract_h(acc0);

      /* compute MeanSZC */
      acc0 = L_mult(COEFZC, coder->MeanSZC, &Overflow);
      acc0 = L_mac(acc0, C_COEFZC, ZC, &Overflow);
      coder->MeanSZC = extract_h(acc0);
      
      /* compute MeanLSF */
      for (i=0; i<M; i++){
        acc0 = L_mult(COEFSD, coder->MeanLSF[i], &Overflow);
        acc0 = L_mac(acc0, C_COEFSD, lsf[i], &Overflow);
        coder->MeanLSF[i] = extract_h(acc0);
      }
    }

    if((sub(frm_count, 128, &Overflow) > 0) && (((sub(coder->MeanSE,coder->Min, &Overflow) < 0) &&
                        (sub(SD, 83, &Overflow) < 0)) || (sub(coder->MeanSE,coder->Min, &Overflow) > 2048))){
      coder->MeanSE = coder->Min;
      coder->count_update = 0;
    }
  }

  coder->vad_prev_energy = ENERGY;

}

/* local function */  
static Word16 MakeDec(
               Word16 dSLE,    /* (i)  : differential low band energy */
               Word16 dSE,     /* (i)  : differential full band energy */
               Word16 SD,      /* (i)  : differential spectral distortion */
               Word16 dSZC     /* (i)  : differential zero crossing rate */
               )
{
  Word32 acc0;
  Flag   Overflow = 0;
  
  /* SD vs dSZC */
  acc0 = L_mult(dSZC, -14680, &Overflow);          /* Q15*Q23*2 = Q39 */  
  acc0 = L_mac(acc0, 8192, -28521, &Overflow);     /* Q15*Q23*2 = Q39 */
  acc0 = L_shr(acc0, 8, &Overflow);                /* Q39 -> Q31 */
  acc0 = L_add(acc0, L_deposit_h(SD), &Overflow);
  if (acc0 > 0) return(VOICE);

  acc0 = L_mult(dSZC, 19065, &Overflow);           /* Q15*Q22*2 = Q38 */
  acc0 = L_mac(acc0, 8192, -19446, &Overflow);     /* Q15*Q22*2 = Q38 */
  acc0 = L_shr(acc0, 7, &Overflow);                /* Q38 -> Q31 */
  acc0 = L_add(acc0, L_deposit_h(SD), &Overflow);
  if (acc0 > 0) return(VOICE);

  /* dSE vs dSZC */
  acc0 = L_mult(dSZC, 20480, &Overflow);           /* Q15*Q13*2 = Q29 */
  acc0 = L_mac(acc0, 8192, 16384, &Overflow);      /* Q13*Q15*2 = Q29 */
  acc0 = L_shr(acc0, 2, &Overflow);                /* Q29 -> Q27 */
  acc0 = L_add(acc0, L_deposit_h(dSE), &Overflow);
  if (acc0 < 0) return(VOICE);

  acc0 = L_mult(dSZC, -16384, &Overflow);          /* Q15*Q13*2 = Q29 */
  acc0 = L_mac(acc0, 8192, 19660, &Overflow);      /* Q13*Q15*2 = Q29 */
  acc0 = L_shr(acc0, 2, &Overflow);                /* Q29 -> Q27 */
  acc0 = L_add(acc0, L_deposit_h(dSE), &Overflow);
  if (acc0 < 0) return(VOICE);
 
  acc0 = L_mult(dSE, 32767, &Overflow);            /* Q11*Q15*2 = Q27 */
  acc0 = L_mac(acc0, 1024, 30802, &Overflow);      /* Q10*Q16*2 = Q27 */
  if (acc0 < 0) return(VOICE);
  
  /* dSE vs SD */
  acc0 = L_mult(SD, -28160, &Overflow);            /* Q15*Q5*2 = Q22 */
  acc0 = L_mac(acc0, 64, 19988, &Overflow);        /* Q6*Q14*2 = Q22 */
  acc0 = L_mac(acc0, dSE, 512, &Overflow);         /* Q11*Q9*2 = Q22 */
  if (acc0 < 0) return(VOICE);

  acc0 = L_mult(SD, 32767, &Overflow);             /* Q15*Q15*2 = Q31 */
  acc0 = L_mac(acc0, 32, -30199, &Overflow);       /* Q5*Q25*2 = Q31 */
  if (acc0 > 0) return(VOICE);

  /* dSLE vs dSZC */
  acc0 = L_mult(dSZC, -20480, &Overflow);          /* Q15*Q13*2 = Q29 */
  acc0 = L_mac(acc0, 8192, 22938, &Overflow);      /* Q13*Q15*2 = Q29 */
  acc0 = L_shr(acc0, 2, &Overflow);                /* Q29 -> Q27 */
  acc0 = L_add(acc0, L_deposit_h(dSE), &Overflow);
  if (acc0 < 0) return(VOICE);

  acc0 = L_mult(dSZC, 23831, &Overflow);           /* Q15*Q13*2 = Q29 */
  acc0 = L_mac(acc0, 4096, 31576, &Overflow);      /* Q12*Q16*2 = Q29 */
  acc0 = L_shr(acc0, 2, &Overflow);                /* Q29 -> Q27 */
  acc0 = L_add(acc0, L_deposit_h(dSE), &Overflow);
  if (acc0 < 0) return(VOICE);
  
  acc0 = L_mult(dSE, 32767, &Overflow);            /* Q11*Q15*2 = Q27 */
  acc0 = L_mac(acc0, 2048, 17367, &Overflow);      /* Q11*Q15*2 = Q27 */
  if (acc0 < 0) return(VOICE);
  
  /* dSLE vs SD */
  acc0 = L_mult(SD, -22400, &Overflow);            /* Q15*Q4*2 = Q20 */
  acc0 = L_mac(acc0, 32, 25395, &Overflow);        /* Q5*Q14*2 = Q20 */
  acc0 = L_mac(acc0, dSLE, 256, &Overflow);        /* Q11*Q8*2 = Q20 */
  if (acc0 < 0) return(VOICE);
  
  /* dSLE vs dSE */
  acc0 = L_mult(dSE, -30427, &Overflow);           /* Q11*Q15*2 = Q27 */
  acc0 = L_mac(acc0, 256, -29959, &Overflow);      /* Q8*Q18*2 = Q27 */
  acc0 = L_add(acc0, L_deposit_h(dSLE), &Overflow);
  if (acc0 > 0) return(VOICE);

  acc0 = L_mult(dSE, -23406, &Overflow);           /* Q11*Q15*2 = Q27 */
  acc0 = L_mac(acc0, 512, 28087, &Overflow);       /* Q19*Q17*2 = Q27 */
  acc0 = L_add(acc0, L_deposit_h(dSLE), &Overflow);
  if (acc0 < 0) return(VOICE);

  acc0 = L_mult(dSE, 24576, &Overflow);            /* Q11*Q14*2 = Q26 */
  acc0 = L_mac(acc0, 1024, 29491, &Overflow);      /* Q10*Q15*2 = Q26 */
  acc0 = L_mac(acc0, dSLE, 16384, &Overflow);      /* Q11*Q14*2 = Q26 */
  if (acc0 < 0) return(VOICE);

  return (NOISE);
}




